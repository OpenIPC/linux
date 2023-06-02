/**
    Audio Echo Cancellation library header file

    This file is the global header of Audio Echo Cancellation library.

    @file       Aec.h
    @ingroup    mIAudEC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/

#ifndef _AUDLIB_AEC_H
#define _AUDLIB_AEC_H

#ifdef __KERNEL__
#include "kdrv_type.h"
#else
#include "kdrv_type.h"
#endif
/**
    @addtogroup mIAudEC
*/
//@{


/**
    Audio Echo Cancellation(AEC) Configuration Select ID

    This is used in audlib_aec_set_config() as the select ID to assign new configurations
    for Audio Echo Cancellation(AEC) library.
*/
typedef enum {
	AEC_CONFIG_ID_SAMPLERATE,       ///< Set SampleRate in Hertz.
	AEC_CONFIG_ID_RECORD_CH_NO,     ///< Set Record Channel Number. 1 is MONO. 2 is Stereo.
	AEC_CONFIG_ID_PLAYBACK_CH_NO,   ///< Set Playback Channel Number. 1 is MONO. 2 is Stereo.

	AEC_CONFIG_ID_NOISE_CANEL_EN,   ///< Set Noise Suppression Enable/Disable. Default is ENABLE.
	AEC_CONFIG_ID_NOISE_CANCEL_LVL, ///< Defualt is -20dB. Suggest value range -3 ~ -40. Unit in dB.
	AEC_CONFIG_ID_ECHO_CANCEL_LVL,  ///< Defualt is -50dB. Suggest value range -30 ~ -60. Unit in dB.

	AEC_CONFIG_ID_NONLINEAR_PROCESS_EN,///< Set Non Linear Process Enable/Disable. Default is DISABLE.
	AEC_CONFIG_ID_AR_MODULE_EN,     ///< Set Amplitude Rate Module Enable/Disable. Default is ENABLE. DISABLE this module if farend signal is saturated.


	AEC_CONFIG_ID_RECORD_ALIGN,     ///< Add 0x00 to the record buffer to align the playback and record buffer.
									///< Unit in samples. For SampleRate larger than 19.2KHz, the max value is 512 samples.
									///< Otherwise(8KHz), the max value is 256 samples. Default is 0.
									///< This is used during "playback start first" so we need to add delay to record buffer.
	AEC_CONFIG_ID_PLAYBACK_ALIGN,   ///< Add 0x00 to the playback buffer to align the playback and record buffer.
									///< Unit in samples. For SampleRate larger than 19.2KHz, the max value is 512 samples.
									///< Otherwise(8KHz), the max value is 256 samples. Default is 0.
									///< This is used during "record start first" so we need to add delay to playback buffer.
	AEC_CONFIG_ID_LEAK_ESTIMTAE_EN, ///< Enable/Disable the AEC leak estimate.
	AEC_CONFIG_ID_LEAK_ESTIMTAE_VAL,///< Initial Condition of the leak estimate. Please use AEC_QCONSTANT16( 0.25 ~ 0.99) as input parameter.

	AEC_CONFIG_ID_FILTER_LEN,		///< Set AEC internal Filter Length. Set to 0 is using default value. Default is 0.
	AEC_CONFIG_ID_FRAME_SIZE,		///< Set AEC internal Frame Size. Set to 0 is using default value. Default is 0.

	AEC_CONFIG_ID_NOTCH_RADIUS,     ///< Set the Notch filter Radius. AEC_QCONSTANT16(0.992,15) is the default value. Adjustible range 0 ~ 1.
									///< Please use AEC_QCONST16( 0.000 ~ 1.000,15) as input parameter.

	AEC_CONFIG_ID_PRELOAD_EN,       ///< Set Preload filter coefficient Enable/Disable. Default is DISABLE.
	AEC_CONFIG_ID_FOREADDR,         ///< Set Buffer Address for Preload FORE filter coefficient
	AEC_CONFIG_ID_BACKADDR,         ///< Set Buffer Address for Preload BACK filter coefficient
	AEC_CONFIG_ID_FORESIZE,         ///< Set Buffer Size for Preload FORE filter coefficient, return FALSE if not enough.
	AEC_CONFIG_ID_BACKSIZE,         ///< Set Buffer Size for Preload BACK filter coefficient, return FALSE if not enough.

	AEC_CONFIG_ID_BUF_ADDR,			///< AEC Lib internal buffer address.
	AEC_CONFIG_ID_BUF_SIZE,			///< AEC Lib buffer size provided by upper layer. Must be larger than audlib_aec_get_required_buffer_size().

	AEC_CONFIG_ID_SPK_NUMBER,       ///< Default is 1. Set to 2 and set AEC_CONFIG_ID_RECORD_CH_NO/AEC_CONFIG_ID_PLAYBACK_CH_NO to 2 to change AEC to DUAL MONO mode.
	AEC_CONFIG_ID_RESERVED_1,       // Default is 1. DO NOT MODIFY THIS.
	AEC_CONFIG_ID_RESERVED_2,       // Default is 0. DO NOT MODIFY THIS.
	ENUM_DUMMY4WORD(AEC_CONFIG_ID)
} AEC_CONFIG_ID;


typedef enum {

	AEC_BUFINFO_ID_INTERNAL,
	AEC_BUFINFO_ID_FORESIZE,
	AEC_BUFINFO_ID_BACKSIZE,

	ENUM_DUMMY4WORD(AEC_BUFINFO_ID)
} AEC_BUFINFO_ID;



/**
	Structure for Audio Echo Cancellation(AEC) input BitStream(BS) information

	Structure for audio Echo Cancellation input BitStream information, such as BS length and BS buffer address.
	The bitstram_buffer_length must be multiple of 1024 samples. This is used in audlib_aec_run().
*/
typedef struct {

	UINT32      bitstream_buffer_play_in;			///< Audio Playback buffer address for AEC input.
	UINT32      bitstream_buffer_record_in;		///< Audio Record buffer address for AEC input.
	UINT32      bitstram_buffer_out;	///< AEC output BitStream buffer Address.

	UINT32      bitstram_buffer_length; ///< Audio Stream Length in Samples count.
										///< This value must be multiples of 1024.

} AEC_BITSTREAM, *PAEC_BITSTREAM;

typedef struct {

	UINT32      internal_buf_sz;		///< AEC internal needed buffer size.
	UINT32      fore_buf_sz;			///< AEC Preload Foreground Coefficient buffer size.
	UINT32      back_buf_sz;			///< AEC Preload Background Coefficient buffer size.

} AEC_BUFSIZE_INFO, *PAEC_BUFSIZE_INFO;


#define AEC_QCONSTANT16(x) ((INT32)(.5+(x)*(((INT32)1)<<(15))))


//
//  Exporting APIs
//

extern ER       audlib_aec_open(void);
extern BOOL     audlib_aec_is_opened(void);
extern ER       audlib_aec_close(void);
extern void     audlib_aec_set_config(AEC_CONFIG_ID aec_sel, INT32 aec_cfg_value);
extern BOOL     audlib_aec_init(void);
extern BOOL     audlib_aec_run(PAEC_BITSTREAM p_aec_io);
extern UINT32   audlib_aec_get_config(AEC_CONFIG_ID aec_sel);
extern INT32	audlib_aec_get_required_buffer_size(AEC_BUFINFO_ID buffer_id);










/*
    AudNS Related Definitions
*/


/**
    Audio Noise Suppression(AudNS) Configuration Select ID

    This is used in audlib_ns_set_config() as the select ID to assign new configurations
    for Audio Echo Cancellation(AEC) library.
*/
typedef enum {
	AUDNS_CONFIG_ID_SAMPLERATE,       ///< Set SampleRate in Hertz. Please use AUDIO_SR as input
	AUDNS_CONFIG_ID_CHANNEL_NO,       ///< Set Record Channel Number. 1 is MONO. 2 is Stereo.

	AUDNS_CONFIG_ID_NOISE_CANCEL_LVL, ///< Defualt is -20dB. Suggest value range -3 ~ -40. Unit in dB.

	AUDNS_CONFIG_ID_BUF_ADDR,		  ///< AudNS Lib internal buffer address.
	AUDNS_CONFIG_ID_BUF_SIZE,		  ///< AudNS Lib buffer size provided by upper layer. Must be larger than audlib_ns_get_required_buffer_size().

	AUDNS_CONFIG_ID_RESERVED_1,       // Default is 1. DO NOT MODIFY THIS.
	AUDNS_CONFIG_ID_RESERVED_2,       // Default is 0. DO NOT MODIFY THIS.

	ENUM_DUMMY4WORD(AUDNS_CONFIG_ID)
} AUDNS_CONFIG_ID;

/**
    Structure for Audio Echo Cancellation(AEC) input BitStream(BS) information

    Structure for audio Echo Cancellation input BitStream information, such as BS length and BS buffer address.
    The bitstram_buffer_length must be multiple of 1024 samples. This is used in audlib_ns_run().
*/
typedef struct {
	UINT32      bitstram_buffer_in;         ///< Audio Recorded buffer address for AudNS input.
	UINT32      bitstram_buffer_out;        ///< AudNS output BitStream buffer Address.

	UINT32      bitstram_buffer_length;     ///< Audio Stream Length in Samples count.
											///< This value must be multiples of 1024.

} AUDNS_BITSTREAM, *PAUDNS_BITSTREAM;


//
//  Exporting APIs
//

extern ER       audlib_ns_open(void);
extern BOOL     audlib_ns_is_opened(void);
extern ER       audlib_ns_close(void);
extern void     audlib_ns_set_config(AUDNS_CONFIG_ID ns_sel, INT32 ns_cfg_value);
extern BOOL     audlib_ns_init(void);
extern BOOL     audlib_ns_run(PAUDNS_BITSTREAM p_ns_io);
extern INT32	audlib_ns_get_required_buffer_size(void);

int kdrv_audlib_aec_init(void);

//@}
#endif
