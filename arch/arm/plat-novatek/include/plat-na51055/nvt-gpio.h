/**
	GPIO header file
	This file will setup gpio related base address
	@file       nvt-gpio.h
	@ingroup
	@note
	Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/
#ifndef __ASM_ARCH_NA51055_NVT_GPIO_H
#define __ASM_ARCH_NA51055_NVT_GPIO_H

#include <linux/types.h>
#include <linux/irq.h>
#include "hardware.h"

#define NVT_GPIO_STG_DATA_0  0x00
#define NVT_GPIO_STG_DIR_0   0x20
#define NVT_GPIO_STG_SET_0   0x40
#define NVT_GPIO_STG_CLR_0   0x60
#define NVT_GPIO_STS_CPU     0x80
#define NVT_GPIO_STS_CPU2    0x84
#define NVT_GPIO_INTEN_CPU   0x90
#define NVT_GPIO_INTEN_CPU2  0x94
#define NVT_GPIO_INT_TYPE    0xA0
#define NVT_GPIO_INT_POL     0xA4
#define NVT_GPIO_EDGE_TYPE   0xA8
#define NVT_GPIO_TO_CPU1     0xB0
#define NVT_GPIO_TO_CPU2     0xB4
#define NVT_DGPIO_STS_CPU    0xC0
#define NVT_DGPIO_STS_CPU2   0xC4
#define NVT_DGPIO_INTEN_CPU  0xD0
#define NVT_DGPIO_INTEN_CPU2 0xD4
#define NVT_DGPIO_INT_TYPE   0xE0
#define NVT_DGPIO_INT_POL    0xE4
#define NVT_DGPIO_EDGE_TYPE  0xE8
#define NVT_DGPIO_TO_CPU1    0xF0
#define NVT_DGPIO_TO_CPU2    0xF4

/* GPIO pin number translation  */
#define C_GPIO(pin)			(pin)
#define P_GPIO(pin)			(pin + 0x20)
#define S_GPIO(pin)			(pin + 0x40)
#define L_GPIO(pin)			(pin + 0x60)
#define D_GPIO(pin)			(pin + 0x80)
#define H_GPIO(pin)			(pin + 0xA0)
#define A_GPIO(pin)			(pin + 0xC0)

#define C_GPIO_NUM			23
#define P_GPIO_NUM			26
#define S_GPIO_NUM			13
#define L_GPIO_NUM			25
#define H_GPIO_NUM			12
#define D_GPIO_NUM			11
#define A_GPIO_NUM			3

#define USB_INT                         A_GPIO(A_GPIO_NUM)

#define NVT_IRQ_GPIO_EXT_START		(IRQ_SPI_END)

/* GPIO total pin number */
#define NVT_GPIO_NUMBER			(224)
#endif /* __ASM_ARCH_NA51055_NVT_GPIO_H */
