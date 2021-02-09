#ifndef P4IPREWRITER_HH
#define P4IPREWRITER_HH
#include <click/bitvector.hh>
#include <click/batchelement.hh>
#include <click/hashtable.hh>
#include <click/ipflowid.hh>
#include <click/timer.hh>
#include <unordered_map>

#include "../../statelib/p4sfcstate.hh"

CLICK_DECLS

/*
 * configure(id, [input_specs+])
 */

class P4IPRewriterInput;
class P4IPRewriterEntry;
class P4IPRewriter;

class P4IPRewriterEntry : public P4SFCState::TableEntryImpl
{
public:
  P4IPRewriterEntry(const IPFlowID &in, const IPFlowID &out);
  void apply(WritablePacket *p);
  String unparse();

private:
  IPFlowID flowid;
  IPFlowID rw_flowid;
};

class P4IPRewriterPattern
{
public:
  P4IPRewriterPattern(const IPAddress &saddr, int sport, const IPAddress &daddr,
                          int dport, bool sequential, bool same_first,
                          uint32_t variation, int vari_target);
  static bool parse_with_ports(const String &str, P4IPRewriterInput *input,
                               Element *context, ErrorHandler *errh);
  static bool parse(const String &str, P4IPRewriterInput *input,
                    Element *context, ErrorHandler *errh);

  void use() { _refcount++; }
  void unuse()
  {
    if (--_refcount <= 0)
      delete this;
  }

  operator bool() const { return _saddr || _sport || _daddr || _dport; }
  IPAddress daddr() const { return _daddr; }

  int rewrite_flowid(const IPFlowID &flowid, IPFlowID &rewritten_flowid);

  void unparse(StringAccum &sa) const;

  enum
  {
    TARGET_NULL = 0,
    TARGET_SADDR = 1,
    TARGET_DADDR,
    TARGET_SPORT,
    TARGET_DPORT
  };

private:
  IPAddress _saddr;
  int _sport; // net byte order
  IPAddress _daddr;
  int _dport; // net byte order

  int _vari_target;
  uint32_t _variation_top;
  uint32_t _next_variation;

  bool _sequential;
  bool _same_first;

  int _refcount;
};

class P4IPRewriterInput
{
public:
  enum
  {
    i_drop,
    i_pattern
  };
  P4IPRewriter *owner;
  int kind;
  uint32_t count;
  uint32_t failures;

  // pattern related vars
  int foutput;
  int routput;
  P4IPRewriterPattern *pattern;

  P4IPRewriterInput();

  int rewrite_flowid(const IPFlowID &flowid, IPFlowID &rewritten_flowid);

  void unparse(StringAccum &sa) const;
};

class P4IPRewriter : public BatchElement
{
public:
  enum
  {
    rw_drop = -1,
    rw_addmap = -2
  }; // rw result

  P4IPRewriter();
  ~P4IPRewriter();

  const char *class_name() const { return "P4IPRewriter"; }
  const char *port_count() const { return "1-/1-"; }
  const char *processing() const { return PUSH; }
  const char *flow_code() const { return "x/y"; }

  int configure(Vector<String> &conf, ErrorHandler *errh);

  P4IPRewriterEntry *add_flow(const IPFlowID &flowid, const IPFlowID &rewritten_flowid);

  Packet *process(int, Packet *);

  void push(int, Packet *) override;
  void push_batch(int port, PacketBatch *batch) override;

protected:
  struct hash_IPFlowID
  {
    size_t operator()(const IPFlowID &a) const
    {
      return a.hashcode();
    }
  };
  P4SFCState::Table<IPFlowID, struct hash_IPFlowID> _map;
  Vector<P4IPRewriterInput> _input_specs;
  bool _debug;

private:
  int parse_input_spec(const String &line, P4IPRewriterInput &is,
                       int input_number, ErrorHandler *errh);
};

CLICK_ENDDECLS
#endif
