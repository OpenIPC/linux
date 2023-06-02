/*
 * arch/arm/mach-gk/dma.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
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
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/dma-mapping.h>
#include <plat/dma.h>
#include <mach/io.h>

static struct dma_s G_dma;
static spinlock_t dma_lock;
static gk_dma_req_t *dummy_descriptor = NULL;
static dma_addr_t dummy_descriptor_phys;

#ifdef CONFIG_GK_DMA_PROC
static const char dma_proc_name[] = "dma";
static struct proc_dir_entry *dma_file;

static int gk_dma_proc_read(char *buf, char **start, off_t off,
	int len, int *eof, void *data)
{
	char *p = buf;

	p += sprintf(p, "You can add something here to debug DMA \n");

	return p - buf;
}
#endif

extern void gk_usb_dma_irq(void);
static irqreturn_t gk_dma_int_handler(int irq, void *dev_id)
{
	struct dma_s				*dma;
	int					i;
	int					j;
	u32					ireg;

	dma = (struct dma_s *)dev_id;

	ireg = gk_dma_readl(DMA_INT_REG);
	for (i = 0; i < NUM_DMA_CHANNELS; i++) {
		if (ireg & (0x1 << i)) {
			dma->chan[i].status = gk_dma_readl(DMA_CHAN_STA_REG(i));
			gk_dma_writel(DMA_CHAN_STA_REG(i), 0);

			for (j = 0; j < dma->chan[i].irq_count; j++) {
				if (dma->chan[i].irq[j].enabled == 1) {
					dma->chan[i].irq[j].handler(
						dma->chan[i].irq[j].harg,
						dma->chan[i].status);
				}
			}
		}
	}
#if 0 
	if (ireg & 0x8) {
		//printk("@:%x\n", ireg);
		//printk("$\n");
		gk_usb_dma_irq();
	}
#endif
	return IRQ_HANDLED;
}

#if (DMA_SUPPORT_DMA_FIOS == 1)
static irqreturn_t gk_dma_fios_int_handler(int irq, void *dev_id)
{
	struct dma_s				*dma;
	int					i;
	u32					ireg;

	dma = (struct dma_s *)dev_id;

	ireg = gk_dma_readl(DMA_FIOS_INT_REG);
	if (ireg & DMA_INT_CHAN0) {
		dma->chan[FIO_DMA_CHAN].status =
			gk_dma_readl(DMA_FIOS_CHAN_STA_REG(FIO_DMA_CHAN));
		gk_dma_writel(DMA_FIOS_CHAN_STA_REG(FIO_DMA_CHAN), 0);

		for (i = 0; i < dma->chan[FIO_DMA_CHAN].irq_count; i++) {
			if (dma->chan[FIO_DMA_CHAN].irq[i].enabled == 1) {
				dma->chan[FIO_DMA_CHAN].irq[i].handler(
					dma->chan[FIO_DMA_CHAN].irq[i].harg,
					dma->chan[FIO_DMA_CHAN].status);
			}
		}
	}

	return IRQ_HANDLED;
}
#endif

int gk_dma_request_irq(int chan,
	gk_dma_handler handler, void *harg)
{
	int					retval = 0;
	int					i;
	unsigned long				flags;
	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		retval = -EINVAL;
		goto gk_dma_request_irq_exit_na;
	}

	if (unlikely(handler == NULL)) {
		pr_err("%s: handler is NULL!\n", __func__);
		retval = -EINVAL;
		goto gk_dma_request_irq_exit_na;
	}

	spin_lock_irqsave(&dma_lock, flags);
	if (unlikely(G_dma.chan[chan].irq_count >
		MAX_DMA_CHANNEL_IRQ_HANDLERS)) {
		pr_err("%s: chan[%d]'s irq_count[%d] > "
			"MAX_DMA_CHANNEL_IRQ_HANDLERS[%d]!\n",
			__func__, chan, G_dma.chan[chan].irq_count,
			MAX_DMA_CHANNEL_IRQ_HANDLERS);
		retval = -EINVAL;
		goto gk_dma_request_irq_exit;
	}

	for (i = 0; i < MAX_DMA_CHANNEL_IRQ_HANDLERS; i++) {
		if (G_dma.chan[chan].irq[i].handler == NULL) {
			G_dma.chan[chan].irq[i].enabled = 0;
			G_dma.chan[chan].irq[i].handler = handler;
			G_dma.chan[chan].irq[i].harg = harg;
			G_dma.chan[chan].irq_count++;
			break;
		}
	}

gk_dma_request_irq_exit:
	spin_unlock_irqrestore(&dma_lock, flags);

gk_dma_request_irq_exit_na:
	return retval;
}
EXPORT_SYMBOL(gk_dma_request_irq);

void gk_dma_free_irq(int chan, gk_dma_handler handler)
{
	int					i;
	unsigned long				flags;

	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		return;
	}

	if (unlikely(handler == NULL)) {
		pr_err("%s: handler is NULL!\n", __func__);
		return;
	}

	spin_lock_irqsave(&dma_lock, flags);
	if (unlikely(G_dma.chan[chan].irq_count >
		MAX_DMA_CHANNEL_IRQ_HANDLERS)) {
		pr_err("%s: chan[%d]'s irq_count[%d] > "
			"MAX_DMA_CHANNEL_IRQ_HANDLERS[%d]!\n",
			__func__, chan, G_dma.chan[chan].irq_count,
			MAX_DMA_CHANNEL_IRQ_HANDLERS);
		goto gk_dma_free_irq_exit;
	}

	for (i = 0; i < MAX_DMA_CHANNEL_IRQ_HANDLERS; i++) {
		if (G_dma.chan[chan].irq[i].handler == handler) {
			G_dma.chan[chan].irq[i].enabled = 0;
			G_dma.chan[chan].irq[i].handler = NULL;
			G_dma.chan[chan].irq[i].harg = NULL;
			G_dma.chan[chan].irq_count--;
			break;
		}
	}

	for (i = i + 1; i < MAX_DMA_CHANNEL_IRQ_HANDLERS; i++) {
		if (G_dma.chan[chan].irq[i].handler != NULL) {
			G_dma.chan[chan].irq[i - 1].enabled =
				G_dma.chan[chan].irq[i].enabled;
			G_dma.chan[chan].irq[i - 1].handler =
				G_dma.chan[chan].irq[i].handler;
			G_dma.chan[chan].irq[i - 1].harg =
				G_dma.chan[chan].irq[i].harg;
			G_dma.chan[chan].irq[i].handler = NULL;
			G_dma.chan[chan].irq[i].harg = NULL;
			G_dma.chan[chan].irq[i].enabled = 0;
		}
	}

gk_dma_free_irq_exit:
	spin_unlock_irqrestore(&dma_lock, flags);
}
EXPORT_SYMBOL(gk_dma_free_irq);

int gk_dma_enable_irq(int chan, gk_dma_handler handler)
{
	int					retval = 0;
	int					i;
	unsigned long				flags;

	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		retval = -EINVAL;
		goto gk_dma_enable_irq_na;
	}

	if (unlikely(handler == NULL)) {
		pr_err("%s: handler is NULL!\n", __func__);
		retval = -EINVAL;
		goto gk_dma_enable_irq_na;
	}

	spin_lock_irqsave(&dma_lock, flags);
	for (i = 0; i < MAX_DMA_CHANNEL_IRQ_HANDLERS; i++) {
		if (G_dma.chan[chan].irq[i].handler == NULL) {
			retval = -EINVAL;
			pr_err("%s: can't find 0x%x!\n",
				__func__, (u32)handler);
			break;
		}

		if (G_dma.chan[chan].irq[i].handler == handler) {
			G_dma.chan[chan].irq[i].enabled = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&dma_lock, flags);

gk_dma_enable_irq_na:
	return retval;
}
EXPORT_SYMBOL(gk_dma_enable_irq);

int gk_dma_disable_irq(int chan, gk_dma_handler handler)
{
	int					retval = 0;
	int					i;
	unsigned long				flags;

	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		retval = -EINVAL;
		goto gk_dma_disable_irq_na;
	}

	if (unlikely(handler == NULL)) {
		pr_err("%s: handler is NULL!\n", __func__);
		retval = -EINVAL;
		goto gk_dma_disable_irq_na;
	}

	spin_lock_irqsave(&dma_lock, flags);
	for (i = 0; i < MAX_DMA_CHANNEL_IRQ_HANDLERS; i++) {
		if (G_dma.chan[chan].irq[i].handler == NULL) {
			retval = -EINVAL;
			pr_err("%s: can't find 0x%x!\n",
				__func__, (u32)handler);
			break;
		}

		if (G_dma.chan[chan].irq[i].handler == handler) {
			G_dma.chan[chan].irq[i].enabled = 0;
			break;
		}
	}
	spin_unlock_irqrestore(&dma_lock, flags);

gk_dma_disable_irq_na:
	return retval;
}
EXPORT_SYMBOL(gk_dma_disable_irq);

static inline int gk_req_dma(gk_dma_req_t * req, int chan)
{
	int					retval = 0;
	u32					ctr = 0;

	if (unlikely(req->xfr_count > 0x003fffff)) {
		pr_err("%s: xfr_count[0x%x] out of range!\n",
			__func__, req->xfr_count);
		retval = -EINVAL;
		goto gk_req_dma_exit;
	}

	gk_dma_writel(DMA_CHAN_STA_REG(chan), 0);

	if (req->next == NULL) {
		gk_dma_writel(DMA_CHAN_SRC_REG(chan), req->src);

		gk_dma_writel(DMA_CHAN_DST_REG(chan), req->dst);

		ctr |= (req->attr | req->xfr_count);
		ctr &= ~DMA_CHANX_CTR_D;
		ctr |= DMA_CHANX_CTR_EN;

	} else {/* Descriptor mode */
		gk_dma_writel((u32) req, DMA_CHAN_DA_REG(chan));

		ctr |= DMA_CHANX_CTR_D;
		ctr |= DMA_CHANX_CTR_EN;
	}
	gk_dma_writel(DMA_CHAN_CTR_REG(chan), ctr);

gk_req_dma_exit:
	return retval;
}

int gk_dma_xfr(gk_dma_req_t *req, int chan)
{
	int					retval = 0;


#ifdef CHECK_DMA_CHAN_USE_FLAG
	unsigned long				flags;
#endif

	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		retval = -EINVAL;
		goto gk_dma_xfr_exit;
	}

	if (unlikely(req == NULL)) {
		pr_err("%s: req is NULL!\n", __func__);
		retval = -EINVAL;
		goto gk_dma_xfr_exit;
	}

#ifdef CHECK_DMA_CHAN_USE_FLAG
	spin_lock_irqsave(&dma_lock, flags);
	G_dma.chan[chan].use_flag = 1;
	spin_unlock_irqrestore(&dma_lock, flags);
#endif

	retval = gk_req_dma(req, chan);

gk_dma_xfr_exit:
	return retval;
}
EXPORT_SYMBOL(gk_dma_xfr);

int gk_dma_desc_xfr(dma_addr_t desc_addr, int chan)
{


	if (unlikely(desc_addr == 0)) {
		pr_err("%s: desc_addr is NULL!\n", __func__);
		return -EINVAL;
	}

	if (unlikely((desc_addr & 0x7) != 0)) {
		pr_err("%s: desc_addr isn't aligned!\n", __func__);
		return -EINVAL;
	}

	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		return -EINVAL;
	}

	if (gk_dma_readl(DMA_CHAN_CTR_REG(chan)) & DMA_CHANX_CTR_EN)
		pr_err("%s: dma (channel = %d) is still enabled!\n", __func__, chan);

	gk_dma_writel(DMA_CHAN_DA_REG(chan), desc_addr);

	gk_dma_writel(DMA_CHAN_CTR_REG(chan), DMA_CHANX_CTR_EN | DMA_CHANX_CTR_D);

	return 0;
}
EXPORT_SYMBOL(gk_dma_desc_xfr);

int gk_dma_desc_stop(int chan)
{


	if (unlikely(chan < 0 || chan >= NUM_DMA_CHANNELS)) {
		pr_err("%s: chan[%d] < NUM_DMA_CHANNELS[%d]!\n",
			__func__, chan, NUM_DMA_CHANNELS);
		return -EINVAL;
	}

	/* Disable DMA: following sequence is not mentioned at APM.*/
	if (gk_dma_readl(DMA_CHAN_CTR_REG(chan)) & DMA_CHANX_CTR_EN) {
		gk_dma_writel(DMA_CHAN_STA_REG(chan), DMA_CHANX_STA_DD);


		gk_dma_writel(DMA_CHAN_DA_REG(chan), dummy_descriptor_phys);

		gk_dma_writel(DMA_CHAN_CTR_REG(chan), 0x28000000);


		udelay(1);
	}

	return 0;
}
EXPORT_SYMBOL(gk_dma_desc_stop);

int gk_dma_read_ctrl_reg(int chan) 
{
	return gk_dma_readl(DMA_CHAN_CTR_REG(chan));//add by DONALD, for audio drv use, mask different register addr macro value.
}
EXPORT_SYMBOL(gk_dma_read_ctrl_reg);


int __init gk_init_dma(void)
{
	int					retval = 0;
	int					i;
	struct dma_s				*dma = &G_dma;
	gk_dma_req_t			*dma_desc_array;

	spin_lock_init(&dma_lock);
	memset(&G_dma, 0x0, sizeof(G_dma));

	dummy_descriptor =  dma_alloc_coherent(NULL,
			sizeof(gk_dma_req_t) * 4,
			&dummy_descriptor_phys, GFP_KERNEL);
	if (!dummy_descriptor) {
		retval = -ENOMEM;
		goto gk_init_dma_exit;
	}

	dma_desc_array = (gk_dma_req_t *)dummy_descriptor;
	dma_desc_array->attr = DMA_DESC_EOC | DMA_DESC_WM | DMA_DESC_NI |
				DMA_DESC_IE | DMA_DESC_ST | DMA_DESC_ID;
	dma_desc_array->src = 0;
	dma_desc_array->next = (gk_dma_req_t *)dummy_descriptor_phys;
	dma_desc_array->rpt = dummy_descriptor_phys + sizeof(gk_dma_req_t);
	dma_desc_array->dst = dummy_descriptor_phys + sizeof(gk_dma_req_t) * 2;
	dma_desc_array->xfr_count = 0;

	retval = request_irq(DMA_IRQ, gk_dma_int_handler,
		IRQ_TYPE_LEVEL_HIGH, "gk-dma", dma);
	if (retval) {
		pr_err("%s: request_irq %d fail %d!\n",
			__func__, DMA_IRQ, retval);
		goto gk_init_dma_exit;
	}

	for (i = 0; i < NUM_DMA_CHANNELS; i++) {
		gk_dma_writel(DMA_CHAN_STA_REG(i), 0);

		gk_dma_writel(DMA_CHAN_CTR_REG(i), 0x38000000);

	}

#if (DMA_SUPPORT_DMA_FIOS == 1)
	retval = request_irq(DMA_FIOS_IRQ, gk_dma_fios_int_handler,
		IRQ_TYPE_LEVEL_HIGH, "gk-fios-dma", dma);
	if (retval){
		pr_err("%s: request_irq %d fail %d!\n",
			__func__, DMA_FIOS_IRQ, retval);
		goto gk_init_dma_exit;
	}
	gk_dma_writel(DMA_FIOS_CHAN_STA_REG(0), 0);


	gk_dma_writel(DMA_FIOS_CHAN_CTR_REG(0), 0x38000000);

#endif

#ifdef CONFIG_GK_DMA_PROC
	dma_file = create_proc_entry(dma_proc_name, S_IRUGO | S_IWUSR,
		get_gk_proc_dir());
	if (dma_file == NULL) {
		retval = -ENOMEM;
		pr_err("%s: for %s fail!\n", __func__, dma_proc_name);
	} else {
		dma_file->read_proc = gk_dma_proc_read;
		dma_file->write_proc = NULL;
	}
#endif

gk_init_dma_exit:
	return retval;
}

