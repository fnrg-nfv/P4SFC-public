#ifndef DEFINE_P4
#define DEFINE_P4

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;
typedef bit<1>  boolean_t;
typedef bit<16> transport_port_t;

const bit<9> SERVER_PORT = 178;
const bit<9> DROP_PORT = 511;
const bit<8> NO_STAGE = 255;


// support ethernet type
const bit<16> TYPE_IPV4 = 0x800;
const bit<16> TYPE_P4SFC = 0x1234;

// support ipv4 protocol type
const bit<8> PROTOCOL_ICMP = 0x01;
const bit<8> PROTOCOL_TCP = 0x06;
const bit<8> PROTOCOL_UDP = 0x11;

#define MAX_SFC_LENGTH 10

// support elements
#define ELEMENT_IPREWRITER 0
#define ELEMENT_IPFILTER 1
#define ELEMENT_IPCLASSIFIER 2
#define ELEMENT_NONE 255

#endif