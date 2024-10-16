/*
 * sound\soc\sunxi\hifi-dsp\sunxi-hifi-pcm.c
 *
 * (C) Copyright 2019-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
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
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma/sunxi-dma.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <asm/dma.h>
#include "sunxi-hifi-pcm.h"
#include "sunxi-cpudai.h"
#include "sunxi-dmic.h"
#include "sunxi-daudio.h"
#include "sunxi-snddmic.h"
#include "sunxi-snddaudio.h"

static u64 sunxi_pcm_mask = DMA_BIT_MASK(32);

static struct snd_pcm_hardware sunxi_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED
				| SNDRV_PCM_INFO_BLOCK_TRANSFER
				| SNDRV_PCM_INFO_MMAP
				| SNDRV_PCM_INFO_MMAP_VALID
				| SNDRV_PCM_INFO_PAUSE
				| SNDRV_PCM_INFO_RESUME
				| SNDRV_PCM_INFO_DRAIN_TRIGGER,
	.formats		= SNDRV_PCM_FMTBIT_S8
				| SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	.rates			= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
	.rate_min		= 8000,
	.rate_max		= 48000,
	.channels_min		= 1,
	.channels_max		= 8,
	/* value must be (2^n)Kbyte */
	.buffer_bytes_max	= SUNXI_AUDIO_CMA_MAX_BYTES_MAX,
	.period_bytes_min	= 256,
	/* value must be (2^(n-1))Kbyte */
	.period_bytes_max	= SUNXI_AUDIO_CMA_MAX_BYTES_MAX / 2,
	.periods_min		= 2,
	.periods_max		= 32,
	.fifo_size		= 128,
};

int sunxi_dma_params_alloc_dma_area(struct snd_soc_dai *dai,
						struct sunxi_dma_params *dma_params)
{
	size_t cma_bytes = SUNXI_AUDIO_CMA_BLOCK_BYTES;

	cma_bytes = dma_params->cma_kbytes * SUNXI_AUDIO_CMA_BLOCK_BYTES;
	if (cma_bytes > SUNXI_AUDIO_CMA_MAX_BYTES_MAX)
		cma_bytes = SUNXI_AUDIO_CMA_MAX_BYTES_MAX;
	if (cma_bytes < SUNXI_AUDIO_CMA_MAX_BYTES_MIN)
		cma_bytes = SUNXI_AUDIO_CMA_MAX_BYTES_MIN;
	dma_params->dma_area = dma_alloc_coherent(dai->dev, cma_bytes,
					&dma_params->phy_addr, GFP_KERNEL);
	if (!dma_params->dma_area) {
		dev_err(dai->dev, "dmaengine alloc coherent failed.\n");
		return -ENOMEM;
	}
	dev_info(dai->dev, "dmaengine alloc coherent phy_addr:0x%llx\n", dma_params->phy_addr);
	dma_params->phy_bytes = cma_bytes;

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_dma_params_alloc_dma_area);

int sunxi_dma_params_free_dma_area(struct snd_soc_dai *dai,
						struct sunxi_dma_params *dma_params)
{

	if (dma_params->dma_area)
		dma_free_coherent(dai->dev, dma_params->phy_bytes,
				dma_params->dma_area, dma_params->phy_addr);

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_dma_params_free_dma_area);

static int sunxi_hifi_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static int sunxi_hifi_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;

	snd_pcm_set_runtime_buffer(substream, NULL);
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static int sunxi_hifi_pcm_prepare_and_submit(struct snd_pcm_substream *substream)
{
	struct dmaengine_pcm_runtime_data *prtd = substream->runtime->private_data;

	prtd->pos = 0;

	return 0;
}

static int sunxi_hifi_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		sunxi_hifi_pcm_prepare_and_submit(substream);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		break;
	}
	return 0;
}

static int sunxi_hifi_pcm_open(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct dmaengine_pcm_runtime_data *prtd;
	struct device *dev = rtd->platform->dev;
	struct sunxi_dma_params *dma_params = NULL;

	dma_params = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);
	if (IS_ERR_OR_NULL(dma_params)) {
		dev_err(dev, "dma_params is null.\n");
		return -EFAULT;
	}
	/* Set HW params now that initialization is complete */
	sunxi_pcm_hardware.buffer_bytes_max = dma_params->cma_kbytes *
					SUNXI_AUDIO_CMA_BLOCK_BYTES;
	sunxi_pcm_hardware.period_bytes_max = sunxi_pcm_hardware.buffer_bytes_max / 2;
	sunxi_pcm_hardware.fifo_size = dma_params->fifo_size;

	snd_soc_set_runtime_hwparams(substream, &sunxi_pcm_hardware);
	ret = snd_pcm_hw_constraint_integer(substream->runtime,
							SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		dev_err(dev, "hw constraint integer failed with err %d\n", ret);
		return ret;
	}

	prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
	if (!prtd)
		return -ENOMEM;

	prtd->dma_chan = NULL;

	substream->runtime->private_data = prtd;

	return 0;
}

static int sunxi_hifi_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = NULL;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;

	if (substream->runtime != NULL) {
		runtime = substream->runtime;
		return dma_mmap_writecombine(substream->pcm->card->dev, vma,
					     runtime->dma_area,
					     runtime->dma_addr,
					     runtime->dma_bytes);
	} else {
		dev_err(dev, "substream->runtime is NULL!\n");
		return -EFAULT;
	}
}

static int sunxi_hifi_pcm_copy(struct snd_pcm_substream *substream, int a,
	 snd_pcm_uframes_t hwoff, void __user *buf, snd_pcm_uframes_t frames)
{
	char *hwbuf;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		hwbuf = runtime->dma_area + frames_to_bytes(runtime, hwoff);
		if (copy_from_user(hwbuf, buf, frames_to_bytes(runtime, frames))) {
			dev_err(dev, "copy_from_user failed.\n");
			return -EFAULT;
		}
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		hwbuf = runtime->dma_area + frames_to_bytes(runtime, hwoff);
		if (copy_to_user(buf, hwbuf, frames_to_bytes(runtime, frames))) {
			dev_err(dev, "copy_to_user failed.\n");
			return -EFAULT;
		}
	}

	return 0;
}

static int sunxi_hifi_pcm_daudio_copy(struct snd_pcm_substream *substream, int a,
	 snd_pcm_uframes_t hwoff, void __user *buf, snd_pcm_uframes_t frames)
{
	char *hwbuf;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_platform *platform = rtd->platform;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		hwbuf = runtime->dma_area + frames_to_bytes(runtime, hwoff);
		if (copy_from_user(hwbuf, buf, frames_to_bytes(runtime, frames))) {
			dev_err(platform->dev, "copy_from_user failed.\n");
			return -EFAULT;
		}

		dev_dbg_ratelimited(platform->dev, "copy_from_user hwoff:%lu, frames:%lu.\n", hwoff, frames);
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		hwbuf = runtime->dma_area + frames_to_bytes(runtime, hwoff);
		if (copy_to_user(buf, hwbuf, frames_to_bytes(runtime, frames))) {
			dev_err(platform->dev, "copy_to_user failed.\n");
			return -EFAULT;
		}
		dev_dbg_ratelimited(platform->dev, "copy_to_user hwoff:%lu, frames:%lu.\n", hwoff, frames);
	}

	return 0;
}

/* 实际位置由dsp的更新 */
static snd_pcm_uframes_t sunxi_hifi_pcm_pointer(struct snd_pcm_substream *substream)
{
	return snd_dmaengine_pcm_pointer_no_residue(substream);
}

static int sunxi_hifi_pcm_prepare(struct snd_pcm_substream *substream)
{
	return 0;
}

static int sunxi_hifi_pcm_close(struct snd_pcm_substream *substream)
{
	struct dmaengine_pcm_runtime_data *prtd = substream->runtime->private_data;

	kfree(prtd);

	return 0;
}

static struct snd_pcm_ops sunxi_hifi_pcm_ops = {
	.open		= sunxi_hifi_pcm_open,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= sunxi_hifi_pcm_hw_params,
	.prepare    = sunxi_hifi_pcm_prepare,
	.trigger	= sunxi_hifi_pcm_trigger,
	.pointer	= sunxi_hifi_pcm_pointer,
	.mmap		= sunxi_hifi_pcm_mmap,
	.copy		= sunxi_hifi_pcm_copy,
	.hw_free	= sunxi_hifi_pcm_hw_free,
	.close		= sunxi_hifi_pcm_close,
};

static struct snd_pcm_ops sunxi_hifi_pcm_ops_daudio = {
	.open		= sunxi_hifi_pcm_open,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= sunxi_hifi_pcm_hw_params,
	.prepare    = sunxi_hifi_pcm_prepare,
	.trigger	= sunxi_hifi_pcm_trigger,
	.pointer	= sunxi_hifi_pcm_pointer,
	.mmap		= sunxi_hifi_pcm_mmap,
	.copy		= sunxi_hifi_pcm_daudio_copy,
	.hw_free	= sunxi_hifi_pcm_hw_free,
	.close		= sunxi_hifi_pcm_close,
};

static int sunxi_hifi_pcm_preallocate_stream_dma_buffer(struct snd_pcm *pcm,
			int stream, size_t buffer_bytes_max)
{
	struct snd_pcm_str *streams = NULL;
	struct snd_pcm_substream *substream = NULL;
	struct snd_dma_buffer *dma_buf = NULL;

	streams = &pcm->streams[stream];
	if (IS_ERR_OR_NULL(streams)) {
		pr_err("[%s] stream=%d streams is null!\n", __func__, stream);
		return -EFAULT;
	}
	substream = pcm->streams[stream].substream;
	if (IS_ERR_OR_NULL(substream)) {
		pr_err("[%s] stream=%d substream is null!\n", __func__, stream);
		return -EFAULT;
	}
	dma_buf = &substream->dma_buffer;
	dma_buf->dev.type = SNDRV_DMA_TYPE_DEV;
	dma_buf->dev.dev = pcm->card->dev;
	dma_buf->private_data = NULL;
	if (buffer_bytes_max > SUNXI_AUDIO_CMA_MAX_BYTES_MAX)
		buffer_bytes_max = SUNXI_AUDIO_CMA_MAX_BYTES_MAX;
	if (buffer_bytes_max < SUNXI_AUDIO_CMA_MAX_BYTES_MIN)
		buffer_bytes_max = SUNXI_AUDIO_CMA_MAX_BYTES_MIN;

	dma_buf->area = dma_alloc_coherent(pcm->card->dev, buffer_bytes_max,
					&dma_buf->addr, GFP_KERNEL);
	if (!dma_buf->area) {
		dev_err(pcm->card->dev, "dmaengine alloc coherent failed.\n");
		return -ENOMEM;
	}
	dma_buf->bytes = buffer_bytes_max;

	return 0;
}

static void sunxi_hifi_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *dma_buf;
	int stream;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		dma_buf = &substream->dma_buffer;
		if (!dma_buf->area)
			continue;

		dma_free_coherent(pcm->card->dev, dma_buf->bytes,
				dma_buf->area, dma_buf->addr);
		dma_buf->area = NULL;
	}
}

static int sunxi_hifi_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_soc_dai_link *dai_link = rtd->dai_link;
	struct snd_pcm *pcm = rtd->pcm;
	struct device *dev = rtd->platform->dev;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct sunxi_dma_params *playback_dma_data = cpu_dai->playback_dma_data;
	struct sunxi_dma_params *capture_dma_data = cpu_dai->capture_dma_data;
	size_t capture_cma_bytes = SUNXI_AUDIO_CMA_BLOCK_BYTES;
	size_t playback_cma_bytes = SUNXI_AUDIO_CMA_BLOCK_BYTES;
	int ret = 0;

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &sunxi_pcm_mask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;

	if (!IS_ERR_OR_NULL(playback_dma_data))
		playback_cma_bytes *= playback_dma_data->cma_kbytes;
	if (!IS_ERR_OR_NULL(capture_dma_data))
		capture_cma_bytes *= capture_dma_data->cma_kbytes;

	if (dai_link->playback_only) {
		ret = sunxi_hifi_pcm_preallocate_stream_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK, playback_cma_bytes);
		if (ret) {
			dev_err(dev, "pcm new playback failed with err=%d\n", ret);
			return ret;
		}
	} else if (dai_link->capture_only) {
		ret = sunxi_hifi_pcm_preallocate_stream_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE, capture_cma_bytes);
		if (ret) {
			dev_err(dev, "pcm new capture failed with err=%d\n", ret);
			return ret;
		}
	} else {
		ret = sunxi_hifi_pcm_preallocate_stream_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK, playback_cma_bytes);
		if (ret) {
			dev_err(dev, "[%s] pcm new playback failed with err=%d\n", pcm->id, ret);
		}
		ret = sunxi_hifi_pcm_preallocate_stream_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE, capture_cma_bytes);
		if (ret) {
			dev_err(dev, "[%s] pcm new capture failed with err=%d\n", pcm->id, ret);
		}
	}

	return 0;
}

static struct snd_soc_platform_driver sunxi_hifi_soc_platform[SUNXI_HIFI_PCM_OPS_MAX] = {
	[SUNXI_HIFI_PCM_OPS_CODEC] = {
		.ops		= &sunxi_hifi_pcm_ops,
		.pcm_new	= sunxi_hifi_pcm_new,
		.pcm_free	= sunxi_hifi_pcm_free_dma_buffers,
	},
	[SUNXI_HIFI_PCM_OPS_DAUDIO] = {
		.ops		= &sunxi_hifi_pcm_ops_daudio,
		.pcm_new	= sunxi_hifi_pcm_new,
		.pcm_free	= sunxi_hifi_pcm_free_dma_buffers,
	},
	[SUNXI_HIFI_PCM_OPS_DMIC] = {
		.ops		= &sunxi_hifi_pcm_ops,
		.pcm_new	= sunxi_hifi_pcm_new,
		.pcm_free	= sunxi_hifi_pcm_free_dma_buffers,
	},
};

/*
 * Register a platform driver with automatic unregistration when the device is
 * unregistered.
 */
int asoc_hifi_platform_register(struct device *dev, unsigned int ops_type)
{
	if (ops_type >= SUNXI_HIFI_PCM_OPS_MAX)
		return -EFAULT;
	return devm_snd_soc_register_platform(dev, &sunxi_hifi_soc_platform[ops_type]);
}
EXPORT_SYMBOL_GPL(asoc_hifi_platform_register);

MODULE_AUTHOR("yumingfeng@allwinnertech.com");
MODULE_DESCRIPTION("sunxi ASoC HiFi platform Driver");
MODULE_LICENSE("GPL");
