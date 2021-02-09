
// ALWAYS INCLUDE <click/config.h> FIRST
#include <click/config.h>

#include <click/error.hh>
#include <click/args.hh>
#include <click/straccum.hh>
#include <clicknet/ip.h>
#include <clicknet/tcp.h>
#include <clicknet/icmp.h>
#include <click/integers.hh>
#include <click/etheraddress.hh>
#include <click/nameinfo.hh>
#include <click/error.hh>
#include <click/glue.hh>

#include "sampleipforwarder.hh"

#include <iostream>
CLICK_DECLS

static void
separate_text(const String &text, Vector<String> &words)
{
    const char *s = text.data();
    int len = text.length();
    int pos = 0;
    while (pos < len)
    {
        while (pos < len && isspace((unsigned char)s[pos]))
            pos++;
        switch (s[pos])
        {
        case '&':
        case '|':
            if (pos < len - 1 && s[pos + 1] == s[pos])
                goto two_char;
            goto one_char;

        case '<':
        case '>':
        case '!':
        case '=':
            if (pos < len - 1 && s[pos + 1] == '=')
                goto two_char;
            goto one_char;

        case '(':
        case ')':
        case '[':
        case ']':
        case ',':
        case ';':
        case '?':
        one_char:
            words.push_back(text.substring(pos, 1));
            pos++;
            break;

        two_char:
            words.push_back(text.substring(pos, 2));
            pos += 2;
            break;

        default:
        {
            int first = pos;
            while (pos < len && (isalnum((unsigned char)s[pos]) || s[pos] == '-' || s[pos] == '.' || s[pos] == '/' || s[pos] == '@' || s[pos] == '_' || s[pos] == ':'))
                pos++;
            if (pos == first)
                pos++;
            words.push_back(text.substring(first, pos - first));
            break;
        }
        }
    }
}


SampleIPForwarderEntry::SampleIPForwarderEntry(IPAddress dst, int port) : _dst(dst), _port(port) {}

inline IPAddress SampleIPForwarderEntry::dst() const
{
    return _dst;
}
inline int SampleIPForwarderEntry::port() const
{
    return _port;
}

SampleIPForwarder::SampleIPForwarder()
{
    in_batch_mode = BATCH_MODE_IFPOSSIBLE;
}

SampleIPForwarder::~SampleIPForwarder()
{
}

int SampleIPForwarder::configure(Vector<String> &conf, ErrorHandler *errh)
{
    _debug = false;
    if (Args(conf, this, errh)
            .read_mp("DEBUG", _debug)
            .consume() < 0)
        return -1;

    for (int argno = 1; argno < conf.size(); argno++)
    {
        // parse every rules into statelib
        Vector<String> words;
        separate_text(cp_unquote(conf[argno]), words);

        SampleIPForwarderEntry *e = parse(words, errh);
        // TODO: priority ascending or descending
        _map[e->dst()] = e;
    }
}

SampleIPForwarderEntry *SampleIPForwarder::parse(Vector<String> &words, ErrorHandler *errh)
{
    int size = words.size();
    if (size != 2)
    {
        errh->message("firewall rule must be 2-tuple, but is a %d-tuple", size);
        return NULL;
    }
    int port = 0;
    IPAddress dst;

    if (!IntArg().parse(words[0], port))
        errh->message("format error %s", words[0]);
    if (!IPAddressArg().parse(words[1], dst, this))
        errh->message("dst IPAddress: parse error");

    return new SampleIPForwarderEntry(dst, port);
}

int SampleIPForwarder::process(int port, Packet *p)
{
    IPFlowID flowid(p);
    int out = 0;
    IPAddress dst = flowid.daddr();
    auto it = _map.find(dst);
    // simulate the action
    if (it != _map.end())
        int _fuck_ = it->second->port();

    if (_debug)
        click_chatter("Forwarder the packet to port %x.", out);
    return out;
}

void SampleIPForwarder::push(int port, Packet *p)
{
    checked_output_push(process(port, p), p);
}

void SampleIPForwarder::push_batch(int port, PacketBatch *batch)
{
    auto fnt = [this, port](Packet *p) { return process(port, p); };
    CLASSIFY_EACH_PACKET(noutputs() + 1, fnt, batch, checked_output_push_batch);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(SampleIPForwarder)
