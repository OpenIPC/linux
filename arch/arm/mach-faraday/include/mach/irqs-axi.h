/*
 *  arch/arm/mach-faraday/include/mach/irqs-axi.h
 *
 *  Copyright (C) 2010 Faraday Technology
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

#ifndef __MACH_IRQS_AXI_H
#define __MACH_IRQS_AXI_H

#ifdef CONFIG_PLATFORM_AXI

/*
 * Interrupt numbers of Hierarchical Architecture
 */

#define IRQ_AXI_START	0

#define IRQ_AXI_FTTMR010_0_T0	(IRQ_AXI_START + 1)
#define IRQ_AXI_FTTMR010_0_T1	(IRQ_AXI_START + 2)
#define IRQ_AXI_FTTMR010_0_T2	(IRQ_AXI_START + 3)
#define IRQ_AXI_FTUART010_0	(IRQ_AXI_START + 4)
#define IRQ_AXI_FTUART010_1	(IRQ_AXI_START + 5)

#define IRQ_AXI_FTDMAC030_0_INT	(IRQ_AXI_START + 11)
#define IRQ_AXI_FTDMAC030_0_TC	(IRQ_AXI_START + 12)
#define IRQ_AXI_FTDMAC030_0_ERR	(IRQ_AXI_START + 13)

#define NR_IRQS			(IRQ_AXI_START + 32)

#endif	/* CONFIG_PLATFORM_AXI */

#endif	/* __MACH_IRQS_AXI_H */
