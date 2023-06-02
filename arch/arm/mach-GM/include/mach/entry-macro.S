/*
 * arch/arm/mach-GM/include/mach/entry-macro.S
 *
 * Faraday Low-level Interrupt Vector Interface Macros
 *
 * Copyright (C) 2005 Faraday Corp. (http://www.faraday-tech.com)
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

#include <mach/irqs.h>
#include <mach/ftintc030.h>
#include <mach/ftintc010.h>

	.macro	check_status, irqstat, tmp
	//???
	stmfd	sp!, {r7}
	ldr	r7, =ftintc030_base_cpu_0_irq_base
	ldr	r7, [r7]
	mov 	r4, \tmp
	mov	r4, r4, lsr #3				/* >>= 5, then << 2 calc INT offset */
	ldr	r4, [r7, r4]
	and	\irqstat, \irqstat, r4
	ldmfd	sp!, {r7}
	.endm

	.macro	disable_fiq
	.endm

	.macro	get_irqnr_preamble, base, tmp
#ifdef CONFIG_FTINTC030
	ldr	\base, =ftintc030_base_addr
#else
	ldr	\base, =ftintc010_base_addr
#endif
	ldr	\base, [\base]
	.endm

	.macro	arch_ret_to_user, tmp1, tmp2
	.endm

#if __LINUX_ARM_ARCH__ >= 5
	.macro	__get_int_nr_and_base, irqnr, irqstat, tmp
	clz	\irqnr, \irqstat				/* 32 if no bits are set, and zero if bit 31 is set */
	rsb	\irqnr, \irqnr, #31			/* irqnr = 31 - irqnr, get irq number */
	.endm
#else
	/*
	 * An O(1) optimized version for getting IRQ/FIQ number
	 * 08/29/2005 Luke Lee
	 * Input/output: irqnr (initial value), irqstat (the word to scan)
	 * Local R/W:	tmp
	 */
	.macro	__get_int_nr_and_base, irqnr, irqstat, tmp
	mov	\tmp, \irqstat, lsl #16	/* check if lower 16 bits = zero */
	cmp	\tmp, #0
	movne	\irqstat, \irqstat, lsl #16	/* irqstat <<= 16 */
	subne	\irqnr, \irqnr, #16	/* irqnr -= 16 */
	tst	\irqstat, #0x00FF0000
	movne	\irqstat, \irqstat, lsl #8	/* irqstat <<= 8 */
	subne	\irqnr, \irqnr, #8	/* irqnr -= 8 */
	tst	\irqstat, #0x0F000000
	movne	\irqstat, \irqstat, lsl #4	/* irqstat <<= 4 */
	subne	\irqnr, \irqnr, #4	/* irqnr -= 4 */
	tst	\irqstat, #0x30000000
	movne	\irqstat, \irqstat, lsl #2	/* irqstat <<= 2 */
	subne	\irqnr, \irqnr, #2	/* irqnr -= 2 */
	tst	\irqstat, #0x40000000
	subne	\irqnr, \irqnr, #1	/* irqnr -= 1 */
	.endm
#endif

#ifdef CONFIG_FTINTC010EX
	/*
	 * Get EXTEND IRQ number and base
	 * Input: base
	 * Output: irqnr, Z flag
	 * Local R/W: irqstat, tmp, base, offset
   * 					base: irq IP base address
   * 					offset: register offset
	 */
	.macro	__get_ex_nr, irqnr, irqstat, tmp, base, offset
	ldr	\irqstat, [\base, \offset]
	cmp	\irqstat, #0
	beq	15f
	mov	\irqnr, #31

	__get_int_nr_and_base	\irqnr, \irqstat, \tmp

	add	\irqnr, \irqnr, #32
	cmp	\irqnr, #NR_IRQS
15:
	.endm	/* get_ex_nr */
#endif

#ifdef CONFIG_FTINTC030
	/*
	 * Get SPI INTC030 IRQ number and base
	 * Input: base
	 * Output: irqnr, Z flag
	 * Local R/W: irqstat, tmp, base, r5
   * 					irqstat: spi status
   * 					tmp: register offset
   * 					base: irq IP base address
   * 					offset: register offset
   * 					r3: irq number base
	 */
	.macro	__get_spi_nr, irqnr, irqstat, tmp, base, offset, mode
	ldr	\irqstat, [\base, \offset]	/* read IRQ status, irq group 1 */

	add \base, \base, #0x20
	mov \tmp, #32
	mov r3, #32										/* move spi base to 32 */
//	???
	check_status \irqstat, r3
	cmp	\irqstat, #0
	beq	7f

	mov	\irqnr, #31								/* irq between this range */
6:
	sub \base, \base, \tmp
	__get_int_nr_and_base	\irqnr, \irqstat, \tmp

	add	\irqnr, \irqnr, r3					/* add irq base and return value */
	cmp	r3, #(NR_IRQS - 32)
	b 9f
7:
	cmp	r3, #(NR_IRQS - 32)				/* check irq overflow */
	bge	9f
	add \tmp, \tmp, #0x20					/* shift to next register offset */
	add \base, \base, #0x20
	ldr	\irqstat, [\base, \offset]	/* read IRQ status, irq group 2~14 */
	add	r3, r3, #32								/* add IRQ number base */
//	???
	check_status \irqstat, r3
	cmp	\irqstat, #0
	beq	7b
	b 6b
9:

	.endm	/* get_spi_nr */
#endif

	/*
	 * Get IRQ number and base
	 * Input: base
	 * Output: irqnr, Z flag
	 * Local R/W: irqstat, tmp
	 */
	.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp

	mov	\irqnr, #0
#ifdef CONFIG_FTINTC030
	ldr	\irqstat, [\base, #FTINTC030_OFFSET_IRQPEND]	/* irq group 0 */

	check_status \irqstat, \irqnr
//???
#else
	ldr	\irqstat, [\base, #FTINTC010_OFFSET_IRQSTATUS]/* irq group 0 */
#endif

	cmp	\irqstat, #0
	beq	3f

	__get_int_nr_and_base	\irqnr, \irqstat, \tmp
	cmp	\irqnr, #NR_IRQS
#if (defined(CONFIG_FTINTC010EX) || defined(CONFIG_FTINTC030))
	b	5f
#endif
3:
#ifdef CONFIG_FTINTC010EX
	__get_ex_nr	\irqnr, \irqstat, \tmp, \base, #FTINTC010_OFFSET_IRQSTATUSEX	/* irq group 1 */
5:
#endif
#ifdef CONFIG_FTINTC030
	__get_spi_nr	\irqnr, \irqstat, \tmp, \base, #FTINTC030_OFFSET_IRQPEND1	, #0	/* irq group 1~14 */
	cmp \irqnr, #0
5:
#endif
	.endm	/* get_irqnr_and_base */

#ifdef CONFIG_FIQ
    /*
    * Get FIQ number and base
	* Input: none
	* Output: irqnr, Z flag
	* Local R/W: irqstat, base, tmp
	*/
	.macro get_fiqnr_and_base, irqnr, irqstat, base, tmp
#ifdef CONFIG_FTINTC030
	ldr \irqstat, [\base, #FTINTC030_OFFSET_IRQPEND]		/* fiq group 0 */
#else
	ldr \irqstat, [\base, #FTINTC010_OFFSET_FIQSTATUS]	/* fiq group 0 */
#endif
	cmp	\irqstat, #0
	beq	2003f
	mov	\irqnr, #0
	__get_int_nr_and_base	\irqnr, \irqstat, \tmp
	cmp	\irqnr, #NR_IRQS
#if (defined(CONFIG_FTINTC010EX) || defined(CONFIG_FTINTC030))
    b   2005f
#endif
2003:
#ifdef CONFIG_FTINTC010EX
	__get_ex_nr	\irqnr, \irqstat, \tmp, \base, #FTINTC010_OFFSET_FIQSTATUSEX	/* fiq group 1 */
2005:
#endif
#ifdef CONFIG_FTINTC030
	__get_spi_nr	\irqnr, \irqstat, \tmp, \base, #FTINTC030_OFFSET_IRQPEND1, #1		/* fiq group 1~14 */
2005:
#endif
    .endm	/* get_fiqnr_and_base */

#endif /* CONFIG_FIQ */


#else	/* CONFIG_CPU_FMP626 */

#include <mach/hardware.h>
#include <asm/hardware/gic.h>

	.macro	disable_fiq
	.endm

	.macro	get_irqnr_preamble, base, tmp
	ldr	\base, =gic_cpu_base_addr
	ldr	\base, [\base]
	.endm

	.macro	arch_ret_to_user, tmp1, tmp2
	.endm

	/*
	 * The interrupt numbering scheme is defined in the
	 * interrupt controller spec.  To wit:
	 *
	 * Interrupts 0-15 are IPI
	 * 16-28 are reserved
	 * 29-31 are local.  We allow 30 to be used for the watchdog.
	 * 32-1020 are global
	 * 1021-1022 are reserved
	 * 1023 is "spurious" (no interrupt)
	 *
	 * For now, we ignore all local interrupts so only return an interrupt if it's
	 * between 30 and 1020.  The test_for_ipi routine below will pick up on IPIs.
	 *
	 * A simple read from the controller will tell us the number of the highest
	 * priority enabled interrupt.  We then just need to check whether it is in the
	 * valid range for an IRQ (30-1020 inclusive).
	 */

	.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp

	ldr	\irqstat, [\base, #GIC_CPU_INTACK] /* bits 12-10 = src CPU, 9-0 = int # */

	ldr	\tmp, =1021

	bic	\irqnr, \irqstat, #0x1c00

	cmp	\irqnr, #29
	cmpcc	\irqnr, \irqnr
	cmpne	\irqnr, \tmp
	cmpcs	\irqnr, \irqnr

	.endm

	/* We assume that irqstat (the raw value of the IRQ acknowledge
	 * register) is preserved from the macro above.
	 * If there is an IPI, we immediately signal end of interrupt on the
	 * controller, since this requires the original irqstat value which
	 * we won't easily be able to recreate later.
	 */

	.macro	test_for_ipi, irqnr, irqstat, base, tmp
	bic	\irqnr, \irqstat, #0x1c00
	cmp	\irqnr, #16
	strcc	\irqstat, [\base, #GIC_CPU_EOI]
	cmpcs	\irqnr, \irqnr
	.endm

	/* As above, this assumes that irqstat and base are preserved.. */

	.macro	test_for_ltirq, irqnr, irqstat, base, tmp
	bic	\irqnr, \irqstat, #0x1c00
	mov 	\tmp, #0
	cmp	\irqnr, #29
	moveq	\tmp, #1
	streq	\irqstat, [\base, #GIC_CPU_EOI]
	cmp	\tmp, #0
	.endm

#endif	/* CONFIG_CPU_FMP626 */
