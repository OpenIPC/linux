/**
    Header file of video codec H264 library

    Exported header file of video codec library.

    @file       VideoCodecH264.h
    @ingroup    mIAVCODEC
    @note       Nothing.
    @version    V1.01.003
    @date       2010/05/05

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#ifndef _VIDEOCODECH264_H
#define _VIDEOCODECH264_H


#include "kwrap/type.h"

#include "video_encode.h"

/*
    Public funtions to get video codec object
*/
//@{
// new API, to support multi-path video encoding
extern PMP_VDOENC_ENCODER MP_H264Enc_getVideoObject(MP_VDOENC_ID VidEncId);
// for backward compatible
extern PMP_VDOENC_ENCODER MP_H264Enc_getVideoEncodeObject(void);

//@}
#endif

