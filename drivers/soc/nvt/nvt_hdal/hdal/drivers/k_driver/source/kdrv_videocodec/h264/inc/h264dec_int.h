#ifndef _H264DEC_INT_H_
#define _H264DEC_INT_H_

#if defined(__FREERTOS)
#define __MODULE__          H264DEC
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#endif

#include "kwrap/type.h"

#include "h26x_def.h"
#include "h26x.h"

#include "h26x_common.h"
#include "h264dec_header.h"

typedef struct _H264DecSeqCfg{
    UINT32 uiSeqCfg;
    UINT32 uiPicSize;
	UINT32 uiPicWidth;
    UINT32 uiPicHeight;
    UINT32 uiRecLineOffset;
}H264DecSeqCfg;

typedef struct _H264DecPicCfg{
    UINT32 uiPicDecNum;
    UINT32 uiPicCfg;
    UINT32 uiSliceCfg1;
    UINT32 uiPicBsLen[2];
}H264DecPicCfg;

typedef struct _H264DEC_CTX{

    H264DecSeqCfg sH264DecSeqCfg;
    H264DecPicCfg sH264DecPicCfg;    
    H264DecHdrObj sH264DecHdrObj;

	H26XDecAddr   sH264VirDecAddr;	
}H264DEC_CTX;

extern void H264Dec_initRegSet(H26XRegSet *pH264RegSet,H26XDecAddr *pH264VirDecAddr);
extern void H264Dec_prepareRegSet(H26XRegSet *pH264RegSet, H26XDecAddr *pH264VirDecAddr, H264DEC_CTX *pContext);
extern void H264Dec_stopRegSet(void);

extern void H264Dec_setFrmIdx(H264DecHdrObj *pH264DecHdrObj, H26XDecAddr *pH264VirDecAddr);
extern void H264Dec_modifyFrmIdx(H264DecHdrObj *pH264DecHdrObj, H26XDecAddr *pH264VirDecAddr);

#endif // _H264DEC_INT_H_

