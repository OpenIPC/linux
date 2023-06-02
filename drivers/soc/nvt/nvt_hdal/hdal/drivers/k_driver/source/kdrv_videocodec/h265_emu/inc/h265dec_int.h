#ifndef _H265DEC_INT_H_
#define _H265DEC_INT_H_

#include "kwrap/type.h"

#include "h26x_def.h"
//#include "h26x_int.h"
#include "h26x.h"
#include "h265dec_header.h"
#include "h26x_common.h"

#if defined(__FREERTOS)
#define __MODULE__          H265DEC
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#endif

typedef struct _H265DecSeqCfg {
	UINT32 uiSeqCfg;
	UINT32 uiPicSize;
	UINT32 uiPicWidth;
	UINT32 uiPicHeight;
	UINT32 uiRecLineOffset;
    UINT32 uiTileWidth[MaxTileCols];
} H265DecSeqCfg;

typedef struct _H265DecPicCfg {
	UINT32 uiPicDecNum;
	UINT32 uiPicCfg;
	UINT32 uiSliceCfg1;
	UINT32 uiPicBsLen[2];
} H265DecPicCfg;

typedef struct _H265DecAddr {
	UINT32 uiRefYAddr[2], uiRefUVAddr[2];
	UINT32 uiRecYAddr[2], uiRecUVAddr[2];
	UINT32 uiColInfoAddr;
	UINT32 uiURIAddr;
	UINT32 uiRecURAddr;
	UINT32 uiCMDBufAddr[2];
	UINT32 uiHwBsAddr[2];
} H265DecAddr;

typedef struct _H265DEC_CTX {

	H265DecSeqCfg sH265DecSeqCfg;
	H265DecPicCfg sH265DecPicCfg;
	H265DecHdrObj sH265DecHdrObj;

	H26XDecAddr   sH265VirDecAddr;
	H26XRegSet    sH265DecRegSet;

} H265DEC_CTX;

extern void H265Dec_initRegSet(H26XRegSet *pH265RegSet, H26XDecAddr *pH265VirDecAddr);
extern void H265Dec_prepareRegSet(H26XRegSet *pH265RegSet, H26XDecAddr *pH265VirDecAddr, H265DEC_CTX *pContext);
extern void H265Dec_stopRegSet(void);

#endif // _H265DEC_INT_H_

