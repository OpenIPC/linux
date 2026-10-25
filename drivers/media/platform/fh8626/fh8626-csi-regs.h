/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Fullhan FH8626V100 MIPI CSI-2 host (DesignWare layout) and wrapper registers.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#ifndef _FH8626_CSI_REGS_H
#define _FH8626_CSI_REGS_H

#include <linux/bits.h>

#define CSI_N_LANES_OFFSET		0x04
#define CSI_RESETN			0x08
#define CSI_PHY_SHUTDOWNZ		0x40
#define CSI_DPHY_RSTZ			0x44
#define CSI_PHY_TEST_CTRL0		0x50
#define CSI_PHY_TEST_CTRL1		0x54

#define CSI_MASK_PHY_FATAL		0xe4
#define CSI_MASK_PKT_FATAL		0xf4
#define CSI_MASK_FRAME_FATAL		0x104
#define CSI_MASK_PHY			0x114
#define CSI_MASK_PKT			0x124
#define CSI_MASK_LINE			0x134

#define WRAP_CTRL			0x00
#define WRAP_CTRL_MODE_MASK		GENMASK(1, 0)
#define WRAP_CTRL_RAW_TYPE		BIT(2)
#define WRAP_INT_EN			0x04

#endif /* _FH8626_CSI_REGS_H */
