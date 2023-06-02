/*
 * FOTG210 UDC Driver supports Bulk transfer so far
 *
 * Copyright (C) 2013 Faraday Technology Corporation
 *
 * Author : Yuan-Hsin Chen <yhchen@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of.h>

//#include <nvt-sys.h>
//#include "top.h"
#include "fotg200.h"
#include <plat/efuse_protected.h>

#define	DRIVER_DESC	"NOVATEK iVot USB Device FOTG200 Controller Driver"
#define	DRIVER_VERSION	"1.00.032"

static const char udc_name[] = "nvt,fotg200_udc";
//static const char * const fotg210_ep_name[] = {
//	"ep0", "ep1", "ep2", "ep3", "ep4"};

static const struct {
	const char *name;
	const struct usb_ep_caps caps;
} ep_info[] = {
#define EP_INFO(_name, _caps) \
	{ \
		.name = _name, \
		.caps = _caps, \
	}

	EP_INFO("ep0",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_CONTROL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep1",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep2",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep3",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep4",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep5",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),

#undef EP_INFO
};

unsigned int outslice = 0;
module_param_named(outslice, outslice, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(outslice, "Fot class(CDC-NCM) out size is unknown, the use of short packet to terminate out");

static unsigned debug_on = 0;
module_param(debug_on, uint, S_IRUGO);
MODULE_PARM_DESC(debug_on, "Whether to enable debug message. Default = 0");

#define numsg		pr_info
#define itfnumsg	pr_info
#define devnumsg	pr_info
#define ep0numsg	pr_info


static void fotg210_disable_fifo_int(struct fotg210_ep *ep)
{
	u32 value;

	value = ioread32(ep->fotg210->reg + FOTG210_DMISGR1);
	if (ep->dir_in)
		value |= DMISGR1_MF_IN_INT(ep->epnum - 1);
	else
		value |= DMISGR1_MF_OUTSPK_INT(ep->epnum - 1);
	iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR1);
}

static void fotg210_enable_fifo_int(struct fotg210_ep *ep)
{
	u32 value = ioread32(ep->fotg210->reg + FOTG210_DMISGR1);

	if (ep->dir_in)
		value &= ~DMISGR1_MF_IN_INT(ep->epnum - 1);
	else
		value &= ~DMISGR1_MF_OUTSPK_INT(ep->epnum - 1);
	iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR1);

	if (debug_on)
		numsg("fotg210_enable_fifo_int epnum=%d reg=0x%08X\n",ep->epnum,value);
}

static void fotg210_set_cxdone(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DCFESR);

	value |= DCFESR_CX_DONE;
	iowrite32(value, fotg210->reg + FOTG210_DCFESR);
}

static void fotg210_done(struct fotg210_ep *ep, struct fotg210_request *req,
			int status)
{
	if (debug_on)
		numsg("fotg210_done 1\n");

	list_del_init(&req->queue);

	/* don't modify queue heads during completion callback */
	if (ep->fotg210->gadget.speed == USB_SPEED_UNKNOWN)
		req->req.status = -ESHUTDOWN;
	else
		req->req.status = status;

	if(req->req.complete != NULL) {
		spin_unlock(&ep->fotg210->lock);
		req->req.complete(&ep->ep, &req->req);
		spin_lock(&ep->fotg210->lock);
	}

	if (ep->epnum) {
		if (list_empty(&ep->queue))
			fotg210_disable_fifo_int(ep);
	} else {
		fotg210_set_cxdone(ep->fotg210);
	}

}

static void fotg210_fifo_ep_mapping(struct fotg210_ep *ep, u32 epnum,
				u32 dir_in)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;

	/* Driver should map an ep to a fifo and then map the fifo
	 * to the ep. What a brain-damaged design!
	 */

	/* map a fifo to an ep */
	val = ioread32(fotg210->reg + FOTG210_EPMAP(epnum-1));
	val &= ~ EPMAP_FIFONOMSK(epnum, dir_in);
	val |= EPMAP_FIFONO(epnum, dir_in);
	if(!dir_in)
		val |= EPMAP_DIROUT;
	iowrite32(val, fotg210->reg + FOTG210_EPMAP(epnum-1));

	/* map the ep to the fifo */
	val = ioread32(fotg210->reg + FOTG210_FIFOMAP(epnum-1));
	//val &= ~ FIFOMAP_EPNOMSK(epnum);
	//val |= FIFOMAP_EPNO(epnum);
	val |= FIFOCF_FIFO_EN(epnum - 1);
	iowrite32(val, fotg210->reg + FOTG210_FIFOMAP(epnum-1));
}

static void fotg210_set_fifo_dir(struct fotg210_ep *ep, u32 epnum, u32 dir_in)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;

	val = ioread32(fotg210->reg + FOTG210_FIFOMAP(epnum-1));
	val |= (dir_in ? FIFOMAP_DIRIN(epnum - 1) : FIFOMAP_DIROUT(epnum - 1));
#if 0
	if((ep->type == USB_ENDPOINT_XFER_BULK))
		val |= FIFOMAP_DOUBLEBLK(epnum - 1);
	else {
		val |= FIFOMAP_SINGLEBLK(epnum - 1);
		if(ep->ep.maxpacket > 512) {
			val |= FIFOCF_BLKSZ_1024(epnum - 1);
		}
	}
#else
	val |= FIFOMAP_SINGLEBLK(epnum - 1);
	if(ep->ep.maxpacket > 512) {
		val |= FIFOCF_BLKSZ_1024(epnum - 1);
	}
#endif
	iowrite32(val, fotg210->reg + FOTG210_FIFOMAP(epnum-1));
}

static void fotg210_set_tfrtype(struct fotg210_ep *ep, u32 epnum, u32 type)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;

	val = ioread32(fotg210->reg + FOTG210_FIFOMAP(epnum-1));
	val |= FIFOCF_TYPE(type, epnum - 1);
	iowrite32(val, fotg210->reg + FOTG210_FIFOMAP(epnum-1));
}

static void fotg210_set_mps(struct fotg210_ep *ep, u32 epnum, u32 mps,
				u32 dir_in)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;
	u32 offset = dir_in ? FOTG210_INEPMPSR(epnum) :
				FOTG210_OUTEPMPSR(epnum);

	val = ioread32(fotg210->reg + offset);
	val &= ~INOUTEPMPSR_MPS(0x7FF);
	val |= INOUTEPMPSR_MPS(mps);
	iowrite32(val, fotg210->reg + offset);

	// fixed HBW = 1
	val &= ~(0x3<<13);
	if(ep->type == USB_ENDPOINT_XFER_ISOC) {
		val |= (0x1<<13);
	}
	iowrite32(val, fotg210->reg + offset);

}

static int fotg210_config_ep(struct fotg210_ep *ep,
		     const struct usb_endpoint_descriptor *desc)
{
	struct fotg210_udc *fotg210 = ep->fotg210;

	fotg210_set_fifo_dir(ep, ep->epnum, ep->dir_in);
	fotg210_set_tfrtype(ep, ep->epnum, ep->type);
	fotg210_set_mps(ep, ep->epnum, ep->ep.maxpacket, ep->dir_in);
	fotg210_fifo_ep_mapping(ep, ep->epnum, ep->dir_in);

	fotg210->ep[ep->epnum] = ep;

	return 0;
}

static int fotg210_ep_enable(struct usb_ep *_ep,
			  const struct usb_endpoint_descriptor *desc)
{
	struct fotg210_ep *ep;


	ep = container_of(_ep, struct fotg210_ep, ep);

	ep->desc = desc;
	ep->epnum = usb_endpoint_num(desc);
	ep->type = usb_endpoint_type(desc);
	ep->dir_in = usb_endpoint_dir_in(desc);
	ep->ep.maxpacket = usb_endpoint_maxp(desc);

	if (debug_on) {
		itfnumsg("fotg210_ep_enable\n");
		itfnumsg("ep-desc len=0x%X type=0x%X epaddr=0x%X attr=0x%X MaxPkt=0x%X intval=0x%X\n",desc->bLength,desc->bDescriptorType
			,desc->bEndpointAddress,desc->bmAttributes,desc->wMaxPacketSize,desc->bInterval);
	}

	return fotg210_config_ep(ep, desc);
}

static void fotg210_reset_tseq(struct fotg210_udc *fotg210, u8 epnum)
{
	struct fotg210_ep *ep = fotg210->ep[epnum];
	u32 value;
	void __iomem *reg;

	reg = (ep->dir_in) ?
		fotg210->reg + FOTG210_INEPMPSR(epnum) :
		fotg210->reg + FOTG210_OUTEPMPSR(epnum);

	/* Note: Driver needs to set and clear INOUTEPMPSR_RESET_TSEQ
	 *	 bit. Controller wouldn't clear this bit. WTF!!!
	 */

	value = ioread32(reg);
	value |= INOUTEPMPSR_RESET_TSEQ;
	iowrite32(value, reg);

	value = ioread32(reg);
	value &= ~INOUTEPMPSR_RESET_TSEQ;
	iowrite32(value, reg);
}

static int fotg210_ep_release(struct fotg210_ep *ep)
{
	if (!ep->epnum)
		return 0;
	ep->epnum = 0;
	ep->stall = 0;
	ep->wedged = 0;

	fotg210_reset_tseq(ep->fotg210, ep->epnum);

	return 0;
}

static int fotg210_ep_disable(struct usb_ep *_ep)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	unsigned long flags;

	BUG_ON(!_ep);

	if (debug_on)
		itfnumsg("fotg210_ep_disable\n");

	ep = container_of(_ep, struct fotg210_ep, ep);

	while (!list_empty(&ep->queue)) {
		req = list_entry(ep->queue.next,
			struct fotg210_request, queue);
		spin_lock_irqsave(&ep->fotg210->lock, flags);
		fotg210_done(ep, req, -ECONNRESET);
		spin_unlock_irqrestore(&ep->fotg210->lock, flags);
	}

	return fotg210_ep_release(ep);
}

static struct usb_request *fotg210_ep_alloc_request(struct usb_ep *_ep,
						gfp_t gfp_flags)
{
	struct fotg210_request *req;

	if (debug_on)
		itfnumsg("fotg210_ep_alloc_request\n");

	req = kzalloc(sizeof(struct fotg210_request), gfp_flags);
	if (!req)
		return NULL;

	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

static void fotg210_ep_free_request(struct usb_ep *_ep,
					struct usb_request *_req)
{
	struct fotg210_request *req;

	if (debug_on)
		itfnumsg("fotg210_ep_free_request\n");

	req = container_of(_req, struct fotg210_request, req);
	kfree(req);
}

static void fotg210_enable_dma(struct fotg210_ep *ep,
			      dma_addr_t d, u32 len)
{
	u32 value;
	struct fotg210_udc *fotg210 = ep->fotg210;

	if(!ep->epnum)
		pr_err("ep0 has no dma. shall not enter fotg210_enable_dma!!\n");

	if(ep->epnum < 3) {
		do {
			value = ioread32(fotg210->reg + FOTG210_DMACPSR1);
		}while(value & DMACPSR1_DMA_START);

		/* set transfer length and direction */
		value = ioread32(fotg210->reg + FOTG210_DMACPSR1);
		value &= ~(DMACPSR1_DMA_LEN(0x7FFFFF) | DMACPSR1_DMA_TYPE(1));
		value |= DMACPSR1_DMA_LEN(len) | DMACPSR1_DMA_TYPE(ep->dir_in);
		iowrite32(value, fotg210->reg + FOTG210_DMACPSR1);

		/* set device DMA target FIFO number */
		iowrite32(((ep->epnum - 1)<<1), fotg210->reg + FOTG210_DMATFNR);

		/* set DMA memory address */
		iowrite32(d, fotg210->reg + FOTG210_DMACPSR2);

		/* enable MDMA_EROR and MDMA_CMPLT interrupt */
		value = ioread32(fotg210->reg + FOTG210_DMISGR2);
		//value &= ~(DMISGR2_MDMA_CMPLT | DMISGR2_MDMA_ERROR);
		value |= (DMISGR2_MDMA_CMPLT | DMISGR2_MDMA_ERROR);
		iowrite32(value, fotg210->reg + FOTG210_DMISGR2);

		/* start DMA */
		value = ioread32(fotg210->reg + FOTG210_DMACPSR1);
		value |= DMACPSR1_DMA_START;
		iowrite32(value, fotg210->reg + FOTG210_DMACPSR1);

	} else if (ep->epnum < 5) {

		do {
			value = ioread32(fotg210->reg + (FOTG210_DMACPSR1_EP3+((ep->epnum-3)<<4)));
		}while(value & DMACPSR1_DMA_START);

		/* set transfer length and direction */
		value = ioread32(fotg210->reg + (FOTG210_DMACPSR1_EP3+((ep->epnum-3)<<4)));
		value &= ~(DMACPSR1_DMA_LEN(0x7FFFFF) | DMACPSR1_DMA_TYPE(1));
		value |= DMACPSR1_DMA_LEN(len) | DMACPSR1_DMA_TYPE(ep->dir_in);
		iowrite32(value, fotg210->reg + (FOTG210_DMACPSR1_EP3+((ep->epnum-3)<<4)));

		/* set DMA memory address */
		iowrite32(d, fotg210->reg + (FOTG210_DMACPSR2_EP3+((ep->epnum-3)<<4)));

		/* enable MDMA_EROR and MDMA_CMPLT interrupt */
		value = ioread32(fotg210->reg + FOTG210_DMISGR2);
		//value |= (DMISGR2_MDMA_CMPLT | DMISGR2_MDMA_ERROR);
		value |= (0xF<<9);
		iowrite32(value, fotg210->reg + FOTG210_DMISGR2);

		/* start DMA */
		value = ioread32(fotg210->reg + (FOTG210_DMACPSR1_EP3+((ep->epnum-3)<<4)));
		value |= DMACPSR1_DMA_START;
		iowrite32(value, fotg210->reg + (FOTG210_DMACPSR1_EP3+((ep->epnum-3)<<4)));
	} else {
		pr_err("err ep number %d!!\n", ep->epnum);
	}

}

static void fotg210_disable_dma(struct fotg210_ep *ep)
{
	//iowrite32(DMATFNR_DISDMA, ep->fotg210->reg + FOTG210_DMATFNR);
}

static void fotg210_wait_dma_done(struct fotg210_ep *ep)
{
	u32 value,completebit;
	unsigned long timeout = 0;

	if (ep->epnum < 3)
		completebit = DISGR2_DMA_CMPLT;
	else
		completebit = (0x1 << (9+((ep->epnum-3)<<1)));

	timeout = jiffies + msecs_to_jiffies(500);
	do {
		value = ioread32(ep->fotg210->reg + FOTG210_DISGR2);
		if ((value & DISGR2_USBRST_INT) ||
		    (value & DISGR2_DMA_ERROR))
			goto dma_reset;

		if (time_after(jiffies, timeout)) {
			//pr_err("%s timeout\n", __func__);
			goto dma_reset;
		}
		cpu_relax();

	} while (!(value & completebit));

	value = 0;
	value |= completebit;
	iowrite32(value, ep->fotg210->reg + FOTG210_DISGR2);

	if ((ep->epnum)&&(ep->dir_in)) {
		do {
			value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
		} while (!(value & DCFESR_FIFO_EMPTY(ep->epnum - 1)));
	}

	return;

dma_reset:
	value = ioread32(ep->fotg210->reg + 0x1C4);
	value |= DMACPSR1_DMA_ABORT;
	iowrite32(value, ep->fotg210->reg + 0x1C4);

	//pr_err("fotg210_wait_dma_done dma_abort\n");

	/* reset fifo */
	if (ep->epnum) {
		value = ioread32(ep->fotg210->reg +
				FOTG210_FIBCR(ep->epnum - 1));
		value = ((ep->epnum - 1)&0x1) ? value|(FIBCR_FFRST<<16) : value|FIBCR_FFRST;
		iowrite32(value, ep->fotg210->reg +
				FOTG210_FIBCR(ep->epnum - 1));
	} else {
		value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
		value |= DCFESR_CX_CLR;
		iowrite32(value, ep->fotg210->reg + FOTG210_DCFESR);
	}

	value = ioread32(ep->fotg210->reg + 0x1C4);
	value &= ~DMACPSR1_DMA_ABORT;
	iowrite32(value, ep->fotg210->reg + 0x1C4);

}

static void ivot_usbdev_start_ep0_data(struct fotg210_ep *ep,
			struct fotg210_request *req)
{
	u32 *buffer;
	u32 value,length,i=0;
	s32	opsize;

	buffer = (u32 *)(req->req.buf + req->req.actual);

	if (req->req.length - req->req.actual > 64)  {
		length = 64;

		if (ep->dir_in) {
			value = ioread32(ep->fotg210->reg +
						FOTG210_DMISGR0);
			value &= ~DMISGR0_MCX_IN_INT;
			iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR0);
		}

	} else {
		length = req->req.length - req->req.actual;

		if (ep->dir_in) {
			value = ioread32(ep->fotg210->reg +
						FOTG210_DMISGR0);
			value |= DMISGR0_MCX_IN_INT;
			iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR0);
		}
	}

	value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
	if(value & DCFESR_CX_DATAPORT_EN)
		pr_err("DATAPORT EN ERROR!!!\n");

	if (ep->dir_in) {
		if (debug_on)
			ep0numsg("ivot_usbdev_start_ep0_data IN 0x%X  act=0x%X\n",req->req.length,req->req.actual);

		value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
		value &= ~DCFESR_CX_FNT_IN;
		value |= (length<<16);
		value |= DCFESR_CX_DATAPORT_EN;
		iowrite32(value, ep->fotg210->reg + FOTG210_DCFESR);


		opsize = length;
		while(opsize>0)
		{
			iowrite32(buffer[i++], ep->fotg210->reg + FOTG210_CXDATAPORT);
			opsize-=4;
		}
	} else {
		if (debug_on)
			ep0numsg("ivot_usbdev_start_ep0_data OUT 0x%X  act=0x%X\n",req->req.length,req->req.actual);

		value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
		value |= DCFESR_CX_DATAPORT_EN;
		iowrite32(value, ep->fotg210->reg + FOTG210_DCFESR);

		opsize = (ioread32(ep->fotg210->reg + FOTG210_DCFESR) & DCFESR_CX_FNT_OUT)>>24;
		while(!opsize) {
			msleep(1);
			opsize = (ioread32(ep->fotg210->reg + FOTG210_DCFESR) & DCFESR_CX_FNT_OUT)>>24;
		}

		while(opsize>0)
		{
			value = ioread32(ep->fotg210->reg + FOTG210_CXDATAPORT);

			if(opsize>=4) {
				buffer[i++] = value;
			} else if (opsize==3) {
				buffer[i] &= ~0xFFFFFF;
				value &= 0xFFFFFF;
				buffer[i] += value;
			} else if (opsize==2) {
				buffer[i] &= ~0xFFFF;
				value &= 0xFFFF;
				buffer[i] += value;
			} else if (opsize==1) {
				buffer[i] &= ~0xFF;
				value &= 0xFF;
				buffer[i] += value;
			}
			opsize-=4;
		}


	}

	value = ioread32(ep->fotg210->reg + FOTG210_DCFESR);
	value &= ~DCFESR_CX_DATAPORT_EN;
	iowrite32(value, ep->fotg210->reg + FOTG210_DCFESR);

	/* update actual transfer length */
	req->req.actual += length;

}


static void fotg210_start_dma(struct fotg210_ep *ep,
			struct fotg210_request *req)
{
	dma_addr_t d;
	u8 *buffer;
	u32 length,slice,szOp;

	if (ep->epnum) {
		if (ep->dir_in) {
			buffer = req->req.buf + req->req.actual;
			length = req->req.length - req->req.actual;

			//if(length>4096)
			//	length=4096;

		} else {
			buffer = req->req.buf + req->req.actual;
			length = ioread32(ep->fotg210->reg +
					FOTG210_FIBCR(ep->epnum - 1));
			length &= FIBCR_BCFX;

			if(length < 512)
				length = length;//short packet
			else
				length = req->req.length - req->req.actual;

			if (outslice && (ep->type == USB_ENDPOINT_XFER_BULK)) {
				if(length > ep->ep.maxpacket)
					length = ep->ep.maxpacket;
			}
		}
	} else {
		pr_err("ep0 has no dma. shall not enter enable_dma!!\n");
		buffer = 0;
		length = 0;
	}

	if (debug_on)
		numsg("fotg210_start_dma %d ",length);

	szOp = 0;

	while(szOp<length) {

		//if((length - szOp) > 4096)
		//	slice = 4096;
		//else
			slice = (length - szOp);

		d = dma_map_single(NULL, (u8 *)(buffer+szOp), slice,
				ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

		if (dma_mapping_error(NULL, d)) {
			pr_err("dma_mapping_error\n");
			return;
		}

		dma_sync_single_for_device(NULL, d, slice,
					   ep->dir_in ? DMA_TO_DEVICE :
						DMA_FROM_DEVICE);

		fotg210_enable_dma(ep, d, slice);

		/* check if dma is done */
		fotg210_wait_dma_done(ep);

		fotg210_disable_dma(ep);

		szOp+=slice;
		dma_unmap_single(NULL, d, slice, DMA_TO_DEVICE);
	}

	/* update actual transfer length */
	req->req.actual += length;

}

static void fotg210_ep0_queue(struct fotg210_ep *ep,
				struct fotg210_request *req)
{
	if (debug_on)
		ep0numsg("fotg210_ep0_queue\n");

	if (!req->req.length) {
		fotg210_done(ep, req, 0);
		return;
	}
	if (ep->dir_in) { /* if IN */
		if (req->req.length) {
			ivot_usbdev_start_ep0_data(ep, req);
		} else {
			pr_err("%s : req->req.length = 0x%x\n",
			       __func__, req->req.length);
		}
		if ((req->req.length == req->req.actual) ||
		    (req->req.actual < ep->ep.maxpacket))
			fotg210_done(ep, req, 0);

	} else { /* OUT */
		u32 value = ioread32(ep->fotg210->reg +
					FOTG210_DMISGR0);

		value &= ~DMISGR0_MCX_OUT_INT;
		iowrite32(value, ep->fotg210->reg + FOTG210_DMISGR0);
	}
}

static int fotg210_ep_queue(struct usb_ep *_ep, struct usb_request *_req,
				gfp_t gfp_flags)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	unsigned long flags;
	int request = 0;

	if (debug_on)
		itfnumsg("fotg210_ep_queue\n");

	ep = container_of(_ep, struct fotg210_ep, ep);
	req = container_of(_req, struct fotg210_request, req);

	if (ep->fotg210->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	spin_lock_irqsave(&ep->fotg210->lock, flags);

	if (list_empty(&ep->queue))
		request = 1;

	list_add_tail(&req->queue, &ep->queue);

	req->req.actual = 0;
	req->req.status = -EINPROGRESS;

	if (!ep->epnum) /* ep0 */
		fotg210_ep0_queue(ep, req);
	else if (request && !ep->stall)
		fotg210_enable_fifo_int(ep);

	spin_unlock_irqrestore(&ep->fotg210->lock, flags);

	return 0;
}

static int fotg210_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct fotg210_ep *ep;
	struct fotg210_request *req;
	unsigned long flags;

	if (debug_on)
		itfnumsg("fotg210_ep_dequeue\n");

	ep = container_of(_ep, struct fotg210_ep, ep);
	req = container_of(_req, struct fotg210_request, req);

	spin_lock_irqsave(&ep->fotg210->lock, flags);
	if (!list_empty(&ep->queue))
		fotg210_done(ep, req, -ECONNRESET);
	spin_unlock_irqrestore(&ep->fotg210->lock, flags);

	return 0;
}

static void fotg210_set_epnstall(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 value;
	void __iomem *reg;

	/* check if IN FIFO is empty before stall */
	if (ep->dir_in) {
		do {
			value = ioread32(fotg210->reg + FOTG210_DCFESR);
		} while (!(value & DCFESR_FIFO_EMPTY(ep->epnum - 1)));
	}

	reg = (ep->dir_in) ?
		fotg210->reg + FOTG210_INEPMPSR(ep->epnum) :
		fotg210->reg + FOTG210_OUTEPMPSR(ep->epnum);
	value = ioread32(reg);
	value |= INOUTEPMPSR_STL_EP;
	iowrite32(value, reg);
}

static void fotg210_clear_epnstall(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 value;
	void __iomem *reg;

	reg = (ep->dir_in) ?
		fotg210->reg + FOTG210_INEPMPSR(ep->epnum) :
		fotg210->reg + FOTG210_OUTEPMPSR(ep->epnum);
	value = ioread32(reg);
	value &= ~INOUTEPMPSR_STL_EP;
	iowrite32(value, reg);
}

static int fotg210_set_halt_and_wedge(struct usb_ep *_ep, int value, int wedge)
{
	struct fotg210_ep *ep;
	struct fotg210_udc *fotg210;
	unsigned long flags;
	int ret = 0;

	ep = container_of(_ep, struct fotg210_ep, ep);

	fotg210 = ep->fotg210;

	spin_lock_irqsave(&ep->fotg210->lock, flags);

	if (value) {
		fotg210_set_epnstall(ep);
		ep->stall = 1;
		if (wedge)
			ep->wedged = 1;
	} else {
		fotg210_reset_tseq(fotg210, ep->epnum);
		fotg210_clear_epnstall(ep);
		ep->stall = 0;
		ep->wedged = 0;
		if (!list_empty(&ep->queue))
			fotg210_enable_fifo_int(ep);
	}

	spin_unlock_irqrestore(&ep->fotg210->lock, flags);
	return ret;
}

static int fotg210_ep_set_halt(struct usb_ep *_ep, int value)
{
	if (debug_on)
		itfnumsg("fotg210_ep_set_halt\n");

	return fotg210_set_halt_and_wedge(_ep, value, 0);
}

static int fotg210_ep_set_wedge(struct usb_ep *_ep)
{
	if (debug_on)
		itfnumsg("fotg210_ep_set_wedge\n");
	return fotg210_set_halt_and_wedge(_ep, 1, 1);
}

static void fotg210_ep_fifo_flush(struct usb_ep *_ep)
{
	if (debug_on)
		itfnumsg("fotg210_ep_fifo_flush\n");
}

static int fotg210_fifo_status(struct usb_ep *ep)
{
	if (debug_on)
		itfnumsg("fotg210_fifo_status\n");
	return 0;
}

static struct usb_ep_ops fotg210_ep_ops = {
	.enable		= fotg210_ep_enable,
	.disable	= fotg210_ep_disable,

	.alloc_request	= fotg210_ep_alloc_request,
	.free_request	= fotg210_ep_free_request,

	.queue		= fotg210_ep_queue,
	.dequeue	= fotg210_ep_dequeue,

	.set_halt	= fotg210_ep_set_halt,
	.fifo_flush	= fotg210_ep_fifo_flush,
	.set_wedge	= fotg210_ep_set_wedge,
	.fifo_status= fotg210_fifo_status,
};

static void fotg210_set_tx0byte(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 val;
	u32 offset = (ep->dir_in) ? FOTG210_INEPMPSR(ep->epnum) :
				FOTG210_OUTEPMPSR(ep->epnum);

	val = ioread32(fotg210->reg + offset);
	val |= INOUTEPMPSR_TX0BYTE_IEP;
	iowrite32(val, fotg210->reg + offset);

	do{
		udelay(5);
		val = ioread32(fotg210->reg + offset);
	}while(val & INOUTEPMPSR_TX0BYTE_IEP);
}


static void fotg210_clear_tx0byte(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_TX0BYTE);

	value &= ~(TX0BYTE_EP1 | TX0BYTE_EP2 | TX0BYTE_EP3
		   | TX0BYTE_EP4);
	iowrite32(value, fotg210->reg + FOTG210_TX0BYTE);
}

static void fotg210_clear_rx0byte(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_RX0BYTE);

	value &= ~(RX0BYTE_EP1 | RX0BYTE_EP2 | RX0BYTE_EP3
		   | RX0BYTE_EP4);
	iowrite32(value, fotg210->reg + FOTG210_RX0BYTE);
}

/* read 8-byte setup packet only */
static void fotg210_rdsetupp(struct fotg210_udc *fotg210,
		   u8 *buffer)
{
#if 1
	u32 *tmp32;
	tmp32 = (u32 *)buffer;
	*tmp32++ = ioread32(fotg210->reg + FOTG210_CXPORT);
	*tmp32   = ioread32(fotg210->reg + FOTG210_CXPORT);

	if (debug_on)
		ep0numsg("SETUP 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n"
			,buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);
#else
	int i = 0;
	u8 *tmp = buffer;
	u32 data;
	u32 length = 8;

	iowrite32(DMATFNR_ACC_CXF, fotg210->reg + FOTG210_DMATFNR);

	for (i = (length >> 2); i > 0; i--) {
		data = ioread32(fotg210->reg + FOTG210_CXPORT);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		*(tmp + 3) = (data >> 24) & 0xFF;
		tmp = tmp + 4;
	}

	switch (length % 4) {
	case 1:
		data = ioread32(fotg210->reg + FOTG210_CXPORT);
		*tmp = data & 0xFF;
		break;
	case 2:
		data = ioread32(fotg210->reg + FOTG210_CXPORT);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		break;
	case 3:
		data = ioread32(fotg210->reg + FOTG210_CXPORT);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		break;
	default:
		break;
	}

	iowrite32(DMATFNR_DISDMA, fotg210->reg + FOTG210_DMATFNR);
#endif

}

static void fotg210_set_configuration(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DAR);

	value |= DAR_AFT_CONF;
	iowrite32(value, fotg210->reg + FOTG210_DAR);

	// reset fifo/ep mapping
	iowrite32(0x88000200, fotg210->reg + 0x160);
	iowrite32(0x88000200, fotg210->reg + 0x164);
	iowrite32(0x88000200, fotg210->reg + 0x168);
	iowrite32(0x88000200, fotg210->reg + 0x16C);
	iowrite32(0x88000200, fotg210->reg + 0x170);
	iowrite32(0x88000200, fotg210->reg + 0x174);
	iowrite32(0x88000200, fotg210->reg + 0x178);
	iowrite32(0x88000200, fotg210->reg + 0x17C);
	iowrite32(0, fotg210->reg + 0x1A8);
	iowrite32(0, fotg210->reg + 0x1AC);

	if(ioread32(fotg210->reg + FOTG210_DMCR) & DMCR_HS_EN)
		iowrite32(0x44C, fotg210->reg + FOTG210_SOFTM);
	else
		iowrite32(0x2710, fotg210->reg + FOTG210_SOFTM);

}

static void fotg210_set_dev_addr(struct fotg210_udc *fotg210, u32 addr)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DAR);

	value |= (addr & 0x7F);
	iowrite32(value, fotg210->reg + FOTG210_DAR);
}

static void fotg210_set_cxstall(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DCFESR);

	value |= DCFESR_CX_STL;
	iowrite32(value, fotg210->reg + FOTG210_DCFESR);
	if (debug_on)
		numsg("fotg210_set_cxstall\n");
}

static void fotg210_request_error(struct fotg210_udc *fotg210)
{
	fotg210_set_cxstall(fotg210);
	pr_err("request error!!\n");
}

static void fotg210_set_address(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	if (ctrl->wValue >= 0x0100) {
		fotg210_request_error(fotg210);
	} else {
		fotg210_set_dev_addr(fotg210, ctrl->wValue);
		fotg210_set_cxdone(fotg210);
	}
}

static void fotg210_set_feature(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		fotg210_set_cxdone(fotg210);
		break;
	case USB_RECIP_INTERFACE:
		fotg210_set_cxdone(fotg210);
		break;
	case USB_RECIP_ENDPOINT: {
		u8 epnum;
		epnum = le16_to_cpu(ctrl->wIndex) & USB_ENDPOINT_NUMBER_MASK;
		if (epnum)
			fotg210_set_epnstall(fotg210->ep[epnum]);
		else
			fotg210_set_cxstall(fotg210);
		fotg210_set_cxdone(fotg210);
		}
		break;
	default:
		fotg210_request_error(fotg210);
		break;
	}
}

static void fotg210_clear_feature(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	struct fotg210_ep *ep =
		fotg210->ep[ctrl->wIndex & USB_ENDPOINT_NUMBER_MASK];

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		fotg210_set_cxdone(fotg210);
		break;
	case USB_RECIP_INTERFACE:
		fotg210_set_cxdone(fotg210);
		break;
	case USB_RECIP_ENDPOINT:
		if (ctrl->wIndex & USB_ENDPOINT_NUMBER_MASK) {
			if (ep->wedged) {
				fotg210_set_cxdone(fotg210);
				break;
			}

			fotg210_clear_epnstall(ep);

			if (ep->stall)
				fotg210_set_halt_and_wedge(&ep->ep, 0, 0);
		}
		fotg210_set_cxdone(fotg210);
		break;
	default:
		fotg210_request_error(fotg210);
		break;
	}
}

static int fotg210_is_epnstall(struct fotg210_ep *ep)
{
	struct fotg210_udc *fotg210 = ep->fotg210;
	u32 value;
	void __iomem *reg;

	reg = (ep->dir_in) ?
		fotg210->reg + FOTG210_INEPMPSR(ep->epnum) :
		fotg210->reg + FOTG210_OUTEPMPSR(ep->epnum);
	value = ioread32(reg);
	return value & INOUTEPMPSR_STL_EP ? 1 : 0;
}

static void fotg210_get_status(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	u8 epnum;

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		fotg210->ep0_data = 1 << USB_DEVICE_SELF_POWERED;
		break;
	case USB_RECIP_INTERFACE:
		fotg210->ep0_data = 0;
		break;
	case USB_RECIP_ENDPOINT:
		epnum = ctrl->wIndex & USB_ENDPOINT_NUMBER_MASK;
		if (epnum)
			fotg210->ep0_data =
				fotg210_is_epnstall(fotg210->ep[epnum])
				<< USB_ENDPOINT_HALT;
		else
			fotg210_request_error(fotg210);
		break;

	default:
		fotg210_request_error(fotg210);
		return;		/* exit */
	}

	fotg210->ep0_req->buf = &fotg210->ep0_data;
	fotg210->ep0_req->length = 2;

	spin_unlock(&fotg210->lock);
	fotg210_ep_queue(fotg210->gadget.ep0, fotg210->ep0_req, GFP_KERNEL);
	spin_lock(&fotg210->lock);
}

static int fotg210_setup_packet(struct fotg210_udc *fotg210,
				struct usb_ctrlrequest *ctrl)
{
	u8 *p = (u8 *)ctrl;
	u8 ret = 0;

	fotg210_rdsetupp(fotg210, p);

	fotg210->ep[0]->dir_in = ctrl->bRequestType & USB_DIR_IN;

	if (fotg210->gadget.speed == USB_SPEED_UNKNOWN) {
		u32 value = ioread32(fotg210->reg + FOTG210_DMCR);
		fotg210->gadget.speed = value & DMCR_HS_EN ?
				USB_SPEED_HIGH : USB_SPEED_FULL;
	}

	/* check request */
	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
		switch (ctrl->bRequest) {
		case USB_REQ_GET_STATUS:
			fotg210_get_status(fotg210, ctrl);
			if (debug_on)
				numsg("USB_REQ_GET_STATUS\n");
			break;
		case USB_REQ_CLEAR_FEATURE:
			fotg210_clear_feature(fotg210, ctrl);
			if (debug_on)
				numsg("USB_REQ_CLEAR_FEATURE\n");
			break;
		case USB_REQ_SET_FEATURE:
			fotg210_set_feature(fotg210, ctrl);
			if (debug_on)
				numsg("USB_REQ_SET_FEATURE\n");
			break;
		case USB_REQ_SET_ADDRESS:
			fotg210_set_address(fotg210, ctrl);
			if (debug_on)
				numsg("USB_REQ_SET_ADDRESS\n");
			break;
		case USB_REQ_SET_CONFIGURATION:
			fotg210_set_configuration(fotg210);
			ret = 1;
			if (debug_on)
				numsg("USB_REQ_SET_CONFIGURATION\n");
			break;
		default:
			ret = 1;
			break;
		}
	} else {
		ret = 1;
	}

	return ret;
}

static void fotg210_ep0out(struct fotg210_udc *fotg210)
{
	struct fotg210_ep *ep = fotg210->ep[0];

	if (debug_on)
		numsg("fotg210_ep0in\n");

	if (!list_empty(&ep->queue) && !ep->dir_in) {
		struct fotg210_request *req;

		req = list_first_entry(&ep->queue,
			struct fotg210_request, queue);

		if (req->req.length)
			ivot_usbdev_start_ep0_data(ep, req);
			//fotg210_start_dma(ep, req);

		if ((req->req.length == req->req.actual) ||
		    (req->req.actual < ep->ep.maxpacket))
			fotg210_done(ep, req, 0);
	} else {
		pr_err("%s : empty queue\n", __func__);
	}
}

static void fotg210_ep0in(struct fotg210_udc *fotg210)
{
	struct fotg210_ep *ep = fotg210->ep[0];

	if (debug_on)
		numsg("fotg210_ep0in\n");

	if ((!list_empty(&ep->queue)) && (ep->dir_in)) {
		struct fotg210_request *req;

		req = list_entry(ep->queue.next,
				struct fotg210_request, queue);

		if (req->req.length)
			ivot_usbdev_start_ep0_data(ep, req);
			//fotg210_start_dma(ep, req);

		if ((req->req.length == req->req.actual) ||
		    (req->req.actual < ep->ep.maxpacket))
			fotg210_done(ep, req, 0);
	} else {
		fotg210_set_cxdone(fotg210);
	}
}

static void fotg210_clear_comabt_int(struct fotg210_udc *fotg210)
{
	u32 value = ioread32(fotg210->reg + FOTG210_DISGR0);

	value &= ~DISGR0_CX_COMABT_INT;
	iowrite32(value, fotg210->reg + FOTG210_DISGR0);
}

static void fotg210_in_fifo_handler(struct fotg210_ep *ep)
{
	struct fotg210_request *req = list_entry(ep->queue.next,
					struct fotg210_request, queue);

	if (debug_on)
		numsg("fotg210_in_fifo_handler %d\n",req->req.length);

	if (req->req.length)
		fotg210_start_dma(ep, req);
	else
		fotg210_set_tx0byte(ep);

	if (req->req.length == req->req.actual)
		fotg210_done(ep, req, 0);
}

static void fotg210_out_fifo_handler(struct fotg210_ep *ep)
{
	struct fotg210_request *req = list_entry(ep->queue.next,
						 struct fotg210_request, queue);

	if (debug_on)
		numsg("fotg210_out_fifo_handler\n");

	fotg210_start_dma(ep, req);

	/* finish out transfer */
	if (req->req.length == req->req.actual ||
	    (req->req.actual & (ep->ep.maxpacket-1)))
		fotg210_done(ep, req, 0);
}

static irqreturn_t fotg210_irq(int irq, void *_fotg210)
{
	struct fotg210_udc *fotg210 = _fotg210;
	u32 int_grp = ioread32(fotg210->reg + FOTG210_DIGR);
	u32 int_msk = ioread32(fotg210->reg + FOTG210_DMIGR);

	int_grp &= ~int_msk;

	spin_lock(&fotg210->lock);

	if (int_grp & DIGR_INT_G2) {
		void __iomem *reg = fotg210->reg + FOTG210_DISGR2;
		u32 int_grp2 = ioread32(reg);
		u32 int_msk2 = ioread32(fotg210->reg + FOTG210_DMISGR2);
		//u32 value;

		int_grp2 &= ~int_msk2;
		//iowrite32(int_grp2, reg);
		//pr_err("2");

		//printk("G2 0x%08X\n",int_grp2);
		if (int_grp2 & DISGR2_USBRST_INT) {
			iowrite32(DISGR2_USBRST_INT, reg);

			// reset releated settings when bus reset got
			iowrite32(0x0, fotg210->reg + FOTG210_DAR);

			iowrite32(0x88000200, fotg210->reg + 0x160);
			iowrite32(0x88000200, fotg210->reg + 0x164);
			iowrite32(0x88000200, fotg210->reg + 0x168);
			iowrite32(0x88000200, fotg210->reg + 0x16C);
			iowrite32(0x88000200, fotg210->reg + 0x170);
			iowrite32(0x88000200, fotg210->reg + 0x174);
			iowrite32(0x88000200, fotg210->reg + 0x178);
			iowrite32(0x88000200, fotg210->reg + 0x17C);
			iowrite32(0, fotg210->reg + 0x1A8);
			iowrite32(0, fotg210->reg + 0x1AC);

			if (debug_on)
				devnumsg("fotg210 udc reset\n");
		}
		if (int_grp2 & DISGR2_SUSP_INT) {
			iowrite32(DISGR2_SUSP_INT, reg);
			if (debug_on)
				devnumsg("fotg210 udc suspend\n");
		}
		if (int_grp2 & DISGR2_RESM_INT) {
			iowrite32(DISGR2_RESM_INT, reg);
			if (debug_on)
				devnumsg("fotg210 udc resume\n");
		}
		if (int_grp2 & DISGR2_ISO_SEQ_ERR_INT) {
			iowrite32(DISGR2_ISO_SEQ_ERR_INT, reg);
			if (debug_on)
				devnumsg("fotg210 iso sequence error\n");
		}
		if (int_grp2 & DISGR2_ISO_SEQ_ABORT_INT) {
			iowrite32(DISGR2_ISO_SEQ_ABORT_INT, reg);
			if (debug_on)
				devnumsg("fotg210 iso sequence abort\n");
		}
		if (int_grp2 & DISGR2_DMA_CMPLT) {
			pr_err("fotg210 DMA_CMPLT\n");
		}
		if (int_grp2 & DISGR2_DMA_ERROR) {
			pr_err("fotg210 DMA_ERROR\n");
		}
		if (int_grp2 & DISGR2_DMA_CMPLT2) {
			pr_err("fotg210 DMA_CMPLT2\n");
		}
		if (int_grp2 & DISGR2_DMA_CMPLT3) {
			pr_err("fotg210 DMA_CMPLT3\n");
		}
		if (int_grp2 & DISGR2_DMA_CMPLT4) {
			pr_err("fotg210 DMA_CMPLT4\n");
		}
		if (int_grp2 & DISGR2_DMA_CMPLT5) {
			pr_err("fotg210 DMA_CMPLT5\n");
		}

		if (int_grp2 & DISGR2_TX0BYTE_INT) {
			iowrite32(DISGR2_TX0BYTE_INT, reg);
			fotg210_clear_tx0byte(fotg210);
			if (debug_on)
				numsg("fotg210 transferred 0 byte\n");
		}
		if (int_grp2 & DISGR2_RX0BYTE_INT) {
			iowrite32(DISGR2_RX0BYTE_INT, reg);
			fotg210_clear_rx0byte(fotg210);
			if (debug_on)
				numsg("fotg210 received 0 byte\n");
		}

	}


	if (int_grp & DIGR_INT_G0) {
		void __iomem *reg = fotg210->reg + FOTG210_DISGR0;
		u32 int_grp0 = ioread32(reg);
		u32 int_msk0 = ioread32(fotg210->reg + FOTG210_DMISGR0);
		struct usb_ctrlrequest ctrl;

		int_grp0 &= ~int_msk0;

		if(int_grp0 & ~0x1) {
			if (debug_on)
				numsg("G0 0x%08X\n",int_grp0);
		}
		//pr_err("0");

		/* the highest priority in this source register */
		if (int_grp0 & DISGR0_CX_COMABT_INT) {
			fotg210_clear_comabt_int(fotg210);
			if (debug_on)
				numsg("fotg210 CX command abort\n");
		}

		if (int_grp0 & DISGR0_CX_SETUP_INT) {
			if (debug_on)
				ep0numsg("fotg210 SETUP\n");
			if (fotg210_setup_packet(fotg210, &ctrl)) {
				spin_unlock(&fotg210->lock);
				if (fotg210->driver->setup(&fotg210->gadget,
							   &ctrl) < 0)
					fotg210_set_cxstall(fotg210);
				spin_lock(&fotg210->lock);
			}
		}
		if (int_grp0 & DISGR0_CX_COMEND_INT) {
			if (debug_on)
				numsg("fotg210 cmd end\n");
		}

		if (int_grp0 & DISGR0_CX_IN_INT) {
			fotg210_ep0in(fotg210);
			if (debug_on)
				numsg("fotg210 CX_IN_INT\n");
		}

		if (int_grp0 & DISGR0_CX_OUT_INT) {
			fotg210_ep0out(fotg210);
			if (debug_on)
				numsg("fotg210 CX_OUT_INT\n");
		}

		if (int_grp0 & DISGR0_CX_COMFAIL_INT) {
			fotg210_set_cxstall(fotg210);
			if (debug_on)
				numsg("fotg210 ep0 fail\n");
		}
	}

	if (int_grp & DIGR_INT_G1) {
		void __iomem *reg = fotg210->reg + FOTG210_DISGR1;
		u32 int_grp1 = ioread32(reg);
		u32 int_msk1 = ioread32(fotg210->reg + FOTG210_DMISGR1);
		int fifo;

		int_grp1 &= ~int_msk1;

		if (debug_on)
			numsg("G1 0x%08X\n",int_grp1);
		//pr_err("1");

		for (fifo = 0; fifo < FOTG210_MAX_FIFO_NUM; fifo+=2) {
			if (int_grp1 & DISGR1_IN_INT(fifo)) {
				fotg210_in_fifo_handler(fotg210->ep[(fifo>>1) + 1]);
			}

			if ((int_grp1 & DISGR1_OUT_INT(fifo)) ||
			    (int_grp1 & DISGR1_SPK_INT(fifo))) {
				fotg210_out_fifo_handler(fotg210->ep[(fifo>>1) + 1]);
			}
		}
	}

	spin_unlock(&fotg210->lock);

	return IRQ_HANDLED;
}

#if 0
static irqreturn_t fotg210_isr(int irq, void *_fotg210)
{
	u32 value;
	struct fotg210_udc *fotg210 = _fotg210;

	spin_lock(&fotg210->lock);

	value = ioread32(fotg210->reg + FOTG210_DMCR);
	value &= ~DMCR_GLINT_EN;
	iowrite32(value, fotg210->reg + FOTG210_DMCR);

	spin_unlock(&fotg210->lock);

	return IRQ_WAKE_THREAD;
}

static irqreturn_t fotg210_ist(int irq, void *_fotg210)
{
	u32 value;
	struct fotg210_udc *fotg210 = (struct fotg210_udc *)_fotg210;

	fotg210_irq(0,(void *)_fotg210);

	value = ioread32(fotg210->reg + FOTG210_DMCR);
	value |= DMCR_GLINT_EN;
	iowrite32(value, fotg210->reg + FOTG210_DMCR);

	return IRQ_HANDLED;
}
#endif

static void fotg210_disable_unplug(struct fotg210_udc *fotg210)
{
	u32 reg = ioread32(fotg210->reg + FOTG210_PHYTMSR);

	reg &= ~PHYTMSR_UNPLUG;
	iowrite32(reg, fotg210->reg + FOTG210_PHYTMSR);
}

static void fotg210_enable_unplugsuspend(struct fotg210_udc *fotg210)
{
	u32 reg = ioread32(fotg210->reg + FOTG210_PHYTMSR);
	reg |= PHYTMSR_UNPLUG;
	iowrite32(reg, fotg210->reg + FOTG210_PHYTMSR);

	reg = ioread32(fotg210->reg + FOTG210_DMACPSR1);
	reg |= DMACPSR1_DEVSUSPEND;
	iowrite32(reg, fotg210->reg + FOTG210_DMACPSR1);


}

static int fotg210_udc_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	struct fotg210_udc *fotg210 = gadget_to_fotg210(g);
	u32 value;

	if (debug_on)
		itfnumsg("fotg210_udc_start\n");

	/* make device exit suspend */
	value = ioread32(fotg210->reg + FOTG210_DMACPSR1);
	value &= ~DMACPSR1_DEVSUSPEND;
	iowrite32(value, fotg210->reg + FOTG210_DMACPSR1);

	/* hook up the driver */
	driver->driver.bus = NULL;
	fotg210->driver = driver;

	fotg210_disable_unplug(fotg210);

	/* enable device global interrupt */
	value = ioread32(fotg210->reg + FOTG210_DMCR);
	value &= ~(0xFF<<16);
	value |= (0x5<<16);
	value |= DMCR_GLINT_EN;
	iowrite32(value|DMCR_SFRST, fotg210->reg + FOTG210_DMCR);
	iowrite32(value, fotg210->reg + FOTG210_DMCR);

	iowrite32(DTR_TST_CLRFF|ioread32(fotg210->reg + FOTG210_DTR), fotg210->reg + FOTG210_DTR);

	return 0;
}

static void fotg210_init(struct fotg210_udc *fotg210)
{
	u32 value;

#if 0
	// Fixed using USB0 as device controller.
	if (!(ioread32((void __iomem *)(NVT_TOP_ADDR_BASE+0x20)) & 0x2))
	{
		iowrite32(ioread32((void __iomem *)(NVT_TOP_ADDR_BASE+0x20))|0x3,
			((void __iomem *)(NVT_TOP_ADDR_BASE+0x20)));
		pr_err("USB_ID select error\n");
	}

	// Set VBUS
	pinmux_set_pinmux(PINMUX_FUNC_ID_USB_VBUSI, PINMUX_USB_SEL_ACTIVE);
#endif

	iowrite32(0x3, fotg210->reg + 0x310);

	/* make device exit suspend */
	value = ioread32(fotg210->reg + FOTG210_DMACPSR1);
	value &= ~DMACPSR1_DEVSUSPEND;
	iowrite32(value, fotg210->reg + FOTG210_DMACPSR1);

	udelay(200);

	/* Configure PHY related settings below */
	{
		u16 data=0;
		s32 result=0;
		u32 temp;
		u8 u2_trim_swctrl=6, u2_trim_sqsel=4, u2_trim_resint=8;

		result= efuse_readParamOps(EFUSE_USBC_TRIM_DATA, &data);
		if(result == 0) {
			u2_trim_swctrl = data&0x7;
			u2_trim_sqsel  = (data>>3)&0x7;
			u2_trim_resint = (data>>6)&0x1F;
		}

		iowrite32(0x20, fotg210->reg+0x1000+(0x51<<2));
		iowrite32(0x30, fotg210->reg+0x1000+(0x50<<2));

		temp = ioread32(fotg210->reg+0x1000+(0x06<<2));
		temp &= ~(0x7<<1);
		temp |= (u2_trim_swctrl<<1);
		iowrite32(temp,fotg210->reg+0x1000+(0x06<<2));

		temp = ioread32(fotg210->reg+0x1000+(0x05<<2));
		temp &= ~(0x7<<2);
		temp |= (u2_trim_sqsel<<2);
		iowrite32(temp,fotg210->reg+0x1000+(0x05<<2));

		iowrite32(0x60+u2_trim_resint, fotg210->reg+0x1000+(0x52<<2));
		iowrite32(0x00, fotg210->reg+0x1000+(0x51<<2));

		iowrite32(0x100+u2_trim_resint, fotg210->reg + 0x30C);
	}


	/* VBUS debounce time */
	iowrite32(ioread32(fotg210->reg + FOTG210_OTGCTRLSTS)|0x400, fotg210->reg + FOTG210_OTGCTRLSTS);

	/* disable device global interrupt */
	value = ioread32(fotg210->reg + FOTG210_DMCR);
	value &= ~DMCR_GLINT_EN;
	value |= DMCR_HALF_SPEED;
	iowrite32(value, fotg210->reg + FOTG210_DMCR);

	/* disable global interrupt and set int polarity to active high */
	iowrite32(GMIR_MHC_INT | GMIR_MOTG_INT, fotg210->reg + FOTG210_GMIR);

	/* disable all fifo interrupt */
	iowrite32(~(u32)0, fotg210->reg + FOTG210_DMISGR1);

	/* disable cmd end */
	value = ioread32(fotg210->reg + FOTG210_DMISGR0);
	value |= (DMISGR0_MCX_COMEND|DMISGR0_MCX_OUT_INT|DMISGR0_MCX_IN_INT);
	iowrite32(value, fotg210->reg + FOTG210_DMISGR0);

	iowrite32(DTR_TST_CLRFF|ioread32(fotg210->reg + FOTG210_DTR), fotg210->reg + FOTG210_DTR);
	iowrite32(0x7, fotg210->reg + FOTG210_DICR);

	/* Mask idle int */
	value = ioread32(fotg210->reg + FOTG210_DMISGR2);
	value |= (DMISGR2_MDMA_IDLE|DMISGR2_MDMA_WAKEUPVBUS|DMISGR2_MDMA_CMPLT|DMISGR2_MDMA_ERROR);
	iowrite32(value, fotg210->reg + FOTG210_DMISGR2);
}

static int fotg210_udc_stop(struct usb_gadget *g)
{
	struct fotg210_udc *fotg210 = gadget_to_fotg210(g);
	unsigned long	flags;
	u32 value;

	if (debug_on)
		itfnumsg("fotg210_udc_stop\n");

	spin_lock_irqsave(&fotg210->lock, flags);

	//fotg210_init(fotg210);
	fotg210->driver = NULL;
	fotg210_enable_unplugsuspend(fotg210);

	/* enable device global interrupt */
	value = ioread32(fotg210->reg + FOTG210_DMCR);
	value &= ~DMCR_GLINT_EN;
	iowrite32(value, fotg210->reg + FOTG210_DMCR);

	spin_unlock_irqrestore(&fotg210->lock, flags);

	return 0;
}

/**
 * fotg200_udc_pullup - Enable/disable pullup on D+ line.
 * @gadget: USB slave device.
 * @is_on: 0 to disable pullup, 1 to enable.
 *
 * See notes in bcm63xx_select_pullup().
 */
static int fotg210_udc_pullup(struct usb_gadget *gadget, int is_on)
{
	return 0;
}

static struct usb_gadget_ops fotg210_gadget_ops = {
	.udc_start		= fotg210_udc_start,
	.udc_stop		= fotg210_udc_stop,
	.pullup			= fotg210_udc_pullup,
};

static int fotg210_udc_remove(struct platform_device *pdev)
{
	struct fotg210_udc *fotg210 = platform_get_drvdata(pdev);

	if (debug_on)
		itfnumsg("fotg210_udc_remove\n");

	usb_del_gadget_udc(&fotg210->gadget);
	free_irq(platform_get_irq(pdev, 0), fotg210);

	fotg210_ep_free_request(&fotg210->ep[0]->ep, fotg210->ep0_req);

	{
		iowrite32(0x20, (void __iomem *)(fotg210->reg+0x1000+(0x51<<2)));
		iowrite32(0x30, (void __iomem *)(fotg210->reg+0x1000+(0x50<<2)));
		iowrite32(0x6D, (void __iomem *)(fotg210->reg+0x1000+(0x52<<2)));
		iowrite32(0x00, (void __iomem *)(fotg210->reg+0x1000+(0x51<<2)));

		iowrite32(0x2, fotg210->reg + 0x310);
		printk("Assert reset!\n");
	}
	fotg210_enable_unplugsuspend(fotg210);

	iounmap(fotg210->reg);
	kfree(fotg210);

	{
		struct clk *source_clk;

		source_clk = clk_get(NULL, "f0600000.usb20");
		if (IS_ERR(source_clk)) {
			printk("faile to get clock f0600000.usb20\n");
		} else {
			clk_unprepare(source_clk);
			clk_put(source_clk);
		}
	}

	return 0;
}

static int fotg210_udc_probe(struct platform_device *pdev)
{
	struct resource *res, *ires;
	struct fotg210_udc *fotg210 = NULL;
	struct fotg210_ep *_ep[FOTG210_MAX_NUM_EP];
	int ret = 0;
	int i;

	if (debug_on)
		numsg("fotg210_udc_probe\n");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("platform_get_resource error.\n");
		return -ENODEV;
	}

	ires = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!ires) {
		pr_err("platform_get_resource IORESOURCE_IRQ error.\n");
		return -ENODEV;
	}

	ret = -ENOMEM;

	/* initialize udc */
	fotg210 = kzalloc(sizeof(struct fotg210_udc), GFP_KERNEL);
	if (fotg210 == NULL) {
		pr_err("kzalloc error\n");
		goto err_alloc;
	}

	for (i = 0; i < FOTG210_MAX_NUM_EP; i++) {
		_ep[i] = kzalloc(sizeof(struct fotg210_ep), GFP_KERNEL);
		if (_ep[i] == NULL) {
			pr_err("_ep kzalloc error\n");
			goto err_alloc;
		}
		fotg210->ep[i] = _ep[i];
	}

	fotg210->reg = ioremap(res->start, resource_size(res));
	if (fotg210->reg == NULL) {
		pr_err("ioremap error.\n");
		goto err_map;
	}

	{
		struct clk *source_clk;

		source_clk = clk_get(NULL, "f0600000.usb20");
		if (IS_ERR(source_clk)) {
			printk("faile to get clock f0600000.usb20\n");
		} else {
			clk_prepare(source_clk);
			clk_put(source_clk);
		}
	}

	spin_lock_init(&fotg210->lock);

	platform_set_drvdata(pdev, fotg210);

	fotg210->gadget.ops = &fotg210_gadget_ops;

	fotg210->gadget.max_speed = USB_SPEED_HIGH;
	fotg210->gadget.dev.parent = &pdev->dev;
	fotg210->gadget.dev.dma_mask = pdev->dev.dma_mask;
	fotg210->gadget.name = udc_name;

	INIT_LIST_HEAD(&fotg210->gadget.ep_list);

	for (i = 0; i < FOTG210_MAX_NUM_EP; i++) {
		struct fotg210_ep *ep = fotg210->ep[i];

		if (i) {
			INIT_LIST_HEAD(&fotg210->ep[i]->ep.ep_list);
			list_add_tail(&fotg210->ep[i]->ep.ep_list,
				      &fotg210->gadget.ep_list);
		}
		ep->fotg210 = fotg210;
		INIT_LIST_HEAD(&ep->queue);
		//ep->ep.name = fotg210_ep_name[i];
		ep->ep.name = ep_info[i].name;
		ep->ep.caps = ep_info[i].caps;
		ep->ep.ops = &fotg210_ep_ops;
		usb_ep_set_maxpacket_limit(&ep->ep, (unsigned short) ~0);
	}
	usb_ep_set_maxpacket_limit(&fotg210->ep[0]->ep, 0x40);
	fotg210->gadget.ep0 = &fotg210->ep[0]->ep;
	INIT_LIST_HEAD(&fotg210->gadget.ep0->ep_list);

	fotg210->ep0_req = fotg210_ep_alloc_request(&fotg210->ep[0]->ep,
				GFP_KERNEL);
	if (fotg210->ep0_req == NULL)
		goto err_req;

	fotg210_init(fotg210);

	//fotg210_disable_unplug(fotg210);

#if 0
	/* Using tasklet for command processing */
	ret =  request_threaded_irq(ires->start, fotg210_isr, fotg210_ist, 
			IRQF_SHARED, udc_name, fotg210);
#else
	ret = request_irq(ires->start, fotg210_irq, IRQF_SHARED,
			  udc_name, fotg210);
#endif

	if (ret < 0) {
		pr_err("request_irq error (%d)\n", ret);
		goto err_irq;
	}

	ret = usb_add_gadget_udc(&pdev->dev, &fotg210->gadget);
	if (ret)
		goto err_add_udc;

	dev_info(&pdev->dev, "version %s\n", DRIVER_VERSION);

	return 0;

err_add_udc:
err_irq:
	free_irq(ires->start, fotg210);

err_req:
	fotg210_ep_free_request(&fotg210->ep[0]->ep, fotg210->ep0_req);

err_map:
	if (fotg210->reg)
		iounmap(fotg210->reg);

err_alloc:
	kfree(fotg210);

	return ret;
}


#ifdef CONFIG_OF
static const struct of_device_id of_fotg210_match[] = {
	{
		.compatible = "nvt,fotg200_udc"
	},

	{ },
};
MODULE_DEVICE_TABLE(of, of_fotg210_match);
#endif


static struct platform_driver fotg210_driver = {
	.driver		= {
		.name =	(char *)udc_name,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(of_fotg210_match),
#endif
	},
	.probe		= fotg210_udc_probe,
	.remove		= fotg210_udc_remove,

};

module_platform_driver(fotg210_driver);

MODULE_AUTHOR("Klins Chen <klins_chen@novatek.com.tw>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
