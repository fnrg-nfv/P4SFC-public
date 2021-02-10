/** P4 Implementation for each element supported by P4SFC
*/
#include "define.p4"


// the default action for each element can be NoAction
// because the nextStage will be set as NO_STAGE in the
// element_complete_control logic as the isStageComplete is 0

control IpRewriter(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    direct_counter(CounterType.packets_and_bytes) rule_frequency;

    action p4sfcNoAction() {
        rule_frequency.count();
    }

    action rewrite(ip4Addr_t srcAddr, ip4Addr_t dstAddr, transport_port_t srcPort, transport_port_t dstPort) {
        meta.isStageComplete = 1;
        hdr.ipv4.srcAddr = srcAddr;
        hdr.ipv4.dstAddr = dstAddr;
        hdr.tcp_udp.srcPort = srcPort;
        hdr.tcp_udp.dstPort = dstPort;
        
        rule_frequency.count();
    }

    table IpRewriter_exact {
        key = {
            hdr.sfc.chainId: exact;
            meta.curNfInstanceId: exact;
            meta.stageId: exact;
            hdr.ipv4.srcAddr: exact;
            hdr.ipv4.dstAddr: exact;
            hdr.tcp_udp.srcPort: exact;
            hdr.tcp_udp.dstPort: exact;
        }
        actions = {
            p4sfcNoAction;
            rewrite;
        }
        default_action = p4sfcNoAction();
        counters=rule_frequency;
        size = 1024;
    }

    apply{
        IpRewriter_exact.apply();
    }
}

control IpFilter(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    
    direct_counter(CounterType.packets_and_bytes) rule_frequency;
    
    action p4sfcNoAction() {
        rule_frequency.count();
    }

    action allow() {
        meta.isStageComplete = 1;
        rule_frequency.count();
    }

    action drop() {
        meta.isStageComplete = 1;
        mark_to_drop(standard_metadata);

        rule_frequency.count();
    }

    table IpFilter_exact {
        key = {
            hdr.sfc.chainId: exact;
            meta.curNfInstanceId: exact;
            meta.stageId: exact;
            hdr.ipv4.srcAddr: exact;
            hdr.ipv4.dstAddr: exact;
            hdr.ipv4.protocol: exact;
        }
        actions = {
            p4sfcNoAction;
            allow;
            drop;
        }
        counters=rule_frequency;
        default_action = p4sfcNoAction();
        size = 1024;
    }
    
    apply{
        IpFilter_exact.apply();
    }
}

control IpClassifier(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    
    direct_counter(CounterType.packets_and_bytes) rule_frequency;

    action p4sfcNoAction() {
        rule_frequency.count();
    }

    action drop() {
        meta.isStageComplete = 1;
        mark_to_drop(standard_metadata);
        rule_frequency.count();
    }
    
    action forward(bit<16> port) {
        meta.isStageComplete = 1;
        standard_metadata.egress_spec = (bit<9>) port;

        rule_frequency.count();
    }

    table IpClassifier_exact {
        key = {
            hdr.sfc.chainId: exact;
            meta.curNfInstanceId: exact;
            meta.stageId: exact;
            hdr.ipv4.dstAddr: exact;
        }
        actions = {
            p4sfcNoAction;
            drop;
            forward;
        }
        counters=rule_frequency;
        default_action = p4sfcNoAction();
        size = 1024;
    }
    
    apply{
        IpClassifier_exact.apply();
    }
}