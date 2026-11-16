/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */
#ifndef __TX_ISP_PARAMS_H__
#define __TX_ISP_PARAMS_H__

#include <linux/bits.h>
#include <linux/media/ingenic/t23-isp-config.h>
#include <linux/types.h>

#define TX_ISP_PARAM_WB			BIT(0)
#define TX_ISP_PARAM_CCM		BIT(1)
#define TX_ISP_PARAM_GAMMA		BIT(2)
#define TX_ISP_PARAM_LSC		BIT(3)
#define TX_ISP_PARAM_MDNS		BIT(4)
#define TX_ISP_PARAM_SDNS		BIT(5)
#define TX_ISP_PARAM_YDNS		BIT(6)
#define TX_ISP_PARAM_YSP		BIT(7)
#define TX_ISP_PARAM_DMSC		BIT(8)

struct tx_isp_params_values {
	struct t23_isp_wb_config wb;
	struct t23_isp_ccm_config ccm;
	struct t23_isp_gamma_config gamma;
	struct t23_isp_mdns_config mdns;
	u32 sdns[T23_ISP_SDNS_WORDS];
	u32 ydns[T23_ISP_YDNS_WORDS];
	u32 ysp[T23_ISP_YSP_WORDS];
	u32 dmsc[T23_ISP_DMSC_WORDS];
	struct t23_isp_lsc_config lsc;
};

struct tx_isp_subdev;

struct tx_isp_params_state {
	struct tx_isp_params_values cfg;
	struct t23_isp_blc_config blc;
	u32 valid;		/* TX_ISP_PARAM_* blocks held in @cfg */
	u32 dirty;		/* TX_ISP_PARAM_* blocks needing HW write */
	bool blc_valid;
	bool blc_dirty;
};

int tx_isp_ext_params_check(const struct t23_isp_ext_params_cfg *ext,
			    size_t payload);
void tx_isp_ext_params_merge(struct tx_isp_params_state *state,
			     const struct t23_isp_ext_params_cfg *ext,
			     size_t payload);
void tx_isp_params_write(struct tx_isp_subdev *sd,
			 struct tx_isp_params_state *state);

#endif
