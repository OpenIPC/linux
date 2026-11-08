/*
 * The memory the ISP and the video encoder share. As on the other OpenIPC
 * SoCs it is whatever Linux does not get: the bootloader passes
 * "mem=${osmem}", and the DRAM above that, up to the end of the memory the
 * device tree declares (which ARM_ATAG_DTB_COMPAT replaces with the
 * bootloader's own), is video memory -- totalmem - osmem. So one device tree
 * serves every memory layout of a SoC.
 *
 * rmem=SIZE[@BASE] on the command line overrides that; without @BASE the
 * region starts where Linux memory ends.
 *
 * The region is taken out of Linux memory and handed to every video device
 * as one coherent DMA pool, so the capture and encoder drivers allocate from
 * the same bitmap.
 *
 * Licensed under GPLv2.
 */
#define pr_fmt(fmt) "rmem: " fmt

#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/memblock.h>
#include <linux/mutex.h>
#include <linux/of_fdt.h>
#include <mach-anyka/ak_rmem.h>

static phys_addr_t rmem_base;
static phys_addr_t rmem_size;
static bool rmem_has_base;

static struct dma_coherent_mem *rmem_pool;
static DEFINE_MUTEX(rmem_lock);

static int __init rmem_param(char *p)
{
	char *end;

	if (!p)
		return -EINVAL;
	rmem_size = memparse(p, &end);
	if (*end == '@') {
		rmem_base = memparse(end + 1, &end);
		rmem_has_base = true;
	}
	if (*end || !rmem_size || !PAGE_ALIGNED(rmem_size) ||
	    (rmem_has_base && !PAGE_ALIGNED(rmem_base))) {
		pr_err("ignoring malformed rmem=%s\n", p);
		rmem_size = 0;
		return -EINVAL;
	}
	return 0;
}
early_param("rmem", rmem_param);

/* The DRAM the device tree declares, before mem= trims what Linux uses. */
static phys_addr_t dram_start, dram_end;

static int __init rmem_scan_memory(unsigned long node, const char *uname,
				   int depth, void *data)
{
	const char *type = of_get_flat_dt_prop(node, "device_type", NULL);
	const __be32 *reg;
	int len;

	if (!type || strcmp(type, "memory"))
		return 0;
	reg = of_get_flat_dt_prop(node, "reg", &len);
	if (!reg || len < (dt_root_addr_cells + dt_root_size_cells) * 4)
		return 0;
	dram_start = dt_mem_next_cell(dt_root_addr_cells, &reg);
	dram_end = dram_start + dt_mem_next_cell(dt_root_size_cells, &reg);
	return 1;
}

/* machine_desc.reserve: after every early parameter, mem= included. */
void __init ak_rmem_reserve(void)
{
	of_scan_flat_dt(rmem_scan_memory, NULL);
	if (!rmem_has_base)
		rmem_base = memblock_end_of_DRAM();
	if (!rmem_size) {
		/* totalmem - osmem: the DRAM mem= left out. */
		if (dram_end <= rmem_base) {
			pr_info("no DRAM above mem=, no video memory\n");
			return;
		}
		rmem_size = dram_end - rmem_base;
	}

	if (dram_end && (rmem_base < dram_start ||
			 rmem_base + rmem_size > dram_end)) {
		pr_err("%pa + %lu MiB is outside DRAM %pa-%pa; no video memory\n",
		       &rmem_base, (unsigned long)(rmem_size >> 20),
		       &dram_start, &dram_end);
		rmem_size = 0;
		return;
	}

	memblock_remove(rmem_base, rmem_size);
	pr_info("%lu MiB at %pa for video\n",
		(unsigned long)(rmem_size >> 20), &rmem_base);
}

/**
 * ak_rmem_device_init() - give @dev the rmem= pool
 * @dev:  the device whose dma_alloc_coherent() should use the pool
 * @base: where the pool starts, for the caller's log
 * @size: the pool size, for the caller's own budgeting
 *
 * Every caller gets the same pool. Returns -ENODEV when the command line
 * has no rmem=, so a driver can fall back to a device tree memory-region.
 */
int ak_rmem_device_init(struct device *dev, phys_addr_t *base, size_t *size)
{
	int ret = 0;

	if (!rmem_size)
		return -ENODEV;

	mutex_lock(&rmem_lock);
	if (dev->dma_mem) {
		ret = -EBUSY;
	} else if (rmem_pool) {
		dev->dma_mem = rmem_pool;
	} else if (dma_declare_coherent_memory(dev, rmem_base, rmem_base,
					       rmem_size,
					       DMA_MEMORY_MAP |
					       DMA_MEMORY_EXCLUSIVE)) {
		rmem_pool = dev->dma_mem;
	} else {
		ret = -ENOMEM;
	}
	mutex_unlock(&rmem_lock);

	if (ret)
		return ret;
	if (base)
		*base = rmem_base;
	if (size)
		*size = rmem_size;
	return 0;
}
EXPORT_SYMBOL_GPL(ak_rmem_device_init);

/* The pool outlives every device: it is shared and never given back. */
void ak_rmem_device_release(struct device *dev)
{
	mutex_lock(&rmem_lock);
	if (rmem_pool && dev->dma_mem == rmem_pool)
		dev->dma_mem = NULL;
	mutex_unlock(&rmem_lock);
}
EXPORT_SYMBOL_GPL(ak_rmem_device_release);
