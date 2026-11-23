// SPDX-License-Identifier: GPL-2.0-only
/*
 * HiSilicon JPEG decoder (JPGD) V4L2 mem2mem driver
 *
 * OUTPUT queue: one complete baseline JPEG file per buffer. CAPTURE queue:
 * the decoded picture as NV21, NV61, NV42 or GREY (one memory plane, the
 * chroma plane directly behind the luma plane). The driver parses the JPEG
 * header when the buffer is queued, converts the quantization and Huffman
 * tables for the hardware and reports the picture size with a source change
 * event, like the other JPEG decoders of this kind (s5p-jpeg, mtk-jpeg).
 *
 * The block has no register description in any data sheet: the layout was
 * taken from the vendor user-space library and kernel shim.
 *
 * The CAPTURE format follows the JPEG as long as no CAPTURE buffer exists: a
 * newly queued file changes it and sends V4L2_EVENT_SOURCE_CHANGE. Once
 * buffers are allocated the format is frozen: a file that does not fit it
 * sends the event too and its job fails with V4L2_BUF_FLAG_ERROR, so that
 * user space frees the buffers and reads the format again.
 */

#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-contig.h>

#include "hisi-jpgd.h"

#define JPGD_DEF_WIDTH		640
#define JPGD_DEF_HEIGHT		480

/*
 * The block is the same on both SoCs as far as the vendor libraries show: the
 * register offsets, the table memory formats and the limits agree. What
 * differs is outside the block (window, interrupt and CRG bits come from the
 * device tree) and what the vendor software leaves in SCALE.
 */
static const struct hisi_jpgd_variant hisi_jpgd_hi3520dv100 = {
	.name = "Hi3520D V100 JPGD",
	.min_dim = 1,
	.max_dim = 8096,
	.picsize_bad = 0xfc00fc00,
	/* outstanding 7 (written by the vendor load script) | jidct_emar_en */
	.scale_set = 0x78,
	.idle_timeout = 0x00ffffff,
	/* read on the board after every reset release: outstanding 3 and bit 7 */
	.idle_scale = 0xb0,
	/* the conversion to 4:2:0 gave wrong chroma on this SoC: not offered */
};

static const struct hisi_jpgd_variant hisi_jpgd_hi3536dv100 = {
	.name = "Hi3536D V100 JPGD",
	/* the data sheet says 8 x 8, the block decodes 1 x 1 up like the vendor library */
	.min_dim = 1,
	.max_dim = 8096,
	.picsize_bad = 0xfc00fc00,
	/*
	 * The reset value 0x34 (outstanding 3, bit 2) plus jidct_emar_en, which
	 * is what the vendor library ends up writing (0x3c): every field the
	 * block needs is programmed, only the unknown bits [31:7] are kept. A
	 * higher outstanding count gave no gain on the board (outstanding 0
	 * is slower), so the vendor value is used.
	 */
	.scale_keep = (u32)~(JPGD_SCALE_SHIFT_MASK | JPGD_SCALE_BIT2 |
			     JPGD_SCALE_EMAR_EN | JPGD_SCALE_OUTSTD),
	.scale_set = JPGD_SCALE_BIT2 | JPGD_SCALE_EMAR_EN |
		     FIELD_PREP_CONST(JPGD_SCALE_OUTSTD, 3),
	/* read on the board: TIMEOUT is all ones, SCALE has bits 2, 4, 5 set */
	.idle_timeout = 0xffffffff,
	.idle_scale = 0x34,
	/* 4:2:2 and 4:4:4 files give a correct NV21 picture (luma exact) */
	.cvt420 = true,
};

static const u32 hisi_jpgd_fourccs[] = {
	V4L2_PIX_FMT_NV21, V4L2_PIX_FMT_NV61, V4L2_PIX_FMT_NV42,
	V4L2_PIX_FMT_GREY,
};

static inline u32 jpgd_read(struct hisi_jpgd_dev *jpgd, u32 off)
{
	return readl(jpgd->base + off);
}

static inline void jpgd_write(struct hisi_jpgd_dev *jpgd, u32 off, u32 val)
{
	writel(val, jpgd->base + off);
}

static inline struct hisi_jpgd_ctx *file_to_ctx(struct file *file)
{
	return container_of(file_to_v4l2_fh(file), struct hisi_jpgd_ctx, fh);
}

static inline struct hisi_jpgd_buf *vb_to_buf(struct vb2_buffer *vb)
{
	return container_of(to_vb2_v4l2_buffer(vb), struct hisi_jpgd_buf,
			    m2m.vb);
}

/* ------------------------------------------------------------ geometry */

/* Output formats the hardware can produce for a JPEG of this sampling */
static bool hisi_jpgd_fmt_allowed(const struct hisi_jpgd_variant *var,
				  u32 fourcc, enum hisi_jpgd_sampling s)
{
	switch (fourcc) {
	case V4L2_PIX_FMT_GREY:
		return s == HISI_JPGD_S400;
	case V4L2_PIX_FMT_NV21:
		return s == HISI_JPGD_S420 ||
		       (var->cvt420 && (s == HISI_JPGD_S422H || s == HISI_JPGD_S444));
	case V4L2_PIX_FMT_NV61:
		return s == HISI_JPGD_S422H;
	case V4L2_PIX_FMT_NV42:
		return s == HISI_JPGD_S444;
	}

	return false;
}

/*
 * The block converts: 4:2:2 or 4:4:4 data written as 4:2:0 (NV21), on a
 * variant that can do it (the Hi3520D result is wrong).
 */
static bool hisi_jpgd_converts(const struct hisi_jpgd_variant *var, u32 fourcc,
			       enum hisi_jpgd_sampling s)
{
	return var->cvt420 && fourcc == V4L2_PIX_FMT_NV21 &&
	       s != HISI_JPGD_S420;
}

static unsigned int hisi_jpgd_max_scale(const struct hisi_jpgd_variant *var,
					u32 fourcc, enum hisi_jpgd_sampling s)
{
	return hisi_jpgd_converts(var, fourcc, s) ? HISI_JPGD_MAX_SCALE_CVT :
						    HISI_JPGD_MAX_SCALE;
}

static u32 hisi_jpgd_native_fourcc(enum hisi_jpgd_sampling s)
{
	switch (s) {
	case HISI_JPGD_S400:
		return V4L2_PIX_FMT_GREY;
	case HISI_JPGD_S422H:
		return V4L2_PIX_FMT_NV61;
	case HISI_JPGD_S444:
		return V4L2_PIX_FMT_NV42;
	default:
		return V4L2_PIX_FMT_NV21;
	}
}

static void hisi_jpgd_calc_layout(const struct hisi_jpgd_info *info, u32 fourcc,
				  unsigned int scale,
				  struct hisi_jpgd_layout *lay)
{
	unsigned int ysize;

	lay->fourcc = fourcc;
	lay->scale = scale;
	lay->width = DIV_ROUND_UP(info->width, 1U << scale);
	lay->height = DIV_ROUND_UP(info->height, 1U << scale);
	lay->ystride = ALIGN(lay->width, HISI_JPGD_STRIDE_ALIGN);
	lay->rows = ALIGN(lay->height, HISI_JPGD_ROW_ALIGN);

	ysize = lay->ystride * lay->rows;
	switch (fourcc) {
	case V4L2_PIX_FMT_NV21:
		lay->uvstride = lay->ystride;
		lay->payload = ysize + ysize / 2;
		lay->sizeimage = lay->payload;
		break;
	case V4L2_PIX_FMT_NV61:
		lay->uvstride = lay->ystride;
		lay->payload = ysize * 2;
		lay->sizeimage = lay->payload;
		break;
	case V4L2_PIX_FMT_NV42:
		lay->uvstride = lay->ystride * 2;
		lay->payload = ysize * 3;
		lay->sizeimage = lay->payload;
		break;
	default:	/* GREY */
		lay->uvstride = 0;
		lay->payload = ysize;
		/* room for a chroma plane in case the block writes one anyway */
		lay->sizeimage = ysize + ysize / 2;
		break;
	}
}

/*
 * The JPEG side is the standard JFIF colorimetry: V4L2_COLORSPACE_JPEG stands
 * for the whole set, the other three fields stay at their defaults.
 */
static void hisi_jpgd_fill_colorimetry(struct v4l2_pix_format_mplane *pix)
{
	pix->field = V4L2_FIELD_NONE;
	pix->colorspace = V4L2_COLORSPACE_JPEG;
	pix->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	pix->quantization = V4L2_QUANTIZATION_DEFAULT;
	pix->xfer_func = V4L2_XFER_FUNC_DEFAULT;
}

/*
 * The block does no colorimetry conversion: the decoded raw picture has the
 * fixed JFIF colorimetry (sRGB primaries and transfer function, BT.601,
 * full range), which is what the V4L2 JPEG codec rules want on the raw side.
 */
static void hisi_jpgd_fill_raw_colorimetry(struct v4l2_pix_format_mplane *pix)
{
	pix->field = V4L2_FIELD_NONE;
	pix->colorspace = V4L2_COLORSPACE_SRGB;
	pix->ycbcr_enc = V4L2_YCBCR_ENC_601;
	pix->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	pix->xfer_func = V4L2_XFER_FUNC_SRGB;
}

/*
 * Adjust a CAPTURE format to what the hardware can produce for the newest
 * JPEG: pixel format by sampling, size by the largest power-of-two reduction
 * that still covers the request (the request is a lower bound: a size of 0
 * asks for the smallest picture, 1/8; the reported height is the aligned row
 * count so that the chroma plane starts at bytesperline * height). The block
 * decodes every size it accepts at scale 0 (the vendor library's 4095 pixel
 * limit belongs to its packed RGB output; checked on the Hi3536D with
 * 4608 x 3456 and 8096 x 2000 files), so no reduction is forced. Before the
 * first file the coded size is unknown: the larger of the OUTPUT format and
 * the request stands in for it, with 4:2:0 sampling.
 */
static void hisi_jpgd_try_fmt_cap(struct hisi_jpgd_ctx *ctx,
				  struct v4l2_pix_format_mplane *pix,
				  struct hisi_jpgd_layout *out)
{
	const struct hisi_jpgd_variant *var = ctx->dev->var;
	struct hisi_jpgd_info guess = {
		.width = clamp(max(ctx->src_fmt.width, pix->width),
			       var->min_dim, var->max_dim),
		.height = clamp(max(ctx->src_fmt.height, pix->height),
				var->min_dim, var->max_dim),
		.sampling = HISI_JPGD_S420,
	};
	const struct hisi_jpgd_info *info = ctx->info_valid ? &ctx->info :
					    &guess;
	struct hisi_jpgd_layout lay;
	unsigned int scale = 0, s;
	u32 fourcc = pix->pixelformat;

	if (ctx->info_valid ?
	    !hisi_jpgd_fmt_allowed(var, fourcc, info->sampling) :
	    !(fourcc == V4L2_PIX_FMT_NV21 || fourcc == V4L2_PIX_FMT_NV61 ||
	      fourcc == V4L2_PIX_FMT_NV42 || fourcc == V4L2_PIX_FMT_GREY))
		fourcc = hisi_jpgd_native_fourcc(info->sampling);

	for (s = hisi_jpgd_max_scale(var, fourcc, info->sampling); s > 0; s--) {
		hisi_jpgd_calc_layout(info, fourcc, s, &lay);
		if (lay.width >= pix->width && lay.rows >= pix->height) {
			scale = s;
			break;
		}
	}
	hisi_jpgd_calc_layout(info, fourcc, scale, &lay);

	memset(pix->reserved, 0, sizeof(pix->reserved));
	memset(pix->plane_fmt, 0, sizeof(pix->plane_fmt));
	pix->pixelformat = fourcc;
	pix->width = lay.width;
	pix->height = lay.rows;
	pix->num_planes = 1;
	pix->plane_fmt[0].bytesperline = lay.ystride;
	pix->plane_fmt[0].sizeimage = lay.sizeimage;
	hisi_jpgd_fill_raw_colorimetry(pix);

	if (out)
		*out = lay;
}

static void hisi_jpgd_fill_out(struct hisi_jpgd_ctx *ctx,
			       struct v4l2_pix_format_mplane *pix)
{
	u32 size = pix->plane_fmt[0].sizeimage;

	if (!size)
		size = HISI_JPGD_DEF_STREAM;
	size = clamp_t(u32, size, HISI_JPGD_MIN_STREAM, HISI_JPGD_MAX_STREAM);

	memset(pix->reserved, 0, sizeof(pix->reserved));
	memset(pix->plane_fmt, 0, sizeof(pix->plane_fmt));
	pix->pixelformat = V4L2_PIX_FMT_JPEG;
	if (ctx->info_valid) {
		pix->width = ctx->info.width;
		pix->height = ctx->info.height;
	} else {
		pix->width = clamp_t(u32, pix->width, ctx->dev->var->min_dim,
				     ctx->dev->var->max_dim);
		pix->height = clamp_t(u32, pix->height, ctx->dev->var->min_dim,
				      ctx->dev->var->max_dim);
	}
	pix->num_planes = 1;
	pix->plane_fmt[0].sizeimage = size;
	hisi_jpgd_fill_colorimetry(pix);
}

/*
 * While the CAPTURE queue has no buffers its format follows the newest JPEG:
 * the sampling decides the pixel format and, unless user space chose a format
 * with S_FMT, the picture is decoded at full size in the native sampling (a
 * size chosen with S_FMT is the lower bound of the reduction; so is the
 * pixel format a request: NV21 for a 4:2:2 or 4:4:4 file on a SoC that
 * converts). With buffers allocated the format must not change:
 * they were sized for it and the block writes the whole picture.
 */
static void hisi_jpgd_sync_cap(struct hisi_jpgd_ctx *ctx)
{
	if (vb2_is_busy(v4l2_m2m_get_vq(ctx->fh.m2m_ctx,
					V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)))
		return;

	if (!ctx->fmt_set) {
		/* No choice of user space yet: native format, full size */
		ctx->dst_fmt.pixelformat = ctx->info_valid ?
			hisi_jpgd_native_fourcc(ctx->info.sampling) :
			V4L2_PIX_FMT_NV21;
		ctx->dst_fmt.width = ctx->info_valid ? ctx->info.width :
				     ctx->src_fmt.width;
		ctx->dst_fmt.height = ctx->info_valid ? ctx->info.height :
				      ctx->src_fmt.height;
	}
	hisi_jpgd_try_fmt_cap(ctx, &ctx->dst_fmt, &ctx->lay);
}

/*
 * The CAPTURE format must describe a layout the hardware can produce for
 * this JPEG: find the scale it encodes.
 */
static int hisi_jpgd_match(struct hisi_jpgd_ctx *ctx,
			   const struct hisi_jpgd_info *info,
			   struct hisi_jpgd_layout *lay)
{
	const struct v4l2_pix_format_mplane *pix = &ctx->dst_fmt;
	struct hisi_jpgd_layout tmp;
	unsigned int s;

	if (!hisi_jpgd_fmt_allowed(ctx->dev->var, pix->pixelformat,
				   info->sampling))
		return -EINVAL;

	for (s = 0;
	     s <= hisi_jpgd_max_scale(ctx->dev->var, pix->pixelformat,
				      info->sampling); s++) {
		hisi_jpgd_calc_layout(info, pix->pixelformat, s, &tmp);
		if (tmp.width == pix->width && tmp.rows == pix->height &&
		    tmp.sizeimage == pix->plane_fmt[0].sizeimage) {
			if (lay)
				*lay = tmp;
			return 0;
		}
	}

	return -EINVAL;
}

/*
 * A new JPEG was queued: remember it, let the CAPTURE format follow it while
 * that is allowed and tell user space when the picture it will get is not the
 * one the CAPTURE format describes (once per change of size or sampling, not
 * once per buffer).
 */
static void hisi_jpgd_new_info(struct hisi_jpgd_ctx *ctx,
			       const struct hisi_jpgd_info *info)
{
	static const struct v4l2_event ev = {
		.type = V4L2_EVENT_SOURCE_CHANGE,
		.u.src_change.changes = V4L2_EVENT_SRC_CH_RESOLUTION,
	};
	bool changed = !ctx->info_valid || info->width != ctx->info.width ||
		       info->height != ctx->info.height ||
		       info->sampling != ctx->info.sampling;
	bool frozen = vb2_is_busy(v4l2_m2m_get_vq(ctx->fh.m2m_ctx,
					V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE));

	ctx->info = *info;
	ctx->info_valid = true;
	ctx->src_fmt.width = info->width;
	ctx->src_fmt.height = info->height;
	hisi_jpgd_sync_cap(ctx);

	/*
	 * With the CAPTURE format frozen the event is for the file that does
	 * not fit it (match() fails): that is how user space learns it has to
	 * renegotiate. match() returns 0 or an errno.
	 */
	if (changed &&
	    (!frozen || hisi_jpgd_match(ctx, info, NULL) != 0))
		v4l2_event_queue_fh(&ctx->fh, &ev);
}

/* -------------------------------------------------------------- hardware */

static void hisi_jpgd_hw_reset(struct hisi_jpgd_dev *jpgd)
{
	u32 val;

	reset_control_assert(jpgd->rst);
	/* The vendor code waits for the busy bit to drop while in reset */
	if (readl_poll_timeout_atomic(jpgd->base + JPGD_START, val,
				      !(val & JPGD_START_BUSY), 10, 1000))
		dev_warn(jpgd->dev, "START still busy in reset\n");
	reset_control_deassert(jpgd->rst);
	usleep_range(10, 20);
}

static void hisi_jpgd_check_idle(struct hisi_jpgd_dev *jpgd)
{
	u32 start = jpgd_read(jpgd, JPGD_START);
	u32 irq = jpgd_read(jpgd, JPGD_INT);
	u32 timeout = jpgd_read(jpgd, JPGD_TIMEOUT);
	u32 scale = jpgd_read(jpgd, JPGD_SCALE);

	dev_info(jpgd->dev, "%s: START 0x%08x INT 0x%08x TIMEOUT 0x%08x SCALE 0x%08x\n",
		 jpgd->var->name, start, irq, timeout, scale);

	if ((start & JPGD_START_GO) || (irq & JPGD_INT_CAUSES) ||
	    timeout != jpgd->var->idle_timeout ||
	    scale != jpgd->var->idle_scale)
		dev_warn(jpgd->dev, "registers after reset release do not match the layout this driver was written for (expected START bit 0 clear, no INT cause, TIMEOUT 0x%08x, SCALE 0x%08x): this may be another block revision, decoding is not reliable\n",
			 jpgd->var->idle_timeout, jpgd->var->idle_scale);

	/* A cause left in INT would end the first job before it ran */
	if (irq & JPGD_INT_CAUSES)
		jpgd_write(jpgd, JPGD_INT, irq & JPGD_INT_CAUSES);
}

static void jpgd_write_words(struct hisi_jpgd_dev *jpgd, u32 off, const u32 *w,
			     unsigned int n)
{
	unsigned int i;

	for (i = 0; i < n; i++)
		jpgd_write(jpgd, off + 4 * i, w[i]);
}

static void hisi_jpgd_hw_start(struct hisi_jpgd_dev *jpgd,
			       const struct hisi_jpgd_buf *src,
			       const struct hisi_jpgd_layout *lay,
			       dma_addr_t sbuf, u32 ssize, u32 spayload,
			       dma_addr_t dbuf)
{
	const struct hisi_jpgd_info *info = &src->info;
	const struct hisi_jpgd_tables *t = &src->tbl;
	static const u8 pictype[] = {
		[HISI_JPGD_S400] = JPGD_PICTYPE_400,
		[HISI_JPGD_S420] = JPGD_PICTYPE_420,
		[HISI_JPGD_S422H] = JPGD_PICTYPE_422H,
		[HISI_JPGD_S444] = JPGD_PICTYPE_444,
	};
	dma_addr_t uv = dbuf + lay->ystride * lay->rows;
	u32 stale = jpgd_read(jpgd, JPGD_INT) & JPGD_INT_CAUSES;

	/* Acknowledge a cause from before this job: it would end the job early */
	if (stale) {
		dev_warn(jpgd->dev, "stale interrupt cause 0x%x before start\n",
			 stale);
		jpgd_write(jpgd, JPGD_INT, stale);
	}
	jpgd_write(jpgd, JPGD_INTMASK, 0);

	jpgd_write_words(jpgd, JPGD_QUANT, t->quant, JPGD_QUANT_WORDS);
	jpgd_write_words(jpgd, JPGD_HDC, t->hdc, JPGD_HDC_WORDS);
	jpgd_write_words(jpgd, JPGD_HACMIN, t->hacmin, JPGD_HAC_WORDS);
	jpgd_write_words(jpgd, JPGD_HACBASE, t->hacbase, JPGD_HAC_WORDS);
	jpgd_write_words(jpgd, JPGD_HACSYM, t->hacsym, JPGD_HACSYM_WORDS);

	jpgd_write(jpgd, JPGD_PICTYPE, pictype[info->sampling]);
	jpgd_write(jpgd, JPGD_SCALE,
		   (jpgd_read(jpgd, JPGD_SCALE) & jpgd->var->scale_keep) |
		   jpgd->var->scale_set | lay->scale);
	jpgd_write(jpgd, JPGD_PICSIZE, info->picsize);
	jpgd_write(jpgd, JPGD_STRIDE, lay->ystride | lay->uvstride << 16);
	jpgd_write(jpgd, JPGD_SAMPLING, info->sampling_reg);
	/* 0 without a restart interval, so no value of an earlier job stays */
	jpgd_write(jpgd, JPGD_DRI, info->restart);

	/* Stream window: the whole OUTPUT buffer; data: the entropy segment */
	jpgd_write(jpgd, JPGD_STADDR, lower_32_bits(sbuf));
	jpgd_write(jpgd, JPGD_ENDADDR, lower_32_bits(sbuf + ssize));
	jpgd_write(jpgd, JPGD_STADD, lower_32_bits(sbuf + info->ecs_offset));
	jpgd_write(jpgd, JPGD_ENDADD, lower_32_bits(sbuf + spayload));
	jpgd_write(jpgd, JPGD_YSTADDR, lower_32_bits(dbuf));
	jpgd_write(jpgd, JPGD_UVSTADDR, lower_32_bits(uv));
	/* native sampling, or the 4:2:0 conversion of the block for NV21 */
	jpgd_write(jpgd, JPGD_OUTTYPE,
		   hisi_jpgd_converts(jpgd->var, lay->fourcc, info->sampling) ?
		   JPGD_OUTTYPE_420SP : 0);

	/* The whole stream is one segment */
	jpgd_write(jpgd, JPGD_RESUME, JPGD_RESUME_LAST);
	jpgd_write(jpgd, JPGD_START, JPGD_START_GO);
}

static const struct v4l2_event hisi_jpgd_eos_event = {
	.type = V4L2_EVENT_EOS
};

/*
 * Number both buffers of a finished job. The job that holds the last OUTPUT
 * buffer queued before a DECODER_CMD STOP ends the drain: its CAPTURE buffer
 * carries V4L2_BUF_FLAG_LAST and the context counts as stopped.
 */
static void hisi_jpgd_finish_bufs(struct hisi_jpgd_ctx *ctx,
				  struct vb2_v4l2_buffer *src,
				  struct vb2_v4l2_buffer *dst)
{
	v4l2_m2m_buf_copy_metadata(src, dst);
	src->sequence = ctx->sequence_out++;
	dst->sequence = ctx->sequence++;
	dst->field = V4L2_FIELD_NONE;

	if (v4l2_m2m_is_last_draining_src_buf(ctx->fh.m2m_ctx, src)) {
		dst->flags |= V4L2_BUF_FLAG_LAST;
		v4l2_m2m_mark_stopped(ctx->fh.m2m_ctx);
		v4l2_event_queue_fh(&ctx->fh, &hisi_jpgd_eos_event);
	}
}

static void hisi_jpgd_job_done(struct hisi_jpgd_dev *jpgd,
			       struct hisi_jpgd_ctx *ctx, u32 status)
{
	enum vb2_buffer_state state = VB2_BUF_STATE_ERROR;
	struct vb2_v4l2_buffer *src, *dst;
	struct hisi_jpgd_layout lay;
	unsigned long payload = 0;

	src = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	dst = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);

	if (status & JPGD_INT_DONE) {
		if (!hisi_jpgd_match(ctx, &vb_to_buf(&src->vb2_buf)->info, &lay)) {
			state = VB2_BUF_STATE_DONE;
			payload = lay.payload;
		} else {
			dev_warn_ratelimited(jpgd->dev,
					     "picture decoded, but the JPEG does not match the CAPTURE format\n");
		}
	} else if (status & JPGD_INT_ERR) {
		dev_err_ratelimited(jpgd->dev, "stream error (INT 0x%x)\n",
				    status);
	} else if (status & JPGD_INT_MORE) {
		dev_err_ratelimited(jpgd->dev,
				    "stream ended before the picture was complete (INT 0x%x)\n",
				    status);
	} else if (status & JPGD_INT_TIMEOUT) {
		dev_err_ratelimited(jpgd->dev, "decoder time-out (INT 0x%x)\n",
				    status);
	} else if (status & JPGD_INT_AUX) {
		dev_err_ratelimited(jpgd->dev,
				    "decoder reported a cause of unknown meaning (INT 0x%x)\n",
				    status);
	}

	/* The block did not finish cleanly: do not trust its state */
	if (state != VB2_BUF_STATE_DONE)
		hisi_jpgd_hw_reset(jpgd);
	vb2_set_plane_payload(&dst->vb2_buf, 0, payload);
	hisi_jpgd_finish_bufs(ctx, src, dst);

	pm_runtime_put_autosuspend(jpgd->dev);
	v4l2_m2m_buf_done_and_job_finish(jpgd->m2m_dev, ctx->fh.m2m_ctx, state);
}

static void hisi_jpgd_device_run(void *priv)
{
	struct hisi_jpgd_ctx *ctx = priv;
	struct hisi_jpgd_dev *jpgd = ctx->dev;
	struct vb2_v4l2_buffer *src, *dst;
	struct hisi_jpgd_layout lay;
	struct hisi_jpgd_buf *sbuf;
	unsigned long flags;
	int ret;

	src = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	dst = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
	sbuf = vb_to_buf(&src->vb2_buf);

	if (!sbuf->valid) {
		dev_err_ratelimited(jpgd->dev,
				    "OUTPUT buffer holds no JPEG the block can decode\n");
		goto err;
	}

	/* The JPEG may have changed since the CAPTURE format was chosen */
	if (hisi_jpgd_match(ctx, &sbuf->info, &lay)) {
		dev_err_ratelimited(jpgd->dev,
				    "JPEG does not match the CAPTURE format\n");
		goto err;
	}

	/* The block writes the whole picture: never into a smaller buffer */
	if (vb2_plane_size(&dst->vb2_buf, 0) < lay.sizeimage) {
		dev_err_ratelimited(jpgd->dev,
				    "CAPTURE buffer too small: %lu < %u bytes\n",
				    vb2_plane_size(&dst->vb2_buf, 0),
				    lay.sizeimage);
		goto err;
	}

	ret = pm_runtime_resume_and_get(jpgd->dev);
	if (ret < 0) {
		dev_err_ratelimited(jpgd->dev, "power up failed: %d\n", ret);
		goto err;
	}

	schedule_delayed_work(&jpgd->watchdog,
			      msecs_to_jiffies(HISI_JPGD_TIMEOUT_MS +
					       (u64)sbuf->info.width * sbuf->info.height /
					       HISI_JPGD_TIMEOUT_PIXELS_PER_MS));

	/*
	 * The lock publishes @curr before START and covers the register
	 * programming, about 270 posted writes (some tens of microseconds once
	 * per picture, which takes milliseconds): not a latency problem.
	 */
	spin_lock_irqsave(&jpgd->hw_lock, flags);
	jpgd->curr = ctx;
	hisi_jpgd_hw_start(jpgd, sbuf, &lay,
			   vb2_dma_contig_plane_dma_addr(&src->vb2_buf, 0),
			   vb2_plane_size(&src->vb2_buf, 0),
			   vb2_get_plane_payload(&src->vb2_buf, 0),
			   vb2_dma_contig_plane_dma_addr(&dst->vb2_buf, 0));
	spin_unlock_irqrestore(&jpgd->hw_lock, flags);

	return;

err:
	vb2_set_plane_payload(&dst->vb2_buf, 0, 0);
	hisi_jpgd_finish_bufs(ctx, src, dst);
	v4l2_m2m_buf_done_and_job_finish(jpgd->m2m_dev, ctx->fh.m2m_ctx,
					 VB2_BUF_STATE_ERROR);
}

static irqreturn_t hisi_jpgd_irq(int irq, void *data)
{
	struct hisi_jpgd_dev *jpgd = data;
	u32 status;

	spin_lock(&jpgd->hw_lock);
	/* INT has unexplained bits above the causes: only touch the cause bits */
	status = jpgd_read(jpgd, JPGD_INT) & JPGD_INT_CAUSES;
	if (!status) {
		spin_unlock(&jpgd->hw_lock);
		return IRQ_NONE;
	}
	jpgd_write(jpgd, JPGD_INT, status);	/* acknowledge (write back) */
	if (!jpgd->curr) {
		/* No running job to give the cause to: the watchdog took it */
		spin_unlock(&jpgd->hw_lock);
		return IRQ_HANDLED;
	}
	jpgd->done = jpgd->curr;
	jpgd->done_status = status;
	jpgd->curr = NULL;
	spin_unlock(&jpgd->hw_lock);

	return IRQ_WAKE_THREAD;
}

static irqreturn_t hisi_jpgd_irq_thread(int irq, void *data)
{
	struct hisi_jpgd_dev *jpgd = data;
	struct hisi_jpgd_ctx *ctx;
	unsigned long flags;
	u32 status;

	spin_lock_irqsave(&jpgd->hw_lock, flags);
	ctx = jpgd->done;
	status = jpgd->done_status;
	jpgd->done = NULL;
	spin_unlock_irqrestore(&jpgd->hw_lock, flags);

	if (!ctx)
		return IRQ_HANDLED;

	/* A watchdog that already started must not run against the next job */
	cancel_delayed_work_sync(&jpgd->watchdog);
	hisi_jpgd_job_done(jpgd, ctx, status);

	return IRQ_HANDLED;
}

static void hisi_jpgd_watchdog(struct work_struct *work)
{
	struct hisi_jpgd_dev *jpgd =
		container_of(work, struct hisi_jpgd_dev, watchdog.work);
	struct hisi_jpgd_ctx *ctx;
	unsigned long flags;

	spin_lock_irqsave(&jpgd->hw_lock, flags);
	/* The interrupt claimed the job (or it is gone): the thread finishes it */
	ctx = jpgd->curr;
	jpgd->curr = NULL;
	spin_unlock_irqrestore(&jpgd->hw_lock, flags);
	if (!ctx)
		return;

	dev_err_ratelimited(jpgd->dev, "decode job timed out, resetting\n");
	hisi_jpgd_job_done(jpgd, ctx, 0);
}

static const struct v4l2_m2m_ops hisi_jpgd_m2m_ops = {
	.device_run = hisi_jpgd_device_run,
};

/* ------------------------------------------------------------ vb2 queues */

static int hisi_jpgd_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
				 unsigned int *nplanes, unsigned int sizes[],
				 struct device *alloc_devs[])
{
	struct hisi_jpgd_ctx *ctx = vb2_get_drv_priv(vq);
	u32 size;

	/* The first buffers are sized for the format the newest JPEG implies */
	if (!V4L2_TYPE_IS_OUTPUT(vq->type) && !vb2_get_num_buffers(vq))
		hisi_jpgd_sync_cap(ctx);

	size = V4L2_TYPE_IS_OUTPUT(vq->type) ?
	       ctx->src_fmt.plane_fmt[0].sizeimage :
	       ctx->dst_fmt.plane_fmt[0].sizeimage;

	if (*nplanes)
		return *nplanes == 1 && sizes[0] >= size ? 0 : -EINVAL;

	*nplanes = 1;
	sizes[0] = size;

	return 0;
}

/*
 * The driver reads the JPEG file through its kernel mapping and writes the
 * zeroed slack behind it. A buffer that was imported from an exporter with a
 * cached mapping (a dma-buf heap) is only coherent with the block and with
 * user space between the CPU access calls of the dma-buf: vb2 does no cache
 * maintenance for imported buffers, the exporter does it in
 * dma_buf_begin_cpu_access() and dma_buf_end_cpu_access(). The direction is
 * DMA_BIDIRECTIONAL because the CPU reads what user space wrote and writes
 * what the block reads. Buffers vb2 allocated itself have no dma-buf here and
 * need nothing: their mapping is the coherent one of the DMA API.
 */
static int hisi_jpgd_cpu_begin(struct vb2_buffer *vb)
{
	if (vb->memory != VB2_MEMORY_DMABUF || !vb->planes[0].dbuf)
		return 0;

	return dma_buf_begin_cpu_access(vb->planes[0].dbuf, DMA_BIDIRECTIONAL);
}

static void hisi_jpgd_cpu_end(struct vb2_buffer *vb)
{
	if (vb->memory != VB2_MEMORY_DMABUF || !vb->planes[0].dbuf)
		return;

	/* Nothing to report to: the data is in place or the exporter is broken */
	dma_buf_end_cpu_access(vb->planes[0].dbuf, DMA_BIDIRECTIONAL);
}

static int hisi_jpgd_buf_prepare(struct vb2_buffer *vb)
{
	struct hisi_jpgd_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct hisi_jpgd_buf *buf = vb_to_buf(vb);
	dma_addr_t addr = vb2_dma_contig_plane_dma_addr(vb, 0);
	unsigned long payload, size = vb2_plane_size(vb, 0);
	struct hisi_jpgd_info info;
	void *vaddr;
	int ret;

	if (vbuf->field == V4L2_FIELD_ANY)
		vbuf->field = V4L2_FIELD_NONE;
	if (vbuf->field != V4L2_FIELD_NONE)
		return -EINVAL;

	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		if (size < ctx->dst_fmt.plane_fmt[0].sizeimage ||
		    (addr & (HISI_JPGD_DST_ALIGN - 1)))
			return -EINVAL;
		return 0;
	}

	/*
	 * QBUF only fails for a buffer the driver cannot use (alignment, no
	 * CPU mapping). A file the block cannot decode (empty, no room for the
	 * zeroed slack, unsupported or corrupt header) is not a QBUF error: the
	 * buffer is marked and its job ends with V4L2_BUF_FLAG_ERROR.
	 */
	buf->valid = false;
	payload = vb2_get_plane_payload(vb, 0);
	if (addr & (HISI_JPGD_SRC_ALIGN - 1))
		return -EINVAL;

	/*
	 * The file is parsed and decoded from the start of the plane. A plane
	 * that starts later (an application sets a data_offset to skip a
	 * header) would be read from the wrong place, so it is refused
	 * instead of being silently shifted.
	 */
	if (vb->planes[0].data_offset)
		return -EINVAL;

	vaddr = vb2_plane_vaddr(vb, 0);
	if (!vaddr)
		return -EINVAL;

	if (!payload || payload + HISI_JPGD_SLACK > size) {
		dev_dbg(ctx->dev->dev, "JPEG buffer: no data or no room for the slack\n");
		return 0;
	}
	ret = hisi_jpgd_cpu_begin(vb);
	if (ret)
		return ret;
	memset(vaddr + payload, 0, HISI_JPGD_SLACK);

	ret = hisi_jpgd_parse(ctx->dev->var, vaddr, payload, &info, &buf->tbl);
	hisi_jpgd_cpu_end(vb);
	if (ret) {
		dev_dbg(ctx->dev->dev, "unsupported JPEG\n");
		return 0;
	}
	buf->info = info;
	buf->valid = true;
	hisi_jpgd_new_info(ctx, &info);

	return 0;
}

static void hisi_jpgd_buf_queue(struct vb2_buffer *vb)
{
	struct hisi_jpgd_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

	/* A STOP with nothing left to decode: this buffer ends the stream */
	if (V4L2_TYPE_IS_CAPTURE(vb->vb2_queue->type) &&
	    vb2_is_streaming(vb->vb2_queue) &&
	    v4l2_m2m_dst_buf_is_last(ctx->fh.m2m_ctx)) {
		vb2_set_plane_payload(vb, 0, 0);
		vbuf->field = V4L2_FIELD_NONE;
		vbuf->sequence = ctx->sequence++;
		v4l2_m2m_last_buffer_done(ctx->fh.m2m_ctx, vbuf);
		v4l2_event_queue_fh(&ctx->fh, &hisi_jpgd_eos_event);
		return;
	}

	/*
	 * A file the block cannot decode never reaches the hardware. With a
	 * CAPTURE buffer ready the job runs and ends both buffers with
	 * V4L2_BUF_FLAG_ERROR (the usual m2m pairing). Without one, no job can
	 * start and a client that waits for the buffer, or for a source change
	 * event that will not come, would wait forever: return the OUTPUT
	 * buffer now with the error flag instead.
	 */
	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type) &&
	    !vb_to_buf(vb)->valid &&
	    !v4l2_m2m_num_dst_bufs_ready(ctx->fh.m2m_ctx)) {
		vbuf->sequence = ctx->sequence_out++;
		v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_ERROR);
		return;
	}

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, vbuf);
}

static int hisi_jpgd_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct hisi_jpgd_ctx *ctx = vb2_get_drv_priv(vq);
	struct v4l2_m2m_ctx *m2m_ctx = ctx->fh.m2m_ctx;

	v4l2_m2m_update_start_streaming_state(m2m_ctx, vq);
	if (V4L2_TYPE_IS_CAPTURE(vq->type))
		return 0;

	/*
	 * A STREAMOFF and STREAMON pair on the OUTPUT queue resumes the decoder
	 * after a drain (dev-decoder.rst, "Drain"): the stopped state ends and
	 * the CAPTURE queue, which delivered the LAST buffer, delivers again.
	 * A drain that is still pending (no CAPTURE buffer to carry the flag
	 * yet) stays: the next CAPTURE buffer ends it.
	 */
	m2m_ctx->has_stopped = false;
	vb2_clear_last_buffer_dequeued(v4l2_m2m_get_dst_vq(m2m_ctx));

	return 0;
}

static void hisi_jpgd_stop_streaming(struct vb2_queue *vq)
{
	struct hisi_jpgd_ctx *ctx = vb2_get_drv_priv(vq);
	struct vb2_v4l2_buffer *vbuf;
	bool stopped;

	for (;;) {
		if (V4L2_TYPE_IS_OUTPUT(vq->type))
			vbuf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
		else
			vbuf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
		if (!vbuf)
			break;
		v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_ERROR);
	}

	stopped = v4l2_m2m_has_stopped(ctx->fh.m2m_ctx);
	v4l2_m2m_update_stop_streaming_state(ctx->fh.m2m_ctx, vq);

	/*
	 * The numbers of a queue restart with its next stream. Not in
	 * start_streaming(): buffers queued before STREAMON reach buf_queue()
	 * before it and may already have been numbered.
	 */
	if (V4L2_TYPE_IS_CAPTURE(vq->type)) {
		ctx->sequence = 0;
		return;
	}
	ctx->sequence_out = 0;

	/*
	 * STREAMOFF of the OUTPUT queue in the middle of a drain ends it at
	 * once (the core flags the next CAPTURE buffer as LAST): that is the
	 * end of the stream, announce it once. A context that had stopped
	 * before has announced it already.
	 */
	if (!stopped && v4l2_m2m_has_stopped(ctx->fh.m2m_ctx))
		v4l2_event_queue_fh(&ctx->fh, &hisi_jpgd_eos_event);
}

static const struct vb2_ops hisi_jpgd_qops = {
	.queue_setup = hisi_jpgd_queue_setup,
	.buf_prepare = hisi_jpgd_buf_prepare,
	.buf_queue = hisi_jpgd_buf_queue,
	.start_streaming = hisi_jpgd_start_streaming,
	.stop_streaming = hisi_jpgd_stop_streaming,
};

static int hisi_jpgd_queue_init(void *priv, struct vb2_queue *src_vq,
				struct vb2_queue *dst_vq)
{
	struct hisi_jpgd_ctx *ctx = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->ops = &hisi_jpgd_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->buf_struct_size = sizeof(struct hisi_jpgd_buf);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->dev->lock;
	src_vq->dev = ctx->dev->dev;
	/*
	 * The CPU reads and writes the JPEG buffer (hisi_jpgd_cpu_begin()):
	 * mapped in both directions, an imported buffer is synced by the
	 * exporter in the direction that the dma-buf CPU access calls use (DMA
	 * API debugging checks that the two agree).
	 */
	src_vq->bidirectional = true;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->ops = &hisi_jpgd_qops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->buf_struct_size = sizeof(struct hisi_jpgd_buf);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->dev->lock;
	dst_vq->dev = ctx->dev->dev;

	return vb2_queue_init(dst_vq);
}

/* ----------------------------------------------------------------- ioctls */

static int hisi_jpgd_querycap(struct file *file, void *priv,
			      struct v4l2_capability *cap)
{
	struct hisi_jpgd_ctx *ctx = file_to_ctx(file);

	strscpy(cap->driver, HISI_JPGD_NAME, sizeof(cap->driver));
	strscpy(cap->card, ctx->dev->var->name, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev_name(ctx->dev->dev));

	return 0;
}

static int hisi_jpgd_enum_fmt_cap(struct file *file, void *priv,
				  struct v4l2_fmtdesc *f)
{
	if (f->index >= ARRAY_SIZE(hisi_jpgd_fourccs))
		return -EINVAL;

	f->pixelformat = hisi_jpgd_fourccs[f->index];

	return 0;
}

static int hisi_jpgd_enum_fmt_out(struct file *file, void *priv,
				  struct v4l2_fmtdesc *f)
{
	if (f->index)
		return -EINVAL;

	f->pixelformat = V4L2_PIX_FMT_JPEG;
	f->flags = V4L2_FMT_FLAG_COMPRESSED;

	return 0;
}

static int hisi_jpgd_enum_framesizes(struct file *file, void *priv,
				     struct v4l2_frmsizeenum *fsize)
{
	struct hisi_jpgd_ctx *ctx = file_to_ctx(file);
	unsigned int i;

	if (fsize->index)
		return -EINVAL;

	if (fsize->pixel_format != V4L2_PIX_FMT_JPEG) {
		for (i = 0; i < ARRAY_SIZE(hisi_jpgd_fourccs); i++)
			if (fsize->pixel_format == hisi_jpgd_fourccs[i])
				break;
		if (i == ARRAY_SIZE(hisi_jpgd_fourccs))
			return -EINVAL;
	}

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	fsize->stepwise.min_width = ctx->dev->var->min_dim;
	fsize->stepwise.max_width = ctx->dev->var->max_dim;
	fsize->stepwise.step_width = 1;
	fsize->stepwise.min_height = ctx->dev->var->min_dim;
	fsize->stepwise.max_height = ctx->dev->var->max_dim;
	fsize->stepwise.step_height = 1;

	return 0;
}

static int hisi_jpgd_g_fmt_out(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	f->fmt.pix_mp = file_to_ctx(file)->src_fmt;

	return 0;
}

static int hisi_jpgd_g_fmt_cap(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	struct hisi_jpgd_ctx *ctx = file_to_ctx(file);

	/* A file queued since the last call may have changed the format */
	hisi_jpgd_sync_cap(ctx);
	f->fmt.pix_mp = ctx->dst_fmt;

	return 0;
}

static int hisi_jpgd_try_fmt_out(struct file *file, void *priv,
				 struct v4l2_format *f)
{
	hisi_jpgd_fill_out(file_to_ctx(file), &f->fmt.pix_mp);

	return 0;
}

static int hisi_jpgd_try_fmt_cap_ioctl(struct file *file, void *priv,
				       struct v4l2_format *f)
{
	hisi_jpgd_try_fmt_cap(file_to_ctx(file), &f->fmt.pix_mp, NULL);

	return 0;
}

static int hisi_jpgd_s_fmt_out(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	struct hisi_jpgd_ctx *ctx = file_to_ctx(file);

	if (vb2_is_busy(v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type)))
		return -EBUSY;

	hisi_jpgd_fill_out(ctx, &f->fmt.pix_mp);
	ctx->src_fmt = f->fmt.pix_mp;

	return 0;
}

static int hisi_jpgd_s_fmt_cap(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	struct hisi_jpgd_ctx *ctx = file_to_ctx(file);

	if (vb2_is_busy(v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type)))
		return -EBUSY;

	hisi_jpgd_try_fmt_cap(ctx, &f->fmt.pix_mp, &ctx->lay);
	ctx->dst_fmt = f->fmt.pix_mp;
	ctx->fmt_set = true;

	return 0;
}

static int hisi_jpgd_g_selection(struct file *file, void *priv,
				 struct v4l2_selection *s)
{
	struct hisi_jpgd_ctx *ctx = file_to_ctx(file);
	const struct hisi_jpgd_layout *lay = &ctx->lay;

	if (s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE &&
	    s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	switch (s->target) {
	case V4L2_SEL_TGT_COMPOSE:
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		/* the picture inside the row-aligned buffer */
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = lay->width;
		s->r.height = lay->height;
		return 0;
	case V4L2_SEL_TGT_COMPOSE_PADDED:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = lay->ystride;
		s->r.height = lay->rows;
		return 0;
	}

	return -EINVAL;
}

static int hisi_jpgd_subscribe_event(struct v4l2_fh *fh,
				     const struct v4l2_event_subscription *sub)
{
	if (sub->type == V4L2_EVENT_EOS)
		return v4l2_event_subscribe(fh, sub, 2, NULL);
	if (sub->type == V4L2_EVENT_SOURCE_CHANGE)
		return v4l2_src_change_event_subscribe(fh, sub);

	return -EINVAL;
}

/*
 * DECODER_CMD as dev-decoder.rst has it. The commands only act while both
 * queues stream, otherwise they succeed and do nothing. A STOP that finds
 * nothing to decode ends the drain inside the core helper, so the stream end
 * is announced here; one that finds a job in the queue is ended by the
 * completion of the job that holds the last OUTPUT buffer. A new drain and the
 * resume by START both end the "last buffer dequeued" state of the CAPTURE
 * queue, otherwise a CAPTURE DQBUF would fail with -EPIPE instead of waiting
 * for the next buffer, and START has to look for work that was queued while the
 * decoder was stopped.
 */
static int hisi_jpgd_decoder_cmd(struct file *file, void *priv,
				 struct v4l2_decoder_cmd *dc)
{
	struct v4l2_fh *fh = file_to_v4l2_fh(file);
	struct v4l2_m2m_ctx *m2m_ctx = fh->m2m_ctx;
	bool stopped;
	int ret;

	ret = v4l2_m2m_ioctl_try_decoder_cmd(file, priv, dc);
	if (ret)
		return ret;

	if (!vb2_is_streaming(v4l2_m2m_get_src_vq(m2m_ctx)) ||
	    !vb2_is_streaming(v4l2_m2m_get_dst_vq(m2m_ctx)))
		return 0;

	stopped = v4l2_m2m_has_stopped(m2m_ctx);
	ret = v4l2_m2m_ioctl_decoder_cmd(file, priv, dc);
	if (ret)
		return ret;

	if (dc->cmd == V4L2_DEC_CMD_START) {
		vb2_clear_last_buffer_dequeued(v4l2_m2m_get_dst_vq(m2m_ctx));
		v4l2_m2m_try_schedule(m2m_ctx);
	} else if (!stopped) {
		/* A drain began (a STOP of a stopped decoder changes nothing) */
		vb2_clear_last_buffer_dequeued(v4l2_m2m_get_dst_vq(m2m_ctx));
		if (v4l2_m2m_has_stopped(m2m_ctx))
			v4l2_event_queue_fh(fh, &hisi_jpgd_eos_event);
	}

	return 0;
}

static const struct v4l2_ioctl_ops hisi_jpgd_ioctl_ops = {
	.vidioc_querycap = hisi_jpgd_querycap,

	.vidioc_enum_fmt_vid_cap = hisi_jpgd_enum_fmt_cap,
	.vidioc_enum_fmt_vid_out = hisi_jpgd_enum_fmt_out,
	.vidioc_enum_framesizes = hisi_jpgd_enum_framesizes,
	.vidioc_g_fmt_vid_cap_mplane = hisi_jpgd_g_fmt_cap,
	.vidioc_g_fmt_vid_out_mplane = hisi_jpgd_g_fmt_out,
	.vidioc_try_fmt_vid_cap_mplane = hisi_jpgd_try_fmt_cap_ioctl,
	.vidioc_try_fmt_vid_out_mplane = hisi_jpgd_try_fmt_out,
	.vidioc_s_fmt_vid_cap_mplane = hisi_jpgd_s_fmt_cap,
	.vidioc_s_fmt_vid_out_mplane = hisi_jpgd_s_fmt_out,
	.vidioc_g_selection = hisi_jpgd_g_selection,

	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf = v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf = v4l2_m2m_ioctl_dqbuf,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,
	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,

	.vidioc_try_decoder_cmd = v4l2_m2m_ioctl_try_decoder_cmd,
	.vidioc_decoder_cmd = hisi_jpgd_decoder_cmd,

	.vidioc_subscribe_event = hisi_jpgd_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

/* ----------------------------------------------------------- file ops */

static int hisi_jpgd_open(struct file *file)
{
	struct hisi_jpgd_dev *jpgd = video_drvdata(file);
	struct hisi_jpgd_ctx *ctx;
	int ret;

	ctx = kzalloc_obj(*ctx);
	if (!ctx)
		return -ENOMEM;

	if (mutex_lock_interruptible(&jpgd->lock)) {
		ret = -ERESTARTSYS;
		goto err_free;
	}

	ctx->dev = jpgd;
	v4l2_fh_init(&ctx->fh, video_devdata(file));

	ctx->src_fmt.width = JPGD_DEF_WIDTH;
	ctx->src_fmt.height = JPGD_DEF_HEIGHT;
	hisi_jpgd_fill_out(ctx, &ctx->src_fmt);
	ctx->dst_fmt.pixelformat = V4L2_PIX_FMT_NV21;
	ctx->dst_fmt.width = JPGD_DEF_WIDTH;
	ctx->dst_fmt.height = JPGD_DEF_HEIGHT;
	hisi_jpgd_try_fmt_cap(ctx, &ctx->dst_fmt, &ctx->lay);

	ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(jpgd->m2m_dev, ctx,
					    hisi_jpgd_queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		goto err_exit;
	}

	v4l2_fh_add(&ctx->fh, file);
	mutex_unlock(&jpgd->lock);

	return 0;

err_exit:
	v4l2_fh_exit(&ctx->fh);
	mutex_unlock(&jpgd->lock);
err_free:
	kfree(ctx);

	return ret;
}

static int hisi_jpgd_release(struct file *file)
{
	struct hisi_jpgd_dev *jpgd = video_drvdata(file);
	struct hisi_jpgd_ctx *ctx = file_to_ctx(file);

	mutex_lock(&jpgd->lock);
	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
	v4l2_fh_del(&ctx->fh, file);
	v4l2_fh_exit(&ctx->fh);
	mutex_unlock(&jpgd->lock);
	kfree(ctx);

	return 0;
}

static const struct v4l2_file_operations hisi_jpgd_fops = {
	.owner = THIS_MODULE,
	.open = hisi_jpgd_open,
	.release = hisi_jpgd_release,
	.poll = v4l2_m2m_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = v4l2_m2m_fop_mmap,
};

/*
 * Called when the last user of the video device is gone: with an open file
 * the device can outlive the unbind of the driver, and the file release still
 * needs the mutex, the mem2mem device and the V4L2 device.
 */
static void hisi_jpgd_vfd_release(struct video_device *vfd)
{
	struct hisi_jpgd_dev *jpgd = container_of(vfd, struct hisi_jpgd_dev, vfd);

	v4l2_m2m_release(jpgd->m2m_dev);
	v4l2_device_unregister(&jpgd->v4l2_dev);
	kfree(jpgd);
}

static const struct video_device hisi_jpgd_videodev = {
	.name = HISI_JPGD_NAME,
	.fops = &hisi_jpgd_fops,
	.ioctl_ops = &hisi_jpgd_ioctl_ops,
	.minor = -1,
	/* hisi_jpgd_vfd_release() is set once the device is registered, see probe */
	.release = video_device_release_empty,
	.vfl_dir = VFL_DIR_M2M,
	.device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING,
};

/* --------------------------------------------------------- power management */

/*
 * The block must have its clock running and its reset released before any
 * register access: the Hi3520D CRG reset value holds it in reset (the window
 * reads zero), and reading a clock-gated block can stall the bus.
 */
static int hisi_jpgd_runtime_resume(struct device *dev)
{
	struct hisi_jpgd_dev *jpgd = dev_get_drvdata(dev);
	int ret;

	ret = clk_prepare_enable(jpgd->clk);
	if (ret)
		return ret;

	ret = reset_control_deassert(jpgd->rst);
	if (ret) {
		clk_disable_unprepare(jpgd->clk);
		return ret;
	}
	usleep_range(10, 20);

	enable_irq(jpgd->irq);

	return 0;
}

static int hisi_jpgd_runtime_suspend(struct device *dev)
{
	struct hisi_jpgd_dev *jpgd = dev_get_drvdata(dev);

	disable_irq(jpgd->irq);
	reset_control_assert(jpgd->rst);
	clk_disable_unprepare(jpgd->clk);

	return 0;
}

/* System sleep: let a running job finish, then power the block down */
static int hisi_jpgd_suspend(struct device *dev)
{
	struct hisi_jpgd_dev *jpgd = dev_get_drvdata(dev);
	int ret;

	v4l2_m2m_suspend(jpgd->m2m_dev);

	ret = pm_runtime_force_suspend(dev);
	/* A failed suspend gets no resume call: do not leave the queue stopped */
	if (ret)
		v4l2_m2m_resume(jpgd->m2m_dev);

	return ret;
}

static int hisi_jpgd_resume(struct device *dev)
{
	struct hisi_jpgd_dev *jpgd = dev_get_drvdata(dev);
	int ret;

	ret = pm_runtime_force_resume(dev);
	/*
	 * Resume the queue even when the block did not come back: a job then
	 * ends with an error (its runtime resume fails) instead of waiting
	 * for a block that never answers.
	 */
	v4l2_m2m_resume(jpgd->m2m_dev);

	return ret;
}

static const struct dev_pm_ops hisi_jpgd_pm_ops = {
	SYSTEM_SLEEP_PM_OPS(hisi_jpgd_suspend, hisi_jpgd_resume)
	RUNTIME_PM_OPS(hisi_jpgd_runtime_suspend, hisi_jpgd_runtime_resume,
		       NULL)
};

/* ------------------------------------------------------------------ probe */

static int hisi_jpgd_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hisi_jpgd_dev *jpgd;
	int ret;

	/* Not devm: freed by the video device release, see hisi_jpgd_vfd_release() */
	jpgd = kzalloc_obj(*jpgd);
	if (!jpgd)
		return -ENOMEM;

	jpgd->dev = dev;
	jpgd->var = of_device_get_match_data(dev);
	if (!jpgd->var) {
		ret = -ENODEV;
		goto err_free;
	}

	mutex_init(&jpgd->lock);
	spin_lock_init(&jpgd->hw_lock);
	INIT_DELAYED_WORK(&jpgd->watchdog, hisi_jpgd_watchdog);
	platform_set_drvdata(pdev, jpgd);

	jpgd->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(jpgd->base)) {
		ret = PTR_ERR(jpgd->base);
		goto err_free;
	}

	jpgd->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(jpgd->clk)) {
		ret = dev_err_probe(dev, PTR_ERR(jpgd->clk), "no clock\n");
		goto err_free;
	}

	jpgd->rst = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR(jpgd->rst)) {
		ret = dev_err_probe(dev, PTR_ERR(jpgd->rst), "no reset\n");
		goto err_free;
	}

	jpgd->irq = platform_get_irq(pdev, 0);
	if (jpgd->irq < 0) {
		ret = jpgd->irq;
		goto err_free;
	}

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret) {
		ret = dev_err_probe(dev, ret, "no 32-bit DMA\n");
		goto err_free;
	}
	dma_set_max_seg_size(dev, DMA_BIT_MASK(32));

	/* Stays disabled until the clock runs and the reset is released */
	ret = devm_request_threaded_irq(dev, jpgd->irq, hisi_jpgd_irq,
					hisi_jpgd_irq_thread, IRQF_NO_AUTOEN,
					dev_name(dev), jpgd);
	if (ret) {
		ret = dev_err_probe(dev, ret, "cannot request irq\n");
		goto err_free;
	}

	pm_runtime_set_autosuspend_delay(dev, HISI_JPGD_AUTOSUSPEND_MS);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_enable(dev);

	/*
	 * Clock on and reset released before the first read (the window reads
	 * as zero while the reset is held, and a gated block can stall the
	 * bus). The block has no ID register: compare the idle registers with
	 * the values the layout was written for and say so loudly if not.
	 */
	ret = pm_runtime_resume_and_get(dev);
	if (ret) {
		pm_runtime_disable(dev);
		ret = dev_err_probe(dev, ret, "cannot power up\n");
		goto err_free;
	}
	hisi_jpgd_check_idle(jpgd);
	pm_runtime_put_autosuspend(dev);

	ret = v4l2_device_register(dev, &jpgd->v4l2_dev);
	if (ret)
		goto err_pm;

	jpgd->m2m_dev = v4l2_m2m_init(&hisi_jpgd_m2m_ops);
	if (IS_ERR(jpgd->m2m_dev)) {
		ret = PTR_ERR(jpgd->m2m_dev);
		dev_err(dev, "cannot init mem2mem device: %d\n", ret);
		goto err_v4l2;
	}

	jpgd->vfd = hisi_jpgd_videodev;
	jpgd->vfd.lock = &jpgd->lock;
	jpgd->vfd.v4l2_dev = &jpgd->v4l2_dev;
	video_set_drvdata(&jpgd->vfd, jpgd);

	ret = video_register_device(&jpgd->vfd, VFL_TYPE_VIDEO, -1);
	if (ret) {
		dev_err(dev, "cannot register video device: %d\n", ret);
		goto err_m2m;
	}
	/*
	 * A failed registration puts the device, which calls the release
	 * callback: the empty one of the template, so the error path above
	 * does the freeing. From here on the last user of the device frees it.
	 */
	jpgd->vfd.release = hisi_jpgd_vfd_release;

	v4l2_info(&jpgd->v4l2_dev, "registered as /dev/video%d\n",
		  jpgd->vfd.num);

	return 0;

err_m2m:
	v4l2_m2m_release(jpgd->m2m_dev);
err_v4l2:
	v4l2_device_unregister(&jpgd->v4l2_dev);
err_pm:
	pm_runtime_dont_use_autosuspend(dev);
	pm_runtime_disable(dev);
	if (!pm_runtime_status_suspended(dev))
		hisi_jpgd_runtime_suspend(dev);
	pm_runtime_set_suspended(dev);
err_free:
	kfree(jpgd);

	return ret;
}

static void hisi_jpgd_remove(struct platform_device *pdev)
{
	struct hisi_jpgd_dev *jpgd = platform_get_drvdata(pdev);

	/*
	 * No new jobs; a running one ends by its interrupt or, at the latest,
	 * by the watchdog. Then the block is powered down. The video device
	 * goes last: files that are still open keep @jpgd, the mem2mem device
	 * and the V4L2 device alive until they are closed (the release
	 * callback of the video device frees them), and when none is open
	 * that happens inside video_unregister_device().
	 */
	v4l2_m2m_suspend(jpgd->m2m_dev);
	cancel_delayed_work_sync(&jpgd->watchdog);

	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
	if (!pm_runtime_status_suspended(&pdev->dev))
		hisi_jpgd_runtime_suspend(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);

	devm_free_irq(&pdev->dev, jpgd->irq, jpgd);
	video_unregister_device(&jpgd->vfd);
}

static const struct of_device_id hisi_jpgd_of_match[] = {
	{ .compatible = "hisilicon,hi3520dv100-jpgd",
	  .data = &hisi_jpgd_hi3520dv100 },
	{ .compatible = "hisilicon,hi3536dv100-jpgd",
	  .data = &hisi_jpgd_hi3536dv100 },
	{ }
};
MODULE_DEVICE_TABLE(of, hisi_jpgd_of_match);

static struct platform_driver hisi_jpgd_driver = {
	.probe = hisi_jpgd_probe,
	.remove = hisi_jpgd_remove,
	.driver = {
		.name = HISI_JPGD_NAME,
		.pm = pm_ptr(&hisi_jpgd_pm_ops),
		.of_match_table = hisi_jpgd_of_match,
	},
};
module_platform_driver(hisi_jpgd_driver);

MODULE_DESCRIPTION("HiSilicon JPEG decoder (JPGD) V4L2 mem2mem driver");
MODULE_LICENSE("GPL");
