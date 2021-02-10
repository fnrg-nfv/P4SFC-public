/*************************************************************************
*********************** M E T A D A T A  *********************************
*************************************************************************/

struct metadata {
    bit<8> curElement;
    bit<1> isNFcomplete;
    bit<8> nextStage;
    bit<16> curNfInstanceId;
    bit<8> stageId;
    bit<1> isStageComplete; // 0 for table miss & 1 for table hit
}