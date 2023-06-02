#ifndef _H26X_COMMON_H_
#define _H26X_COMMON_H_

#if defined(__LINUX)
#include <linux/string.h>
#include <linux/mm.h>
#elif defined(__FREERTOS)
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#endif

#include "kwrap/type.h"

#include "h26x_def.h"
#include "h26xenc_rc.h"

#define Clip3_JM(min,max,val)	(((val)<(min))?(min):(((val)>(max))?(max):(val)))

typedef enum {
	FRM_IDX_ST_0 = 0,
	FRM_IDX_LT_0,
	FRM_IDX_ST_1,
	FRM_IDX_NON_REF,
	FRM_IDX_MAX
} H26XFRM_IDX;

typedef enum{
	H26X_LL_START = 0,
	H26X_LL_WR,
	H26X_LL_RD,
	H26X_LL_FINISH = 15
}H26X_LL_CMD;

typedef struct _H26XEncAddr_{
	UINT32 uiRecRefY[FRM_IDX_MAX], uiRecRefC[FRM_IDX_MAX];
	UINT32 uiRecYLineOffset, uiRecCLineOffset;
	UINT32 uiColMvs[FRM_IDX_MAX];
	UINT32 uiSideInfo[2][FRM_IDX_MAX];
	UINT32 uiSideInfoLineOffset;
	UINT32 uiRRC[2][2];
	UINT32 uiTileExtraY[H26X_MAX_TILE_NUM-1][FRM_IDX_MAX]; // for tile mode
	UINT32 uiTileExtraC[H26X_MAX_TILE_NUM-1][FRM_IDX_MAX]; // for tile mode
	UINT32 uiFrmBufNum;
	UINT32 uiBsdma;
	UINT32 uiNaluLen;
	UINT32 uiSeqHdr;
	UINT32 uiPicHdr;
	UINT32 uiAPBAddr;
	UINT32 uiBsOutAddr;
	UINT32 uiLLCAddr;
	UINT32 uiSliceHdr;
	H26XFRM_IDX eRecIdx, eRefIdx;
} H26XEncAddr;

typedef struct _H26XEncRCCfg_{
	UINT8 ucMaxIQp;			///< Rate Control's Max I Qp
    UINT8 ucMinIQp;			///< Rate Control's Min I Qp
    UINT8 ucMaxPQp;			///< Rate Control's Max P Qp
    UINT8 ucMinPQp;			///< Rate Control's Min P Qp
}H26XEncRcCfg;

typedef struct _H26XEncIspRatioCfg_{
	UINT8 ucRatioBse;
	UINT8 ucEdgeRatio;
	UINT8 ucDn2DRatio;
	UINT8 ucDn3DRatio;
}H26XEncIspRatioCfg;

typedef struct _H26XCOMN_CTX_{
	H26XEncAddr		stVaAddr;
	H26XEncRC		stRc;
    H26XEncIspRatioCfg stIspRatio;

	UINT32 uiWidth;
	UINT32 uiHeight;
	UINT32 uiPicCnt;
	UINT32 uiEncRatio;
	UINT32 uiLTRInterval;
	UINT8  ucSVCLayer;
	UINT8  ucRcMode;
	UINT32 uiLastHwInt;

	UINT32 uipRcLogAddr;
	UINT32 uiRcLogLen;
}H26XCOMN_CTX;

typedef struct _H26XDecAddr {
	UINT32 uiFrmBufNum;
	UINT32 uiRefAndRecYAddr[FRM_IDX_MAX], uiRefAndRecUVAddr[FRM_IDX_MAX];
	INT32  iRefAndRecPOC[FRM_IDX_MAX];
	INT32  iColRefAndRecPOC[FRM_IDX_MAX];
	UINT32 uiRefAndRecIsIntra[FRM_IDX_MAX];
	UINT32 uiRefAndRecIsLT[FRM_IDX_MAX];
	UINT32 uiRef0Idx;
	UINT32 uiRecIdx;
	UINT32 uiURIAddr;
	UINT32 uiRecURAddr;
	UINT32 uiIlfUpBotAddr;
	UINT32 uiIlfSideInfoAddr[FRM_IDX_MAX];
	UINT32 uiIlfRIdx;
	UINT32 uiIlfWIdx;
	UINT32 uiCMDBufAddr;
	UINT32 uiHwBsAddr;

	UINT32 uiHwBSCmdNum;
	UINT32 uiHwBsCmdSize[H26X_MAX_BSDMA_NUM];

	UINT32 uiColMvsAddr[FRM_IDX_MAX];
	UINT32 uiColMvRIdx;
	UINT32 uiColMvWIdx;

	UINT32 uiResYAddr;
	UINT32 uiDummyWTAddr;
} H26XDecAddr;

extern int g_rc_dump_log;

UINT32 log2bin(UINT32 uiVal);

void save_to_reg(UINT32 *reg, INT32 val, UINT32 b_addr, UINT32 bits);
UINT8 bit_reverse(UINT8 pix);
UINT32 get_from_reg(UINT32 reg, UINT32 b_addr, UINT32 bits);
UINT32 set_ll_cmd(H26X_LL_CMD cmd, int job_id, int offset, int burst);
void SetMemoryAddr(UINT32 *puiVirAddr, UINT32 *puiVirMemAddr, UINT32 uiSize);
UINT32 rbspToebsp(UINT8 *pucAddr, UINT8 *pucBuf, UINT32 uiSize);
UINT32 ebspTorbsp(UINT8 *pucAddr, UINT8 *pucBuf, UINT32 uiTotalBytes);
INT32 h26x_setLLCmd(UINT32 id, UINT32 uiVaApbAddr, UINT32 uiVaCurLLAddr, UINT32 uiPaNxtLLAddr,UINT32 uiLLBufSize);
UINT32 buf_chk_sum(UINT8 *buf, UINT32 size, UINT32 format);

#endif // _H26X_COMMON_H_
