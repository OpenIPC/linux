// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang
/*
 * Per-input ISP parameters queued from userspace.
 */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "tx-isp-mdns.h"

#include "tx-isp-device.h"
#include "tx-isp-params.h"

/* SDNS tuning registers; 0x88c4..0x8a5c hold fixed filter kernels. */
static const u16 sdns_regs[T23_ISP_SDNS_WORDS] = {
	0x8800, 0x8804, 0x8808, 0x880c, 0x8810, 0x8814, 0x8818, 0x881c,
	0x8820, 0x8824, 0x8828, 0x882c, 0x8830, 0x8834, 0x8838, 0x883c,
	0x8840, 0x8844, 0x8848, 0x884c, 0x8850, 0x8854, 0x8858, 0x885c,
	0x8860, 0x8864, 0x8868, 0x886c, 0x8870, 0x8874, 0x8878, 0x887c,
	0x8880, 0x8884, 0x8888, 0x888c, 0x8890, 0x8894, 0x8898, 0x889c,
	0x88a0, 0x88a4, 0x88a8, 0x88ac, 0x88b0, 0x88b4, 0x88b8, 0x88bc,
	0x88c0, 0x8a60, 0x8a64, 0x8a68, 0x8a6c, 0x8a70, 0x8a74, 0x8a78,
	0x8a7c, 0x8a80, 0x8a84, 0x8a88, 0x8a8c, 0x8a90, 0x8a94, 0x8a98,
	0x8a9c, 0x8aa0, 0x8aa4, 0x8aa8, 0x8aac, 0x8ab0, 0x8ab4, 0x8ab8,
	0x8abc, 0x8ac0, 0x8ac4, 0x8ac8, 0x8acc, 0x8ad0, 0x8ad4, 0x8ad8,
	0x8adc, 0x8ae0, 0x8ae4, 0x8ae8, 0x8aec, 0x8af0, 0x8af4, 0x8af8,
	0x8afc, 0x8b00, 0x8b04, 0x8b08, 0x8b0c, 0x8b10, 0x8b14, 0x8b18,
	0x8b1c, 0x8b20, 0x8b24, 0x8b28, 0x8b2c, 0x8b30, 0x8b34, 0x8b38,
};

static const u16 dmsc_regs[T23_ISP_DMSC_WORDS] = {
	0x4800, 0x4804, 0x4808, 0x480c, 0x4810, 0x4814, 0x4820, 0x4824,
	0x4828, 0x482c, 0x4830, 0x4834, 0x4838, 0x483c, 0x4840, 0x4844,
	0x4848, 0x484c, 0x4850, 0x4854, 0x4858, 0x485c, 0x4860, 0x4864,
	0x4868, 0x486c, 0x4870, 0x4874, 0x4878, 0x487c, 0x4880, 0x4884,
	0x4888, 0x488c, 0x4890, 0x489c, 0x48a4, 0x48ac, 0x48b0, 0x48b4,
	0x48b8, 0x48c4, 0x48c8, 0x48cc, 0x48d0, 0x48d4, 0x48d8, 0x48dc,
	0x48e0, 0x48e4, 0x48f4, 0x48f8, 0x48fc, 0x4900, 0x4904, 0x4908,
	0x490c, 0x491c, 0x4920, 0x4924, 0x4928, 0x492c, 0x4930, 0x4934,
	0x4944, 0x4948, 0x494c, 0x4950, 0x4954, 0x4958, 0x495c, 0x4960,
	0x4964, 0x4968, 0x496c, 0x4970, 0x4974, 0x4978, 0x497c, 0x4980,
	0x4984, 0x4988, 0x49a8, 0x49ac, 0x49b0, 0x49b4, 0x49b8, 0x49bc,
	0x49c0, 0x49c4, 0x49c8, 0x49cc, 0x49d0, 0x49d4, 0x49d8, 0x49dc,
	0x49e0, 0x49e4, 0x49e8, 0x49ec, 0x49f0, 0x49f4, 0x49f8, 0x49fc,
	0x4a00, 0x4a04, 0x4a08, 0x4a0c, 0x4a10, 0x4a14, 0x4a18, 0x4a1c,
	0x4a20, 0x4a24, 0x4a28, 0x4a2c, 0x4a30, 0x4a34, 0x4a38, 0x4a3c,
	0x4a40, 0x4a44, 0x4a48, 0x4a4c, 0x4a50, 0x4a54, 0x4a58, 0x4a5c,
};

static size_t tx_isp_ext_block_size(u16 type)
{
	switch (type) {
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_BLC:
		return sizeof(struct t23_isp_ext_params_blc_config);
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_WB:
		return sizeof(struct t23_isp_ext_params_wb_config);
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_CCM:
		return sizeof(struct t23_isp_ext_params_ccm_config);
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_GAMMA:
		return sizeof(struct t23_isp_ext_params_gamma_config);
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_LSC:
		return sizeof(struct t23_isp_ext_params_lsc_config);
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_MDNS:
		return sizeof(struct t23_isp_ext_params_mdns_config);
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_SDNS:
		return sizeof(struct t23_isp_ext_params_sdns_config);
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_YDNS:
		return sizeof(struct t23_isp_ext_params_ydns_config);
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_YSP:
		return sizeof(struct t23_isp_ext_params_ysp_config);
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_DMSC:
		return sizeof(struct t23_isp_ext_params_dmsc_config);
	default:
		return 0;
	}
}

static int tx_isp_ext_check_reserved(const struct t23_isp_ext_params_block_header *hdr)
{
	switch (hdr->type) {
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_WB:
		return ((const struct t23_isp_ext_params_wb_config *)hdr)->reserved ?
		       -EINVAL : 0;
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_CCM: {
		const struct t23_isp_ext_params_ccm_config *b = (const void *)hdr;

		return (b->reserved || b->config.reserved) ? -EINVAL : 0;
	}
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_GAMMA: {
		const struct t23_isp_ext_params_gamma_config *b = (const void *)hdr;

		return (b->reserved || b->config.reserved) ? -EINVAL : 0;
	}
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_LSC:
		return ((const struct t23_isp_ext_params_lsc_config *)hdr)->reserved ?
		       -EINVAL : 0;
	case T23_ISP_EXT_PARAMS_BLOCK_TYPE_MDNS:
		return ((const struct t23_isp_ext_params_mdns_config *)hdr)->reserved ?
		       -EINVAL : 0;
	default:
		return 0;
	}
}

int tx_isp_ext_params_check(const struct t23_isp_ext_params_cfg *ext,
			    size_t payload)
{
	size_t hdr_len = offsetof(struct t23_isp_ext_params_cfg, data);
	size_t pos = 0;
	unsigned int i;

	if (payload < hdr_len || ext->version != T23_ISP_EXT_PARAMS_V1 ||
	    ext->data_size > T23_ISP_EXT_PARAMS_MAX_SIZE ||
	    ext->data_size > payload - hdr_len || (ext->data_size & 7))
		return -EINVAL;

	while (pos < ext->data_size) {
		const struct t23_isp_ext_params_block_header *hdr;
		size_t expected;

		if (ext->data_size - pos < sizeof(*hdr))
			return -EINVAL;
		hdr = (const void *)(ext->data + pos);
		expected = tx_isp_ext_block_size(hdr->type);
		if (!expected || hdr->size != expected ||
		    hdr->size > ext->data_size - pos)
			return -EINVAL;
		if ((hdr->flags & ~(T23_ISP_EXT_PARAMS_FL_BLOCK_DISABLE |
				    T23_ISP_EXT_PARAMS_FL_BLOCK_ENABLE)) ||
		    (hdr->flags == (T23_ISP_EXT_PARAMS_FL_BLOCK_DISABLE |
				    T23_ISP_EXT_PARAMS_FL_BLOCK_ENABLE)))
			return -EINVAL;
		if (tx_isp_ext_check_reserved(hdr))
			return -EINVAL;
		if (!(hdr->flags & T23_ISP_EXT_PARAMS_FL_BLOCK_DISABLE)) {
			switch (hdr->type) {
			case T23_ISP_EXT_PARAMS_BLOCK_TYPE_BLC: {
				const struct t23_isp_ext_params_blc_config *b =
					(const void *)hdr;
				if ((b->config.r | b->config.gr |
				     b->config.gb | b->config.b) > 4095)
					return -EINVAL;
				break;
			}
			case T23_ISP_EXT_PARAMS_BLOCK_TYPE_WB: {
				const struct t23_isp_ext_params_wb_config *b =
					(const void *)hdr;
				if (b->config.gain_r > 0x3fff ||
				    b->config.gain_b > 0x3fff)
					return -EINVAL;
				break;
			}
			case T23_ISP_EXT_PARAMS_BLOCK_TYPE_CCM: {
				const struct t23_isp_ext_params_ccm_config *b =
					(const void *)hdr;
				for (i = 0; i < ARRAY_SIZE(b->config.coeff); i++)
					if (b->config.coeff[i] < -8192 ||
					    b->config.coeff[i] > 8191)
						return -EINVAL;
				break;
			}
			case T23_ISP_EXT_PARAMS_BLOCK_TYPE_GAMMA: {
				const struct t23_isp_ext_params_gamma_config *b =
					(const void *)hdr;
				for (i = 0; i < T23_ISP_GAMMA_POINTS; i++)
					if (b->config.curve[i] > 4095)
						return -EINVAL;
				break;
			}
			case T23_ISP_EXT_PARAMS_BLOCK_TYPE_LSC: {
				const struct t23_isp_ext_params_lsc_config *b =
					(const void *)hdr;
				if (b->config.words > T23_ISP_LSC_MAX_WORDS ||
				    b->config.words % 3 ||
				    b->config.scale > 3 ||
				    b->config.mean_en > 1)
					return -EINVAL;
				break;
			}
			case T23_ISP_EXT_PARAMS_BLOCK_TYPE_MDNS: {
				const struct t23_isp_ext_params_mdns_config *b =
					(const void *)hdr;
				if (tx_isp_mdns_check_config(&b->config))
					return -EINVAL;
				break;
			}
			default:
				break;
			}
		}
		pos += hdr->size;
	}
	return 0;
}

static void tx_isp_merge_block(struct tx_isp_params_state *state, u32 bit,
			       void *dst, const void *src, size_t len)
{
	if (!(state->valid & bit) || memcmp(dst, src, len)) {
		memcpy(dst, src, len);
		state->valid |= bit;
		state->dirty |= bit;
	}
}

static void tx_isp_merge_lsc(struct tx_isp_params_state *state,
			     const struct t23_isp_lsc_config *src)
{
	struct t23_isp_lsc_config *dst = &state->cfg.lsc;
	size_t hdr_bytes = offsetof(struct t23_isp_lsc_config, lut);
	u32 words = min_t(u32, src->words, T23_ISP_LSC_MAX_WORDS) / 3 * 3;
	size_t lut_bytes = words * sizeof(dst->lut[0]);

	if (!(state->valid & TX_ISP_PARAM_LSC) ||
	    memcmp(dst, src, hdr_bytes) ||
	    dst->words != words ||
	    memcmp(dst->lut, src->lut, lut_bytes)) {
		memcpy(dst, src, hdr_bytes);
		dst->words = words;
		memcpy(dst->lut, src->lut, lut_bytes);
		state->valid |= TX_ISP_PARAM_LSC;
		state->dirty |= TX_ISP_PARAM_LSC;
	}
}

/* @ext is the validated kernel shadow copy owned by tx-isp-meta. */
void tx_isp_ext_params_merge(struct tx_isp_params_state *state,
			     const struct t23_isp_ext_params_cfg *ext,
			     size_t payload)
{
	struct tx_isp_params_values *dst = &state->cfg;
	size_t hdr_len = offsetof(struct t23_isp_ext_params_cfg, data);
	size_t pos = 0;

	if (!ext || payload < hdr_len || ext->data_size > payload - hdr_len)
		return;

	while (pos + sizeof(struct t23_isp_ext_params_block_header) <= ext->data_size) {
		const struct t23_isp_ext_params_block_header *hdr =
			(const void *)(ext->data + pos);
		bool dis = hdr->flags & T23_ISP_EXT_PARAMS_FL_BLOCK_DISABLE;

		if (hdr->size < sizeof(*hdr) || hdr->size > ext->data_size - pos)
			break;

		switch (hdr->type) {
		case T23_ISP_EXT_PARAMS_BLOCK_TYPE_BLC:
			if (dis) {
				state->blc_valid = false;
				state->blc_dirty = false;
			} else {
				const struct t23_isp_blc_config *src =
					&((const struct t23_isp_ext_params_blc_config *)hdr)->config;

				if (!state->blc_valid ||
				    memcmp(&state->blc, src, sizeof(state->blc))) {
					state->blc = *src;
					state->blc_valid = true;
					state->blc_dirty = true;
				}
			}
			break;
		case T23_ISP_EXT_PARAMS_BLOCK_TYPE_WB:
			if (dis) {
				state->valid &= ~TX_ISP_PARAM_WB;
				state->dirty &= ~TX_ISP_PARAM_WB;
			} else {
				tx_isp_merge_block(state, TX_ISP_PARAM_WB,
						   &dst->wb,
						   &((const struct t23_isp_ext_params_wb_config *)hdr)->config,
						   sizeof(dst->wb));
			}
			break;
		case T23_ISP_EXT_PARAMS_BLOCK_TYPE_CCM:
			if (dis) {
				state->valid &= ~TX_ISP_PARAM_CCM;
				state->dirty &= ~TX_ISP_PARAM_CCM;
			} else {
				tx_isp_merge_block(state, TX_ISP_PARAM_CCM,
						   &dst->ccm,
						   &((const struct t23_isp_ext_params_ccm_config *)hdr)->config,
						   sizeof(dst->ccm));
			}
			break;
		case T23_ISP_EXT_PARAMS_BLOCK_TYPE_GAMMA:
			if (dis) {
				state->valid &= ~TX_ISP_PARAM_GAMMA;
				state->dirty &= ~TX_ISP_PARAM_GAMMA;
			} else {
				tx_isp_merge_block(state, TX_ISP_PARAM_GAMMA,
						   &dst->gamma,
						   &((const struct t23_isp_ext_params_gamma_config *)hdr)->config,
						   sizeof(dst->gamma));
			}
			break;
		case T23_ISP_EXT_PARAMS_BLOCK_TYPE_LSC:
			if (dis) {
				state->valid &= ~TX_ISP_PARAM_LSC;
				state->dirty &= ~TX_ISP_PARAM_LSC;
			} else {
				tx_isp_merge_lsc(state,
						 &((const struct t23_isp_ext_params_lsc_config *)hdr)->config);
			}
			break;
		case T23_ISP_EXT_PARAMS_BLOCK_TYPE_MDNS:
			if (dis) {
				state->valid &= ~TX_ISP_PARAM_MDNS;
				state->dirty &= ~TX_ISP_PARAM_MDNS;
			} else {
				tx_isp_merge_block(state, TX_ISP_PARAM_MDNS,
						   &dst->mdns,
						   &((const struct t23_isp_ext_params_mdns_config *)hdr)->config,
						   sizeof(dst->mdns));
			}
			break;
		case T23_ISP_EXT_PARAMS_BLOCK_TYPE_SDNS:
			if (dis) {
				state->valid &= ~TX_ISP_PARAM_SDNS;
				state->dirty &= ~TX_ISP_PARAM_SDNS;
			} else {
				tx_isp_merge_block(state, TX_ISP_PARAM_SDNS,
						   dst->sdns,
						   ((const struct t23_isp_ext_params_sdns_config *)hdr)->words,
						   sizeof(dst->sdns));
			}
			break;
		case T23_ISP_EXT_PARAMS_BLOCK_TYPE_YDNS:
			if (dis) {
				state->valid &= ~TX_ISP_PARAM_YDNS;
				state->dirty &= ~TX_ISP_PARAM_YDNS;
			} else {
				tx_isp_merge_block(state, TX_ISP_PARAM_YDNS,
						   dst->ydns,
						   ((const struct t23_isp_ext_params_ydns_config *)hdr)->words,
						   sizeof(dst->ydns));
			}
			break;
		case T23_ISP_EXT_PARAMS_BLOCK_TYPE_YSP:
			if (dis) {
				state->valid &= ~TX_ISP_PARAM_YSP;
				state->dirty &= ~TX_ISP_PARAM_YSP;
			} else {
				tx_isp_merge_block(state, TX_ISP_PARAM_YSP,
						   dst->ysp,
						   ((const struct t23_isp_ext_params_ysp_config *)hdr)->words,
						   sizeof(dst->ysp));
			}
			break;
		case T23_ISP_EXT_PARAMS_BLOCK_TYPE_DMSC:
			if (dis) {
				state->valid &= ~TX_ISP_PARAM_DMSC;
				state->dirty &= ~TX_ISP_PARAM_DMSC;
			} else {
				tx_isp_merge_block(state, TX_ISP_PARAM_DMSC,
						   dst->dmsc,
						   ((const struct t23_isp_ext_params_dmsc_config *)hdr)->words,
						   sizeof(dst->dmsc));
			}
			break;
		default:
			break;
		}
		pos += hdr->size;
	}
}

static void tx_isp_params_write_ccm(void __iomem *base, const s16 *c)
{
	unsigned int i;
	u32 value;

	/* Each coefficient word latches through the 0x5000 strobe. */
	for (i = 0; i < 9; i += 2) {
		value = (u16)c[i] & 0x3fff;
		if (i < 8)
			value |= ((u16)c[i + 1] & 0x3fff) << 16;
		writel(ISP_CCM_TRIGGER_STROBE, base + ISP_CCM_TRIGGER);
		writel(value, base + ISP_CCM_COEFF_BASE + i * 2);
	}
}

static void tx_isp_params_write_lsc(void __iomem *base,
				    const struct t23_isp_lsc_config *lsc)
{
	unsigned int i;

	writel(lsc->mesh_height << 16 | lsc->mesh_width, base + ISP_LSC_GRID_SIZE);
	writel(lsc->stride << 16 | lsc->mean_en << 15 | lsc->scale,
	       base + ISP_LSC_CONTROL);
	/* Each mesh cell occupies three words of a four-word slot. */
	for (i = 0; i < lsc->words; i++)
		writel(lsc->lut[i], base + ISP_LSC_LUT_BASE + (i / 3) * 16 + (i % 3) * 4);
	writel(ISP_LSC_LUT_TRIGGER_COMMIT, base + ISP_LSC_LUT_TRIGGER);
}

/* MDNS is written by tx_isp_mdns_start() inside its hold window. */
void tx_isp_params_write(struct tx_isp_subdev *sd,
			 struct tx_isp_params_state *state)
{
	const struct tx_isp_params_values *cfg = &state->cfg;
	void __iomem *base = sd->base;
	u32 dirty = state->valid & state->dirty;
	unsigned int i;
	u32 value;

	if (state->blc_valid && state->blc_dirty) {
		writel(ISP_DGAIN_TRIGGER_COMMIT, base + ISP_DGAIN_TRIGGER);
		writel(state->blc.r & 0xfff, base + ISP_DGAIN_BLC0);
		writel(ISP_DGAIN_TRIGGER_COMMIT, base + ISP_DGAIN_TRIGGER);
		writel(((u32)(state->blc.gb & 0xfff) << 16) |
		       (state->blc.gr & 0xfff), base + ISP_DGAIN_BLC1);
		writel(ISP_DGAIN_TRIGGER_COMMIT, base + ISP_DGAIN_TRIGGER);
		writel(state->blc.b & 0xfff, base + ISP_DGAIN_BLC2);
	}
	if (dirty & TX_ISP_PARAM_WB) {
		u32 r = ISP_WB_GAIN_TAG | cfg->wb.gain_r;
		u32 b = ISP_WB_GAIN_TAG | cfg->wb.gain_b;

		writel(ISP_WB_TRIGGER_COMMIT, base + ISP_WB_TRIGGER);
		writel(r, base + ISP_WB_GAIN_R);
		writel(ISP_WB_TRIGGER_COMMIT, base + ISP_WB_TRIGGER);
		writel(b, base + ISP_WB_GAIN_B);
		writel(ISP_WB_TRIGGER_COMMIT, base + ISP_WB_TRIGGER);
		writel(r, base + ISP_WB_GAIN_GR);
		writel(ISP_WB_TRIGGER_COMMIT, base + ISP_WB_TRIGGER);
		writel(b, base + ISP_WB_GAIN_GB);
	}
	if ((dirty & TX_ISP_PARAM_GAMMA) && sd->res &&
	    resource_size(sd->res) >= 0x50200) {
		for (i = 0; i < T23_ISP_GAMMA_POINTS - 1; i++) {
			value = cfg->gamma.curve[i] |
				(u32)cfg->gamma.curve[i + 1] << 12;
			writel(value, base + ISP_GAMMA_LUT_R + i * 4);
			writel(value, base + ISP_GAMMA_LUT_G + i * 4);
			writel(value, base + ISP_GAMMA_LUT_B + i * 4);
		}
	}
	if (dirty & TX_ISP_PARAM_YSP) {
		for (i = 0; i < T23_ISP_YSP_WORDS; i++)
			writel(cfg->ysp[i], base + ISP_YSP_REG_BASE + i * 4);
		writel(ISP_YSP_REFRESH_COMMIT, base + ISP_YSP_REFRESH);
	}
	if (dirty & TX_ISP_PARAM_DMSC) {
		for (i = 0; i < T23_ISP_DMSC_WORDS; i++)
			writel(cfg->dmsc[i], base + dmsc_regs[i]);
		writel(ISP_DMSC_REFRESH_COMMIT, base + ISP_DMSC_TRIGGER);
	}
	if (dirty & TX_ISP_PARAM_YDNS)
		for (i = 0; i < T23_ISP_YDNS_WORDS; i++)
			writel(cfg->ydns[i], base + ISP_YDNS_REG_BASE + i * 4);
	if (dirty & TX_ISP_PARAM_SDNS) {
		for (i = 0; i < T23_ISP_SDNS_WORDS; i++)
			writel(cfg->sdns[i], base + sdns_regs[i]);
		writel(ISP_SDNS_REFRESH_COMMIT, base + ISP_SDNS_REFRESH);
	}
	if ((dirty & TX_ISP_PARAM_LSC) && sd->res &&
	    resource_size(sd->res) >= 0x2c000)
		tx_isp_params_write_lsc(base, &cfg->lsc);
	if (dirty & TX_ISP_PARAM_CCM)
		tx_isp_params_write_ccm(base, cfg->ccm.coeff);

	state->dirty = 0;
	state->blc_dirty = false;
}
