/*
 *	uvc_gadget.c  --  USB Video Class Gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/video.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>

#include "uvc.h"

unsigned int uvc_gadget_trace_param;
struct usb_ep *video_stream1_ep1;
struct usb_ep *video_stream1_ep2;
struct usb_ep *video_stream2_ep1;
struct usb_ep *video_stream2_ep2;

static unsigned int gLastIntf;
static unsigned int gLastDirOut;


/* --------------------------------------------------------------------------
 * Function descriptors
 */

/* string IDs are assigned dynamically */

#define UVC_STRING_ASSOCIATION_IDX		0
#define UVC_STRING_CONTROL_IDX			1
#define UVC_STRING_STREAMING_IDX		2

static struct usb_string uvc_en_us_strings[] = {
	[UVC_STRING_ASSOCIATION_IDX].s = "Fullhan Webcam",
	[UVC_STRING_CONTROL_IDX].s = "Video Control",
	[UVC_STRING_STREAMING_IDX].s = "Video Streaming",
	{  }
};

static struct usb_gadget_strings uvc_stringtab = {
	.language = 0x0409,	/* en-us */
	.strings = uvc_en_us_strings,
};

static struct usb_gadget_strings *uvc_function_strings[] = {
	&uvc_stringtab,
	NULL,
};

#define UVC_INTF_VIDEO_CONTROL			0
#define UVC_INTF_VIDEO_STREAMING		1

static struct usb_interface_assoc_descriptor uvc_iad = {
	.bLength		= sizeof(uvc_iad),
	.bDescriptorType	= USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface	= 0,
	.bInterfaceCount	= 2,
	.bFunctionClass		= USB_CLASS_VIDEO,
	.bFunctionSubClass	= UVC_SC_VIDEO_INTERFACE_COLLECTION,
	.bFunctionProtocol	= 0x00,
	.iFunction		= 0,
};

static struct usb_interface_descriptor uvc_control_intf  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_VIDEO_CONTROL,
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOCONTROL,
	.bInterfaceProtocol	= 0x00,
	.iInterface		= 0,
};

static struct usb_endpoint_descriptor uvc_control_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize		= cpu_to_le16(16),
	.bInterval		= 8,
};

static struct uvc_control_endpoint_descriptor uvc_control_cs_ep  = {
	.bLength		= UVC_DT_CONTROL_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_CS_ENDPOINT,
	.bDescriptorSubType	= UVC_EP_INTERRUPT,
	.wMaxTransferSize	= cpu_to_le16(16),
};

static struct usb_interface_descriptor uvc_streaming1_intf_alt0 = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_VIDEO_STREAMING,
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 0,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= 0x00,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming1_intf_alt1  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_VIDEO_STREAMING,
	.bAlternateSetting	= 1,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= 0x00,
	.iInterface		= 0,
};

static struct usb_endpoint_descriptor uvc_streaming1_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize		= cpu_to_le16(1020),/*1020*/
	.bInterval		= 1,
};

static struct usb_interface_descriptor uvc_streaming1_intf_alt2  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_VIDEO_STREAMING,
	.bAlternateSetting	= 2,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= 0x00,
	.iInterface		= 0,
};

static struct usb_endpoint_descriptor uvc_streaming1_ep2 = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize		= cpu_to_le16(512),
	.bInterval		= 1,
};

static const struct usb_descriptor_header * const uvc_fs_streaming1[] = {
	(struct usb_descriptor_header *) &uvc_streaming1_intf_alt1,
	(struct usb_descriptor_header *) &uvc_streaming1_ep,
	(struct usb_descriptor_header *) &uvc_streaming1_intf_alt2,
	(struct usb_descriptor_header *) &uvc_streaming1_ep2,
	NULL,
};

static const struct usb_descriptor_header * const uvc_hs_streaming1[] = {
	(struct usb_descriptor_header *) &uvc_streaming1_intf_alt1,
	(struct usb_descriptor_header *) &uvc_streaming1_ep,
	(struct usb_descriptor_header *) &uvc_streaming1_intf_alt2,
	(struct usb_descriptor_header *) &uvc_streaming1_ep2,
	NULL,
};




static struct usb_interface_descriptor uvc_streaming2_intf_alt0 = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_VIDEO_STREAMING,
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 0,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= 0x00,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming2_intf_alt1  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_VIDEO_STREAMING,
	.bAlternateSetting	= 1,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= 0x00,
	.iInterface		= 0,
};

static struct usb_endpoint_descriptor uvc_streaming2_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize		= cpu_to_le16(1020), /*1020*/
	.bInterval		= 1,
};

static struct usb_interface_descriptor uvc_streaming2_intf_alt2  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_VIDEO_STREAMING,
	.bAlternateSetting	= 2,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= 0x00,
	.iInterface		= 0,
};

static struct usb_endpoint_descriptor uvc_streaming2_ep2 = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize		= cpu_to_le16(512),
	.bInterval		= 1,
};

static const struct usb_descriptor_header * const uvc_fs_streaming2[] = {
	(struct usb_descriptor_header *) &uvc_streaming2_intf_alt1,
	(struct usb_descriptor_header *) &uvc_streaming2_ep,
	(struct usb_descriptor_header *) &uvc_streaming2_intf_alt2,
	(struct usb_descriptor_header *) &uvc_streaming2_ep2,
	NULL,
};

static const struct usb_descriptor_header * const uvc_hs_streaming2[] = {
	(struct usb_descriptor_header *) &uvc_streaming2_intf_alt1,
	(struct usb_descriptor_header *) &uvc_streaming2_ep,
	(struct usb_descriptor_header *) &uvc_streaming2_intf_alt2,
	(struct usb_descriptor_header *) &uvc_streaming2_ep2,
	NULL,
};


/* --------------------------------------------------------------------------
 * Control requests
 */

static void
uvc_function_ep0_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct uvc_device *uvc = req->context;
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;

	if (uvc->event_setup_out) {
		uvc->event_setup_out = 0;

		memset(&v4l2_event, 0, sizeof(v4l2_event));
		v4l2_event.type = UVC_EVENT_DATA;
		uvc_event->data.length = req->actual;
		memcpy(&uvc_event->data.data, req->buf, req->actual);
		v4l2_event_queue(uvc->vdev, &v4l2_event);
	}
}

static int
uvc_function_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
/*	struct uvc_device *uvc = to_uvc(f);*/
	struct uvc_common *comm = to_common(f);
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;
	unsigned int intf = ctrl->wIndex & 0xff;

	gLastIntf = intf;
	gLastDirOut = (ctrl->bRequestType & USB_DIR_OUT) == USB_DIR_OUT;

	if ((ctrl->bRequestType & USB_TYPE_MASK) != USB_TYPE_CLASS) {
		INFO(f->config->cdev, "invalid request type\n");
		return -EINVAL;
	}

	/* Stall too big requests. */
	if (le16_to_cpu(ctrl->wLength) > UVC_MAX_REQUEST_SIZE)
		return -EINVAL;

	memset(&v4l2_event, 0, sizeof(v4l2_event));
	v4l2_event.type = UVC_EVENT_SETUP;
	memcpy(&uvc_event->req, ctrl, sizeof(uvc_event->req));


#ifdef UVC_DOUBLE_STREAM
	if (intf == comm->uvc2->streaming_intf) {
		v4l2_event_queue(comm->uvc2->vdev, &v4l2_event);
	}
	else
#endif
	   v4l2_event_queue(comm->uvc1->vdev, &v4l2_event);
	return 0;
}

static int
uvc_function_get_alt(struct usb_function *f, unsigned interface)
{
	struct uvc_device *uvc = to_uvc(f);
	struct uvc_common *comm = to_common(f);

	INFO(f->config->cdev, "uvc_function_get_alt(%u)\n", interface);

	if (interface == uvc->comm->control_intf) {
		return 0;
	} else if (interface == comm->uvc1->streaming_intf)
		return comm->uvc1->state ==
					UVC_STATE_STREAMING ? 1 : 0;

#ifdef UVC_DOUBLE_STREAM
	else if (interface == comm->uvc2->streaming_intf)
		return comm->uvc2->state == UVC_STATE_STREAMING ? 1 : 0;

#endif
	else
		return -EINVAL;

}

static int
uvc_function_set_alt(struct usb_function *f, unsigned interface, unsigned alt)
{
	struct uvc_device *uvc = to_uvc(f);
	struct uvc_common *comm = to_common(f);
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;

	struct uvc_device *uvc1 = comm->uvc1;
	struct uvc_device *uvc2 = comm->uvc2;


	if (interface == uvc1->streaming_intf)
		uvc = uvc1;

#ifdef UVC_DOUBLE_STREAM
	else if (interface == uvc2->streaming_intf)
		uvc = uvc2;

#endif

	INFO(f->config->cdev, "uvc_function_set_alt(%u, %u)\n", interface, alt);

	if (interface == uvc->comm->control_intf) {
		if (alt)
			return -EINVAL;

		if (uvc1->state == UVC_STATE_DISCONNECTED) {
			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UVC_EVENT_CONNECT;
			uvc_event->speed = f->config->cdev->gadget->speed;
			v4l2_event_queue(uvc1->vdev, &v4l2_event);
			uvc1->state = UVC_STATE_CONNECTED;
		}

#ifdef UVC_DOUBLE_STREAM
		if (uvc2->state == UVC_STATE_DISCONNECTED) {
			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UVC_EVENT_CONNECT;
			uvc_event->speed = f->config->cdev->gadget->speed;
			v4l2_event_queue(uvc2->vdev, &v4l2_event);

			uvc2->state = UVC_STATE_CONNECTED;
		}
#endif
		return 0;
	}

	if (interface != uvc1->streaming_intf && interface !=
			uvc2->streaming_intf)
		return -EINVAL;

	switch (alt) {
	case 0:
		if (uvc->state != UVC_STATE_STREAMING)
			return 0;
		memset(&v4l2_event, 0, sizeof(v4l2_event));
		v4l2_event.type = UVC_EVENT_STREAMOFF;
		v4l2_event_queue(uvc->vdev, &v4l2_event);

		uvc->state = UVC_STATE_CONNECTED;
		break;

	case 1:
	case 2:
		if (uvc->state != UVC_STATE_CONNECTED)
			return 0;

		memset(&v4l2_event, 0, sizeof(v4l2_event));
		if (interface == uvc1->streaming_intf) {
			if (alt == 2) {
				uvc->video.ep = video_stream1_ep2;
				v4l2_event.u.data[0] =
				uvc_streaming1_ep2.wMaxPacketSize&0xff;
				v4l2_event.u.data[1] =
				uvc_streaming1_ep2.wMaxPacketSize>>8;
			} else {
				uvc->video.ep = video_stream1_ep1;
				v4l2_event.u.data[0] =
				uvc_streaming1_ep.wMaxPacketSize&0xff;
				v4l2_event.u.data[1] =
				uvc_streaming1_ep.wMaxPacketSize>>8;

			}

			if (uvc->video.ep) {
				if (alt == 2)
					usb_ep_enable(uvc->video.ep,\
					&uvc_streaming1_ep2);
				else
					usb_ep_enable(uvc->video.ep,\
					&uvc_streaming1_ep);
			}
		}
#ifdef UVC_DOUBLE_STREAM
		else if (interface == uvc2->streaming_intf) {
			if (alt == 2) {
				uvc->video.ep = video_stream2_ep2;
				v4l2_event.u.data[0] =
			uvc_streaming2_ep2.wMaxPacketSize&0xff;
				v4l2_event.u.data[1] =
			uvc_streaming2_ep2.wMaxPacketSize>>8;
			} else {
				uvc->video.ep = video_stream2_ep1;
				v4l2_event.u.data[0] =
					uvc_streaming2_ep.wMaxPacketSize&0xff;
				v4l2_event.u.data[1] =
					uvc_streaming2_ep.wMaxPacketSize>>8;

			}

			if (uvc->video.ep) {
				if (alt == 2)
					usb_ep_enable(uvc->video.ep,\
					&uvc_streaming2_ep2);
				else
					usb_ep_enable(uvc->video.ep,\
					&uvc_streaming2_ep);
			}
		}
#endif
		v4l2_event.type = UVC_EVENT_STREAMON;
		v4l2_event_queue(uvc->vdev, &v4l2_event);
		uvc->state = UVC_STATE_STREAMING;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static void
uvc_function_disable(struct usb_function *f)
{
	/*struct uvc_device *uvc = to_uvc(f);*/
	struct uvc_common *comm = to_common(f);
	struct v4l2_event v4l2_event;

	INFO(f->config->cdev, "uvc_function_disable\n");
	printk(KERN_EMERG "######### uvc disable #########\n");

	memset(&v4l2_event, 0, sizeof(v4l2_event));
	v4l2_event.type = UVC_EVENT_DISCONNECT;
	v4l2_event_queue(comm->uvc1->vdev, &v4l2_event);
	comm->uvc1->state = UVC_STATE_DISCONNECTED;

#ifdef UVC_DOUBLE_STREAM
	v4l2_event_queue(comm->uvc2->vdev, &v4l2_event);
	comm->uvc2->state = UVC_STATE_DISCONNECTED;
#endif
}

/* --------------------------------------------------------------------------
 * Connection / disconnection
 */

void
uvc_function_connect(struct uvc_device *uvc)
{
	struct usb_composite_dev *cdev = uvc->comm->func.config->cdev;
	int ret;
	ret = usb_function_activate(&uvc->comm->func);
	if (ret < 0)
		INFO(cdev, "UVC connect failed with %d\n", ret);
}

void
uvc_function_disconnect(struct uvc_device *uvc)
{
	struct usb_composite_dev *cdev = uvc->comm->func.config->cdev;
	int ret;
	ret = usb_function_deactivate(&uvc->comm->func);
	if (ret < 0)
		INFO(cdev, "UVC disconnect failed with %d\n", ret);
}

/* --------------------------------------------------------------------------
 * USB probe and disconnect
 */

static int
uvc_register_video(struct uvc_device *uvc)
{
	struct usb_composite_dev *cdev = uvc->comm->func.config->cdev;
	struct video_device *video;

	/* TODO reference counting. */
	video = video_device_alloc();
	if (video == NULL)
		return -ENOMEM;

	video->parent = &cdev->gadget->dev;
	video->minor = -1;
	video->fops = &uvc_v4l2_fops;
	video->release = video_device_release;
	strncpy(video->name, cdev->gadget->name, sizeof(video->name));

	uvc->vdev = video;
	video_set_drvdata(video, uvc);

	return video_register_device(video, VFL_TYPE_GRABBER, -1);
}

#define UVC_COPY_DESCRIPTOR(mem, dst, desc) \
	do { \
		memcpy(mem, desc, (desc)->bLength); \
		*(dst)++ = mem; \
		mem += (desc)->bLength; \
	} while (0);

#define UVC_COPY_DESCRIPTORS(mem, dst, src) \
	do { \
		const struct usb_descriptor_header * const *__src; \
		for (__src = src; *__src; ++__src) { \
			memcpy(mem, *__src, (*__src)->bLength); \
			*dst++ = mem; \
			mem += (*__src)->bLength; \
		} \
	} while (0)

static struct usb_descriptor_header** /*__init*/
uvc_copy_descriptors(struct uvc_device *uvc, enum usb_device_speed speed)
{
	struct uvc_input_header_descriptor *uvc_streaming_header;
	struct uvc_header_descriptor *uvc_control_header;
	struct uvc_descriptor_header **uvc_streaming_cls;
	const struct usb_descriptor_header * const *uvc_streaming_std;
	const struct usb_descriptor_header * const *src;
	struct usb_descriptor_header **dst;
	struct usb_descriptor_header **hdr;
	unsigned int control_size;
	unsigned int streaming_size;
	unsigned int n_desc;
	unsigned int bytes;
	void *mem;
	int needBufLen;


	unsigned int stream2_avalid = 0;
	struct uvc_descriptor_header **uvc_streaming2_cls = NULL;
	const struct usb_descriptor_header * const *uvc_streaming2_std = NULL;
	unsigned int streaming2_size;

	stream2_avalid = (uvc->comm->desc.fs_streaming2 != NULL);

	uvc_streaming_cls = (speed == USB_SPEED_FULL)
	? uvc->comm->desc.fs_streaming : uvc->comm->desc.hs_streaming;
	uvc_streaming_std = (speed == USB_SPEED_FULL)
	? uvc_fs_streaming1 : uvc_hs_streaming1;


	if (stream2_avalid) {
		uvc_streaming2_cls = (speed == USB_SPEED_FULL)
		? uvc->comm->desc.fs_streaming2 : uvc->comm->desc.hs_streaming2;
		uvc_streaming2_std = (speed == USB_SPEED_FULL)
		? uvc_fs_streaming2 : uvc_hs_streaming2;

		uvc_iad.bInterfaceCount = 3;
	}

	/* Descriptors layout
	 *
	 * uvc_iad
	 * uvc_control_intf
	 * Class-specific UVC control descriptors
	 * uvc_control_ep
	 * uvc_control_cs_ep
	 * uvc_streaming_intf_alt0
	 * Class-specific UVC streaming descriptors
	 * uvc_{fs|hs}_streaming
	 */

	/* Count descriptors and compute their size. */
	control_size = 0;
	streaming_size = 0;
	bytes = uvc_iad.bLength + uvc_control_intf.bLength
		  + uvc_control_ep.bLength + uvc_control_cs_ep.bLength
		  + uvc_streaming1_intf_alt0.bLength;
	n_desc = 5;


	for (src = (const struct usb_descriptor_header **)uvc->comm->desc.control; *src; ++src) {
		control_size += (*src)->bLength;
		bytes += (*src)->bLength;
		n_desc++;
	}
	for (src = (const struct usb_descriptor_header **)uvc_streaming_cls; *src; ++src) {
		streaming_size += (*src)->bLength;
		bytes += (*src)->bLength;
		n_desc++;
	}
	for (src = uvc_streaming_std; *src; ++src) {
		bytes += (*src)->bLength;
		n_desc++;
	}

	if (stream2_avalid) {
		bytes += uvc_streaming2_intf_alt0.bLength;
		n_desc++;
		for (src = (const struct usb_descriptor_header **)uvc_streaming2_cls; *src; ++src) {
			streaming2_size += (*src)->bLength;
			bytes += (*src)->bLength;
			n_desc++;
		}

		for (src = uvc_streaming2_std; *src; ++src) {
			bytes += (*src)->bLength;
			n_desc++;
		}
	}



/*	printk(KERN_EMERG "#########uvc_copy_descriptors  44\n");*/

	needBufLen = (n_desc + 1) * sizeof(*src) + bytes;

	mem = kmalloc(needBufLen, GFP_KERNEL);
	if (mem == NULL)
		return NULL;

	hdr = mem;
	dst = mem;
	mem += (n_desc + 1) * sizeof(*src);

	/* Copy the descriptors. */
	UVC_COPY_DESCRIPTOR(mem, dst, &uvc_iad);
	UVC_COPY_DESCRIPTOR(mem, dst, &uvc_control_intf);

	uvc_control_header = mem;
	UVC_COPY_DESCRIPTORS(mem, dst,
		(const struct usb_descriptor_header **)uvc->comm->desc.control);
	uvc_control_header->wTotalLength = cpu_to_le16(control_size);

	uvc_control_header->baInterfaceNr[0] = uvc->streaming_intf;
	if (stream2_avalid) {
		uvc_control_header->baInterfaceNr[1] =
			uvc->comm->uvc2->streaming_intf;
		uvc_control_header->bInCollection = 2;

	} else {
		uvc_control_header->bInCollection = 1;
	}


	UVC_COPY_DESCRIPTOR(mem, dst, &uvc_control_ep);
	UVC_COPY_DESCRIPTOR(mem, dst, &uvc_control_cs_ep);
	UVC_COPY_DESCRIPTOR(mem, dst, &uvc_streaming1_intf_alt0);

	uvc_streaming_header = mem;
	UVC_COPY_DESCRIPTORS(mem, dst,
		(const struct usb_descriptor_header **)uvc_streaming_cls);
	uvc_streaming_header->wTotalLength = cpu_to_le16(streaming_size);
	uvc_streaming_header->bEndpointAddress =
		uvc_streaming1_ep.bEndpointAddress;

	UVC_COPY_DESCRIPTORS(mem, dst, uvc_streaming_std);

	if (stream2_avalid) {
		UVC_COPY_DESCRIPTOR(mem, dst, &uvc_streaming2_intf_alt0);
		uvc_streaming_header = mem;
		UVC_COPY_DESCRIPTORS(mem, dst,
			(const struct usb_descriptor_header **)uvc_streaming2_cls);
		uvc_streaming_header->wTotalLength =
			cpu_to_le16(streaming2_size);
		uvc_streaming_header->bEndpointAddress =
			uvc_streaming2_ep.bEndpointAddress;
#ifdef UVC_DOUBLE_STREAM
		uvc_streaming_header->bTerminalLink++;
#endif

		UVC_COPY_DESCRIPTORS(mem, dst, uvc_streaming2_std);
	}

	*dst = NULL;

	return hdr;
}

static void
uvc_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct uvc_device *uvc = to_uvc(f);

	INFO(cdev, "uvc_function_unbind\n");
	printk(KERN_EMERG "######### uvc unbind #########\n");

	if (uvc->vdev) {
		if (uvc->vdev->minor == -1)
			video_device_release(uvc->vdev);
		else
			video_unregister_device(uvc->vdev);
		uvc->vdev = NULL;
	}

	if (uvc->comm->control_ep)
		uvc->comm->control_ep->driver_data = NULL;
	if (uvc->video.ep)
		uvc->video.ep->driver_data = NULL;

	if (uvc->comm->control_req) {
		usb_ep_free_request(cdev->gadget->ep0, uvc->comm->control_req);
		kfree(uvc->comm->control_buf);
	}

	kfree(f->descriptors);
	kfree(f->hs_descriptors);

	kfree(uvc);
}

static int __init
uvc_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct uvc_device *uvc = to_uvc(f);
	struct uvc_common *comm = to_common(f);
	struct uvc_device *uvc1, *uvc2;
	struct usb_ep *ep;
	int ret = -EINVAL;

	uvc1 = comm->uvc1;
	uvc2 = comm->uvc2;

	INFO(cdev, "uvc_function_bind\n");

	/* Allocate endpoints. */
	ep = usb_ep_autoconfig(cdev->gadget, &uvc_control_ep);
	if (!ep) {
		INFO(cdev, "Unable to allocate control EP\n");
		goto error;
	}

	comm->control_ep = ep;
	ep->driver_data = uvc;
	ep = usb_ep_autoconfig(cdev->gadget, &uvc_streaming1_ep);
	if (!ep) {
		INFO(cdev, "Unable to allocate streaming EP\n");
		goto error;
	}
	video_stream1_ep1 = ep;
	uvc->video.ep = ep;
	ep->driver_data = uvc1;
	video_stream1_ep2 =
		usb_ep_autoconfig(cdev->gadget, &uvc_streaming1_ep2);
	if (!video_stream1_ep2) {
		INFO(cdev, "Unable to allocate streaming EP2\n");
		goto error;
	}
	video_stream1_ep2->driver_data = uvc1;
	/* Allocate interface IDs. */
	ret = usb_interface_id(c, f);
	if (ret < 0)
		goto error;
	uvc_iad.bFirstInterface = ret;
	uvc_control_intf.bInterfaceNumber = ret;
	comm->control_intf = ret;
	ret = usb_interface_id(c, f);
	if (ret < 0)
		goto error;

	uvc_streaming1_intf_alt0.bInterfaceNumber = ret;
	uvc_streaming1_intf_alt1.bInterfaceNumber = ret;
	uvc_streaming1_intf_alt2.bInterfaceNumber = ret;
	uvc1->streaming_intf = ret;


	if (NULL != comm->desc.fs_streaming2) {

		video_stream2_ep1 =
			usb_ep_autoconfig(cdev->gadget, &uvc_streaming2_ep);
		if (!video_stream2_ep1) {
			INFO(cdev, "Unable to allocate streaming EP\n");
			goto error;
		}

		uvc2->video.ep = video_stream2_ep1;
		video_stream2_ep1->driver_data = uvc2;

		video_stream2_ep2 =
			usb_ep_autoconfig(cdev->gadget, &uvc_streaming2_ep2);
		if (!video_stream2_ep2) {
			INFO(cdev, "Unable to allocate streaming EP2\n");
			goto error;
		}

		video_stream2_ep2->driver_data = uvc2;
		ret = usb_interface_id(c, f);
		if (ret < 0)
			goto error;
		uvc_streaming2_intf_alt0.bInterfaceNumber = ret;
		uvc_streaming2_intf_alt1.bInterfaceNumber = ret;
		uvc_streaming2_intf_alt2.bInterfaceNumber = ret;
		uvc2->streaming_intf = ret;
	}

	/* Copy descriptors. */
	f->descriptors = uvc_copy_descriptors(uvc, USB_SPEED_FULL);
	f->hs_descriptors = uvc_copy_descriptors(uvc, USB_SPEED_HIGH);

	/* Preallocate control endpoint request. */
	comm->control_req = usb_ep_alloc_request(cdev->gadget->ep0, GFP_KERNEL);
	comm->control_buf = kmalloc(UVC_MAX_REQUEST_SIZE, GFP_KERNEL);
	if (uvc->comm->control_req == NULL || uvc->comm->control_buf == NULL) {
		ret = -ENOMEM;
		goto error;
	}

	comm->control_req->buf = uvc->comm->control_buf;
	comm->control_req->complete = uvc_function_ep0_complete;
	comm->control_req->context = uvc;

	/* Avoid letting this gadget enumerate until the userspace server is
	 * active.
	 */
	ret = usb_function_deactivate(f);
	if (ret < 0)
		goto error;

	/* Initialise video. */
	ret = uvc_video_init(&uvc1->video);
	if (ret < 0)
		goto error;
	/* Register a V4L2 device. */
	ret = uvc_register_video(uvc1);
	if (ret < 0) {
		printk(KERN_INFO "Unable to register video device\n");
		goto error;
	}

	if (NULL != comm->desc.fs_streaming2) {
		ret = usb_function_deactivate(f);
		if (ret < 0)
			goto error;
		/* Initialise video. */
		ret = uvc_video_init(&uvc2->video);
		if (ret < 0)
			goto error;
		/* Register a V4L2 device. */
		ret = uvc_register_video(uvc2);
		if (ret < 0) {
			printk(KERN_INFO "Unable to register video2 device\n");
			goto error;
		}
	}
	return 0;

error:
	uvc_function_unbind(c, f);
	return ret;
}

/* --------------------------------------------------------------------------
 * USB gadget function
 */

/**
 * uvc_bind_config - add a UVC function to a configuration
 * @c: the configuration to support the UVC instance
 * Context: single threaded during gadget setup
 *
 * Returns zero on success, else negative errno.
 *
 * Caller must have called @uvc_setup(). Caller is also responsible for
 * calling @uvc_cleanup() before module unload.
 */
int __init
uvc_bind_config(struct usb_configuration *c,
		const struct uvc_descriptor_header * const *control,
		const struct uvc_descriptor_header * const *fs_streaming,
		const struct uvc_descriptor_header * const *hs_streaming)
{
	struct uvc_device *uvc;
	struct uvc_device *uvc2;
	struct uvc_common *comm;
	int ret = 0;

	/* TODO Check if the USB device controller supports the required
	 * features.
	 */
	if (!gadget_is_dualspeed(c->cdev->gadget))
		return -EINVAL;

	uvc = kzalloc(sizeof(*uvc), GFP_KERNEL);
	if (uvc == NULL)
		return -ENOMEM;

	uvc->state = UVC_STATE_DISCONNECTED;


	comm = kzalloc(sizeof(*comm), GFP_KERNEL);
	if (comm == NULL)
		return -ENOMEM;

	uvc->comm = comm;
	comm->uvc1 = uvc;


	uvc2 = kzalloc(sizeof(*uvc), GFP_KERNEL);
	if (uvc == NULL)
		return -ENOMEM;

	uvc2->state = UVC_STATE_DISCONNECTED;
	uvc2->comm = comm;
	comm->uvc2 = uvc2;

	/* Validate the descriptors. */
	if (control == NULL || control[0] == NULL ||
		control[0]->bDescriptorSubType != UVC_VC_HEADER)
		goto error;

	if (fs_streaming == NULL || fs_streaming[0] == NULL ||
		fs_streaming[0]->bDescriptorSubType != UVC_VS_INPUT_HEADER)
		goto error;

	if (hs_streaming == NULL || hs_streaming[0] == NULL ||
		hs_streaming[0]->bDescriptorSubType != UVC_VS_INPUT_HEADER)
		goto error;

	uvc->comm->desc.control = (struct uvc_descriptor_header **)control;
	uvc->comm->desc.fs_streaming =
		(struct uvc_descriptor_header **)fs_streaming;
	uvc->comm->desc.hs_streaming =
		(struct uvc_descriptor_header **)hs_streaming;

#ifdef UVC_DOUBLE_STREAM
	uvc->comm->desc.fs_streaming2 =
		(struct uvc_descriptor_header **)fs_streaming;
	uvc->comm->desc.hs_streaming2 =
		(struct uvc_descriptor_header **)hs_streaming;
#endif

	/* Allocate string descriptor numbers. */
	ret = usb_string_id(c->cdev);
	if (ret < 0)
		goto error;
	uvc_en_us_strings[UVC_STRING_ASSOCIATION_IDX].id = ret;
	uvc_iad.iFunction = ret;

	ret = usb_string_id(c->cdev);
	if (ret < 0)
		goto error;
	uvc_en_us_strings[UVC_STRING_CONTROL_IDX].id = ret;
	uvc_control_intf.iInterface = ret;

	ret = usb_string_id(c->cdev);
	if (ret < 0)
		goto error;
	uvc_en_us_strings[UVC_STRING_STREAMING_IDX].id = ret;
	uvc_streaming1_intf_alt0.iInterface = ret;
	uvc_streaming1_intf_alt1.iInterface = ret;
	uvc_streaming1_intf_alt2.iInterface = ret;

	uvc_streaming2_intf_alt0.iInterface = ret;
	uvc_streaming2_intf_alt1.iInterface = ret;
	uvc_streaming2_intf_alt2.iInterface = ret;

	/* Register the function. */
	uvc->comm->func.name = "uvc";
	uvc->comm->func.strings = uvc_function_strings;
	uvc->comm->func.bind = uvc_function_bind;
	uvc->comm->func.unbind = uvc_function_unbind;
	uvc->comm->func.get_alt = uvc_function_get_alt;
	uvc->comm->func.set_alt = uvc_function_set_alt;
	uvc->comm->func.disable = uvc_function_disable;
	uvc->comm->func.setup = uvc_function_setup;

	ret = usb_add_function(c, &uvc->comm->func);
	if (ret)
		kfree(uvc);
#ifdef UVC_DOUBLE_STREAM
	printk(KERN_EMERG "######### uvc bind 2 streams#########\n");
#else
	printk(KERN_EMERG "######### uvc bind 1 stream#########\n");
#endif
	return 0;

error:
	kfree(uvc);
	return ret;
}

module_param_named(trace, uvc_gadget_trace_param, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(trace, "Trace level bitmask");

