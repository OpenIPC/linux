/*
* linux/drivers/usb/gadget/ambarella_udc.h
* driver for High/Full speed
USB device controller on Ambarella processors
*
* History:
*	2008/06/12 - [Cao Rongrong] created file
*
* Copyright (C) 2008 by Ambarella, Inc.
* http://www.ambarella.com
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
* along with this program; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA  02111-1307, USA.
*/

#ifndef _AMBARELLA_UDC_H
#define _AMBARELLA_UDC_H

#define CTRL_IN			0
#define CTRL_OUT		16

#define EP_IN_NUM		16
#define EP_NUM_MAX		32

#define CTRL_OUT_UDC_IDX	11

#define ISO_MAX_PACKET		3

#define IS_EP0(ep)		(ep->id == CTRL_IN || ep->id == CTRL_OUT)
#define IS_ISO_IN_EP(ep)	(!IS_EP0(ep) && usb_endpoint_is_isoc_in(ep->ep.desc))

#define UDC_DMA_MAXPACKET	65536

#define VBUS_POLL_TIMEOUT	msecs_to_jiffies(500)

//-------------------------------------
// Structure definition
//-------------------------------------
// SETUP buffer descriptor
struct ambarella_setup_desc {
	u32 status;
	u32 reserved;
	u32 data0;
	u32 data1;
	u32 rsvd1;
	u32 rsvd2;
	u32 rsvd3;
	u32 rsvd4;
};

// IN/OUT data descriptor
struct ambarella_data_desc {
	u32 status;
	u32 reserved;
	u32 data_ptr;
	u32 next_desc_ptr;
	u32 rsvd1;
	u32 last_aux;		/* dma enginee may disturb the L bit in status
				 * field, so we use this field as auxiliary to
				 * mark the last descriptor */
	dma_addr_t cur_desc_addr;	/* dma address for this descriptor */
	struct ambarella_data_desc *next_desc_virt;
};


struct ambarella_ep_reg {
	u32 ctrl_reg;
	u32 sts_reg;
	u32 buf_sz_reg;		// IN_EP: buf_sz_reg,     OUT_EP: pkt_frm_num_reg
	u32 max_pkt_sz_reg;	// IN_EP: max_pkt_sz_reg,   OUT EP: buffer_size_max_pkt_sz_reg
	u32 setup_buf_ptr_reg;	// Just for ep0
	u32 dat_desc_ptr_reg;
	//u32 rw_confirm;	// For slave-only mode
};


struct ambarella_request {
	struct list_head	queue;		/* ep's requests */
	struct usb_request	req;

	int			desc_count;
	int			active_desc_count;
	dma_addr_t		data_desc_addr; /* data_desc Physical Address */
	struct ambarella_data_desc 	*data_desc;

	dma_addr_t		dma_aux;
	void			*buf_aux;	/* If the original buffer of
						 * usb_req is not 8-bytes
						 * aligned, we use this buffer
						 * instead */
	unsigned		use_aux_buf : 1,
				mapped : 1;
};


struct ambarella_ep {

	struct list_head		queue;
	struct ambarella_udc		*udc;
	const struct usb_endpoint_descriptor *desc;
	struct usb_ep			ep;
	u8 				id;
	u8 				dir;

	struct ambarella_ep_reg		ep_reg;

	struct ambarella_data_desc 	*data_desc;
	struct ambarella_data_desc 	*last_data_desc;
	dma_addr_t			data_desc_addr; /* data_desc Physical Address */

	unsigned			halted : 1,
					cancel_transfer : 1,
					need_cnak : 1,
					ctrl_sts_phase : 1,
					dma_going : 1;

	unsigned int frame_offset;  /* iso frame num offset */
};

struct ambarella_udc {
	spinlock_t			lock;
	struct device			*dev;
	void __iomem			*base_reg;
	void __iomem			*reset_reg;
	int				irq;
	struct usb_phy			*phy;

	struct proc_dir_entry		*proc_file;
	struct work_struct		uevent_work;
	struct timer_list		vbus_timer;
	enum usb_device_state		pre_state;

	struct usb_gadget		gadget;
	struct usb_gadget_driver	*driver;

	struct dma_pool			*desc_dma_pool;

	struct ambarella_ep		ep[EP_NUM_MAX];
	u32				setup[2];
	dma_addr_t			setup_addr;
	struct ambarella_setup_desc	*setup_buf;
	dma_addr_t			dummy_desc_addr;
	struct ambarella_data_desc	*dummy_desc;

	u16				cur_config;
	u16				cur_intf;
	u16				cur_alt;

	unsigned 			auto_ack_0_pkt : 1,
					remote_wakeup_en  : 1,
					host_suspended : 1,
					sys_suspended : 1,
					reset_by_host : 1,
					vbus_status : 1,
					udc_is_enabled : 1;

	/* dma_fix is only used for S2 chip, due to its DMA engine fault */
	u32				dma_fix;
};

/* Function Declaration  */
static void ambarella_udc_done(struct ambarella_ep *ep,
		struct ambarella_request *req, int status);

static void ambarella_set_tx_dma(struct ambarella_ep *ep,
	struct ambarella_request * req, int fix);

static void ambarella_set_rx_dma(struct ambarella_ep *ep,
	struct ambarella_request * req);

static void ambarella_udc_reinit(struct ambarella_udc *udc);

static int ambarella_udc_set_halt(struct usb_ep *_ep, int value);

static void ambarella_ep_nuke(struct ambarella_ep *ep, int status);

static void ambarella_stop_activity(struct ambarella_udc *udc);

static void ambarella_udc_enable(struct ambarella_udc *udc);

static void ambarella_udc_disable(struct ambarella_udc *udc);

static int ambarella_udc_vbus_session(struct usb_gadget *gadget, int is_active);

static int ambarella_udc_set_pullup(struct ambarella_udc *udc, int is_on);

static int ambarella_udc_get_frame(struct usb_gadget *_gadget);

#endif

