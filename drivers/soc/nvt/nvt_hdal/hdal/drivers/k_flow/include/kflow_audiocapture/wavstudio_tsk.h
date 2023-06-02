/**
    WavStudio module.

    @file       WavStudioTsk.h
    @ingroup    mILIBWAVSTUDIO

    @brief      Header file of WavStudio task.

                APIs/structure/macro declaration of WavStudio task.

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

*/

/**
    @addtogroup mILIBWAVSTUDIO
*/
//@{

#ifndef _WAVSTUDIOTSK_H
#define _WAVSTUDIOTSK_H

#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kdrv_audioio/kdrv_audioio.h"
#include "kdrv_type.h"
//#include "audio_common/include/Audio.h"

#if defined(__UITRON)
extern void wavstudio_install_id(void) _SECTION(".kercfg_text");
#else
extern void wavstudio_install_id(void);
extern void wavstudio_uninstall_id(void);
#endif
extern BOOL wavstudio_chk_mod_installed(void);


/**
    @name Wave studio channel
*/
//@{
//#define WAVSTUD_PLAY_AUD_CH               AUDTS_CH_TX1    ///< Play channel
//#define WAVSTUD_RECORD_AUD_CH             AUDTS_CH_RX     ///< Record channel
//#define WAVSTUD_LOOPBACK_AUD_CH           AUDTS_CH_TXLB   ///< Playback loopback channel
//@}

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

typedef enum {
	AUDIO_CH_LEFT,              ///< Left
	AUDIO_CH_RIGHT,             ///< Right
	AUDIO_CH_STEREO,            ///< Stereo
	AUDIO_CH_MONO,              ///< Mono two channel. Obselete. Shall not use this option.
	AUDIO_CH_DUAL_MONO,         ///< Dual Mono Channels. Valid for record(RX) only.

	ENUM_DUMMY4WORD(AUDIO_CH)
} AUDIO_CH;

typedef struct _AUDIO_BUF_QUEUE {
	UINT32                  uiAddress;  ///< Buffer Starting Address (Unit: byte) (Word alignment)
	UINT32                  uiSize;     ///< Buffer Size (Unit: byte) (Word Alignment)
	UINT32                  uiValidSize;///< Valid PCM data size (Unit: byte).
										///< Returend by aud_getDoneBufferFromQueue()
	struct _AUDIO_BUF_QUEUE *pNext;     ///< Next queue element
} AUDIO_BUF_QUEUE, *PAUDIO_BUF_QUEUE;


/**
    @name Wave studio task status
*/
//@{
#define WAVSTUD_STS_UNKNONWN              0x00000000      ///<
#define WAVSTUD_STS_CLOSED                0x00000001      ///< Task is not opened
#define WAVSTUD_STS_STOPPED               0x00000002      ///< Stopped (No recording or playing)
#define WAVSTUD_STS_GOING                 0x00000004      ///< Stopped (No recording or playing)
#define WAVSTUD_STS_PAUSED                0x00000008      ///< Stopped (No recording or playing)
#define WAVSTUD_STS_SAVING                0x00000010      ///< Saving remaining data in RAM to card
//@}


/**
    Stop count if non-stop playing / recording
*/
#define WAVSTUD_NON_STOP_COUNT            0xFFFFFFFFFFFFFFFFULL

#define WAVSTUD_PORT_VOLUME             0x80000000


/**
    Wave studio event IDs
*/
typedef enum _WAVSTUD_CB_EVT_ {
	WAVSTUD_CB_EVT_OK = 0,              ///< OK
	WAVSTUD_CB_EVT_FAIL,                ///< Fail
	WAVSTUD_CB_EVT_TCHIT,               ///< Time code hit
	WAVSTUD_CB_EVT_STARTED,             ///< WavStudio started
	WAVSTUD_CB_EVT_STOPPED,             ///< WavStudio stopped
	WAVSTUD_CB_EVT_PAUSED,              ///< WavStudio paused
	WAVSTUD_CB_EVT_RESUMED,             ///< WavStudio resumed
	WAVSTUD_CB_EVT_BUF_FULL,            ///< Buffer full
	WAVSTUD_CB_EVT_BUF_EMPTY,           ///< Buffer empty
	WAVSTUD_CB_EVT_READ_FAIL,           ///< Read data fail
	WAVSTUD_CB_EVT_WRITE_FAIL,          ///< Write data fail
	WAVSTUD_CB_EVT_READ_LAST,           ///< Read last data
	WAVSTUD_CB_EVT_BUF_DONE,            ///< Buffer done
	WAVSTUD_CB_EVT_PRELOAD_DONE,        ///< Preload done
	WAVSTUD_CB_EVT_NEW_ENTER,           ///< New enter
	WAVSTUD_CB_EVT_NEW_OK,              ///< New OK
	WAVSTUD_CB_EVT_NEW_FAIL,            ///< New fail
	WAVSTUD_CB_EVT_PROC_ENTER,          ///< PROC enter
	WAVSTUD_CB_EVT_PROC_OK,             ///< PROC ok
	WAVSTUD_CB_EVT_PROC_FAIL,           ///< PROC fail
	WAVSTUD_CB_EVT_MAX
} WAVSTUD_CB_EVT;

/**
    Wave studio configurations
*/
typedef enum _WAVSTUD_CFG {
	WAVSTUD_CFG_MODE,                   ///< WavStudio Mode
	WAVSTUD_CFG_MEM,                    ///< Get need memory size
	WAVSTUD_CFG_VOL,                    ///< Volume
	WAVSTUD_CFG_AUD_SR,                 ///< Sample rate
	WAVSTUD_CFG_AUD_CH,                 ///< Channel
	WAVSTUD_CFG_AUD_BIT_PER_SEC,        ///< Bit per second
	WAVSTUD_CFG_PLAY_OUT_DEV,           ///< Output device index
	WAVSTUD_CFG_GET_STATUS,             ///< WavStudio status
	WAVSTUD_CFG_HDR_ADDR,               ///< Wav header buffer address
	WAVSTUD_CFG_BUFF_UNIT_TIME,         ///< Buffer unit time
	WAVSTUD_CFG_ALLOC_MEM,              ///< Get allocated memory size
	WAVSTUD_CFG_QUEUE_ENTRY_NUM,        ///< Number of play queue entry
	WAVSTUD_CFG_BUFF_UNIT_SIZE,         ///< Size of one unit buffer
	///  = sample rate * byte per sample * channel number * buffer unit time
	///  e.g. 8K SR, 16 bps, stereo, 200ms per buffer = 8000 * 2 * 2 * 0.2
	WAVSTUD_CFG_RECV_CB,                ///< Callback function to notify the source of play data. Only for passive play mode
	WAVSTUD_CFG_AUD_EVT_CB,             ///< Callback function to notify audio driver event.
	WAVSTUD_CFG_PROC_UNIT_SIZE,
	WAVSTUD_CFG_AEC_EN,
	WAVSTUD_CFG_UNLOCK_ADDR,
	WAVSTUD_CFG_PLAY_LB_CHANNEL,
	WAVSTUD_CFG_AUD_CODEC,
	WAVSTUD_CFG_FILT_CB,
	WAVSTUD_CFG_DEFAULT_SETTING,
	WAVSTUD_CFG_NG_THRESHOLD,
	WAVSTUD_CFG_PLAY_WAIT_PUSH,
	WAVSTUD_CFG_ALC_EN,
	WAVSTUD_CFG_REC_SRC,
	WAVSTUD_CFG_VOL_P0,                 ///< Volume 0
	WAVSTUD_CFG_VOL_P1,                 ///< Volume 1
	WAVSTUD_CFG_MAX
} WAVSTUD_CFG;

/**
    Wave studio act types
*/
typedef enum _WAVSTUD_ACT {
	WAVSTUD_ACT_PLAY,                   ///< Play
	WAVSTUD_ACT_REC,                    ///< Record 1
	WAVSTUD_ACT_REC2,                   ///< Record 2
	WAVSTUD_ACT_LB,
	WAVSTUD_ACT_PLAY2,
	WAVSTUD_ACT_MAX,
} WAVSTUD_ACT;

typedef enum _WAVSTUD_PORT {
	WAVSTUD_PORT_GLOBAL,
	WAVSTUD_PORT_0,
	WAVSTUD_PORT_1,
	WAVSTUD_PORT_2,
	WAVSTUD_PORT_3,
	WAVSTUD_PORT_4,
	WAVSTUD_PORT_5,
	WAVSTUD_PORT_6,
	WAVSTUD_PORT_7,
	WAVSTUD_PORT_MAX,
} WAVSTUD_PORT;

/**
    Wave studio memory
*/
typedef struct _WAVSTUD_MEM {
	UINT32 uiAddr;                      ///< Start Memory Address
	UINT32 uiSize;                      ///< Start Memory Size
} WAVSTUD_MEM, *PWAVSTUD_MEM;

/**
    Wave studio event callback function prototype
*/
typedef void (*WAVSTUD_CB)(WAVSTUD_ACT act, UINT32 p1, UINT32 p2);

typedef void (*WAVSTUD_FILTCB)(UINT32 inaddr, UINT32 outaddr, UINT32 length);

/**
    Wave studio memory allocation
*/
typedef struct {
	WAVSTUD_MEM     mem;
	WAVSTUD_CB      wavstud_cb;     ///< Wave studio record/playback callback
} WAVSTUD_APPOBJ, *PWAVSTUD_APPOBJ;

/**
    Bit per sample
*/
typedef enum _WAVSTUD_BITS_PER_SAM {
	WAVSTUD_BITS_PER_SAM_8  = 0x08,     ///< 8 bits PCM
	WAVSTUD_BITS_PER_SAM_16 = 0x10,     ///< 16 bits PCM
} WAVSTUD_BITS_PER_SAM;

/**
    Codec Type
*/
typedef enum {
	WAVSTUDCODEC_PCM,                   ///< PCM
	WAVSTUDCODEC_MAX
} WAVSTUD_CODEC;

/**
    Wave studio audio info
*/
typedef struct _WAVSTUD_AUD_INFO {
	AUDIO_SR            aud_sr;          ///< Sample Rate
	AUDIO_CH            aud_ch;          ///< Channel
	UINT32              bitpersample;     ///< bit Per Sample
	UINT32				ch_num;			///< Channel number
	UINT32              buf_sample_cnt;            ///< buffer sample count
} WAVSTUD_AUD_INFO, *PWAVSTUD_AUD_INFO;


/**
    Time of one buffer
*/
typedef enum _WAVSTUD_UNIT_TIME {
	WAVSTUD_UNIT_TIME_0,
	WAVSTUD_UNIT_TIME_100MS,
	WAVSTUD_UNIT_TIME_200MS,
	WAVSTUD_UNIT_TIME_300MS,
	WAVSTUD_UNIT_TIME_400MS,
	WAVSTUD_UNIT_TIME_500MS,
	WAVSTUD_UNIT_TIME_600MS,
	WAVSTUD_UNIT_TIME_700MS,
	WAVSTUD_UNIT_TIME_800MS,
	WAVSTUD_UNIT_TIME_900MS,
	WAVSTUD_UNIT_TIME_1000MS,
} WAVSTUD_UNIT_TIME;

typedef enum {
	WAVSTUD_DATA_MODE_PUSH,
	WAVSTUD_DATA_MODE_PULL,
	WAVSTUD_DATA_MODE_PUSH_FILE,
	ENUM_DUMMY4WORD(WAVSTUD_DATA_MODE)
} WAVSTUD_DATA_MODE;

/**
    Wave studio audio codec
*/
typedef enum _WAVSTUD_AUD_CODEC {
	WAVSTUD_AUD_CODEC_EMBEDDED,
	WAVSTUD_AUD_CODEC_EXTERNAL,
} WAVSTUD_AUD_CODEC;

/**
    Audio info set
*/
typedef struct _WAVSTUD_INFO_SET {
	UINT32 obj_count;                  ///<
	WAVSTUD_DATA_MODE data_mode;

	WAVSTUD_AUD_INFO aud_info;           ///< Audio info of record
	UINT32 buf_count;                 ///< Record buffer count (Should >= 5)
	WAVSTUD_UNIT_TIME unit_time;      ///< Record unit time
} WAVSTUD_INFO_SET, *PWAVSTUD_INFO_SET;

/**
    AEC object
*/
typedef struct _WAVSTUD_AUD_AEC_OBJ {
	ER(*open)(void);
	ER(*close)(void);
	ER(*start)(INT32, INT32, INT32);
	ER(*stop)(void);
	ER(*apply)(UINT32, UINT32, UINT32, UINT32);
	void(*enable)(BOOL);
	ER(*setbuf)(UINT32, UINT32);
} WAVSTUD_AUD_AEC_OBJ, *PWAVSTUD_AUD_AEC_OBJ;

/**
    Open WAV studio task.

    Start WAV studio task.

    @param[in] pWavObj      Wav Studio application object
    @param[in] dxSndHdl     Dx sound object handle
    @param[in] pWavInfoSet  Audio info, act count, and buffer count of play and record
    @return Open status
        - @b E_SYS: Task is already opened
        - @b E_NOMEM: Memory size is not enough
        - @b E_OK: No error
*/
#if defined(__UITRON)
extern ER wavstudio_open(WAVSTUD_ACT act_type, PWAVSTUD_APPOBJ wav_obj, DX_HANDLE dxsnd_hdl, PWAVSTUD_INFO_SET wavinfo_set);
#else
extern ER wavstudio_open(WAVSTUD_ACT act_type, PWAVSTUD_APPOBJ wav_obj, UINT32 dxsnd_hdl, PWAVSTUD_INFO_SET wavinfo_set);
#endif
/**
    Close WAV studio task.

    Close WAV studio task.

    @return Close status
        - @b E_SYS: Task is already closed
        - @b E_OK: No error
*/
extern ER wavstudio_close(WAVSTUD_ACT act_type);


/**
    Start to record or play.

    Start to record or play.

    @param[in]  actType         Act type.
    @param[in]  info            Audio info configurations.
    @param[in]  stopcount       Stop sample count.
    @param[in]  DataCB          Data callback function pointer.\n
                                When actType is play, the callback function should fill sound data to the buffer.\n
                                When actType is record, the callback function should get sound data from the buffer.
    @return
        - @b TRUE: Start to record
        - @b FALSE: WAV studio task is not in stopped status
*/
extern BOOL wavstudio_start(WAVSTUD_ACT act_type, PWAVSTUD_AUD_INFO info, UINT64 stopcount, BOOL (*data_cb)(PAUDIO_BUF_QUEUE, PAUDIO_BUF_QUEUE, UINT32, UINT64));

/**
    Stop recording / playing.

    Stop recording / playing.

    @param[in]  actType         Act type.
    @return
        - @b TRUE: Stop recording / playing.
        - @b FALSE: WAV studio task doesn't recording or playing.
*/
extern BOOL wavstudio_stop(WAVSTUD_ACT act_type);

/**
    Pause recording / playing.

    Pause recording / playing.

    @param[in]  actType         Act type.
    @return
        -@ TRUE: Pause recording / playing.
        -@ FALSE: Not recording / playing.
*/
extern BOOL wavstudio_pause(WAVSTUD_ACT act_type);

/**
    Resume recording / playing.

    Resume recording / playing from pause status.

    @param[in]  actType         Act type.
    @return
        - @b TRUE: Resume from paused status.
        - @b FALSE: Not in paused status.
*/
extern BOOL wavstudio_resume(WAVSTUD_ACT act_type);

/**
    Set WavStudio configuration.

    Set WavStudio configuration.

    @param[in]  cfg             Configuration type.
    @param[in]  p1              parameter 1. (According to the confiuration type)
    @param[in]  p2              parameter 2. (According to the confiuration type)
    @return
        - @b E_OK: No error.
        - @b E_PAR: Parameter error.
*/
extern UINT32 wavstudio_set_config(WAVSTUD_CFG cfg, UINT32 p1, UINT32 p2);

/**
    Get WavStudio configuration.

    Get WavStudio configuration.

    @param[in]  cfg             Configuration type.
    @param[in]  p1              parameter 1. (According to the confiuration type)
    @param[in]  p2              parameter 2. (According to the confiuration type)
    @return     Configuration value.
*/
extern UINT32 wavstudio_get_config(WAVSTUD_CFG cfg, UINT32 p1, UINT32 p2);

/**
    Wait for WavStudio start.

    Wait for WavStudio start.

    @param[in]  actType         Act type.
*/
extern void wavstudio_wait_start(WAVSTUD_ACT act_type);

/**
    Wait for WavStudio stop.

    Wait for WavStudio stop.

    @param[in]  actType         Act type.
*/
extern void wavstudio_wait_stop(WAVSTUD_ACT act_type);

/**
    Push buffer to queue.

    Push buffer to play queue. Only valid in passive play mode.

    @param[in]      uiAddr      Buffer address.
    @param[in,out]  pSize       Pointer to the buffer size.
                                Return the actual size that is pushed into play queue.
    @return
        - @b TRUE: Buffer is successfully pushed into queue.
        - @b FALSE: Buffer is not completely pushed into queue. Refer to pSize for the actual pushed size.
*/
extern BOOL wavstudio_push_buf_to_queue(UINT32 addr, UINT32 *size);

extern BOOL wavstudio_push_buf_to_queue2(UINT32 addr, UINT32 *size);

extern UINT32 wavstudio_get_remain_buf(WAVSTUD_ACT act_type);

extern UINT32 wavstudio_get_remain_buf2(WAVSTUD_ACT act_type);

#endif

//@}
