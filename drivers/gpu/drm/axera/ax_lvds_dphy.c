/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/of_address.h>
#include <drm/drmP.h>
#include "ax_reg_dphy.h"

struct dphy_lvds_pll_cfg {
	u8 pll_pos_div;
	u16 pll_fbk_int;
	u32 pll_fbk_fra;
};

struct ax_dphy_lvds_pll_cfg {
	unsigned long long bps;
	struct dphy_lvds_pll_cfg pll_cfg;
};

static struct ax_dphy_lvds_pll_cfg ref12m_dphy_lvds_pll_cfg[] = {
	{
		.bps = 80000000UL,
		{
			.pll_fbk_int = 0x6a,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 81000000UL,
		{
			.pll_fbk_int = 0x6c,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 82000000UL,
		{
			.pll_fbk_int = 0x6d,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x4,

		}
	},
	{
		.bps = 83000000UL,
		{
			.pll_fbk_int = 0x6e,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 84000000UL,
		{
			.pll_fbk_int = 0x70,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 85000000UL,
		{
			.pll_fbk_int = 0x71,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 86000000UL,
		{
			.pll_fbk_int = 0x72,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 87000000UL,
		{
			.pll_fbk_int = 0x72,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 88000000UL,
		{
			.pll_fbk_int = 0x74,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 89000000UL,
		{
			.pll_fbk_int = 0x76,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 90000000UL,
		{
			.pll_fbk_int = 0x78,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 91000000UL,
		{
			.pll_fbk_int = 0x79,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 92000000UL,
		{
			.pll_fbk_int = 0x7a,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 93000000UL,
		{
			.pll_fbk_int = 0x7c,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 94000000UL,
		{
			.pll_fbk_int = 0x7d,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 95000000UL,
		{
			.pll_fbk_int = 0x7e,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 96000000UL,
		{
			.pll_fbk_int = 0x80,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 97000000UL,
		{
			.pll_fbk_int = 0x81,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 98000000UL,
		{
			.pll_fbk_int = 0x82,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 99000000UL,
		{
			.pll_fbk_int = 0x84,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 100000000UL,
		{
			.pll_fbk_int = 0x85,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 101000000UL,
		{
			.pll_fbk_int = 0x86,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 102000000UL,
		{
			.pll_fbk_int = 0x88,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 103000000UL,
		{
			.pll_fbk_int = 0x89,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 104000000UL,
		{
			.pll_fbk_int = 0x8a,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 105000000UL,
		{
			.pll_fbk_int = 0x8c,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 106000000UL,
		{
			.pll_fbk_int = 0x8d,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 107000000UL,
		{
			.pll_fbk_int = 0x8e,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 108000000UL,
		{
			.pll_fbk_int = 0x90,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 109000000UL,
		{
			.pll_fbk_int = 0x91,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 110000000UL,
		{
			.pll_fbk_int = 0x92,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 111000000UL,
		{
			.pll_fbk_int = 0x94,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 112000000UL,
		{
			.pll_fbk_int = 0x95,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x4,
		}
	},
	{
		.bps = 176000000UL,
		{
			.pll_fbk_int = 0x75,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x3,
		}
	},
	{
		.bps = 189000000UL,
		{
			.pll_fbk_int = 0x7e,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x3,
		}
	},
	{
		.bps = 205000000UL,
		{
			.pll_fbk_int = 0x88,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x3,
		}
	},
	{
		.bps = 280000000UL,
		{
			.pll_fbk_int = 0xba,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x3,
		}
	},
	{
		.bps = 364000000UL,
		{
			.pll_fbk_int = 0x79,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x2,
		}
	},
	{
		.bps = 455000000UL,
		{
			.pll_fbk_int = 0x97,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x2,
		}
	},
	{
		.bps = 520000000UL,
		{
			.pll_fbk_int = 0xad,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x2,
		}
	},
	{
		.bps = 585000000UL,
		{
			.pll_fbk_int = 0xc3,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x2,
		}
	},
	{
		.bps = 599000000UL,
		{
			.pll_fbk_int = 0xc7,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x2,
		}
	},
	{
		.bps = 600000000UL,
		{
			.pll_fbk_int = 0xc8,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x2,
		}
	},
	{
		.bps = 746000000UL,
		{
			.pll_fbk_int = 0x7c,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x1,
		}
	},
	{
		.bps = 756000000UL,
		{
			.pll_fbk_int = 0x7e,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x1,
		}
	},
	{
		.bps = 1024000000UL,
		{
			.pll_fbk_int = 0xaa,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x1,
		}
	},
	{
		.bps = 1040000000UL,
		{
			.pll_fbk_int = 0xad,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x1,
		}
	},
	{
		.bps = 1078000000UL,
		{
			.pll_fbk_int = 0xb3,
			.pll_fbk_fra = 0xaaaaaa,
			.pll_pos_div = 0x1,
		}
	},
	{
		.bps = 1134000000UL,
		{
			.pll_fbk_int = 0xbd,
			.pll_fbk_fra = 0x0,
			.pll_pos_div = 0x1,
		}
	},
	{
		.bps = 2079000000UL,
		{
			.pll_fbk_int = 0xad,
			.pll_fbk_fra = 0x400000,
			.pll_pos_div = 0x0,
		}
	},
	{
		.bps = 2440000000UL,
		{
			.pll_fbk_int = 0xcb,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x0,
		}
	},
	{
		.bps = 2500000000UL,
		{
			.pll_fbk_int = 0xd0,
			.pll_fbk_fra = 0x555555,
			.pll_pos_div = 0x0,
		}
	},
};

void lvds_dphy_pll_cfg( unsigned long long bps, void __iomem *regs)
{
	int i;

	for (i = 0; sizeof(ref12m_dphy_lvds_pll_cfg) / sizeof(ref12m_dphy_lvds_pll_cfg[0]); i++) {
		if (ref12m_dphy_lvds_pll_cfg[i].bps >= bps) {
			DRM_INFO("%s expected_lane_bps = %lld, actual_lane_bps = %lld\n",
					 __func__,
					 bps,
					 ref12m_dphy_lvds_pll_cfg[i].bps);

			writel(ref12m_dphy_lvds_pll_cfg[i].pll_cfg.pll_pos_div, regs + DPHY_TX0_REG3_ADDR);
			writel(ref12m_dphy_lvds_pll_cfg[i].pll_cfg.pll_fbk_int, regs + DPHY_TX0_REG0_ADDR);
			writel(ref12m_dphy_lvds_pll_cfg[i].pll_cfg.pll_fbk_fra, regs + DPHY_TX0_REG1_ADDR);

			return;
		}
	}
}