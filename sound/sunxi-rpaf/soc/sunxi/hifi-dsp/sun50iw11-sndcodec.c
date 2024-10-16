/*
 * sound\soc\sunxi\hifi-dsp\sunx50iw11-sndcodec.c
 *
 * (C) Copyright 2019-2025
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnetech.com>
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
#include <linux/io.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <linux/of.h>
#include <sound/pcm_params.h>

#include "sun50iw11-sndcodec.h"

static int sunxi_hifi_sndcodec_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;

	dev_info(card->dev, "%s\n", __func__);
	return 0;
}

static int sunxi_hifi_sndcodec_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;

	dev_info(card->dev, "%s\n", __func__);
	return 0;
}

static void sunxi_hifi_sndcodec_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;

	dev_info(card->dev, "%s\n", __func__);
}

/*
 * Card initialization
 */
static int sunxi_hifi_sndcodec_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_card *card = rtd->card;

	dev_info(card->dev, "%s\n", __func__);
	return 0;
}

static struct snd_soc_ops sunxi_hifi_sndcodec_ops = {
	.startup = sunxi_hifi_sndcodec_startup,
	.hw_params	= sunxi_hifi_sndcodec_hw_params,
	.shutdown = sunxi_hifi_sndcodec_shutdown,
};

static int sunxi_hifi_sndcodec_suspend(struct snd_soc_card *card)
{
	return 0;
}

static int sunxi_hifi_sndcodec_resume(struct snd_soc_card *card)
{
	return 0;
}

static struct snd_soc_dai_link sunxi_hifi_sndcodec_dai_link = {
	.name		= "audiocodec",
	.stream_name	= "SUNXI-CODEC",
	.cpu_dai_name	= "sunxi-internal-cpudai",
	.codec_dai_name = "sunxi-hifi-codec",
	.platform_name	= "sunxi-internal-cpudai",
	.codec_name	= "sunxi-internal-codec",
	.init		= sunxi_hifi_sndcodec_init,
	.ops		= &sunxi_hifi_sndcodec_ops,
};

static struct snd_soc_card snd_soc_sunxi_hifi_sndcodec = {
	.name		= "audiocodec",
	.owner		= THIS_MODULE,
	.dai_link	= &sunxi_hifi_sndcodec_dai_link,
	.num_links	= 1,
	.suspend_post	= sunxi_hifi_sndcodec_suspend,
	.resume_post	= sunxi_hifi_sndcodec_resume,
};

static int sunxi_hifi_sndcodec_dev_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct snd_soc_card *card = &snd_soc_sunxi_hifi_sndcodec;
	struct sunxi_sndcodec_priv *sndcodec_priv;
	struct snd_soc_dai_link *dai_link = &sunxi_hifi_sndcodec_dai_link;
	int ret = 0;

	sndcodec_priv = devm_kzalloc(&pdev->dev,
		sizeof(struct sunxi_sndcodec_priv), GFP_KERNEL);
	if (!sndcodec_priv)
		return -ENOMEM;

	dai_link->cpu_dai_name = NULL;
	dai_link->cpu_of_node = of_parse_phandle(np,
					"sunxi,cpudai-controller", 0);
	if (!dai_link->cpu_of_node) {
		dev_err(&pdev->dev, "Property 'sunxi,cpudai-controller' missing or invalid\n");
		ret = -EINVAL;
		goto err_devm_kfree;
	} else {
		dai_link->platform_name = NULL;
		dai_link->platform_of_node = dai_link->cpu_of_node;
	}
	dai_link->codec_name = NULL;
	dai_link->codec_of_node = of_parse_phandle(np,
						"sunxi,audio-codec", 0);
	if (!dai_link->codec_of_node) {
		dev_err(&pdev->dev, "Property 'sunxi,audio-codec' missing or invalid\n");
		ret = -EINVAL;
		goto err_devm_kfree;
	}

	/* register the soc card */
	card->dev = &pdev->dev;

	snd_soc_card_set_drvdata(card, sndcodec_priv);
	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
		goto err_devm_kfree;
	}

	return 0;

err_devm_kfree:
	devm_kfree(&pdev->dev, sndcodec_priv);
	return ret;
}

static int __exit sunxi_hifi_sndcodec_dev_remove(struct platform_device *pdev)
{
	struct sunxi_sndcodec_priv *sndcodec_priv = dev_get_drvdata(&pdev->dev);

	snd_soc_unregister_card(&snd_soc_sunxi_hifi_sndcodec);
	devm_kfree(&pdev->dev, sndcodec_priv);

	return 0;
}

static const struct of_device_id sunxi_card_of_match[] = {
	{ .compatible = "allwinner,sunxi-codec-machine", },
	{},
};

static struct platform_driver sunxi_machine_driver = {
	.driver = {
		.name = "sunxi-codec-machine",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = sunxi_card_of_match,
	},
	.probe = sunxi_hifi_sndcodec_dev_probe,
	.remove = __exit_p(sunxi_hifi_sndcodec_dev_remove),
};

module_platform_driver(sunxi_machine_driver);

MODULE_AUTHOR("yumingfeng <yumingfeng@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI Codec Machine ASoC HiFi driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-codec-machine");
