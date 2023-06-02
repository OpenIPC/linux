/**
    Header file of audio codec aLaw library

    Exported header file of audio encoding codec library.

    @file       AudioCodec_aLaw.h
    @ingroup    mIAVCODEC
    @note       Nothing.
    @version    V1.00.000
    @date       2015/08/17

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/
#ifndef _AUDIOCODECALAW_H
#define _AUDIOCODECALAW_H

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
extern PMP_AUDENC_ENCODER MP_aLawEnc_getAudioObject(MP_AUDENC_ID AudEncId);
extern PMP_AUDENC_ENCODER MP_aLawEnc_getAudioEncodeObject(void);
//extern PMP_AUDENC_ENCODER MP_aLawEnc_getEncodeObject(void);
extern PAUDIO_DECODER MP_aLawDec_getDecodeObject(void);
//@}

#endif

//@}


