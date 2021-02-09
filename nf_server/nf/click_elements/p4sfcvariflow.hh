#ifndef CLICK_P4SFCVARIFLOW_HH
#define CLICK_P4SFCVARIFLOW_HH
#include <click/batchelement.hh>
#include <click/tokenbucket.hh>
#include <click/task.hh>
#include <clicknet/ether.h>

CLICK_DECLS
class HandlerCall;

class P4SFCVariFlow : public BatchElement
{
public:
    P4SFCVariFlow() CLICK_COLD;

    const char *class_name() const override { return "P4SFCVariFlow"; }
    const char *port_count() const override { return PORTS_0_1; }

    int configure(Vector<String> &conf, ErrorHandler *errh) CLICK_COLD;
    int initialize(ErrorHandler *errh) CLICK_COLD;
    bool can_live_reconfigure() const { return true; }
    void cleanup(CleanupStage) CLICK_COLD;

    bool run_task(Task *task);

protected:
    static const unsigned NO_LIMIT = 0xFFFFFFFFU;
    static const unsigned DEF_BATCH_SIZE = 32;

#if HAVE_INT64_TYPES
    typedef uint64_t ucounter_t;
    typedef int64_t counter_t;
#else
    typedef uint32_t ucounter_t;
    typedef int32_t counter_t;
#endif

    TokenBucket _tb;
    ucounter_t _count;
    ucounter_t _limit;
    unsigned _batch_size;
    unsigned _len;
    unsigned _header_len;
    bool _active;
    bool _stop;
    Task _task;
    Timer _timer;
    bool _rate_limit;
    long _rate;

    unsigned _major_flowsize;
    unsigned _major_data;
    unsigned _flowsize;

    unsigned _short_prop;
    unsigned _short_time;

    String _sfch;
    click_ether _ethh;
    struct in_addr _sipaddr;
    struct in_addr _dipaddr;
    unsigned _range;
    unsigned _seed;
    bool _debug;
    Timestamp _now;

    struct flow_t
    {
        // Packet *packet;
        unsigned char *data;
        unsigned flow_count;
        Timestamp duration;
        Timestamp end_time;
    };
    flow_t *_flows;

    void new_header(unsigned char *);
    Timestamp random_duration(bool);
    void setup_flows(ErrorHandler *);

    void print_flow_counts();

    Packet *next_packet();
    void update_flow(flow_t &);
};

CLICK_ENDDECLS
#endif
