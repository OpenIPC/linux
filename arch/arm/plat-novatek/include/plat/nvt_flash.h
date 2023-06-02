/*
	Structure definition for flash module

	Structure definition for flash module.

	@file       nvt_flash.h
	@ingroup
	@note       Nothing.

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/
#ifndef __ASM_ARCH_NA51089_NVT_FLASH_H
#define __ASM_ARCH_NA51089_NVT_FLASH_H

#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>
#include <linux/of.h>

/* macro to define partitions for flash devices */
#define DEFINE_PARTS(n, of, s)		\
{					\
	.name = n,			\
	.offset = of,			\
	.size = s,			\
}

/**
 * struct nvt_flash_info - platform structure for passing flash
 * information
 *
 * name: name of the serial nor flash for identification
 * mem_base: the memory base on which the flash is mapped
 * size: size of the flash in bytes
 * partitions: parition details
 * nr_partitions: number of partitions
 */

struct nvt_flash_info {
	char *name;
	unsigned long mem_base;
	unsigned long size;
	struct mtd_partition *partitions;
	int nr_partitions;
};

/**
 * struct nvt_flash_plat_data - platform structure for configuring smi
 *
 * clk_rate: clk rate at which NVT Flash must operate
 * board_flash_info: specific details of each flash present on board
 */
struct nvt_flash_plat_data {
	unsigned long clk_rate;
	struct nvt_flash_info *board_flash_info;
	struct device_node *np;
};

extern int nvt_check_preload_finish(void);
#endif /* __ASM_ARCH_NA51000_NVT_FLASH_H */
