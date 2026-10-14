// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Anyka AK3918EV200 V4L2 mem2mem H.264 and JPEG encoder.
 *
 * The core is a Hantro/On2 8290, ASIC id 0x82900760: H.264 and JPEG, one
 * picture at a time, maximum width 1280. Which one a frame is comes from the
 * CAPTURE queue's format and is decided per frame, because a JPEG job carries
 * no state: no reference picture, no rate control, no GOP position. So a
 * snapshot costs a running H.264 session one frame's worth of core time and
 * nothing else, and the two can be interleaved from separate contexts.
 *
 * Copyright (C) 2026 Alex Zhang <alex@osqdu.org>
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>

#include <mach/map.h>

#include "ak-venc.h"
#include "ak-venc-compat.h"

/*
 * The clock gate and the soft reset, pulsed in the order VideoStream_Enc_Reset
 * uses: gate the clock off, assert reset, clock on, release reset. Skipping the
 * pulse works once per boot and hangs the board on the second open.
 */
#define AK_VENC_CLK_GATE_REG	0x0800001c
#define AK_VENC_SOFT_RESET_REG	0x08000020
#define AK_VENC_VIDEO_BIT	(1u << 20)

/* swreg1 bit 0, the interrupt line itself. */
#define AK_VENC_IRQ_LINE	0x1u

#define AK_VENC_HDR_MAX		128U

/* The core raises a buffer-full interrupt rather than overrunning, so this is
 * only a floor for what S_FMT reports. */
#define AK_VENC_MIN_BITSTREAM	(128U * 1024U)

/* One 32-bit NAL length per macroblock row, with room to spare: the core
 * writes this table and an undersized one corrupts what follows it. */
#define AK_VENC_NAL_SPARE	16U

struct ak_venc_buffer {
	struct v4l2_m2m_buffer	m2m;
	void __iomem		*map;
};

static const struct ak_venc_fmt ak_venc_formats[] = {
	{
		.name	= "YUV 4:2:0 planar",
		.fourcc	= V4L2_PIX_FMT_YUV420,
		.depth	= 12,
		.types	= AK_VENC_OUTPUT,
	}, {
		.name	= "H.264 Annex B",
		.fourcc	= V4L2_PIX_FMT_H264,
		.depth	= 0,
		.types	= AK_VENC_CAPTURE,
	}, {
		/*
		 * One whole JFIF picture per buffer. The core is time-shared:
		 * a JPEG job carries no state, so it can be run between two
		 * frames of an H.264 session without disturbing it.
		 */
		.name	= "JFIF JPEG",
		.fourcc	= V4L2_PIX_FMT_JPEG,
		.depth	= 0,
		.types	= AK_VENC_CAPTURE,
	},
};

static bool ak_venc_cap_is_jpeg(const struct ak_venc_ctx *ctx)
{
	return ctx->cap_q.fmt && ctx->cap_q.fmt->fourcc == V4L2_PIX_FMT_JPEG;
}

static struct ak_venc_ctx *fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct ak_venc_ctx, fh);
}

static struct ak_venc_buffer *to_ak_buf(struct vb2_buffer *vb)
{
	return ak_venc_vb_to_buf(vb, struct ak_venc_buffer);
}

static const struct ak_venc_fmt *find_format(u32 fourcc, u8 type)
{
	unsigned i;

	for (i = 0; i < ARRAY_SIZE(ak_venc_formats); i++)
		if (ak_venc_formats[i].fourcc == fourcc &&
		    (ak_venc_formats[i].types & type))
			return &ak_venc_formats[i];
	return NULL;
}

static struct ak_venc_q_data *get_q_data(struct ak_venc_ctx *ctx,
					 enum v4l2_buf_type type)
{
	if (V4L2_TYPE_IS_OUTPUT(type))
		return &ctx->out_q;
	return &ctx->cap_q;
}

/* -------------------------------------------------------------------------
 * Hardware
 * ------------------------------------------------------------------------- */

/*
 * Retires a posted write to the quantiser bank. Reading the register back is
 * the only thing that makes a burst of writes to 0x100..0x17c land at the
 * addresses they were given rather than all at the first one.
 */
static u32 ak_venc_reg_retire(void *context, unsigned off)
{
	struct ak_venc_dev *dev = context;

	if (off > AK_VENC_REG_LAST_WRITE)
		return 0;
	return readl(dev->regs + off);
}

static void ak_venc_reg_write(void *context, unsigned off, u32 value)
{
	struct ak_venc_dev *dev = context;

	/* 0x200..0x3fc mirrors swreg0..127; a write there lands on the IRQ,
	 * AXI and address registers. */
	if (off > AK_VENC_REG_LAST_WRITE)
		return;
	writel(value, dev->regs + off);
}

static u32 ak_venc_reg_read(struct ak_venc_dev *dev, unsigned reg)
{
	return readl(dev->regs + reg * 4);
}

static DEFINE_SPINLOCK(ak_venc_sysctrl_lock);

static void ak_venc_sysctrl_set(unsigned long reg_phys, u32 mask, u32 value)
{
	void __iomem *reg = (void __iomem *)(AK_VA_SYSCTRL +
					     (reg_phys - AK_PA_SYSCTRL));
	unsigned long flags;
	u32 v;

	spin_lock_irqsave(&ak_venc_sysctrl_lock, flags);
	v = __raw_readl(reg);
	v = (v & ~mask) | (value & mask);
	__raw_writel(v, reg);
	spin_unlock_irqrestore(&ak_venc_sysctrl_lock, flags);
}

static void ak_venc_hw_on(struct ak_venc_dev *dev)
{
	ak_venc_sysctrl_set(AK_VENC_CLK_GATE_REG, AK_VENC_VIDEO_BIT,
			    AK_VENC_VIDEO_BIT);
	ak_venc_sysctrl_set(AK_VENC_SOFT_RESET_REG, AK_VENC_VIDEO_BIT,
			    AK_VENC_VIDEO_BIT);
	ak_venc_sysctrl_set(AK_VENC_CLK_GATE_REG, AK_VENC_VIDEO_BIT, 0);
	ak_venc_sysctrl_set(AK_VENC_SOFT_RESET_REG, AK_VENC_VIDEO_BIT, 0);
}

static void ak_venc_hw_off(struct ak_venc_dev *dev)
{
	ak_venc_sysctrl_set(AK_VENC_CLK_GATE_REG, AK_VENC_VIDEO_BIT,
			    AK_VENC_VIDEO_BIT);
}

static void ak_venc_stop_core(struct ak_venc_dev *dev)
{
	u32 v = ak_venc_reg_read(dev, AKCAM_H8290_ENABLE_REG);

	writel(v & ~1u, dev->regs + AKCAM_H8290_ENABLE_REG * 4);
}

/* -------------------------------------------------------------------------
 * Per-context DMA: the two frame stores, the NAL length table and the CABAC
 * context tables, in one coherent allocation.
 * ------------------------------------------------------------------------- */

static int ak_venc_alloc_priv(struct ak_venc_ctx *ctx)
{
	struct ak_venc_q_data *q = &ctx->out_q;
	size_t frames, nal;

	/*
	 * A JPEG context needs none of this. The vendor's allocator, asked for
	 * JPEG, returns without allocating a reference frame, a reconstruction
	 * frame, a NAL length table or a CABAC block, and the frame setup then
	 * leaves all four frame-store address registers at zero.
	 */
	if (ak_venc_cap_is_jpeg(ctx))
		return ctx->priv_cpu ? -EINVAL : 0;

	frames = akcam_h8290_frames_bytes(q->width, q->height);
	if (!frames)
		return -EINVAL;

	nal = (ALIGN(q->height, 16) / 16 + AK_VENC_NAL_SPARE) * sizeof(u32);
	nal = ALIGN(nal, 8);
	if (ctx->priv_cpu)
		return ctx->priv_width == q->width &&
		       ctx->priv_height == q->height &&
		       ctx->priv_size == frames + nal + AK_VENC_CABAC_BYTES ?
		       0 : -EINVAL;

	ctx->nal_off = frames;
	ctx->nal_bytes = nal;
	ctx->cabac_off = frames + nal;
	ctx->priv_size = frames + nal + AK_VENC_CABAC_BYTES;

	ctx->priv_cpu = dma_alloc_coherent(ctx->dev->dev, ctx->priv_size,
					   &ctx->priv_dma, GFP_KERNEL);
	if (!ctx->priv_cpu)
		return -ENOMEM;
	ctx->priv_width = q->width;
	ctx->priv_height = q->height;

	memset(ctx->priv_cpu, 0, ctx->priv_size);
	akcam_h8290_frames_init(&ctx->frames, (u32)ctx->priv_dma,
				q->width, q->height);
	return 0;
}

static void ak_venc_free_priv(struct ak_venc_ctx *ctx)
{
	if (!ctx->priv_cpu)
		return;
	dma_free_coherent(ctx->dev->dev, ctx->priv_size, ctx->priv_cpu,
			  ctx->priv_dma);
	ctx->priv_cpu = NULL;
	ctx->priv_size = 0;
	ctx->priv_width = ctx->priv_height = 0;
}

/* -------------------------------------------------------------------------
 * Event counters
 * ------------------------------------------------------------------------- */

static bool ak_venc_note(struct ak_venc_dev *dev, enum ak_venc_event ev)
{
	dev->ev_stats.count[ev]++;

	if (dev->ev_reported & BIT(ev))
		return false;

	dev->ev_reported |= BIT(ev);
	return true;
}

static const char * const ak_venc_event_name[AK_VENC_EVENTS] = {
	[AK_VENC_EV_BUFFER_FULL]	= "buffer_full",
	[AK_VENC_EV_BUS_ERROR]		= "bus_error",
	[AK_VENC_EV_SW_RESET]		= "sw_reset",
	[AK_VENC_EV_PROGRAM_FAIL]	= "program_fail",
};

static ssize_t venc_stats_show(struct device *d, struct device_attribute *attr,
			       char *buf)
{
	struct ak_venc_dev *dev = dev_get_drvdata(d);
	struct ak_venc_event_stats stats;
	unsigned long flags;
	ssize_t len = 0;
	int i;

	if (!dev)
		return -ENODEV;

	spin_lock_irqsave(&dev->irqlock, flags);
	stats = dev->ev_stats;
	spin_unlock_irqrestore(&dev->irqlock, flags);

	for (i = 0; i < AK_VENC_EVENTS; i++)
		len += scnprintf(buf + len, PAGE_SIZE - len, "%s %u\n",
				 ak_venc_event_name[i], stats.count[i]);

	return len;
}
static DEVICE_ATTR_RO(venc_stats);

/* -------------------------------------------------------------------------
 * v4l2_m2m_ops
 * ------------------------------------------------------------------------- */

static void ak_venc_job_done(struct ak_venc_ctx *ctx,
			     enum vb2_buffer_state state)
{
	struct ak_venc_dev *dev = ctx->dev;
	struct vb2_buffer *src, *dst;
	unsigned long flags;

	src = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	dst = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

	spin_lock_irqsave(&dev->irqlock, flags);
	if (src)
		ak_venc_m2m_buf_done(src, state);
	if (dst)
		ak_venc_m2m_buf_done(dst, state);
	spin_unlock_irqrestore(&dev->irqlock, flags);

	v4l2_m2m_job_finish(dev->m2m_dev, ctx->m2m_ctx);
}

static void ak_venc_build_sps(struct ak_venc_ctx *ctx,
			      struct akcam_h264_sps *sps)
{
	akcam_h264_sps_defaults(sps);
	sps->level_idc = ctx->level_idc;
	sps->constraint_set1 =
		ctx->profile == V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE ? 0 : 1;

	/* frame rate = time_scale / (2 * num_units_in_tick) */
	if (ctx->parm_set) {
		sps->vui.num_units_in_tick = ctx->rc_cfg.fps_den;
		sps->vui.time_scale = ctx->rc_cfg.fps_num * 2;
		sps->vui.fixed_frame_rate = 1;
	}
}

static int ak_venc_write_headers(struct ak_venc_ctx *ctx,
				 struct vb2_buffer *dst, unsigned qp,
				 u32 *msb, u32 *lsb, unsigned *rem)
{
	struct ak_venc_buffer *dbuf = to_ak_buf(dst);
	struct akcam_h264_sps sps;
	struct akcam_h264_pps pps;
	u8 hdr[AK_VENC_HDR_MAX];

	ak_venc_build_sps(ctx, &sps);
	if (akcam_h264_sps_geometry(&sps, ctx->out_q.width, ctx->out_q.height))
		return -EINVAL;
	akcam_h264_pps_defaults(&pps);
	pps.pic_init_qp_minus26 = (int)qp - 26;

	ctx->hdr_bytes = ak_venc_headers(hdr, sizeof(hdr), &sps, &pps);
	if (!ctx->hdr_bytes)
		return -EINVAL;
	if (!dbuf->map)
		return -EINVAL;
	if (vb2_plane_size(dst, 0) < ctx->hdr_bytes + 16)
		return -EINVAL;

	memcpy_toio(dbuf->map, hdr, ctx->hdr_bytes);
	*rem = ak_venc_hdr_remainder(hdr, ctx->hdr_bytes, msb, lsb);
	ctx->pps_qp = qp;
	return 0;
}

/*
 * One JPEG picture.
 *
 * Nothing here touches the H.264 session: no rate control, no GOP, no frame
 * number, no reference rotation, and the register file is rebuilt from zero so
 * no H.264 field survives into it. That is what makes a snapshot something that
 * can be dropped between two frames of a running stream.
 *
 * Software writes SOI..SOS into the head of the CAPTURE buffer and the core
 * continues from the last whole 64-bit word of it; the core writes the scan and
 * the EOI. So the finished file is the buffer from offset zero, and the payload
 * is the header's whole words plus what the core reports.
 */
static int ak_venc_program_jpeg(struct ak_venc_ctx *ctx, struct vb2_buffer *src,
				struct vb2_buffer *dst)
{
	struct ak_venc_buffer *dbuf = to_ak_buf(dst);
	struct akcam_h8290_jpeg cfg;
	struct akcam_jpeg_hdr hc;
	dma_addr_t src_dma, dst_dma;
	size_t dst_size;

	src_dma = vb2_dma_contig_plane_dma_addr(src, 0);
	dst_dma = vb2_dma_contig_plane_dma_addr(dst, 0);
	dst_size = vb2_plane_size(dst, 0);
	if (!src_dma || !dst_dma || (dst_dma & 7))
		return -EINVAL;
	if (!dbuf->map)
		return -EINVAL;

	akcam_jpeg_quant_tables(ctx->jpeg_luma, ctx->jpeg_chroma,
				ctx->jpeg_quality);

	memset(&hc, 0, sizeof(hc));
	hc.width = ctx->out_q.width;
	hc.height = ctx->out_q.height;
	hc.luma = ctx->jpeg_luma;
	hc.chroma = ctx->jpeg_chroma;

	ctx->hdr_bytes = akcam_jpeg_write_header(ctx->jpeg_hdr,
						 sizeof(ctx->jpeg_hdr), &hc);
	if (!ctx->hdr_bytes)
		return -EINVAL;
	if (dst_size < ctx->hdr_bytes + 16)
		return -EINVAL;
	memcpy_toio(dbuf->map, ctx->jpeg_hdr, ctx->hdr_bytes);

	memset(&ctx->regs, 0, sizeof(ctx->regs));

	memset(&cfg, 0, sizeof(cfg));
	cfg.width = ctx->out_q.width;
	cfg.height = ctx->out_q.height;
	cfg.input_row_length = ctx->out_q.bytesperline;
	cfg.restart_mb_rows = 0;

	if (akcam_h8290_config_jpeg(&ctx->regs, &cfg))
		return -EINVAL;
	akcam_h8290_quant_apply(&ctx->regs, ctx->jpeg_luma, ctx->jpeg_chroma);
	if (akcam_h8290_input_apply(&ctx->regs, (u32)src_dma,
				    ctx->out_q.bytesperline, ctx->out_q.height))
		return -EINVAL;
	if (akcam_h8290_jpeg_output_apply(&ctx->regs, (u32)dst_dma,
					  dst_size & ~(size_t)7,
					  ctx->jpeg_hdr, ctx->hdr_bytes))
		return -EINVAL;

	ctx->frame_jpeg = true;
	ctx->frame_intra = true;		/* every JPEG stands alone */
	ak_venc_vb_copy_timestamp(dst, src);
	return 0;
}

static int ak_venc_program(struct ak_venc_ctx *ctx, struct vb2_buffer *src,
			   struct vb2_buffer *dst)
{
	struct akcam_h8290_h264 cfg;
	dma_addr_t src_dma, dst_dma;
	size_t dst_size, base_off, stream_bytes;
	u32 msb = 0, lsb = 0;
	unsigned rem = 0;
	int intra;

	if (ak_venc_cap_is_jpeg(ctx))
		return ak_venc_program_jpeg(ctx, src, dst);

	ctx->frame_jpeg = false;

	src_dma = vb2_dma_contig_plane_dma_addr(src, 0);
	dst_dma = vb2_dma_contig_plane_dma_addr(dst, 0);
	dst_size = vb2_plane_size(dst, 0);
	if (!src_dma || !dst_dma || (dst_dma & 7))
		return -EINVAL;

	if (ctx->rc_dirty) {
		if (ak_venc_rc_reconfig(&ctx->rc, &ctx->rc_cfg))
			return -EINVAL;
		ctx->rc_dirty = false;
	}

	intra = ak_venc_gop_is_intra(&ctx->gop) || ak_venc_gop_forced(&ctx->gop);
	ak_venc_rc_before(&ctx->rc, intra, &ctx->rc_frame);

	/* SPS and PPS lead every IDR; the core writes the slice NAL's own
	 * start code, so an inter frame has nothing to prepend. */
	ctx->hdr_bytes = 0;
	if (intra) {
		int ret = ak_venc_write_headers(ctx, dst, ctx->rc_frame.qp,
						&msb, &lsb, &rem);
		if (ret)
			return ret;
	}

	/* The core resumes inside the last 64-bit word software wrote: the
	 * stream base is that word, and swreg22/23/37 carry its used bits. */
	base_off = ctx->hdr_bytes & ~(size_t)7;
	stream_bytes = (dst_size - base_off) & ~(size_t)7;

	akcam_h8290_h264_defaults(&cfg);
	cfg.width = ctx->out_q.width;
	cfg.height = ctx->out_q.height;
	cfg.input_row_length = ctx->out_q.bytesperline;
	cfg.intra = intra;
	cfg.pps_qp = ctx->pps_qp;
	cfg.initial_qp = ctx->rc_frame.qp;
	cfg.qp_min = ctx->rc_frame.qp_min;
	cfg.qp_max = ctx->rc_frame.qp_max;
	cfg.frame_num = intra ? 0 : ctx->frame_num;
	cfg.idr_pic_id = ctx->idr_pic_id;
	cfg.mad_qp_delta = ctx->rc_frame.mad_qp_delta;
	cfg.mad_threshold = ctx->rc_frame.mad_threshold;
	cfg.checkpoint_distance = ctx->rc_frame.checkpoint_distance;
	cfg.checkpoint_target = ctx->rc_frame.checkpoint_target;
	cfg.checkpoint_error = ctx->rc_frame.checkpoint_error;
	cfg.checkpoint_delta_qp = ctx->rc_frame.checkpoint_delta_qp;

	if (akcam_h8290_config_h264(&ctx->regs, &cfg))
		return -EINVAL;
	if (akcam_h8290_input_apply(&ctx->regs, (u32)src_dma,
				    ctx->out_q.bytesperline, ctx->out_q.height))
		return -EINVAL;
	if (akcam_h8290_output_apply(&ctx->regs, (u32)dst_dma + base_off,
				     stream_bytes,
				     (u32)ctx->priv_dma + ctx->nal_off,
				     (u32)ctx->priv_dma + ctx->cabac_off))
		return -EINVAL;

	akcam_h8290_set(&ctx->regs, H8290_STREAM_HEADER_REMAINDER_BITS_MSB_MSB_ALIGNED,
			msb);
	akcam_h8290_set(&ctx->regs, H8290_STREAM_HEADER_REMAINDER_BITS_LSB_MSB_ALIGNED,
			lsb);
	akcam_h8290_set(&ctx->regs, H8290_STREAM_START_OFFSET_AMOUNT_OF_STRMHDRREM_BITS_0,
			rem);

	akcam_h8290_frames_apply(&ctx->frames, &ctx->regs);

	ctx->frame_intra = intra;
	if (intra)
		ak_venc_gop_forced_done(&ctx->gop);
	ak_venc_vb_copy_timestamp(dst, src);
	return 0;
}

static void ak_venc_program_error(struct work_struct *work)
{
	struct ak_venc_ctx *ctx = container_of(work, struct ak_venc_ctx,
					     program_error_work);
	struct ak_venc_dev *dev = ctx->dev;

	disable_irq(dev->irq);
	if (v4l2_m2m_get_curr_priv(dev->m2m_dev) == ctx)
		ak_venc_job_done(ctx, VB2_BUF_STATE_ERROR);
	enable_irq(dev->irq);
}

static void ak_venc_device_run(void *priv)
{
	struct ak_venc_ctx *ctx = priv;
	struct ak_venc_dev *dev = ctx->dev;
	struct vb2_buffer *src, *dst;

	src = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	dst = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
	if (!src || !dst) {
		schedule_work(&ctx->program_error_work);
		return;
	}

	if (ak_venc_program(ctx, src, dst)) {
		if (ak_venc_note(dev, AK_VENC_EV_PROGRAM_FAIL))
			v4l2_err(&dev->v4l2_dev,
				 "cannot program a frame; counted in venc_stats from here on\n");
		schedule_work(&ctx->program_error_work);
		return;
	}

	/* write_frame writes swreg14 last; that write starts the core. */
	akcam_h8290_write_frame(&ctx->regs, ak_venc_reg_write,
				ak_venc_reg_retire, dev);
}

static int ak_venc_job_ready(void *priv)
{
	struct ak_venc_ctx *ctx = priv;

	/* The framework has already checked that both queues have a buffer.
	 * v4l2_m2m_num_src_bufs_ready() and its destination counterpart read
	 * each other's queue context on this tree, so they are not used. */
	return !ctx->aborting;
}

static void ak_venc_job_abort(void *priv)
{
	struct ak_venc_ctx *ctx = priv;
	struct ak_venc_dev *dev = ctx->dev;
	u32 status;

	ctx->aborting = true;
	cancel_work_sync(&ctx->program_error_work);
	/* Completion can advance curr_ctx after mem2mem decides to abort. */
	disable_irq(dev->irq);
	if (v4l2_m2m_get_curr_priv(dev->m2m_dev) == ctx) {
		ak_venc_stop_core(dev);
		status = ak_venc_reg_read(dev, AKCAM_H8290_STATUS_REG);
		writel(status & ~AK_VENC_IRQ_LINE,
		       dev->regs + AKCAM_H8290_STATUS_REG * 4);
		ak_venc_gop_restart(&ctx->gop);
		ak_venc_job_done(ctx, VB2_BUF_STATE_ERROR);
	}
	enable_irq(dev->irq);
}

static void ak_venc_lock(void *priv)
{
	struct ak_venc_ctx *ctx = priv;

	mutex_lock(&ctx->dev->dev_mutex);
}

static void ak_venc_unlock(void *priv)
{
	struct ak_venc_ctx *ctx = priv;

	mutex_unlock(&ctx->dev->dev_mutex);
}

static struct v4l2_m2m_ops ak_venc_m2m_ops = {
	.device_run	= ak_venc_device_run,
	.job_ready	= ak_venc_job_ready,
	.job_abort	= ak_venc_job_abort,
	.lock		= ak_venc_lock,
	.unlock		= ak_venc_unlock,
};

static u32 ak_venc_read_field(struct ak_venc_dev *dev, unsigned field)
{
	int reg = akcam_h8290_reg_of(field);

	if (reg < 0)
		return 0;
	return akcam_h8290_extract(field, ak_venc_reg_read(dev, reg));
}

/*
 * The five per-frame outputs, read before anything can start another frame:
 * the size in bits, the QP sum, the codeword count and the two MB counts.
 */
static void ak_venc_read_result(struct ak_venc_dev *dev,
				struct ak_venc_rc_result *r)
{
	r->bits = ak_venc_read_field(dev,
		H8290_STREAM_BUFFER_LIMIT_64BIT_ADDRESSES_OUTPUT_STREAM_SIZE_B);
	r->qp_sum_div2 = ak_venc_read_field(dev, H8290_QP_SUM_DIV2_OUTPUT);
	r->rlc_div4 = ak_venc_read_field(dev, H8290_RLC_CODEWORD_COUNT_DIV4_OUTPUT);
	r->mb_count = ak_venc_read_field(dev, H8290_MB_COUNT_OUTPUT);
	r->mad_under = ak_venc_read_field(dev,
		H8290_MACROBLOCK_COUNT_WITH_MAD_VALUE_UNDER_THRESHOLD_OUTPUT);
}

static irqreturn_t ak_venc_irq(int irq, void *data)
{
	struct ak_venc_dev *dev = data;
	struct ak_venc_ctx *ctx;
	struct ak_venc_rc_result res;
	enum akcam_h8290_status status;
	enum vb2_buffer_state state = VB2_BUF_STATE_ERROR;
	struct vb2_buffer *dst;
	u32 swreg1;
	size_t bytes;

	swreg1 = ak_venc_reg_read(dev, AKCAM_H8290_STATUS_REG);
	if (!(swreg1 & AK_VENC_IRQ_LINE))
		return IRQ_NONE;

	/* Ack by dropping bit 0 only: the rest of swreg1 is live encoder state
	 * and writing back the masked value would clear it. */
	writel(swreg1 & ~AK_VENC_IRQ_LINE,
	       dev->regs + AKCAM_H8290_STATUS_REG * 4);

	status = akcam_h8290_status(swreg1);
	if (status == AKCAM_H8290_RUNNING)
		return IRQ_HANDLED;

	ctx = v4l2_m2m_get_curr_priv(dev->m2m_dev);
	if (!ctx)
		return IRQ_HANDLED;

	dst = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);

	switch (status) {
	case AKCAM_H8290_FRAME_READY:
		ak_venc_read_result(dev, &res);
		bytes = akcam_h8290_output_bytes(res.bits);
		if (dst && bytes) {
			/* The core resumed inside the last whole 64-bit word
			 * software wrote, and counts from there. */
			bytes += ctx->hdr_bytes & ~(size_t)7;
			if (bytes > vb2_plane_size(dst, 0))
				bytes = vb2_plane_size(dst, 0);
			vb2_set_plane_payload(dst, 0, bytes);
			ak_venc_vb_set_frame_type(dst, ctx->frame_intra);
			state = VB2_BUF_STATE_DONE;
		}

		/*
		 * A JPEG carries no state: no rate control to feed, no
		 * reference to rotate, no GOP position and no frame number. The
		 * H.264 session's bookkeeping is left exactly as it was, which
		 * is what lets a snapshot be taken mid-stream.
		 */
		if (ctx->frame_jpeg)
			break;

		ak_venc_rc_after(&ctx->rc, ctx->frame_intra, &res);

		/* Only a completed picture advances the reference rotation and
		 * the counters the slice header is built from. */
		akcam_h8290_frames_advance(&ctx->frames);
		ak_venc_gop_advance(&ctx->gop, ctx->frame_intra);
		if (ctx->frame_intra) {
			ctx->frame_num = 1;
			ctx->idr_pic_id = (ctx->idr_pic_id + 1) & 15;
		} else {
			ctx->frame_num = (ctx->frame_num + 1) & 0xffff;
		}
		break;
	/*
	 * A failed frame leaves the reference stores holding whatever the core
	 * got to, so the next H.264 frame has to be intra. A failed JPEG says
	 * nothing about the H.264 session, so it does not force one.
	 */
	case AKCAM_H8290_BUFFER_FULL:
		if (ak_venc_note(dev, AK_VENC_EV_BUFFER_FULL))
			v4l2_err(&dev->v4l2_dev,
				 "output buffer full; counted in venc_stats from here on\n");
		ak_venc_stop_core(dev);
		if (!ctx->frame_jpeg)
			ak_venc_gop_restart(&ctx->gop);
		break;
	case AKCAM_H8290_BUS_ERROR:
		if (ak_venc_note(dev, AK_VENC_EV_BUS_ERROR))
			v4l2_err(&dev->v4l2_dev,
				 "bus error or timeout; counted in venc_stats from here on\n");
		ak_venc_stop_core(dev);
		if (!ctx->frame_jpeg)
			ak_venc_gop_restart(&ctx->gop);
		break;
	case AKCAM_H8290_SW_RESET:
		if (ak_venc_note(dev, AK_VENC_EV_SW_RESET))
			v4l2_err(&dev->v4l2_dev,
				 "core reset itself; counted in venc_stats from here on\n");
		if (!ctx->frame_jpeg)
			ak_venc_gop_restart(&ctx->gop);
		break;
	default:
		break;
	}

	if (dst && state != VB2_BUF_STATE_DONE)
		vb2_set_plane_payload(dst, 0, 0);

	ak_venc_job_done(ctx, state);
	return IRQ_HANDLED;
}

/* -------------------------------------------------------------------------
 * videobuf2
 * ------------------------------------------------------------------------- */

static int ak_venc_queue_setup(struct vb2_queue *vq,
			       AK_VENC_QUEUE_SETUP_ARG *fmt,
			       unsigned int *nbuffers, unsigned int *nplanes,
			       unsigned int sizes[], void *alloc_ctxs[])
{
	struct ak_venc_ctx *ctx = vb2_get_drv_priv(vq);
	struct ak_venc_q_data *q = get_q_data(ctx, vq->type);

	if (*nbuffers < 1)
		*nbuffers = 1;

	*nplanes = 1;
	sizes[0] = q->sizeimage;
	alloc_ctxs[0] = ctx->dev->alloc_ctx;
	return 0;
}

static void ak_venc_unmap_buf(struct ak_venc_buffer *buf)
{
	buf->map = NULL;
}

/* CAPTURE headers require a coherent CPU mapping throughout device ownership. */
static int ak_venc_map_capture(struct ak_venc_ctx *ctx, struct vb2_buffer *vb)
{
	struct ak_venc_buffer *buf = to_ak_buf(vb);
	void *vaddr;

	ak_venc_unmap_buf(buf);

	vaddr = vb2_plane_vaddr(vb, 0);
	if (vaddr) {
		buf->map = (void __iomem __force *)vaddr;
		return 0;
	}

	return -EINVAL;
}

static int ak_venc_buf_prepare(struct vb2_buffer *vb)
{
	struct ak_venc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct ak_venc_q_data *q = get_q_data(ctx, vb->vb2_queue->type);

	if (vb2_plane_size(vb, 0) < q->sizeimage)
		return -EINVAL;

	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		vb2_set_plane_payload(vb, 0, q->sizeimage);
		return 0;
	}

	return ak_venc_map_capture(ctx, vb);
}

static AK_VENC_BUF_FINISH_RET ak_venc_buf_finish(struct vb2_buffer *vb)
{
	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type))
		ak_venc_unmap_buf(to_ak_buf(vb));
	ak_venc_buf_finish_done();
}

static void ak_venc_buf_cleanup(struct vb2_buffer *vb)
{
	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type))
		ak_venc_unmap_buf(to_ak_buf(vb));
}

static void ak_venc_buf_queue(struct vb2_buffer *vb)
{
	struct ak_venc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	ak_venc_m2m_buf_queue(ctx->m2m_ctx, vb);
}

static void ak_venc_return_queued(struct ak_venc_ctx *ctx,
				  struct vb2_queue *vq,
				  enum vb2_buffer_state state);

static int ak_venc_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct ak_venc_ctx *ctx = vb2_get_drv_priv(vq);
	struct ak_venc_dev *dev = ctx->dev;
	int ret;

	ret = ak_venc_alloc_priv(ctx);
	if (ret)
		goto fail;

	ctx->rc_cfg.width = ctx->out_q.width;
	ctx->rc_cfg.height = ctx->out_q.height;
	ret = ak_venc_rc_reset(&ctx->rc, &ctx->rc_cfg);
	if (ret) {
		ret = -EINVAL;
		goto fail;
	}
	ctx->rc_dirty = false;

	/* The first streaming queue owns the IRQ until the last queue stops. */
	if (dev->streaming_queues == 0) {
		memset(&dev->ev_stats, 0, sizeof(dev->ev_stats));
		dev->ev_reported = 0;

		ret = request_irq(dev->irq, ak_venc_irq, 0, AK_VENC_NAME, dev);
		if (ret) {
			v4l2_err(&dev->v4l2_dev, "cannot take irq %d\n",
				 dev->irq);
			goto fail;
		}
		ak_venc_hw_on(dev);
	}
	dev->streaming_queues++;

	if (!ctx->streaming) {
		ctx->frame_num = 0;
		ctx->idr_pic_id = 0;
		ak_venc_gop_restart(&ctx->gop);
	}
	ctx->aborting = false;
	ctx->streaming++;

	return 0;

fail:
	ak_venc_return_queued(ctx, vq, VB2_BUF_STATE_QUEUED);
	if (!ctx->streaming)
		ak_venc_free_priv(ctx);
	return ret;
}

/*
 * v4l2_m2m_streamoff() calls vb2_streamoff() before it drops the ready queue,
 * so anything still sitting on that queue is still owned by the driver when
 * __vb2_queue_cancel() counts. Give every one of them back here.
 */
static void ak_venc_return_queued(struct ak_venc_ctx *ctx,
				  struct vb2_queue *vq,
				  enum vb2_buffer_state state)
{
	struct ak_venc_dev *dev = ctx->dev;
	struct vb2_buffer *vb;
	unsigned long flags;

	for (;;) {
		if (V4L2_TYPE_IS_OUTPUT(vq->type))
			vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		else
			vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
		if (!vb)
			break;

		spin_lock_irqsave(&dev->irqlock, flags);
		ak_venc_m2m_buf_done(vb, state);
		spin_unlock_irqrestore(&dev->irqlock, flags);
	}
}

static AK_VENC_STOP_STREAMING_RET ak_venc_stop_streaming(struct vb2_queue *vq)
{
	struct ak_venc_ctx *ctx = vb2_get_drv_priv(vq);
	struct ak_venc_dev *dev = ctx->dev;

	cancel_work_sync(&ctx->program_error_work);
	ak_venc_return_queued(ctx, vq, VB2_BUF_STATE_ERROR);

	if (ctx->streaming)
		ctx->streaming--;
	if (dev->streaming_queues && --dev->streaming_queues == 0) {
		ak_venc_hw_off(dev);
		free_irq(dev->irq, dev);
	}

	if (!ctx->streaming)
		ak_venc_free_priv(ctx);

	ak_venc_stop_streaming_done();
}

static void ak_venc_wait_prepare(struct vb2_queue *vq)
{
	struct ak_venc_ctx *ctx = vb2_get_drv_priv(vq);

	mutex_unlock(&ctx->dev->dev_mutex);
}

static void ak_venc_wait_finish(struct vb2_queue *vq)
{
	struct ak_venc_ctx *ctx = vb2_get_drv_priv(vq);

	mutex_lock(&ctx->dev->dev_mutex);
}

static struct vb2_ops ak_venc_qops = {
	.queue_setup	 = ak_venc_queue_setup,
	.buf_prepare	 = ak_venc_buf_prepare,
	.buf_finish	 = ak_venc_buf_finish,
	.buf_cleanup	 = ak_venc_buf_cleanup,
	.buf_queue	 = ak_venc_buf_queue,
	.start_streaming = ak_venc_start_streaming,
	.stop_streaming	 = ak_venc_stop_streaming,
	.wait_prepare	 = ak_venc_wait_prepare,
	.wait_finish	 = ak_venc_wait_finish,
};

static int queue_init(void *priv, struct vb2_queue *src_vq,
		      struct vb2_queue *dst_vq)
{
	struct ak_venc_ctx *ctx = priv;
	int ret;

	memset(src_vq, 0, sizeof(*src_vq));
	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	src_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct ak_venc_buffer);
	src_vq->ops = &ak_venc_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	memset(dst_vq, 0, sizeof(*dst_vq));
	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_vq->io_modes = VB2_MMAP;
	dst_vq->drv_priv = ctx;
	dst_vq->buf_struct_size = sizeof(struct ak_venc_buffer);
	dst_vq->ops = &ak_venc_qops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	return vb2_queue_init(dst_vq);
}

/* -------------------------------------------------------------------------
 * ioctls
 * ------------------------------------------------------------------------- */

static int vidioc_querycap(struct file *file, void *priv,
			   struct v4l2_capability *cap)
{
	strlcpy(cap->driver, AK_VENC_NAME, sizeof(cap->driver));
	strlcpy(cap->card, "Anyka 8290 H.264/JPEG encoder", sizeof(cap->card));
	strlcpy(cap->bus_info, "platform:" AK_VENC_NAME, sizeof(cap->bus_info));
	cap->device_caps = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int enum_fmt(struct v4l2_fmtdesc *f, u8 type)
{
	unsigned i, num = 0;

	for (i = 0; i < ARRAY_SIZE(ak_venc_formats); i++) {
		if (!(ak_venc_formats[i].types & type))
			continue;
		if (num++ != f->index)
			continue;
		f->pixelformat = ak_venc_formats[i].fourcc;
		strlcpy(f->description, ak_venc_formats[i].name,
			sizeof(f->description));
		if (type == AK_VENC_CAPTURE)
			f->flags = V4L2_FMT_FLAG_COMPRESSED;
		return 0;
	}
	return -EINVAL;
}

static int vidioc_enum_fmt_vid_cap(struct file *file, void *priv,
				   struct v4l2_fmtdesc *f)
{
	return enum_fmt(f, AK_VENC_CAPTURE);
}

static int vidioc_enum_fmt_vid_out(struct file *file, void *priv,
				   struct v4l2_fmtdesc *f)
{
	return enum_fmt(f, AK_VENC_OUTPUT);
}

static int vidioc_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct ak_venc_ctx *ctx = fh_to_ctx(priv);
	struct ak_venc_q_data *q = get_q_data(ctx, f->type);

	f->fmt.pix.width	= q->width;
	f->fmt.pix.height	= q->height;
	f->fmt.pix.field	= V4L2_FIELD_NONE;
	f->fmt.pix.pixelformat	= q->fmt->fourcc;
	f->fmt.pix.bytesperline	= q->bytesperline;
	f->fmt.pix.sizeimage	= q->sizeimage;
	f->fmt.pix.colorspace	= V4L2_COLORSPACE_SMPTE170M;
	return 0;
}

static int vidioc_try_fmt_vid_out(struct file *file, void *priv,
				  struct v4l2_format *f)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;

	if (!find_format(pix->pixelformat, AK_VENC_OUTPUT))
		pix->pixelformat = V4L2_PIX_FMT_YUV420;

	if (pix->width < AK_VENC_MIN_WIDTH)
		pix->width = AK_VENC_MIN_WIDTH;
	if (pix->width > AK_VENC_MAX_WIDTH)
		pix->width = AK_VENC_MAX_WIDTH;
	if (pix->height < AK_VENC_MIN_HEIGHT)
		pix->height = AK_VENC_MIN_HEIGHT;
	if (pix->height > AK_VENC_MAX_HEIGHT)
		pix->height = AK_VENC_MAX_HEIGHT;

	/*
	 * The width is a whole number of macroblocks; the height is not
	 * rounded up, because the core codes ALIGN(height,16) rows and says so
	 * in the bottom-edge overfill field. 640x360 stays 640x360 here.
	 */
	pix->width &= ~15u;
	pix->height &= ~1u;

	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = pix->width;
	pix->sizeimage = pix->width * pix->height * 3 / 2;
	pix->colorspace = V4L2_COLORSPACE_SMPTE170M;
	return 0;
}

static unsigned ak_venc_bitstream_size(unsigned width, unsigned height)
{
	unsigned size = width * height / 2;

	if (size < AK_VENC_MIN_BITSTREAM)
		size = AK_VENC_MIN_BITSTREAM;
	return ALIGN(size, 8);
}

/*
 * A JPEG is one whole picture in one buffer with no rate control to hold it
 * down, so it is sized against the raw frame rather than against a bitrate: at
 * the top of the quality range a 4:2:0 baseline picture of detailed content can
 * pass half the raw size, which is all an H.264 stream is ever given. The
 * buffer-full interrupt is still the backstop.
 */
static unsigned ak_venc_cap_size(u32 fourcc, unsigned width, unsigned height)
{
	unsigned size;

	if (fourcc != V4L2_PIX_FMT_JPEG)
		return ak_venc_bitstream_size(width, height);

	size = width * height * 3 / 2;
	if (size < AK_VENC_MIN_BITSTREAM)
		size = AK_VENC_MIN_BITSTREAM;
	return ALIGN(size, 8);
}

static int vidioc_try_fmt_vid_cap(struct file *file, void *priv,
				  struct v4l2_format *f)
{
	struct ak_venc_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_pix_format *pix = &f->fmt.pix;
	unsigned min;

	if (!find_format(pix->pixelformat, AK_VENC_CAPTURE))
		pix->pixelformat = V4L2_PIX_FMT_H264;
	pix->width = ctx->out_q.width;
	pix->height = ctx->out_q.height;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = 0;
	pix->colorspace = V4L2_COLORSPACE_SMPTE170M;

	min = ak_venc_cap_size(pix->pixelformat, pix->width, pix->height);
	if (pix->sizeimage < min)
		pix->sizeimage = min;
	else
		pix->sizeimage = ALIGN(pix->sizeimage, 8);
	return 0;
}

static bool ak_venc_queues_busy(struct ak_venc_ctx *ctx)
{
	return ctx->streaming ||
		vb2_is_busy(v4l2_m2m_get_vq(ctx->m2m_ctx,
					 V4L2_BUF_TYPE_VIDEO_OUTPUT)) ||
		vb2_is_busy(v4l2_m2m_get_vq(ctx->m2m_ctx,
					 V4L2_BUF_TYPE_VIDEO_CAPTURE));
}

static int vidioc_s_fmt_vid_out(struct file *file, void *priv,
				struct v4l2_format *f)
{
	struct ak_venc_ctx *ctx = fh_to_ctx(priv);
	int ret;

	if (ak_venc_queues_busy(ctx))
		return -EBUSY;

	ret = vidioc_try_fmt_vid_out(file, priv, f);
	if (ret)
		return ret;

	ctx->out_q.fmt = find_format(f->fmt.pix.pixelformat, AK_VENC_OUTPUT);
	ctx->out_q.width = f->fmt.pix.width;
	ctx->out_q.height = f->fmt.pix.height;
	ctx->out_q.bytesperline = f->fmt.pix.bytesperline;
	ctx->out_q.sizeimage = f->fmt.pix.sizeimage;

	/* The coded size follows the raw one. */
	ctx->cap_q.width = ctx->out_q.width;
	ctx->cap_q.height = ctx->out_q.height;
	{
		unsigned min = ak_venc_cap_size(ctx->cap_q.fmt->fourcc,
						ctx->cap_q.width,
						ctx->cap_q.height);

		if (ctx->cap_q.sizeimage < min)
			ctx->cap_q.sizeimage = min;
	}
	return 0;
}

static int vidioc_s_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_format *f)
{
	struct ak_venc_ctx *ctx = fh_to_ctx(priv);
	int ret;

	if (ak_venc_queues_busy(ctx))
		return -EBUSY;

	ret = vidioc_try_fmt_vid_cap(file, priv, f);
	if (ret)
		return ret;

	ctx->cap_q.fmt = find_format(f->fmt.pix.pixelformat, AK_VENC_CAPTURE);
	ctx->cap_q.width = f->fmt.pix.width;
	ctx->cap_q.height = f->fmt.pix.height;
	ctx->cap_q.bytesperline = 0;
	ctx->cap_q.sizeimage = f->fmt.pix.sizeimage;
	return 0;
}

static int vidioc_reqbufs(struct file *file, void *priv,
			  struct v4l2_requestbuffers *reqbufs)
{
	return v4l2_m2m_reqbufs(file, fh_to_ctx(priv)->m2m_ctx, reqbufs);
}

static int vidioc_querybuf(struct file *file, void *priv,
			   struct v4l2_buffer *buf)
{
	return v4l2_m2m_querybuf(file, fh_to_ctx(priv)->m2m_ctx, buf);
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	return v4l2_m2m_qbuf(file, fh_to_ctx(priv)->m2m_ctx, buf);
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *buf)
{
	return v4l2_m2m_dqbuf(file, fh_to_ctx(priv)->m2m_ctx, buf);
}

static int vidioc_streamon(struct file *file, void *priv,
			   enum v4l2_buf_type type)
{
	return v4l2_m2m_streamon(file, fh_to_ctx(priv)->m2m_ctx, type);
}

static int vidioc_streamoff(struct file *file, void *priv,
			    enum v4l2_buf_type type)
{
	return v4l2_m2m_streamoff(file, fh_to_ctx(priv)->m2m_ctx, type);
}

/*
 * The frame rate is what the bit budget is denominated in, and V4L2 carries it
 * on the OUTPUT queue as a frame interval rather than as a control.
 */
static int vidioc_g_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *a)
{
	struct ak_venc_ctx *ctx = fh_to_ctx(priv);

	if (a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	memset(&a->parm, 0, sizeof(a->parm));
	a->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
	a->parm.output.timeperframe.numerator = ctx->rc_cfg.fps_den;
	a->parm.output.timeperframe.denominator = ctx->rc_cfg.fps_num;
	return 0;
}

static int vidioc_s_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *a)
{
	struct ak_venc_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_fract *tpf = &a->parm.output.timeperframe;

	if (a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;
	if (!tpf->numerator || !tpf->denominator)
		return -EINVAL;

	ctx->rc_cfg.fps_num = tpf->denominator;
	ctx->rc_cfg.fps_den = tpf->numerator;
	ctx->parm_set = true;
	ctx->rc_dirty = true;

	memset(&a->parm, 0, sizeof(a->parm));
	a->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
	a->parm.output.timeperframe.numerator = ctx->rc_cfg.fps_den;
	a->parm.output.timeperframe.denominator = ctx->rc_cfg.fps_num;
	return 0;
}

static const struct v4l2_ioctl_ops ak_venc_ioctl_ops = {
	.vidioc_querycap		= vidioc_querycap,

	.vidioc_enum_fmt_vid_cap	= vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= vidioc_g_fmt,
	.vidioc_try_fmt_vid_cap		= vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= vidioc_s_fmt_vid_cap,

	.vidioc_enum_fmt_vid_out	= vidioc_enum_fmt_vid_out,
	.vidioc_g_fmt_vid_out		= vidioc_g_fmt,
	.vidioc_try_fmt_vid_out		= vidioc_try_fmt_vid_out,
	.vidioc_s_fmt_vid_out		= vidioc_s_fmt_vid_out,

	.vidioc_reqbufs			= vidioc_reqbufs,
	.vidioc_querybuf		= vidioc_querybuf,
	.vidioc_qbuf			= vidioc_qbuf,
	.vidioc_dqbuf			= vidioc_dqbuf,

	.vidioc_streamon		= vidioc_streamon,
	.vidioc_streamoff		= vidioc_streamoff,

	.vidioc_g_parm			= vidioc_g_parm,
	.vidioc_s_parm			= vidioc_s_parm,
};

/* -------------------------------------------------------------------------
 * controls
 * ------------------------------------------------------------------------- */

static const char * const ak_venc_mb_rc_mode_menu[] = {
	"MAD",
	"Checkpoint",
	NULL,
};

static unsigned ak_venc_level_idc(unsigned menu)
{
	static const u8 idc[] = {
		10, 11, 11, 12, 13, 20, 21, 22, 30, 31, 32, 40, 41, 42, 50, 51
	};

	if (menu >= ARRAY_SIZE(idc))
		return 30;
	return idc[menu];
}

static void ak_venc_apply_mb_rc(struct ak_venc_ctx *ctx)
{
	if (!ctx->mb_rc_enable)
		ctx->rc_cfg.mb_rc = AK_VENC_MB_RC_OFF;
	else if (ctx->mb_rc_mode == AK_VENC_MB_RC_MODE_CHECKPOINT)
		ctx->rc_cfg.mb_rc = AK_VENC_MB_RC_CHECKPOINT;
	else
		ctx->rc_cfg.mb_rc = AK_VENC_MB_RC_MAD;
}

static int ak_venc_try_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ak_venc_ctx *ctx =
		container_of(ctrl->handler, struct ak_venc_ctx, ctrl_handler);

	if (ctrl->id == V4L2_CID_MPEG_VIDEO_H264_MIN_QP &&
	    ctx->qp_limits[0]->val > ctx->qp_limits[1]->val)
		return -ERANGE;
	return 0;
}

static int ak_venc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ak_venc_ctx *ctx =
		container_of(ctrl->handler, struct ak_venc_ctx, ctrl_handler);
	unsigned long flags;

	switch (ctrl->id) {
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
		ctx->rc_cfg.fixed_i_qp = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP:
		ctx->rc_cfg.fixed_p_qp = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
		if (ctx->qp_limits[0]->val > ctx->qp_limits[1]->val)
			return -ERANGE;
		spin_lock_irqsave(&ctx->dev->irqlock, flags);
		ctx->rc_cfg.qp_min = ctx->qp_limits[0]->val;
		ctx->rc_cfg.qp_max = ctx->qp_limits[1]->val;
		ctx->rc_dirty = true;
		spin_unlock_irqrestore(&ctx->dev->irqlock, flags);
		return 0;
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		ctx->rc_cfg.bitrate = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE_MODE:
		ctx->rc_cfg.cbr = ctrl->val == V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;
		break;
	case V4L2_CID_MPEG_VIDEO_GOP_SIZE:
		ctx->rc_cfg.gop_len = ctrl->val;
		ctx->gop.gop = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
		ctx->rc_cfg.frame_rc = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE:
		ctx->mb_rc_enable = ctrl->val;
		ak_venc_apply_mb_rc(ctx);
		break;
	case V4L2_CID_MPEG_VIDEO_VBV_SIZE:
		/* kilobytes of bitstream, and the bucket counts bits. */
		ctx->rc_cfg.bucket_size = (u32)ctrl->val * 8192;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
		ctx->profile = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LEVEL:
		ctx->level_idc = ak_venc_level_idc(ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME:
		ak_venc_gop_request_intra(&ctx->gop);
		return 0;
	case AK_VENC_CID_INTRA_QP_DELTA:
		ctx->rc_cfg.intra_qp_delta = ctrl->val;
		break;
	case AK_VENC_CID_I_WEIGHT:
		ctx->rc_cfg.i_weight = ctrl->val;
		break;
	case AK_VENC_CID_MB_RC_MODE:
		ctx->mb_rc_mode = ctrl->val;
		ak_venc_apply_mb_rc(ctx);
		break;
	case AK_VENC_CID_MAD_QP_DELTA:
		ctx->rc_cfg.mad_qp_delta = ctrl->val;
		break;
	case AK_VENC_CID_MAD_THRESHOLD:
		ctx->rc_cfg.mad_threshold = ctrl->val;
		break;
	case AK_VENC_CID_MAD_ADAPT:
		ctx->rc_cfg.mad_adapt = ctrl->val;
		break;
	case V4L2_CID_JPEG_COMPRESSION_QUALITY:
		/*
		 * JPEG's only knob, and it is not rate control: it scales the
		 * quantisation tables and the same numbers go into the DQT, so
		 * nothing about the H.264 configuration changes and rc_dirty
		 * would be a lie. Returned early for that reason.
		 */
		ctx->jpeg_quality = ctrl->val;
		return 0;
	default:
		return -EINVAL;
	}

	ctx->rc_dirty = true;
	return 0;
}

static const struct v4l2_ctrl_ops ak_venc_ctrl_ops = {
	.try_ctrl = ak_venc_try_ctrl,
	.s_ctrl = ak_venc_s_ctrl,
};

static const struct v4l2_ctrl_config ak_venc_custom_ctrls[] = {
	{
		.ops	= &ak_venc_ctrl_ops,
		.id	= V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME,
		.name	= "Force Key Frame",
		.type	= V4L2_CTRL_TYPE_BUTTON,
		.flags	= V4L2_CTRL_FLAG_WRITE_ONLY |
			  V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
	}, {
		.ops	= &ak_venc_ctrl_ops,
		.id	= AK_VENC_CID_INTRA_QP_DELTA,
		.name	= "H264 Intra QP Delta",
		.type	= V4L2_CTRL_TYPE_INTEGER,
		.min	= -12,
		.max	= 12,
		.step	= 1,
		.def	= -4,
	}, {
		.ops	= &ak_venc_ctrl_ops,
		.id	= AK_VENC_CID_I_WEIGHT,
		.name	= "H264 I Frame Budget Weight",
		.type	= V4L2_CTRL_TYPE_INTEGER,
		.min	= 1,
		.max	= 32,
		.step	= 1,
		.def	= 6,
	}, {
		.ops	= &ak_venc_ctrl_ops,
		.id	= AK_VENC_CID_MB_RC_MODE,
		.name	= "MB Rate Control Mode",
		.type	= V4L2_CTRL_TYPE_MENU,
		.min	= 0,
		.max	= AK_VENC_MB_RC_MODE_CHECKPOINT,
		.def	= AK_VENC_MB_RC_MODE_MAD,
		.qmenu	= ak_venc_mb_rc_mode_menu,
	}, {
		.ops	= &ak_venc_ctrl_ops,
		.id	= AK_VENC_CID_MAD_QP_DELTA,
		.name	= "MAD Based QP Adjustment",
		.type	= V4L2_CTRL_TYPE_INTEGER,
		.min	= -8,
		.max	= 7,
		.step	= 1,
		.def	= -2,
	}, {
		.ops	= &ak_venc_ctrl_ops,
		.id	= AK_VENC_CID_MAD_THRESHOLD,
		.name	= "MAD Threshold",
		.type	= V4L2_CTRL_TYPE_INTEGER,
		.min	= 0,
		.max	= 63,
		.step	= 1,
		.def	= 12,
	}, {
		.ops	= &ak_venc_ctrl_ops,
		.id	= AK_VENC_CID_MAD_ADAPT,
		.name	= "MAD Threshold Adaptation",
		.type	= V4L2_CTRL_TYPE_BOOLEAN,
		.min	= 0,
		.max	= 1,
		.step	= 1,
		.def	= 1,
	},
};

static int ak_venc_init_ctrls(struct ak_venc_ctx *ctx)
{
	struct v4l2_ctrl_handler *hdl = &ctx->ctrl_handler;
	unsigned i;

	v4l2_ctrl_handler_init(hdl, 20);

	v4l2_ctrl_new_std(hdl, &ak_venc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP,
			  0, 51, 1, AK_VENC_DEF_QP);
	v4l2_ctrl_new_std(hdl, &ak_venc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP,
			  0, 51, 1, AK_VENC_DEF_QP);
	ctx->qp_limits[0] = v4l2_ctrl_new_std(hdl, &ak_venc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_MIN_QP,
			  0, 51, 1, AK_VENC_DEF_QP_MIN);
	ctx->qp_limits[1] = v4l2_ctrl_new_std(hdl, &ak_venc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_H264_MAX_QP,
			  0, 51, 1, AK_VENC_DEF_QP_MAX);
	v4l2_ctrl_new_std(hdl, &ak_venc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_BITRATE,
			  AK_VENC_MIN_BITRATE, AK_VENC_MAX_BITRATE, 1024,
			  AK_VENC_DEF_BITRATE);
	v4l2_ctrl_new_std_menu(hdl, &ak_venc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_BITRATE_MODE,
			       V4L2_MPEG_VIDEO_BITRATE_MODE_CBR, 0,
			       V4L2_MPEG_VIDEO_BITRATE_MODE_VBR);
	v4l2_ctrl_new_std(hdl, &ak_venc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_GOP_SIZE,
			  1, 600, 1, AK_VENC_DEF_GOP);
	v4l2_ctrl_new_std(hdl, &ak_venc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE,
			  0, 1, 1, 0);
	v4l2_ctrl_new_std(hdl, &ak_venc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE,
			  0, 1, 1, 1);
	v4l2_ctrl_new_std(hdl, &ak_venc_ctrl_ops,
			  V4L2_CID_MPEG_VIDEO_VBV_SIZE,
			  0, 8192, 1, 0);
	/* The core has no CABAC table generator yet, so Main is not offered. */
	v4l2_ctrl_new_std_menu(hdl, &ak_venc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_H264_PROFILE,
			       V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE,
			       0,
			       V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE);
	v4l2_ctrl_new_std_menu(hdl, &ak_venc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_H264_LEVEL,
			       V4L2_MPEG_VIDEO_H264_LEVEL_4_0, 0,
			       V4L2_MPEG_VIDEO_H264_LEVEL_3_0);
	/*
	 * The IJG scale, 1..100. Below about 10 the tables leave the lattice
	 * the hardware can hold almost everywhere and the picture is blocks;
	 * the range is still the standard one, because clamping a standard
	 * control to a driver's taste is worse than a bad picture.
	 */
	v4l2_ctrl_new_std(hdl, &ak_venc_ctrl_ops,
			  V4L2_CID_JPEG_COMPRESSION_QUALITY,
			  1, 100, 1, AK_VENC_DEF_JPEG_QUALITY);

	for (i = 0; i < ARRAY_SIZE(ak_venc_custom_ctrls); i++)
		v4l2_ctrl_new_custom(hdl, &ak_venc_custom_ctrls[i], NULL);

	if (hdl->error) {
		int ret = hdl->error;

		v4l2_ctrl_handler_free(hdl);
		return ret;
	}

	v4l2_ctrl_cluster(ARRAY_SIZE(ctx->qp_limits), ctx->qp_limits);
	return v4l2_ctrl_handler_setup(hdl);
}

/* -------------------------------------------------------------------------
 * file operations
 * ------------------------------------------------------------------------- */

static void ak_venc_set_default_fmt(struct ak_venc_ctx *ctx)
{
	ctx->out_q.fmt = find_format(V4L2_PIX_FMT_YUV420, AK_VENC_OUTPUT);
	ctx->out_q.width = AK_VENC_DEF_WIDTH;
	ctx->out_q.height = AK_VENC_DEF_HEIGHT;
	ctx->out_q.bytesperline = AK_VENC_DEF_WIDTH;
	ctx->out_q.sizeimage = AK_VENC_DEF_WIDTH * AK_VENC_DEF_HEIGHT * 3 / 2;

	ctx->cap_q.fmt = find_format(V4L2_PIX_FMT_H264, AK_VENC_CAPTURE);
	ctx->cap_q.width = AK_VENC_DEF_WIDTH;
	ctx->cap_q.height = AK_VENC_DEF_HEIGHT;
	ctx->cap_q.bytesperline = 0;
	ctx->cap_q.sizeimage = ak_venc_cap_size(V4L2_PIX_FMT_H264,
						AK_VENC_DEF_WIDTH,
						AK_VENC_DEF_HEIGHT);
}

static int ak_venc_open(struct file *file)
{
	struct ak_venc_dev *dev = video_drvdata(file);
	struct ak_venc_ctx *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->dev = dev;
	INIT_WORK(&ctx->program_error_work, ak_venc_program_error);
	ak_venc_rc_defaults(&ctx->rc_cfg);
	ctx->rc_cfg.gop_len = AK_VENC_DEF_GOP;
	ctx->rc_cfg.fps_num = AK_VENC_DEF_FPS;
	ctx->rc_cfg.fps_den = 1;
	ctx->gop.gop = AK_VENC_DEF_GOP;
	ctx->mb_rc_enable = 1;
	ctx->mb_rc_mode = AK_VENC_MB_RC_MODE_MAD;
	ctx->profile = V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE;
	ctx->level_idc = 30;
	ctx->pps_qp = AK_VENC_DEF_QP;
	ak_venc_set_default_fmt(ctx);

	v4l2_fh_init(&ctx->fh, dev->vfd);
	file->private_data = &ctx->fh;

	ret = ak_venc_init_ctrls(ctx);
	if (ret)
		goto err_fh;
	ctx->fh.ctrl_handler = &ctx->ctrl_handler;

	ctx->m2m_ctx = v4l2_m2m_ctx_init(dev->m2m_dev, ctx, &queue_init);
	if (IS_ERR(ctx->m2m_ctx)) {
		ret = PTR_ERR(ctx->m2m_ctx);
		goto err_ctrls;
	}

	v4l2_fh_add(&ctx->fh);
	return 0;

err_ctrls:
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
err_fh:
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);
	return ret;
}

static int ak_venc_release(struct file *file)
{
	struct ak_venc_ctx *ctx = fh_to_ctx(file->private_data);
	struct ak_venc_dev *dev = ctx->dev;

	mutex_lock(&dev->dev_mutex);
	v4l2_m2m_ctx_release(ctx->m2m_ctx);
	cancel_work_sync(&ctx->program_error_work);
	ak_venc_free_priv(ctx);
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);
	mutex_unlock(&dev->dev_mutex);
	return 0;
}

static unsigned int ak_venc_poll(struct file *file,
				 struct poll_table_struct *wait)
{
	struct ak_venc_ctx *ctx = fh_to_ctx(file->private_data);

	return v4l2_m2m_poll(file, ctx->m2m_ctx, wait);
}

static int ak_venc_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct ak_venc_ctx *ctx = fh_to_ctx(file->private_data);

	return v4l2_m2m_mmap(file, ctx->m2m_ctx, vma);
}

static const struct v4l2_file_operations ak_venc_fops = {
	.owner		= THIS_MODULE,
	.open		= ak_venc_open,
	.release	= ak_venc_release,
	.poll		= ak_venc_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= ak_venc_mmap,
};

static struct video_device ak_venc_videodev = {
	.name		= AK_VENC_NAME,
	.fops		= &ak_venc_fops,
	.ioctl_ops	= &ak_venc_ioctl_ops,
	.minor		= -1,
	.release	= video_device_release,
	/*
	 * Without this the node defaults to VFL_DIR_RX, and 4.4's
	 * determine_valid_ioctls() computes is_tx as false and clears the
	 * valid bit for every *_vid_out ioctl - so S_FMT on the OUTPUT queue
	 * returns -EINVAL from video_ioctl2 before this driver is reached, and
	 * an m2m encoder that cannot take an input format is inert. 3.4 has no
	 * such gate, which is why this was invisible until the port.
	 */
	.vfl_dir	= VFL_DIR_M2M,
};

/* -------------------------------------------------------------------------
 * platform driver
 * ------------------------------------------------------------------------- */

/*
 * The reference and reconstruction frame stores are one coherent allocation
 * that is order-10 at 720p, which the buddy allocator cannot satisfy on a
 * 36 MiB system once anything has run.  The pool is the same carveout the
 * capture side takes; of_reserved_mem hands a shared-dma-pool to every device
 * that names it, so the two share one bitmap.
 */
static int ak_venc_claim_pool(struct platform_device *pdev)
{
	struct device_node *np;
	struct resource res;
	int ret;

	np = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!np) {
		dev_warn(&pdev->dev,
			 "no memory-region: frame stores come from the buddy allocator and will fail above CIF\n");
		return 0;
	}

	ret = of_address_to_resource(np, 0, &res);
	of_node_put(np);
	if (ret) {
		dev_err(&pdev->dev, "encoder pool has no usable reg\n");
		return ret;
	}

	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret) {
		dev_err(&pdev->dev, "cannot claim the encoder pool: %d\n", ret);
		return ret;
	}

	dev_info(&pdev->dev, "encoder pool at %pa, %u KiB\n",
		 &res.start, (unsigned int)(resource_size(&res) / 1024));
	return 0;
}

static int ak_venc_probe(struct platform_device *pdev)
{
	struct ak_venc_dev *dev;
	struct video_device *vfd;
	struct resource *res;
	u32 id;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->dev = &pdev->dev;
	spin_lock_init(&dev->irqlock);
	mutex_init(&dev->dev_mutex);

	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
	ret = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		goto err_free;

	ret = ak_venc_claim_pool(pdev);
	if (ret)
		goto err_free;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENXIO;
		goto err_free;
	}

	dev->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dev->regs)) {
		ret = PTR_ERR(dev->regs);
		goto err_free;
	}

	dev->irq = platform_get_irq(pdev, 0);
	if (dev->irq < 0) {
		dev_err(&pdev->dev, "no interrupt resource\n");
		ret = dev->irq;
		goto err_unmap;
	}

	dev->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(dev->clk))
		dev->clk = NULL;
	else
		clk_prepare_enable(dev->clk);

	/* The id register reads as zero with the clock gated, so the pulse
	 * comes first. */
	ak_venc_hw_on(dev);
	id = readl(dev->regs);
	ak_venc_hw_off(dev);
	if (id != AK_VENC_ASIC_ID) {
		dev_err(&pdev->dev, "not an 8290: id 0x%08x\n", id);
		ret = -ENODEV;
		goto err_unmap;
	}

	ret = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
	if (ret)
		goto err_unmap;

	dev->alloc_ctx = vb2_dma_contig_init_ctx(&pdev->dev);
	if (IS_ERR(dev->alloc_ctx)) {
		ret = PTR_ERR(dev->alloc_ctx);
		goto err_v4l2;
	}

	vfd = video_device_alloc();
	if (!vfd) {
		ret = -ENOMEM;
		goto err_ctx;
	}
	*vfd = ak_venc_videodev;
	vfd->lock = &dev->dev_mutex;
	vfd->v4l2_dev = &dev->v4l2_dev;
	dev->vfd = vfd;

	dev->m2m_dev = v4l2_m2m_init(&ak_venc_m2m_ops);
	if (IS_ERR(dev->m2m_dev)) {
		ret = PTR_ERR(dev->m2m_dev);
		goto err_vdev;
	}

	video_set_drvdata(vfd, dev);
	platform_set_drvdata(pdev, dev);

	ret = video_register_device(vfd, VFL_TYPE_GRABBER, -1);
	if (ret)
		goto err_m2m;

	if (device_create_file(&pdev->dev, &dev_attr_venc_stats))
		dev_warn(&pdev->dev, "no venc_stats attribute\n");

	if (device_create_file(&pdev->dev, &dev_attr_dma_coherent_pool))
		dev_warn(&pdev->dev, "no dma_coherent_pool attribute\n");

	v4l2_info(&dev->v4l2_dev, "8290 encoder at /dev/video%d, irq %d\n",
		  vfd->num, dev->irq);
	return 0;

err_m2m:
	v4l2_m2m_release(dev->m2m_dev);
err_vdev:
	video_device_release(vfd);
err_ctx:
	vb2_dma_contig_cleanup_ctx(dev->alloc_ctx);
err_v4l2:
	v4l2_device_unregister(&dev->v4l2_dev);
err_unmap:
	if (dev->clk)
		clk_disable_unprepare(dev->clk);
err_free:
	of_reserved_mem_device_release(&pdev->dev);
	kfree(dev);
	return ret;
}

static int ak_venc_remove(struct platform_device *pdev)
{
	struct ak_venc_dev *dev = platform_get_drvdata(pdev);

	device_remove_file(&pdev->dev, &dev_attr_venc_stats);
	video_unregister_device(dev->vfd);
	v4l2_m2m_release(dev->m2m_dev);
	vb2_dma_contig_cleanup_ctx(dev->alloc_ctx);
	v4l2_device_unregister(&dev->v4l2_dev);
	if (dev->clk)
		clk_disable_unprepare(dev->clk);
	of_reserved_mem_device_release(&pdev->dev);
	kfree(dev);
	return 0;
}

static const struct of_device_id ak_venc_of_match[] = {
	{ .compatible = "anyka,ak3918ev200-venc" },
	{ }
};
MODULE_DEVICE_TABLE(of, ak_venc_of_match);

static struct platform_driver ak_venc_driver = {
	.probe	= ak_venc_probe,
	.remove	= ak_venc_remove,
	.driver	= {
		.name		= AK_VENC_NAME,
		.suppress_bind_attrs = true,
		.owner		= THIS_MODULE,
		.of_match_table	= ak_venc_of_match,
	},
};

module_platform_driver(ak_venc_driver);

MODULE_DESCRIPTION("Anyka AK3918EV200 8290 V4L2 mem2mem encoder");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" AK_VENC_NAME);
