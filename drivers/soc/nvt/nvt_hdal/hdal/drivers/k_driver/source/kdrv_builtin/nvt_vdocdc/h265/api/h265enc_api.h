#ifndef _H265_API_H_
#define _H265_API_H_

//#include "Type.h"
#include "kwrap/type.h"

#include "h26x_def.h"
#include "h265_def.h"
#include "h26xenc_int.h"

#if 0
#define DBG_DUMP printk
#define DBG_ERR  printk
#define DBG_IND  printk
#endif

#define FRO_Y           0
#define FRO_C           1

#define FRO_IntraI      0
#define FRO_IntraP      1
#define FRO_InterP      2

#define FRO_Y32         0
#define FRO_Y16         1
#define FRO_Y8          2
#define FRO_Y4          3

#define FRO_C16         0
#define FRO_C8          1
#define FRO_C4          2


typedef struct _H265ENC_INIT_{
	// resolution and lineoffset //
    UINT32          uiWidth;            ///< Original source width, ex: 1920 x 1080, width = 1920
    UINT32          uiHeight;           ///< Original source height, ex: 1920 x 1080, height = 1080 (not 1088)
    UINT32          uiDisplayWidth;     ///< Display source width, ex: 720 x 480, uiWidth = 736, uiDisplayWidth = 720
    //UINT32			uiRecLineOffset;	/// < reconstruct frame line offset //
    BOOL            bTileEn;
	HEVC_QLVL_TYPE  eQualityLvl;		///< Quality level for setting search range and tile config

    UINT32          uiEncBufAddr;       ///< H.265 encoder internal buffer starting address
    UINT32          uiEncBufSize;       ///< H.264 encoder internal buffer size

	// Codec Settings //
    UINT32          uiGopNum;   		///< H.265 encoder GOP frame number
	UINT32          uiUsrQpSize;        // 0 : 16x16, 1 : 32x32, 2 : 64x64

	// Deblocking //
	UINT8           ucDisableDB;        ///< H.265 disable_filter_idc (0=Filter, 1=No Filter, 2=Slice Mode)
    INT8            cDBAlpha;           ///< H.265 deblocking Alpha & C0 offset div. 2, {-6, -5, ... 0, +1, .. +6}
    INT8            cDBBeta;            ///< H.265 deblocking Beta offset div. 2, {-6, -5, ... 0, +1, .. +6}

	// NT96510 //
	INT32           iQpCbOffset;        // -12 ~ 12
	INT32           iQpCrOffset;        // -12 ~ 12

	// SVC //
    UINT8           ucSVCLayer;         ///< H.265 SVC Layer, range : 0 , 1, 2

	// Long-term //
	UINT32          uiLTRInterval;      /// < H.265 long-term reference frame interval. range : 0 ~ 4095
	UINT32          uiLTRPreRef;        /// < H.265 long-term reference setting. 0 : all long-term reference to IDR frame, 1: reference latest long-term reference frame.
	// SAO //
	UINT32          uiSAO;              // Sample Adaptive Offset : 0 : enable, 1 : disable
	UINT32          uiSaoLumaFlag;      // SAO Luma Flag
	UINT32          uiSaoChromaFlag;    // SAO Chroma Flag

	// rate control //
	UINT32          uiBitRate;          ///< H.265 Bit rate (bits per second)
    UINT32          uiFrmRate;        	///< H.265 frame rate (frames per second * 1000)

    UINT8           ucIQP;              ///< H.265 Rate Control's Init IQP
    UINT8           ucPQP;              ///< H.265 Rate Control's Init PQP
    UINT8           ucMaxIQp;			///< H.265 Rate Control's Max I Qp
    UINT8           ucMinIQp;			///< H.265 Rate Control's Min I Qp
    UINT8           ucMaxPQp;			///< H.265 Rate Control's Max P Qp
    UINT8           ucMinPQp;			///< H.265 Rate Control's Min P Qp
    INT32           iIPQPoffset;		///< H.265 Rate Control's I P QP offset (not ready)
    UINT32			uiStaticTime;		///< H.265 rate control's static time, unit: second

	// function //
	BOOL			bFBCEn;				/// < H.265 reconstruct frame buffer compression
	BOOL			bGrayEn;			/// < H.265 encode color to gray
	BOOL			bFastSearchEn;		/// < H.265 fast search range select : 0 : small range, 1 : big range
	BOOL			bHwPaddingEn;		/// < H.265 encode hw padding
	UINT8			ucRotate;			/// < H.265 rotate source image : 0 : no rotate , 1 : counterclockwise rotate , 2 : clockwise rotate (NOTE : shall be set to 0 while enable source compression)
	BOOL            bD2dEn;             /// < H.265 d2d control (0: disable, 1: enable)
	BOOL            bGdcEn;             /// < H.265 gdc control (0: disable, 1: enable)
	BOOL            bColMvEn;           /// < H.265 col mv control (0: disable, 1: enable)

	// SEI //
	UINT8           ucSEIIdfEn;
	UINT32          uiMultiTLayer; 		/// < H.265 Use Multi Temporal Layer : 0 : disable, 1: enable
	UINT8			ucLevelIdc;			/// < H.265 level idc
    BOOL			bVUIEn;				/// < H.265 encode vui header
    UINT16          usSarWidth;         ///< Sample Aspect Ratio width, ex: 1920 x 1080, width = 1920
    UINT16          usSarHeight;        ///< Sample Aspect Ratio height, ex: 1920 x 1080, height = 1080 (not 1088)
    UINT8           ucMatrixCoef;       ///< Matrix coefficients are used to derive the luma and Chroma signals from green, blue, and red primaries. default: 2, range: 0~255
    UINT8           ucTransferCharacteristics;  ///< The opto-electronic transfers characteristic of the source pictures. default: 2, range: 0~255
    UINT8           ucColourPrimaries;  ///< Chromaticity coordinates the source primaries. default: 2, range: 0~255
    UINT8           ucVideoFormat;      ///< Indicate the representation of pictures. default: 5, range: 0~7
    UINT8           ucColorRange;       ///< Indicate the black level and range of the luma and Chroma signals. default: 0, range: 0~1 (0: Not full range, 1: Full range)
    UINT8           bTimeingPresentFlag;   ///< timing info present flag. default: 0, range: 0~1 (0: disable, 1: enable)
	BOOL			bRecBufComm;		///< use common buffer of reconstruct frame buffer.
	UINT32			uiRecBufAddr[3];	///< common buffer of reconstruct frame buffer address.
	UINT32          uiLtrSavingBufBsSize; ///< 0: LTR need to mallocate REC buffer, > 0: LTR Saving Rec Mode, unit is byte
} H265ENC_INIT;

typedef struct _H265ENC_INFO_{
	SLICE_TYPE	ePicType;				///< Encode frame type (I, Idr, P, or B)

	UINT32		uiSrcYAddr;				///< Source Y address
	UINT32		uiSrcCAddr;				///< Source UV address
	UINT32		uiSrcYLineOffset;		///< Y line offset
	UINT32		uiSrcCLineOffset;		///< UV line offset
	BOOL		bSrcCbCrIv;				///< Source CbCr interleave ( 0 : CbCr, 1 : CrCb )
	BOOL		bSrcOutEn;
	UINT8		ucSrcOutMode;
	UINT32		uiSrcOutYAddr;			///< Source Out (3DNR) Y address
	UINT32		uiSrcOutCAddr;			///< Source Out (3DNR) UV address
	UINT32		uiSrcOutYLineOffset;
	UINT32		uiSrcOutCLineOffset;
    BOOL        bSrcD2DEn;              ///< IME direct enable/disable flag
    UINT8       ucSrcD2DMode;           ///< IME direct mode
    UINT32		uiSrcD2DStrpSize[3];	/// < IME direct stripe size
	UINT32		uiBsOutBufAddr;		    ///< Encoded bit-stream Phy address
	UINT32		uiBsOutBufSize;			///< Encoded bit-stream size
	UINT32		uiSrcTimeStamp;
	UINT32		uiTemporalId;
	UINT32		uiNalLenOutAddr;
	BOOL		bSkipFrmEn;
    UINT8       ucSkipFrmMode;          ///< 0: fast, 1: normal speed
	BOOL		bGetSeqHdrEn;			///< get sps and pps from uiBsOutBufAddr

    UINT32      uiPredBitss;
    UINT32      uiH26XBaseVa;
	H26XEncSdeCfg SdeCfg;
} H265ENC_INFO;

typedef struct _H265EncRdoCfg_{
	INT8  global_motion_penalty_I32;
	INT8  global_motion_penalty_I16;
	INT8  global_motion_penalty_I08;

	INT8  cost_bias_skip;
	INT8  cost_bias_merge;

	UINT8 ime_scale_64c;
	UINT8 ime_scale_64p;
	UINT8 ime_scale_32c;
	UINT8 ime_scale_32p;
	UINT8 ime_scale_16c;
} H265EncRdoCfg;

typedef struct _H265EncRdo_{
	BOOL  bEnable;

	H265EncRdoCfg stRdoCfg;

	INT8  rate_bias_I_32L;
	INT8  rate_bias_I_16L;
	INT8  rate_bias_I_08L;
	INT8  rate_bias_I_04L;
	INT8  rate_bias_I_16C;
	INT8  rate_bias_I_08C;

	INT8  rate_bias_I_04C;
	INT8  rate_bias_IP_16L;
	INT8  rate_bias_IP_08L;
	INT8  rate_bias_IP_04L;
	INT8  rate_bias_IP_08C;
	INT8  rate_bias_IP_04C;

	INT8  rate_bias_P_32L;
	INT8  rate_bias_P_16L;
	INT8  rate_bias_P_08L;
	INT8  rate_bias_P_16C;
	INT8  rate_bias_P_08C;
	INT8  rate_bias_P_04C;

	INT8  global_motion_penalty_I32O;
	INT8  global_motion_penalty_I16O;
	INT8  global_motion_penalty_I08O;
} H265EncRdo;

typedef struct _H265EncFroCfg_{
	BOOL bEnable;
	UINT32 uiDC[2][3][4];
	UINT8  ucAC[2][3][4];
	UINT8  ucST[2][3][4];
	UINT8  ucMX[2][3][4];
}H265EncFroCfg;
/*
typedef struct _H265EncResultCfg_{
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
    UINT32 uiInterCnt;
    UINT32 uiSkipCnt;
    UINT32 uiMotionCnt;
}H265EncResultCfg;
*/

typedef struct _H265EncQpRelatedCfg_{
	UINT8 ucMaxCuDQPDepth; // 0~3
	UINT8 ucImeCoherentQP; // 0~3
	UINT8 ucRdoCoherentQP; // 0~3
	UINT8 ucQpMergeMethod; // 0~3
}H265EncQpRelatedCfg;

typedef struct _H265EncTileCfg_{
    BOOL bTileEn;
    UINT8 ucTileNum;
    UINT32 uiTileWidth[H26X_MAX_TILE_NUM];
}H265EncTileCfg;

extern INT32 h265Enc_initEncoder(H265ENC_INIT *pInit, H26XENC_VAR *pVar);
extern INT32 h265Enc_prepareOnePicture(H265ENC_INFO *pInfo, H26XENC_VAR *pVar);
extern INT32 h265Enc_getResult(H26XENC_VAR *pVar, unsigned int enc_mode, H26XEncResultCfg *pResult, UINT32 uiHwInt);

extern INT32 h265Enc_InitRdoCfg(H26XENC_VAR *pVar, H265EncRdo *pRdo);
extern INT32 h265Enc_setRdoCfg(H26XENC_VAR *pVar, H265EncRdoCfg *pRdo);
extern INT32 h265Enc_setFroCfg(H26XENC_VAR *pVar, H265EncFroCfg *pFro);
extern INT32 h265Enc_setSliceSplit(H26XENC_VAR *pVar, H26XEncSliceSplit *pSliceSplit, BOOL bTileEn);
extern void h265Enc_setPicQP(UINT8 ucQP, H26XENC_VAR *pVar);
extern INT32 h265Enc_setQpRelatedCfg(H26XENC_VAR *pVar, H265EncQpRelatedCfg *pQR);
extern void h265Enc_setGopNum(H26XENC_VAR *pVar, UINT32 gop);

extern UINT32 h265Enc_getEncodeRatio( H26XENC_VAR *pVar);
extern UINT32 h265Enc_getSVCLabel(H26XENC_VAR *pVar);
extern UINT32 h265Enc_queryMemSize(H26XEncMeminfo *pInfo);
extern void h265Enc_getSeqHdr(H26XENC_VAR *pVar, UINT32 *pAddr, UINT32 *pSize);
extern UINT32 h265Enc_getNxtPicType(H26XENC_VAR *pVar);
extern void h265Enc_getRdoCfg(H26XENC_VAR *pVar, H265EncRdoCfg *pRdo);
extern UINT32 h265Enc_queryRecFrmSize(const H26XEncMeminfo *pInfo);
#if H26X_SET_PROC_PARAM
extern int h265Enc_getRowRCStopFactor(void);
extern int h265Enc_setRowRCStopFactor(int value);
#endif
extern int h265Enc_setLongStartCode(UINT32 lsc);

#endif

