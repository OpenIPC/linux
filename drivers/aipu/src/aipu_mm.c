// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file aipu_mm.c
 * Implementations of the AIPU memory management supports Address Space Extension (ASE)
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/of.h>
#include "config.h"
#include "aipu_priv.h"
#include "aipu_mm.h"

static struct dts_mem_region_parser mem_region[] = {
	{
		.name = "memory-region",
		.base = 0,
		.size = 0,
		.type = AIPU_MEM_TYPE_CMA,
		.must = 1,
	},
#if (defined AIPU_CONFIG_ENABLE_SRAM) && (AIPU_CONFIG_ENABLE_SRAM == 1)
	{
		.name = "sram-region",
		.base = 0,
		.size = 0,
		.type = AIPU_MEM_TYPE_SRAM,
		.must = 0,
	},
#endif
};

static inline void *aipu_remap_region_nocache(const struct aipu_memory_manager *mm, u64 base, u64 bytes)
{
	if ((!mm) || (!bytes))
		return NULL;

	return memremap(base, bytes, MEMREMAP_WT);
}

static inline void aipu_unmap_region_nocache(void *va)
{
	if (va)
		memunmap(va);
}

static void *aipu_alloc_cma_region_nocache(const struct aipu_memory_manager *mm, u64 *base, u64 bytes)
{
	int ret = 0;
	void *va = NULL;

	if ((!mm) || (!bytes))
		return va;

#if 0
	ret = of_reserved_mem_device_init_by_idx(mm->dev, mm->dev->of_node, mm->ddr_cnt);
	if (ret) {
		dev_err(mm->dev, "init reserved mem failed: idx %d, ret %d\n",
			mm->ddr_cnt, ret);
		goto finish;
	}
#endif

	ret = dma_set_coherent_mask(mm->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(mm->dev, "DMA set coherent mask failed!\n");
		goto finish;
	}

	va = dma_alloc_coherent(mm->dev, bytes, (dma_addr_t *)base, GFP_KERNEL);

finish:
	return va;
}

static void aipu_free_cma_region_nocache(const struct aipu_memory_manager *mm, struct aipu_mem_region *region)
{
	if ((!mm) || (!region))
		return;

	dma_free_coherent(mm->dev, region->tot_bytes, region->va, region->pa);
#if 0
	of_reserved_mem_device_release(mm->dev);
#endif
}

static struct aipu_block *create_block(u64 base, u64 bytes, int tid, enum aipu_mm_data_type type,
	enum aipu_blk_state state)
{
	struct aipu_block *blk = NULL;

	blk = kzalloc(sizeof(*blk), GFP_KERNEL);
	if (!blk)
		return blk;

	blk->pa = base;
	blk->bytes = bytes;
	blk->tid = tid;
	blk->type = type;
	blk->state = state;
	INIT_LIST_HEAD(&blk->list);

	return blk;
}

static inline struct aipu_block *create_block_list_head(u64 base, u64 bytes)
{
	return create_block(base, bytes, 0, AIPU_MM_DATA_TYPE_NONE, AIPU_BLOCK_STATE_FREE);
}

static int aipu_mm_find_block_candidate_no_lock(const struct aipu_block *head, u64 bytes, u64 alignment,
	struct aipu_block **found, u64 *pa)
{
	struct aipu_block *blk_cand = NULL;
	u64 start = 0;
	u64 end = 0;
	int ret = -ENOMEM;

	if (!found)
		return -EINVAL;

	if ((!head) || (!bytes) || (!alignment) || (alignment % PAGE_SIZE)) {
		ret = -EINVAL;
		goto failed;
	}

	list_for_each_entry(blk_cand, &head->list, list) {
		if (blk_cand->state != AIPU_BLOCK_STATE_ALLOCATED) {
			start = ALIGN(blk_cand->pa, alignment);
			end = start + bytes;
			if (end <= (blk_cand->pa + blk_cand->bytes))
				goto success;
		}
	}

failed:
	*found = NULL;
	*pa = 0;
	return ret;

success:
	*found = blk_cand;
	*pa = start;
	return 0;
}

static int aipu_mm_split_block_no_lock(struct aipu_block *target, u64 alloc_base, u64 alloc_bytes,
	enum aipu_mm_data_type type, struct file *filp)
{
	u64 alloc_start = alloc_base;
	u64 alloc_end = alloc_start + alloc_bytes - 1;
	u64 target_start = target->pa;
	u64 target_end = target->pa + target->bytes - 1;
	struct aipu_block *alloc_blk = NULL;
	struct aipu_block *remaining_blk = target;

	if ((!target) || (!alloc_bytes) || (alloc_end < target_start) ||
	    (alloc_end > target_end) || (!filp))
		return -EINVAL;

	if ((alloc_start == target_start) && (alloc_end == target_end)) {
		/*
		 * alloc block:              |<-----------alloc------------>|
		 * equals to
		 * target block to be split: |<----------target------------>|
		 */
		alloc_blk = target;
		alloc_blk->tid = task_pid_nr(current);
		alloc_blk->type = type;
		alloc_blk->state = AIPU_BLOCK_STATE_ALLOCATED;
	} else {
		alloc_blk = create_block(alloc_start, alloc_bytes, task_pid_nr(current),
			type, AIPU_BLOCK_STATE_ALLOCATED);
		if (!alloc_blk)
			return -ENOMEM;
		if ((alloc_start == target_start) && (alloc_end < target_end)) {
			/*
			 * alloc block:              |<---alloc--->|<--remaining-->|
			 * smaller than and start from base of
			 * target block to be split: |<----------target----------->|
			 */
			remaining_blk->pa += alloc_blk->bytes;
			remaining_blk->bytes -= alloc_blk->bytes;
			list_add_tail(&alloc_blk->list, &remaining_blk->list);
		} else if ((alloc_start > target_start) && (alloc_end == target_end)) {
			/*
			 * alloc block:              |<--remaining-->|<---alloc--->|
			 * smaller than and end at end of
			 * target block to be split: |<----------target----------->|
			 */
			remaining_blk->bytes -= alloc_blk->bytes;
			list_add(&alloc_blk->list, &remaining_blk->list);
		} else {
			/*
			 * alloc block:              |<-fr_remaining->|<--alloc-->|<-bk_remaining->|
			 * insides of
			 * target block to be split: |<-------------------target------------------>|
			 */
			/* front remaining */
			remaining_blk->bytes = alloc_start - remaining_blk->pa;
			list_add(&alloc_blk->list, &remaining_blk->list);
			/* back remaining */
			remaining_blk = create_block(alloc_end + 1, target_end - alloc_end,
				task_pid_nr(current), type, AIPU_BLOCK_STATE_FREE);
			list_add(&remaining_blk->list, &alloc_blk->list);
		}
	}

	/* success */
	alloc_blk->filp = filp;
	alloc_blk->tid = task_pid_nr(current);
	return 0;
}

static int aipu_mm_alloc_in_region_compact_no_lock(struct aipu_memory_manager *mm, struct aipu_mem_region *region,
	struct aipu_buf_request *buf_req, struct aipu_buffer *buf, struct file *filp)
{
	int ret = 0;
	u64 compact_bytes = 0;
	u64 alignment = 0;
	u64 alloc_pa = 0;
	struct aipu_block *blk_cand = NULL;

	if ((!region) || (!buf_req) || (!buf))
		return -EINVAL;

	compact_bytes = ALIGN(buf_req->bytes, PAGE_SIZE);
	alignment = buf_req->align_in_page * 4 * 1024;
	ret = aipu_mm_find_block_candidate_no_lock(region->blk_head,
		compact_bytes, alignment, &blk_cand, &alloc_pa);
	if (ret)
		goto finish;

	/* found matching block candidate: update block list */
	if (aipu_mm_split_block_no_lock(blk_cand, alloc_pa, compact_bytes,
	    (enum aipu_mm_data_type)buf_req->data_type, filp)) {
		ret = -ENOMEM;
		goto finish;
	}

	/* success */
	buf->pa = alloc_pa;
	buf->va = (void *)((unsigned long)region->va + alloc_pa - region->pa);
	buf->bytes = compact_bytes;

finish:
	return ret;
}

static int aipu_init_region(int id, struct aipu_memory_manager *mm, u64 base, u64 bytes,
	enum aipu_mem_type type, struct aipu_mem_region *region)
{
	struct aipu_block *new_blk = NULL;

	if ((!mm) || (!bytes) || (!region))
		return -EINVAL;

	region->id = id;

	region->blk_head = create_block_list_head(0, 0);
	new_blk = create_block_list_head(base, bytes);
	list_add(&new_blk->list, &region->blk_head->list);

	region->pa = base;
	region->tot_bytes = bytes;
	region->tot_free_bytes = bytes;
	region->type = type;

	INIT_LIST_HEAD(&region->list);

	return 0;
}

static int aipu_update_mm_regions(struct aipu_memory_manager *mm, struct aipu_mem_region *head,
	int *region_cnt, struct aipu_mem_region *new_region)
{
	if ((!head) || (!region_cnt) || (!new_region))
		return -EINVAL;

	list_add(&new_region->list, &head->list);
	(*region_cnt)++;

	return 0;
}

static struct aipu_mem_region *create_region_list_head(struct aipu_memory_manager *mm)
{
	struct aipu_mem_region *region = NULL;

	region = devm_kzalloc(mm->dev, sizeof(*region), GFP_KERNEL);
	if (!region)
		return region;

	INIT_LIST_HEAD(&region->list);
	return region;
}

static int aipu_mm_free_in_region_no_lock(struct aipu_mem_region *region, struct aipu_buf_desc *buf)
{
	int ret = 0;
	int found = 0;
	struct aipu_block *target = NULL;
	struct aipu_block *prev = NULL;
	struct aipu_block *next = NULL;

	if ((!region) || (!buf))
		return -EINVAL;

	list_for_each_entry(target, &region->blk_head->list, list) {
		if ((target->pa == buf->pa) && (target->bytes == buf->bytes) &&
		    (target->state == AIPU_BLOCK_STATE_ALLOCATED)) {
			found = 1;
			break;
		}
	}

	if (!found) {
		ret = -EINVAL;
		goto finish;
	}

	/* update target block to be free state */
	target->tid = 0;
	target->type = AIPU_MM_DATA_TYPE_NONE;
	target->state = AIPU_BLOCK_STATE_FREE;
	target->filp = NULL;
	target->map_num = 0;

	/*
	 *   merge prev block and next block if they are free/aligned
	 *
	 *   block list: ... <=> |<--prev-->| <=> |<--target-->| <=> |<--next-->| <=> ...
	 *			    free              free           free/aligned
	 *
	 *   block list: ... <=> |<------------merged new block--------------->| <=> ...
	 *					    free
	 */
	prev = list_prev_entry(target, list);
	next = list_next_entry(target, list);

	if ((prev->bytes != 0) && (prev->state == AIPU_BLOCK_STATE_FREE)) {
		prev->bytes += target->bytes;
		list_del(&target->list);
		kfree(target);
		target = prev;
	}

	if ((next->bytes != 0) && (next->state != AIPU_BLOCK_STATE_ALLOCATED)) {
		target->bytes += next->bytes;
		list_del(&next->list);
		kfree(next);
		next = NULL;
	}

	region->tot_free_bytes += buf->bytes;

finish:
	return ret;
}

static int aipu_mm_scan_regions_alloc_no_lock(struct aipu_memory_manager *mm, struct aipu_mem_region *head,
	struct aipu_buf_request *buf_req, struct aipu_buffer *buf, struct file *filp)
{
	int ret = -ENOMEM;
	struct aipu_mem_region *region = NULL;

	if ((!mm) || (!head) || (!buf_req) || (!buf))
		return -EINVAL;

	list_for_each_entry(region, &head->list, list) {
		if (region->tot_free_bytes >= ALIGN(buf_req->bytes, PAGE_SIZE)) {
			ret = aipu_mm_alloc_in_region_compact_no_lock(mm, region, buf_req, buf, filp);
			if (!ret) {
				region->tot_free_bytes -= buf->bytes;
				buf->type = region->type;
				break;
			}
		}
	}

	/* success */
	if (!ret) {
		buf_req->desc.pa = buf->pa;
		buf_req->desc.dev_offset = buf->pa;
		buf_req->desc.bytes = buf->bytes;
	}

	return ret;
}

static struct aipu_mem_region *aipu_mm_find_region_no_lock(struct aipu_mem_region *head, u64 pa, u64 bytes)
{
	struct aipu_mem_region *region = NULL;

	if ((!head) || (!bytes))
		return region;

	list_for_each_entry(region, &head->list, list) {
		if ((pa >= region->pa) &&
		    ((pa + bytes) <= (region->pa + region->tot_bytes)))
			return region;
	}

	return NULL;
}

static int aipu_mm_deinit_region(struct aipu_memory_manager *mm, struct aipu_mem_region *region)
{
	struct aipu_block *prev = NULL;
	struct aipu_block *next = NULL;

	if (!region)
		return -EINVAL;

	list_for_each_entry_safe(prev, next, &region->blk_head->list, list) {
		kfree(prev);
		prev = NULL;
	}
	kfree(region->blk_head);
	region->blk_head = NULL;

	if (region->type == AIPU_MEM_TYPE_SRAM)
		aipu_unmap_region_nocache(region->va);
	else if (region->type == AIPU_MEM_TYPE_CMA)
		aipu_free_cma_region_nocache(mm, region);
	else if (region->type == AIPU_MEM_TYPE_RESERVED)
		aipu_unmap_region_nocache(region->va);

	region->pa = 0;
	region->va = NULL;
	region->tot_bytes = 0;
	region->tot_free_bytes = 0;

	return 0;
}

static int aipu_mm_add_region(struct aipu_memory_manager *mm, u64 base, u64 bytes,
	enum aipu_mem_type type)
{
	int ret = 0;
	int region_id = 0;
	struct aipu_mem_region *region = NULL;

	if ((!mm) || (!bytes))
		return -EINVAL;

	region = devm_kzalloc(mm->dev, sizeof(*region), GFP_KERNEL);
	if (!region)
		return -ENOMEM;

	if (type == AIPU_MEM_TYPE_SRAM)
		region->va = aipu_remap_region_nocache(mm, base, bytes);
	else if (type == AIPU_MEM_TYPE_CMA)
		region->va = aipu_alloc_cma_region_nocache(mm, &base, bytes);
	else if (type == AIPU_MEM_TYPE_RESERVED)
		region->va = aipu_remap_region_nocache(mm, base, bytes);

	if (!region->va)
		return -ENOMEM;

	region_id = mm->sram_cnt + mm->ddr_cnt;
	ret = aipu_init_region(region_id, mm, base, bytes, type, region);
	if (ret)
		return ret;

	if (type == AIPU_MEM_TYPE_SRAM)
		ret = aipu_update_mm_regions(mm, mm->sram_head, &mm->sram_cnt, region);
	else
		ret = aipu_update_mm_regions(mm, mm->ddr_head, &mm->ddr_cnt, region);
	if (ret)
		return ret;

	return 0;
}

int aipu_init_mm(struct aipu_memory_manager *mm, struct platform_device *p_dev)
{
	int ret = 0;
	int iter = 0;
#if 0
	struct device_node *np = NULL;
#else
	int cma_reserve_size = 0;
#endif
	struct resource res;
	u64 sram_pa = 0;
	u64 ddr_pa = 0;

	if ((!mm) || (!p_dev))
		return -EINVAL;

	memset(mm, 0, sizeof(*mm));
	mm->dev = &p_dev->dev;
	mm->sram_head = create_region_list_head(mm);
	mm->sram_cnt = 0;
	mm->ddr_head = create_region_list_head(mm);
	mm->ddr_cnt = 0;
	mutex_init(&mm->region_lock);
	mm->smmu_flag = AIPU_SMMU_FLAG_NO_SMMU;
	mm->sram_dft_dtype = AIPU_MM_DATA_TYPE_NONE;

	mm->sram_disable_head = kzalloc(sizeof(*mm->sram_disable_head), GFP_KERNEL);
	if (!mm->sram_disable_head)
		return -ENOMEM;
	INIT_LIST_HEAD(&mm->sram_disable_head->list);

	if (of_property_read_u64(mm->dev->of_node, "host-aipu-offset", &mm->host_aipu_offset))
		mm->host_aipu_offset = 0;

#if 0
	/**
	 * Though the regions are stored via linked lists,
	 * currently we only support 1 DDR region and/or 1 SoC SRAM region.
	 */
	for (iter = 0; iter < ARRAY_SIZE(mem_region); iter++) {
		np = of_parse_phandle(mm->dev->of_node, mem_region[iter].name, 0);
		if (!np) {
			if (mem_region[iter].must) {
				dev_err(mm->dev, "no %s specified\n", mem_region[iter].name);
				return -ENOMEM;
			}

			dev_info(mm->dev, "no %s specified\n", mem_region[iter].name);
			break;
		}
		if (of_address_to_resource(np, 0, &res)) {
			of_node_put(np);
			return -EINVAL;
		}
		dev_dbg(mm->dev, "get mem region: [0x%llx, 0x%llx]\n", res.start, res.end);

		if (mem_region[iter].type == AIPU_MEM_TYPE_CMA)
			ddr_pa = res.start;
		else
			sram_pa = res.start;

		ret = aipu_mm_add_region(mm, res.start, res.end - res.start + 1,
		    mem_region[iter].type);
		if (ret) {
			dev_err(mm->dev, "add new region failed\n");
			of_node_put(np);
			return -EINVAL;
		}
		of_node_put(np);
	}
#else
	ret = of_property_read_u32(mm->dev->of_node, "cma-reserved-bytes", &cma_reserve_size);
	if (ret) {
		dev_err(mm->dev, "get cma reserved size property failed!");
		return -ENOMEM;
	}

	ret = aipu_mm_add_region(mm, res.start, cma_reserve_size, AIPU_MEM_TYPE_CMA);
	if (ret) {
		dev_err(mm->dev, "add new region failed\n");
		return -EINVAL;
	}
#endif

	/* We only support SRAM & DDR regions in the same 4GB address sapce if there is no SMMU. */
	if (mm->sram_cnt != 0) {
		if (((sram_pa >> 32) != (ddr_pa >> 32)) &&
		    (mm->smmu_flag == AIPU_SMMU_FLAG_NO_SMMU))
			dev_info(mm->dev, "SoC SRAM region is not accepted\n");
		else
			mm->sram_dft_dtype = AIPU_CONFIG_DEFAULT_SRAM_DATA_TYPE;
	}

	/* success */
	return 0;
}

void aipu_deinit_mm(struct aipu_memory_manager *mm)
{
	struct aipu_mem_region *region = NULL;

	if (!mm)
		return;

	if (mm->sram_head) {
		list_for_each_entry(region, &mm->sram_head->list, list) {
			aipu_mm_deinit_region(mm, region);
		}
	}

	if (mm->ddr_head) {
		list_for_each_entry(region, &mm->ddr_head->list, list) {
			aipu_mm_deinit_region(mm, region);
		}
	}
	mutex_destroy(&mm->region_lock);

	kfree(mm->sram_disable_head);
	memset(mm, 0, sizeof(*mm));
}

int aipu_mm_alloc(struct aipu_memory_manager *mm, struct aipu_buf_request *buf_req,
	struct aipu_buffer *buf, struct file *filp)
{
	int ret = 0;

	if ((!mm) || (!buf_req) || (!buf))
		return -EINVAL;

	if ((!buf_req->bytes) || (!buf_req->align_in_page))
		return -EINVAL;

	memset(buf, 0, sizeof(*buf));

	if ((!mm->sram_cnt) && (!mm->ddr_cnt))
		return -ENOMEM;

	mutex_lock(&mm->region_lock);
#if (defined AIPU_CONFIG_ENABLE_SRAM) && (AIPU_CONFIG_ENABLE_SRAM == 1)
	/**
	 * Try to allocate from SRAM first if the data types are matched and
	 * also SRAM is in enable state.
	 */
	if ((mm->sram_dft_dtype == buf_req->data_type) && !mm->sram_disable) {
		ret = aipu_mm_scan_regions_alloc_no_lock(mm, mm->sram_head, buf_req, buf, filp);
		if ((ret && (AIPU_CONFIG_ENABLE_FALL_BACK_TO_DDR == 0)) || (!ret))
			goto unlock;
	}
#endif

	ret = aipu_mm_scan_regions_alloc_no_lock(mm, mm->ddr_head, buf_req, buf, filp);
	if (ret) {
		dev_err(mm->dev, "[MM] buffer allocation failed for: bytes 0x%llx, page align %d\n",
			buf_req->bytes, buf_req->align_in_page);
		goto unlock;
	}

unlock:
	mutex_unlock(&mm->region_lock);
	return ret;
}

int aipu_mm_free(struct aipu_memory_manager *mm, struct aipu_buf_desc *buf)
{
	int ret = 0;
	struct aipu_mem_region *region = NULL;

	if ((!mm) || (!buf))
		return -EINVAL;

	mutex_lock(&mm->region_lock);
	region = aipu_mm_find_region_no_lock(mm->sram_head, buf->pa, buf->bytes);
	if (!region) {
		region = aipu_mm_find_region_no_lock(mm->ddr_head, buf->pa, buf->bytes);
		if (!region) {
			dev_err(mm->dev, "[MM] buffer to free not exists in any region: pa 0x%llx, bytes 0x%llx\n",
				buf->pa, buf->bytes);
			ret = -EINVAL;
			goto unlock;
		}
	}

	ret = aipu_mm_free_in_region_no_lock(region, buf);
	if (ret) {
		dev_err(mm->dev, "[MM] buffer to free not exists in target region: pa 0x%llx, bytes 0x%llx\n",
			buf->pa, buf->bytes);
	}
unlock:
	mutex_unlock(&mm->region_lock);
	return ret;
}

static void free_buffers_in_region_no_lock(struct aipu_mem_region *region, struct file *filp)
{
	struct aipu_block *curr = NULL;
	struct aipu_buf_desc buf;
	int found = 1;

	if ((!region) || (!filp))
		return;

	while (found) {
		found = 0;
		list_for_each_entry(curr, &region->blk_head->list, list) {

			if (curr->filp == filp) {
				buf.pa = curr->pa;
				buf.bytes = curr->bytes;
				aipu_mm_free_in_region_no_lock(region, &buf);
				found = 1;
				break;
			}
		}
	}
}

static void aipu_mm_enable_sram_allocations_no_lock(struct aipu_memory_manager *mm, struct file *filp)
{
	struct aipu_sram_disable_per_fd *curr = NULL;
	struct aipu_sram_disable_per_fd *next = NULL;

	if ((!mm) || (!filp))
		return;

	list_for_each_entry_safe(curr, next, &mm->sram_disable_head->list, list) {
		if (curr->filp == filp) {
			mm->sram_disable -= curr->cnt;
			list_del(&curr->list);
			kfree(curr);
			break;
		};
	}
}

void aipu_mm_free_buffers(struct aipu_memory_manager *mm, struct file *filp)
{
	struct aipu_mem_region *curr = NULL;
	struct aipu_mem_region *next = NULL;

	if ((!mm) || (!filp))
		return;

	mutex_lock(&mm->region_lock);
	list_for_each_entry_safe(curr, next, &mm->sram_head->list, list) {
		free_buffers_in_region_no_lock(curr, filp);
	}

	list_for_each_entry_safe(curr, next, &mm->ddr_head->list, list) {
		free_buffers_in_region_no_lock(curr, filp);
	}

	aipu_mm_enable_sram_allocations_no_lock(mm, filp);

	mutex_unlock(&mm->region_lock);
}

static struct aipu_block *find_block_in_region_by_pa_no_lock(struct aipu_mem_region *head, u64 pa,
	void **va, enum aipu_mem_type *type)
{
	struct aipu_mem_region *region = NULL;
	struct aipu_block *curr = NULL;
	struct aipu_block *blk = NULL;

	if ((!head) || (!va) || (!type))
		return NULL;

	list_for_each_entry(region, &head->list, list) {
		list_for_each_entry(curr, &region->blk_head->list, list) {
			if ((curr->tid == task_pid_nr(current)) &&
			    (curr->pa == pa)) {
				blk = curr;
				break;
			}
		}
		if (blk)
			break;
	}

	if (blk) {
		*va = (void *)((unsigned long)region->va + pa - region->pa);
		*type = region->type;
	} else
		*va = NULL;

	return blk;
}

int aipu_mm_mmap_buf(struct aipu_memory_manager *mm, struct vm_area_struct *vma, struct device *dev)
{
	int ret = 0;
	u64 offset = 0;
	int len = 0;
	unsigned long vm_pgoff = 0;
	struct aipu_block *blk = NULL;
	void *kern = NULL;
	enum aipu_mem_type type;

	if ((!mm) || (!vma))
		return -EINVAL;

	offset = vma->vm_pgoff * PAGE_SIZE;
	len = vma->vm_end - vma->vm_start;

	mutex_lock(&mm->region_lock);
	blk = find_block_in_region_by_pa_no_lock(mm->ddr_head, offset, &kern, &type);
	if (!blk)
		blk = find_block_in_region_by_pa_no_lock(mm->sram_head, offset, &kern, &type);
	mutex_unlock(&mm->region_lock);

	if (!blk)
		return -EINVAL;

	if (blk->map_num)
		return -EINVAL;

	vm_pgoff = vma->vm_pgoff;
	vma->vm_pgoff = 0;
	vma->vm_flags |= VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (type == AIPU_MEM_TYPE_CMA) {
		ret = dma_mmap_coherent(dev, vma, kern, (dma_addr_t)blk->pa, blk->bytes);
	} else if ((type == AIPU_MEM_TYPE_SRAM) || (type == AIPU_MEM_TYPE_RESERVED)) {
		ret = remap_pfn_range(vma, vma->vm_start, blk->pa >> PAGE_SHIFT,
		    vma->vm_end - vma->vm_start, vma->vm_page_prot);
	}

	vma->vm_pgoff = vm_pgoff;
	if (!ret)
		blk->map_num++;

	return ret;
}

int aipu_mm_disable_sram_allocation(struct aipu_memory_manager *mm, struct file *filp)
{
	int ret = 0;
	struct aipu_mem_region *region = NULL;
	struct aipu_sram_disable_per_fd *sram_disable_per_fd = NULL;

	if (!mm)
		return -EINVAL;

	/* If there is no SRAM in this system, it cannot be disabled. */
	if (!mm->sram_cnt)
		return -EPERM;

	mutex_lock(&mm->region_lock);
	/* If SRAM is under using by driver & AIPU, it cannot be disabled. */
	list_for_each_entry(region, &mm->sram_head->list, list) {
		if (region->tot_free_bytes != region->tot_bytes)
			ret = -EPERM;
	}
	if (!ret) {
		int found = 0;

		list_for_each_entry(sram_disable_per_fd, &mm->sram_disable_head->list, list) {
			if (sram_disable_per_fd->filp == filp) {
				sram_disable_per_fd->cnt++;
				found = 1;
				break;
			}
		}
		if (!found) {
			sram_disable_per_fd = kzalloc(sizeof(*sram_disable_per_fd), GFP_KERNEL);
			if (!sram_disable_per_fd) {
				ret = -ENOMEM;
				goto unlock;
			}
			sram_disable_per_fd->cnt++;
			sram_disable_per_fd->filp = filp;
			list_add(&sram_disable_per_fd->list, &mm->sram_disable_head->list);
		}
		mm->sram_disable++;
	}
unlock:
	mutex_unlock(&mm->region_lock);
	return ret;
}

int aipu_mm_enable_sram_allocation(struct aipu_memory_manager *mm, struct file *filp)
{
	int ret = 0;
	struct aipu_sram_disable_per_fd *sram_disable_per_fd = NULL;

	if (!mm)
		return -EINVAL;

	if (!mm->sram_cnt)
		return -EPERM;

	mutex_lock(&mm->region_lock);
	if (mm->sram_disable == 0) {
		ret = -EPERM;
		goto unlock;
	}

	list_for_each_entry(sram_disable_per_fd, &mm->sram_disable_head->list, list) {
		if (sram_disable_per_fd->filp == filp) {
			if (sram_disable_per_fd->cnt)
				sram_disable_per_fd->cnt--;
			break;
		}
	}
	mm->sram_disable--;
unlock:
	mutex_unlock(&mm->region_lock);
	return ret;
}
