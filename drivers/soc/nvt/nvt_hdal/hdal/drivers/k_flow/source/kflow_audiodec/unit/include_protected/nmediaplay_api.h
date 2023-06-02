/**
    Header file of NMedia Player Task

    Exported header file of media player task.

    @file       nmediaplay_api.h
    @ingroup    mIAPPNMEDIAPLAY

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _NMEDIAPLAYAPI_H
#define _NMEDIAPLAYAPI_H

#include "media_def.h"
#include "movieinterface_def.h"

#define    NMEDIAPLAY_US_IN_SECOND       (1000*1000)

/**
    @name Valid audio decode codec

    Valid audio decode codec.
*/
//@{
#define NMEDIAPLAY_DEC_PCM		MEDIAAUDIO_CODEC_SOWT		///< PCM audio format
#define NMEDIAPLAY_DEC_AAC		MEDIAAUDIO_CODEC_MP4A		///< AAC audio format
#define NMEDIAPLAY_DEC_PPCM		MEDIAAUDIO_CODEC_RAW8		///< RAW audio format
#define NMEDIAPLAY_DEC_ULAW		MEDIAAUDIO_CODEC_ULAW		///< ULAW audio format
#define NMEDIAPLAY_DEC_ALAW		MEDIAAUDIO_CODEC_ALAW		///< ULAW audio format
#define NMEDIAPLAY_DEC_XX		MEDIAAUDIO_CODEC_XX			///< Unknown audio format

#define NMEDIAPLAY_DEC_MJPG		MEDIAPLAY_VIDEO_MJPG    	///< motion-jpeg
#define NMEDIAPLAY_DEC_H264		MEDIAPLAY_VIDEO_H264    	///< H.264
#define NMEDIAPLAY_DEC_H265		MEDIAPLAY_VIDEO_H265    	///< H.265
#define NMEDIAPLAY_DEC_YUV		MEDIAPLAY_VIDEO_YUV    		///< YUV

/**
    Speed type definition
*/
typedef enum {
    NMEDIAPLAY_SPEED_NORMAL  = 0,                                  ///< normal speed
    NMEDIAPLAY_SPEED_2X      = 2,                                  ///< 2x    speed, trigger by timer (Play 2*I-frame number in second)
    NMEDIAPLAY_SPEED_4X      = 4,                                  ///< 4x    speed, trigger by timer (Play 4*I-frame number in second)
    NMEDIAPLAY_SPEED_8X      = 8,                                  ///< 8x    speed, trigger by timer (Play 8*I-frame number in second)
    NMEDIAPLAY_SPEED_16X     = 16,                                 ///< 16x   speed, trigger by timer (Play 8*I-frame number & skip 1 I-frames in second)
    NMEDIAPLAY_SPEED_32X     = 32,                                 ///< 32x   speed, trigger by timer (Play 8*I-frame number & skip 3 I-frames in second)
    NMEDIAPLAY_SPEED_64X     = 64,                                 ///< 64x   speed, trigger by timer (Play 8*I-frame number & skip 7 I-frames in second)
    /*
   	NMEDIAPLAY_SPEED_1_2X    = 5000,                               ///< 1/2x  speed, trigger by timer (Play 1/2*I-frame number in second)
    NMEDIAPLAY_SPEED_1_4X    = 2500,                               ///< 1/4x  speed, trigger by timer (Play 1/4*I-frame number in second)
    NMEDIAPLAY_SPEED_1_8X    = 1250,                               ///< 1/8x  speed, trigger by timer (Play 1/8*I-frame number in second)
    NMEDIAPLAY_SPEED_1_16X   = 625,                                ///< 1/16x speed, trigger by timer (Play 1/16*I-frame number in second)
    */
    NMEDIAPLAY_SPEED_1_2X    = NMEDIAPLAY_US_IN_SECOND / 2,       ///< 1/2x  speed, trigger by timer (Play 1/2*I-frame number in second)
    NMEDIAPLAY_SPEED_1_4X    = NMEDIAPLAY_US_IN_SECOND / 4,       ///< 1/4x  speed, trigger by timer (Play 1/4*I-frame number in second)
    NMEDIAPLAY_SPEED_1_8X    = NMEDIAPLAY_US_IN_SECOND / 8,       ///< 1/8x  speed, trigger by timer (Play 1/8*I-frame number in second)
    NMEDIAPLAY_SPEED_1_16X   = NMEDIAPLAY_US_IN_SECOND / 16,      ///< 1/16x speed, trigger by timer (Play 1/16*I-frame number in second)
	ENUM_DUMMY4WORD(NMEDIAPLAY_SPEED_TYPE)
} NMEDIAPLAY_SPEED_TYPE;

/**
    Direction type definition
*/
typedef enum {
    NMEDIAPLAY_DIRECT_FORWARD  = 1,                                ///< forward direction
    NMEDIAPLAY_DIRECT_BACKWARD = 2,                                ///< backward direction
	ENUM_DUMMY4WORD(NMEDIAPLAY_DIRECT_TYPE)
} NMEDIAPLAY_DIRECT_TYPE;

//@}

//extern void NMP_BsDemux_InstallID(void) _SECTION(".kercfg_text");
extern void nmp_vdodec_install_id(void) _SECTION(".kercfg_text");
extern void nmp_auddec_install_id(void) _SECTION(".kercfg_text");
extern void isf_auddec_install_id(void) _SECTION(".kercfg_text");
extern void nmp_vdodec_uninstall_id(void);
extern void nmp_auddec_uninstall_id(void);
extern void isf_auddec_uninstall_id(void);

#endif//_NMEDIAPLAYAPI_H

//@}
