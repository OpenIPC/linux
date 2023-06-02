/*
 * FH irq subsystem
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/stmp_device.h>
#include <asm/exception.h>

#include <linux/irqchip.h>

#define		OFFSET_IRQ_EN_LOW				(0x0000)
#define		OFFSET_IRQ_EN_HIGH				(0x0004)
#define		OFFSET_IRQ_IRQMASK_LOW			(0x0008)
#define		OFFSET_IRQ_IRQMASK_HIGH			(0x000C)
#define		OFFSET_IRQ_IRQFORCE_LOW			(0x0010)
#define		OFFSET_IRQ_IRQFORCE_HIGH		(0x0014)
#define		OFFSET_IRQ_RAWSTATUS_LOW		(0x0018)
#define		OFFSET_IRQ_RAWSTATUS_HIGH		(0x001C)
#define		OFFSET_IRQ_STATUS_LOW			(0x0020)
#define		OFFSET_IRQ_STATUS_HIGH			(0x0024)
#define		OFFSET_IRQ_MASKSTATUS_LOW		(0x0028)
#define		OFFSET_IRQ_MASKSTATUS_HIGH		(0x002C)
#define		OFFSET_IRQ_FINSTATUS_LOW		(0x0030)
#define		OFFSET_IRQ_FINSTATUS_HIGH		(0x0034)
#define		OFFSET_FIQ_EN_LOW				(0x02C0)
#define		OFFSET_FIQ_EN_HIGH				(0x02C4)
#define		OFFSET_FIQ_FIQMASK_LOW			(0x02C8)
#define		OFFSET_FIQ_FIQMASK_HIGH			(0x02CC)
#define		OFFSET_FIQ_FIQFORCE_LOW			(0x02D0)
#define		OFFSET_FIQ_FIQFORCE_HIGH		(0x02D4)
#define		OFFSET_FIQ_RAWSTATUS_LOW		(0x02D8)
#define		OFFSET_FIQ_RAWSTATUS_HIGH		(0x02DC)
#define		OFFSET_FIQ_STATUS_LOW			(0x02E0)
#define		OFFSET_FIQ_STATUS_HIGH			(0x02E4)
#define		OFFSET_FIQ_FINSTATUS_LOW		(0x02E8)
#define		OFFSET_FIQ_FINSTATUS_HIGH		(0x02EC)

#define FH_INTC_NUM_IRQS		64

static void __iomem *fh_intc_base;
static struct irq_domain *fh_intc_domain;

static int fh_intc_set_type(struct irq_data *d, unsigned int type)
{
	// only support level high
	irqd_set_trigger_type(d, IRQF_TRIGGER_HIGH);
	return 0;
}

static void fh_intc_enable_irq(struct irq_data *d)
{
	u32 reg;

	if (d->hwirq > 31) {
		reg = __raw_readl(fh_intc_base + OFFSET_IRQ_EN_HIGH);
		reg |= 1 << (d->hwirq - 32);
		__raw_writel(reg, fh_intc_base + OFFSET_IRQ_EN_HIGH);

	} else {
		reg = __raw_readl(fh_intc_base + OFFSET_IRQ_EN_LOW);
		reg |= 1 << d->hwirq;
		__raw_writel(reg, fh_intc_base + OFFSET_IRQ_EN_LOW);
	}
}

static void fh_intc_disable_irq(struct irq_data *d)
{
	u32 reg;

	if (d->hwirq > 31) {
		reg = __raw_readl(fh_intc_base + OFFSET_IRQ_EN_HIGH);
		reg &= ~(1 << (d->hwirq - 32));
		__raw_writel(reg, fh_intc_base + OFFSET_IRQ_EN_HIGH);

	} else {
		reg = __raw_readl(fh_intc_base + OFFSET_IRQ_EN_LOW);
		reg &= ~(1 << d->hwirq);
		__raw_writel(reg, fh_intc_base + OFFSET_IRQ_EN_LOW);
	}
}

static void fh_intc_mask_irq(struct irq_data *d)
{
	u32 reg;

	if (d->hwirq > 31) {
		reg = __raw_readl(fh_intc_base + OFFSET_IRQ_IRQMASK_HIGH);
		reg |= 1 << (d->hwirq - 32);
		__raw_writel(reg, fh_intc_base + OFFSET_IRQ_IRQMASK_HIGH);

	} else {
		reg = __raw_readl(fh_intc_base + OFFSET_IRQ_IRQMASK_LOW);
		reg |= 1 << d->hwirq;
		__raw_writel(reg, fh_intc_base + OFFSET_IRQ_IRQMASK_LOW);
	}
}

static void fh_intc_unmask_irq(struct irq_data *d)
{
	u32 reg;

	if (d->hwirq > 31) {
		reg = __raw_readl(fh_intc_base + OFFSET_IRQ_IRQMASK_HIGH);
		reg &= ~(1 << (d->hwirq - 32));
		__raw_writel(reg, fh_intc_base + OFFSET_IRQ_IRQMASK_HIGH);

	} else {
		reg = __raw_readl(fh_intc_base + OFFSET_IRQ_IRQMASK_LOW);
		reg &= ~(1 << d->hwirq);
		__raw_writel(reg, fh_intc_base + OFFSET_IRQ_IRQMASK_LOW);
	}
}

static struct irq_chip fh_intc_chip = {
	.irq_mask = fh_intc_mask_irq,
	.irq_unmask = fh_intc_unmask_irq,
	.irq_enable = fh_intc_enable_irq,
	.irq_disable = fh_intc_disable_irq,
	.irq_set_type = fh_intc_set_type,

};

asmlinkage void __exception_irq_entry fh_intc_handle_irq(struct pt_regs *regs)
{
	u32 irqnr;

	irqnr = find_first_bit(fh_intc_base + OFFSET_IRQ_FINSTATUS_LOW, 64);

	handle_domain_irq(fh_intc_domain, irqnr, regs);

}

static int fh_irq_domain_map(struct irq_domain *d, unsigned int virq,
				irq_hw_number_t hw)
{
	struct irq_data *data;


	irq_set_chip_and_handler(virq, &fh_intc_chip, handle_level_irq);
	/* give a default trigger type */
	data = irq_domain_get_irq_data(d, virq);
	irqd_set_trigger_type(data, IRQF_TRIGGER_HIGH);

	return 0;
}

static struct irq_domain_ops fh_irq_domain_ops = {
	.map = fh_irq_domain_map,
	.xlate = irq_domain_xlate_onecell,
};
static void fh_intc_hw_init(void)
{
	WARN_ON(!fh_intc_base);

	__raw_writel(0, fh_intc_base + OFFSET_IRQ_EN_LOW);
	__raw_writel(0, fh_intc_base + OFFSET_IRQ_EN_HIGH);
}

static void fh_intc_create_default_handler(void)
{
	irq_set_default_host(fh_intc_domain);
	set_handle_irq(fh_intc_handle_irq);
}

static int __init fh_intc_of_init(struct device_node *np,
			  struct device_node *interrupt_parent)
{
	fh_intc_base = of_iomap(np, 0);

	fh_intc_hw_init();

	fh_intc_domain = irq_domain_add_linear(np, FH_INTC_NUM_IRQS,
					     &fh_irq_domain_ops, NULL);

	fh_intc_create_default_handler();

	return fh_intc_domain ? 0 : -ENODEV;
}
IRQCHIP_DECLARE(fh, "fh,fh-intc", fh_intc_of_init);


#ifndef CONFIG_USE_OF
void __init fh_intc_init_no_of(unsigned int iovbase)
{
	int irq_base;

	fh_intc_base = (void *)iovbase;

	fh_intc_hw_init();

	irq_base = irq_alloc_descs(-1, 0, FH_INTC_NUM_IRQS, 0);

	if (irq_base < 0)
		irq_base = 0;


	/* create a legacy host */
	fh_intc_domain = irq_domain_add_legacy(NULL, FH_INTC_NUM_IRQS,
					irq_base, 0, &fh_irq_domain_ops, NULL);


	fh_intc_create_default_handler();
}
#endif
