/*
 * uvc_queue.h- Sigmastar
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

/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _UVC_QUEUE_H_
#define _UVC_QUEUE_H_

#include <linux/list.h>
#include <linux/poll.h>
#include <linux/spinlock.h>

#include <media/videobuf2-v4l2.h>

struct file;
struct mutex;

/* Maximum frame size in bytes, for sanity checking. */
#define UVC_MAX_FRAME_SIZE (16 * 1024 * 1024)
/* Maximum number of video buffers. */
#define UVC_MAX_VIDEO_BUFFERS 32

/* ------------------------------------------------------------------------
 * Structures.
 */

enum uvc_buffer_state {
	UVC_BUF_STATE_IDLE = 0,
	UVC_BUF_STATE_QUEUED = 1,
	UVC_BUF_STATE_ACTIVE = 2,
	UVC_BUF_STATE_DONE = 3,
	UVC_BUF_STATE_ERROR = 4,
};

struct uvc_buffer {
	struct vb2_v4l2_buffer buf;
	struct list_head queue;

	enum uvc_buffer_state state;
	void *mem;
	struct sg_table *sgt;
	struct scatterlist *sg;
	unsigned int offset;
	unsigned int length;
	unsigned int bytesused;

#define UVC_BUFFER_FLAGS_FRAME_NOEND (1 << 0)
#define UVC_BUFFER_FLAGS_STILL_IMAGE (1 << 1)
#define UVC_BUFFER_FLAGS_IR_ON (1 << 2)
#define UVC_BUFFER_FLAGS_FRAME_END(flags)                                      \
	((UVC_BUFFER_FLAGS_FRAME_NOEND & flags) == 0)
	__u32 reserved;
};

#define UVC_QUEUE_DISCONNECTED (1 << 0)
#define UVC_QUEUE_DROP_INCOMPLETE (1 << 1)
#define UVC_QUEUE_PAUSED (1 << 2)

struct uvc_video_queue {
	struct vb2_queue queue;

	unsigned int flags;
	__u32 sequence;

	unsigned int buf_used;

	bool use_sg;

	__u32 reserved;
#if defined(CONFIG_UVC_STREAM_ERR_SUPPORT)
	/* Use to inform host to drop the receiving frame(s) */
	__u8 bXferFlag;
#define FLAG_UVC_XFER_OK (0 << 0)
#define FLAG_UVC_XFER_ERR (1 << 0) // flag to indicate data loss in xfer
#define FLAG_UVC_EVENT_KEYFRAME (1 << 6) // flag to send forceIDR event
#define FLAG_UVC_WAIT_KEYFRAME                                                 \
	(1 << 7) // flag to keep drop frames unitl I-frame queue
#endif

	spinlock_t irqlock; /* Protects flags and irqqueue */
	struct list_head irqqueue;
};

static inline int uvc_queue_streaming(struct uvc_video_queue *queue)
{
	return vb2_is_streaming(&queue->queue);
}

int uvcg_queue_init(struct uvc_video_queue *queue, struct device *dev,
		    enum v4l2_buf_type type, struct mutex *lock);

void uvcg_free_buffers(struct uvc_video_queue *queue);

int uvcg_alloc_buffers(struct uvc_video_queue *queue,
		       struct v4l2_requestbuffers *rb);

int uvcg_query_buffer(struct uvc_video_queue *queue, struct v4l2_buffer *buf);

int uvcg_queue_buffer(struct uvc_video_queue *queue, struct v4l2_buffer *buf);

int uvcg_dequeue_buffer(struct uvc_video_queue *queue, struct v4l2_buffer *buf,
			int nonblocking);

__poll_t uvcg_queue_poll(struct uvc_video_queue *queue, struct file *file,
			 poll_table *wait);

int uvcg_queue_mmap(struct uvc_video_queue *queue, struct vm_area_struct *vma);

#ifndef CONFIG_MMU
unsigned long uvcg_queue_get_unmapped_area(struct uvc_video_queue *queue,
					   unsigned long pgoff);
#endif /* CONFIG_MMU */

void uvcg_queue_cancel(struct uvc_video_queue *queue, int disconnect);

int uvcg_queue_enable(struct uvc_video_queue *queue, int enable);

struct uvc_buffer *uvcg_queue_next_buffer(struct uvc_video_queue *queue,
					  struct uvc_buffer *buf);

struct uvc_buffer *uvcg_queue_head(struct uvc_video_queue *queue);

void uvcg_complete_sg(struct uvc_video_queue *queue);

struct uvc_buffer *uvcg_prepare_sg(struct uvc_video_queue *queue);

#endif /* _UVC_QUEUE_H_ */
