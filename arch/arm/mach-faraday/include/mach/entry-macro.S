/*
 *  arch/arm/mach-faraday/include/mach/entry-macro.S
 *
 *  Low-level IRQ helper macros for Faraday platforms
 *
 *  Copyright (C) 2005 Faraday Technology
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CONFIG_CPU_FMP626

#include <mach/ftintc010.h>

	.macro	disable_fiq
	.endm

	.macro	get_irqnr_preamble, base, tmp
	ldr	\base, =ftintc010_base_addr
	ldr	\base, [\base]
	.endm

	.macro	arch_ret_to_user, tmp1, tmp2
	.endm

	/*
	 * Get IRQ number and base
	 * Input: base
	 * Output: irqnr, Z flag
	 * Local R/W: irqstat, tmp
	 */
	.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp

	ldr	\irqstat, [\base, #FTINTC010_OFFSET_IRQSTATUS]
	cmp	\irqstat, #0

#ifndef CONFIG_FTINTC010EX
	beq	1001f
#endif
	movne	\irqnr, #0

#ifdef CONFIG_FTINTC010EX
	bne	1002f

	ldr	\irqstat, [\base, #FTINTC010_OFFSET_IRQSTATUSEX]
	cmp	\irqstat, #0

	beq	1001f
	mov	\irqnr, #32
1002:
#endif

#if __LINUX_ARM_ARCH__ >= 5
	clz	\tmp, \irqstat
	rsb	\tmp, \tmp, #31
	add	\irqnr, \irqnr, \tmp
#else
	movs	\tmp, \irqstat, lsl #16
	movne	\irqstat, \tmp
	addeq	\irqnr, \irqnr, #16

	movs	\tmp, \irqstat, lsl #8
	movne	\irqstat, \tmp
	addeq	\irqnr, \irqnr, #8

	movs	\tmp, \irqstat, lsl #4
	movne	\irqstat, \tmp
	addeq	\irqnr, \irqnr, #4

	movs	\tmp, \irqstat, lsl #2
	movne	\irqstat, \tmp
	addeq	\irqnr, \irqnr, #2

	movs	\tmp, \irqstat, lsl #1
	addeqs	\irqnr, \irqnr, #1
#endif

1001:
	.endm	/* get_irqnr_and_base */

#else	/* CONFIG_CPU_FMP626 */

	.macro	disable_fiq
	.endm

	.macro	arch_ret_to_user, tmp1, tmp2
	.endm

#endif	/* CONFIG_CPU_FMP626 */
