/*
* memcpy_64.S- Sigmastar
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

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/sysreg.h>

#define dstin	x0
#define src	x1
#define count	x2
#define tmp1	x3
#define tmp1w	w3
#define tmp2	x4
#define tmp2w	w4
#define tmp3	x5
#define tmp3w	w5
#define dst	x6
#define A_l	x7
#define A_h	x8
#define B_l	x9
#define B_h	x10
#define C_l	x11
#define C_h	x12
#define D_l	x13
#define D_h	x14
#define QA_l q0
#define QA_h q1
#define QB_l q2
#define QB_h q3

SYM_FUNC_START(_bench_neon_memcpy)
	mov	dst, dstin
	cmp	count, #64
	b.ge	.Lcpy_not_short
	cmp	count, #15
	b.le	.Ltail15tiny
	/* Deal with small copies quickly by dropping straight into the
	 * exit block.  */
.Ltail63:
	/* Copy up to 48 bytes of data.  At this point we only need the
	 * bottom 6 bits of count to be accurate.  */
	ands	tmp1, count, #0x30
	b.eq	.Ltail15
	add	dst, dst, tmp1
	add	src, src, tmp1
	cmp	tmp1w, #0x20
	b.eq	1f
	b.lt	2f
	ldp	A_l, A_h, [src, #-48]
	stp	A_l, A_h, [dst, #-48]
1:
	ldp	A_l, A_h, [src, #-32]
	stp	A_l, A_h, [dst, #-32]
2:
	ldp	A_l, A_h, [src, #-16]
	stp	A_l, A_h, [dst, #-16]
.Ltail15:
	ands	count, count, #15
	beq	1f
	add	src, src, count
	ldp	A_l, A_h, [src, #-16]
	add	dst, dst, count
	stp	A_l, A_h, [dst, #-16]
1:
	ret
.Ltail15tiny:
	/* Copy up to 15 bytes of data.  Does not assume additional data
	   being copied.  */
	tbz	count, #3, 1f
	ldr	tmp1, [src], #8
	str	tmp1, [dst], #8
1:
	tbz	count, #2, 1f
	ldr	tmp1w, [src], #4
	str	tmp1w, [dst], #4
1:
	tbz	count, #1, 1f
	ldrh	tmp1w, [src], #2
	strh	tmp1w, [dst], #2
1:
	tbz	count, #0, 1f
	ldrb	tmp1w, [src]
	strb	tmp1w, [dst]
1:
	ret
.Lcpy_not_short:
	/* We don't much care about the alignment of DST, but we want SRC
	 * to be 128-bit (16 byte) aligned so that we don't cross cache line
	 * boundaries on both loads and stores.  */
	neg	tmp2, src
	ands	tmp2, tmp2, #15		/* Bytes to reach alignment.  */
	b.eq	2f
	sub	count, count, tmp2
	/* Copy more data than needed; it's faster than jumping
	 * around copying sub-Quadword quantities.  We know that
	 * it can't overrun.  */
	ldp	A_l, A_h, [src]
	add	src, src, tmp2
	stp	A_l, A_h, [dst]
	add	dst, dst, tmp2
	/* There may be less than 63 bytes to go now.  */
	cmp	count, #63
	b.le	.Ltail63
2:
	subs	count, count, #128
	b.ge	.Lcpy_body_large
	/* Less than 128 bytes to copy, so handle 64 here and then jump
	 * to the tail.  */
	ldp QA_l, QA_h, [src]
	ldp QB_l, QB_h, [src, #32]
	stp QA_l, QA_h, [dst]
	stp QB_l, QB_h, [dst, #32]
	tst	count, #0x3f
	add	src, src, #64
	add	dst, dst, #64
	b.ne	.Ltail63
	ret
	/* Critical loop.  Start at a new cache line boundary.  Assuming
	 * 64 bytes per line this ensures the entire loop is in one line.  */
	.p2align 6
.Lcpy_body_large:
	/* There are at least 128 bytes to copy.  */
	ldp QA_l, QA_h, [src, #0]
	sub	dst, dst, #32		/* Pre-bias.  */
	ldp QB_l, QB_h, [src, #32]!	/* src += 64 - Pre-bias.  */
1:
	stp QA_l, QA_h, [dst, #32]
	ldp QA_l, QA_h, [src, #32]
	stp QB_l, QB_h, [dst, #64]!
	ldp QB_l, QB_h, [src, #64]!
	subs	count, count, #64
	b.ge	1b
	stp QA_l, QA_h, [dst, #32]
	stp QB_l, QB_h, [dst, #64]
	add	src, src, #32
	add	dst, dst, #64 + 32
	tst	count, #0x3f
	b.ne	.Ltail63
	ret
SYM_FUNC_END(_bench_neon_memcpy)


		/*
		 * Optimized memcpy() for ARM.
         *
		 * note that memcpy() always returns the destination pointer,
		 * so we have to preserve R0.
		 */

SYM_FUNC_START(_bench_memcpy)
		/* The stack must always be 64-bits aligned to be compliant with the
		 * ARM ABI. Since we have to save R0, we might as well save R4
		 * which we can use for better pipelining of the reads below
		 */
SYM_FUNC_END(_bench_memcpy)

