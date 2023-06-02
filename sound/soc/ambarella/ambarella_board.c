/*
 * sound/soc/ambarella_board.c
 *
 * Author: XianqingZheng <xqzheng@ambarella.com>
 *
 * History:
 *	2015/04/28 - [XianqingZheng] Created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/module.h>
#include <linux/of.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <plat/audio.h>
#include <linux/slab.h>
#include <sound/pcm_params.h>

static int fast_boot = 0;
module_param(fast_boot, uint, 0664);

static unsigned int dai_fmt;
module_param(dai_fmt, uint, 0644);
MODULE_PARM_DESC(dai_fmt, "DAI format.");
/* clk_fmt :
* 0 : mclk, bclk provided by cpu
* 1 : bclk provide by cpu, mclk is not used
* 2 : mclk provided by cpu, bclk is provided by codec
* There are four type of connection:
* clk_fmt=0:
* cpu :                   codec:
*     MCLK    ------>  MCKI
*     BCLK    ------>  BICK
*     LRCK    ------>  LRCK
*                   ...
* clk_fmt=1:
* cpu :                   codec:
*     MCLK   is not used
*     BCLK    ------> BICK
*     LRCK    ------> LRCK
*                    ...
* clk_fmt=2:
* cpu :                   codec:
*     MCLK   ------> MCKI
*     BCLK   <------ BICK
*     LRCK   <------ LRCK
* There are one connection we are not used, it is like clk_fmt=0,but power on the codec PLL.
* It is a waster of power, so we do not use it.
*/
static unsigned int dummy_dai_fmt;
module_param(dummy_dai_fmt, uint, 0644);

static unsigned int clk_fmt;
module_param(clk_fmt, uint, 0664);

struct amb_clk {
	int mclk;
	int i2s_div;
	int bclk;
};

static int amba_clk_config(struct snd_pcm_hw_params *params, struct amb_clk *clk)
{
	u32 channels, sample_bits, rate;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		sample_bits = 32;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	default:
		sample_bits = 16;
		break;
	}

	channels = params_channels(params);
	rate = params_rate(params);

	clk->mclk = 12288000;
	clk->bclk = channels * rate * sample_bits;

	clk->i2s_div = (clk->mclk / ( 2 * clk->bclk)) - 1;

	return 0;
}

static int amba_general_board_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int rval = 0;
	int i2s_mode;
	struct amb_clk clk={0};

	rval = amba_clk_config(params, &clk);
	if (rval < 0) {
		pr_err("amba can not support the sample rate\n");
		goto hw_params_exit;
	}

	if (dai_fmt == 0)
		i2s_mode = SND_SOC_DAIFMT_I2S;
	else
		i2s_mode = SND_SOC_DAIFMT_DSP_A;

	/* set the I2S system data format*/
	if (clk_fmt == 2) {
		rval = snd_soc_dai_set_fmt(codec_dai,
			i2s_mode | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
		if (rval < 0) {
			pr_err("can't set codec DAI configuration\n");
			goto hw_params_exit;
		}

		rval = snd_soc_dai_set_fmt(cpu_dai,
			i2s_mode | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
		if (rval < 0) {
			pr_err("can't set cpu DAI configuration\n");
			goto hw_params_exit;
		}
	} else {

		rval = snd_soc_dai_set_fmt(codec_dai,
			i2s_mode | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
		if (rval < 0) {
			pr_err("can't set codec DAI configuration\n");
			goto hw_params_exit;
		}

		rval = snd_soc_dai_set_fmt(cpu_dai,
			i2s_mode | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
		if (rval < 0) {
			pr_err("can't set cpu DAI configuration\n");
			goto hw_params_exit;
		}
	}

	/* set the I2S system clock*/
	switch(clk_fmt) {
	case 0:
		rval = snd_soc_dai_set_sysclk(codec_dai, clk_fmt, clk.mclk, 0);
		break;
	case 1:
		rval = snd_soc_dai_set_sysclk(codec_dai, clk_fmt, clk.bclk, 0);
		break;
	case 2:
		rval = snd_soc_dai_set_sysclk(codec_dai, clk_fmt, clk.mclk, 0);
		break;
	default:
		pr_err("clk_fmt is wrong, just 0, 1, 2 is available!\n");
		goto hw_params_exit;
	}

	if (rval < 0) {
		pr_err("can't set codec MCLK configuration\n");
		goto hw_params_exit;
	}


	rval = snd_soc_dai_set_sysclk(cpu_dai, AMBARELLA_CLKSRC_ONCHIP, clk.mclk, 0);
	if (rval < 0) {
		pr_err("can't set cpu MCLK configuration\n");
		goto hw_params_exit;
	}

	rval = snd_soc_dai_set_clkdiv(cpu_dai, AMBARELLA_CLKDIV_LRCLK, clk.i2s_div);
	if (rval < 0) {
		pr_err("can't set cpu MCLK/SF ratio\n");
		goto hw_params_exit;
	}

hw_params_exit:
	return rval;
}

static int amba_dummy_board_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int i2s_mode, rval = 0;
	struct amb_clk clk={0};

	if (fast_boot)
		return 0;
	rval = amba_clk_config(params, &clk);
	if (rval < 0) {
		pr_err("amba can not support the sample rate\n");
		goto hw_params_exit;
	}

	if (dummy_dai_fmt == 0)
		i2s_mode = SND_SOC_DAIFMT_I2S;
	else
		i2s_mode = SND_SOC_DAIFMT_DSP_A;

	/* set the I2S system data format*/
	if(clk_fmt == 2) {
		rval = snd_soc_dai_set_fmt(cpu_dai,
			i2s_mode | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
	} else {
		rval = snd_soc_dai_set_fmt(cpu_dai,
			i2s_mode | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	}

	if (rval < 0) {
		printk(KERN_ERR "can't set cpu DAI configuration\n");
		goto hw_params_exit;
	}

	/* set the I2S system clock*/
	rval = snd_soc_dai_set_sysclk(cpu_dai, AMBARELLA_CLKSRC_ONCHIP, clk.mclk, 0);
	if (rval < 0) {
		printk(KERN_ERR "can't set cpu MCLK configuration\n");
		goto hw_params_exit;
	}

	rval = snd_soc_dai_set_clkdiv(cpu_dai, AMBARELLA_CLKDIV_LRCLK, clk.i2s_div);
	if (rval < 0) {
		printk(KERN_ERR "can't set cpu MCLK/SF ratio\n");
		goto hw_params_exit;
	}

hw_params_exit:
	return rval;
}

static struct snd_soc_ops amba_general_board_ops = {
	.hw_params = amba_general_board_hw_params,
};

static struct snd_soc_ops amba_dummy_board_ops = {
	.hw_params = amba_dummy_board_hw_params,
};

/* ambevk machine dapm widgets */
static const struct snd_soc_dapm_widget amba_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Mic internal", NULL),
	SND_SOC_DAPM_MIC("Mic external", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),
	SND_SOC_DAPM_LINE("Line Out", NULL),
	SND_SOC_DAPM_HP("HP Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker", NULL),
};

static int amba_codec_init(struct snd_soc_pcm_runtime *rtd)
{
	return 0;
}

/* ambevk audio machine driver */
static struct snd_soc_card snd_soc_card_amba = {
	.owner = THIS_MODULE,
	.dapm_widgets = amba_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(amba_dapm_widgets),
};

static int amba_soc_snd_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *cpup_np, *codec_np, *dummy_codec_np;
	struct snd_soc_dai_link *amba_dai_link;
	struct snd_soc_card *card = &snd_soc_card_amba;
	int rval = 0;

	card->dev = &pdev->dev;

	if (snd_soc_of_parse_card_name(card, "amb,model")) {
		dev_err(&pdev->dev, "Card name is not provided\n");
		return -ENODEV;
	}

	cpup_np = of_parse_phandle(np, "amb,i2s-controllers", 0);
	if (fast_boot)
		codec_np = NULL;
	else
		codec_np = of_parse_phandle(np, "amb,audio-codec", 0);
	dummy_codec_np = of_parse_phandle(np, "amb,dummy-codec", 0);
	if (!cpup_np || !dummy_codec_np) {
		dev_err(&pdev->dev, "phandle missing or invalid\n");
		return -EINVAL;
	}

	if(codec_np) {
		rval = snd_soc_of_parse_audio_routing(card,
		"amb,audio-routing");
		if(rval) {
			dev_err(&pdev->dev, "amb,audio-routing is not specified, please doube check\n");
		}

		/*alloc two amba_dai_link for audio codec and dummy codec*/
		amba_dai_link = devm_kzalloc(&pdev->dev, 2 * sizeof(*amba_dai_link), GFP_KERNEL);
		if (amba_dai_link == NULL) {
			dev_err(&pdev->dev, "alloc memory for amba_dai_link fail!\n");
			return -ENOMEM;
		}

		amba_dai_link[0].codec_of_node = codec_np;
		amba_dai_link[0].cpu_of_node = cpup_np;
		amba_dai_link[0].platform_of_node = cpup_np;
		amba_dai_link[0].init = amba_codec_init;
		amba_dai_link[0].ops = &amba_general_board_ops;

		amba_dai_link[1].codec_of_node = dummy_codec_np;
		amba_dai_link[1].cpu_of_node = cpup_np;
		amba_dai_link[1].platform_of_node = cpup_np;
		amba_dai_link[1].init = amba_codec_init;
		amba_dai_link[1].ops = &amba_dummy_board_ops;

		card->dai_link = amba_dai_link;
		card->num_links = 2;

		/*get parameter from code_np to fill struct snd_soc_dai_link*/
		rval = of_property_read_string_index(np, "amb,codec-name", 0, &card->dai_link[0].name);
		if(rval < 0) {
			dev_err(&pdev->dev, "codec-name got from device tree is invalid\n");
			return -EINVAL;
		}

		rval = of_property_read_string_index(np, "amb,stream-name", 0, &card->dai_link[0].stream_name);
		if(rval < 0) {
			dev_err(&pdev->dev, "stream-name got from device tree is invalid\n");
			return -EINVAL;
		}

		rval = of_property_read_string_index(np, "amb,codec-dai-name", 0, &card->dai_link[0].codec_dai_name);
		if(rval < 0) {
			dev_err(&pdev->dev, "codec-dai-name got from device tree is invalid\n");
			return -EINVAL;
		}

		card->dai_link[1].name = "AMB-DUMMY";
		card->dai_link[1].stream_name = "AMB-DUMMY-STREAM";
		card->dai_link[1].codec_dai_name = "AMBARELLA_DUMMY_CODEC";

	} else {
		/*alloc only one amba_dai_link for dummy codec*/
		amba_dai_link = devm_kzalloc(&pdev->dev, sizeof(*amba_dai_link), GFP_KERNEL);
		if (amba_dai_link == NULL) {
			dev_err(&pdev->dev, "alloc memory for amba_dai_link fail!\n");
			return -ENOMEM;
		}

		amba_dai_link[0].codec_of_node = dummy_codec_np;
		amba_dai_link[0].cpu_of_node = cpup_np;
		amba_dai_link[0].platform_of_node = cpup_np;
		amba_dai_link[0].init = amba_codec_init;
		amba_dai_link[0].ops = &amba_dummy_board_ops;
		card->dai_link = amba_dai_link;
		card->num_links = 1;
		card->dai_link[0].name = "AMB-DUMMY";
		card->dai_link[0].stream_name = "AMB-DUMMY-STREAM";
		card->dai_link[0].codec_dai_name = "AMBARELLA_DUMMY_CODEC";
	}

	of_node_put(codec_np);
	of_node_put(cpup_np);
	of_node_put(dummy_codec_np);

	/*get dai_fmt and clk_fmt from device tree*/
	of_property_read_u32(np, "amb,dai_fmt", &dai_fmt);
	of_property_read_u32(np, "amb,dummy_dai_fmt", &dummy_dai_fmt);
	of_property_read_u32(np, "amb,clk_fmt", &clk_fmt);

	rval = snd_soc_register_card(card);
	if (rval)
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n", rval);

	return rval;
}

static int amba_soc_snd_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&snd_soc_card_amba);
	if(snd_soc_card_amba.dapm_routes != NULL)
		kfree(snd_soc_card_amba.dapm_routes);

	return 0;
}

static const struct of_device_id amba_dt_ids[] = {
	{ .compatible = "ambarella,audio-board", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, amba_dt_ids);

static struct platform_driver amba_soc_snd_driver = {
	.driver = {
		.name = "snd_soc_card_amba",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
		.of_match_table = amba_dt_ids,
	},
	.probe = amba_soc_snd_probe,
	.remove = amba_soc_snd_remove,
};

module_platform_driver(amba_soc_snd_driver);

MODULE_AUTHOR("XianqingZheng<xqzheng@ambarella.com>");
MODULE_DESCRIPTION("Amabrella Board for ALSA");
MODULE_LICENSE("GPL");
MODULE_ALIAS("snd-soc-amba");

