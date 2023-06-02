/**
    Header file of audio codec PPCM library

    Exported header file of audio encoding codec library.

    @file       AudioCodecPPCM.h
    @ingroup    mIAVCODEC
    @note       Nothing.
    @version    V1.01.001
    @date       2014/09/24

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/
#ifndef _AUDIOCODECPPCM_H
#define _AUDIOCODECPPCM_H

#ifdef __KERNEL__
#include "kwrap/type.h"
#include "audio_encode.h"
#include "audio_decode.h"
#else
#include "kwrap/type.h"
#include "audio_encode.h"
#include "audio_decode.h"
#endif

/*
    Public funtions to get audio codec object
*/
//@{
extern PMP_AUDENC_ENCODER MP_PPCMEnc_getEncodeObject(void);
extern PMP_AUDENC_ENCODER MP_PPCMEnc_getNoAlignEncodeObject(void);
//extern PAUDIO_DECODER MP_PPCMDec_getAudioDecodeObject(void);


//@}
#endif

//@}


