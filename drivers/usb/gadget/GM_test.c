/*
 * zero.c -- Gadget Zero, for USB development
 *
 * Copyright (C) 2003-2004 David Brownell
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */



#define DEBUG 1
// #define VERBOSE

//#define BULK_USBS_DEBUG

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
//#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/utsname.h>
#include <linux/device.h>
#include <linux/moduleparam.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "gadget_chips.h"
#include "../host/fotg2xx-config.h"
#include "../host/fotg2xx.h"
#include "../host/fotg2xx-ehci-macro.h"
#include "../host/fotg2xx_opt-macro.h"
#include "GM_udc.h"
#include "GM_test.h"
#include <linux/dma-mapping.h>

/* ////////////////////////////////////////////////////////////////////////////////
//
//  Variables and definitions of System for Enumeration
//
//////////////////////////////////////////////////////////////////////////////// */


/*-------------------------------------------------------------------------*/

#define DRIVER_VERSION		"St Patrick's Day 2004"

static const char shortname [] = "ftst";
static const char longname [] = "Gadget Test";

static const char source_sink [] = "source and sink data";
static const char loopback [] = "Not support lookback" ;

/*-------------------------------------------------------------------------*/

/*
 * driver assumes self-powered hardware, and
 * has no way for users to trigger remote wakeup.
 *
 * this version autoconfigures as much as possible,
 * which is reasonable for most "bulk-only" drivers.
 */
static const char *EP_BULK_IN_NAME;		/* source */
static const char *EP_BULK_OUT_NAME;		/* sink */
static const char *EP_INTR_IN_NAME;		/* source */
static const char *EP_INTR_OUT_NAME;		/* sink */

/*-------------------------------------------------------------------------*/

/* big enough to hold our biggest descriptor */
#define USB_BUFSIZ	256

struct ftst_dev {
	spinlock_t		lock;
	struct usb_gadget	*gadget;
	struct usb_request	*req;		/* for control responses */

	/* when configured, we have one of two configs:
	 * - source data (in to host) and sink it (out from host)
	 * - or loop it back (out from host back in to host)
	 */
	u8			config;
	struct usb_ep		*bulkin_ep, *bulkout_ep,*intrin_ep,*introut_ep;

	/* autoresume timer */
	struct timer_list	resume;
};

#ifdef xprintk
#undef xprintk
#endif
#define xprintk(d,level,fmt,args...) \
	dev_printk(level , &(d)->gadget->dev , fmt , ## args)

#ifdef DEBUG
#define DBG(dev,fmt,args...) \
	xprintk(dev , KERN_DEBUG , fmt , ## args)
#else
#define DBG(dev,fmt,args...) \
	do { } while (0)
#endif /* DEBUG */

#ifdef VERBOSE
#define VDBG	DBG
#else
#define VDBG(dev,fmt,args...) \
	do { } while (0)
#endif /* VERBOSE */

#define ERROR(dev,fmt,args...) \
	xprintk(dev , KERN_ERR , fmt , ## args)
#define UWARN(dev,fmt,args...) \
	xprintk(dev , KERN_WARNING , fmt , ## args)
#define INFO(dev,fmt,args...) \
	xprintk(dev , KERN_INFO , fmt , ## args)

/*-------------------------------------------------------------------------*/

static unsigned qlen = 32;
static unsigned pattern = 0;

/* module_param (buflen, uint, S_IRUGO|S_IWUSR); */
module_param (qlen, uint, S_IRUGO|S_IWUSR);
module_param (pattern, uint, S_IRUGO|S_IWUSR);

/*
 * if it's nonzero, autoresume says how many seconds to wait
 * before trying to wake up the host after suspend.
 */
static unsigned autoresume = 0;
module_param (autoresume, uint, 0);

/*-------------------------------------------------------------------------*/

/* Thanks to NetChip Technologies for donating this product ID.
 *
 * DO NOT REUSE THESE IDs with a protocol-incompatible driver!!  Ever!!
 * Instead:  allocate your own, using normal USB-IF procedures.
 */
#define DRIVER_VENDOR_NUM	0x2310		/* GM */
#define DRIVER_PRODUCT_NUM	0x5678		/* Linux-USB "Gadget Zero" */

/*-------------------------------------------------------------------------*/

/*
 * DESCRIPTORS ... most are static, but strings and (full)
 * configuration descriptors are built on demand.
 */

#define STRING_MANUFACTURER		25
#define STRING_PRODUCT			42
#define STRING_SERIAL			101
#define STRING_SOURCE_SINK		250
#define STRING_LOOPBACK			251

/*
 * This device advertises two configurations; these numbers work
 * on a pxa250 as well as more flexible hardware.
 */
#define	CONFIG_SOURCE_SINK	3
#define	CONFIG_LOOPBACK		2

static struct usb_device_descriptor
device_desc = {
	.bLength =		sizeof device_desc,
	.bDescriptorType =	USB_DT_DEVICE,

	.bcdUSB =		__constant_cpu_to_le16 (0x0200),
	.bDeviceClass =		USB_CLASS_VENDOR_SPEC,

	.idVendor =		__constant_cpu_to_le16 (DRIVER_VENDOR_NUM),
	.idProduct =		__constant_cpu_to_le16 (DRIVER_PRODUCT_NUM),
	.iManufacturer =	STRING_MANUFACTURER,
	.iProduct =		STRING_PRODUCT,
	.iSerialNumber =	STRING_SERIAL,
	.bNumConfigurations =	1,
};

static struct usb_config_descriptor
source_sink_config = {
	.bLength =		sizeof source_sink_config,
	.bDescriptorType =	USB_DT_CONFIG,

	/* compute wTotalLength on the fly */
	.bNumInterfaces =	1,
	.bConfigurationValue =	CONFIG_SOURCE_SINK,
	.iConfiguration =	STRING_SOURCE_SINK,
	.bmAttributes =		USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower =		1,	/* self-powered */
};

static struct usb_otg_descriptor
otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,

	.bmAttributes =		USB_OTG_SRP,
};

/* one interface in each configuration */

static const struct usb_interface_descriptor
source_sink_intf = {
	.bLength =		sizeof source_sink_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bNumEndpoints =	4,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	.iInterface =		STRING_SOURCE_SINK,
};

/* two full speed bulk endpoints; their use is config-dependent */

static struct usb_endpoint_descriptor
fs_bulkin_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor
fs_bulkout_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor
fs_intrin_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bInterval = 		1,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
};

static struct usb_endpoint_descriptor
fs_introut_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bInterval = 		1,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
};

static const struct usb_descriptor_header *fs_source_sink_function [] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	(struct usb_descriptor_header *) &source_sink_intf,
	(struct usb_descriptor_header *) &fs_bulkout_desc,
	(struct usb_descriptor_header *) &fs_bulkin_desc,
	(struct usb_descriptor_header *) &fs_introut_desc,
	(struct usb_descriptor_header *) &fs_intrin_desc,
	NULL,
};


/*
 * usb 2.0 devices need to expose both high speed and full speed
 * descriptors, unless they only run at full speed.
 *
 * that means alternate endpoint descriptors (bigger packets)
 * and a "device qualifier" ... plus more construction options
 * for the config descriptor.
 */

static struct usb_endpoint_descriptor
hs_bulkin_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16 (512),
};

static struct usb_endpoint_descriptor
hs_bulkout_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16 (512),
};

static struct usb_endpoint_descriptor
hs_intrin_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.bInterval = 		1,
	.wMaxPacketSize =	__constant_cpu_to_le16 (64),
};

static struct usb_endpoint_descriptor
hs_introut_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bInterval = 		1,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16 (64),
};

static struct usb_qualifier_descriptor
dev_qualifier = {
	.bLength =		sizeof dev_qualifier,
	.bDescriptorType =	USB_DT_DEVICE_QUALIFIER,

	.bcdUSB =		__constant_cpu_to_le16 (0x0200),
	.bDeviceClass =		USB_CLASS_VENDOR_SPEC,

	.bNumConfigurations =	2,
};

static const struct usb_descriptor_header *hs_source_sink_function [] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	(struct usb_descriptor_header *) &source_sink_intf,
	(struct usb_descriptor_header *) &hs_bulkin_desc,
	(struct usb_descriptor_header *) &hs_bulkout_desc,
	(struct usb_descriptor_header *) &hs_intrin_desc,
	(struct usb_descriptor_header *) &hs_introut_desc,
	NULL,
};


/* maxpacket and other transfer characteristics vary by speed. */
#define ep_desc(g,hs,fs) (((g)->speed==USB_SPEED_HIGH)?(hs):(fs))

static char				manufacturer [50];
static char				serial [128];

/* static strings, in UTF-8 */
static struct usb_string		strings [] = {
	{ STRING_MANUFACTURER, manufacturer, },
	{ STRING_PRODUCT, longname, },
	{ STRING_SERIAL, serial, },
	{ STRING_LOOPBACK, loopback, },
	{ STRING_SOURCE_SINK, source_sink, },
	{  }			/* end of list */
};

static struct usb_gadget_strings	stringtab = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings,
};

/////////////////////////////////////////////////////////////////////////////////
MassStorageState eUsbProessCBW(struct usb_request *req);
MassStorageState eUsbDataOut(struct usb_request *req);
void eUsbDataIn(struct usb_request *req);
void ShowCBW(void);
void ShowCSW(void);
void vUsbSendCSW(struct usb_request *req);

static MassStorageState eUsbMassStorageState;
static u8   u8VirtMemory[MAX_BUFFER_SIZE];
static u32* u32VirtMemory;
static CSW tCSW;
static u16 u16VirtMemoryIndex;
static CBW tCBW;

static u8 u8InterruptCount;
static u8 u8Interrupt_TX_COUNT ;
static u8 u8Interrupt_RX_COUNT ;
static u8 u8InterruptOutCount ;

u8 u8LineCount;
u8 u8UsbMessageLevel;
u8 *u8InterruptArray;

/* ////////////////////////////////////////////////////////////////////////////////
//
//  Functions for Endpoints' Access
//
//////////////////////////////////////////////////////////////////////////////// */

/*
 * config descriptors are also handcrafted.  these must agree with code
 * that sets configurations, and with code managing interfaces and their
 * altsettings.  other complexity may come from:
 *
 *  - high speed support, including "other speed config" rules
 *  - multiple configurations
 *  - interfaces with alternate settings
 *  - embedded class or vendor-specific descriptors
 *
 * this handles high speed, and has a second config that could as easily
 * have been an alternate interface setting (on most hardware).
 *
 * NOTE:  to demonstrate (and test) more USB capabilities, this driver
 * should include an altsetting to test interrupt transfers, including
 * high bandwidth modes at high speed.  (Maybe work like Intel's test
 * device?)
 */
static int
config_buf (struct usb_gadget *gadget,
		u8 *buf, u8 type, unsigned index)
{
	int				is_source_sink;
	int				len;
	const struct usb_descriptor_header **function;
	int				hs = (gadget->speed == USB_SPEED_HIGH);

	/* two configurations will always be index 0 and index 1 */
	if (index > 1)
		return -EINVAL;
	is_source_sink = 1;

	if (type == USB_DT_OTHER_SPEED_CONFIG)
		hs = !hs;
	if (hs)
		function = hs_source_sink_function;
	else
		function = fs_source_sink_function;

	/* for now, don't advertise srp-only devices */
	if (!gadget->is_otg)
		function++;

	len = usb_gadget_config_buf (&source_sink_config,
			buf, USB_BUFSIZ, function);
	if (len < 0)
		return len;
	((struct usb_config_descriptor *) buf)->bDescriptorType = type;
	return len;
}

/*-------------------------------------------------------------------------*/
//static void *FTC_alloc_buffer(struct usb_ep *_ep, unsigned bytes,
//					dma_addr_t *dma, unsigned  gfp_flags);
//static void FTC_free_buffer(struct usb_ep *_ep, void *buf, dma_addr_t dma, unsigned bytes);

static struct usb_request *
alloc_ep_req (struct usb_ep *ep, unsigned length)
{
	struct usb_request	*req;
	struct FTC_ep           *my_ep;
	struct FTC_request	*my_req;

	req = usb_ep_alloc_request (ep, GFP_ATOMIC);

        if (req) {
	   my_ep = container_of(ep, struct FTC_ep, ep);
	   my_req = container_of(req, struct FTC_request, req);
	   my_req->req.buf = dma_alloc_coherent(&my_ep->dev->gadget.dev,length,&(my_req->req.dma),GFP_ATOMIC);
	   if (!my_req->req.buf) {
              printk("cannot allocate DMA buffer for EP:%x\n", (u32) ep);
	   }
	   else {
	      my_req->req.length = length;
	      my_ep->dma=1;
	      //printk("EP %x== alloc buffer = 0x%x\n", (u32) ep, (u32) my_req->req.buf);
	   }
	}
	else
	   printk("Error :usb_ep_alloc_request Not Success !!\n");

	return req;
}

static void free_ep_req (struct usb_ep *ep, struct usb_request *req)
{
	struct FTC_request	*my_req;
	struct FTC_ep	*my_ep;

	if (req->buf) {
	   my_ep = container_of(ep, struct FTC_ep, ep);
	   my_req = container_of(req, struct FTC_request, req);
	   if (my_ep->dma !=0)
	      dma_free_coherent(&my_ep->dev->gadget.dev,my_req->req.length,my_req->req.buf,my_req->req.dma);
	   my_ep->dma=0;
	}

	usb_ep_free_request (ep, req);
}

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

/* if there is only one request in the queue, there'll always be an
 * irq delay between end of one request and start of the next.
 * that prevents using hardware dma queues.
 */
static void source_sink_test (struct usb_ep *ep, struct usb_request *req)
{
	struct ftst_dev	*dev = ep->driver_data;
	int		status = req->status;

	printk("source_sink_test Not implement\n");
	status = usb_ep_queue (ep, req, GFP_ATOMIC);
	if (status) {
		ERROR (dev, "kill %s:  resubmit %d bytes --> %d\n",
				ep->name, req->length, status);
		usb_ep_set_halt (ep);
		/* FIXME recover later ... somehow */
	}

}
static void ftst_bulkin_complete(struct usb_ep *ep, struct usb_request *req);
static void ftst_bulkout_complete(struct usb_ep *ep, struct usb_request *req);
static void ftst_intrin_complete(struct usb_ep *ep, struct usb_request *req);
static void ftst_introut_complete(struct usb_ep *ep, struct usb_request *req);

static unsigned int buflen[] = {
	0,
	512, /* Bulkin i.e. Endpoint 1 buffer size */
	512, /* Bulkout i.e. Endpoint 2 buffer size */
	64, /* Bulkin i.e. Endpoint 3 buffer size */
	64 /* Bulkout i.e. Endpoint 4 buffer size */
};

static void (*source_sink_complete[])(struct usb_ep *ep, struct usb_request *req) = {
		&source_sink_test,
		&ftst_bulkin_complete,
		&ftst_bulkout_complete,
		&ftst_intrin_complete,
		&ftst_introut_complete,
		NULL
};

static struct usb_request *
source_sink_start_ep (struct usb_ep *ep, unsigned gfp_flags,int sz,int i)
{
	struct usb_request	*req;
	int			status;

	req = alloc_ep_req (ep, sz);
	if (!req)
		return NULL;

	memset (req->buf, 0, req->length);
	req->complete = source_sink_complete[i];

	status = usb_ep_queue (ep, req, gfp_flags);
	if (status) {
		struct ftst_dev	*dev = ep->driver_data;

		ERROR (dev, "start %s --> %d\n", ep->name, status);
		free_ep_req (ep, req);
		req = NULL;
	}

	return req;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Bulk IN/OUT transfer test functions
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//		vUsb_APInit()
//		Description: User specified circuit (AP) init
//		input: none
//		output: none
///////////////////////////////////////////////////////////////////////////////
void vUsb_APInit(void)
{
   u32 i;
   //u8 u8_i = 0;

   ////// initial Bulk //////
   eUsbMassStorageState = STATE_CBW;	// Init State

   for(i = 0; i < MAX_BUFFER_SIZE; i++) {
      u8VirtMemory[i] = 0;			// Init Buffer
      //u8VirtMemory[u16_i] = u8_i++;		// Init Buffer
   }

   u32VirtMemory = (u32 *)kmalloc(MAX_BUFFER_SIZE,GFP_KERNEL);
   memcpy(u32VirtMemory,u8VirtMemory,MAX_BUFFER_SIZE);

   tCSW.u32Signature = CSW_SIGNATE;	// Init u32Signature

   ////// initial INT and ISO test ////////////
   u8InterruptCount = 0;
   u8Interrupt_TX_COUNT = 1;
   u8InterruptOutCount = 0;
   u8Interrupt_RX_COUNT = 1;
}

void vUsb_queue_reqest(struct ftst_dev *dev,MassStorageState eState)
{
   struct usb_request *req;

   switch(eState){
      case STATE_CBW:
         if((req = source_sink_start_ep (dev->bulkout_ep, GFP_ATOMIC,512,2)) == NULL){
	    usb_ep_disable (dev->bulkout_ep);
	    printk("Allocate error 1!!\n");
	    return;
	 }
	 break;
      case STATE_CB_DATA_OUT:
	 if((req = source_sink_start_ep (dev->bulkout_ep, GFP_ATOMIC,512,2)) == NULL){
	    usb_ep_disable (dev->bulkout_ep);
	    printk("Allocate error 2!!\n");
	    return;
	 }
	 break;
      case STATE_CSW:
	 if((req = source_sink_start_ep (dev->bulkin_ep, GFP_ATOMIC,13,1)) == NULL){
	    usb_ep_disable (dev->bulkin_ep);
	    printk("Allocate error 3!!\n");
	    return;
	 }
	 vUsbSendCSW(req);
	 break;
      case STATE_CB_DATA_IN:
         if(tCSW.u32DataResidue > 512){
	    if((req=source_sink_start_ep (dev->bulkin_ep, GFP_ATOMIC,512,1)) == NULL){
	       usb_ep_disable (dev->bulkin_ep);
	       printk("Allocate error 4!!\n");
	       return;
	    }
	 }
	 else if ((req=source_sink_start_ep (dev->bulkin_ep, GFP_ATOMIC,tCSW.u32DataResidue,1)) == NULL){
	    usb_ep_disable (dev->bulkin_ep);
	    printk("Allocate error 5!!\n");
	    return;
         }
	 eUsbDataIn(req);
	 break;
      default:
	 break;
   }
}

///////////////////////////////////////////////////////////////////////////////
//		cyg_usb_bulk_in()
//		Description: USB FIFO0 interrupt service process
//		input: none
//		output: none
///////////////////////////////////////////////////////////////////////////////
static void ftst_bulkin_complete(struct usb_ep *ep, struct usb_request *req)
{
   struct FTC_ep *my_ep;

   my_ep = container_of(ep, struct FTC_ep, ep);

   switch(eUsbMassStorageState)
   {
      case STATE_CSW:
         eUsbMassStorageState = STATE_CBW;
	 break;
      case STATE_CB_DATA_IN:
#ifdef BULK_USBS_DEBUG
	 printk("EP1 CS DATA IN\n");
#endif
	 if (tCSW.u32DataResidue == 0)
	    eUsbMassStorageState = STATE_CSW;
	 else
	    eUsbMassStorageState = STATE_CB_DATA_IN;
	 break;
      default:
	 printk("L%x: Error FIFO0_IN interrupt.\n", u8LineCount);
	 break;
   }
   free_ep_req(ep,req);   //release current request
   if (my_ep->stopped)
      vUsb_queue_reqest((struct ftst_dev *)(ep->driver_data),eUsbMassStorageState);
}


static void ftst_bulkout_complete(struct usb_ep *ep, struct usb_request *req)
{
   struct FTC_ep		*my_ep;

   my_ep = container_of(ep, struct FTC_ep, ep);

   switch(eUsbMassStorageState)
   {
      case STATE_CBW:
#ifdef BULK_USBS_DEBUG
         printk("EP2 Out CBW:0x%x\n",req->actual);
#endif
	 if (req->actual == 31)
	    eUsbMassStorageState = eUsbProessCBW(req);
	 else {
#ifdef BULK_USBS_DEBUG
	    printk("EP out STAL SET\n");
#endif
	    usb_ep_set_halt (ep);
         }
	 //printf("FIFO 2:0x%x\n",mUsbFIFOOutByteCount(FIFO2));
	 break;
      case STATE_CB_DATA_OUT:
#if 0
         if((u16)tCBW.u32DataTransferLength != u16FIFOByteCount)
	    printf("Byte Count Error = %d. (Correct = %d)\n",
		u8LineCount, u16FIFOByteCount, (u16)tCBW.u32DataTransferLength);
	 else
#endif
	    eUsbMassStorageState = eUsbDataOut(req);
  	 break;
      default:
         printk("L%x: Error FIFO2_OUT interrupt.\n", u8LineCount);
 	 break;
   }
   free_ep_req(ep,req);
   if (my_ep->stopped)
      vUsb_queue_reqest((struct ftst_dev *)(ep->driver_data),eUsbMassStorageState);
}

///////////////////////////////////////////////////////////////////////////////
//		ShowCBW()
//		Description: show the whole CBW structure
//		input: none
//		output: none
///////////////////////////////////////////////////////////////////////////////
void ShowCBW(void)
{
#if 1
	int i;
	printk("tCBW.u32Signature = 0x%x\n",tCBW.u32Signature);
	printk("tCBW.u32Tag =  0x%x\n",tCBW.u32Tag);
	printk("tCBW.u32DataTransferLength =  0x%x\n",tCBW.u32DataTransferLength);
	printk("tCBW.u8Flags =  0x%x\n",tCBW.u8Flags);
	printk("tCBW.u8LUN = 0x%x\n", tCBW.u8LUN);
	printk("tCBW.u8CBLength =  0x%x\n", tCBW.u8CBLength);
	printk("tCBW.u8CB[0 ~ 15] =");
	for(i = 0; i < 16; i ++)
	{
		printk("%x ", tCBW.u8CB[i]);
		if ((i%8) == 7) printk("\n");
	}
	printk("\n");
#else
	u8 i;
	u8 * pp;
	pp = (u8 *)&tCBW;
	printk("tCBW: \n");
	for (i = 0; i < 16; i ++)
		printk("%02x ", *(pp ++));
	printk("\n");
	for (i = 16; i < 31; i ++)
		printk("%02x ", *(pp ++));
	printk("\n");
#endif
}

///////////////////////////////////////////////////////////////////////////////
//		ShowCSW()
//		Description: show the whole CSW structure
//		input: none
//		output: none
///////////////////////////////////////////////////////////////////////////////
void ShowCSW(void)
{
#if 1
	printk("tCSW.u32Signature =  0x%x\n",tCSW.u32Signature);
	printk("tCSW.u32Tag =  0x%x\n",tCSW.u32Tag);
	printk("tCSW.u32DataResidue =  0x%x\n",tCSW.u32DataResidue);
	printk("tCSW.u8Status = %x\n", tCSW.u8Status);
#else
	if(u8UsbMessageLevel & (MESS_INFO))
	{
		u8 i;
		u8 * pp;
		pp = (u8 *)&tCSW;
		printk("tCSW: \n");
		for (i = 0; i < 13; i ++)
			printk("%02x ", *(pp ++));
		printk("\n");
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////
//		eUsbProessCBW()
//		Description: Process the CBW
//		input: none
//		output: MassStorageState
///////////////////////////////////////////////////////////////////////////////
MassStorageState eUsbProessCBW(	struct usb_request *req)
{
	  MassStorageState eState;
	memcpy(&tCBW,req->buf,31);

#ifdef BULK_USBS_DEBUG
	ShowCBW();
#endif

	if(tCBW.u32Signature != CBW_SIGNATE)
		eState = STATE_CBW;
	else
	{
		// pass u32DataTransferLength to u32DataResidue
		tCSW.u32DataResidue = tCBW.u32DataTransferLength;
		// pass Tag from CBW to CSW
		tCSW.u32Tag = tCBW.u32Tag;
		// Assume Status is CMD_PASS
		tCSW.u8Status = CSW_STATUS_CMD_PASS;
		// Get Virtual Memory start address.
		u16VirtMemoryIndex = tCBW.u8CB[0] | (tCBW.u8CB[1] << 8); // marked by silas???
		u16VirtMemoryIndex=0;// add by silas

		if (tCSW.u32DataResidue == 0)
			eState = STATE_CSW;
		else if (tCBW.u8Flags == 0x00)
			eState = STATE_CB_DATA_OUT;
		else
			eState = STATE_CB_DATA_IN;
	}
	return eState;
}

///////////////////////////////////////////////////////////////////////////////
//		vUsbSendCSW()
//		Description: Send out the CSW structure to PC
//		input: none
//		output: none
///////////////////////////////////////////////////////////////////////////////
void vUsbSendCSW(struct usb_request *req)
{
//	u8 u8Tmp = mUsbHbfCountRd();
	// Send CSW to F0 via DBUS;
#ifdef BULK_USBS_DEBUG
	printk("EP1 IN CSW\n");
	ShowCSW();
#endif
//	printf("Buffer counter = %d",u8Tmp);
	memcpy(req->buf,&tCSW,13);
}

///////////////////////////////////////////////////////////////////////////////
//		eUsbDataIn()
//		Description: Process the data intput stage
//		input: none
//		output: MassStorageState
///////////////////////////////////////////////////////////////////////////////
void eUsbDataIn(struct usb_request *req)
{
   //u16 u16count;
   u32 u16count;

#ifdef BULK_USBS_DEBUG
   printk("eUsbDataIn -> u32DataResidue:%x \n",tCSW.u32DataResidue);
#endif

   if((u16count = tCSW.u32DataResidue) > req->length)
      u16count = req->length;
   //printf("CSW DataTransfer Length:0x%x\n",tCSW.u32DataResidue);
   //printf("count:0x%x\n",u16count);

   // Send u16FIFOByteCount Bytes data to F0 via DBUS;
   memcpy(req->buf,&u8VirtMemory[u16VirtMemoryIndex],u16count);

   u16VirtMemoryIndex += u16count;
   tCSW.u32DataResidue -= u16count;
}

///////////////////////////////////////////////////////////////////////////////
//		eUsbDataOut()
//		Description: Process the data output stage
//		input: none
//		output: MassStorageState
///////////////////////////////////////////////////////////////////////////////
MassStorageState eUsbDataOut(struct usb_request *req)
{
   u32 u32State;

   // Get u16FIFOByteCount Bytes data from F1 via DBUS;
#ifdef BULK_USBS_DEBUG
   printk("EP2 Out CB Data Out \n");
#endif
   memcpy(&u8VirtMemory[u16VirtMemoryIndex],req->buf,req->actual);

#ifdef BULK_USBS_DEBUG
   {
      u32 i;
      u8 *ptr = req->buf;
      for (i=0;i<req->actual;i++)
         printk("%02x ",*(ptr+i));
      printk("\n");
   }
#endif
   u16VirtMemoryIndex += req->actual;
   tCSW.u32DataResidue -= req->actual;
   // printk("EP2 u32DataResidue:0x%x\n",tCSW.u32DataResidue);
   if (tCSW.u32DataResidue == 0)
      u32State= STATE_CSW;
   else
      u32State= STATE_CB_DATA_OUT;

   return u32State;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Interrupt IN/OUT transfer test functions
//
////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//		vUsb_Interrupt_Initial()
//		Description: Initial Interrupt test
//		input: none
//		output: None
///////////////////////////////////////////////////////////////////////////////

void vUsb_Interrupt_Initial(struct ftst_dev *dev)
{
	u8 u8i = 0;
	u16 u16i;
	struct usb_request	*req;
	u8 *u8Array;

	u8InterruptArray =  (u8 *)kmalloc(512,GFP_KERNEL);
	for (u16i = 0; u16i < 512; u16i ++)
		u8InterruptArray[u16i] = u8i ++;

	if((req=source_sink_start_ep (dev->intrin_ep, GFP_ATOMIC,u8Interrupt_TX_COUNT,3)) == NULL){
                usb_ep_disable (dev->intrin_ep);
                printk("Allocate error 6!!\n");
                return;
        }
	u8Array = (u8 *)req->buf;
	for (u8i = 0; u8i < u8Interrupt_TX_COUNT; u8i ++){
		u8Array[u8i] = (u8InterruptCount);
		u8InterruptCount ++;
	}
	u8Interrupt_TX_COUNT++;
	if(u8Interrupt_TX_COUNT > dev->intrin_ep->maxpacket)
		u8Interrupt_TX_COUNT = 1;

	if((req=source_sink_start_ep (dev->introut_ep, GFP_ATOMIC,u8Interrupt_RX_COUNT,4)) == NULL){
		usb_ep_disable (dev->introut_ep);
		printk("Allocate error 7!!\n");
		return;
	}
}

///////////////////////////////////////////////////////////////////////////////
//		vUsb_Interrupt_In()
//		Description: FIFO4 interrupt service process
//		input: none
//		output: none
///////////////////////////////////////////////////////////////////////////////
static void ftst_intrin_complete(struct usb_ep *ep, struct usb_request *req)
{
	u8 u8i;
	u8 *u8Array;
	struct ftst_dev *dev;
	struct FTC_ep		*my_ep;

#if IntTransSizeFix
	u8Interrupt_TX_COUNT = 13;
#endif
	free_ep_req(ep,req);

	my_ep = container_of(ep, struct FTC_ep, ep);
	if (!my_ep->stopped)
	   return;

	dev = (struct ftst_dev *)(ep->driver_data);
	if((req=source_sink_start_ep (dev->intrin_ep, GFP_ATOMIC,u8Interrupt_TX_COUNT,3)) == NULL){
		usb_ep_disable (dev->intrin_ep);
		printk("Allocate error 8!!\n");
		return;
	}

	u8Array = (u8 *)req->buf;
	for (u8i = 0; u8i < u8Interrupt_TX_COUNT; u8i ++){
		u8Array[u8i] = (u8InterruptCount);
		u8InterruptCount ++;
	}

//	if(u8UsbMessageLevel & MESS_INFO)
//		printf("L%x: Interrupt_IN...\n", u8LineCount ++);

	u8Interrupt_TX_COUNT++;
	if(u8Interrupt_TX_COUNT > ep->maxpacket)
		u8Interrupt_TX_COUNT = 1;
}

///////////////////////////////////////////////////////////////////////////////
//		vUsb_Interrupt_Out()
//		Description: FIFO4 interrupt service process
//		input: none
//		output: none
///////////////////////////////////////////////////////////////////////////////
static void ftst_introut_complete(struct usb_ep *ep, struct usb_request *req)
{
   u16 i,u16Interrupt_RX_Count = req->actual;
   u8 *u8Array;
   struct ftst_dev *dev;
   struct FTC_ep		*my_ep;

#if IntTransSizeFix
   u8Interrupt_RX_COUNT = 13;
#endif

   u8Array = (u8 *)req->buf;
   if(u16Interrupt_RX_Count != u8Interrupt_RX_COUNT)
      printk("L%x: Interrupt_Out Byte Count Error = %x...(Correct = %x)\n",
	      u8LineCount ++, u16Interrupt_RX_Count, u8Interrupt_RX_COUNT);

   if (memcmp(u8InterruptArray + u8InterruptOutCount, u8Array, u8Interrupt_RX_COUNT) != 0)
   {
      printk("L%x: Interrupt_Out Data error...\n", u8LineCount ++);

      printk("Device Get Data...");
      for (i = 0; i< u16Interrupt_RX_Count; i++) {
         if(i % 8 == 0)
            printk("\n");
            printk("%02x  ",u8Array[i]);
      }
      printk("\n");

      printk("Correct Data...");
      for (i = 0; i< u8Interrupt_RX_COUNT; i++){
      if(i % 8 == 0)
         printk("\n");
         printk("%02x  ", *(u8InterruptArray + u8InterruptOutCount + i));
      }
      printk("\n");
   }

   u8InterruptOutCount = u8InterruptOutCount + u8Interrupt_RX_COUNT;

   u8Interrupt_RX_COUNT++;
   if(u8Interrupt_RX_COUNT > ep->maxpacket)
      u8Interrupt_RX_COUNT = 1;

   free_ep_req(ep,req);

   my_ep = container_of(ep, struct FTC_ep, ep);
   if (!my_ep->stopped)
      return;

   dev = (struct ftst_dev *)(ep->driver_data);
   /*if((req=source_sink_start_ep (dev->introut_ep, GFP_ATOMIC,u8Interrupt_RX_COUNT,4)) == NULL){ */
   if ((req=source_sink_start_ep (dev->introut_ep, GFP_ATOMIC,64,4)) == NULL){
      usb_ep_disable (dev->introut_ep);
      printk("Allocate error 9!!\n");
      return;
   }
}

/* ////////////////////////////////////////////////////////////////////////////////
//
//  Functions for Enumeration
//
//////////////////////////////////////////////////////////////////////////////// */


/*-------------------------------------------------------------------------*/

static int
set_source_sink_config (struct ftst_dev *dev, unsigned gfp_flags)
{
	int			result = 0;
	struct usb_ep		*ep;
	struct usb_gadget	*gadget = dev->gadget;

	gadget_for_each_ep (ep, gadget) {
		const struct usb_endpoint_descriptor	*d;

		/* one endpoint writes (sources) zeroes in (to the host) */
		if (strcmp (ep->name, EP_BULK_IN_NAME) == 0) {
			d = ep_desc (gadget, &hs_bulkin_desc, &fs_bulkin_desc);
			ep->desc = d;
			result = usb_ep_enable (ep);
			if (result == 0) {
				ep->driver_data = dev;
				dev->bulkin_ep = ep;
				continue;
			}

		/* one endpoint reads (sinks) anything out (from the host) */
		} else if (strcmp (ep->name, EP_BULK_OUT_NAME) == 0) {
			d = ep_desc (gadget, &hs_bulkout_desc, &fs_bulkout_desc);
			ep->desc = d;
			result = usb_ep_enable (ep);
			if (result == 0) {
				ep->driver_data = dev;
				dev->bulkout_ep = ep;
				continue;
			}

		/* ignore any other endpoints */
		}else if (strcmp (ep->name, EP_INTR_IN_NAME) == 0) {
			d = ep_desc (gadget, &hs_intrin_desc, &fs_intrin_desc);
			ep->desc = d;
			result = usb_ep_enable (ep);
			if (result == 0) {
				ep->driver_data = dev;
				dev->intrin_ep = ep;
				continue;
				/*if (source_sink_start_ep (ep, gfp_flags,64,3) != 0) {
					dev->intrin_ep = ep;
					continue;
				}
				usb_ep_disable (ep);
				result = -EIO;*/
			}

		/* one endpoint reads (sinks) anything out (from the host) */
		} else if (strcmp (ep->name, EP_INTR_OUT_NAME) == 0) {
			d = ep_desc (gadget, &hs_introut_desc, &fs_introut_desc);
			ep->desc = d;
			result = usb_ep_enable (ep);
			if (result == 0) {
				ep->driver_data = dev;
				dev->introut_ep = ep;
				continue;
				/*if (source_sink_start_ep (ep, gfp_flags,64,4) != 0) {
					dev->introut_ep = ep;
					continue;
				}
				usb_ep_disable (ep);
				result = -EIO;*/
			}

		/* ignore any other endpoints */
		} else
			continue;

		/* stop on error */
		ERROR (dev, "can't start %s, result %d\n", ep->name, result);
		break;
	}
	if (result == 0)
		DBG (dev, "buflen %p\n", buflen);

	/* caller is responsible for cleanup on error */
	return result;
}

/*-------------------------------------------------------------------------*/
static void ftst_reset_config (struct ftst_dev *dev)
{
	if (dev->config == 0)
		return;

	DBG (dev, "reset config\n");

	/* just disable endpoints, forcing completion of pending i/o.
	 * all our completion handlers free their requests in this case.
	 */
	if (dev->bulkin_ep) {
		usb_ep_disable (dev->bulkin_ep);
		dev->bulkin_ep = NULL;
	}
	if (dev->bulkout_ep) {
		usb_ep_disable (dev->bulkout_ep);
		dev->bulkout_ep = NULL;
	}
	if (dev->intrin_ep) {
		usb_ep_disable (dev->intrin_ep);
		dev->intrin_ep = NULL;
	}
	if (dev->introut_ep) {
		usb_ep_disable (dev->introut_ep);
		dev->introut_ep = NULL;
	}
	dev->config = 0;
	del_timer (&dev->resume);
}

/* change our operational config.  this code must agree with the code
 * that returns config descriptors, and altsetting code.
 *
 * it's also responsible for power management interactions. some
 * configurations might not work with our current power sources.
 *
 * note that some device controller hardware will constrain what this
 * code can do, perhaps by disallowing more than one configuration or
 * by limiting configuration choices (like the pxa2xx).
 */
static int
ftst_set_config (struct ftst_dev *dev, unsigned number, unsigned gfp_flags)
{
	int			result = 0;
	struct usb_gadget	*gadget = dev->gadget;
	struct FTC_udc		*ftc_dev = container_of (dev->gadget, struct FTC_udc, gadget);

	if (number == dev->config)
		return 0;

#define	gadget_is_sa1100(g)	(!strcmp("sa1100_udc", (g)->name))
	if (gadget_is_sa1100 (gadget) && dev->config) {
		/* tx fifo is full, but we can't clear it...*/
		INFO (dev, "can't change configurations\n");
		return -ESPIPE;
	}
	ftst_reset_config (dev);

	switch (number) {
	case CONFIG_SOURCE_SINK:
		result = set_source_sink_config (dev, gfp_flags);
		vUsb_APInit();
		vUsb_queue_reqest(dev,eUsbMassStorageState);
		vUsb_Interrupt_Initial(dev);
		mUsbFIFOReset(ftc_dev->va_base,2);
		break;
	default:
		result = -EINVAL;
		/* FALL THROUGH */
	case 0:
		return result;
	}

	if (!result && (!dev->bulkin_ep || !dev->bulkout_ep))
		result = -ENODEV;
	if (result)
		ftst_reset_config (dev);
	else {
		char *speed;

		switch (gadget->speed) {
		case USB_SPEED_LOW:	speed = "low"; break;
		case USB_SPEED_FULL:	speed = "full"; break;
		case USB_SPEED_HIGH:	speed = "high"; break;
		default: 		speed = "?"; break;
		}

		dev->config = number;
		INFO (dev, "%s speed config #%d: %s\n", speed, number,
					 source_sink );
	}
	return result;
}

/*-------------------------------------------------------------------------*/

static void ftst_setup_complete (struct usb_ep *ep, struct usb_request *req)
{
	if (req->status || req->actual != req->length)
		DBG ((struct ftst_dev *) ep->driver_data,
				"setup complete --> %d, %d/%d\n",
				req->status, req->actual, req->length);
}

/*
 * The setup() callback implements all the ep0 functionality that's
 * not handled lower down, in hardware or the hardware driver (like
 * device and endpoint feature flags, and their status).  It's all
 * housekeeping for the gadget function we're implementing.  Most of
 * the work is in config-specific setup.
 */
static int
ftst_setup (struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	struct ftst_dev		*dev = get_gadget_data (gadget);
	struct usb_request	*req = dev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	/* usually this stores reply data in the pre-allocated ep0 buffer,
	 * but config change events will reconfigure hardware.
	 */

	req->zero = 0;
	switch (ctrl->bRequest) {

	case USB_REQ_GET_DESCRIPTOR:
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;
		switch (w_value >> 8) {

		case USB_DT_DEVICE:
			value = min (w_length, (u16) sizeof device_desc);
			memcpy (req->buf, &device_desc, value);
			break;
		case USB_DT_DEVICE_QUALIFIER:
			value = min (w_length, (u16) sizeof dev_qualifier);
			memcpy (req->buf, &dev_qualifier, value);
			break;

		case USB_DT_OTHER_SPEED_CONFIG:
		case USB_DT_CONFIG:
			value = config_buf (gadget, req->buf,
					w_value >> 8,
					w_value & 0xff);
			if (value >= 0)
				value = min (w_length, (u16) value);
			break;

		case USB_DT_STRING:
			/* wIndex == language code.
			 * this driver only handles one language, you can
			 * add string tables for other languages, using
			 * any UTF-8 characters
			 */
			value = usb_gadget_get_string (&stringtab,
					w_value & 0xff, req->buf);
			if (value >= 0)
				value = min (w_length, (u16) value);
			break;
		}
		break;

	/* currently two configs, two speeds */
	case USB_REQ_SET_CONFIGURATION:
		if (ctrl->bRequestType != 0)
			goto unknown;
		if (gadget->a_hnp_support)
			DBG (dev, "HNP available\n");
		else if (gadget->a_alt_hnp_support)
			DBG (dev, "HNP needs a different root port\n");
		else
			VDBG (dev, "HNP inactive\n");
		spin_lock (&dev->lock);
		value = ftst_set_config (dev, w_value, GFP_ATOMIC);
		spin_unlock (&dev->lock);
		break;
	case USB_REQ_GET_CONFIGURATION:
		if (ctrl->bRequestType != USB_DIR_IN)
			goto unknown;
		*(u8 *)req->buf = dev->config;
		value = min (w_length, (u16) 1);
		break;

	/* until we add altsetting support, or other interfaces,
	 * only 0/0 are possible.  pxa2xx only supports 0/0 (poorly)
	 * and already killed pending endpoint I/O.
	 */
	case USB_REQ_SET_INTERFACE:
		if (ctrl->bRequestType != USB_RECIP_INTERFACE)
			goto unknown;
		spin_lock (&dev->lock);
		if (dev->config && w_index == 0 && w_value == 0) {
			u8		config = dev->config;

			/* resets interface configuration, forgets about
			 * previous transaction state (queued bufs, etc)
			 * and re-inits endpoint state (toggle etc)
			 * no response queued, just zero status == success.
			 * if we had more than one interface we couldn't
			 * use this "reset the config" shortcut.
			 */
			ftst_reset_config (dev);
			ftst_set_config (dev, config, GFP_ATOMIC);
			value = 0;
		}
		spin_unlock (&dev->lock);
		break;
	case USB_REQ_GET_INTERFACE:
		if (ctrl->bRequestType != (USB_DIR_IN|USB_RECIP_INTERFACE))
			goto unknown;
		if (!dev->config)
			break;
		if (w_index != 0) {
			value = -EDOM;
			break;
		}
		*(u8 *)req->buf = 0;
		value = min (w_length, (u16) 1);
		break;

	/*
	 * These are the same vendor-specific requests supported by
	 * Intel's USB 2.0 compliance test devices.  We exceed that
	 * device spec by allowing multiple-packet requests.
	 */
	case 0x5b:	/* control WRITE test -- fill the buffer */
		if (ctrl->bRequestType != (USB_DIR_OUT|USB_TYPE_VENDOR))
			goto unknown;
		if (w_value || w_index)
			break;
		/* just read that many bytes into the buffer */
		if (w_length > USB_BUFSIZ)
			break;
		value = w_length;
		break;
	case 0x5c:	/* control READ test -- return the buffer */
		if (ctrl->bRequestType != (USB_DIR_IN|USB_TYPE_VENDOR))
			goto unknown;
		if (w_value || w_index)
			break;
		/* expect those bytes are still in the buffer; send back */
		if (w_length > USB_BUFSIZ
				|| w_length != req->length)
			break;
		value = w_length;
		break;

	default:
unknown:
		VDBG (dev,
			"unknown control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	/* respond with data transfer before status phase? */
	if (value >= 0) {
		req->length = value;
		req->zero = value < w_length;
		value = usb_ep_queue (gadget->ep0, req, GFP_ATOMIC);
		if (value < 0) {
			DBG (dev, "ep_queue --> %d\n", value);
			req->status = 0;
			ftst_setup_complete (gadget->ep0, req);
		}
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}

static void
ftst_disconnect (struct usb_gadget *gadget)
{
	struct ftst_dev		*dev = get_gadget_data (gadget);
	unsigned long		flags;


	spin_lock_irqsave (&dev->lock, flags);
	ftst_reset_config (dev);

	/* a more significant application might have some non-usb
	 * activities to quiesce here, saving resources like power
	 * or pushing the notification up a network stack.
	 */
	spin_unlock_irqrestore (&dev->lock, flags);

	/* next we may get setup() calls to enumerate new connections;
	 * or an unbind() during shutdown (including removing module).
	 */
}

static void
ftst_autoresume (unsigned long _dev)
{
	struct ftst_dev	*dev = (struct ftst_dev *) _dev;
	int		status;

	/* normally the host would be woken up for something
	 * more significant than just a timer firing...
	 */
	if (dev->gadget->speed != USB_SPEED_UNKNOWN) {
		status = usb_gadget_wakeup (dev->gadget);
		DBG (dev, "wakeup --> %d\n", status);
	}
}

/*-------------------------------------------------------------------------*/

static void
ftst_unbind (struct usb_gadget *gadget)
{
	struct ftst_dev		*dev = get_gadget_data (gadget);

	DBG (dev, "unbind\n");

	/* we've already been disconnected ... no i/o is active */
	if (dev->req) {
	   //free_ep_req (gadget->ep0, dev->req);
	   struct FTC_request	*my_req;
	   struct FTC_ep	*my_ep;

	   if (dev->req->buf) {
	      my_ep = container_of(gadget->ep0, struct FTC_ep, ep);
	      my_req = container_of(dev->req, struct FTC_request, req);
	      kfree(my_req->req.buf);
	      my_ep->dma=0;
	   }

	   usb_ep_free_request (gadget->ep0, dev->req);
        }

	del_timer_sync (&dev->resume);
	kfree (dev);
	set_gadget_data (gadget, NULL);
}

static int __devinit ftst_bind (struct usb_gadget *gadget)
{
	struct ftst_dev		*dev;
	struct usb_ep		*ep;
	int			gcnum;

	/* FIXME this can't yet work right with SH ... it has only
	 * one configuration, numbered one.
	 */
#define	gadget_is_sh(g)		(!strcmp("sh_udc", (g)->name))
	if (gadget_is_sh(gadget))
		return -ENODEV;

	/* Bulk-only drivers like this one SHOULD be able to
	 * autoconfigure on any sane usb controller driver,
	 * but there may also be important quirks to address.
	 */
	usb_ep_autoconfig_reset (gadget);
	ep = usb_ep_autoconfig (gadget, &fs_bulkin_desc);
	if (!ep) {
autoconf_fail:
		printk (KERN_ERR "%s: can't autoconfigure on %s\n",
			shortname, gadget->name);
		return -ENODEV;
	}
	EP_BULK_IN_NAME = ep->name;
	ep->driver_data = ep;	/* claim */
	fotg2xx_dbg(" ftst_bind: %s %d\n",ep->name,ep->maxpacket);

	ep = usb_ep_autoconfig (gadget, &fs_bulkout_desc);
	if (!ep)
		goto autoconf_fail;
	EP_BULK_OUT_NAME = ep->name;
	ep->driver_data = ep;	/* claim */
	fotg2xx_dbg(" ftst_bind: %s %d\n",ep->name,ep->maxpacket);

	ep = usb_ep_autoconfig (gadget, &fs_intrin_desc);
	if (!ep)
		goto autoconf_fail;
	EP_INTR_IN_NAME = ep->name;
	ep->driver_data = ep;	/* claim */
	fotg2xx_dbg(" ftst_bind: %s %d\n",ep->name,ep->maxpacket);

	ep = usb_ep_autoconfig (gadget, &fs_introut_desc);
	if (!ep)
		goto autoconf_fail;
	EP_INTR_OUT_NAME = ep->name;
	ep->driver_data = ep;	/* claim */
	fotg2xx_dbg(" ftst_bind: %s %s %d\n",gadget->name, ep->name,ep->maxpacket);

	gcnum = usb_gadget_controller_number (gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16 (0x0200 + gcnum);
	else {
		/* gadget zero is so simple (for now, no altsettings) that
		 * it SHOULD NOT have problems with bulk-capable hardware.
		 * so warn about unrcognized controllers, don't panic.
		 *
		 * things like configuration and altsetting numbering
		 * can need hardware-specific attention though.
		 */
		printk (KERN_WARNING "%s: controller '%s' not recognized\n",
			shortname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16 (0x9999);
	}


	/* ok, we made sense of the hardware ... */
	dev = kzalloc (sizeof *dev, GFP_KERNEL);
	if (!dev)
		return -ENOMEM;
	//memset (dev, 0, sizeof *dev);
	spin_lock_init (&dev->lock);
	dev->gadget = gadget;
	set_gadget_data (gadget, dev);

	/* preallocate control response and buffer */
	dev->req = usb_ep_alloc_request (gadget->ep0, GFP_KERNEL);
	if (!dev->req)
		goto enomem;

	//dev->req->buf = usb_ep_alloc_buffer (gadget->ep0, USB_BUFSIZ,
	//			&dev->req->dma, GFP_KERNEL);
/*	dev->req->buf = kmalloc(USB_BUFSIZ, GFP_KERNEL);
	if (!dev->req->buf)
		goto enomem;
*/
        {
	   struct FTC_ep	*my_ep;
	   struct FTC_request	*my_req;

       	   my_ep = container_of(gadget->ep0, struct FTC_ep, ep);
	   my_req = container_of(dev->req, struct FTC_request, req);
	   //my_req->req.buf = dma_alloc_coherent(&my_ep->dev->gadget.dev,USB_BUFSIZ,&(my_req->req.dma),GFP_KERNEL);
	   my_req->req.buf = kmalloc(USB_BUFSIZ,GFP_KERNEL);

	   if (!my_req->req.buf) {
              usb_ep_free_request(gadget->ep0,dev->req);
              goto enomem;
           }

	   my_req->req.length = USB_BUFSIZ;
	   my_ep->dma=1;
	   printk("device %x== alloc buffer = 0x%x\n", (u32)&my_ep->dev->gadget.dev, (u32) my_req->req.buf);
	}

	dev->req->complete = ftst_setup_complete;
	device_desc.bMaxPacketSize0 = gadget->ep0->maxpacket;

	/* assume ep0 uses the same value for both speeds ... */
	dev_qualifier.bMaxPacketSize0 = device_desc.bMaxPacketSize0;

	/* and that all endpoints are dual-speed */
	hs_bulkin_desc.bEndpointAddress = fs_bulkin_desc.bEndpointAddress;
	hs_bulkout_desc.bEndpointAddress = fs_bulkout_desc.bEndpointAddress;
	hs_intrin_desc.bEndpointAddress = fs_intrin_desc.bEndpointAddress;
	hs_introut_desc.bEndpointAddress = fs_introut_desc.bEndpointAddress;

	if (gadget->is_otg) {
		otg_descriptor.bmAttributes |= USB_OTG_HNP,
		source_sink_config.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

	if (gadget->is_otg) {
		otg_descriptor.bmAttributes |= USB_OTG_HNP,
		source_sink_config.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

	usb_gadget_set_selfpowered (gadget);

	init_timer (&dev->resume);
	dev->resume.function = ftst_autoresume;
	dev->resume.data = (unsigned long) dev;
	if (autoresume) {
		source_sink_config.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

	gadget->ep0->driver_data = dev;

	INFO (dev, "%s, version: " DRIVER_VERSION "\n", longname);
	INFO (dev, "using %s, BULKIN %s BULKOUT %s \n INTRIN %s INTROUT %s\n", gadget->name,
		EP_BULK_IN_NAME, EP_BULK_OUT_NAME,EP_INTR_IN_NAME,EP_INTR_OUT_NAME);

	snprintf (manufacturer, sizeof manufacturer, " with %s",  //"%s %s with %s",
		//system_utsname.sysname, system_utsname.release,
		gadget->name);

	return 0;

enomem:
	ftst_unbind (gadget);
	return -ENOMEM;
}

/*-------------------------------------------------------------------------*/

static void
ftst_suspend (struct usb_gadget *gadget)
{
   struct ftst_dev		*dev = get_gadget_data (gadget);

   if (gadget->speed == USB_SPEED_UNKNOWN)
      return;

   if (autoresume) {
      mod_timer (&dev->resume, jiffies + (HZ * autoresume));
      DBG (dev, "suspend, wakeup in %d seconds\n", autoresume);
   }
   else
      DBG (dev, "suspend\n");
}

static void
ftst_resume (struct usb_gadget *gadget)
{
   struct ftst_dev		*dev = get_gadget_data (gadget);

   DBG (dev, "resume\n");
   del_timer (&dev->resume);
}

/*-------------------------------------------------------------------------*/

static struct usb_gadget_driver ftst_driver = {
	.max_speed	= USB_SPEED_HIGH,
	.function	= (char *) longname,
	.unbind		= ftst_unbind,

	.setup		= ftst_setup,
	.disconnect	= ftst_disconnect,

	.suspend	= ftst_suspend,
	.resume		= ftst_resume,

	.driver 	= {
		.name		= (char *) shortname,
		// .shutdown = ...
		// .suspend = ...
		// .resume = ...
	},
};

MODULE_AUTHOR ("John/CTD");
MODULE_LICENSE ("Dual BSD/GPL");

static int __init init (void)
{
	/* a real value would likely come through some id prom
	 * or module option.  this one takes at least two packets.
	 */
	strlcpy (serial, "0123456789012345678901234567890123", sizeof serial);

	return usb_gadget_probe_driver (&ftst_driver, ftst_bind);
}
module_init (init);

static void __exit cleanup (void)
{
	usb_gadget_unregister_driver (&ftst_driver);
}
module_exit (cleanup);
