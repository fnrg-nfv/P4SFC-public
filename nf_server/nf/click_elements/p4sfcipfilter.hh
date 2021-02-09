#ifndef CLICK_P4SFCIPFILTER_HH
#define CLICK_P4SFCIPFILTER_HH
#include <click/batchelement.hh>
#include <click/ipflowid.hh>
#include <click/error.hh>
#include <click/vector.hh>
#include <click/packet.hh>

#include "../../statelib/p4sfcstate.hh"

CLICK_DECLS

class P4IPFilterEntry : public P4SFCState::TableEntryImpl
{
public:
    struct Key
    {
    public:
        Key(IPAddress src, IPAddress dst, uint8_t proto) : src(src), dst(dst), proto(proto) {}

        IPAddress src, dst;
        uint8_t proto;
    };
    struct Hash_Key
    {
        size_t operator()(const P4IPFilterEntry::Key &a) const
        {
            return (a.src.hashcode() + a.dst.hashcode()) ^ (a.proto << 24);
        }
    };
    P4IPFilterEntry(Key &key, bool allowed);
    bool allowed() const;
    const Key key() const;
    // void unparse(StringAccum &sa);

    bool _allowed;
private:
    Key _key;
};

class P4SFCIPFilter : public BatchElement
{
public:
    P4SFCIPFilter();
    ~P4SFCIPFilter();

    const char *class_name() const { return "P4SFCIPFilter"; }
    const char *port_count() const override { return "1/-"; }
    const char *processing() const override { return PUSH; }

    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;

    // void push_batch(int port, PacketBatch *);
    void push(int port, Packet *);
    void push_batch(int port, PacketBatch *batch) override;

protected:
    P4IPFilterEntry *parse(Vector<String> &, ErrorHandler *);

    int process(int port, Packet *);
    int apply(P4SFCState::TableEntry *);
    bool match(const IPFlow5ID &, const P4SFCState::TableEntry *);

    template <class T>
    T s2i(const std::string &);

private:
    P4SFCState::Table<P4IPFilterEntry::Key, P4IPFilterEntry::Hash_Key> _map;
    bool _debug;
    // Vector<P4SFCState::TableEntry *> rules;
};

inline bool P4IPFilterEntry::allowed() const
{
    return _allowed;
}
inline const P4IPFilterEntry::Key P4IPFilterEntry::key() const
{
    return _key;
}
inline bool operator==(const struct P4IPFilterEntry::Key &a, const struct P4IPFilterEntry::Key &b)
{
    return ((a.src == b.src) && (a.dst == b.dst) && (a.proto == b.proto));
}

CLICK_ENDDECLS
#endif
