# ******************************************************************************
#   Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
#
#   @file       cpu_s.S
#   @ingroup
#
#   @brief      CPU cache related file
#
#   @note       Nothing.
# ******************************************************************************
#define PLD_SPEEDUP  1
#ifndef ALIGN
#define ALIGN .align		4
#endif

#ifndef ENTRY
#define ENTRY(name) \
	.globl name; \
	ALIGN; \
name:
#endif

#ifndef WEAK
#define WEAK(name) \
	.weak name; \
name:
#endif

#ifndef END
#define END(name) \
	.size name, .-name
#endif

#define ENDPROC(name) \
	.type name, %function; \
	END(name)
#if 0
ENTRY(_lockdownICache)
// This function lockdown instructions into ICache.
// r0 is the starting address and r1 is the ending address
// All instructions located from starting to ending address will be lockdowned into ICache
        STMFD   r13!, {r2-r3, r14}
        // 8 words alignment
        MOV     r2, #0x1F
        BIC     r0, r0, r2
        AND     r3, r1, r2
        CMP     r3, #0x0
        BEQ     I_EightWordAlign
        BIC     r1, r1, r2
        ADD     r1, r1, #16

I_EightWordAlign:
        MOV     r2, #0x80000000
        MCR     p15, 0, r2, c9, c0, 1

I_Lockdown:
        MCR     p15, 0, r0, c7, c13, 1
        ADD     r0, r0, #16
        CMP     r0, r1
        BNE     I_Lockdown

        MOV     r2, #0x0
        MCR     p15, 0, r2, c9, c0, 1

        LDMFD   r13!, {r2-r3, pc}
ENDPROC(_lockdownICache)

// This function lockdown data into DCache
// r0 is the starting address and r1 is the ending address
// All data located from starting to ending address will be lockdowned into DCache
ENTRY(_lockdownDCache)
        STMFD   r13!, {r2-r3, r14}

        // 8 Words alignment
        MOV     r2, #0x1F
        BIC     r0, r0, r2
        AND     r3, r1, r2
        CMP     r3, #0x0
        BEQ     D_EightWordAlign
        BIC     r1, r1, r2
        ADD     r1, r1, #16

D_EightWordAlign:
        MOV     r2, #0x80000000
        MCR     p15, 0, r2, c9, c0, 0

D_Lockdown:
        LDR     r2, [r0], #4
        CMP     r0, r1
        BNE     D_Lockdown

        MOV     r2, #0x0
        MCR     p15, 0, r2, c9, c0, 0

        LDMFD   r13!, {r2-r3, pc}
ENDPROC(_lockdownDCache)
#endif

#define CACHE_LINE_SIZE     32
#define NEON_MAX_PREFETCH_DISTANCE 320
#define ENABLE_UNALIGNED_MEM_ACCESSES 1
    .text
    .fpu    neon

ENTRY(neon_memcpy)
		mov	ip, r0
		cmp	r2, #16
		blt     4f	@ Have less than 16 bytes to copy

		@ First ensure 16 byte alignment for the destination buffer
		tst	r0, #0xF
		beq	2f
		tst	r0, #1
		ldrneb	r3, [r1], #1
		strneb	r3, [ip], #1
		subne	r2, r2, #1
		tst	ip, #2
#ifdef ENABLE_UNALIGNED_MEM_ACCESSES
		ldrneh	r3, [r1], #2
		strneh	r3, [ip], #2
#else
		ldrneb	r3, [r1], #1
		strneb	r3, [ip], #1
		ldrneb	r3, [r1], #1
		strneb	r3, [ip], #1
#endif
		subne	r2, r2, #2

		tst	ip, #4
		beq	1f
		vld4.8	{d0[0], d1[0], d2[0], d3[0]}, [r1]!
		vst4.8	{d0[0], d1[0], d2[0], d3[0]}, [ip, :32]!
		sub	r2, r2, #4
1:
		tst	ip, #8
		beq	2f
		vld1.8	{d0}, [r1]!
		vst1.8	{d0}, [ip, :64]!
		sub	r2, r2, #8
2:
		subs	r2, r2, #32
		blt	3f
		mov	r3, #32

		@ Main copy loop, 32 bytes are processed per iteration.
		@ ARM instructions are used for doing fine-grained prefetch,
		@ increasing prefetch distance progressively up to
		@ NEON_MAX_PREFETCH_DISTANCE at runtime
1:
		vld1.8	{d0-d3}, [r1]!
		cmp	r3, #(NEON_MAX_PREFETCH_DISTANCE - 32)
		pld	[r1, r3]
		addle	r3, r3, #32
		vst1.8	{d0-d3}, [ip, :128]!
		sub	r2, r2, #32
		cmp	r2, r3
		bge	1b
		cmp	r2, #0
		blt	3f
1:		@ Copy the remaining part of the buffer (already prefetched)
		vld1.8	{d0-d3}, [r1]!
		subs	r2, r2, #32
		vst1.8	{d0-d3}, [ip, :128]!
		bge	1b
3:		@ Copy up to 31 remaining bytes
		tst	r2, #16
		beq	4f
		vld1.8	{d0, d1}, [r1]!
		vst1.8	{d0, d1}, [ip, :128]!
4:
		@ Use ARM instructions exclusively for the final trailing part
		@ not fully fitting into full 16 byte aligned block in order
		@ to avoid "ARM store after NEON store" hazard. Also NEON
		@ pipeline will be (mostly) flushed by the time when the
		@ control returns to the caller, making the use of NEON mostly
		@ transparent (and avoiding hazards in the caller code)

#ifdef ENABLE_UNALIGNED_MEM_ACCESSES
		movs	r3, r2, lsl #29
		ldrcs	r3, [r1], #4
		strcs	r3, [ip], #4
		ldrcs	r3, [r1], #4
		strcs	r3, [ip], #4
		ldrmi	r3, [r1], #4
		strmi	r3, [ip], #4
		movs	r2, r2, lsl #31
		ldrcsh	r3, [r1], #2
		strcsh	r3, [ip], #2
		ldrmib	r3, [r1], #1
		strmib	r3, [ip], #1
#else
		movs	r3, r2, lsl #29
		bcc	1f
	.rept	8
		ldrcsb	r3, [r1], #1
		strcsb	r3, [ip], #1
	.endr
1:
		bpl	1f
	.rept	4
		ldrmib	r3, [r1], #1
		strmib	r3, [ip], #1
	.endr
1:
		movs	r2, r2, lsl #31
		ldrcsb	r3, [r1], #1
		strcsb	r3, [ip], #1
		ldrcsb	r3, [r1], #1
		strcsb	r3, [ip], #1
		ldrmib	r3, [r1], #1
		strmib	r3, [ip], #1
#endif
		bx	lr
END(neon_memcpy)

#if(PLD_SPEEDUP)
.global memcpy
ENTRY(memcpy)
#else
.global pld_memcpy
ENTRY(pld_memcpy)
#endif
        .fnstart
        .save       {r0, r4, lr}
        stmfd       sp!, {r0, r4, lr}
        /* Making room for r5-r11 which will be spilled later */
        .pad        #28
        sub         sp, sp, #28

        // preload the destination because we'll align it to a cache line
        // with small writes. Also start the source "pump".
        pld         [r0, #(CACHE_LINE_SIZE*0)]
        pld         [r1, #(CACHE_LINE_SIZE*0)]
        pld         [r1, #(CACHE_LINE_SIZE*1)]

		/* it simplifies things to take care of len<4 early */
		cmp			r2, #4
		blo			copy_last_3_and_return

		/* compute the offset to align the source
		 * offset = (4-(src&3))&3 = -src & 3
		 */
		rsb			r3, r1, #0
		ands		r3, r3, #3
		beq			src_aligned

		/* align source to 32 bits. We need to insert 2 instructions between
		 * a ldr[b|h] and str[b|h] because byte and half-word instructions
		 * stall 2 cycles.
		 */
		movs		r12, r3, lsl #31
		sub			r2, r2, r3		/* we know that r3 <= r2 because r2 >= 4 */
		ldrmib		r3, [r1], #1
		ldrcsb		r4, [r1], #1
		ldrcsb		r12,[r1], #1
        strmib		r3, [r0], #1
		strcsb		r4, [r0], #1
		strcsb		r12,[r0], #1

src_aligned:

		/* see if src and dst are aligned together (congruent) */
		eor			r12, r0, r1
		tst			r12, #3
		bne			non_congruent

        /* Use post-incriment mode for stm to spill r5-r11 to reserved stack
         * frame. Don't update sp.
         */
        stmea		sp, {r5-r11}

		/* align the destination to a cache-line */
		rsb         r3, r0, #0
		ands		r3, r3, #0x1C
		beq         congruent_aligned32
		cmp         r3, r2
		andhi		r3, r2, #0x1C

		/* conditionnaly copies 0 to 7 words (length in r3) */
		movs		r12, r3, lsl #28
		ldmcsia		r1!, {r4, r5, r6, r7}	/* 16 bytes */
		ldmmiia		r1!, {r8, r9}			/*  8 bytes */
		stmcsia		r0!, {r4, r5, r6, r7}
		stmmiia		r0!, {r8, r9}
		tst         r3, #0x4
		ldrne		r10,[r1], #4			/*  4 bytes */
		strne		r10,[r0], #4
		sub         r2, r2, r3

congruent_aligned32:
		/*
		 * here source is aligned to 32 bytes.
		 */

cached_aligned32:
        subs        r2, r2, #32
        blo         less_than_32_left

        /*
         * We preload a cache-line up to 64 bytes ahead. On the 926, this will
         * stall only until the requested world is fetched, but the linefill
         * continues in the the background.
         * While the linefill is going, we write our previous cache-line
         * into the write-buffer (which should have some free space).
         * When the linefill is done, the writebuffer will
         * start dumping its content into memory
         *
         * While all this is going, we then load a full cache line into
         * 8 registers, this cache line should be in the cache by now
         * (or partly in the cache).
         *
         * This code should work well regardless of the source/dest alignment.
         *
         */

        // Align the preload register to a cache-line because the cpu does
        // "critical word first" (the first word requested is loaded first).
        bic         r12, r1, #0x1F
        add         r12, r12, #64

1:      ldmia       r1!, { r4-r11 }
        pld         [r12, #64]
        subs        r2, r2, #32

        // NOTE: if r12 is more than 64 ahead of r1, the following ldrhi
        // for ARM9 preload will not be safely guarded by the preceding subs.
        // When it is safely guarded the only possibility to have SIGSEGV here
        // is because the caller overstates the length.
        ldrhi       r3, [r12], #32      /* cheap ARM9 preload */
        stmia       r0!, { r4-r11 }
		bhs         1b

        add         r2, r2, #32




less_than_32_left:
		/*
		 * less than 32 bytes left at this point (length in r2)
		 */

		/* skip all this if there is nothing to do, which should
		 * be a common case (if not executed the code below takes
		 * about 16 cycles)
		 */
		tst			r2, #0x1F
		beq			1f

		/* conditionnaly copies 0 to 31 bytes */
		movs		r12, r2, lsl #28
		ldmcsia		r1!, {r4, r5, r6, r7}	/* 16 bytes */
		ldmmiia		r1!, {r8, r9}			/*  8 bytes */
		stmcsia		r0!, {r4, r5, r6, r7}
		stmmiia		r0!, {r8, r9}
		movs		r12, r2, lsl #30
		ldrcs		r3, [r1], #4			/*  4 bytes */
		ldrmih		r4, [r1], #2			/*  2 bytes */
		strcs		r3, [r0], #4
		strmih		r4, [r0], #2
		tst         r2, #0x1
		ldrneb		r3, [r1]				/*  last byte  */
		strneb		r3, [r0]

		/* we're done! restore everything and return */
1:		ldmfd		sp!, {r5-r11}
		ldmfd		sp!, {r0, r4, lr}
		bx			lr

		/********************************************************************/

non_congruent:
		/*
		 * here source is aligned to 4 bytes
		 * but destination is not.
		 *
		 * in the code below r2 is the number of bytes read
		 * (the number of bytes written is always smaller, because we have
		 * partial words in the shift queue)
		 */
		cmp			r2, #4
		blo			copy_last_3_and_return

        /* Use post-incriment mode for stm to spill r5-r11 to reserved stack
         * frame. Don't update sp.
         */
        stmea		sp, {r5-r11}

		/* compute shifts needed to align src to dest */
		rsb			r5, r0, #0
		and			r5, r5, #3			/* r5 = # bytes in partial words */
		mov			r12, r5, lsl #3		/* r12 = right */
		rsb			lr, r12, #32		/* lr = left  */

		/* read the first word */
		ldr			r3, [r1], #4
		sub			r2, r2, #4

		/* write a partial word (0 to 3 bytes), such that destination
		 * becomes aligned to 32 bits (r5 = nb of words to copy for alignment)
		 */
		movs		r5, r5, lsl #31
		strmib		r3, [r0], #1
		movmi		r3, r3, lsr #8
		strcsb		r3, [r0], #1
		movcs		r3, r3, lsr #8
		strcsb		r3, [r0], #1
		movcs		r3, r3, lsr #8

		cmp			r2, #4
		blo			partial_word_tail

		/* Align destination to 32 bytes (cache line boundary) */
1:		tst			r0, #0x1c
		beq			2f
		ldr			r5, [r1], #4
		sub         r2, r2, #4
		orr			r4, r3, r5,		lsl lr
		mov			r3, r5,			lsr r12
		str			r4, [r0], #4
        cmp         r2, #4
		bhs			1b
		blo			partial_word_tail

		/* copy 32 bytes at a time */
2:		subs		r2, r2, #32
		blo			less_than_thirtytwo

		/* Use immediate mode for the shifts, because there is an extra cycle
		 * for register shifts, which could account for up to 50% of
		 * performance hit.
		 */

        cmp			r12, #24
		beq			loop24
		cmp			r12, #8
		beq			loop8

loop16:
        ldr         r12, [r1], #4
1:      mov         r4, r12
		ldmia		r1!, {   r5,r6,r7,  r8,r9,r10,r11}
        pld         [r1, #64]
        subs        r2, r2, #32
        ldrhs       r12, [r1], #4
		orr			r3, r3, r4,		lsl #16
		mov			r4, r4,			lsr #16
		orr			r4, r4, r5,		lsl #16
		mov			r5, r5,			lsr #16
		orr			r5, r5, r6,		lsl #16
		mov			r6, r6,			lsr #16
		orr			r6, r6, r7,		lsl #16
		mov			r7, r7,			lsr #16
		orr			r7, r7, r8,		lsl #16
		mov			r8, r8,			lsr #16
		orr			r8, r8, r9,		lsl #16
		mov			r9, r9,			lsr #16
		orr			r9, r9, r10,	lsl #16
		mov			r10, r10,		lsr #16
		orr			r10, r10, r11,	lsl #16
		stmia		r0!, {r3,r4,r5,r6, r7,r8,r9,r10}
		mov			r3, r11,		lsr #16
		bhs			1b
		b			less_than_thirtytwo

loop8:
        ldr         r12, [r1], #4
1:      mov         r4, r12
		ldmia		r1!, {   r5,r6,r7,  r8,r9,r10,r11}
        pld         [r1, #64]
		subs		r2, r2, #32
        ldrhs       r12, [r1], #4
		orr			r3, r3, r4,		lsl #24
		mov			r4, r4,			lsr #8
		orr			r4, r4, r5,		lsl #24
		mov			r5, r5,			lsr #8
		orr			r5, r5, r6,		lsl #24
		mov			r6, r6,			lsr #8
		orr			r6, r6, r7,		lsl #24
		mov			r7, r7,			lsr #8
		orr			r7, r7, r8,		lsl #24
		mov			r8, r8,			lsr #8
		orr			r8, r8, r9,		lsl #24
		mov			r9, r9,			lsr #8
		orr			r9, r9, r10,	lsl #24
		mov			r10, r10,		lsr #8
		orr			r10, r10, r11,	lsl #24
		stmia		r0!, {r3,r4,r5,r6, r7,r8,r9,r10}
		mov			r3, r11,		lsr #8
		bhs			1b
		b			less_than_thirtytwo

loop24:
        ldr         r12, [r1], #4
1:      mov         r4, r12
		ldmia		r1!, {   r5,r6,r7,  r8,r9,r10,r11}
        pld         [r1, #64]
		subs		r2, r2, #32
        ldrhs       r12, [r1], #4
		orr			r3, r3, r4,		lsl #8
		mov			r4, r4,			lsr #24
		orr			r4, r4, r5,		lsl #8
		mov			r5, r5,			lsr #24
		orr			r5, r5, r6,		lsl #8
		mov			r6, r6,			lsr #24
		orr			r6, r6, r7,		lsl #8
		mov			r7, r7,			lsr #24
		orr			r7, r7, r8,		lsl #8
		mov			r8, r8,			lsr #24
		orr			r8, r8, r9,		lsl #8
		mov			r9, r9,			lsr #24
		orr			r9, r9, r10,	lsl #8
		mov			r10, r10,		lsr #24
		orr			r10, r10, r11,	lsl #8
		stmia		r0!, {r3,r4,r5,r6, r7,r8,r9,r10}
		mov			r3, r11,		lsr #24
		bhs			1b


less_than_thirtytwo:
		/* copy the last 0 to 31 bytes of the source */
		rsb			r12, lr, #32		/* we corrupted r12, recompute it  */
		add			r2, r2, #32
		cmp			r2, #4
		blo			partial_word_tail

1:		ldr			r5, [r1], #4
		sub         r2, r2, #4
		orr			r4, r3, r5,		lsl lr
		mov			r3,	r5,			lsr r12
		str			r4, [r0], #4
        cmp         r2, #4
		bhs			1b

partial_word_tail:
		/* we have a partial word in the input buffer */
		movs		r5, lr, lsl #(31-3)
		strmib		r3, [r0], #1
		movmi		r3, r3, lsr #8
		strcsb		r3, [r0], #1
		movcs		r3, r3, lsr #8
		strcsb		r3, [r0], #1

		/* Refill spilled registers from the stack. Don't update sp. */
		ldmfd		sp, {r5-r11}

copy_last_3_and_return:
		movs		r2, r2, lsl #31	/* copy remaining 0, 1, 2 or 3 bytes */
		ldrmib		r2, [r1], #1
		ldrcsb		r3, [r1], #1
		ldrcsb		r12,[r1]
		strmib		r2, [r0], #1
		strcsb		r3, [r0], #1
		strcsb		r12,[r0]

        /* we're done! restore sp and spilled registers and return */
        add         sp,  sp, #28
		ldmfd		sp!, {r0, r4, lr}
		bx			lr
#if(PLD_SPEEDUP)
END(memcpy)
#else
END(pld_memcpy)
#endif

#if(PLD_SPEEDUP)
ENTRY(memset)
#else
ENTRY(pld_memset)
#endif
        .save       {r0, r4-r7, lr}
        stmfd		sp!, {r0, r4-r7, lr}
        rsb			r3, r0, #0
        ands		r3, r3, #3
        cmp         r3, r2
        movhi       r3, r2

        /* splat r1 */
        mov         r1, r1, lsl #24
        orr         r1, r1, r1, lsr #8
        orr         r1, r1, r1, lsr #16
        movs		r12, r3, lsl #31
        strcsb		r1, [r0], #1
        /* can't use strh (alignment unknown) */
        strcsb		r1, [r0], #1
        strmib		r1, [r0], #1
        subs		r2, r2, r3
        ldmlsfd     sp!, {r0, r4-r7, lr}   /* return */
        bxls        lr
        /* align the destination to a cache-line */
        mov         r12, r1
        mov         lr, r1
        mov         r4, r1
        mov         r5, r1
        mov         r6, r1
        mov         r7, r1
        rsb         r3, r0, #0
        ands		r3, r3, #0x1C
        beq         3f
        cmp         r3, r2
        andhi		r3, r2, #0x1C
        sub         r2, r2, r3
        /* conditionally writes 0 to 7 words (length in r3) */
        movs		r3, r3, lsl #28
        stmcsia		r0!, {r1, lr}
        stmcsia		r0!, {r1, lr}
        stmmiia		r0!, {r1, lr}
        movs		r3, r3, lsl #2
        strcs       r1, [r0], #4
3:      subs        r2, r2, #32
        mov         r3, r1
        bmi         2f
1:      subs        r2, r2, #32
        stmia		r0!, {r1,r3,r4,r5,r6,r7,r12,lr}
        bhs         1b
2:      add         r2, r2, #32
        /* conditionally stores 0 to 31 bytes */
        movs		r2, r2, lsl #28
        stmcsia		r0!, {r1,r3,r12,lr}
        stmmiia		r0!, {r1, lr}
        movs		r2, r2, lsl #2
        strcs       r1, [r0], #4
        strmih		r1, [r0], #2
        movs		r2, r2, lsl #2
        strcsb		r1, [r0]
        ldmfd		sp!, {r0, r4-r7, lr}
        bx          lr
#if(PLD_SPEEDUP)
END(memset)
#else
END(pld_memset)
#endif

#if(PLD_SPEEDUP)
ENTRY(memcmp)
#else
ENTRY(pld_memcmp)
#endif
        pld         [r0, #(CACHE_LINE_SIZE * 0)]
        pld         [r0, #(CACHE_LINE_SIZE * 1)]
        /* take of the case where length is 0 or the buffers are the same */

        cmp         r0, r1
        moveq       r0, #0
        bxeq        lr
        pld         [r1, #(CACHE_LINE_SIZE * 0)]
        pld         [r1, #(CACHE_LINE_SIZE * 1)]

        /* make sure we have at least 8+4 bytes, this simplify things below
        * and avoid some overhead for small blocks
        */
        cmp        r2, #(8+4)
        bmi        10f

        .save {r4, lr}
        /* save registers */
        stmfd       sp!, {r4, lr}

        /* since r0 hold the result, move the first source
        * pointer somewhere else
        */
        mov        r4, r0

        /* align first pointer to word boundary
        * offset = -src & 3
        */
        rsb         r3, r4, #0
        ands        r3, r3, #3
        beq         0f

        /* align first pointer  */
        sub         r2, r2, r3
1:      ldrb        r0, [r4], #1
        ldrb        ip, [r1], #1
        subs        r0, r0, ip
        bne         9f
        subs        r3, r3, #1
        bne         1b
0:      /* here the first pointer is aligned, and we have at least 4 bytes
         * to process.
         */
        /* see if the pointers are congruent */

        eor         r0, r4, r1
        ands        r0, r0, #3
        bne         5f

        /* congruent case, 32 bytes per iteration
         * We need to make sure there are at least 32+4 bytes left
         * because we effectively read ahead one word, and we could
         * read past the buffer (and segfault) if we're not careful.
         */
        ldr         ip, [r1]
        subs        r2, r2, #(32 + 4)
        bmi         1f
0:      pld         [r4, #(CACHE_LINE_SIZE * 2)]
        pld         [r1, #(CACHE_LINE_SIZE * 2)]
        ldr         r0, [r4], #4
        ldr         lr, [r1, #4]!
        eors        r0, r0, ip
        ldreq       r0, [r4], #4
        ldreq       ip, [r1, #4]!
        eoreqs      r0, r0, lr
        ldreq       r0, [r4], #4
        ldreq       lr, [r1, #4]!
        eoreqs      r0, r0, ip
        ldreq       r0, [r4], #4
        ldreq       ip, [r1, #4]!
        eoreqs      r0, r0, lr
        ldreq       r0, [r4], #4
        ldreq       lr, [r1, #4]!
        eoreqs      r0, r0, ip
        ldreq       r0, [r4], #4
        ldreq       ip, [r1, #4]!
        eoreqs      r0, r0, lr
        ldreq       r0, [r4], #4
        ldreq       lr, [r1, #4]!
        eoreqs      r0, r0, ip
        ldreq       r0, [r4], #4
        ldreq       ip, [r1, #4]!
        eoreqs      r0, r0, lr
        bne         2f
        subs        r2, r2, #32
        bhs         0b

        /* do we have at least 4 bytes left? */
1:      adds        r2, r2, #(32 - 4 + 4)
        bmi         4f
        /* finish off 4 bytes at a time */
3:      ldr         r0, [r4], #4
        ldr         ip, [r1], #4
        eors        r0, r0, ip
        bne         2f
        subs        r2, r2, #4
        bhs         3b

        /* are we done? */
4:      adds        r2, r2, #4
        moveq       r0, #0
        beq         9f

        /* finish off the remaining bytes */
        b           8f
2:      /* the last 4 bytes are different, restart them */
        sub         r4, r4, #4
        sub         r1, r1, #4
        mov         r2, #4
        /* process the last few bytes */
8:      ldrb        r0, [r4], #1
        ldrb        ip, [r1], #1
        @ stall
        subs        r0, r0, ip
        bne         9f
        subs        r2, r2, #1
        bne         8b
9:      /* restore registers and return */
        ldmfd       sp!, {r4, lr}
        bx          lr
10:     /* process less than 12 bytes */
        cmp         r2, #0
        moveq       r0, #0
        bxeq        lr
        mov         r3, r0
11:     ldrb        r0, [r3], #1
        ldrb        ip, [r1], #1
        subs        r0, ip
        bxne        lr
        subs        r2, r2, #1
        bne         11b
        bx          lr
5:      /*************** non-congruent case ***************/
        and         r0, r1, #3
        cmp         r0, #2
        bne         4f
        /* here, offset is 2 (16-bits aligned, special cased) */
        /* make sure we have at least 16 bytes to process */
        subs        r2, r2, #16
        addmi       r2, r2, #16
        bmi         8b
        /* align the unaligned pointer */
        bic         r1, r1, #3
        ldr         lr, [r1], #4

6:      pld         [r1, #(CACHE_LINE_SIZE * 2)]
        pld         [r4, #(CACHE_LINE_SIZE * 2)]
        mov         ip, lr, lsr #16
        ldr         lr, [r1], #4
        ldr         r0, [r4], #4
        orr         ip, ip, lr, lsl #16
        eors        r0, r0, ip
        moveq       ip, lr, lsr #16
        ldreq       lr, [r1], #4
        ldreq       r0, [r4], #4
        orreq       ip, ip, lr, lsl #16
        eoreqs      r0, r0, ip
        moveq       ip, lr, lsr #16
        ldreq       lr, [r1], #4
        ldreq       r0, [r4], #4
        orreq       ip, ip, lr, lsl #16
        eoreqs      r0, r0, ip
        moveq       ip, lr, lsr #16
        ldreq       lr, [r1], #4
        ldreq       r0, [r4], #4
        orreq       ip, ip, lr, lsl #16
        eoreqs      r0, r0, ip
        bne         7f
        subs        r2, r2, #16
        bhs         6b
        sub         r1, r1, #2
        /* are we done? */
        adds        r2, r2, #16
        moveq       r0, #0
        beq         9b
        /* finish off the remaining bytes */
        b           8b
7:      /* fix up the 2 pointers and fallthrough... */
        sub         r1, r1, #(4+2)
        sub         r4, r4, #4
        mov         r2, #4
        b           8b
4:      /*************** offset is 1 or 3 (less optimized) ***************/
        stmfd		sp!, {r5, r6, r7}
        @ r5 = rhs
        @ r6 = lhs
        @ r7 = scratch
        mov         r5, r0, lsl #3
        /* r5 = right shift */
        rsb         r6, r5, #32
        /* r6 = left shift */
        /* align the unaligned pointer */
        bic         r1, r1, #3
        ldr         r7, [r1], #4
        sub         r2, r2, #8
6:      mov         ip, r7, lsr r5
        ldr         r7, [r1], #4
        ldr         r0, [r4], #4
        orr         ip, ip, r7, lsl r6
        eors        r0, r0, ip
        moveq       ip, r7, lsr r5
        ldreq       r7, [r1], #4
        ldreq       r0, [r4], #4
        orreq       ip, ip, r7, lsl r6
        eoreqs      r0, r0, ip
        bne         7f
        subs        r2, r2, #8
        bhs         6b
        sub         r1, r1, r6, lsr #3
        ldmfd       sp!, {r5, r6, r7}
        /* are we done? */
        adds        r2, r2, #8
        moveq       r0, #0
        beq         9b
        /* finish off the remaining bytes */
        b           8b
7:      /* fix up the 2 pointers and fallthrough... */
        sub         r1, r1, #4
        sub         r1, r1, r6, lsr #3
        sub         r4, r4, #4
        mov         r2, #4
        ldmfd		sp!, {r5, r6, r7}
        b           8b
#if(PLD_SPEEDUP)
END(memcmp)
#else
END(pld_memcmp)
#endif



ENTRY(INCR_64)
/************************************************************************
 * r0 is address of key set that user want to configure
 * r1 is the binary starting address
 ************************************************************************/
#if 0
    STMFD   r13!, {r0-r12, r14}
	LDR	r0, =0xF0290000
	LDR	r1, ='I'
	STR	r1, [r0]
	LDR	r1, ='N'
	STR	r1, [r0]
	LDR	r1, ='C'
	STR	r1, [r0]
	LDR	r1, ='R'
	STR	r1, [r0]
	LDR	r1, ='6'
	STR	r1, [r0]
	LDR	r1, ='4'
    LDMFD   r13!, {r0-r12, pc}   //If r7 !=1 than return
    nop
#else
	STMIA   r13!, {r2-r10, r14}
	LDR	r0, =0xF0290000
	LDR	r1, ='I'
	STR	r1, [r0]
	LDR	r1, ='N'
	STR	r1, [r0]
	LDR	r1, ='C'
	STR	r1, [r0]
	LDR	r1, ='R'
	STR	r1, [r0]
	LDR	r1, ='6'
	STR	r1, [r0]
	LDR	r1, ='4'
	STR	r1, [r0]
    LDMIA   r13!, {r2-r10, pc}   //If r7 !=1 than return
    nop

#endif
ENDPROC(INCR_64)

 /* a prefetch distance of 4 cache-lines works best experimentally */
 #define PREFETCH_DISTANCE   (CACHE_LINE_SIZE*4)
         .text
         .fpu    neon
ENTRY(hybrid_memcpy)
         .save       {r0, lr}
         stmfd       sp!, {r0, lr}


         /* start preloading as early as possible */
         pld         [r1, #(CACHE_LINE_SIZE*0)]
         pld         [r1, #(CACHE_LINE_SIZE*1)]


         /* do we have at least 16-bytes to copy (needed for alignment below) */
         cmp         r2, #16
         blo         5f


         /* align destination to half cache-line for the write-buffer */
         rsb         r3, r0, #0
         ands        r3, r3, #0xF
         beq         0f


         /* copy up to 15-bytes (count in r3) */
         sub         r2, r2, r3
         movs        ip, r3, lsl #31
         ldrmib      lr, [r1], #1
         strmib      lr, [r0], #1
         ldrcsb      ip, [r1], #1
         ldrcsb      lr, [r1], #1
         strcsb      ip, [r0], #1
         strcsb      lr, [r0], #1
         movs        ip, r3, lsl #29
         bge         1f
         // copies 4 bytes, destination 32-bits aligned
         vld4.8      {d0[0], d1[0], d2[0], d3[0]}, [r1]!
         vst4.8      {d0[0], d1[0], d2[0], d3[0]}, [r0, :32]!
 1:      bcc         2f
         // copies 8 bytes, destination 64-bits aligned
         vld1.8      {d0}, [r1]!
         vst1.8      {d0}, [r0, :64]!
 2:


 0:    /* preload immediately the next cache line, which we may need */
       pld         [r1, #(CACHE_LINE_SIZE*0)]
       pld         [r1, #(CACHE_LINE_SIZE*1)]


       /* make sure we have at least 64 bytes to copy */
       subs        r2, r2, #64
       blo         2f


       /* preload all the cache lines we need.
        * NOTE: the number of pld below depends on PREFETCH_DISTANCE,
        * ideally would would increase the distance in the main loop to
        * avoid the goofy code below. In practice this doesn't seem to make
        * a big difference.
        */
       pld         [r1, #(CACHE_LINE_SIZE*2)]
       pld         [r1, #(CACHE_LINE_SIZE*3)]
       pld         [r1, #(PREFETCH_DISTANCE)]


1:      /* The main loop copies 64 bytes at a time */
        vld1.8      {d0  - d3},   [r1]!
        vld1.8      {d4  - d7},   [r1]!
        pld         [r1, #(PREFETCH_DISTANCE)]
        subs        r2, r2, #64
        vst1.8      {d0  - d3},   [r0, :128]!
        vst1.8      {d4  - d7},   [r0, :128]!
        bhs         1b


2:      /* fix-up the remaining count and make sure we have >= 32 bytes left */
        add         r2, r2, #64
        subs        r2, r2, #32
        blo         4f


3:      /* 32 bytes at a time. These cache lines were already preloaded */
        vld1.8      {d0 - d3},  [r1]!
        subs        r2, r2, #32
        vst1.8      {d0 - d3},  [r0, :128]!
        bhs         3b


4:      /* less than 32 left */
        add         r2, r2, #32
        tst         r2, #0x10
        beq         5f
        // copies 16 bytes, 128-bits aligned
        vld1.8      {d0, d1}, [r1]!
        vst1.8      {d0, d1}, [r0, :128]!


5:      /* copy up to 15-bytes (count in r2) */
        movs        ip, r2, lsl #29
        bcc         1f
        vld1.8      {d0}, [r1]!
        vst1.8      {d0}, [r0]!
1:      bge         2f
        vld4.8      {d0[0], d1[0], d2[0], d3[0]}, [r1]!
        vst4.8      {d0[0], d1[0], d2[0], d3[0]}, [r0]!
2:      movs        ip, r2, lsl #31
        ldrmib      r3, [r1], #1
        ldrcsb      ip, [r1], #1
        ldrcsb      lr, [r1], #1
        strmib      r3, [r0], #1
        strcsb      ip, [r0], #1
        strcsb      lr, [r0], #1


        ldmfd       sp!, {r0, lr}
        bx          lr
END(hybrid_memcpy)

