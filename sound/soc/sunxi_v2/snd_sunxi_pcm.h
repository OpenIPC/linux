/* sound\soc\sunxi\snd_sunxi_pcm.h
 * (C) Copyright 2021-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_PCM_H
#define __SND_SUNXI_PCM_H

#define SUNXI_AUDIO_CMA_BLOCK_BYTES	1024
#define SUNXI_AUDIO_CMA_MAX_KBYTES	1024
#define SUNXI_AUDIO_CMA_MIN_KBYTES	64
#define SUNXI_AUDIO_CMA_MAX_BYTES	(SUNXI_AUDIO_CMA_BLOCK_BYTES * SUNXI_AUDIO_CMA_MAX_KBYTES)
#define SUNXI_AUDIO_CMA_MIN_BYTES	(SUNXI_AUDIO_CMA_BLOCK_BYTES * SUNXI_AUDIO_CMA_MIN_KBYTES)

#define SUNXI_AUDIO_FIFO_SIZE		128

struct sunxi_dma_params {
	char *name;
	dma_addr_t dma_addr;
	u8 src_maxburst;
	u8 dst_maxburst;
	u8 dma_drq_type_num;

	/* max buffer set (value must be (2^n)Kbyte) */
	size_t cma_kbytes;
	size_t fifo_size;
};

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_PCM)
extern int snd_sunxi_dma_platform_register(struct device *dev);
extern void snd_sunxi_dma_platform_unregister(struct device *dev);
#else
static inline int snd_sunxi_dma_platform_register(struct device *dev)
{
	pr_err("[sound %4d][PCM %s] PCM API is disabled\n", __LINE__, __func__);
	return 0;
}

static inline void snd_sunxi_dma_platform_unregister(struct device *dev)
{
	pr_err("[sound %4d][PCM %s] PCM API is disabled\n", __LINE__, __func__);
}
#endif

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_PCM_HDMI)
extern int snd_sunxi_hdmi_platform_register(struct device *dev);
extern void snd_sunxi_hdmi_platform_unregister(struct device *dev);
#else
static inline int snd_sunxi_hdmi_platform_register(struct device *dev)
{
	pr_err("[sound %4d][PCM %s] PCM HDMI API is disabled\n", __LINE__, __func__);
	return 0;
}

static inline void snd_sunxi_hdmi_platform_unregister(struct device *dev)
{
	pr_err("[sound %4d][PCM %s] PCM HDMI API is disabled\n", __LINE__, __func__);
}
#endif

#endif /* __SND_SUNXI_PCM_H */
