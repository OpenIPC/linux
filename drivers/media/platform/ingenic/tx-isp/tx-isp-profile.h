/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */
#ifndef TX_ISP_PROFILE_H
#define TX_ISP_PROFILE_H

#include <linux/types.h>
#include <linux/io.h>

int tx_isp_profile_init(void __iomem *base, size_t size,
			unsigned int width, unsigned int height);
void tx_isp_profile_restore_blocks(void __iomem *base, size_t size, u32 mask);

#endif
