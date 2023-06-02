/*
 *	uvc_video.c  --  USB Video Class Gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/video.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>

#include "uvc.h"
#include "uvc_queue.h"
#include "uvc_video.h"
#include "u_uvc.h"

#define UVC_HEADER_LEN 12

extern bool using_cma_buf;

/* --------------------------------------------------------------------------
 * Video codecs
 */

#ifdef CONFIG_USB_WEBCAM_UVC_SUPPORT_SG_TABLE
static int
uvc_video_encode_header_sgt(struct uvc_video *video, struct uvc_buffer *buf,
		struct usb_request *req, int len)
{
	char *data = req->buf;
#if defined(CONFIG_UVC_STREAM_ERR_SUPPORT)
	struct uvc_video_queue *queue = &video->queue;
#endif

	data[0] = UVC_HEADER_LEN;
	data[1] = UVC_STREAM_EOH | video->fid;
	data[2] = 0;
	data[3] = 0;
	data[4] = 0;
	data[5] = 0;
	data[6] = 0;
	data[7] = 0;
	data[8] = 0;
	data[9] = 0;
	data[10] = 0;
	data[11] = 0;

	if ((buf->bytesused - video->queue.buf_used <= len - UVC_HEADER_LEN) && (buf->bFrameEnd)) {
		data[1] |= UVC_STREAM_EOF;
#if defined(CONFIG_UVC_STREAM_ERR_SUPPORT)
		if (queue->bXferFlag & FLAG_UVC_XFER_ERR) {
			data[1] |= UVC_STREAM_ERR;  // inform host to drop the outgoing frame
			queue->bXferFlag = FLAG_UVC_XFER_OK;
			if ((video->fcc == V4L2_PIX_FMT_H264) || (video->fcc == V4L2_PIX_FMT_H265)) {
				// flag to send forceIDR event and wait key frame to xfer
				queue->bXferFlag = FLAG_UVC_EVENT_KEYFRAME | FLAG_UVC_WAIT_KEYFRAME;
			}
		}
		else if (queue->bXferFlag & FLAG_UVC_WAIT_KEYFRAME) {
			if (buf->buf.flags & V4L2_BUF_FLAG_KEYFRAME) {
				queue->bXferFlag = FLAG_UVC_XFER_OK;
			}
			else {
				data[1] |= UVC_STREAM_ERR; // keep inform host to drop the outgoing frame until key frame
			}
		}
#endif
	}

	if (!video->queue.cur_sg)
	{
		video->queue.cur_sg = buf->sgt.sgl;
	}
	sg_init_table(req->sgt.sgl, req->sgt.nents);
	return UVC_HEADER_LEN;
}

static int
uvc_video_encode_data_sgt(struct uvc_video *video, struct uvc_buffer *buf,
		struct usb_request *req, int len)
{
	struct uvc_video_queue *queue = &video->queue;
	unsigned int nbytes;
	void *mem;

	/* Copy video data to the USB buffer. */
	mem = buf->mem + queue->buf_used;
	nbytes = min((unsigned int)len, buf->bytesused - queue->buf_used);

	sg_set_buf(req->sgt.sgl, req->buf, UVC_HEADER_LEN);
	if (sg_copy(queue->cur_sg, sg_next(req->sgt.sgl), nbytes))
	{
		BUG();
	}
	req->sg = req->sgt.sgl;
	req->num_sgs = sg_nents(req->sgt.sgl);
	queue->cur_sg = sg_advance(queue->cur_sg, nbytes);

	queue->buf_used += nbytes;

	return nbytes;
}

static void
uvc_video_encode_isoc_sgt(struct usb_request *req, struct uvc_video *video,
		struct uvc_buffer *buf)
{
	int len = video->req_size;
	int ret;

	/* Add the header. */
	ret = uvc_video_encode_header_sgt(video, buf, req, len);
	len -= ret;

	/* Process video data. */
	ret = uvc_video_encode_data_sgt(video, buf, req, len);
	len -= ret;

	req->length = video->req_size - len;
	req->last_req = 0;

	if (buf->bytesused == video->queue.buf_used) {
		req->last_req = 1;
		video->queue.cur_sg = NULL;
		video->queue.buf_used = 0;
		buf->state = UVC_BUF_STATE_DONE;

		if (buf->bFrameEnd)
			video->fid ^= UVC_STREAM_FID;
	}
}
#endif

static int
uvc_video_encode_header(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len)
{
#if defined(CONFIG_UVC_STREAM_ERR_SUPPORT)
	struct uvc_video_queue *queue = &video->queue;
#endif

	data[0] = UVC_HEADER_LEN;
	data[1] = UVC_STREAM_EOH | video->fid;
	data[2] = 0;
	data[3] = 0;
	data[4] = 0;
	data[5] = 0;
	data[6] = 0;
	data[7] = 0;
	data[8] = 0;
	data[9] = 0;
	data[10] = 0;
	data[11] = 0;

#if defined(CONFIG_SS_GADGET) ||defined(CONFIG_SS_GADGET_MODULE)
	if ((buf->bytesused - video->queue.buf_used <= len - UVC_HEADER_LEN) && (buf->bFrameEnd))
#else
	if (buf->bytesused - video->queue.buf_used <= len - UVC_HEADER_LEN)
#endif
	{
		data[1] |= UVC_STREAM_EOF;
#if defined(CONFIG_UVC_STREAM_ERR_SUPPORT)
		if (queue->bXferFlag & FLAG_UVC_XFER_ERR) {
			data[1] |= UVC_STREAM_ERR;  // inform host to drop the outgoing frame
			queue->bXferFlag = FLAG_UVC_XFER_OK;
			if ((video->fcc == V4L2_PIX_FMT_H264) || (video->fcc == V4L2_PIX_FMT_H265)) {
				// flag to send forceIDR event and wait key frame to xfer
				queue->bXferFlag = FLAG_UVC_EVENT_KEYFRAME | FLAG_UVC_WAIT_KEYFRAME;
			}
		}
		else if (queue->bXferFlag & FLAG_UVC_WAIT_KEYFRAME) {
			if (buf->buf.flags & V4L2_BUF_FLAG_KEYFRAME) {
				queue->bXferFlag = FLAG_UVC_XFER_OK;
			}
			else {
				data[1] |= UVC_STREAM_ERR; // keep inform host to drop the outgoing frame until key frame
			}
		}
#endif
	}

	return UVC_HEADER_LEN;
}

static int
uvc_video_encode_data(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len)
{
	struct uvc_video_queue *queue = &video->queue;
	unsigned int nbytes;
	void *mem;

	/* Copy video data to the USB buffer. */
	mem = buf->mem + queue->buf_used;
	nbytes = min((unsigned int)len, buf->bytesused - queue->buf_used);

	memcpy(data, mem, nbytes);
	queue->buf_used += nbytes;

	return nbytes;
}

#if defined(CONFIG_SS_GADGET) ||defined(CONFIG_SS_GADGET_MODULE)
static void
uvc_video_encode_bulk(struct usb_request *req, struct uvc_video *video,
		struct uvc_buffer *buf)
{
	void *mem = req->buf;
	int len = video->req_size;
	int ret;

	/* Add a header at the beginning of the payload. */
	if (video->payload_size == 0) {
		ret = uvc_video_encode_header(video, buf, mem, len);
		video->payload_size += ret;
		mem += ret;
		len -= ret;
	}

#if defined(CONFIG_SS_GADGET) || defined(CONFIG_SS_GADGET_MODULE)
	if ((buf->bytesused - video->queue.buf_used + UVC_HEADER_LEN) <= video->max_payload_size
			&& (buf->bytesused - video->queue.buf_used + UVC_HEADER_LEN) % video->ep->maxpacket == 0) {
		char *data = req->buf;
		len = buf->bytesused - video->queue.buf_used - 32;
		ret = uvc_video_encode_data(video, buf, mem, len);
		req->length = ret + UVC_HEADER_LEN;
		video->payload_size = 0;
		data[1] &= ~UVC_STREAM_EOF;
		return;
	}
#endif

	/* Process video data. */
	len = min((int)(video->max_payload_size - video->payload_size), len);
	ret = uvc_video_encode_data(video, buf, mem, len);
	video->payload_size += ret;
	len -= ret;

	req->length = video->req_size - len;
	//req->zero = video->payload_size == video->max_payload_size;

	if (buf->bytesused == video->queue.buf_used) {
		video->queue.buf_used = 0;
		buf->state = UVC_BUF_STATE_DONE;
		uvcg_queue_next_buffer(&video->queue, buf);
		if (buf->bFrameEnd)
			video->fid ^= UVC_STREAM_FID;

		video->payload_size = 0;
	}

	if (video->payload_size == video->max_payload_size ||
	    buf->bytesused == video->queue.buf_used)
		video->payload_size = 0;
}
#endif

static void
uvc_video_encode_isoc(struct usb_request *req, struct uvc_video *video,
		struct uvc_buffer *buf)
{
	void *mem = req->buf;
	int len = video->req_size;
	int ret;

	/* Add the header. */
	ret = uvc_video_encode_header(video, buf, mem, len);
	mem += ret;
	len -= ret;

	/* Process video data. */
	ret = uvc_video_encode_data(video, buf, mem, len);
	len -= ret;

	req->length = video->req_size - len;

	if (buf->bytesused == video->queue.buf_used) {
		video->queue.buf_used = 0;
		buf->state = UVC_BUF_STATE_DONE;
		uvcg_queue_next_buffer(&video->queue, buf);
#if defined(CONFIG_SS_GADGET) ||defined(CONFIG_SS_GADGET_MODULE)
		if (buf->bFrameEnd)
			video->fid ^= UVC_STREAM_FID;
#else
		video->fid ^= UVC_STREAM_FID;
#endif
	}
}

/* --------------------------------------------------------------------------
 * Request handling
 */
static int uvcg_video_ep_queue(struct uvc_video *video, struct usb_request *req)
{
	int ret = -ESHUTDOWN;

#ifdef CONFIG_USB_WEBCAM_UVC_SUPPORT_SG_TABLE
#ifdef DEBUG_SG//test
{
		struct uvc_device *uvc = video_to_uvc(video);
		struct usb_gadget * gadget = fuvc_to_gadget(uvc->func);

		if (gadget->sg_supported)
		{
			char *buf = kmalloc(req->length, GFP_KERNEL);
			unsigned int copy_len = 0;
			unsigned int correct = 1;
			unsigned i;

			copy_len = sg_pcopy_to_buffer(req->sgt.sgl, req->sgt.nents, buf, req->length, 0);

			for (i=0; i < req->length;i++)
			{
				if (buf[i]!= ((char*)req->buf)[i])
				{
					printk(KERN_DEBUG"index%d val%d val%d\n", i, buf[i], ((char*)req->buf)[i]);
					correct = 0;
				}
			}
			printk(KERN_DEBUG"data is header0x%x 0x%x\n", buf[0], buf[1]);
			printk(KERN_DEBUG"data is (%s) reqlen%d copylen%d req->num_sgs%d\n",
				correct?"right":"bad", req->length, copy_len, req->num_sgs);
			kfree(buf);
		}
}
#endif
#endif
	if (video->ep && video->ep->enabled) {
	    ret = usb_ep_queue(video->ep, req, GFP_ATOMIC);
		if (ret < 0) {
			uvcg_err(&video->uvc->func, "Failed to queue request (%d).\n", ret);

			/* Isochronous endpoints can't be halted. */
			if (video->ep->desc && usb_endpoint_xfer_bulk(video->ep->desc))
				usb_ep_set_halt(video->ep);
		}
	}

	return ret;
}

/*
 * I somehow feel that synchronisation won't be easy to achieve here. We have
 * three events that control USB requests submission:
 *
 * - USB request completion: the completion handler will resubmit the request
 *   if a video buffer is available.
 *
 * - USB interface setting selection: in response to a SET_INTERFACE request,
 *   the handler will start streaming if a video buffer is available and if
 *   video is not currently streaming.
 *
 * - V4L2 buffer queueing: the driver will start streaming if video is not
 *   currently streaming.
 *
 * Race conditions between those 3 events might lead to deadlocks or other
 * nasty side effects.
 *
 * The "video currently streaming" condition can't be detected by the irqqueue
 * being empty, as a request can still be in flight. A separate "queue paused"
 * flag is thus needed.
 *
 * The paused flag will be set when we try to retrieve the irqqueue head if the
 * queue is empty, and cleared when we queue a buffer.
 *
 * The USB request completion handler will get the buffer at the irqqueue head
 * under protection of the queue spinlock. If the queue is empty, the streaming
 * paused flag will be set. Right after releasing the spinlock a userspace
 * application can queue a buffer. The flag will then cleared, and the ioctl
 * handler will restart the video stream.
 */
static void
uvc_video_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct uvc_video *video = req->context;
	struct uvc_video_queue *queue = &video->queue;
#ifdef CONFIG_USB_WEBCAM_UVC_SUPPORT_SG_TABLE
	struct uvc_device *uvc = video_to_uvc(video);
	struct usb_gadget * gadget = fuvc_to_gadget(uvc->func);
#endif
	unsigned long flags;

	switch (req->status) {
	case 0:
#ifdef CONFIG_USB_WEBCAM_UVC_SUPPORT_SG_TABLE
		if (gadget->sg_supported && req->last_req == 1)
		{
			req->last_req = 0;
			uvcg_complete_sg(queue);
		}
#endif
		break;

	case -ESHUTDOWN:	/* disconnect from host. */
		uvcg_dbg(&video->uvc->func, "VS request cancelled.\n");
		uvcg_queue_cancel(queue, 1);
		break;

	case -ENOSR:    /* uncompleted request */
#if defined(CONFIG_UVC_STREAM_ERR_SUPPORT)
		spin_lock_irqsave(&queue->irqlock, flags);
		if (queue->bXferFlag == FLAG_UVC_XFER_OK)
			queue->bXferFlag = FLAG_UVC_XFER_ERR;
		spin_unlock_irqrestore(&queue->irqlock, flags);
#endif
		req->status = 0;
		break;

	default:
		uvcg_info(&video->uvc->func,
			  "VS request completed with status %d.\n",
			  req->status);
		uvcg_queue_cancel(queue, 0);
	}

	spin_lock_irqsave(&video->req_lock, flags);
	list_add_tail(&req->list, &video->req_free);
	spin_unlock_irqrestore(&video->req_lock, flags);

	schedule_work(&video->pump);
}

static int
uvc_video_free_requests(struct uvc_video *video)
{
	unsigned int i;
	struct uvc_device *uvc = video_to_uvc(video);
	struct usb_gadget * gadget = fuvc_to_gadget(uvc->func);

	for (i = 0; i < UVC_NUM_REQUESTS; ++i) {
		if (video->req[i]) {
#ifdef CONFIG_USB_WEBCAM_UVC_SUPPORT_SG_TABLE
		if (gadget->sg_supported)
		{
			sg_free_table(&video->req[i]->sgt);
		}
#endif
			usb_ep_free_request(video->ep, video->req[i]);
			video->req[i] = NULL;
		}

		if (video->req_buffer[i]) {
			if (!using_cma_buf)
				kfree(video->req_buffer[i]);
			else
				dma_free_coherent(&gadget->dev, video->req_size,
					video->req_buffer[i], video->req_buffer_dma_handle[i]);
			video->req_buffer[i] = NULL;
		}
	}

	INIT_LIST_HEAD(&video->req_free);
	video->req_size = 0;
	return 0;
}

static int
uvc_video_alloc_requests(struct uvc_video *video)
{
	unsigned int req_size;
	unsigned int i;
#if defined(CONFIG_SS_GADGET) ||defined(CONFIG_SS_GADGET_MODULE)
	struct uvc_device *uvc = video_to_uvc(video);
	struct f_uvc_opts *opts = fi_to_f_uvc_opts(uvc->func.fi);
#endif
	int ret = -ENOMEM;

	struct usb_gadget * gadget = fuvc_to_gadget(uvc->func);

	BUG_ON(video->req_size);

#if defined(CONFIG_SS_GADGET) ||defined(CONFIG_SS_GADGET_MODULE)
	if (opts->bulk_streaming_ep)
	{
		req_size = 16 * 1024;
		video->max_payload_size = req_size;
	}
	else
#endif
	{
		req_size = video->ep->maxpacket
			* max_t(unsigned int, video->ep->maxburst, 1)
			* (video->ep->mult);
	}

	for (i = 0; i < UVC_NUM_REQUESTS; ++i) {
		if (!using_cma_buf)
			video->req_buffer[i] = kmalloc(req_size, GFP_KERNEL);
		else
		{
			static u64 dma_mask = 0xffffffffUL;

			gadget->dev.dma_mask = &dma_mask;
			gadget->dev.coherent_dma_mask = dma_mask;

			video->req_buffer[i] = dma_alloc_coherent(&gadget->dev,
				req_size, &video->req_buffer_dma_handle[i], GFP_KERNEL);
		}
		if (video->req_buffer[i] == NULL)
		{
			printk("alloc buffer err\n");
			goto error;
		}

		video->req[i] = usb_ep_alloc_request(video->ep, GFP_KERNEL);
		if (video->req[i] == NULL)
			goto error;

		video->req[i]->buf = video->req_buffer[i];
		video->req[i]->length = 0;
		video->req[i]->complete = uvc_video_complete;
		video->req[i]->context = video;

#ifdef CONFIG_USB_WEBCAM_UVC_SUPPORT_SG_TABLE
	if (gadget->sg_supported)
	{
		ret = sg_alloc_table(&video->req[i]->sgt,
			UVC_MAX_REQ_SG_LIST_NUM,
			GFP_KERNEL);
		if (ret)
		{
			goto error;
		}
	}
#endif
		list_add_tail(&video->req[i]->list, &video->req_free);
	}

	video->req_size = req_size;

	return 0;

error:
	uvc_video_free_requests(video);
	return ret;
}

/* --------------------------------------------------------------------------
 * Video streaming
 */

/*
 * uvcg_video_pump - Pump video data into the USB requests
 *
 * This function fills the available USB requests (listed in req_free) with
 * video data from the queued buffers.
 */
static void uvcg_video_pump(struct work_struct *work)
{
	struct uvc_video *video = container_of(work, struct uvc_video, pump);
	struct uvc_video_queue *queue = &video->queue;
	struct usb_request *req;
	struct uvc_buffer *buf;
#ifdef CONFIG_USB_WEBCAM_UVC_SUPPORT_SG_TABLE
	struct uvc_device *uvc = video_to_uvc(video);
	struct usb_gadget * gadget = fuvc_to_gadget(uvc->func);
#endif
	unsigned long flags;
	int ret;

	/* FIXME TODO Race between uvcg_video_pump and requests completion
	 * handler ???
	 */

	while (1) {
		/* Retrieve the first available USB request, protected by the
		 * request lock.
		 */
		spin_lock_irqsave(&video->req_lock, flags);
		if (list_empty(&video->req_free)) {
			spin_unlock_irqrestore(&video->req_lock, flags);
			return;
		}
		req = list_first_entry(&video->req_free, struct usb_request,
					list);
		list_del(&req->list);
		spin_unlock_irqrestore(&video->req_lock, flags);

		/* Retrieve the first available video buffer and fill the
		 * request, protected by the video queue irqlock.
		 */
		spin_lock_irqsave(&queue->irqlock, flags);

#if defined(CONFIG_UVC_STREAM_ERR_SUPPORT)
		// send event to force encoding a key frame
		if (queue->bXferFlag & FLAG_UVC_EVENT_KEYFRAME) {
			struct v4l2_event v4l2_event;

			queue->bXferFlag &= ~(FLAG_UVC_EVENT_KEYFRAME);
			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UVC_EVENT_FORCEIDR;
#ifdef CONFIG_SS_GADGET_UVC_MULTI_STREAM
			v4l2_event_queue(&((video_to_stream(video))->vdev), &v4l2_event);
#else
			v4l2_event_queue(&uvc->vdev, &v4l2_event);
#endif
		}
#endif
#ifdef CONFIG_USB_WEBCAM_UVC_SUPPORT_SG_TABLE
		if (gadget->sg_supported)
			buf = uvcg_prepare_sg(queue);
		else
#endif
			buf = uvcg_queue_head(queue);
		if (buf == NULL) {
			spin_unlock_irqrestore(&queue->irqlock, flags);
			break;
		}

		video->encode(req, video, buf);

		/* Queue the USB request */
		ret = uvcg_video_ep_queue(video, req);
		spin_unlock_irqrestore(&queue->irqlock, flags);

		if (ret < 0) {
			uvcg_queue_cancel(queue, 0);
			break;
		}
	}

	spin_lock_irqsave(&video->req_lock, flags);
	list_add_tail(&req->list, &video->req_free);
	spin_unlock_irqrestore(&video->req_lock, flags);
	return;
}

/*
 * Enable or disable the video stream.
 */
int uvcg_video_enable(struct uvc_video *video, int enable)
{
	unsigned int i;
	int ret;
#if defined(CONFIG_SS_GADGET) ||defined(CONFIG_SS_GADGET_MODULE)
	struct uvc_device *uvc = video_to_uvc(video);
	struct f_uvc_opts *opts = fi_to_f_uvc_opts(uvc->func.fi);
#ifdef CONFIG_USB_WEBCAM_UVC_SUPPORT_SG_TABLE
	struct usb_gadget * gadget = fuvc_to_gadget(uvc->func);
#endif
#endif

	if (video->ep == NULL) {
		uvcg_info(&video->uvc->func,
			  "Video enable failed, device is uninitialized.\n");
		return -ENODEV;
	}

	if (!enable) {
		cancel_work_sync(&video->pump);
		uvcg_queue_cancel(&video->queue, 0);

		for (i = 0; i < UVC_NUM_REQUESTS; ++i)
			if (video->req[i])
				usb_ep_dequeue(video->ep, video->req[i]);

		uvc_video_free_requests(video);
		uvcg_queue_enable(&video->queue, 0);
		return 0;
	}

	if ((ret = uvcg_queue_enable(&video->queue, 1)) < 0)
		return ret;

#if defined(CONFIG_SS_GADGET) ||defined(CONFIG_SS_GADGET_MODULE)
	if (opts->bulk_streaming_ep)
	{
		video->encode = uvc_video_encode_bulk;
		video->payload_size = 0;
	} else
#endif
	{
		video->encode = uvc_video_encode_isoc;

#ifdef CONFIG_USB_WEBCAM_UVC_SUPPORT_SG_TABLE
	if (gadget->sg_supported)
	{
		video->encode = uvc_video_encode_isoc_sgt;
	}
#endif
	}

	if ((ret = uvc_video_alloc_requests(video)) < 0)
	{
		video->encode = NULL;
		return ret;
	}

	schedule_work(&video->pump);

	return ret;
}

/*
 * Initialize the UVC video stream.
 */
int uvcg_video_init(struct uvc_video *video, struct uvc_device *uvcdev)
{
#if defined(CONFIG_SS_GADGET) ||defined(CONFIG_SS_GADGET_MODULE)
	struct uvc_device *uvc = video_to_uvc(video);
	struct f_uvc_opts *opts = fi_to_f_uvc_opts(uvc->func.fi);
#endif

	INIT_LIST_HEAD(&video->req_free);
	spin_lock_init(&video->req_lock);
	INIT_WORK(&video->pump, uvcg_video_pump);

	video->uvc = uvcdev;
	video->fcc = V4L2_PIX_FMT_YUYV;
	video->bpp = 16;
	video->width = 320;
	video->height = 240;
	video->imagesize = 320 * 240 * 2;

#if defined(CONFIG_SS_GADGET) ||defined(CONFIG_SS_GADGET_MODULE)
	if (opts->bulk_streaming_ep)
		video->max_payload_size = video->imagesize;
#endif
	/* Initialize the video buffers queue. */
	uvcg_queue_init(&video->queue, V4L2_BUF_TYPE_VIDEO_OUTPUT,
			&video->mutex);
	return 0;
}

