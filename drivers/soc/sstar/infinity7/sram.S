/*
* sram.S- Sigmastar
*
* Copyright (c) [2019~2020] SigmaStar Technology.
*
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License version 2 for more details.
*
*/
/*------------------------------------------------------------------------------
    Function Code
-------------------------------------------------------------------------------*/
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/asm-offsets.h>
//#include <asm/hardware/cache-l2x0.h>

    .align 3
.globl sram_suspend_imi
.globl v7_cpu_resume

SYM_FUNC_START(sram_suspend_imi)


SYM_FUNC_END(sram_suspend_imi)
.ltorg
