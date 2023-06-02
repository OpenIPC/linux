#include <mach/io.h>
#include <mach/irqs.h>

		.macro	disable_fiq
		.endm

		.macro  get_irqnr_preamble, base, tmp
		ldr \base, =VA_INTC_REG_BASE
		.endm

		.macro  arch_ret_to_user, tmp1, tmp2
		.endm

		.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp
		@ check low interrupts
		ldr		\irqstat, [\base, #0x30]
		mov		\irqnr, #31
		ands	\irqstat, \irqstat, #0xffffffff

		@ if no low interrupts set, check high interrupts
		ldreq	\irqstat, [\base, #0x34]
		moveq	\irqnr, #63
		andeqs	\irqstat, \irqstat, #0xffffffff

		@ find first active interrupt source
		clzne	\irqstat, \irqstat
		subne	\irqnr, \irqnr, \irqstat
		.endm

		.macro	irq_prio_table
		.endm
