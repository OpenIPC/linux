/*
 * ALSA SoC I2S (McBSP) Audio Layer for TI DAVINCI processor
 *
 * Author:      Vladimir Barinov, <vbarinov@embeddedalley.com>
 * Copyright:   (C) 2007 MontaVista Software, Inc., <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/irqreturn.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <linux/kernel.h>
//#include <mach/asp.h>

#include "fullhan-pcm.h"
#include "fh_i2s.h"

#define I2S_FIFO_LEN_RX  40
#define I2S_FIFO_LEN_TX  40
extern void i2s_irq_enquen(int type, u8 *buff, u8 len,u8 reset);
/*
 * NOTE:  terminology here is confusing.
 *
 *  - This driver supports the "Audio Serial Port" (ASP),
 *    found on dm6446, dm355, and other DaVinci chips.
 *
 *  - But it labels it a "Multi-channel Buffered Serial Port"
 *    (McBSP) as on older chips like the dm642 ... which was
 *    backward-compatible, possibly explaining that confusion.
 *
 *  - OMAP chips have a controller called McBSP, which is
 *    incompatible with the DaVinci flavor of McBSP.
 *
 *  - Newer DaVinci chips have a controller called McASP,
 *    incompatible with ASP and with either McBSP.
 *
 * In short:  this uses ASP to implement I2S, not McBSP.
 * And it won't be the only DaVinci implemention of I2S.
 */
#define DAVINCI_MCBSP_DRR_REG	0x00
#define DAVINCI_MCBSP_DXR_REG	0x04
#define DAVINCI_MCBSP_SPCR_REG	0x08
#define DAVINCI_MCBSP_RCR_REG	0x0c
#define DAVINCI_MCBSP_XCR_REG	0x10
#define DAVINCI_MCBSP_SRGR_REG	0x14
#define DAVINCI_MCBSP_PCR_REG	0x24

#define DAVINCI_MCBSP_SPCR_RRST		(1 << 0)
#define DAVINCI_MCBSP_SPCR_RINTM(v)	((v) << 4)
#define DAVINCI_MCBSP_SPCR_XRST		(1 << 16)
#define DAVINCI_MCBSP_SPCR_XINTM(v)	((v) << 20)
#define DAVINCI_MCBSP_SPCR_GRST		(1 << 22)
#define DAVINCI_MCBSP_SPCR_FRST		(1 << 23)
#define DAVINCI_MCBSP_SPCR_FREE		(1 << 25)

#define DAVINCI_MCBSP_RCR_RWDLEN1(v)	((v) << 5)
#define DAVINCI_MCBSP_RCR_RFRLEN1(v)	((v) << 8)
#define DAVINCI_MCBSP_RCR_RDATDLY(v)	((v) << 16)
#define DAVINCI_MCBSP_RCR_RFIG		(1 << 18)
#define DAVINCI_MCBSP_RCR_RWDLEN2(v)	((v) << 21)
#define DAVINCI_MCBSP_RCR_RFRLEN2(v)	((v) << 24)
#define DAVINCI_MCBSP_RCR_RPHASE	BIT(31)

#define DAVINCI_MCBSP_XCR_XWDLEN1(v)	((v) << 5)
#define DAVINCI_MCBSP_XCR_XFRLEN1(v)	((v) << 8)
#define DAVINCI_MCBSP_XCR_XDATDLY(v)	((v) << 16)
#define DAVINCI_MCBSP_XCR_XFIG		(1 << 18)
#define DAVINCI_MCBSP_XCR_XWDLEN2(v)	((v) << 21)
#define DAVINCI_MCBSP_XCR_XFRLEN2(v)	((v) << 24)
#define DAVINCI_MCBSP_XCR_XPHASE	BIT(31)

#define DAVINCI_MCBSP_SRGR_FWID(v)	((v) << 8)
#define DAVINCI_MCBSP_SRGR_FPER(v)	((v) << 16)
#define DAVINCI_MCBSP_SRGR_FSGM		(1 << 28)
#define DAVINCI_MCBSP_SRGR_CLKSM	BIT(29)

#define DAVINCI_MCBSP_PCR_CLKRP		(1 << 0)
#define DAVINCI_MCBSP_PCR_CLKXP		(1 << 1)
#define DAVINCI_MCBSP_PCR_FSRP		(1 << 2)
#define DAVINCI_MCBSP_PCR_FSXP		(1 << 3)
#define DAVINCI_MCBSP_PCR_SCLKME	(1 << 7)
#define DAVINCI_MCBSP_PCR_CLKRM		(1 << 8)
#define DAVINCI_MCBSP_PCR_CLKXM		(1 << 9)
#define DAVINCI_MCBSP_PCR_FSRM		(1 << 10)
#define DAVINCI_MCBSP_PCR_FSXM		(1 << 11)

enum {
	DAVINCI_MCBSP_WORD_8 = 0,
	DAVINCI_MCBSP_WORD_12,
	DAVINCI_MCBSP_WORD_16,
	DAVINCI_MCBSP_WORD_20,
	DAVINCI_MCBSP_WORD_24,
	DAVINCI_MCBSP_WORD_32,
};
struct my_data{
	struct work_struct my_work;
	int value;
	u8 buff;
	u8 len;
	void __iomem *base;
	};
struct my_data *rx_md,*tx_md;
struct work_struct rx_work_queue,tx_work_queue;
int g_i2s_base;
//init test data
struct my_data *init_data(struct my_data *md,struct work_struct  work_queue)
{
md = (struct my_data *)kmalloc(sizeof(struct my_data),GFP_KERNEL);
md->my_work=work_queue;
return md;
}


static const unsigned char data_type[SNDRV_PCM_FORMAT_S32_LE + 1] = {
		[SNDRV_PCM_FORMAT_S8] = 1, [SNDRV_PCM_FORMAT_S16_LE] = 2,
		[SNDRV_PCM_FORMAT_S32_LE] = 4, };

static const unsigned char asp_word_length[SNDRV_PCM_FORMAT_S32_LE + 1] = {
		[SNDRV_PCM_FORMAT_S8] = DAVINCI_MCBSP_WORD_8, [SNDRV_PCM_FORMAT_S16_LE
				] = DAVINCI_MCBSP_WORD_16, [SNDRV_PCM_FORMAT_S32_LE
				] = DAVINCI_MCBSP_WORD_32, };

static const unsigned char double_fmt[SNDRV_PCM_FORMAT_S32_LE + 1] = {
		[SNDRV_PCM_FORMAT_S8] = SNDRV_PCM_FORMAT_S16_LE,
		[SNDRV_PCM_FORMAT_S16_LE] = SNDRV_PCM_FORMAT_S32_LE, };
enum dma_event_q {
	EVENTQ_0 = 0, EVENTQ_1 = 1, EVENTQ_2 = 2, EVENTQ_3 = 3, EVENTQ_DEFAULT = -1
};
struct davinci_pcm_dma_params {
	int channel; /* sync dma channel ID */
	unsigned short acnt;
	dma_addr_t dma_addr; /* device physical address for DMA */
	unsigned sram_size;
	enum dma_event_q asp_chan_q; /* event queue number for ASP channel */
	enum dma_event_q ram_chan_q; /* event queue number for RAM channel */
	unsigned char data_type; /* xfer data type */
	unsigned char convert_mono_stereo;
	unsigned int fifo_level;
	int (*trigger)(struct snd_pcm_substream *substream, int cmd, int after);
};
struct s6000_i2s_dev {
	dma_addr_t sifbase;
	u8 __iomem *scbbase;
	unsigned int wide;
	unsigned int channel_in;
	unsigned int channel_out;
	unsigned int lines_in;
	unsigned int lines_out;
	struct s6000_pcm_dma_params dma_params;
	int irq;
	void __iomem *base;
	struct clk *clk;
	struct device *dev;
};
struct davinci_mcbsp_dev {
	struct device *dev;
	struct davinci_pcm_dma_params dma_params;
	void __iomem *base;
#define MOD_DSP_A	0
#define MOD_DSP_B	1
	int mode;
	u32 pcr;
	struct clk *clk;
	/*
	 * Combining both channels into 1 element will at least double the
	 * amount of time between servicing the dma channel, increase
	 * effiency, and reduce the chance of overrun/underrun. But,
	 * it will result in the left & right channels being swapped.
	 *
	 * If relabeling the left and right channels is not possible,
	 * you may want to let the codec know to swap them back.
	 *
	 * It may allow x10 the amount of time to service dma requests,
	 * if the codec is master and is using an unnecessarily fast bit clock
	 * (ie. tlvaic23b), independent of the sample rate. So, having an
	 * entire frame at once means it can be serviced at the sample rate
	 * instead of the bit clock rate.
	 *
	 * In the now unlikely case that an underrun still
	 * occurs, both the left and right samples will be repeated
	 * so that no pops are heard, and the left and right channels
	 * won't end up being swapped because of the underrun.
	 */
	unsigned enable_channel_combine :1;

	unsigned int fmt;
	int clk_div;
	int clk_input_pin;
	bool i2s_accurate_sck;
};
struct i2c_adapter *codec_i2c_adapter;
void set_i2c_codec_adapter(struct i2c_adapter * adapter) {
	codec_i2c_adapter = adapter;
}
EXPORT_SYMBOL(set_i2c_codec_adapter);

int i2c_write_codec(u8 addr, u8 data) {
	int rval;
	struct i2c_msg msgs[1];
	u8 send[2];
	msgs[0].len = 2;
	msgs[0].addr = 0x1a;
	msgs[0].flags = 0;
	msgs[0].buf = send;
	send[0] = addr;
	send[1] = data;
	rval = i2c_transfer(codec_i2c_adapter, msgs, 1);
	return rval;
}

static inline void davinci_mcbsp_write_reg(struct davinci_mcbsp_dev *dev,
		int reg, u32 val) {
	__raw_writel(val, dev->base + reg);
}

static inline u32 davinci_mcbsp_read_reg(struct davinci_mcbsp_dev *dev, int reg) {
	return __raw_readl(dev->base + reg);
}

static void toggle_clock(struct davinci_mcbsp_dev *dev, int playback) {
	u32 m = playback ? DAVINCI_MCBSP_PCR_CLKXP : DAVINCI_MCBSP_PCR_CLKRP;
	/* The clock needs to toggle to complete reset.
	 * So, fake it by toggling the clk polarity.
	 */
	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_PCR_REG, dev->pcr ^ m);
	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_PCR_REG, dev->pcr);
}

static void davinci_mcbsp_start(struct davinci_mcbsp_dev *dev,
		struct snd_pcm_substream *substream) {
//	struct snd_soc_pcm_runtime *rtd = substream->private_data;
//	struct snd_soc_platform *platform = rtd->platform;
//	int playback = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
//	u32 spcr;
//	u32 mask = playback ? DAVINCI_MCBSP_SPCR_XRST : DAVINCI_MCBSP_SPCR_RRST;
//	spcr = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SPCR_REG);
//	if (spcr & mask) {
//		/* start off disabled */
//		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG,
//				spcr & ~mask);
//		toggle_clock(dev, playback);
//	}
//	if (dev->pcr & (DAVINCI_MCBSP_PCR_FSXM | DAVINCI_MCBSP_PCR_FSRM |
//			DAVINCI_MCBSP_PCR_CLKXM | DAVINCI_MCBSP_PCR_CLKRM)) {
//		/* Start the sample generator */
//		spcr |= DAVINCI_MCBSP_SPCR_GRST;
//		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, spcr);
//	}
//
//	if (playback) {
//		/* Stop the DMA to avoid data loss */
//		/* while the transmitter is out of reset to handle XSYNCERR */
//		if (platform->driver->ops->trigger) {
//			int ret = platform->driver->ops->trigger(substream,
//				SNDRV_PCM_TRIGGER_STOP);
//			if (ret < 0)
//				printk(KERN_DEBUG "Playback DMA stop failed\n");
//		}
//
//		/* Enable the transmitter */
//		spcr = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SPCR_REG);
//		spcr |= DAVINCI_MCBSP_SPCR_XRS		SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |

//		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, spcr);
//
//		/* wait for any unexpected frame sync error to occur */
//		udelay(100);
//
//		/* Disable the transmitter to clear any outstanding XSYNCERR */
//		spcr = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SPCR_REG);
//		spcr &= ~DAVINCI_MCBSP_SPCR_XRST;
//		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, spcr);
//		toggle_clock(dev, playback);
//
//		/* Restart the DMA */
//		if (platform->driver->ops->trigger) {
//			int ret = platform->driver->ops->trigger(substream,
//				SNDRV_PCM_TRIGGER_START);2
//			if (ret < 0)
//				printk(KERN_DEBUG "Playback DMA start failed\n");
//		}
//	}
//
//	/* Enable transmitter or receiver */
//	spcr = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SPCR_REG);
//	spcr |= mask;
//
//	if (dev->pcr & (DAVINCI_MCBSP_PCR_FSXM | DAVINCI_MCBSP_PCR_FSRM)) {
//		/* Start frame sync */
//		spcr |= DAVINCI_MCBSP_SPCR_FRST;
//	}
//	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, spcr);
}

static void davinci_mcbsp_stop(struct davinci_mcbsp_dev *dev, int playback) {

//	u32 spcr;
//
//	/* Reset transmitter/receiver and sample rate/frame sync generators */
//	spcr = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SPCR_REG);
//	spcr &= ~(DAVINCI_MCBSP_SPCR_GRST | DAVINCI_MCBSP_SPCR_FRST);
//	spcr &= playback ? ~DAVINCI_MCBSP_SPCR_XRST : ~DAVINCI_MCBSP_SPCR_RRST;
//	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, spcr);
//	toggle_clock(dev, playback);
}

#define DEFAULT_BITPERSAMPLE	16

static int davinci_i2s_set_dai_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt) {
//	struct davinci_mcbsp_dev *dev = snd_soc_dai_get_drvdata(cpu_dai);
//	unsigned int pcr;
//	unsigned int srgr;
//	/* Attention srgr is updated by hw_params! */
//	srgr = DAVINCI_MCBSP_SRGR_FSGM |
//		DAVINCI_MCBSP_SRGR_FPER(DEFAULT_BITPERSAMPLE * 2 - 1) |
//		DAVINCI_MCBSP_SRGR_FWID(DEFAULT_BITPERSAMPLE - 1);
//
//	dev->fmt = fmt;
//	/* set master/slave audio interface */
//	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
//	case SND_SOC_DAIFMT_CBS_CFS:
//		/* cpu is master */
//		pcr = DAVINCI_MCBSP_PCR_FSXM |
//			DAVINCI_MCBSP_PCR_FSRM |
//			DAVINCI_MCBSP_PCR_CLKXM |
//			DAVINCI_MCBSP_PCR_CLKRM;
//		break;
//	case SND_SOC_DAIFMT_CBM_CFS:
//		pcr = DAVINCI_MCBSP_PCR_FSRM | DAVINCI_MCBSP_PCR_FSXM;
//		/*
//		 * Selection of the clock input pin that is the
//		 * input for the Sample Rate Generator.
//		 * McBSP FSR and FSX are driven by the Sample Rate
//		 * Generator.
//		 */
//		switch (dev->clk_input_pin) {
//		case MCBSP_CLKS:
//			pcr |= DAVINCI_MCBSP_PCR_CLKXM |
//				DAVINCI_MCBSP_PCR_CLKRM;
//			break;
//		case MCBSP_CLKR:
//			pcr |= DAVINCI_MCBSP_PCR_SCLKME;
//			break;
//		default:
//			dev_err(dev->dev, "bad clk_input_pin\n");
//			return -EINVAL;
//		}
//
//		break;
//	case SND_SOC_DAIFMT_CBM_CFM:
//		/* codec is master */
//		pcr = 0;
//		break;
//	default:
//		printk(KERN_ERR "%s:bad master\n", __func__);
//		return -EINVAL;
//	}
//
//	/* interface format */
//	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
//	case SND_SOC_DAIFMT_I2S:
//		/* Davinci doesn't support TRUE I2S, but some codecs will have
//		 * the left and right channels contiguous. This allows
//		 * dsp_a mode to be used with an inverted normal frame clk.
//		 * If your codec is master and does not have contiguous
//		 * channels, then you will have sound on only one channel.
//		 * Try using a different mode, or codec as slave.
//		 *
//		 * The TLV320AIC33 is an example of a codec where this works.
//		 * It has a variable bit clock frequency allowing it to have
//		 * valid data on every bit clock.
//		 *
//		 * The TLV320AIC23 is an example of a codec where this does not
//		 * work. It has a fixed bit clock frequency with progressively
//		 * more empty bit clock slots between channels as the sample
//		 * rate is lowered.
//		 */
//		fmt ^= SND_SOC_DAIFMT_NB_IF;
//	case SND_SOC_DAIFMT_DSP_A:
//		dev->mode = MOD_DSP_A;
//		break;
//	case SND_SOC_DAIFMT_DSP_B:
//		dev->mode = MOD_DSP_B;
//		break;
//	default:
//		printk(KERN_ERR "%s:bad format\n", __func__);
//		return -EINVAL;
//	}
//
//	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
//	case SND_SOC_DAIFMT_NB_NF:
//		/* CLKRP Receive clock polarity,
//		 *	1 - sampled on rising edge of CLKR
//		 *	valid on rising edge
//		 * CLKXP Transmit clock polarity,
//		 *	1 - clocked on falling edge of CLKX
//		 *	valid on rising edge
//		 * FSRP  Receive frame sync pol, 0 - active high
//		 * FSXP  Transmit frame sync pol, 0 - active high
//		 */
//		pcr |= (DAVINCI_MCBSP_PCR_CLKXP | DAVINCI_MCBSP_PCR_CLKRP);
//		break;
//	case SND_SOC_DAIFMT_IB_IF:
//		/* CLKRP Receive clock polarity,
//		 *	0 - sampled on falling edge of CLKR
//		 *	valid on falling edge
//		 * CLKXP Transmit clock polarity,
//		 *	0 - clocked on rising edge of CLKX
//		 *	valid on falling edge
//		 * FSRP  Receive frame sync pol, 1 - active low
//		 * FSXP  Transmit frame sync pol, 1 - active low
//		 */
//		pcr |= (DAVINCI_MCBSP_PCR_FSXP | DAVINCI_MCBSP_PCR_FSRP);
//		break;
//	case SND_SOC_DAIFMT_NB_IF:
//		/* CLKRP Receive clock polarity,
//		 *	1 - sampled on rising edge of CLKR
//		 *	valid on rising edge
//		 * CLKXP Transmit clock polarity,
//		 *	1 - clocked on falling edge of CLKX
//		 *	valid on rising edge
//		 * FSRP  Receive frame sync pol, 1 - active low
//		 * FSXP  Transmit frame sync pol, 1 - active low
//		 */
//		pcr |= (DAVINCI_MCBSP_PCR_CLKXP | DAVINCI_MCBSP_PCR_CLKRP |
//			DAVINCI_MCBSP_PCR_FSXP | DAVINCI_MCBSP_PCR_FSRP);
//		break;
//	case SND_SOC_DAIFMT_IB_NF:
//		/* CLKRP Receive clock polarity,
//		 *	0 - sampled on falling edge of CLKR
//		 *	valid on falling edge
//		 * CLKXP Transmit clock polarity,
//		 *	0 - clocked on rising edge of CLKX
//		 *	valid on falling edge
//		 * FSRP  Receive frame sync pol, 0 - active high
//		 * FSXP  Transmit frame sync pol, 0 - active high
//		 */
//		break;
//	default:
//		return -EINVAL;
//	}
//	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SRGR_REG, srgr);
//	dev->pcr = pcr;
//	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_PCR_REG, pcr);
	return 0;
}

static int davinci_i2s_dai_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id,
		int div) {

//	struct davinci_mcbsp_dev *dev = snd_soc_dai_get_drvdata(cpu_dai);
//
//	if (div_id != DAVINCI_MCBSP_CLKGDV)
//		return -ENODEV;
//
//	dev->clk_div = div;
	return 0;
}

static int davinci_i2s_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai) {


	struct davinci_mcbsp_dev *dev = snd_soc_dai_get_drvdata(dai);
	struct davinci_pcm_dma_params *dma_params = &dev->dma_params;
	struct snd_interval *i = NULL;
	int mcbsp_word_length, master;
	unsigned int rcr, xcr, srgr, clk_div, freq, framesize;
	u32 spcr;
	snd_pcm_format_t fmt;
	unsigned element_cnt = 1;

	/* general line settings */
#if 0
	spcr = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SPCR_REG);
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		spcr |= DAVINCI_MCBSP_SPCR_RINTM(3) | DAVINCI_MCBSP_SPCR_FREE;
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, spcr);
	} else {
		spcr |= DAVINCI_MCBSP_SPCR_XINTM(3) | DAVINCI_MCBSP_SPCR_FREE;
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, spcr);
	}

	master = dev->fmt & SND_SOC_DAIFMT_MASTER_MASK;
	fmt = params_format(params);
	mcbsp_word_length = asp_word_length[fmt];

	switch (master) {
		case SND_SOC_DAIFMT_CBS_CFS:
		freq = clk_get_rate(dev->clk);
		srgr = DAVINCI_MCBSP_SRGR_FSGM |
		DAVINCI_MCBSP_SRGR_CLKSM;
		srgr |= DAVINCI_MCBSP_SRGR_FWID(mcbsp_word_length *
				8 - 1);
		if (dev->i2s_accurate_sck) {
			clk_div = 256;
			do {
				framesize = (freq / (--clk_div)) /
				params->rate_num *
				params->rate_den;
			}while (((framesize < 33) || (fram		SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
					SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_JOINT_DUPLEX), .formats =
					(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE), .rates =
					(SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_5512 |
					SNDRV_PCM_RATE_8000_192000), .rate_min = 0, .rate_max = 1562500,
					.channels_min = 2, .channels_max = 8, .buffer_bytes_max = 0x7ffffff0,
					.period_bytes_min = 16, .period_bytes_max = 0xfffff0, .periods_min = 2,
					.periods_max = 1024, /* no limit */
					.fifo_size = 0, };
esize > 4095)) &&
					(clk_div));
			clk_div--;
			srgr |= DAVINCI_MCBSP_SRGR_FPER(framesize - 1);
		} else {
			/* symmetric waveforms */
			clk_div = freq / (mcbsp_word_length * 16) /
			params->rate_num * params->rate_den;
			srgr |= DAVINCI_MCBSP_SRGR_FPER(mcbsp_word_length *
					16 - 1);
		}
		clk_div &= 0xFF;
		srgr |= clk_div;
		break;
		case SND_SOC_DAIFMT_CBM_CFS:
		srgr = DAVINCI_MCBSP_SRGR_FSGM;
		clk_div = dev->clk_div - 1;
		srgr |= DAVINCI_MCBSP_SRGR_FWID(mcbsp_word_length * 8 - 1);
		srgr |= DAVINCI_MCBSP_SRGR_FPER(mcbsp_word_length * 16 - 1);
		clk_div &= 0xFF;
		srgr |= clk_div;
		break;
		case SND_SOC_DAIFMT_CBM_CFM:
		/* Clock and frame sync given from external sources */
		i = hw_param_interval(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS);
		srgr = DAVINCI_MCBSP_SRGR_FSGM;
		srgr |= DAVINCI_MCBSP_SRGR_FWID(snd_interval_value(i) - 1);
		pr_debug("%s - %d  FWID set: re-read srgr = %X\n",
				__func__, __LINE__, snd_interval_value(i) - 1);

		i = hw_param_interval(params, SNDRV_PCM_HW_PARAM_FRAME_BITS);
		srgr |= DAVINCI_MCBSP_SRGR_FPER(snd_interval_value(i) - 1);
		break;
		default:
		return -EINVAL;
	}
	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SRGR_REG, srgr);

	rcr = DAVINCI_MCBSP_RCR_RFIG;
	xcr = DAVINCI_MCBSP_XCR_XFIG;
	if (dev->mode == MOD_DSP_B) {
		rcr |= DAVINCI_MCBSP_RCR_RDATDLY(0);
		xcr |= DAVINCI_MCBSP_XCR_XDATDLY(0);
	} else {
		rcr |= DAVINCI_MCBSP_RCR_RDATDLY(1);
		xcr |= DAVINCI_MCBSP_XCR_XDATDLY(1);
	}
	/* Determine xfer data type */
	fmt = params_format(params);
	if ((fmt > SNDRV_PCM_FORMAT_S32_LE) || !data_type[fmt]) {
		printk(KERN_WARNING "davinci-i2s: unsupported PCM format\n");
		return -EINVAL;
	}

	if (params_channels(params) == 2) {
		element_cnt = 2;
		if (double_fmt[fmt] && dev->enad work_func(struct work_struct *work)
		{
			printk("%s \n",__FUNCTION__);ble_channel_combine) {
			element_cnt = 1;
			fmt = double_fmt[fmt];
		}
		switch (master) {
			case SND_SOC_DAIFMT_CBS_CFS:
			case SND_SOC_DAIFMT_CBS_CFM:
			rcr |= DAVINCI_MCBSP_RCR_RFRLEN2(0);
			xcr |= DAVINCI_MCBSP_XCR_XFRLEN2(0);
			rcr |= DAVINCI_MCBSP_RCR_RPHASE;
			xcr |= DAVINCI_MCBSP_XCR_XPHASE;
			break;
			case SND_SOC_DAIFMT_CBM_CFM:
			case SND_SOC_DAIFMT_CBM_CFS:
			rcr |= DAVINCI_MCBSP_RCR_RFRLEN2(element_cnt - 1);
			xcr |= DAVINCI_MCBSP_XCR_XFRLEN2(element_cnt - 1);
			break;
			default:
			return -EINVAL;
		}
	}
//	dma_params->acnt = dma_params->data_type = data_type[fmt];
//	dma_params->fifo_level = 0;
	mcbsp_word_length = asp_word_length[fmt];

	switch (master) {
		case SND_SOC_DAIFMT_CBS_CFS:
		case SND_SOC_DAIFMT_CBS_CFM:
		rcr |= DAVINCI_MCBSP_RCR_RFRLEN1(0);
		xcr |= DAVINCI_MCBSP_XCR_XFRLEN1(0);
		break;
		case SND_SOC_DAIFMT_CBM_CFM:
		case SND_SOC_DAIFMT_CBM_CFS:
		rcr |= DAVINCI_MCBSP_RCR_RFRLEN1(element_cnt - 1);
		xcr |= DAVINCI_MCBSP_XCR_XFRLEN1(element_cnt - 1);
		break;
		default:
		return -EINVAL;
	}

	rcr |= DAVINCI_MCBSP_RCR_RWDLEN1(mcbsp_word_length) |
	DAVINCI_MCBSP_RCR_RWDLEN2(mcbsp_word_length);
	xcr |= DAVINCI_MCBSP_XCR_XWDLEN1(mcbsp_word_length) |
	DAVINCI_MCBSP_XCR_XWDLEN2(mcbsp_word_length);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_XCR_REG, xcr);
	else
	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_RCR_REG, rcr);

	pr_debug("%s - %d  srgr=%X\n", __func__, __LINE__, srgr);
	pr_debug("%s - %d  xcr=%X\n", __func__, __LINE__, xcr);
	pr_debug("%s - %d  rcr=%X\n", __func__, __LINE__, rcr);
#endif


	return 0;
}

static int davinci_i2s_prepare(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai) {

//	struct davinci_mcbsp_dev *dev = snd_soc_dai_get_drvdata(dai);
//	int playback = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
//	davinci_mcbsp_stop(dev, playback);
	return 0;
}
static struct {
	spinlock_t lock;
	void __iomem *regs;
	struct clk *clk;
	unsigned long in_use;
	unsigned long next_heartbeat;
	struct timer_list timer;
	int expect_close;
} dw_i2s;
#define	I2S_IOCTL_BASE	'W'
#define	I2S_GETSUPPORT	_IOR(I2S_IOCTL_BASE, 3, int)
typedef unsigned short UINT16;
typedef unsigned int uint32;
//#define BASEADDR_I2S_REG                    dw_i2s.regs
#define	OFFSET_I2S_IER                      0x0000
#define	OFFSET_I2S_IRER                     0x0004
#define	OFFSET_I2S_ITER                     0x0008
#define	OFFSET_I2S_CER                      0x000c
#define	OFFSET_I2S_CCR                      0x0010
#define	OFFSET_I2S_RXFFR                    0x0014
#define	OFFSET_I2S_TXFFR                    0x0018
#define	OFFSET_I2S_LRBR0                    0x0020
#define	OFFSET_I2S_LRBR1                    0x0060
#define	OFFSET_I2S_LRBR2                    0x00A0
#define	OFFSET_I2S_LRBR3                    0x00E0
#define	OFFSET_I2S_LTHR0                    0x0020
#define	OFFSET_I2S_LTHR1                    0x0060
#define	OFFSET_I2S_LTHR2                    0x00A0
#define OFFSET_I2S_LTHR3                    0x00E0
#define	OFFSET_I2S_RRBR0                    0x0024
#define	OFFSET_I2S_RRBR1                    0x0064
#define	OFFSET_I2S_RRBR2                    0x00A4
#define	OFFSET_I2S_RRBR3                    0x00E4
#define	OFFSET_I2S_RTHR0                    0x0024
#define	OFFSET_I2S_RTHR1                    0x0064
#define	OFFSET_I2S_RTHR2                    0x00A4
#define	OFFSET_I2S_RTHR3                    0x00E4
#define	OFFSET_I2S_RER0                     0x0028
#define	OFFSET_I2S_RER1                     0x0068
#define OFFSET_I2S_RER2                     0x00A8
#define OFFSET_I2S_RER3                     0x00E8
#define	OFFSET_I2S_TER0                     0x002C
#define	OFFSET_I2S_TER1                     0x006C
#define	OFFSET_I2S_TER2                     0x00AC
#define	OFFSET_I2S_TER3                     0x00EC
#define	OFFSET_I2S_RCR0                     0x0030
#define	OFFSET_I2S_RCR1                     0x0070
#define	OFFSET_I2S_RCR2                     0x00B0
#define	OFFSET_I2S_RCR3                     0x00F0
#define	OFFSET_I2S_TCR0                     0x0034
#define	OFFSET_I2S_TCR1                     0x0074
#define	OFFSET_I2S_TCR2                     0x00B4
#define	OFFSET_I2S_TCR3                     0x00F4
#define	OFFSET_I2S_ISR0                     0x0038
#define	OFFSET_I2S_ISR1                     0x0078
#define	OFFSET_I2S_ISR2                     0x00B8
#define	OFFSET_I2S_ISR3                     0x00F8
#define	OFFSET_I2S_IMR0                     0x003C
#define	OFFSET_I2S_IMR1                     0x007C
#define	OFFSET_I2S_IMR2                     0x00BC
#define	OFFSET_I2S_IMR3                     0x00FC
#define	OFFSET_I2S_ROR0                     0x0040
#define OFFSET_I2S_ROR1                     0x0080
#define	OFFSET_I2S_ROR2                     0x00C0
#define	OFFSET_I2S_ROR3                     0x0100
#define	OFFSET_I2S_TOR0                     0x0044
#define	OFFSET_I2S_TOR1                     0x0084
#define	OFFSET_I2S_TOR2                     0x00C4
#define	OFFSET_I2S_TOR3                     0x0104
#define	OFFSET_I2S_RFCR0                    0x0048
#define	OFFSET_I2S_RFCR1                    0x0088
#define	OFFSET_I2S_RFCR2                    0x00C8
#define	OFFSET_I2S_RFCR3                    0x0108
#define OFFSET_I2S_TFCR0                    0x004C
#define OFFSET_I2S_TFCR1                    0x008C
#define	OFFSET_I2S_TFCR2                    0x00CC
#define	OFFSET_I2S_TFCR3                    0x010C
#define	OFFSET_I2S_RFF0                     0x0050
#define	OFFSET_I2S_RFF1                     0x0090
#define	OFFSET_I2S_RFF2                     0x00D4
#define	OFFSET_I2S_RFF3                     0x0110
#define	OFFSET_I2S_TFF0                     0x0054
#define OFFSET_I2S_TFF1                     0x0094
#define OFFSET_I2S_TFF2                     0x00D4
#define	OFFSET_I2S_TFF3                     0x0114
#define	OFFSET_I2S_RXDMA                    0x01C0
#define	OFFSET_I2S_RRXDMA                   0x01C4
#define	OFFSET_I2S_TXDMA                    0x01C8
#define	OFFSET_I2S_RTXDMA                   0x01CC
#define	OFFSET_I2S_COMP_PARAM_2             0x01f0
#define	OFFSET_I2S_COMP_PARAM_1             0x01f4
#define	OFFSET_I2S_COMP_VERSION             0x01f8
#define	OFFSET_I2S_COMP_TYPE                0x01fc
#define RESOLUTION12
#define write_reg(addr,reg)  (*((volatile uint32 *)(addr)))=(uint32)(reg)
#define read_reg(addr)  (*((volatile uint32 *)(addr)))
static int dw_i2s_action(void * base,int channel) {

	int data, rx_data_right, rx_data_left,temp;
	unsigned int i2s_base;
	i2s_base = base;
	temp = read_reg(OFFSET_I2S_IMR0 + i2s_base);
	if (SNDRV_PCM_STREAM_PLAYBACK == channel) {
		write_reg(OFFSET_I2S_TCR0 + i2s_base, 0x4);
		write_reg(OFFSET_I2S_TFCR0 + i2s_base, 0x10);
		write_reg(OFFSET_I2S_ITER + i2s_base, 0x01);
		write_reg(OFFSET_I2S_TXFFR + i2s_base, 1);
		temp &=~(1<<4);
		temp |= (1<<1);
		temp |= (1<<5);
		write_reg(OFFSET_I2S_TER0 + i2s_base, 1);

	} else {
		write_reg(OFFSET_I2S_IRER + i2s_base, 0x01);
		write_reg(OFFSET_I2S_RCR0 + i2s_base, 0x4);
		write_reg(OFFSET_I2S_RFCR0 + i2s_base, I2S_FIFO_LEN_RX);
		write_reg(OFFSET_I2S_RXFFR + i2s_base, 1);
		temp &=~(1<<0);
		temp |= (1<<1);
		temp |= (1<<5);
		write_reg(OFFSET_I2S_RER0 + i2s_base, 1);

	}
	write_reg(OFFSET_I2S_IMR0 + i2s_base, temp); //interrupt mask
}

static void codec_config(void)
{
		i2c_write_codec(0x0, 0x44);//set 8K sample
		i2c_write_codec(0x9, 0x2);
		i2c_write_codec(0x4, 0x10);
		i2c_write_codec(0x1, 0x3c);
		i2c_write_codec(0x5, 0x5);
		i2c_write_codec(0x7, 0xe6);
		i2c_write_codec(0x2, 0x14);
		i2c_write_codec(0x8, 0x38);
		i2c_write_codec(0xf, 0x1b);
		i2c_write_codec(0x10, 0x1b);
}

static int s6000_i2s_start_channel(struct s6000_i2s_dev *dev, int channel) {

	dw_i2s_action(dev->base,channel);
	return 0;
}
static void s6000_i2s_start(struct snd_pcm_substream *substream) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct s6000_i2s_dev *dev = snd_soc_dai_get_drvdata(rtd->cpu_dai);
	s6000_i2s_start_channel(dev, substream->stream);

}
static int s6000_i2s_stop_channel(struct s6000_i2s_dev *dev, int channel) {
	int temp,i;
	temp = read_reg(OFFSET_I2S_IMR0 + dev->base);
	if (SNDRV_PCM_STREAM_PLAYBACK == channel) {
		write_reg(OFFSET_I2S_TER0 + dev->base, 0);
		temp |=(1<<4);
		temp |= (1<<1);
		temp |= (1<<5);
		write_reg(OFFSET_I2S_IMR0 + dev->base,temp); //interrupt mask


	} else {
		write_reg(OFFSET_I2S_RER0 + dev->base, 0);
		temp |=(1<<0);
		temp |= (1<<1);
		temp |= (1<<5);
		write_reg(OFFSET_I2S_IMR0 + dev->base,temp); //interrupt mask

	}
	return 0;

}
static void s6000_i2s_stop(struct snd_pcm_substream *substream) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct s6000_i2s_dev *dev = snd_soc_dai_get_drvdata(rtd->cpu_dai);
	s6000_i2s_stop_channel(dev, substream->stream);
}
static int davinci_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
		int after) {
	int ret = 0;
	int playback = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		s6000_i2s_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	//	if (!after)
			s6000_i2s_stop(substream);
				break;
	}
	return 0;
}

static int davinci_i2s_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai) {
//	struct davinci_mcbsp_dev *dev = snd_soc_dai_get_drvdata(dai);
//
//	snd_soc_dai_set_dma_data(dai, substream, dev->dma_params);
	return 0;
}

static void davinci_i2s_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai) {
//	struct davinci_mcbsp_dev *dev = snd_soc_dai_get_drvdata(dai);
//	int playback = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
//	davinci_mcbsp_stop(dev, playback);
}

#define DAVINCI_I2S_RATES	SNDRV_PCM_RATE_8000_96000

static struct snd_soc_dai_ops davinci_i2s_dai_ops = { .startup =
		davinci_i2s_startup, .shutdown = davinci_i2s_shutdown, .prepare =
		davinci_i2s_prepare, .trigger = davinci_i2s_trigger, .hw_params =
		davinci_i2s_hw_params, .set_fmt = davinci_i2s_set_dai_fmt, .set_clkdiv =
		davinci_i2s_dai_set_clkdiv,

};
int  s6000_i2s_dai_probe(struct snd_soc_dai *dai) {

	struct s6000_i2s_dev *dev = snd_soc_dai_get_drvdata(dai);
	struct s6000_snd_platform_data *pdata = dai->dev->platform_data;
	dai->capture_dma_data = &dev->dma_params;
	dai->playback_dma_data = &dev->dma_params;
	dev->dma_params.trigger = davinci_i2s_trigger;
//	dev->wide = pdata->wide;
//	dev->channel_in = pdata->channel_in;
//	dev->channel_out = pdata->channel_out;
//	dev->lines_in = pdata->lines_in;
//	dev->lines_out = pdata->lines_out;
	dev->dma_params.sif_in = 0xf0901c0;
	dev->dma_params.sif_out = 0xf0901c8;
	return 0;

}
static struct snd_soc_dai_driver davinci_i2s_dai = { .probe =
		s6000_i2s_dai_probe, .playback = { .channels_min = 2, .channels_max = 2,
		.rates = DAVINCI_I2S_RATES, .formats = SNDRV_PCM_FMTBIT_S16_LE, },
		.capture = { .channels_min = 2, .channels_max = 2, .rates =
				DAVINCI_I2S_RATES, .formats = SNDRV_PCM_FMTBIT_S16_LE, }, .ops =
				&davinci_i2s_dai_ops,

};
//work queue funtion


 void capture_work_func(struct work_struct *work)
{
	uint32 count_data,i,audio_data,temp;
	uint32 buff[I2S_FIFO_LEN_RX];
	for(count_data=0;count_data<I2S_FIFO_LEN_RX;count_data++) {
			audio_data = read_reg(OFFSET_I2S_RRBR0 + g_i2s_base);
			i = read_reg(OFFSET_I2S_LRBR0 + g_i2s_base);
			buff[count_data] = audio_data;
	}
	i2s_irq_enquen(SNDRV_PCM_STREAM_CAPTURE,(u8 *)buff,count_data<<2,0);
	temp = read_reg(OFFSET_I2S_IMR0 + g_i2s_base);
	temp &=~(1<<0);
	write_reg(OFFSET_I2S_IMR0 + g_i2s_base,temp);
}
 void playback_work_func(struct work_struct *work)
{
	uint32 count_data,temp;
	uint32 buff[I2S_FIFO_LEN_TX];
	i2s_irq_enquen(SNDRV_PCM_STREAM_PLAYBACK,(u8 *)buff,I2S_FIFO_LEN_TX<<2,0);
	for(count_data=0;count_data<I2S_FIFO_LEN_TX;count_data++) {
			write_reg(OFFSET_I2S_RTHR0 + g_i2s_base,buff[count_data]);
			write_reg(OFFSET_I2S_LTHR0 + g_i2s_base,buff[count_data]);
	}
	temp = read_reg(OFFSET_I2S_IMR0 + g_i2s_base);
	temp &=~(1<<4);
	write_reg(OFFSET_I2S_IMR0 + g_i2s_base,temp);
}

static irqreturn_t davinci_i2s_irq(int irq, void *data) {
	uint32 irq_data,temp;
	struct s6000_i2s_dev *fdev = data;
	irq_data = read_reg(OFFSET_I2S_ISR0 + fdev->base);
	temp = read_reg(OFFSET_I2S_IMR0 + fdev->base);
	if ( (irq_data & 0x10)&&( !(temp&(1<<4))) ) {
		temp |= (1<<4);
		write_reg(OFFSET_I2S_IMR0 + fdev->base,temp);
		schedule_work(&tx_md->my_work);
		//playback_work_func(&tx_md->my_work);
	}
	if ( (irq_data & 0x01)&&( !(temp&(1<<0))) ) {
		temp|= (1<<0);
		write_reg(OFFSET_I2S_IMR0 +fdev->base,temp);
		schedule_work(&rx_md->my_work);
		//capture_work_func(&rx_md->my_work);
	}
		return IRQ_HANDLED;
	}

static void i2s_config(void)
{
	write_reg(OFFSET_I2S_IER + g_i2s_base, 0x01);//i2s   enable
	write_reg(OFFSET_I2S_CCR + g_i2s_base, 0x8);
	write_reg(OFFSET_I2S_CER + g_i2s_base, 0x01);
}
	static int davinci_i2s_probe(struct platform_device *pdev) {
		struct snd_platform_data *pdata = pdev->dev.platform_data;
		struct s6000_i2s_dev *dev;
		struct resource *mem, *ioarea, *res;


		if (!pdata)
			return -EINVAL;

		int ret;

		rx_md=init_data(rx_md,rx_work_queue);
		INIT_WORK(&rx_md->my_work,capture_work_func);
		//rx_wq=create_singlethread_workqueue("capture_workqueue");
		tx_md = init_data(tx_md,tx_work_queue);
		INIT_WORK(&tx_md->my_work,playback_work_func);
		//tx_wq=create_singlethread_workqueue("capture_workqueue");
		//queue_work(wq,&md->my_work);
		mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!mem) {
			printk("i2s platform get resource err\n");
			dev_err(&pdev->dev, "no mem resource?\n");
			return -ENODEV;
		}

		ioarea = request_mem_region(mem->start, resource_size(mem), pdev->name);
		if (!ioarea) {
			printk("i2s request mem region err\n");
			dev_err(&pdev->dev, "McBSP region already claimed\n");
			return -EBUSY;
		}

		dev = kzalloc(sizeof(struct s6000_i2s_dev), GFP_KERNEL);
		if (!dev) {
			printk("i2s kzalloc err \n");
			ret = -ENOMEM;
			goto err_release_region;
		}
		dev->irq = platform_get_irq(pdev, 0);
		int rc;
		rc = request_irq(dev->irq, davinci_i2s_irq,
		IRQF_DISABLED, pdev->name, dev);
		if (rc) {
			printk("request irq err \n");
			free_irq(dev->irq, dev);
		}

		dev->base = ioremap(mem->start, resource_size(mem));
		g_i2s_base = dev->base;
		rx_md->base =dev->base;
		tx_md->base = rx_md->base;
		if (!dev->base) {
			dev_err(&pdev->dev, "ioremap failed\n");
			ret = -ENOMEM;
			goto err_release_clk;
		}

		dev->dev = &pdev->dev;

		dev_set_drvdata(&pdev->dev, dev);

		ret = snd_soc_register_dai(&pdev->dev, &davinci_i2s_dai);
		if (ret != 0)
			goto err_iounmap;

		codec_config();
		i2s_config();

		return 0;

		err_iounmap: iounmap(dev->base);
		err_release_clk: clk_disable(dev->clk);
		clk_put(dev->clk);
		err_free_mem: kfree(dev);
		err_release_region:
		release_mem_region(mem->start, resource_size(mem));

		return ret;
	}

	static int davinci_i2s_remove(struct platform_device *pdev) {
		struct davinci_mcbsp_dev *dev = dev_get_drvdata(&pdev->dev);
		struct resource *mem;

		snd_soc_unregister_dai(&pdev->dev);
//	clk_disable(dev->clk);
//	clk_put(dev->clk);
//	dev->clk = NULL;
		kfree(dev);
		mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		release_mem_region(mem->start, resource_size(mem));

		return 0;
	}

	static struct platform_driver davinci_mcbsp_driver = {
			.probe =davinci_i2s_probe,
			.remove = davinci_i2s_remove,
			.driver = {
					.name =
							"s6000-i2s", .owner = THIS_MODULE,
			},
	};

static int __init davinci_i2s_init(void)
{
	return platform_driver_register(&davinci_mcbsp_driver);
}
	module_init(davinci_i2s_init);

static void __exit davinci_i2s_exit(void)
{
	platform_driver_unregister(&davinci_mcbsp_driver);
}
	module_exit(davinci_i2s_exit);

	MODULE_AUTHOR("Vladimir Barinov");
	MODULE_DESCRIPTION("TI DAVINCI I2S (McBSP) SoC Interface");
	MODULE_LICENSE("GPL");
