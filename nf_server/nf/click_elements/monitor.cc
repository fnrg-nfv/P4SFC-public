// ALWAYS INCLUDE <click/config.h> FIRST
#include <click/config.h>

#include "monitor.hh"
#include <click/error.hh>
#include <iostream>
#include <sys/time.h>
CLICK_DECLS

SampleMonitor::SampleMonitor() {}

SampleMonitor::~SampleMonitor() {}

int SampleMonitor::initialize(ErrorHandler *errh) {
  errh->message("Successfully linked with SampleMonitor!");
  _total_pkts = 0;
  _cur_pkts = 0;
  _last_time = _get_cur_time();
  return 0;
}

Packet *SampleMonitor::simple_action(Packet *p) {
  _total_pkts++;
  _cur_pkts++;
  long int cur_time = _get_cur_time();
  if (cur_time - _last_time >= 500000) { // interval: .5s
    std::cout << "MONITOR == Current time(us):" << cur_time
              << "\tTotal:" << _total_pkts << "\tInterval:" << _cur_pkts
              << std::endl;
    _cur_pkts = 0;
    _last_time = cur_time;
  }
  return p;
}

long int SampleMonitor::_get_cur_time() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  long int us = tp.tv_sec * 1000000 + tp.tv_usec;
  return us;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(SampleMonitor)
