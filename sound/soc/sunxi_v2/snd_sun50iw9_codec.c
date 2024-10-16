/*
 * sound\soc\sunxi\snd_sun50iw9_codec.c
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
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_pcm.h"
#include "snd_sunxi_common.h"
#include "snd_sun50iw9_codec.h"

#define HLOG		"CODEC"
#define DRV_NAME	"sunxi-snd-codec"

#define CLK_PLL_AUDIOX4_NUM	0
#define CLK_AUDIO_CODEC_NUM	1

static struct reg_label g_reg_labels[] = {
	REG_LABEL(SUNXI_DAC_DPC),
	REG_LABEL(SUNXI_DAC_FIFO_CTL),
	REG_LABEL(SUNXI_DAC_FIFO_STA),
	REG_LABEL(SUNXI_DAC_CNT),
	REG_LABEL(SUNXI_DAC_DG_REG),
	REG_LABEL(AC_DAC_REG),
	REG_LABEL(AC_MIXER_REG),
	REG_LABEL(AC_RAMP_REG),
	REG_LABEL_END,
};

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};
static const struct sample_rate g_sample_rate_conv[] = {
	{44100, 0},
	{48000, 0},
	{8000, 5},
	{32000, 1},
	{22050, 2},
	{24000, 2},
	{16000, 3},
	{11025, 4},
	{12000, 4},
	{192000, 6},
	{96000, 7},
};

static void sunxi_internal_codec_init(struct sunxi_codec *codec);
static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_clk *clk);
static void snd_sunxi_clk_exit(struct platform_device *pdev, struct sunxi_clk *clk);
static int snd_sunxi_clk_enable(struct platform_device *pdev, struct sunxi_clk *clk);
static void snd_sunxi_clk_disable(struct platform_device *pdev, struct sunxi_clk *clk);
static int snd_sunxi_regulator_init(struct platform_device *pdev, struct sunxi_regulator *rglt);
static void snd_sunxi_regulator_exit(struct platform_device *pdev, struct sunxi_regulator *rglt);
static int snd_sunxi_regulator_enable(struct platform_device *pdev, struct sunxi_regulator *rglt);
static void snd_sunxi_regulator_disable(struct platform_device *pdev, struct sunxi_regulator *rglt);

static int sunxi_internal_codec_dai_hw_params(struct snd_pcm_substream *substream,
					      struct snd_pcm_hw_params *params,
					      struct snd_soc_dai *dai)
{
	int i;
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		goto playback;
	else
		goto capture;

playback:
	/* set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x3 << DAC_FIFO_MODE, 0x3 << DAC_FIFO_MODE);
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x1 << TX_SAMPLE_BITS,
				   0x0 << TX_SAMPLE_BITS);
	break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x3 << DAC_FIFO_MODE, 0x0 << DAC_FIFO_MODE);
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x1 << TX_SAMPLE_BITS, 0x1 << TX_SAMPLE_BITS);
	break;
	default:
		SND_LOG_ERR(HLOG, "unsupport format\n");
		break;
	}

	/* set rate */
	i = 0;
	for (i = 0; i < ARRAY_SIZE(g_sample_rate_conv); i++) {
		if (g_sample_rate_conv[i].samplerate == params_rate(params))
			break;
	}
	regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL, 0x7 << DAC_FS,
			   g_sample_rate_conv[i].rate_bit << DAC_FS);

	/* set channels */
	switch (params_channels(params)) {
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x1 << DAC_MONO_EN, 0x1 << DAC_MONO_EN);
	break;
	case 2:
		regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
				   0x1 << DAC_MONO_EN, 0x0 << DAC_MONO_EN);
	break;
	default:
		SND_LOG_ERR(HLOG, "unsupport channel\n");
		return -EINVAL;
	}

	return 0;

capture:
	SND_LOG_WARN(HLOG, "unsupport capture\n");

	return 0;
}

static int sunxi_internal_codec_dai_set_pll(struct snd_soc_dai *dai,
					    int pll_id, int source,
					    unsigned int freq_in,
					    unsigned int freq_out)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_clk *clk = &codec->clk;

	SND_LOG_DEBUG(HLOG, "stream -> %s, freq_in ->%u, freq_out ->%u\n",
		      pll_id ? "IN" : "OUT", freq_in, freq_out);

	if (pll_id == SNDRV_PCM_STREAM_PLAYBACK)
		goto playback;
	else
		goto capture;

playback:
	if (clk_set_rate(clk->clk_pll_audio, freq_in)) {
		SND_LOG_ERR(HLOG, "clk pllaudio set rate failed\n");
		return -EINVAL;
	}

	/* moduleclk freq = 49.152/45.1584M, audio clk source = 98.304/90.3168M, own sun50iw9 */
	if (clk_set_rate(clk->clk_audio, freq_out / 2)) {
		SND_LOG_ERR(HLOG, "clk audio set rate failed\n");
		return -EINVAL;
	}

	return 0;

capture:
	SND_LOG_WARN(HLOG, "unsupport capture\n");

	return 0;
}


static int sunxi_internal_codec_dai_prepare(struct snd_pcm_substream *substream,
					    struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		goto playback;
	else
		goto capture;

playback:
	regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
			   0x1 << DAC_FIFO_FLUSH, 0x1 << DAC_FIFO_FLUSH);
	regmap_write(regmap, SUNXI_DAC_FIFO_STA,
		     1 << DAC_TXE_INT | 1 << DAC_TXU_INT | 1 << DAC_TXO_INT);
	regmap_write(regmap, SUNXI_DAC_CNT, 0);

	return 0;

capture:
	SND_LOG_WARN(HLOG, "unsupport capture\n");

	return 0;
}

static int sunxi_internal_codec_dai_trigger(struct snd_pcm_substream *substream,
					    int cmd,
					    struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "cmd -> %d\n", cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   1 << DAC_DRQ_EN, 1 << DAC_DRQ_EN);
		} else {
			SND_LOG_WARN(HLOG, "unsupport capture\n");
		}
	break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFO_CTL,
					   1 << DAC_DRQ_EN, 0 << DAC_DRQ_EN);
		} else {
			SND_LOG_WARN(HLOG, "unsupport capture\n");
		}
	break;
	default:
		SND_LOG_ERR(HLOG, "unsupport cmd\n");
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_internal_codec_dai_ops = {
	.hw_params	= sunxi_internal_codec_dai_hw_params,
	.set_pll	= sunxi_internal_codec_dai_set_pll,
	.prepare	= sunxi_internal_codec_dai_prepare,
	.trigger	= sunxi_internal_codec_dai_trigger,
};

static struct snd_soc_dai_driver sunxi_internal_codec_dai = {
	.name	= DRV_NAME,
	.playback = {
		.stream_name	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_internal_codec_dai_ops,
};

/*******************************************************************************
 * *** sound card & component function source ***
 * @0 sound card probe
 * @1 component function kcontrol register
 ******************************************************************************/
/* components function kcontrol setting */
static const char *sunxi_switch_text[] = {"Off", "On"};
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);

static int sunxi_get_tx_hub_mode(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAC_DPC, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << DAC_HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_set_tx_hub_mode(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, AC_RAMP_REG, 0x1 << RDEN, 0x0 << RDEN);
		regmap_update_bits(regmap, AC_DAC_REG, 0x1 << LINEOUTL_EN, 0x0 << LINEOUTL_EN);
		regmap_update_bits(regmap, AC_DAC_REG, 0x1 << DACLEN, 0x0 << DACLEN);
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DA, 0x0 << EN_DA);

		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << DAC_HUB_EN, 0x0 << DAC_HUB_EN);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << DAC_HUB_EN, 0x1 << DAC_HUB_EN);
		/* set tx route */
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DA, 0x1 << EN_DA);
		regmap_update_bits(regmap, AC_DAC_REG, 0x1 << DACLEN, 0x1 << DACLEN);
		regmap_update_bits(regmap, AC_DAC_REG, 0x1 << LINEOUTL_EN, 0x1 << LINEOUTL_EN);
		regmap_update_bits(regmap, AC_RAMP_REG, 0x1 << RDEN, 0x1 << RDEN);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct snd_kcontrol_new sunxi_tx_hub_controls[] = {
	SOC_ENUM_EXT("tx hub mode", sunxi_tx_hub_mode_enum,
		     sunxi_get_tx_hub_mode, sunxi_set_tx_hub_mode),
};

static int sunxi_codec_playback_event(struct snd_soc_dapm_widget *w,
				      struct snd_kcontrol *k, int event)
{
	unsigned int reg_val;
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DA, 0x1 << EN_DA);
		break;

	case SND_SOC_DAPM_POST_PMD:
		regmap_read(regmap, AC_DAC_REG, &reg_val);
		reg_val &= ((0x1 << DACLEN) | (0x1 << DACREN));
		if (!reg_val)
			regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DA, 0x0 << EN_DA);
		break;

	default:
		break;
	}
	return 0;
}

static int sunxi_lineout_event(struct snd_soc_dapm_widget *w,
			       struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component =
				snd_soc_dapm_to_component(w->dapm);
	struct sunxi_codec *codec =
				snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	struct pa_config *pa_cfg = codec->pa_cfg;

	SND_LOG_DEBUG(HLOG, "\n");

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, AC_DAC_REG,
				   (0x1 << LINEOUTL_EN) | (0x1 << LINEOUTR_EN),
				   (0x1 << LINEOUTL_EN) | (0x1 << LINEOUTR_EN));
		regmap_update_bits(regmap, AC_RAMP_REG,
				    0x1 << RDEN, 0x1 << RDEN);

		snd_sunxi_pa_pin_enable(pa_cfg, codec->pa_pin_max);

		break;

	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, AC_RAMP_REG,
				    0x1 << RDEN, 0x0 << RDEN);
		regmap_update_bits(regmap, AC_DAC_REG,
				   (0x1 << LINEOUTL_EN) | (0x1 << LINEOUTR_EN),
				   (0x0 << LINEOUTL_EN) | (0x0 << LINEOUTR_EN));

		snd_sunxi_pa_pin_disable(pa_cfg, codec->pa_pin_max);

		break;

	default:
		break;
	}

	return 0;
}

static const DECLARE_TLV_DB_SCALE(digital_tlv, 0, -116, -7424);
static const unsigned int lineout_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 0, TLV_DB_SCALE_ITEM(0, 0, 1),
	1, 31, TLV_DB_SCALE_ITEM(-4350, 150, 1),
};

struct snd_kcontrol_new sunxi_codec_controls[] = {
	/* SOC_SINGLE("hub mode switch", SUNXI_DAC_DPC, DAC_HUB_EN, 0x1, 0), */
	SOC_SINGLE_TLV("digital volume", SUNXI_DAC_DPC, DVOL, 0x3F, 1, digital_tlv),
	SOC_SINGLE_TLV("lineout volume", AC_DAC_REG, LINEOUT_VOL, 0x1F, 0, lineout_tlv),
};

static const struct snd_kcontrol_new left_output_mixer[] = {
	SOC_DAPM_SINGLE("DACL Switch", AC_MIXER_REG, LMIX_LDAC, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", AC_MIXER_REG, LMIX_RDAC, 1, 0),
};

static const struct snd_kcontrol_new right_output_mixer[] = {
	SOC_DAPM_SINGLE("DACL Switch", AC_MIXER_REG, RMIX_LDAC, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", AC_MIXER_REG, RMIX_RDAC, 1, 0),
};

static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, AC_DAC_REG, DACLEN, 0,
			      sunxi_codec_playback_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("DACR", "Playback", 0, AC_DAC_REG, DACREN, 0,
			      sunxi_codec_playback_event,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MIXER("OutputL Mixer", AC_MIXER_REG, LMIXEN, 0,
			   left_output_mixer, ARRAY_SIZE(left_output_mixer)),
	SND_SOC_DAPM_MIXER("OutputR Mixer", AC_MIXER_REG, RMIXEN, 0,
			   right_output_mixer, ARRAY_SIZE(right_output_mixer)),

	SND_SOC_DAPM_OUTPUT("LINEOUTL"),
	SND_SOC_DAPM_OUTPUT("LINEOUTR"),

	SND_SOC_DAPM_LINE("LINEOUT", sunxi_lineout_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	{"OutputL Mixer", "DACR Switch", "DACR"},
	{"OutputL Mixer", "DACL Switch", "DACL"},

	{"OutputR Mixer", "DACL Switch", "DACL"},
	{"OutputR Mixer", "DACR Switch", "DACR"},

	{"LINEOUTL", NULL, "OutputL Mixer"},
	{"LINEOUTR", NULL, "OutputR Mixer"},
};

static void sunxi_internal_codec_init(struct sunxi_codec *codec)
{
	struct sunxi_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	/* Disable DRC function for playback */
	regmap_write(regmap, SUNXI_DAC_DAP_CTL, 0);

	/* set digital vol */
	regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x3f << DVOL, 0 << DVOL);

	/* set lineout vol */
	regmap_update_bits(regmap, AC_DAC_REG,
			   0x1f << LINEOUT_VOL,
			   dts->lineout_vol << LINEOUT_VOL);

	regmap_update_bits(regmap, AC_DAC_REG, 0x1 << LMUTE, 0x1 << LMUTE);
	regmap_update_bits(regmap, AC_DAC_REG, 0x1 << RMUTE, 0x1 << RMUTE);

	/* enable rampen to avoid pop */
	regmap_update_bits(regmap, AC_RAMP_REG, 0x7 << RAMP_STEP, 0x1 << RAMP_STEP);
	regmap_update_bits(regmap, AC_DAC_REG, 0x1 << RSWITCH, 0x0 << RSWITCH);
	regmap_update_bits(regmap, AC_DAC_REG, 0x1 << RAMPEN, 0x1 << RAMPEN);
}

static int sunxi_internal_codec_probe(struct snd_soc_codec *snd_codec)
{
	int ret;
	struct snd_soc_dapm_context *dapm = &snd_codec->component.dapm;
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct sunxi_dts *dts = &codec->dts;

	SND_LOG_DEBUG(HLOG, "\n");

	/* component kcontrols -> tx_hub */
	if (dts->tx_hub_en) {
		ret = snd_soc_add_codec_controls(snd_codec, sunxi_tx_hub_controls,
						 ARRAY_SIZE(sunxi_tx_hub_controls));
		if (ret)
			SND_LOG_ERR(HLOG, "add tx_hub kcontrols failed\n");
	}

	ret = snd_soc_add_codec_controls(snd_codec, sunxi_codec_controls,
					 ARRAY_SIZE(sunxi_codec_controls));
	if (ret)
		SND_LOG_ERR(HLOG, "register codec kcontrols failed\n");

	ret = snd_soc_dapm_new_controls(dapm, sunxi_codec_dapm_widgets,
					ARRAY_SIZE(sunxi_codec_dapm_widgets));
	if (ret)
		SND_LOG_ERR(HLOG, "register codec dapm_widgets failed\n");

	ret = snd_soc_dapm_add_routes(dapm, sunxi_codec_dapm_routes,
				      ARRAY_SIZE(sunxi_codec_dapm_routes));
	if (ret)
		SND_LOG_ERR(HLOG, "register codec dapm_routes failed\n");

	sunxi_internal_codec_init(codec);

	return 0;
}

static int sunxi_internal_codec_suspend(struct snd_soc_codec *snd_codec)
{
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_save_reg(regmap, g_reg_labels);
	snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);
	snd_sunxi_regulator_disable(codec->pdev, &codec->rglt);
	snd_sunxi_clk_disable(codec->pdev, &codec->clk);

	return 0;
}

static int sunxi_internal_codec_resume(struct snd_soc_codec *snd_codec)
{
	int ret;
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_pa_pin_disable(codec->pa_cfg, codec->pa_pin_max);
	ret = snd_sunxi_clk_enable(codec->pdev, &codec->clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk enable failed\n");
		return ret;
	}
	ret = snd_sunxi_regulator_enable(codec->pdev, &codec->rglt);
	if (ret) {
		SND_LOG_ERR(HLOG, "regulator enable failed\n");
		return ret;
	}
	sunxi_internal_codec_init(codec);
	snd_sunxi_echo_reg(regmap, g_reg_labels);

	return 0;
}

static struct snd_soc_codec_driver sunxi_internal_codec_dev = {
	.probe		= sunxi_internal_codec_probe,
	.suspend	= sunxi_internal_codec_suspend,
	.resume		= sunxi_internal_codec_resume,
	.ignore_pmdown_time = 1,	/* Doesn't benefit from pmdown delay */
};

/*******************************************************************************
 * *** kernel source ***
 * @1 regmap
 * @2 clk
 * @3 regulator
 * @4 pa pin
 * @5 dts params
 * @6 sysfs debug
 ******************************************************************************/
static struct resource g_res;
static struct regmap_config g_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_AUDIO_MAX_REG,
	.cache_type = REGCACHE_NONE,
};

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_clk *clk)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	/* get parent clk */
	clk->clk_pll_audio = of_clk_get(np, CLK_PLL_AUDIOX4_NUM);
	if (IS_ERR_OR_NULL(clk->clk_pll_audio)) {
		SND_LOG_ERR(HLOG, "clk pll get failed\n");
		ret = PTR_ERR(clk->clk_pll_audio);
		goto err_get_clk_pll_audio;
	}

	/* get module clk */
	clk->clk_audio = of_clk_get(np, CLK_AUDIO_CODEC_NUM);
	if (IS_ERR_OR_NULL(clk->clk_audio)) {
		SND_LOG_ERR(HLOG, "clk audio get failed\n");
		ret = PTR_ERR(clk->clk_audio);
		goto err_get_clk_audio;
	}

	/* set clk audio parent of pllaudio */
	if (clk_set_parent(clk->clk_audio, clk->clk_pll_audio)) {
		SND_LOG_ERR(HLOG, "set parent clk audio failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}

	ret = snd_sunxi_clk_enable(pdev, clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk enable failed\n");
		ret = -EINVAL;
		goto err_clk_enable;
	}

	return 0;

err_clk_enable:
err_set_parent:
	clk_put(clk->clk_audio);
err_get_clk_audio:
	clk_put(clk->clk_pll_audio);
err_get_clk_pll_audio:
	return ret;
}

static void snd_sunxi_clk_exit(struct platform_device *pdev, struct sunxi_clk *clk)
{
	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_clk_disable(pdev, clk);
	clk_put(clk->clk_audio);
	clk_put(clk->clk_pll_audio);
}

static int snd_sunxi_clk_enable(struct platform_device *pdev, struct sunxi_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	if (clk_prepare_enable(clk->clk_pll_audio)) {
		SND_LOG_ERR(HLOG, "pllaudio enable failed\n");
		goto err_enable_clk_pll_audio;
	}

	if (clk_prepare_enable(clk->clk_audio)) {
		SND_LOG_ERR(HLOG, "dacclk enable failed\n");
		goto err_enable_clk_audio;
	}

	return 0;

err_enable_clk_audio:
	clk_disable_unprepare(clk->clk_pll_audio);
err_enable_clk_pll_audio:
	return ret;
}

static void snd_sunxi_clk_disable(struct platform_device *pdev, struct sunxi_clk *clk)
{
	SND_LOG_DEBUG(HLOG, "\n");

	clk_disable_unprepare(clk->clk_audio);
	clk_disable_unprepare(clk->clk_pll_audio);
}

static int snd_sunxi_regulator_init(struct platform_device *pdev, struct sunxi_regulator *rglt)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	rglt->avcc_external = of_property_read_bool(np, "avcc-external");
	ret = of_property_read_u32(np, "avcc-vol", &temp_val);
	if (ret < 0) {
		/* default avcc voltage: 1.8v */
		rglt->avcc_vol = 1800000;
	} else {
		rglt->avcc_vol = temp_val;
	}

	if (rglt->avcc_external) {
		SND_LOG_DEBUG(HLOG, "use external avcc\n");
		rglt->avcc = regulator_get(&pdev->dev, "avcc");
		if (IS_ERR_OR_NULL(rglt->avcc)) {
			SND_LOG_WARN(HLOG, "get avcc failed, unused external pmu\n");
			return 0;
		}
	} else {
		SND_LOG_DEBUG(HLOG, "use internal avcc\n");
		return 0;
	}

	ret = regulator_set_voltage(rglt->avcc, rglt->avcc_vol, rglt->avcc_vol);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "set avcc voltage failed\n");
		ret = -EFAULT;
		goto err_regulator_set_vol_avcc;
	}
	ret = regulator_enable(rglt->avcc);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "enable avcc failed\n");
		ret = -EFAULT;
		goto err_regulator_enable_avcc;
	}

	return 0;

err_regulator_enable_avcc:
err_regulator_set_vol_avcc:
	if (rglt->avcc)
		regulator_put(rglt->avcc);
	return ret;
}

static void snd_sunxi_regulator_exit(struct platform_device *pdev, struct sunxi_regulator *rglt)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc)) {
			regulator_disable(rglt->avcc);
			regulator_put(rglt->avcc);
		}
}

static int snd_sunxi_regulator_enable(struct platform_device *pdev, struct sunxi_regulator *rglt)
{
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc)) {
			ret = regulator_enable(rglt->avcc);
			if (ret) {
				SND_LOG_ERR(HLOG, "enable avcc failed\n");
				return -1;
			}
		}

	return 0;
}

static void snd_sunxi_regulator_disable(struct platform_device *pdev, struct sunxi_regulator *rglt)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt->avcc)
		if (!IS_ERR_OR_NULL(rglt->avcc)) {
			regulator_disable(rglt->avcc);
		}
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_dts *dts)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	/* lineout volume */
	ret = of_property_read_u32(np, "lineout-vol", &temp_val);
	if (ret < 0) {
		dts->lineout_vol = 0;
	} else {
		dts->lineout_vol = temp_val;
	}

	/* tx_hub */
	dts->tx_hub_en = of_property_read_bool(np, "tx-hub-en");

	SND_LOG_DEBUG(HLOG, "lineout vol -> %u\n", dts->lineout_vol);
}

/* sysfs debug */
static ssize_t snd_sunxi_debug_show_reg(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	size_t count = 0;

	count += sprintf(buf + count, "usage->read : echo [num] > audio_reg\n");
	count += sprintf(buf + count, "usage->write: echo [reg] [value] > audio_reg\n");

	count += sprintf(buf + count, "num: 0.all\n");

	return count;
}

static ssize_t snd_sunxi_debug_store_reg(struct device *dev, struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct sunxi_codec *codec = dev_get_drvdata(dev);
	struct regmap *regmap = codec->mem.regmap;
	int scanf_cnt;
	unsigned int num = 0, i = 0;
	unsigned int output_reg_val;
	unsigned int input_reg_val;
	unsigned int input_reg_offset;
	unsigned int size = ARRAY_SIZE(g_reg_labels);

	if (buf[1] == 'x')
		scanf_cnt = sscanf(buf, "0x%x 0x%x", &input_reg_offset, &input_reg_val);
	else
		scanf_cnt = sscanf(buf, "%x", &num);

	if (scanf_cnt <= 0 || num != 0) {
		pr_err("please get the usage by\"cat audio_reg\"\n");
		return count;
	}

	if (scanf_cnt == 1) {
		while ((i < size) && (g_reg_labels[i].name != NULL)) {
			regmap_read(regmap, g_reg_labels[i].address, &output_reg_val);
			pr_info("%-32s [0x%03x]: 0x%8x :0x%x\n",
				g_reg_labels[i].name,
				g_reg_labels[i].address, output_reg_val,
				g_reg_labels[i].value);
			i++;
		}
		return count;
	} else if (scanf_cnt == 2) {
		if (input_reg_offset > SUNXI_AUDIO_MAX_REG) {
			pr_err("reg offset > audio max reg[0x%x]\n", SUNXI_AUDIO_MAX_REG);
			return count;
		}

		regmap_read(regmap, input_reg_offset, &output_reg_val);
		pr_info("reg[0x%03x]: 0x%x (old)\n", input_reg_offset, output_reg_val);
		regmap_write(regmap, input_reg_offset, input_reg_val);
		regmap_read(regmap, input_reg_offset, &output_reg_val);
		pr_info("reg[0x%03x]: 0x%x (new)\n", input_reg_offset, output_reg_val);
	}

	return count;
}

static DEVICE_ATTR(audio_reg, 0644, snd_sunxi_debug_show_reg, snd_sunxi_debug_store_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_audio_reg.attr,
	NULL,
};

static struct attribute_group debug_attr = {
	.name	= "audio_debug",
	.attrs	= audio_debug_attrs,
};

static int sunxi_internal_codec_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_codec *codec;
	struct sunxi_mem *mem;
	struct sunxi_clk *clk;
	struct sunxi_regulator *rglt;
	struct sunxi_dts *dts;

	SND_LOG_DEBUG(HLOG, "\n");

	/* sunxi codec info */
	codec = devm_kzalloc(dev, sizeof(struct sunxi_codec), GFP_KERNEL);
	if (!codec) {
		SND_LOG_ERR(HLOG, "can't allocate sunxi codec memory\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, codec);
	mem = &codec->mem;
	clk = &codec->clk;
	rglt = &codec->rglt;
	dts = &codec->dts;

	/* memio init */
	mem->dev_name = DRV_NAME;
	mem->res = &g_res;
	mem->regmap_config = &g_regmap_config;
	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR(HLOG, "mem init failed\n");
		ret = -ENOMEM;
		goto err_mem_init;
	}

	/* clk init */
	ret = snd_sunxi_clk_init(pdev, clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk init failed\n");
		ret = -ENOMEM;
		goto err_clk_init;
	}

	/* regulator init */
	ret = snd_sunxi_regulator_init(pdev, rglt);
	if (ret) {
		SND_LOG_ERR(HLOG, "regulator init failed\n");
		ret = -ENOMEM;
		goto err_regulator_init;
	}

	/* dts_params init */
	snd_sunxi_dts_params_init(pdev, dts);

	/* pa_pin init */
	codec->pa_cfg = snd_sunxi_pa_pin_init(pdev, &codec->pa_pin_max);

	/* alsa component register */
	ret = snd_soc_register_codec(dev, &sunxi_internal_codec_dev, &sunxi_internal_codec_dai, 1);
	if (ret) {
		SND_LOG_ERR(HLOG, "internal-codec component register failed\n");
		ret = -ENOMEM;
		goto err_register_component;
	}

	ret = snd_sunxi_sysfs_create_group(pdev, &debug_attr);
	if (ret)
		SND_LOG_WARN(HLOG, "sysfs debug create failed\n");

	SND_LOG_DEBUG(HLOG, "register internal-codec codec success\n");

	return 0;

err_register_component:
	snd_sunxi_regulator_exit(pdev, rglt);
err_regulator_init:
	snd_sunxi_clk_exit(pdev, clk);
err_clk_init:
	snd_sunxi_mem_exit(pdev, mem);
err_mem_init:
	devm_kfree(dev, codec);
err_devm_kzalloc:
	of_node_put(np);

	return ret;
}

static int sunxi_internal_codec_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_codec *codec = dev_get_drvdata(dev);
	struct sunxi_mem *mem = &codec->mem;
	struct sunxi_clk *clk = &codec->clk;
	struct sunxi_regulator *rglt = &codec->rglt;

	SND_LOG_DEBUG(HLOG, "\n");

	/* alsa component unregister */
	snd_sunxi_sysfs_remove_group(pdev, &debug_attr);
	snd_soc_unregister_codec(dev);

	snd_sunxi_mem_exit(pdev, mem);
	snd_sunxi_clk_exit(pdev, clk);
	snd_sunxi_regulator_exit(pdev, rglt);
	snd_sunxi_pa_pin_exit(pdev, codec->pa_cfg, codec->pa_pin_max);

	/* sunxi codec custom info free */
	devm_kfree(dev, codec);
	of_node_put(pdev->dev.of_node);

	SND_LOG_DEBUG(HLOG, "unregister internal-codec codec success\n");

	return 0;
}

static const struct of_device_id sunxi_internal_codec_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_internal_codec_of_match);

static struct platform_driver sunxi_internal_codec_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_internal_codec_of_match,
	},
	.probe	= sunxi_internal_codec_dev_probe,
	.remove	= sunxi_internal_codec_dev_remove,
};

int __init sunxi_internal_codec_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_internal_codec_driver);
	if (ret != 0) {
		SND_LOG_ERR(HLOG, "platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_internal_codec_dev_exit(void)
{
	platform_driver_unregister(&sunxi_internal_codec_driver);
}

late_initcall(sunxi_internal_codec_dev_init);
module_exit(sunxi_internal_codec_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard codec of internal-codec");
