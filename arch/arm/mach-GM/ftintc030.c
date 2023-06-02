/*
 *  linux/arch/arm/mach-GM-Duo/FTINTC030.c
 *
 *  Faraday FTINTC030 Interrupt Controller
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
#include <mach/ftintc030.h>

struct ftintc030_chip_data {
	unsigned int irq_offset;
	void __iomem *base;
};

#ifndef MAX_FTINTC030_NR
#define MAX_FTINTC030_NR	1
#endif

static struct ftintc030_chip_data ftintc030_data[MAX_FTINTC030_NR];
static DEFINE_SPINLOCK(ftintc030_lock);

/******************************************************************************
 * internal functions
 *****************************************************************************/
static inline void __iomem *ftintc030_base(unsigned int irq)
{
	struct ftintc030_chip_data *chip_data = irq_get_chip_data(irq);

	return chip_data->base;
}

/*
 * return hardware irq number
 */
static inline unsigned int ftintc030_irq(unsigned int irq)
{
	struct ftintc030_chip_data *chip_data = irq_get_chip_data(irq);

	return irq - chip_data->irq_offset;
}

static inline void ftintc030_clear_irq(void __iomem *base, unsigned int hw_irq)
{
	unsigned int mask;

	mask = 1 << (hw_irq % 32);
	if (hw_irq < 64)
		writel(mask, base + FTINTC030_OFFSET_IRQCLEAR + (0x20 * (hw_irq / 32)));
	else
		writel(mask, base + FTINTC030_OFFSET_IRQCLEAR + (0x20 * ((hw_irq / 32) + 1)));
}

static inline void ftintc030_mask_irq(void __iomem *base, unsigned int hw_irq)
{
	volatile unsigned int target_base, value;
    int i = 0;
    
	/*
	 * 0: masked
	 * 1: unmasked
	 */
    target_base = (u32)base + FTINTC030_OFFSET_IRQTARGET + (hw_irq >> 5) * 4;
    value = readl(target_base);
    value &= ~(1 << (hw_irq & 0x1F));
    writel(value, target_base);
    
    /* FIXME: work around for IC bug */
    while (readl(target_base) != value) {
        writel(value, target_base);
        i ++;
        if (i > 10) {
            printk("%s, ERROR! \n", __func__);
            break;
        }
    }
}

static inline void ftintc030_unmask_irq(void __iomem *base, unsigned int hw_irq)
{
	volatile unsigned int target_base, value;
    int i = 0;
    
	/*
	 * 0: masked
	 * 1: unmasked
	 */
	target_base = (u32)base + FTINTC030_OFFSET_IRQTARGET + (hw_irq >> 5) * 4;
    value = readl(target_base);
    value |= (1 << (hw_irq & 0x1F));
    writel(value, target_base);
    
    /* FIXME: work around for IC bug */
    while (readl(target_base) != value) {
        writel(value, target_base);
        i ++;
        if (i > 10) {
            printk("%s, ERROR! \n", __func__);
            break;
        }
    }
}

/******************************************************************************
 * struct irq_chip functions
 *****************************************************************************/
static inline void ftintc030_set_trig_mode(void __iomem *base, unsigned int hw_irq, int mode)
{
	unsigned int irqmode, reg_offset;

	/*
	 * 0: level trigger
	 * 1: edge trigger
	 */
	if (hw_irq < 64)
	    reg_offset = 0x20 * (hw_irq >> 5);
	else
	    reg_offset = 0x20 * ((hw_irq >> 5) + 1);
	    	 
	irqmode = readl(base + FTINTC030_OFFSET_IRQMODE + reg_offset);

	if(mode)
    	irqmode |= 1 << (hw_irq % 32);
    else
    	irqmode &= ~(1 << (hw_irq % 32));

	writel(irqmode, base + FTINTC030_OFFSET_IRQMODE + reg_offset);
}

static inline void ftintc030_set_trig_level(void __iomem *base, unsigned int hw_irq, int level)
{
	unsigned int irqlevel, reg_offset;

	/*
	 * 0: active-high level trigger / rising edge trigger
	 * 1: active-low level trigger / falling edge trigger
	 */
	if (hw_irq < 64)
	    reg_offset = 0x20 * (hw_irq >> 5);
	else
	    reg_offset = 0x20 * ((hw_irq >> 5) + 1);
	        	    
	irqlevel = readl(base + FTINTC030_OFFSET_IRQLEVEL + reg_offset);
	
	if(level)
    	irqlevel |= 1 << (hw_irq % 32);
    else
    	irqlevel &= ~(1 << (hw_irq % 32));
    	
	writel(irqlevel, base + FTINTC030_OFFSET_IRQLEVEL + reg_offset);
}

static int ftintc030_set_type(struct irq_data *data, unsigned int type)
{
	unsigned int hw_irq = ftintc030_irq(data->irq);
	void __iomem *base = ftintc030_base(data->irq);
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

	spin_lock(&ftintc030_lock);
	ftintc030_set_trig_mode(base, hw_irq, mode);
	ftintc030_set_trig_level(base, hw_irq, level);
	spin_unlock(&ftintc030_lock);
	return 0;
}
/*
 * Edge trigger IRQ chip methods
 */
static void ftintc030_edge_ack(struct irq_data *data)
{
	unsigned int hw_irq = ftintc030_irq(data->irq);
	void __iomem *base = ftintc030_base(data->irq);

	spin_lock(&ftintc030_lock);
	ftintc030_clear_irq(base, hw_irq);
	spin_unlock(&ftintc030_lock);
}

static void ftintc030_mask(struct irq_data *data)
{
	unsigned int hw_irq = ftintc030_irq(data->irq);
	void __iomem *base = ftintc030_base(data->irq);

	spin_lock(&ftintc030_lock);
	ftintc030_mask_irq(base, hw_irq);
	spin_unlock(&ftintc030_lock);
}

static void ftintc030_unmask(struct irq_data *data)
{
	unsigned int hw_irq = ftintc030_irq(data->irq);
	void __iomem *base = ftintc030_base(data->irq);

	spin_lock(&ftintc030_lock);
	ftintc030_unmask_irq(base, hw_irq);
	spin_unlock(&ftintc030_lock);
}

static struct irq_chip ftintc030_edge_chip = {
	.irq_ack	= ftintc030_edge_ack,
	.irq_mask	= ftintc030_mask,
	.irq_unmask	= ftintc030_unmask,
	.irq_set_type	= ftintc030_set_type,
};

/*
 * Level trigger IRQ chip methods
 */
static void ftintc030_level_ack(struct irq_data *data)
{
	/* do nothing */
	if (data)   {}
}

static struct irq_chip ftintc030_level_chip = {
	.irq_ack	= ftintc030_level_ack,
	.irq_mask	= ftintc030_mask,
	.irq_unmask	= ftintc030_unmask,
	.irq_set_type	= ftintc030_set_type,
};

/******************************************************************************
 * initialization functions
 *****************************************************************************/
/* reconfigure the irq type. Maybe the original is edge trigger, now change to level trigger
 * @type:	IRQ_TYPE_{LEVEL,EDGE}_* value - see include/linux/irq.h
 */
int ftintc030_set_irq_type(unsigned int irq, unsigned int type)
{
	switch (type) {
	case IRQ_TYPE_LEVEL_LOW:
	case IRQ_TYPE_LEVEL_HIGH:
		irq_set_chip(irq, &ftintc030_level_chip);
		irq_set_handler(irq, handle_level_irq);
		break;

	case IRQ_TYPE_EDGE_FALLING:
	case IRQ_TYPE_EDGE_RISING:
		irq_set_chip(irq, &ftintc030_edge_chip);
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
void __init ftintc030_init(unsigned int ftintc030_nr, void __iomem *base,
			   unsigned int irq_start,
			   struct ftintc030_trigger_type *trigger_type)
{
	int irq;
	int i, j, offset;

	if (ftintc030_nr >= MAX_FTINTC030_NR)
		BUG();

	ftintc030_data[ftintc030_nr].base = base;
	ftintc030_data[ftintc030_nr].irq_offset = irq_start;

	irq = irq_start;
	/*
	 * mask all interrupts
	 */
	for(i = 0; i < PLATFORM_IRQ_TOTALCOUNT / 32; i++) {
	    if(i >= 2)
		    offset = 0x20 * (i + 1);
	    else
		    offset = 0x20 * i;

#ifndef CONFIG_PLATFORM_GM8210_S //only allow master cpu to configure this
	    writel(0xFFFFFFFF, base + FTINTC030_OFFSET_IRQENABLE + offset);   //ENABLE register
	    writel(~0, base + FTINTC030_OFFSET_IRQCLEAR + offset);

	    /*
	     * setup trigger mode and level
	     */
  	    writel(trigger_type->irqmode[i], base + FTINTC030_OFFSET_IRQMODE + offset);
  	    writel(trigger_type->irqlevel[i], base + FTINTC030_OFFSET_IRQLEVEL + offset);
  	    writel(trigger_type->fiqmode[i], base + FTINTC030_OFFSET_IRQMODE + offset);
  	    writel(trigger_type->fiqlevel[i], base + FTINTC030_OFFSET_IRQLEVEL + offset);
#endif
  	    /*
  	     * setup the linux irq subsystem.
  	     * Note: for FIQ subsystem, it not supported in linux. Instead, we have own FIQ subsystem.
  	     */

  	    for (j = 0; j < 32; j++) {
    		if (trigger_type->irqmode[i] & (1 << j)) {
    		    irq_set_chip(irq, &ftintc030_edge_chip);
    			irq_set_handler(irq, handle_edge_irq);
    		} else {
    		    irq_set_chip(irq, &ftintc030_level_chip);
    			irq_set_handler(irq, handle_level_irq);
    		}

    		irq_set_chip_data(irq, &ftintc030_data[ftintc030_nr]);
    		set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
    		irq++;
	    }
	}
}

/*
 * Set a highlevel chained flow handler for a given IRQ.
 * ftintc030_nr: INTC030 index
 * irq: which irq number is the cascade irq
 * handler: the handler function of this cascade irq
 * handler_data: private data of the handler
 */
void ftintc030_setup_chain_irq(unsigned int ftintc030_nr, unsigned int irq, void *handler, void *handler_data)
{
    if (ftintc030_nr) {}

    if (irq_set_handler_data(irq, handler_data) != 0)
		panic("%s fail! \n", __func__);

    irq_set_chained_handler(irq, handler);
}
