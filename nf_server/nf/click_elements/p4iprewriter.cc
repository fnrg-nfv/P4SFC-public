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

#include "p4iprewriter.hh"
#include "p4header.hh"

CLICK_DECLS

P4IPRewriter::P4IPRewriter()
{
  in_batch_mode = BATCH_MODE_IFPOSSIBLE;
}

P4IPRewriter::~P4IPRewriter()
{
  P4SFCState::shutdownServer();
}

int P4IPRewriter::configure(Vector<String> &conf, ErrorHandler *errh)
{
  // std::cout << "Specs Len: " << conf.size() << std::endl;
  int click_instance_id = 1;
  _debug = false;
  uint16_t port = 0;
  if (Args(conf, this, errh)
          .read_mp("CLICKINSTANCEID", click_instance_id)
          .read_mp("DEBUG", _debug)
          .read_mp("PORT", port)
          .consume() < 0)
    return -1;

  char addr[20];
  sprintf(addr, "0.0.0.0:%d", port);
  P4SFCState::startServer(click_instance_id, std::string(addr));

  if (_debug)
    std::cout << _debug << std::endl;

  for (int i = 3; i < conf.size(); ++i)
  {
    P4IPRewriterInput is;
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

int P4IPRewriter::parse_input_spec(const String &line, P4IPRewriterInput &is,
                                   int input_number, ErrorHandler *errh)
{
  PrefixErrorHandler cerrh(errh, "input spec " + String(input_number) + ": ");
  String word, rest;
  if (!cp_word(line, &word, &rest))
    return cerrh.error("empty argument");
  cp_eat_space(rest);

  is.kind = P4IPRewriterInput::i_drop;
  is.owner = this;

  if (word == "drop")
  {
    if (rest)
      return cerrh.error("syntax error, expected %<%s%>", word.c_str());
  }
  else if (word == "pattern")
  {
    if (!P4IPRewriterPattern::parse(rest, &is, this, &cerrh))
      return -1;
    if ((unsigned)is.foutput >= (unsigned)noutputs() ||
        (unsigned)is.routput >= (unsigned)is.owner->noutputs())
      return cerrh.error("output port out of range");
    is.pattern->use();
    is.kind = P4IPRewriterInput::i_pattern;
  }
  else
  {
    return cerrh.error("unknown specification");
  }

  return 0;
}

inline std::string encode(uint16_t i)
{
  char *c = (char *)&i;
  return std::string(c, sizeof(uint16_t));
}

P4IPRewriterEntry::P4IPRewriterEntry(const IPFlowID &in, const IPFlowID &out)
    : P4SFCState::TableEntryImpl(), flowid(in), rw_flowid(out)
{
  set_table_name(P4_IPRW_TABLE_NAME);
  {
    auto m = add_match();
    m->set_field_name(P4H_IP_SADDR);
    auto ex = m->mutable_exact();
    ex->set_value(in.saddr().data(), 4);
  }
  {
    auto m = add_match();
    m->set_field_name(P4H_IP_DADDR);
    auto ex = m->mutable_exact();
    ex->set_value(in.daddr().data(), 4);
  }
  {
    auto m = add_match();
    m->set_field_name(P4H_IP_SPORT);
    auto ex = m->mutable_exact();
    ex->set_value(encode(in.sport()));
  }
  {
    auto m = add_match();
    m->set_field_name(P4H_IP_DPORT);
    auto ex = m->mutable_exact();
    ex->set_value(encode(in.dport()));
  }

  auto a = mutable_action();
  a->set_action(P4_IPRW_ACTION_NAME);
  {
    auto p = a->add_params();
    p->set_param(P4_IPRW_PARAM_SA);
    p->set_value(out.saddr().data(), 4);
  }
  {
    auto p = a->add_params();
    p->set_param(P4_IPRW_PARAM_DA);
    p->set_value(out.daddr().data(), 4);
  }
  {
    auto p = a->add_params();
    p->set_param(P4_IPRW_PARAM_SP);
    p->set_value(encode(out.sport()));
  }
  {
    auto p = a->add_params();
    p->set_param(P4_IPRW_PARAM_DP);
    p->set_value(encode(out.dport()));
  }
}

String P4IPRewriterEntry::unparse()
{
  StringAccum sa;
  sa << flowid.unparse() << "->" << rw_flowid.unparse() << "\n";
  sa << P4SFCState::TableEntryImpl::unparse().c_str();
  return sa.take_string();
}

void P4IPRewriterEntry::apply(WritablePacket *p)
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

void P4IPRewriter::push_batch(int port, PacketBatch *batch)
{
  auto fnt = [this, port](Packet *p) { return process(port, p); };
  EXECUTE_FOR_EACH_PACKET_DROPPABLE(fnt, batch, [](Packet *p) {})
  if (batch)
    output(0).push_batch(batch);
}

void P4IPRewriter::push(int port, Packet *p_in)
{
  if (Packet *p = process(port, p_in))
    checked_output_push(0, p);
}

Packet *P4IPRewriter::process(int port, Packet *p_in)
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
  P4IPRewriterEntry *entry = (P4IPRewriterEntry *)_map.lookup(flowid);
  if (entry)
  {
    // print the match entry
    if (_debug)
      std::cout << "[match]\t";
  }
  else
  {
    // 2. if not in map, inputspec.output
    P4IPRewriterInput &is = _input_specs.at(port);
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

P4IPRewriterEntry *P4IPRewriter::add_flow(const IPFlowID &flowid, const IPFlowID &rw_flowid)
{
  P4IPRewriterEntry *entry = new P4IPRewriterEntry(flowid, rw_flowid);
  P4IPRewriterEntry *entry_r = new P4IPRewriterEntry(rw_flowid.reverse(), flowid.reverse());

  _map.insert(flowid, entry);
  _map.insert(rw_flowid.reverse(), entry_r);

  return entry;
}

P4IPRewriterInput::P4IPRewriterInput()
    : kind(i_drop), count(0), failures(0), foutput(-1), routput(-1)
{
  pattern = 0;
}

int P4IPRewriterInput::rewrite_flowid(const IPFlowID &flowid, IPFlowID &rw_flowid)
{
  int i;
  switch (kind)
  {
  case i_pattern:
  {
    i = pattern->rewrite_flowid(flowid, rw_flowid);
    if (i == P4IPRewriter::rw_drop)
      ++failures;
    return i;
  }
  case i_drop:
  default:
    return P4IPRewriter::rw_drop;
  }
}

void P4IPRewriterInput::unparse(StringAccum &sa) const
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

void P4IPRewriterPattern::unparse(StringAccum &sa) const
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

P4IPRewriterPattern::P4IPRewriterPattern(const IPAddress &saddr, int sport,
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

static bool parse_ports(const Vector<String> &words, P4IPRewriterInput *input,
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

bool P4IPRewriterPattern::parse(const String &str, P4IPRewriterInput *input,
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
      new P4IPRewriterPattern(saddr, htons(sport), daddr, htons(dport),
                                  sequential, same_first, variation, vari_target);
  return true;
}

int P4IPRewriterPattern::rewrite_flowid(const IPFlowID &flowid, IPFlowID &rw_flowid)
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
  // return P4IPRewriter::rw_drop;
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
  return P4IPRewriter::rw_addmap;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4IPRewriter)
ELEMENT_LIBS(-L../../statelib -lstate -L/usr/local/lib `pkg-config --libs grpc++ grpc` -lgrpc++_reflection -lprotobuf -lpthread -ldl)
