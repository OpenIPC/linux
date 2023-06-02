#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <asm/signal.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/dma-mapping.h>
#include <asm/cache.h>
#include <asm/cacheflush.h>
#include <asm/page.h>
#include <asm/memory.h>
#ifndef CONFIG_ARM64
#include "../../arch/arm/mm/dma.h"
#endif
#include "nvt_perf_dbg.h"
#include "nvt_perf_util.h"

#define ALLOC_PAGE_ORDER 10
#define ALLOC_SIZE (PAGE_SIZE * (1 << ALLOC_PAGE_ORDER))

#ifdef CONFIG_CACHE_L2X0
extern unsigned long nvt_l2c210_clean_all(void);
extern unsigned long nvt_l2c210_flush_all(void);
#endif

#if defined(CONFIG_CPU_V7)
extern void v7_flush_d_cache_all_if(void);
#endif

static DEFINE_RAW_SPINLOCK(cache_lock);

unsigned int time_interval(struct timeval *time_result, struct timeval *time_start, struct timeval *time_end);

int do_cache_perf(void)
{
	unsigned char* buff_va;
	phys_addr_t    buff_pa;
	unsigned int   op_size;
	unsigned int   size_index;
	struct timeval time_start;
	struct timeval time_stop;
	struct timeval time_result;
#ifdef CONFIG_OUTER_CACHE
	struct timeval time_stop_l1;
	struct timeval time_result_l1;
	struct timeval time_result_l2;
#endif
	int ret = 0;

	buff_va = (unsigned char*)__get_free_pages(GFP_KERNEL, ALLOC_PAGE_ORDER);
	if (buff_va == NULL) {
		nvt_dbg(ERR, "__get_free_pages failed\n");
		return -1;
	}

 	buff_pa =  __pa((unsigned long)buff_va);
	size_index = 1;

	raw_spin_lock(&cache_lock);

	/* cache operation via linux native API */
	op_size = 0x8000;

	/* Flush cache range*/
	printk("============== Flush cache range ==============\n");
	do {
		size_index++;
		memset(&buff_va[0], size_index, op_size);

		do_gettimeofday(&time_start);

#ifdef CONFIG_ARM64
		__dma_flush_area(buff_va, op_size);
#else
		dmac_flush_range(buff_va, buff_va + op_size);
#endif

#ifdef CONFIG_OUTER_CACHE
		do_gettimeofday(&time_stop_l1);
		outer_flush_range(buff_pa, buff_pa + op_size);
#endif
		do_gettimeofday(&time_stop);

		ret = time_interval(&time_result, &time_start, &time_stop);
		if (ret < 0) {
			printk("Invalid time value start:%ld.%06ld end:%ld.%06ld\n", time_start.tv_sec, time_start.tv_usec, time_stop.tv_sec, time_stop.tv_usec);
			return ret;
		}

#ifdef CONFIG_OUTER_CACHE
		ret = time_interval(&time_result_l1, &time_start, &time_stop_l1);
		if (ret < 0) {
			printk("Invalid time value(L1) start:%ld.%06ld end:%ld.%06ld\n", time_start.tv_sec, time_start.tv_usec, time_stop_l1.tv_sec, time_stop_l1.tv_usec);
			return ret;
		}

		ret = time_interval(&time_result_l2, &time_stop_l1, &time_stop);
		if (ret < 0) {
			printk("Invalid time value(L2) start:%ld.%06ld end:%ld.%06ld\n", time_start.tv_sec, time_start.tv_usec, time_stop_l1.tv_sec, time_stop_l1.tv_usec);
			return ret;
		}

		printk("cache flush size 0x%8x, Total(sec):%ld.%06ld l1(sec):%ld.%06ld l2(sec):%ld.%06ld\r\n", op_size, time_result.tv_sec, time_result.tv_usec, time_result_l1.tv_sec, time_result_l1.tv_usec, time_result_l2.tv_sec, time_result_l2.tv_usec);
#else
		printk("cache flush size 0x%8x, Total(sec):%ld.%06ld\r\n", op_size, time_result.tv_sec, time_result.tv_usec);
#endif
		op_size = op_size << 1;

	} while (op_size <= ALLOC_SIZE);


	printk("\n");
	op_size = 0x8000;
	/* Flush cache range*/
	printk("============== clean cache range ==============\n");
	do {
		size_index++;
		memset(&buff_va[0], size_index, op_size);

		do_gettimeofday(&time_start);

#ifdef CONFIG_ARM64
		__dma_map_area(buff_va, op_size, DMA_TO_DEVICE);
#else
		dmac_map_area(buff_va, op_size, DMA_TO_DEVICE);
#endif

#ifdef CONFIG_OUTER_CACHE
		do_gettimeofday(&time_stop_l1);
		outer_clean_range(buff_pa, buff_pa + op_size);
#endif
		do_gettimeofday(&time_stop);

		ret = time_interval(&time_result, &time_start, &time_stop);
		if (ret < 0) {
			printk("Invalid time value start:%ld.%06ld end:%ld.%06ld\n", time_start.tv_sec, time_start.tv_usec, time_stop.tv_sec, time_stop.tv_usec);
			return ret;
		}

#ifdef CONFIG_OUTER_CACHE
		ret = time_interval(&time_result_l1, &time_start, &time_stop_l1);
		if (ret < 0) {
			printk("Invalid time value(L1) start:%ld.%06ld end:%ld.%06ld\n", time_start.tv_sec, time_start.tv_usec, time_stop_l1.tv_sec, time_stop_l1.tv_usec);
			return ret;
		}

		ret = time_interval(&time_result_l2, &time_stop_l1, &time_stop);
		if (ret < 0) {
			printk("Invalid time value(L2) start:%ld.%06ld end:%ld.%06ld\n", time_start.tv_sec, time_start.tv_usec, time_stop_l1.tv_sec, time_stop_l1.tv_usec);
			return ret;
		}

		printk("cache clean size 0x%8x, Total(sec):%ld.%06ld l1(sec):%ld.%06ld l2(sec):%ld.%06ld\r\n", op_size, time_result.tv_sec, time_result.tv_usec, time_result_l1.tv_sec, time_result_l1.tv_usec, time_result_l2.tv_sec, time_result_l2.tv_usec);

#else
		printk("cache clean size 0x%8x, Total(sec):%ld.%06ld\r\n", op_size, time_result.tv_sec, time_result.tv_usec);
#endif

		op_size = op_size << 1;

	} while (op_size <= ALLOC_SIZE);


	printk("\n");
	op_size = 0x8000;

	/* Flush cache range*/
	printk("============== Invalidate cache range ==============\n");
	do {
		size_index++;
		memset(&buff_va[0], size_index, op_size);

		do_gettimeofday(&time_start);

#ifdef CONFIG_ARM64
		__dma_map_area(buff_va, op_size, DMA_FROM_DEVICE);
#else
		dmac_map_area(buff_va, op_size, DMA_FROM_DEVICE);
#endif

#ifdef CONFIG_OUTER_CACHE
		do_gettimeofday(&time_stop_l1);
		outer_inv_range(buff_pa, buff_pa + op_size);
#endif
		do_gettimeofday(&time_stop);

		ret = time_interval(&time_result, &time_start, &time_stop);
		if (ret < 0) {
			printk("Invalid time value start:%ld.%06ld end:%ld.%06ld\n", time_start.tv_sec, time_start.tv_usec, time_stop.tv_sec, time_stop.tv_usec);
			return ret;
		}

#ifdef CONFIG_OUTER_CACHE
		ret = time_interval(&time_result_l1, &time_start, &time_stop_l1);
		if (ret < 0) {
			printk("Invalid time value(L1) start:%ld.%06ld end:%ld.%06ld\n", time_start.tv_sec, time_start.tv_usec, time_stop_l1.tv_sec, time_stop_l1.tv_usec);
			return ret;
		}

		ret = time_interval(&time_result_l2, &time_stop_l1, &time_stop);
		if (ret < 0) {
			printk("Invalid time value(L2) start:%ld.%06ld end:%ld.%06ld\n", time_start.tv_sec, time_start.tv_usec, time_stop_l1.tv_sec, time_stop_l1.tv_usec);
			return ret;
		}

		printk("cache invalidate size 0x%8x, Total(sec):%ld.%06ld l1(sec):%ld.%06ld l2(sec):%ld.%06ld\r\n", op_size, time_result.tv_sec, time_result.tv_usec, time_result_l1.tv_sec, time_result_l1.tv_usec, time_result_l2.tv_sec, time_result_l2.tv_usec);
#else
		printk("cache invalidate size 0x%8x, Total(sec):%ld.%06ld\r\n", op_size, time_result.tv_sec, time_result.tv_usec);
#endif
		op_size = op_size << 1;

	} while (op_size <= ALLOC_SIZE);


	printk("\n");
	op_size = 0x8000;

#ifndef CONFIG_ARM64
	printk("============== flush cache all ==============\n");
	do {
		size_index++;
		memset(&buff_va[0], size_index, op_size);

		do_gettimeofday(&time_start);
		flush_cache_all();

#ifdef CONFIG_OUTER_CACHE
#ifdef CONFIG_CACHE_L2X0
		nvt_l2c210_flush_all();
#else
		printk("unknown outer cache type\n");
#endif
#endif
		do_gettimeofday(&time_stop);

		ret = time_interval(&time_result, &time_start, &time_stop);
		if (ret < 0) {
			printk("Invalid time value start:%ld.%06ld end:%ld.%06ld\n", time_start.tv_sec, time_start.tv_usec, time_stop.tv_sec, time_stop.tv_usec);
			return ret;
		}

		printk("cache flush size 0x%8x, Total(sec):%ld.%06ld\r\n", op_size, time_result.tv_sec, time_result.tv_usec);

		op_size = op_size << 1;

	} while (op_size <= ALLOC_SIZE);

	printk("\n");
	op_size = 0x8000;

	printk("============== clean cache all ==============\n");
	do {
		size_index++;
		memset(&buff_va[0], size_index, op_size);

		do_gettimeofday(&time_start);

#if defined(CONFIG_CPU_V7)
		v7_flush_d_cache_all_if();
#else
		flush_cache_all();
#endif

#ifdef CONFIG_OUTER_CACHE
#ifdef CONFIG_CACHE_L2X0
		nvt_l2c210_clean_all();
#else
		printk("unknown outer cache type\n");
#endif
#endif
		do_gettimeofday(&time_stop);

		ret = time_interval(&time_result, &time_start, &time_stop);
		if (ret < 0) {
			printk("Invalid time value start:%ld.%06ld end:%ld.%06ld\n", time_start.tv_sec, time_start.tv_usec, time_stop.tv_sec, time_stop.tv_usec);
			return ret;
		}

		printk("cache clean size 0x%8x, Total(sec):%ld.%06ld\r\n", op_size, time_result.tv_sec, time_result.tv_usec);

		op_size = op_size << 1;

	} while (op_size <= ALLOC_SIZE);
#else
	nvt_dbg(ERR, "The API of flush/clean cache all doesn't support by ARMv8, so ignore related test\n");
#endif

	raw_spin_unlock(&cache_lock);
	free_pages((unsigned long)buff_va, ALLOC_PAGE_ORDER);

	return 0;
}
