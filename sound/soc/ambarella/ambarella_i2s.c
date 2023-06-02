/*
 * sound/soc/ambarella_i2s.c
 *
 * History:
 *	2008/03/03 - [Eric Lee] created file
 *	2008/04/16 - [Eric Lee] Removed the compiling warning
 *	2009/01/22 - [Anthony Ginger] Port to 2.6.28
 *	2009/03/05 - [Cao Rongrong] Update from 2.6.22.10
 *	2009/06/10 - [Cao Rongrong] Port to 2.6.29
 *	2009/06/29 - [Cao Rongrong] Support more mclk and fs
 *	2010/10/25 - [Cao Rongrong] Port to 2.6.36+
 *	2011/03/20 - [Cao Rongrong] Port to 2.6.38
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>
#include <plat/dma.h>
#include <plat/audio.h>
#include "ambarella_pcm.h"
#include "ambarella_i2s.h"

static unsigned int capture_enabled = 1;
module_param(capture_enabled, uint, 0644);
MODULE_PARM_DESC(capture_enabled, "capture_enabled.");

static unsigned int bclk_reverse = 0;
module_param(bclk_reverse, uint, 0644);
MODULE_PARM_DESC(bclk_reverse, "bclk_reverse.");

static DEFINE_MUTEX(clock_reg_mutex);

static inline void dai_tx_enable(void)
{
	amba_setbitsl(I2S_INIT_REG, I2S_TX_ENABLE_BIT);
}

static inline void dai_rx_enable(void)
{
	amba_setbitsl(I2S_INIT_REG, I2S_RX_ENABLE_BIT);
}

static inline void dai_tx_disable(void)
{
	amba_clrbitsl(I2S_INIT_REG, I2S_TX_ENABLE_BIT);
}

static inline void dai_rx_disable(void)
{
	if(capture_enabled)
		amba_clrbitsl(I2S_INIT_REG, I2S_RX_ENABLE_BIT);
}

static inline void dai_tx_fifo_rst(void)
{
	amba_setbitsl(I2S_INIT_REG, I2S_TX_FIFO_RESET_BIT);
}

static inline void dai_rx_fifo_rst(void)
{
	if(capture_enabled)
		amba_setbitsl(I2S_INIT_REG, I2S_RX_FIFO_RESET_BIT);
}

static inline void dai_fifo_rst(void)
{
	if(capture_enabled)
		amba_setbitsl(I2S_INIT_REG, I2S_FIFO_RESET_BIT);
}

static int ambarella_i2s_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	if(substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		dai_rx_disable();
		dai_rx_fifo_rst();
	} else {
		dai_tx_disable();
		dai_tx_fifo_rst();
	}
	return 0;
}

static int ambarella_i2s_startup(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;

	/* Add a rule to enforce the DMA buffer align. */
	ret = snd_pcm_hw_constraint_step(runtime, 0,
		SNDRV_PCM_HW_PARAM_PERIOD_BYTES, 32);
	if (ret)
		goto ambarella_i2s_startup_exit;

	ret = snd_pcm_hw_constraint_step(runtime, 0,
		SNDRV_PCM_HW_PARAM_BUFFER_BYTES, 32);
	if (ret)
		goto ambarella_i2s_startup_exit;

	ret = snd_pcm_hw_constraint_integer(runtime,
		SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		goto ambarella_i2s_startup_exit;

	return 0;
ambarella_i2s_startup_exit:
	return ret;
}

static int ambarella_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *cpu_dai)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);
	struct ambarella_i2s_interface *i2s_intf = &priv_data->amb_i2s_intf;
	u32 clock_reg;

	if( capture_enabled == 0)
		return 0;

	/* Disable tx/rx before initializing */
	dai_tx_disable();
	dai_rx_disable();

	i2s_intf->sfreq = params_rate(params);
	i2s_intf->channels = params_channels(params);
	amba_writel(I2S_REG(I2S_CHANNEL_SELECT_OFFSET), i2s_intf->channels / 2 - 1);

	/* Set format */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		i2s_intf->multi24 = 0;
		i2s_intf->tx_ctrl = I2S_TX_UNISON_BIT;
		i2s_intf->word_len = 15;
		if (i2s_intf->mode == I2S_DSP_MODE) {
			i2s_intf->slots = i2s_intf->channels - 1;
			i2s_intf->word_pos = 15;
		} else {
			i2s_intf->slots = 0;
			i2s_intf->word_pos = 0;
		}
		priv_data->capture_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		break;

	case SNDRV_PCM_FORMAT_S24_LE:		/* 32 bits, valie data in low 3 bytes. */
		i2s_intf->multi24 = I2S_24BITMUX_MODE_ENABLE;
		i2s_intf->tx_ctrl = 0;
		i2s_intf->word_len = 23;
		if (i2s_intf->mode == I2S_DSP_MODE) {
			i2s_intf->slots = i2s_intf->channels - 1;
			i2s_intf->word_pos = 0; /* ignored */
		} else {
			i2s_intf->slots = 0;
			i2s_intf->word_pos = 0; /* ignored */
		}
		priv_data->capture_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		break;

	case SNDRV_PCM_FORMAT_S32_LE:
		i2s_intf->multi24 = I2S_24BITMUX_MODE_ENABLE;
		i2s_intf->tx_ctrl = 0;
		i2s_intf->word_len = 31;
		if (i2s_intf->mode == I2S_DSP_MODE) {
			i2s_intf->slots = i2s_intf->channels - 1;
			i2s_intf->word_pos = 0; /* ignored */
		} else {
			i2s_intf->slots = 0;
			i2s_intf->word_pos = 0; /* ignored */
		}
		priv_data->capture_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		break;

	default:
		goto hw_params_exit;
	}

	if (priv_data->dai_master == true) {
		i2s_intf->tx_ctrl |= I2S_TX_WS_MST_BIT;
		i2s_intf->rx_ctrl |= I2S_RX_WS_MST_BIT;
	} else {
		i2s_intf->tx_ctrl &= ~I2S_TX_WS_MST_BIT;
		i2s_intf->rx_ctrl &= ~I2S_RX_WS_MST_BIT;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		amba_writel(I2S_TX_CTRL_REG, i2s_intf->tx_ctrl);
		amba_writel(I2S_TX_FIFO_LTH_REG, 0x10);
	} else {
		amba_writel(I2S_RX_CTRL_REG, i2s_intf->rx_ctrl);
		amba_writel(I2S_RX_FIFO_GTH_REG, 0x20);
	}

	amba_writel(I2S_MODE_REG, i2s_intf->mode);
	amba_writel(I2S_WLEN_REG, i2s_intf->word_len);
	amba_writel(I2S_WPOS_REG, i2s_intf->word_pos);
	amba_writel(I2S_SLOT_REG, i2s_intf->slots);
	amba_writel(I2S_24BITMUX_MODE_REG, i2s_intf->multi24);

	/* Set clock */
	clk_set_rate(priv_data->mclk, i2s_intf->mclk);

	mutex_lock(&clock_reg_mutex);

	clock_reg = amba_readl(I2S_CLOCK_REG);
	clock_reg &= ~DAI_CLOCK_MASK;
	clock_reg |= i2s_intf->div;

	if (priv_data->dai_master == true)
		clock_reg |= I2S_CLK_MASTER_MODE;
	else
		clock_reg &= ~I2S_CLK_MASTER_MODE;

	if (bclk_reverse)
		clock_reg &= ~I2S_CLK_TX_PO_FALL;
	else
		clock_reg |= I2S_CLK_TX_PO_FALL;

	amba_writel(I2S_CLOCK_REG, clock_reg);

	mutex_unlock(&clock_reg_mutex);

	dai_rx_enable();
	dai_tx_enable();

	msleep(1);

	/* Notify HDMI that the audio interface is changed */
	ambarella_audio_notify_transition(&priv_data->amb_i2s_intf,
		AUDIO_NOTIFY_SETHWPARAMS);
	return 0;

hw_params_exit:
	dai_rx_enable();
	dai_tx_enable();
	return -EINVAL;
}

static int ambarella_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
		struct snd_soc_dai *cpu_dai)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			dai_tx_enable();
		else
			dai_rx_enable();
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
			dai_tx_disable();
			dai_tx_fifo_rst();
		}else{
			dai_rx_disable();
			dai_rx_fifo_rst();
		}
		break;
	default:
		break;
	}

	return 0;
}

/*
 * Set Ambarella I2S DAI format
 */
static int ambarella_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_LEFT_J:
		priv_data->amb_i2s_intf.mode = I2S_LEFT_JUSTIFIED_MODE;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		priv_data->amb_i2s_intf.mode = I2S_RIGHT_JUSTIFIED_MODE;
		break;
	case SND_SOC_DAIFMT_I2S:
		priv_data->amb_i2s_intf.mode = I2S_I2S_MODE;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		priv_data->amb_i2s_intf.mode = I2S_DSP_MODE;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		priv_data->dai_master = true;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		if (priv_data->amb_i2s_intf.mode != I2S_I2S_MODE) {
			printk("DAI can't work in slave mode without standard I2S format!\n");
			return -EINVAL;
		}
		priv_data->dai_master = false;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ambarella_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);

	switch (clk_id) {
	case AMBARELLA_CLKSRC_ONCHIP:
		priv_data->amb_i2s_intf.clksrc = clk_id;
		priv_data->amb_i2s_intf.mclk = freq;
		break;

	case AMBARELLA_CLKSRC_EXTERNAL:
	default:
		printk("CLK SOURCE (%d) is not supported yet\n", clk_id);
		return -EINVAL;
	}

	return 0;
}

static int ambarella_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
		int div_id, int div)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);

	switch (div_id) {
	case AMBARELLA_CLKDIV_LRCLK:
		priv_data->amb_i2s_intf.div = div;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_PM
static int ambarella_i2s_dai_suspend(struct snd_soc_dai *dai)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(dai);
	struct ambarella_i2s_interface *i2s_intf = &priv_data->amb_i2s_intf;

	priv_data->clock_reg = amba_readl(I2S_CLOCK_REG);
	i2s_intf->mode = amba_readl(I2S_MODE_REG);
	i2s_intf->word_len = amba_readl(I2S_WLEN_REG);
	i2s_intf->word_pos = amba_readl(I2S_WPOS_REG);
	i2s_intf->slots = amba_readl(I2S_SLOT_REG);
	i2s_intf->channels = amba_readl(I2S_REG(I2S_CHANNEL_SELECT_OFFSET));
	i2s_intf->rx_ctrl = amba_readl(I2S_RX_CTRL_REG);
	i2s_intf->tx_ctrl = amba_readl(I2S_TX_CTRL_REG);
	i2s_intf->rx_fifo_len = amba_readl(I2S_RX_FIFO_GTH_REG);
	i2s_intf->tx_fifo_len = amba_readl(I2S_TX_FIFO_LTH_REG);
	i2s_intf->multi24 = amba_readl(I2S_24BITMUX_MODE_REG);

	return 0;
}

static int ambarella_i2s_dai_resume(struct snd_soc_dai *dai)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(dai);
	struct ambarella_i2s_interface *i2s_intf = &priv_data->amb_i2s_intf;

	amba_writel(I2S_MODE_REG, i2s_intf->mode);
	amba_writel(I2S_WLEN_REG, i2s_intf->word_len);
	amba_writel(I2S_WPOS_REG, i2s_intf->word_pos);
	amba_writel(I2S_SLOT_REG, i2s_intf->slots);
	amba_writel(I2S_REG(I2S_CHANNEL_SELECT_OFFSET), i2s_intf->channels);
	amba_writel(I2S_RX_CTRL_REG, i2s_intf->rx_ctrl);
	amba_writel(I2S_TX_CTRL_REG, i2s_intf->tx_ctrl);
	amba_writel(I2S_RX_FIFO_GTH_REG, i2s_intf->rx_fifo_len);
	amba_writel(I2S_TX_FIFO_LTH_REG, i2s_intf->tx_fifo_len);
	amba_writel(I2S_24BITMUX_MODE_REG,i2s_intf->multi24);
	amba_writel(I2S_CLOCK_REG, priv_data->clock_reg);

	return 0;
}
#else /* CONFIG_PM */
#define ambarella_i2s_dai_suspend	NULL
#define ambarella_i2s_dai_resume	NULL
#endif /* CONFIG_PM */

static int ambarella_i2s_dai_probe(struct snd_soc_dai *dai)
{
	u32 sfreq, clk_div = 3;
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(dai);
	struct ambarella_i2s_interface *i2s_intf = &priv_data->amb_i2s_intf;

	dai->capture_dma_data = &priv_data->capture_dma_data;
	dai->playback_dma_data = &priv_data->playback_dma_data;

	if (priv_data->default_mclk == 12288000) {
		sfreq = 48000;
	} else if (priv_data->default_mclk == 11289600){
		sfreq = 44100;
	} else {
		priv_data->default_mclk = 12288000;
		sfreq = 48000;
	}

	clk_set_rate(priv_data->mclk, priv_data->default_mclk);

	/*
	 * Dai default smapling rate, polarity configuration.
	 * Note: Just be configured, actually BCLK and LRCLK will not
	 * output to outside at this time.
	 */
	if(capture_enabled)
		amba_writel(I2S_CLOCK_REG, clk_div | I2S_CLK_TX_PO_FALL);

	priv_data->dai_master = true;
	i2s_intf->mode = I2S_I2S_MODE;
	i2s_intf->clksrc = AMBARELLA_CLKSRC_ONCHIP;
	i2s_intf->mclk = priv_data->default_mclk;
	i2s_intf->div = clk_div;
	i2s_intf->sfreq = sfreq;
	i2s_intf->word_len = 15;
	i2s_intf->word_pos = 0;
	i2s_intf->slots = 0;
	i2s_intf->channels = 2;

	/* reset fifo */
	dai_tx_enable();
	dai_rx_enable();
	dai_fifo_rst();

	/* Notify HDMI that the audio interface is initialized */
	ambarella_audio_notify_transition(&priv_data->amb_i2s_intf, AUDIO_NOTIFY_INIT);

	return 0;
}

static int ambarella_i2s_dai_remove(struct snd_soc_dai *dai)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(dai);

	/* Disable I2S clock output */
	amba_writel(I2S_CLOCK_REG, 0x0);

	/* Notify that the audio interface is removed */
	ambarella_audio_notify_transition(&priv_data->amb_i2s_intf,
		AUDIO_NOTIFY_REMOVE);

	return 0;
}

static const struct snd_soc_dai_ops ambarella_i2s_dai_ops = {
	.prepare = ambarella_i2s_prepare,
	.startup = ambarella_i2s_startup,
	.hw_params = ambarella_i2s_hw_params,
	.trigger = ambarella_i2s_trigger,
	.set_fmt = ambarella_i2s_set_fmt,
	.set_sysclk = ambarella_i2s_set_sysclk,
	.set_clkdiv = ambarella_i2s_set_clkdiv,
};

static struct snd_soc_dai_driver ambarella_i2s_dai = {
	.probe = ambarella_i2s_dai_probe,
	.remove = ambarella_i2s_dai_remove,
	.suspend = ambarella_i2s_dai_suspend,
	.resume = ambarella_i2s_dai_resume,
	.playback = {
		.channels_min = 2,
		.channels_max = 0, // initialized in ambarella_i2s_probe function
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE),
	},
	.capture = {
		.channels_min = 2,
		.channels_max = 0, // initialized in ambarella_i2s_probe function
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE),
	},
	.ops = &ambarella_i2s_dai_ops,
	.symmetric_rates = 1,
};

static const struct snd_soc_component_driver ambarella_i2s_component = {
	.name		= "ambarella-i2s",
};

static int ambarella_i2s_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct amb_i2s_priv *priv_data;
	struct pinctrl *pinctrl;
	int channels, rval;

	priv_data = devm_kzalloc(&pdev->dev, sizeof(*priv_data), GFP_KERNEL);
	if (priv_data == NULL)
		return -ENOMEM;

	priv_data->playback_dma_data.addr = I2S_TX_LEFT_DATA_DMA_REG;
	priv_data->playback_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	priv_data->playback_dma_data.maxburst = 32;

	priv_data->capture_dma_data.addr = I2S_RX_DATA_DMA_REG;
	priv_data->capture_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	priv_data->capture_dma_data.maxburst = 32;
	priv_data->mclk = clk_get(&pdev->dev, "gclk_audio");
	if (IS_ERR(priv_data->mclk)) {
		dev_err(&pdev->dev, "Get audio clk failed!\n");
		return PTR_ERR(priv_data->mclk);
	}

	dev_set_drvdata(&pdev->dev, priv_data);

	rval = of_property_read_u32(np, "amb,i2s-channels", &channels);
	if (rval < 0) {
		dev_err(&pdev->dev, "Get channels failed! %d\n", rval);
		return -ENXIO;
	}

	of_property_read_u32(np, "amb,default-mclk", &priv_data->default_mclk);

	ambarella_i2s_dai.playback.channels_max = channels;
	ambarella_i2s_dai.capture.channels_max = channels;

	pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(pinctrl))
		return PTR_ERR(pinctrl);

	rval = snd_soc_register_component(&pdev->dev,
			&ambarella_i2s_component,  &ambarella_i2s_dai, 1);
	if (rval < 0){
		dev_err(&pdev->dev, "register DAI failed\n");
		return rval;
	}

	rval = ambarella_pcm_platform_register(&pdev->dev);
	if (rval) {
		dev_err(&pdev->dev, "register PCM failed: %d\n", rval);
		snd_soc_unregister_component(&pdev->dev);
		return rval;
	}

	return 0;
}

static int ambarella_i2s_remove(struct platform_device *pdev)
{
	ambarella_pcm_platform_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);

	return 0;
}

static const struct of_device_id ambarella_i2s_dt_ids[] = {
	{ .compatible = "ambarella,i2s", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ambarella_i2s_dt_ids);

static struct platform_driver ambarella_i2s_driver = {
	.probe = ambarella_i2s_probe,
	.remove = ambarella_i2s_remove,

	.driver = {
		.name = "ambarella-i2s",
		.owner = THIS_MODULE,
		.of_match_table = ambarella_i2s_dt_ids,
	},
};

module_platform_driver(ambarella_i2s_driver);

MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella Soc I2S Interface");

MODULE_LICENSE("GPL");

