// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 logo windows, exposed as nine V4L2 video output nodes
 *
 * Node 0 uses input frame coordinates and covers every channel; privacy
 * masks share this window. Nodes 1..4 use channel 0 coordinates and nodes
 * 5..8 use channel 1 coordinates. V4L2_CID_ALPHA_COMPONENT sets window
 * opacity in addition to per-pixel alpha.
 *
 * Copyright (C) 2026 Difan Zhang
 */
#include <linux/dma-mapping.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

#include "fh8626-pipeline.h"
#include "fh8626-event.h"

struct fh8626_logo_fmt {
	u32 fourcc;
	u8 depth;
	u8 layout;
	u8 bpp;
	const char *name;
};

/*
 * Hardware pixel depth code (4 for 16-bit pixels, 5 for 32-bit pixels) and
 * 16-bit layout code. Pixels with alpha are blended by their own alpha;
 * the 32-bit layout always carries alpha, so there is no XRGB format.
 */
static const struct fh8626_logo_fmt fh8626_logo_fmts[] = {
	{ V4L2_PIX_FMT_RGB565, 4, 1, 2, "RGB565" },
	{ V4L2_PIX_FMT_ARGB555, 4, 2, 2, "ARGB1555" },
	{ V4L2_PIX_FMT_ARGB444, 4, 3, 2, "ARGB4444" },
	{ V4L2_PIX_FMT_ARGB32, 5, 0, 4, "ARGB8888" },
};

struct fh8626_logo_dev;

struct fh8626_logo_buf {
	struct vb2_v4l2_buffer vb;
	struct list_head node;
};

struct fh8626_logo_slot {
	struct video_device vdev;
	struct vb2_queue queue;
	struct v4l2_ctrl_handler ctrls;
	struct mutex lock;
	struct fh8626_logo_dev *logos;
	struct list_head pending;
	struct v4l2_pix_format pix;
	struct v4l2_rect compose;
	unsigned int index;
	u8 alpha;
	/*
	 * Write the bitmap opposite @shown while the hardware scans out @shown.
	 * An address written at @flip_seq takes effect within two frame starts.
	 */
	void *cpu[2];
	dma_addr_t dma[2];
	size_t dma_size;
	unsigned int shown;
	u32 flip_seq;
	bool flip_live;
	bool failed;
	bool off_pending;
	bool streaming;
	bool queue_inited;
	bool registered;
	bool ref_held;
};

struct fh8626_logo_dev {
	struct device *dma_dev;
	struct fh8626_pipeline *pipe;
	struct kref ref;
	struct fh8626_logo_slot *slot[FH8626_LOGO_SLOTS];
};

static const struct fh8626_logo_fmt *logo_fmt(u32 fourcc)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(fh8626_logo_fmts); ++i)
		if (fh8626_logo_fmts[i].fourcc == fourcc)
			return &fh8626_logo_fmts[i];
	return NULL;
}

static struct fh8626_logo_buf *logo_buf(struct vb2_buffer *vb)
{
	return container_of(to_vb2_v4l2_buffer(vb), struct fh8626_logo_buf,
			    vb);
}

static void logo_dev_free(struct kref *ref)
{
	kfree(container_of(ref, struct fh8626_logo_dev, ref));
}

static void logo_frame(struct fh8626_logo_slot *slot, u32 *width, u32 *height)
{
	struct fh8626_vpu_dev *vpu = NULL;

	if (slot->logos && slot->logos->pipe)
		vpu = slot->logos->pipe->vpu;
	fh8626_vpu_logo_bounds(vpu, slot->index, width, height);
}

static void logo_try_pix(struct fh8626_logo_slot *slot,
			 struct v4l2_pix_format *pix)
{
	const struct fh8626_logo_fmt *fmt = logo_fmt(pix->pixelformat);
	u32 lim_w, lim_h, min_bpl, bpl;

	logo_frame(slot, &lim_w, &lim_h);
	if (!fmt)
		fmt = &fh8626_logo_fmts[0];
	if (!pix->width)
		pix->width = 1;
	if (!pix->height)
		pix->height = 1;
	if (pix->width > lim_w)
		pix->width = lim_w;
	if (pix->height > lim_h)
		pix->height = lim_h;
	min_bpl = round_up(pix->width * fmt->bpp, 8);
	bpl = clamp_t(u32, round_down(pix->bytesperline, 8), min_bpl, 0xfff8);
	pix->pixelformat = fmt->fourcc;
	pix->bytesperline = bpl;
	pix->sizeimage = bpl * pix->height;
	pix->field = V4L2_FIELD_NONE;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->xfer_func = V4L2_XFER_FUNC_SRGB;
	pix->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	pix->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	pix->flags = 0;
	pix->priv = 0;
}

static int logo_program(struct fh8626_logo_slot *slot,
			const struct fh8626_logo_plane *plane)
{
	struct fh8626_vpu_dev *vpu = slot->logos->pipe->vpu;
	int ret;

	if (plane->on && slot->failed)
		return -EIO;
	ret = fh8626_vpu_logo_program(vpu, slot->index, plane,
				     &slot->flip_seq, &slot->flip_live);
	return ret;
}

/*
 * Wait up to two frame starts for the bitmap switch. A timeout does not
 * establish that DMA has stopped.
 */
static int logo_wait_flip(struct fh8626_logo_slot *slot)
{
	int ret;

	if (!slot->flip_live)
		return 0;
	ret = fh8626_vpu_wait_frame_starts(slot->logos->pipe->vpu,
					 slot->flip_seq, 2);
	if (ret) {
		slot->failed = true;
		return ret;
	}
	slot->flip_live = false;
	return 0;
}

static void logo_free_bitmaps(struct fh8626_logo_slot *slot, void *cpu[2],
			      dma_addr_t dma[2], size_t size)
{
	unsigned int i;

	for (i = 0; i < 2; ++i)
		if (cpu[i])
			dma_free_coherent(slot->logos->dma_dev, size, cpu[i],
					  dma[i]);
}

static int logo_hw_off(struct fh8626_logo_slot *slot)
{
	struct fh8626_logo_plane plane = { };
	struct fh8626_logo_dev *logos = slot->logos;
	int ret;

	if (!slot->cpu[0])
		return 0;
	if (!logos || !logos->pipe || !logos->pipe->vpu)
		return -ENODEV;
	if (!slot->off_pending) {
		ret = logo_program(slot, &plane);
		if (ret)
			return ret;
		slot->off_pending = true;
	}
	ret = logo_wait_flip(slot);
	if (ret) {
		dev_warn_ratelimited(logos->dma_dev,
			"logo %u disable unconfirmed; retaining bitmaps\n",
			slot->index);
		return ret;
	}
	logo_free_bitmaps(slot, slot->cpu, slot->dma, slot->dma_size);
	slot->cpu[0] = slot->cpu[1] = NULL;
	slot->dma[0] = slot->dma[1] = 0;
	slot->dma_size = 0;
	slot->off_pending = false;
	slot->failed = false;
	return 0;
}

static void logo_plane(struct fh8626_logo_slot *slot,
		       const struct fh8626_logo_fmt *fmt, dma_addr_t dma,
		       struct fh8626_logo_plane *plane)
{
	plane->phys = dma;
	plane->width = slot->pix.width;
	plane->height = slot->pix.height;
	plane->stride = slot->pix.bytesperline;
	plane->x = slot->compose.left;
	plane->y = slot->compose.top;
	plane->depth = fmt->depth;
	plane->layout = fmt->layout;
	plane->alpha = slot->alpha;
	plane->on = true;
}

static int logo_snapshot(struct fh8626_logo_slot *slot,
			 struct fh8626_logo_buf *buf)
{
	const struct fh8626_logo_fmt *fmt = logo_fmt(slot->pix.pixelformat);
	struct fh8626_logo_plane plane = { };
	struct fh8626_logo_dev *logos = slot->logos;
	void *cpu[2] = { };
	dma_addr_t dma[2];
	unsigned int i, next;
	size_t size;
	void *src;
	int ret;

	if (!fmt || !logos || !logos->pipe || !logos->pipe->vpu)
		return -ENODEV;
	if (slot->failed || slot->off_pending)
		return -EIO;
	if (slot->compose.left < 0 || slot->compose.top < 0)
		return -ERANGE;
	size = slot->pix.sizeimage;
	src = vb2_plane_vaddr(&buf->vb.vb2_buf, 0);
	if (!src || !size)
		return -EINVAL;

	if (slot->dma_size == size) {
		/* The bitmap not on screen is free once the last flip is in */
		ret = logo_wait_flip(slot);
		if (ret)
			return ret;
		next = slot->shown ^ 1;
		memcpy(slot->cpu[next], src, size);
		logo_plane(slot, fmt, slot->dma[next], &plane);
		ret = logo_program(slot, &plane);
		if (!ret)
			slot->shown = next;
		return ret;
	}

	/* Retire the old allocation before publishing a differently sized one. */
	ret = logo_hw_off(slot);
	if (ret)
		return ret;
	for (i = 0; i < 2; ++i) {
		cpu[i] = dma_alloc_coherent(logos->dma_dev, size, &dma[i],
					    GFP_KERNEL);
		if (!cpu[i]) {
			logo_free_bitmaps(slot, cpu, dma, size);
			return -ENOMEM;
		}
	}
	memcpy(cpu[0], src, size);
	logo_plane(slot, fmt, dma[0], &plane);
	ret = logo_program(slot, &plane);
	if (ret) {
		logo_free_bitmaps(slot, cpu, dma, size);
		return ret;
	}
	memcpy(slot->cpu, cpu, sizeof(cpu));
	memcpy(slot->dma, dma, sizeof(dma));
	slot->dma_size = size;
	slot->shown = 0;
	return 0;
}

static void logo_return(struct fh8626_logo_slot *slot,
			enum vb2_buffer_state state)
{
	struct fh8626_logo_buf *buf, *tmp;

	list_for_each_entry_safe(buf, tmp, &slot->pending, node) {
		list_del(&buf->node);
		vb2_buffer_done(&buf->vb.vb2_buf, state);
	}
}

static int logo_queue_setup(struct vb2_queue *q, unsigned int *nbuffers,
			    unsigned int *nplanes, unsigned int sizes[],
			    struct device *alloc_devs[])
{
	struct fh8626_logo_slot *slot = vb2_get_drv_priv(q);
	unsigned int size = slot->pix.sizeimage;

	if (*nplanes)
		return *nplanes != 1 || sizes[0] < size ? -EINVAL : 0;
	*nbuffers = clamp_t(unsigned int, *nbuffers, 1, 2);
	*nplanes = 1;
	sizes[0] = size;
	return 0;
}

static int logo_buf_prepare(struct vb2_buffer *vb)
{
	struct fh8626_logo_slot *slot = vb2_get_drv_priv(vb->vb2_queue);

	if (vb2_plane_size(vb, 0) < slot->pix.sizeimage)
		return -EINVAL;
	vb2_set_plane_payload(vb, 0, slot->pix.sizeimage);
	return 0;
}

static void logo_buf_queue(struct vb2_buffer *vb)
{
	struct fh8626_logo_slot *slot = vb2_get_drv_priv(vb->vb2_queue);
	struct fh8626_logo_buf *buf = logo_buf(vb);
	int ret;

	list_add_tail(&buf->node, &slot->pending);
	if (!slot->streaming)
		return;
	ret = logo_snapshot(slot, buf);
	if (slot->failed)
		vb2_queue_error(&slot->queue);
	list_del(&buf->node);
	vb2_buffer_done(vb, ret ? VB2_BUF_STATE_ERROR : VB2_BUF_STATE_DONE);
}

static int logo_start(struct vb2_queue *q, unsigned int count)
{
	struct fh8626_logo_slot *slot = vb2_get_drv_priv(q);
	struct fh8626_logo_buf *last;
	int ret;

	if (!count || list_empty(&slot->pending))
		return -EINVAL;
	/* STREAMON retries a failed disable without discarding its DMA owner. */
	if (slot->failed || slot->off_pending) {
		ret = logo_hw_off(slot);
		if (ret) {
			logo_return(slot, VB2_BUF_STATE_QUEUED);
			return ret;
		}
	}
	/* The hardware keeps one bitmap; show the last buffer queued */
	last = list_last_entry(&slot->pending, struct fh8626_logo_buf,
			       node);
	ret = logo_snapshot(slot, last);
	if (ret) {
		logo_hw_off(slot);
		logo_return(slot, VB2_BUF_STATE_QUEUED);
		return ret;
	}
	logo_return(slot, VB2_BUF_STATE_DONE);
	slot->streaming = true;
	return 0;
}

static void logo_stop(struct vb2_queue *q)
{
	struct fh8626_logo_slot *slot = vb2_get_drv_priv(q);

	slot->streaming = false;
	logo_hw_off(slot);
	logo_return(slot, VB2_BUF_STATE_ERROR);
}

static const struct vb2_ops logo_qops = {
	.queue_setup = logo_queue_setup,
	.buf_prepare = logo_buf_prepare,
	.buf_queue = logo_buf_queue,
	.start_streaming = logo_start,
	.stop_streaming = logo_stop,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

static int logo_querycap(struct file *file, void *priv,
			 struct v4l2_capability *cap)
{
	struct fh8626_logo_slot *slot = video_drvdata(file);
	unsigned int chan = fh8626_vpu_logo_channel(slot->index);

	strlcpy(cap->driver, "fh8626-v4l2", sizeof(cap->driver));
	if (chan < FH8626_VPU_CHANNELS)
		snprintf(cap->card, sizeof(cap->card), "FH8626 logo channel %u",
			 chan);
	else
		strlcpy(cap->card, "FH8626 logo all channels",
			sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
		 "platform:fh8626-logo%u", slot->index);
	cap->device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int logo_enum_fmt(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	if (f->index >= ARRAY_SIZE(fh8626_logo_fmts))
		return -EINVAL;
	f->pixelformat = fh8626_logo_fmts[f->index].fourcc;
	strlcpy(f->description, fh8626_logo_fmts[f->index].name,
		sizeof(f->description));
	return 0;
}

static int logo_enum_output(struct file *file, void *priv,
			    struct v4l2_output *out)
{
	if (out->index)
		return -EINVAL;
	strlcpy(out->name, "Logo", sizeof(out->name));
	out->type = V4L2_OUTPUT_TYPE_ANALOGVGAOVERLAY;
	return 0;
}

static int logo_g_output(struct file *file, void *priv, unsigned int *index)
{
	*index = 0;
	return 0;
}

static int logo_s_output(struct file *file, void *priv, unsigned int index)
{
	return index ? -EINVAL : 0;
}

static int logo_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct fh8626_logo_slot *slot = video_drvdata(file);

	f->fmt.pix = slot->pix;
	return 0;
}

static int logo_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct fh8626_logo_slot *slot = video_drvdata(file);

	logo_try_pix(slot, &f->fmt.pix);
	return 0;
}

static int logo_s_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct fh8626_logo_slot *slot = video_drvdata(file);
	u32 lim_w, lim_h;

	if (vb2_is_busy(&slot->queue))
		return -EBUSY;
	logo_try_pix(slot, &f->fmt.pix);
	slot->pix = f->fmt.pix;
	slot->compose.width = slot->pix.width;
	slot->compose.height = slot->pix.height;
	logo_frame(slot, &lim_w, &lim_h);
	if (slot->compose.left < 0 || slot->compose.top < 0 ||
	    (u32)slot->compose.left > lim_w - slot->pix.width ||
	    (u32)slot->compose.top > lim_h - slot->pix.height) {
		slot->compose.left = 0;
		slot->compose.top = 0;
	}
	return 0;
}

static int logo_g_selection(struct file *file, void *priv,
			    struct v4l2_selection *sel)
{
	struct fh8626_logo_slot *slot = video_drvdata(file);
	u32 lim_w, lim_h;

	if (sel->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;
	switch (sel->target) {
	case V4L2_SEL_TGT_COMPOSE:
		sel->r = slot->compose;
		return 0;
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = slot->pix.width;
		sel->r.height = slot->pix.height;
		return 0;
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		logo_frame(slot, &lim_w, &lim_h);
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = lim_w;
		sel->r.height = lim_h;
		return 0;
	default:
		return -EINVAL;
	}
}

static int logo_reprogram(struct fh8626_logo_slot *slot)
{
	const struct fh8626_logo_fmt *fmt = logo_fmt(slot->pix.pixelformat);
	struct fh8626_logo_plane plane = { };
	struct fh8626_logo_dev *logos = slot->logos;

	if (!slot->cpu[0] || !fmt || !logos || !logos->pipe ||
	    !logos->pipe->vpu)
		return -ENODEV;
	if (slot->compose.left < 0 || slot->compose.top < 0)
		return -ERANGE;
	logo_plane(slot, fmt, slot->dma[slot->shown], &plane);
	return logo_program(slot, &plane);
}

static int logo_s_selection(struct file *file, void *priv,
			    struct v4l2_selection *sel)
{
	struct fh8626_logo_slot *slot = video_drvdata(file);
	struct v4l2_rect old;
	u32 lim_w, lim_h;
	int ret;

	if (sel->type != V4L2_BUF_TYPE_VIDEO_OUTPUT ||
	    sel->target != V4L2_SEL_TGT_COMPOSE)
		return -EINVAL;
	if (sel->r.width != slot->pix.width ||
	    sel->r.height != slot->pix.height)
		return vb2_is_streaming(&slot->queue) ? -EBUSY : -EINVAL;
	if (sel->r.left < 0 || sel->r.top < 0)
		return -ERANGE;
	logo_frame(slot, &lim_w, &lim_h);
	if ((u32)sel->r.left > lim_w - sel->r.width ||
	    (u32)sel->r.top > lim_h - sel->r.height)
		return -ERANGE;
	old = slot->compose;
	slot->compose = sel->r;
	if (!slot->streaming || !slot->cpu[0])
		return 0;
	ret = logo_reprogram(slot);
	if (ret)
		slot->compose = old;
	return ret;
}

static int logo_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct fh8626_logo_slot *slot =
		container_of(ctrl->handler, struct fh8626_logo_slot, ctrls);
	u8 old = slot->alpha;
	int ret;

	if (ctrl->id != V4L2_CID_ALPHA_COMPONENT)
		return -EINVAL;
	slot->alpha = ctrl->val;
	if (!slot->streaming || !slot->cpu[0])
		return 0;
	ret = logo_reprogram(slot);
	if (ret)
		slot->alpha = old;
	return ret;
}

static const struct v4l2_ctrl_ops logo_ctrl_ops = {
	.s_ctrl = logo_s_ctrl,
};

static const struct v4l2_ioctl_ops logo_ioctl_ops = {
	.vidioc_querycap = logo_querycap,
	.vidioc_enum_fmt_vid_out = logo_enum_fmt,
	.vidioc_g_fmt_vid_out = logo_g_fmt,
	.vidioc_s_fmt_vid_out = logo_s_fmt,
	.vidioc_try_fmt_vid_out = logo_try_fmt,
	.vidioc_enum_output = logo_enum_output,
	.vidioc_g_output = logo_g_output,
	.vidioc_s_output = logo_s_output,
	.vidioc_g_selection = logo_g_selection,
	.vidioc_s_selection = logo_s_selection,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static int logo_open(struct file *file)
{
	struct fh8626_logo_slot *slot = video_drvdata(file);
	int ret;

	if (mutex_lock_interruptible(&slot->lock))
		return -ERESTARTSYS;
	ret = v4l2_fh_open(file);
	mutex_unlock(&slot->lock);
	return ret;
}

static const struct v4l2_file_operations logo_fops = {
	.owner = THIS_MODULE,
	.open = logo_open,
	.release = vb2_fop_release,
	.poll = vb2_fop_poll,
	.mmap = vb2_fop_mmap,
	.unlocked_ioctl = fh8626_video_ioctl2,
};

static void logo_vdev_release(struct video_device *vd)
{
	struct fh8626_logo_slot *slot =
		container_of(vd, struct fh8626_logo_slot, vdev);
	struct fh8626_logo_dev *logos = slot->logos;
	unsigned int index = slot->index;

	if (slot->queue_inited)
		vb2_queue_release(&slot->queue);
	v4l2_ctrl_handler_free(&slot->ctrls);
	mutex_destroy(&slot->lock);
	if (slot->cpu[0]) {
		/* No reset/idle proof: keep DMA memory and its owner permanently. */
		dev_err(logos->dma_dev,
			"logo %u removed with unconfirmed DMA; retaining bitmaps and owner\n",
			index);
		get_device(logos->dma_dev);
		return;
	}
	logos->slot[index] = NULL;
	kfree(slot);
	kref_put(&logos->ref, logo_dev_free);
}

static void logo_slot_abort(struct fh8626_logo_slot *slot)
{
	struct fh8626_logo_dev *logos = slot->logos;
	unsigned int index = slot->index;
	bool held = slot->ref_held;

	logo_hw_off(slot);
	if (slot->queue_inited) {
		vb2_queue_release(&slot->queue);
		slot->queue_inited = false;
	}
	v4l2_ctrl_handler_free(&slot->ctrls);
	mutex_destroy(&slot->lock);
	if (logos)
		logos->slot[index] = NULL;
	kfree(slot);
	if (held && logos)
		kref_put(&logos->ref, logo_dev_free);
}

static int logo_slot_add(struct fh8626_logo_dev *logos, unsigned int index)
{
	struct fh8626_logo_slot *slot;
	struct video_device *vd;
	struct vb2_queue *q;
	struct v4l2_pix_format pix = {
		.width = 64,
		.height = 32,
		.pixelformat = V4L2_PIX_FMT_RGB565,
	};
	int ret;

	slot = kzalloc(sizeof(*slot), GFP_KERNEL);
	if (!slot)
		return -ENOMEM;
	slot->logos = logos;
	slot->index = index;
	mutex_init(&slot->lock);
	INIT_LIST_HEAD(&slot->pending);
	logos->slot[index] = slot;
	logo_try_pix(slot, &pix);
	slot->pix = pix;
	slot->compose.width = pix.width;
	slot->compose.height = pix.height;
	slot->alpha = 255;

	/* Control ioctls run under slot->lock, which serialises s_ctrl */
	v4l2_ctrl_handler_init(&slot->ctrls, 1);
	v4l2_ctrl_new_std(&slot->ctrls, &logo_ctrl_ops,
			  V4L2_CID_ALPHA_COMPONENT, 0, 255, 1, 255);
	if (slot->ctrls.error) {
		ret = slot->ctrls.error;
		logo_slot_abort(slot);
		return ret;
	}

	q = &slot->queue;
	q->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	q->io_modes = VB2_MMAP;
	q->drv_priv = slot;
	q->buf_struct_size = sizeof(struct fh8626_logo_buf);
	q->ops = &logo_qops;
	q->mem_ops = &vb2_vmalloc_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_buffers_needed = 1;
	q->lock = &slot->lock;
	q->dev = logos->dma_dev;
	ret = vb2_queue_init(q);
	if (ret) {
		logo_slot_abort(slot);
		return ret;
	}
	slot->queue_inited = true;

	vd = &slot->vdev;
	vd->fops = &logo_fops;
	vd->ioctl_ops = &logo_ioctl_ops;
	vd->release = logo_vdev_release;
	vd->v4l2_dev = &logos->pipe->v4l2_dev;
	vd->queue = q;
	vd->ctrl_handler = &slot->ctrls;
	vd->lock = &slot->lock;
	vd->vfl_dir = VFL_DIR_TX;
	vd->device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
	snprintf(vd->name, sizeof(vd->name), "fh8626-logo%u", index);
	video_set_drvdata(vd, slot);

	kref_get(&logos->ref);
	slot->ref_held = true;
	ret = video_register_device(vd, VFL_TYPE_GRABBER, -1);
	if (ret) {
		dev_err(logos->dma_dev, "failed to register logo%u: %d\n",
			index, ret);
		if (logos->slot[index])
			logo_slot_abort(slot);
		return ret;
	}
	slot->registered = true;
#ifdef CONFIG_MEDIA_CONTROLLER
	if (!vd->intf_devnode)
		return -ENOMEM;
#endif
	return 0;
}

int fh8626_logo_register(struct platform_device *pdev,
			 struct fh8626_pipeline *pipe)
{
	struct fh8626_logo_dev *logos;
	unsigned int i;
	int ret;

	if (!pdev || !pipe || !pipe->vpu || pipe->logos)
		return -EINVAL;
	if (!fh8626_vpu_device(pipe->vpu))
		return -ENODEV;
	logos = kzalloc(sizeof(*logos), GFP_KERNEL);
	if (!logos)
		return -ENOMEM;
	kref_init(&logos->ref);
	logos->dma_dev = fh8626_vpu_device(pipe->vpu);
	logos->pipe = pipe;
	pipe->logos = logos;
	for (i = 0; i < FH8626_LOGO_SLOTS; ++i) {
		ret = logo_slot_add(logos, i);
		if (ret) {
			fh8626_logo_unregister(pipe);
			return ret;
		}
	}
	return 0;
}

void fh8626_logo_unregister(struct fh8626_pipeline *pipe)
{
	struct fh8626_logo_dev *logos;
	unsigned int i;

	if (!pipe || !pipe->logos)
		return;
	logos = pipe->logos;
	pipe->logos = NULL;
	for (i = 0; i < FH8626_LOGO_SLOTS; ++i) {
		struct fh8626_logo_slot *slot = logos->slot[i];

		if (!slot)
			continue;
		if (!slot->registered) {
			logo_slot_abort(slot);
			continue;
		}
		get_device(&slot->vdev.dev);
		mutex_lock(&slot->lock);
		fh8626_video_withdraw_interface(&slot->vdev);
		video_unregister_device(&slot->vdev);
		fh8626_video_wake_events(&slot->vdev);
		logo_hw_off(slot);
		if (slot->queue_inited) {
			vb2_queue_error(&slot->queue);
			vb2_queue_release(&slot->queue);
			slot->queue.owner = NULL;
			slot->queue_inited = false;
		}
		mutex_unlock(&slot->lock);
		put_device(&slot->vdev.dev);
	}
	logos->pipe = NULL;
	kref_put(&logos->ref, logo_dev_free);
}
