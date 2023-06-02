#ifndef _H26XENC_API_H_
#define _H26XENC_API_H_

#include "kwrap/type.h"
#include "h26x_def.h"
#include "h26xenc_rc.h"

#define H26X_AQ_TH_TBL_SIZE	(30)

typedef struct _H26XEncFroCfg_{	
	BOOL bEnable;
}H26XEncFroCfg;

typedef struct _H26XEncUsrQpCfg_{
	BOOL bEnable;

	UINT32 uiQpMapAddr;
	UINT32 uiQpMapLineOffset;
}H26XEncUsrQpCfg;

typedef struct _H26XEncSliceSplitCfg_ {
	UINT32 uiEnable;        // enable multiple slice , range : 0 ~ 1 //
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

typedef struct _H26XEncMaskInitCfg_{
	UINT8 ucMosaicBlkW;	// keep latest set mosaic blk w //
	UINT8 ucMosaicBlkH;	// keep latest set mosaci blk h //

	UINT8  ucPalY[8];
	UINT8  ucPalCb[8];
	UINT8  ucPalCr[8];
}H26XEncMaskInitCfg;

typedef struct _H26XEncMaskLineCfg_{
	UINT32 uiCoeffa;
	UINT32 uiCoeffb;
	UINT32 uiCoeffc;
	UINT8  ucComp;
}H26XEncMaskLineCfg;

typedef struct _H26XEncMaskWinCfg_{		
	BOOL bEnable;			

	UINT8  ucDid;
	UINT8  ucPalSel;
	UINT8  ucLineHitOpt;
	UINT16 usAlpha;	

	H26XEncMaskLineCfg stLine[4];
}H26XEncMaskWinCfg;

typedef struct _H26XEncGdrCfg_{
	BOOL bEnable;

	UINT32 usPeriod;
	UINT32 usNumber;
	UINT8  ucGdrQpEn;
	UINT8  ucGdrQp;

}H26XEncGdrCfg;

typedef struct _H26XEncRoiCfg_{
	BOOL bEnable;

	UINT16 usCoord_X;
	UINT16 usCoord_Y;
	UINT16 usWidth;
	UINT16 usHeight;
	UINT8  cQP;
	UINT8  ucMode;
}H26XEncRoiCfg;

typedef struct _H26XEncRowRCCfg_{
	BOOL bEnable;

	UINT8 usIPredWt;
	UINT8 usPPredWt;
	UINT8 usScale;
	UINT8 usQPRange;
	UINT8 usQPStep;
	UINT8 usMinQP;
	UINT8 usMaxQP;
    UINT8 usRRCMode; //0:original, 1: new
}H26XEncRowRcCfg;

typedef struct _H26XEncAqCfg_{
	BOOL bEnable;

	UINT8  ucIC2;
	UINT8  ucIStr;
	UINT8  ucPStr;
	UINT8  ucMaxDQp;
	UINT8  ucMinDQp;
	UINT8  ucAslog2;
	UINT8  ucDepth;
	UINT8  ucPlaneX;
	UINT8  ucPlaneY;
	INT16  sTh[H26X_AQ_TH_TBL_SIZE];
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

typedef struct _H26XEncMotionAqCfg_{
	UINT8 ucMode;
	UINT8 uc8x8to16x16Th;
	UINT8 ucDqpRoiTh;
	INT8  cDqp[6];
	UINT8 ucDqpnum;
    UINT8 ucDqpMotTh;
}H26XEncMotionAqCfg;

typedef struct _H26XEncJndCfg_{
	BOOL bEnable;

	UINT8 ucStr;
	UINT8 ucLevel;
	UINT8 ucTh;

    UINT8 ucCStr;
    UINT8 ucR5Flag;
    UINT8 ucBilaFlag;
    UINT8 ucLsigmaTh;
    UINT8 ucLsigma;
}H26XEncJndCfg;

typedef struct _H26XEncSdeCfg_{
	UINT32 uicfg[45];
}H26XEncSdeCfg;

typedef struct _H26XEncQpRelatedCfg_{
	UINT8 ucSaveDeltaQp;  // 0~15
}H26XEncQpRelatedCfg;

typedef struct _H26XEncNaluLenResult_{
	UINT32 uiSliceNum;
	UINT32 uiVaAddr;
}H26XEncNaluLenResult;

typedef struct _H26XEncBgrCfg_{
	BOOL bEnable;
	INT8 bgr_typ;
    INT16 bgr_th[2];
    INT8  bgr_qp[2];
    INT8  bgr_vt[2];
    INT8  bgr_dq[2];
    INT8  bgr_dth[2];
    INT16  bgr_bth[2];
}H26XEncBgrCfg;

typedef struct _H26XEncRmdCfg_{
	UINT8    ucRmdVh4Y;
	UINT8    ucRmdVh8Y;
	UINT8    ucRmdVh16Y;
	UINT8    ucRmdPl16Y;
	UINT8    ucRmdOt4Y;
	UINT8    ucRmdOt8Y;
}H26XEncRmdCfg;

typedef struct _H26XEncLambdaCfg_{
    UINT8 adaptlambda_en;
    INT16 lambda_table[52];
    INT16 sqrt_lambda_table[52];
}H26XEncLambdaCfg;

typedef struct _H26XEncTnrCfg_{
	INT8  nr_3d_mode;						// 0 = nr off, 1 = nr on
	INT8  tnr_osd_mode;
	INT8  mctf_p2p_pixel_blending;
	INT8  tnr_p2p_sad_mode;
	INT8  tnr_mctf_sad_mode;
	INT8  tnr_mctf_bias_mode;

	INT8  nr_3d_adp_th_p2p[3];
	INT8  nr_3d_adp_weight_p2p[3];
	INT8  tnr_p2p_border_check_th;
	INT8  tnr_p2p_border_check_sc;
	INT8  tnr_p2p_input;
	INT8  tnr_p2p_input_weight;
	INT8  cur_p2p_mctf_motion_th;
	INT8  ref_p2p_mctf_motion_th;
	INT8  tnr_p2p_mctf_motion_wt[4];

	INT8  nr3d_temporal_spatial_y[3];
	INT8  nr3d_temporal_spatial_c[3];
	INT8  nr3d_temporal_range_y[3];
	INT8  nr3d_temporal_range_c[3];
	INT8  nr3d_clampy_th;
	INT8  nr3d_clampy_div;
	INT8  nr3d_clampc_th;
	INT8  nr3d_clampc_div;

	INT8  nr3d_temporal_spatial_y_mctf[3];
	INT8  nr3d_temporal_spatial_c_mctf[3];
	INT8  nr3d_temporal_range_y_mctf[3];
	INT8  nr3d_temporal_range_c_mctf[3];
	INT8  nr3d_clampy_th_mctf;
	INT8  nr3d_clampy_div_mctf;
	INT8  nr3d_clampc_th_mctf;
	INT8  nr3d_clampc_div_mctf;

    INT8  cur_motion_rat_th;
    INT8  cur_motion_sad_th;
    INT8  ref_motion_twr_p2p_th[2];
    INT8  cur_motion_twr_p2p_th[2];
    INT8  ref_motion_twr_mctf_th[2];
    INT8  cur_motion_twr_mctf_th[2];
    INT8  nr3d_temporal_spatial_y_1[3];
    INT8  nr3d_temporal_spatial_c_1[3];
    INT8  nr3d_temporal_spatial_y_mctf_1[3];
    INT8  nr3d_temporal_spatial_c_mctf_1[3];

    INT8  sad_twr_p2p_th [2];
    INT8  sad_twr_mctf_th[2];

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
} H26XEncMeminfo;

extern INT32 H26XEnc_setUsrQpCfg(H26XENC_VAR *pVar, H26XEncUsrQpCfg *pUsrQp);
extern INT32 H26XEnc_setPSNRCfg(H26XENC_VAR *pVar, BOOL bEnable);
extern INT32 h26XEnc_setSliceSplitCfg(H26XENC_VAR *pVar, H26XEncSliceSplitCfg *pSliceSplit);
extern INT32 h26XEnc_setVarCfg(H26XENC_VAR *pVar, H26XEncVarCfg *pVarCfg);
extern INT32 h26xEnc_setMaskInitCfg(H26XENC_VAR *pVar, H26XEncMaskInitCfg *pMaskInitCfg);
extern INT32 h26XEnc_setMaskWinCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncMaskWinCfg *pMaskWinCfg);
extern INT32 h26XEnc_setGdrCfg(H26XENC_VAR *pVar, H26XEncGdrCfg *pGdrCfg);
extern INT32 h26XEnc_setRoiCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncRoiCfg *pRoiCfg);
extern INT32 h26XEnc_setRowRcCfg(H26XENC_VAR *pVar, H26XEncRowRcCfg *pRowRcCfg);
extern INT32 h26XEnc_setAqCfg(H26XENC_VAR *pVar, H26XEncAqCfg *pAqCfg);
extern INT32 h26XEnc_setLpmCfg(H26XENC_VAR *pVar, H26XEncLpmCfg *pLpmCfg);
extern INT32 h26XEnc_setRndCfg(H26XENC_VAR *pVar, H26XEncRndCfg *pRndCfg);
extern INT32 h26XEnc_setScdCfg(H26XENC_VAR *pVar, H26XEncScdCfg *pScdCfg);
extern INT32 h26XEnc_setOsgRgbCfg(H26XENC_VAR *pVar, H26XEncOsgRgbCfg *pOsgRgbCfg);
extern INT32 h26XEnc_setOsgPalCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncOsgPalCfg *pOsgPalCfg);
extern INT32 h26XEnc_setOsgChromaAlphaCfg(H26XENC_VAR *pVar, UINT8 ucVal);
extern INT32 h26XEnc_setOsgWinCfg(H26XENC_VAR *pVar, UINT8 ucIdx, H26XEncOsgWinCfg *pOsgWinCfg);
extern INT32 h26XEnc_setMotionAqInitCfg(H26XENC_VAR *pVar);
extern INT32 h26XEnc_setMotionAqCfg(H26XENC_VAR *pVar, H26XEncMotionAqCfg *pMAqCfg);
extern INT32 h26XEnc_setJndCfg(H26XENC_VAR *pVar, H26XEncJndCfg *pJndCfg);
extern INT32 h26XEnc_setSdecCfg(H26XENC_VAR *pVar, H26XEncSdeCfg *pSdecCfg);
extern INT32 h26XEnc_setQpRelatedCfg(H26XENC_VAR *pVar, H26XEncQpRelatedCfg *pQRCfg);
extern INT32 h26XEnc_setRcInit(H26XENC_VAR *pVar, H26XEncRCParam *pRcParam);
extern UINT32 h26xEnc_getVaAPBAddr(H26XENC_VAR *pVar);
extern UINT32 h26xEnc_getVaLLCAddr(H26XENC_VAR *pVar);
extern void h26xEnc_getNaluLenResult(H26XENC_VAR *pVar, H26XEncNaluLenResult *pResult);
extern void h26xEnc_getGdrCfg(H26XENC_VAR *pVar, H26XEncGdrCfg *pGdrCfg);
extern void h26xEnc_getAqCfg(H26XENC_VAR *pVar, H26XEncAqCfg *pAqCfg);
extern UINT32 h26xEnc_getBsLen(H26XENC_VAR *pVar);
extern UINT32 h26XEnc_getUsrQpAddr(H26XENC_VAR *pVar);
extern UINT32 h26xEnc_getMaqMotAddr(H26XENC_VAR *pVar, UINT32 i);
extern UINT32 h26xEnc_getMaqMotSize(H26XENC_VAR *pVar);
extern INT32 h26XEnc_setBgrCfg(H26XENC_VAR *pVar, H26XEncBgrCfg *pBgrCfg);
extern INT32 h26XEnc_setRmdCfg(H26XENC_VAR *pVar, H26XEncRmdCfg *pIraCfg);
extern INT32 h26XEnc_setTnrCfg(H26XENC_VAR *pVar, H26XEncTnrCfg *pTnrCfg);
extern INT32 h26XEnc_setLambdaCfg(H26XENC_VAR *pVar, H26XEncLambdaCfg *pLambdaCfg);
extern void h26XEnc_setUsrQpAddr(H26XENC_VAR *pVar, UINT32 uiAddr);
extern INT32 h26XEnc_setEarlySkipCfg(H26XENC_VAR *pVar, H26XEncEarlySkipCfg *pEarlySkipCfg);
extern INT32 h26XEnc_setSpnCfg(H26XENC_VAR *pVar, H26XEncSpnCfg *pSpnCfg);

#endif // _H26XENC_API_H_
