/*
 * sound\soc\sunxi\sun50iw11\sunxi-pcm.c
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
#include "sunxi-pcm.h"

static u64 sunxi_pcm_mask = DMA_BIT_MASK(32);

static struct snd_pcm_hardware sunxi_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED
				| SNDRV_PCM_INFO_BLOCK_TRANSFER
				| SNDRV_PCM_INFO_MMAP
				| SNDRV_PCM_INFO_MMAP_VALID
				| SNDRV_PCM_INFO_PAUSE
				| SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S8
				| SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	.rates			= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 16,
	/* value must be (2^n)Kbyte */
	.buffer_bytes_max	= SUNXI_AUDIO_CMA_MAX_BYTES_MAX,
	.period_bytes_min	= 256,
	/* value must be (2^(n-1))Kbyte */
	.period_bytes_max	= SUNXI_AUDIO_CMA_MAX_BYTES_MAX / 2,
	.periods_min		= 2,
	.periods_max		= 32,
	.fifo_size		= 128,
};

static int sunxi_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct sunxi_dma_params *dmap;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
	struct dma_chan *chan = snd_dmaengine_pcm_get_chan(substream);
	struct dma_slave_config slave_config;
	int ret;

	dmap = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);

	ret = snd_hwparams_to_dma_slave_config(substream, params,
					&slave_config);
	if (ret) {
		dev_err(dev, "hw params config failed with err %d\n", ret);
		return ret;
	}

	slave_config.dst_maxburst = dmap->dst_maxburst;
	slave_config.src_maxburst = dmap->src_maxburst;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		slave_config.dst_addr = dmap->dma_addr;
		slave_config.src_addr_width = slave_config.dst_addr_width;
		slave_config.slave_id = sunxi_slave_id(dmap->dma_drq_type_num,
						DRQSRC_SDRAM);
	} else {
		slave_config.src_addr =	dmap->dma_addr;
		slave_config.dst_addr_width = slave_config.src_addr_width;
		slave_config.slave_id = sunxi_slave_id(DRQDST_SDRAM,
						dmap->dma_drq_type_num);
	}

	ret = dmaengine_slave_config(chan, &slave_config);
	if (ret < 0) {
		dev_err(dev, "dma slave config failed with err %d\n", ret);
		return ret;
	}

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	return 0;
}

static int sunxi_pcm_hw_free(struct snd_pcm_substream *substream)
{
	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}

static int sunxi_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			snd_dmaengine_pcm_trigger(substream,
					SNDRV_PCM_TRIGGER_START);
			break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			snd_dmaengine_pcm_trigger(substream,
					SNDRV_PCM_TRIGGER_STOP);
			break;
		}
	} else {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			snd_dmaengine_pcm_trigger(substream,
					SNDRV_PCM_TRIGGER_START);
			break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			snd_dmaengine_pcm_trigger(substream,
					SNDRV_PCM_TRIGGER_STOP);
			break;
		}
	}
	return 0;
}

static int sunxi_pcm_open(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct device *dev = rtd->platform->dev;
	struct sunxi_dma_params *dma_params = NULL;

	dma_params = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);
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
	ret = snd_dmaengine_pcm_open_request_chan(substream, NULL,
				NULL);
	if (ret < 0) {
		dev_err(dev, "dmaengine pcm open failed with err %d\n", ret);
		return ret;
	}
	return 0;
}

static int sunxi_pcm_mmap(struct snd_pcm_substream *substream,
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

static int sunxi_pcm_copy(struct snd_pcm_substream *substream, int a,
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

static snd_pcm_uframes_t sunxi_dmaengine_pcm_pointer(struct snd_pcm_substream *substream)
{
	return snd_dmaengine_pcm_pointer(substream);
}

static struct snd_pcm_ops sunxi_pcm_ops = {
	.open		= sunxi_pcm_open,
	.close		= snd_dmaengine_pcm_close_release_chan,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= sunxi_pcm_hw_params,
	.hw_free	= sunxi_pcm_hw_free,
	.trigger	= sunxi_pcm_trigger,
	.pointer	= sunxi_dmaengine_pcm_pointer,
	.mmap		= sunxi_pcm_mmap,
	.copy		= sunxi_pcm_copy,
};

static int sunxi_pcm_preallocate_stream_dma_buffer(struct snd_pcm *pcm,
			int stream, size_t buffer_bytes_max)
{
	struct snd_pcm_str *streams = NULL;
	struct snd_pcm_substream *substream = NULL;
	struct snd_dma_buffer *buf = NULL;

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
	buf = &substream->dma_buffer;
	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	if (buffer_bytes_max > SUNXI_AUDIO_CMA_MAX_BYTES_MAX)
		buffer_bytes_max = SUNXI_AUDIO_CMA_MAX_BYTES_MAX;
	if (buffer_bytes_max < SUNXI_AUDIO_CMA_MAX_BYTES_MIN)
		buffer_bytes_max = SUNXI_AUDIO_CMA_MAX_BYTES_MIN;

	buf->area = dma_alloc_coherent(pcm->card->dev, buffer_bytes_max,
					&buf->addr, GFP_KERNEL);
	if (!buf->area) {
		dev_err(pcm->card->dev, "dmaengine alloc coherent failed.\n");
		return -ENOMEM;
	}
	buf->bytes = buffer_bytes_max;

	return 0;
}

static void sunxi_pcm_free_stream_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf;

	if (IS_ERR_OR_NULL(substream)) {
		pr_err("[%s] stream=%d substream is null!\n", __func__, stream);
		return;
	}

	buf = &substream->dma_buffer;
	if (!buf->area) {
		pr_err("[%s] stream=%d buf->area is null!\n", __func__, stream);
		return;
	}

	dma_free_coherent(pcm->card->dev, buf->bytes,
			buf->area, buf->addr);
	buf->area = NULL;
}

static void sunxi_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_coherent(pcm->card->dev, buf->bytes,
				buf->area, buf->addr);
		buf->area = NULL;
	}
}

static int sunxi_pcm_new(struct snd_soc_pcm_runtime *rtd)
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
		ret = sunxi_pcm_preallocate_stream_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK, playback_cma_bytes);
		if (ret) {
			dev_err(dev, "pcm new playback failed with err=%d\n", ret);
			return ret;
		}
	} else if (dai_link->capture_only) {
		ret = sunxi_pcm_preallocate_stream_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE, capture_cma_bytes);
		if (ret) {
			dev_err(dev, "pcm new capture failed with err=%d\n", ret);
			return ret;
		}
	} else {
		ret = sunxi_pcm_preallocate_stream_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK, playback_cma_bytes);
		if (ret && pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
			dev_err(dev, "pcm new playback failed with err=%d\n", ret);
			goto err_pcm_prealloc_playback_buffer;
		}
		ret = sunxi_pcm_preallocate_stream_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE, capture_cma_bytes);
		if (ret && pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
			dev_err(dev, "pcm new capture failed with err=%d\n", ret);
			goto err_pcm_prealloc_capture_buffer;
		}
	}

	return 0;

err_pcm_prealloc_capture_buffer:
	sunxi_pcm_free_stream_dma_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);
err_pcm_prealloc_playback_buffer:
	return ret;
}

static struct snd_soc_platform_driver sunxi_soc_platform = {
	.ops		= &sunxi_pcm_ops,
	.pcm_new	= sunxi_pcm_new,
	.pcm_free	= sunxi_pcm_free_dma_buffers,
};

/*
 * Register a platform driver with automatic unregistration when the device is
 * unregistered.
 */
int asoc_dma_platform_register(struct device *dev, unsigned int flags)
{
	return devm_snd_soc_register_platform(dev, &sunxi_soc_platform);
}
EXPORT_SYMBOL_GPL(asoc_dma_platform_register);

MODULE_AUTHOR("yumingfeng@allwinnertech.com");
MODULE_DESCRIPTION("sunxi ASoC DMA Driver");
MODULE_LICENSE("GPL");
