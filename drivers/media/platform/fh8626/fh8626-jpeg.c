// SPDX-License-Identifier: GPL-2.0
/*
 * FH8626 hardware JPEG encoder, V4L2 mem2mem driver.
 *
 * NV12 in, JPEG or MJPEG out. The 128-byte quantization table is scaled
 * from the T.81 Annex K tables and reloaded per frame; the Huffman and DHT
 * tables are loaded at probe and after recoverable completion failures.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/soc/fullhan/fh8626-pmu.h>
#include <linux/types.h>

#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-vmalloc.h>
#include <media/videobuf2-v4l2.h>

#include "fh8626-jpeg.h"
#include "fh8626-event.h"

#define DRIVER_NAME "fh8626-jpeg"

/* A failed stop must survive close/remove; never re-probe that engine. */
static LIST_HEAD(fh8626_jpeg_quarantine);
static DEFINE_MUTEX(fh8626_jpeg_quarantine_lock);

static int fh8626_jpeg_wait_idle(struct fh8626_jpeg_dev *dev)
{
	unsigned long deadline = jiffies + msecs_to_jiffies(100);
	u32 val;
	int ret;

	/* Wait for the PMU to report the core idle. */
	do {
		ret = fh8626_pmu_read(0x2038, &val);
		if (ret)
			return ret;
		if (!(val & BIT(14)))
			return 0;
		usleep_range(50, 100);
	} while (time_before(jiffies, deadline));

	return -ETIMEDOUT;
}

static void fh8626_jpeg_free_dma(struct fh8626_jpeg_dev *dev)
{
	if (dev->src_cpu)
		dma_free_coherent(dev->dev, dev->src_size, dev->src_cpu,
				  dev->src_dma);
	if (dev->dst_cpu)
		dma_free_coherent(dev->dev, dev->dst_size, dev->dst_cpu,
				  dev->dst_dma);
	dev->src_cpu = NULL;
	dev->dst_cpu = NULL;
}

static void *fh8626_jpeg_get_dma(struct fh8626_jpeg_dev *dev, void **cpu,
				 dma_addr_t *dma, u32 *size, u32 want)
{
	if (*cpu && *size >= want)
		return *cpu;
	if (*cpu)
		dma_free_coherent(dev->dev, *size, *cpu, *dma);
	*cpu = dma_alloc_coherent(dev->dev, want, dma, GFP_KERNEL);
	*size = *cpu ? want : 0;
	return *cpu;
}

static inline struct fh8626_jpeg_ctx *fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct fh8626_jpeg_ctx, fh);
}

static inline struct fh8626_jpeg_ctx *ctrl_to_ctx(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct fh8626_jpeg_ctx,
			    ctrl_handler);
}

static struct fh8626_jpeg_q_data *get_q_data(struct fh8626_jpeg_ctx *ctx,
					      enum v4l2_buf_type type)
{
	if (V4L2_TYPE_IS_OUTPUT(type))
		return &ctx->out_q;
	return &ctx->cap_q;
}

static void fh8626_jpeg_init_tables(struct fh8626_jpeg_dev *dev)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(fh8626_jpeg_huff_table); i++)
		writel(fh8626_jpeg_huff_table[i],
		       dev->regs + FH8626_JPEG_HUFF_TABLE + i * 4);

	for (i = 0; i < sizeof(fh8626_jpeg_dht_table); i += 4) {
		u32 val = fh8626_jpeg_dht_table[i] |
			  (fh8626_jpeg_dht_table[i + 1] << 8) |
			  (fh8626_jpeg_dht_table[i + 2] << 16) |
			  (fh8626_jpeg_dht_table[i + 3] << 24);
		writel(val, dev->regs + FH8626_JPEG_DHT_TABLE + i);
	}
}

/* Quantizer scale for a quality, in percent of the Annex K tables. */
static u32 fh8626_jpeg_scale(u32 quality)
{
	quality = clamp_val(quality, 1U, 100U);
	return quality < 50 ? 5000 / quality : 200 - quality * 2;
}

static void fh8626_jpeg_set_qtable(struct fh8626_jpeg_dev *dev,
				   unsigned int quality)
{
	unsigned int scale = fh8626_jpeg_scale(quality), i;
	u8 q[128];

	for (i = 0; i < 64; i++) {
		unsigned int zz = fh8626_jpeg_zigzag[i];
		unsigned int val;

		val = (fh8626_jpeg_luma_q50[zz] * scale + 50) / 100;
		q[i] = clamp_val(val, 1U, 255U);

		val = (fh8626_jpeg_chroma_q50[zz] * scale + 50) / 100;
		q[64 + i] = clamp_val(val, 1U, 255U);
	}

	for (i = 0; i < 128; i += 4) {
		u32 val = q[i] | (q[i + 1] << 8) |
			  (q[i + 2] << 16) | (q[i + 3] << 24);

		writel(val, dev->regs + FH8626_JPEG_QTABLE + i);
	}
}

/* Approximate log2(x), in Q8. */
static int fh8626_jpeg_log2(u64 x)
{
	unsigned int n;
	u64 t;

	if (!x)
		return 0;
	n = fls64(x) - 1;
	t = n >= 16 ? x >> (n - 16) : x << (16 - n);
	t -= 1 << 16;
	/* log2(1 + t) ~= t * (1.3466 - 0.3466 * t) for t in [0, 1) */
	t = (t * (88252 - ((22715 * t) >> 16))) >> 16;
	return n * 256 + (int)(t >> 8);
}

/* Approximate 2^(x / 256), clamped to [1, 2^31]. */
static u32 fh8626_jpeg_exp2(int x)
{
	u32 f;
	u64 m;

	if (x < 0)
		return 1;
	x = min(x, 31 * 256);
	f = x & 255;
	/* 2^t ~= 1 + t * (0.6565 + 0.3435 * t) for t in [0, 1), Q16 */
	m = 65536 + ((f * (43024 + ((22512 * f) >> 8))) >> 8);
	return max_t(u64, (m << (x >> 8)) >> 16, 1);
}

static u32 fh8626_jpeg_rc_rate(const struct fh8626_jpeg_ctx *ctx)
{
	u32 num = ctx->target_fps.numerator ? ctx->target_fps.numerator : 25;
	u32 den = ctx->target_fps.denominator ?
		  ctx->target_fps.denominator : 1;

	return clamp_t(u32, DIV_ROUND_CLOSEST(num, den), 1, 120);
}

#define FH8626_JPEG_RC_MAX_QUALITY	98

/*
 * Predict quality from the last frame's log2(bits) + log2(scale), in Q8.
 * Correct accumulated bitrate error over one second (CBR) or two (VBR),
 * limiting scale changes to a factor of four per frame. The quality control
 * sets the initial quality and caps VBR quality.
 */
static u32 fh8626_jpeg_rc_plan(struct fh8626_jpeg_ctx *ctx)
{
	u32 num = ctx->target_fps.numerator ? ctx->target_fps.numerator : 25;
	u32 den = ctx->target_fps.denominator ?
		  ctx->target_fps.denominator : 1;
	bool cbr = READ_ONCE(ctx->bitrate_mode) ==
		   V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;
	u32 max_q = cbr ? FH8626_JPEG_RC_MAX_QUALITY :
		    clamp_t(u32, READ_ONCE(ctx->quality), 1,
			    FH8626_JPEG_RC_MAX_QUALITY);
	int prev = fh8626_jpeg_log2(fh8626_jpeg_scale(ctx->current_qp));
	s64 budget;
	u32 scale, q;
	int s;

	ctx->frame_bits = max_t(u64, div_u64((u64)READ_ONCE(ctx->bitrate) *
					      den, num), 1);
	if (!ctx->rc_valid)
		return min(ctx->current_qp, max_q);

	budget = (s64)ctx->frame_bits -
		 div64_s64(ctx->rc_error,
			   (s64)fh8626_jpeg_rc_rate(ctx) * (cbr ? 1 : 2));
	budget = clamp_t(s64, budget, ctx->frame_bits / 4,
			 (s64)ctx->frame_bits * 4);
	s = ctx->complexity - fh8626_jpeg_log2(budget);
	/*
	 * A Q8 deadband avoids flicker from uneven integer-quality steps;
	 * accumulated rate error eventually moves the budget across it.
	 */
	if (abs(s - prev) < 44)
		return min(ctx->current_qp, max_q);
	s = clamp(s, prev - 2 * 256, prev + 2 * 256);
	scale = clamp_t(u32, fh8626_jpeg_exp2(s), 1, 5000);
	/* Invert fh8626_jpeg_scale(). */
	q = scale >= 100 ? DIV_ROUND_CLOSEST(5000, scale) :
			   DIV_ROUND_CLOSEST(200 - scale, 2);
	return clamp_t(u32, q, 1, max_q);
}

static void fh8626_jpeg_rc_update(struct fh8626_jpeg_ctx *ctx,
				  u32 stream_len, u32 quality)
{
	bool cbr = READ_ONCE(ctx->bitrate_mode) ==
		   V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;
	s64 second = (s64)ctx->frame_bits * fh8626_jpeg_rc_rate(ctx);
	u64 bits = (u64)stream_len * 8;

	ctx->complexity = fh8626_jpeg_log2(bits) +
			  fh8626_jpeg_log2(fh8626_jpeg_scale(quality));
	ctx->rc_valid = true;
	/* Saved credit is bounded; CBR banks at most half a second. */
	ctx->rc_error = clamp(ctx->rc_error + (s64)bits - ctx->frame_bits,
			      cbr ? -second / 2 : -second, second);
}

static u32 fh8626_jpeg_patch_length(void *vaddr, u32 hw_len, u32 capacity)
{
	const u8 *p;
	u32 check_len;
	int i;

	if (!vaddr || hw_len < 2 || hw_len > capacity)
		return 0;

	p = vaddr;

	/* The EOI marker (ff d9) normally ends at hw_len. */
	if (p[hw_len - 2] == 0xff && p[hw_len - 1] == 0xd9)
		return hw_len;

	/* The reported length can be one byte short of the marker's end. */
	if (hw_len < capacity && p[hw_len - 1] == 0xff && p[hw_len] == 0xd9)
		return hw_len + 1;

	check_len = min_t(u32, 16, capacity > hw_len ? capacity - hw_len : 0);
	for (i = 0; i < (int)check_len; i++) {
		if (hw_len + i + 1 < capacity &&
		    p[hw_len + i] == 0xff && p[hw_len + i + 1] == 0xd9)
			return hw_len + i + 2;
	}

	return 0;
}

/*
 * Copy an NV12 picture into the coded layout of whole MCU rows. The missing
 * rows repeat the nearest picture row, either below the picture or above it.
 */
static void fh8626_jpeg_copy_input(struct fh8626_jpeg_dev *dev, const u8 *src,
				   u32 stride, u32 height, u32 aligned_w,
				   u32 aligned_h, bool pad_top)
{
	u8 *y = dev->src_cpu, *uv = y + aligned_w * aligned_h;
	const u8 *src_uv = src + stride * height;
	u32 pad = aligned_h - height, row, from;

	for (row = 0; row < aligned_h; row++) {
		from = pad_top ? (row < pad ? 0 : row - pad) :
				 min(row, height - 1);
		memcpy(y + row * aligned_w, src + from * stride, aligned_w);
	}
	for (row = 0; row < aligned_h / 2; row++) {
		from = pad_top ? (row < pad / 2 ? 0 : row - pad / 2) :
				 min(row, height / 2 - 1);
		memcpy(uv + row * aligned_w, src_uv + from * stride, aligned_w);
	}
}

/*
 * The engine reads whole MCU rows: ALIGN(height, 16) luma rows and half as
 * many chroma rows, each ALIGN(width, 16) bytes long and that far apart.
 * Rows past the picture are read from whatever follows it.
 */
static size_t fh8626_jpeg_direct_size(const struct fh8626_jpeg_q_data *q)
{
	return (size_t)q->bytesperline *
	       (q->height + ALIGN(q->height, FH8626_JPEG_ALIGN_HEIGHT) / 2);
}

/*
 * Direct DMA requires 64-byte-aligned planes with chroma at stride * height.
 * Keep the buffer queued and mapped until completion or idle.
 */
static bool fh8626_jpeg_direct(struct fh8626_jpeg_ctx *ctx,
			       struct vb2_buffer *vb, u32 rotate)
{
	const struct fh8626_jpeg_q_data *q = &ctx->out_q;
	dma_addr_t y = vb2_dma_contig_plane_dma_addr(vb, 0);
	dma_addr_t uv = y + (dma_addr_t)q->bytesperline * q->height;

	/* These rotations need the padding rows above the picture. */
	if ((rotate == 1 || rotate == 2) &&
	    q->height != ALIGN(q->height, FH8626_JPEG_ALIGN_HEIGHT))
		return false;
	return q->bytesperline == ALIGN(q->width, FH8626_JPEG_ALIGN_WIDTH) &&
	       !(y & 63) && !(uv & 63) &&
	       vb2_plane_size(vb, 0) >= fh8626_jpeg_direct_size(q);
}

/* Write the displayed size into the SOF0 frame header the engine emits. */
static void fh8626_jpeg_patch_sof(u8 *p, u32 len, u32 width, u32 height)
{
	u32 pos = 2;

	while (pos + 9 <= len && p[pos] == 0xff) {
		u8 marker = p[pos + 1];
		u32 seg = (p[pos + 2] << 8) | p[pos + 3];

		if (marker == 0xc0) {
			p[pos + 5] = height >> 8;
			p[pos + 6] = height & 0xff;
			p[pos + 7] = width >> 8;
			p[pos + 8] = width & 0xff;
			return;
		}
		if (marker == 0xda || seg < 2)
			return;
		pos += 2 + seg;
	}
}

static irqreturn_t fh8626_jpeg_irq(int irq, void *priv)
{
	struct fh8626_jpeg_dev *dev = priv;
	u32 status;

	status = readl(dev->regs + FH8626_JPEG_INT_STATUS);
	if (!status)
		return IRQ_NONE;

	writel(status, dev->regs + FH8626_JPEG_INT_CLEAR);

	if (status & FH8626_JPEG_INT_DONE) {
		spin_lock(&dev->slock);
		if (dev->inflight) {
			dev->encoded_size =
				readl(dev->regs + FH8626_JPEG_STREAM_SIZE);
			dev->inflight = false;
			complete(&dev->done);
		}
		spin_unlock(&dev->slock);
	}

	return IRQ_HANDLED;
}

static void fh8626_jpeg_queue_error_all(struct fh8626_jpeg_dev *dev)
{
	struct v4l2_fh *fh;
	unsigned long flags;

	/* release() removes the fh before destroying its m2m queues. */
	spin_lock_irqsave(&dev->vfd.fh_lock, flags);
	list_for_each_entry(fh, &dev->vfd.fh_list, list) {
		vb2_queue_error(v4l2_m2m_get_src_vq(fh->m2m_ctx));
		vb2_queue_error(v4l2_m2m_get_dst_vq(fh->m2m_ctx));
	}
	spin_unlock_irqrestore(&dev->vfd.fh_lock, flags);
}

static void fh8626_jpeg_worker(struct work_struct *work)
{
	struct fh8626_jpeg_ctx *ctx =
		container_of(work, struct fh8626_jpeg_ctx, work);
	struct fh8626_jpeg_dev *dev = ctx->dev;
	struct vb2_v4l2_buffer *src_buf, *dst_buf;
	enum vb2_buffer_state state = VB2_BUF_STATE_ERROR;
	u32 width, height, aligned_w, aligned_h;
	u32 mcu_w, mcu_h, mcu_count, burst;
	u32 rotate, cfg_w, cfg_h, cfg_mcu_w, cfg_mcu_h;
	bool transposed;
	u32 cap_size, stream_len = 0;
	u32 frame_qp;
	dma_addr_t y_dma, uv_dma;
	unsigned long flags;
	int ret;

	src_buf = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	dst_buf = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
	if (!src_buf || !dst_buf)
		goto finish;
	if (READ_ONCE(dev->removing) || READ_ONCE(dev->faulted) ||
	    READ_ONCE(ctx->aborting))
		goto done;

	cap_size = min_t(u32, ctx->cap_q.sizeimage,
			 FH8626_JPEG_MAX_STREAM_SIZE);
	cap_size &= ~15U;

	width = ctx->out_q.width;
	height = ctx->out_q.height;
	aligned_w = ALIGN(width, FH8626_JPEG_ALIGN_WIDTH);
	aligned_h = ALIGN(height, FH8626_JPEG_ALIGN_HEIGHT);
	/* Rotation in 90 degree steps; odd steps swap width and height. */
	rotate = READ_ONCE(ctx->rotate) & 3;
	transposed = (rotate == 1 || rotate == 3);

	if (!fh8626_jpeg_get_dma(dev, &dev->dst_cpu, &dev->dst_dma,
				 &dev->dst_size, cap_size))
		goto done;
	if (fh8626_jpeg_direct(ctx, &src_buf->vb2_buf, rotate)) {
		y_dma = vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 0);
		uv_dma = y_dma + ctx->out_q.bytesperline * height;
	} else {
		const u8 *src = vb2_plane_vaddr(&src_buf->vb2_buf, 0);

		if (!src || !fh8626_jpeg_get_dma(dev, &dev->src_cpu,
						 &dev->src_dma, &dev->src_size,
						 aligned_w * aligned_h * 3 / 2))
			goto done;
		/*
		 * Pad above for 90/180 degrees: rotation moves the first input
		 * rows to the end of the coded picture.
		 */
		fh8626_jpeg_copy_input(dev, src, ctx->out_q.bytesperline,
				       height, aligned_w, aligned_h,
				       rotate == 1 || rotate == 2);
		y_dma = dev->src_dma;
		uv_dma = dev->src_dma + aligned_w * aligned_h;
	}
	/* Clear so the EOI scan never sees bytes from a previous frame. */
	memset(dev->dst_cpu, 0, cap_size);

	mcu_w = (width + 15) / 16;
	mcu_h = (height + 15) / 16;
	cfg_h = transposed ? aligned_w : aligned_h;
	cfg_w = transposed ? aligned_h : aligned_w;
	cfg_mcu_h = transposed ? mcu_w : mcu_h;
	cfg_mcu_w = transposed ? mcu_h : mcu_w;

	mcu_count = (((cfg_mcu_h - 1) & 0xff) << 16) | ((cfg_mcu_w - 1) & 0xff);
	/* The DMA burst length follows the input row width. */
	burst = !(aligned_w & 63) ? 7 : !(aligned_w & 31) ? 3 : 1;

	writel(readl(dev->regs + FH8626_JPEG_CHIP_CFG) |
	       FH8626_JPEG_CHIP_CFG_BIT,
	       dev->regs + FH8626_JPEG_CHIP_CFG);

	writel((cfg_h << 16) | 0x0112, dev->regs + FH8626_JPEG_HEIGHT_CFG);
	writel(cfg_w << 16, dev->regs + FH8626_JPEG_WIDTH_CFG);
	writel(y_dma, dev->regs + FH8626_JPEG_Y_ADDR);
	writel(uv_dma, dev->regs + FH8626_JPEG_UV_ADDR);
	writel(mcu_count, dev->regs + FH8626_JPEG_MCU_COUNT);
	/* The input row width stays fixed during rotation. */
	writel((aligned_w << 16) | (rotate << 8) | FH8626_JPEG_INPUT_MEMORY | 1,
	       dev->regs + FH8626_JPEG_ENC_CTRL);
	writel(burst, dev->regs + FH8626_JPEG_BURST_CFG);
	writel(FH8626_JPEG_MODE_VALUE, dev->regs + FH8626_JPEG_MODE);
	writel(dev->dst_dma, dev->regs + FH8626_JPEG_BS_START_ADDR);
	writel(dev->dst_dma, dev->regs + FH8626_JPEG_BS_CURR_ADDR);
	writel(cap_size, dev->regs + FH8626_JPEG_BS_LIMIT);
	writel(cap_size, dev->regs + FH8626_JPEG_BS_MAX_SIZE);
	writel(0, dev->regs + FH8626_JPEG_SLICE_CFG);
	if (ctx->cap_q.pixelformat == V4L2_PIX_FMT_MJPEG) {
		frame_qp = fh8626_jpeg_rc_plan(ctx);
		ctx->current_qp = frame_qp;
		fh8626_jpeg_set_qtable(dev, frame_qp);
	} else {
		frame_qp = READ_ONCE(ctx->quality);
		fh8626_jpeg_set_qtable(dev, frame_qp);
	}

	writel(readl(dev->regs + FH8626_JPEG_INT_STATUS),
	       dev->regs + FH8626_JPEG_INT_CLEAR);
	synchronize_irq(dev->irq);

	spin_lock_irqsave(&dev->slock, flags);
	if (ctx->aborting || dev->removing) {
		spin_unlock_irqrestore(&dev->slock, flags);
		goto done;
	}
	reinit_completion(&dev->done);
	/* Retain the module and private DMA until stop is confirmed. */
	__module_get(THIS_MODULE);
	dev->module_pinned = true;
	dev->inflight = true;
	dev->encoded_size = 0;
	dma_wmb();
	writel(FH8626_JPEG_CTRL_START, dev->regs + FH8626_JPEG_CTRL);
	spin_unlock_irqrestore(&dev->slock, flags);

	ret = wait_for_completion_timeout(&dev->done,
					  msecs_to_jiffies(1000));
	spin_lock_irqsave(&dev->slock, flags);
	dev->inflight = false;
	if (ctx->aborting || dev->removing)
		ret = 0;
	spin_unlock_irqrestore(&dev->slock, flags);

	if (!ret || fh8626_jpeg_wait_idle(dev) ||
	    readl(dev->regs + FH8626_JPEG_HW_ID) !=
	    FH8626_JPEG_EXPECTED_HW_ID ||
	    readl(dev->regs + FH8626_JPEG_MODE) != FH8626_JPEG_MODE_VALUE) {
		bool aborted = READ_ONCE(ctx->aborting) ||
			       READ_ONCE(dev->removing);

		/*
		 * No safe block reset is known. Reuse requires idle and a valid
		 * hardware ID; otherwise retain the device and its DMA storage.
		 */
		ret = fh8626_jpeg_wait_idle(dev);
		if (!ret && readl(dev->regs + FH8626_JPEG_HW_ID) !=
			    FH8626_JPEG_EXPECTED_HW_ID)
			ret = -EIO;
		synchronize_irq(dev->irq);
		if (!ret && !aborted)
			dev_warn_ratelimited(dev->dev,
					     "frame did not complete\n");
		if (ret) {
			WRITE_ONCE(dev->faulted, true);
			fh8626_jpeg_queue_error_all(dev);
			mutex_lock(&fh8626_jpeg_quarantine_lock);
			list_add_tail(&dev->quarantine,
				      &fh8626_jpeg_quarantine);
			mutex_unlock(&fh8626_jpeg_quarantine_lock);
			dev_err(dev->dev, "failed to idle engine: %d\n", ret);
			goto done;
		}
		fh8626_jpeg_init_tables(dev);
		goto unpin;
	}

	dma_rmb();
	stream_len = fh8626_jpeg_patch_length(dev->dst_cpu,
					    dev->encoded_size, cap_size);
	if (stream_len && ((u8 *)dev->dst_cpu)[0] == 0xff &&
	    ((u8 *)dev->dst_cpu)[1] == 0xd8) {
		fh8626_jpeg_patch_sof(dev->dst_cpu, stream_len,
				      transposed ? height : width,
				      transposed ? width : height);
		memcpy(vb2_plane_vaddr(&dst_buf->vb2_buf, 0), dev->dst_cpu,
		       stream_len);
		state = VB2_BUF_STATE_DONE;
		if (ctx->cap_q.pixelformat == V4L2_PIX_FMT_MJPEG)
			fh8626_jpeg_rc_update(ctx, stream_len, frame_qp);
	} else {
		dev_warn_ratelimited(dev->dev,
				     "frame does not fit the %u-byte buffer\n",
				     cap_size);
		if (ctx->cap_q.pixelformat == V4L2_PIX_FMT_MJPEG)
			fh8626_jpeg_rc_update(ctx, cap_size, frame_qp);
	}
unpin:
	dev->module_pinned = false;
	module_put(THIS_MODULE);
done:
	if (READ_ONCE(ctx->aborting) || READ_ONCE(dev->removing))
		state = VB2_BUF_STATE_ERROR;
	src_buf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	dst_buf = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
	if (dst_buf) {
		dst_buf->sequence = ctx->sequence_cap++;
		dst_buf->field = V4L2_FIELD_NONE;
		dst_buf->flags &= ~(V4L2_BUF_FLAG_TSTAMP_SRC_MASK |
			V4L2_BUF_FLAG_KEYFRAME |
			V4L2_BUF_FLAG_PFRAME | V4L2_BUF_FLAG_BFRAME |
			V4L2_BUF_FLAG_LAST);
		if (src_buf) {
			dst_buf->flags |= src_buf->flags &
					  V4L2_BUF_FLAG_TSTAMP_SRC_MASK;
			dst_buf->vb2_buf.timestamp = src_buf->vb2_buf.timestamp;
		}
		if (state == VB2_BUF_STATE_DONE)
			dst_buf->flags |= V4L2_BUF_FLAG_KEYFRAME;
		vb2_set_plane_payload(&dst_buf->vb2_buf, 0,
				      state == VB2_BUF_STATE_DONE ?
				      stream_len : 0);
	}
	if (src_buf) {
		src_buf->sequence = ctx->sequence_out++;
		v4l2_m2m_buf_done(src_buf, state);
	}
	if (dst_buf)
		v4l2_m2m_buf_done(dst_buf, state);
finish:
	v4l2_m2m_job_finish(dev->m2m_dev, ctx->m2m_ctx);
}

static void fh8626_jpeg_device_run(void *priv)
{
	struct fh8626_jpeg_ctx *ctx = priv;

	queue_work(ctx->dev->workqueue, &ctx->work);
}

static int fh8626_jpeg_job_ready(void *priv)
{
	struct fh8626_jpeg_ctx *ctx = priv;

	if (READ_ONCE(ctx->dev->removing) || READ_ONCE(ctx->dev->faulted) ||
	    READ_ONCE(ctx->aborting))
		return 0;
	return v4l2_m2m_num_src_bufs_ready(ctx->m2m_ctx) > 0 &&
	       v4l2_m2m_num_dst_bufs_ready(ctx->m2m_ctx) > 0;
}

static void fh8626_jpeg_job_abort(void *priv)
{
	struct fh8626_jpeg_ctx *ctx = priv;
	struct fh8626_jpeg_dev *dev = ctx->dev;
	unsigned long flags;

	spin_lock_irqsave(&dev->slock, flags);
	ctx->aborting = true;
	if (v4l2_m2m_get_curr_priv(dev->m2m_dev) == ctx)
		complete(&dev->done);
	spin_unlock_irqrestore(&dev->slock, flags);
	/* ctx_release may free m2m_ctx as soon as job_finish wakes it. */
	flush_work(&ctx->work);
}

static const struct v4l2_m2m_ops fh8626_jpeg_m2m_ops = {
	.device_run	= fh8626_jpeg_device_run,
	.job_ready	= fh8626_jpeg_job_ready,
	.job_abort	= fh8626_jpeg_job_abort,
};

static int fh8626_jpeg_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct fh8626_jpeg_ctx *ctx = ctrl_to_ctx(ctrl);

	switch (ctrl->id) {
	case V4L2_CID_JPEG_COMPRESSION_QUALITY:
		WRITE_ONCE(ctx->quality, ctrl->val);
		break;
	case V4L2_CID_ROTATE:
		WRITE_ONCE(ctx->rotate, ctrl->val / 90);
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE_MODE:
		WRITE_ONCE(ctx->bitrate_mode, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		WRITE_ONCE(ctx->bitrate, ctrl->val);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ctrl_ops fh8626_jpeg_ctrl_ops = {
	.s_ctrl = fh8626_jpeg_s_ctrl,
};

static int fh8626_jpeg_setup_ctrls(struct fh8626_jpeg_ctx *ctx)
{
	int ret;

	v4l2_ctrl_handler_init(&ctx->ctrl_handler, 4);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_jpeg_ctrl_ops,
			  V4L2_CID_JPEG_COMPRESSION_QUALITY,
			  1, 100, 1, FH8626_JPEG_DEFAULT_QUALITY);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_jpeg_ctrl_ops,
			  V4L2_CID_ROTATE, 0, 270, 90, 0);
	v4l2_ctrl_new_std_menu(&ctx->ctrl_handler, &fh8626_jpeg_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_BITRATE_MODE,
			       V4L2_MPEG_VIDEO_BITRATE_MODE_CBR,
			       ~((1 << V4L2_MPEG_VIDEO_BITRATE_MODE_VBR) |
				 (1 << V4L2_MPEG_VIDEO_BITRATE_MODE_CBR)),
			       V4L2_MPEG_VIDEO_BITRATE_MODE_VBR);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_jpeg_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_BITRATE,
			  64000, 20000000, 1, 2000000);

	if (ctx->ctrl_handler.error) {
		ret = ctx->ctrl_handler.error;
		v4l2_ctrl_handler_free(&ctx->ctrl_handler);
		return ret;
	}

	ret = v4l2_ctrl_handler_setup(&ctx->ctrl_handler);
	if (ret)
		v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	return ret;
}

static int fh8626_jpeg_queue_setup(struct vb2_queue *vq,
				   unsigned int *nbuffers,
				   unsigned int *nplanes,
				   unsigned int sizes[],
				   struct device *alloc_devs[])
{
	struct fh8626_jpeg_ctx *ctx = vb2_get_drv_priv(vq);
	struct fh8626_jpeg_q_data *q_data = get_q_data(ctx, vq->type);
	u32 bytes = q_data->sizeimage, limit;

	/*
	 * Pad MMAP input for direct DMA; imported images without padding
	 * use the staging copy.
	 */
	if (V4L2_TYPE_IS_OUTPUT(vq->type) && vq->memory == VB2_MEMORY_MMAP)
		bytes = PAGE_ALIGN(max_t(size_t, bytes,
					 fh8626_jpeg_direct_size(q_data)));
	if (*nplanes) {
		if (*nplanes != 1 || sizes[0] < bytes)
			return -EINVAL;
		bytes = sizes[0];
	}
	/* Limit each queue to 3 MiB of local allocations; exclude imports. */
	if (vq->memory == VB2_MEMORY_DMABUF) {
		*nplanes = 1;
		sizes[0] = bytes;
		return 0;
	}
	limit = (3U * 1024 * 1024) / bytes;
	if (vq->num_buffers >= limit)
		return -ENOMEM;
	*nbuffers = min_t(unsigned int, *nbuffers, limit - vq->num_buffers);
	if (!*nbuffers)
		return -ENOMEM;

	*nplanes = 1;
	sizes[0] = bytes;

	return 0;
}

static int fh8626_jpeg_buf_prepare(struct vb2_buffer *vb)
{
	struct fh8626_jpeg_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct fh8626_jpeg_q_data *q_data;

	q_data = get_q_data(ctx, vb->vb2_queue->type);

	if (vb2_plane_size(vb, 0) < q_data->sizeimage)
		return -EINVAL;

	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		if (vb2_get_plane_payload(vb, 0) < q_data->sizeimage)
			return -EINVAL;
		if (to_vb2_v4l2_buffer(vb)->field != V4L2_FIELD_NONE &&
		    to_vb2_v4l2_buffer(vb)->field != V4L2_FIELD_ANY)
			return -EINVAL;
		if (!fh8626_jpeg_direct(ctx, vb, 0) && !vb2_plane_vaddr(vb, 0))
			return -EINVAL;
		to_vb2_v4l2_buffer(vb)->field = V4L2_FIELD_NONE;
		vb2_set_plane_payload(vb, 0, q_data->sizeimage);
	} else {
		vb2_set_plane_payload(vb, 0, 0);
	}

	return 0;
}

static void fh8626_jpeg_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct fh8626_jpeg_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_m2m_buf_queue(ctx->m2m_ctx, vbuf);
}

static int fh8626_jpeg_start_streaming(struct vb2_queue *q,
				       unsigned int count)
{
	struct fh8626_jpeg_ctx *ctx = vb2_get_drv_priv(q);
	struct vb2_v4l2_buffer *buf;

	if (READ_ONCE(ctx->dev->removing) || READ_ONCE(ctx->dev->faulted)) {
		while ((buf = V4L2_TYPE_IS_OUTPUT(q->type) ?
			v4l2_m2m_src_buf_remove(ctx->m2m_ctx) :
			v4l2_m2m_dst_buf_remove(ctx->m2m_ctx)))
			v4l2_m2m_buf_done(buf, VB2_BUF_STATE_QUEUED);
		return -ENODEV;
	}

	WRITE_ONCE(ctx->aborting, false);
	if (V4L2_TYPE_IS_OUTPUT(q->type)) {
		ctx->sequence_out = 0;
	} else {
		ctx->sequence_cap = 0;
		ctx->current_qp = clamp_val(READ_ONCE(ctx->quality), 1U,
					    FH8626_JPEG_RC_MAX_QUALITY);
		ctx->rc_error = 0;
		ctx->rc_valid = false;
	}

	return 0;
}

static void fh8626_jpeg_stop_streaming(struct vb2_queue *q)
{
	struct fh8626_jpeg_ctx *ctx = vb2_get_drv_priv(q);
	struct vb2_v4l2_buffer *vbuf;

	flush_work(&ctx->work);
	for (;;) {
		if (V4L2_TYPE_IS_OUTPUT(q->type))
			vbuf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		else
			vbuf = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

		if (!vbuf)
			break;
		v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_ERROR);
	}
}

static const struct vb2_ops fh8626_jpeg_qops = {
	.queue_setup	 = fh8626_jpeg_queue_setup,
	.buf_prepare	 = fh8626_jpeg_buf_prepare,
	.buf_queue	 = fh8626_jpeg_buf_queue,
	.wait_prepare	 = vb2_ops_wait_prepare,
	.wait_finish	 = vb2_ops_wait_finish,
	.start_streaming = fh8626_jpeg_start_streaming,
	.stop_streaming	 = fh8626_jpeg_stop_streaming,
};

static int fh8626_jpeg_queue_init(void *priv, struct vb2_queue *src_vq,
				  struct vb2_queue *dst_vq)
{
	struct fh8626_jpeg_ctx *ctx = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	/*
	 * USERPTR cannot guarantee the physically contiguous storage required
	 * by this engine; use MMAP or import a contiguous dma-buf.
	 */
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->ops = &fh8626_jpeg_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->dev->dev_mutex;
	src_vq->dev = ctx->dev->dev;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_vq->io_modes = VB2_MMAP;
	dst_vq->drv_priv = ctx;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->ops = &fh8626_jpeg_qops;
	dst_vq->mem_ops = &vb2_vmalloc_memops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->dev->dev_mutex;
	dst_vq->dev = ctx->dev->dev;

	return vb2_queue_init(dst_vq);
}

static int fh8626_jpeg_querycap(struct file *file, void *priv,
				struct v4l2_capability *cap)
{
	strlcpy(cap->driver, DRIVER_NAME, sizeof(cap->driver));
	strlcpy(cap->card, "FH8626 JPEG Encoder", sizeof(cap->card));
	strlcpy(cap->bus_info, "platform:" DRIVER_NAME,
		sizeof(cap->bus_info));
	return 0;
}

static int fh8626_jpeg_enum_fmt_vid_out(struct file *file, void *priv,
				       struct v4l2_fmtdesc *f)
{
	if (f->index != 0)
		return -EINVAL;

	f->pixelformat = V4L2_PIX_FMT_NV12;
	return 0;
}

static int fh8626_jpeg_enum_fmt_vid_cap(struct file *file, void *priv,
				       struct v4l2_fmtdesc *f)
{
	if (f->index == 0)
		f->pixelformat = V4L2_PIX_FMT_JPEG;
	else if (f->index == 1)
		f->pixelformat = V4L2_PIX_FMT_MJPEG;
	else
		return -EINVAL;

	return 0;
}

static int fh8626_jpeg_g_fmt(struct file *file, void *priv,
			     struct v4l2_format *f)
{
	struct fh8626_jpeg_ctx *ctx = fh_to_ctx(priv);
	struct fh8626_jpeg_q_data *q_data = get_q_data(ctx, f->type);

	f->fmt.pix.field = V4L2_FIELD_NONE;
	f->fmt.pix.colorspace = V4L2_COLORSPACE_JPEG;

	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		f->fmt.pix.width = q_data->width;
		f->fmt.pix.height = q_data->height;
		f->fmt.pix.bytesperline = q_data->bytesperline;
		f->fmt.pix.sizeimage = q_data->sizeimage;
		f->fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
	} else {
		bool transposed = (READ_ONCE(ctx->rotate) == 1 ||
				   READ_ONCE(ctx->rotate) == 3);

		f->fmt.pix.width = transposed ? ctx->out_q.height :
						ctx->out_q.width;
		f->fmt.pix.height = transposed ? ctx->out_q.width :
						 ctx->out_q.height;
		f->fmt.pix.bytesperline = 0;
		f->fmt.pix.sizeimage = q_data->sizeimage;
		f->fmt.pix.pixelformat = q_data->pixelformat ?
			q_data->pixelformat : V4L2_PIX_FMT_JPEG;
	}

	return 0;
}

static int fh8626_jpeg_try_fmt(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	struct fh8626_jpeg_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_pix_format *pix = &f->fmt.pix;

	pix->field = V4L2_FIELD_NONE;
	pix->colorspace = V4L2_COLORSPACE_JPEG;

	pix->width = clamp_val(pix->width, FH8626_JPEG_MIN_WIDTH,
			       FH8626_JPEG_MAX_WIDTH);
	pix->height = clamp_val(pix->height, FH8626_JPEG_MIN_HEIGHT,
				FH8626_JPEG_MAX_HEIGHT);

	/*
	 * The engine codes whole 16x16 MCUs. The driver pads the picture
	 * to MCU rows itself and the frame header carries the real height.
	 */
	pix->width = ALIGN(pix->width, FH8626_JPEG_ALIGN_WIDTH);
	pix->height = ALIGN(pix->height, 2);

	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		pix->pixelformat = V4L2_PIX_FMT_NV12;
		pix->bytesperline = pix->width;
		pix->sizeimage = pix->width * pix->height * 3 / 2;
	} else {
		bool transposed = (READ_ONCE(ctx->rotate) == 1 ||
				   READ_ONCE(ctx->rotate) == 3);

		pix->width = transposed ? ctx->out_q.height : ctx->out_q.width;
		pix->height = transposed ? ctx->out_q.width :
					   ctx->out_q.height;
		if (pix->pixelformat != V4L2_PIX_FMT_MJPEG)
			pix->pixelformat = V4L2_PIX_FMT_JPEG;
		pix->bytesperline = 0;
		if (!pix->sizeimage ||
		    pix->sizeimage < (pix->width * pix->height / 2))
			pix->sizeimage = max_t(u32, 65536,
				PAGE_ALIGN(pix->width * pix->height));
		pix->sizeimage = min_t(u32, pix->sizeimage,
					 FH8626_JPEG_MAX_STREAM_SIZE);
	}

	return 0;
}

static int fh8626_jpeg_s_fmt(struct file *file, void *priv,
			     struct v4l2_format *f)
{
	struct fh8626_jpeg_ctx *ctx = fh_to_ctx(priv);
	struct vb2_queue *vq;
	struct fh8626_jpeg_q_data *q_data;
	int ret;

	ret = fh8626_jpeg_try_fmt(file, priv, f);
	if (ret)
		return ret;

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	if (vb2_is_busy(vq))
		return -EBUSY;
	if (V4L2_TYPE_IS_OUTPUT(f->type) &&
	    vb2_is_busy(v4l2_m2m_get_dst_vq(ctx->m2m_ctx)))
		return -EBUSY;

	q_data = get_q_data(ctx, f->type);
	q_data->width = f->fmt.pix.width;
	q_data->height = f->fmt.pix.height;
	q_data->bytesperline = f->fmt.pix.bytesperline;
	q_data->sizeimage = f->fmt.pix.sizeimage;
	q_data->pixelformat = f->fmt.pix.pixelformat;

	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		bool transposed = (READ_ONCE(ctx->rotate) == 1 ||
				   READ_ONCE(ctx->rotate) == 3);

		ctx->cap_q.width = transposed ? f->fmt.pix.height :
						f->fmt.pix.width;
		ctx->cap_q.height = transposed ? f->fmt.pix.width :
						 f->fmt.pix.height;
		ctx->cap_q.sizeimage = min_t(u32, FH8626_JPEG_MAX_STREAM_SIZE,
			max_t(u32, 65536,
			      PAGE_ALIGN(f->fmt.pix.width *
					 f->fmt.pix.height)));
	}

	return 0;
}

static int fh8626_jpeg_enum_framesizes(struct file *file, void *priv,
				       struct v4l2_frmsizeenum *fsize)
{
	if (fsize->index != 0)
		return -EINVAL;

	if (fsize->pixel_format != V4L2_PIX_FMT_JPEG &&
	    fsize->pixel_format != V4L2_PIX_FMT_MJPEG &&
	    fsize->pixel_format != V4L2_PIX_FMT_NV12)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	fsize->stepwise.min_width = FH8626_JPEG_MIN_WIDTH;
	fsize->stepwise.max_width = FH8626_JPEG_MAX_WIDTH;
	fsize->stepwise.step_width = FH8626_JPEG_ALIGN_WIDTH;
	fsize->stepwise.min_height = FH8626_JPEG_MIN_HEIGHT;
	fsize->stepwise.max_height = FH8626_JPEG_MAX_HEIGHT;
	fsize->stepwise.step_height = 2;

	return 0;
}

static const struct v4l2_fract fh8626_jpeg_frameintervals[] = {
	{ 1, 25 },
	{ 1, 30 },
	{ 1, 15 },
	{ 1, 10 },
	{ 1, 5 },
};

static int fh8626_jpeg_enum_frameintervals(struct file *file, void *priv,
					   struct v4l2_frmivalenum *fival)
{
	if (fival->index >= ARRAY_SIZE(fh8626_jpeg_frameintervals))
		return -EINVAL;

	if (fival->pixel_format != V4L2_PIX_FMT_JPEG &&
	    fival->pixel_format != V4L2_PIX_FMT_MJPEG &&
	    fival->pixel_format != V4L2_PIX_FMT_NV12)
		return -EINVAL;

	if (fival->width < FH8626_JPEG_MIN_WIDTH ||
	    fival->width > FH8626_JPEG_MAX_WIDTH ||
	    fival->height < FH8626_JPEG_MIN_HEIGHT ||
	    fival->height > FH8626_JPEG_MAX_HEIGHT)
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete = fh8626_jpeg_frameintervals[fival->index];

	return 0;
}

static int fh8626_jpeg_g_selection(struct file *file, void *priv,
				   struct v4l2_selection *s)
{
	struct fh8626_jpeg_ctx *ctx = fh_to_ctx(priv);

	if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT &&
	    s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	switch (s->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP:
		if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
			return -EINVAL;
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = ctx->out_q.width;
		s->r.height = ctx->out_q.height;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int fh8626_jpeg_g_parm(struct file *file, void *priv,
			      struct v4l2_streamparm *parm)
{
	struct fh8626_jpeg_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_fract *tpf;

	if (parm->type != V4L2_BUF_TYPE_VIDEO_OUTPUT &&
	    parm->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (V4L2_TYPE_IS_OUTPUT(parm->type)) {
		parm->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
		tpf = &parm->parm.output.timeperframe;
	} else {
		parm->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
		tpf = &parm->parm.capture.timeperframe;
	}

	tpf->numerator = ctx->target_fps.denominator ?
			 ctx->target_fps.denominator : 1;
	tpf->denominator = ctx->target_fps.numerator ?
			   ctx->target_fps.numerator : 25;

	return 0;
}

static int fh8626_jpeg_s_parm(struct file *file, void *priv,
			      struct v4l2_streamparm *parm)
{
	struct fh8626_jpeg_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_fract *tpf;

	if (parm->type != V4L2_BUF_TYPE_VIDEO_OUTPUT &&
	    parm->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (V4L2_TYPE_IS_OUTPUT(parm->type)) {
		parm->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
		tpf = &parm->parm.output.timeperframe;
	} else {
		parm->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
		tpf = &parm->parm.capture.timeperframe;
	}

	if (!tpf->numerator || !tpf->denominator) {
		tpf->numerator = 1;
		tpf->denominator = 25;
	}

	ctx->target_fps.numerator = tpf->denominator;
	ctx->target_fps.denominator = tpf->numerator;

	tpf->numerator = ctx->target_fps.denominator;
	tpf->denominator = ctx->target_fps.numerator;

	return 0;
}

static const struct v4l2_ioctl_ops fh8626_jpeg_ioctl_ops = {
	.vidioc_querycap		= fh8626_jpeg_querycap,

	.vidioc_enum_fmt_vid_cap	= fh8626_jpeg_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= fh8626_jpeg_g_fmt,
	.vidioc_try_fmt_vid_cap		= fh8626_jpeg_try_fmt,
	.vidioc_s_fmt_vid_cap		= fh8626_jpeg_s_fmt,

	.vidioc_enum_fmt_vid_out	= fh8626_jpeg_enum_fmt_vid_out,
	.vidioc_g_fmt_vid_out		= fh8626_jpeg_g_fmt,
	.vidioc_try_fmt_vid_out		= fh8626_jpeg_try_fmt,
	.vidioc_s_fmt_vid_out		= fh8626_jpeg_s_fmt,

	.vidioc_enum_framesizes		= fh8626_jpeg_enum_framesizes,
	.vidioc_enum_frameintervals	= fh8626_jpeg_enum_frameintervals,

	.vidioc_g_selection		= fh8626_jpeg_g_selection,

	.vidioc_g_parm			= fh8626_jpeg_g_parm,
	.vidioc_s_parm			= fh8626_jpeg_s_parm,

	.vidioc_reqbufs			= v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf		= v4l2_m2m_ioctl_querybuf,
	.vidioc_create_bufs		= v4l2_m2m_ioctl_create_bufs,
	.vidioc_prepare_buf		= v4l2_m2m_ioctl_prepare_buf,
	.vidioc_qbuf			= v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf			= v4l2_m2m_ioctl_dqbuf,
	.vidioc_expbuf			= v4l2_m2m_ioctl_expbuf,

	.vidioc_streamon		= v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff		= v4l2_m2m_ioctl_streamoff,

	.vidioc_subscribe_event		= v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,
};

static int fh8626_jpeg_open(struct file *file)
{
	struct fh8626_jpeg_dev *dev = video_drvdata(file);
	struct fh8626_jpeg_ctx *ctx;
	int ret;

	if (mutex_lock_interruptible(&dev->dev_mutex))
		return -ERESTARTSYS;
	if (dev->removing || dev->faulted) {
		mutex_unlock(&dev->dev_mutex);
		return -ENODEV;
	}

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		mutex_unlock(&dev->dev_mutex);
		return -ENOMEM;
	}

	v4l2_fh_init(&ctx->fh, video_devdata(file));
	file->private_data = &ctx->fh;
	ctx->dev = dev;
	INIT_WORK(&ctx->work, fh8626_jpeg_worker);

	ctx->out_q.width = FH8626_JPEG_DEFAULT_WIDTH;
	ctx->out_q.height = FH8626_JPEG_DEFAULT_HEIGHT;
	ctx->out_q.bytesperline = FH8626_JPEG_DEFAULT_WIDTH;
	ctx->out_q.sizeimage =
		FH8626_JPEG_DEFAULT_WIDTH * FH8626_JPEG_DEFAULT_HEIGHT * 3 / 2;
	ctx->out_q.pixelformat = V4L2_PIX_FMT_NV12;

	ctx->cap_q.width = FH8626_JPEG_DEFAULT_WIDTH;
	ctx->cap_q.height = FH8626_JPEG_DEFAULT_HEIGHT;
	ctx->cap_q.bytesperline = 0;
	ctx->cap_q.sizeimage =
		PAGE_ALIGN(FH8626_JPEG_DEFAULT_WIDTH *
			   FH8626_JPEG_DEFAULT_HEIGHT);
	ctx->cap_q.pixelformat = V4L2_PIX_FMT_JPEG;

	ctx->quality = FH8626_JPEG_DEFAULT_QUALITY;
	ctx->rotate = 0;
	ctx->bitrate_mode = V4L2_MPEG_VIDEO_BITRATE_MODE_VBR;
	ctx->bitrate = 2000000;
	ctx->target_fps.numerator = 25;
	ctx->target_fps.denominator = 1;
	ctx->current_qp = FH8626_JPEG_DEFAULT_QUALITY;

	ret = fh8626_jpeg_setup_ctrls(ctx);
	if (ret)
		goto err_free;

	ctx->fh.ctrl_handler = &ctx->ctrl_handler;

	ctx->m2m_ctx = v4l2_m2m_ctx_init(dev->m2m_dev, ctx,
					  fh8626_jpeg_queue_init);
	if (IS_ERR(ctx->m2m_ctx)) {
		ret = PTR_ERR(ctx->m2m_ctx);
		goto err_ctrl;
	}
	ctx->fh.m2m_ctx = ctx->m2m_ctx;

	list_add_tail(&ctx->list, &dev->contexts);
	v4l2_fh_add(&ctx->fh);
	mutex_unlock(&dev->dev_mutex);
	return 0;

err_ctrl:
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
err_free:
	v4l2_fh_exit(&ctx->fh);
	file->private_data = NULL;
	kfree(ctx);
	mutex_unlock(&dev->dev_mutex);
	return ret;
}

static int fh8626_jpeg_release(struct file *file)
{
	struct fh8626_jpeg_dev *dev = video_drvdata(file);
	struct fh8626_jpeg_ctx *ctx = fh_to_ctx(file->private_data);

	mutex_lock(&dev->dev_mutex);
	/* Leave the fault fan-out list before the queues are freed. */
	v4l2_fh_del(&ctx->fh);
	v4l2_m2m_ctx_release(ctx->m2m_ctx);
	/* job_finish() wakes ctx_release() before the worker has returned. */
	flush_work(&ctx->work);
	list_del(&ctx->list);
	if (list_empty(&dev->contexts) && !dev->faulted)
		fh8626_jpeg_free_dma(dev);
	v4l2_fh_exit(&ctx->fh);
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	kfree(ctx);
	mutex_unlock(&dev->dev_mutex);

	return 0;
}

static unsigned int fh8626_jpeg_poll(struct file *file, poll_table *wait)
{
	struct fh8626_jpeg_ctx *ctx = fh_to_ctx(file->private_data);
	unsigned int ret;

	mutex_lock(&ctx->dev->dev_mutex);
	if (ctx->dev->removing) {
		ret = POLLERR | POLLHUP;
	} else {
		ret = v4l2_m2m_poll(file, ctx->m2m_ctx, wait);
		if (ctx->dev->faulted ||
		    v4l2_m2m_get_src_vq(ctx->m2m_ctx)->error ||
		    v4l2_m2m_get_dst_vq(ctx->m2m_ctx)->error)
			ret |= POLLERR;
	}
	mutex_unlock(&ctx->dev->dev_mutex);
	return ret;
}

static const struct v4l2_file_operations fh8626_jpeg_fops = {
	.owner		= THIS_MODULE,
	.open		= fh8626_jpeg_open,
	.release	= fh8626_jpeg_release,
	.poll		= fh8626_jpeg_poll,
	.unlocked_ioctl	= fh8626_video_ioctl2,
	.mmap		= v4l2_m2m_fop_mmap,
};

static void fh8626_jpeg_dev_release(struct v4l2_device *v4l2_dev)
{
	struct fh8626_jpeg_dev *dev = container_of(v4l2_dev,
						   struct fh8626_jpeg_dev,
						   v4l2_dev);

	if (dev->m2m_dev)
		v4l2_m2m_release(dev->m2m_dev);
	/* The engine may still use the DMA buffers after a failed stop. */
	if (dev->faulted)
		return;
	fh8626_jpeg_free_dma(dev);
	put_device(dev->dev);
	kfree(dev);
}

static int fh8626_jpeg_probe(struct platform_device *pdev)
{
	struct fh8626_jpeg_dev *dev, *entry;
	struct video_device *vfd;
	struct resource *res;
	u32 hw_id;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res || resource_size(res) < 0x600)
		return -EINVAL;
	mutex_lock(&fh8626_jpeg_quarantine_lock);
	list_for_each_entry(entry, &fh8626_jpeg_quarantine, quarantine) {
		if (entry->resource_start == res->start) {
			mutex_unlock(&fh8626_jpeg_quarantine_lock);
			return -EBUSY;
		}
	}
	mutex_unlock(&fh8626_jpeg_quarantine_lock);
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->dev = get_device(&pdev->dev);
	mutex_init(&dev->dev_mutex);
	spin_lock_init(&dev->slock);
	init_completion(&dev->done);
	INIT_LIST_HEAD(&dev->contexts);
	INIT_LIST_HEAD(&dev->quarantine);
	dev->resource_start = res->start;
	dev->workqueue = alloc_ordered_workqueue(DRIVER_NAME, WQ_MEM_RECLAIM);
	if (!dev->workqueue) {
		ret = -ENOMEM;
		goto err_free;
	}
	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		goto err_workqueue;

	dev->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dev->regs)) {
		ret = PTR_ERR(dev->regs);
		goto err_workqueue;
	}

	/* Clocks are held explicitly so they survive a quarantined removal. */
	dev->clk = clk_get(&pdev->dev, "jpeg");
	if (IS_ERR(dev->clk)) {
		ret = PTR_ERR(dev->clk);
		goto err_workqueue;
	}

	dev->hclk = clk_get(&pdev->dev, "hclk");
	if (IS_ERR(dev->hclk)) {
		ret = PTR_ERR(dev->hclk);
		goto err_put_clk;
	}
	ret = clk_prepare_enable(dev->clk);
	if (ret)
		goto err_put_hclk;
	ret = clk_prepare_enable(dev->hclk);
	if (ret)
		goto err_clk;
	dev->clocks_enabled = true;

	hw_id = readl(dev->regs + FH8626_JPEG_HW_ID);
	if (hw_id != FH8626_JPEG_EXPECTED_HW_ID) {
		dev_err(&pdev->dev,
			"unexpected HW ID: 0x%08x (expected 0x%08x)\n",
			hw_id, FH8626_JPEG_EXPECTED_HW_ID);
		ret = -ENODEV;
		goto err_hclk;
	}

	fh8626_jpeg_init_tables(dev);
	writel(readl(dev->regs + FH8626_JPEG_INT_STATUS),
	       dev->regs + FH8626_JPEG_INT_CLEAR);

	dev->irq = platform_get_irq(pdev, 0);
	if (dev->irq < 0) {
		ret = dev->irq;
		goto err_hclk;
	}

	ret = devm_request_irq(&pdev->dev, dev->irq, fh8626_jpeg_irq, 0,
			       DRIVER_NAME, dev);
	if (ret) {
		dev_err(&pdev->dev, "failed to request IRQ %d: %d\n",
			dev->irq, ret);
		goto err_hclk;
	}
	dev->irq_registered = true;

	ret = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
	if (ret)
		goto err_irq;
	dev->v4l2_dev.release = fh8626_jpeg_dev_release;

	dev->m2m_dev = v4l2_m2m_init(&fh8626_jpeg_m2m_ops);
	if (IS_ERR(dev->m2m_dev)) {
		ret = PTR_ERR(dev->m2m_dev);
		goto err_v4l2_dev;
	}

	vfd = &dev->vfd;
	vfd->fops = &fh8626_jpeg_fops;
	vfd->ioctl_ops = &fh8626_jpeg_ioctl_ops;
	vfd->release = video_device_release_empty;
	vfd->lock = &dev->dev_mutex;
	vfd->v4l2_dev = &dev->v4l2_dev;
	vfd->vfl_dir = VFL_DIR_M2M;
	vfd->device_caps = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;
	strlcpy(vfd->name, DRIVER_NAME, sizeof(vfd->name));
	video_set_drvdata(vfd, dev);

	ret = video_register_device(vfd, VFL_TYPE_GRABBER, -1);
	if (ret) {
		dev_err(&pdev->dev, "failed to register video device: %d\n",
			ret);
		goto err_m2m;
	}

	platform_set_drvdata(pdev, dev);
	dev_dbg(&pdev->dev, "registered %s\n", video_device_node_name(vfd));

	return 0;

err_m2m:
	v4l2_m2m_release(dev->m2m_dev);
err_v4l2_dev:
	v4l2_device_unregister(&dev->v4l2_dev);
err_irq:
	devm_free_irq(&pdev->dev, dev->irq, dev);
err_hclk:
	clk_disable_unprepare(dev->hclk);
err_clk:
	clk_disable_unprepare(dev->clk);
err_put_hclk:
	clk_put(dev->hclk);
err_put_clk:
	clk_put(dev->clk);
err_workqueue:
	destroy_workqueue(dev->workqueue);
err_free:
	put_device(dev->dev);
	kfree(dev);
	return ret;
}

/* Called with dev_mutex held; workers never acquire it. */
static void fh8626_jpeg_disconnect(struct fh8626_jpeg_dev *dev)
{
	struct fh8626_jpeg_ctx *ctx;
	unsigned long flags;

	if (dev->removing)
		return;
	spin_lock_irqsave(&dev->slock, flags);
	dev->removing = true;
	complete(&dev->done);
	spin_unlock_irqrestore(&dev->slock, flags);
	list_for_each_entry(ctx, &dev->contexts, list) {
		vb2_queue_error(v4l2_m2m_get_src_vq(ctx->m2m_ctx));
		vb2_queue_error(v4l2_m2m_get_dst_vq(ctx->m2m_ctx));
		v4l2_m2m_streamoff(NULL, ctx->m2m_ctx,
				   V4L2_BUF_TYPE_VIDEO_OUTPUT);
		v4l2_m2m_streamoff(NULL, ctx->m2m_ctx,
				   V4L2_BUF_TYPE_VIDEO_CAPTURE);
		flush_work(&ctx->work);
	}
	flush_workqueue(dev->workqueue);
	if (dev->irq_registered) {
		devm_free_irq(dev->dev, dev->irq, dev);
		dev->irq_registered = false;
	}
	/* After a failed idle handshake the clocks must stay on. */
	if (dev->clocks_enabled && !dev->faulted) {
		clk_disable_unprepare(dev->hclk);
		clk_disable_unprepare(dev->clk);
		clk_put(dev->hclk);
		clk_put(dev->clk);
		dev->clocks_enabled = false;
	}
}

static int fh8626_jpeg_remove(struct platform_device *pdev)
{
	struct fh8626_jpeg_dev *dev = platform_get_drvdata(pdev);

	mutex_lock(&dev->dev_mutex);
	fh8626_jpeg_disconnect(dev);
	video_unregister_device(&dev->vfd);
	fh8626_video_wake_events(&dev->vfd);
	v4l2_device_unregister(&dev->v4l2_dev);
	platform_set_drvdata(pdev, NULL);
	mutex_unlock(&dev->dev_mutex);
	destroy_workqueue(dev->workqueue);
	v4l2_device_put(&dev->v4l2_dev);
	return 0;
}

static void fh8626_jpeg_shutdown(struct platform_device *pdev)
{
	struct fh8626_jpeg_dev *dev = platform_get_drvdata(pdev);

	mutex_lock(&dev->dev_mutex);
	fh8626_jpeg_disconnect(dev);
	mutex_unlock(&dev->dev_mutex);
}

static int fh8626_jpeg_suspend(struct device *dev)
{
	return -EOPNOTSUPP;
}

static const struct dev_pm_ops fh8626_jpeg_pm_ops = {
	.suspend = fh8626_jpeg_suspend,
	.freeze = fh8626_jpeg_suspend,
	.poweroff = fh8626_jpeg_suspend,
};

static const struct of_device_id fh8626_jpeg_match[] = {
	{ .compatible = "fullhan,fh8626-jpeg" },
	{ .compatible = "fullhan,fh8626v100-jpeg" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, fh8626_jpeg_match);

static struct platform_driver fh8626_jpeg_driver = {
	.probe	= fh8626_jpeg_probe,
	.remove	= fh8626_jpeg_remove,
	.shutdown = fh8626_jpeg_shutdown,
	.driver	= {
		.name		= DRIVER_NAME,
		.of_match_table	= fh8626_jpeg_match,
		.pm = &fh8626_jpeg_pm_ops,
		.suppress_bind_attrs = true,
	},
};

module_platform_driver(fh8626_jpeg_driver);

MODULE_AUTHOR("Difan Zhang");
MODULE_DESCRIPTION("Fullhan FH8626 V4L2 M2M Hardware JPEG Encoder Driver");
MODULE_LICENSE("GPL v2");
