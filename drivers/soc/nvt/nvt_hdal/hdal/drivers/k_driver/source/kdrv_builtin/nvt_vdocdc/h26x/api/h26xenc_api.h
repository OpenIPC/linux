#ifndef _H26XENC_API_H_
#define _H26XENC_API_H_

#include "kwrap/type.h"
#include "h26x_def.h"
#include "h26xenc_rc.h"
#include "h26x_common.h"

#define H26X_AQ_TH_TBL_SIZE	(30)

typedef struct _H26XEncFroCfg_{
	BOOL bEnable;
}H26XEncFroCfg;

typedef struct _H26XEncUsrQpCfg_{
	BOOL bEnable;

	UINT32 uiQpMapAddr;
	UINT32 uiQpMapSize;
	UINT32 uiQpMapLineOffset;
}H26XEncUsrQpCfg;

typedef struct _H26XEncSliceSplitCfg_ {
	BOOL   bEnable;        // enable multiple slice , range : 0 ~ 1 //
	UINT32 uiSliceRowNum;   // number of macroblock rows occupied by a slice , range : 1 ~ SIZE16X(picture_height)
} H26XEncSliceSplitCfg;

typedef struct _H26XEncVarCfg_{
	UINT16 usThreshold;
	UINT8  ucAVGMin;
	UINT8  ucAVGMax;
	UINT8  ucDelta;
	UINT8  ucIRangeDelta;
	UINT8  ucPRangeDelta;
	UINT8  ucMerage;
}H26XEncVarCfg;

typedef struct _H26XEncGdrCfg_{
	BOOL bEnable;

	UINT16 usPeriod;
	UINT16 usNumber;
    //UINT16 usGdrQpEn;
    //UINT16 usGdrQp;
}H26XEncGdrCfg;

typedef struct _H26XEncRoiCfg_{
	BOOL bEnable;

	UINT16 usCoord_X;
	UINT16 usCoord_Y;
	UINT16 usWidth;
	UINT16 usHeight;
	INT8   cQP;
	UINT8  ucMode;
}H26XEncRoiCfg;

typedef struct _H26XEncRowRCCfg_{
	BOOL bEnable;
	UINT8 ucPredWt[2]; // 0 : I. 1 : P
	UINT8 ucScale;
	UINT8 ucQPRange[2];
	UINT8 ucQPStep[2];
	UINT8 ucMinQP[2]; // 0 : I. 1 : P
	UINT8 ucMaxQP[2]; // 0 : I. 1 : P
	UINT32 uiInitFrmCoeff;
}H26XEncRowRcCfg;

typedef struct _H26XEncAqCfg_{
	BOOL bEnable;

	UINT8  ucIC2;
	UINT8  ucIStr;
	UINT8  ucPStr;
	INT8   cMaxDQp;
	INT8   cMinDQp;
	UINT8  ucAslog2;
	UINT8  ucDepth;
	UINT8  ucPlaneX;
	UINT8  ucPlaneY;
	INT16  sTh[H26X_AQ_TH_TBL_SIZE];

	BOOL   bAqMode;
	UINT8  ucIStr1;     //HEVC only, cu32
	UINT8  ucPStr1;     //HEVC only, cu32
	UINT8  ucIStr2;     //HEVC only, cu16
	UINT8  ucPStr2;     //HEVC only, cu16
}H26XEncAqCfg;

typedef struct _H26XEncLpmCfg_{
	BOOL bEnable;

	UINT8 ucRmdSadEn;
	UINT8 ucIMEStopEn;
	UINT8 ucIMEStopTh;
	UINT8 ucRdoStopEn;
	UINT8 ucRdoStopTh;
	UINT8 ucChrmDmEn;
	UINT8 ucQPMapDefulat;
	UINT8 ucI16On;
	UINT8 ucIraEn;
	UINT8 ucIraTh;
}H26XEncLpmCfg;

typedef struct _H26XEncRndCfg_{
	BOOL bEnable;

	unsigned int  uiSeed;
	unsigned char ucRange;
}H26XEncRndCfg;

typedef struct _H26XEncScdCfg_{
	BOOL   bStop;

	UINT16 usTh;
	UINT8  ucSc;
	UINT8  ucOverrideRowRC;
}H26XEncScdCfg;

typedef struct _H26XEncOsgRgbCfg_{
	UINT8  ucRgb2Yuv[3][3];
}H26XEncOsgRgbCfg;

typedef struct _H26XEncOsgPalCfg_{
	UINT8 ucAlpha;
	UINT8 ucRed;
	UINT8 ucGreen;
	UINT8 ucBlue;
}H26XEncOsgPalCfg;


typedef struct _H26XEncOsgGrapCfg_{
	UINT8  ucType;
	UINT16 usWidth;
	UINT16 usHeight;
	UINT16 usLofs;
	UINT32 uiAddr;
	UINT8  ucDropUV;
}H26XEncOsgGrapCfg;

typedef struct _H26XEncOsgDispCfg_{
	UINT8  ucMode;
	UINT16 usXStr;
	UINT16 usYStr;
	UINT8  ucBgAlpha;
	UINT8  ucFgAlpha;
	UINT8  ucMaskType;
	UINT8  ucMaskBdSize;
	UINT8  ucMaskY[2];
	UINT8  ucMaskCb;
	UINT8  ucMaskCr;
}H26XEncOsgDispCfg;

typedef struct _H26XEncOsgGcacCfg_{
	BOOL bEnable;

	UINT8 ucBlkWidth;
	UINT8 ucBlkHeight;
	UINT8 ucBlkHNum;
	UINT8 ucOrgColorLv;
	UINT8 ucInvColorLv;
	UINT8 ucNorDiffTh;
	UINT8 ucInvDiffTh;
	UINT8 ucStaOnlyMode;
	UINT8 ucFillEvalMode;
	UINT8 ucEvalLumTarg;
}H26XEncOsgGcacCfg;

typedef struct _H26XEncOsgQpmapCfg_{
	UINT8 ucLpmMode;
	UINT8 ucTnrMode;
	UINT8 ucFroMode;
	UINT8 ucQpMode;
	INT8 cQpVal;
	UINT8 ucBgrMode;
	UINT8 ucMaqMode;
	UINT8 ucSkipLabel;
}H26XEncOsgQpmapCfg;

typedef struct _H26XEncOsgColorkeyCfg_{
	BOOL bEnable;
	BOOL bAlphaEn;
	UINT8 ucAlpha;
	UINT8 ucRed;
	UINT8 ucGreen;
	UINT8 ucBlue;
}H26XEncOsgColorkeyCfg;


typedef struct _H26XEncOsgWinCfg_{
	BOOL bEnable;

	H26XEncOsgGrapCfg stGrap;
	H26XEncOsgDispCfg stDisp;
	H26XEncOsgGcacCfg stGcac;
	H26XEncOsgQpmapCfg stQpmap;
	H26XEncOsgColorkeyCfg stKey;
}H26XEncOsgWinCfg;

typedef struct _H26XEncMotAqCfg_{
	UINT8 ucMode;
	UINT8 uc8x8to16x16Th;
	UINT8 ucDqpRoiTh;
	INT8  cDqp[6];
}H26XEncMotAqCfg;

typedef struct _H26XEncMotAddrCfg_{
	UINT32 uiMotAddr[3];
	UINT32 uiMotLineOffset;
	UINT8  ucMotBufNum;
    #if H26X_USE_DIFF_MAQ
    UINT32 uiHistMotAddr[H26X_MOTION_BITMAP_NUM+1];
    UINT32 uiMotFrmCnt;
    UINT8  ucCurMotIdx;
    #endif
}H26XEncMotAddrCfg;

typedef struct _H26XEncMdInfoCfg_{
    UINT32 uiMdWidth;
    UINT32 uiMdHeight;
    UINT32 uiMdLofs;
    UINT32 uiMdBufAdr;
    UINT32 uiRot;
}H26XEncMdInfoCfg;

typedef struct _H26XEncJndCfg_{
	BOOL bEnable;

	UINT8 ucStr;
	UINT8 ucLevel;
	UINT8 ucTh;
	UINT8 ucCStr;
	UINT8 ucR5Flag;
	UINT8 ucLsigmaTh;
	UINT8 ucLsigma;
}H26XEncJndCfg;

typedef struct _H26XEncSdeCfg_{
	BOOL bEnable;
	UINT32 uiWidth;
	UINT32 uiHeight;
	UINT32 uiYLofst;
	UINT32 uiCLofst;
}H26XEncSdeCfg;

typedef struct _H26XEncQpRelatedCfg_{
	UINT8 ucSaveDeltaQp;  // 0~15
}H26XEncQpRelatedCfg;

typedef struct _H26XEncNaluLenResult_{
	UINT32 uiSliceNum;
	UINT32 uiVaAddr;
}H26XEncNaluLenResult;

// nt98520 //
typedef struct _H26XEncBgrCfg_{
	BOOL  bEnable;
	INT8  bgr_typ;
	INT16 bgr_th[2];
	INT8  bgr_qp[2];
	INT8  bgr_vt[2];
	INT8  bgr_dq[2];
	INT8  bgr_dth[2];
	INT16 bgr_bth[2];
}H26XEncBgrCfg;

typedef struct _H26XEncRmdCfg_{
	UINT8 ucIraModeBit0;	//264: ucRmdVh4Y, 265: RMD PL Y
	UINT8 ucIraModeBit1;	//264: ucRmdVh8Y, 265: RMD DC Y
	UINT8 ucIraModeBit2;	//264: ucRmdVh16Y, 265: RMD HR Y
	UINT8 ucIraModeBit3;	//264: ucRmdPl16Y, 265: RMD VT Y
	UINT8 ucIraModeBit4;	//264: ucRmdOt4Y, 265: RMD OT Y
	UINT8 ucIraModeBit5;	//264: ucRmdOt8Y
}H26XEncRmdCfg;

typedef struct _H26XEncLambdaCfg_{
    UINT8 adaptlambda_en;
    INT16 lambda_table[52];
    INT16 sqrt_lambda_table[52];
}H26XEncLambdaCfg;

typedef struct _H26XEncTnrCfg_{
	UINT8  nr_3d_mode;						// 0 = nr off, 1 = nr on
	UINT8  tnr_osd_mode;
	UINT8  mctf_p2p_pixel_blending;
	UINT8  tnr_p2p_sad_mode;
	UINT8  tnr_mctf_sad_mode;
	UINT8  tnr_mctf_bias_mode;

	UINT8  nr_3d_adp_th_p2p[3];
	UINT8  nr_3d_adp_weight_p2p[3];
	UINT8  tnr_p2p_border_check_th;
	UINT8  tnr_p2p_border_check_sc;
	UINT8  tnr_p2p_input;
	UINT8  tnr_p2p_input_weight;
	UINT8  cur_p2p_mctf_motion_th;
	UINT8  ref_p2p_mctf_motion_th;
	UINT8  tnr_p2p_mctf_motion_wt[4];

	UINT8  nr3d_temporal_spatial_y[3];
	UINT8  nr3d_temporal_spatial_c[3];
	UINT8  nr3d_temporal_range_y[3];
	UINT8  nr3d_temporal_range_c[3];
	UINT8  nr3d_clampy_th;
	UINT8  nr3d_clampy_div;
	UINT8  nr3d_clampc_th;
	UINT8  nr3d_clampc_div;

	UINT8  nr3d_temporal_spatial_y_mctf[3];
	UINT8  nr3d_temporal_spatial_c_mctf[3];
	UINT8  nr3d_temporal_range_y_mctf[3];
	UINT8  nr3d_temporal_range_c_mctf[3];
	UINT8  nr3d_clampy_th_mctf;
	UINT8  nr3d_clampy_div_mctf;
	UINT8  nr3d_clampc_th_mctf;
	UINT8  nr3d_clampc_div_mctf;

	UINT8  cur_motion_rat_th;
	UINT8  cur_motion_sad_th;
	UINT8  ref_motion_twr_p2p_th[2];
	UINT8  cur_motion_twr_p2p_th[2];
	UINT8  ref_motion_twr_mctf_th[2];
	UINT8  cur_motion_twr_mctf_th[2];
	UINT8  nr3d_temporal_spatial_y_1[3];
	UINT8  nr3d_temporal_spatial_c_1[3];
	UINT8  nr3d_temporal_spatial_y_mctf_1[3];
	UINT8  nr3d_temporal_spatial_c_mctf_1[3];

	UINT8  sad_twr_p2p_th [2];
	UINT8  sad_twr_mctf_th[2];
}H26XEncTnrCfg;

typedef struct _H26XEncEarlySkipCfg_{
	UINT8  ucMDTh;
	UINT8  ucMVTh;
	UINT32 uiSkipCostTh[2];
} H26XEncEarlySkipCfg;

typedef struct _H26XEncSpnCfg_{
	BOOL   bEnable;
	UINT8  ucConEng;
	UINT8  ucSlopConEng;
	UINT8  ucBHC;
	UINT8  ucDHC;
	UINT8  ucEWT;
	UINT8  ucEWG;
	UINT8  ucESS[2];
	UINT8  ucFSS;
	UINT8  ucCT;
	UINT8  ucNL;
	UINT8  ucBIG;
	UINT16 usFlatTh;
	UINT16 usEdgeTh;
	UINT8  ucLowWt;
	UINT8  ucHightWt;
	UINT16 usHLDFlatTh;
	UINT16 usHLDEdgeTh;
	UINT8  ucHLDLowWt;
	UINT8  ucHLDHightWt;
	UINT8  ucEdgeStr;
	UINT8  ucTransitionStr;
	UINT8  ucMotionStr;
	UINT8  ucStaticStr;
	UINT8  ucHLDSlopConEng;
	UINT8  ucFlatStr;
	UINT16 usHLDLumTh;
	UINT8  ucNC[17];
} H26XEncSpnCfg;
typedef struct _H26XEncResultCfg_{
    UINT32 uiBSLen;
    UINT32 uiBSChkSum;
    UINT32 uiRDOCost[2];
	//UINT32 uiQPSum;
	UINT32 uiAvgQP;
	UINT32 uiYPSNR[2];
    UINT32 uiUPSNR[2];
    UINT32 uiVPSNR[2];
	UINT32 uiMOTION_YPSNR[2];
    UINT32 uiMOTION_UPSNR[2];
    UINT32 uiMOTION_VPSNR[2];

    UINT32 uiRecYHitCnt;
    UINT32 uiRecCHitCnt;
    //UINT32 uiInterCnt;
    //UINT32 uiSkipCnt;
    UINT32 uiMotionRatio;

    INT32 iVPSHdrLen;
    INT32 iSPSHdrLen;
    INT32 iPPSHdrLen;

    UINT32 uiInterCnt;
    UINT32 uiSkipCnt;
    UINT32 uiMergeCnt;
    UINT32 uiIntra4Cnt;
    UINT32 uiIntra8Cnt;
    UINT32 uiIntra16Cnt;
    UINT32 uiIntra32Cnt;
    UINT32 uiCU64Cnt;
    UINT32 uiCU32Cnt;
    UINT32 uiCU16Cnt;

	UINT32 uiSvcLable;
	BOOL   bRefLT;
    BOOL   bEVBRStillFlag;
	UINT8  ucNxtPicType;
	UINT8  ucQP;
	UINT8  ucPicType;
	UINT32 uiHwEncTime;
}H26XEncResultCfg;

typedef struct _H26XEncMemInfo_{
	UINT32 uiWidth;
	UINT32 uiHeight;
	UINT32 ucSVCLayer;
	UINT32 uiLTRInterval;
	BOOL   bTileEn;
	BOOL   bD2dEn;
	BOOL   bGdcEn;
	BOOL   bColMvEn;
	UINT8  ucQualityLevel;
	BOOL   bCommReconFrmBuf;
	UINT32  uiLtrSavingBufBsSize;  //0: LTR need to mallocate REC buffer, > 0: LTR Saving Rec Mode, unit is byte
} H26XEncMeminfo;


extern INT32 H26XEnc_setUsrQpCfg(H26XENC_VAR *pVar, H26XEncUsrQpCfg *pUsrQp);
extern INT32 H26XEnc_setPSNRCfg(H26XENC_VAR *pVar, BOOL bEnable);
extern INT32 h26XEnc_setSliceSplitCfg(H26XENC_VAR *pVar, H26XEncSliceSplitCfg *pSliceSplit);
extern INT32 h26XEnc_setVarCfg(H26XENC_VAR *pVar, H26XEncVarCfg *pVarCfg);
extern INT32 h26XEnc_setGdrCfg(H26XENC_VAR *pVar, H26XEncGdrCfg *pGdrCfg);
extern INT32 h26XEnc_setRoiCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncRoiCfg *pRoiCfg);
extern INT32 h26XEnc_setRowRcCfg(H26XENC_VAR *pVar, H26XEncRowRcCfg *pRowRcCfg);
extern INT32 h26XEnc_setAqCfg(H26XENC_VAR *pVar, H26XEncAqCfg *pAqCfg);
extern INT32 h26XEnc_setLpmCfg(H26XENC_VAR *pVar, H26XEncLpmCfg *pLpmCfg);
extern INT32 h26XEnc_setRndCfg(H26XENC_VAR *pVar, H26XEncRndCfg *pRndCfg);
extern INT32 h26XEnc_setScdCfg(H26XENC_VAR *pVar, H26XEncScdCfg *pScdCfg);
extern INT32 h26XEnc_setOsgRgbCfg(H26XENC_VAR *pVar, H26XEncOsgRgbCfg *pOsgRgbCfg);
extern INT32 h26XEnc_setOsgPalCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncOsgPalCfg *pOsgPalCfg);
extern INT32 h26XEnc_setOsgWinCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncOsgWinCfg *pOsgWinCfg);
extern INT32 h26XEnc_setMotAqCfg(H26XENC_VAR *pVar, H26XEncMotAqCfg *pMAqCfg);
//extern INT32 h26XEnc_setMotAddrCfg(H26XENC_VAR *pVar, H26XEncMotAddrCfg *pMotAddrCfg);
extern INT32 h26XEnc_setMdInfoCfg(H26XENC_VAR *pVar, H26XEncMdInfoCfg *pMdInfoCfg);
extern INT32 h26XEnc_setJndCfg(H26XENC_VAR *pVar, H26XEncJndCfg *pJndCfg);
extern INT32 h26XEnc_setQpRelatedCfg(H26XENC_VAR *pVar, H26XEncQpRelatedCfg *pQRCfg);
extern INT32 h26XEnc_setRcInit(H26XENC_VAR *pVar, H26XEncRCParam *pRcParam);
extern void h26XEnc_SetIspRatio(H26XENC_VAR *pVar, H26XEncIspRatioCfg *pIspRatio);
// nt98520 //
extern INT32 h26XEnc_setBgrCfg(H26XENC_VAR *pVar, H26XEncBgrCfg *pBgrCfg);
extern INT32 h26XEnc_setRmdCfg(H26XENC_VAR *pVar, H26XEncRmdCfg *pIraCfg);
extern INT32 h26XEnc_setTnrCfg(H26XENC_VAR *pVar, H26XEncTnrCfg *pTnrCfg);
extern INT32 h26XEnc_setLambdaCfg(H26XENC_VAR *pVar, H26XEncLambdaCfg *pLambdaCfg);
extern INT32 h26XEnc_setIspCbCfg(H26XENC_VAR *pVar, H26XEncIspCbCfg *pIspCbCfg);
extern UINT32 h26xEnc_getVaAPBAddr(H26XENC_VAR *pVar);
extern UINT32 h26xEnc_getVaLLCAddr(H26XENC_VAR *pVar);
extern INT32 h26xEnc_getNaluLenResult(H26XENC_VAR *pVar, H26XEncNaluLenResult *pResult);
extern void h26xEnc_getGdrCfg(H26XENC_VAR *pVar, H26XEncGdrCfg *pGdrCfg);
extern void h26xEnc_getAqCfg(H26XENC_VAR *pVar, H26XEncAqCfg *pAqCfg);
extern UINT32 h26xEnc_getBsLen(H26XENC_VAR *pVar);
extern void h26xEnc_getIspRatioCfg(H26XENC_VAR *pVar, H26XEncIspRatioCfg *pIspRatioCfg);
extern void h26xEnc_getRowRcCfg(H26XENC_VAR *pVar, H26XEncRowRcCfg *pRrcCfg);

#if H26X_SET_PROC_PARAM
extern int h26xEnc_getRCDumpLog(void);
extern int h26xEnc_setRCDumpLog(int value);
#endif
#if JND_DEFAULT_ENABLE
extern int h26xEnc_getJNDParam(H26XEncJndCfg *param);
extern int h26xEnc_setJNDParam(UINT8 enable, UINT8 str, UINT8 level, UINT8 th);
#endif
#if H26X_USE_DIFF_MAQ
extern int h26xEnc_getDiffMAQParam(int *str, int *num, int *morphology, int *idx_1, int *idx_2);
extern int h26xEnc_setDiffMAQParam(int str, int num, int morphology, int idx_1, int idx_2);
extern int h26xEnc_checkDiffMAQEnable(void);
#endif
extern int h26xEnc_getNDQPStep(void);
extern int h26xEnc_setNDQPStep(int value);
extern int h26xEnc_getNDQPRange(void);
extern int h26xEnc_setNDQPRange(int value);

extern int h26xEnc_getVersion(void);
#if H26X_MEM_USAGE
extern UINT32 h26xEnc_getMemUsage(UINT32 type, UINT32 id);
#endif
extern UINT32 h26xEnc_queryRecFrmNum(const H26XEncMeminfo *pInfo);

extern INT32 h26XEnc_setEarlySkipCfg(H26XENC_VAR *pVar, H26XEncEarlySkipCfg *pEarlySkipCfg);
extern INT32 h26XEnc_setSpnCfg(H26XENC_VAR *pVar, H26XEncSpnCfg *pSpnCfg);

#if LPM_PROC_ENABLE
void h26XEnc_getLpmCfg(H26XENC_VAR *pVar, H26XEncLpmCfg *pLpmCfg);
#endif


#endif // _H26XENC_API_H_
