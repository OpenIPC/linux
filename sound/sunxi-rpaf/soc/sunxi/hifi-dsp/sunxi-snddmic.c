/*
 * sound\sunxi-rpaf\soc\sunxi\hifi-dsp\sunxi-snddmic.c
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
#include <linux/clk.h>
#include <linux/mutex.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/io.h>
#include <linux/of.h>

#include "sunxi-snddmic.h"

static int sunxi_snddmic_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;

	dev_info(card->dev, "%s\n", __func__);

	return 0;
}

/*enable the snddmic suspend */
static int sunxi_snddmic_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;

	dev_info(card->dev, "%s\n", __func__);
	return 0;
}

static void sunxi_snddmic_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;

	dev_info(card->dev, "%s\n", __func__);
}

static struct snd_soc_ops sunxi_snddmic_ops = {
	.hw_params	= sunxi_snddmic_hw_params,
	.startup = sunxi_snddmic_startup,
	.shutdown = sunxi_snddmic_shutdown,
};

static int sunxi_snddmic_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_card *card = rtd->card;

	dev_info(card->dev, "%s\n", __func__);
	return 0;
}

static struct snd_soc_dai_link sunxi_snddmic_dai_link = {
	.name		= "DMIC",
	.stream_name	= "SUNXI-DMIC",
	.codec_name	= "dmic-codec",
	.codec_dai_name = "dmic-hifi",
	.ops		= &sunxi_snddmic_ops,
	.init		= sunxi_snddmic_init,
	.capture_only = 1,
};

static struct snd_soc_card snd_soc_sunxi_snddmic = {
	.name		= "snddmic",
	.owner		= THIS_MODULE,
	.dai_link	= &sunxi_snddmic_dai_link,
	.num_links	= 1,
};

static int sunxi_snddmic_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct snd_soc_card *card = &snd_soc_sunxi_snddmic;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_snddmic_priv *snddmic_priv;

	snddmic_priv = devm_kzalloc(&pdev->dev,
			sizeof(struct sunxi_snddmic_priv), GFP_KERNEL);
	if (!snddmic_priv) {
		dev_err(card->dev, "Can't malloc the sunxi_snddmic_priv!\n");
		return -ENOMEM;
	}
	snddmic_priv->card = card;
	card->dev = &pdev->dev;

	sunxi_snddmic_dai_link.cpu_dai_name = NULL;
	sunxi_snddmic_dai_link.cpu_of_node = of_parse_phandle(np,
					"sunxi,dmic-controller", 0);
	if (!sunxi_snddmic_dai_link.cpu_of_node) {
		dev_err(&pdev->dev, "Property 'sunxi,dmic-controller' missing or invalid\n");
		ret = -EINVAL;
		goto err_of_get_cpunode;
	}
	sunxi_snddmic_dai_link.platform_name = NULL;
	sunxi_snddmic_dai_link.platform_of_node =
				sunxi_snddmic_dai_link.cpu_of_node;

	snddmic_priv->codec_device = platform_device_alloc("dmic-codec", -1);
	if (!snddmic_priv->codec_device) {
		dev_err(&pdev->dev, "dmic codec alloc failed\n");
		ret = -ENOMEM;
		goto err_platform_dev_alloc;
	}

	ret = platform_device_add(snddmic_priv->codec_device);
	if (ret) {
		dev_err(&pdev->dev, "dmic codec add failed\n");
		ret = -EBUSY;
		goto err_platform_dev_add;
	}

	snd_soc_card_set_drvdata(card, snddmic_priv);

	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card() fail: %d\n", ret);
		ret = -EBUSY;
		goto err_snd_register_card;
	}
	return ret;

err_snd_register_card:
	platform_device_del(snddmic_priv->codec_device);
err_platform_dev_add:
	platform_device_put(snddmic_priv->codec_device);
err_platform_dev_alloc:
err_of_get_cpunode:
	devm_kfree(&pdev->dev, snddmic_priv);
	return ret;
}

static int sunxi_snddmic_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct sunxi_snddmic_priv *snddmic_priv =
				snd_soc_card_get_drvdata(card);

	snd_soc_unregister_card(card);
	platform_device_del(snddmic_priv->codec_device);
	platform_device_put(snddmic_priv->codec_device);
	devm_kfree(&pdev->dev, snddmic_priv);
	return 0;
}

static const struct of_device_id sunxi_dmic_of_match[] = {
	{ .compatible = "allwinner,sunxi-dmic-machine", },
	{},
};

static struct platform_driver sunxi_dmic_driver = {
	.driver = {
		.name = "snddmic",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_dmic_of_match,
		.pm = &snd_soc_pm_ops,
	},
	.probe = sunxi_snddmic_dev_probe,
	.remove = sunxi_snddmic_dev_remove,
};
module_platform_driver(sunxi_dmic_driver);

MODULE_AUTHOR("yumingfeng <yumingfeng@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI DMIC Machine ASoC driver");
MODULE_ALIAS("platform:snddmic");
MODULE_LICENSE("GPL");
