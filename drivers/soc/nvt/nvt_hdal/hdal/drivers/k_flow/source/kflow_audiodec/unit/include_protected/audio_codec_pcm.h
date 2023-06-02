/**
    Header file of audio encoding codec PCM library

    Exported header file of audio encoding codec library.

    @file       AudioCodecPCM.h
    @ingroup    mIAVCODEC
    @note       Nothing.
    @version    V1.01.003
    @date       2010/05/05

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#ifndef _AUDIOCODECPCM_H
#define _AUDIOCODECPCM_H

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
    Public funtions to get audio encoding object
*/
//@{
//extern PMP_AUDENC_ENCODER MP_PCMEnc_getEncodeObject(void);
//extern PMP_AUDDEC_DECODER MP_PCMDec_getAudioDecodeObject(void);
extern PMP_AUDDEC_DECODER MP_PCMDec_getAudioObject(MP_AUDDEC_ID AudDecId);
extern PMP_AUDDEC_DECODER MP_PCMDec_getAudioDecodeObject(void);

//@}
#endif

//@}

