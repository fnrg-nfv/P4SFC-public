/**
    Control which element should be executed
*/

#include "elements.p4"
#include "define.p4"

control ElementControl(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    
    action set_control_data(bit<8> elementId, bit<8> nextStage, boolean_t isNFcomplete) {
        meta.curElement = elementId;
        meta.isNFcomplete = isNFcomplete;
        meta.nextStage = nextStage;
    }

    table chainId_stageId_exact {
        key = {
            hdr.sfc.chainId: exact;
            meta.curNfInstanceId: exact;
            meta.stageId: exact;
        }
        actions = {
            NoAction;
            set_control_data;
        }
        default_action = NoAction();
        size = 1024;
    }

    IpRewriter()        ipRewriter;
    IpFilter()          ipFilter;
    IpClassifier()      ipClassifier;
    apply {
        // before the actual element control logic,
        // we should clear the tag to avoid anomalies
        // e.g, meta.curElement = last_element if table miss
        meta.isStageComplete = 0;
        meta.curElement = ELEMENT_NONE; 
        
        chainId_stageId_exact.apply();
        if(meta.curElement == ELEMENT_NONE) {
            NoAction();
        }
        else if (meta.curElement == ELEMENT_IPREWRITER) {
            ipRewriter.apply(hdr, meta, standard_metadata);
        }
        else if(meta.curElement == ELEMENT_IPFILTER) {
            ipFilter.apply(hdr, meta, standard_metadata);
        }
        else if(meta.curElement == ELEMENT_IPCLASSIFIER) {
            ipClassifier.apply(hdr, meta, standard_metadata);
        }
    }
}