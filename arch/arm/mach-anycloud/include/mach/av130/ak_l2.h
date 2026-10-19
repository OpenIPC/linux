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

#if defined(CONFIG_MACH_AK37E) ||   \
    defined(CONFIG_MACH_AK3918AV100) ||  \
    defined(CONFIG_MACH_AK3918EV300L)
#include "ak_l2_dual_inner.h"
#else
#include "ak_l2_inner.h"
#endif

#endif //__ASM_ARCH_L2_H__

/*end of file */
