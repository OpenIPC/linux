/*
 * drivers/usb/gadget/s3c_udc_otg.c
 * Samsung S3C on-chip full/high speed USB OTG 2.0 device controllers
 *
 * Copyright (C) 2008 for Samsung Electronics
 *
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

#include "s3c_udc.h"
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <mach/map.h>
#include <plat/regs-otg.h>

#if	defined(CONFIG_USB_GADGET_S3C_OTGD_DMA_MODE) /* DMA mode */
#define OTG_DMA_MODE		1

#elif	defined(CONFIG_USB_GADGET_S3C_OTGD_SLAVE_MODE) /* Slave mode */
#define OTG_DMA_MODE		0
#error " Slave Mode is not implemented to do later"
#else
#error " Unknown S3C OTG operation mode, Select a correct operation mode"
#endif

#undef DEBUG_S3C_UDC_SETUP
#undef DEBUG_S3C_UDC_EP0
#undef DEBUG_S3C_UDC_ISR
#undef DEBUG_S3C_UDC_OUT_EP
#undef DEBUG_S3C_UDC_IN_EP
#undef DEBUG_S3C_UDC

//#define DEBUG_S3C_UDC_SETUP
//#define DEBUG_S3C_UDC_EP0
//#define DEBUG_S3C_UDC_ISR
//#define DEBUG_S3C_UDC_OUT_EP
//#define DEBUG_S3C_UDC_IN_EP
//#define DEBUG_S3C_UDC

#define EP0_CON		0
#define EP1_OUT		1
#define EP2_IN		2
#define EP3_IN		3
#define EP_MASK		0xF

#if defined(DEBUG_S3C_UDC_SETUP) || defined(DEBUG_S3C_UDC_ISR)\
	|| defined(DEBUG_S3C_UDC_OUT_EP)

static char *state_names[] = {
	"WAIT_FOR_SETUP",
	"DATA_STATE_XMIT",
	"DATA_STATE_NEED_ZLP",
	"WAIT_FOR_OUT_STATUS",
	"DATA_STATE_RECV",
	};
#endif

#ifdef DEBUG_S3C_UDC_SETUP
#define DEBUG_SETUP(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_SETUP(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC_EP0
#define DEBUG_EP0(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_EP0(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC
#define DEBUG(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC_ISR
#define DEBUG_ISR(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_ISR(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC_OUT_EP
#define DEBUG_OUT_EP(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_OUT_EP(fmt,args...) do {} while(0)
#endif

#ifdef DEBUG_S3C_UDC_IN_EP
#define DEBUG_IN_EP(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_IN_EP(fmt,args...) do {} while(0)
#endif


#define	DRIVER_DESC		"S3C HS USB OTG Device Driver, (c) 2008-2009 Samsung Electronics"
#define	DRIVER_VERSION		"15 March 2009"

struct s3c_udc	*the_controller;

static const char driver_name[] = "s3c-udc";
static const char driver_desc[] = DRIVER_DESC;
static const char ep0name[] = "ep0-control";

/* Max packet size*/
static unsigned int ep0_fifo_size = 64;
static unsigned int ep_fifo_size =  512;
static unsigned int ep_fifo_size2 = 1024;
static int reset_available = 1;

extern void otg_phy_init(void);
extern void otg_phy_off(void);
extern struct usb_ctrlrequest usb_ctrl;

/*
  Local declarations.
*/
static int s3c_ep_enable(struct usb_ep *ep, const struct usb_endpoint_descriptor *);
static int s3c_ep_disable(struct usb_ep *ep);
static struct usb_request *s3c_alloc_request(struct usb_ep *ep, gfp_t gfp_flags);
static void s3c_free_request(struct usb_ep *ep, struct usb_request *);

static int s3c_queue(struct usb_ep *ep, struct usb_request *, gfp_t gfp_flags);
static int s3c_dequeue(struct usb_ep *ep, struct usb_request *);
static int s3c_fifo_status(struct usb_ep *ep);
static void s3c_fifo_flush(struct usb_ep *ep);
static void s3c_ep0_read(struct s3c_udc *dev);
static void s3c_ep0_kick(struct s3c_udc *dev, struct s3c_ep *ep);
static void s3c_handle_ep0(struct s3c_udc *dev);
static int s3c_ep0_write(struct s3c_udc *dev);
static int write_fifo_ep0(struct s3c_ep *ep, struct s3c_request *req);
static void done(struct s3c_ep *ep, struct s3c_request *req, int status);
static void stop_activity(struct s3c_udc *dev, struct usb_gadget_driver *driver);
static int udc_enable(struct s3c_udc *dev);
static void udc_set_address(struct s3c_udc *dev, unsigned char address);
static void reconfig_usbd(void);
static void set_max_pktsize(struct s3c_udc *dev, enum usb_device_speed speed);
static void nuke(struct s3c_ep *ep, int status);
static int s3c_udc_set_halt(struct usb_ep *_ep, int value);

static struct usb_ep_ops s3c_ep_ops = {
	.enable = s3c_ep_enable,
	.disable = s3c_ep_disable,

	.alloc_request = s3c_alloc_request,
	.free_request = s3c_free_request,

	.queue = s3c_queue,
	.dequeue = s3c_dequeue,

	.set_halt = s3c_udc_set_halt,
	.fifo_status = s3c_fifo_status,
	.fifo_flush = s3c_fifo_flush,
};

#ifdef CONFIG_USB_GADGET_DEBUG_FILES

static const char proc_node_name[] = "driver/udc";

static int
udc_proc_read(char *page, char **start, off_t off, int count,
	      int *eof, void *_dev)
{
	char *buf = page;
	struct s3c_udc *dev = _dev;
	char *next = buf;
	unsigned size = count;
	unsigned long flags;
	int t;

	if (off != 0)
		return 0;

	local_irq_save(flags);

	/* basic device status */
	t = scnprintf(next, size,
		      DRIVER_DESC "\n"
		      "%s version: %s\n"
		      "Gadget driver: %s\n"
		      "\n",
		      driver_name, DRIVER_VERSION,
		      dev->driver ? dev->driver->driver.name : "(none)");
	size -= t;
	next += t;

	local_irq_restore(flags);
	*eof = 1;
	return count - size;
}

#define create_proc_files() \
	create_proc_read_entry(proc_node_name, 0, NULL, udc_proc_read, dev)
#define remove_proc_files() \
	remove_proc_entry(proc_node_name, NULL)

#else	/* !CONFIG_USB_GADGET_DEBUG_FILES */

#define create_proc_files() do {} while (0)
#define remove_proc_files() do {} while (0)

#endif	/* CONFIG_USB_GADGET_DEBUG_FILES */

#if	OTG_DMA_MODE /* DMA Mode */
#include "s3c_udc_otg_xfer_dma.c"

#else	/* Slave Mode */
#include "s3c_udc_otg_xfer_slave.c"
#endif

/*
 * 	udc_disable - disable USB device controller
 */
static void udc_disable(struct s3c_udc *dev)
{
	DEBUG_SETUP("%s: %p\n", __FUNCTION__, dev);

	udc_set_address(dev, 0);

	dev->ep0state = WAIT_FOR_SETUP;
	dev->gadget.speed = USB_SPEED_UNKNOWN;
	dev->usb_address = 0;

	otg_phy_off();
}

/*
 * 	udc_reinit - initialize software state
 */
static void udc_reinit(struct s3c_udc *dev)
{
	unsigned int i;

	DEBUG_SETUP("%s: %p\n", __FUNCTION__, dev);

	/* device/ep0 records init */
	INIT_LIST_HEAD(&dev->gadget.ep_list);
	INIT_LIST_HEAD(&dev->gadget.ep0->ep_list);
	dev->ep0state = WAIT_FOR_SETUP;

	/* basic endpoint records init */
	for (i = 0; i < S3C_MAX_ENDPOINTS; i++) {
		struct s3c_ep *ep = &dev->ep[i];

		if (i != 0)
			list_add_tail(&ep->ep.ep_list, &dev->gadget.ep_list);

		ep->desc = 0;
		ep->stopped = 0;
		INIT_LIST_HEAD(&ep->queue);
		ep->pio_irqs = 0;
	}

	/* the rest was statically initialized, and is read-only */
}

#define BYTES2MAXP(x)	(x / 8)
#define MAXP2BYTES(x)	(x * 8)

/* until it's enabled, this UDC should be completely invisible
 * to any USB host.
 */
static int udc_enable(struct s3c_udc *dev)
{
	DEBUG_SETUP("%s: %p\n", __FUNCTION__, dev);

	otg_phy_init();
	reconfig_usbd();

	DEBUG_SETUP("S3C USB 2.0 OTG Controller Core Initialized : 0x%x\n",
			readl(S3C_UDC_OTG_GINTMSK));

	dev->gadget.speed = USB_SPEED_UNKNOWN;

	return 0;
}

/*
  Register entry point for the peripheral controller driver.
*/
int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct s3c_udc *dev = the_controller;
	int retval;

	DEBUG_SETUP("%s: %s\n", __FUNCTION__, driver->driver.name);

	if (!driver
	    || (driver->speed != USB_SPEED_FULL && driver->speed != USB_SPEED_HIGH)
	    || !driver->bind
	    || !driver->unbind || !driver->disconnect || !driver->setup)
		return -EINVAL;
	if (!dev)
		return -ENODEV;
	if (dev->driver)
		return -EBUSY;

	/* first hook up the driver ... */
	dev->driver = driver;
	dev->gadget.dev.driver = &driver->driver;
	retval = device_add(&dev->gadget.dev);

	if(retval) { /* TODO */
		printk("target device_add failed, error %d\n", retval);
		return retval;
	}

	retval = driver->bind(&dev->gadget);
	if (retval) {
		printk("%s: bind to driver %s --> error %d\n", dev->gadget.name,
		       driver->driver.name, retval);
		device_del(&dev->gadget.dev);

		dev->driver = 0;
		dev->gadget.dev.driver = 0;
		return retval;
	}

	enable_irq(IRQ_OTG);

	printk("Registered gadget driver '%s'\n", driver->driver.name);
	udc_enable(dev);

	return 0;
}

EXPORT_SYMBOL(usb_gadget_register_driver);

/*
  Unregister entry point for the peripheral controller driver.
*/
int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct s3c_udc *dev = the_controller;
	unsigned long flags;

	if (!dev)
		return -ENODEV;
	if (!driver || driver != dev->driver)
		return -EINVAL;

	spin_lock_irqsave(&dev->lock, flags);
	dev->driver = 0;
	stop_activity(dev, driver);
	spin_unlock_irqrestore(&dev->lock, flags);

	driver->unbind(&dev->gadget);
	device_del(&dev->gadget.dev);

	disable_irq(IRQ_OTG);

	printk("Unregistered gadget driver '%s'\n", driver->driver.name);

	udc_disable(dev);

	return 0;
}

EXPORT_SYMBOL(usb_gadget_unregister_driver);

/*
 *	done - retire a request; caller blocked irqs
 */
static void done(struct s3c_ep *ep, struct s3c_request *req, int status)
{
	unsigned int stopped = ep->stopped;

	DEBUG("%s: %s %p, req = %p, stopped = %d\n",
		__FUNCTION__, ep->ep.name, ep, &req->req, stopped);

	list_del_init(&req->queue);

	if (likely(req->req.status == -EINPROGRESS)) {
		req->req.status = status;
	} else {
		status = req->req.status;
	}

	if (status && status != -ESHUTDOWN) {
		DEBUG("complete %s req %p stat %d len %u/%u\n",
			ep->ep.name, &req->req, status,
			req->req.actual, req->req.length);
	}

	/* don't modify queue heads during completion callback */
	ep->stopped = 1;

	spin_unlock(&ep->dev->lock);
	req->req.complete(&ep->ep, &req->req);
	spin_lock(&ep->dev->lock);

	ep->stopped = stopped;
}

/*
 * 	nuke - dequeue ALL requests
 */
static void nuke(struct s3c_ep *ep, int status)
{
	struct s3c_request *req;

	DEBUG("%s: %s %p\n", __FUNCTION__, ep->ep.name, ep);

	/* called with irqs blocked */
	while (!list_empty(&ep->queue)) {
		req = list_entry(ep->queue.next, struct s3c_request, queue);
		done(ep, req, status);
	}
}

static void stop_activity(struct s3c_udc *dev,
			  struct usb_gadget_driver *driver)
{
	int i;

	/* don't disconnect drivers more than once */
	if (dev->gadget.speed == USB_SPEED_UNKNOWN)
		driver = 0;
	dev->gadget.speed = USB_SPEED_UNKNOWN;

	/* prevent new request submissions, kill any outstanding requests  */
	for (i = 0; i < S3C_MAX_ENDPOINTS; i++) {
		struct s3c_ep *ep = &dev->ep[i];
		ep->stopped = 1;
		nuke(ep, -ESHUTDOWN);
	}

	/* report disconnect; the driver is already quiesced */
	if (driver) {
		spin_unlock(&dev->lock);
		driver->disconnect(&dev->gadget);
		spin_lock(&dev->lock);
	}

	/* re-init driver-visible data structures */
	udc_reinit(dev);
}

static void reconfig_usbd(void)
{
	/* 2. Soft-reset OTG Core and then unreset again. */
#ifdef DED_TX_FIFO
	int i;
#endif
	unsigned int uTemp = writel(CORE_SOFT_RESET, S3C_UDC_OTG_GRSTCTL);

	writel(	0<<15		/* PHY Low Power Clock sel*/
		|1<<14		/* Non-Periodic TxFIFO Rewind Enable*/
		|0x5<<10	/* Turnaround time*/
		|0<<9|0<<8	/* [0:HNP disable, 1:HNP enable][ 0:SRP disable, 1:SRP enable] H1= 1,1*/
		|0<<7		/* Ulpi DDR sel*/
		|0<<6		/* 0: high speed utmi+, 1: full speed serial*/
		|0<<4		/* 0: utmi+, 1:ulpi*/
		|1<<3		/* phy i/f  0:8bit, 1:16bit*/
		|0x7<<0,	/* HS/FS Timeout**/
		S3C_UDC_OTG_GUSBCFG);

	/* 3. Put the OTG device core in the disconnected state.*/
	uTemp = readl(S3C_UDC_OTG_DCTL);
	uTemp |= SOFT_DISCONNECT;
	writel(uTemp, S3C_UDC_OTG_DCTL);

	udelay(20);

	/* 4. Make the OTG device core exit from the disconnected state.*/
	uTemp = readl(S3C_UDC_OTG_DCTL);
	uTemp = uTemp & ~SOFT_DISCONNECT;
	writel(uTemp, S3C_UDC_OTG_DCTL);

	/* 5. Configure OTG Core to initial settings of device mode.*/
	writel(1<<18|0x0<<0, S3C_UDC_OTG_DCFG);		/* [][1: full speed(30Mhz) 0:high speed]*/

	mdelay(1);

	/* 6. Unmask the core interrupts*/
	writel(GINTMSK_INIT, S3C_UDC_OTG_GINTMSK);

	/* 7. Set NAK bit of EP0, EP1, EP2*/
	writel(DEPCTL_EPDIS|DEPCTL_SNAK|(0<<0), S3C_UDC_OTG_DOEPCTL(EP0_CON));
	writel(DEPCTL_EPDIS|DEPCTL_SNAK|(0<<0), S3C_UDC_OTG_DIEPCTL(EP0_CON));

	/* 8. Unmask EPO interrupts*/
	writel( ((1<<EP0_CON)<<DAINT_OUT_BIT)|(1<<EP0_CON), S3C_UDC_OTG_DAINTMSK);

	/* 9. Unmask device OUT EP common interrupts*/
	writel(DOEPMSK_INIT, S3C_UDC_OTG_DOEPMSK);

	/* 10. Unmask device IN EP common interrupts*/
	writel(DIEPMSK_INIT, S3C_UDC_OTG_DIEPMSK);

	/* 11. Set Rx FIFO Size*/
	writel(RX_FIFO_SIZE, S3C_UDC_OTG_GRXFSIZ);

	/* 12. Set Non Periodic Tx FIFO Size*/
	writel(NPTX_FIFO_SIZE<<16| NPTX_FIFO_START_ADDR<<0, S3C_UDC_OTG_GNPTXFSIZ);

#ifdef DED_TX_FIFO
	for (i = 1; i < S3C_MAX_ENDPOINTS; i++)
		writel(NPTX_FIFO_SIZE << 16 |
			(NPTX_FIFO_START_ADDR + NPTX_FIFO_SIZE + PTX_FIFO_SIZE*(i-1)) << 0,
			S3C_UDC_OTG_DIEPTXF(i));
#endif

	/* 13. Clear NAK bit of EP0, EP1, EP2*/
	/* For Slave mode*/
	writel(DEPCTL_EPDIS|DEPCTL_CNAK|(0<<0), S3C_UDC_OTG_DOEPCTL(EP0_CON)); /* EP0: Control OUT */

	/* 14. Initialize OTG Link Core.*/
	writel(GAHBCFG_INIT, S3C_UDC_OTG_GAHBCFG);

}

static void set_max_pktsize(struct s3c_udc *dev, enum usb_device_speed speed)
{
	unsigned int ep_ctrl;

	if (speed == USB_SPEED_HIGH) {
		ep0_fifo_size = 64;
		ep_fifo_size = 512;
		ep_fifo_size2 = 1024;
		dev->gadget.speed = USB_SPEED_HIGH;
	} else {
		ep0_fifo_size = 64;
		ep_fifo_size = 64;
		ep_fifo_size2 = 64;
		dev->gadget.speed = USB_SPEED_FULL;
	}

	dev->ep[0].ep.maxpacket = ep0_fifo_size;
	dev->ep[1].ep.maxpacket = ep_fifo_size;
	dev->ep[2].ep.maxpacket = ep_fifo_size;
	dev->ep[3].ep.maxpacket = ep_fifo_size;
	dev->ep[4].ep.maxpacket = ep_fifo_size;
	dev->ep[5].ep.maxpacket = ep_fifo_size2;
	dev->ep[6].ep.maxpacket = ep_fifo_size2;
	dev->ep[7].ep.maxpacket = ep_fifo_size2;
	dev->ep[8].ep.maxpacket = ep_fifo_size2;


	/* EP0 - Control IN (64 bytes)*/
	ep_ctrl = readl(S3C_UDC_OTG_DIEPCTL(EP0_CON));
	writel(ep_ctrl|(0<<0), S3C_UDC_OTG_DIEPCTL(EP0_CON));

	/* EP0 - Control OUT (64 bytes)*/
	ep_ctrl = readl(S3C_UDC_OTG_DOEPCTL(EP0_CON));
	writel(ep_ctrl|(0<<0), S3C_UDC_OTG_DOEPCTL(EP0_CON));
}

static int s3c_ep_enable(struct usb_ep *_ep,
			     const struct usb_endpoint_descriptor *desc)
{
	struct s3c_ep *ep;
	struct s3c_udc *dev;
	unsigned long flags;

	DEBUG("%s: %p\n", __FUNCTION__, _ep);

	ep = container_of(_ep, struct s3c_ep, ep);
	if (!_ep || !desc || ep->desc || _ep->name == ep0name
	    || desc->bDescriptorType != USB_DT_ENDPOINT
	    || ep->bEndpointAddress != desc->bEndpointAddress
	    || ep_maxpacket(ep) < le16_to_cpu(desc->wMaxPacketSize)) {

		DEBUG("%s: bad ep or descriptor\n", __FUNCTION__);
		return -EINVAL;
	}

	/* xfer types must match, except that interrupt ~= bulk */
	if (ep->bmAttributes != desc->bmAttributes
	    && ep->bmAttributes != USB_ENDPOINT_XFER_BULK
	    && desc->bmAttributes != USB_ENDPOINT_XFER_INT) {

		DEBUG("%s: %s type mismatch\n", __FUNCTION__, _ep->name);
		return -EINVAL;
	}

	/* hardware _could_ do smaller, but driver doesn't */
	if ((desc->bmAttributes == USB_ENDPOINT_XFER_BULK
	     && le16_to_cpu(desc->wMaxPacketSize) != ep_maxpacket(ep))
	    || !desc->wMaxPacketSize) {

		DEBUG("%s: bad %s maxpacket\n", __FUNCTION__, _ep->name);
		return -ERANGE;
	}

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN) {

		DEBUG("%s: bogus device state\n", __FUNCTION__);
		return -ESHUTDOWN;
	}

	ep->stopped = 0;
	ep->desc = desc;
	ep->pio_irqs = 0;
	ep->ep.maxpacket = le16_to_cpu(desc->wMaxPacketSize);

	/* Reset halt state */
	s3c_udc_set_halt(_ep, 0);

	spin_lock_irqsave(&ep->dev->lock, flags);
	s3c_udc_ep_activate(ep);
	spin_unlock_irqrestore(&ep->dev->lock, flags);

	DEBUG("%s: enabled %s, stopped = %d, maxpacket = %d\n",
		__FUNCTION__, _ep->name, ep->stopped, ep->ep.maxpacket);
	return 0;
}

/** Disable EP
 */
static int s3c_ep_disable(struct usb_ep *_ep)
{
	struct s3c_ep *ep;
	unsigned long flags;

	DEBUG("%s: %p\n", __FUNCTION__, _ep);

	ep = container_of(_ep, struct s3c_ep, ep);
	if (!_ep || !ep->desc) {
		DEBUG("%s: %s not enabled\n", __FUNCTION__,
		      _ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	spin_lock_irqsave(&ep->dev->lock, flags);

	/* Nuke all pending requests */
	nuke(ep, -ESHUTDOWN);

	ep->desc = 0;
	ep->stopped = 1;

	spin_unlock_irqrestore(&ep->dev->lock, flags);

	DEBUG("%s: disabled %s\n", __FUNCTION__, _ep->name);
	return 0;
}

static struct usb_request *s3c_alloc_request(struct usb_ep *ep,
						 gfp_t gfp_flags)
{
	struct s3c_request *req;

	DEBUG("%s: %s %p\n", __FUNCTION__, ep->name, ep);

	req = kmalloc(sizeof *req, gfp_flags);
	if (!req)
		return 0;

	memset(req, 0, sizeof *req);
	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

static void s3c_free_request(struct usb_ep *ep, struct usb_request *_req)
{
	struct s3c_request *req;

	DEBUG("%s: %p\n", __FUNCTION__, ep);

	req = container_of(_req, struct s3c_request, req);
	WARN_ON(!list_empty(&req->queue));
	kfree(req);
}

/* dequeue JUST ONE request */
static int s3c_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct s3c_ep *ep;
	struct s3c_request *req;
	unsigned long flags;

	DEBUG("%s: %p\n", __FUNCTION__, _ep);

	ep = container_of(_ep, struct s3c_ep, ep);
	if (!_ep || ep->ep.name == ep0name)
		return -EINVAL;

	spin_lock_irqsave(&ep->dev->lock, flags);

	/* make sure it's actually queued on this endpoint */
	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req)
			break;
	}
	if (&req->req != _req) {
		spin_unlock_irqrestore(&ep->dev->lock, flags);
		return -EINVAL;
	}

	done(ep, req, -ECONNRESET);

	spin_unlock_irqrestore(&ep->dev->lock, flags);
	return 0;
}

/** Return bytes in EP FIFO
 */
static int s3c_fifo_status(struct usb_ep *_ep)
{
	int count = 0;
	struct s3c_ep *ep;

	ep = container_of(_ep, struct s3c_ep, ep);
	if (!_ep) {
		DEBUG("%s: bad ep\n", __FUNCTION__);
		return -ENODEV;
	}

	DEBUG("%s: %d\n", __FUNCTION__, ep_index(ep));

	/* LPD can't report unclaimed bytes from IN fifos */
	if (ep_is_in(ep))
		return -EOPNOTSUPP;

	return count;
}

/** Flush EP FIFO
 */
static void s3c_fifo_flush(struct usb_ep *_ep)
{
	struct s3c_ep *ep;

	ep = container_of(_ep, struct s3c_ep, ep);
	if (unlikely(!_ep || (!ep->desc && ep->ep.name != ep0name))) {
		DEBUG("%s: bad ep\n", __FUNCTION__);
		return;
	}

	DEBUG("%s: %d\n", __FUNCTION__, ep_index(ep));
}

/* ---------------------------------------------------------------------------
 * 	device-scoped parts of the api to the usb controller hardware
 * ---------------------------------------------------------------------------
 */

static int s3c_udc_get_frame(struct usb_gadget *_gadget)
{
	/*fram count number [21:8]*/
	unsigned int frame = readl(S3C_UDC_OTG_DSTS);

	DEBUG("%s: %p\n", __FUNCTION__, _gadget);
	return (frame & 0x3ff00);
}

static int s3c_udc_wakeup(struct usb_gadget *_gadget)
{
	DEBUG("%s: %p\n", __FUNCTION__, _gadget);
	return -ENOTSUPP;
}

static const struct usb_gadget_ops s3c_udc_ops = {
	.get_frame = s3c_udc_get_frame,
	.wakeup = s3c_udc_wakeup,
	/* current versions must always be self-powered */
};

static void nop_release(struct device *dev)
{
	DEBUG("%s %s\n", __FUNCTION__, dev->bus_id);
}

static struct s3c_udc memory = {
	.usb_address = 0,

	.gadget = {
		   .ops = &s3c_udc_ops,
		   .ep0 = &memory.ep[0].ep,
		   .name = driver_name,
		   .dev = {
			   .bus_id = "gadget",
			   .release = nop_release,
			   },
		   },

	/* control endpoint */
	.ep[0] = {
		  .ep = {
			 .name = ep0name,
			 .ops = &s3c_ep_ops,
			 .maxpacket = EP0_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = 0,
		  .bmAttributes = 0,

		  .ep_type = ep_control,
		  .fifo = (unsigned int) S3C_UDC_OTG_EP0_FIFO,
		  },

	/* first group of endpoints */
	.ep[1] = {
		  .ep = {
			 .name = "ep1-bulk",
			 .ops = &s3c_ep_ops,
			 .maxpacket = EP_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = 1,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,

		  .ep_type = ep_bulk_out,
		  .fifo = (unsigned int) S3C_UDC_OTG_EP1_FIFO,
		  },

	.ep[2] = {
		  .ep = {
			 .name = "ep2-bulk",
			 .ops = &s3c_ep_ops,
			 .maxpacket = EP_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 2,
		  .bmAttributes = USB_ENDPOINT_XFER_BULK,

		  .ep_type = ep_bulk_in,
		  .fifo = (unsigned int) S3C_UDC_OTG_EP2_FIFO,
		  },

	.ep[3] = {				/* Though NOT USED XXX*/
		  .ep = {
			 .name = "ep3-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = EP_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 3,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .ep_type = ep_interrupt,
		  .fifo = (unsigned int) S3C_UDC_OTG_EP3_FIFO,
		  },
	.ep[4] = {				/* Though NOT USED XXX*/
		  .ep = {
			 .name = "ep4-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = EP_FIFO_SIZE,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 4,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .ep_type = ep_interrupt,
		  .fifo = (unsigned int) S3C_UDC_OTG_EP4_FIFO,
		  },
	.ep[5] = {				/* Though NOT USED XXX*/
		  .ep = {
			 .name = "ep5-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = EP_FIFO_SIZE2,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 5,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .ep_type = ep_interrupt,
		  .fifo = (unsigned int) S3C_UDC_OTG_EP5_FIFO,
		  },
	.ep[6] = {				/* Though NOT USED XXX*/
		  .ep = {
			 .name = "ep6-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = EP_FIFO_SIZE2,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 6,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .ep_type = ep_interrupt,
		  .fifo = (unsigned int) S3C_UDC_OTG_EP6_FIFO,
		  },
	.ep[7] = {				/* Though NOT USED XXX*/
		  .ep = {
			 .name = "ep7-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = EP_FIFO_SIZE2,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 7,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .ep_type = ep_interrupt,
		  .fifo = (unsigned int) S3C_UDC_OTG_EP7_FIFO,
		  },
	.ep[8] = {				/* Though NOT USED XXX*/
		  .ep = {
			 .name = "ep8-int",
			 .ops = &s3c_ep_ops,
			 .maxpacket = EP_FIFO_SIZE2,
			 },
		  .dev = &memory,

		  .bEndpointAddress = USB_DIR_IN | 8,
		  .bmAttributes = USB_ENDPOINT_XFER_INT,

		  .ep_type = ep_interrupt,
		  .fifo = (unsigned int) S3C_UDC_OTG_EP8_FIFO,
		  },
};

/*
 * 	probe - binds to the platform device
 */
static struct clk	*otg_clock = NULL;

static int s3c_udc_probe(struct platform_device *pdev)
{
	struct s3c_udc *dev = &memory;
	int retval;

	DEBUG("%s: %p\n", __FUNCTION__, pdev);

	spin_lock_init(&dev->lock);
	dev->dev = pdev;

	device_initialize(&dev->gadget.dev);
	dev->gadget.dev.parent = &pdev->dev;

	dev->gadget.is_dualspeed = 1;	/* Hack only*/
	dev->gadget.is_otg = 0;
	dev->gadget.is_a_peripheral = 0;
	dev->gadget.b_hnp_enable = 0;
	dev->gadget.a_hnp_support = 0;
	dev->gadget.a_alt_hnp_support = 0;

	the_controller = dev;
	platform_set_drvdata(pdev, dev);

	otg_clock = clk_get(&pdev->dev, "otg");
	if (otg_clock == NULL) {
		printk(KERN_INFO "failed to find otg clock source\n");
		return -ENOENT;
	}
	clk_enable(otg_clock);

	udc_reinit(dev);

	local_irq_disable();

	/* irq setup after old hardware state is cleaned up */
	retval =
	    request_irq(IRQ_OTG, s3c_udc_irq, 0, driver_name, dev);

	if (retval != 0) {
		DEBUG(KERN_ERR "%s: can't get irq %i, err %d\n", driver_name,
		      IRQ_OTG, retval);
		return -EBUSY;
	}

	disable_irq(IRQ_OTG);
	local_irq_enable();
	create_proc_files();

	return retval;
}

static int s3c_udc_remove(struct platform_device *pdev)
{
	struct s3c_udc *dev = platform_get_drvdata(pdev);

	DEBUG("%s: %p\n", __FUNCTION__, pdev);

	if (otg_clock != NULL) {
		clk_disable(otg_clock);
		clk_put(otg_clock);
		otg_clock = NULL;
	}

	remove_proc_files();
	usb_gadget_unregister_driver(dev->driver);

	free_irq(IRQ_OTG, dev);

	platform_set_drvdata(pdev, 0);

	the_controller = 0;

	return 0;
}

#ifdef CONFIG_PM
static int s3c_udc_suspend(struct platform_device *pdev, pm_message_t state)
{
        struct s3c_udc *dev = the_controller;
        int i;

        if (dev->driver) {
                if (dev->driver->suspend)
                        dev->driver->suspend(&dev->gadget);

                /* Terminate any outstanding requests  */
                for (i = 0; i < S3C_MAX_ENDPOINTS; i++) {
                        struct s3c_ep *ep = &dev->ep[i];
                        if ( ep->dev != NULL )
                                spin_lock(&ep->dev->lock);
                        ep->stopped = 1;
                        nuke(ep, -ESHUTDOWN);
                        if ( ep->dev != NULL )
                                spin_unlock(&ep->dev->lock);
                }

                disable_irq(IRQ_OTG);
                udc_disable(dev);
                clk_disable(otg_clock);
        }

        return 0;
}

static int s3c_udc_resume(struct platform_device *pdev)
{
        struct s3c_udc *dev = the_controller;

        if (dev->driver) {
                clk_enable(otg_clock);
                udc_reinit(dev);
                enable_irq(IRQ_OTG);
                udc_enable(dev);

                if (dev->driver->resume)
                        dev->driver->resume(&dev->gadget);
        }

        return 0;
}
#else
#define s3c_udc_suspend NULL
#define s3c_udc_resume  NULL
#endif /* CONFIG_PM */

/*-------------------------------------------------------------------------*/
static struct platform_driver s3c_udc_driver = {
	.probe		= s3c_udc_probe,
	.remove		= s3c_udc_remove,
	.suspend	= s3c_udc_suspend,
	.resume		= s3c_udc_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-usbgadget",
	},
};

static int __init udc_init(void)
{
	int ret;

	ret = platform_driver_register(&s3c_udc_driver);
	if(!ret)
	   printk("%s : %s\n"
	   	  "%s : version %s %s \n",
	   	  driver_name, DRIVER_DESC,
	   	  driver_name, DRIVER_VERSION, OTG_DMA_MODE? "(DMA Mode)" : "(Slave Mode)");

	return ret;
}

static void __exit udc_exit(void)
{
	platform_driver_unregister(&s3c_udc_driver);
	printk("Unloaded %s version %s\n", driver_name, DRIVER_VERSION);
}

module_init(udc_init);
module_exit(udc_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Samsung");
MODULE_LICENSE("GPL");
