/*
    Copyright   Novatek Microelectronics Corp. 2005~2014.  All rights reserved.

    @file       isf_audout.h
    @ingroup    mAudOut

    @note       Nothing.

    @date       2018/06/04
*/

#ifndef _ISF_AUDOUT_H
#define _ISF_AUDOUT_H

#if defined __UITRON || defined __ECOS
#include "Type.h"
#include "ImageStream.h"
#include "WavStudioTsk.h"
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

#define AUDOUT_MAX_IN   2
#define AUDOUT_AUD_NAME_LEN 32

enum {
	AUDOUT_PARAM_START = 0x00018000,
	AUDOUT_PARAM_VOL_IMM,
	AUDOUT_PARAM_OUT_DEV,
	AUDOUT_PARAM_CHANNEL,
	AUDOUT_PARAM_BUF_UNIT_TIME,
	AUDOUT_PARAM_BUF_COUNT,
	AUDOUT_PARAM_MAX_MEM_INFO,
	AUDOUT_PARAM_EN_EVT_CB,
	AUDOUT_PARAM_MAX_FRAME_SAMPLE,
	AUDOUT_PARAM_SYS_INFO,
	AUDOUT_PARAM_AUD_INIT_CFG,
	AUDOUT_PARAM_AUD_LPF_EN,
	AUDOUT_PARAM_WAIT_PUSH,
	AUDOUT_PARAM_PRELOAD_DONE,
	AUDOUT_PARAM_DONE_SIZE,
	AUDOUT_PARAM_NEEDED_BUFF,
	AUDOUT_PARAM_ALLOC_BUFF,
	AUDOUT_PARAM_CHN_STATE,
	AUDOUT_PARAM_AUD_PREPWR_EN,
	AUDOUT_PARAM_DMA_ABORT,
};

typedef enum _AUDOUT_MONO {
	AUDOUT_MONO_LEFT  = 0,               ///< mono left
	AUDOUT_MONO_RIGHT = 1,               ///< mono right
	AUDOUT_MONO_MAX_CNT = 2,             ///< maximum count
	ENUM_DUMMY4WORD(AUDOUT_MONO)
} AUDOUT_MONO;

typedef enum _AUDOUT_OUTPUT {
	AUDOUT_OUTPUT_SPK     = 0,  ///< speaker
	AUDOUT_OUTPUT_LINE    = 1,  ///< lineout
	AUDOUT_OUTPUT_ALL     = 2,  ///< speaker and lineout
	AUDOUT_OUTPUT_I2S     = 3,  ///< I2S
	AUDOUT_OUTPUT_MAX_CNT = 4,  ///< maximum count
	ENUM_DUMMY4WORD(AUDOUT_OUTPUT)
} AUDOUT_OUTPUT;

typedef struct _AUDOUT_SYSINFO {
	UINT32  cur_out_sample_rate;                     ///< output sample rate
	UINT32  cur_sample_bit;                          ///< sample bit width
	UINT32  cur_mode;                                ///< sound mode
	UINT32  cur_in_sample_rate[AUDOUT_MAX_IN];       ///< input sample rate
} AUDOUT_SYSINFO;

typedef struct _AUDOUT_AUDSRC_OBJ {
	int(*pre_init)(int, int, int, int);
	int(*init)(int*, int, int, int, int, short*);
	int(*apply)(int, void*, void*);
	void(*destroy)(int);
} AUDOUT_AUDSRC_OBJ, *PAUDOUT_AUDSRC_OBJ;

typedef struct _AUDOUT_AUD_PINMUX_CFG {
	UINT32 audio_pinmux;
	UINT32 cmd_if_pinmux;
} AUDOUT_AUD_PINMUX_CFG;

typedef struct _AUDOUT_AUD_INIT_PIN_CFG {
	AUDOUT_AUD_PINMUX_CFG pinmux;
} AUDOUT_AUD_INIT_PIN_CFG;

typedef struct _AUDOUT_AUD_INIT_I2S_CFG {
	UINT32 bit_width;
	UINT32 bit_clk_ratio;
	UINT32 op_mode;
	UINT32 tdm_ch;
} AUDOUT_AUD_INIT_I2S_CFG;

typedef struct _AUDOUT_AUD_INIT_CFG_OBJ {
	AUDOUT_AUD_INIT_PIN_CFG pin_cfg;
	AUDOUT_AUD_INIT_I2S_CFG i2s_cfg;
} AUDOUT_AUD_INIT_CFG_OBJ;

typedef struct _AUDOUT_AUD_INIT_CFG {
	CHAR driver_name[AUDOUT_AUD_NAME_LEN];
	AUDOUT_AUD_INIT_CFG_OBJ aud_init_cfg;
} AUDOUT_AUD_INIT_CFG;

typedef struct _AUDOUT_AUD_MEM {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
} AUDOUT_AUD_MEM;

typedef struct _AUDOUT_AUDFILT_OBJ {
	BOOL(*open)(UINT32 , BOOL);
	BOOL(*close)(void);
	BOOL(*apply)(UINT32, UINT32, UINT32);
	void(*design)(void);
} AUDOUT_AUDFILT_OBJ, *PAUDOUT_AUDFILT_OBJ;

typedef struct _AUDOUT_CHN_STATE {
	UINT32 total_num;
	UINT32 free_num;
	UINT32 busy_num;
} AUDOUT_CHN_STATE;

extern ISF_UNIT isf_audout0;
extern ISF_UNIT isf_audout1;

#endif //ISF_AUDOUT_H

