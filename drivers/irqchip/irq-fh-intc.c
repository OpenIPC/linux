// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 root interrupt controller
 *
 * Copyright (C) 2026 Difan Zhang
 *
 * A Synopsys DesignWare APB interrupt controller with 64 level-triggered
 * inputs, each with separate enable and mask bits.
 */

#define pr_fmt(fmt) "fh8626-intc: " fmt

#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/spinlock.h>
#include <asm/exception.h>
#include <asm/mach/irq.h>

#define FH_INTC_ENABLE_L	0x00
#define FH_INTC_ENABLE_H	0x04
#define FH_INTC_MASK_L		0x08
#define FH_INTC_MASK_H		0x0c
#define FH_INTC_FINALSTATUS_L	0x30
#define FH_INTC_FINALSTATUS_H	0x34

#define FH_INTC_NR_IRQS		64

struct fh_intc_chip_data {
	void __iomem		*base;
	struct irq_domain	*domain;
	raw_spinlock_t		lock;
};

static struct fh_intc_chip_data fh_intc_data;

static void fh_intc_mask_irq(struct irq_data *d)
{
	struct fh_intc_chip_data *data = irq_data_get_irq_chip_data(d);
	irq_hw_number_t hw = d->hwirq;
	unsigned long flags;
	void __iomem *reg;
	u32 bit;

	reg = data->base + ((hw < 32) ? FH_INTC_MASK_L : FH_INTC_MASK_H);
	bit = BIT(hw % 32);

	raw_spin_lock_irqsave(&data->lock, flags);
	writel_relaxed(readl_relaxed(reg) | bit, reg);
	raw_spin_unlock_irqrestore(&data->lock, flags);
}

static void fh_intc_unmask_irq(struct irq_data *d)
{
	struct fh_intc_chip_data *data = irq_data_get_irq_chip_data(d);
	irq_hw_number_t hw = d->hwirq;
	unsigned long flags;
	void __iomem *reg;
	u32 bit;

	reg = data->base + ((hw < 32) ? FH_INTC_MASK_L : FH_INTC_MASK_H);
	bit = BIT(hw % 32);

	raw_spin_lock_irqsave(&data->lock, flags);
	writel_relaxed(readl_relaxed(reg) & ~bit, reg);
	raw_spin_unlock_irqrestore(&data->lock, flags);
}

static void fh_intc_enable_irq(struct irq_data *d)
{
	struct fh_intc_chip_data *data = irq_data_get_irq_chip_data(d);
	irq_hw_number_t hw = d->hwirq;
	unsigned long flags;
	void __iomem *en_reg, *mask_reg;
	u32 bit;

	en_reg = data->base + ((hw < 32) ? FH_INTC_ENABLE_L : FH_INTC_ENABLE_H);
	mask_reg = data->base + ((hw < 32) ? FH_INTC_MASK_L : FH_INTC_MASK_H);
	bit = BIT(hw % 32);

	raw_spin_lock_irqsave(&data->lock, flags);
	writel_relaxed(readl_relaxed(en_reg) | bit, en_reg);
	writel_relaxed(readl_relaxed(mask_reg) & ~bit, mask_reg);
	raw_spin_unlock_irqrestore(&data->lock, flags);
}

static void fh_intc_disable_irq(struct irq_data *d)
{
	struct fh_intc_chip_data *data = irq_data_get_irq_chip_data(d);
	irq_hw_number_t hw = d->hwirq;
	unsigned long flags;
	void __iomem *en_reg, *mask_reg;
	u32 bit;

	en_reg = data->base + ((hw < 32) ? FH_INTC_ENABLE_L : FH_INTC_ENABLE_H);
	mask_reg = data->base + ((hw < 32) ? FH_INTC_MASK_L : FH_INTC_MASK_H);
	bit = BIT(hw % 32);

	raw_spin_lock_irqsave(&data->lock, flags);
	writel_relaxed(readl_relaxed(mask_reg) | bit, mask_reg);
	writel_relaxed(readl_relaxed(en_reg) & ~bit, en_reg);
	raw_spin_unlock_irqrestore(&data->lock, flags);
}

static int fh_intc_set_type(struct irq_data *d, unsigned int type)
{
	/* Trigger type and polarity are not programmable. */
	return 0;
}

static struct irq_chip fh_intc_chip = {
	.name		= "FH_INTC",
	.irq_mask	= fh_intc_mask_irq,
	.irq_unmask	= fh_intc_unmask_irq,
	.irq_enable	= fh_intc_enable_irq,
	.irq_disable	= fh_intc_disable_irq,
	.irq_set_type	= fh_intc_set_type,
};

static int fh_intc_domain_map(struct irq_domain *d, unsigned int virq,
			      irq_hw_number_t hw)
{
	irq_set_chip_data(virq, d->host_data);
	irq_set_chip_and_handler(virq, &fh_intc_chip, handle_level_irq);
	irq_set_status_flags(virq, IRQ_LEVEL);
	irq_set_probe(virq);
	return 0;
}

static const struct irq_domain_ops fh_intc_domain_ops = {
	.map	= fh_intc_domain_map,
	.xlate	= irq_domain_xlate_onecell,
};

static void __exception_irq_entry fh_intc_handle_irq(struct pt_regs *regs)
{
	u32 stat_l = readl_relaxed(fh_intc_data.base + FH_INTC_FINALSTATUS_L);
	u32 stat_h = readl_relaxed(fh_intc_data.base + FH_INTC_FINALSTATUS_H);

	while (stat_l) {
		int hwirq = ffs(stat_l) - 1;

		handle_domain_irq(fh_intc_data.domain, hwirq, regs);
		stat_l &= ~BIT(hwirq);
	}

	while (stat_h) {
		int hwirq = ffs(stat_h) - 1;

		handle_domain_irq(fh_intc_data.domain, 32 + hwirq, regs);
		stat_h &= ~BIT(hwirq);
	}
}

static int __init fh_intc_of_init(struct device_node *np,
				  struct device_node *parent)
{
	fh_intc_data.base = of_iomap(np, 0);
	if (!fh_intc_data.base) {
		pr_err("failed to map registers\n");
		return -ENOMEM;
	}

	raw_spin_lock_init(&fh_intc_data.lock);

	/* Disable and mask all inputs left enabled by the boot loader. */
	writel_relaxed(0, fh_intc_data.base + FH_INTC_ENABLE_L);
	writel_relaxed(0, fh_intc_data.base + FH_INTC_ENABLE_H);
	writel_relaxed(0xffffffff, fh_intc_data.base + FH_INTC_MASK_L);
	writel_relaxed(0xffffffff, fh_intc_data.base + FH_INTC_MASK_H);

	fh_intc_data.domain = irq_domain_add_linear(np, FH_INTC_NR_IRQS,
						    &fh_intc_domain_ops,
						    &fh_intc_data);
	if (!fh_intc_data.domain) {
		pr_err("failed to allocate IRQ domain\n");
		iounmap(fh_intc_data.base);
		fh_intc_data.base = NULL;
		return -ENOMEM;
	}

	irq_set_default_host(fh_intc_data.domain);
	set_handle_irq(fh_intc_handle_irq);

	return 0;
}

IRQCHIP_DECLARE(fh8626v100_intc, "fullhan,fh8626v100-intc", fh_intc_of_init);
IRQCHIP_DECLARE(fh8626_intc, "fullhan,fh8626-intc", fh_intc_of_init);
