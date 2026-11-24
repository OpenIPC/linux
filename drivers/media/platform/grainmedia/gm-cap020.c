// SPDX-License-Identifier: GPL-2.0-only
/*
 * Grain Media GM8126 video capture unit (FTCAP020), path 1, packed 4:2:2.
 *
 * Each frame is described by a list of words in memory: the status the unit
 * writes, DMA addresses, register updates and the next list. The unit takes
 * the list its head register names at each frame start and moves on to the
 * next. There is no public documentation; the registers and the order they
 * are written in were observed on a camera's own driver.
 *
 * A list whose path is off and which names itself keeps the chain alive.
 * There are two of these: buffers are entered from the one the chain rests
 * on, and the last buffer leads to the other one, so the entry can be
 * closed again before the chain comes back to it.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-contig.h>

#define CAP_CTRL		0x000
#define   CTRL_BASE		0x10328000
#define   CTRL_ENABLE		BIT(31)
#define CAP_PATHS		0x004
#define CAP_RESET		0x008
#define   RESET_GO		BIT(28)
#define CAP_SCALED0		0x010
#define CAP_CROP_POS0		0x014
#define CAP_CROP0		0x018
#define CAP_SCALED1		0x020
#define CAP_CROP_POS1		0x024
#define CAP_CROP1		0x028
#define CAP_EFFECT1		0x034
#define CAP_FORMAT		0x038
#define CAP_DMA_FORMAT		0x03c
#define CAP_PATH_OFF		0x040
#define   PATH1_OFF		0x001f0000
#define CAP_POLARITY		0x044
#define CAP_IN_SIZE		0x050
#define CAP_ACT_POS0		0x054
#define CAP_ACT_SIZE0		0x058
#define CAP_ACT_POS1		0x064
#define CAP_ACT_SIZE1		0x068
#define CAP_SKIP1		0x204
#define CAP_PITCH1		0x21c
#define CAP_LIST0		0x280
#define CAP_LIST1		0x284
#define CAP_INT			0x3e0
#define CAP_MASK		0x3e4
#define   MASK_USED		0xfbfefbef	/* frame start and errors */
#define   INT_ERRORS		(BIT(10) | BIT(16) | BIT(26))

#define LIST_DONE		BIT(31)
#define LIST_REG(be, off)	((be) << 16 | (off))
#define LIST_DMA(addr)		(BIT(31) | (addr) >> 2)
#define LIST_NEXT(addr)		(BIT(31) | BIT(30) | (addr) >> 2)

#define LIST_WORDS		32
#define LIST_NEXT_WORD		26
#define LIST_BYTES		(LIST_WORDS * sizeof(u32))

#define CAP_MIN_W		32
#define CAP_MIN_H		32
#define CAP_IN_W		640
#define CAP_IN_H		480
#define CAP_BUFS		VB2_MAX_FRAME
/* three idle lists before the buffers' */
#define LIST_REST0		0
#define LIST_REST1		1
#define LIST_REST_PATH0		2
#define LIST_BUFS		3

struct cap_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head node;
	unsigned int list;
};

struct cap_dev {
	struct device *dev;
	void __iomem *base;
	struct v4l2_device v4l2;
	struct v4l2_async_notifier notifier;
	struct v4l2_subdev *sensor;
	struct video_device vdev;
	struct vb2_queue queue;
	struct mutex lock;
	struct clk *clk;
	struct v4l2_pix_format pix;

	/* the lists and what is in them */
	spinlock_t irqlock;
	__le32 *lists;
	dma_addr_t lists_dma;
	struct list_head linked;	/* buffers the chain has */
	struct list_head waiting;	/* queued, not linked yet */
	unsigned int rest;		/* the idle list the chain rests on */
	bool entered;			/* it leads to the first linked one */
	bool stalled;			/* at rest with buffers linked */
	u32 sequence;
};

static __le32 *cap_list(struct cap_dev *cap, unsigned int n)
{
	return cap->lists + n * LIST_WORDS;
}

static u32 cap_list_dma(struct cap_dev *cap, unsigned int n)
{
	return cap->lists_dma + n * LIST_BYTES;
}

static void cap_set_next(struct cap_dev *cap, unsigned int n, unsigned int next)
{
	/* what it leads to must be complete first */
	dma_wmb();
	WRITE_ONCE(cap_list(cap, n)[LIST_NEXT_WORD],
		   cpu_to_le32(LIST_NEXT(cap_list_dma(cap, next))));
}

static unsigned int cap_put(__le32 *l, unsigned int i, u32 be, u32 off, u32 val)
{
	l[i++] = cpu_to_le32(LIST_REG(be, off));
	l[i++] = cpu_to_le32(val);
	return i;
}

/* Fill up with harmless updates to where the next pointer is. */
static void cap_pad(__le32 *l, unsigned int i, unsigned int path)
{
	while (i < LIST_NEXT_WORD)
		i = cap_put(l, i, 0xf, path ? CAP_CROP_POS1 : CAP_CROP_POS0, 0);
}

static void cap_build_rest(struct cap_dev *cap, unsigned int n, unsigned int path)
{
	__le32 *l = cap_list(cap, n);
	unsigned int i;

	l[0] = 0;
	/* the path is off: no address is used */
	for (i = 1; i <= 3; i++)
		l[i] = cpu_to_le32(LIST_DMA(cap->lists_dma));
	i = cap_put(l, 4, path ? 0xc : 0x3, CAP_PATH_OFF,
		    path ? PATH1_OFF : 0x1f);
	i = cap_put(l, i, 0x1, CAP_PATHS, 0x1);
	i = cap_put(l, i, 0xf, path ? CAP_CROP_POS1 : CAP_CROP_POS0, 0);
	cap_pad(l, i, path);
	cap_set_next(cap, n, n);
}

static void cap_build_frame(struct cap_dev *cap, struct cap_buffer *buf)
{
	const u32 size = cap->pix.height << 16 | cap->pix.width;
	dma_addr_t addr = vb2_dma_contig_plane_dma_addr(&buf->vb.vb2_buf, 0);
	__le32 *l = cap_list(cap, buf->list);
	unsigned int i;

	l[0] = 0;
	for (i = 1; i <= 3; i++)	/* packed: the first is the one used */
		l[i] = cpu_to_le32(LIST_DMA((u32)addr));
	i = cap_put(l, 4, 0xf, CAP_PITCH1, cap->pix.bytesperline / 4);
	i = cap_put(l, i, 0x2, CAP_DMA_FORMAT, 0x4140);
	i = cap_put(l, i, 0xc, CAP_PATH_OFF, 0);
	i = cap_put(l, i, 0xf, CAP_ACT_POS1, 0);
	i = cap_put(l, i, 0xf, CAP_ACT_SIZE1, CAP_IN_H << 16 | CAP_IN_W);
	i = cap_put(l, i, 0xf, CAP_SCALED1, size);
	i = cap_put(l, i, 0xf, CAP_CROP_POS1, 0);
	i = cap_put(l, i, 0xf, CAP_CROP1, size);
	i = cap_put(l, i, 0x2, CAP_FORMAT, 0x500);	/* packed 4:2:2 */
	i = cap_put(l, i, 0xf, CAP_SKIP1, 1);
	i = cap_put(l, i, 0x3, CAP_PATHS, 0x1003);
	cap_pad(l, i, 1);
}

/* Append to the chain; it is entered at the next frame start if at rest. */
static void cap_link(struct cap_dev *cap, struct cap_buffer *buf)
{
	struct cap_buffer *last;

	cap_build_frame(cap, buf);
	cap_set_next(cap, buf->list, cap->rest ^ 1);
	if (!list_empty(&cap->linked)) {
		last = list_last_entry(&cap->linked, struct cap_buffer, node);
		cap_set_next(cap, last->list, buf->list);
	}
	list_add_tail(&buf->node, &cap->linked);
}

/* Called with the interrupt lock at each frame start. */
static void cap_frame(struct cap_dev *cap)
{
	const u32 head = readl(cap->base + CAP_LIST1) << 2;
	struct cap_buffer *buf, *tmp;
	bool first_done = false;

	list_for_each_entry_safe(buf, tmp, &cap->linked, node) {
		__le32 status = READ_ONCE(cap_list(cap, buf->list)[0]);

		if (!(le32_to_cpu(status) & LIST_DONE))
			break;
		dma_rmb();
		first_done = true;
		list_del(&buf->node);
		buf->vb.sequence = cap->sequence;
		buf->vb.field = V4L2_FIELD_NONE;
		buf->vb.vb2_buf.timestamp = ktime_get_ns();
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);
	}
	cap->sequence++;

	/* The chain has left its resting list: close the entry. */
	if (first_done && cap->entered) {
		cap_set_next(cap, cap->rest, cap->rest);
		cap_list(cap, cap->rest)[0] = 0;
		cap->rest ^= 1;
		cap->entered = false;
	}
	/* A resting list's status has to be collected for it to run again. */
	cap_list(cap, LIST_REST0)[0] = 0;
	cap_list(cap, LIST_REST1)[0] = 0;
	cap_list(cap, LIST_REST_PATH0)[0] = 0;

	/*
	 * Buffers are appended only while the chain is known not to be at its
	 * end, or at rest; one frame of slack is kept at the end because the
	 * unit may have read the next pointer already.
	 */
	if (cap->entered)
		return;
	if (head == cap_list_dma(cap, cap->rest)) {
		/*
		 * At rest, or about to be: the head may already be past the
		 * list being captured. Buffers linked too late for the chain
		 * to see are still there a frame later, and start again.
		 */
		if (!list_empty(&cap->linked) && !cap->stalled) {
			cap->stalled = true;
			return;
		}
		cap->stalled = false;
		list_splice_init(&cap->linked, &cap->waiting);
		if (list_empty(&cap->waiting))
			return;
		while (!list_empty(&cap->waiting)) {
			buf = list_first_entry(&cap->waiting, struct cap_buffer,
					       node);
			list_del(&buf->node);
			cap_link(cap, buf);
		}
		buf = list_first_entry(&cap->linked, struct cap_buffer, node);
		cap_set_next(cap, cap->rest, buf->list);
		cap->entered = true;
	} else if (!list_empty(&cap->linked) &&
		   !list_is_singular(&cap->linked)) {
		/* the last one is at least a frame away */
		struct cap_buffer *last;

		cap->stalled = false;
		last = list_last_entry(&cap->linked, struct cap_buffer, node);
		if (head == cap_list_dma(cap, last->list))
			return;
		while (!list_empty(&cap->waiting)) {
			buf = list_first_entry(&cap->waiting, struct cap_buffer,
					       node);
			list_del(&buf->node);
			cap_build_frame(cap, buf);
			cap_set_next(cap, buf->list, cap->rest);
			last = list_last_entry(&cap->linked, struct cap_buffer,
					       node);
			cap_set_next(cap, last->list, buf->list);
			list_add_tail(&buf->node, &cap->linked);
		}
	}
}

static irqreturn_t cap_irq(int irq, void *data)
{
	struct cap_dev *cap = data;
	u32 status = readl(cap->base + CAP_INT) & ~MASK_USED;

	if (!status)
		return IRQ_NONE;
	writel(status, cap->base + CAP_INT);
	if (status & INT_ERRORS)
		dev_dbg_ratelimited(cap->dev, "status %08x\n", status);
	if (status & BIT(4)) {
		spin_lock(&cap->irqlock);
		cap_frame(cap);
		spin_unlock(&cap->irqlock);
	}
	return IRQ_HANDLED;
}

/* ---- vb2 ---------------------------------------------------------------- */

static int cap_queue_setup(struct vb2_queue *q, unsigned int *nbufs,
			   unsigned int *nplanes, unsigned int sizes[],
			   struct device *alloc_devs[])
{
	struct cap_dev *cap = vb2_get_drv_priv(q);

	if (*nplanes)
		return sizes[0] < cap->pix.sizeimage ? -EINVAL : 0;
	*nplanes = 1;
	sizes[0] = cap->pix.sizeimage;
	return 0;
}

static int cap_buf_prepare(struct vb2_buffer *vb)
{
	struct cap_dev *cap = vb2_get_drv_priv(vb->vb2_queue);

	if (vb2_plane_size(vb, 0) < cap->pix.sizeimage)
		return -EINVAL;
	vb2_set_plane_payload(vb, 0, cap->pix.sizeimage);
	return 0;
}

static void cap_buf_queue(struct vb2_buffer *vb)
{
	struct cap_dev *cap = vb2_get_drv_priv(vb->vb2_queue);
	struct cap_buffer *buf = container_of(to_vb2_v4l2_buffer(vb),
					      struct cap_buffer, vb);
	unsigned long flags;

	buf->list = LIST_BUFS + vb->index;
	spin_lock_irqsave(&cap->irqlock, flags);
	list_add_tail(&buf->node, &cap->waiting);
	spin_unlock_irqrestore(&cap->irqlock, flags);
}

static void cap_return_all(struct cap_dev *cap, enum vb2_buffer_state state)
{
	struct cap_buffer *buf, *tmp;
	unsigned long flags;

	spin_lock_irqsave(&cap->irqlock, flags);
	list_splice_tail_init(&cap->waiting, &cap->linked);
	list_for_each_entry_safe(buf, tmp, &cap->linked, node) {
		list_del(&buf->node);
		vb2_buffer_done(&buf->vb.vb2_buf, state);
	}
	spin_unlock_irqrestore(&cap->irqlock, flags);
}

static int cap_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct cap_dev *cap = vb2_get_drv_priv(q);
	const u32 in = CAP_IN_H << 16 | CAP_IN_W;
	const u32 size = cap->pix.height << 16 | cap->pix.width;
	int ret;

	ret = clk_prepare_enable(cap->clk);
	if (ret)
		goto err_buffers;

	cap->sequence = 0;
	cap->rest = LIST_REST0;
	cap->entered = false;
	cap->stalled = false;
	cap_build_rest(cap, LIST_REST0, 1);
	cap_build_rest(cap, LIST_REST1, 1);
	cap_build_rest(cap, LIST_REST_PATH0, 0);

	writel(RESET_GO, cap->base + CAP_RESET);
	writel(CTRL_BASE, cap->base + CAP_CTRL);
	writel(0, cap->base + CAP_DMA_FORMAT);
	writel(0xffffffff, cap->base + CAP_INT);
	writel(0x001f001f, cap->base + CAP_PATH_OFF);
	writel(0x1000, cap->base + CAP_PATHS);
	writel(0x00080800, cap->base + CAP_POLARITY);
	writel(in, cap->base + CAP_IN_SIZE);
	writel(0, cap->base + CAP_RESET);
	writel(0x100, cap->base + CAP_DMA_FORMAT);
	writel(0, cap->base + CAP_ACT_POS0);
	writel(in, cap->base + CAP_ACT_SIZE0);
	writel(in, cap->base + CAP_SCALED0);
	writel(0, cap->base + CAP_CROP_POS0);
	writel(in, cap->base + CAP_CROP0);
	writel(0, cap->base + CAP_ACT_POS1);
	writel(in, cap->base + CAP_ACT_SIZE1);
	writel(0, cap->base + CAP_CROP_POS1);
	writel(0x4140, cap->base + CAP_DMA_FORMAT);
	writel(size, cap->base + CAP_SCALED1);
	writel(size, cap->base + CAP_CROP1);
	writel(0x500, cap->base + CAP_FORMAT);
	writel(1, cap->base + CAP_SKIP1);
	writel(cap_list_dma(cap, LIST_REST_PATH0) >> 2, cap->base + CAP_LIST0);
	writel(cap_list_dma(cap, LIST_REST0) >> 2, cap->base + CAP_LIST1);
	writel(0x1100, cap->base + CAP_PATHS);

	ret = v4l2_subdev_call(cap->sensor, video, s_stream, 1);
	if (ret && ret != -ENOIOCTLCMD)
		goto err_clk;

	writel(MASK_USED, cap->base + CAP_MASK);
	writel(CTRL_ENABLE | CTRL_BASE, cap->base + CAP_CTRL);
	return 0;

err_clk:
	clk_disable_unprepare(cap->clk);
err_buffers:
	cap_return_all(cap, VB2_BUF_STATE_QUEUED);
	return ret;
}

static void cap_stop_streaming(struct vb2_queue *q)
{
	struct cap_dev *cap = vb2_get_drv_priv(q);

	writel(0xffffffff, cap->base + CAP_MASK);
	writel(CTRL_BASE, cap->base + CAP_CTRL);
	synchronize_irq(platform_get_irq(to_platform_device(cap->dev), 0));
	v4l2_subdev_call(cap->sensor, video, s_stream, 0);
	/* the frame under way is over before the buffers go back */
	msleep(100);
	writel(RESET_GO, cap->base + CAP_RESET);
	clk_disable_unprepare(cap->clk);
	cap_return_all(cap, VB2_BUF_STATE_ERROR);
}

static const struct vb2_ops cap_vb2_ops = {
	.queue_setup = cap_queue_setup,
	.buf_prepare = cap_buf_prepare,
	.buf_queue = cap_buf_queue,
	.start_streaming = cap_start_streaming,
	.stop_streaming = cap_stop_streaming,
};

/* ---- ioctls --------------------------------------------------------------- */

static int cap_querycap(struct file *file, void *priv,
			struct v4l2_capability *c)
{
	strscpy(c->driver, "gm-cap020", sizeof(c->driver));
	strscpy(c->card, "GM8126 capture", sizeof(c->card));
	return 0;
}

static int cap_enum_fmt(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	if (f->index)
		return -EINVAL;
	f->pixelformat = V4L2_PIX_FMT_UYVY;
	return 0;
}

static void cap_fill_fmt(struct v4l2_pix_format *p)
{
	v4l_bound_align_image(&p->width, CAP_MIN_W, CAP_IN_W, 4,
			      &p->height, CAP_MIN_H, CAP_IN_H, 4, 0);
	p->pixelformat = V4L2_PIX_FMT_UYVY;
	p->field = V4L2_FIELD_NONE;
	p->bytesperline = p->width * 2;
	p->sizeimage = p->bytesperline * p->height;
	p->colorspace = V4L2_COLORSPACE_SMPTE170M;
	p->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	p->quantization = V4L2_QUANTIZATION_DEFAULT;
	p->xfer_func = V4L2_XFER_FUNC_DEFAULT;
}

static int cap_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct cap_dev *cap = video_drvdata(file);

	f->fmt.pix = cap->pix;
	return 0;
}

static int cap_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	cap_fill_fmt(&f->fmt.pix);
	return 0;
}

static int cap_s_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct cap_dev *cap = video_drvdata(file);

	if (vb2_is_busy(&cap->queue))
		return -EBUSY;
	cap_fill_fmt(&f->fmt.pix);
	cap->pix = f->fmt.pix;
	return 0;
}

static int cap_enum_framesizes(struct file *file, void *priv,
			       struct v4l2_frmsizeenum *f)
{
	if (f->index || f->pixel_format != V4L2_PIX_FMT_UYVY)
		return -EINVAL;
	f->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	f->stepwise.min_width = CAP_MIN_W;
	f->stepwise.max_width = CAP_IN_W;
	f->stepwise.step_width = 16;
	f->stepwise.min_height = CAP_MIN_H;
	f->stepwise.max_height = CAP_IN_H;
	f->stepwise.step_height = 16;
	return 0;
}

static int cap_enum_input(struct file *file, void *priv, struct v4l2_input *i)
{
	if (i->index)
		return -EINVAL;
	i->type = V4L2_INPUT_TYPE_CAMERA;
	strscpy(i->name, "Camera", sizeof(i->name));
	return 0;
}

static int cap_g_input(struct file *file, void *priv, unsigned int *i)
{
	*i = 0;
	return 0;
}

static int cap_s_input(struct file *file, void *priv, unsigned int i)
{
	return i ? -EINVAL : 0;
}

static const struct v4l2_ioctl_ops cap_ioctl_ops = {
	.vidioc_querycap = cap_querycap,
	.vidioc_enum_fmt_vid_cap = cap_enum_fmt,
	.vidioc_g_fmt_vid_cap = cap_g_fmt,
	.vidioc_s_fmt_vid_cap = cap_s_fmt,
	.vidioc_try_fmt_vid_cap = cap_try_fmt,
	.vidioc_enum_framesizes = cap_enum_framesizes,
	.vidioc_enum_input = cap_enum_input,
	.vidioc_g_input = cap_g_input,
	.vidioc_s_input = cap_s_input,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_log_status = v4l2_ctrl_log_status,
	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

/* The sensor is powered by its driver's s_power, from first open to last. */
static int cap_open(struct file *file)
{
	struct cap_dev *cap = video_drvdata(file);
	int ret;

	mutex_lock(&cap->lock);
	ret = v4l2_fh_open(file);
	if (!ret && v4l2_fh_is_singular_file(file)) {
		ret = v4l2_subdev_call(cap->sensor, core, s_power, 1);
		if (ret == -ENOIOCTLCMD)
			ret = 0;
		if (ret)
			v4l2_fh_release(file);
	}
	mutex_unlock(&cap->lock);
	return ret;
}

static int cap_release(struct file *file)
{
	struct cap_dev *cap = video_drvdata(file);
	bool last;

	mutex_lock(&cap->lock);
	last = v4l2_fh_is_singular_file(file);
	_vb2_fop_release(file, NULL);
	if (last)
		v4l2_subdev_call(cap->sensor, core, s_power, 0);
	mutex_unlock(&cap->lock);
	return 0;
}

static const struct v4l2_file_operations cap_fops = {
	.owner = THIS_MODULE,
	.open = cap_open,
	.release = cap_release,
	.unlocked_ioctl = video_ioctl2,
	.mmap = vb2_fop_mmap,
	.poll = vb2_fop_poll,
	.read = vb2_fop_read,
};

/* ---- the sensor ------------------------------------------------------------ */

static int cap_bound(struct v4l2_async_notifier *notifier,
		     struct v4l2_subdev *sd, struct v4l2_async_connection *asc)
{
	struct cap_dev *cap = container_of(notifier, struct cap_dev, notifier);

	cap->sensor = sd;
	return 0;
}

static int cap_complete(struct v4l2_async_notifier *notifier)
{
	struct cap_dev *cap = container_of(notifier, struct cap_dev, notifier);
	struct v4l2_subdev_format fmt = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
		.format = {
			.width = CAP_IN_W,
			.height = CAP_IN_H,
			.code = MEDIA_BUS_FMT_UYVY8_2X8,
			.field = V4L2_FIELD_NONE,
		},
	};
	int ret;

	ret = v4l2_subdev_call(cap->sensor, pad, set_fmt, NULL, &fmt);
	if (ret)
		return dev_err_probe(cap->dev, ret, "sensor format\n");
	if (fmt.format.width != CAP_IN_W || fmt.format.height != CAP_IN_H ||
	    fmt.format.code != MEDIA_BUS_FMT_UYVY8_2X8)
		return dev_err_probe(cap->dev, -EINVAL,
				     "sensor gives %ux%u, code %x\n",
				     fmt.format.width, fmt.format.height,
				     fmt.format.code);

	cap->v4l2.ctrl_handler = cap->sensor->ctrl_handler;
	cap->vdev.ctrl_handler = cap->sensor->ctrl_handler;
	ret = video_register_device(&cap->vdev, VFL_TYPE_VIDEO, -1);
	if (ret)
		return ret;
	dev_info(cap->dev, "%s on %s\n", cap->sensor->name,
		 video_device_node_name(&cap->vdev));
	return 0;
}

static void cap_unbind(struct v4l2_async_notifier *notifier,
		       struct v4l2_subdev *sd,
		       struct v4l2_async_connection *asc)
{
	struct cap_dev *cap = container_of(notifier, struct cap_dev, notifier);

	vb2_video_unregister_device(&cap->vdev);
	cap->sensor = NULL;
}

static const struct v4l2_async_notifier_operations cap_async_ops = {
	.bound = cap_bound,
	.complete = cap_complete,
	.unbind = cap_unbind,
};

static int cap_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct v4l2_async_connection *asc;
	struct fwnode_handle *ep;
	struct vb2_queue *q;
	struct cap_dev *cap;
	int irq, ret;

	cap = devm_kzalloc(dev, sizeof(*cap), GFP_KERNEL);
	if (!cap)
		return -ENOMEM;
	cap->dev = dev;
	platform_set_drvdata(pdev, cap);
	mutex_init(&cap->lock);
	spin_lock_init(&cap->irqlock);
	INIT_LIST_HEAD(&cap->linked);
	INIT_LIST_HEAD(&cap->waiting);

	cap->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(cap->base))
		return PTR_ERR(cap->base);
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;
	cap->clk = devm_clk_get_optional(dev, NULL);
	if (IS_ERR(cap->clk))
		return dev_err_probe(dev, PTR_ERR(cap->clk), "clock\n");
	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;
	cap->lists = dmam_alloc_coherent(dev,
					 (LIST_BUFS + CAP_BUFS) * LIST_BYTES,
					 &cap->lists_dma, GFP_KERNEL);
	if (!cap->lists)
		return -ENOMEM;

	writel(0xffffffff, cap->base + CAP_MASK);
	ret = devm_request_irq(dev, irq, cap_irq, 0, dev_name(dev), cap);
	if (ret)
		return ret;

	cap->pix.width = CAP_IN_W;
	cap->pix.height = CAP_IN_H;
	cap_fill_fmt(&cap->pix);

	ret = v4l2_device_register(dev, &cap->v4l2);
	if (ret)
		return ret;

	q = &cap->queue;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_DMABUF | VB2_READ;
	q->drv_priv = cap;
	q->buf_struct_size = sizeof(struct cap_buffer);
	q->ops = &cap_vb2_ops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_queued_buffers = 2;
	q->lock = &cap->lock;
	q->dev = dev;
	ret = vb2_queue_init(q);
	if (ret)
		goto err_v4l2;

	strscpy(cap->vdev.name, "gm-cap020", sizeof(cap->vdev.name));
	cap->vdev.v4l2_dev = &cap->v4l2;
	cap->vdev.fops = &cap_fops;
	cap->vdev.ioctl_ops = &cap_ioctl_ops;
	cap->vdev.release = video_device_release_empty;
	cap->vdev.lock = &cap->lock;
	cap->vdev.queue = q;
	cap->vdev.device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
				V4L2_CAP_READWRITE;
	video_set_drvdata(&cap->vdev, cap);

	v4l2_async_nf_init(&cap->notifier, &cap->v4l2);
	ep = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
	if (!ep) {
		ret = dev_err_probe(dev, -ENODEV, "no endpoint\n");
		goto err_nf;
	}
	asc = v4l2_async_nf_add_fwnode_remote(&cap->notifier, ep,
					      struct v4l2_async_connection);
	fwnode_handle_put(ep);
	if (IS_ERR(asc)) {
		ret = PTR_ERR(asc);
		goto err_nf;
	}
	cap->notifier.ops = &cap_async_ops;
	ret = v4l2_async_nf_register(&cap->notifier);
	if (ret)
		goto err_nf;
	return 0;

err_nf:
	v4l2_async_nf_cleanup(&cap->notifier);
err_v4l2:
	v4l2_device_unregister(&cap->v4l2);
	return ret;
}

static void cap_remove(struct platform_device *pdev)
{
	struct cap_dev *cap = platform_get_drvdata(pdev);

	v4l2_async_nf_unregister(&cap->notifier);
	v4l2_async_nf_cleanup(&cap->notifier);
	v4l2_device_unregister(&cap->v4l2);
}

static const struct of_device_id cap_of_match[] = {
	{ .compatible = "grainmedia,gm8126-cap020" },
	{ }
};
MODULE_DEVICE_TABLE(of, cap_of_match);

static struct platform_driver cap_driver = {
	.probe = cap_probe,
	.remove = cap_remove,
	.driver = {
		.name = "gm-cap020",
		.of_match_table = cap_of_match,
	},
};
module_platform_driver(cap_driver);

MODULE_DESCRIPTION("Grain Media GM8126 capture driver");
MODULE_LICENSE("GPL");
