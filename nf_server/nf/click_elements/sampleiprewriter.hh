#ifndef SAMPLEIPREWRITER_HH
#define SAMPLEIPREWRITER_HH
#include <click/bitvector.hh>
#include <click/batchelement.hh>
#include <click/hashtable.hh>
#include <click/ipflowid.hh>
#include <click/timer.hh>
#include <unordered_map>

CLICK_DECLS

/*
 * configure(id, [input_specs+])
 */

class SampleIPRewriterInput;
class SampleIPRewriterEntry;
class SampleIPRewriter;

class SampleIPRewriterEntry
{
public:
  SampleIPRewriterEntry(const IPFlowID &in, const IPFlowID &out);
  void apply(WritablePacket *p);
  String unparse();

private:
  IPFlowID flowid;
  IPFlowID rw_flowid;
};

class SampleIPRewriterPattern
{
public:
  SampleIPRewriterPattern(const IPAddress &saddr, int sport, const IPAddress &daddr,
                          int dport, bool sequential, bool same_first,
                          uint32_t variation, int vari_target);
  static bool parse_with_ports(const String &str, SampleIPRewriterInput *input,
                               Element *context, ErrorHandler *errh);
  static bool parse(const String &str, SampleIPRewriterInput *input,
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

class SampleIPRewriterInput
{
public:
  enum
  {
    i_drop,
    i_pattern
  };
  SampleIPRewriter *owner;
  int kind;
  uint32_t count;
  uint32_t failures;

  // pattern related vars
  int foutput;
  int routput;
  SampleIPRewriterPattern *pattern;

  SampleIPRewriterInput();

  int rewrite_flowid(const IPFlowID &flowid, IPFlowID &rewritten_flowid);

  void unparse(StringAccum &sa) const;
};

class SampleIPRewriter : public BatchElement
{
public:
  enum
  {
    rw_drop = -1,
    rw_addmap = -2
  }; // rw result

  SampleIPRewriter();
  ~SampleIPRewriter();

  const char *class_name() const { return "SampleIPRewriter"; }
  const char *port_count() const { return "1-/1-"; }
  const char *processing() const { return PUSH; }
  const char *flow_code() const { return "x/y"; }

  int configure(Vector<String> &conf, ErrorHandler *errh);

  SampleIPRewriterEntry *add_flow(const IPFlowID &flowid, const IPFlowID &rewritten_flowid);

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
  std::unordered_map<IPFlowID, SampleIPRewriterEntry *, struct hash_IPFlowID> _map;
  Vector<SampleIPRewriterInput> _input_specs;
  bool _debug;

private:
  int parse_input_spec(const String &line, SampleIPRewriterInput &is,
                       int input_number, ErrorHandler *errh);
};

CLICK_ENDDECLS
#endif
