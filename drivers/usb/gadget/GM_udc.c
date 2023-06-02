/*******************************************************************************
 * Module name: fotg2xx_udc.c
 *
 * Copyright 2006 GM as an unpublished work.
 * All Rights Reserved.
 *
 * The Information contained herein is confidential property of Company.
 * The user, copying, transfer or disclosure of such Information is
 * prohibited except by express written agreement with Company.
 *
 * Written on 2006/6 by Elliot Hou Au-ping.
 *
 * Module Description:
 *  This OTG UDC dirver for GM FOTG220 controller
 *
 ******************************************************************************/
#ifdef CONFIG_USB_DEBUG
        #define DEBUG
#else
        #undef DEBUG
#endif
        #define DEBUG
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/usb/otg.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/moduleparam.h>
#include <linux/dma-mapping.h>
#include <linux/cdev.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <mach/ftpmu010.h>

#include "GM_udc.h"
//#include "../host/fotg2xx_opt-macro.h"

#if defined(CONFIG_PLATFORM_GM8210)
#include <mach/ftpmu010_pcie.h>
#endif
#if defined(CONFIG_PLATFORM_GM8136)
#include <mach/platform/board.h>
#endif

extern void fotg200_handle_irq(int irq);
extern u32 get_fotg2xx_va(int num);
extern u32 get_fotg2xx_irq(int num);
extern int usb_get_pmu_fd(void);

//#define CONFIG_GM_OTG_CHOOSE 0
#define USB_USER_NAME "usb_gadget"


#define	DRIVER_DESC	"FOTG2XX USB Device Controller"
#define	DRIVER_VERSION	"04-Oct 2004"

#define USB_NOTIFY
#undef USB_NOTIFY
/* Used for Signal to User Process */
static dev_t dev_num;
static struct cdev usb_cdev;
static struct class *usb_class = NULL;
static pid_t pid = 0;

static const char driver_name [] = "fotg2xx_udc";
static const char driver_desc [] = DRIVER_DESC;

static char *names [] = {"ep0","ep1-bulkin","ep2-bulkout","ep3-intin","ep4-intout","ep5","ep6","ep7","ep8","ep9","ep10" };
#define BULK_IN_EP_NUM		1
#define BULK_OUT_EP_NUM		2
#define INTR_IN_EP_NUM		3
#define INTR_OUT_EP_NUM		4

static struct FTC_udc *fotg210_udc = NULL;

/*-------------------------------------------------------------------------*/

static void nuke(struct FTC_ep *, int status);

///////////////////////////////////////////////////////////////////////
// Enable endpoint
// EP0 : has been enabled while driver booting up
// Need to give this EP's descriptor
static int FTC_ep_enable(struct usb_ep *_ep, const struct usb_endpoint_descriptor *desc)
{
	struct FTC_udc	*dev;
	struct FTC_ep	*ep;
	u16	max;
	unsigned long flags;

	ep = container_of(_ep, struct FTC_ep, ep);

	DBG_CTRLL("+FTC_ep_enable() : _ep = %x desc = %x ep->desc= %x\n",(u32) _ep, (u32) desc, (u32) ep->desc);

	// check input variable, if there ia any variable undefined, return false
	if (!_ep || !desc || ep->desc
			 || desc->bDescriptorType != USB_DT_ENDPOINT) {
		return -EINVAL;
	}

	// if this is used to enable ep0, return false
	dev = ep->dev;
	if (ep == &dev->ep[0]) {  //no EP0 need to be enabled
		return -EINVAL;
	}

	// if upper level driver not ready or device speed unknown, return false
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN) {
		return -EINVAL;
	}

	if (ep->num != (desc->bEndpointAddress & 0x0f)) {
		return -EINVAL;
	}

	// EP should be Bulk or intr
	switch (desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
		case USB_ENDPOINT_XFER_BULK:
		case USB_ENDPOINT_XFER_INT:
			break;
		default:
			return -EINVAL;
	}

	/* enabling the no-toggle interrupt mode would need an api hook */
	// max = le16_to_cpu(get_unaligned(&desc->wMaxPacketSize));
	 max = le16_to_cpu(desc->wMaxPacketSize);

	// 11/2/05' AHB_DMA
	// Only bulk use AHB_DMA, and not always use DMA, so change while running
	//if ((desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)
	//   ep->dma = 1;
	//else
	ep->dma = 0;		// right now we choice not to use dma

	ep->is_in = (USB_DIR_IN & desc->bEndpointAddress) != 0;

	spin_lock_irqsave(&ep->dev->lock, flags);

	ep->ep.maxpacket = max;
	ep->stopped = 0;
	ep->dma_running= FALSE;
	ep->desc = desc;
	spin_unlock_irqrestore(&ep->dev->lock, flags);

	printk("enable %s %s maxpacket %u\n", ep->ep.name,
		   ep->is_in ? "IN" : "OUT",
		   // ep->dma ? "dma" : "pio",
		   max);

#if defined(USB_NOTIFY)
	sys_kill(pid, SIGUSR1);
#endif

	return 0;
}

static void ep_reset(struct FTC_ep *ep)
{
	//struct FTC_udc		*dev = ep->dev;
	DBG_FUNCC("+ep_reset\n");

	ep->ep.maxpacket = MAX_FIFO_SIZE;
	ep->desc = 0;
	ep->stopped = 1;
	ep->irqs = 0;
	ep->dma = 0;
}

static int FTC_ep_disable(struct usb_ep *_ep)
{
	struct FTC_ep	*ep;
	struct FTC_udc	*dev;
	unsigned long	flags;

	DBG_FUNCC("+FTC_ep_disable()\n");

	ep = container_of(_ep, struct FTC_ep, ep);
	if (!_ep || !ep->desc)
		return -ENODEV;

	//printk("+FTC_ep_disable() : _ep = 0x%x ep->desc = 0x%x\n", _ep , ep->desc);
	dev = ep->dev;

	//John mark for in suspend will reset system
	//john if (dev->ep0state == EP0_SUSPEND)
	//john 	return -EBUSY;

	if (ep == &dev->ep[0])  //john no EP0 need to be enabled
		return -EINVAL;

	Vdbg(dev, "disable %s\n", _ep->name);

	spin_lock_irqsave(&dev->lock, flags);
	nuke(ep, -ESHUTDOWN);
	ep_reset(ep);

	spin_unlock_irqrestore(&dev->lock, flags);

	return 0;
}

/*-------------------------------------------------------------------------*/

static struct usb_request *FTC_alloc_request(struct usb_ep *_ep, unsigned gfp_flags)
{
    struct FTC_request	*req;

    DBG_FUNCC("+FTC_alloc_request\n");
    if (!_ep)
        return 0;

    req = kmalloc(sizeof *req, gfp_flags);
    if (!req)
        return 0;

    memset(req, 0, sizeof *req);
    req->req.dma = DMA_ADDR_INVALID;

    INIT_LIST_HEAD(&req->queue);

    DBG_FUNCC("-FTC_alloc_request\n");
    return &req->req;
}

static void FTC_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
	struct FTC_request	*req;

	DBG_FUNCC("+FTC_free_request()\n");

	if (!_ep || !_req)
		return;

	req = container_of(_req, struct FTC_request, req);
	WARN_ON(!list_empty(&req->queue));
	kfree(req);
}

/*-------------------------------------------------------------------------*/
// finish/abort one request
static void done(struct FTC_ep *ep, struct FTC_request *req, int status)
{
    struct FTC_udc		*dev;
    unsigned stopped = ep->stopped;

    DBG_FUNCC("+done()\n");

    list_del_init(&req->queue);

    if (likely(req->req.status == -EINPROGRESS))   // still ongoing
        req->req.status = status;
    else // has finished
        status = req->req.status;

    dev = ep->dev;

    if (req->mapped)  // DMA mapped
    {
        u32 temp;
        DBG_CTRLL("....dma_unmap_single len = %x dma =%x, dir=%x dev=%x\n",
                req->req.length,req->req.dma,ep->is_in,dev);

        // important : DMA length will set as 16*n bytes
        temp = (req->req.length + 15) / 16;
        temp = temp *16;
        dma_unmap_single((void *)dev, req->req.dma, temp,  //USB_EPX_BUFSIZ,
                ep->is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
        req->req.dma = DMA_ADDR_INVALID;
        req->mapped = 0;
    }

#ifndef USB_TRACE
    if (status && status != -ESHUTDOWN)
#endif
        Vdbg(dev, "complete %s req %p stat %d len %u/%u\n",
                ep->ep.name, &req->req, status,
                req->req.actual, req->req.length);

    /* don't modify queue heads during completion callback */
    if (status == -ESHUTDOWN)
        ep->stopped = 0;
    else
        ep->stopped = 1;

    spin_unlock(&dev->lock);

    req->req.complete(&ep->ep, &req->req);
    spin_lock(&dev->lock);

    if (ep->num==0)
        mUsbEP0DoneSet(ep->dev->va_base);

    ep->stopped = stopped;  //recover

    DBG_FUNCC("-done() stopped=%d\n",stopped);
}


/* dequeue ALL requests */
static void nuke(struct FTC_ep *ep, int status)
{
	struct FTC_request	*req;
	DBG_FUNCC("+nuke() ep addr= 0x%x\n", (u32) ep);

	ep->stopped = 1;
	if (list_empty(&ep->queue))
		return;
	while (!list_empty(&ep->queue))
	{
		req = list_entry(ep->queue.next, struct FTC_request, queue);
		done(ep, req, status);
	}
}


static int FTC_queue(struct usb_ep *_ep, struct usb_request *_req, unsigned gfp_flags);

/* dequeue JUST ONE request */
static int FTC_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
    struct FTC_request	*req;
    struct FTC_ep		*ep;
    struct FTC_udc		*dev;
    unsigned long		flags;

    DBG_FUNCC("+FTC_dequeue()\n");

    ep = container_of(_ep, struct FTC_ep, ep);
    if (!_ep || !_req || (!ep->desc && ep->num != 0))
        return -EINVAL;
    dev = ep->dev;
    if (!dev->driver)
        return -ESHUTDOWN;

    /* we can't touch (dma) registers when suspended */
    if (dev->ep0state == EP0_SUSPEND)
        return -EBUSY;

    Vdbg(dev, "%s %s %s %s %p\n", __FUNCTION__, _ep->name,
            ep->is_in ? "IN" : "OUT",
            ep->dma ? "dma" : "pio",
            _req);

    spin_lock_irqsave(&dev->lock, flags);

    /* make sure it's actually queued on this endpoint */
    list_for_each_entry (req, &ep->queue, queue)
    {
        if (&req->req == _req)
            break;
    }
    if (&req->req != _req)
    {
        spin_unlock_irqrestore (&dev->lock, flags);
        return -EINVAL;
    }

    if (!list_empty(&req->queue))
    {
        done(ep, req, -ECONNRESET);
    }
    else
        req = 0;

    spin_unlock_irqrestore(&dev->lock, flags);

    return req ? 0 : -EOPNOTSUPP;
}

/*-------------------------------------------------------------------------*/

static void FTC_clear_halt(struct FTC_ep *ep)
{
    DBG_FUNCC("+FTC_clear_halt()(ep->num=%d)\n",ep->num);

    // assert (ep->num !=0)
    Vdbg(ep->dev, "%s clear halt\n", ep->ep.name);
    if (ep->num == 0)
    {
        ep->dev->ep0state = EP0_IDLE;
        ep->dev->ep[0].stopped = 0;
    }
    else
    {
        if (ep->is_in)	// IN direction ?
        {
            DBG_CTRLL("FTC_udc==>FTC_clear_halt()==>IN direction, EP%d \n",ep->num);
            mUsbEPinRsTgSet(ep->dev->va_base,ep->num);	// Set Rst_Toggle Bit
            mUsbEPinRsTgClr(ep->dev->va_base,ep->num);	// Clear Rst_Toggle Bit
            mUsbEPinStallClr(ep->dev->va_base,ep->num);	// Clear Stall Bit
        }
        else
        {
            DBG_CTRLL("FTC_udc==>FTC_clear_halt()==>OUT direction, EP%d \n",ep->num);
            mUsbEPoutRsTgSet(ep->dev->va_base,ep->num);	// Set Rst_Toggle Bit
            mUsbEPoutRsTgClr(ep->dev->va_base,ep->num);	// Clear Rst_Toggle Bit
            mUsbEPoutStallClr(ep->dev->va_base,ep->num);	// Clear Stall Bit
        }
    }
    DBG_CTRLL("FTC_udc==>FTC_clear_halt()==>ep->stopped = %d\n",ep->stopped);

    if (ep->stopped)
    {
        ep->stopped = 0;
    }
}

static int FTC_set_halt(struct usb_ep *_ep, int value)
{
    struct FTC_ep	*ep;
    unsigned long	flags;
    int		retval = 0;

    DBG_FUNCC("+FTC_set_halt()\n");
    if (!_ep)
        return -ENODEV;
    ep = container_of (_ep, struct FTC_ep, ep);

    DBG_BULKK("FTC_set_halt()===> (ep->num=%d)(Value=%d)\n",ep->num,value);

    //*********** Process the EP-0 SetHalt *******************
    if (ep->num == 0) {
        if (value == PROTO_STALL) {    // protocol stall, need H/W to reset
            mUsbEP0StallSet(ep->dev->va_base);
        } else if (value == FUNC_STALL) {   // function stall, SW to set/clear, nad EP0 work normally
            ep->dev->ep0state = EP0_STALL;
            ep->dev->ep[0].stopped = 1;
        } else if (value == CLEAR_STALL) {    // clear function stall, SW to set/clear, nad EP0 work normally
            ep->dev->ep0state = EP0_IDLE;
            ep->dev->ep[0].stopped = 0;
        }
        return retval; //EP0 Set Halt will return here

    } else if (!ep->desc) { /* don't change EPxSTATUS_EP_INVALID to READY */
        Info(ep->dev, "%s %s inactive?\n", __FUNCTION__, ep->ep.name);
        return -EINVAL;
    }

    //*********** Process the EP-X SetHalt *******************
    spin_lock_irqsave(&ep->dev->lock, flags);

    if (!list_empty(&ep->queue)) {   // something in queue
        retval = -EAGAIN;
    } else if (!value) {
        FTC_clear_halt(ep);
    } else {
        ep->stopped = 1;
        Vdbg(ep->dev, "%s set halt\n", ep->ep.name);

        if (ep->is_in) { // IN direction ?
            printk("set in stall bit\n");
            mUsbEPinStallSet(ep->dev->va_base,ep->num); // Set in Stall Bit
        } else {
            printk("set out stall bit\n");
            mUsbEPoutStallSet(ep->dev->va_base,ep->num); // Set out Stall Bit
        }
    }
    spin_unlock_irqrestore(&ep->dev->lock, flags);
    return retval;
}



//********************************************************
//Name: FTC_fifo_status
//Description:
//
//********************************************************
static int FTC_fifo_status(struct usb_ep *_ep)
{
	struct FTC_ep *ep;
	u8     u8fifo_n;   //john
	u32	   size;

	DBG_FUNCC("+FTC_fifo_status()\n");

	if (!_ep)
		return -ENODEV;
	ep = container_of(_ep, struct FTC_ep, ep);

	DBG_BULKK("FTC_udc-->FTC_fifo_status-->Check (size is only reported sanely for OUT)");
	/* size is only reported sanely for OUT */
	if (ep->is_in)
	{
		DBG_BULKK("FTC_udc-->FTC_fifo_status-->return -EOPNOTSUPP (ep->is_in)");
		return -EOPNOTSUPP;
	}

	//John for FUSB220
	if (ep->num ==0)
	{   	//EP0
		// note : for EP0, only know empty or not
		size = !mUsbEP0EMPFIFO(ep->dev->va_base);
	}
	else
	{
		DBG_BULKK("FTC_udc-->FTC_fifo_status-->ep->num >0 ");

		u8fifo_n = mUsbEPMapRd(ep->dev->va_base,ep->num); // get the relatived FIFO number
		if (ep->is_in)
			u8fifo_n &= 0x0F;
		else
			u8fifo_n >>= 4;
		if (u8fifo_n >= FUSB220_MAX_FIFO) // over the Max. fifo count ?
			return -ENOBUFS;

		// Check the FIFO had been enable ?
		if ((mUsbFIFOConfigRd(ep->dev->va_base,u8fifo_n) & 0x80) == 0)
			return -ENOBUFS;

		size = mUsbFIFOOutByteCount(ep->dev->va_base,u8fifo_n);
		Vdbg(ep->dev, "%s %s %u\n", __FUNCTION__, ep->ep.name, size);
	}
	return size;
}

static void FTC_fifo_flush(struct usb_ep *_ep)
{
    struct FTC_ep *ep;
    u8     u8fifo_n;   //john

    DBG_FUNCC("+FTC_fifo_flush()\n");

    if (!_ep)
        return;
    ep = container_of(_ep, struct FTC_ep, ep);
    Vdbg(ep->dev, "%s %s\n", __FUNCTION__, ep->ep.name);

    /* don't change EPxSTATUS_EP_INVALID to READY */
    if (!ep->desc && ep->num != 0)
    {
        udc_dbg(ep->dev, "%s %s inactive?\n", __FUNCTION__, ep->ep.name);
        return;
    }

    //John for FUSB220
    if (ep->num ==0)
    {   //EP0
        mUsbEP0ClearFIFO(ep->dev->va_base);
    }
    else
    {
        u8fifo_n = mUsbEPMapRd(ep->dev->va_base,ep->num); // get the relatived FIFO number
        if (ep->is_in)
            u8fifo_n &= 0x0F;
        else
            u8fifo_n >>= 4;
        if (u8fifo_n >= FUSB220_MAX_FIFO) // over the Max. fifo count ?
            return;

        // Check the FIFO had been enable ?
        if ((mUsbFIFOConfigRd(ep->dev->va_base,u8fifo_n) & 0x80) == 0)
            return;

        mUsbFIFOReset(ep->dev->va_base,u8fifo_n); //reset FIFO
        udelay(10);
        mUsbFIFOResetOK(ep->dev->va_base,u8fifo_n); //reset FIFO finish
    }
    return;
}

static struct usb_ep_ops FTC_ep_ops =
{
	.enable	        = FTC_ep_enable,
	.disable	    = FTC_ep_disable,

	.alloc_request	= FTC_alloc_request,
	.free_request	= FTC_free_request,

	.queue	        = FTC_queue,
	.dequeue	    = FTC_dequeue,

	.set_halt	    = FTC_set_halt,

	.fifo_status    = FTC_fifo_status,
	.fifo_flush	    = FTC_fifo_flush,
};

/*-------------------------------------------------------------------------*/

static int FTC_get_frame(struct usb_gadget *_gadget)
{
	struct FTC_udc	*dev;
	u16 retval;
	unsigned long	flags;

	DBG_FUNCC("+FTC_get_frame()\n");

	if (!_gadget)
		return -ENODEV;
	dev = container_of (_gadget, struct FTC_udc, gadget);
	spin_lock_irqsave (&dev->lock, flags);
	retval = ( (mUsbFrameNoHigh(dev->va_base) & 0x07) <<8) | mUsbFrameNoLow(dev->va_base);
	spin_unlock_irqrestore (&dev->lock, flags);

	return retval;
}

static int FTC_wakeup(struct usb_gadget *_gadget)
{
	struct FTC_udc	*dev;
	unsigned long	flags;
	DBG_FUNCC("+FTC_wakeup()\n");

	if (!_gadget)
		return -ENODEV;
	dev = container_of (_gadget, struct FTC_udc, gadget);
	spin_lock_irqsave (&dev->lock, flags);

	// Set "Device_Remote_Wakeup", Turn on the"RMWKUP" bit in Mode Register
	mUsbRmWkupSet(dev->va_base);
	spin_unlock_irqrestore (&dev->lock, flags);
	return 0;
}

static int FTC_set_selfpowered(struct usb_gadget *_gadget, int value)
{
	DBG_FUNCC("+FTC_set_selfpowered()\n");
	return -EOPNOTSUPP;
}

#define USB_ENABLE_DMA 1
#define USB_DISABLE_DMA 2
static int FTC_ioctl(struct usb_gadget *_gadget, unsigned code, unsigned long param)
{
	unsigned long	flags;
	struct FTC_udc	*dev;
	struct FTC_ep	*ep;
	struct usb_ep   *_ep;

	DBG_FUNCC("+FTC_ioctl()\n");

	if (!_gadget)
		return -ENODEV;
	dev = container_of (_gadget, struct FTC_udc, gadget);
	spin_lock_irqsave (&dev->lock, flags);

	switch (code)
	{
		case USB_ENABLE_DMA:   //DMA enable from others
			_ep = (struct usb_ep *)param;
			ep = container_of(_ep, struct FTC_ep, ep);
			ep->dma=1;
			break;
		case USB_DISABLE_DMA:   //DMA disable from others
			_ep = (struct usb_ep *)param;
			ep = container_of(_ep, struct FTC_ep, ep);
			ep->dma=0;
			break;
		default:
			break;
	}

	spin_unlock_irqrestore (&dev->lock, flags);
	return -EOPNOTSUPP;
}

#include "fotg2xx_udc.c"

static void udc_enable(struct FTC_udc *dev)
{
	DBG_FUNCC("+udc_enable()\n");

	// Enable usb200 global interrupt
	mUsbGlobIntEnSet(dev->va_base);
	mUsbChipEnSet(dev->va_base);
}

static int FTC_start(struct usb_gadget_driver *driver,
		int (*bind)(struct usb_gadget *))
{
    int retval = 0;

    if (!fotg210_udc)
        return -ENODEV;

    if (!driver
            || driver->max_speed < USB_SPEED_FULL
            || !bind
            || !driver->setup)
        return -EINVAL;

    if (fotg210_udc->driver)
        return -EBUSY;

    /* Disable Global Interrupt to prevent useless irq */
    mUsbGlobIntDis(fotg210_udc->va_base);

    /* hook up the driver */
    fotg210_udc->driver = driver;
    fotg210_udc->gadget.dev.driver = &driver->driver;

    retval = bind(&fotg210_udc->gadget);

    if (retval) {
        printk("bind to driver %s --> error %d\n", driver->driver.name, retval);
        fotg210_udc->driver = 0;
        return retval;
    }

    /* then enable host detection and ep0; and we're ready
     * for set_configuration as well as eventual disconnect.
     */
    mUsbUnPLGClr(fotg210_udc->va_base);
#ifndef CONFIG_USB_GADGET
    *((volatile u32 *) (fotg210_udc->va_base | 0x134)) = 0;
    *((volatile u32 *) (fotg210_udc->va_base | 0x138)) = 0;
    *((volatile u32 *) (fotg210_udc->va_base | 0x13C)) = 0;
#endif
    vUsbInit(fotg210_udc);
    udc_enable(fotg210_udc);

    DBG_FUNCC("%s\n", __func__);
    return 0;
}

static void stop_activity(struct FTC_udc *dev, struct usb_gadget_driver *driver)
{
	unsigned	i;

	udc_dbg(dev, "%s\n", __FUNCTION__);

	if (dev->gadget.speed == USB_SPEED_UNKNOWN)
		driver = 0;

	// disconnect gadget driver after quiesceing hw and the driver
	/*
	 * When connecting to PC and then reboot, udc_reset here will cause kernel panic
	 * eason@2011/10/04
	 */
	//udc_reset (dev);
	for (i = 0; i < FUSB220_CURRENT_SUPPORT_EP; i++)
		nuke(&dev->ep [i], -ESHUTDOWN);
	if (driver) {
		spin_unlock(&dev->lock);
		driver->disconnect(&dev->gadget);
		spin_lock(&dev->lock);
	}

	if (dev->driver)
		udc_enable(dev);
}

static int FTC_stop(struct usb_gadget_driver *driver)
{
	unsigned long flags;

	DBG_FUNCC("+usb_gadget_unregister_driver()\n");

	if (!fotg210_udc)
		return -ENODEV;
	if (!driver || driver != fotg210_udc->driver)
		return -EINVAL;

	spin_lock_irqsave(&fotg210_udc->lock, flags);
	stop_activity(fotg210_udc, driver);
	spin_unlock_irqrestore(&fotg210_udc->lock, flags);
	driver->unbind(&fotg210_udc->gadget);
	fotg210_udc->gadget.dev.driver = NULL;
	fotg210_udc->driver = NULL;

	return 0;
}

static const struct usb_gadget_ops FTC_ops =
{
	.get_frame	     	= FTC_get_frame,
	.wakeup		     	= FTC_wakeup,
	.set_selfpowered 	= FTC_set_selfpowered,
	.ioctl          	= FTC_ioctl,
    .start              = FTC_start,
    .stop               = FTC_stop,
};

/* USB notify user process method */
static int usb_notify_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int usb_noify_release(struct inode *inode, struct file *filp)
{
	return 0;
}

#define USB_SET_PID 7788
static long usb_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
		case USB_SET_PID:
			if (copy_from_user(&pid, (void __user *)arg, sizeof(pid))) {
				ret = -EFAULT;
				break;
			}
			printk("pid = %d\n", pid);
			break;
		default:
			break;
	}
	return ret;
}

struct file_operations usb_fops = {
	.owner = THIS_MODULE,
	.open = usb_notify_open,
	.release = usb_noify_release,
	.unlocked_ioctl = usb_ioctl,
};
/**********************************/


/*-------------------------------------------------------------------------*/
static void udc_reinit (struct FTC_udc *dev)
{
	unsigned i;

	DBG_FUNCC("+udc_reinit()\n");

	INIT_LIST_HEAD (&dev->gadget.ep_list);
	dev->gadget.ep0 = &dev->ep[0].ep;
	dev->gadget.speed = USB_SPEED_UNKNOWN;
	dev->ep0state = EP0_DISCONNECT;
	dev->irqs = 0;

	for (i = 0; i < FUSB220_CURRENT_SUPPORT_EP; i++)
	{
		struct FTC_ep	*ep = &dev->ep[i];

		ep->num = i;
		ep->ep.name = names[i];
		DBG_CTRLL("EP%d Name = %s \n",i, ep->ep.name);

		ep->ep.ops = &FTC_ep_ops;
		list_add_tail (&ep->ep.ep_list, &dev->gadget.ep_list);
		ep->dev = dev;
		INIT_LIST_HEAD (&ep->queue);
		ep_reset(ep);
	}
	for (i = 0; i < FUSB220_CURRENT_SUPPORT_EP; i++)
		dev->ep[i].irqs = 0;

	dev->ep[0].ep.maxpacket = MAX_EP0_SIZE;
	list_del_init (&dev->ep[0].ep.ep_list);
	//Info(dev,"L%x: GM UDC reinit finish...\n", dev->u8LineCount ++);
}

static void udc_reset(struct FTC_udc *dev)
{
	DBG_FUNCC("+udc_reset()\n");

	//default value
	dev->Dma_Status = PIO_Mode;
	dev->u8LineCount = 0;
	//Info(dev,"***** FTC OTG controller Device Linux Lower Driver *****\n");
	//Info(dev,"L%x: System initial, Please wait...\n", dev->u8LineCount ++);

	// initial Reg setup
	mUsbTstHalfSpeedDis(dev->va_base);		// Set for FPGA Testing 0x02 BIT7
	mUsbUnPLGClr(dev->va_base);				// 0x08 BIT0
 	vUsbInit(dev);

	//Info(dev,"L%x: System reset finish...\n", dev->u8LineCount ++);
	//Info(dev,"\nInterrupt Mask:0x%x\n",bFUSBPort(0x10));
}

static irqreturn_t FUSBD_irq(int irq, void *_dev)
{
	struct FTC_udc *dev = _dev;
	u32 status = wFOTGPeri_Port(dev->va_base, 0x84);

    /* status & (OTGC_INT_A_TYPE | OTGC_INT_B_TYPE) */
	if (status & (BIT0|BIT4|BIT5|BIT8|BIT9|BIT10|BIT11|BIT12)) {
		fotg200_handle_irq(irq);
	}

	spin_lock(&dev->lock);
	dev->usb_interrupt_level1_Save = mUsbIntGroupRegRd(dev->va_base);
	dev->usb_interrupt_level1_Mask = mUsbIntGroupMaskRd(dev->va_base);
	dev->usb_interrupt_level1 = dev->usb_interrupt_level1_Save & ~dev->usb_interrupt_level1_Mask;

    /* check Interrupt Group Register(0x140) and Current Role */
	if (dev->usb_interrupt_level1 != 0 && (wFOTGPeri_Port(dev->va_base, 0x80) & BIT20)){
		/* fotg2xx_dbg("FTC_irq %x \n",dev->usb_interrupt_level1);*/
		dev->irqs++;

		//Info(dev,"\nInterrupt Source:0x%x\n",bFUSBPort(0x20));
		vUsbHandler(dev);
		// Clear usb interrupt flags
		dev->usb_interrupt_level1 = 0;
	}

	spin_unlock(&dev->lock);
	return IRQ_RETVAL(1);
}

static void FTC_usb_remove(void)
{
    DBG_FUNCC("+FTC_usb_remove()\n");

    /* start with the driver above us */
    if (fotg210_udc->driver) {
        /* should have been done already by driver model core */
        uwarn(dev,"remove driver '%s' is still registered\n",
                fotg210_udc->driver->driver.name);
        usb_gadget_unregister_driver(fotg210_udc->driver);
    }

    udc_reset(fotg210_udc);

    if (fotg210_udc->got_irq) {
        unsigned irq_no;

#if !defined(CONFIG_PLATFORM_GM8136)
        irq_no = get_fotg2xx_irq(CONFIG_GM_OTG_CHOOSE);
#else
        switch (CONFIG_GM_OTG_CHOOSE) {
            case 0:
                irq_no = get_fotg2xx_irq(CONFIG_GM_OTG_CHOOSE);
                break;
            case 1:
                irq_no = USB_FOTG2XX_1_IRQ;
                break;
            default:
                panic("%s: No such UDC id: %d\n", __func__, CONFIG_GM_OTG_CHOOSE);
                break;
        }
#endif
        free_irq(irq_no, fotg210_udc);
        fotg210_udc->got_irq = 0;
    }

#if defined(CONFIG_PLATFORM_GM8136)
    if (CONFIG_GM_OTG_CHOOSE == 1)
        iounmap((void __iomem *) fotg210_udc->va_base);
#endif

    device_unregister(&(fotg210_udc->gadget.dev));

    kfree(fotg210_udc);

    fotg210_udc = NULL;

#if defined(CONFIG_PLATFORM_GM8126)
    ftpmu010_write_reg(usb_get_pmu_fd(), 0xA0, 0, BIT3);
#elif defined(CONFIG_PLATFORM_GM8210)
    switch (CONFIG_GM_OTG_CHOOSE) {
        case 0:
            /* Configure OTG0 in host mode */
            ftpmu010_pcie_write_reg(usb_get_pmu_fd(), 0x84, 0, BIT0);
            break;
        case 1:
            /* Configure OTG1 in host mode */
            ftpmu010_pcie_write_reg(usb_get_pmu_fd(), 0x84, 0, BIT9);
            break;
        case 2:
            /* Configure OTG2 in host mode */
            ftpmu010_pcie_write_reg(usb_get_pmu_fd(), 0x84, 0, BIT18);
            break;
        default:
            panic("%s: No such UDC id: %d\n", __func__, CONFIG_GM_OTG_CHOOSE);
            break;
    }
#elif defined(CONFIG_PLATFORM_GM8287)
    switch (CONFIG_GM_OTG_CHOOSE) {
        case 0:
            /* Configure OTG0 in host mode */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, 0, BIT2);
            break;
        case 1:
            /* Configure OTG1 in host mode */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xC8, 0, BIT2);
            break;
        default:
            panic("%s: No such UDC id: %d\n", __func__, CONFIG_GM_OTG_CHOOSE);
            break;
    }
#elif defined(CONFIG_PLATFORM_GM8139)
    /* Configure OTG0 in host mode */
    ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, 0, BIT2);
#elif defined(CONFIG_PLATFORM_GM8136)
    switch (CONFIG_GM_OTG_CHOOSE) {
        case 0:
            /* Configure OTG0 in host mode */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, 0, BIT2);
            break;
        case 1:
            /* Configure OTG1 in host mode */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, 0, BIT0);
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xB4, BIT16, BIT16);
            break;
        default:
            panic("%s: No such UDC id: %d\n", __func__, CONFIG_GM_OTG_CHOOSE);
            break;
    }
#endif

    //Info(dev,"USB device unbind\n");
}

void release_dummy(struct device *dev)
{
   return;
}
/* wrap this driver around the specified pci device, but
 * don't respond over USB until a gadget driver binds to us.
 */
//Trace ok 12212004
static int FTC_usb_probe(void)
{
    int	retval = 0;

    DBG_FUNCC("+FTC_usb_probe()\n");
    printk("CONFIG_GM_OTG_CHOOSE=%d\n", CONFIG_GM_OTG_CHOOSE);

    //<1>.Init struct FTC_udc structure
    /* alloc, and start init */
    spin_lock_init(&fotg210_udc->lock);

    /* the "gadget" abstracts/virtualizes the controller */
    fotg210_udc->enabled = 1;
    fotg210_udc->EPUseDMA = DMA_CHANEL_FREE;
    fotg210_udc->ReqForDMA = 0;
#if !defined(CONFIG_PLATFORM_GM8136)
    fotg210_udc->va_base = get_fotg2xx_va(CONFIG_GM_OTG_CHOOSE);
#else
    switch (CONFIG_GM_OTG_CHOOSE) {
        case 0:
            fotg210_udc->va_base = get_fotg2xx_va(CONFIG_GM_OTG_CHOOSE);
            break;
        case 1:
            fotg210_udc->va_base = (u32) ioremap_nocache(USB_FOTG2XX_1_PA_BASE, USB_FOTG2XX_1_PA_SIZE);
            break;
        default:
            panic("%s: No such UDC id: %d\n", __func__, CONFIG_GM_OTG_CHOOSE);
            break;
    }
#endif
    fotg210_udc->gadget.ops = &FTC_ops;
    fotg210_udc->gadget.name = driver_name;
#if !defined(CONFIG_PLATFORM_GM8136)
	fotg210_udc->gadget.max_speed = USB_SPEED_HIGH;
#else
    switch (CONFIG_GM_OTG_CHOOSE) {
        case 0:
            fotg210_udc->gadget.max_speed = USB_SPEED_HIGH;
            break;
        case 1:
            fotg210_udc->gadget.max_speed = USB_SPEED_FULL;
            break;
        default:
            panic("%s: No such UDC id: %d\n", __func__, CONFIG_GM_OTG_CHOOSE);
            break;
    }
#endif

    fotg210_udc->gadget.dev.parent = NULL;
    fotg210_udc->gadget.dev.dma_mask = (void *)0xffffffff;
    fotg210_udc->gadget.dev.coherent_dma_mask = 0xffffffff;
    fotg210_udc->gadget.dev.release = &release_dummy;

	dev_set_drvdata(&fotg210_udc->gadget.dev, fotg210_udc);
    dev_set_name(&fotg210_udc->gadget.dev, driver_name);

    retval = device_register (&(fotg210_udc->gadget.dev));
    if (retval < 0) {
        printk("%s: device_register() failed\n", __func__);
        goto done;
    }

#if defined(CONFIG_PLATFORM_GM8126)
    ftpmu010_write_reg(usb_get_pmu_fd(), 0xA0, 0, (BIT0|BIT1|BIT2)<<17);
    ftpmu010_write_reg(usb_get_pmu_fd(), 0xA0, BIT1<<17, BIT1<<17); // config reference voltage to 120mV
    ftpmu010_write_reg(usb_get_pmu_fd(), 0xA0, BIT0, BIT0); // enable VBUS input
    ftpmu010_write_reg(usb_get_pmu_fd(), 0xA0, BIT3, BIT3); // set to device mode
#elif defined(CONFIG_PLATFORM_GM8210)
    switch (CONFIG_GM_OTG_CHOOSE) {
        case 0:
            /* Configure OTG0 in device mode */
            ftpmu010_pcie_write_reg(usb_get_pmu_fd(), 0x84, BIT0, BIT0);
            /* Enable OTG0 VBUS input */
            ftpmu010_pcie_write_reg(usb_get_pmu_fd(), 0x84, BIT1, BIT1);
            break;
        case 1:
            /* Configure OTG1 in device mode */
            ftpmu010_pcie_write_reg(usb_get_pmu_fd(), 0x84, BIT0, BIT9);
            /* Enable OTG1 VBUS input */
            ftpmu010_pcie_write_reg(usb_get_pmu_fd(), 0x84, BIT10, BIT10);
            break;
        case 2:
            /* Configure OTG2 in device mode */
            ftpmu010_pcie_write_reg(usb_get_pmu_fd(), 0x84, BIT0, BIT18);
            /* Enable OTG2 VBUS input */
            ftpmu010_pcie_write_reg(usb_get_pmu_fd(), 0x84, BIT19, BIT19);
            break;
        default:
            panic("%s: No such UDC id: %d\n", __func__, CONFIG_GM_OTG_CHOOSE);
            break;
    }
#elif defined(CONFIG_PLATFORM_GM8287)
    switch (CONFIG_GM_OTG_CHOOSE) {
        case 0:
            /* Configure OTG0 in device mode */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, BIT2, BIT2);
            /* Enable OTG0 VBUS input */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, BIT3, BIT3);
            break;
        case 1:
            /* Configure OTG1 in device mode */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xC8, BIT2, BIT2);
            /* Enable OTG1 VBUS input */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xC8, BIT3, BIT3);
            break;
        default:
            panic("%s: No such UDC id: %d\n", __func__, CONFIG_GM_OTG_CHOOSE);
            break;
    }
#elif defined(CONFIG_PLATFORM_GM8139)
    ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, BIT24|BIT25|BIT26, BIT24|BIT25|BIT26);
    ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, BIT4|BIT5|BIT19, BIT4|BIT5|BIT19);
    ftpmu010_write_reg(usb_get_pmu_fd(), 0xAC, 0, BIT0);
    ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, BIT6, BIT6);
    /* Configure OTG0 in device mode */
    ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, BIT2, BIT2);
    /* Enable OTG0 VBUS input */
    ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, BIT3, BIT3);
#elif defined(CONFIG_PLATFORM_GM8136)
    switch (CONFIG_GM_OTG_CHOOSE) {
        case 0:
            /* Configure OTG0 in device mode */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, BIT2, BIT2);
            /* Enable OTG0 VBUS input */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, BIT3, BIT3);
            break;
        case 1:
            {
                /* set pinmux of DP/DM */
                u32 val;

                val = * (volatile u32 *) (PMU_FTPMU010_VA_BASE + 0x64);
                val &= ~((0x3 << 16) | (0x3 << 18));
                val |= (0x1 << 16) | (0x1 << 18);
                * (volatile u32 *) (PMU_FTPMU010_VA_BASE + 0x64) = val;
            }
            /* Configure OTG1 in device mode */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, BIT0, BIT0);
            /* Enable OTG1 VBUS input */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xC0, BIT1, BIT1);
            /* Turn on OTG1 gating clock */
            ftpmu010_write_reg(usb_get_pmu_fd(), 0xB4, 0, BIT16);
            break;
        default:
            panic("%s: No such UDC id: %d\n", __func__, CONFIG_GM_OTG_CHOOSE);
            break;
    }
#endif

    //<2>. udc Reset/udc reinit
    /* init to known state, then setup irqs */
    udc_reset(fotg210_udc);
    udc_reinit(fotg210_udc);

    //<3>.Init USB DEV ISR
    if (!fotg210_udc->got_irq) {
        unsigned irq_no;

#if !defined(CONFIG_PLATFORM_GM8136)
        irq_no = get_fotg2xx_irq(CONFIG_GM_OTG_CHOOSE);
#else
        switch (CONFIG_GM_OTG_CHOOSE) {
            case 0:
                irq_no = get_fotg2xx_irq(CONFIG_GM_OTG_CHOOSE);
                break;
            case 1:
                irq_no = USB_FOTG2XX_1_IRQ;
                break;
            default:
                panic("%s: No such UDC id: %d\n", __func__, CONFIG_GM_OTG_CHOOSE);
                break;
        }
#endif
        if (request_irq(irq_no, FUSBD_irq, IRQF_SHARED, driver_name, fotg210_udc) < 0) {
            uwarn(dev, "request interrupt failed\n");
            retval = -EBUSY;
            goto done;
        }
        fotg210_udc->got_irq = 1;
    }
    mUsbUnPLGSet(fotg210_udc->va_base);

    retval = usb_add_gadget_udc(&(fotg210_udc->gadget.dev), &fotg210_udc->gadget);
    if (retval)
        goto done;
    printk("Init GM UDC ISR finished\n");

    /* Generate device node */
    retval = alloc_chrdev_region(&dev_num, 0, 1, USB_USER_NAME);
    if (unlikely(retval < 0)) {
        printk(KERN_ERR "%s:alloc_chrdev_region failed\n", __func__);
        goto done;
    }
    cdev_init(&usb_cdev, &usb_fops);
    usb_cdev.owner = THIS_MODULE;
    usb_cdev.ops = &usb_fops;
    retval = cdev_add(&usb_cdev, dev_num, 1);
    if (unlikely(retval < 0)) {
        printk(KERN_ERR "%s:cdev_add failed\n", __func__);
        goto err3;
    }
    usb_class = class_create(THIS_MODULE, USB_USER_NAME);
    if (IS_ERR(usb_class)) {
        printk(KERN_ERR "%s:class_create failed\n", __func__);
        goto err2;
    }
    device_create(usb_class, NULL, usb_cdev.dev, NULL, USB_USER_NAME);

    /* done */
    return 0;
err2:
    cdev_del(&usb_cdev);
err3:
    unregister_chrdev_region(dev_num, 1);

done:
    DBG_TEMP("FTC_usb_probe() failed\n");
    if (fotg210_udc)
        FTC_usb_remove();
    return retval;
}

/*-------------------------------------------------------------------------*/
//Trace ok 12212004
static int __init init (void)
{
	//Info(dev,"device init ... %x\n",0);

    fotg210_udc = kzalloc (sizeof(struct FTC_udc), GFP_KERNEL);
    if (fotg210_udc == NULL) {
        printk("Error ==> FOTG2XX Device part Initiation.\n");
        return -ENOMEM;
    }

	return FTC_usb_probe();
}

module_init (init);

//Trace ok 12212004
static void __exit cleanup (void)
{
	//Info(dev,"remove USB device Lower driver\n");
	unregister_chrdev_region(dev_num, 1);
	cdev_del(&usb_cdev);
	device_destroy(usb_class, dev_num);
	class_destroy(usb_class);
	return FTC_usb_remove();
}

module_exit (cleanup);

MODULE_AUTHOR("<GM-tech.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("USB device mode Driver");
