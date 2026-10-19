/*
 * AK3918AV130 keeps the vendor's own copy of this header, from the
 * AnyCloud39AV130 SDK, in mach/av130/: its register map and IRQ layout
 * share nothing with EV200/EV330 beyond the names.
 */
#ifdef CONFIG_MACH_AK3918AV130
#include <mach/av130/map.h>
#else
/*
 * linux/arch/arm/mach-ak39/include/mach/map.h
 *
 * Copyright (C) 2018 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
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
 */


#ifndef __ASM_ARCH_MAP_H
#define __ASM_ARCH_MAP_H

#ifndef __ASSEMBLY__
#define AK_ADDR(x)		((void __iomem *)0xF0000000 + (x))
#else
#define AK_ADDR(x)		(0xF0000000 + (x))
#endif

#ifdef CONFIG_MACH_AK37D
#define AK_VA_SYSCTRL		AK_ADDR(0x00008000)
#define AK_PA_SYSCTRL		(0x08000000)
#define AK_SZ_SYSCTRL		SZ_32K

#define AK_VA_CAMERA		AK_ADDR(0x00010000)
#define AK_PA_CAMERA		(0x20000000)
#define AK_SZ_CAMERA		SZ_64K

#define AK_VA_GUI			AK_ADDR(0x00050000)
#define AK_PA_GUI			(0x20040000)
#define AK_SZ_GUI			SZ_64K

#define AK_VA_SUBCTRL		AK_ADDR(0x00060000)
#define AK_PA_SUBCTRL		(0x20100000)
#define AK_SZ_SUBCTRL		SZ_2M

#define AK_VA_MIPI1			AK_ADDR(0x00262000)
#define AK_PA_MIPI1			(0x20400000)
#define AK_SZ_MIPI1			SZ_64K

#define AK_VA_MIPI2			AK_ADDR(0x00272000)
#define AK_PA_MIPI2			(0x20480000)
#define AK_SZ_MIPI2			SZ_64K

#define AK_VA_L2MEM			AK_ADDR(0x00294000)
#define AK_PA_L2MEM			(0x48000000)
#define AK_SZ_L2MEM			SZ_8K

#define AK_VA_RESERVED_MEM	AK_ADDR(0x00296000)
#define AK_PA_RESERVED_MEM	(0x81400000)
#define AK_SZ_RESERVED_MEM	(0x2000000)

#define AK_VA_L2CTRL			(AK_VA_SUBCTRL + 0x40000)
#define AK_PA_L2CTRL			(AK_PA_SUBCTRL + 0x40000)

#define AK_VA_USB			(AK_VA_SUBCTRL + 0x100000)
#define AK_PA_USB			(AK_PA_SUBCTRL + 0x100000)
#endif

#ifdef CONFIG_MACH_AK37E
#define AK_VA_SYSCTRL		AK_ADDR(0x00008000)
#define AK_PA_SYSCTRL		(0x08000000)
#define AK_SZ_SYSCTRL		SZ_32K

#define AK_VA_CAMERA		AK_ADDR(0x00010000)
#define AK_PA_CAMERA		(0x20000000)
#define AK_SZ_CAMERA		SZ_64K

#define AK_VA_LCD			AK_ADDR(0x00020000)
#define AK_PA_LCD			(0x20010000)
#define AK_SZ_LCD			SZ_64K

#define AK_VA_GUI			AK_ADDR(0x00050000)
#define AK_PA_GUI			(0x20040000)
#define AK_SZ_GUI			SZ_64K

#define AK_VA_SUBCTRL		AK_ADDR(0x00060000)
#define AK_PA_SUBCTRL		(0x20100000)
#define AK_SZ_SUBCTRL		SZ_2M

#define AK_VA_MIPI1			AK_ADDR(0x00262000)
#define AK_PA_MIPI1			(0x20400000)
#define AK_SZ_MIPI1			SZ_64K

#define AK_VA_MIPI2			AK_ADDR(0x00272000)
#define AK_PA_MIPI2			(0x20480000)
#define AK_SZ_MIPI2			SZ_64K

#define AK_VA_L2MEM			AK_ADDR(0x00294000)
#define AK_PA_L2MEM			(0x48000000)
#define AK_SZ_L2MEM			SZ_8K

#define AK_VA_L2_1_MEM		AK_ADDR(0x00296000)
#define AK_PA_L2_1_MEM		(0x58000000)
#define AK_SZ_L2_1_MEM		SZ_8K

#define AK_VA_RESERVED_MEM	AK_ADDR(0x00298000)
#define AK_PA_RESERVED_MEM	(0x81400000)
#define AK_SZ_RESERVED_MEM	(0x2000000)

#define AK_VA_L2CTRL			(AK_VA_SUBCTRL + 0x40000)
#define AK_PA_L2CTRL			(AK_PA_SUBCTRL + 0x40000)

#define AK_VA_L2_1_CTRL			(AK_VA_SUBCTRL + 0x80000)
#define AK_PA_L2_1_CTRL			(AK_PA_SUBCTRL + 0x80000)

#define AK_VA_USB			(AK_VA_SUBCTRL + 0x100000)
#define AK_PA_USB			(AK_PA_SUBCTRL + 0x100000)
#endif

#if defined(CONFIG_MACH_AK39EV330) || defined(CONFIG_MACH_AK3918EV200)
#define AK_VA_SYSCTRL		AK_ADDR(0x00008000)
#define AK_PA_SYSCTRL		(0x08000000)
#define AK_SZ_SYSCTRL		SZ_32K

#define AK_VA_CAMERA		AK_ADDR(0x00010000)
#define AK_PA_CAMERA		(0x20000000)
#define AK_SZ_CAMERA		SZ_64K

#define AK_VA_SUBCTRL		AK_ADDR(0x00060000)
#define AK_PA_SUBCTRL		(0x20100000)
#define AK_SZ_SUBCTRL		SZ_2M

#define AK_VA_MIPI1			AK_ADDR(0x00262000)
#define AK_PA_MIPI1			(0x20400000)
#define AK_SZ_MIPI1			SZ_1M

#define AK_VA_L2MEM			AK_ADDR(0x00362000)
#define AK_PA_L2MEM			(0x48000000)
#define AK_SZ_L2MEM			SZ_8K

#define AK_VA_RESERVED_MEM	AK_ADDR(0x00396000)
#define AK_PA_RESERVED_MEM	(0x81400000)
#define AK_SZ_RESERVED_MEM	(0x2000000)

#define AK_VA_L2CTRL		(AK_VA_SUBCTRL + 0x40000)
#define AK_PA_L2CTRL		(AK_PA_SUBCTRL + 0x40000)

#define AK_VA_USB			(AK_VA_SUBCTRL + 0x100000)
#define AK_PA_USB			(AK_PA_SUBCTRL + 0x100000)

#endif

#endif  /* __ASM_ARCH_MAP_H */

#endif /* CONFIG_MACH_AK3918AV130 */
