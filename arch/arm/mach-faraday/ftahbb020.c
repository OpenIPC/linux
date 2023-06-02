/*
 *  linux/arch/arm/mach-faraday/ftahbb020.c
 *
 *  Faraday FTAHBB020 Interrupt Controller
 *
 *  Copyright (C) 2020 Faraday Technology
 *  Copyright (C) 2020 Po-Yu Chuang <ratbert@faraday-tech.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <asm/io.h>

#include <asm/mach/irq.h>
#include <mach/ftahbb020.h>

struct ftahbb020_chip_data {
	unsigned int irq_offset;
	void __iomem *base;
};

#ifndef MAX_FTAHBB020_NR
#define MAX_FTAHBB020_NR	1
#endif

static struct ftahbb020_chip_data ftahbb020_data[MAX_FTAHBB020_NR];
static DEFINE_SPINLOCK(ftahbb020_lock);

/******************************************************************************
 * internal functions
 *****************************************************************************/
static inline void __iomem *ftahbb020_base(struct irq_data *d)
{
	struct ftahbb020_chip_data *chip_data = irq_data_get_irq_chip_data(d);
	return chip_data->base;
}

/*
 * return hardware irq number
 */
static inline unsigned int ftahbb020_irq(struct irq_data *d)
{
	struct ftahbb020_chip_data *chip_data = irq_data_get_irq_chip_data(d);
	return d->irq - chip_data->irq_offset;
}

static inline void ftahbb020_clear_irq(void __iomem *base, unsigned int hw_irq)
{
	unsigned int mask = 1 << hw_irq;

	writel(mask, base + FTAHBB020_OFFSET_S2P_XIRQCLEAR);
}

static inline void ftahbb020_mask_irq(void __iomem *base, unsigned int hw_irq)
{
	unsigned int mask;

	mask = readl(base + FTAHBB020_OFFSET_S2P_XIRQMASK);
	mask |= 1 << hw_irq;
	writel(mask, base + FTAHBB020_OFFSET_S2P_XIRQMASK);
}

static inline void ftahbb020_unmask_irq(void __iomem *base, unsigned int hw_irq)
{
	unsigned int mask;

	mask = readl(base + FTAHBB020_OFFSET_S2P_XIRQMASK);
	mask &= ~(1 << hw_irq);
	writel(mask, base + FTAHBB020_OFFSET_S2P_XIRQMASK);
}

static inline void ftahbb020_set_trig_mode(void __iomem *base, unsigned int hw_irq,
	int mode)
{
	unsigned int irqmode;

	irqmode = readl(base + FTAHBB020_OFFSET_S2P_XIRQMODE);

	/*
	 * 0: edge trigger
	 * 1: level trigger
	 */
	if (mode)
		irqmode |= 1 << hw_irq;
	else
		irqmode &= ~(1 << hw_irq);

	writel(irqmode, base + FTAHBB020_OFFSET_S2P_XIRQMODE);
}

static inline void ftahbb020_set_trig_level(void __iomem *base, unsigned int hw_irq,
	int level)
{
	unsigned int irqlevel;

	irqlevel = readl(base + FTAHBB020_OFFSET_S2P_XIRQLEVEL);

	/*
	 * 0: active-high level trigger / rising edge trigger
	 * 1: active-low level trigger / falling edge trigger
	 */
	if (level)
		irqlevel |= 1 << hw_irq;
	else
		irqlevel &= ~(1 << hw_irq);

	writel(irqlevel, base + FTAHBB020_OFFSET_S2P_XIRQLEVEL);
}

static inline void ftahbb020_set_trig_both(void __iomem *base, unsigned int hw_irq,
	int both)
{
	unsigned int irqboth;

	irqboth = readl(base + FTAHBB020_OFFSET_S2P_XIRQBOTH);

	if (both)
		irqboth |= 1 << hw_irq;
	else
		irqboth &= ~(1 << hw_irq);

	writel(irqboth, base + FTAHBB020_OFFSET_S2P_XIRQBOTH);
}

/******************************************************************************
 * struct irq_chip functions
 *****************************************************************************/
static int ftahbb020_set_type(struct irq_data *d, unsigned int type)
{
	unsigned int hw_irq = ftahbb020_irq(d);
	void __iomem *base = ftahbb020_base(d);
	int mode = 0;
	int both = 0;
	int level = 0;

	switch (type) {
	case IRQ_TYPE_LEVEL_LOW:
		level = 1;
		/* fall through */

	case IRQ_TYPE_LEVEL_HIGH:
		mode = 1;
		break;

	case IRQ_TYPE_EDGE_FALLING:
		level = 1;
		/* fall through */

	case IRQ_TYPE_EDGE_RISING:
		break;

	case IRQ_TYPE_EDGE_BOTH:
		both = 1;
		break;

	default:
		return -EINVAL;
	}

	spin_lock(&ftahbb020_lock);
	ftahbb020_set_trig_mode(base, hw_irq, mode);
	ftahbb020_set_trig_both(base, hw_irq, both);
	ftahbb020_set_trig_level(base, hw_irq, level);
	spin_unlock(&ftahbb020_lock);
	return 0;
}

/*
 * Edge trigger IRQ chip methods
 */
static void ftahbb020_edge_ack(struct irq_data *d)
{
	unsigned int hw_irq = ftahbb020_irq(d);
	void __iomem *base = ftahbb020_base(d);

	spin_lock(&ftahbb020_lock);
	ftahbb020_clear_irq(base, hw_irq);
	spin_unlock(&ftahbb020_lock);
}

static void ftahbb020_mask(struct irq_data *d)
{
	unsigned int hw_irq = ftahbb020_irq(d);
	void __iomem *base = ftahbb020_base(d);

	spin_lock(&ftahbb020_lock);
	ftahbb020_mask_irq(base, hw_irq);
	spin_unlock(&ftahbb020_lock);
}

static void ftahbb020_unmask(struct irq_data *d)
{
	unsigned int hw_irq = ftahbb020_irq(d);
	void __iomem *base = ftahbb020_base(d);

	spin_lock(&ftahbb020_lock);
	ftahbb020_unmask_irq(base, hw_irq);
	spin_unlock(&ftahbb020_lock);
}

static struct irq_chip ftahbb020_edge_chip = {
	.irq_ack	= ftahbb020_edge_ack,
	.irq_mask	= ftahbb020_mask,
	.irq_unmask	= ftahbb020_unmask,
	.irq_set_type	= ftahbb020_set_type,
};

/*
 * Level trigger IRQ chip methods
 */
static void ftahbb020_level_ack(struct irq_data *d)
{
	/* do nothing */
}

static struct irq_chip ftahbb020_level_chip = {
	.irq_ack	= ftahbb020_level_ack,
	.irq_mask	= ftahbb020_mask,
	.irq_unmask	= ftahbb020_unmask,
	.irq_set_type	= ftahbb020_set_type,
};

/******************************************************************************
 * initialization functions
 *****************************************************************************/
static void ftahbb020_handle_cascade_irq(unsigned int irq, struct irq_desc *desc)
{
	struct ftahbb020_chip_data *chip_data = irq_get_handler_data(irq);
	struct irq_chip *chip = irq_get_chip(irq);
	unsigned int cascade_irq, hw_irq;
	unsigned long status;
	unsigned long mask;

	/* primary controller ack'ing */
	chip->irq_mask(&desc->irq_data);
	chip->irq_ack(&desc->irq_data);

	spin_lock(&ftahbb020_lock);
	status = readl(chip_data->base + FTAHBB020_OFFSET_S2P_XIRQSRC);
	mask = readl(chip_data->base + FTAHBB020_OFFSET_S2P_XIRQMASK);
	spin_unlock(&ftahbb020_lock);

	status &= ~mask;
	if (!status)
		goto out;

	hw_irq = ffs(status) - 1;

	cascade_irq = hw_irq + chip_data->irq_offset;
	generic_handle_irq(cascade_irq);

out:
	/* primary controller unmasking */
	chip->irq_unmask(&desc->irq_data);
}

void __init ftahbb020_cascade_irq(unsigned int ftahbb020_nr, unsigned int irq)
{
	if (ftahbb020_nr >= MAX_FTAHBB020_NR)
		BUG();
	if (irq_set_handler_data(irq, &ftahbb020_data[ftahbb020_nr]) != 0)
		BUG();

	irq_set_chained_handler(irq, ftahbb020_handle_cascade_irq);
}

int ftahbb020_set_irq_type(unsigned int irq, unsigned int type)
{
	switch (type) {
	case IRQ_TYPE_LEVEL_LOW:
	case IRQ_TYPE_LEVEL_HIGH:
		irq_set_chip(irq, &ftahbb020_level_chip);
		irq_set_handler(irq, handle_level_irq);
		break;

	case IRQ_TYPE_EDGE_BOTH:
	case IRQ_TYPE_EDGE_FALLING:
	case IRQ_TYPE_EDGE_RISING:
		irq_set_chip(irq, &ftahbb020_edge_chip);
		irq_set_handler(irq, handle_edge_irq);
		break;

	default:
		return -EINVAL;
	}

	return irq_set_irq_type(irq, type);
}

/*
 * Initialization of external interrupt controller.
 */
void __init ftahbb020_init(unsigned int ftahbb020_nr, void __iomem *base,
			   unsigned int irq_start)
{
	unsigned int irq;
	unsigned int irq_end;

	if (ftahbb020_nr >= MAX_FTAHBB020_NR)
		BUG();

	ftahbb020_data[ftahbb020_nr].base = base;
	ftahbb020_data[ftahbb020_nr].irq_offset = irq_start;

	/*
	 * mask all external interrupts
	 */
	writel(~0, base + FTAHBB020_OFFSET_S2P_XIRQMASK);
	writel(~0, base + FTAHBB020_OFFSET_S2P_XIRQCLEAR);

	/*
	 * initial trigger mode and level
	 */
	writel(0, base + FTAHBB020_OFFSET_S2P_XIRQMODE);
	writel(0, base + FTAHBB020_OFFSET_S2P_XIRQBOTH);
	writel(0, base + FTAHBB020_OFFSET_S2P_XIRQLEVEL);

	/*
	 * enable all external interrupts
	 */
	writel(~0, base + FTAHBB020_OFFSET_S2P_XIRQENABLE);

	/*
	 * setup the linux irq subsystem
	 */
	irq_end = irq_start + 32;
	for (irq = irq_start; irq < irq_end; irq++) {
		irq_set_chip_data(irq, &ftahbb020_data[ftahbb020_nr]);
		set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
		irq_set_chip(irq, &ftahbb020_level_chip);
		irq_set_handler(irq, handle_level_irq);
		irq_set_irq_type(irq, IRQ_TYPE_LEVEL_HIGH);
	}
}
