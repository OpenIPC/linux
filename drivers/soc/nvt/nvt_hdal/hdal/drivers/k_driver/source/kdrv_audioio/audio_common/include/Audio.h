/**
    Header file for Audio module

    This file is the header file that define the API and data type
    for Audio module. The NT96680's audio module is full-duplex capable and allow to transmit
    two playback channels simutaneously.
\n  Please also refer to audio driver application note for sample code examples.

    @file       Audio.h
    @ingroup    mISYSAud
    @note       None.

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/

#ifndef _AUDIO_H
#define _AUDIO_H

#ifdef __KERNEL__
#include "kwrap/type.h"
#elif defined(__FREERTOS)
#include "kwrap/type.h"
#else
#include "Type.h"
#endif
/**
    @addtogroup mISYSAud
*/
//@{
#define AUDIO_IIR_COEF(x) ((INT32)(.5+(x)*(((INT32)1)<<(15))))
#define AUDIO_GAIN_DB(x) (INT32)(x*2)

//
//  Audio Transceive Object Related Definitions
//


/**
    Audio TranSceive(TS) Object ID

    This definition is used in aud_get_transceive_object() to get the selected audio Transmit or
    Receive channel Object. The user is access the audio playback/record channels by the data structure AUDTS_OBJ.
    Please refer to the AUDTS_OBJ for details definitions.
\n  There are four Audio TranSceive channels are available for NT96680. They are TX1/TX2/RX/TXLB channels.
    The Channel TX1/TX2 are used to playback audio sound. The Channel RX is used to record audio data.
    And the Channel TXLB can get the mixed playback sound back to DRAM and the data is used for echo cancellation(AEC).
    These four different channels can be used independently and simultaneously.
\n  The TX1/TX2 channels are mixed by the digital addition automatically inside the hardware engine.
\n  The playback and record audio channels must be using the same audio sampling rate!
*/
typedef enum {
	AUDTS_CH_TX1,   ///< Playback Channel 1. (TimeCode Hit function Available)
	AUDTS_CH_TX2,   ///< Playback Channel 2.
	AUDTS_CH_RX,    ///< Record Channel.(TimeCode Hit function Available)(AVSYNC function Available)
	AUDTS_CH_TXLB,  ///< Playback Loopback Channel.

	AUDTS_CH_TOT,   ///<   Total AUDTS_CH count
	ENUM_DUMMY4WORD(AUDTS_CH)
} AUDTS_CH;


/**
    Audio Transceive Object Feature

    Set dedicate playback/record channel's feature.

    @note For AUDTS_OBJ.setFeature()
*/
typedef enum {
	/*
	    Valid for both playback and record (TX/RX)
	*/
	AUDTS_FEATURE_TIMECODE_HIT,         ///< Set Time Code Hit Event enable. Only TX1 and RX channels have this function.

	/*
	    Valid for record(RX) only
	*/
	AUDTS_FEATURE_AVSYNC,               ///< Set AV SYNC function enable. Only RX channel has this function.
	AUDTS_FEATURE_TIMECODE_LATCH,       ///< Set Time Code Latch Event enable. Only RX channel has this function.
	AUDTS_FEATURE_RECORD_PCM_EXPAND,    ///< Set Record Mono(or Dual-Mono) Channel EXPAND to DRAM Stereo Data.

	/*
	    Valid for playback(TX) only
	*/
	AUDTS_FEATURE_PLAYBACK_PCM_EXPAND,  ///< Set Playback DRAM Mono Source Data EXPAND to Stereo Playback.


	ENUM_DUMMY4WORD(AUDTS_FEATURE)
} AUDTS_FEATURE;


/**
    Audio Transceive Object Configuration

    @note For AUDTS_OBJ.setConfig()
*/
typedef enum {
	AUDTS_CFG_ID_EVENT_HANDLE,      ///< The callback event handle for dedicate playback/record channel.
	///< The callback prototype is AUDIO_CB.

	AUDTS_CFG_ID_TIMECODE_TRIGGER,  ///< The dedicate playback/record channel's timecode trigger value.
	AUDTS_CFG_ID_TIMECODE_OFFSET,   ///< The dedicate playback/record channel's timecode offset value.
	AUDTS_CFG_ID_TIMECODE_VALUE,    ///< The dedicate playback/record channel's current timecode value.

	AUDTS_CFG_ID_PCM_BITLEN,        ///< Configure PCM source as 8bits(AUDIO_PCMLEN_8BITS) or 16bits(AUDIO_PCMLEN_16BITS)
	///< or 32bits(AUDIO_PCMLEN_32BITS). The 32bits only valid for I2S interface.

	ENUM_DUMMY4WORD(AUDTS_CFG_ID)
} AUDTS_CFG_ID;



//
//  Audio Global API Related Definitions
//

/**
    AUDIO Record Source

    This setting depends on the codec driver implementation.
    For Embedded codec, this setting is useless.

    @note For AUDIO_SETTING, and aud_set_record_source()
*/
typedef enum {
	AUDIO_RECSRC_MIC,           ///< Microphone
	AUDIO_RECSRC_DMIC,			///< Digital Microphone
	AUDIO_RECSRC_CD,            ///< CD In
	AUDIO_RECSRC_VIDEO,         ///< Video In
	AUDIO_RECSRC_AUX,           ///< Aux In
	AUDIO_RECSRC_LINE,          ///< Line In
	AUDIO_RECSRC_STEREO_MIX,    ///< Stereo Mix
	AUDIO_RECSRC_MONO_MIX,      ///< Mono Mix
	AUDIO_RECSRC_PHONE,         ///< Phone In

	ENUM_DUMMY4WORD(AUDIO_RECSRC)
} AUDIO_RECSRC;

/**
    AUDIO Effect

    Usages depends on external audio codec library implementation.
    This is useless for embedded audio codec.

    @note For aud_set_effect()
*/
typedef enum {
	AUDIO_EFFECT_NONE,          ///< No special audio effect
	AUDIO_EFFECT_3D_HALF,       ///< 50% 3D effect
	AUDIO_EFFECT_3D_FULL,       ///< 100% 3D effect
	AUDIO_EFFECT_ROCK,          ///< Rock
	AUDIO_EFFECT_POP,           ///< POP
	AUDIO_EFFECT_JAZZ,          ///< Jazz
	AUDIO_EFFECT_CLASSICAL,     ///< Classical
	AUDIO_EFFECT_DNACE,         ///< Dance
	AUDIO_EFFECT_HEAVY,         ///< Heavy
	AUDIO_EFFECT_DISCO,         ///< Disco
	AUDIO_EFFECT_SOFT,          ///< Soft
	AUDIO_EFFECT_LIVE,          ///< Live
	AUDIO_EFFECT_HALL,          ///< Hall

	ENUM_DUMMY4WORD(AUDIO_EFFECT)
} AUDIO_EFFECT;


/**
    AUDIO I2S format

    For embedded audio codec, only the AUDIO_I2SFMT_STANDARD is valid.
    Others options depend on the external audio codec implementation.

    @note For AUDIO_I2S_SETTING, and aud_set_i2s_format()
*/
typedef enum {
	AUDIO_I2SFMT_STANDARD,      ///< I2S Standard
	AUDIO_I2SFMT_DSP,           ///< I2S DSP mode. No supported for internal codec.
	AUDIO_I2SFMT_LIKE_MSB,      ///< I2S Like, MSB justified. No supported for internal codec.
	AUDIO_I2SFMT_LIKE_LSB,      ///< I2S Like, LSB justified. No supported for internal codec.

	ENUM_DUMMY4WORD(AUDIO_I2SFMT)
} AUDIO_I2SFMT;

/**
    AUDIO I2S TDM Channel Bits

    For I2S TDM external audio codec,
    Use "aud_set_i2s_chbits(AUDIO_I2SCH_BITS)" to specify how many bits per audio TDM I2S channel.
*/
typedef enum {
	AUDIO_I2SCH_BITS_16,        ///< I2S Interface is 16bits per channel.
	AUDIO_I2SCH_BITS_32,        ///< I2S Interface is 32bits per channel.

	ENUM_DUMMY4WORD(AUDIO_I2SCH_BITS)
} AUDIO_I2SCH_BITS;

/**
    AUDIO I2S clock ratio

    For embedded audio codec, only the AUDIO_I2SCLKR_256FS_32BIT/AUDIO_I2SCLKR_256FS_64BIT
    /AUDIO_I2SCLKR_256FS_128BIT/AUDIO_I2SCLKR_256FS_256BIT are valid.
    Others options depend on the external audio codec implementation.

    @note For AUDIO_I2S_SETTING, and aud_set_i2s_clkratio()
*/
typedef enum {
	AUDIO_I2SCLKR_256FS_32BIT,  ///< SystemClk = 256 * FrameSync, FrameSync = 32 BitClk
	AUDIO_I2SCLKR_256FS_64BIT,  ///< SystemClk = 256 * FrameSync, FrameSync = 64 BitClk
	AUDIO_I2SCLKR_256FS_128BIT, ///< SystemClk = 256 * FrameSync, FrameSync = 128 BitClk
	AUDIO_I2SCLKR_256FS_256BIT, ///< SystemClk = 256 * FrameSync, FrameSync = 256 BitClk

	AUDIO_I2SCLKR_384FS_32BIT,  ///< SystemClk = 384 * FrameSync, FrameSync = 32 BitClk. No supported for internal codec.
	AUDIO_I2SCLKR_384FS_48BIT,  ///< SystemClk = 384 * FrameSync, FrameSync = 48 BitClk. No supported for internal codec.
	AUDIO_I2SCLKR_384FS_96BIT,  ///< SystemClk = 384 * FrameSync, FrameSync = 96 BitClk. No supported for internal codec.

	AUDIO_I2SCLKR_512FS_32BIT,  ///< SystemClk = 512 * FrameSync, FrameSync = 32 BitClk. No supported for internal codec.
	AUDIO_I2SCLKR_512FS_64BIT,  ///< SystemClk = 512 * FrameSync, FrameSync = 64 BitClk. No supported for internal codec.
	AUDIO_I2SCLKR_512FS_128BIT, ///< SystemClk = 512 * FrameSync, FrameSync = 128 BitClk. No supported for internal codec.

	AUDIO_I2SCLKR_768FS_32BIT,  ///< SystemClk = 768 * FrameSync, FrameSync = 32 BitClk. No supported for internal codec.
	AUDIO_I2SCLKR_768FS_48BIT,  ///< SystemClk = 768 * FrameSync, FrameSync = 48 BitClk. No supported for internal codec.
	AUDIO_I2SCLKR_768FS_64BIT,  ///< SystemClk = 768 * FrameSync, FrameSync = 64 BitClk. No supported for internal codec.
	AUDIO_I2SCLKR_768FS_192BIT, ///< SystemClk = 768 * FrameSync, FrameSync = 192 BitClk. No supported for internal codec.

	AUDIO_I2SCLKR_1024FS_32BIT, ///< SystemClk = 1024 * FrameSync, FrameSync = 32 BitClk. No supported for internal codec.
	AUDIO_I2SCLKR_1024FS_64BIT, ///< SystemClk = 1024 * FrameSync, FrameSync = 64 BitClk. No supported for internal codec.
	AUDIO_I2SCLKR_1024FS_128BIT,///< SystemClk = 1024 * FrameSync, FrameSync = 128 BitClk. No supported for internal codec.
	AUDIO_I2SCLKR_1024FS_256BIT,///< SystemClk = 1024 * FrameSync, FrameSync = 256 BitClk. No supported for internal codec.

	ENUM_DUMMY4WORD(AUDIO_I2SCLKR)
} AUDIO_I2SCLKR;

/**
    AUDIO Format

    Audio Interface format.
    For embedded audio codec, only the AUDIO_FMT_I2S are valid.
    AUDIO_FMT_AC97 is valid or not depends on the external codec driver implementation.

    @note For Fmt of AUDIO_SETTING
*/
typedef enum {
	AUDIO_FMT_I2S,              ///< I2S
	AUDIO_FMT_AC97,             ///< AC97. (Not valid for embedded audio codec)

	ENUM_DUMMY4WORD(AUDIO_FMT)
} AUDIO_FMT;

/**
    AUDIO Channel

    This is used to setup the audio channel as left/right or stereo channels
    for audio playback or record path.
    The option AUDIO_CH_DUAL_MONO is only valid for record path only.
    If I2S TDM 4/6/8 channels is used, this must be set to STEREO.
    When I2S TDM 2 channels is used, this can be used to control left/right/stereo channels.

    @note For AUDIO_SETTING, and AUDTS_OBJ.setChannel()
*/
typedef enum {
	AUDIO_CH_LEFT,              ///< Left
	AUDIO_CH_RIGHT,             ///< Right
	AUDIO_CH_STEREO,            ///< Stereo
	AUDIO_CH_MONO,              ///< Mono two channel. Obselete. Shall not use this option.
	AUDIO_CH_DUAL_MONO,         ///< Dual Mono Channels. Valid for record(RX) only.

	ENUM_DUMMY4WORD(AUDIO_CH)
} AUDIO_CH;

/**
    AUDIO TDM Channel number

    This is used to setup the audio TDM total channel number for playback/record.
    This field is used to specify the DRAM data format of TDM playback/record.

    @note For AUDTS_OBJ.setTdmChannel()
*/
typedef enum {
	AUDIO_TDMCH_2CH,              ///< I2S TDM 2CH
	AUDIO_TDMCH_4CH,              ///< I2S TDM 4CH
	AUDIO_TDMCH_6CH,              ///< I2S TDM 6CH
	AUDIO_TDMCH_8CH,              ///< I2S TDM 8CH

	ENUM_DUMMY4WORD(AUDIO_TDMCH)
} AUDIO_TDMCH;

/**
    AUDIO Sampling Rate

    The user must notice that the embedded audio codec can only support the same sampling rate
    for playback/record. If the user uses the full-duplex mode, must set to the same sampling rate for both
    the playback and record.
    This sampling rate definition is used to control the internal audio hardware engine operating clock frequency.

    @note For SamplingRate of AUDIO_SETTING, AudioTargetSR of AUDIO_RESAMPLE_INFO, and AUDTS_OBJ.setSamplingRate()
*/
typedef enum {
	AUDIO_SR_8000   = 8000,     ///< 8 KHz
	AUDIO_SR_11025  = 11025,    ///< 11.025 KHz
	AUDIO_SR_12000  = 12000,    ///< 12 KHz
	AUDIO_SR_16000  = 16000,    ///< 16 KHz
	AUDIO_SR_22050  = 22050,    ///< 22.05 KHz
	AUDIO_SR_24000  = 24000,    ///< 24 KHz
	AUDIO_SR_32000  = 32000,    ///< 32 KHz
	AUDIO_SR_44100  = 44100,    ///< 44.1 KHz
	AUDIO_SR_48000  = 48000,    ///< 48 KHz

	ENUM_DUMMY4WORD(AUDIO_SR)
} AUDIO_SR;

/**
    AUDIO Output/playback path

    @note For AUDIO_SETTING, and aud_set_output()
*/
typedef enum {
	AUDIO_OUTPUT_SPK,           ///< Speaker.(Valid for embedded audio codec)
	AUDIO_OUTPUT_HP,            ///< Headphone
	AUDIO_OUTPUT_LINE,          ///< Line Out.(Valid for embedded audio codec)
	AUDIO_OUTPUT_MONO,          ///< Mono Out
	AUDIO_OUTPUT_NONE,          ///< Disable output.(Valid for embedded audio codec)
	AUDIO_OUTPUT_ALL,            ///< output to spk + lineout

	ENUM_DUMMY4WORD(AUDIO_OUTPUT)
} AUDIO_OUTPUT;

/**
    AUDIO I2S Setting

    @note For I2S of AUDIO_SETTING
*/
typedef struct {
	AUDIO_I2SCLKR       ClkRatio;       ///< Clock Ratio
	AUDIO_I2SFMT        I2SFmt;         ///< I2S format
	BOOL                bMaster;        ///< Audio Controller Master/Slave Mode
	///< - @b TRUE: Master mode
	///< - @b FALSE: Slave mode
} AUDIO_I2S_SETTING;

/*
    Obselete. This setting is only for backward compatible usage.
    The audio clock setting is depending on samplerate setting only.
*/
typedef enum {
	AUDIO_CLK_24576,
	AUDIO_CLK_12288,
	AUDIO_CLK_8192,
	AUDIO_CLK_6144,
	AUDIO_CLK_4096,
	AUDIO_CLK_3072,
	AUDIO_CLK_2048,
	AUDIO_CLK_11289,
	AUDIO_CLK_5644,
	AUDIO_CLK_2822,

	ENUM_DUMMY4WORD(AUDIO_CLK)
} AUDIO_CLK;

/**
    AUDIO Clock Setting

    @note For Clock of AUDIO_SETTING
*/
typedef struct {
	BOOL                bClkExt;        ///< Clock Source. Embedded auido codec only support internal clock source.
	///< - @b TRUE: External clock source
	///< - @b FALSE: Internal clock source
	AUDIO_CLK           Clk;            ///< Obselete and useless parameter. Backward compatible usage.
} AUDIO_CLK_SETTING;

/**
    AUDIO Initial Settings.

    @note For aud_init()
*/
typedef struct {
	AUDIO_CLK_SETTING   Clock;          ///< Audio clock setting. Obselete and useless parameter.
	AUDIO_I2S_SETTING   I2S;            ///< I2S Setting
	AUDIO_FMT           Fmt;            ///< Audio interface format
	AUDIO_SR            SamplingRate;   ///< Sampling Rate
	AUDIO_CH            Channel;        ///< Audio channel
	AUDIO_RECSRC        RecSrc;         ///< Record Source
	AUDIO_OUTPUT        Output;         ///< Output/playback Path
	BOOL                bEmbedded;      ///< Obselete and useless parameter. Backward compatible usage.

} AUDIO_SETTING, *PAUDIO_SETTING;


/**
    Audio codec selection

    The api aud_switch_codec(AUDIO_CODECSEL) is used to switch audio-codec(internal or external) and the HDMI-audio.

    @note For aud_switch_codec()
*/
typedef enum {
	AUDIO_CODECSEL_DEFAULT,     ///< Default audio codec. Can be embeddec codec or external codec.
	AUDIO_CODECSEL_HDMI,        ///< HDMI audio

	ENUM_DUMMY4WORD(AUDIO_CODECSEL)
} AUDIO_CODECSEL;


/**
    AUDIO Total Volume Level

    @note For aud_set_total_vol_level()
*/
typedef enum {
	AUDIO_VOL_LEVEL8  =  8,            ///< total  8 volume level
	AUDIO_VOL_LEVEL64 = 64,            ///< total 64 volume level

	ENUM_DUMMY4WORD(AUDIO_VOL_LEVEL)
} AUDIO_VOL_LEVEL;

/**
    AUDIO Playback Volume

    If aud_set_total_vol_level() is set to  AUDIO_VOL_LEVEL8,  the AUDIO_VOL_MUTE/AUDIO_VOL_0 ~ AUDIO_VOL_7  are valid.
    If aud_set_total_vol_level() is set to  AUDIO_VOL_LEVEL64, the AUDIO_VOL_MUTE/AUDIO_VOL_0 ~ AUDIO_VOL_63 are valid.

    @note For aud_set_volume()
*/
typedef enum {
	AUDIO_VOL_MUTE,             ///< Volume mute
	AUDIO_VOL_0,                ///< Volume 0
	AUDIO_VOL_1,                ///< Volume 1
	AUDIO_VOL_2,                ///< Volume 2 (Default)
	AUDIO_VOL_3,                ///< Volume 3
	AUDIO_VOL_4,                ///< Volume 4
	AUDIO_VOL_5,                ///< Volume 5
	AUDIO_VOL_6,                ///< Volume 6
	AUDIO_VOL_7,                ///< Volume 7
	AUDIO_VOL_8,                ///< Volume 8
	AUDIO_VOL_9,                ///< Volume 9
	AUDIO_VOL_10,               ///< Volume 10
	AUDIO_VOL_11,               ///< Volume 11
	AUDIO_VOL_12,               ///< Volume 12
	AUDIO_VOL_13,               ///< Volume 13
	AUDIO_VOL_14,               ///< Volume 14
	AUDIO_VOL_15,               ///< Volume 15
	AUDIO_VOL_16,               ///< Volume 16
	AUDIO_VOL_17,               ///< Volume 17
	AUDIO_VOL_18,               ///< Volume 18
	AUDIO_VOL_19,               ///< Volume 19
	AUDIO_VOL_20,               ///< Volume 20
	AUDIO_VOL_21,               ///< Volume 21
	AUDIO_VOL_22,               ///< Volume 22
	AUDIO_VOL_23,               ///< Volume 23
	AUDIO_VOL_24,               ///< Volume 24
	AUDIO_VOL_25,               ///< Volume 25
	AUDIO_VOL_26,               ///< Volume 26
	AUDIO_VOL_27,               ///< Volume 27
	AUDIO_VOL_28,               ///< Volume 28
	AUDIO_VOL_29,               ///< Volume 29
	AUDIO_VOL_30,               ///< Volume 30
	AUDIO_VOL_31,               ///< Volume 31
	AUDIO_VOL_32,               ///< Volume 32
	AUDIO_VOL_33,               ///< Volume 33
	AUDIO_VOL_34,               ///< Volume 34
	AUDIO_VOL_35,               ///< Volume 35
	AUDIO_VOL_36,               ///< Volume 36
	AUDIO_VOL_37,               ///< Volume 37
	AUDIO_VOL_38,               ///< Volume 38
	AUDIO_VOL_39,               ///< Volume 39
	AUDIO_VOL_40,               ///< Volume 40
	AUDIO_VOL_41,               ///< Volume 41
	AUDIO_VOL_42,               ///< Volume 42
	AUDIO_VOL_43,               ///< Volume 43
	AUDIO_VOL_44,               ///< Volume 44
	AUDIO_VOL_45,               ///< Volume 45
	AUDIO_VOL_46,               ///< Volume 46
	AUDIO_VOL_47,               ///< Volume 47
	AUDIO_VOL_48,               ///< Volume 48
	AUDIO_VOL_49,               ///< Volume 49
	AUDIO_VOL_50,               ///< Volume 50
	AUDIO_VOL_51,               ///< Volume 51
	AUDIO_VOL_52,               ///< Volume 52
	AUDIO_VOL_53,               ///< Volume 53
	AUDIO_VOL_54,               ///< Volume 54
	AUDIO_VOL_55,               ///< Volume 55
	AUDIO_VOL_56,               ///< Volume 56
	AUDIO_VOL_57,               ///< Volume 57
	AUDIO_VOL_58,               ///< Volume 58
	AUDIO_VOL_59,               ///< Volume 59
	AUDIO_VOL_60,               ///< Volume 60
	AUDIO_VOL_61,               ///< Volume 61
	AUDIO_VOL_62,               ///< Volume 62
	AUDIO_VOL_63,               ///< Volume 63

	ENUM_DUMMY4WORD(AUDIO_VOL)
} AUDIO_VOL;

/**
    AUDIO Record Gain

    Set record sound volume (ALC target level).

    @note For aud_set_gain()
*/
typedef enum {
	AUDIO_GAIN_MUTE,            ///< Gain mute
	AUDIO_GAIN_0,               ///< Gain 0. (ALC target level to -27.0 dBFS)
	AUDIO_GAIN_1,               ///< Gain 1. (ALC target level to -24.0 dBFS)
	AUDIO_GAIN_2,               ///< Gain 2. (ALC target level to -21.0 dBFS) (Default)
	AUDIO_GAIN_3,               ///< Gain 3. (ALC target level to -18.0 dBFS)
	AUDIO_GAIN_4,               ///< Gain 4. (ALC target level to -15.0 dBFS)
	AUDIO_GAIN_5,               ///< Gain 5. (ALC target level to -12.0 dBFS)
	AUDIO_GAIN_6,               ///< Gain 6. (ALC target level to -9.0 dBFS)
	AUDIO_GAIN_7,               ///< Gain 7. (ALC target level to -6.0 dBFS)

	ENUM_DUMMY4WORD(AUDIO_GAIN)
} AUDIO_GAIN;

/**
    AUDIO Codec Feature

    Set Audio codec global feature ENABLE/DSIABLE.
    Please also refer to the audio driver application note for audio tuning guide introduction.

    @note For aud_set_feature()
*/
typedef enum {
	/*
	    Record Related Features
	*/
	AUDIO_FEATURE_ALC,                      ///< [For default codec] Auto level control (Recording path)
											///<   Default is Enabled after aud_set_default_setting(AUDIO_DEFSET) invoked.
	AUDIO_FEATURE_NOISEGATE_EN,             ///< [For default codec] Audio Record Noise Gate function enable/disable.
											///<   This setting is valid only if AUDIO_FEATURE_ALC is enabled.
											///<   Default is Enabled after aud_set_default_setting(AUDIO_DEFSET) invoked.
	AUDIO_FEATURE_NG_WITH_MICBOOST,         ///< [For default codec] Noise gate function with microphone boost (internal or external) (Default disabled)
	AUDIO_FEATURE_MICBOOST,                 ///< [For default codec] Microphone boost (Default disabled)
	AUDIO_FEATURE_CHECK_PLAY_SAMPLING_RATE, ///< [for default/HDMI codec] Check sampling rate (AUDIO_PARAMETER_CHECKED_PLAY_SAMPLING_RATE) in playback mode. (Default disabled)
	AUDIO_FEATURE_ALC_IIR_EN,               ///< [For default codec] Second Order IIR filter for the ALC function. (Default Disabled)
	AUDIO_FEATURE_OUTPUT_IIR_EN,            ///< [For default codec] Second Order IIR filter for the Recorded Output Data. (Default Disabled)

	/*
	    Playback Related Features
	*/
	AUDIO_FEATURE_CLASSD_OUT_EN,            ///< [For default codec] No support for NT96680.
	AUDIO_FEATURE_PLAY2ALLCODEC,            ///< [For common audio] Playback to all codec (Default disabled)
	AUDIO_FEATURE_INTERFACE_ALWAYS_ACTIVE,  ///< [For common audio] Audio interface always active (Default disabled)
	AUDIO_FEATURE_DISCONNECT_HDMI,          ///< [For common audio] Disconnect HDMI from DAI (Default is disabled)
	AUDIO_FEATURE_SPK_PWR_ALWAYSON,         ///< [For default codec] Speaker power always on (Default disabled)
	AUDIO_FEATURE_HP_PWR_ALWAYSON,          ///< [For default codec] Head Phone power always on (Default enabled)
	AUDIO_FEATURE_LINE_PWR_ALWAYSON,        ///< [For default codec] LineOut power always on (Default enabled)

	AUDIO_FEATURE_HP_DEPOP_EN,              ///< [For default codec] Headphone path Depop function enable/disable. This is valid only if HP power always ON. The Default value is DISABLE.
	AUDIO_FEATURE_LINE_DEPOP_EN,            ///< [For default codec] Lineout path Depop function enable/disable. This is valid only if Lineout power always ON. The Default value is DISABLE.

	/*
	    MISC Features
	*/
	AUDIO_FEATURE_AUTOPINMUX,               ///< [For common audio] Control I2S pinmux (except MCLK) when aud_open()/aud_close() (Default disabled)
	AUDIO_FEATURE_MCLK_AUTOPINMUX,          ///< [For common audio] Control I2S MCLK pinmux when aud_open()/aud_close() (Default disabled)

	// For emulation
	AUDIO_FEATURE_DEBUG_MODE_EN,            //   debug mode (Default disabled)

	// Backward compatible
	AUDIO_FEATURE_SWDISCHARGE_EN,           //   Obselete. No Supported in current chip version.

	AUDIO_FEATURE_PDVCMBIAS_ALWAYSON,       ///< [For default codec] Let ADVCM always power-on for faster Mic stable time. [Default disabled]

	ENUM_DUMMY4WORD(AUDIO_FEATURE)
} AUDIO_FEATURE;




//
//  Audio Set Parameter Related Definitions
//

/**
    AUDIO Codec Parameter

    This definition is used in aud_set_parameter() to assign new parameters to specified function.

    @note For aud_set_parameter()
*/
typedef enum {
	AUDIO_PARAMETER_NOISEGAIN,                  ///< [For default codec] Noise gain. (Valid value: 0x0~0xF)\n
												///<   The Noise gate Target Level = ALC_LEVEL - (NOISE_GATE_THRESHOLD - SIGNAL_LEVEL)*(1 + uiSetting)
	AUDIO_PARAMETER_NOISETHD_WITH_BOOST,        ///< [For default codec] NoiseGate threshold when boost is Enabled. Please use the enumeration "EAC_NG_THRESHOLD" as input parameter uiSetting for embedded audio codec.
	AUDIO_PARAMETER_NOISETHD_WITHOUT_BOOST,     ///< [For default codec] NoiseGate threshold when boost is Disabled. Please use the enumeration "EAC_NG_THRESHOLD" as input parameter uiSetting for embedded audio codec.
	AUDIO_PARAMETER_NOISEGATE_TIME_RESOLUTION,  ///< [For default codec] ALC Time Resolution for NoiseGate. Please use "EAC_ALC_TRESO_BASIS" as input paramter. The default value is EAC_ALC_TRESO_BASIS_10000US.
	AUDIO_PARAMETER_ALC_MAXGAIN,                ///< [For default codec] ALC max gain. Please use the enumeration "EAC_ALC_MAXGAIN" as input parameter uiSetting for embedded audio codec.
	AUDIO_PARAMETER_ALC_MINGAIN,                ///< [For default codec] ALC minimum gain. Please use the enumeration "EAC_ALC_MINGAIN" as input parameter uiSetting for embedded audio codec.
	AUDIO_PARAMETER_ALC_ATTACK_TIME,            ///< [For default codec] ALC Attack Time length. Please reference to eac.h for EAC_CONFIG_AD_ALC_ATTACK_TIME detailed desceiptions of the input value.
	AUDIO_PARAMETER_ALC_DECAY_TIME,             ///< [For default codec] ALC Decay Time length. Please reference to eac.h for EAC_CONFIG_AD_ALC_DECAY_TIME detailed desceiptions of the input value.
	AUDIO_PARAMETER_ALC_HOLD_TIME,              ///< [For default codec] ALC Hold Time length. Please reference to eac.h for EAC_CONFIG_AD_ALC_HOLD_TIME detailed desceiptions of the input value.
	AUDIO_PARAMETER_ALC_TIME_RESOLUTION,        ///< [For default codec] ALC Basis Time Resolution for Attack/Decay Time Calculation. Default value is 0x2FFFFF.
	AUDIO_PARAMETER_CHECKED_PLAY_SAMPLING_RATE, ///< [For common audio] Check playback sampling rate when AUDIO_FEATURE_CHECK_PLAY_SAMPLING_RATE is enabled.
												///<   (Valid setting is AUDIO_SR) (Default is AUDIO_SR_32000)
	AUDIO_PARAMETER_BOOST_GAIN,                 ///< [For default codec] Recording PGA gain select. This would be activate when setting feature AUDIO_FEATURE_MICBOOST enabled.
												///<   Please use EAC_PGABOOST_SEL_20DB/ EAC_PGABOOST_SEL_30DB/ EAC_PGABOOST_SEL_10DB as input parameter uiSetting.
	AUDIO_PARAMETER_RECORD_DIGITAL_GAIN,        ///< [For default codec] Recording Digital Gain. Value range 0x1~0xFF which mapping to -97.0dB ~ +30.0dB(+0.5dB each step). Value 0x0 is mute.
	AUDIO_PARAMETER_DCCAN_RESOLUTION,           ///< [For default codec] Recording DC Cancellation Resolution. Please use the enumeration "EAC_DCCAN_RESO" as input parameter uiSetting for embedded audio codec.
	AUDIO_PARAMETER_RECORD_DELAY,               ///< [For default codec] After record start(aud_record), the MIC_BAIS would be enabled and the external regulator needs some stable time before record really start.
												///<   This delay is used to prevent recording this pop noise. The default value is 200 (mini-seconds). This setting unit is mini-seconds.
	AUDIO_PARAMETER_MAXMINGAIN_OFS,             ///< [For default codec] This paramter can adjust the ALC Max/Min Gain by +2.5 to -3dB simutaneously with 0.5dB per step.
												///<   Please use "EAC_DGAIN" as input parameter. Only +2.5 to -3dB is allowed in current implementation.
	AUDIO_PARAMETER_DAGAIN,                     ///< [For default codec] This parameter can adjust the embedded audio codec DA default digital gain.
												///<   The Value range 0x1~0xFF which mapping to -97.0dB ~ +30.0dB(+0.5dB each step). Value 0x0 is mute. 0dB is 0xC3.
	AUDIO_PARAMETER_ALCNG_ATTACK_TIME,          ///< [For default codec] ALC Noise Gate Attack Time length. Please reference to eac.h for EAC_CONFIG_AD_NG_ATTACK_TIME detailed desceiptions of the input value.
	AUDIO_PARAMETER_ALCNG_DECAY_TIME,           ///< [For default codec] ALC Noise Gate Decay Time length. Please reference to eac.h for EAC_CONFIG_AD_NG_DECAY_TIME detailed desceiptions of the input value.
	AUDIO_PARAMETER_IIRCOEF_L,                  ///< [For default codec]  Left Channel Embedded Second Order IIR filter Coeficients. Please use the address of  PAUDIO_IIRCOEF as input parameter.
	AUDIO_PARAMETER_IIRCOEF_R,                  ///< [For default codec] Right Channel Embedded Second Order IIR filter Coeficients. Please use the address of  PAUDIO_IIRCOEF as input parameter.

	AUDIO_PARAMETER_DBG_MSG_LEVEL,              ///< [For common audio]Debug message output select
												///<   - @b AUDIO_MSG_LVL_0 (Default): only output error message
												///<   - @b AUDIO_MSG_LVL_1: output message when API is invoked
	// For emulation
	AUDIO_PARAMETER_DEBUG_BUF_ADDR,             //< Debug buffer DRAM address
	AUDIO_PARAMETER_DEBUG_BUF_SIZE,             //< Debug buffer size

	AUDIO_PARAMETER_PLAYDGAIN,                  //  Obselete. Backward compatible usage. Please use "AUDIO_PARAMETER_DAGAIN" instead.
	ENUM_DUMMY4WORD(AUDIO_PARAMETER)
} AUDIO_PARAMETER;

/**
    Add Audio playback additional Digital Gain

    This is used in aud_set_parameter(AUDIO_PARAMETER_DAGAIN) to assign additional playback Digital Gain.
*/
typedef enum {
	AUDIO_DAGAIN_0DB        =  0xC3,    ///< Do not add any additional digital gain to audio playback. (Default)

	AUDIO_DAGAIN_P1P5DB     =  0xC6,    ///< Add additional  +1.5dB digital gain to audio playback.
	AUDIO_DAGAIN_P3DB       =  0xC9,    ///< Add additional  +3.0dB digital gain to audio playback.
	AUDIO_DAGAIN_P4P5DB     =  0xCC,    ///< Add additional  +4.5dB digital gain to audio playback.
	AUDIO_DAGAIN_P6DB       =  0xCF,    ///< Add additional  +6.0dB digital gain to audio playback.
	AUDIO_DAGAIN_P7P5DB     =  0xD2,    ///< Add additional  +7.5dB digital gain to audio playback.
	AUDIO_DAGAIN_P9DB       =  0xD5,    ///< Add additional  +9.0dB digital gain to audio playback.
	AUDIO_DAGAIN_P10P5DB    =  0xD8,    ///< Add additional +10.5dB digital gain to audio playback.
	AUDIO_DAGAIN_P12DB      =  0xDB,    ///< Add additional +12.0dB digital gain to audio playback.
	AUDIO_DAGAIN_P13P5DB    =  0xDE,    ///< Add additional +13.5dB digital gain to audio playback.
	AUDIO_DAGAIN_P15DB      =  0xE1,    ///< Add additional +15.0dB digital gain to audio playback.
	AUDIO_DAGAIN_P16P6DB    =  0xE4,    ///< Add additional +16.5dB digital gain to audio playback.
	AUDIO_DAGAIN_P18DB      =  0xE7,    ///< Add additional +18.0dB digital gain to audio playback.

	ENUM_DUMMY4WORD(AUDIO_DAGAIN)
} AUDIO_DAGAIN;

/**
    AUDIO Message Select

    @note For aud_set_parameter(AUDIO_PARAMETER_DBG_MSG_LEVEL)
*/
typedef enum {
	AUDIO_MSG_LVL_0,            ///< Only output message when error is detected
	AUDIO_MSG_LVL_1,            ///< Output all debug messages

	ENUM_DUMMY4WORD(AUDIO_MSG_LVL)
} AUDIO_MSG_LVL;

/**
    Record Embedded Second Order IIR Filter Coeficients

    The second order IIR equation is H(z) =  SectionGain x ((B0 + B1*Z1 + B2*Z2) / (A0 + A1*Z1 + A2*Z2)) x (Total-Gain).
    This can be used to implement audio notch/lowpass/highpass filter as needed.

    @note For aud_set_parameter(AUDIO_PARAMETER_IIRCOEF_L / AUDIO_PARAMETER_IIRCOEF_R, PAUDIO_IIRCOEF).
*/
typedef struct {
	INT32   fTotalGain;     ///< Total Gain
	INT32   fSectionGain;   ///< Section Gain

	INT32   fCoefB0;        ///< Filter coefficient B0
	INT32   fCoefB1;        ///< Filter coefficient B1
	INT32   fCoefB2;        ///< Filter coefficient B2

	INT32   fCoefA0;        ///< Filter coefficient A0
	INT32   fCoefA1;        ///< Filter coefficient A1
	INT32   fCoefA2;        ///< Filter coefficient A2
} AUDIO_IIRCOEF, *PAUDIO_IIRCOEF;

/**
    Audio Default Settings Select

    This is used at aud_set_default_setting() to select audio default configurations.
    Different characteristics for each select option is shown as below.
*/
typedef enum {
	AUDIO_DEFSET_10DB,     	    ///< Boost Gain 10dB. NoiseGate-Enable. ALC-Range(+1.5 ~ +25.5dB).
								///< Atk=0.08ms. Dcy=0.16s. NG-Atk=0.72s. NG-Dcy=0.36s. NG-Target=2.
								///< NoiseGate-TH= -67.5-10=-77.5dB from MIC input.
	AUDIO_DEFSET_10DB_HP_8K,    ///< The Same as AUDIO_DEFSET_10DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR8KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_10DB_HP_16K,   ///< The Same as AUDIO_DEFSET_10DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR16KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_10DB_HP_32K,   ///< The Same as AUDIO_DEFSET_10DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR32KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_10DB_HP_48K,   ///< The Same as AUDIO_DEFSET_10DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR48KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_10DB_HP = AUDIO_DEFSET_10DB_HP_48K,// for backward compatible
	AUDIO_DEFSET_20DB,      	///< Boost Gain 20dB. NoiseGate-Enable. ALC-Range(-9 ~ +21dB).
								///< Atk=0.08ms. Dcy=0.16s. NG-Atk=0.72s. NG-Dcy=0.36s. NG-Target=1.
								///< NoiseGate-TH= -58.5-20=-78.5dB from MIC input.
	AUDIO_DEFSET_20DB_HP_8K,    ///< The Same as AUDIO_DEFSET_20DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR8KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_20DB_HP_16K,   ///< The Same as AUDIO_DEFSET_20DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR16KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_20DB_HP_32K,   ///< The Same as AUDIO_DEFSET_20DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR32KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_20DB_HP_48K,   ///< The Same as AUDIO_DEFSET_20DB but with HighPass-Filter Enabled.
	AUDIO_DEFSET_20DB_HP = AUDIO_DEFSET_20DB_HP_48K,// for backward compatible
								///< HighPass Filter 3dB is about 200Hz for SR48KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_30DB,      	///< Boost Gain 30dB. NoiseGate-Enable. ALC-Range(-9 ~ +13.5dB).
								///< Atk=0.08ms. Dcy=0.16s. NG-Atk=2.88s. NG-Dcy=0.36s. NG-Target=1.
								///< NoiseGate-TH= -40-30=-70dB from MIC input.
	AUDIO_DEFSET_30DB_HP_8K,    ///< The Same as AUDIO_DEFSET_30DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR8KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_30DB_HP_16K,   ///< The Same as AUDIO_DEFSET_30DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR16KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_30DB_HP_32K,   ///< The Same as AUDIO_DEFSET_30DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR32KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_30DB_HP_48K,   ///< The Same as AUDIO_DEFSET_30DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR48KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_30DB_HP = AUDIO_DEFSET_30DB_HP_48K,// for backward compatible

    AUDIO_DEFSET_0DB,      		///< Boost Gain 30dB. NoiseGate-Enable. ALC-Range(-9 ~ +13.5dB).
								///< Atk=0.08ms. Dcy=0.16s. NG-Atk=2.88s. NG-Dcy=0.36s. NG-Target=1.
								///< NoiseGate-TH= -40-30=-70dB from MIC input.
	AUDIO_DEFSET_0DB_HP_8K,   	///< The Same as AUDIO_DEFSET_0DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR8KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_0DB_HP_16K,    ///< The Same as AUDIO_DEFSET_0DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR16KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_0DB_HP_32K,    ///< The Same as AUDIO_DEFSET_0DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR32KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_0DB_HP_48K,    ///< The Same as AUDIO_DEFSET_0DB but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR48KHz. Acting as Wind-Noise suppress filter.

	AUDIO_DEFSET_ALCOFF,        ///< Boost Gain 30dB. NoiseGate-Enable. ALC-Range(-9 ~ +13.5dB).
								///< Atk=0.08ms. Dcy=0.16s. NG-Atk=2.88s. NG-Dcy=0.36s. NG-Target=1.
								///< NoiseGate-TH= -40-30=-70dB from MIC input.
	AUDIO_DEFSET_ALCOFF_HP_8K,  ///< The Same as AUDIO_DEFSET_ALCOFF but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR8KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_ALCOFF_HP_16K, ///< The Same as AUDIO_DEFSET_ALCOFF but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR16KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_ALCOFF_HP_32K, ///< The Same as AUDIO_DEFSET_ALCOFF but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR32KHz. Acting as Wind-Noise suppress filter.
	AUDIO_DEFSET_ALCOFF_HP_48K, ///< The Same as AUDIO_DEFSET_ALCOFF but with HighPass-Filter Enabled.
								///< HighPass Filter 3dB is about 200Hz for SR48KHz. Acting as Wind-Noise suppress filter.

	AUDIO_DEFSET_DMIC,          ///< Default setting for Digital mic.
								///< Atk=0.08ms. Dcy=0.16s. NG-Atk=2.88s. NG-Dcy=0.36s. NG-Target=1.
								///< NoiseGate-TH= -40-30=-70dB from MIC input.
	AUDIO_DEFSET_DMIC_LP_8K,  	///< The Same as AUDIO_DEFSET_DMIC but with HighPass-Filter Enabled.
								///< Low Pass Filter 3dB is about 2000Hz for SR8KHz.
	AUDIO_DEFSET_DMIC_LP_16K, 	///< The Same as AUDIO_DEFSET_DMIC but with HighPass-Filter Enabled.
								///< Low Pass Filter 3dB is about 4500Hz for SR16KHz.
	AUDIO_DEFSET_DMIC_LP_32K, 	///< The Same as AUDIO_DEFSET_DMIC but with HighPass-Filter Enabled.
								///< Low Pass Filter 3dB is about 8000Hz for SR32KHz.
	AUDIO_DEFSET_DMIC_LP_48K, 	///< The Same as AUDIO_DEFSET_DMIC but with HighPass-Filter Enabled.
								///< Low Pass Filter 3dB is about 12000Hz for SR48KHz. 	

	AUDIO_DEFSET_MAX,
	ENUM_DUMMY4WORD(AUDIO_DEFSET)
} AUDIO_DEFSET;



/**
    Audio re-sampling information

    This struct is used in transceive object function "setResampleInfo(PAUDIO_RESAMPLE_INFO pResampleInfo)" to specify
    the playback TX1/TX2 re-sample information. We must note that the TX1's and TX2's target sampling rate(AudioTargetSR)
    must be the same.
*/
typedef struct {
	BOOL            bResample;          ///< Enable/Disable playback re-sampling function
										///< - @b TRUE: enable re-sampling
										///< - @b FALSE: disable re-sampling
	AUDIO_SR        AudioTargetSR;      ///< Target Sampling rate after re-sampling (i.e. actual sampling rate on DAI)

	UINT32          uiDstBufAddr;       ///< buffer address for re-sampling (unit: byte) (Word alignment)
	UINT32          uiDstBufSize;       ///< buffer size for re-sampling, Size must be (target-sampling-rate * 4 * 4) bytes
} AUDIO_RESAMPLE_INFO, *PAUDIO_RESAMPLE_INFO;

/**
    AUDIO Buffer Queue

    This struct is used in transceive object function "addBufferToQueue(PAUDIO_BUF_QUEUE)"/
    "getDoneBufferFromQueue()"/"getCurrentBufferFromQueue()" to add or get audio buffer to audio driver queue.
*/
typedef struct _AUDIO_BUF_QUEUE {
	UINT32                  uiAddress;  ///< Buffer Starting Address (Unit: byte) (Word alignment)
	UINT32                  uiSize;     ///< Buffer Size (Unit: byte) (Word Alignment)
	UINT32                  uiValidSize;///< Valid PCM data size (Unit: byte).
										///< Returend by aud_getDoneBufferFromQueue()
	struct _AUDIO_BUF_QUEUE *pNext;     ///< Next queue element
} AUDIO_BUF_QUEUE, *PAUDIO_BUF_QUEUE;

/**
    Audio Event Handler Prototype

    uiEvent is bitwise event, using AUDIO_EVENT to identidy.
*/
//@{
typedef void (*AUDIO_CB)(UINT32 uiEvent);               ///< uiEvent is refered to AUDIO_EVENT.
//@}

/**
    Audio Samples' bit length select

    This is used in AUDTS_OBJ.setConfig(AUDTS_CFG_ID_PCM_BITLEN) to change PCM data bit-length.
    Default is AUDIO_PCMLEN_16BITS for each transceive object.
*/
typedef enum {
	AUDIO_PCMLEN_8BITS,     ///< Audio Samples are PCM8
	AUDIO_PCMLEN_16BITS,    ///< Audio Samples are PCM16
	AUDIO_PCMLEN_32BITS,    ///< Audio Samples are PCM32

	ENUM_DUMMY4WORD(AUDIO_PCMLEN)
} AUDIO_PCMLEN;

/**
    Audio Event

    This is used in audio event callback(AUDIO_CB) identifications.
*/
typedef enum {
	AUDIO_EVENT_TCLATCH             =   0x00000001,     ///< Time code latch event
	AUDIO_EVENT_TCHIT               =   0x00000002,     ///< Time code hit event
	AUDIO_EVENT_DMADONE             =   0x00000010,     ///< DMA finish transfering one buffer event
	AUDIO_EVENT_BUF_FULL            =   0x00000020,     ///< Buffer queue full event (in record mode)
	AUDIO_EVENT_BUF_EMPTY           =   0x00000040,     ///< Buffer queue empty event (in playback mode)
	AUDIO_EVENT_DONEBUF_FULL        =   0x00000080,     ///< Done buffer queue full event
	AUDIO_EVENT_FIFO_ERROR          =   0x00000100,     ///< FIFO Error
	AUDIO_EVENT_BUF_RTEMPTY         =   0x00000200,     ///< Buffer queue runtime empty event (in playback mode)

	AUDIO_EVENT_DMADONE2            =   0x00010000,     ///< DMA finish transfering one buffer event.
														///< Valid for Dual Mono Record mode only.
	AUDIO_EVENT_BUF_FULL2           =   0x00020000,     ///< Buffer queue full event (in record mode)
														///< Valid for Dual Mono Record mode only.
	AUDIO_EVENT_DONEBUF_FULL2       =   0x00040000,     ///< Done buffer queue full event
														///< Valid for Dual Mono Record mode only.
	AUDIO_EVENT_FIFO_ERROR2         =   0x00080000,     ///< FIFO Error. Valid for Dual Mono Record mode only.

	ENUM_DUMMY4WORD(AUDIO_EVENT)
} AUDIO_EVENT;

/**
    Audio Buffer Queue Select

    @note For AUDTS_OBJ .resetBufferQueue()/.addBufferToQueue()/.isBufferQueueFull()/.getDoneBufferFromQueue()/.getCurrentBufferFromQueue()
*/
typedef enum {
	AUDIO_QUEUE_SEL_DEFAULT             = 0,    ///< Audio Buffer Queue Default channel,
												///< which is used for playback/record buffer except record dual mono.
	AUDIO_QUEUE_SEL_DUALMONO_RECORDCH0  = 0,    ///< Audio Record Dual Mono Buffer Queue Channel 0
	AUDIO_QUEUE_SEL_DUALMONO_RECORDCH1  = 1,    ///< Audio Record Dual Mono Buffer Queue Channel 1

	ENUM_DUMMY4WORD(AUDIO_QUEUE_SEL)
} AUDIO_QUEUE_SEL;


/**
    Audio Transceive Channels Control Object

    The Audio Transceive Object (AUDTS_OBJ) is got from aud_get_transceive_object(AUDTS_CH).
    After AUDTS_OBJ is got, the user shall call AUDTS_OBJ.open() first to open access-key of the object.
    After playback or record is done, the user must call AUDTS_OBJ.stop() and AUDTS_OBJ.close() to close the
    object access. Please also refer to audio driver application note for usage sample code.

    Audio Transmit/Receive Channels Control Object
*/
typedef struct {
	AUDTS_CH            AudTSCH;                ///< Audio Transceive Channel ID.

	ER(*open)(void);                            ///< Open Audio Transceive object.
	ER(*close)(void);                           ///< Close Audio Transceive object.
	BOOL (*isOpened)(void);                     ///< Check if Audio Transceive object is opened or not.
	BOOL (*isBusy)(void);                       ///< Check if Audio Transceive object is Busy or not.

	ER(*playback)(void);                        ///< Start playback. Valid for AUDTS_CH_TX1/AUDTS_CH_TX2 object only.
	ER(*recordPreset)(void);                    ///< Start record pre-set. Valid for AUDTS_CH_RX object only.
	ER(*record)(void);                          ///< Start record. Valid for AUDTS_CH_RX object only.
	ER(*stop)(void);                            ///< Stop playback/record operation.
	ER(*pause)(void);                           ///< Pause playback/record operation.
	ER(*resume)(void);                          ///< Resume playback/record operation after pause.

	void (*setChannel)(AUDIO_CH Channel);                                                               ///< Set Audio Channel as Left/Right/Stereo.
	void (*setTdmChannel)(AUDIO_TDMCH TDM);                                                             ///< Set Audio Channel as
	ER(*setSamplingRate)(AUDIO_SR SampleRate);                                                          ///< Set Audio Samplerate.
	BOOL (*setResampleInfo)(PAUDIO_RESAMPLE_INFO pResampleInfo);                                        ///< Set playback resample. Valid for Playback(TX1/TX2) Channel Only.
	void (*setFeature)(AUDTS_FEATURE Feature, BOOL bEnable);                                            ///< Set Audio Feature.
	void (*setConfig)(AUDTS_CFG_ID CfgSel, UINT32 uiSetting);                                           ///< Set Audio Configuration.
	UINT32(*getConfig)(AUDTS_CFG_ID CfgSel);                                                            ///< Get Audio Configuration.

	void (*resetBufferQueue)(AUDIO_QUEUE_SEL QueueSel);                                                 ///< Reset Audio Buffer Queue
	BOOL (*addBufferToQueue)(AUDIO_QUEUE_SEL QueueSel, PAUDIO_BUF_QUEUE pAudioBufQueue);                ///< Add playback/record Buffer to Audio Buffer Queue
	BOOL (*isBufferQueueFull)(AUDIO_QUEUE_SEL QueueSel);                                                ///< Check if Audio Buffer Queue is full or not.
	PAUDIO_BUF_QUEUE(*getDoneBufferFromQueue)(AUDIO_QUEUE_SEL QueueSel);                                ///< Get Record Done Buffer from Audio Buffer Queue.
	PAUDIO_BUF_QUEUE(*getCurrentBufferFromQueue)(AUDIO_QUEUE_SEL QueueSel);                             ///< Get Record Current Buffer from Audio Buffer Queue. May not finished.

} AUDTS_OBJ, *PAUDTS_OBJ;







//
//  External/Internal Codec Related function table Definitions
//

/**
    AUDIO I2S Control

    This is used for external audio codec register control interface.
    Useless for embedded audio codec.

    @note For AUDIO_DEVICE_OBJ
*/
typedef enum {
	AUDIO_I2SCTRL_SIF,          ///< Use SIF to control I2S codec
	AUDIO_I2SCTRL_I2C,          ///< Use I2C to control I2S codec
	AUDIO_I2SCTRL_GPIO_SIF,     ///< Use GPIO to simulate SIF to control I2S codec
	AUDIO_I2SCTRL_GPIO_I2C,     ///< Use GPIO to simulate I2C to control I2S codec
	AUDIO_I2SCTRL_NONE,         ///< Not a I2S codec

	ENUM_DUMMY4WORD(AUDIO_I2SCTRL)
} AUDIO_I2SCTRL;

/**
    AUDIO Codec Type

    This is use in AUDIO_CODEC_FUNC as the codec content ID.

    @note For codecType of AUDIO_CODEC_FUNC
*/
typedef enum {
	AUDIO_CODEC_TYPE_EMBEDDED,          ///< Embedded audio codec
	AUDIO_CODEC_TYPE_HDMI,              ///< HDMI audio codec
	AUDIO_CODEC_TYPE_EXTERNAL,          ///< External audio codec connected with I2S

	ENUM_DUMMY4WORD(AUDIO_CODEC_TYPE)
} AUDIO_CODEC_TYPE;

/**
    AUDIO Device Control Object

    Set external codec device control interface method and pins source.
    Used for aud_set_device_object(), and aud_get_device_object().
    Useless for embedded audio codec.

*/
typedef struct {
	UINT32          uiGPIOColdReset;                    ///< GPIO pin number of AC97 Cold Reset pin
	AUDIO_I2SCTRL   uiI2SCtrl;                          ///< I2S Control Interface
	UINT32          uiChannel;                          ///< SIF channel number to control I2S codec, only valid when uiI2SCtrl == AUDIO_I2SCTRL_SIF, ADC channel for ADC recording
	UINT32          uiGPIOData;                         ///< GPIO pin number for SIF DATA / I2C SDA, only valid when uiI2SCtrl == AUDIO_I2SCTRL_GPIO_SIF or AUDIO_I2SCTRL_GPIO_I2C
	UINT32          uiGPIOClk;                          ///< GPIO pin number for SIF CLK / I2C SCL, only valid when uiI2SCtrl == AUDIO_I2SCTRL_GPIO_SIF or AUDIO_I2SCTRL_GPIO_I2C
	UINT32          uiGPIOCS;                           ///< GPIO pin number for SIF CS, only valid when uiI2SCtrl == AUDIO_I2SCTRL_GPIO_SIF
	UINT32          uiADCZero;                          ///< ADC value of PCM data = 0

} AUDIO_DEVICE_OBJ, *PAUDIO_DEVICE_OBJ;


/**
    Audio CODEC function table

    Function table exported from audio codec to audio library.
    Users don't need to care contents/member of this structure.
    And should not use the member directly.

    @note For audExtCodec_getFP(), aud_set_extcodec()
*/
typedef struct {
	AUDIO_CODEC_TYPE    codecType;

	void (*setDevObj)(PAUDIO_DEVICE_OBJ pAudDevObj);
	void (*init)(PAUDIO_SETTING pAudio);
	void (*setRecordSource)(AUDIO_RECSRC RecSrc);
	void (*setOutput)(AUDIO_OUTPUT Output);
	void (*setSamplingRate)(AUDIO_SR SamplingRate);
	void (*setTxChannel)(AUDIO_CH Channel);
	void (*setRxChannel)(AUDIO_CH Channel);
	void (*setVolume)(AUDIO_VOL Vol);
	void (*setGain)(AUDIO_GAIN Gain);
	void (*setGainDB)(INT32 db);
	void (*setEffect)(AUDIO_EFFECT Effect);
	BOOL (*setFeature)(AUDIO_FEATURE Feature, BOOL bEnable);
	void (*stopRecord)(void);
	void (*stopPlay)(void);
	void (*recordPreset)(void);
	void (*record)(void);
	void (*playback)(void);
	void (*setFormat)(AUDIO_I2SFMT I2SFmt);
	void (*setClockRatio)(AUDIO_I2SCLKR I2SCLKRatio);
	BOOL (*sendCommand)(UINT32 uiReg, UINT32 uiData);
	BOOL (*readData)(UINT32 uiReg, UINT32 *puiData);
	BOOL (*setParameter)(AUDIO_PARAMETER Parameter, UINT32 uiSetting);
	BOOL (*chkSamplingRate)(void);
	void (*open)(void);
	void (*close)(void);
} AUDIO_CODEC_FUNC, *PAUDIO_CODEC_FUNC;


//
// Audio Export APIs
//

/**
    Initialize AUDIO driver

    Initialize the AUDIO driver based on parameter.
    This function should be call once after power on.

    @param[in] pAudio       Audio setting pointer

    @return void
*/
extern void             aud_init(PAUDIO_SETTING pAudio);

/**
    Open AUDIO driver

    This function should be called before calling any other functions,
    except the following:
    (1) aud_get_device_object()
    (2) aud_get_lock_status()
    (3) aud_init()

    If you want to use GPIO to control I2S codec,
    you should set pDevObj->uiI2SCtrl to AUDIO_I2SCTRL_GPIO_SIF or AUDIO_I2SCTRL_GPIO_I2C,
    and set pDevObj->uiGPIOData, pDevObj->uiGPIOClk, pDevObj->uiGPIOCS to correct GPIO pin number.
    Don't forget to change pinmux of those pins to GPIO by your application.

    @return
        - @b E_OK: open success
        - @b Else: open fail
*/
extern ER               aud_open(void);

/**
    Close AUDIO driver

    Release AUDIO driver and let other application use AUDIO

    @return
        - @b E_OK: close success
        - @b Else: close fail
*/
extern ER               aud_close(void);

/**
    Get the lock status of AUDIO

    This function return the lock status of AUDIO. Return value is as following:
        NO_TASK_LOCKED  :AUDIO is free, no application is using AUDIO
        TASK_LOCKED     :AUDIO is locked by some application

    @return
        - @b FALSE: AUDIO is free
        - @b TRUE: AUDIO is locked by some application
*/
extern BOOL             aud_get_lock_status(void);

/**
    Check whether the Audio module is busy recording/playbacking or not

    This function check whether the Audio module is busy recording/playbacking or not.

    @return audio module busy state:
        - @b TRUE: Busy recording/playbacking
        - @b FALSE: idle or wait for VD from SIE to start recording
*/
extern BOOL             aud_is_busy(void);

/**
    Get Audio TranSceive Object

    Get Audio TranSceive Object. This object is used to control the audio playback/record.

    @param[in] TsCH     TranSceive Object ID.

    @return PAUDTS_OBJ The transceive object pointer.
*/
extern PAUDTS_OBJ       aud_get_transceive_object(AUDTS_CH TsCH);

/**
    Set Audio Record/Playback default settings

    This api is used to configure the audio record/playback default configurations.
    This must be called after aud_open().

    @param[in] DefSet  Default setting select. Please refer to AUDIO_DEFSET for details.

    @return E_OK: configuration done.
*/
extern ER               aud_set_default_setting(AUDIO_DEFSET DefSet);

/**
    Set extended audio codec

    Set extended audio codec. This function should be called once before aud_init().
    If you need to use external audio codec instead of internal audio codec, you should invoke this function to install external audio codec.
    For example, following sequence can install external audio codec:
    (1) audExtCodec_getFP(&gExtCodecFunc);
    (2) aud_set_extcodec(&gExtCodecFunc);

    @param[in] pAudioCodecFunc      Audio codec function pointer
      - @b NULL: uninstall external codec (instead use embedded codec)
      - @b Else: install external codec
    @return void
*/
extern void             aud_set_extcodec(PAUDIO_CODEC_FUNC pAudioCodecFunc);

/**
    Switch audio codec

    Switch audio codec.
    You can use this function to switch between embedded/external audio codec and HDMI audio.
    For example, you should invoke aud_switch_codec(AUDIO_CODECSEL_HDMI) if HDMI plug is detected.
    After HDMI cable is removed, you may invoke aud_switch_codec(AUDIO_CODECSEL_DEFAULT) to switch back to embedded/external audio codec.

    @param[in] AudioCodec           Codec ID
                                    - @b AUDIO_CODECSEL_DEFAULT:    Default audio codec
                                    - @b AUDIO_CODECSEL_HDMI:       Extended audio codec 0
    @return void
*/
extern void             aud_switch_codec(AUDIO_CODECSEL AudioCodec);

/**
    Set Audio Device Control Object

    Set Audio Device Control Object. This device object provides the information
    of the external audio codec control interface.

    @param[in] pDevObj Audio Device Control Object. Please refer to PAUDIO_DEVICE_OBJ for details.

    @return void
*/
extern void             aud_set_device_object(PAUDIO_DEVICE_OBJ pDevObj);

/**
    Get AUDIO Device Object

    Get AUDIO Device Object

    @param[out] pDevObj     Return current AUDIO Device Object

    @return void
*/
extern void             aud_get_device_object(PAUDIO_DEVICE_OBJ pDevObj);

/**
    Select output path

    This function select the output path.
    The result is codec dependent.

    @param[in] Output       Output path. Available values are below:
                            - @b AUDIO_OUTPUT_SPK: output to (Speaker)
                            - @b AUDIO_OUTPUT_HP: output to (Headphone)
                            - @b AUDIO_OUTPUT_LINE: output to (Line Out)
                            - @b AUDIO_OUTPUT_MONO: output to (Mono Out)
                            - @b AUDIO_OUTPUT_NONE: no output
    @return void
*/
extern void             aud_set_output(AUDIO_OUTPUT Output);

/**
    Set special feature

    This function set the special feature to the audio codec.
    The result is codec dependent.
    You should call this function after aud_init().

    @param[in] Feature  Audio feature
    @param[in] bEnable  Enable/Disable the feature
                        - @b Enable Feature
                        - @b Disable Feature
    @return void
*/
extern void             aud_set_feature(AUDIO_FEATURE Feature, BOOL bEnable);

/**
    Set special parameter

    This function set the special parameter to the audio codec.
    The result is codec dependent.
    You should call this function after aud_init().

    @param[in] Parameter    Audio parameter. Available values are below:
    @param[in] uiSetting    parameter setting of Parameter.

    @return void
*/
extern void             aud_set_parameter(AUDIO_PARAMETER Parameter, UINT32 uiSetting);

/**
    Set PCM out total volume level

    This function set PCM out total volume level.The result is codec dependent.
    When the parameter "Audio_VolLevel" is set to AUDIO_VOL_LEVEL8, the valid
    parameter "vol" range of API aud_set_volume() is from AUDIO_VOL_MUTE ~ AUDIO_VOL_7.
    When the parameter "Audio_VolLevel" is set to AUDIO_VOL_LEVEL64, the valid parmeter
    "vol" range of API aud_set_volume() is from AUDIO_VOL_MUTE ~ AUDIO_VOL_63.

    @param[in] Audio_VolLevel   Total volume level. Available values are below:
                                - @b AUDIO_VOL_LEVEL8: Total volume level = 8
                                - @b AUDIO_VOL_LEVEL64: Total volume level = 64

    @return void
*/
extern void             aud_set_total_vol_level(AUDIO_VOL_LEVEL Audio_VolLevel);

/**
    Set PCM out total volume level

    This function get PCM out total volume level.The result is codec dependent.

    @return
        - @b AUDIO_VOL_LEVEL8: Total 8 volume level
        - @b AUDIO_VOL_LEVEL64: Total 64 volume level
*/
extern AUDIO_VOL_LEVEL  aud_get_total_vol_level(void);

/**
    Set PCM out volume

    This function set PCM out volume
    The result is codec dependent.

    @param[in] Vol      PCM out volume
    @return void
*/
extern void             aud_set_volume(AUDIO_VOL Vol);

/**
    Get PCM out volume

    This function get PCM out volume
    The result is codec dependent.

    @return PCM out volume
*/
extern AUDIO_VOL        aud_get_volume(void);

/**
    Set record gain

    This function set record gain
    The result is codec dependent.

    @param[in] Gain     Record gain. Available values are below:
                        - @b AUDIO_GAIN_0: Gain 0
                        - @b AUDIO_GAIN_1: Gain 1
                        - @b AUDIO_GAIN_2: Gain 2
                        - @b AUDIO_GAIN_3: Gain 3
                        - @b AUDIO_GAIN_4: Gain 4
                        - @b AUDIO_GAIN_5: Gain 5
                        - @b AUDIO_GAIN_6: Gain 6
                        - @b AUDIO_GAIN_7: Gain 7
                        - @b AUDIO_GAIN_MUTE: Mute
    @return void
*/
extern void             aud_set_gain(AUDIO_GAIN Gain);

/**
    Set record gain DB

    This function set record gain with db unit
    The result is codec dependent.

    @param[in] AUDIO_GAIN_DB(fDB)  Record gain. (unit: db)

    Please use AUDIO_GAIN_DB() as input.
    Ex: aud_set_gaindb(AUDIO_GAIN_DB(21.5));

    @return void
*/

extern void             aud_set_gaindb(INT32 fDB);

/**
    Set sound effect

    This function set the sound effect when playback
    The result is codec dependent.

    @param[in] Effect   Sound effect. Available values are below:
                        - @b AUDIO_EFFECT_NONE
                        - @b AUDIO_EFFECT_3D_HALF
                        - @b AUDIO_EFFECT_3D_FULL
                        - @b AUDIO_EFFECT_ROCK
                        - @b AUDIO_EFFECT_POP
                        - @b AUDIO_EFFECT_JAZZ
                        - @b AUDIO_EFFECT_CLASSICAL
                        - @b AUDIO_EFFECT_DNACE
                        - @b AUDIO_EFFECT_HEAVY
                        - @b AUDIO_EFFECT_DISCO
                        - @b AUDIO_EFFECT_SOFT
                        - @b AUDIO_EFFECT_LIVE
                        - @b AUDIO_EFFECT_HALL
    @return void
*/
extern void             aud_set_effect(AUDIO_EFFECT Effect);

/**
    Select record source

    This function select the record source.
    The result is codec dependent.

    @param[in] RecSrc       Record source. Available values are below:
                            - @b AUDIO_RECSRC_MIC: record from (Microphone)
                            - @b AUDIO_RECSRC_CD: record from (CD In)
                            - @b AUDIO_RECSRC_VIDEO: record from (Video In)
                            - @b AUDIO_RECSRC_AUX: record from (Aux In)
                            - @b AUDIO_RECSRC_LINE: record from (Line In)
                            - @b AUDIO_RECSRC_STEREO_MIX: record from (Stereo Mix)
                            - @b AUDIO_RECSRC_MONO_MIX: record from (Mono Mix)
                            - @b AUDIO_RECSRC_PHONE: record from (Phone In)
    @return void
*/
extern void             aud_set_record_source(AUDIO_RECSRC RecSrc);

/**
    Dump audio library settings

    Dump audio library state and settings

    @return void
*/
extern void             aud_printSetting(void);

/**
    Set I2S TDM Interface is 16 or 32 bits per audio channel

    Set I2S TDM Interface is 16 or 32 bits per audio channel.
    This can only be used when clock ratio using AUDIO_I2SCLKR_256FS_64BIT / AUDIO_I2SCLKR_256FS_128BIT
    / AUDIO_I2SCLKR_256FS_256BIT only.

    @param[in] ChBits Use AUDIO_I2SCH_BITS_16 or AUDIO_I2SCH_BITS_32

    @return void
*/
extern void             aud_set_i2s_chbits(AUDIO_I2SCH_BITS ChBits);

/**
    Set I2S format

    This function set I2S format
    The result is codec dependent.

    @param[in] I2SFmt   I2S format. Available values are below:
                        - @b AUDIO_I2SFMT_STANDARD: I2S Standard
                        - @b AUDIO_I2SFMT_LIKE_MSB: I2S Like, MSB justified
                        - @b AUDIO_I2SFMT_LIKE_LSB: I2S Like, LSB justified
    @return void
*/
extern void             aud_set_i2s_format(AUDIO_I2SFMT I2SFmt);

/**
    Set I2S clock ratio

    This function set I2S clock ratio
    The result is codec dependent.

    @param[in] I2SCLKRatio  I2S Clock Ratio. Available values are below:
                            - @b AUDIO_I2SCLKR_256FS_32BIT: SystemClk = 256 FrameSync, FrameSync = 32 BitClk
                            - @b AUDIO_I2SCLKR_256FS_64BIT: SystemClk = 256 FrameSync, FrameSync = 64 BitClk
                            - @b AUDIO_I2SCLKR_384FS_32BIT: SystemClk = 384 FrameSync, FrameSync = 32 BitClk
                            - @b AUDIO_I2SCLKR_384FS_48BIT: SystemClk = 384 FrameSync, FrameSync = 48 BitClk
                            - @b AUDIO_I2SCLKR_384FS_96BIT: SystemClk = 384 FrameSync, FrameSync = 96 BitClk
                            - @b AUDIO_I2SCLKR_512FS_32BIT: SystemClk = 512 FrameSync, FrameSync = 32 BitClk
                            - @b AUDIO_I2SCLKR_512FS_64BIT: SystemClk = 512 FrameSync, FrameSync = 64 BitClk
                            - @b AUDIO_I2SCLKR_512FS_128BIT: SystemClk = 512 FrameSync, FrameSync = 128 BitClk
                            - @b AUDIO_I2SCLKR_768FS_32BIT: SystemClk = 768 FrameSync, FrameSync = 32 BitClk
                            - @b AUDIO_I2SCLKR_768FS_48BIT: SystemClk = 768 FrameSync, FrameSync = 48 BitClk
                            - @b AUDIO_I2SCLKR_768FS_64BIT: SystemClk = 768 FrameSync, FrameSync = 64 BitClk
                            - @b AUDIO_I2SCLKR_768FS_192BIT: SystemClk = 768 FrameSync, FrameSync = 192 BitClk
                            - @b AUDIO_I2SCLKR_1024FS_32BIT: SystemClk = 1024 FrameSync, FrameSync = 32 BitClk
                            - @b AUDIO_I2SCLKR_1024FS_64BIT: SystemClk = 1024 FrameSync, FrameSync = 64 BitClk
                            - @b AUDIO_I2SCLKR_1024FS_128BIT: SystemClk = 1024 FrameSync, FrameSync = 128 BitClk
                            - @b AUDIO_I2SCLKR_1024FS_256BIT: SystemClk = 1024 FrameSync, FrameSync = 256 BitClk
    @return void
*/
extern void             aud_set_i2s_clkratio(AUDIO_I2SCLKR I2SCLKRatio);

/**
    Send command to I2S codec

    Send command to I2S codec.
    (OBSOLETE)

    @param[in] uiRegIdx I2S codec register index
    @param[in] uiData   Data you want to send

    @return void
*/
extern BOOL             aud_set_i2s_sentcommand(UINT32 uiRegIdx, UINT32 uiData);

/**
  Read register data from I2S codec

  Read register data from I2S codec.
  (OBSOLETE)

  @param[in] uiRegIdx   I2S codec register index you want to read
  @param[out] puiData   Register data

  @return
        - @b TRUE: read register successfully
        - @b FALSE: read register fail
*/
extern BOOL             aud_get_i2s_readdata(UINT32 uiRegIdx, UINT32 *puiData);

extern void aud_isr_handler(UINT32 uiAudioIntRegStatus);



//@}
#endif
