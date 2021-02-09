#include <bitset>
#include <click/config.h>

#include "latency.hh"
CLICK_DECLS

void Latency::push(int input, Packet *p)
{
  if (input == 0)
    t = Timestamp::now();
  else if (input == 1)
  {
    Timestamp interval = Timestamp::now() - t;
    click_chatter("Interval time: %s\n", interval.unparse_interval().c_str());
  }
  output(input).push(p);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(Latency)
ELEMENT_MT_SAFE(Latency)
