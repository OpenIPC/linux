/*
 *  arch/arm/mach-faraday/include/mach/ftintc010.h
 *
 *  Faraday FTINTC010 Interrupt Controller
 *
 *  Copyright (C) 2009 Faraday Technology
 *  Copyright (C) 2009 Po-Yu Chuang <ratbert@faraday-tech.com>
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

#ifndef __FTINTC010_H
#define __FTINTC010_H

#define FTINTC010_OFFSET_IRQSRC		0x00
#define FTINTC010_OFFSET_IRQMASK	0x04
#define FTINTC010_OFFSET_IRQCLEAR	0x08
#define FTINTC010_OFFSET_IRQMODE	0x0c
#define FTINTC010_OFFSET_IRQLEVEL	0x10
#define FTINTC010_OFFSET_IRQSTATUS	0x14

#define FTINTC010_OFFSET_FIQSRC		0x20
#define FTINTC010_OFFSET_FIQMASK	0x24
#define FTINTC010_OFFSET_FIQCLEAR	0x28
#define FTINTC010_OFFSET_FIQMODE	0x2c
#define FTINTC010_OFFSET_FIQLEVEL	0x30
#define FTINTC010_OFFSET_FIQSTATUS	0x34

#define FTINTC010_OFFSET_IRQSRCEX	0x60
#define FTINTC010_OFFSET_IRQMASKEX	0x64
#define FTINTC010_OFFSET_IRQCLEAREX	0x68
#define FTINTC010_OFFSET_IRQMODEEX	0x6c
#define FTINTC010_OFFSET_IRQLEVELEX	0x70
#define FTINTC010_OFFSET_IRQSTATUSEX	0x74

#ifndef __ASSEMBLY__

#include <linux/irq.h>

void __init ftintc010_init(unsigned int ftintc010_nr, void __iomem *base,
			   unsigned int irq_start);
void __init ftintc010_cascade_irq(unsigned int ftintc010_nr, unsigned int irq);
int ftintc010_set_irq_type(unsigned int irq, unsigned int type);

#endif	/* __ASSEMBLY__ */

#endif /* __FTINTC010_H */
