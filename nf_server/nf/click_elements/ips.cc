// ALWAYS INCLUDE <click/config.h> FIRST
#include <click/config.h>

#include "ips.hh"
#include <click/error.hh>
#include <click/args.hh>
#include <click/straccum.hh>
#include <iostream>
#include <queue>
CLICK_DECLS

SampleIPS::SampleIPS() {}

SampleIPS::~SampleIPS() { }

int number2String(unsigned char *dest, const unsigned char *src, int len)
{
  uint32_t cur_p = 0;
  unsigned char cur_v = 0;

  bool first = true;
  for (int i = 0; i < len; ++i)
  {
    char c = src[i];

    if (c >= '0' && c <= '9')
      c -= '0';
    else if (c >= 'a' && c <= 'f')
      c = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      c = c - 'A' + 10;
    else
      continue;

    if (first)
      cur_v = c, first = false;
    else
    {
      dest[cur_p++] = (cur_v << 4) + c;
      cur_v = 0, first = true;
    }
  }

  if (!first)
    dest[cur_p++] = cur_v;

  return cur_p;
}

#define PATTERN_BUFSIZE 1500

void SampleIPS::parse_pattern(String &s, SampleIPS::IPSPattern *pattern)
{
  unsigned char buf[PATTERN_BUFSIZE];
  unsigned char *bufp = buf;
  const unsigned char *strp = (const unsigned char *)s.data();
  int len = s.length(), cur = 0;
  int i, j;

  while ((i = s.find_left("|", cur)) != -1)
  {
    j = s.find_left("|", i + 1);
    if (j == -1)
      break;

    memcpy(bufp, strp + cur, i - cur);
    bufp += i - cur;

    bufp += number2String(bufp, strp + i + 1, j - i - 1);
    cur = j + 1;
  }

  memcpy(bufp, strp + cur, len - cur);
  bufp += len - cur;

  static int index = 0;
  pattern->index = index++;
  pattern->len = (uint32_t)(bufp - buf),
  pattern->data = (unsigned char *)malloc(bufp - buf),
  memcpy(pattern->data, buf, pattern->len);
}

int SampleIPS::configure(Vector<String> &conf, ErrorHandler *errh)
{
  _debug = false;
  _depth = 64;
  if (Args(conf, this, errh)
          .read_mp("DEBUG", _debug)
          .read_mp("DEPTH", _depth)
          .consume() < 0)
    return -1;

  if (conf.size() == 2)
    errh->warning("empty configuration");

  for (int i = 2; i < conf.size(); ++i)
  {
    if (conf[i].length() == 0)
      continue;

    IPSPattern *pattern = new IPSPattern();
    parse_pattern(conf[i], pattern);
    patterns.push_back(pattern);
    pattern_trie.insert(pattern->data, pattern->len, pattern);
  }
  if (_debug)
    print_patterns();
  return 0;
}

void SampleIPS::IPSPattern::print()
{
  printf("%d: ", index);
  for (uint j = 0; j < len; ++j)
    printf("%02x ", data[j]);
  printf("\n");
}

void SampleIPS::print_patterns(void)
{
  printf("Patterns:\n");
  for (int i = 0; i < patterns.size(); ++i)
    patterns[i]->print();
}

int SampleIPS::process(Packet *p)
{
  const unsigned char *data = p->data();
  int len = p->length();
  if (_depth < len)
    len = _depth;
  IPSPattern *pattern;
  if (pattern = (IPSPattern *)pattern_trie.iterSearch(data, len))
  {
    if (_debug && pattern)
      pattern->print();

    return 0;
  }
  return 1;
}

void SampleIPS::push(int, Packet *p)
{
  output(process(p)).push(p);
}

void SampleIPS::push_batch(int, PacketBatch *batch)
{
  CLASSIFY_EACH_PACKET(noutputs() + 1, process, batch, checked_output_push_batch);
}

// bool SampleIPS::pattern_match(SampleIPS::IPSPattern &pt, Packet *p)
// {
//   uint32_t len = p->length() - pt.len;
//   const unsigned char *d = p->data();

//   bool ret;
//   // naive
//   for (int i = 0; i < len; i++)
//   {
//     ret = true;
//     for (int j = 0; j < pt.len; j++)
//     {
//       if (d[i + j] != pt.data[j])
//       {
//         ret = false;
//         break;
//       }
//     }
//     if (ret)
//       return true;
//   }
//   return false;
// }

CLICK_ENDDECLS
EXPORT_ELEMENT(SampleIPS)
