/*
 * sound\soc\sunxi\sun50iw10-codec.c
 * (C) Copyright 2014-2019
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 * luguofang <luguofang@allwinnertech.com>
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
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/dma/sunxi-dma.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/sunxi-gpio.h>

#include "sunxi_rw_func.h"
#include "sun50iw10-codec.h"

/* digital audio process function */
enum sunxi_hw_dap {
	DAP_HP_EN = 0x1,
	DAP_SPK_EN = 0x2,
	/* DAP_HP_EN | DAP_SPK_EN */
	DAP_HPSPK_EN = 0x3,
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

static const DECLARE_TLV_DB_SCALE(digital_tlv, -7424, 116, 0);
static const DECLARE_TLV_DB_SCALE(headphone_gain_tlv, -4200, 600, 0);
static const DECLARE_TLV_DB_SCALE(mic1gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(mic2gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -11925, 75, 0);
static const unsigned int lineout_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 1, TLV_DB_SCALE_ITEM(0, 0, 1),
	2, 31, TLV_DB_SCALE_ITEM(-4350, 150, 1),
};

static struct reg_label reg_labels[] = {
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
#ifdef SUNXI_CODEC_DAP_ENABLE
	REG_LABEL(SUNXI_DAC_DAP_CTL),
	REG_LABEL(SUNXI_ADC_DAP_CTL),
#endif
	REG_LABEL(SUNXI_ADCL_REG),
	REG_LABEL(SUNXI_ADCR_REG),
	REG_LABEL(SUNXI_DAC_REG),
	REG_LABEL(SUNXI_MICBIAS_REG),
	REG_LABEL(SUNXI_BIAS_REG),
	REG_LABEL(SUNXI_HEADPHONE_REG),
	REG_LABEL(SUNXI_HMIC_CTRL),
	REG_LABEL(SUNXI_HMIC_STS),
	REG_LABEL_END,
};

#ifdef SUNXI_CODEC_DAP_ENABLE
static void adcdrc_config(struct snd_soc_codec *codec)
{
	/* Enable DRC gain Min and Max limit, detect noise, Using Peak Filter */
	snd_soc_update_bits(codec, SUNXI_ADC_DRC_CTRL,
		((0x1 << ADC_DRC_DELAY_BUF_EN) |
		(0x1 << ADC_DRC_GAIN_MAX_EN) | (0x1 << ADC_DRC_GAIN_MIN_EN) |
		(0x1 << ADC_DRC_NOISE_DET_EN) |
		(0x1 << ADC_DRC_SIGNAL_SEL) |
		(0x1 << ADC_DRC_LT_EN) | (0x1 << ADC_DRC_ET_EN)),
		((0x1 << ADC_DRC_DELAY_BUF_EN) |
		(0x1 << ADC_DRC_GAIN_MAX_EN) | (0x1 << ADC_DRC_GAIN_MIN_EN) |
		(0x1 << ADC_DRC_NOISE_DET_EN) |
		(0x1 << ADC_DRC_SIGNAL_SEL) |
		(0x1 << ADC_DRC_LT_EN) | (0x1 << ADC_DRC_ET_EN)));

	/* Left peak filter attack time */
	snd_soc_write(codec, SUNXI_ADC_DRC_LPFHAT, (0x000B77F0 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LPFLAT, 0x000B77F0 & 0xFFFF);
	/* Right peak filter attack time */
	snd_soc_write(codec, SUNXI_ADC_DRC_RPFHAT, (0x000B77F0 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_RPFLAT, 0x000B77F0 & 0xFFFF);
	/* Left peak filter release time */
	snd_soc_write(codec, SUNXI_ADC_DRC_LPFHRT, (0x00FFE1F8 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LPFLRT, 0x00FFE1F8 & 0xFFFF);
	/* Right peak filter release time */
	snd_soc_write(codec, SUNXI_ADC_DRC_RPFHRT, (0x00FFE1F8 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_RPFLRT, 0x00FFE1F8 & 0xFFFF);

	/* Left RMS filter attack time */
	snd_soc_write(codec, SUNXI_ADC_DRC_LPFHAT, (0x00012BB0 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LPFLAT, 0x00012BB0 & 0xFFFF);
	/* Right RMS filter attack time */
	snd_soc_write(codec, SUNXI_ADC_DRC_RPFHAT, (0x00012BB0 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_RPFLAT, 0x00012BB0 & 0xFFFF);

	/* OPL */
	snd_soc_write(codec, SUNXI_ADC_DRC_HOPL, (0xFF641741 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LOPL, 0xFF641741 & 0xFFFF);
	/* OPC */
	snd_soc_write(codec, SUNXI_ADC_DRC_HOPC, (0xFC0380F3 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LOPC, 0xFC0380F3 & 0xFFFF);
	/* OPE */
	snd_soc_write(codec, SUNXI_ADC_DRC_HOPE, (0xF608C25F >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LOPE, 0xF608C25F & 0xFFFF);
	/* LT */
	snd_soc_write(codec, SUNXI_ADC_DRC_HLT, (0x035269E0 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LLT, 0x035269E0 & 0xFFFF);
	/* CT */
	snd_soc_write(codec, SUNXI_ADC_DRC_HCT, (0x06B3002C >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LCT, 0x06B3002C & 0xFFFF);
	/* ET */
	snd_soc_write(codec, SUNXI_ADC_DRC_HET, (0x0A139682 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LET, 0x0A139682 & 0xFFFF);
	/* Ki */
	snd_soc_write(codec, SUNXI_ADC_DRC_HKI, (0x00222222 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LKI, 0x00222222 & 0xFFFF);
	/* Kc */
	snd_soc_write(codec, SUNXI_ADC_DRC_HKC, (0x01000000 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LKC, 0x01000000 & 0xFFFF);
	/* Kn */
	snd_soc_write(codec, SUNXI_ADC_DRC_HKN, (0x01C53EF0 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LKN, 0x01C53EF0 & 0xFFFF);
	/* Ke */
	snd_soc_write(codec, SUNXI_ADC_DRC_HKE, (0x04234F68 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LKE, 0x04234F68 & 0xFFFF);

	/* smooth filter attack time */
	snd_soc_write(codec, SUNXI_ADC_DRC_SFHAT, (0x00017665 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_SFLAT, 0x00017665 & 0xFFFF);
	/* gain smooth filter release time */
	snd_soc_write(codec, SUNXI_ADC_DRC_SFHRT, (0x00000F04 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_SFLRT, 0x00000F04 & 0xFFFF);

	/* gain max setting */
	snd_soc_write(codec, SUNXI_ADC_DRC_MXGHS, (0x69E0F95B >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_MXGLS, 0x69E0F95B & 0xFFFF);

	/* gain min setting */
	snd_soc_write(codec, SUNXI_ADC_DRC_MNGHS, (0xF95B2C3F >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_MNGLS, 0xF95B2C3F & 0xFFFF);

	/* smooth filter release and attack time */
	snd_soc_write(codec, SUNXI_ADC_DRC_EPSHC, (0x00025600 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_EPSHC, 0x00025600 & 0xFFFF);
}

static void adcdrc_enable(struct snd_soc_codec *codec, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct codec_dap *adc_dap = &sunxi_codec->adc_dap;

	if (on) {
		if (adc_dap->drc_enable++ == 0) {
			snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_DRC0_EN), (0x1 << ADC_DRC0_EN));
			if (sunxi_codec->adc_dap_enable++ == 0) {
				snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP0_EN), (0x1 << ADC_DAP0_EN));
			}
		}
	} else {
		if (--adc_dap->drc_enable <= 0) {
			adc_dap->drc_enable = 0;
			if (--sunxi_codec->adc_dap_enable <= 0) {
				sunxi_codec->adc_dap_enable = 0;
				snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP0_EN), (0x0 << ADC_DAP0_EN));
			}
			snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_DRC0_EN), (0x0 << ADC_DRC0_EN));
		}
	}
}

static void adchpf_config(struct snd_soc_codec *codec)
{
	/* HPF */
	snd_soc_write(codec, SUNXI_ADC_DRC_HHPFC, (0xFFFAC1 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_ADC_DRC_LHPFC, 0xFFFAC1 & 0xFFFF);
}

static void adchpf_enable(struct snd_soc_codec *codec, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct codec_dap *adc_dap = &sunxi_codec->adc_dap;

	if (on) {
		if (adc_dap->hpf_enable++ == 0) {
			snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_HPF0_EN), (0x1 << ADC_HPF0_EN));
			if (sunxi_codec->adc_dap_enable++ == 0) {
				snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP0_EN), (0x1 << ADC_DAP0_EN));
			}
		}
	} else {
		if (--adc_dap->hpf_enable <= 0) {
			adc_dap->hpf_enable = 0;
			if (--sunxi_codec->adc_dap_enable <= 0) {
				sunxi_codec->adc_dap_enable = 0;
				snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
					(0x1 << ADC_DAP0_EN), (0x0 << ADC_DAP0_EN));
			}
			snd_soc_update_bits(codec, SUNXI_ADC_DAP_CTL,
				(0x1 << ADC_HPF0_EN), (0x0 << ADC_HPF0_EN));
		}
	}
}

static void dacdrc_config(struct snd_soc_codec *codec)
{
	/* Left peak filter attack time */
	snd_soc_write(codec, SUNXI_DAC_DRC_LPFHAT, (0x000B77BF >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LPFLAT, 0x000B77BF & 0xFFFF);
	/* Right peak filter attack time */
	snd_soc_write(codec, SUNXI_DAC_DRC_RPFHAT, (0x000B77F0 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_RPFLAT, 0x000B77F0 & 0xFFFF);

	/* Left peak filter release time */
	snd_soc_write(codec, SUNXI_DAC_DRC_LPFHRT, (0x00FFE1F8 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LPFLRT, 0x00FFE1F8 & 0xFFFF);
	/* Right peak filter release time */
	snd_soc_write(codec, SUNXI_DAC_DRC_RPFHRT, (0x00FFE1F8 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_RPFLRT, 0x00FFE1F8 & 0xFFFF);

	/* Left RMS filter attack time */
	snd_soc_write(codec, SUNXI_DAC_DRC_LRMSHAT, (0x00012BB0 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LRMSLAT, 0x00012BB0 & 0xFFFF);
	/* Right RMS filter attack time */
	snd_soc_write(codec, SUNXI_DAC_DRC_RRMSHAT, (0x00012BB0 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_RRMSLAT, 0x00012BB0 & 0xFFFF);

	/* smooth filter attack time */
	snd_soc_write(codec, SUNXI_DAC_DRC_SFHAT, (0x00017665 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_SFLAT, 0x00017665 & 0xFFFF);
	/* gain smooth filter release time */
	snd_soc_write(codec, SUNXI_DAC_DRC_SFHRT, (0x00000F04 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_SFLRT, 0x00000F04 & 0xFFFF);

	/* OPL */
	snd_soc_write(codec, SUNXI_DAC_DRC_HOPL, (0xFE56CB10 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LOPL, 0xFE56CB10 & 0xFFFF);
	/* OPC */
	snd_soc_write(codec, SUNXI_DAC_DRC_HOPC, (0xFB04612F >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LOPC, 0xFB04612F & 0xFFFF);
	/* OPE */
	snd_soc_write(codec, SUNXI_DAC_DRC_HOPE, (0xF608C25F >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LOPE, 0xF608C25F & 0xFFFF);
	/* LT */
	snd_soc_write(codec, SUNXI_DAC_DRC_HLT, (0x035269E0 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LLT, 0x035269E0 & 0xFFFF);
	/* CT */
	snd_soc_write(codec, SUNXI_DAC_DRC_HCT, (0x06B3002C >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LCT, 0x06B3002C & 0xFFFF);
	/* ET */
	snd_soc_write(codec, SUNXI_DAC_DRC_HET, (0x0A139682 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LET, 0x0A139682 & 0xFFFF);
	/* Ki */
	snd_soc_write(codec, SUNXI_DAC_DRC_HKI, (0x00400000 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LKI, 0x00400000 & 0xFFFF);
	/* Kc */
	snd_soc_write(codec, SUNXI_DAC_DRC_HKC, (0x00FBCDA5 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LKC, 0x00FBCDA5 & 0xFFFF);
	/* Kn */
	snd_soc_write(codec, SUNXI_DAC_DRC_HKN, (0x0179B472 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LKN, 0x0179B472 & 0xFFFF);
	/* Ke */
	snd_soc_write(codec, SUNXI_DAC_DRC_HKE, (0x04234F68 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LKE, 0x04234F68 & 0xFFFF);
	/* MXG */
	snd_soc_write(codec, SUNXI_DAC_DRC_MXGHS, (0x035269E0 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_MXGLS, 0x035269E0 & 0xFFFF);
	/* MNG */
	snd_soc_write(codec, SUNXI_DAC_DRC_MNGHS, (0xF95B2C3F >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_MNGLS, 0xF95B2C3F & 0xFFFF);
	/* EPS */
	snd_soc_write(codec, SUNXI_DAC_DRC_EPSHC, (0x00025600 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_EPSLC, 0x00025600 & 0xFFFF);
}

static void dacdrc_enable(struct snd_soc_codec *codec, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct codec_dap *dac_dap = &sunxi_codec->dac_dap;

	if (on) {
		if (dac_dap->drc_enable++ == 0) {
			/* detect noise when ET enable */
			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_NOISE_DET_EN),
				(0x1 << DAC_DRC_NOISE_DET_EN));

			/* 0x0:RMS filter; 0x1:Peak filter */
			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_SIGNAL_SEL),
				(0x1 << DAC_DRC_SIGNAL_SEL));

			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_GAIN_MAX_EN),
				(0x1 << DAC_DRC_GAIN_MAX_EN));

			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_GAIN_MIN_EN),
				(0x1 << DAC_DRC_GAIN_MIN_EN));

			/* delay function enable */
			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_DELAY_BUF_EN),
				(0x1 << DAC_DRC_DELAY_BUF_EN));

			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_LT_EN),
				(0x1 << DAC_DRC_LT_EN));
			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_ET_EN),
				(0x1 << DAC_DRC_ET_EN));

			snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_DRC_EN),
				(0x1 << DDAP_DRC_EN));

			if (sunxi_codec->dac_dap_enable++ == 0)
				snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
					(0x1 << DDAP_EN), (0x1 << DDAP_EN));
		}
	} else {
		if (--dac_dap->drc_enable <= 0) {
			dac_dap->drc_enable = 0;
			if (--sunxi_codec->dac_dap_enable <= 0) {
				sunxi_codec->dac_dap_enable = 0;
				snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
					(0x1 << DDAP_EN), (0x0 << DDAP_EN));
			}

			snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_DRC_EN),
				(0x0 << DDAP_DRC_EN));

			/* detect noise when ET enable */
			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_NOISE_DET_EN),
				(0x0 << DAC_DRC_NOISE_DET_EN));

			/* 0x0:RMS filter; 0x1:Peak filter */
			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_SIGNAL_SEL),
				(0x0 << DAC_DRC_SIGNAL_SEL));

			/* delay function enable */
			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_DELAY_BUF_EN),
				(0x0 << DAC_DRC_DELAY_BUF_EN));

			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_GAIN_MAX_EN),
				(0x0 << DAC_DRC_GAIN_MAX_EN));
			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_GAIN_MIN_EN),
				(0x0 << DAC_DRC_GAIN_MIN_EN));

			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_LT_EN),
				(0x0 << DAC_DRC_LT_EN));
			snd_soc_update_bits(codec, SUNXI_DAC_DRC_CTRL,
				(0x1 << DAC_DRC_ET_EN),
				(0x0 << DAC_DRC_ET_EN));
		}
	}
}

static void dachpf_config(struct snd_soc_codec *codec)
{
	/* HPF */
	snd_soc_write(codec, SUNXI_DAC_DRC_HHPFC, (0xFFFAC1 >> 16) & 0xFFFF);
	snd_soc_write(codec, SUNXI_DAC_DRC_LHPFC, 0xFFFAC1 & 0xFFFF);
}

static void dachpf_enable(struct snd_soc_codec *codec, bool on)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct codec_dap *dac_dap = &sunxi_codec->dac_dap;

	if (on) {
		if (dac_dap->hpf_enable++ == 0) {
			snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_HPF_EN), (0x1 << DDAP_HPF_EN));

			if (sunxi_codec->dac_dap_enable++ == 0)
				snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
					(0x1 << DDAP_EN), (0x1 << DDAP_EN));
		}
	} else {
		if (--dac_dap->hpf_enable <= 0) {
			dac_dap->hpf_enable = 0;
			if (--sunxi_codec->dac_dap_enable <= 0) {
				sunxi_codec->dac_dap_enable = 0;
				snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
					(0x1 << DDAP_EN), (0x0 << DDAP_EN));
			}

			snd_soc_update_bits(codec, SUNXI_DAC_DAP_CTL,
				(0x1 << DDAP_HPF_EN),
				(0x0 << DDAP_HPF_EN));
		}
	}
}
#endif

#ifdef SUNXI_CODEC_HUB_ENABLE
static int sunxi_codec_get_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	unsigned int reg_val;

	reg_val = snd_soc_read(codec, SUNXI_DAC_DPC);

	ucontrol->value.integer.value[0] =
				((reg_val & (0x1 << DAC_HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		snd_soc_update_bits(codec, SUNXI_DAC_DPC,
				(0x1 << DAC_HUB_EN), (0x0 << DAC_HUB_EN));
		break;
	case	1:
		snd_soc_update_bits(codec, SUNXI_DAC_DPC,
				(0x1 << DAC_HUB_EN), (0x1 << DAC_HUB_EN));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/* sunxi codec hub mdoe select */
static const char * const sunxi_codec_hub_function[] = {
				"hub_disable", "hub_enable"};

static const struct soc_enum sunxi_codec_hub_mode_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sunxi_codec_hub_function),
			sunxi_codec_hub_function),
};
#endif

static int sunxi_codec_get_dacswap_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	unsigned int reg_val;

	reg_val = snd_soc_read(codec, SUNXI_DAC_DG);

	ucontrol->value.integer.value[0] =
				((reg_val & (0x1 << DAC_SWP)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_dacswap_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		snd_soc_update_bits(codec, SUNXI_DAC_DG,
				(0x1 << DAC_SWP), (0x0 << DAC_SWP));
		break;
	case	1:
		snd_soc_update_bits(codec, SUNXI_DAC_DG,
				(0x1 << DAC_SWP), (0x1 << DAC_SWP));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_codec_get_adcswap_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	unsigned int reg_val;

	reg_val = snd_soc_read(codec, SUNXI_ADC_DG);

	ucontrol->value.integer.value[0] =
				((reg_val & (0x1 << AD_SWP)) ? 1 : 0);

	return 0;
}

static int sunxi_codec_set_adcswap_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		snd_soc_update_bits(codec, SUNXI_ADC_DG,
				(0x1 << AD_SWP), (0x0 << AD_SWP));
		break;
	case	1:
		snd_soc_update_bits(codec, SUNXI_ADC_DG,
				(0x1 << AD_SWP), (0x1 << AD_SWP));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* sunxi codec dac swap func */
static const char * const sunxi_codec_dacswap_function[] = {
				"Off", "On"};

/* sunxi codec adc swap func */
static const char * const sunxi_codec_adcswap_function[] = {
				"Off", "On"};

static const struct soc_enum sunxi_codec_dacswap_func_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sunxi_codec_dacswap_function),
			sunxi_codec_dacswap_function),
};

static const struct soc_enum sunxi_codec_adcswap_func_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sunxi_codec_adcswap_function),
			sunxi_codec_adcswap_function),
};

static int sunxi_codec_hpspeaker_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dacdrc_enable(codec, 1);
		else if (hw_cfg->dacdrc_cfg & DAP_HP_EN)
			dacdrc_enable(codec, 0);

		if (hw_cfg->dachpf_cfg & DAP_SPK_EN)
			dachpf_enable(codec, 1);
		else if (hw_cfg->dachpf_cfg & DAP_HP_EN)
			dachpf_enable(codec, 0);
#endif
		if (spk_cfg->used) {
			gpio_set_value(spk_cfg->spk_gpio, spk_cfg->pa_level);
			/* time delay to wait spk pa work fine */
			msleep(spk_cfg->pa_msleep);
		}
		break;
	case	SND_SOC_DAPM_PRE_PMD:
		if (spk_cfg->used)
			gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dacdrc_enable(codec, 0);
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dachpf_enable(codec, 0);
#endif
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_headphone_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *k,	int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_HP_EN)
			dacdrc_enable(codec, 1);
		if (hw_cfg->dachpf_cfg & DAP_HP_EN)
			dachpf_enable(codec, 1);
#endif
		/*open*/
		snd_soc_update_bits(codec, SUNXI_HEADPHONE_REG,
				(0x1 << HPINPUTEN), (0x1 << HPINPUTEN));
		snd_soc_update_bits(codec, SUNXI_HEADPHONE_REG,
				(0x1 << HPOUTPUTEN), (0x1 << HPOUTPUTEN));
		snd_soc_update_bits(codec, SUNXI_HEADPHONE_REG,
				(0x1 << HPPA_EN), (0x1 << HPPA_EN));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/*close*/
		snd_soc_update_bits(codec, SUNXI_HEADPHONE_REG,
				(0x1 << HPPA_EN), (0x0 << HPPA_EN));
		snd_soc_update_bits(codec, SUNXI_HEADPHONE_REG,
				(0x1 << HPOUTPUTEN), (0x0 << HPOUTPUTEN));
		snd_soc_update_bits(codec, SUNXI_HEADPHONE_REG,
				(0x1 << HPINPUTEN), (0x0 << HPINPUTEN));

#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_HP_EN)
			dacdrc_enable(codec, 0);
		if (hw_cfg->dachpf_cfg & DAP_HP_EN)
			dachpf_enable(codec, 0);
#endif
		break;
	}

	return 0;
}

static int sunxi_codec_lineout_event(struct snd_soc_dapm_widget *w,
				  struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dacdrc_enable(codec, 1);
		if (hw_cfg->dachpf_cfg & DAP_SPK_EN)
			dachpf_enable(codec, 1);
#endif
		snd_soc_update_bits(codec, SUNXI_DAC_REG,
				(0x1 << LINEOUTLEN), (0x1 << LINEOUTLEN));

		if (spk_cfg->used) {
			gpio_set_value(spk_cfg->spk_gpio, spk_cfg->pa_level);
			/* time delay to wait spk pa work fine */
			msleep(spk_cfg->pa_msleep);
		}
		break;
	case	SND_SOC_DAPM_PRE_PMD:
		if (spk_cfg->used)
			gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));

		snd_soc_update_bits(codec, SUNXI_DAC_REG,
				(0x1 << LINEOUTLEN), (0x0 << LINEOUTLEN));

#ifdef SUNXI_CODEC_DAP_ENABLE
		if (hw_cfg->dacdrc_cfg & DAP_SPK_EN)
			dacdrc_enable(codec, 0);
		if (hw_cfg->dachpf_cfg & DAP_SPK_EN)
			dachpf_enable(codec, 0);
#endif
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_playback_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	switch (event) {
	case	SND_SOC_DAPM_PRE_PMU:
		/* DACL to left channel LINEOUT Mute control 0:mute 1: not mute */
		snd_soc_update_bits(codec, SUNXI_DAC_REG,
				(0x1 << DACLMUTE),
				(0x1 << DACLMUTE));
		snd_soc_update_bits(codec, SUNXI_DAC_DPC,
				(0x1<<EN_DAC), (0x1<<EN_DAC));
		msleep(30);
		break;
	case	SND_SOC_DAPM_POST_PMD:
		snd_soc_update_bits(codec, SUNXI_DAC_DPC,
				(0x1<<EN_DAC), (0x0<<EN_DAC));
		/* DACL to left channel LINEOUT Mute control 0:mute 1: not mute */
		snd_soc_update_bits(codec, SUNXI_DAC_REG,
				(0x1 << DACLMUTE),
				(0x0 << DACLMUTE));
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_capture_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	switch (event) {
	case	SND_SOC_DAPM_POST_PMU:
		/* delay 80ms to avoid the capture pop at the beginning */
		mdelay(80);
		snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(0x1 << EN_AD), (0x1 << EN_AD));
		break;
	case	SND_SOC_DAPM_POST_PMD:
		snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(0x1 << EN_AD), (0x0 << EN_AD));
		break;
	default:
		break;
	}

	return 0;
}

static int sunxi_codec_adc_mixer_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k, int event)
{
#ifdef SUNXI_CODEC_DAP_ENABLE
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct codec_hw_config *hw_cfg = &(sunxi_codec->hw_config);
	unsigned int adcctrl_val = 0;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (hw_cfg->adcdrc_cfg & DAP_HP_EN) {
			adcctrl_val = snd_soc_read(codec, SUNXI_ADCL_REG);
			if ((adcctrl_val >> MIC1AMPEN) & 0x1)
				adcdrc_enable(codec, 1);
		} else if (hw_cfg->adcdrc_cfg & DAP_SPK_EN) {
			adcctrl_val = snd_soc_read(codec, SUNXI_ADCR_REG);
			if ((adcctrl_val >> MIC2AMPEN) & 0x1)
				adcdrc_enable(codec, 1);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		if ((hw_cfg->adcdrc_cfg & DAP_SPK_EN) ||
			(hw_cfg->adcdrc_cfg & DAP_HP_EN))
			adcdrc_enable(codec, 0);
		break;
	default:
		break;
	}
#endif

	return 0;
}

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
#ifdef SUNXI_CODEC_HUB_ENABLE
	SOC_ENUM_EXT("codec hub mode", sunxi_codec_hub_mode_enum[0],
				sunxi_codec_get_hub_mode,
				sunxi_codec_set_hub_mode),
#endif
	SOC_ENUM_EXT("DAC Swap", sunxi_codec_dacswap_func_enum[0],
				sunxi_codec_get_dacswap_mode,
				sunxi_codec_set_dacswap_mode),
	SOC_ENUM_EXT("ADC Swap", sunxi_codec_adcswap_func_enum[0],
				sunxi_codec_get_adcswap_mode,
				sunxi_codec_set_adcswap_mode),
	/* Digital Volume */
	SOC_SINGLE_TLV("digital volume", SUNXI_DAC_DPC,
					DVOL, 0x3F, 0, digital_tlv),
	/* MIC1 Gain */
	SOC_SINGLE_TLV("MIC1 gain volume", SUNXI_ADCL_REG,
					ADCL_PGA_GAIN_CTRL, 0x1F, 0, mic1gain_tlv),
	/* MIC2 Gain */
	SOC_SINGLE_TLV("MIC2 gain volume", SUNXI_ADCR_REG,
					ADCR_PGA_GAIN_CTRL, 0x1F, 0, mic2gain_tlv),
	/* DAC Volume */
	SOC_SINGLE_TLV("LINEOUT volume", SUNXI_DAC_REG, LINEOUT_VOL,
			0x1F, 0, lineout_tlv),
	/* DAC Volume / HEADPHONE VOL */
	SOC_DOUBLE_TLV("DAC volume", SUNXI_DAC_VOL_CTRL, DAC_VOL_L, DAC_VOL_R,
		       0xFF, 0, dac_vol_tlv),
	/* ADC Volume */
	SOC_DOUBLE_TLV("ADC volume", SUNXI_ADC_VOL_CTRL, ADC_VOL_L, ADC_VOL_R,
		       0xFF, 0, adc_vol_tlv),
	/* Headphone Gain */
	SOC_SINGLE_TLV("Headphone Volume", SUNXI_DAC_REG, HEADPHONE_GAIN,
			0x7, 0, headphone_gain_tlv),
};

/* lineout controls */
static const char * const left_lineout_text[] = {
	"DAC_SINGLE", "DAC_DIFFER",
};

static const struct soc_enum left_lineout_enum =
	SOC_ENUM_SINGLE(SUNXI_DAC_REG, LINEOUTLDIFFEN,
			ARRAY_SIZE(left_lineout_text), left_lineout_text);

static const struct snd_kcontrol_new left_lineout_mux =
	SOC_DAPM_ENUM("LINEOUT Output Select", left_lineout_enum);

/* mic controls */
static const struct snd_kcontrol_new mic1_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC1 Boost Switch", SUNXI_ADCL_REG, MIC1AMPEN, 1, 0),
};

static const struct snd_kcontrol_new mic2_input_mixer[] = {
	SOC_DAPM_SINGLE("MIC2 Boost Switch", SUNXI_ADCR_REG, MIC2AMPEN, 1, 0),
};

/*audio dapm widget */
static const struct snd_soc_dapm_widget sunxi_codec_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN_E("DACL", "Playback", 0, SUNXI_DAC_REG,
				DACLEN, 0,
				sunxi_codec_playback_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_IN_E("DACR", "Playback", 0, SUNXI_DAC_REG,
				DACREN, 0,
				sunxi_codec_playback_event,
				SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("ADCL", "Capture", 0, SUNXI_ADCL_REG,
				ADCL_EN, 0,
				sunxi_codec_capture_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("ADCR", "Capture", 0, SUNXI_ADCR_REG,
				ADCR_EN, 0,
				sunxi_codec_capture_event,
				SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MUX("LINEOUT Output Select", SND_SOC_NOPM,
			0, 0, &left_lineout_mux),

	SND_SOC_DAPM_MIXER_E("ADCL Input", SND_SOC_NOPM, 0, 0,
			mic1_input_mixer, ARRAY_SIZE(mic1_input_mixer),
			sunxi_codec_adc_mixer_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MIXER_E("ADCR Input", SND_SOC_NOPM, 0, 0,
			mic2_input_mixer, ARRAY_SIZE(mic2_input_mixer),
			sunxi_codec_adc_mixer_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_MICBIAS("MainMic Bias", SUNXI_MICBIAS_REG,
				MMICBIASEN, 0),

	SND_SOC_DAPM_INPUT("MIC1"),
	SND_SOC_DAPM_INPUT("MIC2"),

	SND_SOC_DAPM_OUTPUT("HPOUTL"),
	SND_SOC_DAPM_OUTPUT("HPOUTR"),

	SND_SOC_DAPM_HP("Headphone", sunxi_codec_headphone_event),
	SND_SOC_DAPM_LINE("LINEOUT", sunxi_codec_lineout_event),
	SND_SOC_DAPM_SPK("HpSpeaker", sunxi_codec_hpspeaker_event),
};

static const struct snd_soc_dapm_route sunxi_codec_dapm_routes[] = {
	/* Mic input route */
	{"ADCL Input", "MIC1 Boost Switch", "MIC1"},
	{"ADCR Input", "MIC2 Boost Switch", "MIC2"},

	{"ADCL", NULL, "ADCL Input"},
	{"ADCR", NULL, "ADCR Input"},

	/* LINEOUT Output route */
	{"LINEOUT Output Select", "DAC_SINGLE", "DACL"},
	{"LINEOUT Output Select", "DAC_DIFFER", "DACL"},

	{"LINEOUT", NULL, "LINEOUT Output Select"},

	/* Headphone output route */
	{"HPOUTL", NULL, "DACL"},
	{"HPOUTR", NULL, "DACR"},

	{"Headphone", NULL, "HPOUTL"},
	{"Headphone", NULL, "HPOUTR"},

	{"HpSpeaker", NULL, "HPOUTR"},
	{"HpSpeaker", NULL, "HPOUTL"},
};

static void sunxi_codec_init(struct snd_soc_codec *codec)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	/* DAC_VOL_SEL default disabled */
	snd_soc_update_bits(codec, SUNXI_DAC_VOL_CTRL,
			(0x1 << DAC_VOL_SEL), (0x1 << DAC_VOL_SEL));

	/* ADC_VOL_SEL default disabled */
	snd_soc_update_bits(codec, SUNXI_ADC_VOL_CTRL,
			(0x1 << ADC_VOL_SEL), (0x1 << ADC_VOL_SEL));

	/* Enable ADCFDT to overcome niose at the beginning */
	snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
			(0x7 << ADCDFEN), (0x7 << ADCDFEN));

	/* Digital VOL defeult setting */
	snd_soc_update_bits(codec, SUNXI_DAC_DPC,
			0x3F << DVOL,
			sunxi_codec->digital_vol << DVOL);

	/* LINEOUT VOL defeult setting */
	snd_soc_update_bits(codec, SUNXI_DAC_REG,
			0x1F << LINEOUT_VOL,
			sunxi_codec->lineout_vol << LINEOUT_VOL);

	/* Headphone Gain defeult setting */
	snd_soc_update_bits(codec, SUNXI_DAC_REG,
			0x7 << HEADPHONE_GAIN,
			sunxi_codec->headphonegain << HEADPHONE_GAIN);

	/* ADCL MIC1 gain defeult setting */
	snd_soc_update_bits(codec, SUNXI_ADCL_REG,
			0x1F << ADCL_PGA_GAIN_CTRL,
			sunxi_codec->mic1gain << ADCL_PGA_GAIN_CTRL);

	/* ADCR MIC2 gain defeult setting */
	snd_soc_update_bits(codec, SUNXI_ADCR_REG,
			0x1F << ADCR_PGA_GAIN_CTRL,
			sunxi_codec->mic2gain << ADCR_PGA_GAIN_CTRL);

	/* ADCL/R IOP params default setting */
	snd_soc_update_bits(codec, SUNXI_ADCL_REG,
			0xFF << ADCL_IOPMICL, 0x55 << ADCL_IOPMICL);
	snd_soc_update_bits(codec, SUNXI_ADCR_REG,
			0xFF << ADCR_IOPMICL, 0x55 << ADCR_IOPMICL);

	/* For improve performance of THD+N about HP */
	snd_soc_update_bits(codec, SUNXI_HEADPHONE_REG,
			(0x3 << CP_CLKS), (0x3 << CP_CLKS));

	/* Swap the L/R channel to make it output right */
	snd_soc_update_bits(codec, SUNXI_DAC_DG,
			(0x1<<DAC_SWP), (0x1<<DAC_SWP));

	/* To fix some pop noise */
	snd_soc_update_bits(codec, SUNXI_HEADPHONE_REG,
			(0x1 << HPCALIFIRST), (0x1 << HPCALIFIRST));

	snd_soc_update_bits(codec, SUNXI_HEADPHONE_REG,
			(0x3 << HPPA_DEL), (0x3 << HPPA_DEL));

	snd_soc_update_bits(codec, SUNXI_DAC_REG,
			(0x3 << CPLDO_VOLTAGE), (0x1 << CPLDO_VOLTAGE));

#ifdef SUNXI_CODEC_DAP_ENABLE
	if (sunxi_codec->hw_config.adcdrc_cfg)
		adcdrc_config(codec);
	if (sunxi_codec->hw_config.adchpf_cfg)
		adchpf_config(codec);

	if (sunxi_codec->hw_config.dacdrc_cfg)
		dacdrc_config(codec);
	if (sunxi_codec->hw_config.dachpf_cfg)
		dachpf_config(codec);
#endif
}

static int sunxi_codec_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

#ifdef SUNXI_CODEC_DAP_ENABLE
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (sunxi_codec->hw_config.adchpf_cfg)
			adchpf_enable(codec, 1);
	}
#endif

	return 0;
}

static int sunxi_codec_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	int i = 0;

	switch (params_format(params)) {
	case	SNDRV_PCM_FORMAT_S16_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(3 << FIFO_MODE), (3 << FIFO_MODE));
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(1 << TX_SAMPLE_BITS), (0 << TX_SAMPLE_BITS));
		} else {
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(1 << RX_FIFO_MODE), (1 << RX_FIFO_MODE));
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(1 << RX_SAMPLE_BITS), (0 << RX_SAMPLE_BITS));
		}
		break;
	case	SNDRV_PCM_FORMAT_S24_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(3 << FIFO_MODE), (0 << FIFO_MODE));
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(1 << TX_SAMPLE_BITS), (1 << TX_SAMPLE_BITS));
		} else {
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(1 << RX_FIFO_MODE), (0 << RX_FIFO_MODE));
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(1 << RX_SAMPLE_BITS), (1 << RX_SAMPLE_BITS));
		}
		break;
	default:
		break;
	}

	for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++) {
		if (sample_rate_conv[i].samplerate == params_rate(params)) {
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
					(0x7 << DAC_FS),
					(sample_rate_conv[i].rate_bit << DAC_FS));
			} else {
				if (sample_rate_conv[i].samplerate > 48000)
					return -EINVAL;
				snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
					(0x7 << ADC_FS),
					(sample_rate_conv[i].rate_bit<<ADC_FS));
			}
		}
	}

	/* reset the adchpf func setting for different sampling */
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (sunxi_codec->hw_config.adchpf_cfg) {
			if (params_rate(params) == 16000) {
				snd_soc_write(codec, SUNXI_ADC_DRC_HHPFC,
						(0x00F623A5 >> 16) & 0xFFFF);

				snd_soc_write(codec, SUNXI_ADC_DRC_LHPFC,
							0x00F623A5 & 0xFFFF);

			} else if (params_rate(params) == 44100) {
				snd_soc_write(codec, SUNXI_ADC_DRC_HHPFC,
						(0x00FC60DB >> 16) & 0xFFFF);

				snd_soc_write(codec, SUNXI_ADC_DRC_LHPFC,
							0x00FC60DB & 0xFFFF);
			} else {
				snd_soc_write(codec, SUNXI_ADC_DRC_HHPFC,
						(0x00FCABB3 >> 16) & 0xFFFF);

				snd_soc_write(codec, SUNXI_ADC_DRC_LHPFC,
							0x00FCABB3 & 0xFFFF);
			}
		}
	}

	switch (params_channels(params)) {
	case 1:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(0x1 << DAC_MONO_EN), 0x1 << DAC_MONO_EN);
		} else {
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(0x3 << ADC_CHAN_EN), (0x1 << ADC_CHAN_EN));
		}
		break;
	case 2:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(0x1 << DAC_MONO_EN), (0x0 << DAC_MONO_EN));
		} else {
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(0x3 << ADC_CHAN_EN), (0x3 << ADC_CHAN_EN));
		}
		break;
	default:
		pr_err("[%s] audiocodec only support mono or stereo mode.\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int sunxi_codec_set_sysclk(struct snd_soc_dai *dai,
			int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	switch (clk_id) {
	case	0:
		/* For setting the clk source to 90.3168M to surpport playback */
		if (clk_set_parent(sunxi_codec->dacclk, sunxi_codec->pllcomdiv5)) {
			pr_err("audiocodec: set parent of dacclk to pllcomdiv5 failed\n");
			return -EINVAL;
		}

		if (clk_set_rate(sunxi_codec->dacclk, freq)) {
			pr_err("audiocodec: codec set dac clk rate failed\n");
			return -EINVAL;
		}
		break;
	case	1:
		/* For setting the clk source to 90.3168M to surpport capture */
		if (clk_set_parent(sunxi_codec->adcclk, sunxi_codec->pllcomdiv5)) {
			pr_err("audiocodec: set parent of adcclk to pllcomdiv5 failed\n");
			return -EINVAL;
		}

		if (clk_set_rate(sunxi_codec->adcclk, freq)) {
			pr_err("audiocodec: codec set adc clk rate failed\n");
			return -EINVAL;
		}
		break;
	case	2:
		/* For setting the clk source to 98.304M to surpport playback */
		if (clk_set_parent(sunxi_codec->dacclk, sunxi_codec->pllclkx4)) {
			pr_err("audiocodec: set parent of dacclk to pllclkx4 failed\n");
			return -EINVAL;
		}

		if (clk_set_rate(sunxi_codec->dacclk, freq)) {
			pr_err("audiocodec: codec set dac clk rate failed\n");
			return -EINVAL;
		}
		break;
	case	3:
		/* For setting the clk source to 98.304M to surpport capture */
		if (clk_set_parent(sunxi_codec->adcclk, sunxi_codec->pllclkx4)) {
			pr_err("audiocodec: set parent of adcclk to pllclkx4 failed\n");
			return -EINVAL;
		}

		if (clk_set_rate(sunxi_codec->adcclk, freq)) {
			pr_err("audiocodec: codec set adc clk rate failed\n");
			return -EINVAL;
		}
		break;
	default:
		pr_err("[%s]: Bad clk params input!\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static void sunxi_codec_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
#ifdef SUNXI_CODEC_DAP_ENABLE
	struct snd_soc_codec *codec = dai->codec;
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (sunxi_codec->hw_config.adchpf_cfg)
			adchpf_enable(codec, 0);
	}
#endif
}

static int sunxi_codec_digital_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);

	if (mute) {
		snd_soc_write(codec, SUNXI_DAC_VOL_CTRL, 0);
	} else {
		snd_soc_write(codec, SUNXI_DAC_VOL_CTRL,
				sunxi_codec->dac_digital_vol);
	}

	return 0;
}

static int sunxi_codec_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
			(1 << FIFO_FLUSH), (1 << FIFO_FLUSH));
		snd_soc_write(codec, SUNXI_DAC_FIFOS,
			(1 << DAC_TXE_INT | 1 << DAC_TXU_INT | 1 << DAC_TXO_INT));
		snd_soc_write(codec, SUNXI_DAC_CNT, 0);
	} else {
		snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(1 << ADC_FIFO_FLUSH), (1 << ADC_FIFO_FLUSH));
		snd_soc_write(codec, SUNXI_ADC_FIFOS,
				(1 << ADC_RXA_INT | 1 << ADC_RXO_INT));
		snd_soc_write(codec, SUNXI_ADC_CNT, 0);
	}

	return 0;
}

static int sunxi_codec_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(dai->codec);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(sunxi_codec->regmap, SUNXI_DAC_FIFOC,
				(1 << DAC_DRQ_EN), (1 << DAC_DRQ_EN));
		else
			regmap_update_bits(sunxi_codec->regmap, SUNXI_ADC_FIFOC,
				(1 << ADC_DRQ_EN), (1 << ADC_DRQ_EN));
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(sunxi_codec->regmap, SUNXI_DAC_FIFOC,
				(1 << DAC_DRQ_EN), (0 << DAC_DRQ_EN));
		else
			regmap_update_bits(sunxi_codec->regmap, SUNXI_ADC_FIFOC,
				(1 << ADC_DRQ_EN), (0 << ADC_DRQ_EN));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct snd_soc_dai_ops sunxi_codec_dai_ops = {
	.startup	= sunxi_codec_startup,
	.hw_params	= sunxi_codec_hw_params,
	.shutdown	= sunxi_codec_shutdown,
	.digital_mute	= sunxi_codec_digital_mute,
	.set_sysclk	= sunxi_codec_set_sysclk,
	.trigger	= sunxi_codec_trigger,
	.prepare	= sunxi_codec_prepare,
};

static struct snd_soc_dai_driver sunxi_codec_dai[] = {
	{
		.name	= "sun50iw10codec",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates	= SNDRV_PCM_RATE_8000_192000
				| SNDRV_PCM_RATE_KNOT,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_48000
				| SNDRV_PCM_RATE_KNOT,
			.formats = SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = &sunxi_codec_dai_ops,
	},
};

static int sunxi_codec_probe(struct snd_soc_codec *codec)
{
	int ret;
	struct snd_soc_dapm_context *dapm = &codec->component.dapm;

	ret = snd_soc_add_codec_controls(codec, sunxi_codec_controls,
					ARRAY_SIZE(sunxi_codec_controls));
	if (ret)
		pr_err("failed to register codec controls!\n");

	snd_soc_dapm_new_controls(dapm, sunxi_codec_dapm_widgets,
				ARRAY_SIZE(sunxi_codec_dapm_widgets));
	snd_soc_dapm_add_routes(dapm, sunxi_codec_dapm_routes,
				ARRAY_SIZE(sunxi_codec_dapm_routes));

	sunxi_codec_init(codec);

	return 0;
}

static int sunxi_codec_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static int sunxi_gpio_iodisable(u32 gpio)
{
	char pin_name[8];
	u32 config;
	u32 ret = 0;

	sunxi_gpio_to_name(gpio, pin_name);
	config = 7 << 16;
	ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);

	return ret;
}

static int save_audio_reg(struct sunxi_codec_info *sunxi_codec)
{
	int i = 0;

	while (reg_labels[i].name != NULL) {
		regmap_read(sunxi_codec->regmap, reg_labels[i].address,
			&reg_labels[i].value);
		i++;
	}

	return i;
}

static int echo_audio_reg(struct sunxi_codec_info *sunxi_codec)
{
	int i = 0;

	while (reg_labels[i].name != NULL) {
		regmap_write(sunxi_codec->regmap, reg_labels[i].address,
					reg_labels[i].value);
		i++;
	}

	return i;
}

static int sunxi_codec_suspend(struct snd_soc_codec *codec)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);

	pr_debug("Enter %s\n", __func__);
	save_audio_reg(sunxi_codec);

	if (spk_cfg->used)
		sunxi_gpio_iodisable(spk_cfg->spk_gpio);

	if (sunxi_codec->vol_supply.avcc)
		regulator_disable(sunxi_codec->vol_supply.avcc);

	if (sunxi_codec->vol_supply.cpvin)
		regulator_disable(sunxi_codec->vol_supply.cpvin);

	clk_disable_unprepare(sunxi_codec->dacclk);
	clk_disable_unprepare(sunxi_codec->adcclk);
	clk_disable_unprepare(sunxi_codec->pllcomdiv5);
	clk_disable_unprepare(sunxi_codec->pllcom);
	clk_disable_unprepare(sunxi_codec->pllclkx4);

	pr_debug("End %s\n", __func__);

	return 0;
}

static int sunxi_codec_resume(struct snd_soc_codec *codec)
{
	struct sunxi_codec_info *sunxi_codec = snd_soc_codec_get_drvdata(codec);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);
	unsigned int ret;

	pr_debug("Enter %s\n", __func__);

	if (sunxi_codec->vol_supply.avcc) {
		ret = regulator_enable(sunxi_codec->vol_supply.avcc);
		if (ret)
			pr_err("[%s]: resume avcc enable failed!\n", __func__);
	}

	if (sunxi_codec->vol_supply.cpvin) {
		ret = regulator_enable(sunxi_codec->vol_supply.cpvin);
		if (ret)
			pr_err("[%s]: resume cpvin enable failed!\n", __func__);
	}

	if (clk_set_rate(sunxi_codec->pllcom, 451584000)) {
		pr_err("audiocodec: resume codec source set pllcom rate failed\n");
		return -EBUSY;

	}

	if (clk_set_rate(sunxi_codec->pllclkx4, 98304000)) {
		pr_err("audiocodec: resume codec source set pllclkx4 rate failed\n");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->pllclkx4)) {
		dev_err(sunxi_codec->dev, "enable pllclkx4 failed, resume exit\n");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->pllcom)) {
		dev_err(sunxi_codec->dev, "enable pllcom failed, resume exit\n");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->pllcomdiv5)) {
		dev_err(sunxi_codec->dev, "enable pllcomdiv5 failed, resume exit\n");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->dacclk)) {
		dev_err(sunxi_codec->dev, "enable dacclk failed, resume exit\n");
		return -EBUSY;
	}

	if (clk_prepare_enable(sunxi_codec->adcclk)) {
		dev_err(sunxi_codec->dev, "enable  adcclk failed, resume exit\n");
		clk_disable_unprepare(sunxi_codec->adcclk);
		return -EBUSY;
	}

	if (spk_cfg->used) {
		gpio_direction_output(spk_cfg->spk_gpio, 1);
		gpio_set_value(spk_cfg->spk_gpio, !(spk_cfg->pa_level));
	}

	sunxi_codec_init(codec);
	echo_audio_reg(sunxi_codec);

	pr_debug("End %s\n", __func__);

	return 0;
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

static const struct snd_soc_component_driver sunxi_asoc_cpudai_component = {
	.name = "sunxi-internal-cpudai",
};

static ssize_t show_audio_reg(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sunxi_codec_info *sunxi_codec = dev_get_drvdata(dev);
	int count = 0, i = 0;
	unsigned int reg_val;
	unsigned int size = ARRAY_SIZE(reg_labels);

	count += sprintf(buf, "dump audiocodec reg:\n");

	while ((i < size) && (reg_labels[i].name != NULL)) {
		regmap_read(sunxi_codec->regmap,
				reg_labels[i].address, &reg_val);
		count += sprintf(buf + count, "%-20s [0x%03x]: 0x%-10x save_val:0x%x\n",
			reg_labels[i].name, (reg_labels[i].address),
			reg_val, reg_labels[i].value);
		i++;
	}

	return count;
}

/* ex:
 * param 1: 0 read;1 write
 * param 2: reg value;
 * param 3: write value;
	read:
		echo 0,0x00> audio_reg
	write:
		echo 1,0x00,0xa > audio_reg
*/
static ssize_t store_audio_reg(struct device *dev, struct device_attribute *attr,
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

	if (input_reg_offset > SUNXI_BIAS_REG) {
		pr_err("ERROR: the reg offset[0x%03x] > SUNXI_BIAS_REG[0x%03x]\n",
			input_reg_offset, SUNXI_BIAS_REG);
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

static const struct regmap_config sunxi_codec_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_HMIC_STS,
	.cache_type = REGCACHE_NONE,
};

static int sunxi_internal_codec_probe(struct platform_device *pdev)
{
	struct sunxi_codec_info *sunxi_codec;
	struct resource res;
	struct resource *memregion = NULL;
	struct device_node *np = pdev->dev.of_node;
	int ret;
	unsigned int temp_val;

	if (IS_ERR_OR_NULL(np)) {
		dev_err(&pdev->dev, "pdev->dev.of_node is err.\n");
		ret = -EFAULT;
		goto err_node_put;
	}

	sunxi_codec = devm_kzalloc(&pdev->dev,
				sizeof(struct sunxi_codec_info), GFP_KERNEL);
	if (!sunxi_codec) {
		dev_err(&pdev->dev, "Can't allocate sunxi codec memory\n");
		ret = -ENOMEM;
		goto err_node_put;
	}
	dev_set_drvdata(&pdev->dev, sunxi_codec);
	sunxi_codec->dev = &pdev->dev;

	/* PA/SPK enable property */
	ret = of_property_read_u32(np, "pa_level", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "pa_level get failed.\n");
		sunxi_codec->spk_config.pa_level = 1;
	} else {
		sunxi_codec->spk_config.pa_level = temp_val;
	}


	/* get the gpio number to control external speaker */
	ret = of_get_named_gpio(np, "gpio-spk", 0);
	if (ret >= 0) {
		sunxi_codec->spk_config.used = 1;
		sunxi_codec->spk_config.spk_gpio = ret;
		if (!gpio_is_valid(sunxi_codec->spk_config.spk_gpio)) {
			dev_err(&pdev->dev, "gpio-spk is invalid\n");
			ret = -EINVAL;
			goto err_digital_iounmap;
		} else {
			pr_debug("gpio-spk:%d\n",
					sunxi_codec->spk_config.spk_gpio);
			ret = devm_gpio_request(&pdev->dev,
					sunxi_codec->spk_config.spk_gpio, "SPK");
			if (ret) {
				dev_err(&pdev->dev, "failed to request gpio-spk gpio\n");
				ret = -EBUSY;
				goto err_digital_iounmap;
			} else {
				gpio_direction_output(sunxi_codec->spk_config.spk_gpio, 1);
				gpio_set_value(sunxi_codec->spk_config.spk_gpio,
					!(sunxi_codec->spk_config.pa_level));
			}
		}
	} else {
		sunxi_codec->spk_config.used = 0;
	}

	/* regulator about */
	sunxi_codec->vol_supply.avcc = regulator_get(&pdev->dev, "avcc");
	if (IS_ERR(sunxi_codec->vol_supply.avcc)) {
		pr_err("[%s]:get audio avcc failed\n", __func__);
		ret = -EFAULT;
	} else {
		ret = regulator_set_voltage(sunxi_codec->vol_supply.avcc,
							1800000, 1800000);
		if (ret) {
			pr_err("[%s]: audiocodec avcc set vol failed\n", __func__);
			ret = -EFAULT;
		}

		ret = regulator_enable(sunxi_codec->vol_supply.avcc);
		if (ret) {
			pr_err("[%s]:avcc enable failed!\n", __func__);
			ret = -EFAULT;
		}
	}

	sunxi_codec->vol_supply.cpvin = regulator_get(&pdev->dev, "cpvin");
	if (IS_ERR(sunxi_codec->vol_supply.cpvin)) {
		pr_err("[%s]:get audio cpvin failed\n", __func__);
		ret = -EFAULT;
	} else {
		ret = regulator_set_voltage(sunxi_codec->vol_supply.cpvin,
							1800000, 1800000);
		if (ret) {
			pr_err("[%s]: audiocodec cpvin set vol failed\n", __func__);
			ret = -EFAULT;
		}

		ret = regulator_enable(sunxi_codec->vol_supply.cpvin);
		if (ret) {
			pr_err("[%s]:cpvin enable failed!\n", __func__);
			ret = -EFAULT;
		}
	}

	/* get the parent clk and the module clk */
	sunxi_codec->pllclkx4 = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(sunxi_codec->pllclkx4)) {
		pr_err("audiocodec: pllclkx4 not exist or invaild\n");
		ret = PTR_ERR(sunxi_codec->pllclkx4);
		goto err_devm_kfree;
	}

	sunxi_codec->dacclk = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(sunxi_codec->dacclk)) {
		pr_err("audiocodec: dacclk not exist or invaild\n");
		ret = PTR_ERR(sunxi_codec->dacclk);
		goto err_devm_kfree;
	}

	sunxi_codec->adcclk = of_clk_get(np, 2);
	if (IS_ERR_OR_NULL(sunxi_codec->adcclk)) {
		pr_err("audiocodec: adcclk not exist or invaild\n");
		ret = PTR_ERR(sunxi_codec->adcclk);
		goto err_devm_kfree;
	}

	sunxi_codec->pllcom = of_clk_get(np, 3);
	if (IS_ERR_OR_NULL(sunxi_codec->pllcom)) {
		pr_err("audiocodec: pllcom not exist or invaild\n");
		ret = PTR_ERR(sunxi_codec->pllcom);
		goto err_devm_kfree;
	}

	sunxi_codec->pllcomdiv5 = of_clk_get(np, 4);
	if (IS_ERR_OR_NULL(sunxi_codec->pllcomdiv5)) {
		pr_err("audiocodec: pllcomdiv5 not exist or invaild\n");
		ret = PTR_ERR(sunxi_codec->pllcomdiv5);
		goto err_devm_kfree;
	}

	if (clk_set_parent(sunxi_codec->pllcomdiv5, sunxi_codec->pllcom)) {
		pr_err("audiocodec: set parent of pllcomdiv5 to pllcom failed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	if (clk_set_parent(sunxi_codec->dacclk, sunxi_codec->pllcomdiv5)) {
		pr_err("audiocodec: set parent of dacclk to pllcomdiv5 failed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	if (clk_set_parent(sunxi_codec->adcclk, sunxi_codec->pllcomdiv5)) {
		pr_err("audiocodec: set parent of adcclk to pllcomdiv5 failed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	if (clk_set_rate(sunxi_codec->pllcom, 451584000)) {
		pr_err("audiocodec: codec source set pllcom rate failed\n");
		ret = -EBUSY;
		goto err_devm_kfree;

	}

	if (clk_set_rate(sunxi_codec->pllclkx4, 98304000)) {
		pr_err("audiocodec: codec source set pllclkx4 rate failed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->pllclkx4)) {
		dev_err(&pdev->dev, "pllclkx4 enable failed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->pllcom)) {
		dev_err(&pdev->dev, "pllcom enable failed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->pllcomdiv5)) {
		dev_err(&pdev->dev, "pllcomdiv5 enable failed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	if (clk_prepare_enable(sunxi_codec->dacclk)) {
		dev_err(&pdev->dev, "dacclk enable failed\n");
		ret = -EBUSY;
		goto err_pllclk_put;
	}

	if (clk_prepare_enable(sunxi_codec->adcclk)) {
		dev_err(&pdev->dev, "moduleclk enable failed\n");
		ret = -EBUSY;
		goto err_pllclk_put;
	}

	/* codec reg_base */
	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "Failed to get sunxi resource\n");
		return -EINVAL;
		goto err_moduleclk_disable;
	}

	memregion = devm_request_mem_region(&pdev->dev, res.start,
				resource_size(&res), "sunxi-internal-codec");
	if (!memregion) {
		dev_err(&pdev->dev, "sunxi memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_free_memregion;
	}

	sunxi_codec->digital_base = devm_ioremap(&pdev->dev,
					res.start, resource_size(&res));
	if (!sunxi_codec->digital_base) {
		dev_err(&pdev->dev, "sunxi digital_base ioremap failed\n");
		ret = -EBUSY;
		goto err_moduleclk_disable;
	}

	sunxi_codec->regmap = devm_regmap_init_mmio(&pdev->dev,
				sunxi_codec->digital_base,
				&sunxi_codec_regmap_config);
	if (IS_ERR_OR_NULL(sunxi_codec->regmap)) {
		dev_err(&pdev->dev, "regmap init failed\n");
		ret = PTR_ERR(sunxi_codec->regmap);
		goto err_digital_iounmap;
	}

	/* get the special property form the board.dts */
	ret = of_property_read_u32(np, "digital_vol", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "digital volume get failed\n");
		sunxi_codec->digital_vol = 0;
	} else {
		sunxi_codec->digital_vol = temp_val;
	}

	ret = of_property_read_u32(np, "dac_digital_vol", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "dac digital volume get failed\n");
		sunxi_codec->dac_digital_vol = 0x1A0A0;
	} else {
		sunxi_codec->dac_digital_vol = temp_val;
	}

	/* lineout volume */
	ret = of_property_read_u32(np, "lineout_vol", &temp_val);
	if (ret < 0) {
		pr_err("[audio-codec] lineout volume missing or invalid.\n");
		ret = -EINVAL;
	} else {
		sunxi_codec->lineout_vol = temp_val;
	}

	/* headphone volume */
	ret = of_property_read_u32(np, "headphonegain", &temp_val);
	if (ret < 0) {
		pr_err("[audio-codec] headphonegain missing or invalid.\n");
		sunxi_codec->headphonegain = 0;
	} else {
		sunxi_codec->headphonegain = temp_val;
	}

	/* mic gain for capturing */
	ret = of_property_read_u32(np, "mic1gain", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "mic1gain get failed\n");
		sunxi_codec->mic1gain = 32;
	} else {
		sunxi_codec->mic1gain = temp_val;
	}
	ret = of_property_read_u32(np, "mic2gain", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "mic2gain get failed\n");
		sunxi_codec->mic2gain = 32;
	} else {
		sunxi_codec->mic2gain = temp_val;
	}

	/* Pa's delay time(ms) to work fine */
	ret = of_property_read_u32(np, "pa_msleep_time", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "pa_msleep get failed\n");
		sunxi_codec->spk_config.pa_msleep = 160;
	} else {
		sunxi_codec->spk_config.pa_msleep = temp_val;
	}

	pr_debug("digital_vol:%d, lineout_vol:%d, mic1gain:%d, mic2gain:%d"
		" pa_msleep:%d, pa_level:%d, dac_digital_vol:%d\n",
		sunxi_codec->digital_vol,
		sunxi_codec->lineout_vol,
		sunxi_codec->mic1gain,
		sunxi_codec->mic2gain,
		sunxi_codec->spk_config.pa_msleep,
		sunxi_codec->spk_config.pa_level,
		sunxi_codec->dac_digital_vol);

#ifdef SUNXI_CODEC_DAP_ENABLE
	/* ADC/DAC DRC/HPF func enable property */
	ret = of_property_read_u32(np, "adcdrc_cfg", &temp_val);
	if (ret < 0) {
		pr_err("[%s] adcdrc_cfg configs missing or invalid.\n", __func__);
		ret = -EINVAL;
	} else {
		sunxi_codec->hw_config.adcdrc_cfg = temp_val;
	}

	ret = of_property_read_u32(np, "adchpf_cfg", &temp_val);
	if (ret < 0) {
		pr_err("[%s] adchpf_cfg configs missing or invalid.\n", __func__);
		ret = -EINVAL;
	} else {
		sunxi_codec->hw_config.adchpf_cfg = temp_val;
	}

	ret = of_property_read_u32(np, "dacdrc_cfg", &temp_val);
	if (ret < 0) {
		pr_err("[%s] dacdrc_cfg configs missing or invalid.\n", __func__);
		ret = -EINVAL;
	} else {
		sunxi_codec->hw_config.dacdrc_cfg = temp_val;
	}

	ret = of_property_read_u32(np, "dachpf_cfg", &temp_val);
	if (ret < 0) {
		pr_err("[%s] dachpf_cfg configs missing or invalid.\n", __func__);
		ret = -EINVAL;
	} else {
		sunxi_codec->hw_config.dachpf_cfg = temp_val;
	}

	pr_debug("adcdrc_cfg:%d, adchpf_cfg:%d, dacdrc_cfg:%d, dachpf:%d\n",
		sunxi_codec->hw_config.adcdrc_cfg,
		sunxi_codec->hw_config.adchpf_cfg,
		sunxi_codec->hw_config.dacdrc_cfg,
		sunxi_codec->hw_config.dachpf_cfg);
#endif

	ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_sunxi,
				sunxi_codec_dai, ARRAY_SIZE(sunxi_codec_dai));
	if (ret < 0) {
		dev_err(&pdev->dev, "register codec failed\n");
		goto err_digital_iounmap;
	}

	ret  = sysfs_create_group(&pdev->dev.kobj, &audio_debug_attr_group);
	if (ret) {
		dev_warn(&pdev->dev, "failed to create attr group\n");
		goto err_unregister_codec;
	}

	dev_warn(&pdev->dev, "[%s] codec probe finished.\n", __func__);

	return 0;

err_unregister_codec:
	snd_soc_unregister_codec(&pdev->dev);
err_digital_iounmap:
	devm_iounmap(&pdev->dev, sunxi_codec->digital_base);
err_moduleclk_disable:
	clk_disable_unprepare(sunxi_codec->dacclk);
	clk_disable_unprepare(sunxi_codec->adcclk);
err_pllclk_put:
	clk_disable_unprepare(sunxi_codec->pllclkx4);
	clk_disable_unprepare(sunxi_codec->pllcomdiv5);
	clk_disable_unprepare(sunxi_codec->pllcom);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_codec);
err_node_put:
	of_node_put(np);
err_devm_free_memregion:
	devm_release_mem_region(&pdev->dev, memregion->start,
						resource_size(memregion));
	return ret;
}

static int  __exit sunxi_internal_codec_remove(struct platform_device *pdev)
{
	struct sunxi_codec_info *sunxi_codec = dev_get_drvdata(&pdev->dev);
	struct codec_spk_config *spk_cfg = &(sunxi_codec->spk_config);

	sysfs_remove_group(&pdev->dev.kobj, &audio_debug_attr_group);
	snd_soc_unregister_codec(&pdev->dev);

	clk_disable_unprepare(sunxi_codec->dacclk);
	clk_put(sunxi_codec->dacclk);
	clk_disable_unprepare(sunxi_codec->adcclk);
	clk_put(sunxi_codec->adcclk);
	clk_disable_unprepare(sunxi_codec->pllcomdiv5);
	clk_put(sunxi_codec->pllcomdiv5);
	clk_disable_unprepare(sunxi_codec->pllcom);
	clk_put(sunxi_codec->pllcom);
	clk_disable_unprepare(sunxi_codec->pllclkx4);
	clk_put(sunxi_codec->pllclkx4);

	devm_iounmap(&pdev->dev, sunxi_codec->digital_base);
	devm_kfree(&pdev->dev, sunxi_codec);
	platform_set_drvdata(pdev, NULL);

	if (spk_cfg->used) {
		gpio_set_value(spk_cfg->spk_gpio, 0);
	}

	dev_warn(&pdev->dev, "[%s] codec remove finished.\n", __func__);

	return 0;
}

static const struct of_device_id sunxi_internal_codec_of_match[] = {
	{ .compatible = "allwinner,sunxi-internal-codec", },
	{},
};

static struct platform_driver sunxi_internal_codec_driver = {
	.driver = {
		.name = "sunxi-internal-codec",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_internal_codec_of_match,
	},
	.probe = sunxi_internal_codec_probe,
	.remove = __exit_p(sunxi_internal_codec_remove),
};
module_platform_driver(sunxi_internal_codec_driver);

MODULE_DESCRIPTION("SUNXI Codec ASoC driver");
MODULE_AUTHOR("luguofang <luguofang@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-internal-codec");
