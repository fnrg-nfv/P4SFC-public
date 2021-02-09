#ifndef CLICK_P4SFCSIMUFLOW_HH
#define CLICK_P4SFCSIMUFLOW_HH
#include <click/batchelement.hh>
#include <click/tokenbucket.hh>
#include <click/task.hh>
#include <clicknet/ether.h>

CLICK_DECLS
class HandlerCall;

class P4SFCSimuFlow : public BatchElement
{
public:
    P4SFCSimuFlow() CLICK_COLD;

    const char *class_name() const override { return "P4SFCSimuFlow"; }
    const char *port_count() const override { return PORTS_0_1; }

    int configure(Vector<String> &conf, ErrorHandler *errh) CLICK_COLD;
    int initialize(ErrorHandler *errh) CLICK_COLD;
    bool can_live_reconfigure() const { return true; }
    void cleanup(CleanupStage) CLICK_COLD;

    bool run_task(Task *task);

    //     Packet *pull(int);
    // #if HAVE_BATCH
    //     PacketBatch *pull_batch(int, unsigned);
    // #endif

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

    unsigned _major_flowsize;
    unsigned _major_data;
    unsigned _flowsize;
    uint16_t _sport;
    uint16_t _dport;

    String _sfch;
    click_ether _ethh;
    struct in_addr _sipaddr;
    struct in_addr _dipaddr;
    unsigned _range;
    unsigned _seed;
    bool _debug;

    struct flow_t
    {
        // Packet *packet;
        unsigned char* data;
        unsigned flow_count;
    };
    flow_t *_flows;
    Timestamp _now;

    inline Packet *next_packet();
    void setup_flows(ErrorHandler *);
    void print_flow_counts();
};

CLICK_ENDDECLS
#endif
