/**
    NVT IRQ define
    To define IRQ related setting
    @file      irqs.h 
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_NVT_IVOT_IRQS_H
#define __ASM_ARCH_NVT_IVOT_IRQS_H

#define IRQ_SPI_START                   32
#define IRQ_SPI_END                     256

#define NVT_GPIO_INT_EXT_TOTAL          128

#ifndef CONFIG_SPARSE_IRQ
#define NR_IRQS (256 + NVT_GPIO_INT_EXT_TOTAL)
#endif

#endif /* __ASM_ARCH_NVT_IVOT_IRQS_H */
