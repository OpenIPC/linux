/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *  linux/arch/arm/mm/proc-fa626te.S: MMU functions for FA626TE
 *
 *  Written by : Luke Lee
 *  Copyright (C) 2005 Faraday Corp.
 *  Copyright (C) 2008-2009 Paulius Zaleckas <paulius.zaleckas@teltonika.lt>
 *
 * These are the low level assembler for performing cache and TLB
 * functions on the fa626te. From proc-fa526.S, with the differences of the
 * vendor GM8126 2.6.28 and GM8136 3.3 proc-fa626te.S.
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <linux/cfi_types.h>
#include <linux/pgtable.h>
#include <asm/assembler.h>
#include <asm/hwcap.h>
#include <asm/pgtable-hwdef.h>
#include <asm/page.h>
#include <asm/ptrace.h>

#include "proc-macros.S"

#define CACHE_DLINESIZE	32

	.text
/*
 * cpu_fa626te_proc_init()
 */
SYM_TYPED_FUNC_START(cpu_fa626te_proc_init)
	ret	lr
SYM_FUNC_END(cpu_fa626te_proc_init)

/*
 * cpu_fa626te_proc_fin()
 */
SYM_TYPED_FUNC_START(cpu_fa626te_proc_fin)
	mrc	p15, 0, r0, c1, c0, 0		@ ctrl register
	bic	r0, r0, #0x1000			@ ...i............
	bic	r0, r0, #0x000e			@ ............wca.
	mcr	p15, 0, r0, c1, c0, 0		@ disable caches
	nop
	nop
	ret	lr
SYM_FUNC_END(cpu_fa626te_proc_fin)

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
	.pushsection	.idmap.text, "ax"
SYM_TYPED_FUNC_START(cpu_fa626te_reset)
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
	ret	r0
SYM_FUNC_END(cpu_fa626te_reset)
	.popsection

/*
 * cpu_fa626te_do_idle()
 */
	.align	4
SYM_TYPED_FUNC_START(cpu_fa626te_do_idle)
	ret	lr
SYM_FUNC_END(cpu_fa626te_do_idle)

SYM_TYPED_FUNC_START(cpu_fa626te_dcache_clean_area)
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #CACHE_DLINESIZE
	subs	r1, r1, #CACHE_DLINESIZE
	bhi	1b
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	ret	lr
SYM_FUNC_END(cpu_fa626te_dcache_clean_area)

/* =============================== PageTable ============================== */

/*
 * cpu_fa626te_switch_mm(pgd)
 *
 * Set the translation base pointer to be as described by pgd.
 *
 * pgd: new page tables
 */
	.align	4
SYM_TYPED_FUNC_START(cpu_fa626te_switch_mm)
#ifdef CONFIG_MMU
	mov	ip, #0
#ifdef CONFIG_CPU_DCACHE_WRITETHROUGH
	mcr	p15, 0, ip, c7, c6, 0		@ invalidate D cache
#else
	mcr	p15, 0, ip, c7, c14, 0		@ clean and invalidate whole D cache
#endif
	mcr	p15, 0, ip, c7, c5, 0		@ invalidate I cache
	mcr	p15, 0, ip, c7, c5, 6		@ invalidate BTB since mm changed
	mcr	p15, 0, ip, c7, c10, 4		@ data write barrier
	mcr	p15, 0, ip, c7, c5, 4		@ prefetch flush
	mcr	p15, 0, r0, c2, c0, 0		@ load page table pointer
	mcr	p15, 0, ip, c8, c7, 0		@ invalidate UTLB
#endif
	ret	lr
SYM_FUNC_END(cpu_fa626te_switch_mm)

/*
 * cpu_fa626te_set_pte_ext(ptep, pte, ext)
 *
 * Set a PTE and flush it out
 */
	.align	4
SYM_TYPED_FUNC_START(cpu_fa626te_set_pte_ext)
#ifdef CONFIG_MMU
	armv3_set_pte_ext
	mov	r0, r0
	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
#endif
	ret	lr
SYM_FUNC_END(cpu_fa626te_set_pte_ext)

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

	mrc	p15, 0, r0, c9, c1, 0		@ data scratchpad off
	bic	r0, r0, #1
	mcr	p15, 0, r0, c9, c1, 0
	mrc	p15, 0, r0, c9, c1, 1		@ instruction scratchpad off
	bic	r0, r0, #1
	mcr	p15, 0, r0, c9, c1, 1

	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 6		@ invalidate BTB All
	mcr	p15, 0, r0, c7, c10, 4		@ data write barrier
	mcr	p15, 0, r0, c7, c5, 4		@ prefetch flush

	mov	r0, #0x1f			@ Domains 0, 1 = manager, 2 = client
	mcr	p15, 0, r0, c3, c0		@ load domain access register

	mrc	p15, 0, r0, c1, c0		@ get control register v4
	ldr	r5, fa626te_cr1_clear
	bic	r0, r0, r5
	ldr	r5, fa626te_cr1_set
	orr	r0, r0, r5
	ret	lr
	.size	__fa626te_setup, . - __fa626te_setup

	/*
	 * .RVI ZFRS BLDP WCAM
	 * ..11 1001 .111 1101
	 *
	 */
	.type	fa626te_cr1_clear, #object
	.type	fa626te_cr1_set, #object
fa626te_cr1_clear:
	.word	0x3f7f
fa626te_cr1_set:
	.word	0x397D

	__INITDATA

	@ define struct processor (see <asm/proc-fns.h> and proc-macros.S)
	define_processor_functions fa626te, dabort=v5t_early_abort, pabort=legacy_pabort

	.section ".rodata"

	string	cpu_arch_name, "armv5te"
	string	cpu_elf_name, "v5"
	string	cpu_fa626te_name, "FA626TE"

	.align

	.section ".proc.info.init", "a"

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
	initfn	__fa626te_setup, __fa626te_proc_info
	.long	cpu_arch_name
	.long	cpu_elf_name
	.long	HWCAP_SWP | HWCAP_HALF | HWCAP_THUMB
	.long	cpu_fa626te_name
	.long	fa626te_processor_functions
	.long	fa_tlb_fns
	.long	fa_user_fns
	.long	fa_cache_fns
	.size	__fa626te_proc_info, . - __fa626te_proc_info
