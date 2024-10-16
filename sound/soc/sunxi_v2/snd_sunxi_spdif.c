/*
 * sound\soc\sunxi\snd_sunxi_spdif.c
 * (C) Copyright 2021-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <sound/soc.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_pcm.h"
#include "snd_sunxi_spdif.h"

#define HLOG		"SPDIF"
#define DRV_NAME	"sunxi-snd-plat-spdif"

static int sunxi_spdif_dai_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *params,
				     struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_spdif_dai_set_sysclk(struct snd_soc_dai *dai,
				      int clk_id,
				      unsigned int freq,
				      int dir)
{
	return 0;
}

static int sunxi_spdif_dai_trigger(struct snd_pcm_substream *substream,
				   int cmd,
				   struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_spdif_dai_prepare(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	return 0;
}

static const struct snd_soc_dai_ops sunxi_spdif_dai_ops = {
	.hw_params	= sunxi_spdif_dai_hw_params,
	.set_sysclk	= sunxi_spdif_dai_set_sysclk,
	.trigger	= sunxi_spdif_dai_trigger,
	.prepare	= sunxi_spdif_dai_prepare,
};

static int sunxi_spdif_probe(struct snd_soc_dai *dai)
{
	SND_LOG_DEBUG(HLOG, "\n");

	return 0;
}

static int sunxi_spdif_suspend(struct snd_soc_dai *dai)
{
	SND_LOG_DEBUG(HLOG, "\n");

	return 0;
}

static int sunxi_spdif_resume(struct snd_soc_dai *dai)
{
	SND_LOG_DEBUG(HLOG, "\n");

	return 0;
}

static struct snd_soc_dai_driver sunxi_spdif_dai = {
	.probe = sunxi_spdif_probe,
	.suspend = sunxi_spdif_suspend,
	.resume = sunxi_spdif_resume,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE,
	},
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
			| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE,
	},
	.ops = &sunxi_spdif_dai_ops,
};

static struct snd_soc_component_driver sunxi_spdif_dev = {
	.name		= DRV_NAME,
};

static int sunxi_spdif_dev_probe(struct platform_device *pdev)
{
	int ret;

	ret = snd_soc_register_component(&pdev->dev,
					 &sunxi_spdif_dev,
					 &sunxi_spdif_dai, 1);
	if (ret) {
		SND_LOG_ERR(HLOG, "component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	ret = snd_sunxi_dma_platform_register(&pdev->dev);
	if (ret) {
		SND_LOG_ERR(HLOG, "register ASoC platform failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_sunxi_dma_platform_register;
	}

	SND_LOG_DEBUG(HLOG, "register spdif platform success\n");

	return 0;

err_snd_soc_sunxi_dma_platform_register:
	snd_soc_unregister_component(&pdev->dev);
err_snd_soc_register_component:
	return ret;
}

static int sunxi_spdif_dev_remove(struct platform_device *pdev)
{
	snd_sunxi_dma_platform_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);

	SND_LOG_DEBUG(HLOG, "unregister spdif platform success\n");

	return 0;
}

static const struct of_device_id sunxi_spdif_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_spdif_of_match);

static struct platform_driver sunxi_spdif_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_spdif_of_match,
	},
	.probe	= sunxi_spdif_dev_probe,
	.remove	= sunxi_spdif_dev_remove,
};

module_platform_driver(sunxi_spdif_driver);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard platform of spdif");
