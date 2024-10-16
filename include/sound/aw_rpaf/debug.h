/*
 * linux/sound/aw_rpaf/debug.h -- Remote Process Audio Framework Layer
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

#ifndef _AW_RPAF_DEBUG_H
#define _AW_RPAF_DEBUG_H

#include <sound/aw_rpaf/common.h>

/*
 * param[0] = MSGBOX_SOC_DSP_AUDIO_COMMAND->MSGBOX_SOC_DSP_DEBUG_COMMAND
 * param[1] = *snd_soc_dsp_substream
 * param[2] = SND_SOC_DSP_DEBUG_COMMAND
 * param[3] = *params
 */
struct snd_soc_dsp_debug {
	uint32_t cmd_val;
	uint32_t params_val;

	struct snd_soc_dsp_pcm_params pcm_params;
	/* 共享内存地址，根据首末地址差分配空间大小 */
	uint32_t *buf;
	/* 起始地址和结束地址 */
	uint32_t addr_start;
	uint32_t addr_end;
	/* 读还是写数值 */
	uint32_t mode;

	/*API调用完毕之后需要判断该值 */
	int32_t  ret_val;
};

struct msg_debug_package {
	wait_queue_head_t tsleep;
	spinlock_t lock;
	int32_t wakeup_flag;
	struct snd_soc_dsp_debug soc_debug;
};

#endif
