/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/fh_otg_pcd.c $
 * $Revision: #105 $
 * $Date: 2013/05/16 $
 * $Change: 2231774 $
 *
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================== */
#ifndef FH_HOST_ONLY

/** @file
 * This file implements PCD Core. All code in this file is portable and doesn't
 * use any OS specific functions.
 * PCD Core provides Interface, defined in <code><fh_otg_pcd_if.h></code>
 * header file, which can be used to implement OS specific PCD interface.
 *
 * An important function of the PCD is managing interrupts generated
 * by the FH_otg controller. The implementation of the FH_otg device
 * mode interrupt service routines is in fh_otg_pcd_intr.c.
 *
 * @todo Add Device Mode test modes (Test J mode, Test K mode, etc).
 * @todo Does it work when the request size is greater than DEPTSIZ
 * transfer size
 *
 */

#include "fh_otg_pcd.h"

#ifdef FH_UTE_CFI
#include "fh_otg_cfi.h"

extern int init_cfi(cfiobject_t * cfiobj);
#endif

/**
 * Choose endpoint from ep arrays using usb_ep structure.
 */
static fh_otg_pcd_ep_t *get_ep_from_handle(fh_otg_pcd_t * pcd, void *handle)
{
	int i;
	if (pcd->ep0.priv == handle) {
		return &pcd->ep0;
	}
	for (i = 0; i < MAX_EPS_CHANNELS - 1; i++) {
		if (pcd->in_ep[i].priv == handle)
			return &pcd->in_ep[i];
		if (pcd->out_ep[i].priv == handle)
			return &pcd->out_ep[i];
	}

	return NULL;
}

/**
 * This function completes a request.  It call's the request call back.
 */
void fh_otg_request_done(fh_otg_pcd_ep_t * ep, fh_otg_pcd_request_t * req,
			  int32_t status)
{
	unsigned stopped = ep->stopped;

	FH_DEBUGPL(DBG_PCDV, "%s(ep %p req %p)\n", __func__, ep, req);
	FH_CIRCLEQ_REMOVE_INIT(&ep->queue, req, queue_entry);

	/* don't modify queue heads during completion callback */
	ep->stopped = 1;
	/* spin_unlock/spin_lock now done in fops->complete() */
	ep->pcd->fops->complete(ep->pcd, ep->priv, req->priv, status,
				req->actual);

	if (ep->pcd->request_pending > 0) {
		--ep->pcd->request_pending;
	}

	ep->stopped = stopped;
	FH_FREE(req);
}

/**
 * This function terminates all the requsts in the EP request queue.
 */
void fh_otg_request_nuke(fh_otg_pcd_ep_t * ep)
{
	fh_otg_pcd_request_t *req;

	ep->stopped = 1;

	/* called with irqs blocked?? */
	while (!FH_CIRCLEQ_EMPTY(&ep->queue)) {
		req = FH_CIRCLEQ_FIRST(&ep->queue);
		fh_otg_request_done(ep, req, -FH_E_SHUTDOWN);
	}
}

void fh_otg_pcd_start(fh_otg_pcd_t * pcd,
		       const struct fh_otg_pcd_function_ops *fops)
{
	pcd->fops = fops;
}

/**
 * PCD Callback function for initializing the PCD when switching to
 * device mode.
 *
 * @param p void pointer to the <code>fh_otg_pcd_t</code>
 */
static int32_t fh_otg_pcd_start_cb(void *p)
{
	fh_otg_pcd_t *pcd = (fh_otg_pcd_t *) p;
	fh_otg_core_if_t *core_if = GET_CORE_IF(pcd);

	/*
	 * Initialized the Core for Device mode.
	 */
	if (fh_otg_is_device_mode(core_if)) {
		fh_otg_core_dev_init(core_if);
		/* Set core_if's lock pointer to the pcd->lock */
		core_if->lock = pcd->lock;
	}
	return 1;
}

/** CFI-specific buffer allocation function for EP */
#ifdef FH_UTE_CFI
uint8_t *cfiw_ep_alloc_buffer(fh_otg_pcd_t * pcd, void *pep, fh_dma_t * addr,
			      size_t buflen, int flags)
{
	fh_otg_pcd_ep_t *ep;
	ep = get_ep_from_handle(pcd, pep);
	if (!ep) {
		FH_WARN("bad ep\n");
		return -FH_E_INVALID;
	}

	return pcd->cfi->ops.ep_alloc_buf(pcd->cfi, pcd, ep, addr, buflen,
					  flags);
}
#else
uint8_t *cfiw_ep_alloc_buffer(fh_otg_pcd_t * pcd, void *pep, fh_dma_t * addr,
			      size_t buflen, int flags);
#endif

/**
 * PCD Callback function for notifying the PCD when resuming from
 * suspend.
 *
 * @param p void pointer to the <code>fh_otg_pcd_t</code>
 */
static int32_t fh_otg_pcd_resume_cb(void *p)
{
	fh_otg_pcd_t *pcd = (fh_otg_pcd_t *) p;

	if (pcd->fops->resume) {
		pcd->fops->resume(pcd);
	}

	/* Stop the SRP timeout timer. */
	if ((GET_CORE_IF(pcd)->core_params->phy_type != FH_PHY_TYPE_PARAM_FS)
	    || (!GET_CORE_IF(pcd)->core_params->i2c_enable)) {
		if (GET_CORE_IF(pcd)->srp_timer_started) {
			GET_CORE_IF(pcd)->srp_timer_started = 0;
			FH_TIMER_CANCEL(GET_CORE_IF(pcd)->srp_timer);
		}
	}
	return 1;
}

/**
 * PCD Callback function for notifying the PCD device is suspended.
 *
 * @param p void pointer to the <code>fh_otg_pcd_t</code>
 */
static int32_t fh_otg_pcd_suspend_cb(void *p)
{
	fh_otg_pcd_t *pcd = (fh_otg_pcd_t *) p;

	if (pcd->fops->suspend) {
		FH_SPINUNLOCK(pcd->lock);
		pcd->fops->suspend(pcd);
		FH_SPINLOCK(pcd->lock);
	}

	return 1;
}

/**
 * PCD Callback function for stopping the PCD when switching to Host
 * mode.
 *
 * @param p void pointer to the <code>fh_otg_pcd_t</code>
 */
static int32_t fh_otg_pcd_stop_cb(void *p)
{
	fh_otg_pcd_t *pcd = (fh_otg_pcd_t *) p;
	extern void fh_otg_pcd_stop(fh_otg_pcd_t * _pcd);

	fh_otg_pcd_stop(pcd);
	return 1;
}

/**
 * PCD Callback structure for handling mode switching.
 */
static fh_otg_cil_callbacks_t pcd_callbacks = {
	.start = fh_otg_pcd_start_cb,
	.stop = fh_otg_pcd_stop_cb,
	.suspend = fh_otg_pcd_suspend_cb,
	.resume_wakeup = fh_otg_pcd_resume_cb,
	.p = 0,			/* Set at registration */
};

/**
 * This function allocates a DMA Descriptor chain for the Endpoint
 * buffer to be used for a transfer to/from the specified endpoint.
 */
fh_otg_dev_dma_desc_t *fh_otg_ep_alloc_desc_chain(fh_dma_t * dma_desc_addr,
						    uint32_t count)
{
	return FH_DMA_ALLOC_ATOMIC(count * sizeof(fh_otg_dev_dma_desc_t),
				    dma_desc_addr);
}

/**
 * This function frees a DMA Descriptor chain that was allocated by ep_alloc_desc.
 */
void fh_otg_ep_free_desc_chain(fh_otg_dev_dma_desc_t * desc_addr,
				uint32_t dma_desc_addr, uint32_t count)
{
	FH_DMA_FREE(count * sizeof(fh_otg_dev_dma_desc_t), desc_addr,
		     dma_desc_addr);
}

#ifdef FH_EN_ISOC

/**
 * This function initializes a descriptor chain for Isochronous transfer
 *
 * @param core_if Programming view of FH_otg controller.
 * @param fh_ep The EP to start the transfer on.
 *
 */
void fh_otg_iso_ep_start_ddma_transfer(fh_otg_core_if_t * core_if,
					fh_ep_t * fh_ep)
{

	dsts_data_t dsts = {.d32 = 0 };
	depctl_data_t depctl = {.d32 = 0 };
	volatile uint32_t *addr;
	int i, j;
	uint32_t len;

	if (fh_ep->is_in)
		fh_ep->desc_cnt = fh_ep->buf_proc_intrvl / fh_ep->bInterval;
	else
		fh_ep->desc_cnt =
		    fh_ep->buf_proc_intrvl * fh_ep->pkt_per_frm /
		    fh_ep->bInterval;

	/** Allocate descriptors for double buffering */
	fh_ep->iso_desc_addr =
	    fh_otg_ep_alloc_desc_chain(&fh_ep->iso_dma_desc_addr,
					fh_ep->desc_cnt * 2);
	if (fh_ep->desc_addr) {
		FH_WARN("%s, can't allocate DMA descriptor chain\n", __func__);
		return;
	}

	dsts.d32 = FH_READ_REG32(&core_if->dev_if->dev_global_regs->dsts);

	/** ISO OUT EP */
	if (fh_ep->is_in == 0) {
		dev_dma_desc_sts_t sts = {.d32 = 0 };
		fh_otg_dev_dma_desc_t *dma_desc = fh_ep->iso_desc_addr;
		dma_addr_t dma_ad;
		uint32_t data_per_desc;
		fh_otg_dev_out_ep_regs_t *out_regs =
		    core_if->dev_if->out_ep_regs[fh_ep->num];
		int offset;

		addr = &core_if->dev_if->out_ep_regs[fh_ep->num]->doepctl;
		dma_ad = (dma_addr_t) FH_READ_REG32(&(out_regs->doepdma));

		/** Buffer 0 descriptors setup */
		dma_ad = fh_ep->dma_addr0;

		sts.b_iso_out.bs = BS_HOST_READY;
		sts.b_iso_out.rxsts = 0;
		sts.b_iso_out.l = 0;
		sts.b_iso_out.sp = 0;
		sts.b_iso_out.ioc = 0;
		sts.b_iso_out.pid = 0;
		sts.b_iso_out.framenum = 0;

		offset = 0;
		for (i = 0; i < fh_ep->desc_cnt - fh_ep->pkt_per_frm;
		     i += fh_ep->pkt_per_frm) {

			for (j = 0; j < fh_ep->pkt_per_frm; ++j) {
				uint32_t len = (j + 1) * fh_ep->maxpacket;
				if (len > fh_ep->data_per_frame)
					data_per_desc =
					    fh_ep->data_per_frame -
					    j * fh_ep->maxpacket;
				else
					data_per_desc = fh_ep->maxpacket;
				len = data_per_desc % 4;
				if (len)
					data_per_desc += 4 - len;

				sts.b_iso_out.rxbytes = data_per_desc;
				dma_desc->buf = dma_ad;
				dma_desc->status.d32 = sts.d32;

				offset += data_per_desc;
				dma_desc++;
				dma_ad += data_per_desc;
			}
		}

		for (j = 0; j < fh_ep->pkt_per_frm - 1; ++j) {
			uint32_t len = (j + 1) * fh_ep->maxpacket;
			if (len > fh_ep->data_per_frame)
				data_per_desc =
				    fh_ep->data_per_frame -
				    j * fh_ep->maxpacket;
			else
				data_per_desc = fh_ep->maxpacket;
			len = data_per_desc % 4;
			if (len)
				data_per_desc += 4 - len;
			sts.b_iso_out.rxbytes = data_per_desc;
			dma_desc->buf = dma_ad;
			dma_desc->status.d32 = sts.d32;

			offset += data_per_desc;
			dma_desc++;
			dma_ad += data_per_desc;
		}

		sts.b_iso_out.ioc = 1;
		len = (j + 1) * fh_ep->maxpacket;
		if (len > fh_ep->data_per_frame)
			data_per_desc =
			    fh_ep->data_per_frame - j * fh_ep->maxpacket;
		else
			data_per_desc = fh_ep->maxpacket;
		len = data_per_desc % 4;
		if (len)
			data_per_desc += 4 - len;
		sts.b_iso_out.rxbytes = data_per_desc;

		dma_desc->buf = dma_ad;
		dma_desc->status.d32 = sts.d32;
		dma_desc++;

		/** Buffer 1 descriptors setup */
		sts.b_iso_out.ioc = 0;
		dma_ad = fh_ep->dma_addr1;

		offset = 0;
		for (i = 0; i < fh_ep->desc_cnt - fh_ep->pkt_per_frm;
		     i += fh_ep->pkt_per_frm) {
			for (j = 0; j < fh_ep->pkt_per_frm; ++j) {
				uint32_t len = (j + 1) * fh_ep->maxpacket;
				if (len > fh_ep->data_per_frame)
					data_per_desc =
					    fh_ep->data_per_frame -
					    j * fh_ep->maxpacket;
				else
					data_per_desc = fh_ep->maxpacket;
				len = data_per_desc % 4;
				if (len)
					data_per_desc += 4 - len;

				data_per_desc =
				    sts.b_iso_out.rxbytes = data_per_desc;
				dma_desc->buf = dma_ad;
				dma_desc->status.d32 = sts.d32;

				offset += data_per_desc;
				dma_desc++;
				dma_ad += data_per_desc;
			}
		}
		for (j = 0; j < fh_ep->pkt_per_frm - 1; ++j) {
			data_per_desc =
			    ((j + 1) * fh_ep->maxpacket >
			     fh_ep->data_per_frame) ? fh_ep->data_per_frame -
			    j * fh_ep->maxpacket : fh_ep->maxpacket;
			data_per_desc +=
			    (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;
			sts.b_iso_out.rxbytes = data_per_desc;
			dma_desc->buf = dma_ad;
			dma_desc->status.d32 = sts.d32;

			offset += data_per_desc;
			dma_desc++;
			dma_ad += data_per_desc;
		}

		sts.b_iso_out.ioc = 1;
		sts.b_iso_out.l = 1;
		data_per_desc =
		    ((j + 1) * fh_ep->maxpacket >
		     fh_ep->data_per_frame) ? fh_ep->data_per_frame -
		    j * fh_ep->maxpacket : fh_ep->maxpacket;
		data_per_desc +=
		    (data_per_desc % 4) ? (4 - data_per_desc % 4) : 0;
		sts.b_iso_out.rxbytes = data_per_desc;

		dma_desc->buf = dma_ad;
		dma_desc->status.d32 = sts.d32;

		fh_ep->next_frame = 0;

		/** Write dma_ad into DOEPDMA register */
		FH_WRITE_REG32(&(out_regs->doepdma),
				(uint32_t) fh_ep->iso_dma_desc_addr);

	}
	/** ISO IN EP */
	else {
		dev_dma_desc_sts_t sts = {.d32 = 0 };
		fh_otg_dev_dma_desc_t *dma_desc = fh_ep->iso_desc_addr;
		dma_addr_t dma_ad;
		fh_otg_dev_in_ep_regs_t *in_regs =
		    core_if->dev_if->in_ep_regs[fh_ep->num];
		unsigned int frmnumber;
		fifosize_data_t txfifosize, rxfifosize;

		txfifosize.d32 =
		    FH_READ_REG32(&core_if->dev_if->in_ep_regs[fh_ep->num]->
				   dtxfsts);
		rxfifosize.d32 =
		    FH_READ_REG32(&core_if->core_global_regs->grxfsiz);

		addr = &core_if->dev_if->in_ep_regs[fh_ep->num]->diepctl;

		dma_ad = fh_ep->dma_addr0;

		dsts.d32 =
		    FH_READ_REG32(&core_if->dev_if->dev_global_regs->dsts);

		sts.b_iso_in.bs = BS_HOST_READY;
		sts.b_iso_in.txsts = 0;
		sts.b_iso_in.sp =
		    (fh_ep->data_per_frame % fh_ep->maxpacket) ? 1 : 0;
		sts.b_iso_in.ioc = 0;
		sts.b_iso_in.pid = fh_ep->pkt_per_frm;

		frmnumber = fh_ep->next_frame;

		sts.b_iso_in.framenum = frmnumber;
		sts.b_iso_in.txbytes = fh_ep->data_per_frame;
		sts.b_iso_in.l = 0;

		/** Buffer 0 descriptors setup */
		for (i = 0; i < fh_ep->desc_cnt - 1; i++) {
			dma_desc->buf = dma_ad;
			dma_desc->status.d32 = sts.d32;
			dma_desc++;

			dma_ad += fh_ep->data_per_frame;
			sts.b_iso_in.framenum += fh_ep->bInterval;
		}

		sts.b_iso_in.ioc = 1;
		dma_desc->buf = dma_ad;
		dma_desc->status.d32 = sts.d32;
		++dma_desc;

		/** Buffer 1 descriptors setup */
		sts.b_iso_in.ioc = 0;
		dma_ad = fh_ep->dma_addr1;

		for (i = 0; i < fh_ep->desc_cnt - fh_ep->pkt_per_frm;
		     i += fh_ep->pkt_per_frm) {
			dma_desc->buf = dma_ad;
			dma_desc->status.d32 = sts.d32;
			dma_desc++;

			dma_ad += fh_ep->data_per_frame;
			sts.b_iso_in.framenum += fh_ep->bInterval;

			sts.b_iso_in.ioc = 0;
		}
		sts.b_iso_in.ioc = 1;
		sts.b_iso_in.l = 1;

		dma_desc->buf = dma_ad;
		dma_desc->status.d32 = sts.d32;

		fh_ep->next_frame = sts.b_iso_in.framenum + fh_ep->bInterval;

		/** Write dma_ad into diepdma register */
		FH_WRITE_REG32(&(in_regs->diepdma),
				(uint32_t) fh_ep->iso_dma_desc_addr);
	}
	/** Enable endpoint, clear nak  */
	depctl.d32 = 0;
	depctl.b.epena = 1;
	depctl.b.usbactep = 1;
	depctl.b.cnak = 1;

	FH_MODIFY_REG32(addr, depctl.d32, depctl.d32);
	depctl.d32 = FH_READ_REG32(addr);
}

/**
 * This function initializes a descriptor chain for Isochronous transfer
 *
 * @param core_if Programming view of FH_otg controller.
 * @param ep The EP to start the transfer on.
 *
 */
void fh_otg_iso_ep_start_buf_transfer(fh_otg_core_if_t * core_if,
				       fh_ep_t * ep)
{
	depctl_data_t depctl = {.d32 = 0 };
	volatile uint32_t *addr;

	if (ep->is_in) {
		addr = &core_if->dev_if->in_ep_regs[ep->num]->diepctl;
	} else {
		addr = &core_if->dev_if->out_ep_regs[ep->num]->doepctl;
	}

	if (core_if->dma_enable == 0 || core_if->dma_desc_enable != 0) {
		return;
	} else {
		deptsiz_data_t deptsiz = {.d32 = 0 };

		ep->xfer_len =
		    ep->data_per_frame * ep->buf_proc_intrvl / ep->bInterval;
		ep->pkt_cnt =
		    (ep->xfer_len - 1 + ep->maxpacket) / ep->maxpacket;
		ep->xfer_count = 0;
		ep->xfer_buff =
		    (ep->proc_buf_num) ? ep->xfer_buff1 : ep->xfer_buff0;
		ep->dma_addr =
		    (ep->proc_buf_num) ? ep->dma_addr1 : ep->dma_addr0;

		if (ep->is_in) {
			/* Program the transfer size and packet count
			 *      as follows: xfersize = N * maxpacket +
			 *      short_packet pktcnt = N + (short_packet
			 *      exist ? 1 : 0) 
			 */
			deptsiz.b.mc = ep->pkt_per_frm;
			deptsiz.b.xfersize = ep->xfer_len;
			deptsiz.b.pktcnt =
			    (ep->xfer_len - 1 + ep->maxpacket) / ep->maxpacket;
			FH_WRITE_REG32(&core_if->dev_if->in_ep_regs[ep->num]->
					dieptsiz, deptsiz.d32);

			/* Write the DMA register */
			FH_WRITE_REG32(&
					(core_if->dev_if->in_ep_regs[ep->num]->
					 diepdma), (uint32_t) ep->dma_addr);

		} else {
			deptsiz.b.pktcnt =
			    (ep->xfer_len + (ep->maxpacket - 1)) /
			    ep->maxpacket;
			deptsiz.b.xfersize = deptsiz.b.pktcnt * ep->maxpacket;

			FH_WRITE_REG32(&core_if->dev_if->out_ep_regs[ep->num]->
					doeptsiz, deptsiz.d32);

			/* Write the DMA register */
			FH_WRITE_REG32(&
					(core_if->dev_if->out_ep_regs[ep->num]->
					 doepdma), (uint32_t) ep->dma_addr);

		}
		/** Enable endpoint, clear nak  */
		depctl.d32 = 0;
		depctl.b.epena = 1;
		depctl.b.cnak = 1;

		FH_MODIFY_REG32(addr, depctl.d32, depctl.d32);
	}
}

/**
 * This function does the setup for a data transfer for an EP and
 * starts the transfer. For an IN transfer, the packets will be
 * loaded into the appropriate Tx FIFO in the ISR. For OUT transfers,
 * the packets are unloaded from the Rx FIFO in the ISR.
 *
 * @param core_if Programming view of FH_otg controller.
 * @param ep The EP to start the transfer on.
 */

static void fh_otg_iso_ep_start_transfer(fh_otg_core_if_t * core_if,
					  fh_ep_t * ep)
{
	if (core_if->dma_enable) {
		if (core_if->dma_desc_enable) {
			if (ep->is_in) {
				ep->desc_cnt = ep->pkt_cnt / ep->pkt_per_frm;
			} else {
				ep->desc_cnt = ep->pkt_cnt;
			}
			fh_otg_iso_ep_start_ddma_transfer(core_if, ep);
		} else {
			if (core_if->pti_enh_enable) {
				fh_otg_iso_ep_start_buf_transfer(core_if, ep);
			} else {
				ep->cur_pkt_addr =
				    (ep->proc_buf_num) ? ep->xfer_buff1 : ep->
				    xfer_buff0;
				ep->cur_pkt_dma_addr =
				    (ep->proc_buf_num) ? ep->dma_addr1 : ep->
				    dma_addr0;
				fh_otg_iso_ep_start_frm_transfer(core_if, ep);
			}
		}
	} else {
		ep->cur_pkt_addr =
		    (ep->proc_buf_num) ? ep->xfer_buff1 : ep->xfer_buff0;
		ep->cur_pkt_dma_addr =
		    (ep->proc_buf_num) ? ep->dma_addr1 : ep->dma_addr0;
		fh_otg_iso_ep_start_frm_transfer(core_if, ep);
	}
}

/**
 * This function stops transfer for an EP and
 * resets the ep's variables. 
 *
 * @param core_if Programming view of FH_otg controller.
 * @param ep The EP to start the transfer on.
 */

void fh_otg_iso_ep_stop_transfer(fh_otg_core_if_t * core_if, fh_ep_t * ep)
{
	depctl_data_t depctl = {.d32 = 0 };
	volatile uint32_t *addr;

	if (ep->is_in == 1) {
		addr = &core_if->dev_if->in_ep_regs[ep->num]->diepctl;
	} else {
		addr = &core_if->dev_if->out_ep_regs[ep->num]->doepctl;
	}

	/* disable the ep */
	depctl.d32 = FH_READ_REG32(addr);

	depctl.b.epdis = 1;
	depctl.b.snak = 1;

	FH_WRITE_REG32(addr, depctl.d32);

	if (core_if->dma_desc_enable &&
	    ep->iso_desc_addr && ep->iso_dma_desc_addr) {
		fh_otg_ep_free_desc_chain(ep->iso_desc_addr,
					   ep->iso_dma_desc_addr,
					   ep->desc_cnt * 2);
	}

	/* reset varibales */
	ep->dma_addr0 = 0;
	ep->dma_addr1 = 0;
	ep->xfer_buff0 = 0;
	ep->xfer_buff1 = 0;
	ep->data_per_frame = 0;
	ep->data_pattern_frame = 0;
	ep->sync_frame = 0;
	ep->buf_proc_intrvl = 0;
	ep->bInterval = 0;
	ep->proc_buf_num = 0;
	ep->pkt_per_frm = 0;
	ep->pkt_per_frm = 0;
	ep->desc_cnt = 0;
	ep->iso_desc_addr = 0;
	ep->iso_dma_desc_addr = 0;
}

int fh_otg_pcd_iso_ep_start(fh_otg_pcd_t * pcd, void *ep_handle,
			     uint8_t * buf0, uint8_t * buf1, fh_dma_t dma0,
			     fh_dma_t dma1, int sync_frame, int dp_frame,
			     int data_per_frame, int start_frame,
			     int buf_proc_intrvl, void *req_handle,
			     int atomic_alloc)
{
	fh_otg_pcd_ep_t *ep;
	fh_irqflags_t flags = 0;
	fh_ep_t *fh_ep;
	int32_t frm_data;
	dsts_data_t dsts;
	fh_otg_core_if_t *core_if;

	ep = get_ep_from_handle(pcd, ep_handle);

	if (!ep || !ep->desc || ep->fh_ep.num == 0) {
		FH_WARN("bad ep\n");
		return -FH_E_INVALID;
	}

	FH_SPINLOCK_IRQSAVE(pcd->lock, &flags);
	core_if = GET_CORE_IF(pcd);
	fh_ep = &ep->fh_ep;

	if (ep->iso_req_handle) {
		FH_WARN("ISO request in progress\n");
	}

	fh_ep->dma_addr0 = dma0;
	fh_ep->dma_addr1 = dma1;

	fh_ep->xfer_buff0 = buf0;
	fh_ep->xfer_buff1 = buf1;

	fh_ep->data_per_frame = data_per_frame;

	/** @todo - pattern data support is to be implemented in the future */
	fh_ep->data_pattern_frame = dp_frame;
	fh_ep->sync_frame = sync_frame;

	fh_ep->buf_proc_intrvl = buf_proc_intrvl;

	fh_ep->bInterval = 1 << (ep->desc->bInterval - 1);

	fh_ep->proc_buf_num = 0;

	fh_ep->pkt_per_frm = 0;
	frm_data = ep->fh_ep.data_per_frame;
	while (frm_data > 0) {
		fh_ep->pkt_per_frm++;
		frm_data -= ep->fh_ep.maxpacket;
	}

	dsts.d32 = FH_READ_REG32(&core_if->dev_if->dev_global_regs->dsts);

	if (start_frame == -1) {
		fh_ep->next_frame = dsts.b.soffn + 1;
		if (fh_ep->bInterval != 1) {
			fh_ep->next_frame =
			    fh_ep->next_frame + (fh_ep->bInterval - 1 -
						  fh_ep->next_frame %
						  fh_ep->bInterval);
		}
	} else {
		fh_ep->next_frame = start_frame;
	}

	if (!core_if->pti_enh_enable) {
		fh_ep->pkt_cnt =
		    fh_ep->buf_proc_intrvl * fh_ep->pkt_per_frm /
		    fh_ep->bInterval;
	} else {
		fh_ep->pkt_cnt =
		    (fh_ep->data_per_frame *
		     (fh_ep->buf_proc_intrvl / fh_ep->bInterval)
		     - 1 + fh_ep->maxpacket) / fh_ep->maxpacket;
	}

	if (core_if->dma_desc_enable) {
		fh_ep->desc_cnt =
		    fh_ep->buf_proc_intrvl * fh_ep->pkt_per_frm /
		    fh_ep->bInterval;
	}

	if (atomic_alloc) {
		fh_ep->pkt_info =
		    FH_ALLOC_ATOMIC(sizeof(iso_pkt_info_t) * fh_ep->pkt_cnt);
	} else {
		fh_ep->pkt_info =
		    FH_ALLOC(sizeof(iso_pkt_info_t) * fh_ep->pkt_cnt);
	}
	if (!fh_ep->pkt_info) {
		FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
		return -FH_E_NO_MEMORY;
	}
	if (core_if->pti_enh_enable) {
		fh_memset(fh_ep->pkt_info, 0,
			   sizeof(iso_pkt_info_t) * fh_ep->pkt_cnt);
	}

	fh_ep->cur_pkt = 0;
	ep->iso_req_handle = req_handle;

	FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
	fh_otg_iso_ep_start_transfer(core_if, fh_ep);
	return 0;
}

int fh_otg_pcd_iso_ep_stop(fh_otg_pcd_t * pcd, void *ep_handle,
			    void *req_handle)
{
	fh_irqflags_t flags = 0;
	fh_otg_pcd_ep_t *ep;
	fh_ep_t *fh_ep;

	ep = get_ep_from_handle(pcd, ep_handle);
	if (!ep || !ep->desc || ep->fh_ep.num == 0) {
		FH_WARN("bad ep\n");
		return -FH_E_INVALID;
	}
	fh_ep = &ep->fh_ep;

	fh_otg_iso_ep_stop_transfer(GET_CORE_IF(pcd), fh_ep);

	FH_FREE(fh_ep->pkt_info);
	FH_SPINLOCK_IRQSAVE(pcd->lock, &flags);
	if (ep->iso_req_handle != req_handle) {
		FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
		return -FH_E_INVALID;
	}

	FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);

	ep->iso_req_handle = 0;
	return 0;
}

/**
 * This function is used for perodical data exchnage between PCD and gadget drivers.
 * for Isochronous EPs
 *
 *	- Every time a sync period completes this function is called to
 *	  perform data exchange between PCD and gadget
 */
void fh_otg_iso_buffer_done(fh_otg_pcd_t * pcd, fh_otg_pcd_ep_t * ep,
			     void *req_handle)
{
	int i;
	fh_ep_t *fh_ep;

	fh_ep = &ep->fh_ep;

	FH_SPINUNLOCK(ep->pcd->lock);
	pcd->fops->isoc_complete(pcd, ep->priv, ep->iso_req_handle,
				 fh_ep->proc_buf_num ^ 0x1);
	FH_SPINLOCK(ep->pcd->lock);

	for (i = 0; i < fh_ep->pkt_cnt; ++i) {
		fh_ep->pkt_info[i].status = 0;
		fh_ep->pkt_info[i].offset = 0;
		fh_ep->pkt_info[i].length = 0;
	}
}

int fh_otg_pcd_get_iso_packet_count(fh_otg_pcd_t * pcd, void *ep_handle,
				     void *iso_req_handle)
{
	fh_otg_pcd_ep_t *ep;
	fh_ep_t *fh_ep;

	ep = get_ep_from_handle(pcd, ep_handle);
	if (!ep->desc || ep->fh_ep.num == 0) {
		FH_WARN("bad ep\n");
		return -FH_E_INVALID;
	}
	fh_ep = &ep->fh_ep;

	return fh_ep->pkt_cnt;
}

void fh_otg_pcd_get_iso_packet_params(fh_otg_pcd_t * pcd, void *ep_handle,
				       void *iso_req_handle, int packet,
				       int *status, int *actual, int *offset)
{
	fh_otg_pcd_ep_t *ep;
	fh_ep_t *fh_ep;

	ep = get_ep_from_handle(pcd, ep_handle);
	if (!ep)
		FH_WARN("bad ep\n");

	fh_ep = &ep->fh_ep;

	*status = fh_ep->pkt_info[packet].status;
	*actual = fh_ep->pkt_info[packet].length;
	*offset = fh_ep->pkt_info[packet].offset;
}

#endif /* FH_EN_ISOC */

static void fh_otg_pcd_init_ep(fh_otg_pcd_t * pcd, fh_otg_pcd_ep_t * pcd_ep,
				uint32_t is_in, uint32_t ep_num)
{
	/* Init EP structure */
	pcd_ep->desc = 0;
	pcd_ep->pcd = pcd;
	pcd_ep->stopped = 1;
	pcd_ep->queue_sof = 0;

	/* Init FH ep structure */
	pcd_ep->fh_ep.is_in = is_in;
	pcd_ep->fh_ep.num = ep_num;
	pcd_ep->fh_ep.active = 0;
	pcd_ep->fh_ep.tx_fifo_num = 0;
	/* Control until ep is actvated */
	pcd_ep->fh_ep.type = FH_OTG_EP_TYPE_CONTROL;
	pcd_ep->fh_ep.maxpacket = MAX_PACKET_SIZE;
	pcd_ep->fh_ep.dma_addr = 0;
	pcd_ep->fh_ep.start_xfer_buff = 0;
	pcd_ep->fh_ep.xfer_buff = 0;
	pcd_ep->fh_ep.xfer_len = 0;
	pcd_ep->fh_ep.xfer_count = 0;
	pcd_ep->fh_ep.sent_zlp = 0;
	pcd_ep->fh_ep.total_len = 0;
	pcd_ep->fh_ep.desc_addr = 0;
	pcd_ep->fh_ep.dma_desc_addr = 0;
	FH_CIRCLEQ_INIT(&pcd_ep->queue);
}

/**
 * Initialize ep's
 */
static void fh_otg_pcd_reinit(fh_otg_pcd_t * pcd)
{
	int i;
	uint32_t hwcfg1;
	fh_otg_pcd_ep_t *ep;
	int in_ep_cntr, out_ep_cntr;
	uint32_t num_in_eps = (GET_CORE_IF(pcd))->dev_if->num_in_eps;
	uint32_t num_out_eps = (GET_CORE_IF(pcd))->dev_if->num_out_eps;

	/**
	 * Initialize the EP0 structure.
	 */
	ep = &pcd->ep0;
	fh_otg_pcd_init_ep(pcd, ep, 0, 0);

	in_ep_cntr = 0;
	hwcfg1 = (GET_CORE_IF(pcd))->hwcfg1.d32 >> 3;
	for (i = 1; in_ep_cntr < num_in_eps; i++) {
		if ((hwcfg1 & 0x1) == 0) {
			fh_otg_pcd_ep_t *ep = &pcd->in_ep[in_ep_cntr];
			in_ep_cntr++;
			/**
			 * @todo NGS: Add direction to EP, based on contents
			 * of HWCFG1.  Need a copy of HWCFG1 in pcd structure?
			 * sprintf(";r
			 */
			fh_otg_pcd_init_ep(pcd, ep, 1 /* IN */ , i);

			FH_CIRCLEQ_INIT(&ep->queue);
		}
		hwcfg1 >>= 2;
	}

	out_ep_cntr = 0;
	hwcfg1 = (GET_CORE_IF(pcd))->hwcfg1.d32 >> 2;
	for (i = 1; out_ep_cntr < num_out_eps; i++) {
		if ((hwcfg1 & 0x1) == 0) {
			fh_otg_pcd_ep_t *ep = &pcd->out_ep[out_ep_cntr];
			out_ep_cntr++;
			/**
			 * @todo NGS: Add direction to EP, based on contents
			 * of HWCFG1.  Need a copy of HWCFG1 in pcd structure?
			 * sprintf(";r
			 */
			fh_otg_pcd_init_ep(pcd, ep, 0 /* OUT */ , i);
			FH_CIRCLEQ_INIT(&ep->queue);
		}
		hwcfg1 >>= 2;
	}

	pcd->ep0state = EP0_DISCONNECT;
	pcd->ep0.fh_ep.maxpacket = MAX_EP0_SIZE;
	pcd->ep0.fh_ep.type = FH_OTG_EP_TYPE_CONTROL;
}

/**
 * This function is called when the SRP timer expires. The SRP should
 * complete within 6 seconds.
 */
static void srp_timeout(void *ptr)
{
	gotgctl_data_t gotgctl;
	fh_otg_core_if_t *core_if = (fh_otg_core_if_t *) ptr;
	volatile uint32_t *addr = &core_if->core_global_regs->gotgctl;

	gotgctl.d32 = FH_READ_REG32(addr);

	core_if->srp_timer_started = 0;

	if (core_if->adp_enable) {
		if (gotgctl.b.bsesvld == 0) {
			gpwrdn_data_t gpwrdn = {.d32 = 0 };
			FH_PRINTF("SRP Timeout BSESSVLD = 0\n");
			/* Power off the core */
			if (core_if->power_down == 2) {
				gpwrdn.b.pwrdnswtch = 1;
				FH_MODIFY_REG32(&core_if->
						 core_global_regs->gpwrdn,
						 gpwrdn.d32, 0);
			}

			gpwrdn.d32 = 0;
			gpwrdn.b.pmuintsel = 1;
			gpwrdn.b.pmuactv = 1;
			FH_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0,
					 gpwrdn.d32);
			fh_otg_adp_probe_start(core_if);
		} else {
			FH_PRINTF("SRP Timeout BSESSVLD = 1\n");
			core_if->op_state = B_PERIPHERAL;
			fh_otg_core_init(core_if);
			fh_otg_enable_global_interrupts(core_if);
			cil_pcd_start(core_if);
		}
	}

	if ((core_if->core_params->phy_type == FH_PHY_TYPE_PARAM_FS) &&
	    (core_if->core_params->i2c_enable)) {
		FH_PRINTF("SRP Timeout\n");

		if ((core_if->srp_success) && (gotgctl.b.bsesvld)) {
			if (core_if->pcd_cb && core_if->pcd_cb->resume_wakeup) {
				core_if->pcd_cb->resume_wakeup(core_if->pcd_cb->p);
			}

			/* Clear Session Request */
			gotgctl.d32 = 0;
			gotgctl.b.sesreq = 1;
			FH_MODIFY_REG32(&core_if->core_global_regs->gotgctl,
					 gotgctl.d32, 0);

			core_if->srp_success = 0;
		} else {
			__FH_ERROR("Device not connected/responding\n");
			gotgctl.b.sesreq = 0;
			FH_WRITE_REG32(addr, gotgctl.d32);
		}
	} else if (gotgctl.b.sesreq) {
		FH_PRINTF("SRP Timeout\n");

		__FH_ERROR("Device not connected/responding\n");
		gotgctl.b.sesreq = 0;
		FH_WRITE_REG32(addr, gotgctl.d32);
	} else {
		FH_PRINTF(" SRP GOTGCTL=%0x\n", gotgctl.d32);
	}
}

/**
 * Tasklet
 *
 */
extern void start_next_request(fh_otg_pcd_ep_t * ep);

static void start_xfer_tasklet_func(void *data)
{
	fh_otg_pcd_t *pcd = (fh_otg_pcd_t *) data;
	fh_otg_core_if_t *core_if = GET_CORE_IF(pcd);

	int i;
	depctl_data_t diepctl;

	FH_DEBUGPL(DBG_PCDV, "Start xfer tasklet\n");

	diepctl.d32 = FH_READ_REG32(&core_if->dev_if->in_ep_regs[0]->diepctl);

	if (pcd->ep0.queue_sof) {
		pcd->ep0.queue_sof = 0;
		start_next_request(&pcd->ep0);
		// break;
	}

	for (i = 0; i < core_if->dev_if->num_in_eps; i++) {
		depctl_data_t diepctl;
		diepctl.d32 =
		    FH_READ_REG32(&core_if->dev_if->in_ep_regs[i]->diepctl);

		if (pcd->in_ep[i].queue_sof) {
			pcd->in_ep[i].queue_sof = 0;
			start_next_request(&pcd->in_ep[i]);
			// break;
		}
	}

	return;
}

/**
 * This function initialized the PCD portion of the driver.
 *
 */
fh_otg_pcd_t *fh_otg_pcd_init(fh_otg_core_if_t * core_if)
{
	fh_otg_pcd_t *pcd = NULL;
	fh_otg_dev_if_t *dev_if;
	int i;

	/*
	 * Allocate PCD structure
	 */
	pcd = FH_ALLOC(sizeof(fh_otg_pcd_t));

	if (pcd == NULL) {
		return NULL;
	}

	pcd->lock = FH_SPINLOCK_ALLOC();
	if (!pcd->lock) {
		FH_ERROR("Could not allocate lock for pcd");
		FH_FREE(pcd);
		return NULL;
	}
	/* Set core_if's lock pointer to hcd->lock */
	core_if->lock = pcd->lock;
	pcd->core_if = core_if;

	dev_if = core_if->dev_if;
	dev_if->isoc_ep = NULL;

	if (core_if->hwcfg4.b.ded_fifo_en) {
		FH_PRINTF("Dedicated Tx FIFOs mode\n");
	} else {
		FH_PRINTF("Shared Tx FIFO mode\n");
	}

	/*
	 * Initialized the Core for Device mode here if there is nod ADP support. 
	 * Otherwise it will be done later in fh_otg_adp_start routine.
	 */
	if (fh_otg_is_device_mode(core_if) /*&& !core_if->adp_enable */ ) {
		fh_otg_core_dev_init(core_if);
	}

	/*
	 * Register the PCD Callbacks.
	 */
	fh_otg_cil_register_pcd_callbacks(core_if, &pcd_callbacks, pcd);

	/*
	 * Initialize the DMA buffer for SETUP packets
	 */
	if (GET_CORE_IF(pcd)->dma_enable) {
		pcd->setup_pkt =
		    FH_DMA_ALLOC(sizeof(*pcd->setup_pkt) * 5,
				  &pcd->setup_pkt_dma_handle);
		if (pcd->setup_pkt == NULL) {
			FH_FREE(pcd);
			return NULL;
		}

		pcd->status_buf =
		    FH_DMA_ALLOC(sizeof(uint16_t),
				  &pcd->status_buf_dma_handle);
		if (pcd->status_buf == NULL) {
			FH_DMA_FREE(sizeof(*pcd->setup_pkt) * 5,
				     pcd->setup_pkt, pcd->setup_pkt_dma_handle);
			FH_FREE(pcd);
			return NULL;
		}

		if (GET_CORE_IF(pcd)->dma_desc_enable) {
			dev_if->setup_desc_addr[0] =
			    fh_otg_ep_alloc_desc_chain
			    (&dev_if->dma_setup_desc_addr[0], 1);
			dev_if->setup_desc_addr[1] =
			    fh_otg_ep_alloc_desc_chain
			    (&dev_if->dma_setup_desc_addr[1], 1);
			dev_if->in_desc_addr =
			    fh_otg_ep_alloc_desc_chain
			    (&dev_if->dma_in_desc_addr, 1);
			dev_if->out_desc_addr =
			    fh_otg_ep_alloc_desc_chain
			    (&dev_if->dma_out_desc_addr, 1);
			pcd->data_terminated = 0;

			if (dev_if->setup_desc_addr[0] == 0
			    || dev_if->setup_desc_addr[1] == 0
			    || dev_if->in_desc_addr == 0
			    || dev_if->out_desc_addr == 0) {

				if (dev_if->out_desc_addr)
					fh_otg_ep_free_desc_chain
					    (dev_if->out_desc_addr,
					     dev_if->dma_out_desc_addr, 1);
				if (dev_if->in_desc_addr)
					fh_otg_ep_free_desc_chain
					    (dev_if->in_desc_addr,
					     dev_if->dma_in_desc_addr, 1);
				if (dev_if->setup_desc_addr[1])
					fh_otg_ep_free_desc_chain
					    (dev_if->setup_desc_addr[1],
					     dev_if->dma_setup_desc_addr[1], 1);
				if (dev_if->setup_desc_addr[0])
					fh_otg_ep_free_desc_chain
					    (dev_if->setup_desc_addr[0],
					     dev_if->dma_setup_desc_addr[0], 1);

				FH_DMA_FREE(sizeof(*pcd->setup_pkt) * 5,
					     pcd->setup_pkt,
					     pcd->setup_pkt_dma_handle);
				FH_DMA_FREE(sizeof(*pcd->status_buf),
					     pcd->status_buf,
					     pcd->status_buf_dma_handle);

				FH_FREE(pcd);

				return NULL;
			}
		}
	} else {
		pcd->setup_pkt = FH_ALLOC(sizeof(*pcd->setup_pkt) * 5);
		if (pcd->setup_pkt == NULL) {
			FH_FREE(pcd);
			return NULL;
		}

		pcd->status_buf = FH_ALLOC(sizeof(uint16_t));
		if (pcd->status_buf == NULL) {
			FH_FREE(pcd->setup_pkt);
			FH_FREE(pcd);
			return NULL;
		}
	}

	fh_otg_pcd_reinit(pcd);

	/* Allocate the cfi object for the PCD */
#ifdef FH_UTE_CFI
	pcd->cfi = FH_ALLOC(sizeof(cfiobject_t));
	if (NULL == pcd->cfi)
		goto fail;
	if (init_cfi(pcd->cfi)) {
		CFI_INFO("%s: Failed to init the CFI object\n", __func__);
		goto fail;
	}
#endif

	/* Initialize tasklets */
	pcd->start_xfer_tasklet = FH_TASK_ALLOC("xfer_tasklet",
						 start_xfer_tasklet_func, pcd);
	pcd->test_mode_tasklet = FH_TASK_ALLOC("test_mode_tasklet",
						do_test_mode, pcd);

	/* Initialize SRP timer */
	core_if->srp_timer = FH_TIMER_ALLOC("SRP TIMER", srp_timeout, core_if);

	if (core_if->core_params->dev_out_nak) {
		/** 
		* Initialize xfer timeout timer. Implemented for
		* 2.93a feature "Device DDMA OUT NAK Enhancement"
		*/
		for (i = 0; i < MAX_EPS_CHANNELS; i++) {
			pcd->core_if->ep_xfer_timer[i] =
			    FH_TIMER_ALLOC("ep timer", ep_xfer_timeout,
					    &pcd->core_if->ep_xfer_info[i]);
		}
	}

	return pcd;
#ifdef FH_UTE_CFI
fail:
#endif
	if (pcd->setup_pkt)
		FH_FREE(pcd->setup_pkt);
	if (pcd->status_buf)
		FH_FREE(pcd->status_buf);
#ifdef FH_UTE_CFI
	if (pcd->cfi)
		FH_FREE(pcd->cfi);
#endif
	if (pcd)
		FH_FREE(pcd);
	return NULL;

}

/**
 * Remove PCD specific data
 */
void fh_otg_pcd_remove(fh_otg_pcd_t * pcd)
{
	fh_otg_dev_if_t *dev_if = GET_CORE_IF(pcd)->dev_if;
	int i;
	if (pcd->core_if->core_params->dev_out_nak) {
		for (i = 0; i < MAX_EPS_CHANNELS; i++) {
			FH_TIMER_CANCEL(pcd->core_if->ep_xfer_timer[i]);
			pcd->core_if->ep_xfer_info[i].state = 0;
		}
	}

	if (GET_CORE_IF(pcd)->dma_enable) {
		FH_DMA_FREE(sizeof(*pcd->setup_pkt) * 5, pcd->setup_pkt,
			     pcd->setup_pkt_dma_handle);
		FH_DMA_FREE(sizeof(uint16_t), pcd->status_buf,
			     pcd->status_buf_dma_handle);
		if (GET_CORE_IF(pcd)->dma_desc_enable) {
			fh_otg_ep_free_desc_chain(dev_if->setup_desc_addr[0],
						   dev_if->dma_setup_desc_addr
						   [0], 1);
			fh_otg_ep_free_desc_chain(dev_if->setup_desc_addr[1],
						   dev_if->dma_setup_desc_addr
						   [1], 1);
			fh_otg_ep_free_desc_chain(dev_if->in_desc_addr,
						   dev_if->dma_in_desc_addr, 1);
			fh_otg_ep_free_desc_chain(dev_if->out_desc_addr,
						   dev_if->dma_out_desc_addr,
						   1);
		}
	} else {
		FH_FREE(pcd->setup_pkt);
		FH_FREE(pcd->status_buf);
	}
	FH_SPINLOCK_FREE(pcd->lock);
	/* Set core_if's lock pointer to NULL */
	pcd->core_if->lock = NULL;

	FH_TASK_FREE(pcd->start_xfer_tasklet);
	FH_TASK_FREE(pcd->test_mode_tasklet);
	if (pcd->core_if->core_params->dev_out_nak) {
		for (i = 0; i < MAX_EPS_CHANNELS; i++) {
			if (pcd->core_if->ep_xfer_timer[i]) {
				FH_TIMER_FREE(pcd->core_if->ep_xfer_timer[i]);
			}
		}
	}

/* Release the CFI object's dynamic memory */
#ifdef FH_UTE_CFI
	if (pcd->cfi->ops.release) {
		pcd->cfi->ops.release(pcd->cfi);
	}
#endif

	FH_FREE(pcd);
}

/**
 * Returns whether registered pcd is dual speed or not
 */
uint32_t fh_otg_pcd_is_dualspeed(fh_otg_pcd_t * pcd)
{
	fh_otg_core_if_t *core_if = GET_CORE_IF(pcd);

	if ((core_if->core_params->speed == FH_SPEED_PARAM_FULL) ||
	    ((core_if->hwcfg2.b.hs_phy_type == 2) &&
	     (core_if->hwcfg2.b.fs_phy_type == 1) &&
	     (core_if->core_params->ulpi_fs_ls))) {
		return 0;
	}

	return 1;
}

/**
 * Returns whether registered pcd is OTG capable or not
 */
uint32_t fh_otg_pcd_is_otg(fh_otg_pcd_t * pcd)
{
	fh_otg_core_if_t *core_if = GET_CORE_IF(pcd);
	gusbcfg_data_t usbcfg = {.d32 = 0 };
	uint32_t retval = 0;

	usbcfg.d32 = FH_READ_REG32(&core_if->core_global_regs->gusbcfg);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,6,0)
	if (!usbcfg.b.srpcap || !usbcfg.b.hnpcap)
		return 0;
	else 
		return 1;
# else
	if (!usbcfg.b.srpcap)
		return 0;
	else 
		retval |= 1;

	if (usbcfg.b.hnpcap)
		retval |= 2;
	
	if (core_if->adp_enable) 
		retval |= 4;
#endif

	return retval;
}

/**
 * This function assigns periodic Tx FIFO to an periodic EP
 * in shared Tx FIFO mode
 */
static uint32_t assign_tx_fifo(fh_otg_core_if_t * core_if)
{
	uint32_t TxMsk = 1;
	int i;

	for (i = 0; i < core_if->hwcfg4.b.num_in_eps; ++i) {
		if ((TxMsk & core_if->tx_msk) == 0) {
			core_if->tx_msk |= TxMsk;
			return i + 1;
		}
		TxMsk <<= 1;
	}
	return 0;
}

/**
 * This function assigns periodic Tx FIFO to an periodic EP
 * in shared Tx FIFO mode
 */
static uint32_t assign_perio_tx_fifo(fh_otg_core_if_t * core_if)
{
	uint32_t PerTxMsk = 1;
	int i;
	for (i = 0; i < core_if->hwcfg4.b.num_dev_perio_in_ep; ++i) {
		if ((PerTxMsk & core_if->p_tx_msk) == 0) {
			core_if->p_tx_msk |= PerTxMsk;
			return i + 1;
		}
		PerTxMsk <<= 1;
	}
	return 0;
}

/**
 * This function releases periodic Tx FIFO
 * in shared Tx FIFO mode
 */
static void release_perio_tx_fifo(fh_otg_core_if_t * core_if,
				  uint32_t fifo_num)
{
	core_if->p_tx_msk =
	    (core_if->p_tx_msk & (1 << (fifo_num - 1))) ^ core_if->p_tx_msk;
}

/**
 * This function releases periodic Tx FIFO
 * in shared Tx FIFO mode
 */
static void release_tx_fifo(fh_otg_core_if_t * core_if, uint32_t fifo_num)
{
	core_if->tx_msk =
	    (core_if->tx_msk & (1 << (fifo_num - 1))) ^ core_if->tx_msk;
}

/**
 * This function is being called from gadget 
 * to enable PCD endpoint.
 */
int fh_otg_pcd_ep_enable(fh_otg_pcd_t * pcd,
			  const uint8_t * ep_desc, void *usb_ep)
{
	int num, dir;
	fh_otg_pcd_ep_t *ep = NULL;
	const usb_endpoint_descriptor_t *desc;
	fh_irqflags_t flags;
	fifosize_data_t dptxfsiz = {.d32 = 0 };
	gdfifocfg_data_t gdfifocfg = {.d32 = 0 };
	gdfifocfg_data_t gdfifocfgbase = {.d32 = 0 };
	int retval = 0;
	int i, epcount;

	desc = (const usb_endpoint_descriptor_t *)ep_desc;

	if (!desc) {
		pcd->ep0.priv = usb_ep;
		ep = &pcd->ep0;
		retval = -FH_E_INVALID;
		goto out;
	}

	num = UE_GET_ADDR(desc->bEndpointAddress);
	dir = UE_GET_DIR(desc->bEndpointAddress);

	if (!desc->wMaxPacketSize) {
		FH_WARN("bad maxpacketsize\n");
		retval = -FH_E_INVALID;
		goto out;
	}

	if (dir == UE_DIR_IN) {
		epcount = pcd->core_if->dev_if->num_in_eps;
		for (i = 0; i < epcount; i++) {
			if (num == pcd->in_ep[i].fh_ep.num) {
				ep = &pcd->in_ep[i];
				break;
			}
		}
	} else {
		epcount = pcd->core_if->dev_if->num_out_eps;
		for (i = 0; i < epcount; i++) {
			if (num == pcd->out_ep[i].fh_ep.num) {
				ep = &pcd->out_ep[i];
				break;
			}
		}
	}

	if (!ep) {
		FH_WARN("bad address\n");
		retval = -FH_E_INVALID;
		goto out;
	}

	FH_SPINLOCK_IRQSAVE(pcd->lock, &flags);

	ep->desc = desc;
	ep->priv = usb_ep;

	/*
	 * Activate the EP
	 */
	ep->stopped = 0;

	ep->fh_ep.is_in = (dir == UE_DIR_IN);
	ep->fh_ep.maxpacket = UGETW(desc->wMaxPacketSize);

	ep->fh_ep.type = desc->bmAttributes & UE_XFERTYPE;

	if (ep->fh_ep.is_in) {
		if (!GET_CORE_IF(pcd)->en_multiple_tx_fifo) {
			ep->fh_ep.tx_fifo_num = 0;

			if (ep->fh_ep.type == UE_ISOCHRONOUS) {
				/*
				 * if ISOC EP then assign a Periodic Tx FIFO.
				 */
				ep->fh_ep.tx_fifo_num =
				    assign_perio_tx_fifo(GET_CORE_IF(pcd));
			}
		} else {
			/*
			 * if Dedicated FIFOs mode is on then assign a Tx FIFO.
			 */
			ep->fh_ep.tx_fifo_num =
			    assign_tx_fifo(GET_CORE_IF(pcd));
		}

		/* Calculating EP info controller base address */
		if (ep->fh_ep.tx_fifo_num
		    && GET_CORE_IF(pcd)->en_multiple_tx_fifo) {
			gdfifocfg.d32 =
			    FH_READ_REG32(&GET_CORE_IF(pcd)->
					   core_global_regs->gdfifocfg);
			gdfifocfgbase.d32 = gdfifocfg.d32 >> 16;
			dptxfsiz.d32 =
			    (FH_READ_REG32
			     (&GET_CORE_IF(pcd)->core_global_regs->
			      dtxfsiz[ep->fh_ep.tx_fifo_num - 1]) >> 16);
			gdfifocfg.b.epinfobase =
			    gdfifocfgbase.d32 + dptxfsiz.d32;
			if (GET_CORE_IF(pcd)->snpsid <= OTG_CORE_REV_2_94a) {
				FH_WRITE_REG32(&GET_CORE_IF(pcd)->
						core_global_regs->gdfifocfg,
						gdfifocfg.d32);
			}
		}
	}
	/* Set initial data PID. */
	if (ep->fh_ep.type == UE_BULK) {
		ep->fh_ep.data_pid_start = 0;
	}

	/* Alloc DMA Descriptors */
	if (GET_CORE_IF(pcd)->dma_desc_enable) {
#ifndef FH_UTE_PER_IO
		if (ep->fh_ep.type != UE_ISOCHRONOUS) {
#endif
			ep->fh_ep.desc_addr =
			    fh_otg_ep_alloc_desc_chain(&ep->
							fh_ep.dma_desc_addr,
							MAX_DMA_DESC_CNT);
			if (!ep->fh_ep.desc_addr) {
				FH_WARN("%s, can't allocate DMA descriptor\n",
					 __func__);
				retval = -FH_E_SHUTDOWN;
				FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
				goto out;
			}
#ifndef FH_UTE_PER_IO
		} else {
			ep->fh_ep.desc_addr =
				fh_otg_ep_alloc_desc_chain(&ep->
				fh_ep.dma_desc_addr,
				MAX_DMA_DESC_CNT/2);
			ep->fh_ep.desc_addr1 =
				fh_otg_ep_alloc_desc_chain(&ep->
				fh_ep.dma_desc_addr1,
				MAX_DMA_DESC_CNT/2);
			if (!ep->fh_ep.desc_addr || !ep->fh_ep.desc_addr1) {
				FH_WARN("%s, can't allocate DMA descriptor\n",
					__func__);
				retval = -FH_E_SHUTDOWN;
				FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
				goto out;
			}
			/* Set initial data PID. */
			if (ep->fh_ep.type == UE_ISOCHRONOUS) {
				ep->fh_ep.iso_desc_first = 0;
				ep->fh_ep.iso_desc_second = 0;
				ep->fh_ep.iso_transfer_started = 0;
			}
		}
#endif
	}

	FH_DEBUGPL(DBG_PCD, "Activate %s: type=%d, mps=%d desc=%p\n",
		    (ep->fh_ep.is_in ? "IN" : "OUT"),
		    ep->fh_ep.type, ep->fh_ep.maxpacket, ep->desc);
#ifdef FH_UTE_PER_IO
	ep->fh_ep.xiso_bInterval = 1 << (ep->desc->bInterval - 1);
#endif
	if (ep->fh_ep.type == FH_OTG_EP_TYPE_ISOC) {
		ep->fh_ep.bInterval = 1 << (ep->desc->bInterval - 1);
		ep->fh_ep.frame_num = 0xFFFFFFFF;
	}

	fh_otg_ep_activate(GET_CORE_IF(pcd), &ep->fh_ep);

#ifdef FH_UTE_CFI
	if (pcd->cfi->ops.ep_enable) {
		pcd->cfi->ops.ep_enable(pcd->cfi, pcd, ep);
	}
#endif

	FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);

out:
	return retval;
}

/**
 * This function is being called from gadget 
 * to disable PCD endpoint.
 */
int fh_otg_pcd_ep_disable(fh_otg_pcd_t * pcd, void *ep_handle)
{
	fh_otg_pcd_ep_t *ep;
	fh_irqflags_t flags;
	fh_otg_dev_dma_desc_t *desc_addr;
	fh_dma_t dma_desc_addr;
	gdfifocfg_data_t gdfifocfgbase = {.d32 = 0 };
	gdfifocfg_data_t gdfifocfg = {.d32 = 0 };
	fifosize_data_t dptxfsiz = {.d32 = 0 };

	ep = get_ep_from_handle(pcd, ep_handle);

	if (!ep || !ep->desc) {
		FH_DEBUGPL(DBG_PCD, "bad ep address\n");
		return -FH_E_INVALID;
	}

	FH_SPINLOCK_IRQSAVE(pcd->lock, &flags);

	fh_otg_request_nuke(ep);

	fh_otg_ep_deactivate(GET_CORE_IF(pcd), &ep->fh_ep);
	if (pcd->core_if->core_params->dev_out_nak) {
		FH_TIMER_CANCEL(pcd->core_if->ep_xfer_timer[ep->fh_ep.num]);
		pcd->core_if->ep_xfer_info[ep->fh_ep.num].state = 0;
	}
	ep->desc = NULL;
	ep->stopped = 1;

	gdfifocfg.d32 =
	    FH_READ_REG32(&GET_CORE_IF(pcd)->core_global_regs->gdfifocfg);
	gdfifocfgbase.d32 = gdfifocfg.d32 >> 16;

	if (ep->fh_ep.is_in) {
		if (GET_CORE_IF(pcd)->en_multiple_tx_fifo) {
			/* Flush the Tx FIFO */
			fh_otg_flush_tx_fifo(GET_CORE_IF(pcd),
					      ep->fh_ep.tx_fifo_num);
		}
		release_perio_tx_fifo(GET_CORE_IF(pcd), ep->fh_ep.tx_fifo_num);
		release_tx_fifo(GET_CORE_IF(pcd), ep->fh_ep.tx_fifo_num);
		if (GET_CORE_IF(pcd)->en_multiple_tx_fifo) {
			/* Decreasing EPinfo Base Addr */
			dptxfsiz.d32 =
			    (FH_READ_REG32
			     (&GET_CORE_IF(pcd)->
		      		core_global_regs->dtxfsiz[ep->fh_ep.tx_fifo_num-1]) >> 16);
			gdfifocfg.b.epinfobase = gdfifocfgbase.d32 - dptxfsiz.d32;
			if (GET_CORE_IF(pcd)->snpsid <= OTG_CORE_REV_2_94a) {
				FH_WRITE_REG32(&GET_CORE_IF(pcd)->core_global_regs->gdfifocfg,
						gdfifocfg.d32);
			}
		}
	}

	/* Free DMA Descriptors */
	if (GET_CORE_IF(pcd)->dma_desc_enable) {
		if (ep->fh_ep.type != UE_ISOCHRONOUS) {
			desc_addr = ep->fh_ep.desc_addr;
			dma_desc_addr = ep->fh_ep.dma_desc_addr;

			/* Cannot call dma_free_coherent() with IRQs disabled */
			FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
			fh_otg_ep_free_desc_chain(desc_addr, dma_desc_addr,
						   MAX_DMA_DESC_CNT);

		} else {
			desc_addr = ep->fh_ep.desc_addr;
			dma_desc_addr = ep->fh_ep.dma_desc_addr;

			/* Cannot call dma_free_coherent() with IRQs disabled */
			FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
			fh_otg_ep_free_desc_chain(desc_addr, dma_desc_addr,
				MAX_DMA_DESC_CNT/2);
			desc_addr = ep->fh_ep.desc_addr1;
			dma_desc_addr = ep->fh_ep.dma_desc_addr1;
			fh_otg_ep_free_desc_chain(desc_addr, dma_desc_addr,
				MAX_DMA_DESC_CNT/2);
		}
		goto out_unlocked;
	}
	FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);

out_unlocked:
	FH_DEBUGPL(DBG_PCD, "%d %s disabled\n", ep->fh_ep.num,
		    ep->fh_ep.is_in ? "IN" : "OUT");
	return 0;

}

/**
 * This function initializes dma descriptor chain for ISOC transfers.
 *
 * @param core_if Programming view of FH_otg controller.
 * @param ep The EP to start the transfer on.
 */
void fh_otg_pcd_start_iso_ddma(fh_otg_core_if_t * core_if, fh_otg_pcd_ep_t * ep)
{
	fh_otg_dev_dma_desc_t *dma_desc;
	fh_otg_pcd_request_t *req = NULL;
	fh_ep_t *fhep = NULL;
	uint32_t frame_num = 0;
	int i = 0;
	int j;
	int sync_request = 4;
	uint16_t nat;
	depctl_data_t depctl;                                                                                                                                                                                                           

	fhep = &ep->fh_ep;
	dma_desc = fhep->desc_addr;

	nat = UGETW(ep->desc->wMaxPacketSize);
	nat = (nat >> 11) & 0x03;
	FH_DEBUGPL(DBG_PCD, "nat=%u binterval =%02x\n",nat, fhep->bInterval);
	FH_DEBUGPL(DBG_PCD, "frame_num =  %d\n", fhep->frame_num);

	/* Complete first three IN EP requests for the synchronization */
	if (fhep->is_in) {
		if (!FH_CIRCLEQ_EMPTY(&ep->queue)) {
			for (j = 0; j < sync_request; j++) {
				req = FH_CIRCLEQ_FIRST(&ep->queue);
				if (!req) {
					FH_PRINTF("ISOC 0x%p, req = NULL!\n", ep);
					return;
				} else {
					/* Complete first request */
					req->actual = 0;
					fh_otg_request_done(ep, req, 0);
				}
			}
		} else {
			FH_PRINTF("ISOC ep 0x%p, ep->queue empty!\n", ep);
			return;
		}

		frame_num = fhep->frame_num + (sync_request -1)*fhep->bInterval;
		
		FH_CIRCLEQ_FOREACH(req, &ep->queue, queue_entry) {
			i = i+1;
			frame_num = (frame_num + fhep->bInterval) & 0x3FFF;
			/** DMA Descriptor Setup */
			dma_desc->status.b_iso_in.bs = BS_HOST_BUSY;
			dma_desc->buf = req->dma;
			dma_desc->status.b_iso_in.txbytes = req->length;
			dma_desc->status.b_iso_in.framenum = frame_num;
			dma_desc->status.b_iso_in.txsts = 0;
			dma_desc->status.b_iso_in.sp = (req->length % fhep->maxpacket) ? 1 : 0;
			dma_desc->status.b_iso_in.ioc = 1;
			dma_desc->status.b_iso_in.pid = nat + 1;
			dma_desc->status.b_iso_in.l = 0;
			
			if (req == FH_CIRCLEQ_LAST(&ep->queue)) {
				dma_desc->status.b_iso_in.l = 1;
			}
			dma_desc->status.b_iso_in.bs = BS_HOST_READY;
			FH_DEBUGPL(DBG_PCD, "ISO_DESC #%d %p status = %08x\n", i, dma_desc, dma_desc->status.d32);
			if (i == MAX_DMA_DESC_CNT/2 - 1) {
				dma_desc->status.b_iso_in.l = 1;
				break;
			}
			dma_desc++;
		}
		FH_WRITE_REG32(&core_if->dev_if->in_ep_regs[fhep->num]->diepdma, fhep->dma_desc_addr);
 		FH_DEBUGPL(DBG_PCD, "%d ISOC IN descs were programmed\n", i-1);
		depctl.d32 = 0;
		depctl.b.epena = 1;
		depctl.b.cnak = 1;
		FH_MODIFY_REG32(&core_if->dev_if->in_ep_regs[fhep->num]->diepctl, 0, depctl.d32);
	} else {
		FH_CIRCLEQ_FOREACH(req, &ep->queue, queue_entry) {
			i = i+1;
			frame_num = (frame_num + fhep->bInterval) & 0x3FFF;
			/** DMA Descriptor Setup */
			dma_desc->status.b_iso_out.bs = BS_HOST_BUSY;
			dma_desc->buf = req->dma;
			dma_desc->status.b_iso_out.rxbytes = req->length;
			dma_desc->status.b_iso_out.rxsts = 0;
			dma_desc->status.b_iso_out.sp = (req->length % fhep->maxpacket) ? 1 : 0;
			dma_desc->status.b_iso_out.ioc = 1;
			dma_desc->status.b_iso_out.pid = nat + 1;
			dma_desc->status.b_iso_out.l = 0;

			if (req == FH_CIRCLEQ_LAST(&ep->queue)) {
				dma_desc->status.b_iso_out.l = 1;
			}
			dma_desc->status.b_iso_in.bs = BS_HOST_READY;
			FH_DEBUGPL(DBG_PCD, "ISO_DESC #%d %p status = %08x\n", i, dma_desc, dma_desc->status.d32);
			if (i == MAX_DMA_DESC_CNT/2 - 1) {
				dma_desc->status.b_iso_out.l = 1;
				break;
			}
			dma_desc++;
		}
		FH_WRITE_REG32(&core_if->dev_if->out_ep_regs[fhep->num]->doepdma, fhep->dma_desc_addr);
		FH_DEBUGPL(DBG_PCD, "%d ISOC OUT descs were programmed\n", i-1);
		depctl.d32 = 0;
		depctl.b.epena = 1;
		depctl.b.cnak = 1;
		FH_MODIFY_REG32(&core_if->dev_if->out_ep_regs[fhep->num]->doepctl, 0, depctl.d32);
	}
	fhep->iso_desc_first = i; //vahrama - pay attention previous one was i-1
	fhep->iso_transfer_started = 1;
	fhep->frame_num = frame_num;
	fhep->use_add_buf = 1;
}
/**
 * Program next ISO request to the DMA chain
 *
 */
static void program_next_iso_request_ddma (fh_otg_pcd_ep_t * ep, fh_otg_pcd_request_t * req)
{
	fh_otg_dev_dma_desc_t *dma_desc;
	fh_dma_t dma_desc_addr;
	uint32_t frame_num = 0;
	uint32_t nat;
	uint32_t index;

	FH_DEBUGPL(DBG_PCD, "%s", __FUNCTION__);

	if (ep->fh_ep.use_add_buf) {
		index =	ep->fh_ep.iso_desc_second + 1;
	} else {
		index =	ep->fh_ep.iso_desc_first + 1;
	}

	if (index > MAX_DMA_DESC_CNT/2) {
		FH_PRINTF("There are no free descs in the chain!\n");
		return;
	} 

	if (ep->fh_ep.use_add_buf) {
		dma_desc = &ep->fh_ep.desc_addr1[ep->fh_ep.iso_desc_second];
		dma_desc_addr = ep->fh_ep.dma_desc_addr1;
		ep->fh_ep.iso_desc_second += 1;
	}  else {
		dma_desc = &ep->fh_ep.desc_addr[ep->fh_ep.iso_desc_first];
		dma_desc_addr = ep->fh_ep.dma_desc_addr;
		ep->fh_ep.iso_desc_first += 1;
	}
	nat = UGETW(ep->desc->wMaxPacketSize);
	nat = (nat >> 11) & 0x03;
	frame_num = (ep->fh_ep.frame_num + ep->fh_ep.bInterval) & 0x3FFF;
	if (ep->fh_ep.is_in) {
		/** DMA Descriptor Setup */
		dma_desc->status.b_iso_in.bs = BS_HOST_BUSY;
		dma_desc->buf = req->dma;
		dma_desc->status.b_iso_in.txbytes = req->length;
		dma_desc->status.b_iso_in.framenum = frame_num;
		dma_desc->status.b_iso_in.txsts = 0;
		dma_desc->status.b_iso_in.sp = (req->length % ep->fh_ep.maxpacket) ? 1 : 0;
		dma_desc->status.b_iso_in.ioc = 1;
		dma_desc->status.b_iso_in.pid = nat + 1;
		dma_desc->status.b_iso_in.l = 1;

		dma_desc->status.b_iso_in.bs = BS_HOST_READY;

		/* Clear L bit on the previous desc of the chain */
		if (index > 1) {
			dma_desc--;
			dma_desc->status.b_iso_in.l = 0;
		}					
	}  else {
		/** DMA Descriptor Setup */
		dma_desc->status.b_iso_out.bs = BS_HOST_BUSY;
		dma_desc->buf = req->dma;
		dma_desc->status.b_iso_out.rxbytes = req->length;
		dma_desc->status.b_iso_out.rxsts = 0;
		dma_desc->status.b_iso_out.sp = (req->length % ep->fh_ep.maxpacket) ? 1 : 0;
		dma_desc->status.b_iso_out.ioc = 1;
		dma_desc->status.b_iso_out.pid = nat + 1;
		dma_desc->status.b_iso_out.l = 1;

		dma_desc->status.b_iso_out.bs = BS_HOST_READY;

		/* Clear L bit on the previous desc of the chain */
		if (index > 1) {
			dma_desc--;
			dma_desc->status.b_iso_out.l = 0;
		}
	}
	ep->fh_ep.frame_num = frame_num;

}

/******************************************************************************/
#ifdef FH_UTE_PER_IO

/**
 * Free the request and its extended parts
 *
 */
void fh_pcd_xiso_ereq_free(fh_otg_pcd_ep_t * ep, fh_otg_pcd_request_t * req)
{
	FH_FREE(req->ext_req.per_io_frame_descs);
	FH_FREE(req);
}

/**
 * Start the next request in the endpoint's queue.
 *
 */
int fh_otg_pcd_xiso_start_next_request(fh_otg_pcd_t * pcd,
					fh_otg_pcd_ep_t * ep)
{
	int i;
	fh_otg_pcd_request_t *req = NULL;
	fh_ep_t *fhep = NULL;
	struct fh_iso_xreq_port *ereq = NULL;
	struct fh_iso_pkt_desc_port *ddesc_iso;
	uint16_t nat;
	depctl_data_t diepctl;

	fhep = &ep->fh_ep;

	if (fhep->xiso_active_xfers > 0) {
#if 0	//Disable this to decrease s/w overhead that is crucial for Isoc transfers
		FH_WARN("There are currently active transfers for EP%d \
				(active=%d; queued=%d)", fhep->num, fhep->xiso_active_xfers, 
				fhep->xiso_queued_xfers);
#endif
		return 0;
	}

	nat = UGETW(ep->desc->wMaxPacketSize);
	nat = (nat >> 11) & 0x03;

	if (!FH_CIRCLEQ_EMPTY(&ep->queue)) {
		req = FH_CIRCLEQ_FIRST(&ep->queue);
		ereq = &req->ext_req;
		ep->stopped = 0;

		/* Get the frame number */
		fhep->xiso_frame_num =
		    fh_otg_get_frame_number(GET_CORE_IF(pcd));
		FH_DEBUG("FRM_NUM=%d", fhep->xiso_frame_num);

		ddesc_iso = ereq->per_io_frame_descs;

		if (fhep->is_in) {
			/* Setup DMA Descriptor chain for IN Isoc request */
			for (i = 0; i < ereq->pio_pkt_count; i++) {
				//if ((i % (nat + 1)) == 0)
				if (i > 0)
					fhep->xiso_frame_num =
					    (fhep->xiso_bInterval +
					     fhep->xiso_frame_num) & 0x3FFF;
				fhep->desc_addr[i].buf =
				    req->dma + ddesc_iso[i].offset;
				fhep->desc_addr[i].status.b_iso_in.txbytes =
				    ddesc_iso[i].length;
				fhep->desc_addr[i].status.b_iso_in.framenum =
				    fhep->xiso_frame_num;
				fhep->desc_addr[i].status.b_iso_in.bs =
				    BS_HOST_READY;
				fhep->desc_addr[i].status.b_iso_in.txsts = 0;
				fhep->desc_addr[i].status.b_iso_in.sp =
				    (ddesc_iso[i].length %
				     fhep->maxpacket) ? 1 : 0;
				fhep->desc_addr[i].status.b_iso_in.ioc = 0;
				fhep->desc_addr[i].status.b_iso_in.pid = nat + 1;
				fhep->desc_addr[i].status.b_iso_in.l = 0;

				/* Process the last descriptor */
				if (i == ereq->pio_pkt_count - 1) {
					fhep->desc_addr[i].status.b_iso_in.ioc = 1;
					fhep->desc_addr[i].status.b_iso_in.l = 1;
				}
			}

			/* Setup and start the transfer for this endpoint */
			fhep->xiso_active_xfers++;
			FH_WRITE_REG32(&GET_CORE_IF(pcd)->dev_if->
					in_ep_regs[fhep->num]->diepdma,
					fhep->dma_desc_addr);
			diepctl.d32 = 0;
			diepctl.b.epena = 1;
			diepctl.b.cnak = 1;
			FH_MODIFY_REG32(&GET_CORE_IF(pcd)->dev_if->
					 in_ep_regs[fhep->num]->diepctl, 0,
					 diepctl.d32);
		} else {
			/* Setup DMA Descriptor chain for OUT Isoc request */
			for (i = 0; i < ereq->pio_pkt_count; i++) {
				//if ((i % (nat + 1)) == 0)
				fhep->xiso_frame_num = (fhep->xiso_bInterval + 
										fhep->xiso_frame_num) & 0x3FFF;
				fhep->desc_addr[i].buf =
				    req->dma + ddesc_iso[i].offset;
				fhep->desc_addr[i].status.b_iso_out.rxbytes =
				    ddesc_iso[i].length;
				fhep->desc_addr[i].status.b_iso_out.framenum =
				    fhep->xiso_frame_num;
				fhep->desc_addr[i].status.b_iso_out.bs =
				    BS_HOST_READY;
				fhep->desc_addr[i].status.b_iso_out.rxsts = 0;
				fhep->desc_addr[i].status.b_iso_out.sp =
				    (ddesc_iso[i].length %
				     fhep->maxpacket) ? 1 : 0;
				fhep->desc_addr[i].status.b_iso_out.ioc = 0;
				fhep->desc_addr[i].status.b_iso_out.pid = nat + 1;
				fhep->desc_addr[i].status.b_iso_out.l = 0;
				
				/* Process the last descriptor */
				if (i == ereq->pio_pkt_count - 1) {
					fhep->desc_addr[i].status.b_iso_out.ioc = 1;
					fhep->desc_addr[i].status.b_iso_out.l = 1;
				}			
			}
			
			/* Setup and start the transfer for this endpoint */
			fhep->xiso_active_xfers++;
			FH_WRITE_REG32(&GET_CORE_IF(pcd)->
					dev_if->out_ep_regs[fhep->num]->
					doepdma, fhep->dma_desc_addr);
			diepctl.d32 = 0;
			diepctl.b.epena = 1;
			diepctl.b.cnak = 1;
			FH_MODIFY_REG32(&GET_CORE_IF(pcd)->
					 dev_if->out_ep_regs[fhep->num]->
					 doepctl, 0, diepctl.d32);
		}

	} else {
		ep->stopped = 1;
	}

	return 0;
}

/**
 *	- Remove the request from the queue
 */
void complete_xiso_ep(fh_otg_pcd_ep_t * ep)
{
	fh_otg_pcd_request_t *req = NULL;
	struct fh_iso_xreq_port *ereq = NULL;
	struct fh_iso_pkt_desc_port *ddesc_iso = NULL;
	fh_ep_t *fhep = NULL;
	int i;

	//FH_DEBUG();
	fhep = &ep->fh_ep;

	/* Get the first pending request from the queue */
	if (!FH_CIRCLEQ_EMPTY(&ep->queue)) {
		req = FH_CIRCLEQ_FIRST(&ep->queue);
		if (!req) {
			FH_PRINTF("complete_ep 0x%p, req = NULL!\n", ep);
			return;
		}
		fhep->xiso_active_xfers--;
		fhep->xiso_queued_xfers--;
		/* Remove this request from the queue */
		FH_CIRCLEQ_REMOVE_INIT(&ep->queue, req, queue_entry);
	} else {
		FH_PRINTF("complete_ep 0x%p, ep->queue empty!\n", ep);
		return;
	}

	ep->stopped = 1;
	ereq = &req->ext_req;
	ddesc_iso = ereq->per_io_frame_descs;

	if (fhep->xiso_active_xfers < 0) {
		FH_WARN("EP#%d (xiso_active_xfers=%d)", fhep->num,
			 fhep->xiso_active_xfers);
	}

	/* Fill the Isoc descs of portable extended req from dma descriptors */
	for (i = 0; i < ereq->pio_pkt_count; i++) {
		if (fhep->is_in) {	/* IN endpoints */
			ddesc_iso[i].actual_length = ddesc_iso[i].length -
			    fhep->desc_addr[i].status.b_iso_in.txbytes;
			ddesc_iso[i].status =
			    fhep->desc_addr[i].status.b_iso_in.txsts;
		} else {	/* OUT endpoints */
			ddesc_iso[i].actual_length = ddesc_iso[i].length -
			    fhep->desc_addr[i].status.b_iso_out.rxbytes;
			ddesc_iso[i].status =
			    fhep->desc_addr[i].status.b_iso_out.rxsts;
		}
	}

	FH_SPINUNLOCK(ep->pcd->lock);

	/* Call the completion function in the non-portable logic */
	ep->pcd->fops->xisoc_complete(ep->pcd, ep->priv, req->priv, 0,
				      &req->ext_req);

	FH_SPINLOCK(ep->pcd->lock);

	/* Free the request - specific freeing needed for extended request object */
	fh_pcd_xiso_ereq_free(ep, req);

	/* Start the next request */
	fh_otg_pcd_xiso_start_next_request(ep->pcd, ep);

	return;
}

/**
 * Create and initialize the Isoc pkt descriptors of the extended request.
 *
 */
static int fh_otg_pcd_xiso_create_pkt_descs(fh_otg_pcd_request_t * req,
					     void *ereq_nonport,
					     int atomic_alloc)
{
	struct fh_iso_xreq_port *ereq = NULL;
	struct fh_iso_xreq_port *req_mapped = NULL;
	struct fh_iso_pkt_desc_port *ipds = NULL;	/* To be created in this function */
	uint32_t pkt_count;
	int i;

	ereq = &req->ext_req;
	req_mapped = (struct fh_iso_xreq_port *)ereq_nonport;
	pkt_count = req_mapped->pio_pkt_count;

	/* Create the isoc descs */
	if (atomic_alloc) {
		ipds = FH_ALLOC_ATOMIC(sizeof(*ipds) * pkt_count);
	} else {
		ipds = FH_ALLOC(sizeof(*ipds) * pkt_count);
	}

	if (!ipds) {
		FH_ERROR("Failed to allocate isoc descriptors");
		return -FH_E_NO_MEMORY;
	}

	/* Initialize the extended request fields */
	ereq->per_io_frame_descs = ipds;
	ereq->error_count = 0;
	ereq->pio_alloc_pkt_count = pkt_count;
	ereq->pio_pkt_count = pkt_count;
	ereq->tr_sub_flags = req_mapped->tr_sub_flags;

	/* Init the Isoc descriptors */
	for (i = 0; i < pkt_count; i++) {
		ipds[i].length = req_mapped->per_io_frame_descs[i].length;
		ipds[i].offset = req_mapped->per_io_frame_descs[i].offset;
		ipds[i].status = req_mapped->per_io_frame_descs[i].status;	/* 0 */
		ipds[i].actual_length =
		    req_mapped->per_io_frame_descs[i].actual_length;
	}

	return 0;
}

static void prn_ext_request(struct fh_iso_xreq_port *ereq)
{
	struct fh_iso_pkt_desc_port *xfd = NULL;
	int i;

	FH_DEBUG("per_io_frame_descs=%p", ereq->per_io_frame_descs);
	FH_DEBUG("tr_sub_flags=%d", ereq->tr_sub_flags);
	FH_DEBUG("error_count=%d", ereq->error_count);
	FH_DEBUG("pio_alloc_pkt_count=%d", ereq->pio_alloc_pkt_count);
	FH_DEBUG("pio_pkt_count=%d", ereq->pio_pkt_count);
	FH_DEBUG("res=%d", ereq->res);

	for (i = 0; i < ereq->pio_pkt_count; i++) {
		xfd = &ereq->per_io_frame_descs[0];
		FH_DEBUG("FD #%d", i);

		FH_DEBUG("xfd->actual_length=%d", xfd->actual_length);
		FH_DEBUG("xfd->length=%d", xfd->length);
		FH_DEBUG("xfd->offset=%d", xfd->offset);
		FH_DEBUG("xfd->status=%d", xfd->status);
	}
}

/**
 *
 */
int fh_otg_pcd_xiso_ep_queue(fh_otg_pcd_t * pcd, void *ep_handle,
			      uint8_t * buf, fh_dma_t dma_buf, uint32_t buflen,
			      int zero, void *req_handle, int atomic_alloc,
			      void *ereq_nonport)
{
	fh_otg_pcd_request_t *req = NULL;
	fh_otg_pcd_ep_t *ep;
	fh_irqflags_t flags;
	int res;

	ep = get_ep_from_handle(pcd, ep_handle);
	if (!ep) {
		FH_WARN("bad ep\n");
		return -FH_E_INVALID;
	}

	/* We support this extension only for DDMA mode */
	if (ep->fh_ep.type == FH_OTG_EP_TYPE_ISOC)
		if (!GET_CORE_IF(pcd)->dma_desc_enable)
			return -FH_E_INVALID;

	/* Create a fh_otg_pcd_request_t object */
	if (atomic_alloc) {
		req = FH_ALLOC_ATOMIC(sizeof(*req));
	} else {
		req = FH_ALLOC(sizeof(*req));
	}

	if (!req) {
		return -FH_E_NO_MEMORY;
	}

	/* Create the Isoc descs for this request which shall be the exact match
	 * of the structure sent to us from the non-portable logic */
	res =
	    fh_otg_pcd_xiso_create_pkt_descs(req, ereq_nonport, atomic_alloc);
	if (res) {
		FH_WARN("Failed to init the Isoc descriptors");
		FH_FREE(req);
		return res;
	}

	FH_SPINLOCK_IRQSAVE(pcd->lock, &flags);

	FH_CIRCLEQ_INIT_ENTRY(req, queue_entry);
	req->buf = buf;
	req->dma = dma_buf;
	req->length = buflen;
	req->sent_zlp = zero;
	req->priv = req_handle;

	//FH_SPINLOCK_IRQSAVE(pcd->lock, &flags);
	ep->fh_ep.dma_addr = dma_buf;
	ep->fh_ep.start_xfer_buff = buf;
	ep->fh_ep.xfer_buff = buf;
	ep->fh_ep.xfer_len = 0;
	ep->fh_ep.xfer_count = 0;
	ep->fh_ep.sent_zlp = 0;
	ep->fh_ep.total_len = buflen;

	/* Add this request to the tail */
	FH_CIRCLEQ_INSERT_TAIL(&ep->queue, req, queue_entry);
	ep->fh_ep.xiso_queued_xfers++;

//FH_DEBUG("CP_0");
//FH_DEBUG("req->ext_req.tr_sub_flags=%d", req->ext_req.tr_sub_flags);
//prn_ext_request((struct fh_iso_xreq_port *) ereq_nonport);
//prn_ext_request(&req->ext_req);

	//FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);

	/* If the req->status == ASAP  then check if there is any active transfer
	 * for this endpoint. If no active transfers, then get the first entry
	 * from the queue and start that transfer
	 */
	if (req->ext_req.tr_sub_flags == FH_EREQ_TF_ASAP) {
		res = fh_otg_pcd_xiso_start_next_request(pcd, ep);
		if (res) {
			FH_WARN("Failed to start the next Isoc transfer");
			FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
			FH_FREE(req);
			return res;
		}
	}

	FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
	return 0;
}

#endif
/* END ifdef FH_UTE_PER_IO ***************************************************/
int fh_otg_pcd_ep_queue(fh_otg_pcd_t * pcd, void *ep_handle,
			 uint8_t * buf, fh_dma_t dma_buf, uint32_t buflen,
			 int zero, void *req_handle, int atomic_alloc)
{
	fh_irqflags_t flags;
	fh_otg_pcd_request_t *req;
	fh_otg_pcd_ep_t *ep;
	uint32_t max_transfer;

	ep = get_ep_from_handle(pcd, ep_handle);
	if (!ep || (!ep->desc && ep->fh_ep.num != 0)) {
		FH_WARN("bad ep\n");
		return -FH_E_INVALID;
	}

	if (atomic_alloc) {
		req = FH_ALLOC_ATOMIC(sizeof(*req));
	} else {
		req = FH_ALLOC(sizeof(*req));
	}

	if (!req) {
		return -FH_E_NO_MEMORY;
	}
	FH_CIRCLEQ_INIT_ENTRY(req, queue_entry);
	if (!GET_CORE_IF(pcd)->core_params->opt) {
		if (ep->fh_ep.num != 0) {
			FH_ERROR("queue req %p, len %d buf %p\n",
				  req_handle, buflen, buf);
		}
	}

	req->buf = buf;
	req->dma = dma_buf;
	req->length = buflen;
	req->sent_zlp = zero;
	req->priv = req_handle;
	req->dw_align_buf = NULL;
	if ((dma_buf & 0x3) && GET_CORE_IF(pcd)->dma_enable
	    && !GET_CORE_IF(pcd)->dma_desc_enable)
		req->dw_align_buf = FH_DMA_ALLOC(buflen,
						  &req->dw_align_buf_dma);
	FH_SPINLOCK_IRQSAVE(pcd->lock, &flags);

	/*
	 * After adding request to the queue for IN ISOC wait for In Token Received
	 * when TX FIFO is empty interrupt and for OUT ISOC wait for OUT Token 
	 * Received when EP is disabled interrupt to obtain starting microframe
	 * (odd/even) start transfer
	 */
	if (ep->fh_ep.type == FH_OTG_EP_TYPE_ISOC) {
		if (req != 0) {
			depctl_data_t depctl = {.d32 =
				    FH_READ_REG32(&pcd->core_if->dev_if->
						   in_ep_regs[ep->fh_ep.num]->
						   diepctl) };
			++pcd->request_pending;

			FH_CIRCLEQ_INSERT_TAIL(&ep->queue, req, queue_entry);
			if (ep->fh_ep.is_in) {
				depctl.b.cnak = 1;
				FH_WRITE_REG32(&pcd->core_if->dev_if->
						in_ep_regs[ep->fh_ep.num]->
						diepctl, depctl.d32);
			}
			if (GET_CORE_IF(pcd)->dma_desc_enable) {
				if (ep->fh_ep.iso_transfer_started) {
					/* 
					 * Add next request to the descriptor chain 
					 * currently not in use by HW 
					 */
					program_next_iso_request_ddma(ep, req);
				} else if (!ep->fh_ep.is_in)
					/* For OUT start first request immediately after queue */
					fh_otg_pcd_start_iso_ddma(GET_CORE_IF(pcd), ep);
			}
			
			FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
		}
		return 0;
	}

	/*
	 * For EP0 IN without premature status, zlp is required?
	 */
	if (ep->fh_ep.num == 0 && ep->fh_ep.is_in) {
		FH_DEBUGPL(DBG_PCDV, "%d-OUT ZLP\n", ep->fh_ep.num);
		//_req->zero = 1;
	}

	/* Start the transfer */
	if (FH_CIRCLEQ_EMPTY(&ep->queue) && !ep->stopped) {
		/* EP0 Transfer? */
		if (ep->fh_ep.num == 0) {
			switch (pcd->ep0state) {
			case EP0_IN_DATA_PHASE:
				FH_DEBUGPL(DBG_PCD,
					    "%s ep0: EP0_IN_DATA_PHASE\n",
					    __func__);
				break;

			case EP0_OUT_DATA_PHASE:
				FH_DEBUGPL(DBG_PCD,
					    "%s ep0: EP0_OUT_DATA_PHASE\n",
					    __func__);
				if (pcd->request_config) {
					/* Complete STATUS PHASE */
					ep->fh_ep.is_in = 1;
					pcd->ep0state = EP0_IN_STATUS_PHASE;
				}
				break;

			case EP0_IN_STATUS_PHASE:
				FH_DEBUGPL(DBG_PCD,
					    "%s ep0: EP0_IN_STATUS_PHASE\n",
					    __func__);
				break;

			default:
				FH_DEBUGPL(DBG_ANY, "ep0: odd state %d\n",
					    pcd->ep0state);
				FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
				return -FH_E_SHUTDOWN;
			}

			ep->fh_ep.dma_addr = dma_buf;
			ep->fh_ep.start_xfer_buff = buf;
			ep->fh_ep.xfer_buff = buf;
			ep->fh_ep.xfer_len = buflen;
			ep->fh_ep.xfer_count = 0;
			ep->fh_ep.sent_zlp = 0;
			ep->fh_ep.total_len = ep->fh_ep.xfer_len;

			if (zero) {
				if ((ep->fh_ep.xfer_len %
				     ep->fh_ep.maxpacket == 0)
				    && (ep->fh_ep.xfer_len != 0)) {
					ep->fh_ep.sent_zlp = 1;
				}

			}

			fh_otg_ep0_start_transfer(GET_CORE_IF(pcd),
						   &ep->fh_ep);
		}		// non-ep0 endpoints
		else {
#ifdef FH_UTE_CFI
			if (ep->fh_ep.buff_mode != BM_STANDARD) {
				/* store the request length */
				ep->fh_ep.cfi_req_len = buflen;
				pcd->cfi->ops.build_descriptors(pcd->cfi, pcd,
								ep, req);
			} else {
#endif
				max_transfer =
				    GET_CORE_IF(ep->pcd)->core_params->
				    max_transfer_size;

				/* Setup and start the Transfer */
				if (req->dw_align_buf) {
					if (ep->fh_ep.is_in)
						fh_memcpy(req->dw_align_buf,
							   buf, buflen);
					ep->fh_ep.dma_addr =
					    req->dw_align_buf_dma;
					ep->fh_ep.start_xfer_buff =
					    req->dw_align_buf;
					ep->fh_ep.xfer_buff =
					    req->dw_align_buf;
				} else {
					ep->fh_ep.dma_addr = dma_buf;
					ep->fh_ep.start_xfer_buff = buf;
					ep->fh_ep.xfer_buff = buf;
				}
				ep->fh_ep.xfer_len = 0;
				ep->fh_ep.xfer_count = 0;
				ep->fh_ep.sent_zlp = 0;
				ep->fh_ep.total_len = buflen;

				ep->fh_ep.maxxfer = max_transfer;
				if (GET_CORE_IF(pcd)->dma_desc_enable) {
					uint32_t out_max_xfer =
					    DDMA_MAX_TRANSFER_SIZE -
					    (DDMA_MAX_TRANSFER_SIZE % 4);
					if (ep->fh_ep.is_in) {
						if (ep->fh_ep.maxxfer >
						    DDMA_MAX_TRANSFER_SIZE) {
							ep->fh_ep.maxxfer =
							    DDMA_MAX_TRANSFER_SIZE;
						}
					} else {
						if (ep->fh_ep.maxxfer >
						    out_max_xfer) {
							ep->fh_ep.maxxfer =
							    out_max_xfer;
						}
					}
				}
				if (ep->fh_ep.maxxfer < ep->fh_ep.total_len) {
					ep->fh_ep.maxxfer -=
					    (ep->fh_ep.maxxfer %
					     ep->fh_ep.maxpacket);
				}

				if (zero) {
					if ((ep->fh_ep.total_len %
					     ep->fh_ep.maxpacket == 0)
					    && (ep->fh_ep.total_len != 0)) {
						ep->fh_ep.sent_zlp = 1;
					}
				}
#ifdef FH_UTE_CFI
			}
#endif
			fh_otg_ep_start_transfer(GET_CORE_IF(pcd),
						  &ep->fh_ep);
		}
	}

	if (req != 0) {
		++pcd->request_pending;
		FH_CIRCLEQ_INSERT_TAIL(&ep->queue, req, queue_entry);
		if (ep->fh_ep.is_in && ep->stopped
		    && !(GET_CORE_IF(pcd)->dma_enable)) {
			/** @todo NGS Create a function for this. */
			diepmsk_data_t diepmsk = {.d32 = 0 };
			diepmsk.b.intktxfemp = 1;
			if (GET_CORE_IF(pcd)->multiproc_int_enable) {
				FH_MODIFY_REG32(&GET_CORE_IF(pcd)->
						 dev_if->dev_global_regs->diepeachintmsk
						 [ep->fh_ep.num], 0,
						 diepmsk.d32);
			} else {
				FH_MODIFY_REG32(&GET_CORE_IF(pcd)->
						 dev_if->dev_global_regs->
						 diepmsk, 0, diepmsk.d32);
			}

		}
	}
	FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);

	return 0;
}

int fh_otg_pcd_ep_dequeue(fh_otg_pcd_t * pcd, void *ep_handle,
			   void *req_handle)
{
	fh_irqflags_t flags;
	fh_otg_pcd_request_t *req;
	fh_otg_pcd_ep_t *ep;

	ep = get_ep_from_handle(pcd, ep_handle);
	if (!ep || (!ep->desc && ep->fh_ep.num != 0)) {
		FH_WARN("bad argument\n");
		return -FH_E_INVALID;
	}

	FH_SPINLOCK_IRQSAVE(pcd->lock, &flags);

	/* make sure it's actually queued on this endpoint */
	FH_CIRCLEQ_FOREACH(req, &ep->queue, queue_entry) {
		if (req->priv == (void *)req_handle) {
			break;
		}
	}

	if (req->priv != (void *)req_handle) {
		FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
		return -FH_E_INVALID;
	}

	if (!FH_CIRCLEQ_EMPTY_ENTRY(req, queue_entry)) {
		fh_otg_request_done(ep, req, -FH_E_RESTART);
	} else {
		req = NULL;
	}

	FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);

	return req ? 0 : -FH_E_SHUTDOWN;

}

int fh_otg_pcd_ep_halt(fh_otg_pcd_t * pcd, void *ep_handle, int value)
{
	fh_otg_pcd_ep_t *ep;
	fh_irqflags_t flags;
	int retval = 0;

	ep = get_ep_from_handle(pcd, ep_handle);

	if (!ep || (!ep->desc && ep != &pcd->ep0) ||
	    (ep->desc && (ep->desc->bmAttributes == UE_ISOCHRONOUS))) {
		FH_WARN("%s, bad ep\n", __func__);
		return -FH_E_INVALID;
	}

	FH_SPINLOCK_IRQSAVE(pcd->lock, &flags);
	if (!FH_CIRCLEQ_EMPTY(&ep->queue)) {
		FH_WARN("%d %s XFer In process\n", ep->fh_ep.num,
			 ep->fh_ep.is_in ? "IN" : "OUT");
		retval = -FH_E_AGAIN;
	} else if (value == 0) {
	    ep->fh_ep.stall_clear_flag = 0;
		fh_otg_ep_clear_stall(GET_CORE_IF(pcd), &ep->fh_ep);
	} else if (value == 1) {
	stall:
		if (ep->fh_ep.is_in == 1 && GET_CORE_IF(pcd)->dma_desc_enable) {
			dtxfsts_data_t txstatus;
			fifosize_data_t txfifosize;

			txfifosize.d32 =
			    FH_READ_REG32(&GET_CORE_IF(pcd)->
					   core_global_regs->dtxfsiz[ep->fh_ep.
								     tx_fifo_num]);
			txstatus.d32 =
			    FH_READ_REG32(&GET_CORE_IF(pcd)->
					   dev_if->in_ep_regs[ep->fh_ep.num]->
					   dtxfsts);

			if (txstatus.b.txfspcavail < txfifosize.b.depth) {
				FH_WARN("%s() Data In Tx Fifo\n", __func__);
				retval = -FH_E_AGAIN;
			} else {
				if (ep->fh_ep.num == 0) {
					pcd->ep0state = EP0_STALL;
				}

				ep->stopped = 1;
				fh_otg_ep_set_stall(GET_CORE_IF(pcd),
						     &ep->fh_ep);
			}
		} else {
			if (ep->fh_ep.num == 0) {
				pcd->ep0state = EP0_STALL;
			}

			ep->stopped = 1;
			fh_otg_ep_set_stall(GET_CORE_IF(pcd), &ep->fh_ep);
		}
	} else if (value == 2) {
		ep->fh_ep.stall_clear_flag = 0;
	} else if (value == 3) {
		ep->fh_ep.stall_clear_flag = 1;
		goto stall;
	}

	FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);

	return retval;
}

/**
 * This function initiates remote wakeup of the host from suspend state.
 */
void fh_otg_pcd_rem_wkup_from_suspend(fh_otg_pcd_t * pcd, int set)
{
	dctl_data_t dctl = { 0 };
	fh_otg_core_if_t *core_if = GET_CORE_IF(pcd);
	dsts_data_t dsts;

	dsts.d32 = FH_READ_REG32(&core_if->dev_if->dev_global_regs->dsts);
	if (!dsts.b.suspsts) {
		FH_WARN("Remote wakeup while is not in suspend state\n");
	}
	/* Check if DEVICE_REMOTE_WAKEUP feature enabled */
	if (pcd->remote_wakeup_enable) {
		if (set) {

			if (core_if->adp_enable) {
				gpwrdn_data_t gpwrdn;

				fh_otg_adp_probe_stop(core_if);

				/* Mask SRP detected interrupt from Power Down Logic */
				gpwrdn.d32 = 0;
				gpwrdn.b.srp_det_msk = 1;
				FH_MODIFY_REG32(&core_if->
						 core_global_regs->gpwrdn,
						 gpwrdn.d32, 0);

				/* Disable Power Down Logic */
				gpwrdn.d32 = 0;
				gpwrdn.b.pmuactv = 1;
				FH_MODIFY_REG32(&core_if->
						 core_global_regs->gpwrdn,
						 gpwrdn.d32, 0);

				/*
				 * Initialize the Core for Device mode.
				 */
				core_if->op_state = B_PERIPHERAL;
				fh_otg_core_init(core_if);
				fh_otg_enable_global_interrupts(core_if);
				cil_pcd_start(core_if);

				fh_otg_initiate_srp(core_if);
			}

			dctl.b.rmtwkupsig = 1;
			FH_MODIFY_REG32(&core_if->dev_if->dev_global_regs->
					 dctl, 0, dctl.d32);
			FH_DEBUGPL(DBG_PCD, "Set Remote Wakeup\n");

			fh_mdelay(2);
			FH_MODIFY_REG32(&core_if->dev_if->dev_global_regs->
					 dctl, dctl.d32, 0);
			FH_DEBUGPL(DBG_PCD, "Clear Remote Wakeup\n");
		}
	} else {
		FH_DEBUGPL(DBG_PCD, "Remote Wakeup is disabled\n");
	}
}

#ifdef CONFIG_USB_FH_OTG_LPM
/**
 * This function initiates remote wakeup of the host from L1 sleep state.
 */
void fh_otg_pcd_rem_wkup_from_sleep(fh_otg_pcd_t * pcd, int set)
{
	glpmcfg_data_t lpmcfg;
	pcgcctl_data_t pcgcctl = {.d32 = 0 };
	
	fh_otg_core_if_t *core_if = GET_CORE_IF(pcd);

	lpmcfg.d32 = FH_READ_REG32(&core_if->core_global_regs->glpmcfg);

	/* Check if we are in L1 state */
	if (!lpmcfg.b.prt_sleep_sts) {
		FH_DEBUGPL(DBG_PCD, "Device is not in sleep state\n");
		return;
	}

	/* Check if host allows remote wakeup */
	if (!lpmcfg.b.rem_wkup_en) {
		FH_DEBUGPL(DBG_PCD, "Host does not allow remote wakeup\n");
		return;
	}

	/* Check if Resume OK */
	if (!lpmcfg.b.sleep_state_resumeok) {
		FH_DEBUGPL(DBG_PCD, "Sleep state resume is not OK\n");
		return;
	}

	lpmcfg.d32 = FH_READ_REG32(&core_if->core_global_regs->glpmcfg);
	lpmcfg.b.en_utmi_sleep = 0;
	lpmcfg.b.hird_thres &= (~(1 << 4));
	
	/* Clear Enbl_L1Gating bit. */
	pcgcctl.b.enbl_sleep_gating = 1;
	FH_MODIFY_REG32(core_if->pcgcctl, pcgcctl.d32,0);
			
	FH_WRITE_REG32(&core_if->core_global_regs->glpmcfg, lpmcfg.d32);

	if (set) {
		dctl_data_t dctl = {.d32 = 0 };
		dctl.b.rmtwkupsig = 1;
		/* Set RmtWkUpSig bit to start remote wakup signaling.
		 * Hardware will automatically clear this bit.
		 */
		FH_MODIFY_REG32(&core_if->dev_if->dev_global_regs->dctl,
				 0, dctl.d32);
		FH_DEBUGPL(DBG_PCD, "Set Remote Wakeup\n");
	}

}
#endif

/**
 * Performs remote wakeup.
 */
void fh_otg_pcd_remote_wakeup(fh_otg_pcd_t * pcd, int set)
{
	fh_otg_core_if_t *core_if = GET_CORE_IF(pcd);
	fh_irqflags_t flags;
	if (fh_otg_is_device_mode(core_if)) {
		FH_SPINLOCK_IRQSAVE(pcd->lock, &flags);
#ifdef CONFIG_USB_FH_OTG_LPM
		if (core_if->lx_state == FH_OTG_L1) {
			fh_otg_pcd_rem_wkup_from_sleep(pcd, set);
		} else {
#endif
			fh_otg_pcd_rem_wkup_from_suspend(pcd, set);
#ifdef CONFIG_USB_FH_OTG_LPM
		}
#endif
		FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
	}
	return;
}

void fh_otg_pcd_disconnect_us(fh_otg_pcd_t * pcd, int no_of_usecs)
{
	fh_otg_core_if_t *core_if = GET_CORE_IF(pcd);
	dctl_data_t dctl = { 0 };

	if (fh_otg_is_device_mode(core_if)) {
		dctl.b.sftdiscon = 1;
		FH_PRINTF("Soft disconnect for %d useconds\n",no_of_usecs);
		FH_MODIFY_REG32(&core_if->dev_if->dev_global_regs->dctl, 0, dctl.d32);
		fh_udelay(no_of_usecs);
		FH_MODIFY_REG32(&core_if->dev_if->dev_global_regs->dctl, dctl.d32,0);
		
	} else{
		FH_PRINTF("NOT SUPPORTED IN HOST MODE\n");
	}
	return;

}

int fh_otg_pcd_wakeup(fh_otg_pcd_t * pcd)
{
	dsts_data_t dsts;
	gotgctl_data_t gotgctl;

	/*
	 * This function starts the Protocol if no session is in progress. If
	 * a session is already in progress, but the device is suspended,
	 * remote wakeup signaling is started.
	 */

	/* Check if valid session */
	gotgctl.d32 =
	    FH_READ_REG32(&(GET_CORE_IF(pcd)->core_global_regs->gotgctl));
	if (gotgctl.b.bsesvld) {
		/* Check if suspend state */
		dsts.d32 =
		    FH_READ_REG32(&
				   (GET_CORE_IF(pcd)->dev_if->
				    dev_global_regs->dsts));
		if (dsts.b.suspsts) {
			fh_otg_pcd_remote_wakeup(pcd, 1);
		}
	} else {
		fh_otg_pcd_initiate_srp(pcd);
	}

	return 0;

}

/**
 * Start the SRP timer to detect when the SRP does not complete within
 * 6 seconds.
 *
 * @param pcd the pcd structure.
 */
void fh_otg_pcd_initiate_srp(fh_otg_pcd_t * pcd)
{
	fh_irqflags_t flags;
	FH_SPINLOCK_IRQSAVE(pcd->lock, &flags);
	fh_otg_initiate_srp(GET_CORE_IF(pcd));
	FH_SPINUNLOCK_IRQRESTORE(pcd->lock, flags);
}

int fh_otg_pcd_get_frame_number(fh_otg_pcd_t * pcd)
{
	return fh_otg_get_frame_number(GET_CORE_IF(pcd));
}

int fh_otg_pcd_is_lpm_enabled(fh_otg_pcd_t * pcd)
{
	return GET_CORE_IF(pcd)->core_params->lpm_enable;
}

int fh_otg_pcd_is_besl_enabled(fh_otg_pcd_t * pcd)
{
	return GET_CORE_IF(pcd)->core_params->besl_enable;
}

int fh_otg_pcd_get_param_baseline_besl(fh_otg_pcd_t * pcd)
{
	return GET_CORE_IF(pcd)->core_params->baseline_besl;
}

int fh_otg_pcd_get_param_deep_besl(fh_otg_pcd_t * pcd)
{
	return GET_CORE_IF(pcd)->core_params->deep_besl;
}

uint32_t get_b_hnp_enable(fh_otg_pcd_t * pcd)
{
	return pcd->b_hnp_enable;
}

uint32_t get_a_hnp_support(fh_otg_pcd_t * pcd)
{
	return pcd->a_hnp_support;
}

uint32_t get_a_alt_hnp_support(fh_otg_pcd_t * pcd)
{
	return pcd->a_alt_hnp_support;
}

int fh_otg_pcd_get_rmwkup_enable(fh_otg_pcd_t * pcd)
{
	return pcd->remote_wakeup_enable;
}

#endif /* FH_HOST_ONLY */
