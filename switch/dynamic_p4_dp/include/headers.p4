/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/
#include "define.p4"

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header sfc_t {
    bit<16> chainId;
    bit<16> chainLength;
}

header nf_t {
    bit<15> nfInstanceId;
    bit<1> isLast;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

header tcp_udp_t {
    bit<16>     srcPort;
    bit<16>     dstPort;
}

struct headers {
    ethernet_t  ethernet;
    sfc_t sfc;
    nf_t[MAX_SFC_LENGTH] nfs;
    ipv4_t      ipv4;
    tcp_udp_t   tcp_udp;
}