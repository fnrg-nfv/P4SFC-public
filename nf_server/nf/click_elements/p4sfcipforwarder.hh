#ifndef CLICK_P4SFCIPFORWARDER_HH
#define CLICK_P4SFCIPFORWARDER_HH
#include <click/batchelement.hh>
#include <click/ipflowid.hh>
#include <click/error.hh>
#include <click/vector.hh>
#include <click/packet.hh>

#include "../../statelib/p4sfcstate.hh"

CLICK_DECLS

class P4IPForwarderEntry : public P4SFCState::TableEntryImpl
{
public:
    P4IPForwarderEntry(IPAddress dst, int port);

    IPAddress dst() const;
    int port() const;

private:
    IPAddress _dst;
    int _port;
};

class P4SFCIPForwarder : public BatchElement
{
public:
    P4SFCIPForwarder();
    ~P4SFCIPForwarder();

    const char *class_name() const { return "P4SFCIPForwarder"; }
    const char *port_count() const override { return "1/-"; }
    const char *processing() const override { return PUSH; }

    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;

    // void push_batch(int port, PacketBatch *);
    void push(int port, Packet *);
    void push_batch(int port, PacketBatch *batch) override;

protected:

    P4IPForwarderEntry *parse(Vector<String> &, ErrorHandler *);

    int process(int port, Packet *);

private:
    struct Hash_IPAddress
    {
        size_t operator()(const IPAddress &a) const
        {
            return a.hashcode();
        }
    };
    P4SFCState::Table<IPAddress, struct Hash_IPAddress> _map;
    bool _debug;
};

CLICK_ENDDECLS
#endif
