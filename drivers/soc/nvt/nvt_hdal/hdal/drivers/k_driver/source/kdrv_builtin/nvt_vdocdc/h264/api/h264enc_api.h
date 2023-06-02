#ifndef _H264_API_H_
#define _H264_API_H_

#include "kwrap/type.h"

#include "h26x_def.h"
#include "h26xenc_api.h"

#include "h264_def.h"

typedef struct _H264ENC_INIT_{
	// resolution and lineoffset //
    UINT32          uiWidth;            ///< Original source width, ex: 1920 x 1080, width = 1920
    UINT32          uiHeight;           ///< Original source height, ex: 1920 x 1080, height = 1080 (not 1088)
    UINT32          uiDisplayWidth;     ///< Display source width, ex: 720 x 480, uiWidth = 736, uiDisplayWidth = 720

	// Bitstream Buffer  //
    UINT32          uiEncBufAddr;       ///< H.264 encoder internal buffer starting address
    UINT32          uiEncBufSize;       ///< H.264 encoder internal buffer size

	// Codec Settings //
    UINT32          uiGopNum;   		///< H.264 encoder GOP frame number

	// Deblocking //
    UINT8           ucDisLFIdc;        ///< H.264 disable_filter_idc (0=Filter, 1=No Filter, 2=Slice Mode)
    INT8            cDBAlpha;           ///< H.264 deblocking Alpha & C0 offset div. 2, {-6, -5, ... 0, +1, .. +6}
    INT8            cDBBeta;            ///< H.264 deblocking Beta offset div. 2, {-6, -5, ... 0, +1, .. +6}

	// chorma qp offset //
	INT8			cChrmQPIdx;			///<H.264 chroma QP index offset  (-12 ~ 12)
	INT8			cSecChrmQPIdx;		///<H.264 second chroma QP index offset (-12 ~ 12), only ??

	// SVC //
    UINT8           ucSVCLayer;         ///< H.264 SVC Layer, range : 0 , 1, 2

	// Long-term //
	UINT32          uiLTRInterval;		///< H.264 long-term reference frame interval. range : 0 ~ 4095
	BOOL            bLTRPreRef;			///< H.264 long-term reference setting. 0 : all long-term reference to IDR frame, 1: reference latest long-term reference frame.

	// rate control //
	UINT32          uiBitRate;          ///< H.264 Bit rate (bits per second)
    UINT32          uiFrmRate;        	///< H.264 frame rate (frames per second * 1000)

    UINT8           ucIQP;              ///< H.264 Rate Control's Init IQP
    UINT8           ucPQP;              ///< H.264 Rate Control's Init PQP
    UINT8           ucMaxIQp;			///< H.264 Rate Control's Max I Qp
    UINT8           ucMinIQp;			///< H.264 Rate Control's Min I Qp
    UINT8           ucMaxPQp;			///< H.264 Rate Control's Max P Qp
    UINT8           ucMinPQp;			///< H.264 Rate Control's Min P Qp
    INT32           iIPWeight;			///< H.264 Rate Control's I P weight
    UINT32			uiStaticTime;		///< H.264 rate control's static time, unit: second

	// function //
	BOOL			bFBCEn;				/// < H.264 reconstruct frame buffer compression
	BOOL			bGrayEn;			/// < H.264 encode color to gray
	BOOL			bFastSearchEn;		/// < H.264 encode fast search
	BOOL			bHwPaddingEn;		/// < H.264 encode hw padding
	UINT8			ucRotate;			/// < H.264 rotate source image : 0 : no rotate , 1 : counterclockwise rotate , 2 : clockwise rotate (NOTE : shall be set to 0 while enable source compression)
	BOOL			bD2dEn;				/// < H.264 d2d control (0: disable, 1: enable)
	BOOL			bColMvEn;			/// < H.264 col mv control (0: disable, 1: enable)

	// H264 ONLY //
	H264_PROFILE    eProfile;      		///< H.264 encoder profile (baseline, main, high profile)
	H264_ENTROPY 	eEntropyMode;		/// < H.264 entropy coding mode
	UINT8 			ucLevelIdc;			/// < H.264 encode level idc
    BOOL			bTrans8x8En;		/// < H.264 ucTrans8x8
    BOOL			bForwardRecChrmEn;	/// < H.264  reconstructed chroma value for H264 left reference in intra RMD enable flag

	// VUI header //
    BOOL			bVUIEn;				/// < H.264 encode vui header
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
} H264ENC_INIT;

typedef struct _H264ENC_INFO_{
	SLICE_TYPE	ePicType;				///< Encode frame type (I, Idr, P, or B)

	UINT32		uiSrcYAddr;				///< Source Y address
	UINT32		uiSrcCAddr;				///< Source UV address
	UINT32		uiSrcYLineOffset;		///< Y line offset
	UINT32		uiSrcCLineOffset;		///< UV line offset
	BOOL		bSrcCbCrIv;				/// < Source CbCr interleave ( 0 : CbCr, 1 : CrCb )
	BOOL		bSrcOutEn;
	UINT8		ucSrcOutMode;
	UINT32		uiSrcOutYAddr;			///< Source Out (3DNR) Y address
	UINT32		uiSrcOutCAddr;			///< Source Out (3DNR) UV address
	UINT32		uiSrcOutYLineOffset;
	UINT32		uiSrcOutCLineOffset;
	BOOL        bSrcD2DEn;              ///< IME direct enable/disable flag
	UINT8       ucSrcD2DMode;           ///< IME direct mode
	UINT32		uiSrcD2DStrpSize[3];	/// < IME direct stripe size
	UINT32		uiBsOutBufAddr;			///< Encoded bit-stream address
	UINT32		uiBsOutBufSize;			///< Encoded bit-stream size
	UINT32		uiNalLenOutAddr;
	UINT32		uiSrcTimeStamp;
	UINT32		uiTemporalId;
	BOOL		bSkipFrmEn;
	BOOL		bGetSeqHdrEn;			/// < get sps and pps from uiBsOutBufAddr
	H26XEncSdeCfg SdeCfg;
} H264ENC_INFO;

typedef struct _H264EncRdoCfg_{
	BOOL  bEnable;

	UINT8 ucI_Y4_CB_P;
	UINT8 ucI_Y8_CB_P;
	UINT8 ucI_Y16_CB_P;

	UINT8 ucP_Y4_CB;
	UINT8 ucP_Y8_CB;

	UINT8 ucIP_CB_SKIP;
} H264EncRdoCfg;

typedef struct _H264EncRdo_{
	BOOL  bEnable;

	H264EncRdoCfg stRdoCfg;

	UINT8 ucSlope[3][4];

	UINT8 ucI_Y4_CB;
	UINT8 ucI_Y8_CB;
	UINT8 ucI_Y16_CB;
	UINT8 ucI16_CT_DC;
	UINT8 ucI16_CT_H;
	UINT8 ucI16_CT_V;
	UINT8 ucI16_CT_P;

	UINT8 ucIP_C_CT_DC;
	UINT8 ucIP_C_CT_H;
	UINT8 ucIP_C_CT_V;
	UINT8 ucIP_C_CT_P;

	UINT8 ucP_Y_COEFF_COST_TH;
	UINT8 ucP_C_COEFF_COST_TH;
	UINT8 ucIP_CB_SKIP_BG;
	UINT8 ucIP_CB_SHIFT;
} H264EncRdo;

typedef struct _H264EncFroCfg_{
	BOOL bEnable;

	UINT32 uiDC[3][4];
	UINT8  ucAC[3][4];
	UINT8  ucST[3][4];
	UINT8  ucMX[3][4];
}H264EncFroCfg;


extern INT32 h264Enc_initEncoder(H264ENC_INIT *pInit, H26XENC_VAR *pVar);
#if 0
extern INT32 h264Enc_getH264Header(UINT32 *puiAddr, UINT32 *puiLen, H264ENC_VAR *pH264EncVar);
#endif
extern INT32 h264Enc_prepareOnePicture(H264ENC_INFO *pInfo, H26XENC_VAR *pVar);
extern INT32 h264Enc_getResult(H26XENC_VAR *pVar, unsigned int enc_mode, H26XEncResultCfg *pResult, UINT32 uiHwInt);

extern INT32 h264Enc_InitRdoCfg(H26XENC_VAR *pVar, H264EncRdo *pRdo);
extern INT32 h264Enc_setRdoCfg(H26XENC_VAR *pVar, H264EncRdoCfg *pRdo);
extern INT32 h264Enc_setFroCfg(H26XENC_VAR *pVar, H264EncFroCfg *pFro);

extern UINT32 h264Enc_queryMemSize(H26XEncMeminfo *pInfo);

extern void h264Enc_getSeqHdr(H26XENC_VAR *pVar, UINT32 *pAddr, UINT32 *pSize);
extern UINT32 h264Enc_getGopNum(H26XENC_VAR *pVar);
extern UINT32 h264Enc_getNxtPicType(H26XENC_VAR *pVar);
extern UINT32 h264Enc_getSVCLabel(H26XENC_VAR *pVar);
extern UINT32 h264Enc_getEncodeRatio(H26XENC_VAR *pVar);
extern void h264Enc_getRdoCfg(H26XENC_VAR *pVar, H264EncRdoCfg *pRdo);
extern void h264Enc_setGopNum(H26XENC_VAR *pVar, UINT32 gop);
extern UINT32 h264Enc_queryRecFrmSize(const H26XEncMeminfo *pInfo);

#if H26X_SET_PROC_PARAM
extern int h264Enc_getRowRCStopFactor(void);
extern int h264Enc_setRowRCStopFactor(int value);
extern int h264Enc_getPReduce16Planar(void);
extern int h264Enc_setPReduce16Planar(int value);
#endif

extern int h264Enc_setFrameNumGapAllow(int value);
extern int h264Enc_setFixSPSLog2Poc(int value);
// emulation only //
extern void h264Enc_setPicQP(UINT8 ucQP, H26XENC_VAR *pVar);
#endif

