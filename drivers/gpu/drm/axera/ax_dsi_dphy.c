/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <drm/drm_atomic_helper.h>
#include <drm/drm_bridge.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <video/mipi_display.h>
#include <video/videomode.h>
#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/component.h>
#include <linux/soc/axera/ax_boardinfo.h>

#include "ax_mipi_dsi.h"
#include "ax_reg_dphy.h"

struct dphy_pll_cfg {
	u8 pll_pre_div;
	u16 pll_fbk_int;
	u32 pll_fbk_fra;

	u8 extd_cycle_sel;

	u8 dlane_hs_pre_time;
	u8 dlane_hs_zero_time;
	u8 dlane_hs_trail_time;

	u8 clane_hs_pre_time;
	u8 clane_hs_zero_time;
	u8 clane_hs_trail_time;
	u8 clane_hs_clk_pre_time;
	u8 clane_hs_clk_post_time;
};

struct ax_dphy_pll_cfg {
	unsigned long long lane_bps;
	struct dphy_pll_cfg pll_cfg;
};

static struct ax_dphy_pll_cfg ref12m_dphy_pll_cfg[] = {
	{
		.lane_bps = 79000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6a,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2b,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x71,
		}
	},
	{
		.lane_bps = 81000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6c,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2c,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x71,
		}
	},
	{
		.lane_bps = 83200000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6e,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x1e,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x71,
		}
	},
	{
		.lane_bps = 83250000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6e,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x1e,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x71,
		}
	},
	{
		.lane_bps = 88800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x75,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0xf,
			.dlane_hs_zero_time = 0x1f,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x72,
		}
	},
	{
		.lane_bps = 96000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x80,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x20,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x73,
		}
	},
	{
		.lane_bps = 99000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x21,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x73,
		}
	},
	{
		.lane_bps = 99900000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x21,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x73,
		}
	},
	{
		.lane_bps = 104000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x8a,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x22,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x37,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x73,
		}
	},
	{
		.lane_bps = 108000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x90,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x11,
			.dlane_hs_zero_time = 0x22,
			.dlane_hs_trail_time = 0x18,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3a,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x74,
		}
	},
	{
		.lane_bps = 111000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x11,
			.dlane_hs_zero_time = 0x23,
			.dlane_hs_trail_time = 0x18,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x74,
		}
	},
	{
		.lane_bps = 118800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x9d,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x12,
			.dlane_hs_zero_time = 0x24,
			.dlane_hs_trail_time = 0x19,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3f,
			.clane_hs_trail_time = 0x11,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x75,
		}
	},
	{
		.lane_bps = 124800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa5,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x12,
			.dlane_hs_zero_time = 0x26,
			.dlane_hs_trail_time = 0x1a,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x42,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x76,
		}
	},
	{
		.lane_bps = 124875000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa5,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x12,
			.dlane_hs_zero_time = 0x26,
			.dlane_hs_trail_time = 0x1a,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x42,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x76,
		}
	},
	{
		.lane_bps = 128000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xaa,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x13,
			.dlane_hs_zero_time = 0x26,
			.dlane_hs_trail_time = 0x1a,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x44,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x76,
		}
	},
	{
		.lane_bps = 132000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x13,
			.dlane_hs_zero_time = 0x27,
			.dlane_hs_trail_time = 0x1b,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x46,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x77,
		}
	},
	{
		.lane_bps = 133200000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb1,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x13,
			.dlane_hs_zero_time = 0x28,
			.dlane_hs_trail_time = 0x1b,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x47,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x77,
		}
	},
	{
		.lane_bps = 144000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x14,
			.dlane_hs_zero_time = 0x2a,
			.dlane_hs_trail_time = 0x1c,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4c,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x78,
		}
	},
	{
		.lane_bps = 148000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc5,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x14,
			.dlane_hs_zero_time = 0x2b,
			.dlane_hs_trail_time = 0x1d,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x79,
		}
	},
	{
		.lane_bps = 148500000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc5,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x14,
			.dlane_hs_zero_time = 0x2b,
			.dlane_hs_trail_time = 0x1d,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x79,
		}
	},
	{
		.lane_bps = 151050000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc9,
			.pll_fbk_fra = 0x666666,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x15,
			.dlane_hs_zero_time = 0x2b,
			.dlane_hs_trail_time = 0x1d,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x51,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x79,
		}
	},
	{
		.lane_bps = 156000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x4,
			.dlane_hs_pre_time = 0x15,
			.dlane_hs_zero_time = 0x2c,
			.dlane_hs_trail_time = 0x1e,
			.clane_hs_pre_time = 0xb,
			.clane_hs_zero_time = 0x53,
			.clane_hs_trail_time = 0x16,
			.clane_hs_clk_pre_time = 0xf,
			.clane_hs_clk_post_time = 0x7a,
		}
	},
	{
		.lane_bps = 158400000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x69,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x17,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2a,
			.clane_hs_trail_time = 0xc,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3c,
		}
	},
	{
		.lane_bps = 162000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6c,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x17,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2c,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3d,
		}
	},
	{
		.lane_bps = 166400000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6e,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3d,
		}
	},
	{
		.lane_bps = 166500000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6e,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3d,
		}
	},
	{
		.lane_bps = 177600000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x76,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3e,
		}
	},
	{
		.lane_bps = 178200000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x76,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3e,
		}
	},
	{
		.lane_bps = 192000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x80,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1a,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3f,
		}
	},
	{
		.lane_bps = 198000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1b,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3f,
		}
	},
	{
		.lane_bps = 199000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1b,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3f,
		}
	},
	{
		.lane_bps = 199800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1b,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3f,
		}
	},
	{
		.lane_bps = 208000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x8a,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1c,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x37,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x3f,
		}
	},
	{
		.lane_bps = 216000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x90,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1c,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3a,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x40,
		}
	},
	{
		.lane_bps = 222000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x40,
		}
	},
	{
		.lane_bps = 222750000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x800000,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x40,
		}
	},
	{
		.lane_bps = 237600000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x9e,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x1e,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3f,
			.clane_hs_trail_time = 0x11,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x41,
		}
	},
	{
		.lane_bps = 249600000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x20,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x43,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x42,
		}
	},
	{
		.lane_bps = 249750000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x20,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x43,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x42,
		}
	},
	{
		.lane_bps = 264000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xf,
			.dlane_hs_zero_time = 0x21,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x46,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x43,
		}
	},
	{
		.lane_bps = 266400000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb1,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0xf,
			.dlane_hs_zero_time = 0x22,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x47,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x43,
		}
	},
	{
		.lane_bps = 288000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x24,
			.dlane_hs_trail_time = 0x18,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4c,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x44,
		}
	},
	{
		.lane_bps = 296000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc5,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x25,
			.dlane_hs_trail_time = 0x19,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x45,
		}
	},
	{
		.lane_bps = 297000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc6,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0x10,
			.dlane_hs_zero_time = 0x25,
			.dlane_hs_trail_time = 0x19,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x45,
		}
	},
	{
		.lane_bps = 300000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc8,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0x11,
			.dlane_hs_zero_time = 0x25,
			.dlane_hs_trail_time = 0x19,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x50,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x45,
		}
	},
	{
		.lane_bps = 312000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x3,
			.dlane_hs_pre_time = 0x11,
			.dlane_hs_zero_time = 0x26,
			.dlane_hs_trail_time = 0x1a,
			.clane_hs_pre_time = 0xb,
			.clane_hs_zero_time = 0x53,
			.clane_hs_trail_time = 0x16,
			.clane_hs_clk_pre_time = 0x7,
			.clane_hs_clk_post_time = 0x46,
		}
	},
	{
		.lane_bps = 316800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x69,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xe,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2a,
			.clane_hs_trail_time = 0xc,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x22,
		}
	},
	{
		.lane_bps = 324000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6c,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2c,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x23,
		}
	},
	{
		.lane_bps = 332800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6e,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x15,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x23,
		}
	},
	{
		.lane_bps = 333000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6f,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x15,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x23,
		}
	},
	{
		.lane_bps = 334125000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6f,
			.pll_fbk_fra = 0x600000,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x23,
		}
	},
	{
		.lane_bps = 356400000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x76,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x16,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x24,
		}
	},
	{
		.lane_bps = 384000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x80,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x17,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x25,
		}
	},
	{
		.lane_bps = 396000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x25,
		}
	},
	{
		.lane_bps = 399000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x25,
		}
	},
	{
		.lane_bps = 399600000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x25,
		}
	},
	{
		.lane_bps = 416000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x8a,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x37,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x25,
		}
	},
	{
		.lane_bps = 432000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x90,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3a,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x26,
		}
	},
	{
		.lane_bps = 444000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1a,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x26,
		}
	},
	{
		.lane_bps = 445500000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x800000,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1a,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x26,
		}
	},
	{
		.lane_bps = 475200000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x9e,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1b,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3f,
			.clane_hs_trail_time = 0x11,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x27,
		}
	},
	{
		.lane_bps = 499000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x42,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x28,
		}
	},
	{
		.lane_bps = 499200000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x43,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x28,
		}
	},
	{
		.lane_bps = 499500000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x43,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x28,
		}
	},
	{
		.lane_bps = 528000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1e,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x46,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x29,
		}
	},
	{
		.lane_bps = 532800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb1,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1f,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x47,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x29,
		}
	},
	{
		.lane_bps = 576000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x21,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4c,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x2a,
		}
	},
	{
		.lane_bps = 594000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc6,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x22,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x2b,
		}
	},
	{
		.lane_bps = 600000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc8,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x23,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x50,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x2b,
		}
	},
	{
		.lane_bps = 624000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x2,
			.dlane_hs_pre_time = 0xf,
			.dlane_hs_zero_time = 0x23,
			.dlane_hs_trail_time = 0x18,
			.clane_hs_pre_time = 0xb,
			.clane_hs_zero_time = 0x53,
			.clane_hs_trail_time = 0x16,
			.clane_hs_clk_pre_time = 0x3,
			.clane_hs_clk_post_time = 0x2c,
		}
	},
	{
		.lane_bps = 648000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6c,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x7,
			.dlane_hs_zero_time = 0x13,
			.dlane_hs_trail_time = 0xe,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2c,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x16,
		}
	},
	{
		.lane_bps = 666000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6f,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x7,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xe,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x16,
		}
	},
	{
		.lane_bps = 668250000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6f,
			.pll_fbk_fra = 0x600000,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x13,
			.dlane_hs_trail_time = 0xe,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x16,
		}
	},
	{
		.lane_bps = 699000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x74,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x16,
		}
	},
	{
		.lane_bps = 712800000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x76,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x17,
		}
	},
	{
		.lane_bps = 792000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x16,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x18,
		}
	},
	{
		.lane_bps = 799000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x17,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x18,
		}
	},
	{
		.lane_bps = 799200000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x2aaaaa,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x16,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x18,
		}
	},
	{
		.lane_bps = 832000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x8a,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x37,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x18,
		}
	},
	{
		.lane_bps = 888000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x19,
		}
	},
	{
		.lane_bps = 891000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x800000,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x19,
		}
	},
	{
		.lane_bps = 900000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x96,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x19,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x19,
		}
	},
	{
		.lane_bps = 950400000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x9e,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1a,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3f,
			.clane_hs_trail_time = 0x11,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1a,
		}
	},
	{
		.lane_bps = 999000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1c,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x42,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1b,
		}
	},
	{
		.lane_bps = 1099000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb7,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1e,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x4a,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1d,
		}
	},
	{
		.lane_bps = 1136500000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xbd,
			.pll_fbk_fra = 0x6aaaaa,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1f,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4b,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1d,
		}
	},
	{
		.lane_bps = 1188000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc6,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x21,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1e,
		}
	},
	{
		.lane_bps = 1200000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc8,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x20,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x50,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1e,
		}
	},
	{
		.lane_bps = 1248000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x1,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x22,
			.dlane_hs_trail_time = 0x17,
			.clane_hs_pre_time = 0xb,
			.clane_hs_zero_time = 0x53,
			.clane_hs_trail_time = 0x16,
			.clane_hs_clk_pre_time = 0x1,
			.clane_hs_clk_post_time = 0x1f,
		}
	},
	{
		.lane_bps = 1299000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6c,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x7,
			.dlane_hs_zero_time = 0x12,
			.dlane_hs_trail_time = 0xd,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2c,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0xf,
		}
	},
	{
		.lane_bps = 1332000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x6f,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x7,
			.dlane_hs_zero_time = 0x12,
			.dlane_hs_trail_time = 0xd,
			.clane_hs_pre_time = 0x5,
			.clane_hs_zero_time = 0x2d,
			.clane_hs_trail_time = 0xd,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0xf,
		}
	},
	{
		.lane_bps = 1399000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x74,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x7,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xe,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x10,
		}
	},
	{
		.lane_bps = 1425600000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x76,
			.pll_fbk_fra = 0xc00000,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x13,
			.dlane_hs_trail_time = 0xe,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x2f,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x10,
		}
	},
	{
		.lane_bps = 1500000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x7d,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x14,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x6,
			.clane_hs_zero_time = 0x32,
			.clane_hs_trail_time = 0xe,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x11,
		}
	},
	{
		.lane_bps = 1584000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x8,
			.dlane_hs_zero_time = 0x16,
			.dlane_hs_trail_time = 0xf,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x34,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x11,
		}
	},
	{
		.lane_bps = 1599000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x15,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x35,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x12,
		}
	},
	{
		.lane_bps = 1664000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x8a,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x16,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x37,
			.clane_hs_trail_time = 0xf,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x12,
		}
	},
	{
		.lane_bps = 1699000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x8d,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0x9,
			.dlane_hs_zero_time = 0x17,
			.dlane_hs_trail_time = 0x10,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x39,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x12,
		}
	},
	{
		.lane_bps = 1782000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x800000,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x7,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x13,
		}
	},
	{
		.lane_bps = 1800000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x96,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x18,
			.dlane_hs_trail_time = 0x11,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3c,
			.clane_hs_trail_time = 0x10,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x13,
		}
	},
	{
		.lane_bps = 1899000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0x9e,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xa,
			.dlane_hs_zero_time = 0x1a,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x3f,
			.clane_hs_trail_time = 0x11,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x14,
		}
	},
	{
		.lane_bps = 1998000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0x800000,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1a,
			.dlane_hs_trail_time = 0x12,
			.clane_hs_pre_time = 0x8,
			.clane_hs_zero_time = 0x43,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x14,
		}
	},
	{
		.lane_bps = 1999000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xa6,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1b,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x42,
			.clane_hs_trail_time = 0x12,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x15,
		}
	},
	{
		.lane_bps = 2100000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xaf,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xb,
			.dlane_hs_zero_time = 0x1c,
			.dlane_hs_trail_time = 0x13,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x46,
			.clane_hs_trail_time = 0x13,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x15,
		}
	},
	{
		.lane_bps = 2199000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xb7,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1d,
			.dlane_hs_trail_time = 0x14,
			.clane_hs_pre_time = 0x9,
			.clane_hs_zero_time = 0x4a,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x16,
		}
	},
	{
		.lane_bps = 2299000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xbf,
			.pll_fbk_fra = 0xaaaaaa,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xc,
			.dlane_hs_zero_time = 0x1f,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4c,
			.clane_hs_trail_time = 0x14,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x17,
		}
	},
	{
		.lane_bps = 2376000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc6,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x1f,
			.dlane_hs_trail_time = 0x15,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x4f,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x17,
		}
	},
	{
		.lane_bps = 2400000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xc8,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xd,
			.dlane_hs_zero_time = 0x20,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0xa,
			.clane_hs_zero_time = 0x50,
			.clane_hs_trail_time = 0x15,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x18,
		}
	},
	{
		.lane_bps = 2496000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x0,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x20,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0xb,
			.clane_hs_zero_time = 0x53,
			.clane_hs_trail_time = 0x16,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x18,
		}
	},
	{
		.lane_bps = 2499000000UL,
		{
			.pll_pre_div = 0x0,
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x555555,
			.extd_cycle_sel = 0x0,
			.dlane_hs_pre_time = 0xe,
			.dlane_hs_zero_time = 0x21,
			.dlane_hs_trail_time = 0x16,
			.clane_hs_pre_time = 0xb,
			.clane_hs_zero_time = 0x53,
			.clane_hs_trail_time = 0x16,
			.clane_hs_clk_pre_time = 0x0,
			.clane_hs_clk_post_time = 0x18,
		}
	},
};

static void dsi_dphy_pll_cfg(struct dphy_pll_cfg *cfg, struct cdns_dphy *dphy)
{
	void __iomem *regs = dphy->regs;

	writel(cfg->pll_pre_div, regs + DPHY_TX0_REG2_ADDR);//{0x46, 0x10},		//pre div
	writel(cfg->pll_fbk_int, regs + DPHY_TX0_REG0_ADDR);
	writel(cfg->pll_fbk_fra, regs + DPHY_TX0_REG1_ADDR);

	writel(cfg->extd_cycle_sel, regs + DPHY_TX0_REG9_ADDR);//{0x47, 0x4},		//cycle sel

	writel(cfg->dlane_hs_pre_time, regs + DPHY_TX0_REG10_ADDR);
	writel(cfg->dlane_hs_zero_time, regs + DPHY_TX0_REG11_ADDR);
	writel(cfg->dlane_hs_trail_time, regs + DPHY_TX0_REG12_ADDR);

	writel(cfg->clane_hs_pre_time, regs + DPHY_TX0_REG13_ADDR);
	writel(cfg->clane_hs_zero_time, regs + DPHY_TX0_REG14_ADDR);
	writel(cfg->clane_hs_trail_time, regs + DPHY_TX0_REG15_ADDR);
	writel(cfg->clane_hs_clk_pre_time, regs + DPHY_TX0_REG16_ADDR);
	writel(cfg->clane_hs_clk_post_time, regs + DPHY_TX0_REG17_ADDR);
}

void dsi_dphy_config(int lanes, unsigned long long lane_bps, struct cdns_dphy *dphy)
{
	int i;
	u32 lane_mask;
	int board_id;

	DRM_INFO("%s enter, lanes = %d, lane_bps = %lld\n",
		__func__, lanes, lane_bps);

	for (i = 0, lane_mask = 0; i < lanes; i++) {
		lane_mask |= (1 << i);
	}

	/* leave isolation */
	writel(DPHYTX0_AON_POWER_READY_N_BIT, dphy->power_regs + DPHY_POWER_READY_CLR_ADDR);
	/* power on */
	writel(DPHYTX0_PWR_OFF_BIT, dphy->power_regs + DPHY_POWER_OFF_CLR_ADDR);
	udelay(20);

	board_id = ax_info_get_board_id();

// ### SIPEED EDIT ###
	if (board_id == AX630C_AX631_MAIXCAM2_SOM_0_5G || board_id == AX630C_AX631_MAIXCAM2_SOM_1G
		|| board_id == AX630C_AX631_MAIXCAM2_SOM_2G || board_id == AX630C_AX631_MAIXCAM2_SOM_4G
		|| board_id == AX630C_DEMO_LP4_V1_0 || board_id == AX630C_DEMO_V1_1 || board_id == AX620Q_LP4_DEMO_V1_1) {
		writel(1, dphy->regs + DPHY_TX0_REG22_ADDR);
		writel(0, dphy->regs + DPHY_TX0_REG23_ADDR);
		writel(4, dphy->regs + DPHY_TX0_REG24_ADDR);
	}
// ### SIPEED EDIT ###
	writel(1, dphy->regs + DPHY_PPI_REG_2_SET_ADDR);
	writel(lane_mask, dphy->regs + DPHY_PPI_REG_3_SET_ADDR);
	writel(1, dphy->regs + DPHY_MIPITX0_EN_SET_ADDR);

	for (i = 0; sizeof(ref12m_dphy_pll_cfg) / sizeof(ref12m_dphy_pll_cfg[0]); i++) {
		if (ref12m_dphy_pll_cfg[i].lane_bps >= lane_bps) {
			DRM_INFO("%s expected_lane_bps = %lld, actual_lane_bps = %lld\n",
					 __func__,
					 lane_bps,
					 ref12m_dphy_pll_cfg[i].lane_bps);

			dsi_dphy_pll_cfg(&ref12m_dphy_pll_cfg[i].pll_cfg, dphy);

			break;
		}
	}
}

