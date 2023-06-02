/*
 *  linux/arch/arm/mm/proc-fmp626.S: MMU functions for Faraday FMP626
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
 *
 *
 * These are the low level assembler for performing cache and TLB
 * functions on the fmp626.
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/assembler.h>
#include <asm/hwcap.h>
#include <asm/pgtable-hwdef.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/ptrace.h>
#include "proc-macros.S"

/*
 * This is the maximum size of an area which will be invalidated
 * using the single invalidate entry instructions.  Anything larger
 * than this, and we go for the whole cache.
 *
 * This value should be chosen such that we choose the cheapest
 * alternative.
 */
#define CACHE_DLIMIT	16384

/*
 * the cache line size of the I and D cache
 */
#define CACHE_DLINESIZE	32

#define TTB_S		(1 << 1)
#define TTB_RGN_NC	(0 << 3)
#define TTB_RGN_WBWA	(1 << 3)
#define TTB_RGN_WT	(2 << 3)
#define TTB_RGN_WB	(3 << 3)

#ifndef CONFIG_SMP
#define TTB_FLAGS	TTB_RGN_WBWA
#else
#define TTB_FLAGS	TTB_RGN_WBWA|TTB_S
#endif

	.text
/*
 * cpu_fmp626_proc_init()
 */
ENTRY(cpu_fmp626_proc_init)
	mov	pc, lr

/*
 * cpu_fmp626_proc_fin()
 */
ENTRY(cpu_fmp626_proc_fin)
	mrc	p15, 0, r0, c1, c0, 0		@ ctrl register
	bic	r0, r0, #0x1000			@ ...i............
	bic	r0, r0, #0x000e			@ ............wca.
	mcr	p15, 0, r0, c1, c0, 0		@ disable caches
	mov	pc, lr

/*
 * cpu_fmp626_reset(loc)
 *
 * Perform a soft reset of the system.  Put the CPU into the
 * same state as it would be if it had been reset, and branch
 * to what would be the reset vector.
 *
 * loc: location to jump to for soft reset
 */
	.align	5
	.pushsection	.idmap.text, "ax"
ENTRY(cpu_fmp626_reset)
	mov	ip, #0
	mcr	p15, 0, ip, c7, c7, 0		@ invalidate I,D caches
	mcr	p15, 0, ip, c7, c10, 4		@ drain WB
#ifdef CONFIG_MMU
	mcr	p15, 0, ip, c8, c7, 0		@ invalidate I & D TLBs
#endif
	mrc	p15, 0, ip, c1, c0, 0		@ ctrl register
	bic	ip, ip, #0x000f			@ ............wcam
	bic	ip, ip, #0x1100			@ ...i...s........
	mcr	p15, 0, ip, c1, c0, 0		@ ctrl register
	nop
	nop
	mov	pc, r0
ENDPROC(cpu_fmp626_reset)
	.popsection

/*
 * cpu_fmp626_do_idle()
 *
 * Called with IRQs disabled
 */
	.align	10
ENTRY(cpu_fmp626_do_idle)
	mcr	p15, 0, r0, c7, c0, 4		@ Wait for interrupt
	mov	pc, lr

/*
 *	flush_icache_all()
 *
 *	Flush the whole I-cache.
 */
ENTRY(fmp626_flush_icache_all)
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		@ invalidate I-cache all
	mov	pc,lr

/*
 *	flush_user_cache_all()
 *
 *	Clean and invalidate all cache entries in a particular
 *	address space.
 */
ENTRY(fmp626_flush_user_cache_all)
	/* FALLTHROUGH */

/*
 *	flush_kern_cache_all()
 *
 *	Clean and invalidate the entire cache.
 */
ENTRY(fmp626_flush_kern_cache_all)
	mov	r2, #VM_EXEC
	mov	ip, #0
__flush_whole_cache:
#ifdef CONFIG_CPU_DCACHE_WRITETHROUGH
	mcr	p15, 0, ip, c7, c6, 0		@ invalidate D cache all
#else
	mcr	p15, 0, ip, c7, c14, 0		@ clean and invalidate D cache all
#endif
	tst	r2, #VM_EXEC
	mcrne	p15, 0, ip, c7, c5, 0		@ invalidate I cache all
	mcrne	p15, 0, ip, c7, c10, 4		@ drain WB
	mov	pc, lr

/*
 *	flush_user_cache_range(start, end, flags)
 *
 *	Clean and invalidate a range of cache entries in the
 *	specified address range.
 *
 *	- start	- start address (inclusive)
 *	- end	- end address (exclusive)
 *	- flags	- vm_flags describing address space
 */
ENTRY(fmp626_flush_user_cache_range)
	mov	ip, #0
	sub	r3, r1, r0			@ calculate total size
	cmp	r3, #CACHE_DLIMIT
	bgt	__flush_whole_cache
1:	tst	r2, #VM_EXEC
#ifdef CONFIG_CPU_DCACHE_WRITETHROUGH
	mcr	p15, 0, r0, c7, c6, 1		@ invalidate D entry
	mcrne	p15, 0, r0, c7, c5, 1		@ invalidate I entry
	add	r0, r0, #CACHE_DLINESIZE
#else
	mcr	p15, 0, r0, c7, c14, 1		@ clean and invalidate D entry
	mcrne	p15, 0, r0, c7, c5, 1		@ invalidate I entry
	add	r0, r0, #CACHE_DLINESIZE
#endif
	cmp	r0, r1
	blo	1b
	tst	r2, #VM_EXEC
	mcrne	p15, 0, ip, c7, c10, 4		@ drain WB
	mov	pc, lr

/*
 *	coherent_kern_range(start, end)
 *
 *	Ensure coherency between the Icache and the Dcache in the
 *	region described by start, end.  If you have non-snooping
 *	Harvard caches, you need to implement this function.
 *
 *	- start	- virtual start address
 *	- end	- virtual end address
 */
ENTRY(fmp626_coherent_kern_range)
	/* FALLTHROUGH */

/*
 *	coherent_user_range(start, end)
 *
 *	Ensure coherency between the Icache and the Dcache in the
 *	region described by start, end.  If you have non-snooping
 *	Harvard caches, you need to implement this function.
 *
 *	- start	- virtual start address
 *	- end	- virtual end address
 */
ENTRY(fmp626_coherent_user_range)
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	mcr	p15, 0, r0, c7, c5, 1		@ invalidate I entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

/*
 *	flush_kern_dcache_area(void *addr, size_t size)
 *
 *	Ensure no D cache aliasing occurs, either with itself or
 *	the I cache
 *
 *	- addr	- kernel address
 *	- size	- size of region
 */
ENTRY(fmp626_flush_kern_dcache_area)
	add	r1, r0, r1
1:	mcr	p15, 0, r0, c7, c14, 1		@ clean+invalidate D entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		@ invalidate I cache
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

/*
 *	dma_inv_range(start, end)
 *
 *	Invalidate (discard) the specified virtual address range.
 *	May not write back any entries.  If 'start' or 'end'
 *	are not cache line aligned, those lines must be written
 *	back.
 *
 *	- start	- virtual start address
 *	- end	- virtual end address
 *
 * (same as v4wb)
 */
fmp626_dma_inv_range:
#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH
	tst	r0, #CACHE_DLINESIZE - 1
	mcrne	p15, 0, r0, c7, c10, 1		@ clean D entry
	tst	r1, #CACHE_DLINESIZE - 1
	mcrne	p15, 0, r1, c7, c10, 1		@ clean D entry
#endif
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:	mcr	p15, 0, r0, c7, c6, 1		@ invalidate D entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

/*
 *	dma_clean_range(start, end)
 *
 *	Clean the specified virtual address range.
 *
 *	- start	- virtual start address
 *	- end	- virtual end address
 *
 * (same as v4wb)
 */
fmp626_dma_clean_range:
#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
#endif
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

/*
 *	dma_flush_range(start, end)
 *
 *	Clean and invalidate the specified virtual address range.
 *
 *	- start	- virtual start address
 *	- end	- virtual end address
 */
ENTRY(fmp626_dma_flush_range)
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:
#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH
	mcr	p15, 0, r0, c7, c14, 1		@ clean+invalidate D entry
#else
	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
#endif
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

/*
 *	dma_map_area(start, size, dir)
 *	- start	- kernel virtual start address
 *	- size	- size of region
 *	- dir	- DMA direction
 */
ENTRY(fmp626_dma_map_area)
	add	r1, r1, r0
	cmp	r2, #DMA_TO_DEVICE
	beq	fmp626_dma_clean_range
	bcs	fmp626_dma_inv_range
	b	fmp626_dma_flush_range
ENDPROC(fmp626_dma_map_area)

/*
 *	dma_unmap_area(start, size, dir)
 *	- start	- kernel virtual start address
 *	- size	- size of region
 *	- dir	- DMA direction
 */
ENTRY(fmp626_dma_unmap_area)
	mov	pc, lr
ENDPROC(fmp626_dma_unmap_area)

	@ define struct cpu_cache_fns (see <asm/cacheflush.h> and proc-macros.S)
	define_cache_functions fmp626

ENTRY(cpu_fmp626_dcache_clean_area)
#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #CACHE_DLINESIZE
	subs	r1, r1, #CACHE_DLINESIZE
	bhi	1b
#endif
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

/* =============================== PageTable ============================== */

/*
 * cpu_fmp626_switch_mm(pgd)
 *
 * Set the translation base pointer to be as described by pgd.
 *
 * pgd: new page tables
 */
	.align	5
ENTRY(cpu_fmp626_switch_mm)
#ifdef CONFIG_MMU
	mov	ip, #0
	orr	r0, r0, #TTB_FLAGS
#ifdef CONFIG_CPU_DCACHE_WRITETHROUGH
	mcr	p15, 0, ip, c7, c6, 0		@ invalidate D cache
#else
	mcr	p15, 0, ip, c7, c14, 0		@ clean and invalidate whole D cache
#endif
	mcr	p15, 0, ip, c7, c5, 0		@ invalidate I cache
	mcr	p15, 0, ip, c7, c10, 4		@ drain WB
	mcr	p15, 0, r0, c2, c0, 0		@ load page table pointer
	mcr	p15, 0, ip, c8, c7, 0		@ invalidate I & D TLBs
#endif
	mov	pc, lr

/*
 *	cpu_fmp626_set_pte_ext(ptep, pte, ext)
 *
 *	Set a PTE and flush it out
 *
 *	- ptep  - pointer to level 2 translation table entry
 *		  (hardware version is stored at -1024 bytes)
 *	- pte   - PTE value to store
 *	- ext	- value for extended PTE bits (not used)
 *
 *	Memory types and attributes:
 *	C B	type		attribute
 *	0 0	strong ordered	noncacheable, nonbufferable, shared
 *	0 1	device		noncacheable, bufferable, shared
 *	1 0	normal		write-back cacheable, bufferable, non-shared
 *	1 1	normal		write-back cacheable, bufferable, shared
 */
	.align	5
ENTRY(cpu_fmp626_set_pte_ext)
#ifdef CONFIG_MMU
	armv3_set_pte_ext
	mov	r0, r0
#ifndef CONFIG_CPU_DCACHE_WRITETHROUGH
	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
#endif
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
#endif
	mov	pc, lr

	__CPUINIT

/*
 * Returns:
 *	r5, r6 corrupted
 *	r0 = control register value
 */
	.type	__fmp626_setup, #function
__fmp626_setup:
#ifdef CONFIG_SMP
	mrc	p15, 0, r0, c1, c0, 1		@ Enable SMP/nAMP mode
	orr	r0, r0, #0x20
	mcr	p15, 0, r0, c1, c0, 1
#endif

	mov	r0, #0
	mcr	p15, 0, r0, c7, c7		@ invalidate I,D caches on v4
	mcr	p15, 0, r0, c7, c10, 4		@ drain write buffer on v4
#ifdef CONFIG_MMU
	mcr	p15, 0, r0, c8, c7		@ invalidate I,D TLBs on v4
#endif

	adr	r5, fmp626_crval
	ldmia	r5, {r5, r6}
	mrc	p15, 0, r0, c1, c0		@ get control register v4
	bic	r0, r0, r5
	orr	r0, r0, r6
	mov	pc, lr
	.size	__fmp626_setup, . - __fmp626_setup

	/*
	 * L
	 * 4.VI ZFRS BLDP WCAM
	 * ..11 1.01 .111 1101
	 *
	 */
	.type	fmp626_crval, #object
fmp626_crval:
	crval	clear=0x00003b7f, mmuset=0x0000397d, ucset=0x00001134

	__INITDATA

	@ define struct processor (see <asm/proc-fns.h> and proc-macros.S)
	define_processor_functions fmp626, dabort=v5t_early_abort, pabort=legacy_pabort

	.section ".rodata"

	string cpu_arch_name, "armv5te"
	string cpu_elf_name, "v5"
	string cpu_fmp626_name, "FMP626"

	.align

	.section ".proc.info.init", #alloc, #execinstr

	.type	__fmp626_proc_info,#object
__fmp626_proc_info:
	.long	0x66056268			@ Faraday ARMv5TE, FMP626
	.long	0xff0ffff8
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
	b	__fmp626_setup
	.long	cpu_arch_name
	.long	cpu_elf_name
	.long	HWCAP_SWP|HWCAP_HALF|HWCAP_THUMB|HWCAP_TLS
	.long	cpu_fmp626_name
	.long	fmp626_processor_functions
	.long	fa_tlb_fns
	.long	fa_user_fns
	.long	fmp626_cache_fns
	.size	__fmp626_proc_info, . - __fmp626_proc_info
