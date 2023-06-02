/*
 *  arch/arm/mach-faraday/include/mach/ftpci100.h
 *
 *  Faraday FTPCI100 PCI Bridge Controller
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

#ifndef __FTPCI100_H
#define __FTPCI100_H

#define FTPCI100_OFFSET_IOSIZE		0x00
#define FTPCI100_OFFSET_PROT		0x04
#define FTPCI100_OFFSET_CONFIG		0x28
#define FTPCI100_OFFSET_CONFIGDATA	0x2c

/*
 * I/O space size signal register
 */
#define FTPCI100_IOSIZE_1M		0x0
#define FTPCI100_IOSIZE_2M		0x1
#define FTPCI100_IOSIZE_4M		0x2
#define FTPCI100_IOSIZE_8M		0x3
#define FTPCI100_IOSIZE_16M		0x4
#define FTPCI100_IOSIZE_32M		0x5
#define FTPCI100_IOSIZE_64M		0x6
#define FTPCI100_IOSIZE_128M		0x7
#define FTPCI100_IOSIZE_256M		0x8
#define FTPCI100_IOSIZE_512M		0x9
#define FTPCI100_IOSIZE_1G		0xa
#define FTPCI100_IOSIZE_2G		0xb

/*
 * AHB protection register
 */
#define FTPCI100_PROT_DATA		(1 << 0)
#define FTPCI100_PROT_PRIVILEGED	(1 << 1)
#define FTPCI100_PROT_BUFFERABLE	(1 << 2)
#define FTPCI100_PROT_CACHEABLE		(1 << 3)

/*
 * PCI configuration register
 */
#define FTPCI100_CONFIG_WHERE(where)	((where) & 0xfc)
#define FTPCI100_CONFIG_DEVFN(devfn)	(((devfn) & 0xff) << 8)
#define FTPCI100_CONFIG_BUS(bus)	(((bus) & 0xff) << 16)
#define FTPCI100_CONFIG_ENABLE		(1 << 31)

/*
 * PCI registers
 */
#define PCI_CR2			0x4c
#define  PCI_CR2_INTA_ENABLE	(1 << 22)
#define  PCI_CR2_INTB_ENABLE	(1 << 23)
#define  PCI_CR2_INTC_ENABLE	(1 << 24)
#define  PCI_CR2_INTD_ENABLE	(1 << 25)
#define  PCI_CR2_STATUS_MASK	(0xf << 28)
#define  PCI_CR2_STATUS_INTA	(1 << 28)
#define  PCI_CR2_STATUS_INTB	(1 << 29)
#define  PCI_CR2_STATUS_INTC	(1 << 30)
#define  PCI_CR2_STATUS_INTD	(1 << 31)

#define PCI_MEM_BASE_SIZE_1	0x50
#define PCI_MEM_BASE_SIZE_2	0x54
#define PCI_MEM_BASE_SIZE_3	0x58
#define  PCI_MEM_SIZE_1M	(0x0 << 16)
#define  PCI_MEM_SIZE_2M	(0x1 << 16)
#define  PCI_MEM_SIZE_4M	(0x2 << 16)
#define  PCI_MEM_SIZE_8M	(0x3 << 16)
#define  PCI_MEM_SIZE_16M	(0x4 << 16)
#define  PCI_MEM_SIZE_32M	(0x5 << 16)
#define  PCI_MEM_SIZE_64M	(0x6 << 16)
#define  PCI_MEM_SIZE_128M	(0x7 << 16)
#define  PCI_MEM_SIZE_256M	(0x8 << 16)
#define  PCI_MEM_SIZE_512M	(0x9 << 16)
#define  PCI_MEM_SIZE_1G	(0xa << 16)
#define  PCI_MEM_SIZE_2G	(0xb << 16)
#define  PCI_MEM_BASE(x)	((x) & ~0xfffff)

void __init ftpci100_init(unsigned int nr, void __iomem *base);
void __init ftpci100_int_init(unsigned int nr, unsigned int irq_start);
void __init ftpci100_int_cascade_irq(unsigned int nr, unsigned int irq);

#endif	/* __FTPCI100_H */
