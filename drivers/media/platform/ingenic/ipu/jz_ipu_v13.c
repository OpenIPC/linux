// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2015 Ingenic Semiconductor Co., Ltd.
 * Copyright (C) 2026 Difan Zhang
 * Ingenic T23 IPU format conversion.
 */
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/kref.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>

#include "jz_regs_v13.h"

struct ipu_ctx;

struct ipu_dev {
	struct device *dev;
	struct platform_device *dma_pdev;
	struct kref ref;
	struct v4l2_device v4l2;
	struct video_device *video;
	struct v4l2_m2m_dev *m2m;
	struct mutex lock;
	struct list_head contexts;
	void __iomem *regs;
	struct clk *clk;
	int irq;
	struct work_struct work;
	struct workqueue_struct *workqueue;
	struct completion done;
	struct ipu_ctx *active_ctx;
	u32 irq_status;
	bool removed;
	bool failed;
	bool reserved_mem;
	bool quarantined;
};

struct ipu_ctx {
	struct ipu_dev *ipu;
	struct v4l2_fh fh;
	struct v4l2_ctrl_handler ctrl_handler;
	struct list_head list;
	struct v4l2_pix_format src;
	struct v4l2_pix_format dst;
	struct v4l2_rect crop;
	struct v4l2_rect compose;
	u32 alpha;
	u32 src_sequence;
	u32 dst_sequence;
	atomic_t abort;
};

static struct ipu_ctx *fh_to_ctx(void *fh)
{
	return container_of(fh, struct ipu_ctx, fh);
}

static int ipu_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ipu_ctx *ctx = container_of(ctrl->handler, struct ipu_ctx, ctrl_handler);

	switch (ctrl->id) {
	case V4L2_CID_ALPHA_COMPONENT:
		ctx->alpha = ctrl->val;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct v4l2_ctrl_ops ipu_ctrl_ops = {
	.s_ctrl = ipu_s_ctrl,
};

static struct v4l2_pix_format *ipu_format(struct ipu_ctx *ctx,
					 enum v4l2_buf_type type)
{
	return V4L2_TYPE_IS_OUTPUT(type) ? &ctx->src : &ctx->dst;
}

/*
 * The IPU writes every output line rounded up to a multiple of 64 pixels,
 * and needs 8-byte aligned line addresses.
 */
#define IPU_LINE_ALIGN	64
#define IPU_X_ALIGN	8

static void ipu_pix_format(struct v4l2_pix_format *p, u32 fourcc,
			   unsigned int width, unsigned int height)
{
	memset(p, 0, sizeof(*p));
	p->width = width;
	p->height = height;
	p->pixelformat = fourcc;
	p->field = V4L2_FIELD_NONE;
	p->colorspace = V4L2_COLORSPACE_SMPTE170M;
	p->xfer_func = V4L2_XFER_FUNC_709;
	p->ycbcr_enc = V4L2_YCBCR_ENC_601;

	switch (fourcc) {
	case V4L2_PIX_FMT_ABGR32:
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_XBGR32:
	case V4L2_PIX_FMT_XRGB32:
		p->bytesperline = ALIGN(width, IPU_LINE_ALIGN) * 4;
		p->sizeimage = p->bytesperline * height;
		p->quantization = V4L2_QUANTIZATION_FULL_RANGE;
		break;
	case V4L2_PIX_FMT_NV21:
		p->bytesperline = ALIGN(width, IPU_LINE_ALIGN);
		p->sizeimage = p->bytesperline * height * 3 / 2;
		p->quantization = V4L2_QUANTIZATION_LIM_RANGE;
		break;
	case V4L2_PIX_FMT_NV12:
	default:
		p->bytesperline = width;
		p->sizeimage = width * height * 3 / 2;
		p->quantization = V4L2_QUANTIZATION_LIM_RANGE;
		break;
	}
}

/* The IPU converts NV12 without scaling; it has no resizer. */
static const u32 ipu_src_formats[] = {
	V4L2_PIX_FMT_NV12,
};

static const u32 ipu_dst_formats[] = {
	V4L2_PIX_FMT_NV21,
	V4L2_PIX_FMT_ABGR32,
	V4L2_PIX_FMT_ARGB32,
	V4L2_PIX_FMT_XBGR32,
	V4L2_PIX_FMT_XRGB32,
};

/* IPU_D_FMT output codes; the hardware names RGB orders from the low byte. */
static u32 ipu_d_fmt(u32 fourcc)
{
	switch (fourcc) {
	case V4L2_PIX_FMT_ABGR32:
	case V4L2_PIX_FMT_XBGR32:
		return IPU_D_FMT_OUT_BGRA8888;	/* B, G, R, A in memory */
	case V4L2_PIX_FMT_ARGB32:
	case V4L2_PIX_FMT_XRGB32:
		return IPU_D_FMT_OUT_ARGB8888;	/* A, R, G, B in memory */
	default:
		return IPU_D_FMT_OUT_NV21;	/* NV21 */
	}
}

static unsigned int ipu_bpp(u32 fourcc)
{
	return fourcc == V4L2_PIX_FMT_NV21 ? 1 : 4;
}

/*
 * Compose has the crop size and lies inside the destination. Unless its width
 * is a multiple of the line alignment, it ends at the right edge and the
 * rounded-up line still fits the padded line, so the extra pixels land in
 * the bytesperline padding.
 */
static bool ipu_compose_ok(struct ipu_ctx *ctx, const struct v4l2_rect *r)
{
	u32 line = ctx->dst.bytesperline / ipu_bpp(ctx->dst.pixelformat);

	return r->width == ctx->crop.width && r->height == ctx->crop.height &&
	       r->left + r->width <= ctx->dst.width &&
	       r->top + r->height <= ctx->dst.height &&
	       r->left + ALIGN(r->width, IPU_LINE_ALIGN) <= line &&
	       (r->width % IPU_LINE_ALIGN == 0 ||
		r->left + r->width == ctx->dst.width);
}

static bool ipu_geometry_ok(struct ipu_ctx *ctx)
{
	return ipu_compose_ok(ctx, &ctx->compose);
}

/* Fit compose to the crop size, keeping its position where allowed. */
static void ipu_fit_compose(struct ipu_ctx *ctx)
{
	struct v4l2_rect *r = &ctx->compose;

	r->width = ctx->crop.width;
	r->height = ctx->crop.height;
	if (r->width > ctx->dst.width || r->height > ctx->dst.height)
		return;
	if (r->left + r->width > ctx->dst.width ||
	    r->width % IPU_LINE_ALIGN)
		r->left = ctx->dst.width - r->width;
	if (r->top + r->height > ctx->dst.height)
		r->top = (ctx->dst.height - r->height) & ~1;
}

static int ipu_querycap(struct file *file, void *priv,
			struct v4l2_capability *cap)
{
	struct ipu_dev *ipu = video_drvdata(file);

	strscpy(cap->driver, "t23-ipu", sizeof(cap->driver));
	strscpy(cap->card, "T23 IPU", sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev_name(ipu->dev));
	return 0;
}

static int ipu_enum_fmt(struct file *file, void *priv,
			struct v4l2_fmtdesc *f)
{
	const u32 *formats;
	unsigned int num_formats;

	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		formats = ipu_src_formats;
		num_formats = ARRAY_SIZE(ipu_src_formats);
	} else {
		formats = ipu_dst_formats;
		num_formats = ARRAY_SIZE(ipu_dst_formats);
	}

	if (f->index >= num_formats)
		return -EINVAL;

	f->pixelformat = formats[f->index];
	return 0;
}

static int ipu_get_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	f->fmt.pix = *ipu_format(fh_to_ctx(priv), f->type);
	return 0;
}

static int ipu_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct ipu_ctx *ctx = fh_to_ctx(priv);
	u32 fourcc = f->fmt.pix.pixelformat;
	u32 colorspace = f->fmt.pix.colorspace;
	u32 xfer_func = f->fmt.pix.xfer_func;
	u32 ycbcr_enc = f->fmt.pix.ycbcr_enc;
	u32 quantization = f->fmt.pix.quantization;
	unsigned int width, height;
	const u32 *formats;
	unsigned int num_formats, i;
	bool found = false;

	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		formats = ipu_src_formats;
		num_formats = ARRAY_SIZE(ipu_src_formats);
	} else {
		formats = ipu_dst_formats;
		num_formats = ARRAY_SIZE(ipu_dst_formats);
	}

	for (i = 0; i < num_formats; i++) {
		if (formats[i] == fourcc) {
			found = true;
			break;
		}
	}
	if (!found)
		fourcc = formats[0];

	width = clamp_t(unsigned int, f->fmt.pix.width, 32, 1920) & ~(IPU_X_ALIGN - 1);
	height = clamp_t(unsigned int, f->fmt.pix.height, 32, 1080) & ~1;

	ipu_pix_format(&f->fmt.pix, fourcc, width, height);
	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		if (colorspace >= V4L2_COLORSPACE_SMPTE170M &&
		    colorspace < V4L2_COLORSPACE_LAST &&
		    colorspace != V4L2_COLORSPACE_BT878 &&
		    colorspace != V4L2_COLORSPACE_JPEG)
			f->fmt.pix.colorspace = colorspace;
		if (xfer_func != V4L2_XFER_FUNC_DEFAULT &&
		    xfer_func < V4L2_XFER_FUNC_LAST)
			f->fmt.pix.xfer_func = xfer_func;
		if (ycbcr_enc != V4L2_YCBCR_ENC_DEFAULT &&
		    ycbcr_enc <= V4L2_YCBCR_ENC_SMPTE240M)
			f->fmt.pix.ycbcr_enc = ycbcr_enc;
		if (quantization != V4L2_QUANTIZATION_DEFAULT &&
		    quantization <= V4L2_QUANTIZATION_LIM_RANGE)
			f->fmt.pix.quantization = quantization;
	} else {
		f->fmt.pix.colorspace = ctx->src.colorspace;
		f->fmt.pix.xfer_func = ctx->src.xfer_func;
		f->fmt.pix.ycbcr_enc = ctx->src.ycbcr_enc;
		f->fmt.pix.quantization = ctx->src.quantization;
	}
	return 0;
}

static int ipu_set_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct ipu_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_m2m_ctx *m2m = ctx->fh.m2m_ctx;

	if (vb2_is_busy(v4l2_m2m_get_src_vq(m2m)) ||
	    vb2_is_busy(v4l2_m2m_get_dst_vq(m2m)))
		return -EBUSY;

	ipu_try_fmt(file, priv, f);
	*ipu_format(ctx, f->type) = f->fmt.pix;

	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		ctx->crop.left = 0;
		ctx->crop.top = 0;
		ctx->crop.width = f->fmt.pix.width;
		ctx->crop.height = f->fmt.pix.height;
		ctx->dst.colorspace = ctx->src.colorspace;
		ctx->dst.xfer_func = ctx->src.xfer_func;
		ctx->dst.ycbcr_enc = ctx->src.ycbcr_enc;
		ctx->dst.quantization = ctx->src.quantization;
	}
	ctx->compose.left = 0;
	ctx->compose.top = 0;
	ipu_fit_compose(ctx);

	return 0;
}

static int ipu_g_selection(struct file *file, void *priv,
			   struct v4l2_selection *s)
{
	struct ipu_ctx *ctx = fh_to_ctx(priv);

	if (s->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		switch (s->target) {
		case V4L2_SEL_TGT_CROP:
			s->r = ctx->crop;
			return 0;
		case V4L2_SEL_TGT_CROP_DEFAULT:
		case V4L2_SEL_TGT_CROP_BOUNDS:
			s->r.left = 0;
			s->r.top = 0;
			s->r.width = ctx->src.width;
			s->r.height = ctx->src.height;
			return 0;
		default:
			return -EINVAL;
		}
	} else if (s->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		switch (s->target) {
		case V4L2_SEL_TGT_COMPOSE:
			s->r = ctx->compose;
			return 0;
		case V4L2_SEL_TGT_COMPOSE_DEFAULT:
			s->r.left = 0;
			s->r.top = 0;
			s->r.width = ctx->crop.width;
			s->r.height = ctx->crop.height;
			return 0;
		case V4L2_SEL_TGT_COMPOSE_BOUNDS:
			s->r.left = 0;
			s->r.top = 0;
			s->r.width = ctx->dst.width;
			s->r.height = ctx->dst.height;
			return 0;
		default:
			return -EINVAL;
		}
	}

	return -EINVAL;
}

static int ipu_s_selection(struct file *file, void *priv,
			   struct v4l2_selection *s)
{
	struct ipu_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_m2m_ctx *m2m = ctx->fh.m2m_ctx;

	/* A running job reads the geometry without the device lock. */
	if (vb2_is_busy(v4l2_m2m_get_src_vq(m2m)) ||
	    vb2_is_busy(v4l2_m2m_get_dst_vq(m2m)))
		return -EBUSY;

	if (s->type == V4L2_BUF_TYPE_VIDEO_OUTPUT &&
	    s->target == V4L2_SEL_TGT_CROP) {
		s->r.left = clamp_t(u32, s->r.left, 0, ctx->src.width - 32) &
			    ~(IPU_X_ALIGN - 1);
		s->r.top = clamp_t(u32, s->r.top, 0, ctx->src.height - 32) & ~1;
		s->r.width = clamp_t(u32, s->r.width, 32, ctx->src.width - s->r.left) &
			     ~(IPU_X_ALIGN - 1);
		s->r.height = clamp_t(u32, s->r.height, 32, ctx->src.height - s->r.top) & ~1;
		ctx->crop = s->r;
		ipu_fit_compose(ctx);
		return 0;
	} else if (s->type == V4L2_BUF_TYPE_VIDEO_CAPTURE &&
		   s->target == V4L2_SEL_TGT_COMPOSE) {
		/* Only the position can change: the IPU does not scale. */
		if (ctx->crop.width > ctx->dst.width ||
		    ctx->crop.height > ctx->dst.height)
			return -EINVAL;
		s->r.left = clamp_t(s32, s->r.left, 0,
				    ctx->dst.width - ctx->crop.width) &
			    ~(IPU_X_ALIGN - 1);
		s->r.top = clamp_t(s32, s->r.top, 0,
				   ctx->dst.height - ctx->crop.height) & ~1;
		s->r.width = ctx->crop.width;
		s->r.height = ctx->crop.height;
		if (ctx->crop.width % IPU_LINE_ALIGN)
			s->r.left = ctx->dst.width - ctx->crop.width;
		if (!ipu_compose_ok(ctx, &s->r))
			return -EINVAL;
		ctx->compose = s->r;
		return 0;
	}

	return -EINVAL;
}

static int ipu_queue_setup(struct vb2_queue *q, unsigned int *count,
			   unsigned int *planes, unsigned int sizes[],
			   struct device *alloc_devs[])
{
	struct ipu_ctx *ctx = vb2_get_drv_priv(q);
	struct v4l2_pix_format *fmt = ipu_format(ctx, q->type);

	if (READ_ONCE(ctx->ipu->removed))
		return -ENODEV;
	if (*planes)
		return *planes == 1 && sizes[0] >= fmt->sizeimage ? 0 : -EINVAL;
	*planes = 1;
	sizes[0] = fmt->sizeimage;
	return 0;
}

static int ipu_buf_prepare(struct vb2_buffer *vb)
{
	struct ipu_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct v4l2_pix_format *fmt = ipu_format(ctx, vb->type);
	struct vb2_v4l2_buffer *buf = to_vb2_v4l2_buffer(vb);

	if (READ_ONCE(ctx->ipu->removed))
		return -ENODEV;
	if (READ_ONCE(ctx->ipu->failed))
		return -EIO;
	if (vb2_plane_size(vb, 0) < fmt->sizeimage)
		return -EINVAL;
	if (V4L2_TYPE_IS_OUTPUT(vb->type) && buf->field != V4L2_FIELD_NONE &&
	    buf->field != V4L2_FIELD_ANY)
		return -EINVAL;
	if (V4L2_TYPE_IS_OUTPUT(vb->type)) {
		if (!vb2_get_plane_payload(vb, 0))
			vb2_set_plane_payload(vb, 0, fmt->sizeimage);
		else if (vb2_get_plane_payload(vb, 0) < fmt->sizeimage)
			return -EINVAL;
	} else {
		vb2_set_plane_payload(vb, 0, fmt->sizeimage);
	}
	to_vb2_v4l2_buffer(vb)->field = V4L2_FIELD_NONE;
	return 0;
}

static void ipu_buf_queue(struct vb2_buffer *vb)
{
	struct ipu_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, to_vb2_v4l2_buffer(vb));
}

static void ipu_return_buffers(struct vb2_queue *q, enum vb2_buffer_state state)
{
	struct ipu_ctx *ctx = vb2_get_drv_priv(q);
	struct vb2_v4l2_buffer *buf;

	for (;;) {
		buf = V4L2_TYPE_IS_OUTPUT(q->type) ?
			v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx) :
			v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
		if (!buf)
			break;
		v4l2_m2m_buf_done(buf, state);
	}
}

static int ipu_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct ipu_ctx *ctx = vb2_get_drv_priv(q);

	if (READ_ONCE(ctx->ipu->removed) || READ_ONCE(ctx->ipu->failed)) {
		ipu_return_buffers(q, VB2_BUF_STATE_QUEUED);
		return -ENODEV;
	}
	if (!ipu_geometry_ok(ctx)) {
		ipu_return_buffers(q, VB2_BUF_STATE_QUEUED);
		return -EINVAL;
	}
	if (V4L2_TYPE_IS_OUTPUT(q->type))
		ctx->src_sequence = 0;
	else
		ctx->dst_sequence = 0;
	atomic_set(&ctx->abort, 0);
	return 0;
}

static void ipu_stop_streaming(struct vb2_queue *q)
{
	/* The mem2mem core has already waited for job_abort to finish. */
	ipu_return_buffers(q, VB2_BUF_STATE_ERROR);
}

static const struct vb2_ops ipu_queue_ops = {
	.queue_setup = ipu_queue_setup,
	.buf_prepare = ipu_buf_prepare,
	.buf_queue = ipu_buf_queue,
	.start_streaming = ipu_start_streaming,
	.stop_streaming = ipu_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

static int ipu_queue_init(void *priv, struct vb2_queue *src,
			  struct vb2_queue *dst)
{
	struct ipu_ctx *ctx = priv;
	struct vb2_queue *queues[] = { src, dst };
	unsigned int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(queues); i++) {
		struct vb2_queue *q = queues[i];

		q->type = i ? V4L2_BUF_TYPE_VIDEO_CAPTURE :
			      V4L2_BUF_TYPE_VIDEO_OUTPUT;
		q->io_modes = VB2_MMAP | VB2_DMABUF;
		q->drv_priv = ctx;
		q->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
		q->ops = &ipu_queue_ops;
		q->mem_ops = &vb2_dma_contig_memops;
		q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
		q->lock = &ctx->ipu->lock;
		q->dev = &ctx->ipu->dma_pdev->dev;
		ret = vb2_queue_init(q);
		if (ret)
			return ret;
	}
	return 0;
}

static int ipu_quiesce(struct ipu_dev *ipu)
{
	u32 status;

	writel(0, ipu->regs + IPU_GLB_CTRL);
	writel(readl(ipu->regs + IPU_TRIGGER) | IPU_STOP,
	       ipu->regs + IPU_TRIGGER);
	writel(readl(ipu->regs + IPU_TRIGGER) | IPU_RESET,
	       ipu->regs + IPU_TRIGGER);
	return readl_poll_timeout(ipu->regs + IPU_STATUS, status,
				 !(status & MSTATUS_MSK << MSTATUS_SFT),
				 10, 100000);
}

static void ipu_program(struct ipu_ctx *ctx, dma_addr_t src, dma_addr_t dst)
{
	struct ipu_dev *ipu = ctx->ipu;
	const struct v4l2_pix_format *in = &ctx->src, *out = &ctx->dst;
	const struct v4l2_rect *c = &ctx->crop, *o = &ctx->compose;
	u32 bpp = ipu_bpp(out->pixelformat);

	writel((c->width << 16) | c->height, ipu->regs + IPU_IN_FM_GS);
	writel(ipu_d_fmt(out->pixelformat), ipu->regs + IPU_D_FMT);
	writel(0, ipu->regs + IPU_FM_CTRL);
	writel(in->bytesperline, ipu->regs + IPU_Y_STRIDE);
	writel(in->bytesperline | (in->bytesperline << 16),
	       ipu->regs + IPU_UV_STRIDE);
	writel(src + c->top * in->bytesperline + c->left, ipu->regs + IPU_Y_ADDR);
	writel(src + in->bytesperline * in->height +
	       c->top / 2 * in->bytesperline + c->left, ipu->regs + IPU_U_ADDR);
	writel(out->bytesperline, ipu->regs + IPU_OUT_STRIDE);
	writel(out->bytesperline, ipu->regs + IPU_NV_OUT_STRIDE);
	writel(dst + o->top * out->bytesperline + o->left * bpp,
	       ipu->regs + IPU_OUT_ADDR);
	writel(dst + out->bytesperline * out->height +
	       o->top / 2 * out->bytesperline + o->left,
	       ipu->regs + IPU_NV_OUT_ADDR);

	/* The NV12 background layer carries the output alpha. */
	writel(IPU_OSD_CH0_PARA_DEFAULT, ipu->regs + IPU_OSD_CH0_PARA);
	writel(IPU_OSD_CH_BK_PARA_DEFAULT | CH_BK_GLOBAL_ALPHA |
	       CH_BK_GLO_A(ctx->alpha), ipu->regs + IPU_OSD_CH_BK_PARA);
	writel(0, ipu->regs + IPU_OSD_CH1_PARA);
	writel(0, ipu->regs + IPU_OSD_CH2_PARA);
	writel(0, ipu->regs + IPU_OSD_CH3_PARA);

	/* CSC uses the background input and the channel-zero matrix. */
	writel(IPU_CSC_C0_BT601, ipu->regs + IPU_CH0_CSC_C0_COEF);
	writel(IPU_CSC_C1_BT601, ipu->regs + IPU_CH0_CSC_C1_COEF);
	writel(IPU_CSC_C2_BT601, ipu->regs + IPU_CH0_CSC_C2_COEF);
	writel(IPU_CSC_C3_BT601, ipu->regs + IPU_CH0_CSC_C3_COEF);
	writel(IPU_CSC_C4_BT601, ipu->regs + IPU_CH0_CSC_C4_COEF);
	writel(IPU_CSC_OFSET_BT601, ipu->regs + IPU_CH0_CSC_OFSET_PARA);
	writel(readl(ipu->regs + IPU_STATUS) & ~(OUT_END | SIZE_ERR),
	       ipu->regs + IPU_STATUS);
	writel(IRQ_EN, ipu->regs + IPU_GLB_CTRL);
	writel(U32_MAX, ipu->regs + IPU_REG_CTRL);
	dma_wmb();
	writel(readl(ipu->regs + IPU_TRIGGER) | IPU_RUN,
	       ipu->regs + IPU_TRIGGER);
}

static irqreturn_t ipu_irq(int irq, void *data)
{
	struct ipu_dev *ipu = data;
	u32 status = readl(ipu->regs + IPU_STATUS);

	if (!(status & (OUT_END | SIZE_ERR)))
		return IRQ_NONE;
	writel(0, ipu->regs + IPU_GLB_CTRL);
	writel(status & ~(OUT_END | SIZE_ERR), ipu->regs + IPU_STATUS);
	WRITE_ONCE(ipu->irq_status, status);
	complete(&ipu->done);
	return IRQ_HANDLED;
}

/*
 * Keep a buffer's memory allocated when a stuck IPU may still write to it.
 */
static bool ipu_pin_buffer(struct ipu_dev *ipu, struct vb2_buffer *vb)
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
	if (IS_ERR_OR_NULL(dbuf)) {
		dev_err_once(ipu->dev,
			     "cannot pin buffer %u under a running DMA\n",
			     vb->index);
		return false;
	}
	return true;
}

static void ipu_work(struct work_struct *work)
{
	struct ipu_dev *ipu = container_of(work, struct ipu_dev, work);
	struct ipu_ctx *ctx = READ_ONCE(ipu->active_ctx);
	struct vb2_v4l2_buffer *src, *dst;
	enum vb2_buffer_state state = VB2_BUF_STATE_ERROR;
	int ret;

	src = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
	dst = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
	if (!src || !dst)
		goto finish;
	if (READ_ONCE(ipu->removed) || READ_ONCE(ipu->failed) ||
	    atomic_read(&ctx->abort) || !ipu_geometry_ok(ctx))
		goto finish;
	ret = clk_prepare_enable(ipu->clk);
	if (ret)
		goto finish;
	ret = ipu_quiesce(ipu);
	if (ret) {
		WRITE_ONCE(ipu->failed, true);
		goto disable_clock;
	}
	synchronize_irq(ipu->irq);
	reinit_completion(&ipu->done);
	WRITE_ONCE(ipu->irq_status, 0);
	if (!atomic_read(&ctx->abort) && !READ_ONCE(ipu->removed)) {
		ipu_program(ctx, vb2_dma_contig_plane_dma_addr(&src->vb2_buf, 0),
			    vb2_dma_contig_plane_dma_addr(&dst->vb2_buf, 0));
		ret = wait_for_completion_timeout(&ipu->done,
						  msecs_to_jiffies(2000));
		if (ret && !atomic_read(&ctx->abort) &&
		    (READ_ONCE(ipu->irq_status) & (OUT_END | SIZE_ERR)) == OUT_END)
			state = VB2_BUF_STATE_DONE;
		else if (!ret)
			dev_err(ipu->dev, "conversion timed out\n");
	}
	ret = ipu_quiesce(ipu);
	if (ret) {
		bool pinned;

		WRITE_ONCE(ipu->failed, true);
		ipu->quarantined = true;
		pinned = ipu_pin_buffer(ipu, &src->vb2_buf) &
			 ipu_pin_buffer(ipu, &dst->vb2_buf);
		state = VB2_BUF_STATE_ERROR;
		dev_err(ipu->dev,
			"DMA stop timed out; IPU disabled, buffers %s\n",
			pinned ? "retained" : "may not be pinned");
	}
disable_clock:
	synchronize_irq(ipu->irq);
	clk_disable_unprepare(ipu->clk);
finish:
	if (dst)
		vb2_set_plane_payload(&dst->vb2_buf, 0,
				      state == VB2_BUF_STATE_DONE ?
				      ctx->dst.sizeimage : 0);
	if (src && dst) {
		v4l2_m2m_buf_copy_metadata(src, dst, true);
		src->sequence = ctx->src_sequence++;
		dst->sequence = ctx->dst_sequence++;
	}
	if (src)
		v4l2_m2m_buf_done(src, state);
	if (dst)
		v4l2_m2m_buf_done(dst, state);
	v4l2_m2m_job_finish(ipu->m2m, ctx->fh.m2m_ctx);
}

static void ipu_device_run(void *priv)
{
	struct ipu_ctx *ctx = priv;

	WRITE_ONCE(ctx->ipu->active_ctx, ctx);
	queue_work(ctx->ipu->workqueue, &ctx->ipu->work);
}

static int ipu_job_ready(void *priv)
{
	struct ipu_ctx *ctx = priv;

	return !READ_ONCE(ctx->ipu->removed) &&
	       !READ_ONCE(ctx->ipu->failed) &&
	       !atomic_read(&ctx->abort);
}

static void ipu_job_abort(void *priv)
{
	struct ipu_ctx *ctx = priv;

	atomic_set(&ctx->abort, 1);
	complete(&ctx->ipu->done);
	flush_workqueue(ctx->ipu->workqueue);
}

static const struct v4l2_m2m_ops ipu_m2m_ops = {
	.device_run = ipu_device_run,
	.job_ready = ipu_job_ready,
	.job_abort = ipu_job_abort,
};

static void ipu_destroy(struct kref *ref)
{
	struct ipu_dev *ipu = container_of(ref, struct ipu_dev, ref);

	destroy_workqueue(ipu->workqueue);
	v4l2_m2m_release(ipu->m2m);
	v4l2_device_unregister(&ipu->v4l2);
	/* Pinned buffers still belong to the DMA device. */
	if (!ipu->quarantined) {
		if (ipu->reserved_mem)
			of_reserved_mem_device_release(&ipu->dma_pdev->dev);
		platform_device_unregister(ipu->dma_pdev);
	}
	put_device(ipu->dev);
	kfree(ipu);
}

static int ipu_open(struct file *file)
{
	struct ipu_dev *ipu = video_drvdata(file);
	struct ipu_ctx *ctx;
	int ret;

	mutex_lock(&ipu->lock);
	if (ipu->removed) {
		ret = -ENODEV;
		goto unlock;
	}
	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		ret = -ENOMEM;
		goto unlock;
	}
	ctx->ipu = ipu;
	atomic_set(&ctx->abort, 0);
	ipu_pix_format(&ctx->src, V4L2_PIX_FMT_NV12, 640, 480);
	ipu_pix_format(&ctx->dst, V4L2_PIX_FMT_NV21, 640, 480);
	ctx->crop.left = 0;
	ctx->crop.top = 0;
	ctx->crop.width = 640;
	ctx->crop.height = 480;
	ctx->compose.left = 0;
	ctx->compose.top = 0;
	ctx->compose.width = 640;
	ctx->compose.height = 480;
	ctx->alpha = 255;

	v4l2_ctrl_handler_init(&ctx->ctrl_handler, 1);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &ipu_ctrl_ops,
			  V4L2_CID_ALPHA_COMPONENT, 0, 255, 1, 255);
	if (ctx->ctrl_handler.error) {
		ret = ctx->ctrl_handler.error;
		v4l2_ctrl_handler_free(&ctx->ctrl_handler);
		kfree(ctx);
		goto unlock;
	}

	v4l2_fh_init(&ctx->fh, ipu->video);
	ctx->fh.ctrl_handler = &ctx->ctrl_handler;
	file->private_data = &ctx->fh;
	ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(ipu->m2m, ctx, ipu_queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		v4l2_ctrl_handler_free(&ctx->ctrl_handler);
		v4l2_fh_exit(&ctx->fh);
		kfree(ctx);
		goto unlock;
	}
	v4l2_ctrl_handler_setup(&ctx->ctrl_handler);
	kref_get(&ipu->ref);
	list_add_tail(&ctx->list, &ipu->contexts);
	v4l2_fh_add(&ctx->fh);
	ret = 0;
unlock:
	mutex_unlock(&ipu->lock);
	return ret;
}

static int ipu_release(struct file *file)
{
	struct ipu_ctx *ctx = fh_to_ctx(file->private_data);
	struct ipu_dev *ipu = ctx->ipu;

	mutex_lock(&ipu->lock);
	atomic_set(&ctx->abort, 1);
	flush_workqueue(ipu->workqueue);
	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	list_del(&ctx->list);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	mutex_unlock(&ipu->lock);
	kfree(ctx);
	kref_put(&ipu->ref, ipu_destroy);
	return 0;
}

static const struct v4l2_ioctl_ops ipu_ioctl_ops = {
	.vidioc_querycap = ipu_querycap,
	.vidioc_enum_fmt_vid_cap = ipu_enum_fmt,
	.vidioc_enum_fmt_vid_out = ipu_enum_fmt,
	.vidioc_g_fmt_vid_cap = ipu_get_fmt,
	.vidioc_g_fmt_vid_out = ipu_get_fmt,
	.vidioc_try_fmt_vid_cap = ipu_try_fmt,
	.vidioc_try_fmt_vid_out = ipu_try_fmt,
	.vidioc_s_fmt_vid_cap = ipu_set_fmt,
	.vidioc_s_fmt_vid_out = ipu_set_fmt,
	.vidioc_g_selection = ipu_g_selection,
	.vidioc_s_selection = ipu_s_selection,
	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_qbuf = v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf = v4l2_m2m_ioctl_dqbuf,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,
	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,
	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static const struct v4l2_file_operations ipu_fops = {
	.owner = THIS_MODULE,
	.open = ipu_open,
	.release = ipu_release,
	.unlocked_ioctl = video_ioctl2,
	.poll = v4l2_m2m_fop_poll,
	.mmap = v4l2_m2m_fop_mmap,
};

static void ipu_video_release(struct video_device *video)
{
	struct ipu_dev *ipu = video_get_drvdata(video);

	video_device_release(video);
	kref_put(&ipu->ref, ipu_destroy);
}

static int ipu_probe(struct platform_device *pdev)
{
	struct ipu_dev *ipu;
	struct video_device *video;
	int ret;

	ipu = kzalloc(sizeof(*ipu), GFP_KERNEL);
	if (!ipu)
		return -ENOMEM;
	ipu->dev = get_device(&pdev->dev);
	kref_init(&ipu->ref);
	mutex_init(&ipu->lock);
	INIT_LIST_HEAD(&ipu->contexts);
	INIT_WORK(&ipu->work, ipu_work);
	init_completion(&ipu->done);
	ipu->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(ipu->regs)) {
		ret = PTR_ERR(ipu->regs);
		goto free;
	}
	ipu->clk = devm_clk_get(&pdev->dev, "ipu");
	if (IS_ERR(ipu->clk)) {
		ret = PTR_ERR(ipu->clk);
		dev_err(&pdev->dev, "failed to get ipu clock: %d\n", ret);
		goto free;
	}
	ipu->irq = platform_get_irq(pdev, 0);
	if (ipu->irq < 0) {
		ret = ipu->irq;
		dev_err(&pdev->dev, "failed to get IRQ: %d\n", ret);
		goto free;
	}
	ipu->dma_pdev = platform_device_alloc("t23-ipu-dma",
					      PLATFORM_DEVID_AUTO);
	if (!ipu->dma_pdev) {
		ret = -ENOMEM;
		goto free;
	}
	ipu->dma_pdev->dev.parent = &pdev->dev;
	ipu->dma_pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	ipu->dma_pdev->dev.dma_mask = &ipu->dma_pdev->dev.coherent_dma_mask;
	ret = platform_device_add(ipu->dma_pdev);
	if (ret) {
		platform_device_put(ipu->dma_pdev);
		goto free;
	}
	ret = of_dma_configure(&ipu->dma_pdev->dev, pdev->dev.of_node, true);
	if (ret)
		goto unregister_dma;
	ret = dma_set_mask_and_coherent(&ipu->dma_pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		goto unregister_dma;
	vb2_dma_contig_set_max_seg_size(&ipu->dma_pdev->dev, DMA_BIT_MASK(32));
	ret = of_reserved_mem_device_init_by_idx(&ipu->dma_pdev->dev,
					       pdev->dev.of_node, 0);
	if (!ret)
		ipu->reserved_mem = true;
	else if (ret != -ENODEV && ret != -EBUSY)
		goto unregister_dma;
	ret = clk_prepare_enable(ipu->clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to enable clock: %d\n", ret);
		goto release_mem;
	}
	ret = ipu_quiesce(ipu);
	clk_disable_unprepare(ipu->clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to quiesce IPU: %d\n", ret);
		goto release_mem;
	}
	ret = devm_request_irq(&pdev->dev, ipu->irq, ipu_irq, 0,
			       dev_name(&pdev->dev), ipu);
	if (ret) {
		dev_err(&pdev->dev, "failed to request IRQ %d: %d\n", ipu->irq, ret);
		goto release_mem;
	}
	ret = v4l2_device_register(&pdev->dev, &ipu->v4l2);
	if (ret)
		goto release_mem;
	ipu->m2m = v4l2_m2m_init(&ipu_m2m_ops);
	if (IS_ERR(ipu->m2m)) {
		ret = PTR_ERR(ipu->m2m);
		goto unregister_v4l2;
	}
	ipu->workqueue = alloc_ordered_workqueue("t23-ipu", WQ_MEM_RECLAIM);
	if (!ipu->workqueue) {
		ret = -ENOMEM;
		goto release_m2m;
	}
	video = video_device_alloc();
	if (!video) {
		ret = -ENOMEM;
		goto destroy_workqueue;
	}
	ipu->video = video;
	strscpy(video->name, "T23 IPU", sizeof(video->name));
	video->v4l2_dev = &ipu->v4l2;
	video->fops = &ipu_fops;
	video->ioctl_ops = &ipu_ioctl_ops;
	video->lock = &ipu->lock;
	video->release = ipu_video_release;
	video->vfl_dir = VFL_DIR_M2M;
	video->device_caps = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;
	video_set_drvdata(video, ipu);
	kref_get(&ipu->ref);
	ret = video_register_device(video, VFL_TYPE_VIDEO, -1);
	if (ret) {
		video_device_release(video);
		kref_put(&ipu->ref, ipu_destroy);
		goto destroy_workqueue;
	}

	platform_set_drvdata(pdev, ipu);
	return 0;
destroy_workqueue:
	destroy_workqueue(ipu->workqueue);
release_m2m:
	v4l2_m2m_release(ipu->m2m);
unregister_v4l2:
	v4l2_device_unregister(&ipu->v4l2);
release_mem:
	if (ipu->reserved_mem)
		of_reserved_mem_device_release(&ipu->dma_pdev->dev);
unregister_dma:
	platform_device_unregister(ipu->dma_pdev);
free:
	put_device(ipu->dev);
	kfree(ipu);
	return ret;
}

static void ipu_remove(struct platform_device *pdev)
{
	struct ipu_dev *ipu = platform_get_drvdata(pdev);
	struct ipu_ctx *ctx;

	mutex_lock(&ipu->lock);
	WRITE_ONCE(ipu->removed, true);
	complete(&ipu->done);
	flush_workqueue(ipu->workqueue);
	list_for_each_entry(ctx, &ipu->contexts, list) {
		v4l2_m2m_streamoff(NULL, ctx->fh.m2m_ctx,
				   V4L2_BUF_TYPE_VIDEO_OUTPUT);
		v4l2_m2m_streamoff(NULL, ctx->fh.m2m_ctx,
				   V4L2_BUF_TYPE_VIDEO_CAPTURE);
		vb2_queue_error(v4l2_m2m_get_src_vq(ctx->fh.m2m_ctx));
		vb2_queue_error(v4l2_m2m_get_dst_vq(ctx->fh.m2m_ctx));
	}
	v4l2_device_disconnect(&ipu->v4l2);
	mutex_unlock(&ipu->lock);

	video_unregister_device(ipu->video);
	kref_put(&ipu->ref, ipu_destroy);
}

static const struct of_device_id ipu_of_match[] = {
	{ .compatible = "ingenic,t23-ipu" },
	{ }
};
MODULE_DEVICE_TABLE(of, ipu_of_match);

static struct platform_driver ipu_driver = {
	.probe = ipu_probe,
	.remove = ipu_remove,
	.driver = {
		.name = "t23-ipu",
		.of_match_table = ipu_of_match,
	},
};
module_platform_driver(ipu_driver);

MODULE_DESCRIPTION("Ingenic T23 IPU V4L2 mem2mem converter");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(DMA_BUF);
