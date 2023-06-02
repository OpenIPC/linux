/*
 * ad73311.c  --  ALSA Soc AD73311 codec support
 *
 * Copyright:	Analog Device Inc.
 * Author:	Cliff Cai <cliff.cai@analog.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "fsh0ls029aa.h"
static int ak4104_set_dai_fmt(){

	return 0;
}
static int ak4104_hw_params(){

	return 0;
}
static int ak4104_sys_params(){

	return 0;
}
static struct snd_soc_dai_ops ak4101_dai_ops = {
	.hw_params = ak4104_hw_params,
	.set_fmt = ak4104_set_dai_fmt,
	.set_sysclk=ak4104_sys_params,
};

static struct snd_soc_dai_driver ad73311_dai = {
	.name = "fh-acodec-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE, },
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE, },
		.ops=&ak4101_dai_ops,
};

static struct snd_soc_codec_driver soc_codec_dev_ad73311;

static int ad73311_probe(struct platform_device *pdev)
{
//	printk("ad73311 probe \n");
	return snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_ad73311, &ad73311_dai, 1);
}

static int __devexit ad73311_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver ad73311_codec_driver = {
	.driver = {
			.name = "fh-acodec",
			.owner = THIS_MODULE,
	},

	.probe = ad73311_probe,
	.remove = __devexit_p(ad73311_remove),
};

static int __init ad73311_init(void)
{
	return platform_driver_register(&ad73311_codec_driver);
}
module_init(ad73311_init);

static void __exit ad73311_exit(void)
{
	platform_driver_unregister(&ad73311_codec_driver);
}
module_exit(ad73311_exit);

MODULE_DESCRIPTION("ASoC ad73311 driver");
MODULE_AUTHOR("Cliff Cai ");
MODULE_LICENSE("GPL");
