/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_LVDS_H
#define __AX_LVDS_H

#include "ax_drm_drv.h"

enum lvds_pixel_format {
	LVDS_VESA_24,
	LVDS_JEIDA_24,
	LVDS_VESA_18,
	LVDS_JEIDA_18,
};

struct ax_lvds {
	struct drm_encoder encoder;
	struct drm_connector connector;
	struct drm_bridge bridge;
	struct drm_panel *panel;

	void __iomem *regs;
	void __iomem *dphytx_regs;
	void __iomem *comm_sys_regs;
	void __iomem *dispc_sys_regs;

	struct reset_control *lvds_prst_ctrl;
	struct reset_control *dphytx_pll_rst_ctrl;
	struct reset_control *dphytx_pll_div7_rst_ctrl;

	struct clk *lvds_p_clk;
	struct clk *dphytx_pll_clk;
	struct clk *dphytx_pll_div7_clk;
	struct clk *dphytx_ref_clk;
	struct clk *comm_dphytx_tlb_clk;
	int fmt_out;
	int clock;
	bool status;
};

#endif /* __AX_LVDS_H */
