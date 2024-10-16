/* sound\soc\sunxi\hifi-dsp\sunxi-pcm.h
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
#ifndef __SUNXI_HIFI_PCM_H_
#define __SUNXI_HIFI_PCM_H_

#define SUNXI_AUDIO_CMA_BLOCK_BYTES 1024
#define SUNXI_AUDIO_CMA_MAX_KBYTES 1024
#define SUNXI_AUDIO_CMA_MIN_KBYTES 64
#define SUNXI_AUDIO_CMA_MAX_BYTES_MAX \
		(SUNXI_AUDIO_CMA_BLOCK_BYTES * SUNXI_AUDIO_CMA_MAX_KBYTES)
#define SUNXI_AUDIO_CMA_MAX_BYTES_MIN \
		(SUNXI_AUDIO_CMA_BLOCK_BYTES * SUNXI_AUDIO_CMA_MIN_KBYTES)

#include <sound/pcm.h>
#include <linux/dmaengine.h>
#include <sound/aw_rpaf/common.h>
#include <sound/aw_rpaf/mixer.h>
#include <sound/aw_rpaf/debug.h>
#include <sound/aw_rpaf/substream.h>
#include <sound/aw_rpaf/component.h>
#include <sound/aw_rpaf/component-core.h>

/* sync from kernel/sound/core/pcm_dmaengine.c */
struct dmaengine_pcm_runtime_data {
	struct dma_chan *dma_chan;
	dma_cookie_t cookie;

	unsigned int pos;
};

struct sunxi_dma_params {
	char *name;
	dma_addr_t dma_addr;
	/* value must be (2^n)Kbyte */
	size_t cma_kbytes;
	size_t fifo_size;

	unsigned char *dma_area;/* virtual pointer */
	dma_addr_t phy_addr;	/* physical address */
	size_t phy_bytes;			/* buffer size in bytes */
};

enum sunxi_hifi_pcm_ops_type {
	SUNXI_HIFI_PCM_OPS_CODEC = 0,
	SUNXI_HIFI_PCM_OPS_DAUDIO = 1,
	SUNXI_HIFI_PCM_OPS_DMIC = 2,
	SUNXI_HIFI_PCM_OPS_MAX,
};

int sunxi_dma_params_alloc_dma_area(struct snd_soc_dai *dai,
						struct sunxi_dma_params *dma_params);
int sunxi_dma_params_free_dma_area(struct snd_soc_dai *dai,
						struct sunxi_dma_params *dma_params);

int asoc_hifi_platform_register(struct device *dev, unsigned int ops_type);
#endif /* __SUNXI_PCM_H_ */
