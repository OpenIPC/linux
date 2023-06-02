/*
 * Copyright (C) 2005 Mips Technologies
 * Author: Chris Dearman, chris@mips.com derived from fpu.h
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#ifndef _ASM_MXU_V3_H
#define _ASM_MXU_V3_H

#include <asm/cpu.h>
#include <asm/cpu-features.h>
#include <asm/hazards.h>
#include <asm/mipsregs.h>
#include <linux/printk.h>

void __init_mxuv3(void);
void __save_mxuv3(void *tsk_void);
void __restore_mxuv3(void *tsk_void);

static inline void init_mxuv3(void)
{
	if(cpu_has_mxuv3)
		__init_mxuv3();
}


#define save_mxuv3(tsk)							\
	do {										\
		if (cpu_has_mxuv3)						\
			__save_mxuv3(tsk);					\
	} while (0)

#define restore_mxuv3(tsk)						\
	do {										\
		if (cpu_has_mxuv3)						\
			__restore_mxuv3(tsk);					\
	} while (0)

#endif /* _ASM_MXU_V3_H */
