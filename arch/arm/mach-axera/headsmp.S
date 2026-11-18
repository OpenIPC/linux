/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  Cloned from linux/arch/arm/mach-realview/headsmp.S
 *
 *  Copyright (c) 2003 ARM Limited
 *  All Rights Reserved
 */
#include <linux/linkage.h>
#include <linux/init.h>

#include <asm/assembler.h>

/*
 * axera specific entry point for secondary CPUs.  This provides
 * a "holding pen" into which all secondary cores are held until we're
 * ready for them to initialise.
 */
ENTRY(secondary_holding_pen)
	mrc	p15, 0, r0, c0, c0, 5
	and	r0, r0, #15
	cmp r0, #CONFIG_NR_CPUS
	blt 2f
3:
	wfi
	b 3b
2:
	adr	r4, 1f
	ldmia	r4, {r5, r6, r8}
	sub	r4, r4, r5
	add	r6, r6, r4
	add r8, r8, r4
pen:
	ldr	r7, [r6]
	cmp	r7, r0
	bxeq r8
	b	pen
ENDPROC(secondary_holding_pen)

	.align 2
1:	.long	.
	.long	pen_release
	.long	secondary_startup
