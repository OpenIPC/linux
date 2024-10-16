/*
 * (C) Copyright 2019-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/device.h>
#include "aipu_io.h"
#include "aipu_pmu.h"

static aipu_pmu_t aipu_pmu = {
	.core = NULL,
	.enable = 0,
};

static enum hrtimer_restart aipu_pmu_timer_function(struct hrtimer *hrtimer)
{
	static u64 total_ms;
	static u64 total_bw_read_kb;
	static u64 total_bw_write_kb;
	static u64 total_req_read;
	static u64 total_req_write;
	u32 bw_read_kb;
	u32 bw_write_kb;
	u32 req_read;
	u32 req_write;
	ktime_t now;
	unsigned long flags;

	now = hrtimer->base->get_time();

	bw_read_kb = aipu_read32(&aipu_pmu.core->reg[0], AIPU_AXI_PMU_BW_RD_OFFSET);
	bw_write_kb = aipu_read32(&aipu_pmu.core->reg[0], AIPU_AXI_PMU_BW_WR_OFFSET);
	req_read = aipu_read32(&aipu_pmu.core->reg[0], AIPU_AXI_PMU_REQ_RD_OFFSET);
	req_write = aipu_read32(&aipu_pmu.core->reg[0], AIPU_AXI_PMU_REQ_WR_OFFSET);
	total_bw_read_kb += bw_read_kb;
	total_bw_write_kb += bw_write_kb;
	total_req_read += req_read;
	total_req_write += req_write;

	total_ms += 1;
	/* Print the statistic once per second */
	if (total_ms >= 1000) {
		printk(KERN_NOTICE "[AIPU PMU] bw read: %llu KB, bw write: %llu KB, "
				"req read: %llu, req write: %llu\n",
				total_bw_read_kb, total_bw_write_kb,
				total_req_read, total_req_write);
		total_ms = 0;
		total_bw_read_kb = 0;
		total_bw_write_kb = 0;
		total_req_read = 0;
		total_req_write = 0;
	}

	read_lock_irqsave(&aipu_pmu.lock, flags);
	if (!aipu_pmu.enable) {
		read_unlock_irqrestore(&aipu_pmu.lock, flags);
		return HRTIMER_NORESTART;
	}
	read_unlock_irqrestore(&aipu_pmu.lock, flags);
	hrtimer_forward(&aipu_pmu.timer, now, ms_to_ktime(1));
	return HRTIMER_RESTART;
}

static ssize_t aipu_pmu_en_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 val = aipu_read32(&aipu_pmu.core->reg[0], AIPU_AXI_PMU_EN_OFFSET);
	return scnprintf(buf, PAGE_SIZE, "%u\n", val);
}

static ssize_t aipu_pmu_en_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long flags;
	int val;
	sscanf(buf, "%d", &val);

	write_lock_irqsave(&aipu_pmu.lock, flags);
	if (aipu_pmu.enable == val) {
		write_unlock_irqrestore(&aipu_pmu.lock, flags);
		return count;
	}
	aipu_pmu.enable = val;
	if (aipu_pmu.enable) {
		/* Set PMU period to 1ms */
		ktime_t expires = ms_to_ktime(1);
		aipu_write32(&aipu_pmu.core->reg[0], AIPU_AXI_PMU_PRD_OFFSET,
				aipu_pmu.clk_rate / 1000);
		aipu_write32(&aipu_pmu.core->reg[0], AIPU_AXI_PMU_EN_OFFSET, 0x1);
		hrtimer_start(&aipu_pmu.timer, expires, HRTIMER_MODE_REL);
	} else {
		aipu_write32(&aipu_pmu.core->reg[0], AIPU_AXI_PMU_EN_OFFSET, 0x0);
		hrtimer_cancel(&aipu_pmu.timer);
	}
	write_unlock_irqrestore(&aipu_pmu.lock, flags);
	return count;
}

/*
 * Usage:
 * Print PMU (Performance Monitoring Unit) registers statistic periodically:
 *	echo 1 > pmu_en
 * Disable the print:
 *	echo 0 > pmu_en
 */
static DEVICE_ATTR(pmu_en, 0644, aipu_pmu_en_show, aipu_pmu_en_store);

int aipu_init_pmu(struct device *dev, unsigned long clk_rate, struct aipu_core *core)
{
	aipu_pmu.enable = 0;
	aipu_pmu.clk_rate = clk_rate;
	aipu_pmu.core = core;
	rwlock_init(&aipu_pmu.lock);
	hrtimer_init(&aipu_pmu.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	aipu_pmu.timer.function = aipu_pmu_timer_function;
	device_create_file(dev, &dev_attr_pmu_en);
	return 0;
}

void aipu_deinit_pmu(struct device *dev)
{
	device_remove_file(dev, &dev_attr_pmu_en);
}
