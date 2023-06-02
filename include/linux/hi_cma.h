/*
 * hi_cma.h
 *
 * Copyright (c) 2019 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __HI_CMA_H__
#define __HI_CMA_H__

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
int is_hicma_address(phys_addr_t phys, unsigned long size);
phys_addr_t hisi_get_zones_start(void);
struct cma_zone *hisi_get_cma_zone(const char *name);
struct device *hisi_get_cma_device(const char *name);
int __init hisi_declare_heap_memory(void);
#endif /* CONFIG_CMA */

#endif
