/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file aipu_io.h
 * AIPU IO R/W API header file
 */

#ifndef __AIPU_IO_H__
#define __AIPU_IO_H__

#include <linux/device.h>
#include <asm/io.h>
#include <asm/types.h>

/**
 * struct io_region - a general struct describe IO region
 *
 * @phys: physical address base of an IO region
 * @kern: kernel virtual address base remapped from phys
 * @size: size of an IO region in byte
 */
struct io_region {
	u64  phys;
	void *kern;
	u32  size;
};

/**
 * @brief create AIPU IO region using physical base address
 *
 * @param dev: device pointer
 * @param region: core IO region pointer allocated by calling function
 * @param phys_base: base address
 * @param size: region size
 *
 * @return 0 if successful; others if failed;
 */
int init_aipu_ioregion(struct device *dev, struct io_region *region, u64 phys_base, u32 size);
/**
 * @brief destroy an AIPU IO region
 *
 * @param region: region pointer
 */
void deinit_aipu_ioregion(struct io_region *region);
/**
 * @brief read AIPU register in word (with memory barrier)
 *
 * @param region: IO region providing the base address
 * @param offset: AIPU register offset
 * @return register value
 */
u32 aipu_read32(struct io_region *region, int offset);
/**
 * @brief write AIPU register in word (with memory barrier)
 *
 * @param region: IO region providing the base address
 * @param offset: AIPU register offset
 * @param data:   data to be writen
 * @return void
 */
void aipu_write32(struct io_region *region, int offset, unsigned int data);

#endif /* __AIPU_IO_H__ */
