#ifndef CLICK_SAMPLEIPFORWARDER_HH 
#define CLICK_SAMPLEIPFORWARDER_HH 
#include <click/batchelement.hh>
#include <click/ipflowid.hh>
#include <click/error.hh>
#include <click/vector.hh>
#include <click/packet.hh>
#include <unordered_map>

CLICK_DECLS

class SampleIPForwarderEntry
{
public:
    SampleIPForwarderEntry(IPAddress dst, int port);

    IPAddress dst() const;
    int port() const;

private:
    IPAddress _dst;
    int _port;
};

class SampleIPForwarder : public BatchElement
{
public:
    SampleIPForwarder();
    ~SampleIPForwarder();

    const char *class_name() const { return "SampleIPForwarder"; }
    const char *port_count() const override { return "1/-"; }
    const char *processing() const override { return PUSH; }

    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;

    // void push_batch(int port, PacketBatch *);
    void push(int port, Packet *);
    void push_batch(int port, PacketBatch *batch) override;

protected:
    SampleIPForwarderEntry *parse(Vector<String> &, ErrorHandler *);

    int process(int port, Packet *);

private:
    struct Hash_IPAddress
    {
        size_t operator()(const IPAddress &a) const
        {
            return a.hashcode();
        }
    };
    std::unordered_map<IPAddress, SampleIPForwarderEntry *, struct Hash_IPAddress> _map;
    bool _debug;
};

CLICK_ENDDECLS
#endif
