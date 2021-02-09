#ifndef CLICK_SAMPLEIPFILTER_HH
#define CLICK_SAMPLEIPFILTER_HH
#include <click/batchelement.hh>
#include <click/ipflowid.hh>
#include <click/error.hh>
#include <click/vector.hh>
#include <click/packet.hh>
#include <unordered_map>

CLICK_DECLS

class SampleIPFilterEntry
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
        size_t operator()(const SampleIPFilterEntry::Key &a) const
        {
            return (a.src.hashcode() + a.dst.hashcode()) ^ (a.proto << 24);
        }
    };
    SampleIPFilterEntry(Key &key, bool allowed);
    bool allowed() const;
    const Key key() const;
    // void unparse(StringAccum &sa);

private:
    Key _key;
    bool _allowed;
};

class SampleIPFilter : public BatchElement
{
public:
    SampleIPFilter();
    ~SampleIPFilter();

    const char *class_name() const { return "SampleIPFilter"; }
    const char *port_count() const override { return "1/-"; }
    const char *processing() const override { return PUSH; }

    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;

    // void push_batch(int port, PacketBatch *);
    void push(int port, Packet *);
    void push_batch(int port, PacketBatch *batch) override;

protected:
    SampleIPFilterEntry *parse(Vector<String> &, ErrorHandler *);

    int process(int port, Packet *);

private:
    std::unordered_map<SampleIPFilterEntry::Key, SampleIPFilterEntry *, SampleIPFilterEntry::Hash_Key> _map;
    bool _debug;
    // Vector<P4SFCState::TableEntry *> rules;
};

inline bool SampleIPFilterEntry::allowed() const
{
    return _allowed;
}
inline const SampleIPFilterEntry::Key SampleIPFilterEntry::key() const
{
    return _key;
}
inline bool operator==(const struct SampleIPFilterEntry::Key &a, const struct SampleIPFilterEntry::Key &b)
{
    return ((a.src == b.src) && (a.dst == b.dst) && (a.proto == b.proto));
}

CLICK_ENDDECLS
#endif
