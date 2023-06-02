/*
    Copyright   Novatek Microelectronics Corp. 2005~2014.  All rights reserved.

    @file       isf_audcap.h
    @ingroup    mAudCap

    @note       Nothing.

    @date       2018/06/04
*/

#ifndef _ISF_AUDCAP_H_
#define _ISF_AUDCAP_H_

#if defined __UITRON || defined __ECOS
#include "Type.h"
#include "isf_stream_def.h"
#include "isf_stream.h"
#else
#ifdef __KERNEL__
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#else
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#endif
#endif

#define AUDCAP_MAX_OUT  16 ///< max count of output of this device (interface)
#define AUDCAP_AUD_NAME_LEN 32

enum {
	AUDCAP_PARAM_START = 0x00019000,
	AUDCAP_PARAM_VOL_IMM,
	AUDCAP_PARAM_AEC_OBJ,
	AUDCAP_PARAM_CHANNEL,
	AUDCAP_PARAM_BUF_UNIT_TIME,
	AUDCAP_PARAM_BUF_COUNT,
	AUDCAP_PARAM_AEC_EN,
	AUDCAP_PARAM_ANR_OBJ,
	AUDCAP_PARAM_ANR_EN,
	AUDCAP_PARAM_AEC_PLAY_CHANNEL,
	AUDCAP_PARAM_MAX_MEM_INFO,
	AUDCAP_PARAM_AEC_BUFSIZE,
	AUDCAP_PARAM_ANR_BUFSIZE,
	AUDCAP_PARAM_AUD_CODEC,
	AUDCAP_PARAM_AUDFILT_OBJ,
	AUDCAP_PARAM_AUDFILT_EN,
	AUDCAP_PARAM_FRAME_SAMPLE,
	AUDCAP_PARAM_AEC_CONFIG,
	AUDCAP_PARAM_ANR_CONFIG,
	AUDCAP_PARAM_AEC_MONO_CHANNEL,
	AUDCAP_PARAM_BUFINFO_PHYADDR,
	AUDCAP_PARAM_BUFINFO_SIZE,
	AUDCAP_PARAM_MAX_FRAME_SAMPLE,
	AUDCAP_PARAM_SYS_INFO,
	AUDCAP_PARAM_DUAL_MONO,
	AUDCAP_PARAM_AEC_MAX_CONFIG,
	AUDCAP_PARAM_ANR_MAX_CONFIG,
	AUDCAP_PARAM_AUD_INIT_CFG,
	AUDCAP_PARAM_AUD_DEFAULT_SETTING,
	AUDCAP_PARAM_AUD_NG_THRESHOLD,
	AUDCAP_PARAM_AGC_CFG,
	AUDCAP_PARAM_AUD_ALC_EN,
	AUDCAP_PARAM_REC_SRC,
	AUDCAP_PARAM_AUD_PREPWR_EN,
	AUDCAP_PARAM_LB_CONFIG,
	AUDCAP_PARAM_DMA_ABORT,
};

typedef enum _AUDCAP_MONO {
	AUDCAP_MONO_LEFT  = 0,               ///< mono left
	AUDCAP_MONO_RIGHT = 1,               ///< mono right
	AUDCAP_MONO_MAX_CNT = 2,             ///< maximum count
	ENUM_DUMMY4WORD(AUDCAP_MONO)
} AUDCAP_MONO;

typedef enum _AUDIOCAP_LB_CH {
    AUDIOCAP_LB_CH_LEFT          = 0x00000000,  ///< AEC loopback from output left channel
	AUDIOCAP_LB_CH_RIGHT         = 0x00000001,  ///< AEC loopback from output right channel
	AUDIOCAP_LB_CH_STEREO        = 0x00000002,  ///< AEC loopback from output left and right channels
	AUDIOCAP_LB_CH_0             = 0x00000010,  ///< reserved
	AUDIOCAP_LB_CH_1             = 0x00000020,  ///< reserved
	AUDIOCAP_LB_CH_2             = 0x00000040,  ///< reserved
	AUDIOCAP_LB_CH_3             = 0x00000080,  ///< reserved
	AUDIOCAP_LB_CH_4             = 0x00000100,  ///< reserved
	AUDIOCAP_LB_CH_5             = 0x00000200,  ///< reserved
	AUDIOCAP_LB_CH_6             = 0x00000400,  ///< reserved
	AUDIOCAP_LB_CH_7             = 0x00000800,  ///< reserved
	ENUM_DUMMY4WORD(AUDIOCAP_LB_CH)
} AUDIOCAP_LB_CH;

typedef struct _AUDCAP_AEC_CONFIG {
	BOOL  enabled;               ///< AEC enable
	BOOL  leak_estimate_enabled; ///< leak estimate enable
	INT32 leak_estimate_value;   ///< initial condition of the leak estimate. value range 25 ~ 99
	INT32 noise_cancel_level;    ///< noise cancel level. suggest value range -40 ~ -3. unit in dB
	INT32 echo_cancel_level;     ///< echo cancel level. suggest value range -60 ~ -30. unit in dB
	INT32 filter_length;         ///< internal filter length
	INT32 frame_size;            ///< internal frame size
	INT32 notch_radius;          ///< notch filter radius. value range 0 ~ 1000
	AUDIOCAP_LB_CH lb_channel;
} AUDCAP_AEC_CONFIG;

typedef struct _AUDCAP_ANR_CONFIG {
	BOOL  enabled;             ///< ANR enable
	INT32 suppress_level;      ///< maximum suppression level of noise
	INT32 hpf_cut_off_freq;    ///< cut-off frequency of HPF pre-filtering
	INT32 bias_sensitive;      ///< bias sensitive
} AUDCAP_ANR_CONFIG;

typedef struct _AUDCAP_AGC_CONFIG {
	BOOL   enable;
	UINT32 decay_time;
	UINT32 attack_time;
	INT32  target_lvl;
	INT32  max_gain;
	INT32  min_gain;
	INT32  ng_threshold;
} AUDCAP_AGC_CONFIG;

typedef struct {
	ER(*open)(void);
	ER(*close)(void);
	ER(*start)(INT32, INT32, INT32);
	ER(*stop)(void);
	ER(*apply)(UINT32, UINT32, UINT32, UINT32);
	void(*enable)(BOOL);
	UINT32(*getbuf)(UINT32);
	ER(*setbuf)(UINT32, UINT32);
} AUDCAP_AEC_OBJ, *PAUDCAP_AEC_OBJ;

typedef struct {
	ER(*open)(INT32, INT32);
	ER(*close)(void);
	ER(*apply)(UINT32, UINT32);
	void(*enable)(BOOL);
	UINT32(*getbuf)(INT32, INT32);
	void(*setbuf)(UINT32, UINT32);
} AUDCAP_ANR_OBJ, *PAUDCAP_ANR_OBJ;

typedef struct _AUDCAP_AUDSRC_OBJ {
	int(*pre_init)(int, int, int, int);
	int(*init)(int*, int, int, int, int, short*);
	int(*apply)(int, void*, void*);
	void(*destroy)(int);
} AUDCAP_AUDSRC_OBJ, *PAUDCAP_AUDSRC_OBJ;

typedef struct _AUDCAP_SYSINFO {
	UINT32  cur_in_sample_rate;                       ///< input sample rate
	UINT32  cur_sample_bit;                           ///< sample bit width
	UINT32  cur_mode;                                 ///< sound mode
	UINT32  cur_out_sample_rate[AUDCAP_MAX_OUT];       ///< output sample rate
} AUDCAP_SYSINFO;

typedef struct _AUDCAP_AUD_PINMUX_CFG {
	UINT32 audio_pinmux;
	UINT32 cmd_if_pinmux;
} AUDCAP_AUD_PINMUX_CFG;

typedef struct _AUDCAP_AUD_INIT_PIN_CFG {
	AUDCAP_AUD_PINMUX_CFG pinmux;
} AUDCAP_AUD_INIT_PIN_CFG;

typedef struct _AUDCAP_AUD_INIT_I2S_CFG {
	UINT32 bit_width;
	UINT32 bit_clk_ratio;
	UINT32 op_mode;
	UINT32 tdm_ch;
} AUDCAP_AUD_INIT_I2S_CFG;

typedef struct _AUDCAP_AUD_INIT_CFG_OBJ {
	AUDCAP_AUD_INIT_PIN_CFG pin_cfg;
	AUDCAP_AUD_INIT_I2S_CFG i2s_cfg;
} AUDCAP_AUD_INIT_CFG_OBJ;

typedef struct _AUDCAP_AUD_INIT_CFG {
	CHAR driver_name[AUDCAP_AUD_NAME_LEN];
	AUDCAP_AUD_INIT_CFG_OBJ aud_init_cfg;
} AUDCAP_AUD_INIT_CFG;

typedef struct _AUDCAP_LB_CONFIG {
	BOOL  enabled;
	AUDIOCAP_LB_CH lb_channel;
} AUDCAP_LB_CONFIG;

extern ISF_UNIT isf_audcap;

#endif //_ISF_AUDCAP_H_
