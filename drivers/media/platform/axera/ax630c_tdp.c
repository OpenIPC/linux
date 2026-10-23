// SPDX-License-Identifier: GPL-2.0-only
/* AX630C TDP V4L2 mem2mem scaler / colour converter.
 * DMA-BUF input from VIN or VDEC, reusable vb2 CAPTURE buffers. No private
 * register ioctl and no shared CDMA/VIN/VENC reset. Register protocol derived
 * from Axera's GPL ax_tdp module.
 */
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/workqueue.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>

#define TDP_MAX_W 4096
#define TDP_MAX_H 2400
#define TDP_STATUS 0x004
#define TDP_IRQ_MASK 0x00c
#define TDP_IRQ_CLEAR 0x010
#define TDP_START 0x01c
#define TDP_AXI_ENABLE 0x2f0
#define TDP_AXI_BUSY 0x2f4
#define TDP_DONE BIT(0)
#define TDP_ERRORS (BIT(2) | BIT(3))

struct tdp_dev {
	struct device *dev;
	void __iomem *regs;
	struct clk_bulk_data clocks[2];
	struct reset_control *resets;
	struct mutex lock;
	struct completion done;
	struct work_struct work;
	struct v4l2_device v4l2;
	struct video_device video;
	struct v4l2_m2m_dev *m2m;
	u32 irq_status;
	bool poisoned;
	atomic64_t jobs, errors, timeouts;
};
struct tdp_ctx {
	struct tdp_dev *tdp;
	struct v4l2_fh fh;
	struct v4l2_pix_format src, dst;
	struct v4l2_rect crop;
	u32 src_sequence, dst_sequence;
};
struct tdp_buffer {
	struct v4l2_m2m_buffer m2m;
	/* Pin physical storage before DMA; retain it if AXI cannot drain. AX630C
	 * has no IOMMU, so retaining this storage also preserves the DMA address.
	 */
	struct dma_buf *pin;
	bool quarantined;
};
static struct tdp_ctx *file_ctx(struct file *f)
{
	return container_of(f->private_data, struct tdp_ctx, fh);
}
static struct v4l2_pix_format *tdp_format(struct tdp_ctx *ctx, unsigned int type)
{
	return V4L2_TYPE_IS_OUTPUT(type) ? &ctx->src : &ctx->dst;
}
static void tdp_write(struct tdp_dev *tdp, u32 reg, u32 value)
{
	writel_relaxed(value, tdp->regs + reg);
}

static irqreturn_t tdp_irq(int irq, void *data)
{
	struct tdp_dev *tdp = data;
	u32 status = readl(tdp->regs + TDP_STATUS) & 0xf;

	if (!status)
		return IRQ_NONE;
	writel(status, tdp->regs + TDP_IRQ_CLEAR);
	if (status & (TDP_DONE | TDP_ERRORS)) {
		tdp->irq_status = status;
		complete(&tdp->done);
	}
	return IRQ_HANDLED;
}

static int tdp_runtime_resume(struct device *dev)
{
	struct tdp_dev *tdp = dev_get_drvdata(dev);
	int ret = clk_bulk_prepare_enable(ARRAY_SIZE(tdp->clocks), tdp->clocks);

	if (ret)
		return ret;
	ret = reset_control_deassert(tdp->resets);
	if (ret) {
		clk_bulk_disable_unprepare(ARRAY_SIZE(tdp->clocks), tdp->clocks);
		return ret;
	}
	writel(0xf, tdp->regs + TDP_IRQ_MASK);
	writel(0xf, tdp->regs + TDP_IRQ_CLEAR);
	writel(1, tdp->regs + TDP_AXI_ENABLE);
	return 0;
}

static int tdp_runtime_suspend(struct device *dev)
{
	struct tdp_dev *tdp = dev_get_drvdata(dev);
	u32 busy;
	int ret;

	writel(0xf, tdp->regs + TDP_IRQ_MASK);
	writel(0, tdp->regs + TDP_AXI_ENABLE);
	ret = readl_poll_timeout(tdp->regs + TDP_AXI_BUSY, busy, !busy, 10, 10000);
	if (ret) {
		writel(1, tdp->regs + TDP_AXI_ENABLE);
		return -EBUSY;
	}
	clk_bulk_disable_unprepare(ARRAY_SIZE(tdp->clocks), tdp->clocks);
	return 0;
}

/* Disable requests and drain AXI before resetting or dropping DMA references.
 * A permanently busy engine retains its mappings and power until reboot.
 */
static int tdp_abort(struct tdp_dev *tdp)
{
	u32 busy;
	int ret;

	writel(0xf, tdp->regs + TDP_IRQ_MASK);
	writel(0, tdp->regs + TDP_AXI_ENABLE);
	ret = readl_poll_timeout(tdp->regs + TDP_AXI_BUSY, busy, !busy, 10, 100000);
	if (ret)
		return ret;
	ret = reset_control_assert(tdp->resets);
	if (ret)
		return ret;
	udelay(2);
	ret = reset_control_deassert(tdp->resets);
	if (ret)
		return ret;
	writel(0xf, tdp->regs + TDP_IRQ_CLEAR);
	writel(1, tdp->regs + TDP_AXI_ENABLE);
	return 0;
}

static void tdp_program(struct tdp_ctx *ctx, dma_addr_t input, dma_addr_t output)
{
	struct tdp_dev *tdp = ctx->tdp;
	const struct v4l2_pix_format *s = &ctx->src, *d = &ctx->dst;
	const struct v4l2_rect *c = &ctx->crop;
	u32 src_width = c->width, src_height = c->height;
	u32 src_stride = s->bytesperline, dst_stride = d->bytesperline;
	u32 dst_width = d->width, dst_height = d->height;
	dma_addr_t chroma = input + (u64)src_stride * s->height;
	bool nv12 = s->pixelformat == V4L2_PIX_FMT_NV12;
	bool limited = s->quantization != V4L2_QUANTIZATION_FULL_RANGE;
	bool bt709 = s->ycbcr_enc == V4L2_YCBCR_ENC_709;
	int cy = limited ? 298 : 256;
	int bu = bt709 ? (limited ? 541 : 475) : (limited ? 516 : 454);
	int gu = bt709 ? (limited ? -55 : -48) : (limited ? -100 : -88);
	int gv = bt709 ? (limited ? -136 : -120) : (limited ? -208 : -183);
	int rv = bt709 ? (limited ? 459 : 403) : (limited ? 409 : 359);
	u32 sx = (src_width << 10) / dst_width;
	u32 sy = (src_height << 10) / dst_height;
	s32 px = ((s32)sx - 1024) >> 1;
	s32 py = ((s32)sy - 1024) >> 1;
	static const u16 clear_regs[] = {
		0x034, 0x03c, 0x058, 0x0b4, 0x0d4, 0x0d8, 0x0dc, 0x0e0,
		0x0e4, 0x0e8, 0x0ec, 0x0f0, 0x0f4, 0x0f8, 0x15c, 0x290,
		0x2c0, 0x310, 0x314,
	};
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(clear_regs); i++)
		tdp_write(tdp, clear_regs[i], 0);
	tdp_write(tdp, 0x020, 1);
	tdp_write(tdp, 0x024, src_width | src_height << 16);
	/* Vendor global NV12 (3) maps to IVPS 0x800a, then reader code 0x0a.
	 * Code 0x10 is single-plane luma, not a semiplanar YUV reader.
	 */
	tdp_write(tdp, 0x028, nv12 ? 0x0a : 0x13);
	tdp_write(tdp, 0x040, 0);
	tdp_write(tdp, 0x044, input + (u64)c->top * src_stride + c->left * (nv12 ? 1 : 2));
	tdp_write(tdp, 0x048, src_stride);
	tdp_write(tdp, 0x04c, 0);
	tdp_write(tdp, 0x050, nv12 ? chroma + (u64)(c->top / 2) * src_stride + c->left : input);
	tdp_write(tdp, 0x054, src_stride);
	tdp_write(tdp, 0x0b8, sx != 1024 || sy != 1024);
	tdp_write(tdp, 0x0bc, dst_width | dst_height << 16);
	tdp_write(tdp, 0x0c0, sx | sy << 16);
	/* Pixel-centre phases in Q10, including negative phases for upscaling.
	 * A fixed 1024 phase only works for a 3:1 reduction; other factors can
	 * make the scaler wait for pixels beyond the input frame.
	 */
	tdp_write(tdp, 0x0c4, py);
	tdp_write(tdp, 0x0c8, px);
	tdp_write(tdp, 0x0cc, py);
	tdp_write(tdp, 0x0d0, px);
	/* B/G/R rows, signed 11-bit coefficients, eight fractional bits.
	 * Full-range BT.601 defaults are replaced by limited-range BT.709.
	 */
	tdp_write(tdp, 0x250, 1);
	tdp_write(tdp, 0x254, cy);
	tdp_write(tdp, 0x258, bu);
	tdp_write(tdp, 0x25c, 0);
	tdp_write(tdp, 0x260, cy);
	tdp_write(tdp, 0x264, gu & 0x7ff);
	tdp_write(tdp, 0x268, gv & 0x7ff);
	tdp_write(tdp, 0x26c, cy);
	tdp_write(tdp, 0x270, 0);
	tdp_write(tdp, 0x274, rv);
	/* Input offsets are subtracted before CSC. Chroma is centred by the
	 * packed-YUYV reader; subtract the limited-range luma pedestal here.
	 */
	tdp_write(tdp, 0x278, limited ? 16 : 0);
	for (i = 0x27c; i <= 0x28c; i += 4)
		tdp_write(tdp, i, 0);
	tdp_write(tdp, 0x29c, 1);
	tdp_write(tdp, 0x2a0, 1);
	tdp_write(tdp, 0x2a4, 0x28); /* little-endian RGB565 */
	tdp_write(tdp, 0x2a8, 0);
	tdp_write(tdp, 0x2ac, output);
	tdp_write(tdp, 0x2b0, dst_stride);
	tdp_write(tdp, 0x2b4, 0);
	tdp_write(tdp, 0x2b8, output);
	tdp_write(tdp, 0x2bc, dst_stride);
	writel(0xf, tdp->regs + TDP_IRQ_CLEAR);
	writel(2, tdp->regs + TDP_IRQ_MASK); /* mask start, enable completion/error */
	writel(4, tdp->regs + 0x2fc);
	/* Orders coefficients, dimensions and both DMA buffers before start. */
	writel(1, tdp->regs + TDP_START);
}


static int tdp_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	bool out = V4L2_TYPE_IS_OUTPUT(f->type);
	u32 bpp, stride, size;

	if (out && mp->pixelformat != V4L2_PIX_FMT_NV12)
		mp->pixelformat = V4L2_PIX_FMT_YUYV;
	if (!out)
		mp->pixelformat = V4L2_PIX_FMT_RGB565;
	mp->width = clamp_t(u32, mp->width, 2, TDP_MAX_W) & ~1;
	mp->height = clamp_t(u32, mp->height, 2, TDP_MAX_H);
	if (mp->pixelformat == V4L2_PIX_FMT_NV12)
		mp->height = ALIGN(mp->height, 2);
	bpp = mp->pixelformat == V4L2_PIX_FMT_NV12 ? 1 : 2;
	stride = ALIGN(clamp_t(u32, mp->plane_fmt[0].bytesperline,
				mp->width * bpp, 16384), 16);
	size = stride * mp->height;
	if (mp->pixelformat == V4L2_PIX_FMT_NV12)
		size += size / 2;
	mp->field = V4L2_FIELD_NONE;
	mp->num_planes = 1;
	memset(mp->reserved, 0, sizeof(mp->reserved));
	memset(mp->plane_fmt, 0, sizeof(mp->plane_fmt));
	mp->plane_fmt[0].bytesperline = stride;
	mp->plane_fmt[0].sizeimage = size;
	if (out) {
		if (mp->colorspace != V4L2_COLORSPACE_REC709 &&
		    mp->colorspace != V4L2_COLORSPACE_SMPTE170M &&
		    mp->colorspace != V4L2_COLORSPACE_JPEG)
			mp->colorspace = V4L2_COLORSPACE_REC709;
		if (!mp->ycbcr_enc)
			mp->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(mp->colorspace);
		if (mp->ycbcr_enc != V4L2_YCBCR_ENC_601 &&
		    mp->ycbcr_enc != V4L2_YCBCR_ENC_709)
			return -EINVAL;
		if (!mp->quantization)
			mp->quantization = V4L2_MAP_QUANTIZATION_DEFAULT(false,
				mp->colorspace, mp->ycbcr_enc);
		if (mp->quantization != V4L2_QUANTIZATION_FULL_RANGE &&
		    mp->quantization != V4L2_QUANTIZATION_LIM_RANGE)
			return -EINVAL;
	} else {
		struct tdp_ctx *ctx = file_ctx(file);

		mp->colorspace = ctx->src.colorspace;
		mp->ycbcr_enc = ctx->src.ycbcr_enc;
		mp->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	}
	if (!mp->xfer_func)
		mp->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(mp->colorspace);
	return 0;
}
static int tdp_s_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct tdp_ctx *ctx = file_ctx(file);
	struct v4l2_pix_format *pix = tdp_format(ctx, f->type);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	int ret;

	if (vb2_is_busy(v4l2_m2m_get_src_vq(ctx->fh.m2m_ctx)) ||
	    vb2_is_busy(v4l2_m2m_get_dst_vq(ctx->fh.m2m_ctx)))
		return -EBUSY;
	ret = tdp_try_fmt(file, priv, f);
	if (ret)
		return ret;
	*pix = (struct v4l2_pix_format) {
		.width = mp->width, .height = mp->height,
		.pixelformat = mp->pixelformat, .field = mp->field,
		.bytesperline = mp->plane_fmt[0].bytesperline,
		.sizeimage = mp->plane_fmt[0].sizeimage,
		.colorspace = mp->colorspace, .ycbcr_enc = mp->ycbcr_enc,
		.quantization = mp->quantization, .xfer_func = mp->xfer_func,
	};
	if (V4L2_TYPE_IS_OUTPUT(f->type))
		ctx->crop = (struct v4l2_rect){ 0, 0, pix->width, pix->height };
	return 0;
}
static int tdp_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct v4l2_pix_format *p = tdp_format(file_ctx(file), f->type);

	f->fmt.pix_mp = (struct v4l2_pix_format_mplane){
		.width = p->width, .height = p->height, .pixelformat = p->pixelformat,
		.field = p->field, .colorspace = p->colorspace, .num_planes = 1,
		.ycbcr_enc = p->ycbcr_enc, .quantization = p->quantization,
		.xfer_func = p->xfer_func,
		.plane_fmt[0] = { .sizeimage = p->sizeimage, .bytesperline = p->bytesperline },
	};
	return 0;
}
static int tdp_enum_fmt(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		if (f->index > 1)
			return -EINVAL;
		f->pixelformat = f->index ? V4L2_PIX_FMT_NV12 : V4L2_PIX_FMT_YUYV;
	} else {
		if (f->index)
			return -EINVAL;
		f->pixelformat = V4L2_PIX_FMT_RGB565;
	}
	return 0;
}
static int tdp_querycap(struct file *file, void *priv, struct v4l2_capability *cap)
{
	strscpy(cap->driver, "ax630c-tdp", sizeof(cap->driver));
	strscpy(cap->card, "AX630C TDP", sizeof(cap->card));
	strscpy(cap->bus_info, "platform:ax630c-tdp", sizeof(cap->bus_info));
	return 0;
}
static int tdp_g_selection(struct file *file, void *priv, struct v4l2_selection *s)
{
	struct tdp_ctx *ctx = file_ctx(file);

	if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;
	switch (s->target) {
	case V4L2_SEL_TGT_CROP:
		s->r = ctx->crop;
		break;
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_CROP_DEFAULT:
		s->r = (struct v4l2_rect){ 0, 0, ctx->src.width, ctx->src.height };
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
static int tdp_s_selection(struct file *file, void *priv, struct v4l2_selection *s)
{
	struct tdp_ctx *ctx = file_ctx(file);
	struct v4l2_rect r = s->r;

	if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT || s->target != V4L2_SEL_TGT_CROP || s->flags)
		return -EINVAL;
	if (vb2_is_busy(v4l2_m2m_get_src_vq(ctx->fh.m2m_ctx)) ||
	    vb2_is_busy(v4l2_m2m_get_dst_vq(ctx->fh.m2m_ctx)))
		return -EBUSY;
	if (r.left < 0 || r.top < 0 || r.width < 2 || r.height < 2 ||
	    (u64)r.left + r.width > ctx->src.width ||
	    (u64)r.top + r.height > ctx->src.height || (r.left & 1) ||
	    (r.width & 1) || (ctx->src.pixelformat == V4L2_PIX_FMT_NV12 && ((r.top | r.height) & 1)))
		return -EINVAL;
	ctx->crop = r;
	return 0;
}
static int tdp_queue_setup(struct vb2_queue *q, unsigned int *count,
			 unsigned int *planes, unsigned int sizes[], struct device *alloc_devs[])
{
	struct tdp_ctx *ctx = vb2_get_drv_priv(q);
	u32 size = tdp_format(ctx, q->type)->sizeimage;

	if (*planes)
		return *planes != 1 || sizes[0] < size ? -EINVAL : 0;
	*planes = 1;
	sizes[0] = size;
	return 0;
}
static int tdp_buf_prepare(struct vb2_buffer *vb)
{
	struct tdp_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct tdp_buffer *b = container_of(to_vb2_v4l2_buffer(vb), struct tdp_buffer, m2m.vb);
	unsigned int size = tdp_format(ctx, vb->type)->sizeimage;
	unsigned int offset = vb->planes[0].data_offset;
	dma_addr_t address = vb2_dma_contig_plane_dma_addr(vb, 0);

	if (ctx->tdp->poisoned)
		return -EIO;
	if (vb2_plane_size(vb, 0) < (u64)size + offset ||
	    (u64)address + size + offset > (1ULL << 32) || (offset & 15))
		return -EINVAL;
	if (V4L2_TYPE_IS_OUTPUT(vb->type) && vb2_get_plane_payload(vb, 0) < (u64)size + offset)
		return -EINVAL;
	if (!b->pin || (vb->memory == V4L2_MEMORY_DMABUF &&
		       b->pin != vb->planes[0].dbuf)) {
		if (b->pin)
			dma_buf_put(b->pin);
		b->pin = vb->planes[0].dbuf;
		if (b->pin)
			get_dma_buf(b->pin);
		else
			b->pin = vb2_dma_contig_memops.get_dmabuf(vb, vb->planes[0].mem_priv, O_CLOEXEC);
		if (IS_ERR_OR_NULL(b->pin)) {
			int ret = b->pin ? PTR_ERR(b->pin) : -ENOMEM;

			b->pin = NULL;
			return ret;
		}
	}
	vb2_set_plane_payload(vb, 0, size + offset);
	return 0;
}
static void tdp_buf_cleanup(struct vb2_buffer *vb)
{
	struct tdp_buffer *b = container_of(to_vb2_v4l2_buffer(vb), struct tdp_buffer, m2m.vb);

	if (b->pin && !b->quarantined)
		dma_buf_put(b->pin);
	/* vb2 also calls cleanup when an imported slot changes DMA-BUF. The
	 * vb2_buffer itself survives and is prepared again; never leave its
	 * old reference behind to be put a second time. Quarantined storage
	 * deliberately keeps its reference until reboot.
	 */
	b->pin = NULL;
}
static void tdp_buf_queue(struct vb2_buffer *vb)
{
	struct tdp_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, to_vb2_v4l2_buffer(vb));
}
static int tdp_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct tdp_ctx *ctx = vb2_get_drv_priv(q);

	if (V4L2_TYPE_IS_OUTPUT(q->type))
		ctx->src_sequence = 0;
	else
		ctx->dst_sequence = 0;
	return 0;
}
static void tdp_stop_streaming(struct vb2_queue *q)
{
	struct tdp_ctx *ctx = vb2_get_drv_priv(q);
	struct vb2_v4l2_buffer *b;

	/* m2m STREAMOFF has already waited for the bounded worker / abort. */
	while ((b = V4L2_TYPE_IS_OUTPUT(q->type) ? v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx) :
		v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx)))
		v4l2_m2m_buf_done(b, VB2_BUF_STATE_ERROR);
}
static const struct vb2_ops tdp_queue_ops = {
	.queue_setup = tdp_queue_setup, .buf_prepare = tdp_buf_prepare,
	.buf_cleanup = tdp_buf_cleanup, .buf_queue = tdp_buf_queue,
	.start_streaming = tdp_start_streaming, .stop_streaming = tdp_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare, .wait_finish = vb2_ops_wait_finish,
};
static int tdp_queue_init(void *priv, struct vb2_queue *src, struct vb2_queue *dst)
{
	struct tdp_ctx *ctx = priv;
	struct vb2_queue *queues[] = { src, dst };
	unsigned int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(queues); i++) {
		struct vb2_queue *q = queues[i];

		q->type = i ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		q->io_modes = VB2_MMAP | VB2_DMABUF;
		q->drv_priv = ctx;
		q->buf_struct_size = sizeof(struct tdp_buffer);
		q->ops = &tdp_queue_ops;
		q->mem_ops = &vb2_dma_contig_memops;
		q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
		q->lock = &ctx->tdp->lock;
		q->dev = ctx->tdp->dev;
		q->allow_cache_hints = 1;
		q->dma_attrs = DMA_ATTR_ALLOC_SINGLE_PAGES;
		ret = vb2_queue_init(q);
		if (ret)
			return ret;
	}
	return 0;
}
static void tdp_work(struct work_struct *work)
{
	struct tdp_dev *tdp = container_of(work, struct tdp_dev, work);
	struct tdp_ctx *ctx = v4l2_m2m_get_curr_priv(tdp->m2m);
	struct vb2_v4l2_buffer *src = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	struct vb2_v4l2_buffer *dst = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
	struct tdp_buffer *s = container_of(src, struct tdp_buffer, m2m.vb);
	struct tdp_buffer *d = container_of(dst, struct tdp_buffer, m2m.vb);
	const struct v4l2_rect *c = &ctx->crop;
	u32 busy;
	int ret;

	if (tdp->poisoned || c->width > ctx->dst.width * 32 ||
	    ctx->dst.width > c->width * 32 || c->height > ctx->dst.height * 32 ||
	    ctx->dst.height > c->height * 32) {
		ret = -EINVAL;
		goto finish;
	}
	ret = pm_runtime_resume_and_get(tdp->dev);
	if (ret < 0)
		goto finish;
	reinit_completion(&tdp->done);
	tdp->irq_status = 0;
	atomic64_inc(&tdp->jobs);
	tdp_program(ctx, vb2_dma_contig_plane_dma_addr(&src->vb2_buf, 0) + src->vb2_buf.planes[0].data_offset,
		vb2_dma_contig_plane_dma_addr(&dst->vb2_buf, 0) + dst->vb2_buf.planes[0].data_offset);
	ret = wait_for_completion_timeout(&tdp->done, msecs_to_jiffies(100)) ?
		(tdp->irq_status & TDP_ERRORS ? -EIO : 0) : -ETIMEDOUT;
	/* Processing DONE can precede the last AXI write response. A following
	 * context may close/reuse these pages immediately, so retire the bus
	 * transactions before publishing either buffer or starting the next job.
	 */
	if (!ret)
		ret = readl_poll_timeout(tdp->regs + TDP_AXI_BUSY, busy, !busy, 2, 10000);
	if (ret) {
		atomic64_inc(&tdp->errors);
		if (ret == -ETIMEDOUT)
			atomic64_inc(&tdp->timeouts);
		dev_err_ratelimited(tdp->dev, "transform failed: %d, status %#x\n",
			ret, readl(tdp->regs + TDP_STATUS));
		if (tdp_abort(tdp)) {
			tdp->poisoned = true;
			s->quarantined = d->quarantined = true;
			__module_get(THIS_MODULE);
			dev_err(tdp->dev, "DMA did not drain; retaining buffers and power until reboot\n");
			goto finish;
		}
	}
	writel(0xf, tdp->regs + TDP_IRQ_MASK);
	pm_runtime_mark_last_busy(tdp->dev);
	pm_runtime_put_autosuspend(tdp->dev);
finish:
	dma_rmb();
	src = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
	dst = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
	v4l2_m2m_buf_copy_metadata(src, dst, true);
	src->sequence = ctx->src_sequence++;
	dst->sequence = ctx->dst_sequence++;
	v4l2_m2m_buf_done(src, ret ? VB2_BUF_STATE_ERROR : VB2_BUF_STATE_DONE);
	v4l2_m2m_buf_done(dst, ret ? VB2_BUF_STATE_ERROR : VB2_BUF_STATE_DONE);
	v4l2_m2m_job_finish(tdp->m2m, ctx->fh.m2m_ctx);
}
static void tdp_device_run(void *priv)
{
	struct tdp_ctx *ctx = priv;

	schedule_work(&ctx->tdp->work);
}
static void tdp_job_abort(void *priv)
{
	struct tdp_ctx *ctx = priv;

	/* No mutex used by work; completion/reset wait is at most 200 ms. */
	flush_work(&ctx->tdp->work);
}
static const struct v4l2_m2m_ops tdp_m2m_ops = {
	.device_run = tdp_device_run, .job_abort = tdp_job_abort,
};
static int tdp_open(struct file *file)
{
	struct tdp_dev *tdp = video_drvdata(file);
	struct tdp_ctx *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	ctx->tdp = tdp;
	ctx->src = (struct v4l2_pix_format){ .width = 640, .height = 360,
		.pixelformat = V4L2_PIX_FMT_YUYV, .bytesperline = 1280, .sizeimage = 640*360*2,
		.field = V4L2_FIELD_NONE, .colorspace = V4L2_COLORSPACE_REC709,
		.ycbcr_enc = V4L2_YCBCR_ENC_709, .quantization = V4L2_QUANTIZATION_LIM_RANGE,
		.xfer_func = V4L2_XFER_FUNC_709 };
	ctx->dst = ctx->src;
	ctx->dst.pixelformat = V4L2_PIX_FMT_RGB565;
	ctx->dst.quantization = V4L2_QUANTIZATION_FULL_RANGE;
	ctx->crop = (struct v4l2_rect){ 0, 0, 640, 360 };
	v4l2_fh_init(&ctx->fh, &tdp->video);
	file->private_data = &ctx->fh;
	mutex_lock(&tdp->lock);
	ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(tdp->m2m, ctx, tdp_queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		mutex_unlock(&tdp->lock);
		v4l2_fh_exit(&ctx->fh);
		kfree(ctx);
		return ret;
	}
	v4l2_fh_add(&ctx->fh, file);
	mutex_unlock(&tdp->lock);
	return 0;
}
static int tdp_release(struct file *file)
{
	struct tdp_ctx *ctx = file_ctx(file);

	mutex_lock(&ctx->tdp->lock);
	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
	v4l2_fh_del(&ctx->fh, file);
	v4l2_fh_exit(&ctx->fh);
	mutex_unlock(&ctx->tdp->lock);
	kfree(ctx);
	return 0;
}
static const struct v4l2_ioctl_ops tdp_ioctl_ops = {
	.vidioc_querycap = tdp_querycap,
	.vidioc_enum_fmt_vid_out = tdp_enum_fmt, .vidioc_enum_fmt_vid_cap = tdp_enum_fmt,
	.vidioc_g_fmt_vid_out_mplane = tdp_g_fmt, .vidioc_g_fmt_vid_cap_mplane = tdp_g_fmt,
	.vidioc_try_fmt_vid_out_mplane = tdp_try_fmt, .vidioc_try_fmt_vid_cap_mplane = tdp_try_fmt,
	.vidioc_s_fmt_vid_out_mplane = tdp_s_fmt, .vidioc_s_fmt_vid_cap_mplane = tdp_s_fmt,
	.vidioc_g_selection = tdp_g_selection, .vidioc_s_selection = tdp_s_selection,
	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs, .vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_qbuf = v4l2_m2m_ioctl_qbuf, .vidioc_dqbuf = v4l2_m2m_ioctl_dqbuf,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,
	.vidioc_streamon = v4l2_m2m_ioctl_streamon, .vidioc_streamoff = v4l2_m2m_ioctl_streamoff,
};
static const struct v4l2_file_operations tdp_fops = {
	.owner = THIS_MODULE, .open = tdp_open, .release = tdp_release,
	.poll = v4l2_m2m_fop_poll, .unlocked_ioctl = video_ioctl2,
	.mmap = v4l2_m2m_fop_mmap,
};
static ssize_t stats_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct tdp_dev *tdp = dev_get_drvdata(dev);

	return sysfs_emit(buf, "jobs %lld\nerrors %lld\ntimeouts %lld\npoisoned %u\n",
		atomic64_read(&tdp->jobs), atomic64_read(&tdp->errors),
		atomic64_read(&tdp->timeouts), READ_ONCE(tdp->poisoned));
}
static DEVICE_ATTR_RO(stats);
static struct attribute *tdp_attrs[] = { &dev_attr_stats.attr, NULL };
ATTRIBUTE_GROUPS(tdp);
static const struct dev_pm_ops tdp_pm_ops = {
	RUNTIME_PM_OPS(tdp_runtime_suspend, tdp_runtime_resume, NULL)
};
static int tdp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct tdp_dev *tdp;
	int ret, irq;

	tdp = devm_kzalloc(dev, sizeof(*tdp), GFP_KERNEL);
	if (!tdp)
		return -ENOMEM;
	tdp->dev = dev;
	tdp->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(tdp->regs))
		return PTR_ERR(tdp->regs);
	tdp->clocks[0].id = "core";
	tdp->clocks[1].id = "pclk";
	ret = devm_clk_bulk_get(dev, ARRAY_SIZE(tdp->clocks), tdp->clocks);
	if (ret)
		return dev_err_probe(dev, ret, "clocks\n");
	tdp->resets = devm_reset_control_array_get_exclusive(dev);
	if (IS_ERR(tdp->resets))
		return dev_err_probe(dev, PTR_ERR(tdp->resets), "resets\n");
	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;
	mutex_init(&tdp->lock);
	init_completion(&tdp->done);
	INIT_WORK(&tdp->work, tdp_work);
	platform_set_drvdata(pdev, tdp);
	ret = tdp_runtime_resume(dev);
	if (ret)
		return ret;
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto suspend;
	}
	ret = devm_request_irq(dev, irq, tdp_irq, 0, dev_name(dev), tdp);
	if (ret)
		goto suspend;
	ret = v4l2_device_register(dev, &tdp->v4l2);
	if (ret)
		goto suspend;
	tdp->m2m = v4l2_m2m_init(&tdp_m2m_ops);
	if (IS_ERR(tdp->m2m)) {
		ret = PTR_ERR(tdp->m2m);
		goto unregister;
	}
	pm_runtime_set_active(dev);
	pm_runtime_set_autosuspend_delay(dev, 1000);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_enable(dev);
	strscpy(tdp->video.name, "ax630c-tdp", sizeof(tdp->video.name));
	tdp->video.v4l2_dev = &tdp->v4l2;
	tdp->video.fops = &tdp_fops;
	tdp->video.ioctl_ops = &tdp_ioctl_ops;
	tdp->video.release = video_device_release_empty;
	tdp->video.lock = &tdp->lock;
	tdp->video.vfl_dir = VFL_DIR_M2M;
	tdp->video.device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
	video_set_drvdata(&tdp->video, tdp);
	ret = video_register_device(&tdp->video, VFL_TYPE_VIDEO, -1);
	if (ret) {
		pm_runtime_disable(dev);
		v4l2_m2m_release(tdp->m2m);
		goto unregister;
	}
	dev_info(dev, "V4L2 YUYV/NV12 to scaled RGB565 as %s, version %#x\n",
		video_device_node_name(&tdp->video), readl(tdp->regs));
	pm_runtime_mark_last_busy(dev);
	pm_runtime_idle(dev);
	return 0;
unregister:
	v4l2_device_unregister(&tdp->v4l2);
suspend:
	tdp_runtime_suspend(dev);
	return ret;
}
static void tdp_remove(struct platform_device *pdev)
{
	struct tdp_dev *tdp = platform_get_drvdata(pdev);

	video_unregister_device(&tdp->video);
	cancel_work_sync(&tdp->work);
	v4l2_m2m_release(tdp->m2m);
	v4l2_device_unregister(&tdp->v4l2);
	pm_runtime_disable(&pdev->dev);
	/* The platform device survives module unload/reload. Do not take an
	 * unbalanced runtime reference here: a later probe would inherit it
	 * and could never autosuspend. No jobs remain after cancel_work_sync.
	 */
	if (!tdp->poisoned && !pm_runtime_status_suspended(&pdev->dev) &&
	    !tdp_runtime_suspend(&pdev->dev))
		pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_dont_use_autosuspend(&pdev->dev);
}
static const struct of_device_id tdp_of_match[] = {
	{ .compatible = "axera,ax630c-tdp" }, { }
};
MODULE_DEVICE_TABLE(of, tdp_of_match);
static struct platform_driver tdp_driver = {
	.probe = tdp_probe, .remove = tdp_remove,
	.driver = {
		.name = "ax630c-tdp", .of_match_table = tdp_of_match,
		.pm = pm_ptr(&tdp_pm_ops), .dev_groups = tdp_groups,
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(tdp_driver);
MODULE_DESCRIPTION("Axera AX630C TDP V4L2 mem2mem scaler / CSC");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS("DMA_BUF");
