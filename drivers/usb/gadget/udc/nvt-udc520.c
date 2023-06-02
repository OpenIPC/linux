/*
 * NVT680 UDC Driver supports Bulk transfer so far
 *
 * Copyright (C) 2017 Novatek Technology Corporation
 *
 * Author : Klins Chen <klins_chen@novatek.com.tw>
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
#include <linux/of.h>
#include <linux/clk.h>
#include <plat/efuse_protected.h>

#include "nvt520.h"

#define	DRIVER_DESC	"NOVATEK iVot USB Device NVT520 Controller Driver"
#define	DRIVER_VERSION	"19-May-2020"

static const char udc_name[] = "nvt,nvt520_udc";

//static const char * const nvt680_ep_name[] = {
//	"ep0", "ep1", "ep2", "ep3", "ep4", "ep5", "ep6", "ep7", "ep8"};

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
		USB_EP_CAPS(USB_EP_CAPS_TYPE_INT, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep4",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),
	EP_INFO("ep5",
		USB_EP_CAPS(USB_EP_CAPS_TYPE_ALL, USB_EP_CAPS_DIR_ALL)),

#undef EP_INFO
};



#define	NVTUSB_DEBUG	0

#if NVTUSB_DEBUG
#define numsg		printk
#define itfnumsg	printk
#define devnumsg	printk
#define ep0numsg	printk
#define intrmsg		printk
#else
#define numsg		pr_debug
#define itfnumsg	pr_debug
#define devnumsg	pr_debug
#define ep0numsg	pr_debug
#define intrmsg		pr_debug
#endif

#if 1

#define USB_GETREG(x)    ioread32(nvt680->reg + (x))
#define USB_SETREG(x, y) iowrite32((y), nvt680->reg + (x));

static int nvt680_ep_queue(struct usb_ep *_ep, struct usb_request *_req, gfp_t gfp_flags);

void usbnvt_set_ep0_done(struct nvt680_udc *nvt680);
BOOL usbnvt_ep_stall_status(struct nvt680_udc *nvt680, UINT32 EPn);
void usbnvt_set_ep_stall(struct nvt680_udc *nvt680, UINT32 ep_number);
void usbnvt_clr_ep_stall(struct nvt680_udc *nvt680, UINT32 EPn);
void usbnvt_mask_ep_interrupt(struct nvt680_udc *nvt680, UINT32 EPn);
void usbnvt_unmask_ep_interrupt(struct nvt680_udc *nvt680, UINT32 EPn);
void nvt680_stop_ep_transfer(struct nvt680_ep *ep);
void nvt680_enable_ep_transfer(struct nvt680_ep *ep);
void usbnvt_set_ep_write(struct nvt680_udc *nvt680, UINT32 EPn, dma_addr_t d_adr, UINT32 length);
void usbnvt_set_ep_read(struct nvt680_udc *nvt680, UINT32 EPn, dma_addr_t d_adr, UINT32 length);

#if 1
static void nvt680_done(struct nvt680_ep *ep, struct nvt680_request *req, int status)
{
	unsigned long flags;

	if(ep->epnum) {
		itfnumsg("nvt680_done %d\n", ep->epnum);
	} else {
		ep0numsg("nvt680_done %d\n", ep->epnum);
	}
	spin_lock_irqsave(&ep->nvt680->lock, flags);
	list_del_init(&req->queue);
	spin_unlock_irqrestore(&ep->nvt680->lock, flags);

	/* don't modify queue heads during completion callback */
	if (ep->nvt680->gadget.speed == USB_SPEED_UNKNOWN) {
		req->req.status = -ESHUTDOWN;
	} else {
		req->req.status = status;
	}

	if(req->req.complete != NULL) {
		req->req.complete(&ep->ep, &req->req);
	}


	if ((ep->epnum)&&(status == 0)) {
		if (list_empty(&ep->queue)) {
			nvt680_stop_ep_transfer(ep);
		} else {
			if (ep->type != USB_ENDPOINT_XFER_ISOC) {
						nvt680_enable_ep_transfer(ep);
			} else {
				dma_addr_t d_adr;
				UINT32 d_len, epnum = ep->epnum;
				struct nvt680_udc *nvt680;

				nvt680 = ep->nvt680;

				if(ep->dir_in) {
					ep = nvt680->ep[epnum];
					req = list_entry(ep->queue.next, struct nvt680_request, queue);

					if ((req->req.length == 0) && (req->req.actual == 0)) {

						T_USB_DEVMAXPS_EP_REG DevEPMaxPS;

						spin_lock_irqsave(&nvt680->lock, flags);
						DevEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((epnum - 1) << 2));
						DevEPMaxPS.bit.TX0BYTE_IEP = 1;
						USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((epnum - 1) << 2)), DevEPMaxPS.reg);
						spin_unlock_irqrestore(&nvt680->lock, flags);

						do {
							DevEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((epnum - 1) << 2));
							if(!DevEPMaxPS.bit.TX0BYTE_IEP) {
								break;
							}
							msleep(1);
						}while(DevEPMaxPS.bit.TX0BYTE_IEP);

						nvt680_done(ep, req, 0);
					} else {
						d_len = req->req.length;
						d_adr = dma_map_single(NULL, (u8 *)(req->req.buf), d_len, ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

						if (dma_mapping_error(NULL, d_adr)) {
							pr_err("dma_mapping_error\n");
						}

						dma_sync_single_for_device(NULL, d_adr, d_len, ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

						req->req.actual = d_len;
						ep->d_adr = d_adr;
						ep->d_len = d_len;

						usbnvt_set_ep_write(nvt680, epnum, d_adr, d_len);

						nvt680_stop_ep_transfer(ep);
					}
				} else {
					dma_addr_t d_adr;
					UINT32 d_len, epnum = ep->epnum;
					T_USB_DEVFIFOBYTECNT0_REG	reg_devfifo_bc;

					ep = nvt680->ep[epnum];
					req = list_entry(ep->queue.next, struct nvt680_request, queue);

					//reg_devfifo_bc.reg = USB_GETREG(USB_DEVFIFOBYTECNT0_REG_OFS+((epnum-1)<<2));
					//d_len = reg_devfifo_bc.bit.BC_F0;
					reg_devfifo_bc.reg = USB_GETREG(USB_DEVFIFOBYTECNT0_REG_OFS+((nvt680->ep_2_fifo[epnum]>>1)<<2));
					d_len = (reg_devfifo_bc.reg >> ((nvt680->ep_2_fifo[epnum]&0x1)<<4)) & 0x7FF;

					if(!(d_len & 511)) {
						d_len = req->req.length;
					}

					d_adr = dma_map_single(NULL, (u8 *)(req->req.buf), d_len, ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

					if (dma_mapping_error(NULL, d_adr)) {
						pr_err("dma_mapping_error\n");
					}

					dma_sync_single_for_device(NULL, d_adr, d_len, ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

					req->req.actual = d_len;
					ep->d_adr = d_adr;
					ep->d_len = d_len;

					usbnvt_set_ep_read(nvt680, epnum, d_adr, d_len);

					nvt680_stop_ep_transfer(ep);
				}
			}

		}
	}

}


static int nvt680_set_halt_and_wedge(struct usb_ep *_ep, int value, int wedge)
{
	struct nvt680_ep *ep;
	struct nvt680_udc *nvt680;
	unsigned long flags;
	int ret = 0;

	ep = container_of(_ep, struct nvt680_ep, ep);

	nvt680 = ep->nvt680;


	if (value) {
		usbnvt_set_ep_stall(nvt680, ep->epnum);

		ep->stall = 1;

		if (wedge)
			ep->wedged = 1;

	} else {
		T_USB_DEVMAXPS_EP_REG       devEPMaxPS;

		spin_lock_irqsave(&ep->nvt680->lock, flags);
		devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((ep->epnum - 1) << 2));
		devEPMaxPS.bit.RSTG_EP = 1;
		USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((ep->epnum - 1) << 2)), devEPMaxPS.reg);
		devEPMaxPS.bit.RSTG_EP = 0;
		USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((ep->epnum - 1) << 2)), devEPMaxPS.reg);
		spin_unlock_irqrestore(&ep->nvt680->lock, flags);

		usbnvt_clr_ep_stall(nvt680, ep->epnum);

		ep->stall = 0;
		ep->wedged = 0;

		if (!list_empty(&ep->queue)) {
			if (list_empty(&ep->queue)) {
				nvt680_stop_ep_transfer(ep);
			} else {
				nvt680_enable_ep_transfer(ep);
			}
		}
	}

	return ret;
}


static void nvt680_start_ep0_data(struct nvt680_ep *ep,
			struct nvt680_request *req)
{
	u32 *buffer;
	u32 value,length,i=0;
	s32	opsize;
	T_USB_DEVINTMASKGROUP0_REG reg_mask_g0;
	T_USB_DEVCXCFGFIFOSTATUS_REG reg_cxcfg;
	unsigned long flags;
	struct nvt680_udc *nvt680 = ep->nvt680;


	buffer = (u32 *)(req->req.buf + req->req.actual);

	if (req->req.length - req->req.actual > 64)  {
		length = 64;

		if (ep->dir_in) {
			spin_lock_irqsave(&ep->nvt680->lock, flags);
			reg_mask_g0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
			reg_mask_g0.bit.MCX_IN_INT = 0;
			USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, reg_mask_g0.reg);
			spin_unlock_irqrestore(&ep->nvt680->lock, flags);
		}
	} else {
		length = req->req.length - req->req.actual;

		if (ep->dir_in) {
			spin_lock_irqsave(&ep->nvt680->lock, flags);
			reg_mask_g0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
			reg_mask_g0.bit.MCX_IN_INT = 1;
			USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, reg_mask_g0.reg);
			spin_unlock_irqrestore(&ep->nvt680->lock, flags);
		}
	}

	reg_cxcfg.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
	if(reg_cxcfg.bit.CX_DATAPORT_EN)
		pr_err("DATAPORT EN ERROR!!!\n");


	if (ep->dir_in) {
		ep0numsg("fotg200_start_ep0_data IN 0x%X  act=0x%X len=%d\n",req->req.length,req->req.actual,length);

		spin_lock_irqsave(&ep->nvt680->lock, flags);

		reg_cxcfg.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		reg_cxcfg.bit.CX_FNT_IN      = length;
		reg_cxcfg.bit.CX_DATAPORT_EN = 1;
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, reg_cxcfg.reg);

		opsize = length;
		while(opsize>0)
		{
			USB_SETREG(USB_DEVCXDATAPORT_REG_OFS, buffer[i++]);
			opsize-=4;
		}

		reg_cxcfg.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		reg_cxcfg.bit.CX_DATAPORT_EN = 0;
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, reg_cxcfg.reg);

		spin_unlock_irqrestore(&ep->nvt680->lock, flags);

	} else {
		ep0numsg("fotg200_start_ep0_data OUT 0x%X  act=0x%X\n",req->req.length,req->req.actual);

		reg_cxcfg.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		opsize = reg_cxcfg.bit.CX_FNT_OUT;

		while(!opsize) {
			msleep(1);
			reg_cxcfg.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
			opsize = reg_cxcfg.bit.CX_FNT_OUT;
		}

		spin_lock_irqsave(&ep->nvt680->lock, flags);

		reg_cxcfg.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		reg_cxcfg.bit.CX_DATAPORT_EN = 1;
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, reg_cxcfg.reg);

		while(opsize>0)
		{
			value = USB_GETREG(USB_DEVCXDATAPORT_REG_OFS);

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


		reg_cxcfg.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		reg_cxcfg.bit.CX_DATAPORT_EN = 0;
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, reg_cxcfg.reg);

		spin_unlock_irqrestore(&ep->nvt680->lock, flags);


	}

	/* update actual transfer length */
	req->req.actual += length;

}


static void nvt680_ep0_queue(struct nvt680_ep *ep, struct nvt680_request *req)
{
	unsigned long flags;
	struct nvt680_udc *nvt680 = ep->nvt680;

	ep0numsg("nvt680_ep0_queue <%d>\n", req->req.length);

	if (!req->req.length) {

		usbnvt_set_ep0_done(nvt680);
		nvt680_done(ep, req, 0);
		return;
	}

	if (ep->dir_in) {
		/* if IN */

		if (req->req.length) {
			nvt680_start_ep0_data(ep, req);
		} else {
			pr_err("%s : req->req.length = 0x%x\n",
			       __func__, req->req.length);
		}

		if ((req->req.length == req->req.actual) || (req->req.actual < ep->ep.maxpacket)) {
			nvt680_done(ep, req, 0);

			usbnvt_set_ep0_done(nvt680);
		}

	} else {	/* OUT */
		T_USB_DEVINTMASKGROUP0_REG reg_mask_g0;

		spin_lock_irqsave(&ep->nvt680->lock, flags);
		reg_mask_g0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
		reg_mask_g0.bit.MCX_OUT_INT = 0;
		USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, reg_mask_g0.reg);
		spin_unlock_irqrestore(&ep->nvt680->lock, flags);
	}
}



static void nvt680_get_status(struct nvt680_udc *nvt680, struct usb_ctrlrequest *ctrl)
{
	u8 epnum;

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		nvt680->ep0_data = 1 << USB_DEVICE_SELF_POWERED;
		break;
	case USB_RECIP_INTERFACE:
		nvt680->ep0_data = 0;
		break;
	case USB_RECIP_ENDPOINT:
		epnum = ctrl->wIndex & USB_ENDPOINT_NUMBER_MASK;

		if (epnum)
			nvt680->ep0_data = usbnvt_ep_stall_status(nvt680, epnum)<< USB_ENDPOINT_HALT;
		else
			usbnvt_set_ep_stall(nvt680, 0);

		break;

	default:
		usbnvt_set_ep_stall(nvt680, 0);
		return;		/* exit */
	}

	nvt680->ep0_req->buf = &nvt680->ep0_data;
	nvt680->ep0_req->length = 2;
	nvt680_ep_queue(nvt680->gadget.ep0, nvt680->ep0_req, GFP_KERNEL);

}

static void nvt680_clear_feature(struct nvt680_udc *nvt680, struct usb_ctrlrequest *ctrl)
{
	struct nvt680_ep *ep = nvt680->ep[ctrl->wIndex & USB_ENDPOINT_NUMBER_MASK];

	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		usbnvt_set_ep0_done(nvt680);
		break;
	case USB_RECIP_INTERFACE:
		usbnvt_set_ep0_done(nvt680);
		break;
	case USB_RECIP_ENDPOINT:

		if (ctrl->wIndex & USB_ENDPOINT_NUMBER_MASK) {

			if (ep->wedged) {
				usbnvt_set_ep0_done(nvt680);
				break;
			}

			usbnvt_clr_ep_stall(nvt680, ep->epnum);

			if (ep->stall) {
				printk("nvt680_clear_feature nvt680_set_halt_and_wedge\r\n");
				nvt680_set_halt_and_wedge(&ep->ep, 0, 0);
			}
		}

		usbnvt_set_ep0_done(nvt680);

		break;
	default:
		usbnvt_set_ep_stall(nvt680, 0);
		break;
	}
}

static void nvt680_set_feature(struct nvt680_udc *nvt680, struct usb_ctrlrequest *ctrl)
{
	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		usbnvt_set_ep0_done(nvt680);
		break;

	case USB_RECIP_INTERFACE:
		usbnvt_set_ep0_done(nvt680);
		break;

	case USB_RECIP_ENDPOINT: {
		u8 epnum;

		epnum = le16_to_cpu(ctrl->wIndex) & USB_ENDPOINT_NUMBER_MASK;

		usbnvt_set_ep_stall(nvt680, epnum);

		usbnvt_set_ep0_done(nvt680);
	} break;

	default:
		usbnvt_set_ep_stall(nvt680, 0);
		break;
	}
}

static void nvt680_set_fifo_cfg(struct nvt680_ep *ep, u32 epnum, u32 dir_in,u32 type, u32 mps)
{
	struct nvt680_udc *nvt680 = ep->nvt680;
	T_USB_DEVBUFCFG0_REG	reg_devbufcfg0;
	T_USB_DEVMAXPS_EP_REG 	DevEPMaxPS;
	unsigned long flags;

	spin_lock_irqsave(&nvt680->lock, flags);

	DevEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((epnum - 1) << 2));
	DevEPMaxPS.bit.MAXPS_EP = mps;

	if (ep->type == USB_ENDPOINT_XFER_ISOC) {
		if ((ep->ep.maxpacket == 0x400)&&(ep->ep.mult == 1))
			DevEPMaxPS.bit.TX_NUM_HBW_EP = 2;
		else
			DevEPMaxPS.bit.TX_NUM_HBW_EP = 1;

	} else {
		DevEPMaxPS.bit.TX_NUM_HBW_EP = 0;
	}

	if (dir_in > 0) {
		DevEPMaxPS.bit.DIR_EP = 0;
		DevEPMaxPS.bit.FNO_IEP = nvt680->ep_2_fifo[epnum];
	} else {
		DevEPMaxPS.bit.DIR_EP = 1;
		DevEPMaxPS.bit.FNO_OEP = nvt680->ep_2_fifo[epnum];
	}
	USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((epnum - 1) << 2)), DevEPMaxPS.reg);

	{
		u8 startfifo,regofs;

		startfifo = nvt680->ep_2_fifo[epnum];
		if(startfifo < 4) {
			regofs = 0;
		} else {
			regofs = 4;
			startfifo -= 4;
		}

		reg_devbufcfg0.reg = USB_GETREG(USB_DEVBUFCFG0_REG_OFS+regofs);

		if(mps <= 512) {
			reg_devbufcfg0.reg &= ~(0xFF << (startfifo<<3));
			reg_devbufcfg0.reg |= (type << (startfifo<<3));//buf_type
			reg_devbufcfg0.reg |= (0x0    << (startfifo<<3));//buf_blkno
			reg_devbufcfg0.reg |= (0    << (startfifo<<3));//buf_blksz
			if(dir_in > 0)
				reg_devbufcfg0.reg |= (0x20<< (startfifo<<3));//dir_fifo

			reg_devbufcfg0.reg |= (0x80 << (startfifo<<3));//buf_en

		} else if ((ep->ep.maxpacket == 0x400)&&(ep->ep.mult == 1)&&(ep->type == USB_ENDPOINT_XFER_ISOC)&&(regofs==4)) {
			reg_devbufcfg0.reg = 0x353535B5;
		} else {

			reg_devbufcfg0.reg &= ~(0xFFFF << (startfifo<<3));
			reg_devbufcfg0.reg |= (type << (startfifo<<3));//buf_type
			reg_devbufcfg0.reg |= (0    << (startfifo<<3));//buf_blkno
			reg_devbufcfg0.reg |= (0x10 << (startfifo<<3));//buf_blksz
			if(dir_in > 0)
				reg_devbufcfg0.reg |= (0x20<< (startfifo<<3));//dir_fifo

			reg_devbufcfg0.reg |= (0x80 << (startfifo<<3));//buf_en

			reg_devbufcfg0.reg |= (type << ((startfifo<<3)+8));//buf_type
			reg_devbufcfg0.reg |= (0    << ((startfifo<<3)+8));//buf_blkno
			reg_devbufcfg0.reg |= (0x10 << ((startfifo<<3)+8));//buf_blksz
			if(dir_in > 0)
				reg_devbufcfg0.reg |= (0x20<< ((startfifo<<3)+8));//dir_fifo
		}

		USB_SETREG(USB_DEVBUFCFG0_REG_OFS+regofs, reg_devbufcfg0.reg);
	}

	spin_unlock_irqrestore(&nvt680->lock, flags);

}

static int nvt680_config_ep(struct nvt680_ep *ep, const struct usb_endpoint_descriptor *desc)
{
	struct nvt680_udc *nvt680 = ep->nvt680;

	if ((ep->ep.mult == 1) && (ep->type == USB_ENDPOINT_XFER_ISOC) && (ep->ep.maxpacket == 0x400)) {
		if ((nvt680->fifo_vld_msk & 0x0F) == 0x0F) {

			nvt680->ep_2_fifo[ep->epnum]		= 0;
			nvt680->fifo_2_ep[nvt680->ep_2_fifo[ep->epnum]] = ep->epnum;
			nvt680->fifo_vld_msk &= 0xF0;
		} else {
			pr_err("FIFO 4-7 for isoc-in 2048 not available\n");
			return -ENOPKG;
		}
#if 1
	} else if (ep->type == USB_ENDPOINT_XFER_ISOC) {
		u8 i,chkmsk;

		if (ep->ep.mult) {
			pr_err("mult is supported!\n");
		}

		if (ep->ep.maxpacket > 512)
			chkmsk = 0x3;
		else
			chkmsk = 0x1;

		for (i=4; i < 8; i+=2) {
			if (((nvt680->fifo_vld_msk>>i) & chkmsk) == chkmsk) {
				nvt680->ep_2_fifo[ep->epnum]		= i;
				nvt680->fifo_2_ep[nvt680->ep_2_fifo[ep->epnum]] = ep->epnum;
				nvt680->fifo_vld_msk = nvt680->fifo_vld_msk & ~(chkmsk << i);
				break;
			}

			if (i==6) {
				pr_err("FIFO search not available\n");
				return -ENOPKG;
			}
		}
#endif
	} else {
		u8 i,chkmsk;

		if (ep->ep.mult) {
			pr_err("mult is supported!\n");
		}

		if (ep->ep.maxpacket > 512)
			chkmsk = 0x3;
		else
			chkmsk = 0x1;

		for (i=4; i < 8; i++) {
			if (((nvt680->fifo_vld_msk>>i) & chkmsk) == chkmsk) {
				nvt680->ep_2_fifo[ep->epnum]		= i;
				nvt680->fifo_2_ep[nvt680->ep_2_fifo[ep->epnum]] = ep->epnum;
				nvt680->fifo_vld_msk = nvt680->fifo_vld_msk & ~(chkmsk << i);
				break;
			}

			//if (i==7) {
			//	pr_err("FIFO search not available\n");
			//	return -ENOPKG;
			//}
		}

		if (i==8) {
			for (i=0; i < 4; i++) {
				if (((nvt680->fifo_vld_msk>>i) & chkmsk) == chkmsk) {
					nvt680->ep_2_fifo[ep->epnum]		= i;
					nvt680->fifo_2_ep[nvt680->ep_2_fifo[ep->epnum]] = ep->epnum;
					nvt680->fifo_vld_msk = nvt680->fifo_vld_msk & ~(chkmsk << i);
					break;
				}

				if (i==3) {
					pr_err("FIFO search not available\n");
					return -ENOPKG;
				}
			}
		}


	}


	nvt680_set_fifo_cfg(ep, ep->epnum, ep->dir_in, ep->type, ep->ep.maxpacket);
	nvt680->ep_en &= ~(0x1<<ep->epnum);
	nvt680->ep[ep->epnum] = ep;

	return 0;
}

void nvt680_enable_ep_transfer(struct nvt680_ep *ep)
{
	struct nvt680_udc *nvt680 = ep->nvt680;

	devnumsg("nvt680_enable_ep_transfer %d\n", ep->epnum);

	usbnvt_unmask_ep_interrupt(nvt680, ep->epnum);
}

void nvt680_stop_ep_transfer(struct nvt680_ep *ep)
{
	struct nvt680_udc *nvt680 = ep->nvt680;

	devnumsg("nvt680_stop_ep_transfer %d\n", ep->epnum);

	usbnvt_mask_ep_interrupt(nvt680, ep->epnum);
}

static int nvt680_ep_release(struct nvt680_ep *ep)
{
	struct nvt680_udc *nvt680 = ep->nvt680;
	T_USB_DEVMAXPS_EP_REG       devEPMaxPS;
	unsigned long flags;

	if (!ep->epnum)
		return 0;

	spin_lock_irqsave(&ep->nvt680->lock, flags);
	devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((ep->epnum - 1) << 2));
	devEPMaxPS.bit.RSTG_EP = 1;
	USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((ep->epnum - 1) << 2)), devEPMaxPS.reg);
	devEPMaxPS.bit.RSTG_EP = 0;
	USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((ep->epnum - 1) << 2)), devEPMaxPS.reg);
	spin_unlock_irqrestore(&ep->nvt680->lock, flags);

	//ep->epnum = 0;
	ep->stall = 0;
	ep->wedged = 0;

#if 1
	if ((ep->ep.mult == 1) && (ep->type == USB_ENDPOINT_XFER_ISOC) && (ep->ep.maxpacket == 0x400)) {
		if ((nvt680->fifo_vld_msk & 0xF0) == 0) {

			//nvt680->ep_2_fifo[ep->epnum]		= 0xF;
			//nvt680->fifo_2_ep[nvt680->ep_2_fifo[ep->epnum]] = ep->epnum;
			nvt680->fifo_vld_msk |= 0xF;
		} else {
			pr_err("FIFO 0-3 for isoc-in 2048 unmap error\n");
		}
	} else {
		u8 chkmsk;

		if (ep->ep.mult) {
			pr_err("mult is supported!\n");
		}

		if (ep->ep.maxpacket > 512)
			chkmsk = 0x3;
		else
			chkmsk = 0x1;

		if (((nvt680->fifo_vld_msk >> nvt680->ep_2_fifo[ep->epnum]) & chkmsk) == 0) {
			//nvt680->ep_2_fifo[ep->epnum]		= 0xF;
			//nvt680->fifo_2_ep[nvt680->ep_2_fifo[ep->epnum]] = ep->epnum;
			nvt680->fifo_vld_msk = nvt680->fifo_vld_msk | (chkmsk << nvt680->ep_2_fifo[ep->epnum]);
		} else {
			pr_err("FIFO unmap error\n");
		}

	}

	nvt680->ep_en |= (0x1<<ep->epnum);

#endif
	return 0;
}


#endif

void usbnvt_set_ep_read(struct nvt680_udc *nvt680, UINT32 EPn, dma_addr_t d_adr, UINT32 length)
{
#if 1
	UINT32                        fifonum;
	UINT32                        uiRegOfs;
	unsigned long				  flags;

	fifonum = nvt680->ep_2_fifo[EPn];

	if(fifonum >= 4) {
		T_USB_DEVDMA2CTRL0_REG        devDMAxCtl;
		T_USB_DEVDMA2CTRL1_REG        devDMAxCtl2;

		uiRegOfs = (fifonum-4) << 3;

		itfnumsg("usbnvt_set_ep_read: EP%d Addr=0x%08X Size=0x%08X\n", EPn, d_adr, length);

		devDMAxCtl.reg = USB_GETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs);
		if (devDMAxCtl.bit.DMA_START == 1) {
			printk("EP%d: DMA-%d not available\r\n", EPn, fifonum);
			return;
		}

		spin_lock_irqsave(&nvt680->lock, flags);

		devDMAxCtl.bit.DMA_TYPE = 0;// FIFO to memory
		devDMAxCtl.bit.DMA_LEN  = length;
		USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

		// set Dma Addr
		devDMAxCtl2.reg = d_adr;
		USB_SETREG(USB_DEVDMA2CTRL1_REG_OFS + uiRegOfs, devDMAxCtl2.reg);

		// start Dma
		devDMAxCtl.bit.DMA_START = 1;
		USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

		spin_unlock_irqrestore(&nvt680->lock, flags);
	}else {
		T_USB_DEVDMACTRL1_REG         devDMACtl1;
		T_USB_DEVDMACTRL2_REG         devDMACtl2;
		T_USB_DEVACCFIFO_REG    	  devAccFifo;

		do {
			devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);

			if(devDMACtl1.bit.DMA_START) {
				cpu_relax();
				udelay(100);
			}
		}while(devDMACtl1.bit.DMA_START);

		// Config DMA direction & length
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_TYPE = 0; // FIFO to memory
		devDMACtl1.bit.DMA_LEN  = length;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		devAccFifo.reg = 0;
		devAccFifo.bit.ACC_F0_3 = fifonum;
		USB_SETREG(USB_DEVACCFIFO_REG_OFS, devAccFifo.reg);

		// set Dma Addr
		devDMACtl2.reg = d_adr;
		USB_SETREG(USB_DEVDMACTRL2_REG_OFS, devDMACtl2.reg);

		// start Dma
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_START = 1;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

	}
#else
	printk("usbnvt_set_ep_read: EP%d Addr=0x%08X Size=0x%08X\n", EPn, d_adr, length);
#endif
}

void usbnvt_set_ep_write(struct nvt680_udc *nvt680, UINT32 EPn, dma_addr_t d_adr, UINT32 length)
{
#if 1
	UINT32                  	  fifonum;
	UINT32                        uiRegOfs;
	unsigned long				  flags;

	fifonum = nvt680->ep_2_fifo[EPn];

	if(fifonum >= 4) {
		T_USB_DEVDMA2CTRL0_REG        devDMAxCtl;
		T_USB_DEVDMA2CTRL1_REG        devDMAxCtl2;

		uiRegOfs = (fifonum-4) << 3;

		itfnumsg("usbnvt_set_ep_write: EP%d Addr=0x%08X Size=0x%08X\n", EPn, d_adr, length);

		//do {
		//	devDMAxCtl.reg = USB_GETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs);
		//} while(devDMAxCtl.bit.DMA_START == 1);
		if (devDMAxCtl.bit.DMA_START == 1) {
			printk("EP%d: DMA-%d not available\r\n", EPn, fifonum);
			return;
		}

		spin_lock_irqsave(&nvt680->lock, flags);

		devDMAxCtl.bit.DMA_TYPE = 1;// memory to FIFO
		devDMAxCtl.bit.DMA_LEN  = length;
		USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

		// set Dma Addr
		devDMAxCtl2.reg = d_adr;
		USB_SETREG(USB_DEVDMA2CTRL1_REG_OFS + uiRegOfs, devDMAxCtl2.reg);

		// start Dma
		devDMAxCtl.bit.DMA_START = 1;
		USB_SETREG(USB_DEVDMA2CTRL0_REG_OFS + uiRegOfs, devDMAxCtl.reg);

		spin_unlock_irqrestore(&nvt680->lock, flags);
	}else {
		T_USB_DEVDMACTRL1_REG         devDMACtl1;
		T_USB_DEVDMACTRL2_REG         devDMACtl2;
		T_USB_DEVACCFIFO_REG    	  devAccFifo;

		do {
			devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);

			if(devDMACtl1.bit.DMA_START) {
				cpu_relax();
				udelay(100);
			}
		}while(devDMACtl1.bit.DMA_START);

		// Config DMA direction & length
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_TYPE = 1; // memory to FIFO
		devDMACtl1.bit.DMA_LEN  = length;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

		devAccFifo.reg = 0;
		devAccFifo.bit.ACC_F0_3 = fifonum;
		USB_SETREG(USB_DEVACCFIFO_REG_OFS, devAccFifo.reg);

		// set Dma Addr
		devDMACtl2.reg = d_adr;
		USB_SETREG(USB_DEVDMACTRL2_REG_OFS, devDMACtl2.reg);

		// start Dma
		devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
		devDMACtl1.bit.DMA_START = 1;
		USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

	}
#else
	printk("usbnvt_set_ep_write: EP%d Addr=0x%08X Size=0x%08X\n", EPn, d_adr, length);
#endif
	return;
}



BOOL usbnvt_get_ep_direction_in(struct nvt680_udc *nvt680, UINT32 EPn)
{
	T_USB_DEVMAXPS_EP_REG   devEPMaxPS;

	devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((EPn - 1) << 2));
	return !devEPMaxPS.bit.DIR_EP;
}

void usbnvt_mask_ep_interrupt(struct nvt680_udc *nvt680, UINT32 EPn)
{
	T_USB_DEVINTMASKGROUP0_REG devIntMskGrp0;
	T_USB_DEVINTMASKGROUP1_REG intGroup1Mask;
	unsigned long flags;

	if (EPn == 0) {
		devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
		devIntMskGrp0.bit.MCX_IN_INT  = 1;
		devIntMskGrp0.bit.MCX_OUT_INT = 1;
		USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);
		return;
	}

	if(nvt680->ep_2_fifo[EPn] == 0xF) {
		printk("usbnvt_unmask_ep_interrupt ERROR!!!!!!!!!!!!!!!!!!!!!!\r\n");
	}

	devnumsg("usbnvt_mask_ep_interrupt %d\n", EPn);

	spin_lock_irqsave(&nvt680->lock, flags);

	if (usbnvt_get_ep_direction_in(nvt680, EPn)) {
		intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
		intGroup1Mask.reg |= (0x1 << (16 + nvt680->ep_2_fifo[EPn]));
		USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, intGroup1Mask.reg);
	} else {
		intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
		intGroup1Mask.reg |= (0x3 << ( nvt680->ep_2_fifo[EPn] << 1));
		USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, intGroup1Mask.reg);
	}

	spin_unlock_irqrestore(&nvt680->lock, flags);

}

void usbnvt_unmask_ep_interrupt(struct nvt680_udc *nvt680, UINT32 EPn)
{
	T_USB_DEVINTMASKGROUP0_REG devIntMskGrp0;
	T_USB_DEVINTMASKGROUP1_REG intGroup1Mask;
	unsigned long flags;

	if (EPn == 0) {
		devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
		devIntMskGrp0.bit.MCX_IN_INT  = 0;
		devIntMskGrp0.bit.MCX_OUT_INT = 0;
		USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);
		return;
	}

	if(nvt680->ep_2_fifo[EPn] == 0xF) {
		printk("usbnvt_unmask_ep_interrupt ERROR!!!!!!!!!!!!!!!!!!!!!!\r\n");
	}


	devnumsg("usbnvt_unmask_ep_interrupt %d\n", EPn);

	spin_lock_irqsave(&nvt680->lock, flags);

	if (usbnvt_get_ep_direction_in(nvt680, EPn)) {
		intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
		intGroup1Mask.reg &= ~(0x1 << (16 +  nvt680->ep_2_fifo[EPn]));
		USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, intGroup1Mask.reg);
	} else {
		intGroup1Mask.reg  = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
		intGroup1Mask.reg &= ~(0x3 << ( nvt680->ep_2_fifo[EPn] << 1));
		USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, intGroup1Mask.reg);
	}

	spin_unlock_irqrestore(&nvt680->lock, flags);

}

BOOL usbnvt_ep_stall_status(struct nvt680_udc *nvt680, UINT32 EPn)
{
	T_USB_DEVMAXPS_EP_REG   devEPMaxPS;

	devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((EPn - 1) << 2));

	return devEPMaxPS.bit.STL_EP;
}

BOOL usbnvt_is_highspeed_enabled(struct nvt680_udc *nvt680)
{
	T_USB_DEVMAINCTRL_REG devMainCtl;

	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	return (BOOL)(devMainCtl.bit.HS_EN);
}

void usbnvt_set_ep0_done(struct nvt680_udc *nvt680)
{
	T_USB_DEVCXCFGFIFOSTATUS_REG    reg_cxcfg;
	unsigned long flags;

	ep0numsg("usbnvt_set_ep0_done\n");

	spin_lock_irqsave(&nvt680->lock, flags);

	reg_cxcfg.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
	reg_cxcfg.bit.CX_DONE = 1;
	USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, reg_cxcfg.reg);

	spin_unlock_irqrestore(&nvt680->lock, flags);
}

void usbnvt_set_ep_stall(struct nvt680_udc *nvt680, UINT32 ep_number)
{
	T_USB_DEVMAXPS_EP_REG           devEPMaxPS;
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;
	unsigned long flags;

	//printk("usbnvt_set_ep_stall %d\r\n", ep_number);

	spin_lock_irqsave(&nvt680->lock, flags);

	if (ep_number == 0) {
		devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
		devCXCFG.bit.CX_STL = 1;
		USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);
	} else if (ep_number <= 15) {
		devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((ep_number - 1) << 2));
		devEPMaxPS.bit.STL_EP = 1;
		USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((ep_number - 1) << 2)), devEPMaxPS.reg);
	}

	spin_unlock_irqrestore(&nvt680->lock, flags);
}

void usbnvt_clr_ep_stall(struct nvt680_udc *nvt680, UINT32 EPn)
{
	T_USB_DEVMAXPS_EP_REG   devEPMaxPS;
	unsigned long flags;

	spin_lock_irqsave(&nvt680->lock, flags);
	devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((EPn - 1) << 2));
	devEPMaxPS.bit.STL_EP = 0;
	USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((EPn - 1) << 2)), devEPMaxPS.reg);
	spin_unlock_irqrestore(&nvt680->lock, flags);
}

void usbnvt_set_id(struct nvt680_udc *nvt680, BOOL device)
{
	T_USB_TOP_REG               RegTop;

	RegTop.reg = USB_GETREG(USB_TOP_REG_OFS);
	RegTop.bit.ID    = device;
	USB_SETREG(USB_TOP_REG_OFS, RegTop.reg);
}

static void usbnvt_enable_highspeed(struct nvt680_udc *nvt680, BOOL highspeed)
{
	T_USB_DEVMAINCTRL_REG devMainCtl;

	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);

	if (highspeed) {
		devMainCtl.bit.FORCE_FS = 0; // bit[9]
	} else {
		devMainCtl.bit.FORCE_FS = 1; // bit[9]
	}

	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);
}

void usbnvt_clr_fifo(struct nvt680_udc *nvt680)
{
	T_USB_DEVTEST_REG devTest;

	devTest.reg = USB_GETREG(USB_DEVTEST_REG_OFS);
	devTest.bit.TST_CLRFF = 1;
	USB_SETREG(USB_DEVTEST_REG_OFS, devTest.reg);
}

void usbnvt_power_on_init(struct nvt680_udc *nvt680)
{
	T_USB_DEVDMACTRL0_REG       devDMACtl0;
	T_USB_DEVDMACTRL1_REG       devDMACtl1;
	T_USB_HCMISC_REG            hcMiscReg;

	itfnumsg("%s: enter\r\n", __func__);

	//default set to device mode
	usbnvt_set_id(nvt680, TRUE);

	//It is possible that loader has turn off the USB physical layer.
	//So we should turn on the usb physical layer.
	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	devDMACtl1.bit.DEVPHY_SUSPEND = 0;
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

	usbnvt_enable_highspeed(nvt680, TRUE);

	// Bug fix for host resume pulse
	hcMiscReg.reg = USB_GETREG(USB_HCMISC_REG_OFS);
	hcMiscReg.bit.RESM_SE0_CNT = 0x22;
	USB_SETREG(USB_HCMISC_REG_OFS, hcMiscReg.reg);

	// set DMA access SDRAM
	devDMACtl0.reg = USB_GETREG(USB_DEVDMACTRL0_REG_OFS);
	devDMACtl0.bit.USB_ACCESS_SELECT = 0;
	USB_SETREG(USB_DEVDMACTRL0_REG_OFS, devDMACtl0.reg);

	// Clear FIFOs
	usbnvt_clr_fifo(nvt680);

	//USB Device is unattached.
	//Disable USB physcial layer. IT will auto re-function when USB plug-in
	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	devDMACtl1.bit.DEVPHY_SUSPEND   = 1;
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);

	{
		T_USB_DCD_COUNTER_REG   RegDcdCnt;

		RegDcdCnt.reg   = USB_GETREG(USB_DCD_COUNTER_REG_OFS);
		RegDcdCnt.bit.DCD_COUNT = 2;
		USB_SETREG(USB_DCD_COUNTER_REG_OFS, RegDcdCnt.reg);
	}

	{
		UINT16 data=0;
		INT32 result=0;
		UINT32 temp;
		UINT8 u2_trim_swctrl=6, u2_trim_sqsel=4, u2_trim_resint=8;


		#define U2PHY_SETREG(ofs,value)	writel((value), (volatile void __iomem *)(0xFD601000+((ofs)<<2)))
		#define U2PHY_GETREG(ofs)	readl((volatile void __iomem *)(0xFD601000+((ofs)<<2)))

		result= efuse_readParamOps(EFUSE_USBC_TRIM_DATA, &data);
		if(result == 0) {
			u2_trim_swctrl = data&0x7;
			u2_trim_sqsel  = (data>>3)&0x7;
			u2_trim_resint = (data>>6)&0x1F;
		}

		U2PHY_SETREG(0x51, 0x20);
		U2PHY_SETREG(0x50, 0x30);

		temp = U2PHY_GETREG(0x06);
		temp &= ~(0x7<<1);
		temp |= (u2_trim_swctrl<<1);
		U2PHY_SETREG(0x06, temp);

		temp = U2PHY_GETREG(0x05);
		temp &= ~(0x7<<2);
		temp |= (u2_trim_sqsel<<2);
		U2PHY_SETREG(0x05, temp);

		U2PHY_SETREG(0x52, 0x60+u2_trim_resint);
		U2PHY_SETREG(0x51, 0x00);

		writel(0x100+u2_trim_resint, (volatile unsigned long *)(0xFD600000+0x30C));
	}

	itfnumsg("%s: exit\r\n", __func__);

}

void usbnvt_init_controller(struct nvt680_udc *nvt680)
{
	T_USB_DEVADDR_REG           devAddr;
	T_USB_PHYTSTSELECT_REG      devPhyTest;
	T_USB_DEVIDLECNT_REG        devIdleCnt;
	T_USB_DEVMAINCTRL_REG       devMainCtl;
	T_USB_DEVTEST_REG           devTest;
	T_USB_GLOBALINTMASK_REG     glbIntMsk;
	T_USB_DEVINTMASKGROUP0_REG  devIntMskGrp0;
	T_USB_DEVINTMASKGROUP2_REG  devIntMskGrp2;
	T_USB_OTGCTRLSTATUS_REG     otgCtrlSts;
	T_USB_DEVDMACTRL1_REG       devDMACtl1;
	T_USB_DEVDMACTRL1_REG       devDMACtl1_bak;
	T_USB_TOP_REG               RegTop;

	RegTop.reg = USB_GETREG(USB_TOP_REG_OFS);
	RegTop.bit.VBUSI = 1;
	USB_SETREG(USB_TOP_REG_OFS, RegTop.reg);

	// decrease VBUS debounce time from 540us to 200us
	otgCtrlSts.reg = USB_GETREG(USB_OTGCTRLSTATUS_REG_OFS);
	otgCtrlSts.bit.Vbus_FLT_SEL = 1;
	USB_SETREG(USB_OTGCTRLSTATUS_REG_OFS, otgCtrlSts.reg);

	//turn on the usb physical layer.
	devDMACtl1.reg = USB_GETREG(USB_DEVDMACTRL1_REG_OFS);
	devDMACtl1_bak.reg = devDMACtl1.reg;
	devDMACtl1.bit.DEVPHY_SUSPEND = 0;
	USB_SETREG(USB_DEVDMACTRL1_REG_OFS, devDMACtl1.reg);
	if (devDMACtl1_bak.bit.DEVPHY_SUSPEND == 1) {
		// wait VBUS debounce time passed
		udelay(200);
	}

	//set USB Device in unconfigure state
	devAddr.reg          = USB_GETREG(USB_DEVADDR_REG_OFS);
	devAddr.bit.AFT_CONF = 0;
	USB_SETREG(USB_DEVADDR_REG_OFS, devAddr.reg);

	// chip enable
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	devMainCtl.bit.CHIP_EN    = 1;
	devMainCtl.bit.HALF_SPEED = 0;

	// Fix printer connect fail if test mode is enabled
	devMainCtl.bit.RESET_DEBOUNCE_INTERVAL = 5; // set debounce time to 5ms
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

	// suspend counter
	devIdleCnt.reg = USB_GETREG(USB_DEVIDLECNT_REG_OFS);
	devIdleCnt.bit.IDLE_CNT = 7;
	USB_SETREG(USB_DEVIDLECNT_REG_OFS, devIdleCnt.reg);

	// clear interrupt
	USB_SETREG(USB_DEVINTGROUP2_REG_OFS, 0xFFFFFFFF);

	// disable all fifo interrupt
	USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, 0xFFFFFF);

	// soft reset
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	devMainCtl.bit.SFRST = 1;
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);
	// soft reset clear
	devMainCtl.bit.SFRST = 0;
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

	// clear all fifo
	devTest.reg = USB_GETREG(USB_DEVTEST_REG_OFS);
	devTest.bit.TST_CLRFF = 1;
	USB_SETREG(USB_DEVTEST_REG_OFS, devTest.reg);

	// disable Host & OTG interrupt
	glbIntMsk.reg = USB_GETREG(USB_GLOBALINTMASK_REG_OFS);
	glbIntMsk.bit.MOTG_INT = 1;
	glbIntMsk.bit.MHC_INT  = 1;
	USB_SETREG(USB_GLOBALINTMASK_REG_OFS, glbIntMsk.reg);

	// maks CX_IN/CX_OUT/CX_COMEND interrupt
	devIntMskGrp0.reg = USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS);
	devIntMskGrp0.bit.MCX_IN_INT  = 1;
	devIntMskGrp0.bit.MCX_OUT_INT = 1;
	devIntMskGrp0.bit.MCX_COMEND  = 1;
	USB_SETREG(USB_DEVINTMASKGROUP0_REG_OFS, devIntMskGrp0.reg);

	// mask DEV_IDLE interrupt
	devIntMskGrp2.reg = USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS);
	devIntMskGrp2.bit.MDEV_IDLE             = 1;
	devIntMskGrp2.bit.MDEV_WAKEUP_BYVBUS    = 1;
	USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, devIntMskGrp2.reg);

	// enable global interrupt
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	devMainCtl.bit.GLINT_EN = 1;
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

	devPhyTest.reg = USB_GETREG(USB_PHYTSTSELECT_REG_OFS);
	devPhyTest.bit.UNPLUG = 1;
	USB_SETREG(USB_PHYTSTSELECT_REG_OFS, devPhyTest.reg);
}

static void usbnvt_global_interrupt_enable(struct nvt680_udc *nvt680, BOOL enable)
{
	T_USB_DEVMAINCTRL_REG	reg_devmainctrl;
	unsigned long flags;

	spin_lock_irqsave(&nvt680->lock, flags);
	reg_devmainctrl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	reg_devmainctrl.bit.GLINT_EN = enable;
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, reg_devmainctrl.reg);
	spin_unlock_irqrestore(&nvt680->lock, flags);
}


static void usbnvt_intr_g2_busreset(struct nvt680_udc *nvt680)
{
	T_USB_DEVINTMASKGROUP1_REG      devIntMskGrp1;
	T_USB_DEVTEST_REG               devTest;
	T_USB_DEVCXCFGFIFOSTATUS_REG    devCXCFG;
	T_USB_DEVMAINCTRL_REG           devMainCtl;
	T_USB_DEVFIFOBYTECNT0_REG       devFIFOByteCnt;
	UINT32                          i;
	unsigned long flags;

	ep0numsg("usbintr G2: bus-reset\n");

	spin_lock_irqsave(&nvt680->lock, flags);

	//Set USB device Addr=0 & not in configured state
	USB_SETREG(USB_DEVADDR_REG_OFS, 0);

	// disable FIFO0 IN int // TODO Review
	devIntMskGrp1.reg = USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS);
	devIntMskGrp1.bit.MF0_IN_INT = 1;  // bit[16]
	USB_SETREG(USB_DEVINTMASKGROUP1_REG_OFS, devIntMskGrp1.reg);

	// Clear All FIFO
	devTest.reg = USB_GETREG(USB_DEVTEST_REG_OFS);
	devTest.bit.TST_CLRFF = 1; // bit[0]
	USB_SETREG(USB_DEVTEST_REG_OFS, devTest.reg);

	for (i = 0; i < 4; i++) {
		devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFOBYTECNT0_REG_OFS + (i << 2));
		devFIFOByteCnt.bit.FFRST0 = 1;
		devFIFOByteCnt.bit.FFRST1 = 1;
		USB_SETREG(USB_DEVFIFOBYTECNT0_REG_OFS, devFIFOByteCnt.reg + (i << 2));
	}

	// clear EP0 Stall
	devCXCFG.reg = USB_GETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS);
	devCXCFG.bit.CX_STL = 0; // bit[2]
	USB_SETREG(USB_DEVCXCFGFIFOSTATUS_REG_OFS, devCXCFG.reg);

	// In 2.0 spec page255, the remote wakeup should be disabled after bus reset.
	devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	devMainCtl.bit.CAP_RMWAKUP = 0;
	USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);

	spin_unlock_irqrestore(&nvt680->lock, flags);

}

static void usbnvt_intr_g2_suspend(struct nvt680_udc *nvt680)
{
	T_USB_DEVFIFOBYTECNT0_REG   devFIFOByteCnt;
	//T_USB_DEVMAINCTRL_REG       devMainCtl;
	UINT32                      i;
	unsigned long flags;

	ep0numsg("usbintr G2: suspend\n");

	spin_lock_irqsave(&nvt680->lock, flags);

	//Clear all USB FIFOs
	for (i = 0; i < 4; i++) {
		devFIFOByteCnt.reg = USB_GETREG(USB_DEVFIFOBYTECNT0_REG_OFS + (i << 2));
		devFIFOByteCnt.bit.FFRST0 = 1;
		devFIFOByteCnt.bit.FFRST1 = 1;
		USB_SETREG(USB_DEVFIFOBYTECNT0_REG_OFS, devFIFOByteCnt.reg + (i << 2));
	}

	//*****************************************
	//Defualt mode to enter suspend.
	//devMainCtl.reg = USB_GETREG(USB_DEVMAINCTRL_REG_OFS);
	//devMainCtl.bit.GOSUSP = 1;
	//USB_SETREG(USB_DEVMAINCTRL_REG_OFS, devMainCtl.reg);
	//*****************************************

	spin_unlock_irqrestore(&nvt680->lock, flags);

}


static void usbnvt_intr_g0_setup(struct nvt680_udc *nvt680)
{
	struct usb_ctrlrequest ctrl;
	UINT32 *pControlData = (UINT32 *)(&ctrl);
	unsigned long flags;

	*pControlData++ = USB_GETREG(USB_DEVDMACTRL3_REG_OFS);
	*pControlData   = USB_GETREG(USB_DEVDMACTRL3_REG_OFS);

	ep0numsg("usbintr G0: CX_SETUP_INT 0x%02X 0x%02X 0x%04X 0x%04X 0x%04X\n"
		, ctrl.bRequestType, ctrl.bRequest, ctrl.wValue, ctrl.wIndex, ctrl.wLength);


	nvt680->ep[0]->dir_in = ctrl.bRequestType & USB_DIR_IN;
	nvt680->gadget.speed = USB_GETREG(USB_DEVMAINCTRL_REG_OFS) & (0x1<<6) ? USB_SPEED_HIGH : USB_SPEED_FULL;

	/*
		Handle SETUP
	*/

	if ((ctrl.bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
		switch (ctrl.bRequest) {
		case USB_REQ_GET_STATUS:
			nvt680_get_status(nvt680, &ctrl);
			ep0numsg("USB_REQ_GET_STATUS\n");
			break;
		case USB_REQ_CLEAR_FEATURE:
			nvt680_clear_feature(nvt680, &ctrl);
			ep0numsg("USB_REQ_CLEAR_FEATURE\n");
			break;
		case USB_REQ_SET_FEATURE:
			nvt680_set_feature(nvt680, &ctrl);
			ep0numsg("USB_REQ_SET_FEATURE\n");
			break;
		case USB_REQ_SET_ADDRESS: {
			T_USB_DEVADDR_REG   reg_dev_addr;

			ep0numsg("USB_REQ_SET_ADDRESS\n");

			spin_lock_irqsave(&nvt680->lock, flags);

			reg_dev_addr.reg = USB_GETREG(USB_DEVADDR_REG_OFS);
			reg_dev_addr.bit.DEVADR = ctrl.wValue & 0x7F;
			USB_SETREG(USB_DEVADDR_REG_OFS, reg_dev_addr.reg);

			spin_unlock_irqrestore(&nvt680->lock, flags);

			usbnvt_set_ep0_done(nvt680);
		} break;

		case USB_REQ_SET_CONFIGURATION: {
			T_USB_DEVADDR_REG   devAddr;

			spin_lock_irqsave(&nvt680->lock, flags);

			if (ctrl.wValue == 0) {
				devAddr.reg = USB_GETREG(USB_DEVADDR_REG_OFS);
				devAddr.bit.AFT_CONF = 0;
				USB_SETREG(USB_DEVADDR_REG_OFS, devAddr.reg);
			} else {
				UINT32                      EPn;
				T_USB_DEVMAXPS_EP_REG       devEPMaxPS;

				// Clear All EPx & FIFOx map register
				USB_SETREG(USB_DEVMAXPS_EP1_REG_OFS,    0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP2_REG_OFS,    0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP3_REG_OFS,    0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP4_REG_OFS,    0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP5_REG_OFS,    0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP6_REG_OFS,    0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP7_REG_OFS,    0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP8_REG_OFS,    0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP9_REG_OFS,    0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP10_REG_OFS,   0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP11_REG_OFS,   0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP12_REG_OFS,   0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP13_REG_OFS,   0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP14_REG_OFS,   0x88000200);
				USB_SETREG(USB_DEVMAXPS_EP15_REG_OFS,   0x88000200);

				USB_SETREG(USB_DEVBUFCFG0_REG_OFS,      0);
				USB_SETREG(USB_DEVBUFCFG1_REG_OFS,      0);

				usbnvt_clr_fifo(nvt680);

				for (EPn = 1; EPn <= 15; EPn++) {
					// Reset Toggle sequence for IN EP
					devEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((EPn - 1) << 2));
					devEPMaxPS.bit.RSTG_EP = 1;
					USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((EPn - 1) << 2)), devEPMaxPS.reg);
					devEPMaxPS.bit.RSTG_EP = 0;
					USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((EPn - 1) << 2)), devEPMaxPS.reg);
				}

				devAddr.reg = USB_GETREG(USB_DEVADDR_REG_OFS);
				devAddr.bit.AFT_CONF = 1;
				USB_SETREG(USB_DEVADDR_REG_OFS, devAddr.reg);

				if (usbnvt_is_highspeed_enabled(nvt680)) {
					USB_SETREG(USB_DEVSOFTIMERMASK_REG_OFS, 0x44c);
				} else {
					USB_SETREG(USB_DEVSOFTIMERMASK_REG_OFS, 0x2710);
				}

			}

			spin_unlock_irqrestore(&nvt680->lock, flags);

			// The ep enable would be invoked here
			if (nvt680->driver->setup(&nvt680->gadget, &ctrl) < 0) {
				usbnvt_set_ep_stall(nvt680, 0);
				ep0numsg("SET STALL\n");
			}

			//else {
			//	Must not set done here. because gadget would send 0 byte ep0.
			//	The done is set induced by that 0byte request.
			//	usbnvt_set_ep0_done(nvt680);
			//}

			ep0numsg("USB_REQ_SET_CONFIGURATION\n");
		} break;


		default: {
				if (nvt680->driver->setup(&nvt680->gadget, &ctrl) < 0) {
					usbnvt_set_ep_stall(nvt680, 0);
					ep0numsg("SET STALL\n");
				}
		}
		break;

		}
	} else {
		if (nvt680->driver->setup(&nvt680->gadget, &ctrl) < 0) {
			usbnvt_set_ep_stall(nvt680, 0);
			ep0numsg("SET STALL\n");
		}

	}


}

static void usbnvt_intr_g0_cx_in(struct nvt680_udc *nvt680)
{
	struct nvt680_ep *ep = nvt680->ep[0];

	ep0numsg("nvt680_ep0in\n");

	if ((!list_empty(&ep->queue)) && (ep->dir_in)) {
		struct nvt680_request *req;

		req = list_entry(ep->queue.next, struct nvt680_request, queue);

		if (req->req.length) {
			nvt680_start_ep0_data(ep, req);
		}

		if ((req->req.length == req->req.actual) || (req->req.actual < ep->ep.maxpacket)) {

			nvt680_done(ep, req, 0);
			usbnvt_set_ep0_done(nvt680);
		}

	} else {
		usbnvt_set_ep0_done(nvt680);
	}
}

static void usbnvt_intr_g0_cx_out(struct nvt680_udc *nvt680)
{
	struct nvt680_ep *ep = nvt680->ep[0];

	ep0numsg("nvt680_ep0out\n");

	if (!list_empty(&ep->queue) && !ep->dir_in) {
		struct nvt680_request *req;

		req = list_first_entry(&ep->queue, struct nvt680_request, queue);

		if (req->req.length) {
			nvt680_start_ep0_data(ep, req);
		}

		if ((req->req.length == req->req.actual) || (req->req.actual < ep->ep.maxpacket)) {
			nvt680_done(ep, req, 0);
			usbnvt_set_ep0_done(nvt680);
		}

	} else {
		numsg("%s : empty queue\n", __func__);
	}
}


static void usbnvt_intr_g0_cx_abort(struct nvt680_udc *nvt680)
{
	T_USB_DEVINTGROUP0_REG          devIntGrp0;
	unsigned long flags;

	ep0numsg("nvt680_cx_abort\n");

	spin_lock_irqsave(&nvt680->lock, flags);

	devIntGrp0.reg = USB_GETREG(USB_DEVINTGROUP0_REG_OFS);
	devIntGrp0.bit.CX_COMABORT_INT = 0;
	USB_SETREG(USB_DEVINTGROUP0_REG_OFS, devIntGrp0.reg);

	spin_unlock_irqrestore(&nvt680->lock, flags);
}


#endif
////////////////////////////////////////////////////////////////////////////////////////////


#if 1
static irqreturn_t nvt680_interrupt(int irq, void *_nvt680)
{
	struct nvt680_udc *nvt680 = _nvt680;

	//intrmsg("nvt680_interrupt\n");

	usbnvt_global_interrupt_enable(nvt680, FALSE);
	return IRQ_WAKE_THREAD;
}

static irqreturn_t nvt680_thread_interrupt(int irq, void *_nvt680)
{
	struct nvt680_udc *nvt680 = _nvt680;
	T_USB_DEVINTGROUP_REG 		devIntGrp;
	T_USB_DEVINTGROUP0_REG		devIntGrp0;
	T_USB_DEVINTGROUP1_REG		devIntGrp1;
	T_USB_DEVINTGROUP2_REG		devIntGrp2;
	unsigned long flags, fifono;

	devIntGrp.reg = USB_GETREG(USB_DEVINTGROUP_REG_OFS) & (~USB_GETREG(USB_DEVINTGROUPMASK_REG_OFS));

	if (devIntGrp.bit.INT_G2) {

		//USB_SETREG(USB_DEVINTMASKGROUP2_REG_OFS, (((0xF<<(4+7))+0x30000)));
		devIntGrp2.reg = USB_GETREG(USB_DEVINTGROUP2_REG_OFS) & ( ~USB_GETREG(USB_DEVINTMASKGROUP2_REG_OFS));
		USB_SETREG(USB_DEVINTGROUP2_REG_OFS, devIntGrp2.reg);

		//intrmsg("usbintr G2: 0x%08X\n", devIntGrp2.reg);

		if(devIntGrp2.bit.USBRST_INT) {
			usbnvt_intr_g2_busreset(nvt680);
		}

		if(devIntGrp2.bit.SUSP_INT) {
			usbnvt_intr_g2_suspend(nvt680);
		}

		if(devIntGrp2.bit.RESM_INT) {
			ep0numsg("usbintr G2: resume\n");
		}

		// DMA-1
		for(fifono=0; fifono < 1; fifono++) {
			if(devIntGrp2.reg & (0x1 << (fifono+7))) {
				struct nvt680_ep *ep;
				struct nvt680_request *req;
				int epnum;

				fifono = USB_GETREG(USB_DEVACCFIFO_REG_OFS);
				epnum = nvt680->fifo_2_ep[fifono];

				if(epnum == 0xF) {
					pr_err("*************fifo_2_ep cmplt mapping_error %d 0x%08X****************\n", (int)fifono, devIntGrp2.reg);
				}

				ep = nvt680->ep[epnum];

				intrmsg("usbintr G2: DMA%d_CMPLT\n", (int)fifono);

				spin_lock_irqsave(&nvt680->lock, flags);
				if(ep->d_adr && ep->d_len) {
					dma_unmap_single(NULL, ep->d_adr, ep->d_len, ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
					ep->d_adr = 0;
					ep->d_len = 0;
				}
				spin_unlock_irqrestore(&nvt680->lock, flags);

				//printk("&&&& %d %d\n\n",fifono,list_empty(&ep->queue));
				if(!list_empty(&ep->queue)) {
					req = list_entry(ep->queue.next, struct nvt680_request, queue);
					nvt680_done(ep, req, 0);
				}
			}
		}

		// DMA-2~5
		for(fifono=4; fifono < 8; fifono++) {
			if(devIntGrp2.reg & (0x1 << (fifono+5))) {
				struct nvt680_ep *ep;
				struct nvt680_request *req;
				int epnum = nvt680->fifo_2_ep[fifono];

				if(epnum == 0xF) {
					pr_err("*************fifo_2_ep cmplt mapping_error %d 0x%08X****************\n", (int)fifono, devIntGrp2.reg);
				}

				ep = nvt680->ep[epnum];

				intrmsg("usbintr G2: DMA%d_CMPLT\n", (int)fifono-2);

				spin_lock_irqsave(&nvt680->lock, flags);
				if(ep->d_adr && ep->d_len) {
					dma_unmap_single(NULL, ep->d_adr, ep->d_len, ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
					ep->d_adr = 0;
					ep->d_len = 0;
				}
				spin_unlock_irqrestore(&nvt680->lock, flags);

				//printk("&&&& %d %d\n\n",fifono,list_empty(&ep->queue));
				if(!list_empty(&ep->queue)) {
					req = list_entry(ep->queue.next, struct nvt680_request, queue);
					nvt680_done(ep, req, 0);
				}
			}
		}


		if(devIntGrp2.bit.SEQ_ERR_INT) {
			intrmsg("usbintr G2: SEQ_ERR_INT\n");
		}

		if(devIntGrp2.bit.SEQ_ABORT_INT) {
			intrmsg("usbintr G2: SEQ_ABORT_INT\n");
		}

		if(devIntGrp2.bit.TX0BYTE_INT) {
			intrmsg("usbintr G2: TX0BYTE_INT\n");
			USB_SETREG(USB_DEVTX0BYTE_REG_OFS, 0);
		}

		if(devIntGrp2.bit.RX0BYTE_INT) {
			intrmsg("usbintr G2: RX0BYTE_INT\n");
			USB_SETREG(USB_DEVRX0BYTE_REG_OFS, 0);
		}

		if(devIntGrp2.bit.DEV_IDLE) {
			intrmsg("usbintr G2: DEV_IDLE\n");
		}
		if(devIntGrp2.bit.DEV_WAKEUP_BYVBUS) {
			intrmsg("usbintr G2: DEV_WAKEUP_BYVBUS\n");
		}

	}

	if (devIntGrp.bit.INT_G0) {
		devIntGrp0.reg = USB_GETREG(USB_DEVINTGROUP0_REG_OFS) & ( ~ USB_GETREG(USB_DEVINTMASKGROUP0_REG_OFS));
		ep0numsg("usbintr G0: 0x%08X\n", devIntGrp0.reg);

		if(devIntGrp0.bit.CX_SETUP_INT) {
			usbnvt_intr_g0_setup(nvt680);
		}

		if(devIntGrp0.bit.CX_IN_INT) {
			usbnvt_intr_g0_cx_in(nvt680);
		}

		if(devIntGrp0.bit.CX_OUT_INT) {
			usbnvt_intr_g0_cx_out(nvt680);
		}

		if(devIntGrp0.bit.CX_COMEND) {
			printk("CX_COMEND ??\n");
		}

		if(devIntGrp0.bit.CX_COMFAIL_INT) {

			spin_lock_irqsave(&nvt680->lock, flags);
			devIntGrp0.reg = USB_GETREG(USB_DEVINTGROUP0_REG_OFS);
			devIntGrp0.bit.CX_COMFAIL_INT = 0;
			USB_SETREG(USB_DEVINTGROUP0_REG_OFS, devIntGrp0.reg);
			spin_unlock_irqrestore(&nvt680->lock, flags);

			usbnvt_set_ep_stall(nvt680, 0);
		}

		if(devIntGrp0.bit.CX_COMABORT_INT) {
			usbnvt_intr_g0_cx_abort(nvt680);
		}

	}

	if (devIntGrp.bit.INT_G1) {
		devIntGrp1.reg = USB_GETREG(USB_DEVINTGROUP1_REG_OFS) & ( ~ USB_GETREG(USB_DEVINTMASKGROUP1_REG_OFS));
		intrmsg("usbintr G1: 0x%08X\n", devIntGrp1.reg);


		/* fifo out */
		for(fifono = 0; fifono < 8; fifono++) {

			// (devIntGrp1.bit.F0_OUT_INT || devIntGrp1.bit.F0_SPK_INT)
			if(devIntGrp1.reg & (0x3 << (fifono<<1))) {
				struct nvt680_ep *ep;
				struct nvt680_request *req;
				dma_addr_t d_adr;
				UINT32 d_len, epnum = nvt680->fifo_2_ep[fifono];
				T_USB_DEVFIFOBYTECNT0_REG	reg_devfifo_bc;

				if(epnum == 0xF) {
					pr_err("fifo_2_ep out mapping_error %d\n", (int)fifono);
				}

				ep = nvt680->ep[epnum];
				req = list_entry(ep->queue.next, struct nvt680_request, queue);

				//reg_devfifo_bc.reg = USB_GETREG(USB_DEVFIFOBYTECNT0_REG_OFS+((epnum-1)<<2));
				//d_len = reg_devfifo_bc.bit.BC_F0;
				reg_devfifo_bc.reg = USB_GETREG(USB_DEVFIFOBYTECNT0_REG_OFS+((nvt680->ep_2_fifo[epnum]>>1)<<2));
				d_len = (reg_devfifo_bc.reg >> ((nvt680->ep_2_fifo[epnum]&0x1)<<4)) & 0x7FF;

				if(!(d_len & 511)) {
					d_len = req->req.length;
				}

				d_adr = dma_map_single(NULL, (u8 *)(req->req.buf), d_len, ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

				if (dma_mapping_error(NULL, d_adr)) {
					pr_err("dma_mapping_error\n");
				}

				dma_sync_single_for_device(NULL, d_adr, d_len, ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

				req->req.actual = d_len;
				ep->d_adr = d_adr;
				ep->d_len = d_len;

				usbnvt_set_ep_read(nvt680, epnum, d_adr, d_len);

				//nvt680_stop_ep_transfer(ep);
				usbnvt_mask_ep_interrupt(nvt680, epnum);

			}

		}


		/* fifo in */
		for(fifono = 0; fifono < 8; fifono++) {

			if(devIntGrp1.reg & (0x1 << (fifono+16))) {
				struct nvt680_ep *ep;
				struct nvt680_request *req;
				dma_addr_t d_adr;
				UINT32 d_len, epnum = nvt680->fifo_2_ep[fifono];

				if(epnum == 0xF) {
					pr_err("fifo_2_ep in mapping_error %d\n", (int)fifono);
				}

				ep = nvt680->ep[epnum];
				req = list_entry(ep->queue.next, struct nvt680_request, queue);

				if ((req->req.length == 0) && (req->req.actual == 0)) {

					T_USB_DEVMAXPS_EP_REG DevEPMaxPS;

					spin_lock_irqsave(&nvt680->lock, flags);
					DevEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((epnum - 1) << 2));
					DevEPMaxPS.bit.TX0BYTE_IEP = 1;
					USB_SETREG((USB_DEVMAXPS_EP1_REG_OFS + ((epnum - 1) << 2)), DevEPMaxPS.reg);
					spin_unlock_irqrestore(&nvt680->lock, flags);

					do {
						DevEPMaxPS.reg = USB_GETREG(USB_DEVMAXPS_EP1_REG_OFS + ((epnum - 1) << 2));
						if(!DevEPMaxPS.bit.TX0BYTE_IEP) {
							break;
						}
						msleep(1);
					}while(DevEPMaxPS.bit.TX0BYTE_IEP);

					nvt680_done(ep, req, 0);
				} else {
					d_len = req->req.length;
					d_adr = dma_map_single(NULL, (u8 *)(req->req.buf), d_len, ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

					if (dma_mapping_error(NULL, d_adr)) {
						pr_err("dma_mapping_error\n");
					}

					dma_sync_single_for_device(NULL, d_adr, d_len, ep->dir_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

					req->req.actual = d_len;
					ep->d_adr = d_adr;
					ep->d_len = d_len;

					usbnvt_set_ep_write(nvt680, epnum, d_adr, d_len);

					//nvt680_stop_ep_transfer(ep);
					usbnvt_mask_ep_interrupt(nvt680, epnum);
				}

			}

		}

	}


	usbnvt_global_interrupt_enable(nvt680, TRUE);
	return IRQ_HANDLED;
}

#endif

static int nvt680_ep_enable(struct usb_ep *_ep, const struct usb_endpoint_descriptor *desc)
{
	struct nvt680_ep *ep;
	#define QH_ISO_MULT(x)		((x >> 11) & 0x03)

	ep = container_of(_ep, struct nvt680_ep, ep);

	ep->desc = desc;
	ep->epnum = usb_endpoint_num(desc);
	ep->type = usb_endpoint_type(desc);
	ep->dir_in = usb_endpoint_dir_in(desc);
	ep->ep.maxpacket = usb_endpoint_maxp(desc)& 0x07ff;
	ep->ep.mult = QH_ISO_MULT(usb_endpoint_maxp(desc));

	itfnumsg("fotg200_ep_enable: ep-desc len=0x%X type=0x%X epaddr=0x%X attr=0x%X MaxPkt=0x%X intval=0x%X mult=%d\n"
		,desc->bLength,desc->bDescriptorType ,desc->bEndpointAddress,desc->bmAttributes,desc->wMaxPacketSize,desc->bInterval, ep->ep.mult);
	//printk("fotg200_ep_enable: ep-desc len=0x%X type=0x%X epaddr=0x%X attr=0x%X MaxPkt=0x%X intval=0x%X mult=%d\n"
	//	,desc->bLength,desc->bDescriptorType ,desc->bEndpointAddress,desc->bmAttributes,desc->wMaxPacketSize,desc->bInterval, ep->ep.mult);

	return nvt680_config_ep(ep, desc);
}

static int nvt680_ep_disable(struct usb_ep *_ep)
{
	struct nvt680_ep *ep;
	struct nvt680_request *req;

	BUG_ON(!_ep);

	itfnumsg("nvt680_ep_disable\n");

	ep = container_of(_ep, struct nvt680_ep, ep);
	//printk("nvt680_ep_disable %d\n",ep->epnum);

	while (!list_empty(&ep->queue)) {
		req = list_entry(ep->queue.next, struct nvt680_request, queue);
		nvt680_done(ep, req, -ESHUTDOWN);
	}

	return nvt680_ep_release(ep);
}

static struct usb_request *nvt680_ep_alloc_request(struct usb_ep *_ep,
						gfp_t gfp_flags)
{
	struct nvt680_request *req;

	itfnumsg("nvt680_ep_alloc_request\n");

	req = kzalloc(sizeof(struct nvt680_request), gfp_flags);
	if (!req)
		return NULL;

	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

static void nvt680_ep_free_request(struct usb_ep *_ep,
					struct usb_request *_req)
{
	struct nvt680_request *req;

	itfnumsg("nvt680_ep_free_request\n");

	req = container_of(_req, struct nvt680_request, req);
	kfree(req);

}

static int nvt680_ep_queue(struct usb_ep *_ep, struct usb_request *_req, gfp_t gfp_flags)
{
	struct nvt680_ep *ep;
	struct nvt680_request *req;
	unsigned long flags;
	int empty_list = 0;

	ep = container_of(_ep, struct nvt680_ep, ep);
	req = container_of(_req, struct nvt680_request, req);

	if(ep->epnum) {
		itfnumsg("nvt680_ep_queue %d\n", ep->epnum);
	} else {
		ep0numsg("nvt680_ep0_queue %d\n", ep->epnum);
	}

	if (ep->nvt680->gadget.speed == USB_SPEED_UNKNOWN) {
		printk("unknown speed\n");
		return -ESHUTDOWN;
	}

	spin_lock_irqsave(&ep->nvt680->lock, flags);

	empty_list = list_empty(&ep->queue);

	list_add_tail(&req->queue, &ep->queue);

	req->req.actual = 0;
	req->req.status = -EINPROGRESS;

	spin_unlock_irqrestore(&ep->nvt680->lock, flags);

	if (!ep->epnum) {
		/* ep0 */
		nvt680_ep0_queue(ep, req);
	} else if (empty_list) {
		nvt680_enable_ep_transfer(ep);
	}

	return 0;
}

static int nvt680_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct nvt680_ep *ep;
	struct nvt680_request *req;

	itfnumsg("nvt680_ep_dequeue\n");

	ep = container_of(_ep, struct nvt680_ep, ep);
	req = container_of(_req, struct nvt680_request, req);

	if (!list_empty(&ep->queue)) {
		nvt680_done(ep, req, -ECONNRESET);
	}

	return 0;
}

static int nvt680_ep_set_halt(struct usb_ep *_ep, int value)
{
	itfnumsg("nvt680_ep_set_halt\n");
	return nvt680_set_halt_and_wedge(_ep, value, 0);
}

static int nvt680_ep_set_wedge(struct usb_ep *_ep)
{
	itfnumsg("nvt680_ep_set_wedge\n");
	return nvt680_set_halt_and_wedge(_ep, 1, 1);
}

static void nvt680_ep_fifo_flush(struct usb_ep *_ep)
{
	itfnumsg("nvt680_ep_fifo_flush\n");
}

static int nvt680_fifo_status(struct usb_ep *ep)
{
	itfnumsg("nvt680_fifo_status\n");
	return 0;
}

static struct usb_ep_ops nvt680_ep_ops = {
	.enable		= nvt680_ep_enable,
	.disable	= nvt680_ep_disable,

	.alloc_request	= nvt680_ep_alloc_request,
	.free_request	= nvt680_ep_free_request,

	.queue		= nvt680_ep_queue,
	.dequeue	= nvt680_ep_dequeue,

	.set_halt	= nvt680_ep_set_halt,
	.fifo_flush	= nvt680_ep_fifo_flush,
	.set_wedge	= nvt680_ep_set_wedge,
	.fifo_status= nvt680_fifo_status,
};

static int nvt680_udc_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	struct nvt680_udc *nvt680 = gadget_to_nvt680(g);
	int i, ret = 0;

	itfnumsg("nvt680_udc_start %d\n", nvt680->irq_no);

	usbnvt_power_on_init(nvt680);
	usbnvt_init_controller(nvt680);

	for (i = 0; i < NVT680_MAX_NUM_EP; i++) {
		nvt680->ep_2_fifo[i]=0xF;
	}

	for (i = 0; i < 8; i++) {
		nvt680->fifo_2_ep[i]=0xF;
	}

	nvt680->fifo_vld_msk = 0xff;
	nvt680->fifo_vld_idx = 0;

#if 1
	ret = request_threaded_irq(nvt680->irq_no, nvt680_interrupt, nvt680_thread_interrupt,
			IRQF_SHARED, udc_name, nvt680);

	if (ret < 0) {
		pr_err("request_irq error (%d)\n", ret);
		goto err_irq;
	}
#endif

	/* hook up the driver */
	driver->driver.bus = NULL;
	nvt680->driver = driver;

	return 0;

err_irq:
	free_irq(nvt680->irq_no, nvt680);

	return ret;

}


static int nvt680_udc_stop(struct usb_gadget *g)
{
	struct nvt680_udc *nvt680 = gadget_to_nvt680(g);
	unsigned long	flags;
	u32 value;

	itfnumsg("nvt680_udc_stop\n");

	spin_lock_irqsave(&nvt680->lock, flags);

	//fotg210_init(fotg210);
	nvt680->driver = NULL;
	//nvt680_enable_unplugsuspend(nvt680);

	/* enable device global interrupt */
	value = ioread32((void __iomem *)0xFD0200A4);
	value &= ~(0x1<<19);
	iowrite32(value, (void __iomem *)0xFD0200A4);
	value |=  (0x1<<19);
	iowrite32(value, (void __iomem *)0xFD0200A4);
	spin_unlock_irqrestore(&nvt680->lock, flags);

	return 0;
}

/**
 * nvt680_udc_pullup - Enable/disable pullup on D+ line.
 * @gadget: USB slave device.
 * @is_on: 0 to disable pullup, 1 to enable.
 *
 * See notes in bcm63xx_select_pullup().
 */
static int nvt680_udc_pullup(struct usb_gadget *gadget, int is_on)
{
	T_USB_PHYTSTSELECT_REG      devPhyTest;
	struct nvt680_udc *nvt680 = gadget_to_nvt680(gadget);

	itfnumsg("nvt680_udc_pullup %d\n", is_on);

	devPhyTest.reg = USB_GETREG(USB_PHYTSTSELECT_REG_OFS);
	devPhyTest.bit.UNPLUG = !is_on;
	USB_SETREG(USB_PHYTSTSELECT_REG_OFS, devPhyTest.reg);

	return 0;
}

static int nvt680_udc_get_frame(struct usb_gadget *g)
{
	itfnumsg("nvt680_udc_get_frame\n");
	return 0;// return 11 bits SOF number
}

static struct usb_gadget_ops nvt680_gadget_ops = {
	.get_frame		= nvt680_udc_get_frame,
	.udc_start		= nvt680_udc_start,
	.udc_stop		= nvt680_udc_stop,
	.pullup			= nvt680_udc_pullup,
};




static int nvt680_udc_remove(struct platform_device *pdev)
{
	struct nvt680_udc *nvt680 = platform_get_drvdata(pdev);

	itfnumsg("nvt680_udc_remove\n");

	usb_del_gadget_udc(&nvt680->gadget);
	free_irq(platform_get_irq(pdev, 0), nvt680);

	nvt680_ep_free_request(&nvt680->ep[0]->ep, nvt680->ep0_req);
	iounmap(nvt680->reg);
	kfree(nvt680);

	{
		struct resource *res;
		struct clk *source_clk;

		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		release_mem_region(res->start, resource_size(res));

		source_clk = clk_get(&pdev->dev, "f0600000.usb20");
		if (IS_ERR(source_clk)) {
			printk("faile to get clock f0600000.usb20\n");
		} else {
			/* toggle reset */
			clk_prepare(source_clk);
			clk_unprepare(source_clk);
			clk_put(source_clk);
		}
	}

	return 0;
}

static int nvt680_udc_probe(struct platform_device *pdev)
{
	struct resource *res, *ires;
	struct nvt680_udc *nvt680 = NULL;
	struct nvt680_ep *_ep[NVT680_MAX_NUM_EP];
	int ret = 0;
	int i;

	itfnumsg("nvt680_udc_probe\n");

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

	/* initialize nvt680_udc data struct */
	nvt680 = kzalloc(sizeof(struct nvt680_udc), GFP_KERNEL);
	if (nvt680 == NULL) {
		pr_err("kzalloc error\n");
		goto err_alloc;
	}

	for (i = 0; i < NVT680_MAX_NUM_EP; i++) {
		_ep[i] = kzalloc(sizeof(struct nvt680_ep), GFP_KERNEL);
		if (_ep[i] == NULL) {
			pr_err("_ep kzalloc error\n");
			goto err_alloc;
		}
		nvt680->ep[i] = _ep[i];

		nvt680->ep_2_fifo[i]=0xF;
	}

	for (i = 0; i < 8; i++) {
		nvt680->fifo_2_ep[i]=0xF;
	}

	if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
		pr_err("failed to request memory resource %s\n", pdev->name);
		goto err_alloc;
	}

	nvt680->reg = ioremap_nocache(res->start, resource_size(res));
	if (nvt680->reg == NULL) {
		pr_err("ioremap error.\n");
		goto err_map;
	}

	spin_lock_init(&nvt680->lock);

	platform_set_drvdata(pdev, nvt680);

	nvt680->gadget.ops = &nvt680_gadget_ops;

	nvt680->gadget.max_speed = USB_SPEED_HIGH;
	nvt680->gadget.dev.parent = &pdev->dev;
	nvt680->gadget.dev.dma_mask = pdev->dev.dma_mask;
	nvt680->gadget.name = udc_name;

	INIT_LIST_HEAD(&nvt680->gadget.ep_list);

	for (i = 0; i < NVT680_MAX_NUM_EP; i++) {
		struct nvt680_ep *ep = nvt680->ep[i];

		if (i) {
			INIT_LIST_HEAD(&nvt680->ep[i]->ep.ep_list);
			list_add_tail(&nvt680->ep[i]->ep.ep_list,
				      &nvt680->gadget.ep_list);
		}
		ep->nvt680 = nvt680;
		INIT_LIST_HEAD(&ep->queue);
		//ep->ep.name = nvt680_ep_name[i];
		ep->ep.name = ep_info[i].name;
		ep->ep.caps = ep_info[i].caps;
		ep->ep.ops = &nvt680_ep_ops;
		usb_ep_set_maxpacket_limit(&ep->ep, (unsigned short) ~0);
	}
	usb_ep_set_maxpacket_limit(&nvt680->ep[0]->ep, 0x40);
	nvt680->gadget.ep0 = &nvt680->ep[0]->ep;
	INIT_LIST_HEAD(&nvt680->gadget.ep0->ep_list);

	nvt680->ep0_req = nvt680_ep_alloc_request(&nvt680->ep[0]->ep,
				GFP_KERNEL);

	if (nvt680->ep0_req == NULL)
		goto err_req;

	usbnvt_power_on_init(nvt680);

	nvt680->irq_no = ires->start;

	ret = usb_add_gadget_udc(&pdev->dev, &nvt680->gadget);
	if (ret)
		goto err_add_udc;

	dev_info(&pdev->dev, "version %s\n", DRIVER_VERSION);

	return 0;

err_add_udc:
err_req:
	nvt680_ep_free_request(&nvt680->ep[0]->ep, nvt680->ep0_req);

err_map:
	if (nvt680->reg)
		iounmap(nvt680->reg);

err_alloc:
	kfree(nvt680);

	return ret;
}


#ifdef CONFIG_OF
static const struct of_device_id of_nvt680_match[] = {
	{
		.compatible = "nvt,fotg200_udc"
	},

	{ },
};
MODULE_DEVICE_TABLE(of, of_nvt680_match);
#endif


static struct platform_driver nvt680_driver = {
	.driver		= {
		.name =	(char *)udc_name,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(of_nvt680_match),
#endif
	},
	.probe		= nvt680_udc_probe,
	.remove		= nvt680_udc_remove,

};

module_platform_driver(nvt680_driver);

MODULE_AUTHOR("Klins Chen <klins_chen@novatek.com.tw>");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.01.001");
MODULE_DESCRIPTION(DRIVER_DESC);
