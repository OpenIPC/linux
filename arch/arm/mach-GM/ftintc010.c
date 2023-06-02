/*
 *  linux/arch/arm/mach-GM/ftintc010.c
 *
 *  Faraday FTINTC010 Interrupt Controller
 *
 *  Copyright (C) 2009 Po-Yu Chuang
 *  Copyright (C) 2009 Faraday Corp. (http://www.faraday-tech.com)
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
#include <mach/ftintc010.h>

struct ftintc010_chip_data {
	unsigned int irq_offset;
	void __iomem *base;
};

#ifndef MAX_FTINTC010_NR
#define MAX_FTINTC010_NR	1
#endif

static struct ftintc010_chip_data ftintc010_data[MAX_FTINTC010_NR];
static DEFINE_SPINLOCK(ftintc010_lock);

/******************************************************************************
 * internal functions
 *****************************************************************************/
static inline void __iomem *ftintc010_base(unsigned int irq)
{
	struct ftintc010_chip_data *chip_data = irq_get_chip_data(irq);
	
	return chip_data->base;
}

/*
 * return hardware irq number
 */
static inline unsigned int ftintc010_irq(unsigned int irq)
{
	struct ftintc010_chip_data *chip_data = irq_get_chip_data(irq);
	
	return irq - chip_data->irq_offset;
}

static inline void ftintc010_clear_irq(void __iomem *base, unsigned int hw_irq)
{
	unsigned int mask;

	if (hw_irq < 32) {
		mask = 1 << hw_irq;

		writel(mask, base + FTINTC010_OFFSET_IRQCLEAR);
	} else {
		mask = 1 << (hw_irq - 32);
		writel(mask, base + FTINTC010_OFFSET_IRQCLEAREX);
	}
}

static inline void ftintc010_mask_irq(void __iomem *base, unsigned int hw_irq)
{
	unsigned int mask;

	/*
	 * 0: masked
	 * 1: unmasked
	 */
	if (hw_irq < 32) {
		mask = readl(base + FTINTC010_OFFSET_IRQMASK);
		mask &= ~(1 << hw_irq);
		writel(mask, base + FTINTC010_OFFSET_IRQMASK);
	} else {
		mask = readl(base + FTINTC010_OFFSET_IRQMASKEX);
		mask &= ~(1 << (hw_irq - 32));
		writel(mask, base + FTINTC010_OFFSET_IRQMASKEX);
	}
}

static inline void ftintc010_unmask_irq(void __iomem *base, unsigned int hw_irq)
{
	unsigned int mask;

	/*
	 * 0: masked
	 * 1: unmasked
	 */
	if (hw_irq < 32) {
		mask = readl(base + FTINTC010_OFFSET_IRQMASK);
		mask |= 1 << hw_irq;
		writel(mask, base + FTINTC010_OFFSET_IRQMASK);
	} else {
		mask = readl(base + FTINTC010_OFFSET_IRQMASKEX);
		mask |= 1 << (hw_irq - 32);
		writel(mask, base + FTINTC010_OFFSET_IRQMASKEX);
	}
}

/******************************************************************************
 * struct irq_chip functions
 *****************************************************************************/
static inline void ftintc010_set_trig_mode(void __iomem *base, unsigned int hw_irq, int mode)
{
	unsigned int irqmode, fiqmode;

	/*
	 * 0: level trigger
	 * 1: edge trigger
	 */
	if (hw_irq < 32) {
		irqmode = readl(base + FTINTC010_OFFSET_IRQMODE);
		if (mode)
			irqmode |= 1 << hw_irq;
		else
			irqmode &= ~(1 << hw_irq);
		writel(irqmode, base + FTINTC010_OFFSET_IRQMODE);
	} else {
		irqmode = readl(base + FTINTC010_OFFSET_IRQMODEEX);
		if (mode)
			irqmode |= 1 << (hw_irq - 32);
		else
			irqmode &= ~(1 << (hw_irq - 32));
		writel(irqmode, base + FTINTC010_OFFSET_IRQMODEEX);
	}
	
	/* fiq */
	if (hw_irq < 32) {
	    fiqmode = readl(base + FTINTC010_OFFSET_FIQMODE);
		if (mode)
			fiqmode |= 1 << hw_irq;
		else
			fiqmode &= ~(1 << hw_irq);
		writel(fiqmode, base + FTINTC010_OFFSET_FIQMODE);
	} else {
	    fiqmode = readl(base + FTINTC010_OFFSET_FIQMODEEX);
		if (mode)
			fiqmode |= 1 << (hw_irq - 32);
		else
			fiqmode &= ~(1 << (hw_irq - 32));
		writel(fiqmode, base + FTINTC010_OFFSET_FIQMODEEX);
	}
}

static inline void ftintc010_set_trig_level(void __iomem *base, unsigned int hw_irq, int level)
{
	unsigned int irqlevel, fiqlevel;

	/*
	 * 0: active-high level trigger / rising edge trigger
	 * 1: active-low level trigger / falling edge trigger
	 */
	if (hw_irq < 32) {
		irqlevel = readl(base + FTINTC010_OFFSET_IRQLEVEL);
		if (level)
			irqlevel |= 1 << hw_irq;
		else
			irqlevel &= ~(1 << hw_irq);
		writel(irqlevel, base + FTINTC010_OFFSET_IRQLEVEL);
	} else {
		irqlevel = readl(base + FTINTC010_OFFSET_IRQLEVELEX);
		if (level)
			irqlevel |= 1 << (hw_irq - 32);
		else
			irqlevel &= ~(1 << (hw_irq - 32));
		writel(irqlevel, base + FTINTC010_OFFSET_IRQLEVELEX);
	}
	
	/* fiq */
	if (hw_irq < 32) {
		fiqlevel = readl(base + FTINTC010_OFFSET_FIQLEVEL);
		if (level)
			fiqlevel |= 1 << hw_irq;
		else
			fiqlevel &= ~(1 << hw_irq);
		writel(irqlevel, base + FTINTC010_OFFSET_FIQLEVEL);
	} else {
		fiqlevel = readl(base + FTINTC010_OFFSET_FIQLEVELEX);
		if (level)
			fiqlevel |= 1 << (hw_irq - 32);
		else
			irqlevel &= ~(1 << (hw_irq - 32));
		writel(fiqlevel, base + FTINTC010_OFFSET_FIQLEVELEX);
	}
}

static int ftintc010_set_type(struct irq_data *data, unsigned int type)
{
	unsigned int hw_irq = ftintc010_irq(data->irq);
	void __iomem *base = ftintc010_base(data->irq);
	int mode = 0;
	int level = 0;

	switch (type) {
	case IRQ_TYPE_LEVEL_LOW:
		level = 1;
		/* fall through */

	case IRQ_TYPE_LEVEL_HIGH:
		break;

	case IRQ_TYPE_EDGE_FALLING:
		level = 1;
		/* fall through */

	case IRQ_TYPE_EDGE_RISING:
		mode = 1;
		break;

	default:
		return -EINVAL;
	}

	spin_lock(&ftintc010_lock);
	ftintc010_set_trig_mode(base, hw_irq, mode);
	ftintc010_set_trig_level(base, hw_irq, level);
	spin_unlock(&ftintc010_lock);
	return 0;
}
/*
 * Edge trigger IRQ chip methods
 */
static void ftintc010_edge_ack(struct irq_data *data)
{
	unsigned int hw_irq = ftintc010_irq(data->irq);
	void __iomem *base = ftintc010_base(data->irq);

	spin_lock(&ftintc010_lock);
	ftintc010_clear_irq(base, hw_irq);
	spin_unlock(&ftintc010_lock);
}

static void ftintc010_mask(struct irq_data *data)
{
	unsigned int hw_irq = ftintc010_irq(data->irq);
	void __iomem *base = ftintc010_base(data->irq);

	spin_lock(&ftintc010_lock);
	ftintc010_mask_irq(base, hw_irq);
	spin_unlock(&ftintc010_lock);
}

static void ftintc010_unmask(struct irq_data *data)
{
	unsigned int hw_irq = ftintc010_irq(data->irq);
	void __iomem *base = ftintc010_base(data->irq);

	spin_lock(&ftintc010_lock);
	ftintc010_unmask_irq(base, hw_irq);
	spin_unlock(&ftintc010_lock);
}

static struct irq_chip ftintc010_edge_chip = {
	.irq_ack	= ftintc010_edge_ack,
	.irq_mask	= ftintc010_mask,
	.irq_unmask	= ftintc010_unmask,
	.irq_set_type	= ftintc010_set_type,
};

/*
 * Level trigger IRQ chip methods
 */
static void ftintc010_level_ack(struct irq_data *data)
{
	/* do nothing */
	if (data)   {}
}

static struct irq_chip ftintc010_level_chip = {
	.irq_ack	= ftintc010_level_ack,
	.irq_mask	= ftintc010_mask,
	.irq_unmask	= ftintc010_unmask,
	.irq_set_type	= ftintc010_set_type,
};

/******************************************************************************
 * initialization functions
 *****************************************************************************/
static void ftintc010_handle_cascade_irq(unsigned int irq, struct irq_desc *desc)
{
	struct ftintc010_chip_data *chip_data = irq_get_handler_data(irq);
	struct irq_chip *chip = irq_get_chip(irq);
	unsigned int cascade_irq, hw_irq;
	unsigned long status;

	/* defined in asm/mach/irq.h. 
	 * mask the primary's irq.
	 */
	chained_irq_enter(chip, desc);

	status = readl(chip_data->base + FTINTC010_OFFSET_IRQSTATUS);

	if (status) {
		hw_irq = ffs(status) - 1;
	} else {
		status = readl(chip_data->base + FTINTC010_OFFSET_IRQSTATUSEX);

		if (!status)
			goto out;

		hw_irq = ffs(status) - 1 + 32;
	}

	cascade_irq = hw_irq + chip_data->irq_offset;
	generic_handle_irq(cascade_irq);

out:
	/* primary controller unmasking */
	chained_irq_exit(chip, desc);
}

void __init ftintc010_cascade_irq(unsigned int ftintc010_nr, unsigned int irq)
{
	if (ftintc010_nr >= MAX_FTINTC010_NR)
		BUG();
		
	if (irq_set_handler_data(irq, &ftintc010_data[ftintc010_nr]) != 0)
		BUG();

	irq_set_chained_handler(irq, ftintc010_handle_cascade_irq);
}

/* reconfigure the irq type. Maybe the original is edge trigger, now change to level trigger 
 * @type:	IRQ_TYPE_{LEVEL,EDGE}_* value - see include/linux/irq.h
 */
int ftintc010_set_irq_type(unsigned int irq, unsigned int type)
{            
	switch (type) {
	case IRQ_TYPE_LEVEL_LOW:
	case IRQ_TYPE_LEVEL_HIGH:
		irq_set_chip(irq, &ftintc010_level_chip);
		irq_set_handler(irq, handle_level_irq);
		break;

	case IRQ_TYPE_EDGE_FALLING:
	case IRQ_TYPE_EDGE_RISING:
		irq_set_chip(irq, &ftintc010_edge_chip);
		irq_set_handler(irq, handle_edge_irq);
		break;

	default:
		return -EINVAL;
	}
    
    /* kernel function */
	return irq_set_irq_type(irq, type);
}

/*
 * Initialization of master interrupt controller, after this INTC is
 * enabled, the rest of Linux initialization codes can then be completed.
 * For example, timer interrupts and UART interrupts must be enabled during
 * the boot process.
 */
void __init ftintc010_init(unsigned int ftintc010_nr, void __iomem *base,
			   unsigned int irq_start,
			   struct ftintc010_trigger_type *trigger_type)
{
	int irq;
	int i;

	if (ftintc010_nr >= MAX_FTINTC010_NR)
		BUG();
    
	ftintc010_data[ftintc010_nr].base = base;
	ftintc010_data[ftintc010_nr].irq_offset = irq_start;

	/*
	 * mask all interrupts
	 */
	writel(0, base + FTINTC010_OFFSET_IRQMASK);
	writel(0, base + FTINTC010_OFFSET_FIQMASK);
	writel(~0, base + FTINTC010_OFFSET_IRQCLEAR);
	writel(~0, base + FTINTC010_OFFSET_FIQCLEAR);

	writel(0, base + FTINTC010_OFFSET_IRQMASKEX);
	writel(~0, base + FTINTC010_OFFSET_IRQCLEAREX);
	writel(0, base + FTINTC010_OFFSET_FIQMASKEX);
	writel(~0, base + FTINTC010_OFFSET_FIQCLEAREX);

	/*
	 * setup trigger mode and level
	 */
	writel(trigger_type->irqmode, base + FTINTC010_OFFSET_IRQMODE);
	writel(trigger_type->irqlevel, base + FTINTC010_OFFSET_IRQLEVEL);
	writel(trigger_type->fiqmode, base + FTINTC010_OFFSET_FIQMODE);
	writel(trigger_type->fiqlevel, base + FTINTC010_OFFSET_FIQLEVEL);

	writel(trigger_type->irqmodeex, base + FTINTC010_OFFSET_IRQMODEEX);
	writel(trigger_type->irqlevelex, base + FTINTC010_OFFSET_IRQLEVELEX);
	writel(trigger_type->fiqmodeex, base + FTINTC010_OFFSET_FIQMODEEX);
	writel(trigger_type->fiqlevelex, base + FTINTC010_OFFSET_FIQLEVELEX);


	/*
	 * setup the linux irq subsystem. 
	 * Note: for FIQ subsystem, it not supported in linux. Instead, we have own FIQ subsystem.
	 */
	irq = irq_start;
	for (i = 0; i < 32; i++) {
		if (trigger_type->irqmode & (1 << i)) {
		    irq_set_chip(irq, &ftintc010_edge_chip);
			irq_set_handler(irq, handle_edge_irq);
		} else {
		    irq_set_chip(irq, &ftintc010_level_chip);
			irq_set_handler(irq, handle_level_irq);
		}

		irq_set_chip_data(irq, &ftintc010_data[ftintc010_nr]);
		set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
		irq++;
	}


    /* keep irq number counting up */
	for (i = 0; i < 32; i++) {
		if (trigger_type->irqmodeex & (1 << i)) {
			irq_set_chip(irq, &ftintc010_edge_chip);
			irq_set_handler(irq, handle_edge_irq);
		} else {
			irq_set_chip(irq, &ftintc010_level_chip);
			irq_set_handler(irq, handle_level_irq);
		}

		irq_set_chip_data(irq, &ftintc010_data[ftintc010_nr]);
		set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
		irq++;
	}
}
