#ifndef _H264DEC_API_H_
#define _H264DEC_API_H_

#include "kwrap/type.h"

#include "h264_def.h"
#include "h26x_def.h"

#define H264DEC_SUCCESS     1
#define H264DEC_FAIL        0

typedef struct _H264DEC_CODECINFO{
    H264_PROFILE  uiProfile;    ///< H.264 codec profile
    UINT32  uiLevel;            ///< H.264 codec level
    UINT32  uiRefFrameNum;      ///< H.264 reference frame number
} H264DEC_CODECINFO, *PH264DEC_CODECINFO;

typedef struct _H264DEC_INIT{
    UINT32 uiWidth;             ///< Decoded image width (buffer width should be aligned to 16)
    UINT32 uiHeight;            ///< Decoded image height (buffer height should be aligned to 16)
    UINT32 uiYLineOffset;       ///< Decoded image Y line offset
    UINT32 uiUVLineOffset;      ///< Decoded image UV line offset
    UINT32 uiDecBufAddr;        ///< H.264 decoder internal buffer starting address
    UINT32 uiDecBufSize;        ///< H.264 decoder internal buffer size
    UINT32 uiH264DescAddr;      ///< H.264 description data starting address
    UINT32 uiH264DescSize;      ///< H.264 description data size
    UINT32 uiDisplayWidth;      ///< Display source width, ex: 720 x 480, uiWidth = 736, uiDisplayWidth = 720

    H264DEC_CODECINFO  CodecInfo;   ///< H.264 codec information
}H264DEC_INIT, *PH264DEC_INIT;

typedef struct _H264DEC_INFO{
    UINT32 uiSrcYAddr;     ///< Output (decoded) source image Y address
    UINT32 uiSrcUVAddr;    ///< Output (decoded) source image UV address
    UINT32 uiBSAddr;       ///< Input bit-stream address
    UINT32 uiBSSize;       ///< Input bit-stream size
    UINT32 uiCurBsSize;	   ///< Input current bit-stream size

	// shall be to remove //
	UINT32 uiDecFrameType;					///< Decoded frame type: I, P, or B
    //UINT32 uiDecFrameNum;                   ///< Decode frame number (single, 2-B, ...)
} H264DEC_INFO, *PH264DEC_INFO;

#if 0
typedef struct _H264DEC_VAR{
    UINT32         uiDecId;
    void           *pContext;
} H264DEC_VAR;
#endif

typedef struct _H264DecMemInfo_{
	UINT32 uiHdrBsAddr;		///< [input]  : decode sps/pps heeader biststream addr
	UINT32 uiHdrBsLen;		///< [input]  : decode sps/pps heeader biststream size
	UINT32 uiWidth;			///< [output] : return width
	UINT32 uiHeight;		///< [output] : return height
	UINT32 uiMemSize;		///< [output] : return memory requirement size
}H264DecMemInfo;

extern INT32 h264Dec_initDecoder(PH264DEC_INIT pH264DecInit, H26XDEC_VAR *pH264DecVar);
extern INT32 h264Dec_prepareOnePicture(PH264DEC_INFO pH264DecInfo, H26XDEC_VAR *pH264DecVar);
extern INT32 h264Dec_setNextBsBuf(H26XDEC_VAR *pH264DecVar, UINT32 uiBsAddr, UINT32 uiBsSize, UINT32 uiTotalBsSize);
extern INT32 h264Dec_IsDecodeFinish(void);
extern void h264Dec_reset(void);
extern INT32 h264Dec_queryMemSize(H264DecMemInfo *pMemInfo);
extern UINT32 h264Dec_getResYAddr(H26XDEC_VAR *pH264DecVar);
extern UINT32 h264Dec_getResYAddr2(H26XDEC_VAR *pH264DecVar);
#endif // _H264DEC_API_H_
