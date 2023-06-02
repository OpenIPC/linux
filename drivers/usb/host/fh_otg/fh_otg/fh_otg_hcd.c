/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/fh_otg_hcd.c $
 * $Revision: #110 $
 * $Date: 2013/05/19 $
 * $Change: 2234022 $
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
#ifndef FH_DEVICE_ONLY

/** @file
 * This file implements HCD Core. All code in this file is portable and doesn't
 * use any OS specific functions.
 * Interface provided by HCD Core is defined in <code><hcd_if.h></code>
 * header file.
 */

#include "fh_otg_hcd.h"
#include "fh_otg_regs.h"

fh_otg_hcd_t *fh_otg_hcd_alloc_hcd(void)
{
	return FH_ALLOC(sizeof(fh_otg_hcd_t));
}

/**
 * Connection timeout function.  An OTG host is required to display a
 * message if the device does not connect within 10 seconds.
 */
void fh_otg_hcd_connect_timeout(void *ptr)
{
	fh_otg_hcd_t *hcd;
	gpwrdn_data_t gpwrdn;
	FH_DEBUGPL(DBG_HCDV, "%s(%p)\n", __func__, ptr);
	FH_PRINTF("Connect Timeout\n");
	__FH_ERROR("Device Not Connected/Responding\n");
	/** Remove buspower after 10s */
	hcd = ptr;
	if (hcd->core_if->otg_ver)
		fh_otg_set_prtpower(hcd->core_if, 0);
	if (hcd->core_if->adp_enable && !hcd->core_if->adp.probe_enabled) {
		cil_hcd_disconnect(hcd->core_if);
		gpwrdn.d32 = 0;
		/* Enable Power Down Logic */
		gpwrdn.b.pmuintsel = 1;
		gpwrdn.b.pmuactv = 1;
		gpwrdn.b.dis_vbus = 1;
		FH_MODIFY_REG32(&hcd->core_if->core_global_regs->gpwrdn, 0, gpwrdn.d32);
		
		/* Unmask SRP detected interrupt from Power Down Logic */
		gpwrdn.d32 = 0;
		gpwrdn.b.srp_det_msk = 1;
		FH_MODIFY_REG32(&hcd->core_if->core_global_regs->gpwrdn, 0, gpwrdn.d32);

		fh_mdelay(220);
		fh_otg_adp_probe_start(hcd->core_if);
	}
}

#ifdef DEBUG
static void dump_channel_info(fh_otg_hcd_t * hcd, fh_otg_qh_t * qh)
{
	if (qh->channel != NULL) {
		fh_hc_t *hc = qh->channel;
		fh_list_link_t *item;
		fh_otg_qh_t *qh_item;
		int num_channels = hcd->core_if->core_params->host_channels;
		int i;

		fh_otg_hc_regs_t *hc_regs;
		hcchar_data_t hcchar;
		hcsplt_data_t hcsplt;
		hctsiz_data_t hctsiz;
		uint32_t hcdma;

		hc_regs = hcd->core_if->host_if->hc_regs[hc->hc_num];
		hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);
		hcsplt.d32 = FH_READ_REG32(&hc_regs->hcsplt);
		hctsiz.d32 = FH_READ_REG32(&hc_regs->hctsiz);
		hcdma = FH_READ_REG32(&hc_regs->hcdma);

		FH_PRINTF("  Assigned to channel %p:\n", hc);
		FH_PRINTF("    hcchar 0x%08x, hcsplt 0x%08x\n", hcchar.d32,
			   hcsplt.d32);
		FH_PRINTF("    hctsiz 0x%08x, hcdma 0x%08x\n", hctsiz.d32,
			   hcdma);
		FH_PRINTF("    dev_addr: %d, ep_num: %d, ep_is_in: %d\n",
			   hc->dev_addr, hc->ep_num, hc->ep_is_in);
		FH_PRINTF("    ep_type: %d\n", hc->ep_type);
		FH_PRINTF("    max_packet: %d\n", hc->max_packet);
		FH_PRINTF("    data_pid_start: %d\n", hc->data_pid_start);
		FH_PRINTF("    xfer_started: %d\n", hc->xfer_started);
		FH_PRINTF("    halt_status: %d\n", hc->halt_status);
		FH_PRINTF("    xfer_buff: %p\n", hc->xfer_buff);
		FH_PRINTF("    xfer_len: %d\n", hc->xfer_len);
		FH_PRINTF("    qh: %p\n", hc->qh);
		FH_PRINTF("  NP inactive sched:\n");
		FH_LIST_FOREACH(item, &hcd->non_periodic_sched_inactive) {
			qh_item =
			    FH_LIST_ENTRY(item, fh_otg_qh_t, qh_list_entry);
			FH_PRINTF("    %p\n", qh_item);
		}
		FH_PRINTF("  NP active sched:\n");
		FH_LIST_FOREACH(item, &hcd->non_periodic_sched_active) {
			qh_item =
			    FH_LIST_ENTRY(item, fh_otg_qh_t, qh_list_entry);
			FH_PRINTF("    %p\n", qh_item);
		}
		FH_PRINTF("  Channels: \n");
		for (i = 0; i < num_channels; i++) {
			fh_hc_t *hc = hcd->hc_ptr_array[i];
			FH_PRINTF("    %2d: %p\n", i, hc);
		}
	}
}
#endif /* DEBUG */

/**
 * Work queue function for starting the HCD when A-Cable is connected.
 * The hcd_start() must be called in a process context.
 */
static void hcd_start_func(void *_vp)
{
	fh_otg_hcd_t *hcd = (fh_otg_hcd_t *) _vp;

	FH_DEBUGPL(DBG_HCDV, "%s() %p\n", __func__, hcd);
	if (hcd) {
		hcd->fops->start(hcd);
	}
}

static void del_xfer_timers(fh_otg_hcd_t * hcd)
{
#ifdef DEBUG
	int i;
	int num_channels = hcd->core_if->core_params->host_channels;
	for (i = 0; i < num_channels; i++) {
		FH_TIMER_CANCEL(hcd->core_if->hc_xfer_timer[i]);
	}
#endif
}

static void del_timers(fh_otg_hcd_t * hcd)
{
	del_xfer_timers(hcd);
	FH_TIMER_CANCEL(hcd->conn_timer);
}

/**
 * Processes all the URBs in a single list of QHs. Completes them with
 * -ETIMEDOUT and frees the QTD.
 */
static void kill_urbs_in_qh_list(fh_otg_hcd_t * hcd, fh_list_link_t * qh_list)
{
	fh_list_link_t *qh_item;
	fh_otg_qh_t *qh;
	fh_otg_qtd_t *qtd, *qtd_tmp;

	FH_LIST_FOREACH(qh_item, qh_list) {
		qh = FH_LIST_ENTRY(qh_item, fh_otg_qh_t, qh_list_entry);
		FH_CIRCLEQ_FOREACH_SAFE(qtd, qtd_tmp,
					 &qh->qtd_list, qtd_list_entry) {
			qtd = FH_CIRCLEQ_FIRST(&qh->qtd_list);
			if (qtd->urb != NULL) {
				if(!qtd->urb->priv) {
					FH_ERROR("urb->priv is NULL !!!!\n");
					return;
				}
				if(!hcd->fops)
					FH_ERROR("hcd->fops is NULL !!!!!\n");
				if(!hcd->fops->complete)
					FH_ERROR("fops->complete is NULL !!!!\n");
				hcd->fops->complete(hcd, qtd->urb->priv,
						    qtd->urb, -FH_E_TIMEOUT);
				fh_otg_hcd_qtd_remove_and_free(hcd, qtd, qh);
			}

		}
	}
}

/**
 * Responds with an error status of ETIMEDOUT to all URBs in the non-periodic
 * and periodic schedules. The QTD associated with each URB is removed from
 * the schedule and freed. This function may be called when a disconnect is
 * detected or when the HCD is being stopped.
 */
static void kill_all_urbs(fh_otg_hcd_t * hcd)
{
	kill_urbs_in_qh_list(hcd, &hcd->non_periodic_sched_inactive);
	kill_urbs_in_qh_list(hcd, &hcd->non_periodic_sched_active);
	kill_urbs_in_qh_list(hcd, &hcd->periodic_sched_inactive);
	kill_urbs_in_qh_list(hcd, &hcd->periodic_sched_ready);
	kill_urbs_in_qh_list(hcd, &hcd->periodic_sched_assigned);
	kill_urbs_in_qh_list(hcd, &hcd->periodic_sched_queued);
}

/**
 * Start the connection timer.  An OTG host is required to display a
 * message if the device does not connect within 10 seconds.  The
 * timer is deleted if a port connect interrupt occurs before the
 * timer expires.
 */
static void fh_otg_hcd_start_connect_timer(fh_otg_hcd_t * hcd)
{
	FH_TIMER_SCHEDULE(hcd->conn_timer, 10000 /* 10 secs */ );
}

/**
 * HCD Callback function for disconnect of the HCD.
 *
 * @param p void pointer to the <code>struct usb_hcd</code>
 */
static int32_t fh_otg_hcd_session_start_cb(void *p)
{
	fh_otg_hcd_t *fh_otg_hcd;
	FH_DEBUGPL(DBG_HCDV, "%s(%p)\n", __func__, p);
	fh_otg_hcd = p;
	fh_otg_hcd_start_connect_timer(fh_otg_hcd);
	return 1;
}

/**
 * HCD Callback function for starting the HCD when A-Cable is
 * connected.
 *
 * @param p void pointer to the <code>struct usb_hcd</code>
 */
static int32_t fh_otg_hcd_start_cb(void *p)
{
	fh_otg_hcd_t *fh_otg_hcd = p;
	fh_otg_core_if_t *core_if;
	hprt0_data_t hprt0;
	uint32_t timeout = 50;

	core_if = fh_otg_hcd->core_if;
	/**@todo vahrama: Check the timeout value for OTG 2.0 */
	if (core_if->otg_ver)
		timeout = 25;
	if (core_if->op_state == B_HOST) {
		/*
		 * Reset the port.  During a HNP mode switch the reset
		 * needs to occur within 1ms and have a duration of at
		 * least 50ms.
		 */
		hprt0.d32 = fh_otg_read_hprt0(core_if);
		hprt0.b.prtrst = 1;
		FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
		if (core_if->otg_ver) {
			fh_mdelay(60);
			hprt0.d32 = fh_otg_read_hprt0(core_if);
			hprt0.b.prtrst = 0;
			FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
		}
	}
	FH_WORKQ_SCHEDULE_DELAYED(core_if->wq_otg,
				hcd_start_func, fh_otg_hcd, timeout,
				"start hcd");

	return 1;
}

/**
 * HCD Callback function for disconnect of the HCD.
 *
 * @param p void pointer to the <code>struct usb_hcd</code>
 */
static int32_t fh_otg_hcd_disconnect_cb(void *p)
{
	gintsts_data_t intr;
	fh_otg_hcd_t *fh_otg_hcd = p;

	/*
	 * Set status flags for the hub driver.
	 */
	fh_otg_hcd->flags.b.port_connect_status_change = 1;
	fh_otg_hcd->flags.b.port_connect_status = 0;

	/*
	 * Shutdown any transfers in process by clearing the Tx FIFO Empty
	 * interrupt mask and status bits and disabling subsequent host
	 * channel interrupts.
	 */
	intr.d32 = 0;
	intr.b.nptxfempty = 1;
	intr.b.ptxfempty = 1;
	intr.b.hcintr = 1;
	FH_MODIFY_REG32(&fh_otg_hcd->core_if->core_global_regs->gintmsk,
			 intr.d32, 0);
	FH_MODIFY_REG32(&fh_otg_hcd->core_if->core_global_regs->gintsts,
			 intr.d32, 0);

	/*
	 * Turn off the vbus power only if the core has transitioned to device
	 * mode. If still in host mode, need to keep power on to detect a
	 * reconnection.
	 */
	if (fh_otg_is_device_mode(fh_otg_hcd->core_if)) {
		if (fh_otg_hcd->core_if->op_state != A_SUSPEND) {
			hprt0_data_t hprt0 = {.d32 = 0 };
			FH_PRINTF("Disconnect: PortPower off\n");
			hprt0.b.prtpwr = 0;
			FH_WRITE_REG32(fh_otg_hcd->core_if->host_if->hprt0,
					hprt0.d32);
		}
		/** Delete timers if become device */
		del_timers(fh_otg_hcd);
		fh_otg_disable_host_interrupts(fh_otg_hcd->core_if);
	}

	/* Respond with an error status to all URBs in the schedule. */
	kill_all_urbs(fh_otg_hcd);

	if (fh_otg_is_host_mode(fh_otg_hcd->core_if)) {
		/* Clean up any host channels that were in use. */
		int num_channels;
		int i;
		fh_hc_t *channel;
		fh_otg_hc_regs_t *hc_regs;
		hcchar_data_t hcchar;

		if (fh_otg_hcd->core_if->otg_ver == 1)
			del_xfer_timers(fh_otg_hcd);
		else
			del_timers(fh_otg_hcd);

		num_channels = fh_otg_hcd->core_if->core_params->host_channels;

		if (!fh_otg_hcd->core_if->dma_enable) {
			/* Flush out any channel requests in slave mode. */
			for (i = 0; i < num_channels; i++) {
				channel = fh_otg_hcd->hc_ptr_array[i];
				if (FH_CIRCLEQ_EMPTY_ENTRY
				    (channel, hc_list_entry)) {
					hc_regs =
					    fh_otg_hcd->core_if->
					    host_if->hc_regs[i];
					hcchar.d32 =
					    FH_READ_REG32(&hc_regs->hcchar);
					if (hcchar.b.chen) {
						hcchar.b.chen = 0;
						hcchar.b.chdis = 1;
						hcchar.b.epdir = 0;
						FH_WRITE_REG32
						    (&hc_regs->hcchar,
						     hcchar.d32);
					}
				}
			}
		}

		for (i = 0; i < num_channels; i++) {
			channel = fh_otg_hcd->hc_ptr_array[i];
			if (FH_CIRCLEQ_EMPTY_ENTRY(channel, hc_list_entry)) {
				hc_regs =
				    fh_otg_hcd->core_if->host_if->hc_regs[i];
				hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);
				if (hcchar.b.chen) {
					/* Halt the channel. */
					hcchar.b.chdis = 1;
					FH_WRITE_REG32(&hc_regs->hcchar,
							hcchar.d32);
				}

				fh_otg_hc_cleanup(fh_otg_hcd->core_if,
						   channel);
				FH_CIRCLEQ_INSERT_TAIL
				    (&fh_otg_hcd->free_hc_list, channel,
				     hc_list_entry);
				/*
				 * Added for Descriptor DMA to prevent channel double cleanup
				 * in release_channel_ddma(). Which called from ep_disable
				 * when device disconnect.
				 */
				channel->qh = NULL;
			}
		}
	}

	if (fh_otg_hcd->fops->disconnect) {
		fh_otg_hcd->fops->disconnect(fh_otg_hcd);
	}

	return 1;
}

/**
 * HCD Callback function for stopping the HCD.
 *
 * @param p void pointer to the <code>struct usb_hcd</code>
 */
static int32_t fh_otg_hcd_stop_cb(void *p)
{
	fh_otg_hcd_t *fh_otg_hcd = p;

	FH_DEBUGPL(DBG_HCDV, "%s(%p)\n", __func__, p);
	fh_otg_hcd_stop(fh_otg_hcd);
	return 1;
}

#ifdef CONFIG_USB_FH_OTG_LPM
/**
 * HCD Callback function for sleep of HCD.
 *
 * @param p void pointer to the <code>struct usb_hcd</code>
 */
static int fh_otg_hcd_sleep_cb(void *p)
{
	fh_otg_hcd_t *hcd = p;

	fh_otg_hcd_free_hc_from_lpm(hcd);

	return 0;
}
#endif

/**
 * HCD Callback function for Remote Wakeup.
 *
 * @param p void pointer to the <code>struct usb_hcd</code>
 */
static int fh_otg_hcd_rem_wakeup_cb(void *p)
{
	fh_otg_hcd_t *hcd = p;

	if (hcd->core_if->lx_state == FH_OTG_L2) {
		hcd->flags.b.port_suspend_change = 1;
	}
#ifdef CONFIG_USB_FH_OTG_LPM
	else {
		hcd->flags.b.port_l1_change = 1;
	}
#endif
	return 0;
}

/**
 * Halts the FH_otg host mode operations in a clean manner. USB transfers are
 * stopped.
 */
void fh_otg_hcd_stop(fh_otg_hcd_t * hcd)
{
	hprt0_data_t hprt0 = {.d32 = 0 };

	FH_DEBUGPL(DBG_HCD, "FH OTG HCD STOP\n");

	/*
	 * The root hub should be disconnected before this function is called.
	 * The disconnect will clear the QTD lists (via ..._hcd_urb_dequeue)
	 * and the QH lists (via ..._hcd_endpoint_disable).
	 */

	/* Turn off all host-specific interrupts. */
	fh_otg_disable_host_interrupts(hcd->core_if);

	/* Turn off the vbus power */
	FH_PRINTF("PortPower off\n");
	hprt0.b.prtpwr = 0;
	FH_WRITE_REG32(hcd->core_if->host_if->hprt0, hprt0.d32);
	fh_mdelay(1);
}

int fh_otg_hcd_urb_enqueue(fh_otg_hcd_t * hcd,
			    fh_otg_hcd_urb_t * fh_otg_urb, void **ep_handle,
			    int atomic_alloc)
{
	fh_irqflags_t flags;
	int retval = 0;
	fh_otg_qtd_t *qtd;
	gintmsk_data_t intr_mask = {.d32 = 0 };

	if (!hcd->flags.b.port_connect_status) {
		/* No longer connected. */
		FH_ERROR("Not connected\n");
		return -FH_E_NO_DEVICE;
	}

	qtd = fh_otg_hcd_qtd_create(fh_otg_urb, atomic_alloc);
	if (qtd == NULL) {
		FH_ERROR("FH OTG HCD URB Enqueue failed creating QTD\n");
		return -FH_E_NO_MEMORY;
	}

	retval =
	    fh_otg_hcd_qtd_add(qtd, hcd, (fh_otg_qh_t **) ep_handle, atomic_alloc);
	if (retval < 0) {
		FH_ERROR("FH OTG HCD URB Enqueue failed adding QTD. "
			  "Error status %d\n", retval);
		fh_otg_hcd_qtd_free(qtd);
	} else {
		qtd->qh = *ep_handle;
	}
	intr_mask.d32 = FH_READ_REG32(&hcd->core_if->core_global_regs->gintmsk);
	if (!intr_mask.b.sofintr && retval == 0) {
		fh_otg_transaction_type_e tr_type;
		if ((qtd->qh->ep_type == UE_BULK)
		    && !(qtd->urb->flags & URB_GIVEBACK_ASAP)) {
			/* Do not schedule SG transactions until qtd has URB_GIVEBACK_ASAP set */
			return 0;
		}
		FH_SPINLOCK_IRQSAVE(hcd->lock, &flags);
		tr_type = fh_otg_hcd_select_transactions(hcd);
		if (tr_type != FH_OTG_TRANSACTION_NONE) {
			fh_otg_hcd_queue_transactions(hcd, tr_type);
		}
		FH_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);
	}

	return retval;
}

int fh_otg_hcd_urb_dequeue(fh_otg_hcd_t * hcd,
			    fh_otg_hcd_urb_t * fh_otg_urb)
{
	fh_otg_qh_t *qh;
	fh_otg_qtd_t *urb_qtd;

	urb_qtd = fh_otg_urb->qtd;
	qh = urb_qtd->qh;
	if (!urb_qtd) {
		printk(KERN_ERR "## Urb QTD is NULL ##\n");
		return -EINVAL;
	}

	qh = urb_qtd->qh;
	if (!qh) {
		printk(KERN_ERR "## Urb QH is NULL ##\n");
		return -EINVAL;
	}
#ifdef DEBUG
	if (CHK_DEBUG_LEVEL(DBG_HCDV | DBG_HCD_URB)) {
		if (urb_qtd->in_process) {
			dump_channel_info(hcd, qh);
		}
	}
#endif
	if (urb_qtd->in_process && qh->channel) {
		/* The QTD is in process (it has been assigned to a channel). */
		if (hcd->flags.b.port_connect_status) {
			/*
			 * If still connected (i.e. in host mode), halt the
			 * channel so it can be used for other transfers. If
			 * no longer connected, the host registers can't be
			 * written to halt the channel since the core is in
			 * device mode.
			 */
			fh_otg_hc_halt(hcd->core_if, qh->channel,
					FH_OTG_HC_XFER_URB_DEQUEUE);
		}
	}

	/*
	 * Free the QTD and clean up the associated QH. Leave the QH in the
	 * schedule if it has any remaining QTDs.
	 */

	if (!hcd->core_if->dma_desc_enable) {
		uint8_t b = urb_qtd->in_process;
		fh_otg_hcd_qtd_remove_and_free(hcd, urb_qtd, qh);
		if (b) {
			fh_otg_hcd_qh_deactivate(hcd, qh, 0);
			qh->channel = NULL;
		} else if (FH_CIRCLEQ_EMPTY(&qh->qtd_list)) {
			fh_otg_hcd_qh_remove(hcd, qh);
		}
	} else {
		fh_otg_hcd_qtd_remove_and_free(hcd, urb_qtd, qh);
	}
	return 0;
}

int fh_otg_hcd_endpoint_disable(fh_otg_hcd_t * hcd, void *ep_handle,
				 int retry)
{
	fh_otg_qh_t *qh = (fh_otg_qh_t *) ep_handle;
	int retval = 0;
	fh_irqflags_t flags;

	if (retry < 0) {
		retval = -FH_E_INVALID;
		goto done;
	}

	if (!qh) {
		retval = -FH_E_INVALID;
		goto done;
	}

	FH_SPINLOCK_IRQSAVE(hcd->lock, &flags);

	while (!FH_CIRCLEQ_EMPTY(&qh->qtd_list) && retry) {
		FH_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);
		retry--;
		fh_msleep(5);
		FH_SPINLOCK_IRQSAVE(hcd->lock, &flags);
	}

	fh_otg_hcd_qh_remove(hcd, qh);

	FH_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);
	/*
	 * Split fh_otg_hcd_qh_remove_and_free() into qh_remove
	 * and qh_free to prevent stack dump on FH_DMA_FREE() with
	 * irq_disabled (spinlock_irqsave) in fh_otg_hcd_desc_list_free()
	 * and fh_otg_hcd_frame_list_alloc().
	 */
	fh_otg_hcd_qh_free(hcd, qh);

done:
	return retval;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
int fh_otg_hcd_endpoint_reset(fh_otg_hcd_t * hcd, void *ep_handle)
{
	int retval = 0;
	fh_otg_qh_t *qh = (fh_otg_qh_t *) ep_handle;
	if (!qh)
		return -FH_E_INVALID;

	qh->data_toggle = FH_OTG_HC_PID_DATA0;
	return retval;
}
#endif

/**
 * HCD Callback structure for handling mode switching.
 */
static fh_otg_cil_callbacks_t hcd_cil_callbacks = {
	.start = fh_otg_hcd_start_cb,
	.stop = fh_otg_hcd_stop_cb,
	.disconnect = fh_otg_hcd_disconnect_cb,
	.session_start = fh_otg_hcd_session_start_cb,
	.resume_wakeup = fh_otg_hcd_rem_wakeup_cb,
#ifdef CONFIG_USB_FH_OTG_LPM
	.sleep = fh_otg_hcd_sleep_cb,
#endif
	.p = 0,
};

/**
 * Reset tasklet function
 */
static void reset_tasklet_func(void *data)
{
	fh_otg_hcd_t *fh_otg_hcd = (fh_otg_hcd_t *) data;
	fh_otg_core_if_t *core_if = fh_otg_hcd->core_if;
	hprt0_data_t hprt0;

	FH_DEBUGPL(DBG_HCDV, "USB RESET tasklet called\n");

	hprt0.d32 = fh_otg_read_hprt0(core_if);
	hprt0.b.prtrst = 1;
	FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
	fh_mdelay(60);

	hprt0.b.prtrst = 0;
	FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
	fh_otg_hcd->flags.b.port_reset_change = 1;
}

static void qh_list_free(fh_otg_hcd_t * hcd, fh_list_link_t * qh_list)
{
	fh_list_link_t *item;
	fh_otg_qh_t *qh;
	fh_irqflags_t flags;

	if (!qh_list->next) {
		/* The list hasn't been initialized yet. */
		return;
	}
	/*
	 * Hold spinlock here. Not needed in that case if bellow 
	 * function is being called from ISR 
	 */
	FH_SPINLOCK_IRQSAVE(hcd->lock, &flags);
	/* Ensure there are no QTDs or URBs left. */
	kill_urbs_in_qh_list(hcd, qh_list);
	FH_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);

	FH_LIST_FOREACH(item, qh_list) {
		qh = FH_LIST_ENTRY(item, fh_otg_qh_t, qh_list_entry);
		fh_otg_hcd_qh_remove_and_free(hcd, qh);
	}
}

/**
 * Exit from Hibernation if Host did not detect SRP from connected SRP capable
 * Device during SRP time by host power up.
 */
void fh_otg_hcd_power_up(void *ptr)
{
	gpwrdn_data_t gpwrdn = {.d32 = 0 };
	fh_otg_core_if_t *core_if = (fh_otg_core_if_t *) ptr;

	FH_PRINTF("%s called\n", __FUNCTION__);

	if (!core_if->hibernation_suspend) {
		FH_PRINTF("Already exited from Hibernation\n");
		return;
	}

	/* Switch on the voltage to the core */
	gpwrdn.b.pwrdnswtch = 1;
	FH_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);
	fh_udelay(10);

	/* Reset the core */
	gpwrdn.d32 = 0;
	gpwrdn.b.pwrdnrstn = 1;
	FH_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);
	fh_udelay(10);

	/* Disable power clamps */
	gpwrdn.d32 = 0;
	gpwrdn.b.pwrdnclmp = 1;
	FH_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);

	/* Remove reset the core signal */
	gpwrdn.d32 = 0;
	gpwrdn.b.pwrdnrstn = 1;
	FH_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, 0, gpwrdn.d32);
	fh_udelay(10);

	/* Disable PMU interrupt */
	gpwrdn.d32 = 0;
	gpwrdn.b.pmuintsel = 1;
	FH_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);

	core_if->hibernation_suspend = 0;

	/* Disable PMU */
	gpwrdn.d32 = 0;
	gpwrdn.b.pmuactv = 1;
	FH_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);
	fh_udelay(10);

	/* Enable VBUS */
	gpwrdn.d32 = 0;
	gpwrdn.b.dis_vbus = 1;
	FH_MODIFY_REG32(&core_if->core_global_regs->gpwrdn, gpwrdn.d32, 0);

	core_if->op_state = A_HOST;
	fh_otg_core_init(core_if);
	fh_otg_enable_global_interrupts(core_if);
	cil_hcd_start(core_if);
}

/**
 * Frees secondary storage associated with the fh_otg_hcd structure contained
 * in the struct usb_hcd field.
 */
static void fh_otg_hcd_free(fh_otg_hcd_t * fh_otg_hcd)
{
	int i;

	FH_DEBUGPL(DBG_HCD, "FH OTG HCD FREE\n");

	del_timers(fh_otg_hcd);

	/* Free memory for QH/QTD lists */
	qh_list_free(fh_otg_hcd, &fh_otg_hcd->non_periodic_sched_inactive);
	qh_list_free(fh_otg_hcd, &fh_otg_hcd->non_periodic_sched_active);
	qh_list_free(fh_otg_hcd, &fh_otg_hcd->periodic_sched_inactive);
	qh_list_free(fh_otg_hcd, &fh_otg_hcd->periodic_sched_ready);
	qh_list_free(fh_otg_hcd, &fh_otg_hcd->periodic_sched_assigned);
	qh_list_free(fh_otg_hcd, &fh_otg_hcd->periodic_sched_queued);

	/* Free memory for the host channels. */
	for (i = 0; i < MAX_EPS_CHANNELS; i++) {
		fh_hc_t *hc = fh_otg_hcd->hc_ptr_array[i];

#ifdef DEBUG
		if (fh_otg_hcd->core_if->hc_xfer_timer[i]) {
			FH_TIMER_FREE(fh_otg_hcd->core_if->hc_xfer_timer[i]);
		}
#endif
		if (hc != NULL) {
			FH_DEBUGPL(DBG_HCDV, "HCD Free channel #%i, hc=%p\n",
				    i, hc);
			FH_FREE(hc);
		}
	}

	if (fh_otg_hcd->core_if->dma_enable) {
		if (fh_otg_hcd->status_buf_dma) {
			FH_DMA_FREE(FH_OTG_HCD_STATUS_BUF_SIZE,
				     fh_otg_hcd->status_buf,
				     fh_otg_hcd->status_buf_dma);
		}
	} else if (fh_otg_hcd->status_buf != NULL) {
		FH_FREE(fh_otg_hcd->status_buf);
	}
	FH_SPINLOCK_FREE(fh_otg_hcd->lock);
	/* Set core_if's lock pointer to NULL */
	fh_otg_hcd->core_if->lock = NULL;

	FH_TIMER_FREE(fh_otg_hcd->conn_timer);
	FH_TASK_FREE(fh_otg_hcd->reset_tasklet);

#ifdef FH_DEV_SRPCAP
	if (fh_otg_hcd->core_if->power_down == 2 &&
	    fh_otg_hcd->core_if->pwron_timer) {
		FH_TIMER_FREE(fh_otg_hcd->core_if->pwron_timer);
	}
#endif
	FH_FREE(fh_otg_hcd);
}

int fh_otg_hcd_init(fh_otg_hcd_t * hcd, fh_otg_core_if_t * core_if)
{
	int retval = 0;
	int num_channels;
	int i;
	fh_hc_t *channel;

	hcd->lock = FH_SPINLOCK_ALLOC();
	if (!hcd->lock) {
		FH_ERROR("Could not allocate lock for pcd");
		FH_FREE(hcd);
		retval = -FH_E_NO_MEMORY;
		goto out;
	}
	hcd->core_if = core_if;

	/* Register the HCD CIL Callbacks */
	fh_otg_cil_register_hcd_callbacks(hcd->core_if,
					   &hcd_cil_callbacks, hcd);

	/* Initialize the non-periodic schedule. */
	FH_LIST_INIT(&hcd->non_periodic_sched_inactive);
	FH_LIST_INIT(&hcd->non_periodic_sched_active);

	/* Initialize the periodic schedule. */
	FH_LIST_INIT(&hcd->periodic_sched_inactive);
	FH_LIST_INIT(&hcd->periodic_sched_ready);
	FH_LIST_INIT(&hcd->periodic_sched_assigned);
	FH_LIST_INIT(&hcd->periodic_sched_queued);

	/*
	 * Create a host channel descriptor for each host channel implemented
	 * in the controller. Initialize the channel descriptor array.
	 */
	FH_CIRCLEQ_INIT(&hcd->free_hc_list);
	num_channels = hcd->core_if->core_params->host_channels;
	FH_MEMSET(hcd->hc_ptr_array, 0, sizeof(hcd->hc_ptr_array));
	for (i = 0; i < num_channels; i++) {
		channel = FH_ALLOC(sizeof(fh_hc_t));
		if (channel == NULL) {
			retval = -FH_E_NO_MEMORY;
			FH_ERROR("%s: host channel allocation failed\n",
				  __func__);
			fh_otg_hcd_free(hcd);
			goto out;
		}
		channel->hc_num = i;
		hcd->hc_ptr_array[i] = channel;
#ifdef DEBUG
		hcd->core_if->hc_xfer_timer[i] =
		    FH_TIMER_ALLOC("hc timer", hc_xfer_timeout,
				    &hcd->core_if->hc_xfer_info[i]);
#endif
		FH_DEBUGPL(DBG_HCDV, "HCD Added channel #%d, hc=%p\n", i,
			    channel);
	}

	/* Initialize the Connection timeout timer. */
	hcd->conn_timer = FH_TIMER_ALLOC("Connection timer",
					  fh_otg_hcd_connect_timeout, hcd);

	/* Initialize reset tasklet. */
	hcd->reset_tasklet = FH_TASK_ALLOC("reset_tasklet", reset_tasklet_func, hcd);
#ifdef FH_DEV_SRPCAP
	if (hcd->core_if->power_down == 2) {
		/* Initialize Power on timer for Host power up in case hibernation */
		hcd->core_if->pwron_timer = FH_TIMER_ALLOC("PWRON TIMER",
									fh_otg_hcd_power_up, core_if);
	}
#endif	

	/*
	 * Allocate space for storing data on status transactions. Normally no
	 * data is sent, but this space acts as a bit bucket. This must be
	 * done after usb_add_hcd since that function allocates the DMA buffer
	 * pool.
	 */
	if (hcd->core_if->dma_enable) {
		hcd->status_buf =
		    FH_DMA_ALLOC(FH_OTG_HCD_STATUS_BUF_SIZE,
				  &hcd->status_buf_dma);
	} else {
		hcd->status_buf = FH_ALLOC(FH_OTG_HCD_STATUS_BUF_SIZE);
	}
	if (!hcd->status_buf) {
		retval = -FH_E_NO_MEMORY;
		FH_ERROR("%s: status_buf allocation failed\n", __func__);
		fh_otg_hcd_free(hcd);
		goto out;
	}

	hcd->otg_port = 1;
	hcd->frame_list = NULL;
	hcd->frame_list_dma = 0;
	hcd->periodic_qh_count = 0;
out:
	return retval;
}

void fh_otg_hcd_remove(fh_otg_hcd_t * hcd)
{
	/* Turn off all host-specific interrupts. */
	fh_otg_disable_host_interrupts(hcd->core_if);

	fh_otg_hcd_free(hcd);
}

/**
 * Initializes dynamic portions of the FH_otg HCD state.
 */
static void fh_otg_hcd_reinit(fh_otg_hcd_t * hcd)
{
	int num_channels;
	int i;
	fh_hc_t *channel;
	fh_hc_t *channel_tmp;

	hcd->flags.d32 = 0;

	hcd->non_periodic_qh_ptr = &hcd->non_periodic_sched_active;
	hcd->non_periodic_channels = 0;
	hcd->periodic_channels = 0;

	/*
	 * Put all channels in the free channel list and clean up channel
	 * states.
	 */
	FH_CIRCLEQ_FOREACH_SAFE(channel, channel_tmp,
				 &hcd->free_hc_list, hc_list_entry) {
		FH_CIRCLEQ_REMOVE(&hcd->free_hc_list, channel, hc_list_entry);
	}

	num_channels = hcd->core_if->core_params->host_channels;
	for (i = 0; i < num_channels; i++) {
		channel = hcd->hc_ptr_array[i];
		FH_CIRCLEQ_INSERT_TAIL(&hcd->free_hc_list, channel,
					hc_list_entry);
		fh_otg_hc_cleanup(hcd->core_if, channel);
	}

	/* Initialize the FH core for host mode operation. */
	fh_otg_core_host_init(hcd->core_if);

	/* Set core_if's lock pointer to the hcd->lock */
	hcd->core_if->lock = hcd->lock;
}

/**
 * Assigns transactions from a QTD to a free host channel and initializes the
 * host channel to perform the transactions. The host channel is removed from
 * the free list.
 *
 * @param hcd The HCD state structure.
 * @param qh Transactions from the first QTD for this QH are selected and
 * assigned to a free host channel.
 */
static int assign_and_init_hc(fh_otg_hcd_t * hcd, fh_otg_qh_t * qh)
{
	fh_hc_t *hc = NULL;
	fh_otg_qtd_t *qtd;
	fh_otg_hcd_urb_t *urb;
	void* ptr = NULL;
	hcchar_data_t hcchar;
	int num_channels;
	int i;

	FH_DEBUGPL(DBG_HCDV, "%s(%p,%p)\n", __func__, hcd, qh);

	qtd = FH_CIRCLEQ_FIRST(&qh->qtd_list);
	if (!qtd) {
		FH_ERROR("qh qtd is none\n");
		return -ENOMEM;
	}
	urb = qtd->urb;
	if (!urb) {
		FH_ERROR("hcd urb is none\n");
		return -ENOMEM;
	}
	num_channels = hcd->core_if->core_params->host_channels;

	/* WA to not select channel with chdis bit set, this was 
	 * observed after role switch as part of OTG 2.0 HNP
	 */
	for (i = 0; i < num_channels; i++) {
		hc = FH_CIRCLEQ_FIRST(&hcd->free_hc_list);
		hcchar.d32 = FH_READ_REG32(&hcd->core_if->host_if->hc_regs[hc->hc_num]->hcchar);
		FH_DEBUGPL(DBG_HCDV, "HC num = %d HCCHAR %08x\n", hc->hc_num, hcchar.d32);
		if(!hcchar.b.chdis && !hcchar.b.chen)
			break;
		FH_CIRCLEQ_REMOVE_INIT(&hcd->free_hc_list, hc, hc_list_entry);
		FH_CIRCLEQ_INSERT_TAIL(&hcd->free_hc_list, hc, hc_list_entry);
		hc = NULL;
	}
		if (!hc) {
			FH_ERROR("No free channel with en and dis bits 0\n");
			return -ENOMEM;
	}



	/* Remove the host channel from the free list. */
	FH_CIRCLEQ_REMOVE_INIT(&hcd->free_hc_list, hc, hc_list_entry);

	qh->channel = hc;

	qtd->in_process = 1;

	/*
	 * Use usb_pipedevice to determine device address. This address is
	 * 0 before the SET_ADDRESS command and the correct address afterward.
	 */
	hc->dev_addr = fh_otg_hcd_get_dev_addr(&urb->pipe_info);
	hc->ep_num = fh_otg_hcd_get_ep_num(&urb->pipe_info);
	hc->speed = qh->dev_speed;
	hc->max_packet = fh_max_packet(qh->maxp);

	hc->xfer_started = 0;
	hc->halt_status = FH_OTG_HC_XFER_NO_HALT_STATUS;
	hc->error_state = (qtd->error_count > 0);
	hc->halt_on_queue = 0;
	hc->halt_pending = 0;
	hc->requests = 0;

	/*
	 * The following values may be modified in the transfer type section
	 * below. The xfer_len value may be reduced when the transfer is
	 * started to accommodate the max widths of the XferSize and PktCnt
	 * fields in the HCTSIZn register.
	 */

	hc->ep_is_in = (fh_otg_hcd_is_pipe_in(&urb->pipe_info) != 0);
	if (hc->ep_is_in) {
		hc->do_ping = 0;
	} else {
		hc->do_ping = qh->ping_state;
	}

	hc->data_pid_start = qh->data_toggle;
	hc->multi_count = 1;

	if (urb->actual_length > urb->length &&
		!fh_otg_hcd_is_pipe_in(&urb->pipe_info)) {
		urb->actual_length = urb->length;
	}

	if (hcd->core_if->dma_enable) {
		hc->xfer_buff = (uint8_t *) urb->dma + urb->actual_length;

		/* For non-dword aligned case */
		if (((unsigned long)hc->xfer_buff & 0x3)
		    && !hcd->core_if->dma_desc_enable) {
			ptr = (uint8_t *) urb->buf + urb->actual_length;
		}
	} else {
		hc->xfer_buff = (uint8_t *) urb->buf + urb->actual_length;
	}
	hc->xfer_len = urb->length - urb->actual_length;
	hc->xfer_count = 0;

	/*
	 * Set the split attributes
	 */
	hc->do_split = 0;
	if (qh->do_split) {
		uint32_t hub_addr, port_addr;
		hc->do_split = 1;
		hc->xact_pos = qtd->isoc_split_pos;
		hc->complete_split = qtd->complete_split;
		hcd->fops->hub_info(hcd, urb->priv, &hub_addr, &port_addr);
		hc->hub_addr = (uint8_t) hub_addr;
		hc->port_addr = (uint8_t) port_addr;
	}

	switch (fh_otg_hcd_get_pipe_type(&urb->pipe_info)) {
	case UE_CONTROL:
		hc->ep_type = FH_OTG_EP_TYPE_CONTROL;
		switch (qtd->control_phase) {
		case FH_OTG_CONTROL_SETUP:
			FH_DEBUGPL(DBG_HCDV, "  Control setup transaction\n");
			hc->do_ping = 0;
			hc->ep_is_in = 0;
			hc->data_pid_start = FH_OTG_HC_PID_SETUP;
			if (hcd->core_if->dma_enable) {
				hc->xfer_buff = (uint8_t *) urb->setup_dma;
			} else {
				hc->xfer_buff = (uint8_t *) urb->setup_packet;
			}
			hc->xfer_len = 8;
			ptr = NULL;
			break;
		case FH_OTG_CONTROL_DATA:
			FH_DEBUGPL(DBG_HCDV, "  Control data transaction\n");
			hc->data_pid_start = qtd->data_toggle;
			break;
		case FH_OTG_CONTROL_STATUS:
			/*
			 * Direction is opposite of data direction or IN if no
			 * data.
			 */
			FH_DEBUGPL(DBG_HCDV, "  Control status transaction\n");
			if (urb->length == 0) {
				hc->ep_is_in = 1;
			} else {
				hc->ep_is_in =
				    fh_otg_hcd_is_pipe_out(&urb->pipe_info);
			}
			if (hc->ep_is_in) {
				hc->do_ping = 0;
			}

			hc->data_pid_start = FH_OTG_HC_PID_DATA1;

			hc->xfer_len = 0;
			if (hcd->core_if->dma_enable) {
				hc->xfer_buff = (uint8_t *) hcd->status_buf_dma;
			} else {
				hc->xfer_buff = (uint8_t *) hcd->status_buf;
			}
			ptr = NULL;
			break;
		}
		break;
	case UE_BULK:
		hc->ep_type = FH_OTG_EP_TYPE_BULK;
		break;
	case UE_INTERRUPT:
		hc->ep_type = FH_OTG_EP_TYPE_INTR;
		break;
	case UE_ISOCHRONOUS:
		{
			struct fh_otg_hcd_iso_packet_desc *frame_desc;

			hc->ep_type = FH_OTG_EP_TYPE_ISOC;

			if (hcd->core_if->dma_desc_enable)
				break;

			frame_desc = &urb->iso_descs[qtd->isoc_frame_index];

			frame_desc->status = 0;

			if (hcd->core_if->dma_enable) {
				hc->xfer_buff = (uint8_t *) urb->dma;
			} else {
				hc->xfer_buff = (uint8_t *) urb->buf;
			}
			hc->xfer_buff +=
			    frame_desc->offset + qtd->isoc_split_offset;
			hc->xfer_len =
			    frame_desc->length - qtd->isoc_split_offset;

			/* For non-dword aligned buffers */
			if (((unsigned long)hc->xfer_buff & 0x3)
			    && hcd->core_if->dma_enable) {
				ptr =
				    (uint8_t *) urb->buf + frame_desc->offset +
				    qtd->isoc_split_offset;
			} else
				ptr = NULL;

			if (hc->xact_pos == FH_HCSPLIT_XACTPOS_ALL) {
				if (hc->xfer_len <= 188) {
					hc->xact_pos = FH_HCSPLIT_XACTPOS_ALL;
				} else {
					hc->xact_pos =
					    FH_HCSPLIT_XACTPOS_BEGIN;
				}
			}
		}
		break;
	}
	/* non DWORD-aligned buffer case */	
	if (ptr) {
		uint32_t buf_size;
		if (hc->ep_type != FH_OTG_EP_TYPE_ISOC) {
			buf_size = hcd->core_if->core_params->max_transfer_size;
		} else {				
			buf_size = 4096;
		}
		if (!qh->dw_align_buf) {
			qh->dw_align_buf = FH_DMA_ALLOC_ATOMIC(buf_size,
							 &qh->dw_align_buf_dma);
            printk(KERN_ERR "FH_DMA_ALLOC_ATOMIC (%p)\n", qh->dw_align_buf);
			if (!qh->dw_align_buf) {
				FH_ERROR
				    ("%s: Failed to allocate memory to handle "
				     "non-dword aligned buffer case\n",
				     __func__);
				return -ENOMEM;
			}
		}
		if (!hc->ep_is_in) {
			fh_memcpy(qh->dw_align_buf, ptr, hc->xfer_len);
		}
		hc->align_buff = qh->dw_align_buf_dma;
	} else {
		hc->align_buff = 0;
	}

	if (hc->ep_type == FH_OTG_EP_TYPE_INTR ||
	    hc->ep_type == FH_OTG_EP_TYPE_ISOC) {
		/*
		 * This value may be modified when the transfer is started to
		 * reflect the actual transfer length.
		 */
		hc->multi_count = fh_hb_mult(qh->maxp);
	}

	if (hcd->core_if->dma_desc_enable)
		hc->desc_list_addr = qh->desc_list_dma;

	fh_otg_hc_init(hcd->core_if, hc);
	hc->qh = qh;

	return 0;
}

/**
 * This function selects transactions from the HCD transfer schedule and
 * assigns them to available host channels. It is called from HCD interrupt
 * handler functions.
 *
 * @param hcd The HCD state structure.
 *
 * @return The types of new transactions that were assigned to host channels.
 */
fh_otg_transaction_type_e fh_otg_hcd_select_transactions(fh_otg_hcd_t * hcd)
{
	fh_list_link_t *qh_ptr;
	fh_otg_qh_t *qh;
	int num_channels;
	fh_otg_transaction_type_e ret_val = FH_OTG_TRANSACTION_NONE;

#ifdef DEBUG_SOF
	FH_DEBUGPL(DBG_HCD, "  Select Transactions\n");
#endif

	/* Process entries in the periodic ready list. */
	qh_ptr = FH_LIST_FIRST(&hcd->periodic_sched_ready);

	while (qh_ptr != &hcd->periodic_sched_ready &&
	       !FH_CIRCLEQ_EMPTY(&hcd->free_hc_list)) {

		qh = FH_LIST_ENTRY(qh_ptr, fh_otg_qh_t, qh_list_entry);
		if (assign_and_init_hc(hcd, qh))
			break;

		/*
		 * Move the QH from the periodic ready schedule to the
		 * periodic assigned schedule.
		 */
		qh_ptr = FH_LIST_NEXT(qh_ptr);
		FH_LIST_MOVE_HEAD(&hcd->periodic_sched_assigned,
				   &qh->qh_list_entry);

		ret_val = FH_OTG_TRANSACTION_PERIODIC;
	}

	/*
	 * Process entries in the inactive portion of the non-periodic
	 * schedule. Some free host channels may not be used if they are
	 * reserved for periodic transfers.
	 */
	qh_ptr = hcd->non_periodic_sched_inactive.next;
	num_channels = hcd->core_if->core_params->host_channels;
	while (qh_ptr != &hcd->non_periodic_sched_inactive &&
	       (hcd->non_periodic_channels <
		num_channels - hcd->periodic_channels) &&
	       !FH_CIRCLEQ_EMPTY(&hcd->free_hc_list)) {
		qh = FH_LIST_ENTRY(qh_ptr, fh_otg_qh_t, qh_list_entry);

		if (FH_CIRCLEQ_EMPTY(&qh->qtd_list))
			break;
		if (assign_and_init_hc(hcd, qh))
			break;

		/*
		 * Move the QH from the non-periodic inactive schedule to the
		 * non-periodic active schedule.
		 */
		qh_ptr = FH_LIST_NEXT(qh_ptr);
		FH_LIST_MOVE_HEAD(&hcd->non_periodic_sched_active,
				   &qh->qh_list_entry);

		if (ret_val == FH_OTG_TRANSACTION_NONE) {
			ret_val = FH_OTG_TRANSACTION_NON_PERIODIC;
		} else {
			ret_val = FH_OTG_TRANSACTION_ALL;
		}

		hcd->non_periodic_channels++;
	}

	return ret_val;
}

/**
 * Attempts to queue a single transaction request for a host channel
 * associated with either a periodic or non-periodic transfer. This function
 * assumes that there is space available in the appropriate request queue. For
 * an OUT transfer or SETUP transaction in Slave mode, it checks whether space
 * is available in the appropriate Tx FIFO.
 *
 * @param hcd The HCD state structure.
 * @param hc Host channel descriptor associated with either a periodic or
 * non-periodic transfer.
 * @param fifo_dwords_avail Number of DWORDs available in the periodic Tx
 * FIFO for periodic transfers or the non-periodic Tx FIFO for non-periodic
 * transfers.
 *
 * @return 1 if a request is queued and more requests may be needed to
 * complete the transfer, 0 if no more requests are required for this
 * transfer, -1 if there is insufficient space in the Tx FIFO.
 */
static int queue_transaction(fh_otg_hcd_t * hcd,
			     fh_hc_t * hc, uint16_t fifo_dwords_avail)
{
	int retval;

	if (hcd->core_if->dma_enable) {
		if (hcd->core_if->dma_desc_enable) {
			if (!hc->xfer_started
			    || (hc->ep_type == FH_OTG_EP_TYPE_ISOC)) {
				fh_otg_hcd_start_xfer_ddma(hcd, hc->qh);
				hc->qh->ping_state = 0;
			}
		} else if (!hc->xfer_started) {
			fh_otg_hc_start_transfer(hcd->core_if, hc);
			hc->qh->ping_state = 0;
		}
		retval = 0;
	} else if (hc->halt_pending) {
		/* Don't queue a request if the channel has been halted. */
		retval = 0;
	} else if (hc->halt_on_queue) {
		fh_otg_hc_halt(hcd->core_if, hc, hc->halt_status);
		retval = 0;
	} else if (hc->do_ping) {
		if (!hc->xfer_started) {
			fh_otg_hc_start_transfer(hcd->core_if, hc);
		}
		retval = 0;
	} else if (!hc->ep_is_in || hc->data_pid_start == FH_OTG_HC_PID_SETUP) {
		if ((fifo_dwords_avail * 4) >= hc->max_packet) {
			if (!hc->xfer_started) {
				fh_otg_hc_start_transfer(hcd->core_if, hc);
				retval = 1;
			} else {
				retval =
				    fh_otg_hc_continue_transfer(hcd->core_if,
								 hc);
			}
		} else {
			retval = -1;
		}
	} else {
		if (!hc->xfer_started) {
			fh_otg_hc_start_transfer(hcd->core_if, hc);
			retval = 1;
		} else {
			retval = fh_otg_hc_continue_transfer(hcd->core_if, hc);
		}
	}

	return retval;
}

/**
 * Processes periodic channels for the next frame and queues transactions for
 * these channels to the FH_otg controller. After queueing transactions, the
 * Periodic Tx FIFO Empty interrupt is enabled if there are more transactions
 * to queue as Periodic Tx FIFO or request queue space becomes available.
 * Otherwise, the Periodic Tx FIFO Empty interrupt is disabled.
 */
static void process_periodic_channels(fh_otg_hcd_t * hcd)
{
	hptxsts_data_t tx_status;
	fh_list_link_t *qh_ptr;
	fh_otg_qh_t *qh;
	int status;
	int no_queue_space = 0;
	int no_fifo_space = 0;

	fh_otg_host_global_regs_t *host_regs;
	host_regs = hcd->core_if->host_if->host_global_regs;

	FH_DEBUGPL(DBG_HCDV, "Queue periodic transactions\n");
#ifdef DEBUG
	tx_status.d32 = FH_READ_REG32(&host_regs->hptxsts);
	FH_DEBUGPL(DBG_HCDV,
		    "  P Tx Req Queue Space Avail (before queue): %d\n",
		    tx_status.b.ptxqspcavail);
	FH_DEBUGPL(DBG_HCDV, "  P Tx FIFO Space Avail (before queue): %d\n",
		    tx_status.b.ptxfspcavail);
#endif

	qh_ptr = hcd->periodic_sched_assigned.next;
	while (qh_ptr != &hcd->periodic_sched_assigned) {
		tx_status.d32 = FH_READ_REG32(&host_regs->hptxsts);
		if (tx_status.b.ptxqspcavail == 0) {
			no_queue_space = 1;
			break;
		}

		qh = FH_LIST_ENTRY(qh_ptr, fh_otg_qh_t, qh_list_entry);

		/*
		 * Set a flag if we're queuing high-bandwidth in slave mode.
		 * The flag prevents any halts to get into the request queue in
		 * the middle of multiple high-bandwidth packets getting queued.
		 */
		if (!hcd->core_if->dma_enable && qh->channel->multi_count > 1) {
			hcd->core_if->queuing_high_bandwidth = 1;
		}
		status =
		    queue_transaction(hcd, qh->channel,
				      tx_status.b.ptxfspcavail);
		if (status < 0) {
			no_fifo_space = 1;
			break;
		}

		/*
		 * In Slave mode, stay on the current transfer until there is
		 * nothing more to do or the high-bandwidth request count is
		 * reached. In DMA mode, only need to queue one request. The
		 * controller automatically handles multiple packets for
		 * high-bandwidth transfers.
		 */
		if (hcd->core_if->dma_enable || status == 0 ||
		    qh->channel->requests == qh->channel->multi_count) {
			qh_ptr = qh_ptr->next;
			/*
			 * Move the QH from the periodic assigned schedule to
			 * the periodic queued schedule.
			 */
			FH_LIST_MOVE_HEAD(&hcd->periodic_sched_queued,
					   &qh->qh_list_entry);

			/* done queuing high bandwidth */
			hcd->core_if->queuing_high_bandwidth = 0;
		}
	}

	if (!hcd->core_if->dma_enable) {
		fh_otg_core_global_regs_t *global_regs;
		gintmsk_data_t intr_mask = {.d32 = 0 };

		global_regs = hcd->core_if->core_global_regs;
		intr_mask.b.ptxfempty = 1;
#ifdef DEBUG
		tx_status.d32 = FH_READ_REG32(&host_regs->hptxsts);
		FH_DEBUGPL(DBG_HCDV,
			    "  P Tx Req Queue Space Avail (after queue): %d\n",
			    tx_status.b.ptxqspcavail);
		FH_DEBUGPL(DBG_HCDV,
			    "  P Tx FIFO Space Avail (after queue): %d\n",
			    tx_status.b.ptxfspcavail);
#endif
		if (!FH_LIST_EMPTY(&hcd->periodic_sched_assigned) ||
		    no_queue_space || no_fifo_space) {
			/*
			 * May need to queue more transactions as the request
			 * queue or Tx FIFO empties. Enable the periodic Tx
			 * FIFO empty interrupt. (Always use the half-empty
			 * level to ensure that new requests are loaded as
			 * soon as possible.)
			 */
			FH_MODIFY_REG32(&global_regs->gintmsk, 0,
					 intr_mask.d32);
		} else {
			/*
			 * Disable the Tx FIFO empty interrupt since there are
			 * no more transactions that need to be queued right
			 * now. This function is called from interrupt
			 * handlers to queue more transactions as transfer
			 * states change.
			 */
			FH_MODIFY_REG32(&global_regs->gintmsk, intr_mask.d32,
					 0);
		}
	}
}

/**
 * Processes active non-periodic channels and queues transactions for these
 * channels to the FH_otg controller. After queueing transactions, the NP Tx
 * FIFO Empty interrupt is enabled if there are more transactions to queue as
 * NP Tx FIFO or request queue space becomes available. Otherwise, the NP Tx
 * FIFO Empty interrupt is disabled.
 */
static void process_non_periodic_channels(fh_otg_hcd_t * hcd)
{
	gnptxsts_data_t tx_status;
	fh_list_link_t *orig_qh_ptr;
	fh_otg_qh_t *qh;
	int status;
	int no_queue_space = 0;
	int no_fifo_space = 0;
	int more_to_do = 0;

	fh_otg_core_global_regs_t *global_regs =
	    hcd->core_if->core_global_regs;

	FH_DEBUGPL(DBG_HCDV, "Queue non-periodic transactions\n");
#ifdef DEBUG
	tx_status.d32 = FH_READ_REG32(&global_regs->gnptxsts);
	FH_DEBUGPL(DBG_HCDV,
		    "  NP Tx Req Queue Space Avail (before queue): %d\n",
		    tx_status.b.nptxqspcavail);
	FH_DEBUGPL(DBG_HCDV, "  NP Tx FIFO Space Avail (before queue): %d\n",
		    tx_status.b.nptxfspcavail);
#endif
	/*
	 * Keep track of the starting point. Skip over the start-of-list
	 * entry.
	 */
	if (hcd->non_periodic_qh_ptr == &hcd->non_periodic_sched_active) {
		hcd->non_periodic_qh_ptr = hcd->non_periodic_qh_ptr->next;
	}
	orig_qh_ptr = hcd->non_periodic_qh_ptr;

	/*
	 * Process once through the active list or until no more space is
	 * available in the request queue or the Tx FIFO.
	 */
	do {
		tx_status.d32 = FH_READ_REG32(&global_regs->gnptxsts);
		if (!hcd->core_if->dma_enable && tx_status.b.nptxqspcavail == 0) {
			no_queue_space = 1;
			break;
		}

		qh = FH_LIST_ENTRY(hcd->non_periodic_qh_ptr, fh_otg_qh_t,
				    qh_list_entry);
		status =
		    queue_transaction(hcd, qh->channel,
				      tx_status.b.nptxfspcavail);

		if (status > 0) {
			more_to_do = 1;
		} else if (status < 0) {
			no_fifo_space = 1;
			break;
		}

		/* Advance to next QH, skipping start-of-list entry. */
		hcd->non_periodic_qh_ptr = hcd->non_periodic_qh_ptr->next;
		if (hcd->non_periodic_qh_ptr == &hcd->non_periodic_sched_active) {
			hcd->non_periodic_qh_ptr =
			    hcd->non_periodic_qh_ptr->next;
		}

	} while (hcd->non_periodic_qh_ptr != orig_qh_ptr);

	if (!hcd->core_if->dma_enable) {
		gintmsk_data_t intr_mask = {.d32 = 0 };
		intr_mask.b.nptxfempty = 1;

#ifdef DEBUG
		tx_status.d32 = FH_READ_REG32(&global_regs->gnptxsts);
		FH_DEBUGPL(DBG_HCDV,
			    "  NP Tx Req Queue Space Avail (after queue): %d\n",
			    tx_status.b.nptxqspcavail);
		FH_DEBUGPL(DBG_HCDV,
			    "  NP Tx FIFO Space Avail (after queue): %d\n",
			    tx_status.b.nptxfspcavail);
#endif
		if (more_to_do || no_queue_space || no_fifo_space) {
			/*
			 * May need to queue more transactions as the request
			 * queue or Tx FIFO empties. Enable the non-periodic
			 * Tx FIFO empty interrupt. (Always use the half-empty
			 * level to ensure that new requests are loaded as
			 * soon as possible.)
			 */
			FH_MODIFY_REG32(&global_regs->gintmsk, 0,
					 intr_mask.d32);
		} else {
			/*
			 * Disable the Tx FIFO empty interrupt since there are
			 * no more transactions that need to be queued right
			 * now. This function is called from interrupt
			 * handlers to queue more transactions as transfer
			 * states change.
			 */
			FH_MODIFY_REG32(&global_regs->gintmsk, intr_mask.d32,
					 0);
		}
	}
}

/**
 * This function processes the currently active host channels and queues
 * transactions for these channels to the FH_otg controller. It is called
 * from HCD interrupt handler functions.
 *
 * @param hcd The HCD state structure.
 * @param tr_type The type(s) of transactions to queue (non-periodic,
 * periodic, or both).
 */
void fh_otg_hcd_queue_transactions(fh_otg_hcd_t * hcd,
				    fh_otg_transaction_type_e tr_type)
{
#ifdef DEBUG_SOF
	FH_DEBUGPL(DBG_HCD, "Queue Transactions\n");
#endif
	/* Process host channels associated with periodic transfers. */
	if ((tr_type == FH_OTG_TRANSACTION_PERIODIC ||
	     tr_type == FH_OTG_TRANSACTION_ALL) &&
	    !FH_LIST_EMPTY(&hcd->periodic_sched_assigned)) {

		process_periodic_channels(hcd);
	}

	/* Process host channels associated with non-periodic transfers. */
	if (tr_type == FH_OTG_TRANSACTION_NON_PERIODIC ||
	    tr_type == FH_OTG_TRANSACTION_ALL) {
		if (!FH_LIST_EMPTY(&hcd->non_periodic_sched_active)) {
			process_non_periodic_channels(hcd);
		} else {
			/*
			 * Ensure NP Tx FIFO empty interrupt is disabled when
			 * there are no non-periodic transfers to process.
			 */
			gintmsk_data_t gintmsk = {.d32 = 0 };
			gintmsk.b.nptxfempty = 1;
			FH_MODIFY_REG32(&hcd->core_if->
					 core_global_regs->gintmsk, gintmsk.d32,
					 0);
		}
	}
}

#ifdef FH_HS_ELECT_TST
/*
 * Quick and dirty hack to implement the HS Electrical Test
 * SINGLE_STEP_GET_DEVICE_DESCRIPTOR feature.
 *
 * This code was copied from our userspace app "hset". It sends a
 * Get Device Descriptor control sequence in two parts, first the
 * Setup packet by itself, followed some time later by the In and
 * Ack packets. Rather than trying to figure out how to add this
 * functionality to the normal driver code, we just hijack the
 * hardware, using these two function to drive the hardware
 * directly.
 */

static fh_otg_core_global_regs_t *global_regs;
static fh_otg_host_global_regs_t *hc_global_regs;
static fh_otg_hc_regs_t *hc_regs;
static uint32_t *data_fifo;

static void do_setup(void)
{
	gintsts_data_t gintsts;
	hctsiz_data_t hctsiz;
	hcchar_data_t hcchar;
	haint_data_t haint;
	hcint_data_t hcint;

	/* Enable HAINTs */
	FH_WRITE_REG32(&hc_global_regs->haintmsk, 0x0001);

	/* Enable HCINTs */
	FH_WRITE_REG32(&hc_regs->hcintmsk, 0x04a3);

	/* Read GINTSTS */
	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

	/* Read HAINT */
	haint.d32 = FH_READ_REG32(&hc_global_regs->haint);

	/* Read HCINT */
	hcint.d32 = FH_READ_REG32(&hc_regs->hcint);

	/* Read HCCHAR */
	hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);

	/* Clear HCINT */
	FH_WRITE_REG32(&hc_regs->hcint, hcint.d32);

	/* Clear HAINT */
	FH_WRITE_REG32(&hc_global_regs->haint, haint.d32);

	/* Clear GINTSTS */
	FH_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

	/* Read GINTSTS */
	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

	/*
	 * Send Setup packet (Get Device Descriptor)
	 */

	/* Make sure channel is disabled */
	hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);
	if (hcchar.b.chen) {
		hcchar.b.chdis = 1;
//              hcchar.b.chen = 1;
		FH_WRITE_REG32(&hc_regs->hcchar, hcchar.d32);
		//sleep(1);
		fh_mdelay(1000);

		/* Read GINTSTS */
		gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

		/* Read HAINT */
		haint.d32 = FH_READ_REG32(&hc_global_regs->haint);

		/* Read HCINT */
		hcint.d32 = FH_READ_REG32(&hc_regs->hcint);

		/* Read HCCHAR */
		hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);

		/* Clear HCINT */
		FH_WRITE_REG32(&hc_regs->hcint, hcint.d32);

		/* Clear HAINT */
		FH_WRITE_REG32(&hc_global_regs->haint, haint.d32);

		/* Clear GINTSTS */
		FH_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

		hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);
	}

	/* Set HCTSIZ */
	hctsiz.d32 = 0;
	hctsiz.b.xfersize = 8;
	hctsiz.b.pktcnt = 1;
	hctsiz.b.pid = FH_OTG_HC_PID_SETUP;
	FH_WRITE_REG32(&hc_regs->hctsiz, hctsiz.d32);

	/* Set HCCHAR */
	hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);
	hcchar.b.eptype = FH_OTG_EP_TYPE_CONTROL;
	hcchar.b.epdir = 0;
	hcchar.b.epnum = 0;
	hcchar.b.mps = 8;
	hcchar.b.chen = 1;
	FH_WRITE_REG32(&hc_regs->hcchar, hcchar.d32);

	/* Fill FIFO with Setup data for Get Device Descriptor */
	data_fifo = (uint32_t *) ((char *)global_regs + 0x1000);
	FH_WRITE_REG32(data_fifo++, 0x01000680);
	FH_WRITE_REG32(data_fifo++, 0x00080000);

	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

	/* Wait for host channel interrupt */
	do {
		gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);
	} while (gintsts.b.hcintr == 0);

	/* Disable HCINTs */
	FH_WRITE_REG32(&hc_regs->hcintmsk, 0x0000);

	/* Disable HAINTs */
	FH_WRITE_REG32(&hc_global_regs->haintmsk, 0x0000);

	/* Read HAINT */
	haint.d32 = FH_READ_REG32(&hc_global_regs->haint);

	/* Read HCINT */
	hcint.d32 = FH_READ_REG32(&hc_regs->hcint);

	/* Read HCCHAR */
	hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);

	/* Clear HCINT */
	FH_WRITE_REG32(&hc_regs->hcint, hcint.d32);

	/* Clear HAINT */
	FH_WRITE_REG32(&hc_global_regs->haint, haint.d32);

	/* Clear GINTSTS */
	FH_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

	/* Read GINTSTS */
	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);
}

static void do_in_ack(void)
{
	gintsts_data_t gintsts;
	hctsiz_data_t hctsiz;
	hcchar_data_t hcchar;
	haint_data_t haint;
	hcint_data_t hcint;
	host_grxsts_data_t grxsts;

	/* Enable HAINTs */
	FH_WRITE_REG32(&hc_global_regs->haintmsk, 0x0001);

	/* Enable HCINTs */
	FH_WRITE_REG32(&hc_regs->hcintmsk, 0x04a3);

	/* Read GINTSTS */
	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

	/* Read HAINT */
	haint.d32 = FH_READ_REG32(&hc_global_regs->haint);

	/* Read HCINT */
	hcint.d32 = FH_READ_REG32(&hc_regs->hcint);

	/* Read HCCHAR */
	hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);

	/* Clear HCINT */
	FH_WRITE_REG32(&hc_regs->hcint, hcint.d32);

	/* Clear HAINT */
	FH_WRITE_REG32(&hc_global_regs->haint, haint.d32);

	/* Clear GINTSTS */
	FH_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

	/* Read GINTSTS */
	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

	/*
	 * Receive Control In packet
	 */

	/* Make sure channel is disabled */
	hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);
	if (hcchar.b.chen) {
		hcchar.b.chdis = 1;
		hcchar.b.chen = 1;
		FH_WRITE_REG32(&hc_regs->hcchar, hcchar.d32);
		//sleep(1);
		fh_mdelay(1000);

		/* Read GINTSTS */
		gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

		/* Read HAINT */
		haint.d32 = FH_READ_REG32(&hc_global_regs->haint);

		/* Read HCINT */
		hcint.d32 = FH_READ_REG32(&hc_regs->hcint);

		/* Read HCCHAR */
		hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);

		/* Clear HCINT */
		FH_WRITE_REG32(&hc_regs->hcint, hcint.d32);

		/* Clear HAINT */
		FH_WRITE_REG32(&hc_global_regs->haint, haint.d32);

		/* Clear GINTSTS */
		FH_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

		hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);
	}

	/* Set HCTSIZ */
	hctsiz.d32 = 0;
	hctsiz.b.xfersize = 8;
	hctsiz.b.pktcnt = 1;
	hctsiz.b.pid = FH_OTG_HC_PID_DATA1;
	FH_WRITE_REG32(&hc_regs->hctsiz, hctsiz.d32);

	/* Set HCCHAR */
	hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);
	hcchar.b.eptype = FH_OTG_EP_TYPE_CONTROL;
	hcchar.b.epdir = 1;
	hcchar.b.epnum = 0;
	hcchar.b.mps = 8;
	hcchar.b.chen = 1;
	FH_WRITE_REG32(&hc_regs->hcchar, hcchar.d32);

	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

	/* Wait for receive status queue interrupt */
	do {
		gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);
	} while (gintsts.b.rxstsqlvl == 0);

	/* Read RXSTS */
	grxsts.d32 = FH_READ_REG32(&global_regs->grxstsp);

	/* Clear RXSTSQLVL in GINTSTS */
	gintsts.d32 = 0;
	gintsts.b.rxstsqlvl = 1;
	FH_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

	switch (grxsts.b.pktsts) {
	case FH_GRXSTS_PKTSTS_IN:
		/* Read the data into the host buffer */
		if (grxsts.b.bcnt > 0) {
			int i;
			int word_count = (grxsts.b.bcnt + 3) / 4;

			data_fifo = (uint32_t *) ((char *)global_regs + 0x1000);

			for (i = 0; i < word_count; i++) {
				(void)FH_READ_REG32(data_fifo++);
			}
		}
		break;

	default:
		break;
	}

	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

	/* Wait for receive status queue interrupt */
	do {
		gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);
	} while (gintsts.b.rxstsqlvl == 0);

	/* Read RXSTS */
	grxsts.d32 = FH_READ_REG32(&global_regs->grxstsp);

	/* Clear RXSTSQLVL in GINTSTS */
	gintsts.d32 = 0;
	gintsts.b.rxstsqlvl = 1;
	FH_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

	switch (grxsts.b.pktsts) {
	case FH_GRXSTS_PKTSTS_IN_XFER_COMP:
		break;

	default:
		break;
	}

	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

	/* Wait for host channel interrupt */
	do {
		gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);
	} while (gintsts.b.hcintr == 0);

	/* Read HAINT */
	haint.d32 = FH_READ_REG32(&hc_global_regs->haint);

	/* Read HCINT */
	hcint.d32 = FH_READ_REG32(&hc_regs->hcint);

	/* Read HCCHAR */
	hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);

	/* Clear HCINT */
	FH_WRITE_REG32(&hc_regs->hcint, hcint.d32);

	/* Clear HAINT */
	FH_WRITE_REG32(&hc_global_regs->haint, haint.d32);

	/* Clear GINTSTS */
	FH_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

	/* Read GINTSTS */
	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

//      usleep(100000);
//      mdelay(100);
	fh_mdelay(1);

	/*
	 * Send handshake packet
	 */

	/* Read HAINT */
	haint.d32 = FH_READ_REG32(&hc_global_regs->haint);

	/* Read HCINT */
	hcint.d32 = FH_READ_REG32(&hc_regs->hcint);

	/* Read HCCHAR */
	hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);

	/* Clear HCINT */
	FH_WRITE_REG32(&hc_regs->hcint, hcint.d32);

	/* Clear HAINT */
	FH_WRITE_REG32(&hc_global_regs->haint, haint.d32);

	/* Clear GINTSTS */
	FH_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

	/* Read GINTSTS */
	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

	/* Make sure channel is disabled */
	hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);
	if (hcchar.b.chen) {
		hcchar.b.chdis = 1;
		hcchar.b.chen = 1;
		FH_WRITE_REG32(&hc_regs->hcchar, hcchar.d32);
		//sleep(1);
		fh_mdelay(1000);

		/* Read GINTSTS */
		gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

		/* Read HAINT */
		haint.d32 = FH_READ_REG32(&hc_global_regs->haint);

		/* Read HCINT */
		hcint.d32 = FH_READ_REG32(&hc_regs->hcint);

		/* Read HCCHAR */
		hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);

		/* Clear HCINT */
		FH_WRITE_REG32(&hc_regs->hcint, hcint.d32);

		/* Clear HAINT */
		FH_WRITE_REG32(&hc_global_regs->haint, haint.d32);

		/* Clear GINTSTS */
		FH_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

		hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);
	}

	/* Set HCTSIZ */
	hctsiz.d32 = 0;
	hctsiz.b.xfersize = 0;
	hctsiz.b.pktcnt = 1;
	hctsiz.b.pid = FH_OTG_HC_PID_DATA1;
	FH_WRITE_REG32(&hc_regs->hctsiz, hctsiz.d32);

	/* Set HCCHAR */
	hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);
	hcchar.b.eptype = FH_OTG_EP_TYPE_CONTROL;
	hcchar.b.epdir = 0;
	hcchar.b.epnum = 0;
	hcchar.b.mps = 8;
	hcchar.b.chen = 1;
	FH_WRITE_REG32(&hc_regs->hcchar, hcchar.d32);

	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);

	/* Wait for host channel interrupt */
	do {
		gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);
	} while (gintsts.b.hcintr == 0);

	/* Disable HCINTs */
	FH_WRITE_REG32(&hc_regs->hcintmsk, 0x0000);

	/* Disable HAINTs */
	FH_WRITE_REG32(&hc_global_regs->haintmsk, 0x0000);

	/* Read HAINT */
	haint.d32 = FH_READ_REG32(&hc_global_regs->haint);

	/* Read HCINT */
	hcint.d32 = FH_READ_REG32(&hc_regs->hcint);

	/* Read HCCHAR */
	hcchar.d32 = FH_READ_REG32(&hc_regs->hcchar);

	/* Clear HCINT */
	FH_WRITE_REG32(&hc_regs->hcint, hcint.d32);

	/* Clear HAINT */
	FH_WRITE_REG32(&hc_global_regs->haint, haint.d32);

	/* Clear GINTSTS */
	FH_WRITE_REG32(&global_regs->gintsts, gintsts.d32);

	/* Read GINTSTS */
	gintsts.d32 = FH_READ_REG32(&global_regs->gintsts);
}
#endif

/** Handles hub class-specific requests. */
int fh_otg_hcd_hub_control(fh_otg_hcd_t * fh_otg_hcd,
			    uint16_t typeReq,
			    uint16_t wValue,
			    uint16_t wIndex, uint8_t * buf, uint16_t wLength)
{
	int retval = 0;

	fh_otg_core_if_t *core_if = fh_otg_hcd->core_if;
	usb_hub_descriptor_t *hub_desc;
	hprt0_data_t hprt0 = {.d32 = 0 };

	uint32_t port_status;

	switch (typeReq) {
	case UCR_CLEAR_HUB_FEATURE:
		FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
			    "ClearHubFeature 0x%x\n", wValue);
		switch (wValue) {
		case UHF_C_HUB_LOCAL_POWER:
		case UHF_C_HUB_OVER_CURRENT:
			/* Nothing required here */
			break;
		default:
			retval = -FH_E_INVALID;
			FH_ERROR("FH OTG HCD - "
				  "ClearHubFeature request %xh unknown\n",
				  wValue);
		}
		break;
	case UCR_CLEAR_PORT_FEATURE:
#ifdef CONFIG_USB_FH_OTG_LPM
		if (wValue != UHF_PORT_L1)
#endif
			if (!wIndex || wIndex > 1)
				goto error;

		switch (wValue) {
		case UHF_PORT_ENABLE:
			FH_DEBUGPL(DBG_ANY, "FH OTG HCD HUB CONTROL - "
				    "ClearPortFeature USB_PORT_FEAT_ENABLE\n");
			hprt0.d32 = fh_otg_read_hprt0(core_if);
			hprt0.b.prtena = 1;
			FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
			break;
		case UHF_PORT_SUSPEND:
			FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
				    "ClearPortFeature USB_PORT_FEAT_SUSPEND\n");

			if (core_if->power_down == 2) {
				fh_otg_host_hibernation_restore(core_if, 0, 0);
			} else {
				FH_WRITE_REG32(core_if->pcgcctl, 0);
				fh_mdelay(5);

				hprt0.d32 = fh_otg_read_hprt0(core_if);
				hprt0.b.prtres = 1;
				FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
				hprt0.b.prtsusp = 0;
				/* Clear Resume bit */
				fh_mdelay(100);
				hprt0.b.prtres = 0;
				FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
			}
			break;
#ifdef CONFIG_USB_FH_OTG_LPM
		case UHF_PORT_L1:
			{
				pcgcctl_data_t pcgcctl = {.d32 = 0 };
				glpmcfg_data_t lpmcfg = {.d32 = 0 };

				lpmcfg.d32 =
				    FH_READ_REG32(&core_if->
						   core_global_regs->glpmcfg);
				lpmcfg.b.en_utmi_sleep = 0;
				lpmcfg.b.hird_thres &= (~(1 << 4));
				lpmcfg.b.prt_sleep_sts = 1;
				FH_WRITE_REG32(&core_if->
						core_global_regs->glpmcfg,
						lpmcfg.d32);

				/* Clear Enbl_L1Gating bit. */
				pcgcctl.b.enbl_sleep_gating = 1;
				FH_MODIFY_REG32(core_if->pcgcctl, pcgcctl.d32,
						 0);

				fh_mdelay(5);

				hprt0.d32 = fh_otg_read_hprt0(core_if);
				hprt0.b.prtres = 1;
				FH_WRITE_REG32(core_if->host_if->hprt0,
						hprt0.d32);
				/* This bit will be cleared in wakeup interrupt handle */
				break;
			}
#endif
		case UHF_PORT_POWER:
			FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
				    "ClearPortFeature USB_PORT_FEAT_POWER\n");
			hprt0.d32 = fh_otg_read_hprt0(core_if);
			hprt0.b.prtpwr = 0;
			FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
			break;
		case UHF_PORT_INDICATOR:
			FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
				    "ClearPortFeature USB_PORT_FEAT_INDICATOR\n");
			/* Port inidicator not supported */
			break;
		case UHF_C_PORT_CONNECTION:
			/* Clears drivers internal connect status change
			 * flag */
			FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
				    "ClearPortFeature USB_PORT_FEAT_C_CONNECTION\n");
			fh_otg_hcd->flags.b.port_connect_status_change = 0;
			break;
		case UHF_C_PORT_RESET:
			/* Clears the driver's internal Port Reset Change
			 * flag */
			FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
				    "ClearPortFeature USB_PORT_FEAT_C_RESET\n");
			fh_otg_hcd->flags.b.port_reset_change = 0;
			break;
		case UHF_C_PORT_ENABLE:
			/* Clears the driver's internal Port
			 * Enable/Disable Change flag */
			FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
				    "ClearPortFeature USB_PORT_FEAT_C_ENABLE\n");
			fh_otg_hcd->flags.b.port_enable_change = 0;
			break;
		case UHF_C_PORT_SUSPEND:
			/* Clears the driver's internal Port Suspend
			 * Change flag, which is set when resume signaling on
			 * the host port is complete */
			FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
				    "ClearPortFeature USB_PORT_FEAT_C_SUSPEND\n");
			fh_otg_hcd->flags.b.port_suspend_change = 0;
			break;
#ifdef CONFIG_USB_FH_OTG_LPM
		case UHF_C_PORT_L1:
			fh_otg_hcd->flags.b.port_l1_change = 0;
			break;
#endif
		case UHF_C_PORT_OVER_CURRENT:
			FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
				    "ClearPortFeature USB_PORT_FEAT_C_OVER_CURRENT\n");
			fh_otg_hcd->flags.b.port_over_current_change = 0;
			break;
		default:
			retval = -FH_E_INVALID;
			FH_ERROR("FH OTG HCD - "
				  "ClearPortFeature request %xh "
				  "unknown or unsupported\n", wValue);
		}
		break;
	case UCR_GET_HUB_DESCRIPTOR:
		FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
			    "GetHubDescriptor\n");
		hub_desc = (usb_hub_descriptor_t *) buf;
		hub_desc->bDescLength = 9;
		hub_desc->bDescriptorType = 0x29;
		hub_desc->bNbrPorts = 1;
		USETW(hub_desc->wHubCharacteristics, 0x08);
		hub_desc->bPwrOn2PwrGood = 1;
		hub_desc->bHubContrCurrent = 0;
		hub_desc->DeviceRemovable[0] = 0;
		hub_desc->DeviceRemovable[1] = 0xff;
		break;
	case UCR_GET_HUB_STATUS:
		FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
			    "GetHubStatus\n");
		FH_MEMSET(buf, 0, 4);
		break;
	case UCR_GET_PORT_STATUS:
		FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
			    "GetPortStatus wIndex = 0x%04x FLAGS=0x%08x\n",
			    wIndex, fh_otg_hcd->flags.d32);
		if (!wIndex || wIndex > 1)
			goto error;

		port_status = 0;

		if (fh_otg_hcd->flags.b.port_connect_status_change)
			port_status |= (1 << UHF_C_PORT_CONNECTION);

		if (fh_otg_hcd->flags.b.port_enable_change)
			port_status |= (1 << UHF_C_PORT_ENABLE);

		if (fh_otg_hcd->flags.b.port_suspend_change)
			port_status |= (1 << UHF_C_PORT_SUSPEND);

		if (fh_otg_hcd->flags.b.port_l1_change)
			port_status |= (1 << UHF_C_PORT_L1);

		if (fh_otg_hcd->flags.b.port_reset_change) {
			port_status |= (1 << UHF_C_PORT_RESET);
		}

		if (fh_otg_hcd->flags.b.port_over_current_change) {
			FH_WARN("Overcurrent change detected\n");
			port_status |= (1 << UHF_C_PORT_OVER_CURRENT);
		}

		if (!fh_otg_hcd->flags.b.port_connect_status) {
			/*
			 * The port is disconnected, which means the core is
			 * either in device mode or it soon will be. Just
			 * return 0's for the remainder of the port status
			 * since the port register can't be read if the core
			 * is in device mode.
			 */
			*((__le32 *) buf) = fh_cpu_to_le32(&port_status);
			break;
		}

		hprt0.d32 = FH_READ_REG32(core_if->host_if->hprt0);
		FH_DEBUGPL(DBG_HCDV, "  HPRT0: 0x%08x\n", hprt0.d32);

		if (hprt0.b.prtconnsts)
			port_status |= (1 << UHF_PORT_CONNECTION);

		if (hprt0.b.prtena)
			port_status |= (1 << UHF_PORT_ENABLE);

		if (hprt0.b.prtsusp)
			port_status |= (1 << UHF_PORT_SUSPEND);

		if (hprt0.b.prtovrcurract)
			port_status |= (1 << UHF_PORT_OVER_CURRENT);

		if (hprt0.b.prtrst)
			port_status |= (1 << UHF_PORT_RESET);

		if (hprt0.b.prtpwr)
			port_status |= (1 << UHF_PORT_POWER);

		if (hprt0.b.prtspd == FH_HPRT0_PRTSPD_HIGH_SPEED)
			port_status |= (1 << UHF_PORT_HIGH_SPEED);
		else if (hprt0.b.prtspd == FH_HPRT0_PRTSPD_LOW_SPEED)
			port_status |= (1 << UHF_PORT_LOW_SPEED);

		if (hprt0.b.prttstctl)
			port_status |= (1 << UHF_PORT_TEST);
		if (fh_otg_get_lpm_portsleepstatus(fh_otg_hcd->core_if)) {
			port_status |= (1 << UHF_PORT_L1);
		}
		/*
		   For Synopsys HW emulation of Power down wkup_control asserts the 
		   hreset_n and prst_n on suspned. This causes the HPRT0 to be zero. 
		   We intentionally tell the software that port is in L2Suspend state. 
		   Only for STE.
		*/
		if ((core_if->power_down == 2)
		    && (core_if->hibernation_suspend == 1)) {
			port_status |= (1 << UHF_PORT_SUSPEND);
		}
		/* USB_PORT_FEAT_INDICATOR unsupported always 0 */

		*((__le32 *) buf) = fh_cpu_to_le32(&port_status);

		break;
	case UCR_SET_HUB_FEATURE:
		FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
			    "SetHubFeature\n");
		/* No HUB features supported */
		break;
	case UCR_SET_PORT_FEATURE:
		if (wValue != UHF_PORT_TEST && (!wIndex || wIndex > 1))
			goto error;

		if (!fh_otg_hcd->flags.b.port_connect_status) {
			/*
			 * The port is disconnected, which means the core is
			 * either in device mode or it soon will be. Just
			 * return without doing anything since the port
			 * register can't be written if the core is in device
			 * mode.
			 */
			break;
		}

		switch (wValue) {
		case UHF_PORT_SUSPEND:
			FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
				    "SetPortFeature - USB_PORT_FEAT_SUSPEND\n");
			if (fh_otg_hcd_otg_port(fh_otg_hcd) != wIndex) {
				goto error;
			}
			if (core_if->power_down == 2) {
				int timeout = 300;
				fh_irqflags_t flags;
				pcgcctl_data_t pcgcctl = {.d32 = 0 };
				gpwrdn_data_t gpwrdn = {.d32 = 0 };
				gusbcfg_data_t gusbcfg = {.d32 = 0 };
#ifdef FH_DEV_SRPCAP
				int32_t otg_cap_param = core_if->core_params->otg_cap;
#endif
				FH_PRINTF("Preparing for complete power-off\n");

				/* Save registers before hibernation */
				fh_otg_save_global_regs(core_if);
				fh_otg_save_host_regs(core_if);

				hprt0.d32 = fh_otg_read_hprt0(core_if);
				hprt0.b.prtsusp = 1;
				hprt0.b.prtena = 0;
				FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
				/* Spin hprt0.b.prtsusp to became 1 */
				do {
					hprt0.d32 = fh_otg_read_hprt0(core_if);
					if (hprt0.b.prtsusp) {
						break;
					}
					fh_mdelay(1);
				} while (--timeout);
				if (!timeout) {
					FH_WARN("Suspend wasn't genereted\n");
				}
				fh_udelay(10);

				/*
				 * We need to disable interrupts to prevent servicing of any IRQ
				 * during going to hibernation
				 */
				FH_SPINLOCK_IRQSAVE(fh_otg_hcd->lock, &flags);
				core_if->lx_state = FH_OTG_L2;
#ifdef FH_DEV_SRPCAP
				hprt0.d32 = fh_otg_read_hprt0(core_if);
				hprt0.b.prtpwr = 0;
				hprt0.b.prtena = 0;
				FH_WRITE_REG32(core_if->host_if->hprt0,
						hprt0.d32);
#endif
				gusbcfg.d32 =
				    FH_READ_REG32(&core_if->core_global_regs->
						   gusbcfg);
				if (gusbcfg.b.ulpi_utmi_sel == 1) {
					/* ULPI interface */
					/* Suspend the Phy Clock */
					pcgcctl.d32 = 0;
					pcgcctl.b.stoppclk = 1;
					FH_MODIFY_REG32(core_if->pcgcctl, 0,
							 pcgcctl.d32);
					fh_udelay(10);
					gpwrdn.b.pmuactv = 1;
					FH_MODIFY_REG32(&core_if->
							 core_global_regs->
							 gpwrdn, 0, gpwrdn.d32);
				} else {
					/* UTMI+ Interface */
					gpwrdn.b.pmuactv = 1;
					FH_MODIFY_REG32(&core_if->
							 core_global_regs->
							 gpwrdn, 0, gpwrdn.d32);
					fh_udelay(10);
					pcgcctl.b.stoppclk = 1;
					FH_MODIFY_REG32(core_if->pcgcctl, 0, pcgcctl.d32);
					fh_udelay(10);
				}
#ifdef FH_DEV_SRPCAP				
				gpwrdn.d32 = 0;
				gpwrdn.b.dis_vbus = 1;
				FH_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);
#endif
				gpwrdn.d32 = 0;
				gpwrdn.b.pmuintsel = 1;
				FH_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);
				fh_udelay(10);

				gpwrdn.d32 = 0;
#ifdef FH_DEV_SRPCAP
				gpwrdn.b.srp_det_msk = 1;
#endif
				gpwrdn.b.disconn_det_msk = 1;
				gpwrdn.b.lnstchng_msk = 1;
				gpwrdn.b.sts_chngint_msk = 1;
				FH_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);
				fh_udelay(10);

				/* Enable Power Down Clamp and all interrupts in GPWRDN */
				gpwrdn.d32 = 0;
				gpwrdn.b.pwrdnclmp = 1;
				FH_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);
				fh_udelay(10);

				/* Switch off VDD */
				gpwrdn.d32 = 0;
				gpwrdn.b.pwrdnswtch = 1;
				FH_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);

#ifdef FH_DEV_SRPCAP
				if (otg_cap_param == FH_OTG_CAP_PARAM_HNP_SRP_CAPABLE)
				{
					core_if->pwron_timer_started = 1;
					FH_TIMER_SCHEDULE(core_if->pwron_timer, 6000 /* 6 secs */ );
				}
#endif
				/* Save gpwrdn register for further usage if stschng interrupt */
				core_if->gr_backup->gpwrdn_local =
						FH_READ_REG32(&core_if->core_global_regs->gpwrdn);

				/* Set flag to indicate that we are in hibernation */
				core_if->hibernation_suspend = 1;
				FH_SPINUNLOCK_IRQRESTORE(fh_otg_hcd->lock,flags);

				FH_PRINTF("Host hibernation completed\n");
				// Exit from case statement
				break;

			}
			if (fh_otg_hcd_otg_port(fh_otg_hcd) == wIndex &&
			    fh_otg_hcd->fops->get_b_hnp_enable(fh_otg_hcd)) {
				gotgctl_data_t gotgctl = {.d32 = 0 };
				gotgctl.b.hstsethnpen = 1;
				FH_MODIFY_REG32(&core_if->core_global_regs->
						 gotgctl, 0, gotgctl.d32);
				core_if->op_state = A_SUSPEND;
			}
			hprt0.d32 = fh_otg_read_hprt0(core_if);
			hprt0.b.prtsusp = 1;
			FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
			{
				fh_irqflags_t flags;
				/* Update lx_state */
				FH_SPINLOCK_IRQSAVE(fh_otg_hcd->lock, &flags);
				core_if->lx_state = FH_OTG_L2;
				FH_SPINUNLOCK_IRQRESTORE(fh_otg_hcd->lock, flags);
			}
			/* Suspend the Phy Clock */
			if (core_if->otg_ver == 0) {
				pcgcctl_data_t pcgcctl = {.d32 = 0 };
				pcgcctl.b.stoppclk = 1;
				FH_MODIFY_REG32(core_if->pcgcctl, 0,
						 pcgcctl.d32);
				fh_udelay(10);
			}

			/* For HNP the bus must be suspended for at least 200ms. */
			if (fh_otg_hcd->fops->get_b_hnp_enable(fh_otg_hcd)) {
				if (core_if->otg_ver) {
					pcgcctl_data_t pcgcctl = {.d32 = 0 };
					pcgcctl.b.stoppclk = 1;	
					FH_MODIFY_REG32(core_if->pcgcctl, pcgcctl.d32, 0);
				}
				fh_mdelay(200);
			}

			/** @todo - check how sw can wait for 1 sec to check asesvld??? */
#if 0 //vahrama !!!!!!!!!!!!!!!!!!
			if (core_if->adp_enable) {
				gotgctl_data_t gotgctl = {.d32 = 0 };
				gpwrdn_data_t gpwrdn;

				while (gotgctl.b.asesvld == 1) {
					gotgctl.d32 =
					    FH_READ_REG32(&core_if->
							   core_global_regs->
							   gotgctl);
					fh_mdelay(100);
				}

				/* Enable Power Down Logic */
				gpwrdn.d32 = 0;
				gpwrdn.b.pmuactv = 1;
				FH_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);

				/* Unmask SRP detected interrupt from Power Down Logic */
				gpwrdn.d32 = 0;
				gpwrdn.b.srp_det_msk = 1;
				FH_MODIFY_REG32(&core_if->core_global_regs->
						 gpwrdn, 0, gpwrdn.d32);

				fh_otg_adp_probe_start(core_if);
			}
#endif
			break;
		case UHF_PORT_POWER:
			FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
				    "SetPortFeature - USB_PORT_FEAT_POWER\n");
			hprt0.d32 = fh_otg_read_hprt0(core_if);
			hprt0.b.prtpwr = 1;
			FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
			break;
		case UHF_PORT_RESET:
			if ((core_if->power_down == 2)
			    && (core_if->hibernation_suspend == 1)) {
				/* If we are going to exit from Hibernated
				 * state via USB RESET.
				 */
				fh_otg_host_hibernation_restore(core_if, 0, 1);
			} else {
				hprt0.d32 = fh_otg_read_hprt0(core_if);

				FH_DEBUGPL(DBG_HCD,
					    "FH OTG HCD HUB CONTROL - "
					    "SetPortFeature - USB_PORT_FEAT_RESET\n");
				{
					pcgcctl_data_t pcgcctl = {.d32 = 0 };
					pcgcctl.b.enbl_sleep_gating = 1;
					pcgcctl.b.stoppclk = 1;
					FH_MODIFY_REG32(core_if->pcgcctl, pcgcctl.d32, 0);
					FH_WRITE_REG32(core_if->pcgcctl, 0);
				}
#ifdef CONFIG_USB_FH_OTG_LPM
				{
					glpmcfg_data_t lpmcfg;
					lpmcfg.d32 =
						FH_READ_REG32(&core_if->core_global_regs->glpmcfg);
					if (lpmcfg.b.prt_sleep_sts) {
						lpmcfg.b.en_utmi_sleep = 0;
						lpmcfg.b.hird_thres &= (~(1 << 4));
						FH_WRITE_REG32
						    (&core_if->core_global_regs->glpmcfg,
						     lpmcfg.d32);
						fh_mdelay(1);
					}
				}
#endif
				hprt0.d32 = fh_otg_read_hprt0(core_if);
				/* Clear suspend bit if resetting from suspended state. */
				hprt0.b.prtsusp = 0;
				/* When B-Host the Port reset bit is set in
				 * the Start HCD Callback function, so that
				 * the reset is started within 1ms of the HNP
				 * success interrupt. */
				if (!fh_otg_hcd_is_b_host(fh_otg_hcd)) {
					hprt0.b.prtpwr = 1;
					hprt0.b.prtrst = 1;
					FH_PRINTF("Indeed it is in host mode hprt0 = %08x\n",hprt0.d32);
					FH_WRITE_REG32(core_if->host_if->hprt0,
							hprt0.d32);
				}
				/* Clear reset bit in 10ms (FS/LS) or 50ms (HS) */
				fh_mdelay(60);
				hprt0.b.prtrst = 0;
				FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
				core_if->lx_state = FH_OTG_L0;	/* Now back to the on state */
			}
			break;
#ifdef FH_HS_ELECT_TST
		case UHF_PORT_TEST:
			{
				uint32_t t;
				gintmsk_data_t gintmsk;

				t = (wIndex >> 8);	/* MSB wIndex USB */
				FH_DEBUGPL(DBG_HCD,
					    "FH OTG HCD HUB CONTROL - "
					    "SetPortFeature - USB_PORT_FEAT_TEST %d\n",
					    t);
				FH_WARN("USB_PORT_FEAT_TEST %d\n", t);
				if (t < 6) {
					hprt0.d32 = fh_otg_read_hprt0(core_if);
					hprt0.b.prttstctl = t;
					FH_WRITE_REG32(core_if->host_if->hprt0,
							hprt0.d32);
				} else {
					/* Setup global vars with reg addresses (quick and
					 * dirty hack, should be cleaned up)
					 */
					global_regs = core_if->core_global_regs;
					hc_global_regs =
					    core_if->host_if->host_global_regs;
					hc_regs =
					    (fh_otg_hc_regs_t *) ((char *)
								   global_regs +
								   0x500);
					data_fifo =
					    (uint32_t *) ((char *)global_regs +
							  0x1000);

					if (t == 6) {	/* HS_HOST_PORT_SUSPEND_RESUME */
						/* Save current interrupt mask */
						gintmsk.d32 =
						    FH_READ_REG32
						    (&global_regs->gintmsk);

						/* Disable all interrupts while we muck with
						 * the hardware directly
						 */
						FH_WRITE_REG32(&global_regs->gintmsk, 0);

						/* 15 second delay per the test spec */
						fh_mdelay(15000);

						/* Drive suspend on the root port */
						hprt0.d32 =
						    fh_otg_read_hprt0(core_if);
						hprt0.b.prtsusp = 1;
						hprt0.b.prtres = 0;
						FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);

						/* 15 second delay per the test spec */
						fh_mdelay(15000);

						/* Drive resume on the root port */
						hprt0.d32 =
						    fh_otg_read_hprt0(core_if);
						hprt0.b.prtsusp = 0;
						hprt0.b.prtres = 1;
						FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);
						fh_mdelay(100);

						/* Clear the resume bit */
						hprt0.b.prtres = 0;
						FH_WRITE_REG32(core_if->host_if->hprt0, hprt0.d32);

						/* Restore interrupts */
						FH_WRITE_REG32(&global_regs->gintmsk, gintmsk.d32);
					} else if (t == 7) {	/* SINGLE_STEP_GET_DEVICE_DESCRIPTOR setup */
						/* Save current interrupt mask */
						gintmsk.d32 =
						    FH_READ_REG32
						    (&global_regs->gintmsk);

						/* Disable all interrupts while we muck with
						 * the hardware directly
						 */
						FH_WRITE_REG32(&global_regs->gintmsk, 0);

						/* 15 second delay per the test spec */
						fh_mdelay(15000);

						/* Send the Setup packet */
						do_setup();

						/* 15 second delay so nothing else happens for awhile */
						fh_mdelay(15000);

						/* Restore interrupts */
						FH_WRITE_REG32(&global_regs->gintmsk, gintmsk.d32);
					} else if (t == 8) {	/* SINGLE_STEP_GET_DEVICE_DESCRIPTOR execute */
						/* Save current interrupt mask */
						gintmsk.d32 =
						    FH_READ_REG32
						    (&global_regs->gintmsk);

						/* Disable all interrupts while we muck with
						 * the hardware directly
						 */
						FH_WRITE_REG32(&global_regs->gintmsk, 0);

						/* Send the Setup packet */
						do_setup();

						/* 15 second delay so nothing else happens for awhile */
						fh_mdelay(15000);

						/* Send the In and Ack packets */
						do_in_ack();

						/* 15 second delay so nothing else happens for awhile */
						fh_mdelay(15000);

						/* Restore interrupts */
						FH_WRITE_REG32(&global_regs->gintmsk, gintmsk.d32);
					}
				}
				break;
			}
#endif /* FH_HS_ELECT_TST */

		case UHF_PORT_INDICATOR:
			FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB CONTROL - "
				    "SetPortFeature - USB_PORT_FEAT_INDICATOR\n");
			/* Not supported */
			break;
		default:
			retval = -FH_E_INVALID;
			FH_ERROR("FH OTG HCD - "
				  "SetPortFeature request %xh "
				  "unknown or unsupported\n", wValue);
			break;
		}
		break;
#ifdef CONFIG_USB_FH_OTG_LPM
	case UCR_SET_AND_TEST_PORT_FEATURE:
		if (wValue != UHF_PORT_L1) {
			goto error;
		}
		{
			int portnum, hird, devaddr, remwake;
			glpmcfg_data_t lpmcfg;
			uint32_t time_usecs;
			gintsts_data_t gintsts;
			gintmsk_data_t gintmsk;

			if (!fh_otg_get_param_lpm_enable(core_if)) {
				goto error;
			}
			if (wValue != UHF_PORT_L1 || wLength != 1) {
				goto error;
			}
			/* Check if the port currently is in SLEEP state */
			lpmcfg.d32 =
			    FH_READ_REG32(&core_if->core_global_regs->glpmcfg);
			if (lpmcfg.b.prt_sleep_sts) {
				FH_INFO("Port is already in sleep mode\n");
				buf[0] = 0;	/* Return success */
				break;
			}

			portnum = wIndex & 0xf;
			hird = (wIndex >> 4) & 0xf;
			devaddr = (wIndex >> 8) & 0x7f;
			remwake = (wIndex >> 15);

			if (portnum != 1) {
				retval = -FH_E_INVALID;
				FH_WARN
				    ("Wrong port number(%d) in SetandTestPortFeature request\n",
				     portnum);
				break;
			}

			FH_PRINTF
			    ("SetandTestPortFeature request: portnum = %d, hird = %d, devaddr = %d, rewake = %d\n",
			     portnum, hird, devaddr, remwake);
			/* Disable LPM interrupt */
			gintmsk.d32 = 0;
			gintmsk.b.lpmtranrcvd = 1;
			FH_MODIFY_REG32(&core_if->core_global_regs->gintmsk,
					 gintmsk.d32, 0);

			if (fh_otg_hcd_send_lpm
			    (fh_otg_hcd, devaddr, hird, remwake)) {
				retval = -FH_E_INVALID;
				break;
			}

			time_usecs = 10 * (lpmcfg.b.retry_count + 1);
			/* We will consider timeout if time_usecs microseconds pass,
			 * and we don't receive LPM transaction status.
			 * After receiving non-error responce(ACK/NYET/STALL) from device,
			 *  core will set lpmtranrcvd bit.
			 */
			do {
				gintsts.d32 =
				    FH_READ_REG32(&core_if->core_global_regs->gintsts);
				if (gintsts.b.lpmtranrcvd) {
					break;
				}
				fh_udelay(1);
			} while (--time_usecs);
			/* lpm_int bit will be cleared in LPM interrupt handler */

			/* Now fill status
			 * 0x00 - Success
			 * 0x10 - NYET
			 * 0x11 - Timeout
			 */
			if (!gintsts.b.lpmtranrcvd) {
				buf[0] = 0x3;	/* Completion code is Timeout */
				fh_otg_hcd_free_hc_from_lpm(fh_otg_hcd);
			} else {
				lpmcfg.d32 =
				    FH_READ_REG32(&core_if->core_global_regs->glpmcfg);
				if (lpmcfg.b.lpm_resp == 0x3) {
					/* ACK responce from the device */
					buf[0] = 0x00;	/* Success */
				} else if (lpmcfg.b.lpm_resp == 0x2) {
					/* NYET responce from the device */
					buf[0] = 0x2;
				} else {
					/* Otherwise responce with Timeout */
					buf[0] = 0x3;
				}
			}
			FH_PRINTF("Device responce to LPM trans is %x\n",
				   lpmcfg.b.lpm_resp);
			FH_MODIFY_REG32(&core_if->core_global_regs->gintmsk, 0,
					 gintmsk.d32);

			break;
		}
#endif /* CONFIG_USB_FH_OTG_LPM */
	default:
error:
		retval = -FH_E_INVALID;
		FH_WARN("FH OTG HCD - "
			 "Unknown hub control request type or invalid typeReq: %xh wIndex: %xh wValue: %xh\n",
			 typeReq, wIndex, wValue);
		break;
	}

	return retval;
}

#ifdef CONFIG_USB_FH_OTG_LPM
/** Returns index of host channel to perform LPM transaction. */
int fh_otg_hcd_get_hc_for_lpm_tran(fh_otg_hcd_t * hcd, uint8_t devaddr)
{
	fh_otg_core_if_t *core_if = hcd->core_if;
	fh_hc_t *hc;
	hcchar_data_t hcchar;
	gintmsk_data_t gintmsk = {.d32 = 0 };

	if (FH_CIRCLEQ_EMPTY(&hcd->free_hc_list)) {
		FH_PRINTF("No free channel to select for LPM transaction\n");
		return -1;
	}

	hc = FH_CIRCLEQ_FIRST(&hcd->free_hc_list);

	/* Mask host channel interrupts. */
	gintmsk.b.hcintr = 1;
	FH_MODIFY_REG32(&core_if->core_global_regs->gintmsk, gintmsk.d32, 0);

	/* Fill fields that core needs for LPM transaction */
	hcchar.b.devaddr = devaddr;
	hcchar.b.epnum = 0;
	hcchar.b.eptype = FH_OTG_EP_TYPE_CONTROL;
	hcchar.b.mps = 64;
	hcchar.b.lspddev = (hc->speed == FH_OTG_EP_SPEED_LOW);
	hcchar.b.epdir = 0;	/* OUT */
	FH_WRITE_REG32(&core_if->host_if->hc_regs[hc->hc_num]->hcchar,
			hcchar.d32);

	/* Remove the host channel from the free list. */
	FH_CIRCLEQ_REMOVE_INIT(&hcd->free_hc_list, hc, hc_list_entry);

	FH_PRINTF("hcnum = %d devaddr = %d\n", hc->hc_num, devaddr);

	return hc->hc_num;
}

/** Release hc after performing LPM transaction */
void fh_otg_hcd_free_hc_from_lpm(fh_otg_hcd_t * hcd)
{
	fh_hc_t *hc;
	glpmcfg_data_t lpmcfg;
	uint8_t hc_num;

	lpmcfg.d32 = FH_READ_REG32(&hcd->core_if->core_global_regs->glpmcfg);
	hc_num = lpmcfg.b.lpm_chan_index;

	hc = hcd->hc_ptr_array[hc_num];

	FH_PRINTF("Freeing channel %d after LPM\n", hc_num);
	/* Return host channel to free list */
	FH_CIRCLEQ_INSERT_TAIL(&hcd->free_hc_list, hc, hc_list_entry);
}

int fh_otg_hcd_send_lpm(fh_otg_hcd_t * hcd, uint8_t devaddr, uint8_t hird,
			 uint8_t bRemoteWake)
{
	glpmcfg_data_t lpmcfg;
	pcgcctl_data_t pcgcctl = {.d32 = 0 };
	int channel;

	channel = fh_otg_hcd_get_hc_for_lpm_tran(hcd, devaddr);
	if (channel < 0) {
		return channel;
	}

	pcgcctl.b.enbl_sleep_gating = 1;
	FH_MODIFY_REG32(hcd->core_if->pcgcctl, 0, pcgcctl.d32);

	/* Read LPM config register */
	lpmcfg.d32 = FH_READ_REG32(&hcd->core_if->core_global_regs->glpmcfg);

	/* Program LPM transaction fields */
	lpmcfg.b.rem_wkup_en = bRemoteWake;
	lpmcfg.b.hird = hird;
	
	if(fh_otg_get_param_besl_enable(hcd->core_if)) {
		lpmcfg.b.hird_thres = 0x16;
		lpmcfg.b.en_besl = 1;
	} else {
		lpmcfg.b.hird_thres = 0x1c;
	}
	
	lpmcfg.b.lpm_chan_index = channel;
	lpmcfg.b.en_utmi_sleep = 1;
	/* Program LPM config register */
	FH_WRITE_REG32(&hcd->core_if->core_global_regs->glpmcfg, lpmcfg.d32);

	/* Send LPM transaction */
	lpmcfg.b.send_lpm = 1;
	FH_WRITE_REG32(&hcd->core_if->core_global_regs->glpmcfg, lpmcfg.d32);

	return 0;
}

#endif /* CONFIG_USB_FH_OTG_LPM */

int fh_otg_hcd_is_status_changed(fh_otg_hcd_t * hcd, int port)
{
	int retval;

	if (port != 1) {
		return -FH_E_INVALID;
	}

	retval = (hcd->flags.b.port_connect_status_change ||
		  hcd->flags.b.port_reset_change ||
		  hcd->flags.b.port_enable_change ||
		  hcd->flags.b.port_suspend_change ||
		  hcd->flags.b.port_over_current_change);
#ifdef DEBUG
	if (retval) {
		FH_DEBUGPL(DBG_HCD, "FH OTG HCD HUB STATUS DATA:"
			    " Root port status changed\n");
		FH_DEBUGPL(DBG_HCDV, "  port_connect_status_change: %d\n",
			    hcd->flags.b.port_connect_status_change);
		FH_DEBUGPL(DBG_HCDV, "  port_reset_change: %d\n",
			    hcd->flags.b.port_reset_change);
		FH_DEBUGPL(DBG_HCDV, "  port_enable_change: %d\n",
			    hcd->flags.b.port_enable_change);
		FH_DEBUGPL(DBG_HCDV, "  port_suspend_change: %d\n",
			    hcd->flags.b.port_suspend_change);
		FH_DEBUGPL(DBG_HCDV, "  port_over_current_change: %d\n",
			    hcd->flags.b.port_over_current_change);
	}
#endif
	return retval;
}

int fh_otg_hcd_get_frame_number(fh_otg_hcd_t * fh_otg_hcd)
{
	hfnum_data_t hfnum;
	hfnum.d32 =
	    FH_READ_REG32(&fh_otg_hcd->core_if->host_if->host_global_regs->
			   hfnum);

#ifdef DEBUG_SOF
	FH_DEBUGPL(DBG_HCDV, "FH OTG HCD GET FRAME NUMBER %d\n",
		    hfnum.b.frnum);
#endif
	return hfnum.b.frnum;
}

int fh_otg_hcd_start(fh_otg_hcd_t * hcd,
		      struct fh_otg_hcd_function_ops *fops)
{
	int retval = 0;
	hprt0_data_t hprt0;

	hcd->fops = fops;
	if (!fh_otg_is_device_mode(hcd->core_if) && 
		(!hcd->core_if->adp_enable || hcd->core_if->adp.adp_started)) {
		fh_otg_hcd_reinit(hcd);
	} else {
		if (hcd->core_if->adp_enable) {
			/* Clear any interrupt pending in the HPRT, sometimes 
			 * Port Connect Detected is not being cleared*/
			hprt0.d32 = FH_READ_REG32(hcd->core_if->host_if->hprt0);
			FH_WRITE_REG32(hcd->core_if->host_if->hprt0, hprt0.d32);
		}
		retval = -FH_E_NO_DEVICE;
	}

	return retval;
}

void *fh_otg_hcd_get_priv_data(fh_otg_hcd_t * hcd)
{
	return hcd->priv;
}

void fh_otg_hcd_set_priv_data(fh_otg_hcd_t * hcd, void *priv_data)
{
	hcd->priv = priv_data;
}

uint32_t fh_otg_hcd_otg_port(fh_otg_hcd_t * hcd)
{
	return hcd->otg_port;
}

uint32_t fh_otg_hcd_is_b_host(fh_otg_hcd_t * hcd)
{
	uint32_t is_b_host;
	if (hcd->core_if->op_state == B_HOST) {
		is_b_host = 1;
	} else {
		is_b_host = 0;
	}

	return is_b_host;
}

fh_otg_hcd_urb_t *fh_otg_hcd_urb_alloc(fh_otg_hcd_t * hcd,
					 int iso_desc_count, int atomic_alloc)
{
	fh_otg_hcd_urb_t *fh_otg_urb;
	uint32_t size;

	size =
	    sizeof(*fh_otg_urb) +
	    iso_desc_count * sizeof(struct fh_otg_hcd_iso_packet_desc);
	if (atomic_alloc)
		fh_otg_urb = FH_ALLOC_ATOMIC(size);
	else
		fh_otg_urb = FH_ALLOC(size);

	fh_otg_urb->packet_count = iso_desc_count;

	return fh_otg_urb;
}

void fh_otg_hcd_urb_set_pipeinfo(fh_otg_hcd_urb_t * fh_otg_urb,
				  uint8_t dev_addr, uint8_t ep_num,
				  uint8_t ep_type, uint8_t ep_dir, uint16_t mps)
{
	fh_otg_hcd_fill_pipe(&fh_otg_urb->pipe_info, dev_addr, ep_num,
			      ep_type, ep_dir, mps);
#if 0
	FH_PRINTF
	    ("addr = %d, ep_num = %d, ep_dir = 0x%x, ep_type = 0x%x, mps = %d\n",
	     dev_addr, ep_num, ep_dir, ep_type, mps);
#endif
}

void fh_otg_hcd_urb_set_params(fh_otg_hcd_urb_t * fh_otg_urb,
				void *urb_handle, void *buf, fh_dma_t dma,
				uint32_t buflen, void *setup_packet,
				fh_dma_t setup_dma, uint32_t flags,
				uint16_t interval)
{
	fh_otg_urb->priv = urb_handle;
	fh_otg_urb->buf = buf;
	fh_otg_urb->dma = dma;
	fh_otg_urb->length = buflen;
	fh_otg_urb->setup_packet = setup_packet;
	fh_otg_urb->setup_dma = setup_dma;
	fh_otg_urb->flags = flags;
	fh_otg_urb->interval = interval;
	fh_otg_urb->status = -FH_E_IN_PROGRESS;
}

uint32_t fh_otg_hcd_urb_get_status(fh_otg_hcd_urb_t * fh_otg_urb)
{
	return fh_otg_urb->status;
}

uint32_t fh_otg_hcd_urb_get_actual_length(fh_otg_hcd_urb_t * fh_otg_urb)
{
	return fh_otg_urb->actual_length;
}

uint32_t fh_otg_hcd_urb_get_error_count(fh_otg_hcd_urb_t * fh_otg_urb)
{
	return fh_otg_urb->error_count;
}

void fh_otg_hcd_urb_set_iso_desc_params(fh_otg_hcd_urb_t * fh_otg_urb,
					 int desc_num, uint32_t offset,
					 uint32_t length)
{
	fh_otg_urb->iso_descs[desc_num].offset = offset;
	fh_otg_urb->iso_descs[desc_num].length = length;
}

uint32_t fh_otg_hcd_urb_get_iso_desc_status(fh_otg_hcd_urb_t * fh_otg_urb,
					     int desc_num)
{
	return fh_otg_urb->iso_descs[desc_num].status;
}

uint32_t fh_otg_hcd_urb_get_iso_desc_actual_length(fh_otg_hcd_urb_t *
						    fh_otg_urb, int desc_num)
{
	return fh_otg_urb->iso_descs[desc_num].actual_length;
}

int fh_otg_hcd_is_bandwidth_allocated(fh_otg_hcd_t * hcd, void *ep_handle)
{
	int allocated = 0;
	fh_otg_qh_t *qh = (fh_otg_qh_t *) ep_handle;

	if (qh) {
		if (!FH_LIST_EMPTY(&qh->qh_list_entry)) {
			allocated = 1;
		}
	}
	return allocated;
}

int fh_otg_hcd_is_bandwidth_freed(fh_otg_hcd_t * hcd, void *ep_handle)
{
	fh_otg_qh_t *qh = (fh_otg_qh_t *) ep_handle;
	int freed = 0;
	FH_ASSERT(qh, "qh is not allocated\n");

	if (FH_LIST_EMPTY(&qh->qh_list_entry)) {
		freed = 1;
	}

	return freed;
}

uint8_t fh_otg_hcd_get_ep_bandwidth(fh_otg_hcd_t * hcd, void *ep_handle)
{
	fh_otg_qh_t *qh = (fh_otg_qh_t *) ep_handle;
	FH_ASSERT(qh, "qh is not allocated\n");
	return qh->usecs;
}

void fh_otg_hcd_dump_state(fh_otg_hcd_t * hcd)
{
#ifdef DEBUG
	int num_channels;
	int i;
	gnptxsts_data_t np_tx_status;
	hptxsts_data_t p_tx_status;

	num_channels = hcd->core_if->core_params->host_channels;
	FH_PRINTF("\n");
	FH_PRINTF
	    ("************************************************************\n");
	FH_PRINTF("HCD State:\n");
	FH_PRINTF("  Num channels: %d\n", num_channels);
	for (i = 0; i < num_channels; i++) {
		fh_hc_t *hc = hcd->hc_ptr_array[i];
		FH_PRINTF("  Channel %d:\n", i);
		FH_PRINTF("    dev_addr: %d, ep_num: %d, ep_is_in: %d\n",
			   hc->dev_addr, hc->ep_num, hc->ep_is_in);
		FH_PRINTF("    speed: %d\n", hc->speed);
		FH_PRINTF("    ep_type: %d\n", hc->ep_type);
		FH_PRINTF("    max_packet: %d\n", hc->max_packet);
		FH_PRINTF("    data_pid_start: %d\n", hc->data_pid_start);
		FH_PRINTF("    multi_count: %d\n", hc->multi_count);
		FH_PRINTF("    xfer_started: %d\n", hc->xfer_started);
		FH_PRINTF("    xfer_buff: %p\n", hc->xfer_buff);
		FH_PRINTF("    xfer_len: %d\n", hc->xfer_len);
		FH_PRINTF("    xfer_count: %d\n", hc->xfer_count);
		FH_PRINTF("    halt_on_queue: %d\n", hc->halt_on_queue);
		FH_PRINTF("    halt_pending: %d\n", hc->halt_pending);
		FH_PRINTF("    halt_status: %d\n", hc->halt_status);
		FH_PRINTF("    do_split: %d\n", hc->do_split);
		FH_PRINTF("    complete_split: %d\n", hc->complete_split);
		FH_PRINTF("    hub_addr: %d\n", hc->hub_addr);
		FH_PRINTF("    port_addr: %d\n", hc->port_addr);
		FH_PRINTF("    xact_pos: %d\n", hc->xact_pos);
		FH_PRINTF("    requests: %d\n", hc->requests);
		FH_PRINTF("    qh: %p\n", hc->qh);
		if (hc->xfer_started) {
			hfnum_data_t hfnum;
			hcchar_data_t hcchar;
			hctsiz_data_t hctsiz;
			hcint_data_t hcint;
			hcintmsk_data_t hcintmsk;
			hfnum.d32 =
			    FH_READ_REG32(&hcd->core_if->
					   host_if->host_global_regs->hfnum);
			hcchar.d32 =
			    FH_READ_REG32(&hcd->core_if->host_if->
					   hc_regs[i]->hcchar);
			hctsiz.d32 =
			    FH_READ_REG32(&hcd->core_if->host_if->
					   hc_regs[i]->hctsiz);
			hcint.d32 =
			    FH_READ_REG32(&hcd->core_if->host_if->
					   hc_regs[i]->hcint);
			hcintmsk.d32 =
			    FH_READ_REG32(&hcd->core_if->host_if->
					   hc_regs[i]->hcintmsk);
			FH_PRINTF("    hfnum: 0x%08x\n", hfnum.d32);
			FH_PRINTF("    hcchar: 0x%08x\n", hcchar.d32);
			FH_PRINTF("    hctsiz: 0x%08x\n", hctsiz.d32);
			FH_PRINTF("    hcint: 0x%08x\n", hcint.d32);
			FH_PRINTF("    hcintmsk: 0x%08x\n", hcintmsk.d32);
		}
		if (hc->xfer_started && hc->qh) {
			fh_otg_qtd_t *qtd;
			fh_otg_hcd_urb_t *urb;
			
			FH_CIRCLEQ_FOREACH(qtd, &hc->qh->qtd_list, qtd_list_entry) {
				if (!qtd->in_process)
					break;
				
				urb = qtd->urb;
			FH_PRINTF("    URB Info:\n");
			FH_PRINTF("      qtd: %p, urb: %p\n", qtd, urb);
			if (urb) {
				FH_PRINTF("      Dev: %d, EP: %d %s\n",
					   fh_otg_hcd_get_dev_addr(&urb->
								    pipe_info),
					   fh_otg_hcd_get_ep_num(&urb->
								  pipe_info),
					   fh_otg_hcd_is_pipe_in(&urb->
								  pipe_info) ?
					   "IN" : "OUT");
				FH_PRINTF("      Max packet size: %d\n",
					   fh_otg_hcd_get_mps(&urb->
							       pipe_info));
				FH_PRINTF("      transfer_buffer: %p\n",
					   urb->buf);
				FH_PRINTF("      transfer_dma: %p\n",
					   (void *)urb->dma);
				FH_PRINTF("      transfer_buffer_length: %d\n",
					   urb->length);
					FH_PRINTF("      actual_length: %d\n",
						   urb->actual_length);
				}
			}
		}
	}
	FH_PRINTF("  non_periodic_channels: %d\n", hcd->non_periodic_channels);
	FH_PRINTF("  periodic_channels: %d\n", hcd->periodic_channels);
	FH_PRINTF("  periodic_usecs: %d\n", hcd->periodic_usecs);
	np_tx_status.d32 =
	    FH_READ_REG32(&hcd->core_if->core_global_regs->gnptxsts);
	FH_PRINTF("  NP Tx Req Queue Space Avail: %d\n",
		   np_tx_status.b.nptxqspcavail);
	FH_PRINTF("  NP Tx FIFO Space Avail: %d\n",
		   np_tx_status.b.nptxfspcavail);
	p_tx_status.d32 =
	    FH_READ_REG32(&hcd->core_if->host_if->host_global_regs->hptxsts);
	FH_PRINTF("  P Tx Req Queue Space Avail: %d\n",
		   p_tx_status.b.ptxqspcavail);
	FH_PRINTF("  P Tx FIFO Space Avail: %d\n", p_tx_status.b.ptxfspcavail);
	fh_otg_hcd_dump_frrem(hcd);
	fh_otg_dump_global_registers(hcd->core_if);
	fh_otg_dump_host_registers(hcd->core_if);
	FH_PRINTF
	    ("************************************************************\n");
	FH_PRINTF("\n");
#endif
}

#ifdef DEBUG
void fh_print_setup_data(uint8_t * setup)
{
	int i;
	if (CHK_DEBUG_LEVEL(DBG_HCD)) {
		FH_PRINTF("Setup Data = MSB ");
		for (i = 7; i >= 0; i--)
			FH_PRINTF("%02x ", setup[i]);
		FH_PRINTF("\n");
		FH_PRINTF("  bmRequestType Tranfer = %s\n",
			   (setup[0] & 0x80) ? "Device-to-Host" :
			   "Host-to-Device");
		FH_PRINTF("  bmRequestType Type = ");
		switch ((setup[0] & 0x60) >> 5) {
		case 0:
			FH_PRINTF("Standard\n");
			break;
		case 1:
			FH_PRINTF("Class\n");
			break;
		case 2:
			FH_PRINTF("Vendor\n");
			break;
		case 3:
			FH_PRINTF("Reserved\n");
			break;
		}
		FH_PRINTF("  bmRequestType Recipient = ");
		switch (setup[0] & 0x1f) {
		case 0:
			FH_PRINTF("Device\n");
			break;
		case 1:
			FH_PRINTF("Interface\n");
			break;
		case 2:
			FH_PRINTF("Endpoint\n");
			break;
		case 3:
			FH_PRINTF("Other\n");
			break;
		default:
			FH_PRINTF("Reserved\n");
			break;
		}
		FH_PRINTF("  bRequest = 0x%0x\n", setup[1]);
		FH_PRINTF("  wValue = 0x%0x\n", *((uint16_t *) & setup[2]));
		FH_PRINTF("  wIndex = 0x%0x\n", *((uint16_t *) & setup[4]));
		FH_PRINTF("  wLength = 0x%0x\n\n", *((uint16_t *) & setup[6]));
	}
}
#endif

void fh_otg_hcd_dump_frrem(fh_otg_hcd_t * hcd)
{
#if 0
	FH_PRINTF("Frame remaining at SOF:\n");
	FH_PRINTF("  samples %u, accum %llu, avg %llu\n",
		   hcd->frrem_samples, hcd->frrem_accum,
		   (hcd->frrem_samples > 0) ?
		   hcd->frrem_accum / hcd->frrem_samples : 0);

	FH_PRINTF("\n");
	FH_PRINTF("Frame remaining at start_transfer (uframe 7):\n");
	FH_PRINTF("  samples %u, accum %llu, avg %llu\n",
		   hcd->core_if->hfnum_7_samples,
		   hcd->core_if->hfnum_7_frrem_accum,
		   (hcd->core_if->hfnum_7_samples >
		    0) ? hcd->core_if->hfnum_7_frrem_accum /
		   hcd->core_if->hfnum_7_samples : 0);
	FH_PRINTF("Frame remaining at start_transfer (uframe 0):\n");
	FH_PRINTF("  samples %u, accum %llu, avg %llu\n",
		   hcd->core_if->hfnum_0_samples,
		   hcd->core_if->hfnum_0_frrem_accum,
		   (hcd->core_if->hfnum_0_samples >
		    0) ? hcd->core_if->hfnum_0_frrem_accum /
		   hcd->core_if->hfnum_0_samples : 0);
	FH_PRINTF("Frame remaining at start_transfer (uframe 1-6):\n");
	FH_PRINTF("  samples %u, accum %llu, avg %llu\n",
		   hcd->core_if->hfnum_other_samples,
		   hcd->core_if->hfnum_other_frrem_accum,
		   (hcd->core_if->hfnum_other_samples >
		    0) ? hcd->core_if->hfnum_other_frrem_accum /
		   hcd->core_if->hfnum_other_samples : 0);

	FH_PRINTF("\n");
	FH_PRINTF("Frame remaining at sample point A (uframe 7):\n");
	FH_PRINTF("  samples %u, accum %llu, avg %llu\n",
		   hcd->hfnum_7_samples_a, hcd->hfnum_7_frrem_accum_a,
		   (hcd->hfnum_7_samples_a > 0) ?
		   hcd->hfnum_7_frrem_accum_a / hcd->hfnum_7_samples_a : 0);
	FH_PRINTF("Frame remaining at sample point A (uframe 0):\n");
	FH_PRINTF("  samples %u, accum %llu, avg %llu\n",
		   hcd->hfnum_0_samples_a, hcd->hfnum_0_frrem_accum_a,
		   (hcd->hfnum_0_samples_a > 0) ?
		   hcd->hfnum_0_frrem_accum_a / hcd->hfnum_0_samples_a : 0);
	FH_PRINTF("Frame remaining at sample point A (uframe 1-6):\n");
	FH_PRINTF("  samples %u, accum %llu, avg %llu\n",
		   hcd->hfnum_other_samples_a, hcd->hfnum_other_frrem_accum_a,
		   (hcd->hfnum_other_samples_a > 0) ?
		   hcd->hfnum_other_frrem_accum_a /
		   hcd->hfnum_other_samples_a : 0);

	FH_PRINTF("\n");
	FH_PRINTF("Frame remaining at sample point B (uframe 7):\n");
	FH_PRINTF("  samples %u, accum %llu, avg %llu\n",
		   hcd->hfnum_7_samples_b, hcd->hfnum_7_frrem_accum_b,
		   (hcd->hfnum_7_samples_b > 0) ?
		   hcd->hfnum_7_frrem_accum_b / hcd->hfnum_7_samples_b : 0);
	FH_PRINTF("Frame remaining at sample point B (uframe 0):\n");
	FH_PRINTF("  samples %u, accum %llu, avg %llu\n",
		   hcd->hfnum_0_samples_b, hcd->hfnum_0_frrem_accum_b,
		   (hcd->hfnum_0_samples_b > 0) ?
		   hcd->hfnum_0_frrem_accum_b / hcd->hfnum_0_samples_b : 0);
	FH_PRINTF("Frame remaining at sample point B (uframe 1-6):\n");
	FH_PRINTF("  samples %u, accum %llu, avg %llu\n",
		   hcd->hfnum_other_samples_b, hcd->hfnum_other_frrem_accum_b,
		   (hcd->hfnum_other_samples_b > 0) ?
		   hcd->hfnum_other_frrem_accum_b /
		   hcd->hfnum_other_samples_b : 0);
#endif
}

#endif /* FH_DEVICE_ONLY */
