#ifndef _UAC_QUEUE_H_
#define _UAC_QUEUE_H_

#ifdef __KERNEL__

#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/videodev2.h>
#include <media/videobuf2-core.h>

/* Maximum frame size in bytes, for sanity checking. */
#define UVC_MAX_FRAME_SIZE	(16*1024*1024)
/* Maximum number of video buffers. */
#define UVC_MAX_VIDEO_BUFFERS	32

/* ------------------------------------------------------------------------
 * Structures.
 */

enum uac_buffer_state {
	UVC_BUF_STATE_IDLE	= 0,
	UVC_BUF_STATE_QUEUED	= 1,
	UVC_BUF_STATE_ACTIVE	= 2,
	UVC_BUF_STATE_DONE	= 3,
	UVC_BUF_STATE_ERROR	= 4,
};

struct uac_buffer {
	struct vb2_buffer buf;
	struct list_head queue;

	enum uac_buffer_state state;
	void *mem;
	unsigned int length;
	unsigned int bytesused;
};

#define UVC_QUEUE_DISCONNECTED		(1 << 0)
#define UVC_QUEUE_DROP_INCOMPLETE	(1 << 1)
#define UVC_QUEUE_PAUSED		(1 << 2)

struct uac_queue {
	struct vb2_queue queue;
	struct mutex mutex;	/* Protects queue */

	unsigned int flags;
	__u32 sequence;

	unsigned int buf_used;

	spinlock_t irqlock;	/* Protects flags and irqqueue */
	struct list_head irqqueue;
};

static inline int uac_queue_streaming(struct uac_queue *queue)
{
	return vb2_is_streaming(&queue->queue);
}

int uac_queue_init(struct uac_queue *queue);

void uac_free_buffers(struct uac_queue *queue);

int uac_alloc_buffers(struct uac_queue *queue,
		       struct v4l2_requestbuffers *rb);

int uac_query_buffer(struct uac_queue *queue, struct v4l2_buffer *buf);

int uac_queue_buffer(struct uac_queue *queue, struct v4l2_buffer *buf);

int uac_dequeue_buffer(struct uac_queue *queue,
			struct v4l2_buffer *buf, int nonblocking);

unsigned int uac_queue_poll(struct uac_queue *queue,
			     struct file *file, poll_table *wait);


void uac_queue_cancel(struct uac_queue *queue, int disconnect);

int uac_queue_enable(struct uac_queue *queue, int enable);

struct uac_buffer *uac_queue_next_buffer(struct uac_queue *queue,
					  struct uac_buffer *buf);

struct uac_buffer *uac_queue_head(struct uac_queue *queue);

#endif /* __KERNEL__ */

#endif /* _UAC_QUEUE_H_ */

