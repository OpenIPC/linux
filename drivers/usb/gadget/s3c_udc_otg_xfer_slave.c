/*
 * drivers/usb/gadget/s3c_udc_otg_xfer_slave.c
 * Samsung S3C on-chip full/high speed USB OTG 2.0 device controllers
 *
 * Copyright (C) 2009 for Samsung Electronics
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define GINTMSK_INIT	(INT_RESUME|INT_ENUMDONE|INT_RESET|INT_SUSPEND|INT_RX_FIFO_NOT_EMPTY)
#define DOEPMSK_INIT	(AHB_ERROR)
#define DIEPMSK_INIT	(NON_ISO_IN_EP_TIMEOUT|AHB_ERROR)
#define GAHBCFG_INIT	(PTXFE_HALF|NPTXFE_HALF|MODE_SLAVE|BURST_SINGLE|GBL_INT_UNMASK)

u32 tx_ep_num = 2;

static int set_interface_first = 0;

/*-------------------------------------------------------------------------*/

/** Read to request from FIFO (max read == bytes in fifo)
 *  Return:  0 = still running, 1 = completed, negative = errno
 */
static int read_fifo(struct s3c_ep *ep, struct s3c_request *req)
{
	u32 csr, gintmsk;
	u32 *buf;
	u32 bufferspace, count, count_bytes, is_short = 0;
	u32 fifo = ep->fifo;

	csr = readl(S3C_UDC_OTG_GRXSTSP);
	count_bytes = (csr & 0x7ff0)>>4;

	gintmsk = readl(S3C_UDC_OTG_GINTMSK);

	if(!count_bytes) {
		DEBUG_OUT_EP("%s: count_bytes %d bytes\n", __FUNCTION__, count_bytes);

		// Unmask USB OTG 2.0 interrupt source : INT_RX_FIFO_NOT_EMPTY
		writel(gintmsk | INT_RX_FIFO_NOT_EMPTY, S3C_UDC_OTG_GINTMSK);
		return 0;
	}

	buf = req->req.buf + req->req.actual;
	prefetchw(buf);
	bufferspace = req->req.length - req->req.actual;

	count = count_bytes / 4;
	if(count_bytes%4) count = count + 1;

	req->req.actual += min(count_bytes, bufferspace);

	is_short = (count_bytes < ep->ep.maxpacket);
	DEBUG_OUT_EP("%s: read %s, %d bytes%s req %p %d/%d GRXSTSP:0x%x\n",
		__FUNCTION__,
		ep->ep.name, count_bytes,
		is_short ? "/S" : "", req, req->req.actual, req->req.length, csr);

	while (likely(count-- != 0)) {
		u32 byte = (u32) readl(fifo);

		if (unlikely(bufferspace == 0)) {
			/* this happens when the driver's buffer
		 	* is smaller than what the host sent.
		 	* discard the extra data.
		 	*/
			if (req->req.status != -EOVERFLOW)
				printk("%s overflow %d\n", ep->ep.name, count);
			req->req.status = -EOVERFLOW;
		} else {
			*buf++ = byte;
			bufferspace-=4;
		}
 	 }

	// Unmask USB OTG 2.0 interrupt source : INT_RX_FIFO_NOT_EMPTY
	writel(gintmsk | INT_RX_FIFO_NOT_EMPTY, S3C_UDC_OTG_GINTMSK);

	/* completion */
	if (is_short || req->req.actual == req->req.length) {
		done(ep, req, 0);
		return 1;
	}

	/* finished that packet.  the next one may be waiting... */
	return 0;
}

/* Inline code */
static __inline__ int write_packet(struct s3c_ep *ep,
				   struct s3c_request *req, int max)
{
	u32 *buf;
	int length, count;
	u32 fifo = ep->fifo, in_ctrl;

	buf = req->req.buf + req->req.actual;
	prefetch(buf);

	length = req->req.length - req->req.actual;
	length = min(length, max);
	req->req.actual += length;

	DEBUG("%s: Write %d (max %d), fifo=0x%x\n",
		__FUNCTION__, length, max, fifo);

	if(ep_index(ep) == EP0_CON) {
		writel((1<<19)|(length<<0), (u32) S3C_UDC_OTG_DIEPTSIZ0);

		in_ctrl =  readl(S3C_UDC_OTG_DIEPCTL0);
		writel(DEPCTL_EPENA|DEPCTL_CNAK|(EP2_IN<<11)| in_ctrl, (u32) S3C_UDC_OTG_DIEPCTL0);

		DEBUG_EP0("%s:(DIEPTSIZ0):0x%x, (DIEPCTL0):0x%x, (GNPTXSTS):0x%x\n", __FUNCTION__,
			readl(S3C_UDC_OTG_DIEPTSIZ0),readl(S3C_UDC_OTG_DIEPCTL0),
			readl(S3C_UDC_OTG_GNPTXSTS));

		udelay(30);

	} else if ((ep_index(ep) == EP2_IN)) {
		writel((1<<19)|(length<<0), S3C_UDC_OTG_DIEPTSIZ2);

		in_ctrl =  readl(S3C_UDC_OTG_DIEPCTL2);
		writel(DEPCTL_EPENA|DEPCTL_CNAK|(EP2_IN<<11)| in_ctrl, (u32) S3C_UDC_OTG_DIEPCTL2);

		DEBUG_IN_EP("%s:(DIEPTSIZ2):0x%x, (DIEPCTL2):0x%x, (GNPTXSTS):0x%x\n", __FUNCTION__,
			readl(S3C_UDC_OTG_DIEPTSIZ2),readl(S3C_UDC_OTG_DIEPCTL2),
			readl(S3C_UDC_OTG_GNPTXSTS));

		udelay(30);

	} else if ((ep_index(ep) == EP3_IN)) {

		if (set_interface_first == 1) {
			DEBUG_IN_EP("%s: first packet write skipped after set_interface\n", __FUNCTION__);
			set_interface_first = 0;
			return length;
		}

		writel((1<<19)|(length<<0), S3C_UDC_OTG_DIEPTSIZ3);

		in_ctrl =  readl(S3C_UDC_OTG_DIEPCTL3);
		writel(DEPCTL_EPENA|DEPCTL_CNAK|(EP2_IN<<11)| in_ctrl, (u32) S3C_UDC_OTG_DIEPCTL3);

		DEBUG_IN_EP("%s:(DIEPTSIZ3):0x%x, (DIEPCTL3):0x%x, (GNPTXSTS):0x%x\n", __FUNCTION__,
			readl(S3C_UDC_OTG_DIEPTSIZ3),readl(S3C_UDC_OTG_DIEPCTL3),
			readl(S3C_UDC_OTG_GNPTXSTS));

		udelay(30);

	} else {
		printk("%s: --> Error Unused Endpoint!!\n",
			__FUNCTION__);
		BUG();
	}

	for (count=0;count<length;count+=4) {
	  	writel(*buf++, fifo);
	}
	return length;
}

/** Write request to FIFO (max write == maxp size)
 *  Return:  0 = still running, 1 = completed, negative = errno
 */
static int write_fifo(struct s3c_ep *ep, struct s3c_request *req)
{
	u32 max, gintmsk;
	unsigned count;
	int is_last = 0, is_short = 0;

	gintmsk = readl(S3C_UDC_OTG_GINTMSK);

	max = le16_to_cpu(ep->desc->wMaxPacketSize);
	count = write_packet(ep, req, max);

	/* last packet is usually short (or a zlp) */
	if (unlikely(count != max))
		is_last = is_short = 1;
	else {
		if (likely(req->req.length != req->req.actual)
		    || req->req.zero)
			is_last = 0;
		else
			is_last = 1;
		/* interrupt/iso maxpacket may not fill the fifo */
		is_short = unlikely(max < ep_maxpacket(ep));
	}

	DEBUG_IN_EP("%s: wrote %s %d bytes%s%s req %p %d/%d\n",
			__FUNCTION__,
      			ep->ep.name, count,
     	 		is_last ? "/L" : "", is_short ? "/S" : "",
      			req, req->req.actual, req->req.length);

	/* requests complete when all IN data is in the FIFO */
	if (is_last) {
		if(!ep_index(ep)){
			printk("%s: --> Error EP0 must not come here!\n",
				__FUNCTION__);
			BUG();
		}
		writel(gintmsk&(~INT_NP_TX_FIFO_EMPTY), S3C_UDC_OTG_GINTMSK);
		done(ep, req, 0);
		return 1;
	}

	// Unmask USB OTG 2.0 interrupt source : INT_NP_TX_FIFO_EMPTY
	writel(gintmsk | INT_NP_TX_FIFO_EMPTY, S3C_UDC_OTG_GINTMSK);
	return 0;
}

/* ********************************************************************************************* */
/* Bulk OUT (recv)
 */

static void s3c_out_epn(struct s3c_udc *dev, u32 ep_idx)
{
	struct s3c_ep *ep = &dev->ep[ep_idx];
	struct s3c_request *req;

	if (unlikely(!(ep->desc))) {
		/* Throw packet away.. */
		printk("%s: No descriptor?!?\n", __FUNCTION__);
		return;
	}

	if (list_empty(&ep->queue))
		req = 0;
	else
		req = list_entry(ep->queue.next,
				struct s3c_request, queue);

	if (unlikely(!req)) {
		DEBUG_OUT_EP("%s: NULL REQ on OUT EP-%d\n", __FUNCTION__, ep_idx);
		return;

	} else {
		read_fifo(ep, req);
	}

}

/**
 * s3c_in_epn - handle IN interrupt
 */
static void s3c_in_epn(struct s3c_udc *dev, u32 ep_idx)
{
	struct s3c_ep *ep = &dev->ep[ep_idx];
	struct s3c_request *req;

	if (list_empty(&ep->queue))
		req = 0;
	else
		req = list_entry(ep->queue.next, struct s3c_request, queue);

	if (unlikely(!req)) {
		DEBUG_IN_EP("%s: NULL REQ on IN EP-%d\n", __FUNCTION__, ep_idx);
		return;
	}
	else {
		write_fifo(ep, req);
	}

}

/*
 *	elfin usb client interrupt handler.
 */
static irqreturn_t s3c_udc_irq(int irq, void *_dev)
{
	struct s3c_udc *dev = _dev;
	u32 intr_status;
	u32 usb_status, ep_ctrl, gintmsk;

	spin_lock(&dev->lock);

	intr_status = readl(S3C_UDC_OTG_GINTSTS);
	gintmsk = readl(S3C_UDC_OTG_GINTMSK);

	DEBUG_ISR("\n**** %s : GINTSTS=0x%x(on state %s), GINTMSK : 0x%x\n",
			__FUNCTION__, intr_status, state_names[dev->ep0state], gintmsk);

	if (!intr_status) {
		spin_unlock(&dev->lock);
		return IRQ_HANDLED;
	}

	if (intr_status & INT_ENUMDONE) {
		DEBUG_SETUP("####################################\n");
		DEBUG_SETUP("    %s: Speed Detection interrupt\n",
				__FUNCTION__);
		writel(INT_ENUMDONE, S3C_UDC_OTG_GINTSTS);

		usb_status = (readl(S3C_UDC_OTG_DSTS) & 0x6);

		if (usb_status & (USB_FULL_30_60MHZ | USB_FULL_48MHZ)) {
			DEBUG_SETUP("    %s: Full Speed Detection\n",__FUNCTION__);
			set_max_pktsize(dev, USB_SPEED_FULL);

		} else {
			DEBUG_SETUP("    %s: High Speed Detection : 0x%x\n", __FUNCTION__, usb_status);
			set_max_pktsize(dev, USB_SPEED_HIGH);
		}
	}

	if (intr_status & INT_EARLY_SUSPEND) {
		DEBUG_SETUP("####################################\n");
		DEBUG_SETUP("    %s:Early suspend interrupt\n", __FUNCTION__);
		writel(INT_EARLY_SUSPEND, S3C_UDC_OTG_GINTSTS);
	}

	if (intr_status & INT_SUSPEND) {
		usb_status = readl(S3C_UDC_OTG_DSTS);
		DEBUG_SETUP("####################################\n");
		DEBUG_SETUP("    %s:Suspend interrupt :(DSTS):0x%x\n", __FUNCTION__, usb_status);
		writel(INT_SUSPEND, S3C_UDC_OTG_GINTSTS);

		if (dev->gadget.speed != USB_SPEED_UNKNOWN
		    && dev->driver
		    && dev->driver->suspend) {

			dev->driver->suspend(&dev->gadget);
		}
	}

	if (intr_status & INT_RESUME) {
		DEBUG_SETUP("####################################\n");
		DEBUG_SETUP("    %s: Resume interrupt\n", __FUNCTION__);
		writel(INT_RESUME, S3C_UDC_OTG_GINTSTS);

		if (dev->gadget.speed != USB_SPEED_UNKNOWN
		    && dev->driver
		    && dev->driver->resume) {

			dev->driver->resume(&dev->gadget);
		}
	}

	if (intr_status & INT_RESET) {
		usb_status = readl(S3C_UDC_OTG_GOTGCTL);
		DEBUG_SETUP("####################################\n");
		DEBUG_SETUP("    %s: Reset interrupt - (GOTGCTL):0x%x\n", __FUNCTION__, usb_status);
		writel(INT_RESET, S3C_UDC_OTG_GINTSTS);

		if((usb_status & 0xc0000) == (0x3 << 18)) {
			if(reset_available) {
				DEBUG_SETUP("     ===> OTG core got reset (%d)!! \n", reset_available);
				reconfig_usbd();
				dev->ep0state = WAIT_FOR_SETUP;
				reset_available = 0;
			}
		} else {
			reset_available = 1;
			DEBUG_SETUP("      RESET handling skipped : reset_available : %d\n", reset_available);
		}
	}

	if (intr_status & INT_RX_FIFO_NOT_EMPTY) {
		u32 grx_status, packet_status, ep_num, fifoCntByte = 0;

		// Mask USB OTG 2.0 interrupt source : INT_RX_FIFO_NOT_EMPTY
		gintmsk &= ~INT_RX_FIFO_NOT_EMPTY;
		writel(gintmsk, S3C_UDC_OTG_GINTMSK);

		grx_status = readl(S3C_UDC_OTG_GRXSTSR);
		DEBUG_ISR("    INT_RX_FIFO_NOT_EMPTY(GRXSTSR):0x%x, GINTMSK:0x%x\n", grx_status, gintmsk);

		packet_status = grx_status & 0x1E0000;
		fifoCntByte = (grx_status & 0x7ff0)>>4;
		ep_num = grx_status & EP_MASK;

		if (fifoCntByte) {

			if (packet_status == SETUP_PKT_RECEIVED)  {
				DEBUG_EP0("      => A SETUP data packet received : %d bytes\n", fifoCntByte);
				s3c_handle_ep0(dev);

				// Unmask USB OTG 2.0 interrupt source : INT_RX_FIFO_NOT_EMPTY
				gintmsk |= INT_RX_FIFO_NOT_EMPTY;

			} else if (packet_status == OUT_PKT_RECEIVED) {

				if(ep_num == EP1_OUT) {
					ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL1);
					DEBUG_ISR("      => A Bulk OUT data packet received : %d bytes, (DOEPCTL1):0x%x\n",
						fifoCntByte, ep_ctrl);
					s3c_out_epn(dev, 1);
					gintmsk = readl(S3C_UDC_OTG_GINTMSK);
					writel(ep_ctrl | DEPCTL_CNAK, S3C_UDC_OTG_DOEPCTL1);
				} else if (ep_num == EP0_CON) {
					ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL0);
					DEBUG_EP0("      => A CONTROL OUT data packet received : %d bytes, (DOEPCTL0):0x%x\n",
						fifoCntByte, ep_ctrl);
					dev->ep0state = DATA_STATE_RECV;
					s3c_ep0_read(dev);
					gintmsk |= INT_RX_FIFO_NOT_EMPTY;
				} else {
					DEBUG_ISR("      => Unused EP: %d bytes, (GRXSTSR):0x%x\n", fifoCntByte, grx_status);
				}
			} else {
				grx_status = readl(S3C_UDC_OTG_GRXSTSP);

				// Unmask USB OTG 2.0 interrupt source : INT_RX_FIFO_NOT_EMPTY
				gintmsk |= INT_RX_FIFO_NOT_EMPTY;

				DEBUG_ISR("      => A reserved packet received : %d bytes\n", fifoCntByte);
			}
		} else {
			if (dev->ep0state == DATA_STATE_XMIT) {
				ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL0);
				DEBUG_EP0("      => Write ep0 continue... (DOEPCTL0):0x%x\n", ep_ctrl);
				s3c_ep0_write(dev);
			}

			if (packet_status == SETUP_TRANSACTION_COMPLETED) {
				ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL0);
				DEBUG_EP0("      => A SETUP transaction completed (DOEPCTL0):0x%x\n", ep_ctrl);
				writel(ep_ctrl | DEPCTL_CNAK, S3C_UDC_OTG_DOEPCTL0);

			} else if (packet_status == OUT_TRANSFER_COMPLELTED) {
				if (ep_num == EP1_OUT) {
					ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL1);
					DEBUG_ISR("      => An OUT transaction completed (DOEPCTL1):0x%x\n", ep_ctrl);
					writel(ep_ctrl | DEPCTL_CNAK, S3C_UDC_OTG_DOEPCTL1);
				} else if (ep_num == EP0_CON) {
					ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL0);
					DEBUG_ISR("      => An OUT transaction completed (DOEPCTL0):0x%x\n", ep_ctrl);
					writel(ep_ctrl | DEPCTL_CNAK, S3C_UDC_OTG_DOEPCTL0);
				} else {
					DEBUG_ISR("      => Unused EP: %d bytes, (GRXSTSR):0x%x\n", fifoCntByte, grx_status);
				}
			} else if (packet_status == OUT_PKT_RECEIVED) {
				DEBUG_ISR("      => A  OUT PACKET RECEIVED (NO FIFO CNT BYTE)...(GRXSTSR):0x%x\n", grx_status);
			} else {
				DEBUG_ISR("      => A RESERVED PACKET RECEIVED (NO FIFO CNT BYTE)...(GRXSTSR):0x%x\n", grx_status);
			}

			grx_status = readl(S3C_UDC_OTG_GRXSTSP);

			// Unmask USB OTG 2.0 interrupt source : INT_RX_FIFO_NOT_EMPTY
			gintmsk |= INT_RX_FIFO_NOT_EMPTY;

		}

		// Un/Mask USB OTG 2.0 interrupt sources
		writel(gintmsk, S3C_UDC_OTG_GINTMSK);

		spin_unlock(&dev->lock);
		return IRQ_HANDLED;
	}


	if (intr_status & INT_NP_TX_FIFO_EMPTY) {
		DEBUG_ISR("    INT_NP_TX_FIFO_EMPTY (GNPTXSTS):0x%x, (GINTMSK):0x%x, ep_num=%d\n",
				readl(S3C_UDC_OTG_GNPTXSTS),
				readl(S3C_UDC_OTG_GINTMSK),
				tx_ep_num);

		s3c_in_epn(dev, tx_ep_num);
	}

	spin_unlock(&dev->lock);

	return IRQ_HANDLED;
}

/** Queue one request
 *  Kickstart transfer if needed
 */
static int s3c_queue(struct usb_ep *_ep, struct usb_request *_req,
			 gfp_t gfp_flags)
{
	struct s3c_request *req;
	struct s3c_ep *ep;
	struct s3c_udc *dev;
	unsigned long flags;

	req = container_of(_req, struct s3c_request, req);
	if (unlikely(!_req || !_req->complete || !_req->buf
			|| !list_empty(&req->queue)))
	{
		DEBUG("%s: bad params\n", __FUNCTION__);
		return -EINVAL;
	}

	ep = container_of(_ep, struct s3c_ep, ep);
	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		DEBUG("%s: bad ep\n", __FUNCTION__);
		return -EINVAL;
	}

	dev = ep->dev;
	if (unlikely(!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)) {
		DEBUG("%s: bogus device state %p\n", __FUNCTION__, dev->driver);
		return -ESHUTDOWN;
	}

	DEBUG("\n%s: %s queue req %p, len %d buf %p\n",
		__FUNCTION__, _ep->name, _req, _req->length, _req->buf);

	spin_lock_irqsave(&dev->lock, flags);

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	/* kickstart this i/o queue? */
	DEBUG("%s: Add to ep=%d, Q empty=%d, stopped=%d\n",
		__FUNCTION__, ep_index(ep), list_empty(&ep->queue), ep->stopped);

	if (list_empty(&ep->queue) && likely(!ep->stopped)) {
		u32 csr;

		if (unlikely(ep_index(ep) == 0)) {
			/* EP0 */
			list_add_tail(&req->queue, &ep->queue);
			s3c_ep0_kick(dev, ep);
			req = 0;

		} else if (ep_is_in(ep)) {
			csr = readl((u32) S3C_UDC_OTG_GINTSTS);
			DEBUG_IN_EP("%s: ep_is_in, S3C_UDC_OTG_GINTSTS=0x%x\n",
				__FUNCTION__, csr);

			if((csr & INT_NP_TX_FIFO_EMPTY) &&
			   (write_fifo(ep, req) == 1)) {
				req = 0;
			} else {
				DEBUG("++++ IN-list_add_taill::req=%p, ep=%d\n",
					req, ep_index(ep));
				tx_ep_num = ep_index(ep);
			}
		} else {
			csr = readl((u32) S3C_UDC_OTG_GINTSTS);
			DEBUG_OUT_EP("%s: ep_is_out, S3C_UDC_OTG_GINTSTS=0x%x\n",
				__FUNCTION__, csr);

			if((csr & INT_RX_FIFO_NOT_EMPTY) &&
			   (read_fifo(ep, req) == 1))
				req = 0;
			else
				DEBUG("++++ OUT-list_add_taill::req=%p, DOEPCTL1:0x%x\n",
					req, readl(S3C_UDC_OTG_DOEPCTL1));
		}
	}

	/* pio or dma irq handler advances the queue. */
	if (likely(req != 0))
		list_add_tail(&req->queue, &ep->queue);

	spin_unlock_irqrestore(&dev->lock, flags);

	return 0;
}

/****************************************************************/
/* End Point 0 related functions                                */
/****************************************************************/

/* return:  0 = still running, 1 = completed, negative = errno */
static int write_fifo_ep0(struct s3c_ep *ep, struct s3c_request *req)
{
	u32 max;
	unsigned count;
	int is_last;

	max = ep_maxpacket(ep);

	DEBUG_EP0("%s: max = %d\n", __FUNCTION__, max);

	count = write_packet(ep, req, max);

	/* last packet is usually short (or a zlp) */
	if (likely(count != max))
		is_last = 1;
	else {
		if (likely(req->req.length != req->req.actual) || req->req.zero)
			is_last = 0;
		else
			is_last = 1;
	}

	DEBUG_EP0("%s: wrote %s %d bytes%s %d left %p\n", __FUNCTION__,
		  ep->ep.name, count,
		  is_last ? "/L" : "", req->req.length - req->req.actual, req);

	/* requests complete when all IN data is in the FIFO */
	if (is_last) {
		return 1;
	}

	return 0;
}

static __inline__ int s3c_fifo_read(struct s3c_ep *ep, u32 *cp, int max)
{
	int bytes;
	int count;
	u32 grx_status = readl(S3C_UDC_OTG_GRXSTSP);
	bytes = (grx_status & 0x7ff0)>>4;

	DEBUG_EP0("%s: GRXSTSP=0x%x, bytes=%d, ep_index=%d, fifo=0x%x\n",
			__FUNCTION__, grx_status, bytes, ep_index(ep), ep->fifo);

	// 32 bits interface
	count = bytes / 4;

	while (count--) {
		*cp++ = (u32) readl(S3C_UDC_OTG_EP0_FIFO);
	}

	return bytes;
}

static int read_fifo_ep0(struct s3c_ep *ep, struct s3c_request *req)
{
	u32 csr;
	u32 *buf;
	unsigned bufferspace, count, is_short, bytes;
	u32 fifo = ep->fifo;

	DEBUG_EP0("%s\n", __FUNCTION__);

	csr = readl(S3C_UDC_OTG_GRXSTSP);
	bytes = (csr & 0x7ff0)>>4;

	buf = req->req.buf + req->req.actual;
	prefetchw(buf);
	bufferspace = req->req.length - req->req.actual;

	/* read all bytes from this packet */
	if (likely((csr & EP_MASK) == EP0_CON)) {
		count = bytes / 4;
		req->req.actual += min(bytes, bufferspace);

	} else	{		// zlp
		count = 0;
		bytes = 0;
	}

	is_short = (bytes < ep->ep.maxpacket);
	DEBUG_EP0("%s: read %s %02x, %d bytes%s req %p %d/%d\n",
		  __FUNCTION__,
		  ep->ep.name, csr, bytes,
		  is_short ? "/S" : "", req, req->req.actual, req->req.length);

	while (likely(count-- != 0)) {
		u32 byte = (u32) readl(fifo);

		if (unlikely(bufferspace == 0)) {
			/* this happens when the driver's buffer
			 * is smaller than what the host sent.
			 * discard the extra data.
			 */
			if (req->req.status != -EOVERFLOW)
				DEBUG_EP0("%s overflow %d\n", ep->ep.name,
					  count);
			req->req.status = -EOVERFLOW;
		} else {
			*buf++ = byte;
			bufferspace = bufferspace - 4;
		}
	}

	/* completion */
	if (is_short || req->req.actual == req->req.length) {
		return 1;
	}

	return 0;
}

/**
 * udc_set_address - set the USB address for this device
 * @address:
 *
 * Called from control endpoint function
 * after it decodes a set address setup packet.
 */
static void udc_set_address(struct s3c_udc *dev, unsigned char address)
{
	u32 ctrl = readl(S3C_UDC_OTG_DCFG);
	writel(address << 4 | ctrl, S3C_UDC_OTG_DCFG);

	ctrl = readl(S3C_UDC_OTG_DIEPCTL0);
	writel(DEPCTL_EPENA|DEPCTL_CNAK|ctrl, S3C_UDC_OTG_DIEPCTL0); /* EP0: Control IN */

	DEBUG_EP0("%s: USB OTG 2.0 Device address=%d, DCFG=0x%x\n",
		__FUNCTION__, address, readl(S3C_UDC_OTG_DCFG));

	dev->usb_address = address;
}



static int first_time = 1;

static void s3c_ep0_read(struct s3c_udc *dev)
{
	struct s3c_request *req;
	struct s3c_ep *ep = &dev->ep[0];
	int ret;

	if (!list_empty(&ep->queue))
		req = list_entry(ep->queue.next, struct s3c_request, queue);
	else {
		printk("%s: ---> BUG\n", __FUNCTION__);
		BUG();	//logic ensures		-jassi
		return;
	}

	DEBUG_EP0("%s: req.length = 0x%x, req.actual = 0x%x\n",
		__FUNCTION__, req->req.length, req->req.actual);

	if(req->req.length == 0) {
		dev->ep0state = WAIT_FOR_SETUP;
		first_time = 1;
		done(ep, req, 0);
		return;
	}

	if(!req->req.actual && first_time){	//for SetUp packet
		first_time = 0;
		return;
	}

	ret = read_fifo_ep0(ep, req);
	if (ret) {
		dev->ep0state = WAIT_FOR_SETUP;
		first_time = 1;
		done(ep, req, 0);
		return;
	}

}

/*
 * DATA_STATE_XMIT
 */
static int s3c_ep0_write(struct s3c_udc *dev)
{
	struct s3c_request *req;
	struct s3c_ep *ep = &dev->ep[0];
	int ret, need_zlp = 0;

	DEBUG_EP0("%s: ep0 write\n", __FUNCTION__);

	if (list_empty(&ep->queue))
		req = 0;
	else
		req = list_entry(ep->queue.next, struct s3c_request, queue);

	if (!req) {
		DEBUG_EP0("%s: NULL REQ\n", __FUNCTION__);
		return 0;
	}

	DEBUG_EP0("%s: req.length = 0x%x, req.actual = 0x%x\n",
		__FUNCTION__, req->req.length, req->req.actual);

	if (req->req.length == 0) {
		dev->ep0state = WAIT_FOR_SETUP;
	   	done(ep, req, 0);
		return 1;
	}

	if (req->req.length - req->req.actual == ep0_fifo_size) {
		/* Next write will end with the packet size, */
		/* so we need Zero-length-packet */
		need_zlp = 1;
	}

	ret = write_fifo_ep0(ep, req);

	if ((ret == 1) && !need_zlp) {
		/* Last packet */
		DEBUG_EP0("%s: finished, waiting for status\n", __FUNCTION__);
		dev->ep0state = WAIT_FOR_SETUP;
	} else {
		DEBUG_EP0("%s: not finished\n", __FUNCTION__);
	}

	if (need_zlp) {
		DEBUG_EP0("%s: Need ZLP!\n", __FUNCTION__);
		dev->ep0state = DATA_STATE_NEED_ZLP;
	}

	if(ret)
	   done(ep, req, 0);

	return 1;
}

static int s3c_udc_set_halt(struct usb_ep *_ep, int value)
{
	struct s3c_ep	*ep;
	u32 ep_num;
	ep = container_of(_ep, struct s3c_ep, ep);
	ep_num =ep_index(ep);
	
	DEBUG("%s: ep_num = %d, value = %d\n", __FUNCTION__, ep_num, value);
	/* TODO */
	return 0;
}

void s3c_udc_ep_activate(struct s3c_ep *ep)
{
	/* TODO */
}

/*
 * WAIT_FOR_SETUP (OUT_PKT_RDY)
 */
static void s3c_ep0_setup(struct s3c_udc *dev)
{
	struct s3c_ep *ep = &dev->ep[0];
	int i, bytes, is_in;
	u32 ep_ctrl;

	/* Nuke all previous transfers */
	nuke(ep, -EPROTO);

	/* read control req from fifo (8 bytes) */
	bytes = s3c_fifo_read(ep, (u32 *)&usb_ctrl, 8);

	DEBUG_SETUP("Read CTRL REQ %d bytes\n", bytes);
	DEBUG_SETUP("  CTRL.bRequestType = 0x%x (is_in %d)\n", usb_ctrl.bRequestType,
		    usb_ctrl.bRequestType & USB_DIR_IN);
	DEBUG_SETUP("  CTRL.bRequest = 0x%x\n", usb_ctrl.bRequest);
	DEBUG_SETUP("  CTRL.wLength = 0x%x\n", usb_ctrl.wLength);
	DEBUG_SETUP("  CTRL.wValue = 0x%x (%d)\n", usb_ctrl.wValue, usb_ctrl.wValue >> 8);
	DEBUG_SETUP("  CTRL.wIndex = 0x%x\n", usb_ctrl.wIndex);

	/* Set direction of EP0 */
	if (likely(usb_ctrl.bRequestType & USB_DIR_IN)) {
		ep->bEndpointAddress |= USB_DIR_IN;
		is_in = 1;
	} else {
		ep->bEndpointAddress &= ~USB_DIR_IN;
		is_in = 0;
	}

	dev->req_pending = 1;

	/* Handle some SETUP packets ourselves */
	switch (usb_ctrl.bRequest) {
		case USB_REQ_SET_ADDRESS:
			if (usb_ctrl.bRequestType
				!= (USB_TYPE_STANDARD | USB_RECIP_DEVICE))
				break;

			DEBUG_SETUP("%s: *** USB_REQ_SET_ADDRESS (%d)\n",
					__FUNCTION__, usb_ctrl.wValue);
			udc_set_address(dev, usb_ctrl.wValue);
			return;

		case USB_REQ_SET_CONFIGURATION :
			DEBUG_SETUP("============================================\n");
			DEBUG_SETUP("%s: USB_REQ_SET_CONFIGURATION (%d)\n",
					__FUNCTION__, usb_ctrl.wValue);
config_change:
			// Just to send ZLP(Zero length Packet) to HOST in response to SET CONFIGURATION
			ep_ctrl = readl(S3C_UDC_OTG_DIEPCTL0);
			writel(DEPCTL_EPENA|DEPCTL_CNAK|ep_ctrl, S3C_UDC_OTG_DIEPCTL0); /* EP0: Control IN */

			// For Startng EP1 on this new configuration
			ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL1);
			writel(DEPCTL_EPDIS|DEPCTL_CNAK|DEPCTL_BULK_TYPE|DEPCTL_USBACTEP|ep_ctrl, S3C_UDC_OTG_DOEPCTL1); /* EP1: Bulk OUT */

			// For starting EP2 on this new configuration
			ep_ctrl = readl(S3C_UDC_OTG_DIEPCTL2);
			writel(DEPCTL_BULK_TYPE|DEPCTL_USBACTEP|ep_ctrl, S3C_UDC_OTG_DIEPCTL2); /* EP2: Bulk IN */

			// For starting EP3 on this new configuration
			ep_ctrl = readl(S3C_UDC_OTG_DIEPCTL3);
			writel(DEPCTL_BULK_TYPE|DEPCTL_USBACTEP|ep_ctrl, S3C_UDC_OTG_DIEPCTL3); /* EP3: INTR IN */

			DEBUG_SETUP("%s:(DOEPCTL1):0x%x, (DIEPCTL2):0x%x, (DIEPCTL3):0x%x\n",
				__FUNCTION__,
				readl(S3C_UDC_OTG_DOEPCTL1),
				readl(S3C_UDC_OTG_DIEPCTL2),
				readl(S3C_UDC_OTG_DIEPCTL3));

			DEBUG_SETUP("============================================\n");

			reset_available = 1;
			dev->req_config = 1;
			break;

		case USB_REQ_GET_DESCRIPTOR:
			DEBUG_SETUP("%s: *** USB_REQ_GET_DESCRIPTOR  \n",__FUNCTION__);
			break;

		case USB_REQ_SET_INTERFACE:
			DEBUG_SETUP("%s: *** USB_REQ_SET_INTERFACE (%d)\n",
					__FUNCTION__, usb_ctrl.wValue);

			set_interface_first = 1;
			goto config_change;
			break;

		case USB_REQ_GET_CONFIGURATION:
			DEBUG_SETUP("%s: *** USB_REQ_GET_CONFIGURATION  \n",__FUNCTION__);
			break;

		case USB_REQ_GET_STATUS:
			DEBUG_SETUP("%s: *** USB_REQ_GET_STATUS  \n",__FUNCTION__);
			break;

		default:
			DEBUG_SETUP("%s: *** Default of usb_ctrl.bRequest=0x%x happened.\n",
					__FUNCTION__, usb_ctrl.bRequest);
			break;
	}

	if (likely(dev->driver)) {
		/* device-2-host (IN) or no data setup command,
		 * process immediately */
		spin_unlock(&dev->lock);
		DEBUG_SETUP("%s: ctrlrequest will be passed to fsg_setup()\n", __FUNCTION__);
		i = dev->driver->setup(&dev->gadget, (struct usb_ctrlrequest *)&usb_ctrl);
		spin_lock(&dev->lock);

		if (i < 0) {
			/* setup processing failed, force stall */
			DEBUG_SETUP("%s: gadget setup FAILED (stalling), setup returned %d\n",
				__FUNCTION__, i);
			/* ep->stopped = 1; */
			dev->ep0state = WAIT_FOR_SETUP;
		}
	}
}

/*
 * handle ep0 interrupt
 */
static void s3c_handle_ep0(struct s3c_udc *dev)
{
	if (dev->ep0state == WAIT_FOR_SETUP) {
		DEBUG_EP0("%s: WAIT_FOR_SETUP\n", __FUNCTION__);
		s3c_ep0_setup(dev);

	} else {
		DEBUG_EP0("%s: strange state!!(state = %s)\n",
			__FUNCTION__, state_names[dev->ep0state]);
	}
}

static void s3c_ep0_kick(struct s3c_udc *dev, struct s3c_ep *ep)
{
	DEBUG_EP0("%s: ep_is_in = %d\n", __FUNCTION__, ep_is_in(ep));
	if (ep_is_in(ep)) {
		dev->ep0state = DATA_STATE_XMIT;
		s3c_ep0_write(dev);
	} else {
		dev->ep0state = DATA_STATE_RECV;
		s3c_ep0_read(dev);
	}
}
