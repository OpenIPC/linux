/*
 * (C) Copyright 2019-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef _AIPU_PMU_H_
#define _AIPU_PMU_H_

#include <linux/rwlock_types.h>
#include <linux/hrtimer.h>
#include "aipu_core.h"

/**
 * Zhouyi AIPU PMU (Performance Monitoring Unit) Control Register Map
 */
#define AIPU_AXI_PMU_EN_OFFSET		0x400
#define AIPU_AXI_PMU_CLR_OFFSET		0x404
#define AIPU_AXI_PMU_PRD_OFFSET		0x408
#define AIPU_AXI_PMU_LAT_RD_OFFSET	0x40C
#define AIPU_AXI_PMU_LAT_WR_OFFSET	0x410
#define AIPU_AXI_PMU_REQ_RD_OFFSET	0x414
#define AIPU_AXI_PMU_REQ_WR_OFFSET	0x418
#define AIPU_AXI_PMU_BW_RD_OFFSET	0x41C
#define AIPU_AXI_PMU_BW_WR_OFFSET	0x420

typedef struct {
	struct aipu_core *core;
	int enable;
	rwlock_t lock;
	unsigned long clk_rate;		/* Hz */
	struct hrtimer timer;
} aipu_pmu_t;

int aipu_init_pmu(struct device *dev, unsigned long clk_rate, struct aipu_core *core);
void aipu_deinit_pmu(struct device *dev);

#endif /* ifndef _AIPU_PMU_H_ */
