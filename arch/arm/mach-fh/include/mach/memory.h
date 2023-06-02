/*
 * fh memory space definitions
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <asm/page.h>
#include <asm/sizes.h>

/**************************************************************************
 * Definitions
 **************************************************************************/
#define FH_DDR_BASE	0xA0000000

#define PLAT_PHYS_OFFSET FH_DDR_BASE

#endif /* __ASM_ARCH_MEMORY_H */
