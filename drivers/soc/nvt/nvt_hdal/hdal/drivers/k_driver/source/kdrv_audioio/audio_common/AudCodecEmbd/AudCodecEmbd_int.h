/*
    Build in audio codec driver internal header

    Build in audio codec driver internal header file

    @file       AudCodecEmbd_int.h
    @ingroup    mISYSAud
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifndef _AUD_EMBD_INT_H
#define _AUD_EMBD_INT_H


#ifdef __KERNEL__
//#include "audio/Audio.h"
//#include "audio/eac.h"
#include "audio_dbg.h"
#elif defined(__FREERTOS)
#include <string.h>
#else
#include <string.h>
#include "Utility.h"
#include "Audio.h"
#include "pll.h"
#include "top.h"
#include "eac.h"
#include "timer.h"
#endif
#include "audio_int.h"

/** \addtogroup mISYSAud */
//@{

#define AUDCODEC_DEBUG              DISABLE
#if AUDCODEC_DEBUG
#define audcodec_debug(msg)         DBG_WRN msg
#else
#define audcodec_debug(msg)
#endif

#define AUDCODEC_STOP_TARGET        0
#define AUDCODEC_PGA_STEPTIME       800 //us


#define AUDCODEC_STATE_STOP         0
#define AUDCODEC_STATE_PLAY         1
#define AUDCODEC_STATE_RECORD       2

#define AUDIO_DA_SILENCE_HANDLE     DISABLE

#define AUDIO_DA_SW_DISCHARGE       0x80000000
#define AUDIO_DA_DISCHARGE_EN       0x00000001

#define AUDIO_DEPOP_PERIOD          2000000//us
#define AUDIO_DISCHARGE_PERIOD      1500000//us

/*
     AUDIO codec operation state
*/
typedef enum {
	AUDCODEC_PLAYBACK,              //< playback state
	AUDCODEC_STOP,                  //< stop state

	ENUM_DUMMY4WORD(AUDCODEC_OP)
} AUDCODEC_OP;

/*
    Structure of Embedded Audio Codec Parameters
*/
typedef struct {
	UINT32              audio_state;
	UINT32              power_state;

	AUDIO_OUTPUT        output_path;
	AUDIO_CH            output_channel;     // Playback
	AUDIO_CH            input_channel;      // Record
	AUDIO_GAIN          record_gain;
	AUDIO_VOL           play_volume;
	EAC_PGABOOST_SEL    boost_value;
	EAC_NG_THRESHOLD    noise_gate_threshold[2];

	UINT32              digital_gain2;
	AUDIO_SR            audio_sample_rate;
	AUDIO_SR            play_sample_rate;
	BOOL                b_chk_play_sample_rate;

	BOOL                b_spk_power_always_on;
	BOOL                b_headphone_power_always_on;
	BOOL                b_record_preset;
	UINT32              record_delay;
	BOOL                b_headphone_depop_en;
	BOOL                b_headphone_depop_done;
	UINT32              need_discharge;
	UINT32              play_digital_gain;
	BOOL				advcm_always_on;
} AUDCODEC_PARAM, *PAUDCODEC_PARAM;

static void audcodec_open(void);
static void audcodec_close(void);
static void audcodec_init(PAUDIO_SETTING pAudio);
static void audcodec_set_record_source(AUDIO_RECSRC RecSrc);
static void audcodec_set_output(AUDIO_OUTPUT Output);
static void audcodec_set_samplerate(AUDIO_SR SamplingRate);
static void audcodec_set_tx_channel(AUDIO_CH Channel);
static void audcodec_set_rx_channel(AUDIO_CH Channel);
static void audcodec_set_volume(AUDIO_VOL Vol);
static void audcodec_set_gain(AUDIO_GAIN Gain);
static void audcodec_set_gaindb(INT32 db);
static BOOL audcodec_set_feature(AUDIO_FEATURE Feature, BOOL bEnable);
static void audcodec_stop_record(void);
static void audcodec_stop_play(void);
static void audcodec_preset_record(void);
static void audcodec_record(void);
static void audcodec_playback(void);
static BOOL audcodec_set_parameter(AUDIO_PARAMETER Parameter, UINT32 uiSetting);
static BOOL audcodec_chk_samplerate(void);

#if !_FPGA_EMULATION_
extern void tc680_init(void);
extern ER   tc680_writeReg(UINT32 uiOffset, UINT32 uiValue);
extern ER   tc680_readReg(UINT32 uiOffset, UINT32 *puiValue);
#endif

//@}
#endif


