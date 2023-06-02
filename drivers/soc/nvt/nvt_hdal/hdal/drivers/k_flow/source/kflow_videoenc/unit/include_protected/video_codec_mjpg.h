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

#include "video_encode.h"

/*
    Public funtions to get video codec object
*/
//@{
extern PMP_VDOENC_ENCODER MP_MjpgEnc_getVideoEncodeObject(void);
extern PMP_VDOENC_ENCODER MP_MjpgEnc_getVideoObject(MP_VDOENC_ID VidEncId);

//@}
#endif

