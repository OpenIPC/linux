/*
 *  linux/arch/arm/mm/proc-fa626te.S: MMU functions for FA626TE
 *
 *  Written by : Luke Lee
 *  Copyright (C) 2005 Faraday Corp.
 *  Copyright (C) 2008-2009 Paulius Zaleckas <paulius.zaleckas@teltonika.lt>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *
 * These are the low level assembler for performing cache and TLB
 * functions on the fa626te.
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/assembler.h>
#include <asm/hwcap.h>
#include <asm/pgtable-hwdef.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/ptrace.h>
#include <asm/system.h>

#include "proc-macros.S"

#define CACHE_DLINESIZE	32

	.text
/*
 * cpu_fa626te_proc_init()
 */
ENTRY(cpu_fa626te_proc_init)
	mov	pc, lr

/*
 * cpu_fa626te_proc_fin()
 */
ENTRY(cpu_fa626te_proc_fin)
	mrc	p15, 0, r0, c1, c0, 0		@ ctrl register
	bic	r0, r0, #0x1000			@ ...i............
	bic	r0, r0, #0x000e			@ ............wca.
	mcr	p15, 0, r0, c1, c0, 0		@ disable caches
	nop
	nop
	mov	pc, lr

/*
 * cpu_fa626te_reset(loc)
 *
 * Perform a soft reset of the system.  Put the CPU into the
 * same state as it would be if it had been reset, and branch
 * to what would be the reset vector.
 *
 * loc: location to jump to for soft reset
 */
	.align	4
	.pushsection    .idmap.text, "ax"
ENTRY(cpu_fa626te_reset)
/* TODO: Use CP8 if possible... */
	mov	ip, #0
	mcr	p15, 0, ip, c7, c7, 0		@ invalidate I,D caches
	mcr	p15, 0, ip, c7, c10, 4		@ drain WB
#ifdef CONFIG_MMU
	mcr	p15, 0, ip, c8, c7, 0		@ invalidate I & D TLBs
#endif
	mrc	p15, 0, ip, c1, c0, 0		@ ctrl register
	bic	ip, ip, #0x000f			@ ............wcam
	bic	ip, ip, #0x1100			@ ...i...s........
	bic	ip, ip, #0x0800			@ BTB off
	mcr	p15, 0, ip, c1, c0, 0		@ ctrl register
	nop
	nop
	mov	pc, r0
ENDPROC(cpu_fa626te_reset)
	.popsection

/*
 * cpu_fa626te_do_idle()
 */
	.align	4
ENTRY(cpu_fa626te_do_idle)
	mrs	r0, cpsr
	bic	r0, r0, #0x80			@ enable irq
	msr	cpsr_c, r0
	mcr	p15, 0, r0, c7, c0, 4		@ Wait for interrupt
	orr	r0, r0, #0x80			@ disable irq
	msr	cpsr_c, r0
	mov	pc, lr


ENTRY(cpu_fa626te_dcache_clean_area)
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #CACHE_DLINESIZE
	subs	r1, r1, #CACHE_DLINESIZE
	bhi	1b
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

/* =============================== PageTable ============================== */

/*
 * cpu_fa626te_switch_mm(pgd)
 *
 * Set the translation base pointer to be as described by pgd.
 *
 * pgd: new page tables
 */
	.align	4
ENTRY(cpu_fa626te_switch_mm)
#ifdef CONFIG_MMU
	mov	ip, #0
	mcr	p15, 0, ip, c7, c5, 6		@ invalidate BTB since mm changed
	mcr	p15, 0, ip, c7, c10, 4		@ data write barrier
	mcr	p15, 0, r0, c2, c0, 0		@ load page table pointer
	mcr	p15, 0, ip, c8, c7, 0		@ invalidate UTLB
#endif
	mov	pc, lr

/*
 * cpu_fa626te_set_pte_ext(ptep, pte, ext)
 *
 * Set a PTE and flush it out
 */
	.align	4
ENTRY(cpu_fa626te_set_pte_ext)
#ifdef CONFIG_MMU
	armv3_set_pte_ext
	mov	r0, r0
	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
#endif
	mov	pc, lr

	__CPUINIT

	.type	__fa626te_setup, #function
__fa626te_setup:
	/* On return of this routine, r0 must carry correct flags for CFG register */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7		@ invalidate I,D caches on v4
	mcr	p15, 0, r0, c7, c10, 4		@ drain write buffer on v4
#ifdef CONFIG_MMU
	mcr	p15, 0, r0, c8, c7		@ invalidate I,D TLBs on v4
#endif
	mcr	p15, 0, r0, c7, c5, 5		@ invalidate IScratchpad RAM

	mcr	p15, 0, r0, c7, c5, 6		@ invalidate BTB All
	mcr	p15, 0, r0, c7, c10, 4		@ data write barrier
	mcr	p15, 0, r0, c7, c5, 4		@ prefetch flush

	mov	r0, #0x1f			@ Domains 0, 1 = manager, 2 = client
	mcr	p15, 0, r0, c3, c0		@ load domain access register

	adr	r5, fa626te_crval
	ldmia	r5, {r5, r6}
	mrc	p15, 0, r0, c1, c0		@ get control register v4
	bic	r0, r0, r5
	orr	r0, r0, r6
	mov	pc, lr
	.size	__fa626te_setup, . - __fa626te_setup

	/*
	 * .RVI ZFRS BLDP WCAM
	 * ..11 1001 .111 1101
	 *
	 */
	.type	fa626te_crval, #object
fa626te_crval:
	crval	clear=0x3f7f, mmuset=0x397D, ucset=0x3174

	__INITDATA

	@ define struct processor (see <asm/proc-fns.h> and proc-macros.S)
	define_processor_functions fa626te, dabort=v5t_early_abort, pabort=legacy_pabort

	.section ".rodata"

	string	cpu_arch_name, "armv5te"
	string	cpu_elf_name, "v5"
	string	cpu_fa626te_name, "FA6"

	.align

	.section ".proc.info.init", #alloc, #execinstr

	.type	__fa626te_proc_info,#object
__fa626te_proc_info:
	.long	0x66056260
	.long	0xff0ffff0
	.long   PMD_TYPE_SECT | \
		PMD_SECT_BUFFERABLE | \
		PMD_SECT_CACHEABLE | \
		PMD_BIT4 | \
		PMD_SECT_AP_WRITE | \
		PMD_SECT_AP_READ
	.long   PMD_TYPE_SECT | \
		PMD_BIT4 | \
		PMD_SECT_AP_WRITE | \
		PMD_SECT_AP_READ
	b	__fa626te_setup
	.long	cpu_arch_name
	.long	cpu_elf_name
	.long	HWCAP_SWP | HWCAP_HALF | HWCAP_THUMB
	.long	cpu_fa626te_name
	.long	fa626te_processor_functions
	.long	fa_tlb_fns
	.long	fa_user_fns
	.long	fa_cache_fns
	.size	__fa626te_proc_info, . - __fa626te_proc_info
