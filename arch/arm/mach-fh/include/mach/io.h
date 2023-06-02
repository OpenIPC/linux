/*
 * fh io definitions
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __ASM_ARCH_IO_H
#define __ASM_ARCH_IO_H

#include <mach/chip.h>

#define IO_SPACE_LIMIT 0xffffffff

/*
 * We don't actually have real ISA nor PCI buses, but there is so many
 * drivers out there that might just work if we fake them...
 */
#define __io(a)			__typesafe_io(a)
#define __mem_pci(a)		(a)
#define __mem_isa(a)		(a)

#ifndef CONFIG_JLINK_DEBUG
#define FH_VIRT 		0xFE000000

#define		VA_INTC_REG_BASE	(FH_VIRT + 0x00000)
#define		VA_TIMER_REG_BASE	(FH_VIRT + 0x10000)
#define		VA_UART0_REG_BASE	(FH_VIRT + 0x20000)
//#define		VA_GMAC_REG_BASE	(FH_VIRT + 0x30000)
//#define		VA_SPI0_REG_BASE	(FH_VIRT + 0x40000)
//#define		VA_GPIO_REG_BASE	(FH_VIRT + 0x50000)
//#define		VA_DMAC_REG_BASE	(FH_VIRT + 0x60000)
//#define		VA_SDC0_REG_BASE	(FH_VIRT + 0x70000)
//#define		VA_I2C_REG_BASE		(FH_VIRT + 0x80000)
#define		VA_PMU_REG_BASE		(FH_VIRT + 0x90000)
//#define		VA_SDC1_REG_BASE	(FH_VIRT + 0xa0000)
//
#define		VA_UART1_REG_BASE	(FH_VIRT + 0xb0000)
#define		VA_PAE_REG_BASE		(FH_VIRT + 0xc0000)

#define		VA_RAM_REG_BASE		(FH_VIRT + 0xd0000)
#define		VA_DDRC_REG_BASE	(FH_VIRT + 0xe0000)
#define		VA_UART2_REG_BASE	(FH_VIRT + 0xf0000)
#ifdef CONFIG_ARCH_FH
#define VA_CONSOLE_REG_BASE 		VA_UART1_REG_BASE
#else
#define VA_CONSOLE_REG_BASE 		VA_UART0_REG_BASE
#endif

#define I2C_OFFSET	(VA_I2C_REG_BASE - I2C_REG_BASE)
#define VI2C(x)		(x + I2C_OFFSET)

#define INTC_OFFSET	(VA_INTC_REG_BASE - INTC_REG_BASE)
#define VINTC(x)	(x + INTC_OFFSET)

#define TIME_OFFSET	(VA_TIMER_REG_BASE - TIMER_REG_BASE)
#define VTIMER(x)	(x + TIME_OFFSET)

#define UART0_OFFSET	(VA_UART0_REG_BASE - UART0_REG_BASE)
#define VUART0(x)	(x + UART0_OFFSET)

#define UART1_OFFSET	(VA_UART1_REG_BASE - UART1_REG_BASE)
#define VUART1(x)	(x + UART1_OFFSET)

#define UART2_OFFSET	(VA_UART2_REG_BASE - UART2_REG_BASE)
#define VUART2(x)	(x + UART2_OFFSET)

#define SPI0_OFFSET	(VA_SPI0_REG_BASE - SPI0_REG_BASE)
#define VSPI0(x)	(x + SPI0_OFFSET)

#define GMAC_OFFSET	(VA_GMAC_REG_BASE - GMAC_REG_BASE)
#define VGMAC(x)	(x + GMAC_OFFSET)

#define DMAC_OFFSET	(VA_DMAC_REG_BASE - DMAC_REG_BASE)
#define VDMAC(x)	(x + DMAC_OFFSET)

#define SDC0_OFFSET	(VA_SDC0_REG_BASE - SDC0_REG_BASE)
#define VSDC0(x)	(x + SDC0_OFFSET)

#define SDC1_OFFSET	(VA_SDC1_REG_BASE - SDC1_REG_BASE)
#define VSDC1(x)	(x + SDC1_OFFSET)

#define GPIO_OFFSET	(VA_GPIO_REG_BASE - GPIO_REG_BASE)
#define VGPIO(x)	(x + GPIO_OFFSET)

#define PMU_OFFSET	(VA_PMU_REG_BASE - PMU_REG_BASE)
#define VPMU(x)		(x + PMU_OFFSET)

#define PAE_OFFSET	(VA_PAE_REG_BASE - PAE_REG_BASE)
#define VPAE(x)		(x + PAE_OFFSET)

#else
#define		VA_INTC_REG_BASE		INTC_REG_BASE
#define		VA_TIMER_REG_BASE		TIMER_REG_BASE
#define		VA_UART0_REG_BASE		UART0_REG_BASE
#define		VA_UART1_REG_BASE		UART1_REG_BASE
#define		VA_GMAC_REG_BASE		GMAC_REG_BASE
#define		VA_DMAC_REG_BASE		DMAC_REG_BASE
#define		VA_I2C_REG_BASE			I2C_REG_BASE
#define		VA_SDC0_REG_BASE		SDC0_REG_BASE

#define		VA_SPI0_REG_BASE		SPI0_REG_BASE

#define		VA_GPIO_REG_BASE		GPIO0_REG_BASE
#define		VA_PMU_REG_BASE			PMU_REG_BASE

//#define		VA_GPIO_REG_BASE		(FH_VIRT + 0x500000)

#define VINTC(x)		 x
#define VTIMER(x)		x
#define VUART0(x)		x
#define VUART1(x)		x
#define VGMAC(x)		x

#define VDMAC(x)		x
#define VI2C(x)			x
#define VSDC0(x)			x

#define VSPI0(x)		x
#define VPMU(x)		x

#endif
#endif /* __ASM_ARCH_IO_H */
