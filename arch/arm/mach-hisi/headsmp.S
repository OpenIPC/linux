/*
 *  Copyright (c) 2014 Hisilicon Limited.
 *  Copyright (c) 2014 Linaro Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/assembler.h>

	__CPUINIT

ENTRY(hix5hd2_secondary_startup)
	bl	v7_invalidate_l1
	b	secondary_startup
ENDPROC(hix5hd2_secondary_startup)


ENTRY(hi3519_secondary_startup)
	/* Set A17 acinactive to 1, bit 8 is acinactive */
	bl      hi_pmc_clear_a17_ac

	bl	cci_enable_port_for_self

	/* config l2ctl */
	mrc     p15, 1, r0, c9, c0, 2
	mov     r2, #0x100000
	orr     r1, r1, r2
	mcr     p15, 1, r0, c9, c0, 2

	/*
	 * set SMP bit ACTLR register for A17 slave core
	 */
	mrc     p15, 0, r0, c1, c0, 1
	orr     r0, #(1 << 6)
	mcr     p15, 0, r0, c1, c0, 1

	bl      v7_invalidate_l1

	b       secondary_startup
ENDPROC(hi3519_secondary_startup)

ENTRY(hi3519_cpu_resume)
	safe_svcmode_maskall    r1

	/* Set A17 acinactive to 0, bit 8 is acinactive */
	bl      hi_pmc_clear_a17_ac

	bl	cci_enable_port_for_self

        /* config l2ctl, just follow the datasheet why?*/
        mrc     p15, 1, r0, c9, c0, 2
        mov     r2, #0x100000
        orr     r1, r1, r2
        mcr     p15, 1, r0, c9, c0, 2

        /*
         * set SMP bit ACTLR register for A17 slave core
         */
        mrc     p15, 0, r0, c1, c0, 1
        orr     r0, #(1 << 6)
        mcr     p15, 0, r0, c1, c0, 1

        b       cpu_resume

ENDPROC(hi3519_cpu_resume)

ENTRY(hi3516av200_secondary_startup)
	/* Set A17 acinactive to 1, bit 8 is acinactive */
	bl      hi_pmc_clear_a17_ac

	bl	cci_enable_port_for_self

	/* config l2ctl */
	mrc     p15, 1, r0, c9, c0, 2
	mov     r2, #0x100000
	orr     r1, r1, r2
	mcr     p15, 1, r0, c9, c0, 2

	/*
	 * set SMP bit ACTLR register for A17 slave core
	 */
	mrc     p15, 0, r0, c1, c0, 1
	orr     r0, #(1 << 6)
	mcr     p15, 0, r0, c1, c0, 1

	bl      v7_invalidate_l1

	b       secondary_startup
ENDPROC(hi3516av200_secondary_startup)

ENTRY(hi3516av200_cpu_resume)
	safe_svcmode_maskall    r1

	/* Set A17 acinactive to 0, bit 8 is acinactive */
	bl      hi_pmc_clear_a17_ac

	bl	cci_enable_port_for_self

        /* config l2ctl, just follow the datasheet why?*/
        mrc     p15, 1, r0, c9, c0, 2
        mov     r2, #0x100000
        orr     r1, r1, r2
        mcr     p15, 1, r0, c9, c0, 2

        /*
         * set SMP bit ACTLR register for A17 slave core
         */
        mrc     p15, 0, r0, c1, c0, 1
        orr     r0, #(1 << 6)
        mcr     p15, 0, r0, c1, c0, 1

        b       cpu_resume

ENDPROC(hi3516av200_cpu_resume)

ENTRY(hi3559_secondary_startup)
	/* Set A17 acinactive to 1, bit 8 is acinactive */
	bl      hi_pmc_clear_a17_ac

	bl	cci_enable_port_for_self

	/* config l2ctl */
	mrc     p15, 1, r0, c9, c0, 2
	mov     r2, #0x100000
	orr     r1, r1, r2
	mcr     p15, 1, r0, c9, c0, 2

	/*
	 * set SMP bit ACTLR register for A17 slave core
	 */
	mrc     p15, 0, r0, c1, c0, 1
	orr     r0, #(1 << 6)
	mcr     p15, 0, r0, c1, c0, 1

	bl      v7_invalidate_l1

	b       secondary_startup
ENDPROC(hi3559_secondary_startup)

ENTRY(hi3559_cpu_resume)
	safe_svcmode_maskall    r1

	/* Set A17 acinactive to 0, bit 8 is acinactive */
	bl      hi_pmc_clear_a17_ac

	bl	cci_enable_port_for_self

        /* config l2ctl, just follow the datasheet why?*/
        mrc     p15, 1, r0, c9, c0, 2
        mov     r2, #0x100000
        orr     r1, r1, r2
        mcr     p15, 1, r0, c9, c0, 2

        /*
         * set SMP bit ACTLR register for A17 slave core
         */
        mrc     p15, 0, r0, c1, c0, 1
        orr     r0, #(1 << 6)
        mcr     p15, 0, r0, c1, c0, 1

        b       cpu_resume

ENDPROC(hi3559_cpu_resume)

#ifdef CONFIG_ARCH_HI3531D
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@
@  corrupt: r0, r1, r2, r3
@
.align  2

flash_cache_all:

    /* disable MMU stuff and caches */
    mrc p15, 0, r0, c1, c0, 0
    orr r0, r0, #0x00002000      /* clear bits 13 (--V-)   */
    bic r0, r0, #0x00000007      /* clear bits 2:0 (-CAM)  */
    orr r0, r0, #0x00000002      /* set bit 1 (--A-) Align */
    orr r0, r0, #0x00000800      /* set bit 12 (Z---) BTB  */
    mcr p15, 0, r0, c1, c0, 0

    /*
     * Invalidate L1 I/D
     */
    mov r0, #0                   /* set up for MCR */
    mcr p15, 0, r0, c8, c7, 0    /* invalidate TLBs */
    mcr p15, 0, r0, c7, c5, 0    /* invalidate icache */

    /* Invalidate L1 D-cache */
    mcr p15, 2, r0, c0, c0, 0    /* select L1 data cache */
    /* Read Current Cache Size Identification Register */
    mrc p15, 1, r3, c0, c0, 0
    ldr r1, =0x1ff
    and r3, r1, r3, LSR #13      /* r3 = (number of sets -1) */
    mov r0, #0
way_loop:
    mov r1, #0                  /* r1->set counter */
line_loop:
    mov r2, r0, LSL #30
    orr r2, r1, LSL #5           /* r2->set/way cache-op format */
    mcr     p15, 0, r2, c7, c6, 2    /* Invalidate line described by r2 */
    add r1, r1, #1               /* Increment set counter */
    cmp r1, r3                   /* Check if the last set is reached */
    ble line_loop                /* if not, continue the set_loop */
    add r0, r0, #1               /* else, Increment way counter */
    cmp r0, #4                   /* Check if the last way is reached */
    blt way_loop                 /* if not, continue the way_loop */

    mov pc, lr
#endif

ENTRY(hi3536c_secondary_startup)
    /* set the cpu to SVC32 mode */
    mrs r0, cpsr
    bic r0, r0, #0x1f  /* r0 = ((~0x1F) & r0) */
    orr r0, r0, #0xd3  /* r0 = (0xd3 | r0)    */
    msr cpsr, r0

    mrc p15, 0, r0, c0, c0, 5
    and r0, r0, #15
    adr r4, 1f
    ldmia   r4, {r5, r6}
    sub r4, r4, r5
    add r6, r6, r4
pen_36c:    ldr r7, [r6]
    cmp r7, r0
    bne pen_36c

    /*
     * we've been released from the holding pen: secondary_stack
     * should now contain the SVC stack for this core
     */
    b   secondary_startup

1:  .long   .
    .long   pen_release
ENDPROC(hi3536c_secondary_startup)

ENTRY(hi3531d_secondary_startup)
    /* set the cpu to SVC32 mode */
    mrs r0, cpsr
    bic r0, r0, #0x1f  /* r0 = ((~0x1F) & r0) */
    orr r0, r0, #0xd3  /* r0 = (0xd3 | r0)    */
    msr cpsr, r0

#ifdef CONFIG_ARCH_HI3531D
    bl flash_cache_all
#endif

    mrc p15, 0, r0, c0, c0, 5
    and r0, r0, #15
    adr r4, 1f
    ldmia   r4, {r5, r6}
    sub r4, r4, r5
    add r6, r6, r4
pen:    ldr r7, [r6]
    cmp r7, r0
    bne pen

    /*
     * we've been released from the holding pen: secondary_stack
     * should now contain the SVC stack for this core
     */
   b   secondary_startup

 1:  .long   .
    .long   pen_release
ENDPROC(hi3531d_secondary_startup)

ENTRY(hi3521d_secondary_startup)
    /* set the cpu to SVC32 mode */
    mrs r0, cpsr
    bic r0, r0, #0x1f  /* r0 = ((~0x1F) & r0) */
    orr r0, r0, #0xd3  /* r0 = (0xd3 | r0)    */
    msr cpsr, r0

    mrc p15, 0, r0, c0, c0, 5
    and r0, r0, #15
    adr r4, 1f
    ldmia   r4, {r5, r6}
    sub r4, r4, r5
    add r6, r6, r4
pen_21d:    ldr r7, [r6]
    cmp r7, r0
    bne pen_21d

    /*
     * we've been released from the holding pen: secondary_stack
     * should now contain the SVC stack for this core
     */
    b   secondary_startup

1:  .long   .
    .long   pen_release
ENDPROC(hi3521d_secondary_startup)

ENTRY(hi3520dv400_secondary_startup)
    /* set the cpu to SVC32 mode */
    mrs r0, cpsr
    bic r0, r0, #0x1f  /* r0 = ((~0x1F) & r0) */
    orr r0, r0, #0xd3  /* r0 = (0xd3 | r0)    */
    msr cpsr, r0

    mrc p15, 0, r0, c0, c0, 5
    and r0, r0, #15
    adr r4, 1f
    ldmia   r4, {r5, r6}
    sub r4, r4, r5
    add r6, r6, r4
pen_20dv400:    ldr r7, [r6]
    cmp r7, r0
    bne pen_20dv400

    /*
     * we've been released from the holding pen: secondary_stack
     * should now contain the SVC stack for this core
     */
    b   secondary_startup

1:  .long   .
    .long   pen_release
ENDPROC(hi3520dv400_secondary_startup)