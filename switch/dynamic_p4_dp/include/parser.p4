/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/
#include "define.p4"

parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {

    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            TYPE_IPV4: parse_ipv4;
            TYPE_P4SFC: parse_sfc;
            default: accept;
        }
    }

    state parse_sfc {
        packet.extract(hdr.sfc);
        transition select(hdr.sfc.chainLength) {
            0: parse_ipv4;
            default: parse_nf;
        }
    }

    state parse_nf {
        packet.extract(hdr.nfs.next);
        transition select(hdr.nfs.last.isLast) {
            0: parse_nf;
            default: parse_ipv4;
        }
    }


    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            PROTOCOL_TCP: parse_tcp_udp;
            PROTOCOL_UDP: parse_tcp_udp;
            default: accept;
        }
    }

    state parse_tcp_udp {
        packet.extract(hdr.tcp_udp);
        transition accept;
    }
}