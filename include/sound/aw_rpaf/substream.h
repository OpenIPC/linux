/*
 * linux/sound/aw_rpaf/substream.h -- Remote Process Audio Framework Layer
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

#ifndef _AW_RPAF_SUBSTREAM_H_
#define _AW_RPAF_SUBSTREAM_H_

#include <sound/pcm.h>
#include <sound/aw_rpaf/common.h>
#include <sound/aw_rpaf/component.h>

struct sram_buffer {
	void *buf_addr;
	unsigned char used;
	struct list_head list;
};

/*
 * param[0] = MSGBOX_SOC_DSP_AUDIO_COMMAND->MSGBOX_SOC_DSP_*_COMMAND
 * param[1] = *snd_soc_dsp_substream
 * param[2] = SND_SOC_DSP_*_COMMAND
 * param[3] = *params/NULL
 */
struct snd_soc_dsp_substream {
	uint32_t id;
	unsigned char used;

	uint32_t cmd_val;
	uint32_t params_val;
	uint32_t audio_standby;

	struct snd_soc_dsp_pcm_params params;

	/* share data address */
	uint32_t input_addr;
	uint32_t output_addr;
	/* data_length < buf_size */
	uint32_t input_size;
	uint32_t output_size;

	/*API调用完毕之后需要判断该值 */
	int32_t ret_val;

	struct list_head list;
};

//共享内存分配：
//（1）用于音频对象的存储
//（2）用于音频共享数据

//策略：
//每次都会传输共用一个对象区域，附带共享音频buffer
struct snd_soc_dsp_queue_item {
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_component *soc_comp;
	struct snd_soc_dsp_mixer *soc_mixer;
	struct snd_soc_dsp_debug *soc_debug;

	uint32_t msg_val;
	uint32_t cmd_val;
	uint32_t param_val;
};

/*
 * For DSP Audio Framework API
 */
struct snd_dsp_hal_substream_ops {
	/*
	 * ALSA PCM audio operations - all optional.
	 * Called by soc-core during audio PCM operations.
	 */
	/* 对接声卡的开关操作 */
	//int32_t (*startup)(struct snd_dsp_hal_substream *substream);
	int32_t (*startup)(void *substream);
	void (*shutdown)(void *substream);

	int32_t (*prepare)(void *substream);
	int32_t (*start)(void *substream);
	int32_t (*stop)(void *substream);

	/* 将音频PCM格式传入进行设置 */
	int32_t (*hw_params)(void *substream);

	/* 用于数据的读操作, 数据最后才给到substream->soc_substream->buf_addr */
	snd_pcm_sframes_t (*readi)(void *substream);
	/* 用于数据的写操作, 数据最后才给到substream->soc_substream->buf_addr */
	snd_pcm_sframes_t (*writei)(void *substream);

	uint32_t (*status_params)(void *substream,
				//enum SND_SOC_DSP_PARAMS_COMMAND
				enum SND_SOC_DSP_PCM_COMMAND cmd,
				void *params);
};

struct snd_dsp_hal_substream_driver {
	//int32_t (*probe)(struct snd_dsp_hal_substream *substream);
	int32_t (*probe)(void *substream);
	int32_t (*remove)(void *substream);
	int32_t (*suspend)(void *substream);
	int32_t (*resume)(void *substream);
};

struct msg_substream_package {
	wait_queue_head_t tsleep;
	spinlock_t lock;
	int32_t wakeup_flag;
	struct snd_soc_dsp_substream soc_substream;
	struct snd_pcm_substream *substream;
};

#endif

