/*
 * arch/arm/mach-gk7101/cache.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>

#include <asm/cacheflush.h>
#ifdef CONFIG_CACHE_PL310
#include <asm/hardware/cache-l2x0.h>
#endif
#include <asm/io.h>

#include <mach/hardware.h>
#include <plat/cache.h>

/* ==========================================================================*/
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX	"gk_config."


/* ==========================================================================*/
#ifdef CONFIG_OUTER_CACHE
static u32 cache_l2_status = 0;
#ifdef CONFIG_CACHE_PL310
static void __iomem *gk_cache_l2_base = __io(GK_VA_L2CC_BASE);
#endif
#endif

static int cache_check_start = 1;
module_param(cache_check_start, int, 0644);
static int cache_check_end = 0;
module_param(cache_check_end, int, 0644);
static int cache_check_fail_halt = 0;
module_param(cache_check_fail_halt, int, 0644);

/* ==========================================================================*/
void gk_cache_clean_range(void *addr, unsigned int size)
{
#ifdef CONFIG_CPU_DCACHE_DISABLE
    return ;
#else

	u32					vstart;
	u32					vend;
#ifdef CONFIG_OUTER_CACHE
	u32					pstart;
#endif
	u32					addr_tmp;

	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;
	if (cache_check_start && (vstart != (u32)addr)) {
		pr_warn("%s start:0x%08x vs 0x%08x\n",
			__func__, vstart, (u32)addr);
		if (cache_check_fail_halt)
			BUG();
	}
	if (cache_check_end && (vend != ((u32)addr + size))) {
		pr_warn("%s start:0x%08x vs 0x%08x\n",
			__func__, vend, ((u32)addr + size));
		if (cache_check_fail_halt)
			BUG();
	}
#ifdef CONFIG_OUTER_CACHE
	pstart = gk_virt_to_phys(vstart);
#endif

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c10, 1" : : "r" (addr_tmp));
	}
	dsb();

#ifdef CONFIG_OUTER_CACHE
	outer_clean_range(pstart, (pstart + size));
#endif

#endif  //CONFIG_CPU_DCACHE_DISABLE
}
EXPORT_SYMBOL(gk_cache_clean_range);

void gk_cache_inv_range(void *addr, unsigned int size)
{
#ifdef CONFIG_CPU_DCACHE_DISABLE
    return ;
#else

	u32					vstart;
	u32					vend;
#ifdef CONFIG_OUTER_CACHE
	u32					pstart;
#endif
	u32					addr_tmp;

	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;
	if (cache_check_start && (vstart != (u32)addr)) {
		pr_warn("%s start:0x%08x vs 0x%08x\n",
			__func__, vstart, (u32)addr);
		if (cache_check_fail_halt)
			BUG();
	}
	if (cache_check_end && (vend != ((u32)addr + size))) {
		pr_warn("%s start:0x%08x vs 0x%08x\n",
			__func__, vend, ((u32)addr + size));
		if (cache_check_fail_halt)
			BUG();
	}
#ifdef CONFIG_OUTER_CACHE
	pstart = gk_virt_to_phys(vstart);
	outer_inv_range(pstart, (pstart + size));
#endif

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c6, 1" : : "r" (addr_tmp));
	}
	dsb();

#ifdef CONFIG_OUTER_CACHE
	outer_inv_range(pstart, (pstart + size));

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c6, 1" : : "r" (addr_tmp));
	}
	dsb();
#endif

#endif  //CONFIG_CPU_DCACHE_DISABLE
}
EXPORT_SYMBOL(gk_cache_inv_range);

void gk_cache_flush_range(void *addr, unsigned int size)
{
#ifdef CONFIG_CPU_DCACHE_DISABLE
    return ;
#else

	u32					vstart;
	u32					vend;
#ifdef CONFIG_OUTER_CACHE
	u32					pstart;
#endif
	u32					addr_tmp;

	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;
	if (cache_check_start && (vstart != (u32)addr)) {
		pr_warn("%s start:0x%08x vs 0x%08x\n",
			__func__, vstart, (u32)addr);
		if (cache_check_fail_halt)
			BUG();
	}
	if (cache_check_end && (vend != ((u32)addr + size))) {
		pr_warn("%s start:0x%08x vs 0x%08x\n",
			__func__, vend, ((u32)addr + size));
		if (cache_check_fail_halt)
			BUG();
	}
#ifdef CONFIG_OUTER_CACHE
	pstart = gk_virt_to_phys(vstart);

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c10, 1" : : "r" (addr_tmp));
	}
	dsb();

	outer_flush_range(pstart, (pstart + size));

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c6, 1" : : "r" (addr_tmp));
	}
	dsb();
#else
	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c14, 1" : : "r" (addr_tmp));
	}
	dsb();
#endif

#endif  //CONFIG_CPU_DCACHE_DISABLE
}
EXPORT_SYMBOL(gk_cache_flush_range);

void gk_cache_pli_range(void *addr, unsigned int size)
{
#ifdef CONFIG_CPU_DCACHE_DISABLE
    return ;
#else

	u32					vstart;
	u32					vend;
	u32					addr_tmp;

	vstart = (u32)addr & CACHE_LINE_MASK;
	vend = ((u32)addr + size + CACHE_LINE_SIZE - 1) & CACHE_LINE_MASK;

	for (addr_tmp = vstart; addr_tmp < vend; addr_tmp += CACHE_LINE_SIZE) {
#if __LINUX_ARM_ARCH__ >= 7
		__asm__ __volatile__ (
			"pli [%0]" : : "r" (addr_tmp));
#elif __LINUX_ARM_ARCH__ >= 5
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c13, 1" : : "r" (addr_tmp));
#else
#error "PLI not supported"
#endif
	}

#endif  //CONFIG_CPU_DCACHE_DISABLE
}
EXPORT_SYMBOL(gk_cache_pli_range);

#ifdef CONFIG_OUTER_CACHE
/* ==========================================================================*/
void gk_cache_l2_enable_raw()
{
#ifdef CONFIG_CPU_DCACHE_DISABLE
    return ;
#else

	if (!outer_is_enabled()) {

#ifdef CONFIG_CACHE_PL310
		if (readl(gk_cache_l2_base + L2X0_DATA_LATENCY_CTRL) !=
			0x00000120) {
			writel(0x00000120, (gk_cache_l2_base +
				L2X0_DATA_LATENCY_CTRL));
			l2x0_init(gk_cache_l2_base,
				((1 << L2X0_AUX_CTRL_ASSOCIATIVITY_SHIFT) |
				(0x1 << L2X0_AUX_CTRL_CR_POLICY_SHIFT) |
				(0x2 << L2X0_AUX_CTRL_WAY_SIZE_SHIFT) |
				(1 << L2X0_AUX_CTRL_DATA_PREFETCH_SHIFT) |
				(1 << L2X0_AUX_CTRL_INSTR_PREFETCH_SHIFT)),
				L2X0_AUX_CTRL_MASK);
		} else
#endif
			outer_enable();
	}

#endif  //CONFIG_CPU_DCACHE_DISABLE
}
EXPORT_SYMBOL(gk_cache_l2_enable_raw);

void gk_cache_l2_disable_raw()
{
#ifdef CONFIG_CPU_DCACHE_DISABLE
    return ;
#else

	flush_cache_all();
	outer_flush_all();
	outer_disable();
	outer_inv_all();
	flush_cache_all();

#endif  //CONFIG_CPU_DCACHE_DISABLE
}
EXPORT_SYMBOL(gk_cache_l2_disable_raw);

int gk_cache_l2_enable()
{
#ifdef CONFIG_CPU_DCACHE_DISABLE
    return 0;
#else

	gk_cache_l2_enable_raw();
	return outer_is_enabled() ? 0 : -1;

#endif  //CONFIG_CPU_DCACHE_DISABLE
}
EXPORT_SYMBOL(gk_cache_l2_enable);

int gk_cache_l2_disable()
{
#ifdef CONFIG_CPU_DCACHE_DISABLE
    return 0;
#else

	unsigned long flags;

	if (outer_is_enabled()) {
		disable_nonboot_cpus();
		local_irq_save(flags);
		gk_cache_l2_disable_raw();
		local_irq_restore(flags);
		enable_nonboot_cpus();
	}


	return outer_is_enabled() ? -1 : 0;

#endif  //CONFIG_CPU_DCACHE_DISABLE
}
EXPORT_SYMBOL(gk_cache_l2_disable);

/* =========================Debug Only========================================*/
int cache_l2_set_status(const char *val, const struct kernel_param *kp)
{
	int ret;

	ret = param_set_uint(val, kp);
	if (!ret) {
		if (cache_l2_status) {
			ret = gk_cache_l2_enable();
		} else {
			ret = gk_cache_l2_disable();
		}
	}

	return ret;
}

static int cache_l2_get_status(char *buffer, const struct kernel_param *kp)
{
	cache_l2_status = outer_is_enabled();

	return param_get_uint(buffer, kp);
}

static struct kernel_param_ops param_ops_cache_l2 = {
	.set = cache_l2_set_status,
	.get = cache_l2_get_status,
};
module_param_cb(cache_l2, &param_ops_cache_l2, &cache_l2_status, 0644);
#endif

