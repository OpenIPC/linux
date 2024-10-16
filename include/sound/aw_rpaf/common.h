/*
 * linux/sound/aw_rpaf/common.h -- Remote Process Audio Framework Layer
 *
 * (C) Copyright 2019-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef _AW_RPAF_COMMON_H_
#define _AW_RPAF_COMMON_H_

#define SRAM_PCM_CACHE_QUEUE 16U
#define DSP_SOUND_CARDS 4U

#include <uapi/sound/asound.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <sound/soc-topology.h>

static const char *const awrpaf_info_pre = "AWRPAF_INFO";
static const char *const awrpaf_debug_pre = "AWRPAF_DEBUG";
static const char *const awrpaf_err_pre = "AWRPAF_ERR";

#if 0
#define awrpaf_info(fmt, ...) \
	printk(KERN_INFO pr_fmt("[%s][%s:%d]"fmt), \
			awrpaf_info_pre, __func__, __LINE__, ##__VA_ARGS__)
#else
#define awrpaf_info(fmt, ...)
#endif

#if 0
#define awrpaf_debug(fmt, ...) \
	printk(KERN_DEBUG pr_fmt("[%s][%s:%d]"fmt), \
			awrpaf_debug_pre, __func__, __LINE__, ##__VA_ARGS__)
#else
#define awrpaf_debug(fmt, ...)
#endif

#define awrpaf_err(fmt, ...) \
	printk(KERN_ERR pr_fmt("[%s][%s:%d]"fmt), \
			awrpaf_err_pre, __func__, __LINE__, ##__VA_ARGS__)

/* the arm is 64bit cpu and dsp is 32bit cpu */
struct msg_audio_package {
	uint32_t audioMsgVal;
	uint32_t sharePointer;
};

/** PCM sample format */
enum hifi_pcm_format {
	/** Unknown */
	SND_PCM_FORMAT_UNKNOWN = -1,
	/** Signed 8 bit */
	SND_PCM_FORMAT_S8 = 0,
	/** Unsigned 8 bit */
	SND_PCM_FORMAT_U8,
	/** Signed 16 bit Little Endian */
	SND_PCM_FORMAT_S16_LE,
	/** Signed 16 bit Big Endian */
	SND_PCM_FORMAT_S16_BE,
	/** Unsigned 16 bit Little Endian */
	SND_PCM_FORMAT_U16_LE,
	/** Unsigned 16 bit Big Endian */
	SND_PCM_FORMAT_U16_BE,
	/** Signed 24 bit Little Endian using low three bytes in 32-bit word */
	SND_PCM_FORMAT_S24_LE,
	/** Signed 24 bit Big Endian using low three bytes in 32-bit word */
	SND_PCM_FORMAT_S24_BE,
	/** Unsigned 24 bit Little Endian using low three bytes in 32-bit word */
	SND_PCM_FORMAT_U24_LE,
	/** Unsigned 24 bit Big Endian using low three bytes in 32-bit word */
	SND_PCM_FORMAT_U24_BE,
	/** Signed 32 bit Little Endian */
	SND_PCM_FORMAT_S32_LE,
	/** Signed 32 bit Big Endian */
	SND_PCM_FORMAT_S32_BE,
	/** Unsigned 32 bit Little Endian */
	SND_PCM_FORMAT_U32_LE,
	/** Unsigned 32 bit Big Endian */
	SND_PCM_FORMAT_U32_BE,

	/* only support little endian */
	/** Signed 16 bit CPU endian */
	SND_PCM_FORMAT_S16 = SND_PCM_FORMAT_S16_LE,
	/** Unsigned 16 bit CPU endian */
	SND_PCM_FORMAT_U16 = SND_PCM_FORMAT_U16_LE,
	/** Signed 24 bit CPU endian */
	SND_PCM_FORMAT_S24 = SND_PCM_FORMAT_S24_LE,
	/** Unsigned 24 bit CPU endian */
	SND_PCM_FORMAT_U24 = SND_PCM_FORMAT_U24_LE,
	/** Signed 32 bit CPU endian */
	SND_PCM_FORMAT_S32 = SND_PCM_FORMAT_S32_LE,
	/** Unsigned 32 bit CPU endian */
	SND_PCM_FORMAT_U32 = SND_PCM_FORMAT_U32_LE,

	SND_PCM_FORMAT_LAST = SND_PCM_FORMAT_U32_BE,
};

enum snd_stream_direction {
	SND_STREAM_PLAYBACK = 0,
	SND_STREAM_CAPTURE,
};

enum snd_codec_type {
	SND_CODEC_TYPE_PCM,
	SND_CODEC_TYPE_MP3,
	SND_CODEC_TYPE_AAC,
	SND_CODEC_TYPE_OTHER,
};

enum snd_data_type {
	SND_DATA_TYPE_PCM,
	SND_DATA_TYPE_RAW,
	SND_DATA_TYPE_OTHER,
};

enum MSGBOX_SOC_DSP_AUDIO_COMMAND {
	MSGBOX_SOC_DSP_AUDIO_NULL_COMMAND = 0,
	/* PCM stream的同步操作接口 */
	MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND,
	/* 该定义各式各样并且种类过多， 暂不做全功能支持，后者后期迭代增加 */
	MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND,

	/* for misc driver */
	/* 该定义用于调试，如：外挂codec寄存器的读写 */
	MSGBOX_SOC_DSP_AUDIO_DEBUG_COMMAND,
	/* 该定义用于组件数据读写 */
	MSGBOX_SOC_DSP_AUDIO_COMPONENT_COMMAND,
};

/*
 * param[0] = MSGBOX_SOC_DSP_AUDIO_COMMAND->MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND
 * param[1] = *snd_soc_dsp_substream
 * param[2] = SND_SOC_DSP_PCM_COMMAND
 * param[3] = *params
 */
enum SND_SOC_DSP_PCM_COMMAND {
	/*
	 * for cpudai driver and machine driver
	 * need to get status after execing at dsp & sync
	 */
	SND_SOC_DSP_PCM_PROBE,
	SND_SOC_DSP_PCM_SUSPEND,
	SND_SOC_DSP_PCM_RESUME,
	SND_SOC_DSP_PCM_REMOVE,

	/* the cmd of pcm stream interface & sync */
	SND_SOC_DSP_PCM_STARTUP,
	SND_SOC_DSP_PCM_HW_PARAMS,
	SND_SOC_DSP_PCM_PREPARE,
	SND_SOC_DSP_PCM_WRITEI,
	SND_SOC_DSP_PCM_READI,
	SND_SOC_DSP_PCM_START,
	SND_SOC_DSP_PCM_STOP,
	SND_SOC_DSP_PCM_DRAIN,
	SND_SOC_DSP_PCM_SHUTDOWN,
};

/*
 * param[0] = MSGBOX_DSP_AUDIO_COMMAND->MSGBOX_DSP_AUDIO_CTL_COMMAND
 * param[1] = *snd_dsp_hal_mixer
 * param[2] = SND_SOC_DSP_MIXER_COMMAND
 * param[3] = NULL
 */
enum SND_SOC_DSP_MIXER_COMMAND {
	/* the cmd of interface & sync */
	SND_SOC_DSP_MIXER_OPEN = 0,
	SND_SOC_DSP_MIXER_WRITE,
	SND_SOC_DSP_MIXER_READ,
	SND_SOC_DSP_MIXER_CLOSE,
};

/*
 * param[0] = MSGBOX_SOC_DSP_AUDIO_COMMAND->MSGBOX_SOC_DSP_DEBUG_COMMAND
 * param[1] = *snd_soc_dsp_substream
 * param[2] = SND_SOC_DSP_DEBUG_COMMAND
 * param[3] = *params
 */
enum SND_SOC_DSP_DEBUG_COMMAND {
	/* for getting params */
	SND_SOC_DSP_DEBUG_GET_REG,
	/* for setting params */
	SND_SOC_DSP_DEBUG_SET_REG,
	/* the cmd of pcm stream status interface & sync */
	SND_SOC_DSP_DEBUG_GET_HWPARAMS,
	SND_SOC_DSP_DEBUG_GET_PCM_STATUS,
};

/*
 * param[0] = MSGBOX_DSP_AUDIO_COMMAND->MSGBOX_DSP_AUDIO_COMPONENT_COMMAND
 * param[1] = *snd_dsp_hal_component
 * param[2] = SND_SOC_DSP_COMPONENT_COMMAND
 * param[3] = NULL
 */
enum SND_SOC_DSP_COMPONENT_COMMAND {
	/* the cmd of interface & sync */
	SND_SOC_DSP_COMPONENT_CREATE,
	SND_SOC_DSP_COMPONENT_REMOVE,
	SND_SOC_DSP_COMPONENT_SUSPEND,
	SND_SOC_DSP_COMPONENT_RESUME,
	SND_SOC_DSP_COMPONENT_STATUS,
	SND_SOC_DSP_COMPONENT_SW_PARAMS,
	SND_SOC_DSP_COMPONENT_START,
	SND_SOC_DSP_COMPONENT_STOP,
	SND_SOC_DSP_COMPONENT_WRITE,
	SND_SOC_DSP_COMPONENT_READ,

	/* stream component */
	SND_SOC_DSP_COMPONENT_SET_STREAM_PARAMS,

	/* algo control */
	SND_SOC_DSP_COMPONENT_ALGO_GET,
	SND_SOC_DSP_COMPONENT_ALGO_SET,
};

/* 判断当前状态 */
enum SND_DSP_COMPONENT_STATE {
	SND_DSP_COMPONENT_STATE_OPEN,
	SND_DSP_COMPONENT_STATE_CREATE,
	SND_DSP_COMPONENT_STATE_SETUP,
	SND_DSP_COMPONENT_STATE_START,
	SND_DSP_COMPONENT_STATE_RUNNING,
	SND_DSP_COMPONENT_STATE_STOP,
	SND_DSP_COMPONENT_STATE_REMOVE,
	SND_DSP_COMPONENT_STATE_CLOSE,
};

/* 用于dsp任务之间信息单元传递 */
struct snd_dsp_hal_queue_item {
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_component *soc_component;
	struct snd_soc_dsp_mixer *soc_mixer;
	struct snd_soc_dsp_debug *soc_debug;

	struct snd_dsp_hal_substream *hal_substream;
	struct snd_dsp_hal_component *hal_component;
	struct snd_dsp_hal_mixer *hal_mixer;

	uint32_t pxAudioMsgVal;
	unsigned char used;
	struct list_head list;

	int32_t ret_val;
};

/* 会缓存在dram中 */
struct snd_soc_dsp_sdram_buf {
	void *read_buf;
	uint32_t read_size;
	struct list_head list;
};

struct snd_dsp_hal_component_process_driver {
	/* 分配自身算法需要的结构空间变量和配置参数 */
	//int32_t (*create)(void **handle, struct snd_soc_dsp_native_component *native_component);
	int32_t (*create)(void **handle, void *native_component);
	/* 实现该算法处理buffer，并回填buffer, 算法设计不同时更改输入和输出buf */
	int32_t (*process)(void *handle, void *input_buffer, uint32_t * const input_size,
				void *output_buffer, uint32_t * const output_size);
	/* 算法资源释放 */
	int32_t (*release)(void *handle);
};

struct snd_soc_dsp_pcm_params {
	/* eg:0 sndcodec; 1 snddmic; 2 snddaudio0; 3 snddaudio1 */
	int32_t card;
	int32_t device;
	/*
	 * 根据名字匹配:
	 * 0: hw:audiocodec;
	 * 1: hw:snddmic;
	 * 2: hw:snddaudio0;
	 * 3: hw:snddaudio1;
	 */
	char driver[32];
	/* 1:capture; 0:playback */
	enum snd_stream_direction stream;

	/* -- HW params -- */
	snd_pcm_format_t format;		/* SNDRV_PCM_FORMAT_* */
	uint32_t rate;				/* rate in Hz */
	uint32_t channels;
	uint32_t resample_rate;
	/* only for hw substream */
	uint32_t period_size; /* 中断周期 */
	uint32_t periods;          /* 中断周期个数 */
	/* 在流中buffer务必一致大小, 代码中务必检查！ */
	uint32_t buffer_size;	/* 共享buf大小 */
	uint32_t pcm_frames;

	/* data type */
	enum snd_data_type data_type;
	/* mp3 - aac */
	enum snd_codec_type codec_type;
	/* dsp pcm status */
	int32_t status;

	/* 从设备树种获取的私有数据 */
	uint32_t dts_data;

	/* for dsp0 is 1, for dsp1 is 0 */
	uint32_t hw_stream;

	/* dsp data transmission mode */
	uint32_t data_mode;
	/* soc stream wake/sleep */
	uint32_t stream_wake;

	/* 独立算法组件用到的参数:buffer大小 */
	unsigned int input_size;
	unsigned int output_size;
	unsigned int dump_size;
	/* 保存算法用到的参数，具体由算法定义，预留32字节 */
	uint32_t algo_params[8];
};

enum snd_dsp_handle_type {
	HANDLE_TYPE_NULL = 0,
	HANDLE_TYPE_SOC_MIXER,
	HANDLE_TYPE_HAL_MIXER,
	HANDLE_TYPE_HAL_SUBSTREAM,
	HANDLE_TYPE_HAL_COMPONENT,
};

enum snd_dsp_list_type {
	LIST_TYPE_NULL = 0,
	LIST_TYPE_HAL_MIXER,
	LIST_TYPE_SOC_MIXER,
	LIST_TYPE_HAL_SUBSTREAM,
	LIST_TYPE_SOC_SUBSTREAM,
	LIST_TYPE_HAL_COMPONENT,
	LIST_TYPE_SOC_COMPONENT,
};

#endif
