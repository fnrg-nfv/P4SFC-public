#ifndef PRINTTIMEELEMENT_HH
#define PRINTTIME_HH
#include <click/batchelement.hh>
CLICK_DECLS

class PrintTime : public BatchElement
{
public:
    PrintTime();
    ~PrintTime();

    const char *class_name() const { return "PrintTime"; }
    const char *port_count() const { return "1/1"; }

    Packet *simple_action(Packet *);
    PacketBatch *simple_action_batch(PacketBatch *);
    int configure(Vector<String> &, ErrorHandler *);
    void add_handlers() CLICK_COLD;

    bool _debug;
    uint32_t _cnt;
    Timestamp _latency;
    uint32_t _header_offset;

private:
    static String read_handler(Element *, void *) CLICK_COLD;
};

CLICK_ENDDECLS
#endif
