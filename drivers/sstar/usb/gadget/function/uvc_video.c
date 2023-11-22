/*
 * uvc_video.c- Sigmastar
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

// SPDX-License-Identifier: GPL-2.0+
/*
 *	uvc_video.c  --  USB Video Class Gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/video.h>
#include <linux/usb/g_uvc.h>
#include <asm/unaligned.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>

#include "uvc.h"
#include "uvc_queue.h"
#include "uvc_video.h"
#include "u_uvc.h"

/* --------------------------------------------------------------------------
 * Video codecs
 */

static int uvc_video_encode_header(struct uvc_video *video,
				   struct uvc_buffer *buf, u8 *data, int len)
{
#ifndef CONFIG_SS_GADGET_UVC_MULTI_STREAM
	struct uvc_device *uvc = container_of(video, struct uvc_device, video);
#else
	struct uvc_device *uvc = video_to_uvc(video);
#endif
	struct usb_composite_dev *cdev = uvc->func.config->cdev;
	struct timespec64 ts = ns_to_timespec64(buf->buf.vb2_buf.timestamp);
	int pos = 2;

	data[1] = UVC_STREAM_EOH | video->fid;

	if (video->queue.buf_used == 0 && ts.tv_sec) {
		/* dwClockFrequency is 48 MHz */
		u32 pts = ((u64)ts.tv_sec * USEC_PER_SEC +
			   ts.tv_nsec / NSEC_PER_USEC) *
			  48;

		data[1] |= UVC_STREAM_PTS;
		put_unaligned_le32(pts, &data[pos]);
		pos += 4;
	} else {
		data[1] |= UVC_STREAM_PTS;
		pos += 4;
	}

	if (cdev->gadget->ops->get_frame) {
		u32 sof, stc;

		sof = usb_gadget_frame_number(cdev->gadget);
		ktime_get_ts64(&ts);
		stc = ((u64)ts.tv_sec * USEC_PER_SEC +
		       ts.tv_nsec / NSEC_PER_USEC) *
		      48;

		data[1] |= UVC_STREAM_SCR;
		put_unaligned_le32(stc, &data[pos]);
		put_unaligned_le16(sof, &data[pos + 4]);
		pos += 6;
	} else {
		data[1] |= UVC_STREAM_SCR;
		pos += 6;
	}

#ifdef CONFIG_WINDOWS_HELLO_SUPPORT
	if (video->fcc == V4L2_PIX_FMT_GREY) {
		KSCAMERA_METADATA_FRAMEILLUMINATION uvc_payload_metadata;

		uvc_payload_metadata.Header.MetadataId =
			MetadataId_FrameIllumination;
		uvc_payload_metadata.Header.Size =
			sizeof(KSCAMERA_METADATA_FRAMEILLUMINATION);
		uvc_payload_metadata.Flags =
			(buf->reserved & UVC_BUFFER_FLAGS_IR_ON) ?
				      KSCAMERA_METADATA_FRAMEILLUMINATION_FLAG_ON :
				      KSCAMERA_METADATA_FRAMEILLUMINATION_FLAG_OFF;
		uvc_payload_metadata.Reserved = 0;

		memcpy(&data[pos], &uvc_payload_metadata,
		       uvc_payload_metadata.Header.Size);
		pos += uvc_payload_metadata.Header.Size;
	}
#endif

	data[0] = pos;

	if (buf->bytesused - video->queue.buf_used <= len - pos &&
	    UVC_BUFFER_FLAGS_FRAME_END(buf->reserved)) {
		data[1] |= UVC_STREAM_EOF;

#ifdef CONFIG_UVC_STREAM_ERR_SUPPORT
		/* inform host to drop the outgoing frame */
		if (video->queue.bXferFlag & FLAG_UVC_XFER_ERR) {
			data[1] |= UVC_STREAM_ERR;
			video->queue.bXferFlag = FLAG_UVC_XFER_OK;
			if ((video->fcc == V4L2_PIX_FMT_H264) ||
			    (video->fcc == V4L2_PIX_FMT_HEVC))
				/* flag to send forceIDR event and wait key frame to xfer */
				video->queue.bXferFlag =
					FLAG_UVC_EVENT_KEYFRAME |
					FLAG_UVC_WAIT_KEYFRAME;
		} else if (video->queue.bXferFlag & FLAG_UVC_WAIT_KEYFRAME) {
			if (buf->buf.flags & V4L2_BUF_FLAG_KEYFRAME)
				video->queue.bXferFlag = FLAG_UVC_XFER_OK;
			else
				/* keep inform host to drop the outgoing frame until key frame */
				data[1] |= UVC_STREAM_ERR;
		}
#endif
	}

	return pos;
}

static int uvc_video_encode_data(struct uvc_video *video,
				 struct uvc_buffer *buf, u8 *data, int len)
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

static void uvc_video_encode_bulk(struct usb_request *req,
				  struct uvc_video *video,
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

	/* Patch for mac-os, make short packet to avoid sending ZLP */
	if ((buf->bytesused - video->queue.buf_used + 12) <=
		    video->max_payload_size &&
	    (buf->bytesused - video->queue.buf_used + 12) %
			    video->ep->maxpacket ==
		    0) {
		char *data = req->buf;
		len = buf->bytesused - video->queue.buf_used - 32;
		ret = uvc_video_encode_data(video, buf, mem, len);
		req->length = ret + 12;
		video->payload_size = 0;
		data[1] &= ~UVC_STREAM_EOF;
		return;
	}

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
		if (UVC_BUFFER_FLAGS_FRAME_END(buf->reserved))
			video->fid ^= UVC_STREAM_FID;

		video->payload_size = 0;
	}

	if (video->payload_size == video->max_payload_size ||
	    buf->bytesused == video->queue.buf_used)
		video->payload_size = 0;
}

static void uvc_video_encode_isoc_sg(struct usb_request *req,
				     struct uvc_video *video,
				     struct uvc_buffer *buf)
{
	unsigned int pending = buf->bytesused - video->queue.buf_used;
	struct uvc_request *ureq = req->context;
	struct scatterlist *sg, *iter;
	unsigned int len = video->req_size;
	unsigned int sg_left, part = 0;
	unsigned int i;
	int header_len;

	sg = ureq->sgt.sgl;
	sg_init_table(sg, ureq->sgt.nents);

	/* Init the header. */
	header_len = uvc_video_encode_header(video, buf, ureq->header,
					     video->req_size);
	sg_set_buf(sg, ureq->header, header_len);
	len -= header_len;

	if (pending <= len)
		len = pending;

	req->length = (len == pending) ? len + header_len : video->req_size;

	/* Init the pending sgs with payload */
	sg = sg_next(sg);

	for_each_sg (sg, iter, ureq->sgt.nents - 1, i) {
		if (!len || !buf->sg)
			break;

		sg_left = sg_dma_len(buf->sg) - buf->offset;
		part = min_t(unsigned int, len, sg_left);

		sg_set_page(iter, sg_page(buf->sg), part, buf->offset);

		if (part == sg_left) {
			buf->offset = 0;
			buf->sg = sg_next(buf->sg);
		} else {
			buf->offset += part;
		}
		len -= part;
	}

	/* Assign the video data with header. */
	req->buf = NULL;
	req->sg = ureq->sgt.sgl;
	req->num_sgs = i + 1;

	req->length -= len;
	video->queue.buf_used += req->length - header_len;

	if (buf->bytesused == video->queue.buf_used || !buf->sg) {
		video->queue.buf_used = 0;
		buf->state = UVC_BUF_STATE_DONE;
		buf->offset = 0;
		/* don't queue next until last req done */
		req->last_req = 1;
		if (UVC_BUFFER_FLAGS_FRAME_END(buf->reserved))
			video->fid ^= UVC_STREAM_FID;
	}
}

static void uvc_video_encode_isoc(struct usb_request *req,
				  struct uvc_video *video,
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
		if (UVC_BUFFER_FLAGS_FRAME_END(buf->reserved))
			video->fid ^= UVC_STREAM_FID;
	}
}

/* --------------------------------------------------------------------------
 * Request handling
 */

static int uvcg_video_ep_queue(struct uvc_video *video, struct usb_request *req)
{
	int ret;

	ret = usb_ep_queue(video->ep, req, GFP_ATOMIC);
	if (ret < 0) {
		uvcg_err(&video->uvc->func, "Failed to queue request (%d).\n",
			 ret);

		/* If the endpoint is disabled the descriptor may be NULL. */
		if (video->ep->desc) {
			/* Isochronous endpoints can't be halted. */
			if (usb_endpoint_xfer_bulk(video->ep->desc))
				usb_ep_set_halt(video->ep);
		}
	}

	return ret;
}

static void uvc_video_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct uvc_request *ureq = req->context;
	struct uvc_video *video = ureq->video;
	struct uvc_video_queue *queue = &video->queue;
#ifndef CONFIG_SS_GADGET_UVC_MULTI_STREAM
	struct uvc_device *uvc = video->uvc;
#else
	struct uvc_streaming *stream = video_to_stream(video);
#endif
	unsigned long flags;

	switch (req->status) {
	case 0:
		if (queue->use_sg && req->last_req) {
			req->last_req = 0;
			spin_lock_irqsave(&queue->irqlock, flags);
			uvcg_complete_sg(queue);
			spin_unlock_irqrestore(&queue->irqlock, flags);
		}
		break;

	case -ESHUTDOWN: /* disconnect from host. */
		uvcg_dbg(&video->uvc->func, "VS request cancelled.\n");
		uvcg_queue_cancel(queue, 1);
		break;

	case -EXDEV: /* uncompleted request */
		if (queue->use_sg && req->last_req) {
			req->last_req = 0;
			spin_lock_irqsave(&queue->irqlock, flags);
			uvcg_complete_sg(queue);
			spin_unlock_irqrestore(&queue->irqlock, flags);
		}
#ifdef CONFIG_UVC_STREAM_ERR_SUPPORT
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

#ifndef CONFIG_SS_GADGET_UVC_MULTI_STREAM
	if (uvc->state == UVC_STATE_STREAMING)
#else
	if (stream->state == UVC_STATE_STREAMING)
#endif
		schedule_work(&video->pump);
}

static int uvc_video_free_requests(struct uvc_video *video)
{
	unsigned int i;

	if (video->ureq) {
		for (i = 0; i < video->uvc_num_requests; ++i) {
			sg_free_table(&video->ureq[i].sgt);

			if (video->ureq[i].req) {
				usb_ep_free_request(video->ep,
						    video->ureq[i].req);
				video->ureq[i].req = NULL;
			}

			if (video->ureq[i].req_buffer) {
				kfree(video->ureq[i].req_buffer);
				video->ureq[i].req_buffer = NULL;
			}
		}

		kfree(video->ureq);
		video->ureq = NULL;
	}

	INIT_LIST_HEAD(&video->req_free);
	video->req_size = 0;
	return 0;
}

static int uvc_video_alloc_requests(struct uvc_video *video)
{
	struct uvc_device *uvc = video_to_uvc(video);
	struct f_uvc_opts *opts = fi_to_f_uvc_opts(uvc->func.fi);
	unsigned int req_size;
	unsigned int i;
	int ret = -ENOMEM;

	BUG_ON(video->req_size);

	if (opts->bulk_streaming_ep) {
		req_size = 16 * 1024;
		video->max_payload_size = req_size;
	} else {
		req_size = video->ep->maxpacket *
			   max_t(unsigned int, video->ep->maxburst, 1) *
			   (video->ep->mult);
	}

	video->ureq = kcalloc(video->uvc_num_requests,
			      sizeof(struct uvc_request), GFP_KERNEL);
	if (video->ureq == NULL)
		return -ENOMEM;

	for (i = 0; i < video->uvc_num_requests; ++i) {
		video->ureq[i].req_buffer = kmalloc(req_size, GFP_KERNEL);
		if (video->ureq[i].req_buffer == NULL)
			goto error;

		video->ureq[i].req =
			usb_ep_alloc_request(video->ep, GFP_KERNEL);
		if (video->ureq[i].req == NULL)
			goto error;

		video->ureq[i].req->buf = video->ureq[i].req_buffer;
		video->ureq[i].req->length = 0;
		video->ureq[i].req->complete = uvc_video_complete;
		video->ureq[i].req->context = &video->ureq[i];
		video->ureq[i].video = video;

		list_add_tail(&video->ureq[i].req->list, &video->req_free);
		/* req_size/PAGE_SIZE + 1 for overruns and + 1 for header */
		sg_alloc_table(&video->ureq[i].sgt,
			       DIV_ROUND_UP(req_size - UVCG_REQUEST_HEADER_LEN,
					    PAGE_SIZE) +
				       2,
			       GFP_KERNEL);
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
	struct usb_request *req = NULL;
	struct uvc_buffer *buf;
	unsigned long flags;
	int ret;

	while (video->ep->enabled) {
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

#ifdef CONFIG_UVC_STREAM_ERR_SUPPORT
		/* send event to force encoding a key frame */
		if (queue->bXferFlag & FLAG_UVC_EVENT_KEYFRAME) {
			struct v4l2_event v4l2_event;

			queue->bXferFlag &= ~(FLAG_UVC_EVENT_KEYFRAME);
			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UVC_EVENT_FORCEIDR;
#ifndef CONFIG_SS_GADGET_UVC_MULTI_STREAM
			v4l2_event_queue(&video->uvc->vdev, &v4l2_event);
#else
			v4l2_event_queue(&((video_to_stream(video))->vdev),
					 &v4l2_event);
#endif
		}
#endif

		if (queue->use_sg)
			buf = uvcg_prepare_sg(queue);
		else
			buf = uvcg_queue_head(queue);
		if (buf == NULL) {
			spin_unlock_irqrestore(&queue->irqlock, flags);
			break;
		}

		video->encode(req, video, buf);

		/* With usb3 we have more requests. This will decrease the
		 * interrupt load to a quarter but also catches the corner
		 * cases, which needs to be handled */
		if (list_empty(&video->req_free) ||
		    buf->state == UVC_BUF_STATE_DONE ||
		    !(video->req_int_count %
		      DIV_ROUND_UP(video->uvc_num_requests, 4))) {
			video->req_int_count = 0;
			req->no_interrupt = 0;
		} else {
			req->no_interrupt = 1;
		}

		/* patch for mantis 1699038 */
		req->no_interrupt = 0;

		/* Queue the USB request */
		ret = uvcg_video_ep_queue(video, req);
		spin_unlock_irqrestore(&queue->irqlock, flags);

		if (ret < 0) {
			uvcg_queue_cancel(queue, 0);
			break;
		}
		video->req_int_count++;
	}

	if (!req)
		return;

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
	struct uvc_device *uvc = video_to_uvc(video);
	struct f_uvc_opts *opts = fi_to_f_uvc_opts(uvc->func.fi);
	unsigned int i;
	int ret;

	if (video->ep == NULL) {
		uvcg_info(&video->uvc->func,
			  "Video enable failed, device is uninitialized.\n");
		return -ENODEV;
	}

	if (!enable) {
		cancel_work_sync(&video->pump);
		uvcg_queue_cancel(&video->queue, 0);

		for (i = 0; i < video->uvc_num_requests; ++i)
			if (video->ureq && video->ureq[i].req)
				usb_ep_dequeue(video->ep, video->ureq[i].req);

		uvc_video_free_requests(video);
		uvcg_queue_enable(&video->queue, 0);
		return 0;
	}

	if ((ret = uvcg_queue_enable(&video->queue, 1)) < 0)
		return ret;

	if ((ret = uvc_video_alloc_requests(video)) < 0)
		return ret;

	if (opts->bulk_streaming_ep) {
		video->encode = uvc_video_encode_bulk;
		video->payload_size = 0;
	} else
		video->encode = video->queue.use_sg ? uvc_video_encode_isoc_sg :
							    uvc_video_encode_isoc;

	video->req_int_count = 0;

	schedule_work(&video->pump);

	return ret;
}

/*
 * Initialize the UVC video stream.
 */
int uvcg_video_init(struct uvc_video *video, struct uvc_device *uvc)
{
#ifndef CONFIG_SS_GADGET_UVC_MULTI_STREAM
	struct device *dev = uvc->v4l2_dev.dev->parent;
#else
	struct uvc_streaming *stream = video_to_stream(video);
	struct device *dev = stream->v4l2_dev.dev->parent;
#endif

	INIT_LIST_HEAD(&video->req_free);
	spin_lock_init(&video->req_lock);
	INIT_WORK(&video->pump, uvcg_video_pump);

	video->uvc = uvc;
	video->fcc = V4L2_PIX_FMT_YUYV;
	video->bpp = 16;
	video->width = 320;
	video->height = 240;
	video->imagesize = 320 * 240 * 2;

	/* Initialize the video buffers queue. */
	uvcg_queue_init(&video->queue, dev, V4L2_BUF_TYPE_VIDEO_OUTPUT,
			&video->mutex);
	return 0;
}
