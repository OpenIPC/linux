#ifndef _H26XENC_INT_H_
#define _H26XENC_INT_H_

#include "kwrap/type.h"

#include "h26x_def.h"

#include "h26x.h"

#include "h26xenc_api.h"

typedef struct _H26XUsrQp_{
	H26XEncUsrQpCfg stCfg;

	UINT32 uiUsrQpMapSize;
	UINT32 uiQpMapAddr;
}H26XUsrQp;

typedef struct _H26XEncSliceSplit_{
	H26XEncSliceSplitCfg stCfg;
	H26XEncSliceSplitCfg stExe;

	UINT32 uiNaluNum;
}H26XEncSliceSplit;

typedef struct _H26XEncGdr_{
	H26XEncGdrCfg stCfg;

	UINT16 usStart;
	UINT32 uiStartPicCnt;
}H26XEncGdr;

typedef struct _H26XEncRoi_{
	UINT8 ucNumber;

	H26XEncRoiCfg stCfg[H26X_MAX_ROI_W];
}H26XEncRoi;

typedef struct _H26XEncRowRc_{
	H26XEncRowRcCfg stCfg;

	UINT8  ucMinCostTh;	// need init while enable rrc //

	UINT8  ucInitQp;
	UINT32 uiPlannedStop;
	UINT32 uiPlannedTop;
	UINT32 uiPlannedBot;
	UINT16 usRefFrmCoeff[H26X_MAX_TILE_NUM][MAX_RRC_FRAME_LEVEL]; // 0 : I. 1 : P
	UINT32 uiFrmCostLsb[MAX_RRC_FRAME_LEVEL]; // 0 : I. 1 : P
	UINT32 uiFrmCostMsb[MAX_RRC_FRAME_LEVEL]; // 0 : I. 1 : P
	UINT32 uiFrmCmpxLsb[MAX_RRC_FRAME_LEVEL]; // 0 : I. 1 : P
	UINT32 uiFrmCmpxMsb[MAX_RRC_FRAME_LEVEL]; // 0 : I. 1 : P
	INT32  iPrevPicBitsBias[MAX_RRC_FRAME_LEVEL];// 0 : I. 1 : P
	UINT32 uiPredBits;

	BOOL  bRRCMode; //0:original, 1: new
	UINT8 ucTileQPStep[2];
	BOOL  bTileQPRst;
	UINT8 ucZeroBitMod;

	INT32 iTH_0;
	INT32 iTH_1;
	INT32 iTH_2;
	INT32 iTH_M;
	INT32 iTH_3;
	INT32 iTH_4;
	INT32 iBeta;
	INT32 m_ModU_Frm[4];  // 0 : I. 1 : P
	INT32 m_ModD_Frm[4];  // 0 : I. 1 : P
	INT64 i_all_ref_pred_tmpl;
	INT32 iLastFrmAvgQPDiff;
	INT32 iTargetBitsScale;
	UINT8 ucNDQPStep;
	UINT8 ucNDQPRange;
	#if RRC_BY_FRAME_LEVEL
	UINT8 ucFrameLevel;
	#endif
}H26XEncRowRc;

typedef struct _H26XEncAq_{
	H26XEncAqCfg stCfg;
}H26XEncAq;

typedef struct _H26XEncLpm_{
	H26XEncLpmCfg stCfg;
}H26XEncLpm;

typedef struct _H26XEncRnd_{
	H26XEncRndCfg stCfg;
}h26XEncRnd;

typedef struct _H26XEncScd_{
	H26XEncScdCfg stCfg;
}H26XEncScd;

typedef struct _H26XEncVar_{
	H26XEncVarCfg stCfg;
}H26XEncVar;

typedef struct _H26XEncOsg_{
	H26XEncOsgRgbCfg stRgb;
	H26XEncOsgWinCfg stWin[10];
	H26XEncOsgPalCfg stPal[16];
	UINT32 uiGcacStatAddr0;
	UINT32 uiGcacStatAddr1;
}H26XEncOsg;

typedef struct _H26XEncMotAq_{
	H26XEncMotAqCfg		stCfg;
	H26XEncMotAddrCfg	stAddrCfg;
    H26XEncMdInfoCfg    stMdInfoCfg;
}H26XEncMotAq;

typedef struct _H26XEncJnd_{
	H26XEncJndCfg stCfg;
}H26XEncJnd;

typedef struct _H26XEncBgr_{
	H26XEncBgrCfg stCfg;
}H26XEncBgr;

typedef struct _H26XEncRmd_{
	H26XEncRmdCfg stCfg;
}H26XEncRmd;

typedef struct _H26XEncTnr_{
	H26XEncTnrCfg stCfg;
}H26XEncTnr;

typedef struct _H26XEncLambda_{
	H26XEncLambdaCfg stCfg;
}H26XEncLambda;

typedef struct _H26XEncEarlySkip_{
	H26XEncEarlySkipCfg stCfg;
} H26XEncEarlySkip;

typedef struct _H26XEncSpn_{
	H26XEncSpnCfg stCfg;
} H26XEncSpn;
typedef struct _H26XFUNC_CTX_{
	H26XUsrQp			stUsrQp;
	H26XEncSliceSplit	stSliceSplit;
	H26XEncGdr			stGdr;
	H26XEncRoi			stRoi;
	H26XEncRowRc		stRowRc;
	H26XEncAq			stAq;
	H26XEncLpm			stLpm;
	h26XEncRnd			stRnd;
	H26XEncScd			stScd;
	H26XEncOsg			stOsg;
	H26XEncMotAq		stMAq;
	H26XEncJnd			stJnd;
	H26XEncQpRelatedCfg	stQR;
	H26XEncVarCfg		stVar;

	// nt98528 //
	H26XEncBgr			stBgr;
    H26XEncRmd          stRmd;
    H26XEncTnr          stTnr;
    H26XEncLambda       stLambda;
	H26XEncEarlySkip	stEarlySkip;
	H26XEncSpn			stSpn;
	
	BOOL bPSNREn;	
}H26XFUNC_CTX;

extern void h26XEnc_getRowRcState(SLICE_TYPE ePicType, H26XRegSet *pRegSet, H26XEncRowRc *pRowRc, UINT32 uiSVCId);
extern void h26XEnc_setInitSde(H26XRegSet *pRegSet);
extern UINT8 h26XEnc_getRcQuant(H26XENC_VAR *pVar, SLICE_TYPE ePicType, BOOL bIsLTRFrm, UINT32 uiBsOutBufSize);
extern void h26XEnc_UpdateRc(H26XENC_VAR *pVar, SLICE_TYPE ePicType, H26XEncResultCfg *pResult);

#endif	//_H26XENC_INT_H_

