/*************************************************************************
***********************  D E P A R S E R  *******************************
*************************************************************************/

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.sfc);
        packet.emit(hdr.nfs);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp_udp);
    }
}