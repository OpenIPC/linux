/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file aipu_mm.h
 * Header of the AIPU memory management supports Address Space Extension (ASE)
 */

#ifndef __AIPU_MM_H__
#define __AIPU_MM_H__

#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include "armchina_aipu.h"

struct aipu_buffer {
	u64 pa;
	void *va;
	u64 bytes;
	u32 type;
};

enum aipu_blk_state {
	AIPU_BLOCK_STATE_FREE,
	AIPU_BLOCK_STATE_ALLOCATED,
};

enum aipu_mem_type {
	AIPU_MEM_TYPE_SRAM,
	AIPU_MEM_TYPE_CMA,
	AIPU_MEM_TYPE_RESERVED,
};

enum aipu_smmu_flag {
	AIPU_SMMU_FLAG_NO_SMMU,
};

struct dts_mem_region_parser {
	char *name;
	u64 base;
	u64 size;
	enum aipu_mem_type type;
	int must;
};

struct aipu_block {
	u64 pa;
	u64 bytes;
	enum aipu_mm_data_type type;
	enum aipu_blk_state state;
	struct list_head list;
	int tid;
	struct file *filp;
	int map_num;
};

struct aipu_mem_region {
	int id;
	struct aipu_block *blk_head;
	u64 pa;
	void *va;
	u64 tot_bytes;
	u64 tot_free_bytes;
	int flag;
	enum aipu_mem_type type;
	struct list_head list;
};

struct aipu_sram_disable_per_fd {
	int cnt;
	struct file *filp;
	struct list_head list;
};

struct aipu_memory_manager {
	u64 host_aipu_offset;
	struct device *dev;
	struct aipu_mem_region *sram_head;
	int sram_cnt;
	struct aipu_mem_region *ddr_head;
	int ddr_cnt;
	struct mutex region_lock;
	int smmu_flag;
	int sram_dft_dtype;
	int sram_disable;
	struct aipu_sram_disable_per_fd *sram_disable_head;
};

/**
 * @brief initialize mm module during driver probe phase
 *
 * @param mm: memory manager struct allocated by user
 * @param p_dev: platform device struct pointer
 *
 * @return 0 if successful; others if failed.
 */
int aipu_init_mm(struct aipu_memory_manager *mm, struct platform_device *p_dev);
/**
 * @brief alloc memory buffer for user request
 *
 * @param mm: memory manager struct allocated by user
 * @param buf_req:  buffer request struct from userland
 * @param buf: successfully allocated buffer descriptor
 * @param filp: file struct pointer
 *
 * @return 0 if successful; others if failed.
 */
int aipu_mm_alloc(struct aipu_memory_manager *mm, struct aipu_buf_request *buf_req,
	struct aipu_buffer *buf, struct file *filp);
/**
 * @brief free buffer allocated by aipu_mm_alloc
 *
 * @param mm: memory manager struct allocated by user
 * @param buf: buffer descriptor to be released
 *
 * @return 0 if successful; others if failed.
 */
int aipu_mm_free(struct aipu_memory_manager *mm, struct aipu_buf_desc *buf);
/**
 * @brief free all the buffers allocated from one fd (filp)
 *
 * @param mm: mm struct pointer
 * @param filp: file struct pointer
 *
 * @return void
 */
void aipu_mm_free_buffers(struct aipu_memory_manager *mm, struct file *filp);
/**
 * @brief mmap an allocated buffer for user thread
 *
 * @param mm: mm struct pointer
 * @param vma: vm_area_struct
 * @param dev: device struct
 *
 * @return 0 if successful; others if failed.
 */
int aipu_mm_mmap_buf(struct aipu_memory_manager *mm, struct vm_area_struct *vma,
	struct device *dev);
/**
 * @brief disable buffer allocations from soc sram
 *
 * @param mm: mm struct pointer
 * @param filp: file struct pointer
 *
 * @return 0 if successful; others if failed.
 */
int aipu_mm_disable_sram_allocation(struct aipu_memory_manager *mm, struct file *filp);
/**
 * @brief enable buffer allocations from soc sram (disabled before)
 *
 * @param mm: mm struct pointer
 * @param filp: file struct pointer
 *
 * @return 0 if successful; others if failed.
 */
int aipu_mm_enable_sram_allocation(struct aipu_memory_manager *mm, struct file *filp);
/**
 * @brief de-initialize mm module while kernel module unloading
 *
 * @param mm: memory manager struct allocated by user
 *
 * @return void
 */
void aipu_deinit_mm(struct aipu_memory_manager *mm);

#endif /* __AIPU_MM_H__ */
