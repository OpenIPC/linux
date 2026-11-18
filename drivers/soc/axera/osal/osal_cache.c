/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <asm/cacheflush.h>
#include <linux/dma-direction.h>
#include <linux/version.h>

#include "osal_ax.h"
#include "osal_dev_ax.h"

void AX_OSAL_DEV_invalidate_dcache_area(void * addr, int size)
{
#ifdef CONFIG_ARM64
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	dcache_inval_poc((unsigned long) addr, (unsigned long) (addr + size));
#else
	__inval_dcache_area(addr, (size_t)size);
#endif
#else
	dmac_inv_range(addr, addr+size);
#endif

	return;
}

EXPORT_SYMBOL(AX_OSAL_DEV_invalidate_dcache_area);

void AX_OSAL_DEV_flush_dcache_area(void * kvirt, unsigned long length)
{
#ifdef CONFIG_ARM64
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	dcache_clean_inval_poc((unsigned long) kvirt, (unsigned long) (kvirt + length));
#else
	__flush_dcache_area(kvirt, (size_t)length);
#endif
#else
	__cpuc_flush_dcache_area(kvirt, length);
#endif
}

EXPORT_SYMBOL(AX_OSAL_DEV_flush_dcache_area);

void AX_OSAL_DEV_outer_dcache_area(u64 phys_addr_start, u64 phys_addr_end)
{
#ifdef CONFIG_ARM64
	return;
#else
	outer_flush_range(phys_addr_start, phys_addr_end);
#endif
}

EXPORT_SYMBOL(AX_OSAL_DEV_outer_dcache_area);

void AX_OSAL_DEV_flush_dcache_all(void)
{
#ifdef CONFIG_ARM64
	//axera_armv8_flush_cache_all();
#else

#ifdef CONFIG_SMP
    on_each_cpu((smp_call_func_t)__cpuc_flush_kern_all, NULL, 1);
#else
    __cpuc_flush_kern_all();
#endif

    outer_flush_all();

#endif
}

EXPORT_SYMBOL(AX_OSAL_DEV_flush_dcache_all);
