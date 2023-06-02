 /* ==========================================================================
  * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/fh_otg_pcd_linux.c $
  * $Revision: #30 $
  * $Date: 2015/08/06 $
  * $Change: 2913039 $
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
 * This file implements the Peripheral Controller Driver.
 *
 * The Peripheral Controller Driver (PCD) is responsible for
 * translating requests from the Function Driver into the appropriate
 * actions on the FH_otg controller. It isolates the Function Driver
 * from the specifics of the controller by providing an API to the
 * Function Driver.
 *
 * The Peripheral Controller Driver for Linux will implement the
 * Gadget API, so that the existing Gadget drivers can be used.
 * (Gadget Driver is the Linux terminology for a Function Driver.)
 *
 * The Linux Gadget API is defined in the header file
 * <code><linux/usb_gadget.h></code>.  The USB EP operations API is
 * defined in the structure <code>usb_ep_ops</code> and the USB
 * Controller API is defined in the structure
 * <code>usb_gadget_ops</code>.
 *
 */
#include <linux/platform_device.h>

#include "fh_otg_os_dep.h"
#include "fh_otg_pcd_if.h"
#include "fh_otg_pcd.h"
#include "fh_otg_driver.h"
#include "fh_otg_dbg.h"

static struct gadget_wrapper {
	fh_otg_pcd_t *pcd;

	struct usb_gadget gadget;
	struct usb_gadget_driver *driver;

	struct usb_ep ep0;
	struct usb_ep in_ep[16];
	struct usb_ep out_ep[16];

} *gadget_wrapper;

/* Display the contents of the buffer */
extern void dump_msg(const u8 * buf, unsigned int length);
/**
 * Get the fh_otg_pcd_ep_t* from usb_ep* pointer - NULL in case
 * if the endpoint is not found
 */
static struct fh_otg_pcd_ep *ep_from_handle(fh_otg_pcd_t * pcd, void *handle)
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

/* USB Endpoint Operations */
/*
 * The following sections briefly describe the behavior of the Gadget
 * API endpoint operations implemented in the FH_otg driver
 * software. Detailed descriptions of the generic behavior of each of
 * these functions can be found in the Linux header file
 * include/linux/usb_gadget.h.
 *
 * The Gadget API provides wrapper functions for each of the function
 * pointers defined in usb_ep_ops. The Gadget Driver calls the wrapper
 * function, which then calls the underlying PCD function. The
 * following sections are named according to the wrapper
 * functions. Within each section, the corresponding FH_otg PCD
 * function name is specified.
 *
 */

/**
 * This function is called by the Gadget Driver for each EP to be
 * configured for the current configuration (SET_CONFIGURATION).
 *
 * This function initializes the fh_otg_ep_t data structure, and then
 * calls fh_otg_ep_activate.
 */
static int ep_enable(struct usb_ep *usb_ep,
		     const struct usb_endpoint_descriptor *ep_desc)
{
	int retval;

	FH_DEBUGPL(DBG_PCDV, "%s(%p,%p)\n", __func__, usb_ep, ep_desc);

	if (!usb_ep || !ep_desc || ep_desc->bDescriptorType != USB_DT_ENDPOINT) {
		FH_WARN("%s, bad ep or descriptor\n", __func__);
		return -EINVAL;
	}
	if (usb_ep == &gadget_wrapper->ep0) {
		FH_WARN("%s, bad ep(0)\n", __func__);
		return -EINVAL;
	}

	/* Check FIFO size? */
	if (!ep_desc->wMaxPacketSize) {
		FH_WARN("%s, bad %s maxpacket\n", __func__, usb_ep->name);
		return -ERANGE;
	}

	if (!gadget_wrapper->driver ||
	    gadget_wrapper->gadget.speed == USB_SPEED_UNKNOWN) {
		FH_WARN("%s, bogus device state\n", __func__);
		return -ESHUTDOWN;
	}

	/* Delete after check - MAS */
#if 0
	nat = (uint32_t) ep_desc->wMaxPacketSize;
	printk(KERN_ALERT "%s: nat (before) =%d\n", __func__, nat);
	nat = (nat >> 11) & 0x03;
	printk(KERN_ALERT "%s: nat (after) =%d\n", __func__, nat);
#endif
	retval = fh_otg_pcd_ep_enable(gadget_wrapper->pcd,
				       (const uint8_t *)ep_desc,
				       (void *)usb_ep);
	if (retval) {
		FH_WARN("fh_otg_pcd_ep_enable failed\n");
		return -EINVAL;
	}

	usb_ep->maxpacket = le16_to_cpu(ep_desc->wMaxPacketSize);

	return 0;
}

/**
 * This function is called when an EP is disabled due to disconnect or
 * change in configuration. Any pending requests will terminate with a
 * status of -ESHUTDOWN.
 *
 * This function modifies the fh_otg_ep_t data structure for this EP,
 * and then calls fh_otg_ep_deactivate.
 */
static int ep_disable(struct usb_ep *usb_ep)
{
	int retval;

	FH_DEBUGPL(DBG_PCDV, "%s(%p)\n", __func__, usb_ep);
	if (!usb_ep) {
		FH_DEBUGPL(DBG_PCD, "%s, %s not enabled\n", __func__,
			    usb_ep ? usb_ep->name : NULL);
		return -EINVAL;
	}

	retval = fh_otg_pcd_ep_disable(gadget_wrapper->pcd, usb_ep);
	if (retval) {
		retval = -EINVAL;
	}

	return retval;
}

/**
 * This function allocates a request object to use with the specified
 * endpoint.
 *
 * @param ep The endpoint to be used with with the request
 * @param gfp_flags the GFP_* flags to use.
 */
static struct usb_request *fh_otg_pcd_alloc_request(struct usb_ep *ep,
						     gfp_t gfp_flags)
{
	struct usb_request *usb_req;

	FH_DEBUGPL(DBG_PCDV, "%s(%p,%d)\n", __func__, ep, gfp_flags);
	if (0 == ep) {
		FH_WARN("%s() %s\n", __func__, "Invalid EP!\n");
		return 0;
	}
	usb_req = kmalloc(sizeof(*usb_req), gfp_flags);
	if (0 == usb_req) {
		FH_WARN("%s() %s\n", __func__, "request allocation failed!\n");
		return 0;
	}
	memset(usb_req, 0, sizeof(*usb_req));
	usb_req->dma = FH_DMA_ADDR_INVALID;

	return usb_req;
}

/**
 * This function frees a request object.
 *
 * @param ep The endpoint associated with the request
 * @param req The request being freed
 */
static void fh_otg_pcd_free_request(struct usb_ep *ep, struct usb_request *req)
{
	FH_DEBUGPL(DBG_PCDV, "%s(%p,%p)\n", __func__, ep, req);

	if (0 == ep || 0 == req) {
		FH_WARN("%s() %s\n", __func__,
			 "Invalid ep or req argument!\n");
		return;
	}

	kfree(req);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
/**
 * This function allocates an I/O buffer to be used for a transfer
 * to/from the specified endpoint.
 *
 * @param usb_ep The endpoint to be used with with the request
 * @param bytes The desired number of bytes for the buffer
 * @param dma Pointer to the buffer's DMA address; must be valid
 * @param gfp_flags the GFP_* flags to use.
 * @return address of a new buffer or null is buffer could not be allocated.
 */
static void *fh_otg_pcd_alloc_buffer(struct usb_ep *usb_ep, unsigned bytes,
				      dma_addr_t * dma, gfp_t gfp_flags)
{
	void *buf;
	fh_otg_pcd_t *pcd = 0;

	pcd = gadget_wrapper->pcd;

	FH_DEBUGPL(DBG_PCDV, "%s(%p,%d,%p,%0x)\n", __func__, usb_ep, bytes,
		    dma, gfp_flags);

	/* Check dword alignment */
	if ((bytes & 0x3UL) != 0) {
		FH_WARN("%s() Buffer size is not a multiple of"
			 "DWORD size (%d)", __func__, bytes);
	}

	buf = dma_alloc_coherent(NULL, bytes, dma, gfp_flags);

	/* Check dword alignment */
	if (((int)buf & 0x3UL) != 0) {
		FH_WARN("%s() Buffer is not DWORD aligned (%p)",
			 __func__, buf);
	}

	return buf;
}

/**
 * This function frees an I/O buffer that was allocated by alloc_buffer.
 *
 * @param usb_ep the endpoint associated with the buffer
 * @param buf address of the buffer
 * @param dma The buffer's DMA address
 * @param bytes The number of bytes of the buffer
 */
static void fh_otg_pcd_free_buffer(struct usb_ep *usb_ep, void *buf,
				    dma_addr_t dma, unsigned bytes)
{
	fh_otg_pcd_t *pcd = 0;

	pcd = gadget_wrapper->pcd;

	FH_DEBUGPL(DBG_PCDV, "%s(%p,%0x,%d)\n", __func__, buf, dma, bytes);

	dma_free_coherent(NULL, bytes, buf, dma);
}
#endif

/**
 * This function is used to submit an I/O Request to an EP.
 *
 *	- When the request completes the request's completion callback
 *	  is called to return the request to the driver.
 *	- An EP, except control EPs, may have multiple requests
 *	  pending.
 *	- Once submitted the request cannot be examined or modified.
 *	- Each request is turned into one or more packets.
 *	- A BULK EP can queue any amount of data; the transfer is
 *	  packetized.
 *	- Zero length Packets are specified with the request 'zero'
 *	  flag.
 */
static int ep_queue(struct usb_ep *usb_ep, struct usb_request *usb_req,
		    gfp_t gfp_flags)
{
	fh_otg_pcd_t *pcd;
	struct fh_otg_pcd_ep *ep;
	int retval, is_isoc_ep, is_in_ep;
	dma_addr_t dma_addr;

	FH_DEBUGPL(DBG_PCDV, "%s(%p,%p,%d)\n",
		    __func__, usb_ep, usb_req, gfp_flags);

	if (!usb_req || !usb_req->complete || !usb_req->buf) {
		FH_WARN("bad params\n");
		return -EINVAL;
	}

	if (!usb_ep) {
		FH_WARN("bad ep\n");
		return -EINVAL;
	}

	pcd = gadget_wrapper->pcd;
	if (!gadget_wrapper->driver ||
	    gadget_wrapper->gadget.speed == USB_SPEED_UNKNOWN) {
		FH_DEBUGPL(DBG_PCDV, "gadget.speed=%d\n",
			    gadget_wrapper->gadget.speed);
		FH_WARN("bogus device state\n");
		return -ESHUTDOWN;
	}

	FH_DEBUGPL(DBG_PCD, "%s queue req %p, len %d buf %p\n",
		    usb_ep->name, usb_req, usb_req->length, usb_req->buf);

	usb_req->status = -EINPROGRESS;
	usb_req->actual = 0;

	ep = ep_from_handle(pcd, usb_ep);
	if (ep == NULL) {
		is_isoc_ep = 0;
		is_in_ep = 0;
	} else {
		is_isoc_ep = (ep->fh_ep.type == FH_OTG_EP_TYPE_ISOC) ? 1 : 0;
		is_in_ep = ep->fh_ep.is_in;
	}

	dma_addr = usb_req->dma;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,27)
	if (GET_CORE_IF(pcd)->dma_enable) {
		struct platform_device *dev =
		    gadget_wrapper->pcd->otg_dev->os_dep.pdev;
		if (dma_addr == FH_DMA_ADDR_INVALID) {
			if (usb_req->length != 0) {
				dma_addr = dma_map_single(dev, usb_req->buf,
					usb_req->length, is_in_ep ?
					DMA_TO_DEVICE : DMA_FROM_DEVICE);
				usb_req->dma = dma_addr;
			} else {
				dma_addr = 0;
			}
		}
	}
#endif

#ifdef FH_UTE_PER_IO
	if (is_isoc_ep == 1) {
		retval =
		    fh_otg_pcd_xiso_ep_queue(pcd, usb_ep, usb_req->buf,
					      dma_addr, usb_req->length,
					      usb_req->zero, usb_req,
					      gfp_flags == GFP_ATOMIC ? 1 : 0,
					      &usb_req->ext_req);
		if (retval)
			return -EINVAL;

		return 0;
	}
#endif
	retval = fh_otg_pcd_ep_queue(pcd, usb_ep, usb_req->buf, dma_addr,
				      usb_req->length, usb_req->zero, usb_req,
				      gfp_flags == GFP_ATOMIC ? 1 : 0);
	if (retval) {
		return -EINVAL;
	}

	return 0;
}

/**
 * This function cancels an I/O request from an EP.
 */
static int ep_dequeue(struct usb_ep *usb_ep, struct usb_request *usb_req)
{
	FH_DEBUGPL(DBG_PCDV, "%s(%p,%p)\n", __func__, usb_ep, usb_req);

	if (!usb_ep || !usb_req) {
		FH_WARN("bad argument\n");
		return -EINVAL;
	}
	if (!gadget_wrapper->driver ||
	    gadget_wrapper->gadget.speed == USB_SPEED_UNKNOWN) {
		FH_WARN("bogus device state\n");
		return -ESHUTDOWN;
	}
	if (fh_otg_pcd_ep_dequeue(gadget_wrapper->pcd, usb_ep, usb_req)) {
		return -EINVAL;
	}

	return 0;
}

/**
 * usb_ep_set_halt stalls an endpoint.
 *
 * usb_ep_clear_halt clears an endpoint halt and resets its data
 * toggle.
 *
 * Both of these functions are implemented with the same underlying
 * function. The behavior depends on the value argument.
 *
 * @param[in] usb_ep the Endpoint to halt or clear halt.
 * @param[in] value
 *	- 0 means clear_halt.
 *	- 1 means set_halt,
 *	- 2 means clear stall lock flag.
 *	- 3 means set  stall lock flag.
 */
static int ep_halt(struct usb_ep *usb_ep, int value)
{
	int retval = 0;

	FH_DEBUGPL(DBG_PCD, "HALT %s %d\n", usb_ep->name, value);

	if (!usb_ep) {
		FH_WARN("bad ep\n");
		return -EINVAL;
	}

	retval = fh_otg_pcd_ep_halt(gadget_wrapper->pcd, usb_ep, value);
	if (retval == -FH_E_AGAIN) {
		return -EAGAIN;
	} else if (retval) {
		retval = -EINVAL;
	}

	return retval;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
static int ep_wedge(struct usb_ep *usb_ep)
{
	FH_DEBUGPL(DBG_PCD, "WEDGE %s\n", usb_ep->name);

	return ep_halt(usb_ep, 3);
}
#endif

#ifdef FH_EN_ISOC
/**
 * This function is used to submit an ISOC Transfer Request to an EP.
 *
 *	- Every time a sync period completes the request's completion callback
 *	  is called to provide data to the gadget driver.
 *	- Once submitted the request cannot be modified.
 *	- Each request is turned into periodic data packets untill ISO
 *	  Transfer is stopped..
 */
static int iso_ep_start(struct usb_ep *usb_ep, struct usb_iso_request *req,
			gfp_t gfp_flags)
{
	int retval = 0;

	if (!req || !req->process_buffer || !req->buf0 || !req->buf1) {
		FH_WARN("bad params\n");
		return -EINVAL;
	}

	if (!usb_ep) {
		FH_PRINTF("bad params\n");
		return -EINVAL;
	}

	req->status = -EINPROGRESS;

	retval =
	    fh_otg_pcd_iso_ep_start(gadget_wrapper->pcd, usb_ep, req->buf0,
				     req->buf1, req->dma0, req->dma1,
				     req->sync_frame, req->data_pattern_frame,
				     req->data_per_frame,
				     req->
				     flags & USB_REQ_ISO_ASAP ? -1 :
				     req->start_frame, req->buf_proc_intrvl,
				     req, gfp_flags == GFP_ATOMIC ? 1 : 0);

	if (retval) {
		return -EINVAL;
	}

	return retval;
}

/**
 * This function stops ISO EP Periodic Data Transfer.
 */
static int iso_ep_stop(struct usb_ep *usb_ep, struct usb_iso_request *req)
{
	int retval = 0;
	if (!usb_ep) {
		FH_WARN("bad ep\n");
	}

	if (!gadget_wrapper->driver ||
	    gadget_wrapper->gadget.speed == USB_SPEED_UNKNOWN) {
		FH_DEBUGPL(DBG_PCDV, "gadget.speed=%d\n",
			    gadget_wrapper->gadget.speed);
		FH_WARN("bogus device state\n");
	}

	fh_otg_pcd_iso_ep_stop(gadget_wrapper->pcd, usb_ep, req);
	if (retval) {
		retval = -EINVAL;
	}

	return retval;
}

static struct usb_iso_request *alloc_iso_request(struct usb_ep *ep,
						 int packets, gfp_t gfp_flags)
{
	struct usb_iso_request *pReq = NULL;
	uint32_t req_size;

	req_size = sizeof(struct usb_iso_request);
	req_size +=
	    (2 * packets * (sizeof(struct usb_gadget_iso_packet_descriptor)));

	pReq = kmalloc(req_size, gfp_flags);
	if (!pReq) {
		FH_WARN("Can't allocate Iso Request\n");
		return 0;
	}
	pReq->iso_packet_desc0 = (void *)(pReq + 1);

	pReq->iso_packet_desc1 = pReq->iso_packet_desc0 + packets;

	return pReq;
}

static void free_iso_request(struct usb_ep *ep, struct usb_iso_request *req)
{
	kfree(req);
}

static struct usb_isoc_ep_ops fh_otg_pcd_ep_ops = {
	.ep_ops = {
			.enable = ep_enable,
			.disable = ep_disable,

		  	.alloc_request = fh_otg_pcd_alloc_request,
			.free_request = fh_otg_pcd_free_request,

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
		 .alloc_buffer = fh_otg_pcd_alloc_buffer,
         .free_buffer = fh_otg_pcd_free_buffer,
#endif

    	   .queue = ep_queue,
	       .dequeue = ep_dequeue,

		   .set_halt = ep_halt,
		 
		    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
					.set_wedge = ep_wedge,
			#endif
					.fifo_status = 0,
					.fifo_flush = 0,
			},

	.iso_ep_start = iso_ep_start,
	.iso_ep_stop = iso_ep_stop,
	.alloc_iso_request = alloc_iso_request,
	.free_iso_request = free_iso_request,
};

#else

static struct usb_ep_ops fh_otg_pcd_ep_ops = {
	.enable = ep_enable,
	.disable = ep_disable,

	.alloc_request = fh_otg_pcd_alloc_request,
	.free_request = fh_otg_pcd_free_request,

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
	.alloc_buffer = fh_otg_pcd_alloc_buffer,
	.free_buffer = fh_otg_pcd_free_buffer,
#endif

	.queue = ep_queue,
	.dequeue = ep_dequeue,

	.set_halt = ep_halt,
	
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
		.set_wedge = ep_wedge,
    #endif

	.fifo_status = 0,
	.fifo_flush = 0,

};

#endif /* _EN_ISOC_ */
/*	Gadget Operations */
/**
 * The following gadget operations will be implemented in the FH_otg
 * PCD. Functions in the API that are not described below are not
 * implemented.
 *
 * The Gadget API provides wrapper functions for each of the function
 * pointers defined in usb_gadget_ops. The Gadget Driver calls the
 * wrapper function, which then calls the underlying PCD function. The
 * following sections are named according to the wrapper functions
 * (except for ioctl, which doesn't have a wrapper function). Within
 * each section, the corresponding FH_otg PCD function name is
 * specified.
 *
 */

/**
 *Gets the USB Frame number of the last SOF.
 */
static int get_frame_number(struct usb_gadget *gadget)
{
	struct gadget_wrapper *d;

	FH_DEBUGPL(DBG_PCDV, "%s(%p)\n", __func__, gadget);

	if (gadget == 0) {
		return -ENODEV;
	}

	d = container_of(gadget, struct gadget_wrapper, gadget);
	return fh_otg_pcd_get_frame_number(d->pcd);
}

#ifdef CONFIG_USB_FH_OTG_LPM
static int test_lpm_enabled(struct usb_gadget *gadget)
{
	struct gadget_wrapper *d;

	d = container_of(gadget, struct gadget_wrapper, gadget);

	return fh_otg_pcd_is_lpm_enabled(d->pcd);
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
static int test_besl_enabled(struct usb_gadget *gadget)
{
	struct gadget_wrapper *d;

	d = container_of(gadget, struct gadget_wrapper, gadget);

	return fh_otg_pcd_is_besl_enabled(d->pcd);
}
static int get_param_baseline_besl(struct usb_gadget *gadget)
{
	struct gadget_wrapper *d;

	d = container_of(gadget, struct gadget_wrapper, gadget);

	return fh_otg_pcd_get_param_baseline_besl(d->pcd);
}
static int get_param_deep_besl(struct usb_gadget *gadget)
{
	struct gadget_wrapper *d;

	d = container_of(gadget, struct gadget_wrapper, gadget);

	return fh_otg_pcd_get_param_deep_besl(d->pcd);
}
#endif
#endif

/**
 * Initiates Session Request Protocol (SRP) to wakeup the host if no
 * session is in progress. If a session is already in progress, but
 * the device is suspended, remote wakeup signaling is started.
 *
 */
static int wakeup(struct usb_gadget *gadget)
{
	struct gadget_wrapper *d;

	FH_DEBUGPL(DBG_PCDV, "%s(%p)\n", __func__, gadget);

	if (gadget == 0) {
		return -ENODEV;
	} else {
		d = container_of(gadget, struct gadget_wrapper, gadget);
	}
	fh_otg_pcd_wakeup(d->pcd);
	return 0;
}

static const struct usb_gadget_ops fh_otg_pcd_ops = {
	.get_frame = get_frame_number,
	.wakeup = wakeup,
#ifdef CONFIG_USB_FH_OTG_LPM
	.lpm_support = test_lpm_enabled,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)	
	.besl_support = test_besl_enabled,
	.get_baseline_besl = get_param_baseline_besl,
	.get_deep_besl = get_param_deep_besl,
#endif	
#endif
	// current versions must always be self-powered
};

static int _setup(fh_otg_pcd_t * pcd, uint8_t * bytes)
{
	int retval = -FH_E_NOT_SUPPORTED;
	if (gadget_wrapper->driver && gadget_wrapper->driver->setup) {
		retval = gadget_wrapper->driver->setup(&gadget_wrapper->gadget,
						       (struct usb_ctrlrequest
							*)bytes);
	}

	if (retval == -ENOTSUPP) {
		retval = -FH_E_NOT_SUPPORTED;
	} else if (retval < 0) {
		retval = -FH_E_INVALID;
	}

	return retval;
}

#ifdef FH_EN_ISOC
static int _isoc_complete(fh_otg_pcd_t * pcd, void *ep_handle,
			  void *req_handle, int proc_buf_num)
{
	int i, packet_count;
	struct usb_gadget_iso_packet_descriptor *iso_packet = 0;
	struct usb_iso_request *iso_req = req_handle;

	if (proc_buf_num) {
		iso_packet = iso_req->iso_packet_desc1;
	} else {
		iso_packet = iso_req->iso_packet_desc0;
	}
	packet_count =
	    fh_otg_pcd_get_iso_packet_count(pcd, ep_handle, req_handle);
	for (i = 0; i < packet_count; ++i) {
		int status;
		int actual;
		int offset;
		fh_otg_pcd_get_iso_packet_params(pcd, ep_handle, req_handle,
						  i, &status, &actual, &offset);
		switch (status) {
		case -FH_E_NO_DATA:
			status = -ENODATA;
			break;
		default:
			if (status) {
				FH_PRINTF("unknown status in isoc packet\n");
			}

		}
		iso_packet[i].status = status;
		iso_packet[i].offset = offset;
		iso_packet[i].actual_length = actual;
	}

	iso_req->status = 0;
	iso_req->process_buffer(ep_handle, iso_req);

	return 0;
}
#endif /* FH_EN_ISOC */

#ifdef FH_UTE_PER_IO
/**
 * Copy the contents of the extended request to the Linux usb_request's
 * extended part and call the gadget's completion.
 *
 * @param pcd			Pointer to the pcd structure
 * @param ep_handle		Void pointer to the usb_ep structure
 * @param req_handle	Void pointer to the usb_request structure
 * @param status		Request status returned from the portable logic
 * @param ereq_port		Void pointer to the extended request structure
 *						created in the the portable part that contains the
 *						results of the processed iso packets.
 */
static int _xisoc_complete(fh_otg_pcd_t * pcd, void *ep_handle,
			   void *req_handle, int32_t status, void *ereq_port)
{
	struct fh_ute_iso_req_ext *ereqorg = NULL;
	struct fh_iso_xreq_port *ereqport = NULL;
	struct fh_ute_iso_packet_descriptor *desc_org = NULL;
	int i;
	struct usb_request *req;
	//struct fh_ute_iso_packet_descriptor *
	//int status = 0;

	req = (struct usb_request *)req_handle;
	ereqorg = &req->ext_req;
	ereqport = (struct fh_iso_xreq_port *)ereq_port;
	desc_org = ereqorg->per_io_frame_descs;

	if (req && req->complete) {
		/* Copy the request data from the portable logic to our request */
		for (i = 0; i < ereqport->pio_pkt_count; i++) {
			desc_org[i].actual_length =
			    ereqport->per_io_frame_descs[i].actual_length;
			desc_org[i].status =
			    ereqport->per_io_frame_descs[i].status;
		}

		switch (status) {
		case -FH_E_SHUTDOWN:
			req->status = -ESHUTDOWN;
			break;
		case -FH_E_RESTART:
			req->status = -ECONNRESET;
			break;
		case -FH_E_INVALID:
			req->status = -EINVAL;
			break;
		case -FH_E_TIMEOUT:
			req->status = -ETIMEDOUT;
			break;
		default:
			req->status = status;
		}

		/* And call the gadget's completion */
		req->complete(ep_handle, req);
	}

	return 0;
}
#endif /* FH_UTE_PER_IO */

static int _complete(fh_otg_pcd_t *pcd, void *ep_handle,
		     void *req_handle, int32_t status, uint32_t actual)
{
	struct usb_request *req = (struct usb_request *)req_handle;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,27)
	if (GET_CORE_IF(pcd)->dma_enable && req->length != 0) {
		struct platform_device *dev =
			gadget_wrapper->pcd->otg_dev->os_dep.pdev;
		struct fh_otg_pcd_ep *ep = ep_from_handle(pcd, ep_handle);
		int is_in_ep = 0;

		if (ep)
			is_in_ep = ep->fh_ep.is_in;

		dma_unmap_single(dev, req->dma, req->length, is_in_ep ?
				 PCI_DMA_TODEVICE : PCI_DMA_FROMDEVICE);
		req->dma = FH_DMA_ADDR_INVALID;
	};
#endif

	if (req && req->complete) {
		switch (status) {
		case -FH_E_SHUTDOWN:
			req->status = -ESHUTDOWN;
			break;
		case -FH_E_RESTART:
			req->status = -ECONNRESET;
			break;
		case -FH_E_INVALID:
			req->status = -EINVAL;
			break;
		case -FH_E_TIMEOUT:
			req->status = -ETIMEDOUT;
			break;
		default:
			req->status = status;

		}

		req->actual = actual;
		FH_SPINUNLOCK(pcd->lock);
		req->complete(ep_handle, req);
		FH_SPINLOCK(pcd->lock);
	}

	return 0;
}

static int _connect(fh_otg_pcd_t * pcd, int speed)
{
	gadget_wrapper->gadget.speed = speed;
	return 0;
}

static int _disconnect(fh_otg_pcd_t * pcd)
{
	if (gadget_wrapper->driver && gadget_wrapper->driver->disconnect) {
		gadget_wrapper->driver->disconnect(&gadget_wrapper->gadget);
	}
	return 0;
}

static int _resume(fh_otg_pcd_t * pcd)
{
	if (gadget_wrapper->driver && gadget_wrapper->driver->resume) {
		gadget_wrapper->driver->resume(&gadget_wrapper->gadget);
	}

	return 0;
}

static int _suspend(fh_otg_pcd_t * pcd)
{
	if (gadget_wrapper->driver && gadget_wrapper->driver->suspend) {
		gadget_wrapper->driver->suspend(&gadget_wrapper->gadget);
	}
	return 0;
}

/**
 * This function updates the otg values in the gadget structure.
 */
static int _hnp_changed(fh_otg_pcd_t * pcd)
{

	if (!gadget_wrapper->gadget.is_otg)
		return 0;

	gadget_wrapper->gadget.b_hnp_enable = get_b_hnp_enable(pcd);
	gadget_wrapper->gadget.a_hnp_support = get_a_hnp_support(pcd);
	gadget_wrapper->gadget.a_alt_hnp_support = get_a_alt_hnp_support(pcd);
	return 0;
}

static int _reset(fh_otg_pcd_t * pcd)
{
	return 0;
}

#ifdef FH_UTE_CFI
static int _cfi_setup(fh_otg_pcd_t * pcd, void *cfi_req)
{
	int retval = -FH_E_INVALID;
	if (gadget_wrapper->driver->cfi_feature_setup) {
		retval =
		    gadget_wrapper->driver->
		    cfi_feature_setup(&gadget_wrapper->gadget,
				      (struct cfi_usb_ctrlrequest *)cfi_req);
	}

	return retval;
}
#endif

static const struct fh_otg_pcd_function_ops fops = {
	.complete = _complete,
#ifdef FH_EN_ISOC
	.isoc_complete = _isoc_complete,
#endif
	.setup = _setup,
	.disconnect = _disconnect,
	.connect = _connect,
	.resume = _resume,
	.suspend = _suspend,
	.hnp_changed = _hnp_changed,
	.reset = _reset,
#ifdef FH_UTE_CFI
	.cfi_setup = _cfi_setup,
#endif
#ifdef FH_UTE_PER_IO
	.xisoc_complete = _xisoc_complete,
#endif
};

/**
 * This function is the top level PCD interrupt handler.
 */
static irqreturn_t fh_otg_pcd_irq(int irq, void *dev)
{
	fh_otg_pcd_t *pcd = dev;
	int32_t retval = IRQ_NONE;

	retval = fh_otg_pcd_handle_intr(pcd);
	if (retval != 0) {
		S3C2410X_CLEAR_EINTPEND();
	}
	return IRQ_RETVAL(retval);
}

/**
 * This function initialized the usb_ep structures to there default
 * state.
 *
 * @param d Pointer on gadget_wrapper.
 */
void gadget_add_eps(struct gadget_wrapper *d)
{
	static const char *names[] = {

		"ep0",
		"ep1in",
		"ep2out",
		"ep3in",
		"ep4out",
		"ep5in",
		"ep6out",
		"ep7in",
		"ep8out",
		"ep9in",
		"ep10out",
		"ep11in",
		"ep12out",
	};

	int i;
	struct usb_ep *ep;
	int8_t dev_endpoints;

	FH_DEBUGPL(DBG_PCDV, "%s\n", __func__);

	INIT_LIST_HEAD(&d->gadget.ep_list);
	d->gadget.ep0 = &d->ep0;
	d->gadget.speed = USB_SPEED_UNKNOWN;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0)
	d->gadget.max_speed = USB_SPEED_HIGH;
#endif

	INIT_LIST_HEAD(&d->gadget.ep0->ep_list);

	/**
	 * Initialize the EP0 structure.
	 */
	ep = &d->ep0;

	/* Init the usb_ep structure. */
	ep->name = names[0];
	ep->ops = (struct usb_ep_ops *)&fh_otg_pcd_ep_ops;

	/**
	 * @todo NGS: What should the max packet size be set to
	 * here?  Before EP type is set?
	 */
	ep->maxpacket = MAX_PACKET_SIZE;
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
	ep->maxpacket_limit = MAX_PACKET_SIZE;
#endif
	
	fh_otg_pcd_ep_enable(d->pcd, NULL, ep);

	list_add_tail(&ep->ep_list, &d->gadget.ep_list);

	/**
	 * Initialize the EP structures.
	 */
	dev_endpoints = d->pcd->core_if->dev_if->num_in_eps;

	for (i = 0; i < dev_endpoints; i++) {
		ep = &d->in_ep[i];

		/* Init the usb_ep structure. */
		ep->name = names[d->pcd->in_ep[i].fh_ep.num];
		ep->ops = (struct usb_ep_ops *)&fh_otg_pcd_ep_ops;

		/**
		 * @todo NGS: What should the max packet size be set to
		 * here?  Before EP type is set?
		 */
		ep->maxpacket = MAX_PACKET_SIZE;		
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
		ep->maxpacket_limit = MAX_PACKET_SIZE;
#endif
		
		list_add_tail(&ep->ep_list, &d->gadget.ep_list);
	}

	dev_endpoints = d->pcd->core_if->dev_if->num_out_eps;

	for (i = 0; i < dev_endpoints; i++) {
		ep = &d->out_ep[i];

		/* Init the usb_ep structure. */
		ep->name = names[d->pcd->out_ep[i].fh_ep.num];
		ep->ops = (struct usb_ep_ops *)&fh_otg_pcd_ep_ops;

		/**
		 * @todo NGS: What should the max packet size be set to
		 * here?  Before EP type is set?
		 */
		ep->maxpacket = MAX_PACKET_SIZE;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
		ep->maxpacket_limit = MAX_PACKET_SIZE;
#endif

		list_add_tail(&ep->ep_list, &d->gadget.ep_list);
	}


	/* remove ep0 from the list.  There is a ep0 pointer. */
	list_del_init(&d->ep0.ep_list);

	d->ep0.maxpacket = MAX_EP0_SIZE;
}

/**
 * This function releases the Gadget device.
 * required by device_unregister().
 *
 * @todo Should this do something?	Should it free the PCD?
 */
static void fh_otg_pcd_gadget_release(struct device *dev)
{
	FH_DEBUGPL(DBG_PCDV, "%s(%p)\n", __func__, dev);
}

static struct gadget_wrapper *alloc_wrapper(struct platform_device *_dev)
{
	static char pcd_name[] = "fh_otg";
    fh_otg_device_t *otg_dev = platform_get_drvdata(_dev);

	struct gadget_wrapper *d;
	int retval;

	d = FH_ALLOC(sizeof(*d));
	if (d == NULL) {
		return NULL;
	}

	memset(d, 0, sizeof(*d));

	d->gadget.name = pcd_name;
	d->pcd = otg_dev->pcd;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	strcpy(d->gadget.dev.bus_id, "gadget");
#else
	dev_set_name(&d->gadget.dev, "%s", "gadget");
#endif

	d->gadget.dev.parent = &_dev->dev;
	d->gadget.dev.release = fh_otg_pcd_gadget_release;
	d->gadget.ops = &fh_otg_pcd_ops;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)
	d->gadget.is_dualspeed = fh_otg_pcd_is_dualspeed(otg_dev->pcd);
#endif
	d->gadget.is_otg = fh_otg_pcd_is_otg(otg_dev->pcd);

	d->driver = 0;
	/* Register the gadget device */
	retval = device_register(&d->gadget.dev);
	if (retval != 0) {
		FH_ERROR("device_register failed\n");
		FH_FREE(d);
		return NULL;
	}

	return d;
}

static void free_wrapper(struct gadget_wrapper *d)
{
	if (d->driver) {
		/* should have been done already by driver model core */
		FH_WARN("driver '%s' is still registered\n",
			 d->driver->driver.name);
		usb_gadget_unregister_driver(d->driver);
	}

	device_unregister(&d->gadget.dev);
	FH_FREE(d);
}

/**
 * This function initialized the PCD portion of the driver.
 *
 */
int pcd_init(struct platform_device *dev, int irq)
{
	printk(KERN_ERR "%s(%p)\n", __func__, dev);
	fh_otg_device_t *otg_dev = platform_get_drvdata(dev);

	int retval = 0;

	otg_dev->pcd = fh_otg_pcd_init(otg_dev->core_if);

	if (!otg_dev->pcd) {
		FH_ERROR("fh_otg_pcd_init failed\n");
		return -ENOMEM;
	}

	otg_dev->pcd->otg_dev = otg_dev;
	gadget_wrapper = alloc_wrapper(dev);

	/*
	 * Initialize EP structures
	 */
	gadget_add_eps(gadget_wrapper);
	/*
	 * Setup interupt handler
	 */

	retval = request_irq(irq, fh_otg_pcd_irq,
			     IRQF_SHARED | IRQF_DISABLED,
			     gadget_wrapper->gadget.name, otg_dev->pcd);
	if (retval != 0) {
		FH_ERROR("request of irq%d failed\n", irq);
		free_wrapper(gadget_wrapper);
		return -EBUSY;
	}

	fh_otg_pcd_start(gadget_wrapper->pcd, &fops);
	platform_set_drvdata(dev, otg_dev);

	return retval;
}

/**
 * Cleanup the PCD.
 */
void pcd_remove(struct platform_device *dev, int irq)
{

	fh_otg_device_t *otg_dev = platform_get_drvdata(dev);
	printk(KERN_ERR "%s(%p)(%p)\n", __func__, dev, otg_dev);
	
	fh_otg_pcd_t *pcd = otg_dev->pcd;

	/*
	 * Free the IRQ
	 */
	printk(KERN_ERR "pcd free irq :%d\n", irq);
	free_irq(irq, pcd);
	free_wrapper(gadget_wrapper);
	fh_otg_pcd_remove(otg_dev->pcd);	
	otg_dev->pcd = 0;
}

/**
 * This function registers a gadget driver with the PCD.
 *
 * When a driver is successfully registered, it will receive control
 * requests including set_configuration(), which enables non-control
 * requests.  then usb traffic follows until a disconnect is reported.
 * then a host may connect again, or the driver might get unbound.
 *
 * @param driver The driver being registered
 * @param bind The bind function of gadget driver
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
int usb_gadget_register_driver(struct usb_gadget_driver *driver)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
	int usb_gadget_probe_driver(struct usb_gadget_driver *driver)
#else
int usb_gadget_probe_driver(struct usb_gadget_driver *driver,
		int (*bind)(struct usb_gadget *))
#endif
{
	int retval;

	FH_DEBUGPL(DBG_PCD, "registering gadget driver '%s'\n",
		    driver->driver.name);

	if (!driver || 
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)
		driver->speed == USB_SPEED_UNKNOWN ||
#else
		driver->max_speed == USB_SPEED_UNKNOWN ||
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37) || LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
	    !driver->bind ||
#else
		!bind ||
#endif
	    !driver->unbind || !driver->disconnect || !driver->setup) {
		FH_DEBUGPL(DBG_PCDV, "EINVAL\n");
		return -EINVAL;
	}
	if (gadget_wrapper == 0) {
		FH_DEBUGPL(DBG_PCDV, "ENODEV\n");
		return -ENODEV;
	}
	if (gadget_wrapper->driver != 0) {
		FH_DEBUGPL(DBG_PCDV, "EBUSY (%p)\n", gadget_wrapper->driver);
		return -EBUSY;
	}

	/* hook up the driver */
	gadget_wrapper->driver = driver;
	gadget_wrapper->gadget.dev.driver = &driver->driver;

	FH_DEBUGPL(DBG_PCD, "bind to driver %s\n", driver->driver.name);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
	retval = driver->bind(&gadget_wrapper->gadget);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)
	retval = driver->bind(&gadget_wrapper->gadget,gadget_wrapper->driver);		
#else
	retval = bind(&gadget_wrapper->gadget);
#endif
	if (retval) {
		FH_ERROR("bind to driver %s --> error %d\n",
			  driver->driver.name, retval);
		gadget_wrapper->driver = 0;
		gadget_wrapper->gadget.dev.driver = 0;
		return retval;
	}
	FH_DEBUGPL(DBG_ANY, "registered gadget driver '%s'\n",
		    driver->driver.name);
	return 0;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
EXPORT_SYMBOL(usb_gadget_register_driver);
#else
EXPORT_SYMBOL(usb_gadget_probe_driver);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,1)

int usb_udc_attach_driver(const char *name, struct usb_gadget_driver *driver)
 {
	int retval;
	if(strcmp(name, "fh_otg")){
		FH_ERROR("NO FH DEV FOUND \n");
		return -ENODEV;
	}	

	FH_DEBUGPL(DBG_PCD, "Registering gadget driver '%s'\n",
		    driver->driver.name);

	if (!driver || driver->max_speed == USB_SPEED_UNKNOWN || !driver->bind ||
	    !driver->unbind || !driver->disconnect || !driver->setup) {
		FH_DEBUGPL(DBG_PCDV, "EINVAL\n");
		return -EINVAL;
	}
	if (gadget_wrapper == 0) {
		FH_DEBUGPL(DBG_PCDV, "ENODEV\n");
		return -ENODEV;
	}
	if (gadget_wrapper->driver != 0) {
		FH_DEBUGPL(DBG_PCDV, "EBUSY (%p)\n", gadget_wrapper->driver);
		return -EBUSY;
	}

	/* hook up the driver */
	gadget_wrapper->driver = driver;
	gadget_wrapper->gadget.dev.driver = &driver->driver;

	FH_DEBUGPL(DBG_PCD, "bind to driver %s\n", driver->driver.name);
	retval = driver->bind(&gadget_wrapper->gadget,gadget_wrapper->driver);		
	if (retval) {
		FH_ERROR("bind to driver %s --> error %d\n",
			  driver->driver.name, retval);
		gadget_wrapper->driver = 0;
		gadget_wrapper->gadget.dev.driver = 0;
		return retval;
	}
	FH_DEBUGPL(DBG_ANY, "registered gadget driver '%s'\n",
		    driver->driver.name);
	return 0;
}
EXPORT_SYMBOL(usb_udc_attach_driver);

void usb_gadget_set_state(struct usb_gadget *gadget,
                enum usb_device_state state)
{
        gadget->state = state;
		FH_SCHEDULE_SYSTEM_WORK(&gadget->work);
}
EXPORT_SYMBOL_GPL(usb_gadget_set_state);

#endif 



/**
 * This function unregisters a gadget driver
 *
 * @param driver The driver being unregistered
 */
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	//FH_DEBUGPL(DBG_PCDV,"%s(%p)\n", __func__, _driver);

	if (gadget_wrapper == 0) {
		FH_DEBUGPL(DBG_ANY, "%s Return(%d): s_pcd==0\n", __func__,
			    -ENODEV);
		return -ENODEV;
	}
	if (driver == 0 || driver != gadget_wrapper->driver) {
		FH_DEBUGPL(DBG_ANY, "%s Return(%d): driver?\n", __func__,
			    -EINVAL);
		return -EINVAL;
	}

	driver->unbind(&gadget_wrapper->gadget);
	gadget_wrapper->driver = 0;

	FH_DEBUGPL(DBG_ANY, "unregistered driver '%s'\n", driver->driver.name);
	return 0;
}

EXPORT_SYMBOL(usb_gadget_unregister_driver);

#endif /* FH_HOST_ONLY */
