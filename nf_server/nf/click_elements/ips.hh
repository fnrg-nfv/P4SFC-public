#ifndef IPS_HH
#define IPS_HH
#include <click/batchelement.hh>
CLICK_DECLS

#define TRIENODESIZE 256
struct TrieNode
{
  void *flag;
  int depth;
  TrieNode *failure;
  TrieNode *parent;
  TrieNode *child[TRIENODESIZE];
  TrieNode(int d, TrieNode *p)
  {
    depth = d;
    parent = p;
    flag = NULL;
    for (int i = 0; i < TRIENODESIZE; ++i)
      child[i] = NULL;
  }
  ~TrieNode()
  {
    for (size_t i = 0; i < TRIENODESIZE; i++)
      child[i]->~TrieNode();
    delete[] this;
  }
};

class Trie
{
private:
public:
  TrieNode *root = new TrieNode(0, 0);
  void insert(unsigned char *pattern, int len, void *val)
  {
    TrieNode *cur = root;
    for (int i = 0; i < len; ++i)
    {
      uint8_t cid = pattern[i];
      if (cur->child[cid] == NULL)
        cur->child[cid] = new TrieNode(0, 0);
      cur = cur->child[cid];
    }
    if (!(cur->flag))
      cur->flag = val;
  }
  void *search(const unsigned char *big_str, int len)
  {
    TrieNode *cur = root;
    for (int i = 0; i < len; ++i)
    {
      int cid = big_str[i];
      if (cur->flag)
      {
        return cur->flag;
      }

      if (cur->child[cid] == NULL)
      {
        return NULL;
      }
      cur = cur->child[cid];
    }
    return cur->flag;
  }
  void *iterSearch(const unsigned char *big_str, int len)
  {
    void *ret = NULL;
    for (int i = 0; i < len; ++i)
    {
      if (ret = search(big_str + i, len - i))
      {
        return ret;
      }
    }
    return NULL;
  }
};

class SampleIPS : public BatchElement
{
public:
  SampleIPS();
  ~SampleIPS();

  const char *class_name() const { return "SampleIPS"; }
  const char *port_count() const { return "1/2"; }
  const char *processing() const { return PUSH; }

  int configure(Vector<String> &conf, ErrorHandler *errh);

  void push(int, Packet *);
  void push_batch(int, PacketBatch *);

  struct IPSPattern
  {
    int index;
    uint32_t len;
    unsigned char *data;

    void print();
  };
  bool pattern_match(IPSPattern &pt, Packet *p);
  // bool pattern_match(String &pt, Packet *p);

protected:
  int process(Packet *p);
  void print_patterns(void);
  void parse_pattern(String &s, IPSPattern *pattern);
  Trie pattern_trie;

  bool _debug;
  uint32_t _depth;
  Vector<IPSPattern*> patterns;
};

CLICK_ENDDECLS
#endif
