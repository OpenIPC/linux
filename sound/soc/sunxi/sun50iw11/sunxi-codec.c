/*
 * sound\soc\sunxi\sun50iw11\sunxi-codec.c
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
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/pinctrl/consumer.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/dmaengine_pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/sunxi-gpio.h>
#include <linux/dma/sunxi-dma.h>

#include "sunxi-codec.h"
#include "sunxi-sndcodec.h"

#ifdef CONFIG_SND_SUN50IW11_MAD
#include "sunxi-mad.h"
#endif

#define	DRV_NAME	"sunxi-internal-codec"

/* need optimize */
static int spk_msleep = 120;
module_param(spk_msleep, int, 0644);
MODULE_PARM_DESC(spk_msleep, "SUNXI codec speaker msleep for pop sound.");

static int dac_msleep = 10;
module_param(dac_msleep, int, 0644);
MODULE_PARM_DESC(dac_msleep, "SUNXI codec dac msleep for pop sound.");

static struct label reg_labels[] = {
	LABEL(SUNXI_DAC_DPC),
	LABEL(SUNXI_DAC_VOL_CTL),
	LABEL(SUNXI_DAC_FIFO_CTL),
	LABEL(SUNXI_DAC_FIFO_STA),
	LABEL(SUNXI_DAC_CNT),
	LABEL(SUNXI_DAC_DG),

	LABEL(SUNXI_ADC_FIFO_CTL),
	LABEL(SUNXI_ADC_VOL_CTL1),
	LABEL(SUNXI_ADC_FIFO_STA),
	LABEL(SUNXI_ADC_VOL_CTL2),
	LABEL(SUNXI_ADC_CNT),
	LABEL(SUNXI_ADC_DG),
	LABEL(SUNXI_ADC_DIG_CTL),
	LABEL(SUNXI_VAR1_SPEEDUP_DOWN_CTL),

	/* DAP */
	LABEL(SUNXI_DAC_DAP_CTL),
	LABEL(SUNXI_ADC_DAP_CTL),

	LABEL(SUNXI_ADC1_REG),
	LABEL(SUNXI_ADC2_REG),
	LABEL(SUNXI_ADC3_REG),
	LABEL(SUNXI_ADC4_REG),
	LABEL(SUNXI_DAC_REG),
	LABEL(SUNXI_MICBIAS_REG),
	LABEL(SUNXI_RAMP_REG),
	LABEL(SUNXI_BIAS_REG),
	LABEL(SUNXI_ADC5_REG),
	LABEL(SUNXI_ADC6_REG),

	LABEL_END,
};

static const struct sample_rate sample_rate_conv[] = {
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

static const DECLARE_TLV_DB_SCALE(digital_vol_ctl_tlv, -7424, 116, 0);

static const unsigned int adc_gain_tlv[] = {
	TLV_DB_RANGE_HEAD(3),
	0x0, 0x0, TLV_DB_SCALE_ITEM(0, 0, 0),
	0x1, 0x3, TLV_DB_SCALE_ITEM(600, 0, 0),
	0x4, 0x1F, TLV_DB_SCALE_ITEM(900, 100, 0),
};

static const unsigned int digital_vol_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0x0, 0x0, TLV_DB_SCALE_ITEM(0, 0, 0),
	0x1, 0xFF, TLV_DB_SCALE_ITEM(-11925, 75, 0),
};

static const unsigned int spk_vol_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0x0, 0x1, TLV_DB_SCALE_ITEM(0, 0, 0),
	0x2, 0x1F, TLV_DB_SCALE_ITEM(-4350, 150, 0),
};

static void adcdrc_config(struct snd_soc_codec *codec)
{
	/* Left peak filter attack time */
	snd_soc_write(codec, AC_ADC_DRC_LPFHAT, (0x000B77BF >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LPFLAT, 0x000B77BF & 0xFFFF);
	/* Right peak filter attack time */
	snd_soc_write(codec, AC_ADC_DRC_RPFHAT, (0x000B77BF >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_RPFLAT, 0x000B77BF & 0xFFFF);
	/* Left peak filter release time */
	snd_soc_write(codec, AC_ADC_DRC_LPFHRT, (0x00FFE1F8 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LPFLRT, 0x00FFE1F8 & 0xFFFF);
	/* Right peak filter release time */
	snd_soc_write(codec, AC_ADC_DRC_RPFHRT, (0x00FFE1F8 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_RPFLRT, 0x00FFE1F8 & 0xFFFF);

	/* Left RMS filter attack time */
	snd_soc_write(codec, AC_ADC_DRC_LPFHAT, (0x00012BAF >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LPFLAT, 0x00012BAF & 0xFFFF);
	/* Right RMS filter attack time */
	snd_soc_write(codec, AC_ADC_DRC_RPFHAT, (0x00012BAF >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_RPFLAT, 0x00012BAF & 0xFFFF);

	/* smooth filter attack time */
	snd_soc_write(codec, AC_ADC_DRC_SFHAT, (0x00025600 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_SFLAT, 0x00025600 & 0xFFFF);
	/* gain smooth filter release time */
	snd_soc_write(codec, AC_ADC_DRC_SFHRT, (0x00000F04 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_SFLRT, 0x00000F04 & 0xFFFF);

	/* OPL */
	snd_soc_write(codec, AC_ADC_DRC_HOPL, (0xFBD8FBA7 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LOPL, 0xFBD8FBA7 & 0xFFFF);
	/* OPC */
	snd_soc_write(codec, AC_ADC_DRC_HOPC, (0xF95B2C3F >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LOPC, 0xF95B2C3F & 0xFFFF);
	/* OPE */
	snd_soc_write(codec, AC_ADC_DRC_HOPE, (0xF45F8D6E >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LOPE, 0xF45F8D6E & 0xFFFF);
	/* LT */
	snd_soc_write(codec, AC_ADC_DRC_HLT, (0x01A934F0 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LLT, 0x01A934F0 & 0xFFFF);
	/* CT */
	snd_soc_write(codec, AC_ADC_DRC_HCT, (0x06A4D3C0 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LCT, 0x06A4D3C0 & 0xFFFF);
	/* ET */
	snd_soc_write(codec, AC_ADC_DRC_HET, (0x0BA07291 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LET, 0x0BA07291 & 0xFFFF);
	/* Ki */
	snd_soc_write(codec, AC_ADC_DRC_HKI, (0x00051EB8 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LKI, 0x00051EB8 & 0xFFFF);
	/* Kc */
	snd_soc_write(codec, AC_ADC_DRC_HKC, (0x00800000 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LKC, 0x00800000 & 0xFFFF);
	/* Kn */
	snd_soc_write(codec, AC_ADC_DRC_HKN, (0x01000000 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LKN, 0x01000000 & 0xFFFF);
	/* Ke */
	snd_soc_write(codec, AC_ADC_DRC_HKE, (0x0000F45F >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LKE, 0x0000F45F & 0xFFFF);
}

static void adchpf_config(struct snd_soc_codec *codec)
{
	/* HPF */
	snd_soc_write(codec, AC_ADC_DRC_HHPFC, (0xFFFAC1 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_ADC_DRC_LHPFC, 0xFFE644 & 0xFFFF);
}

static void adcdrc_enable(struct snd_soc_codec *codec, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	if (on) {
		snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
			(0x1 << ADC_DRC0_EN | 0x1 << ADC_DRC1_EN | 0x1 << ADC_DRC2_EN),
			(0x1 << ADC_DRC0_EN | 0x1 << ADC_DRC1_EN | 0x1 << ADC_DRC2_EN));

		if (sunxi_codec->adc_dap_enable++ == 0) {
			snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_DAP0_EN | 0x1 << ADC_DAP1_EN | 0x1 << ADC_DRC2_EN),
				(0x1 << ADC_DAP0_EN | 0x1 << ADC_DAP1_EN | 0x1 << ADC_DRC2_EN));
		}
	} else {
		if (--sunxi_codec->adc_dap_enable == 0) {
			snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_DAP0_EN | 0x1 << ADC_DAP1_EN | 0x1 << ADC_DRC2_EN),
				(0x0 << ADC_DAP0_EN | 0x0 << ADC_DAP1_EN | 0x0 << ADC_DRC2_EN));
		}
		snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
			(0x1 << ADC_DRC0_EN | 0x1 << ADC_DRC1_EN | 0x1 << ADC_DRC2_EN),
			(0x0 << ADC_DRC0_EN | 0x0 << ADC_DRC1_EN | 0x0 << ADC_DRC2_EN));
	}
}

static void adchpf_enable(struct snd_soc_codec *codec, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	if (on) {
		snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
			(0x1 << ADC_HPF0_EN | 0x1 << ADC_HPF1_EN | 0x1 << ADC_HPF2_EN),
			(0x1 << ADC_HPF0_EN | 0x1 << ADC_HPF1_EN | 0x1 << ADC_HPF2_EN));

		if (sunxi_codec->adc_dap_enable++ == 0) {
			snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_DAP0_EN | 0x1 << ADC_DAP1_EN | 0x1 << ADC_DAP2_EN),
				(0x1 << ADC_DAP0_EN | 0x1 << ADC_DAP1_EN | 0x1 << ADC_DAP2_EN));
		}
	} else {
		if (--sunxi_codec->adc_dap_enable == 0) {
			snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_DAP0_EN | 0x1 << ADC_DAP1_EN),
				(0x0 << ADC_DAP0_EN | 0x0 << ADC_DAP1_EN));
		}
		snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
			(0x1 << ADC_HPF0_EN | 0x1 << ADC_HPF1_EN | 0x1 << ADC_HPF2_EN),
			(0x0 << ADC_HPF0_EN | 0x0 << ADC_HPF1_EN | 0x0 << ADC_HPF2_EN));
	}
}

static void dacdrc_config(struct snd_soc_codec *codec)
{
	/* Left peak filter attack time */
	snd_soc_write(codec, AC_DAC_DRC_LPFHAT, (0x000B77BF >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LPFLAT, 0x000B77BF & 0xFFFF);
	/* Right peak filter attack time */
	snd_soc_write(codec, AC_DAC_DRC_RPFHAT, (0x000B77BF >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_RPFLAT, 0x000B77BF & 0xFFFF);
	/* Left peak filter release time */
	snd_soc_write(codec, AC_DAC_DRC_LPFHRT, (0x00FFE1F8 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LPFLRT, 0x00FFE1F8 & 0xFFFF);
	/* Right peak filter release time */
	snd_soc_write(codec, AC_DAC_DRC_RPFHRT, (0x00FFE1F8 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_RPFLRT, 0x00FFE1F8 & 0xFFFF);

	/* Left RMS filter attack time */
	snd_soc_write(codec, AC_DAC_DRC_LPFHAT, (0x00012BAF >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LPFLAT, 0x00012BAF & 0xFFFF);
	/* Right RMS filter attack time */
	snd_soc_write(codec, AC_DAC_DRC_RPFHAT, (0x00012BAF >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_RPFLAT, 0x00012BAF & 0xFFFF);

	/* smooth filter attack time */
	snd_soc_write(codec, AC_DAC_DRC_SFHAT, (0x00025600 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_SFLAT, 0x00025600 & 0xFFFF);
	/* gain smooth filter release time */
	snd_soc_write(codec, AC_DAC_DRC_SFHRT, (0x00000F04 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_SFLRT, 0x00000F04 & 0xFFFF);

	/* OPL */
	snd_soc_write(codec, AC_DAC_DRC_HOPL, (0xFBD8FBA7 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LOPL, 0xFBD8FBA7 & 0xFFFF);
	/* OPC */
	snd_soc_write(codec, AC_DAC_DRC_HOPC, (0xF95B2C3F >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LOPC, 0xF95B2C3F & 0xFFFF);
	/* OPE */
	snd_soc_write(codec, AC_DAC_DRC_HOPE, (0xF45F8D6E >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LOPE, 0xF45F8D6E & 0xFFFF);
	/* LT */
	snd_soc_write(codec, AC_DAC_DRC_HLT, (0x01A934F0 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LLT, 0x01A934F0 & 0xFFFF);
	/* CT */
	snd_soc_write(codec, AC_DAC_DRC_HCT, (0x06A4D3C0 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LCT, 0x06A4D3C0 & 0xFFFF);
	/* ET */
	snd_soc_write(codec, AC_DAC_DRC_HET, (0x0BA07291 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LET, 0x0BA07291 & 0xFFFF);
	/* Ki */
	snd_soc_write(codec, AC_DAC_DRC_HKI, (0x00051EB8 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LKI, 0x00051EB8 & 0xFFFF);
	/* Kc */
	snd_soc_write(codec, AC_DAC_DRC_HKC, (0x00800000 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LKC, 0x00800000 & 0xFFFF);
	/* Kn */
	snd_soc_write(codec, AC_DAC_DRC_HKN, (0x01000000 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LKN, 0x01000000 & 0xFFFF);
	/* Ke */
	snd_soc_write(codec, AC_DAC_DRC_HKE, (0x0000F45F >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LKE, 0x0000F45F & 0xFFFF);
}

static void dachpf_config(struct snd_soc_codec *codec)
{
	/* HPF */
	snd_soc_write(codec, AC_DAC_DRC_HHPFC, (0xFFFAC1 >> 16) & 0xFFFF);
	snd_soc_write(codec, AC_DAC_DRC_LHPFC, 0xFFFAC1 & 0xFFFF);
}

static void dacdrc_enable(struct snd_soc_codec *codec, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	if (on) {
		/* detect noise when ET enable */
		snd_soc_update_bits(codec, AC_DAC_DRC_CTL,
			(0x1 << DAC_DRC_CTL_CONTROL_DRC_EN),
			(0x1 << DAC_DRC_CTL_CONTROL_DRC_EN));
		/* 0x0:RMS filter; 0x1:Peak filter */
		snd_soc_update_bits(codec, AC_DAC_DRC_CTL,
			(0x1 << DAC_DRC_CTL_SIGNAL_FUN_SEL),
			(0x1 << DAC_DRC_CTL_SIGNAL_FUN_SEL));
		/* delay function enable */
		snd_soc_update_bits(codec, AC_DAC_DRC_CTL,
			(0x1 << DAC_DRC_CTL_DEL_FUN_EN),
			(0x0 << DAC_DRC_CTL_DEL_FUN_EN));

		snd_soc_update_bits(codec, AC_DAC_DRC_CTL,
			(0x1 << DAC_DRC_CTL_DRC_LT_EN),
			(0x1 << DAC_DRC_CTL_DRC_LT_EN));
		snd_soc_update_bits(codec, AC_DAC_DRC_CTL,
			(0x1 << DAC_DRC_CTL_DRC_ET_EN),
			(0x1 << DAC_DRC_CTL_DRC_ET_EN));

		snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
			(0x1 << DDAP_DRC_EN),
			(0x1 << DDAP_DRC_EN));

		if (sunxi_codec->dac_dap_enable++ == 0)
			snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_EN),
				(0x1 << DDAP_EN));
	} else {
		if (--sunxi_codec->dac_dap_enable == 0)
			snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_EN),
				(0x0 << DDAP_EN));

		snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
			(0x1 << DDAP_DRC_EN),
			(0x0 << DDAP_DRC_EN));

		/* detect noise when ET enable */
		snd_soc_update_bits(codec, AC_DAC_DRC_CTL,
			(0x1 << DAC_DRC_CTL_CONTROL_DRC_EN),
			(0x0 << DAC_DRC_CTL_CONTROL_DRC_EN));
		/* 0x0:RMS filter; 0x1:Peak filter */
		snd_soc_update_bits(codec, AC_DAC_DRC_CTL,
			(0x1 << DAC_DRC_CTL_SIGNAL_FUN_SEL),
			(0x1 << DAC_DRC_CTL_SIGNAL_FUN_SEL));
		/* delay function enable */
		snd_soc_update_bits(codec, AC_DAC_DRC_CTL,
			(0x1 << DAC_DRC_CTL_DEL_FUN_EN),
			(0x0 << DAC_DRC_CTL_DEL_FUN_EN));

		snd_soc_update_bits(codec, AC_DAC_DRC_CTL,
			(0x1 << DAC_DRC_CTL_DRC_LT_EN),
			(0x0 << DAC_DRC_CTL_DRC_LT_EN));
		snd_soc_update_bits(codec, AC_DAC_DRC_CTL,
			(0x1 << DAC_DRC_CTL_DRC_ET_EN),
			(0x0 << DAC_DRC_CTL_DRC_ET_EN));
	}
}

static void dachpf_enable(struct snd_soc_codec *codec, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	if (on) {
		snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
			(0x1 << DDAP_HPF_EN),
			(0x1 << DDAP_HPF_EN));

		if (sunxi_codec->dac_dap_enable++ == 0)
			snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_EN),
				(0x1 << DDAP_EN));
	} else {
		if (--sunxi_codec->dac_dap_enable == 0)
			snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_EN),
				(0x0 << DDAP_EN));

		snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
			(0x1 << DDAP_HPF_EN),
			(0x0 << DDAP_HPF_EN));
	}
}

static int sunxi_codec_get_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	unsigned int reg_val;

	reg_val = snd_soc_read(codec, SUNXI_DAC_DPC);

	ucontrol->value.integer.value[0] = ((reg_val & (1<<DAC_HUB_EN)) ? 2 : 1);
	return 0;
}

static int sunxi_codec_set_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);

	switch (ucontrol->value.integer.value[0]) {
	case	0:
	case	1:
		snd_soc_update_bits(codec, SUNXI_DAC_DPC,
				(0x1 << DAC_HUB_EN), (0x0 << DAC_HUB_EN));
		break;
	case	2:
		snd_soc_update_bits(codec, SUNXI_DAC_DPC,
				(0x1 << DAC_HUB_EN), (0x1 << DAC_HUB_EN));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/* sunxi codec hub mdoe select */
static const char * const sunxi_codec_hub_function[] = {"null",
			"hub_disable", "hub_enable"};

static const struct soc_enum sunxi_codec_hub_mode_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sunxi_codec_hub_function),
			sunxi_codec_hub_function),
};

static int sunxi_speaker_event(struct snd_soc_dapm_widget *w,
				  struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
		snd_soc_update_bits(codec, SUNXI_DAC_REG,
				(0x1 << LMUTE_EN) | (0x1 << RMUTE_EN),
				(0x1 << LMUTE_EN) | (0x1 << RMUTE_EN));
		snd_soc_update_bits(codec, SUNXI_DAC_REG,
				(0x1 << SPKOUTL_EN) | (0x1 << SPKOUTR_EN),
				(0x1 << SPKOUTL_EN) | (0x1 << SPKOUTR_EN));
		/*  enable ramp manual control */
		snd_soc_update_bits(codec, SUNXI_RAMP_REG,
				(0x1 << RAMP_CTL_EN), (0x1 << RAMP_CTL_EN));

		if (sunxi_codec->pa_power_cfg.used) {
			gpio_set_value(sunxi_codec->pa_power_cfg.gpio, 1);
		}
		if (sunxi_codec->spk_cfg.used) {
			gpio_set_value(sunxi_codec->spk_cfg.gpio,
				sunxi_codec->spk_cfg.pa_ctl_level);
			/*
			 * time delay to wait spk pa work fine,
			 * general setting 160ms. For pop sound,
			 * maybe set less than it.
			 */
			msleep(sunxi_codec->spk_cfg.pa_msleep_time);
		}
		break;
	case	SND_SOC_DAPM_PRE_PMD:
		if (sunxi_codec->pa_power_cfg.used) {
			gpio_set_value(sunxi_codec->pa_power_cfg.gpio, 0);
		}
		if (sunxi_codec->spk_cfg.used) {
			gpio_set_value(sunxi_codec->spk_cfg.gpio,
				!(sunxi_codec->spk_cfg.pa_ctl_level));
		}

		/*  disable ramp manual control */
		snd_soc_update_bits(codec, SUNXI_RAMP_REG,
				(0x1 << RAMP_CTL_EN), (0x0 << RAMP_CTL_EN));

		snd_soc_update_bits(codec, SUNXI_DAC_REG,
				(0x1 << SPKOUTL_EN) | (0x1 << SPKOUTR_EN),
				(0x0 << SPKOUTL_EN) | (0x0 << SPKOUTR_EN));
		snd_soc_update_bits(codec, SUNXI_DAC_REG,
				(0x1 << LMUTE_EN) | (0x1 << RMUTE_EN),
				(0x0 << LMUTE_EN) | (0x0 << RMUTE_EN));
		break;
	default:
		break;
	}
	return 0;
}

static int sunxi_playback_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case	SND_SOC_DAPM_PRE_PMU:
		if (sunxi_codec->dac_enable++ == 0) {
			snd_soc_update_bits(codec, SUNXI_DAC_DPC,
				(0x1 << EN_DAC), (0x1 << EN_DAC));
			/* time delay to wait digital dac work fine */
			msleep(dac_msleep);
		}
		break;
	case	SND_SOC_DAPM_POST_PMD:
		if (--sunxi_codec->dac_enable == 0) {
			snd_soc_update_bits(codec, SUNXI_DAC_DPC,
				(0x1 << EN_DAC), (0x0 << EN_DAC));
		}
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_capture_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
		if (sunxi_codec->adc_enable++ == 0) {
			snd_soc_update_bits(codec, SUNXI_ADC_FIFO_CTL,
				(0x1 << EN_AD), (0x1 << EN_AD));
		}
		break;
	case	SND_SOC_DAPM_POST_PMD:
		if (--sunxi_codec->adc_enable == 0) {
			snd_soc_update_bits(codec, SUNXI_ADC_FIFO_CTL,
				(0x1 << EN_AD), (0x0 << EN_AD));
		}
		break;
	}

	return 0;
}

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	SOC_ENUM_EXT("codec hub mode", sunxi_codec_hub_mode_enum[0],
				sunxi_codec_get_hub_mode,
				sunxi_codec_set_hub_mode),

	SOC_SINGLE_TLV("digital volume", SUNXI_DAC_DPC,
				DVOL, 0x3F, 1, digital_vol_ctl_tlv),
	SOC_SINGLE("DAC digital volume switch", SUNXI_DAC_VOL_CTL,
				DAC_VOL_SEL, 0x1, 0),
	SOC_SINGLE_TLV("DACL digital volume", SUNXI_DAC_VOL_CTL,
				DAC_VOL_L, 0xFF, 0, digital_vol_tlv),
	SOC_SINGLE_TLV("DACR digital volume", SUNXI_DAC_VOL_CTL,
				DAC_VOL_R, 0xFF, 0, digital_vol_tlv),
	SOC_SINGLE_TLV("MIC1 gain volume", SUNXI_ADC1_REG,
				ADC_PGA_GAIN_CTL, 0x1F, 0, adc_gain_tlv),
	SOC_SINGLE_TLV("MIC2 gain volume", SUNXI_ADC2_REG,
				ADC_PGA_GAIN_CTL, 0x1F, 0, adc_gain_tlv),
	SOC_SINGLE_TLV("MIC3 gain volume", SUNXI_ADC3_REG,
				ADC_PGA_GAIN_CTL, 0x1F, 0, adc_gain_tlv),
	SOC_SINGLE_TLV("MIC4 gain volume", SUNXI_ADC4_REG,
				ADC_PGA_GAIN_CTL, 0x1F, 0, adc_gain_tlv),
	SOC_SINGLE_TLV("MIC5 gain volume", SUNXI_ADC5_REG,
				ADC_PGA_GAIN_CTL, 0x1F, 0, adc_gain_tlv),
	SOC_SINGLE_TLV("MIC6 gain volume", SUNXI_ADC6_REG,
				ADC_PGA_GAIN_CTL, 0x1F, 0, adc_gain_tlv),

	SOC_SINGLE("ADC1_2 digital volume switch", SUNXI_ADC_DIG_CTL,
				ADC1_2_VOL_EN, 0x1, 0),
	SOC_SINGLE("ADC3_4 digital volume switch", SUNXI_ADC_DIG_CTL,
				ADC3_4_VOL_EN, 0x1, 0),
	SOC_SINGLE("ADC5_6 digital volume switch", SUNXI_ADC_DIG_CTL,
				ADC5_6_VOL_EN, 0x1, 0),

	SOC_SINGLE_TLV("ADC1 digital volume", SUNXI_ADC_VOL_CTL1,
				ADC1_VOL, 0xFF, 0, digital_vol_tlv),
	SOC_SINGLE_TLV("ADC2 digital volume", SUNXI_ADC_VOL_CTL1,
				ADC2_VOL, 0xFF, 0, digital_vol_tlv),
	SOC_SINGLE_TLV("ADC3 digital volume", SUNXI_ADC_VOL_CTL1,
				ADC3_VOL, 0xFF, 0, digital_vol_tlv),
	SOC_SINGLE_TLV("ADC4 digital volume", SUNXI_ADC_VOL_CTL1,
				ADC4_VOL, 0xFF, 0, digital_vol_tlv),
	SOC_SINGLE_TLV("ADC5 digital volume", SUNXI_ADC_VOL_CTL2,
				ADC5_VOL, 0xFF, 0, digital_vol_tlv),
	SOC_SINGLE_TLV("ADC6 digital volume", SUNXI_ADC_VOL_CTL2,
				ADC6_VOL, 0xFF, 0, digital_vol_tlv),

	SOC_SINGLE_TLV("Speaker volume", SUNXI_DAC_REG,
				SPK_VOL_CTL, 0x1F, 0, spk_vol_tlv),
};

/* SPKL Mux select */
const char * const spk_output_diff_text[] = {
	"Single-End", "Differential",
};

static const struct soc_enum left_spk_output_enum =
	SOC_ENUM_SINGLE(SUNXI_DAC_REG, SPKOUTL_DIFF_EN,
		ARRAY_SIZE(spk_output_diff_text), spk_output_diff_text);

static const struct snd_kcontrol_new left_spk_mux =
	SOC_DAPM_ENUM("Left Speaker Mux", left_spk_output_enum);

/* SPKR Mux select */
static const struct soc_enum right_spk_output_enum =
	SOC_ENUM_SINGLE(SUNXI_DAC_REG, SPKOUTR_DIFF_EN,
		ARRAY_SIZE(spk_output_diff_text), spk_output_diff_text);

static const struct snd_kcontrol_new right_spk_mux =
	SOC_DAPM_ENUM("Right Speaker Mux", right_spk_output_enum);

static const struct snd_kcontrol_new mic1_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Switch", SUNXI_ADC1_REG, MIC_PGA_EN, 1, 0),
};

static const struct snd_kcontrol_new mic2_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC2 Switch", SUNXI_ADC2_REG, MIC_PGA_EN, 1, 0),
};

static const struct snd_kcontrol_new mic3_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC3 Switch", SUNXI_ADC3_REG, MIC_PGA_EN, 1, 0),
};

static const struct snd_kcontrol_new mic4_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC4 Switch", SUNXI_ADC4_REG, MIC_PGA_EN, 1, 0),
};

static const struct snd_kcontrol_new mic5_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC5 Switch", SUNXI_ADC5_REG, MIC_PGA_EN, 1, 0),
};

static const struct snd_kcontrol_new mic6_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC6 Switch", SUNXI_ADC6_REG, MIC_PGA_EN, 1, 0),
};

static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	/* for playback */
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0,
			SUNXI_DAC_REG, DACLEN, 0,
			sunxi_playback_event,
			SND_SOC_DAPM_PRE_PMU| SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("DACR", "Playback", 0,
			SUNXI_DAC_REG, DACREN, 0,
			sunxi_playback_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX("Left Speaker Mux", SND_SOC_NOPM,
			0, 0, &left_spk_mux),
	SND_SOC_DAPM_MUX("Right Speaker Mux", SND_SOC_NOPM,
			0, 0, &right_spk_mux),

	SND_SOC_DAPM_OUTPUT("SPKL"),
	SND_SOC_DAPM_OUTPUT("SPKR"),

	SND_SOC_DAPM_SPK("External Speaker", sunxi_speaker_event),

	/* for capture */
	SND_SOC_DAPM_AIF_OUT_E("ADC1", "Capture", 0,
			SUNXI_ADC1_REG, ADC_EN, 0,
			sunxi_capture_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADC2", "Capture", 0,
			SUNXI_ADC2_REG, ADC_EN, 0,
			sunxi_capture_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADC3", "Capture", 0,
			SUNXI_ADC3_REG, ADC_EN, 0,
			sunxi_capture_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADC4", "Capture", 0,
			SUNXI_ADC4_REG, ADC_EN, 0,
			sunxi_capture_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADC5", "Capture", 0,
			SUNXI_ADC5_REG, ADC_EN, 0,
			sunxi_capture_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("ADC6", "Capture", 0,
			SUNXI_ADC6_REG, ADC_EN, 0,
			sunxi_capture_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MIXER("ADC1 Input", SUNXI_ADC_DIG_CTL, 0, 0,
			mic1_input_mixer, ARRAY_SIZE(mic1_input_mixer)),
	SND_SOC_DAPM_MIXER("ADC2 Input", SUNXI_ADC_DIG_CTL, 1, 0,
			mic2_input_mixer, ARRAY_SIZE(mic2_input_mixer)),
	SND_SOC_DAPM_MIXER("ADC3 Input", SUNXI_ADC_DIG_CTL, 2, 0,
			mic3_input_mixer, ARRAY_SIZE(mic3_input_mixer)),
	SND_SOC_DAPM_MIXER("ADC4 Input", SUNXI_ADC_DIG_CTL, 3, 0,
			mic4_input_mixer, ARRAY_SIZE(mic4_input_mixer)),
	SND_SOC_DAPM_MIXER("ADC5 Input", SUNXI_ADC_DIG_CTL, 4, 0,
			mic5_input_mixer, ARRAY_SIZE(mic5_input_mixer)),
	SND_SOC_DAPM_MIXER("ADC6 Input", SUNXI_ADC_DIG_CTL, 5, 0,
			mic6_input_mixer, ARRAY_SIZE(mic6_input_mixer)),

	SND_SOC_DAPM_MICBIAS("MainMic Bias", SUNXI_MICBIAS_REG,
			MMICBIASEN, 0),

	SND_SOC_DAPM_INPUT("MIC1"),
	SND_SOC_DAPM_INPUT("MIC2"),
	SND_SOC_DAPM_INPUT("MIC3"),
	SND_SOC_DAPM_INPUT("MIC4"),
	SND_SOC_DAPM_INPUT("MIC5"),
	SND_SOC_DAPM_INPUT("MIC6"),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	/* input route */
	{"MIC1", NULL, "MainMic Bias"},
	{"MIC2", NULL, "MainMic Bias"},
	{"MIC3", NULL, "MainMic Bias"},
	{"MIC4", NULL, "MainMic Bias"},
	{"MIC5", NULL, "MainMic Bias"},
	{"MIC6", NULL, "MainMic Bias"},

	{"ADC1 Input", "MIC1 Switch", "MIC1"},
	{"ADC2 Input", "MIC2 Switch", "MIC2"},
	{"ADC3 Input", "MIC3 Switch", "MIC3"},
	{"ADC4 Input", "MIC4 Switch", "MIC4"},
	{"ADC5 Input", "MIC5 Switch", "MIC5"},
	{"ADC6 Input", "MIC6 Switch", "MIC6"},

	{"ADC1", NULL, "ADC1 Input"},
	{"ADC2", NULL, "ADC2 Input"},
	{"ADC3", NULL, "ADC3 Input"},
	{"ADC4", NULL, "ADC4 Input"},
	{"ADC5", NULL, "ADC5 Input"},
	{"ADC6", NULL, "ADC6 Input"},

	/* output route */
	{"Left Speaker Mux", "Single-End", "DACL"},
	{"Right Speaker Mux", "Single-End", "DACR"},

	{"Left Speaker Mux", "Differential", "DACL"},
	{"Right Speaker Mux", "Differential", "DACR"},

	{"SPKL", NULL, "Left Speaker Mux"},
	{"SPKR", NULL, "Right Speaker Mux"},
};

static void sunxi_codec_init(struct snd_soc_codec *codec)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	/* Disable DRC function */
	snd_soc_write(codec, SUNXI_DAC_DAP_CTL, 0);

	/* Disable HPF(high passed filter) */
	snd_soc_update_bits(codec, SUNXI_DAC_DPC,
			(1 << HPF_EN), (0x0 << HPF_EN));

	snd_soc_update_bits(codec, SUNXI_ADC_FIFO_CTL,
			(0xFF << RX_FIFO_TRG_LEVEL), (0x80 << RX_FIFO_TRG_LEVEL));

	/* Enable ADCFDT to overcome niose at the beginning */
	snd_soc_update_bits(codec, SUNXI_ADC_FIFO_CTL,
			(7 << ADCDFEN), (7 << ADCDFEN));

	snd_soc_update_bits(codec, SUNXI_DAC_DPC,
			0x3f << DVOL,
			sunxi_codec->gain_cfg.digital_vol << DVOL);

	/* Not used the ramp func cause there is the MUTE to avoid pop noise */
	snd_soc_update_bits(codec, SUNXI_DAC_REG,
			(0x1 << RSWITCH), (0x1 << RSWITCH));

	snd_soc_update_bits(codec, SUNXI_ADC1_REG,
			0x1F << ADC_PGA_GAIN_CTL,
			sunxi_codec->gain_cfg.mic1gain << ADC_PGA_GAIN_CTL);
	snd_soc_update_bits(codec, SUNXI_ADC2_REG,
			0x1F << ADC_PGA_GAIN_CTL,
			sunxi_codec->gain_cfg.mic1gain << ADC_PGA_GAIN_CTL);
	snd_soc_update_bits(codec, SUNXI_ADC3_REG,
			0x1F << ADC_PGA_GAIN_CTL,
			sunxi_codec->gain_cfg.mic3gain << ADC_PGA_GAIN_CTL);
	snd_soc_update_bits(codec, SUNXI_ADC4_REG,
			0x1F << ADC_PGA_GAIN_CTL,
			sunxi_codec->gain_cfg.mic4gain << ADC_PGA_GAIN_CTL);
	snd_soc_update_bits(codec, SUNXI_ADC5_REG,
			0x1F << ADC_PGA_GAIN_CTL,
			sunxi_codec->gain_cfg.mic5gain << ADC_PGA_GAIN_CTL);
	snd_soc_update_bits(codec, SUNXI_ADC6_REG,
			0x1F << ADC_PGA_GAIN_CTL,
			sunxi_codec->gain_cfg.mic6gain << ADC_PGA_GAIN_CTL);

	snd_soc_update_bits(codec, SUNXI_DAC_REG,
			0x1f << SPK_VOL_CTL,
			sunxi_codec->gain_cfg.spk_vol << SPK_VOL_CTL);

	if (sunxi_codec->hw_cfg.adcdrc_cfg)
		adcdrc_config(codec);

	if (sunxi_codec->hw_cfg.adchpf_cfg)
		adchpf_config(codec);

	if (sunxi_codec->hw_cfg.dacdrc_cfg)
		dacdrc_config(codec);

	if (sunxi_codec->hw_cfg.dachpf_cfg)
		dachpf_config(codec);
}

static int sunxi_codec_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	int i = 0;
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
#ifdef CONFIG_SND_SUN50IW11_MAD
	struct sunxi_mad_priv *mad_priv = sunxi_codec->mad_priv;
#endif

	switch (params_format(params)) {
	case	SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_update_bits(codec, SUNXI_DAC_FIFO_CTL,
				(3 << FIFO_MODE), (3 << FIFO_MODE));
			snd_soc_update_bits(codec, SUNXI_DAC_FIFO_CTL,
				(1 << TX_SAMPLE_BITS), (0 << TX_SAMPLE_BITS));
		} else {
			snd_soc_update_bits(codec, SUNXI_ADC_FIFO_CTL,
				(1 << RX_FIFO_MODE), (1 << RX_FIFO_MODE));
			snd_soc_update_bits(codec, SUNXI_ADC_FIFO_CTL,
				(1 << RX_SAMPLE_BITS), (0 << RX_SAMPLE_BITS));
		}
		break;
	case	SNDRV_PCM_FORMAT_S32_LE:
		/* only for the compatible of tinyalsa */
	case	SNDRV_PCM_FORMAT_S24_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_update_bits(codec, SUNXI_DAC_FIFO_CTL,
				(3 << FIFO_MODE), (0 << FIFO_MODE));
			snd_soc_update_bits(codec, SUNXI_DAC_FIFO_CTL,
				(1 << TX_SAMPLE_BITS), (1 << TX_SAMPLE_BITS));
		} else {
			snd_soc_update_bits(codec, SUNXI_ADC_FIFO_CTL,
				(1 << RX_FIFO_MODE), (0 << RX_FIFO_MODE));
			snd_soc_update_bits(codec, SUNXI_ADC_FIFO_CTL,
				(1 << RX_SAMPLE_BITS), (1 << RX_SAMPLE_BITS));
		}
		break;
	default:
		pr_err("[%s] params_format[%d] error!\n", __func__,
			params_format(params));
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++) {
		if (sample_rate_conv[i].samplerate == params_rate(params)) {
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				snd_soc_update_bits(codec, SUNXI_DAC_FIFO_CTL,
					(0x7 << DAC_FS),
					(sample_rate_conv[i].rate_bit << DAC_FS));
			} else {
				if (sample_rate_conv[i].samplerate > 48000)
					return -EINVAL;
				snd_soc_update_bits(codec, SUNXI_ADC_FIFO_CTL,
					(0x7 << ADC_FS),
					(sample_rate_conv[i].rate_bit << ADC_FS));
			}
		}
	}
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (params_channels(params)) {
		case 1:
			snd_soc_update_bits(codec, SUNXI_DAC_FIFO_CTL,
					(1 << DAC_MONO_EN), 1 << DAC_MONO_EN);
			break;
		case 2:
			snd_soc_update_bits(codec, SUNXI_DAC_FIFO_CTL,
					(1 << DAC_MONO_EN), (0 << DAC_MONO_EN));
			break;
		default:
			pr_warn("[%s] cannot support the channels:%u.\n",
				__func__, params_channels(params));
			return -EINVAL;
		}
	}

	/* MAD hw_params */
#ifdef CONFIG_SND_SUN50IW11_MAD
	/*mad only supported 16k/48KHz samplerate when capturing*/
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (mad_priv->mad_bind == 1) {
			/* mad only receive the high 16bit */
			regmap_update_bits(sunxi_codec->regmap,
					SUNXI_ADC_FIFO_CTL,
					(0x1 << RX_FIFO_MODE),
					(0x1 << RX_FIFO_MODE));
			/* mad config */
			if (params_format(params) != SNDRV_PCM_FORMAT_S16_LE) {
				dev_err(sunxi_codec->dev,
					"unsupported mad sample bits\n");
				return -EINVAL;
			}

			mad_priv->audio_src_chan_num = params_channels(params);
			mad_priv->sample_rate = params_rate(params);
			sunxi_mad_audio_src_chan_num(mad_priv->audio_src_chan_num);
			sunxi_lpsd_chan_sel(mad_priv->lpsd_chan_sel);
			sunxi_mad_standby_chan_sel(mad_priv->mad_standby_chan_sel);
		}
	}
#endif

	return 0;
}

static int sunxi_codec_set_sysclk(struct snd_soc_dai *dai,
			int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct clk *clk_module;
	struct clk *clk_pll_audio;

	switch (freq) {
	case 24576000:
		clk_pll_audio = sunxi_codec->clk_pll_audio0_div2;
		break;
	case 22579200:
		clk_pll_audio = sunxi_codec->clk_pll_audio1;
		freq *= 4;
		if (clk_set_rate(clk_pll_audio, freq)) {
			dev_err(sunxi_codec->dev, "set pll_audio rate failed\n");
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	if (dir == SNDRV_PCM_STREAM_PLAYBACK) {
		clk_module = sunxi_codec->clk_dac;
	} else if (dir == SNDRV_PCM_STREAM_CAPTURE) {
		clk_module = sunxi_codec->clk_adc;
	}

	if (clk_set_parent(clk_module, clk_pll_audio)) {
		dev_err(dai->dev, "set parent of module clk to pll_audio failed\n");
		return -EBUSY;
	}
	if (clk_set_rate(clk_module, freq)) {
		dev_err(sunxi_codec->dev, "set pll_module rate failed\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_codec_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
#ifdef CONFIG_SND_SUN50IW11_MAD
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct sunxi_sndcodec_priv *sndcodec_priv =
				snd_soc_card_get_drvdata(card);
#endif
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (sunxi_codec->hw_cfg.dacdrc_cfg)
			dacdrc_enable(codec, 1);
		if (sunxi_codec->hw_cfg.dachpf_cfg)
			dachpf_enable(codec, 1);
	} else {
		if (sunxi_codec->hw_cfg.adcdrc_cfg)
			adcdrc_enable(codec, 1);

		if (sunxi_codec->hw_cfg.adchpf_cfg)
			adchpf_enable(codec, 1);
#ifdef CONFIG_SND_SUN50IW11_MAD
		sunxi_codec->mad_priv = &(sndcodec_priv->mad_priv);
#endif
	}

	return 0;
}

#ifdef CONFIG_SND_SUN50IW11_MAD
static void sunxi_codec_mad_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct sunxi_mad_priv *mad_priv = sunxi_codec->mad_priv;

	if ((substream->stream == SNDRV_PCM_STREAM_CAPTURE) &&
		(mad_priv->mad_bind == 1)) {
		regmap_update_bits(sunxi_codec->regmap,
				SUNXI_ADC_FIFO_CTL,
				(0x1 << MAD_DATA_EN),
				(0x0 << MAD_DATA_EN));
		/*
		 * pcm_close:
		 * [1] cpu_dai->ops->shutdown;
		 * [2] codec_dai->ops->shutdown;
		 * [3] platform->ops->shutdown;
		 */
		/* diable daudio src*/
		sunxi_mad_audio_source_sel(MAD_PATH_CODEC, false);
		sunxi_codec->capture_en = 0;
		/* should close the ADC_EN */
		regmap_update_bits(sunxi_codec->regmap, SUNXI_ADC_FIFO_CTL,
				(0x1 << EN_AD), (0x0 << EN_AD));

		regmap_update_bits(sunxi_codec->regmap, SUNXI_ADC_DIG_CTL,
				(0xFF << ADC_CHANNEL_EN),
				(0x0 << ADC_CHANNEL_EN));
		sunxi_mad_close();

#ifndef MAD_CLK_ALWAYS_ON
		/*sunxi_mad_sram_set_reset_bit();*/
		if (sunxi_codec->mad_clk_enable) {
			sunxi_mad_module_clk_enable(false);
			sunxi_codec->mad_clk_enable = false;
		}
#endif
	}
}
#endif

static void sunxi_codec_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (sunxi_codec->hw_cfg.dacdrc_cfg)
			dacdrc_enable(codec, 0);
		if (sunxi_codec->hw_cfg.dachpf_cfg)
			dachpf_enable(codec, 0);
	} else {
		if (sunxi_codec->hw_cfg.adcdrc_cfg)
			adcdrc_enable(codec, 0);

		if (sunxi_codec->hw_cfg.adchpf_cfg)
			adchpf_enable(codec, 0);
	}
#ifdef CONFIG_SND_SUN50IW11_MAD
	sunxi_codec_mad_shutdown(substream, dai);
#endif
}

static int sunxi_codec_digital_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

#ifdef CONFIG_SND_SUN50IW11_MAD
static void sunxi_codec_mad_debug_control(struct sunxi_codec_info *sunxi_codec,
					bool enable)
{
	struct sunxi_mad_priv *mad_priv = sunxi_codec->mad_priv;

	if (enable) {
		if (!sunxi_codec->mad_clk_enable) {
			sunxi_mad_module_clk_enable(true);
			sunxi_codec->mad_clk_enable = true;
		}
		sunxi_mad_open();

		sunxi_mad_hw_params(mad_priv->audio_src_chan_num,
					mad_priv->sample_rate);
		sunxi_mad_audio_src_chan_num(mad_priv->audio_src_chan_num);
		sunxi_lpsd_chan_sel(mad_priv->lpsd_chan_sel);
		sunxi_mad_standby_chan_sel(mad_priv->mad_standby_chan_sel);
		sunxi_mad_dma_type(SUNXI_MAD_DMA_IO);
		sunxi_sram_ahb1_threshole_init();
		sunxi_mad_sram_init();

		sunxi_mad_audio_source_sel(MAD_PATH_CODEC, true);
		sunxi_mad_dma_enable(true);
	} else {
		sunxi_mad_close();
		/* regmap_update_bits(sunxi_codec->regmap, SUNXI_ADC_FIFO_CTL, */
		/* (0x1 << EN_AD), (0x0 << EN_AD)); */

		sunxi_mad_dma_enable(false);

		sunxi_mad_sram_set_reset_bit();
		if (sunxi_codec->mad_clk_enable) {
			sunxi_mad_module_clk_enable(false);
			sunxi_codec->mad_clk_enable = false;
		}
	}
}
#endif

static int sunxi_codec_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
#ifdef CONFIG_SND_SUN50IW11_MAD
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct sunxi_mad_priv *mad_priv = sunxi_codec->mad_priv;
#endif

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_update_bits(codec, SUNXI_DAC_FIFO_CTL,
				(1 << FIFO_FLUSH), (1 << FIFO_FLUSH));
		snd_soc_write(codec, SUNXI_DAC_FIFO_STA,
			(1 << DAC_TXE_INT | 1 << DAC_TXU_INT | 1 << DAC_TXO_INT));
		snd_soc_write(codec, SUNXI_DAC_CNT, 0);
	} else {
#ifdef CONFIG_SND_SUN50IW11_MAD
		if (mad_priv->mad_bind && sunxi_codec->capture_en &&
			(substream->runtime->status->state != SNDRV_PCM_STATE_XRUN)) {
			snd_printd("don't need to flush fifo and clear count.\n");
			return 0;
		}

		if (mad_priv->mad_bind == 1) {
			if (sunxi_codec->capture_en) {
				printk("%s!!!!\n", __func__);
				return 0;
			} else {
				sunxi_codec_mad_debug_control(sunxi_codec, false);
		}
	}
#endif
		snd_soc_update_bits(codec, SUNXI_ADC_FIFO_CTL,
			(1 << ADC_FIFO_FLUSH), (1 << ADC_FIFO_FLUSH));
		snd_soc_write(codec, SUNXI_ADC_FIFO_STA,
			(1 << ADC_RXA_INT | 1 << ADC_RXO_INT));
		snd_soc_write(codec, SUNXI_ADC_CNT, 0);
	}

	return 0;
}

#ifdef CONFIG_SND_SUN50IW11_MAD
void sunxi_codec_mad_enter_standby(struct sunxi_codec_info *sunxi_codec)
{
	sunxi_codec->capture_en = 1;

	sunxi_codec_mad_debug_control(sunxi_codec, false);

	regmap_update_bits(sunxi_codec->regmap,
			SUNXI_ADC_FIFO_CTL,
			(1 << EN_AD), (0 << EN_AD));

	regmap_update_bits(sunxi_codec->regmap, SUNXI_ADC_FIFO_CTL,
			(1 << ADC_FIFO_FLUSH), (1 << ADC_FIFO_FLUSH));
	regmap_write(sunxi_codec->regmap, SUNXI_ADC_FIFO_STA,
			(1 << ADC_RXA_INT | 1 << ADC_RXO_INT));
	regmap_write(sunxi_codec->regmap, SUNXI_ADC_CNT, 0);

	regmap_update_bits(sunxi_codec->regmap,
			SUNXI_ADC_FIFO_CTL, (1 << EN_AD), (1 << EN_AD));

	regmap_update_bits(sunxi_codec->regmap, SUNXI_ADC_FIFO_CTL,
			(0x1 << MAD_DATA_EN), (0x1 << MAD_DATA_EN));

	sunxi_codec_mad_debug_control(sunxi_codec, true);
}
EXPORT_SYMBOL_GPL(sunxi_codec_mad_enter_standby);
#endif

#ifdef CONFIG_SND_SUN50IW11_MAD
static void sunxi_codec_adc_mad_enable(struct sunxi_codec_info *sunxi_codec, bool enable)
{
	struct sunxi_mad_priv *mad_priv = sunxi_codec->mad_priv;

	if (enable) {
		/* enable adc */
		if (mad_priv->mad_bind) {
			if (sunxi_codec->capture_en) {
				sunxi_mad_dma_type(SUNXI_MAD_DMA_IO);
				sunxi_mad_dma_enable(true);
			} else {
				regmap_update_bits(sunxi_codec->regmap,
						SUNXI_ADC_FIFO_CTL,
						(0x1 << MAD_DATA_EN),
						(0x1 << MAD_DATA_EN));

				sunxi_codec_mad_debug_control(sunxi_codec, true);

				/* regmap_update_bits(sunxi_codec->regmap, SUNXI_ADC_FIFO_CTL, */
				/* 		(0x1 << EN_AD), (0x1 << EN_AD)); */
				sunxi_codec->capture_en = 1;
			}
		} else {
			regmap_update_bits(sunxi_codec->regmap,
					SUNXI_ADC_FIFO_CTL,
					(1 << ADC_DRQ_EN), (1 << ADC_DRQ_EN));
		}
	} else {
		if (mad_priv->mad_bind) {
			regmap_update_bits(sunxi_codec->regmap,
					SUNXI_ADC_FIFO_CTL,
					(0x1 << MAD_DATA_EN),
					(0x0 << MAD_DATA_EN));

			sunxi_codec_mad_debug_control(sunxi_codec, false);
			sunxi_codec->capture_en = 0;
		} else {
			regmap_update_bits(sunxi_codec->regmap,
					SUNXI_ADC_FIFO_CTL,
					(1 << ADC_DRQ_EN), (0 << ADC_DRQ_EN));
		}
	}
}
#endif

static int sunxi_codec_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
#ifdef CONFIG_SND_SUN50IW11_MAD
	struct sunxi_mad_priv *mad_priv = sunxi_codec->mad_priv;
#endif

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(sunxi_codec->regmap,
					SUNXI_DAC_FIFO_CTL,
					(1 << DAC_DRQ_EN), (1 << DAC_DRQ_EN));
		else {
#ifdef CONFIG_SND_SUN50IW11_MAD
			sunxi_codec_adc_mad_enable(sunxi_codec, true);
#else
			regmap_update_bits(sunxi_codec->regmap,
					SUNXI_ADC_FIFO_CTL,
					(1 << ADC_DRQ_EN), (1 << ADC_DRQ_EN));
#endif
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(sunxi_codec->regmap,
					SUNXI_DAC_FIFO_CTL,
					(1 << DAC_DRQ_EN), (0 << DAC_DRQ_EN));
		else {
#ifdef CONFIG_SND_SUN50IW11_MAD
			sunxi_codec_adc_mad_enable(sunxi_codec, false);
#else
			regmap_update_bits(sunxi_codec->regmap,
					SUNXI_ADC_FIFO_CTL,
					(1 << ADC_DRQ_EN), (0 << ADC_DRQ_EN));
#endif
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct snd_soc_dai_ops sunxi_codec_dai_ops = {
	.startup	= sunxi_codec_startup,
	.hw_params	= sunxi_codec_hw_params,
	.set_sysclk	= sunxi_codec_set_sysclk,
	.prepare	= sunxi_codec_prepare,
	.digital_mute	= sunxi_codec_digital_mute,
	.trigger	= sunxi_codec_trigger,
	.shutdown	= sunxi_codec_shutdown,
};

static struct snd_soc_dai_driver sunxi_codec_dai[] = {
	{
		.name	= "sun50iw11codec",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates	= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 6,
			.rates = SNDRV_PCM_RATE_8000_48000
				| SNDRV_PCM_RATE_KNOT,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &sunxi_codec_dai_ops,
	},
};

static int sunxi_codec_probe(struct snd_soc_codec *codec)
{
	int ret;
	struct snd_soc_dapm_context *dapm = &codec->component.dapm;
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	sunxi_codec->codec = codec;

	ret = snd_soc_add_codec_controls(codec, sunxi_codec_controls,
					ARRAY_SIZE(sunxi_codec_controls));
	if (ret)
		pr_err("failed to register codec controls!\n");

	snd_soc_dapm_new_controls(dapm, sunxi_codec_dapm_widgets,
			ARRAY_SIZE(sunxi_codec_dapm_widgets));
	snd_soc_dapm_add_routes(dapm, sunxi_codec_dapm_routes,
			ARRAY_SIZE(sunxi_codec_dapm_routes));

	sunxi_codec_init(codec);

	sunxi_codec->adc_enable = 0;
	sunxi_codec->dac_enable = 0;
	sunxi_codec->dac_dap_enable = 0;
	sunxi_codec->adc_dap_enable = 0;

	return 0;
}

static int sunxi_codec_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static int sunxi_gpio_iodisable(u32 gpio)
{
	char pin_name[8];
	u32 config, ret;

	sunxi_gpio_to_name(gpio, pin_name);
	config = 7 << 16;
	ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
	return ret;
}

static int save_audio_reg(struct sunxi_codec_info *sunxi_codec)
{
	int i = 0;

	while (reg_labels[i].name != NULL) {
		reg_labels[i].value = readl(sunxi_codec->addr_base +
						reg_labels[i].address);
		i++;
	}

	return i;
}

static int echo_audio_reg(struct sunxi_codec_info *sunxi_codec)
{
	int i = 0;

	while (reg_labels[i].name != NULL) {
		snd_soc_write(sunxi_codec->codec, reg_labels[i].address,
				reg_labels[i].value);
		i++;
	}

	return i;
}

static int sunxi_codec_suspend(struct snd_soc_codec *codec)
{
	struct sunxi_codec_info *sunxi_codec =
			snd_soc_codec_get_drvdata(codec);

	pr_debug("Enter %s\n", __func__);

	/* save the audio reg */
	save_audio_reg(sunxi_codec);

	if (sunxi_codec->pa_power_cfg.used) {
		gpio_set_value(sunxi_codec->pa_power_cfg.gpio, 0);
		sunxi_gpio_iodisable(sunxi_codec->pa_power_cfg.gpio);
	}
	if (sunxi_codec->spk_cfg.used) {
		gpio_set_value(sunxi_codec->spk_cfg.gpio,
			!(sunxi_codec->spk_cfg.pa_ctl_level));
		sunxi_gpio_iodisable(sunxi_codec->spk_cfg.gpio);
	}

	clk_disable_unprepare(sunxi_codec->clk_pll_audio1);
	clk_disable_unprepare(sunxi_codec->clk_adc);
	clk_disable_unprepare(sunxi_codec->clk_dac);

	pr_debug("End %s\n", __func__);

	return 0;
}

static int sunxi_codec_resume(struct snd_soc_codec *codec)
{
	struct sunxi_codec_info *sunxi_codec =
			snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	pr_debug("Enter %s\n", __func__);

	if (clk_prepare_enable(sunxi_codec->clk_pll_audio1)) {
		dev_err(sunxi_codec->dev, "enable pll_audio1 failed, resume exit\n");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->clk_adc)) {
		dev_err(sunxi_codec->dev, "enable clk_adc failed, resume exit\n");
		ret = -EBUSY;
		goto err_enable_clk_adc;
	}
	if (clk_prepare_enable(sunxi_codec->clk_dac)) {
		dev_err(sunxi_codec->dev, "enable clk_dac failed, resume exit\n");
		ret = -EBUSY;
		goto err_enable_clk_dac;
	}

	/* for stable the power and clk */
	msleep(20);

	if (sunxi_codec->pa_power_cfg.used) {
		gpio_direction_output(sunxi_codec->pa_power_cfg.gpio, 1);
		gpio_set_value(sunxi_codec->pa_power_cfg.gpio, 1);
	}
	if (sunxi_codec->spk_cfg.used) {
		gpio_direction_output(sunxi_codec->spk_cfg.gpio, 1);
		gpio_set_value(sunxi_codec->spk_cfg.gpio,
				!sunxi_codec->spk_cfg.pa_ctl_level);
	}

	sunxi_codec_init(codec);
	/* echo the reg that had saved */
	echo_audio_reg(sunxi_codec);

	pr_debug("End %s\n", __func__);

	return 0;

err_enable_clk_dac:
	clk_disable_unprepare(sunxi_codec->clk_adc);
err_enable_clk_adc:
	clk_disable_unprepare(sunxi_codec->clk_pll_audio1);
	return ret;
}

static unsigned int sunxi_codec_read(struct snd_soc_codec *codec,
					unsigned int reg)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	unsigned int reg_val;

	regmap_read(sunxi_codec->regmap, reg, &reg_val);

	return reg_val;
}

static int sunxi_codec_write(struct snd_soc_codec *codec,
				unsigned int reg, unsigned int val)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	regmap_write(sunxi_codec->regmap, reg, val);

	return 0;
};

static struct snd_soc_codec_driver soc_codec_dev_sunxi = {
	.probe = sunxi_codec_probe,
	.remove = sunxi_codec_remove,
	.suspend = sunxi_codec_suspend,
	.resume = sunxi_codec_resume,
	.read = sunxi_codec_read,
	.write = sunxi_codec_write,
	.ignore_pmdown_time = 1,
};

static ssize_t show_audio_reg(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sunxi_codec_info *sunxi_codec = dev_get_drvdata(dev);
	int count = 0, i = 0;
	unsigned int reg_val;

	count += sprintf(buf, "dump audio reg:\n");

	while (reg_labels[i].name != NULL) {
		reg_val = snd_soc_read(sunxi_codec->codec, reg_labels[i].address);
		count += sprintf(buf + count, "%-20s[0x%03x]: 0x%-10x  Save:0x%x\n",
			reg_labels[i].name, reg_labels[i].address,
			reg_val, reg_labels[i].value);
		i++;
	}

	return count;
}

/* ex:
 * param 1: 0 read;1 write
 * param 2: reg value;
 * param 3: write value;
 *	read:
 *		echo 0,0x00> audio_reg
 *		echo 0,0x00> audio_reg
 *	write:
 *		echo 1,0x00,0xa > audio_reg
 *		echo 1,0x00,0xff > audio_reg
 */
static ssize_t store_audio_reg(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	int rw_flag;
	int input_reg_val = 0;
	int input_reg_offset = 0;
	struct sunxi_codec_info *sunxi_codec = dev_get_drvdata(dev);

	ret = sscanf(buf, "%d,0x%x,0x%x", &rw_flag,
			&input_reg_offset, &input_reg_val);
	dev_info(dev, "ret:%d, reg_offset:%d, reg_val:0x%x\n",
			ret, input_reg_offset, input_reg_val);

	if (!(rw_flag == 1 || rw_flag == 0)) {
		pr_err("not rw_flag\n");
		ret = count;
		goto out;
	}
	if (rw_flag) {
		regmap_write(sunxi_codec->regmap,
					input_reg_offset, input_reg_val);
	} else {
		regmap_read(sunxi_codec->regmap,
					input_reg_offset, &input_reg_val);
		dev_info(dev, "\n\n Reg[0x%x] : 0x%08x\n\n",
					input_reg_offset, input_reg_val);
	}
	ret = count;

out:
	return ret;
}

static DEVICE_ATTR(audio_reg, 0644, show_audio_reg, store_audio_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_audio_reg.attr,
	NULL,
};

static struct attribute_group audio_debug_attr_group = {
	.name   = "audio_reg_debug",
	.attrs  = audio_debug_attrs,
};

static struct regmap_config sunxi_codec_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_ADC6_REG,
	.cache_type = REGCACHE_NONE,
};

static int  sunxi_codec_dev_probe(struct platform_device *pdev)
{
	struct sunxi_codec_info *sunxi_codec;
	struct device_node *np = pdev->dev.of_node;
	struct gpio_config config_gpio;
	unsigned int temp_val;
	int ret;

	sunxi_codec = devm_kzalloc(&pdev->dev,
				sizeof(struct sunxi_codec_info), GFP_KERNEL);
	if (!sunxi_codec) {
		dev_err(&pdev->dev, "Can't allocate sunxi codec memory\n");
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}
	dev_set_drvdata(&pdev->dev, sunxi_codec);
	sunxi_codec->dev = &pdev->dev;

	ret = of_address_to_resource(np, 0, &(sunxi_codec->res));

	sunxi_codec->clk_pll_audio0_div2 = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(sunxi_codec->clk_pll_audio0_div2)) {
		dev_err(&pdev->dev, "pll_audio0_div2 not exist or invaild\n");
		ret = PTR_ERR(sunxi_codec->clk_pll_audio0_div2);
		goto err_get_pll_audio0_div2;
	}

	sunxi_codec->clk_pll_audio1 = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(sunxi_codec->clk_pll_audio1)) {
		dev_err(&pdev->dev, "pll_audio1 not exist or invaild\n");
		ret = PTR_ERR(sunxi_codec->clk_pll_audio1);
		goto err_get_pll_audio1;
	}

	sunxi_codec->clk_adc = of_clk_get(np, 2);
	if (IS_ERR_OR_NULL(sunxi_codec->clk_adc)) {
		dev_err(&pdev->dev, "adc clk  not exist or invaild\n");
		ret = PTR_ERR(sunxi_codec->clk_adc);
		goto err_get_adc_clk;
	}
	sunxi_codec->clk_dac = of_clk_get(np, 3);
	if (IS_ERR_OR_NULL(sunxi_codec->clk_dac)) {
		dev_err(&pdev->dev, "dac clk  not exist or invaild\n");
		ret = PTR_ERR(sunxi_codec->clk_dac);
		goto err_get_dac_clk;
	}

	sunxi_codec->clk_pll_audio0_div5 = of_clk_get(np, 4);
	if (IS_ERR_OR_NULL(sunxi_codec->clk_pll_audio0_div5)) {
		dev_err(&pdev->dev, "pll_audio_div5 clk  not exist or invaild\n");
		ret = PTR_ERR(sunxi_codec->clk_pll_audio0_div5);
		goto err_get_pll_audio0_div5;
	}
	if (clk_prepare_enable(sunxi_codec->clk_pll_audio0_div2)) {
		dev_err(&pdev->dev, "pll_audio0_div2 enable failed\n");
		ret = -EBUSY;
		goto err_enable_pll_audio0_div2;
	}
	if (clk_prepare_enable(sunxi_codec->clk_pll_audio1)) {
		dev_err(&pdev->dev, "pll_audio1 enable failed\n");
		ret = -EBUSY;
		goto err_enable_pll_audio1;
	}

	if (clk_prepare_enable(sunxi_codec->clk_adc)) {
		dev_err(&pdev->dev, "adc clk enable failed\n");
		ret = -EBUSY;
		goto err_enable_adc_clk;
	}

	if (clk_prepare_enable(sunxi_codec->clk_dac)) {
		dev_err(&pdev->dev, "dac clk enable failed\n");
		ret = -EBUSY;
		goto err_enable_dac_clk;
	}

	if (clk_prepare_enable(sunxi_codec->clk_pll_audio0_div5)) {
		dev_err(&pdev->dev, "pll_audio0_div5 enable failed\n");
		ret = -EBUSY;
		goto err_enable_pll_audio0_div5;
	}
	/* codec reg_base */
	sunxi_codec->addr_base = of_iomap(np, 0);
	if (sunxi_codec->addr_base == NULL) {
		dev_err(&pdev->dev, "digital register iomap failed\n");
		ret = -EINVAL;
		goto err_get_addr_base;
	}

	sunxi_codec_regmap_config.max_register = resource_size(&sunxi_codec->res);
	sunxi_codec->regmap = devm_regmap_init_mmio(&pdev->dev,
				sunxi_codec->addr_base, &sunxi_codec_regmap_config);
	if (IS_ERR(sunxi_codec->regmap)) {
		dev_err(&pdev->dev, "regmap init failed\n");
		ret = PTR_ERR(sunxi_codec->regmap);
		goto err_devm_regmap_init;
	}

	ret = of_property_read_u32(np, "digital_vol", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "digital_vol get failed\n");
		sunxi_codec->gain_cfg.digital_vol = 0;
	} else {
		sunxi_codec->gain_cfg.digital_vol = temp_val;
	}

	ret = of_property_read_u32(np, "spk_vol", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "speaker volume get failed\n");
		sunxi_codec->gain_cfg.spk_vol = 0x1a;
	} else {
		sunxi_codec->gain_cfg.spk_vol = temp_val;
	}

	ret = of_property_read_u32(np, "mic1gain", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "mic1 gain get failed\n");
		sunxi_codec->gain_cfg.mic1gain = 0x1D;
	} else {
		sunxi_codec->gain_cfg.mic1gain = temp_val;
	}

	ret = of_property_read_u32(np, "mic2gain", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "mic2 gain get failed\n");
		sunxi_codec->gain_cfg.mic2gain = 0x1D;
	} else {
		sunxi_codec->gain_cfg.mic2gain = temp_val;
	}

	ret = of_property_read_u32(np, "mic3gain", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "mic3 gain get failed\n");
		sunxi_codec->gain_cfg.mic3gain = 0x1D;
	} else {
		sunxi_codec->gain_cfg.mic3gain = temp_val;
	}

	ret = of_property_read_u32(np, "mic4gain", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "mic4 gain get failed\n");
		sunxi_codec->gain_cfg.mic4gain = 0x1D;
	} else {
		sunxi_codec->gain_cfg.mic4gain = temp_val;
	}

	ret = of_property_read_u32(np, "mic5gain", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "mic5 gain get failed\n");
		sunxi_codec->gain_cfg.mic5gain = 0x1D;
	} else {
		sunxi_codec->gain_cfg.mic5gain = temp_val;
	}

	ret = of_property_read_u32(np, "mic6gain", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "mic6 gain get failed\n");
		sunxi_codec->gain_cfg.mic6gain = 0x1D;
	} else {
		sunxi_codec->gain_cfg.mic6gain = temp_val;
	}

	ret = of_property_read_u32(np, "pa_msleep_time", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "pa_msleep_time get failed\n");
		sunxi_codec->spk_cfg.pa_msleep_time = 160;
	} else {
		sunxi_codec->spk_cfg.pa_msleep_time = temp_val;
	}

	ret = of_property_read_u32(np, "pa_ctl_level", &temp_val);
	if (ret < 0) {
		pr_err("[audio-codec] pa_clt_level config missing or invalid.\n");
		sunxi_codec->spk_cfg.pa_ctl_level = 1;
	} else {
		sunxi_codec->spk_cfg.pa_ctl_level = temp_val;
	}

	ret = of_property_read_u32(np, "adcdrc_cfg", &temp_val);
	if (ret < 0) {
		pr_err("[audio-codec] adcdrc_cfg config missing or invalid.\n");
		sunxi_codec->hw_cfg.adcdrc_cfg = 0;
	} else {
		sunxi_codec->hw_cfg.adcdrc_cfg = temp_val;
	}

	ret = of_property_read_u32(np, "adchpf_cfg", &temp_val);
	if (ret < 0) {
		pr_err("[audio-codec] adchpf_cfg config missing or invalid.\n");
		sunxi_codec->hw_cfg.adchpf_cfg = 0;
	} else {
		sunxi_codec->hw_cfg.adchpf_cfg = temp_val;
	}

	ret = of_property_read_u32(np, "dacdrc_cfg", &temp_val);
	if (ret < 0) {
		pr_err("[audio-codec] dacdrc_cfg config missing or invalid.\n");
		sunxi_codec->hw_cfg.dacdrc_cfg = 0;
	} else {
		sunxi_codec->hw_cfg.dacdrc_cfg = temp_val;
	}

	ret = of_property_read_u32(np, "dachpf_cfg", &temp_val);
	if (ret < 0) {
		pr_err("[audio-codec] dachpf_cfg config missing or invalid.\n");
		sunxi_codec->hw_cfg.dachpf_cfg = 0;
	} else {
		sunxi_codec->hw_cfg.dachpf_cfg = temp_val;
	}

	sunxi_codec->spk_cfg.gpio = of_get_named_gpio_flags(np, "gpio-spk", 0,
				       (enum of_gpio_flags *)&config_gpio);
	if (gpio_is_valid(sunxi_codec->spk_cfg.gpio)) {
		sunxi_codec->spk_cfg.used = 1;
		ret = devm_gpio_request(&pdev->dev,
					sunxi_codec->spk_cfg.gpio, "SPK");
		if (ret) {
			dev_err(&pdev->dev, "failed to request gpio-spk gpio\n");
			ret = -EBUSY;
			goto err_devm_gpio_request_pa_mute;
		} else {
			gpio_direction_output(sunxi_codec->spk_cfg.gpio, 1);
			gpio_set_value(sunxi_codec->spk_cfg.gpio,
				!(sunxi_codec->spk_cfg.pa_ctl_level));
		}
	} else {
		sunxi_codec->spk_cfg.used = 0;
		dev_err(&pdev->dev, "gpio-spk is invalid!\n");
	}

	sunxi_codec->pa_power_cfg.gpio =
				of_get_named_gpio_flags(np, "gpio-pa-power", 0,
					(enum of_gpio_flags *)&config_gpio);
	if (gpio_is_valid(sunxi_codec->pa_power_cfg.gpio)) {
		sunxi_codec->pa_power_cfg.used = 1;
		ret = devm_gpio_request(&pdev->dev,
				sunxi_codec->pa_power_cfg.gpio, "PA Power");
		if (ret) {
			dev_err(&pdev->dev, "failed to request gpio-pa-power\n");
			ret = -EBUSY;
			sunxi_codec->pa_power_cfg.used = 0;
			goto err_devm_gpio_request_pa_power;
		} else {
			gpio_direction_output(sunxi_codec->pa_power_cfg.gpio, 1);
			gpio_set_value(sunxi_codec->pa_power_cfg.gpio, 0);
		}
	} else {
		sunxi_codec->pa_power_cfg.used = 0;
		pr_debug("gpio-pa-power is invalid!\n");
	}

	ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_sunxi,
				sunxi_codec_dai, ARRAY_SIZE(sunxi_codec_dai));
	if (ret < 0) {
		dev_err(&pdev->dev, "register codec failed\n");
		ret = -EBUSY;
		goto err_register_codec;
	}

	ret  = sysfs_create_group(&pdev->dev.kobj, &audio_debug_attr_group);
	if (ret < 0) {
		dev_warn(&pdev->dev, "failed to create attr group\n");
		goto err_sysfs_create;
	}

	return 0;

err_sysfs_create:
	snd_soc_unregister_codec(&pdev->dev);
err_register_codec:
	if (sunxi_codec->pa_power_cfg.used)
		devm_gpio_free(&pdev->dev, sunxi_codec->pa_power_cfg.gpio);
err_devm_gpio_request_pa_power:
	if (sunxi_codec->spk_cfg.used)
		devm_gpio_free(&pdev->dev, sunxi_codec->spk_cfg.gpio);
err_devm_gpio_request_pa_mute:
err_devm_regmap_init:
	iounmap(sunxi_codec->addr_base);
err_get_addr_base:
	clk_disable_unprepare(sunxi_codec->clk_pll_audio0_div5);
err_enable_pll_audio0_div5:
	clk_disable_unprepare(sunxi_codec->clk_dac);
err_enable_dac_clk:
	clk_disable_unprepare(sunxi_codec->clk_adc);
err_enable_adc_clk:
	clk_disable_unprepare(sunxi_codec->clk_pll_audio1);
err_enable_pll_audio1:
	clk_disable_unprepare(sunxi_codec->clk_pll_audio0_div2);
err_enable_pll_audio0_div2:
	clk_put(sunxi_codec->clk_pll_audio0_div5);
err_get_pll_audio0_div5:
	clk_put(sunxi_codec->clk_dac);
err_get_dac_clk:
	clk_put(sunxi_codec->clk_adc);
err_get_adc_clk:
	clk_put(sunxi_codec->clk_pll_audio1);
err_get_pll_audio1:
	clk_put(sunxi_codec->clk_pll_audio0_div2);
err_get_pll_audio0_div2:
	devm_kfree(&pdev->dev, sunxi_codec);
err_devm_kzalloc:
	of_node_put(np);
	return ret;
}

static int  __exit sunxi_codec_dev_remove(struct platform_device *pdev)
{
	struct sunxi_codec_info *sunxi_codec = dev_get_drvdata(&pdev->dev);
	struct device_node *np = pdev->dev.of_node;

	snd_soc_unregister_codec(&pdev->dev);

	iounmap(sunxi_codec->addr_base);

	clk_disable_unprepare(sunxi_codec->clk_adc);
	clk_put(sunxi_codec->clk_adc);
	clk_disable_unprepare(sunxi_codec->clk_dac);
	clk_put(sunxi_codec->clk_dac);

	clk_disable_unprepare(sunxi_codec->clk_pll_audio0_div2);
	clk_put(sunxi_codec->clk_pll_audio0_div2);
	clk_disable_unprepare(sunxi_codec->clk_pll_audio1);
	clk_put(sunxi_codec->clk_pll_audio1);

	if (sunxi_codec->pa_power_cfg.used)
		devm_gpio_free(&pdev->dev, sunxi_codec->pa_power_cfg.gpio);
	if (sunxi_codec->spk_cfg.used)
		devm_gpio_free(&pdev->dev, sunxi_codec->spk_cfg.gpio);

	sysfs_remove_group(&pdev->dev.kobj, &audio_debug_attr_group);

	devm_kfree(&pdev->dev, sunxi_codec);
	of_node_put(np);

	return 0;
}

static const struct of_device_id sunxi_internal_codec_of_match[] = {
	{ .compatible = "allwinner,sunxi-internal-codec", },
	{},
};

static struct platform_driver sunxi_internal_codec_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_internal_codec_of_match,
	},
	.probe = sunxi_codec_dev_probe,
	.remove = __exit_p(sunxi_codec_dev_remove),
};
module_platform_driver(sunxi_internal_codec_driver);

MODULE_DESCRIPTION("SUNXI Codec ASoC driver");
MODULE_AUTHOR("yumingfeng <yumingfeng@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-internal-codec");
