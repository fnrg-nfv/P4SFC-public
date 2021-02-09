#ifndef CLICK_CUSTOMENCAP_HH
#define CLICK_CUSTOMENCAP_HH
#include <click/batchelement.hh>
#include <click/bitvector.hh>
#include <queue>
CLICK_DECLS

class CustomEncap : public BatchElement
{
public:
  CustomEncap() CLICK_COLD;

  const char *class_name() const { return "CustomEncap"; }
  const char *port_count() const { return PORTS_1_1; }
  // const char *processing() const { return "a/a"; }
  // const char *flow_code() const { return "x/y"; }

  int configure(Vector<String> &conf, ErrorHandler *errh) CLICK_COLD;
#if HAVE_BATCH
  PacketBatch *simple_action_batch(PacketBatch *);
#endif
  Packet *simple_action(Packet *);

protected:
  char *_header;
  int _len;

private:
  void parse_pattern(String &s);
};

CLICK_ENDDECLS
#endif
