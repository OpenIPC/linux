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
#include <linux/jiffies.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/video.h>
#include <asm/unaligned.h>
#include <media/v4l2-dev.h>

#include "uvc.h"
#include "uvc_queue.h"
#include "uvc_video.h"
#ifdef CONFIG_ARCH_AXERA
#define HEADER_SIZE 32
#define HEADER_MAGIC_NUM 123
#define HEADER_DUPLICATE 1
#define HEADER_UVC_STANDARD_SIZE 12
#endif
/* --------------------------------------------------------------------------
 * Video codecs
 */

#ifdef CONFIG_ARCH_AXERA
static u32 axera_uvc_fnv1a32(const u8 *data, size_t size)
{
	u32 hash = 2166136261U;
	size_t index;

	for (index = 0; index < size; ++index) {
		hash ^= data[index];
		hash *= 16777619U;
	}

	return hash;
}

static bool axera_uvc_transport_metadata_valid(const u8 *metadata)
{
	return metadata[0] == HEADER_MAGIC_NUM &&
		metadata[2] == 1 &&
		metadata[3] == HEADER_SIZE &&
		get_unaligned_le32(metadata + 16) ==
		axera_uvc_fnv1a32(metadata, 16);
}

static u32 axera_uvc_clock_48mhz(void)
{
	u64 nanoseconds = ktime_get_ns();
	u64 ticks = div_u64(nanoseconds, 1000) * 48;

	ticks += div_u64((nanoseconds % 1000) * 48, 1000);
	return (u32)ticks;
}
#endif

static int
uvc_video_encode_header(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len)
{
#ifdef CONFIG_ARCH_AXERA
	const u8 *transport = buf->mem;
	bool metadata_valid = axera_uvc_transport_metadata_valid(transport);
	bool first_payload = video->queue.buf_used == 0;
	int header_size = metadata_valid ? HEADER_UVC_STANDARD_SIZE : 2;

	if (len < header_size)
		return 0;

	data[0] = header_size;
	data[1] = UVC_STREAM_EOH | video->fid;
	if (metadata_valid) {
		struct uvc_device *uvc = container_of(video,
			struct uvc_device, video);
		int sof;

		if (first_payload) {
			video->metadata_stc = axera_uvc_clock_48mhz();
			sof = usb_gadget_frame_number(
				uvc->func.config->cdev->gadget);
			video->metadata_sof = sof < 0 ? 0 : sof & 0x7ff;
		}
		data[1] |= UVC_STREAM_PTS | UVC_STREAM_SCR;
		put_unaligned_le32(get_unaligned_le32(transport + 4), data + 2);
		put_unaligned_le32(video->metadata_stc, data + 6);
		put_unaligned_le16(video->metadata_sof, data + 10);
	}

	if((HEADER_MAGIC_NUM == *(char *)buf->mem) && (HEADER_DUPLICATE == *((char *)buf->mem + 1))
		&& usb_endpoint_xfer_bulk(video->ep->desc) ) {
		return header_size;
	} else {
		if (buf->bytesused - video->queue.buf_used <= len - header_size)
			data[1] |= UVC_STREAM_EOF;
	}
	return header_size;
#else
	data[0] = 2;
	data[1] = UVC_STREAM_EOH | video->fid;
	if (buf->bytesused - video->queue.buf_used <= len - 2)
		data[1] |= UVC_STREAM_EOF;
	return 2;
#endif
}

static int
uvc_video_encode_data(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len)
{
	struct uvc_video_queue *queue = &video->queue;
	unsigned int nbytes;

	void *mem;

#ifdef CONFIG_ARCH_AXERA
	void *tmp_buf;

	if(HEADER_MAGIC_NUM == *(char*)buf->mem){
		tmp_buf = (char*)buf->mem + HEADER_SIZE;
	}else{
		tmp_buf = (char*)buf->mem;
	}

	mem = tmp_buf + queue->buf_used;
#else
	mem = buf->mem + queue->buf_used;
#endif
	nbytes = min((unsigned int)len, buf->bytesused - queue->buf_used);

	memcpy(data, mem, nbytes);
	queue->buf_used += nbytes;

	return nbytes;
}

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

	/* Process video data. */
	len = min((int)(video->max_payload_size - video->payload_size), len);

#ifdef CONFIG_ARCH_AXERA
	if((HEADER_MAGIC_NUM == *(char *)buf->mem) && (HEADER_DUPLICATE == *((char *)buf->mem + 1))){
		video->queue.buf_used = 0;
		buf->bytesused = 0;
		ret = 0;
	} else {
		ret = uvc_video_encode_data(video, buf, mem, len);
	}
#else
	ret = uvc_video_encode_data(video, buf, mem, len);
#endif

	video->payload_size += ret;
	len -= ret;

	req->length = video->req_size - len;
	req->zero = video->payload_size == video->max_payload_size;

	if (buf->bytesused == video->queue.buf_used) {
		video->queue.buf_used = 0;
		buf->state = UVC_BUF_STATE_DONE;
		uvcg_queue_next_buffer(&video->queue, buf);
		video->fid ^= UVC_STREAM_FID;

		video->payload_size = 0;
		req->zero = 1;
	}

	if (video->payload_size == video->max_payload_size ||
	    buf->bytesused == video->queue.buf_used)
		video->payload_size = 0;
}

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

#ifdef CONFIG_ARCH_AXERA
	if((HEADER_MAGIC_NUM == *(char *)buf->mem) && (HEADER_DUPLICATE == *((char *)buf->mem + 1))){
		video->queue.buf_used = ret;
		len = video->req_size;
	} else {
		ret = uvc_video_encode_data(video, buf, mem, len);
		len -= ret;
	}
#else
	ret = uvc_video_encode_data(video, buf, mem, len);
	len -= ret;
#endif

	req->length = video->req_size - len;

	if (buf->bytesused == video->queue.buf_used) {
		video->queue.buf_used = 0;
		buf->state = UVC_BUF_STATE_DONE;
		uvcg_queue_next_buffer(&video->queue, buf);
		video->fid ^= UVC_STREAM_FID;
	}
}

/* --------------------------------------------------------------------------
 * Request handling
 */

static int uvcg_video_ep_queue(struct uvc_video *video, struct usb_request *req)
{
	int ret;

	if (!READ_ONCE(video->is_enabled))
		return -ESHUTDOWN;

	atomic_inc(&video->queued);
	ret = usb_ep_queue(video->ep, req, GFP_ATOMIC);
	if (ret < 0) {
		atomic_dec(&video->queued);
		printk(KERN_INFO "Failed to queue request (%d).\n", ret);
		/* Isochronous endpoints can't be halted. */
#ifdef CONFIG_ARCH_AXERA
		if (video->ep->desc != NULL && usb_endpoint_xfer_bulk(video->ep->desc))
#else
		if (usb_endpoint_xfer_bulk(video->ep->desc))
#endif
			usb_ep_set_halt(video->ep);
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
	struct uvc_buffer *buf;
	unsigned long flags;
	bool enabled;
	int ret;

	/*
	 * Keep the callback alive in the disable wait condition. queued alone
	 * is insufficient because it is decremented before the callback has
	 * finished touching req and video.
	 */
	atomic_inc(&video->callbacks);
	spin_lock_irqsave(&video->req_lock, flags);
	if (WARN_ON_ONCE(atomic_read(&video->queued) == 0))
		enabled = false;
	else {
		atomic_dec(&video->queued);
		enabled = READ_ONCE(video->is_enabled);
	}
	spin_unlock_irqrestore(&video->req_lock, flags);
	wake_up_all(&video->req_wait);

	if (!enabled)
		goto requeue;

	switch (req->status) {
	case 0:
		break;

	case -ESHUTDOWN:	/* disconnect from host. */
		//printk(KERN_DEBUG "VS request cancelled.\n");
		uvcg_queue_cancel(queue, 1);
		goto requeue;

	default:
		//printk(KERN_DEBUG "VS request completed with status %d.\n",
		//	req->status);
		uvcg_queue_cancel(queue, 0);
		goto requeue;
	}

	spin_lock_irqsave(&video->queue.irqlock, flags);
	buf = uvcg_queue_head(&video->queue);
	if (buf == NULL) {
		spin_unlock_irqrestore(&video->queue.irqlock, flags);
		goto requeue;
	}

	video->encode(req, video, buf);

	ret = uvcg_video_ep_queue(video, req);
	spin_unlock_irqrestore(&video->queue.irqlock, flags);

	if (ret < 0) {
		uvcg_queue_cancel(queue, 0);
		goto requeue;
	}

	goto done;

requeue:
	spin_lock_irqsave(&video->req_lock, flags);

	list_add_tail(&req->list, &video->req_free);
	spin_unlock_irqrestore(&video->req_lock, flags);

done:
	atomic_dec(&video->callbacks);
	wake_up_all(&video->req_wait);
}

static int
uvc_video_free_requests(struct uvc_video *video)
{
	unsigned int i;

	for (i = 0; i < UVC_NUM_REQUESTS; ++i) {
		if (video->req[i]) {
			usb_ep_free_request(video->ep, video->req[i]);
			video->req[i] = NULL;
		}

		if (video->req_buffer[i]) {
			kfree(video->req_buffer[i]);
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
	int ret = -ENOMEM;
	struct uvc_device *uvc;

	uvc = container_of(video, struct uvc_device, video);
	BUG_ON(video->req_size);

	if (!usb_endpoint_xfer_bulk(video->ep->desc)) {
		req_size = video->ep->maxpacket
			 * max_t(unsigned int, video->ep->maxburst, 1)
			 * (video->ep->mult);
	} else {
		req_size = video->ep->maxpacket
			 * max_t(unsigned int, video->ep->maxburst, 1);
	}

	for (i = 0; i < UVC_NUM_REQUESTS; ++i) {
		video->req_buffer[i] = kmalloc(req_size, GFP_KERNEL);
		if (video->req_buffer[i] == NULL)
			goto error;

		video->req[i] = usb_ep_alloc_request(video->ep, GFP_KERNEL);
		if (video->req[i] == NULL)
			goto error;

		video->req[i]->buf = video->req_buffer[i];
		video->req[i]->length = 0;
		video->req[i]->complete = uvc_video_complete;
		video->req[i]->context = video;

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
int uvcg_video_pump(struct uvc_video *video)
{
	struct uvc_video_queue *queue = &video->queue;
	struct usb_request *req;
	struct uvc_buffer *buf;
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
		if (!READ_ONCE(video->is_enabled) || list_empty(&video->req_free)) {
			spin_unlock_irqrestore(&video->req_lock, flags);
			return 0;
		}
		req = list_first_entry(&video->req_free, struct usb_request,
					list);
		list_del(&req->list);
		spin_unlock_irqrestore(&video->req_lock, flags);

		/* Retrieve the first available video buffer and fill the
		 * request, protected by the video queue irqlock.
		 */
		spin_lock_irqsave(&queue->irqlock, flags);
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
	return 0;
}

/*
 * Enable or disable the video stream.
 */
int uvcg_video_enable(struct uvc_video *video, int enable)
{
	unsigned long flags;
	int ret;

	if (video->ep == NULL) {
		printk(KERN_INFO "Video enable failed, device is "
			"uninitialized.\n");
		return -ENODEV;
	}

	if (!enable)
		return uvcg_video_stop(video);

	spin_lock_irqsave(&video->req_lock, flags);
	if (video->req_size || atomic_read(&video->queued) != 0 ||
	    atomic_read(&video->callbacks) != 0) {
		spin_unlock_irqrestore(&video->req_lock, flags);
		return -EBUSY;
	}
	WRITE_ONCE(video->is_enabled, true);
	spin_unlock_irqrestore(&video->req_lock, flags);

	if ((ret = uvcg_queue_enable(&video->queue, 1)) < 0) {
		spin_lock_irqsave(&video->req_lock, flags);
		WRITE_ONCE(video->is_enabled, false);
		spin_unlock_irqrestore(&video->req_lock, flags);
		return ret;
	}

	if ((ret = uvc_video_alloc_requests(video)) < 0) {
		spin_lock_irqsave(&video->req_lock, flags);
		WRITE_ONCE(video->is_enabled, false);
		spin_unlock_irqrestore(&video->req_lock, flags);
		uvcg_queue_enable(&video->queue, 0);
		return ret;
	}

	if (video->max_payload_size) {
#ifdef CONFIG_ARCH_AXERA
		video->max_payload_size = video->imagesize;
#endif
		video->encode = uvc_video_encode_bulk;
		video->payload_size = 0;
	} else
		video->encode = uvc_video_encode_isoc;

	return uvcg_video_pump(video);
}

/* Stop requests from process context, then disable the endpoint. */
int uvcg_video_stop(struct uvc_video *video)
{
	long timeout;
	int ret = 0;

	if (!video->ep)
		return -ENODEV;

	WRITE_ONCE(video->is_enabled, false);
#ifdef CONFIG_ARCH_AXERA
	uvcg_queue_cancel(&video->queue, 0);
#endif
	/*
	 * DWC3 endpoint disable issues one END_TRANSFER and gives back every
	 * request on the endpoint. Dequeuing requests one by one would issue
	 * repeated END_TRANSFER commands for the same transfer resource.
	 */
	if (video->ep->enabled)
		ret = usb_ep_disable(video->ep);
	if (ret < 0)
		return ret;

	timeout = wait_event_timeout(video->req_wait,
		atomic_read(&video->queued) == 0 &&
		atomic_read(&video->callbacks) == 0,
		msecs_to_jiffies(2000));
	if (!timeout) {
		printk(KERN_ERR "UVC endpoint stop drain timed out: queued=%d callbacks=%d\n",
			atomic_read(&video->queued), atomic_read(&video->callbacks));
		return -ETIMEDOUT;
	}

	uvc_video_free_requests(video);
	uvcg_queue_enable(&video->queue, 0);
	return 0;
}
#ifdef CONFIG_UVC_H264
/*
 * Initialize the UVC video stream.
 */
int uvcg_video_init(struct uvc_video *video, struct uvc_device *uvc)
{
	INIT_LIST_HEAD(&video->req_free);
	spin_lock_init(&video->req_lock);
	WRITE_ONCE(video->is_enabled, false);
	atomic_set(&video->queued, 0);
	atomic_set(&video->callbacks, 0);
	init_waitqueue_head(&video->req_wait);

	video->fcc = V4L2_PIX_FMT_YUYV;
	video->bpp = 16;
	video->width = 320;
	video->height = 240;
	video->imagesize = 320 * 240 * 2;
	video->uvc = uvc;

	/* Initialize the video buffers queue. */
	uvcg_queue_init(&video->queue, V4L2_BUF_TYPE_VIDEO_OUTPUT,
			&video->mutex);
	return 0;
}
#else
/*
 * Initialize the UVC video stream.
 */
int uvcg_video_init(struct uvc_video *video)
{
	INIT_LIST_HEAD(&video->req_free);
	spin_lock_init(&video->req_lock);
	WRITE_ONCE(video->is_enabled, false);
	atomic_set(&video->queued, 0);
	atomic_set(&video->callbacks, 0);
	init_waitqueue_head(&video->req_wait);

	video->fcc = V4L2_PIX_FMT_YUYV;
	video->bpp = 16;
	video->width = 320;
	video->height = 240;
	video->imagesize = 320 * 240 * 2;

	/* Initialize the video buffers queue. */
	uvcg_queue_init(&video->queue, V4L2_BUF_TYPE_VIDEO_OUTPUT,
			&video->mutex);
	return 0;
}
#endif
