/*
 * simpleidle.{cc,hh} -- element sits there and kills packets sans notification
 * Eddie Kohler
 *
 * Copyright (c) 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include <click/error.hh>
#include <stdio.h>

#include "customencap.hh"

CLICK_DECLS

CustomEncap::CustomEncap() {}

int CustomEncap::configure(Vector<String> &conf, ErrorHandler *errh)
{
  if (conf.size() != 1)
  {
    errh->warning("error configuration");
    _len = 0;
  }
  else
  {
    parse_pattern(conf[0]);
    errh->debug("CE config: %s\n", conf[0].c_str());
  }
  return 0;
}

void CustomEncap::parse_pattern(String &s)
{
  char *data = (char *)malloc(s.length() / 2);
  uint32_t cur_p = 0;
  unsigned char cur_v = 0;

  bool first = true;
  for (int i = 0; i < s.length(); ++i)
  {
    char c = s.at(i);

    if (c >= '0' && c <= '9')
      c -= '0';
    else if (c >= 'a' && c <= 'f')
      c = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      c = c - 'A' + 10;
    else
      continue;

    if (first)
    {
      cur_v = c, first = false;
    }
    else
    {
      data[cur_p++] = (cur_v << 4) + c;
      cur_v = 0, first = true;
    }
  }

  if (!first)
    data[cur_p++] = cur_v;

  _len = cur_p;
  _header = (char *)realloc(data, cur_p);
}

// void CustomEncap::push(int input, Packet *p) {
//   assert(input == 0);
//   p = simple_action(p);
//   output(0).push(p);
// }

#if HAVE_BATCH
PacketBatch *
CustomEncap::simple_action_batch(PacketBatch *head)
{
  Packet *p = head;
  while (p != NULL)
  {
    p = p->push(_len);
    const unsigned char *pdata = p->data();
    memcpy((char *)pdata, _header, _len);
    p = p->next();
  }
  return head;
}
#endif

Packet *CustomEncap::simple_action(Packet *p)
{
  p = p->push(_len);
  const unsigned char *pdata = p->data();
  memcpy((char *)pdata, _header, _len);
  return p;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(CustomEncap)
ELEMENT_MT_SAFE(CustomEncap)
