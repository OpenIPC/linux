/**
    NVT memory operation handling
    To handle the flush allocate and other memory handling api header file
    @file       fmem.h
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_NVT_IVOT_FMEM_H
#define __ASM_ARCH_NVT_IVOT_FMEM_H

#include <linux/mm.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <linux/list.h>
#include <linux/dma-direction.h>

#define NVT_FMEM_VERSION	"0.0.1"

struct nvt_cma_info_t {
	struct list_head	list;			// List for cma area
	struct cma			*cma;
	const char			*name;
	struct device		*dev;
	phys_addr_t			start;
	unsigned int		size;
	unsigned int		id;				// Which cma area
	struct kmem_cache *	fmem_mem_cache;
	struct list_head	fmem_list_root;	// List for allocated fmem info
};

typedef enum {
	NVT_FMEM_ALLOC_CACHE	= 0x1,
	NVT_FMEM_ALLOC_NONCACHE,
	NVT_FMEM_ALLOC_BUFFER,
	NVT_FMEM_ALLOC_MAX = 999,
} nvt_fmem_alloc_type_t;

struct nvt_fmem_mem_info_t {
	nvt_fmem_alloc_type_t	type;
	struct device			*dev;
	phys_addr_t				paddr;
	struct page 			*pages;
	unsigned int			page_count;
	void					*vaddr;
	size_t					size;
};

#define NVT_FMEM_MEM_MAGIC		0x55AA55AA
struct nvt_fmem_mem_cma_info_t {
	struct list_head		list;
	unsigned long			magic_num;
	struct nvt_fmem_mem_info_t mem_info;
};

static inline int nvt_fmem_mem_info_init(struct nvt_fmem_mem_info_t *info, nvt_fmem_alloc_type_t type, size_t size, struct device *dev)
{
	int ret = 0;

	if (info != NULL) {
		info->size = size;
		info->type = type;
		info->dev = dev;
		ret = 0;
	} else {
		ret = -1;
	}

	return ret;
}
/**
 * @brief to resolve the virtual address (including direct mapping, ioremap or user space address to
 *      its real physical address.
 *
 * @parm vaddr indicates any virtual address
 *
 * @return  >= 0 for success, 0xFFFFFFFF for fail
 */
phys_addr_t fmem_lookup_pa(unsigned int vaddr);

/* @this function is a data cache operation function,
 * @parm: vaddr: any virtual address
 * @parm: dir will be:
 * DMA_BIDIRECTIONAL = 0, it means flush operation.
 * DMA_TO_DEVICE = 1, it means clean operation.
 * DMA_FROM_DEVICE = 2, it means invalidate operation.
 * DMA_NONE = 3,
 */
void fmem_dcache_sync(void *vaddr, u32 len, enum dma_data_direction dir);

/* @this function is a data cache operation function,
 * @parm: vaddr: any virtual address
 * @parm: dir will be:
 * DMA_BIDIRECTIONAL = 0, it means flush operation.
 * DMA_TO_DEVICE = 1, it means clean operation.
 * DMA_FROM_DEVICE = 2, it means invalidate operation.
 * DMA_NONE = 3,
 * @para:cache_op_cpu_id indicate which CPU involve the cache operation
 */
unsigned long fmem_dcache_sync_by_cpu(void *vaddr, u32 len, enum dma_data_direction dir, unsigned int cache_op_cpu_id);

/* @this function is a video buffer data cache operation function,
 *       default will ignore the cache flush operation
 * @parm: vaddr: any virtual address
 * @parm: dir will be:
 * DMA_BIDIRECTIONAL = 0, it means flush operation.
 * DMA_TO_DEVICE = 1, it means clean operation.
 * DMA_FROM_DEVICE = 2, it means invalidate operation.
 * DMA_NONE = 3,
 */
void fmem_dcache_sync_vb(void *vaddr, u32 len, enum dma_data_direction dir);

/* @this function used to release memory to cma region.
 * @ 	Actually it call dma_free_coherent/dma_free_writecombine/dma_release_from_contiguous() in kernel
 * @param:
 *	  handle: Using the allocated API return handle value as this argument
 *	  index: cma area index (0, 1,......)
 * return: 0 for success, < 0 for fail
 */
int fmem_release_from_cma(void *handle, unsigned int index);

/* @this function used to release cma area memory
 * @ 	Actually it call dma_alloc_coherent/dma_alloc_writecombine/dma_alloc_from_contiguous in kernel
 * @param:
 *	   nvt_fmem_info: memory size and cache type,
 *	   index: cma area index (0, 1,.....)
 * return: handle pointer for success, NULL for fail
 */
void* fmem_alloc_from_cma(struct nvt_fmem_mem_info_t *nvt_fmem_info, unsigned int index);
#endif /* __ASM_ARCH_NVT_IVOT_FMEM_H */
