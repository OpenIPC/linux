/*
 * Copyright (c) XMEDIA. All rights reserved.
 */
#ifndef __XMEDIA_CMA_H__
#define __XMEDIA_CMA_H__

#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
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
int is_cma_address(phys_addr_t phys, unsigned long size);
phys_addr_t xmedia_get_zones_start(void);
struct cma_zone *xmedia_get_cma_zone(const char *name);
struct device *xmedia_get_cma_device(const char *name);
int __init xmedia_declare_heap_memory(void);
#endif /* CONFIG_CMA */

#endif
