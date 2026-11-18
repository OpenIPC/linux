/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_VIRT_CONNECTOR_H
#define __AX_VIRT_CONNECTOR_H

#include "ax_drm_drv.h"

/* common_sys_glb registers definition start */
#define COMM_SYSGLB_CLK_EB0                      0x24
#define COMM_SYSGLB_CLK_EB0_SET                  0x28
#define COMM_SYSGLB_CLK_EB0_CLR                  0x2C

#define COMM_SYSGLB_CLK_NX_DPULITE_EB            BIT(13)
#define COMM_SYSGLB_CLK_1X_DPULITE_EB            BIT(7)

#define COMM_SYSGLB_VO_CFG                       0x424
#define COMM_SYSGLB_VO_CFG_SET                   0x428
#define COMM_SYSGLB_VO_CFG_CLR                   0x42C

#define COMM_SYSGLB_LCD_VOMUX_SEL	         BIT(0)
#define COMM_SYSGLB_DPULITE_DMUX_SEL	         BIT(1)
#define COMM_SYSGLB_DPULITE_DPHYTX_EN	         BIT(2)
#define COMM_SYSGLB_DPULITE_TX_CLKING_MODE	 BIT(3)
/* common_sys_glb registers definition end */


/* flash_sys_glb registers definition start */
#define FLASH_SYSGLB_CLK_EB0                    0x4
#define FLASH_SYSGLB_CLK_EB0_SET                0x4004
#define FLASH_SYSGLB_CLK_EB0_CLR                0x8004

#define FLASH_SYSGLB_CLK_1X_DPU                BIT(0)
#define FLASH_SYSGLB_CLK_NX_DPU                BIT(7)

#define FLASH_SYSGLB_IMAGE_TX                   0x1B8
#define FLASH_SYSGLB_IMAGE_TX_SET               0x41B8
#define FLASH_SYSGLB_IMAGE_TX_CLR               0x81B8

#define FLASH_SYSGLB_IMAGE_TX_EN                BIT(0)
/* flash_sys_glb registers definition end */

/* dispc_sys_glb registers definition start */
#define DISPC_SYSGLB_LVDS_CLK_SEL                   0x4C
#define DISPC_SYSGLB_LVDS_CLK_SEL_SET               0x120
#define DISPC_SYSGLB_LVDS_CLK_SEL_CLR               0x124
/* dispc_sys_glb registers definition end */

enum {
	VIRT_STATUS_CREATED = 1,
	VIRT_STATUS_ENABLED,
	VIRT_STATUS_DISABLED,
	VIRT_STATUS_DESTORY,
};

struct ax_virt_connector {
	u32 id;
	int state;
	int intf_type;
	int fmt_out;
	bool bt_8bit_low;

	int encoder_type;
	struct drm_encoder encoder;

	int connector_type;
	struct drm_connector connector;
	u32 dmux_sel;

	struct {
		struct drm_property *intf_type;
		struct drm_property *fmt_out;
	} props;

	struct drm_panel *panel;

	struct drm_crtc *crtc;
	void __iomem *common_glb_regs;
	void __iomem *flashsys_glb_regs;
	void __iomem *dispsys_glb_regs;

	struct reset_control *common_1x_rst_ctrl;
	struct reset_control *common_nx_rst_ctrl;
	struct reset_control *flash_1x_rst_ctrl;
	struct reset_control *flash_nx_rst_ctrl;

	struct clk *common_1x_clk;
	struct clk *common_nx_clk;
	struct clk *flash_1x_clk;
	struct clk *flash_nx_clk;
	int clock;
};

#endif /* __AX_VIRT_CONNECTOR_H */
