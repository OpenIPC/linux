// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2010, Lars-Peter Clausen <lars@metafoo.de>
 *  Copyright (C) 2026 Difan Zhang
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/dmaengine_pcm.h>

#define JZ_REG_AIC_CONF		0x00
#define JZ_REG_AIC_CTRL		0x04
#define JZ_REG_AIC_I2S_FMT	0x10
#define JZ_REG_AIC_FIFO_STATUS	0x14
#define JZ_REG_AIC_I2S_STATUS	0x1c
#define JZ_REG_AIC_CLK_DIV	0x30
#define JZ_REG_AIC_FIFO		0x34
#define T23_REG_AIC_LOOP_FIFO	0x38

#define T23_AIC_CONF_FIFO_RST		GENMASK(14, 13)
#define JZ_AIC_CONF_OVERFLOW_PLAY_LAST	BIT(6)
#define JZ_AIC_CONF_INTERNAL_CODEC	BIT(5)
#define JZ_AIC_CONF_I2S			BIT(4)
#define JZ_AIC_CONF_RESET		BIT(3)
#define JZ_AIC_CONF_BIT_CLK_MASTER	BIT(2)
#define JZ_AIC_CONF_SYNC_CLK_MASTER	BIT(1)
#define JZ_AIC_CONF_ENABLE		BIT(0)

#define JZ_AIC_CTRL_CHANNEL_MASK	GENMASK(26, 24)
/* Copy transmitted samples into the loop FIFO on T23 */
#define T23_AIC_CTRL_LOOP		(BIT(23) | BIT(22))
#define JZ_AIC_CTRL_OUTPUT_SAMPLE_SIZE	GENMASK(21, 19)
#define JZ_AIC_CTRL_INPUT_SAMPLE_SIZE	GENMASK(18, 16)
#define JZ_AIC_CTRL_ENABLE_RX_DMA	BIT(15)
#define JZ_AIC_CTRL_ENABLE_TX_DMA	BIT(14)
#define T23_AIC_CTRL_MONO_MASK		GENMASK(13, 12)
#define T23_AIC_CTRL_MONO_RIGHT		BIT(12)
#define JZ_AIC_CTRL_MONO_TO_STEREO	BIT(11)
#define JZ_AIC_CTRL_SWITCH_ENDIANNESS	BIT(10)
#define JZ_AIC_CTRL_SIGNED_TO_UNSIGNED	BIT(9)
#define JZ_AIC_CTRL_TFLUSH		BIT(8)
#define JZ_AIC_CTRL_RFLUSH		BIT(7)
#define JZ_AIC_CTRL_ENABLE_ROR_INT	BIT(6)
#define JZ_AIC_CTRL_ENABLE_TUR_INT	BIT(5)
#define JZ_AIC_CTRL_ENABLE_RFS_INT	BIT(4)
#define JZ_AIC_CTRL_ENABLE_TFS_INT	BIT(3)
#define JZ_AIC_CTRL_ENABLE_LOOPBACK	BIT(2)
#define JZ_AIC_CTRL_ENABLE_PLAYBACK	BIT(1)
#define JZ_AIC_CTRL_ENABLE_CAPTURE	BIT(0)

#define T23_AIC_I2S_FMT_INTERNAL_CLOCK	BIT(17)
#define JZ_AIC_I2S_FMT_DISABLE_BIT_CLK	BIT(12)
#define JZ_AIC_I2S_FMT_DISABLE_BIT_ICLK	BIT(13)
#define JZ_AIC_I2S_FMT_ENABLE_SYS_CLK	BIT(4)
#define JZ_AIC_I2S_FMT_MSB		BIT(0)

#define JZ_AIC_I2S_STATUS_BUSY		BIT(2)

struct i2s_soc_info {
	struct snd_soc_dai_driver *dai;

	struct reg_field field_rx_fifo_thresh;
	struct reg_field field_tx_fifo_thresh;
	struct reg_field field_i2sdiv_capture;
	struct reg_field field_i2sdiv_playback;

	bool shared_fifo_flush;
	bool is_t23;
};

struct jz4740_i2s {
	struct regmap *regmap;
	phys_addr_t phys_base;

	struct regmap_field *field_rx_fifo_thresh;
	struct regmap_field *field_tx_fifo_thresh;
	struct regmap_field *field_i2sdiv_capture;
	struct regmap_field *field_i2sdiv_playback;

	struct clk *clk_aic;
	struct clk *clk_i2s;
	struct clk *clk_tx;
	struct clk *clk_rx;

	struct snd_dmaengine_dai_dma_data playback_dma_data;
	struct snd_dmaengine_dai_dma_data capture_dma_data;

	const struct i2s_soc_info *soc_info;

	spinlock_t lock;
	unsigned int play_rate;
	unsigned int loop_rate;
	bool tx_clk_on;
	bool rx_clk_on;
	bool capture_running;
};

/* T23 playback reference: the loop FIFO, read through its own DMA channel */
struct t23_aic_loop {
	struct jz4740_i2s *i2s;
	struct device *aic;
	struct snd_dmaengine_dai_dma_data dma_data;
};

static const unsigned int t23_i2s_rates[8] = {
	8000, 12000, 16000, 24000, 32000, 44100, 48000, 96000
};

static const struct snd_pcm_hw_constraint_list t23_i2s_rate_constraints = {
	.count = ARRAY_SIZE(t23_i2s_rates),
	.list = t23_i2s_rates,
};

static int jz4740_i2s_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct jz4740_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	int ret;

	if (i2s->soc_info->is_t23) {
		bool is_play = substream->stream == SNDRV_PCM_STREAM_PLAYBACK;
		struct clk *clk = is_play ? i2s->clk_tx : i2s->clk_rx;
		unsigned int loop_rate = READ_ONCE(i2s->loop_rate);

		ret = snd_pcm_hw_constraint_list(substream->runtime, 0,
						 SNDRV_PCM_HW_PARAM_RATE,
						 &t23_i2s_rate_constraints);
		if (ret < 0)
			return ret;

		if (is_play && loop_rate) {
			ret = snd_pcm_hw_constraint_single(substream->runtime,
							   SNDRV_PCM_HW_PARAM_RATE,
							   loop_rate);
			if (ret < 0)
				return ret;
		}

		if (clk) {
			bool other_on = is_play ? i2s->rx_clk_on :
						  i2s->tx_clk_on;

			if (!(other_on && i2s->clk_tx == i2s->clk_rx) &&
			    !clk_get_rate(clk)) {
				ret = clk_set_rate(clk, 8000 * 256);
				if (ret)
					return ret;
			}
			ret = clk_prepare_enable(clk);
			if (ret)
				return ret;
			if (is_play)
				i2s->tx_clk_on = true;
			else
				i2s->rx_clk_on = true;
		}
	}

	/*
	 * When we can flush FIFOs independently, only flush the FIFO
	 * that is starting up. We can do this when the DAI is active
	 * because it does not disturb other active substreams.
	 */
	if (!i2s->soc_info->shared_fifo_flush) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_set_bits(i2s->regmap, JZ_REG_AIC_CTRL, JZ_AIC_CTRL_TFLUSH);
		else
			regmap_set_bits(i2s->regmap, JZ_REG_AIC_CTRL, JZ_AIC_CTRL_RFLUSH);
	}

	if (snd_soc_dai_active(dai))
		return 0;

	/*
	 * When there is a shared flush bit for both FIFOs, the TFLUSH
	 * bit flushes both FIFOs. Flushing while the DAI is active would
	 * cause FIFO underruns in other active substreams so we have to
	 * guard this behind the snd_soc_dai_active() check.
	 */
	if (i2s->soc_info->shared_fifo_flush)
		regmap_set_bits(i2s->regmap, JZ_REG_AIC_CTRL, JZ_AIC_CTRL_TFLUSH);

	if (!i2s->soc_info->is_t23) {
		ret = clk_prepare_enable(i2s->clk_i2s);
		if (ret)
			return ret;
	}

	regmap_set_bits(i2s->regmap, JZ_REG_AIC_CONF, JZ_AIC_CONF_ENABLE);
	return 0;
}

static void jz4740_i2s_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct jz4740_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	unsigned long flags;

	if (i2s->soc_info->is_t23) {
		bool is_play = substream->stream == SNDRV_PCM_STREAM_PLAYBACK;
		struct clk *clk = is_play ? i2s->clk_tx : i2s->clk_rx;

		if (clk) {
			if (is_play && i2s->tx_clk_on) {
				clk_disable_unprepare(clk);
				i2s->tx_clk_on = false;
			} else if (!is_play && i2s->rx_clk_on) {
				clk_disable_unprepare(clk);
				i2s->rx_clk_on = false;
			}
		}

		if (is_play) {
			spin_lock_irqsave(&i2s->lock, flags);
			i2s->play_rate = 0;
			spin_unlock_irqrestore(&i2s->lock, flags);
		}
	}

	if (snd_soc_dai_active(dai))
		return;

	regmap_clear_bits(i2s->regmap, JZ_REG_AIC_CONF, JZ_AIC_CONF_ENABLE);

	if (!i2s->soc_info->is_t23)
		clk_disable_unprepare(i2s->clk_i2s);
}

static int jz4740_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
	struct snd_soc_dai *dai)
{
	struct jz4740_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	unsigned long flags;
	uint32_t mask, val;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		mask = JZ_AIC_CTRL_ENABLE_PLAYBACK | JZ_AIC_CTRL_ENABLE_TX_DMA;
	else
		mask = JZ_AIC_CTRL_ENABLE_CAPTURE | JZ_AIC_CTRL_ENABLE_RX_DMA;

	if (!i2s->soc_info->is_t23) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			regmap_set_bits(i2s->regmap, JZ_REG_AIC_CTRL, mask);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			regmap_clear_bits(i2s->regmap, JZ_REG_AIC_CTRL, mask);
			break;
		default:
			return -EINVAL;
		}

		return 0;
	}

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		val = mask;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		val = 0;
		break;
	default:
		return -EINVAL;
	}

	spin_lock_irqsave(&i2s->lock, flags);
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		i2s->capture_running = !!val;
		if (!val)
			mask |= T23_AIC_CTRL_LOOP;
	}
	regmap_update_bits(i2s->regmap, JZ_REG_AIC_CTRL,
			   mask | JZ_AIC_CTRL_TFLUSH | JZ_AIC_CTRL_RFLUSH, val);
	spin_unlock_irqrestore(&i2s->lock, flags);

	return 0;
}

static int jz4740_i2s_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct jz4740_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	const unsigned int conf_mask = JZ_AIC_CONF_BIT_CLK_MASTER |
				       JZ_AIC_CONF_SYNC_CLK_MASTER;
	unsigned int conf = 0, format = 0;

	if (i2s->soc_info->is_t23)
		format |= T23_AIC_I2S_FMT_INTERNAL_CLOCK;

	switch (fmt & SND_SOC_DAIFMT_CLOCK_PROVIDER_MASK) {
	case SND_SOC_DAIFMT_BP_FP:
		conf |= JZ_AIC_CONF_BIT_CLK_MASTER | JZ_AIC_CONF_SYNC_CLK_MASTER;
		format |= JZ_AIC_I2S_FMT_ENABLE_SYS_CLK;
		break;
	case SND_SOC_DAIFMT_BC_FP:
		conf |= JZ_AIC_CONF_SYNC_CLK_MASTER;
		break;
	case SND_SOC_DAIFMT_BP_FC:
		conf |= JZ_AIC_CONF_BIT_CLK_MASTER;
		break;
	case SND_SOC_DAIFMT_BC_FC:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_MSB:
		format |= JZ_AIC_I2S_FMT_MSB;
		break;
	case SND_SOC_DAIFMT_I2S:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	default:
		return -EINVAL;
	}

	regmap_update_bits(i2s->regmap, JZ_REG_AIC_CONF, conf_mask, conf);
	regmap_write(i2s->regmap, JZ_REG_AIC_I2S_FMT, format);

	return 0;
}

static int jz4740_i2s_get_i2sdiv(unsigned long mclk, unsigned long rate,
				 unsigned long i2sdiv_max)
{
	unsigned long div, rate1, rate2, err1, err2;

	div = mclk / (64 * rate);
	if (div == 0)
		div = 1;

	rate1 = mclk / (64 * div);
	rate2 = mclk / (64 * (div + 1));

	err1 = abs(rate1 - rate);
	err2 = abs(rate2 - rate);

	/*
	 * Choose the divider that produces the smallest error in the
	 * output rate and reject dividers with a 5% or higher error.
	 * In the event that both dividers are outside the acceptable
	 * error margin, reject the rate to prevent distorted audio.
	 * (The number 5% is arbitrary.)
	 */
	if (div <= i2sdiv_max && err1 <= err2 && err1 < rate/20)
		return div;
	if (div < i2sdiv_max && err2 < rate/20)
		return div + 1;

	return -EINVAL;
}

static int jz4740_i2s_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct jz4740_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	struct regmap_field *div_field;
	struct clk *clk;
	unsigned long i2sdiv_max, flags;
	unsigned int sample_size;
	uint32_t ctrl, conf, mask;
	int div = 1, ret;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		sample_size = 0;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		sample_size = 1;
		break;
	case SNDRV_PCM_FORMAT_S20_LE:
		sample_size = 3;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		sample_size = 4;
		break;
	default:
		return -EINVAL;
	}

	if (!i2s->soc_info->is_t23) {
		regmap_read(i2s->regmap, JZ_REG_AIC_CTRL, &ctrl);
		regmap_read(i2s->regmap, JZ_REG_AIC_CONF, &conf);

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			ctrl &= ~JZ_AIC_CTRL_OUTPUT_SAMPLE_SIZE;
			ctrl |= FIELD_PREP(JZ_AIC_CTRL_OUTPUT_SAMPLE_SIZE,
					   sample_size);

			if (params_channels(params) == 1)
				ctrl |= JZ_AIC_CTRL_MONO_TO_STEREO;
			else
				ctrl &= ~JZ_AIC_CTRL_MONO_TO_STEREO;

			div_field = i2s->field_i2sdiv_playback;
			i2sdiv_max = GENMASK(i2s->soc_info->field_i2sdiv_playback.msb,
					     i2s->soc_info->field_i2sdiv_playback.lsb);
		} else {
			ctrl &= ~JZ_AIC_CTRL_INPUT_SAMPLE_SIZE;
			ctrl |= FIELD_PREP(JZ_AIC_CTRL_INPUT_SAMPLE_SIZE,
					   sample_size);

			div_field = i2s->field_i2sdiv_capture;
			i2sdiv_max = GENMASK(i2s->soc_info->field_i2sdiv_capture.msb,
					     i2s->soc_info->field_i2sdiv_capture.lsb);
		}

		if (conf & (JZ_AIC_CONF_BIT_CLK_MASTER |
			    JZ_AIC_CONF_SYNC_CLK_MASTER)) {
			div = jz4740_i2s_get_i2sdiv(clk_get_rate(i2s->clk_i2s),
						    params_rate(params),
						    i2sdiv_max);
			if (div < 0)
				return div;
		}

		regmap_write(i2s->regmap, JZ_REG_AIC_CTRL, ctrl);
		regmap_field_write(div_field, div - 1);

		return 0;
	}

	clk = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ?
	      i2s->clk_tx : i2s->clk_rx;
	if (clk) {
		ret = clk_set_rate(clk, params_rate(params) * 256);
		if (ret)
			return ret;
	}

	regmap_read(i2s->regmap, JZ_REG_AIC_CONF, &conf);

	ctrl = 0;
	mask = JZ_AIC_CTRL_TFLUSH | JZ_AIC_CTRL_RFLUSH;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		mask |= JZ_AIC_CTRL_OUTPUT_SAMPLE_SIZE |
			JZ_AIC_CTRL_MONO_TO_STEREO |
			JZ_AIC_CTRL_CHANNEL_MASK;
		ctrl |= FIELD_PREP(JZ_AIC_CTRL_OUTPUT_SAMPLE_SIZE, sample_size) |
			FIELD_PREP(JZ_AIC_CTRL_CHANNEL_MASK,
				   params_channels(params) - 1);
		if (params_channels(params) == 1)
			ctrl |= JZ_AIC_CTRL_MONO_TO_STEREO;

		div_field = i2s->field_i2sdiv_playback;
		i2sdiv_max = GENMASK(i2s->soc_info->field_i2sdiv_playback.msb,
				     i2s->soc_info->field_i2sdiv_playback.lsb);
	} else {
		mask |= JZ_AIC_CTRL_INPUT_SAMPLE_SIZE | T23_AIC_CTRL_MONO_MASK;
		ctrl |= FIELD_PREP(JZ_AIC_CTRL_INPUT_SAMPLE_SIZE, sample_size);
		if (params_channels(params) == 1)
			ctrl |= T23_AIC_CTRL_MONO_RIGHT;

		div_field = i2s->field_i2sdiv_capture;
		i2sdiv_max = GENMASK(i2s->soc_info->field_i2sdiv_capture.msb,
				     i2s->soc_info->field_i2sdiv_capture.lsb);
	}

	if ((conf & (JZ_AIC_CONF_BIT_CLK_MASTER | JZ_AIC_CONF_SYNC_CLK_MASTER)) &&
	    clk) {
		div = jz4740_i2s_get_i2sdiv(clk_get_rate(clk),
					    params_rate(params), i2sdiv_max);
		if (div < 0)
			return div;
	}

	spin_lock_irqsave(&i2s->lock, flags);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (i2s->loop_rate && i2s->loop_rate != params_rate(params)) {
			spin_unlock_irqrestore(&i2s->lock, flags);
			return -EBUSY;
		}
		i2s->play_rate = params_rate(params);
	}
	regmap_update_bits(i2s->regmap, JZ_REG_AIC_CTRL, mask, ctrl);
	regmap_field_write(div_field, div - 1);
	spin_unlock_irqrestore(&i2s->lock, flags);

	return 0;
}

static int jz4740_i2s_dai_probe(struct snd_soc_dai *dai)
{
	struct jz4740_i2s *i2s = snd_soc_dai_get_drvdata(dai);

	snd_soc_dai_init_dma_data(dai, &i2s->playback_dma_data,
		&i2s->capture_dma_data);

	return 0;
}

static const struct snd_soc_dai_ops jz4740_i2s_dai_ops = {
	.probe = jz4740_i2s_dai_probe,
	.startup = jz4740_i2s_startup,
	.shutdown = jz4740_i2s_shutdown,
	.trigger = jz4740_i2s_trigger,
	.hw_params = jz4740_i2s_hw_params,
	.set_fmt = jz4740_i2s_set_fmt,
};

#define JZ4740_I2S_FMTS (SNDRV_PCM_FMTBIT_S8 | \
			 SNDRV_PCM_FMTBIT_S16_LE | \
			 SNDRV_PCM_FMTBIT_S20_LE | \
			 SNDRV_PCM_FMTBIT_S24_LE)

static struct snd_soc_dai_driver jz4740_i2s_dai = {
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
		.formats = JZ4740_I2S_FMTS,
	},
	.capture = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
		.formats = JZ4740_I2S_FMTS,
	},
	.symmetric_rate = 1,
	.ops = &jz4740_i2s_dai_ops,
};

static const struct i2s_soc_info jz4740_i2s_soc_info = {
	.dai			= &jz4740_i2s_dai,
	.field_rx_fifo_thresh	= REG_FIELD(JZ_REG_AIC_CONF, 12, 15),
	.field_tx_fifo_thresh	= REG_FIELD(JZ_REG_AIC_CONF, 8, 11),
	.field_i2sdiv_capture	= REG_FIELD(JZ_REG_AIC_CLK_DIV, 0, 3),
	.field_i2sdiv_playback	= REG_FIELD(JZ_REG_AIC_CLK_DIV, 0, 3),
	.shared_fifo_flush	= true,
};

static const struct i2s_soc_info jz4760_i2s_soc_info = {
	.dai			= &jz4740_i2s_dai,
	.field_rx_fifo_thresh	= REG_FIELD(JZ_REG_AIC_CONF, 24, 27),
	.field_tx_fifo_thresh	= REG_FIELD(JZ_REG_AIC_CONF, 16, 20),
	.field_i2sdiv_capture	= REG_FIELD(JZ_REG_AIC_CLK_DIV, 0, 3),
	.field_i2sdiv_playback	= REG_FIELD(JZ_REG_AIC_CLK_DIV, 0, 3),
};

static const struct i2s_soc_info x1000_i2s_soc_info = {
	.dai = &jz4740_i2s_dai,
	.field_rx_fifo_thresh	= REG_FIELD(JZ_REG_AIC_CONF, 24, 27),
	.field_tx_fifo_thresh	= REG_FIELD(JZ_REG_AIC_CONF, 16, 20),
	.field_i2sdiv_capture	= REG_FIELD(JZ_REG_AIC_CLK_DIV, 0, 8),
	.field_i2sdiv_playback	= REG_FIELD(JZ_REG_AIC_CLK_DIV, 0, 8),
};

static struct snd_soc_dai_driver jz4770_i2s_dai = {
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
		.formats = JZ4740_I2S_FMTS,
	},
	.capture = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
		.formats = JZ4740_I2S_FMTS,
	},
	.ops = &jz4740_i2s_dai_ops,
};

static const struct i2s_soc_info jz4770_i2s_soc_info = {
	.dai			= &jz4770_i2s_dai,
	.field_rx_fifo_thresh	= REG_FIELD(JZ_REG_AIC_CONF, 24, 27),
	.field_tx_fifo_thresh	= REG_FIELD(JZ_REG_AIC_CONF, 16, 20),
	.field_i2sdiv_capture	= REG_FIELD(JZ_REG_AIC_CLK_DIV, 8, 11),
	.field_i2sdiv_playback	= REG_FIELD(JZ_REG_AIC_CLK_DIV, 0, 3),
};

static const struct i2s_soc_info jz4780_i2s_soc_info = {
	.dai			= &jz4770_i2s_dai,
	.field_rx_fifo_thresh	= REG_FIELD(JZ_REG_AIC_CONF, 24, 27),
	.field_tx_fifo_thresh	= REG_FIELD(JZ_REG_AIC_CONF, 16, 20),
	.field_i2sdiv_capture	= REG_FIELD(JZ_REG_AIC_CLK_DIV, 8, 11),
	.field_i2sdiv_playback	= REG_FIELD(JZ_REG_AIC_CLK_DIV, 0, 3),
};

static struct snd_soc_dai_driver t23_i2s_dai = {
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_96000 | SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_96000 | SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.symmetric_rate = 1,
	.ops = &jz4740_i2s_dai_ops,
};

static const struct i2s_soc_info t23_i2s_soc_info = {
	.dai			= &t23_i2s_dai,
	.field_rx_fifo_thresh	= REG_FIELD(JZ_REG_AIC_CONF, 24, 27),
	.field_tx_fifo_thresh	= REG_FIELD(JZ_REG_AIC_CONF, 16, 20),
	.field_i2sdiv_capture	= REG_FIELD(JZ_REG_AIC_CLK_DIV, 8, 11),
	.field_i2sdiv_playback	= REG_FIELD(JZ_REG_AIC_CLK_DIV, 0, 3),
	.is_t23			= true,
};

static int t23_aic_loop_dai_probe(struct snd_soc_dai *dai)
{
	struct t23_aic_loop *loop = snd_soc_dai_get_drvdata(dai);

	snd_soc_dai_init_dma_data(dai, NULL, &loop->dma_data);
	return 0;
}

static int t23_aic_loop_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct t23_aic_loop *loop = snd_soc_dai_get_drvdata(dai);
	unsigned int rate = READ_ONCE(loop->i2s->play_rate);
	int ret;

	ret = snd_pcm_hw_constraint_list(substream->runtime, 0,
					 SNDRV_PCM_HW_PARAM_RATE,
					 &t23_i2s_rate_constraints);
	if (ret < 0 || !rate)
		return ret < 0 ? ret : 0;

	ret = snd_pcm_hw_constraint_single(substream->runtime,
					   SNDRV_PCM_HW_PARAM_RATE, rate);
	return ret < 0 ? ret : 0;
}

static int t23_aic_loop_hw_params(struct snd_pcm_substream *substream,
				  struct snd_pcm_hw_params *params,
				  struct snd_soc_dai *dai)
{
	struct t23_aic_loop *loop = snd_soc_dai_get_drvdata(dai);
	struct jz4740_i2s *i2s = loop->i2s;
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&i2s->lock, flags);
	if (i2s->play_rate && i2s->play_rate != params_rate(params))
		ret = -EBUSY;
	else
		i2s->loop_rate = params_rate(params);
	spin_unlock_irqrestore(&i2s->lock, flags);
	return ret;
}

static int t23_aic_loop_set(struct jz4740_i2s *i2s, bool on)
{
	unsigned long flags;

	spin_lock_irqsave(&i2s->lock, flags);
	if (on && !i2s->capture_running) {
		spin_unlock_irqrestore(&i2s->lock, flags);
		return -EBUSY;
	}
	regmap_update_bits(i2s->regmap, JZ_REG_AIC_CTRL,
			   JZ_AIC_CTRL_TFLUSH | JZ_AIC_CTRL_RFLUSH |
			   T23_AIC_CTRL_LOOP,
			   on ? T23_AIC_CTRL_LOOP : 0);
	spin_unlock_irqrestore(&i2s->lock, flags);
	return 0;
}

static int t23_aic_loop_trigger(struct snd_pcm_substream *substream, int cmd,
				struct snd_soc_dai *dai)
{
	struct t23_aic_loop *loop = snd_soc_dai_get_drvdata(dai);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		return t23_aic_loop_set(loop->i2s, true);
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		return t23_aic_loop_set(loop->i2s, false);
	default:
		return -EINVAL;
	}
}

static void t23_aic_loop_shutdown(struct snd_pcm_substream *substream,
				  struct snd_soc_dai *dai)
{
	struct t23_aic_loop *loop = snd_soc_dai_get_drvdata(dai);
	unsigned long flags;

	t23_aic_loop_set(loop->i2s, false);
	spin_lock_irqsave(&loop->i2s->lock, flags);
	loop->i2s->loop_rate = 0;
	spin_unlock_irqrestore(&loop->i2s->lock, flags);
}

static const struct snd_soc_dai_ops t23_aic_loop_dai_ops = {
	.probe = t23_aic_loop_dai_probe,
	.startup = t23_aic_loop_startup,
	.shutdown = t23_aic_loop_shutdown,
	.trigger = t23_aic_loop_trigger,
	.hw_params = t23_aic_loop_hw_params,
};

static struct snd_soc_dai_driver t23_aic_loop_dai = {
	.name = "t23-aic-loop-dai",
	.capture = {
		.stream_name = "Loop Capture",
		.channels_min = 1,
		.channels_max = 1,
		.rates = SNDRV_PCM_RATE_8000_96000 | SNDRV_PCM_RATE_KNOT,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
	.ops = &t23_aic_loop_dai_ops,
};

static const struct snd_soc_component_driver t23_aic_loop_component = {
	.name		= "t23-aic-loop",
};

static int t23_aic_loop_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct platform_device *aic;
	struct device_node *np;
	struct t23_aic_loop *loop;
	int ret;

	loop = devm_kzalloc(dev, sizeof(*loop), GFP_KERNEL);
	if (!loop)
		return -ENOMEM;

	np = of_parse_phandle(dev->of_node, "ingenic,aic", 0);
	if (!np)
		return -EINVAL;
	aic = of_find_device_by_node(np);
	of_node_put(np);
	if (!aic)
		return -EPROBE_DEFER;

	loop->aic = &aic->dev;
	loop->i2s = platform_get_drvdata(aic);
	if (!loop->i2s) {
		put_device(loop->aic);
		return -EPROBE_DEFER;
	}

	if (!device_link_add(dev, loop->aic, DL_FLAG_AUTOREMOVE_CONSUMER)) {
		put_device(loop->aic);
		return -EINVAL;
	}

	loop->dma_data.addr = loop->i2s->phys_base + T23_REG_AIC_LOOP_FIFO;
	loop->dma_data.maxburst = 8;
	platform_set_drvdata(pdev, loop);

	ret = devm_snd_soc_register_component(dev, &t23_aic_loop_component,
					      &t23_aic_loop_dai, 1);
	if (!ret)
		ret = devm_snd_dmaengine_pcm_register(dev, NULL, 0);
	if (ret)
		put_device(loop->aic);
	return ret;
}

static void t23_aic_loop_remove(struct platform_device *pdev)
{
	struct t23_aic_loop *loop = platform_get_drvdata(pdev);

	put_device(loop->aic);
}

static const struct of_device_id t23_aic_loop_of_matches[] = {
	{ .compatible = "ingenic,t23-aic-loop" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, t23_aic_loop_of_matches);

static struct platform_driver t23_aic_loop_driver = {
	.probe = t23_aic_loop_probe,
	.remove = t23_aic_loop_remove,
	.driver = {
		.name = "t23-aic-loop",
		.of_match_table = t23_aic_loop_of_matches,
	},
};

static int jz4740_i2s_suspend(struct snd_soc_component *component)
{
	struct jz4740_i2s *i2s = snd_soc_component_get_drvdata(component);

	if (snd_soc_component_active(component)) {
		regmap_clear_bits(i2s->regmap, JZ_REG_AIC_CONF, JZ_AIC_CONF_ENABLE);
		if (i2s->soc_info->is_t23) {
			if (i2s->tx_clk_on && i2s->clk_tx)
				clk_disable_unprepare(i2s->clk_tx);
			if (i2s->rx_clk_on && i2s->clk_rx)
				clk_disable_unprepare(i2s->clk_rx);
		} else {
			clk_disable_unprepare(i2s->clk_i2s);
		}
	}

	clk_disable_unprepare(i2s->clk_aic);

	return 0;
}

static int jz4740_i2s_resume(struct snd_soc_component *component)
{
	struct jz4740_i2s *i2s = snd_soc_component_get_drvdata(component);
	int ret;

	ret = clk_prepare_enable(i2s->clk_aic);
	if (ret) {
		if (i2s->soc_info->is_t23) {
			i2s->tx_clk_on = false;
			i2s->rx_clk_on = false;
		}
		return ret;
	}

	if (snd_soc_component_active(component)) {
		if (i2s->soc_info->is_t23) {
			if (i2s->tx_clk_on && i2s->clk_tx) {
				ret = clk_prepare_enable(i2s->clk_tx);
				if (ret) {
					i2s->tx_clk_on = false;
					i2s->rx_clk_on = false;
					goto err_disable_aic;
				}
			}
			if (i2s->rx_clk_on && i2s->clk_rx) {
				ret = clk_prepare_enable(i2s->clk_rx);
				if (ret) {
					if (i2s->tx_clk_on && i2s->clk_tx)
						clk_disable_unprepare(i2s->clk_tx);
					i2s->tx_clk_on = false;
					i2s->rx_clk_on = false;
					goto err_disable_aic;
				}
			}
		} else {
			ret = clk_prepare_enable(i2s->clk_i2s);
			if (ret)
				goto err_disable_aic;
		}

		regmap_set_bits(i2s->regmap, JZ_REG_AIC_CONF, JZ_AIC_CONF_ENABLE);
	}

	return 0;

err_disable_aic:
	clk_disable_unprepare(i2s->clk_aic);
	return ret;
}

static int jz4740_i2s_probe(struct snd_soc_component *component)
{
	struct jz4740_i2s *i2s = snd_soc_component_get_drvdata(component);
	int ret;

	ret = clk_prepare_enable(i2s->clk_aic);
	if (ret)
		return ret;

	if (i2s->soc_info->is_t23) {
		u32 val;

		if (i2s->clk_tx) {
			ret = clk_set_rate(i2s->clk_tx, 8000 * 256);
			if (ret)
				goto err_disable_aic;
			ret = clk_prepare_enable(i2s->clk_tx);
			if (ret)
				goto err_disable_aic;
		}
		if (i2s->clk_rx) {
			ret = clk_set_rate(i2s->clk_rx, 8000 * 256);
			if (ret)
				goto err_disable_tx;
			ret = clk_prepare_enable(i2s->clk_rx);
			if (ret)
				goto err_disable_tx;
		}

		regmap_write(i2s->regmap, JZ_REG_AIC_CONF,
			     JZ_AIC_CONF_RESET |
			     JZ_AIC_CONF_BIT_CLK_MASTER |
			     JZ_AIC_CONF_SYNC_CLK_MASTER);
		ret = regmap_read_poll_timeout(i2s->regmap, JZ_REG_AIC_CONF,
					       val, !(val & T23_AIC_CONF_FIFO_RST),
					       1000, 100000);
		if (i2s->clk_rx)
			clk_disable_unprepare(i2s->clk_rx);
		if (i2s->clk_tx)
			clk_disable_unprepare(i2s->clk_tx);
		if (ret)
			goto err_disable_aic;

		regmap_write(i2s->regmap, JZ_REG_AIC_CONF,
			     JZ_AIC_CONF_OVERFLOW_PLAY_LAST |
			     JZ_AIC_CONF_I2S | JZ_AIC_CONF_INTERNAL_CODEC);
		regmap_field_write(i2s->field_rx_fifo_thresh, 8);
		regmap_field_write(i2s->field_tx_fifo_thresh, 16);
		regmap_write(i2s->regmap, JZ_REG_AIC_CTRL, 0);
		regmap_write(i2s->regmap, JZ_REG_AIC_I2S_FMT,
			     T23_AIC_I2S_FMT_INTERNAL_CLOCK);
		return 0;

err_disable_tx:
		if (i2s->clk_tx)
			clk_disable_unprepare(i2s->clk_tx);
err_disable_aic:
		clk_disable_unprepare(i2s->clk_aic);
		return ret;
	}

	regmap_write(i2s->regmap, JZ_REG_AIC_CONF, JZ_AIC_CONF_RESET);

	regmap_write(i2s->regmap, JZ_REG_AIC_CONF,
		     JZ_AIC_CONF_OVERFLOW_PLAY_LAST |
		     JZ_AIC_CONF_I2S | JZ_AIC_CONF_INTERNAL_CODEC);

	regmap_field_write(i2s->field_rx_fifo_thresh, 7);
	regmap_field_write(i2s->field_tx_fifo_thresh, 8);

	return 0;
}

static void jz4740_i2s_remove(struct snd_soc_component *component)
{
	struct jz4740_i2s *i2s = snd_soc_component_get_drvdata(component);

	clk_disable_unprepare(i2s->clk_aic);
}

static const struct snd_soc_component_driver jz4740_i2s_component = {
	.name			= "jz4740-i2s",
	.probe			= jz4740_i2s_probe,
	.remove			= jz4740_i2s_remove,
	.suspend		= jz4740_i2s_suspend,
	.resume			= jz4740_i2s_resume,
	.legacy_dai_naming	= 1,
};

static const struct of_device_id jz4740_of_matches[] = {
	{ .compatible = "ingenic,jz4740-i2s", .data = &jz4740_i2s_soc_info },
	{ .compatible = "ingenic,jz4760-i2s", .data = &jz4760_i2s_soc_info },
	{ .compatible = "ingenic,jz4770-i2s", .data = &jz4770_i2s_soc_info },
	{ .compatible = "ingenic,jz4780-i2s", .data = &jz4780_i2s_soc_info },
	{ .compatible = "ingenic,x1000-i2s", .data = &x1000_i2s_soc_info },
	{ .compatible = "ingenic,t23-aic", .data = &t23_i2s_soc_info },
	{ .compatible = "ingenic,t23-i2s", .data = &t23_i2s_soc_info },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, jz4740_of_matches);

static int jz4740_i2s_init_regmap_fields(struct device *dev,
					 struct jz4740_i2s *i2s)
{
	i2s->field_rx_fifo_thresh =
		devm_regmap_field_alloc(dev, i2s->regmap,
					i2s->soc_info->field_rx_fifo_thresh);
	if (IS_ERR(i2s->field_rx_fifo_thresh))
		return PTR_ERR(i2s->field_rx_fifo_thresh);

	i2s->field_tx_fifo_thresh =
		devm_regmap_field_alloc(dev, i2s->regmap,
					i2s->soc_info->field_tx_fifo_thresh);
	if (IS_ERR(i2s->field_tx_fifo_thresh))
		return PTR_ERR(i2s->field_tx_fifo_thresh);

	i2s->field_i2sdiv_capture =
		devm_regmap_field_alloc(dev, i2s->regmap,
					i2s->soc_info->field_i2sdiv_capture);
	if (IS_ERR(i2s->field_i2sdiv_capture))
		return PTR_ERR(i2s->field_i2sdiv_capture);

	i2s->field_i2sdiv_playback =
		devm_regmap_field_alloc(dev, i2s->regmap,
					i2s->soc_info->field_i2sdiv_playback);
	if (IS_ERR(i2s->field_i2sdiv_playback))
		return PTR_ERR(i2s->field_i2sdiv_playback);

	return 0;
}

static const struct regmap_config jz4740_i2s_regmap_config = {
	.reg_bits	= 32,
	.reg_stride	= 4,
	.val_bits	= 32,
	.max_register	= JZ_REG_AIC_FIFO,
};

static const struct regmap_config t23_i2s_regmap_config = {
	.reg_bits	= 32,
	.reg_stride	= 4,
	.val_bits	= 32,
	.max_register	= T23_REG_AIC_LOOP_FIFO,
};

static int jz4740_i2s_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct jz4740_i2s *i2s;
	struct resource *mem;
	void __iomem *regs;
	int ret;

	i2s = devm_kzalloc(dev, sizeof(*i2s), GFP_KERNEL);
	if (!i2s)
		return -ENOMEM;

	spin_lock_init(&i2s->lock);
	i2s->soc_info = device_get_match_data(dev);

	regs = devm_platform_get_and_ioremap_resource(pdev, 0, &mem);
	if (IS_ERR(regs))
		return PTR_ERR(regs);

	i2s->phys_base = mem->start;
	i2s->playback_dma_data.maxburst = i2s->soc_info->is_t23 ? 32 : 16;
	i2s->playback_dma_data.addr = mem->start + JZ_REG_AIC_FIFO;

	i2s->capture_dma_data.maxburst = 16;
	i2s->capture_dma_data.addr = mem->start + JZ_REG_AIC_FIFO;

	i2s->clk_aic = devm_clk_get(dev, "aic");
	if (IS_ERR(i2s->clk_aic))
		return PTR_ERR(i2s->clk_aic);

	if (i2s->soc_info->is_t23) {
		i2s->clk_tx = devm_clk_get_optional(dev, "cgu_i2s_spk");
		if (IS_ERR(i2s->clk_tx))
			return PTR_ERR(i2s->clk_tx);

		i2s->clk_rx = devm_clk_get_optional(dev, "cgu_i2s_mic");
		if (IS_ERR(i2s->clk_rx))
			return PTR_ERR(i2s->clk_rx);

		if (!i2s->clk_tx) {
			i2s->clk_tx = devm_clk_get(dev, "i2s");
			if (IS_ERR(i2s->clk_tx))
				return PTR_ERR(i2s->clk_tx);
			if (!i2s->clk_rx)
				i2s->clk_rx = i2s->clk_tx;
		}
		i2s->clk_i2s = i2s->clk_tx;
	} else {
		i2s->clk_i2s = devm_clk_get(dev, "i2s");
		if (IS_ERR(i2s->clk_i2s))
			return PTR_ERR(i2s->clk_i2s);
	}

	i2s->regmap = devm_regmap_init_mmio(&pdev->dev, regs,
					    i2s->soc_info->is_t23 ?
					    &t23_i2s_regmap_config :
					    &jz4740_i2s_regmap_config);
	if (IS_ERR(i2s->regmap))
		return PTR_ERR(i2s->regmap);

	ret = jz4740_i2s_init_regmap_fields(dev, i2s);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, i2s);

	ret = devm_snd_soc_register_component(dev, &jz4740_i2s_component,
					      i2s->soc_info->dai, 1);
	if (ret)
		return ret;

	return devm_snd_dmaengine_pcm_register(dev, NULL,
		SND_DMAENGINE_PCM_FLAG_COMPAT);
}

static void jz4740_i2s_dev_remove(struct platform_device *pdev)
{
	struct jz4740_i2s *i2s = platform_get_drvdata(pdev);

	if (i2s && i2s->soc_info->is_t23 && i2s->regmap) {
		regmap_write(i2s->regmap, JZ_REG_AIC_CTRL, 0);
		regmap_write(i2s->regmap, JZ_REG_AIC_CONF, 0);
	}
}

static struct platform_driver jz4740_i2s_driver = {
	.probe = jz4740_i2s_dev_probe,
	.remove = jz4740_i2s_dev_remove,
	.driver = {
		.name = "jz4740-i2s",
		.of_match_table = jz4740_of_matches,
	},
};

static struct platform_driver * const jz4740_i2s_drivers[] = {
	&jz4740_i2s_driver,
	&t23_aic_loop_driver,
};

static int __init jz4740_i2s_init(void)
{
	return platform_register_drivers(jz4740_i2s_drivers,
					 ARRAY_SIZE(jz4740_i2s_drivers));
}
module_init(jz4740_i2s_init);

static void __exit jz4740_i2s_exit(void)
{
	platform_unregister_drivers(jz4740_i2s_drivers,
				    ARRAY_SIZE(jz4740_i2s_drivers));
}
module_exit(jz4740_i2s_exit);

MODULE_AUTHOR("Lars-Peter Clausen, <lars@metafoo.de>");
MODULE_DESCRIPTION("Ingenic JZ4740 SoC I2S driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:jz4740-i2s");
