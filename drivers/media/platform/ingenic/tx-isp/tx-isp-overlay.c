// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang
/*
 * Privacy masks, vector lines and on-screen display drawn by the T23 ISP
 * output scaler.
 *
 * Each input has four mask rectangles and six vector lines, set through
 * array controls on its capture nodes in sensor pixels; masks scale to
 * every output while lines are drawn on MSCA channel 0. Each input also has
 * eight OSD regions, each a video output node: the last queued bitmap is
 * blended into the main output at the compose rectangle on every frame and
 * is returned once a newer buffer replaces it. The hardware draws the OSD
 * on MSCA channel 0 only, so it does not reach the scaled outputs, and
 * takes one pixel format for all regions of a frame. Overlapping blocks
 * corrupt the overlap and every later block, so a region that overlaps a
 * lower-numbered one is not drawn.
 */
#include <linux/dma-mapping.h>
#include <linux/media/ingenic/t23-isp-config.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-contig.h>

#include "tx-isp-overlay.h"

/* Largest frame the MSCA geometry registers describe */
#define T23_OSD_MAX_SIZE	4095

struct t23_osd_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
};

static inline struct t23_osd_buffer *to_t23_osd_buffer(struct vb2_buffer *vb)
{
	return container_of(to_vb2_v4l2_buffer(vb), struct t23_osd_buffer, vb);
}

static const struct {
	u32 fourcc;
	u8 bpp;
} t23_osd_formats[] = {
	{ V4L2_PIX_FMT_ABGR32, 4 },
	{ V4L2_PIX_FMT_XBGR32, 4 },
	{ V4L2_PIX_FMT_ARGB32, 4 },
	{ V4L2_PIX_FMT_XRGB32, 4 },
	{ V4L2_PIX_FMT_ARGB555, 2 },
	{ V4L2_PIX_FMT_XRGB555, 2 },
};

/* Full-range BT.601, as the capture nodes report */
static u32 t23_mask_yuv(u32 rgb)
{
	int r = rgb >> 16 & 0xff, g = rgb >> 8 & 0xff, b = rgb & 0xff;
	int y = (77 * r + 150 * g + 29 * b + 128) >> 8;
	int u = ((-43 * r - 85 * g + 128 * b + 128) >> 8) + 128;
	int v = ((128 * r - 107 * g - 21 * b + 128) >> 8) + 128;

	return FIELD_PREP(MSCA_MASK_COLOR_Y, clamp(y, 0, 255)) |
	       FIELD_PREP(MSCA_MASK_COLOR_U, clamp(u, 0, 255)) |
	       FIELD_PREP(MSCA_MASK_COLOR_V, clamp(v, 0, 255));
}

static int t23_mask_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct t23_overlay_input *in = ctrl->priv;
	struct t23_overlay *overlay = in->overlay;
	const u16 *rect = ctrl->p_new.p_u16;
	unsigned long flags;
	unsigned int i;

	spin_lock_irqsave(&overlay->lock, flags);
	switch (ctrl->id) {
	case V4L2_CID_T23_PRIVACY_MASK:
		for (i = 0; i < T23_MSCA_MASKS; i++, rect += 4)
			in->mask[i] = (struct tx_isp_rect){
				rect[0], rect[1], rect[2], rect[3]
			};
		break;
	case V4L2_CID_T23_PRIVACY_MASK_COLOR:
		in->mask_yuv = t23_mask_yuv(ctrl->val);
		break;
	case V4L2_CID_T23_DRAW_LINE:
		for (i = 0; i < T23_MSCA_LINES; i++, rect += 5)
			in->line[i] = (struct tx_isp_line){
				rect[0], rect[1], rect[2], rect[3], rect[4]
			};
		break;
	case V4L2_CID_T23_DRAW_LINE_COLOR:
		in->line_yuv = t23_mask_yuv(ctrl->val);
		break;
	}
	spin_unlock_irqrestore(&overlay->lock, flags);
	return 0;
}

static const struct v4l2_ctrl_ops t23_mask_ops = {
	.s_ctrl = t23_mask_s_ctrl,
};

static const struct v4l2_ctrl_config t23_mask_rects = {
	.ops = &t23_mask_ops,
	.id = V4L2_CID_T23_PRIVACY_MASK,
	.name = "Privacy Mask",
	.type = V4L2_CTRL_TYPE_U16,
	.dims = { T23_ISP_PRIVACY_MASKS, 4 },
	.max = T23_OSD_MAX_SIZE,
	.step = 1,
};

static const struct v4l2_ctrl_config t23_mask_color = {
	.ops = &t23_mask_ops,
	.id = V4L2_CID_T23_PRIVACY_MASK_COLOR,
	.name = "Privacy Mask Color",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.max = 0xffffff,
	.step = 1,
};

static const struct v4l2_ctrl_config t23_draw_lines = {
	.ops = &t23_mask_ops,
	.id = V4L2_CID_T23_DRAW_LINE,
	.name = "Draw Line",
	.type = V4L2_CTRL_TYPE_U16,
	.dims = { T23_ISP_DRAW_LINES, 5 },
	.max = T23_OSD_MAX_SIZE,
	.step = 1,
};

static const struct v4l2_ctrl_config t23_draw_line_color = {
	.ops = &t23_mask_ops,
	.id = V4L2_CID_T23_DRAW_LINE_COLOR,
	.name = "Draw Line Color",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.max = 0xffffff,
	.step = 1,
};

void t23_overlay_init(struct t23_overlay *overlay, struct device *dev,
		      bool (*sync)(struct t23_overlay *, unsigned int))
{
	unsigned int i, j;

	BUILD_BUG_ON(T23_ISP_PRIVACY_MASKS != T23_MSCA_MASKS);
	BUILD_BUG_ON(T23_ISP_DRAW_LINES != T23_MSCA_LINES);
	BUILD_BUG_ON(T23_MSCA_OSD_REGIONS != MSCA_OSD_BLOCKS);
	overlay->dev = dev;
	overlay->sync = sync;
	spin_lock_init(&overlay->lock);
	for (i = 0; i < T23_OVERLAY_INPUTS; i++) {
		struct t23_overlay_input *in = &overlay->inputs[i];

		in->overlay = overlay;
		in->index = i;
		in->mask_yuv = t23_mask_yuv(0);
		in->line_yuv = t23_mask_yuv(0);
		for (j = 0; j < T23_MSCA_OSD_REGIONS; j++) {
			in->osd[j].input = in;
			in->osd[j].index = j;
			INIT_LIST_HEAD(&in->osd[j].bufs);
		}
	}
}

void t23_overlay_init_controls(struct t23_overlay *overlay, unsigned int input,
			       struct v4l2_ctrl_handler *hdl)
{
	static const struct v4l2_ctrl_config * const cfgs[] = {
		&t23_mask_rects,
		&t23_mask_color,
		&t23_draw_lines,
		&t23_draw_line_color,
	};
	struct v4l2_ctrl *ctrl;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(cfgs); i++) {
		ctrl = v4l2_ctrl_new_custom(hdl, cfgs[i], NULL);
		if (ctrl)
			ctrl->priv = &overlay->inputs[input];
	}
}

static int t23_osd_bpp(u32 fourcc)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(t23_osd_formats); i++)
		if (t23_osd_formats[i].fourcc == fourcc)
			return t23_osd_formats[i].bpp;
	return 0;
}

/*
 * Even sizes and positions keep the bitmap on whole NV12 chroma samples.
 * Lines start on 8-byte boundaries.
 */
static void t23_osd_try_format(const struct t23_overlay_input *in,
			       struct v4l2_pix_format *pix)
{
	u32 max_width = in->width ?: T23_OSD_MAX_SIZE;
	u32 max_height = in->height ?: T23_OSD_MAX_SIZE;
	int bpp = t23_osd_bpp(pix->pixelformat);

	if (!bpp) {
		pix->pixelformat = t23_osd_formats[0].fourcc;
		bpp = t23_osd_formats[0].bpp;
	}
	pix->width = clamp_t(u32, pix->width, 2, max_width) & ~1U;
	pix->height = clamp_t(u32, pix->height, 2, max_height) & ~1U;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = ALIGN(pix->width * bpp, 8);
	pix->sizeimage = pix->bytesperline * pix->height;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->xfer_func = V4L2_XFER_FUNC_DEFAULT;
	pix->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	pix->quantization = V4L2_QUANTIZATION_DEFAULT;
	pix->flags = 0;
	pix->priv = 0;
}

/* Keep the bitmap inside the frame; called with the lock held. */
static void t23_osd_clamp_position(struct t23_osd_region *rg)
{
	u32 width = rg->input->width ?: T23_OSD_MAX_SIZE;
	u32 height = rg->input->height ?: T23_OSD_MAX_SIZE;

	rg->left = min_t(u32, rg->left, (width - min(width, rg->pix.width)) & ~1U);
	rg->top = min_t(u32, rg->top, (height - min(height, rg->pix.height)) & ~1U);
}

void t23_overlay_set_frame(struct t23_overlay *overlay, unsigned int input,
			   u32 width, u32 height)
{
	struct t23_overlay_input *in = &overlay->inputs[input];
	unsigned long flags;
	unsigned int i;

	spin_lock_irqsave(&overlay->lock, flags);
	in->width = width;
	in->height = height;
	for (i = 0; i < T23_MSCA_OSD_REGIONS; i++)
		t23_osd_clamp_position(&in->osd[i]);
	spin_unlock_irqrestore(&overlay->lock, flags);
}

static bool t23_osd_overlaps(const struct tx_isp_rect *a,
			     const struct tx_isp_rect *b)
{
	return a->left < b->left + b->width && b->left < a->left + a->width &&
	       a->top < b->top + b->height && b->top < a->top + a->height;
}

void t23_overlay_frame(struct t23_overlay *overlay, unsigned int input,
		       struct tx_isp_msca_overlay *out)
{
	struct t23_overlay_input *in = &overlay->inputs[input];
	struct t23_osd_buffer *buf;
	unsigned long flags;
	unsigned int i, j;

	memset(out, 0, sizeof(*out));
	spin_lock_irqsave(&overlay->lock, flags);
	memcpy(out->mask, in->mask, sizeof(out->mask));
	out->mask_yuv = in->mask_yuv;
	memcpy(out->line, in->line, sizeof(out->line));
	out->line_yuv = in->line_yuv;
	for (i = 0; i < T23_MSCA_OSD_REGIONS; i++) {
		struct t23_osd_region *rg = &in->osd[i];

		if (!rg->streaming)
			continue;
		buf = list_first_entry_or_null(&rg->bufs, struct t23_osd_buffer,
					       list);
		if (buf) {
			list_del(&buf->list);
			if (rg->shown)
				vb2_buffer_done(&rg->shown->vb.vb2_buf,
						VB2_BUF_STATE_DONE);
			rg->shown = buf;
		}
		if (!rg->shown || rg->left + rg->pix.width > in->width ||
		    rg->top + rg->pix.height > in->height)
			continue;
		out->osd[i].rect = (struct tx_isp_rect){
			rg->left, rg->top, rg->pix.width, rg->pix.height
		};
		for (j = 0; j < i; j++)
			if (out->osd[j].addr &&
			    t23_osd_overlaps(&out->osd[i].rect, &out->osd[j].rect))
				break;
		if (j < i)
			continue;
		out->osd[i].addr =
			vb2_dma_contig_plane_dma_addr(&rg->shown->vb.vb2_buf, 0);
		out->osd[i].stride = rg->pix.bytesperline;
		/* Streaming regions of an input share one format. */
		out->osd_format = rg->pix.pixelformat;
	}
	spin_unlock_irqrestore(&overlay->lock, flags);
}

static int t23_osd_queue_setup(struct vb2_queue *q, unsigned int *nbuffers,
			       unsigned int *nplanes, unsigned int sizes[],
			       struct device *alloc_devs[])
{
	struct t23_osd_region *rg = vb2_get_drv_priv(q);

	if (*nplanes)
		return *nplanes == 1 && sizes[0] >= rg->pix.sizeimage ?
		       0 : -EINVAL;
	*nplanes = 1;
	sizes[0] = rg->pix.sizeimage;
	return 0;
}

static int t23_osd_buf_prepare(struct vb2_buffer *vb)
{
	struct t23_osd_region *rg = vb2_get_drv_priv(vb->vb2_queue);
	dma_addr_t addr = vb2_dma_contig_plane_dma_addr(vb, 0);
	u32 size = rg->pix.sizeimage;

	if (vb2_plane_size(vb, 0) < size ||
	    vb2_get_plane_payload(vb, 0) < size || addr & 7)
		return -EINVAL;
	return 0;
}

static void t23_osd_buf_queue(struct vb2_buffer *vb)
{
	struct t23_osd_region *rg = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long flags;

	spin_lock_irqsave(&rg->input->overlay->lock, flags);
	list_add_tail(&to_t23_osd_buffer(vb)->list, &rg->bufs);
	spin_unlock_irqrestore(&rg->input->overlay->lock, flags);
}

static void t23_osd_return_buffers(struct t23_osd_region *rg,
				   enum vb2_buffer_state state)
{
	struct t23_overlay *overlay = rg->input->overlay;
	struct t23_osd_buffer *buf, *tmp;
	unsigned long flags;
	LIST_HEAD(done);

	spin_lock_irqsave(&overlay->lock, flags);
	if (rg->shown)
		list_add(&rg->shown->list, &done);
	rg->shown = NULL;
	list_splice_tail_init(&rg->bufs, &done);
	spin_unlock_irqrestore(&overlay->lock, flags);
	list_for_each_entry_safe(buf, tmp, &done, list) {
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, state);
	}
}

/* The regions of an input stream with one pixel format at a time. */
static int t23_osd_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct t23_osd_region *rg = vb2_get_drv_priv(q);
	struct t23_overlay_input *in = rg->input;
	unsigned long flags;
	unsigned int i;

	spin_lock_irqsave(&in->overlay->lock, flags);
	for (i = 0; i < T23_MSCA_OSD_REGIONS; i++)
		if (in->osd[i].streaming &&
		    in->osd[i].pix.pixelformat != rg->pix.pixelformat)
			break;
	if (i == T23_MSCA_OSD_REGIONS)
		rg->streaming = true;
	spin_unlock_irqrestore(&in->overlay->lock, flags);
	if (!rg->streaming) {
		t23_osd_return_buffers(rg, VB2_BUF_STATE_QUEUED);
		return -EBUSY;
	}
	return 0;
}

static bool t23_osd_pin_buffer(struct vb2_buffer *vb)
{
	struct dma_buf *dbuf;

	if (vb->memory == VB2_MEMORY_DMABUF) {
		get_dma_buf(vb->planes[0].dbuf);
		return true;
	}
	if (!vb->vb2_queue->mem_ops->get_dmabuf)
		return false;
	dbuf = vb->vb2_queue->mem_ops->get_dmabuf(vb, vb->planes[0].mem_priv,
						  O_RDWR);
	return !IS_ERR_OR_NULL(dbuf);
}

static void t23_osd_stop_streaming(struct vb2_queue *q)
{
	struct t23_osd_region *rg = vb2_get_drv_priv(q);
	struct t23_overlay *overlay = rg->input->overlay;
	unsigned long flags;

	spin_lock_irqsave(&overlay->lock, flags);
	rg->streaming = false;
	spin_unlock_irqrestore(&overlay->lock, flags);
	/* A frame armed before the stop may still read the shown bitmap. */
	if (!overlay->sync(overlay, rg->input->index)) {
		struct t23_osd_buffer *shown;
		bool pinned = false;

		spin_lock_irqsave(&overlay->lock, flags);
		shown = rg->shown;
		spin_unlock_irqrestore(&overlay->lock, flags);
		/* get_dmabuf() may sleep, so pin in process context. */
		if (shown)
			pinned = t23_osd_pin_buffer(&shown->vb.vb2_buf);
		dev_err_once(overlay->dev,
			     "OSD DMA stop timed out; %s active buffer\n",
			     pinned ? "pinned" : "could not pin");
	}
	t23_osd_return_buffers(rg, VB2_BUF_STATE_ERROR);
}

static const struct vb2_ops t23_osd_vb2_ops = {
	.queue_setup = t23_osd_queue_setup,
	.buf_prepare = t23_osd_buf_prepare,
	.buf_queue = t23_osd_buf_queue,
	.start_streaming = t23_osd_start_streaming,
	.stop_streaming = t23_osd_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

static int t23_osd_querycap(struct file *file, void *fh,
			    struct v4l2_capability *cap)
{
	struct t23_osd_region *rg = video_drvdata(file);

	strscpy(cap->driver, "tx-isp", sizeof(cap->driver));
	strscpy(cap->card, rg->vdev.name, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev_name(rg->input->overlay->dev));
	return 0;
}

static int t23_osd_enum_fmt(struct file *file, void *fh,
			    struct v4l2_fmtdesc *f)
{
	if (f->index >= ARRAY_SIZE(t23_osd_formats))
		return -EINVAL;
	f->pixelformat = t23_osd_formats[f->index].fourcc;
	return 0;
}

static int t23_osd_g_fmt(struct file *file, void *fh, struct v4l2_format *f)
{
	struct t23_osd_region *rg = video_drvdata(file);

	f->fmt.pix = rg->pix;
	return 0;
}

static int t23_osd_try_fmt(struct file *file, void *fh, struct v4l2_format *f)
{
	struct t23_osd_region *rg = video_drvdata(file);

	t23_osd_try_format(rg->input, &f->fmt.pix);
	return 0;
}

static int t23_osd_s_fmt(struct file *file, void *fh, struct v4l2_format *f)
{
	struct t23_osd_region *rg = video_drvdata(file);
	unsigned long flags;

	if (vb2_is_busy(&rg->queue))
		return -EBUSY;
	t23_osd_try_format(rg->input, &f->fmt.pix);
	spin_lock_irqsave(&rg->input->overlay->lock, flags);
	rg->pix = f->fmt.pix;
	t23_osd_clamp_position(rg);
	spin_unlock_irqrestore(&rg->input->overlay->lock, flags);
	return 0;
}

static int t23_osd_g_selection(struct file *file, void *fh,
			       struct v4l2_selection *s)
{
	struct t23_osd_region *rg = video_drvdata(file);
	struct t23_overlay_input *in = rg->input;

	if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;
	switch (s->target) {
	case V4L2_SEL_TGT_COMPOSE:
		s->r.left = rg->left;
		s->r.top = rg->top;
		break;
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
		s->r.left = 0;
		s->r.top = 0;
		break;
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		if (!in->width)
			return -ENOLINK;
		s->r = (struct v4l2_rect){ 0, 0, in->width, in->height };
		return 0;
	default:
		return -EINVAL;
	}
	s->r.width = rg->pix.width;
	s->r.height = rg->pix.height;
	return 0;
}

/* The bitmap is not scaled: only the compose position can change. */
static int t23_osd_s_selection(struct file *file, void *fh,
			       struct v4l2_selection *s)
{
	struct t23_osd_region *rg = video_drvdata(file);
	unsigned long flags;

	if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT ||
	    s->target != V4L2_SEL_TGT_COMPOSE)
		return -EINVAL;
	spin_lock_irqsave(&rg->input->overlay->lock, flags);
	rg->left = clamp(s->r.left, 0, (s32)T23_OSD_MAX_SIZE) & ~1;
	rg->top = clamp(s->r.top, 0, (s32)T23_OSD_MAX_SIZE) & ~1;
	t23_osd_clamp_position(rg);
	s->r = (struct v4l2_rect){
		rg->left, rg->top, rg->pix.width, rg->pix.height
	};
	spin_unlock_irqrestore(&rg->input->overlay->lock, flags);
	return 0;
}

static const struct v4l2_ioctl_ops t23_osd_ioctl_ops = {
	.vidioc_querycap = t23_osd_querycap,
	.vidioc_enum_fmt_vid_out = t23_osd_enum_fmt,
	.vidioc_g_fmt_vid_out = t23_osd_g_fmt,
	.vidioc_s_fmt_vid_out = t23_osd_s_fmt,
	.vidioc_try_fmt_vid_out = t23_osd_try_fmt,
	.vidioc_g_selection = t23_osd_g_selection,
	.vidioc_s_selection = t23_osd_s_selection,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
};

static const struct v4l2_file_operations t23_osd_fops = {
	.owner = THIS_MODULE,
	.open = v4l2_fh_open,
	.release = vb2_fop_release,
	.poll = vb2_fop_poll,
	.mmap = vb2_fop_mmap,
	.unlocked_ioctl = video_ioctl2,
};

static int t23_osd_node_init(struct t23_osd_region *rg,
			     struct v4l2_device *v4l2_dev, struct mutex *lock)
{
	struct vb2_queue *q = &rg->queue;
	int ret;

	rg->pix = (struct v4l2_pix_format){
		.width = 64,
		.height = 32,
		.pixelformat = t23_osd_formats[0].fourcc,
	};
	t23_osd_try_format(rg->input, &rg->pix);

	q->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	q->io_modes = VB2_MMAP | VB2_DMABUF;
	q->drv_priv = rg;
	q->ops = &t23_osd_vb2_ops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->buf_struct_size = sizeof(struct t23_osd_buffer);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	q->lock = lock;
	q->dev = rg->input->overlay->dev;
	ret = vb2_queue_init(q);
	if (ret)
		return ret;
	rg->queue_initialized = true;

	rg->vdev = (struct video_device){
		.fops = &t23_osd_fops,
		.ioctl_ops = &t23_osd_ioctl_ops,
		.release = video_device_release_empty,
		.v4l2_dev = v4l2_dev,
		.queue = q,
		.lock = lock,
		.vfl_dir = VFL_DIR_TX,
		.device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING,
	};
	snprintf(rg->vdev.name, sizeof(rg->vdev.name), "T23 osd%u.%u",
		 rg->input->index, rg->index);
	rg->vdev.entity.function = MEDIA_ENT_F_IO_V4L;
	rg->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&rg->vdev.entity, 1, &rg->pad);
	if (ret)
		return ret;
	rg->entity_initialized = true;
	video_set_drvdata(&rg->vdev, rg);

	ret = video_register_device(&rg->vdev, VFL_TYPE_VIDEO, -1);
	if (ret)
		return ret;
	rg->registered = true;
	return 0;
}

int t23_overlay_register(struct t23_overlay *overlay,
			 struct v4l2_device *v4l2_dev, struct mutex *lock,
			 struct media_entity *isp[], unsigned int osd_pad)
{
	unsigned int i, j;
	int ret;

	for (i = 0; i < T23_OVERLAY_INPUTS; i++) {
		for (j = 0; j < T23_MSCA_OSD_REGIONS; j++) {
			struct t23_osd_region *rg = &overlay->inputs[i].osd[j];

			ret = t23_osd_node_init(rg, v4l2_dev, lock);
			if (ret)
				goto fail;
			ret = media_create_pad_link(&rg->vdev.entity, 0, isp[i],
						    osd_pad + j,
						    MEDIA_LNK_FL_ENABLED);
			if (ret)
				goto fail;
		}
	}
	return 0;

fail:
	t23_overlay_unregister(overlay);
	return ret;
}

void t23_overlay_unregister(struct t23_overlay *overlay)
{
	unsigned int i, j;

	for (i = 0; i < T23_OVERLAY_INPUTS; i++) {
		for (j = 0; j < T23_MSCA_OSD_REGIONS; j++) {
			struct t23_osd_region *rg = &overlay->inputs[i].osd[j];

			if (!rg->registered)
				continue;
			video_unregister_device(&rg->vdev);
			rg->registered = false;
		}
	}
}

void t23_overlay_release(struct t23_overlay *overlay)
{
	unsigned int i, j;

	for (i = 0; i < T23_OVERLAY_INPUTS; i++) {
		for (j = 0; j < T23_MSCA_OSD_REGIONS; j++) {
			struct t23_osd_region *rg = &overlay->inputs[i].osd[j];

			if (rg->queue_initialized)
				vb2_queue_release(&rg->queue);
			if (rg->entity_initialized)
				media_entity_cleanup(&rg->vdev.entity);
			rg->queue_initialized = false;
			rg->entity_initialized = false;
		}
	}
}
