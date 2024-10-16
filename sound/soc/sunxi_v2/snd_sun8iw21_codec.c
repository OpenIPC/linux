/*
 * sound\soc\sunxi\snd_sun8iw21_codec.c
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
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/regmap.h>
#include <linux/of_address.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "snd_sunxi_log.h"
#include "snd_sunxi_pcm.h"
#include "snd_sunxi_rxsync.h"
#include "snd_sunxi_common.h"
#include "snd_sun8iw21_codec.h"

#define HLOG		"CODEC"
#define DRV_NAME	"sunxi-snd-codec"

struct resource g_res;
static struct regmap_config g_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_CODEC_REG_MAX,
	.cache_type = REGCACHE_NONE,
};

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

static const struct sample_rate sample_rate_conv[] = {
	{8000,   5},
	{11025,  4},
	{12000,  4},
	{16000,  3},
	{22050,  2},
	{24000,  2},
	{32000,  1},
	{44100,  0},
	{48000,  0},
	{96000,  7},
	{192000, 6},
};

static struct reg_label g_reg_labels[] = {
	REG_LABEL(SUNXI_DAC_DPC),
	REG_LABEL(SUNXI_DAC_VOL_CTRL),
	REG_LABEL(SUNXI_DAC_FIFOC),
	REG_LABEL(SUNXI_DAC_FIFOS),
	REG_LABEL(SUNXI_DAC_CNT),
	REG_LABEL(SUNXI_DAC_DG),
	REG_LABEL(SUNXI_ADC_FIFOC),
	REG_LABEL(SUNXI_ADC_VOL_CTRL),
	REG_LABEL(SUNXI_ADC_FIFOS),
	REG_LABEL(SUNXI_ADC_CNT),
	REG_LABEL(SUNXI_ADC_DG),
	REG_LABEL(SUNXI_ADC_DIG_CTRL),
	REG_LABEL(SUNXI_VRA1SPEEDUP_DOWN_CTRL),
	REG_LABEL(SUNXI_DAC_DAP_CTL),
	REG_LABEL(SUNXI_ADC_DAP_CTL),
	REG_LABEL(SUNXI_ADC1_REG),
	REG_LABEL(SUNXI_ADC2_REG),
	REG_LABEL(SUNXI_DAC_REG),
	REG_LABEL(SUNXI_MICBIAS_REG),
	REG_LABEL(SUNXI_RAMP_REG),
	REG_LABEL(SUNXI_BIAS_REG),
	REG_LABEL(SUNXI_POWER_REG),
	REG_LABEL(SUNXI_ADC_CUR_REG),
	REG_LABEL_END,
};

static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_clk *clk);
static void snd_sunxi_clk_exit(struct platform_device *pdev, struct sunxi_clk *clk);
static int snd_sunxi_clk_enable(struct platform_device *pdev, struct sunxi_clk *clk);
static void snd_sunxi_clk_disable(struct platform_device *pdev, struct sunxi_clk *clk);
static int snd_sunxi_regulator_init(struct platform_device *pdev, struct sunxi_regulator *rglt);
static void snd_sunxi_regulator_exit(struct platform_device *pdev, struct sunxi_regulator *rglt);
static int snd_sunxi_regulator_enable(struct platform_device *pdev, struct sunxi_regulator *rglt);
static void snd_sunxi_regulator_disable(struct platform_device *pdev, struct sunxi_regulator *rglt);

static void sunxi_rx_sync_enable(void *data, bool enable);

static int sunxi_internal_codec_dai_startup(struct snd_pcm_substream *substream,
					    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *snd_codec = dai->codec;
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct sunxi_dts *dts = &codec->dts;

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (dts->rx_sync_en && dts->rx_sync_ctl)
			sunxi_rx_sync_startup((void *)codec, dts->rx_sync_domain, dts->rx_sync_id,
					      sunxi_rx_sync_enable);
	}

	return 0;
}

static void sunxi_internal_codec_dai_shutdown(struct snd_pcm_substream *substream,
					      struct snd_soc_dai *dai)
{
	struct snd_soc_codec *snd_codec = dai->codec;
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct sunxi_dts *dts = &codec->dts;

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (dts->rx_sync_en && dts->rx_sync_ctl)
			sunxi_rx_sync_shutdown(dts->rx_sync_domain, dts->rx_sync_id);
	}
}

static void adchpf_config(struct regmap *regmap, unsigned int rate)
{
	switch (rate) {
	case 16000:
		regmap_write(regmap, SUNXI_ADC_DRC_HHPFC, (0x00F623A5 >> 16) & 0xFFFF);
		regmap_write(regmap, SUNXI_ADC_DRC_LHPFC, 0x00F623A5 & 0xFFFF);
		break;
	case 44100:
		regmap_write(regmap, SUNXI_ADC_DRC_HHPFC, (0x00FC60DB >> 16) & 0xFFFF);
		regmap_write(regmap, SUNXI_ADC_DRC_LHPFC, 0x00FC60DB & 0xFFFF);
		break;
	default:
		regmap_write(regmap, SUNXI_ADC_DRC_HHPFC, (0x00FCABB3 >> 16) & 0xFFFF);
		regmap_write(regmap, SUNXI_ADC_DRC_LHPFC, 0x00FCABB3 & 0xFFFF);
		break;
	}
}

static int sunxi_internal_codec_dai_hw_params(struct snd_pcm_substream *substream,
					      struct snd_pcm_hw_params *params,
					      struct snd_soc_dai *dai)
{
	int i;
	struct snd_soc_codec *snd_codec = dai->codec;
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	/* set bits */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				3 << FIFO_MODE, 3 << FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				1 << TX_SAMPLE_BITS, 0 << TX_SAMPLE_BITS);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				1 << RX_FIFO_MODE, 1 << RX_FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				1 << RX_SAMPLE_BITS, 0 << RX_SAMPLE_BITS);
		}
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				3 << FIFO_MODE, 0 << FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				1 << TX_SAMPLE_BITS, 1 << TX_SAMPLE_BITS);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				1 << RX_FIFO_MODE, 0 << RX_FIFO_MODE);
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				1 << RX_SAMPLE_BITS, 1 << RX_SAMPLE_BITS);
		}
		break;
	default:
		break;
	}

	/* set rate */
	for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++) {
		if (sample_rate_conv[i].samplerate == params_rate(params)) {
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
					0x7 << DAC_FS,
					sample_rate_conv[i].rate_bit << DAC_FS);
			} else {
				if (sample_rate_conv[i].samplerate > 48000)
					return -EINVAL;
				regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					0x7 << ADC_FS,
					sample_rate_conv[i].rate_bit << ADC_FS);
			}
		}
	}

	/* reset the adchpf func setting for different sampling */
	adchpf_config(regmap, params_rate(params));

	/* set channels */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (params_channels(params)) {
		case 1:
			/* DACL & DACR send same data */
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				0x1 << DAC_MONO_EN, 0x1 << DAC_MONO_EN);
			break;
		case 2:
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				0x1 << DAC_MONO_EN, 0x0 << DAC_MONO_EN);
			break;
		default:
			SND_LOG_WARN(HLOG, "not support channels:%u",
				     params_channels(params));
			return -EINVAL;
		}
	}

	return 0;
}

static int sunxi_internal_codec_dai_set_pll(struct snd_soc_dai *dai,
					    int pll_id, int source,
					    unsigned int freq_in,
					    unsigned int freq_out)
{
	struct snd_soc_codec *snd_codec = dai->codec;
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct sunxi_clk *clk = &codec->clk;

	SND_LOG_DEBUG(HLOG, "stream -> %s, freq_in ->%u, freq_out ->%u\n",
		      pll_id ? "adc" : "dac", freq_in, freq_out);

	if (pll_id == SNDRV_PCM_STREAM_PLAYBACK)
		goto set_pll_playback;
	else
		goto set_pll_capture;

set_pll_playback:
	if (clk_set_rate(clk->pllaudio, freq_in)) {
		SND_LOG_ERR(HLOG, "pllaudio set rate failed\n");
		return -EINVAL;
	}

	if (clk_set_rate(clk->dacclk, freq_out)) {
		SND_LOG_ERR(HLOG, "dacclk set rate failed\n");
		return -EINVAL;
	}

	return 0;

set_pll_capture:
	if (clk_set_rate(clk->pllaudio, freq_in)) {
		SND_LOG_ERR(HLOG, "pllaudio set rate failed\n");
		return -EINVAL;
	}

	if (clk_set_rate(clk->adcclk, freq_out)) {
		SND_LOG_ERR(HLOG, "adcclk set rate failed\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_internal_codec_dai_prepare(struct snd_pcm_substream *substream,
					    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *snd_codec = dai->codec;
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
				   1 << FIFO_FLUSH, 1 << FIFO_FLUSH);
		regmap_write(regmap, SUNXI_DAC_FIFOS,
			     1 << DAC_TXE_INT |\
			     1 << DAC_TXU_INT |\
			     1 << DAC_TXO_INT);
		regmap_write(regmap, SUNXI_DAC_CNT, 0);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << ADC_FIFO_FLUSH, 1 << ADC_FIFO_FLUSH);
		regmap_write(regmap, SUNXI_ADC_FIFOS,
			     1 << ADC_RXA_INT |\
			     1 << ADC_RXO_INT);
		regmap_write(regmap, SUNXI_ADC_CNT, 0);
	}

	return 0;
}

static int sunxi_internal_codec_dai_trigger(struct snd_pcm_substream *substream,
					    int cmd,
					    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *snd_codec = dai->codec;
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct sunxi_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "cmd -> %d\n", cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
					   1 << DAC_DRQ_EN, 1 << DAC_DRQ_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   1 << ADC_DRQ_EN, 1 << ADC_DRQ_EN);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, true);
		}
	break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
					   1 << DAC_DRQ_EN, 0 << DAC_DRQ_EN);
		} else {
			regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
					   1 << ADC_DRQ_EN, 0 << ADC_DRQ_EN);
			if (dts->rx_sync_en && dts->rx_sync_ctl)
				sunxi_rx_sync_control(dts->rx_sync_domain, dts->rx_sync_id, false);
		}
	break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_internal_codec_dai_ops = {
	.startup	= sunxi_internal_codec_dai_startup,
	.set_pll	= sunxi_internal_codec_dai_set_pll,
	.hw_params	= sunxi_internal_codec_dai_hw_params,
	.prepare	= sunxi_internal_codec_dai_prepare,
	.trigger	= sunxi_internal_codec_dai_trigger,
	.shutdown	= sunxi_internal_codec_dai_shutdown,
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
	.capture = {
		.stream_name	= "Capture",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates		= SNDRV_PCM_RATE_8000_48000
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
static void sunxi_rx_sync_enable(void *data, bool enable)
{
	struct sunxi_codec *codec = data;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "%s\n", enable ? "on" : "off");

	if (enable) {
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << RX_SYNC_EN_START, 1 << RX_SYNC_EN_START);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   1 << RX_SYNC_EN_START, 0 << RX_SYNC_EN_START);
	}

	return;
}

/* components function kcontrol setting */
static int sunxi_get_tx_hub_mode(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *snd_codec = snd_soc_component_to_codec(component);
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
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
	struct snd_soc_codec *snd_codec = snd_soc_component_to_codec(component);
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << LINEOUTLEN, 0x0 << LINEOUTLEN);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACLEN, 0x0 << DACLEN);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACLMUTE, 0x0 << DACLMUTE);
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DAC, 0x0 << EN_DAC);
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << DAC_HUB_EN, 0x0 << DAC_HUB_EN);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << DAC_HUB_EN, 0x1 << DAC_HUB_EN);
		regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x1 << EN_DAC, 0x1 << EN_DAC);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACLMUTE, 0x1 << DACLMUTE);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << DACLEN, 0x1 << DACLEN);
		regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1 << LINEOUTLEN, 0x1 << LINEOUTLEN);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sunxi_get_rx_sync_mode(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_dts *dts = &codec->dts;

	ucontrol->value.integer.value[0] = dts->rx_sync_ctl;

	return 0;
}

static int sunxi_set_rx_sync_mode(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		dts->rx_sync_ctl = 0;
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << RX_SYNC_EN, 0x0 << RX_SYNC_EN);
		break;
	case 1:
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << RX_SYNC_EN, 0x1 << RX_SYNC_EN);
		dts->rx_sync_ctl = 1;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const char *sunxi_switch_text[] = {"Off", "On"};

static SOC_ENUM_SINGLE_EXT_DECL(sunxi_tx_hub_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_rx_sync_mode_enum, sunxi_switch_text);
static const struct snd_kcontrol_new sunxi_tx_hub_controls[] = {
	SOC_ENUM_EXT("tx hub mode", sunxi_tx_hub_mode_enum,
		     sunxi_get_tx_hub_mode, sunxi_set_tx_hub_mode),
};
static const struct snd_kcontrol_new sunxi_rx_sync_controls[] = {
	SOC_ENUM_EXT("rx sync mode", sunxi_rx_sync_mode_enum,
		     sunxi_get_rx_sync_mode, sunxi_set_rx_sync_mode),
};

/* kcontrol setting */
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_codec_dacdrc_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_codec_adcdrc_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_codec_dachpf_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_EXT_DECL(sunxi_codec_adchpf_mode_enum, sunxi_switch_text);
static SOC_ENUM_SINGLE_DECL(sunxi_codec_adc_swap_enum, SUNXI_ADC_DG, AD_SWP1, sunxi_switch_text);

static const DECLARE_TLV_DB_SCALE(digital_tlv, -7424, 116, 0);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(mic_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(fmin_gain_tlv, 0, 600, 0);
static const DECLARE_TLV_DB_SCALE(linein_gain_tlv, 0, 600, 0);
static const unsigned int lineout_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 1, TLV_DB_SCALE_ITEM(0, 0, 1),
	2, 31, TLV_DB_SCALE_ITEM(-4350, 150, 1),
};

/* DAC&ADC-DRC&HPF FUNC */
static void dacdrc_enable(struct sunxi_codec *codec, bool on)
{
	struct sunxi_dap *dac_dap = &codec->dac_dap;
	struct regmap *regmap = codec->mem.regmap;

	mutex_lock(&dac_dap->mutex);
	if (on) {
		if (dac_dap->dap_enable == 0) {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_EN, 0x1 << DDAP_EN);
		}

		dac_dap->dap_enable |= BIT(0);
		regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
				   0x1 << DDAP_DRC_EN, 0x1 << DDAP_DRC_EN);
	} else {
		regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
				   0x1 << DDAP_DRC_EN, 0x0 << DDAP_DRC_EN);
		dac_dap->dap_enable &= ~BIT(0);

		if (dac_dap->dap_enable == 0) {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_EN, 0x0 << DDAP_EN);
		}
	}
	mutex_unlock(&dac_dap->mutex);
}

static int sunxi_codec_get_dacdrc_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAC_DAP_CTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << DDAP_DRC_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_dacdrc_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		dacdrc_enable(codec, 0);
		break;
	case 1:
		dacdrc_enable(codec, 1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void adcdrc_enable(struct sunxi_codec *codec, bool on)
{
	struct sunxi_dap *adc_dap = &codec->adc_dap;
	struct regmap *regmap = codec->mem.regmap;

	mutex_lock(&adc_dap->mutex);
	if (on) {
		if (adc_dap->dap_enable == 0) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP0_EN, 0x1 << ADC_DAP0_EN);
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP1_EN, 0x1 << ADC_DAP1_EN);
		}

		adc_dap->dap_enable |= BIT(0);
		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
				   0x1 << ADC_DRC0_EN, 0x1 << ADC_DRC0_EN);
		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
				   0x1 << ADC_DRC1_EN, 0x1 << ADC_DRC1_EN);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
				   0x1 << ADC_DRC0_EN, 0x0 << ADC_DRC0_EN);
		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
				   0x1 << ADC_DRC1_EN, 0x0 << ADC_DRC1_EN);
		adc_dap->dap_enable &= ~BIT(0);

		if (adc_dap->dap_enable == 0) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP0_EN, 0x0 << ADC_DAP0_EN);
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP1_EN, 0x0 << ADC_DAP1_EN);
		}
	}
	mutex_unlock(&adc_dap->mutex);
}

static int sunxi_codec_get_adcdrc_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << ADC_DRC0_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_adcdrc_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		adcdrc_enable(codec, 0);
		break;
	case 1:
		adcdrc_enable(codec, 1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void dachpf_enable(struct sunxi_codec *codec, bool on)
{
	struct sunxi_dap *dac_dap = &codec->dac_dap;
	struct regmap *regmap = codec->mem.regmap;

	mutex_lock(&dac_dap->mutex);
	if (on) {
		if (dac_dap->dap_enable == 0) {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_EN, 0x1 << DDAP_EN);
		}

		dac_dap->dap_enable |= BIT(1);
		regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
				   0x1 << DDAP_HPF_EN, 0x1 << DDAP_HPF_EN);
	} else {
		regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
				   0x1 << DDAP_HPF_EN, 0x0 << DDAP_HPF_EN);

		dac_dap->dap_enable &= ~BIT(1);
		if (dac_dap->dap_enable == 0) {
			regmap_update_bits(regmap, SUNXI_DAC_DAP_CTL,
					   0x1 << DDAP_EN, 0x0 << DDAP_EN);
		}
	}
	mutex_unlock(&dac_dap->mutex);
}

static int sunxi_codec_get_dachpf_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_DAC_DAP_CTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << DDAP_HPF_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_dachpf_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		dachpf_enable(codec, 0);
		break;
	case 1:
		dachpf_enable(codec, 1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void adchpf_enable(struct sunxi_codec *codec, bool on)
{
	struct sunxi_dap *adc_dap = &codec->adc_dap;
	struct regmap *regmap = codec->mem.regmap;

	mutex_lock(&adc_dap->mutex);
	if (on) {
		if (adc_dap->dap_enable == 0) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP0_EN, 0x1 << ADC_DAP0_EN);
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP1_EN, 0x1 << ADC_DAP1_EN);
		}

		adc_dap->dap_enable |= BIT(1);
		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
				   0x1 << ADC_HPF0_EN, 0x1 << ADC_HPF0_EN);
		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
				   0x1 << ADC_HPF1_EN, 0x1 << ADC_HPF1_EN);
	} else {
		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
				   0x1 << ADC_HPF0_EN, 0x0 << ADC_HPF0_EN);
		regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
				   0x1 << ADC_HPF1_EN, 0x0 << ADC_HPF1_EN);
		adc_dap->dap_enable &= ~BIT(1);

		if (adc_dap->dap_enable == 0) {
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP0_EN, 0x0 << ADC_DAP0_EN);
			regmap_update_bits(regmap, SUNXI_ADC_DAP_CTL,
					   0x1 << ADC_DAP1_EN, 0x0 << ADC_DAP1_EN);
		}
	}
	mutex_unlock(&adc_dap->mutex);
}

static int sunxi_codec_get_adchpf_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, SUNXI_ADC_DAP_CTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (0x1 << ADC_HPF0_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_adchpf_mode(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		adchpf_enable(codec, 0);
		break;
	case 1:
		adchpf_enable(codec, 1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sunxi_get_gain_volsw(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_dts *dts = &codec->dts;
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	unsigned int invert = mc->invert;
	unsigned int shift = mc->shift;
	int max = mc->max;
	int min = mc->min;
	u32 val_tmp;

	switch (shift) {
	case MIC1_GAIN_SHIFT:
		val_tmp = dts->mic1gain;
	break;
	case MIC2_GAIN_SHIFT:
		val_tmp = dts->mic2gain;
	break;
	default:
		SND_LOG_ERR(HLOG, "the gain is null\n");
	break;
	}

	if (val_tmp > max || val_tmp < min)
		return -1;

	ucontrol->value.integer.value[0] = val_tmp - min;
	if (invert)
		ucontrol->value.integer.value[0] =
			max - ucontrol->value.integer.value[0];

	return 0;
}

static int sunxi_put_gain_volsw(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_codec *codec = snd_soc_component_get_drvdata(component);
	struct sunxi_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;
	struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;
	unsigned int invert = mc->invert;
	unsigned int shift = mc->shift;
	int max = mc->max;
	int min = mc->min;
	u32 val_tmp;

	val_tmp = ucontrol->value.integer.value[0] + min;
	if (invert)
		val_tmp = max - val_tmp;

	switch (shift) {
	case MIC1_GAIN_SHIFT:
		dts->mic1gain = val_tmp;
		if (codec->mic1gain_run)
			regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1F << ADC1_PGA_GAIN_CTRL,
					   dts->mic1gain << ADC1_PGA_GAIN_CTRL);
	break;
	case MIC2_GAIN_SHIFT:
		dts->mic2gain = val_tmp;
		if (codec->mic2gain_run)
			regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1F << ADC2_PGA_GAIN_CTRL,
					   dts->mic2gain << ADC2_PGA_GAIN_CTRL);
	break;
	default:
		SND_LOG_ERR(HLOG, "the gain is null\n");
	break;
	}

	return 0;
}

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	/* DAP func */
	SOC_ENUM_EXT("DACDRC", sunxi_codec_dacdrc_mode_enum,
		     sunxi_codec_get_dacdrc_mode, sunxi_codec_set_dacdrc_mode),
	SOC_ENUM_EXT("ADCDRC", sunxi_codec_adcdrc_mode_enum,
		     sunxi_codec_get_adcdrc_mode, sunxi_codec_set_adcdrc_mode),
	SOC_ENUM_EXT("DACHPF", sunxi_codec_dachpf_mode_enum,
		     sunxi_codec_get_dachpf_mode, sunxi_codec_set_dachpf_mode),
	SOC_ENUM_EXT("ADCHPF", sunxi_codec_adchpf_mode_enum,
		     sunxi_codec_get_adchpf_mode, sunxi_codec_set_adchpf_mode),

	SOC_ENUM("ADC1 ADC2 swap", sunxi_codec_adc_swap_enum),
	/* Digital Volume */
	SOC_SINGLE_TLV("digital volume", SUNXI_DAC_DPC,
		       DVOL, 0x3F, 1, digital_tlv),
	/* DAC Volume */
	SOC_SINGLE_TLV("DAC volume", SUNXI_DAC_VOL_CTRL,
		       DAC_VOL_L, 0xFF, 0, dac_vol_tlv),
	/* ADC1 Volume */
	SOC_SINGLE_TLV("ADC1 volume", SUNXI_ADC_VOL_CTRL,
		       ADC1_VOL, 0xFF, 0, adc_vol_tlv),
	/* ADC2 Volume */
	SOC_SINGLE_TLV("ADC2 volume", SUNXI_ADC_VOL_CTRL,
		       ADC2_VOL, 0xFF, 0, adc_vol_tlv),
	/* MIC1 Gain */
	SOC_SINGLE_EXT_TLV("MIC1 gain volume", SND_SOC_NOPM,
			   MIC1_GAIN_SHIFT, 0x1F, 0,
			   sunxi_get_gain_volsw, sunxi_put_gain_volsw,
			   mic_gain_tlv),
	/* MIC2 Gain */
	SOC_SINGLE_EXT_TLV("MIC2 gain volume", SND_SOC_NOPM,
			   MIC2_GAIN_SHIFT, 0x1F, 0,
			   sunxi_get_gain_volsw, sunxi_put_gain_volsw,
			   mic_gain_tlv),
	/* LINEIN_L Gain */
	SOC_SINGLE_TLV("LINEINL gain volume", SUNXI_ADC1_REG,
		       LINEINLG, 0x1, 0, linein_gain_tlv),
	/* LINEIN_R Gain */
	SOC_SINGLE_TLV("LINEINR gain volume", SUNXI_ADC2_REG,
		       LINEINRG, 0x1, 0, linein_gain_tlv),
	/* LINEOUT Volume */
	SOC_SINGLE_TLV("LINEOUT volume", SUNXI_DAC_REG,
		       LINEOUT_VOL, 0x1F, 0, lineout_tlv),
};

/* dapm setting */
static const char *differ_select_text[] = {"single", "differ"};
static const char *differ_select_text_i[] = {"differ", "single"};

static SOC_ENUM_SINGLE_DECL(lineoutl_output_enum,
			    SUNXI_DAC_REG, LINEOUTLDIFFEN, differ_select_text);
static SOC_ENUM_SINGLE_DECL(mic1_input_enum,
			    SUNXI_ADC1_REG, MIC1_SIN_EN, differ_select_text_i);
static SOC_ENUM_SINGLE_DECL(mic2_input_enum,
			    SUNXI_ADC2_REG, MIC2_SIN_EN, differ_select_text_i);

static const struct snd_kcontrol_new lineoutl_output_mux =
	SOC_DAPM_ENUM("LINEOUT Output Select", lineoutl_output_enum);
static const struct snd_kcontrol_new mic1_input_mux =
	SOC_DAPM_ENUM("MIC1 Input Select", mic1_input_enum);
static const struct snd_kcontrol_new mic2_input_mux =
	SOC_DAPM_ENUM("MIC2 Input Select", mic2_input_enum);

static int sunxi_codec_dacl_event(struct snd_soc_dapm_widget *w,
				  struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *snd_codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, SUNXI_DAC_REG,
				   0x1 << DACLEN, 0x1 << DACLEN);
		regmap_update_bits(regmap, SUNXI_DAC_REG,
				   0x1 << DACLMUTE, 0x1 << DACLMUTE);
		regmap_update_bits(regmap, SUNXI_DAC_DPC,
				   0x1<<EN_DAC, 0x1<<EN_DAC);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_DAC_DPC,
				   0x1<<EN_DAC, 0x0<<EN_DAC);
		regmap_update_bits(regmap, SUNXI_DAC_REG,
				   0x1 << DACLMUTE, 0x0 << DACLMUTE);
		regmap_update_bits(regmap, SUNXI_DAC_REG,
				   0x1 << DACLEN, 0x0 << DACLEN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_adc1_event(struct snd_soc_dapm_widget *w,
				  struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *snd_codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		codec->mic1gain_run = 1;
		regmap_update_bits(regmap, SUNXI_ADC_DIG_CTRL,
				   0x1 << ADC1_CHANNEL_EN,
				   0x1 << ADC1_CHANNEL_EN);
		regmap_update_bits(regmap, SUNXI_ADC1_REG,
				   0x1 << ADC1_EN, 0x1 << ADC1_EN);
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   0x1 << EN_AD, 0x1 << EN_AD);
		break;
	case SND_SOC_DAPM_POST_PMD:
		codec->mic1gain_run = 0;
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   0x1 << EN_AD, 0x0 << EN_AD);
		regmap_update_bits(regmap, SUNXI_ADC1_REG,
				   0x1 << ADC1_EN, 0x0 << ADC1_EN);
		regmap_update_bits(regmap, SUNXI_ADC_DIG_CTRL,
				   0x1 << ADC1_CHANNEL_EN,
				   0x0 << ADC1_CHANNEL_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_adc2_event(struct snd_soc_dapm_widget *w,
				  struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *snd_codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		codec->mic2gain_run = 1;
		regmap_update_bits(regmap, SUNXI_ADC_DIG_CTRL,
				   0x1 << ADC2_CHANNEL_EN,
				   0x1 << ADC2_CHANNEL_EN);
		regmap_update_bits(regmap, SUNXI_ADC2_REG,
				   0x1 << ADC2_EN, 0x1 << ADC2_EN);
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   0x1 << EN_AD, 0x1 << EN_AD);
		break;
	case SND_SOC_DAPM_POST_PMD:
		codec->mic2gain_run = 0;
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
				   0x1 << EN_AD, 0x0 << EN_AD);
		regmap_update_bits(regmap, SUNXI_ADC2_REG,
				   0x1 << ADC2_EN, 0x0 << ADC2_EN);
		regmap_update_bits(regmap, SUNXI_ADC_DIG_CTRL,
				   0x1 << ADC2_CHANNEL_EN,
				   0x0 << ADC2_CHANNEL_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_mic1_gain_event(struct snd_soc_dapm_widget *w,
				 struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *snd_codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct sunxi_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1F << ADC1_PGA_GAIN_CTRL,
				   dts->mic1gain << ADC1_PGA_GAIN_CTRL);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1F << ADC1_PGA_GAIN_CTRL,
				   0x0 << ADC1_PGA_GAIN_CTRL);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_mic2_gain_event(struct snd_soc_dapm_widget *w,
				 struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *snd_codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct sunxi_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1F << ADC2_PGA_GAIN_CTRL,
				   dts->mic2gain << ADC2_PGA_GAIN_CTRL);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1F << ADC2_PGA_GAIN_CTRL,
				   0x0 << ADC2_PGA_GAIN_CTRL);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_mic1_event(struct snd_soc_dapm_widget *w,
				  struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *snd_codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, SUNXI_ADC1_REG,
				   0x1 << MIC1_PGA_EN, 0x1 << MIC1_PGA_EN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_ADC1_REG,
				   0x1 << MIC1_PGA_EN, 0x0 << MIC1_PGA_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_mic2_event(struct snd_soc_dapm_widget *w,
				  struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *snd_codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		regmap_update_bits(regmap, SUNXI_ADC2_REG,
				   0x1 << MIC2_PGA_EN, 0x1 << MIC2_PGA_EN);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(regmap, SUNXI_ADC2_REG,
				   0x1 << MIC2_PGA_EN, 0x0 << MIC2_PGA_EN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_linein_event(struct snd_soc_dapm_widget *w,
				    struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *snd_codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_ADC1_REG,
				   0x1 << LINEINLEN, 0x1 << LINEINLEN);
		regmap_update_bits(regmap, SUNXI_ADC2_REG,
				   0x1 << LINEINREN, 0x1 << LINEINREN);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, SUNXI_ADC1_REG,
				   0x1 << LINEINLEN, 0x0 << LINEINLEN);
		regmap_update_bits(regmap, SUNXI_ADC2_REG,
				   0x1 << LINEINREN, 0x0 << LINEINREN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_lineout_event(struct snd_soc_dapm_widget *w,
				     struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *snd_codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(regmap, SUNXI_DAC_REG,
				   0x1 << LINEOUTLEN, 0x1 << LINEOUTLEN);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(regmap, SUNXI_DAC_REG,
				   0x1 << LINEOUTLEN, 0x0 << LINEOUTLEN);
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_speaker_event(struct snd_soc_dapm_widget *w,
				     struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *snd_codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);

	SND_LOG_DEBUG(HLOG, "event -> 0x%x\n", event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_sunxi_pa_pin_enable(codec->pa_config, codec->pa_pin_max);
		break;
	case SND_SOC_DAPM_PRE_PMD:
		snd_sunxi_pa_pin_disable(codec->pa_config, codec->pa_pin_max);
		break;
	default:
		break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, SND_SOC_NOPM, 0, 0,
			      sunxi_codec_dacl_event,
			      SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("ADC1", "Capture", 0, SND_SOC_NOPM, 0, 0,
			       sunxi_codec_adc1_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADC2", "Capture", 0, SND_SOC_NOPM, 0, 0,
			       sunxi_codec_adc2_event,
			       SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX("LINEOUT Output Select", SND_SOC_NOPM, 0, 0,
			 &lineoutl_output_mux),

	SND_SOC_DAPM_MUX("MIC1 Input Select", SND_SOC_NOPM, 0, 0,
			 &mic1_input_mux),
	SND_SOC_DAPM_MUX("MIC2 Input Select", SND_SOC_NOPM, 0, 0,
			 &mic2_input_mux),

	SND_SOC_DAPM_PGA_E("MIC1 PGA", SND_SOC_NOPM, 0, 0, NULL, 0,
			   sunxi_mic1_gain_event,
			   SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("MIC2 PGA", SND_SOC_NOPM, 0, 0, NULL, 0,
			   sunxi_mic2_gain_event,
			   SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MICBIAS("MIC1 Bias", SUNXI_MICBIAS_REG, MMICBIASEN, 0),
	SND_SOC_DAPM_MICBIAS("MIC2 Bias", SUNXI_MICBIAS_REG, MMICBIASEN, 0),

	SND_SOC_DAPM_INPUT("MIC1_PIN"),
	SND_SOC_DAPM_INPUT("MIC2_PIN"),
	SND_SOC_DAPM_INPUT("LINEINL_PIN"),
	SND_SOC_DAPM_INPUT("LINEINR_PIN"),
	SND_SOC_DAPM_OUTPUT("LINEOUTL_PIN"),

	SND_SOC_DAPM_MIC("MIC1", sunxi_codec_mic1_event),
	SND_SOC_DAPM_MIC("MIC2", sunxi_codec_mic2_event),
	SND_SOC_DAPM_LINE("LINEIN", sunxi_codec_linein_event),
	SND_SOC_DAPM_LINE("LINEOUT", sunxi_codec_lineout_event),
	SND_SOC_DAPM_SPK("SPK", sunxi_codec_speaker_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	/* input route -> MIC1 MIC2 */
	{"MIC1_PIN", NULL, "MIC1"},
	{"MIC2_PIN", NULL, "MIC2"},

	{"MIC1 Bias", NULL, "MIC1_PIN"},
	{"MIC2 Bias", NULL, "MIC2_PIN"},

	{"MIC1 Input Select", "single", "MIC1 Bias"},
	{"MIC1 Input Select", "differ", "MIC1 Bias"},
	{"MIC2 Input Select", "single", "MIC2 Bias"},
	{"MIC2 Input Select", "differ", "MIC2 Bias"},

	{"MIC1 PGA", NULL, "MIC1 Input Select"},
	{"MIC2 PGA", NULL, "MIC2 Input Select"},

	{"ADC1", NULL, "MIC1 PGA"},
	{"ADC2", NULL, "MIC2 PGA"},

	/* input route -> LINEIN */
	{"LINEINL_PIN", NULL, "LINEIN"},
	{"LINEINR_PIN", NULL, "LINEIN"},

	{"ADC1", NULL, "LINEINL_PIN"},
	{"ADC2", NULL, "LINEINR_PIN"},

	/* output route -> LINEOUT */
	{"LINEOUT Output Select", "single", "DACL"},
	{"LINEOUT Output Select", "differ", "DACL"},

	{"LINEOUTL_PIN", NULL, "LINEOUT Output Select"},
	{"LINEOUT", NULL, "LINEOUTL_PIN"},
	{"SPK", NULL, "LINEOUTL_PIN"},
};

static void sunxi_codec_init(struct snd_soc_codec *snd_codec)
{
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct sunxi_regulator *rglt = &codec->rglt;
	struct sunxi_dts *dts = &codec->dts;
	struct regmap *regmap = codec->mem.regmap;
	unsigned int adc_dtime_map;
	unsigned int avcc_vol_map;

	SND_LOG_DEBUG(HLOG, "\n");

	if (rglt->external_avcc) {
		regmap_update_bits(regmap, SUNXI_POWER_REG, 0x1 << ALDO_EN, 0x0 << ALDO_EN);
	} else {
		switch (rglt->avcc_vol) {
		case 1650000:
			avcc_vol_map = 0;
			break;
		case 1700000:
			avcc_vol_map = 1;
			break;
		case 1750000:
			avcc_vol_map = 2;
			break;
		case 1800000:
			avcc_vol_map = 3;
			break;
		case 1850000:
			avcc_vol_map = 4;
			break;
		case 1900000:
			avcc_vol_map = 5;
			break;
		case 1950000:
			avcc_vol_map = 6;
			break;
		case 2000000:
			avcc_vol_map = 7;
			break;
		default:
			avcc_vol_map = 3;
			SND_LOG_DEBUG(HLOG, "unsupport avcc vol, default 1.8v\n");
			break;
		}
		regmap_update_bits(regmap, SUNXI_POWER_REG, 0x7 << ALDO_OUTPUT_VOLTAGE,
				   avcc_vol_map << ALDO_OUTPUT_VOLTAGE);
		regmap_update_bits(regmap, SUNXI_POWER_REG, 0x1 << ALDO_EN, 0x1 << ALDO_EN);
	}

	regmap_update_bits(regmap, SUNXI_RAMP_REG, 0x1 << RMC_EN, 0x1 << RMC_EN);

	/* Enable ADCFDT to overcome niose at the beginning */
	regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
			   0x1 << ADCDFEN, 0x1 << ADCDFEN);
	switch (dts->adc_dtime) {
	case 5:
		adc_dtime_map = 0;
		break;
	case 10:
		adc_dtime_map = 1;
		break;
	case 20:
		adc_dtime_map = 2;
		break;
	case 30:
		adc_dtime_map = 3;
		break;
	case 0:
	default:
		regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << ADCDFEN, 0x0 << ADCDFEN);
		break;
	}
	regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x3 << ADCFDT, adc_dtime_map << ADCFDT);
	regmap_update_bits(regmap, SUNXI_ADC_FIFOC, 0x1 << RX_SYNC_EN, 0x0 << RX_SYNC_EN);

	/* Digital VOL defeult setting */
	regmap_update_bits(regmap, SUNXI_DAC_DPC, 0x3F << DVOL, 0 << DVOL);

	/* LINEOUT VOL defeult setting */
	regmap_update_bits(regmap, SUNXI_DAC_REG, 0x1F << LINEOUT_VOL,
			   dts->lineout_vol << LINEOUT_VOL);

	/* ADCL MIC1 gain defeult setting */
	regmap_update_bits(regmap, SUNXI_ADC1_REG, 0x1F << ADC1_PGA_GAIN_CTRL,
			   dts->mic1gain << ADC1_PGA_GAIN_CTRL);
	/* ADCR MIC2 gain defeult setting */
	regmap_update_bits(regmap, SUNXI_ADC2_REG, 0x1F << ADC2_PGA_GAIN_CTRL,
			   dts->mic2gain << ADC2_PGA_GAIN_CTRL);

	/* ADC IOP params default setting */
	regmap_update_bits(regmap, SUNXI_ADC1_REG, 0xFF << ADC1_IOPMIC, 0x55 << ADC1_IOPMIC);
	regmap_update_bits(regmap, SUNXI_ADC2_REG, 0xFF << ADC2_IOPMIC, 0x55 << ADC2_IOPMIC);

	/* lineout & mic1 & mic2 diff or single setting */
	if (dts->lineout_single)
		regmap_update_bits(regmap, SUNXI_DAC_REG,
				   0x1 << LINEOUTLDIFFEN, 0x0 << LINEOUTLDIFFEN);
	else
		regmap_update_bits(regmap, SUNXI_DAC_REG,
				   0x1 << LINEOUTLDIFFEN, 0x1 << LINEOUTLDIFFEN);
	if (dts->mic1_single)
		regmap_update_bits(regmap, SUNXI_ADC1_REG,
				   0x1 << MIC1_SIN_EN, 0x1 << MIC1_SIN_EN);
	else
		regmap_update_bits(regmap, SUNXI_ADC1_REG,
				   0x1 << MIC1_SIN_EN, 0x0 << MIC1_SIN_EN);
	if (dts->mic2_single)
		regmap_update_bits(regmap, SUNXI_ADC2_REG,
				   0x1 << MIC2_SIN_EN, 0x1 << MIC2_SIN_EN);
	else
		regmap_update_bits(regmap, SUNXI_ADC2_REG,
				   0x1 << MIC2_SIN_EN, 0x0 << MIC2_SIN_EN);

	regmap_update_bits(regmap, SUNXI_DAC_VOL_CTRL, 1 << DAC_VOL_SEL, 1 << DAC_VOL_SEL);
	regmap_update_bits(regmap, SUNXI_ADC_DIG_CTRL, 1 << ADC1_2_VOL_EN, 1 << ADC1_2_VOL_EN);
}

static int sunxi_internal_codec_probe(struct snd_soc_codec *snd_codec)
{
	int ret;
	struct snd_soc_dapm_context *dapm = &snd_codec->component.dapm;
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct sunxi_dts *dts = &codec->dts;

	SND_LOG_DEBUG(HLOG, "\n");

	codec->mic1gain_run = 0;
	codec->mic2gain_run = 0;

	mutex_init(&codec->dac_dap.mutex);
	mutex_init(&codec->adc_dap.mutex);

	/* component kcontrols -> tx_hub */
	if (dts->tx_hub_en) {
		ret = snd_soc_add_codec_controls(snd_codec, sunxi_tx_hub_controls,
						 ARRAY_SIZE(sunxi_tx_hub_controls));
		if (ret)
			SND_LOG_ERR(HLOG, "add tx_hub kcontrols failed\n");
	}

	/* component kcontrols -> rx_sync */
	if (dts->rx_sync_en) {
		ret = snd_soc_add_codec_controls(snd_codec, sunxi_rx_sync_controls,
						 ARRAY_SIZE(sunxi_rx_sync_controls));
		if (ret)
			SND_LOG_ERR(HLOG, "add rx_sync kcontrols failed\n");
	}

	ret = snd_soc_add_codec_controls(snd_codec, sunxi_codec_controls,
					 ARRAY_SIZE(sunxi_codec_controls));
	if (ret)
		SND_LOG_ERR(HLOG, "add kcontrols failed\n");

	snd_soc_dapm_new_controls(dapm, sunxi_codec_dapm_widgets,
				  ARRAY_SIZE(sunxi_codec_dapm_widgets));
	snd_soc_dapm_add_routes(dapm, sunxi_codec_dapm_routes,
				ARRAY_SIZE(sunxi_codec_dapm_routes));

	sunxi_codec_init(snd_codec);

	return 0;
}

static int sunxi_internal_codec_remove(struct snd_soc_codec *snd_codec)
{
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);

	SND_LOG_DEBUG(HLOG, "\n");

	mutex_destroy(&codec->dac_dap.mutex);
	mutex_destroy(&codec->adc_dap.mutex);

	return 0;
}

static int sunxi_internal_codec_suspend(struct snd_soc_codec *snd_codec)
{
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct sunxi_clk *clk = &codec->clk;
	struct sunxi_regulator *rglt = &codec->rglt;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	/* save reg value */
	snd_sunxi_save_reg(regmap, g_reg_labels);

	/* disable clk & regulator */
	snd_sunxi_pa_pin_disable(codec->pa_config, codec->pa_pin_max);
	snd_sunxi_regulator_disable(codec->pdev, rglt);
	snd_sunxi_clk_disable(codec->pdev, clk);

	return 0;
}

static int sunxi_internal_codec_resume(struct snd_soc_codec *snd_codec)
{
	int ret;
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct sunxi_clk *clk = &codec->clk;
	struct sunxi_regulator *rglt = &codec->rglt;
	struct regmap *regmap = codec->mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_pa_pin_disable(codec->pa_config, codec->pa_pin_max);
	ret = snd_sunxi_clk_enable(codec->pdev, clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk enable failed\n");
		return ret;
	}
	ret = snd_sunxi_regulator_enable(codec->pdev, rglt);
	if (ret) {
		SND_LOG_ERR(HLOG, "regulator enable failed\n");
		return ret;
	}

	/* for codec init */
	sunxi_codec_init(snd_codec);

	/* resume reg value */
	snd_sunxi_echo_reg(regmap, g_reg_labels);

	/* for clear TX fifo */
	regmap_update_bits(regmap, SUNXI_DAC_FIFOC,
			   1 << FIFO_FLUSH, 1 << FIFO_FLUSH);
	regmap_write(regmap, SUNXI_DAC_FIFOS,
		     1 << DAC_TXE_INT | 1 << DAC_TXU_INT | 1 << DAC_TXO_INT);
	regmap_write(regmap, SUNXI_DAC_CNT, 0);

	/* for clear RX fifo */
	regmap_update_bits(regmap, SUNXI_ADC_FIFOC,
			   1 << ADC_FIFO_FLUSH, 1 << ADC_FIFO_FLUSH);
	regmap_write(regmap, SUNXI_ADC_FIFOS,
		     1 << ADC_RXA_INT | 1 << ADC_RXO_INT);
	regmap_write(regmap, SUNXI_ADC_CNT, 0);

	return 0;
}

static unsigned int sunxi_internal_codec_read(struct snd_soc_codec *snd_codec, unsigned int reg)
{
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;
	unsigned int reg_val;

	regmap_read(regmap, reg, &reg_val);

	return reg_val;
}

static int sunxi_internal_codec_write(struct snd_soc_codec *snd_codec,
				      unsigned int reg, unsigned int val)
{
	struct sunxi_codec *codec = snd_soc_codec_get_drvdata(snd_codec);
	struct regmap *regmap = codec->mem.regmap;

	regmap_write(regmap, reg, val);

	return 0;
};

static struct snd_soc_codec_driver sunxi_internal_codec_dev = {
	.probe		= sunxi_internal_codec_probe,
	.remove		= sunxi_internal_codec_remove,
	.suspend	= sunxi_internal_codec_suspend,
	.resume		= sunxi_internal_codec_resume,
	.read		= sunxi_internal_codec_read,
	.write		= sunxi_internal_codec_write,
	.ignore_pmdown_time = 1,	/* Doesn't benefit from pmdown delay */
};

/*******************************************************************************
 * *** kernel source ***
 * @1 regmap
 * @2 clk
 * @3 regulator
 * @4 pa pin
 * @5 dts params
 ******************************************************************************/
static int snd_sunxi_clk_init(struct platform_device *pdev, struct sunxi_clk *clk)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	clk->pllaudio = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(clk->pllaudio)) {
		SND_LOG_ERR(HLOG, "pllaudio get failed\n");
		ret = PTR_ERR(clk->pllaudio);
		goto err_pllaudio;
	}
	clk->dacclk = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(clk->dacclk)) {
		SND_LOG_ERR(HLOG, "dacclk get failed\n");
		ret = PTR_ERR(clk->dacclk);
		goto err_dacclk;
	}
	clk->adcclk = of_clk_get(np, 2);
	if (IS_ERR_OR_NULL(clk->adcclk)) {
		SND_LOG_ERR(HLOG, "adcclk get failed\n");
		ret = PTR_ERR(clk->adcclk);
		goto err_adcclk;
	}

	if (clk_set_parent(clk->dacclk, clk->pllaudio)) {
		SND_LOG_ERR(HLOG, "set parent of dacclk to pllaudio failed\n");
		ret = -EINVAL;
		goto err_set_parent;
	}
	if (clk_set_parent(clk->adcclk, clk->pllaudio)) {
		SND_LOG_ERR(HLOG, "set parent of adcclk to pllaudio failed\n");
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
	clk_put(clk->adcclk);
err_adcclk:
	clk_put(clk->dacclk);
err_dacclk:
	clk_put(clk->pllaudio);
err_pllaudio:
	return ret;
}

static void snd_sunxi_clk_exit(struct platform_device *pdev, struct sunxi_clk *clk)
{
	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_clk_disable(pdev, clk);
	clk_put(clk->adcclk);
	clk_put(clk->dacclk);
	clk_put(clk->pllaudio);
}

static int snd_sunxi_clk_enable(struct platform_device *pdev, struct sunxi_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	/* (22579200 or 24576000) * n */
	if (clk_set_rate(clk->pllaudio, 22579200)) {
		SND_LOG_ERR(HLOG, "pllaudio set rate failed\n");
		goto err_set_rate;
	}

	if (clk_prepare_enable(clk->pllaudio)) {
		SND_LOG_ERR(HLOG, "pllaudio enable failed\n");
		goto err_enable_pllaudio;
	}

	if (clk_prepare_enable(clk->dacclk)) {
		SND_LOG_ERR(HLOG, "dacclk enable failed\n");
		goto err_enable_dacclk;
	}

	if (clk_prepare_enable(clk->adcclk)) {
		SND_LOG_ERR(HLOG, "adcclk enable failed\n");
		goto err_enable_adcclk;
	}

	return 0;

err_enable_adcclk:
	clk_disable_unprepare(clk->dacclk);
err_enable_dacclk:
	clk_disable_unprepare(clk->pllaudio);
err_enable_pllaudio:
err_set_rate:
	return ret;
}

static void snd_sunxi_clk_disable(struct platform_device *pdev, struct sunxi_clk *clk)
{
	SND_LOG_DEBUG(HLOG, "\n");

	clk_disable_unprepare(clk->adcclk);
	clk_disable_unprepare(clk->dacclk);
	clk_disable_unprepare(clk->pllaudio);
}

static int snd_sunxi_regulator_init(struct platform_device *pdev, struct sunxi_regulator *rglt)
{
	int ret = 0;
	unsigned int temp_val;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	rglt->external_avcc = of_property_read_bool(np, "external-avcc");
	ret = of_property_read_u32(np, "avcc-vol", &temp_val);
	if (ret < 0) {
		/* default avcc voltage: 1.8v */
		rglt->avcc_vol = 1800000;
	} else {
		rglt->avcc_vol = temp_val;
	}

	if (rglt->external_avcc) {
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

static void snd_sunxi_dts_params_init(struct platform_device *pdev,
				      struct sunxi_dts *dts)
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

	/* mic gain for capturing */
	ret = of_property_read_u32(np, "mic1gain", &temp_val);
	if (ret < 0) {
		dts->mic1gain = 31;
	} else {
		dts->mic1gain = temp_val;
	}
	ret = of_property_read_u32(np, "mic2gain", &temp_val);
	if (ret < 0) {
		dts->mic2gain = 31;
	} else {
		dts->mic2gain = temp_val;
	}

	ret = of_property_read_u32(np, "adcdelaytime", &temp_val);
	if (ret < 0) {
		dts->adc_dtime = 0;
	} else {
		switch (temp_val) {
		case 0:
		case 5:
		case 10:
		case 20:
		case 30:
			dts->adc_dtime = temp_val;
			break;
		default:
			SND_LOG_WARN(HLOG, "adc delay time supoort only 0,5,10,20,30ms\n");
			dts->adc_dtime = 0;
			break;
		}
	}

	/* lineout & mic1 & mic2 diff or single */
	dts->lineout_single = of_property_read_bool(np, "lineout-single");
	dts->mic1_single = of_property_read_bool(np, "mic1-single");
	dts->mic2_single = of_property_read_bool(np, "mic2-single");

	/* tx_hub */
	dts->tx_hub_en = of_property_read_bool(np, "tx-hub-en");

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

	SND_LOG_DEBUG(HLOG, "lineout vol -> %u\n", dts->lineout_vol);
	SND_LOG_DEBUG(HLOG, "mic1 gain   -> %u\n", dts->mic1gain);
	SND_LOG_DEBUG(HLOG, "mic2 gain   -> %u\n", dts->mic2gain);
}

static int sunxi_internal_codec_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_codec *codec;
	struct sunxi_mem *mem;

	SND_LOG_DEBUG(HLOG, "\n");

	/* sunxi codec info */
	codec = devm_kzalloc(dev, sizeof(struct sunxi_codec), GFP_KERNEL);
	if (IS_ERR_OR_NULL(codec)) {
		SND_LOG_ERR(HLOG, "alloc sunxi_codec failed\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(dev, codec);
	codec->pdev = pdev;

	/* for kernel source */
	mem = &codec->mem;
	mem->dev_name = DRV_NAME;
	mem->res = &g_res;
	mem->regmap_config = &g_regmap_config;
	ret = snd_sunxi_mem_init(pdev, mem);
	if (ret) {
		SND_LOG_ERR(HLOG, "mem init failed\n");
		ret = -ENOMEM;
		goto err_mem_init;
	}

	ret = snd_sunxi_clk_init(pdev, &codec->clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk init failed\n");
		ret = -ENOMEM;
		goto err_clk_init;
	}

	ret = snd_sunxi_regulator_init(pdev, &codec->rglt);
	if (ret) {
		SND_LOG_ERR(HLOG, "regulator init failed\n");
		ret = -ENOMEM;
		goto err_regulator_init;
	}

	codec->pa_config = snd_sunxi_pa_pin_init(pdev, &codec->pa_pin_max);

	snd_sunxi_dts_params_init(pdev, &codec->dts);

	/* alsa codec register */
	ret = snd_soc_register_codec(dev, &sunxi_internal_codec_dev, &sunxi_internal_codec_dai, 1);
	if (ret) {
		SND_LOG_ERR(HLOG, "internal-codec component register failed\n");
		ret = -ENOMEM;
		goto err_register_component;
	}

	SND_LOG_DEBUG(HLOG, "register internal-codec codec success\n");

	return 0;

err_register_component:
	snd_sunxi_regulator_exit(pdev, &codec->rglt);
err_regulator_init:
	snd_sunxi_clk_exit(pdev, &codec->clk);
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
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_codec *codec = dev_get_drvdata(&pdev->dev);
	struct sunxi_dts *dts = &codec->dts;

	SND_LOG_DEBUG(HLOG, "\n");

	/* remove components */
	if (dts->rx_sync_en)
		sunxi_rx_sync_remove(dts->rx_sync_domain);

	/* alsa codec unregister */
	snd_soc_unregister_codec(&pdev->dev);

	/* for kernel source */
	snd_sunxi_clk_exit(pdev, &codec->clk);
	snd_sunxi_mem_exit(pdev, &codec->mem);
	snd_sunxi_regulator_exit(pdev, &codec->rglt);
	snd_sunxi_pa_pin_exit(pdev, codec->pa_config, codec->pa_pin_max);

	/* sunxi codec custom info free */
	devm_kfree(dev, codec);
	of_node_put(np);

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

module_platform_driver(sunxi_internal_codec_driver);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard codec of internal-codec");
