/*
 * ALSA PCM interface for the Stetch s6000 family
 *
 * Author:      Daniel Gloeckner, <dg@emlix.com>
 * Copyright:   (C) 2009 emlix GmbH <info@emlix.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <asm/dma.h>
//#include <variant/dmac.h>
#include "dma.h"
#include "fullhan-pcm.h"

#define S6_PCM_PREALLOCATE_SIZE (96 * 1024)
#define S6_PCM_PREALLOCATE_MAX  (2048 * 1024)


struct snd_pcm_substream *capture_substream,*play_substream;



static struct snd_pcm_hardware s6000_pcm_hardware = { .info =
		(SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
		SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_JOINT_DUPLEX), .formats =
		(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE), .rates =
		(SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_5512 |
		SNDRV_PCM_RATE_8000_192000), .rate_min = 0, .rate_max = 1562500,
		.channels_min = 2, .channels_max = 8, .buffer_bytes_max = 0x7ffffff0,
		.period_bytes_min = 16, .period_bytes_max = 0xfffff0, .periods_min = 2,
		.periods_max = 1024, /* no limit */
		.fifo_size = 0, };

struct s6000_runtime_data {
	spinlock_t lock;
	int period; /* current DMA period */
	int pos;
};


void test_dma_copy(unsigned int src, unsigned int dst) {
	Dma_SetTxrType(0, DMA_TTFC_M2M_DMAC);
	Dma_SetSrcWidth(0, 2);
	Dma_SetSrcSize(0, 0);
	Dma_SetDstWidth(0, 2);				// UART can only accept 8bits input
	Dma_SetDstSize(0, 0);		// burst size, UART has 16bytes FIFO, 1/2 thrl
	Dma_SetSrcAddress(0, src);
	Dma_SetDstAddress(0, dst);
	Dma_SetSrcIncDirection(0, DMA_DIR_INC);
	Dma_SetDstIncDirection(0, DMA_DIR_INC);
	Dma_EnableIsrBit(0, DMA_INTT_BLOCK);		// block finish ISR.

	Dma_SetTxrSize(0, 4);					// copy 1K bytes.

	Dma_EnableChan(0);

}

static void copy_finish(struct snd_pcm_substream *substream ) {
	snd_pcm_period_elapsed(substream);
}

void i2s_irq_enquen(int type, u8 *buff, u8 len,u8 reset)
{
	struct snd_pcm_substream *substream;
	struct snd_pcm_runtime *runtime;
	struct s6000_runtime_data *prtd;
	static int rx_count = 0,tx_count = 0;
	if (SNDRV_PCM_STREAM_PLAYBACK == type) {
		if(reset){
			tx_count = 0;
			return;
		}
		if (!play_substream) {
			return;
		}
		substream = play_substream;
		runtime = substream->runtime;
		prtd = runtime->private_data;
		memcpy(buff,runtime->dma_area+prtd->pos,len);
		tx_count += len;
		if (tx_count >= snd_pcm_lib_period_bytes(substream)) {
			tx_count = 0;
			copy_finish(substream);
		}
	} else {
		if(reset){
					rx_count = 0;
					return;
			}
		if(!capture_substream){
			return;
		}
		substream = capture_substream;
		runtime = substream->runtime;
		prtd = runtime->private_data;
		memcpy(runtime->dma_area+prtd->pos,buff,len);
		rx_count += len;
		if (rx_count >= snd_pcm_lib_period_bytes(substream) ) {
			rx_count = 0;
			copy_finish(substream);
		}
	}
	prtd->pos += len;
	if (prtd->pos  >= snd_pcm_lib_buffer_bytes(substream)) {
			prtd->pos = 0;
	}
}
EXPORT_SYMBOL(i2s_irq_enquen);


static irqreturn_t s6000_pcm_irq(int irq, void *data) {
//	struct snd_pcm *pcm = data;
//	struct snd_soc_pcm_runtime *runtime = pcm->private_data;
//	struct s6000_runtime_data *prtd;
//	unsigned int has_xrun;
//	int i, ret = IRQ_NONE;
//
//	for (i = 0; i < 2; ++i) {
//		struct snd_pcm_substream *substream = pcm->streams[i].substream;
//		struct s6000_pcm_dma_params *params = snd_soc_dai_get_dma_data(
//				runtime->cpu_dai, substream);
//		u32 channel;
//		unsigned int pending;
//
//		if (substream == SNDRV_PCM_STREAM_PLAYBACK)
//			channel = params->dma_s6000_runtime_dataout;
//		else
//			channel = params->dma_in;
//
//		has_xrun = params->check_xrun(runtime->cpu_dai);
//
//		if (!channel)
//			continue;
//
//		if (unlikely(has_xrun & (1 << i)) && substream->runtime
//				&& snd_pcm_running(substream)) {
//			dev_dbg(pcm->dev, "xrun\n");
//			snd_pcm_stop(substream, SNDRV_PCM_STATE_XRUN);
//			ret = IRQ_HANDLED;
//		}
//
//		pending = s6dmac_int_sources(DMA_MASK_DMAC(channel),
//				DMA_INDEX_CHNL(channel));
//
//		if (pending & 1) {
//			ret = IRQ_HANDLED;
//			if (likely(substream->runtime && snd_pcm_running(substream))) {
//				snd_pcm_period_elapsed(substream);
//				dev_dbg(pcm->dev, "period elapsed %x %x\n",
//						s6dmac_cur_src(DMA_MASK_DMAC(channel),
//								DMA_INDEX_CHNL(channel)),
//						s6dmac_cur_dst(DMA_MASK_DMAC(channel),
//								DMA_INDEX_CHNL(channel)));
//				prtd = substrcopy_finisheam->runtime->private_data;
//				spin_lock(&prtd->lock);
//				s6000_pcm_enqueue_dma(substream);
//				spin_unlock(&prtd->lock);
//			}
//		}
//
//		if (unlikely(pending & ~7)) {
//		if (pending & (1 << 3))
//		printk(KERN_WARNING
//				"s6000-pcm: DMA %x Underflow\n",
//				channel);
//		if (pending & (1 << 4))
//		printk(KERN_WARNING
//				"s6000-pcm: DMA %x Overflow\n",
//				channel);
//		if (pending & 0x1e0)
//		printk(KERN_WARNING
//				"s6000-pcm: DMA %x Master Error "
//				"(mask %x)\n",
//				channel, pending >> 5);
//
//	}
//}

//return ret;
}

static int s6000_pcm_start(struct snd_pcm_substream *substream) {

struct s6000_runtime_data *prtd = substream->runtime->private_data;
struct snd_soc_pcm_runtime *soc_runtime = substream->private_data;
struct s6000_pcm_dma_params *par;
unsigned long flags;
int srcinc;
u32 dma;
par = snd_soc_dai_get_dma_data(soc_runtime->cpu_dai, substream);

//spin_lock_irqsave(&prtd->lock, flags);

if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
	srcinc = 1;
	dma = par->dma_out;
} else {
	srcinc = 0;
	dma = par->dma_in;
}

//	s6dmac_enable_chan(DMA_MASK_DMAC(dma), DMA_INDEX_CHNL(dma),
//			   1 /* priority 1 (0 is max) */,
//			   0 /* peripheral requests w/o xfer length mode */,
//			   srcinc /* source address increment */,
//			   srcinc^1 /* destination address increment */,
//			   0 /* chunksize 0 (skip impossible on this dma) */,
//			   0 /* source skip after chunk (impossible) */,
//			   0 /* destination skip after chunk (impossible) */,
//			   4 /* 16 byte burst size */,
//			   -1 /* don't conserve bandwidth */,
//			   0 /* low watermark irq descriptor threshold */,
//			   0 /* disable hardware timestamps */,
//			   1 /* enable channel */);
//			prtd->period = 0;
//	s6000_pcm_enqueue_dma(substream);

//spin_unlock_irqrestore(&prtd->lock, flags);

return 0;
}

static int s6000_pcm_stop(struct snd_pcm_substream *substream) {
	struct s6000_runtime_data *prtd = substream->runtime->private_data;
	struct snd_soc_pcm_runtime *soc_runtime = substream->private_data;
	unsigned long flags;
	capture_substream = play_substream = 0;
//	u32 channel;

//	par = snd_soc_dai_get_dma_data(soc_runtime->cpu_dai, substream);
//
//	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
//		channel = par->dma_out;
//	else
//		channel = par->dma_in;
//
//	s6dmac_set_terminal_count(DMA_MASK_DMAC(channel),
//				  DMA_INDEX_CHNL(channel), 0);
//


return 0;
}

static int s6000_pcm_trigger(struct snd_pcm_substream *substream, int cmd) {
		struct snd_soc_pcm_runtime *soc_runtime = substream->private_data;
		struct s6000_pcm_dma_params *par;
		int ret;

		par = snd_soc_dai_get_dma_data(soc_runtime->cpu_dai, substream);

		ret = par->trigger(substream, cmd, 0);
		if (ret < 0)
			return ret;

		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			i2s_irq_enquen(substream->stream,0,0,1);
			ret = s6000_pcm_start(substream);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			ret = s6000_pcm_stop(substream);

			break;
		default:
			ret = -EINVAL;
		}
		if (ret < 0)
			return ret;

		return par->trigger(substream, cmd, 1);
}

static int s6000_pcm_prepare(struct snd_pcm_substream *substream)
{
	return 0;
}


static snd_pcm_uframes_t s6000_pcm_pointer(struct snd_pcm_substream *substream) {
struct snd_pcm_runtime *runtime = substream->runtime;
struct s6000_runtime_data *prtd = runtime->private_data;
unsigned int offset;
offset = bytes_to_frames(runtime, prtd->pos);
if (unlikely(offset >= runtime->buffer_size))
	offset = 0;
return offset;
}

static int s6000_pcm_open(struct snd_pcm_substream *substream) {
	struct snd_soc_pcm_runtime *soc_runtime = substream->private_data;
	struct s6000_pcm_dma_params *par;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s6000_runtime_data *prtd;
	int ret;
	par = snd_soc_dai_get_dma_data(soc_runtime->cpu_dai, substream);
	snd_soc_set_runtime_hwparams(substream, &s6000_pcm_hardware);
	ret = snd_pcm_hw_constraint_step(runtime, 0,
	SNDRV_PCM_HW_PARAM_PERIOD_BYTES, 16);
	if (ret < 0)
		return ret;
	ret = snd_pcm_hw_constraint_step(runtime, 0,
	SNDRV_PCM_HW_PARAM_BUFFER_BYTES, 16);
	if (ret < 0)
		return ret;
	ret = snd_pcm_hw_constraint_integer(runtime,
	SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		return ret;




	//	if (par->same_rate) {
	//		printk("s6000 pcm open 5.0\n");
	//		int rate;
	//		spin_lock(&par->lock); /* needed? */
	//		rate = par->rate;
	//		spin_unlock(&par->lock);
	//		printk("s6000 pcm open 5.1\n");
	//		if (rate != -1) {
	//			ret = snd_pcm_hw_constraint_minmax(runtime,
	//							SNDRV_PCM_HW_PARAM_RATE,
	//							rate, rate);
	//			printk("s6000 pcm open 5.2\n");
	//			if (ret < 0)
	//				return ret;
	//		}
	//	}
	prtd = kzalloc(sizeof(struct s6000_runtime_data), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&prtd->lock);
	prtd->period = 0;
	prtd->pos = 0;
	runtime->private_data = prtd;
	/*remember to judge capture or play stream*/
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_substream = substream;
	}else {
		capture_substream = substream;
	}
	//irq_emulation();
	return 0;
}

static int s6000_pcm_close(struct snd_pcm_substream *substream) {
	struct s6000_runtime_data *prtd = substream->runtime->private_data;
	unsigned long flags;
	spin_lock_irqsave(&prtd->lock, flags);
		kfree(prtd);
	spin_unlock_irqrestore(&prtd->lock, flags);
return 0;
}

static int s6000_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *hw_params) {
struct snd_soc_pcm_runtime *soc_runtime = substream->private_data;
struct s6000_pcm_dma_params *par;
int ret;
ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
if (ret < 0) {
	printk(KERN_WARNING "s6000-pcm: allocation of memory failed\n");
	return ret;
}
par = snd_soc_dai_get_dma_data(soc_runtime->cpu_dai, substream);

	if (par->same_rate) {
		spin_lock(&par->lock);
		if (par->rate == -1 ||
		    !(par->in_use & ~(1 << substream->stream))) {
			par->rate = params_rate(hw_params);
			par->in_use |= 1 << substream->stream;
		} else if (params_rate(hw_params) != par->rate) {
			snd_pcm_lib_free_pages(substream);
			par->in_use &= ~(1 << substream->stream);
			ret = -EBUSY;
		}
		spin_unlock(&par->lock);
	}
	return ret;
}

static int s6000_pcm_hw_free(struct snd_pcm_substream *substream) {
	struct snd_soc_pcm_runtime *soc_runtime = substream->private_data;
		struct s6000_pcm_dma_params *par =
			snd_soc_dai_get_dma_data(soc_runtime->cpu_dai, substream);

		spin_lock(&par->lock);
		par->in_use &= ~(1 << substream->stream);
		if (!par->in_use)
			par->rate = -1;
		spin_unlock(&par->lock);

		return snd_pcm_lib_free_pages(substream);
}
int pxa2xx_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma) {
struct snd_pcm_runtime *runtime = substream->runtime;
return dma_mmap_writecombine(substream->pcm->card->dev, vma, runtime->dma_area,
		runtime->dma_addr, runtime->dma_bytes);
}
int s6000_pcm_copy(struct snd_pcm_substream *substream, int channel,
	snd_pcm_uframes_t pos, void __user *buf, snd_pcm_uframes_t count) {


return 0;
}
static struct snd_pcm_ops s6000_pcm_ops = { .copy = s6000_pcm_copy, .open =
	s6000_pcm_open, .close = s6000_pcm_close, .ioctl = snd_pcm_lib_ioctl,
	.hw_params = s6000_pcm_hw_params, .hw_free = s6000_pcm_hw_free, .trigger =
			s6000_pcm_trigger, .prepare = s6000_pcm_prepare, .pointer =
			s6000_pcm_pointer,
//	.mmap=pxa2xx_pcm_mmap,
	};

static void s6000_pcm_free(struct snd_pcm *pcm) {
//	struct snd_soc_pcm_runtime *runtime = pcm->private_data;
//	struct s6000_pcm_dma_params *params =
//		snd_soc_dai_get_dma_data(runtime->cpu_dai, pcm->streams[0].substream);
//
//	free_irq(params->irq, pcm);
//	snd_pcm_lib_preallocate_free_for_all(pcm);
}

static u64 s6000_pcm_dmamask = DMA_BIT_MASK(32);
static int davinci_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream,
	size_t size) {
struct snd_pcm_substream *substream = pcm->streams[stream].substream;
struct snd_dma_buffer *buf = &substream->dma_buffer;

buf->dev.type = SNDRV_DMA_TYPE_DEV;
buf->dev.dev = pcm->card->dev;
buf->private_data = NULL;
buf->area = dma_alloc_writecombine(pcm->card->dev, size, &buf->addr,
GFP_KERNEL);
pr_debug("davinci_pcm: preallocate_dma_buffer: area=%p, addr=%p, "
		"size=%d\n", (void *) buf->area, (void *) buf->addr, size);

if (!buf->area)
	return -ENOMEM;

buf->bytes = size;
return 0;
}
static u64 davinci_pcm_dmamask = 0xffffffff;

static int s6000_pcm_new(struct snd_card *card, struct snd_soc_dai *dai,
	struct snd_pcm *pcm) {
struct snd_soc_pcm_runtime *runtime = pcm->private_data;
struct s6000_pcm_dma_params *params;
int res;
int ret;

#if 0
if (dai->driver->playback.channels_min) {
	ret = davinci_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK,
			pcm_hardware_playback.buffer_bytes_max);
	if (ret)
	return ret;
}

if (dai->driver->capture.channels_min) {
	ret = davinci_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE,
			pcm_hardware_capture.buffer_bytes_max);
	if (ret)
	return ret;
}
#endif
params = snd_soc_dai_get_dma_data(runtime->cpu_dai, pcm->streams[0].substream);
if (!card->dev->dma_mask)
	card->dev->dma_mask = &s6000_pcm_dmamask;
if (!card->dev->coherent_dma_mask)
	card->dev->coherent_dma_mask = DMA_BIT_MASK(32);


res = snd_pcm_lib_preallocate_pages_for_all(pcm,
SNDRV_DMA_TYPE_DEV, card->dev,
S6_PCM_PREALLOCATE_SIZE,
S6_PCM_PREALLOCATE_MAX);
if (res)
printk(KERN_WARNING "s6000-pcm: preallocation failed\n");


return 0;
}

static struct snd_soc_platform_driver s6000_soc_platform = { .ops =
	&s6000_pcm_ops, .pcm_new = s6000_pcm_new, .pcm_free = s6000_pcm_free, };

static int __devinit s6000_soc_platform_probe(struct platform_device *pdev)
{
void * dw;
int err,irq;

err = request_irq(9, s6000_pcm_irq, 0, "dw_dmac", dw);
if (err) {

	printk("pcm  dma interrput err \n");
}
return snd_soc_register_platform(&pdev->dev, &s6000_soc_platform);
}

static int __devexit s6000_soc_platform_remove(struct platform_device *pdev)
{
snd_soc_unregister_platform(&pdev->dev);
return 0;
}

static struct platform_driver s6000_pcm_driver = { .driver = { .name =
	"fh-pcm-audio", .owner = THIS_MODULE, },

.probe = s6000_soc_platform_probe, .remove = __devexit_p(
	s6000_soc_platform_remove), };

static int __init snd_s6000_pcm_init(void)
{

	return platform_driver_register(&s6000_pcm_driver);
}
module_init(snd_s6000_pcm_init);

static void __exit snd_s6000_pcm_exit(void)
{
	platform_driver_unregister(&s6000_pcm_driver);
}
module_exit(snd_s6000_pcm_exit);

MODULE_AUTHOR("Daniel Gloeckner");
MODULE_DESCRIPTION("Stretch s6000 family PCM DMA module");
MODULE_LICENSE("GPL");
