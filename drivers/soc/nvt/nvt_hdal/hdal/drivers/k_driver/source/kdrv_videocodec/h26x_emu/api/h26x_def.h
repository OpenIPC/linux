#ifndef _H26X_DEF_H_
#define _H26X_DEF_H_

#include "kwrap/type.h"

#define H26X_MAX_WIDTH		(4096)
#define H26X_MAX_HEIGHT		(4096)
#define H26X_MIN_MB			(16)
//#define H26X_MAX_BSDMA_NUM	(H26X_MAX_HEIGHT/H26X_MIN_MB)
#define H26X_MAX_BSDMA_NUM 0x2000
#define H26X_MAX_ROI_W		(10)
//#define H26X_ENC_MODE		(1)	// 0 : link-list , 1 : direct mode
#define H264_P_REDUCE_16_PLANAR (0)
#define USE_JND_DEFAULT_VALUE   (0)

#define H26XENC_BS_OUTBUF_MAX_SIZE 0x800000
#define H26X_MAX_TILE_NUM	    (5)
#define H26X_FIRST_TILE_MIN_W   (576)
#define H26X_MIDDLE_TILE_MIN_W  (576)
#define H26X_LAST_TILE_MIN_W    (448)
#define H26X_FIRST_TILE_MAX_W_SR36   (1024)
#define H26X_MIDDLE_TILE_MAX_W_SR36  (896)
#define H26X_LAST_TILE_MAX_W_SR36    (1024)
#define H26X_FIRST_TILE_MAX_W_SR28   (1408)
#define H26X_MIDDLE_TILE_MAX_W_SR28  (1280)
#define H26X_LAST_TILE_MAX_W_SR28    (1408)
#define H26X_SR36_MAX_W              (4736) //H26X_FIRST_TILE_MAX_W_SR36+H26X_MIDDLE_TILE_MAX_W_SR36*3+H26X_LAST_TILE_MAX_W_SR36
#define H26X_SR28_MAX_W              (6656) //H26X_FIRST_TILE_MAX_W_SR28+H26X_MIDDLE_TILE_MAX_W_SR28*3+H26X_LAST_TILE_MAX_W_SR28

#define SIZE_256X(a)			((((a) + 255)>>8)<<8)
#define SIZE_128X(a)			((((a) + 127)>>7)<<7)
#define SIZE_64X(a)				((((a) + 63)>>6)<<6)
#define SIZE_32X(a)				((((a) + 31)>>5)<<5)
#define SIZE_16X(a)				((((a) + 15)>>4)<<4)
#define SIZE_8X(a)				((((a) + 7)>>3)<<3)
#define SIZE_4X(a)				((((a) + 3)>>2)<<2)
#define SIZE_2X(a)              (((a + 1)>>1)<<1)

#define H26X_SWAP(A,B,TT) {TT TMP;\
                      TMP=A;\
                      A=B;\
                      B=TMP;}

#define IS_ISLICE(X) (X == I_SLICE || X == IDR_SLICE)
#define IS_PSLICE(X) (X == P_SLICE)
#define IS_IDRSLICE(X) (X == IDR_SLICE)

typedef enum{
	H26XENC_FAIL = 0,
	H26XENC_SUCCESS,
}H26XEncStatus;

typedef enum{
	VCODEC_H265 = 0,
	VCODEC_H264 = 1,
	VCODEC_NON_DEFINE
}CODEC_TYPE;

typedef enum{
    P_SLICE = 0,
    B_SLICE = 1,
    I_SLICE = 2,
    IDR_SLICE = 3,
    UNSET_SLICE_TYPE = 0xFF,
}SLICE_TYPE;

typedef struct _H26XENC_VAR_{
	CODEC_TYPE eCodecType;

    UINT32 uiEncId;
	UINT32 uiAPBAddr;
	UINT32 uiReportAddr[2];
	UINT32 uiCtxSize;

    void   *pVdoCtx;
	void   *pFuncCtx;
	void   *pComnCtx;
} H26XENC_VAR;

typedef struct _H26XDEC_VUI_{
	BOOL   bPresentFlag;
	UINT32 uiSarWidth;
	UINT32 uiSarHeight;
	UINT8  ucMatrixCoeff;
	UINT8  ucTransChar;
	UINT8  ucColorPrimaries;
	UINT8  ucVideoFmt;
	UINT8  ucColorRange;
	BOOL   bTimingPresentFlag;
} H26XDEC_VUI;

typedef struct _H26XDEC_VAR_{
	CODEC_TYPE eCodecType;

	UINT32 uiAPBAddr;
	UINT32 uiCtxSize;
	H26XDEC_VUI stVUI;

    void   *pContext;
} H26XDEC_VAR;

typedef struct _H26XEncSrcDecomp_ {
	UINT32 uiVirSrcDecSideInfoAddr;
	UINT32 uiSrcDecktable0;
	UINT32 uiSrcDecktable1;
	UINT32 uiSrcDecktable2;
	UINT32 uiSrcDecStripNum;
	UINT32 uiSrcDecStrip01Size;
	UINT32 uiSrcDecStrip23Size;
} H26XEncSrcDecomp;

typedef enum {
	CU16_QP_DEFAULT = 0,
	CU16_QP_DISABLE_AQVAR = 1,
	CU16_QP_DISABLE_AQ = 2,
	CU16_QP_FIXEDQP = 3,
} CU16_QP_MODE;

#endif

