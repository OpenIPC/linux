/*------------------------------------------------------------------------------
	Copyright (c) 2008 MStar Semiconductor, Inc.  All rights reserved.
------------------------------------------------------------------------------*/

/**
*  @file infinity_codec.h
*  @brief AUDIO Driver control command
*/

/**
* \defgroup audio_group  AUDIO driver
* @{
*/

#ifndef _BACH_CODEC_H_
#define _BACH_CODEC_H_

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------

//=============================================================================
// AUDIO CTRL CMD  defines
//=============================================================================

/**
* Used to get current chip name
*/
#define CHIP_VERSION "Chip Version"

/**
* Used to set Playback volume 0~94 (mapping to -64dB~30dB)
*/
#define MAIN_PLAYBACK_VOLUME "Main Playback Volume"

/**
* Used to set Capture volume 0~94 (mapping to -64dB~30dB)
*/
#define MAIN_CAPTURE_VOLUME "Main Capture Volume"

/**
* Used to set microphone pre gain level 0~7
*/
#define MICIN_GAIN_LEVEL "Mic Gain Level"

/**
* Used to set microphone gain level 0~3
*/
#define MICIN_PREGAIN_LEVEL "Mic PreGain Level"


/**
* Used to set line-in gain level 0~7
*/
#define LINEIN_GAIN_LEVEL "LineIn Gain Level"

/**
* Used to set internal debug sinegen gain level 0~4 (-6dB per step)
*/
#define SINEGEN_GAIN_LEVEL "SineGen Gain Level"

/**
* Used to set internal debug sinegen rate set 0~10 (0 for singen disabling)
*/
#define SINEGEN_RATE_SELECT "SineGen Rate Select"

/**
* Used to select audio playback input (DMA Reader, ADC In)
*/
#define MAIN_PLAYBACK_MUX "Main Playback Mux"

/**
* Used to select ADC input (Line-in, Mic-in)
*/
#define ADC_MUX "ADC Mux"


//=============================================================================
// enum
//=============================================================================

/// just for internal used
enum
{
AUD_PLAYBACK_MUX = 0,
AUD_ADC_MUX,    //0x1
AUD_ATOP_PWR,   //0x2
AUD_DPGA_PWR,   //0x3
AUD_PLAYBACK_DPGA,
AUD_CAPTURE_DPGA,
AUD_MIC_GAIN,
AUD_MICPRE_GAIN,
AUD_LINEIN_GAIN,
AUD_DIGMIC_PWR,
AUD_DBG_SINERATE,
AUD_DBG_SINEGAIN,
AUD_REG_LEN,
};

#endif /* _BACH_CODEC_H_ */


/** @} */ // end of audio_group
