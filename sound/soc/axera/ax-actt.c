/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <sound/designware_i2s.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>
#include "local.h"
#include "ax-misc.h"
#include <linux/kthread.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regmap.h>
#include "ax-actt.h"
#include <linux/reset.h>

struct ax_actt_coeff_div {
	u32 rate;       /* sample rate */
	u32 bit_width;
	u32 chn_num;
	u32 sclk;
};

struct ax_mic_gpio {
	int l_p;       /* sample rate */
	int l_n;
	int r_n;
	int r_p;
};
struct ax_tx_pa_gpio {
	int pa_speaker;
	int pa_linout;
};

struct ax_actt_dev {
	void __iomem *actt_base;
	struct clk *actt_clk;
	struct clk *actt_tlb_clk;
	struct regmap *regmap;
	u32 bit_width;
	u32 samplerate_h;
	u32 samplerate_l;
	u32 blk_frequency;
	u32 format;
	u32 clksel;
	char rx_again_l;
	char rx_again_r;
	char alc_status;
	char rx_bypass1;
	char rx_bypass2;
	char rx_3d;
	char tx_bypass1;
	char tx_bypass2;
	char tx_3d;
	struct ax_tx_pa_gpio pa_gpio;
	struct reset_control *prst;
	struct reset_control *rst;
};

static const struct ax_actt_coeff_div actt_coeff_div[] = {
	/* rate  bit_width  chn_num  sclk */
	{96000, SNDRV_PCM_FORMAT_S32_LE, 2, 0x00},
	{96000, SNDRV_PCM_FORMAT_S24_LE, 2, 0x00},
	{96000, SNDRV_PCM_FORMAT_S16_LE, 2, 0x01},
	{48000, SNDRV_PCM_FORMAT_S32_LE, 2, 0x01},
	{48000, SNDRV_PCM_FORMAT_S24_LE, 2, 0x01},
	{48000, SNDRV_PCM_FORMAT_S16_LE, 2, 0x02},
	{32000, SNDRV_PCM_FORMAT_S32_LE, 2, 0x01},
	{32000, SNDRV_PCM_FORMAT_S24_LE, 2, 0x02},
	{32000, SNDRV_PCM_FORMAT_S16_LE, 2, 0x02},
	{24000, SNDRV_PCM_FORMAT_S32_LE, 2, 0x02},
	{24000, SNDRV_PCM_FORMAT_S24_LE, 2, 0x02},
	{24000, SNDRV_PCM_FORMAT_S16_LE, 2, 0x03},
	{16000, SNDRV_PCM_FORMAT_S32_LE, 2, 0x02},
	{16000, SNDRV_PCM_FORMAT_S24_LE, 2, 0x03},
	{16000, SNDRV_PCM_FORMAT_S16_LE, 2, 0x03},
	{8000, SNDRV_PCM_FORMAT_S32_LE, 2, 0x03},
	{8000, SNDRV_PCM_FORMAT_S24_LE, 2, 0x03},
	{8000, SNDRV_PCM_FORMAT_S16_LE, 2, 0x03},
};

static const int actt_rx_iir_addr[] = {0X74, 0X7e, 0X88, 0X92, 0X9c, 0x60, 0x6a}; /* 60-lfp, 6a-hfp */
static const int actt_tx_iir_addr[] = {0Xc4, 0Xce, 0Xd8, 0Xe2, 0Xec, 0Xb0, 0Xba}; /* b0-lfp, ba-hfp */

#define ACTT_RATES (SNDRV_PCM_RATE_192000 | \
		SNDRV_PCM_RATE_96000 | \
		SNDRV_PCM_RATE_88200 | \
		SNDRV_PCM_RATE_8000_48000)
#define ACTT_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
		SNDRV_PCM_FMTBIT_S18_3LE | \
		SNDRV_PCM_FMTBIT_S20_3LE | \
		SNDRV_PCM_FMTBIT_S24_LE | \
		SNDRV_PCM_FMTBIT_S32_LE)

static inline void actt_write_reg(void __iomem *io_base, int reg, u32 val)
{
	writel(val, io_base + reg);
}

static inline u32 actt_read_reg(void __iomem *io_base, int reg)
{
	return readl(io_base + reg);
}

static int actt_startup(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	pr_debug("bengin actt_startup");
	return 0;
}

static int actt_get_coeff(int rate, int bit_width)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(actt_coeff_div); i++) {
		if (actt_coeff_div[i].rate == rate && actt_coeff_div[i].bit_width == bit_width)
			return i;
	}
	return -EINVAL;
}

static int actt_get_params(struct snd_pcm_hw_params *params,
			      struct ax_actt_dev *dev)
{
	int coeff;

	if(NULL == dev) {
		pr_err("actt_get_bit_width, get dev info failed.\n");
	}

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		dev->bit_width = 0x00;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		dev->bit_width = 0x08;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		dev->bit_width = 0x0c;
		break;
	default:
		pr_err("actt: unsupported PCM fmt  %d",
			params_format(params));
		return -EINVAL;
	}

	switch (params_rate(params)) {
	case 8000:
		dev->samplerate_h = 1;
		dev->samplerate_l = 127;
		break;
	case 16000:
		dev->samplerate_h = 0;
		dev->samplerate_l = 191;
		break;
	case 24000:
		dev->samplerate_h = 0;
		dev->samplerate_l = 127;
		break;
	case 32000:
		dev->samplerate_h = 0;
		dev->samplerate_l = 95;
		break;
	case 48000:
		dev->samplerate_h = 0;
		dev->samplerate_l = 63;
		break;
	case 96000:
		dev->samplerate_h = 0;
		dev->samplerate_l = 31;
		break;
	default:
		pr_err("actt: unsupported PCM samplerate_h %d",
			params_rate(params));
		return -EINVAL;
	}
	coeff = actt_get_coeff(params_rate(params),params_format(params));
	if (coeff < 0) {
		pr_err("Unable to configure sample rate %dHz with %d bit_width\n",
			params_rate(params), params_format(params));
		return coeff;
	}

	dev->blk_frequency = actt_coeff_div[coeff].sclk << 6;
	pr_debug("coeff: %d  dev->blk_frequency: %x \n",coeff, dev->blk_frequency);

	return 0;
}

static int actt_hw_params(struct snd_pcm_substream *substream,
			  struct snd_pcm_hw_params *params,
			  struct snd_soc_dai *dai)
{
	struct ax_actt_dev *dev = snd_soc_dai_get_drvdata(dai);
	u32 reg = 0;

	pr_debug("bengin actt_hw_params set.");
	if (actt_get_params(params, dev)) {
		pr_err("actt_get_params failed!\n");
		return -EINVAL;
	}

	/* when tx and rx use the same clk, tx and rx can get the clk and work normally only this reg enablede,. Or maybe suspend. */
#ifdef INTERNAL_REF
	actt_write_reg(dev->actt_base, RX_IIS_ENABLE, ENABLE);
#endif
	reg = actt_read_reg(dev->actt_base, RX_IIS_FORMAT);
	actt_write_reg(dev->actt_base, RX_IIS_FORMAT, (reg & RX_IIS_MCLK_MASK) | (dev->blk_frequency));

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		dev->rx_again_l = actt_read_reg(dev->actt_base, RX_LEFT_ANA_GAIN_SET);
		dev->rx_again_r = actt_read_reg(dev->actt_base, RX_RIGHT_ANA_GAIN_SET);
		dev->alc_status = actt_read_reg(dev->actt_base, RX_ALC_SET);

		dev->rx_bypass1 = actt_read_reg(dev->actt_base, RX_IIR_BYPASS1_ENABLE);
		dev->rx_bypass2 = actt_read_reg(dev->actt_base, RX_IIR_BYPASS2_ENABLE);
		dev->rx_3d = actt_read_reg(dev->actt_base, RX_3D_ENABLE);

		actt_write_reg(dev->actt_base, RX_LEFT_ANA_GAIN_SET, RX_ANA_GAIN);
		actt_write_reg(dev->actt_base, RX_RIGHT_ANA_GAIN_SET, RX_ANA_GAIN);
		actt_write_reg(dev->actt_base, RX_IIR_BYPASS1_ENABLE,
			       RX_IIR_BYPASS1_DEFAULT);
		actt_write_reg(dev->actt_base, RX_IIR_BYPASS2_ENABLE,
			       RX_IIR_BYPASS2_DEFAULT);
		actt_write_reg(dev->actt_base, RX_3D_ENABLE, RX_3D_DEFAULT);
		udelay(1000);
		/* set bitwidth */
		reg = actt_read_reg(dev->actt_base, RX_IIS_FORMAT);
		actt_write_reg(dev->actt_base, RX_IIS_FORMAT,
			       reg  | dev-> bit_width | dev->format);

		/* set sample */
		actt_write_reg(dev->actt_base, RX_CIC_RATE_SET_BIT_H,
			       dev->samplerate_h);
		actt_write_reg(dev->actt_base, RX_CIC_RATE_SET_BIT_L,
			       dev->samplerate_l);

		actt_write_reg(dev->actt_base, RX_IIS_CLK_SEL, IIS_CLK_SEL_DEFAULT);

	} else {
		/* set bitwidth */
		reg = actt_read_reg(dev->actt_base, TX_IIS_FORMAT);
		actt_write_reg(dev->actt_base, TX_IIS_FORMAT,
			       ((reg & TX_IIS_FORMAT_MASK) | dev->bit_width | dev->format));

		/* set sample */
		actt_write_reg(dev->actt_base, TX_CIC_RATE_SET_BIT_H,
			       dev->samplerate_h);
		actt_write_reg(dev->actt_base, TX_CIC_RATE_SET_BIT_L,
			       dev->samplerate_l);

		actt_write_reg(dev->actt_base, TX_IIS_CLK_SEL, IIS_CLK_SEL_DEFAULT);
	}

	return 0;
}

static int actt_mute(struct snd_soc_dai *dai, int mute, int direction)
{
	u32 reg = 0;
	struct ax_actt_dev *dev = snd_soc_dai_get_drvdata(dai);
	pr_debug("bengin actt_mute %d\n",mute);
	if (SNDRV_PCM_STREAM_PLAYBACK == direction) {
		reg = actt_read_reg(dev->actt_base, TX_MUTE_ENABLE);
		if (mute) {
			actt_write_reg(dev->actt_base, TX_MUTE_ENABLE, reg | TX_MUTE_ENABLE_MASK);
		} else {
			actt_write_reg(dev->actt_base, TX_MUTE_ENABLE, reg & TX_MUTE_DISABLE_MASK);
		}
	}
	return 0;
}

static int actt_set_sysclk(struct snd_soc_dai *dai,
			   int clk_id, unsigned int freq, int dir)
{
	pr_debug("bengin actt_set_sysclk");

	switch (freq) {
	case 11289600:
		pr_debug("%s, freq: %u\n", __func__, freq);
		break;
	case 12288000:
		pr_debug("%s, freq: %u\n", __func__, freq);
		break;
	default:
		pr_err("%s, actt don't support freq: %u\n", __func__, freq);
		return -EINVAL;
	}

	return 0;
}

static int actt_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct ax_actt_dev *dev = snd_soc_dai_get_drvdata(dai);
	pr_debug("%s, fmt: 0x%x\n", __func__, fmt);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		pr_debug("%s, actt rx only support master fmt_mode: 0x%x\n",
			__func__, SND_SOC_DAIFMT_CBM_CFM);
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		pr_debug("%s, actt tx only support salve .fmt_mode: 0x%x\n",
			__func__, SND_SOC_DAIFMT_CBS_CFS);
		break;
	default:
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		pr_debug("actt in I2S Format\n");
		dev->format = IIS_FORMAT_DEFAULT;
		dev->clksel = IIS_CLK_SEL_DEFAULT;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		return -EINVAL;
	case SND_SOC_DAIFMT_LEFT_J:
		pr_debug("actt in LJ Format\n");
		break;
	case SND_SOC_DAIFMT_DSP_A:
		pr_debug("actt in DSP-A Format\n");
		break;
	case SND_SOC_DAIFMT_DSP_B:
		pr_debug("actt in DSP-B Format\n");
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int actt_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct ax_actt_dev *dev = snd_soc_dai_get_drvdata(dai);

	pr_debug("bengin actt_hw_free");
	return 0;
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		actt_write_reg(dev->actt_base, RX_DEAL_ENABLE, DISABLE);
	} else {
		actt_write_reg(dev->actt_base, TX_DEAL_ENABLE, DISABLE);
		actt_write_reg(dev->actt_base, TX_IIS_ENABLE, DISABLE);
	}

	return 0;
}

static const struct snd_soc_dai_ops actt_dai_ops = {
	.startup = actt_startup,
	.hw_params = actt_hw_params,
	.mute_stream = actt_mute,
	.set_sysclk = actt_set_sysclk,
	.set_fmt = actt_set_dai_fmt,
	//.no_capture_mute = 1,
	.hw_free = actt_hw_free,
};

static int actt_set_bias_level(struct snd_soc_component *component,
			       enum snd_soc_bias_level level)
{
	pr_debug("actt_set_bias_level %d\n",level);

	switch (level) {
	case SND_SOC_BIAS_ON:
		/* asic suggest enable reg as test case. if enable here, it will be revert(rx tx same time). */
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

static int actt_component_probe(struct snd_soc_component *component)
{
	pr_debug("actt_component_probe\n ");
	return 0;
}

static void actt_remove(struct snd_soc_component *component)
{
	actt_set_bias_level(component,SND_SOC_BIAS_OFF);
	pr_debug("actt_remove\n ");
}

static int actt_resume(struct snd_soc_component *component)
{
	int ret;
	struct ax_actt_dev *dev = snd_soc_component_get_drvdata(component);

	ret = clk_prepare_enable(dev->actt_clk);
	if (ret < 0) {
		pr_err("actt_clk prepare failed\n");
		return ret;
	}

	reset_control_deassert(dev->rst);
	return 0;
}

static int actt_suspend(struct snd_soc_component *component)
{
	struct ax_actt_dev *dev = snd_soc_component_get_drvdata(component);

	clk_disable_unprepare(dev->actt_clk);

	reset_control_assert(dev->rst);
	return 0;
}

static int actt_iir_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol, int reg)
{
	struct snd_soc_component *component =
			snd_soc_kcontrol_component(kcontrol);
	struct soc_bytes_ext *params = (void *)kcontrol->private_value;
	int i, REG, ret, reg_val;
	u8 *val;
	REG = reg << 2;

	val = (u8 *)ucontrol->value.bytes.data;
	for (i = 0; i < params->max; i++) {
		ret = snd_soc_component_read(component, (REG + 0x4 * i), &reg_val);
		if (ret < 0) {
			pr_err("snd_soc_component_read failed. ret = %d.\n", ret);
			return ret;
		}
		/*
		 * conversion of 16-bit integers between native CPU format and big endian format
		 */
		memcpy(val + i, (u8*)&reg_val, sizeof(u8));
	}

	return 0;
}

static int actt_iir_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol, int reg)
{
	struct snd_soc_component *component =
			snd_soc_kcontrol_component(kcontrol);
	struct soc_bytes_ext *params = (void *)kcontrol->private_value;
	void *data;
	u8 *val;
	int i, ret, REG;

	data = kmemdup(ucontrol->value.bytes.data,
		params->max, GFP_KERNEL | GFP_DMA);
	if (!data)
		return -ENOMEM;
	REG = reg << 2;
	val = (u8 *)data;
	for (i = 0; i < params->max; i++) {
		ret = snd_soc_component_write(component, (REG + 0X4 * i), *(val + i));
		if (ret) {
			dev_err(component->dev, "EQ configuration fail, register: %x ret: %d\n", reg + 0x4, ret);
			kfree(data);
			return ret;
		}
	}
	kfree(data);
	return 0;
}

static int actt_rx_eq_band0_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_rx_iir_addr[0]);
	return 0;
}

static int actt_rx_eq_band0_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_rx_iir_addr[0]);
	return 0;
}
static int actt_rx_eq_band1_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_rx_iir_addr[1]);
	return 0;
}

static int actt_rx_eq_band1_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_rx_iir_addr[1]);
	return 0;
}
static int actt_rx_eq_band2_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_rx_iir_addr[2]);
	return 0;
}

static int actt_rx_eq_band2_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_rx_iir_addr[2]);
	return 0;
}
static int actt_rx_eq_band3_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_rx_iir_addr[3]);
	return 0;
}

static int actt_rx_eq_band3_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_rx_iir_addr[3]);
	return 0;
}
static int actt_rx_eq_band4_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_rx_iir_addr[4]);
	return 0;
}

static int actt_rx_eq_band4_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_rx_iir_addr[4]);
	return 0;
}
static int actt_rx_lfp_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_rx_iir_addr[5]);
	return 0;
}

static int actt_rx_lfp_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_rx_iir_addr[5]);
	return 0;
}
static int actt_rx_hfp_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_rx_iir_addr[6]);
	return 0;
}

static int actt_rx_hfp_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_rx_iir_addr[6]);
	return 0;
}


static int actt_tx_eq_band0_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_tx_iir_addr[0]);
	return 0;
}

static int actt_tx_eq_band0_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_tx_iir_addr[0]);
	return 0;
}
static int actt_tx_eq_band1_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_tx_iir_addr[1]);
	return 0;
}

static int actt_tx_eq_band1_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_tx_iir_addr[1]);
	return 0;
}
static int actt_tx_eq_band2_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_tx_iir_addr[2]);
	return 0;
}

static int actt_tx_eq_band2_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_tx_iir_addr[2]);
	return 0;
}
static int actt_tx_eq_band3_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_tx_iir_addr[3]);
	return 0;
}

static int actt_tx_eq_band3_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_tx_iir_addr[3]);
	return 0;
}
static int actt_tx_eq_band4_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_tx_iir_addr[4]);
	return 0;
}

static int actt_tx_eq_band4_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_tx_iir_addr[4]);
	return 0;
}
static int actt_tx_lfp_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_tx_iir_addr[5]);
	return 0;
}

static int actt_tx_lfp_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_tx_iir_addr[5]);
	return 0;
}
static int actt_tx_hfp_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_get(kcontrol,ucontrol, actt_tx_iir_addr[6]);
	return 0;
}

static int actt_tx_hfp_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	actt_iir_put(kcontrol,ucontrol, actt_tx_iir_addr[6]);
	return 0;
}


static const DECLARE_TLV_DB_SCALE(rx_l_ana_tlv,0,50,0);
static const DECLARE_TLV_DB_SCALE(rx_r_ana_tlv,0,50,0);
static const DECLARE_TLV_DB_SCALE(rx_l_dig_tlv,0,50,0);
static const DECLARE_TLV_DB_SCALE(rx_r_dig_tlv,0,50,0);
static const DECLARE_TLV_DB_SCALE(alc_target_level_tlv,-1000,-30,0);
static const DECLARE_TLV_DB_SCALE(alc_rms_max_tlv,-500,-25,0);
static const DECLARE_TLV_DB_SCALE(alc_gate_threshold_tlv,-8000,20,0);
static const DECLARE_TLV_DB_SCALE(alc_gain_max_tlv,1000,30,0);
static const DECLARE_TLV_DB_SCALE(tx_l_ana_tlv,-1800,37,0);
static const DECLARE_TLV_DB_SCALE(tx_r_ana_tlv,-1800,37,0);
static const DECLARE_TLV_DB_SCALE(tx_l_dig_tlv,0,50,0);
static const DECLARE_TLV_DB_SCALE(tx_r_dig_tlv,0,50,0);

static const struct snd_kcontrol_new actt_snd_controls[] = {
	SOC_SINGLE("ALC ENABLE", RX_ALC_SET,7, 1, 0), /* 0 -> manual a and d gain; 1-> auto */
	SOC_SINGLE_TLV("RX LEFT ANA GAIN", RX_LEFT_ANA_GAIN_SET, 0, 0X36, 0, rx_l_ana_tlv),
	SOC_SINGLE_TLV("RX RIGHT ANA GAIN", RX_RIGHT_ANA_GAIN_SET, 0, 0X36, 0, rx_r_ana_tlv),
	SOC_SINGLE_TLV("RX LEFT DIG GAIN", RX_LEFT_DIG_GAIN_SET, 0, 0X78, 0, rx_l_dig_tlv),
	SOC_SINGLE_TLV("RX RIGHT DIG GAIN", RX_RIGHT_DIG_GAIN_SET, 0, 0X78, 0, rx_r_dig_tlv),
	SOC_SINGLE_TLV("ALC TARGET LEVEL", RX_ALC_TARGET_LEVEL,0, 20, 0, alc_target_level_tlv),
	SOC_SINGLE_TLV("ALC RMS MAX", RX_ALC_RMS_MAX,0, 20, 0, alc_rms_max_tlv),
	SOC_SINGLE_TLV("ALC GATE THRESHOLD", RX_ALC_GATE_THRESHOLD_LEVEL,0, 200, 0, alc_gate_threshold_tlv),
	SOC_SINGLE_TLV("ALC GAIN MAX", RX_ALC_GAIN_MAX,0, 200, 0, alc_gain_max_tlv),
	SOC_SINGLE("ADC MUTE", RX_MUTE_ENABLE,7, 1, 0),
	SOC_SINGLE("DAC MUTE", TX_MUTE_ENABLE,4, 1, 0),
	SOC_SINGLE_TLV("TX LEFT ANA GAIN", TX_LEFT_ANA_GAIN_SET,0, 0X31, 0, tx_l_ana_tlv),
	SOC_SINGLE_TLV("TX RIGHT ANA GAIN", TX_RIGHT_ANA_GAIN_SET,0, 0X31, 0, tx_r_ana_tlv),
	SOC_SINGLE_TLV("TX LEFT DIG GAIN", TX_LEFT_DIG_GAIN_SET,0, 0X78, 0, tx_l_dig_tlv),
	SOC_SINGLE_TLV("TX RIGHT DIG GAIN", TX_RIGHT_DIG_GAIN_SET,0, 0X78, 0,tx_r_dig_tlv),
	SOC_SINGLE("TX LR SWAP", TX_IIS_FORMAT,0,1,0),
	SND_SOC_BYTES_EXT("RX LPF Parameters", 10, actt_rx_lfp_get, actt_rx_lfp_put),
	SND_SOC_BYTES_EXT("RX HPF Parameters", 10, actt_rx_hfp_get, actt_rx_hfp_put),
	SND_SOC_BYTES_EXT("RX EQ Parameters Band 0", 10, actt_rx_eq_band0_get, actt_rx_eq_band0_put),
	SND_SOC_BYTES_EXT("RX EQ Parameters Band 1", 10, actt_rx_eq_band1_get, actt_rx_eq_band1_put),
	SND_SOC_BYTES_EXT("RX EQ Parameters Band 2", 10, actt_rx_eq_band2_get, actt_rx_eq_band2_put),
	SND_SOC_BYTES_EXT("RX EQ Parameters Band 3", 10, actt_rx_eq_band3_get, actt_rx_eq_band3_put),
	SND_SOC_BYTES_EXT("RX EQ Parameters Band 4", 10, actt_rx_eq_band4_get, actt_rx_eq_band4_put),
	SOC_SINGLE("RX LPF BYPASS", RX_IIR_BYPASS1_ENABLE, 5, 1, 0),
	SOC_SINGLE("RX HPF BYPASS", RX_IIR_BYPASS1_ENABLE, 4, 1, 0),
	SOC_SINGLE("RX EQ BYPASS Band 0", RX_IIR_BYPASS1_ENABLE, 2, 1, 0),
	SOC_SINGLE("RX EQ BYPASS Band 1", RX_IIR_BYPASS1_ENABLE, 1, 1, 0),
	SOC_SINGLE("RX EQ BYPASS Band 2", RX_IIR_BYPASS1_ENABLE, 0, 1, 0),
	SOC_SINGLE("RX EQ BYPASS Band 3", RX_IIR_BYPASS2_ENABLE, 1, 1, 0),
	SOC_SINGLE("RX EQ BYPASS Band 4", RX_IIR_BYPASS2_ENABLE, 0, 1, 0),
	SND_SOC_BYTES_EXT("TX LPF Parameters", 10, actt_tx_lfp_get, actt_tx_lfp_put),
	SND_SOC_BYTES_EXT("TX HPF Parameters", 10, actt_tx_hfp_get, actt_tx_hfp_put),
	SND_SOC_BYTES_EXT("TX EQ Parameters Band 0", 10, actt_tx_eq_band0_get, actt_tx_eq_band0_put),
	SND_SOC_BYTES_EXT("TX EQ Parameters Band 1", 10, actt_tx_eq_band1_get, actt_tx_eq_band1_put),
	SND_SOC_BYTES_EXT("TX EQ Parameters Band 2", 10, actt_tx_eq_band2_get, actt_tx_eq_band2_put),
	SND_SOC_BYTES_EXT("TX EQ Parameters Band 3", 10, actt_tx_eq_band3_get, actt_tx_eq_band3_put),
	SND_SOC_BYTES_EXT("TX EQ Parameters Band 4", 10, actt_tx_eq_band4_get, actt_tx_eq_band4_put),
	SOC_SINGLE("TX LPF BYPASS", TX_IIR_BYPASS1_ENABLE, 5, 1, 0),
	SOC_SINGLE("TX HPF BYPASS", TX_IIR_BYPASS1_ENABLE, 4, 1, 0),
	SOC_SINGLE("TX EQ BYPASS Band 0", TX_IIR_BYPASS1_ENABLE, 2, 1, 0),
	SOC_SINGLE("TX EQ BYPASS Band 1", TX_IIR_BYPASS1_ENABLE, 1, 1, 0),
	SOC_SINGLE("TX EQ BYPASS Band 2", TX_IIR_BYPASS1_ENABLE, 0, 1, 0),
	SOC_SINGLE("TX EQ BYPASS Band 3", TX_IIR_BYPASS2_ENABLE, 1, 1, 0),
	SOC_SINGLE("TX EQ BYPASS Band 4", TX_IIR_BYPASS2_ENABLE, 0, 1, 0),
};

static const char * const actt_input_mux_txt[] = {
	"FROM DMIC",
	"FROM AMIC"
};

static const unsigned int actt_input_mux_values[] = {
	0, 1
};

static const struct soc_enum actt_input_mux_enum =
	SOC_VALUE_ENUM_SINGLE(INPUT_SEL, 0, 1,
		ARRAY_SIZE(actt_input_mux_txt),
		actt_input_mux_txt,
		actt_input_mux_values);

static const struct snd_kcontrol_new actt_input_mux_controls =
	SOC_DAPM_ENUM("INPUT ROUTE", actt_input_mux_enum);


static const struct snd_kcontrol_new actt_amic_control =
	SOC_DAPM_SINGLE("Switch", INPUT_SEL, 2, 1, 0);

/* Line out switch */
static const struct snd_kcontrol_new actt_dmic_control =
	SOC_DAPM_SINGLE("Switch", DMIC_SET, 7, 1, 0);


static int actt_rx_set_mic_gpio_output(int iGpiostatus, struct ax_mic_gpio* mic_gpio)
{
	 struct device_node *node = NULL;
	 int ret = 0;
	 if (NULL == mic_gpio) {
		pr_err("mic_gpio is null\n");
		return -1;
	 }

	 node = of_find_compatible_node(NULL,NULL,"axera,ax_actt");
	 if(!node){
		pr_err("get node miclp error\n");
	 } else {
		mic_gpio->l_p = of_get_named_gpio(node, "gpio-mic-lp",0);
		ret = gpio_request(mic_gpio->l_p,NULL);
		if (ret != 0) {
			pr_err("cannot request gpio-mic-lp GPIO, %d\n",ret);
			return -1;
		}

		ret = gpio_direction_output(mic_gpio->l_p,iGpiostatus);
		if (ret != 0) {
			pr_err("cannot set gpio-mic-lp GPIO dir, %d\n",ret);
			return -1;
		}

		mic_gpio->l_n = of_get_named_gpio(node, "gpio-mic-ln",0);
		ret = gpio_request(mic_gpio->l_n,NULL);
		if (ret != 0) {
			pr_err("cannot request gpio-mic-ln GPIO\n");
			return -1;
		}

		ret = gpio_direction_output(mic_gpio->l_n,iGpiostatus);
		if (ret != 0) {
			pr_err("cannot set gpio-mic-ln GPIO dir, %d\n",ret);
			return -1;
		}

		mic_gpio->r_n = of_get_named_gpio(node, "gpio-mic-rn",0);
		ret = gpio_request(mic_gpio->r_n,NULL);
		if (ret != 0) {
			pr_err("cannot request gpio-mic-rn GPIO\n");
			return -1;
		}

		ret = gpio_direction_output(mic_gpio->r_n,iGpiostatus);
		if (ret != 0) {
			pr_err("cannot set gpio-mic-rn GPIO dir, %d\n",ret);
			return -1;
		}

		mic_gpio->r_p = of_get_named_gpio(node, "gpio-mic-rp",0);
		ret = gpio_request(mic_gpio->r_p,NULL);
		if (ret != 0) {
			pr_err("cannot request gpio-mic-rp GPIO\n");
			return -1;
		}

		ret = gpio_direction_output(mic_gpio->r_p,iGpiostatus);
		if (ret != 0) {
			pr_err("cannot set gpio-mic-rp GPIO dir, %d\n",ret);
			return -1;
		}
	 }

	 return 0;
}

static int actt_rx_set_mic_gpio_input(struct ax_mic_gpio* mic_gpio)
{
	if (NULL == mic_gpio) {
		pr_err("mic_gpio is null\n");
		return -1;
	}
	if (gpio_is_valid(mic_gpio->l_p)) {
		gpio_direction_input(mic_gpio->l_p);
		gpio_free(mic_gpio->l_p);
	}
	if (gpio_is_valid(mic_gpio->l_n)) {
		gpio_direction_input(mic_gpio->l_n);
		gpio_free(mic_gpio->l_n);
	}
	if (gpio_is_valid(mic_gpio->r_n)) {
		gpio_direction_input(mic_gpio->r_n);
		gpio_free(mic_gpio->r_n);
	}
	if (gpio_is_valid(mic_gpio->r_p)) {
		gpio_direction_input(mic_gpio->r_p);
		gpio_free(mic_gpio->r_p);
	}
	return 0;
}

static int actt_rx_amic_en(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ax_actt_dev *dev = snd_soc_component_get_drvdata(component);
	struct ax_mic_gpio mic_gpio = {0};
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (actt_rx_set_mic_gpio_output(0, &mic_gpio)) {
			pr_err("actt_rx_set_mic_gpio_output 0 failed!\n");
			return -1;
		}
		udelay(1000);
		snd_soc_component_update_bits(component, INPUT_SEL, 0x4, 0x4);
		udelay(1000);
		if (actt_rx_set_mic_gpio_input(&mic_gpio)) {
			pr_err("actt_rx_set_mic_gpio_input 0 failed!\n");
			return -1;
		}
		udelay(1000);
		actt_write_reg(dev->actt_base, RX_DEAL_ENABLE, ENABLE);
		udelay(1000);
		actt_write_reg(dev->actt_base, RX_ALC_SET, dev->alc_status);
		/* alc_en is 7 bit. alc is en, again and dgain need to be 0db */
		if ((dev->alc_status >> 7)) {
			actt_write_reg(dev->actt_base, RX_LEFT_ANA_GAIN_SET, 0);
			actt_write_reg(dev->actt_base, RX_RIGHT_ANA_GAIN_SET, 0);
			actt_write_reg(dev->actt_base, RX_LEFT_DIG_GAIN_SET, 0);
			actt_write_reg(dev->actt_base, RX_RIGHT_DIG_GAIN_SET, 0);
		} else {
			actt_write_reg(dev->actt_base, RX_LEFT_ANA_GAIN_SET, dev->rx_again_l);
			actt_write_reg(dev->actt_base, RX_RIGHT_ANA_GAIN_SET, dev->rx_again_r);
		}
		actt_write_reg(dev->actt_base, RX_IIR_BYPASS1_ENABLE, dev->rx_bypass1);
		actt_write_reg(dev->actt_base, RX_IIR_BYPASS2_ENABLE, dev->rx_bypass2);
		actt_write_reg(dev->actt_base, RX_3D_ENABLE, dev->rx_3d);
		msleep(20);
#ifdef INTERNAL_REF
		/* actt_write_reg(dev->actt_base, RX_IIS_ENABLE, ENABLE); */ /* RX_IIS_ENABLE has ben set at hw */
#else
		actt_write_reg(dev->actt_base, RX_IIS_ENABLE, ENABLE);
#endif
		pr_debug("actt_rx_amic_en! ");
		break;
	case SND_SOC_DAPM_PRE_PMD:
#ifdef INTERNAL_REF
		/* actt_write_reg(dev->actt_base, RX_IIS_ENABLE, DISABLE); */ /* RX_IIS_ENABLE cannot be dis for tx/rx use together. */
#else
		actt_write_reg(dev->actt_base, RX_IIS_ENABLE, DISABLE);
#endif
		actt_write_reg(dev->actt_base, RX_DEAL_ENABLE, DISABLE);
		snd_soc_component_update_bits(component, INPUT_SEL, 0x4, 0x0);
		pr_debug("actt_rx_amic_dis! ");
		break;
	}

	return 0;
}

static int actt_rx_dmic_en(struct snd_soc_dapm_widget *w,
			   struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ax_actt_dev *dev = snd_soc_component_get_drvdata(component);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		snd_soc_component_update_bits(component, DMIC_SET, 0x80, 0x80);
		actt_write_reg(dev->actt_base, RX_DEAL_ENABLE, ENABLE);
		udelay(1000);
		actt_write_reg(dev->actt_base, RX_ALC_SET, dev->alc_status);
		actt_write_reg(dev->actt_base, RX_LEFT_ANA_GAIN_SET, dev->rx_again_l);
		actt_write_reg(dev->actt_base, RX_RIGHT_ANA_GAIN_SET, dev->rx_again_r);
		msleep(20);
#ifdef INTERNAL_REF
		/* actt_write_reg(dev->actt_base, RX_IIS_ENABLE, ENABLE); */ /* RX_IIS_ENABLE has ben set at hw */
#else
		actt_write_reg(dev->actt_base, RX_IIS_ENABLE, ENABLE);
#endif
		pr_debug("actt_rx_dmic_en!");
		break;
	case SND_SOC_DAPM_PRE_PMD:
#ifdef INTERNAL_REF
		/* actt_write_reg(dev->actt_base, RX_IIS_ENABLE, DISABLE); */ /* RX_IIS_ENABLE cannot be dis for tx/rx use together. */
#else
		actt_write_reg(dev->actt_base, RX_IIS_ENABLE, DISABLE);
#endif
		actt_write_reg(dev->actt_base, RX_DEAL_ENABLE, DISABLE);
		snd_soc_component_update_bits(component, DMIC_SET, 0x80, 0x0);
		pr_debug("actt_rx_dmic_dis!");
		break;
	}

	return 0;
}

static int actt_tx_get_pa(int iGpiostatus, struct ax_tx_pa_gpio* pa_gpio)
{
	 struct device_node *node = NULL;
	 int ret = 0;
	 if (NULL == pa_gpio) {
		pr_err("pa_gpio is null\n");
		return -1;
	 }

	 node = of_find_compatible_node(NULL,NULL,"axera,ax_actt");
	 if(!node){
		pr_err("get node miclp error\n");
	 } else {
		pa_gpio->pa_speaker = of_get_named_gpio(node, "gpio-pa-speaker",0);
		if (pa_gpio->pa_speaker < 0) {
			pr_err("don't get pa_gpio->speaker gpio! %d\n",pa_gpio->pa_speaker);
		} else {
			ret = gpio_request(pa_gpio->pa_speaker,NULL);
			if (ret != 0) {
				pr_err("cannot request gpio-pa-speaker GPIO\n");
				return -1;
			}

			ret = gpio_direction_output(pa_gpio->pa_speaker,iGpiostatus);
			if (ret != 0) {
				pr_err("cannot set gpio-pa-speaker GPIO dir, %d\n",ret);
				return -1;
			}
		}

		pa_gpio->pa_linout = of_get_named_gpio(node, "gpio-pa-lineout",0);
		if (pa_gpio->pa_linout < 0) {
			pr_err("don't get pa_gpio->pa_linout gpio!  %d\n",pa_gpio->pa_linout);
		} else {
			ret = gpio_request(pa_gpio->pa_linout,NULL);
			if (ret != 0) {
				pr_err("cannot request gpio-pa-lineout GPIO\n");
				return -1;
			}

			ret = gpio_direction_output(pa_gpio->pa_linout,iGpiostatus);
			if (ret != 0) {
				pr_err("cannot set gpio-pa-lineout GPIO dir, %d\n",ret);
				return -1;
			}
		}
	 }

	 return 0;
}

static int actt_tx_free_pa(struct ax_tx_pa_gpio* pa_gpio)
{
	if (NULL == pa_gpio) {
		pr_err("pa_gpio is null\n");
		return -1;
	}

	if (gpio_is_valid(pa_gpio->pa_speaker)) {
		gpio_free(pa_gpio->pa_speaker);
		pr_debug("pa_gpio->pa_speaker free ok\n");
	}

	if (gpio_is_valid(pa_gpio->pa_linout)) {
		gpio_direction_input(pa_gpio->pa_linout);
		gpio_free(pa_gpio->pa_linout);
		pr_debug("pa_gpio->pa_linout free ok\n");
	}

	return 0;
}

static int actt_tx_set_pa_value(int iGpio_value, struct ax_tx_pa_gpio* pa_gpio)
{
	if (NULL == pa_gpio) {
		pr_err("pa_gpio is null\n");
		return -1;
	}

	if (gpio_is_valid(pa_gpio->pa_speaker)) {
		gpio_set_value(pa_gpio->pa_speaker,iGpio_value);
		pr_debug("pa_gpio->pa_speaker set value %d\n",iGpio_value);
	}

	if (gpio_is_valid(pa_gpio->pa_linout)) {
		gpio_set_value(pa_gpio->pa_linout,iGpio_value);
		pr_debug("pa_gpio->pa_linout set value %d\n",iGpio_value);
	}
	return 0;
}

static int actt_tx_en(struct snd_soc_dapm_widget *w,
		      struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ax_actt_dev *dev = snd_soc_component_get_drvdata(component);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		actt_write_reg(dev->actt_base, TX_IIS_ENABLE, ENABLE);
		actt_write_reg(dev->actt_base, TX_DEAL_ENABLE, ENABLE);
		udelay(1000);
		actt_tx_get_pa(0,&dev->pa_gpio);
		actt_tx_set_pa_value(1,&dev->pa_gpio);
		pr_debug("actt_tx_en!");
		break;
	case SND_SOC_DAPM_PRE_PMD:
		actt_tx_set_pa_value(0,&dev->pa_gpio);
		actt_tx_free_pa(&dev->pa_gpio);
		udelay(1000);
		actt_write_reg(dev->actt_base, TX_DEAL_ENABLE, DISABLE);
		actt_write_reg(dev->actt_base, TX_IIS_ENABLE, DISABLE);
		pr_debug("actt_tx_disable!");
		break;
	}

	return 0;
}

static const struct snd_soc_dapm_widget actt_dapm_widgets[] = {
	/* Input*/
	SND_SOC_DAPM_INPUT("AMIC"),
	SND_SOC_DAPM_INPUT("DMIC"),
	SND_SOC_DAPM_ADC_E("AMIC ENABLE", NULL, SND_SOC_NOPM, 2, 0,
			   actt_rx_amic_en, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_ADC_E("DMIC ENABLE", NULL, SND_SOC_NOPM, 7, 0,
			   actt_rx_dmic_en, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_MUX("INPUT MUX", SND_SOC_NOPM, 0, 0, &actt_input_mux_controls),
	SND_SOC_DAPM_AIF_OUT("I2S OUT", "I2S Capture", 0, SND_SOC_NOPM, 0, 0),
   /* Output */
	SND_SOC_DAPM_AIF_IN("I2S IN", "I2S Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC_E("TX ENABLE", NULL, SND_SOC_NOPM, 0, 0,
		actt_tx_en,SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_OUTPUT("DIFFERENTIAL OUT"),
};
static const struct snd_soc_dapm_route actt_dapm_routes[] = {
	/* capture */
	{"AMIC ENABLE", NULL, "AMIC"},
	{"DMIC ENABLE", NULL, "DMIC"},
	{"INPUT MUX", "FROM AMIC", "AMIC ENABLE"},
	{"INPUT MUX", "FROM DMIC", "DMIC ENABLE"},
	{"I2S OUT", NULL, "INPUT MUX"},

	/* playback */
	{"TX ENABLE", NULL, "I2S IN"},
	{"DIFFERENTIAL OUT", NULL, "TX ENABLE"},
};

static const struct snd_soc_component_driver actt_component_driver = {
	.probe = actt_component_probe,
	.remove = actt_remove,
	.suspend = actt_suspend,
	.resume = actt_resume,
	.set_bias_level = actt_set_bias_level,
	.controls             = actt_snd_controls,
	.num_controls = ARRAY_SIZE(actt_snd_controls),
	.dapm_widgets        = actt_dapm_widgets,
	.num_dapm_widgets    = ARRAY_SIZE(actt_dapm_widgets),
	.dapm_routes         = actt_dapm_routes,
	.num_dapm_routes     = ARRAY_SIZE(actt_dapm_routes),

};

static struct snd_soc_dai_driver actt_dai = {
	.name = "actt",
	.playback = {
		     .stream_name = "Playback",
		     .channels_min = 2,
		     .channels_max = 2,
		     .rates = ACTT_RATES,
		     .formats = ACTT_FORMATS,
		     },
	.capture = {
		    .stream_name = "Capture",
		    .channels_min = 2,
		    .channels_max = 2,
		    .rates = ACTT_RATES,
		    .formats = ACTT_FORMATS,
		    },
	.ops = &actt_dai_ops,
};

static int ax_actt_remove(struct platform_device *pdev)
{
	pr_debug("ax_actt_remove\n");
	return 0;
}

static const struct of_device_id ax_actt_of_match[] = {
	{.compatible = "axera,ax_actt"},
	{},
};

#if 0
static int ax_actt_runtime_resume(struct device *dev)
{
	pr_info("ax_actt_runtime_resume\n");
	return 0;
}

static int ax_actt_runtime_suspend(struct device *dev)
{
	pr_info("ax_actt_runtime_suspend\n");
	return 0;
}

static const struct dev_pm_ops ax_actt_pm_ops = {
	SET_RUNTIME_PM_OPS(ax_actt_runtime_suspend, ax_actt_runtime_resume,
			   NULL)
};
#endif

static const struct reg_default actt_reg_defaults[] = {
	{RX_IIR_BYPASS1_ENABLE, RX_IIR_BYPASS1_DEFAULT},
	{RX_IIR_BYPASS2_ENABLE, RX_IIR_BYPASS2_DEFAULT},
	{RX_3D_ENABLE, RX_3D_DEFAULT},
	{TX_IIR_BYPASS1_ENABLE, TX_IIR_BYPASS1_DEFAULT},
	{TX_IIR_BYPASS2_ENABLE, TX_IIR_BYPASS2_DEFAULT},
	{TX_3D_ENABLE, TX_3D_DEFAULT},
};
/* eq */
/* if eq is opened, you need to set the eq value, or the audio maybe disappeared.  */

static int actt_remap_write_reg(void *context, unsigned int reg, unsigned int val)
{
	void __iomem *io_base = (void __iomem *)context;

	writel(val, io_base + reg);

	return 0;
}

static int actt_remap_read_reg(void *context, unsigned int reg, unsigned int *val)
{
	void __iomem *io_base = (void __iomem *)context;

	*val = readl(io_base + reg);

	return 0;
}

const struct regmap_config actt_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= 0xfff,
	.reg_defaults = actt_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(actt_reg_defaults),
	.reg_write = actt_remap_write_reg,
	.reg_read = actt_remap_read_reg,
	.fast_io = true,
};

static int ax_actt_probe(struct platform_device *pdev)
{
	struct ax_actt_dev *dev;
	const struct of_device_id *of_id;
	int ret;
	struct resource *res;
	int reg = 0;
	pr_debug("ax_actt_probe\n");

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	of_id = of_match_device(ax_actt_of_match, &pdev->dev);
	if (!of_id)
		return -EINVAL;

	/* bus clk for innner audio codec . as pclk*/
	dev->actt_tlb_clk = devm_clk_get(&pdev->dev, "audio_tlb_clk");
	if (IS_ERR(dev->actt_tlb_clk)) {
		pr_err("get actt_tlb_clk failed\n");
		return PTR_ERR(dev->actt_tlb_clk);
	}

	ret = clk_prepare_enable(dev->actt_tlb_clk);
	if (ret < 0) {
		pr_err("actt_tlb_clk prepare failed\n");
		return ret;
	}

	udelay(1);

	dev->prst = devm_reset_control_get_optional(&pdev->dev, "prst");
	if (IS_ERR(dev->prst))
		return PTR_ERR(dev->prst);

	reset_control_deassert(dev->prst);

	dev->rst = devm_reset_control_get_optional(&pdev->dev, "rst");
	if (IS_ERR(dev->rst))
		return PTR_ERR(dev->rst);

	reset_control_deassert(dev->rst);

	/* just for work, as mclk */
	dev->actt_clk = devm_clk_get(&pdev->dev, "audio_clk");
	if (IS_ERR(dev->actt_clk)) {
		pr_err("get actt_clk failed\n");
		return PTR_ERR(dev->actt_clk);
	}

	ret = clk_prepare_enable(dev->actt_clk);
	if (ret < 0) {
		pr_err("actt_clk prepare failed\n");
		return ret;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev->actt_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dev->actt_base)) {
		pr_err("remap actt_base base failed\n");
		return PTR_ERR(dev->actt_base);
	}
	/* follow reg table default. */
	dev->bit_width = 0x08;
	dev->blk_frequency = 0x00;
	dev->samplerate_h = 0;
	dev->samplerate_l = 95;

	dev->regmap = devm_regmap_init(&pdev->dev, NULL, (void *)dev->actt_base, &actt_regmap_config);
	if (IS_ERR(dev->regmap)) {
		ret = PTR_ERR(dev->regmap);
		pr_err("Failed to init regmap: %d\n", ret);
		return ret;
	}

	dev_set_drvdata(&pdev->dev, dev);

	devm_snd_soc_register_component(&pdev->dev, &actt_component_driver,
					&actt_dai, 1);

	/* set tx again at first */
	actt_write_reg(dev->actt_base, RX_LEFT_ANA_GAIN_SET, RX_ANA_GAIN_DEFAULT);
	actt_write_reg(dev->actt_base, RX_RIGHT_ANA_GAIN_SET, RX_ANA_GAIN_DEFAULT);
	actt_write_reg(dev->actt_base, RX_LEFT_DIG_GAIN_SET, RX_DIG_GAIN_DEFAULT);
	actt_write_reg(dev->actt_base, RX_RIGHT_DIG_GAIN_SET, RX_DIG_GAIN_DEFAULT);
	actt_write_reg(dev->actt_base, TX_LEFT_ANA_GAIN_SET, TX_ANA_GAIN_DEFAULT);
	actt_write_reg(dev->actt_base, TX_RIGHT_ANA_GAIN_SET, TX_ANA_GAIN_DEFAULT);
	actt_write_reg(dev->actt_base, TX_LEFT_DIG_GAIN_SET, TX_DIG_GAIN_DEFAULT);
	actt_write_reg(dev->actt_base, TX_RIGHT_DIG_GAIN_SET, TX_DIG_GAIN_DEFAULT);
	//actt_write_reg(dev->actt_base, RX_ALC_SET, RX_ALC_DEFAULT);
	actt_write_reg(dev->actt_base, ANA_DAC_CONTROLS_0, ANA_DAC_CONTROLS_0_DEFAULT);

	reg = actt_read_reg(dev->actt_base, RX_IIS_FORMAT);
	reg = reg & RX_IIS_FORMAT_CLEAN;
	actt_write_reg(dev->actt_base, RX_IIS_FORMAT, reg);
	actt_write_reg(dev->actt_base, RX_ADC_CONTROL0, RX_ADC_CONTROL0_DEFAULT);

	return 0;
}

struct platform_driver ax_actt_driver = {
	.probe = ax_actt_probe,
	.remove = ax_actt_remove,
	.driver = {
		   .name = "designware-actt",
		   .of_match_table = of_match_ptr(ax_actt_of_match),
		   //.pm = &ax_actt_pm_ops,
		   },
};

module_platform_driver(ax_actt_driver);

MODULE_AUTHOR("zhangjuan@axera-tech.com");
MODULE_DESCRIPTION("Inner codec - actt");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:actt");
