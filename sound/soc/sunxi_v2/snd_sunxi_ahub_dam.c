/*
 * sound\soc\sunxi\snd_sunxi_ahub_dam.c
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
#include <sound/soc.h>
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

#include "snd_sunxi_log.h"
#include "snd_sunxi_pcm.h"
#include "snd_sunxi_ahub_dam.h"

#define HLOG		"AHUB_DAM"
#define DRV_NAME	"sunxi-snd-plat-ahub_dam"

#define CLK_PLL_AUDIO_NUM	0
#define CLK_PLL_AUDIOX4_NUM	1
#define CLK_AUDIO_HUB_NUM	2

#define REG_LABEL(constant)	{#constant, constant, 0}
#define REG_LABEL_END		{NULL, 0, 0}

struct audio_reg_label {
	const char *name;
	const unsigned int address;
	unsigned int value;
};

/* ahub_reg */
static struct audio_reg_label ahub_reg_public[] = {
	REG_LABEL(SUNXI_AHUB_CTL),
	REG_LABEL(SUNXI_AHUB_VER),
	REG_LABEL(SUNXI_AHUB_RST),
	REG_LABEL(SUNXI_AHUB_GAT),
	REG_LABEL_END,
};

/* APBIF */
static struct audio_reg_label ahub_reg_apbif0[] = {
	REG_LABEL(SUNXI_AHUB_APBIF_TX_CTL(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_TX_IRQ_CTL(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_TX_IRQ_STA(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_CTL(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_STA(0)),
	/* SUNXI_AHUB_APBIF_TXFIFO(0), */
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_CNT(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_CTL(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_IRQ_CTL(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_IRQ_STA(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CTL(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_STA(0)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CONT(0)),
	/* REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO(0)), */
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CNT(0)),
	REG_LABEL_END,
};

static struct audio_reg_label ahub_reg_apbif1[] = {
	REG_LABEL(SUNXI_AHUB_APBIF_TX_CTL(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_TX_IRQ_CTL(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_TX_IRQ_STA(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_CTL(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_STA(1)),
	/* REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO(1)), */
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_CNT(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_CTL(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_IRQ_CTL(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_IRQ_STA(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CTL(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_STA(1)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CONT(1)),
	/* REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO(1)), */
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CNT(1)),
	REG_LABEL_END,
};

static struct audio_reg_label ahub_reg_apbif2[] = {
	REG_LABEL(SUNXI_AHUB_APBIF_TX_CTL(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_TX_IRQ_CTL(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_TX_IRQ_STA(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_CTL(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_STA(2)),
	/* REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO(2)), */
	REG_LABEL(SUNXI_AHUB_APBIF_TXFIFO_CNT(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_CTL(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_IRQ_CTL(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_RX_IRQ_STA(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CTL(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_STA(2)),
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CONT(2)),
	/* REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO(2)), */
	REG_LABEL(SUNXI_AHUB_APBIF_RXFIFO_CNT(2)),
	REG_LABEL_END,
};

/* I2S */
static struct audio_reg_label ahub_reg_i2s0[] = {
	REG_LABEL(SUNXI_AHUB_I2S_CTL(0)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT0(0)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT1(0)),
	REG_LABEL(SUNXI_AHUB_I2S_CLKD(0)),
	REG_LABEL(SUNXI_AHUB_I2S_RXCONT(0)),
	REG_LABEL(SUNXI_AHUB_I2S_CHCFG(0)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_CTL(0)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_STA(0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(0, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(0, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(0, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(0, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(0, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(0, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(0, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(0, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(0, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(0, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(0, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(0, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_SLOT(0)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP0(0)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP1(0)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP2(0)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP3(0)),
	REG_LABEL_END,
};

static struct audio_reg_label ahub_reg_i2s1[] = {
	REG_LABEL(SUNXI_AHUB_I2S_CTL(1)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT0(1)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT1(1)),
	REG_LABEL(SUNXI_AHUB_I2S_CLKD(1)),
	REG_LABEL(SUNXI_AHUB_I2S_RXCONT(1)),
	REG_LABEL(SUNXI_AHUB_I2S_CHCFG(1)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_CTL(1)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_STA(1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(1, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(1, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(1, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(1, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(1, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(1, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(1, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(1, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(1, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(1, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(1, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(1, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_SLOT(1)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP0(1)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP1(1)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP2(1)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP3(1)),
	REG_LABEL_END,
};

static struct audio_reg_label ahub_reg_i2s2[] = {
	REG_LABEL(SUNXI_AHUB_I2S_CTL(2)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT0(2)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT1(2)),
	REG_LABEL(SUNXI_AHUB_I2S_CLKD(2)),
	REG_LABEL(SUNXI_AHUB_I2S_RXCONT(2)),
	REG_LABEL(SUNXI_AHUB_I2S_CHCFG(2)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_CTL(2)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_STA(2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(2, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(2, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(2, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(2, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(2, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(2, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(2, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(2, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(2, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(2, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(2, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(2, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_SLOT(2)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP0(2)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP1(2)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP2(2)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP3(2)),
	REG_LABEL_END,
};

static struct audio_reg_label ahub_reg_i2s3[] = {
	REG_LABEL(SUNXI_AHUB_I2S_CTL(3)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT0(3)),
	REG_LABEL(SUNXI_AHUB_I2S_FMT1(3)),
	REG_LABEL(SUNXI_AHUB_I2S_CLKD(3)),
	REG_LABEL(SUNXI_AHUB_I2S_RXCONT(3)),
	REG_LABEL(SUNXI_AHUB_I2S_CHCFG(3)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_CTL(3)),
	REG_LABEL(SUNXI_AHUB_I2S_IRQ_STA(3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(3, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(3, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(3, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_SLOT(3, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(3, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(3, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(3, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP0(3, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(3, 0)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(3, 1)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(3, 2)),
	REG_LABEL(SUNXI_AHUB_I2S_OUT_CHMAP1(3, 3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_SLOT(3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP0(3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP1(3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP2(3)),
	REG_LABEL(SUNXI_AHUB_I2S_IN_CHMAP3(3)),
	REG_LABEL_END,
};

/* DAM */
static struct audio_reg_label ahub_reg_dam0[] = {
	REG_LABEL(SUNXI_AHUB_DAM_CTL(0)),
	REG_LABEL(SUNXI_AHUB_DAM_RX0_SRC(0)),
	REG_LABEL(SUNXI_AHUB_DAM_RX1_SRC(0)),
	REG_LABEL(SUNXI_AHUB_DAM_RX2_SRC(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL0(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL1(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL2(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL3(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL4(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL5(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL6(0)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL7(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL0(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL1(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL2(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL3(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL4(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL5(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL6(0)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL7(0)),
	REG_LABEL_END,
};

static struct audio_reg_label ahub_reg_dam1[] = {
	REG_LABEL(SUNXI_AHUB_DAM_CTL(1)),
	REG_LABEL(SUNXI_AHUB_DAM_RX0_SRC(1)),
	REG_LABEL(SUNXI_AHUB_DAM_RX1_SRC(1)),
	REG_LABEL(SUNXI_AHUB_DAM_RX2_SRC(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL0(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL1(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL2(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL3(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL4(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL5(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL6(1)),
	REG_LABEL(SUNXI_AHUB_DAM_MIX_CTL7(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL0(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL1(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL2(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL3(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL4(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL5(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL6(1)),
	REG_LABEL(SUNXI_AHUB_DAM_GAIN_CTL7(1)),
	REG_LABEL_END,
};

struct audio_reg_label *ahub_reg_all[] = {
	ahub_reg_public,
	ahub_reg_apbif0,
	ahub_reg_apbif1,
	ahub_reg_apbif2,
	ahub_reg_i2s0,
	ahub_reg_i2s1,
	ahub_reg_i2s2,
	ahub_reg_i2s3,
	ahub_reg_dam0,
	ahub_reg_dam1,
};

static struct resource sunxi_res;
static struct regmap_config sunxi_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_AHUB_MAX_REG,
	.cache_type = REGCACHE_NONE,
};
struct sunxi_ahub_mem sunxi_mem = {
	.res = &sunxi_res,
};
static struct sunxi_ahub_clk sunxi_clk;

static int sunxi_ahub_dam_dai_suspend(struct snd_soc_dai *dai)
{
	struct sunxi_ahub_clk *clk = &sunxi_clk;

	(void)dai;

	SND_LOG_DEBUG(HLOG, "\n");

	clk_disable_unprepare(clk->clk_module);
	clk_disable_unprepare(clk->clk_pll);
	clk_disable_unprepare(clk->clk_pllx4);

	return 0;
}

static int sunxi_ahub_dam_dai_resume(struct snd_soc_dai *dai)
{
	struct sunxi_ahub_clk *clk = &sunxi_clk;

	(void)dai;

	SND_LOG_DEBUG(HLOG, "\n");

	if (clk_prepare_enable(clk->clk_pll)) {
		SND_LOG_ERR(HLOG, "clk_pll enable failed\n");
		return -EBUSY;
	}
	if (clk_prepare_enable(clk->clk_pllx4)) {
		SND_LOG_ERR(HLOG, "clk_pllx4 enable failed\n");
		return -EBUSY;
	}
	if (clk_prepare_enable(clk->clk_module)) {
		SND_LOG_ERR(HLOG, "clk_module enable failed\n");
		return -EBUSY;
	}

	return 0;
}

static struct snd_soc_dai_driver sunxi_ahub_dam_dai = {
	.suspend	= sunxi_ahub_dam_dai_suspend,
	.resume		= sunxi_ahub_dam_dai_resume,
};

static struct snd_soc_platform_driver sunxi_ahub_dam_platform = {
};

static int sunxi_ahub_dam_probe(struct snd_soc_component *component)
{
	SND_LOG_DEBUG(HLOG, "\n");

	return 0;
}

static const unsigned int ahub_mux_reg[] = {
	SUNXI_AHUB_APBIF_RXFIFO_CONT(0),
	SUNXI_AHUB_APBIF_RXFIFO_CONT(1),
	SUNXI_AHUB_APBIF_RXFIFO_CONT(2),
	SUNXI_AHUB_I2S_RXCONT(0),
	SUNXI_AHUB_I2S_RXCONT(1),
	SUNXI_AHUB_I2S_RXCONT(2),
	SUNXI_AHUB_I2S_RXCONT(3),
	SUNXI_AHUB_DAM_RX0_SRC(0),
	SUNXI_AHUB_DAM_RX1_SRC(0),
	SUNXI_AHUB_DAM_RX2_SRC(0),
	SUNXI_AHUB_DAM_RX0_SRC(1),
	SUNXI_AHUB_DAM_RX1_SRC(1),
	SUNXI_AHUB_DAM_RX2_SRC(1),
};

static const unsigned int ahub_mux_values[] = {
	0,
	1 << I2S_RX_APBIF_TXDIF0,
	1 << I2S_RX_APBIF_TXDIF1,
	1 << I2S_RX_APBIF_TXDIF2,
	1 << I2S_RX_I2S0_TXDIF,
	1 << I2S_RX_I2S1_TXDIF,
	1 << I2S_RX_I2S2_TXDIF,
	1 << I2S_RX_I2S3_TXDIF,
	1 << I2S_RX_DAM0_TXDIF,
	1 << I2S_RX_DAM1_TXDIF,
};

static const char *ahub_mux_text[] = {
	"NONE",
	"APBIF_TXDIF0",
	"APBIF_TXDIF1",
	"APBIF_TXDIF2",
	"I2S0_TXDIF",
	"I2S1_TXDIF",
	"I2S2_TXDIF",
	"I2S3_TXDIF",
	"DAM0_TXDIF",
	"DAM1_TXDIF",
};

static SOC_ENUM_SINGLE_EXT_DECL(ahub_mux, ahub_mux_text);

static int sunxi_ahub_mux_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	int i;
	unsigned int reg_val;
	unsigned int src_reg;
	struct regmap *regmap = sunxi_mem.regmap;

	if (kcontrol->id.numid > ARRAY_SIZE(ahub_mux_reg))
		return -EINVAL;

	src_reg = ahub_mux_reg[kcontrol->id.numid - 1];
	regmap_read(regmap, src_reg, &reg_val);
	reg_val &= 0xEE888000;

	for (i = 1; i < ARRAY_SIZE(ahub_mux_values); i++) {
		if (reg_val & ahub_mux_values[i]) {
			ucontrol->value.integer.value[0] = i;
			return 0;
		}
	}
	ucontrol->value.integer.value[0] = 0;

	return 0;
}

static int sunxi_ahub_mux_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	unsigned int src_reg, src_regbit;
	struct regmap *regmap = sunxi_mem.regmap;

	if (kcontrol->id.numid > ARRAY_SIZE(ahub_mux_reg))
		return -EINVAL;
	if (ucontrol->value.integer.value[0] > ARRAY_SIZE(ahub_mux_values))
		return -EINVAL;

	src_reg = ahub_mux_reg[kcontrol->id.numid - 1];
	src_regbit = ahub_mux_values[ucontrol->value.integer.value[0]];
	regmap_update_bits(regmap, src_reg, 0xEE888000, src_regbit);

	return 0;
}

static const struct snd_kcontrol_new sunxi_ahub_dam_controls[] = {
	SOC_ENUM_EXT("APBIF0 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("APBIF1 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("APBIF2 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("I2S0 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("I2S1 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("I2S2 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("I2S3 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM0C0 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM0C1 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM0C2 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM1C0 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM1C1 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
	SOC_ENUM_EXT("DAM1C2 Src Select", ahub_mux, sunxi_ahub_mux_get, sunxi_ahub_mux_set),
};

static struct snd_soc_component_driver sunxi_ahub_dam_dev = {
	.name		= DRV_NAME,
	.probe		= sunxi_ahub_dam_probe,
	.controls	= sunxi_ahub_dam_controls,
	.num_controls	= ARRAY_SIZE(sunxi_ahub_dam_controls),
};

struct sunxi_dapm_widget {
	unsigned int id;
	void *priv;				/* widget specific data */
	int (*event)(void *, bool);
};

static void sunxi_ahub_dam_mux_get(unsigned int mux_seq, unsigned int *mux_src)
{
	int i;
	unsigned int reg_val;
	struct regmap *regmap = sunxi_mem.regmap;

	regmap_read(regmap, ahub_mux_reg[mux_seq], &reg_val);
	reg_val &= 0xEE888000;
	for (i = 0; i < ARRAY_SIZE(ahub_mux_values); i++)
		if (reg_val & ahub_mux_values[i])
			break;

	*mux_src = i;
}

void sunxi_ahub_dam_ctrl(bool enable,
			 unsigned int apb_num, unsigned int tdm_num, unsigned int channels)
{
	static unsigned char dam_use;
	unsigned char dam0_mask = 0x07;
	unsigned char dam1_mask = 0x70;
	unsigned int mux_src;
	struct regmap *regmap = sunxi_mem.regmap;

	SND_LOG_DEBUG(HLOG, "\n");

	if (enable)
		goto enable;
	else
		goto disable;

enable:
	sunxi_ahub_dam_mux_get(7, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use | (dam0_mask & BIT(0));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_RX0EN, 1 << DAM_CTL_RX0EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_RX0_NUM,
				   (channels - 1) << DAM_CTL_RX0_NUM);
	}
	sunxi_ahub_dam_mux_get(8, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use | (dam0_mask & BIT(1));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_RX1EN, 1 << DAM_CTL_RX1EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_RX1_NUM,
				   (channels - 1) << DAM_CTL_RX1_NUM);
	}
	sunxi_ahub_dam_mux_get(9, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use | (dam0_mask & BIT(2));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_RX2EN, 1 << DAM_CTL_RX2EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_RX2_NUM,
				   (channels - 1) << DAM_CTL_RX2_NUM);
	}

	sunxi_ahub_dam_mux_get(10, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use | (dam1_mask & BIT(4 + 0));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_RX0EN, 1 << DAM_CTL_RX0EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_RX0_NUM,
				   (channels - 1) << DAM_CTL_RX0_NUM);
	}
	sunxi_ahub_dam_mux_get(11, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use | (dam1_mask & BIT(4 + 1));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_RX1EN, 1 << DAM_CTL_RX1EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_RX1_NUM,
				   (channels - 1) << DAM_CTL_RX1_NUM);
	}
	sunxi_ahub_dam_mux_get(12, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use | (dam1_mask & BIT(4 + 2));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_RX2EN, 1 << DAM_CTL_RX2EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_RX2_NUM,
				   (channels - 1) << DAM_CTL_RX2_NUM);
	}

	if (dam_use & dam0_mask) {
		regmap_update_bits(regmap, SUNXI_AHUB_RST, 1 << DAM0_RST, 1 << DAM0_RST);
		regmap_update_bits(regmap, SUNXI_AHUB_GAT, 1 << DAM0_GAT, 1 << DAM0_GAT);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_TX_NUM,
				   (channels - 1) << DAM_CTL_TX_NUM);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_TXEN, 1 << DAM_CTL_TXEN);
	}
	if (dam_use & dam1_mask) {
		regmap_update_bits(regmap, SUNXI_AHUB_RST, 1 << DAM1_RST, 1 << DAM1_RST);
		regmap_update_bits(regmap, SUNXI_AHUB_GAT, 1 << DAM1_GAT, 1 << DAM1_GAT);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_TX_NUM,
				   (channels - 1) << DAM_CTL_TX_NUM);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_TXEN, 1 << DAM_CTL_TXEN);
	}

	return;

disable:
	sunxi_ahub_dam_mux_get(7, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use & (dam0_mask & (~BIT(0)));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_RX0EN, 0 << DAM_CTL_RX0EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_RX0_NUM, 0x0 << DAM_CTL_RX0_NUM);
	}
	sunxi_ahub_dam_mux_get(8, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use & (dam0_mask & (~BIT(1)));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_RX1EN, 0 << DAM_CTL_RX1EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_RX1_NUM, 0x0 << DAM_CTL_RX1_NUM);
	}
	sunxi_ahub_dam_mux_get(9, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use & (dam0_mask & (~BIT(2)));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_RX2EN, 0 << DAM_CTL_RX2EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_RX2_NUM, 0x0 << DAM_CTL_RX2_NUM);
	}

	sunxi_ahub_dam_mux_get(10, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use & (dam1_mask & (~BIT(4 + 0)));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_RX0EN, 0 << DAM_CTL_RX0EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_RX0_NUM, 0x0 << DAM_CTL_RX0_NUM);
	}
	sunxi_ahub_dam_mux_get(11, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use & (dam1_mask & (~BIT(4 + 1)));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_RX1EN, 0 << DAM_CTL_RX1EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_RX1_NUM, 0x0 << DAM_CTL_RX1_NUM);
	}
	sunxi_ahub_dam_mux_get(12, &mux_src);
	if (mux_src == (apb_num + 1) || mux_src == (tdm_num + 4)) {
		dam_use = dam_use & (dam1_mask & (~BIT(4 + 2)));
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_RX2EN, 0 << DAM_CTL_RX2EN);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_RX2_NUM, 0x0 << DAM_CTL_RX2_NUM);
	}

	if (!(dam_use & dam0_mask)) {
		regmap_update_bits(regmap, SUNXI_AHUB_RST, 1 << DAM0_RST, 0 << DAM0_RST);
		regmap_update_bits(regmap, SUNXI_AHUB_GAT, 1 << DAM0_GAT, 0 << DAM0_GAT);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   0xF << DAM_CTL_TX_NUM, 0x0 << DAM_CTL_TX_NUM);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(0),
				   1 << DAM_CTL_TXEN, 0 << DAM_CTL_TXEN);
	}
	if (!(dam_use & dam1_mask)) {
		regmap_update_bits(regmap, SUNXI_AHUB_RST, 1 << DAM1_RST, 0 << DAM1_RST);
		regmap_update_bits(regmap, SUNXI_AHUB_GAT, 1 << DAM1_GAT, 0 << DAM1_GAT);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   0xF << DAM_CTL_TX_NUM, 0x0 << DAM_CTL_TX_NUM);
		regmap_update_bits(regmap, SUNXI_AHUB_DAM_CTL(1),
				   1 << DAM_CTL_TXEN, 0 << DAM_CTL_TXEN);
	}
	return;
}
EXPORT_SYMBOL_GPL(sunxi_ahub_dam_ctrl);

/*******************************************************************************
 * for kernel source
 ******************************************************************************/
int snd_soc_sunxi_ahub_mem_get(struct sunxi_ahub_mem *mem)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (IS_ERR_OR_NULL(sunxi_mem.regmap)) {
		SND_LOG_ERR(HLOG, "regmap is invalid\n");
		return -EINVAL;
	}
	if (IS_ERR_OR_NULL(sunxi_mem.res)) {
		SND_LOG_ERR(HLOG, "res is invalid\n");
		return -EINVAL;
	}

	mem->regmap = sunxi_mem.regmap;
	mem->res = sunxi_mem.res;

	return 0;
}
EXPORT_SYMBOL_GPL(snd_soc_sunxi_ahub_mem_get);

int snd_soc_sunxi_ahub_clk_get(struct sunxi_ahub_clk *clk)
{
	SND_LOG_DEBUG(HLOG, "\n");

	if (IS_ERR_OR_NULL(sunxi_clk.clk_pll)) {
		SND_LOG_ERR(HLOG, "clk_pll is invalid\n");
		return -EINVAL;
	}
	if (IS_ERR_OR_NULL(sunxi_clk.clk_pllx4)) {
		SND_LOG_ERR(HLOG, "clk_pllx4 is invalid\n");
		return -EINVAL;
	}
	if (IS_ERR_OR_NULL(sunxi_clk.clk_module)) {
		SND_LOG_ERR(HLOG, "clk_module is invalid\n");
		return -EINVAL;
	}

	clk->clk_pll = sunxi_clk.clk_pll;
	clk->clk_pllx4 = sunxi_clk.clk_pllx4;
	clk->clk_module = sunxi_clk.clk_module;

	return 0;
}
EXPORT_SYMBOL_GPL(snd_soc_sunxi_ahub_clk_get);

static int snd_soc_sunxi_ahub_mem_init(struct platform_device *pdev,
				       struct device_node *np,
				       struct sunxi_ahub_mem *mem)
{
	int ret = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = of_address_to_resource(np, 0, mem->res);
	if (ret) {
		SND_LOG_ERR(HLOG, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem->memregion = devm_request_mem_region(&pdev->dev,
						      mem->res->start,
						      resource_size(mem->res),
						      DRV_NAME);
	if (IS_ERR_OR_NULL(mem->memregion)) {
		SND_LOG_ERR(HLOG, "memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem->membase = devm_ioremap(&pdev->dev,
					 mem->memregion->start,
					 resource_size(mem->memregion));
	if (IS_ERR_OR_NULL(mem->membase)) {
		SND_LOG_ERR(HLOG, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	mem->regmap = devm_regmap_init_mmio(&pdev->dev,
						 mem->membase,
						 &sunxi_regmap_config);
	if (IS_ERR_OR_NULL(mem->regmap)) {
		SND_LOG_ERR(HLOG, "regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem->memregion->start,
				resource_size(mem->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	return ret;
};

static int snd_soc_sunxi_ahub_clk_init(struct platform_device *pdev,
				       struct device_node *np,
				       struct sunxi_ahub_clk *clk)
{
	int ret = 0;

	SND_LOG_DEBUG(HLOG, "\n");

	/* get clk of ahub */
	clk->clk_module = of_clk_get(np, CLK_AUDIO_HUB_NUM);
	if (IS_ERR_OR_NULL(clk->clk_module)) {
		SND_LOG_ERR(HLOG, "clk module get failed\n");
		ret = -EBUSY;
		goto err_module_clk;
	}
	clk->clk_pll = of_clk_get(np, CLK_PLL_AUDIO_NUM);
	if (IS_ERR_OR_NULL(clk->clk_pll)) {
		SND_LOG_ERR(HLOG, "clk pll get failed\n");
		ret = -EBUSY;
		goto err_pll_clk;
	}
	clk->clk_pllx4 = of_clk_get(np, CLK_PLL_AUDIOX4_NUM);
	if (IS_ERR_OR_NULL(clk->clk_pllx4)) {
		SND_LOG_ERR(HLOG, "clk pllx4 get failed\n");
		ret = -EBUSY;
		goto err_pllx4_clk;
	}

	/* set ahub clk parent */
	if (clk_set_parent(clk->clk_module, clk->clk_pllx4)) {
		SND_LOG_ERR(HLOG, "set parent of clk_module to pllx4 failed\n");
		ret = -EINVAL;
		goto err_set_parent_clk;
	}

	/* enable clk of ahub */
	if (clk_prepare_enable(clk->clk_pll)) {
		SND_LOG_ERR(HLOG, "clk_pll enable failed\n");
		ret = -EBUSY;
		goto err_pll_clk_enable;
	}
	if (clk_prepare_enable(clk->clk_pllx4)) {
		SND_LOG_ERR(HLOG, "clk_pllx4 enable failed\n");
		ret = -EBUSY;
		goto err_pllx4_clk_enable;
	}
	if (clk_prepare_enable(clk->clk_module)) {
		SND_LOG_ERR(HLOG, "clk_module enable failed\n");
		ret = -EBUSY;
		goto err_module_clk_enable;
	}

	return 0;

err_module_clk_enable:
	clk_disable_unprepare(clk->clk_pllx4);
err_pllx4_clk_enable:
	clk_disable_unprepare(clk->clk_pll);
err_pll_clk_enable:
err_set_parent_clk:
	clk_put(clk->clk_pllx4);
err_pllx4_clk:
	clk_put(clk->clk_pll);
err_pll_clk:
	clk_put(clk->clk_module);
err_module_clk:
	return ret;
}

/* sysfs debug */
static ssize_t snd_sunxi_debug_show_reg(struct device *dev, struct device_attribute *attr,
					char *buf)
{
	size_t count = 0;

	count += sprintf(buf + count, "usage->read : echo [num] > audio_reg\n");
	count += sprintf(buf + count, "usage->write: echo [reg] [value] > audio_reg\n");

	count += sprintf(buf + count, "num: 0.public\n");
	count += sprintf(buf + count, "num: 1.APBIF0 2.APBIF1 3.APBIF2\n");
	count += sprintf(buf + count, "num: 4.I2S0   5.I2S1   6.I2S2   7.I2S3\n");
	count += sprintf(buf + count, "num: 8.DAM0   9.DAM1   a.all\n");

	return count;
}

static ssize_t snd_sunxi_debug_store_reg(struct device *dev, struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct regmap *regmap = sunxi_mem.regmap;
	int scanf_cnt;
	unsigned int num = 0;
	unsigned int i = 0, j = 0;
	unsigned int input_reg_val;
	unsigned int input_reg_offset;
	unsigned int output_reg_val;

	if (buf[1] == 'x')
		scanf_cnt = sscanf(buf, "0x%x 0x%x", &input_reg_offset, &input_reg_val);
	else
		scanf_cnt = sscanf(buf, "%x", &num);

	if (scanf_cnt <= 0 || num > 10 || num < 0) {
		pr_err("please get the usage by\"cat audio_reg\"\n");
		return count;
	}

	/* echo [num] > audio_reg */
	if (scanf_cnt == 1 && num != 10) {
		while (ahub_reg_all[num][i].name != NULL) {
			regmap_read(regmap, ahub_reg_all[num][i].address, &output_reg_val);
			pr_info("%-32s [0x%03x]: 0x%8x :0x%x\n",
				ahub_reg_all[num][i].name,
				ahub_reg_all[num][i].address, output_reg_val,
				ahub_reg_all[num][i].value);
			i++;
		}
		return count;
	} else if (num == 10) {
		while (i < 10) {
			while (ahub_reg_all[i][j].name != NULL) {
				regmap_read(regmap, ahub_reg_all[i][j].address, &output_reg_val);
				pr_info("%-32s [0x%03x]: 0x%8x :0x%x\n",
					ahub_reg_all[i][j].name,
					ahub_reg_all[i][j].address, output_reg_val,
					ahub_reg_all[i][j].value);
				j++;
			}
			pr_info("\n");
			j = 0;
			i++;
		}
		return count;
	}

	if (input_reg_offset > SUNXI_AHUB_MAX_REG) {
		pr_err("reg offset > ahub max reg[0x%x]\n", SUNXI_AHUB_MAX_REG);
		return count;
	}

	/* echo [offset] [value] > audio_reg */
	regmap_read(regmap, input_reg_offset, &output_reg_val);
	pr_info("reg[0x%03x]: 0x%x (old)\n", input_reg_offset, output_reg_val);
	regmap_write(regmap, input_reg_offset, input_reg_val);
	regmap_read(regmap, input_reg_offset, &output_reg_val);
	pr_info("reg[0x%03x]: 0x%x (new)\n", input_reg_offset, output_reg_val);
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

static int sunxi_ahub_dam_dev_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;

	SND_LOG_DEBUG(HLOG, "\n");

	ret = snd_soc_sunxi_ahub_mem_init(pdev, np, &sunxi_mem);
	if (ret) {
		SND_LOG_ERR(HLOG, "remap init failed\n");
		ret = -EINVAL;
		goto err_snd_soc_sunxi_ahub_mem_init;
	}

	ret = snd_soc_sunxi_ahub_clk_init(pdev, np, &sunxi_clk);
	if (ret) {
		SND_LOG_ERR(HLOG, "clk init failed\n");
		ret = -EINVAL;
		goto err_snd_soc_sunxi_ahub_clk_init;
	}

	ret = snd_soc_register_component(&pdev->dev,
					 &sunxi_ahub_dam_dev,
					 &sunxi_ahub_dam_dai, 1);
	if (ret) {
		SND_LOG_ERR(HLOG, "component register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_component;
	}

	ret = snd_soc_register_platform(&pdev->dev, &sunxi_ahub_dam_platform);
	if (ret) {
		SND_LOG_ERR(HLOG, "platform register failed\n");
		ret = -ENOMEM;
		goto err_snd_soc_register_platform;
	}

	ret = snd_sunxi_sysfs_create_group(pdev, &debug_attr);
	if (ret)
		SND_LOG_WARN(HLOG, "sysfs debug create failed\n");

	SND_LOG_DEBUG(HLOG, "register ahub_dam platform success\n");

	return 0;

err_snd_soc_register_platform:
err_snd_soc_register_component:
err_snd_soc_sunxi_ahub_clk_init:
err_snd_soc_sunxi_ahub_mem_init:
	of_node_put(np);
	return ret;
}

static int sunxi_ahub_dam_dev_remove(struct platform_device *pdev)
{
	struct sunxi_ahub_mem *mem = &sunxi_mem;
	struct sunxi_ahub_clk *clk = &sunxi_clk;

	SND_LOG_DEBUG(HLOG, "\n");

	snd_sunxi_sysfs_remove_group(pdev, &debug_attr);
	snd_soc_unregister_platform(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);

	devm_iounmap(&pdev->dev, mem->membase);
	devm_release_mem_region(&pdev->dev, mem->memregion->start, resource_size(mem->memregion));

	clk_disable_unprepare(clk->clk_module);
	clk_put(clk->clk_module);
	clk_disable_unprepare(clk->clk_pll);
	clk_put(clk->clk_pll);
	clk_disable_unprepare(clk->clk_pllx4);
	clk_put(clk->clk_pllx4);

	SND_LOG_DEBUG(HLOG, "unregister ahub_dam platform success\n");

	return 0;
}

static const struct of_device_id sunxi_ahub_dam_of_match[] = {
	{ .compatible = "allwinner," DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_ahub_dam_of_match);

static struct platform_driver sunxi_ahub_dam_driver = {
	.driver	= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= sunxi_ahub_dam_of_match,
	},
	.probe	= sunxi_ahub_dam_dev_probe,
	.remove	= sunxi_ahub_dam_dev_remove,
};

int __init sunxi_ahub_dam_dev_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_ahub_dam_driver);
	if (ret != 0) {
		SND_LOG_ERR(HLOG, "platform driver register failed\n");
		return -EINVAL;
	}

	return ret;
}

void __exit sunxi_ahub_dam_dev_exit(void)
{
	platform_driver_unregister(&sunxi_ahub_dam_driver);
}

late_initcall(sunxi_ahub_dam_dev_init);
module_exit(sunxi_ahub_dam_dev_exit);

MODULE_AUTHOR("Dby@allwinnertech.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sunxi soundcard platform of ahub_dam");
