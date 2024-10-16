/*
 * linux/sound/aw_rpaf/rpmsg_hifi.h -- Remote Process Audio Framework Layer
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

#ifndef _RPMSG_HIFI_H
#define _RPMSG_HIFI_H

#include <sound/aw_rpaf/common.h>
#include <sound/aw_rpaf/mixer.h>
#include <sound/aw_rpaf/component-core.h>

enum SUNXI_HIFI_ID {
	SUNXI_HIFI0 = 0,
	SUNXI_HIFI1 = 1,
};

struct rpmsg_hifi_priv {
	struct rpmsg_device *rpmsg_dev;
	struct msg_audio_package msg_pack;
	struct workqueue_struct *wq;
	char wq_name[32];
	struct work_struct rpmsg_recv_work;
	int32_t rx_count;
};

int32_t sunxi_hifi_register_sound_drv_info(const char *name, void *data);
int32_t sunxi_hifi_unregister_sound_drv_info(const char *name, void *data);

struct msg_component_package *sunxi_hifi_list_msg_component_find_item(
	struct msg_component_package *msg_component);
int sunxi_hifi_list_msg_component_add_tail(struct msg_component_package *msg_component);
int sunxi_hifi_list_msg_component_remove_item(struct msg_component_package *msg_component);

int32_t sunxi_hifi_nonblock_send(uint32_t hifi_id,
				struct msg_audio_package *msg_package);

int32_t sunxi_hifi_component_block_send(uint32_t hifi_id,
				struct msg_component_package *msg_component);
int32_t sunxi_hifi_component_nonblock_send(uint32_t hifi_id,
				struct msg_component_package *msg_component);

int32_t sunxi_hifi_substream_set_stream_component(uint32_t hifi_id,
		struct snd_soc_dai *dai,
		struct snd_soc_dsp_substream *soc_substream,
		struct snd_dsp_component *dsp_component);
int32_t snd_soc_rpaf_pcm_update_stream_process(struct snd_dsp_component *stream_dsp_component);
int32_t sunxi_hifi_substream_release_stream_component(struct snd_soc_dai *dai,
		struct snd_dsp_component *dsp_component);

int32_t sunxi_hifi_cpudai_substream_block_send(uint32_t hifi_id,
		struct snd_pcm_substream *substream, struct snd_soc_dai *dai,
		int32_t stream, unsigned long msg_cmd);

int32_t sunxi_hifi_daudio_substream_block_send(uint32_t hifi_id,
		struct snd_pcm_substream *substream, struct snd_soc_dai *dai,
		int32_t stream, unsigned long msg_cmd);

int32_t sunxi_hifi_dmic_substream_block_send(uint32_t hifi_id,
		struct snd_pcm_substream *substream, struct snd_soc_dai *dai,
		int32_t stream, unsigned long msg_cmd);
#endif
