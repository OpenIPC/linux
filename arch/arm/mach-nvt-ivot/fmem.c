/**
    NVT memory operation handling
    To handle the flush allocate and other memory handling api
    @file       fmem.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/memblock.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/mmzone.h>
#include <linux/memory.h>
#include <linux/kallsyms.h>
#include <linux/nodemask.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/cma.h>
#include <linux/mutex.h>
#include <asm/setup.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/tlbflush.h>
#include <asm/glue-cache.h>
#include <asm/outercache.h>
#include <asm/mach/map.h>
#include <asm/mmu.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/of.h>
#include <linux/spinlock.h>
#include <linux/cpumask.h>
#include <linux/smp.h>
#include <mach/fmem.h>
#include "../../arch/arm/mm/dma.h"
#include "../../arch/arm/mm/mm.h"

#define DEBUG_WRONG_CACHE_API   0x0
#define DEBUG_WRONG_CACHE_VA    0x1
#define DEBUG_WRONG_CACHE_LEN   0x2

#define CACHE_ALIGN_MASK        0x3F  //cache line with 64 bytes
#define MAX_CMD_LENGTH          30
#define MAX_CACHE_TRY_LOCK      10

/*
 * Local variables declaration
 */
static DEFINE_SEMAPHORE(sema_cma);
static DEFINE_SEMAPHORE(sema_cma_mem);
static DEFINE_SEMAPHORE(sema_mem_lock);
#ifdef CONFIG_CMA
struct list_head nvt_cma_info_list_root;
#endif

u32 nonlinear_cpuaddr_flush = 0;
u32 cpuaddr_flush = 0;
u32 va_not_64align = 0, length_not_64align = 0;
u32 debug_counter = 0;
static u32 vb_cache_flush_en = 0;
unsigned int total_num_cpu = NR_CPUS;

struct cma {
	unsigned long   base_pfn;
	unsigned long   count;
	unsigned long   *bitmap;
	unsigned int order_per_bit; /* Order of pages represented by one bit */
	struct mutex    lock;
#ifdef CONFIG_CMA_DEBUGFS
	struct hlist_head mem_head;
	spinlock_t mem_head_lock;
#endif
};

unsigned long  fmem_dcache_sync_op(void *vaddr, u32 len, enum dma_data_direction dir, unsigned int num_cpu_involve, unsigned int use_native_api);

#if (defined(CONFIG_NVT_CACHE_FLUSH_ALL_FOR_HDAL) || \
	defined(CONFIG_NVT_CACHE_INVALIDATE_ALL_FOR_HDAL))
static DEFINE_RAW_SPINLOCK(l1_flush_all_lock);

#define CACHE_THREAD_OP_THRESHOLD    0x400000
#ifdef CONFIG_CACHE_L2X0
	extern unsigned int l2c310_get_l2_size(void);
	#define CACHE_FLUSH_SIZE (l2c310_get_l2_size() << CONFIG_NVT_CACHE_FLUSH_ALL_THRESHOLD)
	#define CACHE_INV_SIZE (l2c310_get_l2_size() << CONFIG_NVT_CACHE_INVALIDATE_ALL_THRESHOLD)
#else
	#define CACHE_FLUSH_SIZE (0x20000)
	#define CACHE_INV_SIZE   (0x20000)
#endif

#ifdef CONFIG_OUTER_CACHE
extern unsigned long nvt_l2c210_clean_all(void);
extern unsigned long nvt_l2c210_flush_all(void);
#endif

#if defined(CONFIG_CPU_V7)
extern void v7_flush_d_cache_all_if(void);
#endif

unsigned long nvt_cache_op(void *vaddr, phys_addr_t paddr, u32 len, enum dma_data_direction dir, unsigned int max_retry, unsigned int num_cpu_involve)
{
	unsigned long ret = 0;
	unsigned long flags;

#ifdef CONFIG_OUTER_CACHE
	int try_lock;
#endif

	//L1 cache operation
	if (vaddr != NULL) {
		raw_spin_lock_irqsave(&l1_flush_all_lock, flags);

		switch (dir) {
		case DMA_BIDIRECTIONAL:
#ifdef CONFIG_NVT_CACHE_FLUSH_ALL_FOR_HDAL
			if (num_cpu_involve == 1) {
				flush_cache_all();
			} else
#endif
				dmac_flush_range(vaddr, vaddr + len);
			break;

		case DMA_TO_DEVICE:
#ifdef CONFIG_NVT_CACHE_FLUSH_ALL_FOR_HDAL
			if (num_cpu_involve == 1) {
#if defined(CONFIG_CPU_V7)
				v7_flush_d_cache_all_if();
#else
				flush_cache_all();
#endif
			} else
#endif
				dmac_map_area(vaddr, len, DMA_TO_DEVICE);
			break;

		case DMA_FROM_DEVICE:
#ifdef CONFIG_NVT_CACHE_INVALIDATE_ALL_FOR_HDAL
			if (num_cpu_involve == 1) {
				flush_cache_all();
			} else
#endif
				dmac_map_area(vaddr, len, DMA_FROM_DEVICE);
			break;

		default:
			pr_err("fmem:unknown data dir:%d\n", dir);
			raw_spin_unlock_irqrestore(&l1_flush_all_lock, flags);
			return (unsigned long)-1;
		}
		raw_spin_unlock_irqrestore(&l1_flush_all_lock, flags);
	}

#ifdef CONFIG_CACHE_L2X0
	//Do L2 cache operation if L2X0 present.
	try_lock = 0;
	ret = (unsigned long)-1;
	while (try_lock < max_retry) {
		switch (dir) {

		case DMA_BIDIRECTIONAL:
#ifdef CONFIG_NVT_CACHE_FLUSH_ALL_FOR_HDAL
			ret = nvt_l2c210_flush_all();
#else
			ret = (unsigned long) - 1;
			try_lock = max_retry;
#endif
			break;

		case DMA_TO_DEVICE:
#ifdef CONFIG_NVT_CACHE_FLUSH_ALL_FOR_HDAL
			ret = nvt_l2c210_clean_all();
#else
			ret = (unsigned long) - 1;
			try_lock = max_retry;
#endif
			break;

		case DMA_FROM_DEVICE:
#ifdef CONFIG_NVT_CACHE_INVALIDATE_ALL_FOR_HDAL
			ret = nvt_l2c210_flush_all();
#else
			ret = (unsigned long) - 1;
			try_lock = max_retry;
#endif
			break;

		default:
			pr_err("fmem:unknown data dir:%d\n", dir);
			return (unsigned long)-1;
		}

		if (ret != (unsigned long) -1) {
			break;
		}

		try_lock++;
		if (try_lock < max_retry) {
			udelay(200);
		}
	}
#else
	ret = (unsigned long)-1;
#endif

#if CONFIG_OUTER_CACHE
	/* Execute native cache operation when last cache operation failed */
	if (ret == (unsigned long) -1) {
		if (paddr != 0) {
			ret = 0;

			switch (dir) {
			case DMA_BIDIRECTIONAL:
				outer_flush_range(paddr, paddr + len);
				break;

			case DMA_TO_DEVICE:
				outer_clean_range(paddr, paddr + len);
				break;

			case DMA_FROM_DEVICE:
				outer_inv_range(paddr, paddr + len);
				break;

			default:
				pr_err("fmem:unknown data dir:%d\n", dir);
				ret = (unsigned long)-1;
			}
		}
	}
#endif

	return ret;
}

unsigned long fmem_dcache_sync_by_cpu(void *vaddr, u32 len, enum dma_data_direction dir, unsigned int cache_op_cpu_id)
{
	unsigned int cpu_id;
	unsigned long result;

	if (cache_op_cpu_id >= total_num_cpu) {
		pr_err("%s: Invalidate cpu id:%d\n", __func__, cache_op_cpu_id);
		return (unsigned long)-1;
	}

	if (in_irq() || irqs_disabled()) {
		result = fmem_dcache_sync_op(vaddr, len, dir, NR_CPUS, 1);
	} else {

		cpu_id = get_cpu();

		if (cpu_id == cache_op_cpu_id) {
			result = fmem_dcache_sync_op(vaddr, len, dir, 1, 0);
		} else {
			pr_err("%s:cpu_id mismatch(cpu_id:%d cache_op_cpu_id:%d), use default API to do cache operation\n", __func__, cpu_id, cache_op_cpu_id);
			result = fmem_dcache_sync_op(vaddr, len, dir, total_num_cpu, 0);
		}

		put_cpu();
	}

	return result;
}

#else
unsigned long fmem_dcache_sync_by_cpu(void *vaddr, u32 len, enum dma_data_direction dir, unsigned int cache_op_cpu_id)
{
	return fmem_dcache_sync_op(vaddr, len, dir, total_num_cpu, 0);
}
#endif

/* proc function
 */
static struct proc_dir_entry *fmem_proc_root = NULL;

/*************************************************************************************************************
 * cma handling
 *************************************************************************************************************/

#ifdef CONFIG_CMA
static struct nvt_cma_info_t* fmem_get_cma_info(unsigned int id)
{
	struct nvt_cma_info_t *curr_info, *next_info;

	down(&sema_cma);
	list_for_each_entry_safe(curr_info, next_info, &nvt_cma_info_list_root, list) {
        if (curr_info->id == id) {
			up(&sema_cma);
			return curr_info;
		}
	}
	up(&sema_cma);

	return NULL;
}

static struct nvt_cma_info_t* fmem_cma_info_alloc(struct platform_device *pdev, unsigned int id)
{
	struct nvt_cma_info_t* cma_info = NULL;

	if (id > MAX_CMA_AREAS) {
		dev_err(&pdev->dev, "fmem: Your cma area %u is larger than %d\n", id, MAX_CMA_AREAS);
		return NULL;
	}

	cma_info = fmem_get_cma_info(id);
	if (cma_info != NULL) {
		dev_err(&pdev->dev, "fmem: This cma area %u had already added\n", id);
		return NULL;
	}

	cma_info = (struct nvt_cma_info_t*)kzalloc(sizeof(struct nvt_cma_info_t), GFP_KERNEL);
	if (cma_info == NULL) {
		dev_err(&pdev->dev, "fmem: Your cma area %u can't be allocated\n", id);
		return NULL;
	}
	INIT_LIST_HEAD(&cma_info->list);
	INIT_LIST_HEAD(&cma_info->fmem_list_root);
	cma_info->fmem_mem_cache = kmem_cache_create("fmem_mem_cache", sizeof(struct nvt_fmem_mem_cma_info_t), 0, SLAB_PANIC, NULL);
	if (!cma_info->fmem_mem_cache) {
		pr_err("%s: Allocating memory failed\n", __func__);
		return NULL;
	}
	cma_info->id = id;

	return cma_info;
}

static struct nvt_fmem_mem_cma_info_t* fmem_get_cmamem_info(struct nvt_fmem_mem_cma_info_t* my_fmem_mem_info, unsigned int id)
{
	struct nvt_fmem_mem_cma_info_t *curr_info, *next_info;
	struct nvt_cma_info_t* cma_info = NULL;

	if (id > MAX_CMA_AREAS) {
		pr_err("fmem/%s: Your cma area %u is larger than %d\n", __func__, id, MAX_CMA_AREAS);
		return NULL;
	}

	cma_info = fmem_get_cma_info(id);
	if (cma_info == NULL) {
		pr_err("fmem/%s: This cma area %u can't be used\n", __func__, id);
		return NULL;
	}

	down(&sema_cma_mem);
	list_for_each_entry_safe(curr_info, next_info, &cma_info->fmem_list_root, list) {
        if ((unsigned long)my_fmem_mem_info == (unsigned long)curr_info) {
			up(&sema_cma_mem);
			if (curr_info->magic_num != NVT_FMEM_MEM_MAGIC) {
				panic("fmem/%s: Error magic number 0x%08lx\n", __func__, curr_info->magic_num);
			}
			return curr_info;
		}
	}
	up(&sema_cma_mem);

	return NULL;
}


static void* fmem_mem_info_alloc(struct nvt_fmem_mem_info_t *nvt_fmem_info, unsigned int id)
{
	struct nvt_cma_info_t* cma_info = NULL;
	struct nvt_fmem_mem_cma_info_t* my_fmem_meminfo = NULL;

	if (id > MAX_CMA_AREAS) {
		pr_err("fmem/%s: Your cma area %u is larger than %d\n", __func__, id, MAX_CMA_AREAS);
		return NULL;
	}

	cma_info = fmem_get_cma_info(id);
	if (cma_info == NULL) {
		pr_err("fmem/%s: This cma area %u can't be used\n", __func__, id);
		return NULL;
	}

	/* Allocate a buffer to record memory info */
	my_fmem_meminfo = (struct nvt_fmem_mem_cma_info_t *)kmem_cache_zalloc(cma_info->fmem_mem_cache, GFP_KERNEL);
	if (!my_fmem_meminfo) {
		return NULL;
	}

	/* Copy memory info to managed data struct */
	memcpy(&my_fmem_meminfo->mem_info, nvt_fmem_info, sizeof(struct nvt_fmem_mem_info_t));
	INIT_LIST_HEAD(&my_fmem_meminfo->list);
	/* Add to list for the managed use */
	down(&sema_cma_mem);
	list_add_tail(&my_fmem_meminfo->list, &cma_info->fmem_list_root);
	up(&sema_cma_mem);
	my_fmem_meminfo->magic_num = NVT_FMEM_MEM_MAGIC;

	return (void*)my_fmem_meminfo;
}

static int fmem_mem_info_release(struct nvt_fmem_mem_cma_info_t *my_fmem_meminfo, unsigned int id)
{
	struct nvt_cma_info_t* cma_info = NULL;

	if (id > MAX_CMA_AREAS) {
		pr_err("fmem/%s: Your cma area %u is larger than %d\n", __func__, id, MAX_CMA_AREAS);
		return -1;
	}

	cma_info = fmem_get_cma_info(id);
	if (cma_info == NULL) {
		pr_err("fmem/%s: This cma area %u can't be used\n", __func__, id);
		return -1;
	}

	if (my_fmem_meminfo->magic_num != NVT_FMEM_MEM_MAGIC) {
		pr_err("fmem/%s: Error magic number with %p id: %d\n", __func__, my_fmem_meminfo, id);
		return -1;
	}

	/* Copy memory info to managed data struct */
	memset(&my_fmem_meminfo->mem_info, 0, sizeof(struct nvt_fmem_mem_info_t));
	/* Delete list from fmem_list_root */
	down(&sema_cma_mem);
	list_del(&my_fmem_meminfo->list);
	up(&sema_cma_mem);
	/* Release a buffer to record memory info */
	kmem_cache_free(cma_info->fmem_mem_cache, my_fmem_meminfo);

	return 0;
}

static int fmem_cma_info_free(struct platform_device *pdev, unsigned int id)
{
	struct nvt_cma_info_t *curr_info, *next_info;

	if (id > MAX_CMA_AREAS) {
		dev_err(&pdev->dev, "fmem: Your cma area %u is larger than %d\n", id, MAX_CMA_AREAS);
		return -1;
	}

	down(&sema_cma);
	list_for_each_entry_safe(curr_info, next_info, &nvt_cma_info_list_root, list) {
		if (curr_info->id == id) {
			list_del(&curr_info->list);
			memset(curr_info, 0, sizeof(struct nvt_cma_info_t));
			kfree(curr_info);
			up(&sema_cma);
			return 0;
		}
	}
	up(&sema_cma);

	dev_err(&pdev->dev, "fmem: We can't free this id%u \n", id);
	return -1;
}

static int fmem_add_cma_info(struct platform_device *pdev, struct nvt_cma_info_t* cma_info)
{
	if (cma_info == NULL) {
		dev_err(&pdev->dev, "fmem: We can't add this cma_info because this is NULL\n");
		return -1;
	}

	down(&sema_cma);
	list_add_tail(&cma_info->list, &nvt_cma_info_list_root);
	cma_info->name = pdev->name;
	cma_info->dev = &pdev->dev;
	up(&sema_cma);

	return 0;
}

static int nvt_cma_probe(struct platform_device *pdev)
{
	struct nvt_cma_info_t *cma_info = NULL;
	int ret = 0;
	unsigned int id = 0;

	/* pointer to shared-dma-pool in dts */
	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "memory reserved init failed with %d\n", ret);
		return ret;
	}

	of_property_read_u32(pdev->dev.of_node, "id",
				&id);

	cma_info = fmem_cma_info_alloc(pdev, id);
	if (cma_info == NULL)
		return -1;

	cma_info->cma = dev_get_cma_area(&pdev->dev);
	if (!cma_info->cma) {
		dev_err(&pdev->dev, "fmem: cma id%u probe failed\n", id);
		return -1;
	} else {
		cma_info->start = cma_get_base(cma_info->cma);
		cma_info->size = cma_get_size(cma_info->cma);
	}

	ret = fmem_add_cma_info(pdev, cma_info);
	if (ret < 0) {
		fmem_cma_info_free(pdev, id);
		return -1;
	}

	dev_info(&pdev->dev, "fmem: cma area id: %u, cma base:0x%x, size:0x%x added\n", id, cma_info->start, (u32)cma_info->size);
	dev_info(&pdev->dev, "fmem/Version: %s\n", NVT_FMEM_VERSION);
	dev_info(&pdev->dev, "Probe successfully\n");
	return 0;
}

static const struct of_device_id nvt_cma_dt_match[] = {
	{.compatible = "nvt,nvt_cma"},
	{},
};
MODULE_DEVICE_TABLE(of, nvt_cma_dt_match);

static struct platform_driver nvt_cma_driver = {
	.probe = nvt_cma_probe,
	.driver = {
		.name = "nvt_cma",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(nvt_cma_dt_match),
	},
};

static int __init nvt_cma_early_init(void)
{
	int ret = 0;

	INIT_LIST_HEAD(&nvt_cma_info_list_root);
	ret = platform_driver_register(&nvt_cma_driver);
	if (ret < 0) {
		pr_err("%s: init failed\n", __func__);
	}

	return 0;
}
core_initcall(nvt_cma_early_init);
#endif

void* fmem_alloc_from_cma(struct nvt_fmem_mem_info_t *nvt_fmem_info, unsigned int index)
{
	#ifdef CONFIG_CMA
	struct page *pages;
	void *vaddr = NULL;
	void *handle = NULL;
	unsigned page_count = ALIGN(nvt_fmem_info->size, PAGE_SIZE)/PAGE_SIZE;
	struct nvt_cma_info_t *cma_info = NULL;
	struct device * dev = NULL;


	down(&sema_mem_lock);
	/* To get cma info by id */
	cma_info = fmem_get_cma_info(index);
	if (cma_info == NULL) {
		pr_err("fmem/%s: This cma area %u can't be used\n", __func__, index);
		goto alloc_cma_exit;
	}
	dev = cma_info->dev;

	if (!dev)
		goto alloc_cma_exit;

	if (page_count == 0)
		goto alloc_cma_exit;

	if (nvt_fmem_info->type == NVT_FMEM_ALLOC_CACHE) {
		pages = dma_alloc_from_contiguous(dev, page_count, 0, __GFP_NOWARN);
		if (pages != NULL) {
			nvt_fmem_info->paddr = __pfn_to_phys(page_to_pfn(pages));
			vaddr = __va(nvt_fmem_info->paddr);
		}
	} else if (nvt_fmem_info->type == NVT_FMEM_ALLOC_NONCACHE) {
		vaddr = dma_alloc_coherent(dev, nvt_fmem_info->size, &nvt_fmem_info->paddr,  GFP_KERNEL);
		page_count = 0;
		pages = NULL;
	} else {
		vaddr = dma_alloc_writecombine(dev, nvt_fmem_info->size, &nvt_fmem_info->paddr, GFP_KERNEL);
		page_count = 0;
		pages = NULL;
	}

	if (vaddr == NULL) {
		pr_err("%s:%d Can't to create this buffer with CMA area %d\n", __func__, __LINE__, index);
		goto alloc_cma_exit;
	}

	nvt_fmem_info->vaddr = vaddr;
	nvt_fmem_info->page_count = page_count;
	nvt_fmem_info->pages = pages;
	nvt_fmem_info->size = ALIGN(nvt_fmem_info->size, PAGE_SIZE);

	handle = fmem_mem_info_alloc(nvt_fmem_info, index);
	if (handle == NULL) {
		if (nvt_fmem_info->type == NVT_FMEM_ALLOC_CACHE) {
			dma_release_from_contiguous(dev, pages, page_count);
		} else if (nvt_fmem_info->type == NVT_FMEM_ALLOC_NONCACHE) {
			dma_free_coherent(dev, nvt_fmem_info->size, vaddr, nvt_fmem_info->paddr);
		} else {
			dma_free_writecombine(dev, nvt_fmem_info->size, vaddr, nvt_fmem_info->paddr);
		}
		goto alloc_cma_exit;
	}

	up(&sema_mem_lock);
	return handle;

alloc_cma_exit:
	up(&sema_mem_lock);
	return NULL;
	#else
	return NULL;
	#endif
}

int fmem_release_from_cma(void *handle, unsigned int index)
{
	#ifdef CONFIG_CMA
	struct nvt_fmem_mem_cma_info_t* my_fmem_meminfo = NULL;
	int ret = 0;
	struct nvt_cma_info_t *cma_info = NULL;
	struct device * dev = NULL;

	down(&sema_mem_lock);
	/* To get cma info by id */
	cma_info = fmem_get_cma_info(index);
	if (cma_info == NULL) {
		pr_err("fmem/%s: This cma area %u can't be used\n", __func__, index);
		goto release_cma_exit;
	}
	dev = cma_info->dev;

	if (!dev)
		goto release_cma_exit;

	my_fmem_meminfo = fmem_get_cmamem_info((struct nvt_fmem_mem_cma_info_t*)handle, index);
	if (my_fmem_meminfo == NULL) {
		pr_err("fmem/%s: Error to release buffer with this handle %p\n", __func__, handle);
		goto release_cma_exit;
	}

	if (my_fmem_meminfo->mem_info.type == NVT_FMEM_ALLOC_CACHE) {
		dma_release_from_contiguous(dev, my_fmem_meminfo->mem_info.pages, my_fmem_meminfo->mem_info.page_count);
	} else if (my_fmem_meminfo->mem_info.type == NVT_FMEM_ALLOC_NONCACHE) {
		dma_free_coherent(dev, my_fmem_meminfo->mem_info.size, my_fmem_meminfo->mem_info.vaddr, my_fmem_meminfo->mem_info.paddr);
	} else {
		dma_free_writecombine(dev, my_fmem_meminfo->mem_info.size, my_fmem_meminfo->mem_info.vaddr, my_fmem_meminfo->mem_info.paddr);
	}

	ret = fmem_mem_info_release(my_fmem_meminfo, index);
	if (ret < 0) {
		pr_err("fmem/%s: We can't release this cma memory info\n", __func__);
		goto release_cma_exit;
	}

	up(&sema_mem_lock);
	return 0;

release_cma_exit:
	up(&sema_mem_lock);
	return -1;
	#else
	return 0;
	#endif
}

/*************************************************************************************************************
 * fmem proc handling
 *************************************************************************************************************/

static ssize_t proc_resolve_pa(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	unsigned char value[30];
	unsigned int vaddr;
	phys_addr_t  paddr;

	if (copy_from_user(value, buffer, count))
		return 0;

	sscanf(value, "%x\n", &vaddr);
	paddr = fmem_lookup_pa(vaddr);

	printk("Resolve vaddr: 0x%x ---> paddr: 0x%x \n", vaddr, paddr);

	return count;
}

static int resolve_proc_show(struct seq_file *m, void *v)
{
	return 0;
}
static int resolve_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, resolve_proc_show, NULL);
}

static const struct file_operations resolve_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= resolve_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= proc_resolve_pa,
};

#ifdef CONFIG_CMA
struct nvtcma_proc_list_cmainfo_t {
	struct seq_file *sfile;
	phys_addr_t		base;
	size_t			size;
};

static int nvtcma_list_all_cma_info(struct cma *cma, void*data)
{
	struct nvtcma_proc_list_cmainfo_t *cmainfo = (struct nvtcma_proc_list_cmainfo_t *)data;
	unsigned long used = 0;
	u64 val = 0;

	if (cma_get_base(cma) == cmainfo->base) {
		mutex_lock(&cma->lock);
		/* pages counter is smaller than sizeof(int) */
		used = bitmap_weight(cma->bitmap, (int)cma->count);
		mutex_unlock(&cma->lock);
		val = (u64)used << cma->order_per_bit;
		if (cmainfo->size != cma_get_size(cma))
			seq_printf(cmainfo->sfile, "You used the dma api directly, not from nvt fmem api!!!\n");
		seq_printf(cmainfo->sfile, "CMA managed >> Used/Total size: %lu(bytes)/%lu(bytes), %llu(pages)\n", (used * PAGE_SIZE), cma_get_size(cma), val);
	}

	return 0;
}

static int nvtcma_proc_show(struct seq_file *sfile, void *v)
{
	struct nvt_cma_info_t *curr_info, *next_info;
	struct nvt_fmem_mem_cma_info_t *curr_mem_info, *next_mem_info;
	struct nvtcma_proc_list_cmainfo_t proc_cmainfo;
	size_t total_used_size = 0;

	seq_printf(sfile, "================= NVT CMA INFO =================\n");
	down(&sema_cma);
	list_for_each_entry_safe(curr_info, next_info, &nvt_cma_info_list_root, list) {
		seq_printf(sfile, "\r\tArea:%d Name: %s Total size: %u\n", curr_info->id, curr_info->name, curr_info->size);
		seq_printf(sfile, "\r\tphysical address: 0x%08x@0x%08x \n", curr_info->size, curr_info->start);
		down(&sema_cma_mem);
		list_for_each_entry_safe(curr_mem_info, next_mem_info, &curr_info->fmem_list_root, list) {
			seq_printf(sfile, "\r\t\t Device_Name: %s, Type: %s, Physical addr:0x%08x, Virtual addr: 0x%08lx, size: %u\n",
															(curr_mem_info->mem_info.dev != NULL)?curr_mem_info->mem_info.dev->init_name:"",
															(curr_mem_info->mem_info.type == NVT_FMEM_ALLOC_CACHE)?"NVT_FMEM_ALLOC_CACHE":
															(curr_mem_info->mem_info.type == NVT_FMEM_ALLOC_NONCACHE)?"NVT_FMEM_ALLOC_NONCACHE":
															(curr_mem_info->mem_info.type == NVT_FMEM_ALLOC_BUFFER)?"NVT_FMEM_ALLOC_BUFFER":"ERROR Type",
															curr_mem_info->mem_info.paddr,
															(unsigned long)curr_mem_info->mem_info.vaddr,
															curr_mem_info->mem_info.size);
			total_used_size += curr_mem_info->mem_info.size;
		}
		proc_cmainfo.sfile = sfile;
		proc_cmainfo.base = curr_info->start;
		proc_cmainfo.size = curr_info->size;
		cma_for_each_area(nvtcma_list_all_cma_info, (void*)&proc_cmainfo);
		seq_printf(sfile, "Fmem managed >> Used/Total size: %u(bytes)/%u(bytes)\n\n", total_used_size, curr_info->size);
		up(&sema_cma_mem);
	}
	up(&sema_cma);

	return 0;
}
static int nvtcma_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvtcma_proc_show, NULL);
}

static const struct file_operations nvtcma_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= nvtcma_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static int version_proc_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "Version: %s\n", NVT_FMEM_VERSION);

	return 0;
}

static int version_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, version_proc_show, NULL);
}

static const struct file_operations version_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= version_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init fmem_proc_init(void)
{
	struct proc_dir_entry *p;
	static struct proc_dir_entry *myproc = NULL;

	p = proc_mkdir("fmem", NULL);
	if (p == NULL) {
		printk("%s, fail! \n", __func__);
		return -1;
	}

	fmem_proc_root = p;

	/* resolve, va->pa
	*/
	myproc = proc_create("resolve", S_IRUGO, fmem_proc_root, &resolve_proc_fops);
	if (myproc == NULL)
		panic("FMEM: Fail to create proc resolve_proc!\n");

	/* CMA info */
	#ifdef CONFIG_CMA
	myproc = proc_create("cma_info", S_IRUGO, fmem_proc_root, &nvtcma_proc_fops);
	if (myproc == NULL)
		panic("FMEM: Fail to create proc cma info!\n");
	#endif
	/* Version info */
	myproc = proc_create("version", S_IRUGO, fmem_proc_root, &version_proc_fops);
	if (myproc == NULL)
		panic("FMEM: Fail to create proc version info!\n");

	return 0;
}

static int __init fmem_init(void)
{
	int ret;

#ifdef CONFIG_HOTPLUG_CPU
	total_num_cpu = num_present_cpus();
#else
	total_num_cpu = num_online_cpus();
#endif

	ret =fmem_proc_init();

	return ret;
}

core_initcall(fmem_init);

/**
 * @brief to resolve the virtual address (including direct mapping, ioremap or user space address to
 *      its real physical address.
 *
 * @parm vaddr indicates any virtual address
 *
 * @return  >= 0 for success, 0xFFFFFFFF for fail
 */
phys_addr_t fmem_lookup_pa(unsigned int vaddr)
{
	pgd_t *pgdp;
	pmd_t *pmdp;
	pud_t *pudp;
	pte_t *ptep;
	phys_addr_t paddr = 0xFFFFFFFF;

	/* check if va is module space global variable */
	if (vaddr >= TASK_SIZE && vaddr < PAGE_OFFSET) {
		printk("Invalid va 0x%x , TASK_SIZE = 0x%x, PAGE_OFFSET = 0x%x\r\n", vaddr, (int)TASK_SIZE, (int)PAGE_OFFSET);
		dump_stack();
		return paddr;
	}
	/* for speed up */
	if (virt_addr_valid(vaddr)) {
		return (phys_addr_t)__pa(vaddr);
	}

	/*
	* The pgd is never bad, and a pmd always exists(as it's folded into the pgd entry)
	*/
	if (vaddr >= TASK_SIZE) { /* use init_mm as mmu to look up in kernel mode */
		pgdp = pgd_offset_k(vaddr);
	} else if (current && current->mm){
		pgdp = pgd_offset(current->mm, vaddr);
	} else {
		printk("Invalid va 0x%x\r\n", vaddr);
		dump_stack();
		return paddr;
	}

	if (unlikely(pgd_none(*pgdp))) {
		printk("fmem: 0x%x not mapped in pgd table! \n", vaddr);
		goto err;
	}

	/* because we don't have 3-level MMU, so pud = pgd. Here we are in order to meet generic Linux
	* version, pud is listed here.
	*/
	pudp = pud_offset(pgdp, vaddr);
	if (unlikely(pud_none(*pudp))) {
		printk(KERN_INFO"fmem: 0x%x not mapped in pud table! \n", vaddr);
		goto err;
	}

	pmdp = pmd_offset(pudp, vaddr);
	if (unlikely(pmd_none(*pmdp))) {
		printk(KERN_INFO"fmem: 0x%x not mapped in pmd 0x%x! \n", vaddr, (u32)pmdp);
		goto err;
	}

	if (!pmd_bad(*pmdp)) {
		//u32 v_addr;

		/* page mapping */
		ptep = pte_offset_kernel(pmdp, vaddr);
		if (pte_none(*ptep)) {
			pr_err("fmem: 0x%x not mapped in pte! \n", vaddr);
			goto err;
		}
		paddr = (pte_pfn(*ptep) << PAGE_SHIFT) + (vaddr & (PAGE_SIZE - 1));
	} else {
		/* section mapping. The cases are:
		* 1. DDR direct mapping
		* 2. ioremap's vaddr, size and paddr all are 2M alignment.
		*/
		if (vaddr & SECTION_SIZE)
			pmdp ++; /* move to pmd[1] */
		paddr = (pmd_val(*pmdp) & SECTION_MASK) | (vaddr & ~SECTION_MASK);
	}
err:
	return paddr;
}

/**
 * @brief to resolve the virtual address (including direct mapping, ioremap or user space address to
 *      its real physical address.
 *
 * @parm va indicates any virtual address
 * @parm mm is the process mm
 * @parm ret_pa is returned pa if succeeded
 *
 * @return >= 0 for success, -1 for failure
 */
int fmem_ptdump_v2p(unsigned long va, struct mm_struct *mm, phys_addr_t *ret_pa)
{
	pgd_t *pgdp;
	pmd_t *pmdp;
	pud_t *pudp;
	pte_t *ptep;
	struct static_vm *p_static_vm;

	/* for speed up */
	if (virt_addr_valid(va)) {
		*ret_pa = __pa(va);
		return 0;
	}

	/* find static mapping address */
	p_static_vm = find_static_vm_vaddr((void *)va);
	if (NULL != p_static_vm) {
		*ret_pa = (phys_addr_t)va - (phys_addr_t)p_static_vm->vm.addr + p_static_vm->vm.phys_addr;
		return 0;
	}

	/*
	* The pgd is never bad, and a pmd always exists(as it's folded into the pgd entry)
	*/
	if (va >= TASK_SIZE) {/* use init_mm as mmu to look up in kernel mode */
		pgdp = pgd_offset_k(va);
	} else {
		pgdp = pgd_offset(mm, va);
	}

	if (unlikely(pgd_none(*pgdp))) {
		goto err;
	}

	/* because we don't have 3-level MMU, so pud = pgd. Here we are in order to meet generic Linux
	* version, pud is listed here.
	*/
	pudp = pud_offset(pgdp, va);
	if (unlikely(pud_none(*pudp))) {
		goto err;
	}

	pmdp = pmd_offset(pudp, va);
	if (unlikely(pmd_none(*pmdp))) {
		goto err;
	}

	if (!pmd_bad(*pmdp)) {
		/* page mapping */
		ptep = pte_offset_kernel(pmdp, va);
		if (pte_none(*ptep)) {
			goto err;
		}
		*ret_pa = (pte_pfn(*ptep) << PAGE_SHIFT) + (va & (PAGE_SIZE - 1));
		return 0;
	} else {
		/* section mapping. The cases are:
		* 1. DDR direct mapping
		* 2. ioremap's vaddr, size and paddr all are 2M alignment.
		*/
		if (va & SECTION_SIZE) {
			pmdp ++; /* move to pmd[1] */
		}
		*ret_pa = (pmd_val(*pmdp) & SECTION_MASK) | (va & ~SECTION_MASK);
		return 0;
	}

err:
	*ret_pa = (phys_addr_t)-1;
	return -1;
}

static void dev_release(struct device *dev)
{
	return;
}

#define DRIVER_NAME "fmem_sync"
static struct platform_device pseudo_dev = {
	.name = DRIVER_NAME,
	.id   = 0,
	.num_resources = 0,
	.dev  = {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.release = dev_release,
	}
};

/* @this function is a data cache operation function,
 * @parm: vaddr: any virtual address
 * @parm: dir will be:
 * DMA_BIDIRECTIONAL = 0, it means flush operation.
 * DMA_TO_DEVICE = 1, it means clean operation.
 * DMA_FROM_DEVICE = 2, it means invalidate operation.
 * DMA_NONE = 3,
 */
void fmem_dcache_sync(void *vaddr, u32 len, enum dma_data_direction dir)
{
	fmem_dcache_sync_op(vaddr, len, dir, total_num_cpu, 0);
}

unsigned long fmem_dcache_sync_op(void *vaddr, u32 len, enum dma_data_direction dir, unsigned int num_cpu_involve, unsigned int use_native_api)
{
	struct device *dev = &pseudo_dev.dev;
	unsigned long ret = 0;
	#ifdef CONFIG_OUTER_CACHE
	phys_addr_t paddr = fmem_lookup_pa((unsigned int)vaddr);
	#endif

	if (!valid_dma_direction(dir))
		panic("%s, invalid direction: %d \n", __func__, dir);

	/* kernel buffer may not cache line alignment, it only can use both clean/inv
	* for safe. Others we regard them as warning cases in coding.
	*/
	if (dir != DMA_BIDIRECTIONAL) {
		if ((u32)vaddr & CACHE_ALIGN_MASK) {
			va_not_64align ++;
			if (debug_counter & (0x1 << DEBUG_WRONG_CACHE_VA)) {
				printk("%s, warning, vaddr: 0x%x not cache alignment! \n", __func__, (u32)vaddr);
				dump_stack();
			}
		}

		if (len & CACHE_ALIGN_MASK) {
			length_not_64align ++;
			if (debug_counter & (0x1 << DEBUG_WRONG_CACHE_LEN)) {
				printk("%s, warning, len: %d not cache alignment! \n", __func__, len);
				dump_stack();
			}
		}
	}

	if (virt_addr_valid(vaddr) && virt_addr_valid(vaddr + len - 1)) {
		/* Notice:
		* The following code only for the direct mapping VA
		*/
		switch (dir) {
		case DMA_BIDIRECTIONAL:
			dma_map_single(dev, vaddr, len, DMA_TO_DEVICE);
			/* v7_dma_unmap_area is only valid for DMA_FROM_DEVICE */
			dma_unmap_single(dev, __pa(vaddr), len, DMA_FROM_DEVICE);
			break;
		case DMA_TO_DEVICE:
			dma_map_single(dev, vaddr, len, DMA_TO_DEVICE);
			break;
		case DMA_FROM_DEVICE:
			/* v7_dma_unmap_area is only valid for DMA_FROM_DEVICE */
			dma_unmap_single(dev, __pa(vaddr), len, DMA_FROM_DEVICE);
			break;
		case DMA_NONE:
			break;
		}
		cpuaddr_flush ++;
	} else {
		/* Notice:
		* We must process outer cache if have. The code is not implement yet!
		*/
		switch (dir) {
		case DMA_BIDIRECTIONAL:
#ifdef CONFIG_NVT_CACHE_FLUSH_ALL_FOR_HDAL
			if ((len > CACHE_FLUSH_SIZE) && (use_native_api == 0)) {
				ret = nvt_cache_op(vaddr, paddr, len, DMA_BIDIRECTIONAL, MAX_CACHE_TRY_LOCK, num_cpu_involve);
			} else {
#else
			{
#endif
				dmac_flush_range(vaddr, vaddr + len);
				#ifdef CONFIG_OUTER_CACHE
				outer_flush_range(paddr, paddr + len);
				#endif
			}
			break;

		case DMA_TO_DEVICE:
#ifdef CONFIG_NVT_CACHE_FLUSH_ALL_FOR_HDAL
			if (len > CACHE_FLUSH_SIZE && (use_native_api == 0)) {
				ret = nvt_cache_op(vaddr, paddr, len, DMA_TO_DEVICE, MAX_CACHE_TRY_LOCK, num_cpu_involve);
			} else {
#else
			{
#endif
				dmac_map_area(vaddr, len, DMA_TO_DEVICE);
				#ifdef CONFIG_OUTER_CACHE
				outer_clean_range(paddr, paddr + len);
				#endif
			}
			break;
		case DMA_FROM_DEVICE:
#ifdef CONFIG_NVT_CACHE_INVALIDATE_ALL_FOR_HDAL
			if (len > CACHE_INV_SIZE && (use_native_api == 0)) {
				ret = nvt_cache_op(vaddr, paddr, len, DMA_FROM_DEVICE, MAX_CACHE_TRY_LOCK, num_cpu_involve);
			} else {
#else
			{
#endif
				dmac_map_area(vaddr, len, DMA_FROM_DEVICE);
				#ifdef CONFIG_OUTER_CACHE
				outer_inv_range(paddr, paddr + len);
				#endif
			}
			break;
		case DMA_NONE:
			break;
		}
		nonlinear_cpuaddr_flush ++;
		if (debug_counter & (0x1 << DEBUG_WRONG_CACHE_API)) {
			printk("%s, warning, memory addr 0x%x not direct mapped! \n", __func__, (u32)vaddr);
			dump_stack();
		}
	}

	return ret;
}


void fmem_dcache_sync_vb(void *vaddr, u32 len, enum dma_data_direction dir)
{
	struct device *dev = &pseudo_dev.dev;
	#ifdef CONFIG_OUTER_CACHE
	phys_addr_t paddr = fmem_lookup_pa((unsigned int)vaddr);
	#endif

	if (0 == vb_cache_flush_en){
		return;
	}
	if (!valid_dma_direction(dir))
		panic("%s, invalid direction: %d \n", __func__, dir);

	/* kernel buffer may not cache line alignment, it only can use both clean/inv
	* for safe. Others we regard them as warning cases in coding.
	*/
	if (dir != DMA_BIDIRECTIONAL) {
		if ((u32)vaddr & CACHE_ALIGN_MASK) {
			va_not_64align ++;
			if (debug_counter & (0x1 << DEBUG_WRONG_CACHE_VA)) {
				printk("%s, warning, vaddr: 0x%x not cache alignment! \n", __func__, (u32)vaddr);
				dump_stack();
			}
		}

		if (len & CACHE_ALIGN_MASK) {
			length_not_64align ++;
			if (debug_counter & (0x1 << DEBUG_WRONG_CACHE_LEN)) {
				printk("%s, warning, len: %d not cache alignment! \n", __func__, len);
				dump_stack();
			}
		}
	}

	if (virt_addr_valid(vaddr) && virt_addr_valid(vaddr + len - 1)) {
		/* Notice:
		* The following code only for the direct mapping VA
		*/
		switch (dir) {
		case DMA_BIDIRECTIONAL:
			dma_map_single(dev, vaddr, len, DMA_TO_DEVICE);
			/* v7_dma_unmap_area is only valid for DMA_FROM_DEVICE */
			dma_unmap_single(dev, __pa(vaddr), len, DMA_FROM_DEVICE);
			break;
		case DMA_TO_DEVICE:
			dma_map_single(dev, vaddr, len, DMA_TO_DEVICE);
			break;
		case DMA_FROM_DEVICE:
			/* v7_dma_unmap_area is only valid for DMA_FROM_DEVICE */
			dma_unmap_single(dev, __pa(vaddr), len, DMA_FROM_DEVICE);
			break;
		case DMA_NONE:
			break;
		}
		cpuaddr_flush ++;
	} else {
		/* Notice:
		* We must process outer cache if have. The code is not implement yet!
		*/
		switch (dir) {
		case DMA_BIDIRECTIONAL:
			dmac_flush_range(vaddr, vaddr + len);
			#ifdef CONFIG_OUTER_CACHE
			outer_flush_range(paddr, paddr + len);
			#endif
			break;
		case DMA_TO_DEVICE:
			dmac_map_area(vaddr, len, DMA_TO_DEVICE);
			#ifdef CONFIG_OUTER_CACHE
			outer_clean_range(paddr, paddr + len);
			#endif
			break;
		case DMA_FROM_DEVICE:
			dmac_map_area(vaddr, len, DMA_FROM_DEVICE);
			#ifdef CONFIG_OUTER_CACHE
			outer_inv_range(paddr, paddr + len);
			#endif
			break;
		case DMA_NONE:
			break;
		}
		nonlinear_cpuaddr_flush ++;
		if (debug_counter & (0x1 << DEBUG_WRONG_CACHE_API)) {
			printk("%s, warning, memory addr 0x%x not direct mapped! \n", __func__, (u32)vaddr);
			dump_stack();
		}
	}
}

EXPORT_SYMBOL(fmem_alloc_from_cma);
EXPORT_SYMBOL(fmem_release_from_cma);
EXPORT_SYMBOL(fmem_lookup_pa);
EXPORT_SYMBOL(fmem_dcache_sync);
EXPORT_SYMBOL(fmem_dcache_sync_vb);
EXPORT_SYMBOL(fmem_dcache_sync_by_cpu);
