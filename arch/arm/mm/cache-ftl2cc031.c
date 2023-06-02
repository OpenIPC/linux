/*
 * arch/arm/mm/cache-ftl2cc031.c - FTL2CC031 cache controller support
 *
 * Copyright (C) 2010-2012 Faraday Corp. Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/interrupt.h>
#include <asm/cacheflush.h>
#include <asm/hardware/cache-ftl2cc031.h>
#include <mach/platform/board.h>

#define CACHE_LINE_SIZE		32
#define MAX_LINE_COUNT		1024	//b[9:0]

static void __iomem *l2x0_base;
static DEFINE_RAW_SPINLOCK(l2x0_lock);
static uint32_t l2x0_way_mask;	/* Bitmask of active ways */
static uint32_t l2x0_size;

static irqreturn_t l2cache_handle_irq(int irq, void *dev_id);

static inline void flib_cache_wait(void __iomem *reg, unsigned long mask)
{
	/* wait for cache operation by line or way to complete */
	while (readl_relaxed(reg) & mask)
		cpu_relax();
}

static inline void flib_cache_sync(void)
{
	void __iomem *base = l2x0_base;
	writel_relaxed(L2X0_CACHE_SYNC, base + L2X0_CACHE_MAINTENANCE);
	flib_cache_wait(base + L2X0_CACHE_MAINTENANCE, L2X0_CACHE_SYNC);
}

static inline void fa_l2x0_clean_PA(unsigned long addr, unsigned long line_cnt)
{
	void __iomem *base = l2x0_base;
	if (line_cnt == 0)
		return;

	writel_relaxed(line_cnt-1, base + L2X0_CACHE_MAINTENAN_CNT);
	writel_relaxed(addr, base + L2X0_CACHE_MAINTENAN_PA);
	writel_relaxed(L2X0_CLEAN_PA, base + L2X0_CACHE_MAINTENANCE);
	flib_cache_wait(base + L2X0_CACHE_MAINTENANCE, L2X0_CLEAN_PA);
}

static inline void fa_l2x0_inv_PA(unsigned long addr, unsigned long line_cnt)
{
	void __iomem *base = l2x0_base;
	if (line_cnt == 0)
		return;

	writel_relaxed(line_cnt-1, base + L2X0_CACHE_MAINTENAN_CNT);
	writel_relaxed(addr, base + L2X0_CACHE_MAINTENAN_PA);
	writel_relaxed(L2X0_INV_PA, base + L2X0_CACHE_MAINTENANCE);
	flib_cache_wait(base + L2X0_CACHE_MAINTENANCE, L2X0_INV_PA);
}

#define fa_l2x0_set_debug	NULL

static inline void fa_l2x0_flush_PA(unsigned long addr, unsigned long line_cnt)
{
	void __iomem *base = l2x0_base;
	if (line_cnt == 0)
		return;

	writel_relaxed(line_cnt-1, base + L2X0_CACHE_MAINTENAN_CNT);
	writel_relaxed(addr, base + L2X0_CACHE_MAINTENAN_PA);
	writel_relaxed(L2X0_CLEAN_INV_PA, base + L2X0_CACHE_MAINTENANCE);
	flib_cache_wait(base + L2X0_CACHE_MAINTENANCE, L2X0_CLEAN_INV_PA);
}

///////////////////////////////////////////////////////////////////
static void fa_l2x0_cache_sync(void)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	flib_cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}


static void __fa_l2x0_flush_all(void)
{
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_WAY_IDX);
	writel_relaxed(L2X0_CLEAN_INV_WAY, l2x0_base + L2X0_CACHE_MAINTENANCE);
	flib_cache_wait(l2x0_base + L2X0_CACHE_MAINTENANCE, L2X0_CLEAN_INV_WAY);
	//flib_cache_sync();
}

static void fa_l2x0_flush_all(void)
{
	unsigned long flags;

	/* clean and invalid all ways */
	raw_spin_lock_irqsave(&l2x0_lock, flags);
	__fa_l2x0_flush_all();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void fa_l2x0_clean_all(void)
{
	unsigned long flags;

	/* clean all ways */
	raw_spin_lock_irqsave(&l2x0_lock, flags);
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_WAY_IDX);
	writel_relaxed(L2X0_CLEAN_WAY, l2x0_base + L2X0_CACHE_MAINTENANCE);
	flib_cache_wait(l2x0_base + L2X0_CACHE_MAINTENANCE, L2X0_CLEAN_WAY);
	flib_cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void fa_l2x0_inv_all(void)
{
	unsigned long flags;

	/* invalidate all ways */
	raw_spin_lock_irqsave(&l2x0_lock, flags);
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_WAY_IDX);
	writel_relaxed(L2X0_INV_WAY, l2x0_base + L2X0_CACHE_MAINTENANCE);
	flib_cache_wait(l2x0_base + L2X0_CACHE_MAINTENANCE, L2X0_INV_WAY);
	//flib_cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void fa_l2x0_inv_range(unsigned long start, unsigned long end)
{
	unsigned long flags;
	unsigned long cnt;

//	printk("Faraday FTL2CC031 l2x0_invalid_range()\n");

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	if (start & (CACHE_LINE_SIZE - 1)) {
		start &= ~(CACHE_LINE_SIZE - 1);
		fa_l2x0_flush_PA(start, 1);
		start += CACHE_LINE_SIZE;
	}

	if (end & (CACHE_LINE_SIZE - 1)) {
		end &= ~(CACHE_LINE_SIZE - 1);
		fa_l2x0_flush_PA(end, 1);
	}

	cnt = (end + CACHE_LINE_SIZE -1 -start)/CACHE_LINE_SIZE;
	if (cnt <= MAX_LINE_COUNT)
		fa_l2x0_inv_PA(start, cnt);
	else {
		do {
			fa_l2x0_inv_PA(start, MAX_LINE_COUNT);
			start += (MAX_LINE_COUNT*CACHE_LINE_SIZE);
			cnt -= MAX_LINE_COUNT;
			if (cnt <= MAX_LINE_COUNT) {
				fa_l2x0_inv_PA(start, cnt);
				cnt = 0;
			}
		} while (cnt > 0);
	}

	//flib_cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void fa_l2x0_clean_range(unsigned long start, unsigned long end)
{
	unsigned long flags;
	unsigned long cnt;

//	printk("Faraday FTL2CC031 l2x0_clean_range()\n");

	if ((end - start) >= l2x0_size) {
		fa_l2x0_clean_all();
		return;
	}

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	start &= ~(CACHE_LINE_SIZE - 1);
	cnt = (end + CACHE_LINE_SIZE -1 -start)/CACHE_LINE_SIZE;
	if (cnt <= MAX_LINE_COUNT)
		fa_l2x0_clean_PA(start, cnt);
	else {
		do {
			fa_l2x0_clean_PA(start, MAX_LINE_COUNT);
			start += (MAX_LINE_COUNT*CACHE_LINE_SIZE);
			cnt -= MAX_LINE_COUNT;
			if (cnt <= MAX_LINE_COUNT) {
				fa_l2x0_clean_PA(start, cnt);
				cnt = 0;
			}
		} while (cnt > 0);
	}

	//flib_cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void fa_l2x0_flush_range(unsigned long start, unsigned long end)
{
	unsigned long flags;
	unsigned long cnt;

//	printk("Faraday FTL2CC031 l2x0_flush_range()\n");

	if ((end - start) >= l2x0_size) {
		fa_l2x0_flush_all();
		return;
	}

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	start &= ~(CACHE_LINE_SIZE - 1);
	cnt = (end + CACHE_LINE_SIZE -1 -start)/CACHE_LINE_SIZE;
	if (cnt <= MAX_LINE_COUNT)
		fa_l2x0_flush_PA(start, cnt);
	else {
		do {
			fa_l2x0_flush_PA(start, MAX_LINE_COUNT);
			start += (MAX_LINE_COUNT*CACHE_LINE_SIZE);
			cnt -= MAX_LINE_COUNT;
			if (cnt <= MAX_LINE_COUNT) {
				fa_l2x0_flush_PA(start, cnt);
				cnt = 0;
			}
		} while (cnt > 0);
	}

	//flib_cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void fa_l2x0_disable(void)
{
        unsigned long flags;

        raw_spin_lock_irqsave(&l2x0_lock, flags);
        __fa_l2x0_flush_all();
        writel_relaxed(0, l2x0_base + L2X0_CTRL);
        dsb();
        raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void fa_l2x0_unlock(void)
{
	writel_relaxed(0x0, l2x0_base + L2X0_LOCKDOWN_WAY_D);
	writel_relaxed(0x0, l2x0_base + L2X0_LOCKDOWN_WAY_I);
}

void __init ftl2cc031_init(void __iomem *base, __u32 aux_val, __u32 aux_mask)
{
	__u32 aux;
	__u32 way_size = 0;
	int ways, ret;

	l2x0_base = base;

	aux = readl_relaxed(l2x0_base + L2X0_AUX_CTRL);
	aux &= ~aux_mask;
	aux |= aux_val;

	if (aux & AUX_WAYNUM_16)
		ways = 16;
	else
		ways = 8;

	l2x0_way_mask = (1 << ways) -1;

	/*
	 * L2 cache Size = Way size * Number of ways
	 */
	way_size = (aux & AUX_WAYSIZE_MASK) >> 16;
	way_size = 1 << (way_size + 4);
	l2x0_size = ways * way_size * SZ_1K;

	/*
	 * Check if L2CC controller is already enabled.
	 * If you are booting from non-secure mode
	 * accessing the below registers will fault.
	 */
	if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & CTRL_L2CC_EN)) {
		/* Make sure that I&D is not locked down when starting */
		fa_l2x0_unlock();

		/* L2CC controller is disabled */
		writel_relaxed(aux, l2x0_base + L2X0_AUX_CTRL);

		/* Tag RAM initialization */
		writel_relaxed(CTRL_TAG_INIT, l2x0_base + L2X0_CTRL);
		/* wait for the operation to complete */
		while (readl_relaxed(l2x0_base + L2X0_CTRL) & CTRL_TAG_INIT)
			;
		/* enable L2CC */
		writel_relaxed(CTRL_L2CC_EN, l2x0_base + L2X0_CTRL);

        /* when l2 cache is enabled, the controller will invalidate all ways */
		//fa_l2x0_inv_all();
	}

    /* outer_cache is an extern variable in outercache.h of arch/arm/include/asm */
	outer_cache.inv_range = fa_l2x0_inv_range;
	outer_cache.clean_range = fa_l2x0_clean_range;
	outer_cache.flush_range = fa_l2x0_flush_range;
	outer_cache.sync = fa_l2x0_cache_sync;
	outer_cache.flush_all = fa_l2x0_flush_all;
	outer_cache.inv_all = fa_l2x0_inv_all;
	outer_cache.disable = fa_l2x0_disable;
	outer_cache.set_debug = fa_l2x0_set_debug;

	printk(KERN_INFO "Faraday FTL2CC031 cache controller enabled\n");
	printk(KERN_INFO "%d ways, AUX_CTRL 0x%08x, Cache size: %d B\n",
			ways, aux, l2x0_size);

    ret = request_irq(L2CACHE_FTL2CC031_IRQ, l2cache_handle_irq, 0, "l2_cache", NULL);
    if (ret < 0)
        panic("%s, request irq %d fail! \n", __func__, L2CACHE_FTL2CC031_IRQ);

    writel_relaxed(0x3, l2x0_base + L2X0_INTR_MASK);    /* Tag parity error/Data parity error */
}

/*
 * Interrupt handler while error happen
 */
static irqreturn_t l2cache_handle_irq(int irq, void *dev_id)
{
    volatile u32 value;

    value = readl_relaxed(l2x0_base + L2X0_RAW_INTR_SRC);
    writel_relaxed(value, l2x0_base + L2X0_INTR_CLEAR);

    while (value & 0x3) {   /* Tag parity error/Data parity error */
        if (value & 0x1)
            printk("L2 cache tag ram error! \n");
        if (value & 0x2)
            printk("L2 cache data ram error! \n");
    }

    return IRQ_HANDLED;
}
