
// ALWAYS INCLUDE <click/config.h> FIRST
#include <click/config.h>

#include "sampleipfilter.hh"
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

SampleIPFilter::SampleIPFilter()
{
    in_batch_mode = BATCH_MODE_IFPOSSIBLE;
}

SampleIPFilter::~SampleIPFilter() {}

int SampleIPFilter::configure(Vector<String> &conf, ErrorHandler *errh)
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

        SampleIPFilterEntry *e = parse(words, errh);
        _map[e->key()] = e;
        // if (_debug)
        //     click_chatter("entry: %s", e->unparse().c_str());
    }
}

SampleIPFilterEntry::SampleIPFilterEntry(SampleIPFilterEntry::Key &key, bool allowed) : _key(key), _allowed(allowed) {}

SampleIPFilterEntry *SampleIPFilter::parse(Vector<String> &words, ErrorHandler *errh)
{
    size_t size = words.size();
    if (size != 4)
    {
        errh->message("firewall rule must be 4-tuple, but is a %d-tuple", size);
        return NULL;
    }

    bool allowed = false;
    IPAddress src, dst;
    uint8_t proto = 0;

    if (words[0].compare("allow") == 0)
        allowed = true;
    else if (words[0].compare("deny") == 0)
        allowed = false;
    else
        errh->message("format error %s", words[0]);

    if (!IPAddressArg().parse(words[1], src, this))
        errh->message("src IPAddress: parse error");
    if (!IPAddressArg().parse(words[2], dst, this))
        errh->message("dst IPAddress: parse error");
    if (!IntArg().parse(words[3], proto))
        errh->message("bad proto number %s", words[3].c_str());
    SampleIPFilterEntry::Key key(src, dst, proto);
    return new SampleIPFilterEntry(key, allowed);
}

int SampleIPFilter::process(int port, Packet *p)
{
    IPFlow5ID flowid(p);
    int out = 0;
    SampleIPFilterEntry::Key key(flowid.saddr(), flowid.daddr(), flowid.proto());
    auto it = _map.find(key);
    if (it != _map.end())
        if (it->second->allowed())
            out = 0;

    if (_debug)
        click_chatter("Filter the packet to port %x.", out);
    return out;
}

void SampleIPFilter::push(int port, Packet *p)
{
    checked_output_push(process(port, p), p);
}

void SampleIPFilter::push_batch(int port, PacketBatch *batch)
{
    auto fnt = [this, port](Packet *p) { return process(port, p); };
    CLASSIFY_EACH_PACKET(noutputs() + 1, fnt, batch, checked_output_push_batch);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(SampleIPFilter)
