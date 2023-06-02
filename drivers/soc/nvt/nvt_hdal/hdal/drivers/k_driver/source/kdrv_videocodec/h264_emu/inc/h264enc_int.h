#ifndef _H264_INT_H_
#define _H264_INT_H_

#define __MODULE__          H264ENC
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER

#include "kwrap/type.h"

#include "h26x.h"
#include "h26x_def.h"
#include "h26x_common.h"
#include "h26xenc_int.h"

#include "h264_def.h"
#include "h264enc_api.h"



typedef struct _H264EncSeqCfg_{
	H264_PROFILE eProfile;
	H264_ENTROPY eEntropyMode;

	UINT8   ucLevelIdc;
    UINT16	usWidth;
    UINT16	usHeight;
	UINT16	usMbWidth;
	UINT16	usMbHeight;
	UINT32	uiTotalMBs;

	UINT32	uiDisplayWidth;
	UINT32	uiGopNum;
    UINT16	usLog2MaxFrm;
    UINT16	usLog2MaxPoc;

	INT8	cChrmQPIdx;
	INT8	cSecChrmQPIdx;
	UINT8   ucDisLFIdc;
    INT8    cDBAlpha;
    INT8    cDBBeta;

	BOOL	bTrans8x8En;
    BOOL	bDirect8x8En;		// with profile, not from init  //
    BOOL	bForwardRecChrmEn;

	// reference frame //
    UINT8	ucSVCLayer;
	UINT32	uiLTRInterval;
	BOOL	bLTRPreRef;
	UINT8	ucNumRefIdxL0;

	// rate control //
	UINT32	uiBitRate;
    UINT32	uiFrmRate;
	UINT8	ucInitIQP;
    UINT8	ucInitPQP;

	// function //
	BOOL	bFBCEn;
	BOOL	bGrayEn;
	BOOL	bGrayColorEn;
	BOOL	bFastSearchEn;
	BOOL	bHwPaddingEn;
	UINT8	ucRotate;
    BOOL    bD2dEn;

	// vui header //
	BOOL	bVUIEn;
    UINT16  usSarWidth;
    UINT16  usSarHeight;
    UINT8   ucMatrixCoef;
    UINT8   ucTransferCharacteristics;
    UINT8   ucColourPrimaries;
    UINT8   ucVideoFormat;
    UINT8   ucColorRange;
    UINT8   bTimeingPresentFlag;
}H264EncSeqCfg;

typedef struct _H264EncPicCfg_{
	SLICE_TYPE ePicType;

    UINT8	ucSliceQP;

    UINT32	uiPicCnt;          ///< every picture ++
    UINT32	uiPicCntInGop;     ///< Nth picture in gop

	INT16	sPoc;
	INT16	sPocLsb;

	UINT32	uiNalHdrLenSum;
	UINT32	uiNalRefIdc;
	BOOL	bLastRecIsLT;

	UINT32	config_loop_filter;	// inloop filter (ilf) //
	UINT32	cabac_init_idc;

	UINT32	uiSrcTimeStamp;
	UINT32 	uiSliceRowNum;
    UINT32  uiAvgQP;
}H264EncPicCfg;


typedef struct _H264ENC_CTX_{
    H264EncSeqCfg	stSeqCfg;
    H264EncPicCfg	stPicCfg;
	H264EncRdoCfg   stRdoCfg;
	H264EncFroCfg   stFroCfg;

	H26XEncAddr		stAddr;
	H26XEncRcCfg	stRcCfg;

	UINT32 uiRecYLineOffset;
	UINT32 uiRecCLineOffset;
	UINT32 uiSideInfoLineOffset;

	H26XFRM_IDX eRecIdx;
	H26XFRM_IDX eRefIdx;
	H26XFRM_IDX eLastRecIdx;

	UINT32 uiSeqHdrLen;
	UINT32 uiSPSHdrLen;
	UINT32 uiPPSHdrLen;
	UINT32 uiSvcLable;
} H264ENC_CTX;

extern void h264Enc_initCfg(H264ENC_INIT *pInit, H26XENC_VAR *pVar);
extern void h264Enc_updatePicCfg(H264ENC_CTX *pVdoCtx);
extern void h264Enc_modifyRefFrm(H264ENC_CTX *pVdoCtx, UINT32 uiPicCnt, BOOL bRecFBCEn);
extern void h264Enc_updateRowRcCfg(SLICE_TYPE ePicType, H26XEncRowRc *pRowRc);
extern void h264Enc_initRrc(H264ENC_CTX *pVdoCtx, H26XEncRowRc *pRowRc, BOOL bIsPFrm, INT32 iCurFrameNo, UINT8  usInitQp, UINT32 uiAvgQP);
extern INT32 h264Enc_setRowRcCfg2(H26XENC_VAR *pVar, H26XEncRowRc *pRowRc);

#endif

