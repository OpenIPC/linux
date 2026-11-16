// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang
#include <linux/delay.h>
#include <linux/videodev2.h>

#include "tx-isp-device.h"

struct t23_isp_reg {
	u32 offset;
	u32 value;
};

/* SC1A4T linear-mode demosaic coefficients. */
static const struct t23_isp_reg dmsc_linear[] = {
	{ 0x4800, 0x00000000 },
	{ 0x4854, 0x00001fff },
	{ 0x4858, 0x00001fff },
	{ 0x486c, 0x00001fff },
	{ 0x4870, 0x00001fff },
	{ 0x4890, 0x00200820 },
	{ 0x48b0, 0x08080601 },
	{ 0x48b4, 0x08000233 },
	{ 0x4944, 0x00000000 },
	{ 0x4950, 0x00000000 },
	{ 0x4954, 0x00000000 },
	{ 0x4958, 0x00000000 },
	{ 0x495c, 0x00000000 },
	{ 0x4960, 0x00000000 },
	{ 0x4964, 0x00000000 },
	{ 0x4968, 0x00000000 },
	{ 0x496c, 0x00000000 },
	{ 0x4970, 0x00000000 },
	{ 0x4974, 0x00000000 },
	{ 0x4978, 0x00000000 },
	{ 0x497c, 0x00000000 },
	{ 0x4984, 0x04000400 },
	{ 0x49a8, 0x00000000 },
	{ 0x49c4, 0x0fff00c8 },
	{ 0x49d0, 0x0fff00c8 },
	{ 0x49dc, 0x0fff00c8 },
	{ 0x49e0, 0x0000001f },
	{ 0x49f8, 0x00010010 },
	{ 0x4804, 0x000078c8 },
	{ 0x4808, 0x00c80078 },
	{ 0x480c, 0x00640064 },
	{ 0x4810, 0x00000058 },
	{ 0x4814, 0x00000000 },
	{ 0x4820, 0x7f6404b0 },
	{ 0x4824, 0x00000000 },
	{ 0x4828, 0x7f6404b0 },
	{ 0x482c, 0x00000000 },
	{ 0x4830, 0x7f6404b0 },
	{ 0x4834, 0x07d3feb8 },
	{ 0x4838, 0x0fa0001e },
	{ 0x483c, 0x000000fe },
	{ 0x4840, 0x00040100 },
	{ 0x4844, 0x00100600 },
	{ 0x4848, 0x00640078 },
	{ 0x484c, 0x00c800c8 },
	{ 0x4850, 0x0fff0000 },
	{ 0x485c, 0x00080080 },
	{ 0x4860, 0x005a005a },
	{ 0x4864, 0x00200020 },
	{ 0x4868, 0x0fff0000 },
	{ 0x4874, 0x00fe0000 },
	{ 0x4878, 0x00fe0064 },
	{ 0x487c, 0x000003df },
	{ 0x4880, 0x00100000 },
	{ 0x4884, 0x00fe00c8 },
	{ 0x4888, 0x07d000c0 },
	{ 0x488c, 0x033d0f9f },
	{ 0x489c, 0x00640f3c },
	{ 0x48a4, 0x0fff003c },
	{ 0x48ac, 0x0000003c },
	{ 0x48b8, 0x00c82420 },
	{ 0x48c4, 0x00002003 },
	{ 0x4948, 0x0f9f0000 },
	{ 0x494c, 0x0f9f0000 },
	{ 0x4980, 0x00000000 },
	{ 0x4988, 0x0320000a },
	{ 0x49ac, 0x00060005 },
	{ 0x49b0, 0x00280090 },
	{ 0x49b4, 0x000a0014 },
	{ 0x49b8, 0x00000fff },
	{ 0x49bc, 0x00000014 },
	{ 0x49c0, 0x0008003c },
	{ 0x49c8, 0x00000078 },
	{ 0x49cc, 0x0008003c },
	{ 0x49d4, 0x000000f0 },
	{ 0x49d8, 0x0008003c },
	{ 0x49e4, 0x0fff012c },
	{ 0x49e8, 0x002b0113 },
	{ 0x49ec, 0x00000000 },
	{ 0x49f0, 0x0fff0000 },
	{ 0x49f4, 0x0000012c },
	{ 0x49fc, 0x000f0000 },
	{ 0x4a00, 0x00000000 },
	{ 0x4a04, 0x000ff020 },
	{ 0x4a08, 0x00000030 },
	{ 0x48c8, 0x00000000 },
	{ 0x48cc, 0x00000000 },
	{ 0x48d0, 0x00000000 },
	{ 0x48d4, 0x00000000 },
	{ 0x48d8, 0x00000000 },
	{ 0x48dc, 0x00000000 },
	{ 0x48e0, 0x00000000 },
	{ 0x48e4, 0x00000000 },
	{ 0x48f4, 0x1d65540c },
	{ 0x48f8, 0x208207de },
	{ 0x48fc, 0x20820820 },
	{ 0x4900, 0x20820820 },
	{ 0x4904, 0x20820820 },
	{ 0x4908, 0x20820820 },
	{ 0x490c, 0x00000820 },
	{ 0x491c, 0x10820820 },
	{ 0x4920, 0x02208410 },
	{ 0x4924, 0x00000042 },
	{ 0x4928, 0x00000000 },
	{ 0x492c, 0x00000000 },
	{ 0x4930, 0x00000000 },
	{ 0x4934, 0x00000000 },
	{ 0x4a0c, 0x01000100 },
	{ 0x4a10, 0x01000100 },
	{ 0x4a14, 0x01000100 },
	{ 0x4a18, 0x01000100 },
	{ 0x4a1c, 0x01000100 },
	{ 0x4a20, 0x01000100 },
	{ 0x4a24, 0x01000100 },
	{ 0x4a28, 0x01000100 },
	{ 0x4a2c, 0x01000100 },
	{ 0x4a30, 0x01000100 },
	{ 0x4a34, 0x01000100 },
	{ 0x4a38, 0x01000100 },
	{ 0x4a3c, 0x01000100 },
	{ 0x4a40, 0x01000100 },
	{ 0x4a44, 0x01000100 },
	{ 0x4a48, 0x01000100 },
	{ 0x4a4c, 0x01000100 },
	{ 0x4a50, 0x01000100 },
	{ 0x4a54, 0x01000100 },
	{ 0x4a58, 0x01000100 },
	{ 0x4a5c, 0x01000100 },
	{ 0x499c, 0x00000001 },
};

/*
 * Scaler filter kernel: 257 samples from the tap centre to two taps out,
 * 1536 at the centre.
 */
static const s16 msca_kernel[257] = {
	1535, 1534, 1532, 1528, 1523, 1516, 1507, 1497,
	1486, 1473, 1459, 1444, 1427, 1409, 1389, 1369,
	1347, 1324, 1300, 1274, 1248, 1221, 1194, 1165,
	1135, 1105, 1074, 1043, 1011, 979, 946, 913,
	880, 847, 813, 779, 745, 711, 678, 645,
	612, 579, 546, 514, 483, 452, 421, 391,
	361, 332, 304, 277, 251, 225, 200, 175,
	152, 130, 109, 89, 69, 50, 32, 16,
	0, -15, -29, -41, -53, -64, -75, -84,
	-92, -99, -106, -112, -117, -121, -125, -128,
	-130, -131, -132, -132, -132, -131, -130, -128,
	-126, -124, -121, -118, -114, -110, -106, -102,
	-98, -94, -89, -84, -79, -75, -70, -65,
	-60, -55, -51, -47, -43, -39, -35, -31,
	-27, -24, -21, -18, -15, -12, -10, -8,
	-7, -5, -4, -3, -2, -1, 0, 0,
	0, 0, 0, 0, -1, -2, -3, -4,
	-5, -7, -8, -9, -10, -12, -14, -16,
	-17, -19, -20, -22, -23, -25, -26, -27,
	-28, -30, -31, -32, -33, -34, -34, -35,
	-35, -36, -36, -36, -36, -36, -36, -36,
	-35, -34, -33, -33, -32, -31, -29, -28,
	-27, -26, -24, -23, -21, -20, -18, -16,
	-14, -13, -11, -9, -7, -5, -3, -1,
	0, 2, 3, 5, 6, 8, 9, 11,
	12, 13, 14, 15, 16, 17, 18, 19,
	19, 20, 20, 21, 21, 21, 21, 21,
	21, 21, 21, 21, 20, 20, 19, 19,
	18, 18, 17, 16, 15, 15, 14, 13,
	12, 12, 11, 10, 9, 8, 7, 6,
	6, 6, 5, 4, 3, 2, 2, 2,
	2, 2, 1, 0, 0, 0, 0, 0,
	0,
};

#define MSCA_PHASES		8
#define MSCA_COEFFS		(2 * MSCA_PHASES + 1)
#define MSCA_COEFF_WORDS	((MSCA_COEFFS + 1) / 2)
#define MSCA_STEP_ONE		0x4000	/* output/input ratio of 1 */
#define MSCA_RATIO_MAX		0x6aaa

/*
 * Sample the kernel for an output/input @ratio. Downscaling stretches it
 * over more input pixels, which band-limits the output.
 */
static void msca_kernel_sample(u32 ratio, s16 coeff[MSCA_COEFFS])
{
	u32 step = ratio / 2, start = 0, end = 0x400, pos, frac;
	unsigned int i = 0, seg = 0;
	s32 v;

	while (i < MSCA_COEFFS) {
		pos = i * step;
		if (pos >= end) {
			start = end;
			end += 0x400;
			seg++;
			continue;
		}
		frac = pos - start;
		v = msca_kernel[seg + 1] * (s32)frac +
		    msca_kernel[seg] * (s32)(0x400 - frac);
		coeff[i++] = (v >> 10) + ((v >> 9) & 1);
	}
}

/* Scale the four taps of each phase to a gain of exactly 2048. */
static void msca_kernel_normalize(s16 coeff[MSCA_COEFFS])
{
	static const int offset[4] = { -MSCA_PHASES, 0, MSCA_PHASES, 2 * MSCA_PHASES };
	static const u8 raise[4] = { 1, 2, 0, 3 }, lower[4] = { 3, 0, 2, 1 };
	s16 out[MSCA_COEFFS] = { 0 };
	unsigned int phase, k;

	for (phase = 0; phase < MSCA_PHASES; phase++) {
		int tap[4], v[4], sum = 0, error = -2048;

		for (k = 0; k < 4; k++) {
			tap[k] = abs(offset[k] - (int)phase);
			sum += coeff[tap[k]];
		}
		for (k = 0; k < 4; k++) {
			int half = coeff[tap[k]] < 0 ? -(sum / 2) : sum / 2;

			v[k] = (coeff[tap[k]] * 2048 + half) / sum;
			error += v[k];
		}
		/* Rounding leaves at most two units over or under. */
		for (k = 0; error < 0; k++, error++)
			v[raise[k]]++;
		for (k = 0; error > 0; k++, error--)
			v[lower[k]]--;
		for (k = 0; k < 4; k++)
			out[tap[k]] = v[k];
	}
	memcpy(coeff, out, sizeof(out));
}

/* Queue one direction's filter on the indirect port at @address. */
static void msca_write_filter(void __iomem *base, u32 address, u32 ratio)
{
	s16 coeff[MSCA_COEFFS + 1];
	unsigned int i;

	msca_kernel_sample(min_t(u32, ratio, MSCA_RATIO_MAX), coeff);
	msca_kernel_normalize(coeff);
	coeff[MSCA_COEFFS] = 0;
	for (i = 0; i < MSCA_COEFF_WORDS; i++) {
		writel((coeff[2 * i] & (u16)MSCA_COEF_LO) |
		       (u32)(coeff[2 * i + 1] & (u16)MSCA_COEF_LO) << 16,
		       base + MSCA_SHD_FIFO);
		writel(address + i * 4, base + MSCA_SHD_FIFO);
	}
}

static const struct t23_isp_reg csc_bt601_full[] = {
	{ 0x6000, 0x0000001f },
	{ 0x6004, 0x00000000 },
	{ 0x6010, 0x07596532 },
	{ 0x6014, 0x20054cad },
	{ 0x6018, 0x0536b600 },
	{ 0x6020, 0x00008000 },
	{ 0x6030, 0xff00ff00 },
};

void tx_isp_core_dmsc_defaults(u32 values[3])
{
	values[0] = ISP_DMSC_REG_08_DEFAULT;
	values[1] = ISP_DMSC_REG_48_DEFAULT;
	values[2] = ISP_DMSC_REG_60_DEFAULT;
}

static void t23_write_regs(void __iomem *base,
			  const struct t23_isp_reg *regs, unsigned int count)
{
	unsigned int i;

	for (i = 0; i < count; i++)
		writel(regs[i].value, base + regs[i].offset);
}

static void t23_msca_aux_init(void __iomem *base)
{
	unsigned int ch, mask, offset;

	/* No mask, line or OSD plane owns a buffer at stream start. */
	for (ch = 0; ch < 3; ch++) {
		for (mask = 0; mask < 4; mask++) {
			offset = MSCA_MASK(ch, mask);
			writel(0, base + offset);
			writel(0, base + offset + 8);
			writel(0, base + offset + 4);
		}
		writel(0, base + MSCA_MASK_EN);
	}
	for (offset = MSCA_LINE(0); offset <= MSCA_LINE(MSCA_LINES - 1) + 0xc; offset += 4)
		writel(0, base + offset);
	writel(0, base + MSCA_OSD_ORDER);
	for (offset = MSCA_OSD_BLOCK(0); offset <= MSCA_OSD_BLOCK(MSCA_OSD_BLOCKS - 1) + 0xc; offset += 4)
		writel(0, base + offset);
	writel(0, base + MSCA_OSD_EN);
	writel(MSCA_OSD_CTRL_DEFAULT, base + MSCA_OSD_CTRL);
}

/* Channel disable: hold the line flush, drop the channel bit, trigger. */
static void t23_msca_channel_disable(void __iomem *base, unsigned int ch)
{
	writel(1, base + MSCA_Y_RESET(ch));
	writel(1, base + MSCA_UV_RESET(ch));
	writel(readl(base + MSCA_CH_EN_MASK) & ~MSCA_CH_EN(ch),
	       base + MSCA_CH_EN_MASK);
	writel(1, base + MSCA_SHD_TRIGGER);
	writel(1, base + MSCA_CH_REG_4C(ch));
	writel(1, base + MSCA_CH_REG_6C(ch));
}

static int t23_msca_channel_configure(struct tx_isp_subdev *sd)
{
	struct tx_isp_core_device *core = sd->dev_priv;
	void __iomem *base = sd->base;
	u32 width = core->geometry >> 16;
	u32 height = core->geometry & 0xffff;
	u32 posted = ((height & 0x1fff) << 16) | (width & 0xfff);
	unsigned int ch;

	/*
	 * Channel 0 stays disabled. Its size registers keep the 1920x1080
	 * reset value until a VIC frame commits the shadow.
	 */
	writel(0, base + MSCA_FCROP_POS);
	writel(posted, base + MSCA_FCROP_SIZE);
	writel(MSCA_GEOM(1280, 720), base + MSCA_CH_SCALE_SIZE(1));
	writel(MSCA_GEOM(1280, 720), base + MSCA_CH_CROP_SIZE(1));
	writel(MSCA_GEOM(1280, 720), base + MSCA_CH_SCALE_SIZE(2));
	writel(MSCA_GEOM(1280, 720), base + MSCA_CH_CROP_SIZE(2));
	for (ch = 0; ch < 3; ch++)
		t23_msca_channel_disable(base, ch);
	writel(ISP_TOP_REG_33C_DEFAULT, base + ISP_TOP_REG_33C);
	dev_dbg_once(&g_tx_isp_dev->pdev->dev,
		     "t23 MSCA post: run=%08x wrote=%08x geom=%08x ch1=%08x mode=%08x trig=%08x en=%08x\n",
		     readl(base + ISP_IPC_TRIGGER), posted, readl(base + MSCA_FCROP_SIZE),
		     readl(base + MSCA_CH_SCALE_SIZE(1)), readl(base + MSCA_REG_060),
		     readl(base + MSCA_SHD_TRIGGER),
		     readl(base + MSCA_CH_EN_MASK));
	t23_msca_aux_init(base);
	return 0;
}

int tx_isp_core_output_init(struct tx_isp_subdev *sd)
{
	void __iomem *base = sd->base;
	unsigned int ch;

	writel(0, base + MSCA_CH_EN_MASK);
	for (ch = 0; ch < 3; ch++) {
		writel(1, base + MSCA_Y_RESET(ch));
		writel(1, base + MSCA_UV_RESET(ch));
	}
	writel(ISP_DGAIN_LUT_PARAM0_DEFAULT, base + ISP_DGAIN_LUT_PARAM0);
	writel(ISP_DGAIN_LUT_PARAM1_DEFAULT, base + ISP_DGAIN_LUT_PARAM1);
	writel(ISP_DGAIN_TRIGGER_COMMIT, base + ISP_DGAIN_TRIGGER);
	writel(0, base + ISP_DGAIN_BLC3);
	writel(ISP_DGAIN_TRIGGER_COMMIT, base + ISP_DGAIN_TRIGGER);
	writel(ISP_DGAIN_GAIN_ONE, base + ISP_DGAIN_GAIN01);
	writel(ISP_DGAIN_TRIGGER_COMMIT, base + ISP_DGAIN_TRIGGER);
	writel(ISP_DGAIN_GAIN_ONE, base + ISP_DGAIN_GAIN23);
	writel(ISP_DGAIN_TRIGGER_COMMIT, base + ISP_DGAIN_TRIGGER);
	writel(ISP_DGAIN_BLC_ONE, base + ISP_DGAIN_BLC0);
	writel(ISP_DGAIN_TRIGGER_COMMIT, base + ISP_DGAIN_TRIGGER);
	writel(ISP_DGAIN_BLC_PAIR_ONE, base + ISP_DGAIN_BLC1);
	writel(ISP_DGAIN_TRIGGER_COMMIT, base + ISP_DGAIN_TRIGGER);
	writel(ISP_DGAIN_BLC_ONE, base + ISP_DGAIN_BLC2);
	t23_write_regs(base, dmsc_linear, ARRAY_SIZE(dmsc_linear));
	t23_write_regs(base, csc_bt601_full, ARRAY_SIZE(csc_bt601_full));
	return t23_msca_channel_configure(sd);
}

void tx_isp_core_restore_dmsc(struct tx_isp_subdev *sd)
{
	if (!sd || !sd->base)
		return;
	t23_write_regs(sd->base, dmsc_linear, ARRAY_SIZE(dmsc_linear));
}

static void t23_msca_snapshot(struct tx_isp_subdev *sd, const char *stage)
{
	void __iomem *channel = sd->base;

	pr_debug("t23 MSCA %s: run=%08x mode=%08x geom=%08x/%08x/%08x shd_trig=%08x shd_dma=%08x/%08x en=%08x\n",
		 stage, readl(sd->base + ISP_IPC_TRIGGER), readl(sd->base + MSCA_REG_060),
		 readl(sd->base + MSCA_FCROP_SIZE), readl(channel + MSCA_CH_SCALE_SIZE(0)),
		 readl(channel + MSCA_CH_CROP_SIZE(0)), readl(sd->base + MSCA_SHD_TRIGGER),
		 readl(sd->base + MSCA_SHD_DMA), readl(sd->base + MSCA_SHD_STAT),
		 readl(sd->base + MSCA_CH_EN_MASK));
	pr_debug("t23 ISP blocks %s: gib=%08x dmsc=%08x csc=%08x/%08x awb=%08x key=%08x\n",
		 stage, readl(sd->base + ISP_DGAIN_GAIN01), readl(sd->base + ISP_DMSC_REG_04),
		 readl(sd->base + ISP_CSC_CONTROL), readl(sd->base + ISP_CSC_COEFF0),
		 readl(sd->base + ISP_AWB_LUM_TH_FREQ), readl(sd->base + ISP_TOP_REG_33C));
	pr_debug("t23 MSCA config %s: crop=%08x/%08x step=%08x/%08x out=%08x stride=%08x/%08x reg50=%08x\n",
		 stage, readl(channel + MSCA_CH_FCROP_POS(0)), readl(channel + MSCA_CH_CROP_POS(0)),
		 readl(channel + MSCA_CH_VSTEP(0)), readl(channel + MSCA_CH_HSTEP(0)),
		 readl(channel + MSCA_CH_OUT(0)), readl(channel + MSCA_Y_STRIDE(0)),
		 readl(channel + MSCA_UV_STRIDE(0)), readl(sd->base + MSCA_REG_050));
}

/* Filters of the enabled channels, loaded by the shadow DMA. */
static void msca_write_filters(struct tx_isp_subdev *sd,
			       const struct tx_isp_msca_target out[])
{
	struct tx_isp_core_device *core = sd->dev_priv;
	u32 width = core->geometry >> 16, height = core->geometry & 0xffff;
	unsigned int ch, entries = 0;

	writel(MSCA_SHD_CTRL_DEFAULT_BEGIN, sd->base + MSCA_SHD_CTRL);
	for (ch = 0; ch < T23_MSCA_OUTPUTS; ch++) {
		if (!out[ch].width)
			continue;
		msca_write_filter(sd->base, MSCA_SHD_VCOEF(ch),
				  (out[ch].height << 14) / height);
		msca_write_filter(sd->base, MSCA_SHD_HCOEF(ch),
				  (out[ch].width << 14) / width);
		entries += 2 * MSCA_COEFF_WORDS;
	}
	writel(MSCA_SHD_CTRL_DEFAULT_END, sd->base + MSCA_SHD_CTRL);
	writel(((entries - 1) << 4) | MSCA_SHD_DMA_START, sd->base + MSCA_SHD_DMA);
}

/* OSD pixel formats: type in bits 0-1 of MSCA_OSD_CTRL, and colour order */
static const struct {
	u32 fourcc;
	u32 ctrl;	/* bit 8 ignores the pixel alpha */
	u32 order;
} msca_osd_formats[] = {
	{ V4L2_PIX_FMT_ABGR32, 0, 0 },		/* bytes B, G, R, A */
	{ V4L2_PIX_FMT_XBGR32, BIT(8), 0 },
	{ V4L2_PIX_FMT_ARGB32, 0, 13 },		/* bytes A, R, G, B */
	{ V4L2_PIX_FMT_XRGB32, BIT(8), 13 },
	{ V4L2_PIX_FMT_ARGB555, 1, 0 },
	{ V4L2_PIX_FMT_XRGB555, 1 | BIT(8), 0 },
};

static int msca_osd_format(u32 fourcc)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(msca_osd_formats); i++)
		if (msca_osd_formats[i].fourcc == fourcc)
			return i;
	return -EINVAL;
}

static int msca_check_overlay(const struct tx_isp_msca_overlay *ov,
			      u32 width, u32 height)
{
	unsigned int n;

	for (n = 0; n < T23_MSCA_OSD_REGIONS; n++) {
		const struct tx_isp_msca_osd *osd = &ov->osd[n];
		const struct tx_isp_rect *r = &osd->rect;

		if (!osd->addr)
			continue;
		if (msca_osd_format(ov->osd_format) < 0 || osd->addr & 7 ||
		    !r->width || !r->height || r->left + r->width > width ||
		    r->top + r->height > height)
			return -EINVAL;
	}
	return 0;
}

/* Scale input span [@start, @end) to an output of @out pixels from @in. */
static void msca_scale_span(u32 start, u32 end, u32 in, u32 out,
			    u32 *first, u32 *last)
{
	*first = min(start, in) * out / in;
	*last = DIV_ROUND_UP(min(end, in) * out, in);
}

static void msca_write_line(void __iomem *base, unsigned int n,
			    const struct tx_isp_line *ln, u32 color,
			    u32 width, u32 height)
{
	u32 x0, y0, x1, y1, lw, dx, ady, octant, slope;
	s32 dy;

	if (!ln->width || !width || !height) {
		writel(0, base + MSCA_LINE_CTRL(n));
		return;
	}
	x0 = min_t(u32, ln->x0, width - 1);
	y0 = min_t(u32, ln->y0, height - 1);
	x1 = min_t(u32, ln->x1, width - 1);
	y1 = min_t(u32, ln->y1, height - 1);
	if (x0 == x1 && y0 == y1) {
		writel(0, base + MSCA_LINE_CTRL(n));
		return;
	}
	if (x1 < x0 || (x0 == x1 && y1 < y0)) {
		swap(x0, x1);
		swap(y0, y1);
	}
	lw = min_t(u32, ln->width, 255);
	dx = x1 - x0;
	dy = (s32)y1 - (s32)y0;
	ady = abs(dy);

	writel(x0 << 16 | y0, base + MSCA_LINE_START(n));
	writel(FIELD_PREP(MSCA_LINE_COLOR_WIDTH, lw) | color,
	       base + MSCA_LINE_COLOR(n));
	if (!dx || !ady) {
		writel((dx ?: lw) << 16 | (ady ?: lw),
		       base + MSCA_LINE_END(n));
		writel(1, base + MSCA_LINE_CTRL(n));
		return;
	}
	octant = (dy < 0 ? 1 : 0) | (dx < ady ? 2 : 0);
	slope = dx >= ady ? (ady << 8) / dx : (dx << 8) / ady;
	writel(dx << 16 | ady, base + MSCA_LINE_END(n));
	writel(FIELD_PREP(MSCA_LINE_CTRL_OCTANT, octant) |
	       FIELD_PREP(MSCA_LINE_CTRL_SLOPE, slope) |
	       MSCA_LINE_CTRL_DDA_EN | 1,
	       base + MSCA_LINE_CTRL(n));
}

static void msca_write_overlay(struct tx_isp_subdev *sd,
			       const struct tx_isp_msca_target out[],
			       const struct tx_isp_msca_overlay *ov)
{
	struct tx_isp_core_device *core = sd->dev_priv;
	u32 width = core->geometry >> 16, height = core->geometry & 0xffff;
	const struct tx_isp_rect *r;
	unsigned int ch, n;
	u32 masks = 0, blocks = 0;
	int format;

	for (ch = 0; ch < T23_MSCA_OUTPUTS; ch++) {
		if (!out[ch].width)
			continue;
		for (n = 0; n < T23_MSCA_MASKS; n++) {
			void __iomem *reg = sd->base + MSCA_MASK(ch, n);
			u32 x0, x1, y0, y1;

			r = &ov->mask[n];
			msca_scale_span(r->left, r->left + r->width, width,
					out[ch].width, &x0, &x1);
			msca_scale_span(r->top, r->top + r->height, height,
					out[ch].height, &y0, &y1);
			if (x1 <= x0 || y1 <= y0) {
				writel(0, reg + 4);
				continue;
			}
			writel(x0 << 16 | y0, reg);
			writel((x1 - x0) << 16 | (y1 - y0), reg + 4);
			writel(ov->mask_yuv, reg + 8);
			masks |= MSCA_MASK_EN_CH(ch);
		}
	}
	writel(masks, sd->base + MSCA_MASK_EN);

	for (n = 0; n < T23_MSCA_LINES; n++)
		msca_write_line(sd->base, n, &ov->line[n], ov->line_yuv,
				out[0].width, out[0].height);

	for (n = 0; n < T23_MSCA_OSD_REGIONS; n++) {
		const struct tx_isp_msca_osd *osd = &ov->osd[n];

		if (!osd->addr)
			continue;
		r = &osd->rect;
		writel(r->left << 16 | r->top, sd->base + MSCA_OSD_POS(n));
		writel(r->width << 16 | r->height, sd->base + MSCA_OSD_SIZE(n));
		writel(osd->addr, sd->base + MSCA_OSD_ADDR(n));
		writel(osd->stride, sd->base + MSCA_OSD_STRIDE(n));
		blocks |= MSCA_OSD_EN_BLOCK(n);
	}
	if (blocks) {
		format = msca_osd_format(ov->osd_format);
		writel(msca_osd_formats[format].ctrl | MSCA_OSD_CTRL_DEFAULT,
		       sd->base + MSCA_OSD_CTRL);
		writel(msca_osd_formats[format].order, sd->base + MSCA_OSD_ORDER);
	}
	writel(blocks, sd->base + MSCA_OSD_EN);
}

int tx_isp_core_dma_arm(struct tx_isp_subdev *sd,
			const struct tx_isp_msca_target out[T23_MSCA_OUTPUTS],
			const struct tx_isp_msca_overlay *overlay,
			unsigned int input)
{
	struct tx_isp_core_device *core = sd->dev_priv;
	u32 width = core->geometry >> 16, height = core->geometry & 0xffff;
	u32 reg50 = MSCA_REG_050_DEFAULT;
	void __iomem *channel;
	unsigned int ch;
	u32 mask = 0;
	int ret;

	for (ch = 0; ch < T23_MSCA_OUTPUTS; ch++) {
		if (!out[ch].width)
			continue;
		/* The FIFOs drop the low three address bits. */
		if ((out[ch].y | out[ch].uv) & 7 || out[ch].width > width ||
		    out[ch].height > height || !out[ch].height)
			return -EINVAL;
		mask |= BIT(ch);
	}
	if (!mask)
		return -EINVAL;
	ret = msca_check_overlay(overlay, width, height);
	if (ret)
		return ret;

	ret = t23_isp_stats_configure(&core->stats, sd->base, input);
	if (ret)
		return ret;
	WRITE_ONCE(core->stats_irq, 0);
	ret = tx_isp_core_mdns_start(sd, input);
	if (ret)
		return ret;
	t23_msca_snapshot(sd, "before-start");
	if ((readl(sd->base + MSCA_SHD_DMA) & MSCA_SHD_DMA_BUSY) ||
	    (readl(sd->base + MSCA_SHD_STAT) >> 1))
		return -EBUSY;
	msca_write_filters(sd, out);
	/*
	 * Once a frame has committed the init shadow, width goes in the
	 * high half. Strides follow that trigger.
	 */
	writel(0, sd->base + MSCA_FCROP_POS);
	writel(core->geometry, sd->base + MSCA_FCROP_SIZE);
	for (ch = 0; ch < T23_MSCA_OUTPUTS; ch++) {
		u32 size = out[ch].width << 16 | out[ch].height;

		if (!out[ch].width)
			continue;
		channel = sd->base + MSCA_CH_BASE(ch);
		writel(0, channel);
		writel(size, channel + MSCA_CH_OFF_SCALE_SIZE);
		writel(0, channel + MSCA_CH_OFF_CROP_POS);
		writel(size, channel + MSCA_CH_OFF_CROP_SIZE);
		writel(MSCA_SCALE_PARA0_DEFAULT, channel + MSCA_CH_OFF_SCALE_PARA0);
		writel(MSCA_SCALE_PARA1_DEFAULT, channel + MSCA_CH_OFF_SCALE_PARA1);
		writel((height << 14) / out[ch].height,
		       channel + MSCA_CH_OFF_VSTEP);
		writel((width << 14) / out[ch].width,
		       channel + MSCA_CH_OFF_HSTEP);
		if (out[ch].hflip)
			reg50 |= MSCA_REG_050_MIRROR(ch);
		if (out[ch].vflip)
			reg50 |= MSCA_REG_050_FLIP(ch);
	}
	msca_write_overlay(sd, out, overlay);
	writel(reg50, sd->base + MSCA_REG_050);
	writel(0, sd->base + MSCA_SCALE_CTL_MODE);
	writel(1, sd->base + MSCA_SHD_TRIGGER);
	core->done_mask = 0;
	for (ch = 0; ch < T23_MSCA_OUTPUTS; ch++) {
		core->armed[ch] = out[ch];
		channel = sd->base + MSCA_CH_BASE(ch);
		if (!out[ch].width) {
			writel(0, channel + MSCA_CH_OFF_Y_STRIDE);
			writel(0, channel + MSCA_CH_OFF_UV_STRIDE);
			continue;
		}
		writel(out[ch].width, channel + MSCA_CH_OFF_Y_STRIDE);
		writel(out[ch].width, channel + MSCA_CH_OFF_UV_STRIDE);
		writel(0, sd->base + MSCA_Y_RESET(ch));
		writel(0, sd->base + MSCA_UV_RESET(ch));
		writel(out[ch].y, sd->base + MSCA_Y_FIFO(ch));
		writel(out[ch].uv, sd->base + MSCA_UV_FIFO(ch));
	}
	core->armed_mask = mask;
	/* A channel left enabled would keep writing to its last buffer. */
	writel((readl(sd->base + MSCA_CH_EN_MASK) &
		~GENMASK(T23_MSCA_OUTPUTS - 1, 0)) | mask,
	       sd->base + MSCA_CH_EN_MASK);
	writel(1, sd->base + MSCA_SHD_TRIGGER);
	dev_dbg_once(&g_tx_isp_dev->pdev->dev,
		     "t23 MSCA arm: run=%08x mode=%08x geom=%08x/%08x/%08x en=%08x stride=%08x\n",
		     readl(sd->base + ISP_IPC_TRIGGER), readl(sd->base + MSCA_REG_060),
		     readl(sd->base + MSCA_FCROP_SIZE), readl(sd->base + MSCA_CH_SCALE_SIZE(0)),
		     readl(sd->base + MSCA_CH_CROP_SIZE(0)), readl(sd->base + MSCA_CH_EN_MASK),
		     readl(sd->base + MSCA_Y_STRIDE(0)));
	t23_msca_snapshot(sd, "after-queue");
	return 0;
}

void tx_isp_core_dma_stop(struct tx_isp_subdev *sd, bool abort)
{
	unsigned int ch, n;

	if (abort) {
		writel(ISP_IPC_TRIGGER_STOP, sd->base + ISP_IPC_TRIGGER);
		writel(0, sd->base + ISP_INT_MASK);
		writel(0, sd->base + ISP_INT1_MASK);
	}
	for (ch = 0; ch < T23_MSCA_OUTPUTS; ch++) {
		writel(1, sd->base + MSCA_Y_RESET(ch));
		writel(1, sd->base + MSCA_UV_RESET(ch));
	}
	for (n = 0; n < MSCA_LINES; n++)
		writel(0, sd->base + MSCA_LINE_CTRL(n));
	writel(0, sd->base + MSCA_CH_EN_MASK);
	writel(0, sd->base + MSCA_MASK_EN);
	writel(0, sd->base + MSCA_OSD_EN);
	writel(1, sd->base + MSCA_SHD_TRIGGER);
}

void tx_isp_core_park_channel(struct tx_isp_subdev *sd, unsigned int ch,
			      dma_addr_t scratch_dma)
{
	struct tx_isp_core_device *core = sd ? sd->dev_priv : NULL;
	void __iomem *channel;

	if (!core || !sd->base || ch >= T23_MSCA_OUTPUTS)
		return;
	channel = sd->base + MSCA_CH_BASE(ch);
	writel(0, channel + MSCA_CH_OFF_Y_STRIDE);
	writel(0, channel + MSCA_CH_OFF_UV_STRIDE);
	if (scratch_dma && !(scratch_dma & 7)) {
		writel(scratch_dma, sd->base + MSCA_Y_FIFO(ch));
		writel(scratch_dma + 2048, sd->base + MSCA_UV_FIFO(ch));
	}
	writel(1, sd->base + MSCA_Y_RESET(ch));
	writel(1, sd->base + MSCA_UV_RESET(ch));
	writel(readl(sd->base + MSCA_CH_EN_MASK) & ~MSCA_CH_EN(ch),
	       sd->base + MSCA_CH_EN_MASK);
	writel(1, sd->base + MSCA_SHD_TRIGGER);
	memset(&core->armed[ch], 0, sizeof(core->armed[ch]));
	core->armed_mask &= ~BIT(ch);
	core->done_mask &= ~BIT(ch);
}
