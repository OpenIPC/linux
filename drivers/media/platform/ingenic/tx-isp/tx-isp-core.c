// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 image signal processor
 *
 * Copyright (C) 2017-2023 Ingenic Semiconductor Co., Ltd.
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
#include <linux/media-bus-format.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>

#include "tx-isp-device.h"
#include "tx-isp-mdns.h"
#include "tx-isp-profile.h"

static int mdns = -1;
module_param(mdns, int, 0644);
MODULE_PARM_DESC(mdns, "Temporal noise reduction: -1=auto (<=720p), 0=bypass, 1=enable");

static struct tx_isp_core_device *g_core_dev;
static u32 core_irq_seen;

static int t23_mbus_to_bayer_order(u32 code, u32 *order)
{
	switch (code) {
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		*order = ISP_TOP_BAYER_RGGB;
		return 0;
	case MEDIA_BUS_FMT_SBGGR10_1X10:
		*order = ISP_TOP_BAYER_BGGR;
		return 0;
	case MEDIA_BUS_FMT_SGBRG10_1X10:
		*order = ISP_TOP_BAYER_GBRG;
		return 0;
	case MEDIA_BUS_FMT_SGRBG10_1X10:
		*order = ISP_TOP_BAYER_GRBG;
		return 0;
	default:
		return -EINVAL;
	}
}

static void core_mdns_release(struct tx_isp_core_device *core)
{
	unsigned int i;

	/* The caller has completed the bus-stop/reset handshake. */
	for (i = 0; i < ARRAY_SIZE(core->history); i++) {
		tx_isp_mdns_frame_done(&core->history[i], true);
		tx_isp_mdns_free(&core->history[i]);
	}
}

int tx_isp_core_mdns_start(struct tx_isp_subdev *sd, unsigned int input)
{
	struct tx_isp_core_device *core = sd->dev_priv;
	unsigned int i;
	int ret;

	if (!core->mdns_enabled)
		return 0;
	if (input >= ARRAY_SIZE(core->history))
		return -EINVAL;
	if (!core->history[input].cpu)
		return 0;
	for (i = 0; i < ARRAY_SIZE(core->history); i++)
		if (core->history[i].active)
			return -EBUSY;
	ret = tx_isp_mdns_start(&core->history[input], sd->base,
				TX_ISP_MDNS_SC1A4T_LINEAR_DAY, 0,
				core->params[input].valid & TX_ISP_PARAM_MDNS ?
				&core->params[input].cfg.mdns : NULL);
	if (!ret)
		core->mdns_input = input;
	return ret;
}

void tx_isp_core_queue_ext_params(struct tx_isp_subdev *sd, unsigned int input,
				  const struct t23_isp_ext_params_cfg *ext,
				  size_t payload)
{
	struct tx_isp_core_device *core = sd->dev_priv;

	if (input < ARRAY_SIZE(core->history))
		tx_isp_ext_params_merge(&core->params[input], ext, payload);
}

void tx_isp_core_reset_params(struct tx_isp_subdev *sd, unsigned int input)
{
	struct tx_isp_core_device *core = sd->dev_priv;

	if (input < ARRAY_SIZE(core->history)) {
		core->params[input].valid = 0;
		core->params[input].dirty = 0;
		core->params[input].blc_valid = false;
		core->params[input].blc_dirty = false;
	}
}

/* The two inputs share the core, so switching input flushes all active blocks. */
void tx_isp_core_apply_params(struct tx_isp_subdev *sd, unsigned int input)
{
	struct tx_isp_core_device *core = sd->dev_priv;
	u32 valid, dropped;

	if (input >= ARRAY_SIZE(core->history))
		return;

	valid = core->params[input].valid;
	dropped = core->hw_custom_mask & ~valid;
	if (dropped) {
		if (dropped & TX_ISP_PARAM_DMSC) {
			tx_isp_core_restore_dmsc(sd);
			writel(ISP_DMSC_REFRESH_COMMIT,
			       sd->base + ISP_DMSC_TRIGGER);
		}
		tx_isp_profile_restore_blocks(sd->base,
					      sd->res ? resource_size(sd->res) : 0,
					      dropped);
		core->hw_custom_mask &= ~dropped;
	}
	if (core->last_params_input != (int)input) {
		core->params[input].dirty = valid;
		core->params[input].blc_dirty = core->params[input].blc_valid;
	}
	tx_isp_params_write(sd, &core->params[input]);
	core->hw_custom_mask |= valid;
	core->last_params_input = input;
}

void tx_isp_core_mdns_frame_done(struct tx_isp_subdev *sd)
{
	struct tx_isp_core_device *core = sd->dev_priv;

	if (core->mdns_enabled)
		tx_isp_mdns_frame_done(&core->history[core->mdns_input], false);
}

static int core_stream_on(struct tx_isp_subdev *sd)
{
	struct tx_isp_core_device *core = sd ? sd->dev_priv : NULL;
	u32 width, height, inputs;
	unsigned int i;
	bool want_mdns;
	int ret;

	if (!core)
		return -EINVAL;
	if (core->running)
		return 0;
	core_irq_seen = 0;
	width = core->geometry >> 16;
	height = core->geometry & 0xffff;
	inputs = core->active_inputs ? core->active_inputs : BIT(0);
	if (g_tx_isp_dev->processed &&
	    (!sd->res || resource_size(sd->res) < 0x80180))
		return -EINVAL;
	if (mdns == 0)
		want_mdns = false;
	else if (mdns > 0)
		want_mdns = true;
	else
		want_mdns = (width * height <= 1280U * 720U);
	core->mdns_enabled = false;
	if (g_tx_isp_dev->processed && want_mdns) {
		core->mdns_enabled = true;
		for (i = 0; i < ARRAY_SIZE(core->history); i++) {
			if (!(inputs & BIT(i)))
				continue;
			ret = tx_isp_mdns_alloc(&g_tx_isp_dev->pdev->dev,
					&core->history[i], width, height);
			if (ret == -ENOMEM) {
				dev_warn(&g_tx_isp_dev->pdev->dev,
					 "CMA tight; bypassing MDNS history\n");
				core_mdns_release(core);
				core->mdns_enabled = false;
				break;
			}
			if (ret) {
				core_mdns_release(core);
				core->mdns_enabled = false;
				return ret;
			}
		}
	}
	core->input_control = readl(sd->base + ISP_IPC_MODE);
	/* Temporal filtering requires history buffers; the VIC path has none. */
	writel(ISP_MDNS_FUNC_CTRL_BYPASS, sd->base + ISP_MDNS_FUNC_CONTROL);
	writel(ISP_MDNS_FUNC_GROUP_DEFAULT, sd->base + ISP_MDNS_FUNC_GROUP);
	writel(0, sd->base + ISP_MDNS_DEBUG_CONTROL);
	writel(ISP_MDNS_REFRESH_VAL, sd->base + ISP_MDNS_FUNC_REFRESH);
	writel(ISP_MDNS_TRIGGER_START, sd->base + ISP_MDNS_REG_TRIGGER);
	writel(core->geometry, sd->base + ISP_TOP_IMAGE_SIZE);
	writel(core->bayer_order, sd->base + ISP_TOP_BAYER_ORDER);
	writel((readl(sd->base + ISP_TOP_CTRL) & ~BIT(23)) | ISP_TOP_CTRL_ENABLE,
	       sd->base + ISP_TOP_CTRL);
	if (g_tx_isp_dev->processed) {
		writel(ISP_TOP_REG_1C_DEFAULT, sd->base + ISP_TOP_REG_1C);
		writel(ISP_TOP_REG_2C_DEFAULT, sd->base + ISP_TOP_REG_2C);
		writel(ISP_TOP_REG_10_DEFAULT, sd->base + ISP_TOP_REG_10);
		writel(ISP_TOP_REG_30_DEFAULT, sd->base + ISP_TOP_REG_30);
		writel(ISP_TOP_CTRL_DEFAULT, sd->base + ISP_TOP_CTRL);
		writel(ISP_TOP_REG_33C_DEFAULT, sd->base + ISP_TOP_REG_33C);
		/* Post output shadow before ISP trigger runs. */
		ret = tx_isp_core_output_init(sd);
		if (ret)
			return ret;
		writel(ISP_IPC_MODE_DEFAULT, sd->base + ISP_IPC_MODE);
		writel(ISP_IPC_TRIGGER_START, sd->base + ISP_IPC_TRIGGER);
		ret = tx_isp_profile_init(sd->base, resource_size(sd->res),
					 core->geometry >> 16,
					 core->geometry & 0xffff);
		if (ret)
			return ret;
		core->hw_custom_mask = 0;
		core->last_params_input = -1;
		ret = tx_isp_mdns_configure_bypassed(sd->base,
				core->geometry >> 16, core->geometry & 0xffff,
				TX_ISP_MDNS_SC1A4T_LINEAR_DAY, 0);
		if (ret)
			return ret;
		ret = t23_isp_stats_configure(&core->stats, sd->base, 0);
		if (ret)
			return ret;
		writel(U32_MAX, sd->base + ISP_INT_CLEAR);
		writel(ISP_INT_MASK_ALL, sd->base + ISP_INT_MASK);
		writel(ISP_INT1_BIT15, sd->base + ISP_INT1_MASK);
	}
	if (!g_tx_isp_dev->processed) {
		writel(ISP_IPC_MODE_DEFAULT, sd->base + ISP_IPC_MODE);
		writel(ISP_IPC_TRIGGER_START, sd->base + ISP_IPC_TRIGGER);
		writel(ISP_IPC_MODE_DEFAULT | ISP_IPC_MODE_PORT1,
		       sd->base + ISP_IPC_MODE);
	}
	core->running = true;
	return 0;
}

static int core_stream_off(struct tx_isp_subdev *sd)
{
	struct tx_isp_core_device *core = sd ? sd->dev_priv : NULL;
	bool was_running;
	int ret;

	if (!core)
		return -EINVAL;
	was_running = core->running;
	if (core->running) {
		writel(ISP_IPC_TRIGGER_STOP, sd->base + ISP_IPC_TRIGGER);
		writel(0, sd->base + ISP_INT_MASK);
		writel(0, sd->base + ISP_INT1_MASK);
		writel(1, sd->base + MSCA_Y_RESET(0));
		writel(1, sd->base + MSCA_UV_RESET(0));
		writel(0, sd->base + MSCA_CH_EN_MASK);
		writel(1, sd->base + MSCA_SHD_TRIGGER);
		writel(core->input_control, sd->base + ISP_IPC_MODE);
	}
	core->running = false;
	if (was_running || core->stats.active || core->history[0].cpu ||
	    core->history[1].cpu ||
	    g_tx_isp_dev->dma_quiescence_failed) {
		ret = reset_control_reset(g_tx_isp_dev->reset);
		if (ret) {
			g_tx_isp_dev->dma_quiescence_failed = true;
			dev_err(&g_tx_isp_dev->pdev->dev,
				"ISP bus stop failed: %d\n", ret);
			return ret;
		}
		core->stats.active = false;
		core_mdns_release(core);
		g_tx_isp_dev->dma_quiescence_failed = false;
	}
	return 0;
}

static int core_s_stream(struct tx_isp_subdev *sd, int enable)
{
	if (!sd)
		return -EINVAL;
	return enable ? core_stream_on(sd) : core_stream_off(sd);
}

int tx_isp_core_s_stream(struct tx_isp_subdev *sd, int enable)
{
	return core_s_stream(sd, enable);
}

static const u16 gamma_t70[129] = {
	0, 175, 275, 358, 431, 499, 561, 620,
	677, 730, 782, 832, 880, 927, 973, 1018,
	1061, 1104, 1146, 1187, 1227, 1266, 1305, 1343,
	1381, 1418, 1455, 1491, 1526, 1562, 1596, 1631,
	1665, 1698, 1731, 1764, 1797, 1829, 1861, 1893,
	1924, 1955, 1986, 2017, 2047, 2077, 2107, 2137,
	2166, 2195, 2224, 2253, 2282, 2310, 2338, 2366,
	2394, 2422, 2449, 2477, 2504, 2531, 2558, 2584,
	2611, 2637, 2664, 2690, 2716, 2742, 2767, 2793,
	2818, 2844, 2869, 2894, 2919, 2944, 2969, 2993,
	3018, 3042, 3067, 3091, 3115, 3139, 3163, 3187,
	3211, 3234, 3258, 3281, 3305, 3328, 3351, 3374,
	3397, 3420, 3443, 3466, 3488, 3511, 3534, 3556,
	3578, 3601, 3623, 3645, 3667, 3689, 3711, 3733,
	3755, 3777, 3798, 3820, 3841, 3863, 3884, 3906,
	3927, 3948, 3969, 3990, 4011, 4032, 4053, 4074,
	4095,
};

static const u16 gamma_t100[129] = {
	0, 451, 618, 744, 847, 938, 1019, 1093,
	1161, 1225, 1285, 1342, 1396, 1448, 1498, 1545,
	1591, 1636, 1679, 1721, 1761, 1801, 1839, 1877,
	1913, 1949, 1984, 2019, 2052, 2085, 2118, 2149,
	2181, 2211, 2242, 2271, 2301, 2329, 2358, 2386,
	2413, 2441, 2468, 2494, 2520, 2546, 2572, 2597,
	2622, 2647, 2671, 2695, 2719, 2743, 2766, 2789,
	2812, 2835, 2858, 2880, 2902, 2924, 2945, 2967,
	2988, 3009, 3030, 3051, 3072, 3092, 3113, 3133,
	3153, 3172, 3192, 3212, 3231, 3250, 3269, 3288,
	3307, 3326, 3345, 3363, 3381, 3400, 3418, 3436,
	3454, 3472, 3489, 3507, 3524, 3542, 3559, 3576,
	3593, 3610, 3627, 3644, 3660, 3677, 3693, 3710,
	3726, 3742, 3759, 3775, 3791, 3807, 3822, 3838,
	3854, 3869, 3885, 3900, 3916, 3931, 3946, 3962,
	3977, 3992, 4007, 4021, 4036, 4051, 4066, 4080,
	4095,
};

static const u16 gamma_t180[129] = {
	0, 1203, 1433, 1587, 1707, 1806, 1891, 1966,
	2033, 2095, 2151, 2203, 2252, 2298, 2342, 2383,
	2422, 2460, 2495, 2530, 2563, 2594, 2625, 2655,
	2683, 2711, 2738, 2764, 2790, 2815, 2839, 2862,
	2885, 2908, 2930, 2952, 2973, 2993, 3013, 3033,
	3053, 3072, 3091, 3109, 3127, 3145, 3162, 3180,
	3197, 3213, 3230, 3246, 3262, 3278, 3293, 3308,
	3323, 3338, 3353, 3368, 3382, 3396, 3410, 3424,
	3437, 3451, 3464, 3477, 3490, 3503, 3516, 3529,
	3541, 3554, 3566, 3578, 3590, 3602, 3614, 3625,
	3637, 3648, 3659, 3671, 3682, 3693, 3704, 3715,
	3725, 3736, 3747, 3757, 3767, 3778, 3788, 3798,
	3808, 3818, 3828, 3838, 3848, 3857, 3867, 3876,
	3886, 3895, 3905, 3914, 3923, 3932, 3941, 3950,
	3959, 3968, 3977, 3986, 3994, 4003, 4012, 4020,
	4029, 4037, 4046, 4054, 4062, 4071, 4079, 4087,
	4095,
};

const s16 tx_isp_default_ccm[9] = {
	2104, -748, -329,
	-238, 1583, -320,
	76, -1041, 1988
};

void tx_isp_core_set_blc_default(struct tx_isp_subdev *sd)
{
	if (!sd || !sd->base)
		return;

	writel(ISP_DGAIN_TRIGGER_COMMIT, sd->base + ISP_DGAIN_TRIGGER);
	writel(ISP_DGAIN_BLC_ONE, sd->base + ISP_DGAIN_BLC0);
	writel(ISP_DGAIN_TRIGGER_COMMIT, sd->base + ISP_DGAIN_TRIGGER);
	writel(ISP_DGAIN_BLC_PAIR_ONE, sd->base + ISP_DGAIN_BLC1);
	writel(ISP_DGAIN_TRIGGER_COMMIT, sd->base + ISP_DGAIN_TRIGGER);
	writel(ISP_DGAIN_BLC_ONE, sd->base + ISP_DGAIN_BLC2);
}

void tx_isp_core_set_dgain(struct tx_isp_subdev *sd, u32 dgain_q10)
{
	u32 g = clamp_val(dgain_q10, 256U, 16383U);
	u32 pair = g | (g << 16);

	if (!sd || !sd->base)
		return;

	writel(ISP_DGAIN_TRIGGER_COMMIT, sd->base + ISP_DGAIN_TRIGGER);
	writel(pair, sd->base + ISP_DGAIN_GAIN01);
	writel(ISP_DGAIN_TRIGGER_COMMIT, sd->base + ISP_DGAIN_TRIGGER);
	writel(pair, sd->base + ISP_DGAIN_GAIN23);
}

void tx_isp_core_set_wb(struct tx_isp_subdev *sd, u32 r_gain_q8, u32 b_gain_q8)
{
	u32 r_gain_q10 = ISP_WB_GAIN_TAG | min_t(u64, (u64)r_gain_q8 * 4, 0x3fff);
	u32 b_gain_q10 = ISP_WB_GAIN_TAG | min_t(u64, (u64)b_gain_q8 * 4, 0x3fff);

	if (!sd || !sd->base)
		return;

	writel(ISP_WB_TRIGGER_COMMIT, sd->base + ISP_WB_TRIGGER);
	writel(r_gain_q10, sd->base + ISP_WB_GAIN_R);
	writel(ISP_WB_TRIGGER_COMMIT, sd->base + ISP_WB_TRIGGER);
	writel(b_gain_q10, sd->base + ISP_WB_GAIN_B);
	writel(ISP_WB_TRIGGER_COMMIT, sd->base + ISP_WB_TRIGGER);
	writel(r_gain_q10, sd->base + ISP_WB_GAIN_GR);
	writel(ISP_WB_TRIGGER_COMMIT, sd->base + ISP_WB_TRIGGER);
	writel(b_gain_q10, sd->base + ISP_WB_GAIN_GB);
}

u16 tx_isp_calc_gamma_entry(int j, u32 g)
{
	if (j <= 0)
		return 0;
	if (j >= 128)
		return 4095;
	if (g <= 100) {
		if (g <= 70)
			return gamma_t70[j];
		return gamma_t70[j] + ((gamma_t100[j] - gamma_t70[j]) * (g - 70)) / 30;
	} else {
		if (g >= 180)
			return gamma_t180[j];
		return gamma_t100[j] + ((gamma_t180[j] - gamma_t100[j]) * (g - 100)) / 80;
	}
}

int tx_isp_core_set_gamma(struct tx_isp_subdev *sd, u32 gamma_val)
{
	u32 value;
	unsigned int j;

	if (!sd || !sd->base)
		return -EINVAL;
	if (!sd->res || resource_size(sd->res) < 0x50200)
		return -EOPNOTSUPP;

	for (j = 0; j < 128; j++) {
		value = tx_isp_calc_gamma_entry(j, gamma_val) |
			(u32)tx_isp_calc_gamma_entry(j + 1, gamma_val) << 12;
		writel(value, sd->base + ISP_GAMMA_LUT_R + j * 4);
		writel(value, sd->base + ISP_GAMMA_LUT_G + j * 4);
		writel(value, sd->base + ISP_GAMMA_LUT_B + j * 4);
	}
	return 0;
}

void tx_isp_core_set_ccm(struct tx_isp_subdev *sd, const s16 ccm[9])
{
	u32 value;
	unsigned int i;

	if (!sd || !sd->base)
		return;
	if (!ccm)
		ccm = tx_isp_default_ccm;

	for (i = 0; i < 9; i += 2) {
		value = ccm[i] & 0x3fff;
		if (i + 1 < 9)
			value |= (ccm[i + 1] & 0x3fff) << 16;
		writel(ISP_CCM_TRIGGER_STROBE, sd->base + ISP_CCM_TRIGGER);
		writel(value, sd->base + ISP_CCM_COEFF_BASE + i * 2);
	}
}

static u32 dmsc_sharpness_scale(u32 base, u32 strength, u32 maximum)
{
	base = min(base, maximum);
	if (strength <= 128)
		return base * strength >> 7;
	return base + ((maximum - base) * (strength - 128) >> 7);
}

void tx_isp_core_set_sharpness(struct tx_isp_subdev *sd, u32 sharpness)
{
	u32 defaults[3], value;

	if (!sd || !sd->base)
		return;
	sharpness = min(sharpness, 255U);
	tx_isp_core_dmsc_defaults(defaults);
	value = (defaults[0] & 0xffff0000) |
		dmsc_sharpness_scale(defaults[0] & 0xffff, sharpness, 0xfff);
	writel(value, sd->base + ISP_DMSC_REG_08);
	value = dmsc_sharpness_scale(defaults[1] & 0xffff, sharpness, 0x3ff) |
		dmsc_sharpness_scale(defaults[1] >> 16, sharpness, 0x3ff) << 16;
	writel(value, sd->base + ISP_DMSC_REG_48);
	value = dmsc_sharpness_scale(defaults[2] & 0xffff, sharpness, 0xfff) |
		dmsc_sharpness_scale(defaults[2] >> 16, sharpness, 0xfff) << 16;
	writel(value, sd->base + ISP_DMSC_REG_60);
}

int tx_isp_core_init_hardware(struct tx_isp_subdev *sd)
{
	if (!sd || !sd->base)
		return -EINVAL;

	tx_isp_core_set_wb(sd, 256, 256);
	tx_isp_core_set_ccm(sd, tx_isp_default_ccm);
	return 0;
}

static irqreturn_t core_interrupt_service_routine(struct tx_isp_subdev *sd,
						  bool *handled)
{
	u32 stat, all_done;
	bool dma_done = false, error = false;

	if (!sd || !sd->base)
		return IRQ_NONE;

	all_done = readl(sd->base + ISP_INT1_STATUS);
	stat = readl(sd->base + ISP_INT_STATUS);
	dev_dbg(&g_tx_isp_dev->pdev->dev,
		"t23 ISP interrupt: %#x all_done=%#x\n", stat, all_done);
	if ((stat & ISP_INT_ERR_MASK) && g_tx_isp_dev->processed) {
		if (!(core_irq_seen & ISP_INT_ERR_MASK)) {
			void __iomem *vic =
				g_tx_isp_dev->subdevs[TX_ISP_SUBDEV_VIC]->base;
			struct device *dev = &g_tx_isp_dev->pdev->dev;

			dev_dbg(dev, "t23 BCSH: %08x %08x %08x %08x %08x %08x\n",
				readl(sd->base + ISP_BCSH_REG_00), readl(sd->base + ISP_BCSH_REG_18),
				readl(sd->base + ISP_BCSH_REG_38), readl(sd->base + ISP_BCSH_REG_54),
				readl(sd->base + ISP_BCSH_REG_64), readl(sd->base + ISP_BCSH_REG_70));
			dev_dbg(dev, "t23 MDNS: %08x %08x %08x %08x\n",
				readl(sd->base + ISP_MDNS_FUNC_CONTROL), readl(sd->base + ISP_MDNS_FUNC_GROUP),
				readl(sd->base + ISP_MDNS_DEBUG_CONTROL), readl(sd->base + ISP_MDNS_FUNC_REFRESH));

			dev_dbg(dev, "t23 ISP first fault: prior=%08x stat=%08x done=%08x id=%08x geom=%08x top=%08x gate=%08x/%08x\n",
				 core_irq_seen, stat, all_done,
				 readl(sd->base + ISP_TOP_REG_00), readl(sd->base + ISP_TOP_IMAGE_SIZE),
				 readl(sd->base + ISP_TOP_CTRL), readl(sd->base + ISP_TOP_REG_10),
				 readl(sd->base + ISP_TOP_REG_30));
			dev_dbg(dev, "t23 ISP first fault: run=%08x mode=%08x input=%08x msca_en=%08x msca_mode=%08x geom=%08x/%08x/%08x\n",
				 readl(sd->base + ISP_IPC_TRIGGER), readl(sd->base + ISP_IPC_MODE),
				 readl(sd->base + ISP_IPC_INPUT_STAT), readl(sd->base + MSCA_CH_EN_MASK),
				 readl(sd->base + MSCA_REG_060), readl(sd->base + MSCA_FCROP_SIZE),
				 readl(sd->base + MSCA_CH_SCALE_SIZE(0)), readl(sd->base + MSCA_CH_CROP_SIZE(0)));
			dev_dbg(dev, "t23 ISP first fault: queue=%08x/%08x fifo=%08x/%08x vic=%08x/%08x/%08x ivdc=%08x\n",
				 readl(sd->base + MSCA_CH_REG_34(0)), readl(sd->base + MSCA_CH_REG_54(0)),
				 readl(sd->base + MSCA_Y_DONE_EMPTY(0)), readl(sd->base + MSCA_UV_DONE_EMPTY(0)),
				 readl(vic + VIC_STATUS_REG_0), readl(vic + VIC_STATUS_REG_1),
				 readl(vic + VIC_PORT_STATUS),
				 readl(g_tx_isp_dev->ivdc_base + IVDC_TOP_INFO1));
			dev_dbg(dev, "t23 ISP first fault: ivdc_status=%08x lines=%08x info=%08x/%08x stride=%08x/%08x reg50=%08x ctl=%08x\n",
				 readl(g_tx_isp_dev->ivdc_base + IVDC_INTC_STATE),
				 readl(g_tx_isp_dev->ivdc_base + IVDC_MAX_DDR_LINE),
				 readl(g_tx_isp_dev->ivdc_base + IVDC_TOP_INFO),
				 readl(g_tx_isp_dev->ivdc_base + IVDC_ISP_INFO),
				 readl(sd->base + MSCA_Y_STRIDE(0)), readl(sd->base + MSCA_UV_STRIDE(0)),
				 readl(sd->base + MSCA_REG_050), readl(sd->base + MSCA_SCALE_CTL_MODE));
		}
		{
			u32 input_reg = readl(sd->base + ISP_IPC_INPUT_STAT);

			dev_err_ratelimited(&g_tx_isp_dev->pdev->dev,
					    "ISP pipeline error: %#x, input %#x\n",
					    stat, input_reg);
		}
		error = true;
	} else if ((stat & (ISP_INT_MSCA_CH0_DONE | ISP_INT_MSCA_CH1_DONE |
			    ISP_INT_MSCA_CH2_DONE)) &&
		   g_tx_isp_dev->processed) {
		struct tx_isp_core_device *core = sd->dev_priv;
		unsigned int ch;
		u32 y, uv;

		/* Status bit n: MSCA channel n finished a frame. */
		for (ch = 0; ch < T23_MSCA_OUTPUTS; ch++) {
			if (!(stat & BIT(ch)) || !(core->armed_mask & BIT(ch)))
				continue;
			if ((readl(sd->base + MSCA_Y_DONE_EMPTY(ch)) & MSCA_DONE_EMPTY) ||
			    (readl(sd->base + MSCA_UV_DONE_EMPTY(ch)) & MSCA_DONE_EMPTY)) {
				error = true;
				break;
			}
			y = readl(sd->base + MSCA_Y_DONE(ch)) & MSCA_DONE_ADDR;
			uv = readl(sd->base + MSCA_UV_DONE(ch)) & MSCA_DONE_ADDR;
			if (y != core->armed[ch].y || uv != core->armed[ch].uv) {
				error = true;
				break;
			}
			core->done_mask |= BIT(ch);
		}
		dma_done = core->armed_mask &&
			   core->done_mask == core->armed_mask;
	}
	core_irq_seen |= stat;
	if (stat & ISP_CORE_STATS_IRQS) {
		struct tx_isp_core_device *core = sd->dev_priv;

		core->stats_irq |= stat & ISP_CORE_STATS_IRQS;
	}
	if (all_done)
		writel(all_done, sd->base + ISP_INT1_CLEAR);
	if (stat)
		writel(stat, sd->base + ISP_INT_CLEAR);

	if (g_tx_isp_dev->processed)
		t23_isp_video_processed_event(g_tx_isp_dev, dma_done,
					      all_done & ISP_INT1_BIT15, error);

	if (!stat && !all_done)
		return IRQ_NONE;


	if (handled)
		*handled = true;
	return IRQ_HANDLED;
}

static struct tx_isp_subdev_ops core_subdev_ops = {
	.interrupt_service_routine = core_interrupt_service_routine,
};

int t23_isp_configure_input(struct tx_isp_device *isp,
			    struct v4l2_subdev *sensor, unsigned int port,
			    u32 active_inputs)
{
	struct v4l2_subdev_format fmt = { .which = V4L2_SUBDEV_FORMAT_ACTIVE };
	struct v4l2_mbus_config bus = { };
	struct v4l2_ctrl *link_freq;
	struct tx_isp_subdev *core_sd;
	struct tx_isp_core_device *core;
	u32 bayer_order;
	s64 frequency;
	unsigned int lanes, pad = 0;
	int ret;

	if (!isp || !sensor)
		return -EINVAL;
	core_sd = isp->subdevs[TX_ISP_SUBDEV_CORE];
	core = core_sd ? core_sd->dev_priv : NULL;
	if (!core)
		return -ENODEV;
	for (pad = 0; pad < sensor->entity.num_pads; pad++)
		if (sensor->entity.pads[pad].flags & MEDIA_PAD_FL_SOURCE)
			break;
	if (pad == sensor->entity.num_pads)
		pad = 0;
	fmt.pad = pad;
	ret = v4l2_subdev_call_state_active(sensor, pad, get_fmt, &fmt);
	if (ret)
		return ret;
	if (t23_mbus_to_bayer_order(fmt.format.code, &bayer_order) ||
	    !fmt.format.width || fmt.format.width > 1920 ||
	    !fmt.format.height || fmt.format.height > 1080 ||
	    ((fmt.format.width | fmt.format.height) & 1))
		return -EINVAL;
	ret = v4l2_subdev_call(sensor, pad, get_mbus_config, pad, &bus);
	if (ret)
		return ret;
	if (bus.type != V4L2_MBUS_CSI2_DPHY)
		return -EINVAL;
	lanes = bus.bus.mipi_csi2.num_data_lanes;
	if (lanes < 1 || lanes > 2)
		return -EINVAL;
	link_freq = v4l2_ctrl_find(sensor->ctrl_handler, V4L2_CID_LINK_FREQ);
	if (!link_freq || link_freq->type != V4L2_CTRL_TYPE_INTEGER_MENU)
		return -ENOLINK;
	ret = v4l2_ctrl_g_ctrl(link_freq);
	if (ret < 0 || ret > link_freq->maximum)
		return -EINVAL;
	frequency = link_freq->qmenu_int[ret];
	if (frequency < 40000000 || frequency >= 750000000)
		return -EINVAL;
	if (isp->streaming)
		return -EBUSY;
	ret = reset_control_reset(isp->reset);
	if (ret) {
		dev_err(&isp->pdev->dev, "ISP reset failed: %d\n", ret);
		return ret;
	}
	core->stats.active = false;
	core_mdns_release(core);
	isp->dma_quiescence_failed = false;
	ret = tx_isp_csi_configure(isp->subdevs[TX_ISP_SUBDEV_CSI], lanes,
				 div_s64(frequency * 2, 1000000));
	if (ret)
		return ret;
	ret = tx_isp_vic_configure(isp->subdevs[TX_ISP_SUBDEV_VIC],
				 fmt.format.width, fmt.format.height, port);
	if (!ret) {
		core->geometry = (fmt.format.width << 16) | fmt.format.height;
		core->bayer_order = bayer_order;
		core->active_inputs = active_inputs ? active_inputs : BIT(0);
	}
	return ret;
}

int tx_isp_core_probe(struct platform_device *pdev)
{
	struct tx_isp_core_device *core;
	int ret;

	core = kzalloc(sizeof(*core), GFP_KERNEL);
	if (!core)
		return -ENOMEM;

	ret = tx_isp_subdev_init(pdev, &core->sd, &core_subdev_ops);
	if (ret) {
		kfree(core);
		return ret;
	}

	core->sd.dev_priv = core;
	core->last_params_input = -1;
	/* MDNS stays in the processed chain and needs history buffers. */
	core->mdns_enabled = true;
	core->params = vzalloc(array_size(ARRAY_SIZE(core->history),
					  sizeof(*core->params)));
	if (!core->params) {
		tx_isp_subdev_deinit(&core->sd);
		kfree(core);
		return -ENOMEM;
	}
	ret = t23_isp_stats_alloc(&core->stats, &g_tx_isp_dev->pdev->dev);
	if (ret) {
		vfree(core->params);
		tx_isp_subdev_deinit(&core->sd);
		kfree(core);
		return ret;
	}
	g_core_dev = core;

	tx_isp_core_init_hardware(&core->sd);

	return 0;
}

int tx_isp_core_remove(struct platform_device *pdev)
{
	struct tx_isp_core_device *core = g_core_dev;

	if (core) {
		core_stream_off(&core->sd);
		if (t23_isp_stats_free(&core->stats))
			dev_err(&pdev->dev,
				"ISP statistics memory retained: DMA is not quiescent\n");
		tx_isp_subdev_deinit(&core->sd);
		vfree(core->params);
		kfree(core);
		g_core_dev = NULL;
	}
	return 0;
}
