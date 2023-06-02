/*
 *  arch/arm/mach-faraday/include/mach/ftahbb020.h
 *
 *  Faraday FTAHBB020 AHB Bridge
 *
 *  Copyright (C) 2010 Faraday Technology
 *  Copyright (C) 2010 Po-Yu Chuang <ratbert@faraday-tech.com>
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

#ifndef __FTAHBB020_H
#define __FTAHBB020_H

#define FTAHBB020_OFFSET_P2S_IRQSRC	0x0
#define FTAHBB020_OFFSET_P2S_IRQMASK	0x4
#define FTAHBB020_OFFSET_P2S_IRQCLEAR	0x8
#define FTAHBB020_OFFSET_P2S_BUFATTR	0xc
#define FTAHBB020_OFFSET_P2S_PROTENABLE	0x10
#define FTAHBB020_OFFSET_P2S_XIRQSRC	0x14
#define FTAHBB020_OFFSET_P2S_XIRQMASK	0x18
#define FTAHBB020_OFFSET_P2S_XIRQCLEAR	0x1c
#define FTAHBB020_OFFSET_P2S_XIRQMODE	0x20
#define FTAHBB020_OFFSET_P2S_XIRQBOTH	0x24
#define FTAHBB020_OFFSET_P2S_XIRQLEVEL	0x28
#define FTAHBB020_OFFSET_P2S_XIRQENABLE	0x2c
#define FTAHBB020_OFFSET_P2S_IWBA(x)	(0x100 + (x) * 0x10)
#define FTAHBB020_OFFSET_P2S_IWAT(x)	(0x104 + (x) * 0x10)
#define FTAHBB020_OFFSET_P2S_IWTA(x)	(0x108 + (x) * 0x10)
#define FTAHBB020_OFFSET_S2P_IRQSRC	0x400
#define FTAHBB020_OFFSET_S2P_IRQMASK	0x404
#define FTAHBB020_OFFSET_S2P_IRQCLEAR	0x408
#define FTAHBB020_OFFSET_S2P_BUFATTR	0x40c
#define FTAHBB020_OFFSET_S2P_PROTENABLE	0x410
#define FTAHBB020_OFFSET_S2P_XIRQSRC	0x414
#define FTAHBB020_OFFSET_S2P_XIRQMASK	0x418
#define FTAHBB020_OFFSET_S2P_XIRQCLEAR	0x41c
#define FTAHBB020_OFFSET_S2P_XIRQMODE	0x420
#define FTAHBB020_OFFSET_S2P_XIRQBOTH	0x424
#define FTAHBB020_OFFSET_S2P_XIRQLEVEL	0x428
#define FTAHBB020_OFFSET_S2P_XIRQENABLE	0x42c
#define FTAHBB020_OFFSET_S2P_IWBA(x)	(0x500 + (x) * 0x10)
#define FTAHBB020_OFFSET_S2P_IWAT(x)	(0x504 + (x) * 0x10)
#define FTAHBB020_OFFSET_S2P_IWTA(x)	(0x508 + (x) * 0x10)

#ifndef __ASSEMBLY__

#include <linux/irq.h>

void __init ftahbb020_init(unsigned int ftahbb020_nr, void __iomem *base,
			   unsigned int irq_start);
void __init ftahbb020_cascade_irq(unsigned int ftahbb020_nr, unsigned int irq);
int ftahbb020_set_irq_type(unsigned int irq, unsigned int type);

#endif	/* __ASSEMBLY__ */

#endif	/* __FTAHBB020_H */
