/*
 *  Copyright (C) 2009-2010, Lars-Peter Clausen <lars@metafoo.de>
 *  ingenic platform IRQ support
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General	 Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/timex.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/irqchip.h>
#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/irq_cpu.h>
#include <asm/irq.h>

static void __iomem *intc_base;
static unsigned ingenic_num_chips;

#define TRACE_IRQ        1
#define PART_OFF	0x20

#define ISR_OFF		(0x00)
#define IMR_OFF		(0x04)
#define IMSR_OFF	(0x08)
#define IMCR_OFF	(0x0c)
#define IPR_OFF		(0x10)

#define DSR0_OFF	(0x34)
#define DMR0_OFF	(0x38)
#define DPR0_OFF	(0x3c)
#define DSR1_OFF	(0x40)
#define DMR1_OFF	(0x44)
#define DPR1_OFF	(0x48)

static irqreturn_t ingenic_cascade(int irq, void *data)
{
	uint32_t irq_reg;
	unsigned i;

	for (i = 0; i < ingenic_num_chips; i++) {
		irq_reg = readl(intc_base + (i * PART_OFF) + IPR_OFF);
		if (!irq_reg)
			continue;
		generic_handle_irq(__fls(irq_reg) + (i * 32) + XBURST_SOC_IRQ_BASE);
	}

	return IRQ_HANDLED;
}

static void ingenic_irq_pm_set_mask(struct irq_chip_generic *gc, uint32_t unmask)
{
	struct irq_chip_regs *regs = &gc->chip_types->regs;

	writel(unmask, gc->reg_base + regs->enable);
	writel(~unmask, gc->reg_base + regs->disable);
}

void ingenic_irq_suspend(struct irq_data *data)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(data);
	ingenic_irq_pm_set_mask(gc, gc->wake_active);
}

void ingenic_irq_resume(struct irq_data *data)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(data);
	ingenic_irq_pm_set_mask(gc, gc->mask_cache);
}

static struct irqaction ingenic_cascade_action = {
	.handler = ingenic_cascade,
	.name = "ingenic cascade interrupt",
};

static int __init ingenic_intc_of_init(struct device_node *node, unsigned num_chips)
{
	struct irq_chip_generic *gc;
	struct irq_chip_type *ct;
	struct irq_domain *domain;
	struct resource res;
	int ret, parent_irq;
	unsigned i;

	parent_irq = irq_of_parse_and_map(node, 0);
	if (!parent_irq)
		return -EINVAL;

	ret = of_address_to_resource(node, 0, &res);
	if (ret < 0)
		return ret;

	ingenic_num_chips = num_chips;
	intc_base = ioremap(res.start,
			       ((num_chips - 1) * PART_OFF) + 0x14);

	domain = irq_domain_add_legacy(node, num_chips * 32, XBURST_SOC_IRQ_BASE, 0,
				       &irq_domain_simple_ops, NULL);
	if (!domain)
		pr_warn("unable to register IRQ domain\n");

	for (i = 0; i < num_chips; i++) {
		/* Mask all irqs */
		writel(0xffffffff, intc_base + (i * PART_OFF) + IMSR_OFF);

		gc = irq_alloc_generic_chip("INTC", 1, XBURST_SOC_IRQ_BASE + (i * 32),
					    intc_base + (i * PART_OFF),
					    handle_level_irq);
		gc->wake_enabled = IRQ_MSK(32);
		ct = gc->chip_types;
		ct->regs.enable = IMCR_OFF;
		ct->regs.disable = IMSR_OFF;
		ct->chip.irq_unmask = irq_gc_unmask_enable_reg;
		ct->chip.irq_mask = irq_gc_mask_disable_reg;
		ct->chip.irq_mask_ack = irq_gc_mask_disable_reg;
		ct->chip.irq_set_wake = irq_gc_set_wake;
		ct->chip.irq_suspend = ingenic_irq_suspend;
		ct->chip.irq_resume = ingenic_irq_resume;
#ifdef CONFIG_SPARSE_IRQ
		irq_alloc_descs(gc->irq_base, -1, gc->num_ct, of_node_to_nid(domain->of_node));
#endif
		irq_setup_generic_chip(gc, IRQ_MSK(32), 0, 0, IRQ_NOPROBE | IRQ_LEVEL);
	}

	setup_irq(parent_irq, &ingenic_cascade_action);

	/* enable cpu interrupt mask */
	set_c0_status(IE_IRQ0 | IE_IRQ1);
	return 0;
}

static int __init ingenic_intc_init(struct device_node *node,
		struct device_node *parent)
{
	return ingenic_intc_of_init(node, INTC_CHIP_NUM);
}
IRQCHIP_DECLARE(ingenic_intc, "ingenic,ingenic-intc", ingenic_intc_init);

#if 0
/*************************************************************************
if nand was config:
	After cpu was woke up,the mcu should be handler
those interruptions of GPIO0,so that it was convenient to read/write nand,
the process of GPIO's interruption as follows
	|----------------------------------|
	|               GPIO		   |
	|                 |                |
	|      |--****--INTC---------->|   |
	|      |   MASK       UNMASK   |   |
	|      CPU                    MCU  |
	|                              |   |
	|                         MAILEBOX |
	|      | <---------------------|   |
	|      |                           |
	|    ingenicdma                         |
	|      |                           |
	|   soft intc                      |
	|      |                           |
	| handler func of intc             |
	|__________________________________|

	On the contraty, when cpu will go to sleep, the cpu should be handler
GPIO0's interruptions, it was convenient to the next step, which is that cpu was
woke up.
				GPIO
				  |
		       |---------INTC-------->|
		       | unmask          mask |
		       CPU                   MCU
		       |
		       |
		 handler func of intc
************************************************************************/

#define IRQ_IS_GGPIO(irq)  (irq >= IRQ_GPIO0 && irq <= IRQ_GPIO_END)
extern void mcu_gpio_register(unsigned int ggpio, unsigned int reg);
extern void mcu_gpio_unregister(unsigned int ggpio);

void ingenic_change_irq_destination(int irq, bool cpu_to_mcu, unsigned gpio_pending_reg)
{
	int reg = 0;
	if (WARN((irq < 0 || irq > 63),
				"irq %d not support switch between cpu and mcu\n",
				irq))
		return;

	if (cpu_to_mcu) {
		if (irq < 32) {
			writel((1 << irq), intc_base + IMSR_OFF);
			if (IRQ_IS_GGPIO(irq))
				mcu_gpio_register((irq-IRQ_GPIO0), gpio_pending_reg);
			reg = readl(intc_base + DMR0_OFF);
			writel((reg & (~(1 << irq))), intc_base + DMR0_OFF);
		} else {
			writel((1 << (irq - 32)), intc_base + PART_OFF + IMSR_OFF);
			reg = readl(intc_base + DMR1_OFF);
			writel((reg & (~(1 << (irq - 32)))), intc_base + DMR1_OFF);
		}
	} else {
		if (irq < 32) {
			reg = readl(intc_base + DMR0_OFF);
			writel((reg | (1 << irq)), intc_base + DMR0_OFF);
			if (IRQ_IS_GGPIO(irq))
				mcu_gpio_unregister((irq-IRQ_GPIO0));
			writel((1 << irq), intc_base + IMCR_OFF);
		} else {
			reg = readl(intc_base + DMR1_OFF);
			writel((reg | (1 << (irq - 32))), intc_base + DMR1_OFF);
			writel((1 << (irq - 32)), intc_base + PART_OFF + IMCR_OFF);
		}
	}
	return;
}
EXPORT_SYMBOL_GPL(ingenic_change_irq_destination);
#endif
#ifdef CONFIG_DEBUG_FS
static inline void intc_seq_reg(struct seq_file *s, const char *name,
	unsigned int reg)
{
	seq_printf(s, "%s:\t\t%08x\n", name, readl(intc_base + reg));
}

static int intc_regs_show(struct seq_file *s, void *unused)
{
	intc_seq_reg(s, "Status", ISR_OFF);
	intc_seq_reg(s, "Mask", IMR_OFF);
	intc_seq_reg(s, "Pending", IPR_OFF);

	return 0;
}

static int intc_regs_open(struct inode *inode, struct file *file)
{
	return single_open(file, intc_regs_show, NULL);
}

static const struct file_operations intc_regs_operations = {
	.open		= intc_regs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init intc_debugfs_init(void)
{
	(void) debugfs_create_file("intc_regs", S_IFREG | S_IRUGO,
				NULL, NULL, &intc_regs_operations);
	return 0;
}
subsys_initcall(intc_debugfs_init);
#endif
