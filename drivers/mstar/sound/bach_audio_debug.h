/*------------------------------------------------------------------------------
	Copyright (c) 2010 MStar Semiconductor, Inc.  All rights reserved.
------------------------------------------------------------------------------*/
#ifndef _BACH_AUDIO_DEBUG_H_
#define _BACH_AUDIO_DEBUG_H_
//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------

#if 0
#define TRACE_LEVEL_TAG        1
#define ERROR_LEVEL_TAG        1
#define DAI_LEVEL_TAG          0
#define CODEC_LEVEL_TAG        1
#define PLATFORM_LEVEL_TAG     1
#define PCM_LEVEL_TAG          1
#define PCM_DEBUG_LEVEL_TAG    0
#define PLAYBACK_IRQ_LEVEL_TAG 0
#define CAPTURE_IRQ_LEVEL_TAG  0
#else
#define TRACE_LEVEL_TAG        0
#define ERROR_LEVEL_TAG        1
#define DAI_LEVEL_TAG          0
#define CODEC_LEVEL_TAG        0
#define PLATFORM_LEVEL_TAG     0
#define PCM_LEVEL_TAG          0
#define PCM_DEBUG_LEVEL_TAG    0
#define PLAYBACK_IRQ_LEVEL_TAG 0
#define CAPTURE_IRQ_LEVEL_TAG  0
#endif


#define TRACE_LEVEL              "[AUDIO TRACE]"
#define ERROR_LEVEL              "[AUDIO ERROR]"
#define DAI_LEVEL                "[AUDIO DAI]"
#define CODEC_LEVEL              "[AUDIO CODEC]"
#define PLATFORM_LEVEL           "[AUDIO PLATFORM]"
#define PCM_LEVEL                "[AUDIO PCM]"
#define PCM_DEBUG_LEVEL          "[PCM DEBUG]"
#define PLAYBACK_IRQ_LEVEL       "[PLAYBACK IRQ]"
#define CAPTURE_IRQ_LEVEL        "[CAPTURE IRQ]"

#define LOG_MSG 					1
#if LOG_MSG
#define AUD_PRINTF(level ,fmt, arg...)		if (level##_TAG) printk(KERN_ERR level fmt, ##arg);
#else
#define AUD_PRINTF(level ,fmt, arg...)
#endif

#endif  /* _BACH_AUDIO_DEBUG_H_ */
