/*
 * AMD ALSA SoC PCM Driver for ACP 2.x
 *
 * Copyright 2014-2015 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/sizes.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>

#include <sound/soc.h>
#include <sound/tlv.h>

#include "nvt_alsa.h"
#include "eac.h"
#include "eac_int.h"
#include "eac_reg.h"



#define DBG_WRN(fmt, args...) 	//printk(fmt, ##args)
#define ENABLE					1
#define DISABLE					0

#define EAC_REG_BASE 0xF0640000

#define EAC_SPK_OUT_VOL			0xAC
#define EAC_MIC_IN_VOL			0x20



/*				DECLARE_TLV_DB_SCALE(name,  min,step, mute) */
static const DECLARE_TLV_DB_SCALE(spk_tlv, -9750,  50, 1);// -97 ~ +30dB
static const DECLARE_TLV_DB_SCALE(mic_tlv, -2100, 150, 0);// -21 ~ +25.5dB

static struct snd_kcontrol_new nvt_embedded_controls[] = {
//SOC_DOUBLE_TLV("Lineout Playback Volume",EAC_SPK_OUT_VOL, 0, 8, 255, 0, spk_tlv),
//SOC_DOUBLE_TLV("MIC Capture Volume",EAC_MIC_IN_VOL, 0, 8, 31, 0, mic_tlv),
};

static int nvt_embedded_probe(struct snd_soc_component *codec)
{
	int status;

	status = snd_soc_add_component_controls(codec, nvt_embedded_controls,ARRAY_SIZE(nvt_embedded_controls));

	DBG_WRN("nvt_embedded_probe\n");
	if (status < 0) {
		DBG_WRN("%s: failed to add nvt_embedded_controls (%d).\n",__func__, status);
		return status;
	}

	return 0;
}

static void nvt_embedded_remove(struct snd_soc_component *codec)
{
	//struct adau1701 *adau1701 = snd_soc_codec_get_drvdata(codec);
	DBG_WRN("nvt_embedded_remove\n");
	return;
}

static int nvt_embedded_set_bias_level(struct snd_soc_component *codec, enum snd_soc_bias_level level)
{
	//struct adau1701 *adau1701 = snd_soc_codec_get_drvdata(codec);

	DBG_WRN("nvt_embedded_set_bias_level %d\n",(int)level);

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		break;
	case SND_SOC_BIAS_OFF:
		break;
	}

	return 0;
}


static int nvt_embedded_read_reg(void *context, unsigned int reg,
				 unsigned int *value)
{

	*value = (unsigned int)ioread32((volatile void __iomem *)(_EAC_REG_BASE_ADDR[0]+reg));

	//DBG_WRN("nvt_embedded_read_reg\n");

	return 0;
}

/* Write to a register in the audio-bank of AB8500 */
static int nvt_embedded_write_reg(void *context, unsigned int reg,
				  unsigned int value)
{

	//iowrite32(value, (volatile void __iomem *)(EAC_REG_BASE+reg));

	//set load
	//iowrite32(0x1,  (volatile void __iomem *)(EAC_REG_BASE+0x4));

	DBG_WRN("nvt_embedded_write_reg 0x%08x = 0x%x\n",_EAC_REG_BASE_ADDR[0]+reg,value);

	return 0;
}

static const struct regmap_config nvt_embedded_regmap = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_read = nvt_embedded_read_reg,
	.reg_write = nvt_embedded_write_reg,
};


//! codec
static struct snd_soc_component_driver nvt_embedded_codec = {
	.probe			= nvt_embedded_probe,
	.remove			= nvt_embedded_remove,
	//.resume		= adau1701_resume,
	//.suspend		= adau1701_suspend,
	.set_bias_level		= nvt_embedded_set_bias_level,
	.idle_bias_on		= false,


	/*
	.component_driver = {
		.controls		= adau1701_controls,
		.num_controls		= ARRAY_SIZE(adau1701_controls),
		.dapm_widgets		= adau1701_dapm_widgets,
		.num_dapm_widgets	= ARRAY_SIZE(adau1701_dapm_widgets),
		.dapm_routes		= adau1701_dapm_routes,
		.num_dapm_routes	= ARRAY_SIZE(adau1701_dapm_routes),
	},
	*/

	//.set_sysclk		= adau1701_set_sysclk,
	//.set_pll
};




static int nvt_embd_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	//struct adau1373 *adau1373 = snd_soc_codec_get_drvdata(codec);
	DBG_WRN("nvt_embd_hw_params\n");

	return 0;
}


static int nvt_embd_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	DBG_WRN("nvt_embd_prepare\r\n");

	return 0;
}


static const struct snd_soc_dai_ops nvt_embd_dai_ops = {
	.hw_params		= nvt_embd_hw_params,
	.prepare        = nvt_embd_prepare,
	//.set_sysclk	= adau1373_set_dai_sysclk,
	//.set_fmt		= adau1373_set_dai_fmt,
	//.set_bclk_ratio
};


static struct snd_soc_dai_driver nvt_embd_codec_dais[] = {

	{
		.name = "nvt,codec_dai_l",
		.id = 0,
/*
		.playback = {
			.stream_name 	= "lineout",
			.channels_min 	= 1,
			.channels_max 	= 2,
			.rates 			= SNDRV_PCM_RATE_8000_48000,
			.rate_min 		= 8000,
			.rate_max 		= 48000,
			.formats 		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U8,
		},
*/
		.capture = {
			.stream_name 	= "mic",
			.channels_min 	= 1,
			.channels_max 	= 2,
			.rates 			= SNDRV_PCM_RATE_8000_48000,
			.rate_min 		= 8000,
			.rate_max 		= 48000,
			.formats 		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_U8,
		},
		.ops = &nvt_embd_dai_ops,
	},
};


static int nvt_embd_codec_device_probe(struct platform_device *pdev)
{
	struct regmap *embbed_regmap;
	int status;
	void __iomem *eac_mmio;

	eac_mmio = ioremap_nocache(EAC_REG_BASE, 0x300);
	_EAC_REG_BASE_ADDR[0] = (UINT32)eac_mmio;

	DBG_WRN( "nvt_embd_codec_device_probe\n");


	embbed_regmap = devm_regmap_init(&pdev->dev, NULL, &pdev->dev,
					   &nvt_embedded_regmap);
	if (IS_ERR(embbed_regmap)) {
		status = PTR_ERR(embbed_regmap);
		dev_err(&pdev->dev, "%s: Failed to allocate regmap: %d\n",
			__func__, status);
		return status;
	}

	return snd_soc_register_component(&pdev->dev, &nvt_embedded_codec, nvt_embd_codec_dais, ARRAY_SIZE(nvt_embd_codec_dais));
}

static int nvt_embd_codec_device_remove (struct platform_device *pdev)
{
	DBG_WRN( "nvt_embd_codec_device_remove\n");

	snd_soc_unregister_component(&pdev->dev);
	return 0;
}


static struct platform_driver nvt_embd_codec_driver = {
	.driver		= {
		.name		= "nvt,embd_codec_l",
		.owner		= THIS_MODULE,
	},
	.probe		= nvt_embd_codec_device_probe,
	.remove		= nvt_embd_codec_device_remove,
};


static struct platform_device *nvt_pdev;


int __init nvt_alsa_embd_module_init(void)
{
	int iRet;
	struct platform_device *pdev;

	DBG_WRN("nvt_alsa_embd_module_init\r\n");

	iRet = platform_driver_register(&nvt_embd_codec_driver);
	if (iRet) {
		printk("nvt_alsa_embd_module_init platform_driver_register error\n");
	}

	pdev = platform_device_alloc("nvt,embd_codec_l",-1);
	if (!pdev)
	{
		printk("platform_device_alloc failed!!\n");
	}

	iRet = platform_device_add(pdev);
	if (iRet)
	{
		platform_device_put(pdev);
		printk( "platform_device_add failed(%d)! \n", iRet);
	}
	nvt_pdev = pdev;

	return 0;
}

void __exit nvt_alsa_embd_module_exit(void)
{
	DBG_WRN("nvt_alsa_embd_module_exit\r\n");

	platform_device_unregister(nvt_pdev);
	platform_driver_unregister(&nvt_embd_codec_driver);
}

module_init(nvt_alsa_embd_module_init);
module_exit(nvt_alsa_embd_module_exit);


//module_platform_driver(nvt_dma_driver);

MODULE_AUTHOR("klins_chen@novatek.com.tw");
MODULE_DESCRIPTION("NOVATEK ALSA Embedded Codec Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:snd-soc-nvt-embd");
