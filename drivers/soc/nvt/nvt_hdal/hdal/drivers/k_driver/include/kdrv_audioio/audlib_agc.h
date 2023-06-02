/**
    Audio Gain Control (AGC) library header file

    This file is the global header of Audio Gain Control (AGC) library.

    @file       Agc.h
    @ingroup    mIAudGC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/

#ifndef _AUDLIB_AGC_H
#define _AUDLIB_AGC_H

#include "kdrv_type.h"

/**
    @addtogroup mIAudGC
*/
//@{

/**
    Audio Gain Control (AGC) Configuration Select ID

    This is used in audlib_agc_set_config() as the select ID to assign new configurations
    for Audio Gain Control (AGC) library.
*/
typedef enum {
	AGC_CONFIG_ID_SAMPLERATE,       ///< Set SampleRate in Hertz.
	AGC_CONFIG_ID_CHANNEL_NO,		///< Please input 1 or 2 audio channel numbers. Default is 1.
	AGC_CONFIG_ID_PEAKDET_SPEED,	///< Set the peak detect speed in ms. Set 0 to use driver default value.

	AGC_CONFIG_ID_TARGET_LVL,	    ///< Set Auto Gain Control Target Level. Please use AGC_DB(x) as input. x Valid Range is -3 ~ -40 (dB).
	AGC_CONFIG_ID_MAXGAIN,			///< Set Auto Gain Control Maximum Gain value. Please use AGC_DB(x) as input. x Valid range is 0 ~ +30 (dB).
	AGC_CONFIG_ID_MINGAIN,			///< Set Auto Gain Control Minimum Gain value. Please use AGC_DB(x) as input. x Valid range is 0 ~ -30 (dB).
	AGC_CONFIG_ID_DECAY_TIME,		///< Set Auto Gain Control Normal State DECAY  time (Gain Ramp Up Speed).Please use AGC_TRESO_BASIS as input.
	AGC_CONFIG_ID_ATTACK_TIME,		///< Set Auto Gain Control Normal State ATTACK time (Gain Ramp Down Speed).Please use AGC_TRESO_BASIS as input.

	AGC_CONFIG_ID_NG_THD,			///< Set Auto Gain Control NoiseGate Threshold. Please use AGC_DB(x) as input.
									///< Default value is AGC_DB(-100) which is equal to NoiseGate Disabled.
	AGC_CONFIG_ID_NG_DECAY_TIME,	///< Set Auto Gain Control NoiseGate State DECAY  time (Gain Ramp Up Speed). Please use AGC_TRESO_BASIS as input.
	AGC_CONFIG_ID_NG_ATTACK_TIME,	///< Set Auto Gain Control NoiseGate State ATTACK time (Gain Ramp Down Speed). Please use AGC_TRESO_BASIS as input.
	AGC_CONFIG_ID_NG_TARGET_RATIO,	///< Set Auto Gain Control NoiseGate Target level ratio. Allowed setup value is 0 or 1. Default is 0.

	AGC_CONFIG_ID_MSGDUMP,			///< ENABLE/DISABLE the settings message dump at audlib_agc_init().
	ENUM_DUMMY4WORD(AGC_CONFIG_ID)
} AGC_CONFIG_ID;


/**
	Auto Gain Control (AGC) basic time resolution

	This is used in audlib_agc_set_config(AGC_CONFIG_ID_DECAY_TIME/AGC_CONFIG_ID_ATTACK_TIME/AGC_CONFIG_ID_NG_DECAY_TIME/AGC_CONFIG_ID_NG_ATTACK_TIME)
	to setup the AGC gain control variation speed.
	This parameters is a roughly measure result by using 48Khz audio sample.
	And the test condition is making AGC decay/attack changing total 25dB.
*/
typedef enum {
	AGC_TRESO_BASIS_500US   = 5,		///< 500us  changing speed for AGC Decay/Attack
	AGC_TRESO_BASIS_1MS		= 10,		///< 1ms    changing speed for AGC Decay/Attack
	AGC_TRESO_BASIS_2MS		= 17,		///< 2ms    changing speed for AGC Decay/Attack
	AGC_TRESO_BASIS_5MS		= 30,		///< 5ms    changing speed for AGC Decay/Attack
	AGC_TRESO_BASIS_10MS	= 50,		///< 10ms   changing speed for AGC Decay/Attack
	AGC_TRESO_BASIS_50MS    = 500,		///< 50ms   changing speed for AGC Decay/Attack
	AGC_TRESO_BASIS_100MS   = 1000,		///< 100ms  changing speed for AGC Decay/Attack
	AGC_TRESO_BASIS_200MS	= 1600,		///< 200ms  changing speed for AGC Decay/Attack
	AGC_TRESO_BASIS_500MS   = 5000,		///< 500ms  changing speed for AGC Decay/Attack
	AGC_TRESO_BASIS_1000MS  = 10000,	///< 1000ms changing speed for AGC Decay/Attack

	ENUM_DUMMY4WORD(AGC_TRESO_BASIS)
} AGC_TRESO_BASIS;


/**
    Structure for Audio Gain Control (AGC) input BitStream(BS) information

    Structure for audio Echo Cancellation input BitStream information, such as BS length and BS buffer address.
    The bitstram_buffer_length must be multiple of 1024 samples. This is used in audlib_agc_run().
*/
typedef struct {
	UINT32      bitstram_buffer_in;			///< Audio BitStream buffer address for AGC input
	UINT32      bitstram_buffer_out;		///< Audio BitStream buffer address for AGC output
	UINT32      bitstram_buffer_length;		///< Audio Stream Length in Samples count. Allowed Unit in 1 samples.
} AGC_BITSTREAM, *PAGC_BITSTREAM;

/**
	This is used is audlib_agc_set_config() for paramters settings.
*/
#define AGC_DB(x)					((INT32)((x)*AGC_GAINDB_FRACTIONAL))





/*
	Driver internal usage definotions
*/
#define AGC_GAINDB_FRACTIONAL_BITS	10
#define AGC_GAINDB_FRACTIONAL		1024
#define AGC_GAINCTRL_FRACTIONAL		(AGC_GAINDB_FRACTIONAL_BITS-1)


//
//  Exporting APIs
//

extern ER       audlib_agc_open(void);
extern BOOL     audlib_agc_is_opened(void);
extern ER       audlib_agc_close(void);
extern void     audlib_agc_set_config(AGC_CONFIG_ID agc_sel, INT32 agc_cfg_value);
extern BOOL     audlib_agc_init(void);
extern BOOL     audlib_agc_run(PAGC_BITSTREAM p_agc_io);

int kdrv_audlib_agc_init(void);


//@}
#endif
