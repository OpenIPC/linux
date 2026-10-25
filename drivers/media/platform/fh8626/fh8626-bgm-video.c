// SPDX-License-Identifier: GPL-2.0
/*
 * FH8626 background model: GREY input to GREY foreground mask (mem2mem).
 *
 * Copyright (C) 2026 Difan Zhang
 */
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/kref.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-vmalloc.h>

#include "fh8626-bgm-hw.h"
#include "fh8626-event.h"

#define BGM_NAME "fh8626-bgm"
#define BGM_QUEUE_BYTES (8U * 1024 * 1024)

struct bgm_ctx;

struct bgm_dev {
	struct kref ref;
	struct mutex lock;
	struct v4l2_device v4l2;
	struct video_device video;
	struct v4l2_m2m_dev *m2m;
	struct workqueue_struct *workqueue;
	struct fh8626_bgm_hw *hw;
	struct v4l2_ctrl_handler ctrl_handler;
	u32 md_mode;
	struct dentry *debugfs;
	struct clk *core_clk;
	struct clk *hclk;
	struct bgm_ctx *ctx;
	bool disconnected;
	spinlock_t stats_lock;
	bool stats_valid;
	u32 stats_sequence;
	u32 stats_width;
	u32 stats_height;
	u64 stats_timestamp;
	u32 stats[FH8626_BGM_STAT_COUNT];
};

struct bgm_ctx {
	struct v4l2_fh fh;
	struct bgm_dev *dev;
	struct work_struct work;
	u32 width;
	u32 height;
	u32 sequence;
	bool streaming[2];
	bool hw_ready;
	bool aborting;
};

static struct bgm_ctx *file_ctx(struct file *file)
{
	return container_of(file->private_data, struct bgm_ctx, fh);
}

static unsigned int queue_index(enum v4l2_buf_type type)
{
	return type == V4L2_BUF_TYPE_VIDEO_CAPTURE;
}

static void return_buffers(struct bgm_ctx *ctx, enum v4l2_buf_type type,
			   enum vb2_buffer_state state)
{
	struct vb2_v4l2_buffer *buf;

	for (;;) {
		if (V4L2_TYPE_IS_OUTPUT(type))
			buf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
		else
			buf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
		if (!buf)
			break;
		if (!V4L2_TYPE_IS_OUTPUT(type))
			vb2_set_plane_payload(&buf->vb2_buf, 0, 0);
		v4l2_m2m_buf_done(buf, state);
	}
}

/* Worker never acquires dev->lock: STREAMOFF/release hold it while waiting. */
static void bgm_work(struct work_struct *work)
{
	struct bgm_ctx *ctx = container_of(work, struct bgm_ctx, work);
	struct bgm_dev *dev = ctx->dev;
	struct fh8626_bgm_hw_result result;
	struct vb2_v4l2_buffer *src, *dst;
	enum vb2_buffer_state state = VB2_BUF_STATE_ERROR;
	const u8 *fg;
	void *input;
	u8 *output;
	u32 i, pixels = ctx->width * ctx->height;
	int ret = -ECANCELED;
	unsigned long flags;

	src = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
	dst = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
	if (!src || !dst)
		goto done;

	src->sequence = dst->sequence = ctx->sequence++;
	dst->vb2_buf.timestamp = src->vb2_buf.timestamp;
	dst->timecode = src->timecode;
	dst->flags = src->flags & (V4L2_BUF_FLAG_TIMECODE |
				  V4L2_BUF_FLAG_TSTAMP_SRC_MASK);
	dst->field = V4L2_FIELD_NONE;
	vb2_set_plane_payload(&dst->vb2_buf, 0, 0);
	if (READ_ONCE(ctx->aborting) || READ_ONCE(dev->disconnected))
		goto done;

	input = vb2_plane_vaddr(&src->vb2_buf, 0);
	output = vb2_plane_vaddr(&dst->vb2_buf, 0);
	if (!input || !output) {
		ret = -EFAULT;
		goto done;
	}

	if (READ_ONCE(dev->md_mode) == V4L2_DETECT_MD_MODE_DISABLED) {
		memset(output, 0, pixels);
		vb2_set_plane_payload(&dst->vb2_buf, 0, pixels);
		state = VB2_BUF_STATE_DONE;
		ret = 0;
		goto done;
	}

	ret = fh8626_bgm_hw_process(dev->hw, input, &result);
	if (ret || READ_ONCE(ctx->aborting) || READ_ONCE(dev->disconnected))
		goto done;
	fg = result.map[FH8626_BGM_FG].data;
	if (!fg || result.map[FH8626_BGM_FG].length < pixels / 4) {
		ret = -EIO;
		goto done;
	}

	/*
	 * Foreground A is the even bit of each two-bit cell, four pixels per
	 * byte from the low bits up.
	 */
	for (i = 0; i < pixels; i++)
		output[i] = fg[i >> 2] & BIT(2 * (i & 3)) ? 255 : 0;
	if (!READ_ONCE(ctx->aborting) && !READ_ONCE(dev->disconnected)) {
		spin_lock_irqsave(&dev->stats_lock, flags);
		memcpy(dev->stats, result.stats, sizeof(dev->stats));
		dev->stats_sequence = src->sequence;
		dev->stats_timestamp = src->vb2_buf.timestamp;
		dev->stats_width = ctx->width;
		dev->stats_height = ctx->height;
		dev->stats_valid = true;
		spin_unlock_irqrestore(&dev->stats_lock, flags);
		vb2_set_plane_payload(&dst->vb2_buf, 0, pixels);
		state = VB2_BUF_STATE_DONE;
	}
done:
	if (ret && ret != -ECANCELED) {
		WRITE_ONCE(ctx->aborting, true);
		vb2_queue_error(v4l2_m2m_get_src_vq(ctx->fh.m2m_ctx));
		vb2_queue_error(v4l2_m2m_get_dst_vq(ctx->fh.m2m_ctx));
	}
	if (src)
		v4l2_m2m_buf_done(src, state);
	if (dst)
		v4l2_m2m_buf_done(dst, state);
	v4l2_m2m_job_finish(dev->m2m, ctx->fh.m2m_ctx);
}

static void bgm_device_run(void *priv)
{
	struct bgm_ctx *ctx = priv;

	queue_work(ctx->dev->workqueue, &ctx->work);
}

static int bgm_job_ready(void *priv)
{
	struct bgm_ctx *ctx = priv;

	return READ_ONCE(ctx->hw_ready) && !READ_ONCE(ctx->aborting) &&
		!READ_ONCE(ctx->dev->disconnected);
}

static void bgm_job_abort(void *priv)
{
	struct bgm_ctx *ctx = priv;

	/* The backend wait is bounded; DMA stop remains unconfirmed here. */
	WRITE_ONCE(ctx->aborting, true);
}

static const struct v4l2_m2m_ops bgm_m2m_ops = {
	.device_run = bgm_device_run,
	.job_ready = bgm_job_ready,
	.job_abort = bgm_job_abort,
};

static int bgm_queue_setup(struct vb2_queue *q, unsigned int *nbuffers,
			   unsigned int *nplanes, unsigned int sizes[],
			   struct device *alloc_devs[])
{
	struct bgm_ctx *ctx = vb2_get_drv_priv(q);
	u32 size = ctx->width * ctx->height;

	if (ctx->dev->disconnected)
		return -ENODEV;
	if (*nplanes)
		return *nplanes != 1 || sizes[0] < size ? -EINVAL : 0;
	*nbuffers = min_t(unsigned int, *nbuffers, BGM_QUEUE_BYTES / size);
	*nplanes = 1;
	sizes[0] = size;
	return 0;
}

static int bgm_buf_prepare(struct vb2_buffer *vb)
{
	struct bgm_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *buf = to_vb2_v4l2_buffer(vb);
	u32 size = ctx->width * ctx->height;

	if (ctx->dev->disconnected)
		return -ENODEV;
	if (vb2_plane_size(vb, 0) < size)
		return -EINVAL;
	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		if (buf->field == V4L2_FIELD_ANY)
			buf->field = V4L2_FIELD_NONE;
		if (buf->field != V4L2_FIELD_NONE ||
		    vb2_get_plane_payload(vb, 0) < size)
			return -EINVAL;
		vb2_set_plane_payload(vb, 0, size);
	} else {
		vb2_set_plane_payload(vb, 0, 0);
	}
	return 0;
}

static void bgm_buf_queue(struct vb2_buffer *vb)
{
	struct bgm_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, to_vb2_v4l2_buffer(vb));
}

static int bgm_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct bgm_ctx *ctx = vb2_get_drv_priv(q);
	unsigned int index = queue_index(q->type);
	int ret;

	if (ctx->dev->disconnected) {
		ret = -ENODEV;
		goto fail;
	}
	if (fh8626_bgm_hw_poisoned(ctx->dev->hw)) {
		ret = -EIO;
		goto fail;
	}
	if (ctx->streaming[index ^ 1]) {
		ret = fh8626_bgm_hw_alloc(ctx->dev->hw, ctx->width,
					  ctx->height);
		if (ret)
			goto fail;
		ctx->sequence = 0;
		WRITE_ONCE(ctx->aborting, false);
		WRITE_ONCE(ctx->hw_ready, true);
	}
	ctx->streaming[index] = true;
	return 0;
fail:
	return_buffers(ctx, q->type, VB2_BUF_STATE_QUEUED);
	return ret;
}

static void bgm_stop_streaming(struct vb2_queue *q)
{
	struct bgm_ctx *ctx = vb2_get_drv_priv(q);

	WRITE_ONCE(ctx->aborting, true);
	/* job_finish() may wake release before the worker stops using ctx. */
	flush_work(&ctx->work);
	ctx->streaming[queue_index(q->type)] = false;
	if (ctx->hw_ready) {
		fh8626_bgm_hw_free(ctx->dev->hw);
		WRITE_ONCE(ctx->hw_ready, false);
	}
	return_buffers(ctx, q->type, VB2_BUF_STATE_ERROR);
}

static const struct vb2_ops bgm_queue_ops = {
	.queue_setup = bgm_queue_setup,
	.buf_prepare = bgm_buf_prepare,
	.buf_queue = bgm_buf_queue,
	.start_streaming = bgm_start_streaming,
	.stop_streaming = bgm_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

static int bgm_queue_init(void *priv, struct vb2_queue *src,
			  struct vb2_queue *dst)
{
	struct bgm_ctx *ctx = priv;
	struct vb2_queue *q;
	int i, ret;

	for (i = 0; i < 2; i++) {
		q = i ? dst : src;
		q->type = i ? V4L2_BUF_TYPE_VIDEO_CAPTURE :
			      V4L2_BUF_TYPE_VIDEO_OUTPUT;
		q->io_modes = VB2_MMAP;
		q->drv_priv = ctx;
		q->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
		q->ops = &bgm_queue_ops;
		q->mem_ops = &vb2_vmalloc_memops;
		q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
		q->lock = &ctx->dev->lock;
		ret = vb2_queue_init(q);
		if (ret)
			return ret;
	}
	return 0;
}

static int bgm_querycap(struct file *file, void *priv,
			struct v4l2_capability *cap)
{
	strlcpy(cap->driver, BGM_NAME, sizeof(cap->driver));
	strlcpy(cap->card, "FH8626 foreground mask", sizeof(cap->card));
	strlcpy(cap->bus_info, "platform:" BGM_NAME, sizeof(cap->bus_info));
	cap->device_caps = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int bgm_enum_fmt(struct file *file, void *priv,
			struct v4l2_fmtdesc *f)
{
	if (f->index)
		return -EINVAL;
	f->pixelformat = V4L2_PIX_FMT_GREY;
	strlcpy(f->description, V4L2_TYPE_IS_OUTPUT(f->type) ?
		"8-bit greyscale" : "Foreground A mask",
		sizeof(f->description));
	return 0;
}

static void bgm_set_pix(struct v4l2_pix_format *pix, u32 width, u32 height)
{
	memset(pix, 0, sizeof(*pix));
	pix->width = width;
	pix->height = height;
	pix->pixelformat = V4L2_PIX_FMT_GREY;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = width;
	pix->sizeimage = width * height;
	pix->colorspace = V4L2_COLORSPACE_RAW;
	pix->xfer_func = V4L2_XFER_FUNC_NONE;
	pix->quantization = V4L2_QUANTIZATION_FULL_RANGE;
}

static int bgm_get_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct bgm_ctx *ctx = file_ctx(file);

	bgm_set_pix(&f->fmt.pix, ctx->width, ctx->height);
	return 0;
}

static int bgm_try_output(struct file *file, void *priv,
			  struct v4l2_format *f)
{
	u32 width = clamp_t(u32, f->fmt.pix.width,
		FH8626_BGM_MIN_WIDTH, FH8626_BGM_MAX_WIDTH);
	u32 height = clamp_t(u32, f->fmt.pix.height,
		FH8626_BGM_MIN_HEIGHT, FH8626_BGM_MAX_HEIGHT);

	width = round_down(width, FH8626_BGM_ALIGNMENT);
	height = round_down(height, FH8626_BGM_ALIGNMENT);
	bgm_set_pix(&f->fmt.pix, width, height);
	return 0;
}

static bool bgm_queues_busy(struct bgm_ctx *ctx)
{
	return vb2_is_busy(v4l2_m2m_get_src_vq(ctx->fh.m2m_ctx)) ||
		vb2_is_busy(v4l2_m2m_get_dst_vq(ctx->fh.m2m_ctx));
}

static int bgm_set_output(struct file *file, void *priv,
			  struct v4l2_format *f)
{
	struct bgm_ctx *ctx = file_ctx(file);

	if (bgm_queues_busy(ctx))
		return -EBUSY;
	bgm_try_output(file, priv, f);
	ctx->width = f->fmt.pix.width;
	ctx->height = f->fmt.pix.height;
	return 0;
}

static int bgm_set_capture(struct file *file, void *priv,
			   struct v4l2_format *f)
{
	if (bgm_queues_busy(file_ctx(file)))
		return -EBUSY;
	/* Mask geometry always follows the input plane; no scaler exists. */
	return bgm_get_fmt(file, priv, f);
}

static int bgm_enum_framesizes(struct file *file, void *priv,
			      struct v4l2_frmsizeenum *f)
{
	if (f->index || f->pixel_format != V4L2_PIX_FMT_GREY)
		return -EINVAL;
	f->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	f->stepwise.min_width = FH8626_BGM_MIN_WIDTH;
	f->stepwise.max_width = FH8626_BGM_MAX_WIDTH;
	f->stepwise.step_width = FH8626_BGM_ALIGNMENT;
	f->stepwise.min_height = FH8626_BGM_MIN_HEIGHT;
	f->stepwise.max_height = FH8626_BGM_MAX_HEIGHT;
	f->stepwise.step_height = FH8626_BGM_ALIGNMENT;
	return 0;
}

#define V4L2_CID_FH8626_BGM_REINIT (V4L2_CID_USER_BASE + 0x1000)

static int bgm_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct bgm_dev *dev = container_of(ctrl->handler, struct bgm_dev,
					   ctrl_handler);

	switch (ctrl->id) {
	case V4L2_CID_DETECT_MD_MODE:
		WRITE_ONCE(dev->md_mode, ctrl->val);
		if (ctrl->val == V4L2_DETECT_MD_MODE_GLOBAL && dev->hw)
			fh8626_bgm_hw_reinit_background(dev->hw);
		break;
	case V4L2_CID_DETECT_MD_GLOBAL_THRESHOLD:
		if (dev->hw)
			fh8626_bgm_hw_set_threshold(dev->hw, ctrl->val);
		break;
	case V4L2_CID_FH8626_BGM_REINIT:
		if (dev->hw)
			fh8626_bgm_hw_reinit_background(dev->hw);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ctrl_ops bgm_ctrl_ops = {
	.s_ctrl = bgm_s_ctrl,
};

static const struct v4l2_ctrl_config bgm_ctrl_reinit = {
	.ops = &bgm_ctrl_ops,
	.id = V4L2_CID_FH8626_BGM_REINIT,
	.name = "Background Model Reinit",
	.type = V4L2_CTRL_TYPE_BUTTON,
	.flags = V4L2_CTRL_FLAG_WRITE_ONLY | V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
};

static const struct v4l2_ioctl_ops bgm_ioctl_ops = {
	.vidioc_querycap = bgm_querycap,
	.vidioc_enum_fmt_vid_out = bgm_enum_fmt,
	.vidioc_enum_fmt_vid_cap = bgm_enum_fmt,
	.vidioc_enum_framesizes = bgm_enum_framesizes,
	.vidioc_g_fmt_vid_out = bgm_get_fmt,
	.vidioc_g_fmt_vid_cap = bgm_get_fmt,
	.vidioc_try_fmt_vid_out = bgm_try_output,
	.vidioc_try_fmt_vid_cap = bgm_get_fmt,
	.vidioc_s_fmt_vid_out = bgm_set_output,
	.vidioc_s_fmt_vid_cap = bgm_set_capture,
	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf = v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf = v4l2_m2m_ioctl_dqbuf,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,
	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

/*
 * Called under dev->lock. Stopping through mem2mem also cancels a job
 * that is scheduled and waiting to start.
 */
static void bgm_quiesce(struct bgm_ctx *ctx)
{
	WRITE_ONCE(ctx->aborting, true);
	v4l2_m2m_streamoff(NULL, ctx->fh.m2m_ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT);
	v4l2_m2m_streamoff(NULL, ctx->fh.m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	flush_work(&ctx->work);
}

static int bgm_open(struct file *file)
{
	struct bgm_dev *dev = video_drvdata(file);
	struct bgm_ctx *ctx;
	int ret;

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;
	if (dev->disconnected) {
		ret = -ENODEV;
		goto unlock;
	}
	if (dev->ctx) {
		ret = -EBUSY;
		goto unlock;
	}
	if (fh8626_bgm_hw_poisoned(dev->hw)) {
		ret = -EIO;
		goto unlock;
	}
	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		ret = -ENOMEM;
		goto unlock;
	}
	ctx->dev = dev;
	ctx->width = 160;
	ctx->height = 96;
	ctx->aborting = true;
	INIT_WORK(&ctx->work, bgm_work);
	v4l2_fh_init(&ctx->fh, &dev->video);
	ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(dev->m2m, ctx, bgm_queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		v4l2_fh_exit(&ctx->fh);
		kfree(ctx);
		goto unlock;
	}
	file->private_data = &ctx->fh;
	ctx->fh.ctrl_handler = &dev->ctrl_handler;
	v4l2_fh_add(&ctx->fh);
	dev->ctx = ctx;
	ret = 0;
unlock:
	mutex_unlock(&dev->lock);
	return ret;
}

static int bgm_release(struct file *file)
{
	struct bgm_ctx *ctx = file_ctx(file);
	struct bgm_dev *dev = ctx->dev;

	mutex_lock(&dev->lock);
	bgm_quiesce(ctx);
	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	dev->ctx = NULL;
	mutex_unlock(&dev->lock);
	kfree(ctx);
	return 0;
}

static unsigned int bgm_poll(struct file *file, poll_table *wait)
{
	struct bgm_ctx *ctx = file_ctx(file);
	unsigned int ret;

	mutex_lock(&ctx->dev->lock);
	ret = ctx->dev->disconnected ? POLLERR | POLLHUP :
		v4l2_m2m_poll(file, ctx->fh.m2m_ctx, wait);
	/* Inspect errors after poll has registered its waiters. */
	if (v4l2_m2m_get_src_vq(ctx->fh.m2m_ctx)->error ||
	    v4l2_m2m_get_dst_vq(ctx->fh.m2m_ctx)->error)
		ret |= POLLERR;
	mutex_unlock(&ctx->dev->lock);
	return ret;
}

static int bgm_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct bgm_ctx *ctx = file_ctx(file);
	int ret;

	mutex_lock(&ctx->dev->lock);
	ret = ctx->dev->disconnected ? -ENODEV :
		v4l2_m2m_mmap(file, ctx->fh.m2m_ctx, vma);
	mutex_unlock(&ctx->dev->lock);
	return ret;
}

static const struct v4l2_file_operations bgm_fops = {
	.owner = THIS_MODULE,
	.open = bgm_open,
	.release = bgm_release,
	.poll = bgm_poll,
	.mmap = bgm_mmap,
	.unlocked_ioctl = fh8626_video_ioctl2,
};

static void bgm_dev_release(struct kref *ref)
{
	struct bgm_dev *dev = container_of(ref, struct bgm_dev, ref);

	if (dev->m2m)
		v4l2_m2m_release(dev->m2m);
	if (dev->workqueue)
		destroy_workqueue(dev->workqueue);
	v4l2_ctrl_handler_free(&dev->ctrl_handler);
	v4l2_device_unregister(&dev->v4l2);
	kfree(dev);
}

static void bgm_video_release(struct video_device *video)
{
	struct bgm_dev *dev = container_of(video, struct bgm_dev, video);

	kref_put(&dev->ref, bgm_dev_release);
}

static int statistics_show(struct seq_file *seq, void *unused)
{
	static const u16 offsets[FH8626_BGM_STAT_COUNT] = {
		0xbc, 0xc0, 0xc4, 0xc8, 0xcc, 0xd0, 0xd4, 0xd8, 0xdc,
		0xf0, 0xe0, 0xf4, 0xe4, 0xf8, 0xe8, 0xfc, 0xec, 0x100,
	};
	struct bgm_dev *dev = seq->private;
	u32 stats[FH8626_BGM_STAT_COUNT], sequence, width, height;
	u64 timestamp;
	unsigned long flags;
	unsigned int i;
	bool valid;

	spin_lock_irqsave(&dev->stats_lock, flags);
	valid = dev->stats_valid;
	memcpy(stats, dev->stats, sizeof(stats));
	sequence = dev->stats_sequence;
	timestamp = dev->stats_timestamp;
	width = dev->stats_width;
	height = dev->stats_height;
	spin_unlock_irqrestore(&dev->stats_lock, flags);
	if (!valid) {
		seq_puts(seq, "no completed frame\n");
		return 0;
	}
	seq_printf(seq, "sequence=%u timestamp_ns=%llu width=%u height=%u\n",
		   sequence, timestamp, width, height);
	for (i = 0; i < ARRAY_SIZE(stats); i++)
		seq_printf(seq, "reg_%03x=%u\n", offsets[i], stats[i]);
	return 0;
}

static int statistics_open(struct inode *inode, struct file *file)
{
	struct bgm_dev *dev = inode->i_private;
	int ret;

	/* Hold a reference for the seq_file lifetime, past platform removal. */
	kref_get(&dev->ref);
	ret = single_open(file, statistics_show, dev);
	if (ret)
		kref_put(&dev->ref, bgm_dev_release);
	return ret;
}

static int statistics_release(struct inode *inode, struct file *file)
{
	struct seq_file *seq = file->private_data;
	struct bgm_dev *dev = seq->private;
	int ret = single_release(inode, file);

	kref_put(&dev->ref, bgm_dev_release);
	return ret;
}

static const struct file_operations statistics_fops = {
	.owner = THIS_MODULE,
	.open = statistics_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = statistics_release,
};

static int bgm_probe(struct platform_device *pdev)
{
	struct bgm_dev *dev;
	struct resource *res;
	void __iomem *regs;
	bool quarantined;
	int irq, ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;
	kref_init(&dev->ref); /* platform reference, separate from video node */
	mutex_init(&dev->lock);
	spin_lock_init(&dev->stats_lock);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
		goto put_dev;
	}
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto put_dev;
	}
	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		goto put_dev;
	/* Clocks are held explicitly so a fault can outlive remove(). */
	dev->core_clk = clk_get(&pdev->dev, "bgm");
	if (IS_ERR(dev->core_clk)) {
		ret = PTR_ERR(dev->core_clk);
		goto put_dev;
	}
	dev->hclk = clk_get(&pdev->dev, "hclk");
	if (IS_ERR(dev->hclk)) {
		ret = PTR_ERR(dev->hclk);
		goto put_core_clk;
	}
	ret = clk_prepare_enable(dev->hclk);
	if (ret)
		goto put_hclk;
	ret = clk_prepare_enable(dev->core_clk);
	if (ret)
		goto disable_hclk;
	dev->hw = fh8626_bgm_hw_create(&pdev->dev, regs, irq, res->start);
	if (IS_ERR(dev->hw)) {
		ret = PTR_ERR(dev->hw);
		goto disable_core;
	}
	dev->workqueue = alloc_ordered_workqueue(BGM_NAME, WQ_MEM_RECLAIM);
	if (!dev->workqueue) {
		ret = -ENOMEM;
		goto destroy_hw;
	}
	ret = v4l2_device_register(&pdev->dev, &dev->v4l2);
	if (ret)
		goto destroy_hw;
	dev->m2m = v4l2_m2m_init(&bgm_m2m_ops);
	if (IS_ERR(dev->m2m)) {
		ret = PTR_ERR(dev->m2m);
		dev->m2m = NULL;
		goto destroy_hw;
	}
	ret = v4l2_ctrl_handler_init(&dev->ctrl_handler, 3);
	if (ret)
		goto destroy_hw;
	v4l2_ctrl_new_std_menu(&dev->ctrl_handler, &bgm_ctrl_ops,
			       V4L2_CID_DETECT_MD_MODE,
			       V4L2_DETECT_MD_MODE_GLOBAL, 0,
			       V4L2_DETECT_MD_MODE_GLOBAL);
	v4l2_ctrl_new_std(&dev->ctrl_handler, &bgm_ctrl_ops,
			  V4L2_CID_DETECT_MD_GLOBAL_THRESHOLD,
			  1, 255, 1, FH8626_BGM_DEFAULT_THRESHOLD);
	v4l2_ctrl_new_custom(&dev->ctrl_handler, &bgm_ctrl_reinit, NULL);
	if (dev->ctrl_handler.error) {
		ret = dev->ctrl_handler.error;
		v4l2_ctrl_handler_free(&dev->ctrl_handler);
		goto destroy_hw;
	}
	dev->md_mode = V4L2_DETECT_MD_MODE_GLOBAL;
	strlcpy(dev->video.name, BGM_NAME, sizeof(dev->video.name));
	dev->video.v4l2_dev = &dev->v4l2;
	dev->video.fops = &bgm_fops;
	dev->video.ioctl_ops = &bgm_ioctl_ops;
	dev->video.lock = &dev->lock;
	dev->video.ctrl_handler = &dev->ctrl_handler;
	dev->video.vfl_dir = VFL_DIR_M2M;
	dev->video.device_caps = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;
	dev->video.release = bgm_video_release;
	video_set_drvdata(&dev->video, dev);
	kref_get(&dev->ref);
	ret = video_register_device(&dev->video, VFL_TYPE_GRABBER, -1);
	if (ret) {
		if (dev->video.minor == -1)
			kref_put(&dev->ref, bgm_dev_release);
		goto destroy_hw;
	}
	platform_set_drvdata(pdev, dev);
	dev->debugfs = debugfs_create_dir(dev_name(&pdev->dev), NULL);
	if (IS_ERR_OR_NULL(dev->debugfs))
		dev->debugfs = NULL;
	else
		debugfs_create_file("statistics", 0444, dev->debugfs, dev,
				    &statistics_fops);
	dev_dbg(&pdev->dev, "registered %s\n",
		video_device_node_name(&dev->video));
	return 0;

destroy_hw:
	quarantined = fh8626_bgm_hw_destroy(dev->hw);
	dev->hw = NULL;
	if (quarantined)
		goto put_dev;
disable_core:
	clk_disable_unprepare(dev->core_clk);
disable_hclk:
	clk_disable_unprepare(dev->hclk);
put_hclk:
	clk_put(dev->hclk);
put_core_clk:
	clk_put(dev->core_clk);
put_dev:
	kref_put(&dev->ref, bgm_dev_release);
	return ret;
}

/* Caller holds dev->lock. Also used at shutdown without freeing the node. */
static void bgm_disconnect_hw(struct bgm_dev *dev)
{
	WRITE_ONCE(dev->disconnected, true);
	if (dev->ctx) {
		vb2_queue_error(v4l2_m2m_get_src_vq(dev->ctx->fh.m2m_ctx));
		vb2_queue_error(v4l2_m2m_get_dst_vq(dev->ctx->fh.m2m_ctx));
		bgm_quiesce(dev->ctx);
	}
	/*
	 * IRQ teardown precedes clock teardown. A timed-out backend keeps its
	 * DMA allocations to prevent reuse while DMA stop is unconfirmed.
	 */
	if (dev->hw) {
		bool quarantined = fh8626_bgm_hw_destroy(dev->hw);

		dev->hw = NULL;
		if (!quarantined) {
			clk_disable_unprepare(dev->core_clk);
			clk_disable_unprepare(dev->hclk);
			clk_put(dev->core_clk);
			clk_put(dev->hclk);
		}
	}
}

static int bgm_remove(struct platform_device *pdev)
{
	struct bgm_dev *dev = platform_get_drvdata(pdev);

	debugfs_remove_recursive(dev->debugfs);
	mutex_lock(&dev->lock);
	bgm_disconnect_hw(dev);
	video_unregister_device(&dev->video);
	fh8626_video_wake_events(&dev->video);
	v4l2_device_disconnect(&dev->v4l2);
	platform_set_drvdata(pdev, NULL);
	mutex_unlock(&dev->lock);
	kref_put(&dev->ref, bgm_dev_release);
	return 0;
}

static void bgm_shutdown(struct platform_device *pdev)
{
	struct bgm_dev *dev = platform_get_drvdata(pdev);

	mutex_lock(&dev->lock);
	bgm_disconnect_hw(dev);
	mutex_unlock(&dev->lock);
}

static int bgm_suspend(struct device *dev)
{
	/* No state save/reset sequence: refuse to suspend live DMA. */
	return -EOPNOTSUPP;
}

static const struct dev_pm_ops bgm_pm_ops = {
	.suspend = bgm_suspend,
	.freeze = bgm_suspend,
	.poweroff = bgm_suspend,
};

static const struct of_device_id bgm_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-bgm" },
	{ }
};
MODULE_DEVICE_TABLE(of, bgm_of_match);

static struct platform_driver bgm_driver = {
	.probe = bgm_probe,
	.remove = bgm_remove,
	.shutdown = bgm_shutdown,
	.driver = {
		.name = BGM_NAME,
		.of_match_table = bgm_of_match,
		.pm = &bgm_pm_ops,
	},
};
module_platform_driver(bgm_driver);

MODULE_DESCRIPTION("FH8626V100 V4L2 foreground mask processor");
MODULE_LICENSE("GPL");
