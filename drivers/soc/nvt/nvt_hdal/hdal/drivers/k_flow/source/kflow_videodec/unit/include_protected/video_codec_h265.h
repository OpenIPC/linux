/**
    Header file of video codec H265 library

    Exported header file of video codec library.

    @file       VideoCodecH265.h
    @ingroup    mIAVCODEC
    @note       Nothing.
    @version    V1.01.003
    @date       2010/05/05

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#ifndef _VIDEOCODECH265_H
#define _VIDEOCODECH265_H

#include "kwrap/type.h"

#include "video_decode.h"

/*
    Public funtions to get video codec object
*/
//@{
// new API, to support multi-path video decoding
extern PMP_VDODEC_DECODER MP_H265Dec_getVideoObject(MP_VDODEC_ID VidDecId);
// for backward compatible
extern PMP_VDODEC_DECODER MP_H265Dec_getVideoDecodeObject(void);

//@}
#endif

