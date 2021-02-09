#ifndef CLICK_MJTELEMENT_HH
#define CLICK_MJTELEMENT_HH
#include <click/bitvector.hh>
#include <click/batchelement.hh>
#include <queue>
CLICK_DECLS

/*
 * =c
 * MjtElement
 * =s basicsources
 * discards packets
 * =d
 *
 * Like Idle, but does not provide notification.
 *
 * =sa Idle
 */

typedef union
{
  uint16_t data;
  struct a
  {
    uint16_t id : 15;
    uint16_t isLast : 1;
  };
} nf_header_t;

typedef struct
{
  uint16_t id;
  uint16_t len;
  nf_header_t nfs[0];
} p4sfc_header_t;

class P4SFCEncap : public BatchElement
{
public:
  enum
  {
    pull_success,
    pull_fail
  };
  P4SFCEncap() {}
  ~P4SFCEncap() {}

  int configure(Vector<String> &conf, ErrorHandler *errh) CLICK_COLD;

  const char *class_name() const { return "P4SFCEncap"; }
  const char *port_count() const { return "2/2"; }
  const char *processing() const { return "a/a"; }
  // TODO: figure out what flow_code means
  const char *flow_code() const { return "x/x"; }

  void push(int, Packet *) override;
  void push_batch(int, PacketBatch *) override;

protected:
  std::queue<p4sfc_header_t *> _sfch_queue;

private:
  inline Packet *process(int, Packet *);
  inline int pull_p4sfc_header(Packet *, p4sfc_header_t *&);
  inline Packet *push_p4sfc_header(Packet *, p4sfc_header_t *);
};

CLICK_ENDDECLS
#endif
