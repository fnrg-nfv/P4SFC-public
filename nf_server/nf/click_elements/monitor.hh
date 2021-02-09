#ifndef SAMPLEMONITOR_HH
#define SAMPLEMONITOR_HH
#include <click/element.hh>
CLICK_DECLS

class SampleMonitor : public Element {
public:
  SampleMonitor();
  ~SampleMonitor();

  int initialize(ErrorHandler *errh);

  const char *class_name() const { return "SampleMonitor"; }
  const char *port_count() const { return "1/1"; }
  const char *processing() const { return "a/a"; }
  const char *flow_code() const { return "x/y"; }

  Packet *simple_action(Packet *);


protected:
  int _cur_pkts;
  int _total_pkts;

private:
  long int _last_time;
  long int _get_cur_time();
};

CLICK_ENDDECLS
#endif
