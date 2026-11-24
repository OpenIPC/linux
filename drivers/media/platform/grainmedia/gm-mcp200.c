// SPDX-License-Identifier: GPL-2.0-only
/*
 * Grain Media GM8126 H.264 encoder (FTMCP200), as a stateful V4L2 mem2mem
 * device: packed 4:2:2 in, baseline Annex B out.
 *
 * There is no public documentation; the registers, the DMA chain and their
 * values were observed on a camera's own driver.
 *
 * The driver writes the parameter sets and slice headers itself, element by
 * element, through the bit packer the engine then continues with the slice
 * data. The source comes through a wrapper that turns the raster into
 * macroblocks. The chain of DMA commands for one macroblock row lives in
 * memory; it depends on the picture's size, and every picture on the
 * addresses.
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>

/* engine */
#define M2_SIZE			0x000
#define M2_PARM			0x004
#define M2_MEM0			0x00c
#define M2_QP			0x010
#define M2_MEM1			0x014
#define M2_SOURCE		0x018
#define M2_PACK_DATA		0x01c
#define M2_GO			0x020
#define   GO_I			1
#define   GO_P			3
#define   GO_RESET		BIT(31)
#define M2_PACK			0x024
#define   PACK_GO		BIT(6)
#define   PACK_UE		BIT(7)
#define   PACK_EP		BIT(8)
#define   PACK_TRAILING		BIT(9)
#define   PACK_CLEAR		BIT(11)
#define   PACK_BUSY		(PACK_GO | PACK_TRAILING)
#define M2_STATUS		0x040
#define M2_BYTES		0x044
#define M2_IRQ			0x048
#define M2_CHAIN		0x098
#define M2_CHAIN_CTL		0x09c
#define M2_BS			0x0a8
#define M2_BS_CTL		0x0ac
/* the DMA unit's other half repeats the registers from 0x98 here */
#define M2_HALF			0x080

/* wrapper */
#define W_LEFT			0x00
#define W_RIGHT			0x04
#define W_WIDTH			0x08
#define W_HEIGHT		0x0c
#define W_ADDR			0x18
#define W_FORMAT		0x1c
#define   W_PACKED_422		0x65
#define W_GO			0x20

#define CHAIN_WORDS		(0x140 / 4)

#define M2_CHROMA_QP		5
#define M2_DEBLOCK		3	/* both offsets / 2 */
#define M2_POC_BITS		5
#define M2_INIT_QP		26

#define M2_MIN			64
#define M2_MAX_W		1280
#define M2_MAX_H		1024
#define M2_TIMEOUT_MS		1000

enum { M2_SRC, M2_DST };

struct m2_dev {
	struct device *dev;
	void __iomem *base, *wrap;
	struct v4l2_device v4l2;
	struct video_device vdev;
	struct v4l2_m2m_dev *m2m;
	struct mutex lock;
	struct clk *clk;
	struct delayed_work watchdog;
	u32 job, irq_job;		/* the one running, the one done */
};

struct m2_buf {
	void *cpu;
	dma_addr_t dma;
	size_t size;
};

struct m2_ctx {
	struct v4l2_fh fh;
	struct m2_dev *m2;
	struct v4l2_ctrl_handler hdl;
	struct v4l2_pix_format fmt[2];
	struct v4l2_fract interval;
	u32 sequence[2];

	struct m2_buf pic[2];		/* reconstruction and reference */
	struct m2_buf info;		/* per macroblock */
	struct m2_buf chain;

	u32 gop, qp_i, qp_p, qp_min, qp_max, bitrate;
	bool rate_control, force_idr;

	u32 frame;			/* since the IDR */
	u32 idr;			/* idr_pic_id */
	unsigned int cur;		/* which of pic[] is being written */
	u32 qp;
	s64 budget;			/* bits owed by the rate control */
	bool job_idr;
};

static struct m2_ctx *file_to_ctx(struct file *file)
{
	return container_of(file_to_v4l2_fh(file), struct m2_ctx, fh);
}

/* ---- the bit packer --------------------------------------------------------- */

static void m2_pack(struct m2_dev *m2, u32 val, u32 ctl)
{
	u32 v;

	writel(val, m2->base + M2_PACK_DATA);
	writel(ctl, m2->base + M2_PACK);
	/* a set bit is an element still being packed */
	readl_poll_timeout_atomic(m2->base + M2_PACK, v, !(v & PACK_BUSY),
				  0, 1000);
}

static void put_u(struct m2_dev *m2, unsigned int bits, u32 val)
{
	m2_pack(m2, val, PACK_EP | PACK_GO | bits);
}

static void put_ue(struct m2_dev *m2, u32 val)
{
	m2_pack(m2, val, PACK_EP | PACK_UE | PACK_GO);
}

static void put_se(struct m2_dev *m2, s32 val)
{
	put_ue(m2, val > 0 ? 2 * val - 1 : -2 * val);
}

static void put_nal(struct m2_dev *m2, u8 header)
{
	m2_pack(m2, 0, PACK_GO | 8);
	m2_pack(m2, 1, PACK_GO | 24);
	m2_pack(m2, header, PACK_GO | 8);
}

static void put_trailing(struct m2_dev *m2)
{
	u32 v;

	writel(PACK_TRAILING, m2->base + M2_PACK);
	readl_poll_timeout_atomic(m2->base + M2_PACK, v, !(v & PACK_BUSY),
				  0, 1000);
}

/* the lowest level the size and rate fit, table A-1 */
static u32 m2_level(struct m2_ctx *ctx)
{
	static const struct {
		u32 level, frame, rate;
	} levels[] = {
		{ 30, 1620, 40500 }, { 31, 3600, 108000 }, { 32, 5120, 216000 },
		{ 40, 8192, 245760 }, { 42, 8704, 522240 }, { 50, 22080, 589824 },
	};
	const u32 mbs = ctx->fmt[M2_SRC].width / 16 *
			(ctx->fmt[M2_SRC].height / 16);
	const u32 rate = div_u64((u64)mbs * ctx->interval.denominator,
				 ctx->interval.numerator);
	int i;

	for (i = 0; i < ARRAY_SIZE(levels); i++)
		if (mbs <= levels[i].frame && rate <= levels[i].rate)
			return levels[i].level;
	return 51;
}

static void m2_sps_pps(struct m2_ctx *ctx)
{
	struct m2_dev *m2 = ctx->m2;
	const u32 mbw = ctx->fmt[M2_SRC].width / 16;
	const u32 mbh = ctx->fmt[M2_SRC].height / 16;

	put_nal(m2, 0x67);
	put_u(m2, 8, 66);		/* baseline */
	put_u(m2, 8, 0);		/* constraints */
	put_u(m2, 8, m2_level(ctx));
	put_ue(m2, 0);			/* seq_parameter_set_id */
	put_ue(m2, 0);			/* log2_max_frame_num - 4 */
	put_ue(m2, 0);			/* pic_order_cnt_type */
	put_ue(m2, M2_POC_BITS - 4);
	put_ue(m2, 1);			/* max_num_ref_frames */
	put_u(m2, 1, 0);		/* gaps_in_frame_num */
	put_ue(m2, mbw - 1);
	put_ue(m2, mbh - 1);
	put_u(m2, 1, 1);		/* frame_mbs_only */
	put_u(m2, 1, 1);		/* direct_8x8_inference */
	put_u(m2, 1, 0);		/* frame_cropping */
	put_u(m2, 1, 1);		/* VUI: timing, no reordering */
	put_u(m2, 1, 0);		/* aspect_ratio_info */
	put_u(m2, 1, 0);		/* overscan_info */
	put_u(m2, 1, 0);		/* video_signal_type */
	put_u(m2, 1, 0);		/* chroma_loc_info */
	put_u(m2, 1, 1);		/* timing_info */
	put_u(m2, 32, ctx->interval.numerator);
	put_u(m2, 32, ctx->interval.denominator * 2);
	put_u(m2, 1, 0);		/* fixed_frame_rate */
	put_u(m2, 1, 0);		/* nal_hrd */
	put_u(m2, 1, 0);		/* vcl_hrd */
	put_u(m2, 1, 0);		/* pic_struct */
	put_u(m2, 1, 1);		/* bitstream_restriction */
	put_u(m2, 1, 1);		/* motion vectors over the edges */
	put_ue(m2, 0);
	put_ue(m2, 0);
	put_ue(m2, 7);
	put_ue(m2, 7);
	put_ue(m2, 0);			/* max_num_reorder_frames */
	put_ue(m2, 1);			/* max_dec_frame_buffering */
	put_trailing(m2);

	put_nal(m2, 0x68);
	put_ue(m2, 0);			/* pic_parameter_set_id */
	put_ue(m2, 0);			/* seq_parameter_set_id */
	put_u(m2, 1, 0);		/* CAVLC */
	put_u(m2, 1, 0);		/* pic_order_present */
	put_ue(m2, 0);			/* one slice group */
	put_ue(m2, 0);			/* num_ref_idx_l0 - 1 */
	put_ue(m2, 0);
	put_u(m2, 1, 0);		/* weighted_pred */
	put_u(m2, 2, 0);
	put_se(m2, 0);			/* pic_init_qp - 26 */
	put_se(m2, 0);
	put_se(m2, M2_CHROMA_QP);
	put_u(m2, 1, 1);		/* deblocking_filter_control */
	put_u(m2, 1, 0);		/* constrained_intra_pred */
	put_u(m2, 1, 0);
	put_trailing(m2);
}

static void m2_slice_header(struct m2_ctx *ctx, bool idr)
{
	struct m2_dev *m2 = ctx->m2;

	put_nal(m2, idr ? 0x65 : 0x41);
	put_ue(m2, 0);			/* first_mb */
	put_ue(m2, idr ? 7 : 5);
	put_ue(m2, 0);			/* pps */
	put_u(m2, 4, ctx->frame & 0xf);
	if (idr)
		put_ue(m2, ctx->idr & 1);
	put_u(m2, M2_POC_BITS, (ctx->frame * 2) & ((1 << M2_POC_BITS) - 1));
	if (idr) {
		put_u(m2, 1, 0);	/* no_output_of_prior_pics */
		put_u(m2, 1, 0);	/* long_term_reference */
	} else {
		put_u(m2, 1, 0);	/* num_ref_idx_override */
		put_u(m2, 1, 0);	/* ref_pic_list_modification */
		put_u(m2, 1, 0);	/* adaptive marking */
	}
	put_se(m2, (s32)ctx->qp - M2_INIT_QP);
	put_ue(m2, 0);			/* deblock */
	put_se(m2, M2_DEBLOCK);
	put_se(m2, M2_DEBLOCK);
}

/* ---- the DMA chain ------------------------------------------------------------ */

static u32 chain_step(s32 words, u32 len)
{
	return ((u32)words & 0xffff) << 8 | len;
}

/* What depends on the size only. Words 0-3 of each command: address, the
 * local side, the two-dimensional step, control. */
static void m2_chain_init(struct m2_ctx *ctx)
{
	const s32 w = ctx->fmt[M2_SRC].width / 16;
	const u32 mbs = w * (ctx->fmt[M2_SRC].height / 16);
	u32 *c = ctx->chain.cpu;

	memset(c, 0, ctx->chain.size);
	c[0x00] = ctx->chain.dma;
	c[0x01] = 0x4e;
	c[0x02] = 3;
	c[0x03] = 0x40;
	c[0x06] = 1;
	c[0x07] = 0x1020;
	c[0x08] = 8;
	c[0x0b] = 0x9050;
	c[0x0c] = chain_step(-64 * w - 15, 0x20);
	c[0x0f] = 0x9030;
	c[0x10] = chain_step(-32 * w - 15, 0x10);
	c[0x13] = 0x10;
	c[0x17] = 0x10;
	c[0x1b] = 0x800c;
	c[0x1c] = (mbs - 1) << 24;
	c[0x1f] = 0xc;
	c[0x20] = (mbs - 1) << 24;
	c[0x23] = 0x10c0;
	c[0x24] = chain_step(64 * w - 63, 0x20);
	c[0x27] = 0x1010;
	c[0x28] = chain_step(256 * w - 63, 4);
	c[0x2b] = 0x1060;
	c[0x2c] = chain_step(32 * w - 31, 0x10);
	c[0x2f] = 0x1080;
	c[0x30] = chain_step(64 * w - 63, 0x20);
	c[0x33] = 8;
	c[0x37] = 0x1040;
	c[0x38] = chain_step(32 * w - 31, 0x10);
	c[0x3b] = 0x1080;
	c[0x3c] = chain_step(64 * w - 63, 0x20);
	c[0x3f] = 8;
	c[0x43] = 0x1040;
	c[0x44] = chain_step(32 * w - 31, 0x10);
	c[0x47] = 0x8040;
	c[0x48] = chain_step(-64 * w - 15, 0x20);
	c[0x4b] = 0x8020;
	c[0x4c] = chain_step(-32 * w - 15, 0x10);
}

/* The pictures' addresses, before each one. */
static void m2_chain_update(struct m2_ctx *ctx)
{
	const u32 w = ctx->fmt[M2_SRC].width / 16;
	const u32 luma = w * (ctx->fmt[M2_SRC].height / 16) * 256;
	const u32 rec = ctx->pic[ctx->cur].dma;
	const u32 ref = ctx->pic[!ctx->cur].dma;
	const u32 info = ctx->info.dma;
	u32 *c = ctx->chain.cpu;

	c[0x0a] = (rec + w * 0x100) | 3;
	c[0x0e] = (rec + luma + w * 0x80) | 2;
	c[0x12] = (rec + 0xc0) | 3;
	c[0x16] = (rec + luma + 0x40) | 2;
	c[0x1a] = info | 1;
	c[0x1d] = info;
	c[0x1e] = info | 1;
	c[0x21] = info;
	c[0x22] = ref | 3;
	c[0x26] = (ref + 0xe0) | 3;
	c[0x2a] = (ref + luma) | 2;
	c[0x2e] = ref | 3;
	c[0x32] = (ref + w * 0x200) | 3;
	c[0x36] = (ref + luma) | 2;
	c[0x3a] = (ref + luma - w * 0x200) | 3;
	c[0x3e] = (ref + luma - w * 0x400 + 0xe0) | 3;
	c[0x42] = (ref + luma + luma / 2 - w * 0x100) | 2;
	c[0x46] = rec | 3;
	c[0x4a] = (rec + luma) | 2;
	dma_wmb();
}

/* ---- running ------------------------------------------------------------------- */

static void m2_both(struct m2_dev *m2, u32 reg, u32 val)
{
	writel(val, m2->base + reg);
	writel(val, m2->base + reg + M2_HALF);
}

static void m2_hw_init(struct m2_dev *m2)
{
	writel(GO_RESET, m2->base + M2_GO);
	m2_both(m2, M2_CHAIN_CTL, 0x4e);
	m2_both(m2, M2_BS_CTL, 0x8010);
	/* the local memories' timing, as the vendor's table */
	writel(0x0000dbc0, m2->base + M2_MEM0);
	writel(0x001f243f, m2->base + M2_MEM1);
	writel(0, m2->base + M2_IRQ);
}

static void m2_device_run(void *priv)
{
	struct m2_ctx *ctx = priv;
	struct m2_dev *m2 = ctx->m2;
	struct vb2_v4l2_buffer *src = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	struct vb2_v4l2_buffer *dst = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
	const u32 mbw = ctx->fmt[M2_SRC].width / 16;
	const u32 mbh = ctx->fmt[M2_SRC].height / 16;
	const u32 stride = ctx->fmt[M2_SRC].bytesperline / 32;
	u32 parm;
	bool idr;

	idr = !ctx->frame || ctx->force_idr ||
	      (ctx->gop && ctx->frame >= ctx->gop);
	if (idr) {
		if (ctx->frame)
			ctx->idr++;
		ctx->frame = 0;
		ctx->force_idr = false;
	}
	if (!ctx->rate_control)
		ctx->qp = idr ? ctx->qp_i : ctx->qp_p;
	ctx->job_idr = idr;

	m2_chain_update(ctx);

	writel(0, m2->base + M2_IRQ);
	writel(PACK_CLEAR, m2->base + M2_PACK);
	writel((mbw - 1) | (mbh - 1) << 8 | (mbw * mbh - 1) << 16,
	       m2->base + M2_SIZE);
	parm = 0x00440000 | M2_DEBLOCK << 12 | M2_DEBLOCK << 8 | M2_CHROMA_QP;
	writel(parm, m2->base + M2_PARM);
	writel(ctx->qp << 4, m2->base + M2_QP);
	m2_both(m2, M2_CHAIN, (u32)ctx->chain.dma);
	writel(1, m2->base + M2_SOURCE);

	writel(0, m2->wrap + W_LEFT);
	writel(stride - mbw, m2->wrap + W_RIGHT);
	writel(mbw, m2->wrap + W_WIDTH);
	writel(mbh, m2->wrap + W_HEIGHT);
	writel(W_PACKED_422, m2->wrap + W_FORMAT);
	writel((u32)vb2_dma_contig_plane_dma_addr(&src->vb2_buf, 0) >> 2,
	       m2->wrap + W_ADDR);
	writel(1, m2->wrap + W_GO);

	m2_both(m2, M2_BS,
		(u32)vb2_dma_contig_plane_dma_addr(&dst->vb2_buf, 0) | 1);
	if (idr)
		m2_sps_pps(ctx);
	m2_slice_header(ctx, idr);

	WRITE_ONCE(m2->job, m2->job + 1);
	schedule_delayed_work(&m2->watchdog, msecs_to_jiffies(M2_TIMEOUT_MS));
	writel(idr ? GO_I : GO_P, m2->base + M2_GO);
}

/* One step of QP for a picture's worth over or under, by a leaky bucket. */
static void m2_rate(struct m2_ctx *ctx, u32 bytes)
{
	u32 target, qp = ctx->qp;
	s64 limit;

	if (!ctx->rate_control)
		return;
	target = div_u64((u64)ctx->bitrate * ctx->interval.numerator,
			 ctx->interval.denominator);
	limit = 8LL * target;
	ctx->budget += (s64)bytes * 8 - target;
	ctx->budget = clamp_t(s64, ctx->budget, -limit, 4 * limit);
	if (ctx->job_idr)	/* its cost is spread over the group */
		return;
	if (bytes * 8 > target && ctx->budget > 0)
		qp += 1 + (bytes * 8 > 2 * target);
	else if (bytes * 8 < target - target / 8 && ctx->budget <= 0)
		qp -= 1;
	ctx->qp = clamp(qp, ctx->qp_min, ctx->qp_max);
}

static void m2_job_done(struct m2_dev *m2, enum vb2_buffer_state state)
{
	struct m2_ctx *ctx = v4l2_m2m_get_curr_priv(m2->m2m);
	struct vb2_v4l2_buffer *src, *dst;
	u32 bytes;

	if (!ctx)
		return;
	src = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
	dst = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
	bytes = readl(m2->base + M2_BYTES);
	if (bytes > vb2_plane_size(&dst->vb2_buf, 0))
		state = VB2_BUF_STATE_ERROR;

	v4l2_m2m_buf_copy_metadata(src, dst);
	src->sequence = ctx->sequence[M2_SRC]++;
	dst->sequence = ctx->sequence[M2_DST]++;
	dst->flags &= ~(V4L2_BUF_FLAG_KEYFRAME | V4L2_BUF_FLAG_PFRAME);
	dst->flags |= ctx->job_idr ? V4L2_BUF_FLAG_KEYFRAME :
				     V4L2_BUF_FLAG_PFRAME;
	vb2_set_plane_payload(&dst->vb2_buf, 0,
			      state == VB2_BUF_STATE_DONE ? bytes : 0);
	if (state == VB2_BUF_STATE_DONE) {
		m2_rate(ctx, bytes);
		ctx->frame++;
		ctx->cur ^= 1;
	} else {
		ctx->frame = 0;		/* start again with an IDR */
		ctx->idr++;
	}
	v4l2_m2m_buf_done(src, state);
	v4l2_m2m_buf_done(dst, state);
	v4l2_m2m_job_finish(m2->m2m, ctx->fh.m2m_ctx);
}

static irqreturn_t m2_irq(int irq, void *data)
{
	struct m2_dev *m2 = data;

	if (!(readl(m2->base + M2_IRQ) & 1))
		return IRQ_NONE;
	writel(0, m2->base + M2_IRQ);
	WRITE_ONCE(m2->irq_job, READ_ONCE(m2->job));
	return IRQ_WAKE_THREAD;
}

static irqreturn_t m2_irq_thread(int irq, void *data)
{
	struct m2_dev *m2 = data;

	/* not for a job the watchdog has given up */
	if (READ_ONCE(m2->irq_job) != READ_ONCE(m2->job))
		return IRQ_HANDLED;
	if (cancel_delayed_work_sync(&m2->watchdog))
		m2_job_done(m2, VB2_BUF_STATE_DONE);
	return IRQ_HANDLED;
}

static void m2_watchdog(struct work_struct *work)
{
	struct m2_dev *m2 = container_of(work, struct m2_dev, watchdog.work);

	dev_err(m2->dev, "encoding timed out\n");
	m2_hw_init(m2);
	WRITE_ONCE(m2->job, m2->job + 1);
	m2_job_done(m2, VB2_BUF_STATE_ERROR);
}

static const struct v4l2_m2m_ops m2_m2m_ops = {
	.device_run = m2_device_run,
};

/* ---- vb2 -------------------------------------------------------------------------- */

static struct v4l2_pix_format *ctx_fmt(struct m2_ctx *ctx, u32 type)
{
	return &ctx->fmt[V4L2_TYPE_IS_OUTPUT(type) ? M2_SRC : M2_DST];
}

static int m2_queue_setup(struct vb2_queue *q, unsigned int *nbufs,
			  unsigned int *nplanes, unsigned int sizes[],
			  struct device *alloc_devs[])
{
	struct v4l2_pix_format *p = ctx_fmt(vb2_get_drv_priv(q), q->type);

	if (*nplanes)
		return sizes[0] < p->sizeimage ? -EINVAL : 0;
	*nplanes = 1;
	sizes[0] = p->sizeimage;
	return 0;
}

static int m2_buf_prepare(struct vb2_buffer *vb)
{
	struct m2_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	if (vb2_plane_size(vb, 0) < ctx_fmt(ctx, vb->type)->sizeimage)
		return -EINVAL;
	return 0;
}

static int m2_buf_out_validate(struct vb2_buffer *vb)
{
	to_vb2_v4l2_buffer(vb)->field = V4L2_FIELD_NONE;
	return 0;
}

static void m2_buf_queue(struct vb2_buffer *vb)
{
	struct m2_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, to_vb2_v4l2_buffer(vb));
}

static void m2_free(struct m2_ctx *ctx, struct m2_buf *b)
{
	if (b->cpu)
		dma_free_coherent(ctx->m2->dev, b->size, b->cpu, b->dma);
	b->cpu = NULL;
}

static int m2_alloc(struct m2_ctx *ctx, struct m2_buf *b, size_t size)
{
	b->size = PAGE_ALIGN(size);
	b->cpu = dma_alloc_coherent(ctx->m2->dev, b->size, &b->dma, GFP_KERNEL);
	return b->cpu ? 0 : -ENOMEM;
}

static void m2_free_all(struct m2_ctx *ctx)
{
	m2_free(ctx, &ctx->pic[0]);
	m2_free(ctx, &ctx->pic[1]);
	m2_free(ctx, &ctx->info);
	m2_free(ctx, &ctx->chain);
}

static void m2_return_all(struct m2_ctx *ctx, u32 type,
			  enum vb2_buffer_state state)
{
	struct vb2_v4l2_buffer *vb;

	for (;;) {
		vb = V4L2_TYPE_IS_OUTPUT(type) ?
		     v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx) :
		     v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
		if (!vb)
			return;
		v4l2_m2m_buf_done(vb, state);
	}
}

static int m2_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct m2_ctx *ctx = vb2_get_drv_priv(q);
	const u32 mbs = ctx->fmt[M2_SRC].width / 16 *
			(ctx->fmt[M2_SRC].height / 16);
	int ret;

	if (!V4L2_TYPE_IS_OUTPUT(q->type)) {
		ctx->sequence[M2_DST] = 0;
		return 0;
	}
	ctx->sequence[M2_SRC] = 0;
	ret = m2_alloc(ctx, &ctx->pic[0], mbs * 384);
	ret = ret ?: m2_alloc(ctx, &ctx->pic[1], mbs * 384);
	ret = ret ?: m2_alloc(ctx, &ctx->info, mbs * 64);
	ret = ret ?: m2_alloc(ctx, &ctx->chain, CHAIN_WORDS * sizeof(u32));
	if (ret) {
		m2_free_all(ctx);
		m2_return_all(ctx, q->type, VB2_BUF_STATE_QUEUED);
		return ret;
	}
	m2_chain_init(ctx);
	ctx->frame = 0;
	ctx->cur = 0;
	ctx->budget = 0;
	ctx->qp = ctx->qp_i;
	return 0;
}

static void m2_stop_streaming(struct vb2_queue *q)
{
	struct m2_ctx *ctx = vb2_get_drv_priv(q);

	m2_return_all(ctx, q->type, VB2_BUF_STATE_ERROR);
	if (V4L2_TYPE_IS_OUTPUT(q->type))
		m2_free_all(ctx);
}

static const struct vb2_ops m2_vb2_ops = {
	.queue_setup = m2_queue_setup,
	.buf_out_validate = m2_buf_out_validate,
	.buf_prepare = m2_buf_prepare,
	.buf_queue = m2_buf_queue,
	.start_streaming = m2_start_streaming,
	.stop_streaming = m2_stop_streaming,
};

static int m2_queue_init(void *priv, struct vb2_queue *src,
			 struct vb2_queue *dst)
{
	struct m2_ctx *ctx = priv;
	struct vb2_queue *q[] = { src, dst };
	int i, ret;

	for (i = 0; i < 2; i++) {
		q[i]->type = i ? V4L2_BUF_TYPE_VIDEO_CAPTURE :
				 V4L2_BUF_TYPE_VIDEO_OUTPUT;
		q[i]->io_modes = VB2_MMAP | VB2_DMABUF;
		q[i]->drv_priv = ctx;
		q[i]->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
		q[i]->ops = &m2_vb2_ops;
		q[i]->mem_ops = &vb2_dma_contig_memops;
		q[i]->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
		q[i]->lock = &ctx->m2->lock;
		q[i]->dev = ctx->m2->dev;
		ret = vb2_queue_init(q[i]);
		if (ret)
			return ret;
	}
	return 0;
}

/* ---- ioctls ------------------------------------------------------------------------ */

static int m2_querycap(struct file *file, void *priv, struct v4l2_capability *c)
{
	strscpy(c->driver, "gm-mcp200", sizeof(c->driver));
	strscpy(c->card, "GM8126 H.264 encoder", sizeof(c->card));
	return 0;
}

static int m2_enum_fmt(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	if (f->index)
		return -EINVAL;
	f->pixelformat = V4L2_TYPE_IS_OUTPUT(f->type) ? V4L2_PIX_FMT_UYVY :
							V4L2_PIX_FMT_H264;
	return 0;
}

static void m2_fill_fmt(struct v4l2_pix_format *p, bool src)
{
	v4l_bound_align_image(&p->width, M2_MIN, M2_MAX_W, 4,
			      &p->height, M2_MIN, M2_MAX_H, 4, 0);
	p->field = V4L2_FIELD_NONE;
	if (src) {
		p->pixelformat = V4L2_PIX_FMT_UYVY;
		p->bytesperline = p->width * 2;
		p->sizeimage = p->bytesperline * p->height;
	} else {
		p->pixelformat = V4L2_PIX_FMT_H264;
		p->bytesperline = 0;
		/*
		 * The engine is not told where the buffer ends. A macroblock
		 * may take 400 bytes; twice that, and room for headers.
		 */
		p->sizeimage = p->width * p->height / 256 * 800 + 4096;
	}
}

static int m2_g_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	f->fmt.pix = *ctx_fmt(file_to_ctx(file), f->type);
	return 0;
}

static int m2_try_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct m2_ctx *ctx = file_to_ctx(file);
	const bool src = V4L2_TYPE_IS_OUTPUT(f->type);

	if (!src) {		/* the coded size is the source's */
		f->fmt.pix.width = ctx->fmt[M2_SRC].width;
		f->fmt.pix.height = ctx->fmt[M2_SRC].height;
		f->fmt.pix.colorspace = ctx->fmt[M2_SRC].colorspace;
	}
	m2_fill_fmt(&f->fmt.pix, src);
	return 0;
}

static int m2_s_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct m2_ctx *ctx = file_to_ctx(file);
	struct v4l2_pix_format *dst = &ctx->fmt[M2_DST];

	if (vb2_is_busy(v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type)))
		return -EBUSY;
	if (V4L2_TYPE_IS_OUTPUT(f->type) &&
	    vb2_is_busy(v4l2_m2m_get_vq(ctx->fh.m2m_ctx,
					V4L2_BUF_TYPE_VIDEO_CAPTURE)))
		return -EBUSY;
	m2_try_fmt(file, priv, f);
	*ctx_fmt(ctx, f->type) = f->fmt.pix;
	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		dst->width = f->fmt.pix.width;
		dst->height = f->fmt.pix.height;
		dst->colorspace = f->fmt.pix.colorspace;
		m2_fill_fmt(dst, false);
	}
	return 0;
}

static int m2_enum_framesizes(struct file *file, void *priv,
			      struct v4l2_frmsizeenum *f)
{
	if (f->index || (f->pixel_format != V4L2_PIX_FMT_UYVY &&
			 f->pixel_format != V4L2_PIX_FMT_H264))
		return -EINVAL;
	f->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	f->stepwise.min_width = M2_MIN;
	f->stepwise.max_width = M2_MAX_W;
	f->stepwise.step_width = 16;
	f->stepwise.min_height = M2_MIN;
	f->stepwise.max_height = M2_MAX_H;
	f->stepwise.step_height = 16;
	return 0;
}

static int m2_g_parm(struct file *file, void *priv, struct v4l2_streamparm *p)
{
	struct m2_ctx *ctx = file_to_ctx(file);

	if (!V4L2_TYPE_IS_OUTPUT(p->type))
		return -EINVAL;
	p->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
	p->parm.output.timeperframe = ctx->interval;
	return 0;
}

static int m2_s_parm(struct file *file, void *priv, struct v4l2_streamparm *p)
{
	struct m2_ctx *ctx = file_to_ctx(file);
	struct v4l2_fract *t = &p->parm.output.timeperframe;

	if (!V4L2_TYPE_IS_OUTPUT(p->type))
		return -EINVAL;
	if (!t->numerator || !t->denominator ||
	    t->denominator > 120 * t->numerator || t->denominator > 120000)
		*t = (struct v4l2_fract){ 1, 30 };
	ctx->interval = *t;
	return m2_g_parm(file, priv, p);
}

static const struct v4l2_ioctl_ops m2_ioctl_ops = {
	.vidioc_querycap = m2_querycap,
	.vidioc_enum_fmt_vid_cap = m2_enum_fmt,
	.vidioc_enum_fmt_vid_out = m2_enum_fmt,
	.vidioc_g_fmt_vid_cap = m2_g_fmt,
	.vidioc_g_fmt_vid_out = m2_g_fmt,
	.vidioc_try_fmt_vid_cap = m2_try_fmt,
	.vidioc_try_fmt_vid_out = m2_try_fmt,
	.vidioc_s_fmt_vid_cap = m2_s_fmt,
	.vidioc_s_fmt_vid_out = m2_s_fmt,
	.vidioc_enum_framesizes = m2_enum_framesizes,
	.vidioc_g_parm = m2_g_parm,
	.vidioc_s_parm = m2_s_parm,
	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf = v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf = v4l2_m2m_ioctl_dqbuf,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,
	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,
	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static int m2_s_ctrl(struct v4l2_ctrl *c)
{
	struct m2_ctx *ctx = container_of(c->handler, struct m2_ctx, hdl);

	switch (c->id) {
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		ctx->bitrate = c->val;
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
		ctx->rate_control = c->val;
		break;
	case V4L2_CID_MPEG_VIDEO_GOP_SIZE:
		ctx->gop = c->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
		ctx->qp_i = c->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP:
		ctx->qp_p = c->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
		ctx->qp_min = c->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
		ctx->qp_max = c->val;
		break;
	case V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME:
		ctx->force_idr = true;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct v4l2_ctrl_ops m2_ctrl_ops = {
	.s_ctrl = m2_s_ctrl,
};

static int m2_init_ctrls(struct m2_ctx *ctx)
{
	struct v4l2_ctrl_handler *h = &ctx->hdl;
	const struct v4l2_ctrl_ops *o = &m2_ctrl_ops;

	v4l2_ctrl_handler_init(h, 10);
	v4l2_ctrl_new_std(h, o, V4L2_CID_MPEG_VIDEO_BITRATE, 32000, 8000000, 1,
			  1024000);
	v4l2_ctrl_new_std(h, o, V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE, 0, 1, 1, 1);
	v4l2_ctrl_new_std(h, o, V4L2_CID_MPEG_VIDEO_GOP_SIZE, 0, 600, 1, 30);
	v4l2_ctrl_new_std(h, o, V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP, 10, 51, 1, 30);
	v4l2_ctrl_new_std(h, o, V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP, 10, 51, 1, 30);
	v4l2_ctrl_new_std(h, o, V4L2_CID_MPEG_VIDEO_H264_MIN_QP, 10, 51, 1, 10);
	v4l2_ctrl_new_std(h, o, V4L2_CID_MPEG_VIDEO_H264_MAX_QP, 10, 51, 1, 48);
	v4l2_ctrl_new_std(h, o, V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME, 0, 0, 0, 0);
	v4l2_ctrl_new_std_menu(h, o, V4L2_CID_MPEG_VIDEO_H264_PROFILE,
			       V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE,
			       ~BIT(V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE),
			       V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE);
	if (h->error) {
		int ret = h->error;

		v4l2_ctrl_handler_free(h);
		return ret;
	}
	ctx->fh.ctrl_handler = h;
	return v4l2_ctrl_handler_setup(h);
}

static int m2_open(struct file *file)
{
	struct m2_dev *m2 = video_drvdata(file);
	struct m2_ctx *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	ctx->m2 = m2;
	ctx->interval = (struct v4l2_fract){ 1, 30 };
	ctx->fmt[M2_SRC].width = ctx->fmt[M2_DST].width = 640;
	ctx->fmt[M2_SRC].height = ctx->fmt[M2_DST].height = 480;
	m2_fill_fmt(&ctx->fmt[M2_SRC], true);
	m2_fill_fmt(&ctx->fmt[M2_DST], false);
	ctx->fmt[M2_SRC].colorspace = V4L2_COLORSPACE_SMPTE170M;
	ctx->fmt[M2_DST].colorspace = V4L2_COLORSPACE_SMPTE170M;

	v4l2_fh_init(&ctx->fh, &m2->vdev);
	ret = m2_init_ctrls(ctx);
	if (ret)
		goto err_fh;
	ctx->force_idr = false;
	ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(m2->m2m, ctx, m2_queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		goto err_ctrl;
	}
	v4l2_fh_add(&ctx->fh, file);
	return 0;

err_ctrl:
	v4l2_ctrl_handler_free(&ctx->hdl);
err_fh:
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);
	return ret;
}

static int m2_release(struct file *file)
{
	struct m2_dev *m2 = video_drvdata(file);
	struct m2_ctx *ctx = file_to_ctx(file);

	mutex_lock(&m2->lock);
	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
	mutex_unlock(&m2->lock);
	v4l2_fh_del(&ctx->fh, file);
	v4l2_ctrl_handler_free(&ctx->hdl);
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);
	return 0;
}

static const struct v4l2_file_operations m2_fops = {
	.owner = THIS_MODULE,
	.open = m2_open,
	.release = m2_release,
	.poll = v4l2_m2m_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = v4l2_m2m_fop_mmap,
};

static int m2_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct video_device *v;
	struct m2_dev *m2;
	int irq, ret;

	m2 = devm_kzalloc(dev, sizeof(*m2), GFP_KERNEL);
	if (!m2)
		return -ENOMEM;
	m2->dev = dev;
	mutex_init(&m2->lock);
	INIT_DELAYED_WORK(&m2->watchdog, m2_watchdog);
	platform_set_drvdata(pdev, m2);
	m2->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(m2->base))
		return PTR_ERR(m2->base);
	m2->wrap = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(m2->wrap))
		return PTR_ERR(m2->wrap);
	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;
	m2->clk = devm_clk_get_enabled(dev, NULL);
	if (IS_ERR(m2->clk))
		return dev_err_probe(dev, PTR_ERR(m2->clk), "clock\n");
	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;
	m2_hw_init(m2);
	ret = devm_request_threaded_irq(dev, irq, m2_irq, m2_irq_thread, 0,
					dev_name(dev), m2);
	if (ret)
		return ret;

	ret = v4l2_device_register(dev, &m2->v4l2);
	if (ret)
		return ret;
	m2->m2m = v4l2_m2m_init(&m2_m2m_ops);
	if (IS_ERR(m2->m2m)) {
		ret = PTR_ERR(m2->m2m);
		goto err_v4l2;
	}
	v = &m2->vdev;
	strscpy(v->name, "gm-mcp200", sizeof(v->name));
	v->fops = &m2_fops;
	v->ioctl_ops = &m2_ioctl_ops;
	v->release = video_device_release_empty;
	v->vfl_dir = VFL_DIR_M2M;
	v->lock = &m2->lock;
	v->v4l2_dev = &m2->v4l2;
	v->device_caps = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;
	video_set_drvdata(v, m2);
	ret = video_register_device(v, VFL_TYPE_VIDEO, -1);
	if (ret)
		goto err_m2m;
	dev_info(dev, "H.264 encoder on %s\n", video_device_node_name(v));
	return 0;

err_m2m:
	v4l2_m2m_release(m2->m2m);
err_v4l2:
	v4l2_device_unregister(&m2->v4l2);
	return ret;
}

static void m2_remove(struct platform_device *pdev)
{
	struct m2_dev *m2 = platform_get_drvdata(pdev);

	video_unregister_device(&m2->vdev);
	cancel_delayed_work_sync(&m2->watchdog);
	v4l2_m2m_release(m2->m2m);
	v4l2_device_unregister(&m2->v4l2);
}

static const struct of_device_id m2_of_match[] = {
	{ .compatible = "grainmedia,gm8126-mcp200" },
	{ }
};
MODULE_DEVICE_TABLE(of, m2_of_match);

static struct platform_driver m2_driver = {
	.probe = m2_probe,
	.remove = m2_remove,
	.driver = {
		.name = "gm-mcp200",
		.of_match_table = m2_of_match,
	},
};
module_platform_driver(m2_driver);

MODULE_DESCRIPTION("Grain Media GM8126 H.264 encoder driver");
MODULE_LICENSE("GPL");
