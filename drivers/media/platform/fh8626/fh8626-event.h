/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _FH8626_EVENT_H
#define _FH8626_EVENT_H

#include <media/v4l2-event.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>

/* An open file retains its node and event queue throughout this wait. */
static long fh8626_dequeue_event(struct file *file, unsigned int cmd, void *arg)
{
	struct video_device *vd = video_devdata(file);
	struct v4l2_fh *fh = file->private_data;
	int ret;

	for (;;) {
		if (!video_is_registered(vd))
			return -ENODEV;
		ret = v4l2_event_dequeue(fh, arg, true);
		if (ret != -ENOENT || (file->f_flags & O_NONBLOCK))
			return ret;
		ret = wait_event_interruptible(fh->wait,
			!video_is_registered(vd) || v4l2_event_pending(fh));
		if (ret)
			return ret;
	}
}

static inline long fh8626_video_ioctl2(struct file *file, unsigned int cmd,
				      unsigned long arg)
{
	if (cmd == VIDIOC_DQEVENT)
		return video_usercopy(file, cmd, arg, fh8626_dequeue_event);
	return video_ioctl2(file, cmd, arg);
}

static inline void fh8626_video_wake_events(struct video_device *vd)
{
	struct v4l2_fh *fh;
	unsigned long flags;

	spin_lock_irqsave(&vd->fh_lock, flags);
	list_for_each_entry(fh, &vd->fh_list, list)
		wake_up_all(&fh->wait);
	spin_unlock_irqrestore(&vd->fh_lock, flags);
}

#endif
