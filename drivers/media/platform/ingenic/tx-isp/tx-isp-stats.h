/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */
#ifndef __TX_ISP_STATS_H__
#define __TX_ISP_STATS_H__

#include <linux/dma-mapping.h>
#include <linux/io.h>

/* Per input: AE +0, histogram +0x1000, AWB +0x2000, AF +0x3000 */
#define T23_STATS_INPUT_SIZE	0xc000

struct t23_isp_stats {
	struct device *dev;
	void *buffer;
	dma_addr_t dma;
	bool active;
};

int t23_isp_stats_alloc(struct t23_isp_stats *stats, struct device *dev);
int t23_isp_stats_configure(struct t23_isp_stats *stats, void __iomem *base,
			    unsigned int input);
int t23_isp_stats_free(struct t23_isp_stats *stats);

#endif
