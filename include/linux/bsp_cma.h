/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */
#ifndef __BSP_CMA_H__
#define __BSP_CMA_H__

#ifdef CONFIG_ARCH_BSP
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dma-map-ops.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/cma.h>
#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>

#define NAME_LEN_MAX   64
#define ZONE_MAX       64

struct cma_zone {
	struct device pdev;
	char name[NAME_LEN_MAX];
	gfp_t gfp;
	phys_addr_t phys_start;
	phys_addr_t nbytes;
	u32 alloc_type;
	u32 block_align;
};

#ifdef CONFIG_CMA
#if defined(CONFIG_ARM)
#include <linux/pgtable.h>

/*
 * Re-map the kernel linear mapping of a CMA page range with the given cache
 * attributes and flush the corresponding TLB entries. Exported so
 * out-of-tree MMZ allocators (openhisilicon) do not have to reference the
 * non-exported init_mm / flush_tlb_kernel_range symbols directly.
 *
 * @page must be a lowmem page and @size must be non-zero and page-aligned
 * (__apply_to_page_range() walks page by page and would otherwise run past
 * the range). Returns 0 on success, -EINVAL on a bad argument.
 *
 * ARM32 caches are VIPT, so the linear map and the vmalloc mapping of the
 * same CMA page must carry identical attributes; AArch64 caches are PIPT and
 * its MMZ allocator does not re-map the linear alias at all.
 */
int bsp_cma_dma_remap(struct page *page, size_t size, pgprot_t prot);
#endif /* CONFIG_ARM */

int is_cma_address(phys_addr_t phys, unsigned long size);
phys_addr_t get_zones_start(void);
struct cma_zone *get_cma_zone(const char *name);
struct device *get_cma_device(const char *name);
int __init declare_heap_memory(void);
void __init bsp_memory_init(void);
#endif /* CONFIG_CMA */
#endif /* CONFIG_ARCH_BSP */

#endif
