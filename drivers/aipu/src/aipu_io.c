// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file aipu_io.c
 * Implementations of AIPU IO R/W API
 */

#include "aipu_io.h"

int init_aipu_ioregion(struct device *dev, struct io_region *region, u64 phys_base, u32 size)
{
	int ret = 0;

	if ((!size) || (!dev) || (!region)) {
		dev_dbg(dev, "invalid input args size/dev/region!");
		goto fail;
	}

	if (!request_mem_region(phys_base, size, "aipu")) {
		dev_err(dev, "request IO region failed");
		goto fail;
	}

	region->kern = ioremap(phys_base, size);
	if (!region->kern) {
		dev_err(dev, "ioremap failed");
		goto fail;
	}

	region->phys = phys_base;
	region->size = size;

	/* success */
	goto finish;

fail:
	dev_err(dev, "init IO region [0x%llx, 0x%llx] failed",
		phys_base, phys_base + size - 1);
	ret = -EINVAL;

finish:
	return ret;
}

void deinit_aipu_ioregion(struct io_region *region)
{
	if (region && region->kern) {
		iounmap(region->kern);
		release_mem_region(region->phys, region->size);
		region->kern = NULL;
		region->phys = 0;
		region->size = 0;
	}
}

u32 aipu_read32(struct io_region *region, int offset)
{
	if (region && region->kern && (offset < region->size))
		return readl((void __iomem *)((unsigned long)(region->kern) + offset));
	return -EINVAL;
}

void aipu_write32(struct io_region *region, int offset, unsigned int data)
{
	if (region && region->kern && (offset < region->size))
		return writel((u32)data, (void __iomem *)((unsigned long)(region->kern) + offset));
}
