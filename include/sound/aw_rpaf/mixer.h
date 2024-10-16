/*
 * linux/sound/aw_rpaf/mixer.h -- Remote Process Audio Framework Layer
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

#ifndef _AW_RPAF_MIXER_H_
#define _AW_RPAF_MIXER_H_

#include <sound/aw_rpaf/common.h>

/*
 * param[0] = MSGBOX_SOC_DSP_AUDIO_COMMAND->MSGBOX_SOC_DSP_AUDIO_MIXER_COMMAND
 * param[1] = *snd_soc_dsp_mixer
 * param[2] = SND_SOC_DSP_*_COMMAND
 * param[3] = *params/NULL
 */
struct snd_soc_dsp_mixer {
	uint32_t id;
	unsigned char used;

	uint32_t cmd_val;
	uint32_t params_val;

	/* eg:0 sndcodec; 1 snddmic; 2 snddaudio0; 3 snddaudio1 */
	int32_t card;
	int32_t device;
	/*
	 * 根据名字匹配:
	 * 0: maudiocodec; 1: msnddmic; 2: msnddaudio0; 3: msnddaudio1;
	 */
	char driver[32];

	/* ctl name length */
	char ctl_name[44];
	uint32_t value;

	/*API调用完毕之后需要判断该值 */
	int32_t ret_val;

	struct list_head list;
};

struct snd_dsp_hal_mixer_ops {
	//int32_t (*open)(struct snd_dsp_hal_mixer *mixer);
	int32_t (*open)(void *mixer);
	int32_t (*close)(void *mixer);
	int32_t (*read)(void *mixer);
	int32_t (*write)(void *mixer);
};

struct msg_mixer_package {
	wait_queue_head_t tsleep;
	spinlock_t lock;
	int32_t wakeup_flag;
	struct snd_soc_dsp_mixer soc_mixer;
};

#endif

