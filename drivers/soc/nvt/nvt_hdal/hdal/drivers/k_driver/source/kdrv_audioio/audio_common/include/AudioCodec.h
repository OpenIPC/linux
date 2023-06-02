/**
    Header file for Audio Codec module

    This file is the header file that define the API and data type
    for Audio Codec module.

    @file       AudioCodec.h
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#ifndef _AUDIOCODEC_H
#define _AUDIOCODEC_H

#include "Audio.h"

/**
    @addtogroup mISYSAud
*/
//@{

#define AUDIO_CODEC_FUNC_USE_DEFAULT    (0xE0000000)

/*
	Embedded audio codec object
*/
extern void 	audcodec_get_fp(PAUDIO_CODEC_FUNC pAudCodecFunc);

/*
	Dummy External audio codec object
*/
extern void     aud_ext_codec_emu_get_fp(PAUDIO_CODEC_FUNC pAudCodecFunc);

/*
	Respeaker 4-MIC <TDM-4CH> external audio codec object
*/
extern void     aud_ext_ac108_get_fp(PAUDIO_CODEC_FUNC pAudCodecFunc);


//@}

#endif

