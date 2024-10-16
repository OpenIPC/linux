/*
 * sound\soc\sunxi\hifi-dsp\sunxi-cpudai.h
 *
 * (C) Copyright 2019-2025
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
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
#ifndef _SUNXI_CPUDAI_H
#define _SUNXI_CPUDAI_H

#include "sunxi-hifi-pcm.h"

struct sunxi_cpudai_info {
	struct device *dev;
	struct snd_soc_dai *cpu_dai;
	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;

	/* for hifi */
	unsigned int capturing;
	unsigned int playing;
	char wq_capture_name[32];
	struct mutex rpmsg_mutex_capture;
	struct workqueue_struct *wq_capture;
	struct delayed_work trigger_work_capture;
	char wq_playback_name[32];
	struct mutex rpmsg_mutex_playback;
	struct workqueue_struct *wq_playback;
	struct delayed_work trigger_work_playback;

	struct msg_substream_package msg_playback;
	struct msg_substream_package msg_capture;

	/* init for mixer move to codec driver */
	//struct msg_mixer_package msg_mixer;
	struct msg_debug_package msg_debug;
	struct snd_dsp_component dsp_playcomp;
	struct snd_dsp_component dsp_capcomp;

	/* eg:0 sndcodec; 1 snddmic; 2 snddaudio0; */
	unsigned int dsp_card;
	/* default is 0, for reserved */
	unsigned int dsp_device;
};

#endif
