/* ==========================================================================
 * $File: //dwh/usb_iip/dev/software/otg/linux/drivers/fh_otg_hcd_linux.c $
 * $Revision: #25 $
 * $Date: 2015/09/08 $
 * $Change: 2943025 $
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

/**
 * @file
 *
 * This file contains the implementation of the HCD. In Linux, the HCD
 * implements the hc_driver API.
 */
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#include <asm/io.h>
#include <linux/usb.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
#include <../drivers/usb/core/hcd.h>
#else
#include <linux/usb/hcd.h>
#endif

#include "fh_otg_hcd_if.h"
#include "fh_otg_dbg.h"
#include "fh_otg_driver.h"
#include "fh_otg_hcd.h"
/**
 * Gets the endpoint number from a _bEndpointAddress argument. The endpoint is
 * qualified with its direction (possible 32 endpoints per device).
 */
#define fh_ep_addr_to_endpoint(_bEndpointAddress_) ((_bEndpointAddress_ & USB_ENDPOINT_NUMBER_MASK) | \
						     ((_bEndpointAddress_ & USB_DIR_IN) != 0) << 4)

static const char fh_otg_hcd_name[] = "fh_otg_hcd";

/** @name Linux HC Driver API Functions */
/** @{ */
static int urb_enqueue(struct usb_hcd *hcd,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
		       struct usb_host_endpoint *ep,
#endif
		       struct urb *urb, gfp_t mem_flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
static int urb_dequeue(struct usb_hcd *hcd, struct urb *urb);
#else
static int urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status);
#endif

static void endpoint_disable(struct usb_hcd *hcd, struct usb_host_endpoint *ep);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
static void endpoint_reset(struct usb_hcd *hcd, struct usb_host_endpoint *ep);
#endif
static irqreturn_t fh_otg_hcd_irq(struct usb_hcd *hcd);
extern int hcd_start(struct usb_hcd *hcd);
extern void hcd_stop(struct usb_hcd *hcd);
static int get_frame_number(struct usb_hcd *hcd);
extern int hub_status_data(struct usb_hcd *hcd, char *buf);
extern int hub_control(struct usb_hcd *hcd,
		       u16 typeReq,
		       u16 wValue, u16 wIndex, char *buf, u16 wLength);

struct wrapper_priv_data {
	fh_otg_hcd_t *fh_otg_hcd;
};

/** @} */

static struct hc_driver fh_otg_hc_driver = {

	.description = fh_otg_hcd_name,
	.product_desc = "FH OTG Controller",
	.hcd_priv_size = sizeof(struct wrapper_priv_data),

	.irq = fh_otg_hcd_irq,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,12,0)
	.flags = HCD_MEMORY | HCD_USB2,
#else
	.flags = HCD_MEMORY | HCD_USB2 | HCD_BH,
#endif


	.start = hcd_start,
	.stop = hcd_stop,

	.urb_enqueue = urb_enqueue,
	.urb_dequeue = urb_dequeue,
	.endpoint_disable = endpoint_disable,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	.endpoint_reset = endpoint_reset,
#endif
	.get_frame_number = get_frame_number,

	.hub_status_data = hub_status_data,
	.hub_control = hub_control,
	//.bus_suspend =                
	//.bus_resume =         
};

/** Gets the fh_otg_hcd from a struct usb_hcd */
static inline fh_otg_hcd_t *hcd_to_fh_otg_hcd(struct usb_hcd *hcd)
{
	struct wrapper_priv_data *p;
	p = (struct wrapper_priv_data *)(hcd->hcd_priv);
	return p->fh_otg_hcd;
}

/** Gets the struct usb_hcd that contains a fh_otg_hcd_t. */
static inline struct usb_hcd *fh_otg_hcd_to_hcd(fh_otg_hcd_t * fh_otg_hcd)
{
	return fh_otg_hcd_get_priv_data(fh_otg_hcd);
}

/** Gets the usb_host_endpoint associated with an URB. */
inline struct usb_host_endpoint *fh_urb_to_endpoint(struct urb *urb)
{
	struct usb_device *dev = urb->dev;
	int ep_num = usb_pipeendpoint(urb->pipe);

	if (usb_pipein(urb->pipe))
		return dev->ep_in[ep_num];
	else
		return dev->ep_out[ep_num];
}

static int _disconnect(fh_otg_hcd_t * hcd)
{
	struct usb_hcd *usb_hcd = fh_otg_hcd_to_hcd(hcd);

	usb_hcd->self.is_b_host = 0;
	return 0;
}

static int _start(fh_otg_hcd_t * hcd)
{
	struct usb_hcd *usb_hcd = fh_otg_hcd_to_hcd(hcd);

	usb_hcd->self.is_b_host = fh_otg_hcd_is_b_host(hcd);
	hcd_start(usb_hcd);

	return 0;
}

static int _hub_info(fh_otg_hcd_t * hcd, void *urb_handle, uint32_t * hub_addr,
		     uint32_t * port_addr)
{
	struct urb *urb = (struct urb *)urb_handle;
	if (urb->dev->tt) {
		*hub_addr = urb->dev->tt->hub->devnum;
	} else {
		*hub_addr = 0;
	}
	*port_addr = urb->dev->ttport;
	return 0;
}

static int _speed(fh_otg_hcd_t * hcd, void *urb_handle)
{
	struct urb *urb = (struct urb *)urb_handle;
	return urb->dev->speed;
}

static int _get_b_hnp_enable(fh_otg_hcd_t * hcd)
{
	struct usb_hcd *usb_hcd = fh_otg_hcd_to_hcd(hcd);
	return usb_hcd->self.b_hnp_enable;
}

static void allocate_bus_bandwidth(struct usb_hcd *hcd, uint32_t bw,
				   struct urb *urb)
{
	hcd_to_bus(hcd)->bandwidth_allocated += bw / urb->interval;
	if (usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS) {
		hcd_to_bus(hcd)->bandwidth_isoc_reqs++;
	} else {
		hcd_to_bus(hcd)->bandwidth_int_reqs++;
	}
}

static void free_bus_bandwidth(struct usb_hcd *hcd, uint32_t bw,
			       struct urb *urb)
{
	hcd_to_bus(hcd)->bandwidth_allocated -= bw / urb->interval;
	if (usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS) {
		hcd_to_bus(hcd)->bandwidth_isoc_reqs--;
	} else {
		hcd_to_bus(hcd)->bandwidth_int_reqs--;
	}
}

/**
 * Sets the final status of an URB and returns it to the device driver. Any
 * required cleanup of the URB is performed.
 */
static int _complete(fh_otg_hcd_t * hcd, void *urb_handle,
		     fh_otg_hcd_urb_t * fh_otg_urb, int32_t status)
{
	struct urb *urb = (struct urb *)urb_handle;
#ifdef DEBUG
	if (CHK_DEBUG_LEVEL(DBG_HCDV | DBG_HCD_URB)) {
		FH_PRINTF("%s: urb %p, device %d, ep %d %s, status=%d\n",
			   __func__, urb, usb_pipedevice(urb->pipe),
			   usb_pipeendpoint(urb->pipe),
			   usb_pipein(urb->pipe) ? "IN" : "OUT", status);
		if (usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS) {
			int i;
			for (i = 0; i < urb->number_of_packets; i++) {
				FH_PRINTF("  ISO Desc %d status: %d\n",
					   i, urb->iso_frame_desc[i].status);
			}
		}
	}
#endif

	urb->actual_length = fh_otg_hcd_urb_get_actual_length(fh_otg_urb);
	/* Convert status value. */
	switch (status) {
	case -FH_E_PROTOCOL:
		status = -EPROTO;
		break;
	case -FH_E_IN_PROGRESS:
		status = -EINPROGRESS;
		break;
	case -FH_E_PIPE:
		status = -EPIPE;
		break;
	case -FH_E_IO:
		status = -EIO;
		break;
	case -FH_E_TIMEOUT:
		status = -ETIMEDOUT;
		break;
	case -FH_E_OVERFLOW:
		status = -EOVERFLOW;
		break;
	default:
		if (status) {
			FH_PRINTF("Uknown urb status %d\n", status);

		}
	}

	if (usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS) {
		int i;

		urb->error_count = fh_otg_hcd_urb_get_error_count(fh_otg_urb);
		for (i = 0; i < urb->number_of_packets; ++i) {
			urb->iso_frame_desc[i].actual_length =
			    fh_otg_hcd_urb_get_iso_desc_actual_length
			    (fh_otg_urb, i);
			urb->iso_frame_desc[i].status =
			    fh_otg_hcd_urb_get_iso_desc_status(fh_otg_urb, i);
		}
	}

	urb->status = status;
	if (!status) {
		if ((urb->transfer_flags & URB_SHORT_NOT_OK) &&
		    (urb->actual_length < urb->transfer_buffer_length)) {
			urb->status = -EREMOTEIO;
		}
	}

	if ((usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS) ||
	    (usb_pipetype(urb->pipe) == PIPE_INTERRUPT)) {
		struct usb_host_endpoint *ep = fh_urb_to_endpoint(urb);
		if (ep) {
			free_bus_bandwidth(fh_otg_hcd_to_hcd(hcd),
					   fh_otg_hcd_get_ep_bandwidth(hcd,
									ep->hcpriv),
					   urb);
		}
	}
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,7,7)
	usb_hcd_unlink_urb_from_ep(fh_otg_hcd_to_hcd(hcd), urb);
#endif
	
	urb->hcpriv = NULL;
	FH_FREE(fh_otg_urb);

	//printk("\nfuck...\n");
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
	FH_SPINUNLOCK(hcd->lock);
	usb_hcd_giveback_urb(fh_otg_hcd_to_hcd(hcd), urb);
	FH_SPINLOCK(hcd->lock);
#elseif LINUX_VERSION_CODE <= KERNEL_VERSION(3,7,7)
	FH_SPINUNLOCK(hcd->lock);
	usb_hcd_giveback_urb(fh_otg_hcd_to_hcd(hcd), urb, status);
	FH_SPINLOCK(hcd->lock);
#else
	usb_hcd_giveback_urb(fh_otg_hcd_to_hcd(hcd), urb, status);
#endif

	return 0;
}

static struct fh_otg_hcd_function_ops hcd_fops = {
	.start = _start,
	.disconnect = _disconnect,
	.hub_info = _hub_info,
	.speed = _speed,
	.complete = _complete,
	.get_b_hnp_enable = _get_b_hnp_enable,
};

/**
 * Initializes the HCD. This function allocates memory for and initializes the
 * static parts of the usb_hcd and fh_otg_hcd structures. It also registers the
 * USB bus with the core and calls the hc_driver->start() function. It returns
 * a negative error on failure.
 */
int hcd_init(struct platform_device *dev, int irq)
{
	struct usb_hcd *hcd = NULL;
	fh_otg_hcd_t *fh_otg_hcd = NULL;
	fh_otg_device_t *otg_dev = platform_get_drvdata(dev);

	int retval = 0;

	printk(KERN_ERR "FH OTG HCD INIT (%p)\n", otg_dev);

	/* Set device flags indicating whether the HCD supports DMA */
	if (otg_dev->core_if->dma_enable > 0) {
		if (dma_set_mask(&dev->dev, DMA_BIT_MASK(32)) < 0)
			printk(KERN_ERR "can't set DMA mask\n");
		if (dma_set_coherent_mask(&dev->dev, DMA_BIT_MASK(32)) < 0)
			printk(KERN_ERR "can't set coherent DMA mask\n");
	}

	/*
	 * Allocate memory for the base HCD plus the FH OTG HCD.
	 * Initialize the base HCD.
	 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	hcd = usb_create_hcd(&fh_otg_hc_driver, &dev->dev, dev->dev.bus_id);
#else
	hcd = usb_create_hcd(&fh_otg_hc_driver, &dev->dev, dev_name(&dev->dev));

	hcd->has_tt = 1;
//      hcd->uses_new_polling = 1;
//      hcd->poll_rh = 0;
#endif
	if (!hcd) {
		retval = -ENOMEM;
		goto error1;
	}

	printk(KERN_ERR "hcd regs before base(%p)\n", otg_dev->os_dep.base);
	hcd->regs = otg_dev->os_dep.base;

	/* Initialize the FH OTG HCD. */
	fh_otg_hcd = fh_otg_hcd_alloc_hcd();
	if (!fh_otg_hcd) {
		goto error2;
	}
	((struct wrapper_priv_data *)(hcd->hcd_priv))->fh_otg_hcd =
	    fh_otg_hcd;
	otg_dev->hcd = fh_otg_hcd;

	if (fh_otg_hcd_init(fh_otg_hcd, otg_dev->core_if)) {
		goto error2;
	}

	otg_dev->hcd->otg_dev = otg_dev;
	hcd->self.otg_port = fh_otg_hcd_otg_port(fh_otg_hcd);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33) //don't support for LM(with 2.6.20.1 kernel)
    //hcd->self.otg_version = fh_otg_get_otg_version(otg_dev->core_if);
	/* Don't support SG list at this point */
	hcd->self.sg_tablesize = 0;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
	/* Do not to do HNP polling if not capable */
	if (otg_dev->core_if->otg_ver)
		hcd->self.is_hnp_cap = fh_otg_get_hnpcapable(otg_dev->core_if);
#endif
	/*
	 * Finish generic HCD initialization and start the HCD. This function
	 * allocates the DMA buffer pool, registers the USB bus, requests the
	 * IRQ line, and calls hcd_start method.
	 */
	retval = usb_add_hcd(hcd, irq, IRQF_SHARED | IRQF_DISABLED);
	if (retval < 0) {
		goto error2;
	}

	fh_otg_hcd_set_priv_data(fh_otg_hcd, hcd);
	platform_set_drvdata(dev, otg_dev);
	return 0;

error2:
	usb_put_hcd(hcd);
error1:
	return retval;
}

/**
 * Removes the HCD.
 * Frees memory and resources associated with the HCD and deregisters the bus.
 */
void hcd_remove(struct platform_device *dev)
{
	fh_otg_device_t *otg_dev = platform_get_drvdata(dev);


	fh_otg_hcd_t *fh_otg_hcd;
	struct usb_hcd *hcd;

	FH_DEBUGPL(DBG_HCD, "FH OTG HCD REMOVE\n");

	if (!otg_dev) {
		FH_DEBUGPL(DBG_ANY, "%s: otg_dev NULL!\n", __func__);
		return;
	}

	fh_otg_hcd = otg_dev->hcd;

	if (!fh_otg_hcd) {
		FH_DEBUGPL(DBG_ANY, "%s: otg_dev->hcd NULL!\n", __func__);
		return;
	}

	hcd = fh_otg_hcd_to_hcd(fh_otg_hcd);

	if (!hcd) {
		FH_DEBUGPL(DBG_ANY,
			    "%s: fh_otg_hcd_to_hcd(fh_otg_hcd) NULL!\n",
			    __func__);
		return;
	}
	usb_remove_hcd(hcd);
	fh_otg_hcd_set_priv_data(fh_otg_hcd, NULL);
	fh_otg_hcd_remove(fh_otg_hcd);
	usb_put_hcd(hcd);
}

/* =========================================================================
 *  Linux HC Driver Functions
 * ========================================================================= */

/** Initializes the FH_otg controller and its root hub and prepares it for host
 * mode operation. Activates the root port. Returns 0 on success and a negative
 * error code on failure. */
int hcd_start(struct usb_hcd *hcd)
{
	fh_otg_hcd_t *fh_otg_hcd = hcd_to_fh_otg_hcd(hcd);
	struct usb_bus *bus;

	FH_DEBUGPL(DBG_HCD, "FH OTG HCD START\n");
	bus = hcd_to_bus(hcd);

	hcd->state = HC_STATE_RUNNING;
	if (fh_otg_hcd_start(fh_otg_hcd, &hcd_fops)) {
		if (fh_otg_hcd->core_if->otg_ver && fh_otg_is_device_mode(fh_otg_hcd->core_if))
			fh_otg_hcd->core_if->op_state = B_PERIPHERAL;
		return 0;
	}

	/* Initialize and connect root hub if one is not already attached */
	if (bus->root_hub) {
		FH_DEBUGPL(DBG_HCD, "FH OTG HCD Has Root Hub\n");
		/* Inform the HUB driver to resume. */
		usb_hcd_resume_root_hub(hcd);
	}

	return 0;
}

/**
 * Halts the FH_otg host mode operations in a clean manner. USB transfers are
 * stopped.
 */
void hcd_stop(struct usb_hcd *hcd)
{
	fh_otg_hcd_t *fh_otg_hcd = hcd_to_fh_otg_hcd(hcd);

	fh_otg_hcd_stop(fh_otg_hcd);
}

/** Returns the current frame number. */
static int get_frame_number(struct usb_hcd *hcd)
{
	fh_otg_hcd_t *fh_otg_hcd = hcd_to_fh_otg_hcd(hcd);

	return fh_otg_hcd_get_frame_number(fh_otg_hcd);
}

#ifdef DEBUG
static void dump_urb_info(struct urb *urb, char *fn_name)
{
	printk("%s, urb %p\n", fn_name, urb);
	printk("  Device address: %d\n", usb_pipedevice(urb->pipe));
	printk("  Endpoint: %d, %s\n", usb_pipeendpoint(urb->pipe),
		   (usb_pipein(urb->pipe) ? "IN" : "OUT"));
	printk("  Endpoint type: %s\n", ( {
					     char *pipetype;
					     switch (usb_pipetype(urb->pipe)) {
case PIPE_CONTROL:
pipetype = "CONTROL"; break; case PIPE_BULK:
pipetype = "BULK"; break; case PIPE_INTERRUPT:
pipetype = "INTERRUPT"; break; case PIPE_ISOCHRONOUS:
pipetype = "ISOCHRONOUS"; break; default:
					     pipetype = "UNKNOWN"; break;};
					     pipetype;}
		   )) ;
	printk("  Speed: %s\n", ( {
				     char *speed; switch (urb->dev->speed) {
case USB_SPEED_HIGH:
speed = "HIGH"; break; case USB_SPEED_FULL:
speed = "FULL"; break; case USB_SPEED_LOW:
speed = "LOW"; break; default:
				     speed = "UNKNOWN"; break;};
				     speed;}
		   )) ;
	printk("  Max packet size: %d\n",
		   usb_maxpacket(urb->dev, urb->pipe, usb_pipeout(urb->pipe)));
	printk("  Data buffer length: %d\n", urb->transfer_buffer_length);
	printk("  Transfer buffer: %p, Transfer DMA: %p\n",
		   urb->transfer_buffer, (void *)urb->transfer_dma);
	printk("  Setup buffer: %p, Setup DMA: %p\n",
		   urb->setup_packet, (void *)urb->setup_dma);
    printk("  Interval: %d\n", urb->interval);
	if (usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS) {
		int i;
		for (i = 0; i < urb->number_of_packets; i++) {
			printk("  ISO Desc %d:\n", i);
			printk("    offset: %d, length %d\n",
				   urb->iso_frame_desc[i].offset,
				   urb->iso_frame_desc[i].length);
		}
	}
}

#endif

/** Starts processing a USB transfer request specified by a USB Request Block
 * (URB). mem_flags indicates the type of memory allocation to use while
 * processing this URB. */
static int ___urb_enqueue(struct usb_hcd *hcd,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
		       struct usb_host_endpoint *ep,
#endif
		       struct urb *urb, gfp_t mem_flags)
{
	int retval = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
	struct usb_host_endpoint *ep = urb->ep;
#endif
	fh_otg_hcd_t *fh_otg_hcd = hcd_to_fh_otg_hcd(hcd);
	fh_otg_hcd_urb_t *fh_otg_urb;
	int i;
	int alloc_bandwidth = 0;
	uint8_t ep_type = 0;
	uint32_t flags = 0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,7,7)	
	fh_irqflags_t irqflags;
#endif
	void *buf;

#ifdef DEBUG
	if (CHK_DEBUG_LEVEL(DBG_HCDV | DBG_HCD_URB)) {
		dump_urb_info(urb, "urb_enqueue");
	}
#endif

	if ((usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS)
	    || (usb_pipetype(urb->pipe) == PIPE_INTERRUPT)) {
		if (!fh_otg_hcd_is_bandwidth_allocated
		    (fh_otg_hcd, &ep->hcpriv)) {
			alloc_bandwidth = 1;
		}
	}

	switch (usb_pipetype(urb->pipe)) {
	case PIPE_CONTROL:
		ep_type = USB_ENDPOINT_XFER_CONTROL;
		break;
	case PIPE_ISOCHRONOUS:
		ep_type = USB_ENDPOINT_XFER_ISOC;
		break;
	case PIPE_BULK:
		ep_type = USB_ENDPOINT_XFER_BULK;
		break;
	case PIPE_INTERRUPT:
		ep_type = USB_ENDPOINT_XFER_INT;
		break;
	default:
		FH_WARN("Wrong ep type\n");
	}

	fh_otg_urb = fh_otg_hcd_urb_alloc(fh_otg_hcd,
					    urb->number_of_packets,
					    mem_flags == GFP_ATOMIC ? 1 : 0);

	fh_otg_hcd_urb_set_pipeinfo(fh_otg_urb, usb_pipedevice(urb->pipe),
				     usb_pipeendpoint(urb->pipe), ep_type,
				     usb_pipein(urb->pipe),
				     usb_maxpacket(urb->dev, urb->pipe,
						   !(usb_pipein(urb->pipe))));

	buf = urb->transfer_buffer;
	if (hcd->self.uses_dma) {
		/*
		 * Calculate virtual address from physical address,
		 * because some class driver may not fill transfer_buffer.
		 * In Buffer DMA mode virual address is used,
		 * when handling non DWORD aligned buffers.
		 */
		buf = phys_to_virt(urb->transfer_dma);
	}

	if (!(urb->transfer_flags & URB_NO_INTERRUPT))
		flags |= URB_GIVEBACK_ASAP;
	if (urb->transfer_flags & URB_ZERO_PACKET)
		flags |= URB_SEND_ZERO_PACKET;

	fh_otg_hcd_urb_set_params(fh_otg_urb, urb, buf,
				   urb->transfer_dma,
				   urb->transfer_buffer_length,
				   urb->setup_packet,
				   urb->setup_dma, flags, urb->interval);

	for (i = 0; i < urb->number_of_packets; ++i) {
		fh_otg_hcd_urb_set_iso_desc_params(fh_otg_urb, i,
						    urb->
						    iso_frame_desc[i].offset,
						    urb->
						    iso_frame_desc[i].length);
	}

	urb->hcpriv = fh_otg_urb;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,7,7)	
	FH_SPINLOCK_IRQSAVE(fh_otg_hcd->lock, &irqflags);
	retval = usb_hcd_link_urb_to_ep(hcd, urb);
	FH_SPINUNLOCK_IRQRESTORE(fh_otg_hcd->lock, irqflags);
	if (retval)
		goto fail1;
#endif
	
	retval = fh_otg_hcd_urb_enqueue(fh_otg_hcd, fh_otg_urb, &ep->hcpriv,
					 mem_flags == GFP_ATOMIC ? 1 : 0);
	if (retval){
		goto fail2;
	}
	
	if (alloc_bandwidth) {
		allocate_bus_bandwidth(hcd,
		       fh_otg_hcd_get_ep_bandwidth
		       (fh_otg_hcd, ep->hcpriv), urb);
	}
	
	return 0;
	
fail2:
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,7,7)
    FH_SPINLOCK_IRQSAVE(fh_otg_hcd->lock, &irqflags);
    fh_otg_urb->priv = NULL;
    usb_hcd_unlink_urb_from_ep(hcd, urb);
    FH_SPINUNLOCK_IRQRESTORE(fh_otg_hcd->lock, irqflags);
fail1:
#endif
    urb->hcpriv = NULL;
    FH_FREE(fh_otg_urb);
	
	return retval;
}

static int urb_enqueue(struct usb_hcd *hcd,
		       struct urb *urb, gfp_t mem_flags)
{
	int ret;
	unsigned long flagxx;

	local_irq_save(flagxx);
	ret = ___urb_enqueue(hcd, urb, GFP_ATOMIC);
	local_irq_restore(flagxx);

	return ret;
}

/** Aborts/cancels a USB transfer request. Always returns 0 to indicate
 * success.  */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
static int urb_dequeue(struct usb_hcd *hcd, struct urb *urb)
#else
static int urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status)
#endif
{
	fh_irqflags_t flags;
	fh_otg_hcd_t *fh_otg_hcd;
	int rc = 0;
	FH_DEBUGPL(DBG_HCD, "FH OTG HCD URB Dequeue\n");

	fh_otg_hcd = hcd_to_fh_otg_hcd(hcd);

#ifdef DEBUG
	if (CHK_DEBUG_LEVEL(DBG_HCDV | DBG_HCD_URB)) {
		dump_urb_info(urb, "urb_dequeue");
	}
#endif

	FH_SPINLOCK_IRQSAVE(fh_otg_hcd->lock, &flags);
	
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,7,7)
	rc = usb_hcd_check_unlink_urb(hcd, urb, status);
	if (rc)
		goto out;
#endif

	if (!urb->hcpriv) {
		FH_DEBUGPL(DBG_HCD, "urb->hcpriv is NULL\n");
		goto out;
	}

	rc = fh_otg_hcd_urb_dequeue(fh_otg_hcd, urb->hcpriv);

#if LINUX_VERSION_CODE > KERNEL_VERSION(3,7,7)
	usb_hcd_unlink_urb_from_ep(hcd, urb);
#endif
	
	FH_FREE(urb->hcpriv);
	urb->hcpriv = NULL;

	/* Higher layer software sets URB status. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
	usb_hcd_giveback_urb(hcd, urb);
#else
	FH_SPINUNLOCK_IRQRESTORE(fh_otg_hcd->lock, flags); //mvardan
	usb_hcd_giveback_urb(hcd, urb, status);
	FH_SPINLOCK_IRQSAVE(fh_otg_hcd->lock, &flags); //mvardan
#endif
	if (CHK_DEBUG_LEVEL(DBG_HCDV | DBG_HCD_URB)) {
		FH_PRINTF("Called usb_hcd_giveback_urb()\n");
		FH_PRINTF("  urb->status = %d\n", urb->status);
	}
out:
	FH_SPINUNLOCK_IRQRESTORE(fh_otg_hcd->lock, flags);

	return rc;
}

/* Frees resources in the FH_otg controller related to a given endpoint. Also
 * clears state in the HCD related to the endpoint. Any URBs for the endpoint
 * must already be dequeued. */
static void endpoint_disable(struct usb_hcd *hcd, struct usb_host_endpoint *ep)
{
	fh_otg_hcd_t *fh_otg_hcd = hcd_to_fh_otg_hcd(hcd);

	FH_DEBUGPL(DBG_HCD,
		    "FH OTG HCD EP DISABLE: _bEndpointAddress=0x%02x, "
		    "endpoint=%d\n", ep->desc.bEndpointAddress,
		    fh_ep_addr_to_endpoint(ep->desc.bEndpointAddress));
	fh_otg_hcd_endpoint_disable(fh_otg_hcd, ep->hcpriv, 250);
	ep->hcpriv = NULL;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
/* Resets endpoint specific parameter values, in current version used to reset 
 * the data toggle(as a WA). This function can be called from usb_clear_halt routine */
static void endpoint_reset(struct usb_hcd *hcd, struct usb_host_endpoint *ep)
{
	fh_irqflags_t flags;
	struct usb_device *udev = NULL;
	int epnum = usb_endpoint_num(&ep->desc);
	int is_out = usb_endpoint_dir_out(&ep->desc);
	int is_control = usb_endpoint_xfer_control(&ep->desc);
	fh_otg_hcd_t *fh_otg_hcd = hcd_to_fh_otg_hcd(hcd);
	struct platform_device *_dev = fh_otg_hcd->otg_dev->os_dep.pdev;

	if (_dev)
		udev = to_usb_device(&_dev->dev);
	else
		return;

	FH_DEBUGPL(DBG_HCD, "FH OTG HCD EP RESET: Endpoint Num=0x%02d\n", epnum);

	FH_SPINLOCK_IRQSAVE(fh_otg_hcd->lock, &flags);
	usb_settoggle(udev, epnum, is_out, 0);
	if (is_control)
		usb_settoggle(udev, epnum, !is_out, 0);

	if (ep->hcpriv) {
		fh_otg_hcd_endpoint_reset(fh_otg_hcd, ep->hcpriv);
	}
	FH_SPINUNLOCK_IRQRESTORE(fh_otg_hcd->lock, flags);
}
#endif

/** Handles host mode interrupts for the FH_otg controller. Returns IRQ_NONE if
 * there was no interrupt to handle. Returns IRQ_HANDLED if there was a valid
 * interrupt.
 *
 * This function is called by the USB core when an interrupt occurs */
static irqreturn_t fh_otg_hcd_irq(struct usb_hcd *hcd)
{
	fh_otg_hcd_t *fh_otg_hcd = hcd_to_fh_otg_hcd(hcd);
	int32_t retval = fh_otg_hcd_handle_intr(fh_otg_hcd);
	if (retval != 0) {
		S3C2410X_CLEAR_EINTPEND();
	}
	return IRQ_RETVAL(retval);
}

/** Creates Status Change bitmap for the root hub and root port. The bitmap is
 * returned in buf. Bit 0 is the status change indicator for the root hub. Bit 1
 * is the status change indicator for the single root port. Returns 1 if either
 * change indicator is 1, otherwise returns 0. */
int hub_status_data(struct usb_hcd *hcd, char *buf)
{
	fh_otg_hcd_t *fh_otg_hcd = hcd_to_fh_otg_hcd(hcd);

	buf[0] = 0;
	buf[0] |= (fh_otg_hcd_is_status_changed(fh_otg_hcd, 1)) << 1;

	return (buf[0] != 0);
}

/** Handles hub class-specific requests. */
int hub_control(struct usb_hcd *hcd,
		u16 typeReq, u16 wValue, u16 wIndex, char *buf, u16 wLength)
{
	int retval;

	retval = fh_otg_hcd_hub_control(hcd_to_fh_otg_hcd(hcd),
					 typeReq, wValue, wIndex, buf, wLength);

	switch (retval) {
	case -FH_E_INVALID:
		retval = -EINVAL;
		break;
	}

	return retval;
}

#endif /* FH_DEVICE_ONLY */
