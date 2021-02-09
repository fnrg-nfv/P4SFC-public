#ifndef CLICK_LATENCY_HH
#define CLICK_LATENCY_HH
#include <click/bitvector.hh>
#include <click/batchelement.hh>
#include <click/timestamp.hh>
#include <queue>
CLICK_DECLS

class Latency: public Element
{
public:
  Latency() {}
  ~Latency() {}

  const char *class_name() const { return "Latency"; }
  const char *port_count() const { return "2/2"; }

  void push(int, Packet *) override;

private:
  Timestamp t;
};

CLICK_ENDDECLS
#endif
