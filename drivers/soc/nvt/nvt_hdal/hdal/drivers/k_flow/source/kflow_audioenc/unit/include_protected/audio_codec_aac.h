/**
    Header file of audio codec AAC library

    Exported header file of audio encoding codec library.

    @file       AudioCodecAAC.h
    @ingroup    mIAVCODEC
    @note       Nothing.
    @version    V1.01.003
    @date       2010/05/05

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/
#ifndef _AUDIOCODECAAC_H
#define _AUDIOCODECAAC_H

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
extern PMP_AUDENC_ENCODER MP_AACEnc_getAudioObject(MP_AUDENC_ID AudEncId);
extern PMP_AUDENC_ENCODER MP_AACEnc_getAudioEncodeObject(void);
//extern PMP_AUDENC_ENCODER MP_AACEnc_getEncodeObject(void);
//extern PMP_AUDENC_ENCODER MP_AACEnc_getNoAlignEncodeObject(void);//2012/02/23 Meg
extern PAUDIO_DECODER MP_AACDec_getAudioDecodeObject(void);
extern ER MP_AACDec_parseADTS(UINT32 bsAddr, UINT32 *pHeader_length, UINT32 *pStream_length);


//@}
#endif

//@}


