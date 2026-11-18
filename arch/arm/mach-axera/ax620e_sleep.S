/*
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/memory.h>
#include <asm/cp15.h>
#include "ax620e_pm_reg.h"

	.align	2
	.arm
ENTRY(ax620e_slp_cpu_resume)

	@eic mask clr
	ldr r0, =COMMON_SYS_GLB_PHY_BASE
	ldr r1, =COMMON_SYS_EIC_MASK_ENABLE_CLR_ADDR
	add r0, r0, r1
	mov r1, #1
	str r1, [r0]

#ifdef CONFIG_THUMB2_KERNEL
	badr    r9, 1f			@ Kernel is entered in ARM.
	bx  r9				@ If this is a Thumb-2 kernel,
	.thumb				@ switch to Thumb now.
1:
#endif
	cpsid aif, #SVC_MODE

	mrrc p15, 1, r0, r1, c15 //read CPUECTLR
	orr r0, r0, #0x40		@Enable SMP mode
	mcrr p15, 1, r0, r1, c15 //write CPUECTLR

	mrc	p15, 0, r1, c0, c0, 5
	and	r1, r1, #0xf
	cmp	r1, #0
	/* olny cpu0 can continue to run, the others is halt here */
	beq	cpu0run

	/* sometime boot cores less than real processor cores,
     * the unused cores enter wfi state
	 */
	cmp r1, #CONFIG_NR_CPUS
	blt secondary_run
wfi_loop:
	wfi
	b wfi_loop

secondary_run:
	ldr r0, =PEN_RELEASE_PHY_ADDR
	ldr r0, [r0]      @ pen_release physical addr

	ldr r8, =SECONDARY_STARTUP_PHY_ADDR
	ldr r8, [r8]       @ secondary_startup physical addr
#ifdef CONFIG_THUMB2_KERNEL
	orr r8, r8, #1
#endif
	/* secondary cores enter wfe before ddr ready */
	wfe
pen:
	ldr	r2, [r0]
	cmp	r2, r1
	bxeq r8
	b	pen

cpu0run:
	/* cpu sys wakeup here */
	ldr sp, =#IRAM_SP_TEMP_ADDR
	ldr r1, =#CPU_SYS_SLEEP_CODE_ADDR
	mov r0, #0
#ifdef CONFIG_THUMB2_KERNEL
	orr r1, r1, #1
#endif
	blx r1

	/* ddr reinit here*/
	ldr r1, =#DDR_SYS_SLEEP_CODE_ADDR
	mov r0, #0x0
#ifdef CONFIG_THUMB2_KERNEL
	orr r1, r1, #1
#endif
	blx r1

	/* let secondary cores loop pen_release */
	sev

	/* run cpu_resume */
	ldr r0, =#CPU_RESUME_PHY_ADDR
	ldr r0, [r0]
#ifdef CONFIG_THUMB2_KERNEL
	orr r0, r0, #1
#endif
	bx r0
ENDPROC(ax620e_slp_cpu_resume)

ENTRY(ax620e_cpu_sleep_enter)
	/* after mmu off, r0 save axera_cpu_do_sleep physical addr in iram for ret */
	ldr r2, =ax620e_cpu_do_sleep
	ldr r3, =ax620e_slp_cpu_resume
	sub r2, r2 ,r3
	ldr r1, =#IRAM_BASE_PHY_ADDR
	add r1, r2, r1
#ifdef CONFIG_THUMB2_KERNEL
	orr r1, r1, #1
#endif

	adr r2, ax620e_disable_mmu
#ifdef CONFIG_THUMB2_KERNEL
	orr r2, r2, #1
#endif
	blx r2
ENDPROC(ax620e_cpu_sleep_enter)

/* r1: the physical addr of axera_cpu_do_sleep */
ENTRY(ax620e_disable_mmu)
	mrc p15, 0, r3, c1, c0, 0
	mov	r2, #CR_I | CR_Z | CR_C | CR_M
	bic r3, r3, r2
	dsb
	mcr p15, 0, r3, c1, c0, 0
	isb

	ret r1
ENDPROC(ax620e_disable_mmu)

ENTRY(ax620e_cpu_do_sleep)
	/* ddr sleep config */
	mov r0, #0x1
	ldr sp, =#IRAM_SP_TEMP_ADDR
	ldr r1, =#DDR_SYS_SLEEP_CODE_ADDR
#ifdef CONFIG_THUMB2_KERNEL
	orr r1, r1, #1
#endif
	blx r1

	/* cpu sleep config */
	mov r0, #0x1
	ldr r1, =#CPU_SYS_SLEEP_CODE_ADDR
#ifdef CONFIG_THUMB2_KERNEL
	orr r1, r1, #1
#endif
	blx r1

	dsb
	isb
	wfi

	mov r0, #1
	bx lr
ENDPROC(ax620e_cpu_do_sleep)
	.ltorg

ENTRY(ax620e_slp_resume_code_sz)
	.word   . - ax620e_slp_cpu_resume
