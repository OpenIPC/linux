/*
 *  arch/arm/mach-faraday/include/mach/ftintc030.h
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

#ifndef __FTINTC030_H
#define __FTINTC030_H


#define FTINTC030_OFFSET_IRQSRC		0x00
#define FTINTC030_OFFSET_IRQENABLE  0x04
#define FTINTC030_OFFSET_IRQCLEAR	0x08
#define FTINTC030_OFFSET_IRQMODE	0x0c
#define FTINTC030_OFFSET_IRQLEVEL	0x10
#define FTINTC030_OFFSET_IRQPEND	0x14//IRQSTATUS
#define FTINTC030_OFFSET_IRQPEND1	0x34


#define FTINTC030_OFFSET_IRQCONFIG	0x200
//#define FTINTC030_OFFSET_IRQTARGET	0x420

#define FTINTC030_OFFSET_CPU_0_FIQ	0x420
#define FTINTC030_OFFSET_CPU_0_IRQ	0x45C
#define FTINTC030_OFFSET_CPU_1_FIQ	0x498
#define FTINTC030_OFFSET_CPU_1_IRQ	0x4D4
#define FTINTC030_OFFSET_CPU_2_FIQ	0x510
#define FTINTC030_OFFSET_CPU_2_IRQ	0x54C
#define FTINTC030_OFFSET_CPU_3_FIQ	0x588
#define FTINTC030_OFFSET_CPU_3_IRQ	0x5C4

#define FTINTC010_OFFSET_FIQCLEAREX	0x88

#ifndef __ASSEMBLY__
struct ftintc030_trigger_type {
	unsigned int irqmode[15];
	unsigned int irqlevel[15];
	unsigned int fiqmode[15];
	unsigned int fiqlevel[15];
};

/* IRQ */
#define FTINTC030_TARGETIRQ_CPU0   0x1
#define FTINTC030_TARGETIRQ_CPU1   0x2
#define FTINTC030_TARGETIRQ_CPU2   0x4
#define FTINTC030_TARGETIRQ_CPU3   0x8
/* FIQ */
#define FTINTC030_TARGETFIQ_CPU0   0x1
#define FTINTC030_TARGETFIQ_CPU1   0x2
#define FTINTC030_TARGETFIQ_CPU2   0x4
#define FTINTC030_TARGETFIQ_CPU3   0x8

void __init ftintc030_cascade_irq(unsigned int ftintc030_nr, unsigned int irq);

void __init ftintc030_init(unsigned int ftintc030_nr, void __iomem *base,
			   unsigned int irq_start,
			   struct ftintc030_trigger_type *trigger_type);

/*
 * Set a highlevel chained flow handler for a given IRQ.
 * ftintc030_nr: INTC030 index
 * irq: which irq number is the cascade irq
 * handler: the handler function of this cascade irq
 * handler_data: private data of the handler
 */
void ftintc030_setup_chain_irq(unsigned int ftintc030_nr, unsigned int irq, void *handler, void *handler_data);

#endif /* __ASSEMBLY__ */

#endif /* __FTINTC030_H */
