/*
 * sound\soc\sunxi\snd_sunxi_dmic.c
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
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/dma/sunxi-dma.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_pcm.h"
#include "snd_sunxi_rxsync.h"
#include "snd_sunxi_dmic.h"

#define HLOG		"DMIC"
#define DRV_NAME	"sunxi-snd-plat-dmic"

/* for sample rate conver */
struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

static const struct sample_rate sample_rate_conv[] = {
	{44100, 0x0},
	{48000, 0x0},
	{22050, 0x2},
	/* KNOT support */
	{24000, 0x2},
	{11025, 0x4},
	{12000, 0x4},
	{32000, 0x1},
	{16000, 0x3},
	{8000,  0x5},
};

/* for reg debug */
#define REG_LABEL(constant)	{#constant, constant, 0}
#define REG_LABEL_END		{NULL, 0, 0}

struct reg_label {
	const char *name;
	const unsigned int address;
	unsigned int value;
};
static struct reg_label g_reg_labels[] = {
	REG_LABEL(SUNXI_DMIC_EN),
	REG_LABEL(SUNXI_DMIC_SR),
	REG_LABEL(SUNXI_DMIC_CTR),
	/* REG_LABEL(SUNXI_DMIC_DATA), */
	REG_LABEL(SUNXI_DMIC_INTC),
	REG_LABEL(SUNXI_DMIC_INTS),
	REG_LABEL(SUNXI_DMIC_FIFO_CTR),
	REG_LABEL(SUNXI_DMIC_FIFO_STA),
	REG_LABEL(SUNXI_DMIC_CH_NUM),
	REG_LABEL(SUNXI_DMIC_CH_MAP),
	REG_LABEL(SUNXI_DMIC_CNT),
	REG_LABEL(SUNXI_DMIC_DATA0_1_VOL),
	REG_LABEL(SUNXI_DMIC_DATA2_3_VOL),
	REG_LABEL(SUNXI_DMIC_HPF_CTRL),
	REG_LABEL(SUNXI_DMIC_HPF_COEF),
	REG_LABEL(SUNXI_DMIC_HPF_GAIN),
	REG_LABEL(SUNXI_DMIC_REV),
	REG_LABEL_END,
};

static struct regmap_config g_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_DMIC_REG_MAX,
	.cache_type = REGCACHE_NONE,
};

static int snd_sunxi_save_reg(struct regmap *regmap,
			      struct reg_label *reg_labels);
static int snd_sunxi_echo_reg(struct regmap *regmap,
			      struct reg_label *reg_labels);

static void sunxi_rx_sync_enable(void *data, bool enable);

static int sunxi_dmic_dai_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
				  unsigned int freq_in, unsigned int freq_out)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);
	struct sunxi_dmic_clk *clk = &dmic->clk;

	SND_LOG_DEBUG(HLOG, "\n");

	if (clk_set_parent(clk->clk_module, clk->pllaudio)) {
		SND_LOG_ERR(HLOG, "set parent of clk_module to pllaudio failed\n");
		return -EINVAL;
	}

	if (clk_set_rate(clk->pllaudio, freq_in)) {
		SND_LOG_ERR(HLOG, "freq : %u pll clk unsupport\n", freq_in);
		return -EINVAL;
	}

	if (clk_set_rate(clk->clk_module, freq_out)) {
		SND_LOG_ERR(HLOG, "freq : %u module clk unsupport\n", freq_out);
		return -EINVAL;
	}

	return 0;
}

static int sunxi_dmic_dai_startup(struct snd_pcm_substream *substream,
				    struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);
	struct sunxi_dmic_dts *dts = &dmic->dts;
	struct regmap *regmap = dmic->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		snd_soc_dai_set_dma_data(dai, substream, &dmic->capture_dma_param);
		if (dts->rx_sync_en && dts->rx_sync_ctl)
			sunxi_rx_sync_startup((void *)regmap, dts->rx_sync_domain, dts->rx_sync_id,
					      sunxi_rx_sync_enable);
	}

	return 0;
}

static void sunxi_dmic_dai_shutdown(struct snd_pcm_substream *substream,
				      struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);
	struct sunxi_dmic_dts *dts = &dmic->dts;

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (dts->rx_sync_en && dts->rx_sync_ctl)
			sunxi_rx_sync_shutdown(dts->rx_sync_domain, dts->rx_sync_id);
	}

	return;
}

static int sunxi_dmic_dai_hw_params(struct snd_pcm_substream *substream,
				    struct snd_pcm_hw_params *params,
				    struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = dmic->mem.regmap;
	unsigned int channels;
	unsigned int channels_en[8] = {
		0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff
	};
	int i;

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		SND_LOG_ERR(HLOG, "unsupport playback\n");
		return -EINVAL;
	}

	/* set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		regmap_update_bits(regmap, SUNXI_DMIC_FIFO_CTR,
				   0x1 << DMIC_SAMPLE_RESOLUTION,
				   0x0 << DMIC_SAMPLE_RESOLUTION);
		regmap_update_bits(regmap, SUNXI_DMIC_FIFO_CTR,
				   0x1 << DMIC_FIFO_MODE,
				   0x1 << DMIC_FIFO_MODE);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		regmap_update_bits(regmap, SUNXI_DMIC_FIFO_CTR,
				   0x1 << DMIC_SAMPLE_RESOLUTION,
				   0x1 << DMIC_SAMPLE_RESOLUTION);
		regmap_update_bits(regmap, SUNXI_DMIC_FIFO_CTR,
				   0x1 << DMIC_FIFO_MODE,
				   0x0 << DMIC_FIFO_MODE);
		break;
	default:
		SND_LOG_ERR(HLOG, "unrecognized format\n");
		return -EINVAL;
	}

	/* set rate */
	for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++) {
		if (sample_rate_conv[i].samplerate == params_rate(params)) {
			if (sample_rate_conv[i].samplerate > 48000)
				return -EINVAL;
			regmap_update_bits(regmap, SUNXI_DMIC_SR, 0x7 << DMIC_SR,
					   sample_rate_conv[i].rate_bit << DMIC_SR);
		}
	}

	/* oversamplerate adjust */
	if (params_rate(params) >= 24000)
		regmap_update_bits(regmap, SUNXI_DMIC_CTR,
				   1 << DMIC_OVERSAMPLE_RATE, 1 << DMIC_OVERSAMPLE_RATE);
	else
		regmap_update_bits(regmap, SUNXI_DMIC_CTR,
				   1 << DMIC_OVERSAMPLE_RATE, 0 << DMIC_OVERSAMPLE_RATE);

	/* set channels */
	channels = params_channels(params);
	regmap_update_bits(regmap, SUNXI_DMIC_CH_NUM, 0x7 << DMIC_CH_NUM,
			   (channels - 1) << DMIC_CH_NUM);
	regmap_update_bits(regmap, SUNXI_DMIC_EN, 0xFF << DATA_CH_EN,
			   channels_en[channels - 1] << DATA_CH_EN);

	/* enabled HPF */
	regmap_write(regmap, SUNXI_DMIC_HPF_CTRL, channels_en[channels - 1]);

	return 0;
}

static int sunxi_dmic_dai_prepare(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);
	struct regmap *regmap = dmic->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	regmap_update_bits(regmap, SUNXI_DMIC_FIFO_CTR, 1 << DMIC_FIFO_FLUSH, 1 << DMIC_FIFO_FLUSH);

	regmap_write(regmap, SUNXI_DMIC_INTS,
		     (1 << FIFO_OVERRUN_IRQ_PENDING) | (1 << FIFO_DATA_IRQ_PENDING));
	regmap_write(regmap, SUNXI_DMIC_CNT, 0x0);

	return 0;
}

static void sunxi_dmic_dai_rx_route(struct sunxi_dmic *dmic, bool enable)
{
	struct regmap *regmap = dmic->mem.regmap;

	if (enable) {
		regmap_update_bits(regmap, SUNXI_DMIC_INTC, 0x1 << FIFO_DRQ_EN, 0x1 << FIFO_DRQ_EN);
		regmap_update_bits(regmap, SUNXI_DMIC_EN, 0x1 << GLOBE_EN, 0x1 << GLOBE_EN);
	} else {
		regmap_update_bits(regmap, SUNXI_DMIC_EN, 0x1 << GLOBE_EN, 0x0 << GLOBE_EN);
		regmap_update_bits(regmap, SUNXI_DMIC_INTC, 0x1 << FIFO_DRQ_EN, 0x0 << FIFO_DRQ_EN);
	}
}

static int sunxi_dmic_dai_trigger(struct snd_pcm_substream *substream,
				  int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);
	struct sunxi_dmic_dts *dts = &dmic->dts;

	SND_LOG_DEBUG(HLOG, "\n");

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			sunxi_dmic_dai_rx_route(dmic, true);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, true);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			sunxi_dmic_dai_rx_route(dmic, false);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, false);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_dmic_dai_ops = {
	/* call by machine */
	.set_pll	= sunxi_dmic_dai_set_pll,
	/* call by asoc */
	.startup	= sunxi_dmic_dai_startup,
	.hw_params	= sunxi_dmic_dai_hw_params,
	.prepare	= sunxi_dmic_dai_prepare,
	.trigger	= sunxi_dmic_dai_trigger,
	.shutdown	= sunxi_dmic_dai_shutdown,
};

static int sunxi_dmic_init(struct sunxi_dmic *dmic)
{
	struct sunxi_dmic_dts *dts = &dmic->dts;
	struct regmap *regmap = dmic->mem.regmap;
	unsigned int rx_dtime_map;

	SND_LOG_DEBUG(HLOG, "\n");

	/* set rx channel map */
	regmap_write(regmap, SUNXI_DMIC_CH_MAP, dts->rx_chmap);

	/* set rxfifo delay time */
	switch (dts->rx_dtime) {
	case 5:
		rx_dtime_map = 0;
		break;
	case 10:
		rx_dtime_map = 1;
		break;
	case 20:
		rx_dtime_map = 2;
		break;
	case 30:
		rx_dtime_map = 3;
		break;
	case 0:
	default:
		break;
	}
	regmap_update_bits(regmap, SUNXI_DMIC_CTR, 0x3 << DMICFDT, rx_dtime_map << DMICFDT);
	if (dts->rx_dtime)
		regmap_update_bits(regmap, SUNXI_DMIC_CTR, 0x1 << DMICDFEN, 0x1 << DMICDFEN);
	else
		regmap_update_bits(regmap, SUNXI_DMIC_CTR, 0x1 << DMICDFEN, 0x0 << DMICDFEN);

	/* disable rx_sync_en default */
	regmap_update_bits(regmap, SUNXI_DMIC_EN, 0x1 << RX_SYNC_EN, 0x0 << RX_SYNC_EN);

	/* diabsle LR SWAP default */
	regmap_update_bits(regmap, SUNXI_DMIC_CTR, 1 << DATA0_LR_SWAP_EN, 0 << DATA0_LR_SWAP_EN);
	regmap_update_bits(regmap, SUNXI_DMIC_CTR, 1 << DATA1_LR_SWAP_EN, 0 << DATA1_LR_SWAP_EN);
	regmap_update_bits(regmap, SUNXI_DMIC_CTR, 1 << DATA2_LR_SWAP_EN, 0 << DATA2_LR_SWAP_EN);
	regmap_update_bits(regmap, SUNXI_DMIC_CTR, 1 << DATA3_LR_SWAP_EN, 0 << DATA3_LR_SWAP_EN);

	/* set the digital volume */
	regmap_update_bits(regmap, SUNXI_DMIC_DATA0_1_VOL,
			(0xFF << DATA0L_VOL) | (0xFF << DATA0R_VOL),
			(dts->data_vol << DATA0L_VOL) | (dts->data_vol << DATA0R_VOL));
	regmap_update_bits(regmap, SUNXI_DMIC_DATA0_1_VOL,
			(0xFF << DATA1L_VOL) | (0xFF << DATA1R_VOL),
			(dts->data_vol << DATA1L_VOL) | (dts->data_vol << DATA1R_VOL));

	regmap_update_bits(regmap, SUNXI_DMIC_DATA2_3_VOL,
			(0xFF << DATA2L_VOL) | (0xFF << DATA2R_VOL),
			(dts->data_vol << DATA2L_VOL) | (dts->data_vol << DATA2R_VOL));
	regmap_update_bits(regmap, SUNXI_DMIC_DATA2_3_VOL,
			(0xFF << DATA3L_VOL) | (0xFF << DATA3R_VOL),
			(dts->data_vol << DATA3L_VOL) | (dts->data_vol << DATA3R_VOL));

	return 0;
}

static int sunxi_dmic_dai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);

	SND_LOG_DEBUG(HLOG, "\n");

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai, NULL, &dmic->capture_dma_param);

	sunxi_dmic_init(dmic);

	return 0;
}

static int sunxi_dmic_dai_suspend(struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);
	struct sunxi_dmic_clk *clk = &dmic->clk;
	struct regmap *regmap = dmic->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	/* save reg value */
	snd_sunxi_save_reg(regmap, g_reg_labels);

	/* disable clk & regulator */
	snd_sunxi_clk_disable(clk);

	return 0;
}

static int sunxi_dmic_dai_resume(struct snd_soc_dai *dai)
{
	struct sunxi_dmic *dmic = snd_soc_dai_get_drvdata(dai);
	struct sunxi_dmic_clk *clk = &dmic->clk;
	struct regmap *regmap = dmic->mem.regmap;
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = snd_sunxi_clk_enable(clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk enable failed\n");
		return ret;
	}

	/* for dmic init */
	sunxi_dmic_init(dmic);

	/* resume reg value */
	snd_sunxi_echo_reg(regmap, g_reg_labels);

	/* for clear RX fifo */
	regmap_update_bits(regmap, SUNXI_DMIC_FIFO_CTR,
			   (1 << DMIC_FIFO_FLUSH), (1 << DMIC_FIFO_FLUSH));
	regmap_write(regmap, SUNXI_DMIC_INTS,
		     (1 << FIFO_OVERRUN_IRQ_PENDING) | (1 << FIFO_DATA_IRQ_PENDING));
	regmap_write(regmap, SUNXI_DMIC_CNT, 0x0);

	return 0;
}

static struct snd_soc_dai_driver sunxi_dmic_dai = {
	.probe		= sunxi_dmic_dai_probe,
	.suspend	= sunxi_dmic_dai_suspend,
	.resume		= sunxi_dmic_dai_resume,
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 8,
		.rates		= SNDRV_PCM_RATE_8000_48000
				| SNDRV_PCM_RATE_KNOT,
		.formats	= SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
	},
	.ops = &sunxi_dmic_dai_ops,
};

/*******************************************************************************
 * *** sound card & component function source ***
 * @0 sound card probe
 * @1 component function kcontrol register
 ******************************************************************************/
static void sunxi_rx_sync_enable(void *data, bool enable)
{
	struct regmap *regmap = data;

	SND_LOG_DEBUG(HLOG, "%s\n", enable ? "on" : "off");

	if (enable)
		regmap_update_bits(regmap, SUNXI_DMIC_EN,
			0x1 << RX_SYNC_EN_START, 0x1 << RX_SYNC_EN_START);
	else
		regmap_update_bits(regmap, SUNXI_DMIC_EN,
			0x1 << RX_SYNC_EN_START, 0x0 << RX_SYNC_EN_START);
}

static int sunxi_get_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct sunxi_dmic *dmic = snd_soc_codec_get_drvdata(codec);
	struct sunxi_dmic_dts *dts = &dmic->dts;

	ucontrol->value.integer.value[0] = dts->rx_sync_ctl;

	return 0;
}

static int sunxi_set_rx_sync_mode(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct sunxi_dmic *dmic = snd_soc_codec_get_drvdata(codec);
	struct sunxi_dmic_dts *dts = &dmic->dts;
	struct regmap *regmap = dmic->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		dts->rx_sync_ctl = 0;
		regmap_update_bits(regmap, SUNXI_DMIC_EN,
				   0x1 << RX_SYNC_EN, 0x0 << RX_SYNC_EN);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DMIC_EN,
				   0x1 << RX_SYNC_EN, 0x1 << RX_SYNC_EN);
		dts->rx_sync_ctl = 1;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const char *sunxi_switch_text[] = {"Off", "On"};

static SOC_ENUM_SINGLE_EXT_DECL(sunxi_rx_sync_mode_enum, sunxi_switch_text);
static const struct snd_kcontrol_new sunxi_rx_sync_controls[] = {
	SOC_ENUM_EXT("rx sync mode", sunxi_rx_sync_mode_enum,
		     sunxi_get_rx_sync_mode, sunxi_set_rx_sync_mode),
};

static const DECLARE_TLV_DB_SCALE(digital_tlv, -12000, 75, 7125);
static const struct snd_kcontrol_new sunxi_dmic_controls[] = {
	/* Digital Volume */
	SOC_SINGLE_TLV("L0 volume", SUNXI_DMIC_DATA0_1_VOL, DATA0L_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("R0 volume", SUNXI_DMIC_DATA0_1_VOL, DATA0R_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("L1 volume", SUNXI_DMIC_DATA0_1_VOL, DATA1L_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("R1 volume", SUNXI_DMIC_DATA0_1_VOL, DATA1R_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("L2 volume", SUNXI_DMIC_DATA2_3_VOL, DATA2L_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("R2 volume", SUNXI_DMIC_DATA2_3_VOL, DATA2R_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("L3 volume", SUNXI_DMIC_DATA2_3_VOL, DATA3L_VOL, 0xFF, 0, digital_tlv),
	SOC_SINGLE_TLV("R3 volume", SUNXI_DMIC_DATA2_3_VOL, DATA3R_VOL, 0xFF, 0, digital_tlv),
};

static int sunxi_dmic_probe(struct snd_soc_component *component)
{
	struct sunxi_dmic *dmic = snd_soc_component_get_drvdata(component);
	struct sunxi_dmic_dts *dts = &dmic->dts;
	int ret;

	SND_LOG_DEBUG(HLOG, "\n");

	/* component kcontrols -> rx_sync */
	if (dts->rx_sync_en) {
		ret = snd_soc_add_component_controls(component, sunxi_rx_sync_controls,
						     ARRAY_SIZE(sunxi_rx_sync_controls));
		if (ret)
			SND_LOG_ERR(HLOG, "add rx_sync kcontrols failed\n");
	}

	return 0;
}

static struct snd_soc_component_driver sunxi_dmic_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_dmic_probe,
	.controls	= sunxi_dmic_controls,
	.num_controls	= ARRAY_SIZE(sunxi_dmic_controls),
};

/*******************************************************************************
 * *** kernel source ***
 * @0 reg debug
 * @1 regmap
 * @2 clk
 * @3 dts params
 ******************************************************************************/
static int snd_sunxi_save_reg(struct regmap *regmap,
			      struct reg_label *reg_labels)
{
	int i = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	while (reg_labels[i].name != NULL) {
		regmap_read(regmap,
			    reg_labels[i].address, &(reg_labels[i].value));
		i++;
	}

	return i;
}

static int snd_sunxi_echo_reg(struct regmap *regmap,
			      struct reg_label *reg_labels)
{
	int i = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	while (reg_labels[i].name != NULL) {
		regmap_write(regmap,
			     reg_labels[i].address, reg_labels[i].value);
		i++;
	}

	return i;
}

static int snd_sunxi_mem_init(struct platform_device *pdev, struct sunxi_dmic_mem *mem)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = of_address_to_resource(np, 0, &mem->res);
	if (ret) {
		SND_LOG_ERR(HLOG, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem->memregion = devm_request_mem_region(&pdev->dev, mem->res.start,
						 resource_size(&mem->res), DRV_NAME);
	if (IS_ERR_OR_NULL(mem->memregion)) {
		SND_LOG_ERR(HLOG, "memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem->membase = devm_ioremap(&pdev->dev, mem->memregion->start,
				    resource_size(mem->memregion));
	if (IS_ERR_OR_NULL(mem->membase)) {
		SND_LOG_ERR(HLOG, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem->regmap = devm_regmap_init_mmio(&pdev->dev, mem->membase, &g_regmap_config);
	if (IS_ERR_OR_NULL(mem->regmap)) {
		SND_LOG_ERR(HLOG, "regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
}

static void snd_sunxi_mem_exit(struct platform_device *pdev,
			       struct sunxi_dmic_mem *mem)
{
	SND_LOG_DEBUG(HLOG, "\n");

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));
}

static void snd_sunxi_dts_params_init(struct platform_device *pdev, struct sunxi_dmic_dts *dts)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	/* get dma params */
	ret = of_property_read_u32(np, "capture-cma", &temp_val);
	if (ret != 0) {
		dts->capture_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
		SND_LOG_WARN(HLOG, "capture-cma missing, using default value\n");
	} else {
		if (temp_val		> SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val	< SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val	= SUNXI_AUDIO_CMA_MIN_KBYTES;

		dts->capture_cma = temp_val;
	}
	ret = of_property_read_u32(np, "rx-fifo-size", &temp_val);
	if (ret != 0) {
		dts->capture_fifo_size = SUNXI_AUDIO_FIFO_SIZE;
		SND_LOG_WARN(HLOG, "rx-fifo-size miss,using default value\n");
	} else {
		dts->capture_fifo_size = temp_val;
	}

	ret = of_property_read_u32(np, "rx-chmap", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "rx-chmap config missing\n");
		dts->rx_chmap = 0x76543210;
	} else {
		dts->rx_chmap = temp_val;
	}
	ret = of_property_read_u32(np, "data-vol", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "data-vol config missing\n");
		dts->data_vol = 0xA0;
	} else {
		if (temp_val > 0xFF)
			temp_val = 0XFF;
		dts->data_vol = temp_val;
	}
	ret = of_property_read_u32(np, "rxdelaytime", &temp_val);
	if (ret < 0) {
		SND_LOG_WARN(HLOG, "rxdelaytime config missing\n");
		dts->rx_dtime = 0;
	} else {
		switch (temp_val) {
		case 0:
		case 5:
		case 10:
		case 20:
		case 30:
			dts->rx_dtime = temp_val;
			break;
		default:
			SND_LOG_WARN(HLOG, "rx delay time supoort only 0,5,10,20,30ms\n");
			dts->rx_dtime = 0;
			break;
		}
	}

	SND_LOG_DEBUG(HLOG, "capture-cma  : %u\n", dts->capture_cma);
	SND_LOG_DEBUG(HLOG, "rx-fifo-size : %u\n", dts->capture_fifo_size);

	/* components func -> rx_sync */
	dts->rx_sync_en = of_property_read_bool(np, "rx-sync-en");
	if (dts->rx_sync_en) {
		dts->rx_sync_ctl = false;
		dts->rx_sync_domain = RX_SYNC_SYS_DOMAIN;
		dts->rx_sync_id = sunxi_rx_sync_probe(dts->rx_sync_domain);
		if (dts->rx_sync_id < 0) {
			SND_LOG_ERR(HLOG, "sunxi_rx_sync_probe failed\n");
		} else {
			SND_LOG_DEBUG(HLOG, "sunxi_rx_sync_probe successful. domain=%d, id=%d\n",
				     dts->rx_sync_domain, dts->rx_sync_id);
		}
	}
}

static int snd_sunxi_pin_init(struct platform_device *pdev, struct sunxi_dmic_pinctl *pin)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	if (of_property_read_bool(np, "pinctrl-used")) {
		pin->pinctrl_used = 1;
	} else {
		pin->pinctrl_used = 0;
		SND_LOG_DEBUG(HLOG, "unused pinctrl\n");
		return 0;
	}

	pin->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(pin->pinctrl)) {
		SND_LOG_ERR(HLOG, "pinctrl get failed\n");
		ret = -EINVAL;
		return ret;
	}
	pin->pinstate = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pin->pinstate)) {
		SND_LOG_ERR(HLOG, "pinctrl default state get fail\n");
		ret = -EINVAL;
		goto err_loopup_pinstate;
	}
	pin->pinstate_sleep = pinctrl_lookup_state(pin->pinctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pin->pinstate_sleep)) {
		SND_LOG_ERR(HLOG, "pinctrl sleep state get failed\n");
		ret = -EINVAL;
		goto err_loopup_pin_sleep;
	}
	ret = pinctrl_select_state(pin->pinctrl, pin->pinstate);
	if (ret < 0) {
		SND_LOG_ERR(HLOG, "dmic set pinctrl default state fail\n");
		ret = -EBUSY;
		goto err_pinctrl_select_default;
	}

	return 0;

err_pinctrl_select_default:
err_loopup_pin_sleep:
err_loopup_pinstate:
	devm_pinctrl_put(pin->pinctrl);
	return ret;
}
static void snd_sunxi_dma_params_init(struct sunxi_dmic *dmic)
{
	struct resource *res = &dmic->mem.res;
	struct sunxi_dmic_dts *dts = &dmic->dts;

	SND_LOG_DEBUG(HLOG, "\n");

	dmic->capture_dma_param.dma_addr = res->start + SUNXI_DMIC_DATA;
	dmic->capture_dma_param.dma_drq_type_num = DRQSRC_DMIC;
	dmic->capture_dma_param.src_maxburst = 8;
	dmic->capture_dma_param.dst_maxburst = 8;
	dmic->capture_dma_param.cma_kbytes = dts->capture_cma;
	dmic->capture_dma_param.fifo_size = dts->capture_fifo_size;
};

static int sunxi_dmic_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_dmic *dmic;
	struct sunxi_dmic_mem *mem;
	struct sunxi_dmic_clk *clk;
	struct sunxi_dmic_pinctl *pin;
	struct sunxi_dmic_dts *dts;

	SND_LOG_DEBUG(HLOG, "\n");

	/* sunxi dmic info */
	dmic = devm_kzalloc(dev, sizeof(struct sunxi_dmic), GFP_KERNEL);
	if (IS_ERR_OR_NULL(dmic)) {
		SND_LOG_ERR(HLOG, "alloc sunxi_dmic failed\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, dmic);
	dmic->pdev = pdev;
	mem = &dmic->mem;
	clk = &dmic->clk;
	pin = &dmic->pin;
	dts = &dmic->dts;

	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR(HLOG, "remap init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_mem_init;
	}
	ret = snd_sunxi_clk_init(pdev, clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_clk_init;
	}

	snd_sunxi_dts_params_init(pdev, dts);

	ret = snd_sunxi_pin_init(pdev, pin);
	if (ret) {
		SND_LOG_ERR(HLOG, "pinctrl init failed\n");
		ret = -EINVAL;
		goto err_snd_sunxi_pin_init;
	}

	snd_sunxi_dma_params_init(dmic);

	ret = snd_soc_register_component(&pdev->dev,
					 &sunxi_dmic_dev,
					 &sunxi_dmic_dai, 1);
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

	SND_LOG_DEBUG(HLOG, "register dmic platform success\n");

	return 0;

err_snd_soc_sunxi_dma_platform_register:
	snd_soc_unregister_component(&pdev->dev);
err_snd_soc_register_component:
err_snd_sunxi_pin_init:
	snd_sunxi_clk_exit(clk);
err_snd_sunxi_clk_init:
	snd_sunxi_mem_exit(pdev, mem);
err_snd_sunxi_mem_init:
	devm_kfree(dev, dmic);
err_devm_kzalloc:
	of_node_put(np);

	return ret;
}

static int sunxi_dmic_dev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_dmic *dmic = dev_get_drvdata(&pdev->dev);
	struct sunxi_dmic_mem *mem = &dmic->mem;
	struct sunxi_dmic_clk *clk = &dmic->clk;
	struct sunxi_dmic_pinctl *pin = &dmic->pin;
	struct sunxi_dmic_dts *dts = &dmic->dts;

	/* remove components */
	if (dts->rx_sync_en)
		sunxi_rx_sync_remove(dts->rx_sync_domain);

	snd_sunxi_dma_platform_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);

	snd_sunxi_clk_exit(clk);
	snd_sunxi_mem_exit(pdev, mem);
	if (pin->pinctrl_used) {
		devm_pinctrl_put(pin->pinctrl);
	}

	devm_kfree(dev, dmic);
	of_node_put(np);

	SND_LOG_DEBUG(HLOG, "unregister dmic platform success\n");

	return 0;
}

static const struct of_device_id sunxi_dmic_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_dmic_of_match);

static struct platform_driver sunxi_dmic_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_dmic_of_match,
	},
	.probe	= sunxi_dmic_dev_probe,
	.remove	= sunxi_dmic_dev_remove,
};

module_platform_driver(sunxi_dmic_driver);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard platform of dmic");
