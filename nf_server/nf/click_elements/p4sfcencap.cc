#include <bitset>
#include <click/config.h>
#include <iostream>

#include "p4sfcencap.hh"

CLICK_DECLS

int P4SFCEncap::configure(Vector<String> &conf, ErrorHandler *errh)
{
  in_batch_mode = BATCH_MODE_YES;
}

inline Packet *P4SFCEncap::process(int input, Packet *p)
{
  p4sfc_header_t *sfch;
  if (input == 0)
  {
    int result = pull_p4sfc_header(p, sfch);
    if (result == pull_fail)
    {
      p->kill();
      return 0;
    }
    _sfch_queue.push(sfch);
    return p;
  }
  else if (input == 1)
  {
    // assert(!_sfch_queue.empty());
    if (_sfch_queue.empty())
    {
      p->kill();
      return 0;
    }

    sfch = _sfch_queue.front();
    _sfch_queue.pop();
    p = push_p4sfc_header(p, sfch);
    free(sfch);
    return p;
  }
  p->kill();
  return 0;
}

void P4SFCEncap::push(int input, Packet *p)
{
  Packet *q = process(input, p);
  if (q)
    output(input).push(q);
}

void P4SFCEncap::push_batch(int input, PacketBatch *batch)
{
  auto fnt = [this, input](Packet *p) { return process(input, p); };
  EXECUTE_FOR_EACH_PACKET_DROPPABLE(fnt, batch, [](Packet *p) {})
  if (batch)
    output(input).push_batch(batch);
}

inline int P4SFCEncap::pull_p4sfc_header(Packet *p, p4sfc_header_t *&sfch)
{
  const unsigned char *data = p->data();
  uint16_t len = ntohs(*(uint16_t *)(data + 2));

  if (len == 0 || len > 100)
  {
    click_chatter("drop(sfc len: %d)\n", len);
    return pull_fail;
  }

  size_t size = sizeof(p4sfc_header_t) + len * sizeof(nf_header_t);

  sfch = (p4sfc_header_t *)malloc(size);
  sfch->id = ntohs(*(uint16_t *)data);
  sfch->len = len;
  data += sizeof(p4sfc_header_t);

  for (int i = 0; i < len; i++, data += sizeof(nf_header_t))
  {
    sfch->nfs[i].data = ntohs(*(const uint16_t *)data);
  }

  p->pull(size);
  return pull_success;
}

inline Packet *P4SFCEncap::push_p4sfc_header(Packet *p, p4sfc_header_t *sfch)
{
  size_t push_size = sizeof(p4sfc_header_t) + (sfch->len - 1) * sizeof(nf_header_t);
  WritablePacket *wp = p->push(push_size);
  const unsigned char *data = wp->data();

  *(uint16_t *)data = htons(sfch->id);
  data += 2;

  *(uint16_t *)data = htons(sfch->len - 1);
  data += 2;

  for (int i = 1; i < sfch->len; i++, data += sizeof(nf_header_t))
  {
    *(uint16_t *)data = htons(sfch->nfs[i].data);
  }

  return wp;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(P4SFCEncap)
ELEMENT_MT_SAFE(P4SFCEncap)
