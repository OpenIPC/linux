// SPDX-License-Identifier: GPL-2.0
/*
 * FH8626 PAE H.264 encoder, V4L2 mem2mem driver.
 *
 * The device has three register windows: core registers, a working buffer
 * window and the motion estimation SRAM.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/kref.h>
#include <linux/of.h>
#include <linux/gcd.h>
#include <linux/math64.h>
#include <linux/workqueue.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

#include "fh8626-encoder-regs.h"
#include "fh8626-pae-hw.h"
#include "fh8626-event.h"

#define FH8626_ENC_NAME		"fh8626-encoder"

/*
 * Core clock, bus clock and the two gates of the engine clock domain. The
 * engine accepts register writes with the first two running. Frame coding
 * and reset acknowledgment require all four clocks.
 */
static const char * const fh8626_enc_clk_names[] = {
	"pae_clk", "pae_hclk_gate", "multi_pae_clk", "pae_adpt_clk",
};

#define FH8626_ENC_NUM_CLKS	ARRAY_SIZE(fh8626_enc_clk_names)

enum {
	FH8626_QUEUE_OUTPUT = 0,	/* NV12 input */
	FH8626_QUEUE_CAPTURE = 1,	/* H.264 output */
};

struct fh8626_enc_fmt {
	u32	fourcc;
	u32	type;
};

static const struct fh8626_enc_fmt fh8626_enc_formats[] = {
	{
		.fourcc = V4L2_PIX_FMT_NV12,
		.type = V4L2_BUF_TYPE_VIDEO_OUTPUT,
	},
	{
		.fourcc = V4L2_PIX_FMT_H264,
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
	},
};

struct fh8626_enc_q_data {
	u32				width;
	u32				height;
	u32				stride;
	u32				sizeimage;
	const struct fh8626_enc_fmt	*fmt;
};

struct fh8626_enc_dev {
	struct kref		ref;
	struct list_head	contexts;
	bool			disconnected;
	struct device		*dev;
	struct v4l2_device	v4l2_dev;
	struct video_device	vfd;
	struct v4l2_m2m_dev	*m2m_dev;

	void __iomem		*core_regs;
	void __iomem		*buf_regs;
	void __iomem		*sram_base;
	int			irq;

	struct clk		*clks[FH8626_ENC_NUM_CLKS];

	struct mutex		dev_mutex;
	struct workqueue_struct *workqueue;
	struct fh8626_pae_hw *hw;
	bool irq_registered;
	struct dentry		*debugfs;
};

enum {
	FH8626_RC_I = 0,
	FH8626_RC_P = 1,
};

/* Frames in the one-second peak window, at most. */
#define FH8626_RC_WINDOW	64
/* Maximum IDR retries; also used for undersized frames. */
#define FH8626_RC_RETRIES	3
/* I frames are planned this many QP steps finer than P frames. */
#define FH8626_RC_I_OFFSET	2

/*
 * Per-slice-type complexity: log2(bits) + qp / 6, in Q8.
 * The model assumes coded size halves every six QP steps.
 */
struct fh8626_enc_model {
	int	avg;
	int	last;
	int	jumps;	/* consecutive large changes, negative downwards */
	bool	valid;
};

/*
 * Frame-level rate control; sizes are in bits. A GOP error integral tracks
 * average bitrate; a leaky bucket and sliding window constrain frame budgets.
 */
struct fh8626_enc_rc {
	struct fh8626_enc_model model[2];
	int	qp_p;		/* QP of the latest coded P frame, or -1 */
	s64	abr_error;	/* coded minus budgeted bits */
	s64	planned;	/* abr_error the GOP plan expects by now */
	int	bias;		/* QP offset for lasting model error, Q8 */
	u64	vbv_fill;	/* encoder buffer level after the last frame */
	u32	window[FH8626_RC_WINDOW];	/* recent frame sizes */
	unsigned int window_pos;
	bool	can_skip;	/* the decoder holds a reference picture */
	u32	frame_bits;	/* average budget of one frame */
	u32	drain_bits;	/* buffer drain per frame */
	u32	nominal;	/* this frame's share of the GOP plan */
	u64	room;		/* bits the frame may take without overflow */
	u64	full;		/* room when the buffer is empty */
	unsigned int window_len;	/* frames in one second */
	unsigned int retries;
	int	try_qp;		/* previous attempt at this frame, or -1 */
	u64	try_bits;
	u32	skip_run;	/* frames skipped in a row */
};

struct fh8626_enc_ctx {
	struct list_head		list;
	struct v4l2_fh			fh;
	struct fh8626_enc_dev		*dev;
	struct v4l2_m2m_ctx		*m2m_ctx;
	struct fh8626_enc_q_data	q_data[2];
	struct fh8626_pae_ctx		*pae;

	struct v4l2_ctrl_handler	ctrl_handler;
	u32				qp_i;
	u32				qp_p;
	u32				gop_size;
	u32				bitrate_mode;
	u32				bitrate;
	u32				bitrate_peak;
	u32				min_qp;
	u32				max_qp;
	u32				cpb_size;	/* KiB, 0 auto */
	bool				frame_rc;
	u32				lf_mode;
	s32				lf_alpha;
	s32				lf_beta;
	bool				sar_enable;
	u32				sar_idc;
	u32				sar_width;
	u32				sar_height;
	bool				force_key_frame;
	struct v4l2_fract		timeperframe;

	/* Header settings latched at each IDR so P slices match the PPS. */
	struct fh8626_pae_deblock	deblock;
	struct fh8626_pae_vui		vui;
	/* Deblocking settings of the PPS last sent, for skipped frames. */
	struct fh8626_pae_deblock	sent_deblock;

	struct fh8626_enc_rc		rc;
	u32				gop_count;
	bool				idr_pending;
	u16				frame_num;
	u16				poc;
	/* Frames coded by the engine; selects its macroblock buffers. */
	u32				hw_frames;

	u32 colorspace;
	u32 xfer_func;
	u32 ycbcr_enc;
	u32 quantization;
	struct work_struct work;
	spinlock_t state_lock;
	bool streaming[2];
	bool aborting;
	bool draining;
	bool drained;
	bool eos_pending;
	unsigned int drain_remaining;

	u32				sequence_out;
	u32				sequence_cap;
};

/*
 * Default to four bits per pixel to limit contiguous CMA allocations.
 * Userspace may request up to one byte per pixel.
 */
static u32 fh8626_enc_coded_size(u32 width, u32 height)
{
	return ALIGN(width * height / 2, 4096);
}

static inline struct fh8626_enc_ctx *fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct fh8626_enc_ctx, fh);
}

static inline struct fh8626_enc_q_data *get_q_data(struct fh8626_enc_ctx *ctx,
						   enum v4l2_buf_type type)
{
	if (V4L2_TYPE_IS_OUTPUT(type))
		return &ctx->q_data[FH8626_QUEUE_OUTPUT];
	return &ctx->q_data[FH8626_QUEUE_CAPTURE];
}

static irqreturn_t fh8626_enc_isr(int irq, void *data)
{
	struct fh8626_enc_dev *dev = data;

	return dev->hw ? fh8626_pae_irq(dev->hw) : IRQ_NONE;
}

static void fh8626_enc_eos(struct fh8626_enc_ctx *ctx)
{
	struct v4l2_event event = { .type = V4L2_EVENT_EOS };

	v4l2_event_queue_fh(&ctx->fh, &event);
}

/* state_lock serializes publishing LAST/EOS with a subsequent START. */
static void fh8626_enc_empty_last(struct fh8626_enc_ctx *ctx,
				 struct vb2_v4l2_buffer *dst)
{
	dst->sequence = ctx->sequence_cap++;
	dst->field = V4L2_FIELD_NONE;
	dst->vb2_buf.timestamp = 0;
	dst->flags &= ~(V4L2_BUF_FLAG_KEYFRAME | V4L2_BUF_FLAG_PFRAME |
			V4L2_BUF_FLAG_BFRAME |
			V4L2_BUF_FLAG_TSTAMP_SRC_MASK);
	dst->flags |= V4L2_BUF_FLAG_LAST;
	vb2_set_plane_payload(&dst->vb2_buf, 0, 0);
	v4l2_m2m_buf_done(dst, VB2_BUF_STATE_DONE);
	fh8626_enc_eos(ctx);
}

/* H.264 Annex E code points for the V4L2 colorimetry of the input. */
static u8 fh8626_enc_vui_primaries(u32 colorspace)
{
	switch (colorspace) {
	case V4L2_COLORSPACE_REC709:
	case V4L2_COLORSPACE_SRGB:
	case V4L2_COLORSPACE_JPEG:
		return 1;
	case V4L2_COLORSPACE_470_SYSTEM_M:
		return 4;
	case V4L2_COLORSPACE_470_SYSTEM_BG:
		return 5;
	case V4L2_COLORSPACE_SMPTE170M:
		return 6;
	case V4L2_COLORSPACE_SMPTE240M:
		return 7;
	case V4L2_COLORSPACE_BT2020:
		return 9;
	case V4L2_COLORSPACE_DCI_P3:
		return 11;
	default:
		return 2;
	}
}

static u8 fh8626_enc_vui_transfer(u32 xfer_func)
{
	switch (xfer_func) {
	case V4L2_XFER_FUNC_709:
		return 1;
	case V4L2_XFER_FUNC_SMPTE240M:
		return 7;
	case V4L2_XFER_FUNC_NONE:
		return 8;
	case V4L2_XFER_FUNC_SRGB:
		return 13;
	case V4L2_XFER_FUNC_SMPTE2084:
		return 16;
	default:
		return 2;
	}
}

static u8 fh8626_enc_vui_matrix(u32 ycbcr_enc)
{
	switch (ycbcr_enc) {
	case V4L2_YCBCR_ENC_709:
	case V4L2_YCBCR_ENC_XV709:
		return 1;
	case V4L2_YCBCR_ENC_601:
	case V4L2_YCBCR_ENC_XV601:
		return 6;
	case V4L2_YCBCR_ENC_SMPTE240M:
		return 7;
	case V4L2_YCBCR_ENC_BT2020:
		return 9;
	case V4L2_YCBCR_ENC_BT2020_CONST_LUM:
		return 10;
	default:
		return 2;
	}
}

/* Called for each IDR; the SPS/PPS sent with it describe the whole GOP. */
static void fh8626_enc_latch_headers(struct fh8626_enc_ctx *ctx)
{
	struct fh8626_pae_deblock *deblock = &ctx->deblock;
	struct fh8626_pae_vui *vui = &ctx->vui;
	u32 idc;

	deblock->disable_idc = READ_ONCE(ctx->lf_mode);
	deblock->alpha_c0_offset_div2 = READ_ONCE(ctx->lf_alpha);
	deblock->beta_offset_div2 = READ_ONCE(ctx->lf_beta);
	deblock->control_present = deblock->disable_idc ||
				   deblock->alpha_c0_offset_div2 ||
				   deblock->beta_offset_div2;

	memset(vui, 0, sizeof(*vui));
	vui->aspect_ratio_present = READ_ONCE(ctx->sar_enable);
	idc = READ_ONCE(ctx->sar_idc);
	if (idc == V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_EXTENDED) {
		vui->aspect_ratio_idc = FH8626_PAE_SAR_EXTENDED;
		vui->sar_width = READ_ONCE(ctx->sar_width);
		vui->sar_height = READ_ONCE(ctx->sar_height);
	} else {
		vui->aspect_ratio_idc = idc;
	}
	vui->video_signal_present = true;
	vui->video_format = 5; /* unspecified */
	vui->full_range = ctx->quantization == V4L2_QUANTIZATION_FULL_RANGE;
	vui->colour_description_present = true;
	vui->colour_primaries = fh8626_enc_vui_primaries(ctx->colorspace);
	vui->transfer_characteristics =
		fh8626_enc_vui_transfer(ctx->xfer_func);
	vui->matrix_coefficients = fh8626_enc_vui_matrix(ctx->ycbcr_enc);
	/* One frame lasts two ticks of a progressive sequence. */
	vui->timing_present = true;
	vui->num_units_in_tick = ctx->timeperframe.numerator;
	vui->time_scale = 2 * ctx->timeperframe.denominator;
	/* Capture may drop frames, so the rate is nominal only. */
	vui->fixed_frame_rate = false;
}

/* Approximate log2(x), in Q8. */
static int fh8626_enc_log2(u64 x)
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

/* Approximate 2^(x / 256), clamped to at least 1. */
static u64 fh8626_enc_exp2(int x)
{
	u32 f;
	u64 m;

	if (x < 0)
		return 1;
	x = min(x, 47 * 256);
	f = x & 255;
	/* 2^t ~= 1 + t * (0.6565 + 0.3435 * t) for t in [0, 1), Q16 */
	m = 65536 + ((f * (43024 + ((22512 * f) >> 8))) >> 8);
	return max_t(u64, (m << (x >> 8)) >> 16, 1);
}

static int fh8626_enc_complexity(u64 bits, int qp)
{
	return fh8626_enc_log2(max_t(u64, bits, 1)) +
	       DIV_ROUND_CLOSEST(qp * 128, 3);
}

/* Expected size of a frame of the given complexity at qp. */
static u64 fh8626_enc_model_bits(int complexity, int qp)
{
	return fh8626_enc_exp2(complexity - DIV_ROUND_CLOSEST(qp * 128, 3));
}

/* QP at which a frame of the given complexity is expected to cost bits. */
static int fh8626_enc_model_qp(int complexity, u64 bits)
{
	return DIV_ROUND_CLOSEST((complexity -
				  fh8626_enc_log2(max_t(u64, bits, 1))) * 3,
				 128);
}

/*
 * Initial I-frame QP by budget; entry i corresponds to log2(bpp) = i - 6.
 */
static const u8 fh8626_enc_bpp_qp[] = { 45, 41, 37, 33, 29, 25, 21, 17, 13 };

static int fh8626_enc_start_qp(u64 bits, u32 pixels)
{
	const unsigned int last = ARRAY_SIZE(fh8626_enc_bpp_qp) - 1;
	int l = fh8626_enc_log2(max_t(u64, bits, 1)) -
		fh8626_enc_log2(max(pixels, 1U)) + 6 * 256;
	unsigned int i;

	if (l <= 0)
		return fh8626_enc_bpp_qp[0];
	i = l >> 8;
	if (i >= last)
		return fh8626_enc_bpp_qp[last];
	return fh8626_enc_bpp_qp[i] -
	       DIV_ROUND_CLOSEST((fh8626_enc_bpp_qp[i] -
				  fh8626_enc_bpp_qp[i + 1]) * (l & 255), 256);
}

static void fh8626_enc_rc_reset(struct fh8626_enc_ctx *ctx)
{
	struct fh8626_enc_rc *rc = &ctx->rc;

	memset(rc, 0, sizeof(*rc));
	rc->qp_p = -1;
	rc->try_qp = -1;
}

static u64 fh8626_enc_rc_recent(const struct fh8626_enc_rc *rc,
				unsigned int count)
{
	unsigned int i, pos = rc->window_pos;
	u64 sum = 0;

	for (i = 0; i < count; i++) {
		pos = (pos + FH8626_RC_WINDOW - 1) % FH8626_RC_WINDOW;
		sum += rc->window[pos];
	}
	return sum;
}

/*
 * Allow skips while buffered data exceeds one second of 64-byte skip
 * frames. Limit consecutive skips to four seconds at the nominal rate.
 */
static bool fh8626_enc_rc_draining(const struct fh8626_enc_rc *rc)
{
	return rc->can_skip && rc->skip_run < 4 * rc->window_len &&
	       rc->room + (u64)rc->window_len * 512 < rc->full;
}

/*
 * Plan I/P QPs jointly against the GOP budget, correcting accumulated error
 * over one second (CBR) or two (VBR). Buffer limits override QP step limits.
 * Return -1 to skip when max_qp cannot fit and buffered data can drain.
 */
static int fh8626_enc_rc_plan(struct fh8626_enc_ctx *ctx, bool idr, u32 gop,
			      size_t capacity)
{
	struct fh8626_enc_rc *rc = &ctx->rc;
	struct fh8626_enc_model *mi = &rc->model[FH8626_RC_I];
	struct fh8626_enc_model *mp = &rc->model[FH8626_RC_P];
	struct fh8626_enc_model *m = idr ? mi : mp;
	const struct fh8626_enc_q_data *raw =
		&ctx->q_data[FH8626_QUEUE_OUTPUT];
	u32 num = ctx->timeperframe.numerator;
	u32 den = ctx->timeperframe.denominator;
	u32 bitrate = READ_ONCE(ctx->bitrate);
	u32 peak = max(READ_ONCE(ctx->bitrate_peak), bitrate);
	u32 cpb = READ_ONCE(ctx->cpb_size);
	bool cbr = READ_ONCE(ctx->bitrate_mode) ==
		   V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;
	int min_qp = READ_ONCE(ctx->min_qp);
	int max_qp = READ_ONCE(ctx->max_qp);
	int i_offset = DIV_ROUND_CLOSEST(FH8626_RC_I_OFFSET * 128, 3);
	unsigned int n, g = max(gop, 1U);
	u64 limit, vbv, full, recent, room, gop_bits;
	int qp, qp_gop, ci, cp, cg, cons, r;
	s64 err, horizon, f;

	min_qp = min(min_qp, max_qp);
	rc->frame_bits = max_t(u64, div_u64((u64)bitrate * num, den), 1);
	rc->drain_bits = max_t(u64, div_u64((u64)(cbr ? bitrate : peak) * num,
					    den), 1);
	n = clamp_t(u32, DIV_ROUND_CLOSEST(den, num), 1, FH8626_RC_WINDOW);

	/*
	 * Room left in the one-second window and in the buffer. A CBR window
	 * may run an eighth over the bitrate so that a GOP spending exactly
	 * its budget still leaves its I frame some room.
	 */
	limit = (u64)rc->drain_bits * n;
	vbv = cpb ? (u64)cpb * 8192 : limit;
	if (cbr)
		limit += limit / 8;
	full = min3(limit, vbv, (u64)capacity * 8 * 15 / 16);
	recent = fh8626_enc_rc_recent(rc, n - 1);
	room = limit > recent ? limit - recent : 0;
	room = min3(room, vbv > rc->vbv_fill ? vbv - rc->vbv_fill : 0, full);
	rc->room = room;
	rc->full = full;
	rc->window_len = n;
	rc->retries = 0;
	rc->try_qp = -1;
	if (idr)
		rc->planned = 0;

	if (!READ_ONCE(ctx->frame_rc)) {
		rc->nominal = rc->frame_bits;
		return idr ? READ_ONCE(ctx->qp_i) : READ_ONCE(ctx->qp_p);
	}

	if (!mi->valid && !mp->valid) {
		gop_bits = div_u64((u64)rc->frame_bits * g * 4, g + 3);
		rc->nominal = min_t(u64, gop_bits, U32_MAX);
		qp = fh8626_enc_start_qp(min(gop_bits, room * 3 / 4),
					 raw->width * raw->height);
		return clamp(qp, min_qp, max_qp);
	}

	/* Without a model of its own, a P frame costs a quarter of an I. */
	ci = mi->valid ? mi->avg : mp->avg + 2 * 256;
	cp = mp->valid ? mp->avg : mi->avg - 2 * 256;
	/* The whole GOP as one complexity at the P frame QP. */
	cg = ci + i_offset;
	if (g > 1)
		cg = fh8626_enc_log2(fh8626_enc_exp2(cg) +
				     (u64)(g - 1) * fh8626_enc_exp2(cp));
	qp_gop = fh8626_enc_model_qp(cg, (u64)rc->frame_bits * g);
	rc->nominal = min_t(u64, idr ?
			    fh8626_enc_model_bits(ci, qp_gop -
						  FH8626_RC_I_OFFSET) :
			    fh8626_enc_model_bits(cp, qp_gop), U32_MAX);

	err = rc->abr_error - rc->planned;
	horizon = (s64)rc->frame_bits * n * (cbr ? 1 : 2);
	f = clamp_t(s64, 256 - div64_s64(err * 256, horizon), 64, 1024);
	qp = qp_gop - DIV_ROUND_CLOSEST((fh8626_enc_log2(f) - 8 * 256) * 3,
					128);
	/*
	 * An error that persists beyond an eighth of the horizon is a model
	 * bias; fold it into a QP offset at up to 4 steps per second.
	 */
	r = clamp_t(s64, div64_s64(err * 256, horizon), -256, 256);
	if (abs(r) > 32)
		rc->bias = clamp(rc->bias + r * 4 / (int)n,
				 -12 * 256, 12 * 256);
	if (idr)
		qp -= FH8626_RC_I_OFFSET;
	qp += DIV_ROUND_CLOSEST(rc->bias, 256);
	if (rc->qp_p >= 0) {
		int base = rc->qp_p - (idr ? FH8626_RC_I_OFFSET : 0);
		int follow = base + 4;

		/*
		 * When the latest P frame alone would take a quarter of the
		 * room, rise faster towards the QP it calls for.
		 */
		if (!idr && mp->valid &&
		    fh8626_enc_model_bits(mp->last, rc->qp_p) > room / 4)
			follow = clamp(fh8626_enc_model_qp(mp->last,
					div_u64(rc->nominal * f, 256)),
				       follow, base + 8);
		qp = idr ? clamp(qp, base - 3, base + 3) :
			   clamp(qp, base - 2, follow);
	}

	cons = m->valid ? max(m->avg, m->last) : (idr ? ci : cp);
	if (idr && mp->valid)
		cons = max(cons, mp->last);
	qp = max(qp, fh8626_enc_model_qp(cons, idr ? room * 3 / 4 : room / 2));
	qp = clamp(qp, min_qp, max_qp);

	if (fh8626_enc_rc_draining(rc) &&
	    fh8626_enc_model_bits(cons, max_qp) > room)
		return -1;
	return qp;
}

/*
 * Judge a coded frame against the room planned for it. Returns 0 to send
 * it, another QP to code an IDR frame again, or -1 to drop the frame. The
 * engine has already overwritten its reference with a dropped frame, so the
 * caller sends a skipped frame in its place and codes an IDR frame next.
 */
static int fh8626_enc_rc_check(struct fh8626_enc_ctx *ctx, bool idr, int qp,
			       u64 bits, bool overrun)
{
	struct fh8626_enc_rc *rc = &ctx->rc;
	int max_qp = READ_ONCE(ctx->max_qp);
	int min_qp = min_t(int, READ_ONCE(ctx->min_qp), max_qp);
	int slope = 6 * 256, step, d;

	if (!READ_ONCE(ctx->frame_rc))
		return 0;
	if (!overrun && bits <= rc->room) {
		int c, q;

		/*
		 * An IDR frame far below its plan, as after a change to easier
		 * content, is coded once more at the QP its size calls for.
		 */
		if (!idr || rc->retries || bits >= rc->nominal / 8 ||
		    qp <= min_qp)
			return 0;
		c = fh8626_enc_complexity(bits, qp);
		q = max(fh8626_enc_model_qp(c, rc->nominal),
			fh8626_enc_model_qp(c, rc->room * 3 / 4));
		/* 0 would read as "send it". */
		q = clamp(q, max3(min_qp, qp - 24, 1), qp);
		if (q == qp)
			return 0;
		rc->try_qp = qp;
		rc->try_bits = bits;
		rc->retries++;
		return q;
	}
	if (!idr && rc->can_skip)
		return -1;
	if (idr && rc->retries < FH8626_RC_RETRIES && qp < max_qp) {
		/* QP steps per halving, from the previous attempt if any. */
		d = fh8626_enc_log2(rc->try_bits) - fh8626_enc_log2(bits);
		if (rc->try_qp >= 0 && qp != rc->try_qp &&
		    (qp > rc->try_qp) == (d > 0) && d)
			slope = clamp((qp - rc->try_qp) * 65536 / d,
				      4 * 256, 16 * 256);
		d = fh8626_enc_log2(bits) -
		    fh8626_enc_log2(max_t(u64, rc->room * 3 / 4, 1));
		step = DIV_ROUND_UP(max(d, 1) * slope, 65536);
		rc->try_qp = qp;
		rc->try_bits = bits;
		rc->retries++;
		return min(qp + step, max_qp);
	}
	if (fh8626_enc_rc_draining(rc))
		return -1;
	dev_warn_ratelimited(ctx->dev->dev,
			     "%llu-bit frame at QP %d exceeds the %llu bits left by the rate limit\n",
			     bits, qp, rc->room);
	return 0;
}

static void fh8626_enc_rc_learn(struct fh8626_enc_ctx *ctx, bool idr, int qp,
				u64 bits)
{
	struct fh8626_enc_rc *rc = &ctx->rc;
	struct fh8626_enc_model *mi = &rc->model[FH8626_RC_I];
	struct fh8626_enc_model *m = idr ? mi : &rc->model[FH8626_RC_P];
	int c = fh8626_enc_complexity(bits, qp);
	int d = c - m->avg;

	m->last = c;
	if (!m->valid) {
		m->avg = c;
		m->jumps = 0;
		m->valid = true;
		return;
	}
	/* Within a factor of 2.8, average out frame-to-frame variation. */
	if (abs(d) <= 384) {
		m->jumps = 0;
		m->avg += idr ? d / 2 : d / 4;
		return;
	}
	m->jumps = d > 0 ? max(m->jumps, 0) + 1 : min(m->jumps, 0) - 1;
	if (!idr && abs(m->jumps) < 2) {
		m->avg += d / 8;
		return;
	}
	m->avg = c;
	m->jumps = 0;
	if (!idr && mi->valid)
		mi->avg += d;
}

static void fh8626_enc_rc_account(struct fh8626_enc_ctx *ctx, bool idr,
				  int qp, u64 bits, bool coded)
{
	struct fh8626_enc_rc *rc = &ctx->rc;
	bool cbr = READ_ONCE(ctx->bitrate_mode) ==
		   V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;
	s64 second = (s64)rc->frame_bits * rc->window_len;

	rc->window[rc->window_pos] = min_t(u64, bits, U32_MAX);
	rc->window_pos = (rc->window_pos + 1) % FH8626_RC_WINDOW;
	rc->vbv_fill = rc->vbv_fill + bits > rc->drain_bits ?
		       rc->vbv_fill + bits - rc->drain_bits : 0;
	/* Bank at most half a second of credit; bound debt by rate mode. */
	rc->abr_error += (s64)bits - rc->frame_bits;
	rc->abr_error = clamp(rc->abr_error, -second / 2,
			      cbr ? second : 2 * second);
	if (!coded) {
		rc->skip_run++;
		return;
	}
	rc->skip_run = 0;
	rc->planned += (s64)rc->nominal - rc->frame_bits;
	if (!idr)
		rc->qp_p = qp;
	rc->can_skip = true;
}

/*
 * Include padding beyond the declared image plane in an imported dma-buf.
 * The buffer remains queued and mapped until the worker finishes or resets
 * the engine.
 */
static size_t fh8626_enc_extent(struct vb2_buffer *vb)
{
	size_t length = vb2_plane_size(vb, 0);

	if (vb->memory == VB2_MEMORY_DMABUF && vb->planes[0].dbuf)
		length = max(length, vb->planes[0].dbuf->size);
	return length;
}

static bool fh8626_enc_direct(struct fh8626_enc_ctx *ctx,
			      struct vb2_buffer *vb)
{
	const struct fh8626_enc_q_data *raw = &ctx->q_data[FH8626_QUEUE_OUTPUT];

	return fh8626_pae_input_direct(raw->width, raw->height, raw->stride,
				       vb2_dma_contig_plane_dma_addr(vb, 0),
				       fh8626_enc_extent(vb));
}

static void fh8626_enc_work(struct work_struct *work)
{
	struct fh8626_enc_ctx *ctx =
		container_of(work, struct fh8626_enc_ctx, work);
	struct fh8626_enc_dev *dev = ctx->dev;
	struct vb2_v4l2_buffer *src, *dst;
	struct fh8626_pae_frame frame;
	unsigned long flags;
	bool last = false;
	bool is_idr = true;
	bool coded = false;
	u32 gop;
	int ret = -ECANCELED, qp, action;
	u64 bits;

	src = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	dst = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
	if (src && dst && ctx->pae && !READ_ONCE(ctx->aborting) &&
	    !READ_ONCE(dev->disconnected)) {
		gop = READ_ONCE(ctx->gop_size);
		/* A requested IDR frame waits while frames are skipped. */
		if (READ_ONCE(ctx->force_key_frame)) {
			WRITE_ONCE(ctx->force_key_frame, false);
			ctx->idr_pending = true;
		}
		if (gop <= 1 || ctx->gop_count == 0)
			ctx->idr_pending = true;
		is_idr = ctx->idr_pending;

		memset(&frame, 0, sizeof(frame));
		if (fh8626_enc_direct(ctx, &src->vb2_buf)) {
			frame.dma = vb2_dma_contig_plane_dma_addr(&src->vb2_buf,
								  0);
			frame.dma_length = fh8626_enc_extent(&src->vb2_buf);
		} else {
			frame.nv12 = vb2_plane_vaddr(&src->vb2_buf, 0);
		}
		frame.stream = vb2_plane_vaddr(&dst->vb2_buf, 0);
		frame.capacity = vb2_plane_size(&dst->vb2_buf, 0);
		frame.frame_num = ctx->frame_num;
		frame.poc = ctx->poc;
		frame.idr_pic_id = ctx->sequence_out & 0xffff;
		frame.mb_parity = ctx->hw_frames & 1;
		frame.vui = &ctx->vui;

		qp = fh8626_enc_rc_plan(ctx, is_idr, gop, frame.capacity);
		if (qp >= 0 && is_idr)
			fh8626_enc_latch_headers(ctx);
		frame.deblock = ctx->deblock;
		while (qp >= 0) {
			frame.qp = qp;
			frame.idr = is_idr;
			ret = fh8626_pae_encode_frame(ctx->pae, &frame,
						      &is_idr);
			if (ret <= 0 && ret != -ENOSPC) {
				/*
				 * Skipped frames would follow a lost picture.
				 */
				ctx->rc.can_skip = false;
				break;
			}
			bits = ret > 0 ? (u64)ret * 8 : (u64)frame.capacity * 8;
			fh8626_enc_rc_learn(ctx, is_idr, qp, bits);
			action = fh8626_enc_rc_check(ctx, is_idr, qp, bits,
						     ret < 0);
			if (action > 0) {
				qp = action;
				continue;
			}
			if (!action) {
				coded = ret > 0;
				if (!coded)
					ctx->rc.can_skip = false;
				break;
			}
			/* Drop the frame; decoders keep the old reference. */
			ctx->hw_frames++;
			ctx->idr_pending = true;
			qp = -1;
		}
		if (qp < 0) {
			frame.idr = false;
			frame.qp = ctx->rc.qp_p >= 0 ? ctx->rc.qp_p : 26;
			frame.deblock = ctx->sent_deblock;
			ret = fh8626_pae_skip_frame(ctx->pae, &frame);
			is_idr = false;
		}
		if (ret > 0) {
			fh8626_enc_rc_account(ctx, is_idr, frame.qp,
					      (u64)ret * 8, coded);
			if (coded)
				ctx->hw_frames++;
			if (coded && is_idr) {
				ctx->sent_deblock = frame.deblock;
				ctx->gop_count = 1;
				ctx->idr_pending = false;
				ctx->frame_num = 1;
				ctx->poc = 2;
			} else {
				ctx->gop_count = (ctx->gop_count + 1) %
						 max(1U, gop);
				/* Skipped frames are non-reference pictures. */
				if (coded)
					ctx->frame_num = (ctx->frame_num + 1) &
							 0xffff;
				ctx->poc = (ctx->poc + 2) & 0xffff;
			}
		}
	}
	spin_lock_irqsave(&ctx->state_lock, flags);
	src = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	dst = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
	if (ctx->draining && ctx->drain_remaining && !--ctx->drain_remaining) {
		ctx->drained = true;
		last = true;
	}
	if (READ_ONCE(ctx->aborting))
		ret = -ECANCELED;
	if (dst) {
		dst->sequence = ctx->sequence_cap++;
		dst->field = V4L2_FIELD_NONE;
		dst->flags &= ~(V4L2_BUF_FLAG_KEYFRAME |
				V4L2_BUF_FLAG_PFRAME |
				V4L2_BUF_FLAG_BFRAME | V4L2_BUF_FLAG_LAST);
		if (src) {
			dst->vb2_buf.timestamp = src->vb2_buf.timestamp;
			dst->flags &= ~V4L2_BUF_FLAG_TSTAMP_SRC_MASK;
			dst->flags |= src->flags &
				      V4L2_BUF_FLAG_TSTAMP_SRC_MASK;
		}
		if (ret > 0) {
			if (is_idr)
				dst->flags |= V4L2_BUF_FLAG_KEYFRAME;
			else
				dst->flags |= V4L2_BUF_FLAG_PFRAME;
		}
		if (last)
			dst->flags |= V4L2_BUF_FLAG_LAST;
		vb2_set_plane_payload(&dst->vb2_buf, 0, ret > 0 ? ret : 0);
	}
	if (src) {
		src->sequence = ctx->sequence_out++;
		v4l2_m2m_buf_done(src, ret > 0 ? VB2_BUF_STATE_DONE :
						 VB2_BUF_STATE_ERROR);
	}
	if (dst)
		v4l2_m2m_buf_done(dst, ret > 0 ? VB2_BUF_STATE_DONE :
						 VB2_BUF_STATE_ERROR);
	if (last)
		fh8626_enc_eos(ctx);
	spin_unlock_irqrestore(&ctx->state_lock, flags);
	if (dev->hw && fh8626_pae_poisoned(dev->hw)) {
		vb2_queue_error(v4l2_m2m_get_src_vq(ctx->m2m_ctx));
		vb2_queue_error(v4l2_m2m_get_dst_vq(ctx->m2m_ctx));
		WRITE_ONCE(ctx->aborting, true);
	}
	v4l2_m2m_job_finish(dev->m2m_dev, ctx->m2m_ctx);
}

static void fh8626_enc_device_run(void *priv)
{
	struct fh8626_enc_ctx *ctx = priv;

	queue_work(ctx->dev->workqueue, &ctx->work);
}

static int fh8626_enc_job_ready(void *priv)
{
	struct fh8626_enc_ctx *ctx = priv;

	return !READ_ONCE(ctx->aborting) && !READ_ONCE(ctx->drained) &&
		!READ_ONCE(ctx->dev->disconnected) &&
		ctx->streaming[0] && ctx->streaming[1];
}

static void fh8626_enc_job_abort(void *priv)
{
	struct fh8626_enc_ctx *ctx = priv;

	WRITE_ONCE(ctx->aborting, true);
	/* The worker never takes dev_mutex and its hardware wait is bounded. */
	flush_work(&ctx->work);
}

static const struct v4l2_m2m_ops fh8626_enc_m2m_ops = {
	.device_run = fh8626_enc_device_run,
	.job_ready = fh8626_enc_job_ready,
	.job_abort = fh8626_enc_job_abort,
};

static int fh8626_enc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct fh8626_enc_ctx *ctx = container_of(ctrl->handler,
						  struct fh8626_enc_ctx,
						  ctrl_handler);

	switch (ctrl->id) {
	case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
		WRITE_ONCE(ctx->qp_i, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP:
		WRITE_ONCE(ctx->qp_p, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_GOP_SIZE:
		WRITE_ONCE(ctx->gop_size, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE_MODE:
		WRITE_ONCE(ctx->bitrate_mode, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE:
		WRITE_ONCE(ctx->bitrate, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_BITRATE_PEAK:
		WRITE_ONCE(ctx->bitrate_peak, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
		WRITE_ONCE(ctx->frame_rc, !!ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
		WRITE_ONCE(ctx->min_qp, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
		WRITE_ONCE(ctx->max_qp, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_CPB_SIZE:
		WRITE_ONCE(ctx->cpb_size, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE:
		WRITE_ONCE(ctx->lf_mode, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA:
		WRITE_ONCE(ctx->lf_alpha, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA:
		WRITE_ONCE(ctx->lf_beta, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_ENABLE:
		WRITE_ONCE(ctx->sar_enable, !!ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_IDC:
		WRITE_ONCE(ctx->sar_idc, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH:
		WRITE_ONCE(ctx->sar_width, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT:
		WRITE_ONCE(ctx->sar_height, ctrl->val);
		break;
	case V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME:
		WRITE_ONCE(ctx->force_key_frame, true);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct v4l2_ctrl_ops fh8626_enc_ctrl_ops = {
	.s_ctrl = fh8626_enc_s_ctrl,
};

static int fh8626_enc_setup_ctrls(struct fh8626_enc_ctx *ctx)
{
	struct v4l2_ctrl *ctrl;
	int ret;

	v4l2_ctrl_handler_init(&ctx->ctrl_handler, 21);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP, 0, 51, 1, 28);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP, 0, 51, 1, 28);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_GOP_SIZE, 1, 250, 1, 1);
	v4l2_ctrl_new_std_menu(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_BITRATE_MODE,
		V4L2_MPEG_VIDEO_BITRATE_MODE_CBR,
		~((1 << V4L2_MPEG_VIDEO_BITRATE_MODE_VBR) |
		  (1 << V4L2_MPEG_VIDEO_BITRATE_MODE_CBR)),
		V4L2_MPEG_VIDEO_BITRATE_MODE_VBR);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_BITRATE, 64000, 20000000, 1000,
		2000000);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_BITRATE_PEAK, 64000, 20000000, 1000,
		4000000);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE, 0, 1, 1, 1);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_H264_MIN_QP, 0, 51, 1, 12);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_H264_MAX_QP, 0, 51, 1, 51);
	/* Rate limit buffer in KiB; 0 holds one second at the limit rate. */
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_H264_CPB_SIZE, 0, 65535, 1, 0);
	v4l2_ctrl_new_std_menu(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE, 2, 0,
		V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_ENABLED);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA, -6, 6, 1, 0);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA, -6, 6, 1, 0);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_ENABLE, 0, 1, 1, 0);
	v4l2_ctrl_new_std_menu(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_IDC,
		V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_EXTENDED, 0,
		V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_1x1);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH, 0, 0xffff, 1, 1);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT, 0, 0xffff, 1, 1);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &fh8626_enc_ctrl_ops,
		V4L2_CID_MPEG_VIDEO_FORCE_KEY_FRAME, 0, 0, 0, 0);
	ctrl = v4l2_ctrl_new_std_menu(&ctx->ctrl_handler, NULL,
		V4L2_CID_MPEG_VIDEO_H264_PROFILE,
		V4L2_MPEG_VIDEO_H264_PROFILE_MAIN,
		~(1ULL << V4L2_MPEG_VIDEO_H264_PROFILE_MAIN),
		V4L2_MPEG_VIDEO_H264_PROFILE_MAIN);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	ctrl = v4l2_ctrl_new_std_menu(&ctx->ctrl_handler, NULL,
		V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE,
		V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CABAC,
		~(1ULL << V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CABAC),
		V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CABAC);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	ret = ctx->ctrl_handler.error;
	if (!ret)
		ret = v4l2_ctrl_handler_setup(&ctx->ctrl_handler);
	ctx->fh.ctrl_handler = &ctx->ctrl_handler;
	return ret;
}

static int fh8626_enc_queue_setup(struct vb2_queue *vq,
				  unsigned int *nbuffers,
				  unsigned int *nplanes,
				  unsigned int sizes[],
				  struct device *alloc_devs[])
{
	struct fh8626_enc_ctx *ctx = vb2_get_drv_priv(vq);
	struct fh8626_enc_q_data *q_data = get_q_data(ctx, vq->type);
	u32 bytes = q_data->sizeimage, limit;

	/*
	 * Pad MMAP input for direct DMA; imported images without padding
	 * use the staging copy.
	 */
	if (V4L2_TYPE_IS_OUTPUT(vq->type) && vq->memory == VB2_MEMORY_MMAP)
		bytes = PAGE_ALIGN(max_t(size_t, bytes,
					 fh8626_pae_direct_size(q_data->width,
								q_data->height,
					 q_data->stride)));
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

static int fh8626_enc_buf_prepare(struct vb2_buffer *vb)
{
	struct fh8626_enc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct fh8626_enc_q_data *q_data =
		get_q_data(ctx, vb->vb2_queue->type);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

	if (vb2_plane_size(vb, 0) < q_data->sizeimage)
		return -EINVAL;
	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		if (vbuf->field == V4L2_FIELD_ANY)
			vbuf->field = V4L2_FIELD_NONE;
		if (vbuf->field != V4L2_FIELD_NONE ||
		    vb2_get_plane_payload(vb, 0) < q_data->sizeimage)
			return -EINVAL;
		if (!fh8626_enc_direct(ctx, vb) && !vb2_plane_vaddr(vb, 0))
			return -EINVAL;
	} else {
		vb2_set_plane_payload(vb, 0, 0);
	}

	return 0;
}

static void fh8626_enc_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct fh8626_enc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long flags;
	bool eos = false;

	spin_lock_irqsave(&ctx->state_lock, flags);
	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type) && ctx->eos_pending) {
		ctx->eos_pending = false;
		eos = true;
	}
	if (eos) {
		fh8626_enc_empty_last(ctx, vbuf);
		spin_unlock_irqrestore(&ctx->state_lock, flags);
		return;
	}
	spin_unlock_irqrestore(&ctx->state_lock, flags);

	v4l2_m2m_buf_queue(ctx->m2m_ctx, vbuf);
}

static int fh8626_enc_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct fh8626_enc_ctx *ctx = vb2_get_drv_priv(q);
	struct vb2_v4l2_buffer *buf;
	unsigned int index = V4L2_TYPE_IS_OUTPUT(q->type) ? 0 : 1;
	struct fh8626_enc_q_data *raw = &ctx->q_data[0];
	int ret = 0;

	if (ctx->dev->disconnected) {
		ret = -ENODEV;
	} else if (ctx->streaming[index ^ 1]) {
		ctx->pae = fh8626_pae_ctx_create(ctx->dev->hw, raw->width,
						 raw->height, raw->stride,
						 ctx->q_data[1].sizeimage);
		if (IS_ERR(ctx->pae)) {
			ret = PTR_ERR(ctx->pae);
			ctx->pae = NULL;
		}
	}
	if (!ret) {
		ctx->streaming[index] = true;
		if (!index)
			ctx->sequence_out = 0;
		else
			ctx->sequence_cap = 0;
		if (ctx->streaming[0] && ctx->streaming[1]) {
			ctx->aborting = false;
			ctx->draining = ctx->drained = ctx->eos_pending = false;
			ctx->drain_remaining = 0;
			ctx->gop_count = 0;
			ctx->idr_pending = true;
			ctx->frame_num = 0;
			ctx->poc = 0;
			ctx->hw_frames = 0;
			fh8626_enc_rc_reset(ctx);
			ctx->force_key_frame = false;
		}
		return 0;
	}
	for (;;) {
		buf = V4L2_TYPE_IS_OUTPUT(q->type) ?
			v4l2_m2m_src_buf_remove(ctx->m2m_ctx) :
			v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
		if (!buf)
			break;
		v4l2_m2m_buf_done(buf, VB2_BUF_STATE_QUEUED);
	}
	return ret;
}

static void fh8626_enc_stop_streaming(struct vb2_queue *q)
{
	struct fh8626_enc_ctx *ctx = vb2_get_drv_priv(q);
	struct vb2_v4l2_buffer *vbuf;
	unsigned int index = V4L2_TYPE_IS_OUTPUT(q->type) ? 0 : 1;

	WRITE_ONCE(ctx->aborting, true);
	ctx->streaming[index] = false;
	flush_work(&ctx->work);
	fh8626_pae_ctx_destroy(ctx->pae);
	ctx->pae = NULL;
	ctx->draining = ctx->drained = ctx->eos_pending = false;
	ctx->drain_remaining = 0;

	for (;;) {
		if (V4L2_TYPE_IS_OUTPUT(q->type))
			vbuf = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		else
			vbuf = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

		if (!vbuf)
			break;
		if (!V4L2_TYPE_IS_OUTPUT(q->type))
			vb2_set_plane_payload(&vbuf->vb2_buf, 0, 0);
		v4l2_m2m_buf_done(vbuf, VB2_BUF_STATE_ERROR);
	}
}

static const struct vb2_ops fh8626_enc_qops = {
	.queue_setup		= fh8626_enc_queue_setup,
	.buf_prepare		= fh8626_enc_buf_prepare,
	.buf_queue		= fh8626_enc_buf_queue,
	.start_streaming	= fh8626_enc_start_streaming,
	.stop_streaming		= fh8626_enc_stop_streaming,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
};

static int fh8626_enc_queue_init(void *priv, struct vb2_queue *src_vq,
				 struct vb2_queue *dst_vq)
{
	struct fh8626_enc_ctx *ctx = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	/*
	 * USERPTR cannot guarantee the physically contiguous storage required
	 * by this engine; use MMAP or import a contiguous dma-buf.
	 */
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->ops = &fh8626_enc_qops;
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
	dst_vq->ops = &fh8626_enc_qops;
	dst_vq->mem_ops = &vb2_vmalloc_memops;
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->dev->dev_mutex;
	dst_vq->dev = ctx->dev->dev;

	return vb2_queue_init(dst_vq);
}

static int fh8626_enc_vidioc_querycap(struct file *file, void *priv,
				     struct v4l2_capability *cap)
{
	strlcpy(cap->driver, FH8626_ENC_NAME, sizeof(cap->driver));
	strlcpy(cap->card, "Fullhan FH8626 PAE Video Encoder",
		sizeof(cap->card));
	strlcpy(cap->bus_info, "platform:fh8626-pae", sizeof(cap->bus_info));
	cap->device_caps = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int fh8626_enc_vidioc_enum_fmt(struct file *file, void *priv,
				     struct v4l2_fmtdesc *f)
{
	unsigned int i, index = 0;

	for (i = 0; i < ARRAY_SIZE(fh8626_enc_formats); i++) {
		const struct fh8626_enc_fmt *fmt = &fh8626_enc_formats[i];

		if (fmt->type != f->type)
			continue;
		if (index++ == f->index) {
			f->pixelformat = fmt->fourcc;
			return 0;
		}
	}
	return -EINVAL;
}

static int fh8626_enc_vidioc_g_fmt(struct file *file, void *priv,
				   struct v4l2_format *f)
{
	struct fh8626_enc_ctx *ctx = fh_to_ctx(priv);
	struct fh8626_enc_q_data *q_data = get_q_data(ctx, f->type);

	memset(&f->fmt.pix, 0, sizeof(f->fmt.pix));
	f->fmt.pix.width = q_data->width;
	f->fmt.pix.height = q_data->height;
	f->fmt.pix.field = V4L2_FIELD_NONE;
	f->fmt.pix.pixelformat = q_data->fmt->fourcc;
	f->fmt.pix.bytesperline = q_data->stride;
	f->fmt.pix.sizeimage = q_data->sizeimage;
	f->fmt.pix.colorspace = ctx->colorspace;
	f->fmt.pix.xfer_func = ctx->xfer_func;
	f->fmt.pix.ycbcr_enc = ctx->ycbcr_enc;
	f->fmt.pix.quantization = ctx->quantization;
	return 0;
}

static void fh8626_enc_try_colorimetry(struct v4l2_pix_format *pix)
{
	if (pix->colorspace == V4L2_COLORSPACE_DEFAULT ||
	    pix->colorspace > V4L2_COLORSPACE_DCI_P3)
		pix->colorspace = V4L2_COLORSPACE_SRGB;
	if (pix->xfer_func == V4L2_XFER_FUNC_DEFAULT ||
	    pix->xfer_func > V4L2_XFER_FUNC_SMPTE2084)
		pix->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(pix->colorspace);
	if (pix->ycbcr_enc == V4L2_YCBCR_ENC_DEFAULT ||
	    pix->ycbcr_enc > V4L2_YCBCR_ENC_SMPTE240M)
		pix->ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(pix->colorspace);
	if (pix->quantization == V4L2_QUANTIZATION_DEFAULT ||
	    pix->quantization > V4L2_QUANTIZATION_LIM_RANGE)
		pix->quantization = V4L2_MAP_QUANTIZATION_DEFAULT(false,
						pix->colorspace,
						pix->ycbcr_enc);
}

static int fh8626_enc_vidioc_try_fmt(struct file *file, void *priv,
				     struct v4l2_format *f)
{
	struct fh8626_enc_ctx *ctx = fh_to_ctx(priv);
	struct fh8626_enc_q_data *raw = &ctx->q_data[FH8626_QUEUE_OUTPUT];

	f->fmt.pix.width = clamp_t(u32, f->fmt.pix.width,
				   PAE_MIN_WIDTH, PAE_MAX_WIDTH);
	f->fmt.pix.height = clamp_t(u32, f->fmt.pix.height,
				    PAE_MIN_HEIGHT, PAE_MAX_HEIGHT);
	f->fmt.pix.width = ALIGN(f->fmt.pix.width, 2);
	f->fmt.pix.height = ALIGN(f->fmt.pix.height, 2);
	f->fmt.pix.field = V4L2_FIELD_NONE;
	f->fmt.pix.priv = 0;
	f->fmt.pix.flags = 0;

	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		fh8626_enc_try_colorimetry(&f->fmt.pix);
		f->fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
		/* The engine reads rows of whole 32-pixel groups. */
		f->fmt.pix.bytesperline = ALIGN(f->fmt.pix.width, 32);
		f->fmt.pix.sizeimage = (f->fmt.pix.bytesperline *
					f->fmt.pix.height * 3) / 2;
	} else {
		f->fmt.pix.colorspace = ctx->colorspace;
		f->fmt.pix.xfer_func = ctx->xfer_func;
		f->fmt.pix.ycbcr_enc = ctx->ycbcr_enc;
		f->fmt.pix.quantization = ctx->quantization;
		f->fmt.pix.width = raw->width;
		f->fmt.pix.height = raw->height;
		f->fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
		f->fmt.pix.bytesperline = 0;
		f->fmt.pix.sizeimage =
			clamp_t(u32, f->fmt.pix.sizeimage,
				fh8626_enc_coded_size(raw->width, raw->height),
				ALIGN(raw->width * raw->height, 4096));
	}

	return 0;
}

static int fh8626_enc_vidioc_s_fmt(struct file *file, void *priv,
				   struct v4l2_format *f)
{
	struct fh8626_enc_ctx *ctx = fh_to_ctx(priv);
	struct fh8626_enc_q_data *q_data = get_q_data(ctx, f->type);
	struct fh8626_enc_q_data *coded = &ctx->q_data[FH8626_QUEUE_CAPTURE];
	int ret;

	if (vb2_is_busy(v4l2_m2m_get_src_vq(ctx->m2m_ctx)) ||
	    vb2_is_busy(v4l2_m2m_get_dst_vq(ctx->m2m_ctx)))
		return -EBUSY;

	ret = fh8626_enc_vidioc_try_fmt(file, priv, f);
	if (ret)
		return ret;

	q_data->width = f->fmt.pix.width;
	q_data->height = f->fmt.pix.height;
	q_data->stride = f->fmt.pix.bytesperline;
	q_data->sizeimage = f->fmt.pix.sizeimage;
	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		ctx->colorspace = f->fmt.pix.colorspace;
		ctx->xfer_func = f->fmt.pix.xfer_func;
		ctx->ycbcr_enc = f->fmt.pix.ycbcr_enc;
		ctx->quantization = f->fmt.pix.quantization;
		coded->width = q_data->width;
		coded->height = q_data->height;
		coded->sizeimage = fh8626_enc_coded_size(coded->width,
							 coded->height);
	}

	return 0;
}

static int fh8626_enc_qbuf(struct file *file, void *priv,
			   struct v4l2_buffer *buf)
{
	struct fh8626_enc_ctx *ctx = fh_to_ctx(priv);

	if (V4L2_TYPE_IS_OUTPUT(buf->type) && READ_ONCE(ctx->draining))
		return -EPIPE;
	return v4l2_m2m_ioctl_qbuf(file, priv, buf);
}

static int fh8626_enc_try_command(struct file *file, void *priv,
				 struct v4l2_encoder_cmd *cmd)
{
	if (cmd->cmd != V4L2_ENC_CMD_START && cmd->cmd != V4L2_ENC_CMD_STOP)
		return -EINVAL;
	cmd->flags = 0;
	return 0;
}

static int fh8626_enc_command(struct file *file, void *priv,
			     struct v4l2_encoder_cmd *cmd)
{
	struct fh8626_enc_ctx *ctx = fh_to_ctx(priv);
	struct vb2_v4l2_buffer *dst = NULL;
	unsigned long flags;
	int ret = fh8626_enc_try_command(file, priv, cmd);

	if (ret)
		return ret;
	if (!ctx->streaming[0] || !ctx->streaming[1])
		return -EPIPE;
	spin_lock_irqsave(&ctx->state_lock, flags);
	if (cmd->cmd == V4L2_ENC_CMD_START) {
		struct vb2_queue *cap = v4l2_m2m_get_vq(ctx->m2m_ctx,
						V4L2_BUF_TYPE_VIDEO_CAPTURE);

		/*
		 * Consume the previous LAST before starting another drain;
		 * a late DQBUF could otherwise set the EOF latch after START.
		 */
		if (ctx->draining && (!ctx->drained || ctx->eos_pending ||
				     !cap->last_buffer_dequeued)) {
			spin_unlock_irqrestore(&ctx->state_lock, flags);
			return -EBUSY;
		}
		vb2_clear_last_buffer_dequeued(cap);
		ctx->draining = ctx->drained = false;
	} else if (!ctx->draining) {
		ctx->draining = true;
		ctx->drain_remaining =
			v4l2_m2m_num_src_bufs_ready(ctx->m2m_ctx);
		if (!ctx->drain_remaining) {
			ctx->drained = true;
			dst = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
			ctx->eos_pending = !dst;
		}
	}
	if (dst)
		fh8626_enc_empty_last(ctx, dst);
	spin_unlock_irqrestore(&ctx->state_lock, flags);
	return 0;
}

static int fh8626_enc_enum_framesizes(struct file *file, void *priv,
				      struct v4l2_frmsizeenum *fsize)
{
	if (fsize->index != 0)
		return -EINVAL;
	if (fsize->pixel_format != V4L2_PIX_FMT_NV12 &&
	    fsize->pixel_format != V4L2_PIX_FMT_H264)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	fsize->stepwise.min_width = PAE_MIN_WIDTH;
	fsize->stepwise.max_width = PAE_MAX_WIDTH;
	fsize->stepwise.step_width = 2;
	fsize->stepwise.min_height = PAE_MIN_HEIGHT;
	fsize->stepwise.max_height = PAE_MAX_HEIGHT;
	fsize->stepwise.step_height = 2;
	return 0;
}

static int fh8626_enc_enum_frameintervals(struct file *file, void *priv,
					  struct v4l2_frmivalenum *fival)
{
	if (fival->index != 0)
		return -EINVAL;
	if (fival->pixel_format != V4L2_PIX_FMT_NV12 &&
	    fival->pixel_format != V4L2_PIX_FMT_H264)
		return -EINVAL;
	if (fival->width < PAE_MIN_WIDTH || fival->width > PAE_MAX_WIDTH ||
	    fival->height < PAE_MIN_HEIGHT || fival->height > PAE_MAX_HEIGHT)
		return -EINVAL;

	fival->type = V4L2_FRMIVAL_TYPE_STEPWISE;
	fival->stepwise.min.numerator = 1;
	fival->stepwise.min.denominator = 60;
	fival->stepwise.max.numerator = 1;
	fival->stepwise.max.denominator = 1;
	fival->stepwise.step.numerator = 1;
	fival->stepwise.step.denominator = 60;
	return 0;
}

static int fh8626_enc_g_parm(struct file *file, void *priv,
			     struct v4l2_streamparm *a)
{
	struct fh8626_enc_ctx *ctx = fh_to_ctx(priv);

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE &&
	    a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	if (V4L2_TYPE_IS_OUTPUT(a->type)) {
		a->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
		a->parm.output.timeperframe = ctx->timeperframe;
	} else {
		a->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
		a->parm.capture.timeperframe = ctx->timeperframe;
	}
	return 0;
}

static int fh8626_enc_s_parm(struct file *file, void *priv,
			     struct v4l2_streamparm *a)
{
	struct fh8626_enc_ctx *ctx = fh_to_ctx(priv);
	struct v4l2_fract *timeperframe;

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE &&
	    a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	timeperframe = V4L2_TYPE_IS_OUTPUT(a->type) ?
		&a->parm.output.timeperframe :
		&a->parm.capture.timeperframe;

	if (!timeperframe->numerator || !timeperframe->denominator) {
		timeperframe->numerator = 1;
		timeperframe->denominator = 25;
	} else if (timeperframe->denominator < timeperframe->numerator) {
		timeperframe->numerator = 1;
		timeperframe->denominator = 1;
	} else if (timeperframe->denominator / 60 >= timeperframe->numerator) {
		timeperframe->numerator = 1;
		timeperframe->denominator = 60;
	} else {
		/* Keep fractional rates; rate control budgets by interval. */
		unsigned long d = gcd(timeperframe->numerator,
				      timeperframe->denominator);

		timeperframe->numerator /= d;
		timeperframe->denominator /= d;
	}

	ctx->timeperframe = *timeperframe;
	return fh8626_enc_g_parm(file, priv, a);
}

static int fh8626_enc_subscribe(struct v4l2_fh *fh,
			       const struct v4l2_event_subscription *sub)
{
	if (sub->type == V4L2_EVENT_EOS)
		return v4l2_event_subscribe(fh, sub, 1, NULL);
	return v4l2_ctrl_subscribe_event(fh, sub);
}

static const struct v4l2_ioctl_ops fh8626_enc_ioctl_ops = {
	.vidioc_querycap	= fh8626_enc_vidioc_querycap,

	.vidioc_enum_framesizes = fh8626_enc_enum_framesizes,
	.vidioc_enum_frameintervals = fh8626_enc_enum_frameintervals,
	.vidioc_g_parm		= fh8626_enc_g_parm,
	.vidioc_s_parm		= fh8626_enc_s_parm,

	.vidioc_enum_fmt_vid_cap = fh8626_enc_vidioc_enum_fmt,
	.vidioc_g_fmt_vid_cap	= fh8626_enc_vidioc_g_fmt,
	.vidioc_try_fmt_vid_cap	= fh8626_enc_vidioc_try_fmt,
	.vidioc_s_fmt_vid_cap	= fh8626_enc_vidioc_s_fmt,

	.vidioc_enum_fmt_vid_out = fh8626_enc_vidioc_enum_fmt,
	.vidioc_g_fmt_vid_out	= fh8626_enc_vidioc_g_fmt,
	.vidioc_try_fmt_vid_out	= fh8626_enc_vidioc_try_fmt,
	.vidioc_s_fmt_vid_out	= fh8626_enc_vidioc_s_fmt,

	.vidioc_reqbufs		= v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf	= v4l2_m2m_ioctl_querybuf,
	.vidioc_create_bufs	= v4l2_m2m_ioctl_create_bufs,
	.vidioc_expbuf		= v4l2_m2m_ioctl_expbuf,
	.vidioc_qbuf		= fh8626_enc_qbuf,
	.vidioc_dqbuf		= v4l2_m2m_ioctl_dqbuf,
	.vidioc_prepare_buf	= v4l2_m2m_ioctl_prepare_buf,
	.vidioc_streamon	= v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff	= v4l2_m2m_ioctl_streamoff,

	.vidioc_try_encoder_cmd = fh8626_enc_try_command,
	.vidioc_encoder_cmd = fh8626_enc_command,
	.vidioc_subscribe_event	= fh8626_enc_subscribe,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static int fh8626_enc_open(struct file *file)
{
	struct fh8626_enc_dev *dev = video_drvdata(file);
	struct fh8626_enc_ctx *ctx;
	int ret;

	if (mutex_lock_interruptible(&dev->dev_mutex))
		return -ERESTARTSYS;
	if (dev->disconnected) {
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
	ctx->colorspace = V4L2_COLORSPACE_SRGB;
	ctx->xfer_func = V4L2_XFER_FUNC_SRGB;
	ctx->ycbcr_enc = V4L2_YCBCR_ENC_601;
	ctx->quantization = V4L2_QUANTIZATION_LIM_RANGE;
	INIT_WORK(&ctx->work, fh8626_enc_work);
	spin_lock_init(&ctx->state_lock);

	ctx->q_data[FH8626_QUEUE_OUTPUT].fmt = &fh8626_enc_formats[0];
	ctx->q_data[FH8626_QUEUE_OUTPUT].width = PAE_DEF_WIDTH;
	ctx->q_data[FH8626_QUEUE_OUTPUT].height = PAE_DEF_HEIGHT;
	ctx->q_data[FH8626_QUEUE_OUTPUT].stride = PAE_DEF_WIDTH;
	ctx->q_data[FH8626_QUEUE_OUTPUT].sizeimage =
		(PAE_DEF_WIDTH * PAE_DEF_HEIGHT * 3) / 2;

	ctx->q_data[FH8626_QUEUE_CAPTURE].fmt = &fh8626_enc_formats[1];
	ctx->q_data[FH8626_QUEUE_CAPTURE].width = PAE_DEF_WIDTH;
	ctx->q_data[FH8626_QUEUE_CAPTURE].height = PAE_DEF_HEIGHT;
	ctx->q_data[FH8626_QUEUE_CAPTURE].stride = 0;
	ctx->q_data[FH8626_QUEUE_CAPTURE].sizeimage =
		fh8626_enc_coded_size(PAE_DEF_WIDTH, PAE_DEF_HEIGHT);

	ctx->qp_i = 28;
	ctx->qp_p = 28;
	ctx->gop_size = 1;
	ctx->bitrate_mode = V4L2_MPEG_VIDEO_BITRATE_MODE_VBR;
	ctx->bitrate = 2000000;
	ctx->bitrate_peak = 4000000;
	ctx->frame_rc = true;
	ctx->min_qp = 12;
	ctx->max_qp = 51;
	ctx->sar_idc = V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_1x1;
	ctx->sar_width = 1;
	ctx->sar_height = 1;
	ctx->timeperframe.numerator = 1;
	ctx->timeperframe.denominator = 25;
	fh8626_enc_rc_reset(ctx);

	ret = fh8626_enc_setup_ctrls(ctx);
	if (ret)
		goto err_free;

	ctx->m2m_ctx = v4l2_m2m_ctx_init(dev->m2m_dev, ctx,
					 fh8626_enc_queue_init);
	if (IS_ERR(ctx->m2m_ctx)) {
		ret = PTR_ERR(ctx->m2m_ctx);
		goto err_free;
	}

	ctx->fh.m2m_ctx = ctx->m2m_ctx;
	list_add_tail(&ctx->list, &dev->contexts);
	v4l2_fh_add(&ctx->fh);
	mutex_unlock(&dev->dev_mutex);
	return 0;

err_free:
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	v4l2_fh_exit(&ctx->fh);
	file->private_data = NULL;
	kfree(ctx);
	mutex_unlock(&dev->dev_mutex);
	return ret;
}

static int fh8626_enc_release(struct file *file)
{
	struct fh8626_enc_ctx *ctx = fh_to_ctx(file->private_data);
	struct fh8626_enc_dev *dev = ctx->dev;

	mutex_lock(&dev->dev_mutex);
	v4l2_m2m_ctx_release(ctx->m2m_ctx);
	/* job_finish() can wake ctx_release() before the worker returns. */
	flush_work(&ctx->work);
	ctx->fh.m2m_ctx = NULL;
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	list_del(&ctx->list);
	kfree(ctx);
	mutex_unlock(&dev->dev_mutex);

	return 0;
}

static unsigned int fh8626_enc_poll(struct file *file, poll_table *wait)
{
	struct fh8626_enc_ctx *ctx = fh_to_ctx(file->private_data);
	unsigned int ret;

	mutex_lock(&ctx->dev->dev_mutex);
	if (ctx->dev->disconnected) {
		ret = POLLERR | POLLHUP;
	} else {
		ret = v4l2_m2m_poll(file, ctx->m2m_ctx, wait);
		/* Check after registering waiters to catch pending errors. */
		if (v4l2_m2m_get_src_vq(ctx->m2m_ctx)->error ||
		    v4l2_m2m_get_dst_vq(ctx->m2m_ctx)->error)
			ret |= POLLERR;
	}
	mutex_unlock(&ctx->dev->dev_mutex);
	return ret;
}

static int fh8626_enc_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct fh8626_enc_ctx *ctx = fh_to_ctx(file->private_data);
	int ret;

	mutex_lock(&ctx->dev->dev_mutex);
	ret = ctx->dev->disconnected ? -ENODEV :
		v4l2_m2m_mmap(file, ctx->m2m_ctx, vma);
	mutex_unlock(&ctx->dev->dev_mutex);
	return ret;
}

static void fh8626_enc_show_stats(struct seq_file *s, const char *name,
				  const struct fh8626_pae_stats *st)
{
	u64 frames = max_t(u64, st->frames, 1);

	seq_printf(s,
		   "%s frames %llu copied %llu busy_us %llu avg_us %llu max_us %u job_avg_us %llu dma_kib %zu\n",
		   name, st->frames, st->copied,
		   div_u64(st->busy_ns, NSEC_PER_USEC),
		   div64_u64(st->busy_ns, frames * NSEC_PER_USEC),
		   st->busy_max_ns / (u32)NSEC_PER_USEC,
		   div64_u64(st->job_ns, frames * NSEC_PER_USEC),
		   st->dma_bytes / 1024);
}

static int fh8626_enc_stats_show(struct seq_file *s, void *unused)
{
	struct fh8626_enc_dev *dev = s->private;
	struct fh8626_pae_stats st;
	struct fh8626_enc_ctx *ctx;
	char name[32];

	mutex_lock(&dev->dev_mutex);
	if (dev->hw) {
		fh8626_pae_get_stats(dev->hw, &st);
		fh8626_enc_show_stats(s, "engine", &st);
	}
	list_for_each_entry(ctx, &dev->contexts, list) {
		if (!ctx->pae)
			continue;
		fh8626_pae_ctx_get_stats(ctx->pae, &st);
		snprintf(name, sizeof(name), "%ux%u",
			 ctx->q_data[FH8626_QUEUE_OUTPUT].width,
			 ctx->q_data[FH8626_QUEUE_OUTPUT].height);
		fh8626_enc_show_stats(s, name, &st);
	}
	mutex_unlock(&dev->dev_mutex);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(fh8626_enc_stats);

static const struct v4l2_file_operations fh8626_enc_fops = {
	.owner		= THIS_MODULE,
	.open		= fh8626_enc_open,
	.release	= fh8626_enc_release,
	.poll		= fh8626_enc_poll,
	.unlocked_ioctl	= fh8626_video_ioctl2,
	.mmap		= fh8626_enc_mmap,
};

static void fh8626_enc_dev_release(struct kref *ref)
{
	struct fh8626_enc_dev *dev =
		container_of(ref, struct fh8626_enc_dev, ref);

	if (dev->m2m_dev)
		v4l2_m2m_release(dev->m2m_dev);
	if (dev->workqueue)
		destroy_workqueue(dev->workqueue);
	v4l2_device_unregister(&dev->v4l2_dev);
	put_device(dev->dev);
	kfree(dev);
}

static void fh8626_enc_video_release(struct video_device *video)
{
	struct fh8626_enc_dev *dev =
		container_of(video, struct fh8626_enc_dev, vfd);

	kref_put(&dev->ref, fh8626_enc_dev_release);
}

static void fh8626_enc_put_clks(struct fh8626_enc_dev *enc_dev)
{
	unsigned int i;

	for (i = 0; i < FH8626_ENC_NUM_CLKS; i++) {
		clk_put(enc_dev->clks[i]);
		enc_dev->clks[i] = NULL;
	}
}

/* Clocks are held explicitly so they survive a quarantined removal. */
static int fh8626_enc_get_clks(struct fh8626_enc_dev *enc_dev)
{
	unsigned int i;

	for (i = 0; i < FH8626_ENC_NUM_CLKS; i++) {
		const char *name = fh8626_enc_clk_names[i];
		struct clk *clk = clk_get(enc_dev->dev, name);

		if (IS_ERR(clk)) {
			fh8626_enc_put_clks(enc_dev);
			return PTR_ERR(clk);
		}
		enc_dev->clks[i] = clk;
	}
	return 0;
}

static void fh8626_enc_disable_clks(struct fh8626_enc_dev *enc_dev,
				    unsigned int count)
{
	while (count--)
		clk_disable_unprepare(enc_dev->clks[count]);
}

static int fh8626_enc_enable_clks(struct fh8626_enc_dev *enc_dev)
{
	unsigned int i;
	int ret;

	for (i = 0; i < FH8626_ENC_NUM_CLKS; i++) {
		ret = clk_prepare_enable(enc_dev->clks[i]);
		if (ret) {
			fh8626_enc_disable_clks(enc_dev, i);
			return ret;
		}
	}
	return 0;
}

static int fh8626_enc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fh8626_enc_dev *enc_dev;
	struct resource *res;
	resource_size_t core_start;
	bool quarantined;
	int ret;

	enc_dev = kzalloc(sizeof(*enc_dev), GFP_KERNEL);
	if (!enc_dev)
		return -ENOMEM;

	enc_dev->dev = get_device(dev);
	kref_init(&enc_dev->ref);
	INIT_LIST_HEAD(&enc_dev->contexts);
	mutex_init(&enc_dev->dev_mutex);
	enc_dev->workqueue = alloc_ordered_workqueue("fh8626-pae",
						     WQ_MEM_RECLAIM);
	if (!enc_dev->workqueue) {
		ret = -ENOMEM;
		goto err_dev;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -EINVAL;
		goto err_dev;
	}
	core_start = res->start;
	enc_dev->core_regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(enc_dev->core_regs)) {
		ret = PTR_ERR(enc_dev->core_regs);
		goto err_dev;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	enc_dev->buf_regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(enc_dev->buf_regs)) {
		ret = PTR_ERR(enc_dev->buf_regs);
		goto err_dev;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	enc_dev->sram_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(enc_dev->sram_base)) {
		ret = PTR_ERR(enc_dev->sram_base);
		goto err_dev;
	}

	enc_dev->irq = platform_get_irq(pdev, 0);
	if (enc_dev->irq < 0) {
		ret = enc_dev->irq;
		goto err_dev;
	}

	ret = fh8626_enc_get_clks(enc_dev);
	if (ret)
		goto err_dev;
	/*
	 * The engine addresses memory with 19 bits in 1 KiB units; bus
	 * translation comes from the parent dma-ranges.
	 */
	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(29));
	if (ret)
		goto err_put_clks;
	ret = fh8626_enc_enable_clks(enc_dev);
	if (ret)
		goto err_put_clks;

	enc_dev->hw = fh8626_pae_create(dev, enc_dev->core_regs,
					enc_dev->buf_regs, enc_dev->sram_base,
					core_start);
	if (IS_ERR(enc_dev->hw)) {
		ret = PTR_ERR(enc_dev->hw);
		enc_dev->hw = NULL;
		goto err_clk;
	}
	ret = devm_request_irq(dev, enc_dev->irq, fh8626_enc_isr,
			       IRQF_SHARED, FH8626_ENC_NAME, enc_dev);
	if (ret)
		goto err_hw;
	enc_dev->irq_registered = true;

	ret = v4l2_device_register(dev, &enc_dev->v4l2_dev);
	if (ret) {
		dev_err(dev, "failed to register V4L2 device: %d\n", ret);
		goto err_irq;
	}

	enc_dev->m2m_dev = v4l2_m2m_init(&fh8626_enc_m2m_ops);
	if (IS_ERR(enc_dev->m2m_dev)) {
		ret = PTR_ERR(enc_dev->m2m_dev);
		enc_dev->m2m_dev = NULL;
		dev_err(dev, "failed to initialize V4L2 M2M device: %d\n",
			ret);
		goto err_irq;
	}

	enc_dev->vfd.fops = &fh8626_enc_fops;
	enc_dev->vfd.ioctl_ops = &fh8626_enc_ioctl_ops;
	enc_dev->vfd.release = fh8626_enc_video_release;
	enc_dev->vfd.lock = &enc_dev->dev_mutex;
	enc_dev->vfd.v4l2_dev = &enc_dev->v4l2_dev;
	enc_dev->vfd.vfl_dir = VFL_DIR_M2M;
	enc_dev->vfd.device_caps = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;
	strlcpy(enc_dev->vfd.name, FH8626_ENC_NAME,
		sizeof(enc_dev->vfd.name));
	video_set_drvdata(&enc_dev->vfd, enc_dev);

	kref_get(&enc_dev->ref);
	ret = video_register_device(&enc_dev->vfd, VFL_TYPE_GRABBER, -1);
	if (ret) {
		if (enc_dev->vfd.minor == -1)
			kref_put(&enc_dev->ref, fh8626_enc_dev_release);
		dev_err(dev, "failed to register video device: %d\n", ret);
		goto err_irq;
	}

	platform_set_drvdata(pdev, enc_dev);
	enc_dev->debugfs = debugfs_create_dir(dev_name(dev), NULL);
	if (IS_ERR_OR_NULL(enc_dev->debugfs))
		enc_dev->debugfs = NULL;
	else
		debugfs_create_file("statistics", 0444, enc_dev->debugfs,
				    enc_dev, &fh8626_enc_stats_fops);
	dev_dbg(dev, "registered %s\n",
		video_device_node_name(&enc_dev->vfd));

	return 0;

err_irq:
	devm_free_irq(dev, enc_dev->irq, enc_dev);
	enc_dev->irq_registered = false;
err_hw:
	quarantined = fh8626_pae_destroy(enc_dev->hw);
	enc_dev->hw = NULL;
	if (quarantined)
		goto err_dev;
err_clk:
	fh8626_enc_disable_clks(enc_dev, FH8626_ENC_NUM_CLKS);
err_put_clks:
	fh8626_enc_put_clks(enc_dev);
err_dev:
	kref_put(&enc_dev->ref, fh8626_enc_dev_release);
	return ret;
}

/* Caller holds dev_mutex; workers never take it. Used for reboot as well. */
static void fh8626_enc_disconnect_hw(struct fh8626_enc_dev *enc_dev)
{
	struct fh8626_enc_ctx *ctx;

	WRITE_ONCE(enc_dev->disconnected, true);
	list_for_each_entry(ctx, &enc_dev->contexts, list) {
		vb2_queue_error(v4l2_m2m_get_src_vq(ctx->m2m_ctx));
		vb2_queue_error(v4l2_m2m_get_dst_vq(ctx->m2m_ctx));
		v4l2_m2m_streamoff(NULL, ctx->m2m_ctx,
				   V4L2_BUF_TYPE_VIDEO_OUTPUT);
		v4l2_m2m_streamoff(NULL, ctx->m2m_ctx,
				   V4L2_BUF_TYPE_VIDEO_CAPTURE);
		WRITE_ONCE(ctx->aborting, true);
		flush_work(&ctx->work);
	}
	if (enc_dev->irq_registered) {
		devm_free_irq(enc_dev->dev, enc_dev->irq, enc_dev);
		enc_dev->irq_registered = false;
	}
	if (enc_dev->hw) {
		bool quarantined = fh8626_pae_destroy(enc_dev->hw);

		enc_dev->hw = NULL;
		if (!quarantined) {
			fh8626_enc_disable_clks(enc_dev, FH8626_ENC_NUM_CLKS);
			fh8626_enc_put_clks(enc_dev);
		}
	}
}

static int fh8626_enc_remove(struct platform_device *pdev)
{
	struct fh8626_enc_dev *enc_dev = platform_get_drvdata(pdev);

	/* The statistics file takes dev_mutex; remove it first. */
	debugfs_remove_recursive(enc_dev->debugfs);
	enc_dev->debugfs = NULL;
	mutex_lock(&enc_dev->dev_mutex);
	fh8626_enc_disconnect_hw(enc_dev);
	video_unregister_device(&enc_dev->vfd);
	fh8626_video_wake_events(&enc_dev->vfd);
	v4l2_device_disconnect(&enc_dev->v4l2_dev);
	platform_set_drvdata(pdev, NULL);
	mutex_unlock(&enc_dev->dev_mutex);
	kref_put(&enc_dev->ref, fh8626_enc_dev_release);

	return 0;
}

static void fh8626_enc_shutdown(struct platform_device *pdev)
{
	struct fh8626_enc_dev *enc_dev = platform_get_drvdata(pdev);

	mutex_lock(&enc_dev->dev_mutex);
	fh8626_enc_disconnect_hw(enc_dev);
	mutex_unlock(&enc_dev->dev_mutex);
}

static int fh8626_enc_suspend(struct device *dev)
{
	return -EOPNOTSUPP;
}

static const struct dev_pm_ops fh8626_enc_pm_ops = {
	.suspend = fh8626_enc_suspend,
	.freeze = fh8626_enc_suspend,
	.poweroff = fh8626_enc_suspend,
};

static const struct of_device_id fh8626_enc_of_match[] = {
	{ .compatible = "fullhan,fh8626-pae" },
	{ .compatible = "fullhan,fh8626v100-encoder" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, fh8626_enc_of_match);

static struct platform_driver fh8626_enc_driver = {
	.probe		= fh8626_enc_probe,
	.remove		= fh8626_enc_remove,
	.shutdown = fh8626_enc_shutdown,
	.driver		= {
		.name	= FH8626_ENC_NAME,
		.of_match_table = fh8626_enc_of_match,
		.pm = &fh8626_enc_pm_ops,
	},
};

module_platform_driver(fh8626_enc_driver);

MODULE_DESCRIPTION("Fullhan FH8626 PAE H.264 Video Encoder Driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
