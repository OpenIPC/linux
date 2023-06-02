/*
 * GM DMA memory operation (MEM_TO_MEM)
 *
 * Copyright (C) 2012 Faraday Technology Corp.
 *
 * Author : Shuao-kai Li <easonli@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */
#include <linux/dmaengine.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>
#include <linux/hardirq.h>
#include <linux/export.h>
#include <mach/ftapbb020.h>
#include <mach/ftdmac020.h>
#include <mach/ftdmac030.h>
#include <mach/dma_gm.h>

struct dma_gm {
	struct dma_chan	*chan;
	struct dma_async_tx_descriptor *desc;
//	struct dma_slave_config slave; /* It's useless now */
	enum dma_status dma_st;
	dma_cap_mask_t mask;
	dma_cookie_t cookie;
	wait_queue_head_t dma_wait_queue;
};

#define err(fmt, args...) printk(KERN_ERR "%s: " fmt, __func__, ##args)

static void dma_callback(void *param)
{
	struct dma_gm *gm = (struct dma_gm *)param;
	gm->dma_st = DMA_SUCCESS;
#if 1/*don't use interruptible mode*/
    wake_up(&gm->dma_wait_queue);
#else    
	wake_up_interruptible(&gm->dma_wait_queue);
#endif
}

static void dma_wait(struct dma_gm *gm)
{
	int status;
#if 1/*don't use interruptible mode*/
    status = wait_event_timeout(gm->dma_wait_queue,
                            gm->dma_st == DMA_SUCCESS,
                            60 * HZ);
#else
	status = wait_event_interruptible_timeout(gm->dma_wait_queue,
						gm->dma_st == DMA_SUCCESS,
						60 * HZ);
#endif
	if (status == 0) {
		err("Timeout in DMA\n");
	}
}

int dma_memcpy(enum dma_kind style, dma_addr_t dest, dma_addr_t src, size_t len)
{
	struct dma_gm gm_chan;
	int ret = 0, flag = 0;

	if (in_interrupt())
		panic("Fatal exception in interrupt");

	memset(&gm_chan, 0, sizeof(gm_chan));
	init_waitqueue_head(&gm_chan.dma_wait_queue);
	dma_cap_zero(gm_chan.mask);
	dma_cap_set(DMA_MEMCPY, gm_chan.mask);

	switch (style) {
#if defined(CONFIG_FTAPBB020)
		case APB_DMA:
		{
			struct ftapbb020_dma_slave slave;
			memset(&slave, 0, sizeof(slave));
			slave.id = 0;
			slave.common.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			slave.common.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			gm_chan.chan = dma_request_channel(gm_chan.mask, ftapbb020_chan_filter, (void *)&slave);
			if (!gm_chan.chan) {
				err("dma_chan is NULL\n");
				return -ENXIO;
			}
		}
		break;
#endif
#if defined(CONFIG_FTDMAC020)
		case AHB_DMA:
		{
			struct ftdmac020_dma_slave slave;
			memset(&slave, 0, sizeof(slave));
			slave.id = 0;
			slave.handshake = -1;

			/* Use BUS 1 for shared cpu bus loading */
			slave.src_sel = 1;
			slave.dst_sel = 1;
			gm_chan.chan = dma_request_channel(gm_chan.mask, ftdmac020_chan_filter, (void *)&slave);
			if (!gm_chan.chan) {
				err("dma_chan is NULL\n");
				return -ENXIO;
			}
		}
		break;
#endif
#if defined(CONFIG_FTDMAC030)
		case AXI_DMA:
		{
			struct ftdmac030_dma_slave slave;
			memset(&slave, 0, sizeof(slave));
			slave.handshake = -1;

			gm_chan.chan = dma_request_channel(gm_chan.mask, ftdmac030_chan_filter, (void *)&slave);
			if (!gm_chan.chan) {
				err("dma_chan is NULL\n");
				return -ENXIO;
			}
		}
		break;
#endif
		default:
			err("No such DMA bus\n");
			return -ENXIO;
		break;
	}

	/* filter fn has already set dma_slave_config. */
	//dmaengine_slave_config(gm_chan.chan, &gm_chan.slave);

	flag = DMA_PREP_INTERRUPT | DMA_CTRL_ACK | DMA_COMPL_SKIP_SRC_UNMAP | DMA_COMPL_SKIP_DEST_UNMAP;
	gm_chan.desc = dmaengine_prep_dma_memcpy(gm_chan.chan, dest, src, len, flag);
	if (!gm_chan.desc) {
		err("Set DMA failed!!\n");
		ret = -EIO;
		goto fail;
	}
	gm_chan.cookie = dmaengine_submit(gm_chan.desc);
	if (dma_submit_error(gm_chan.cookie)) {
		err("DMA submit failed\n");
		dmaengine_terminate_all(gm_chan.chan);
		ret = -EIO;
		goto fail;
	}
	gm_chan.desc->callback = dma_callback;
	gm_chan.desc->callback_param = &gm_chan;
	gm_chan.dma_st = DMA_IN_PROGRESS;
	dma_async_issue_pending(gm_chan.chan);
	dma_wait(&gm_chan);
fail:
	dma_release_channel(gm_chan.chan);
	return ret;
}
EXPORT_SYMBOL(dma_memcpy);

int dma_memset(enum dma_kind style, dma_addr_t dest, int value, size_t len)
{
	struct dma_gm gm_chan;
	int ret = 0, flag = 0;

	if (in_interrupt())
		panic("Fatal exception in interrupt");

	memset(&gm_chan, 0, sizeof(gm_chan));
	init_waitqueue_head(&gm_chan.dma_wait_queue);
	dma_cap_zero(gm_chan.mask);
	dma_cap_set(DMA_MEMSET, gm_chan.mask);

	switch (style) {
#if defined(CONFIG_FTAPBB020)
		case APB_DMA:
		{
			struct ftapbb020_dma_slave slave;
			memset(&slave, 0, sizeof(slave));
			slave.id = 0;
			slave.common.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			slave.common.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
			gm_chan.chan = dma_request_channel(gm_chan.mask, ftapbb020_chan_filter, (void *)&slave);
			if (!gm_chan.chan) {
				err("dma_chan is NULL\n");
				return -ENXIO;
			}
		}
		break;
#endif
#if defined(CONFIG_FTDMAC020)
		case AHB_DMA:
		{
			struct ftdmac020_dma_slave slave;
			memset(&slave, 0, sizeof(slave));
			slave.id = 0;
			slave.handshake = -1;

			/* Use BUS 1 for shared cpu bus loading */
			slave.src_sel = 1;
			slave.dst_sel = 1;
			gm_chan.chan = dma_request_channel(gm_chan.mask, ftdmac020_chan_filter, (void *)&slave);
			if (!gm_chan.chan) {
				err("dma_chan is NULL\n");
				return -ENXIO;
			}
		}
		break;
#endif
#if defined(CONFIG_FTDMAC030)
		case AXI_DMA:
		{
			struct ftdmac030_dma_slave slave;
			memset(&slave, 0, sizeof(slave));
			slave.handshake = -1;

			gm_chan.chan = dma_request_channel(gm_chan.mask, ftdmac030_chan_filter, (void *)&slave);
			if (!gm_chan.chan) {
				err("dma_chan is NULL\n");
				return -ENXIO;
			}
		}
		break;
#endif
		default:
			err("No such DMA bus\n");
			return -ENXIO;
		break;
	}

	/* filter fn has already set dma_slave_config. */
	//dmaengine_slave_config(gm_chan.chan, &gm_chan.slave);

	flag = DMA_PREP_INTERRUPT | DMA_CTRL_ACK | DMA_COMPL_SKIP_SRC_UNMAP | DMA_COMPL_SKIP_DEST_UNMAP;
	gm_chan.desc = dmaengine_prep_dma_memset(gm_chan.chan, dest, value, len, flag);
	if (!gm_chan.desc) {
		err("Set DMA failed!!\n");
		ret = -EIO;
		goto fail;
	}

	gm_chan.cookie = dmaengine_submit(gm_chan.desc);
	if (dma_submit_error(gm_chan.cookie)) {
		err("DMA submit failed\n");
		dmaengine_terminate_all(gm_chan.chan);
		ret = -EIO;
		goto fail;
	}

	gm_chan.desc->callback = dma_callback;
	gm_chan.desc->callback_param = &gm_chan;
	gm_chan.dma_st = DMA_IN_PROGRESS;
	dma_async_issue_pending(gm_chan.chan);
	dma_wait(&gm_chan);
fail:
	dma_release_channel(gm_chan.chan);
	return ret;
}
EXPORT_SYMBOL(dma_memset);
