/*
* amb_stream.c -- Ambarella Data Streaming Gadget
*
* History:
*	2009/01/01 - [Cao Rongrong] created file
*
* Copyright (C) 2008 by Ambarella, Inc.
* http://www.ambarella.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*/


/*
 * Ambarella Data Streaming Gadget only needs two bulk endpoints, and it
 * supports single configurations.
 *
 * Module options include:
 *   buflen=N	default N=4096, buffer size used
 *
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>

#include <linux/usb/composite.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <asm/uaccess.h>

#include "gadget_chips.h"

#include <mach/hardware.h>

USB_GADGET_COMPOSITE_OPTIONS();

#define DRIVER_VERSION		"15 July 2016"

static const char shortname[] = "g_amb_stream";
static const char longname[] = "Ambarella Data Streaming Gadget";

/* ==========================================================================*/
#define	AMBA_DEV_MAJOR			(248)
#define	AMBA_DEV_MINOR_PUBLIC_START	(128)
#define	AMBA_DEV_MINOR_PUBLIC_END	(240)
/*-------------------------------------------------------------------------*/
#ifdef DEBUG
#define AMB_DBG(dev,fmt,args...) \
	dev_printk(KERN_DEBUG , &(dev)->gadget->dev , fmt , ## args)
#else
#define AMB_DBG(dev,fmt,args...) \
	do { } while (0)
#endif /* DEBUG */

#define AMB_ERROR(dev,fmt,args...) \
	dev_printk(KERN_ERR , &(dev)->gadget->dev , fmt , ## args)

#define AMB_INFO(dev,fmt,args...) \
	dev_printk(KERN_INFO , &(dev)->gadget->dev , fmt , ## args)
/*-------------------------------------------------------------------------*/
#define AMB_GADGET_MAJOR			AMBA_DEV_MAJOR
#define AMB_GADGET_MINOR_START		(AMBA_DEV_MINOR_PUBLIC_START + 0)
/*-------------------------------------------------------------------------*/
static struct cdev ag_cdev;

/* big enough to hold our biggest descriptor */
#define USB_BUFSIZ	256
/*-------------------------------------------------------------------------*/
#define AG_NOTIFY_INTERVAL		5	/* 1 << 5 == 32 msec */
#define AG_NOTIFY_MAXPACKET		8

#define SIMPLE_CMD_SIZE		32
struct amb_cmd {
	u32 signature;
	u32 command;
	u32 parameter[(SIMPLE_CMD_SIZE / sizeof(u32)) - 2];
};

struct amb_rsp {
	u32 signature;
	u32 response;
	u32 parameter0;
	u32 parameter1;
};

struct amb_ack {
	u32 signature;
	u32 acknowledge;
	u32 parameter0;
	u32 parameter1;
};

/* for bNotifyType */
#define PORT_STATUS_CHANGE		0x55
#define PORT_NOTIFY_IDLE		0xff
/* for value */
#define PORT_FREE_ALL			2
#define PORT_CONNECT			1
#define PORT_NO_CONNECT		0
/* for status */
#define DEVICE_OPENED			1
#define DEVICE_NOT_OPEN		0

struct amb_notify {
	u16	bNotifyType;
	u16	port_id;
	u16	value;
	u16	status;
};

struct amb_notify g_port = {
	.bNotifyType = PORT_NOTIFY_IDLE,
	.port_id = 0xffff,
	.value = 0,
};

/*-------------------------------------------------------------------------*/
#define AMB_DATA_STREAM_MAGIC	'u'
#define AMB_DATA_STREAM_WR_RSP	_IOW(AMB_DATA_STREAM_MAGIC, 1, struct amb_rsp *)
#define AMB_DATA_STREAM_RD_CMD	_IOR(AMB_DATA_STREAM_MAGIC, 1, struct amb_cmd *)
#define AMB_DATA_STREAM_STATUS_CHANGE	_IOW(AMB_DATA_STREAM_MAGIC, 2, struct amb_notify *)
/*-------------------------------------------------------------------------*/
struct amb_dev {
	spinlock_t		lock;
	wait_queue_head_t	wq;
	struct mutex		mtx;

	int 				config;
	struct usb_gadget	*gadget;
	struct usb_request	*notify_req;

	struct usb_ep		*in_ep;
	struct usb_ep		*out_ep;
	struct usb_ep		*notify_ep;

	struct list_head		in_idle_list;	/* list of idle write requests */
	struct list_head		in_queue_list;	/* list of queueing write requests */
	struct list_head		out_req_list;	/* list of bulk out requests */

	struct usb_function		func;

	int			open_count;
	int			error;
	char		interface;
};

struct amb_dev *ag_device;
static dev_t ag_dev_id;
static struct class *ag_class;
/*-------------------------------------------------------------------------*/
static void notify_worker(struct work_struct *work);
static DECLARE_WORK(notify_work, notify_worker);
/*-------------------------------------------------------------------------*/
static unsigned int buflen = (48*1024);
module_param (buflen, uint, S_IRUGO);
MODULE_PARM_DESC(buflen, "buffer length, default=48K");

static unsigned int qdepth = 5;
module_param (qdepth, uint, S_IRUGO);
MODULE_PARM_DESC(qdepth, "bulk transfer queue depth, default=5");
/*-------------------------------------------------------------------------*/
/*
 * DESCRIPTORS ... most are static, but strings and (full)
 * configuration descriptors are built on demand.
 */

#define STRING_SOURCE_SINK		USB_GADGET_FIRST_AVAIL_IDX

#define DRIVER_VENDOR_ID	0x4255		/* Ambarella */
#define DRIVER_PRODUCT_ID	0x0001

#define DEV_CONFIG_VALUE		1

static struct usb_device_descriptor ag_device_desc = {
	.bLength =		sizeof ag_device_desc,
	.bDescriptorType =	USB_DT_DEVICE,

	.bcdUSB =		cpu_to_le16 (0x0200),
	.bDeviceClass =		USB_CLASS_VENDOR_SPEC,

	.idVendor =		cpu_to_le16 (DRIVER_VENDOR_ID),
	.idProduct =		cpu_to_le16 (DRIVER_PRODUCT_ID),
	.bNumConfigurations =	1,
};

static struct usb_config_descriptor amb_bulk_config = {
	.bLength =		sizeof amb_bulk_config,
	.bDescriptorType =	USB_DT_CONFIG,

	.bNumInterfaces =	1,
	.bConfigurationValue =	DEV_CONFIG_VALUE,
	.iConfiguration =	STRING_SOURCE_SINK,
	.bmAttributes =		USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
};

/* one interface in each configuration */
static struct usb_interface_descriptor amb_data_stream_intf = {
	.bLength =		sizeof amb_data_stream_intf,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bNumEndpoints =	3,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	.iInterface =		STRING_SOURCE_SINK,
};

/* two full speed bulk endpoints; their use is config-dependent */

static struct usb_endpoint_descriptor fs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_intr_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(AG_NOTIFY_MAXPACKET),
	.bInterval =		1 << AG_NOTIFY_INTERVAL,
};

static struct usb_descriptor_header *fs_amb_data_stream_function[] = {
	(struct usb_descriptor_header *) &amb_data_stream_intf,
	(struct usb_descriptor_header *) &fs_bulk_out_desc,
	(struct usb_descriptor_header *) &fs_bulk_in_desc,
	(struct usb_descriptor_header *) &fs_intr_in_desc,
	NULL,
};

static struct usb_endpoint_descriptor hs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	/* bEndpointAddress will be copied from fs_bulk_in_desc during amb_bind() */
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16 (512),
};

static struct usb_endpoint_descriptor hs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16 (512),
};

static struct usb_endpoint_descriptor hs_intr_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(AG_NOTIFY_MAXPACKET),
	.bInterval =		AG_NOTIFY_INTERVAL + 4,
};

static struct usb_qualifier_descriptor dev_qualifier = {
	.bLength =		sizeof dev_qualifier,
	.bDescriptorType =	USB_DT_DEVICE_QUALIFIER,

	.bcdUSB =		cpu_to_le16 (0x0200),
	.bDeviceClass =		USB_CLASS_VENDOR_SPEC,

	.bNumConfigurations =	1,
};

static struct usb_descriptor_header *hs_amb_data_stream_function[] = {
	(struct usb_descriptor_header *) &amb_data_stream_intf,
	(struct usb_descriptor_header *) &hs_bulk_in_desc,
	(struct usb_descriptor_header *) &hs_bulk_out_desc,
	(struct usb_descriptor_header *) &hs_intr_in_desc,
	NULL,
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,
	.bmAttributes =		USB_OTG_SRP,
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};
/*-------------------------------------------------------------------------*/
static char serial[40] = "123456789ABC";

/* static strings, in UTF-8 */
static struct usb_string		strings[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "",
	[USB_GADGET_PRODUCT_IDX].s = longname,
	[USB_GADGET_SERIAL_IDX].s = serial,
	[STRING_SOURCE_SINK].s = "bulk in and out data",
	{  }			/* end of list */
};

static struct usb_gadget_strings	stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings,
};

static struct usb_gadget_strings	*dev_strings[] = {
	&stringtab_dev,
	NULL,
};

/*-------------------------------------------------------------------------*/
/* add a request to the tail of a list */
void req_put(struct amb_dev *dev,
		struct list_head *head, struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* get a request and remove it from the head of a list */
struct usb_request *req_get(struct amb_dev *dev, struct list_head *head)
{
	struct usb_request *req;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = NULL;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

/* get a request and move it from the head of "from" list
  * to the head of "to" list
  */
struct usb_request *req_move(struct amb_dev *dev,
		struct list_head *from, struct list_head *to)
{
	struct usb_request *req;
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(from)) {
		req = NULL;
	} else {
		req = list_first_entry(from, struct usb_request, list);
		list_move_tail(&req->list, to);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

static int amb_send(u8 *buf, u32 len)
{
	struct amb_dev *dev = ag_device;
	struct usb_request *req = NULL;
	int rval = 0;

	if (buf == NULL || len > buflen)
		return -EFAULT;

	if(wait_event_interruptible(dev->wq,
		(req = req_move(dev, &dev->in_idle_list, &dev->in_queue_list))
		|| dev->error)){
		rval = -EINTR;
		goto exit;
	}

	if(dev->error) {
		AMB_ERROR(dev, "%s: device error", __func__);
		spin_lock_irq(&dev->lock);
		if (req)
			list_move_tail(&req->list, &dev->in_idle_list);
		spin_unlock_irq(&dev->lock);
		rval = -EIO;
		goto exit;
	}

	memcpy(req->buf, buf, len);

	req->length = len;
	rval = usb_ep_queue(dev->in_ep, req, GFP_ATOMIC);
	if (rval != 0) {
		AMB_ERROR(dev, "%s: cannot queue bulk in request, "
			"rval=%d\n", __func__, rval);
		spin_lock_irq(&dev->lock);
		list_move_tail(&req->list, &dev->in_idle_list);
		spin_unlock_irq(&dev->lock);
		goto exit;
	}

exit:
	return rval;
}

static int amb_recv(u8 *buf, u32 len)
{
	struct amb_dev *dev = ag_device;
	struct usb_request *req = NULL;
	int rval = 0;

	if (buf == NULL || len > buflen)
		return -EFAULT;

	if(wait_event_interruptible(dev->wq,
		(req = req_get(dev, &dev->out_req_list)) || dev->error)){
		return -EINTR;
	}

	if (req) {
		memcpy(buf, req->buf, req->actual);

		req->length = buflen;
		if ((rval = usb_ep_queue(dev->out_ep, req, GFP_ATOMIC))) {
			AMB_ERROR(dev, "%s: can't queue bulk out request, "
				"rval = %d\n", __func__, rval);
		}
	}

	if(dev->error) {
		AMB_ERROR(dev, "%s: device error", __func__);
		rval = -EIO;
	}

	return rval;
}


/*-------------------------------------------------------------------------*/
static long ag_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rval = 0;
	struct amb_dev *dev = ag_device;
	struct amb_cmd _cmd;
	struct amb_rsp rsp;
	struct amb_notify notify;

	AMB_DBG(dev, "%s: Enter\n", __func__);

	mutex_lock(&dev->mtx);

	switch (cmd) {
	case AMB_DATA_STREAM_WR_RSP:

		if(copy_from_user(&rsp, (struct amb_rsp __user *)arg, sizeof(rsp))){
			mutex_unlock(&dev->mtx);
			return -EFAULT;
		}

		rval = amb_send((u8 *)&rsp, sizeof(rsp));
		break;

	case AMB_DATA_STREAM_RD_CMD:

		rval = amb_recv((u8 *)&_cmd, sizeof(_cmd));
		if(rval < 0)
			break;

		if(copy_to_user((struct amb_cmd __user *)arg, &_cmd, sizeof(_cmd))){
			mutex_unlock(&dev->mtx);
			return -EFAULT;
		}

		break;

	case AMB_DATA_STREAM_STATUS_CHANGE:
		if(copy_from_user(&notify, (struct amb_notify __user *)arg, sizeof(notify))){
			mutex_unlock(&dev->mtx);
			return -EFAULT;
		}

		spin_lock_irq(&dev->lock);
		g_port.bNotifyType = notify.bNotifyType;
		g_port.port_id = notify.port_id;
		g_port.value = notify.value;
		spin_unlock_irq(&dev->lock);
		break;

	default:
		rval = -ENOIOCTLCMD;
		break;
	}

	mutex_unlock(&dev->mtx);

	AMB_DBG(dev, "%s: Exit\n", __func__);

	return rval;
}
static int ag_open(struct inode *inode, struct file *filp)
{
	struct amb_dev *dev = ag_device;
	struct usb_request *req = NULL;
	int rval = 0;

	mutex_lock(&dev->mtx);

	/* gadget have not been configured */
	if(dev->config == 0){
		rval = -ENOTCONN;
		goto exit;
	}

	if(dev->open_count > 0){
		rval = -EBUSY;
		goto exit;
	}

	dev->open_count++;
	dev->error = 0;

	/* throw away the data received on last connection */
	while ((req = req_get(dev, &dev->out_req_list))) {
		/* re-use the req again */
		req->length = buflen;
		if ((rval = usb_ep_queue(dev->out_ep, req, GFP_ATOMIC))) {
			AMB_ERROR(dev, "%s: can't queue bulk out request, "
				"rval = %d\n", __func__, rval);
			break;
		}
	}
exit:
	mutex_unlock(&dev->mtx);
	return rval;
}

static int ag_release(struct inode *inode, struct file *filp)
{
	struct amb_dev *dev = ag_device;
	struct usb_request *req;
	int rval = 0;

	mutex_lock(&dev->mtx);

	/* dequeue the bulk-in request */
	while ((req = req_move(dev, &dev->in_queue_list, &dev->in_idle_list))) {
		rval = usb_ep_dequeue(dev->in_ep, req);
		if (rval != 0) {
			AMB_ERROR(dev, "%s: cannot dequeue bulk in request, "
				"rval=%d\n", __func__, rval);
			break;
		}
	}

	dev->open_count--;

	spin_lock_irq(&dev->lock);
	g_port.bNotifyType = PORT_STATUS_CHANGE;
	g_port.port_id = 0xffff;
	g_port.value = PORT_FREE_ALL;
	g_port.status = DEVICE_NOT_OPEN;
	spin_unlock_irq(&dev->lock);

	mutex_unlock(&dev->mtx);

	return rval;
}

static int ag_read(struct file *file, char __user *buf,
	size_t count, loff_t *ppos)
{
	struct amb_dev *dev = ag_device;
	struct usb_request *req = NULL;
	int len = 0, rval = 0;

	mutex_lock(&dev->mtx);

	while(count > 0){
		if(wait_event_interruptible(dev->wq,
			(req = req_get(dev, &dev->out_req_list)) || dev->error)){
			mutex_unlock(&dev->mtx);
			return -EINTR;
		}

		if(dev->error){
			AMB_ERROR(dev, "%s: device error", __func__);
			if (req) {
				req->length = buflen;
				rval = usb_ep_queue(dev->out_ep, req, GFP_ATOMIC);
				if (rval)
					AMB_ERROR(dev, "%s: "
						"can't queue bulk out request, "
						"rval = %d\n", __func__, rval);
			}
			mutex_unlock(&dev->mtx);
			return -EIO;
		}

		if(copy_to_user(buf + len, req->buf, req->actual)){
			pr_err("len = %d, actual = %d\n", len, req->actual);
			mutex_unlock(&dev->mtx);
			return -EFAULT;
		}
		len += req->actual;
		count -= req->actual;

		req->length = buflen;
		if ((rval = usb_ep_queue(dev->out_ep, req, GFP_ATOMIC))) {
			AMB_ERROR(dev, "%s: can't queue bulk out request, "
				"rval = %d\n", __func__, rval);
			len = rval;
			break;
		}

		if (len % buflen != 0)
			break;
	}

	mutex_unlock(&dev->mtx);

	return len;
}

static int ag_write(struct file *file, const char __user *buf,
	size_t count, loff_t *ppos)
{
	int rval, size, len = 0;
	struct amb_dev *dev = ag_device;
	struct usb_request *req = NULL;

	mutex_lock(&dev->mtx);

	while(count > 0) {
		if(wait_event_interruptible(dev->wq,
			(req = req_move(dev, &dev->in_idle_list, &dev->in_queue_list))
			|| dev->error)){
			mutex_unlock(&dev->mtx);
			return -EINTR;
		}

		if(dev->error){
			AMB_ERROR(dev, "%s: device error", __func__);
			spin_lock_irq(&dev->lock);
			if (req)
				list_move_tail(&req->list, &dev->in_idle_list);
			spin_unlock_irq(&dev->lock);
			mutex_unlock(&dev->mtx);
			return -EIO;
		}

		size = count < buflen ? count : buflen;
		if(copy_from_user(req->buf, buf + len, size)){
			mutex_unlock(&dev->mtx);
			return -EFAULT;
		}

		req->length = size;
		if ((count - size == 0) && (size % dev->in_ep->maxpacket == 0))
			req->zero = 1;

		rval = usb_ep_queue(dev->in_ep, req, GFP_ATOMIC);
		if (rval != 0) {
			AMB_ERROR(dev, "%s: cannot queue bulk in request, "
				"rval=%d\n", __func__, rval);
			list_move_tail(&req->list, &dev->in_idle_list);
			mutex_unlock(&dev->mtx);
			return rval;
		}

		count -= size;
		len += size;
	}

	mutex_unlock(&dev->mtx);

	AMB_DBG(dev, "%s: Exit\n", __func__);

	return len;
}

static const struct file_operations ag_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ag_ioctl,
	.open = ag_open,
	.read = ag_read,
	.write = ag_write,
	.release = ag_release
};

static void amb_notify_complete (struct usb_ep *ep, struct usb_request *req);

static void notify_worker(struct work_struct *work)
{
	struct amb_dev		*dev = ag_device;
	struct usb_request	*req = NULL;
	struct amb_notify	*event = NULL;
	unsigned long flags;
	int rval = 0;

	if (dev && (req = dev->notify_req)) {
		event = req->buf;
		spin_lock_irqsave(&dev->lock, flags);
		memcpy(event, &g_port, sizeof(struct amb_notify));
		g_port.bNotifyType = PORT_NOTIFY_IDLE;
		g_port.port_id = 0xffff;
		g_port.value = 0;
		if(dev->open_count > 0)
			g_port.status = DEVICE_OPENED;
		else
			g_port.status = DEVICE_NOT_OPEN;

		req->length = AG_NOTIFY_MAXPACKET;
		req->complete = amb_notify_complete;
		req->context = dev;
		req->zero = !(req->length % dev->notify_ep->maxpacket);
		spin_unlock_irqrestore(&dev->lock, flags);

		rval = usb_ep_queue(dev->notify_ep, req, GFP_ATOMIC);
		if (rval < 0)
			AMB_DBG(dev, "status buf queue --> %d\n", rval);
	}
}

static struct usb_request *amb_alloc_buf_req (struct usb_ep *ep,
		unsigned length, gfp_t kmalloc_flags)
{
	struct usb_request	*req;

	req = usb_ep_alloc_request(ep, kmalloc_flags);
	if (req) {
		req->length = length;
		req->buf = kmalloc(length, kmalloc_flags);
		if (!req->buf) {
			usb_ep_free_request(ep, req);
			req = NULL;
		}
	}
	return req;
}

static void amb_free_buf_req (struct usb_ep *ep, struct usb_request *req)
{
	if(req){
		if (req->buf){
			kfree(req->buf);
			req->buf = NULL;
		}
		usb_ep_free_request(ep, req);
	}
}

static void amb_bulk_in_complete (struct usb_ep *ep, struct usb_request *req)
{
	struct amb_dev	*dev = ep->driver_data;
	int		status = req->status;
	unsigned long	flags;
	int 		rval = 0;

	spin_lock_irqsave(&dev->lock, flags);
	switch (status) {
	case 0: 			/* normal completion? */
		dev->error = 0;
		list_move_tail(&req->list, &dev->in_idle_list);
		break;
	/* this endpoint is normally active while we're configured */
	case -ECONNRESET:		/* request dequeued */
		dev->error = 1;
		usb_ep_fifo_flush(ep);
		break;
	case -ESHUTDOWN:		/* disconnect from host */
		dev->error = 1;
		amb_free_buf_req(ep, req);
		break;
	default:
		AMB_ERROR(dev, "%s complete --> %d, %d/%d\n", ep->name,
				status, req->actual, req->length);
		dev->error = 1;
		/* queue request again */
		rval = usb_ep_queue(dev->in_ep, req, GFP_ATOMIC);
		if (rval != 0) {
			AMB_ERROR(dev, "%s: cannot queue bulk in request, "
				"rval=%d\n", __func__, rval);
		}
		break;
	}
	wake_up_interruptible(&dev->wq);
	spin_unlock_irqrestore(&dev->lock, flags);
}

static void amb_bulk_out_complete (struct usb_ep *ep, struct usb_request *req)
{
	struct amb_dev	*dev = ep->driver_data;
	int		status = req->status;
	unsigned long	flags;
	int 		rval = 0;

	spin_lock_irqsave(&dev->lock, flags);
	switch (status) {
	case 0: 			/* normal completion */
		dev->error = 0;
		list_add_tail(&req->list, &dev->out_req_list);
		break;
	/* this endpoint is normally active while we're configured */
	case -ECONNRESET:		/* request dequeued */
		dev->error = 1;
		usb_ep_fifo_flush(ep);
		break;
	case -ESHUTDOWN:		/* disconnect from host */
		dev->error = 1;
		amb_free_buf_req(ep, req);
		break;
	default:
		AMB_ERROR(dev, "%s complete --> %d, %d/%d\n", ep->name,
				status, req->actual, req->length);
		dev->error = 1;
		/* queue request again */
		rval = usb_ep_queue(dev->out_ep, req, GFP_ATOMIC);
		if (rval != 0) {
			AMB_ERROR(dev, "%s: cannot queue bulk in request, "
				"rval=%d\n", __func__, rval);
		}
		break;
	}
	wake_up_interruptible(&dev->wq);
	spin_unlock_irqrestore(&dev->lock, flags);
}

static void amb_notify_complete (struct usb_ep *ep, struct usb_request *req)
{
	struct amb_dev	*dev = ep->driver_data;
	int				status = req->status;
	unsigned long	flags;

	spin_lock_irqsave(&dev->lock, flags);
	req->context = NULL;

	switch (status) {
	case 0: 			/* normal completion */
		/* issue the second notification if host reads the previous one */
		schedule_work(&notify_work);
		break;

	/* this endpoint is normally active while we're configured */
	case -ECONNRESET:		/* request dequeued */
		usb_ep_fifo_flush(ep);
		break;
	case -ESHUTDOWN:		/* disconnect from host */
		amb_free_buf_req(ep, req);
		break;
	default:
		AMB_ERROR(dev, "%s complete --> %d, %d/%d\n", ep->name,
				status, req->actual, req->length);
		break;
	}
	spin_unlock_irqrestore(&dev->lock, flags);
}

static void amb_start_notify (struct amb_dev *dev)
{
	struct usb_request *req = dev->notify_req;
	struct amb_notify *event;
	int value;

	/* flush old status
	 * FIXME iff req->context != null just dequeue it
	 */
	//usb_ep_disable(dev->notify_ep);
	//usb_ep_enable(dev->notify_ep);

	event = req->buf;
	event->bNotifyType = cpu_to_le16(PORT_NOTIFY_IDLE);
	event->port_id = cpu_to_le16 (0);
	event->value = cpu_to_le32 (0);
	event->status = DEVICE_NOT_OPEN;

	req->length = AG_NOTIFY_MAXPACKET;
	req->complete = amb_notify_complete;
	req->context = dev;
	req->zero = !(req->length % dev->notify_ep->maxpacket);

	value = usb_ep_queue(dev->notify_ep, req, GFP_ATOMIC);
	if (value < 0)
		AMB_DBG (dev, "status buf queue --> %d\n", value);
}
/*-------------------------------------------------------------------------*/
static void amb_cfg_unbind(struct usb_configuration *c)
{
	device_destroy(ag_class, ag_dev_id);
}

static struct usb_configuration amb_config_driver = {
	.label			= "AMB Stream Gadget",
	.unbind			= amb_cfg_unbind,
	.bConfigurationValue	= 1,
	//.iConfiguration	=	STRING_SOURCE_SINK,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.MaxPower		= 1,
};

static int amb_set_stream_config(struct amb_dev *dev)
{
	int		result = 0, i;
	struct usb_gadget	*gadget = dev->gadget;
	struct usb_ep *ep;
	struct usb_request *req;

	/* one endpoint writes data in (to the host) */
	result = config_ep_by_speed(gadget, &(dev->func), dev->in_ep);
	if (result)
		goto exit;
	result = usb_ep_enable(dev->in_ep);
	if (result != 0) {
		AMB_ERROR(dev, "%s: can't enable %s, result=%d\n",
			__func__, dev->in_ep->name, result);
		goto exit;
	}

	/* one endpoint reads data out (from the host) */
	result = config_ep_by_speed(gadget, &(dev->func), dev->out_ep);
	if (result)
		goto exit;
	result = usb_ep_enable(dev->out_ep);
	if (result != 0) {
		AMB_ERROR(dev, "%s: can't enable %s, result=%d\n",
			__func__, dev->out_ep->name, result);
		usb_ep_disable(dev->in_ep);
		goto exit;
	}

	/* one endpoint report status  (to the host) */
	result = config_ep_by_speed(gadget, &(dev->func), dev->notify_ep);
	if (result)
		goto exit;
	result = usb_ep_enable(dev->notify_ep);
	if (result != 0) {
		AMB_ERROR(dev, "%s: can't enable %s, result=%d\n",
			__func__, dev->notify_ep->name, result);
		usb_ep_disable(dev->out_ep);
		usb_ep_disable(dev->in_ep);
		dev->out_ep->desc = NULL;
		dev->in_ep->desc = NULL;
		goto exit;
	}

	/* allocate and queue read requests */
	ep = dev->out_ep;
	ep->driver_data = dev;
	for (i = 0; i < qdepth && result == 0; i++) {
		req = amb_alloc_buf_req(ep, buflen, GFP_ATOMIC);
		if (req) {
			req->complete = amb_bulk_out_complete;
			result = usb_ep_queue(ep, req, GFP_ATOMIC);
			if (result < 0) {
				AMB_ERROR(dev, "%s: can't queue bulk out request, "
					"rval = %d\n", __func__, result);
			}
		} else {
			AMB_ERROR(dev, "%s: can't allocate bulk in requests\n", __func__);
			result = -ENOMEM;
			goto exit;
		}
	}

	/* allocate write requests, and put on free list */
	ep = dev->in_ep;
	ep->driver_data = dev;
	for (i = 0; i < qdepth; i++) {
		req = amb_alloc_buf_req(ep, buflen, GFP_ATOMIC);
		if (req) {
			req->complete = amb_bulk_in_complete;
			req_put(dev, &dev->in_idle_list, req);
		} else {
			AMB_ERROR(dev, "%s: can't allocate bulk in requests\n", __func__);
			result = -ENOMEM;
			goto exit;
		}
	}

	ep = dev->notify_ep;
	ep->driver_data = dev;
	if (dev->notify_ep) {
		dev->notify_req = amb_alloc_buf_req(ep,
						AG_NOTIFY_MAXPACKET,GFP_ATOMIC);
		amb_start_notify(dev);
	}

exit:
	/* caller is responsible for cleanup on error */
	return result;
}

static void amb_reset_config (struct amb_dev *dev)
{
	struct usb_request *req;
	unsigned long flags;

	if (dev->config == 0)
		return;
	dev->config = 0;

	spin_lock_irqsave(&dev->lock, flags);

	/* free write requests on the free list */
	while(!list_empty(&dev->in_idle_list)) {
		req = list_entry(dev->in_idle_list.next,
				struct usb_request, list);
		list_del_init(&req->list);
		req->length = buflen;
		amb_free_buf_req(dev->in_ep, req);
	}
	while(!list_empty(&dev->in_queue_list)) {
		req = list_entry(dev->in_queue_list.prev,
				struct usb_request, list);
		list_del_init(&req->list);
		req->length = buflen;
		amb_free_buf_req(dev->in_ep, req);
	}

	while(!list_empty(&dev->out_req_list)) {
		req = list_entry(dev->out_req_list.prev,
				struct usb_request, list);
		list_del_init(&req->list);
		req->length = buflen;
		amb_free_buf_req(dev->out_ep, req);
	}

	spin_unlock_irqrestore(&dev->lock, flags);
	/* just disable endpoints, forcing completion of pending i/o.
	 * all our completion handlers free their requests in this case.
	 */
	/* Disable the endpoints */
	if (dev->notify_ep)
		usb_ep_disable(dev->notify_ep);
	usb_ep_disable(dev->in_ep);
	usb_ep_disable(dev->out_ep);
}

static int amb_set_interface(struct amb_dev *dev, unsigned number)
{
	int result = 0;

	/* Free the current interface */
	//amb_reset_config(dev);

	result = amb_set_stream_config(dev);

	if (!result) {
		dev->interface = number;
		dev->config = 1;
	}
	return result;
}

static int __init amb_func_bind(struct usb_configuration *c,
								struct usb_function *f)
{
	struct amb_dev *dev = container_of(f, struct amb_dev, func);
	struct usb_composite_dev *cdev = c->cdev;
	struct usb_ep *ep;
	int id;
	int ret = 0;

	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	amb_data_stream_intf.bInterfaceNumber = id;

	/* allocate bulk endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &fs_bulk_in_desc);
	if (!ep) {
autoconf_fail:
		ERROR(cdev, "%s: can't autoconfigure on %s\n",
			f->name, cdev->gadget->name);
		return -ENODEV;
	}
	ep->driver_data = dev;	/* claim the endpoint */
	dev->in_ep = ep;

	ep = usb_ep_autoconfig(cdev->gadget, &fs_bulk_out_desc);
	if (!ep) {
		goto autoconf_fail;
	}
	ep->driver_data = dev;	/* claim the endpoint */
	dev->out_ep = ep;

	ep = usb_ep_autoconfig(cdev->gadget, &fs_intr_in_desc);
	if (!ep) {
		goto autoconf_fail;
	}
	dev->notify_ep = ep;
	ep->driver_data = dev;	/* claim the endpoint */

	/* support all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */
	hs_bulk_in_desc.bEndpointAddress = fs_bulk_in_desc.bEndpointAddress;
	hs_bulk_out_desc.bEndpointAddress = fs_bulk_out_desc.bEndpointAddress;
	hs_intr_in_desc.bEndpointAddress = fs_intr_in_desc.bEndpointAddress;

	ret = usb_assign_descriptors(f, fs_amb_data_stream_function,
				hs_amb_data_stream_function, NULL);
	if (ret)
		goto fail;

	printk("AMB STREAM: %s speed IN/%s OUT/%s NOTIFY/%s\n",
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			dev->in_ep->name, dev->out_ep->name,dev->notify_ep->name);
	return 0;
fail:
	usb_free_all_descriptors(f);
	if (dev->in_ep)
		dev->in_ep->driver_data = NULL;
	if (dev->out_ep)
		dev->out_ep->driver_data = NULL;
	if (dev->notify_ep)
		dev->notify_ep->driver_data = NULL;

	printk("%s: can't bind, err %d\n", f->name, ret);
	return ret;
}

static void amb_func_unbind(struct usb_configuration *c,
							struct usb_function *f)
{
	usb_free_all_descriptors(f);
}

static int amb_func_set_alt(struct usb_function *f,
							unsigned inf, unsigned alt)
{
	struct amb_dev *dev = container_of(f, struct amb_dev, func);
	int ret = -ENOTSUPP;

	if (!alt)
		ret = amb_set_interface(dev, inf);
	return ret;
}

static void amb_func_disable(struct usb_function *f)
{
	struct amb_dev *dev = container_of(f, struct amb_dev, func);
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	amb_reset_config(dev);
	spin_unlock_irqrestore(&dev->lock, flags);
}

static int __init amb_bind_config(struct usb_configuration *c)
{
	struct usb_gadget	*gadget = c->cdev->gadget;
	struct amb_dev		*dev;
	struct device		*pdev;
	int status = -ENOMEM;

	usb_ep_autoconfig_reset(gadget);

	dev = kzalloc(sizeof(struct amb_dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->func.name = shortname;
	dev->func.bind = amb_func_bind;
	dev->func.unbind = amb_func_unbind;
	dev->func.set_alt = amb_func_set_alt;
	dev->func.disable = amb_func_disable;

	spin_lock_init (&dev->lock);
	mutex_init(&dev->mtx);
	init_waitqueue_head(&dev->wq);
	INIT_LIST_HEAD(&dev->in_idle_list);
	INIT_LIST_HEAD(&dev->in_queue_list);
	INIT_LIST_HEAD(&dev->out_req_list);

	dev->gadget = gadget;
	dev->error = 0;
	ag_device = dev;

	status = usb_add_function(c, &dev->func);
	if (status) {
		kfree(dev);
		return status;
	}

	/* Setup the sysfs files for the gadget. */
	pdev = device_create(ag_class, NULL, ag_dev_id,
				  NULL, "amb_gadget");
	if (IS_ERR(pdev)) {
		status = PTR_ERR(pdev);
		ERROR(dev, "Failed to create device: amb_gadget\n");
		goto fail;
	}

	usb_gadget_set_selfpowered(gadget);

	if (gadget->is_otg) {
		otg_descriptor.bmAttributes |= USB_OTG_HNP;
		amb_config_driver.descriptors = otg_desc;
		amb_config_driver.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}
#if 0
	spin_lock_init (&dev->lock);
	mutex_init(&dev->mtx);
	init_waitqueue_head(&dev->wq);
	INIT_LIST_HEAD(&dev->in_idle_list);
	INIT_LIST_HEAD(&dev->in_queue_list);
	INIT_LIST_HEAD(&dev->out_req_list);

	dev->gadget = gadget;
	dev->error = 0;
	ag_device = dev;
#endif
	return 0;

fail:
	amb_cfg_unbind(c);
	return status;
}

static int amb_unbind(struct usb_composite_dev *cdev)
{
#if 0
	struct usb_gadget	*gadget = cdev->gadget;
	struct amb_dev		*dev = get_gadget_data (gadget);

	kfree (dev);
	set_gadget_data(gadget, NULL);
	ag_device = NULL;
#else
#endif
	return 0;
}

/*-------------------------------------------------------------------------*/
static int __init amb_bind(struct usb_composite_dev *cdev)
{
	int ret;
	ret = usb_string_ids_tab(cdev, strings);
	if (ret < 0)
		return ret;

	ag_device_desc.iManufacturer = strings[USB_GADGET_MANUFACTURER_IDX].id;
	ag_device_desc.iProduct = strings[USB_GADGET_PRODUCT_IDX].id;
	ag_device_desc.iSerialNumber = strings[USB_GADGET_SERIAL_IDX].id;
	amb_config_driver.iConfiguration = strings[STRING_SOURCE_SINK].id;

	ret = usb_add_config(cdev, &amb_config_driver, amb_bind_config);
	if (ret < 0)
		return ret;

	usb_composite_overwrite_options(cdev, &coverwrite);
	INFO(cdev, "%s, version: " DRIVER_VERSION "\n", longname);

	return 0;
}

static __refdata struct usb_composite_driver amb_gadget_driver = {
	.name		=	shortname,
	.dev		=	&ag_device_desc,
	.strings	=	dev_strings,
	.max_speed	=	USB_SPEED_HIGH,
	.bind		=	amb_bind,
	.unbind		=	amb_unbind,
};

static int __init amb_gadget_init(void)
{
	int rval = 0;

	ag_class = class_create(THIS_MODULE, "amb_stream_gadget");
	if (IS_ERR(ag_class)) {
		rval = PTR_ERR(ag_class);
		pr_err("unable to create usb_gadget class %d\n", rval);
		return rval;
	}

	if (buflen >= 65536) {
		pr_err("amb_gadget_init: buflen is too large\n");
		rval = -EINVAL;
		goto out1;
	}

	ag_dev_id = MKDEV(AMB_GADGET_MAJOR, AMB_GADGET_MINOR_START);
	rval = register_chrdev_region(ag_dev_id, 1, "amb_gadget");
	if(rval < 0){
		pr_err("amb_gadget_init: register devcie number error!\n");
		goto out1;
	}

	cdev_init(&ag_cdev, &ag_fops);
	ag_cdev.owner = THIS_MODULE;
	rval = cdev_add(&ag_cdev, ag_dev_id, 1);
	if (rval) {
		pr_err("amb_gadget_init: cdev_add failed\n");
		unregister_chrdev_region(ag_dev_id, 1);
		goto out1;
	}

	rval = usb_composite_probe(&amb_gadget_driver);
	if (rval) {
		pr_err("amb_gadget_init: cannot register gadget driver, "
			"rval=%d\n", rval);
		class_destroy(ag_class);
		goto out0;
	}
out1:
	if(rval)
		class_destroy(ag_class);
out0:
	return rval;
}
module_init (amb_gadget_init);

static void __exit amb_gadget_exit (void)
{
	usb_composite_unregister(&amb_gadget_driver);

	unregister_chrdev_region(ag_dev_id, 1);
	cdev_del(&ag_cdev);
	class_destroy(ag_class);
}
module_exit (amb_gadget_exit);

MODULE_AUTHOR ("Cao Rongrong <rrcao@ambarella.com>");
MODULE_LICENSE ("GPL");


