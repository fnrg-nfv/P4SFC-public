/**
    Control how packets are forwarded according to sfc_id.
*/
#include "define.p4"

control ForwardControl(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    action drop() {
        mark_to_drop(standard_metadata);
    }

    action set_output_port(egressSpec_t port) {
        standard_metadata.egress_spec = port;
    }

    action send_to_server() {
        standard_metadata.egress_spec = SERVER_PORT;
    }

    table chainId_chainLength_exact {
        key = {
            hdr.sfc.chainId: exact;
            hdr.sfc.chainLength: exact;
        }
        actions = {
            set_output_port;
            send_to_server;
            drop;
        }
        size = 1024;
        default_action = drop();
    }

    apply {
        chainId_chainLength_exact.apply();
    }
}