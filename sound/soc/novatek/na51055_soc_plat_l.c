/*
 * AMD ALSA SoC PCM Driver for ACP 2.x
 *
 * Copyright 2014-2015 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/sizes.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>

#include <sound/soc.h>

#include "nvt_alsa.h"
#include "dai.h"
#include "dai_int.h"
#include "dai_reg.h"

#define DBG_WRN(fmt, args...) //printk(fmt, ##args)
#define ENABLE	1
#define DISABLE	0

/*
	Currently the dtsi is not used. Todo in the future
*/
#define IOBASE_DAI	0xF0630000
#define IOIRQ_DAI	51


#define PLAYBACK_MIN_NUM_PERIODS    2
#define PLAYBACK_MAX_NUM_PERIODS    16
#define PLAYBACK_MAX_PERIOD_SIZE    16384
#define PLAYBACK_MIN_PERIOD_SIZE    1024
#define CAPTURE_MIN_NUM_PERIODS     2
#define CAPTURE_MAX_NUM_PERIODS     16
#define CAPTURE_MAX_PERIOD_SIZE     16384
#define CAPTURE_MIN_PERIOD_SIZE     1024

#define MAX_BUFFER (PLAYBACK_MAX_PERIOD_SIZE * PLAYBACK_MAX_NUM_PERIODS)
#define MIN_BUFFER MAX_BUFFER

static const struct snd_pcm_hardware nvt_pcm_hardware_playback = {
	.info 				=	SNDRV_PCM_INFO_INTERLEAVED |
							SNDRV_PCM_INFO_BLOCK_TRANSFER | SNDRV_PCM_INFO_MMAP |
							SNDRV_PCM_INFO_MMAP_VALID | SNDRV_PCM_INFO_BATCH |
							SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,

	.formats 			= SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
	.channels_min 		= 1,
	.channels_max 		= 8,
	.rates 				= SNDRV_PCM_RATE_8000_48000,
	.rate_min 			= 8000,
	.rate_max 			= 48000,
	.buffer_bytes_max 	= PLAYBACK_MAX_NUM_PERIODS * PLAYBACK_MAX_PERIOD_SIZE,
	.period_bytes_min 	= PLAYBACK_MIN_PERIOD_SIZE,
	.period_bytes_max 	= PLAYBACK_MAX_PERIOD_SIZE,
	.periods_min 		= PLAYBACK_MIN_NUM_PERIODS,
	.periods_max 		= PLAYBACK_MAX_NUM_PERIODS,
};

static const struct snd_pcm_hardware nvt_pcm_hardware_capture = {
	.info 				= 	SNDRV_PCM_INFO_INTERLEAVED |
							SNDRV_PCM_INFO_BLOCK_TRANSFER | SNDRV_PCM_INFO_MMAP |
							SNDRV_PCM_INFO_MMAP_VALID | SNDRV_PCM_INFO_BATCH |
	    					SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,

	.formats 			= SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
	.channels_min 		= 1,
	.channels_max 		= 8,
	.rates 				= SNDRV_PCM_RATE_8000_48000,
	.rate_min 			= 8000,
	.rate_max 			= 48000,
	.buffer_bytes_max 	= CAPTURE_MAX_NUM_PERIODS * CAPTURE_MAX_PERIOD_SIZE,
	.period_bytes_min 	= CAPTURE_MIN_PERIOD_SIZE,
	.period_bytes_max 	= CAPTURE_MAX_PERIOD_SIZE,
	.periods_min 		= CAPTURE_MIN_NUM_PERIODS,
	.periods_max 		= CAPTURE_MAX_NUM_PERIODS,
};

struct nvtdai_drv_data {
	struct snd_pcm_substream *play_stream;
	struct snd_pcm_substream *capture_stream;
	void __iomem *dai_mmio;
};


void nvt_set_sample_rate (UINT32 samplerate)
{
/*	struct clk *eac_clk, *pll7_clk;
	static UINT32 stored_sr = 0;

	if(stored_sr != samplerate) {

		DBG_WRN("nvt_set_sample_rate %d\n", samplerate);

		stored_sr = samplerate;

		switch (samplerate) {
		case 11025:
		case 22050:
		case 44100: {
	        pll7_clk = clk_get(NULL, "pll7");
	        if (IS_ERR(pll7_clk)) {
				printk("get pll7 failed\r\n");
			}
	        clk_set_rate(pll7_clk, 349977600);
		} break;

		case 8000:
		case 12000:
		case 16000:
		case 24000:
		case 32000:
		case 48000:
		default: {
			pll7_clk = clk_get(NULL, "pll7");
	        if (IS_ERR(pll7_clk)) {
				printk("get pll7 failed\r\n");
			}
	        clk_set_rate(pll7_clk, 344064000);

		} break;

		}
		clk_put(pll7_clk);


		eac_clk = clk_get(NULL, "f0640000.eacdac");
		if (IS_ERR(eac_clk)) {
			printk("failed to get eac-da clk\n");
		}
		clk_set_rate(eac_clk, samplerate*256);
		clk_put(eac_clk);
	}*/
}



static irqreturn_t dma_irq_handler(int irq, void *arg)
{
	T_DAI_INTSTATUS_REG     reg_status;
	struct nvtdai_drv_data 	*irq_data;
	struct device 			*dev = arg;
	//UINT32 temp = 0;

	if(dai_is_txlb_enable() == FALSE)
		return IRQ_NONE;

	reg_status.reg = DAI_GETREG(DAI_INTSTATUS_REG_OFS) & 0x40000;
	if (reg_status.reg == 0) {
		return IRQ_NONE;
	}

	DAI_SETREG(DAI_INTSTATUS_REG_OFS,  reg_status.reg);
	//printk("2");

	irq_data = dev_get_drvdata(dev);

	if (reg_status.reg) {

		//temp = dai_get_rx_config(DAI_RXCFG_ID_TIMECODE_TRIG)
		//	+ irq_data->capture_stream->runtime->period_size;
		//dai_set_txlb_config(DAI_RXCFG_ID_TIMECODE_TRIG, temp);

		snd_pcm_period_elapsed(irq_data->capture_stream);
	}

	return IRQ_HANDLED;
}

static int nvt_dma_open(struct snd_pcm_substream *substream)
{
	//u16 bank;
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *prtd = substream->private_data;
	struct snd_soc_component *component = snd_soc_rtdcom_lookup(prtd, "alsa_component_l");
	struct nvtdai_drv_data *intr_data = dev_get_drvdata(component->dev);

	struct audio_substream_data *adata =
		kzalloc(sizeof(struct audio_substream_data), GFP_KERNEL);

	DBG_WRN("nvt_dma_open\r\n");

	if (adata == NULL)
		return -ENOMEM;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		runtime->hw = nvt_pcm_hardware_playback;
	else
		runtime->hw = nvt_pcm_hardware_capture;

	ret = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		dev_err(component->dev, "set integer constraint failed\n");
		kfree(adata);
		return ret;
	}

	adata->dai_mmio = intr_data->dai_mmio;
	runtime->private_data = adata;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		intr_data->play_stream = substream;
	} else {
		intr_data->capture_stream = substream;
	}

	return 0;
}

static int nvt_dma_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	int status;
	uint64_t size;
	struct snd_dma_buffer *dma_buffer;
	struct snd_pcm_runtime *runtime;
	struct audio_substream_data *rtd;

	DBG_WRN("nvt_dma_hw_params\r\n");

	dma_buffer = &substream->dma_buffer;

	runtime = substream->runtime;
	rtd = runtime->private_data;

	if (WARN_ON(!rtd))
		return -EINVAL;

	size = params_buffer_bytes(params);
	status = snd_pcm_lib_malloc_pages(substream, size);

	return status;
}

static int nvt_dma_hw_free(struct snd_pcm_substream *substream)
{
	DBG_WRN("nvt_dma_hw_free\r\n");

	return snd_pcm_lib_free_pages(substream);
}

static snd_pcm_uframes_t nvt_dma_pointer(struct snd_pcm_substream *substream)
{
	snd_pcm_uframes_t	frames;
	unsigned long		bytes;
	struct snd_pcm_runtime *runtime = substream->runtime;

	//DBG_WRN("nvt_dma_pointer\r\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		bytes = DAI_GETREG(DAI_TX1DMACURRENT_REG_OFS) - DAI_GETREG(DAI_TX1DMASTART_REG_OFS);
	} else {
		bytes = DAI_GETREG(DAI_TXLBDMACURRENT_REG_OFS) - DAI_GETREG(DAI_TXLBDMASTART_REG_OFS);
	}

	frames = bytes_to_frames(runtime, bytes);

	if (frames >= runtime->buffer_size)
		frames -= runtime->buffer_size;

	return frames;
}

static int nvt_dma_mmap(struct snd_pcm_substream *substream,
			struct vm_area_struct *vma)
{
	DBG_WRN("nvt_dma_mmap\r\n");

	return snd_pcm_lib_default_mmap(substream, vma);
}

static int nvt_dma_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	//struct audio_substream_data *rtd = runtime->private_data;

	DBG_WRN("nvt_dma_prepare\r\n");

	DBG_WRN("rt format=%d rate=%d ch=%d frmbits=%d\r\n", (int)runtime->format, (int)runtime->rate, (int)runtime->channels, (int)runtime->frame_bits);
	DBG_WRN("rt addr=0x%08X bufsz=%d-frames prdsz=%d-frames\r\n", (int)runtime->dma_addr, (int)runtime->buffer_size, (int)runtime->period_size);

	// Set clock rate
	nvt_set_sample_rate(runtime->rate);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {

		if (runtime->format == SNDRV_PCM_FORMAT_S16_LE) {
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_PCMLEN, DAI_DRAMPCM_16);
		} else if (runtime->format == SNDRV_PCM_FORMAT_S32_LE) {
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_PCMLEN, DAI_DRAMPCM_32);
		} else if (runtime->format == SNDRV_PCM_FORMAT_U8) {
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_PCMLEN, DAI_DRAMPCM_8);
		}

		if (runtime->channels == 2) {
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,		DAI_DRAMPCM_STEREO);
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL,		DAI_CH_STEREO);
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_TOTAL_CH,		DAI_TOTCH_2CH);
		} else if (runtime->channels == 1) {
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,		DAI_DRAMPCM_MONO);
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL,		DAI_CH_MONO_LEFT);
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_TOTAL_CH,		DAI_TOTCH_2CH);
		} else if (runtime->channels == 4) {
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,		DAI_DRAMPCM_STEREO);
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL,		DAI_CH_STEREO);
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_TOTAL_CH,		DAI_TOTCH_4CH);
		} else if (runtime->channels == 6) {
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,		DAI_DRAMPCM_STEREO);
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL,		DAI_CH_STEREO);
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_TOTAL_CH,		DAI_TOTCH_6CH);
		} else if (runtime->channels == 8) {
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,		DAI_DRAMPCM_STEREO);
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL,		DAI_CH_STEREO);
			dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_TOTAL_CH,		DAI_TOTCH_8CH);
		} else {
			printk(KERN_ERR "%s:bad ch-no %d\n", __func__, runtime->channels);
			return -EINVAL;
		}

		dai_set_tx_dma_para(DAI_TXCH_TX1,	(UINT32)runtime->dma_area, frames_to_bytes(runtime, runtime->buffer_size)>>2);

	} else {

		if (runtime->format == SNDRV_PCM_FORMAT_S16_LE) {
			dai_set_txlb_config(DAI_TXLBCFG_ID_PCMLEN, DAI_DRAMPCM_16);
		} else if (runtime->format == SNDRV_PCM_FORMAT_S32_LE) {
			dai_set_txlb_config(DAI_TXLBCFG_ID_PCMLEN, DAI_DRAMPCM_32);
		} else if (runtime->format == SNDRV_PCM_FORMAT_U8) {
			dai_set_txlb_config(DAI_TXLBCFG_ID_PCMLEN, DAI_DRAMPCM_8);
		}


		if (runtime->channels == 2) {
			dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,		DAI_DRAMPCM_STEREO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,		DAI_CH_STEREO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_TOTAL_CH,	DAI_TOTCH_2CH);
		} else if (runtime->channels == 1) {
			dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,		DAI_DRAMPCM_MONO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,		DAI_CH_MONO_LEFT);
			dai_set_txlb_config(DAI_TXLBCFG_ID_TOTAL_CH,	DAI_TOTCH_2CH);
		} else if (runtime->channels == 4) {
			dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,		DAI_DRAMPCM_STEREO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,		DAI_CH_STEREO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_TOTAL_CH,	DAI_TOTCH_4CH);
		} else if (runtime->channels == 6) {
			dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,		DAI_DRAMPCM_STEREO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,		DAI_CH_STEREO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_TOTAL_CH,	DAI_TOTCH_6CH);
		} else if (runtime->channels == 8) {
			dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,		DAI_DRAMPCM_STEREO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,		DAI_CH_STEREO);
			dai_set_txlb_config(DAI_TXLBCFG_ID_TOTAL_CH,	DAI_TOTCH_8CH);
		} else {
			printk(KERN_ERR "%s:bad ch-no %d\n", __func__, runtime->channels);
			return -EINVAL;
		}

		dai_set_txlb_dma_para((UINT32)runtime->dma_area, frames_to_bytes(runtime, runtime->buffer_size)>>2);

	}
	return 0;
}

static int nvt_dma_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret;
	//u32 loops = 1000;

	struct snd_pcm_runtime *runtime = substream->runtime;
	//struct snd_soc_pcm_runtime *prtd = substream->private_data;
	struct audio_substream_data *rtd = runtime->private_data;

	DBG_WRN("nvt_dma_trigger %d\r\n", cmd);

	if (!rtd)
		return -EINVAL;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			dai_enable_tx(DAI_TXCH_TX1, ENABLE);
		} else {
			dai_enable_txlb(ENABLE);
		}
		ret = 0;
		break;

	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {

			dai_set_tx_config(0, DAI_TXCFG_ID_TIMECODE_OFS,  0);
			dai_set_tx_config(0, DAI_TXCFG_ID_TIMECODE_TRIG, runtime->period_size);
			dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_TX1TCHIT_INT);

			dai_enable_tx_dma(DAI_TXCH_TX1,		ENABLE);
			udelay(2);
			dai_enable_tx(DAI_TXCH_TX1, ENABLE);
		} else {
			//******************************************************************************
			//dai_set_txlb_config(DAI_RXCFG_ID_TIMECODE_OFS,  0);
			//dai_set_txlb_config(DAI_RXCFG_ID_TIMECODE_TRIG, runtime->period_size);
			//dai_set_config(DAI_CONFIG_ID_SET_INTEN, DAI_TXLBDMADONE_INT);
			dai_enable_txlb_dma(ENABLE);
			udelay(2);
			dai_enable_txlb(ENABLE);
		}
		ret = 0;
		break;

	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			dai_enable_tx(DAI_TXCH_TX1, DISABLE);
		} else {
			dai_enable_txlb(DISABLE);
		}
		ret = 0;
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {

			dai_enable_tx(DAI_TXCH_TX1, DISABLE);
			udelay(2);
			dai_enable_tx_dma(DAI_TXCH_TX1,		DISABLE);

			dai_set_config(DAI_CONFIG_ID_CLR_INTEN, DAI_TX1TCHIT_INT);
		} else {

			dai_enable_txlb(DISABLE);
			udelay(2);
			dai_enable_txlb_dma(DISABLE);

			//******************************************************************************
			//dai_set_config(DAI_CONFIG_ID_CLR_INTEN, DAI_TXLBDMADONE_INT);
		}
		ret = 0;
		break;

	default:
		ret = -EINVAL;

	}
	return ret;
}

static int nvt_dma_new(struct snd_soc_pcm_runtime *rtd)
{

	DBG_WRN("nvt_dma_new\r\n");

	return snd_pcm_lib_preallocate_pages_for_all(rtd->pcm,
			SNDRV_DMA_TYPE_DEV, NULL, MIN_BUFFER, MAX_BUFFER);
}

static int nvt_dma_close(struct snd_pcm_substream *substream)
{
	//u16 bank;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct audio_substream_data *rtd = runtime->private_data;
	struct snd_soc_pcm_runtime *prtd = substream->private_data;
	struct snd_soc_component *component = snd_soc_rtdcom_lookup(prtd, "alsa_component_l");
	struct nvtdai_drv_data *adata = dev_get_drvdata(component->dev);

	kfree(rtd);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		adata->play_stream = NULL;
	} else {
		adata->capture_stream = NULL;
	}

	DBG_WRN("nvt_dma_close\r\n");

	return 0;
}

#if 1
/*static const struct snd_soc_component_driver fake_component = {
	.name		= "alsa_component",
};*/

/*
static int nvt_cpudai_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	DBG_WRN("nvt_cpudai_prepare\n");

	return 0;
}

static int nvt_cpudai_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	DBG_WRN("nvt_cpudai_trigger\n");

	return 0;
}

static int nvt_cpudai_startup(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
	//struct platform_device *pdev = to_platform_device(dai->dev);

	DBG_WRN("nvt_cpudai_startup\n");
	return 0;
}

static void nvt_cpudai_shutdown(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *dai)
{
	//struct platform_device *pdev = to_platform_device(dai->dev);
	DBG_WRN("nvt_cpudai_shutdown\n");
}

static int nvt_cpudai_set_clkdiv(struct snd_soc_dai *cpu_dai,
				int div_id, int div)
{
	DBG_WRN("nvt_cpudai_set_clkdiv\n");

	return 0;
}

static int nvt_cpudai_set_sysclk(struct snd_soc_dai *dai,
				int clk_id, unsigned int freq, int dir)
{
	DBG_WRN("nvt_cpudai_set_sysclk\n");

	return 0;
}

static int nvt_cpudai_set_channel_map(struct snd_soc_dai *dai,
		unsigned int tx_num, unsigned int *tx_slot,
		unsigned int rx_num, unsigned int *rx_slot)
{
	DBG_WRN("nvt_cpudai_set_channel_map\n");

	return 0;
}

static int nvt_cpudai_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *dai)
{
	DBG_WRN("nvt_cpudai_hw_params\n");


	#if 0
	//////////////////////////////////////////////////////////////////////////
	snd_soc_dai_set_sysclk
	snd_soc_codec_set_sysclk
	snd_soc_dai_set_clkdiv
	snd_soc_dai_set_bclk_ratio


	// used in the machine driver
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	// set the codec system clock for DAC and ADC
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, 24576000, SND_SOC_CLOCK_IN);
	// set codec DAI slots, 8 channels, all channels are enabled
	ret = snd_soc_dai_set_tdm_slot(codec_dai, 0xFF, 0xFF, 8, 32);
	ret = snd_soc_dai_set_tdm_slot(cpu_dai, 0xFF, 0xFF, 8, 32);
	//////////////////////////////////////////////////////////////////////////
	#endif

	return 0;
}
*/
#if 0
static int nvt_cpudai_set_tdm_slot(struct snd_soc_dai *dai, unsigned int tx_mask, unsigned int rx_mask,
	int slots, int width)
{
	DBG_WRN("nvt_cpudai_set_tdm_slot slot-%d width-%d tm=0x%02X rm=0x%02X\n", slots, width, tx_mask, rx_mask);

	if (width == 16) {
		dai_set_i2s_config(DAI_I2SCONFIG_ID_CHANNEL_LEN, 0);
	} else if (width == 32) {
		dai_set_i2s_config(DAI_I2SCONFIG_ID_CHANNEL_LEN, 1);
	} else {
		printk(KERN_ERR "%s:bad width %d\n", __func__, width);
		return -EINVAL;
	}

	return 0;
}



static int nvt_cpudai_set_dai_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	DBG_WRN("nvt_cpudai_set_dai_fmt 0x%08X\n",fmt);


	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		dai_set_config(DAI_CONFIG_ID_EXTCODEC_EN,	ENABLE);
		dai_set_config(DAI_CONFIG_ID_RX_SRC_SEL,	DAI_RX_SRC_I2S);
		break;

	default:
		dai_set_config(DAI_CONFIG_ID_EXTCODEC_EN,	DISABLE);
		dai_set_config(DAI_CONFIG_ID_RX_SRC_SEL,	DAI_RX_SRC_EMBEDDED);
		printk(KERN_ERR "%s:bad format\n", __func__);
		return -EINVAL;
	}

	/* Set FCK Invert */
	if((fmt & SND_SOC_DAIFMT_INV_MASK) == SND_SOC_DAIFMT_NB_IF) {
		dai_set_i2s_config(DAI_I2SCONFIG_ID_CLK_INV, ENABLE);
	} else {
		dai_set_i2s_config(DAI_I2SCONFIG_ID_CLK_INV, DISABLE);
	}

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		/* cpu is master */
		dai_set_i2s_config(DAI_I2SCONFIG_ID_OPMODE, DAI_OP_MASTER);
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		/* codec is master */
		dai_set_i2s_config(DAI_I2SCONFIG_ID_OPMODE, DAI_OP_SLAVE);
		break;

	default:
		printk(KERN_ERR "%s:bad master\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int nvt_cpudai_set_bclk_ratio(struct snd_soc_dai *dai, unsigned int ratio)
{
	DBG_WRN("nvt_cpudai_set_bclk_ratio %d\n",ratio);

	if (ratio == 32) {
		dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO, DAI_I2SCLKR_256FS_32BIT);
	} else if (ratio == 64) {
		dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO, DAI_I2SCLKR_256FS_64BIT);
	} else if (ratio == 128) {
		dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO, DAI_I2SCLKR_256FS_128BIT);
	} else if (ratio == 256) {
		dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO, DAI_I2SCLKR_256FS_256BIT);
	}

	return 0;
}
#endif
/*

*/

static const struct snd_soc_dai_ops nvt_cpudai_ops = {
	//.prepare			= nvt_cpudai_prepare,
	//.startup			= nvt_cpudai_startup,
	//.shutdown			= nvt_cpudai_shutdown,
	//.trigger			= nvt_cpudai_trigger,
	//.hw_params		= nvt_cpudai_hw_params,
	//.set_clkdiv		= nvt_cpudai_set_clkdiv,
	//.set_sysclk 		= nvt_cpudai_set_sysclk,
	//.set_channel_map 	= nvt_cpudai_set_channel_map,
	//.set_tdm_slot	 	= nvt_cpudai_set_tdm_slot,
	//.set_fmt			= nvt_cpudai_set_dai_fmt,
	//.set_bclk_ratio 	= nvt_cpudai_set_bclk_ratio,
};

static struct snd_soc_dai_driver nvt_platform_cpudai[] = {
	{
		.name = "nvt,cpu_dai_l",
		.id = 0,
/*
		.playback = {
			.stream_name 	= "cpudai-play",
			.channels_min 	= 1,
			.channels_max 	= 8,
			.rates 			= SNDRV_PCM_RATE_8000_48000,
			.rate_min 		= 8000,
			.rate_max 		= 48000,
			.formats 		= SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
		},
*/
		.capture  = {
			.stream_name 	= "cpudai-loopback",
			.channels_min 	= 1,
			.channels_max 	= 8,
			.rates 			= SNDRV_PCM_RATE_8000_48000,
			.rate_min 		= 8000,
			.rate_max 		= 48000,
			.formats 		=  SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &nvt_cpudai_ops,
	},

#if 1
	// When the component of dai = 1, dai name depends on "device_name"."device_id"
	// if the compenent of dai > 1, dai name used dai_drv->name
	// To fit this logic in soc driver, add one more define(not used)
	{
		.name = "nvt-cpu-dai.1",
		.id = 1,
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates 			= SNDRV_PCM_RATE_8000_48000,
			.formats 		= SNDRV_PCM_FMTBIT_S16_LE,// | SNDRV_PCM_FMTBIT_S32_LE,
		},
	},
#endif
};

#endif



static const struct snd_pcm_ops nvt_dma_ops = {
	.open 		= nvt_dma_open,
	.close 		= nvt_dma_close,
	.ioctl 		= snd_pcm_lib_ioctl,
	.hw_params 	= nvt_dma_hw_params,
	.hw_free 	= nvt_dma_hw_free,
	.trigger 	= nvt_dma_trigger,
	.pointer 	= nvt_dma_pointer,
	.mmap 		= nvt_dma_mmap,
	.prepare 	= nvt_dma_prepare,
};

static struct snd_soc_component_driver nvt_asoc_component = {
	.name		= "alsa_component_l",
	.ops 		= &nvt_dma_ops,
	.pcm_new 	= nvt_dma_new,
};

static int nvt_audio_probe(struct platform_device *pdev)
{
	int status;
	struct nvtdai_drv_data *nvtdai_drv_data;
	//struct clk *dai_clk;//, *eac_clk;
	//struct resource *res;

	nvtdai_drv_data = devm_kzalloc(&pdev->dev, sizeof(struct nvtdai_drv_data),
					GFP_KERNEL);
	if (nvtdai_drv_data == NULL)
		return -ENOMEM;

	//res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	//nvtdai_drv_data->dai_mmio = devm_ioremap_resource(&pdev->dev, res);

	nvtdai_drv_data->dai_mmio = ioremap_nocache(IOBASE_DAI, 0x100);
	_DAI_REG_BASE_ADDR[0] = (UINT32)nvtdai_drv_data->dai_mmio;

	//dai_clk = clk_get(NULL, "f0630000.dai");
	//if (IS_ERR(dai_clk)) {
	//	printk("failed to get dai clk\n");
	//}
	// reset controller
	//clk_prepare(dai_clk);
	//clk_put(dai_clk);

	nvt_set_sample_rate(48000);

	nvtdai_drv_data->play_stream = NULL;
	nvtdai_drv_data->capture_stream = NULL;

	//res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	//if (!res) {
	//	dev_err(&pdev->dev, "IORESOURCE_IRQ FAILED\n");
	//	return -ENODEV;
	//}
	//dev_err(&pdev->dev, "IORESOURCE_IRQ FAILED %d\n",res->start);

	status = devm_request_irq(&pdev->dev, IOIRQ_DAI, dma_irq_handler,
					IRQF_SHARED | IRQF_TRIGGER_HIGH, "DAI_IRQ", &pdev->dev);
	if (status) {
		dev_err(&pdev->dev, "DAI IRQ request failed\n");
		return status;
	}

	dev_set_drvdata(&pdev->dev, nvtdai_drv_data);

	/* Initialize the ACP */
	//acp_init(nvtdai_drv_data->dai_mmio);

	/*status = snd_soc_register_platform(&pdev->dev, &nvt_asoc_platform);
	if (status != 0) {
		dev_err(&pdev->dev, "Fail to register ALSA platform device\n");
		return status;
	}*/

	status = snd_soc_register_component(&pdev->dev, &nvt_asoc_component, nvt_platform_cpudai, ARRAY_SIZE(nvt_platform_cpudai));
	if (status)
	{
		dev_err(&pdev->dev, "snd_soc_register_component failed(%d)!\n", status);
		return status;
	}

	dai_open(NULL);
	dai_enable_dai(ENABLE);

	/*
		Not used first
	*/
	//pm_runtime_set_autosuspend_delay(&pdev->dev, 10000);
	//pm_runtime_use_autosuspend(&pdev->dev);
	//pm_runtime_enable(&pdev->dev);

	DBG_WRN("nvt_audio_probe\r\n");

	return status;
}

static int nvt_audio_remove(struct platform_device *pdev)
{
	//struct nvtdai_drv_data *adata = dev_get_drvdata(&pdev->dev);

	//acp_deinit(adata->dai_mmio);
	snd_soc_unregister_component(&pdev->dev);
	//pm_runtime_disable(&pdev->dev);

	//dai_enable_dai(DISABLE);
	//dai_close();

	DBG_WRN("nvt_audio_remove\r\n");

	return 0;
}

static int nvt_pcm_resume(struct device *dev)
{
	/*
	u16 bank;
	struct nvtdai_drv_data *adata = dev_get_drvdata(dev);

	acp_init(adata->dai_mmio);

	if (adata->play_stream && adata->play_stream->runtime) {
		for (bank = 1; bank <= 4; bank++)
			acp_set_sram_bank_state(adata->dai_mmio, bank,
						true);
		config_acp_dma(adata->dai_mmio,
				adata->play_stream->runtime->private_data);
	}
	if (adata->capture_stream && adata->capture_stream->runtime) {
		for (bank = 5; bank <= 8; bank++)
			acp_set_sram_bank_state(adata->dai_mmio, bank,
						true);
		config_acp_dma(adata->dai_mmio,
				adata->capture_stream->runtime->private_data);
	}
	acp_reg_write(1, adata->dai_mmio, mmACP_EXTERNAL_INTR_ENB);
	*/

	DBG_WRN("nvt_pcm_resume\r\n");

	return 0;
}

static int nvt_pcm_runtime_suspend(struct device *dev)
{
	//struct nvtdai_drv_data *adata = dev_get_drvdata(dev);

	//acp_deinit(adata->dai_mmio);
	//acp_reg_write(0, adata->dai_mmio, mmACP_EXTERNAL_INTR_ENB);

	DBG_WRN("nvt_pcm_runtime_suspend\r\n");
	return 0;
}

static int nvt_pcm_runtime_resume(struct device *dev)
{
	//struct nvtdai_drv_data *adata = dev_get_drvdata(dev);

	//acp_init(adata->dai_mmio);
	//acp_reg_write(1, adata->dai_mmio, mmACP_EXTERNAL_INTR_ENB);

	DBG_WRN("nvt_pcm_runtime_resume\r\n");
	return 0;
}

static const struct dev_pm_ops nvt_pm_ops = {
	.resume = nvt_pcm_resume,
	.runtime_suspend = nvt_pcm_runtime_suspend,
	.runtime_resume = nvt_pcm_runtime_resume,
};


//#ifdef CONFIG_OF
//static const struct of_device_id of_nvtplat_match[] = {
//	{
//		.compatible = "nvt,alsa_plat"
//	},
//	{ },
//};
//MODULE_DEVICE_TABLE(of, of_nvtplat_match);
//#endif

static struct platform_driver nvt_dma_driver = {
	.probe = nvt_audio_probe,
	.remove = nvt_audio_remove,
	.driver = {
		.name = "nvt,alsa_plat_l",
		.pm = &nvt_pm_ops,
//#ifdef CONFIG_OF
//		.of_match_table = of_match_ptr(of_nvtplat_match),
//#endif
	},
};

static struct platform_device *nvt_pdev;


int __init nvt_alsapcm_module_init(void)
{
	int iRet;
	struct platform_device *pdev;

	DBG_WRN("nvt_alsapcm_module_init\r\n");

	iRet = platform_driver_register(&nvt_dma_driver);
	if (iRet) {
		printk("nvt_alsapcm_module_init platform_driver_register error\n");
	}

	pdev = platform_device_alloc("nvt,alsa_plat_l",-1);
	if (!pdev)
	{
		printk("platform_device_alloc failed!!\n");
	}

	iRet = platform_device_add(pdev);
	if (iRet)
	{
		platform_device_put(pdev);
		printk( "platform_device_add failed(%d)! \n", iRet);
	}
	nvt_pdev = pdev;

	return 0;
}

void __exit nvt_alsapcm_module_exit(void)
{
	DBG_WRN("nvt_alsapcm_module_exit\r\n");

	platform_device_unregister(nvt_pdev);
	platform_driver_unregister(&nvt_dma_driver);
}

module_init(nvt_alsapcm_module_init);
module_exit(nvt_alsapcm_module_exit);


//module_platform_driver(nvt_dma_driver);

MODULE_AUTHOR("klins_chen@novatek.com.tw");
MODULE_DESCRIPTION("NOVATEK Audio PCM Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:nvt-dma-audio");
