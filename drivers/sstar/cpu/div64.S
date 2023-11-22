/*
* XXX.c - Sigmastar
*
* Copyright (c) [2019~2022] SigmaStar Technology.
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

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  drivers/sstar/memory/div64.S
 */
#if defined(CONFIG_ARM) && defined(CONFIG_AEABI)
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/unwind.h>

.extern div64_u64_rem
.extern div64_s64_rem

/* Perform 64 bit unsigned division.
   Inputs:
        r0:r1   numerator
        r2:r3   denominator
   Outputs:
        r0:r1   quotient
        r2:r3   remainder
 */
ENTRY(__aeabi_uldivmod)
UNWIND(.fnstart)
UNWIND(.save	{r4, lr})
	push	{r4, lr}
	sub	sp, #16
	add	r4, sp, #8
	str	r4, [sp, #0]
	bl	div64_u64_rem
	ldmia	r4, {r2, r3}
	adds	r4, #8
	mov	sp, r4
	pop	{r4, pc}
UNWIND(.fnend)
ENDPROC(__aeabi_uldivmod)


/* Perform 64 bit signed division.
   Inputs:
        r0:r1   numerator
        r2:r3   denominator
   Outputs:
        r0:r1   quotient
        r2:r3   remainder
 */
ENTRY(__aeabi_ldivmod)
UNWIND(.fnstart)
UNWIND(.save	{r4, lr})
	push	{r4, lr}
	sub	sp, #16
	add	r4, sp, #8
	str	r4, [sp, #0]
	bl	div64_s64_rem
	ldmia	r4, {r2, r3}
	adds	r4, #8
	mov	sp, r4
	pop	{r4, pc}
UNWIND(.fnend)
ENDPROC(__aeabi_ldivmod)
#endif
