/*
 * uvc.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *	uvc_gadget.h  --  USB Video Class Gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 */

#ifndef _UVC_GADGET_H_
#define _UVC_GADGET_H_

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/usb/composite.h>
#include <linux/videodev2.h>

#include <media/v4l2-device.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-fh.h>

#include "u_uvc.h"
#include "uvc_queue.h"

struct usb_ep;
struct usb_request;
struct uvc_descriptor_header;
struct uvc_device;

/* ------------------------------------------------------------------------
 * Debugging, printing and logging
 */

#define UVC_TRACE_PROBE (1 << 0)
#define UVC_TRACE_DESCR (1 << 1)
#define UVC_TRACE_CONTROL (1 << 2)
#define UVC_TRACE_FORMAT (1 << 3)
#define UVC_TRACE_CAPTURE (1 << 4)
#define UVC_TRACE_CALLS (1 << 5)
#define UVC_TRACE_IOCTL (1 << 6)
#define UVC_TRACE_FRAME (1 << 7)
#define UVC_TRACE_SUSPEND (1 << 8)
#define UVC_TRACE_STATUS (1 << 9)

#define UVC_WARN_MINMAX 0
#define UVC_WARN_PROBE_DEF 1

extern unsigned int uvc_gadget_trace_param;

#define uvc_trace(flag, msg...)                                                \
	do {                                                                   \
		if (uvc_gadget_trace_param & flag)                             \
			printk(KERN_DEBUG "uvcvideo: " msg);                   \
	} while (0)

#define uvcg_dbg(f, fmt, args...)                                              \
	dev_dbg(&(f)->config->cdev->gadget->dev, "%s: " fmt, (f)->name, ##args)
#define uvcg_info(f, fmt, args...)                                             \
	dev_info(&(f)->config->cdev->gadget->dev, "%s: " fmt, (f)->name, ##args)
#define uvcg_warn(f, fmt, args...)                                             \
	dev_warn(&(f)->config->cdev->gadget->dev, "%s: " fmt, (f)->name, ##args)
#define uvcg_err(f, fmt, args...)                                              \
	dev_err(&(f)->config->cdev->gadget->dev, "%s: " fmt, (f)->name, ##args)

/* ------------------------------------------------------------------------
 * Driver specific constants
 */

#ifdef CONFIG_UVC_STREAM_ERR_SUPPORT
#define UVC_ISOC_NUM_REQUESTS 8
#else
#define UVC_ISOC_NUM_REQUESTS 40
#endif
#define UVC_BULK_NUM_REQUESTS 8

#define UVC_MAX_REQUEST_SIZE 64
#define UVC_MAX_EVENTS 4

#define UVCG_REQUEST_HEADER_LEN 12

/* ------------------------------------------------------------------------
 * Structures
 */
struct uvc_request {
	struct usb_request *req;
	u8 *req_buffer;
	struct uvc_video *video;
	struct sg_table sgt;
	u8 header[UVCG_REQUEST_HEADER_LEN];
};

struct uvc_video {
	struct uvc_device *uvc;
	struct usb_ep *ep;

	struct usb_endpoint_descriptor ep_desc;
	struct usb_ss_ep_comp_descriptor ep_comp_desc;

	struct work_struct pump;

	/* Frame parameters */
	u8 bpp;
	u32 fcc;
	unsigned int width;
	unsigned int height;
	unsigned int imagesize;
	struct mutex mutex; /* protects frame parameters */

	unsigned int uvc_num_requests;

	/* Requests */
	unsigned int req_size;
	struct uvc_request *ureq;
	struct list_head req_free;
	spinlock_t req_lock;

	unsigned int req_int_count;

	void (*encode)(struct usb_request *req, struct uvc_video *video,
		       struct uvc_buffer *buf);

	/* Context data used by the completion handler */
	__u32 payload_size;
	__u32 max_payload_size;

	struct uvc_video_queue queue;
	unsigned int fid;
	unsigned int altsetting;
};

enum uvc_state {
	UVC_STATE_DISCONNECTED,
	UVC_STATE_CONNECTED,
	UVC_STATE_STREAMING,
};

#ifndef CONFIG_SS_GADGET_UVC_MULTI_STREAM
struct uvc_device {
	struct video_device vdev;
	struct v4l2_device v4l2_dev;
	enum uvc_state state;
	struct usb_function func;
	struct uvc_video video;
	bool func_connected;

	/* Descriptors */
	struct {
		const struct uvc_descriptor_header *const *fs_control;
		const struct uvc_descriptor_header *const *ss_control;
		const struct uvc_descriptor_header *const *fs_streaming;
		const struct uvc_descriptor_header *const *hs_streaming;
		const struct uvc_descriptor_header *const *ss_streaming;
	} desc;

	unsigned int control_intf;
	struct usb_ep *control_ep;
	struct usb_request *control_req;
	void *control_buf;

	unsigned int streaming_intf;

	/* Events */
	unsigned int event_length;
	unsigned int event_setup_out : 1;
};
#else
struct uvc_device {
	//struct video_device vdev;
	//struct v4l2_device v4l2_dev;
	//enum uvc_state state;
	struct usb_function func;
	//struct uvc_video video;
	//bool func_connected;
	struct list_head streams;
	unsigned int nstreams;

	/* Descriptors */
	struct {
		const struct uvc_descriptor_header *const *fs_control;
		const struct uvc_descriptor_header *const *ss_control;
		const struct uvc_descriptor_header *const *fs_streaming;
		const struct uvc_descriptor_header *const *hs_streaming;
		const struct uvc_descriptor_header *const *ss_streaming;
	} desc;

	unsigned int control_intf;
	struct usb_ep *control_ep;
	struct usb_request *control_req;
	void *control_buf;

	//unsigned int streaming_intf;

	/* Events */
	unsigned int event_length;
	unsigned int event_setup_out : 1;
};

struct uvc_streaming {
	struct list_head list;
	struct uvc_device *dev;

	/* Streams */
	struct video_device vdev;
	struct v4l2_device v4l2_dev;
	enum uvc_state state;
	struct uvc_video video;
	bool func_connected;

	/* Descriptors */
	unsigned int streaming_intf;
	unsigned char iInterface;
	unsigned char bTerminalID;
	unsigned int fs_streaming_size;
	unsigned int hs_streaming_size;
	unsigned int ss_streaming_size;
	void *fs_streaming_buf;
	void *hs_streaming_buf;
	void *ss_streaming_buf;

	/* Events */
	unsigned int event_setup_out : 1;
};
#endif

static inline struct uvc_device *to_uvc(struct usb_function *f)
{
	return container_of(f, struct uvc_device, func);
}

struct uvc_file_handle {
	struct v4l2_fh vfh;
	struct uvc_video *device;
	bool is_uvc_app_handle;
};

#define to_uvc_file_handle(handle)                                             \
	container_of(handle, struct uvc_file_handle, vfh)

/* ------------------------------------------------------------------------
 * Functions
 */

extern void uvc_function_setup_continue(struct uvc_device *uvc);
extern void uvc_endpoint_stream(struct uvc_device *dev);

extern void uvc_function_connect(struct uvc_device *uvc);
extern void uvc_function_disconnect(struct uvc_device *uvc);

#endif /* _UVC_GADGET_H_ */
