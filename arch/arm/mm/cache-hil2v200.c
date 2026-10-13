// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * HiSilicon L2 cache controller v200 ("hil2v200"), as found in the
 * Hi3520DV200. It is not an ARM PL310: maintenance is done by physical
 * address through the INVALID/CLEAN registers, or a whole way at a time
 * through the AUTO register, and a read of SYNC drains the controller.
 *
 * Register layout and enable sequence follow the hil2v200 driver in the
 * vendor's 3.0.8 BSP.
 */

#define pr_fmt(fmt) "hil2v200: " fmt

#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/spinlock.h>
#include <asm/hardware/cache-hil2v200.h>
#include <asm/outercache.h>
#include <asm/processor.h>

#define L2_CTRL			0x000
#define L2_AUCTRL		0x004
#define  AUCTRL_EVENT_BUS_EN	BIT(12)
#define  AUCTRL_MONITOR_EN	BIT(13)
#define L2_INTMASK		0x100
#define L2_RINT			0x108
#define  RINT_AUTO_END		BIT(14)
#define L2_INTCLR		0x10c
#define L2_SYNC			0x200
#define L2_MAINT_AUTO		0x20c
#define  MAINT_AUTO_START	BIT(0)
#define  MAINT_AUTO_CLEAN	BIT(1)
#define  MAINT_AUTO_WAY_SHIFT	2
#define L2_INVALID		0x210
#define L2_CLEAN		0x214
#define  OP_BY_ADDRESS		BIT(0)
#define L2_SPECIAL_CHECK0	0x804
#define L2_SPECIAL_CHECK1	0x808

#define L2_LINE_SIZE		32
#define L2_WAYS			8
#define L2_SIZE			SZ_128K		/* Hi3520DV200 */

static void __iomem *l2_base;
static DEFINE_RAW_SPINLOCK(l2_lock);
static bool l2_off __initdata;

/* "nol2": leave the L2 disabled, for comparisons or as an escape hatch. */
static int __init hil2_nol2(char *unused)
{
	l2_off = true;
	return 0;
}
early_param("nol2", hil2_nol2);

/* Clean or invalidate one whole way and wait for the controller to finish. */
static void hil2_way_op(unsigned int way, bool clean)
{
	u32 cmd = (way << MAINT_AUTO_WAY_SHIFT) | MAINT_AUTO_START;

	if (clean)
		cmd |= MAINT_AUTO_CLEAN;
	writel_relaxed(cmd, l2_base + L2_MAINT_AUTO);
	while (!(readl_relaxed(l2_base + L2_RINT) & RINT_AUTO_END))
		cpu_relax();
	writel_relaxed(readl_relaxed(l2_base + L2_RINT), l2_base + L2_INTCLR);
}

static void __hil2_inv_all(void)
{
	unsigned int way;

	for (way = 0; way < L2_WAYS; way++)
		hil2_way_op(way, false);
}

static void __hil2_flush_all(void)
{
	unsigned int way;

	for (way = 0; way < L2_WAYS; way++)
		hil2_way_op(way, true);
	__hil2_inv_all();
}

static void hil2_flush_all(void)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&l2_lock, flags);
	__hil2_flush_all();
	raw_spin_unlock_irqrestore(&l2_lock, flags);
}

static void hil2_range_op(unsigned long start, unsigned long end,
			  unsigned int reg)
{
	unsigned long addr, flags;

	raw_spin_lock_irqsave(&l2_lock, flags);
	for (addr = start & ~(L2_LINE_SIZE - 1); addr < end; addr += L2_LINE_SIZE)
		writel_relaxed(addr | OP_BY_ADDRESS, l2_base + reg);
	raw_spin_unlock_irqrestore(&l2_lock, flags);
}

static void hil2_inv_range(unsigned long start, unsigned long end)
{
	/* never widened to the whole cache: that would drop other dirty lines */
	hil2_range_op(start, end, L2_INVALID);
}

static void hil2_clean_range(unsigned long start, unsigned long end)
{
	unsigned long addr, flags;

	if (end - start >= L2_SIZE) {
		raw_spin_lock_irqsave(&l2_lock, flags);
		for (addr = 0; addr < L2_WAYS; addr++)
			hil2_way_op(addr, true);
		raw_spin_unlock_irqrestore(&l2_lock, flags);
		return;
	}
	hil2_range_op(start, end, L2_CLEAN);
}

static void hil2_flush_range(unsigned long start, unsigned long end)
{
	unsigned long addr, flags;

	if (end - start >= L2_SIZE) {
		hil2_flush_all();
		return;
	}
	raw_spin_lock_irqsave(&l2_lock, flags);
	for (addr = start & ~(L2_LINE_SIZE - 1); addr < end; addr += L2_LINE_SIZE) {
		writel_relaxed(addr | OP_BY_ADDRESS, l2_base + L2_CLEAN);
		writel_relaxed(addr | OP_BY_ADDRESS, l2_base + L2_INVALID);
	}
	raw_spin_unlock_irqrestore(&l2_lock, flags);
}

static void hil2_sync(void)
{
	readl_relaxed(l2_base + L2_SYNC);
}

static void hil2_disable(void)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&l2_lock, flags);
	__hil2_flush_all();
	writel_relaxed(0, l2_base + L2_CTRL);
	raw_spin_unlock_irqrestore(&l2_lock, flags);
}

int __init hil2v200_of_init(void)
{
	struct device_node *np;
	u32 aux;

	if (l2_off) {
		pr_info("disabled on the command line (nol2)\n");
		return -ENODEV;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,hi3520dv200-l2cache");
	if (!np || !of_device_is_available(np)) {
		of_node_put(np);
		return -ENODEV;
	}
	l2_base = of_iomap(np, 0);
	of_node_put(np);
	if (!l2_base)
		return -ENOMEM;

	writel_relaxed(0, l2_base + L2_CTRL);

	aux = readl_relaxed(l2_base + L2_AUCTRL);
	writel_relaxed(aux | AUCTRL_EVENT_BUS_EN | AUCTRL_MONITOR_EN,
		       l2_base + L2_AUCTRL);

	/* clear stale error state; the vendor enables all but AUTO_END */
	writel_relaxed(0, l2_base + L2_INTCLR);
	writel_relaxed(0, l2_base + L2_SPECIAL_CHECK0);
	writel_relaxed(0, l2_base + L2_SPECIAL_CHECK1);
	writel_relaxed(0x3fff, l2_base + L2_INTMASK);

	__hil2_inv_all();
	writel_relaxed(1, l2_base + L2_CTRL);

	outer_cache.inv_range = hil2_inv_range;
	outer_cache.clean_range = hil2_clean_range;
	outer_cache.flush_range = hil2_flush_range;
	outer_cache.flush_all = hil2_flush_all;
	outer_cache.sync = hil2_sync;
	outer_cache.disable = hil2_disable;

	pr_info("%u KB, %u ways, %u B lines, enabled\n",
		L2_SIZE / SZ_1K, L2_WAYS, L2_LINE_SIZE);
	return 0;
}
