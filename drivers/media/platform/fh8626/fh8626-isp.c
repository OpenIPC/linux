// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 image signal processor subdevice
 *
 * Sole owner of the ISP/VPU register window and its interrupt.
 *
 * Copyright (C) 2026 Difan Zhang
 */
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/fixp-arith.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kref.h>
#include <media/fh8626-subdev.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/fh8626-isp.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>

#include "fh8626-isp-regs.h"
#include "fh8626-isp-defaults.h"
#include "fh8626-pipeline.h"

/* Three frame periods at the slowest sensor rate, with margin */
#define ISP_QUIESCE_MS 1000
#define ISP_STAT_STRIDE 0x21270
#define ISP_STAT_BYTES (2 * ISP_STAT_STRIDE)
/*
 * The NR3D history buffer holds 10-bit packed Bayer with line
 * stride ALIGN(w * 10 / 8, 8). The NR3D coefficient map holds one
 * byte per 4x4 block, row stride ALIGN(w / 4, 8), h / 4 rows. The DMA
 * engine requires 128-byte aligned base addresses for both.
 */
#define ISP_NR3D_ALIGN 128U
#define ISP_NR3D_BUF_STRIDE_FOR(w) ALIGN(((w) * 10U) / 8U, 8U)
#define ISP_NR3D_COEFF_STRIDE_FOR(w) ALIGN((w) / 4U, 8U)
#define ISP_NR3D_BUF_BYTES_FOR(w, h) \
	ALIGN((size_t)(h) * ISP_NR3D_BUF_STRIDE_FOR(w), ISP_NR3D_ALIGN)
#define ISP_NR3D_COEFF_BYTES_FOR(w, h) \
	ALIGN((size_t)((h) / 4U) * ISP_NR3D_COEFF_STRIDE_FOR(w), \
	      ISP_NR3D_ALIGN)
#define ISP_NR3D_STRENGTH_DEFAULT 0x54U
#define ISP_NR3D_NOISE_DEFAULT 0x20dU
#define ISP_NR3D_SHIFT_DEFAULT 8U
#define ISP_NR3D_MOTION_DEFAULT 0x78U
#define ISP_NR3D_NOISE_MAX 0x7ffffU
/* Bayer noise filter defaults, the same for every channel */
#define ISP_BNR_STRENGTH_DEFAULT 0x0aU
#define ISP_BNR_NOISE_DEFAULT 0x64U
#define ISP_BNR_SHIFT_DEFAULT 2U
#define ISP_CCM_UNITY 512
/*
 * Unity-gain defaults: light filtering on the finest luma band, no chroma
 * filtering, and fixed edge enhancement and purple fringe thresholds.
 */
#define ISP_YNR_KNEE0 80U
#define ISP_YNR_KNEE1 128U
#define ISP_YNR_NOISE_DEFAULT 5U
#define ISP_APC_DETAIL_STRENGTH_DEFAULT 112U
#define ISP_APC_EDGE_STRENGTH_DEFAULT 170U
#define ISP_APC_DETAIL_GAIN_DEFAULT 7U
#define ISP_APC_EDGE_GAIN_DEFAULT 8U
#define ISP_SAT_SCALE_UNITY 64U
#define ISP_PFC_HUE_DEFAULT 15U
#define ISP_PFC_RANGE_DEFAULT 30U
#define ISP_PFC_RANGE_MAX 89U
#define ISP_PFC_SHIFT 8U

static const u16 isp_ynr_threshold_default[ISP_YNR_BANDS] = { 16, 16, 20 };
static const u16 isp_ynr_strength_default[ISP_YNR_BANDS] = { 6, 0, 0 };

/* Weight against normalised luma difference, the same for every band */
static const u16 isp_ynr_lut[ISP_YNR_LUT_POINTS] = {
	96, 96, 80, 80, 64, 64, 48, 48, 32, 32, 16, 16, 5, 5, 5, 5,
};

/* Purple fringe luma, exposure and saturation gating thresholds */
static const u32 isp_pfc_thresholds[3] = {
	0x080e62aa, 0x0007fc08, 0x00032014,
};

struct fh8626_isp_dev {
	struct kref ref;
	struct fh8626_subdev_lifetime lifetime;
	struct list_head quarantine;
	resource_size_t phys;
	bool dma_pinned;
	struct device *dev;
	struct v4l2_subdev sd;
	struct v4l2_ctrl_handler ctrls;
	struct v4l2_ctrl *wb[3];
	struct media_pad pads[2];
	struct v4l2_mbus_framefmt format;
	void __iomem *base;
	int irq;
	struct clk *clk, *hclk;
	struct mutex mutex;
	spinlock_t lock;
	struct fh8626_vpu_dev *vpu;
	bool streaming, prepared, dma_fault, wb_pending, stat_pending;
	/* Stopping: the DMA engines are being turned off at frame starts */
	bool quiescing, quiesce_written, quiesced;
	/* A stop missed the frame boundary; the DMA targets stay allocated */
	bool unsettled;
	u32 quiesce_seq;
	wait_queue_head_t quiesce_wq;
	bool stat_error;
	bool digital_gain_pending, nr3d_pending, nr3d_ready, nr3d_first_frame;
	bool color_pending, sharpness_pending;
	u8 nr3d_init_starts;	/* picture starts until history is valid */
	bool bnr_pending, black_pending, ccm_pending, gamma_pending;
	bool hlr_pending, ynr_pending, cnr_pending, pfc_pending;
	u16 hlr_threshold;
	u16 ynr_threshold[ISP_YNR_BANDS];
	u16 ynr_strength[ISP_YNR_BANDS];
	u8 ynr_noise[FH8626_ISP_YNR_NOISE_POINTS];
	u8 cnr_strength;
	u8 cnr_threshold[FH8626_ISP_CNR_THRESHOLDS];
	u8 apc_strength[FH8626_ISP_APC_PATHS];
	u8 apc_gain[FH8626_ISP_APC_PATHS];
	u8 sat_scale;
	bool pfc_enable;
	u16 pfc_hue;
	u8 pfc_range;
	u8 bayer_pattern;
	u8 nr3d_strength;
	u8 nr3d_shift;
	u8 nr3d_motion;
	u8 nr3d_blend;
	u8 nr3d_preset;
	u32 nr3d_noise;
	u8 bnr_strength;
	u8 bnr_shift;
	u32 bnr_noise;
	u16 black[FH8626_ISP_BLACK_LEVELS];	/* R, Gr, Gb, B */
	s16 ccm[FH8626_ISP_CCM_ENTRIES];
	u16 gamma_rgb[FH8626_ISP_GAMMA_POINTS];
	u16 gamma_y[FH8626_ISP_GAMMA_POINTS];
	u8 brightness;
	u8 contrast;
	u8 saturation;
	u8 sharpness;
	u8 auto_exposure;
	bool auto_wb;
	u16 gains[4];
	u16 digital_gain;
	void *stat_cpu;
	dma_addr_t stat_dma;
	void *nr3d_buf_cpu;
	dma_addr_t nr3d_buf_dma;
	size_t nr3d_buf_bytes;
	void *nr3d_coeff_cpu;
	dma_addr_t nr3d_coeff_dma;
	size_t nr3d_coeff_bytes;
	u32 stat_slot, sequence;
	struct fh8626_isp_stats stats;
	struct fh8626_isp_stats pending_stats;
	struct fh8626_isp_ae_stats ae_stats;
	u32 pending_motion[FH8626_ISP_MOTION_WORDS];
	bool pending_motion_valid;
	struct fh8626_isp_grid_stats grid;
};

static LIST_HEAD(faulted_isps);
static DEFINE_MUTEX(faulted_isps_lock);

static struct fh8626_isp_dev *to_isp(struct v4l2_subdev *sd)
{
	return container_of(sd, struct fh8626_isp_dev, sd);
}

static void isp_release(struct kref *ref)
{
	struct fh8626_isp_dev *isp =
		container_of(ref, struct fh8626_isp_dev, ref);

	v4l2_ctrl_handler_free(&isp->ctrls);
	media_entity_cleanup(&isp->sd.entity);
	mutex_destroy(&isp->mutex);
	put_device(isp->dev);
	kfree(isp);
}

static int isp_registered(struct v4l2_subdev *sd)
{
	kref_get(&to_isp(sd)->ref);
	return 0;
}

static void isp_subdev_release(struct v4l2_subdev *sd)
{
	kref_put(&to_isp(sd)->ref, isp_release);
}

static const struct v4l2_subdev_internal_ops isp_internal_ops = {
	.registered = isp_registered,
	.unregistered = isp_subdev_release,
};

int fh8626_isp_dma_begin(struct fh8626_isp_dev *isp)
{
	if (isp->dma_fault)
		return -EIO;
	if (!isp->dma_pinned) {
		if (!try_module_get(THIS_MODULE))
			return -ENODEV;
		isp->dma_pinned = true;
	}
	return 0;
}

void fh8626_isp_dma_end(struct fh8626_isp_dev *isp)
{
	if (isp->dma_pinned && !isp->dma_fault) {
		isp->dma_pinned = false;
		module_put(THIS_MODULE);
	}
}

void __iomem *fh8626_isp_base(struct fh8626_isp_dev *isp)
{
	return isp->base;
}

int fh8626_isp_irq_number(struct fh8626_isp_dev *isp)
{
	return isp->irq;
}

void fh8626_isp_dma_fault(struct fh8626_isp_dev *isp)
{
	/*
	 * DMA may still be live after an unacknowledged reset. Retain private
	 * DMA allocations through remove to prevent writes into reused pages.
	 * Only a reboot clears this state.
	 */
	WRITE_ONCE(isp->dma_fault, true);
}

void fh8626_isp_update_bits(struct fh8626_isp_dev *isp, u32 reg,
			    u32 mask, u32 value)
{
	unsigned long flags;

	spin_lock_irqsave(&isp->lock, flags);
	writel((readl(isp->base + reg) & ~mask) | (value & mask),
	       isp->base + reg);
	spin_unlock_irqrestore(&isp->lock, flags);
}

void fh8626_isp_set_digital_gain(struct fh8626_isp_dev *isp, u16 gain_q6)
{
	u32 value = ((gain_q6 & 0x3ff) << 3) | ((gain_q6 & 0x3ff) << 19);

	writel(value, isp->base + ISP_GAIN_0);
	writel(value, isp->base + ISP_GAIN_1);
}

/*
 * Program the NR3D filter coefficient block, the noise threshold
 * and the temporal coefficient in ISP_NR3D_COEFF_CFG[15:8].
 * Called with isp->lock held.
 */
static void isp_program_nr3d(struct fh8626_isp_dev *isp)
{
	static const u32 startup[] = {
		0x15662002, 0x00040000, 0x78553c26, 0x02fa0045,
		0x0fe10cf6, 0x4a3e3219, 0x786c6256,
	};
	const u32 *coeff = startup;
	unsigned int i;
	u32 word;

	BUILD_BUG_ON(ARRAY_SIZE(fh8626_isp_nr3d_profiles) != 4);
	BUILD_BUG_ON(ARRAY_SIZE(fh8626_isp_nr3d_profiles[0]) !=
		     ARRAY_SIZE(startup));
	if (isp->nr3d_preset)
		coeff = fh8626_isp_nr3d_profiles[isp->nr3d_preset - 1];
	for (i = 0; i < ARRAY_SIZE(startup); ++i)
		writel(coeff[i], isp->base + ISP_NR3D_COEFF(i));
	word = readl(isp->base + ISP_NR3D_COEFF_CFG) & ~ISP_NR3D_BLEND_MASK;
	writel(word | FIELD_PREP(ISP_NR3D_BLEND_MASK, isp->nr3d_blend),
	       isp->base + ISP_NR3D_COEFF_CFG);
	writel(ISP_NR_PARAMS(isp->nr3d_shift, isp->nr3d_noise,
			     isp->nr3d_strength), isp->base + ISP_NR3D_NOISE_TH);
}

/* Bayer noise filter: one word per Bayer channel. Called with lock held. */
static void isp_apply_bnr(struct fh8626_isp_dev *isp)
{
	u32 word = ISP_NR_PARAMS(isp->bnr_shift, isp->bnr_noise,
				 isp->bnr_strength);
	unsigned int i;

	for (i = 0; i < 4; ++i)
		writel(word, isp->base + ISP_BNR_CHANNEL(i));
	isp->bnr_pending = false;
}

/*
 * Colour correction matrix: two signed 13-bit Q9 entries per word, then
 * three zero offsets. Called with isp->lock held.
 */
static void isp_apply_ccm(struct fh8626_isp_dev *isp)
{
	s16 e[2 * ISP_CCM_WORDS] = { 0 };
	unsigned int i;

	memcpy(e, isp->ccm, sizeof(isp->ccm));
	for (i = 0; i < ISP_CCM_WORDS; ++i)
		writel(((u32)e[2 * i] & 0x1fff) |
		       (((u32)e[2 * i + 1] & 0x1fff) << 16),
		       isp->base + ISP_CCM_BASE + 4 * i);
	isp->ccm_pending = false;
}

/*
 * Load both tone curves and run both curve stages: the colour curve on
 * R, G and B before the colour space conversion and the luma curve on the
 * converted luma. Called with isp->lock held.
 */
static void isp_apply_gamma(struct fh8626_isp_dev *isp)
{
	unsigned int i, bank;
	u32 v;

	for (i = 0; i < ISP_GAMMA_WORDS; ++i) {
		v = isp->gamma_rgb[2 * i] |
		    ((u32)isp->gamma_rgb[2 * i + 1] << 16);
		for (bank = 0; bank < ISP_GAMMA_Y_BANK; ++bank)
			writel(v, isp->base + ISP_GAMMA_BANK(bank) + 4 * i);
		v = isp->gamma_y[2 * i] | ((u32)isp->gamma_y[2 * i + 1] << 16);
		writel(v, isp->base + ISP_GAMMA_BANK(ISP_GAMMA_Y_BANK) +
		       4 * i);
	}
	v = readl(isp->base + ISP_PIPE_CTRL);
	writel(v & ~(ISP_PIPE_RGB_GAMMA_BYPASS | ISP_PIPE_Y_GAMMA_BYPASS),
	       isp->base + ISP_PIPE_CTRL);
	v = readl(isp->base + ISP_CSC_CTRL);
	writel(v & ~ISP_CSC_Y_CURVE_SEL, isp->base + ISP_CSC_CTRL);
	isp->gamma_pending = false;
}

static void isp_apply_nr3d(struct fh8626_isp_dev *isp)
{
	u32 bayer = readl(isp->base + ISP_BAYER_FMT);
	u32 ctrl;
	bool was_bypassed = !!(bayer & ISP_BAYER_NR3D_BYPASS);

	isp->nr3d_pending = false;
	/* Never leave NR3D enabled without valid history buffers */
	if (!isp->nr3d_ready || !isp->nr3d_strength) {
		writel(bayer | ISP_BAYER_NR3D_BYPASS,
		       isp->base + ISP_BAYER_FMT);
		writel(readl(isp->base + ISP_NR3D_CTRL) |
		       ISP_NR3D_FIRST_FRAME, isp->base + ISP_NR3D_CTRL);
		isp->nr3d_first_frame = false;
		return;
	}

	isp_program_nr3d(isp);
	ctrl = (readl(isp->base + ISP_NR3D_CTRL) & ~0x0ff0U) |
	       ((u32)isp->nr3d_motion << 4);
	if (!isp->streaming || was_bypassed || isp->nr3d_first_frame) {
		ctrl |= ISP_NR3D_FIRST_FRAME;
		isp->nr3d_first_frame = true;
		isp->nr3d_init_starts = 2;
	}
	writel(ctrl, isp->base + ISP_NR3D_CTRL);
	writel(bayer & ~ISP_BAYER_NR3D_BYPASS, isp->base + ISP_BAYER_FMT);
}

/*
 * Restore the range after black subtraction with a Q8 WB multiplier of
 * 4096 / (4096 - mean black); 256 is unity.
 */
static u32 isp_wb_black_scale(struct fh8626_isp_dev *isp)
{
	u32 b01 = readl(isp->base + ISP_BLC_01);
	u32 b23 = readl(isp->base + ISP_BLC_23);
	u32 mean = ((b01 & 0xfff) + ((b01 >> 16) & 0xfff) +
		    (b23 & 0xfff) + ((b23 >> 16) & 0xfff)) >> 2;

	if (mean >= 0x1000)
		return 256;
	return 0x100000U / (0x1000U - mean);
}

static u16 isp_wb_scaled(u16 gain, u32 scale)
{
	u32 corrected = (u32)(((u64)scale * gain) >> 8);

	return corrected > 0xffff ? 0xffff : (u16)corrected;
}

/*
 * Per-pattern map from colour (R, G, B, second G) to physical Bayer slot,
 * used by both the white-balance and the black-level registers.
 */
static const u8 isp_bayer_slot[4][4] = {
	{ 0, 1, 3, 2 }, { 1, 0, 2, 3 },
	{ 3, 2, 0, 1 }, { 2, 3, 1, 0 },
};

/* isp->gains keeps the pre-black-level value cached by the controls */
static void isp_apply_wb(struct fh8626_isp_dev *isp)
{
	u32 scale = isp_wb_black_scale(isp);
	u16 slot[4];
	unsigned int i;

	for (i = 0; i < 4; ++i)
		slot[isp_bayer_slot[isp->bayer_pattern][i]] =
			isp_wb_scaled(isp->gains[i], scale);
	writel(slot[0] | ((u32)slot[1] << 16), isp->base + ISP_WB_GAIN_01);
	writel(slot[2] | ((u32)slot[3] << 16), isp->base + ISP_WB_GAIN_23);
	isp->wb_pending = false;
}

/*
 * Black level per colour. The WB range compensation depends on it, so the
 * gains are rewritten in the same frame. Called with isp->lock held.
 */
static void isp_apply_black(struct fh8626_isp_dev *isp)
{
	/* Control order R, Gr, Gb, B; slot map order R, G, B, second G */
	const u16 colour[4] = {
		isp->black[0], isp->black[1], isp->black[3], isp->black[2],
	};
	u16 slot[4];
	unsigned int i;

	for (i = 0; i < 4; ++i)
		slot[isp_bayer_slot[isp->bayer_pattern][i]] =
			colour[i] & 0xfff;
	writel(slot[0] | ((u32)slot[1] << 16), isp->base + ISP_BLC_01);
	writel(slot[2] | ((u32)slot[3] << 16), isp->base + ISP_BLC_23);
	isp->black_pending = false;
	isp_apply_wb(isp);
}

/*
 * Brightness, contrast and saturation act on the converted YCbCr signal.
 * The controls are centred on 128: contrast and saturation map to Q6
 * gains (128 -> 1.0x, 255 -> ~2.0x), brightness to a signed luma offset.
 * The saturation gain is further scaled by the Q6 policy scale. The Cb/Cr
 * output offsets stay neutral. Called with isp->lock held.
 */
static void isp_apply_color(struct fh8626_isp_dev *isp)
{
	u32 sat = min_t(u32, ((u32)isp->saturation * isp->sat_scale) /
			(2 * ISP_SAT_SCALE_UNITY), 0xff);
	u32 v;

	v = readl(isp->base + ISP_LUMA_ADJ) &
	    ~(ISP_CONTRAST_MASK | ISP_BRIGHTNESS_MASK);
	v |= FIELD_PREP(ISP_CONTRAST_MASK, isp->contrast / 2);
	v |= FIELD_PREP(ISP_BRIGHTNESS_MASK, (u8)(isp->brightness - 128));
	writel(v, isp->base + ISP_LUMA_ADJ);
	v = readl(isp->base + ISP_SATURATION) & ~ISP_SATURATION_MASK;
	v |= FIELD_PREP(ISP_SATURATION_MASK, sat);
	writel(v, isp->base + ISP_SATURATION);
	writel(ISP_CHROMA_NEUTRAL, isp->base + ISP_CHROMA_OFFSET);
	isp->color_pending = false;
}

static void isp_update(struct fh8626_isp_dev *isp, u32 reg, u32 mask, u32 val)
{
	writel((readl(isp->base + reg) & ~mask) | (val & mask),
	       isp->base + reg);
}

/*
 * Edge enhancement: per-path gains and enhancement limits, the limits
 * scaled by V4L2_CID_SHARPNESS around 128. Both positive and negative
 * limits take the same value. Called with isp->lock held.
 */
static void isp_apply_sharpness(struct fh8626_isp_dev *isp)
{
	static const u32 cfg[FH8626_ISP_APC_PATHS] = {
		ISP_APC_DETAIL_CFG, ISP_APC_EDGE_CFG,
	};
	unsigned int i;
	u32 s;

	isp_update(isp, ISP_APC_GAIN,
		   ISP_APC_DETAIL_GAIN_MASK | ISP_APC_EDGE_GAIN_MASK,
		   FIELD_PREP(ISP_APC_DETAIL_GAIN_MASK, isp->apc_gain[0]) |
		   FIELD_PREP(ISP_APC_EDGE_GAIN_MASK, isp->apc_gain[1]));
	for (i = 0; i < FH8626_ISP_APC_PATHS; ++i) {
		s = min_t(u32, ((u32)isp->apc_strength[i] * isp->sharpness) /
			  128U, 0xff);
		isp_update(isp, cfg[i], ISP_APC_STRENGTH_MASK,
			   FIELD_PREP(ISP_APC_STRENGTH_MASK, s * 0x101));
	}
	isp_update(isp, ISP_PIPE_CTRL, ISP_PIPE_APC_BYPASS, 0);
	isp->sharpness_pending = false;
}

/*
 * Luma noise filter: band thresholds, the brightness curve through
 * (0, 0), (KNEE0, noise[0]) and (KNEE1, noise[1]), flat above KNEE1, and
 * the band weight tables scaled by the band strengths. Called with
 * isp->lock held.
 */
static void isp_apply_ynr(struct fh8626_isp_dev *isp)
{
	u32 y0 = isp->ynr_noise[0];
	u32 y1 = max_t(u32, isp->ynr_noise[1], y0);
	u32 slope0 = min_t(u32, (y0 << 8) / ISP_YNR_KNEE0, 0x3ff);
	u32 slope1 = min_t(u32, ((y1 - y0) << 8) /
			   (ISP_YNR_KNEE1 - ISP_YNR_KNEE0), 0x3ff);
	unsigned int band, i;
	u32 w[2];

	isp_update(isp, ISP_YNR_TH0, ISP_YNR_TH0_MASK,
		   FIELD_PREP(ISP_YNR_TH0_MASK, isp->ynr_threshold[0]));
	writel(isp->ynr_threshold[1] | ((u32)isp->ynr_threshold[2] << 16),
	       isp->base + ISP_YNR_TH12);
	isp_update(isp, ISP_YNR_MODE, ISP_YNR_MODE_MASK, 0);
	writel(FIELD_PREP(ISP_YNR_SLOPE0_MASK, slope0),
	       isp->base + ISP_YNR_CURVE0);
	writel(FIELD_PREP(ISP_YNR_KNEE_MASK, ISP_YNR_KNEE0) |
	       FIELD_PREP(ISP_YNR_LEVEL_MASK, y0) |
	       FIELD_PREP(ISP_YNR_SLOPE1_MASK, slope1),
	       isp->base + ISP_YNR_CURVE1);
	writel(FIELD_PREP(ISP_YNR_KNEE_MASK, ISP_YNR_KNEE1) |
	       FIELD_PREP(ISP_YNR_LEVEL_MASK, y1),
	       isp->base + ISP_YNR_CURVE2);
	for (band = 0; band < ISP_YNR_BANDS; ++band) {
		for (i = 0; i < ISP_YNR_LUT_POINTS; i += 2) {
			w[0] = min_t(u32, ((u32)isp_ynr_lut[i] *
				     isp->ynr_strength[band]) >> 6, 0x3ff);
			w[1] = min_t(u32, ((u32)isp_ynr_lut[i + 1] *
				     isp->ynr_strength[band]) >> 6, 0x3ff);
			writel(w[0] | (w[1] << 16),
			       isp->base + ISP_YNR_LUT(band) + 2 * i);
		}
	}
	isp_update(isp, ISP_PIPE_CTRL, ISP_PIPE_YNR_BYPASS, 0);
	isp->ynr_pending = false;
}

static void isp_apply_cnr(struct fh8626_isp_dev *isp)
{
	u32 fmt = readl(isp->base + ISP_CSC_CTRL) & ISP_CSC_CTRL_FMT_MASK;

	writel(FIELD_PREP(ISP_CNR_TH_LO_MASK, isp->cnr_threshold[0]) |
	       FIELD_PREP(ISP_CNR_TH_HI_MASK, isp->cnr_threshold[1]),
	       isp->base + ISP_CNR_TH);
	isp_update(isp, ISP_CNR_CFG, ISP_CNR_STRENGTH_MASK | ISP_CNR_FMT_MASK,
		   FIELD_PREP(ISP_CNR_STRENGTH_MASK, isp->cnr_strength) |
		   FIELD_PREP(ISP_CNR_FMT_MASK, fmt));
	isp_update(isp, ISP_PIPE_CTRL, ISP_PIPE_CNR_BYPASS, 0);
	isp->cnr_pending = false;
}

static s32 isp_q31_to_q7(s32 v)
{
	s64 x = (s64)v * 128;

	x += x < 0 ? -(1LL << 30) : (1LL << 30);
	return clamp_t(s32, div_s64(x, 1LL << 31), -127, 127);
}

/* tan(degrees) in Q8, saturated to 10 bits; 0 <= degrees < 90 */
static u32 isp_tan_q8(unsigned int degrees)
{
	s32 c = fixp_cos32(degrees);

	if (c <= 0)
		return 0x3ff;
	return min_t(u32, div_s64((s64)fixp_sin32(degrees) << 8, c), 0x3ff);
}

/*
 * Purple fringe correction: sector centre and the tangent of the sector
 * half-width divided by 1..6, which the hardware uses to grade the
 * correction towards the sector edge. Called with isp->lock held.
 */
static void isp_apply_pfc(struct fh8626_isp_dev *isp)
{
	u32 t[6];
	unsigned int k;

	for (k = 0; k < ARRAY_SIZE(t); ++k)
		t[k] = isp_tan_q8(isp->pfc_range / (k + 1));
	writel(((u32)(u8)isp_q31_to_q7(fixp_cos32(isp->pfc_hue)) << 8) |
	       (u8)isp_q31_to_q7(fixp_sin32(isp->pfc_hue)),
	       isp->base + ISP_PFC_HUE);
	writel(t[0] | (t[1] << 10) | (t[2] << 20), isp->base + ISP_PFC_RANGE0);
	writel(t[3] | (t[4] << 10) | (t[5] << 20), isp->base + ISP_PFC_RANGE1);
	for (k = 0; k < ARRAY_SIZE(isp_pfc_thresholds); ++k)
		writel(isp_pfc_thresholds[k], isp->base + ISP_PFC_TH(k));
	isp_update(isp, ISP_PFC_CTRL, ISP_PFC_SHIFT_MASK | ISP_PFC_ENABLE,
		   FIELD_PREP(ISP_PFC_SHIFT_MASK, ISP_PFC_SHIFT) |
		   (isp->pfc_enable ? ISP_PFC_ENABLE : 0));
	isp_update(isp, ISP_PIPE_CTRL, ISP_PIPE_PFC_BYPASS,
		   isp->pfc_enable ? 0 : ISP_PIPE_PFC_BYPASS);
	isp->pfc_pending = false;
}

/*
 * Highlight chroma suppression threshold. The stage stays enabled so that
 * clipped highlights do not pick up a colour cast; the threshold must be
 * programmed because it resets to zero, which suppresses all chroma.
 * Called with isp->lock held.
 */
static void isp_apply_hlr(struct fh8626_isp_dev *isp)
{
	u32 v = readl(isp->base + ISP_HLR_CFG) & ~ISP_HLR_THRESHOLD_MASK;

	writel(v | FIELD_PREP(ISP_HLR_THRESHOLD_MASK, isp->hlr_threshold),
	       isp->base + ISP_HLR_CFG);
	isp->hlr_pending = false;
}

void fh8626_isp_set_awb_gains(struct fh8626_isp_dev *isp,
			      u16 r, u16 gr, u16 gb, u16 b)
{
	unsigned long flags;

	spin_lock_irqsave(&isp->lock, flags);
	isp->gains[0] = r;
	isp->gains[1] = gr;
	isp->gains[2] = b;
	isp->gains[3] = gb;
	/*
	 * While streaming, wait for frame end so a frame keeps one gain. A
	 * prepared idle ISP has no frame end, so write at once; otherwise
	 * prepare() applies the cached value.
	 */
	if (isp->streaming)
		isp->wb_pending = true;
	else if (isp->prepared)
		isp_apply_wb(isp);
	else
		isp->wb_pending = false;
	spin_unlock_irqrestore(&isp->lock, flags);
}

static void isp_stat_address(struct fh8626_isp_dev *isp)
{
	dma_addr_t addr = isp->stat_dma + isp->stat_slot * ISP_STAT_STRIDE;

	/* Both AE statistics DMA banks of the selected slot */
	writel(addr + 0x5c8, isp->base + ISP_STAT_ADDR0);
	writel(addr + 0x105c8, isp->base + ISP_STAT_ADDR1);
}

static void isp_geometry(struct fh8626_isp_dev *isp)
{
	u32 w = isp->format.width, h = isp->format.height;
	u32 size = (h - 1) << 16 | (w - 1);
	u32 grid = ((w >> 5) - 1) | (((h >> 5) - 1) << 8) | 0x0f0f0000;
	unsigned int bank;

	writel(size, isp->base + ISP_INPUT_SIZE);
	writel(size, isp->base + ISP_INPUT_FRAME_SIZE);
	writel(0, isp->base + ISP_CROP_START);
	writel(size, isp->base + ISP_CROP_SIZE);
	writel(ISP_NR3D_BUF_STRIDE_FOR(w), isp->base + ISP_NR3D_BUF_STRIDE);
	writel(ISP_NR3D_COEFF_STRIDE_FOR(w),
	       isp->base + ISP_NR3D_COEFF_STRIDE);
	writel(((h / 32 - 1) << 24) | ((w / 48 - 1) << 16),
	       isp->base + 0x1ec);
	writel(0, isp->base + 0x5c0);
	writel(size, isp->base + 0x5c4);
	for (bank = 0; bank < 2; ++bank) {
		writel((w / 6 - 1) << 16, isp->base + ISP_AWB_WIN_X01(bank));
		writel((w / 3 - 1) | ((w / 2 - 1) << 16),
		       isp->base + ISP_AWB_WIN_X23(bank));
		writel((h / 6 - 1) << 16, isp->base + ISP_AWB_WIN_Y01(bank));
		writel((h / 3 - 1) | ((h / 2 - 1) << 16),
		       isp->base + ISP_AWB_WIN_Y23(bank));
	}
	writel(grid, isp->base + 0x1cc);
	writel(0, isp->base + 0x1d0);
	writel(0x1fff1, isp->base + 0x1d4);
	writel(grid, isp->base + 0x1dc);
	writel(0, isp->base + 0x1e0);
	writel(0x1fff1, isp->base + 0x1e4);
	writel((w / 16) << 16, isp->base + 0x38c);
	writel(w / 8 | ((3 * w / 16) << 16), isp->base + 0x390);
	writel(w / 4 | ((5 * w / 16) << 16), isp->base + 0x394);
	writel(3 * w / 8 | ((7 * w / 16) << 16), isp->base + 0x398);
	writel(w / 2 - 1, isp->base + 0x39c);
	writel(h / 8 | ((h / 4) << 16), isp->base + 0x3a0);
	writel(3 * h / 8 | ((h / 2 - 1) << 16), isp->base + 0x3a4);
	writel(0xfff0000, isp->base + 0x3a8);
}

static void isp_alloc_nr3d_buffers(struct fh8626_isp_dev *isp, u32 width,
				   u32 height);

static void isp_write_ltm_curve(struct fh8626_isp_dev *isp, u32 reg,
				const u32 *points, unsigned int count)
{
	unsigned int i;

	for (i = 0; i + 1 < count; i += 2)
		writel(points[i] | (points[i + 1] << 16),
		       isp->base + reg + (i / 2) * 4);
	if (i < count)
		writel(points[i], isp->base + reg + (i / 2) * 4);
}

int fh8626_isp_prepare(struct fh8626_isp_dev *isp)
{
	size_t nr3d_buf_req, nr3d_coeff_req;
	unsigned int i;
	unsigned long flags;
	bool nr3d_ready;
	int ret = 0;

	mutex_lock(&isp->mutex);
	if (isp->streaming || isp->dma_fault) {
		ret = isp->dma_fault ? -EIO : -EBUSY;
		goto out;
	}
	writel(0, isp->base + ISP_CTRL);
	writel(0, isp->base + ISP_INT_ENABLE);
	for (i = 0; i < ARRAY_SIZE(fh8626_isp_defaults); ++i)
		writel(fh8626_isp_defaults[i].value,
		       isp->base + fh8626_isp_defaults[i].reg);
	isp_write_ltm_curve(isp, ISP_LTM_Y_CURVE_WORD(0), fh8626_isp_ltm_y,
			    ARRAY_SIZE(fh8626_isp_ltm_y));
	isp_write_ltm_curve(isp, ISP_LTM_SAT_CURVE_WORD(0),
			    fh8626_isp_ltm_saturation,
			    ARRAY_SIZE(fh8626_isp_ltm_saturation));
	for (i = 0; i + 1 < ARRAY_SIZE(fh8626_isp_ltm_lut); ++i)
		writel(fh8626_isp_ltm_lut[i] |
		       ((u32)fh8626_isp_ltm_lut[i + 1] << 16),
		       isp->base + ISP_LTM_LUT(i));
	writel(0x44, isp->base + 0x178);
	/* NR3D stays bypassed until its buffer addresses are written below */
	writel((0x0a71eb14 | ISP_BAYER_NR3D_BYPASS) & ~0x1e,
	       isp->base + ISP_BAYER_FMT);
	fh8626_isp_update_bits(isp, ISP_BAYER_FMT, 0x1e,
			       (isp->bayer_pattern << 1) |
			       (isp->bayer_pattern << 3));
	fh8626_isp_update_bits(isp, ISP_RGBA_CTRL, ISP_RGBA_BAYER_MASK,
			       FIELD_PREP(ISP_RGBA_BAYER_MASK, isp->bayer_pattern));
	/*
	 * Both tone curve stages start bypassed; the apply step enables them.
	 * The block mean statistics have no memory target and stay off.
	 */
	writel(0x610, isp->base + ISP_PIPE_CTRL);
	isp_geometry(isp);
	/* Unless a stop missed its frame boundary, the old history is idle */
	if (!READ_ONCE(isp->unsettled))
		isp_alloc_nr3d_buffers(isp, isp->format.width,
				       isp->format.height);
	nr3d_buf_req = ISP_NR3D_BUF_BYTES_FOR(isp->format.width,
					      isp->format.height);
	nr3d_coeff_req = ISP_NR3D_COEFF_BYTES_FOR(isp->format.width,
						  isp->format.height);
	nr3d_ready = isp->nr3d_buf_cpu && isp->nr3d_coeff_cpu &&
		     isp->nr3d_buf_bytes >= nr3d_buf_req &&
		     isp->nr3d_coeff_bytes >= nr3d_coeff_req &&
		     IS_ALIGNED(isp->nr3d_buf_dma, ISP_NR3D_ALIGN) &&
		     IS_ALIGNED(isp->nr3d_coeff_dma, ISP_NR3D_ALIGN);
	if (nr3d_ready) {
		memset(isp->nr3d_buf_cpu, 0, nr3d_buf_req);
		memset(isp->nr3d_coeff_cpu, 0, nr3d_coeff_req);
		/* Addresses must be in place before NR3D leaves bypass */
		writel(isp->nr3d_coeff_dma, isp->base + ISP_NR3D_COEFF_ADDR);
		writel(isp->nr3d_buf_dma, isp->base + ISP_NR3D_BUF_ADDR);
	}
	memset(isp->stat_cpu, 0, ISP_STAT_BYTES);
	isp->stat_slot = 0;
	isp_stat_address(isp);
	spin_lock_irqsave(&isp->lock, flags);
	fh8626_isp_set_digital_gain(isp, isp->digital_gain ?
				    isp->digital_gain : 64);
	isp->digital_gain_pending = false;
	isp->nr3d_ready = nr3d_ready;
	isp->nr3d_first_frame = false;
	isp_apply_nr3d(isp);
	isp_apply_bnr(isp);
	isp_apply_ccm(isp);
	isp_apply_gamma(isp);
	isp_apply_black(isp);	/* also writes the WB gains */
	isp_apply_color(isp);
	isp_apply_sharpness(isp);
	isp_apply_hlr(isp);
	isp_apply_ynr(isp);
	isp_apply_cnr(isp);
	isp_apply_pfc(isp);
	isp->stats.flags = 0;
	isp->ae_stats.flags = 0;
	isp->grid.flags = 0;
	isp->stat_pending = false;
	isp->stat_error = false;
	isp->prepared = true;
	spin_unlock_irqrestore(&isp->lock, flags);
out:
	mutex_unlock(&isp->mutex);
	return ret;
}

/*
 * Each Bayer site has a 4096-byte bank: a 16x16 raster of 16-byte cells
 * holding sum, count, maximum and minimum. Banks follow physical 2x2 site
 * order; isp_bayer_slot maps colours to banks. Addresses latch at the next
 * frame start, so the completed frame uses the slot programmed one picture
 * start earlier. Caller holds isp->lock.
 */
static void isp_grid_snapshot(struct fh8626_isp_dev *isp, u64 stamp)
{
	/* Grid channel order R, Gr, Gb, B; slot map order R, G, B, second G */
	static const u8 colour[FH8626_ISP_GRID_CHANNELS] = { 0, 1, 3, 2 };
	struct fh8626_isp_grid_stats *g = &isp->grid;
	u8 *base = isp->stat_cpu + (isp->stat_slot ^ 1) * ISP_STAT_STRIDE +
		   0x5c8;
	unsigned int bank[FH8626_ISP_GRID_CHANNELS];
	unsigned int n, c;

	BUILD_BUG_ON(sizeof(*g) != FH8626_ISP_GRID_STATS_WORDS * 4);
	for (c = 0; c < FH8626_ISP_GRID_CHANNELS; ++c)
		bank[c] = isp_bayer_slot[isp->bayer_pattern][colour[c]];
	g->version = FH8626_ISP_STATS_VERSION;
	g->sequence = isp->stats.sequence;
	g->timestamp_ns_low = lower_32_bits(stamp);
	g->timestamp_ns_high = upper_32_bits(stamp);
	g->flags = isp->stats.flags & FH8626_ISP_STATS_VALID;
	if (g->flags && isp->pending_motion_valid)
		g->flags |= FH8626_ISP_GRID_MOTION_VALID;
	g->bayer_selector = isp->bayer_pattern;
	g->width = isp->format.width;
	g->height = isp->format.height;
	g->cols = FH8626_ISP_GRID_COLS;
	g->rows = FH8626_ISP_GRID_ROWS;
	g->motion_cols = FH8626_ISP_MOTION_COLS;
	g->motion_rows = FH8626_ISP_MOTION_ROWS;
	memcpy(g->motion, isp->pending_motion, sizeof(g->motion));
	dma_rmb();
	for (n = 0; n < FH8626_ISP_GRID_CELLS; ++n) {
		u32 count = 0, max = 0;

		for (c = 0; c < FH8626_ISP_GRID_CHANNELS; ++c) {
			__le32 *cell = (__le32 *)(base + bank[c] * 0x1000 +
						  n * 16);

			g->cells[n].sum[c] = le32_to_cpu(READ_ONCE(cell[0]));
			count += le32_to_cpu(READ_ONCE(cell[1]));
			max = max(max, le32_to_cpu(READ_ONCE(cell[2])));
		}
		g->cells[n].count = count;
		g->cells[n].max = max;
	}
}

static void isp_ae_snapshot(struct fh8626_isp_dev *isp, u64 stamp)
{
	/*
	 * Nine overlapping metering windows on the 16x16 cell grid
	 * (x0, x1, y0, y1), with unequal areas.
	 */
	static const u8 regions[9][4] = {
		{ 0, 5, 0, 5 }, { 6, 10, 0, 4 }, { 11, 15, 0, 4 },
		{ 0, 4, 6, 10 }, { 5, 10, 5, 10 }, { 11, 15, 5, 9 },
		{ 0, 4, 11, 15 }, { 5, 9, 11, 15 }, { 10, 15, 10, 15 },
	};
	struct fh8626_isp_ae_stats *a = &isp->ae_stats;
	const struct fh8626_isp_grid_stats *g = &isp->grid;
	unsigned int n, x, y, c;

	isp_grid_snapshot(isp, stamp);
	a->version = FH8626_ISP_STATS_VERSION;
	a->sequence = isp->stats.sequence;
	a->timestamp_ns_low = lower_32_bits(stamp);
	a->timestamp_ns_high = upper_32_bits(stamp);
	a->flags = isp->stats.flags & FH8626_ISP_STATS_VALID;
	a->bayer_selector = isp->bayer_pattern;
	a->width = isp->format.width;
	a->height = isp->format.height;
	for (n = 0; n < 9; ++n) {
		u64 sum = 0, count = 0;

		for (y = regions[n][2]; y <= regions[n][3]; ++y) {
			for (x = regions[n][0]; x <= regions[n][1]; ++x) {
				unsigned int i = y * FH8626_ISP_GRID_COLS + x;

				for (c = 0; c < FH8626_ISP_GRID_CHANNELS; ++c)
					sum += g->cells[i].sum[c];
				count += g->cells[i].count;
			}
		}
		a->blocks[n].sum_low = lower_32_bits(sum);
		a->blocks[n].sum_high = upper_32_bits(sum);
		a->blocks[n].count_low = lower_32_bits(count);
		a->blocks[n].count_high = upper_32_bits(count);
	}
}

/*
 * Once one whole frame has been written into the NR3D history and
 * coefficient buffers, let temporal filtering use them. Called with
 * isp->lock held.
 */
static void isp_nr3d_history_valid(struct fh8626_isp_dev *isp)
{
	writel(readl(isp->base + ISP_NR3D_CTRL) & ~ISP_NR3D_FIRST_FRAME,
	       isp->base + ISP_NR3D_CTRL);
	isp->nr3d_first_frame = false;
	isp->nr3d_init_starts = 0;
}

static void isp_apply_pending(struct fh8626_isp_dev *isp)
{
	if (isp->nr3d_pending)
		isp_apply_nr3d(isp);
	if (isp->bnr_pending)
		isp_apply_bnr(isp);
	if (isp->ccm_pending)
		isp_apply_ccm(isp);
	if (isp->gamma_pending)
		isp_apply_gamma(isp);
	if (isp->black_pending)
		isp_apply_black(isp);
	if (isp->wb_pending)
		isp_apply_wb(isp);
	if (isp->color_pending)
		isp_apply_color(isp);
	if (isp->sharpness_pending)
		isp_apply_sharpness(isp);
	if (isp->hlr_pending)
		isp_apply_hlr(isp);
	if (isp->ynr_pending)
		isp_apply_ynr(isp);
	if (isp->cnr_pending)
		isp_apply_cnr(isp);
	if (isp->pfc_pending)
		isp_apply_pfc(isp);
	if (isp->digital_gain_pending) {
		fh8626_isp_set_digital_gain(isp, isp->digital_gain);
		isp->digital_gain_pending = false;
	}
}

/*
 * AWB sums behave as pre-WB relative to ISP_WB_GAIN, so consumers calculate
 * absolute gains. Caller holds isp->lock.
 */
static void isp_capture_awb(struct fh8626_isp_dev *isp, u64 stamp)
{
	struct fh8626_isp_stats *s = &isp->pending_stats;
	unsigned int i;

	s->version = FH8626_ISP_STATS_VERSION;
	s->timestamp_ns_low = lower_32_bits(stamp);
	s->timestamp_ns_high = upper_32_bits(stamp);
	s->flags = FH8626_ISP_STATS_PRE_WB;
	if (!isp->stat_error)
		s->flags |= FH8626_ISP_STATS_VALID;
	isp->stat_error = false;
	s->bayer_selector = isp->bayer_pattern;
	s->width = isp->format.width;
	s->height = isp->format.height;
	isp->pending_motion_valid = !!(readl(isp->base + ISP_BAYER_FMT) &
				       ISP_BAYER_MOTION_STAT);
	for (i = 0; i < FH8626_ISP_MOTION_WORDS; ++i)
		isp->pending_motion[i] = isp->pending_motion_valid ?
			readl(isp->base + ISP_MOTION_MAP + 4 * i) : 0;
	for (i = 0; i < FH8626_ISP_STATS_BLOCKS; ++i) {
		void __iomem *r = isp->base + 0xae0 + i * 16;

		s->blocks[i].r_sum = readl(r);
		s->blocks[i].g_sum = readl(r + 4);
		s->blocks[i].b_sum = readl(r + 8);
		s->blocks[i].count = readl(r + 12);
	}
}

/*
 * NR3D and statistics DMA retain latched targets and enables across stops.
 * Disable them while frames still flow, then wait two picture starts for
 * a complete frame with DMA disabled before releasing the old targets.
 * Returns true once quiesced. Caller holds isp->lock.
 */
static bool isp_quiesce_step(struct fh8626_isp_dev *isp)
{
	u32 bayer;

	++isp->sequence;
	if (!isp->quiesce_written) {
		bayer = readl(isp->base + ISP_BAYER_FMT);
		bayer |= ISP_BAYER_NR3D_BYPASS;
		bayer &= ~ISP_BAYER_STATS_EN;
		writel(bayer, isp->base + ISP_BAYER_FMT);
		isp->quiesce_written = true;
		isp->quiesce_seq = isp->sequence;
		return false;
	}
	if (isp->sequence - isp->quiesce_seq < 2)
		return false;
	isp->quiesced = true;
	return true;
}

static bool isp_quiesce_done(struct fh8626_isp_dev *isp)
{
	unsigned long flags;
	bool done;

	spin_lock_irqsave(&isp->lock, flags);
	done = isp->quiesced || !isp->streaming;
	spin_unlock_irqrestore(&isp->lock, flags);
	return done;
}

/*
 * Bring the ISP DMA engines to rest on a frame boundary. Call while the
 * sensor still delivers frames, after the VPU channels have been drained
 * and before the sensor is stopped; it takes up to three frame periods.
 * The ISP stays quiesced until it is stopped and prepared again.
 */
int fh8626_isp_quiesce(struct fh8626_isp_dev *isp)
{
	unsigned long flags;
	bool streaming;

	spin_lock_irqsave(&isp->lock, flags);
	streaming = isp->streaming;
	if (streaming && !isp->quiescing) {
		isp->quiescing = true;
		isp->quiesce_written = false;
		isp->quiesced = false;
	}
	spin_unlock_irqrestore(&isp->lock, flags);
	if (!streaming)
		return 0;
	if (!wait_event_timeout(isp->quiesce_wq, isp_quiesce_done(isp),
				msecs_to_jiffies(ISP_QUIESCE_MS))) {
		WRITE_ONCE(isp->unsettled, true);
		return -ETIMEDOUT;
	}
	return 0;
}

static irqreturn_t isp_irq(int irq, void *data)
{
	struct fh8626_isp_dev *isp = data;
	struct fh8626_vpu_dev *vpu = READ_ONCE(isp->vpu);
	struct v4l2_event event = { .type = V4L2_EVENT_FRAME_SYNC };
	u32 status = readl(isp->base + ISP_INT_STATUS);
	u64 stamp = ktime_get_ns();
	unsigned long flags;
	bool notify = false, quiesced = false;

	if (!status)
		return IRQ_NONE;
	/* Status is write-one-to-clear; the VPU gets its events first */
	writel(status, isp->base + ISP_INT_STATUS);
	if (vpu)
		fh8626_vpu_irq(vpu, status, stamp);
	spin_lock_irqsave(&isp->lock, flags);
	if (isp->streaming && isp->quiescing) {
		if (status & ISP_INT_PICTURE_START)
			quiesced = isp_quiesce_step(isp);
		spin_unlock_irqrestore(&isp->lock, flags);
		if (quiesced)
			wake_up_all(&isp->quiesce_wq);
		return IRQ_HANDLED;
	}
	if (isp->streaming && (status & ISP_INT_OVERFLOW)) {
		/* Also invalidate a snapshot still awaiting publication */
		isp->stat_error = true;
		isp->pending_stats.flags &= ~FH8626_ISP_STATS_VALID;
	}
	if (isp->streaming && (status & ISP_INT_FRAME_END)) {
		/*
		 * Frame end programs the selected slot and snapshots AWB;
		 * picture start publishes it and selects the next slot, whose
		 * DMA address is installed at the following frame end.
		 */
		isp_stat_address(isp);
		isp_capture_awb(isp, stamp);
		isp->stat_pending = true;
		if (isp->nr3d_first_frame)
			isp_nr3d_history_valid(isp);
		isp_apply_pending(isp);
	}
	if (isp->streaming && (status & ISP_INT_PICTURE_START)) {
		/*
		 * The start-up overflow latches stat_error and frame end is
		 * not repeated, so that END snapshot stays invalid. Picture
		 * start is the steady edge: sample AWB here when the pending
		 * snapshot cannot be published. The stat DMA target stays on
		 * the slot just produced until the AE read is done.
		 */
		if (!(status & ISP_INT_OVERFLOW))
			isp->stat_error = false;
		if (!isp->stat_pending ||
		    !(isp->pending_stats.flags & FH8626_ISP_STATS_VALID)) {
			isp_capture_awb(isp, stamp);
			isp->stat_pending = true;
		}
		/*
		 * Frame end is not always reported. Two picture starts allow
		 * one complete frame to initialise NR3D history.
		 */
		if (isp->nr3d_first_frame && isp->nr3d_init_starts &&
		    !--isp->nr3d_init_starts)
			isp_nr3d_history_valid(isp);
		++isp->sequence;
		if (isp->stat_pending) {
			isp->stats = isp->pending_stats;
			isp->stats.sequence = isp->sequence;
			isp->stats.timestamp_ns_low = lower_32_bits(stamp);
			isp->stats.timestamp_ns_high = upper_32_bits(stamp);
			isp_ae_snapshot(isp, stamp);
			isp->stat_slot ^= 1;
			isp->stat_pending = false;
			isp_stat_address(isp);
		}
		isp_apply_pending(isp);
		event.u.frame_sync.frame_sequence = isp->sequence;
		notify = true;
	}
	spin_unlock_irqrestore(&isp->lock, flags);
	if (notify)
		v4l2_subdev_notify_event(&isp->sd, &event);
	return IRQ_HANDLED;
}

static irqreturn_t isp_irq_thread(int irq, void *data)
{
	return isp_irq(irq, data);
}

/*
 * Mark a parameter block for the next frame boundary while streaming, apply
 * it at once on a prepared idle ISP, or leave it to prepare(). Called with
 * isp->lock held.
 */
static void isp_commit(struct fh8626_isp_dev *isp, bool *pending,
		       void (*apply)(struct fh8626_isp_dev *isp))
{
	if (isp->streaming)
		*pending = true;
	else if (isp->prepared)
		apply(isp);
	else
		*pending = false;
}

static int isp_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct fh8626_isp_dev *isp =
		container_of(ctrl->handler, struct fh8626_isp_dev, ctrls);
	unsigned long flags;
	unsigned int i;

	switch (ctrl->id) {
	case V4L2_CID_POWER_LINE_FREQUENCY:
		/* Policy input for userspace; no ISP register */
		return 0;
	case V4L2_CID_FH8626_NR3D_STRENGTH:
	case V4L2_CID_FH8626_NR3D_NOISE:
	case V4L2_CID_FH8626_NR3D_NOISE_SHIFT:
	case V4L2_CID_FH8626_NR3D_MOTION:
	case V4L2_CID_FH8626_NR3D_BLEND:
	case V4L2_CID_FH8626_NR3D_PRESET:
		spin_lock_irqsave(&isp->lock, flags);
		switch (ctrl->id) {
		case V4L2_CID_FH8626_NR3D_STRENGTH:
			isp->nr3d_strength = ctrl->val;
			break;
		case V4L2_CID_FH8626_NR3D_NOISE:
			isp->nr3d_noise = ctrl->val;
			break;
		case V4L2_CID_FH8626_NR3D_NOISE_SHIFT:
			isp->nr3d_shift = ctrl->val;
			break;
		case V4L2_CID_FH8626_NR3D_MOTION:
			isp->nr3d_motion = ctrl->val;
			break;
		case V4L2_CID_FH8626_NR3D_BLEND:
			isp->nr3d_blend = ctrl->val;
			break;
		default:
			isp->nr3d_preset = ctrl->val;
			break;
		}
		isp_commit(isp, &isp->nr3d_pending, isp_apply_nr3d);
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_FH8626_BNR_STRENGTH:
	case V4L2_CID_FH8626_BNR_NOISE:
	case V4L2_CID_FH8626_BNR_NOISE_SHIFT:
		spin_lock_irqsave(&isp->lock, flags);
		if (ctrl->id == V4L2_CID_FH8626_BNR_STRENGTH)
			isp->bnr_strength = ctrl->val;
		else if (ctrl->id == V4L2_CID_FH8626_BNR_NOISE)
			isp->bnr_noise = ctrl->val;
		else
			isp->bnr_shift = ctrl->val;
		isp_commit(isp, &isp->bnr_pending, isp_apply_bnr);
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_FH8626_HLR_THRESHOLD:
		spin_lock_irqsave(&isp->lock, flags);
		isp->hlr_threshold = ctrl->val;
		isp_commit(isp, &isp->hlr_pending, isp_apply_hlr);
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_FH8626_YNR_THRESHOLD:
	case V4L2_CID_FH8626_YNR_STRENGTH:
	case V4L2_CID_FH8626_YNR_NOISE:
		spin_lock_irqsave(&isp->lock, flags);
		for (i = 0; i < ctrl->elems; ++i) {
			u16 v = ctrl->p_new.p_u16[i];

			if (ctrl->id == V4L2_CID_FH8626_YNR_THRESHOLD)
				isp->ynr_threshold[i] = v;
			else if (ctrl->id == V4L2_CID_FH8626_YNR_STRENGTH)
				isp->ynr_strength[i] = v;
			else
				isp->ynr_noise[i] = v;
		}
		isp_commit(isp, &isp->ynr_pending, isp_apply_ynr);
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_FH8626_CNR_STRENGTH:
	case V4L2_CID_FH8626_CNR_THRESHOLD:
		spin_lock_irqsave(&isp->lock, flags);
		if (ctrl->id == V4L2_CID_FH8626_CNR_STRENGTH)
			isp->cnr_strength = ctrl->val;
		else
			for (i = 0; i < FH8626_ISP_CNR_THRESHOLDS; ++i)
				isp->cnr_threshold[i] = ctrl->p_new.p_u16[i];
		isp_commit(isp, &isp->cnr_pending, isp_apply_cnr);
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_FH8626_APC_STRENGTH:
	case V4L2_CID_FH8626_APC_GAIN:
		spin_lock_irqsave(&isp->lock, flags);
		for (i = 0; i < FH8626_ISP_APC_PATHS; ++i) {
			if (ctrl->id == V4L2_CID_FH8626_APC_STRENGTH)
				isp->apc_strength[i] = ctrl->p_new.p_u16[i];
			else
				isp->apc_gain[i] = ctrl->p_new.p_u16[i];
		}
		isp_commit(isp, &isp->sharpness_pending, isp_apply_sharpness);
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_FH8626_PFC_ENABLE:
	case V4L2_CID_FH8626_PFC_HUE:
	case V4L2_CID_FH8626_PFC_HUE_RANGE:
		spin_lock_irqsave(&isp->lock, flags);
		if (ctrl->id == V4L2_CID_FH8626_PFC_ENABLE)
			isp->pfc_enable = ctrl->val;
		else if (ctrl->id == V4L2_CID_FH8626_PFC_HUE)
			isp->pfc_hue = ctrl->val;
		else
			isp->pfc_range = ctrl->val;
		isp_commit(isp, &isp->pfc_pending, isp_apply_pfc);
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_FH8626_BLACK_LEVEL:
		spin_lock_irqsave(&isp->lock, flags);
		for (i = 0; i < FH8626_ISP_BLACK_LEVELS; ++i)
			isp->black[i] = ctrl->p_new.p_u16[i];
		isp_commit(isp, &isp->black_pending, isp_apply_black);
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_FH8626_CCM:
		spin_lock_irqsave(&isp->lock, flags);
		for (i = 0; i < FH8626_ISP_CCM_ENTRIES; ++i)
			isp->ccm[i] = ctrl->p_new.p_s32[i];
		isp_commit(isp, &isp->ccm_pending, isp_apply_ccm);
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_FH8626_GAMMA_RGB:
	case V4L2_CID_FH8626_GAMMA_Y:
		spin_lock_irqsave(&isp->lock, flags);
		for (i = 0; i < FH8626_ISP_GAMMA_POINTS; ++i) {
			if (ctrl->id == V4L2_CID_FH8626_GAMMA_RGB)
				isp->gamma_rgb[i] = ctrl->p_new.p_u16[i];
			else
				isp->gamma_y[i] = ctrl->p_new.p_u16[i];
		}
		isp_commit(isp, &isp->gamma_pending, isp_apply_gamma);
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_BRIGHTNESS:
	case V4L2_CID_CONTRAST:
	case V4L2_CID_SATURATION:
	case V4L2_CID_FH8626_SATURATION_SCALE:
		spin_lock_irqsave(&isp->lock, flags);
		if (ctrl->id == V4L2_CID_BRIGHTNESS)
			isp->brightness = ctrl->val;
		else if (ctrl->id == V4L2_CID_CONTRAST)
			isp->contrast = ctrl->val;
		else if (ctrl->id == V4L2_CID_SATURATION)
			isp->saturation = ctrl->val;
		else
			isp->sat_scale = ctrl->val;
		isp_commit(isp, &isp->color_pending, isp_apply_color);
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_SHARPNESS:
		spin_lock_irqsave(&isp->lock, flags);
		isp->sharpness = ctrl->val;
		isp_commit(isp, &isp->sharpness_pending, isp_apply_sharpness);
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_EXPOSURE_AUTO:
		spin_lock_irqsave(&isp->lock, flags);
		isp->auto_exposure = ctrl->val;
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_AUTO_WHITE_BALANCE:
		spin_lock_irqsave(&isp->lock, flags);
		isp->auto_wb = ctrl->val;
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	case V4L2_CID_RED_BALANCE:
	case V4L2_CID_BLUE_BALANCE:
	case V4L2_CID_FH8626_WB_GAIN_G: {
		u16 green = isp->wb[2] ? isp->wb[2]->val : 512;

		fh8626_isp_set_awb_gains(isp, isp->wb[0]->val, green, green,
					 isp->wb[1]->val);
		return 0;
	}
	case V4L2_CID_DIGITAL_GAIN:
		spin_lock_irqsave(&isp->lock, flags);
		isp->digital_gain = ctrl->val;
		if (isp->streaming) {
			isp->digital_gain_pending = true;
		} else if (isp->prepared) {
			fh8626_isp_set_digital_gain(isp, isp->digital_gain);
			isp->digital_gain_pending = false;
		} else {
			isp->digital_gain_pending = false;
		}
		spin_unlock_irqrestore(&isp->lock, flags);
		return 0;
	}
	return -EINVAL;
}

static int isp_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct fh8626_isp_dev *isp =
		container_of(ctrl->handler, struct fh8626_isp_dev, ctrls);
	unsigned long flags;

	if (ctrl->id != V4L2_CID_FH8626_STATS &&
	    ctrl->id != V4L2_CID_FH8626_AE_STATS &&
	    ctrl->id != V4L2_CID_FH8626_GRID_STATS)
		return -EINVAL;
	spin_lock_irqsave(&isp->lock, flags);
	if (ctrl->id == V4L2_CID_FH8626_STATS)
		memcpy(ctrl->p_new.p_u32, &isp->stats, sizeof(isp->stats));
	else if (ctrl->id == V4L2_CID_FH8626_AE_STATS)
		memcpy(ctrl->p_new.p_u32, &isp->ae_stats,
		       sizeof(isp->ae_stats));
	else
		memcpy(ctrl->p_new.p_u32, &isp->grid, sizeof(isp->grid));
	spin_unlock_irqrestore(&isp->lock, flags);
	return 0;
}

static const struct v4l2_ctrl_ops isp_ctrl_ops = {
	.s_ctrl = isp_s_ctrl,
	.g_volatile_ctrl = isp_g_volatile_ctrl,
};

static int isp_mask_try_ctrl(struct v4l2_ctrl *ctrl)
{
	const struct fh8626_privacy_mask *m = (const void *)ctrl->p_new.p_u32;
	unsigned int i;

	for (i = 0; i < FH8626_PRIVACY_MASKS; ++i, ++m) {
		if ((m->flags & ~FH8626_PRIVACY_MASK_ENABLE) ||
		    m->color > 0xffffff)
			return -EINVAL;
		if (!(m->flags & FH8626_PRIVACY_MASK_ENABLE))
			continue;
		if (!m->width || !m->height ||
		    m->left >= FH_FRAME_WIDTH_MAX ||
		    m->width > FH_FRAME_WIDTH_MAX - m->left ||
		    m->top >= FH_FRAME_HEIGHT_MAX ||
		    m->height > FH_FRAME_HEIGHT_MAX - m->top)
			return -ERANGE;
	}
	return 0;
}

static int isp_mask_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct fh8626_isp_dev *isp =
		container_of(ctrl->handler, struct fh8626_isp_dev, ctrls);
	const struct fh8626_privacy_mask *m = (const void *)ctrl->p_new.p_u32;
	struct fh8626_mask_rect rect[FH8626_PRIVACY_MASKS];
	bool any = false;
	unsigned int i;

	for (i = 0; i < FH8626_PRIVACY_MASKS; ++i) {
		rect[i].left = m[i].left;
		rect[i].top = m[i].top;
		rect[i].width = m[i].width;
		rect[i].height = m[i].height;
		rect[i].color = m[i].color;
		rect[i].on = m[i].flags & FH8626_PRIVACY_MASK_ENABLE;
		any |= rect[i].on;
	}
	/* The handler is set up before the VPU exists, with every mask off */
	if (!isp->vpu)
		return any ? -ENODEV : 0;
	return fh8626_vpu_set_privacy_masks(isp->vpu, rect);
}

static const struct v4l2_ctrl_ops isp_mask_ctrl_ops = {
	.try_ctrl = isp_mask_try_ctrl,
	.s_ctrl = isp_mask_s_ctrl,
};

static const struct v4l2_ctrl_config isp_mask_ctrl = {
	.ops = &isp_mask_ctrl_ops,
	.id = V4L2_CID_FH8626_PRIVACY_MASKS,
	.name = "Privacy masks",
	.type = V4L2_CTRL_TYPE_U32,
	.min = 0,
	.max = 0xffffffffU,
	.step = 1,
	.dims = { FH8626_PRIVACY_MASKS, FH8626_PRIVACY_MASK_WORDS },
};

static int isp_s_stream(struct v4l2_subdev *sd, int on)
{
	struct fh8626_isp_dev *isp = to_isp(sd);
	unsigned long flags;
	int ret = 0;

	mutex_lock(&isp->mutex);
	if (!!on == isp->streaming)
		goto out;
	if (on) {
		if (!isp->prepared || isp->dma_fault) {
			ret = -EIO;
			goto out;
		}
		writel(readl(isp->base + ISP_INT_STATUS),
		       isp->base + ISP_INT_STATUS);
		spin_lock_irqsave(&isp->lock, flags);
		isp->streaming = true;
		isp->quiescing = false;
		if (isp->color_pending)
			isp_apply_color(isp);
		if (isp->sharpness_pending)
			isp_apply_sharpness(isp);
		spin_unlock_irqrestore(&isp->lock, flags);
		/*
		 * Overflow stays asserted after the first frame and, if left
		 * enabled, is the only interrupt that keeps firing. Mask it so
		 * the picture start and VPU events are delivered.
		 */
		writel(~ISP_INT_OVERFLOW, isp->base + ISP_INT_ENABLE);
		writel(1, isp->base + ISP_CTRL);
		fh8626_isp_update_bits(isp, ISP_INPUT_CTRL, BIT(0), BIT(0));
	} else {
		/*
		 * Gate the input, wait about half a frame, then disable the
		 * ISP. The wait does not prove the pipeline has drained; the
		 * VPU stop must confirm that before DMA memory is released.
		 */
		fh8626_isp_update_bits(isp, ISP_INPUT_CTRL, BIT(0), 0);
		msleep(41);
		writel(0, isp->base + ISP_CTRL);
		fh8626_isp_update_bits(isp, ISP_NR3D_CTRL,
				       ISP_NR3D_FIRST_FRAME,
				       ISP_NR3D_FIRST_FRAME);
		fh8626_isp_update_bits(isp, ISP_BAYER_FMT,
				       ISP_BAYER_NR3D_BYPASS,
				       ISP_BAYER_NR3D_BYPASS);
		writel(0, isp->base + ISP_INT_ENABLE);
		synchronize_irq(isp->irq);
		spin_lock_irqsave(&isp->lock, flags);
		isp->streaming = false;
		isp->quiescing = false;
		isp->prepared = false;
		isp->nr3d_first_frame = false;
		isp->nr3d_pending = false;
		isp->color_pending = false;
		isp->sharpness_pending = false;
		isp->ynr_pending = false;
		isp->cnr_pending = false;
		isp->pfc_pending = false;
		isp->stats.flags = 0;
		isp->ae_stats.flags = 0;
		isp->grid.flags = 0;
		isp->stat_pending = false;
		isp->stat_error = false;
		isp->digital_gain_pending = false;
		spin_unlock_irqrestore(&isp->lock, flags);
	}
out:
	mutex_unlock(&isp->mutex);
	return ret;
}

static int isp_code_selector(u32 code)
{
	switch (code) {
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		return 0;
	case MEDIA_BUS_FMT_SGRBG10_1X10:
		return 1;
	case MEDIA_BUS_FMT_SBGGR10_1X10:
		return 2;
	case MEDIA_BUS_FMT_SGBRG10_1X10:
		return 3;
	default:
		return -EINVAL;
	}
}

static int isp_get_fmt(struct v4l2_subdev *sd,
		       struct v4l2_subdev_pad_config *cfg,
		       struct v4l2_subdev_format *f)
{
	struct fh8626_isp_dev *isp = to_isp(sd);

	if (f->pad > 1 ||
	    (f->which != V4L2_SUBDEV_FORMAT_TRY &&
	     f->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;
	mutex_lock(&isp->mutex);
	if (f->which == V4L2_SUBDEV_FORMAT_TRY) {
		f->format = *v4l2_subdev_get_try_format(sd, cfg, f->pad);
	} else {
		f->format = isp->format;
		if (f->pad) {
			f->format.code = MEDIA_BUS_FMT_YUYV8_1X16;
			f->format.colorspace = V4L2_COLORSPACE_SRGB;
			f->format.xfer_func = V4L2_XFER_FUNC_SRGB;
			f->format.ycbcr_enc = V4L2_YCBCR_ENC_601;
			f->format.quantization = V4L2_QUANTIZATION_LIM_RANGE;
		}
	}
	mutex_unlock(&isp->mutex);
	return 0;
}

static int isp_set_fmt(struct v4l2_subdev *sd,
		       struct v4l2_subdev_pad_config *cfg,
		       struct v4l2_subdev_format *f)
{
	struct fh8626_isp_dev *isp = to_isp(sd);
	struct v4l2_mbus_framefmt source;
	int selector;

	if (f->pad > 1 ||
	    (f->which != V4L2_SUBDEV_FORMAT_TRY &&
	     f->which != V4L2_SUBDEV_FORMAT_ACTIVE))
		return -EINVAL;
	if (f->pad)
		return isp_get_fmt(sd, cfg, f);
	selector = isp_code_selector(f->format.code);
	if (selector < 0)
		return selector;
	mutex_lock(&isp->mutex);
	if (f->which == V4L2_SUBDEV_FORMAT_ACTIVE &&
	    (isp->streaming || sd->entity.stream_count)) {
		mutex_unlock(&isp->mutex);
		return -EBUSY;
	}
	if (f->format.width > 1280 || f->format.height > 720) {
		f->format.width = 1920;
		f->format.height = 1080;
	} else {
		f->format.width = 1280;
		f->format.height = 720;
	}
	f->format.field = V4L2_FIELD_NONE;
	f->format.colorspace = V4L2_COLORSPACE_RAW;
	f->format.xfer_func = V4L2_XFER_FUNC_NONE;
	f->format.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	f->format.quantization = V4L2_QUANTIZATION_FULL_RANGE;
	memset(f->format.reserved, 0, sizeof(f->format.reserved));
	source = f->format;
	source.code = MEDIA_BUS_FMT_YUYV8_1X16;
	source.colorspace = V4L2_COLORSPACE_SRGB;
	source.xfer_func = V4L2_XFER_FUNC_SRGB;
	source.ycbcr_enc = V4L2_YCBCR_ENC_601;
	source.quantization = V4L2_QUANTIZATION_LIM_RANGE;
	if (f->which == V4L2_SUBDEV_FORMAT_TRY) {
		*v4l2_subdev_get_try_format(sd, cfg, 0) = f->format;
		*v4l2_subdev_get_try_format(sd, cfg, 1) = source;
	} else {
		isp->format = f->format;
		isp->bayer_pattern = selector;
		isp->prepared = false;
	}
	mutex_unlock(&isp->mutex);
	return 0;
}

static int isp_init_cfg(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg)
{
	struct v4l2_subdev_format f = {
		.which = V4L2_SUBDEV_FORMAT_TRY,
		.format.code = MEDIA_BUS_FMT_SRGGB10_1X10,
	};

	return isp_set_fmt(sd, cfg, &f);
}

static int isp_subscribe(struct v4l2_subdev *sd, struct v4l2_fh *fh,
			 struct v4l2_event_subscription *sub)
{
	if (sub->type == V4L2_EVENT_FRAME_SYNC && !sub->id)
		return v4l2_event_subscribe(fh, sub, 4, NULL);
	return v4l2_ctrl_subdev_subscribe_event(sd, fh, sub);
}

static int isp_enum_code(struct v4l2_subdev *sd,
			 struct v4l2_subdev_pad_config *cfg,
			 struct v4l2_subdev_mbus_code_enum *code)
{
	static const u32 codes[] = {
		MEDIA_BUS_FMT_SRGGB10_1X10,
		MEDIA_BUS_FMT_SGRBG10_1X10,
		MEDIA_BUS_FMT_SBGGR10_1X10,
		MEDIA_BUS_FMT_SGBRG10_1X10,
	};

	if (code->pad > 1 ||
	    code->index >= (code->pad ? 1 : ARRAY_SIZE(codes)))
		return -EINVAL;
	code->code = code->pad ? MEDIA_BUS_FMT_YUYV8_1X16 : codes[code->index];
	return 0;
}

static int isp_enum_frame_size(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
{
	static const struct {
		u32 width;
		u32 height;
	} sizes[] = {
		{ 1280, 720 },
		{ 1920, 1080 },
	};

	if (fse->pad > 1 || fse->index >= ARRAY_SIZE(sizes))
		return -EINVAL;

	if (fse->pad) {
		if (fse->code != MEDIA_BUS_FMT_YUYV8_1X16)
			return -EINVAL;
	} else {
		if (isp_code_selector(fse->code) < 0)
			return -EINVAL;
	}

	fse->min_width = sizes[fse->index].width;
	fse->max_width = sizes[fse->index].width;
	fse->min_height = sizes[fse->index].height;
	fse->max_height = sizes[fse->index].height;

	return 0;
}

static const struct media_entity_operations isp_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_subdev_core_ops isp_core_ops = {
	.subscribe_event = isp_subscribe,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_pad_ops isp_pad_ops = {
	.init_cfg = isp_init_cfg,
	.get_fmt = isp_get_fmt,
	.set_fmt = isp_set_fmt,
	.enum_mbus_code = isp_enum_code,
	.enum_frame_size = isp_enum_frame_size,
};

static const struct v4l2_subdev_video_ops isp_video_ops = {
	.s_stream = isp_s_stream,
};

static const struct v4l2_subdev_ops isp_ops = {
	.core = &isp_core_ops,
	.video = &isp_video_ops,
	.pad = &isp_pad_ops,
};

struct fh8626_isp_dev *fh8626_isp_from_subdev(struct v4l2_subdev *sd)
{
	return to_isp(sd);
}

struct fh8626_vpu_dev *fh8626_isp_vpu(struct fh8626_isp_dev *isp)
{
	return isp->vpu;
}

static void isp_free_nr3d_buffers(struct fh8626_isp_dev *isp)
{
	if (isp->nr3d_buf_cpu) {
		dma_free_coherent(isp->dev, isp->nr3d_buf_bytes,
				  isp->nr3d_buf_cpu, isp->nr3d_buf_dma);
		isp->nr3d_buf_cpu = NULL;
		isp->nr3d_buf_bytes = 0;
	}
	if (isp->nr3d_coeff_cpu) {
		dma_free_coherent(isp->dev, isp->nr3d_coeff_bytes,
				  isp->nr3d_coeff_cpu, isp->nr3d_coeff_dma);
		isp->nr3d_coeff_cpu = NULL;
		isp->nr3d_coeff_bytes = 0;
	}
}

/*
 * Size the 3DNR history for a frame. The buffers are reused when they match
 * and are replaced only while no stream can be writing them.
 */
static void isp_alloc_nr3d_buffers(struct fh8626_isp_dev *isp, u32 width,
				   u32 height)
{
	size_t buf_sz = ISP_NR3D_BUF_BYTES_FOR(width, height);
	size_t coeff_sz = ISP_NR3D_COEFF_BYTES_FOR(width, height);

	if (isp->nr3d_buf_cpu && isp->nr3d_coeff_cpu &&
	    isp->nr3d_buf_bytes == buf_sz && isp->nr3d_coeff_bytes == coeff_sz)
		return;
	isp_free_nr3d_buffers(isp);
	isp->nr3d_buf_cpu = dma_alloc_coherent(isp->dev, buf_sz,
					       &isp->nr3d_buf_dma, GFP_KERNEL);
	isp->nr3d_coeff_cpu = dma_alloc_coherent(isp->dev, coeff_sz,
						 &isp->nr3d_coeff_dma,
						 GFP_KERNEL);
	isp->nr3d_buf_bytes = isp->nr3d_buf_cpu ? buf_sz : 0;
	isp->nr3d_coeff_bytes = isp->nr3d_coeff_cpu ? coeff_sz : 0;
	if (!isp->nr3d_buf_cpu || !isp->nr3d_coeff_cpu) {
		isp_free_nr3d_buffers(isp);
		dev_warn(isp->dev, "3DNR buffer alloc failed, using bypass\n");
	}
}

static const struct v4l2_ctrl_config isp_int_ctrls[] = {
	{
		.id = V4L2_CID_FH8626_NR3D_NOISE, .name = "3D NR noise level",
		.type = V4L2_CTRL_TYPE_INTEGER, .min = 0,
		.max = ISP_NR3D_NOISE_MAX, .step = 1,
		.def = ISP_NR3D_NOISE_DEFAULT,
	}, {
		.id = V4L2_CID_FH8626_NR3D_NOISE_SHIFT,
		.name = "3D NR noise scale", .type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0, .max = 15, .step = 1, .def = ISP_NR3D_SHIFT_DEFAULT,
	}, {
		.id = V4L2_CID_FH8626_NR3D_MOTION,
		.name = "3D NR motion weight",
		.type = V4L2_CTRL_TYPE_INTEGER, .min = 0, .max = 255, .step = 1,
		.def = ISP_NR3D_MOTION_DEFAULT,
	}, {
		.id = V4L2_CID_FH8626_NR3D_BLEND, .name = "3D NR blend",
		.type = V4L2_CTRL_TYPE_INTEGER, .min = 0, .max = 255, .step = 1,
		.def = 0,
	}, {
		.id = V4L2_CID_FH8626_NR3D_PRESET,
		.name = "3D NR coefficient set",
		.type = V4L2_CTRL_TYPE_INTEGER, .min = 0, .max = 4, .step = 1,
		.def = 0,
	}, {
		.id = V4L2_CID_FH8626_BNR_STRENGTH, .name = "Bayer NR strength",
		.type = V4L2_CTRL_TYPE_INTEGER, .min = 0, .max = 255, .step = 1,
		.def = ISP_BNR_STRENGTH_DEFAULT,
	}, {
		.id = V4L2_CID_FH8626_BNR_NOISE, .name = "Bayer NR noise level",
		.type = V4L2_CTRL_TYPE_INTEGER, .min = 0,
		.max = ISP_NR3D_NOISE_MAX, .step = 1,
		.def = ISP_BNR_NOISE_DEFAULT,
	}, {
		.id = V4L2_CID_FH8626_BNR_NOISE_SHIFT,
		.name = "Bayer NR noise scale", .type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0, .max = 15, .step = 1, .def = ISP_BNR_SHIFT_DEFAULT,
	}, {
		.id = V4L2_CID_FH8626_HLR_THRESHOLD,
		.name = "Highlight chroma threshold",
		.type = V4L2_CTRL_TYPE_INTEGER, .min = 0,
		.max = FH8626_ISP_HLR_THRESHOLD_MAX, .step = 1,
		.def = FH8626_ISP_HLR_THRESHOLD_MAX,
	}, {
		.id = V4L2_CID_FH8626_CNR_STRENGTH,
		.name = "Chroma NR strength",
		.type = V4L2_CTRL_TYPE_INTEGER, .min = 0, .max = 255, .step = 1,
		.def = 0,
	}, {
		.id = V4L2_CID_FH8626_SATURATION_SCALE,
		.name = "Saturation policy scale",
		.type = V4L2_CTRL_TYPE_INTEGER, .min = 0, .max = 255, .step = 1,
		.def = ISP_SAT_SCALE_UNITY,
	}, {
		.id = V4L2_CID_FH8626_PFC_ENABLE,
		.name = "Purple fringe correction",
		.type = V4L2_CTRL_TYPE_BOOLEAN, .min = 0, .max = 1, .step = 1,
		.def = 1,
	}, {
		.id = V4L2_CID_FH8626_PFC_HUE, .name = "Purple fringe hue",
		.type = V4L2_CTRL_TYPE_INTEGER, .min = 0, .max = 359, .step = 1,
		.def = ISP_PFC_HUE_DEFAULT,
	}, {
		.id = V4L2_CID_FH8626_PFC_HUE_RANGE,
		.name = "Purple fringe hue range",
		.type = V4L2_CTRL_TYPE_INTEGER, .min = 1,
		.max = ISP_PFC_RANGE_MAX, .step = 1,
		.def = ISP_PFC_RANGE_DEFAULT,
	},
};

/*
 * Array controls whose elements do not share one default: seed the current
 * value from the driver state so that v4l2_ctrl_handler_setup() programs
 * the same values as prepare().
 */
static void isp_new_array_ctrls(struct fh8626_isp_dev *isp)
{
	struct v4l2_ctrl_config cfg = {
		.ops = &isp_ctrl_ops, .id = V4L2_CID_FH8626_BLACK_LEVEL,
		.name = "Black level", .type = V4L2_CTRL_TYPE_U16,
		.min = 0, .max = 4095, .step = 1,
		.dims = { FH8626_ISP_BLACK_LEVELS },
	};
	struct v4l2_ctrl *ctrl;
	unsigned int i;

	v4l2_ctrl_new_custom(&isp->ctrls, &cfg, NULL);

	cfg.id = V4L2_CID_FH8626_CCM;
	cfg.name = "Colour correction matrix";
	cfg.type = V4L2_CTRL_TYPE_INTEGER;
	cfg.min = -4096;
	cfg.max = 4095;
	cfg.dims[0] = FH8626_ISP_CCM_ENTRIES;
	ctrl = v4l2_ctrl_new_custom(&isp->ctrls, &cfg, NULL);
	if (ctrl)
		for (i = 0; i < FH8626_ISP_CCM_ENTRIES; ++i)
			ctrl->p_cur.p_s32[i] = ctrl->p_new.p_s32[i] =
				isp->ccm[i];

	cfg.type = V4L2_CTRL_TYPE_U16;
	cfg.min = 0;
	cfg.max = 1023;
	cfg.dims[0] = FH8626_ISP_GAMMA_POINTS;
	cfg.id = V4L2_CID_FH8626_GAMMA_RGB;
	cfg.name = "Colour tone curve";
	ctrl = v4l2_ctrl_new_custom(&isp->ctrls, &cfg, NULL);
	if (ctrl)
		for (i = 0; i < FH8626_ISP_GAMMA_POINTS; ++i)
			ctrl->p_cur.p_u16[i] = ctrl->p_new.p_u16[i] =
				isp->gamma_rgb[i];
	cfg.id = V4L2_CID_FH8626_GAMMA_Y;
	cfg.name = "Luma tone curve";
	ctrl = v4l2_ctrl_new_custom(&isp->ctrls, &cfg, NULL);
	if (ctrl)
		for (i = 0; i < FH8626_ISP_GAMMA_POINTS; ++i)
			ctrl->p_cur.p_u16[i] = ctrl->p_new.p_u16[i] =
				isp->gamma_y[i];

	cfg.id = V4L2_CID_FH8626_YNR_THRESHOLD;
	cfg.name = "Luma NR thresholds";
	cfg.max = 0xffff;
	cfg.dims[0] = FH8626_ISP_YNR_BANDS;
	ctrl = v4l2_ctrl_new_custom(&isp->ctrls, &cfg, NULL);
	if (ctrl)
		for (i = 0; i < FH8626_ISP_YNR_BANDS; ++i)
			ctrl->p_cur.p_u16[i] = ctrl->p_new.p_u16[i] =
				isp->ynr_threshold[i];
	cfg.id = V4L2_CID_FH8626_YNR_STRENGTH;
	cfg.name = "Luma NR strength";
	cfg.max = 1023;
	ctrl = v4l2_ctrl_new_custom(&isp->ctrls, &cfg, NULL);
	if (ctrl)
		for (i = 0; i < FH8626_ISP_YNR_BANDS; ++i)
			ctrl->p_cur.p_u16[i] = ctrl->p_new.p_u16[i] =
				isp->ynr_strength[i];
	cfg.id = V4L2_CID_FH8626_YNR_NOISE;
	cfg.name = "Luma NR noise levels";
	cfg.max = 255;
	cfg.def = ISP_YNR_NOISE_DEFAULT;
	cfg.dims[0] = FH8626_ISP_YNR_NOISE_POINTS;
	v4l2_ctrl_new_custom(&isp->ctrls, &cfg, NULL);
	cfg.id = V4L2_CID_FH8626_CNR_THRESHOLD;
	cfg.name = "Chroma NR thresholds";
	cfg.def = 0;
	cfg.dims[0] = FH8626_ISP_CNR_THRESHOLDS;
	v4l2_ctrl_new_custom(&isp->ctrls, &cfg, NULL);

	cfg.dims[0] = FH8626_ISP_APC_PATHS;
	cfg.id = V4L2_CID_FH8626_APC_STRENGTH;
	cfg.name = "Edge enhancement strength";
	ctrl = v4l2_ctrl_new_custom(&isp->ctrls, &cfg, NULL);
	if (ctrl)
		for (i = 0; i < FH8626_ISP_APC_PATHS; ++i)
			ctrl->p_cur.p_u16[i] = ctrl->p_new.p_u16[i] =
				isp->apc_strength[i];
	cfg.id = V4L2_CID_FH8626_APC_GAIN;
	cfg.name = "Edge enhancement gain";
	ctrl = v4l2_ctrl_new_custom(&isp->ctrls, &cfg, NULL);
	if (ctrl)
		for (i = 0; i < FH8626_ISP_APC_PATHS; ++i)
			ctrl->p_cur.p_u16[i] = ctrl->p_new.p_u16[i] =
				isp->apc_gain[i];
}

static int fh8626_isp_probe(struct platform_device *pdev)
{
	struct fh8626_isp_dev *isp;
	struct fh8626_isp_dev *faulted;
	struct resource *res;
	struct v4l2_ctrl_config stats_ctrl = {
		.ops = &isp_ctrl_ops, .id = V4L2_CID_FH8626_STATS,
		.name = "ISP frame statistics", .type = V4L2_CTRL_TYPE_U32,
		.min = 0, .max = 0xffffffffU, .step = 1,
		.dims = { FH8626_ISP_STATS_WORDS },
		.flags = V4L2_CTRL_FLAG_READ_ONLY | V4L2_CTRL_FLAG_VOLATILE,
	};
	struct v4l2_ctrl_config nr3d_ctrl = {
		.ops = &isp_ctrl_ops, .id = V4L2_CID_FH8626_NR3D_STRENGTH,
		.name = "3D noise reduction",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0, .max = 255, .step = 1,
		.def = ISP_NR3D_STRENGTH_DEFAULT,
	};
	struct v4l2_ctrl_config green_ctrl = {
		.ops = &isp_ctrl_ops, .id = V4L2_CID_FH8626_WB_GAIN_G,
		.name = "White balance green",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 1, .max = 0x1fff, .step = 1, .def = 512,
	};
	unsigned int i;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res || resource_size(res) < 0x4000)
		return -EINVAL;
	mutex_lock(&faulted_isps_lock);
	list_for_each_entry(faulted, &faulted_isps, quarantine) {
		if (faulted->phys == res->start) {
			mutex_unlock(&faulted_isps_lock);
			return -EIO;
		}
	}
	mutex_unlock(&faulted_isps_lock);
	isp = kzalloc(sizeof(*isp), GFP_KERNEL);
	if (!isp)
		return -ENOMEM;
	isp->dev = get_device(&pdev->dev);
	isp->phys = res->start;
	kref_init(&isp->ref);
	isp->lifetime.ref = &isp->ref;
	isp->lifetime.release = isp_release;
	INIT_LIST_HEAD(&isp->quarantine);
	init_waitqueue_head(&isp->quiesce_wq);
	mutex_init(&isp->mutex);
	spin_lock_init(&isp->lock);
	v4l2_subdev_init(&isp->sd, &isp_ops);
	v4l2_set_subdev_hostdata(&isp->sd, &isp->lifetime);
	isp->sd.internal_ops = &isp_internal_ops;
	isp->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(isp->base)) {
		ret = PTR_ERR(isp->base);
		goto err_put;
	}
	isp->irq = platform_get_irq(pdev, 0);
	if (isp->irq < 0) {
		ret = isp->irq;
		goto err_put;
	}
	isp->clk = devm_clk_get(&pdev->dev, "aclk");
	isp->hclk = devm_clk_get(&pdev->dev, "hclk");
	if (IS_ERR(isp->clk) || IS_ERR(isp->hclk)) {
		ret = IS_ERR(isp->clk) ? PTR_ERR(isp->clk) :
					 PTR_ERR(isp->hclk);
		goto err_put;
	}
	ret = clk_prepare_enable(isp->clk);
	if (ret)
		goto err_put;
	ret = clk_prepare_enable(isp->hclk);
	if (ret)
		goto err_clk;
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		goto err_hclk;
	isp->stat_cpu = dma_alloc_coherent(&pdev->dev, ISP_STAT_BYTES,
					   &isp->stat_dma, GFP_KERNEL);
	if (!isp->stat_cpu) {
		ret = -ENOMEM;
		goto err_hclk;
	}
	isp_alloc_nr3d_buffers(isp, FH_FRAME_WIDTH_DEFAULT,
			       FH_FRAME_HEIGHT_DEFAULT);
	writel(0, isp->base + ISP_INT_ENABLE);
	isp->sd.owner = THIS_MODULE;
	isp->sd.dev = &pdev->dev;
	isp->sd.fwnode = of_fwnode_handle(pdev->dev.of_node);
	strlcpy(isp->sd.name, "fh8626-isp", sizeof(isp->sd.name));
	isp->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
			 V4L2_SUBDEV_FL_HAS_EVENTS;
	isp->format.width = FH_FRAME_WIDTH_DEFAULT;
	isp->format.height = FH_FRAME_HEIGHT_DEFAULT;
	isp->format.code = MEDIA_BUS_FMT_SRGGB10_1X10;
	isp->format.field = V4L2_FIELD_NONE;
	isp->format.colorspace = V4L2_COLORSPACE_RAW;
	isp->format.xfer_func = V4L2_XFER_FUNC_NONE;
	isp->format.quantization = V4L2_QUANTIZATION_FULL_RANGE;
	isp->nr3d_strength = ISP_NR3D_STRENGTH_DEFAULT;
	isp->nr3d_noise = ISP_NR3D_NOISE_DEFAULT;
	isp->nr3d_shift = ISP_NR3D_SHIFT_DEFAULT;
	isp->nr3d_motion = ISP_NR3D_MOTION_DEFAULT;
	isp->bnr_strength = ISP_BNR_STRENGTH_DEFAULT;
	isp->bnr_noise = ISP_BNR_NOISE_DEFAULT;
	isp->bnr_shift = ISP_BNR_SHIFT_DEFAULT;
	isp->hlr_threshold = FH8626_ISP_HLR_THRESHOLD_MAX;
	BUILD_BUG_ON(FH8626_ISP_YNR_BANDS != ISP_YNR_BANDS);
	memcpy(isp->ynr_threshold, isp_ynr_threshold_default,
	       sizeof(isp->ynr_threshold));
	memcpy(isp->ynr_strength, isp_ynr_strength_default,
	       sizeof(isp->ynr_strength));
	isp->ynr_noise[0] = isp->ynr_noise[1] = ISP_YNR_NOISE_DEFAULT;
	isp->apc_strength[0] = ISP_APC_DETAIL_STRENGTH_DEFAULT;
	isp->apc_strength[1] = ISP_APC_EDGE_STRENGTH_DEFAULT;
	isp->apc_gain[0] = ISP_APC_DETAIL_GAIN_DEFAULT;
	isp->apc_gain[1] = ISP_APC_EDGE_GAIN_DEFAULT;
	isp->sat_scale = ISP_SAT_SCALE_UNITY;
	isp->pfc_enable = true;
	isp->pfc_hue = ISP_PFC_HUE_DEFAULT;
	isp->pfc_range = ISP_PFC_RANGE_DEFAULT;
	isp->ccm[0] = isp->ccm[4] = isp->ccm[8] = ISP_CCM_UNITY;
	BUILD_BUG_ON(ARRAY_SIZE(fh8626_isp_gamma_y) != FH8626_ISP_GAMMA_POINTS);
	BUILD_BUG_ON(ARRAY_SIZE(fh8626_isp_gamma_rgb) != FH8626_ISP_GAMMA_POINTS);
	BUILD_BUG_ON(FH8626_ISP_GAMMA_POINTS != 2 * ISP_GAMMA_WORDS);
	BUILD_BUG_ON(FH8626_ISP_CCM_ENTRIES > 2 * ISP_CCM_WORDS);
	memcpy(isp->gamma_rgb, fh8626_isp_gamma_rgb, sizeof(isp->gamma_rgb));
	memcpy(isp->gamma_y, fh8626_isp_gamma_y, sizeof(isp->gamma_y));
	isp->brightness = 128;
	isp->contrast = 128;
	isp->saturation = 128;
	isp->sharpness = 128;
	isp->auto_exposure = V4L2_EXPOSURE_AUTO;
	isp->auto_wb = true;
	isp->stats.version = FH8626_ISP_STATS_VERSION;
	isp->ae_stats.version = FH8626_ISP_STATS_VERSION;
	isp->grid.version = FH8626_ISP_STATS_VERSION;
	isp->grid.cols = FH8626_ISP_GRID_COLS;
	isp->grid.rows = FH8626_ISP_GRID_ROWS;
	isp->grid.motion_cols = FH8626_ISP_MOTION_COLS;
	isp->grid.motion_rows = FH8626_ISP_MOTION_ROWS;
	v4l2_ctrl_handler_init(&isp->ctrls, 48);
	isp->wb[0] = v4l2_ctrl_new_std(&isp->ctrls, &isp_ctrl_ops,
			V4L2_CID_RED_BALANCE, 1, 0x1fff, 1, 512);
	isp->wb[1] = v4l2_ctrl_new_std(&isp->ctrls, &isp_ctrl_ops,
			V4L2_CID_BLUE_BALANCE, 1, 0x1fff, 1, 512);
	isp->wb[2] = v4l2_ctrl_new_custom(&isp->ctrls, &green_ctrl, NULL);
	isp->digital_gain = 64;
	v4l2_ctrl_new_std(&isp->ctrls, &isp_ctrl_ops,
			V4L2_CID_DIGITAL_GAIN, 64, 1023, 1, 64);
	v4l2_ctrl_new_std(&isp->ctrls, &isp_ctrl_ops,
			V4L2_CID_BRIGHTNESS, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&isp->ctrls, &isp_ctrl_ops,
			V4L2_CID_CONTRAST, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&isp->ctrls, &isp_ctrl_ops,
			V4L2_CID_SATURATION, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&isp->ctrls, &isp_ctrl_ops,
			V4L2_CID_SHARPNESS, 0, 255, 1, 128);
	v4l2_ctrl_new_std_menu(&isp->ctrls, &isp_ctrl_ops,
			V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_MANUAL, 0,
			V4L2_EXPOSURE_AUTO);
	v4l2_ctrl_new_std(&isp->ctrls, &isp_ctrl_ops,
			V4L2_CID_AUTO_WHITE_BALANCE, 0, 1, 1, 1);
	v4l2_ctrl_new_custom(&isp->ctrls, &nr3d_ctrl, NULL);
	v4l2_ctrl_new_std_menu(&isp->ctrls, &isp_ctrl_ops,
			V4L2_CID_POWER_LINE_FREQUENCY,
			V4L2_CID_POWER_LINE_FREQUENCY_60HZ, 0,
			V4L2_CID_POWER_LINE_FREQUENCY_DISABLED);
	for (i = 0; i < ARRAY_SIZE(isp_int_ctrls); ++i) {
		struct v4l2_ctrl_config cfg = isp_int_ctrls[i];

		cfg.ops = &isp_ctrl_ops;
		v4l2_ctrl_new_custom(&isp->ctrls, &cfg, NULL);
	}
	isp_new_array_ctrls(isp);
	BUILD_BUG_ON(sizeof(struct fh8626_privacy_mask) !=
		     FH8626_PRIVACY_MASK_WORDS * sizeof(u32));
	v4l2_ctrl_new_custom(&isp->ctrls, &isp_mask_ctrl, NULL);
	v4l2_ctrl_new_custom(&isp->ctrls, &stats_ctrl, NULL);
	stats_ctrl.id = V4L2_CID_FH8626_AE_STATS;
	stats_ctrl.name = "ISP AE frame statistics";
	v4l2_ctrl_new_custom(&isp->ctrls, &stats_ctrl, NULL);
	stats_ctrl.id = V4L2_CID_FH8626_GRID_STATS;
	stats_ctrl.name = "ISP zone statistics";
	stats_ctrl.dims[0] = FH8626_ISP_GRID_STATS_WORDS;
	v4l2_ctrl_new_custom(&isp->ctrls, &stats_ctrl, NULL);
	if (isp->ctrls.error) {
		ret = isp->ctrls.error;
		goto err_ctrl;
	}
	v4l2_ctrl_cluster(ARRAY_SIZE(isp->wb), isp->wb);
	isp->sd.ctrl_handler = &isp->ctrls;
	ret = v4l2_ctrl_handler_setup(&isp->ctrls);
	if (ret)
		goto err_ctrl;
	isp->pads[0].flags = MEDIA_PAD_FL_SINK;
	isp->pads[1].flags = MEDIA_PAD_FL_SOURCE;
	isp->sd.entity.function = MEDIA_ENT_F_PROC_VIDEO_PIXEL_FORMATTER;
	isp->sd.entity.ops = &isp_entity_ops;
	ret = media_entity_pads_init(&isp->sd.entity, 2, isp->pads);
	if (ret)
		goto err_ctrl;
	isp->vpu = fh8626_vpu_create(&pdev->dev, isp);
	if (IS_ERR(isp->vpu)) {
		ret = PTR_ERR(isp->vpu);
		isp->vpu = NULL;
		goto err_ctrl;
	}
	ret = devm_request_threaded_irq(&pdev->dev, isp->irq, NULL,
					isp_irq_thread, IRQF_ONESHOT,
					dev_name(&pdev->dev), isp);
	if (ret)
		goto err_vpu;
	ret = v4l2_async_register_subdev(&isp->sd);
	if (ret) {
		dev_err(&pdev->dev, "failed to register async subdev: %d\n",
			ret);
		goto err_irq;
	}
	platform_set_drvdata(pdev, isp);
	return 0;
err_irq:
	devm_free_irq(&pdev->dev, isp->irq, isp);
	if (isp->dma_fault) {
		mutex_lock(&faulted_isps_lock);
		list_add(&isp->quarantine, &faulted_isps);
		mutex_unlock(&faulted_isps_lock);
		return ret;
	}
err_vpu:
	fh8626_vpu_destroy(isp->vpu);
	isp->vpu = NULL;
err_ctrl:
	isp_free_nr3d_buffers(isp);
	dma_free_coherent(&pdev->dev, ISP_STAT_BYTES, isp->stat_cpu,
			  isp->stat_dma);
err_hclk:
	clk_disable_unprepare(isp->hclk);
err_clk:
	clk_disable_unprepare(isp->clk);
err_put:
	kref_put(&isp->ref, isp_release);
	return ret;
}

static int fh8626_isp_remove(struct platform_device *pdev)
{
	struct fh8626_isp_dev *isp = platform_get_drvdata(pdev);

	v4l2_async_unregister_subdev(&isp->sd);
	isp_s_stream(&isp->sd, 0);
	fh8626_vpu_pipeline_stop(isp->vpu);
	devm_free_irq(&pdev->dev, isp->irq, isp);
	platform_set_drvdata(pdev, NULL);
	if (isp->dma_fault) {
		/* Keep the object, DMA memory and module pinned until reboot */
		mutex_lock(&faulted_isps_lock);
		list_add(&isp->quarantine, &faulted_isps);
		mutex_unlock(&faulted_isps_lock);
		return 0;
	}
	fh8626_vpu_destroy(isp->vpu);
	isp_free_nr3d_buffers(isp);
	dma_free_coherent(&pdev->dev, ISP_STAT_BYTES, isp->stat_cpu,
			  isp->stat_dma);
	clk_disable_unprepare(isp->hclk);
	clk_disable_unprepare(isp->clk);
	kref_put(&isp->ref, isp_release);
	return 0;
}

static const struct of_device_id fh8626_isp_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-isp" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh8626_isp_of_match);

struct platform_driver fh8626_isp_driver = {
	.probe = fh8626_isp_probe,
	.remove = fh8626_isp_remove,
	.driver = {
		.name = "fh8626-isp",
		.suppress_bind_attrs = true,
		.of_match_table = fh8626_isp_of_match,
	},
};
