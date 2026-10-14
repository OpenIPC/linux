/*
 * Copyright (c) LOTUS. All rights reserved.
 */

#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

#include <mach/platform.h>

#ifdef __ASSEMBLY__
.macro stopwatch_reg_write reg, val
	ldr r0, =\reg
	ldr r1, =\val
	str r1, [r0]
	dsb
.endm

.macro stopwatch_reg_read reg
	ldr r1, =\reg
	ldr r0, [r1]
	dsb
.endm

.macro STOPWATCH_TRIGGER
	stopwatch_reg_write REG_STOPWATCH_TRIGGER, (0x1 << TRIGGER_BIT)
.endm

.macro STOPWATCH_CLEAR
	stopwatch_reg_write REG_STOPWATCH_TRIGGER, (0x1 << CLEAR_BIT)
.endm

#else /* __ASSEMBLY__ */

/* stopwatch function */
void stopwatch_print(void);
int stopwatch_clear(void);
int stopwatch_trigger(void);
void timestamp_test(void);

#endif /* __ASSEMBLY__ */
#endif /* End of #ifndef TIMESTAMP_H */
