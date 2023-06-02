/**
    Header file of video codec MJPG library

    Exported header file of video codec library.

    @file       VideoCodecMJPG.h
    @ingroup    mIAVCODEC
    @note       Nothing.
    @version    V1.01.003
    @date       2010/05/05

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#ifndef _VIDEOCODECMJPG_H
#define _VIDEOCODECMJPG_H

#include "kwrap/type.h"
#define _SECTION(sec)

#include "video_decode.h"

/*
    Public funtions to get video codec object
*/
//@{
// new API, to support multi-path video decoding
extern PMP_VDODEC_DECODER MP_MjpgDec_getVideoObject(MP_VDODEC_ID VidDecId);
// for backward compatible
extern PMP_VDODEC_DECODER MP_MjpgDec_getVideoDecodeObject(void);

//@}
#endif

