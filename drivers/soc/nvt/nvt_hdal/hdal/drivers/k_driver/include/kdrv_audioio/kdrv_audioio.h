/**
    KDRV Header file of Audio Input(Capture)/Output.

    Exported KDRV header file of Audio Input(Capture)/Output.

    @file       kdrv_audioio.h
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _KDRV_AUDIOIO_H
#define _KDRV_AUDIOIO_H

#if 1//__KERNEL__
#include "kdrv_type.h"
#else
#endif

#include "kdrv_audioio/audlib_aec.h"
#include "kdrv_audioio/audlib_agc.h"
#include "kdrv_audioio/audlib_anr.h"
#include "kdrv_audioio/audlib_filt.h"
#include "kdrv_audioio/audlib_src.h"
#include "kdrv_audioio/audlib_adpcm.h"
#include "kdrv_audioio/audlib_g711.h"
#include "kdrv_audioio/audlib_aac.h"








typedef enum {
	KDRV_AUDIOLIB_ID_AEC,
	KDRV_AUDIOLIB_ID_AGC,
	KDRV_AUDIOLIB_ID_ANR,
	KDRV_AUDIOLIB_ID_FILT,
	KDRV_AUDIOLIB_ID_SRC,
	KDRV_AUDIOLIB_ID_ADPCM,
	KDRV_AUDIOLIB_ID_G711,
	KDRV_AUDIOLIB_ID_AAC,
	KDRV_AUDIOLIB_ID_MAX,
	ENUM_DUMMY4WORD(KDRV_AUDIOLIB_ID)
} KDRV_AUDIOLIB_ID;


typedef struct {
	ER       (*open)(void);
	BOOL     (*is_opened)(void);
	ER       (*close)(void);
	void     (*set_config)(AEC_CONFIG_ID aec_sel, INT32 aec_cfg_value);
	BOOL     (*init)(void);
	BOOL     (*run)(PAEC_BITSTREAM p_aec_io);
	UINT32   (*get_config)(AEC_CONFIG_ID aec_sel);
	INT32	 (*get_required_buffer_size)(AEC_BUFINFO_ID buffer_id);
} KDRV_AUDIO_AEC_FUNC;

typedef struct {
	ER       (*open)(void);
	BOOL     (*is_opened)(void);
	ER       (*close)(void);
	void     (*set_config)(AGC_CONFIG_ID agc_sel, INT32 agc_cfg_value);
	BOOL     (*init)(void);
	BOOL     (*run)(PAGC_BITSTREAM p_agc_io);
} KDRV_AUDIO_AGC_FUNC;

typedef struct {
	int      (*get_version)(void);
	int      (*pre_init)(struct ANR_CONFIG *ptANR);
	int      (*init)(int *phandle, struct ANR_CONFIG *ptANR);
	void     (*set_snri)(int value);
	int      (*get_snri)(void);
	void     (*detect_reset)(int inkey);
	int      (*detect)(int outkey, short *pIn, struct ANR_CONFIG *ptANR);
	int      (*run)(int outkey, short *pIn, short *pOut);
	void     (*destroy)(int *handle);
} KDRV_AUDIO_ANR_FUNC;

typedef struct {
	BOOL     (*open)(PAUDFILT_INIT p_filt_init);
	BOOL     (*is_open)(void);
	BOOL     (*close)(void);
	BOOL     (*init)(void);
	BOOL     (*set_config)(AUDFILT_SELECTION filt_sel, PAUDFILT_CONFIG p_filt_config);
	BOOL     (*enable_filt)(AUDFILT_SELECTION filt_sel, BOOL enable);
	BOOL     (*design_filt)(PAUDFILT_EQPARAM p_eq_param, PAUDFILT_CONFIG p_filt_config, AUDFILT_DESGIN_CTRL design_ctrl);
	BOOL     (*run)(PAUDFILT_BITSTREAM  p_filt_io);
	void     (*enable_eq)(BOOL enable, UINT32 band_num);
	BOOL     (*design_eq)(AUDFILT_EQBAND band_index, PAUDFILT_EQPARAM p_eq_param);
} KDRV_AUDIO_FILT_FUNC;

typedef struct {
	int      (*get_version)(void);
	int      (*pre_init)(int ch, int in_count, int out_count, int one_frame_mode);
	int      (*init)(int *handle, int ch, int in_count, int out_count, int one_frame_mode, short *out_mem);
	int      (*run)(int handle, void *p_buffer_in, void *p_buffer_out);
	void	 (*destroy)(int handle);
} KDRV_AUDIO_SRC_FUNC;

typedef struct {
	UINT32   (*encode_mono)(INT16 *p_data_in, INT8 *p_data_out, UINT32 sample_count, PADPCM_STATE adpcm_state);
	UINT32   (*encode_stereo)(INT16 *p_data_in, INT8 *p_data_out, UINT32 sample_count, PADPCM_STATE adpcm_state);
	UINT32   (*decode_mono)(INT8 *p_data_in, INT16 *p_data_out, UINT32 sample_count, PADPCM_STATE adpcm_state);
	UINT32   (*decode_stereo)(INT8 *p_data_in, INT16 *p_data_out, UINT32 sample_count, PADPCM_STATE adpcm_state);
	UINT32   (*encode_packet_mono)(INT16 *p_data_in, INT8 *p_data_out, UINT32 sample_count, PADPCM_STATE adpcm_state);
	UINT32   (*encode_packet_stereo)(INT16 *p_data_in, INT8 *p_data_out, UINT32 sample_count, PADPCM_STATE adpcm_state);
	UINT32   (*decode_packet_mono)(INT8 *p_data_in, INT16 *p_data_out, UINT32 sample_count);
	UINT32   (*decode_packet_stereo)(INT8 *p_data_in, INT16 *p_data_out, UINT32 sample_count);
} KDRV_AUDIO_ADPCM_FUNC;

typedef struct {
	ER 		 (*ulaw_encode)(INT16 *p_data_in, UINT8 *p_data_out, UINT32 sample_count, BOOL input_swap);
	ER 		 (*ulaw_decode)(UINT8 *p_data_in, INT16 *p_data_out, UINT32 sample_count, BOOL duplicate_channel, BOOL output_swap);
	ER 		 (*alaw_encode)(INT16 *p_data_in, UINT8 *p_data_out, UINT32 sample_count, BOOL input_swap);
	ER 		 (*alaw_decode)(UINT8 *p_data_in, INT16 *p_data_out, UINT32 sample_count, BOOL duplicate_channel, BOOL output_swap);
} KDRV_AUDIO_G711_FUNC;

typedef struct {
	INT32 	 (*encode_init)(PAUDLIB_AAC_CFG p_encode_cfg);
	INT32 	 (*decode_init)(PAUDLIB_AAC_CFG p_decode_cfg);
	INT32 	 (*encode_one_frame)(PAUDLIB_AAC_BUFINFO p_encode_buf_info, PAUDLIB_AACE_RTNINFO p_return_info);
	INT32 	 (*decode_one_frame)(PAUDLIB_AAC_BUFINFO p_decode_buf_info, PAUDLIB_AACD_RTNINFO p_return_info);
} KDRV_AUDIO_AAC_FUNC;








typedef struct {
	union {
		KDRV_AUDIO_AEC_FUNC aec;
		KDRV_AUDIO_AGC_FUNC agc;
		KDRV_AUDIO_ANR_FUNC anr;
		KDRV_AUDIO_FILT_FUNC filter;
		KDRV_AUDIO_SRC_FUNC src;
		KDRV_AUDIO_ADPCM_FUNC adpcm;
		KDRV_AUDIO_G711_FUNC g711;
		KDRV_AUDIO_AAC_FUNC aac;
	};
} KDRV_AUDIOLIB_FUNC;

INT32 kdrv_audioio_reg_audiolib(KDRV_AUDIOLIB_ID id, KDRV_AUDIOLIB_FUNC *p_func);
KDRV_AUDIOLIB_FUNC* kdrv_audioio_get_audiolib(KDRV_AUDIOLIB_ID id);



/*************************
*	Capture related items:
**************************/

/**
    Audio capture path

    Select the audio capture path.
    Used in kdrv_audioio_set(KDRV_AUDIOIO_CAP_PATH, KDRV_AUDIO_CAP_PATH_*) / kdrv_audioio_get(KDRV_AUDIOIO_CAP_PATH)
*/
typedef enum {
	KDRV_AUDIO_CAP_PATH_AMIC,			///< Analog Microphone (Default)
	KDRV_AUDIO_CAP_PATH_DMIC,			///< Digital Microphone
	KDRV_AUDIO_CAP_PATH_I2S,			///< Digital Microphone

	ENUM_DUMMY4WORD(KDRV_AUDIO_CAP_PATH)
} KDRV_AUDIO_CAP_PATH;


/**
    Audio Capture Default Settings Select

    This is used at kdrv_audioio_set(KDRV_AUDIOIO_CAP_DEFAULT_CFG, KDRV_AUDIO_CAP_DEFSET_*)
    to select audio default configurations. Different characteristics for each select option is shown as below.
*/
typedef enum {
	KDRV_AUDIO_CAP_DEFSET_10DB,			///< Boost Gain 10dB. NoiseGate-Enable. ALC-Range(+1.5 ~ +25.5dB).
										///< Atk=0.08ms. Dcy=0.16s. NG-Atk=0.72s. NG-Dcy=0.36s. NG-Target=2.
										///< NoiseGate-TH= -67.5-10=-77.5dB from MIC input.
	KDRV_AUDIO_CAP_DEFSET_10DB_HP_8K,	///< The Same as KDRV_AUDIO_DEFSET_10DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR8KHz.
	KDRV_AUDIO_CAP_DEFSET_10DB_HP_16K,  ///< The Same as KDRV_AUDIO_DEFSET_10DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR16KHz.
	KDRV_AUDIO_CAP_DEFSET_10DB_HP_32K,	///< The Same as KDRV_AUDIO_DEFSET_10DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR32KHz.
	KDRV_AUDIO_CAP_DEFSET_10DB_HP_48K,  ///< The Same as KDRV_AUDIO_DEFSET_10DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR48KHz.

	KDRV_AUDIO_CAP_DEFSET_20DB,         ///< Boost Gain 20dB. NoiseGate-Enable. ALC-Range(-9 ~ +21dB).
										///< Atk=0.08ms. Dcy=0.16s. NG-Atk=0.72s. NG-Dcy=0.36s. NG-Target=1.
										///< NoiseGate-TH= -58.5-20=-78.5dB from MIC input.
	KDRV_AUDIO_CAP_DEFSET_20DB_HP_8K,   ///< The Same as KDRV_AUDIO_DEFSET_20DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR8KHz.
	KDRV_AUDIO_CAP_DEFSET_20DB_HP_16K,  ///< The Same as KDRV_AUDIO_DEFSET_20DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR16KHz.
	KDRV_AUDIO_CAP_DEFSET_20DB_HP_32K,  ///< The Same as KDRV_AUDIO_DEFSET_20DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR32KHz.
	KDRV_AUDIO_CAP_DEFSET_20DB_HP_48K,  ///< The Same as KDRV_AUDIO_DEFSET_20DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR48KHz.

	KDRV_AUDIO_CAP_DEFSET_30DB,         ///< Boost Gain 30dB. NoiseGate-Enable. ALC-Range(-9 ~ +13.5dB).
										///< Atk=0.08ms. Dcy=0.16s. NG-Atk=2.88s. NG-Dcy=0.36s. NG-Target=1.
										///< NoiseGate-TH= -40-30=-70dB from MIC input.
	KDRV_AUDIO_CAP_DEFSET_30DB_HP_8K,   ///< The Same as KDRV_AUDIO_DEFSET_30DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR8KHz.
	KDRV_AUDIO_CAP_DEFSET_30DB_HP_16K,  ///< The Same as KDRV_AUDIO_DEFSET_30DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR16KHz.
	KDRV_AUDIO_CAP_DEFSET_30DB_HP_32K,  ///< The Same as KDRV_AUDIO_DEFSET_30DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR32KHz.
	KDRV_AUDIO_CAP_DEFSET_30DB_HP_48K,  ///< The Same as KDRV_AUDIO_DEFSET_30DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR48KHz.

	KDRV_AUDIO_CAP_DEFSET_0DB,          ///< Boost Gain 0dB. NoiseGate-Enable. ALC-Range(-9 ~ +13.5dB).
										///< Atk=0.08ms. Dcy=0.16s. NG-Atk=2.88s. NG-Dcy=0.36s. NG-Target=1.
										///< NoiseGate-TH= -40-0=-40dB from MIC input.
	KDRV_AUDIO_CAP_DEFSET_0DB_HP_8K,    ///< The Same as KDRV_AUDIO_DEFSET_0DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR8KHz.
	KDRV_AUDIO_CAP_DEFSET_0DB_HP_16K,   ///< The Same as KDRV_AUDIO_DEFSET_0DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR16KHz.
	KDRV_AUDIO_CAP_DEFSET_0DB_HP_32K,   ///< The Same as KDRV_AUDIO_DEFSET_0DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR32KHz.
	KDRV_AUDIO_CAP_DEFSET_0DB_HP_48K,   ///< The Same as KDRV_AUDIO_DEFSET_0DB but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR48KHz.

	KDRV_AUDIO_CAP_DEFSET_ALCOFF,       ///< Boost Gain 0dB. NoiseGate-Disable. ALC-Disable
	KDRV_AUDIO_CAP_DEFSET_ALCOFF_HP_8K, ///< The Same as KDRV_AUDIO_DEFSET_ALCOFF but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR8KHz.
	KDRV_AUDIO_CAP_DEFSET_ALCOFF_HP_16K,///< The Same as KDRV_AUDIO_DEFSET_ALCOFF but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR16KHz.
	KDRV_AUDIO_CAP_DEFSET_ALCOFF_HP_32K,///< The Same as KDRV_AUDIO_DEFSET_ALCOFF but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR32KHz.
	KDRV_AUDIO_CAP_DEFSET_ALCOFF_HP_48K,///< The Same as KDRV_AUDIO_DEFSET_ALCOFF but with HighPass-Filter Enabled.
										///< HighPass Filter 3dB is about 200Hz for SR48KHz.
	KDRV_AUDIO_CAP_DEFSET_DMIC,         ///< Default setting for Digital mic.
										///< Atk=0.08ms. Dcy=0.16s. NG-Atk=2.88s. NG-Dcy=0.36s. NG-Target=1.
										///< NoiseGate-TH= -40-30=-70dB from MIC input.
	KDRV_AUDIO_CAP_DEFSET_DMIC_LP_8K,  	///< The Same as AUDIO_DEFSET_DMIC but with HighPass-Filter Enabled.
										///< Low Pass Filter 3dB is about 2000Hz for SR8KHz.
	KDRV_AUDIO_CAP_DEFSET_DMIC_LP_16K, 	///< The Same as AUDIO_DEFSET_DMIC but with HighPass-Filter Enabled.
										///< Low Pass Filter 3dB is about 4500Hz for SR16KHz.
	KDRV_AUDIO_CAP_DEFSET_DMIC_LP_32K, 	///< The Same as AUDIO_DEFSET_DMIC but with HighPass-Filter Enabled.
										///< Low Pass Filter 3dB is about 8000Hz for SR32KHz.
	KDRV_AUDIO_CAP_DEFSET_DMIC_LP_48K, 	///< The Same as AUDIO_DEFSET_DMIC but with HighPass-Filter Enabled.
										///< Low Pass Filter 3dB is about 12000Hz for SR48KHz. 											

	KDRV_AUDIO_CAP_DEFSET_MAX,
	ENUM_DUMMY4WORD(KDRV_AUDIO_CAP_DEFSET)
} KDRV_AUDIO_CAP_DEFSET;


/**
    Audio capture mono select

    When KDRV_AUDIOIO_CAP_CHANNEL_NUMBER is set to 1, this is used to select this 1 channel is from left or right on physical path.
    Used in kdrv_audioio_set(KDRV_AUDIOIO_CAP_MONO_SEL, KDRV_AUDIO_CAP_MONO_*) / kdrv_audioio_get(KDRV_AUDIOIO_CAP_MONO_SEL)
*/
typedef enum {
	KDRV_AUDIO_CAP_MONO_LEFT,			///< LEFT channel (Default)
	KDRV_AUDIO_CAP_MONO_RIGHT,			///< RIGHT channel

	ENUM_DUMMY4WORD(KDRV_AUDIO_CAP_MONO)
} KDRV_AUDIO_CAP_MONO;


/**
    Auto Level Control and Noise Gate Time Resolution basis
*/
typedef enum {
	KDRV_AUDIO_CAP_TRESO_BASIS_800US   = 0x40457,  ///< Time Basis as  0.8ms for ALC Attack/Decay Time adjustment.
	KDRV_AUDIO_CAP_TRESO_BASIS_1000US  = 0x5056C,  ///< Time Basis as  1.0ms for ALC Attack/Decay Time adjustment.
	KDRV_AUDIO_CAP_TRESO_BASIS_2000US  = 0xA0000,  ///< Time Basis as  2.0ms for ALC Attack/Decay Time adjustment.
	KDRV_AUDIO_CAP_TRESO_BASIS_5000US  = 0x120000, ///< Time Basis as  5.0ms for ALC Attack/Decay Time adjustment.
	KDRV_AUDIO_CAP_TRESO_BASIS_10000US = 0x1FFFFF, ///< Time Basis as 10.0ms for ALC Attack/Decay Time adjustment.
	KDRV_AUDIO_CAP_TRESO_BASIS_15000US = 0x2FFFFF, ///< Time Basis as 15.0ms for ALC Attack/Decay Time adjustment.
	KDRV_AUDIO_CAP_TRESO_BASIS_45000US = 0x7FFFFF, ///< Time Basis as 45.0ms for ALC Attack/Decay Time adjustment.

	ENUM_DUMMY4WORD(KDRV_AUDIO_CAP_TRESO_BASIS)
} KDRV_AUDIO_CAP_TRESO_BASIS;


/**
    Embedded codec audio Capture Second Order IIR Filter Coeficients

    The second order IIR equation is H(z) =  SectionGain x ((B0 + B1*Z1 + B2*Z2) / (A0 + A1*Z1 + A2*Z2)) x (Total-Gain).
    This can be used to implement audio notch/lowpass/highpass filter as needed.

    @note For KDRV_AUDIOIO_CAP_IIRCOEF_L / KDRV_AUDIOIO_CAP_IIRCOEF_R
*/
typedef struct {
	INT32   total_gain;     ///< Total Gain
	INT32   section_gain;   ///< Section Gain

	INT32   coef_b0;        ///< Filter coefficient B0
	INT32   coef_b1;        ///< Filter coefficient B1
	INT32   coef_b2;        ///< Filter coefficient B2

	INT32   coef_a0;        ///< Filter coefficient A0
	INT32   coef_a1;        ///< Filter coefficient A1
	INT32   coef_a2;        ///< Filter coefficient A2
} KDRV_AUDIO_CAP_IIRCOEF, *PKDRV_AUDIO_CAP_IIRCOEF;


/*************************
*	Output related items:
**************************/

/**
    Audio output path

    Select the audio output path.
    Used in kdrv_audioio_set(KDRV_AUDIOIO_OUT_PATH,KDRV_AUDIO_OUT_PATH_*) / kdrv_audioio_get(KDRV_AUDIOIO_OUT_PATH)
*/
typedef enum {
	KDRV_AUDIO_OUT_PATH_NONE,		///< Turn off all the output path. (Default)
	KDRV_AUDIO_OUT_PATH_SPEAKER,	///< Output to speaker path
	KDRV_AUDIO_OUT_PATH_LINEOUT,	///< Output to lineout path
	KDRV_AUDIO_OUT_PATH_I2S,		///< Output to I2S path
	KDRV_AUDIO_OUT_PATH_ALL,        ///< Output to speaker + lineout path

	KDRV_AUDIO_OUT_PATH_MAX,
	ENUM_DUMMY4WORD(KDRV_AUDIO_OUT_PATH)
} KDRV_AUDIO_OUT_PATH;

/**
    Audio output mono select

    When KDRV_AUDIOIO_OUT_CHANNEL_NUMBER is set to 1, this is used to select this 1 channel is from left or right on physical path.
    Used in kdrv_audioio_set(KDRV_AUDIOIO_OUT_MONO_SEL, KDRV_AUDIO_OUT_MONO_*) / kdrv_audioio_get(KDRV_AUDIOIO_OUT_MONO_SEL)
*/
typedef enum {
	KDRV_AUDIO_OUT_MONO_LEFT,			///< LEFT channel (Default)
	KDRV_AUDIO_OUT_MONO_RIGHT,			///< RIGHT channel

	ENUM_DUMMY4WORD(KDRV_AUDIO_OUT_MONO)
} KDRV_AUDIO_OUT_MONO;


/*************************
*	Global items
**************************/

typedef struct {
	UINT32 handler;			///< the handler of hardware
	UINT32 curr_timecode;	///< timestamp of output buffer
	UINT32 channel_number;	///< the current count of channels
} KDRV_AUDIOIO_CB_INFO;

/**
    Audio i2s operate mode
    Used in kdrv_audioio_set(KDRV_AUDIOIO_GLOBAL_I2S_OPMODE, KDRV_AUDIO_I2S_OPMODE_*)
*/
typedef enum {
	KDRV_AUDIO_I2S_OPMODE_SLAVE,			///< i2s work at slave mode
	KDRV_AUDIO_I2S_OPMODE_MASTER,		///< i2s work at master mode

	ENUM_DUMMY4WORD(KDRV_AUDIO_I2S_OPMODE)
} KDRV_AUDIO_I2S_OPMODE;

/**
    Audio i2s data order type
    Used in kdrv_audioio_set(KDRV_AUDIOIO_GLOBAL_I2S_DATA_ORDER, KDRV_AUDIO_I2S_DATA_ORDER_*)
*/
typedef enum {
	KDRV_AUDIO_I2S_DATA_ORDER_TYPE1,		///< i2s data order type 1. Ex: TDM_ch = 8 [data_0][data_2][data_4][data_6][data_1][data_3][data_5][data_7]
											///<                            TDM_ch = 6 [data_0][data_2][data_4][data_1][data_3][data_5]
											///<                            TDM_ch = 4 [data_0][data_2][data_1][data_3]
											///<                            TDM_ch = 2 [data_0][data_1]
	KDRV_AUDIO_I2S_DATA_ORDER_TYPE2,		///< i2s data order type 2. Ex: TDM_ch = 8 [data_0][data_1][data_2][data_3][data_4][data_5][data_6][data_7]
											///<                            TDM_ch = 6 [data_0][data_1][data_2][data_3][data_4][data_5]
											///<                            TDM_ch = 4 [data_0][data_1][data_2][data_3]
											///<                            TDM_ch = 2 [data_0][data_1]

	ENUM_DUMMY4WORD(KDRV_AUDIO_I2S_DATA_ORDER)
} KDRV_AUDIO_I2S_DATA_ORDER;


typedef enum {
	/*
		Global GROUP settings are validate for both the audio capture/output path.
	*/
	KDRV_AUDIOIO_GLOBAL_BASE = 0x01000000,

	KDRV_AUDIOIO_GLOBAL_SAMPLE_RATE,		///< [capt/out][set/get][embd/i2s]:	Audio Sample Rate. Valid setting value are 8000/11025/12000/16000/24000/32000/44100/48000.
	KDRV_AUDIOIO_GLOBAL_IS_BUSY,			///< [capt/out][get][embd/i2s]:		Get the current engine is in busy because of record/playback ongoing. TRUE: is BUSY. FALSE: is IDLE.
	KDRV_AUDIOIO_GLOBAL_I2S_BIT_WIDTH,		///< [capt/out][set/get][i2s]:		I2S interafce bit-width per audio data channel. Valid input is 16 or 32.
	KDRV_AUDIOIO_GLOBAL_I2S_BITCLK_RATIO,	///< [capt/out][set/get][i2s]:		I2S interface bit-clk vs sample-rae ratio. Valid input is 32/64/128/256. others is illegal. default is 32.
	KDRV_AUDIOIO_GLOBAL_I2S_OPMODE,         ///< [capt/out][set/get][i2s]:      I2S interface is master or slave mode. Valid input is KDRV_AUDIO_I2S_OPMODE for master/slave mode. Default is master mode.
	KDRV_AUDIOIO_GLOBAL_I2S_DATA_ORDER,     ///< [capt/out][set/get][i2s]:		I2S interface data order setting. detail please refer to KDRV_AUDIO_I2S_DATA_ORDER. Default is KDRV_AUDIO_I2S_DATA_ORDER_TYPE1.
	KDRV_AUDIOIO_GLOBAL_CLOCK_ALWAYS_ON,	///< [capt/out][set/get][embd/i2s]:	Audio clock always on control. TRUE to enable always ON. FALSE to dynamic ON/OFF.
	KDRV_AUDIOIO_GLOBAL_ISR_CB,


	/*
		Capture GROUP settings:
	*/
	KDRV_AUDIOIO_CAP_BASE = 0x02000000,
	KDRV_AUDIOIO_CAP_PATH,					///< [cap][set/get][embd/i2s]:	Switch the audio capture path source. Please use KDRV_AUDIO_CAP_PATH as path select parameter.
	KDRV_AUDIOIO_CAP_DEFAULT_CFG,			///< [cap][set][embd]:			Set the audio embedeed codec capture path default configurations. Please use KDRV_AUDIO_CAP_DEFSET as setting select number.
	KDRV_AUDIOIO_CAP_CHANNEL_NUMBER,		///< [cap][set/get][embd/i2s]:	Audio capture channel number.
											///<							Embedded codec capture path valid settings is 1/2.
											///<							I2S capture path valid settings is 1/2/4/6/8.
	KDRV_AUDIOIO_CAP_MONO_SEL,				///< [cap][set/get][embd/i2s]:	When KDRV_AUDIOIO_CAP_CHANNEL_NUMBER set to 1.
											///<							Use to to select this 1 channel from Left or Right on physical path.
											///<							Please use KDRV_AUDIO_CAP_MONO_LEFT or KDRV_AUDIO_CAP_MONO_RIGHT as input param.
	KDRV_AUDIOIO_CAP_MONO_EXPAND,			///< [cap][set/get][embd/i2s]:	When KDRV_AUDIOIO_CAP_CHANNEL_NUMBER set to 1, Expand the same mono data output to 2 channels on the DRAM buffer.
											///<							Set TRUE to ENABLE. FALSE to DISABLE.
	KDRV_AUDIOIO_CAP_BIT_WIDTH,				///< [cap][set/get][embd/i2s]:	Setting audio capture bit-width per channel. Valid setting value is 8/16/32.(Means PCM8/PCM16/PCM32)
	KDRV_AUDIOIO_CAP_VOLUME,				///< [cap][set/get][embd]:		Set audio capture volume. Normal input range is 0~100.<PGA gain>
	KDRV_AUDIOIO_CAP_VOLUME_LEFT,			///< [cap][set/get][embd]:		Same fucntion as KDRV_AUDIOIO_CAP_VOLUME but only set left channel
	KDRV_AUDIOIO_CAP_VOLUME_RIGHT,			///< [cap][set/get][embd]:		Same fucntion as KDRV_AUDIOIO_CAP_VOLUME but only set right channel											///<							0 is mute. 100 is normal maximum volume by using PGA gain.
											///<							101 ~ 200 is adding additional digital gain.
	KDRV_AUDIOIO_CAP_DCCAN_RESOLUTION,      ///< [cap][set/get][embd]:		Recording DC Cancellation Resolution. Valid setting range is 0~7.
	KDRV_AUDIOIO_CAP_TRIGGER_DELAY,         ///< [cap][set/get][embd]:		Embedded codec capture start delay, the MIC_BAIS would be enabled and the external regulator needs some stable time before record really start.
											///<							This delay is used to prevent recording this pop noise. The default value is 200 (mini-seconds). This setting unit is mini-seconds.
	KDRV_AUDIOIO_CAP_AMIC_BOOST,			///< [cap][set/get][embd]:		Embedded codec analog microphone pga boost gain. Valid setting value is 0/10/20/30.
	KDRV_AUDIOIO_CAP_ALC_EN,				///< [cap][set/get][embd]:		Embedded codec Auto level control ENABLE/DISABLE.
											///<							Default is Enabled after KDRV_AUDIOIO_CAP_DEFAULT_CFG is invoked.
	KDRV_AUDIOIO_CAP_ALC_MAXGAIN,			///< [cap][set/get][embd]:		Embedded codec ALC maximum PGA gain. Valid setting range is -21 ~ +26 (dB).
	KDRV_AUDIOIO_CAP_ALC_MINGAIN,           ///< [cap][set/get][embd]:		Embedded codec ALC minimum PGA gain. Valid setting range is -21 ~ +26 (dB).

	KDRV_AUDIOIO_CAP_ALC_ATTACK_TIME,       ///< [cap][set/get][embd]:		Embedded codec ALC Attack Time length.  Valid setting range is 0~10.
											///<							Time length would be "(2 ^ KDRV_AUDIOIO_CAP_ALC_ATTACK_TIME) x KDRV_AUDIOIO_CAP_ALC_TIME_RESOLUTION-Basis".
	KDRV_AUDIOIO_CAP_ALC_DECAY_TIME,        ///< [cap][set/get][embd]:		Embedded codec ALC Decay Time length. Valid setting range is 0~10.
											///<							Time length would be "(2 ^ KDRV_AUDIOIO_CAP_ALC_DECAY_TIME) x KDRV_AUDIOIO_CAP_ALC_TIME_RESOLUTION-Basis".
	KDRV_AUDIOIO_CAP_ALC_HOLD_TIME,         ///< [cap][set/get][embd]:		Embedded codec ALC Hold Time length. Valid setting range is 0~15.
											///<							Time length would be "(2 ^ (uiCfgValue-1)) x 117".
	KDRV_AUDIOIO_CAP_ALC_TIME_RESOLUTION,   ///< [cap][set/get][embd]:		Embedded codec ALC Basis Time Resolution for Attack/Decay Time Calculation.
											///<							Please use "KDRV_AUDIO_CAP_TRESO_BASIS" as input paramter.
	KDRV_AUDIOIO_CAP_NOISEGATE_EN,			///< [cap][set/get][embd]:		Embedded codec Noise Gate function ENABLE/DISABLE.
											///<							This setting is valid only if KDRV_AUDIOIO_CAP_ALC_EN is enabled.
											///<							Default is Enabled after KDRV_AUDIOIO_CAP_DEFAULT_CFG is invoked.
	KDRV_AUDIOIO_CAP_NOISEGATE_THRESHOLD,	///< [cap][set/get][embd]:		Embedded codec Noise gate threshold value. Valid setting range -30 ~ -77 (dB).
	KDRV_AUDIOIO_CAP_NG_BOOST_COMPENSATION,	///< [cap][set/get][embd]:		Embedded codec Noise gate function with boost gain compensation. 1/0 for enalbe/disable boost gain compensation. default is enable.
	KDRV_AUDIOIO_CAP_NOISEGAIN,				///< [cap][set/get][embd]:		Embedded codec Noise gain. (Valid value: 0x0~0xF). If the KDRV_AUDIOIO_CAP_NOISEGATE_EN enabled
											///<							The Noise gate Target Level = ALC_LEVEL - (NOISE_GATE_THRESHOLD - SIGNAL_LEVEL)*(1 + KDRV_AUDCAP_PARAM_NOISEGAIN).
	KDRV_AUDIOIO_CAP_ALCNG_ATTACK_TIME,     ///< [cap][set/get][embd]:		Embedded codec ALC Attack Time length in NoiseGate State. Valid setting range is 0~10.
											///<							Time length would be "(2 ^ KDRV_AUDIOIO_CAP_ALCNG_ATTACK_TIME) x KDRV_AUDIOIO_CAP_NOISEGATE_TIME_RESOLUTION-Basis".
	KDRV_AUDIOIO_CAP_ALCNG_DECAY_TIME,      ///< [cap][set/get][embd]:		Embedded codec ALC Decay Time length in NoiseGate State. Valid setting range is 0~10.
											///<							Time length would be "(2 ^ KDRV_AUDIOIO_CAP_ALCNG_DECAY_TIME) x KDRV_AUDIOIO_CAP_NOISEGATE_TIME_RESOLUTION-Basis".
	KDRV_AUDIOIO_CAP_NOISEGATE_TIME_RESOLUTION,///< [cap][set/get][embd]:	Embedded codec ALC Time Resolution in the NoiseGate state. Please use "KDRV_AUDIO_CAP_TRESO_BASIS" as input paramter.
	KDRV_AUDIOIO_CAP_ALC_IIR_EN,            ///< [cap][set/get][embd]:		Second Order IIR filter for the ALC function. (Default Disabled)
	KDRV_AUDIOIO_CAP_OUTPUT_IIR_EN,         ///< [cap][set/get][embd]:		Second Order IIR filter for the Recorded Output Data. (Default Disabled)
	KDRV_AUDIOIO_CAP_IIRCOEF_L,             ///< [cap][set][embd]:			Left Channel Embedded Second Order IIR filter Coeficients. Please use the address of  PKDRV_AUDIO_CAP_IIRCOEF as input parameter.
	KDRV_AUDIOIO_CAP_IIRCOEF_R,             ///< [cap][set][embd]:			Right Channel Embedded Second Order IIR filter Coeficients. Please use the address of  PKDRV_AUDIO_CAP_IIRCOEF as input parameter.

	KDRV_AUDIOIO_CAP_TIMECODE_HIT_EN,		///< [cap][set/get][embd/i2s]:	Set Embedded codec capture Time Code Hit Event enable.
	KDRV_AUDIOIO_CAP_TIMECODE_HIT_CB,       ///< [cap][set][embd/i2s]:		The callback event handle for dedicate capture channel.
											///<							The callback prototype is KDRV_CALLBACK_FUNC..
	KDRV_AUDIOIO_CAP_TIMECODE_TRIGGER,      ///< [cap][set/get][embd/i2s]:	The timecode trigger value.
											///<							Timecode is sample value. When is time code trigger value is hit, the timecode Callback would be called.
	KDRV_AUDIOIO_CAP_TIMECODE_OFFSET,       ///< [cap][set/get][embd/i2s]:	Assign the timecode start counting offset value.
	KDRV_AUDIOIO_CAP_TIMECODE_VALUE,        ///< [cap][get][embd/i2s]:		Get codec capture current timecode value..
	KDRV_AUDIOIO_CAP_PDVCMBIAS_ALWAYS_ON,	///< [cap][set][embd]:			Let ADVCM always power-on for faster Mic stable time. [Default disabled]


	/*
		Output GROUP settings:
	*/
	KDRV_AUDIOIO_OUT_BASE = 0x04000000,
	KDRV_AUDIOIO_OUT_PATH,					///< [out][set/get][embd/i2s]:	Switch the audio output path source. Please use KDRV_AUDIO_OUT_PATH as path select parameter.
	KDRV_AUDIOIO_OUT_HDMI_PATH_EN,			///< [out][set/get][embd/i2s]:	Select HDMI output path ENABLE/DISABLE.
	KDRV_AUDIOIO_OUT_CHANNEL_NUMBER,		///< [out][set/get][embd/i2s]:	Audio output channel number.
											///<							Embedded codec output path valid settings is 1/2.
											///<							I2S output path valid settings is 1/2/4/6/8.
	KDRV_AUDIOIO_OUT_MONO_SEL,				///< [out][set/get][embd/i2s]:	When KDRV_AUDIOIO_OUT_CHANNEL_NUMBER set to 1.
											///<							Use to to select this 1 channel from Left or Right on physical path.
											///<							Please use KDRV_AUDIO_OUT_MONO_LEFT or KDRV_AUDIO_OUT_MONO_RIGHT as input param.
	KDRV_AUDIOIO_OUT_MONO_EXPAND,			///< [out][set/get][embd/i2s]:	When KDRV_AUDIOIO_OUT_CHANNEL_NUMBER set to 1, Expand the same mono data output to 2 channels output path.
											///<							This can be used in the HDMI application.
											///<							Set TRUE to ENABLE. FALSE to DISABLE.
	KDRV_AUDIOIO_OUT_BIT_WIDTH,				///< [out][set/get][embd/i2s]:	Setting audio output source data bit-width per channel. Valid setting value is 8/16/32.(Means PCM8/PCM16/PCM32)
	KDRV_AUDIOIO_OUT_VOLUME,				///< [out][set/get][embd]:		Set audio output volume. Normal input range is 0~100.<PGA gain>
											///<							0 is mute. 100 is normal maximum volume by using PGA gain.
											///<							101 ~ 200 is adding additional digital gain.
	KDRV_AUDIOIO_OUT_VOLUME_LEFT,			///< [out][set/get][embd]:		Set audio output left channel volume. 0 is mute. 1 ~ 200 is  digital gain.
	KDRV_AUDIOIO_OUT_VOLUME_RIGHT,			///< [out][set/get][embd]:		Set audio output right channel volume. 0 is mute. 1 ~ 200 is  digital gain.
	KDRV_AUDIOIO_OUT_SPK_PWR_ALWAYS_ON,		///< [out][set/get][embd]:		Speaker output path power always ON control. TRUE to enable always ON. FALSE to dynamic ON/OFF.(Default disabled)
	KDRV_AUDIOIO_OUT_LINE_PWR_ALWAYS_ON,	///< [out][set/get][embd]:		Lineout output path power always ON control. TRUE to enable always ON. FALSE to dynamic ON/OFF.(Default disabled)

	KDRV_AUDIOIO_OUT_TIMECODE_HIT_EN,       ///< [out][set/get][embd/i2s]:	Set Time Code Hit Event ENABLE/DISABLE.
											///<							Timecode is sample value. When is time code trigger value is hit, the timecode Callback would be called.
	KDRV_AUDIOIO_OUT_TIMECODE_HIT_CB,       ///< [out][set][embd/i2s]:		The callback event handle for dedicate output channel.
											///<							The callback prototype is KDRV_CALLBACK_FUNC.
	KDRV_AUDIOIO_OUT_TIMECODE_TRIGGER,      ///< [out][set/get][embd/i2s]:	The timecode trigger value.
											///<							Timecode is sample value. When is time code trigger value is hit, the timecode Callback would be called.
	KDRV_AUDIOIO_OUT_TIMECODE_OFFSET,       ///< [out][set/get][embd/i2s]:	Assign the timecode start counting offset value.
	KDRV_AUDIOIO_OUT_TIMECODE_VALUE,        ///< [out][get][embd/i2s]:		Get codec output current timecode value.


	KDRV_AUDIOIO_PARAM_ID_MAX,
	ENUM_DUMMY4WORD(KDRV_AUDIOIO_PARAM_ID)
} KDRV_AUDIOIO_PARAM_ID;


/*!
 * @fn INT32 kdrv_audioio_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware. Please use KDRV_AUDCAP_ENGINE0 / KDRV_AUDCAP_ENGINE1 / KDRV_AUDOUT_ENGINE0 / KDRV_AUDOUT_ENGINE1.
 * @return return 0 on success, -1 on error
 */
UINT32 kdrv_audioio_open(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_audioio_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware. Please use KDRV_AUDCAP_ENGINE0 / KDRV_AUDCAP_ENGINE1 / KDRV_AUDOUT_ENGINE0 / KDRV_AUDOUT_ENGINE1.
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_audioio_close(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_audioio_set(UINT32 handler, KDRV_AUDIOIO_PARAM_ID id, VOID *param)
 * @brief set parameters to hardware engine
 * @param id	    the handler of hardware
 * @param param_id	the param_id of parameters
 * @param param		the parameters
 * @return return 0 on success, -1 on error
 */
INT32 __kdrv_audioio_set(UINT32 id, KDRV_AUDIOIO_PARAM_ID param_id, VOID *param);
#define kdrv_audioio_set(handler, id, param)  __kdrv_audioio_set((UINT32)(handler), (KDRV_AUDIOIO_PARAM_ID)(id), (VOID *)(param))

/*!
 * @fn INT32 kdrv_audioio_get(UINT32 handler, KDRV_AUDIOIO_PARAM_ID param_id, VOID *param)
 * @brief set parameters to hardware engine
 * @param id	    the handler of hardware
 * @param param_id	the param_id of parameters
 * @param param		the parameters
 * @return return 0 on success, -1 on error
 */
INT32 __kdrv_audioio_get(UINT32 id, KDRV_AUDIOIO_PARAM_ID param_id, VOID *param);
#define kdrv_audioio_get(handler, id, param)  __kdrv_audioio_get((UINT32)(handler), (KDRV_AUDIOIO_PARAM_ID)(id), (VOID *)(param))

/*!
 * @fn INT32 kdrv_audioio_trigger(UINT32 handler,
							KDRV_BUFFER_INFO *p_au_frame_buffer,
							KDRV_CALLBACK_FUNC *p_cb_func,
							VOID *user_data);
 * @brief trigger hardware engine
 * @param id					the handler of hardware
 * @param p_in_au_frame_buffer	the input audio frame buffer
 * @param p_cb_func				the callback function
 * @param user_data				the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_audioio_trigger(UINT32 id,
							KDRV_BUFFER_INFO *p_au_frame_buffer,
							KDRV_CALLBACK_FUNC *p_cb_func,
							VOID *user_data);


/*!
 * @fn INT32 kdrv_audioio_trigger_not_start(UINT32 handler,
							KDRV_BUFFER_INFO *p_au_frame_buffer,
							KDRV_CALLBACK_FUNC *p_cb_func,
							VOID *user_data);
 * @brief trigger hardware engine but only insert buffer and won't start engine
 * @param id     				the handler of hardware
 * @param p_in_au_frame_buffer	the input audio frame buffer
 * @param p_cb_func 			the callback function
 * @param user_data 			the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_audioio_trigger_not_start(UINT32 id,
							KDRV_BUFFER_INFO *p_au_frame_buffer,
							KDRV_CALLBACK_FUNC *p_cb_func,
							VOID *user_data);

void kdrv_audioio_abort(void);



#endif
