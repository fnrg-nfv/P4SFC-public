/**
    Perform the necessary operations after each element completes its own logic.
*/ 

control ElementCompleteControl(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {

    apply {
        // Element may drop packet, if so, subsequent elements are unnecessary
        if(standard_metadata.egress_spec == DROP_PORT || meta.isStageComplete == 0) {
            meta.nextStage = NO_STAGE;
        }
        else {
            meta.stageId = meta.stageId + 1;
            if (meta.isNFcomplete == 1) {
                hdr.nfs.pop_front(1); // remove completed nf
                hdr.sfc.chainLength = hdr.sfc.chainLength - 1;
                meta.stageId = 0;
            }
        }
    }
}