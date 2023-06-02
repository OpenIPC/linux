/*
 *	uac_v4l2.c  --  USB Video Class Gadget driver
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
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>

#include "uac_ex.h"
#include "uac_queue_ex.h"

static int
uac_v4l2_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	strlcpy(cap->driver, "g_uac", sizeof(cap->driver));
	strlcpy(cap->card, "g_uac", sizeof(cap->card));

	cap->capabilities = V4L2_CAP_AUDIO;

	return 0;
}
static int
uac_v4l2_get_format(struct file *file, void *fh, struct v4l2_format *fmt)
{
	return 0;
}

static int
uac_v4l2_set_format(struct file *file, void *fh, struct v4l2_format *fmt)
{
	return 0;
}

static int
uac_v4l2_reqbufs(struct file *file, void *fh, struct v4l2_requestbuffers *b)
{
	struct video_device *vdev = video_devdata(file);
	struct uac_device *uac = video_get_drvdata(vdev);

	if (b->type != uac->queue.queue.type)
		return -EINVAL;

	return uac_alloc_buffers(&uac->queue, b);
}

static int
uac_v4l2_querybuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vdev = video_devdata(file);
	struct uac_device *uac = video_get_drvdata(vdev);

	return uac_query_buffer(&uac->queue, b);
}

static int
uac_v4l2_qbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vdev = video_devdata(file);
	struct uac_device *uac = video_get_drvdata(vdev);

	return uac_queue_buffer(&uac->queue, b);
}

static int
uac_v4l2_dqbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vdev = video_devdata(file);
	struct uac_device *uac = video_get_drvdata(vdev);

	return uac_dequeue_buffer(&uac->queue, b, file->f_flags & O_NONBLOCK);
}

static int
uac_v4l2_streamon(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct video_device *vdev = video_devdata(file);
	struct uac_device *uac = video_get_drvdata(vdev);

	int ret;

	if (type != uac->queue.queue.type)
		return -EINVAL;

	ret = uac_queue_enable(&uac->queue, 1);
	if (ret < 0)
		return ret;

	return 0;
}

static int
uac_v4l2_streamoff(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct video_device *vdev = video_devdata(file);
	struct uac_device *uac = video_get_drvdata(vdev);

	if (type != uac->queue.queue.type)
		return -EINVAL;

	return uac_queue_enable(&uac->queue, 0);
}

static int
uac_v4l2_subscribe_event(struct v4l2_fh *fh,
			 const struct v4l2_event_subscription *sub)
{
	if (sub->type < UAC_EVENT_FIRST || sub->type > UAC_EVENT_LAST)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, 2, NULL);
}

static int
uac_v4l2_unsubscribe_event(struct v4l2_fh *fh,
			   const struct v4l2_event_subscription *sub)
{
	return v4l2_event_unsubscribe(fh, sub);
}

static long
uac_v4l2_ioctl_default(struct file *file, void *fh, bool valid_prio,
		unsigned int cmd, void *arg)
{
	switch (cmd) {
	default:
		return -ENOIOCTLCMD;
	}
}

const struct v4l2_ioctl_ops uac_v4l2_ioctl_ops = {
	.vidioc_querycap = uac_v4l2_querycap,
	.vidioc_reqbufs = uac_v4l2_reqbufs,
	.vidioc_querybuf = uac_v4l2_querybuf,
	.vidioc_g_fmt_vid_out = uac_v4l2_get_format,
	.vidioc_s_fmt_vid_out = uac_v4l2_set_format,
	.vidioc_qbuf = uac_v4l2_qbuf,
	.vidioc_dqbuf = uac_v4l2_dqbuf,
	.vidioc_streamon = uac_v4l2_streamon,
	.vidioc_streamoff = uac_v4l2_streamoff,
	.vidioc_subscribe_event = uac_v4l2_subscribe_event,
	.vidioc_unsubscribe_event = uac_v4l2_unsubscribe_event,
	.vidioc_default = uac_v4l2_ioctl_default,
};

/* --------------------------------------------------------------------------
 * V4L2
 */

static int
uac_v4l2_open(struct file *file)
{
	struct video_device *vdev = video_devdata(file);
	struct uac_device *uac_dev = video_get_drvdata(vdev);
	struct uac_file_handle *handle;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (handle == NULL)
		return -ENOMEM;

	v4l2_fh_init(&handle->vfh, vdev);
	v4l2_fh_add(&handle->vfh);

	handle->dev = uac_dev;
	file->private_data = &handle->vfh;

	return 0;
}

static int
uac_v4l2_release(struct file *file)
{
	struct uac_file_handle *handle = to_uac_file_handle(file->private_data);
	struct uac_device *uac_dev = handle->dev;

	uac_queue_enable(&uac_dev->queue, 0);
	uac_free_buffers(&uac_dev->queue);

	file->private_data = NULL;
	v4l2_fh_del(&handle->vfh);
	v4l2_fh_exit(&handle->vfh);
	kfree(handle);

	return 0;
}

static unsigned int
uac_v4l2_poll(struct file *file, poll_table *wait)
{
	struct video_device *vdev = video_devdata(file);
	struct uac_device *uac = video_get_drvdata(vdev);

	return uac_queue_poll(&uac->queue, file, wait);
}

struct v4l2_file_operations uac_v4l2_fops = {
	.owner		= THIS_MODULE,
	.open		= uac_v4l2_open,
	.release	= uac_v4l2_release,
	.ioctl		= video_ioctl2,
	.poll		= uac_v4l2_poll,
};
