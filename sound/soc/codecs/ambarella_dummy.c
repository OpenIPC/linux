/*
 * ambarella_dummy.c  --  A2SAUC ALSA SoC Audio driver
 *
 * History:
 *	2008/10/17 - [Andrew Lu] created file
 *	2009/03/12 - [Cao Rongrong] Port to 2.6.27
 *	2009/06/10 - [Cao Rongrong] Port to 2.6.29
 *	2011/03/20 - [Cao Rongrong] Port to 2.6.38
 *
 * Coryright (c) 2008-2009, Ambarella, Inc.
 * http://www.ambarella.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/of_gpio.h>
#include "ambarella_dummy.h"

static inline unsigned int ambdummy_codec_read(struct snd_soc_codec *codec,
	unsigned int reg)
{
	return 0;
}

static inline int ambdummy_codec_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int value)
{
	return 0;
}

static int ambdummy_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int ambdummy_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	codec->dapm.bias_level = level;
	return 0;
}

#define AMBDUMMY_RATES SNDRV_PCM_RATE_8000_48000

#define AMBDUMMY_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops ambdummy_dai_ops = {
	.digital_mute = ambdummy_mute,
};

static struct snd_soc_dai_driver ambdummy_dai = {
	.name = "AMBARELLA_DUMMY_CODEC",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 6,
		.rates = AMBDUMMY_RATES,
		.formats = AMBDUMMY_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 6,
		.rates = AMBDUMMY_RATES,
		.formats = AMBDUMMY_FORMATS,},
	.ops = &ambdummy_dai_ops,
};

#if defined(CONFIG_PM)
static int ambdummy_suspend(struct snd_soc_codec *codec)
{
	ambdummy_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	ambdummy_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int ambdummy_resume(struct snd_soc_codec *codec)
{
	ambdummy_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	ambdummy_set_bias_level(codec, SND_SOC_BIAS_ON);

	return 0;
}
#else
#define ambdummy_suspend NULL
#define ambdummy_resume NULL
#endif
static int ambdummy_probe(struct snd_soc_codec *codec)
{
	printk(KERN_INFO "AMBARELLA SoC Audio DUMMY Codec\n");

	ambdummy_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	return 0;
}

/* power down chip */
static int ambdummy_remove(struct snd_soc_codec *codec)
{
	ambdummy_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_ambdummy = {
	.probe = 	ambdummy_probe,
	.remove = 	ambdummy_remove,
	.suspend = 	ambdummy_suspend,
	.resume =	ambdummy_resume,
	.reg_cache_size	= 0,
	.read =		ambdummy_codec_read,
	.write =	ambdummy_codec_write,
	.set_bias_level =ambdummy_set_bias_level,
};

static int ambdummy_codec_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_ambdummy, &ambdummy_dai, 1);
}

static int ambdummy_codec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct of_device_id ambdummy_of_match[] = {
	{ .compatible = "ambarella,dummycodec",},
	{},
};
MODULE_DEVICE_TABLE(of, ambdummy_of_match);

static struct platform_driver ambdummy_codec_driver = {
	.probe		= ambdummy_codec_probe,
	.remove		= ambdummy_codec_remove,
	.driver		= {
		.name	= "ambdummy-codec",
		.owner	= THIS_MODULE,
		.of_match_table = ambdummy_of_match,
	},
};

static int __init ambarella_dummy_codec_init(void)
{
	return platform_driver_register(&ambdummy_codec_driver);

}
module_init(ambarella_dummy_codec_init);

static void __exit ambarella_dummy_codec_exit(void)
{
	platform_driver_unregister(&ambdummy_codec_driver);
}
module_exit(ambarella_dummy_codec_exit);

MODULE_DESCRIPTION("Soc Ambarella Dummy Codec Driver");
MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_LICENSE("GPL");

