/*
* mmu_v7.S- Sigmastar
*
* Copyright (c) [2019~2020] SigmaStar Technology.
*
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License version 2 for more details.
*
*/
#include <linux/pgtable.h>
#include <asm/pgtable-hwdef.h>

#include "../../mm/proc-macros.S"

#ifdef CONFIG_ARM_LPAE
    #ERR not support LPAE STR !!!
#else // from: arch/arm/mm/proc-v7-2level.S
/* PTWs cacheable, inner WB not shareable, outer WB not shareable */
#define TTB_S		(1 << 1)
#define TTB_RGN_NC	(0 << 3)
#define TTB_RGN_OC_WBWA	(1 << 3)
#define TTB_RGN_OC_WT	(2 << 3)
#define TTB_RGN_OC_WB	(3 << 3)
#define TTB_NOS		(1 << 5)
#define TTB_IRGN_NC	((0 << 0) | (0 << 6))
#define TTB_IRGN_WBWA	((0 << 0) | (1 << 6))
#define TTB_IRGN_WT	((1 << 0) | (0 << 6))
#define TTB_IRGN_WB	((1 << 0) | (1 << 6))


#ifdef CONFIG_SS_PM_AOV
.equ SSCHIPVER, 0x1F003c04
.equ SSCHIPVER_VA, 0xFD003c04
#else
.equ SSCHIPVER, 0xFD003c04
.equ SSCHIPVER_VA, 0xFD003c04

#endif

#define TTB_FLAGS_UP	TTB_IRGN_WB|TTB_RGN_OC_WB
#define PMD_FLAGS_UP	PMD_SECT_WB
.equ	PRRR,	0xff0a81a8
.equ	NMRR,	0x40e040e0
.equ	NMRR1,	0x40604060
	/*
	 * Macro for setting up the TTBRx and TTBCR registers.
	 * - \ttb0 and \ttb1 updated with the corresponding flags.
	 */
	.macro	v7_ttb_setup, zero, ttbr0l, ttbr0h, ttbr1, tmp
	mcr	p15, 0, \zero, c2, c0, 2	@ TTB control register
	ALT_SMP(orr	\ttbr0l, \ttbr0l, #TTB_FLAGS_SMP)
	ALT_UP(orr	\ttbr0l, \ttbr0l, #TTB_FLAGS_UP)
	ALT_SMP(orr	\ttbr1, \ttbr1, #TTB_FLAGS_SMP)
	ALT_UP(orr	\ttbr1, \ttbr1, #TTB_FLAGS_UP)
	mcr	p15, 0, \ttbr1, c2, c0, 0	@ load TTB0
	@mcr	p15, 0, \ttbr1, c2, c0, 1	@ load TTB1
	.endm
#endif

#if 1
    .macro v7_dcache_flush_all tag
	dmb					@ ensure ordering with previous memory accesses
	mrc	p15, 1, r0, c0, c0, 1		@ read clidr
	mov	r3, r0, lsr #23			@ move LoC into position
	ands	r3, r3, #7 << 1			@ extract LoC*2 from clidr
	beq	finished_\tag			@ if loc is 0, then no need to clean
start_flush_levels_\tag:
	mov	r10, #0				@ start clean at cache level 0
flush_levels_\tag:
	add	r2, r10, r10, lsr #1		@ work out 3x current cache level
	mov	r1, r0, lsr r2			@ extract cache type bits from clidr
	and	r1, r1, #7			@ mask of the bits for current cache only
	cmp	r1, #2				@ see what cache we have at this level
	blt	skip_\tag				@ skip if no cache, or just i-cache
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	isb					@ isb to sych the new cssr&csidr
	mrc	p15, 1, r1, c0, c0, 0		@ read the new csidr
	and	r2, r1, #7			@ extract the length of the cache lines
	add	r2, r2, #4			@ add 4 (line length offset)
	movw	r4, #0x3ff
	ands	r4, r4, r1, lsr #3		@ find maximum number on the way size
	clz	r5, r4				@ find bit position of way size increment
	movw	r7, #0x7fff
	ands	r7, r7, r1, lsr #13		@ extract max number of the index size
loop1_\tag:
	mov	r9, r7				@ create working copy of max index
loop2_\tag:
 ARM(	orr	r11, r10, r4, lsl r5	)	@ factor way and cache number into r11
 THUMB(	lsl	r6, r4, r5		)
 THUMB(	orr	r11, r10, r6		)	@ factor way and cache number into r11
 ARM(	orr	r11, r11, r9, lsl r2	)	@ factor index number into r11
 THUMB(	lsl	r6, r9, r2		)
 THUMB(	orr	r11, r11, r6		)	@ factor index number into r11
	mcr	p15, 0, r11, c7, c14, 2		@ clean & invalidate by set/way
	subs	r9, r9, #1			@ decrement the index
	bge	loop2_\tag
	subs	r4, r4, #1			@ decrement the way
	bge	loop1_\tag
skip_\tag:
	add	r10, r10, #2			@ increment cache number
	cmp	r3, r10
	dsb
	bgt	flush_levels_\tag
finished_\tag:
	mov	r10, #0				@ switch back to cache level 0
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	dsb	st
	isb
    .endm

#else
    .macro v7_dcache_flush_all
	dmb					@ ensure ordering with previous memory accesses
	mrc	p15, 1, r0, c0, c0, 1		@ read clidr
	mov	r3, r0, lsr #23			@ move LoC into position
	ands	r3, r3, #7 << 1			@ extract LoC*2 from clidr
	beq	finished			@ if loc is 0, then no need to clean
start_flush_levels:
	mov	r10, #0				@ start clean at cache level 0
flush_levels:
	add	r2, r10, r10, lsr #1		@ work out 3x current cache level
	mov	r1, r0, lsr r2			@ extract cache type bits from clidr
	and	r1, r1, #7			@ mask of the bits for current cache only
	cmp	r1, #2				@ see what cache we have at this level
	blt	skip				@ skip if no cache, or just i-cache
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	isb					@ isb to sych the new cssr&csidr
	mrc	p15, 1, r1, c0, c0, 0		@ read the new csidr
	and	r2, r1, #7			@ extract the length of the cache lines
	add	r2, r2, #4			@ add 4 (line length offset)
	movw	r4, #0x3ff
	ands	r4, r4, r1, lsr #3		@ find maximum number on the way size
	clz	r5, r4				@ find bit position of way size increment
	movw	r7, #0x7fff
	ands	r7, r7, r1, lsr #13		@ extract max number of the index size
loop1:
	mov	r9, r7				@ create working copy of max index
loop2:
 ARM(	orr	r11, r10, r4, lsl r5	)	@ factor way and cache number into r11
 THUMB(	lsl	r6, r4, r5		)
 THUMB(	orr	r11, r10, r6		)	@ factor way and cache number into r11
 ARM(	orr	r11, r11, r9, lsl r2	)	@ factor index number into r11
 THUMB(	lsl	r6, r9, r2		)
 THUMB(	orr	r11, r11, r6		)	@ factor index number into r11
	mcr	p15, 0, r11, c7, c14, 2		@ clean & invalidate by set/way
	subs	r9, r9, #1			@ decrement the index
	bge	loop2
	subs	r4, r4, #1			@ decrement the way
	bge	loop1
skip:
	add	r10, r10, #2			@ increment cache number
	cmp	r3, r10
	dsb
	bgt	flush_levels
finished:
	mov	r10, #0				@ switch back to cache level 0
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	dsb	st
	isb
    .endm
#endif

/*
 * We place the page tables 16k at IMI address 0xA00C0000
 * and we hope that nothing else is using it. *
 */
    .macro mmu_table_init
    // with the follwoing TLB entry:
    //    VA             PA
    // 0xF9000000    0xA0000000
    // r3: mmu table address (VA)
    mov r3, #0xF9000000
    orr r3, #0x00030000
    bic r3, r3, #0xff       @ Align the pointer
    bic r3, r3, #0x3f00

	/* 1st level MMU translation table descriptor format - Section
	 *  - Total table size: 16KB
	 *  - Each entry control 1MB section
	 *  - REF: DDI0406C, Figure B3-4 Short-descriptor first-level descriptor formats
	 *
	 * |31                        20|19|18|17|16|15|14  12|11 10|9|8                5| 4| 3| 2| 1| 0|
	 * ----------------------------------------------------------------------------------------------
	 * | Setion base addr, PA[31:20]|NS| 0|nG| S|AP| TEX  | AP  | |    Domain        |XN| C| B| 1|  |
	 * ----------------------------------------------------------------------------------------------
	 *                                                                                            PXN
     */

    /*
     * Invalid all section entry
     */
    mov r0, r3
    mov r1, #0x00       @ PXN=0, indicate invalid entry
    add r2, r3, #16384
1:  str r1, [r0]
    add r0, r0, #4
    teq r0, r2
    bne 1b

    mov r6, 0x0c | 0x02 @ !XN
    /*
     * IMI Mapping
     *      VA              PA
     *   0xF9000000     0xA0000000
     *   0xF90FFFFF     0xA00FFFFF
     */
    orr r1, r6, #0x04       @ ensure B is set for this (Outer and Inter Write-Back, no Write-Allocte)
    orr r1, r1, #0x1 << 12    @ ensure TEX[2:0] is (0b001) for this (Outer and Inter nonecachable)
    orr r1, r1, #3 << 10
    mov r2, #0xA0000000
    add r1, r1, r2
    mov r2, #0xF9000000
    mov r2, r2, lsr #20
    add r0, r3, r2, lsl #2
    str r1, [r0]

    /*
     * RIU Mapping
     *      VA              PA
     *   0xFD000000     0x1F000000
     *   0xFD1FFFFF     0x1F1FFFFF
     */
    mov r1, #0x12       @ XN|U + section mapping
    orr r1, r1, #3 << 10
    mov r2, #0x1F000000
    add r1, r1, r2
    mov r2, #0xFD000000
    mov r2, r2, lsr #20
    add r0, r3, r2, lsl #2
    str r1, [r0], #4
    add r1, r1, #1048576
    str r1, [r0]

    /*
     * RIU Mapping
     *      VA              PA
     *   0xFD200000     0x1F200000
     *   0xFD3FFFFF     0x1F3FFFFF
     */
    mov r1, #0x12       @ XN|U + section mapping
    orr r1, r1, #3 << 10
    mov r2, #0x1F000000
    orr r2, #0x00200000
    add r1, r1, r2
    mov r2, #0xFD000000
    orr r2, #0x00200000
    mov r2, r2, lsr #20
    add r0, r3, r2, lsl #2
    str r1, [r0], #4
    add r1, r1, #1048576
    str r1, [r0]

    .endm


    .macro switch_mmu_table_to_imi
    // replace TTBR0
#if 0 //def CONFIG_ARCH_SSTAR
	mrc	p15, 0, r12, c1, c0, 1
	orr	r12, r12, #(1 << 6)    @ Enable SMP
	mcr	p15, 0, r12, c1, c0, 1
#endif
#ifdef CONFIG_SMP
	orr	r10, r10, #(1 << 6)		@ Enable SMP/nAMP mode
	ALT_SMP(mrc	p15, 0, r0, c1, c0, 1)
	ALT_UP(mov	r0, r10)		@ fake it for UP
	orr	r10, r10, r0			@ Set required bits
	teq	r10, r0				@ Were they already set?
	mcrne	p15, 0, r10, c1, c0, 1		@ No, update register
#endif
	dsb	ishst
	mov	r10, #0
	ldr r8, =0xA0030000
	mcr	p15, 0, r10, c7, c5, 0		@ I+BTB cache invalidate

#ifdef CONFIG_MMU
	mcr	p15, 0, r10, c8, c7, 0		@ invalidate I + D TLBs
	v7_ttb_setup r10, r4, r5, r8, r3	@ TTBCR, TTBRx setup
	ldr	r4, =PRRR			@ PRRR

#ifdef CONFIG_ARCH_INFINITY6C
	ldr r0, =SSCHIPVER
	ldr r1, [r0]
	ldr r3, =0x00
	cmp  r1, r3
	ldreq   r6, =NMRR1
	ldrne	r6, =NMRR
#else
	ldr   	r6, =NMRR		@ NMRR
#endif
	mcr	p15, 0, r4, c10, c2, 0		@ write PRRR
	mcr	p15, 0, r6, c10, c2, 1		@ write NMRR
#endif
    isb
	dsb	ishst				@ Complete invalidations
    .endm

    .macro mmu_get_mmu_table_addr, addr
	mrc	p15, 0, \addr, c2, c0, 0	@ get TTB0
    .endm

    .macro switch_mmu_table_to_miu, addr
	dsb	ishst
	mov	r10, #0
	mcr	p15, 0, r10, c7, c5, 0		@ I+BTB cache invalidate
#ifdef CONFIG_MMU
	mcr	p15, 0, r10, c8, c7, 0		@ invalidate I + D TLBs
	v7_ttb_setup r10, r4, r5, \addr, r3	@ TTBCR, TTBRx setup
#endif
    isb
	dsb	ishst				@ Complete invalidations
    .endm

