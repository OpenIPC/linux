#include <asm/unwind.h>

#if __LINUX_ARM_ARCH__ >= 6
	.macro	bitop, name, instr
ENTRY(	\name		)
UNWIND(	.fnstart	)
	ands	ip, r1, #3
	strneb	r1, [ip]		@ assert word-aligned
	mov	r2, #1
	and	r3, r0, #31		@ Get bit offset
	mov	r0, r0, lsr #5
	add	r1, r1, r0, lsl #2	@ Get word offset
	mov	r3, r2, lsl r3
1:	ldrex	r2, [r1]
	\instr	r2, r2, r3
	strex	r0, r2, [r1]
	cmp	r0, #0
	bne	1b
	bx	lr
UNWIND(	.fnend		)
ENDPROC(\name		)
	.endm

	.macro	testop, name, instr, store
ENTRY(	\name		)
UNWIND(	.fnstart	)
	ands	ip, r1, #3
	strneb	r1, [ip]		@ assert word-aligned
	mov	r2, #1
	and	r3, r0, #31		@ Get bit offset
	mov	r0, r0, lsr #5
	add	r1, r1, r0, lsl #2	@ Get word offset
	mov	r3, r2, lsl r3		@ create mask
	smp_dmb
1:	ldrex	r2, [r1]
	ands	r0, r2, r3		@ save old value of bit
	\instr	r2, r2, r3		@ toggle bit
	strex	ip, r2, [r1]
	cmp	ip, #0
	bne	1b
	smp_dmb
	cmp	r0, #0
	movne	r0, #1
2:	bx	lr
UNWIND(	.fnend		)
ENDPROC(\name		)
	.endm
#elif defined(CONFIG_CPU_FMP626)
	.macro	bitop, name, instr
ENTRY(	\name		)
UNWIND(	.fnstart	)
	mov	r2, #1
	and	r3, r0, #7		@ Get bit offset
	add	r1, r1, r0, lsr #3	@ Get byte offset
	mov	r3, r2, lsl r3
1:	ldc	p13, c0, [r1], {0}	@ set address for ldrexb
	mrc	p13, 0, r2, c0, c0, 0	@ ldrexb
	\instr	r2, r2, r3
	mcr	p13, 0, r2, c0, c0, 0	@ data for strexb
	stc	p13, c0, [r1], {0}	@ strexb to address
	mrc	p13, 0, r0, c0, c0, 2	@ strexb status
	cmp	r0, #0
	bne	1b
	mov	pc, lr
UNWIND(	.fnend		)
ENDPROC(\name		)
	.endm

	.macro	testop, name, instr, store
ENTRY(	\name		)
UNWIND(	.fnstart	)
	and	r3, r0, #7		@ Get bit offset
	mov	r2, #1
	add	r1, r1, r0, lsr #3	@ Get byte offset
	mov	r3, r2, lsl r3		@ create mask
	smp_dmb
1:	ldc	p13, c0, [r1], {0}	@ set address for ldrexb
	mrc	p13, 0, r2, c0, c0, 0	@ ldrexb
	ands	r0, r2, r3		@ save old value of bit
	\instr	r2, r2, r3		@ toggle bit
	mcr	p13, 0, r2, c0, c0, 0	@ data for strexb
	stc	p13, c0, [r1], {0}	@ strexb to address
	mrc	p13, 0, ip, c0, c0, 2	@ strexb status
	smp_dmb
	cmp	ip, #0
	bne	1b
	cmp	r0, #0
	movne	r0, #1
2:	mov	pc, lr
UNWIND(	.fnend		)
ENDPROC(\name		)
	.endm
#else
	.macro	bitop, name, instr
ENTRY(	\name		)
UNWIND(	.fnstart	)
	ands	ip, r1, #3
	strneb	r1, [ip]		@ assert word-aligned
	and	r2, r0, #31
	mov	r0, r0, lsr #5
	mov	r3, #1
	mov	r3, r3, lsl r2
	save_and_disable_irqs ip
	ldr	r2, [r1, r0, lsl #2]
	\instr	r2, r2, r3
	str	r2, [r1, r0, lsl #2]
	restore_irqs ip
	mov	pc, lr
UNWIND(	.fnend		)
ENDPROC(\name		)
	.endm

/**
 * testop - implement a test_and_xxx_bit operation.
 * @instr: operational instruction
 * @store: store instruction
 *
 * Note: we can trivially conditionalise the store instruction
 * to avoid dirtying the data cache.
 */
	.macro	testop, name, instr, store
ENTRY(	\name		)
UNWIND(	.fnstart	)
	ands	ip, r1, #3
	strneb	r1, [ip]		@ assert word-aligned
	and	r3, r0, #31
	mov	r0, r0, lsr #5
	save_and_disable_irqs ip
	ldr	r2, [r1, r0, lsl #2]!
	mov	r0, #1
	tst	r2, r0, lsl r3
	\instr	r2, r2, r0, lsl r3
	\store	r2, [r1]
	moveq	r0, #0
	restore_irqs ip
	mov	pc, lr
UNWIND(	.fnend		)
ENDPROC(\name		)
	.endm
#endif
