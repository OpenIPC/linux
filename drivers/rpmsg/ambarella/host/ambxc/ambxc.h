#ifndef __AMBXC_H__
#define __AMBXC_H__

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/videobuf2-vmalloc.h>

#define AMBXC_NAME		"ambxc"

#define MEM2MEM_DEF_NUM_BUFS	VIDEO_MAX_FRAME
#define MEM2MEM_VID_MEM_LIMIT	(16 * 1024 * 1024)

#define dprintk(dev, fmt, arg...) \
	v4l2_dbg(1, 1, &dev->v4l2_dev, "%s: " fmt, __func__, ## arg)

#define BUF_SIZE (0x80000)

struct ambxc_fmt {
	char	*name;
	u32	fourcc;
	u32	types;
	int	field;
};

#define NUM_FORMATS ARRAY_SIZE(formats)

/* Per-queue, driver-specific private data */
struct ambxc_q_data {
	unsigned int		width;
	unsigned int		height;
	unsigned int		sizeimage;
	struct ambxc_fmt	*fmt;
};

enum {
	V4L2_M2M_SRC = 0,
	V4L2_M2M_DST = 1,
};

struct ambxc_dev {
	struct v4l2_device	v4l2_dev;
	struct video_device	*vfd;

	atomic_t		inst;
	atomic_t		ref_cnt;
	struct ambxc_ctx	*ctx;
	struct mutex		dev_mutex;
	spinlock_t		irqlock;
	struct task_struct	*kthread_capture;
	struct task_struct	*kthread_feed;
	wait_queue_head_t	queue_capture;
	wait_queue_head_t	queue_feed;

	struct v4l2_m2m_dev	*m2m_dev;
};

struct ambxc_ctx {
	struct v4l2_fh		fh;
	struct ambxc_dev	*dev;

	struct v4l2_ctrl_handler hdl;
	struct v4l2_ctrl *ctrls[128];

	struct v4l2_m2m_ctx	*m2m_ctx;

	/* Source and destination queue data */
	struct ambxc_q_data	q_data[2];
};

static inline struct ambxc_ctx* file2ctx(struct file *file)
{
	return container_of(file->private_data, struct ambxc_ctx, fh);
}

#endif /* __AMBXC_H__ */
