/*
 * AK3918AV130 keeps the vendor's own copy of this header, from the
 * AnyCloud39AV130 SDK, in mach/av130/: its register map and IRQ layout
 * share nothing with EV200/EV330 beyond the names.
 */
#ifdef CONFIG_MACH_AK3918AV130
#include <mach/av130/ak_l2.h>
#else
/*
 * linux/arch/arm/mach-anycloud/include/mach/ak_l2.h
 *
 * Copyright (C) 2020 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
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

#ifndef __ASM_ARCH_L2_H__
#define __ASM_ARCH_L2_H__

#if defined(CONFIG_MACH_AK37E)
#include "ak_l2_dual_inner.h"
#else
#include "ak_l2_inner.h"
#endif

#endif //__ASM_ARCH_L2_H__
#endif /* CONFIG_MACH_AK3918AV130 */
