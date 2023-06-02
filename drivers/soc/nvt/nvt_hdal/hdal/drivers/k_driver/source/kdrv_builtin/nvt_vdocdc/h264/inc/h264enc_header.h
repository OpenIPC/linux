#ifndef _H264_HEADER_H_
#define _H264_HEADER_H_

#include "kwrap/type.h"

#include "h26x.h"
#include "h26x_def.h"
#include "h26x_common.h"

#include "h264_def.h"
#include "h264enc_api.h"

extern void h264Enc_encSeqHeader(H264ENC_CTX *pVdoCtx, H26XCOMN_CTX *pComnCtx);
extern void h264Enc_encSliceHeader(H264ENC_CTX *pVdoCtx, H26XFUNC_CTX *pFuncCtx, H26XCOMN_CTX *pComnCtx, BOOL bGetSeqHdrEn);

#endif	// _H264_HEADER_H_

