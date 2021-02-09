/*
 * p4iprewriter.{cc,hh}
 * Virgil Ma
 *
 * Copyright (c) 2000 Massachusetts Institute of Technology
 * Copyright (c) 2000 Mazu Networks, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

// ALWAYS INCLUDE <click/config.h> FIRST
#include <click/config.h>

#include <click/args.hh>
#include <click/error.hh>
#include <click/straccum.hh>
#include <clicknet/udp.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <arpa/inet.h>

#include "sampleiprewriter.hh"

CLICK_DECLS

SampleIPRewriter::SampleIPRewriter()
{
  in_batch_mode = BATCH_MODE_IFPOSSIBLE;
}

SampleIPRewriter::~SampleIPRewriter() {}

int SampleIPRewriter::configure(Vector<String> &conf, ErrorHandler *errh)
{
  _debug = false;
  if (Args(conf, this, errh)
          .read_mp("DEBUG", _debug)
          .consume() < 0)
    return -1;

  if (_debug)
    std::cout << _debug << std::endl;

  for (int i = 1; i < conf.size(); ++i)
  {
    SampleIPRewriterInput is;
    if (parse_input_spec(conf[i], is, i, errh) >= 0)
      _input_specs.push_back(is);

    if (_debug)
    {
      StringAccum sa;
      is.unparse(sa);
      std::cout << i << ": " << conf[i].c_str() << "\tUNPARSE: " << sa.c_str() << std::endl;
    }
  }

  return _input_specs.size() == ninputs() ? 0 : -1;
}

int SampleIPRewriter::parse_input_spec(const String &line, SampleIPRewriterInput &is,
                                       int input_number, ErrorHandler *errh)
{
  PrefixErrorHandler cerrh(errh, "input spec " + String(input_number) + ": ");
  String word, rest;
  if (!cp_word(line, &word, &rest))
    return cerrh.error("empty argument");
  cp_eat_space(rest);

  is.kind = SampleIPRewriterInput::i_drop;
  is.owner = this;

  if (word == "drop")
  {
    if (rest)
      return cerrh.error("syntax error, expected %<%s%>", word.c_str());
  }
  else if (word == "pattern")
  {
    if (!SampleIPRewriterPattern::parse(rest, &is, this, &cerrh))
      return -1;
    if ((unsigned)is.foutput >= (unsigned)noutputs() ||
        (unsigned)is.routput >= (unsigned)is.owner->noutputs())
      return cerrh.error("output port out of range");
    is.pattern->use();
    is.kind = SampleIPRewriterInput::i_pattern;
  }
  else
  {
    return cerrh.error("unknown specification");
  }

  return 0;
}


SampleIPRewriterEntry::SampleIPRewriterEntry(const IPFlowID &in, const IPFlowID &out)
    : flowid(in), rw_flowid(out) {}

String SampleIPRewriterEntry::unparse()
{
  StringAccum sa;
  sa << flowid.unparse() << "->" << rw_flowid.unparse();
  return sa.take_string();
}

void SampleIPRewriterEntry::apply(WritablePacket *p)
{
  assert(p->has_network_header());
  click_ip *iph = p->ip_header();

  // IP header
  iph->ip_src = rw_flowid.saddr();
  iph->ip_dst = rw_flowid.daddr();

  if (!IP_FIRSTFRAG(iph))
    return;
  click_udp *udph = p->udp_header(); // TCP ports in the same place
  udph->uh_sport = rw_flowid.sport();
  udph->uh_dport = rw_flowid.dport();
  iph->ip_sum = 0;
  iph->ip_sum = click_in_cksum((unsigned char *)iph, sizeof(click_ip));
}

void SampleIPRewriter::push_batch(int port, PacketBatch *batch)
{
  auto fnt = [this, port](Packet *p) { return process(port, p); };
  EXECUTE_FOR_EACH_PACKET_DROPPABLE(fnt, batch, [](Packet *p) {})
  if (batch)
    output(0).push_batch(batch);
}

void SampleIPRewriter::push(int port, Packet *p_in)
{
  if (Packet *p = process(port, p_in))
    checked_output_push(0, p);
}

Packet *SampleIPRewriter::process(int port, Packet *p_in)
{
  WritablePacket *p = p_in->uniqueify();
  click_ip *iph = p->ip_header();

  if ((iph->ip_p != IP_PROTO_TCP && iph->ip_p != IP_PROTO_UDP) ||
      !IP_FIRSTFRAG(iph) || p->transport_length() < 8)
  {
    p->kill();
    return NULL;
  }

  IPFlowID flowid(p);

  // 1. check in map
  SampleIPRewriterEntry *entry;
  auto it = _map.find(flowid);
  if (it != _map.end())
  {
    entry = it->second;
    // print the match entry
    if (_debug)
      std::cout << "[match]\t";
  }
  else
  {
    // 2. if not in map, inputspec.output
    SampleIPRewriterInput &is = _input_specs.at(port);
    IPFlowID rw_flowid = IPFlowID::uninitialized_t();
    int result = is.rewrite_flowid(flowid, rw_flowid);
    if (result == rw_addmap)
    {
      entry = add_flow(flowid, rw_flowid);
      if (!entry)
        return p;
      if (_debug)
        std::cout << "[new]\t";
    }
    else if (result == rw_drop)
      return NULL;
  }
  if (_debug)
    std::cout << entry->unparse().c_str() << std::endl;

  // update the header
  entry->apply(p);

  // output(entry->output()).push(p);
  return p;
}

SampleIPRewriterEntry *SampleIPRewriter::add_flow(const IPFlowID &flowid, const IPFlowID &rw_flowid)
{
  SampleIPRewriterEntry *entry = new SampleIPRewriterEntry(flowid, rw_flowid);
  SampleIPRewriterEntry *entry_r = new SampleIPRewriterEntry(rw_flowid.reverse(), flowid.reverse());

  _map[flowid] = entry;
  _map[rw_flowid.reverse()] = entry_r;

  return entry;
}

SampleIPRewriterInput::SampleIPRewriterInput()
    : kind(i_drop), count(0), failures(0), foutput(-1), routput(-1)
{
  pattern = 0;
}

int SampleIPRewriterInput::rewrite_flowid(const IPFlowID &flowid, IPFlowID &rw_flowid)
{
  int i;
  switch (kind)
  {
  case i_pattern:
  {
    i = pattern->rewrite_flowid(flowid, rw_flowid);
    if (i == SampleIPRewriter::rw_drop)
      ++failures;
    return i;
  }
  case i_drop:
  default:
    return SampleIPRewriter::rw_drop;
  }
}

void SampleIPRewriterInput::unparse(StringAccum &sa) const
{
  sa << "{";
  if (kind == i_drop)
    sa << "kind: drop, ";
  else if (kind == i_pattern)
  {
    sa << "kind: pattern, ";
    sa << "foutput: " << foutput << ", ";
    sa << "routput: " << routput << ", ";
    pattern->unparse(sa);
  }
  sa << "count: " << count << ", ";
  sa << "failures: " << failures;
  sa << "}";
}

void SampleIPRewriterPattern::unparse(StringAccum &sa) const
{
  sa << "[";
  sa << "(" << _saddr << " " << ntohs(_sport) << " " << _daddr << " "
     << ntohs(_dport) << "),";
  sa << " _variation_top: " << _variation_top << ",";
  sa << " _next_variation: " << _next_variation << ",";
  sa << " _sequential: " << _sequential << ",";
  sa << " _same_first: " << _same_first << ",";
  sa << " _refcount: " << _refcount << ",";
  sa << "]";
}

SampleIPRewriterPattern::SampleIPRewriterPattern(const IPAddress &saddr, int sport,
                                                 const IPAddress &daddr, int dport,
                                                 bool sequential, bool same_first,
                                                 uint32_t variation, int vari_target)
    : _saddr(saddr), _sport(sport), _daddr(daddr), _dport(dport),
      _variation_top(variation), _next_variation(0), _sequential(sequential),
      _same_first(same_first), _refcount(0), _vari_target(vari_target) {}

enum
{
  PE_SYNTAX,
  PE_NOPATTERN,
  PE_SADDR,
  PE_SPORT,
  PE_DADDR,
  PE_DPORT
};
static const char *const pe_messages[] = {
    "syntax error", "no such pattern", "bad source address",
    "bad source port", "bad destination address", "bad destination port"};

static inline bool pattern_error(int what, ErrorHandler *errh)
{
  return errh->error(pe_messages[what]), false;
}

static bool parse_ports(const Vector<String> &words, SampleIPRewriterInput *input,
                        Element *, ErrorHandler *errh)
{
  if (!(words.size() == 2 && IntArg().parse(words[0], input->foutput)))
    return errh->error("bad forward port"), false;
  if (IntArg().parse(words[1], input->routput))
    return true;
  else
    return errh->error("bad reply port"), false;
}

static bool port_variation(const String &str, int32_t &port, int32_t &variation)
{
  const char *end = str.end();
  const char *dash = find(str.begin(), end, '-');
  int32_t port2 = 0;

  if (IntArg().parse(str.substring(str.begin(), dash), port) &&
      IntArg().parse(str.substring(dash + 1, end), port2) && port >= 0 &&
      port2 >= port && port2 < 65536)
  {
    variation = port2 - port;
    return true;
  }
  else
    return false;
}

static bool addr_variation(const String &str, IPAddress &baseAddr, int32_t &variation)
{
  const char *end = str.end();
  const char *dash = find(str.begin(), end, '-');
  IPAddress addr2;
  if (IPAddressArg().parse(str.substring(str.begin(), dash), baseAddr) &&
      IPAddressArg().parse(str.substring(dash + 1, end), addr2) && ntohl(addr2.addr()) >= ntohl(baseAddr.addr()))
  {
    variation = ntohl(addr2.addr()) - ntohl(baseAddr.addr());
    return true;
  }
  else
    return false;
}

bool SampleIPRewriterPattern::parse(const String &str, SampleIPRewriterInput *input,
                                    Element *context, ErrorHandler *errh)
{
  Vector<String> words, port_words;
  cp_spacevec(str, words);

  port_words.push_back(words[words.size() - 2]);
  port_words.push_back(words[words.size() - 1]);
  words.resize(words.size() - 2);

  if (!parse_ports(port_words, input, context, errh))
    return false;

  if (words.size() != 4)
    return pattern_error(PE_SYNTAX, errh), false;

  IPAddress saddr, daddr;
  int32_t sport = 0, dport = 0, variation = 0;
  bool sequential = true, same_first = false;
  int vari_target = 0;

  // source address
  int i = 0;
  if (!(words[i].equals("-", 1) ||
        IPAddressArg().parse(words[i], saddr, context)))
    return pattern_error(PE_SADDR, errh);

  // source port
  ++i;
  if (!(words[i].equals("-", 1) || (IntArg().parse(words[i], sport) && sport > 0 && sport < 65536)))
  {
    if (port_variation(words[i], sport, variation))
      vari_target = TARGET_SPORT;
    else
      return pattern_error(PE_SPORT, errh);
  }

  // destination address
  ++i;
  if (!(words[i].equals("-", 1) || IPAddressArg().parse(words[i], saddr, context)))
  {
    if (addr_variation(words[i], daddr, variation))
      vari_target = TARGET_DADDR;
    else
      return pattern_error(PE_DADDR, errh);
  }

  // destination port
  ++i;
  if (!(words[i].equals("-", 1) ||
        (IntArg().parse(words[i], dport) && dport > 0 && dport < 65536)))
    return pattern_error(PE_DPORT, errh);

  input->pattern =
      new SampleIPRewriterPattern(saddr, htons(sport), daddr, htons(dport),
                                  sequential, same_first, variation, vari_target);
  return true;
}

int SampleIPRewriterPattern::rewrite_flowid(const IPFlowID &flowid, IPFlowID &rw_flowid)
{
  rw_flowid = flowid;
  if (_saddr)
    rw_flowid.set_saddr(_saddr);
  if (_sport)
    rw_flowid.set_sport(_sport);
  if (_daddr)
    rw_flowid.set_daddr(_daddr);
  if (_dport)
    rw_flowid.set_dport(_dport);

  uint32_t val, base;

  // TOFIX: for test. correct behavior is to drop.
  if (_next_variation > _variation_top)
    _next_variation = 0;
  // return SampleIPRewriter::rw_drop;
  val = _next_variation;

  if (_vari_target == TARGET_SPORT)
  {
    base = ntohs(_sport);
    rw_flowid.set_sport(htons(base + val));
  }
  else if (_vari_target == TARGET_DADDR)
  {
    base = ntohl(_daddr.addr());
    rw_flowid.set_daddr(IPAddress(htonl(base + val)));
  }

  _next_variation = val + 1;
  return SampleIPRewriter::rw_addmap;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(SampleIPRewriter)
