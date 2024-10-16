/*
 * sound\soc\sunxi\hifi-dsp\sun50iw11-cpudai.c
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

#include <linux/module.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/dma/sunxi-dma.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>

#include <asm/cacheflush.h>
#include <sound/aw_rpaf/rpmsg_hifi.h>
#include <sound/aw_rpaf/component-core.h>

#include "sunxi-hifi-pcm.h"
#include "sunxi-cpudai.h"
#include "sun50iw11-codec.h"

#define DRV_NAME "sunxi-internal-cpudai"

static long playback_component_type;
module_param(playback_component_type, long, 0644);
MODULE_PARM_DESC(playback_component_type, "playback component type for dump");

static long capture_component_type;
module_param(capture_component_type, long, 0644);
MODULE_PARM_DESC(capture_component_type, "capture component type for dump");

static int codec_vad_mode_get(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_codec_get_drvdata(codec);
	struct msg_substream_package *msg_substream = &sunxi_cpudai->msg_capture;
	struct snd_soc_dsp_substream *soc_substream = &msg_substream->soc_substream;

	ucontrol->value.integer.value[0] = soc_substream->audio_standby;
	return 0;
}

static int codec_vad_mode_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_codec_get_drvdata(codec);
	struct msg_substream_package *msg_substream = &sunxi_cpudai->msg_capture;
	struct snd_soc_dsp_substream *soc_substream = &msg_substream->soc_substream;

	soc_substream->audio_standby = ucontrol->value.integer.value[0];

	return 0;
}

static const struct snd_kcontrol_new sunxi_cpudai_controls[] = {
	SOC_SINGLE_EXT("codec vad support", 0, 0, 1, 0, codec_vad_mode_get, codec_vad_mode_put),
};

static int sunxi_hifi_cpudai_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct mutex *rpmsg_mutex;
	int ret = 0;

	if (IS_ERR_OR_NULL(sunxi_cpudai)) {
		dev_err(dai->dev, "[%s] sunxi_cpudai is NULL!\n", __func__);
		return -ENOMEM;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_dai_set_dma_data(dai, substream,
					&sunxi_cpudai->playback_dma_param);
		msg_substream = &sunxi_cpudai->msg_playback;
		rpmsg_mutex = &sunxi_cpudai->rpmsg_mutex_playback;
		sunxi_cpudai->playing = 0;
		soc_substream = &msg_substream->soc_substream;
		soc_substream->output_addr = sunxi_cpudai->playback_dma_param.phy_addr;
		dev_dbg(dai->dev, "coherent phy_addr:0x%x\n", soc_substream->output_addr);
	} else {
		snd_soc_dai_set_dma_data(dai, substream, &sunxi_cpudai->capture_dma_param);
		msg_substream = &sunxi_cpudai->msg_capture;
		rpmsg_mutex = &sunxi_cpudai->rpmsg_mutex_capture;
		sunxi_cpudai->capturing = 0;
		soc_substream = &msg_substream->soc_substream;
		soc_substream->input_addr = sunxi_cpudai->capture_dma_param.phy_addr;
		dev_dbg(dai->dev, "coherent phy_addr:0x%x\n", soc_substream->input_addr);
	}
	msg_substream->substream = substream;

	/* 发送给msgbox to dsp */
	mutex_lock(rpmsg_mutex);
	awrpaf_debug("\n");
	soc_substream->cmd_val = SND_SOC_DSP_PCM_STARTUP;
	ret = sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "cpudai send failed.\n");
		soc_substream->cmd_val = SND_SOC_DSP_PCM_SHUTDOWN;
		__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
		sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
		awrpaf_debug("\n");
		return ret;
	}
	return soc_substream->ret_val;
}

static int sunxi_hifi_cpudai_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params;
	unsigned int frame_bytes;
	unsigned int sample_bits;
	struct mutex *rpmsg_mutex;
	int ret = 0;

	if (IS_ERR_OR_NULL(sunxi_cpudai)) {
		dev_err(dai->dev, "[%s] sunxi_cpudai is null.\n", __func__);
		return -ENOMEM;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		msg_substream = &sunxi_cpudai->msg_playback;
		rpmsg_mutex = &sunxi_cpudai->rpmsg_mutex_playback;
	} else {
		msg_substream = &sunxi_cpudai->msg_capture;
		rpmsg_mutex = &sunxi_cpudai->rpmsg_mutex_capture;
	}
	soc_substream = &msg_substream->soc_substream;

	mutex_lock(rpmsg_mutex);
	pcm_params = &soc_substream->params;
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		pcm_params->format = SND_PCM_FORMAT_S16_LE;
		sample_bits = 16;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		pcm_params->format = SND_PCM_FORMAT_S24_LE;
		sample_bits = 32;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		pcm_params->format = SND_PCM_FORMAT_S32_LE;
		sample_bits = 32;
		break;
	default:
		mutex_unlock(rpmsg_mutex);
		dev_err(sunxi_cpudai->dev, "unrecognized format\n");
		return -EINVAL;
	}

	pcm_params->channels = params_channels(params);
	pcm_params->rate = params_rate(params);
	pcm_params->period_size = params_period_size(params);
	pcm_params->periods = params_periods(params);
	pcm_params->pcm_frames = pcm_params->period_size;
	/* 在流中buffer务必一致大小, 代码中务必检查！ */
	pcm_params->buffer_size = params_buffer_size(params);
	frame_bytes = pcm_params->channels * sample_bits / 8;

	dev_info(sunxi_cpudai->dev, "======== hw_params ========\n");
	dev_info(sunxi_cpudai->dev, "pcm_params->format:%u\n", pcm_params->format);
	dev_info(sunxi_cpudai->dev, "pcm_params->channels:%u\n", pcm_params->channels);
	dev_info(sunxi_cpudai->dev, "pcm_params->rate:%u\n", pcm_params->rate);
	dev_info(sunxi_cpudai->dev, "pcm_params->period_size:%u\n", pcm_params->period_size);
	dev_info(sunxi_cpudai->dev, "pcm_params->periods:%u\n", pcm_params->periods);
	dev_info(sunxi_cpudai->dev, "pcm_params->pcm_frames:%u\n", pcm_params->pcm_frames);
	dev_info(sunxi_cpudai->dev, "pcm_params->buffer_size:%u\n", pcm_params->buffer_size);
	dev_info(sunxi_cpudai->dev, "===========================\n");
	soc_substream->input_size = pcm_params->period_size * frame_bytes;
	soc_substream->output_size = pcm_params->period_size * frame_bytes;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		struct snd_dsp_component *dsp_component = &sunxi_cpudai->dsp_playcomp;
		struct msg_component_package *msg_component = &dsp_component->msg_component;
		struct snd_soc_dsp_component *soc_component = &msg_component->soc_component;

		soc_component->component_type = playback_component_type;

		ret = sunxi_hifi_substream_set_stream_component(SUNXI_HIFI0,
				dai, soc_substream, dsp_component);
		if (ret < 0) {
			dev_err(dai->dev, "set stream component send failed.\n");
			return ret;
		}
	} else {
		struct snd_dsp_component *dsp_component = &sunxi_cpudai->dsp_capcomp;
		struct msg_component_package *msg_component = &dsp_component->msg_component;
		struct snd_soc_dsp_component *soc_component = &msg_component->soc_component;

		soc_component->component_type = capture_component_type;

		ret = sunxi_hifi_substream_set_stream_component(SUNXI_HIFI0,
				dai, soc_substream, dsp_component);
		if (ret < 0) {
			dev_err(dai->dev, "set stream component send failed.\n");
			return ret;
		}
	}

	/* 发送给msgbox to dsp */
	soc_substream->cmd_val = SND_SOC_DSP_PCM_HW_PARAMS;
	awrpaf_debug("\n");
	ret = sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0, substream, dai,
			substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "cpudai send failed.\n");
		return ret;
	}
	if (soc_substream->ret_val < 0)
		dev_err(dai->dev, "soc_substream hw_params ret_val:%d\n",
				soc_substream->ret_val);
	return soc_substream->ret_val;
}

static int sunxi_hifi_cpudai_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct mutex *rpmsg_mutex;
	int ret = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		msg_substream = &sunxi_cpudai->msg_playback;
		rpmsg_mutex = &sunxi_cpudai->rpmsg_mutex_playback;
		/* waitting stop play */
		while (sunxi_cpudai->playing)
			msleep(20);
	} else {
		msg_substream = &sunxi_cpudai->msg_capture;
		rpmsg_mutex = &sunxi_cpudai->rpmsg_mutex_capture;
		/* waitting stop capture */
		while (sunxi_cpudai->capturing)
			msleep(20);
	}
	soc_substream = &msg_substream->soc_substream;

	/* 发送给msgbox to dsp */
	mutex_lock(rpmsg_mutex);
	awrpaf_debug("\n");
	soc_substream->cmd_val = SND_SOC_DSP_PCM_PREPARE;
	ret = sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "cpudai send failed.\n");
		return ret;
	}
	return soc_substream->ret_val;
}

static int sunxi_hifi_cpudai_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct delayed_work *trigger_work;
	struct workqueue_struct *wq_pcm;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			msg_substream = &sunxi_cpudai->msg_playback;
			soc_substream = &msg_substream->soc_substream;
			soc_substream->cmd_val = SND_SOC_DSP_PCM_WRITEI;
			trigger_work = &sunxi_cpudai->trigger_work_playback;
			wq_pcm = sunxi_cpudai->wq_playback;
		} else {
			msg_substream = &sunxi_cpudai->msg_capture;
			soc_substream = &msg_substream->soc_substream;
			soc_substream->cmd_val = SND_SOC_DSP_PCM_READI;
			trigger_work = &sunxi_cpudai->trigger_work_capture;
			wq_pcm = sunxi_cpudai->wq_capture;
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if ((substream->stream == SNDRV_PCM_STREAM_PLAYBACK) &&
			(substream->runtime->status->state == SNDRV_PCM_STATE_DRAINING)) {
			dev_info(dai->dev, "drain stopping\n");
			return 0;
		}
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			msg_substream = &sunxi_cpudai->msg_playback;
			trigger_work = &sunxi_cpudai->trigger_work_playback;
			wq_pcm = sunxi_cpudai->wq_playback;
		} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			msg_substream = &sunxi_cpudai->msg_capture;
			trigger_work = &sunxi_cpudai->trigger_work_capture;
			wq_pcm = sunxi_cpudai->wq_capture;
		}
		soc_substream = &msg_substream->soc_substream;
		soc_substream->cmd_val = SND_SOC_DSP_PCM_STOP;
		break;
	case SNDRV_PCM_TRIGGER_DRAIN:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			msg_substream = &sunxi_cpudai->msg_playback;
			trigger_work = &sunxi_cpudai->trigger_work_playback;
			wq_pcm = sunxi_cpudai->wq_playback;
			soc_substream = &msg_substream->soc_substream;
			soc_substream->cmd_val = SND_SOC_DSP_PCM_DRAIN;
			dev_info(dai->dev, "drain starting\n");
		}
		break;
	default:
		return -EINVAL;
	}
	queue_delayed_work(wq_pcm, trigger_work, 0);

	return 0;
}

static void sunxi_hifi_cpudai_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_dsp_component *dsp_component;
	struct mutex *rpmsg_mutex;
	unsigned int *work_running;
	int i = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		msg_substream = &sunxi_cpudai->msg_playback;
		rpmsg_mutex = &sunxi_cpudai->rpmsg_mutex_playback;
		work_running = &sunxi_cpudai->playing;
	} else {
		msg_substream = &sunxi_cpudai->msg_capture;
		rpmsg_mutex = &sunxi_cpudai->rpmsg_mutex_capture;
		work_running = &sunxi_cpudai->capturing;
	}
	soc_substream = &msg_substream->soc_substream;

	awrpaf_debug("\n");
	for (i = 0; i < 50; i++) {
		if (*work_running == 0)
			break;
		msleep(20);
	}
	awrpaf_debug("\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dsp_component = &sunxi_cpudai->dsp_playcomp;
	else
		dsp_component = &sunxi_cpudai->dsp_capcomp;

	sunxi_hifi_substream_release_stream_component(dai, dsp_component);

	/* 发送给msgbox to dsp */
	mutex_lock(rpmsg_mutex);
	soc_substream->cmd_val = SND_SOC_DSP_PCM_SHUTDOWN;
	awrpaf_debug("\n");
	sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0, substream, dai,
						substream->stream, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);

	msg_substream->substream = NULL;
}

static struct snd_soc_dai_ops sunxi_hifi_cpudai_dai_ops = {
	.startup = sunxi_hifi_cpudai_startup,
	.hw_params = sunxi_hifi_cpudai_hw_params,
	.prepare = sunxi_hifi_cpudai_prepare,
	.trigger = sunxi_hifi_cpudai_trigger,
	.shutdown = sunxi_hifi_cpudai_shutdown,
};

static int sunxi_hifi_cpudai_init(struct sunxi_cpudai_info *sunxi_cpudai,
					struct snd_soc_dai *dai)
{
	struct msg_substream_package *msg_playback = &sunxi_cpudai->msg_playback;
	struct msg_substream_package *msg_capture = &sunxi_cpudai->msg_capture;
	struct msg_debug_package *msg_debug = &sunxi_cpudai->msg_debug;
//	struct snd_soc_dsp_debug *soc_debug = &msg_debug->soc_debug;
	struct msg_component_package *msg_component;
	struct snd_dsp_component *dsp_component;
//	struct snd_soc_dsp_component *soc_component;
	struct snd_soc_dsp_substream *soc_substream;
	struct snd_soc_dsp_pcm_params *pcm_params;
	int ret = 0;

	sunxi_cpudai->cpu_dai = dai;

	/* init for playback */
	init_waitqueue_head(&msg_playback->tsleep);
	spin_lock_init(&msg_playback->lock);
	msg_playback->wakeup_flag = 0;
	soc_substream = &msg_playback->soc_substream;
	pcm_params = &soc_substream->params;
	/* 1:capture; 0:playback */
	pcm_params->stream = SND_STREAM_PLAYBACK;
	pcm_params->card = sunxi_cpudai->dsp_card;
	pcm_params->device = sunxi_cpudai->dsp_device;
	/*
	 * 根据名字匹配(delete: hw:):
	 * 0: hw:audiocodec;
	 * 1: hw:snddmic;
	 * 2: hw:snddaudio0;
	 */
	snprintf(pcm_params->driver, 31, "audiocodec");
	/* dma buffer alloc */
	sunxi_dma_params_alloc_dma_area(dai, &sunxi_cpudai->playback_dma_param);
	if (!sunxi_cpudai->playback_dma_param.dma_area) {
		dev_err(dai->dev, "playback dmaengine alloc coherent failed.\n");
		ret = -ENOMEM;
		goto err_dma_alloc_playback;
	}

	/* init for playback component */
	dsp_component = &sunxi_cpudai->dsp_playcomp;
	msg_component = &dsp_component->msg_component;
	init_waitqueue_head(&msg_component->tsleep);
	spin_lock_init(&msg_component->lock);
	msg_component->wakeup_flag = 0;

	/* init for capture */
	init_waitqueue_head(&msg_capture->tsleep);
	spin_lock_init(&msg_capture->lock);
	msg_capture->wakeup_flag = 0;
	soc_substream = &msg_capture->soc_substream;
	pcm_params = &soc_substream->params;
	pcm_params->stream = SND_STREAM_CAPTURE;
	pcm_params->card = sunxi_cpudai->dsp_card;
	pcm_params->device = sunxi_cpudai->dsp_device;
	snprintf(pcm_params->driver, 31, "audiocodec");
	/* dma buffer alloc */
	sunxi_dma_params_alloc_dma_area(dai, &sunxi_cpudai->capture_dma_param);
	if (!sunxi_cpudai->capture_dma_param.dma_area) {
		dev_err(dai->dev, "capture dmaengine alloc coherent failed.\n");
		ret = -ENOMEM;
		goto err_dma_alloc_capture;
	}

	/* init for capture component */
	dsp_component = &sunxi_cpudai->dsp_capcomp;
	msg_component = &dsp_component->msg_component;
	init_waitqueue_head(&msg_component->tsleep);
	spin_lock_init(&msg_component->lock);
	msg_component->wakeup_flag = 0;

	/* init for debug */
	init_waitqueue_head(&msg_debug->tsleep);
	spin_lock_init(&msg_debug->lock);
	msg_debug->wakeup_flag = 0;

	/* register sunxi_cpudai_info to rpmsg_hifi driver */
	sunxi_hifi_register_sound_drv_info(pcm_params->driver, sunxi_cpudai);

	/* pdata_info */
	return 0;

err_dma_alloc_capture:
	sunxi_dma_params_free_dma_area(dai, &sunxi_cpudai->playback_dma_param);
err_dma_alloc_playback:
	return ret;
}

static int sunxi_hifi_cpudai_uninit(struct sunxi_cpudai_info *sunxi_cpudai,
					struct snd_soc_dai *dai)
{
	struct msg_substream_package *msg_substream;
	struct snd_soc_dsp_substream *soc_substream;

	sunxi_dma_params_free_dma_area(dai, &sunxi_cpudai->playback_dma_param);
	sunxi_dma_params_free_dma_area(dai, &sunxi_cpudai->capture_dma_param);

	msg_substream = &sunxi_cpudai->msg_playback;
	soc_substream = &msg_substream->soc_substream;
	/* for unregister sunxi_cpudai_info from rpmsg_hifi driver */
	sunxi_hifi_unregister_sound_drv_info(soc_substream->params.driver, sunxi_cpudai);

	return 0;
}

int sunxi_hifi_cpudai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_capture = &sunxi_cpudai->msg_capture;
	struct msg_substream_package *msg_playback = &sunxi_cpudai->msg_playback;
	struct snd_soc_dsp_substream *soc_substream;
	int ret = 0;

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai, &sunxi_cpudai->playback_dma_param,
				&sunxi_cpudai->capture_dma_param);

	sunxi_hifi_cpudai_init(sunxi_cpudai, dai);

	dev_info(dai->dev, "%s start.\n", __func__);
	/* 发送给msgbox to dsp */
	/* for hifi pcm playback create hal object */
	soc_substream = &msg_playback->soc_substream;
	soc_substream->cmd_val = SND_SOC_DSP_PCM_PROBE;
	ret = sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_PLAYBACK,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	if (ret < 0) {
		dev_err(dai->dev, "%s probe playback failed.\n", __func__);
		goto err_probe_playback;
	} else if (soc_substream->ret_val < 0) {
		dev_err(dai->dev, "%s probe playback dsp failed.\n", __func__);
		ret = soc_substream->ret_val;
		goto err_probe_playback;
	}

	/* for capture */
	soc_substream = &msg_capture->soc_substream;
	soc_substream->cmd_val = SND_SOC_DSP_PCM_PROBE;
	ret = sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_CAPTURE,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	if (ret < 0) {
		dev_err(dai->dev, "%s probe capture failed.\n", __func__);
		goto err_probe_capture;
	} else if (soc_substream->ret_val < 0) {
		dev_err(dai->dev, "%s probe capture dsp failed.\n", __func__);
		ret = soc_substream->ret_val;
		goto err_probe_capture;
	}

	dev_info(dai->dev, "%s stop.\n", __func__);
	return 0;

err_probe_capture:
err_probe_playback:
	sunxi_hifi_cpudai_uninit(sunxi_cpudai, dai);
	return ret;
}

static int sunxi_hifi_cpudai_suspend(struct snd_soc_dai *dai)
{
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_capture = &sunxi_cpudai->msg_capture;
	struct msg_substream_package *msg_playback = &sunxi_cpudai->msg_playback;
	struct snd_soc_dsp_substream *soc_substream;
	struct mutex *rpmsg_mutex;
	int ret;

	pr_debug("[%s] suspend .%s start\n", __func__,
			dev_name(sunxi_cpudai->dev));

	/* for capture */
	rpmsg_mutex = &sunxi_cpudai->rpmsg_mutex_capture;
	mutex_lock(rpmsg_mutex);
	soc_substream = &msg_capture->soc_substream;
	soc_substream->cmd_val = SND_SOC_DSP_PCM_SUSPEND;
	ret = sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_CAPTURE,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "%s suspend capture failed.\n", __func__);
		return ret;
	} else if (soc_substream->ret_val < 0) {
		dev_err(dai->dev, "%s suspend capture dsp failed.\n", __func__);
		return soc_substream->ret_val;
	}

	/* for playback */
	rpmsg_mutex = &sunxi_cpudai->rpmsg_mutex_playback;
	mutex_lock(rpmsg_mutex);
	soc_substream = &msg_playback->soc_substream;
	soc_substream->cmd_val = SND_SOC_DSP_PCM_SUSPEND;
	ret = sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_PLAYBACK,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "%s suspend playback failed.\n", __func__);
		return ret;
	} else if (soc_substream->ret_val < 0) {
		dev_err(dai->dev, "%s suspend playback dsp failed.\n", __func__);
		return soc_substream->ret_val;
	}

	pr_debug("[%s] suspend .%s stop\n", __func__,
			dev_name(sunxi_cpudai->dev));
	return 0;

}

static int sunxi_hifi_cpudai_resume(struct snd_soc_dai *dai)
{
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_capture = &sunxi_cpudai->msg_capture;
	struct msg_substream_package *msg_playback = &sunxi_cpudai->msg_playback;
	struct snd_soc_dsp_substream *soc_substream;
	struct mutex *rpmsg_mutex;
	int ret;

	pr_debug("[%s] resume .%s start\n", __func__,
			dev_name(sunxi_cpudai->dev));

	/* for capture */
	rpmsg_mutex = &sunxi_cpudai->rpmsg_mutex_capture;
	mutex_lock(rpmsg_mutex);
	soc_substream = &msg_capture->soc_substream;
	soc_substream->cmd_val = SND_SOC_DSP_PCM_RESUME;
	ret = sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_CAPTURE,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "%s suspend capture failed.\n", __func__);
		return ret;
	} else if (soc_substream->ret_val < 0) {
		dev_err(dai->dev, "%s suspend capture dsp failed.\n", __func__);
		return soc_substream->ret_val;
	}

	/* for playback */
	rpmsg_mutex = &sunxi_cpudai->rpmsg_mutex_playback;
	mutex_lock(rpmsg_mutex);
	soc_substream = &msg_playback->soc_substream;
	soc_substream->cmd_val = SND_SOC_DSP_PCM_RESUME;
	ret = sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_PLAYBACK,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	mutex_unlock(rpmsg_mutex);
	if (ret < 0) {
		dev_err(dai->dev, "%s suspend playback failed.\n", __func__);
		return ret;
	} else if (soc_substream->ret_val < 0) {
		dev_err(dai->dev, "%s suspend playback dsp failed.\n", __func__);
		return soc_substream->ret_val;
	}

	pr_debug("[%s] resume .%s stop\n", __func__,
			dev_name(sunxi_cpudai->dev));

	return 0;
}

int sunxi_hifi_cpudai_remove(struct snd_soc_dai *dai)
{
	struct sunxi_cpudai_info *sunxi_cpudai = snd_soc_dai_get_drvdata(dai);
	struct msg_substream_package *msg_capture = &sunxi_cpudai->msg_capture;
	struct msg_substream_package *msg_playback = &sunxi_cpudai->msg_playback;
	struct snd_soc_dsp_substream *soc_substream;
	int ret = 0;

	dev_info(dai->dev, "%s start.\n", __func__);
	/* for playback */
	soc_substream = &msg_playback->soc_substream;
	soc_substream->cmd_val = SND_SOC_DSP_PCM_REMOVE;
	sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_PLAYBACK,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);

	/* for capture */
	soc_substream = &msg_capture->soc_substream;
	soc_substream->cmd_val = SND_SOC_DSP_PCM_REMOVE;
	ret = sunxi_hifi_daudio_substream_block_send(SUNXI_HIFI0, NULL, dai,
							SNDRV_PCM_STREAM_CAPTURE,
							MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
	__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
	if (ret < 0) {
		dev_err(dai->dev, "remove send failed.\n");
		sunxi_hifi_cpudai_uninit(sunxi_cpudai, dai);
		return -EFAULT;
	}
	sunxi_hifi_cpudai_uninit(sunxi_cpudai, dai);

	dev_info(dai->dev, "%s stop.\n", __func__);

	return 0;
}

static struct snd_soc_dai_driver sunxi_hifi_cpudai_dai = {
	.probe = sunxi_hifi_cpudai_probe,
	.remove = sunxi_hifi_cpudai_remove,
	.suspend = sunxi_hifi_cpudai_suspend,
	.resume = sunxi_hifi_cpudai_resume,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000 |
			SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
			SNDRV_PCM_FMTBIT_S24_LE |
			SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 6,
		.rates = SNDRV_PCM_RATE_8000_48000 |
			SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
			SNDRV_PCM_FMTBIT_S24_LE |
			SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_hifi_cpudai_dai_ops,
};

static const struct snd_soc_component_driver sunxi_hifi_cpudai_component = {
	.name = DRV_NAME,
	.controls	= sunxi_cpudai_controls,
	.num_controls	= ARRAY_SIZE(sunxi_cpudai_controls),
};

static const struct of_device_id sunxi_hifi_cpudai_of_match[] = {
	{ .compatible = "allwinner,sunxi-internal-cpudai", },
	{},
};

static void trigger_work_playback_func(struct work_struct *work)
{
	struct sunxi_cpudai_info *sunxi_cpudai =
			container_of(work, struct sunxi_cpudai_info, trigger_work_playback.work);
	struct msg_substream_package *msg_substream = &sunxi_cpudai->msg_playback;
	struct snd_pcm_substream *substream = msg_substream->substream;
	struct snd_soc_dsp_substream *soc_substream = &msg_substream->soc_substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned int pos = 0;
	int ret = 0;

	for (;;) {
		pr_debug("[%s] line:%d cmd_val=%d, state=%d\n", __func__, __LINE__,
			soc_substream->cmd_val,
			runtime->status->state);
		mutex_lock(&sunxi_cpudai->rpmsg_mutex_playback);
		if (soc_substream->cmd_val == SND_SOC_DSP_PCM_WRITEI) {
			snd_pcm_stream_lock_irq(substream);
			if (runtime->status->state != SNDRV_PCM_STATE_RUNNING) {
				dev_err(sunxi_cpudai->cpu_dai->dev,
					"%s state:%d\n", __func__, runtime->status->state);
				snd_pcm_stream_unlock_irq(substream);
				mutex_unlock(&sunxi_cpudai->rpmsg_mutex_playback);
				break;
			}
			sunxi_cpudai->playing = 1;
			/* update the capture soc_substream output addr */
			pos = snd_dmaengine_pcm_pointer_no_residue(substream);
			soc_substream->input_addr = runtime->dma_addr + frames_to_bytes(runtime, pos);
			snd_pcm_stream_unlock_irq(substream);

			dev_dbg(sunxi_cpudai->cpu_dai->dev,
				"[%s] dma_addr:0x%llx, pos:%u, input_addr:0x%x\n",
				__func__, runtime->dma_addr, pos, soc_substream->input_addr);
			ret = sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0,
				substream, sunxi_cpudai->cpu_dai,
				SNDRV_PCM_STREAM_PLAYBACK, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
			__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
			mutex_unlock(&sunxi_cpudai->rpmsg_mutex_playback);

			/* 拷贝并发送通知component */
			snd_soc_rpaf_pcm_update_stream_process(&sunxi_cpudai->dsp_playcomp);

			if (ret != 0) {
				dev_err(sunxi_cpudai->cpu_dai->dev,
					"%s state:%d, ret=%d\n", __func__, runtime->status->state, ret);
			}
			if (runtime->status->state == SNDRV_PCM_STATE_RUNNING) {
				queue_delayed_work(sunxi_cpudai->wq_playback,
					&sunxi_cpudai->trigger_work_playback, 0);

			}
			break;
		} else if (soc_substream->cmd_val == SND_SOC_DSP_PCM_STOP) {
			awrpaf_debug("\n");
			/* send to dsp */
			sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0,
				substream, sunxi_cpudai->cpu_dai,
				SNDRV_PCM_STREAM_PLAYBACK, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
			__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
			sunxi_cpudai->playing = 0;
			awrpaf_debug("\n");
			mutex_unlock(&sunxi_cpudai->rpmsg_mutex_playback);
			break;
		} else if (soc_substream->cmd_val == SND_SOC_DSP_PCM_DRAIN) {
			awrpaf_debug("\n");
			/* send to dsp */
			sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0,
				substream, sunxi_cpudai->cpu_dai,
				SNDRV_PCM_STREAM_PLAYBACK, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
			__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));

			snd_pcm_stream_lock_irq(substream);
			snd_pcm_drain_done(substream);
			snd_pcm_update_state(substream, runtime);
			sunxi_cpudai->playing = 0;
			snd_pcm_stream_unlock_irq(substream);

			awrpaf_debug("\n");
			mutex_unlock(&sunxi_cpudai->rpmsg_mutex_playback);
			break;
		} else if (soc_substream->cmd_val == SND_SOC_DSP_PCM_SUSPEND) {
			sunxi_cpudai->playing = 0;
			mutex_unlock(&sunxi_cpudai->rpmsg_mutex_playback);
			break;
		}
		mutex_unlock(&sunxi_cpudai->rpmsg_mutex_playback);
	}
}

static void trigger_work_capture_func(struct work_struct *work)
{
	struct sunxi_cpudai_info *sunxi_cpudai =
			container_of(work, struct sunxi_cpudai_info, trigger_work_capture.work);
	struct msg_substream_package *msg_substream = &sunxi_cpudai->msg_capture;
	struct snd_soc_dsp_substream *soc_substream = &msg_substream->soc_substream;
	struct snd_pcm_substream *substream = msg_substream->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned int pos = 0;
	int ret = 0;

	for (;;) {
		pr_debug("[%s] line:%d cmd_val=%d, state=%d\n", __func__, __LINE__,
			soc_substream->cmd_val,
			runtime->status->state);
		mutex_lock(&sunxi_cpudai->rpmsg_mutex_capture);
		if (soc_substream->cmd_val == SND_SOC_DSP_PCM_READI) {
			snd_pcm_sframes_t avail;
			unsigned long delay;

			if (runtime->status->state != SNDRV_PCM_STATE_RUNNING) {
				dev_err(sunxi_cpudai->cpu_dai->dev,
					"%s state:%d\n", __func__, runtime->status->state);
				mutex_unlock(&sunxi_cpudai->rpmsg_mutex_capture);
				break;
			}
			sunxi_cpudai->capturing = 1;

			if (!soc_substream->audio_standby)
				goto do_readi;
			avail = snd_pcm_capture_hw_avail(runtime);
			if (avail > runtime->period_size)
				goto do_readi;
			delay = runtime->period_size * 1000 / runtime->rate;
			if (delay > 10)
				delay = 10;
			pr_debug("[%s] line:%d avail = %ld, delay=%lu\n",
				__func__, __LINE__, avail, delay);
			queue_delayed_work(sunxi_cpudai->wq_capture,
				&sunxi_cpudai->trigger_work_capture, delay);
			mutex_unlock(&sunxi_cpudai->rpmsg_mutex_capture);
			break;
do_readi:
			/* update the capture soc_substream output addr */
			pos = snd_dmaengine_pcm_pointer_no_residue(substream);
			soc_substream->output_addr = runtime->dma_addr + frames_to_bytes(runtime, pos);
			dev_dbg(sunxi_cpudai->cpu_dai->dev,
				"[%s] dma_addr:0x%llx, pos:%u, output_addr:0x%x\n",
				 __func__, runtime->dma_addr, pos, soc_substream->output_addr);
			/* send to dsp */
			ret = sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0,
				substream, sunxi_cpudai->cpu_dai,
				SNDRV_PCM_STREAM_CAPTURE, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
			__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
			mutex_unlock(&sunxi_cpudai->rpmsg_mutex_capture);

			/* 拷贝并发送通知component */
			snd_soc_rpaf_pcm_update_stream_process(&sunxi_cpudai->dsp_capcomp);

			if (ret != 0) {
				dev_err(sunxi_cpudai->cpu_dai->dev,
					"%s state:%d, ret=%d\n", __func__, runtime->status->state, ret);
			}
			if (runtime->status->state == SNDRV_PCM_STATE_RUNNING) {
				queue_delayed_work(sunxi_cpudai->wq_capture,
					&sunxi_cpudai->trigger_work_capture, 0);
			}
			break;
		} else if (soc_substream->cmd_val == SND_SOC_DSP_PCM_STOP) {
			awrpaf_debug("\n");
			/* send to dsp */
			sunxi_hifi_cpudai_substream_block_send(SUNXI_HIFI0,
				substream, sunxi_cpudai->cpu_dai,
				SNDRV_PCM_STREAM_CAPTURE, MSGBOX_SOC_DSP_AUDIO_PCM_COMMAND);
			__dma_flush_range(soc_substream, sizeof(struct snd_soc_dsp_substream));
			sunxi_cpudai->capturing = 0;
			mutex_unlock(&sunxi_cpudai->rpmsg_mutex_capture);
			break;
		} else if (soc_substream->cmd_val == SND_SOC_DSP_PCM_SUSPEND) {
			sunxi_cpudai->capturing = 0;
			mutex_unlock(&sunxi_cpudai->rpmsg_mutex_capture);
			break;
		}
		mutex_unlock(&sunxi_cpudai->rpmsg_mutex_capture);
	}
}

static int sunxi_hifi_cpudai_dev_probe(struct platform_device *pdev)
{
	struct sunxi_cpudai_info *sunxi_cpudai;
	struct device_node *np = pdev->dev.of_node;
	unsigned int temp_val = 0;
	int ret;

	sunxi_cpudai = devm_kzalloc(&pdev->dev,
			sizeof(struct sunxi_cpudai_info), GFP_KERNEL);
	if (!sunxi_cpudai) {
		ret = -ENOMEM;
		goto err_node_put;
	}
	dev_set_drvdata(&pdev->dev, sunxi_cpudai);
	sunxi_cpudai->dev = &pdev->dev;

	ret = snd_soc_register_component(&pdev->dev, &sunxi_hifi_cpudai_component,
					&sunxi_hifi_cpudai_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	ret = asoc_hifi_platform_register(&pdev->dev, SUNXI_HIFI_PCM_OPS_CODEC);
	if (ret) {
		dev_err(&pdev->dev, "Could not register PCM: %d\n", ret);
		goto err_unregister_component;
	}

	ret = of_property_read_u32(np, "playback_cma", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "playback cma kbytes config missing or invalid.\n");
		sunxi_cpudai->playback_dma_param.cma_kbytes = SUNXI_AUDIO_CMA_MAX_KBYTES;
	} else {
		if (temp_val > SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val < SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MIN_KBYTES;
		sunxi_cpudai->playback_dma_param.cma_kbytes = temp_val;
	}

	ret = of_property_read_u32(np, "capture_cma", &temp_val);
	if (ret != 0) {
		dev_warn(&pdev->dev, "capture cma kbytes config missing or invalid.\n");
		sunxi_cpudai->capture_dma_param.cma_kbytes = SUNXI_AUDIO_CMA_MAX_KBYTES;
	} else {
		if (temp_val > SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val < SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MIN_KBYTES;
		sunxi_cpudai->capture_dma_param.cma_kbytes = temp_val;
	}

	sunxi_cpudai->playback_dma_param.fifo_size = CODEC_TX_FIFO_SIZE;
	sunxi_cpudai->capture_dma_param.fifo_size = CODEC_RX_FIFO_SIZE;

	/* for hifi dsp param init config */
	/* eg:0 sndcodec; 1 snddmic; 2 snddaudio0; */
	ret = of_property_read_u32(np, "dsp_card", &temp_val);
	if (ret < 0)
		sunxi_cpudai->dsp_card = 0;
	else
		sunxi_cpudai->dsp_card = temp_val;

	/* default is 0, for reserved */
	ret = of_property_read_u32(np, "dsp_device", &temp_val);
	if (ret < 0)
		sunxi_cpudai->dsp_device = 0;
	else
		sunxi_cpudai->dsp_device = temp_val;

	mutex_init(&sunxi_cpudai->rpmsg_mutex_playback);
	mutex_init(&sunxi_cpudai->rpmsg_mutex_capture);

	snprintf(sunxi_cpudai->wq_capture_name, sizeof(sunxi_cpudai->wq_capture_name),
			"hifi-codec-capture");
	sunxi_cpudai->wq_capture = create_workqueue(sunxi_cpudai->wq_capture_name);
	if (IS_ERR_OR_NULL(sunxi_cpudai->wq_capture)) {
		ret = -ENOMEM;
		goto err_create_wq_capture;
	}
	INIT_DELAYED_WORK(&sunxi_cpudai->trigger_work_capture, trigger_work_capture_func);
	snprintf(sunxi_cpudai->wq_playback_name, sizeof(sunxi_cpudai->wq_playback_name),
			"hifi-codec-playback");
	sunxi_cpudai->wq_playback = create_workqueue(sunxi_cpudai->wq_playback_name);
	if (IS_ERR_OR_NULL(sunxi_cpudai->wq_playback)) {
		ret = -ENOMEM;
		goto err_create_wq_playback;
	}
	INIT_DELAYED_WORK(&sunxi_cpudai->trigger_work_playback, trigger_work_playback_func);

	return 0;

err_create_wq_playback:
	mutex_destroy(&sunxi_cpudai->rpmsg_mutex_capture);
err_create_wq_capture:
err_unregister_component:
	snd_soc_unregister_component(&pdev->dev);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_cpudai);
err_node_put:
	of_node_put(np);
	return ret;
}

static int __exit sunxi_hifi_cpudai_dev_remove(struct platform_device *pdev)
{
	struct sunxi_cpudai_info *sunxi_cpudai = dev_get_drvdata(&pdev->dev);

	mutex_destroy(&sunxi_cpudai->rpmsg_mutex_playback);
	mutex_destroy(&sunxi_cpudai->rpmsg_mutex_capture);

	snd_soc_unregister_component(&pdev->dev);
	devm_kfree(&pdev->dev, sunxi_cpudai);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver sunxi_hifi_cpudai_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_hifi_cpudai_of_match,
	},
	.probe = sunxi_hifi_cpudai_dev_probe,
	.remove = __exit_p(sunxi_hifi_cpudai_dev_remove),
};
module_platform_driver(sunxi_hifi_cpudai_driver);

MODULE_AUTHOR("yumingfeng <yumingfeng@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI cpudai ASoC HiFi Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-internal-cpudai");
