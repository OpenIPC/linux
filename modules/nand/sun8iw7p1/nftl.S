/*SPDX-Licensen-Identifier: GPL-2.0*/

	.arch armv7-a
	.fpu softvfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 2
	.eabi_attribute 30, 2
	.eabi_attribute 34, 1
	.eabi_attribute 18, 2
	.file	"nftl.c"
	.text
	.align	2
	.global	_nand_sector_write
	.syntax unified
	.arm
	.type	_nand_sector_write, %function
_nand_sector_write:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.size	_nand_sector_write, .-_nand_sector_write
	.align	2
	.global	_nand_flush_sector_write_cache
	.syntax unified
	.arm
	.type	_nand_flush_sector_write_cache, %function
_nand_flush_sector_write_cache:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.size	_nand_flush_sector_write_cache, .-_nand_flush_sector_write_cache
	.align	2
	.global	_nftl_nand_erase_superblk
	.syntax unified
	.arm
	.type	_nftl_nand_erase_superblk, %function
_nftl_nand_erase_superblk:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r2, [r0]
	mov	r3, r1
	push	{r4, r5, r6, lr}
	sub	sp, sp, #16
	ldrh	r6, [r1, #2]
	mov	r1, sp
	ldr	r2, [r2, #4]
	ldrh	r5, [r3]
	ldrh	r4, [r3, #4]
	ldr	lr, [r3, #8]
	mov	r0, r2
	ldr	ip, [r3, #12]
	strh	r6, [sp, #2]	@ movhi
	ldr	r3, [r2, #84]
	strh	r5, [sp]	@ movhi
	strh	r4, [sp, #4]	@ movhi
	str	lr, [sp, #8]
	str	ip, [sp, #12]
	blx	r3
	add	sp, sp, #16
	@ sp needed
	pop	{r4, r5, r6, pc}
	.size	_nftl_nand_erase_superblk, .-_nftl_nand_erase_superblk
	.align	2
	.global	_nftl_nand_is_blk_good
	.syntax unified
	.arm
	.type	_nftl_nand_is_blk_good, %function
_nftl_nand_is_blk_good:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r2, [r0]
	mov	r3, r1
	push	{r4, r5, r6, lr}
	sub	sp, sp, #16
	ldrh	r6, [r1, #2]
	mov	r1, sp
	ldr	r2, [r2, #4]
	ldrh	r5, [r3]
	ldrh	r4, [r3, #4]
	ldr	lr, [r3, #8]
	mov	r0, r2
	ldr	ip, [r3, #12]
	strh	r6, [sp, #2]	@ movhi
	ldr	r3, [r2, #96]
	strh	r5, [sp]	@ movhi
	strh	r4, [sp, #4]	@ movhi
	str	lr, [sp, #8]
	str	ip, [sp, #12]
	blx	r3
	add	sp, sp, #16
	@ sp needed
	pop	{r4, r5, r6, pc}
	.size	_nftl_nand_is_blk_good, .-_nftl_nand_is_blk_good
	.align	2
	.global	_nftl_nand_mark_bad_blk
	.syntax unified
	.arm
	.type	_nftl_nand_mark_bad_blk, %function
_nftl_nand_mark_bad_blk:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r2, [r0]
	mov	r3, r1
	push	{r4, r5, r6, lr}
	sub	sp, sp, #16
	ldrh	r6, [r1, #2]
	mov	r1, sp
	ldr	r2, [r2, #4]
	ldrh	r5, [r3]
	ldrh	r4, [r3, #4]
	ldr	lr, [r3, #8]
	mov	r0, r2
	ldr	ip, [r3, #12]
	strh	r6, [sp, #2]	@ movhi
	ldr	r3, [r2, #100]
	strh	r5, [sp]	@ movhi
	strh	r4, [sp, #4]	@ movhi
	str	lr, [sp, #8]
	str	ip, [sp, #12]
	blx	r3
	add	sp, sp, #16
	@ sp needed
	pop	{r4, r5, r6, pc}
	.size	_nftl_nand_mark_bad_blk, .-_nftl_nand_mark_bad_blk
	.align	2
	.syntax unified
	.arm
	.type	free_nrc, %function
free_nrc:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	cmp	r0, #0
	beq	.L15
	push	{r4, lr}
	mov	r1, #0
	mov	r4, r0
	ldr	r0, [r0, #8]
	bl	NAND_Free
	ldr	r0, [r4, #4]
	mov	r1, #0
	bl	NAND_Free
	ldr	r0, [r4, #12]
	mov	r1, #0
	bl	NAND_Free
	mov	r0, r4
	mov	r1, #0
	bl	NAND_Free
	mov	r0, #0
	pop	{r4, pc}
.L15:
	mov	r0, #0
	bx	lr
	.size	free_nrc, .-free_nrc
	.align	2
	.syntax unified
	.arm
	.type	m7_end_one_read_retry.part.1, %function
m7_end_one_read_retry.part.1:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, lr}
	mov	r4, r0
	sub	sp, sp, #20
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	ldr	r0, [r4, #248]
	bl	ndfc_is_toogle_interface
	cmp	r0, #0
	bne	.L22
	movw	r5, #:lower16:.LANCHOR0
	movt	r5, #:upper16:.LANCHOR0
.L18:
	mov	r3, #0
	mov	r2, #1
	movw	r1, #:lower16:.LANCHOR1
	str	r2, [sp, #8]
	str	r3, [sp, #4]
	mov	r2, r3
	str	r3, [sp]
	movt	r1, #:upper16:.LANCHOR1
	mov	r0, r4
	bl	set_cmd_with_nand_bus
	ldr	r3, [r5]
	cmp	r3, #1
	beq	.L24
	mov	r0, r4
	bl	nand_disable_chip
	add	sp, sp, #20
	@ sp needed
	pop	{r4, r5, pc}
.L22:
	ldr	r0, [r4, #248]
	movw	r5, #:lower16:.LANCHOR0
	bl	ndfc_set_legacy_interface
	movt	r5, #:upper16:.LANCHOR0
	mov	r3, #1
	str	r3, [r5]
	b	.L18
.L24:
	ldr	r0, [r4, #248]
	bl	ndfc_set_toogle_interface
	mov	r0, r4
	bl	nand_disable_chip
	add	sp, sp, #20
	@ sp needed
	pop	{r4, r5, pc}
	.size	m7_end_one_read_retry.part.1, .-m7_end_one_read_retry.part.1
	.align	2
	.syntax unified
	.arm
	.type	print_free_list.part.6, %function
print_free_list.part.6:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r4, r0
	movw	r0, #:lower16:.LC5
	movt	r0, #:upper16:.LC5
	bl	NAND_Print
	ldr	r4, [r4, #172]
	cmp	r4, #0
	popeq	{r4, r5, r6, r7, r8, pc}
	movw	r7, #:lower16:.LC6
	movw	r6, #:lower16:.LC7
	movw	r5, #:lower16:.LC8
	movt	r7, #:upper16:.LC6
	movt	r6, #:upper16:.LC7
	movt	r5, #:upper16:.LC8
.L27:
	ldrh	r2, [r4, #6]
	mov	r0, r7
	ldrh	r1, [r4]
	bl	NAND_Print
	ldr	r1, [r4, #8]
	mov	r0, r6
	bl	NAND_Print
	ldrh	r1, [r4, #2]
	mov	r0, r5
	bl	NAND_Print
	ldr	r4, [r4, #20]
	cmp	r4, #0
	bne	.L27
	pop	{r4, r5, r6, r7, r8, pc}
	.size	print_free_list.part.6, .-print_free_list.part.6
	.align	2
	.syntax unified
	.arm
	.type	print_block_invalid_list.part.7, %function
print_block_invalid_list.part.7:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r4, r0
	movw	r0, #:lower16:.LC9
	movt	r0, #:upper16:.LC9
	bl	NAND_Print
	ldr	r4, [r4, #200]
	cmp	r4, #0
	popeq	{r4, r5, r6, r7, r8, pc}
	movw	r7, #:lower16:.LC6
	movw	r6, #:lower16:.LC10
	movw	r5, #:lower16:.LC8
	movt	r7, #:upper16:.LC6
	movt	r6, #:upper16:.LC10
	movt	r5, #:upper16:.LC8
.L33:
	ldrh	r2, [r4, #6]
	mov	r0, r7
	ldrh	r1, [r4]
	bl	NAND_Print
	ldrh	r2, [r4, #4]
	mov	r0, r6
	ldr	r1, [r4, #8]
	bl	NAND_Print
	ldrh	r1, [r4, #2]
	mov	r0, r5
	bl	NAND_Print
	ldr	r4, [r4, #12]
	cmp	r4, #0
	bne	.L33
	pop	{r4, r5, r6, r7, r8, pc}
	.size	print_block_invalid_list.part.7, .-print_block_invalid_list.part.7
	.align	2
	.syntax unified
	.arm
	.type	check_static_wear_leveling.part.9, %function
check_static_wear_leveling.part.9:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	ip, [r0, #200]
	cmp	ip, #0
	beq	.L64
	ldr	r3, [r0, #172]
	cmp	r3, #0
	beq	.L64
	ldr	r2, [r0, #292]
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	ldrh	r9, [r0, #18]
	ldr	r8, [r2, #8]
	cmp	r8, r9
	blt	.L40
	ldr	r2, [r0, #4]
	mov	lr, #0
	mov	r1, lr
	ldrh	r5, [r2, #16]
.L42:
	ldrh	r2, [r3, #6]
	cmp	r2, r1
	cmpcs	r5, r2
	movhi	r4, #1
	movls	r4, #0
	cmp	r4, #0
	movne	lr, r3
	ldr	r3, [r3, #20]
	movne	r1, r2
	cmp	r3, #0
	bne	.L42
	cmp	lr, #0
	beq	.L40
	movw	r6, #52429
	ldrh	r1, [lr, #6]
	movt	r6, 52428
	umull	r6, r7, r5, r6
	ubfx	r6, r7, #2, #16
	cmp	r1, r6
	bcc	.L40
	sub	r8, r8, r9
	mov	r4, r3
	mov	lr, r5
.L45:
	ldrh	r3, [ip, #6]
	cmp	r5, r3
	bls	.L44
	ldr	r2, [ip, #8]
	cmp	r8, r2
	movhi	r2, #1
	movls	r2, #0
	cmp	r3, lr
	movhi	r2, #0
	cmp	r2, #0
	movne	lr, r3
	movne	r4, ip
.L44:
	ldr	ip, [ip, #12]
	cmp	ip, #0
	bne	.L45
	cmp	r4, #0
	beq	.L40
	ldrh	r2, [r4, #6]
	cmp	r1, r2
	bls	.L40
	lsl	r3, r1, #1
	add	ip, r3, r1
	cmp	ip, r5, lsl #1
	ble	.L47
	movw	r6, #43691
	movw	r3, #574
	movt	r6, 43690
	umull	r6, r7, r5, r6
	ubfx	r6, r7, #3, #16
	strh	r6, [r0, r3]	@ movhi
.L48:
	sub	r3, r1, r2
	cmp	r3, r6
	blt	.L40
	mov	r5, r0
	movw	r0, #:lower16:.LC11
	movt	r0, #:upper16:.LC11
	bl	NAND_Print
	movw	r0, #:lower16:.LC12
	ldrh	r1, [r4]
	movt	r0, #:upper16:.LC12
	bl	NAND_Print
	movw	r0, #:lower16:.LC13
	ldrh	r1, [r5, #18]
	movt	r0, #:upper16:.LC13
	bl	NAND_Print
	str	r4, [r5, #580]
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L40:
	mov	r0, #1
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L64:
	mov	r0, #1
	bx	lr
.L47:
	cmp	r5, r3
	bge	.L49
	movw	r6, #52429
	movw	r3, #574
	movt	r6, 52428
	umull	r6, r7, r5, r6
	ubfx	r6, r7, #3, #16
	strh	r6, [r0, r3]	@ movhi
	b	.L48
.L49:
	cmp	r5, ip
	movwlt	r6, #18725
	movwlt	r3, #574
	movtlt	r6, 9362
	movwge	r3, #574
	umulllt	r6, r7, r5, r6
	sublt	r6, r5, r7
	addlt	r6, r7, r6, lsr #1
	ubfxlt	r6, r6, #2, #16
	strh	r6, [r0, r3]	@ movhi
	b	.L48
	.size	check_static_wear_leveling.part.9, .-check_static_wear_leveling.part.9
	.align	2
	.syntax unified
	.arm
	.type	get_empty_cahce_node.part.11, %function
get_empty_cahce_node.part.11:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldrh	r1, [r0, #6]
	str	lr, [sp, #-4]!
	ldr	r3, [r0, #984]
	cmp	r1, #9
	bls	.L97
	cmp	r3, #0
	bne	.L72
.L79:
	mov	r2, r3
	mov	r0, r2
	ldr	pc, [sp], #4
.L77:
	ldrh	r2, [r3, #2]
	cmp	r2, #0
	beq	.L98
	ldr	r3, [r3, #16]
.L97:
	cmp	r3, #0
	bne	.L77
	b	.L79
.L70:
	ldr	r3, [r3, #16]
	cmp	r3, #0
	beq	.L79
.L72:
	ldrh	r2, [r3, #2]
	cmp	r2, #2
	bne	.L70
	ldr	ip, [r3, #20]
	sub	r1, r1, #1
	ldr	r2, [r3, #16]
	mov	lr, #255
	str	r2, [ip, #16]
	ldr	r2, [r3, #16]
	cmp	r2, #0
	strne	ip, [r2, #20]
	mov	ip, #0
	mov	r2, r3
	strh	r1, [r0, #6]	@ movhi
	strh	lr, [r3, #2]	@ movhi
	mov	r0, r2
	str	ip, [r3, #16]
	str	ip, [r3, #20]
	ldr	pc, [sp], #4
.L98:
	ldr	r1, [r3, #20]
	mov	ip, #0
	ldr	r2, [r3, #16]
	mov	lr, #255
	str	r2, [r1, #16]
	ldr	r2, [r3, #16]
	cmp	r2, #0
	strne	r1, [r2, #20]
	mov	r2, r3
	ldrh	r1, [r0, #2]
	sub	r1, r1, #1
	strh	r1, [r0, #2]	@ movhi
	mov	r0, r2
	strh	lr, [r3, #2]	@ movhi
	str	ip, [r3, #16]
	str	ip, [r3, #20]
	ldr	pc, [sp], #4
	.size	get_empty_cahce_node.part.11, .-get_empty_cahce_node.part.11
	.align	2
	.global	nand_cache_read
	.syntax unified
	.arm
	.type	nand_cache_read, %function
nand_cache_read:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	subs	r8, r3, #0
	sub	sp, sp, #12
	beq	.L150
	ldr	r3, [r0, #1972]
	lsr	r7, r2, #8
	uxtb	r2, r2
	mov	r5, r0
	mov	r4, r1
	lsl	r7, r7, #9
	ldrh	r3, [r3]
	lsl	r6, r2, #9
	cmp	r3, #0
	bne	.L151
	ldr	r9, [r0, #1608]
	cmp	r9, #0
	bne	.L107
	b	.L103
.L152:
	ldr	r9, [r9, #24]
	cmp	r9, #0
	beq	.L103
.L107:
	ldr	r3, [r9, #8]
	cmp	r4, r3
	bne	.L152
	ldrh	fp, [r9, #6]
	ldrh	r3, [r9, #4]
	ldr	r10, [r9, #12]
	lsl	fp, fp, #9
	lsl	r3, r3, #9
	uxth	fp, fp
	uxth	r3, r3
	add	r3, r3, fp
	cmp	r7, r3
	bhi	.L108
	add	r2, r7, r6
	cmp	r3, r2
	bcs	.L153
.L108:
	mov	r1, r4
	ldr	r3, [r5, #2004]
	ldr	r2, [r5, #1616]
	mov	r0, r5
	blx	r3
	ldr	r3, [r5, #4]
	add	r0, r10, fp
	ldr	r1, [r5, #1616]
	ldrh	r2, [r3, #26]
	add	r1, r1, fp
	sub	r2, r2, fp
	bl	NAND_Memcpy
	ldr	r3, [r5, #4]
	mov	r0, r8
	mov	r2, r6
	add	r1, r10, r7
	ldrb	r3, [r3, #8]	@ zero_extendqisi2
	strh	r3, [r9, #6]	@ movhi
	bl	NAND_Memcpy
	mov	r0, #0
.L144:
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L103:
	ldr	fp, [r5, #1568]
	cmp	fp, #0
	bne	.L111
	b	.L121
.L154:
	ldr	fp, [fp, #16]
	cmp	fp, #0
	beq	.L121
.L111:
	ldr	r3, [fp, #8]
	cmp	r4, r3
	bne	.L154
	ldrh	r3, [fp, #2]
	cmp	r3, #0
	bne	.L155
	ldr	r2, [fp, #20]
	add	r9, r5, #584
	ldr	r3, [fp, #16]
	mov	ip, #255
	ldr	r1, [fp, #12]
	mov	r0, r8
	add	r5, r5, #1552
	str	r3, [r2, #16]
	ldr	r3, [fp, #16]
	add	r1, r1, r7
	cmp	r3, #0
	strne	r2, [r3, #20]
	mov	r2, r6
	ldrh	r3, [r9, #2]
	sub	r3, r3, #1
	strh	r3, [r9, #2]	@ movhi
	mov	r3, #0
	strh	ip, [fp, #2]	@ movhi
	str	r3, [fp, #16]
	str	r3, [fp, #20]
	bl	NAND_Memcpy
	b	.L116
.L128:
	mov	r5, r3
.L116:
	ldr	r3, [r5, #16]
	cmp	r3, #0
	bne	.L128
	ldrh	r2, [r9, #2]
	mov	r0, r3
	str	fp, [r5, #16]
	str	r3, [fp, #16]
	add	r2, r2, #1
	str	r5, [fp, #20]
	strh	r3, [fp, #2]	@ movhi
	strh	r2, [r9, #2]	@ movhi
	b	.L144
.L151:
	ldr	r9, [r0, #604]
	ldr	r3, [r0, #2004]
	mov	r2, r9
	blx	r3
	mov	r2, r6
	mov	r10, r0
	add	r1, r9, r7
	mov	r0, r8
	bl	NAND_Memcpy
	cmp	r10, #10
	movne	r0, r10
	bne	.L144
	ldr	ip, [r5, #1968]
	mov	r1, r4
	mov	r2, r9
	ldr	r4, [r5, #2000]
	mov	r0, r5
	ldr	r3, [ip, #120]
	add	r3, r3, #1
	str	r3, [ip, #120]
	blx	r4
	mov	r0, #0
	b	.L144
.L153:
	mov	r0, r8
	mov	r2, r6
	add	r1, r10, r7
	bl	NAND_Memcpy
	mov	r0, #0
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L121:
	add	r9, r5, #584
	ldrh	r3, [r9]
	cmp	r3, #0
	beq	.L112
	ldrh	r2, [r9, #10]
	cmp	r2, #255
	beq	.L156
	sub	r2, r3, #1
	add	r1, r5, #624
	uxth	r2, r2
	mov	r3, #1
	add	r1, r1, #2
	add	r2, r2, r3
.L117:
	cmp	r2, r3
	mov	ip, r3
	add	r3, r3, #1
	beq	.L112
	ldrh	r0, [r1], #32
	cmp	r0, #255
	bne	.L117
.L113:
	add	ip, r9, ip, lsl #5
	add	fp, ip, #8
.L118:
	ldr	r3, [fp, #12]
	mov	r1, r4
	str	r4, [fp, #8]
	mov	r0, r5
	ldr	r10, [r5, #2004]
	mov	r2, r3
	str	r3, [sp]
	blx	r10
	ldr	r2, [r5, #4]
	mov	r1, #0
	strh	r1, [fp, #4]	@ movhi
	mov	r3, r0
	add	ip, r5, #1552
	ldrb	r2, [r2, #8]	@ zero_extendqisi2
	strh	r2, [fp, #6]	@ movhi
	b	.L120
.L129:
	mov	ip, r10
.L120:
	ldr	r10, [ip, #16]
	cmp	r10, #0
	bne	.L129
	ldrh	lr, [r9, #2]
	mov	r0, r8
	str	r3, [sp, #4]
	mov	r2, r6
	ldr	r3, [sp]
	add	lr, lr, #1
	str	fp, [ip, #16]
	str	r10, [fp, #16]
	add	r1, r3, r7
	str	ip, [fp, #20]
	strh	r10, [fp, #2]	@ movhi
	strh	lr, [r9, #2]	@ movhi
	bl	NAND_Memcpy
	ldr	r3, [sp, #4]
	cmp	r3, #10
	movne	r0, r3
	bne	.L144
	ldr	ip, [r5, #1968]
	mov	r1, r4
	ldr	r2, [sp]
	mov	r0, r5
	ldr	r4, [r5, #2000]
	ldr	r3, [ip, #120]
	add	r3, r3, #1
	str	r3, [ip, #120]
	blx	r4
	mov	r0, r10
	b	.L144
.L156:
	mov	ip, #0
	b	.L113
.L150:
	movw	r0, #:lower16:.LC14
	movt	r0, #:upper16:.LC14
	bl	NAND_Print
	mov	r0, r8
	b	.L144
.L155:
	ldr	r1, [fp, #12]
	mov	r0, r8
	mov	r2, r6
	add	r1, r1, r7
	bl	NAND_Memcpy
	mov	r0, #0
	b	.L144
.L112:
	mov	r0, r9
	bl	get_empty_cahce_node.part.11
	subs	fp, r0, #0
	bne	.L118
	movw	r0, #:lower16:.LC15
	ldrh	r3, [r9, #6]
	movt	r0, #:upper16:.LC15
	ldrh	r2, [r9, #2]
	ldrh	r1, [r9, #4]
	bl	NAND_Print
	mov	r0, #1
	b	.L144
	.size	nand_cache_read, .-nand_cache_read
	.align	2
	.global	_nftl_nand_read_page
	.syntax unified
	.arm
	.type	_nftl_nand_read_page, %function
_nftl_nand_read_page:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r6, r0
	ldr	r0, [r0, #1968]
	mov	r3, r1
	sub	sp, sp, #16
	ldrh	ip, [r3, #2]
	mov	r1, sp
	ldr	r2, [r6]
	ldrd	r4, [r0, #88]
	ldrh	r7, [r3]
	ldr	r2, [r2, #4]
	ldrh	r8, [r3, #4]
	adds	r4, r4, #1
	ldr	lr, [r3, #8]
	adc	r5, r5, #0
	strh	ip, [sp, #2]	@ movhi
	ldr	ip, [r3, #12]
	strd	r4, [r0, #88]
	mov	r0, r2
	ldr	r3, [r2, #88]
	strh	r7, [sp]	@ movhi
	strh	r8, [sp, #4]	@ movhi
	str	lr, [sp, #8]
	str	ip, [sp, #12]
	blx	r3
	cmn	r0, #2
	ldreq	r2, [r6, #1968]
	ldreq	r3, [r2, #136]
	addeq	r3, r3, #1
	streq	r3, [r2, #136]
	add	sp, sp, #16
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
	.size	_nftl_nand_read_page, .-_nftl_nand_read_page
	.align	2
	.global	_nftl_nand_write_page
	.syntax unified
	.arm
	.type	_nftl_nand_write_page, %function
_nftl_nand_write_page:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r6, r0
	ldr	r0, [r0, #1968]
	mov	r3, r1
	sub	sp, sp, #16
	ldrh	ip, [r3, #2]
	mov	r1, sp
	ldr	r2, [r6]
	ldrd	r4, [r0, #96]
	ldrh	r7, [r3]
	ldr	r2, [r2, #4]
	ldrh	r8, [r3, #4]
	adds	r4, r4, #1
	ldr	lr, [r3, #8]
	adc	r5, r5, #0
	strh	ip, [sp, #2]	@ movhi
	ldr	ip, [r3, #12]
	strd	r4, [r0, #96]
	mov	r0, r2
	ldr	r3, [r2, #92]
	strh	r7, [sp]	@ movhi
	strh	r8, [sp, #4]	@ movhi
	str	lr, [sp, #8]
	str	ip, [sp, #12]
	blx	r3
	cmp	r0, #0
	ldrne	r2, [r6, #1968]
	ldrne	r3, [r2, #140]
	addne	r3, r3, #1
	strne	r3, [r2, #140]
	add	sp, sp, #16
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
	.size	_nftl_nand_write_page, .-_nftl_nand_write_page
	.align	2
	.syntax unified
	.arm
	.type	print_smart.part.16, %function
print_smart.part.16:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	ldr	r4, [r0, #1968]
	movw	r0, #:lower16:.LC16
	movt	r0, #:upper16:.LC16
	ldr	r1, [r4, #64]
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC17
	ldrd	r2, [r4, #72]
	movt	r0, #:upper16:.LC17
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC18
	ldrd	r2, [r4, #80]
	movt	r0, #:upper16:.LC18
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC19
	ldrd	r2, [r4, #88]
	movt	r0, #:upper16:.LC19
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC20
	ldrd	r2, [r4, #96]
	movt	r0, #:upper16:.LC20
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC21
	ldrd	r2, [r4, #104]
	movt	r0, #:upper16:.LC21
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC22
	ldrd	r2, [r4, #112]
	movt	r0, #:upper16:.LC22
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC23
	ldr	r1, [r4, #120]
	movt	r0, #:upper16:.LC23
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC24
	ldr	r1, [r4, #124]
	movt	r0, #:upper16:.LC24
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC25
	ldr	r1, [r4, #128]
	movt	r0, #:upper16:.LC25
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC26
	ldr	r1, [r4, #132]
	movt	r0, #:upper16:.LC26
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC27
	ldr	r1, [r4, #184]
	movt	r0, #:upper16:.LC27
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC28
	ldr	r1, [r4, #136]
	movt	r0, #:upper16:.LC28
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC29
	ldr	r1, [r4, #140]
	movt	r0, #:upper16:.LC29
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC30
	ldr	r1, [r4, #144]
	movt	r0, #:upper16:.LC30
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC31
	ldr	r1, [r4, #148]
	movt	r0, #:upper16:.LC31
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC32
	ldr	r1, [r4, #152]
	movt	r0, #:upper16:.LC32
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC33
	ldr	r1, [r4, #156]
	movt	r0, #:upper16:.LC33
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC34
	ldr	r1, [r4, #160]
	movt	r0, #:upper16:.LC34
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC35
	ldr	r1, [r4, #164]
	movt	r0, #:upper16:.LC35
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC36
	ldr	r1, [r4, #168]
	movt	r0, #:upper16:.LC36
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC37
	ldr	r1, [r4, #172]
	movt	r0, #:upper16:.LC37
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC38
	ldr	r1, [r4, #176]
	movt	r0, #:upper16:.LC38
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC39
	ldr	r1, [r4, #180]
	movt	r0, #:upper16:.LC39
	pop	{r4, lr}
	b	NAND_Print_DBG
	.size	print_smart.part.16, .-print_smart.part.16
	.align	2
	.syntax unified
	.arm
	.type	print_nftl_zone.part.17, %function
print_nftl_zone.part.17:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r4, r0
	movw	r0, #:lower16:.LC40
	sub	sp, sp, #16
	movt	r0, #:upper16:.LC40
	bl	NAND_Print_DBG
	ldr	r3, [r4, #4]
	movw	r0, #:lower16:.LC41
	movt	r0, #:upper16:.LC41
	ldrh	r1, [r3, #24]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #4]
	movw	r0, #:lower16:.LC42
	movt	r0, #:upper16:.LC42
	ldrh	r1, [r3, #26]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #4]
	movw	r0, #:lower16:.LC43
	movt	r0, #:upper16:.LC43
	ldrh	r1, [r3, #20]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #4]
	movw	r0, #:lower16:.LC44
	movt	r0, #:upper16:.LC44
	ldrh	r1, [r3, #16]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #4]
	movw	r0, #:lower16:.LC45
	movt	r0, #:upper16:.LC45
	ldrh	r1, [r3, #30]
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC46
	ldr	r1, [r4, #356]
	movt	r0, #:upper16:.LC46
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC47
	ldrb	r1, [r4, #16]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC47
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC48
	ldrb	r1, [r4, #17]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC48
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC49
	ldrh	r1, [r4, #18]
	movt	r0, #:upper16:.LC49
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC50
	ldrh	r1, [r4, #20]
	movt	r0, #:upper16:.LC50
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC51
	ldr	r1, [r4, #24]
	movt	r0, #:upper16:.LC51
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC52
	ldr	r1, [r4, #32]
	movt	r0, #:upper16:.LC52
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC53
	ldrh	r1, [r4, #22]
	movt	r0, #:upper16:.LC53
	bl	NAND_Print_DBG
	add	r3, r4, #380
	movw	r0, #:lower16:.LC54
	ldrh	r1, [r3]
	movt	r0, #:upper16:.LC54
	bl	NAND_Print_DBG
	movw	r3, #382
	movw	r0, #:lower16:.LC55
	ldrh	r1, [r4, r3]
	movt	r0, #:upper16:.LC55
	bl	NAND_Print_DBG
	add	r3, r4, #388
	movw	r0, #:lower16:.LC56
	ldrh	r1, [r3]
	movt	r0, #:upper16:.LC56
	bl	NAND_Print_DBG
	add	r3, r4, #384
	movw	r0, #:lower16:.LC57
	ldrh	r1, [r3]
	movt	r0, #:upper16:.LC57
	bl	NAND_Print_DBG
	movw	r3, #394
	movw	r0, #:lower16:.LC58
	ldrh	r1, [r4, r3]
	movt	r0, #:upper16:.LC58
	bl	NAND_Print_DBG
	ldr	r3, [r4, #40]
	movw	r0, #:lower16:.LC59
	movt	r0, #:upper16:.LC59
	ldrh	r1, [r3]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #44]
	movw	r0, #:lower16:.LC60
	movt	r0, #:upper16:.LC60
	ldrh	r1, [r3]
	bl	NAND_Print_DBG
	add	r2, r4, #272
	add	r3, r4, #260
	ldm	r2, {r0, r1, r2}
	stm	sp, {r0, r1, r2}
	movw	r0, #:lower16:.LC61
	movt	r0, #:upper16:.LC61
	ldm	r3, {r1, r2, r3}
	bl	NAND_Print_DBG
	add	r2, r4, #308
	add	r3, r4, #296
	ldm	r2, {r0, r1, r2}
	stm	sp, {r0, r1, r2}
	movw	r0, #:lower16:.LC62
	movt	r0, #:upper16:.LC62
	ldm	r3, {r1, r2, r3}
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC63
	ldr	r1, [r4, #292]
	movt	r0, #:upper16:.LC63
	bl	NAND_Print_DBG
	ldr	r3, [r4, #292]
	movw	r0, #:lower16:.LC64
	movt	r0, #:upper16:.LC64
	ldrh	r1, [r3]
	bl	NAND_Print_DBG
	add	r3, r4, #320
	movw	r0, #:lower16:.LC65
	ldrh	r1, [r3]
	movt	r0, #:upper16:.LC65
	bl	NAND_Print_DBG
	ldr	r3, [r4, #292]
	movw	r0, #:lower16:.LC66
	movt	r0, #:upper16:.LC66
	ldr	r1, [r3, #8]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #48]
	movw	r0, #:lower16:.LC67
	movt	r0, #:upper16:.LC67
	ldrh	r2, [r3, #6]
	ldrh	r1, [r3]
	bl	NAND_Print_DBG
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	bne	.L175
.L169:
	movw	r0, #:lower16:.LC73
	ldr	r1, [r4, #360]
	movt	r0, #:upper16:.LC73
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC74
	ldr	r1, [r4, #284]
	movt	r0, #:upper16:.LC74
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC75
	ldr	r1, [r4, #376]
	movt	r0, #:upper16:.LC75
	bl	NAND_Print_DBG
	add	r3, r4, #584
	movw	r0, #:lower16:.LC76
	ldrh	r1, [r3]
	movt	r0, #:upper16:.LC76
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC77
	ldr	r1, [r4, #368]
	movt	r0, #:upper16:.LC77
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC78
	ldr	r1, [r4, #372]
	movt	r0, #:upper16:.LC78
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC79
	movt	r0, #:upper16:.LC79
	ldrh	r1, [r3]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC80
	movt	r0, #:upper16:.LC80
	ldrh	r1, [r3, #2]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC81
	movt	r0, #:upper16:.LC81
	ldrh	r1, [r3, #4]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC82
	movt	r0, #:upper16:.LC82
	ldrh	r1, [r3, #6]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC83
	movt	r0, #:upper16:.LC83
	ldrh	r1, [r3, #10]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC84
	movt	r0, #:upper16:.LC84
	ldrh	r1, [r3, #12]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC85
	movt	r0, #:upper16:.LC85
	ldrh	r1, [r3, #14]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC86
	movt	r0, #:upper16:.LC86
	ldrh	r1, [r3, #16]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC87
	movt	r0, #:upper16:.LC87
	ldrh	r1, [r3, #18]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC88
	movt	r0, #:upper16:.LC88
	ldrh	r1, [r3, #20]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC89
	movt	r0, #:upper16:.LC89
	ldrh	r1, [r3, #22]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC90
	movt	r0, #:upper16:.LC90
	ldrh	r1, [r3, #24]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC91
	movt	r0, #:upper16:.LC91
	ldrh	r1, [r3, #26]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #1972]
	movw	r0, #:lower16:.LC92
	movt	r0, #:upper16:.LC92
	ldr	r1, [r3, #28]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #356]
	cmp	r3, #0
	bne	.L176
	add	sp, sp, #16
	@ sp needed
	pop	{r4, pc}
.L176:
	mov	r0, r4
	add	sp, sp, #16
	@ sp needed
	pop	{r4, lr}
	b	print_smart.part.16
.L175:
	add	r2, r4, #340
	add	r3, r4, #328
	ldm	r2, {r0, r1, r2}
	stm	sp, {r0, r1, r2}
	movw	r0, #:lower16:.LC68
	movt	r0, #:upper16:.LC68
	ldm	r3, {r1, r2, r3}
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC69
	ldr	r1, [r4, #324]
	movt	r0, #:upper16:.LC69
	bl	NAND_Print_DBG
	ldr	r3, [r4, #324]
	movw	r0, #:lower16:.LC70
	movt	r0, #:upper16:.LC70
	ldrh	r1, [r3]
	bl	NAND_Print_DBG
	add	r3, r4, #352
	movw	r0, #:lower16:.LC71
	ldrh	r1, [r3]
	movt	r0, #:upper16:.LC71
	bl	NAND_Print_DBG
	ldr	r3, [r4, #324]
	movw	r0, #:lower16:.LC72
	movt	r0, #:upper16:.LC72
	ldr	r1, [r3, #8]
	bl	NAND_Print_DBG
	b	.L169
	.size	print_nftl_zone.part.17, .-print_nftl_zone.part.17
	.align	2
	.syntax unified
	.arm
	.type	crc_wrong.isra.18, %function
crc_wrong.isra.18:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	sub	ip, r3, #1
	add	r4, r3, #9
	mov	r5, #0
.L179:
	ldrb	lr, [ip, #1]!	@ zero_extendqisi2
	cmp	lr, #255
	addeq	r5, r5, #1
	cmp	ip, r4
	bne	.L179
	cmp	r5, #8
	bls	.L180
	ldrb	ip, [r3]	@ zero_extendqisi2
	cmp	ip, #255
	beq	.L189
.L180:
	mov	ip, r3
	add	r5, r3, #4
	mov	lr, #0
.L182:
	ldrb	r4, [ip, #1]!	@ zero_extendqisi2
	cmp	ip, r5
	orr	lr, r4, lr, lsl #8
	bne	.L182
	movw	ip, #65535
	ldrb	r4, [r0, #8]	@ zero_extendqisi2
	movt	ip, 43690
	cmp	lr, ip
	beq	.L183
	lsl	r4, r4, #9
.L186:
	cmp	r4, #0
	beq	.L188
	movw	ip, #:lower16:.LANCHOR2
	add	lr, r2, r4
	movt	ip, #:upper16:.LANCHOR2
	movw	r1, #65535
.L185:
	ldrb	r0, [r2], #1	@ zero_extendqisi2
	eor	r0, r0, r1
	cmp	r2, lr
	uxtb	r0, r0
	lsl	r0, r0, #1
	ldrh	r0, [ip, r0]
	eor	r1, r0, r1, lsr #8
	bne	.L185
.L184:
	ldrb	r0, [r3, #12]	@ zero_extendqisi2
	ldrb	r2, [r3, #13]	@ zero_extendqisi2
	orr	r2, r2, r0, lsl #8
	cmp	r1, r2
	beq	.L189
	movw	r0, #:lower16:.LC93
	mov	r3, r4
	movt	r0, #:upper16:.LC93
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, pc}
.L183:
	ldr	r4, [r1]
	b	.L186
.L189:
	mov	r0, #0
	pop	{r4, r5, r6, pc}
.L188:
	movw	r1, #65535
	b	.L184
	.size	crc_wrong.isra.18, .-crc_wrong.isra.18
	.align	2
	.global	_nand_sector_read
	.syntax unified
	.arm
	.type	_nand_sector_read, %function
_nand_sector_read:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.size	_nand_sector_read, .-_nand_sector_read
	.align	2
	.global	_shutdown_op
	.syntax unified
	.arm
	.type	_shutdown_op, %function
_shutdown_op:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.size	_shutdown_op, .-_shutdown_op
	.align	2
	.global	print_all_nftl_blk
	.syntax unified
	.arm
	.type	print_all_nftl_blk, %function
print_all_nftl_blk:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	movw	r4, #:lower16:.LANCHOR0
	movt	r4, #:upper16:.LANCHOR0
	ldr	r3, [r4, #12]
	cmp	r3, #0
	popeq	{r4, r5, r6, pc}
	movw	r5, #:lower16:.LC94
	add	r4, r4, #4
	movt	r5, #:upper16:.LC94
.L199:
	ldr	r1, [r3]
	mov	r0, r5
	bl	NAND_Print_DBG
	ldr	r4, [r4, #8]
	ldr	r3, [r4, #8]
	cmp	r3, #0
	bne	.L199
	pop	{r4, r5, r6, pc}
	.size	print_all_nftl_blk, .-print_all_nftl_blk
	.align	2
	.global	get_nftl_num
	.syntax unified
	.arm
	.type	get_nftl_num, %function
get_nftl_num:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	ldr	r3, [r3, #12]
	cmp	r3, #0
	beq	.L206
	mov	r0, #0
.L205:
	ldr	r3, [r3, #8]
	add	r0, r0, #1
	cmp	r3, #0
	bne	.L205
	bx	lr
.L206:
	mov	r0, r3
	bx	lr
	.size	get_nftl_num, .-get_nftl_num
	.align	2
	.global	get_nftl_cap
	.syntax unified
	.arm
	.type	get_nftl_cap, %function
get_nftl_cap:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	ldr	r3, [r3, #12]
	cmp	r3, #0
	beq	.L211
	mov	r0, #0
.L210:
	ldr	r2, [r3]
	ldr	r3, [r3, #8]
	add	r0, r0, r2
	cmp	r3, #0
	bne	.L210
	bx	lr
.L211:
	mov	r0, r3
	bx	lr
	.size	get_nftl_cap, .-get_nftl_cap
	.align	2
	.global	get_first_nftl_cap
	.syntax unified
	.arm
	.type	get_first_nftl_cap, %function
get_first_nftl_cap:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	ldr	r3, [r3, #12]
	ldr	r0, [r3]
	bx	lr
	.size	get_first_nftl_cap, .-get_first_nftl_cap
	.align	2
	.global	nftl_read
	.syntax unified
	.arm
	.type	nftl_read, %function
nftl_read:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	subs	lr, r1, #0
	beq	.L224
	mov	r1, r0
	movw	r0, #:lower16:.LANCHOR0
	movt	r0, #:upper16:.LANCHOR0
	ldr	r0, [r0, #12]
	b	.L217
.L218:
	ldr	r0, [r0, #8]
	sub	r1, r1, ip
	cmp	r0, #0
	beq	.L225
.L217:
	ldr	ip, [r0]
	cmp	r1, ip
	bcs	.L218
	mov	r3, r2
	mov	r2, lr
	ldr	lr, [r0, #40]
	mov	ip, lr
	pop	{r4, lr}
	bx	ip
.L224:
	mov	r0, lr
	pop	{r4, pc}
.L225:
	movw	r0, #:lower16:.LC95
	mov	r2, lr
	movt	r0, #:upper16:.LC95
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, pc}
	.size	nftl_read, .-nftl_read
	.align	2
	.global	nftl_write
	.syntax unified
	.arm
	.type	nftl_write, %function
nftl_write:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	subs	lr, r1, #0
	beq	.L236
	mov	r1, r0
	movw	r0, #:lower16:.LANCHOR0
	movt	r0, #:upper16:.LANCHOR0
	ldr	r0, [r0, #12]
	b	.L229
.L230:
	ldr	r0, [r0, #8]
	sub	r1, r1, ip
	cmp	r0, #0
	beq	.L237
.L229:
	ldr	ip, [r0]
	cmp	r1, ip
	bcs	.L230
	mov	r3, r2
	mov	r2, lr
	ldr	lr, [r0, #44]
	mov	ip, lr
	pop	{r4, lr}
	bx	ip
.L236:
	mov	r0, lr
	pop	{r4, pc}
.L237:
	movw	r0, #:lower16:.LC95
	mov	r2, lr
	movt	r0, #:upper16:.LC95
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, pc}
	.size	nftl_write, .-nftl_write
	.align	2
	.global	nftl_discard
	.syntax unified
	.arm
	.type	nftl_discard, %function
nftl_discard:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	subs	r2, r1, #0
	beq	.L249
	movw	r3, #:lower16:.LANCHOR0
	mov	r1, r0
	movt	r3, #:upper16:.LANCHOR0
	ldr	r0, [r3, #12]
	b	.L241
.L242:
	ldr	r0, [r0, #8]
	sub	r1, r1, r3
	cmp	r0, #0
	beq	.L250
.L241:
	ldr	r3, [r0]
	cmp	r1, r3
	bcs	.L242
	ldr	r3, [r0, #52]
	bx	r3
.L249:
	mov	r0, r2
	bx	lr
.L250:
	movw	r0, #:lower16:.LC95
	push	{r4, lr}
	movt	r0, #:upper16:.LC95
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, pc}
	.size	nftl_discard, .-nftl_discard
	.align	2
	.global	nftl_flush_write_cache
	.syntax unified
	.arm
	.type	nftl_flush_write_cache, %function
nftl_flush_write_cache:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:.LANCHOR0
	push	{r4, lr}
	movt	r3, #:upper16:.LANCHOR0
	ldr	r4, [r3, #12]
	cmp	r4, #0
	beq	.L256
.L253:
	ldr	r3, [r4, #48]
	mov	r0, r4
	movw	r1, #65535
	blx	r3
	ldr	r4, [r4, #8]
	cmp	r4, #0
	bne	.L253
.L256:
	mov	r0, #0
	pop	{r4, pc}
	.size	nftl_flush_write_cache, .-nftl_flush_write_cache
	.align	2
	.global	nftl_malloc
	.syntax unified
	.arm
	.type	nftl_malloc, %function
nftl_malloc:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	cmp	r0, #1572864
	push	{r4, lr}
	mov	r4, r0
	bls	.L259
	movw	r0, #:lower16:.LC96
	mov	r1, r4
	movt	r0, #:upper16:.LC96
	bl	NAND_Print
.L259:
	mov	r0, r4
	pop	{r4, lr}
	b	NAND_Malloc
	.size	nftl_malloc, .-nftl_malloc
	.align	2
	.syntax unified
	.arm
	.type	init_nrc.isra.0, %function
init_nrc.isra.0:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r6, r0
	sub	sp, sp, #8
	mov	r0, #16
	mov	r7, r1
	bl	NAND_Malloc
	subs	r4, r0, #0
	beq	.L281
	mov	r2, #16
	mov	r1, #0
	bl	NAND_Memset
	ldr	r3, [r6]
	ldrh	r0, [r3, #26]
	ldrh	r5, [r3, #20]
	add	r0, r0, #64
	mul	r0, r5, r0
	bl	nftl_malloc
	cmp	r0, #0
	str	r0, [r4, #8]
	beq	.L282
	lsl	r8, r5, #2
	mov	r0, r8
	bl	nftl_malloc
	cmp	r0, #0
	str	r0, [r4, #12]
	beq	.L283
	mov	r2, r8
	mov	r1, #255
	bl	NAND_Memset
	mov	r0, r5
	bl	nftl_malloc
	cmp	r0, #0
	str	r0, [r4, #4]
	beq	.L284
	mov	r2, r5
	mov	r1, #0
	bl	NAND_Memset
	str	r4, [r7]
	movw	r0, #:lower16:.LC101
	ldr	r1, [r6]
	mov	r2, r4
	ldr	ip, [r4, #4]
	movt	r0, #:upper16:.LC101
	ldr	r3, [r4, #8]
	ldrh	r1, [r1, #20]
	str	ip, [sp, #4]
	ldr	ip, [r4, #12]
	str	ip, [sp]
	bl	NAND_Print
	mov	r0, #0
.L263:
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L281:
	movw	r0, #:lower16:.LC97
	movw	r2, #1254
	movt	r0, #:upper16:.LC97
	ldr	r1, .L285
	bl	NAND_Print
	mvn	r0, #0
	b	.L263
.L283:
	movw	r0, #:lower16:.LC99
	movw	r2, #1268
	movt	r0, #:upper16:.LC99
	ldr	r1, .L285
	bl	NAND_Print
.L265:
	ldr	r0, [r4, #8]
	cmp	r0, #0
	beq	.L268
	mov	r1, #0
	bl	NAND_Free
.L268:
	ldr	r0, [r4, #12]
	cmp	r0, #0
	beq	.L269
	mov	r1, #0
	bl	NAND_Free
.L269:
	ldr	r0, [r4, #4]
	cmp	r0, #0
	beq	.L270
	mov	r1, #0
	bl	NAND_Free
.L270:
	mov	r0, r4
	mov	r1, #0
	bl	NAND_Free
	mov	r3, #0
	mvn	r0, #0
	str	r3, [r7]
	b	.L263
.L282:
	movw	r0, #:lower16:.LC98
	movw	r2, #1263
	ldr	r1, .L285
	movt	r0, #:upper16:.LC98
	bl	NAND_Print
	b	.L265
.L284:
	movw	r0, #:lower16:.LC100
	movw	r2, #1274
	ldr	r1, .L285
	movt	r0, #:upper16:.LC100
	bl	NAND_Print
	b	.L265
.L286:
	.align	2
.L285:
	.word	.LANCHOR2+512
	.size	init_nrc.isra.0, .-init_nrc.isra.0
	.align	2
	.global	nftl_free
	.syntax unified
	.arm
	.type	nftl_free, %function
nftl_free:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r1, #0
	b	NAND_Free
	.size	nftl_free, .-nftl_free
	.align	2
	.global	add_nftl_blk_list
	.syntax unified
	.arm
	.type	add_nftl_blk_list, %function
add_nftl_blk_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, #0
	str	r3, [r1, #8]
	b	.L289
.L290:
	mov	r0, r3
.L289:
	ldr	r3, [r0, #8]
	cmp	r3, #0
	bne	.L290
	str	r1, [r0, #8]
	bx	lr
	.size	add_nftl_blk_list, .-add_nftl_blk_list
	.align	2
	.global	del_last_nftl_blk
	.syntax unified
	.arm
	.type	del_last_nftl_blk, %function
del_last_nftl_blk:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #8]
	b	.L292
.L294:
	ldr	r2, [r3, #8]
	cmp	r2, #0
	beq	.L296
	mov	r0, r3
	mov	r3, r2
.L292:
	cmp	r3, #0
	bne	.L294
	mov	r0, r3
	bx	lr
.L296:
	str	r2, [r0, #8]
	mov	r0, r3
	bx	lr
	.size	del_last_nftl_blk, .-del_last_nftl_blk
	.align	2
	.global	m5_readretry_init
	.syntax unified
	.arm
	.type	m5_readretry_init, %function
m5_readretry_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r1, [r0, #252]
	mov	r2, r0
	mov	ip, #0
	push	{r4, lr}
	str	ip, [r2, #176]
	mov	r3, #15
	movw	r4, #:lower16:.LANCHOR0
	movw	r0, #:lower16:.LC102
	ldrh	r2, [r1, #42]
	movt	r4, #:upper16:.LANCHOR0
	mov	r1, r3
	movt	r0, #:upper16:.LC102
	strb	r3, [r4, #77]
	strb	r2, [r4, #76]
	bl	NAND_Print_DBG
	ldrb	r3, [r4, #76]	@ zero_extendqisi2
	cmp	r3, #32
	mvnne	r0, #99
	moveq	r0, #0
	pop	{r4, pc}
	.size	m5_readretry_init, .-m5_readretry_init
	.align	2
	.global	m5_vender_set_param
	.syntax unified
	.arm
	.type	m5_vender_set_param, %function
m5_vender_set_param:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, lr}
	mov	r4, r1
	sub	sp, sp, #20
	mov	r1, #1
	str	r3, [sp, #8]
	mov	r3, r2
	str	r1, [sp, #4]
	mov	r2, #0
	str	r4, [sp]
	ldr	r1, .L303
	bl	set_cmd_with_nand_bus
	ldrb	ip, [r4, #3]	@ zero_extendqisi2
	mov	r5, r0
	movw	r0, #:lower16:.LC103
	ldrb	r3, [r4, #2]	@ zero_extendqisi2
	ldrb	r2, [r4, #1]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC103
	ldrb	r1, [r4]	@ zero_extendqisi2
	str	ip, [sp]
	bl	NAND_Print_DBG
	mov	r0, r5
	add	sp, sp, #20
	@ sp needed
	pop	{r4, r5, pc}
.L304:
	.align	2
.L303:
	.word	.LANCHOR1+4
	.size	m5_vender_set_param, .-m5_vender_set_param
	.align	2
	.global	m5_readretry_exit
	.syntax unified
	.arm
	.type	m5_readretry_exit, %function
m5_readretry_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.size	m5_readretry_exit, .-m5_readretry_exit
	.align	2
	.global	m5_set_readretry
	.syntax unified
	.arm
	.type	m5_set_readretry, %function
m5_set_readretry:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r4, r0
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	ldr	ip, [r4, #176]
	mov	r0, r4
	ldr	r1, .L312
	mov	r3, #4
	sub	r2, r1, #4
	add	r1, r1, ip, lsl #2
	bl	m5_vender_set_param
	subs	r5, r0, #0
	beq	.L307
	movw	r0, #:lower16:.LC104
	ldr	r1, [r4, #176]
	movt	r0, #:upper16:.LC104
	bl	NAND_Print
.L307:
	mov	r0, r4
	bl	nand_disable_chip
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L313:
	.align	2
.L312:
	.word	.LANCHOR1+12
	.size	m5_set_readretry, .-m5_set_readretry
	.align	2
	.global	m5_read_page_end
	.syntax unified
	.arm
	.type	m5_read_page_end, %function
m5_read_page_end:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, r7, r8, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r1, [r0]
	sub	sp, sp, #8
	mov	r8, r0
	ldr	r0, [r3]
	bl	nci_get_from_nsi
	mov	r7, r0
	mov	r0, r8
	bl	m0_read_page_end_not_retry
	cmn	r0, #2
	mov	r5, r0
	beq	.L324
	mov	r0, r5
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L324:
	movw	r0, #:lower16:.LC105
	movw	r6, #:lower16:.LANCHOR0
	movt	r0, #:upper16:.LC105
	movt	r6, #:upper16:.LANCHOR0
	bl	NAND_Print_DBG
	ldrb	r3, [r6, #77]	@ zero_extendqisi2
	cmp	r3, #1
	ble	.L316
	mov	r4, #1
	b	.L318
.L317:
	ldrb	r3, [r6, #77]	@ zero_extendqisi2
	cmp	r3, r4
	ble	.L316
.L318:
	str	r4, [r7, #176]
	mov	r0, r7
	bl	m5_set_readretry
	subs	r5, r0, #0
	add	r4, r4, #1
	bne	.L317
	mov	r0, r8
	bl	m0_read_page_start
	mov	r5, r0
	mov	r0, r8
	bl	m0_read_page_end_not_retry
	orr	r5, r5, r0
	cmp	r5, #0
	cmpne	r5, #10
	bne	.L317
	ldr	r2, [r7, #248]
	movw	r0, #:lower16:.LC106
	ldr	lr, [r8, #8]
	movt	r0, #:upper16:.LC106
	ldr	ip, [r7, #176]
	mov	r5, #10
	ldr	r1, [r2, #8]
	ldr	r3, [r8, #4]
	ldr	r2, [r7, #20]
	str	lr, [sp]
	str	ip, [sp, #4]
	bl	NAND_Print_DBG
.L316:
	mov	r3, #0
	mov	r0, r7
	str	r3, [r7, #176]
	bl	m5_set_readretry
	mov	r0, r5
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
	.size	m5_read_page_end, .-m5_read_page_end
	.align	2
	.global	m5_special_init
	.syntax unified
	.arm
	.type	m5_special_init, %function
m5_special_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r4, [r3, #8]
	cmp	r4, #0
	beq	.L326
	mov	r5, #0
.L327:
	mov	r0, r4
	bl	m5_readretry_init
	ldr	r4, [r4]
	orr	r5, r5, r0
	cmp	r4, #0
	bne	.L327
	cmp	r5, #0
	beq	.L326
	movw	r3, #:lower16:.LANCHOR0
	movw	r0, #:lower16:.LC108
	movt	r3, #:upper16:.LANCHOR0
	movt	r0, #:upper16:.LC108
	ldrb	r2, [r3, #77]	@ zero_extendqisi2
	ldrb	r1, [r3, #76]	@ zero_extendqisi2
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L326:
	movw	r3, #:lower16:.LANCHOR0
	movw	ip, #:lower16:function_read_page_end
	movt	r3, #:upper16:.LANCHOR0
	movw	lr, #:lower16:m5_read_page_end
	movw	r0, #:lower16:.LC107
	movt	ip, #:upper16:function_read_page_end
	movt	lr, #:upper16:m5_read_page_end
	ldrb	r2, [r3, #77]	@ zero_extendqisi2
	ldrb	r1, [r3, #76]	@ zero_extendqisi2
	mov	r5, #0
	movt	r0, #:upper16:.LC107
	str	lr, [ip]
	bl	NAND_Print_DBG
	mov	r0, r5
	pop	{r4, r5, r6, pc}
	.size	m5_special_init, .-m5_special_init
	.align	2
	.global	m5_special_exit
	.syntax unified
	.arm
	.type	m5_special_exit, %function
m5_special_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r3, [r3, #8]
	cmp	r3, #0
	beq	.L336
.L337:
	ldr	r3, [r3]
	cmp	r3, #0
	bne	.L337
.L336:
	movw	r0, #:lower16:.LC109
	movt	r0, #:upper16:.LC109
	bl	NAND_Print_DBG
	mov	r0, #0
	pop	{r4, pc}
	.size	m5_special_exit, .-m5_special_exit
	.align	2
	.global	m0_special_init
	.syntax unified
	.arm
	.type	m0_special_init, %function
m0_special_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:function_read_page_end
	movw	r2, #:lower16:m0_read_page_end_not_retry
	movw	r0, #:lower16:.LC110
	push	{r4, lr}
	movt	r3, #:upper16:function_read_page_end
	movt	r2, #:upper16:m0_read_page_end_not_retry
	movt	r0, #:upper16:.LC110
	str	r2, [r3]
	bl	NAND_Print_DBG
	mov	r0, #0
	pop	{r4, pc}
	.size	m0_special_init, .-m0_special_init
	.align	2
	.global	m0_special_exit
	.syntax unified
	.arm
	.type	m0_special_exit, %function
m0_special_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r0, #:lower16:.LC111
	push	{r4, lr}
	movt	r0, #:upper16:.LC111
	bl	NAND_Print_DBG
	mov	r0, #0
	pop	{r4, pc}
	.size	m0_special_exit, .-m0_special_exit
	.align	2
	.global	m0_is_lsb_page
	.syntax unified
	.arm
	.type	m0_is_lsb_page, %function
m0_is_lsb_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #1
	bx	lr
	.size	m0_is_lsb_page, .-m0_is_lsb_page
	.align	2
	.global	m3_lsb_init
	.syntax unified
	.arm
	.type	m3_lsb_init, %function
m3_lsb_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r2, [r0, #252]
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	mvn	r0, #91
	push	{r4, r5, lr}
	mov	ip, #5
	ldr	r2, [r2, #40]
	mvn	r1, #90
	mvn	r5, #79
	mvn	r4, #78
	strb	r0, [r3, #84]
	mvn	lr, #54
	strb	r2, [r3, #80]
	lsr	r0, r2, #16
	lsr	r2, r2, #8
	strb	ip, [r3, #81]
	strb	r1, [r3, #85]
	mov	ip, #1
	mov	r1, #37
	strb	r0, [r3, #78]
	strb	r2, [r3, #79]
	mov	r0, #0
	strb	r5, [r3, #86]
	strb	r4, [r3, #87]
	strb	lr, [r3, #88]
	strb	ip, [r3, #96]
	strb	r1, [r3, #92]
	strb	r1, [r3, #93]
	strb	r1, [r3, #94]
	strb	r1, [r3, #95]
	pop	{r4, r5, pc}
	.size	m3_lsb_init, .-m3_lsb_init
	.align	2
	.global	m3_lsb_exit
	.syntax unified
	.arm
	.type	m3_lsb_exit, %function
m3_lsb_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r4, r0
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	mov	r2, #0
	mov	r0, r4
	mov	r1, r2
	bl	set_one_cmd
	mov	r0, r4
	mov	r1, #0
	bl	set_one_addr
	mov	r0, r4
	mov	r1, #0
	bl	set_one_addr
	mov	r0, r4
	mov	r1, #0
	bl	set_one_addr
	mov	r0, r4
	mov	r1, #0
	bl	set_one_addr
	mov	r0, r4
	mov	r1, #0
	bl	set_one_addr
	mov	r2, #1
	mov	r1, #48
	mov	r0, r4
	bl	set_one_cmd
	movw	r0, #:lower16:.LC112
	movt	r0, #:upper16:.LC112
	bl	NAND_Print_DBG
	mov	r0, r4
	bl	nand_disable_chip
	mov	r0, #0
	pop	{r4, pc}
	.size	m3_lsb_exit, .-m3_lsb_exit
	.align	2
	.global	m7_activation_seq
	.syntax unified
	.arm
	.type	m7_activation_seq, %function
m7_activation_seq:
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r2, .L358
	push	{r4, r5, r6, r7, lr}
	mov	r5, r0
	sub	sp, sp, #44
	mov	r4, #0
	add	r3, sp, #16
	add	r7, sp, #28
	ldr	r1, [r2, #4]	@ unaligned
	ldr	r0, [r2]	@ unaligned
	ldrb	r2, [r2, #8]	@ zero_extendqisi2
	str	r4, [sp, #32]
	stmia	r3!, {r0, r1}
	mov	r0, r5
	strb	r2, [r3]
	str	r4, [sp, #28]
	strb	r4, [sp, #36]
	bl	nand_enable_chip
	ldr	r0, [r5, #248]
	bl	ndfc_disable_randomize
	mov	r1, #2
	mov	r3, r4
	mov	r2, r4
	str	r4, [sp, #4]
	str	r4, [sp]
	mov	r0, r5
	str	r1, [sp, #8]
	ldr	r1, .L358+4
	bl	set_cmd_with_nand_bus
	mov	r1, #9
	mov	r3, #1
	mov	r2, r4
	mov	r6, r0
	str	r1, [sp, #8]
	mov	r0, r5
	add	r1, sp, #16
	str	r3, [sp, #4]
	str	r7, [sp]
	ldr	r3, .L358+8
	bl	set_cmd_with_nand_bus
	orr	r4, r6, r0
	mov	r0, r5
	bl	nand_disable_chip
	cmp	r4, #0
	beq	.L353
	movw	r0, #:lower16:.LC113
	mov	r1, r4
	movt	r0, #:upper16:.LC113
	bl	NAND_Print
.L353:
	mov	r0, r4
	add	sp, sp, #44
	@ sp needed
	pop	{r4, r5, r6, r7, pc}
.L359:
	.align	2
.L358:
	.word	.LANCHOR2+524
	.word	.LANCHOR1+72
	.word	.LANCHOR1+76
	.size	m7_activation_seq, .-m7_activation_seq
	.align	2
	.global	m7_readretry_init
	.syntax unified
	.arm
	.type	m7_readretry_init, %function
m7_readretry_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r2, [r0, #252]
	mov	r1, #0
	push	{r4, lr}
	movw	r4, #:lower16:.LANCHOR0
	str	r1, [r0, #176]
	movt	r4, #:upper16:.LANCHOR0
	ldr	r3, [r2, #40]
	ubfx	r1, r3, #16, #8
	strb	r3, [r4, #99]
	sub	r2, r1, #48
	lsr	r3, r3, #8
	uxtb	r2, r2
	strb	r1, [r4, #97]
	strb	r3, [r4, #98]
	cmp	r2, #1
	bls	.L365
.L361:
	cmp	r2, #2
	bhi	.L363
	movw	r0, #:lower16:.LC114
	ldrb	r1, [r4, #98]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC114
	bl	NAND_Print_DBG
	mov	r0, #0
	pop	{r4, pc}
.L365:
	bl	m7_activation_seq
	ldrb	r2, [r4, #97]	@ zero_extendqisi2
	sub	r2, r2, #48
	uxtb	r2, r2
	b	.L361
.L363:
	mvn	r0, #98
	pop	{r4, pc}
	.size	m7_readretry_init, .-m7_readretry_init
	.align	2
	.global	m7_readretry_exit
	.syntax unified
	.arm
	.type	m7_readretry_exit, %function
m7_readretry_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.size	m7_readretry_exit, .-m7_readretry_exit
	.align	2
	.global	m7_read_retry_clock_save
	.syntax unified
	.arm
	.type	m7_read_retry_clock_save, %function
m7_read_retry_clock_save:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #248]
	movw	r1, #:lower16:.LANCHOR0
	movt	r1, #:upper16:.LANCHOR0
	push	{r4, lr}
	add	r2, r1, #100
	mov	r4, r0
	add	r1, r1, #104
	ldr	r0, [r3, #8]
	bl	NAND_GetClk
	ldr	r3, [r4, #248]
	mov	r2, #20
	mov	r1, #10
	ldr	r0, [r3, #8]
	bl	NAND_SetClk
	mov	r0, #0
	pop	{r4, pc}
	.size	m7_read_retry_clock_save, .-m7_read_retry_clock_save
	.align	2
	.global	m7_read_retry_clock_recover
	.syntax unified
	.arm
	.type	m7_read_retry_clock_recover, %function
m7_read_retry_clock_recover:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r1, [r0, #248]
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	push	{r4, lr}
	ldr	r2, [r3, #100]
	ldr	r0, [r1, #8]
	ldr	r1, [r3, #104]
	bl	NAND_SetClk
	mov	r0, #0
	pop	{r4, pc}
	.size	m7_read_retry_clock_recover, .-m7_read_retry_clock_recover
	.align	2
	.global	m7_end_one_read_retry
	.syntax unified
	.arm
	.type	m7_end_one_read_retry, %function
m7_end_one_read_retry:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	ldrb	r3, [r3, #97]	@ zero_extendqisi2
	sub	r3, r3, #48
	cmp	r3, #1
	bls	.L375
	mov	r0, #0
	bx	lr
.L375:
	push	{r4, lr}
	bl	m7_end_one_read_retry.part.1
	mov	r0, #0
	pop	{r4, pc}
	.size	m7_end_one_read_retry, .-m7_end_one_read_retry
	.align	2
	.global	m7_vender_set_param
	.syntax unified
	.arm
	.type	m7_vender_set_param, %function
m7_vender_set_param:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, lr}
	movw	r4, #:lower16:.LANCHOR0
	movt	r4, #:upper16:.LANCHOR0
	mov	r7, r2
	mov	r6, r0
	sub	sp, sp, #20
	ldrb	r3, [r4, #97]	@ zero_extendqisi2
	mov	r8, r1
	sub	r2, r3, #48
	cmp	r2, #1
	movhi	r0, #0
	bls	.L381
.L377:
	cmp	r3, #50
	beq	.L382
	add	sp, sp, #20
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
.L382:
	mov	r2, #1
	mov	r1, #4
	stmib	sp, {r1, r2}
	mov	r3, r7
	str	r8, [sp]
	mov	r0, r6
	ldr	r1, .L383
	bl	set_cmd_with_nand_bus
	add	sp, sp, #20
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
.L381:
	mov	r9, #0
	mov	r3, #2
	str	r3, [sp, #8]
	mov	r2, r9
	mov	r3, r9
	ldr	r1, .L383+4
	str	r9, [sp, #4]
	str	r9, [sp]
	bl	set_cmd_with_nand_bus
	mov	r3, #3
	mov	r1, #1
	mov	r5, r0
	str	r3, [sp, #8]
	str	r1, [sp, #4]
	mov	r3, r7
	mov	r0, r6
	mov	r2, r9
	str	r8, [sp]
	ldr	r1, .L383+8
	bl	set_cmd_with_nand_bus
	ldrb	r3, [r4, #97]	@ zero_extendqisi2
	orr	r0, r5, r0
	b	.L377
.L384:
	.align	2
.L383:
	.word	.LANCHOR1+92
	.word	.LANCHOR1+72
	.word	.LANCHOR1+88
	.size	m7_vender_set_param, .-m7_vender_set_param
	.align	2
	.global	m7_set_readretry
	.syntax unified
	.arm
	.type	m7_set_readretry, %function
m7_set_readretry:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r4, r0
	mov	r6, r1
	bl	m7_read_retry_clock_save
	mov	r0, r4
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	ldr	r0, [r4, #248]
	bl	ndfc_is_toogle_interface
	cmp	r0, #0
	bne	.L406
	movw	r5, #:lower16:.LANCHOR0
	movt	r5, #:upper16:.LANCHOR0
	ldrb	r3, [r5, #97]	@ zero_extendqisi2
	cmp	r3, #48
	beq	.L409
.L387:
	cmp	r3, #49
	beq	.L410
	cmp	r3, #50
	movne	r6, #0
	beq	.L411
.L391:
	ldr	r3, [r5]
	cmp	r3, #1
	beq	.L412
.L393:
	mov	r0, r4
	bl	nand_disable_chip
	ldr	r3, [r4, #248]
	ldr	r2, [r5, #100]
	ldr	r1, [r5, #104]
	ldr	r0, [r3, #8]
	bl	NAND_SetClk
	mov	r0, r6
	pop	{r4, r5, r6, pc}
.L406:
	ldr	r0, [r4, #248]
	movw	r5, #:lower16:.LANCHOR0
	bl	ndfc_set_legacy_interface
	movt	r5, #:upper16:.LANCHOR0
	mov	r3, #1
	str	r3, [r5]
	ldrb	r3, [r5, #97]	@ zero_extendqisi2
	cmp	r3, #48
	bne	.L387
.L409:
	cmp	r6, #255
	beq	.L388
	cmp	r6, #0
	bne	.L413
.L389:
	ldr	r3, [r4, #176]
	mov	r2, #16
	ldr	r1, .L414
	strb	r2, [r5, #98]
	add	r3, r3, r3, lsl #1
	add	r1, r1, r3
.L390:
	ldr	r2, .L414+4
	mov	r3, #3
	mov	r0, r4
.L408:
	bl	m7_vender_set_param
	mov	r2, #0
	mov	r6, r0
	mov	r1, #182
	mov	r0, r4
	bl	set_one_cmd
	ldr	r3, [r5]
	cmp	r3, #1
	bne	.L393
.L412:
	ldr	r0, [r4, #248]
	bl	ndfc_set_toogle_interface
	b	.L393
.L413:
	tst	r6, #1
	bne	.L389
.L388:
	ldr	r3, [r4, #176]
	mov	r2, #20
	ldr	r1, .L414+8
	strb	r2, [r5, #98]
	add	r3, r3, r3, lsl #1
	add	r1, r1, r3
	b	.L390
.L410:
	ldr	r2, [r4, #176]
	mov	r3, #3
	ldr	ip, .L414+12
	mov	r0, r4
	add	r1, r2, r2, lsl #1
	sub	r2, ip, #12
	add	r1, ip, r1
	b	.L408
.L411:
	ldr	ip, [r4, #176]
	mov	r3, #1
	ldr	r1, .L414+16
	mov	r0, r4
	sub	r2, r1, #4
	add	r1, r1, ip, lsl #2
	bl	m7_vender_set_param
	mov	r2, #0
	mov	r6, r0
	mov	r1, #93
	mov	r0, r4
	bl	set_one_cmd
	b	.L391
.L415:
	.align	2
.L414:
	.word	.LANCHOR1+96
	.word	.LANCHOR1+204
	.word	.LANCHOR1+144
	.word	.LANCHOR1+228
	.word	.LANCHOR1+260
	.size	m7_set_readretry, .-m7_set_readretry
	.align	2
	.global	m7_exit_readretry
	.syntax unified
	.arm
	.type	m7_exit_readretry, %function
m7_exit_readretry:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r3, #0
	sub	sp, sp, #8
	mov	r5, r0
	str	r3, [sp, #4]
	bl	nand_enable_chip
	ldr	r0, [r5, #248]
	bl	ndfc_disable_randomize
	ldr	r0, [r5, #248]
	bl	ndfc_is_toogle_interface
	cmp	r0, #0
	bne	.L425
	movw	r4, #:lower16:.LANCHOR0
	movt	r4, #:upper16:.LANCHOR0
	ldrb	r3, [r4, #97]	@ zero_extendqisi2
	cmp	r3, #48
	beq	.L427
.L418:
	cmp	r3, #49
	beq	.L428
	cmp	r3, #50
	movne	r6, #0
	beq	.L429
.L419:
	ldr	r3, [r4]
	cmp	r3, #1
	beq	.L430
.L421:
	mov	r0, r5
	bl	nand_disable_chip
	mov	r0, r6
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, pc}
.L425:
	ldr	r0, [r5, #248]
	movw	r4, #:lower16:.LANCHOR0
	bl	ndfc_set_legacy_interface
	movt	r4, #:upper16:.LANCHOR0
	mov	r3, #1
	str	r3, [r4]
	ldrb	r3, [r4, #97]	@ zero_extendqisi2
	cmp	r3, #48
	bne	.L418
.L427:
	mov	r3, #3
	ldr	r2, .L431
	add	r1, sp, #4
	mov	r0, r5
	bl	m7_vender_set_param
	ldr	r3, [r4]
	mov	r6, r0
	cmp	r3, #1
	bne	.L421
.L430:
	ldr	r0, [r5, #248]
	bl	ndfc_set_toogle_interface
	b	.L421
.L428:
	mov	r3, #3
	ldr	r2, .L431+4
	add	r1, sp, #4
	mov	r0, r5
	bl	m7_vender_set_param
	mov	r6, r0
	b	.L419
.L429:
	mov	r2, #1
	mov	r1, #255
	mov	r0, r5
	bl	set_one_cmd
	mov	r3, #1
	ldr	r2, .L431+8
	add	r1, sp, #4
	mov	r0, r5
	bl	m7_vender_set_param
	mov	r6, r0
	b	.L419
.L432:
	.align	2
.L431:
	.word	.LANCHOR1+204
	.word	.LANCHOR1+216
	.word	.LANCHOR1+256
	.size	m7_exit_readretry, .-m7_exit_readretry
	.align	2
	.global	m7_read_page_end
	.syntax unified
	.arm
	.type	m7_read_page_end, %function
m7_read_page_end:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, r7, r8, r9, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r1, [r0]
	sub	sp, sp, #12
	mov	r6, r0
	ldr	r0, [r3]
	bl	nci_get_from_nsi
	mov	r7, r0
	mov	r0, r6
	bl	m0_read_page_end_not_retry
	cmn	r0, #2
	mov	r5, r0
	beq	.L451
	mov	r0, r5
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
.L451:
	movw	r0, #:lower16:.LC115
	movw	r8, #:lower16:.LANCHOR0
	movt	r0, #:upper16:.LC115
	movt	r8, #:upper16:.LANCHOR0
	bl	NAND_Print_DBG
	ldrb	r3, [r8, #98]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L435
	movw	r9, #:lower16:.LC117
	mov	r4, #0
	movt	r9, #:upper16:.LC117
	b	.L442
.L436:
	ldrb	r3, [r8, #98]	@ zero_extendqisi2
	add	r4, r4, #1
	cmp	r3, r4
	ble	.L435
.L442:
	str	r4, [r7, #176]
	mov	r0, r7
	ldrh	r1, [r6, #8]
	bl	m7_set_readretry
	subs	r5, r0, #0
	bne	.L436
	mov	r0, r6
	bl	m0_read_page_start
	mov	r5, r0
	mov	r0, r6
	bl	m0_read_page_end_not_retry
	ldrb	r3, [r8, #97]	@ zero_extendqisi2
	orr	r5, r5, r0
	sub	r3, r3, #48
	cmp	r3, #1
	bls	.L452
.L437:
	ldr	ip, [r6, #20]
	mov	r0, #0
	sub	r3, ip, #1
	add	r1, ip, #9
.L439:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r2, #255
	addeq	r0, r0, #1
	cmp	r1, r3
	bne	.L439
	cmp	r0, #8
	bls	.L440
	ldrb	r3, [ip]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L453
.L440:
	cmp	r5, #0
	cmpne	r5, #10
	bne	.L436
	ldr	r2, [r7, #248]
	movw	r0, #:lower16:.LC116
	ldr	lr, [r6, #8]
	movt	r0, #:upper16:.LC116
	ldr	ip, [r7, #176]
	mov	r5, #10
	ldr	r1, [r2, #8]
	ldr	r3, [r6, #4]
	ldr	r2, [r7, #20]
	str	lr, [sp]
	str	ip, [sp, #4]
	bl	NAND_Print_DBG
.L435:
	mov	r3, #0
	mov	r0, r7
	str	r3, [r7, #176]
	bl	m7_exit_readretry
	mov	r0, r5
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
.L453:
	ldr	r1, [r7, #248]
	mov	r0, r9
	ldr	lr, [r7, #176]
	mvn	r5, #1
	ldr	ip, [r6, #8]
	ldr	r3, [r6, #4]
	ldr	r2, [r7, #20]
	ldr	r1, [r1, #8]
	stm	sp, {ip, lr}
	bl	NAND_Print_DBG
	b	.L436
.L452:
	mov	r0, r7
	bl	m7_end_one_read_retry.part.1
	b	.L437
	.size	m7_read_page_end, .-m7_read_page_end
	.align	2
	.global	m7_special_init
	.syntax unified
	.arm
	.type	m7_special_init, %function
m7_special_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r4, [r3, #8]
	cmp	r4, #0
	beq	.L455
	mov	r5, #0
.L456:
	mov	r0, r4
	bl	m7_readretry_init
	ldr	r4, [r4]
	orr	r5, r5, r0
	cmp	r4, #0
	bne	.L456
	cmp	r5, #0
	beq	.L455
	movw	r1, #:lower16:.LANCHOR0
	movw	r0, #:lower16:.LC119
	movt	r1, #:upper16:.LANCHOR0
	movt	r0, #:upper16:.LC119
	ldrb	r3, [r1, #99]	@ zero_extendqisi2
	ldrb	r2, [r1, #98]	@ zero_extendqisi2
	ldrb	r1, [r1, #97]	@ zero_extendqisi2
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L455:
	movw	r1, #:lower16:.LANCHOR0
	movw	ip, #:lower16:function_read_page_end
	movt	r1, #:upper16:.LANCHOR0
	movw	lr, #:lower16:m7_read_page_end
	movw	r0, #:lower16:.LC118
	ldrb	r3, [r1, #99]	@ zero_extendqisi2
	ldrb	r2, [r1, #98]	@ zero_extendqisi2
	movt	ip, #:upper16:function_read_page_end
	movt	lr, #:upper16:m7_read_page_end
	ldrb	r1, [r1, #97]	@ zero_extendqisi2
	mov	r5, #0
	movt	r0, #:upper16:.LC118
	str	lr, [ip]
	bl	NAND_Print_DBG
	mov	r0, r5
	pop	{r4, r5, r6, pc}
	.size	m7_special_init, .-m7_special_init
	.align	2
	.global	m7_special_exit
	.syntax unified
	.arm
	.type	m7_special_exit, %function
m7_special_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r3, [r3, #8]
	cmp	r3, #0
	beq	.L465
.L466:
	ldr	r3, [r3]
	cmp	r3, #0
	bne	.L466
.L465:
	movw	r0, #:lower16:.LC120
	movt	r0, #:upper16:.LC120
	bl	NAND_Print_DBG
	mov	r0, #0
	pop	{r4, pc}
	.size	m7_special_exit, .-m7_special_exit
	.align	2
	.global	m6_readretry_init
	.syntax unified
	.arm
	.type	m6_readretry_init, %function
m6_readretry_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r1, [r0, #252]
	mov	r2, r0
	push	{r4, lr}
	mov	r4, #0
	str	r4, [r2, #176]
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	movw	r0, #:lower16:.LC121
	ldr	r2, [r1, #40]
	movt	r0, #:upper16:.LC121
	strb	r2, [r3, #110]
	lsr	ip, r2, #8
	lsr	r2, r2, #16
	uxtb	r1, ip
	strb	ip, [r3, #109]
	strb	r2, [r3, #108]
	bl	NAND_Print_DBG
	mov	r0, r4
	pop	{r4, pc}
	.size	m6_readretry_init, .-m6_readretry_init
	.align	2
	.global	m6_readretry_exit
	.syntax unified
	.arm
	.type	m6_readretry_exit, %function
m6_readretry_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.size	m6_readretry_exit, .-m6_readretry_exit
	.align	2
	.global	m6_vender_pre_condition
	.syntax unified
	.arm
	.type	m6_vender_pre_condition, %function
m6_vender_pre_condition:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r1, #0
	sub	sp, sp, #16
	mov	r3, #2
	mov	r2, r1
	str	r3, [sp, #8]
	mov	r3, r1
	str	r1, [sp, #4]
	str	r1, [sp]
	ldr	r1, .L481
	bl	set_cmd_with_nand_bus
	subs	r4, r0, #0
	beq	.L476
	movw	r0, #:lower16:.LC122
	movt	r0, #:upper16:.LC122
	bl	NAND_Print
.L476:
	mov	r0, r4
	add	sp, sp, #16
	@ sp needed
	pop	{r4, pc}
.L482:
	.align	2
.L481:
	.word	.LANCHOR1+336
	.size	m6_vender_pre_condition, .-m6_vender_pre_condition
	.align	2
	.global	m6_read_retry_clock_save
	.syntax unified
	.arm
	.type	m6_read_retry_clock_save, %function
m6_read_retry_clock_save:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #248]
	movw	r1, #:lower16:.LANCHOR0
	movt	r1, #:upper16:.LANCHOR0
	push	{r4, lr}
	add	r2, r1, #112
	mov	r4, r0
	add	r1, r1, #116
	ldr	r0, [r3, #8]
	bl	NAND_GetClk
	ldr	r3, [r4, #248]
	mov	r2, #20
	mov	r1, #10
	ldr	r0, [r3, #8]
	bl	NAND_SetClk
	mov	r0, #0
	pop	{r4, pc}
	.size	m6_read_retry_clock_save, .-m6_read_retry_clock_save
	.align	2
	.global	m6_read_retry_clock_recover
	.syntax unified
	.arm
	.type	m6_read_retry_clock_recover, %function
m6_read_retry_clock_recover:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r1, [r0, #248]
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	push	{r4, lr}
	ldr	r2, [r3, #112]
	ldr	r0, [r1, #8]
	ldr	r1, [r3, #116]
	bl	NAND_SetClk
	mov	r0, #0
	pop	{r4, pc}
	.size	m6_read_retry_clock_recover, .-m6_read_retry_clock_recover
	.align	2
	.global	m6_vender_set_param
	.syntax unified
	.arm
	.type	m6_vender_set_param, %function
m6_vender_set_param:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	movw	r7, #:lower16:.LANCHOR0
	movt	r7, #:upper16:.LANCHOR0
	mov	r9, r2
	movw	r6, #:lower16:.LANCHOR1
	sub	sp, sp, #20
	ldrb	r2, [r7, #108]	@ zero_extendqisi2
	mov	r5, r1
	mov	r10, r3
	mov	r8, r0
	cmp	r2, #18
	movne	r4, #0
	movtne	r6, #:upper16:.LANCHOR1
	beq	.L509
	cmp	r2, #17
	beq	.L510
.L495:
	cmp	r2, #16
	beq	.L511
.L499:
	mov	r2, #0
	ldrb	r1, [r6, #360]	@ zero_extendqisi2
	mov	r0, r8
	bl	set_one_cmd
	ldrb	r1, [r6, #361]	@ zero_extendqisi2
	orr	r4, r4, r0
	mov	r2, #0
	mov	r0, r8
	bl	set_one_cmd
	ldrb	ip, [r5, #3]	@ zero_extendqisi2
	orr	r4, r4, r0
	movw	r0, #:lower16:.LC123
	ldrb	r3, [r5, #2]	@ zero_extendqisi2
	ldrb	r2, [r5, #1]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC123
	ldrb	r1, [r5]	@ zero_extendqisi2
	str	ip, [sp]
	bl	NAND_Print_DBG
	mov	r0, r4
.L505:
	add	sp, sp, #20
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L511:
	ldrb	r3, [r5]	@ zero_extendqisi2
	cmp	r3, #255
	bne	.L500
	ldrb	r3, [r5, #1]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L512
.L500:
	mov	r2, #1
	str	r10, [sp, #8]
	str	r2, [sp, #4]
	mov	r3, r9
	str	r5, [sp]
	mov	r2, #0
	ldr	r1, .L514
	mov	r0, r8
	bl	set_cmd_with_nand_bus
	orr	r4, r4, r0
	b	.L499
.L512:
	ldrb	r3, [r5, #2]	@ zero_extendqisi2
	cmp	r3, #255
	bne	.L500
	ldrb	r3, [r5, #3]	@ zero_extendqisi2
	cmp	r3, #255
	bne	.L500
	ldrb	r1, [r6, #348]	@ zero_extendqisi2
	mov	r0, r8
	mov	r2, #1
	bl	set_one_cmd
	orr	r0, r4, r0
	b	.L505
.L509:
	mov	fp, #1
	movt	r6, #:upper16:.LANCHOR1
	str	r3, [sp, #8]
	mov	r2, #0
	str	r1, [sp]
	mov	r3, r9
	add	r1, r6, #340
	str	fp, [sp, #4]
	bl	set_cmd_with_nand_bus
	ldrb	r3, [r5]	@ zero_extendqisi2
	mov	r4, r0
	cmp	r3, #0
	bne	.L507
	ldrb	r3, [r5, #1]	@ zero_extendqisi2
	cmp	r3, #0
	bne	.L507
	ldrb	r3, [r5, #2]	@ zero_extendqisi2
	cmp	r3, #0
	bne	.L507
	ldrb	r3, [r5, #3]	@ zero_extendqisi2
	cmp	r3, #0
	bne	.L507
	ldr	r3, [r7, #120]
	cmp	r3, #0
	bne	.L507
.L508:
	mov	r2, fp
	ldrb	r1, [r6, #348]	@ zero_extendqisi2
	mov	r0, r8
	bl	set_one_cmd
	orr	r0, r4, r0
	b	.L505
.L507:
	ldrb	r2, [r7, #108]	@ zero_extendqisi2
	cmp	r2, #17
	bne	.L495
.L510:
	mov	fp, #1
	mov	r3, r9
	str	r10, [sp, #8]
	mov	r2, #0
	str	r5, [sp]
	mov	r0, r8
	str	fp, [sp, #4]
	ldr	r1, .L514+4
	bl	set_cmd_with_nand_bus
	ldrb	r3, [r5]	@ zero_extendqisi2
	orr	r4, r4, r0
	cmp	r3, #0
	bne	.L496
	ldrb	r3, [r5, #1]	@ zero_extendqisi2
	cmp	r3, #0
	bne	.L497
	ldrb	r3, [r5, #2]	@ zero_extendqisi2
	cmp	r3, #0
	bne	.L497
	ldrb	r3, [r5, #3]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L508
.L497:
	ldrb	r3, [r7, #108]	@ zero_extendqisi2
	cmp	r3, #16
	bne	.L499
	b	.L500
.L496:
	cmp	r3, #2
	bne	.L513
	ldrb	r2, [r5, #1]	@ zero_extendqisi2
	cmp	r2, #0
	bne	.L497
	ldrb	r3, [r5, #2]	@ zero_extendqisi2
	cmp	r3, #126
	bne	.L497
	ldrb	r3, [r5, #3]	@ zero_extendqisi2
	cmp	r3, #124
	bne	.L497
	ldrb	r1, [r6, #352]	@ zero_extendqisi2
	mov	r0, r8
	bl	set_one_cmd
	ldrb	r2, [r7, #108]	@ zero_extendqisi2
	orr	r4, r4, r0
	b	.L495
.L513:
	ldrb	r2, [r7, #108]	@ zero_extendqisi2
	b	.L495
.L515:
	.align	2
.L514:
	.word	.LANCHOR1+356
	.word	.LANCHOR1+340
	.size	m6_vender_set_param, .-m6_vender_set_param
	.align	2
	.global	m6_set_readretry
	.syntax unified
	.arm
	.type	m6_set_readretry, %function
m6_set_readretry:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	movw	r5, #:lower16:.LANCHOR0
	movt	r5, #:upper16:.LANCHOR0
	ldrb	r3, [r5, #108]	@ zero_extendqisi2
	sub	r3, r3, #16
	cmp	r3, #2
	bhi	.L524
	mov	r4, r0
	bl	m6_read_retry_clock_save
	mov	r0, r4
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	ldr	r3, [r4, #176]
	cmp	r3, #0
	beq	.L529
.L518:
	ldr	r0, [r4, #248]
	bl	ndfc_is_toogle_interface
	cmp	r0, #0
	bne	.L530
	ldrb	r3, [r5, #108]	@ zero_extendqisi2
	cmp	r3, #17
	beq	.L531
.L520:
	cmp	r3, #18
	beq	.L532
	ldr	r1, .L534
	mov	r3, #4
	ldr	r0, [r4, #176]
	sub	r2, r1, #100
	add	r1, r1, r0, lsl #2
.L521:
	mov	r0, r4
	bl	m6_vender_set_param
	ldr	r3, [r5, #124]
	mov	r6, r0
	cmp	r3, #1
	beq	.L533
.L523:
	mov	r0, r4
	bl	nand_disable_chip
	ldr	r3, [r4, #248]
	ldr	r2, [r5, #112]
	ldr	r1, [r5, #116]
	ldr	r0, [r3, #8]
	bl	NAND_SetClk
	mov	r0, r6
	pop	{r4, r5, r6, pc}
.L530:
	ldr	r0, [r4, #248]
	bl	ndfc_set_legacy_interface
	mov	r3, #1
	str	r3, [r5, #124]
	ldrb	r3, [r5, #108]	@ zero_extendqisi2
	cmp	r3, #17
	bne	.L520
.L531:
	ldr	r2, [r4, #176]
	mov	r3, #5
	ldr	r1, .L534+4
	add	r0, r2, r2, lsl #2
	sub	r2, r1, #12
	add	r1, r1, r0
	b	.L521
.L529:
	mov	r0, r4
	bl	m6_vender_pre_condition
	b	.L518
.L532:
	ldr	r2, [r4, #176]
	mov	r3, #5
	ldr	r1, .L534+8
	add	r0, r2, r2, lsl #2
	sub	r2, r1, #52
	add	r1, r1, r0
	b	.L521
.L533:
	ldr	r0, [r4, #248]
	bl	ndfc_set_toogle_interface
	b	.L523
.L524:
	mvn	r0, #88
	pop	{r4, r5, r6, pc}
.L535:
	.align	2
.L534:
	.word	.LANCHOR1+472
	.word	.LANCHOR1+376
	.word	.LANCHOR1+416
	.size	m6_set_readretry, .-m6_set_readretry
	.align	2
	.global	m6_exit_readretry
	.syntax unified
	.arm
	.type	m6_exit_readretry, %function
m6_exit_readretry:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	movw	r6, #:lower16:.LANCHOR0
	movt	r6, #:upper16:.LANCHOR0
	ldrb	r3, [r6, #108]	@ zero_extendqisi2
	sub	r3, r3, #16
	cmp	r3, #2
	bhi	.L541
	mov	r4, r0
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	ldr	r0, [r4, #248]
	bl	ndfc_is_toogle_interface
	cmp	r0, #0
	bne	.L548
	ldrb	r3, [r6, #108]	@ zero_extendqisi2
	cmp	r3, #17
	beq	.L542
.L550:
	cmp	r3, #18
	beq	.L543
	ldr	r2, .L551
	mov	r3, #0
	add	r1, r2, #124
.L539:
	mov	r0, r4
	bl	m6_vender_set_param
	ldr	r3, [r6, #124]
	mov	r5, r0
	cmp	r3, #1
	beq	.L549
.L540:
	mov	r0, r4
	bl	nand_disable_chip
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L548:
	ldr	r0, [r4, #248]
	bl	ndfc_set_legacy_interface
	mov	r3, #1
	str	r3, [r6, #124]
	ldrb	r3, [r6, #108]	@ zero_extendqisi2
	cmp	r3, #17
	bne	.L550
.L542:
	ldr	r2, .L551+4
	mov	r3, #5
	add	r1, r2, #47
	b	.L539
.L543:
	ldr	r2, .L551+4
	mov	r3, #5
	add	r1, r2, #102
	b	.L539
.L549:
	ldr	r0, [r4, #248]
	bl	ndfc_set_toogle_interface
	b	.L540
.L541:
	mvn	r0, #87
	pop	{r4, r5, r6, pc}
.L552:
	.align	2
.L551:
	.word	.LANCHOR1+372
	.word	.LANCHOR1+364
	.size	m6_exit_readretry, .-m6_exit_readretry
	.align	2
	.global	m6_read_page_end
	.syntax unified
	.arm
	.type	m6_read_page_end, %function
m6_read_page_end:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, r7, r8, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r1, [r0]
	sub	sp, sp, #8
	mov	r7, r0
	ldr	r0, [r3]
	bl	nci_get_from_nsi
	mov	r6, r0
	mov	r0, r7
	bl	m0_read_page_end_not_retry
	cmn	r0, #2
	mov	r5, r0
	beq	.L566
	mov	r0, r5
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L566:
	movw	r0, #:lower16:.LC124
	movw	r8, #:lower16:.LANCHOR0
	movt	r8, #:upper16:.LANCHOR0
	movt	r0, #:upper16:.LC124
	bl	NAND_Print_DBG
	ldrb	r3, [r8, #109]	@ zero_extendqisi2
	mov	r2, #1
	str	r2, [r8, #120]
	cmp	r3, #0
	beq	.L555
	mov	r4, #0
	b	.L557
.L556:
	ldrb	r3, [r8, #109]	@ zero_extendqisi2
	cmp	r3, r4
	ble	.L555
.L557:
	str	r4, [r6, #176]
	mov	r0, r6
	bl	m6_set_readretry
	subs	r5, r0, #0
	add	r4, r4, #1
	bne	.L556
	mov	r0, r7
	bl	m0_read_page_start
	mov	r5, r0
	mov	r0, r7
	bl	m0_read_page_end_not_retry
	orr	r5, r5, r0
	cmp	r5, #0
	cmpne	r5, #10
	bne	.L556
	ldr	r2, [r6, #248]
	movw	r0, #:lower16:.LC125
	ldr	lr, [r7, #8]
	movt	r0, #:upper16:.LC125
	ldr	ip, [r6, #176]
	mov	r5, #10
	ldr	r1, [r2, #8]
	ldr	r3, [r7, #4]
	ldr	r2, [r6, #20]
	str	lr, [sp]
	str	ip, [sp, #4]
	bl	NAND_Print_DBG
.L555:
	mov	r3, #0
	mov	r0, r6
	str	r3, [r6, #176]
	str	r3, [r8, #120]
	bl	m6_exit_readretry
	mov	r0, r5
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
	.size	m6_read_page_end, .-m6_read_page_end
	.align	2
	.global	m6_special_init
	.syntax unified
	.arm
	.type	m6_special_init, %function
m6_special_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r4, [r3, #8]
	cmp	r4, #0
	beq	.L568
	mov	r5, #0
.L569:
	mov	r0, r4
	bl	m6_readretry_init
	ldr	r4, [r4]
	orr	r5, r5, r0
	cmp	r4, #0
	bne	.L569
	cmp	r5, #0
	beq	.L568
	movw	r1, #:lower16:.LANCHOR0
	movw	r0, #:lower16:.LC127
	movt	r1, #:upper16:.LANCHOR0
	movt	r0, #:upper16:.LC127
	ldrb	r3, [r1, #110]	@ zero_extendqisi2
	ldrb	r2, [r1, #109]	@ zero_extendqisi2
	ldrb	r1, [r1, #108]	@ zero_extendqisi2
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L568:
	movw	r1, #:lower16:.LANCHOR0
	movw	ip, #:lower16:function_read_page_end
	movt	r1, #:upper16:.LANCHOR0
	movw	lr, #:lower16:m6_read_page_end
	movw	r0, #:lower16:.LC126
	ldrb	r3, [r1, #110]	@ zero_extendqisi2
	ldrb	r2, [r1, #109]	@ zero_extendqisi2
	movt	ip, #:upper16:function_read_page_end
	movt	lr, #:upper16:m6_read_page_end
	ldrb	r1, [r1, #108]	@ zero_extendqisi2
	mov	r5, #0
	movt	r0, #:upper16:.LC126
	str	lr, [ip]
	bl	NAND_Print_DBG
	mov	r0, r5
	pop	{r4, r5, r6, pc}
	.size	m6_special_init, .-m6_special_init
	.align	2
	.global	m6_special_exit
	.syntax unified
	.arm
	.type	m6_special_exit, %function
m6_special_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r3, [r3, #8]
	cmp	r3, #0
	beq	.L578
.L579:
	ldr	r3, [r3]
	cmp	r3, #0
	bne	.L579
.L578:
	movw	r0, #:lower16:.LC128
	movt	r0, #:upper16:.LC128
	bl	NAND_Print_DBG
	mov	r0, #0
	pop	{r4, pc}
	.size	m6_special_exit, .-m6_special_exit
	.align	2
	.global	m6_is_lsb_page
	.syntax unified
	.arm
	.type	m6_is_lsb_page, %function
m6_is_lsb_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r0, #0
	beq	.L587
	movw	r3, #:lower16:g_nsi
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r3, [r3, #8]
	ldr	r3, [r3, #252]
	ldr	r3, [r3, #16]
	sub	r3, r3, #1
	cmp	r0, r3
	beq	.L588
	and	r0, r0, #1
	bx	lr
.L587:
	mov	r0, #1
	bx	lr
.L588:
	mov	r0, #0
	bx	lr
	.size	m6_is_lsb_page, .-m6_is_lsb_page
	.align	2
	.global	m7_is_lsb_page
	.syntax unified
	.arm
	.type	m7_is_lsb_page, %function
m7_is_lsb_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	b	m6_is_lsb_page
	.size	m7_is_lsb_page, .-m7_is_lsb_page
	.align	2
	.global	m5_is_lsb_page
	.syntax unified
	.arm
	.type	m5_is_lsb_page, %function
m5_is_lsb_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	b	m6_is_lsb_page
	.size	m5_is_lsb_page, .-m5_is_lsb_page
	.align	2
	.global	m1_write_page_FF
	.syntax unified
	.arm
	.type	m1_write_page_FF, %function
m1_write_page_FF:
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	movt	r3, #:upper16:g_nsi
	mov	r9, r0
	sub	sp, sp, #36
	mov	r5, r1
	ldr	r0, [r3]
	ldr	r1, [r9]
	bl	nci_get_from_nsi
	ldr	r4, [r0, #248]
	mov	r7, r0
	ldr	r2, [r0, #20]
	ldr	ip, [r9, #4]
	ldr	r3, [r4, #12]
	add	r1, r4, #28
	str	r1, [sp, #12]
	ldr	lr, [r0, #24]
	cmp	r2, r3
	bcs	.L593
	cmp	ip, lr
	bcs	.L593
	bl	nand_read_chip_status_ready
	mov	r0, r7
	bl	nand_enable_chip
	ldr	r0, [r7, #248]
	bl	ndfc_disable_randomize
	ldr	r0, [r7, #248]
	bl	ndfc_disable_ecc
	add	r0, r4, #28
	bl	ndfc_clean_cmd_seq
	cmp	r5, #8
	beq	.L602
	cmp	r5, #16
	moveq	r3, #640
	movne	r3, #0
	str	r3, [sp, #28]
	moveq	r3, #18
	movne	r3, #0
	str	r3, [sp, #16]
.L596:
	add	r3, r4, #56
	mov	r8, #0
	str	r3, [sp, #20]
	mov	r6, r8
	ldr	r3, [sp, #16]
	sub	r3, r3, #1
	str	r3, [sp, #24]
	b	.L600
.L599:
	ldr	lr, [r9, #16]
	mov	fp, #0
	str	fp, [r4, #28]
	mov	fp, #133
	str	fp, [r4, #44]
	add	r5, lr, r5
	mov	lr, #1024
	str	ip, [r4, #40]
	str	ip, [r4, #48]
	str	ip, [r4, #88]
	str	ip, [r4, #80]
	str	ip, [r4, #84]
	str	r5, [r4, #96]
	str	lr, [r4, #92]
	str	ip, [r4, #76]
	str	r10, [r4, #72]
.L609:
	ldr	ip, [sp, #20]
	str	ip, [sp]
	bl	fill_cmd_addr
	ldr	r1, [sp, #12]
	ldr	r0, [r7, #248]
	bl	ndfc_execute_cmd
.L598:
	ldr	r3, [sp, #16]
	add	r6, r6, #1
	cmp	r3, r6
	beq	.L610
.L600:
	cmp	r6, #0
	mov	r10, #2
	mov	ip, #1
	mov	r3, #0
	mov	r2, r8
	mov	r1, r10
	lsl	r5, r6, #10
	beq	.L611
	ldr	r0, [sp, #24]
	cmp	r0, r6
	mov	r0, r5
	bne	.L599
	ldr	r3, [r9, #16]
	mov	r1, #0
	mov	r2, #1
	mov	ip, #2
	str	r1, [r4, #28]
	mov	r1, #133
	add	r5, r3, r5
	ldr	r3, [sp, #28]
	str	r1, [r4, #44]
	mov	r1, ip
	str	r2, [r4, #40]
	str	r3, [r4, #92]
	mov	r3, #0
	str	r2, [r4, #48]
	str	r2, [r4, #88]
	str	r2, [r4, #80]
	str	r2, [r4, #84]
	str	r2, [r4, #76]
	mov	r2, r8
	str	r5, [r4, #96]
	str	ip, [r4, #72]
	b	.L609
.L610:
	mov	r3, #1
	mov	r2, #16
	mov	r0, #0
	str	r2, [r4, #44]
	str	r0, [r4, #28]
	str	r3, [r4, #40]
	str	r3, [r4, #48]
	str	r3, [r4, #112]
	ldr	r1, [sp, #12]
	ldr	r0, [r7, #248]
	bl	ndfc_execute_cmd
	mov	r0, r7
	bl	nand_read_chip_status_ready
	subs	r4, r0, #0
	bne	.L612
.L601:
	mov	r0, r7
	bl	nand_disable_chip
	mov	r0, r4
	add	sp, sp, #36
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L612:
	movw	r0, #:lower16:.LC130
	movt	r0, #:upper16:.LC130
	bl	NAND_Print
	b	.L601
.L602:
	mov	r3, #832
	str	r3, [sp, #28]
	mov	r3, #9
	str	r3, [sp, #16]
	b	.L596
.L593:
	movw	r0, #:lower16:.LC129
	ldr	r1, [r4, #8]
	movt	r0, #:upper16:.LC129
	stm	sp, {ip, lr}
	bl	NAND_Print
	mvn	r0, #107
	add	sp, sp, #36
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L611:
	ldr	r0, [r9, #16]
	mov	r1, #128
	mov	r2, #1024
	str	r1, [r4, #44]
	str	ip, [r4, #40]
	str	r0, [r4, #96]
	str	ip, [r4, #48]
	str	ip, [r4, #88]
	str	ip, [r4, #80]
	str	ip, [r4, #84]
	str	ip, [r4, #76]
	str	r2, [r4, #92]
	str	r6, [r4, #28]
	ldmib	r9, {r1, r2}
	ldr	r0, [r7, #36]
	bl	get_row_addr
	ldr	r1, [sp, #20]
	mov	r8, r0
	mov	r3, #5
	mov	r0, r5
	str	r3, [r4, #72]
	mov	r2, r8
	str	r1, [sp]
	mov	r3, #3
	mov	r1, r10
	bl	fill_cmd_addr
	ldr	r1, [sp, #12]
	ldr	r0, [r7, #248]
	bl	ndfc_execute_cmd
	b	.L598
	.size	m1_write_page_FF, .-m1_write_page_FF
	.align	2
	.global	m1_vender_get_param
	.syntax unified
	.arm
	.type	m1_vender_get_param, %function
m1_vender_get_param:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	ip, #55
	sub	sp, sp, #16
	mov	r7, r0
	mov	r6, r3
	mov	r5, r1
	mov	r4, r2
	strb	ip, [sp, #15]
	bl	nand_enable_chip
	ldr	r0, [r7, #248]
	bl	ndfc_disable_randomize
	cmp	r6, #0
	beq	.L616
	add	r6, r4, r6
	mov	r8, #1
.L615:
	mov	r3, r5
	mov	r2, r4
	str	r8, [sp]
	add	r1, sp, #15
	add	r4, r4, #1
	mov	r0, r7
	bl	get_data_with_nand_bus_one_cmd
	cmp	r4, r6
	add	r5, r5, #1
	bne	.L615
.L616:
	mov	r0, r7
	bl	nand_disable_chip
	mov	r0, #0
	add	sp, sp, #16
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
	.size	m1_vender_get_param, .-m1_vender_get_param
	.align	2
	.global	m3_getdefaultparam
	.syntax unified
	.arm
	.type	m3_getdefaultparam, %function
m3_getdefaultparam:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	movw	r6, #:lower16:.LANCHOR0
	movt	r6, #:upper16:.LANCHOR0
	mov	r5, r1
	add	r2, r6, #128
	add	r1, r0, #48
	ldrb	r3, [r6, #80]	@ zero_extendqisi2
	mov	r4, r0
	bl	m1_vender_get_param
	ldrb	r3, [r6, #80]	@ zero_extendqisi2
	cmp	r3, #0
	popeq	{r4, r5, r6, pc}
	add	r2, r4, #47
	sub	r1, r5, #1
	mov	r3, #0
.L622:
	ldrb	ip, [r2, #1]!	@ zero_extendqisi2
	add	r3, r3, #1
	strb	ip, [r1, #1]!
	ldrb	ip, [r6, #80]	@ zero_extendqisi2
	cmp	ip, r3
	bgt	.L622
	pop	{r4, r5, r6, pc}
	.size	m3_getdefaultparam, .-m3_getdefaultparam
	.align	2
	.global	m3_vender_get_param
	.syntax unified
	.arm
	.type	m3_vender_get_param, %function
m3_vender_get_param:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	b	m1_vender_get_param
	.size	m3_vender_get_param, .-m3_vender_get_param
	.align	2
	.global	m1_vender_set_param
	.syntax unified
	.arm
	.type	m1_vender_set_param, %function
m1_vender_set_param:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, lr}
	mov	ip, #54
	sub	sp, sp, #28
	mov	r6, r0
	mov	r8, r3
	mov	r9, r1
	mov	r4, r2
	strb	ip, [sp, #23]
	bl	nand_enable_chip
	ldr	r0, [r6, #248]
	bl	ndfc_disable_randomize
	cmp	r8, #0
	beq	.L631
	mov	r5, r9
	add	r8, r4, r8
	mov	r7, #1
.L630:
	mov	r3, r4
	mov	r2, #0
	str	r5, [sp]
	add	r1, sp, #23
	str	r7, [sp, #8]
	mov	r0, r6
	str	r7, [sp, #4]
	add	r4, r4, #1
	bl	set_cmd_with_nand_bus
	mov	r2, #0
	mov	r1, #22
	mov	r0, r6
	add	r5, r5, #1
	bl	set_one_cmd
	cmp	r4, r8
	bne	.L630
.L631:
	mov	r0, r6
	bl	nand_disable_chip
	ldrb	ip, [r9, #3]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC103
	ldrb	r3, [r9, #2]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC103
	ldrb	r2, [r9, #1]	@ zero_extendqisi2
	ldrb	r1, [r9]	@ zero_extendqisi2
	str	ip, [sp]
	bl	NAND_Print_DBG
	mov	r0, #0
	add	sp, sp, #28
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
	.size	m1_vender_set_param, .-m1_vender_set_param
	.align	2
	.global	m3_set_readretry
	.syntax unified
	.arm
	.type	m3_set_readretry, %function
m3_set_readretry:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	movw	r6, #:lower16:.LANCHOR0
	movt	r6, #:upper16:.LANCHOR0
	ldr	r7, [r0, #176]
	sub	sp, sp, #8
	ldrb	r3, [r6, #79]	@ zero_extendqisi2
	cmp	r7, r3
	bhi	.L644
	ldrb	r3, [r6, #80]	@ zero_extendqisi2
	cmp	r3, #0
	addeq	r5, sp, #4
	beq	.L637
	add	ip, r6, #132
	add	r5, sp, #4
	add	r4, r3, #47
	add	ip, ip, r7, lsl #3
	mov	lr, r5
	add	r4, r0, r4
	add	r1, r0, #47
.L638:
	ldrb	r2, [r1, #1]!	@ zero_extendqisi2
	ldrsh	r8, [ip], #2
	cmp	r1, r4
	add	r2, r2, r8
	sxth	r2, r2
	usat	r2, #8, r2
	strb	r2, [lr], #1
	bne	.L638
.L637:
	sub	r2, r7, #2
	sub	r7, r7, #5
	cmp	r2, #4
	mov	r1, r5
	movls	r2, #0
	strbls	r2, [sp, #4]
	cmp	r7, #1
	movls	r2, #0
	strbls	r2, [sp, #5]
	ldr	r2, .L651
	bl	m1_vender_set_param
	mov	r8, r0
	movw	r0, #:lower16:.LC131
	movt	r0, #:upper16:.LC131
	bl	NAND_Print_DBG
	ldrb	r3, [r6, #80]	@ zero_extendqisi2
	cmp	r3, #0
	movwne	r7, #:lower16:.LC133
	movne	r4, #0
	movtne	r7, #:upper16:.LC133
	beq	.L643
.L642:
	ldrb	r1, [r5], #1	@ zero_extendqisi2
	mov	r0, r7
	bl	NAND_Print_DBG
	ldrb	r3, [r6, #80]	@ zero_extendqisi2
	add	r4, r4, #1
	cmp	r3, r4
	bhi	.L642
.L643:
	movw	r0, #:lower16:.LC132
	movt	r0, #:upper16:.LC132
	bl	NAND_Print_DBG
	mov	r0, r8
.L636:
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L644:
	mvn	r0, #52
	b	.L636
.L652:
	.align	2
.L651:
	.word	.LANCHOR0+128
	.size	m3_set_readretry, .-m3_set_readretry
	.align	2
	.global	m3_read_page_end
	.syntax unified
	.arm
	.type	m3_read_page_end, %function
m3_read_page_end:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, r7, r8, r9, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r1, [r0]
	sub	sp, sp, #12
	mov	r6, r0
	ldr	r0, [r3]
	bl	nci_get_from_nsi
	mov	r5, r0
	mov	r0, r6
	bl	m0_read_page_end_not_retry
	cmn	r0, #2
	mov	r4, r0
	beq	.L666
	mov	r0, r4
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
.L666:
	movw	r0, #:lower16:.LC134
	movw	r8, #:lower16:.LANCHOR0
	movt	r0, #:upper16:.LC134
	movt	r8, #:upper16:.LANCHOR0
	bl	NAND_Print_DBG
	ldrb	r2, [r8, #79]	@ zero_extendqisi2
	cmp	r2, #0
	beq	.L655
	mov	r7, #0
	mov	r9, r7
	b	.L660
.L658:
	bl	m0_read_page_start
	mov	r4, r0
	mov	r0, r6
	bl	m0_read_page_end_not_retry
	orr	r4, r4, r0
	cmp	r4, #0
	cmpne	r4, #10
	beq	.L667
	ldrb	r2, [r8, #79]	@ zero_extendqisi2
	cmp	r2, r7
	ble	.L655
.L660:
	ldr	r3, [r5, #176]
	mov	r0, r5
	add	r7, r7, #1
	add	r3, r3, #1
	cmp	r3, r2
	movhi	r3, r9
	str	r3, [r5, #176]
	bl	m3_set_readretry
	subs	r4, r0, #0
	mov	r0, r6
	beq	.L658
	mov	r3, #0
	mov	r0, r5
	str	r3, [r5, #176]
	bl	m3_set_readretry
.L655:
	mov	r3, #0
	mov	r0, r5
	str	r3, [r5, #176]
	bl	m3_set_readretry
	mov	r0, r4
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
.L667:
	ldr	r2, [r5, #248]
	movw	r0, #:lower16:.LC135
	ldr	lr, [r6, #8]
	movt	r0, #:upper16:.LC135
	ldr	ip, [r5, #176]
	mov	r4, #10
	ldr	r1, [r2, #8]
	ldr	r3, [r6, #4]
	ldr	r2, [r5, #20]
	str	lr, [sp]
	str	ip, [sp, #4]
	bl	NAND_Print_DBG
	b	.L655
	.size	m3_read_page_end, .-m3_read_page_end
	.align	2
	.global	m3_setdefaultparam
	.syntax unified
	.arm
	.type	m3_setdefaultparam, %function
m3_setdefaultparam:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	movw	r6, #:lower16:.LANCHOR0
	movt	r6, #:upper16:.LANCHOR0
	sub	sp, sp, #8
	ldrb	r3, [r6, #80]	@ zero_extendqisi2
	cmp	r3, #0
	addne	lr, r3, #47
	addne	r2, r0, #47
	addne	lr, r0, lr
	subne	r1, sp, #1
	beq	.L673
.L672:
	ldrb	ip, [r2, #1]!	@ zero_extendqisi2
	cmp	r2, lr
	strb	ip, [r1, #1]!
	bne	.L672
.L673:
	ldr	r2, .L679
	mov	r1, sp
	bl	m1_vender_set_param
	mov	r8, r0
	movw	r0, #:lower16:.LC136
	movt	r0, #:upper16:.LC136
	bl	NAND_Print_DBG
	ldrb	r3, [r6, #80]	@ zero_extendqisi2
	cmp	r3, #0
	movwne	r7, #:lower16:.LC137
	subne	r5, sp, #1
	movtne	r7, #:upper16:.LC137
	movne	r4, #0
	beq	.L671
.L674:
	ldrb	r1, [r5, #1]!	@ zero_extendqisi2
	mov	r0, r7
	bl	NAND_Print_DBG
	ldrb	r3, [r6, #80]	@ zero_extendqisi2
	add	r4, r4, #1
	cmp	r3, r4
	bgt	.L674
.L671:
	movw	r0, #:lower16:.LC132
	movt	r0, #:upper16:.LC132
	bl	NAND_Print_DBG
	mov	r0, r8
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L680:
	.align	2
.L679:
	.word	.LANCHOR0+128
	.size	m3_setdefaultparam, .-m3_setdefaultparam
	.align	2
	.global	m3_get_default_param
	.syntax unified
	.arm
	.type	m3_get_default_param, %function
m3_get_default_param:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, lr}
	sub	sp, sp, #28
	mov	r4, r0
	add	r1, sp, #20
	bl	m3_getdefaultparam
	ldr	r1, [r4, #248]
	movw	r0, #:lower16:.LC138
	ldrb	lr, [sp, #22]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC138
	ldrb	ip, [sp, #21]	@ zero_extendqisi2
	ldrb	r5, [sp, #23]	@ zero_extendqisi2
	ldr	r2, [r4, #20]
	ldr	r1, [r1, #8]
	ldrb	r3, [sp, #20]	@ zero_extendqisi2
	stm	sp, {ip, lr}
	str	r5, [sp, #8]
	bl	NAND_Print_DBG
	mov	r0, r4
	bl	m3_setdefaultparam
	mov	r0, #0
	add	sp, sp, #28
	@ sp needed
	pop	{r4, r5, pc}
	.size	m3_get_default_param, .-m3_get_default_param
	.align	2
	.global	m3_readretry_init
	.syntax unified
	.arm
	.type	m3_readretry_init, %function
m3_readretry_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #252]
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	movw	r6, #:lower16:.LANCHOR0
	movt	r6, #:upper16:.LANCHOR0
	ldr	r4, [r3, #40]
	ubfx	r1, r4, #16, #8
	strb	r4, [r6, #80]
	lsr	r3, r4, #8
	cmp	r1, #0
	strb	r1, [r6, #78]
	strb	r3, [r6, #79]
	bne	.L684
	uxtb	r3, r3
	uxtb	r5, r4
	ldr	r4, .L701
	add	r7, r3, #1
	mvn	r3, #82
	strb	r3, [r6, #129]
	mvn	r3, #81
	strb	r3, [r6, #130]
	mvn	r2, #83
	mvn	r3, #80
	lsl	r8, r5, #1
	strb	r2, [r6, #128]
	mov	lr, r1
	strb	r3, [r6, #131]
	add	r6, r6, #132
.L685:
	cmp	r5, #0
	addne	ip, r4, r8
	movne	r2, r6
	movne	r3, r4
	beq	.L689
.L686:
	ldrsh	r1, [r3], #2
	cmp	r3, ip
	strh	r1, [r2], #2	@ movhi
	bne	.L686
.L689:
	add	lr, lr, #1
	add	r4, r4, #8
	cmp	lr, r7
	add	r6, r6, #8
	bne	.L685
.L687:
	mov	ip, #0
	add	r3, r0, #47
	mov	r1, ip
	add	r2, r0, #175
	str	ip, [r0, #176]
.L696:
	strb	r1, [r3, #1]!
	cmp	r3, r2
	bne	.L696
	bl	m3_get_default_param
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L684:
	cmp	r1, #1
	bne	.L690
	uxtb	r4, r4
	ldr	lr, .L701+4
	uxtb	r3, r3
	mvn	r2, #88
	lsl	r9, r4, #1
	add	r7, r6, #132
	mov	r5, #0
	strb	r2, [r6, #128]
	mvn	r1, #82
	add	r8, r3, #1
	mvn	r2, #81
	mvn	r3, #80
	strb	r1, [r6, #129]
	strb	r2, [r6, #130]
	strb	r3, [r6, #131]
.L691:
	cmp	r4, #0
	addne	ip, lr, r9
	movne	r2, r7
	movne	r3, lr
	beq	.L694
.L692:
	ldrsh	r1, [r3], #2
	cmp	r3, ip
	strh	r1, [r2], #2	@ movhi
	bne	.L692
.L694:
	add	r5, r5, #1
	add	lr, lr, #8
	cmp	r5, r8
	add	r7, r7, #8
	bne	.L691
	b	.L687
.L690:
	movw	r0, #:lower16:.LC139
	movt	r0, #:upper16:.LC139
	bl	NAND_Print
	mvn	r0, #53
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L702:
	.align	2
.L701:
	.word	.LANCHOR2+536
	.word	.LANCHOR2+592
	.size	m3_readretry_init, .-m3_readretry_init
	.align	2
	.global	m3_special_init
	.syntax unified
	.arm
	.type	m3_special_init, %function
m3_special_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r4, [r3, #8]
	cmp	r4, #0
	beq	.L704
	mov	r5, #0
.L705:
	mov	r0, r4
	bl	m3_readretry_init
	ldr	r4, [r4]
	orr	r5, r5, r0
	cmp	r4, #0
	bne	.L705
	cmp	r5, #0
	beq	.L704
	movw	r1, #:lower16:.LANCHOR0
	movw	r0, #:lower16:.LC141
	movt	r1, #:upper16:.LANCHOR0
	movt	r0, #:upper16:.LC141
	ldrb	r3, [r1, #80]	@ zero_extendqisi2
	ldrb	r2, [r1, #79]	@ zero_extendqisi2
	ldrb	r1, [r1, #78]	@ zero_extendqisi2
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L704:
	movw	r1, #:lower16:.LANCHOR0
	movw	ip, #:lower16:function_read_page_end
	movt	r1, #:upper16:.LANCHOR0
	movw	lr, #:lower16:m3_read_page_end
	movw	r0, #:lower16:.LC140
	ldrb	r3, [r1, #80]	@ zero_extendqisi2
	ldrb	r2, [r1, #79]	@ zero_extendqisi2
	movt	ip, #:upper16:function_read_page_end
	movt	lr, #:upper16:m3_read_page_end
	ldrb	r1, [r1, #78]	@ zero_extendqisi2
	mov	r5, #0
	movt	r0, #:upper16:.LC140
	str	lr, [ip]
	bl	NAND_Print_DBG
	mov	r0, r5
	pop	{r4, r5, r6, pc}
	.size	m3_special_init, .-m3_special_init
	.align	2
	.global	m3_readretry_exit
	.syntax unified
	.arm
	.type	m3_readretry_exit, %function
m3_readretry_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r4, r0
	movw	r0, #:lower16:.LC142
	movt	r0, #:upper16:.LC142
	bl	NAND_Print_DBG
	mov	r0, r4
	bl	m3_setdefaultparam
	mov	r0, #0
	str	r0, [r4, #176]
	pop	{r4, pc}
	.size	m3_readretry_exit, .-m3_readretry_exit
	.align	2
	.global	m3_special_exit
	.syntax unified
	.arm
	.type	m3_special_exit, %function
m3_special_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r4, [r3, #8]
	cmp	r4, #0
	beq	.L718
	movw	r5, #:lower16:.LC142
	mov	r6, #0
	movt	r5, #:upper16:.LC142
.L717:
	mov	r0, r5
	bl	NAND_Print_DBG
	mov	r0, r4
	bl	m3_setdefaultparam
	str	r6, [r4, #176]
	ldr	r4, [r4]
	cmp	r4, #0
	bne	.L717
.L718:
	movw	r0, #:lower16:.LC143
	movt	r0, #:upper16:.LC143
	bl	NAND_Print_DBG
	mov	r0, #0
	pop	{r4, r5, r6, pc}
	.size	m3_special_exit, .-m3_special_exit
	.align	2
	.global	m3_lsb_disable
	.syntax unified
	.arm
	.type	m3_lsb_disable, %function
m3_lsb_disable:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r1, #:lower16:.LANCHOR0
	push	{r4, lr}
	movt	r1, #:upper16:.LANCHOR0
	add	r2, r1, #84
	add	r1, r1, #188
	ldrb	r3, [r1, #-107]	@ zero_extendqisi2
	bl	m1_vender_set_param
	mov	r0, #0
	pop	{r4, pc}
	.size	m3_lsb_disable, .-m3_lsb_disable
	.align	2
	.global	m3_lsb_enable
	.syntax unified
	.arm
	.type	m3_lsb_enable, %function
m3_lsb_enable:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	movw	r5, #:lower16:.LANCHOR0
	movt	r5, #:upper16:.LANCHOR0
	sub	sp, sp, #8
	add	r4, r5, #188
	add	r2, r5, #84
	ldrb	r3, [r5, #81]	@ zero_extendqisi2
	mov	r1, r4
	mov	r6, r0
	bl	m1_vender_get_param
	ldrb	r3, [r5, #81]	@ zero_extendqisi2
	cmp	r3, #0
	moveq	r1, sp
	beq	.L725
	add	ip, r5, #92
	mov	r0, r4
	mov	r1, sp
	add	r5, r4, r3
	mov	lr, sp
.L726:
	ldrb	r2, [r0], #1	@ zero_extendqisi2
	ldrb	r4, [ip], #1	@ zero_extendqisi2
	cmp	r0, r5
	add	r2, r2, r4
	strb	r2, [lr], #1
	bne	.L726
.L725:
	mov	r0, r6
	ldr	r2, .L732
	bl	m1_vender_set_param
	mov	r0, #0
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, pc}
.L733:
	.align	2
.L732:
	.word	.LANCHOR0+84
	.size	m3_lsb_enable, .-m3_lsb_enable
	.align	2
	.global	m3_vender_set_param
	.syntax unified
	.arm
	.type	m3_vender_set_param, %function
m3_vender_set_param:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	b	m1_vender_set_param
	.size	m3_vender_set_param, .-m3_vender_set_param
	.align	2
	.global	m1_major_check_byte
	.syntax unified
	.arm
	.type	m1_major_check_byte, %function
m1_major_check_byte:
	@ args = 8, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	sub	sp, sp, #20
	ldr	r6, [sp, #60]
	mov	r7, r2
	str	r0, [sp, #12]
	cmp	r2, r6, lsr #1
	bcc	.L761
	mov	r4, #0
	mov	r8, r3
	movw	r2, #:lower16:.LC146
	movw	r3, #:lower16:.LC145
	mov	r10, r1
	mov	r9, r4
	mov	r5, r4
	mov	fp, #1
	movt	r3, #:upper16:.LC145
	movt	r2, #:upper16:.LC146
	str	r3, [sp, #4]
	str	r2, [sp, #8]
.L749:
	cmp	r6, #0
	beq	.L754
	mov	r3, #0
	lsl	ip, fp, r5
	mov	r2, r3
	mov	r1, r8
	add	lr, r8, r6
.L740:
	ldrb	r0, [r1], #1	@ zero_extendqisi2
	tst	r0, ip
	addne	r2, r2, #1
	addeq	r3, r3, #1
	cmp	lr, r1
	bne	.L740
	cmp	r2, r7
	bls	.L762
.L747:
	orr	r4, r4, ip
	uxtb	r4, r4
.L742:
	add	r9, r9, #1
.L748:
	add	r5, r5, #1
	cmp	r5, #8
	bne	.L749
	cmp	r9, #8
	ldreq	r3, [sp, #12]
	moveq	r0, #0
	ldrne	r2, [sp, #12]
	mvnne	r0, #79
	mvnne	r3, #0
	strbeq	r4, [r3]
	strbne	r3, [r2]
.L737:
	add	sp, sp, #20
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L762:
	cmp	r3, r7
	bhi	.L742
.L751:
	cmp	r10, #1
	bne	.L743
	cmp	r6, #0
	beq	.L753
	ldr	r1, [sp, #56]
	mov	r3, #0
	lsl	ip, r10, r5
	mov	r2, r3
	add	lr, r1, r6
.L746:
	ldrb	r0, [r1], #1	@ zero_extendqisi2
	tst	r0, ip
	addne	r3, r3, #1
	addeq	r2, r2, #1
	cmp	lr, r1
	bne	.L746
	cmp	r2, r7
	bhi	.L747
	cmp	r3, r7
	bhi	.L742
.L743:
	mov	r1, r5
	ldr	r0, [sp, #4]
	bl	NAND_Print
	mov	r1, r5
	ldr	r0, [sp, #8]
	bl	NAND_Print
	b	.L748
.L754:
	mov	r2, r6
	mov	r3, r6
	b	.L751
.L753:
	mov	r3, r6
	mov	r2, r6
	b	.L743
.L761:
	movw	r0, #:lower16:.LC144
	mov	r2, r6
	mov	r1, r7
	movt	r0, #:upper16:.LC144
	bl	NAND_Print
	ldr	r2, [sp, #12]
	mvn	r3, #0
	mvn	r0, #106
	strb	r3, [r2]
	b	.L737
	.size	m1_major_check_byte, .-m1_major_check_byte
	.align	2
	.global	m1_get_read_retry_cfg
	.syntax unified
	.arm
	.type	m1_get_read_retry_cfg, %function
m1_get_read_retry_cfg:
	@ args = 0, pretend = 0, frame = 64
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	sub	sp, sp, #76
	mov	r6, r0
	mov	fp, r1
	mov	r5, r2
	mov	r1, #0
	mov	r2, #32
	add	r0, sp, #8
	add	r8, sp, #40
	mov	r7, r3
	bl	memset
	mov	r2, #32
	mov	r0, r8
	mov	r1, #0
	add	r4, r7, #7
	bl	memset
	sub	r3, r7, #1
	add	r2, sp, #7
.L764:
	ldrb	r1, [r3, #1]!	@ zero_extendqisi2
	cmp	r3, r4
	strb	r1, [r2, #1]!
	bne	.L764
	mov	r3, #8
	str	r8, [sp]
	str	r3, [sp, #4]
	mov	r2, #4
	add	r3, sp, r3
	mov	r1, #0
	mov	r0, r6
	bl	m1_major_check_byte
	subs	r10, r0, #0
	blt	.L784
	movw	r0, #:lower16:.LC148
	ldrb	r1, [r6]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC148
	bl	NAND_Print_DBG
	add	r1, r7, #15
	add	r3, sp, #7
.L767:
	ldrb	r2, [r4, #1]!	@ zero_extendqisi2
	cmp	r1, r4
	strb	r2, [r3, #1]!
	bne	.L767
	mov	r3, #8
	str	r8, [sp]
	str	r3, [sp, #4]
	mov	r2, #4
	add	r3, sp, r3
	mov	r1, #0
	mov	r0, fp
	bl	m1_major_check_byte
	subs	r10, r0, #0
	blt	.L785
	movw	r0, #:lower16:.LC150
	ldrb	r1, [fp]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC150
	bl	NAND_Print_DBG
	ldrb	r1, [r6]	@ zero_extendqisi2
	ldrb	r2, [fp]	@ zero_extendqisi2
	cmp	r1, #8
	bne	.L770
	cmp	r2, #4
	moveq	r9, r1
	moveq	r6, #0
	addeq	r4, sp, #15
	bne	.L770
.L775:
	add	r3, r7, r6
	add	r1, sp, #39
	add	r2, sp, #7
.L772:
	ldrb	ip, [r3, #16]	@ zero_extendqisi2
	add	r3, r3, #64
	ldrb	r0, [r3, #-16]	@ zero_extendqisi2
	strb	ip, [r2, #1]!
	cmp	r4, r2
	strb	r0, [r1, #1]!
	bne	.L772
	str	r9, [sp, #4]
	add	r3, sp, #8
	str	r8, [sp]
	mov	r2, #4
	mov	r1, #1
	add	r0, r5, r6
	bl	m1_major_check_byte
	subs	r10, r0, #0
	blt	.L786
	add	r6, r6, #1
	cmp	r6, #32
	bne	.L775
	mov	r10, #0
.L774:
	movw	r7, #:lower16:.LC153
	movw	r6, #:lower16:.LC132
	sub	r5, r5, #1
	movt	r7, #:upper16:.LC153
	movt	r6, #:upper16:.LC132
	mov	r4, #0
.L776:
	add	r4, r4, #1
	ldrb	r1, [r5, #1]!	@ zero_extendqisi2
	mov	r0, r7
	bl	NAND_Print_DBG
	cmp	r4, #32
	mov	r0, r6
	beq	.L787
	tst	r4, #7
	bne	.L776
	bl	NAND_Print_DBG
	b	.L776
.L787:
	movw	r0, #:lower16:.LC132
	movt	r0, #:upper16:.LC132
	bl	NAND_Print_DBG
	mov	r0, r10
.L766:
	add	sp, sp, #76
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L786:
	movw	r0, #:lower16:.LC152
	mov	r1, r6
	movt	r0, #:upper16:.LC152
	bl	NAND_Print
	b	.L774
.L784:
	movw	r0, #:lower16:.LC147
	movt	r0, #:upper16:.LC147
	bl	NAND_Print
	mov	r0, r10
	add	sp, sp, #76
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L785:
	movw	r0, #:lower16:.LC149
	movt	r0, #:upper16:.LC149
	bl	NAND_Print
	mov	r0, r10
	add	sp, sp, #76
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L770:
	movw	r0, #:lower16:.LC151
	movt	r0, #:upper16:.LC151
	bl	NAND_Print
	mvn	r0, #80
	b	.L766
	.size	m1_get_read_retry_cfg, .-m1_get_read_retry_cfg
	.align	2
	.global	m1_read_otp_info_hynix
	.syntax unified
	.arm
	.type	m1_read_otp_info_hynix, %function
m1_read_otp_info_hynix:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r4, r0
	sub	sp, sp, #32
	mov	r6, r1
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	ldr	r2, [r4, #20]
	movw	r0, #:lower16:.LC154
	ldr	r3, [r4, #248]
	movt	r0, #:upper16:.LC154
	add	r3, r3, r2, lsl #2
	ldr	r2, [r3, #1044]
	ldr	r1, [r3, #1012]
	bl	NAND_Print_DBG
	cmp	r6, #0
	beq	.L797
	movw	r5, #:lower16:.LANCHOR0
	mov	r2, #1
	movt	r5, #:upper16:.LANCHOR0
	mov	r1, #255
	mov	r0, r4
	mov	r7, #1
	bl	set_one_cmd
	ldrb	r3, [r5, #193]	@ zero_extendqisi2
	mov	r1, #54
	mov	r2, #56
	add	r8, sp, #20
	mov	ip, #82
	strb	r2, [sp, #24]
	cmp	r3, #6
	strb	r1, [sp, #16]
	moveq	r3, #14
	add	r1, sp, #16
	strbeq	r3, [sp, #24]
	mov	r2, #0
	add	r3, sp, #24
	str	r7, [sp, #8]
	str	r7, [sp, #4]
	mov	r0, r4
	str	r8, [sp]
	strb	ip, [sp, #20]
	bl	set_cmd_with_nand_bus
	mov	r2, #0
	mov	r1, #22
	mov	r0, r4
	bl	set_one_cmd
	mov	r2, #0
	mov	r1, #23
	mov	r0, r4
	bl	set_one_cmd
	mov	r2, #0
	mov	r1, #4
	mov	r0, r4
	bl	set_one_cmd
	mov	r2, #0
	mov	r1, #25
	mov	r0, r4
	bl	set_one_cmd
	mov	r2, #0
	mov	r0, r4
	mov	r1, r2
	bl	set_one_cmd
	mov	r1, #0
	mov	r0, r4
	bl	set_one_addr
	mov	r1, #0
	mov	r0, r4
	bl	set_one_addr
	mov	r1, #0
	mov	r0, r4
	bl	set_one_addr
	mov	r1, #2
	mov	r0, r4
	bl	set_one_addr
	mov	r1, #0
	mov	r0, r4
	bl	set_one_addr
	mov	r3, #528
	mov	ip, #48
	add	r1, sp, #16
	str	r3, [sp]
	mov	r2, #0
	mov	r3, r6
	mov	r0, r4
	strb	ip, [sp, #16]
	bl	get_data_with_nand_bus_one_cmd
	mov	r2, r7
	mov	r1, #255
	mov	r0, r4
	mov	r7, #1
	bl	set_one_cmd
	ldrb	r3, [r5, #193]	@ zero_extendqisi2
	mov	r1, #54
	mov	r5, #0
	mov	r2, #56
	strb	r2, [sp, #24]
	mov	r0, r4
	cmp	r3, #6
	strb	r1, [sp, #16]
	moveq	r3, #14
	add	r1, sp, #16
	strbeq	r3, [sp, #24]
	mov	r2, r5
	add	r3, sp, #24
	str	r8, [sp]
	str	r7, [sp, #8]
	str	r7, [sp, #4]
	strb	r5, [sp, #20]
	bl	set_cmd_with_nand_bus
	mov	r2, r5
	mov	r1, #22
	mov	r0, r4
	bl	set_one_cmd
	mov	r2, r5
	mov	r1, r5
	mov	r0, r4
	bl	set_one_cmd
	mov	r1, r5
	mov	r0, r4
	bl	set_one_addr
	mov	r1, r5
	mov	r0, r4
	bl	set_one_addr
	mov	r1, r5
	mov	r0, r4
	bl	set_one_addr
	mov	r1, r5
	mov	r0, r4
	bl	set_one_addr
	mov	r1, r5
	mov	r0, r4
	bl	set_one_addr
	mov	r2, r7
	mov	r1, #48
	mov	r0, r4
	bl	set_one_cmd
	ldrb	r1, [r6]	@ zero_extendqisi2
	ldrb	r2, [r6, #1]	@ zero_extendqisi2
	cmp	r1, #8
	bne	.L794
	cmp	r2, #8
	bne	.L794
	mov	r0, r4
	bl	nand_disable_chip
	mov	r0, r5
.L790:
	add	sp, sp, #32
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L794:
	movw	r0, #:lower16:.LC156
	movt	r0, #:upper16:.LC156
	bl	NAND_Print
	mov	r0, r4
	bl	nand_disable_chip
	mvn	r0, #95
	add	sp, sp, #32
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L797:
	mov	r0, r4
	bl	nand_disable_chip
	movw	r0, #:lower16:.LC155
	movt	r0, #:upper16:.LC155
	bl	NAND_Print
	mvn	r0, #78
	b	.L790
	.size	m1_read_otp_info_hynix, .-m1_read_otp_info_hynix
	.align	2
	.global	m1_get_rr_value_otp_hynix
	.syntax unified
	.arm
	.type	m1_get_rr_value_otp_hynix, %function
m1_get_rr_value_otp_hynix:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r6, r0
	sub	sp, sp, #8
	mov	r0, #528
	bl	nand_get_temp_buf
	subs	r5, r0, #0
	beq	.L804
	mov	r1, r5
	mov	r0, r6
	bl	m1_read_otp_info_hynix
	cmp	r0, #0
	blt	.L805
.L801:
	mov	r3, r5
	add	r2, r6, #48
	add	r1, sp, #7
	add	r0, sp, #6
	bl	m1_get_read_retry_cfg
	subs	r4, r0, #0
	blt	.L806
.L802:
	mov	r0, r5
	mov	r1, #528
	bl	nand_free_temp_buf
	mov	r0, r4
.L800:
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, pc}
.L805:
	ldr	r3, [r6, #248]
	movw	r0, #:lower16:.LC158
	ldr	r2, [r6, #20]
	movt	r0, #:upper16:.LC158
	ldr	r1, [r3, #8]
	bl	NAND_Print
	b	.L801
.L806:
	ldr	r3, [r6, #248]
	movw	r0, #:lower16:.LC159
	ldr	r2, [r6, #20]
	movt	r0, #:upper16:.LC159
	ldr	r1, [r3, #8]
	bl	NAND_Print
	b	.L802
.L804:
	movw	r0, #:lower16:.LC157
	movt	r0, #:upper16:.LC157
	bl	NAND_Print
	mvn	r0, #77
	b	.L800
	.size	m1_get_rr_value_otp_hynix, .-m1_get_rr_value_otp_hynix
	.align	2
	.global	m1_saveotpvalue
	.syntax unified
	.arm
	.type	m1_saveotpvalue, %function
m1_saveotpvalue:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	r3, r1, #1
	add	r0, r0, #47
	add	r1, r1, #31
.L808:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r3, r1
	strb	r2, [r0, #1]!
	bne	.L808
	bx	lr
	.size	m1_saveotpvalue, .-m1_saveotpvalue
	.align	2
	.global	m1_getotpparam
	.syntax unified
	.arm
	.type	m1_getotpparam, %function
m1_getotpparam:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	movw	r7, #:lower16:.LC160
	mov	r6, r0
	mov	r4, r1
	movt	r7, #:upper16:.LC160
	mov	r5, #0
.L811:
	mov	r1, r5
	mov	r0, r7
	bl	NAND_Print_DBG
	mov	r0, r6
	add	r5, r5, #1
	bl	m1_get_rr_value_otp_hynix
	cmp	r0, #0
	bne	.L811
	add	lr, r6, #48
	add	r1, r4, #4
	add	r6, r6, #80
.L814:
	sub	r3, r1, #4
	mov	r2, lr
.L813:
	ldrb	ip, [r2], #1	@ zero_extendqisi2
	strb	ip, [r3], #1
	cmp	r1, r3
	bne	.L813
	add	lr, lr, #4
	add	r1, r1, #4
	cmp	lr, r6
	bne	.L814
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, pc}
	.size	m1_getotpparam, .-m1_getotpparam
	.align	2
	.global	m1_setdefaultparam
	.syntax unified
	.arm
	.type	m1_setdefaultparam, %function
m1_setdefaultparam:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	movw	r6, #:lower16:.LANCHOR0
	movt	r6, #:upper16:.LANCHOR0
	sub	sp, sp, #8
	ldrb	r3, [r6, #194]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L825
	add	lr, r3, #47
	add	r2, r0, #47
	add	lr, r0, lr
	sub	r1, sp, #1
.L826:
	ldrb	ip, [r2, #1]!	@ zero_extendqisi2
	cmp	r2, lr
	strb	ip, [r1, #1]!
	bne	.L826
.L825:
	ldr	r2, .L837
	mov	r1, sp
	bl	m1_vender_set_param
	mov	r8, r0
	movw	r0, #:lower16:.LC136
	movt	r0, #:upper16:.LC136
	bl	NAND_Print_DBG
	ldrb	r3, [r6, #194]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L829
	movw	r7, #:lower16:.LC137
	sub	r5, sp, #1
	movt	r7, #:upper16:.LC137
	mov	r4, #0
.L828:
	ldrb	r1, [r5, #1]!	@ zero_extendqisi2
	mov	r0, r7
	bl	NAND_Print_DBG
	ldrb	r3, [r6, #194]	@ zero_extendqisi2
	add	r4, r4, #1
	cmp	r3, r4
	bhi	.L828
.L829:
	movw	r0, #:lower16:.LC132
	movt	r0, #:upper16:.LC132
	bl	NAND_Print_DBG
	mov	r0, r8
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L838:
	.align	2
.L837:
	.word	.LANCHOR0+196
	.size	m1_setdefaultparam, .-m1_setdefaultparam
	.align	2
	.global	m1_init_readretry_param
	.syntax unified
	.arm
	.type	m1_init_readretry_param, %function
m1_init_readretry_param:
	@ args = 0, pretend = 0, frame = 192
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #252]
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	mov	r4, r0
	sub	sp, sp, #212
	ldr	r0, [r3, #12]
	lsl	r0, r0, #9
	bl	nand_get_temp_buf
	subs	r8, r0, #0
	beq	.L862
	movw	r0, #18048
	bl	nand_get_temp_buf
	subs	r3, r0, #0
	str	r3, [sp, #32]
	beq	.L874
	ldr	r3, [sp, #32]
	mvn	r1, #0
	add	r2, r3, #17920
	sub	r3, r3, #1
	add	r2, r2, #127
.L842:
	strb	r1, [r3, #1]!
	cmp	r2, r3
	bne	.L842
	movw	r3, #:lower16:.LC162
	movw	r2, #:lower16:.LC166
	movt	r3, #:upper16:.LC162
	add	r7, sp, #144
	str	r3, [sp, #40]
	sub	r3, r8, #1
	str	r3, [sp, #36]
	add	r3, r8, #31
	movt	r2, #:upper16:.LC166
	str	r3, [sp, #28]
	add	r3, r8, #63
	str	r2, [sp, #24]
	str	r3, [sp, #44]
.L859:
	movw	fp, #:lower16:.LC163
	mov	r5, #8
	movt	fp, #:upper16:.LC163
	mov	r9, #0
	str	fp, [sp, #20]
	ldr	fp, [sp, #40]
.L861:
	ldr	r1, [r4, #16]
	mov	r3, #16
	ldr	r2, [r4, #28]
	add	r0, sp, #52
	str	r3, [sp, #76]
	str	r1, [sp, #52]
	str	r2, [sp, #64]
	str	r5, [sp, #56]
	str	r9, [sp, #60]
	str	r8, [sp, #68]
	str	r7, [sp, #72]
	bl	m0_read_page
	ldr	r1, [r4, #248]
	mov	r6, r0
	ldrb	lr, [sp, #146]	@ zero_extendqisi2
	mov	r0, fp
	ldrb	r10, [sp, #147]	@ zero_extendqisi2
	ldr	r1, [r1, #8]
	ldrb	ip, [sp, #145]	@ zero_extendqisi2
	str	lr, [sp, #8]
	ldrb	lr, [sp, #144]	@ zero_extendqisi2
	ldr	r3, [sp, #56]
	ldr	r2, [sp, #52]
	str	r10, [sp, #12]
	str	ip, [sp, #4]
	str	lr, [sp]
	bl	NAND_Print_DBG
	cmp	r6, #0
	blt	.L843
	ldrb	r3, [sp, #144]	@ zero_extendqisi2
	cmp	r3, #0
	bne	.L843
	ldrb	r3, [sp, #145]	@ zero_extendqisi2
	cmp	r3, #79
	beq	.L875
.L843:
	add	r5, r5, #1
	cmp	r5, #12
	bne	.L861
	ldr	r3, [r4, #248]
	movw	r0, #:lower16:.LC170
	movt	r0, #:upper16:.LC170
	ldr	r1, [r3, #8]
	bl	NAND_Print_DBG
	add	r1, sp, #80
	mov	r0, r4
	bl	m1_getotpparam
	mov	r0, r4
	bl	m1_setdefaultparam
	ldr	r0, [sp, #36]
	add	r3, sp, #79
	ldr	r1, [sp, #28]
	add	lr, sp, #111
.L851:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	lr, r3
	mvn	ip, r2
	strb	r2, [r0, #1]!
	strb	ip, [r1, #1]!
	bne	.L851
	movw	fp, #:lower16:.LC165
	mov	r3, #0
	movt	fp, #:upper16:.LC165
	mov	r6, #8
	str	fp, [sp, #20]
	mov	r10, r3
	ldr	fp, [sp, #32]
	mov	r2, #66
	strb	r3, [sp, #144]
	mov	r3, #79
	strb	r3, [sp, #145]
	strb	r3, [sp, #146]
	strb	r2, [sp, #147]
.L858:
	ldr	r3, [r4, #16]
	add	r0, sp, #52
	str	r6, [sp, #56]
	str	r3, [sp, #52]
	bl	m0_erase_block
	cmp	r0, #0
	strge	r10, [sp, #60]
	movge	r5, #0
	movge	r9, #16
	blt	.L876
.L856:
	ldr	r3, [r4, #28]
	add	r0, sp, #52
	str	r8, [sp, #68]
	str	r7, [sp, #72]
	str	r9, [sp, #76]
	str	r3, [sp, #64]
	bl	m0_write_page
	cmp	r0, #0
	blt	.L877
.L854:
	add	r5, r5, #1
	cmp	r5, #2
	str	r5, [sp, #60]
	bne	.L856
	ldr	r1, [r4, #28]
	add	r0, sp, #52
	str	fp, [sp, #68]
	str	r7, [sp, #72]
	lsr	r1, r1, #1
	bl	m1_write_page_FF
	cmp	r0, #0
	blt	.L878
.L853:
	add	r6, r6, #1
	cmp	r6, #12
	bne	.L858
	ldr	r3, [r4, #248]
	movw	r0, #:lower16:.LC167
	ldr	r2, [sp, #52]
	movt	r0, #:upper16:.LC167
	ldr	r1, [r3, #8]
	bl	NAND_Print_DBG
	b	.L859
.L878:
	ldr	r3, [sp, #60]
	ldr	r0, [sp, #24]
	ldr	r2, [sp, #56]
	ldr	r1, [sp, #52]
	bl	NAND_Print
	b	.L853
.L877:
	ldr	r3, [sp, #60]
	ldr	r0, [sp, #24]
	ldr	r2, [sp, #56]
	ldr	r1, [sp, #52]
	bl	NAND_Print
	b	.L854
.L876:
	ldr	r2, [sp, #56]
	ldr	r0, [sp, #20]
	ldr	r1, [sp, #52]
	bl	NAND_Print
	b	.L853
.L875:
	ldrb	r3, [sp, #146]	@ zero_extendqisi2
	cmp	r3, #79
	bne	.L843
	ldrb	r3, [sp, #147]	@ zero_extendqisi2
	cmp	r3, #66
	bne	.L843
	ldr	r1, [sp, #36]
	ldr	ip, [sp, #28]
	mov	r6, r1
	mov	r0, ip
	b	.L845
.L844:
	cmp	ip, r1
	beq	.L879
.L845:
	ldrb	r3, [r0, #1]!	@ zero_extendqisi2
	ldrb	r2, [r1, #1]!	@ zero_extendqisi2
	add	r3, r3, r2
	cmp	r3, #255
	beq	.L844
	ldr	r0, [sp, #20]
	bl	NAND_Print_DBG
	b	.L843
.L879:
	ldr	r3, [sp, #36]
	ldr	r1, [sp, #44]
	b	.L847
.L880:
	cmp	r1, r3
	beq	.L843
.L847:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r2, #0
	beq	.L880
	ldr	r1, [r4, #248]
	movw	r0, #:lower16:.LC168
	ldr	r3, [sp, #56]
	movt	r0, #:upper16:.LC168
	ldr	r2, [sp, #52]
	movw	r7, #:lower16:.LC164
	ldr	r1, [r1, #8]
	movw	r9, #:lower16:.LC132
	bl	NAND_Print_DBG
	ldr	r3, [r4, #248]
	movw	r0, #:lower16:.LC169
	ldr	r2, [sp, #52]
	movt	r7, #:upper16:.LC164
	movt	r0, #:upper16:.LC169
	movt	r9, #:upper16:.LC132
	ldr	r1, [r3, #8]
	mov	r5, r8
	mov	r10, #0
	bl	NAND_Print_DBG
	ldrb	r1, [r8]	@ zero_extendqisi2
	mov	r0, r7
	bl	NAND_Print_DBG
.L860:
	add	r10, r10, #1
	mov	r0, r7
	cmp	r10, #32
	beq	.L881
.L849:
	ldrb	r1, [r5, #1]!	@ zero_extendqisi2
	bl	NAND_Print_DBG
	and	r3, r10, #7
	cmp	r3, #7
	bne	.L860
	mov	r0, r9
	add	r10, r10, #1
	bl	NAND_Print_DBG
	cmp	r10, #32
	mov	r0, r7
	bne	.L849
.L881:
	ldr	r1, [sp, #28]
	add	r3, r4, #47
.L850:
	ldrb	r2, [r6, #1]!	@ zero_extendqisi2
	cmp	r1, r6
	strb	r2, [r3, #1]!
	bne	.L850
	mov	r0, r4
	bl	m1_setdefaultparam
	movw	r1, #18048
	ldr	r0, [sp, #32]
	bl	nand_free_temp_buf
	ldr	r3, [r4, #252]
	mov	r0, r8
	ldr	r1, [r3, #12]
	lsl	r1, r1, #9
	bl	nand_free_temp_buf
	mov	r0, #0
.L871:
	add	sp, sp, #212
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L862:
	mvn	r0, #76
	b	.L871
.L874:
	ldr	r3, [r4, #252]
	mov	r0, r8
	ldr	r1, [r3, #12]
	lsl	r1, r1, #9
	bl	nand_free_temp_buf
	movw	r0, #:lower16:.LC161
	movt	r0, #:upper16:.LC161
	bl	NAND_Print
	mvn	r0, #75
	b	.L871
	.size	m1_init_readretry_param, .-m1_init_readretry_param
	.align	2
	.global	m1_readretry_init
	.syntax unified
	.arm
	.type	m1_readretry_init, %function
m1_readretry_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r2, [r0, #252]
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	push	{r4, lr}
	ldr	r2, [r2, #40]
	ubfx	r1, r2, #16, #8
	strb	r2, [r3, #194]
	cmp	r1, #4
	lsr	r2, r2, #8
	strb	r1, [r3, #193]
	strb	r2, [r3, #200]
	beq	.L890
	cmp	r1, #6
	bne	.L885
	mov	lr, #14
	mov	ip, #15
	mov	r1, #16
	mov	r2, #17
	strb	lr, [r3, #196]
	strb	ip, [r3, #197]
	strb	r1, [r3, #198]
	strb	r2, [r3, #199]
.L884:
	mov	ip, #0
	add	r3, r0, #47
	mov	r1, ip
	add	r2, r0, #175
	str	ip, [r0, #176]
.L887:
	strb	r1, [r3, #1]!
	cmp	r3, r2
	bne	.L887
	bl	m1_init_readretry_param
	mov	r0, #0
	pop	{r4, pc}
.L890:
	mov	lr, #56
	mov	ip, #57
	mov	r1, #58
	mov	r2, #59
	strb	lr, [r3, #196]
	strb	ip, [r3, #197]
	strb	r1, [r3, #198]
	strb	r2, [r3, #199]
	b	.L884
.L885:
	movw	r0, #:lower16:.LC139
	movt	r0, #:upper16:.LC139
	bl	NAND_Print
	mvn	r0, #74
	pop	{r4, pc}
	.size	m1_readretry_init, .-m1_readretry_init
	.align	2
	.global	m1_readretry_exit
	.syntax unified
	.arm
	.type	m1_readretry_exit, %function
m1_readretry_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r4, r0
	movw	r0, #:lower16:.LC171
	movt	r0, #:upper16:.LC171
	bl	NAND_Print_DBG
	mov	r0, r4
	bl	m1_setdefaultparam
	mov	r0, #0
	str	r0, [r4, #176]
	pop	{r4, pc}
	.size	m1_readretry_exit, .-m1_readretry_exit
	.align	2
	.global	m1_set_readretry
	.syntax unified
	.arm
	.type	m1_set_readretry, %function
m1_set_readretry:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:.LANCHOR0
	ldr	ip, [r0, #176]
	movt	r3, #:upper16:.LANCHOR0
	ldrb	r2, [r3, #200]	@ zero_extendqisi2
	cmp	ip, r2
	bhi	.L897
	push	{r4, lr}
	sub	sp, sp, #8
	ldrb	r3, [r3, #194]	@ zero_extendqisi2
	cmp	r3, #0
	moveq	r1, sp
	beq	.L895
	add	ip, r0, ip, lsl #2
	sub	r4, r3, #1
	mov	r1, sp
	add	ip, ip, #47
	add	r4, sp, r4
	sub	r2, sp, #1
.L896:
	ldrb	lr, [ip, #1]!	@ zero_extendqisi2
	strb	lr, [r2, #1]!
	cmp	r2, r4
	bne	.L896
.L895:
	ldr	r2, .L904
	bl	m1_vender_set_param
	add	sp, sp, #8
	@ sp needed
	pop	{r4, pc}
.L897:
	mvn	r0, #73
	bx	lr
.L905:
	.align	2
.L904:
	.word	.LANCHOR0+196
	.size	m1_set_readretry, .-m1_set_readretry
	.align	2
	.global	m1_read_page_end
	.syntax unified
	.arm
	.type	m1_read_page_end, %function
m1_read_page_end:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, r7, r8, r9, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r1, [r0]
	sub	sp, sp, #12
	mov	r6, r0
	ldr	r0, [r3]
	bl	nci_get_from_nsi
	mov	r5, r0
	mov	r0, r6
	bl	m0_read_page_end_not_retry
	cmn	r0, #2
	mov	r4, r0
	beq	.L919
	mov	r0, r4
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
.L919:
	movw	r0, #:lower16:.LC172
	movw	r8, #:lower16:.LANCHOR0
	movt	r0, #:upper16:.LC172
	movt	r8, #:upper16:.LANCHOR0
	bl	NAND_Print_DBG
	ldrb	r2, [r8, #200]	@ zero_extendqisi2
	cmp	r2, #0
	beq	.L908
	mov	r7, #0
	mov	r9, r7
	b	.L913
.L911:
	bl	m0_read_page_start
	mov	r4, r0
	mov	r0, r6
	bl	m0_read_page_end_not_retry
	orr	r4, r4, r0
	cmp	r4, #0
	cmpne	r4, #10
	beq	.L920
	ldrb	r2, [r8, #200]	@ zero_extendqisi2
	cmp	r2, r7
	ble	.L908
.L913:
	ldr	r3, [r5, #176]
	mov	r0, r5
	add	r7, r7, #1
	add	r3, r3, #1
	cmp	r3, r2
	movhi	r3, r9
	str	r3, [r5, #176]
	bl	m1_set_readretry
	subs	r4, r0, #0
	mov	r0, r6
	beq	.L911
	mov	r3, #0
	mov	r0, r5
	str	r3, [r5, #176]
	bl	m1_set_readretry
.L908:
	mov	r3, #0
	mov	r0, r5
	str	r3, [r5, #176]
	bl	m1_set_readretry
	mov	r0, r4
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
.L920:
	ldr	r2, [r5, #248]
	movw	r0, #:lower16:.LC173
	ldr	lr, [r6, #8]
	movt	r0, #:upper16:.LC173
	ldr	ip, [r5, #176]
	mov	r4, #10
	ldr	r1, [r2, #8]
	ldr	r3, [r6, #4]
	ldr	r2, [r5, #20]
	str	lr, [sp]
	str	ip, [sp, #4]
	bl	NAND_Print_DBG
	b	.L908
	.size	m1_read_page_end, .-m1_read_page_end
	.align	2
	.global	m1_special_init
	.syntax unified
	.arm
	.type	m1_special_init, %function
m1_special_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r4, [r3, #8]
	cmp	r4, #0
	beq	.L922
	mov	r5, #0
.L923:
	mov	r0, r4
	bl	m1_readretry_init
	ldr	r4, [r4]
	orr	r5, r5, r0
	cmp	r4, #0
	bne	.L923
	cmp	r5, #0
	beq	.L922
	movw	r1, #:lower16:.LANCHOR0
	movw	r0, #:lower16:.LC175
	movt	r1, #:upper16:.LANCHOR0
	movt	r0, #:upper16:.LC175
	ldrb	r3, [r1, #194]	@ zero_extendqisi2
	ldrb	r2, [r1, #200]	@ zero_extendqisi2
	ldrb	r1, [r1, #193]	@ zero_extendqisi2
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L922:
	movw	r1, #:lower16:.LANCHOR0
	movw	ip, #:lower16:function_read_page_end
	movt	r1, #:upper16:.LANCHOR0
	movw	lr, #:lower16:m1_read_page_end
	movw	r0, #:lower16:.LC174
	ldrb	r3, [r1, #194]	@ zero_extendqisi2
	ldrb	r2, [r1, #200]	@ zero_extendqisi2
	movt	ip, #:upper16:function_read_page_end
	movt	lr, #:upper16:m1_read_page_end
	ldrb	r1, [r1, #193]	@ zero_extendqisi2
	mov	r5, #0
	movt	r0, #:upper16:.LC174
	str	lr, [ip]
	bl	NAND_Print_DBG
	mov	r0, r5
	pop	{r4, r5, r6, pc}
	.size	m1_special_init, .-m1_special_init
	.align	2
	.global	m1_special_exit
	.syntax unified
	.arm
	.type	m1_special_exit, %function
m1_special_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r4, [r3, #8]
	cmp	r4, #0
	beq	.L934
	movw	r5, #:lower16:.LC171
	mov	r6, #0
	movt	r5, #:upper16:.LC171
.L933:
	mov	r0, r5
	bl	NAND_Print_DBG
	mov	r0, r4
	bl	m1_setdefaultparam
	str	r6, [r4, #176]
	ldr	r4, [r4]
	cmp	r4, #0
	bne	.L933
.L934:
	movw	r0, #:lower16:.LC176
	movt	r0, #:upper16:.LC176
	bl	NAND_Print_DBG
	mov	r0, #0
	pop	{r4, r5, r6, pc}
	.size	m1_special_exit, .-m1_special_exit
	.align	2
	.global	m1_is_lsb_page
	.syntax unified
	.arm
	.type	m1_is_lsb_page, %function
m1_is_lsb_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	movw	r3, #:lower16:g_nsi
	cmp	r0, #0
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r3, [r3, #8]
	ldr	r3, [r3, #32]
	beq	.L940
	sub	r3, r3, #1
	cmp	r0, r3
	beq	.L941
	and	r0, r0, #1
	bx	lr
.L940:
	mov	r0, #1
	bx	lr
.L941:
	mov	r0, #0
	bx	lr
	.size	m1_is_lsb_page, .-m1_is_lsb_page
	.align	2
	.global	micron_intel_set_feature
	.syntax unified
	.arm
	.type	micron_intel_set_feature, %function
micron_intel_set_feature:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, lr}
	sub	sp, sp, #20
	mov	r3, r1
	mov	r4, #0
	mov	r6, r2
	add	r1, sp, #7
	mov	r7, r0
	add	r2, sp, #8
	strb	r3, [sp, #7]
	mov	r5, r3
	strb	r6, [sp, #8]
	strb	r4, [sp, #9]
	strb	r4, [sp, #10]
	strb	r4, [sp, #11]
	bl	nand_set_feature
	add	r2, sp, #12
	add	r1, sp, #7
	mov	r0, r7
	bl	nand_get_feature
	ldrb	r2, [sp, #12]	@ zero_extendqisi2
	ldrb	r3, [sp, #8]	@ zero_extendqisi2
	cmp	r2, r3
	moveq	r0, r4
	bne	.L946
.L943:
	add	sp, sp, #20
	@ sp needed
	pop	{r4, r5, r6, r7, pc}
.L946:
	movw	r0, #:lower16:.LC177
	mov	r2, r6
	mov	r1, r5
	movt	r0, #:upper16:.LC177
	bl	NAND_Print
	mvn	r0, #71
	b	.L943
	.size	micron_intel_set_feature, .-micron_intel_set_feature
	.align	2
	.global	m4_readretry_init
	.syntax unified
	.arm
	.type	m4_readretry_init, %function
m4_readretry_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #252]
	mov	r1, #137
	push	{r4, r5, r6, r7, r8, lr}
	mov	r7, #0
	ldr	r5, .L955
	movw	r4, #:lower16:.LANCHOR0
	movt	r4, #:upper16:.LANCHOR0
	mov	r2, r7
	ldr	r3, [r3, #40]
	mov	r6, r0
	str	r7, [r0, #176]
	str	r5, [r4, #204]
	lsr	ip, r3, #16
	lsr	r3, r3, #8
	strb	ip, [r4, #208]
	strb	r3, [r4, #209]
	bl	micron_intel_set_feature
	ldrb	r3, [r4, #208]	@ zero_extendqisi2
	cmp	r3, #65
	moveq	r1, #12
	streq	r5, [r4, #204]
	strbeq	r1, [r4, #209]
	beq	.L949
	cmp	r3, #64
	beq	.L954
	cmp	r3, #80
	bne	.L951
	mov	r3, #7
	mov	r1, #147
	add	r5, r5, #24
	mov	r2, r7
	mov	r0, r6
	str	r5, [r4, #204]
	strb	r3, [r4, #209]
	bl	micron_intel_set_feature
	ldrb	r1, [r4, #209]	@ zero_extendqisi2
.L949:
	movw	r0, #:lower16:.LC179
	movt	r0, #:upper16:.LC179
	bl	NAND_Print_DBG
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, pc}
.L954:
	mov	r3, #10
	add	r5, r5, #12
	str	r5, [r4, #204]
	mov	r1, r3
	strb	r3, [r4, #209]
	b	.L949
.L951:
	movw	r0, #:lower16:.LC178
	movt	r0, #:upper16:.LC178
	bl	NAND_Print
	mvn	r0, #70
	pop	{r4, r5, r6, r7, r8, pc}
.L956:
	.align	2
.L955:
	.word	.LANCHOR1+500
	.size	m4_readretry_init, .-m4_readretry_init
	.align	2
	.global	m4_readretry_exit
	.syntax unified
	.arm
	.type	m4_readretry_exit, %function
m4_readretry_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	movw	r3, #:lower16:.LANCHOR0
	mov	r2, #0
	movt	r3, #:upper16:.LANCHOR0
	mov	r0, r2
	strb	r2, [r3, #208]
	strb	r2, [r3, #209]
	bx	lr
	.size	m4_readretry_exit, .-m4_readretry_exit
	.align	2
	.global	m4_set_readretry
	.syntax unified
	.arm
	.type	m4_set_readretry, %function
m4_set_readretry:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r4, r0
	movw	r5, #:lower16:.LANCHOR0
	movt	r5, #:upper16:.LANCHOR0
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	ldrb	r3, [r5, #208]	@ zero_extendqisi2
	sub	r2, r3, #64
	cmp	r3, #80
	cmpne	r2, #1
	movhi	r7, #0
	bhi	.L959
	ldr	r1, [r4, #176]
	uxtb	r2, r1
	cmp	r2, #255
	movne	r6, r2
	moveq	r6, #0
	cmp	r3, #80
	beq	.L967
.L961:
	ldr	r3, [r5, #204]
	mov	r1, #137
	mov	r0, r4
	ldrb	r2, [r3, r6]	@ zero_extendqisi2
	bl	micron_intel_set_feature
	mov	r7, r0
.L959:
	ldr	r3, [r4, #176]
	mov	r0, r4
	cmp	r3, #255
	moveq	r3, #0
	streq	r3, [r4, #176]
	bl	nand_disable_chip
	mov	r0, r7
	pop	{r4, r5, r6, r7, r8, pc}
.L967:
	ldr	r3, [r5, #204]
	ldrb	r3, [r3, r6]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L968
.L962:
	cmp	r1, #255
	bne	.L961
	mov	r2, #0
	mov	r1, #137
	mov	r0, r4
	bl	micron_intel_set_feature
	mov	r2, #0
	mov	r7, r0
	mov	r1, #147
	mov	r0, r4
	bl	micron_intel_set_feature
	orr	r7, r7, r0
	b	.L959
.L968:
	mov	r2, #1
	mov	r1, #147
	mov	r0, r4
	bl	micron_intel_set_feature
	subs	r7, r0, #0
	bne	.L959
	ldr	r1, [r4, #176]
	b	.L962
	.size	m4_set_readretry, .-m4_set_readretry
	.align	2
	.global	m4_read_page_end
	.syntax unified
	.arm
	.type	m4_read_page_end, %function
m4_read_page_end:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, r7, r8, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r1, [r0]
	sub	sp, sp, #8
	mov	r8, r0
	ldr	r0, [r3]
	bl	nci_get_from_nsi
	mov	r7, r0
	mov	r0, r8
	bl	m0_read_page_end_not_retry
	cmn	r0, #2
	mov	r5, r0
	beq	.L982
	mov	r0, r5
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L982:
	movw	r0, #:lower16:.LC180
	movw	r6, #:lower16:.LANCHOR0
	movt	r0, #:upper16:.LC180
	movt	r6, #:upper16:.LANCHOR0
	bl	NAND_Print_DBG
	ldrb	r3, [r6, #209]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L971
	mov	r4, #0
	b	.L973
.L972:
	ldrb	r3, [r6, #209]	@ zero_extendqisi2
	cmp	r3, r4
	ble	.L971
.L973:
	str	r4, [r7, #176]
	mov	r0, r7
	bl	m4_set_readretry
	subs	r5, r0, #0
	add	r4, r4, #1
	bne	.L972
	mov	r0, r8
	bl	m0_read_page_start
	mov	r5, r0
	mov	r0, r8
	bl	m0_read_page_end_not_retry
	orr	r5, r5, r0
	cmp	r5, #0
	cmpne	r5, #10
	bne	.L972
	ldr	r2, [r7, #248]
	movw	r0, #:lower16:.LC181
	ldr	lr, [r8, #8]
	movt	r0, #:upper16:.LC181
	ldr	ip, [r7, #176]
	mov	r5, #10
	ldr	r1, [r2, #8]
	ldr	r3, [r8, #4]
	ldr	r2, [r7, #20]
	str	lr, [sp]
	str	ip, [sp, #4]
	bl	NAND_Print_DBG
.L971:
	mov	r3, #255
	mov	r0, r7
	str	r3, [r7, #176]
	bl	m4_set_readretry
	mov	r0, r5
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
	.size	m4_read_page_end, .-m4_read_page_end
	.align	2
	.global	m4_special_init
	.syntax unified
	.arm
	.type	m4_special_init, %function
m4_special_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r4, [r3, #8]
	cmp	r4, #0
	beq	.L984
	mov	r5, #0
.L985:
	mov	r0, r4
	bl	m4_readretry_init
	ldr	r4, [r4]
	orr	r5, r5, r0
	cmp	r4, #0
	bne	.L985
	cmp	r5, #0
	beq	.L984
	movw	r3, #:lower16:.LANCHOR0
	movw	r0, #:lower16:.LC183
	movt	r3, #:upper16:.LANCHOR0
	movt	r0, #:upper16:.LC183
	ldrb	r2, [r3, #209]	@ zero_extendqisi2
	ldrb	r1, [r3, #208]	@ zero_extendqisi2
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L984:
	movw	r3, #:lower16:.LANCHOR0
	movw	ip, #:lower16:function_read_page_end
	movt	r3, #:upper16:.LANCHOR0
	movw	lr, #:lower16:m4_read_page_end
	movw	r0, #:lower16:.LC182
	movt	ip, #:upper16:function_read_page_end
	movt	lr, #:upper16:m4_read_page_end
	ldrb	r2, [r3, #209]	@ zero_extendqisi2
	ldrb	r1, [r3, #208]	@ zero_extendqisi2
	mov	r5, #0
	movt	r0, #:upper16:.LC182
	str	lr, [ip]
	bl	NAND_Print_DBG
	mov	r0, r5
	pop	{r4, r5, r6, pc}
	.size	m4_special_init, .-m4_special_init
	.align	2
	.global	m4_special_exit
	.syntax unified
	.arm
	.type	m4_special_exit, %function
m4_special_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r3, [r3, #8]
	cmp	r3, #0
	beq	.L996
.L995:
	ldr	r3, [r3]
	cmp	r3, #0
	bne	.L995
	movw	r2, #:lower16:.LANCHOR0
	movt	r2, #:upper16:.LANCHOR0
	strb	r3, [r2, #208]
	strb	r3, [r2, #209]
.L996:
	movw	r0, #:lower16:.LC184
	movt	r0, #:upper16:.LC184
	bl	NAND_Print_DBG
	mov	r0, #0
	pop	{r4, pc}
	.size	m4_special_exit, .-m4_special_exit
	.align	2
	.global	m4_0x41_is_lsb_page
	.syntax unified
	.arm
	.type	m4_0x41_is_lsb_page, %function
m4_0x41_is_lsb_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	r3, r0, #2
	cmp	r3, #1
	bls	.L1004
	movw	r3, #:lower16:g_nsi
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r3, [r3, #8]
	ldr	r3, [r3, #252]
	ldr	r3, [r3, #16]
	sub	r2, r3, #2
	cmp	r0, r2
	beq	.L1004
	sub	r3, r3, #1
	cmp	r0, r3
	beq	.L1004
	and	r0, r0, #3
	cmp	r0, #1
	movhi	r0, #0
	movls	r0, #1
	bx	lr
.L1004:
	mov	r0, #1
	bx	lr
	.size	m4_0x41_is_lsb_page, .-m4_0x41_is_lsb_page
	.align	2
	.global	m4_0x42_is_lsb_page
	.syntax unified
	.arm
	.type	m4_0x42_is_lsb_page, %function
m4_0x42_is_lsb_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	r3, r0, #7
	cmp	r0, #5
	cmphi	r3, #1
	movls	r3, #1
	movhi	r3, #0
	bls	.L1008
	movw	r2, #509
	cmp	r0, r2
	beq	.L1008
	cmp	r0, #508
	cmpne	r0, #6
	movw	r1, #511
	moveq	r2, #1
	movne	r2, #0
	cmp	r0, r1
	orreq	r2, r2, #1
	cmp	r2, #0
	bne	.L1009
	and	r0, r0, #3
	sub	r0, r0, #2
	cmp	r0, #1
	movhi	r0, #0
	movls	r0, #1
	bx	lr
.L1008:
	mov	r0, #1
	bx	lr
.L1009:
	mov	r0, r3
	bx	lr
	.size	m4_0x42_is_lsb_page, .-m4_0x42_is_lsb_page
	.align	2
	.global	m2_vender_get_param
	.syntax unified
	.arm
	.type	m2_vender_get_param, %function
m2_vender_get_param:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r7, r0
	sub	sp, sp, #16
	mov	r6, r3
	mov	r5, r1
	mov	r4, r2
	bl	nand_enable_chip
	ldr	r0, [r7, #248]
	bl	ndfc_disable_randomize
	cmp	r6, #0
	mov	r3, #55
	strb	r3, [sp, #15]
	beq	.L1013
	add	r6, r4, r6
	mov	r8, #1
.L1012:
	mov	r3, r5
	mov	r2, r4
	str	r8, [sp]
	add	r1, sp, #15
	add	r4, r4, #1
	mov	r0, r7
	bl	get_data_with_nand_bus_one_cmd
	cmp	r4, r6
	add	r5, r5, #1
	bne	.L1012
.L1013:
	mov	r0, r7
	bl	nand_disable_chip
	mov	r0, #0
	add	sp, sp, #16
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
	.size	m2_vender_get_param, .-m2_vender_get_param
	.align	2
	.global	m2_vender_set_param
	.syntax unified
	.arm
	.type	m2_vender_set_param, %function
m2_vender_set_param:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	ip, #54
	sub	sp, sp, #24
	mov	r6, r0
	mov	r8, r3
	mov	r5, r1
	mov	r4, r2
	strb	ip, [sp, #23]
	bl	nand_enable_chip
	ldr	r0, [r6, #248]
	bl	ndfc_disable_randomize
	cmp	r8, #0
	beq	.L1020
	add	r8, r4, r8
	mov	r7, #1
.L1019:
	mov	r3, r4
	mov	r2, #0
	str	r5, [sp]
	add	r1, sp, #23
	str	r7, [sp, #8]
	mov	r0, r6
	str	r7, [sp, #4]
	add	r4, r4, #1
	bl	set_cmd_with_nand_bus
	mov	r2, #0
	mov	r1, #22
	mov	r0, r6
	add	r5, r5, #1
	bl	set_one_cmd
	cmp	r4, r8
	bne	.L1019
.L1020:
	mov	r0, r6
	bl	nand_disable_chip
	mov	r0, #0
	add	sp, sp, #24
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
	.size	m2_vender_set_param, .-m2_vender_set_param
	.align	2
	.global	m2_lsb_init
	.syntax unified
	.arm
	.type	m2_lsb_init, %function
m2_lsb_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r2, [r0, #252]
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	push	{r4, lr}
	ldr	r2, [r2, #40]
	ubfx	r1, r2, #16, #8
	strb	r2, [r3, #212]
	cmp	r1, #2
	lsr	r2, r2, #8
	strb	r1, [r3, #210]
	strb	r2, [r3, #211]
	beq	.L1029
	cmp	r1, #3
	beq	.L1030
	mov	r0, #0
	pop	{r4, pc}
.L1030:
	mov	r4, #4
	mvn	lr, #95
	mvn	ip, #94
	mvn	r0, #88
	mvn	r1, #87
.L1028:
	mov	r2, #10
	strb	r0, [r3, #218]
	strb	r4, [r3, #213]
	mov	r0, #0
	strb	lr, [r3, #216]
	strb	ip, [r3, #217]
	strb	r1, [r3, #219]
	strb	r2, [r3, #220]
	strb	r2, [r3, #221]
	strb	r2, [r3, #222]
	strb	r2, [r3, #223]
	pop	{r4, pc}
.L1029:
	mov	r4, #4
	mvn	lr, #79
	mvn	ip, #78
	mvn	r0, #95
	mvn	r1, #94
	b	.L1028
	.size	m2_lsb_init, .-m2_lsb_init
	.align	2
	.global	m2_lsb_enable
	.syntax unified
	.arm
	.type	m2_lsb_enable, %function
m2_lsb_enable:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	movw	r4, #:lower16:.LANCHOR0
	movt	r4, #:upper16:.LANCHOR0
	sub	sp, sp, #8
	add	r5, r4, #224
	add	r2, r4, #216
	ldrb	r3, [r4, #213]	@ zero_extendqisi2
	mov	r1, r5
	mov	r6, r0
	bl	m2_vender_get_param
	ldrb	r3, [r4, #213]	@ zero_extendqisi2
	cmp	r3, #0
	addeq	r1, sp, #4
	beq	.L1032
	add	r1, sp, #4
	mov	r0, r5
	add	r2, r4, #220
	add	r5, r5, r3
	mov	lr, r1
.L1033:
	ldrb	ip, [r0], #1	@ zero_extendqisi2
	ldrb	r4, [r2], #1	@ zero_extendqisi2
	cmp	r0, r5
	add	ip, ip, r4
	strb	ip, [lr], #1
	bne	.L1033
.L1032:
	mov	r0, r6
	ldr	r2, .L1039
	bl	m2_vender_set_param
	mov	r0, #0
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, pc}
.L1040:
	.align	2
.L1039:
	.word	.LANCHOR0+216
	.size	m2_lsb_enable, .-m2_lsb_enable
	.align	2
	.global	m2_lsb_disable
	.syntax unified
	.arm
	.type	m2_lsb_disable, %function
m2_lsb_disable:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r1, #:lower16:.LANCHOR0
	push	{r4, lr}
	movt	r1, #:upper16:.LANCHOR0
	add	r2, r1, #216
	add	r1, r1, #224
	ldrb	r3, [r1, #-11]	@ zero_extendqisi2
	bl	m2_vender_set_param
	mov	r0, #0
	pop	{r4, pc}
	.size	m2_lsb_disable, .-m2_lsb_disable
	.align	2
	.global	m2_lsb_exit
	.syntax unified
	.arm
	.type	m2_lsb_exit, %function
m2_lsb_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r4, r0
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	mov	r2, #0
	mov	r0, r4
	mov	r1, r2
	bl	set_one_cmd
	mov	r0, r4
	mov	r1, #0
	bl	set_one_addr
	mov	r0, r4
	mov	r1, #0
	bl	set_one_addr
	mov	r0, r4
	mov	r1, #0
	bl	set_one_addr
	mov	r0, r4
	mov	r1, #0
	bl	set_one_addr
	mov	r0, r4
	mov	r1, #0
	bl	set_one_addr
	mov	r2, #1
	mov	r1, #48
	mov	r0, r4
	bl	set_one_cmd
	movw	r0, #:lower16:.LC185
	movt	r0, #:upper16:.LC185
	bl	NAND_Print_DBG
	mov	r0, r4
	bl	nand_disable_chip
	mov	r0, #0
	pop	{r4, pc}
	.size	m2_lsb_exit, .-m2_lsb_exit
	.align	2
	.global	m2_vender_get_param_otp_hynix
	.syntax unified
	.arm
	.type	m2_vender_get_param_otp_hynix, %function
m2_vender_get_param_otp_hynix:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:.LANCHOR0
	push	{r4, r5, r6, r7, r8, lr}
	movt	r3, #:upper16:.LANCHOR0
	sub	sp, sp, #32
	mov	r2, #0
	ldrb	r3, [r3, #210]	@ zero_extendqisi2
	strb	r2, [sp, #28]
	cmp	r3, #2
	beq	.L1068
	cmp	r3, #3
	bne	.L1058
	mvn	r1, #81
	mvn	r2, #79
	mov	r3, #77
	strb	r1, [sp, #24]
	strb	r2, [sp, #25]
	strb	r3, [sp, #29]
.L1047:
	mov	r4, r0
	mov	r0, #1024
	bl	nand_get_temp_buf
	mov	r5, r0
	mov	r0, r4
	bl	nand_enable_chip
	mov	r6, #1
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	mov	r2, r6
	mov	r1, #255
	mov	r0, r4
	bl	set_one_cmd
	add	r2, sp, #28
	mov	ip, #54
	add	r1, sp, #20
	add	r3, sp, #24
	str	r2, [sp]
	mov	r0, r4
	mov	r2, #0
	str	r6, [sp, #8]
	str	r6, [sp, #4]
	strb	ip, [sp, #20]
	bl	set_cmd_with_nand_bus
	mov	r2, #0
	add	r0, sp, #29
	add	r3, sp, #25
	mov	r1, r2
	str	r0, [sp]
	mov	r0, r4
	str	r6, [sp, #8]
	str	r6, [sp, #4]
	bl	set_cmd_with_nand_bus
	mov	r2, #0
	mov	r1, #22
	mov	r0, r4
	bl	set_one_cmd
	mov	r2, #0
	mov	r1, #23
	mov	r0, r4
	bl	set_one_cmd
	mov	r2, #0
	mov	r1, #4
	mov	r0, r4
	bl	set_one_cmd
	mov	r2, #0
	mov	r1, #25
	mov	r0, r4
	bl	set_one_cmd
	mov	r2, #0
	mov	r0, r4
	mov	r1, r2
	bl	set_one_cmd
	mov	r1, #0
	mov	r0, r4
	bl	set_one_addr
	mov	r1, #0
	mov	r0, r4
	bl	set_one_addr
	mov	r1, #0
	mov	r0, r4
	bl	set_one_addr
	mov	r1, #2
	mov	r0, r4
	bl	set_one_addr
	mov	r1, #0
	mov	r0, r4
	bl	set_one_addr
	mov	r2, #2
	add	r1, sp, #20
	str	r2, [sp]
	mov	ip, #48
	mov	r2, #0
	mov	r3, r5
	mov	r0, r4
	strb	ip, [sp, #20]
	bl	get_data_with_nand_bus_one_cmd
	ldrb	r1, [r5]	@ zero_extendqisi2
	ldrb	r2, [r5, #1]	@ zero_extendqisi2
	cmp	r1, #8
	beq	.L1069
.L1050:
	movw	r0, #:lower16:.LC156
	mvn	r6, #95
	movt	r0, #:upper16:.LC156
	bl	NAND_Print
.L1051:
	mov	r3, #1024
	mov	r2, #0
	str	r3, [sp]
	mov	r1, r2
	mov	r3, r5
	mov	r0, r4
	bl	get_data_with_nand_bus_one_cmd
	add	r7, r5, #63
	mov	r1, #0
	add	r8, r4, #47
.L1052:
	sub	r2, r7, #64
	mov	r0, r8
	mov	lr, r7
	b	.L1054
.L1071:
	cmp	r7, r2
	beq	.L1070
.L1054:
	ldrb	ip, [r2, #1]!	@ zero_extendqisi2
	strb	ip, [r0, #1]!
	ldrb	r3, [lr, #1]!	@ zero_extendqisi2
	add	r3, r3, ip
	cmp	r3, #255
	beq	.L1071
	add	r1, r1, #1
	add	r7, r7, #128
	cmp	r1, #8
	bne	.L1052
	movw	r0, #:lower16:.LC187
	mvn	r6, #94
	movt	r0, #:upper16:.LC187
	bl	NAND_Print_DBG
.L1057:
	mov	r2, #1
	mov	r1, #255
	mov	r0, r4
	bl	set_one_cmd
	mov	r2, #1
	mov	r1, #56
	mov	r0, r4
	bl	set_one_cmd
	mov	r0, r4
	bl	nand_disable_chip
	mov	r0, r5
	mov	r1, #1024
	bl	nand_free_temp_buf
	mov	r0, r6
.L1063:
	add	sp, sp, #32
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L1070:
	movw	r0, #:lower16:.LC186
	movt	r0, #:upper16:.LC186
	bl	NAND_Print_DBG
	b	.L1057
.L1069:
	cmp	r2, #8
	moveq	r6, #0
	bne	.L1050
	b	.L1051
.L1068:
	mvn	ip, #0
	mvn	r1, #51
	mov	r2, #64
	mov	r3, #77
	strb	ip, [sp, #24]
	strb	r1, [sp, #25]
	strb	r2, [sp, #28]
	strb	r3, [sp, #29]
	b	.L1047
.L1058:
	mvn	r0, #96
	b	.L1063
	.size	m2_vender_get_param_otp_hynix, .-m2_vender_get_param_otp_hynix
	.align	2
	.global	m2_saveotpvalue
	.syntax unified
	.arm
	.type	m2_saveotpvalue, %function
m2_saveotpvalue:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	r3, r1, #1
	add	r0, r0, #47
	add	r1, r1, #63
.L1073:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r3, r1
	strb	r2, [r0, #1]!
	bne	.L1073
	bx	lr
	.size	m2_saveotpvalue, .-m2_saveotpvalue
	.align	2
	.global	m2_getotpparam
	.syntax unified
	.arm
	.type	m2_getotpparam, %function
m2_getotpparam:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	movw	r8, #:lower16:.LC160
	mov	r6, r0
	mov	r4, r1
	movt	r8, #:upper16:.LC160
	mov	r5, #0
.L1077:
	mov	r1, r5
	mov	r0, r8
	bl	NAND_Print_DBG
	mov	r0, r6
	add	r5, r5, #1
	bl	m2_vender_get_param_otp_hynix
	subs	r7, r0, #0
	beq	.L1076
	cmp	r5, #5
	bne	.L1077
	movw	r0, #:lower16:.LC188
	movt	r0, #:upper16:.LC188
	bl	NAND_Print
	mov	r0, r7
	pop	{r4, r5, r6, r7, r8, pc}
.L1076:
	add	r0, r4, #8
	add	ip, r6, #47
	add	lr, r6, #111
.L1080:
	sub	r3, r0, #8
	mov	r2, ip
.L1079:
	ldrb	r1, [r2, #1]!	@ zero_extendqisi2
	strb	r1, [r3], #1
	cmp	r3, r0
	bne	.L1079
	add	ip, ip, #8
	add	r0, r3, #8
	cmp	ip, lr
	bne	.L1080
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, pc}
	.size	m2_getotpparam, .-m2_getotpparam
	.align	2
	.global	m2_setdefaultparam
	.syntax unified
	.arm
	.type	m2_setdefaultparam, %function
m2_setdefaultparam:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	movw	r6, #:lower16:.LANCHOR0
	movt	r6, #:upper16:.LANCHOR0
	sub	sp, sp, #8
	ldrb	r3, [r6, #212]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L1090
	add	lr, r3, #47
	add	r2, r0, #47
	add	lr, r0, lr
	sub	r1, sp, #1
.L1091:
	ldrb	ip, [r2, #1]!	@ zero_extendqisi2
	cmp	r2, lr
	strb	ip, [r1, #1]!
	bne	.L1091
.L1090:
	ldr	r2, .L1102
	mov	r1, sp
	bl	m2_vender_set_param
	mov	r8, r0
	movw	r0, #:lower16:.LC136
	movt	r0, #:upper16:.LC136
	bl	NAND_Print_DBG
	ldrb	r3, [r6, #212]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L1094
	movw	r7, #:lower16:.LC137
	sub	r5, sp, #1
	movt	r7, #:upper16:.LC137
	mov	r4, #0
.L1093:
	ldrb	r1, [r5, #1]!	@ zero_extendqisi2
	mov	r0, r7
	bl	NAND_Print_DBG
	ldrb	r3, [r6, #212]	@ zero_extendqisi2
	add	r4, r4, #1
	cmp	r3, r4
	bhi	.L1093
.L1094:
	movw	r0, #:lower16:.LC132
	movt	r0, #:upper16:.LC132
	bl	NAND_Print_DBG
	mov	r0, r8
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L1103:
	.align	2
.L1102:
	.word	.LANCHOR0+228
	.size	m2_setdefaultparam, .-m2_setdefaultparam
	.align	2
	.global	m2_init_readretry_param
	.syntax unified
	.arm
	.type	m2_init_readretry_param, %function
m2_init_readretry_param:
	@ args = 0, pretend = 0, frame = 168
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #252]
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	mov	r5, r0
	sub	sp, sp, #188
	ldr	r0, [r3, #12]
	lsl	r0, r0, #9
	bl	nand_get_temp_buf
	subs	r7, r0, #0
	beq	.L1130
	movw	r2, #:lower16:.LC162
	movw	r3, #:lower16:.LANCHOR0
	movt	r2, #:upper16:.LC162
	add	r4, sp, #120
	mov	r10, r2
	movt	r3, #:upper16:.LANCHOR0
	str	r3, [sp, #20]
.L1105:
	movw	r3, #:lower16:.LC163
	mov	r6, #8
	mov	r9, #0
	movt	r3, #:upper16:.LC163
	str	r3, [sp, #16]
.L1121:
	ldr	r1, [r5, #16]
	mov	r3, #16
	ldr	r2, [r5, #28]
	add	r0, sp, #28
	str	r3, [sp, #52]
	str	r1, [sp, #28]
	str	r2, [sp, #40]
	str	r6, [sp, #32]
	str	r9, [sp, #36]
	str	r7, [sp, #44]
	str	r4, [sp, #48]
	bl	m0_read_page
	ldr	r1, [r5, #248]
	mov	r8, r0
	ldrb	ip, [sp, #122]	@ zero_extendqisi2
	mov	r0, r10
	ldrb	fp, [sp, #123]	@ zero_extendqisi2
	ldr	r1, [r1, #8]
	ldrb	lr, [sp, #121]	@ zero_extendqisi2
	str	ip, [sp, #8]
	ldrb	ip, [sp, #120]	@ zero_extendqisi2
	ldr	r3, [sp, #32]
	ldr	r2, [sp, #28]
	str	fp, [sp, #12]
	stm	sp, {ip, lr}
	bl	NAND_Print_DBG
	cmp	r8, #0
	blt	.L1107
	ldrb	r3, [sp, #120]	@ zero_extendqisi2
	cmp	r3, #0
	bne	.L1107
	ldrb	r3, [sp, #121]	@ zero_extendqisi2
	cmp	r3, #79
	beq	.L1134
.L1107:
	add	r6, r6, #1
	cmp	r6, #12
	bne	.L1121
	ldr	r3, [r5, #248]
	movw	r0, #:lower16:.LC170
	movt	r0, #:upper16:.LC170
	ldr	r1, [r3, #8]
	bl	NAND_Print_DBG
	add	r1, sp, #56
	mov	r0, r5
	bl	m2_getotpparam
	cmp	r0, #0
	bne	.L1135
	mov	r0, r5
	bl	m2_setdefaultparam
	add	r3, sp, #56
	sub	r0, r7, #1
	add	r1, r7, #63
.L1116:
	mov	r2, r3
	ldrb	ip, [r3], #1	@ zero_extendqisi2
	strb	ip, [r0, #1]!
	cmp	r4, r3
	ldrb	r2, [r2]	@ zero_extendqisi2
	mvn	r2, r2
	strb	r2, [r1, #1]!
	bne	.L1116
	mov	r2, #0
	mov	r3, #79
	mov	r1, #66
	mov	r0, r5
	movw	fp, #:lower16:.LC166
	movw	r9, #:lower16:.LC165
	mov	r8, r2
	strb	r2, [sp, #120]
	strb	r3, [sp, #121]
	movt	fp, #:upper16:.LC166
	strb	r3, [sp, #122]
	movt	r9, #:upper16:.LC165
	strb	r1, [sp, #123]
	mov	r6, #8
	bl	m2_lsb_init
	mov	r0, r5
	bl	m2_lsb_enable
.L1119:
	ldr	r1, [r5, #16]
	mov	r3, #16
	ldr	r2, [r5, #28]
	add	r0, sp, #28
	str	r6, [sp, #32]
	str	r8, [sp, #36]
	str	r7, [sp, #44]
	str	r4, [sp, #48]
	str	r1, [sp, #28]
	str	r2, [sp, #40]
	str	r3, [sp, #52]
	bl	m0_erase_block
	cmp	r0, #0
	add	r0, sp, #28
	blt	.L1136
	bl	m0_write_page
	cmp	r0, #0
	blt	.L1137
.L1118:
	add	r6, r6, #1
	cmp	r6, #12
	bne	.L1119
	ldr	r2, .L1141
	mov	r0, r5
	ldr	r3, [sp, #20]
	add	r1, r2, #8
	ldrb	r3, [r3, #213]	@ zero_extendqisi2
	bl	m2_vender_set_param
	mov	r0, r5
	bl	m2_lsb_exit
	ldr	r3, [r5, #248]
	movw	r0, #:lower16:.LC190
	movt	r0, #:upper16:.LC190
	ldr	r1, [r3, #8]
	bl	NAND_Print_DBG
	b	.L1105
.L1136:
	ldr	r2, [sp, #32]
	mov	r0, r9
	ldr	r1, [sp, #28]
	bl	NAND_Print
	b	.L1118
.L1134:
	ldrb	r3, [sp, #122]	@ zero_extendqisi2
	cmp	r3, #79
	bne	.L1107
	ldrb	r3, [sp, #123]	@ zero_extendqisi2
	cmp	r3, #66
	bne	.L1107
	sub	r8, r7, #1
	add	fp, r7, #63
	mov	r1, r8
	mov	r0, fp
	b	.L1109
.L1108:
	cmp	fp, r1
	beq	.L1138
.L1109:
	ldrb	r3, [r0, #1]!	@ zero_extendqisi2
	ldrb	r2, [r1, #1]!	@ zero_extendqisi2
	add	r3, r3, r2
	cmp	r3, #255
	beq	.L1108
	ldr	r0, [sp, #16]
	bl	NAND_Print_DBG
	b	.L1107
.L1137:
	ldr	r3, [sp, #36]
	mov	r0, fp
	ldr	r2, [sp, #32]
	ldr	r1, [sp, #28]
	bl	NAND_Print
	b	.L1118
.L1135:
	mov	r6, r0
.L1115:
	ldr	r3, [r5, #252]
	mov	r0, r7
	ldr	r1, [r3, #12]
	lsl	r1, r1, #9
	bl	nand_free_temp_buf
	mov	r0, r6
.L1131:
	add	sp, sp, #188
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L1138:
	mov	r3, r8
	b	.L1111
.L1139:
	cmp	fp, r3
	beq	.L1107
.L1111:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r2, #0
	beq	.L1139
	ldr	r1, [r5, #248]
	movw	r0, #:lower16:.LC168
	ldr	r3, [sp, #32]
	movt	r0, #:upper16:.LC168
	ldr	r2, [sp, #28]
	movw	r9, #:lower16:.LC164
	ldr	r1, [r1, #8]
	movw	r10, #:lower16:.LC132
	bl	NAND_Print_DBG
	ldr	r3, [r5, #248]
	movw	r0, #:lower16:.LC169
	ldr	r2, [sp, #28]
	movt	r9, #:upper16:.LC164
	movt	r0, #:upper16:.LC169
	movt	r10, #:upper16:.LC132
	ldr	r1, [r3, #8]
	mov	r6, r7
	mov	r4, #0
	bl	NAND_Print_DBG
	ldrb	r1, [r7]	@ zero_extendqisi2
	mov	r0, r9
	bl	NAND_Print_DBG
.L1120:
	add	r4, r4, #1
	mov	r0, r9
	cmp	r4, #64
	beq	.L1140
.L1113:
	ldrb	r1, [r6, #1]!	@ zero_extendqisi2
	bl	NAND_Print_DBG
	and	r3, r4, #7
	cmp	r3, #7
	bne	.L1120
	mov	r0, r10
	add	r4, r4, #1
	bl	NAND_Print_DBG
	cmp	r4, #64
	mov	r0, r9
	bne	.L1113
.L1140:
	add	r3, r5, #47
.L1114:
	ldrb	r2, [r8, #1]!	@ zero_extendqisi2
	cmp	fp, r8
	strb	r2, [r3, #1]!
	bne	.L1114
	mov	r0, r5
	bl	m2_setdefaultparam
	mov	r6, r0
	b	.L1115
.L1130:
	movw	r0, #:lower16:.LC189
	movt	r0, #:upper16:.LC189
	bl	NAND_Print_DBG
	mvn	r0, #93
	b	.L1131
.L1142:
	.align	2
.L1141:
	.word	.LANCHOR0+216
	.size	m2_init_readretry_param, .-m2_init_readretry_param
	.align	2
	.global	m2_readretry_init
	.syntax unified
	.arm
	.type	m2_readretry_init, %function
m2_readretry_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r2, [r0, #252]
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	push	{r4, r5, r6, lr}
	ldr	r2, [r2, #40]
	ubfx	r1, r2, #16, #8
	strb	r2, [r3, #212]
	cmp	r1, #2
	lsr	r2, r2, #8
	strb	r1, [r3, #210]
	strb	r2, [r3, #211]
	beq	.L1153
	cmp	r1, #3
	bne	.L1146
	mvn	r1, #79
	mvn	r2, #78
	strb	r1, [r3, #228]
	mvn	r5, #77
	strb	r2, [r3, #229]
	mvn	r4, #76
	mvn	lr, #75
	mvn	ip, #74
	mvn	r1, #73
	mvn	r2, #72
.L1152:
	strb	ip, [r3, #233]
	mov	ip, #0
	strb	r1, [r3, #234]
	mov	r1, ip
	strb	r2, [r3, #235]
	add	r2, r0, #175
	strb	r5, [r3, #230]
	strb	r4, [r3, #231]
	strb	lr, [r3, #232]
	add	r3, r0, #47
	str	ip, [r0, #176]
.L1148:
	strb	r1, [r3, #1]!
	cmp	r3, r2
	bne	.L1148
	pop	{r4, r5, r6, lr}
	b	m2_init_readretry_param
.L1153:
	mvn	r1, #51
	mvn	r2, #64
	strb	r1, [r3, #228]
	mvn	r5, #85
	strb	r2, [r3, #229]
	mvn	r4, #84
	mvn	lr, #50
	mvn	ip, #82
	mvn	r1, #81
	mvn	r2, #80
	b	.L1152
.L1146:
	movw	r0, #:lower16:.LC139
	movt	r0, #:upper16:.LC139
	bl	NAND_Print
	mvn	r0, #92
	pop	{r4, r5, r6, pc}
	.size	m2_readretry_init, .-m2_readretry_init
	.align	2
	.global	m2_readretry_exit
	.syntax unified
	.arm
	.type	m2_readretry_exit, %function
m2_readretry_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r4, r0
	movw	r0, #:lower16:.LC191
	movt	r0, #:upper16:.LC191
	bl	NAND_Print_DBG
	mov	r0, r4
	bl	m2_setdefaultparam
	mov	r0, #0
	str	r0, [r4, #176]
	pop	{r4, pc}
	.size	m2_readretry_exit, .-m2_readretry_exit
	.align	2
	.global	m2_set_readretry
	.syntax unified
	.arm
	.type	m2_set_readretry, %function
m2_set_readretry:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	movw	r6, #:lower16:.LANCHOR0
	movt	r6, #:upper16:.LANCHOR0
	ldr	ip, [r0, #176]
	sub	sp, sp, #8
	ldrb	r3, [r6, #211]	@ zero_extendqisi2
	cmp	ip, r3
	bhi	.L1163
	ldrb	r3, [r6, #212]	@ zero_extendqisi2
	cmp	r3, #0
	moveq	r1, sp
	beq	.L1158
	add	ip, r0, ip, lsl #3
	sub	r4, r3, #1
	mov	r1, sp
	add	ip, ip, #47
	add	r4, sp, r4
	sub	r2, sp, #1
.L1159:
	ldrb	lr, [ip, #1]!	@ zero_extendqisi2
	strb	lr, [r2, #1]!
	cmp	r2, r4
	bne	.L1159
.L1158:
	ldr	r2, .L1170
	bl	m2_vender_set_param
	mov	r8, r0
	movw	r0, #:lower16:.LC192
	movt	r0, #:upper16:.LC192
	bl	NAND_Print_DBG
	ldrb	r3, [r6, #212]	@ zero_extendqisi2
	cmp	r3, #0
	movwne	r7, #:lower16:.LC133
	subne	r5, sp, #1
	movtne	r7, #:upper16:.LC133
	movne	r4, #0
	beq	.L1162
.L1161:
	ldrb	r1, [r5, #1]!	@ zero_extendqisi2
	mov	r0, r7
	bl	NAND_Print
	ldrb	r3, [r6, #212]	@ zero_extendqisi2
	add	r4, r4, #1
	cmp	r3, r4
	bhi	.L1161
.L1162:
	movw	r0, #:lower16:.LC132
	movt	r0, #:upper16:.LC132
	bl	NAND_Print_DBG
	mov	r0, r8
.L1157:
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L1163:
	mvn	r0, #91
	b	.L1157
.L1171:
	.align	2
.L1170:
	.word	.LANCHOR0+228
	.size	m2_set_readretry, .-m2_set_readretry
	.align	2
	.global	m2_read_page_end
	.syntax unified
	.arm
	.type	m2_read_page_end, %function
m2_read_page_end:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r1, [r0]
	sub	sp, sp, #12
	mov	r4, r0
	ldr	r0, [r3]
	bl	nci_get_from_nsi
	mov	r5, r0
	mov	r0, r4
	bl	m0_read_page_end_not_retry
	cmn	r0, #2
	mov	r10, r0
	beq	.L1194
	mov	r0, r10
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L1194:
	movw	r0, #:lower16:.LC193
	movw	r7, #:lower16:.LANCHOR0
	movt	r0, #:upper16:.LC193
	movt	r7, #:upper16:.LANCHOR0
	bl	NAND_Print_DBG
	ldrb	r2, [r7, #211]	@ zero_extendqisi2
	cmp	r2, #0
	beq	.L1174
	mov	r6, #0
	movw	r9, #:lower16:.LC117
	mov	r8, r6
	movt	r9, #:upper16:.LC117
.L1182:
	ldr	r3, [r5, #176]
	mov	r0, r5
	add	r3, r3, #1
	cmp	r3, r2
	movhi	r3, r8
	str	r3, [r5, #176]
	bl	m2_set_readretry
	subs	r10, r0, #0
	bne	.L1195
	mov	r0, r4
	bl	m0_read_page_start
	mov	fp, r0
	mov	r0, r4
	bl	m0_read_page_end_not_retry
	ldr	lr, [r4, #20]
	orr	r0, fp, r0
	mov	ip, r10
	sub	r3, lr, #1
	add	r1, lr, #9
.L1179:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r2, #255
	addeq	ip, ip, #1
	cmp	r3, r1
	bne	.L1179
	cmp	ip, #8
	bls	.L1180
	ldrb	r3, [lr]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L1196
.L1180:
	cmp	r0, #0
	cmpne	r0, #10
	beq	.L1197
.L1183:
	ldrb	r2, [r7, #211]	@ zero_extendqisi2
	add	r6, r6, #1
	cmp	r2, r6
	bgt	.L1182
	mov	r10, r0
.L1174:
	mov	r3, #0
	mov	r0, r5
	str	r3, [r5, #176]
	bl	m2_set_readretry
	mov	r0, r10
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L1196:
	ldr	r1, [r5, #248]
	mov	r0, r9
	ldr	lr, [r5, #176]
	ldr	ip, [r4, #8]
	ldr	r3, [r4, #4]
	ldr	r2, [r5, #20]
	ldr	r1, [r1, #8]
	stm	sp, {ip, lr}
	bl	NAND_Print
	mvn	r0, #1
	b	.L1183
.L1195:
	mov	r3, #0
	mov	r0, r5
	str	r3, [r5, #176]
	bl	m2_set_readretry
	b	.L1174
.L1197:
	ldrb	ip, [lr, #4]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC194
	ldrb	r3, [lr, #3]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC194
	ldrb	r2, [lr, #2]	@ zero_extendqisi2
	mov	r10, #10
	ldrb	r1, [lr, #1]	@ zero_extendqisi2
	str	ip, [sp]
	bl	NAND_Print_DBG
	ldr	r2, [r5, #248]
	movw	r0, #:lower16:.LC195
	ldr	lr, [r4, #8]
	movt	r0, #:upper16:.LC195
	ldr	ip, [r5, #176]
	ldr	r1, [r2, #8]
	ldr	r3, [r4, #4]
	ldr	r2, [r5, #20]
	str	lr, [sp]
	str	ip, [sp, #4]
	bl	NAND_Print
	b	.L1174
	.size	m2_read_page_end, .-m2_read_page_end
	.align	2
	.global	m2_special_init
	.syntax unified
	.arm
	.type	m2_special_init, %function
m2_special_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r4, [r3, #8]
	cmp	r4, #0
	beq	.L1199
	mov	r5, #0
.L1200:
	mov	r0, r4
	bl	m2_readretry_init
	ldr	r4, [r4]
	orr	r5, r5, r0
	cmp	r4, #0
	bne	.L1200
	cmp	r5, #0
	beq	.L1199
	movw	r1, #:lower16:.LANCHOR0
	movw	r0, #:lower16:.LC197
	movt	r1, #:upper16:.LANCHOR0
	movt	r0, #:upper16:.LC197
	ldrb	r3, [r1, #212]	@ zero_extendqisi2
	ldrb	r2, [r1, #211]	@ zero_extendqisi2
	ldrb	r1, [r1, #210]	@ zero_extendqisi2
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L1199:
	movw	r1, #:lower16:.LANCHOR0
	movw	ip, #:lower16:function_read_page_end
	movt	r1, #:upper16:.LANCHOR0
	movw	lr, #:lower16:m2_read_page_end
	movw	r0, #:lower16:.LC196
	ldrb	r3, [r1, #212]	@ zero_extendqisi2
	ldrb	r2, [r1, #211]	@ zero_extendqisi2
	movt	ip, #:upper16:function_read_page_end
	movt	lr, #:upper16:m2_read_page_end
	ldrb	r1, [r1, #210]	@ zero_extendqisi2
	mov	r5, #0
	movt	r0, #:upper16:.LC196
	str	lr, [ip]
	bl	NAND_Print_DBG
	mov	r0, r5
	pop	{r4, r5, r6, pc}
	.size	m2_special_init, .-m2_special_init
	.align	2
	.global	m2_special_exit
	.syntax unified
	.arm
	.type	m2_special_exit, %function
m2_special_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r4, [r3, #8]
	cmp	r4, #0
	beq	.L1211
	movw	r5, #:lower16:.LC191
	mov	r6, #0
	movt	r5, #:upper16:.LC191
.L1210:
	mov	r0, r5
	bl	NAND_Print_DBG
	mov	r0, r4
	bl	m2_setdefaultparam
	str	r6, [r4, #176]
	ldr	r4, [r4]
	cmp	r4, #0
	bne	.L1210
.L1211:
	movw	r0, #:lower16:.LC198
	movt	r0, #:upper16:.LC198
	bl	NAND_Print_DBG
	mov	r0, #0
	pop	{r4, r5, r6, pc}
	.size	m2_special_exit, .-m2_special_exit
	.align	2
	.global	m2_is_lsb_page
	.syntax unified
	.arm
	.type	m2_is_lsb_page, %function
m2_is_lsb_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r0, #1
	bls	.L1217
	movw	r3, #:lower16:g_nsi
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r3, [r3, #8]
	ldr	r3, [r3, #252]
	ldr	r3, [r3, #16]
	sub	r2, r3, #2
	cmp	r0, r2
	beq	.L1219
	sub	r3, r3, #1
	cmp	r0, r3
	beq	.L1219
	and	r0, r0, #3
	sub	r0, r0, #2
	cmp	r0, #1
	movhi	r0, #0
	movls	r0, #1
	bx	lr
.L1219:
	mov	r0, #0
	bx	lr
.L1217:
	mov	r0, #1
	bx	lr
	.size	m2_is_lsb_page, .-m2_is_lsb_page
	.align	2
	.global	m4_0x40_is_lsb_page
	.syntax unified
	.arm
	.type	m4_0x40_is_lsb_page, %function
m4_0x40_is_lsb_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	b	m2_is_lsb_page
	.size	m4_0x40_is_lsb_page, .-m4_0x40_is_lsb_page
	.align	2
	.global	m3_is_lsb_page
	.syntax unified
	.arm
	.type	m3_is_lsb_page, %function
m3_is_lsb_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	b	m2_is_lsb_page
	.size	m3_is_lsb_page, .-m3_is_lsb_page
	.align	2
	.global	m9_dsp_on
	.syntax unified
	.arm
	.type	m9_dsp_on, %function
m9_dsp_on:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r1, [r0]
	ldr	r0, [r3]
	bl	nci_get_from_nsi
	mov	r4, r0
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	mov	r2, #0
	mov	r1, #38
	mov	r0, r4
	bl	set_one_cmd
	mov	r0, r4
	bl	nand_disable_chip
	mov	r0, #0
	pop	{r4, pc}
	.size	m9_dsp_on, .-m9_dsp_on
	.align	2
	.global	m9_cmd25
	.syntax unified
	.arm
	.type	m9_cmd25, %function
m9_cmd25:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r1, [r0]
	ldr	r0, [r3]
	bl	nci_get_from_nsi
	mov	r4, r0
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	mov	r2, #0
	mov	r1, #37
	mov	r0, r4
	bl	set_one_cmd
	mov	r0, r4
	bl	nand_disable_chip
	mov	r0, #0
	pop	{r4, pc}
	.size	m9_cmd25, .-m9_cmd25
	.align	2
	.global	m9_set_lmflgfix_next
	.syntax unified
	.arm
	.type	m9_set_lmflgfix_next, %function
m9_set_lmflgfix_next:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, r7, r8, lr}
	movt	r3, #:upper16:g_nsi
	sub	sp, sp, #24
	mov	r6, r1
	add	r7, sp, #22
	ldr	r1, [r0]
	ldr	r0, [r3]
	bl	nci_get_from_nsi
	mov	r4, r0
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	mov	r2, #0
	mov	r1, #92
	mov	r0, r4
	bl	set_one_cmd
	mov	r2, #0
	mov	r1, #197
	mov	r0, r4
	bl	set_one_cmd
	mov	ip, #1
	add	r3, sp, #21
	str	ip, [sp, #8]
	mov	r2, ip
	str	ip, [sp, #4]
	mov	lr, #0
	strb	ip, [sp, #22]
	mov	r0, r4
	mov	ip, #85
	add	r1, sp, #23
	str	r7, [sp]
	strb	lr, [sp, #21]
	strb	ip, [sp, #23]
	bl	set_cmd_with_nand_bus
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	ldrb	r3, [r3, #236]	@ zero_extendqisi2
	cmp	r3, #52
	moveq	r3, #34
	strbeq	r3, [sp, #21]
	beq	.L1228
	cmp	r3, #53
	moveq	r3, #37
	movne	r3, #35
	strb	r3, [sp, #21]
.L1228:
	mov	r5, #1
	mov	r8, #85
	mov	r2, r5
	add	r3, sp, #21
	add	r1, sp, #23
	str	r7, [sp]
	str	r5, [sp, #8]
	mov	r0, r4
	str	r5, [sp, #4]
	strb	r6, [sp, #22]
	mov	r6, #0
	strb	r8, [sp, #23]
	bl	set_cmd_with_nand_bus
	mov	r2, r5
	add	r3, sp, #21
	add	r1, sp, #23
	str	r5, [sp, #8]
	str	r5, [sp, #4]
	mov	r0, r4
	str	r7, [sp]
	strb	r8, [sp, #23]
	strb	r6, [sp, #21]
	strb	r6, [sp, #22]
	bl	set_cmd_with_nand_bus
	mov	r0, r4
	bl	nand_disable_chip
	mov	r0, r6
	add	sp, sp, #24
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
	.size	m9_set_lmflgfix_next, .-m9_set_lmflgfix_next
	.align	2
	.global	m9_readretry_init
	.syntax unified
	.arm
	.type	m9_readretry_init, %function
m9_readretry_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #252]
	movw	r2, #:lower16:.LANCHOR0
	push	{r4, lr}
	mov	r4, #0
	movt	r2, #:upper16:.LANCHOR0
	str	r4, [r0, #176]
	ldr	r3, [r3, #40]
	ubfx	r1, r3, #16, #8
	strb	r3, [r2, #238]
	sub	r0, r1, #50
	lsr	r3, r3, #8
	cmp	r0, #3
	strb	r1, [r2, #236]
	strb	r3, [r2, #237]
	bhi	.L1235
	movw	r0, #:lower16:.LC200
	uxtb	r1, r3
	movt	r0, #:upper16:.LC200
	bl	NAND_Print
	mov	r0, r4
	pop	{r4, pc}
.L1235:
	movw	r0, #:lower16:.LC199
	movt	r0, #:upper16:.LC199
	bl	NAND_Print
	mvn	r0, #98
	pop	{r4, pc}
	.size	m9_readretry_init, .-m9_readretry_init
	.align	2
	.global	m9_readretry_exit
	.syntax unified
	.arm
	.type	m9_readretry_exit, %function
m9_readretry_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.size	m9_readretry_exit, .-m9_readretry_exit
	.align	2
	.global	m9_read_retry_clock_save
	.syntax unified
	.arm
	.type	m9_read_retry_clock_save, %function
m9_read_retry_clock_save:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #248]
	movw	r1, #:lower16:.LANCHOR0
	movt	r1, #:upper16:.LANCHOR0
	push	{r4, lr}
	add	r2, r1, #240
	mov	r4, r0
	add	r1, r1, #244
	ldr	r0, [r3, #8]
	bl	NAND_GetClk
	ldr	r3, [r4, #248]
	mov	r2, #20
	mov	r1, #10
	ldr	r0, [r3, #8]
	bl	NAND_SetClk
	mov	r0, #0
	pop	{r4, pc}
	.size	m9_read_retry_clock_save, .-m9_read_retry_clock_save
	.align	2
	.global	m9_read_retry_clock_recover
	.syntax unified
	.arm
	.type	m9_read_retry_clock_recover, %function
m9_read_retry_clock_recover:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r1, [r0, #248]
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	push	{r4, lr}
	ldr	r2, [r3, #240]
	ldr	r0, [r1, #8]
	ldr	r1, [r3, #244]
	bl	NAND_SetClk
	mov	r0, #0
	pop	{r4, pc}
	.size	m9_read_retry_clock_recover, .-m9_read_retry_clock_recover
	.align	2
	.global	m9_vender_set_param
	.syntax unified
	.arm
	.type	m9_vender_set_param, %function
m9_vender_set_param:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	str	lr, [sp, #-4]!
	mov	ip, #1
	sub	sp, sp, #20
	mov	lr, #4
	mov	r3, r2
	mov	r2, ip
	str	r1, [sp]
	str	ip, [sp, #8]
	str	lr, [sp, #4]
	ldr	r1, .L1243
	bl	set_cmd_with_nand_bus
	add	sp, sp, #20
	@ sp needed
	ldr	pc, [sp], #4
.L1244:
	.align	2
.L1243:
	.word	.LANCHOR1+532
	.size	m9_vender_set_param, .-m9_vender_set_param
	.align	2
	.global	m9_set_readretry
	.syntax unified
	.arm
	.type	m9_set_readretry, %function
m9_set_readretry:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, lr}
	mov	r4, r0
	sub	sp, sp, #20
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	ldrb	r3, [r3, #236]	@ zero_extendqisi2
	cmp	r3, #52
	beq	.L1250
	cmp	r3, #53
	ldr	r3, [r4, #176]
	ldreq	r1, .L1251
	ldrne	r1, .L1251+4
	add	r1, r1, r3, lsl #2
.L1247:
	ldr	r3, .L1251+8
	mov	r2, #1
	mov	r0, #4
	str	r1, [sp]
	str	r2, [sp, #8]
	sub	r1, r3, #380
	str	r0, [sp, #4]
	mov	r0, r4
	bl	set_cmd_with_nand_bus
	mov	r2, #0
	mov	r5, r0
	mov	r1, #93
	mov	r0, r4
	bl	set_one_cmd
	mov	r0, r4
	bl	nand_disable_chip
	mov	r0, r5
	add	sp, sp, #20
	@ sp needed
	pop	{r4, r5, pc}
.L1250:
	ldr	r1, .L1251+12
	ldr	r3, [r4, #176]
	add	r1, r1, r3, lsl #2
	b	.L1247
.L1252:
	.align	2
.L1251:
	.word	.LANCHOR1+664
	.word	.LANCHOR1+788
	.word	.LANCHOR1+912
	.word	.LANCHOR1+536
	.size	m9_set_readretry, .-m9_set_readretry
	.align	2
	.global	m9_exit_readretry
	.syntax unified
	.arm
	.type	m9_exit_readretry, %function
m9_exit_readretry:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, lr}
	sub	sp, sp, #28
	mov	r4, r0
	mov	r3, #0
	add	r5, sp, #24
	str	r3, [r5, #-4]!
	bl	nand_enable_chip
	ldr	r0, [r4, #248]
	bl	ndfc_disable_randomize
	mov	r0, r4
	mov	r2, #1
	mov	r1, #255
	bl	set_one_cmd
	movw	r1, #:lower16:.LANCHOR1
	mov	r2, #1
	mov	ip, #4
	movt	r1, #:upper16:.LANCHOR1
	add	r3, r1, #912
	str	r5, [sp]
	add	r1, r1, #532
	str	r2, [sp, #8]
	str	ip, [sp, #4]
	mov	r0, r4
	bl	set_cmd_with_nand_bus
	mov	r5, r0
	mov	r0, r4
	bl	nand_disable_chip
	mov	r0, r5
	add	sp, sp, #28
	@ sp needed
	pop	{r4, r5, pc}
	.size	m9_exit_readretry, .-m9_exit_readretry
	.align	2
	.global	m9_read_page_end
	.syntax unified
	.arm
	.type	m9_read_page_end, %function
m9_read_page_end:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r1, [r0]
	sub	sp, sp, #8
	mov	r5, r0
	ldr	r0, [r3]
	bl	nci_get_from_nsi
	mov	r6, r0
	mov	r0, r5
	bl	m0_read_page_end_not_retry
	cmn	r0, #2
	mov	r4, r0
	beq	.L1337
	mov	r0, r4
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L1337:
	movw	r0, #:lower16:.LC201
	movw	r8, #:lower16:.LANCHOR0
	movt	r0, #:upper16:.LC201
	movt	r8, #:upper16:.LANCHOR0
	bl	NAND_Print
	ldrb	r3, [r8, #237]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L1265
	movw	r10, #:lower16:.LC117
	movw	r9, #:lower16:.LC202
	movt	r10, #:upper16:.LC117
	movt	r9, #:upper16:.LC202
	mov	r7, #0
	b	.L1266
.L1339:
	mov	r0, r9
	bl	NAND_Print_DBG
.L1259:
	ldrb	r3, [r8, #237]	@ zero_extendqisi2
	add	r7, r7, #1
	cmp	r3, r7
	ble	.L1338
.L1266:
	str	r7, [r6, #176]
	mov	r0, r6
	ldrh	r1, [r5, #8]
	bl	m9_set_readretry
	subs	r4, r0, #0
	bne	.L1339
	mov	r0, r5
	bl	m0_read_page_start
	mov	r4, r0
	mov	r0, r5
	bl	m0_read_page_end_not_retry
	ldr	ip, [r5, #20]
	orr	r4, r4, r0
	mov	r0, #0
	sub	r3, ip, #1
	add	r1, ip, #9
.L1261:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r2, #255
	addeq	r0, r0, #1
	cmp	r1, r3
	bne	.L1261
	cmp	r0, #8
	bls	.L1262
	ldrb	r3, [ip]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L1340
.L1262:
	cmp	r4, #0
	cmpne	r4, #10
	bne	.L1259
.L1305:
	ldr	r2, [r6, #248]
	cmp	r4, #0
	ldr	lr, [r5, #8]
	movweq	r0, #:lower16:.LC203
	ldr	ip, [r6, #176]
	movwne	r0, #:lower16:.LC204
	ldr	r1, [r2, #8]
	movteq	r0, #:upper16:.LC203
	ldreq	r3, [r5, #4]
	movtne	r0, #:upper16:.LC204
	ldrne	r3, [r5, #4]
	mov	r4, #10
	ldr	r2, [r6, #20]
	str	lr, [sp]
	str	ip, [sp, #4]
	bl	NAND_Print_DBG
.L1265:
	mov	r3, #0
	mov	r0, r6
	str	r3, [r6, #176]
	bl	m9_exit_readretry
	mov	r0, r4
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L1340:
	ldr	r1, [r6, #248]
	mov	r0, r10
	ldr	lr, [r6, #176]
	add	r7, r7, #1
	ldr	ip, [r5, #8]
	mvn	r4, #1
	ldr	r3, [r5, #4]
	ldr	r2, [r6, #20]
	ldr	r1, [r1, #8]
	stm	sp, {ip, lr}
	bl	NAND_Print_DBG
	ldrb	r3, [r8, #237]	@ zero_extendqisi2
	cmp	r3, r7
	bgt	.L1266
.L1338:
	ldrb	r2, [r8, #236]	@ zero_extendqisi2
	cmp	r2, #51
	beq	.L1267
	cmp	r3, #0
	beq	.L1265
	movw	r10, #:lower16:.LC117
	movw	r9, #:lower16:.LC202
	movt	r10, #:upper16:.LC117
	movt	r9, #:upper16:.LC202
	mov	r7, #0
	b	.L1278
.L1342:
	mov	r0, r9
	bl	NAND_Print_DBG
.L1272:
	ldrb	r3, [r8, #237]	@ zero_extendqisi2
	add	r7, r7, #1
	cmp	r3, r7
	ble	.L1341
.L1278:
	str	r7, [r6, #176]
	mov	r0, r6
	ldrh	r1, [r5, #8]
	bl	m9_set_readretry
	subs	r4, r0, #0
	bne	.L1342
	mov	r0, r5
	bl	m9_dsp_on
	mov	r0, r5
	bl	m0_read_page_start
	mov	r4, r0
	mov	r0, r5
	bl	m0_read_page_end_not_retry
	ldr	ip, [r5, #20]
	orr	r4, r4, r0
	mov	r0, #0
	sub	r3, ip, #1
	add	r1, ip, #9
.L1274:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r2, #255
	addeq	r0, r0, #1
	cmp	r1, r3
	bne	.L1274
	cmp	r0, #8
	bls	.L1275
	ldrb	r3, [ip]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L1343
.L1275:
	cmp	r4, #0
	cmpne	r4, #10
	bne	.L1272
	b	.L1305
.L1343:
	ldr	r1, [r6, #248]
	mov	r0, r10
	ldr	lr, [r6, #176]
	mvn	r4, #1
	ldr	ip, [r5, #8]
	ldr	r3, [r5, #4]
	ldr	r2, [r6, #20]
	ldr	r1, [r1, #8]
	stm	sp, {ip, lr}
	bl	NAND_Print_DBG
	b	.L1272
.L1267:
	cmp	r3, #0
	beq	.L1265
	movw	r10, #:lower16:.LC117
	movw	r9, #:lower16:.LC202
	movt	r10, #:upper16:.LC117
	movt	r9, #:upper16:.LC202
	mov	r7, #0
.L1301:
	str	r7, [r6, #176]
	mov	r0, r6
	ldrh	r1, [r5, #8]
	bl	m9_set_readretry
	subs	r4, r0, #0
	bne	.L1344
	ldrb	r3, [r8, #236]	@ zero_extendqisi2
	cmp	r3, #52
	beq	.L1345
	cmp	r3, #53
	mov	r0, r5
	moveq	r1, #26
	movne	r1, #192
	bl	m9_set_lmflgfix_next
.L1290:
	mov	r0, r5
	bl	m9_cmd25
	ldrb	r3, [r8, #236]	@ zero_extendqisi2
	sub	r2, r3, #52
	cmp	r3, #50
	cmpne	r2, #1
	bhi	.L1292
	mov	r0, r5
	bl	m9_dsp_on
.L1292:
	mov	r0, r5
	bl	m0_read_page_start
	mov	r4, r0
	mov	r0, r5
	bl	m0_read_page_end_not_retry
	ldrb	r3, [r8, #236]	@ zero_extendqisi2
	orr	r4, r4, r0
	cmp	r3, #52
	beq	.L1346
	cmp	r3, #53
	mov	r0, r5
	moveq	r1, #10
	movne	r1, #64
	bl	m9_set_lmflgfix_next
.L1294:
	ldr	ip, [r5, #20]
	mov	r0, #0
	sub	r3, ip, #1
	add	r1, ip, #9
.L1297:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r2, #255
	addeq	r0, r0, #1
	cmp	r1, r3
	bne	.L1297
	cmp	r0, #8
	bls	.L1298
	ldrb	r3, [ip]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L1347
.L1298:
	cmp	r4, #0
	cmpne	r4, #10
	beq	.L1305
.L1288:
	ldrb	r3, [r8, #237]	@ zero_extendqisi2
	add	r7, r7, #1
	cmp	r3, r7
	bgt	.L1301
	b	.L1265
.L1344:
	mov	r0, r9
	bl	NAND_Print_DBG
	b	.L1288
.L1347:
	ldr	r1, [r6, #248]
	mov	r0, r10
	ldr	lr, [r6, #176]
	mvn	r4, #1
	ldr	ip, [r5, #8]
	ldr	r3, [r5, #4]
	ldr	r2, [r6, #20]
	ldr	r1, [r1, #8]
	stm	sp, {ip, lr}
	bl	NAND_Print_DBG
	b	.L1288
.L1346:
	mov	r1, #194
	mov	r0, r5
	bl	m9_set_lmflgfix_next
	b	.L1294
.L1345:
	mov	r1, #198
	mov	r0, r5
	bl	m9_set_lmflgfix_next
	b	.L1290
.L1341:
	cmp	r3, #0
	beq	.L1265
	movw	r10, #:lower16:.LC117
	movw	r9, #:lower16:.LC202
	movt	r10, #:upper16:.LC117
	movt	r9, #:upper16:.LC202
	mov	r7, #0
	b	.L1286
.L1348:
	mov	r0, r9
	bl	NAND_Print_DBG
.L1280:
	ldrb	r3, [r8, #237]	@ zero_extendqisi2
	add	r7, r7, #1
	cmp	r3, r7
	ble	.L1267
.L1286:
	str	r7, [r6, #176]
	mov	r0, r6
	ldrh	r1, [r5, #8]
	bl	m9_set_readretry
	subs	r4, r0, #0
	bne	.L1348
	mov	r0, r5
	bl	m9_cmd25
	mov	r0, r5
	bl	m0_read_page_start
	mov	r4, r0
	mov	r0, r5
	bl	m0_read_page_end_not_retry
	ldr	ip, [r5, #20]
	orr	r4, r4, r0
	mov	r0, #0
	sub	r3, ip, #1
	add	r1, ip, #9
.L1282:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r2, #255
	addeq	r0, r0, #1
	cmp	r1, r3
	bne	.L1282
	cmp	r0, #8
	bls	.L1283
	ldrb	r3, [ip]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L1349
.L1283:
	cmp	r4, #0
	cmpne	r4, #10
	bne	.L1280
	b	.L1305
.L1349:
	ldr	r1, [r6, #248]
	mov	r0, r10
	ldr	lr, [r6, #176]
	mvn	r4, #1
	ldr	ip, [r5, #8]
	ldr	r3, [r5, #4]
	ldr	r2, [r6, #20]
	ldr	r1, [r1, #8]
	stm	sp, {ip, lr}
	bl	NAND_Print_DBG
	b	.L1280
	.size	m9_read_page_end, .-m9_read_page_end
	.align	2
	.global	m9_special_init
	.syntax unified
	.arm
	.type	m9_special_init, %function
m9_special_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r4, [r3, #8]
	cmp	r4, #0
	beq	.L1351
	mov	r5, #0
.L1352:
	mov	r0, r4
	bl	m9_readretry_init
	ldr	r4, [r4]
	orr	r5, r5, r0
	cmp	r4, #0
	bne	.L1352
	cmp	r5, #0
	beq	.L1351
	movw	r1, #:lower16:.LANCHOR0
	movw	r0, #:lower16:.LC206
	movt	r1, #:upper16:.LANCHOR0
	movt	r0, #:upper16:.LC206
	ldrb	r3, [r1, #238]	@ zero_extendqisi2
	ldrb	r2, [r1, #237]	@ zero_extendqisi2
	ldrb	r1, [r1, #236]	@ zero_extendqisi2
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L1351:
	movw	r1, #:lower16:.LANCHOR0
	movw	ip, #:lower16:function_read_page_end
	movt	r1, #:upper16:.LANCHOR0
	movw	lr, #:lower16:m9_read_page_end
	movw	r0, #:lower16:.LC205
	ldrb	r3, [r1, #238]	@ zero_extendqisi2
	ldrb	r2, [r1, #237]	@ zero_extendqisi2
	movt	ip, #:upper16:function_read_page_end
	movt	lr, #:upper16:m9_read_page_end
	ldrb	r1, [r1, #236]	@ zero_extendqisi2
	mov	r5, #0
	movt	r0, #:upper16:.LC205
	str	lr, [ip]
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, pc}
	.size	m9_special_init, .-m9_special_init
	.align	2
	.global	m9_special_exit
	.syntax unified
	.arm
	.type	m9_special_exit, %function
m9_special_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:g_nsi
	ldr	r3, [r3]
	ldr	r4, [r3, #8]
	cmp	r4, #0
	movne	r6, #0
	bne	.L1364
	b	.L1365
.L1363:
	ldr	r4, [r4]
	cmp	r4, #0
	beq	.L1365
.L1364:
	ldr	r3, [r4, #220]
	cmp	r3, #0
	beq	.L1363
	str	r6, [r4, #196]
	mov	r0, r4
	bl	_setup_nand_toggle_vendor_specific_feature
	subs	r5, r0, #0
	beq	.L1363
	movw	r0, #:lower16:.LC208
	movt	r0, #:upper16:.LC208
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L1365:
	movw	r0, #:lower16:.LC207
	movt	r0, #:upper16:.LC207
	bl	NAND_Print
	mov	r0, #0
	pop	{r4, r5, r6, pc}
	.size	m9_special_exit, .-m9_special_exit
	.align	2
	.global	m9_check_bad_block_first_burn
	.syntax unified
	.arm
	.type	m9_check_bad_block_first_burn, %function
m9_check_bad_block_first_burn:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:g_nsi
	push	{r4, r5, r6, r7, r8, r9, lr}
	movt	r3, #:upper16:g_nsi
	mov	r7, r0
	sub	sp, sp, #12
	ldr	r1, [r0]
	ldr	r0, [r3]
	bl	nci_get_from_nsi
	ldr	r4, [r0, #248]
	mov	r5, r0
	ldr	r2, [r0, #20]
	ldr	ip, [r7, #4]
	ldr	r3, [r4, #12]
	ldr	lr, [r0, #24]
	cmp	r2, r3
	bcs	.L1376
	cmp	ip, lr
	bcs	.L1376
	mov	r0, r4
	add	r9, r4, #28
	bl	ndfc_disable_randomize
	mov	r0, r5
	mov	r6, #1
	bl	nand_read_chip_status_ready
	mov	r0, r5
	mov	r8, #0
	bl	nand_enable_chip
	mov	r0, r9
	bl	ndfc_clean_cmd_seq
	mov	r3, #128
	mov	r2, #162
	str	r8, [r4, #28]
	str	r8, [r4, #52]
	str	r8, [r4, #112]
	str	r2, [r4, #44]
	str	r6, [r4, #40]
	ldmib	r7, {r1, r2}
	str	r6, [r4, #48]
	str	r6, [r4, #100]
	str	r6, [r4, #108]
	str	r3, [r4, #104]
	ldr	r0, [r5, #36]
	bl	get_row_addr
	add	r3, r4, #116
	mov	r1, #5
	mov	r2, r0
	str	r1, [r4, #132]
	mov	r0, r8
	str	r3, [sp]
	mov	r1, #2
	mov	r3, #3
	bl	fill_cmd_addr
	mov	r3, #16
	mov	r1, r9
	str	r3, [r4, #164]
	str	r6, [r4, #160]
	str	r6, [r4, #168]
	str	r6, [r4, #172]
	ldr	r0, [r5, #248]
	bl	ndfc_execute_cmd
	mov	r0, r5
	bl	nand_read_chip_status_ready
	mov	r4, r0
	mov	r0, r5
	bl	nand_disable_chip
	mov	r0, r4
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
.L1376:
	movw	r0, #:lower16:.LC129
	ldr	r1, [r4, #8]
	movt	r0, #:upper16:.LC129
	stm	sp, {ip, lr}
	bl	NAND_Print
	mvn	r0, #9
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
	.size	m9_check_bad_block_first_burn, .-m9_check_bad_block_first_burn
	.align	2
	.global	add_free_list
	.syntax unified
	.arm
	.type	add_free_list, %function
add_free_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	add	r0, r0, #152
	b	.L1381
.L1383:
	ldrh	r2, [r3, #6]
	ldrh	ip, [r1, #6]
	cmp	ip, r2
	bls	.L1385
	mov	r0, r3
.L1381:
	ldr	r3, [r0, #20]
	cmp	r3, #0
	bne	.L1383
	str	r1, [r0, #20]
	str	r0, [r1, #24]
	mov	r0, #0
	str	r3, [r1, #20]
	bx	lr
.L1385:
	str	r1, [r3, #24]
	str	r3, [r1, #20]
	str	r1, [r0, #20]
	str	r0, [r1, #24]
	mov	r0, #0
	bx	lr
	.size	add_free_list, .-add_free_list
	.align	2
	.global	del_free_list
	.syntax unified
	.arm
	.type	del_free_list, %function
del_free_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, r0
	ldr	r0, [r0, #172]
	cmp	r0, #0
	bxeq	lr
	ldr	r2, [r0, #20]
	str	r2, [r3, #172]
	ldr	r2, [r0, #20]
	str	r2, [r3, #172]
	ldr	r2, [r0, #20]
	cmp	r2, #0
	addne	r3, r3, #152
	strne	r3, [r2, #24]
	bx	lr
	.size	del_free_list, .-del_free_list
	.align	2
	.global	del_free_list_by_block
	.syntax unified
	.arm
	.type	del_free_list_by_block, %function
del_free_list_by_block:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #172]
	cmp	r0, #0
	bxeq	lr
	cmp	r0, r1
	bne	.L1397
	b	.L1395
.L1399:
	cmp	r1, r0
	beq	.L1395
.L1397:
	ldr	r0, [r0, #20]
	cmp	r0, #0
	bne	.L1399
	bx	lr
.L1395:
	ldr	r3, [r0, #20]
	ldr	r2, [r0, #24]
	str	r3, [r2, #20]
	ldr	r3, [r0, #20]
	cmp	r3, #0
	strne	r2, [r3, #24]
	mov	r3, #0
	str	r3, [r0, #24]
	str	r3, [r0, #20]
	bx	lr
	.size	del_free_list_by_block, .-del_free_list_by_block
	.align	2
	.global	is_block_in_free_list
	.syntax unified
	.arm
	.type	is_block_in_free_list, %function
is_block_in_free_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #172]
	cmp	r0, #0
	bxeq	lr
	cmp	r0, r1
	bne	.L1411
	b	.L1415
.L1412:
	cmp	r1, r0
	beq	.L1415
.L1411:
	ldr	r0, [r0, #20]
	cmp	r0, #0
	bne	.L1412
	bx	lr
.L1415:
	mov	r0, #1
	bx	lr
	.size	is_block_in_free_list, .-is_block_in_free_list
	.align	2
	.global	print_free_list
	.syntax unified
	.arm
	.type	print_free_list, %function
print_free_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #356]
	cmp	r3, #0
	bxeq	lr
	b	print_free_list.part.6
	.size	print_free_list, .-print_free_list
	.align	2
	.global	add_block_invalid_list
	.syntax unified
	.arm
	.type	add_block_invalid_list, %function
add_block_invalid_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	add	r0, r0, #188
	b	.L1419
.L1421:
	ldrh	r2, [r3, #4]
	ldrh	ip, [r1, #4]
	cmp	ip, r2
	bcs	.L1423
	mov	r0, r3
.L1419:
	ldr	r3, [r0, #12]
	cmp	r3, #0
	bne	.L1421
	str	r1, [r0, #12]
	str	r0, [r1, #16]
	mov	r0, #0
	str	r3, [r1, #12]
	bx	lr
.L1423:
	str	r1, [r3, #16]
	str	r3, [r1, #12]
	str	r1, [r0, #12]
	str	r0, [r1, #16]
	mov	r0, #0
	bx	lr
	.size	add_block_invalid_list, .-add_block_invalid_list
	.align	2
	.global	del_block_invalid_list
	.syntax unified
	.arm
	.type	del_block_invalid_list, %function
del_block_invalid_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, r0
	ldr	r0, [r0, #200]
	cmp	r0, #0
	bxeq	lr
	ldr	r2, [r0, #12]
	str	r2, [r3, #200]
	ldr	r2, [r0, #12]
	cmp	r2, #0
	addne	r3, r3, #188
	strne	r3, [r2, #16]
	bx	lr
	.size	del_block_invalid_list, .-del_block_invalid_list
	.align	2
	.global	del_block_invalid_list_by_block
	.syntax unified
	.arm
	.type	del_block_invalid_list_by_block, %function
del_block_invalid_list_by_block:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #200]
	cmp	r0, #0
	bxeq	lr
	cmp	r0, r1
	bne	.L1435
	b	.L1433
.L1437:
	cmp	r1, r0
	beq	.L1433
.L1435:
	ldr	r0, [r0, #12]
	cmp	r0, #0
	bne	.L1437
	bx	lr
.L1433:
	ldr	r3, [r0, #12]
	ldr	r2, [r0, #16]
	str	r3, [r2, #12]
	ldr	r3, [r0, #12]
	cmp	r3, #0
	strne	r2, [r3, #16]
	mov	r3, #0
	str	r3, [r0, #16]
	str	r3, [r0, #12]
	bx	lr
	.size	del_block_invalid_list_by_block, .-del_block_invalid_list_by_block
	.align	2
	.global	is_block_in_invalid_list
	.syntax unified
	.arm
	.type	is_block_in_invalid_list, %function
is_block_in_invalid_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #200]
	cmp	r0, #0
	bxeq	lr
	cmp	r0, r1
	bne	.L1449
	b	.L1453
.L1450:
	cmp	r1, r0
	beq	.L1453
.L1449:
	ldr	r0, [r0, #12]
	cmp	r0, #0
	bne	.L1450
	bx	lr
.L1453:
	mov	r0, #1
	bx	lr
	.size	is_block_in_invalid_list, .-is_block_in_invalid_list
	.align	2
	.global	print_block_invalid_list
	.syntax unified
	.arm
	.type	print_block_invalid_list, %function
print_block_invalid_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #356]
	cmp	r3, #0
	bxeq	lr
	b	print_block_invalid_list.part.7
	.size	print_block_invalid_list, .-print_block_invalid_list
	.align	2
	.global	add_block_count_list
	.syntax unified
	.arm
	.type	add_block_count_list, %function
add_block_count_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	add	r0, r0, #224
	b	.L1457
.L1459:
	ldr	ip, [r3, #8]
	ldr	r2, [r1, #8]
	cmp	ip, r2
	bge	.L1461
	mov	r0, r3
.L1457:
	ldr	r3, [r0, #28]
	cmp	r3, #0
	bne	.L1459
	str	r1, [r0, #28]
	str	r0, [r1, #32]
	mov	r0, #0
	str	r3, [r1, #28]
	bx	lr
.L1461:
	str	r1, [r3, #32]
	str	r3, [r1, #28]
	str	r1, [r0, #28]
	str	r0, [r1, #32]
	mov	r0, #0
	bx	lr
	.size	add_block_count_list, .-add_block_count_list
	.align	2
	.global	del_block_count_list
	.syntax unified
	.arm
	.type	del_block_count_list, %function
del_block_count_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r2, [r0, #252]
	mov	r3, r0
	add	r0, r0, #224
	cmp	r2, #0
	ldrne	r2, [r2, #28]
	strne	r2, [r3, #252]
	bx	lr
	.size	del_block_count_list, .-del_block_count_list
	.align	2
	.global	del_block_count_by_block
	.syntax unified
	.arm
	.type	del_block_count_by_block, %function
del_block_count_by_block:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #252]
	cmp	r0, #0
	bxeq	lr
	cmp	r0, r1
	bne	.L1471
	b	.L1469
.L1473:
	cmp	r1, r0
	beq	.L1469
.L1471:
	ldr	r0, [r0, #28]
	cmp	r0, #0
	bne	.L1473
	bx	lr
.L1469:
	ldr	r3, [r0, #28]
	ldr	r2, [r0, #32]
	str	r3, [r2, #28]
	ldr	r3, [r0, #28]
	cmp	r3, #0
	strne	r2, [r3, #32]
	mov	r3, #0
	str	r3, [r0, #32]
	str	r3, [r0, #28]
	bx	lr
	.size	del_block_count_by_block, .-del_block_count_by_block
	.align	2
	.global	print_block_count_list
	.syntax unified
	.arm
	.type	print_block_count_list, %function
print_block_count_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #356]
	cmp	r3, #0
	bxeq	lr
	push	{r4, r5, r6, lr}
	mov	r4, r0
	movw	r0, #:lower16:.LC209
	movt	r0, #:upper16:.LC209
	bl	NAND_Print
	ldr	r4, [r4, #252]
	cmp	r4, #0
	popeq	{r4, r5, r6, pc}
	movw	r6, #:lower16:.LC210
	movw	r5, #:lower16:.LC211
	movt	r6, #:upper16:.LC210
	movt	r5, #:upper16:.LC211
.L1485:
	ldrh	r1, [r4]
	mov	r0, r6
	bl	NAND_Print
	ldr	r1, [r4, #8]
	mov	r0, r5
	bl	NAND_Print
	ldr	r4, [r4, #28]
	cmp	r4, #0
	bne	.L1485
	pop	{r4, r5, r6, pc}
	.size	print_block_count_list, .-print_block_count_list
	.align	2
	.global	put_phy_block_to_free_list
	.syntax unified
	.arm
	.type	put_phy_block_to_free_list, %function
put_phy_block_to_free_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	mov	r3, #0
	str	lr, [sp, #-4]!
	add	r2, r0, #152
	mov	lr, r0
	str	r3, [r1, #20]
	str	r3, [r1, #24]
	b	.L1493
.L1495:
	ldrh	r0, [r3, #6]
	ldrh	ip, [r1, #6]
	cmp	ip, r0
	bls	.L1498
	mov	r2, r3
.L1493:
	ldr	r3, [r2, #20]
	cmp	r3, #0
	bne	.L1495
	str	r1, [r2, #20]
	str	r3, [r1, #20]
	str	r2, [r1, #24]
.L1494:
	ldrh	r3, [lr, #22]
	mov	r0, #0
	add	r3, r3, #1
	strh	r3, [lr, #22]	@ movhi
	ldr	pc, [sp], #4
.L1498:
	str	r1, [r3, #24]
	str	r3, [r1, #20]
	str	r1, [r2, #20]
	str	r2, [r1, #24]
	b	.L1494
	.size	put_phy_block_to_free_list, .-put_phy_block_to_free_list
	.align	2
	.global	out_phy_block_from_free_list
	.syntax unified
	.arm
	.type	out_phy_block_from_free_list, %function
out_phy_block_from_free_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #172]
	cmp	r3, #0
	beq	.L1508
	ldr	r2, [r3, #20]
	str	r2, [r0, #172]
	ldr	r2, [r3, #20]
	str	r2, [r0, #172]
	ldr	r2, [r3, #20]
	cmp	r2, #0
	addne	r1, r0, #152
	strne	r1, [r2, #24]
	mov	r1, #0
	ldrh	r2, [r0, #22]
	str	r1, [r3, #20]
	str	r1, [r3, #24]
	cmp	r2, r1
	subne	r2, r2, #1
	strhne	r2, [r0, #22]	@ movhi
.L1508:
	mov	r0, r3
	bx	lr
	.size	out_phy_block_from_free_list, .-out_phy_block_from_free_list
	.align	2
	.global	get_max_erase_time_from_free_list
	.syntax unified
	.arm
	.type	get_max_erase_time_from_free_list, %function
get_max_erase_time_from_free_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #172]
	cmp	r3, #0
	beq	.L1513
	mov	r0, #0
	str	lr, [sp, #-4]!
	mov	ip, r0
.L1512:
	ldrh	r2, [r3, #6]
	cmp	r2, r0
	movcs	lr, #1
	movcc	lr, #0
	cmp	r2, r1
	movcs	lr, #0
	cmp	lr, #0
	movne	ip, r3
	ldr	r3, [r3, #20]
	movne	r0, r2
	cmp	r3, #0
	bne	.L1512
	cmp	ip, #0
	beq	.L1515
	ldrh	r0, [ip, #6]
	ldr	pc, [sp], #4
.L1515:
	movw	r0, #65535
	ldr	pc, [sp], #4
.L1513:
	movw	r0, #65535
	bx	lr
	.size	get_max_erase_time_from_free_list, .-get_max_erase_time_from_free_list
	.align	2
	.global	get_block_max_erase_time_from_free_list
	.syntax unified
	.arm
	.type	get_block_max_erase_time_from_free_list, %function
get_block_max_erase_time_from_free_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #172]
	cmp	r3, #0
	beq	.L1522
	mov	ip, #0
	str	lr, [sp, #-4]!
	mov	r0, ip
.L1521:
	ldrh	r2, [r3, #6]
	cmp	r2, ip
	movcs	lr, #1
	movcc	lr, #0
	cmp	r2, r1
	movcs	lr, #0
	cmp	lr, #0
	movne	r0, r3
	ldr	r3, [r3, #20]
	movne	ip, r2
	cmp	r3, #0
	bne	.L1521
	ldr	pc, [sp], #4
.L1522:
	mov	r0, r3
	bx	lr
	.size	get_block_max_erase_time_from_free_list, .-get_block_max_erase_time_from_free_list
	.align	2
	.global	get_block_max_erase_time_from_invalid_list
	.syntax unified
	.arm
	.type	get_block_max_erase_time_from_invalid_list, %function
get_block_max_erase_time_from_invalid_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #200]
	cmp	r3, #0
	beq	.L1531
	mov	ip, #0
	str	lr, [sp, #-4]!
	mov	r0, ip
.L1530:
	ldrh	r2, [r3, #6]
	cmp	r2, ip
	movcs	lr, #1
	movcc	lr, #0
	cmp	r2, r1
	movcs	lr, #0
	cmp	lr, #0
	movne	r0, r3
	ldr	r3, [r3, #12]
	movne	ip, r2
	cmp	r3, #0
	bne	.L1530
	ldr	pc, [sp], #4
.L1531:
	mov	r0, r3
	bx	lr
	.size	get_block_max_erase_time_from_invalid_list, .-get_block_max_erase_time_from_invalid_list
	.align	2
	.global	get_block_min_erase_time_from_invalid_list
	.syntax unified
	.arm
	.type	get_block_min_erase_time_from_invalid_list, %function
get_block_min_erase_time_from_invalid_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #200]
	ldr	r0, [r0, #4]
	push	{r4, lr}
	cmp	r3, #0
	ldrh	r4, [r0, #16]
	beq	.L1540
	mov	r0, #0
.L1539:
	ldrh	ip, [r3, #6]
	cmp	ip, r1
	bcs	.L1538
	ldr	lr, [r3, #8]
	cmp	ip, r4
	cmpls	lr, r2
	movcc	lr, #1
	movcs	lr, #0
	cmp	lr, #0
	movne	r4, ip
	movne	r0, r3
.L1538:
	ldr	r3, [r3, #12]
	cmp	r3, #0
	bne	.L1539
	pop	{r4, pc}
.L1540:
	mov	r0, r3
	pop	{r4, pc}
	.size	get_block_min_erase_time_from_invalid_list, .-get_block_min_erase_time_from_invalid_list
	.align	2
	.global	get_block_max_count_from_invalid_list
	.syntax unified
	.arm
	.type	get_block_max_count_from_invalid_list, %function
get_block_max_count_from_invalid_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #200]
	cmp	r0, #0
	ldrne	r1, [r0, #8]
	movne	r3, r0
	bne	.L1549
	b	.L1551
.L1548:
	ldr	r2, [r3, #8]
	cmp	r1, r2
	movls	r1, r2
	movls	r0, r3
.L1549:
	ldr	r3, [r3, #12]
	cmp	r3, #0
	bne	.L1548
	bx	lr
.L1551:
	bx	lr
	.size	get_block_max_count_from_invalid_list, .-get_block_max_count_from_invalid_list
	.align	2
	.global	get_block_min_count_from_invalid_list
	.syntax unified
	.arm
	.type	get_block_min_count_from_invalid_list, %function
get_block_min_count_from_invalid_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #200]
	cmp	r0, #0
	ldrne	r1, [r0, #8]
	movne	r3, r0
	bne	.L1557
	b	.L1559
.L1556:
	ldr	r2, [r3, #8]
	cmp	r1, r2
	movcs	r1, r2
	movcs	r0, r3
.L1557:
	ldr	r3, [r3, #12]
	cmp	r3, #0
	bne	.L1556
	bx	lr
.L1559:
	bx	lr
	.size	get_block_min_count_from_invalid_list, .-get_block_min_count_from_invalid_list
	.align	2
	.global	out_phy_block_from_free_list_by_block
	.syntax unified
	.arm
	.type	out_phy_block_from_free_list_by_block, %function
out_phy_block_from_free_list_by_block:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #172]
	cmp	r3, #0
	bne	.L1567
	b	.L1561
.L1564:
	ldr	r3, [r3, #20]
	cmp	r3, #0
	beq	.L1561
.L1567:
	cmp	r1, r3
	bne	.L1564
	ldr	r1, [r3, #24]
	ldr	r2, [r3, #20]
	str	r2, [r1, #20]
	ldr	r2, [r3, #20]
	cmp	r2, #0
	strne	r1, [r2, #24]
	mov	r1, #0
	ldrh	r2, [r0, #22]
	str	r1, [r3, #20]
	str	r1, [r3, #24]
	cmp	r2, r1
	subne	r2, r2, #1
	strhne	r2, [r0, #22]	@ movhi
.L1561:
	mov	r0, r3
	bx	lr
	.size	out_phy_block_from_free_list_by_block, .-out_phy_block_from_free_list_by_block
	.align	2
	.global	out_phy_block_from_free_list_by_erase_time
	.syntax unified
	.arm
	.type	out_phy_block_from_free_list_by_erase_time, %function
out_phy_block_from_free_list_by_erase_time:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #172]
	cmp	r3, #0
	beq	.L1588
	mov	ip, #0
	push	{r4, lr}
	mov	lr, ip
.L1579:
	ldrh	r2, [r3, #6]
	cmp	r2, ip
	movcs	r4, #1
	movcc	r4, #0
	cmp	r2, r1
	movcs	r4, #0
	cmp	r4, #0
	movne	lr, r3
	ldr	r3, [r3, #20]
	movne	ip, r2
	cmp	r3, #0
	bne	.L1579
	cmp	lr, #0
	beq	.L1577
	mov	r1, lr
	pop	{r4, lr}
	b	out_phy_block_from_free_list_by_block
.L1577:
	mov	r0, #0
	pop	{r4, pc}
.L1588:
	mov	r0, #0
	bx	lr
	.size	out_phy_block_from_free_list_by_erase_time, .-out_phy_block_from_free_list_by_erase_time
	.align	2
	.global	put_phy_block_to_invalid_page_list
	.syntax unified
	.arm
	.type	put_phy_block_to_invalid_page_list, %function
put_phy_block_to_invalid_page_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, #0
	add	r0, r0, #188
	str	r3, [r1, #12]
	str	r3, [r1, #16]
	b	.L1590
.L1592:
	ldrh	r2, [r3, #4]
	ldrh	ip, [r1, #4]
	cmp	ip, r2
	bcs	.L1594
	mov	r0, r3
.L1590:
	ldr	r3, [r0, #12]
	cmp	r3, #0
	bne	.L1592
	str	r1, [r0, #12]
	str	r0, [r1, #16]
	mov	r0, #0
	str	r3, [r1, #12]
	bx	lr
.L1594:
	str	r1, [r3, #16]
	str	r3, [r1, #12]
	str	r1, [r0, #12]
	str	r0, [r1, #16]
	mov	r0, #0
	bx	lr
	.size	put_phy_block_to_invalid_page_list, .-put_phy_block_to_invalid_page_list
	.align	2
	.global	out_phy_block_from_invalid_page_list
	.syntax unified
	.arm
	.type	out_phy_block_from_invalid_page_list, %function
out_phy_block_from_invalid_page_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #200]
	cmp	r3, #0
	beq	.L1600
	ldr	r2, [r3, #12]
	str	r2, [r0, #200]
	ldr	r2, [r3, #12]
	cmp	r2, #0
	addne	r0, r0, #188
	strne	r0, [r2, #16]
	mov	r2, #0
	str	r2, [r3, #12]
	str	r2, [r3, #16]
.L1600:
	mov	r0, r3
	bx	lr
	.size	out_phy_block_from_invalid_page_list, .-out_phy_block_from_invalid_page_list
	.align	2
	.global	out_phy_block_from_invalid_page_list_by_block
	.syntax unified
	.arm
	.type	out_phy_block_from_invalid_page_list_by_block, %function
out_phy_block_from_invalid_page_list_by_block:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #200]
	cmp	r0, #0
	bne	.L1611
	bx	lr
.L1609:
	ldr	r0, [r0, #12]
	cmp	r0, #0
	beq	.L1620
.L1611:
	cmp	r1, r0
	bne	.L1609
	ldr	r3, [r0, #12]
	ldr	r2, [r0, #16]
	str	r3, [r2, #12]
	ldr	r3, [r0, #12]
	cmp	r3, #0
	strne	r2, [r3, #16]
	mov	r3, #0
	str	r3, [r0, #12]
	str	r3, [r0, #16]
	bx	lr
.L1620:
	bx	lr
	.size	out_phy_block_from_invalid_page_list_by_block, .-out_phy_block_from_invalid_page_list_by_block
	.align	2
	.global	phy_block_from_invalid_page_incr
	.syntax unified
	.arm
	.type	phy_block_from_invalid_page_incr, %function
phy_block_from_invalid_page_incr:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r1, #12]
	push	{r4, r5, r6, lr}
	mov	r4, r1
	mov	r5, r0
	cmp	r3, #0
	beq	.L1637
.L1622:
	ldrh	r3, [r4, #4]
	ldr	r2, [r5, #4]
	add	r3, r3, #1
	ldrh	r2, [r2, #20]
	uxth	r3, r3
	strh	r3, [r4, #4]	@ movhi
	cmp	r2, r3
	bcc	.L1638
.L1625:
	ldr	r3, [r4, #16]
	add	r5, r5, #188
	cmp	r3, r5
	beq	.L1624
	ldrh	r1, [r4, #4]
	b	.L1627
.L1629:
	ldr	r2, [r3, #16]
	str	r2, [r4, #16]
	ldr	r2, [r3, #16]
	str	r4, [r2, #12]
	ldr	r2, [r4, #12]
	str	r2, [r3, #12]
	ldr	r2, [r4, #12]
	cmp	r2, #0
	strne	r3, [r2, #16]
	str	r3, [r4, #12]
	str	r4, [r3, #16]
	ldr	r3, [r4, #16]
	ldr	r2, [r3, #16]
	cmp	r2, #0
	beq	.L1624
.L1627:
	ldrh	r2, [r3, #4]
	cmp	r2, r1
	bcc	.L1629
.L1624:
	mov	r0, #0
	pop	{r4, r5, r6, pc}
.L1638:
	movw	r0, #:lower16:.LC213
	movt	r0, #:upper16:.LC213
	bl	NAND_Print
	b	.L1625
.L1637:
	ldr	r3, [r1, #16]
	cmp	r3, #0
	bne	.L1622
	ldr	r3, [r0, #4]
	ldrh	r2, [r1, #4]
	ldrh	r3, [r3, #20]
	add	r2, r2, #1
	uxth	r2, r2
	cmp	r3, r2
	strh	r2, [r1, #4]	@ movhi
	bcs	.L1624
	movw	r0, #:lower16:.LC212
	ldrh	r1, [r1]
	movt	r0, #:upper16:.LC212
	bl	NAND_Print
	b	.L1624
	.size	phy_block_from_invalid_page_incr, .-phy_block_from_invalid_page_incr
	.align	2
	.global	adjust_invaild_list
	.syntax unified
	.arm
	.type	adjust_invaild_list, %function
adjust_invaild_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r1, [r0, #200]
	cmp	r1, #0
	beq	.L1654
	push	{r4, r5, r6, r7, lr}
	ldr	r5, [r0, #4]
	ldrh	r2, [r1, #4]
	ldrh	r3, [r5, #20]
	cmp	r2, r3
	bne	.L1651
	add	r6, r0, #152
	add	r7, r0, #188
	mov	r4, #0
.L1642:
	ldr	r3, [r1, #12]
	mov	r2, r6
	str	r3, [r0, #200]
	ldr	r3, [r1, #12]
	cmp	r3, #0
	strne	r7, [r3, #16]
	str	r4, [r1, #12]
	str	r4, [r1, #16]
	str	r4, [r1, #20]
	str	r4, [r1, #24]
	b	.L1644
.L1646:
	ldrh	ip, [r3, #6]
	ldrh	lr, [r1, #6]
	cmp	lr, ip
	bls	.L1655
	mov	r2, r3
.L1644:
	ldr	r3, [r2, #20]
	cmp	r3, #0
	bne	.L1646
	str	r1, [r2, #20]
	str	r3, [r1, #20]
	str	r2, [r1, #24]
.L1645:
	ldr	r1, [r0, #200]
	ldrh	r3, [r0, #22]
	cmp	r1, #0
	add	r3, r3, #1
	strh	r3, [r0, #22]	@ movhi
	beq	.L1651
	ldrh	r2, [r1, #4]
	ldrh	r3, [r5, #20]
	cmp	r2, r3
	beq	.L1642
.L1651:
	mov	r0, #0
	pop	{r4, r5, r6, r7, pc}
.L1655:
	str	r1, [r3, #24]
	str	r3, [r1, #20]
	str	r1, [r2, #20]
	str	r2, [r1, #24]
	b	.L1645
.L1654:
	mov	r0, #0
	bx	lr
	.size	adjust_invaild_list, .-adjust_invaild_list
	.align	2
	.global	zone_param_exit
	.syntax unified
	.arm
	.type	zone_param_exit, %function
zone_param_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r6, r0
	add	r4, r0, #48
	add	r5, r0, #148
.L1658:
	mov	r1, #0
	ldr	r0, [r4, #4]!
	cmp	r0, r1
	beq	.L1657
	bl	NAND_Free
.L1657:
	cmp	r4, r5
	bne	.L1658
	ldr	r0, [r6, #260]
	mov	r1, #0
	bl	NAND_Free
	ldr	r0, [r6, #296]
	mov	r1, #0
	bl	NAND_Free
	ldr	r0, [r6, #328]
	mov	r1, #0
	bl	NAND_Free
	ldr	r0, [r6, #284]
	mov	r1, #0
	bl	NAND_Free
	ldr	r0, [r6, #288]
	mov	r1, #0
	bl	NAND_Free
	mov	r0, #0
	pop	{r4, r5, r6, pc}
	.size	zone_param_exit, .-zone_param_exit
	.global	__aeabi_uidiv
	.align	2
	.global	is_no_use_device
	.syntax unified
	.arm
	.type	is_no_use_device, %function
is_no_use_device:
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #24]
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	mov	r4, r0
	sub	sp, sp, #32
	mov	r8, r1
	cmp	r3, #0
	ldr	r9, [r0, #40]
	beq	.L1665
	ldr	r10, [r0, #4]
	ldr	r0, [r0, #32]
	ldrb	r1, [r10, #8]	@ zero_extendqisi2
	add	r0, r3, r0
	bl	__aeabi_uidiv
	ldrh	r1, [r10, #20]
	bl	__aeabi_uidiv
	subs	r7, r0, #0
	mov	r3, #0
	strh	r3, [r4, #18]	@ movhi
	beq	.L1669
.L1666:
	add	r9, r9, #36
	mov	r5, #0
	mov	r6, #255
	b	.L1674
.L1677:
	cmp	r3, r7
	bcs	.L1669
.L1674:
	ldrh	ip, [r9, #-36]
	sub	r3, r9, #36
	ldrh	lr, [r10, #28]
	add	r1, sp, #16
	ldr	r2, [r4, #284]
	mov	r0, r4
	str	r3, [r4, #44]
	strh	lr, [sp, #20]	@ movhi
	strh	ip, [sp, #18]	@ movhi
	strh	r5, [sp, #16]	@ movhi
	str	sp, [sp, #28]
	str	r2, [sp, #24]
	ldr	r3, [r4, #1992]
	blx	r3
	cmp	r0, #1
	mov	r10, r0
	beq	.L1680
	strh	r6, [r9, #-34]	@ movhi
	add	r9, r9, #36
	ldrh	r3, [r4, #18]
	ldr	r10, [r4, #4]
	add	r3, r3, #1
	uxth	r3, r3
	ldrh	r2, [r10, #24]
	strh	r3, [r4, #18]	@ movhi
	ldrh	r1, [r9, #-72]
	sub	r2, r2, #1
	cmp	r1, r2
	bne	.L1677
.L1669:
	mov	r0, #1
.L1676:
	add	sp, sp, #32
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L1665:
	strh	r3, [r0, #18]	@ movhi
	mvn	r7, #0
	ldr	r10, [r0, #4]
	b	.L1666
.L1680:
	ldr	r3, [r4, #1980]
	add	r1, sp, #16
	mov	r0, r4
	blx	r3
	ldrb	r3, [sp, #1]	@ zero_extendqisi2
	cmp	r3, #170
	bne	.L1669
	ldrb	r3, [sp, #2]	@ zero_extendqisi2
	cmp	r3, #238
	bne	.L1669
	cmp	r0, #10
	cmpne	r0, #0
	bne	.L1669
	ldrb	r0, [sp, #3]	@ zero_extendqisi2
	ldrb	r1, [sp, #4]	@ zero_extendqisi2
	ldrb	r2, [sp, #5]	@ zero_extendqisi2
	ldrb	r3, [sp, #6]	@ zero_extendqisi2
	orr	r1, r1, r0, lsl #8
	orr	r2, r2, r1, lsl #8
	orr	r3, r3, r2, lsl #8
	cmp	r8, r3
	streq	r9, [r4, #40]
	moveq	r0, #0
	beq	.L1676
	movw	r0, #:lower16:.LC214
	movt	r0, #:upper16:.LC214
	bl	NAND_Print
	mov	r0, r10
	b	.L1676
	.size	is_no_use_device, .-is_no_use_device
	.align	2
	.global	get_vaild_blocks
	.syntax unified
	.arm
	.type	get_vaild_blocks, %function
get_vaild_blocks:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	sub	r8, r2, r1
	ldr	r2, [r0, #4]
	cmp	r1, r8
	sub	sp, sp, #16
	add	r3, r1, r1, lsl #3
	ldr	r4, [r2, #32]
	bcs	.L1686
	lsl	r3, r3, #2
	mov	r6, r0
	mov	r5, r1
	add	r3, r3, #36
	mov	r7, r8
	add	r4, r4, r3
	b	.L1685
.L1683:
	ldr	r3, [r6, #4]
	add	r4, r4, #36
	ldrh	r2, [r4, #-72]
	ldrh	r3, [r3, #24]
	sub	r3, r3, #1
	cmp	r2, r3
	beq	.L1691
	cmp	r8, r5
	beq	.L1691
.L1685:
	ldrh	r2, [r4, #-36]
	mov	r1, sp
	ldr	r3, [r6, #1992]
	mov	r0, r6
	add	r5, r5, #1
	strh	r2, [sp, #2]	@ movhi
	blx	r3
	cmp	r0, #159
	bne	.L1683
	subs	r7, r7, #1
	bne	.L1683
.L1691:
	mov	r0, r7
.L1689:
	add	sp, sp, #16
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L1686:
	mov	r0, r8
	b	.L1689
	.size	get_vaild_blocks, .-get_vaild_blocks
	.align	2
	.global	init_zone_after_first_scan
	.syntax unified
	.arm
	.type	init_zone_after_first_scan, %function
init_zone_after_first_scan:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r5, r1
	ldr	r6, [r0, #4]
	mov	r4, r0
	ldr	r0, [r0, #24]
	ldrb	r1, [r6, #8]	@ zero_extendqisi2
	bl	__aeabi_uidiv
	ldrh	r1, [r6, #20]
	bl	__aeabi_uidiv
	mov	r6, r0
	mov	r2, r0
	movw	r0, #:lower16:.LC215
	mov	r1, r5
	movt	r0, #:upper16:.LC215
	bl	NAND_Print_DBG
	cmp	r6, r5
	bcs	.L1700
	ldr	r0, [r4, #4]
	sub	r2, r5, r6
	ldr	r1, [r4, #1972]
	ldrb	ip, [r0, #8]	@ zero_extendqisi2
	ldrh	r3, [r0, #20]
	ldrh	r0, [r1, #12]
	mul	r3, r3, ip
	sub	r0, r0, #1
	cmp	r2, r0
	mul	r3, r2, r3
	str	r3, [r4, #32]
	bcc	.L1701
	movw	r6, #52429
	ldrh	r3, [r1, #14]
	movt	r6, 52428
	add	r0, r4, #380
	umull	r6, r7, r2, r6
	ubfx	r5, r7, #2, #16
	cmp	r5, r3
	strh	r5, [r0]	@ movhi
	strhcc	r3, [r0]	@ movhi
	movcc	r5, r3
	ldrh	r0, [r1, #18]
	ldrh	r1, [r1, #20]
	mul	r0, r2, r0
	bl	__aeabi_uidiv
	uxth	r3, r0
	add	r2, r5, #1
	add	r1, r4, #384
	cmp	r2, r3
	mov	ip, #1
	movwlt	r3, #382
	movwge	r3, #382
	addge	r5, r5, #2
	strhlt	r0, [r4, r3]	@ movhi
	strhge	r5, [r4, r3]	@ movhi
	movw	r2, #386
	mov	r3, #0
	strh	ip, [r1]	@ movhi
	mov	r0, r3
	strh	r3, [r4, r2]	@ movhi
	pop	{r4, r5, r6, r7, r8, pc}
.L1700:
	movw	r0, #:lower16:.LC216
	movt	r0, #:upper16:.LC216
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, r7, r8, pc}
.L1701:
	movw	r0, #:lower16:.LC217
	mov	r2, r6
	mov	r1, r5
	movt	r0, #:upper16:.LC217
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, r7, r8, pc}
	.size	init_zone_after_first_scan, .-init_zone_after_first_scan
	.align	2
	.global	do_write_error_in_build_list
	.syntax unified
	.arm
	.type	do_write_error_in_build_list, %function
do_write_error_in_build_list:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	mov	r7, r2
	ldr	r2, [r0, #4]
	sub	sp, sp, #24
	mov	r6, #0
	mov	r9, r1
	ldrh	ip, [r7]
	add	r1, sp, #8
	ldrh	r2, [r2, #28]
	mov	r8, r3
	strh	r6, [sp, #8]	@ movhi
	mov	r5, r0
	ldr	r3, [r0, #1976]
	strh	ip, [sp, #10]	@ movhi
	str	r6, [sp, #16]
	strh	r2, [sp, #12]	@ movhi
	str	r6, [sp, #20]
	blx	r3
	ldrh	r3, [r7, #6]
	cmp	r0, #0
	strh	r6, [r7, #4]	@ movhi
	add	r3, r3, #1
	strh	r3, [r7, #6]	@ movhi
	bne	.L1703
	cmp	r8, r6
	movwne	r10, #:lower16:.LC219
	movne	r4, r0
	movtne	r10, #:upper16:.LC219
	bne	.L1708
	b	.L1709
.L1707:
	cmp	r4, r8
	bcs	.L1709
.L1708:
	uxth	r1, r4
	ldr	r3, [r5, #284]
	ldr	r6, [r5, #1988]
	mov	r2, r7
	str	r1, [sp]
	mov	r0, r5
	mov	r1, r9
	add	r4, r4, #1
	blx	r6
	cmp	r0, #0
	beq	.L1707
	mov	r0, r10
	bl	NAND_Print
	cmp	r4, r8
	bcc	.L1708
.L1709:
	ldr	r3, [r5, #4]
	mov	r4, #0
	ldrh	r2, [r9]
	add	r1, sp, #8
	strh	r4, [sp, #8]	@ movhi
	mov	r0, r5
	ldrh	r3, [r3, #28]
	strh	r2, [sp, #10]	@ movhi
	str	r4, [sp, #16]
	strh	r3, [sp, #12]	@ movhi
	str	r4, [sp, #20]
	ldr	r3, [r5, #1996]
	blx	r3
	cmp	r8, r4
	movne	r0, r4
	moveq	r0, r8
	addeq	r3, r5, #320
	streq	r7, [r5, #292]
	strheq	r8, [r3]	@ movhi
.L1706:
	add	sp, sp, #24
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L1703:
	movw	r0, #:lower16:.LC218
	movt	r0, #:upper16:.LC218
	bl	NAND_Print
	add	r1, sp, #8
	ldr	r3, [r5, #1996]
	mov	r0, r5
	blx	r3
	mov	r0, #1
	add	sp, sp, #24
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
	.size	do_write_error_in_build_list, .-do_write_error_in_build_list
	.align	2
	.global	nftl_check_pattern
	.syntax unified
	.arm
	.type	nftl_check_pattern, %function
nftl_check_pattern:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r2, #0
	ble	.L1719
	ldrb	r3, [r0]	@ zero_extendqisi2
	cmp	r3, r1
	bne	.L1721
	add	r2, r0, r2
	add	r0, r0, #1
	b	.L1717
.L1718:
	ldrb	r3, [r0], #1	@ zero_extendqisi2
	cmp	r3, r1
	bne	.L1721
.L1717:
	cmp	r0, r2
	bne	.L1718
.L1719:
	mov	r0, #0
	bx	lr
.L1721:
	mov	r0, #1
	bx	lr
	.size	nftl_check_pattern, .-nftl_check_pattern
	.align	2
	.global	is_last_phy_block
	.syntax unified
	.arm
	.type	is_last_phy_block, %function
is_last_phy_block:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r1, #4]
	ldrh	r0, [r0]
	ldrh	r3, [r3, #24]
	sub	r3, r3, #1
	sub	r0, r3, r0
	clz	r0, r0
	lsr	r0, r0, #5
	bx	lr
	.size	is_last_phy_block, .-is_last_phy_block
	.align	2
	.global	is_phy_block_valid
	.syntax unified
	.arm
	.type	is_phy_block_valid, %function
is_phy_block_valid:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r1, #4]
	ldrh	r0, [r0]
	ldrh	r3, [r3, #24]
	cmp	r0, r3
	movcs	r0, #0
	movcc	r0, #1
	bx	lr
	.size	is_phy_block_valid, .-is_phy_block_valid
	.align	2
	.global	get_phy_block_addr
	.syntax unified
	.arm
	.type	get_phy_block_addr, %function
get_phy_block_addr:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #4]
	add	r1, r1, r1, lsl #3
	ldr	r0, [r3, #32]
	add	r0, r0, r1, lsl #2
	bx	lr
	.size	get_phy_block_addr, .-get_phy_block_addr
	.align	2
	.global	check_cross_talk
	.syntax unified
	.arm
	.type	check_cross_talk, %function
check_cross_talk:
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #4]
	ldrh	r2, [r3, #20]
	cmp	r2, #0
	beq	.L1733
	push	{r4, r5, r6, r7, lr}
	mov	r4, #0
	sub	sp, sp, #36
	mov	r6, r1
	mov	r5, r0
	mov	r7, r4
.L1729:
	ldrh	r2, [r3, #28]
	add	r1, sp, #16
	ldrh	r3, [r6]
	mov	r0, r5
	strh	r4, [sp, #16]	@ movhi
	add	r4, r4, #1
	strh	r2, [sp, #20]	@ movhi
	strh	r3, [sp, #18]	@ movhi
	str	r7, [sp, #24]
	str	sp, [sp, #28]
	ldr	r3, [r5, #1980]
	blx	r3
	cmp	r0, #0
	bne	.L1728
	ldrb	r3, [sp, #14]	@ zero_extendqisi2
	cmp	r3, #99
	bhi	.L1728
	ldr	r2, [r5, #1968]
	ldr	r1, [r2, #172]
	cmp	r1, r3
	bls	.L1734
.L1728:
	ldr	r3, [r5, #4]
	ldrh	r2, [r3, #20]
	cmp	r2, r4
	bhi	.L1729
	mov	r0, #0
	add	sp, sp, #36
	@ sp needed
	pop	{r4, r5, r6, r7, pc}
.L1734:
	mov	r0, #0
	str	r3, [r2, #172]
	add	sp, sp, #36
	@ sp needed
	pop	{r4, r5, r6, r7, pc}
.L1733:
	mov	r0, #0
	bx	lr
	.size	check_cross_talk, .-check_cross_talk
	.align	2
	.global	init_smart_info
	.syntax unified
	.arm
	.type	init_smart_info, %function
init_smart_info:
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r2, #328
	mov	r4, r0
	mov	r6, r1
	sub	sp, sp, #32
	mov	r1, #0
	ldr	r0, [r0, #1968]
	movw	r5, #43521
	bl	NAND_Memset
	ldr	r3, [r4, #1968]
	cmp	r6, #0
	movt	r5, 43690
	ldr	r2, [r3, #144]
	str	r5, [r3, #64]
	add	r2, r2, #1
	str	r2, [r3, #144]
	beq	.L1747
	ldr	r2, [r4, #4]
	add	r1, sp, #16
	ldrh	lr, [r6]
	mov	r0, r4
	ldr	ip, [r4, #284]
	ldrh	r3, [r2, #20]
	ldrh	r2, [r2, #28]
	strh	lr, [sp, #18]	@ movhi
	sub	r3, r3, #1
	str	ip, [sp, #24]
	strh	r3, [sp, #16]	@ movhi
	strh	r2, [sp, #20]	@ movhi
	ldr	r3, [r4, #1980]
	str	sp, [sp, #28]
	blx	r3
	ldrb	lr, [sp, #1]	@ zero_extendqisi2
	movw	ip, #65535
	ldrb	r1, [sp, #2]	@ zero_extendqisi2
	movt	ip, 43690
	ldrb	r2, [sp, #3]	@ zero_extendqisi2
	ldrb	r3, [sp, #4]	@ zero_extendqisi2
	orr	r1, r1, lr, lsl #8
	orr	r2, r2, r1, lsl #8
	orr	r3, r3, r2, lsl #8
	cmp	r3, ip
	beq	.L1751
.L1739:
	ldr	r3, [r4, #356]
	cmp	r3, #0
	bne	.L1752
.L1747:
	mov	r0, #0
	add	sp, sp, #32
	@ sp needed
	pop	{r4, r5, r6, pc}
.L1752:
	mov	r0, r4
	bl	print_smart.part.16
	mov	r0, #0
	add	sp, sp, #32
	@ sp needed
	pop	{r4, r5, r6, pc}
.L1751:
	cmp	r0, #10
	cmpne	r0, #0
	bne	.L1739
	ldr	r3, [r4, #308]
	ldr	r1, [r4, #284]
	add	r1, r1, r3
	ldr	r3, [r1, #64]
	cmp	r3, r5
	bne	.L1739
	mov	r2, #328
	ldr	r0, [r4, #1968]
	bl	NAND_Memcpy
	ldr	r3, [r4, #1968]
	ldr	r1, [r3, #168]
	ldr	r2, [r3, #144]
	cmn	r1, #1
	add	r2, r2, #1
	str	r2, [r3, #144]
	moveq	r2, #0
	streq	r2, [r3, #168]
	ldr	r2, [r3, #176]
	cmn	r2, #1
	moveq	r2, #0
	streq	r2, [r3, #176]
	ldr	r2, [r3, #180]
	cmn	r2, #1
	moveq	r2, #0
	streq	r2, [r3, #180]
	b	.L1739
	.size	init_smart_info, .-init_smart_info
	.align	2
	.global	get_used_block_count
	.syntax unified
	.arm
	.type	get_used_block_count, %function
get_used_block_count:
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	sub	sp, sp, #32
	ldr	lr, [r0, #4]
	add	r2, sp, #16
	mov	ip, #0
	ldrh	r1, [r1]
	ldr	r4, [r0, #284]
	ldrh	lr, [lr, #28]
	strh	r1, [sp, #2]	@ movhi
	mov	r1, sp
	str	r2, [sp, #12]
	str	r4, [sp, #8]
	ldr	r3, [r0, #1980]
	strh	lr, [sp, #4]	@ movhi
	strh	ip, [sp]	@ movhi
	blx	r3
	ldrb	r1, [sp, #23]	@ zero_extendqisi2
	ldrb	r2, [sp, #24]	@ zero_extendqisi2
	ldrb	r3, [sp, #25]	@ zero_extendqisi2
	ldrb	r0, [sp, #26]	@ zero_extendqisi2
	orr	r2, r2, r1, lsl #8
	orr	r3, r3, r2, lsl #8
	orr	r0, r0, r3, lsl #8
	add	sp, sp, #32
	@ sp needed
	pop	{r4, pc}
	.size	get_used_block_count, .-get_used_block_count
	.align	2
	.global	block_last_page_ecc_error
	.syntax unified
	.arm
	.type	block_last_page_ecc_error, %function
block_last_page_ecc_error:
	@ args = 0, pretend = 0, frame = 48
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	mov	r5, r0
	movw	r0, #:lower16:.LC220
	sub	sp, sp, #52
	movt	r0, #:upper16:.LC220
	mov	r8, r1
	bl	NAND_Print
	mov	r0, #4096
	bl	NAND_Malloc
	subs	r3, r0, #0
	str	r3, [sp, #8]
	beq	.L1794
.L1756:
	ldr	r2, [r5, #4]
	ldrh	fp, [r2, #20]
	cmp	fp, #0
	beq	.L1777
	ldr	r3, [sp, #8]
	movw	r7, #:lower16:.LC221
	movt	r7, #:upper16:.LC221
	mov	r4, #0
	sub	r6, r3, #4
.L1759:
	ldrh	r0, [r2, #26]
	add	r4, r4, #1
	bl	nftl_malloc
	cmp	r0, #0
	str	r0, [r6, #4]!
	beq	.L1795
.L1758:
	ldr	r2, [r5, #4]
	ldrh	r3, [r2, #20]
	cmp	r3, r4
	bgt	.L1759
	cmp	r3, #0
	beq	.L1778
	ldr	r10, [sp, #8]
	mov	r3, #0
	movw	r1, #:lower16:.LC222
	movw	r0, #:lower16:.LC225
	mov	r6, r3
	add	r7, sp, #16
	add	r9, sp, #32
	add	r4, sp, #26
	movt	r1, #:upper16:.LC222
	movt	r0, #:upper16:.LC225
	str	r3, [sp, #4]
	str	r1, [sp, #12]
	str	r0, [sp]
.L1764:
	ldrh	r0, [r2, #28]
	uxth	fp, r6
	ldrh	ip, [r8]
	mov	r1, r9
	ldr	r2, [r5, #284]
	strh	r0, [sp, #36]	@ movhi
	mov	r0, r5
	strh	ip, [sp, #34]	@ movhi
	str	r2, [sp, #40]
	str	r7, [sp, #44]
	ldr	r2, [r5, #1980]
	strh	fp, [sp, #32]	@ movhi
	blx	r2
	mov	r2, r7
	mov	ip, #0
.L1761:
	ldrb	r3, [r2], #1	@ zero_extendqisi2
	cmp	r3, #255
	addeq	ip, ip, #1
	cmp	r2, r4
	bne	.L1761
	cmp	ip, #8
	bls	.L1762
	ldrb	r2, [sp, #16]	@ zero_extendqisi2
	cmp	r2, #255
	beq	.L1796
.L1762:
	ldrb	r3, [sp, #17]	@ zero_extendqisi2
	and	r2, r3, #240
	cmp	r2, #192
	beq	.L1797
	cmp	r3, #170
	beq	.L1774
	ldr	r0, [sp]
	bl	NAND_Print
.L1763:
	ldr	r2, [r5, #4]
	add	r6, r6, #1
	add	r10, r10, #4
	ldrh	r3, [r2, #20]
	cmp	r3, r6
	bgt	.L1764
	uxth	fp, r6
.L1757:
	ldrh	lr, [r2, #28]
	mov	r1, r9
	ldr	r2, [r5, #284]
	mov	r0, r5
	ldrh	ip, [r8]
	strh	lr, [sp, #36]	@ movhi
	str	r2, [sp, #40]
	strh	fp, [sp, #32]	@ movhi
	str	r7, [sp, #44]
	strh	ip, [sp, #34]	@ movhi
	ldr	r2, [r5, #1976]
	blx	r2
	cmp	r0, #0
	bne	.L1798
.L1765:
	ldr	r3, [sp, #4]
	cmp	r3, #0
	beq	.L1767
	ldr	r2, [sp, #8]
	mov	r4, #0
	mov	r10, r3
	sub	r6, r2, #4
.L1768:
	ldr	r3, [r5, #4]
	mov	r1, r9
	ldrh	ip, [r8]
	mov	r0, r5
	ldr	r2, [r6, #4]!
	ldrh	r3, [r3, #28]
	strh	r4, [sp, #32]	@ movhi
	add	r4, r4, #1
	strh	ip, [sp, #34]	@ movhi
	strh	r3, [sp, #36]	@ movhi
	str	r2, [sp, #40]
	str	r7, [sp, #44]
	ldr	r3, [r5, #1984]
	blx	r3
	cmp	r4, r10
	bne	.L1768
.L1767:
	ldr	r3, [r5, #4]
	ldrh	r3, [r3, #20]
	cmp	r3, #0
	beq	.L1771
	ldr	r3, [sp, #8]
	mov	r4, #0
	sub	r6, r3, #4
.L1770:
	mov	r1, #0
	ldr	r0, [r6, #4]!
	bl	NAND_Free
	ldr	r3, [r5, #4]
	add	r4, r4, #1
	ldrh	r3, [r3, #20]
	cmp	r3, r4
	bgt	.L1770
.L1771:
	ldr	r0, [sp, #8]
	mov	r1, #0
	bl	NAND_Free
	movw	r0, #:lower16:.LC224
	movt	r0, #:upper16:.LC224
	bl	NAND_Print
	mov	r0, r8
	add	sp, sp, #52
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L1774:
	cmp	r0, #10
	cmpne	r0, #0
	beq	.L1799
.L1776:
	ldr	r0, [sp, #12]
	bl	NAND_Print
	b	.L1763
.L1797:
	cmp	r0, #10
	cmpne	r0, #0
	bne	.L1763
	ldr	r2, [sp, #4]
	ldr	r3, [r5, #4]
	ldr	r1, [r5, #284]
	add	r2, r2, #1
	ldr	r0, [r10]
	str	r2, [sp, #4]
	ldrh	r2, [r3, #26]
	bl	NAND_Memcpy
	b	.L1763
.L1799:
	ldr	r2, [sp, #4]
	ldr	r3, [r5, #4]
	ldr	r1, [r5, #284]
	add	r2, r2, #1
	ldr	r0, [r10]
	str	r2, [sp, #4]
	ldrh	r2, [r3, #26]
	bl	NAND_Memcpy
	b	.L1776
.L1795:
	mov	r0, r7
	bl	NAND_Print
	b	.L1758
.L1796:
	ldr	r2, [r5, #4]
	b	.L1757
.L1798:
	ldr	r2, [r5, #1996]
	mov	r1, r9
	mov	r0, r5
	blx	r2
	mov	r0, r5
	bl	out_phy_block_from_free_list
	subs	r8, r0, #0
	beq	.L1800
	ldr	r2, [r5, #4]
	mov	r1, r9
	ldrh	ip, [r8]
	mov	r0, r5
	ldr	r3, [r5, #284]
	ldrh	r2, [r2, #28]
	strh	fp, [sp, #32]	@ movhi
	str	r3, [sp, #40]
	strh	ip, [sp, #34]	@ movhi
	strh	r2, [sp, #36]	@ movhi
	str	r7, [sp, #44]
	ldr	r3, [r5, #1976]
	blx	r3
	b	.L1765
.L1777:
	str	fp, [sp, #4]
	add	r7, sp, #16
	add	r9, sp, #32
	b	.L1757
.L1794:
	movw	r0, #:lower16:.LC221
	movt	r0, #:upper16:.LC221
	bl	NAND_Print
	b	.L1756
.L1800:
	movw	r0, #:lower16:.LC223
	movt	r0, #:upper16:.LC223
	bl	NAND_Print
	b	.L1767
.L1778:
	mov	fp, r3
	add	r7, sp, #16
	str	r3, [sp, #4]
	add	r9, sp, #32
	b	.L1757
	.size	block_last_page_ecc_error, .-block_last_page_ecc_error
	.align	2
	.global	print_nftl_zone
	.syntax unified
	.arm
	.type	print_nftl_zone, %function
print_nftl_zone:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #356]
	cmp	r3, #0
	bxeq	lr
	b	print_nftl_zone.part.17
	.size	print_nftl_zone, .-print_nftl_zone
	.align	2
	.global	nand_dbg_phy_read
	.syntax unified
	.arm
	.type	nand_dbg_phy_read, %function
nand_dbg_phy_read:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, lr}
	mov	r5, r0
	sub	sp, sp, #36
	mov	r0, #4096
	mov	r6, r1
	mov	r7, r2
	bl	NAND_Malloc
	subs	r4, r0, #0
	beq	.L1806
.L1804:
	add	r3, sp, #16
	mov	r2, r7
	mov	r1, r6
	str	r3, [sp, #4]
	str	r4, [sp]
	mov	r0, r5
	mov	r3, #8
	bl	PageRead
	ldrb	lr, [sp, #20]	@ zero_extendqisi2
	mov	r5, r0
	ldrb	ip, [sp, #19]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC227
	ldrb	r3, [sp, #18]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC227
	ldrb	r2, [sp, #17]	@ zero_extendqisi2
	stm	sp, {ip, lr}
	ldrb	r1, [sp, #16]	@ zero_extendqisi2
	bl	NAND_Print
	ldrb	lr, [sp, #25]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC228
	ldrb	ip, [sp, #24]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC228
	ldrb	r3, [sp, #23]	@ zero_extendqisi2
	ldrb	r2, [sp, #22]	@ zero_extendqisi2
	stm	sp, {ip, lr}
	ldrb	r1, [sp, #21]	@ zero_extendqisi2
	bl	NAND_Print
	ldrb	ip, [r4, #4]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC229
	ldrb	r3, [r4, #2]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC229
	ldrb	r2, [r4, #1]	@ zero_extendqisi2
	ldrb	r1, [r4]	@ zero_extendqisi2
	str	ip, [sp, #4]
	ldrb	ip, [r4, #3]	@ zero_extendqisi2
	str	ip, [sp]
	bl	NAND_Print
	ldrb	ip, [r4, #10]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC230
	ldrb	r3, [r4, #7]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC230
	ldrb	r2, [r4, #6]	@ zero_extendqisi2
	ldrb	r1, [r4, #5]	@ zero_extendqisi2
	str	ip, [sp, #8]
	ldrb	ip, [r4, #9]	@ zero_extendqisi2
	str	ip, [sp, #4]
	ldrb	ip, [r4, #8]	@ zero_extendqisi2
	str	ip, [sp]
	bl	NAND_Print
	mov	r0, r4
	mov	r1, #0
	bl	NAND_Free
	mov	r0, r5
	add	sp, sp, #36
	@ sp needed
	pop	{r4, r5, r6, r7, pc}
.L1806:
	movw	r0, #:lower16:.LC226
	ldr	r1, .L1807
	movt	r0, #:upper16:.LC226
	bl	NAND_Print
	b	.L1804
.L1808:
	.align	2
.L1807:
	.word	.LANCHOR2+648
	.size	nand_dbg_phy_read, .-nand_dbg_phy_read
	.align	2
	.global	nand_dbg_zone_phy_read
	.syntax unified
	.arm
	.type	nand_dbg_zone_phy_read, %function
nand_dbg_zone_phy_read:
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, lr}
	mov	r6, r0
	sub	sp, sp, #52
	mov	r0, #4096
	mov	r5, r1
	mov	r7, r2
	bl	NAND_Malloc
	subs	r4, r0, #0
	beq	.L1812
.L1810:
	ldr	r2, [r6, #4]
	add	r5, r5, r5, lsl #3
	add	ip, sp, #32
	ldr	r3, [r6, #1980]
	lsl	r5, r5, #2
	mov	lr, #8
	ldr	r2, [r2, #32]
	add	r1, sp, #16
	mov	r0, r6
	ldrh	r2, [r2, r5]
	strh	lr, [sp, #20]	@ movhi
	str	ip, [sp, #28]
	strh	r2, [sp, #18]	@ movhi
	strh	r7, [sp, #16]	@ movhi
	str	r4, [sp, #24]
	blx	r3
	ldrb	lr, [sp, #36]	@ zero_extendqisi2
	mov	r5, r0
	ldrb	ip, [sp, #35]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC227
	ldrb	r3, [sp, #34]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC227
	ldrb	r2, [sp, #33]	@ zero_extendqisi2
	stm	sp, {ip, lr}
	ldrb	r1, [sp, #32]	@ zero_extendqisi2
	bl	NAND_Print
	ldrb	lr, [sp, #41]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC228
	ldrb	ip, [sp, #40]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC228
	ldrb	r3, [sp, #39]	@ zero_extendqisi2
	ldrb	r2, [sp, #38]	@ zero_extendqisi2
	stm	sp, {ip, lr}
	ldrb	r1, [sp, #37]	@ zero_extendqisi2
	bl	NAND_Print
	ldrb	ip, [r4, #4]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC229
	ldrb	r3, [r4, #2]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC229
	ldrb	r2, [r4, #1]	@ zero_extendqisi2
	ldrb	r1, [r4]	@ zero_extendqisi2
	str	ip, [sp, #4]
	ldrb	ip, [r4, #3]	@ zero_extendqisi2
	str	ip, [sp]
	bl	NAND_Print
	ldrb	ip, [r4, #10]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC230
	ldrb	r3, [r4, #7]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC230
	ldrb	r2, [r4, #6]	@ zero_extendqisi2
	ldrb	r1, [r4, #5]	@ zero_extendqisi2
	str	ip, [sp, #8]
	ldrb	ip, [r4, #9]	@ zero_extendqisi2
	str	ip, [sp, #4]
	ldrb	ip, [r4, #8]	@ zero_extendqisi2
	str	ip, [sp]
	bl	NAND_Print
	mov	r0, r4
	mov	r1, #0
	bl	NAND_Free
	mov	r0, r5
	add	sp, sp, #52
	@ sp needed
	pop	{r4, r5, r6, r7, pc}
.L1812:
	movw	r0, #:lower16:.LC226
	ldr	r1, .L1813
	movt	r0, #:upper16:.LC226
	bl	NAND_Print
	b	.L1810
.L1814:
	.align	2
.L1813:
	.word	.LANCHOR2+668
	.size	nand_dbg_zone_phy_read, .-nand_dbg_zone_phy_read
	.align	2
	.global	nand_dbg_zone_phy_write
	.syntax unified
	.arm
	.type	nand_dbg_zone_phy_write, %function
nand_dbg_zone_phy_write:
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r5, r0
	sub	sp, sp, #32
	mov	r0, #8192
	mov	r4, r1
	mov	r7, r2
	bl	NAND_Malloc
	subs	r6, r0, #0
	beq	.L1818
.L1816:
	ldr	r3, [r5, #4]
	mov	r0, r6
	mov	r2, #4096
	mov	r1, #165
	add	r4, r4, r4, lsl #3
	ldr	r8, [r3, #32]
	bl	NAND_Memset
	ldr	lr, [r5, #4]
	lsl	r4, r4, #2
	mvn	r3, #0
	mvn	r2, #90
	strb	r3, [sp, #16]
	strb	r3, [sp, #17]
	add	ip, sp, #16
	strb	r3, [sp, #18]
	mov	r1, sp
	strb	r3, [sp, #19]
	mov	r0, r5
	strb	r3, [sp, #20]
	strb	r3, [sp, #23]
	strb	r3, [sp, #24]
	strb	r3, [sp, #25]
	strb	r3, [sp, #26]
	strb	r2, [sp, #21]
	ldrh	r3, [lr, #28]
	strb	r2, [sp, #22]
	ldrh	r2, [r8, r4]
	strh	r3, [sp, #4]	@ movhi
	str	ip, [sp, #12]
	strh	r2, [sp, #2]	@ movhi
	ldr	r3, [r5, #1984]
	strh	r7, [sp]	@ movhi
	str	r6, [sp, #8]
	blx	r3
	mov	r1, #0
	mov	r4, r0
	mov	r0, r6
	bl	NAND_Free
	mov	r0, r4
	add	sp, sp, #32
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L1818:
	movw	r0, #:lower16:.LC231
	ldr	r1, .L1819
	movt	r0, #:upper16:.LC231
	bl	NAND_Print
	b	.L1816
.L1820:
	.align	2
.L1819:
	.word	.LANCHOR2+692
	.size	nand_dbg_zone_phy_write, .-nand_dbg_zone_phy_write
	.align	2
	.global	nand_dbg_phy_write
	.syntax unified
	.arm
	.type	nand_dbg_phy_write, %function
nand_dbg_phy_write:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, lr}
	mov	r5, r0
	sub	sp, sp, #28
	mov	r0, #8192
	mov	r6, r1
	mov	r7, r2
	bl	NAND_Malloc
	subs	r4, r0, #0
	beq	.L1824
.L1822:
	mov	r0, r4
	mov	r2, #4096
	mov	r1, #165
	bl	NAND_Memset
	add	r3, sp, #8
	mvn	ip, #0
	mvn	lr, #90
	mov	r2, r7
	mov	r1, r6
	str	r3, [sp, #4]
	str	r4, [sp]
	mov	r3, #8
	mov	r0, r5
	strb	ip, [sp, #8]
	strb	ip, [sp, #9]
	strb	ip, [sp, #10]
	strb	ip, [sp, #11]
	strb	ip, [sp, #12]
	strb	ip, [sp, #15]
	strb	ip, [sp, #16]
	strb	ip, [sp, #17]
	strb	ip, [sp, #18]
	strb	lr, [sp, #13]
	strb	lr, [sp, #14]
	bl	PageWrite
	mov	r1, #0
	mov	r5, r0
	mov	r0, r4
	bl	NAND_Free
	mov	r0, r5
	add	sp, sp, #28
	@ sp needed
	pop	{r4, r5, r6, r7, pc}
.L1824:
	movw	r0, #:lower16:.LC231
	ldr	r1, .L1825
	movt	r0, #:upper16:.LC231
	bl	NAND_Print
	b	.L1822
.L1826:
	.align	2
.L1825:
	.word	.LANCHOR2+716
	.size	nand_dbg_phy_write, .-nand_dbg_phy_write
	.align	2
	.global	nand_dbg_phy_erase
	.syntax unified
	.arm
	.type	nand_dbg_phy_erase, %function
nand_dbg_phy_erase:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	b	BlockErase
	.size	nand_dbg_phy_erase, .-nand_dbg_phy_erase
	.align	2
	.global	nand_dbg_single_phy_erase
	.syntax unified
	.arm
	.type	nand_dbg_single_phy_erase, %function
nand_dbg_single_phy_erase:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	b	nand_physic_erase_block
	.size	nand_dbg_single_phy_erase, .-nand_dbg_single_phy_erase
	.align	2
	.global	clean_block_list
	.syntax unified
	.arm
	.type	clean_block_list, %function
clean_block_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r5, r0
	mov	r4, #0
	mvn	r6, #0
	b	.L1832
.L1830:
	strh	r4, [r0, #4]	@ movhi
	strh	r4, [r0, #2]	@ movhi
	str	r6, [r0, #8]
	str	r4, [r0, #12]
	str	r4, [r0, #16]
	str	r4, [r0, #20]
	str	r4, [r0, #24]
	str	r4, [r0, #28]
	str	r4, [r0, #32]
.L1832:
	mov	r0, r5
	bl	out_phy_block_from_free_list
	cmp	r0, #0
	bne	.L1830
	ldr	r3, [r5, #200]
	cmp	r3, #0
	addne	ip, r5, #188
	mvnne	r1, #0
	beq	.L1841
.L1834:
	ldr	r2, [r3, #12]
	str	r2, [r5, #200]
	ldr	r2, [r3, #12]
	cmp	r2, #0
	strne	ip, [r2, #16]
	strh	r0, [r3, #4]	@ movhi
	strh	r0, [r3, #2]	@ movhi
	str	r1, [r3, #8]
	str	r0, [r3, #12]
	str	r0, [r3, #16]
	str	r0, [r3, #20]
	str	r0, [r3, #24]
	str	r0, [r3, #28]
	str	r0, [r3, #32]
	ldr	r3, [r5, #200]
	cmp	r3, #0
	bne	.L1834
	pop	{r4, r5, r6, pc}
.L1841:
	pop	{r4, r5, r6, pc}
	.size	clean_block_list, .-clean_block_list
	.align	2
	.global	nand_clean_zone_table2
	.syntax unified
	.arm
	.type	nand_clean_zone_table2, %function
nand_clean_zone_table2:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #4]
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	mov	r5, r0
	ldr	r0, [r0, #24]
	ldrb	r1, [r3, #8]	@ zero_extendqisi2
	bl	__aeabi_uidiv
	mov	r2, r0
	movw	r0, #:lower16:.LC232
	lsl	r8, r2, #2
	ldrb	r1, [r5, #16]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC232
	bl	NAND_Print
	lsrs	r9, r8, #20
	beq	.L1843
	add	r6, r9, #12
	movw	r7, #:lower16:.LC233
	add	r6, r5, r6, lsl #2
	movt	r7, #:upper16:.LC233
	add	r4, r5, #48
.L1844:
	ldr	r0, [r4, #4]!
	mov	r2, #1048576
	mov	r1, #255
	bl	NAND_Memset
	mov	r1, #1048576
	mov	r0, r7
	bl	NAND_Print
	cmp	r4, r6
	bne	.L1844
.L1843:
	ubfx	r8, r8, #0, #20
	cmp	r8, #0
	bne	.L1856
.L1845:
	mov	r0, #0
	add	ip, r5, #320
	mvn	r4, #0
	add	r3, r5, #396
	mov	r1, r0
	mov	r2, r0
	str	r0, [r5, #292]
	add	lr, r5, #352
	strh	r4, [ip]	@ movhi
	add	ip, r5, #392
	str	r0, [r5, #324]
	strh	r4, [lr]	@ movhi
	str	r0, [r5, #172]
	str	r0, [r5, #176]
	str	r0, [r5, #164]
	str	r0, [r5, #168]
	str	r0, [r5, #180]
	str	r0, [r5, #184]
	str	r0, [r5, #208]
	str	r0, [r5, #212]
	str	r0, [r5, #200]
	str	r0, [r5, #204]
	str	r0, [r5, #216]
	str	r0, [r5, #220]
	str	r0, [r5, #244]
	str	r0, [r5, #248]
	str	r0, [r5, #236]
	str	r0, [r5, #240]
	str	r0, [r5, #252]
	str	r0, [r5, #256]
	strh	r0, [r5, #20]	@ movhi
	strh	r0, [r5, #22]	@ movhi
	strh	r0, [ip]	@ movhi
.L1846:
	strh	r1, [r3]	@ movhi
	add	r1, r1, #1
	cmp	r1, #10
	strh	r2, [r3, #2]	@ movhi
	str	r2, [r3, #4]
	add	r3, r3, #16
	str	r2, [r3, #-8]
	str	r2, [r3, #-4]
	bne	.L1846
	add	r0, r5, #556
	mov	ip, #255
	movw	r1, #394
	movw	r3, #558
	strh	ip, [r0]	@ movhi
	mov	r0, #0
	str	r2, [r5, #560]
	str	r2, [r5, #564]
	str	r2, [r5, #568]
	strh	r2, [r5, r1]	@ movhi
	strh	r2, [r5, r3]	@ movhi
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L1856:
	add	r9, r5, r9, lsl #2
	mov	r2, r8
	mov	r1, #255
	ldr	r0, [r9, #52]
	bl	NAND_Memset
	movw	r0, #:lower16:.LC233
	mov	r1, r8
	movt	r0, #:upper16:.LC233
	bl	NAND_Print
	b	.L1845
	.size	nand_clean_zone_table2, .-nand_clean_zone_table2
	.align	2
	.global	nand_power_off_ecc_error
	.syntax unified
	.arm
	.type	nand_power_off_ecc_error, %function
nand_power_off_ecc_error:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r4, r2
	ldrb	r3, [r2]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L1858
	movw	r0, #:lower16:.LC234
	movt	r0, #:upper16:.LC234
	bl	NAND_Print
.L1859:
	movw	r0, #:lower16:.LC238
	ldr	r1, .L1878
	mov	r3, #1
	movw	r2, #2821
	movt	r0, #:upper16:.LC238
	movw	r5, #:lower16:.LC237
	bl	NAND_Print
	movw	r0, #:lower16:.LC239
	mov	r6, r4
	movt	r0, #:upper16:.LC239
	add	r4, r4, #1
	bl	NAND_Print
	mov	r0, r5
	ldrb	r1, [r6], #16	@ zero_extendqisi2
	movt	r0, #:upper16:.LC237
	mov	r5, r0
	bl	NAND_Print
.L1865:
	ldrb	r1, [r4], #1	@ zero_extendqisi2
	mov	r0, r5
	bl	NAND_Print
	cmp	r6, r4
	bne	.L1865
	movw	r0, #:lower16:.LC132
	movt	r0, #:upper16:.LC132
	bl	NAND_Print
	movw	r0, #:lower16:.LC132
	movt	r0, #:upper16:.LC132
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, pc}
.L1858:
	mov	r5, r0
	mov	r6, r1
	bl	is_physic_info_enable_crc
	cmp	r0, #0
	bne	.L1875
	ldrb	r3, [r4, #11]	@ zero_extendqisi2
	cmp	r3, #165
	bne	.L1862
	ldrb	r3, [r4, #12]	@ zero_extendqisi2
	cmp	r3, #165
	beq	.L1876
.L1862:
	movw	r0, #:lower16:.LC236
	movt	r0, #:upper16:.LC236
	bl	NAND_Print
	b	.L1859
.L1875:
	mov	r2, r6
	mov	r3, r4
	add	r1, r5, #308
	ldr	r0, [r5, #4]
	bl	crc_wrong.isra.18
	cmp	r0, #0
	bne	.L1877
.L1861:
	mov	r0, #0
	pop	{r4, r5, r6, pc}
.L1876:
	ldrb	r3, [r4, #13]	@ zero_extendqisi2
	cmp	r3, #165
	bne	.L1862
	b	.L1861
.L1877:
	movw	r0, #:lower16:.LC235
	movt	r0, #:upper16:.LC235
	bl	NAND_Print
	b	.L1859
.L1879:
	.align	2
.L1878:
	.word	.LANCHOR2+736
	.size	nand_power_off_ecc_error, .-nand_power_off_ecc_error
	.align	2
	.global	recover_phy_page_mapping
	.syntax unified
	.arm
	.type	recover_phy_page_mapping, %function
recover_phy_page_mapping:
	@ args = 0, pretend = 0, frame = 40
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	mov	r8, r2
	mov	r7, r0
	ldr	r2, [r0, #308]
	sub	sp, sp, #44
	mov	r9, r1
	mov	r0, r8
	mov	r1, #255
	bl	NAND_Memset
	ldr	r2, [r7, #4]
	ldrh	r3, [r2, #20]
	cmp	r3, #0
	beq	.L1903
	movw	fp, #:lower16:.LC245
	movw	r10, #13107
	movw	r3, #:lower16:.LC243
	movt	fp, #:upper16:.LC245
	movt	r10, 43537
	mov	r5, #0
	add	r4, sp, #18
	movt	r3, #:upper16:.LC243
	str	r3, [sp, #4]
.L1896:
	ldrh	ip, [r2, #28]
	add	r1, sp, #24
	ldr	r3, [r7, #284]
	mov	r0, r7
	ldrh	r2, [r9]
	strh	ip, [sp, #28]	@ movhi
	add	ip, sp, #8
	str	r3, [sp, #32]
	strh	r5, [sp, #24]	@ movhi
	ldr	r3, [r7, #1980]
	str	ip, [sp, #36]
	strh	r2, [sp, #26]	@ movhi
	blx	r3
	add	r3, sp, #8
	mov	r6, r0
	mov	r1, #0
.L1883:
	ldrb	r2, [r3], #1	@ zero_extendqisi2
	cmp	r2, #255
	addeq	r1, r1, #1
	cmp	r3, r4
	bne	.L1883
	cmp	r1, #8
	bls	.L1884
	ldrb	r3, [sp, #8]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L1919
.L1884:
	ldrb	r3, [sp, #9]	@ zero_extendqisi2
	and	r2, r3, #240
	cmp	r2, #192
	beq	.L1920
	cmp	r3, #170
	beq	.L1921
.L1901:
	ldrb	r0, [sp, #10]	@ zero_extendqisi2
	ldrb	r1, [sp, #11]	@ zero_extendqisi2
	ldrb	r2, [sp, #12]	@ zero_extendqisi2
	orr	r3, r0, r3, lsl #8
	orr	r3, r1, r3, lsl #8
	orr	r3, r2, r3, lsl #8
	cmp	r3, r10
	beq	.L1922
	movw	r0, #:lower16:.LC246
	mov	r1, r5
	movt	r0, #:upper16:.LC246
	bl	NAND_Print
.L1891:
	cmp	r6, #0
	cmpne	r6, #10
	bne	.L1889
.L1894:
	ldr	r2, [r7, #4]
	add	r5, r5, #1
	add	r8, r8, #4
	ldrh	r3, [r2, #20]
	cmp	r3, r5
	bgt	.L1896
	mov	r0, r5
.L1909:
	add	sp, sp, #44
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L1922:
	cmp	r6, #10
	cmpne	r6, #0
	bne	.L1891
	str	r10, [r8]
	mov	r1, r5
	ldr	r0, [sp, #4]
	bl	NAND_Print
	cmp	r6, #0
	cmpne	r6, #10
	beq	.L1894
.L1889:
	movw	r0, #:lower16:.LC244
	mov	r2, r5
	ldrh	r1, [r9]
	movt	r0, #:upper16:.LC244
	bl	NAND_Print
	movw	r0, #65535
	add	sp, sp, #44
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L1919:
	cmp	r6, #10
	cmpne	r6, #0
	beq	.L1923
	ldrb	r3, [sp, #9]	@ zero_extendqisi2
	and	r2, r3, #240
	cmp	r2, #192
	beq	.L1891
	cmp	r3, #170
	bne	.L1901
.L1921:
	mov	r1, r5
	mov	r0, fp
	bl	NAND_Print
	b	.L1891
.L1920:
	cmp	r6, #10
	cmpne	r6, #0
	bne	.L1891
	add	r2, sp, #8
	ldr	r1, [r7, #284]
	mov	r0, r7
	bl	nand_power_off_ecc_error
	cmp	r0, #0
	bne	.L1924
	ldrb	r0, [sp, #9]	@ zero_extendqisi2
	ldrb	r1, [sp, #10]	@ zero_extendqisi2
	ldrb	r2, [sp, #11]	@ zero_extendqisi2
	ldrb	r3, [sp, #12]	@ zero_extendqisi2
	orr	r1, r1, r0, lsl #8
	orr	r2, r2, r1, lsl #8
	orr	r3, r3, r2, lsl #8
	bic	r3, r3, #-268435456
	str	r3, [r8]
	ldr	r2, [r7, #28]
	cmp	r2, r3
	bhi	.L1891
	movw	r0, #:lower16:.LC242
	mov	r2, r5
	movt	r0, #:upper16:.LC242
	ldrh	r1, [r9]
	bl	NAND_Print
	mvn	r3, #0
	str	r3, [r8]
	b	.L1891
.L1924:
	movw	r0, #:lower16:.LC241
	movt	r0, #:upper16:.LC241
	bl	NAND_Print
	b	.L1889
.L1923:
	movw	r0, #:lower16:.LC240
	mov	r1, r5
	movt	r0, #:upper16:.LC240
	bl	NAND_Print
	mov	r0, r5
	add	sp, sp, #44
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L1903:
	mov	r0, r3
	b	.L1909
	.size	recover_phy_page_mapping, .-recover_phy_page_mapping
	.align	2
	.global	recover_block_phy_page_mapping
	.syntax unified
	.arm
	.type	recover_block_phy_page_mapping, %function
recover_block_phy_page_mapping:
	@ args = 0, pretend = 0, frame = 40
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	mov	r8, r2
	mov	r5, r0
	ldr	r2, [r0, #308]
	sub	sp, sp, #52
	mov	r7, r1
	mov	r0, r8
	mov	r1, #255
	bl	NAND_Memset
	ldr	r3, [r5, #4]
	ldrh	r2, [r3, #20]
	cmp	r2, #0
	beq	.L1949
	movw	r10, #:lower16:.LC250
	movw	fp, #:lower16:.LC249
	movw	r2, #:lower16:.LC242
	movt	r10, #:upper16:.LC250
	movt	fp, #:upper16:.LC249
	mov	r4, #0
	add	r9, sp, #16
	movt	r2, #:upper16:.LC242
	str	r2, [sp, #12]
.L1938:
	ldrh	ip, [r3, #28]
	add	r1, sp, #32
	ldr	r3, [r5, #284]
	mov	r0, r5
	ldrh	r2, [r7]
	strh	ip, [sp, #36]	@ movhi
	str	r3, [sp, #40]
	strh	r4, [sp, #32]	@ movhi
	str	r9, [sp, #44]
	strh	r2, [sp, #34]	@ movhi
	ldr	r3, [r5, #1980]
	blx	r3
	subs	r6, r0, #0
	bge	.L1927
	movw	r0, #:lower16:.LC247
	mov	r1, r6
	mov	r3, r4
	ldrh	r2, [r7]
	movt	r0, #:upper16:.LC247
	bl	NAND_Print
	ldr	r3, [r5, #1980]
	add	r1, sp, #32
	mov	r0, r5
	blx	r3
	subs	r6, r0, #0
	bge	.L1927
	movw	r0, #:lower16:.LC247
	mov	r3, r4
	ldrh	r2, [r7]
	mov	r1, r6
	movt	r0, #:upper16:.LC247
	bl	NAND_Print
	mov	r3, #1
	movw	r0, #:lower16:.LC248
	ldrh	r2, [r7]
	mov	r1, r6
	str	r3, [sp]
	movt	r0, #:upper16:.LC248
	mov	r3, r4
	bl	NAND_Print
	ldrb	r3, [sp, #17]	@ zero_extendqisi2
	and	r3, r3, #240
	cmp	r3, #192
	beq	.L1933
.L1930:
	mov	r3, r9
	add	r1, sp, #26
	mov	r0, #0
.L1936:
	ldrb	r2, [r3], #1	@ zero_extendqisi2
	cmp	r2, #255
	addeq	r0, r0, #1
	cmp	r3, r1
	bne	.L1936
	cmp	r0, #8
	bls	.L1937
	ldrb	r3, [sp, #16]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L1933
.L1937:
	mov	r2, r4
	ldrh	r1, [r7]
	mov	r0, r10
	bl	NAND_Print
.L1933:
	ldr	r3, [r5, #4]
	add	r4, r4, #1
	add	r8, r8, #4
	ldrh	r2, [r3, #20]
	cmp	r2, r4
	bgt	.L1938
.L1949:
	mov	r0, #0
	add	sp, sp, #52
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L1927:
	ldrb	r3, [sp, #17]	@ zero_extendqisi2
	and	r3, r3, #240
	cmp	r3, #192
	bne	.L1930
	cmp	r6, #10
	cmpne	r6, #0
	bne	.L1933
	mov	r2, r9
	ldr	r1, [r5, #284]
	mov	r0, r5
	bl	nand_power_off_ecc_error
	cmp	r0, #0
	bne	.L1951
	ldrb	r0, [sp, #17]	@ zero_extendqisi2
	ldrb	r1, [sp, #18]	@ zero_extendqisi2
	ldrb	r2, [sp, #19]	@ zero_extendqisi2
	ldrb	r3, [sp, #20]	@ zero_extendqisi2
	orr	r1, r1, r0, lsl #8
	orr	r2, r2, r1, lsl #8
	orr	r3, r3, r2, lsl #8
	bic	r3, r3, #-268435456
	str	r3, [r8]
	ldr	r2, [r5, #28]
	cmp	r2, r3
	bhi	.L1933
	mov	r2, r4
	ldrh	r1, [r7]
	ldr	r0, [sp, #12]
	bl	NAND_Print
	mvn	r3, #0
	str	r3, [r8]
	b	.L1933
.L1951:
	mov	r0, fp
	bl	NAND_Print
	b	.L1933
	.size	recover_block_phy_page_mapping, .-recover_block_phy_page_mapping
	.align	2
	.global	get_used_page_num
	.syntax unified
	.arm
	.type	get_used_page_num, %function
get_used_page_num:
	@ args = 0, pretend = 0, frame = 56
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	ip, [r0, #4]
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	mov	r8, #0
	sub	sp, sp, #68
	str	r8, [r2]
	ldrh	r9, [ip, #20]
	cmp	r9, r8
	beq	.L1953
	mov	r6, r3
	mov	r10, r2
	movw	r3, #:lower16:.LC251
	movw	r2, #:lower16:.LC252
	cmp	r6, #0
	movt	r3, #:upper16:.LC251
	movt	r2, #:upper16:.LC252
	str	r10, [sp, #16]
	str	r1, [sp, #24]
	mov	r5, r0
	str	r3, [sp, #12]
	mov	r9, #1
	str	r2, [sp, #20]
	mov	r10, r1
	beq	.L1954
.L1994:
	ldr	r2, [r6, #4]
	ldrb	r2, [r2, r8]	@ zero_extendqisi2
	cmp	r2, #0
	bne	.L1954
	ldr	r2, [r6, #8]
	ldrh	r7, [ip, #26]
	ldrb	fp, [r2, r8]	@ zero_extendqisi2
	add	r7, r7, fp
.L1955:
	ldrh	r2, [r10]
	add	r1, sp, #48
	ldrh	ip, [ip, #28]
	mov	r0, r5
	strh	r8, [sp, #48]	@ movhi
	strh	r2, [sp, #50]	@ movhi
	strh	ip, [sp, #52]	@ movhi
	str	fp, [sp, #56]
	str	r7, [sp, #60]
	ldr	r2, [r5, #1980]
	blx	r2
	cmp	r0, #0
	cmpne	r0, #10
	mov	r4, r0
	bne	.L1991
.L1956:
	sub	r2, r7, #1
	add	r0, r7, #9
	mov	ip, #0
.L1958:
	ldrb	r3, [r2, #1]!	@ zero_extendqisi2
	cmp	r3, #255
	addeq	ip, ip, #1
	cmp	r2, r0
	bne	.L1958
	cmp	ip, #8
	bls	.L1959
	ldrb	r2, [r7]	@ zero_extendqisi2
	cmp	r2, #255
	beq	.L1992
.L1959:
	cmp	r4, #10
	cmpne	r4, #0
	moveq	r4, #1
	movne	r4, #0
	cmp	fp, #0
	moveq	r4, #0
	andne	r4, r4, #1
	cmp	r4, #0
	beq	.L1962
	mov	r2, r7
	mov	r1, fp
	mov	r0, r5
	bl	nand_power_off_ecc_error
	cmp	r0, #0
	bne	.L1993
	ldr	r2, [r6, #4]
	mov	r1, #1
	strb	r1, [r2, r8]
	ldr	r2, [r6]
	cmp	r2, r9
	strcc	r9, [r6]
.L1962:
	ldr	ip, [r5, #4]
	add	r8, r8, #1
	add	r1, r9, #1
	ldrh	r2, [ip, #20]
	cmp	r2, r9
	bls	.L1953
	cmp	r6, #0
	mov	r9, r1
	bne	.L1994
.L1954:
	ldrb	r2, [ip, #8]	@ zero_extendqisi2
	cmp	r2, #8
	ldreq	fp, [r5, #284]
	addeq	r7, sp, #32
	addne	r7, sp, #32
	movne	fp, #0
	b	.L1955
.L1991:
	mov	r2, r8
	ldrh	r1, [r10]
	ldr	r0, [sp, #12]
	bl	NAND_Print
	ldr	r3, [sp, #16]
	ldr	r2, [r3]
	add	r2, r2, #1
	str	r2, [r3]
	b	.L1956
.L1993:
	ldr	r0, [sp, #20]
	bl	NAND_Print
	b	.L1962
.L1953:
	mov	r8, r9
.L1981:
	mov	r0, r8
	add	sp, sp, #68
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L1992:
	ldr	r3, [r5, #4]
	ldrh	r2, [r3, #20]
	cmp	r2, r8
	bls	.L1981
	ldr	r9, [sp, #24]
	movw	r2, #:lower16:.LC253
	movw	r1, #:lower16:.LC227
	movw	r0, #:lower16:.LC228
	add	fp, r8, #1
	mov	r10, r8
	movt	r2, #:upper16:.LC253
	movt	r1, #:upper16:.LC227
	movt	r0, #:upper16:.LC228
	str	r2, [sp, #12]
	str	r1, [sp, #16]
	str	r0, [sp, #20]
	str	r8, [sp, #28]
.L1970:
	cmp	r6, #0
	beq	.L1976
	ldr	r2, [r6, #4]
	ldrb	r2, [r2, r10]	@ zero_extendqisi2
	cmp	r2, #0
	addne	r4, sp, #32
	movne	r8, #0
	ldreq	r2, [r6, #8]
	ldrheq	r4, [r3, #26]
	ldrbeq	r8, [r2, r10]	@ zero_extendqisi2
	addeq	r4, r4, r8
.L1965:
	ldrh	r2, [r3, #28]
	add	r1, sp, #48
	ldrh	r3, [r9]
	mov	r0, r5
	strh	r10, [sp, #48]	@ movhi
	strh	r2, [sp, #52]	@ movhi
	strh	r3, [sp, #50]	@ movhi
	str	r8, [sp, #56]
	ldr	r3, [r5, #1980]
	str	r4, [sp, #60]
	blx	r3
	sub	r3, r4, #1
	mov	r7, r0
	add	r1, r4, #9
	mov	r0, #0
.L1967:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r2, #255
	addeq	r0, r0, #1
	cmp	r1, r3
	bne	.L1967
	cmp	r0, #8
	bls	.L1968
	ldrb	r3, [r4]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L1978
.L1968:
	mov	r2, r10
	ldrh	r1, [r9]
	ldr	r0, [sp, #12]
	bl	NAND_Print
	ldrb	ip, [r4, #4]	@ zero_extendqisi2
	ldrb	r3, [r4, #2]	@ zero_extendqisi2
	ldrb	r2, [r4, #1]	@ zero_extendqisi2
	ldrb	r1, [r4]	@ zero_extendqisi2
	str	ip, [sp, #4]
	ldrb	ip, [r4, #3]	@ zero_extendqisi2
	ldr	r0, [sp, #16]
	str	ip, [sp]
	bl	NAND_Print
	ldrb	ip, [r4, #9]	@ zero_extendqisi2
	ldrb	r3, [r4, #7]	@ zero_extendqisi2
	ldrb	r2, [r4, #6]	@ zero_extendqisi2
	ldrb	r1, [r4, #5]	@ zero_extendqisi2
	str	ip, [sp, #4]
	ldrb	ip, [r4, #8]	@ zero_extendqisi2
	ldr	r0, [sp, #20]
	str	ip, [sp]
	bl	NAND_Print
	cmp	r7, #10
	cmpne	r7, #0
	moveq	r7, #1
	movne	r7, #0
	cmp	r8, #0
	moveq	r8, #0
	andne	r8, r7, #1
	cmp	r8, #0
	beq	.L1978
	ldr	r3, [r6, #4]
	mov	r2, #1
	mov	r1, fp
	strb	r2, [r3, r10]
	ldr	r3, [r6]
	cmp	r3, fp
	strcc	fp, [r6]
.L1969:
	ldr	r3, [r5, #4]
	add	fp, fp, #1
	add	r10, r10, #1
	ldrh	r2, [r3, #20]
	cmp	r2, r1
	bhi	.L1970
	ldr	r8, [sp, #28]
	mov	r0, r8
	add	sp, sp, #68
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L1978:
	mov	r1, fp
	b	.L1969
.L1976:
	add	r4, sp, #32
	mov	r8, r6
	b	.L1965
	.size	get_used_page_num, .-get_used_page_num
	.align	2
	.global	get_used_page_num_no_crosstalk
	.syntax unified
	.arm
	.type	get_used_page_num_no_crosstalk, %function
get_used_page_num_no_crosstalk:
	@ args = 0, pretend = 0, frame = 48
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	ip, [r0, #4]
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	mov	fp, #0
	str	fp, [r2]
	sub	sp, sp, #52
	ldrh	r2, [ip, #20]
	cmp	r2, fp
	beq	.L1996
	mov	r7, r3
	mov	r10, r1
	movw	r2, #:lower16:.LC255
	movw	r1, #:lower16:.LC254
	cmp	r7, #0
	movt	r2, #:upper16:.LC255
	movt	r1, #:upper16:.LC254
	mov	r6, r0
	str	fp, [sp]
	mov	r5, fp
	str	fp, [sp, #4]
	mov	r8, #1
	str	r2, [sp, #12]
	mvn	r9, #0
	str	r1, [sp, #8]
	beq	.L2014
.L2042:
	ldr	r2, [r7, #4]
	ldrb	r2, [r2, r5]	@ zero_extendqisi2
	cmp	r2, #0
	addne	r4, sp, #16
	movne	fp, #0
	bne	.L1997
	ldrh	r4, [ip, #26]
	ldr	r2, [r7, #8]
	add	fp, r4, #64
	mla	fp, r5, fp, r2
	add	r4, fp, r4
.L1997:
	ldrh	ip, [ip, #28]
	add	r1, sp, #32
	ldrh	r2, [r10]
	mov	r0, r6
	strh	r5, [sp, #32]	@ movhi
	strh	ip, [sp, #36]	@ movhi
	strh	r2, [sp, #34]	@ movhi
	str	fp, [sp, #40]
	ldr	r2, [r6, #1980]
	str	r4, [sp, #44]
	blx	r2
	cmp	r0, #0
	cmpne	r0, #10
	movne	r0, #1
	moveq	r0, #0
	subeq	r2, r4, #1
	addeq	ip, r4, #9
	bne	.L2037
.L2001:
	ldrb	r3, [r2, #1]!	@ zero_extendqisi2
	cmp	r3, #255
	addeq	r0, r0, #1
	cmp	r2, ip
	bne	.L2001
	cmp	r0, #8
	bls	.L2002
	ldrb	r2, [r4]	@ zero_extendqisi2
	cmp	r2, #255
	beq	.L2038
.L2002:
	mov	r1, fp
	mov	r2, r4
	mov	r0, r6
	bl	nand_power_off_ecc_error
	cmp	r0, #0
	bne	.L2039
	ldr	r3, [sp, #4]
	cmn	r9, #1
	add	r3, r3, #1
	str	r3, [sp, #4]
	beq	.L2040
.L1999:
	ldr	ip, [r6, #4]
	add	r5, r5, #1
	add	r1, r8, #1
	ldrh	r2, [ip, #20]
	cmp	r2, r8
	bls	.L2041
	cmp	r7, #0
	mov	r8, r1
	bne	.L2042
.L2014:
	add	r4, sp, #16
	mov	fp, r7
	b	.L1997
.L2038:
	ldr	r3, [sp]
	cmn	r9, #1
	moveq	r9, r5
	add	r3, r3, #1
	cmp	r3, #3
	str	r3, [sp]
	bne	.L1999
.L2004:
	ldr	r3, [sp, #4]
	cmp	r3, r9
	beq	.L2021
	movw	r0, #:lower16:.LC256
	mov	r2, r3
	ldrh	r1, [r10]
	mov	r3, r9
	movt	r0, #:upper16:.LC256
	bl	NAND_Print
	ldr	r1, [r6, #1968]
	ldr	r2, [r7]
	ldr	r3, [r1, #172]
	cmp	r2, #0
	add	r3, r3, #1
	str	r3, [r1, #172]
	beq	.L2021
	mov	r0, #0
.L2009:
	ldr	r3, [r7, #4]
	ldrb	r3, [r3, r0]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L2008
	ldr	r1, [r6, #4]
	ldr	r3, [r6, #1968]
	ldr	r2, [r7, #8]
	ldrh	r1, [r1, #26]
	ldr	ip, [r3, #172]
	add	r2, r2, r1
	add	r3, r1, #64
	mla	r3, r0, r3, r2
	strb	ip, [r3, #14]
	ldr	r2, [r7]
.L2008:
	add	r0, r0, #1
	cmp	r2, r0
	bhi	.L2009
.L2021:
	mov	r0, r9
	add	sp, sp, #52
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L2037:
	cmn	r9, #1
	mov	r2, r5
	ldrh	r1, [r10]
	moveq	r9, r5
	ldr	r0, [sp, #8]
	bl	NAND_Print
	b	.L1999
.L2039:
	ldr	r0, [sp, #12]
	bl	NAND_Print
	b	.L1999
.L2040:
	ldr	r2, [r7, #4]
	mov	r1, #1
	lsl	ip, r5, #2
	strb	r1, [r2, r5]
	add	r1, r4, #4
	ldr	lr, [r7, #12]
.L2005:
	ldrb	r3, [r4, #1]!	@ zero_extendqisi2
	cmp	r4, r1
	orr	r0, r3, r0, lsl #8
	bne	.L2005
	bic	r0, r0, #-268435456
	str	r0, [lr, ip]
	ldr	r2, [r7]
	cmp	r2, r8
	strcc	r8, [r7]
	b	.L1999
.L2041:
	cmn	r9, #1
	bne	.L2004
.L1996:
	mov	r9, r2
	mov	r0, r9
	add	sp, sp, #52
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
	.size	get_used_page_num_no_crosstalk, .-get_used_page_num_no_crosstalk
	.align	2
	.global	set_spare_data
	.syntax unified
	.arm
	.type	set_spare_data, %function
set_spare_data:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r3, #0
	bxeq	lr
	sub	r3, r3, #1
	add	r2, r0, r2
.L2045:
	lsl	ip, r3, #3
	sub	r3, r3, #1
	cmn	r3, #1
	lsr	ip, r1, ip
	strb	ip, [r2], #1
	bne	.L2045
	bx	lr
	.size	set_spare_data, .-set_spare_data
	.align	2
	.global	get_spare_data
	.syntax unified
	.arm
	.type	get_spare_data, %function
get_spare_data:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r2, #0
	beq	.L2051
	add	r2, r2, r1
	add	r1, r0, r1
	add	r2, r0, r2
	mov	r0, #0
.L2050:
	ldrb	r3, [r1], #1	@ zero_extendqisi2
	cmp	r2, r1
	orr	r0, r3, r0, lsl #8
	bne	.L2050
	bx	lr
.L2051:
	mov	r0, r2
	bx	lr
	.size	get_spare_data, .-get_spare_data
	.align	2
	.global	set_oob_logic_page
	.syntax unified
	.arm
	.type	set_oob_logic_page, %function
set_oob_logic_page:
	@ args = 4, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, lr}
	mvn	r0, #63
	ldrh	ip, [sp, #20]
	lsr	r7, r2, #16
	orr	r0, r0, r2, lsr #24
	lsr	r6, r2, #8
	lsr	r4, r3, #24
	lsr	lr, r3, #16
	strb	r3, [r1, #10]
	lsr	r3, r3, #8
	lsr	r5, ip, #8
	strb	r2, [r1, #4]
	mvn	r2, #0
	strb	r7, [r1, #2]
	strb	r2, [r1]
	strb	r6, [r1, #3]
	strb	r0, [r1, #1]
	strb	ip, [r1, #6]
	strb	r5, [r1, #5]
	strb	r4, [r1, #7]
	strb	lr, [r1, #8]
	strb	r3, [r1, #9]
	pop	{r4, r5, r6, r7, pc}
	.size	set_oob_logic_page, .-set_oob_logic_page
	.align	2
	.global	set_oob_special_page
	.syntax unified
	.arm
	.type	set_oob_special_page, %function
set_oob_special_page:
	@ args = 4, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, lr}
	lsr	ip, r3, #16
	ldrh	r0, [sp, #20]
	lsr	r7, r2, #24
	lsr	r6, r2, #16
	lsr	r5, r2, #8
	lsr	lr, r3, #24
	strb	r3, [r1, #10]
	lsr	r3, r3, #8
	strb	r2, [r1, #4]
	lsr	r4, r0, #8
	mvn	r2, #0
	strb	r2, [r1]
	strb	r7, [r1, #1]
	strb	r6, [r1, #2]
	strb	r5, [r1, #3]
	strb	r0, [r1, #6]
	strb	r4, [r1, #5]
	strb	lr, [r1, #7]
	strb	ip, [r1, #8]
	strb	r3, [r1, #9]
	pop	{r4, r5, r6, r7, pc}
	.size	set_oob_special_page, .-set_oob_special_page
	.align	2
	.global	mark_free_block
	.syntax unified
	.arm
	.type	mark_free_block, %function
mark_free_block:
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #44
	add	r6, sp, #24
	mov	r4, r0
	mov	r5, r1
	mov	r0, r6
	mov	r2, #16
	mov	r1, #165
	bl	NAND_Memset
	ldr	r1, [r4, #4]
	movw	r7, #21845
	ldrh	r8, [r5, #6]
	movt	r7, 21845
	ldrh	lr, [r5]
	mov	r2, r7
	ldr	ip, [r4, #284]
	mov	r0, r4
	ldrh	r9, [r1, #28]
	mov	r1, r6
	ldr	r3, [r5, #8]
	str	r8, [sp]
	mov	r8, #0
	strh	lr, [sp, #10]	@ movhi
	str	ip, [sp, #16]
	strh	r9, [sp, #12]	@ movhi
	str	r6, [sp, #20]
	strh	r8, [sp, #8]	@ movhi
	bl	set_oob_special_page
	ldr	r3, [r4, #1984]
	add	r1, sp, #8
	mov	r0, r4
	blx	r3
	ldr	ip, [r4, #4]
	mov	r2, r7
	ldrh	r9, [r5, #6]
	mov	lr, #1
	ldr	r3, [r5, #8]
	mov	r1, r6
	ldrh	ip, [ip, #28]
	mov	r0, r4
	str	r9, [sp]
	ldrh	r9, [r5]
	strh	ip, [sp, #12]	@ movhi
	ldr	ip, [r4, #284]
	strh	lr, [sp, #8]	@ movhi
	strh	r9, [sp, #10]	@ movhi
	str	ip, [sp, #16]
	str	r6, [sp, #20]
	bl	set_oob_special_page
	ldr	r3, [r4, #1984]
	add	r1, sp, #8
	mov	r0, r4
	blx	r3
	ldr	r0, [r4, #4]
	mov	r2, r7
	ldrh	lr, [r5, #6]
	mov	r1, r6
	ldr	r3, [r5, #8]
	ldrh	ip, [r0, #20]
	ldrh	r9, [r0, #28]
	mov	r0, r4
	str	lr, [sp]
	sub	ip, ip, #2
	ldrh	lr, [r5]
	strh	ip, [sp, #8]	@ movhi
	ldr	ip, [r4, #284]
	strh	lr, [sp, #10]	@ movhi
	strh	r9, [sp, #12]	@ movhi
	str	ip, [sp, #16]
	str	r6, [sp, #20]
	bl	set_oob_special_page
	ldr	r3, [r4, #1984]
	add	r1, sp, #8
	mov	r0, r4
	blx	r3
	ldr	lr, [r4, #4]
	mov	r2, r7
	ldrh	r9, [r5, #6]
	mov	r1, r6
	ldr	r3, [r5, #8]
	mov	r0, r4
	ldrh	ip, [lr, #20]
	ldrh	r7, [lr, #28]
	ldrh	r5, [r5]
	str	r9, [sp]
	sub	ip, ip, #1
	ldr	lr, [r4, #284]
	strh	ip, [sp, #8]	@ movhi
	str	r6, [sp, #20]
	str	lr, [sp, #16]
	strh	r7, [sp, #12]	@ movhi
	strh	r5, [sp, #10]	@ movhi
	bl	set_oob_special_page
	add	r1, sp, #8
	ldr	r3, [r4, #1984]
	mov	r0, r4
	blx	r3
	mov	r0, r8
	add	sp, sp, #44
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
	.size	mark_free_block, .-mark_free_block
	.align	2
	.global	erase_block
	.syntax unified
	.arm
	.type	erase_block, %function
erase_block:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r6, r0
	movw	r0, #:lower16:.LC257
	sub	sp, sp, #16
	mov	r4, r1
	movt	r0, #:upper16:.LC257
	ldrh	r1, [r1]
	mov	r5, r2
	bl	NAND_Print
	ldr	r2, [r6, #4]
	mov	r7, #0
	ldrh	r1, [r4]
	mov	r0, r6
	ldr	r3, [r6, #1976]
	ldrh	r2, [r2, #28]
	strh	r1, [sp, #2]	@ movhi
	mov	r1, sp
	strh	r7, [sp]	@ movhi
	strh	r2, [sp, #4]	@ movhi
	str	r7, [sp, #8]
	str	r7, [sp, #12]
	blx	r3
	subs	r8, r0, #0
	bne	.L2067
	mvn	r3, #0
	strh	r5, [r4, #6]	@ movhi
	str	r8, [r4, #20]
	add	r1, r6, #152
	str	r8, [r4, #24]
	str	r3, [r4, #8]
	b	.L2062
.L2064:
	ldrh	r0, [r3, #6]
	cmp	r5, r0
	bls	.L2068
	mov	r1, r3
.L2062:
	ldr	r3, [r1, #20]
	cmp	r3, #0
	bne	.L2064
	str	r4, [r1, #20]
	str	r3, [r4, #20]
	str	r1, [r4, #24]
.L2063:
	ldrh	r3, [r6, #22]
	mov	r1, r4
	mov	r0, r6
	add	r3, r3, #1
	strh	r3, [r6, #22]	@ movhi
	bl	mark_free_block
	ldr	r3, [r6, #4]
	mov	r2, #0
	mov	r0, r8
	ldrh	r3, [r3, #20]
	strh	r2, [r4, #2]	@ movhi
	strh	r3, [r4, #4]	@ movhi
	add	sp, sp, #16
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L2068:
	str	r4, [r3, #24]
	str	r3, [r4, #20]
	str	r4, [r1, #20]
	str	r1, [r4, #24]
	b	.L2063
.L2067:
	movw	r0, #:lower16:.LC258
	ldrh	r1, [r4]
	movt	r0, #:upper16:.LC258
	bl	NAND_Print
	ldr	r3, [r6, #1996]
	mov	r1, sp
	mov	r0, r6
	blx	r3
	ldrh	r3, [r6, #20]
	mov	r2, #255
	strh	r7, [r4, #4]	@ movhi
	mov	r0, r8
	strh	r2, [r4, #2]	@ movhi
	add	r3, r3, #1
	strh	r3, [r6, #20]	@ movhi
	add	sp, sp, #16
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
	.size	erase_block, .-erase_block
	.align	2
	.global	adjust_free_list
	.syntax unified
	.arm
	.type	adjust_free_list, %function
adjust_free_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r4, r0
	bl	out_phy_block_from_free_list
	subs	r5, r0, #0
	beq	.L2072
.L2070:
	ldrh	r2, [r5, #6]
	mov	r1, r5
	mov	r0, r4
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	mov	r1, r5
	mov	r0, r4
	bl	out_phy_block_from_free_list_by_block
	mov	r3, r0
	movw	r0, #:lower16:.LC260
	str	r3, [r4, #48]
	movt	r0, #:upper16:.LC260
	ldrh	r2, [r3, #6]
	ldrh	r1, [r3]
	bl	NAND_Print
	mov	r0, #0
	pop	{r4, r5, r6, pc}
.L2072:
	movw	r0, #:lower16:.LC259
	movt	r0, #:upper16:.LC259
	bl	NAND_Print
	b	.L2070
	.size	adjust_free_list, .-adjust_free_list
	.align	2
	.global	nand_dbg_zone_erase
	.syntax unified
	.arm
	.type	nand_dbg_zone_erase, %function
nand_dbg_zone_erase:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #4]
	add	r1, r1, r1, lsl #3
	push	{r4, r5, r6, lr}
	mov	r4, r0
	mov	r5, r2
	ldr	r3, [r3, #32]
	add	r1, r3, r1, lsl #2
	bl	out_phy_block_from_free_list_by_block
	mov	r2, r5
	mov	r1, r0
	mov	r0, r4
	pop	{r4, r5, r6, lr}
	b	erase_block
	.size	nand_dbg_zone_erase, .-nand_dbg_zone_erase
	.align	2
	.global	torture_block_and_mark
	.syntax unified
	.arm
	.type	torture_block_and_mark, %function
torture_block_and_mark:
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, lr}
	mov	r4, r0
	movw	r0, #:lower16:.LC257
	sub	sp, sp, #44
	mov	r5, #0
	mov	r6, r1
	movt	r0, #:upper16:.LC257
	ldrh	r1, [r1]
	str	r5, [sp, #16]
	str	r5, [sp, #8]
	str	r5, [sp, #12]
	str	r5, [sp, #19]	@ unaligned
	bl	NAND_Print_DBG
	ldr	r2, [r4, #4]
	mov	r0, r4
	ldrh	r3, [r6]
	add	r1, sp, #24
	strh	r5, [sp, #24]	@ movhi
	ldrh	r2, [r2, #28]
	str	r5, [sp, #32]
	str	r5, [sp, #36]
	strh	r3, [sp, #26]	@ movhi
	strh	r2, [sp, #28]	@ movhi
	ldr	r3, [r4, #1976]
	blx	r3
	subs	r5, r0, #0
	beq	.L2112
.L2089:
	movw	r0, #:lower16:.LC258
	ldrh	r1, [r6]
	movt	r0, #:upper16:.LC258
	bl	NAND_Print
.L2077:
	add	r1, sp, #24
	ldr	r3, [r4, #1996]
	mov	r0, r4
	blx	r3
	ldrh	r3, [r4, #20]
	mov	r1, #0
	mov	r2, #255
	strh	r1, [r6, #4]	@ movhi
	strh	r2, [r6, #2]	@ movhi
	add	r3, r3, #1
	strh	r3, [r4, #20]	@ movhi
.L2107:
	mov	r0, r5
	add	sp, sp, #44
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
.L2112:
	ldrh	r3, [r6, #6]
	mov	r1, #165
	ldr	r2, [r4, #4]
	mov	r7, r5
	ldr	r0, [r4, #284]
	add	r3, r3, #1
	strh	r3, [r6, #6]	@ movhi
	ldrh	r2, [r2, #26]
	bl	memset
	movw	r3, #42405
	movt	r3, 42405
	str	r3, [sp, #16]
	str	r3, [sp, #8]
	str	r3, [sp, #12]
	str	r3, [sp, #19]	@ unaligned
.L2079:
	ldr	r3, [r4, #4]
	add	lr, sp, #8
	ldrh	r2, [r6]
	add	r1, sp, #24
	mov	r0, r4
	strh	r7, [sp, #24]	@ movhi
	ldrh	ip, [r3, #28]
	ldr	r3, [r4, #284]
	str	lr, [sp, #36]
	strh	ip, [sp, #28]	@ movhi
	str	r3, [sp, #32]
	strh	r2, [sp, #26]	@ movhi
	ldr	r3, [r4, #1984]
	blx	r3
	subs	r5, r0, #0
	bne	.L2078
	add	r7, r7, #1
	cmp	r7, #3
	bne	.L2079
	ldr	r3, [r4, #4]
	mov	r8, r5
	ldr	r0, [r4, #284]
	mov	r9, r5
	add	r7, sp, #23
	ldrh	r2, [r3, #26]
.L2095:
	mov	r1, #0
	bl	memset
	ldr	r3, [r4, #4]
	add	lr, sp, #8
	ldrh	r2, [r6]
	add	r1, sp, #24
	str	r9, [sp, #16]
	mov	r0, r4
	ldrh	ip, [r3, #28]
	ldr	r3, [r4, #284]
	str	r9, [sp, #8]
	str	r9, [sp, #12]
	str	r3, [sp, #32]
	str	r9, [sp, #19]	@ unaligned
	strh	r8, [sp, #24]	@ movhi
	str	lr, [sp, #36]
	strh	ip, [sp, #28]	@ movhi
	strh	r2, [sp, #26]	@ movhi
	ldr	r3, [r4, #1980]
	blx	r3
	subs	r5, r0, #0
	bne	.L2080
	ldr	lr, [r4, #4]
	ldr	r0, [r4, #284]
	ldrh	r2, [lr, #26]
	cmp	r2, #0
	beq	.L2081
	ldrb	r3, [r0]	@ zero_extendqisi2
	cmp	r3, #165
	bne	.L2082
	add	r1, r0, #1
	add	ip, r0, r2
.L2087:
	cmp	ip, r1
	beq	.L2081
	ldrb	r3, [r1], #1	@ zero_extendqisi2
	cmp	r3, #165
	beq	.L2087
.L2082:
	mov	r1, #165
	movw	r0, #:lower16:.LC263
	ldrh	r2, [r6]
	mov	r3, r8
	str	r1, [sp]
	movt	r0, #:upper16:.LC263
	ldr	r1, .L2114
	mov	r5, #1
	bl	NAND_Print
	b	.L2077
.L2081:
	add	r1, sp, #8
.L2085:
	ldrb	ip, [r1], #1	@ zero_extendqisi2
	cmp	ip, #165
	bne	.L2084
	cmp	r7, r1
	bne	.L2085
	add	r8, r8, #1
	cmp	r8, #3
	bne	.L2095
	ldrh	ip, [lr, #28]
	mov	r3, #0
	ldrh	r2, [r6]
	add	r1, sp, #24
	strh	r3, [sp, #24]	@ movhi
	mov	r0, r4
	str	r3, [sp, #32]
	str	r3, [sp, #36]
	strh	ip, [sp, #28]	@ movhi
	strh	r2, [sp, #26]	@ movhi
	ldr	r3, [r4, #1976]
	blx	r3
	subs	r5, r0, #0
	bne	.L2089
	ldrh	r0, [r6, #6]
	mvn	r3, #0
	str	r5, [r6, #20]
	add	r2, r4, #152
	str	r5, [r6, #24]
	add	r0, r0, #1
	str	r3, [r6, #8]
	uxth	r0, r0
	strh	r0, [r6, #6]	@ movhi
	b	.L2091
.L2093:
	ldrh	r1, [r3, #6]
	cmp	r0, r1
	bls	.L2113
	mov	r2, r3
.L2091:
	ldr	r3, [r2, #20]
	cmp	r3, #0
	bne	.L2093
	str	r6, [r2, #20]
	str	r3, [r6, #20]
	str	r2, [r6, #24]
.L2092:
	ldrh	r3, [r4, #22]
	mov	r1, r6
	mov	r0, r4
	add	r3, r3, #1
	strh	r3, [r4, #22]	@ movhi
	bl	mark_free_block
	ldr	r2, [r4, #4]
	mov	r3, #0
	mov	r5, r3
	ldrh	r2, [r2, #20]
	strh	r3, [r6, #2]	@ movhi
	strh	r2, [r6, #4]	@ movhi
	b	.L2107
.L2084:
	mov	r1, #165
	movw	r0, #:lower16:.LC264
	ldrh	r2, [r6]
	mov	r3, r8
	str	r1, [sp]
	movt	r0, #:upper16:.LC264
	ldr	r1, .L2114
	mov	r5, #1
	bl	NAND_Print
	b	.L2077
.L2113:
	str	r6, [r3, #24]
	str	r3, [r6, #20]
	str	r6, [r2, #20]
	str	r2, [r6, #24]
	b	.L2092
.L2078:
	movw	r0, #:lower16:.LC261
	mov	r3, r7
	ldrh	r2, [r6]
	movt	r0, #:upper16:.LC261
	ldr	r1, .L2114
	bl	NAND_Print
	b	.L2077
.L2080:
	movw	r0, #:lower16:.LC262
	mov	r3, r8
	ldrh	r2, [r6]
	movt	r0, #:upper16:.LC262
	ldr	r1, .L2114
	bl	NAND_Print
	b	.L2077
.L2115:
	.align	2
.L2114:
	.word	.LANCHOR2+764
	.size	torture_block_and_mark, .-torture_block_and_mark
	.align	2
	.global	_nftl_nand_copy_page
	.syntax unified
	.arm
	.type	_nftl_nand_copy_page, %function
_nftl_nand_copy_page:
	@ args = 4, pretend = 0, frame = 48
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #60
	ldr	lr, [r0, #4]
	mov	r8, r1
	mov	r9, r2
	add	r7, sp, #8
	ldrh	ip, [r2]
	mov	r4, r0
	ldrh	r2, [lr, #28]
	ldrh	r1, [r1]
	str	r3, [sp, #32]
	str	r3, [sp, #48]
	ldrh	r3, [sp, #88]
	strh	r2, [sp, #28]	@ movhi
	strh	r2, [sp, #44]	@ movhi
	strh	r1, [sp, #26]	@ movhi
	add	r1, sp, #24
	strh	ip, [sp, #42]	@ movhi
	strh	r3, [sp, #24]	@ movhi
	strh	r3, [sp, #40]	@ movhi
	ldr	r2, [r0, #1980]
	str	r7, [sp, #36]
	str	r7, [sp, #52]
	blx	r2
	ldrb	r6, [sp, #10]	@ zero_extendqisi2
	cmp	r0, #10
	ldrb	r2, [sp, #9]	@ zero_extendqisi2
	movne	r5, r0
	ldrh	ip, [r9, #6]
	mov	r1, r7
	ldrb	lr, [sp, #11]	@ zero_extendqisi2
	mov	r0, r4
	ldr	r3, [r8, #8]
	moveq	r5, #0
	orr	r2, r6, r2, lsl #8
	ldrb	r6, [sp, #12]	@ zero_extendqisi2
	add	ip, ip, #1
	uxth	ip, ip
	orr	r2, lr, r2, lsl #8
	str	ip, [sp]
	orr	r2, r6, r2, lsl #8
	bl	set_oob_special_page
	ldr	r3, [r4, #1984]
	mov	r0, r4
	add	r1, sp, #40
	blx	r3
	orr	r0, r5, r0, lsl #8
	add	sp, sp, #60
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
	.size	_nftl_nand_copy_page, .-_nftl_nand_copy_page
	.align	2
	.global	set_no_use_page
	.syntax unified
	.arm
	.type	set_no_use_page, %function
set_no_use_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mvn	r0, #0
	mvn	r2, #85
	mvn	r3, #68
	strb	r0, [r1]
	strb	r2, [r1, #1]
	strb	r3, [r1, #2]
	bx	lr
	.size	set_no_use_page, .-set_no_use_page
	.align	2
	.global	set_start_block_flag
	.syntax unified
	.arm
	.type	set_start_block_flag, %function
set_start_block_flag:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	lsr	ip, r2, #24
	lsr	r0, r2, #16
	lsr	r3, r2, #8
	strb	r2, [r1, #6]
	str	lr, [sp, #-4]!
	strb	ip, [r1, #3]
	mvn	lr, #0
	mvn	ip, #85
	mvn	r2, #17
	strb	lr, [r1]
	strb	ip, [r1, #1]
	strb	r2, [r1, #2]
	strb	r0, [r1, #4]
	strb	r3, [r1, #5]
	ldr	pc, [sp], #4
	.size	set_start_block_flag, .-set_start_block_flag
	.align	2
	.global	set_end_block_flag
	.syntax unified
	.arm
	.type	set_end_block_flag, %function
set_end_block_flag:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mvn	r0, #0
	mvn	r2, #85
	mvn	r3, #51
	strb	r0, [r1]
	strb	r2, [r1, #1]
	strb	r3, [r1, #2]
	bx	lr
	.size	set_end_block_flag, .-set_end_block_flag
	.align	2
	.global	build_zone_list_first
	.syntax unified
	.arm
	.type	build_zone_list_first, %function
build_zone_list_first:
	@ args = 0, pretend = 0, frame = 40
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	mov	r4, r0
	movw	r0, #:lower16:.LC265
	sub	sp, sp, #44
	movt	r0, #:upper16:.LC265
	str	r1, [sp, #4]
	bl	NAND_Print_DBG
	ldr	r3, [r4, #24]
	ldr	r5, [r4, #40]
	cmp	r3, #0
	beq	.L2124
	ldr	r6, [r4, #4]
	ldr	r0, [r4, #32]
	ldrb	r1, [r6, #8]	@ zero_extendqisi2
	add	r0, r3, r0
	bl	__aeabi_uidiv
	ldrh	r1, [r6, #20]
	bl	__aeabi_uidiv
	subs	r8, r0, #0
	mov	r3, #0
	strh	r3, [r4, #18]	@ movhi
	beq	.L2144
.L2143:
	add	r5, r5, #36
	mov	r7, #0
	mov	fp, #255
	b	.L2142
.L2126:
	ldrh	r1, [r5, #-36]
	ldr	r2, [r4, #4]
	strh	fp, [r5, #-34]	@ movhi
.L2128:
	mvn	r3, #0
	str	r7, [r5, #-16]
	strh	r3, [r5, #-32]	@ movhi
	ldrh	ip, [r2, #24]
	str	r7, [r5, #-24]
	str	r7, [r5, #-20]
	add	r0, ip, r3
	str	r6, [r4, #44]
	cmp	r1, r0
	ldrh	r1, [r4, #18]
	beq	.L2141
	cmp	r1, r8
	add	r5, r5, #36
	bcs	.L2145
	ldr	r6, [r4, #4]
.L2142:
	ldrh	r2, [r5, #-36]
	add	r1, sp, #24
	ldrh	ip, [r6, #28]
	sub	r6, r5, #36
	ldr	r3, [r4, #284]
	mov	r0, r4
	strh	r2, [sp, #26]	@ movhi
	add	r2, sp, #8
	strh	ip, [sp, #28]	@ movhi
	mov	r9, r5
	str	r3, [sp, #32]
	strh	r7, [sp, #24]	@ movhi
	str	r2, [sp, #36]
	str	r6, [r4, #44]
	ldr	r3, [r4, #1992]
	blx	r3
	cmp	r0, #1
	bne	.L2126
	ldr	r3, [r4, #1976]
	add	r1, sp, #24
	mov	r0, r4
	blx	r3
	subs	r10, r0, #0
	bne	.L2153
	ldrh	r2, [r4, #18]
	add	r2, r2, #1
	uxth	r2, r2
	cmp	r2, #1
	strh	r2, [r4, #18]	@ movhi
	strh	r10, [r5, #-34]	@ movhi
	strh	r10, [r5, #-30]	@ movhi
	beq	.L2129
	ldrh	r1, [r5, #-36]
	ldr	r2, [r4, #4]
	b	.L2128
.L2145:
	mov	r6, r9
.L2141:
	cmp	r1, #1
	bls	.L2125
	cmn	r8, #1
	sub	r1, r1, #1
	strh	r1, [r4, #18]	@ movhi
	beq	.L2136
	ldrh	r3, [r6]
	cmp	r0, r3
	beq	.L2136
	cmp	ip, r3
	bls	.L2136
	mov	r5, #0
	mov	r7, #255
	b	.L2140
.L2139:
	ldr	r2, [r4, #4]
	ldrh	r3, [r6, #36]!
	ldrh	r1, [r2, #24]
	cmp	r1, r3
	bls	.L2136
.L2140:
	ldrh	ip, [r2, #28]
	add	r1, sp, #24
	ldr	r2, [r4, #284]
	mov	r0, r4
	strh	r3, [sp, #26]	@ movhi
	add	r3, sp, #8
	str	r3, [sp, #36]
	strh	r5, [sp, #24]	@ movhi
	strh	ip, [sp, #28]	@ movhi
	str	r2, [sp, #32]
	ldr	r3, [r4, #1992]
	blx	r3
	cmp	r0, #1
	strhne	r7, [r6, #2]	@ movhi
	bne	.L2139
	add	r1, sp, #24
	ldr	r3, [r4, #1976]
	mov	r0, r4
	blx	r3
	subs	r8, r0, #0
	add	r1, sp, #24
	mov	r0, r4
	beq	.L2138
	strh	r7, [r6, #2]	@ movhi
	ldr	r3, [r4, #1996]
	blx	r3
	b	.L2139
.L2136:
	mov	r0, #0
.L2151:
	add	sp, sp, #44
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L2124:
	strh	r3, [r4, #18]	@ movhi
	mvn	r8, #0
	ldr	r6, [r4, #4]
	b	.L2143
.L2153:
	add	r1, sp, #24
	strh	fp, [r5, #-34]	@ movhi
	ldr	r3, [r4, #1996]
	mov	r0, r4
	blx	r3
	ldrh	r1, [r5, #-36]
	ldr	r2, [r4, #4]
	b	.L2128
.L2129:
	mov	r2, #16
	mov	r1, #255
	add	r0, sp, #8
	bl	NAND_Memset
	ldr	r2, [sp, #4]
	add	r1, sp, #8
	mov	r0, r4
	bl	set_start_block_flag
	ldr	r2, [r4, #4]
	add	r3, sp, #8
	ldrh	r1, [r5, #-36]
	strh	r10, [sp, #24]	@ movhi
	ldrh	r0, [r2, #20]
	ldrh	ip, [r2, #28]
	strh	r1, [sp, #26]	@ movhi
	cmp	r0, #0
	ldr	r0, [r4, #284]
	strh	ip, [sp, #28]	@ movhi
	str	r3, [sp, #36]
	str	r0, [sp, #32]
	beq	.L2130
.L2131:
	ldr	r2, [r4, #1984]
	add	r1, sp, #24
	mov	r0, r4
	add	r10, r10, #1
	blx	r2
	ldr	r2, [r4, #4]
	ldrh	r1, [sp, #24]
	ldrh	r0, [r2, #20]
	add	r1, r1, #1
	strh	r1, [sp, #24]	@ movhi
	cmp	r0, r10
	bhi	.L2131
	ldrh	r1, [r5, #-36]
.L2130:
	str	r5, [r4, #40]
	b	.L2128
.L2144:
	mov	r1, r8
.L2125:
	movw	r0, #:lower16:.LC266
	movt	r0, #:upper16:.LC266
	bl	NAND_Print
	mov	r0, #1
	b	.L2151
.L2138:
	mov	r2, #16
	mov	r1, #255
	add	r0, sp, #8
	bl	NAND_Memset
	add	r1, sp, #8
	mov	r0, r4
	bl	set_end_block_flag
	ldr	r2, [r4, #4]
	add	r1, sp, #24
	ldrh	ip, [r6]
	mov	r0, r4
	ldr	r3, [r4, #284]
	ldrh	r2, [r2, #28]
	strh	ip, [sp, #26]	@ movhi
	add	ip, sp, #8
	str	r3, [sp, #32]
	strh	r8, [sp, #24]	@ movhi
	str	ip, [sp, #36]
	strh	r2, [sp, #28]	@ movhi
	ldr	r3, [r4, #1984]
	blx	r3
	mov	r0, r8
	b	.L2151
	.size	build_zone_list_first, .-build_zone_list_first
	.align	2
	.global	get_bad_flag_from_oob
	.syntax unified
	.arm
	.type	get_bad_flag_from_oob, %function
get_bad_flag_from_oob:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, #0
	add	r1, r0, #4
.L2155:
	ldrb	r2, [r0, #1]!	@ zero_extendqisi2
	cmp	r1, r0
	orr	r3, r2, r3, lsl #8
	bne	.L2155
	adds	r0, r3, #0
	movne	r0, #1
	bx	lr
	.size	get_bad_flag_from_oob, .-get_bad_flag_from_oob
	.align	2
	.global	get_block_used_count_from_oob
	.syntax unified
	.arm
	.type	get_block_used_count_from_oob, %function
get_block_used_count_from_oob:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	add	r3, r0, #6
	mov	r2, #0
	add	r0, r0, #10
.L2158:
	ldrb	r1, [r3, #1]!	@ zero_extendqisi2
	cmp	r3, r0
	orr	r2, r1, r2, lsl #8
	bne	.L2158
	mov	r0, r2
	bx	lr
	.size	get_block_used_count_from_oob, .-get_block_used_count_from_oob
	.align	2
	.global	get_logic_page_from_oob
	.syntax unified
	.arm
	.type	get_logic_page_from_oob, %function
get_logic_page_from_oob:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, #0
	add	r1, r0, #4
.L2161:
	ldrb	r2, [r0, #1]!	@ zero_extendqisi2
	cmp	r1, r0
	orr	r3, r2, r3, lsl #8
	bne	.L2161
	bic	r0, r3, #-268435456
	bx	lr
	.size	get_logic_page_from_oob, .-get_logic_page_from_oob
	.align	2
	.global	get_special_data_from_oob
	.syntax unified
	.arm
	.type	get_special_data_from_oob, %function
get_special_data_from_oob:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, r0
	mov	r0, #0
	add	r1, r3, #4
.L2164:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r3, r1
	orr	r0, r2, r0, lsl #8
	bne	.L2164
	bx	lr
	.size	get_special_data_from_oob, .-get_special_data_from_oob
	.align	2
	.global	get_erase_count_from_oob
	.syntax unified
	.arm
	.type	get_erase_count_from_oob, %function
get_erase_count_from_oob:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldrb	r2, [r0, #5]	@ zero_extendqisi2
	ldrb	r0, [r0, #6]	@ zero_extendqisi2
	orr	r0, r0, r2, lsl #8
	bx	lr
	.size	get_erase_count_from_oob, .-get_erase_count_from_oob
	.align	2
	.global	is_ftl_start_flag_page
	.syntax unified
	.arm
	.type	is_ftl_start_flag_page, %function
is_ftl_start_flag_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldrb	r3, [r0, #1]	@ zero_extendqisi2
	cmp	r3, #170
	ldrbeq	r0, [r0, #2]	@ zero_extendqisi2
	subeq	r0, r0, #238
	clzeq	r0, r0
	lsreq	r0, r0, #5
	movne	r0, #0
	bx	lr
	.size	is_ftl_start_flag_page, .-is_ftl_start_flag_page
	.align	2
	.global	is_ftl_end_flag_page
	.syntax unified
	.arm
	.type	is_ftl_end_flag_page, %function
is_ftl_end_flag_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldrb	r3, [r0, #1]	@ zero_extendqisi2
	cmp	r3, #170
	ldrbeq	r0, [r0, #2]	@ zero_extendqisi2
	subeq	r0, r0, #204
	clzeq	r0, r0
	lsreq	r0, r0, #5
	movne	r0, #0
	bx	lr
	.size	is_ftl_end_flag_page, .-is_ftl_end_flag_page
	.align	2
	.global	is_ftl_special_data
	.syntax unified
	.arm
	.type	is_ftl_special_data, %function
is_ftl_special_data:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldrb	r0, [r0, #1]	@ zero_extendqisi2
	sub	r0, r0, #170
	clz	r0, r0
	lsr	r0, r0, #5
	bx	lr
	.size	is_ftl_special_data, .-is_ftl_special_data
	.align	2
	.global	is_ftl_logic_page_data
	.syntax unified
	.arm
	.type	is_ftl_logic_page_data, %function
is_ftl_logic_page_data:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldrb	r0, [r0, #1]	@ zero_extendqisi2
	and	r0, r0, #240
	sub	r0, r0, #192
	clz	r0, r0
	lsr	r0, r0, #5
	bx	lr
	.size	is_ftl_logic_page_data, .-is_ftl_logic_page_data
	.align	2
	.global	is_nouse_page
	.syntax unified
	.arm
	.type	is_nouse_page, %function
is_nouse_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	ip, #0
	sub	r3, r0, #1
	add	r1, r0, #9
.L2177:
	ldrb	r2, [r3, #1]!	@ zero_extendqisi2
	cmp	r2, #255
	addeq	ip, ip, #1
	cmp	r3, r1
	bne	.L2177
	cmp	ip, #8
	ldrbhi	r0, [r0]	@ zero_extendqisi2
	subhi	r0, r0, #255
	clzhi	r0, r0
	lsrhi	r0, r0, #5
	movls	r0, #0
	bx	lr
	.size	is_nouse_page, .-is_nouse_page
	.align	2
	.global	is_fill_page
	.syntax unified
	.arm
	.type	is_fill_page, %function
is_fill_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, #0
	add	r1, r0, #4
.L2182:
	ldrb	r2, [r0, #1]!	@ zero_extendqisi2
	cmp	r1, r0
	orr	r3, r2, r3, lsl #8
	bne	.L2182
	movw	r0, #21845
	movt	r0, 21845
	sub	r0, r3, r0
	clz	r0, r0
	lsr	r0, r0, #5
	bx	lr
	.size	is_fill_page, .-is_fill_page
	.align	2
	.global	is_phy_mapping_page
	.syntax unified
	.arm
	.type	is_phy_mapping_page, %function
is_phy_mapping_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, #0
	add	r1, r0, #4
.L2185:
	ldrb	r2, [r0, #1]!	@ zero_extendqisi2
	cmp	r1, r0
	orr	r3, r2, r3, lsl #8
	bne	.L2185
	movw	r0, #65535
	movt	r0, 43690
	sub	r0, r3, r0
	clz	r0, r0
	lsr	r0, r0, #5
	bx	lr
	.size	is_phy_mapping_page, .-is_phy_mapping_page
	.align	2
	.global	is_function_info_page
	.syntax unified
	.arm
	.type	is_function_info_page, %function
is_function_info_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, #0
	add	r1, r0, #4
.L2188:
	ldrb	r2, [r0, #1]!	@ zero_extendqisi2
	cmp	r1, r0
	orr	r3, r2, r3, lsl #8
	bne	.L2188
	lsr	r3, r3, #16
	mov	r0, #0
	movt	r0, 43537
	lsl	r3, r3, #16
	sub	r0, r3, r0
	clz	r0, r0
	lsr	r0, r0, #5
	bx	lr
	.size	is_function_info_page, .-is_function_info_page
	.align	2
	.global	is_power_down_info_page
	.syntax unified
	.arm
	.type	is_power_down_info_page, %function
is_power_down_info_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, #0
	add	r1, r0, #4
.L2191:
	ldrb	r2, [r0, #1]!	@ zero_extendqisi2
	cmp	r1, r0
	orr	r3, r2, r3, lsl #8
	bne	.L2191
	movw	r0, #13107
	movt	r0, 43537
	sub	r0, r3, r0
	clz	r0, r0
	lsr	r0, r0, #5
	bx	lr
	.size	is_power_down_info_page, .-is_power_down_info_page
	.align	2
	.global	get_logic_page_map
	.syntax unified
	.arm
	.type	get_logic_page_map, %function
get_logic_page_map:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #28]
	cmp	r3, r1
	bls	.L2194
	cmp	r1, #262144
	movcc	r3, #0
	subcs	r3, r1, #262144
	ubfxcs	r1, r1, #0, #18
	lsrcs	r3, r3, #18
	addcs	r3, r3, #1
.L2196:
	add	r0, r0, r3, lsl #2
	ldr	r0, [r0, #52]
	add	r0, r0, r1, lsl #2
	bx	lr
.L2194:
	movw	r0, #:lower16:.LC267
	mov	r2, r1
	push	{r4, lr}
	movt	r0, #:upper16:.LC267
	ldr	r1, .L2200
	bl	NAND_Print
	mov	r0, #0
	pop	{r4, pc}
.L2201:
	.align	2
.L2200:
	.word	.LANCHOR2+788
	.size	get_logic_page_map, .-get_logic_page_map
	.align	2
	.global	recover_logic_page_mapping
	.syntax unified
	.arm
	.type	recover_logic_page_mapping, %function
recover_logic_page_mapping:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	cmp	r3, #0
	beq	.L2221
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	mov	r6, r1
	mov	r10, r3
	movw	r1, #:lower16:.LC269
	movw	r3, #:lower16:.LC271
	sub	sp, sp, #20
	movw	fp, #:lower16:.LC270
	movt	r3, #:upper16:.LC271
	movt	r1, #:upper16:.LC269
	mov	r5, r0
	sub	r7, r2, #4
	str	r3, [sp]
	movt	fp, #:upper16:.LC270
	str	r1, [sp, #12]
	mov	r4, #0
	b	.L2213
.L2204:
	cmn	r3, #1
	beq	.L2212
	mov	r2, r4
	ldrh	r1, [r6]
	mov	r0, fp
	bl	NAND_Print
.L2212:
	ldrh	r3, [r6, #4]
	ldr	r2, [r5, #4]
	add	r3, r3, #1
	ldrh	r2, [r2, #20]
	uxth	r3, r3
	strh	r3, [r6, #4]	@ movhi
	cmp	r2, r3
	bcc	.L2222
.L2206:
	add	r4, r4, #1
	cmp	r10, r4
	beq	.L2223
.L2213:
	ldr	r3, [r7, #4]!
	ldr	r2, [r5, #28]
	cmp	r3, r2
	bcs	.L2204
	mov	r1, r3
	mov	r0, r5
	bl	get_logic_page_map
	subs	r9, r0, #0
	beq	.L2224
	ldrh	r8, [r9, #2]
	movw	r3, #65535
	cmp	r8, r3
	beq	.L2208
	ldr	r1, [r5, #4]
	add	r8, r8, r8, lsl #3
	lsl	r8, r8, #2
	ldr	r2, [r1, #32]
	add	r3, r2, r8
	cmp	r6, r3
	beq	.L2209
	mov	r1, r3
	mov	r0, r5
	str	r2, [sp, #8]
	str	r3, [sp, #4]
	bl	phy_block_from_invalid_page_incr
	ldr	r3, [sp, #4]
	ldr	r1, [r5, #4]
	ldr	r2, [sp, #8]
	ldrh	r0, [r3, #4]
	ldrh	r3, [r1, #20]
	cmp	r0, r3
	bhi	.L2225
.L2208:
	ldrh	r3, [r9]
	bfi	r3, r4, #0, #12
	add	r4, r4, #1
	strh	r3, [r9]	@ movhi
	cmp	r10, r4
	ldrh	r3, [r6]
	strh	r3, [r9, #2]	@ movhi
	bne	.L2213
.L2223:
	mov	r0, #0
	add	sp, sp, #20
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L2222:
	ldr	r0, [sp]
	bl	NAND_Print
	b	.L2206
.L2209:
	ldrh	r0, [r6, #4]
	add	r0, r0, #1
	strh	r0, [r6, #4]	@ movhi
	ldrh	r0, [r3, #4]
	ldrh	r3, [r1, #20]
	cmp	r0, r3
	bls	.L2208
.L2225:
	ldrh	r1, [r2, r8]
	ldr	r0, [sp, #12]
	bl	NAND_Print
	b	.L2208
.L2224:
	movw	r0, #:lower16:.LC268
	movt	r0, #:upper16:.LC268
	bl	NAND_Print_DBG
	ldrh	r3, [r6, #4]
	add	r3, r3, #1
	strh	r3, [r6, #4]	@ movhi
	b	.L2206
.L2221:
	mov	r0, #0
	bx	lr
	.size	recover_logic_page_mapping, .-recover_logic_page_mapping
	.align	2
	.global	malloc_logic_page_map
	.syntax unified
	.arm
	.type	malloc_logic_page_map, %function
malloc_logic_page_map:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	add	r5, r0, #48
	mov	r4, r1
	mov	r7, r0
	add	r1, r0, #148
	mov	r3, r5
	mov	r2, #0
.L2227:
	str	r2, [r3, #4]!
	cmp	r3, r1
	bne	.L2227
	cmp	r4, #262144
	bcc	.L2235
	ubfx	r8, r4, #0, #18
	sub	r9, r4, #6553600
	mov	r6, #0
	b	.L2229
.L2232:
	cmp	r4, r8
	beq	.L2228
.L2229:
	mov	r0, #1048576
	sub	r4, r4, #262144
	bl	NAND_Malloc
	cmp	r0, #0
	mov	r2, #1048576
	mov	r1, #255
	str	r0, [r5, #4]!
	beq	.L2241
	bl	NAND_Memset
	cmp	r4, r9
	add	r6, r6, #1
	bne	.L2232
	cmp	r4, #0
	bne	.L2243
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L2235:
	mov	r8, r4
	mov	r6, #0
.L2228:
	lsl	r8, r8, #2
	add	r7, r7, r6, lsl #2
	mov	r0, r8
	bl	nftl_malloc
	cmp	r0, #0
	str	r0, [r7, #52]
	beq	.L2241
	mov	r2, r8
	mov	r1, #255
	bl	NAND_Memset
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L2241:
	movw	r0, #:lower16:.LC272
	mov	r1, r6
	movt	r0, #:upper16:.LC272
	bl	NAND_Print
	mvn	r0, #1
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L2243:
	movw	r0, #:lower16:.LC272
	mov	r1, r4
	movt	r0, #:upper16:.LC272
	bl	NAND_Print
	mvn	r0, #1
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
	.size	malloc_logic_page_map, .-malloc_logic_page_map
	.align	2
	.global	zone_param_init
	.syntax unified
	.arm
	.type	zone_param_init, %function
zone_param_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r5, r1
	ldr	r1, [r0, #4]
	mov	r4, r0
	mov	r6, r2
	mov	r7, r3
	ldrh	r0, [r1, #26]
	bl	nftl_malloc
	cmp	r0, #0
	str	r0, [r4, #284]
	beq	.L2247
	ldr	r3, [r4, #4]
	ldrh	r0, [r3, #26]
	bl	nftl_malloc
	cmp	r0, #0
	str	r0, [r4, #288]
	beq	.L2247
	ldr	r1, [r4, #4]
	add	ip, r5, r5, lsl #3
	mov	r3, #0
	str	r6, [r4, #24]
	str	r7, [r4, #32]
	mov	r0, r6
	ldr	r2, [r1, #32]
	strh	r5, [r4, #36]	@ movhi
	strh	r3, [r4, #18]	@ movhi
	strh	r3, [r4, #20]	@ movhi
	add	r2, r2, ip, lsl #2
	strh	r3, [r4, #22]	@ movhi
	str	r3, [r4, #44]
	str	r2, [r4, #40]
	ldrb	r1, [r1, #8]	@ zero_extendqisi2
	bl	__aeabi_uidiv
	mov	r1, r0
	mov	r0, r4
	bl	malloc_logic_page_map
	subs	r5, r0, #0
	bne	.L2258
	ldr	r3, [r4, #4]
	mov	r2, #320
	ldrh	r3, [r3, #20]
	str	r2, [r4, #316]
	lsl	r3, r3, #2
	add	r0, r3, r2
	str	r3, [r4, #308]
	str	r0, [r4, #300]
	bl	nftl_malloc
	cmp	r0, #0
	str	r0, [r4, #296]
	beq	.L2259
	ldr	ip, [r4, #308]
	mov	r1, #255
	str	r0, [r4, #304]
	ldr	r2, [r4, #300]
	add	r3, r0, ip
	str	r3, [r4, #312]
	str	r3, [r4, #1968]
	bl	NAND_Memset
	ldr	r3, [r4, #308]
	ldr	r0, [r4, #300]
	str	r3, [r4, #340]
	str	r0, [r4, #332]
	bl	nftl_malloc
	cmp	r0, #0
	str	r0, [r4, #328]
	beq	.L2260
	ldr	ip, [r4, #340]
	mov	r1, #255
	str	r0, [r4, #336]
	ldr	r2, [r4, #332]
	add	r3, r0, ip
	str	r3, [r4, #344]
	bl	NAND_Memset
	ldr	r2, [r4, #308]
	ldr	r3, [r4, #316]
	ldr	r0, [r4, #300]
	str	r2, [r4, #272]
	str	r3, [r4, #280]
	str	r0, [r4, #264]
	bl	nftl_malloc
	cmp	r0, #0
	str	r0, [r4, #260]
	beq	.L2261
	ldr	r2, [r4, #4]
	movw	r6, #43691
	movt	r6, 43690
	ldr	r1, [r4, #272]
	str	r0, [r4, #268]
	add	r3, r4, #396
	ldrh	r2, [r2, #16]
	add	r0, r0, r1
	str	r5, [r4, #356]
	str	r0, [r4, #276]
	add	r0, r4, #572
	str	r5, [r4, #376]
	mov	r1, r5
	umull	r6, r7, r2, r6
	lsr	ip, r2, #1
	str	r5, [r4, #360]
	mov	r2, r5
	str	r5, [r4, #364]
	mvn	r6, #0
	str	r5, [r4, #368]
	str	r5, [r4, #372]
	lsr	lr, r7, #2
	strh	ip, [r0]	@ movhi
	movw	ip, #574
	add	r0, r4, #576
	strh	lr, [r4, ip]	@ movhi
	movw	r7, #578
	strh	r5, [r0]	@ movhi
	add	lr, r4, #320
	add	ip, r4, #352
	add	r0, r4, #392
	str	r5, [r4, #580]
	str	r5, [r4, #292]
	strh	r5, [r4, r7]	@ movhi
	strh	r6, [lr]	@ movhi
	str	r5, [r4, #324]
	strh	r6, [ip]	@ movhi
	str	r5, [r4, #172]
	str	r5, [r4, #176]
	str	r5, [r4, #164]
	str	r5, [r4, #168]
	str	r5, [r4, #180]
	str	r5, [r4, #184]
	str	r5, [r4, #208]
	str	r5, [r4, #212]
	str	r5, [r4, #200]
	str	r5, [r4, #204]
	str	r5, [r4, #216]
	str	r5, [r4, #220]
	str	r5, [r4, #244]
	str	r5, [r4, #248]
	str	r5, [r4, #236]
	str	r5, [r4, #240]
	str	r5, [r4, #252]
	str	r5, [r4, #256]
	strh	r5, [r0]	@ movhi
.L2252:
	strh	r1, [r3]	@ movhi
	add	r1, r1, #1
	cmp	r1, #10
	strh	r2, [r3, #2]	@ movhi
	str	r2, [r3, #4]
	add	r3, r3, #16
	str	r2, [r3, #-8]
	str	r2, [r3, #-4]
	bne	.L2252
	add	r0, r4, #556
	mov	ip, #255
	movw	r1, #394
	movw	r3, #558
	strh	ip, [r0]	@ movhi
	str	r2, [r4, #560]
	str	r2, [r4, #564]
	str	r2, [r4, #568]
	strh	r2, [r4, r1]	@ movhi
	strh	r2, [r4, r3]	@ movhi
.L2246:
	mov	r0, r5
	pop	{r4, r5, r6, r7, r8, pc}
.L2258:
	movw	r0, #:lower16:.LC274
	mvn	r5, #1
	movt	r0, #:upper16:.LC274
	bl	NAND_Print
	b	.L2246
.L2247:
	movw	r0, #:lower16:.LC273
	mvn	r5, #1
	movt	r0, #:upper16:.LC273
	bl	NAND_Print
	b	.L2246
.L2260:
	movw	r0, #:lower16:.LC276
	mvn	r5, #1
	movt	r0, #:upper16:.LC276
	bl	NAND_Print
	b	.L2246
.L2259:
	movw	r0, #:lower16:.LC275
	mvn	r5, #1
	movt	r0, #:upper16:.LC275
	bl	NAND_Print
	b	.L2246
.L2261:
	movw	r0, #:lower16:.LC277
	mvn	r5, #1
	movt	r0, #:upper16:.LC277
	bl	NAND_Print
	b	.L2246
	.size	zone_param_init, .-zone_param_init
	.align	2
	.global	free_logic_page_map
	.syntax unified
	.arm
	.type	free_logic_page_map, %function
free_logic_page_map:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	add	r4, r0, #48
	add	r5, r0, #148
.L2264:
	mov	r1, #0
	ldr	r0, [r4, #4]!
	cmp	r0, r1
	beq	.L2263
	bl	NAND_Free
.L2263:
	cmp	r4, r5
	bne	.L2264
	mov	r0, #0
	pop	{r4, r5, r6, pc}
	.size	free_logic_page_map, .-free_logic_page_map
	.align	2
	.global	set_physic_op_par
	.syntax unified
	.arm
	.type	set_physic_op_par, %function
set_physic_op_par:
	@ args = 8, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	str	lr, [sp, #-4]!
	ldr	lr, [sp, #4]
	ldr	ip, [sp, #8]
	strh	r2, [r0]	@ movhi
	strh	r1, [r0, #2]	@ movhi
	strh	r3, [r0, #4]	@ movhi
	str	lr, [r0, #8]
	str	ip, [r0, #12]
	ldr	pc, [sp], #4
	.size	set_physic_op_par, .-set_physic_op_par
	.align	2
	.global	nftl_sector_cache_init
	.syntax unified
	.arm
	.type	nftl_sector_cache_init, %function
nftl_sector_cache_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #1620
	push	{r4, r5, r6, lr}
	add	r2, r0, r3
	mov	r4, r0
	mov	r5, #0
	mov	r1, #16
	strh	r1, [r0, r3]	@ movhi
	mov	r0, #8192
	strh	r5, [r2, #2]	@ movhi
	str	r5, [r4, #1624]
	bl	NAND_Malloc
	cmp	r0, r5
	str	r0, [r4, #1624]
	beq	.L2278
	add	r3, r4, #1616
	mov	r2, r5
	add	r3, r3, #12
	mov	r1, r5
	mov	lr, #255
	mvn	ip, #0
	str	r5, [r4, #1960]
	str	r5, [r4, #1964]
.L2275:
	strh	r2, [r3]	@ movhi
	add	r2, r2, #1
	cmp	r2, #16
	str	r0, [r3, #8]
	strh	lr, [r3, #2]	@ movhi
	add	r0, r0, #512
	str	ip, [r3, #4]
	add	r3, r3, #20
	str	r1, [r3, #-8]
	str	r1, [r3, #-4]
	bne	.L2275
	mov	r0, #0
	pop	{r4, r5, r6, pc}
.L2278:
	movw	r0, #:lower16:.LC221
	movt	r0, #:upper16:.LC221
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, pc}
	.size	nftl_sector_cache_init, .-nftl_sector_cache_init
	.align	2
	.global	nftl_sector_cache_exit
	.syntax unified
	.arm
	.type	nftl_sector_cache_exit, %function
nftl_sector_cache_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r1, #0
	ldr	r0, [r0, #1624]
	bl	NAND_Free
	mov	r0, #0
	pop	{r4, pc}
	.size	nftl_sector_cache_exit, .-nftl_sector_cache_exit
	.align	2
	.global	search_from_sector_cache_write_list
	.syntax unified
	.arm
	.type	search_from_sector_cache_write_list, %function
search_from_sector_cache_write_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #340]
	cmp	r0, #0
	bxeq	lr
	ldr	r3, [r0, #4]
	cmp	r1, r3
	bne	.L2284
	b	.L2290
.L2285:
	ldr	r3, [r0, #4]
	cmp	r3, r1
	bxeq	lr
.L2284:
	ldr	r0, [r0, #12]
	cmp	r0, #0
	bne	.L2285
	bx	lr
.L2290:
	bx	lr
	.size	search_from_sector_cache_write_list, .-search_from_sector_cache_write_list
	.align	2
	.global	get_empty_sector_cahce_node
	.syntax unified
	.arm
	.type	get_empty_sector_cahce_node, %function
get_empty_sector_cahce_node:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, #0
	mov	r2, r0
.L2294:
	ldrh	r1, [r2, #10]
	add	r2, r2, #20
	cmp	r1, #255
	beq	.L2296
	add	r3, r3, #1
	cmp	r3, #16
	bne	.L2294
	mov	r0, #0
	bx	lr
.L2296:
	add	r3, r3, r3, lsl #2
	add	r3, r0, r3, lsl #2
	add	r0, r3, #8
	bx	lr
	.size	get_empty_sector_cahce_node, .-get_empty_sector_cahce_node
	.align	2
	.global	add_to_sector_cache_write_list_tail
	.syntax unified
	.arm
	.type	add_to_sector_cache_write_list_tail, %function
add_to_sector_cache_write_list_tail:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	mov	ip, r0
	add	r2, r0, #328
	b	.L2298
.L2299:
	mov	r2, r3
.L2298:
	ldr	r3, [r2, #12]
	cmp	r3, #0
	bne	.L2299
	push	{r4, lr}
	mov	r4, #1
	ldrh	lr, [ip, #2]
	mov	r0, r3
	str	r1, [r2, #12]
	str	r3, [r1, #12]
	add	r3, lr, r4
	str	r2, [r1, #16]
	strh	r4, [r1, #2]	@ movhi
	strh	r3, [ip, #2]	@ movhi
	pop	{r4, pc}
	.size	add_to_sector_cache_write_list_tail, .-add_to_sector_cache_write_list_tail
	.align	2
	.global	del_from_sector_cache_write_list
	.syntax unified
	.arm
	.type	del_from_sector_cache_write_list, %function
del_from_sector_cache_write_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r1, #12]
	mov	r2, r0
	ldr	r0, [r1, #16]
	mov	ip, #255
	str	r3, [r0, #12]
	ldr	r3, [r1, #12]
	cmp	r3, #0
	strne	r0, [r3, #16]
	mov	r0, #0
	ldrh	r3, [r2, #2]
	str	r0, [r1, #12]
	str	r0, [r1, #16]
	mov	r0, r1
	sub	r3, r3, #1
	strh	ip, [r1, #2]	@ movhi
	strh	r3, [r2, #2]	@ movhi
	bx	lr
	.size	del_from_sector_cache_write_list, .-del_from_sector_cache_write_list
	.align	2
	.global	get_empty_gc_node
	.syntax unified
	.arm
	.type	get_empty_gc_node, %function
get_empty_gc_node:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r2, #0
	mov	r3, r0
.L2309:
	ldrh	r1, [r3, #6]
	add	r3, r3, #16
	cmp	r1, #0
	beq	.L2311
	add	r2, r2, #1
	cmp	r2, #10
	bne	.L2309
	mov	r0, #0
	bx	lr
.L2311:
	add	r0, r0, r2, lsl #4
	add	r0, r0, #4
	bx	lr
	.size	get_empty_gc_node, .-get_empty_gc_node
	.align	2
	.global	search_gc_node
	.syntax unified
	.arm
	.type	search_gc_node, %function
search_gc_node:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #172]
	cmp	r0, #0
	bxeq	lr
	ldr	r3, [r0, #4]
	cmp	r1, r3
	bne	.L2315
	b	.L2321
.L2316:
	ldr	r3, [r0, #4]
	cmp	r3, r1
	bxeq	lr
.L2315:
	ldr	r0, [r0, #8]
	cmp	r0, #0
	bne	.L2316
	bx	lr
.L2321:
	bx	lr
	.size	search_gc_node, .-search_gc_node
	.align	2
	.global	add_to_gc_list_tail
	.syntax unified
	.arm
	.type	add_to_gc_list_tail, %function
add_to_gc_list_tail:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	mov	ip, r0
	add	r2, r0, #164
	b	.L2323
.L2324:
	mov	r2, r3
.L2323:
	ldr	r3, [r2, #8]
	cmp	r3, #0
	bne	.L2324
	str	lr, [sp, #-4]!
	mov	r0, r3
	ldrh	lr, [ip]
	str	r1, [r2, #8]
	str	r3, [r1, #8]
	add	r3, lr, #1
	str	r2, [r1, #12]
	strh	r3, [ip]	@ movhi
	ldr	pc, [sp], #4
	.size	add_to_gc_list_tail, .-add_to_gc_list_tail
	.align	2
	.global	del_from_gc_list
	.syntax unified
	.arm
	.type	del_from_gc_list, %function
del_from_gc_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r1, #8]
	mov	r2, r0
	ldr	r0, [r1, #12]
	mov	ip, #0
	str	r3, [r0, #8]
	ldr	r3, [r1, #8]
	cmp	r3, #0
	strne	r0, [r3, #12]
	mov	r0, r1
	ldrh	r3, [r2]
	str	ip, [r1, #8]
	str	ip, [r1, #12]
	sub	r3, r3, #1
	strh	r3, [r2]	@ movhi
	bx	lr
	.size	del_from_gc_list, .-del_from_gc_list
	.align	2
	.global	add_prio_gc
	.syntax unified
	.arm
	.type	add_prio_gc, %function
add_prio_gc:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	ldr	lr, [r0, #564]
	cmp	lr, #0
	beq	.L2332
	ldr	r3, [lr, #4]
	cmp	r1, r3
	beq	.L2342
	mov	r3, lr
	b	.L2335
.L2336:
	ldr	ip, [r3, #4]
	cmp	r1, ip
	beq	.L2342
.L2335:
	ldr	r3, [r3, #8]
	cmp	r3, #0
	bne	.L2336
.L2332:
	add	r3, r0, #396
	mov	ip, #0
	add	r3, r3, #2
.L2339:
	ldrh	r4, [r3]
	add	r3, r3, #16
	cmp	r4, #0
	beq	.L2348
	add	ip, ip, #1
	cmp	ip, #10
	bne	.L2339
.L2342:
	mov	r0, #1
	pop	{r4, r5, r6, r7, r8, pc}
.L2348:
	lsl	ip, ip, #4
	add	r5, r0, #392
	movw	r8, #398
	add	r7, r0, ip
	add	r6, r5, ip
	str	r1, [r7, #400]
	mov	r3, lr
	str	r4, [r7, #404]
	add	lr, r6, #4
	add	r1, r0, #556
	strh	r2, [r7, r8]	@ movhi
	b	.L2338
.L2340:
	mov	r1, r3
	ldr	r3, [r3, #8]
.L2338:
	cmp	r3, #0
	bne	.L2340
	add	ip, r0, ip
	str	lr, [r1, #8]
	mov	r0, r3
	str	r3, [ip, #404]
	str	r1, [ip, #408]
	ldrh	r3, [r5]
	add	r3, r3, #1
	strh	r3, [r5]	@ movhi
	pop	{r4, r5, r6, r7, r8, pc}
	.size	add_prio_gc, .-add_prio_gc
	.align	2
	.global	first_scan_all_blocks
	.syntax unified
	.arm
	.type	first_scan_all_blocks, %function
first_scan_all_blocks:
	@ args = 0, pretend = 0, frame = 48
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, .L2442
	mov	ip, #0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	mov	r4, r0
	sub	sp, sp, #108
	ldr	r5, [r4, #40]
	add	r7, sp, #72
	add	r2, r3, #8
	ldr	r0, [r3]	@ unaligned
	mov	r1, r7
	ldrb	lr, [r3, #4]	@ zero_extendqisi2
	mov	r3, #47
	strb	ip, [sp, #73]
	str	r0, [sp, #88]
	mov	r0, ip
	strb	lr, [sp, #92]
	strb	r3, [sp, #72]
	bl	nand_fdt_get_prop_string_by_path
	cmp	r0, #0
	beq	.L2350
	mov	r2, #3
	add	r1, sp, #88
	bl	nand_memcmp
	cmp	r0, #0
	beq	.L2430
.L2350:
	mvn	r0, #0
.L2413:
	add	sp, sp, #108
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L2430:
	ldr	r2, [r4, #4]
	ldrh	r3, [r5]
	strh	r0, [r4, #18]	@ movhi
	ldrh	r2, [r2, #24]
	cmp	r2, r3
	bls	.L2384
	movw	r2, #21845
	movw	r1, #:lower16:.LC279
	movw	r10, #:lower16:.LC284
	movt	r2, 21845
	movt	r1, #:upper16:.LC279
	add	r5, r5, #36
	str	r0, [sp, #56]
	movt	r10, #:upper16:.LC284
	str	r2, [sp, #60]
	str	r1, [sp, #64]
	b	.L2383
.L2353:
	ldrh	r1, [r5, #-36]
	mov	r0, r10
	bl	NAND_Print_DBG
	mov	r3, #255
	strh	r6, [r5, #-30]	@ movhi
	strh	r3, [r5, #-34]	@ movhi
	ldrh	r3, [r4, #20]
	add	r3, r3, #1
	strh	r3, [r4, #20]	@ movhi
.L2361:
	ldr	r2, [r4, #4]
	ldrh	r3, [r5], #36
	ldrh	r2, [r2, #24]
	cmp	r2, r3
	bls	.L2384
.L2383:
	mov	r6, #0
	strh	r3, [sp, #90]	@ movhi
	str	r7, [sp, #100]
	add	r1, sp, #88
	strh	r6, [sp, #88]	@ movhi
	mov	r0, r4
	strh	r6, [sp, #92]	@ movhi
	sub	r8, r5, #36
	str	r6, [sp, #96]
	ldr	r3, [r4, #1992]
	blx	r3
	cmp	r0, #1
	bne	.L2353
	ldr	r3, [r4, #1980]
	add	r1, sp, #88
	mov	r0, r4
	blx	r3
	ldrb	r2, [sp, #73]	@ zero_extendqisi2
	mov	r9, r0
	str	r8, [r4, #44]
	cmp	r2, #170
	beq	.L2431
.L2354:
	mov	fp, r7
	add	r6, sp, #82
	mov	r3, r7
	mov	r0, #0
.L2357:
	ldrb	r1, [r3], #1	@ zero_extendqisi2
	cmp	r1, #255
	addeq	r0, r0, #1
	cmp	r6, r3
	bne	.L2357
	cmp	r0, #8
	bls	.L2358
	ldrb	r3, [sp, #72]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L2432
.L2358:
	and	r3, r2, #240
	cmp	r3, #192
	beq	.L2385
	ldrb	r3, [sp, #74]	@ zero_extendqisi2
	mov	r1, #0
	movt	r1, 43537
	orr	r0, r3, r2, lsl #8
	cmp	r1, r0, lsl #16
	beq	.L2385
	cmp	r2, #170
	beq	.L2433
.L2366:
	ldrb	ip, [sp, #75]	@ zero_extendqisi2
	orr	r1, r3, r2, lsl #8
	ldrb	r0, [sp, #76]	@ zero_extendqisi2
	ldr	lr, [sp, #60]
	orr	r1, ip, r1, lsl #8
	orr	r1, r0, r1, lsl #8
	cmp	r1, lr
	beq	.L2434
	ldrb	lr, [sp, #78]	@ zero_extendqisi2
	str	ip, [sp]
	ldrb	ip, [sp, #77]	@ zero_extendqisi2
	str	r0, [sp, #4]
	movw	r0, #:lower16:.LC287
	ldrb	r1, [sp, #72]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC287
	str	lr, [sp, #12]
	str	ip, [sp, #8]
	bl	NAND_Print_DBG
	ldrh	r3, [sp, #56]
	cmp	r9, #10
	cmpne	r9, #0
	moveq	r9, #1
	movne	r9, #0
	add	r2, r3, #1
	strh	r2, [r5, #-30]	@ movhi
	bne	.L2435
	ldrb	r2, [sp, #72]	@ zero_extendqisi2
	cmp	r2, #255
	beq	.L2373
	movw	r0, #:lower16:.LC280
	ldrh	r1, [r5, #-36]
	movt	r0, #:upper16:.LC280
	bl	NAND_Print_DBG
	mov	r3, #255
	strh	r3, [r5, #-34]	@ movhi
	ldrh	r3, [r4, #20]
	add	r3, r3, #1
	strh	r3, [r4, #20]	@ movhi
	b	.L2361
.L2385:
	cmp	r9, #10
	cmpne	r9, #0
	beq	.L2414
	cmp	r2, #170
	ldrb	r3, [sp, #74]	@ zero_extendqisi2
	bne	.L2366
.L2433:
	cmp	r3, #238
	bne	.L2366
	movw	r0, #:lower16:.LC286
	movt	r0, #:upper16:.LC286
	bl	NAND_Print_DBG
	b	.L2361
.L2431:
	ldrb	r3, [sp, #74]	@ zero_extendqisi2
	cmp	r3, #204
	bne	.L2354
	movw	r0, #:lower16:.LC285
	ldrh	r1, [r8]
	movt	r0, #:upper16:.LC285
	bl	NAND_Print
	mov	r0, r6
	b	.L2413
.L2432:
	ldrh	r2, [r4, #18]
	mov	r3, #0
	add	r1, r4, #152
	add	r2, r2, #1
	strh	r2, [r4, #18]	@ movhi
	str	r3, [r5, #-16]
	str	r3, [r5, #-12]
	ldr	r2, [r4, #172]
	strh	r3, [r5, #-34]	@ movhi
	strh	r3, [r5, #-30]	@ movhi
	cmp	r2, r3
	strh	r3, [r5, #-32]	@ movhi
	beq	.L2436
	str	r8, [r2, #24]
	str	r2, [r5, #-16]
	str	r8, [r4, #172]
	str	r1, [r5, #-12]
.L2360:
	ldrh	r3, [r4, #22]
	add	r3, r3, #1
	strh	r3, [r4, #22]	@ movhi
	b	.L2361
.L2434:
	ldrh	r1, [r4, #18]
	mov	r3, #0
	ldr	ip, [r4, #4]
	add	r2, r4, #152
	add	r1, r1, #1
	strh	r1, [r4, #18]	@ movhi
	strh	r3, [r5, #-34]	@ movhi
	ldrb	r0, [sp, #77]	@ zero_extendqisi2
	ldrb	r1, [sp, #78]	@ zero_extendqisi2
	orr	r1, r1, r0, lsl #8
	uxth	r0, r1
	str	r1, [sp, #56]
	strh	r0, [r5, #-30]	@ movhi
	ldrh	r1, [ip, #20]
	str	r3, [r5, #-16]
	str	r3, [r5, #-12]
	strh	r1, [r5, #-32]	@ movhi
	b	.L2370
.L2372:
	ldrh	r1, [r3, #6]
	cmp	r1, r0
	bcs	.L2437
	mov	r2, r3
.L2370:
	ldr	r3, [r2, #20]
	cmp	r3, #0
	bne	.L2372
	str	r8, [r2, #20]
	str	r3, [r5, #-16]
	str	r2, [r5, #-12]
.L2371:
	ldrh	r3, [r4, #22]
	ldr	r0, [sp, #64]
	add	r3, r3, #1
	strh	r3, [r4, #22]	@ movhi
	ldrh	r1, [r5, #-36]
	bl	NAND_Print_DBG
	b	.L2361
.L2384:
	mov	r0, #0
	add	sp, sp, #108
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L2414:
	ldrb	lr, [sp, #79]	@ zero_extendqisi2
	mov	ip, #1
	ldrb	r1, [sp, #80]	@ zero_extendqisi2
	mov	r3, #0
	ldrh	r0, [r4, #18]
	ldrb	r2, [sp, #81]	@ zero_extendqisi2
	orr	r1, r1, lr, lsl #8
	add	r0, r0, ip
	strh	r0, [r4, #18]	@ movhi
	orr	r2, r2, r1, lsl #8
	ldrb	r0, [sp, #82]	@ zero_extendqisi2
	strh	ip, [r5, #-34]	@ movhi
	ldrb	r1, [sp, #78]	@ zero_extendqisi2
	ldrb	ip, [sp, #77]	@ zero_extendqisi2
	orr	r0, r0, r2, lsl #8
	strh	r3, [r5, #-32]	@ movhi
	str	r3, [r5, #-8]
	str	r0, [r5, #-28]
	orr	r2, r1, ip, lsl #8
	str	r3, [r5, #-4]
	mov	r1, r2
	str	r2, [sp, #56]
	strh	r1, [r5, #-30]	@ movhi
	add	r2, r4, #224
	b	.L2363
.L2365:
	ldr	r1, [r3, #8]
	cmp	r0, r1
	ble	.L2438
	mov	r2, r3
.L2363:
	ldr	r3, [r2, #28]
	cmp	r3, #0
	bne	.L2365
	str	r8, [r2, #28]
	str	r3, [r5, #-8]
	str	r2, [r5, #-4]
.L2364:
	cmp	r9, #10
	bne	.L2361
.L2423:
	ldr	r3, [r4, #1972]
	ldrh	r3, [r3, #4]
	cmp	r3, #0
	beq	.L2361
	mov	r1, r8
	mov	r2, #1
	mov	r0, r4
	bl	add_prio_gc
	movw	r0, #:lower16:.LC278
	movt	r0, #:upper16:.LC278
	bl	NAND_Print_DBG
	b	.L2361
.L2437:
	str	r8, [r3, #24]
	str	r3, [r5, #-16]
	str	r8, [r2, #20]
	str	r2, [r5, #-12]
	b	.L2371
.L2438:
	str	r8, [r3, #32]
	str	r3, [r5, #-8]
	str	r8, [r2, #28]
	str	r2, [r5, #-4]
	b	.L2364
.L2435:
	movw	r0, #:lower16:.LC281
	ldrh	r1, [r5, #-36]
	movt	r0, #:upper16:.LC281
	bl	NAND_Print
	ldr	r3, [r4, #4]
	add	r1, sp, #88
	ldrh	r2, [r5, #-36]
	mov	r0, r4
	ldrh	r3, [r3, #20]
	strh	r2, [sp, #90]	@ movhi
	strh	r9, [sp, #92]	@ movhi
	sub	r3, r3, #1
	str	r9, [sp, #96]
	strh	r3, [sp, #88]	@ movhi
	str	r7, [sp, #100]
	ldr	r3, [r4, #1980]
	blx	r3
	cmp	r0, #10
	cmpne	r0, #0
	str	r0, [sp, #68]
	bne	.L2374
	ldrb	ip, [sp, #73]	@ zero_extendqisi2
	movw	r0, #65535
	ldrb	r1, [sp, #74]	@ zero_extendqisi2
	movt	r0, 43690
	ldrb	r2, [sp, #75]	@ zero_extendqisi2
	ldrb	r3, [sp, #76]	@ zero_extendqisi2
	orr	r1, r1, ip, lsl #8
	orr	r1, r2, r1, lsl #8
	orr	r3, r3, r1, lsl #8
	cmp	r3, r0
	beq	.L2439
.L2376:
	ldrb	r3, [fp], #1	@ zero_extendqisi2
	cmp	r3, #255
	addeq	r9, r9, #1
	cmp	r6, fp
	bne	.L2376
	cmp	r9, #8
	bls	.L2381
	ldrb	r3, [sp, #72]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L2440
.L2381:
	ldrh	r1, [r5, #-36]
	movw	r0, #:lower16:.LC290
	movt	r0, #:upper16:.LC290
.L2424:
	bl	NAND_Print
	ldrh	r2, [r5, #-30]
	mov	r1, r8
	mov	r0, r4
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	cmp	r0, #0
	bne	.L2361
.L2420:
	ldrh	r3, [r4, #18]
	add	r3, r3, #1
	strh	r3, [r4, #18]	@ movhi
	b	.L2361
.L2374:
	movw	r0, #:lower16:.LC282
	ldrh	r1, [r5, #-36]
	movt	r0, #:upper16:.LC282
	bl	NAND_Print
	ldrb	lr, [sp, #76]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC283
	ldrb	ip, [sp, #75]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC283
	ldrb	r6, [sp, #77]	@ zero_extendqisi2
	ldrb	r3, [sp, #74]	@ zero_extendqisi2
	stm	sp, {ip, lr}
	ldrb	ip, [sp, #86]	@ zero_extendqisi2
	ldrb	lr, [sp, #85]	@ zero_extendqisi2
	str	r6, [sp, #8]
	str	ip, [sp, #44]
	ldrb	ip, [sp, #84]	@ zero_extendqisi2
	str	lr, [sp, #40]
	ldrb	lr, [sp, #83]	@ zero_extendqisi2
	str	ip, [sp, #36]
	ldrb	ip, [sp, #82]	@ zero_extendqisi2
	str	lr, [sp, #32]
	ldrb	lr, [sp, #81]	@ zero_extendqisi2
	str	ip, [sp, #28]
	ldrb	ip, [sp, #80]	@ zero_extendqisi2
	ldrb	r6, [sp, #87]	@ zero_extendqisi2
	str	lr, [sp, #24]
	str	ip, [sp, #20]
	ldrb	lr, [sp, #79]	@ zero_extendqisi2
	ldrb	ip, [sp, #78]	@ zero_extendqisi2
	ldrb	r2, [sp, #73]	@ zero_extendqisi2
	ldrb	r1, [sp, #72]	@ zero_extendqisi2
	str	lr, [sp, #16]
	str	ip, [sp, #12]
	str	r6, [sp, #48]
	bl	NAND_Print
	mov	r1, r8
	mov	r0, r4
	bl	torture_block_and_mark
	cmp	r0, #0
	bne	.L2361
	b	.L2420
.L2373:
	add	r2, r3, #2
	mov	r1, r8
	uxth	r2, r2
	mov	r0, r4
	bl	erase_block
	cmp	r0, #0
	bne	.L2361
	b	.L2420
.L2436:
	str	r8, [r4, #172]
	str	r2, [r5, #-16]
	str	r1, [r5, #-12]
	b	.L2360
.L2439:
	movw	r0, #:lower16:.LC288
	ldrh	r1, [r5, #-36]
	movt	r0, #:upper16:.LC288
	bl	NAND_Print
	ldrh	r0, [r4, #18]
	mov	ip, #1
	ldrb	lr, [sp, #79]	@ zero_extendqisi2
	add	r3, r4, #224
	ldrb	r1, [sp, #80]	@ zero_extendqisi2
	ldrb	r2, [sp, #81]	@ zero_extendqisi2
	add	r0, r0, ip
	strh	r0, [r4, #18]	@ movhi
	orr	r1, r1, lr, lsl #8
	ldrb	lr, [sp, #82]	@ zero_extendqisi2
	strh	ip, [r5, #-34]	@ movhi
	ldrb	ip, [sp, #77]	@ zero_extendqisi2
	orr	r2, r2, r1, lsl #8
	ldrb	r0, [sp, #78]	@ zero_extendqisi2
	orr	r1, lr, r2, lsl #8
	strh	r9, [r5, #-32]	@ movhi
	str	r9, [r5, #-8]
	str	r1, [r5, #-28]
	orr	r2, r0, ip, lsl #8
	str	r9, [r5, #-4]
	str	r2, [sp, #56]
	strh	r2, [r5, #-30]	@ movhi
	b	.L2377
.L2379:
	ldr	r0, [r2, #8]
	cmp	r1, r0
	ble	.L2441
	mov	r3, r2
.L2377:
	ldr	r2, [r3, #28]
	cmp	r2, #0
	bne	.L2379
	str	r8, [r3, #28]
	stmdb	r5, {r2, r3}
.L2378:
	ldr	r3, [sp, #68]
	cmp	r3, #10
	bne	.L2361
	b	.L2423
.L2440:
	movw	r0, #:lower16:.LC289
	ldrh	r1, [r5, #-36]
	movt	r0, #:upper16:.LC289
	b	.L2424
.L2441:
	str	r8, [r2, #32]
	str	r2, [r5, #-8]
	str	r8, [r3, #28]
	str	r3, [r5, #-4]
	b	.L2378
.L2443:
	.align	2
.L2442:
	.word	.LANCHOR2+808
	.size	first_scan_all_blocks, .-first_scan_all_blocks
	.align	2
	.global	check_static_wear_leveling
	.syntax unified
	.arm
	.type	check_static_wear_leveling, %function
check_static_wear_leveling:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #580]
	cmp	r3, #0
	beq	.L2446
	mov	r0, #1
	bx	lr
.L2446:
	b	check_static_wear_leveling.part.9
	.size	check_static_wear_leveling, .-check_static_wear_leveling
	.align	2
	.global	prio_gc_one
	.syntax unified
	.arm
	.type	prio_gc_one, %function
prio_gc_one:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #4]
	add	r1, r1, r1, lsl #3
	cmp	r2, #1
	push	{r4, lr}
	ldr	r3, [r3, #32]
	add	r1, r3, r1, lsl #2
	beq	.L2448
	cmp	r2, #2
	movne	r2, #3
	moveq	r2, #2
.L2448:
	bl	add_prio_gc
	mov	r0, #0
	pop	{r4, pc}
	.size	prio_gc_one, .-prio_gc_one
	.align	2
	.global	get_total_invalid_page
	.syntax unified
	.arm
	.type	get_total_invalid_page, %function
get_total_invalid_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #200]
	cmp	r3, #0
	beq	.L2455
	mov	r0, #0
.L2454:
	ldrh	r2, [r3, #4]
	ldr	r3, [r3, #12]
	add	r0, r0, r2
	cmp	r3, #0
	bne	.L2454
	bx	lr
.L2455:
	mov	r0, r3
	bx	lr
	.size	get_total_invalid_page, .-get_total_invalid_page
	.align	2
	.global	is_current_blcok
	.syntax unified
	.arm
	.type	is_current_blcok, %function
is_current_blcok:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldrb	r3, [r0, #17]	@ zero_extendqisi2
	tst	r3, #1
	beq	.L2458
	ldr	r3, [r0, #324]
	cmp	r1, r3
	beq	.L2460
.L2458:
	ldr	r0, [r0, #292]
	sub	r0, r0, r1
	clz	r0, r0
	lsr	r0, r0, #5
	bx	lr
.L2460:
	mov	r0, #1
	bx	lr
	.size	is_current_blcok, .-is_current_blcok
	.align	2
	.global	static_wear_leveling
	.syntax unified
	.arm
	.type	static_wear_leveling, %function
static_wear_leveling:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	add	r3, r0, #380
	ldrh	r2, [r0, #22]
	ldrh	r3, [r3]
	push	{r4, lr}
	cmp	r2, r3
	bls	.L2473
	ldr	r3, [r0, #368]
	cmp	r3, #19
	bls	.L2473
	ldr	r2, [r0, #1968]
	ldr	r1, [r0, #200]
	ldr	r3, [r2, #184]
	cmp	r1, #0
	ldrne	ip, [r1, #8]
	add	r3, r3, #1
	str	r3, [r2, #184]
	movne	r3, r1
	bne	.L2471
	b	.L2466
.L2468:
	ldr	r2, [r3, #8]
	cmp	r2, ip
	movls	ip, r2
	movls	r1, r3
.L2471:
	ldr	r3, [r3, #12]
	cmp	r3, #0
	bne	.L2468
.L2466:
	mov	r4, r0
	mov	r2, #5
	bl	add_prio_gc
	ldr	r2, [r4, #4]
	cmp	r0, #0
	ldr	r3, [r4, #372]
	ldrh	r2, [r2, #20]
	addeq	r3, r3, #4
	streq	r3, [r4, #372]
	cmp	r2, r3
	movls	r3, #0
	strls	r3, [r4, #368]
	strls	r3, [r4, #372]
.L2473:
	mov	r0, #0
	pop	{r4, pc}
	.size	static_wear_leveling, .-static_wear_leveling
	.align	2
	.global	nftl_cache_init
	.syntax unified
	.arm
	.type	nftl_cache_init, %function
nftl_cache_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #1972]
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	mov	r6, #0
	add	r8, r0, #584
	mov	r7, r0
	ldrh	r3, [r3, #22]
	strh	r6, [r8, #2]	@ movhi
	strh	r6, [r8, #4]	@ movhi
	cmp	r3, r6
	strh	r3, [r8]	@ movhi
	strh	r6, [r8, #6]	@ movhi
	addne	r4, r0, #592
	str	r6, [r0, #1568]
	movne	r5, r6
	str	r6, [r0, #1572]
	movne	r10, #255
	str	r6, [r0, #1576]
	mvnne	r9, #0
	str	r6, [r0, #1580]
	str	r6, [r0, #1600]
	str	r6, [r0, #1604]
	str	r6, [r0, #1608]
	str	r6, [r0, #1612]
	bne	.L2480
	b	.L2481
.L2479:
	ldrh	r3, [r8]
	cmp	r3, r6
	bls	.L2481
.L2480:
	str	r5, [r4, #16]
	add	r4, r4, #32
	str	r5, [r4, #-12]
	str	r5, [r4, #-8]
	str	r5, [r4, #-4]
	ldr	r3, [r7, #4]
	strh	r6, [r4, #-32]	@ movhi
	add	r6, r6, #1
	strh	r10, [r4, #-30]	@ movhi
	strh	r5, [r4, #-28]	@ movhi
	strh	r5, [r4, #-26]	@ movhi
	str	r9, [r4, #-24]
	ldrh	r0, [r3, #26]
	bl	nftl_malloc
	cmp	r0, #0
	str	r0, [r4, #-20]
	bne	.L2479
.L2477:
	movw	r0, #:lower16:.LC221
	movt	r0, #:upper16:.LC221
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L2481:
	ldr	r3, [r7, #4]
	ldrh	r0, [r3, #26]
	bl	nftl_malloc
	cmp	r0, #0
	str	r0, [r7, #1616]
	beq	.L2477
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
	.size	nftl_cache_init, .-nftl_cache_init
	.align	2
	.global	nftl_cache_exit
	.syntax unified
	.arm
	.type	nftl_cache_exit, %function
nftl_cache_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	add	r6, r0, #584
	ldrh	r3, [r6]
	mov	r7, r0
	cmp	r3, #0
	beq	.L2490
	mov	r5, r0
	mov	r4, #0
.L2489:
	ldr	r0, [r5, #604]
	mov	r1, #0
	bl	NAND_Free
	ldrh	r3, [r6]
	add	r4, r4, #1
	add	r5, r5, #32
	cmp	r3, r4
	bgt	.L2489
.L2490:
	ldr	r0, [r7, #1616]
	mov	r1, #0
	bl	NAND_Free
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, pc}
	.size	nftl_cache_exit, .-nftl_cache_exit
	.align	2
	.global	nftl_zone_release
	.syntax unified
	.arm
	.type	nftl_zone_release, %function
nftl_zone_release:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r4, r0
	bl	nftl_cache_exit
	mov	r0, r4
	bl	zone_param_exit
	ldr	r3, [r4, #4]
	mov	r1, #0
	ldr	r0, [r3, #32]
	bl	NAND_Free
	ldr	r0, [r4, #4]
	mov	r1, #0
	bl	NAND_Free
	mov	r0, r4
	mov	r1, #0
	pop	{r4, lr}
	b	NAND_Free
	.size	nftl_zone_release, .-nftl_zone_release
	.align	2
	.global	nftl_exit
	.syntax unified
	.arm
	.type	nftl_exit, %function
nftl_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r4, r0
	ldr	r0, [r0, #4]
	bl	free_nand_partition
	ldr	r0, [r4, #12]
	bl	nftl_zone_release
	ldr	r0, [r4, #16]
	mov	r1, #0
	bl	NAND_Free
	mov	r0, #0
	pop	{r4, pc}
	.size	nftl_exit, .-nftl_exit
	.align	2
	.global	nand_ftl_exit
	.syntax unified
	.arm
	.type	nand_ftl_exit, %function
nand_ftl_exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	movw	r5, #:lower16:.LANCHOR0
	movt	r5, #:upper16:.LANCHOR0
	add	r2, r5, #4
	ldr	r4, [r5, #12]
	b	.L2499
.L2501:
	ldr	r3, [r4, #8]
	cmp	r3, #0
	beq	.L2509
	mov	r2, r4
	mov	r4, r3
.L2499:
	cmp	r4, #0
	bne	.L2501
.L2507:
	mov	r0, #0
	pop	{r4, r5, r6, pc}
.L2509:
	ldr	r6, .L2511
	str	r3, [r2, #8]
.L2500:
	mov	r0, r4
	bl	nftl_exit
	mov	r0, r4
	mov	r1, #0
	bl	NAND_Free
	ldr	r4, [r5, #12]
	mov	r2, r6
	b	.L2502
.L2503:
	ldr	r3, [r4, #8]
	cmp	r3, #0
	beq	.L2510
	mov	r2, r4
	mov	r4, r3
.L2502:
	cmp	r4, #0
	bne	.L2503
	b	.L2507
.L2510:
	str	r3, [r2, #8]
	b	.L2500
.L2512:
	.align	2
.L2511:
	.word	.LANCHOR0+4
	.size	nand_ftl_exit, .-nand_ftl_exit
	.align	2
	.global	nftl_cache_clean
	.syntax unified
	.arm
	.type	nftl_cache_clean, %function
nftl_cache_clean:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r1, [r0, #1972]
	mov	r2, #0
	add	r3, r0, #584
	ldrh	r1, [r1, #22]
	strh	r2, [r3, #2]	@ movhi
	strh	r2, [r3, #4]	@ movhi
	cmp	r1, r2
	strh	r1, [r3]	@ movhi
	strh	r2, [r3, #6]	@ movhi
	str	r2, [r0, #1568]
	str	r2, [r0, #1572]
	str	r2, [r0, #1576]
	str	r2, [r0, #1580]
	str	r2, [r0, #1600]
	str	r2, [r0, #1604]
	str	r2, [r0, #1608]
	str	r2, [r0, #1612]
	beq	.L2520
	str	lr, [sp, #-4]!
	add	r0, r0, #592
	mov	r3, r2
	mov	lr, #255
	mvn	ip, #0
.L2515:
	strh	r2, [r0]	@ movhi
	add	r2, r2, #1
	cmp	r2, r1
	strh	lr, [r0, #2]	@ movhi
	str	ip, [r0, #8]
	add	r0, r0, #32
	str	r3, [r0, #-16]
	str	r3, [r0, #-12]
	str	r3, [r0, #-8]
	str	r3, [r0, #-4]
	strh	r3, [r0, #-28]	@ movhi
	strh	r3, [r0, #-26]	@ movhi
	bne	.L2515
	mov	r0, #0
	ldr	pc, [sp], #4
.L2520:
	mov	r0, #0
	bx	lr
	.size	nftl_cache_clean, .-nftl_cache_clean
	.align	2
	.global	search_from_cache_read_list
	.syntax unified
	.arm
	.type	search_from_cache_read_list, %function
search_from_cache_read_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #984]
	cmp	r0, #0
	bxeq	lr
	ldr	r3, [r0, #8]
	cmp	r1, r3
	bne	.L2524
	b	.L2530
.L2525:
	ldr	r3, [r0, #8]
	cmp	r3, r1
	bxeq	lr
.L2524:
	ldr	r0, [r0, #16]
	cmp	r0, #0
	bne	.L2525
	bx	lr
.L2530:
	bx	lr
	.size	search_from_cache_read_list, .-search_from_cache_read_list
	.align	2
	.global	search_from_cache_write_list
	.syntax unified
	.arm
	.type	search_from_cache_write_list, %function
search_from_cache_write_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #1024]
	cmp	r0, #0
	bxeq	lr
	ldr	r3, [r0, #8]
	cmp	r1, r3
	bne	.L2534
	b	.L2540
.L2535:
	ldr	r3, [r0, #8]
	cmp	r3, r1
	bxeq	lr
.L2534:
	ldr	r0, [r0, #24]
	cmp	r0, #0
	bne	.L2535
	bx	lr
.L2540:
	bx	lr
	.size	search_from_cache_write_list, .-search_from_cache_write_list
	.align	2
	.global	add_to_cache_read_list_tail
	.syntax unified
	.arm
	.type	add_to_cache_read_list_tail, %function
add_to_cache_read_list_tail:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	add	ip, r0, #968
	b	.L2542
.L2545:
	mov	ip, r3
.L2542:
	ldr	r3, [ip, #16]
	cmp	r3, #0
	bne	.L2545
	cmp	r2, #0
	str	r1, [ip, #16]
	str	r3, [r1, #16]
	ldrheq	r3, [r0, #2]
	ldrhne	r3, [r0, #6]
	str	ip, [r1, #20]
	addeq	r3, r3, #1
	strh	r2, [r1, #2]	@ movhi
	addne	r3, r3, #1
	strheq	r3, [r0, #2]	@ movhi
	strhne	r3, [r0, #6]	@ movhi
	mov	r0, #0
	bx	lr
	.size	add_to_cache_read_list_tail, .-add_to_cache_read_list_tail
	.align	2
	.global	add_to_cache_write_list_tail
	.syntax unified
	.arm
	.type	add_to_cache_write_list_tail, %function
add_to_cache_write_list_tail:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	mov	ip, r0
	add	r2, r0, #1000
	b	.L2547
.L2548:
	mov	r2, r3
.L2547:
	ldr	r3, [r2, #24]
	cmp	r3, #0
	bne	.L2548
	push	{r4, lr}
	mov	r4, #1
	ldrh	lr, [ip, #4]
	mov	r0, r3
	str	r1, [r2, #24]
	str	r3, [r1, #24]
	add	r3, lr, r4
	str	r2, [r1, #28]
	strh	r4, [r1, #2]	@ movhi
	strh	r3, [ip, #4]	@ movhi
	pop	{r4, pc}
	.size	add_to_cache_write_list_tail, .-add_to_cache_write_list_tail
	.align	2
	.global	del_from_cache_read_list
	.syntax unified
	.arm
	.type	del_from_cache_read_list, %function
del_from_cache_read_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r2, [r1, #20]
	ldr	r3, [r1, #16]
	str	r3, [r2, #16]
	ldr	r3, [r1, #16]
	cmp	r3, #0
	strne	r2, [r3, #20]
	mov	r2, #255
	ldrh	r3, [r1, #2]
	cmp	r3, #0
	ldrheq	r3, [r0, #2]
	ldrhne	r3, [r0, #6]
	subeq	r3, r3, #1
	subne	r3, r3, #1
	strheq	r3, [r0, #2]	@ movhi
	strhne	r3, [r0, #6]	@ movhi
	mov	r3, #0
	strh	r2, [r1, #2]	@ movhi
	mov	r0, r1
	str	r3, [r1, #16]
	str	r3, [r1, #20]
	bx	lr
	.size	del_from_cache_read_list, .-del_from_cache_read_list
	.align	2
	.global	del_from_cache_write_list
	.syntax unified
	.arm
	.type	del_from_cache_write_list, %function
del_from_cache_write_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r1, #24]
	mov	r2, r0
	ldr	r0, [r1, #28]
	mov	ip, #255
	str	r3, [r0, #24]
	ldr	r3, [r1, #24]
	cmp	r3, #0
	strne	r0, [r3, #28]
	mov	r0, #0
	ldrh	r3, [r2, #4]
	str	r0, [r1, #24]
	str	r0, [r1, #28]
	mov	r0, r1
	sub	r3, r3, #1
	strh	ip, [r1, #2]	@ movhi
	strh	r3, [r2, #4]	@ movhi
	bx	lr
	.size	del_from_cache_write_list, .-del_from_cache_write_list
	.align	2
	.global	get_empty_cahce_node
	.syntax unified
	.arm
	.type	get_empty_cahce_node, %function
get_empty_cahce_node:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldrh	ip, [r0]
	cmp	ip, #0
	beq	.L2577
	ldrh	r3, [r0, #10]
	str	lr, [sp, #-4]!
	cmp	r3, #255
	beq	.L2579
	sub	ip, ip, #1
	mov	r3, #1
	uxth	ip, ip
	mov	r2, r0
	add	ip, ip, r3
	b	.L2566
.L2568:
	ldrh	r1, [r2, #42]
	add	r2, r2, #32
	cmp	r1, #255
	beq	.L2564
.L2566:
	cmp	r3, ip
	mov	lr, r3
	add	r3, r3, #1
	bne	.L2568
	ldr	lr, [sp], #4
	b	get_empty_cahce_node.part.11
.L2579:
	mov	lr, #0
.L2564:
	add	r0, r0, lr, lsl #5
	add	r0, r0, #8
	ldr	pc, [sp], #4
.L2577:
	b	get_empty_cahce_node.part.11
	.size	get_empty_cahce_node, .-get_empty_cahce_node
	.global	__aeabi_uidivmod
	.align	2
	.global	nand_op
	.syntax unified
	.arm
	.type	nand_op, %function
nand_op:
	@ args = 4, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	cmp	r3, #0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	sub	sp, sp, #4
	ldr	r5, [sp, #40]
	beq	.L2587
	mov	r4, r3
	ldr	r3, [r1, #24]
	mov	r10, r2
	add	r2, r4, r2
	mov	r7, r1
	mov	fp, r0
	cmp	r2, r3
	bhi	.L2593
	ldr	r1, [r1, #4]
	movw	r2, #:lower16:nand_cache_read
	movw	r3, #:lower16:nand_cache_write
	cmp	fp, #0
	movt	r2, #:upper16:nand_cache_read
	movt	r3, #:upper16:nand_cache_write
	ldrb	r6, [r1, #8]	@ zero_extendqisi2
	mov	r0, r10
	moveq	fp, r2
	movne	fp, r3
	mov	r1, r6
	bl	__aeabi_uidiv
	mov	r1, r6
	mov	r8, r0
	mov	r0, r10
	bl	__aeabi_uidivmod
	sub	r6, r6, r1
	uxth	r6, r6
	cmp	r4, r6
	bhi	.L2584
	orr	r2, r4, r1, lsl #8
	mov	r3, r5
	mov	r1, r8
	uxth	r2, r2
	mov	r0, r7
	mov	ip, fp
	add	sp, sp, #4
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	bx	ip
.L2587:
	mov	r0, r3
.L2581:
	add	sp, sp, #4
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L2584:
	orr	r2, r6, r1, lsl #8
	sub	r9, r4, r6
	mov	r3, r5
	uxth	r2, r2
	mov	r1, r8
	mov	r0, r7
	blx	fp
	cmp	r9, #0
	mov	r4, r0
	beq	.L2581
	ldr	r3, [r7, #4]
	ldrb	ip, [r3, #8]	@ zero_extendqisi2
	cmp	r9, ip
	bls	.L2590
.L2586:
	uxth	r10, ip
	add	r5, r5, r6, lsl #9
	add	r8, r8, #1
	mov	r3, r5
	mov	r2, r10
	mov	r1, r8
	mov	r0, r7
	sub	r9, r9, ip
	blx	fp
	ldr	r3, [r7, #4]
	orr	r4, r4, r0
	mov	r6, r10
	ldrb	ip, [r3, #8]	@ zero_extendqisi2
	cmp	ip, r9
	bcc	.L2586
.L2585:
	add	r3, r5, r10, lsl #9
	uxth	r2, r9
	add	r1, r8, #1
	mov	r0, r7
	blx	fp
	orr	r0, r0, r4
	add	sp, sp, #4
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L2593:
	movw	r0, #:lower16:.LC291
	movt	r0, #:upper16:.LC291
	bl	NAND_Print
	mov	r0, #1
	add	sp, sp, #4
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L2590:
	mov	r10, r6
	b	.L2585
	.size	nand_op, .-nand_op
	.align	2
	.global	__nand_read
	.syntax unified
	.arm
	.type	__nand_read, %function
__nand_read:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, lr}
	mov	r7, r3
	ldr	r3, [r0, #356]
	sub	sp, sp, #12
	mov	r4, r0
	mov	r5, r1
	mov	r6, r2
	cmp	r3, #3
	beq	.L2597
.L2595:
	ldr	ip, [r4, #1968]
	mov	r2, r5
	mov	r1, r4
	mov	r3, r6
	mov	r0, #0
	ldrd	r4, [ip, #72]
	adds	r4, r4, r6
	adc	r5, r5, #0
	strd	r4, [ip, #72]
	str	r7, [sp]
	bl	nand_op
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, pc}
.L2597:
	movw	r0, #:lower16:.LC292
	movt	r0, #:upper16:.LC292
	bl	NAND_Print
	b	.L2595
	.size	__nand_read, .-__nand_read
	.align	2
	.global	_nand_read
	.syntax unified
	.arm
	.type	_nand_read, %function
_nand_read:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #12]
	b	__nand_read
	.size	_nand_read, .-_nand_read
	.align	2
	.global	nand_sector_cache_read
	.syntax unified
	.arm
	.type	nand_sector_cache_read, %function
nand_sector_cache_read:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	ip, [r0, #1960]
	push	{r4, lr}
	cmp	ip, #0
	beq	.L2600
	ldr	r3, [ip, #4]
	cmp	r1, r3
	bne	.L2603
	b	.L2601
.L2604:
	ldr	lr, [ip, #4]
	cmp	r1, lr
	beq	.L2601
.L2603:
	ldr	ip, [ip, #12]
	cmp	ip, #0
	bne	.L2604
.L2600:
	mov	r3, r2
	mov	r2, #1
	bl	__nand_read
	cmp	r0, #0
	bne	.L2606
	pop	{r4, pc}
.L2601:
	mov	r0, r2
	ldr	r1, [ip, #8]
	mov	r2, #512
	bl	NAND_Memcpy
	mov	r0, #0
	pop	{r4, pc}
.L2606:
	movw	r0, #:lower16:.LC293
	movt	r0, #:upper16:.LC293
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, pc}
	.size	nand_sector_cache_read, .-nand_sector_cache_read
	.align	2
	.global	__nand_sector_read
	.syntax unified
	.arm
	.type	__nand_sector_read, %function
__nand_sector_read:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	cmp	r2, #0
	push	{r4, r5, r6, r7, r8, lr}
	beq	.L2615
	mov	r8, r0
	mov	r4, r1
	mov	r5, r3
	add	r7, r1, r2
	mov	r6, #0
.L2614:
	mov	r2, r5
	mov	r1, r4
	mov	r0, r8
	add	r4, r4, #1
	bl	nand_sector_cache_read
	cmp	r7, r4
	add	r5, r5, #512
	orr	r6, r6, r0
	bne	.L2614
.L2613:
	mov	r0, r6
	pop	{r4, r5, r6, r7, r8, pc}
.L2615:
	mov	r6, r2
	b	.L2613
	.size	__nand_sector_read, .-__nand_sector_read
	.align	2
	.global	__nand_write
	.syntax unified
	.arm
	.type	__nand_write, %function
__nand_write:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, lr}
	mov	r7, r3
	ldr	r3, [r0, #356]
	sub	sp, sp, #12
	mov	r4, r0
	mov	r5, r1
	mov	r6, r2
	cmp	r3, #3
	beq	.L2621
.L2619:
	ldr	ip, [r4, #1968]
	mov	r2, r5
	mov	r1, r4
	mov	r3, r6
	mov	r0, #1
	ldrd	r4, [ip, #80]
	adds	r4, r4, r6
	adc	r5, r5, #0
	strd	r4, [ip, #80]
	str	r7, [sp]
	bl	nand_op
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, pc}
.L2621:
	movw	r0, #:lower16:.LC294
	movt	r0, #:upper16:.LC294
	bl	NAND_Print
	b	.L2619
	.size	__nand_write, .-__nand_write
	.align	2
	.global	_nand_write
	.syntax unified
	.arm
	.type	_nand_write, %function
_nand_write:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #12]
	b	__nand_write
	.size	_nand_write, .-_nand_write
	.align	2
	.global	write_data_to_nand
	.syntax unified
	.arm
	.type	write_data_to_nand, %function
write_data_to_nand:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r6, r0
	mov	r0, #512
	bl	NAND_Malloc
	subs	r5, r0, #0
	beq	.L2628
.L2624:
	mov	r2, #512
	mov	r1, #170
	mov	r0, r5
	mov	r4, #0
	bl	NAND_Memset
.L2625:
	mov	r2, #1
	mov	r1, r4
	mov	r3, r5
	add	r4, r4, r2
	mov	r0, r6
	bl	__nand_write
	cmp	r4, #38912
	bne	.L2625
	mov	r0, r5
	mov	r1, #0
	bl	NAND_Free
	pop	{r4, r5, r6, pc}
.L2628:
	movw	r0, #:lower16:.LC295
	ldr	r1, .L2629
	movt	r0, #:upper16:.LC295
	bl	NAND_Print
	b	.L2624
.L2630:
	.align	2
.L2629:
	.word	.LANCHOR2+824
	.size	write_data_to_nand, .-write_data_to_nand
	.align	2
	.global	nand_sector_cache_write
	.syntax unified
	.arm
	.type	nand_sector_cache_write, %function
nand_sector_cache_write:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r5, r1
	ldr	r7, [r0, #1960]
	mov	r8, r2
	cmp	r7, #0
	beq	.L2632
	ldr	r3, [r7, #4]
	mov	r4, r7
	cmp	r1, r3
	bne	.L2635
	b	.L2633
.L2636:
	ldr	r3, [r4, #4]
	cmp	r5, r3
	beq	.L2633
.L2635:
	ldr	r4, [r4, #12]
	cmp	r4, #0
	bne	.L2636
.L2632:
	add	r6, r0, #1616
	add	r6, r6, #4
	ldrh	r3, [r6, #2]
	cmp	r3, #7
	bls	.L2665
	ldr	r1, [r7, #16]
	mov	ip, #255
	ldr	r2, [r7, #12]
	str	r2, [r1, #12]
	ldr	r2, [r7, #12]
	cmp	r2, #0
	strne	r1, [r2, #16]
	sub	r1, r3, #1
	mov	r3, #0
	strh	ip, [r7, #2]	@ movhi
	str	r3, [r7, #12]
	mov	r2, #1
	str	r3, [r7, #16]
	ldr	r3, [r7, #8]
	strh	r1, [r6, #2]	@ movhi
	ldr	r1, [r7, #4]
	bl	__nand_write
	cmp	r0, #0
	bne	.L2666
.L2640:
	mov	r2, #512
	str	r5, [r7, #4]
	mov	r1, r8
	ldr	r0, [r7, #8]
	bl	NAND_Memcpy
	add	r2, r6, #328
	b	.L2644
.L2650:
	mov	r2, r3
.L2644:
	ldr	r3, [r2, #12]
	cmp	r3, #0
	bne	.L2650
	ldrh	r1, [r6, #2]
	mov	ip, #1
	str	r7, [r2, #12]
	mov	r0, r3
	str	r3, [r7, #12]
	add	r3, r1, ip
	str	r2, [r7, #16]
	strh	ip, [r7, #2]	@ movhi
	strh	r3, [r6, #2]	@ movhi
	pop	{r4, r5, r6, r7, r8, pc}
.L2633:
	ldr	r2, [r4, #16]
	add	r6, r0, #1616
	ldr	r3, [r4, #12]
	add	r6, r6, #4
	mov	r0, #0
	mov	ip, #255
	mov	r1, r8
	str	r3, [r2, #12]
	ldr	r3, [r4, #12]
	cmp	r3, #0
	strne	r2, [r3, #16]
	mov	r2, #512
	ldrh	r3, [r6, #2]
	str	r0, [r4, #12]
	str	r0, [r4, #16]
	sub	r3, r3, #1
	strh	ip, [r4, #2]	@ movhi
	ldr	r0, [r4, #8]
	strh	r3, [r6, #2]	@ movhi
	bl	NAND_Memcpy
	add	r2, r6, #328
	b	.L2637
.L2649:
	mov	r2, r3
.L2637:
	ldr	r3, [r2, #12]
	cmp	r3, #0
	bne	.L2649
	ldrh	r1, [r6, #2]
	mov	ip, #1
	str	r4, [r2, #12]
	mov	r0, r3
	str	r3, [r4, #12]
	add	r3, r1, ip
	str	r2, [r4, #16]
	strh	ip, [r4, #2]	@ movhi
	strh	r3, [r6, #2]	@ movhi
	pop	{r4, r5, r6, r7, r8, pc}
.L2665:
	add	r0, r0, #1616
	mov	r3, #0
	add	r0, r0, #14
.L2642:
	ldrh	r2, [r0]
	add	r0, r0, #20
	cmp	r2, #255
	beq	.L2667
	add	r3, r3, #1
	cmp	r3, #16
	bne	.L2642
	movw	r0, #:lower16:.LC297
	movt	r0, #:upper16:.LC297
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, r7, r8, pc}
.L2667:
	add	r3, r3, r3, lsl #2
	add	r7, r6, r3, lsl #2
	add	r7, r7, #8
	b	.L2640
.L2666:
	movw	r0, #:lower16:.LC296
	movt	r0, #:upper16:.LC296
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, r7, r8, pc}
	.size	nand_sector_cache_write, .-nand_sector_cache_write
	.align	2
	.global	__nand_sector_write
	.syntax unified
	.arm
	.type	__nand_sector_write, %function
__nand_sector_write:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	cmp	r2, #0
	push	{r4, r5, r6, r7, r8, lr}
	beq	.L2671
	mov	r8, r0
	mov	r4, r1
	mov	r5, r3
	add	r7, r1, r2
	mov	r6, #0
.L2670:
	mov	r2, r5
	mov	r1, r4
	mov	r0, r8
	add	r4, r4, #1
	bl	nand_sector_cache_write
	cmp	r7, r4
	add	r5, r5, #512
	orr	r6, r6, r0
	bne	.L2670
.L2669:
	mov	r0, r6
	pop	{r4, r5, r6, r7, r8, pc}
.L2671:
	mov	r6, r2
	b	.L2669
	.size	__nand_sector_write, .-__nand_sector_write
	.align	2
	.global	__nand_flush_sector_write_cache
	.syntax unified
	.arm
	.type	__nand_flush_sector_write_cache, %function
__nand_flush_sector_write_cache:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r8, #0
	add	r6, r0, #1616
	mov	r5, r0
	mov	r4, r1
	add	r6, r6, #4
	mov	r7, r8
	b	.L2675
.L2678:
	ldr	ip, [r1, #16]
	ldr	r3, [r1, #12]
	str	r3, [ip, #12]
	ldr	r3, [r1, #12]
	cmp	r3, #0
	strne	ip, [r3, #16]
	ldrh	ip, [r6, #2]
	ldr	r3, [r1, #8]
	str	r7, [r1, #12]
	sub	ip, ip, #1
	str	r7, [r1, #16]
	strh	lr, [r1, #2]	@ movhi
	strh	ip, [r6, #2]	@ movhi
	ldr	r1, [r1, #4]
	bl	__nand_write
	subs	r4, r4, #1
	mov	r8, r0
	beq	.L2677
.L2675:
	ldr	r1, [r5, #1960]
	mov	lr, #255
	mov	r2, #1
	mov	r0, r5
	cmp	r1, #0
	bne	.L2678
.L2677:
	cmp	r8, #0
	beq	.L2679
	movw	r0, #:lower16:.LC298
	movt	r0, #:upper16:.LC298
	bl	NAND_Print
.L2679:
	mov	r0, r8
	pop	{r4, r5, r6, r7, r8, pc}
	.size	__nand_flush_sector_write_cache, .-__nand_flush_sector_write_cache
	.align	2
	.global	flush_write_cache_to_nand
	.syntax unified
	.arm
	.type	flush_write_cache_to_nand, %function
flush_write_cache_to_nand:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, .L2730
	push	{r4, r5, r6, r7, r8, lr}
	mov	r4, r0
	ldr	r5, [r0, #1608]
	sub	sp, sp, #16
	mov	ip, r3
	mov	r2, sp
	ldr	r0, [r3]	@ unaligned
	ldr	r1, [r3, #4]	@ unaligned
	cmp	r5, #0
	add	r3, sp, #8
	stmia	r2!, {r0, r1}
	ldr	r0, [ip, #8]!	@ unaligned
	ldr	r1, [ip, #4]	@ unaligned
	stmia	r3!, {r0, r1}
	beq	.L2725
	ldr	r2, [r5, #28]
	add	r6, r4, #584
	ldr	r3, [r5, #24]
	mov	r0, #255
	ldr	r1, [r4, #4]
	ldr	r7, [r5, #12]
	str	r3, [r2, #24]
	ldr	r3, [r5, #24]
	cmp	r3, #0
	strne	r2, [r3, #28]
	mov	r2, #0
	ldrh	r3, [r6, #4]
	str	r2, [r5, #24]
	str	r2, [r5, #28]
	sub	r3, r3, #1
	strh	r0, [r5, #2]	@ movhi
	ldrh	r2, [r5, #6]
	strh	r3, [r6, #4]	@ movhi
	ldrb	r3, [r1, #8]	@ zero_extendqisi2
	cmp	r2, r3
	beq	.L2695
	ldr	r3, [r4, #2004]
	mov	r0, r4
	ldr	r2, [r4, #1616]
	ldr	r1, [r5, #8]
	blx	r3
	mov	r8, r0
	ldrh	r0, [r5, #6]
	ldr	r3, [r4, #4]
	ldr	r1, [r4, #1616]
	lsl	r0, r0, #9
	ldrh	r2, [r3, #26]
	uxth	r0, r0
	sub	r2, r2, r0
	add	r1, r1, r0
	add	r0, r7, r0
	bl	NAND_Memcpy
	cmp	r8, #10
	beq	.L2726
.L2724:
	ldr	r1, [r4, #4]
.L2695:
	mov	r2, #0
	ldr	r3, [r4, #2000]
	strh	r2, [r5, #4]	@ movhi
	mov	r0, r4
	ldrb	ip, [r1, #8]	@ zero_extendqisi2
	mov	r2, r7
	ldr	r1, [r5, #8]
	strh	ip, [r5, #6]	@ movhi
	blx	r3
	cmp	r0, #0
	bne	.L2727
.L2697:
	ldr	r8, [r4, #4]
	mov	r1, sp
	mov	r2, #8
	mov	r0, r8
	bl	strncmp
	tst	r0, #255
	bne	.L2728
.L2698:
	add	r3, r4, #320
	ldrh	r0, [r3]
	cmp	r0, #0
	bne	.L2729
.L2699:
	add	r0, r4, #1552
	b	.L2701
.L2702:
	mov	r0, r3
.L2701:
	ldr	r3, [r0, #16]
	cmp	r3, #0
	bne	.L2702
	ldrh	r2, [r6, #6]
	mov	r1, #2
	str	r5, [r0, #16]
	str	r3, [r5, #16]
	add	r3, r2, #1
	str	r0, [r5, #20]
	strh	r1, [r5, #2]	@ movhi
	strh	r3, [r6, #6]	@ movhi
.L2692:
	mov	r0, #0
	add	sp, sp, #16
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L2728:
	add	r1, sp, #8
	mov	r0, r8
	mov	r2, #8
	bl	strncmp
	tst	r0, #255
	bne	.L2699
	b	.L2698
.L2727:
	movw	r0, #:lower16:.LC301
	movt	r0, #:upper16:.LC301
	bl	NAND_Print
	b	.L2697
.L2729:
	sub	r0, r0, #1
	bl	nand_phy_get_page_type
	cmp	r0, #2
	bne	.L2699
	mov	r2, r7
	ldr	r3, [r4, #2000]
	ldr	r1, [r5, #8]
	mov	r0, r4
	blx	r3
	cmp	r0, #0
	beq	.L2699
	movw	r0, #:lower16:.LC301
	movt	r0, #:upper16:.LC301
	bl	NAND_Print
	b	.L2699
.L2726:
	movw	r0, #:lower16:.LC300
	ldrh	r2, [r6, #6]
	ldr	r1, [r5, #8]
	movt	r0, #:upper16:.LC300
	bl	NAND_Print
	b	.L2724
.L2725:
	movw	r0, #:lower16:.LC299
	movt	r0, #:upper16:.LC299
	bl	NAND_Print
	b	.L2692
.L2731:
	.align	2
.L2730:
	.word	.LANCHOR2+844
	.size	flush_write_cache_to_nand, .-flush_write_cache_to_nand
	.align	2
	.global	nand_cache_write
	.syntax unified
	.arm
	.type	nand_cache_write, %function
nand_cache_write:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	subs	r10, r3, #0
	sub	sp, sp, #12
	beq	.L2836
	mov	r4, r2
	ldr	r2, [r0, #1972]
	lsl	r7, r4, #1
	lsl	r8, r4, #9
	ldrh	r3, [r2]
	mov	r6, r0
	mov	r5, r1
	and	r7, r7, #65024
	and	r8, r8, #65024
	cmp	r3, #0
	bne	.L2837
	ldrh	r3, [r2, #2]
	cmp	r3, #0
	beq	.L2838
.L2736:
	ldr	r4, [r6, #1608]
	cmp	r4, #0
	bne	.L2754
	b	.L2750
.L2839:
	ldr	r4, [r4, #24]
	cmp	r4, #0
	beq	.L2750
.L2754:
	ldr	r3, [r4, #8]
	cmp	r5, r3
	bne	.L2839
	ldr	r2, [r4, #28]
	add	r9, r6, #584
	ldr	r3, [r4, #24]
	mov	r0, #255
	ldr	r1, [r6, #4]
	str	r3, [r2, #24]
	ldr	r3, [r4, #24]
	cmp	r3, #0
	strne	r2, [r3, #28]
	mov	r2, #0
	ldrh	r3, [r9, #4]
	str	r2, [r4, #24]
	str	r2, [r4, #28]
	sub	r3, r3, #1
	strh	r0, [r4, #2]	@ movhi
	strh	r3, [r9, #4]	@ movhi
	ldrh	r3, [r4, #6]
	ldrb	r2, [r1, #8]	@ zero_extendqisi2
	ldr	r1, [r4, #12]
	cmp	r2, r3
	str	r1, [sp]
	beq	.L2756
	lsl	fp, r3, #9
	uxth	fp, fp
	cmp	r7, fp
	bne	.L2757
	add	r3, r3, r8, lsr #9
	strh	r3, [r4, #6]	@ movhi
.L2756:
	ldr	r3, [sp]
	mov	r1, r10
	mov	r2, r8
	add	r0, r3, r7
	bl	NAND_Memcpy
	add	r1, r6, #1584
	b	.L2759
.L2799:
	mov	r1, r2
.L2759:
	ldr	r2, [r1, #24]
	cmp	r2, #0
	bne	.L2799
	ldr	r3, [r6, #1568]
	mov	ip, #1
	ldrh	r0, [r9, #4]
	str	r4, [r1, #24]
	cmp	r3, #0
	str	r2, [r4, #24]
	add	r2, r0, ip
	str	r1, [r4, #28]
	strh	ip, [r4, #2]	@ movhi
	strh	r2, [r9, #4]	@ movhi
	bne	.L2764
	b	.L2760
.L2840:
	ldr	r3, [r3, #16]
	cmp	r3, #0
	beq	.L2760
.L2764:
	ldr	r2, [r3, #8]
	cmp	r5, r2
	bne	.L2840
	ldr	r1, [r3, #20]
	ldr	r2, [r3, #16]
	str	r2, [r1, #16]
	ldr	r2, [r3, #16]
	cmp	r2, #0
	strne	r1, [r2, #20]
	mov	r1, #255
	ldrh	r2, [r3, #2]
	cmp	r2, #0
	ldrheq	r2, [r9, #2]
	ldrhne	r2, [r9, #6]
	subeq	r2, r2, #1
	subne	r2, r2, #1
	strheq	r2, [r9, #2]	@ movhi
	strhne	r2, [r9, #6]	@ movhi
	mov	r2, #0
	mov	r0, r2
	strh	r1, [r3, #2]	@ movhi
	str	r2, [r3, #16]
	str	r2, [r3, #20]
.L2822:
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L2838:
	ldr	r3, [r0, #4]
	ldrb	r3, [r3, #8]	@ zero_extendqisi2
	cmp	r3, r4
	bne	.L2736
	ldrh	r3, [r2, #26]
	cmp	r3, #0
	bne	.L2736
	ldr	r3, [r0, #1608]
	cmp	r3, #0
	bne	.L2741
	b	.L2737
.L2841:
	ldr	r3, [r3, #24]
	cmp	r3, #0
	beq	.L2737
.L2741:
	ldr	r2, [r3, #8]
	cmp	r5, r2
	bne	.L2841
	ldr	r1, [r3, #28]
	mov	r0, #0
	ldr	r2, [r3, #24]
	mov	ip, #255
	str	r2, [r1, #24]
	ldr	r2, [r3, #24]
	cmp	r2, #0
	strne	r1, [r2, #28]
	add	r2, r6, #584
	ldrh	r1, [r2, #4]
	str	r0, [r3, #24]
	str	r0, [r3, #28]
	sub	r1, r1, #1
	strh	ip, [r3, #2]	@ movhi
	strh	r1, [r2, #4]	@ movhi
.L2737:
	ldr	r3, [r6, #1568]
	cmp	r3, #0
	bne	.L2746
	b	.L2742
.L2842:
	ldr	r3, [r3, #16]
	cmp	r3, #0
	beq	.L2742
.L2746:
	ldr	r2, [r3, #8]
	cmp	r5, r2
	bne	.L2842
	ldr	r1, [r3, #20]
	ldr	r2, [r3, #16]
	str	r2, [r1, #16]
	ldr	r2, [r3, #16]
	cmp	r2, #0
	strne	r1, [r2, #20]
	add	r1, r6, #584
	ldrh	r2, [r3, #2]
	cmp	r2, #0
	ldrheq	r2, [r1, #2]
	ldrhne	r2, [r1, #6]
	subeq	r2, r2, #1
	subne	r2, r2, #1
	strheq	r2, [r1, #2]	@ movhi
	strhne	r2, [r1, #6]	@ movhi
	mov	r2, #0
	mov	r1, #255
	str	r2, [r3, #16]
	strh	r1, [r3, #2]	@ movhi
	str	r2, [r3, #20]
.L2742:
	mov	r2, r10
	mov	r1, r5
	ldr	r3, [r6, #2000]
	mov	r0, r6
	blx	r3
	cmp	r0, #0
	bne	.L2749
.L2760:
	mov	r0, #0
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L2750:
	ldr	r4, [r6, #1568]
	cmp	r4, #0
	bne	.L2769
	b	.L2770
.L2843:
	ldr	r4, [r4, #16]
	cmp	r4, #0
	beq	.L2770
.L2769:
	ldr	r3, [r4, #8]
	cmp	r5, r3
	bne	.L2843
	ldr	r0, [r4, #12]
	mov	r2, r8
	mov	r1, r10
	add	r9, r6, #584
	add	r0, r0, r7
	bl	NAND_Memcpy
	ldr	r3, [r6, #4]
	mov	r2, #0
	strh	r2, [r4, #4]	@ movhi
	ldr	r2, [r4, #20]
	ldrb	r1, [r3, #8]	@ zero_extendqisi2
	ldr	r3, [r4, #16]
	strh	r1, [r4, #6]	@ movhi
	str	r3, [r2, #16]
	ldr	r3, [r4, #16]
	cmp	r3, #0
	strne	r2, [r3, #20]
	mov	r2, #0
	ldrh	r3, [r4, #2]
	cmp	r3, #0
	ldrheq	r3, [r9, #2]
	ldrhne	r3, [r9, #6]
	subeq	r3, r3, #1
	subne	r3, r3, #1
	strheq	r3, [r9, #2]	@ movhi
	strhne	r3, [r9, #6]	@ movhi
	add	r3, r6, #1584
	str	r2, [r4, #16]
	str	r2, [r4, #20]
	b	.L2773
.L2800:
	mov	r3, r1
.L2773:
	ldr	r1, [r3, #24]
	cmp	r1, #0
	bne	.L2800
	ldrh	r2, [r9, #4]
	mov	r10, #1
	ldr	r0, [r6, #1972]
	str	r4, [r3, #24]
	add	r2, r2, r10
	str	r1, [r4, #24]
	ldrh	r0, [r0, #24]
	uxth	r2, r2
	str	r3, [r4, #28]
	strh	r10, [r4, #2]	@ movhi
	cmp	r0, r2
	strh	r2, [r9, #4]	@ movhi
	bhi	.L2760
	mov	r0, r6
	bl	flush_write_cache_to_nand
	cmp	r0, #0
	beq	.L2760
	movw	r0, #:lower16:.LC305
	movt	r0, #:upper16:.LC305
	bl	NAND_Print
	mov	r0, r10
	b	.L2822
.L2757:
	ldr	r3, [r6, #2004]
	mov	r1, r5
	ldr	r2, [r6, #1616]
	mov	r0, r6
	blx	r3
	ldr	r2, [r6, #4]
	ldr	r3, [sp]
	ldr	r1, [r6, #1616]
	ldrh	r2, [r2, #26]
	str	r0, [sp, #4]
	add	r0, r3, fp
	add	r1, r1, fp
	sub	r2, r2, fp
	bl	NAND_Memcpy
	ldr	r2, [r6, #4]
	ldr	r3, [sp, #4]
	cmp	r3, #10
	ldrb	r3, [r2, #8]	@ zero_extendqisi2
	strh	r3, [r4, #6]	@ movhi
	bne	.L2756
	movw	r0, #:lower16:.LC304
	mov	r1, r5
	movt	r0, #:upper16:.LC304
	bl	NAND_Print
	b	.L2756
.L2837:
	ldr	r4, [r0, #604]
	ldr	r3, [r0, #2004]
	mov	r2, r4
	blx	r3
	mov	r2, r8
	mov	r1, r10
	add	r0, r4, r7
	bl	NAND_Memcpy
	mov	r2, r4
	mov	r1, r5
	ldr	r3, [r6, #2000]
	mov	r0, r6
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	bx	r3
.L2770:
	add	r9, r6, #584
	b	.L2775
.L2776:
	bl	flush_write_cache_to_nand
	cmp	r0, #0
	bne	.L2774
	ldr	r2, [r6, #1972]
.L2775:
	ldrh	r3, [r2, #24]
	mov	r1, #0
	ldrh	r2, [r9, #4]
	mov	r0, r6
	cmp	r2, r3
	bcs	.L2776
	ldrh	r3, [r9]
	cmp	r3, r1
	beq	.L2777
	ldrh	r2, [r9, #10]
	cmp	r2, #255
	beq	.L2844
	sub	r0, r3, #1
	add	r2, r6, #624
	uxth	r0, r0
	mov	r3, #1
	add	r2, r2, #2
	add	r0, r0, r3
	b	.L2780
.L2782:
	ldrh	r1, [r2], #32
	cmp	r1, #255
	beq	.L2778
.L2780:
	cmp	r3, r0
	mov	ip, r3
	add	r3, r3, #1
	bne	.L2782
.L2777:
	mov	r0, r9
	bl	get_empty_cahce_node.part.11
	subs	r4, r0, #0
	beq	.L2845
.L2781:
	cmp	r7, #0
	str	r5, [r4, #8]
	ldr	fp, [r4, #12]
	bne	.L2783
	lsr	r3, r8, #9
	strh	r7, [r4, #4]	@ movhi
	strh	r3, [r4, #6]	@ movhi
.L2784:
	mov	r2, r8
	mov	r1, r10
	add	r0, fp, r7
	add	r6, r6, #1584
	bl	NAND_Memcpy
	b	.L2786
.L2802:
	mov	r6, r3
.L2786:
	ldr	r3, [r6, #24]
	cmp	r3, #0
	bne	.L2802
	ldrh	r2, [r9, #4]
	mov	r1, #1
	str	r4, [r6, #24]
	mov	r0, r3
	str	r3, [r4, #24]
	add	r3, r2, r1
	str	r6, [r4, #28]
	strh	r1, [r4, #2]	@ movhi
	strh	r3, [r9, #4]	@ movhi
	b	.L2822
.L2749:
	movw	r0, #:lower16:.LC303
	mov	r1, r4
	movt	r0, #:upper16:.LC303
	bl	NAND_Print
	mov	r0, #1
	b	.L2822
.L2836:
	movw	r0, #:lower16:.LC302
	movt	r0, #:upper16:.LC302
	bl	NAND_Print
	mov	r0, r10
	b	.L2822
.L2774:
	movw	r0, #:lower16:.LC306
	movt	r0, #:upper16:.LC306
	bl	NAND_Print
	mov	r0, #1
	b	.L2822
.L2844:
	mov	ip, #0
.L2778:
	add	ip, r9, ip, lsl #5
	add	r4, ip, #8
	b	.L2781
.L2783:
	ldr	r3, [r6, #2004]
	mov	r2, fp
	mov	r1, r5
	mov	r0, r6
	blx	r3
	cmp	r0, #10
	beq	.L2846
.L2785:
	ldr	r3, [r6, #4]
	mov	r2, #0
	strh	r2, [r4, #4]	@ movhi
	ldrb	r3, [r3, #8]	@ zero_extendqisi2
	strh	r3, [r4, #6]	@ movhi
	b	.L2784
.L2845:
	movw	r0, #:lower16:.LC15
	ldrh	r3, [r9, #6]
	movt	r0, #:upper16:.LC15
	ldrh	r2, [r9, #2]
	ldrh	r1, [r9, #4]
	bl	NAND_Print
	mov	r0, #1
	b	.L2822
.L2846:
	movw	r0, #:lower16:.LC307
	mov	r1, r5
	movt	r0, #:upper16:.LC307
	bl	NAND_Print
	b	.L2785
	.size	nand_cache_write, .-nand_cache_write
	.align	2
	.global	__nand_flush_write_cache
	.syntax unified
	.arm
	.type	__nand_flush_write_cache, %function
__nand_flush_write_cache:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	mov	r4, #0
	mov	r6, r0
	mov	r7, r1
	mov	r5, r4
	b	.L2848
.L2850:
	add	r4, r4, #1
	bl	flush_write_cache_to_nand
	cmp	r7, r4
	orr	r5, r5, r0
	beq	.L2849
.L2848:
	ldr	r3, [r6, #1608]
	mov	r1, #1
	mov	r0, r6
	cmp	r3, #0
	bne	.L2850
.L2849:
	cmp	r5, #0
	bne	.L2863
.L2851:
	cmp	r4, #0
	beq	.L2852
	ldr	r3, [r6, #356]
	cmp	r3, #0
	bne	.L2864
.L2852:
	mov	r0, r5
	pop	{r4, r5, r6, r7, r8, pc}
.L2864:
	movw	r0, #:lower16:.LC308
	mov	r2, r4
	ldrb	r1, [r6, #16]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC308
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, r7, r8, pc}
.L2863:
	movw	r0, #:lower16:.LC298
	movt	r0, #:upper16:.LC298
	bl	NAND_Print
	b	.L2851
	.size	__nand_flush_write_cache, .-__nand_flush_write_cache
	.align	2
	.global	_nand_flush_write_cache
	.syntax unified
	.arm
	.type	_nand_flush_write_cache, %function
_nand_flush_write_cache:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #12]
	b	__nand_flush_write_cache
	.size	_nand_flush_write_cache, .-_nand_flush_write_cache
	.align	2
	.global	nand_clean_zone_table
	.syntax unified
	.arm
	.type	nand_clean_zone_table, %function
nand_clean_zone_table:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r1, #1000
	mov	r4, r0
	bl	__nand_flush_write_cache
	mov	r0, r4
	bl	clean_block_list
	mov	r0, r4
	bl	nand_clean_zone_table2
	mov	r0, r4
	bl	nftl_cache_exit
	mov	r0, #0
	pop	{r4, pc}
	.size	nand_clean_zone_table, .-nand_clean_zone_table
	.align	2
	.global	nand_discard
	.syntax unified
	.arm
	.type	nand_discard, %function
nand_discard:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #24]
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	mov	r4, r2
	add	r2, r1, r2
	cmp	r2, r3
	bhi	.L2906
	mov	r5, r1
	ldr	r1, [r0, #1968]
	mov	r8, r0
	mov	r0, r5
	ldr	ip, [r8, #4]
	ldrd	r2, [r1, #104]
	adds	r2, r2, r4
	adc	r3, r3, #0
	strd	r2, [r1, #104]
	ldrb	r6, [ip, #8]	@ zero_extendqisi2
	mov	r1, r6
	bl	__aeabi_uidiv
	mov	r1, r6
	mov	r9, r0
	mov	r0, r5
	bl	__aeabi_uidivmod
	cmp	r1, #0
	bne	.L2907
	cmp	r4, r6
	bcc	.L2874
.L2873:
	mov	r1, r6
	mov	r0, r4
	bl	__aeabi_uidiv
	cmp	r0, #0
	beq	.L2874
	add	r6, r9, r0
	mov	r7, #255
	mov	r5, #0
	add	r4, r8, #584
.L2889:
	ldr	r3, [r8, #1972]
	ldrh	r3, [r3]
	cmp	r3, #0
	bne	.L2876
	ldr	r3, [r8, #1608]
	cmp	r3, #0
	bne	.L2881
	b	.L2877
.L2908:
	ldr	r3, [r3, #24]
	cmp	r3, #0
	beq	.L2877
.L2881:
	ldr	r2, [r3, #8]
	cmp	r2, r9
	bne	.L2908
	ldr	r2, [r3, #24]
	ldr	r1, [r3, #28]
	str	r2, [r1, #24]
	ldr	r2, [r3, #24]
	cmp	r2, #0
	strne	r1, [r2, #28]
	ldrh	r2, [r4, #4]
	str	r5, [r3, #24]
	str	r5, [r3, #28]
	sub	r2, r2, #1
	strh	r7, [r3, #2]	@ movhi
	strh	r2, [r4, #4]	@ movhi
.L2877:
	ldr	r3, [r8, #1568]
	cmp	r3, #0
	bne	.L2886
	b	.L2876
.L2909:
	ldr	r3, [r3, #16]
	cmp	r3, #0
	beq	.L2876
.L2886:
	ldr	r2, [r3, #8]
	cmp	r2, r9
	bne	.L2909
	ldr	r2, [r3, #16]
	ldr	r1, [r3, #20]
	str	r2, [r1, #16]
	ldr	r2, [r3, #16]
	cmp	r2, #0
	strne	r1, [r2, #20]
	ldrh	r2, [r3, #2]
	cmp	r2, #0
	ldrheq	r2, [r4, #2]
	ldrhne	r2, [r4, #6]
	subeq	r2, r2, #1
	subne	r2, r2, #1
	strheq	r2, [r4, #2]	@ movhi
	strhne	r2, [r4, #6]	@ movhi
	strh	r7, [r3, #2]	@ movhi
	str	r5, [r3, #16]
	str	r5, [r3, #20]
.L2876:
	mov	r1, r9
	ldr	r3, [r8, #2008]
	mov	r0, r8
	add	r9, r9, #1
	blx	r3
	ldr	r3, [r8, #4]
	ldr	ip, [r8, #1968]
	ldrb	r2, [r3, #8]	@ zero_extendqisi2
	mov	r3, #0
	ldrd	r0, [ip, #112]
	adds	r2, r2, r0
	adc	r3, r3, r1
	cmp	r9, r6
	strd	r2, [ip, #112]
	bne	.L2889
.L2874:
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L2907:
	sub	r3, r6, r1
	uxth	r3, r3
	cmp	r4, r3
	bcc	.L2874
	add	r9, r9, #1
	sub	r4, r4, r3
	b	.L2873
.L2906:
	movw	r0, #:lower16:.LC309
	movt	r0, #:upper16:.LC309
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
	.size	nand_discard, .-nand_discard
	.align	2
	.global	__nand_discard
	.syntax unified
	.arm
	.type	__nand_discard, %function
__nand_discard:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #356]
	push	{r4, r5, r6, lr}
	mov	r4, r0
	mov	r5, r1
	mov	r6, r2
	cmp	r3, #3
	beq	.L2913
.L2911:
	mov	r2, r6
	mov	r1, r5
	mov	r0, r4
	pop	{r4, r5, r6, lr}
	b	nand_discard
.L2913:
	movw	r0, #:lower16:.LC310
	movt	r0, #:upper16:.LC310
	bl	NAND_Print
	b	.L2911
	.size	__nand_discard, .-__nand_discard
	.align	2
	.global	_nand_discard
	.syntax unified
	.arm
	.type	_nand_discard, %function
_nand_discard:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #12]
	b	__nand_discard
	.size	_nand_discard, .-_nand_discard
	.align	2
	.global	__shutdown_op
	.syntax unified
	.arm
	.type	__shutdown_op, %function
__shutdown_op:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.size	__shutdown_op, .-__shutdown_op
	.align	2
	.global	print_logic_page_map
	.syntax unified
	.arm
	.type	print_logic_page_map, %function
print_logic_page_map:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #4]
	push	{r4, r5, r6, r7, r8, r9, lr}
	mov	r5, r0
	sub	sp, sp, #12
	ldr	r0, [r0, #24]
	ldrb	r1, [r3, #8]	@ zero_extendqisi2
	bl	__aeabi_uidiv
	subs	r7, r0, #0
	beq	.L2923
	movw	r6, #:lower16:.LC312
	movw	r9, #:lower16:.LC311
	movt	r6, #:upper16:.LC312
	movt	r9, #:upper16:.LC311
	mov	r4, #0
	movw	r8, #65535
.L2920:
	mov	r1, r4
	mov	r0, r5
	bl	get_logic_page_map
	subs	r3, r0, #0
	mov	r1, r4
	mov	r0, r6
	beq	.L2925
	ldrh	r2, [r3, #2]
	cmp	r2, r8
	beq	.L2919
	ldrb	ip, [r3, #1]	@ zero_extendqisi2
	ldrh	r3, [r3]
	ubfx	ip, ip, #4, #1
	str	ip, [sp]
	ubfx	r3, r3, #0, #12
	bl	NAND_Print
.L2919:
	add	r4, r4, #1
	cmp	r7, r4
	bne	.L2920
.L2923:
	mov	r0, #0
	add	sp, sp, #12
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, pc}
.L2925:
	mov	r0, r9
	bl	NAND_Print_DBG
	b	.L2919
	.size	print_logic_page_map, .-print_logic_page_map
	.align	2
	.global	get_last_nftl
	.syntax unified
	.arm
	.type	get_last_nftl, %function
get_last_nftl:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r0, #0
	bne	.L2928
	b	.L2933
.L2929:
	mov	r0, r3
.L2928:
	ldr	r3, [r0, #8]
	cmp	r3, #0
	bne	.L2929
	bx	lr
.L2933:
	bx	lr
	.size	get_last_nftl, .-get_last_nftl
	.align	2
	.global	get_nftl_need_read_claim
	.syntax unified
	.arm
	.type	get_nftl_need_read_claim, %function
get_nftl_need_read_claim:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r0, #0
	beq	.L2935
	mov	r2, r0
	b	.L2936
.L2942:
	mov	r2, r3
.L2936:
	ldr	r3, [r2, #8]
	cmp	r3, #0
	bne	.L2942
	ldr	r2, [r2, #12]
	ldr	r3, [r0, #12]
	ldr	r2, [r2, #1968]
	ldrb	r3, [r3, #16]	@ zero_extendqisi2
	ldr	r2, [r2, #176]
	cmp	r3, r2
	bne	.L2939
	b	.L2946
.L2940:
	ldr	r3, [r0, #12]
	ldrb	r3, [r3, #16]	@ zero_extendqisi2
	cmp	r3, r2
	bxeq	lr
.L2939:
	ldr	r0, [r0, #8]
	cmp	r0, #0
	bne	.L2940
	bx	lr
.L2935:
	ldr	r3, [r0, #12]
	.inst	0xe7f000f0
.L2946:
	bx	lr
	.size	get_nftl_need_read_claim, .-get_nftl_need_read_claim
	.align	2
	.global	get_nftl_read_claim_page_no
	.syntax unified
	.arm
	.type	get_nftl_read_claim_page_no, %function
get_nftl_read_claim_page_no:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r0, #0
	bne	.L2949
	b	.L2955
.L2950:
	mov	r0, r3
.L2949:
	ldr	r3, [r0, #8]
	cmp	r3, #0
	bne	.L2950
	ldr	r3, [r0, #12]
	ldr	r3, [r3, #1968]
	ldr	r0, [r3, #180]
	bx	lr
.L2955:
	ldr	r3, [r0, #12]
	.inst	0xe7f000f0
	.size	get_nftl_read_claim_page_no, .-get_nftl_read_claim_page_no
	.align	2
	.global	clean_read_reclaim_complete_flag
	.syntax unified
	.arm
	.type	clean_read_reclaim_complete_flag, %function
clean_read_reclaim_complete_flag:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r0, #0
	bxeq	lr
.L2958:
	ldr	r2, [r0, #12]
	ldr	r0, [r0, #8]
	ldr	r3, [r2, #364]
	cmp	r0, #0
	clz	r3, r3
	lsr	r3, r3, #5
	str	r3, [r2, #364]
	bne	.L2958
	bx	lr
	.size	clean_read_reclaim_complete_flag, .-clean_read_reclaim_complete_flag
	.align	2
	.global	set_nftl_read_claim_complete
	.syntax unified
	.arm
	.type	set_nftl_read_claim_complete, %function
set_nftl_read_claim_complete:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	ip, [r1, #12]
	cmp	r0, #0
	mov	r3, #1
	str	r3, [ip, #360]
	beq	.L2962
	mov	r2, r0
	b	.L2963
.L2970:
	mov	r2, r3
.L2963:
	ldr	r3, [r2, #8]
	cmp	r3, #0
	bne	.L2970
	ldr	r3, [r1, #8]
	cmp	r3, #0
	beq	.L2976
.L2968:
	ldr	r2, [r2, #12]
	ldrb	r3, [ip, #16]	@ zero_extendqisi2
	ldr	r1, [r2, #1968]
	add	r3, r3, #1
	str	r3, [r1, #176]
.L2966:
	mov	r0, #0
	str	r0, [r1, #180]
	bx	lr
.L2976:
	ldr	r2, [r2, #12]
	ldr	r1, [r2, #1968]
	str	r3, [r1, #176]
.L2965:
	ldr	r2, [r0, #12]
	ldr	r0, [r0, #8]
	ldr	r3, [r2, #364]
	cmp	r0, #0
	clz	r3, r3
	lsr	r3, r3, #5
	str	r3, [r2, #364]
	bne	.L2965
	b	.L2966
.L2962:
	ldr	r3, [r1, #8]
	cmp	r3, #0
	movne	r2, r0
	bne	.L2968
	ldr	r2, [r3, #12]
	ldr	r1, [r2, #1968]
	str	r3, [r1, #176]
	b	.L2966
	.size	set_nftl_read_claim_complete, .-set_nftl_read_claim_complete
	.align	2
	.global	set_nftl_read_claim_flag
	.syntax unified
	.arm
	.type	set_nftl_read_claim_flag, %function
set_nftl_read_claim_flag:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r0, #0
	beq	.L2982
.L2979:
	ldr	r3, [r0, #12]
	ldr	r0, [r0, #8]
	ldr	r3, [r3, #1968]
	cmp	r0, #0
	str	r1, [r3, #176]
	str	r2, [r3, #180]
	bne	.L2979
.L2982:
	mov	r0, #0
	bx	lr
	.size	set_nftl_read_claim_flag, .-set_nftl_read_claim_flag
	.align	2
	.global	get_cold_logic_page
	.syntax unified
	.arm
	.type	get_cold_logic_page, %function
get_cold_logic_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	cmp	r1, r2
	bcs	.L2995
	push	{r4, r5, r6, r7, r8, lr}
	movw	r8, #:lower16:.LC311
	mov	r4, r1
	mov	r6, r2
	mov	r5, r0
	movw	r7, #65535
	movt	r8, #:upper16:.LC311
	b	.L2988
.L2986:
	ldrh	r2, [r3, #2]
	cmp	r2, r7
	beq	.L2987
	ldrb	r2, [r3, #1]	@ zero_extendqisi2
	ldr	r1, [r5, #364]
	ubfx	r0, r2, #4, #1
	cmp	r0, r1
	bne	.L2996
.L2987:
	add	r4, r4, #1
	cmp	r6, r4
	beq	.L2997
.L2988:
	mov	r1, r4
	mov	r0, r5
	bl	get_logic_page_map
	subs	r3, r0, #0
	bne	.L2986
	mov	r0, r8
	add	r4, r4, #1
	bl	NAND_Print_DBG
	cmp	r6, r4
	bne	.L2988
.L2997:
	mvn	r0, #0
	pop	{r4, r5, r6, r7, r8, pc}
.L2996:
	bfi	r2, r1, #4, #1
	mov	r0, r4
	strb	r2, [r3, #1]
	pop	{r4, r5, r6, r7, r8, pc}
.L2995:
	mvn	r0, #0
	bx	lr
	.size	get_cold_logic_page, .-get_cold_logic_page
	.align	2
	.global	read_reclaim
	.syntax unified
	.arm
	.type	read_reclaim, %function
read_reclaim:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	subs	r4, r0, #0
	beq	.L2999
	mov	r6, r1
	mov	r7, r2
	mov	r1, r4
	b	.L3000
.L3003:
	mov	r1, r2
.L3000:
	ldr	r2, [r1, #8]
	cmp	r2, #0
	bne	.L3003
	ldr	r3, [r1, #12]
	ldr	r0, [r6, #12]
	ldr	r1, [r3, #1968]
	ldr	r2, [r0, #28]
	ldr	r1, [r1, #180]
	bl	get_cold_logic_page
	cmn	r0, #1
	mov	r5, r0
	beq	.L3008
	ldr	r0, [r6, #12]
	mov	r3, r7
	mov	r1, r5
	ldr	r2, [r0, #4]
	ldrb	r2, [r2, #8]	@ zero_extendqisi2
	bl	nand_cache_read
	ldr	r3, [r6, #12]
	ldrb	r2, [r3, #16]	@ zero_extendqisi2
.L3002:
	ldr	r3, [r4, #12]
	ldr	r4, [r4, #8]
	ldr	r3, [r3, #1968]
	cmp	r4, #0
	str	r2, [r3, #176]
	str	r5, [r3, #180]
	bne	.L3002
	mov	r0, r4
	pop	{r4, r5, r6, r7, r8, pc}
.L2999:
	ldr	r3, [r4, #12]
	.inst	0xe7f000f0
.L3008:
	mov	r1, r6
	mov	r0, r4
	pop	{r4, r5, r6, r7, r8, lr}
	b	set_nftl_read_claim_complete
	.size	read_reclaim, .-read_reclaim
	.align	2
	.global	nftl_get_boot_cnt
	.syntax unified
	.arm
	.type	nftl_get_boot_cnt, %function
nftl_get_boot_cnt:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #1968]
	ldr	r0, [r3, #144]
	bx	lr
	.size	nftl_get_boot_cnt, .-nftl_get_boot_cnt
	.align	2
	.global	nftl_get_zone_cap
	.syntax unified
	.arm
	.type	nftl_get_zone_cap, %function
nftl_get_zone_cap:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0, #24]
	bx	lr
	.size	nftl_get_zone_cap, .-nftl_get_zone_cap
	.align	2
	.global	nftl_set_zone_test
	.syntax unified
	.arm
	.type	nftl_set_zone_test, %function
nftl_set_zone_test:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r1, #0
	str	r1, [r0, #356]
	bxeq	lr
	b	print_nftl_zone.part.17
	.size	nftl_set_zone_test, .-nftl_set_zone_test
	.align	2
	.global	nftl_get_zone_priv
	.syntax unified
	.arm
	.type	nftl_get_zone_priv, %function
nftl_get_zone_priv:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r0, [r0]
	bx	lr
	.size	nftl_get_zone_priv, .-nftl_get_zone_priv
	.align	2
	.global	nftl_get_zone_write_cache_nums
	.syntax unified
	.arm
	.type	nftl_get_zone_write_cache_nums, %function
nftl_get_zone_write_cache_nums:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	add	r0, r0, #588
	ldrh	r0, [r0]
	bx	lr
	.size	nftl_get_zone_write_cache_nums, .-nftl_get_zone_write_cache_nums
	.align	2
	.global	print_smart
	.syntax unified
	.arm
	.type	print_smart, %function
print_smart:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #356]
	cmp	r3, #0
	bxeq	lr
	b	print_smart.part.16
	.size	print_smart, .-print_smart
	.align	2
	.global	nand_crc16
	.syntax unified
	.arm
	.type	nand_crc16, %function
nand_crc16:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	cmp	r2, #0
	bxeq	lr
	movw	ip, #:lower16:.LANCHOR2
	add	r2, r1, r2
	movt	ip, #:upper16:.LANCHOR2
.L3019:
	ldrb	r3, [r1], #1	@ zero_extendqisi2
	eor	r3, r3, r0
	cmp	r1, r2
	uxtb	r3, r3
	lsl	r3, r3, #1
	ldrh	r3, [ip, r3]
	eor	r0, r3, r0, lsr #8
	bne	.L3019
	bx	lr
	.size	nand_crc16, .-nand_crc16
	.align	2
	.global	do_write_error
	.syntax unified
	.arm
	.type	do_write_error, %function
do_write_error:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	movw	r10, #:lower16:.LC314
	sub	sp, sp, #24
	mov	r5, r0
	mov	r6, r1
	movt	r10, #:upper16:.LC314
	mov	r9, #0
.L3025:
	mov	r0, r5
	bl	out_phy_block_from_free_list
	subs	r8, r0, #0
	beq	.L3030
	ldr	r3, [r5, #4]
	add	r1, sp, #8
	ldrh	r2, [r8]
	mov	r0, r5
	strh	r9, [sp, #8]	@ movhi
	ldrh	r3, [r3, #28]
	strh	r2, [sp, #10]	@ movhi
	str	r9, [sp, #16]
	strh	r3, [sp, #12]	@ movhi
	str	r9, [sp, #20]
	ldr	r3, [r5, #1976]
	blx	r3
	ldrh	r3, [r8, #6]
	cmp	r0, #0
	strh	r9, [r8, #4]	@ movhi
	add	r3, r3, #1
	strh	r3, [r8, #6]	@ movhi
	bne	.L3054
	ldr	r2, [r6]
	ldrh	r1, [r6, #28]
	ldr	r3, [r2, #8]
	cmp	r1, #0
	add	r3, r3, #1
	str	r3, [r8, #8]
	beq	.L3031
	ldr	r3, [r6, #12]
	ldr	r3, [r3]
	cmn	r3, #1
	beq	.L3031
	mov	r4, r0
	b	.L3032
.L3034:
	ldrh	r3, [r6, #28]
	cmp	r3, r4
	bls	.L3036
	ldr	r3, [r6, #12]
	ldr	r3, [r3, r4, lsl #2]
	cmn	r3, #1
	beq	.L3036
	ldr	r2, [r6]
.L3032:
	uxth	r0, r4
	ldr	r3, [r5, #284]
	mov	r1, r2
	ldr	r7, [r5, #1988]
	mov	r2, r8
	str	r0, [sp]
	mov	r0, r5
	add	r4, r4, #1
	blx	r7
	cmp	r0, #0
	beq	.L3034
	ldr	r3, [r5, #4]
	add	r1, sp, #8
	ldrh	r2, [r8]
	mov	r0, r5
	strh	r9, [sp, #8]	@ movhi
	ldrh	r3, [r3, #28]
	strh	r2, [sp, #10]	@ movhi
	str	r9, [sp, #16]
	strh	r3, [sp, #12]	@ movhi
	str	r9, [sp, #20]
	ldr	r3, [r5, #1996]
	blx	r3
	b	.L3025
.L3036:
	ldr	r2, [r6]
.L3031:
	ldr	r3, [r5, #4]
	mov	r4, #0
	ldrh	r0, [r2, #4]
	add	r1, sp, #8
	ldrh	r2, [r2]
	ldrh	r3, [r3, #28]
	strh	r4, [sp, #8]	@ movhi
	strh	r0, [r8, #4]	@ movhi
	mov	r0, r5
	strh	r2, [sp, #10]	@ movhi
	strh	r3, [sp, #12]	@ movhi
	str	r4, [sp, #16]
	ldr	r3, [r5, #1996]
	str	r4, [sp, #20]
	blx	r3
	ldr	r3, [r6]
	ldrh	r2, [r6, #28]
	ldrh	r3, [r3, #2]
	cmp	r2, r4
	movne	r7, r4
	strh	r3, [r8, #2]	@ movhi
	str	r8, [r6]
	beq	.L3042
.L3041:
	ldr	r3, [r6, #12]
	mov	r0, r5
	ldr	r3, [r3, r7, lsl #2]
	cmn	r3, #1
	mov	r1, r3
	beq	.L3039
	bl	get_logic_page_map
	cmp	r0, #0
	ldrne	r2, [r6]
	ldrhne	r3, [r0]
	ldrheq	r2, [r6, #28]
	ldrhne	r1, [r2]
	bfine	r3, r4, #0, #12
	strhne	r3, [r0]	@ movhi
	ldrhne	r2, [r6, #28]
	strhne	r1, [r0, #2]	@ movhi
.L3039:
	add	r7, r7, #1
	add	r4, r4, #1
	cmp	r2, r7
	ubfx	r4, r4, #0, #12
	bhi	.L3041
.L3042:
	mov	r0, #0
.L3027:
	add	sp, sp, #24
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L3054:
	ldr	r3, [r5, #1996]
	add	r1, sp, #8
	mov	r0, r5
	blx	r3
	mov	r0, r10
	bl	NAND_Print
	add	r3, r5, #380
	ldrh	r2, [r5, #22]
	ldrh	r3, [r3]
	cmp	r2, r3, lsr #2
	bhi	.L3025
.L3030:
	movw	r0, #:lower16:.LC313
	movt	r0, #:upper16:.LC313
	bl	NAND_Print
	mov	r0, #1
	b	.L3027
	.size	do_write_error, .-do_write_error
	.align	2
	.global	write_phy_data_to_current_block
	.syntax unified
	.arm
	.type	write_phy_data_to_current_block, %function
write_phy_data_to_current_block:
	@ args = 4, pretend = 0, frame = 48
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	sub	sp, sp, #60
	add	r9, sp, #24
	mov	r4, r0
	mov	r6, r1
	mov	r0, r9
	mov	r1, #165
	mov	r5, r2
	mov	r2, #16
	str	r3, [sp, #16]
	ldr	r8, [sp, #96]
	bl	NAND_Memset
	bl	is_physic_info_enable_crc
	cmp	r0, #0
	bne	.L3056
	ldr	r2, [r4, #4]
.L3057:
	movw	fp, #13107
	movw	r3, #:lower16:.LC316
	movt	fp, 43537
	movw	r10, #65535
	movt	r3, #:upper16:.LC316
	str	fp, [sp, #12]
	movt	r10, 43690
	str	r3, [sp, #20]
	mov	r7, #0
	mov	fp, r5
	b	.L3069
.L3064:
	ldr	ip, [sp, #12]
	cmp	r8, ip
	beq	.L3075
.L3063:
	mov	r5, #1
	ldr	r3, [r4, #1984]
	add	r1, sp, #40
	strh	r5, [r0, #2]	@ movhi
	mov	r0, r4
	blx	r3
	adds	r0, r0, #0
	movne	r0, #1
	cmp	r7, r5
	movgt	r0, #0
	cmp	r0, #0
	beq	.L3067
	movw	r0, #:lower16:.LC317
	ldrh	r1, [sp, #42]
	movt	r0, #:upper16:.LC317
	bl	NAND_Print
	mov	r1, r6
	mov	r0, r4
	bl	do_write_error
	cmp	r0, #0
	bne	.L3068
	ldr	r2, [r4, #4]
	add	r7, r7, r5
.L3069:
	ldr	r0, [r6]
	cmp	r8, r10
	ldrh	r3, [r6, #28]
	ldrh	ip, [r2, #28]
	ldrh	r1, [r0]
	str	fp, [sp, #48]
	str	r9, [sp, #52]
	strh	r1, [sp, #42]	@ movhi
	strh	r3, [sp, #40]	@ movhi
	strh	ip, [sp, #44]	@ movhi
	beq	.L3076
	cmp	r8, #192
	bne	.L3064
	ldrh	r2, [r2, #20]
	sub	r2, r2, #1
	cmp	r3, r2
	beq	.L3077
.L3065:
	ldrh	ip, [r0, #6]
	mov	r1, r9
	ldr	r3, [r0, #8]
	mov	r0, r4
	ldr	r2, [sp, #16]
	str	ip, [sp]
	bl	set_oob_logic_page
	ldr	r0, [r6]
	b	.L3063
.L3075:
	ldrh	r2, [r2, #20]
	sub	r2, r2, #1
	cmp	r3, r2
	beq	.L3078
.L3066:
	ldr	r3, [r0, #8]
	ldr	r2, [sp, #12]
.L3074:
	ldrh	ip, [r0, #6]
	mov	r1, r9
	mov	r0, r4
	str	ip, [sp]
	bl	set_oob_special_page
	ldr	r0, [r6]
	b	.L3063
.L3076:
	ldr	r3, [r0, #8]
	mov	r2, r8
	b	.L3074
.L3068:
	movw	r0, #:lower16:.LC318
	movt	r0, #:upper16:.LC318
	bl	NAND_Print
	mov	r0, r5
.L3067:
	add	sp, sp, #60
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3056:
	ldr	r2, [r4, #4]
	movw	r3, #65535
	movt	r3, 43690
	cmp	r8, r3
	ldrb	ip, [r2, #8]	@ zero_extendqisi2
	ldreq	ip, [r4, #308]
	lslne	ip, ip, #9
	uxth	ip, ip
	cmp	ip, #0
	beq	.L3070
	movw	lr, #:lower16:.LANCHOR2
	add	ip, r5, ip
	movt	lr, #:upper16:.LANCHOR2
	mov	r0, r5
	movw	r1, #65535
.L3061:
	ldrb	r3, [r0], #1	@ zero_extendqisi2
	eor	r3, r3, r1
	cmp	r0, ip
	uxtb	r3, r3
	lsl	r3, r3, #1
	ldrh	r3, [lr, r3]
	eor	r1, r3, r1, lsr #8
	bne	.L3061
	ubfx	r3, r1, #8, #8
	uxtb	r1, r1
.L3060:
	strb	r3, [sp, #36]
	strb	r1, [sp, #37]
	b	.L3057
.L3077:
	movw	r0, #:lower16:.LC315
	movt	r0, #:upper16:.LC315
	bl	NAND_Print
	ldr	r0, [r6]
	b	.L3065
.L3078:
	ldr	r0, [sp, #20]
	bl	NAND_Print
	ldr	r0, [r6]
	b	.L3066
.L3070:
	mov	r1, #255
	mov	r3, r1
	b	.L3060
	.size	write_phy_data_to_current_block, .-write_phy_data_to_current_block
	.align	2
	.global	write_logic_data_to_current_block
	.syntax unified
	.arm
	.type	write_logic_data_to_current_block, %function
write_logic_data_to_current_block:
	@ args = 4, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	cmp	r2, #192
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	mov	r4, r0
	sub	sp, sp, #8
	mov	r6, r1
	mov	r7, r3
	beq	.L3102
	mov	ip, r2
	movw	r2, #4369
	movt	r2, 43537
	cmp	ip, r2
	beq	.L3103
	movw	r2, #13107
	movt	r2, 43537
	cmp	ip, r2
	beq	.L3104
.L3091:
	mov	r0, r4
	bl	adjust_invaild_list
	mov	r0, #0
.L3082:
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L3102:
	mov	r1, r3
	bl	get_logic_page_map
	subs	r9, r0, #0
	beq	.L3087
	ldrh	r3, [r9, #2]
	movw	r2, #65535
	cmp	r3, r2
	beq	.L3094
	ldr	r2, [r4, #4]
	add	r3, r3, r3, lsl #3
	ldr	r1, [r6]
	ldr	r8, [r2, #32]
	add	r8, r8, r3, lsl #2
	cmp	r1, r8
	movne	r10, #1
	beq	.L3105
.L3083:
	mov	r2, #192
	mov	r3, r7
	str	r2, [sp]
	mov	r1, r6
	ldr	r2, [sp, #40]
	mov	r0, r4
	bl	write_phy_data_to_current_block
	subs	r5, r0, #0
	bne	.L3106
	ldrh	r2, [r6, #28]
	ldr	r3, [r4, #364]
	ldrh	r0, [r9]
	ldr	lr, [r6]
	ubfx	ip, r2, #0, #12
	clz	r3, r3
	add	r1, r2, #1
	bfi	r0, ip, #0, #12
	lsr	r3, r3, #5
	ldrh	r5, [lr]
	lsr	r2, r0, #8
	ldr	lr, [r6, #12]
	bfi	r2, r3, #4, #1
	strh	r0, [r9]	@ movhi
	strh	r5, [r9, #2]	@ movhi
	strb	r2, [r9, #1]
	str	r7, [lr, ip, lsl #2]
	strh	r1, [r6, #28]	@ movhi
.L3085:
	cmp	r10, #1
	bne	.L3091
	mov	r1, r8
	mov	r0, r4
	bl	phy_block_from_invalid_page_incr
	b	.L3091
.L3103:
	mov	r1, r3
	bl	get_logic_page_map
	subs	r5, r0, #0
	beq	.L3087
	ldrh	r3, [r5, #2]
	movw	r2, #65535
	cmp	r3, r2
	beq	.L3097
	ldr	r2, [r4, #4]
	add	r3, r3, r3, lsl #3
	ldr	r1, [r6]
	ldr	r8, [r2, #32]
	add	r8, r8, r3, lsl #2
	cmp	r1, r8
	movne	r10, #1
	beq	.L3107
.L3088:
	ldrh	r2, [r5]
	movw	r3, #4095
	mvn	r1, #0
	strh	r1, [r5, #2]	@ movhi
	bfi	r2, r3, #0, #12
	ubfx	r3, r2, #8, #8
	strh	r2, [r5]	@ movhi
	orr	r3, r3, #16
	strb	r3, [r5, #1]
	b	.L3085
.L3104:
	ldr	r5, [r1]
	ldr	r2, [sp, #40]
	ldrh	lr, [r5, #4]
	add	lr, lr, #1
	strh	lr, [r5, #4]	@ movhi
	str	ip, [sp]
	bl	write_phy_data_to_current_block
	subs	r5, r0, #0
	bne	.L3108
	ldrh	r3, [r6, #28]
	add	r3, r3, #1
	strh	r3, [r6, #28]	@ movhi
	b	.L3091
.L3094:
	mov	r10, #0
	b	.L3083
.L3097:
	mov	r10, #0
	b	.L3088
.L3106:
	movw	r0, #:lower16:.LC320
	movt	r0, #:upper16:.LC320
	bl	NAND_Print
	mov	r0, r5
	b	.L3082
.L3108:
	movw	r0, #:lower16:.LC321
	movt	r0, #:upper16:.LC321
	bl	NAND_Print
	mov	r0, r5
	b	.L3082
.L3105:
	ldrh	r3, [r8, #4]
	ldrh	r2, [r2, #20]
	add	r3, r3, #1
	uxth	r3, r3
	cmp	r2, r3
	strh	r3, [r8, #4]	@ movhi
	bcs	.L3094
	movw	r0, #:lower16:.LC319
	ldrh	r1, [r6, #28]
	movt	r0, #:upper16:.LC319
	mov	r10, #0
	bl	NAND_Print
	b	.L3083
.L3107:
	ldrh	r3, [r8, #4]
	mvn	r0, #0
	ldrh	ip, [r2, #20]
	ldrh	r2, [r5]
	add	r3, r3, #1
	ldr	r1, [r6, #12]
	uxth	r3, r3
	ubfx	r2, r2, #0, #12
	cmp	ip, r3
	str	r0, [r1, r2, lsl #2]
	strh	r3, [r8, #4]	@ movhi
	bcs	.L3097
	movw	r0, #:lower16:.LC319
	ldrh	r1, [r6, #28]
	movt	r0, #:upper16:.LC319
	mov	r10, #0
	bl	NAND_Print
	b	.L3088
.L3087:
	movw	r0, #:lower16:.LC268
	movt	r0, #:upper16:.LC268
	bl	NAND_Print_DBG
	mov	r0, #1
	b	.L3082
	.size	write_logic_data_to_current_block, .-write_logic_data_to_current_block
	.align	2
	.global	write_data_to_page
	.syntax unified
	.arm
	.type	write_data_to_page, %function
write_data_to_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	sub	sp, sp, #8
	str	r3, [sp]
	mov	r6, r1
	mov	r7, r2
	mov	r8, r3
	mov	r3, r2
	mov	r2, r1
	add	r1, r0, #292
	mov	r4, r0
	bl	write_logic_data_to_current_block
	subs	r5, r0, #0
	bne	.L3113
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	bne	.L3116
.L3111:
	mov	r0, r5
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
.L3116:
	str	r8, [sp]
	mov	r3, r7
	mov	r2, r6
	add	r1, r4, #324
	mov	r0, r4
	bl	write_logic_data_to_current_block
	cmp	r0, #0
	beq	.L3111
.L3113:
	movw	r0, #:lower16:.LC322
	mov	r5, #1
	movt	r0, #:upper16:.LC322
	bl	NAND_Print
	mov	r0, r5
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, r7, r8, pc}
	.size	write_data_to_page, .-write_data_to_page
	.align	2
	.global	_nftl_discard_logic_page
	.syntax unified
	.arm
	.type	_nftl_discard_logic_page, %function
_nftl_discard_logic_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r2, r1
	movw	r1, #4369
	movt	r1, 43537
	mov	r3, #0
	b	write_data_to_page
	.size	_nftl_discard_logic_page, .-_nftl_discard_logic_page
	.align	2
	.global	get_new_current_block
	.syntax unified
	.arm
	.type	get_new_current_block, %function
get_new_current_block:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r1]
	push	{r4, r5, r6, lr}
	mov	r4, r1
	mov	r6, r0
	mov	r1, #255
	ldr	r2, [r4, #16]
	ldr	r0, [r4, #12]
	ldr	r5, [r3, #8]
	bl	NAND_Memset
	movw	r3, #578
	ldrh	r3, [r6, r3]
	cmp	r3, #0
	beq	.L3119
	ldr	r3, [r6, #4]
	mov	r0, r6
	ldrh	r1, [r3, #16]
	bl	out_phy_block_from_free_list_by_erase_time
	cmp	r0, #0
	mov	r3, r0
	str	r0, [r4]
	beq	.L3123
	movw	r0, #:lower16:.LC324
	ldrh	r2, [r3, #6]
	movt	r0, #:upper16:.LC324
	ldrh	r1, [r3]
	bl	NAND_Print
	ldr	ip, [r4]
	b	.L3122
.L3119:
	mov	r0, r6
	bl	out_phy_block_from_free_list
	mov	ip, r0
	str	r0, [r4]
.L3122:
	cmp	ip, #0
	beq	.L3123
	add	r3, r5, #1
	mov	r2, #0
	strh	r2, [r4, #28]	@ movhi
	str	r3, [ip, #8]
	ldrb	r1, [r6, #17]	@ zero_extendqisi2
	ands	r1, r1, #1
	addne	r3, r5, #2
	movne	r0, r2
	strne	r3, [ip, #8]
	moveq	r0, r1
	pop	{r4, r5, r6, pc}
.L3123:
	movw	r0, #:lower16:.LC323
	movt	r0, #:upper16:.LC323
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, pc}
	.size	get_new_current_block, .-get_new_current_block
	.align	2
	.global	new_block_init_for_write
	.syntax unified
	.arm
	.type	new_block_init_for_write, %function
new_block_init_for_write:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #4]
	ldrh	ip, [r1]
	push	{r4, r5, r6, r7, lr}
	mov	r6, #0
	ldrh	r3, [r3, #28]
	sub	sp, sp, #20
	mov	r4, r1
	mov	r1, sp
	strh	r6, [sp]	@ movhi
	mov	r5, r0
	strh	r3, [sp, #4]	@ movhi
	mov	r7, r2
	ldr	r3, [r0, #1976]
	strh	ip, [sp, #2]	@ movhi
	str	r6, [sp, #8]
	str	r6, [sp, #12]
	blx	r3
	ldrh	r3, [r4, #6]
	ldr	r2, [r5, #376]
	strh	r6, [r4, #4]	@ movhi
	add	r3, r3, #1
	uxth	r3, r3
	cmp	r3, r2
	strh	r3, [r4, #6]	@ movhi
	bls	.L3130
	ldr	r2, [r5, #4]
	ldrh	r2, [r2, #16]
	cmp	r2, r3
	strhi	r3, [r5, #376]
.L3130:
	cmp	r0, #0
	str	r7, [r4, #8]
	beq	.L3131
	mov	r1, sp
	ldr	r3, [r5, #1996]
	mov	r0, r5
	blx	r3
	movw	r0, #:lower16:.LC325
	movt	r0, #:upper16:.LC325
	bl	NAND_Print
	mov	r0, #1
.L3131:
	add	sp, sp, #20
	@ sp needed
	pop	{r4, r5, r6, r7, pc}
	.size	new_block_init_for_write, .-new_block_init_for_write
	.align	2
	.global	cross_talk_rebuild_current_block
	.syntax unified
	.arm
	.type	cross_talk_rebuild_current_block, %function
cross_talk_rebuild_current_block:
	@ args = 0, pretend = 0, frame = 56
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	sub	sp, sp, #76
	add	r3, sp, #72
	mov	r4, #0
	mov	r5, r0
	mov	r8, r1
	str	r4, [r3, #-36]!
	add	r0, r0, #4
	mov	r1, r3
	mov	r6, r2
	bl	init_nrc.isra.0
	cmp	r0, r4
	bne	.L3158
	ldr	r3, [sp, #36]
	add	r2, sp, #28
	mov	r1, r8
	mov	r0, r5
	bl	get_used_page_num
	cmp	r6, #0
	mov	r7, r0
	beq	.L3159
	ldr	r3, [sp, #36]
	add	r2, sp, #32
	mov	r1, r6
	mov	r0, r5
	bl	get_used_page_num
	ldr	lr, [r6, #8]
	mov	r9, r0
	ldrh	ip, [r6]
	movw	r0, #:lower16:.LC329
	ldr	r3, [r8, #8]
	movt	r0, #:upper16:.LC329
	ldrh	r1, [r8]
	mov	r2, r7
	stmib	sp, {r9, lr}
	str	ip, [sp]
	bl	NAND_Print
	ldr	r0, [sp, #36]
	ldr	r3, [r0]
	cmp	r3, #0
	beq	.L3160
	cmp	r7, r9
	ldrhls	r2, [r6, #6]
	ldrhhi	r2, [r8, #6]
	ldrhls	r4, [r8, #6]
	ldrhhi	r4, [r6, #6]
	str	r2, [sp, #20]
	ldr	r2, [sp, #28]
	cmp	r2, #0
	beq	.L3161
.L3142:
	ldr	r2, [sp, #20]
	add	r4, r4, #20
	add	r2, r2, #20
	str	r2, [sp, #20]
.L3143:
	cmp	r7, r9
	bcc	.L3162
	movw	r0, #:lower16:.LC332
	mov	r2, r9
	mov	r1, r7
	movt	r0, #:upper16:.LC332
	bl	NAND_Print
	add	r2, r4, #1
	mov	r1, r6
	uxth	r2, r2
	mov	r0, r5
	bl	erase_block
.L3138:
	movw	r6, #:lower16:.LC334
	movt	r6, #:upper16:.LC334
	b	.L3148
.L3145:
	ldr	r2, [r8, #8]
	add	r2, r2, #1
	str	r2, [r4, #8]
	bl	new_block_init_for_write
	subs	r7, r0, #0
	mov	r0, r6
	beq	.L3163
	bl	NAND_Print
.L3148:
	mov	r0, r5
	bl	out_phy_block_from_free_list
	subs	r4, r0, #0
	mov	r0, r5
	mov	r1, r4
	bne	.L3145
	movw	r0, #:lower16:.LC333
	movt	r0, #:upper16:.LC333
	bl	NAND_Print
	mov	r0, r4
.L3136:
	add	sp, sp, #76
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3162:
	movw	r0, #:lower16:.LC331
	mov	r2, r9
	mov	r1, r7
	movt	r0, #:upper16:.LC331
	bl	NAND_Print
	add	r2, r4, #1
	mov	r1, r8
	uxth	r2, r2
	mov	r0, r5
	mov	r8, r6
	bl	erase_block
	b	.L3138
.L3161:
	ldr	r2, [sp, #32]
	cmp	r2, #0
	beq	.L3143
	b	.L3142
.L3163:
	ldr	r3, [sp, #36]
	ldr	ip, [r3]
	cmp	ip, #0
	movwne	r10, #:lower16:.LC335
	movne	r6, r7
	movtne	r10, #:upper16:.LC335
	addne	r9, sp, #40
	bne	.L3151
	b	.L3152
.L3164:
	ldr	r1, [r3, #8]
	ldr	r3, [r5, #4]
	ldrb	r1, [r1, r6]	@ zero_extendqisi2
	ldrh	r3, [r3, #26]
	add	r1, r1, r3
	bl	NAND_Memcpy
	ldrb	ip, [sp, #41]	@ zero_extendqisi2
	mov	r1, r9
	ldrb	r2, [sp, #42]	@ zero_extendqisi2
	mov	r0, r5
	ldrb	lr, [sp, #43]	@ zero_extendqisi2
	ldr	r3, [sp, #36]
	ldrh	fp, [r4]
	orr	r2, r2, ip, lsl #8
	ldr	ip, [r5, #4]
	ldr	r3, [r3, #8]
	orr	r2, lr, r2, lsl #8
	ldrb	lr, [sp, #44]	@ zero_extendqisi2
	ldrh	ip, [ip, #28]
	ldrb	r3, [r3, r6]	@ zero_extendqisi2
	strh	r7, [sp, #56]	@ movhi
	add	r7, r7, #1
	orr	r2, lr, r2, lsl #8
	ldrh	lr, [r4, #6]
	str	r3, [sp, #64]
	ldr	r3, [r4, #8]
	str	lr, [sp]
	strh	fp, [sp, #58]	@ movhi
	str	r9, [sp, #68]
	strh	ip, [sp, #60]	@ movhi
	bl	set_oob_special_page
	ldr	r3, [r5, #1984]
	add	r1, sp, #56
	mov	r0, r5
	blx	r3
.L3150:
	ldr	r3, [sp, #36]
	add	r6, r6, #1
	ldr	ip, [r3]
	cmp	ip, r6
	bls	.L3152
.L3151:
	ldr	lr, [r3, #4]
	mov	r2, #16
	mov	r0, r9
	mov	r1, r6
	ldrb	lr, [lr, r6]	@ zero_extendqisi2
	cmp	lr, #0
	bne	.L3164
	mov	r2, ip
	mov	r0, r10
	bl	NAND_Print
	b	.L3150
.L3152:
	ldr	r3, [sp, #20]
	mov	r1, r8
	mov	r0, r5
	add	r2, r3, #1
	uxth	r2, r2
	bl	erase_block
	ldr	r0, [sp, #36]
	bl	free_nrc
	mov	r0, r4
	add	sp, sp, #76
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3159:
	movw	r0, #:lower16:.LC327
	mov	r2, r7
	movt	r0, #:upper16:.LC327
	ldr	r3, [r8, #8]
	ldrh	r1, [r8]
	bl	NAND_Print
	ldr	r0, [sp, #36]
	ldr	r4, [r0]
	cmp	r4, #0
	bne	.L3138
	bl	free_nrc
	movw	r0, #:lower16:.LC328
	movt	r0, #:upper16:.LC328
	bl	NAND_Print
	ldrh	r2, [r8, #6]
	mov	r0, r5
	mov	r1, r8
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	mov	r0, r4
	b	.L3136
.L3158:
	ldr	r0, [sp, #36]
	bl	free_nrc
	movw	r0, #:lower16:.LC326
	movt	r0, #:upper16:.LC326
	bl	NAND_Print
	mov	r0, r4
	add	sp, sp, #76
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3160:
	str	r3, [sp, #20]
	bl	free_nrc
	movw	r0, #:lower16:.LC330
	movt	r0, #:upper16:.LC330
	bl	NAND_Print
	ldrh	r2, [r8, #6]
	mov	r1, r8
	mov	r0, r5
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	ldrh	r2, [r6, #6]
	mov	r0, r5
	mov	r1, r6
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	ldr	r3, [sp, #20]
	mov	r0, r3
	b	.L3136
	.size	cross_talk_rebuild_current_block, .-cross_talk_rebuild_current_block
	.align	2
	.global	no_cross_talk_rebuild_current_block
	.syntax unified
	.arm
	.type	no_cross_talk_rebuild_current_block, %function
no_cross_talk_rebuild_current_block:
	@ args = 0, pretend = 0, frame = 56
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	add	r7, r0, #4
	sub	sp, sp, #72
	mov	r4, #0
	mov	r5, r0
	mov	r6, r1
	mov	r0, r7
	add	r1, sp, #32
	mov	r9, r2
	str	r4, [sp, #32]
	str	r4, [sp, #36]
	bl	init_nrc.isra.0
	subs	r8, r0, #0
	bne	.L3199
	ldr	r3, [sp, #32]
	add	r2, sp, #24
	mov	r1, r6
	mov	r0, r5
	bl	get_used_page_num_no_crosstalk
	cmp	r9, #0
	mov	r4, r0
	beq	.L3200
	mov	r0, r7
	add	r1, sp, #36
	bl	init_nrc.isra.0
	cmp	r0, #0
	bne	.L3201
	ldr	r3, [sp, #36]
	add	r2, sp, #28
	mov	r1, r9
	mov	r0, r5
	bl	get_used_page_num_no_crosstalk
	ldr	ip, [r9, #8]
	mov	r8, r0
	ldr	r3, [r6, #8]
	movw	r0, #:lower16:.LC339
	ldrh	r1, [r6]
	movt	r0, #:upper16:.LC339
	stmib	sp, {r8, ip}
	mov	r2, r4
	ldrh	ip, [r9]
	str	ip, [sp]
	bl	NAND_Print
	ldr	r7, [sp, #36]
	ldr	r3, [r7]
	cmp	r3, #0
	beq	.L3202
	cmp	r4, r8
	bcc	.L3203
	ldr	r7, [sp, #32]
	movw	r0, #:lower16:.LC332
	mov	r2, r8
	mov	r1, r4
	movt	r0, #:upper16:.LC332
	ldr	r3, [r7]
	bl	NAND_Print
	mov	r3, r9
	mov	r9, r6
	mov	r6, r3
.L3175:
	ldr	r3, [r5, #4]
	mov	r4, #0
	ldrh	r2, [r6]
	add	r1, sp, #40
	strh	r4, [sp, #40]	@ movhi
	mov	r0, r5
	ldrh	r3, [r3, #28]
	strh	r2, [sp, #42]	@ movhi
	str	r4, [sp, #48]
	strh	r3, [sp, #44]	@ movhi
	str	r4, [sp, #52]
	ldr	r3, [r5, #1976]
	blx	r3
	cmp	r0, r4
	bne	.L3204
.L3176:
	ldr	r3, [r9, #8]
	str	r3, [r6, #8]
.L3171:
	ldr	r2, [r7]
	cmp	r2, #0
	movne	r8, #0
	movwne	r10, #:lower16:.LC335
	movne	r4, r8
	movtne	r10, #:upper16:.LC335
	beq	.L3184
.L3183:
	ldr	r3, [r7, #4]
	ldrb	r3, [r3, r4]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L3180
	ldr	r3, [r5, #4]
	mov	r2, #0
	ldr	r0, [r7, #8]
	ldrh	lr, [r6]
	ldrh	r1, [r3, #26]
	ldrh	ip, [r3, #28]
	strh	r8, [sp, #56]	@ movhi
	add	r3, r1, #64
	strh	lr, [sp, #58]	@ movhi
	mul	r3, r4, r3
	strh	ip, [sp, #60]	@ movhi
	add	r1, r1, r3
	add	r3, r0, r3
	add	r1, r0, r1
	str	r3, [sp, #64]
	add	ip, r1, #4
	mov	r3, r1
	str	r1, [sp, #68]
.L3181:
	ldrb	r0, [r3, #1]!	@ zero_extendqisi2
	cmp	ip, r3
	orr	r2, r0, r2, lsl #8
	bne	.L3181
	ldrh	ip, [r6, #6]
	mov	r0, r5
	ldr	r3, [r6, #8]
	add	r8, r8, #1
	str	ip, [sp]
	bl	set_oob_special_page
	ldr	r3, [r5, #1984]
	add	r1, sp, #56
	mov	r0, r5
	blx	r3
.L3182:
	ldr	r2, [r7]
	add	r4, r4, #1
	cmp	r2, r4
	bhi	.L3183
.L3184:
	ldrh	r2, [r9, #6]
	mov	r1, r9
	mov	r0, r5
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	ldr	r0, [sp, #32]
	bl	free_nrc
	ldr	r0, [sp, #36]
	bl	free_nrc
	mov	r0, r6
.L3167:
	add	sp, sp, #72
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L3203:
	movw	r0, #:lower16:.LC331
	mov	r2, r8
	mov	r1, r4
	movt	r0, #:upper16:.LC331
	bl	NAND_Print
	b	.L3175
.L3180:
	mov	r1, r4
	mov	r0, r10
	bl	NAND_Print
	b	.L3182
.L3204:
	movw	r0, #:lower16:.LC341
	ldrh	r1, [r6]
	movt	r0, #:upper16:.LC341
	movw	r8, #:lower16:.LC334
	bl	NAND_Print
	ldr	r3, [r5, #1996]
	add	r1, sp, #40
	mov	r0, r5
	movt	r8, #:upper16:.LC334
	blx	r3
	ldrh	r3, [r5, #20]
	mov	r2, #255
	strh	r4, [r9, #4]	@ movhi
	strh	r2, [r9, #2]	@ movhi
	add	r3, r3, #1
	strh	r3, [r5, #20]	@ movhi
	b	.L3177
.L3205:
	ldr	r2, [r6, #8]
	bl	new_block_init_for_write
	cmp	r0, #0
	mov	r0, r8
	beq	.L3176
	bl	NAND_Print
.L3177:
	mov	r0, r5
	bl	out_phy_block_from_free_list
	subs	r6, r0, #0
	mov	r0, r5
	mov	r1, r6
	bne	.L3205
.L3178:
	movw	r0, #:lower16:.LC333
	movt	r0, #:upper16:.LC333
	bl	NAND_Print
	mov	r0, #0
	add	sp, sp, #72
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L3200:
	movw	r0, #:lower16:.LC337
	mov	r2, r4
	movt	r0, #:upper16:.LC337
	ldr	r3, [r6, #8]
	ldrh	r1, [r6]
	bl	NAND_Print
	ldr	r7, [sp, #32]
	ldr	r4, [r7]
	cmp	r4, #0
	movwne	r8, #:lower16:.LC334
	movtne	r8, #:upper16:.LC334
	bne	.L3197
	b	.L3206
.L3170:
	ldr	r2, [r6, #8]
	str	r2, [r4, #8]
	bl	new_block_init_for_write
	cmp	r0, #0
	mov	r0, r8
	beq	.L3185
	bl	NAND_Print
.L3197:
	mov	r0, r5
	bl	out_phy_block_from_free_list
	subs	r4, r0, #0
	mov	r0, r5
	mov	r1, r4
	bne	.L3170
	b	.L3178
.L3199:
	movw	r0, #:lower16:.LC336
	movt	r0, #:upper16:.LC336
	bl	NAND_Print
	ldr	r0, [sp, #32]
	bl	free_nrc
	mov	r0, r4
	add	sp, sp, #72
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L3201:
	ldr	r0, [sp, #32]
	bl	free_nrc
	ldr	r0, [sp, #36]
	bl	free_nrc
	movw	r0, #:lower16:.LC330
	movt	r0, #:upper16:.LC330
	bl	NAND_Print
	mov	r0, r8
	b	.L3167
.L3185:
	mov	r9, r6
	mov	r6, r4
	b	.L3171
.L3202:
	ldr	r0, [sp, #32]
	str	r3, [sp, #20]
	bl	free_nrc
	ldr	r0, [sp, #36]
	bl	free_nrc
	movw	r0, #:lower16:.LC340
	movt	r0, #:upper16:.LC340
	bl	NAND_Print
	ldrh	r2, [r6, #6]
	mov	r1, r6
	mov	r0, r5
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	ldrh	r2, [r9, #6]
	mov	r0, r5
	mov	r1, r9
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	ldr	r3, [sp, #20]
	mov	r0, r3
	b	.L3167
.L3206:
	mov	r0, r7
	bl	free_nrc
	movw	r0, #:lower16:.LC338
	movt	r0, #:upper16:.LC338
	bl	NAND_Print
	ldrh	r2, [r6, #6]
	mov	r0, r5
	mov	r1, r6
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	mov	r0, r4
	b	.L3167
	.size	no_cross_talk_rebuild_current_block, .-no_cross_talk_rebuild_current_block
	.align	2
	.global	current_block_ecc_error
	.syntax unified
	.arm
	.type	current_block_ecc_error, %function
current_block_ecc_error:
	@ args = 0, pretend = 0, frame = 56
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	movw	r8, #:lower16:.LC342
	sub	sp, sp, #60
	mov	r4, r0
	mov	r6, r1
	movt	r8, #:upper16:.LC342
	b	.L3212
.L3209:
	ldr	r2, [r6, #8]
	str	r2, [r5, #8]
	bl	new_block_init_for_write
	subs	r7, r0, #0
	mov	r0, r8
	beq	.L3211
	bl	NAND_Print
.L3208:
.L3212:
	mov	r0, r4
	bl	out_phy_block_from_free_list
	subs	r5, r0, #0
	mov	r0, r4
	mov	r1, r5
	bne	.L3209
	movw	r0, #:lower16:.LC223
	movt	r0, #:upper16:.LC223
	bl	NAND_Print
	mov	r0, r5
	add	sp, sp, #60
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3211:
	movw	r0, #:lower16:.LC343
	ldrh	r1, [r6]
	movt	r0, #:upper16:.LC343
	bl	NAND_Print
	ldr	r3, [r4, #4]
	ldrh	r2, [r3, #20]
	cmp	r2, #0
	beq	.L3225
	movw	r8, #:lower16:.LC346
	movw	r10, #:lower16:.LC345
	movw	r9, #:lower16:.LC344
	mov	fp, r7
	movt	r8, #:upper16:.LC346
	movt	r10, #:upper16:.LC345
	movt	r9, #:upper16:.LC344
.L3221:
	ldrh	ip, [r3, #28]
	add	r1, sp, #24
	ldrh	r2, [r6]
	mov	r0, r4
	ldr	r3, [r4, #284]
	strh	ip, [sp, #28]	@ movhi
	add	ip, sp, #8
	strh	r2, [sp, #26]	@ movhi
	str	r3, [sp, #32]
	strh	fp, [sp, #24]	@ movhi
	ldr	r3, [r4, #1980]
	str	ip, [sp, #36]
	blx	r3
	ldrb	r2, [sp, #9]	@ zero_extendqisi2
	mov	r3, r0
	and	r1, r2, #240
	cmp	r1, #192
	beq	.L3234
	cmp	r2, #170
	beq	.L3222
	add	r3, sp, #8
	add	r1, sp, #18
	mov	r0, #0
.L3219:
	ldrb	r2, [r3], #1	@ zero_extendqisi2
	cmp	r2, #255
	addeq	r0, r0, #1
	cmp	r1, r3
	bne	.L3219
	cmp	r0, #8
	bls	.L3220
	ldrb	r3, [sp, #8]	@ zero_extendqisi2
	cmp	r3, #255
	beq	.L3217
.L3220:
	movw	r0, #:lower16:.LC349
	mov	r1, fp
	movt	r0, #:upper16:.LC349
	bl	NAND_Print
.L3217:
	ldr	r3, [r4, #4]
	add	fp, fp, #1
	ldrh	r2, [r3, #20]
	cmp	r2, fp
	bgt	.L3221
.L3213:
	ldrh	r2, [r6, #6]
	mov	r1, r6
	mov	r0, r4
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	movw	r0, #:lower16:.LC347
	mov	r2, r7
	mov	r1, fp
	movt	r0, #:upper16:.LC347
	bl	NAND_Print
	mov	r0, r5
	add	sp, sp, #60
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3234:
	cmp	r0, #10
	cmpne	r0, #0
	bne	.L3235
	add	r2, sp, #8
	ldr	r1, [r4, #284]
	mov	r0, r4
	str	r3, [sp, #4]
	bl	nand_power_off_ecc_error
	cmp	r0, #0
	ldr	r3, [sp, #4]
	beq	.L3216
	mov	r0, r9
	bl	NAND_Print
	b	.L3217
.L3222:
	movw	r0, #:lower16:.LC348
	mov	r1, fp
	movt	r0, #:upper16:.LC348
	bl	NAND_Print
	b	.L3217
.L3235:
	mov	r1, fp
	mov	r0, r8
	bl	NAND_Print
	b	.L3217
.L3216:
	mov	r2, r3
	mov	r1, fp
	mov	r0, r10
	bl	NAND_Print
	ldr	r3, [r4, #4]
	add	r1, sp, #40
	ldrh	ip, [r5]
	mov	r0, r4
	strh	r7, [sp, #40]	@ movhi
	add	r7, r7, #1
	ldrh	r2, [r3, #28]
	ldr	r3, [r4, #284]
	strh	ip, [sp, #42]	@ movhi
	strh	r2, [sp, #44]	@ movhi
	add	r2, sp, #8
	str	r3, [sp, #48]
	str	r2, [sp, #52]
	ldr	r3, [r4, #1984]
	blx	r3
	b	.L3217
.L3225:
	mov	r7, r2
	mov	fp, r2
	b	.L3213
	.size	current_block_ecc_error, .-current_block_ecc_error
	.align	2
	.global	second_scan_all_blocks
	.syntax unified
	.arm
	.type	second_scan_all_blocks, %function
second_scan_all_blocks:
	@ args = 0, pretend = 0, frame = 64
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	mov	r4, r0
	ldr	r7, [r0, #4]
	sub	sp, sp, #84
	ldr	r0, [r0, #24]
	ldrb	r1, [r7, #8]	@ zero_extendqisi2
	bl	__aeabi_uidiv
	ldr	r5, [r4, #252]
	str	r0, [r4, #28]
	cmp	r5, #0
	beq	.L3237
	ldr	r3, [r4, #1968]
	mov	r6, r5
.L3238:
	ldr	r2, [r6, #8]
	ldr	r6, [r6, #28]
	cmp	r6, #0
	bne	.L3238
	str	r2, [sp, #28]
	movw	fp, #:lower16:.LANCHOR0
	ldr	r2, [r5, #8]
	movw	r1, #65535
	ldr	lr, [sp, #28]
	movw	r0, #:lower16:.LC378
	movw	ip, #:lower16:.LC356
	movt	fp, #:upper16:.LANCHOR0
	mov	r9, r6
	add	r10, sp, #48
	movt	r1, 43690
	movt	r0, #:upper16:.LC378
	movt	ip, #:upper16:.LC356
	str	lr, [r3, #160]
	str	r2, [r3, #164]
	str	r1, [sp, #16]
	str	r0, [sp, #24]
	str	ip, [sp, #36]
	str	r6, [sp, #40]
	str	r6, [sp, #20]
	str	r6, [sp, #44]
	str	r6, [sp, #32]
.L3302:
	ldrh	r3, [r7, #20]
	mov	r2, #1
	ldrh	r1, [r5]
	ldrh	r0, [r7, #28]
	ldr	ip, [r4, #284]
	sub	r3, r3, #1
	strh	r3, [sp, #64]	@ movhi
	strh	r1, [sp, #66]	@ movhi
	add	r1, sp, #64
	strh	r0, [sp, #68]	@ movhi
	mov	r0, r4
	str	ip, [sp, #72]
	str	r2, [fp, #248]
	ldr	r3, [r4, #1980]
	str	r10, [sp, #76]
	blx	r3
	mov	r3, #0
	mov	r6, r0
	str	r3, [fp, #248]
	bl	is_physic_info_enable_crc
	cmp	r0, #0
	bne	.L3390
.L3240:
	ldrh	r3, [r5, #6]
	movw	r1, #49999
	ldr	r0, [r4, #376]
	ldr	r7, [r5, #8]
	cmp	r3, r0
	ldrb	r0, [sp, #49]	@ zero_extendqisi2
	movhi	r2, #1
	movls	r2, #0
	cmp	r3, r1
	movhi	r2, #0
	cmp	r2, #0
	ldr	r2, [r4, #1968]
	strne	r3, [r4, #376]
	ldr	r1, [r2, #156]
	strne	r3, [r2, #152]
	cmp	r3, r1
	ldrb	r1, [sp, #50]	@ zero_extendqisi2
	strcc	r3, [r2, #156]
	ldrb	r2, [sp, #51]	@ zero_extendqisi2
	ldrb	r3, [sp, #52]	@ zero_extendqisi2
	orr	r1, r1, r0, lsl #8
	orr	r2, r2, r1, lsl #8
	orr	r3, r3, r2, lsl #8
	ldr	r2, [sp, #16]
	cmp	r3, r2
	beq	.L3391
.L3245:
	mov	r3, r10
	add	r1, sp, #58
	mov	r0, #0
.L3248:
	ldrb	r2, [r3], #1	@ zero_extendqisi2
	cmp	r2, #255
	addeq	r0, r0, #1
	cmp	r1, r3
	bne	.L3248
	cmp	r0, #8
	ldrb	r1, [sp, #48]	@ zero_extendqisi2
	bls	.L3251
	cmp	r1, #255
	beq	.L3392
.L3251:
	ldrb	lr, [sp, #53]	@ zero_extendqisi2
	ldrb	r8, [sp, #54]	@ zero_extendqisi2
	ldrb	ip, [sp, #52]	@ zero_extendqisi2
	str	lr, [sp, #8]
	ldrb	lr, [sp, #51]	@ zero_extendqisi2
	ldrb	r3, [sp, #50]	@ zero_extendqisi2
	ldrb	r2, [sp, #49]	@ zero_extendqisi2
	ldr	r0, [sp, #24]
	str	r8, [sp, #12]
	str	ip, [sp, #4]
	str	lr, [sp]
	bl	NAND_Print
	cmp	r6, #10
	cmpne	r6, #0
	bne	.L3393
	movw	r0, #:lower16:.LC355
	ldrh	r2, [r5]
	movt	r0, #:upper16:.LC355
	ldrb	r1, [r4, #16]	@ zero_extendqisi2
	bl	NAND_Print
.L3253:
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	ldr	r2, [sp, #28]
	cmp	r2, r7
	movne	r3, #0
	andeq	r3, r3, #1
	cmp	r3, #0
	bne	.L3394
	ldrh	r3, [r5, #4]
	mov	r2, #170
	strh	r2, [r5, #2]	@ movhi
	mov	r1, r5
	ldr	r2, [r4, #304]
	mov	r0, r4
	add	r3, r3, #1
	str	r5, [sp, #20]
	strh	r3, [r5, #4]	@ movhi
	bl	recover_block_phy_page_mapping
	ldr	r3, [r4, #4]
	mov	r1, r5
	ldr	r2, [r4, #304]
	mov	r0, r4
	ldrh	r3, [r3, #20]
	sub	r3, r3, #1
	bl	recover_logic_page_mapping
	mov	r2, #3
	mov	r1, r5
	mov	r0, r4
	bl	add_prio_gc
.L3246:
	ldr	r5, [r5, #28]
	cmp	r5, #0
	beq	.L3395
.L3255:
	ldr	r7, [r4, #4]
	b	.L3302
.L3393:
	ldrh	r2, [r5]
	ldr	r0, [sp, #36]
	ldrb	r1, [r4, #16]	@ zero_extendqisi2
	bl	NAND_Print
	b	.L3253
.L3390:
	mov	r3, r10
	ldr	r2, [r4, #284]
	add	r1, r4, #308
	ldr	r0, [r4, #4]
	bl	crc_wrong.isra.18
	cmp	r0, #0
	mvnne	r6, #1
	b	.L3240
.L3391:
	cmp	r6, #10
	cmpne	r6, #0
	bne	.L3245
	ldr	r3, [r4, #4]
	mov	r1, r5
	ldrh	ip, [r5, #4]
	mov	r0, r4
	ldr	r2, [r4, #284]
	ldrh	r3, [r3, #20]
	add	ip, ip, #1
	str	r5, [sp, #40]
	strh	ip, [r5, #4]	@ movhi
	sub	r3, r3, #1
	bl	recover_logic_page_mapping
	str	r5, [sp, #20]
	ldr	r5, [r5, #28]
	cmp	r5, #0
	bne	.L3255
.L3395:
	ldr	r3, [sp, #20]
	mov	fp, r7
	ldr	r1, [sp, #40]
	mov	r0, r4
	ldr	r8, [sp, #32]
	adds	r7, r3, #0
	ldr	r6, [sp, #44]
	movne	r7, #1
	cmp	r9, #0
	moveq	r5, #1
	movne	r5, #0
	moveq	r7, #0
	andne	r7, r7, #1
	bl	init_smart_info
	ldr	r10, [r4, #252]
	cmp	r8, #0
	movne	r5, #0
	andeq	r5, r5, #1
	cmp	r10, #0
	beq	.L3263
.L3256:
	mov	ip, #0
	add	lr, r4, #188
.L3262:
	cmp	r10, r9
	cmpne	r8, r10
	movne	r3, #1
	moveq	r3, #0
	cmp	r6, r10
	moveq	r3, #0
	andne	r3, r3, #1
	cmp	r3, #0
	beq	.L3259
	str	ip, [r10, #12]
	mov	r2, lr
	str	ip, [r10, #16]
	b	.L3260
.L3261:
	ldrh	r1, [r3, #4]
	ldrh	r0, [r10, #4]
	cmp	r0, r1
	bcs	.L3396
	mov	r2, r3
.L3260:
	ldr	r3, [r2, #12]
	cmp	r3, #0
	bne	.L3261
	str	r10, [r2, #12]
	str	r3, [r10, #12]
	str	r2, [r10, #16]
.L3259:
	ldr	r10, [r10, #28]
	cmp	r10, #0
	bne	.L3262
.L3263:
	mov	r0, r4
	bl	adjust_invaild_list
	cmp	r7, #0
	beq	.L3258
	ldr	r3, [sp, #20]
	ldr	r2, [r9, #8]
	ldr	r1, [r3, #8]
	add	r3, r1, #1
	cmp	r3, r2
	beq	.L3258
	movw	r0, #:lower16:.LC357
	movt	r0, #:upper16:.LC357
	bl	NAND_Print
.L3258:
	cmp	r5, #0
	mov	r3, #0
	str	r3, [r4, #292]
	bne	.L3306
	cmp	r8, #0
	beq	.L3397
	ldrb	r3, [r4, #16]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L3398
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	bne	.L3399
.L3275:
	ldr	r2, [r9, #8]
	ldr	r3, [r8, #8]
	cmp	r2, r3
	beq	.L3400
	movw	r0, #:lower16:.LC363
	movt	r0, #:upper16:.LC363
	bl	NAND_Print
	ldr	r3, [sp, #20]
	movw	r0, #:lower16:.LC364
	movt	r0, #:upper16:.LC364
	ldr	r2, [r3, #8]
	ldrh	r1, [r3]
	bl	NAND_Print
	movw	r0, #:lower16:.LC365
	ldr	r2, [r9, #8]
	ldrh	r1, [r9]
	movt	r0, #:upper16:.LC365
	bl	NAND_Print
	movw	r0, #:lower16:.LC366
	ldr	r2, [r8, #8]
	ldrh	r1, [r8]
	movt	r0, #:upper16:.LC366
	bl	NAND_Print
	ldr	r2, [r4, #304]
	mov	r1, r9
	mov	r0, r4
	bl	recover_block_phy_page_mapping
	ldr	r3, [r4, #4]
	mov	r1, r9
	ldr	r2, [r4, #304]
	mov	r0, r4
	ldrh	r3, [r3, #20]
	sub	r3, r3, #1
	bl	recover_logic_page_mapping
	mov	r1, r9
	mov	r2, #3
	mov	r0, r4
	bl	add_prio_gc
	ldr	r2, [r4, #304]
	mov	r1, r8
	mov	r0, r4
	bl	recover_block_phy_page_mapping
	ldr	r3, [r4, #4]
	mov	r1, r8
	ldr	r2, [r4, #304]
	mov	r0, r4
	ldrh	r3, [r3, #20]
	sub	r3, r3, #1
	bl	recover_logic_page_mapping
	mov	r1, r8
	mov	r2, #3
	mov	r0, r4
	bl	add_prio_gc
.L3266:
	ldr	r3, [r4, #292]
	cmp	r3, #0
	beq	.L3278
.L3294:
	mov	r0, r4
	bl	adjust_invaild_list
	ldr	r3, [r4, #292]
	mov	r0, r4
	str	r3, [r4, #48]
	bl	adjust_free_list
	movw	r0, #:lower16:.LC381
	ldrh	r1, [r4, #22]
	movt	r0, #:upper16:.LC381
	bl	NAND_Print
	mov	r0, #0
	add	sp, sp, #84
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3392:
	movw	r0, #:lower16:.LC379
	ldrh	r1, [sp, #66]
	movt	r0, #:upper16:.LC379
	bl	NAND_Print
	cmp	r9, #0
	beq	.L3401
	ldr	r3, [sp, #32]
	cmp	r3, #0
	beq	.L3402
	movw	r0, #:lower16:.LC352
	ldr	r2, [r9, #8]
	ldrh	r1, [r9]
	movt	r0, #:upper16:.LC352
	bl	NAND_Print
	ldr	r6, [sp, #32]
	movw	r0, #:lower16:.LC353
	movt	r0, #:upper16:.LC353
	ldr	r2, [r6, #8]
	ldrh	r1, [r6]
	bl	NAND_Print
	movw	r0, #:lower16:.LC354
	mov	r2, r7
	ldrh	r1, [r5]
	movt	r0, #:upper16:.LC354
	bl	NAND_Print
	mov	r1, r9
	mov	r2, #3
	mov	r0, r4
	mov	r9, r6
	bl	add_prio_gc
	str	r5, [sp, #32]
	b	.L3246
.L3396:
	str	r10, [r3, #16]
	str	r3, [r10, #12]
	str	r10, [r2, #12]
	str	r2, [r10, #16]
	b	.L3259
.L3394:
	ldrh	r2, [r5, #6]
	mov	r1, r5
	mov	r0, r4
	str	r5, [sp, #44]
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	b	.L3246
.L3401:
	mov	r9, r5
	b	.L3246
.L3402:
	movw	r0, #:lower16:.LC350
	ldrh	r2, [r5]
	movt	r0, #:upper16:.LC350
	ldrh	r1, [r9]
	bl	NAND_Print
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	bne	.L3310
	ldr	r3, [r9, #8]
	ldr	ip, [r5, #8]
	cmp	r3, ip
	beq	.L3310
	movw	r0, #:lower16:.LC351
	ldrh	r2, [r5]
	ldrh	r1, [r9]
	movt	r0, #:upper16:.LC351
	str	ip, [sp]
	bl	NAND_Print
	ldr	r2, [r4, #304]
	mov	r1, r9
	mov	r0, r4
	bl	recover_phy_page_mapping
	ldr	r3, [r4, #4]
	mov	r1, r9
	ldr	r2, [r4, #304]
	mov	r0, r4
	ldrh	r3, [r3, #20]
	sub	r3, r3, #1
	bl	recover_logic_page_mapping
	mov	r1, r9
	mov	r2, #3
	mov	r0, r4
	mov	r9, r5
	bl	add_prio_gc
	b	.L3246
.L3310:
	str	r5, [sp, #32]
	b	.L3246
.L3410:
	mov	r0, r4
	str	r10, [sp, #20]
	bl	adjust_invaild_list
	mvn	r3, #0
	str	r10, [r4, #292]
	mov	fp, r3
.L3306:
	movw	r0, #:lower16:.LC358
	movt	r0, #:upper16:.LC358
	bl	NAND_Print_DBG
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	ldr	r3, [sp, #20]
	beq	.L3265
	cmp	r3, #0
	beq	.L3266
	mov	r1, r3
	mov	r2, #3
	mov	r0, r4
	bl	add_prio_gc
	b	.L3266
.L3399:
	mov	r1, r9
	mov	r2, r8
	mov	r0, r4
	bl	cross_talk_rebuild_current_block
	subs	r9, r0, #0
	beq	.L3383
.L3271:
	ldr	r3, [r4, #292]
	cmp	r3, #0
	bne	.L3294
.L3305:
	movw	r5, #:lower16:.LC367
	mov	r10, #0
	movt	r5, #:upper16:.LC367
	add	r8, r4, #320
	movw	r6, #65535
	b	.L3280
.L3404:
	ldrh	r1, [r9]
	mov	r10, #1
	bl	NAND_Print
	mov	r1, r9
	mov	r0, r4
	bl	current_block_ecc_error
	subs	r9, r0, #0
	beq	.L3403
.L3280:
	ldr	r2, [r4, #304]
	mov	r1, r9
	mov	r0, r4
	bl	recover_phy_page_mapping
	mov	r2, r0
	mov	r0, r5
	uxth	r3, r2
	cmp	r3, r6
	strh	r3, [r8]	@ movhi
	beq	.L3404
	movw	r0, #:lower16:.LC369
	uxth	r2, r2
	ldrh	r1, [r9]
	movt	r0, #:upper16:.LC369
	str	r9, [r4, #292]
	bl	NAND_Print_DBG
	ldrh	r3, [r8]
	mov	r0, r4
	ldr	r2, [r4, #304]
	ldr	r1, [r4, #292]
	bl	recover_logic_page_mapping
	ldr	r3, [r4, #292]
	ldrh	r2, [r8]
	ldrh	r3, [r3, #4]
	cmp	r2, r3
	bcc	.L3405
.L3282:
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	beq	.L3294
	ldr	r7, [r4, #324]
	movw	r3, #:lower16:.LC374
	movw	r2, #:lower16:.LC373
	movw	fp, #:lower16:.LC342
	movt	r3, #:upper16:.LC374
	movt	r2, #:upper16:.LC373
	cmp	r7, #0
	movt	fp, #:upper16:.LC342
	str	r3, [sp, #20]
	addne	r6, r4, #352
	str	r2, [sp, #16]
	beq	.L3375
.L3284:
	ldr	r2, [r4, #336]
	mov	r1, r7
	mov	r0, r4
	bl	recover_phy_page_mapping
	uxth	r2, r0
	movw	r3, #65535
	cmp	r2, r3
	strh	r2, [r6]	@ movhi
	ldreq	r0, [sp, #16]
	beq	.L3381
	ldr	r1, [r4, #292]
	uxth	r3, r0
	ldr	lr, [r4, #324]
	mov	r0, r4
	ldr	r2, [r4, #336]
	ldr	ip, [r1, #8]
	mov	r1, lr
	add	ip, ip, #1
	str	ip, [lr, #8]
	bl	recover_logic_page_mapping
	ldrh	r1, [r6]
	ldrh	r2, [r8]
	cmp	r1, r2
	bhi	.L3406
	bcs	.L3294
	mov	r0, fp
.L3381:
	bl	NAND_Print
	ldr	r1, [r4, #324]
	mov	r0, r4
	ldrh	r2, [r1, #6]
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	mov	r3, #0
	str	r3, [r4, #324]
.L3287:
	mov	r7, #0
.L3375:
	mov	r0, r4
	bl	out_phy_block_from_free_list
	cmp	r0, #0
	mov	r3, r0
	str	r0, [r4, #324]
	beq	.L3281
	ldr	r2, [r4, #292]
	add	r6, r4, #352
	strh	r7, [r6]	@ movhi
	mov	r1, r0
	mov	r0, r4
	ldr	r2, [r2, #8]
	add	r2, r2, #1
	str	r2, [r3, #8]
	bl	new_block_init_for_write
	cmp	r0, #0
	bne	.L3407
	ldrh	r9, [r8]
	cmp	r9, #0
	beq	.L3382
	movw	r7, #:lower16:.LC372
	mov	r5, r0
	movt	r7, #:upper16:.LC372
	b	.L3290
.L3289:
	cmp	r9, r5
	bls	.L3382
.L3290:
	uxth	ip, r5
	ldr	r3, [r4, #284]
	ldr	r2, [r4, #324]
	mov	r0, r4
	ldr	r1, [r4, #292]
	add	r5, r5, #1
	str	ip, [sp]
	ldr	ip, [r4, #1988]
	blx	ip
	cmp	r0, #0
	beq	.L3289
	mov	r0, r7
	bl	NAND_Print
	cmp	r9, r5
	bhi	.L3290
.L3382:
	ldr	r7, [r4, #324]
	b	.L3284
.L3398:
	movw	r0, #:lower16:.LC360
	movt	r0, #:upper16:.LC360
	bl	NAND_Print
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	beq	.L3275
	b	.L3399
.L3265:
	cmp	r3, #0
	beq	.L3266
	ldrh	r3, [r3, #2]
	cmp	r3, #170
	bne	.L3266
	ldr	r1, [sp, #20]
	mov	r2, #3
	mov	r0, r4
	bl	add_prio_gc
	b	.L3266
.L3406:
	ldr	r0, [sp, #20]
	bl	NAND_Print
	cmp	r10, #0
	beq	.L3293
	ldr	r1, [r4, #292]
	mov	r0, r4
	ldrh	r2, [r1, #6]
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	ldrh	r3, [r6]
	ldr	r0, [r4, #324]
	ldr	r2, [r4, #300]
	ldr	r1, [r4, #328]
	str	r0, [r4, #292]
	ldr	r0, [r4, #296]
	strh	r3, [r8]	@ movhi
	bl	NAND_Memcpy
	mov	r3, #0
	str	r3, [r4, #324]
	b	.L3287
.L3295:
	movw	r0, #:lower16:.LC377
	movt	r0, #:upper16:.LC377
	bl	NAND_Print
	ldr	r3, [r4, #356]
	cmp	r3, #0
	beq	.L3281
	mov	r0, r4
	bl	print_free_list.part.6
	ldr	r3, [r4, #356]
	cmp	r3, #0
	beq	.L3281
	mov	r0, r4
	bl	print_block_invalid_list.part.7
.L3281:
	movw	r0, #:lower16:.LC380
	movt	r0, #:upper16:.LC380
	bl	NAND_Print
.L3383:
	mov	r0, #1
	add	sp, sp, #84
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3400:
	movw	r0, #:lower16:.LC361
	ldrh	r1, [r9]
	movt	r0, #:upper16:.LC361
	bl	NAND_Print_DBG
	movw	r0, #:lower16:.LC362
	ldr	r2, [r8, #8]
	ldrh	r1, [r8]
	movt	r0, #:upper16:.LC362
	bl	NAND_Print_DBG
	mov	r1, r9
	mov	r2, r8
	mov	r0, r4
	bl	no_cross_talk_rebuild_current_block
	mov	r9, r0
.L3273:
	ldr	r3, [r4, #292]
	cmp	r3, #0
	bne	.L3294
.L3303:
	cmp	r9, #0
	bne	.L3305
.L3278:
	movw	r0, #:lower16:.LC376
	movt	r0, #:upper16:.LC376
	bl	NAND_Print_DBG
	ldr	r3, [r4, #172]
	cmp	r3, #0
	beq	.L3295
	mov	r0, r4
	bl	out_phy_block_from_free_list
	cmp	r0, #0
	str	r0, [r4, #292]
	beq	.L3281
	add	r3, r4, #320
	add	r2, fp, #1
	mov	r5, #0
	strh	r5, [r3]	@ movhi
	str	r2, [r0, #8]
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	ands	r3, r3, #1
	streq	r3, [r4, #324]
	beq	.L3294
	mov	r0, r4
	bl	out_phy_block_from_free_list
	cmp	r0, r5
	str	r0, [r4, #324]
	beq	.L3281
	ldr	r3, [r4, #292]
	add	r2, r4, #352
	strh	r5, [r2]	@ movhi
	ldr	r3, [r3, #8]
	add	r3, r3, #1
	str	r3, [r0, #8]
	b	.L3294
.L3407:
	movw	r0, #:lower16:.LC371
	movt	r0, #:upper16:.LC371
	bl	NAND_Print
	str	r7, [r4, #324]
	b	.L3287
.L3397:
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	bne	.L3408
	ldr	r3, [sp, #20]
	cmp	r3, #0
	beq	.L3272
	ldrh	r3, [r3, #2]
	cmp	r3, #170
	beq	.L3409
.L3272:
	ldrb	r3, [r4, #16]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L3303
.L3304:
	mov	r1, r9
	mov	r2, #0
	mov	r0, r4
	bl	no_cross_talk_rebuild_current_block
	mov	r9, r0
	b	.L3273
.L3237:
	mov	r1, r5
	mov	r0, r4
	bl	init_smart_info
	ldr	r10, [r4, #252]
	cmp	r10, #0
	beq	.L3410
	mov	r8, r5
	mvn	r3, #0
	mov	r5, #1
	mov	r9, r8
	str	r8, [sp, #20]
	mov	r6, r8
	mov	r7, r8
	mov	fp, r3
	b	.L3256
.L3403:
	movw	r0, #:lower16:.LC368
	movt	r0, #:upper16:.LC368
	bl	NAND_Print
	movw	r0, #:lower16:.LC380
	movt	r0, #:upper16:.LC380
	bl	NAND_Print
	b	.L3383
.L3408:
	movw	r0, #:lower16:.LC359
	ldr	r2, [r9, #8]
	movt	r0, #:upper16:.LC359
	ldrh	r1, [r9]
	bl	NAND_Print_DBG
	ldr	r3, [sp, #20]
	cmp	r3, #0
	beq	.L3269
	ldrh	r3, [r3, #2]
	cmp	r3, #170
	beq	.L3411
.L3269:
	mov	r1, r9
	mov	r2, #0
	mov	r0, r4
	bl	cross_talk_rebuild_current_block
	subs	r9, r0, #0
	bne	.L3271
	b	.L3383
.L3405:
	movw	r0, #:lower16:.LC370
	movt	r0, #:upper16:.LC370
	bl	NAND_Print
	b	.L3282
.L3409:
	ldr	r1, [sp, #20]
	mov	r2, #3
	mov	r0, r4
	bl	add_prio_gc
	ldrb	r3, [r4, #16]	@ zero_extendqisi2
	cmp	r3, #0
	beq	.L3273
	b	.L3304
.L3293:
	movw	r0, #:lower16:.LC375
	ldrh	r2, [r8]
	ldrh	r1, [r6]
	movt	r0, #:upper16:.LC375
	bl	NAND_Print
	b	.L3294
.L3411:
	ldr	r1, [sp, #20]
	mov	r2, #3
	mov	r0, r4
	bl	add_prio_gc
	b	.L3269
	.size	second_scan_all_blocks, .-second_scan_all_blocks
	.align	2
	.global	do_before_write
	.syntax unified
	.arm
	.type	do_before_write, %function
do_before_write:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r4, r0
	add	r5, r0, #292
.L3413:
	mov	r0, r4
	ldrh	r3, [r5, #28]
	cmp	r3, #0
	beq	.L3416
.L3422:
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	addne	r5, r4, #324
	bne	.L3417
.L3420:
	mov	r0, #0
	pop	{r4, r5, r6, pc}
.L3416:
	ldr	r1, [r4, #292]
	ldr	r2, [r1, #8]
	bl	new_block_init_for_write
	cmp	r0, #0
	mov	r1, r5
	mov	r0, r4
	beq	.L3422
	ldr	r2, [r4, #292]
	ldr	r3, [r2, #8]
	sub	ip, r3, #1
	sub	r3, r3, #2
	str	ip, [r2, #8]
	ldrb	ip, [r4, #17]	@ zero_extendqisi2
	tst	ip, #1
	strne	r3, [r2, #8]
	bl	get_new_current_block
	cmp	r0, #0
	beq	.L3413
	movw	r0, #:lower16:.LC382
	movt	r0, #:upper16:.LC382
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, pc}
.L3419:
	ldr	r1, [r4, #324]
	ldr	r2, [r1, #8]
	bl	new_block_init_for_write
	cmp	r0, #0
	mov	r1, r5
	mov	r0, r4
	beq	.L3420
	ldr	r2, [r4, #324]
	ldr	r3, [r2, #8]
	sub	ip, r3, #1
	sub	r3, r3, #2
	str	ip, [r2, #8]
	ldrb	ip, [r4, #17]	@ zero_extendqisi2
	tst	ip, #1
	strne	r3, [r2, #8]
	bl	get_new_current_block
	cmp	r0, #0
	bne	.L3437
.L3417:
	ldrh	r3, [r5, #28]
	mov	r0, r4
	cmp	r3, #0
	beq	.L3419
	mov	r0, #0
	pop	{r4, r5, r6, pc}
.L3437:
	movw	r0, #:lower16:.LC383
	movt	r0, #:upper16:.LC383
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, pc}
	.size	do_before_write, .-do_before_write
	.align	2
	.global	check_invalid_page_right
	.syntax unified
	.arm
	.type	check_invalid_page_right, %function
check_invalid_page_right:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r3, r4, r5, r6, r7, r8, r9, r10, fp, lr}
	subs	r6, r3, #0
	mov	r8, r2
	beq	.L3446
	mov	r5, #0
	movw	r9, #:lower16:.LC384
	mov	r7, r0
	mov	r10, r1
	mov	r4, r5
	movt	r9, #:upper16:.LC384
	b	.L3444
.L3440:
	add	r4, r4, #1
	add	r5, r5, #1
	cmp	r4, r6
	bcs	.L3439
.L3444:
	ldr	r2, [r10, #8]
	mov	r0, r7
	ldr	r2, [r2, r4, lsl #2]
	cmn	r2, #1
	mov	r1, r2
	beq	.L3440
	bl	get_logic_page_map
	subs	fp, r0, #0
	beq	.L3451
.L3441:
	ldrh	r2, [fp, #2]
	ldrh	r3, [r8]
	cmp	r2, r3
	bne	.L3440
	ldrh	r3, [fp]
	ubfx	r3, r3, #0, #12
	cmp	r3, r4
	bne	.L3440
	add	r4, r4, #1
	cmp	r4, r6
	bcc	.L3444
.L3439:
	ldrh	r2, [r8, #4]
	cmp	r2, r5
	beq	.L3447
	movw	r0, #:lower16:.LC385
	mov	r3, r6
	mov	r1, r5
	movt	r0, #:upper16:.LC385
	bl	NAND_Print
	mov	r0, #1
	pop	{r3, r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3447:
	mov	r0, #0
	pop	{r3, r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3451:
	mov	r0, r9
	bl	NAND_Print
	b	.L3441
.L3446:
	mov	r5, r6
	b	.L3439
	.size	check_invalid_page_right, .-check_invalid_page_right
	.align	2
	.global	write_phy_page_map_to_current_block
	.syntax unified
	.arm
	.type	write_phy_page_map_to_current_block, %function
write_phy_page_map_to_current_block:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	add	r3, r0, #324
	push	{r4, r5, r6, lr}
	cmp	r1, r3
	sub	sp, sp, #8
	mov	r5, r0
	mov	r4, r1
	beq	.L3462
.L3453:
	movw	r1, #65535
	ldr	r2, [r4, #4]
	movt	r1, 43690
	mov	r3, #0
	str	r1, [sp]
	mov	r0, r5
	mov	r1, r4
	bl	write_phy_data_to_current_block
	subs	r6, r0, #0
	bne	.L3463
	mov	r1, r4
	ldrh	r3, [r4, #28]
	ldr	r2, [r1], #4
	mov	r0, r5
	add	r3, r3, #1
	ldrh	ip, [r2, #4]
	uxth	r3, r3
	strh	r3, [r4, #28]	@ movhi
	add	ip, ip, #1
	strh	ip, [r2, #4]	@ movhi
	bl	check_invalid_page_right
	ldr	r3, [r5, #4]
	ldrh	r2, [r4, #28]
	ldrh	r3, [r3, #20]
	cmp	r2, r3
	movne	r0, r6
	beq	.L3464
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, pc}
.L3464:
	ldr	ip, [r4]
	add	r2, r5, #188
	str	r6, [ip, #12]
	str	r6, [ip, #16]
	b	.L3456
.L3458:
	ldrh	r1, [r3, #4]
	ldrh	r0, [ip, #4]
	cmp	r0, r1
	bcs	.L3465
	mov	r2, r3
.L3456:
	ldr	r3, [r2, #12]
	cmp	r3, #0
	bne	.L3458
	str	ip, [r2, #12]
	str	r3, [ip, #12]
	str	r2, [ip, #16]
.L3457:
	mov	r0, r5
	bl	adjust_invaild_list
	mov	r0, #0
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, pc}
.L3463:
	movw	r0, #:lower16:.LC386
	movt	r0, #:upper16:.LC386
	bl	NAND_Print
	mov	r0, r6
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, pc}
.L3465:
	str	ip, [r3, #16]
	str	r3, [ip, #12]
	str	ip, [r2, #12]
	str	r2, [ip, #16]
	b	.L3457
.L3462:
	ldr	r1, [r0, #312]
	mov	r2, #328
	ldr	r0, [r0, #344]
	bl	NAND_Memcpy
	b	.L3453
	.size	write_phy_page_map_to_current_block, .-write_phy_page_map_to_current_block
	.align	2
	.syntax unified
	.arm
	.type	do_after_write.part.21, %function
do_after_write.part.21:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	add	r5, r0, #292
	mov	r1, r5
	mov	r4, r0
	bl	write_phy_page_map_to_current_block
	cmp	r0, #0
	bne	.L3484
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	bne	.L3485
.L3470:
	mov	r1, r5
	mov	r0, r4
	bl	get_new_current_block
	subs	r5, r0, #0
	bne	.L3486
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	bne	.L3487
.L3473:
	ldr	r3, [r4, #368]
	add	r3, r3, #1
	str	r3, [r4, #368]
.L3468:
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L3485:
	add	r1, r4, #324
	mov	r0, r4
	bl	write_phy_page_map_to_current_block
	cmp	r0, #0
	beq	.L3470
	movw	r0, #:lower16:.LC387
	movt	r0, #:upper16:.LC387
	bl	NAND_Print
	b	.L3470
.L3484:
	movw	r0, #:lower16:.LC387
	mov	r5, #1
	movt	r0, #:upper16:.LC387
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, pc}
.L3487:
	add	r1, r4, #324
	mov	r0, r4
	bl	get_new_current_block
	cmp	r0, #0
	beq	.L3473
	movw	r0, #:lower16:.LC389
	mov	r5, #1
	movt	r0, #:upper16:.LC389
	bl	NAND_Print
	b	.L3468
.L3486:
	movw	r0, #:lower16:.LC388
	mov	r5, #1
	movt	r0, #:upper16:.LC388
	bl	NAND_Print
	mov	r0, r5
	pop	{r4, r5, r6, pc}
	.size	do_after_write.part.21, .-do_after_write.part.21
	.align	2
	.global	do_after_write
	.syntax unified
	.arm
	.type	do_after_write, %function
do_after_write:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r1, [r0, #4]
	add	r3, r0, #292
	ldrh	r2, [r3, #28]
	ldrh	r3, [r1, #20]
	sub	r3, r3, #1
	cmp	r2, r3
	beq	.L3491
	mov	r0, #0
	bx	lr
.L3491:
	b	do_after_write.part.21
	.size	do_after_write, .-do_after_write
	.align	2
	.global	nand_write_data_page
	.syntax unified
	.arm
	.type	nand_write_data_page, %function
nand_write_data_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	ip, [r0, #4]
	push	{r4, r5, r6, r7, r8, lr}
	add	r5, r0, #292
	ldrh	lr, [r5, #28]
	mov	r4, r0
	mov	r6, r1
	mov	r7, r2
	ldrh	ip, [ip, #20]
	mov	r8, r3
	sub	ip, ip, #1
	cmp	lr, ip
	beq	.L3506
.L3494:
	mov	r0, r4
	bl	do_before_write
	cmp	r0, #0
	bne	.L3507
	mov	r1, r6
	mov	r3, r8
	mov	r2, r7
	mov	r0, r4
	bl	write_data_to_page
	subs	r6, r0, #0
	bne	.L3508
	ldr	r3, [r4, #4]
	ldrh	r2, [r5, #28]
	ldrh	r3, [r3, #20]
	sub	r3, r3, #1
	cmp	r2, r3
	beq	.L3509
.L3502:
	mov	r0, r6
	pop	{r4, r5, r6, r7, r8, pc}
.L3507:
	movw	r0, #:lower16:.LC391
	mov	r6, #1
	movt	r0, #:upper16:.LC391
	bl	NAND_Print
	mov	r0, r6
	pop	{r4, r5, r6, r7, r8, pc}
.L3508:
	movw	r0, #:lower16:.LC392
	mov	r6, #1
	movt	r0, #:upper16:.LC392
	bl	NAND_Print
	mov	r0, r6
	pop	{r4, r5, r6, r7, r8, pc}
.L3506:
	bl	do_after_write.part.21
	cmp	r0, #0
	beq	.L3494
	movw	r0, #:lower16:.LC390
	mov	r6, #1
	movt	r0, #:upper16:.LC390
	bl	NAND_Print
	mov	r0, r6
	pop	{r4, r5, r6, r7, r8, pc}
.L3509:
	mov	r0, r4
	bl	do_after_write.part.21
	cmp	r0, #0
	beq	.L3502
	movw	r0, #:lower16:.LC393
	mov	r6, #1
	movt	r0, #:upper16:.LC393
	bl	NAND_Print
	mov	r0, r6
	pop	{r4, r5, r6, r7, r8, pc}
	.size	nand_write_data_page, .-nand_write_data_page
	.align	2
	.global	nand_write_logic_page_no_gc
	.syntax unified
	.arm
	.type	nand_write_logic_page_no_gc, %function
nand_write_logic_page_no_gc:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r3, r2
	mov	r2, r1
	mov	r1, #192
	b	nand_write_data_page
	.size	nand_write_logic_page_no_gc, .-nand_write_logic_page_no_gc
	.align	2
	.global	garbage_collect_first
	.syntax unified
	.arm
	.type	garbage_collect_first, %function
garbage_collect_first:
	@ args = 0, pretend = 0, frame = 48
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	sub	sp, sp, #60
	mov	r4, r1
	str	r2, [sp, #12]
	mov	r5, r0
	mov	r2, #16
	add	r0, sp, #24
	mov	r1, #90
	bl	NAND_Memset
	cmp	r4, #0
	beq	.L3582
	ldrb	r3, [r5, #17]	@ zero_extendqisi2
	tst	r3, #1
	bne	.L3583
.L3515:
	ldr	r3, [r5, #292]
	cmp	r4, r3
	bne	.L3584
.L3538:
	mov	r0, #0
.L3570:
	add	sp, sp, #60
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3584:
	mov	r1, r4
	mov	r0, r5
	bl	out_phy_block_from_invalid_page_list_by_block
	subs	r7, r0, #0
	beq	.L3513
.L3533:
	ldr	ip, [r5, #1968]
	add	r1, sp, #40
	ldr	lr, [r5, #4]
	mov	r0, r5
	ldr	r2, [ip, #124]
	ldrh	r3, [lr, #20]
	ldrh	lr, [lr, #28]
	add	r2, r2, #1
	str	r2, [ip, #124]
	sub	r3, r3, #1
	ldrh	r2, [r7]
	strh	r3, [sp, #40]	@ movhi
	ldr	r3, [r5, #284]
	strh	r2, [sp, #42]	@ movhi
	add	r2, sp, #24
	strh	lr, [sp, #44]	@ movhi
	str	r2, [sp, #52]
	str	r3, [sp, #48]
	ldr	r3, [r5, #1980]
	blx	r3
	ldrb	lr, [sp, #25]	@ zero_extendqisi2
	movw	ip, #65535
	ldrb	r1, [sp, #26]	@ zero_extendqisi2
	movt	ip, 43690
	ldrb	r2, [sp, #27]	@ zero_extendqisi2
	ldrb	r3, [sp, #28]	@ zero_extendqisi2
	orr	r1, r1, lr, lsl #8
	orr	r2, r2, r1, lsl #8
	orr	r3, r3, r2, lsl #8
	cmp	r3, ip
	beq	.L3585
	movw	r0, #:lower16:.LC402
	movt	r0, #:upper16:.LC402
	bl	NAND_Print
	ldr	ip, [r5, #4]
	movw	r0, #:lower16:.LC403
	ldrh	r1, [r7]
	mov	r3, #0
	movt	r0, #:upper16:.LC403
	ldrh	r2, [ip, #20]
	ldrh	ip, [ip, #28]
	sub	r2, r2, #1
	str	ip, [sp]
	bl	NAND_Print
	ldrb	ip, [sp, #28]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC404
	ldrb	r3, [sp, #27]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC404
	ldrb	r2, [sp, #26]	@ zero_extendqisi2
	ldrb	r1, [sp, #25]	@ zero_extendqisi2
	str	ip, [sp]
	bl	NAND_Print
	ldrb	lr, [sp, #33]	@ zero_extendqisi2
	movw	r0, #:lower16:.LC405
	ldrb	ip, [sp, #32]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC405
	ldrb	r3, [sp, #31]	@ zero_extendqisi2
	ldrb	r2, [sp, #30]	@ zero_extendqisi2
	ldrb	r1, [sp, #29]	@ zero_extendqisi2
	stm	sp, {ip, lr}
	bl	NAND_Print
	movw	r0, #:lower16:.LC406
	ldrh	r2, [r7]
	ldrb	r1, [r5, #16]	@ zero_extendqisi2
	movt	r0, #:upper16:.LC406
	bl	NAND_Print
	ldr	r2, [r5, #268]
	mov	r1, r7
	mov	r0, r5
	bl	recover_block_phy_page_mapping
.L3520:
	ldr	r3, [r5, #4]
	mov	r2, r7
	add	r1, r5, #260
	mov	r0, r5
	ldrh	r3, [r3, #20]
	bl	check_invalid_page_right
	cmp	r0, #0
	beq	.L3586
.L3536:
	mov	r3, #1
	movw	r0, #:lower16:.LC396
	str	r3, [sp, #16]
	movt	r0, #:upper16:.LC396
	movw	r3, #65535
	str	r3, [sp, #20]
	bl	NAND_Print
.L3521:
	ldr	r1, [r5, #4]
	ldrh	r3, [r1, #20]
	cmp	r3, #1
	beq	.L3522
	movw	r10, #:lower16:.LC397
	movw	r9, #:lower16:.LC398
	movw	r8, #:lower16:.LC399
	movt	r10, #:upper16:.LC397
	movt	r9, #:upper16:.LC398
	movt	r8, #:upper16:.LC399
	mov	r4, #0
	b	.L3527
.L3578:
	ldr	r1, [r5, #4]
.L3523:
	ldrh	r3, [r1, #20]
	add	r4, r4, #1
	sub	r2, r3, #1
	cmp	r2, r4
	bls	.L3526
.L3527:
	ldr	r3, [r5, #268]
	ldr	r6, [r3, r4, lsl #2]
	cmn	r6, #1
	beq	.L3523
	mov	r1, r6
	mov	r0, r5
	bl	get_logic_page_map
	ldrh	r3, [r7]
	mov	fp, r0
	ldrh	r2, [r0, #2]
	cmp	r3, r2
	bne	.L3578
	ldrh	r2, [r0]
	ubfx	r2, r2, #0, #12
	cmp	r2, r4
	bne	.L3578
	ldr	r2, [r5, #1968]
	add	r1, sp, #40
	ldr	r0, [r5, #4]
	ldr	ip, [r5, #288]
	ldr	lr, [r2, #128]
	ldrh	r0, [r0, #28]
	strh	r3, [sp, #42]	@ movhi
	add	r3, sp, #24
	str	r3, [sp, #52]
	add	r3, lr, #1
	str	r3, [r2, #128]
	strh	r0, [sp, #44]	@ movhi
	mov	r0, r5
	ldr	r3, [r5, #1980]
	strh	r4, [sp, #40]	@ movhi
	str	ip, [sp, #48]
	blx	r3
	ldrb	r0, [sp, #25]	@ zero_extendqisi2
	ldrb	r1, [sp, #26]	@ zero_extendqisi2
	ldrb	r2, [sp, #27]	@ zero_extendqisi2
	ldrb	r3, [sp, #28]	@ zero_extendqisi2
	orr	r1, r1, r0, lsl #8
	orr	r2, r2, r1, lsl #8
	orr	r3, r3, r2, lsl #8
	bic	r3, r3, #-268435456
	cmp	r6, r3
	beq	.L3524
	ldrh	r1, [r7]
	mov	ip, #1
	mov	r2, r4
	str	r6, [sp]
	mov	r0, r10
	str	ip, [sp, #16]
	bl	NAND_Print
	ldrb	ip, [sp, #28]	@ zero_extendqisi2
	ldrb	r0, [sp, #27]	@ zero_extendqisi2
	ldrb	r3, [sp, #26]	@ zero_extendqisi2
	ldrb	r2, [sp, #25]	@ zero_extendqisi2
	stm	sp, {r0, ip}
	mov	r0, r9
	ldrb	r1, [sp, #24]	@ zero_extendqisi2
	bl	NAND_Print
	ldrb	r0, [sp, #32]	@ zero_extendqisi2
	ldrb	ip, [sp, #33]	@ zero_extendqisi2
	ldrb	r3, [sp, #31]	@ zero_extendqisi2
	ldrb	r2, [sp, #30]	@ zero_extendqisi2
	ldrb	r1, [sp, #29]	@ zero_extendqisi2
	stm	sp, {r0, ip}
	mov	r0, r8
	bl	NAND_Print
	ldrh	r3, [fp]
	movw	r2, #4095
	mvn	r1, #0
	movw	r0, #65534
	strh	r1, [fp, #2]	@ movhi
	bfi	r3, r2, #0, #12
	ldr	r1, [r5, #4]
	str	r0, [sp, #20]
	strh	r3, [fp]	@ movhi
	b	.L3523
.L3583:
	ldr	r3, [r5, #324]
	cmp	r4, r3
	bne	.L3515
	b	.L3538
.L3513:
	movw	r0, #:lower16:.LC394
	movt	r0, #:upper16:.LC394
	bl	NAND_Print
	mov	r0, #8
	add	sp, sp, #60
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3585:
	cmp	r0, #0
	cmpne	r0, #10
	bne	.L3534
	ldr	r2, [r5, #272]
	ldr	r1, [r5, #284]
	ldr	r0, [r5, #268]
	bl	NAND_Memcpy
	ldr	r3, [r5, #4]
	mov	r2, r7
	add	r1, r5, #260
	mov	r0, r5
	ldrh	r3, [r3, #20]
	bl	check_invalid_page_right
	cmp	r0, #0
	bne	.L3536
	ldr	r3, [sp, #12]
	str	r0, [sp, #16]
	str	r3, [sp, #20]
	b	.L3521
.L3524:
	mov	r2, r6
	ldr	r3, [r5, #288]
	mov	r1, #192
	mov	r0, r5
	bl	nand_write_data_page
	cmp	r0, #0
	bne	.L3587
	ldr	r3, [sp, #20]
	sub	r3, r3, #1
	uxth	r3, r3
	cmp	r3, #0
	str	r3, [sp, #20]
	bne	.L3578
	ldr	r3, [r5, #4]
	ldrh	r3, [r3, #20]
	sub	r2, r3, #1
.L3526:
	cmp	r4, r2
	beq	.L3522
.L3528:
	ldrh	r0, [r7, #4]
	ldr	r2, [sp, #12]
	cmp	r0, r3
	cmpne	r2, #0
	bne	.L3529
	ldr	r3, [sp, #16]
	cmp	r3, #0
	bne	.L3588
.L3529:
	mov	r3, #0
	add	r2, r5, #188
	str	r3, [r7, #12]
	str	r3, [r7, #16]
	b	.L3530
.L3532:
	ldrh	r1, [r3, #4]
	cmp	r0, r1
	bcs	.L3589
	mov	r2, r3
.L3530:
	ldr	r3, [r2, #12]
	cmp	r3, #0
	bne	.L3532
	str	r7, [r2, #12]
	str	r3, [r7, #12]
	str	r2, [r7, #16]
.L3531:
	mov	r0, r5
	bl	adjust_invaild_list
	mov	r0, #0
	add	sp, sp, #60
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3534:
	movw	r0, #:lower16:.LC395
	ldrh	r1, [r7]
	movt	r0, #:upper16:.LC395
	bl	NAND_Print
	ldr	r2, [r5, #268]
	mov	r1, r7
	mov	r0, r5
	bl	recover_block_phy_page_mapping
	b	.L3520
.L3589:
	str	r7, [r3, #16]
	str	r3, [r7, #12]
	str	r7, [r2, #12]
	str	r2, [r7, #16]
	b	.L3531
.L3582:
	ldr	r7, [r5, #200]
	cmp	r7, #0
	beq	.L3513
	ldr	r3, [r7, #12]
	str	r3, [r5, #200]
	ldr	r3, [r7, #12]
	cmp	r3, #0
	addne	r2, r5, #188
	strne	r2, [r3, #16]
.L3514:
	mov	r3, #0
	str	r3, [r7, #12]
	str	r3, [r7, #16]
	b	.L3533
.L3586:
	mov	r3, #1
	str	r3, [sp, #16]
	movw	r3, #65535
	str	r3, [sp, #20]
	b	.L3521
.L3522:
	movw	r2, #386
	mov	r1, #0
	strh	r1, [r5, r2]	@ movhi
	b	.L3528
.L3588:
	movw	r0, #:lower16:.LC401
	movt	r0, #:upper16:.LC401
	bl	NAND_Print
	ldrh	r2, [r7, #6]
	mov	r0, r5
	mov	r1, r7
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	mov	r0, #0
	add	sp, sp, #60
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3587:
	movw	r0, #:lower16:.LC400
	movt	r0, #:upper16:.LC400
	bl	NAND_Print
	mov	r0, #1
	b	.L3570
	.size	garbage_collect_first, .-garbage_collect_first
	.align	2
	.global	do_prio_gc
	.syntax unified
	.arm
	.type	do_prio_gc, %function
do_prio_gc:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	sub	sp, sp, #8
	ldr	r5, [r0, #564]
	cmp	r5, #0
	beq	.L3618
	ldr	r3, [r5, #8]
	add	ip, r0, #392
	ldr	r2, [r5, #12]
	ldrh	lr, [ip]
	ldr	r1, [r5, #4]
	str	r3, [r2, #8]
	ldr	r3, [r5, #8]
	cmp	r3, #0
	strne	r2, [r3, #12]
	mov	r3, #0
	str	r3, [r5, #8]
	str	r3, [r5, #12]
	sub	r3, lr, #1
	strh	r3, [ip]	@ movhi
	ldrb	r3, [r0, #17]	@ zero_extendqisi2
	tst	r3, #1
	bne	.L3641
.L3593:
	ldr	r3, [r0, #292]
	cmp	r1, r3
	beq	.L3594
	cmp	r1, #0
	mov	r4, r0
	beq	.L3642
	ldr	r3, [r4, #200]
	cmp	r3, #0
	bne	.L3601
	b	.L3597
.L3643:
	ldr	r3, [r3, #12]
	cmp	r3, #0
	beq	.L3597
.L3601:
	cmp	r1, r3
	bne	.L3643
	ldrh	r3, [r5, #2]
	movw	r6, #394
	cmp	r3, #5
	strh	r3, [r4, r6]	@ movhi
	beq	.L3644
	movw	r2, #65535
	mov	r0, r4
	bl	garbage_collect_first
	cmp	r0, #0
	bne	.L3645
	ldr	r3, [r4, #172]
	ldr	r1, [r5, #4]
	cmp	r3, #0
	bne	.L3607
	b	.L3603
.L3646:
	ldr	r3, [r3, #20]
	cmp	r3, #0
	beq	.L3603
.L3607:
	cmp	r1, r3
	bne	.L3646
	mov	r0, r4
	bl	out_phy_block_from_free_list_by_block
	subs	r6, r0, #0
	beq	.L3647
	ldrh	r2, [r6, #6]
	mov	r1, r6
	mov	r0, r4
	add	r2, r2, #1
	uxth	r2, r2
	bl	erase_block
	b	.L3637
.L3641:
	ldr	r3, [r0, #324]
	cmp	r1, r3
	bne	.L3593
.L3594:
	add	r3, r0, #556
	b	.L3596
.L3619:
	mov	r3, r2
.L3596:
	ldr	r2, [r3, #8]
	cmp	r2, #0
	bne	.L3619
	mov	r0, r2
	str	r5, [r3, #8]
	str	r2, [r5, #8]
	str	r3, [r5, #12]
	strh	lr, [ip]	@ movhi
.L3629:
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, pc}
.L3597:
	ldrh	r1, [r1]
	movw	r0, #:lower16:.LC410
	movt	r0, #:upper16:.LC410
.L3636:
	ldrh	r2, [r5, #2]
	bl	NAND_Print
.L3637:
	mov	r3, #0
	movw	r2, #394
	strh	r3, [r5, #2]	@ movhi
	mov	r0, r3
	strh	r3, [r4, r2]	@ movhi
	str	r3, [r5, #4]
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, pc}
.L3603:
	movw	r0, #:lower16:.LC411
	ldrh	r1, [r1]
	movt	r0, #:upper16:.LC411
	b	.L3636
.L3618:
	mov	r0, r5
	add	sp, sp, #8
	@ sp needed
	pop	{r4, r5, r6, pc}
.L3644:
	mov	r2, #4
	mov	r0, r4
	bl	garbage_collect_first
	mov	r3, #0
	strh	r3, [r5, #2]	@ movhi
	mov	r0, r3
	strh	r3, [r4, r6]	@ movhi
	str	r3, [r5, #4]
	b	.L3629
.L3645:
	ldr	r3, [r5, #4]
	movw	r0, #:lower16:.LC408
	movt	r0, #:upper16:.LC408
	ldrh	r2, [r5, #2]
	ldrh	r1, [r3]
	bl	NAND_Print
	mov	r3, #0
	mov	r0, #1
	strh	r3, [r5, #2]	@ movhi
	strh	r3, [r4, r6]	@ movhi
	str	r3, [r5, #4]
	b	.L3629
.L3642:
	movw	r0, #:lower16:.LC407
	str	r1, [sp, #4]
	movt	r0, #:upper16:.LC407
	bl	NAND_Print
	ldr	r1, [sp, #4]
	movw	r3, #394
	strh	r1, [r5, #2]	@ movhi
	mov	r0, r1
	strh	r1, [r4, r3]	@ movhi
	str	r1, [r5, #4]
	b	.L3629
.L3647:
	ldr	r3, [r5, #4]
	movw	r0, #:lower16:.LC409
	movt	r0, #:upper16:.LC409
	ldrh	r2, [r5, #2]
	ldrh	r1, [r3]
	bl	NAND_Print
	movw	r3, #394
	strh	r6, [r5, #2]	@ movhi
	mov	r0, r6
	strh	r6, [r4, r3]	@ movhi
	str	r6, [r5, #4]
	b	.L3629
	.size	do_prio_gc, .-do_prio_gc
	.align	2
	.global	prio_gc_all
	.syntax unified
	.arm
	.type	prio_gc_all, %function
prio_gc_all:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r6, r0
	mov	r4, #10
	mov	r5, #0
.L3649:
	mov	r0, r6
	bl	do_prio_gc
	subs	r4, r4, #1
	orr	r5, r5, r0
	bne	.L3649
	mov	r0, r5
	pop	{r4, r5, r6, pc}
	.size	prio_gc_all, .-prio_gc_all
	.align	2
	.global	_nftl_nand_read_logic_page
	.syntax unified
	.arm
	.type	_nftl_nand_read_logic_page, %function
_nftl_nand_read_logic_page:
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, lr}
	sub	sp, sp, #36
	mov	r6, r2
	mov	r4, r0
	mov	r7, r1
	bl	get_logic_page_map
	subs	r5, r0, #0
	beq	.L3658
	ldrh	r1, [r5, #2]
	movw	r3, #65535
	cmp	r1, r3
	beq	.L3659
	ldrb	r3, [r5, #1]	@ zero_extendqisi2
	ldr	r0, [r4, #364]
	ldr	r2, [r4, #4]
	bfi	r3, r0, #4, #1
	mov	r0, r4
	strb	r3, [r5, #1]
	ldrh	r3, [r5]
	ldrh	r2, [r2, #28]
	strh	r1, [sp, #18]	@ movhi
	add	r1, sp, #16
	ubfx	r3, r3, #0, #12
	str	r6, [sp, #24]
	strh	r3, [sp, #16]	@ movhi
	strh	r2, [sp, #20]	@ movhi
	str	sp, [sp, #28]
	ldr	r3, [r4, #1980]
	blx	r3
	cmn	r0, #2
	mov	r6, r0
	beq	.L3660
.L3656:
	mov	r0, r4
	bl	do_prio_gc
	mov	r0, r6
.L3654:
	add	sp, sp, #36
	@ sp needed
	pop	{r4, r5, r6, r7, pc}
.L3659:
	ldr	r3, [r4, #4]
	mov	r0, r6
	mov	r1, #255
	ldrh	r2, [r3, #26]
	bl	NAND_Memset
	mov	r0, #0
	add	sp, sp, #36
	@ sp needed
	pop	{r4, r5, r6, r7, pc}
.L3660:
	ldr	r1, [r4, #4]
	mov	r2, #1
	ldrh	r3, [r5, #2]
	mov	r0, r4
	ldr	r1, [r1, #32]
	add	r3, r3, r3, lsl #3
	add	r1, r1, r3, lsl #2
	bl	add_prio_gc
	movw	r0, #:lower16:.LC412
	mov	r1, r7
	movt	r0, #:upper16:.LC412
	bl	NAND_Print
	b	.L3656
.L3658:
	movw	r0, #:lower16:.LC384
	movt	r0, #:upper16:.LC384
	bl	NAND_Print_DBG
	mov	r0, #1
	b	.L3654
	.size	_nftl_nand_read_logic_page, .-_nftl_nand_read_logic_page
	.align	2
	.syntax unified
	.arm
	.type	gc_one.part.24, %function
gc_one.part.24:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	movw	r2, #65535
	mov	r1, #0
	bl	garbage_collect_first
	cmp	r0, #8
	mov	r4, r0
	beq	.L3664
	cmp	r0, #0
	beq	.L3666
	movw	r0, #:lower16:.LC414
	movt	r0, #:upper16:.LC414
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, pc}
.L3666:
	movw	r0, #:lower16:.LC413
	movt	r0, #:upper16:.LC413
	bl	NAND_Print
	mov	r0, r4
	pop	{r4, pc}
.L3664:
	pop	{r4, pc}
	.size	gc_one.part.24, .-gc_one.part.24
	.align	2
	.global	gc_one
	.syntax unified
	.arm
	.type	gc_one, %function
gc_one:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #200]
	cmp	r3, #0
	beq	.L3668
	ldrh	r3, [r3, #4]
	cmp	r3, r1
	bhi	.L3672
.L3668:
	mov	r0, #8
	bx	lr
.L3672:
	b	gc_one.part.24
	.size	gc_one, .-gc_one
	.align	2
	.global	gc_all
	.syntax unified
	.arm
	.type	gc_all, %function
gc_all:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r4, r0
	mov	r5, r1
.L3676:
	ldr	r3, [r4, #200]
	cmp	r3, #0
	beq	.L3675
	ldrh	r3, [r3, #4]
	cmp	r3, r5
	bhi	.L3682
.L3675:
	movw	r0, #:lower16:.LC415
	movt	r0, #:upper16:.LC415
	bl	NAND_Print
	mov	r0, #0
	pop	{r4, r5, r6, pc}
.L3682:
	mov	r0, r4
	bl	gc_one.part.24
	cmp	r0, #8
	bne	.L3676
	b	.L3675
	.size	gc_all, .-gc_all
	.align	2
	.global	gc_win_block
	.syntax unified
	.arm
	.type	gc_win_block, %function
gc_win_block:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #4]
	push	{r4, r5, r6, r7, r8, lr}
	ldrh	r6, [r3, #20]
	mul	r6, r1, r6
	cmp	r6, #0
	beq	.L3696
	ldr	r3, [r0, #200]
	cmp	r3, #0
	beq	.L3696
	movw	r7, #:lower16:.LC416
	mov	r5, r0
	mov	r4, #0
	movt	r7, #:upper16:.LC416
	b	.L3687
.L3690:
	mov	r0, r7
	bl	NAND_Print
	cmp	r6, r4
	ble	.L3696
.L3691:
	ldr	r3, [r5, #200]
	cmp	r3, #0
	beq	.L3696
.L3687:
	ldrh	r3, [r3, #4]
	mov	r0, r5
	cmp	r3, #1
	add	r4, r3, r4
	sub	r4, r4, #1
	bls	.L3690
	bl	gc_one.part.24
	cmp	r0, #0
	bne	.L3690
	cmp	r6, r4
	bgt	.L3691
.L3696:
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, pc}
	.size	gc_win_block, .-gc_win_block
	.align	2
	.global	power_on_gc
	.syntax unified
	.arm
	.type	power_on_gc, %function
power_on_gc:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}
	add	r6, r0, #380
	ldrh	r1, [r0, #22]
	ldrh	r3, [r6]
	cmp	r3, r1
	bls	.L3722
	ldr	r3, [r0, #4]
	mov	r4, r0
	ldr	r5, [r0, #200]
	ldrh	r3, [r3, #20]
	cmp	r5, #0
	sub	r7, r3, #10
	uxth	r7, r7
	beq	.L3703
	ldrh	r2, [r5, #4]
	mov	r5, #0
	cmp	r2, r7
	bls	.L3703
.L3706:
	mov	r0, r4
	bl	gc_one.part.24
	cmp	r0, #8
	add	r3, r5, #1
	beq	.L3723
	ldrh	r1, [r4, #22]
	uxth	r5, r3
	ldrh	r3, [r6]
	cmp	r3, r1
	bls	.L3721
	ldr	r3, [r4, #200]
	cmp	r3, #0
	beq	.L3721
	ldrh	r3, [r3, #4]
	cmp	r3, r7
	bhi	.L3706
.L3721:
	ldr	r3, [r4, #4]
	ldrh	r3, [r3, #20]
.L3703:
	cmp	r1, #4
	lsr	r6, r3, #3
	bhi	.L3709
	movw	r7, #:lower16:.LC418
	movt	r7, #:upper16:.LC418
.L3710:
	mov	r0, r7
	bl	NAND_Print
	ldr	r3, [r4, #200]
	cmp	r3, #0
	beq	.L3709
	ldrh	r3, [r3, #4]
	cmp	r3, r6
	bhi	.L3724
.L3709:
	movw	r0, #:lower16:.LC419
	mov	r1, r5
	movt	r0, #:upper16:.LC419
	bl	NAND_Print
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, pc}
.L3724:
	mov	r0, r4
	bl	gc_one.part.24
	cmp	r0, #8
	add	r3, r5, #1
	beq	.L3709
	ldrh	r1, [r4, #22]
	uxth	r5, r3
	cmp	r1, #4
	bls	.L3710
	b	.L3709
.L3722:
	movw	r0, #:lower16:.LC417
	movt	r0, #:upper16:.LC417
	bl	NAND_Print
	mov	r0, #0
	pop	{r4, r5, r6, r7, r8, pc}
.L3723:
	ldr	r3, [r4, #4]
	ldrh	r1, [r4, #22]
	ldrh	r3, [r3, #20]
	b	.L3703
	.size	power_on_gc, .-power_on_gc
	.align	2
	.global	build_zone_list
	.syntax unified
	.arm
	.type	build_zone_list, %function
build_zone_list:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r4, r0
	movw	r0, #:lower16:.LC420
	movt	r0, #:upper16:.LC420
	bl	NAND_Print_DBG
	mov	r0, r4
	bl	first_scan_all_blocks
	ldrh	r1, [r4, #18]
	mov	r0, r4
	bl	init_zone_after_first_scan
	cmp	r0, #0
	bne	.L3734
	mov	r0, r4
	bl	second_scan_all_blocks
	cmp	r0, #0
	bne	.L3735
	mov	r0, r4
	bl	power_on_gc
	ldr	r3, [r4, #356]
	cmp	r3, #0
	bne	.L3736
.L3729:
	movw	r0, #:lower16:.LC423
	movt	r0, #:upper16:.LC423
	bl	NAND_Print
	mov	r0, #0
	pop	{r4, pc}
.L3735:
	movw	r0, #:lower16:.LC422
	movt	r0, #:upper16:.LC422
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, pc}
.L3734:
	movw	r0, #:lower16:.LC421
	movt	r0, #:upper16:.LC421
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, pc}
.L3736:
	mov	r0, r4
	bl	print_nftl_zone.part.17
	b	.L3729
	.size	build_zone_list, .-build_zone_list
	.align	2
	.global	nand_find_zone_table
	.syntax unified
	.arm
	.type	nand_find_zone_table, %function
nand_find_zone_table:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r4, r0
	bl	clean_block_list
	mov	r0, r4
	bl	nand_clean_zone_table2
	movw	r0, #:lower16:.LC424
	movt	r0, #:upper16:.LC424
	bl	NAND_Print
	mov	r0, r4
	bl	build_zone_list
	movw	r0, #:lower16:.LC425
	movt	r0, #:upper16:.LC425
	bl	NAND_Print
	mov	r0, r4
	bl	nftl_cache_init
	movw	r0, #:lower16:.LC426
	movt	r0, #:upper16:.LC426
	bl	NAND_Print
	ldr	r3, [r4, #356]
	cmp	r3, #0
	bne	.L3745
.L3739:
	movw	r0, #:lower16:.LC426
	movt	r0, #:upper16:.LC426
	bl	NAND_Print
	movw	r0, #:lower16:.LC427
	pop	{r4, lr}
	movt	r0, #:upper16:.LC427
	b	NAND_Print
.L3745:
	mov	r0, r4
	bl	print_block_invalid_list.part.7
	ldr	r3, [r4, #356]
	cmp	r3, #0
	beq	.L3739
	mov	r0, r4
	bl	print_free_list.part.6
	b	.L3739
	.size	nand_find_zone_table, .-nand_find_zone_table
	.align	2
	.global	nand_check_table
	.syntax unified
	.arm
	.type	nand_check_table, %function
nand_check_table:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r1, #1000
	mov	r4, r0
	bl	__nand_flush_write_cache
	movw	r0, #:lower16:.LC428
	movt	r0, #:upper16:.LC428
	bl	NAND_Print
	ldr	r3, [r4, #356]
	cmp	r3, #0
	beq	.L3747
	mov	r0, r4
	bl	print_nftl_zone.part.17
.L3747:
	movw	r0, #:lower16:.LC428
	movt	r0, #:upper16:.LC428
	bl	NAND_Print
	mov	r0, r4
	bl	nand_clean_zone_table
	movw	r0, #:lower16:.LC428
	movt	r0, #:upper16:.LC428
	bl	NAND_Print
	mov	r0, r4
	bl	nand_find_zone_table
	mov	r0, r4
	bl	write_data_to_nand
	movw	r0, #:lower16:.LC428
	movt	r0, #:upper16:.LC428
	bl	NAND_Print
	mov	r0, #0
	pop	{r4, pc}
	.size	nand_check_table, .-nand_check_table
	.align	2
	.global	nftl_start
	.syntax unified
	.arm
	.type	nftl_start, %function
nftl_start:
	@ args = 40, pretend = 0, frame = 40
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	sub	sp, sp, #44
	ldr	ip, [sp, #80]
	mov	r8, r2
	ldr	r3, [sp, #88]
	ldr	lr, [sp, #84]
	str	r1, [sp, #16]
	lsr	ip, ip, #9
	ldr	r1, [sp, #92]
	lsr	r3, r3, #9
	str	r0, [sp, #12]
	orr	r2, ip, lr, lsl #23
	movw	r0, #2012
	ldr	r6, [sp, #96]
	orr	r3, r3, r1, lsl #23
	ldr	r9, [sp, #100]
	str	r2, [sp, #28]
	str	r3, [sp, #20]
	bl	NAND_Malloc
	cmp	r0, #0
	str	r0, [r8]
	beq	.L3753
	lsr	r9, r9, #9
	movw	r10, #:lower16:g_nsi
	lsr	r6, r6, #9
	movw	r2, #:lower16:_nftl_nand_erase_superblk
	lsl	r3, r9, #9
	movw	r1, #:lower16:_nftl_nand_read_page
	mov	r4, r0
	uxth	r3, r3
	movt	r10, #:upper16:g_nsi
	movt	r2, #:upper16:_nftl_nand_erase_superblk
	str	r3, [sp, #24]
	movt	r1, #:upper16:_nftl_nand_read_page
	mov	r3, #0
	str	r2, [sp, #32]
	str	r1, [sp, #36]
	str	r3, [sp, #8]
.L3754:
	movw	r2, #2012
	mov	r1, #0
	mov	r0, r4
	bl	NAND_Memset
	ldr	r3, [sp, #12]
	mov	r0, #36
	str	r3, [r4]
	ldr	r3, [sp, #16]
	str	r3, [r4, #1972]
	bl	NAND_Malloc
	cmp	r0, #0
	mov	r3, r0
	str	r0, [r4, #4]
	beq	.L3753
	ldr	r2, [r10]
	mov	r1, r6
	ldr	r0, [sp, #20]
	ldr	r2, [r2, #8]
	ldr	ip, [r2, #8]!	@ unaligned
	ldr	r2, [r2, #4]	@ unaligned
	str	ip, [r3]	@ unaligned
	str	r2, [r3, #4]	@ unaligned
	ldrh	r2, [sp, #24]
	ldr	r3, [r4, #4]
	strh	r2, [r3, #26]	@ movhi
	ldrb	r2, [sp, #104]	@ zero_extendqisi2
	strb	r2, [r3, #10]
	bl	__aeabi_uidiv
	ldr	r3, [r4, #1972]
	mov	r7, r0
	ldrh	r3, [r3, #12]
	cmp	r0, r3
	bls	.L3766
	ldr	r2, [r4, #4]
	uxth	r5, r0
	mov	r1, r9
	mov	r0, r6
	add	fp, r7, #1
	strh	r5, [r2, #24]	@ movhi
	str	r2, [sp]
	bl	__aeabi_uidiv
	ldr	r2, [sp]
	add	ip, fp, fp, lsl #3
	strb	r9, [r2, #8]
	ldr	r1, [r4, #4]
	strh	r0, [r2, #20]	@ movhi
	lsl	r2, ip, #2
	movw	r0, #3000
	ldrb	ip, [r1, #8]	@ zero_extendqisi2
	strh	r0, [r1, #16]	@ movhi
	mov	r0, r2
	str	r1, [sp]
	strh	ip, [r1, #28]	@ movhi
	str	r2, [sp, #4]
	bl	nftl_malloc
	ldr	ip, [r4, #4]
	ldr	r1, [sp]
	str	r0, [r1, #32]
	ldr	r0, [ip, #32]
	cmp	r0, #0
	beq	.L3753
	ldr	r2, [sp, #4]
	mov	r1, #255
	sub	r2, r2, #36
	bl	NAND_Memset
	cmp	fp, #0
	beq	.L3760
	ldr	r2, [r4, #4]
	mov	r0, #0
	mov	r1, r0
	mvn	ip, #0
	ldr	r2, [r2, #32]
.L3759:
	strh	r0, [r2]	@ movhi
	add	r0, r0, #1
	cmp	fp, r0
	strh	r1, [r2, #4]	@ movhi
	strh	r1, [r2, #2]	@ movhi
	add	r2, r2, #36
	str	ip, [r2, #-28]
	strh	r1, [r2, #-30]	@ movhi
	str	r1, [r2, #-24]
	str	r1, [r2, #-20]
	str	r1, [r2, #-16]
	str	r1, [r2, #-12]
	str	r1, [r2, #-8]
	str	r1, [r2, #-4]
	bne	.L3759
.L3760:
	ldr	r3, [r8]
	movw	r1, #:lower16:_nftl_nand_write_page
	movt	r1, #:upper16:_nftl_nand_write_page
	movw	r2, #:lower16:_nftl_nand_is_blk_good
	movt	r2, #:upper16:_nftl_nand_is_blk_good
	ldr	ip, [sp, #36]
	str	r1, [r3, #1984]
	movw	r0, #:lower16:_nftl_nand_mark_bad_blk
	ldr	r1, [sp, #32]
	movt	r0, #:upper16:_nftl_nand_mark_bad_blk
	str	r2, [r3, #1992]
	movw	r2, #:lower16:_nftl_nand_read_logic_page
	movt	r2, #:upper16:_nftl_nand_read_logic_page
	str	r0, [r3, #1996]
	str	r1, [r3, #1976]
	movw	r1, #:lower16:_nftl_nand_copy_page
	movt	r1, #:upper16:_nftl_nand_copy_page
	str	r2, [r3, #2004]
	str	r1, [r3, #1988]
	movw	r2, #:lower16:_nftl_discard_logic_page
	movw	r1, #:lower16:_nftl_nand_write_logic_page
	movt	r2, #:upper16:_nftl_discard_logic_page
	movt	r1, #:upper16:_nftl_nand_write_logic_page
	str	r2, [r3, #2008]
	str	r1, [r3, #2000]
	mov	r1, r6
	str	ip, [r3, #1980]
	ldr	r0, [sp, #28]
	bl	__aeabi_uidiv
	ldr	r3, [r4, #1972]
	sub	r0, r5, r0
	uxth	r2, r0
	ldrh	r3, [r3, #12]
	strh	r2, [r4, #22]	@ movhi
	cmp	r3, r2
	bhi	.L3789
	movw	r0, #:lower16:.LC430
	mov	r1, r7
	movt	r0, #:upper16:.LC430
	bl	NAND_Print_DBG
	ldrh	r3, [r4, #22]
	mov	r1, #0
	ldr	r0, [r8]
	sub	r2, r7, r3
	mul	r2, r6, r2
	mul	r3, r3, r6
	bl	zone_param_init
	subs	r5, r0, #0
	bne	.L3790
	ldr	r3, [r4, #1972]
	ldrb	r2, [sp, #112]	@ zero_extendqisi2
	strb	r2, [r4, #16]
	ldrh	r2, [r3, #26]
	cmp	r2, #0
	ldrbne	r2, [r4, #17]	@ zero_extendqisi2
	orrne	r2, r2, #1
	strbne	r2, [r4, #17]
	ldrh	r3, [r3, #10]
	cmp	r3, #0
	beq	.L3763
	mov	r1, r7
	ldr	r0, [r8]
	bl	is_no_use_device
	cmp	r0, #1
	beq	.L3791
.L3763:
	ldr	r0, [r8]
	bl	build_zone_list
	subs	r7, r0, #0
	beq	.L3764
	movw	r0, #:lower16:.LC433
	movt	r0, #:upper16:.LC433
	bl	NAND_Print
	ldr	r3, [sp, #8]
	cmp	r3, #0
	bne	.L3765
	movw	r0, #:lower16:.LC434
	mov	r3, #1
	movt	r0, #:upper16:.LC434
	str	r3, [sp, #8]
	bl	NAND_Print
	b	.L3754
.L3765:
	mov	r5, r7
.L3755:
	mov	r0, r5
	add	sp, sp, #44
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L3791:
	mov	r1, r7
	ldr	r0, [r8]
	bl	build_zone_list_first
	cmp	r0, #0
	beq	.L3763
	mov	r3, r0
	movw	r0, #:lower16:.LC432
	movt	r0, #:upper16:.LC432
	mov	r5, r3
	bl	NAND_Print
	b	.L3755
.L3753:
	movw	r0, #:lower16:.LC221
	mvn	r5, #1
	movt	r0, #:upper16:.LC221
	bl	NAND_Print
	b	.L3755
.L3766:
	mvn	r5, #0
	b	.L3755
.L3790:
	movw	r0, #:lower16:.LC431
	movt	r0, #:upper16:.LC431
	bl	NAND_Print
	b	.L3755
.L3789:
	movw	r0, #:lower16:.LC429
	mov	r1, r2
	movt	r0, #:upper16:.LC429
	mvn	r5, #1
	bl	NAND_Print
	b	.L3755
.L3764:
	ldr	r0, [r8]
	bl	nftl_cache_init
	b	.L3755
	.size	nftl_start, .-nftl_start
	.align	2
	.global	nftl_initialize
	.syntax unified
	.arm
	.type	nftl_initialize, %function
nftl_initialize:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	ip, .L3813
	push	{r4, r5, r6, r7, r8, r9, r10, lr}
	mov	r4, r0
	mov	r6, r1
	sub	sp, sp, #56
	mov	lr, #0
	add	r3, sp, #44
	ldr	r0, [ip]	@ unaligned
	add	r2, ip, #12
	ldr	r1, [ip, #4]	@ unaligned
	ldrb	r5, [ip, #8]	@ zero_extendqisi2
	mov	ip, #47
	strb	lr, [sp, #41]
	stmia	r3!, {r0, r1}
	mov	r0, lr
	add	r1, sp, #40
	strb	r5, [r3]
	strb	ip, [sp, #40]
	ldr	r5, [r4, #4]
	bl	nand_fdt_get_prop_string_by_path
	cmp	r0, #0
	beq	.L3805
	add	r1, sp, #44
	mov	r2, #9
	bl	nand_memcmp
	subs	r7, r0, #0
	bne	.L3805
	movw	r0, #:lower16:.LC439
	movw	ip, #:lower16:.LC440
	movt	r0, #:upper16:.LC439
	movt	ip, #:upper16:.LC440
	str	r0, [sp, #4]
	movw	r3, #:lower16:.LC435
	movw	r2, #:lower16:.LC436
	movw	r1, #:lower16:.LC437
	movw	r0, #:lower16:.LC438
	movt	r3, #:upper16:.LC435
	str	ip, [sp]
	movt	r2, #:upper16:.LC436
	movt	r1, #:upper16:.LC437
	movt	r0, #:upper16:.LC438
	bl	NAND_Print
	ldrh	r3, [r5, #34]
	cmp	r3, #15
	bls	.L3805
	mov	r0, #32
	bl	NAND_Malloc
	cmp	r0, #0
	str	r0, [r4, #16]
	beq	.L3811
	ldr	r1, [r5, #80]
	movw	r8, #2474
	movw	r10, #2478
	strh	r7, [r0]	@ movhi
	movw	lr, #50816
	mov	ip, #1
	ldr	r2, [r1, #36]
	movt	lr, 19
	strh	r7, [r0, #8]	@ movhi
	strh	r7, [r0, #10]	@ movhi
	ldrh	r3, [r2, r8]
	add	r7, r2, #2480
	ldrh	r9, [r7]
	ldrh	r7, [r2, r10]
	cmp	r3, #0
	str	lr, [r0, #28]
	strh	r9, [r0, #16]	@ movhi
	mov	lr, #3
	mov	r9, #8
	strh	r7, [r0, #12]	@ movhi
	mov	r7, #2
	strh	ip, [r0, #2]	@ movhi
	strh	ip, [r0, #4]	@ movhi
	strh	ip, [r0, #6]	@ movhi
	strh	r9, [r0, #14]	@ movhi
	strh	r7, [r0, #18]	@ movhi
	strh	lr, [r0, #20]	@ movhi
	bne	.L3797
	mov	r3, #30
	strh	lr, [r0, #24]	@ movhi
	strh	r3, [r0, #22]	@ movhi
.L3798:
	ldrh	r2, [r1, #2]
	cmp	r2, #0
	strh	r2, [r0, #26]	@ movhi
	movne	r2, #3
	moveq	r2, #1
	cmp	r3, #30
	strh	r2, [r0, #24]	@ movhi
	movhi	r3, #30
	movw	r2, #24888
	strhhi	r3, [r0, #22]	@ movhi
	movt	r2, 117
	ldr	r3, [r1, #36]
	mov	r1, #1
	strh	r1, [r0, #24]	@ movhi
	ldr	r1, [r3, #2492]
	sub	r3, r1, #86016
	sub	r3, r3, #384
	cmp	r3, r2
	mov	r3, #0
	strls	r1, [r0, #28]
	add	r2, r4, #12
	str	r6, [sp, #32]
	mov	r1, r0
	str	r5, [sp, #28]
	mov	r0, r4
	str	r3, [sp, #36]
	ldrh	r3, [r5, #34]
	str	r3, [sp, #24]
	ldrh	r3, [r5, #40]
	str	r3, [sp, #20]
	ldr	r3, [r5, #44]
	str	r3, [sp, #16]
	ldrd	r6, [r5, #72]
	strd	r6, [sp, #8]
	ldrd	r6, [r5, #64]
	strd	r6, [sp]
	bl	nftl_start
	subs	r6, r0, #0
	bne	.L3794
	ldr	r2, [r5, #80]
	movw	r0, #:lower16:.LC441
	ldr	r3, [r4, #12]
	movt	r0, #:upper16:.LC441
	ldr	r2, [r2, #36]
	ldr	r3, [r3, #4]
	ldr	r1, [r2, #2484]
	ldr	r2, [r2, #2488]
	strh	r1, [r3, #16]	@ movhi
	strh	r2, [r3, #30]	@ movhi
	uxth	r1, r1
	bl	NAND_Print_DBG
	ldr	r1, [r4, #12]
	movw	r2, #:lower16:_nand_read
	movw	r3, #:lower16:_nand_write
	movt	r2, #:upper16:_nand_read
	movt	r3, #:upper16:_nand_write
	movw	lr, #:lower16:_nand_discard
	ldr	r1, [r1, #24]
	movw	ip, #:lower16:_nand_flush_write_cache
	movw	r0, #:lower16:_shutdown_op
	str	r2, [r4, #40]
	str	r3, [r4, #44]
	movw	r2, #:lower16:_nand_sector_write
	str	r1, [r4]
	movw	r3, #:lower16:_nand_flush_sector_write_cache
	movw	r1, #:lower16:_nand_sector_read
	movt	lr, #:upper16:_nand_discard
	movt	ip, #:upper16:_nand_flush_write_cache
	movt	r0, #:upper16:_shutdown_op
	movt	r1, #:upper16:_nand_sector_read
	movt	r2, #:upper16:_nand_sector_write
	movt	r3, #:upper16:_nand_flush_sector_write_cache
	str	r6, [r4, #8]
	str	lr, [r4, #52]
	str	ip, [r4, #48]
	str	r0, [r4, #56]
	str	r1, [r4, #60]
	str	r2, [r4, #64]
	str	r3, [r4, #68]
.L3794:
	mov	r0, r6
	add	sp, sp, #56
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L3797:
	cmp	r3, #255
	beq	.L3812
	cmp	r3, #10
	bls	.L3800
	mov	r3, #10
	mov	ip, #30
	strh	r3, [r2, r8]	@ movhi
	mov	r3, ip
	strh	lr, [r0, #24]	@ movhi
	strh	ip, [r0, #22]	@ movhi
	b	.L3798
.L3800:
	add	r2, r3, r3, lsl #2
	add	r3, r2, r2, lsl #2
	lsl	r2, r2, #3
	lsl	r3, r3, #1
	strh	r2, [r0, #24]	@ movhi
	uxth	r3, r3
	strh	r3, [r0, #22]	@ movhi
	b	.L3798
.L3805:
	mvn	r6, #0
	mov	r0, r6
	add	sp, sp, #56
	@ sp needed
	pop	{r4, r5, r6, r7, r8, r9, r10, pc}
.L3812:
	mov	r2, #50
	mov	lr, #40
	strh	ip, [r0]	@ movhi
	mov	r3, r2
	strh	r2, [r0, #22]	@ movhi
	strh	lr, [r0, #24]	@ movhi
	b	.L3798
.L3811:
	movw	r0, #:lower16:.LC221
	mov	r6, #1
	movt	r0, #:upper16:.LC221
	bl	NAND_Print
	b	.L3794
.L3814:
	.align	2
.L3813:
	.word	.LANCHOR2+860
	.size	nftl_initialize, .-nftl_initialize
	.align	2
	.global	nftl_add
	.syntax unified
	.arm
	.type	nftl_add, %function
nftl_add:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r5, r0
	mov	r0, #72
	bl	NAND_Malloc
	subs	r4, r0, #0
	beq	.L3822
	mov	r0, r5
	bl	build_nand_partition
	ldrh	r1, [r5]
	str	r0, [r4, #4]
	mov	r0, r4
	bl	nftl_initialize
	cmp	r0, #0
	streq	r0, [r4, #8]
	ldreq	r2, .L3824
	beq	.L3819
	b	.L3823
.L3820:
	mov	r2, r3
.L3819:
	ldr	r3, [r2, #8]
	cmp	r3, #0
	bne	.L3820
	movw	r0, #:lower16:.LC443
	str	r4, [r2, #8]
	movt	r0, #:upper16:.LC443
	bl	NAND_Print_DBG
	mov	r0, r4
	pop	{r4, r5, r6, pc}
.L3823:
	movw	r0, #:lower16:.LC442
	movt	r0, #:upper16:.LC442
	bl	NAND_Print
	mov	r0, #0
	pop	{r4, r5, r6, pc}
.L3822:
	movw	r0, #:lower16:.LC221
	movt	r0, #:upper16:.LC221
	bl	NAND_Print
	mov	r0, r4
	pop	{r4, r5, r6, pc}
.L3825:
	.align	2
.L3824:
	.word	.LANCHOR0+4
	.size	nftl_add, .-nftl_add
	.align	2
	.global	nftl_build_all
	.syntax unified
	.arm
	.type	nftl_build_all, %function
nftl_build_all:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	mov	r2, #0
	ldr	r4, [r0, #56]
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	str	r2, [r3, #12]
	cmp	r4, r2
	bne	.L3830
	b	.L3831
.L3829:
	ldr	r4, [r4, #48]
	cmp	r4, #0
	beq	.L3831
.L3830:
	mov	r0, r4
	bl	nftl_add
	cmp	r0, #0
	bne	.L3829
	movw	r0, #:lower16:.LC444
	movt	r0, #:upper16:.LC444
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, pc}
.L3831:
	mov	r0, #0
	pop	{r4, pc}
	.size	nftl_build_all, .-nftl_build_all
	.align	2
	.global	nftl_build_one
	.syntax unified
	.arm
	.type	nftl_build_one, %function
nftl_build_one:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	subs	r5, r1, #0
	ldr	r4, [r0, #56]
	movweq	r3, #:lower16:.LANCHOR0
	movteq	r3, #:upper16:.LANCHOR0
	streq	r5, [r3, #12]
	b	.L3843
.L3838:
	ldr	r4, [r4, #48]
.L3843:
	cmp	r4, #0
	beq	.L3845
	ldrh	r3, [r4]
	cmp	r5, r3
	bne	.L3838
	mov	r0, r4
	bl	nftl_add
	cmp	r0, #0
	bne	.L3838
	movw	r0, #:lower16:.LC444
	movt	r0, #:upper16:.LC444
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, pc}
.L3845:
	mov	r0, #0
	pop	{r4, r5, r6, pc}
	.size	nftl_build_one, .-nftl_build_one
	.align	2
	.global	all_time_gc
	.syntax unified
	.arm
	.type	all_time_gc, %function
all_time_gc:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r2, #382
	ldrh	r1, [r0, #22]
	ldrh	r2, [r0, r2]
	cmp	r1, r2
	bcs	.L3855
	add	r2, r0, #384
	ldrh	r2, [r2]
	cmp	r2, #0
	beq	.L3855
	ldr	r2, [r0, #200]
	cmp	r2, #0
	beq	.L3855
	ldr	r3, [r0, #4]
	ldrh	r2, [r2, #4]
	ldrh	r3, [r3, #20]
	sub	r3, r3, #3
	uxth	r3, r3
	cmp	r2, r3
	bhi	.L3856
.L3855:
	mov	r0, #0
	bx	lr
.L3856:
	push	{r4, lr}
	bl	gc_one.part.24
	mov	r0, #0
	pop	{r4, pc}
	.size	all_time_gc, .-all_time_gc
	.align	2
	.global	garbage_collect
	.syntax unified
	.arm
	.type	garbage_collect, %function
garbage_collect:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	add	r3, r0, #380
	ldrh	r1, [r0, #22]
	ldrh	r3, [r3]
	push	{r4, lr}
	cmp	r3, r1
	add	r3, r0, #384
	movcs	r2, #0
	strhcs	r2, [r3]	@ movhi
	bcs	.L3859
	ldrh	r2, [r3]
	cmp	r2, #0
	beq	.L3859
.L3883:
	mov	r3, #0
.L3880:
	mov	r0, r3
	pop	{r4, pc}
.L3859:
	movw	r2, #382
	ldrh	r2, [r0, r2]
	cmp	r2, r1
	bls	.L3861
	movw	r3, #386
	mov	r4, r0
	ldrh	r3, [r0, r3]
	cmp	r3, #0
	bne	.L3862
	ldr	r3, [r0, #200]
	cmp	r3, #0
	beq	.L3884
	ldr	r0, [r0, #4]
	ldrh	r2, [r3, #4]
	ldrh	r3, [r0, #20]
	cmp	r2, r3, lsr #1
	bcc	.L3864
	add	r3, r4, #388
	mov	r2, #4
	strh	r2, [r3]	@ movhi
.L3865:
	ldr	r2, [r4, #1972]
	ldrh	r2, [r2, #16]
	cmp	r2, r1
	bcs	.L3872
	ldrh	r2, [r3]
	b	.L3871
.L3861:
	mov	r2, #0
	mov	lr, #1
	strh	lr, [r3]	@ movhi
	add	r1, r0, #388
	movw	ip, #386
	mov	r3, r2
	strh	r2, [r0, ip]	@ movhi
	mov	r0, r3
	strh	r2, [r1]	@ movhi
	pop	{r4, pc}
.L3862:
	add	r3, r0, #388
	movw	r1, #65535
	ldrh	r2, [r3]
	cmp	r2, r1
	beq	.L3869
.L3871:
	mov	r1, #0
	mov	r0, r4
	bl	garbage_collect_first
	cmp	r0, #0
	beq	.L3883
	movw	r0, #:lower16:.LC447
	ldrh	r1, [r4, #22]
	movt	r0, #:upper16:.LC447
	bl	NAND_Print
	mov	r3, #1
	b	.L3880
.L3864:
	cmp	r2, r3, lsr #2
	bcs	.L3885
	cmp	r2, r3, lsr #3
	addcs	r3, r4, #388
	movcs	r2, #16
	strhcs	r2, [r3]	@ movhi
	bcs	.L3865
.L3867:
	movw	r0, #:lower16:.LC446
	movt	r0, #:upper16:.LC446
	bl	NAND_Print
	ldr	r2, [r4, #1972]
	add	r3, r4, #388
	ldrh	r1, [r4, #22]
	mvn	r0, #0
	strh	r0, [r3]	@ movhi
	ldrh	r2, [r2, #16]
	cmp	r1, r2
	bhi	.L3869
.L3872:
	mvn	r2, #0
	strh	r2, [r3]	@ movhi
.L3869:
	mov	r0, r4
	mov	r1, #1
	bl	gc_win_block
	b	.L3883
.L3885:
	add	r3, r4, #388
	mov	r2, #8
	strh	r2, [r3]	@ movhi
	b	.L3865
.L3884:
	movw	r0, #:lower16:.LC445
	movt	r0, #:upper16:.LC445
	bl	NAND_Print
	mov	r3, #1
	b	.L3880
	.size	garbage_collect, .-garbage_collect
	.align	2
	.global	_nftl_nand_write_logic_page
	.syntax unified
	.arm
	.type	_nftl_nand_write_logic_page, %function
_nftl_nand_write_logic_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, lr}
	mov	r3, r2
	mov	r2, r1
	mov	r1, #192
	mov	r5, r0
	bl	nand_write_data_page
	subs	r4, r0, #0
	bne	.L3890
	mov	r0, r5
	bl	garbage_collect
	mov	r0, r5
	bl	all_time_gc
	mov	r0, r5
	bl	do_prio_gc
	mov	r0, r4
	pop	{r4, r5, r6, pc}
.L3890:
	movw	r0, #:lower16:.LC448
	movt	r0, #:upper16:.LC448
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, pc}
	.size	_nftl_nand_write_logic_page, .-_nftl_nand_write_logic_page
	.align	2
	.global	nftl_ops_init
	.syntax unified
	.arm
	.type	nftl_ops_init, %function
nftl_ops_init:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r2, #:lower16:_nftl_nand_erase_superblk
	movw	r3, #:lower16:_nftl_nand_read_page
	movt	r2, #:upper16:_nftl_nand_erase_superblk
	movt	r3, #:upper16:_nftl_nand_read_page
	push	{r4, r5, lr}
	movw	ip, #:lower16:_nftl_nand_copy_page
	movw	r5, #:lower16:_nftl_nand_write_page
	movw	r4, #:lower16:_nftl_nand_is_blk_good
	movw	lr, #:lower16:_nftl_nand_mark_bad_blk
	movw	r1, #:lower16:_nftl_nand_read_logic_page
	str	r2, [r0, #1976]
	movw	r2, #:lower16:_nftl_nand_write_logic_page
	str	r3, [r0, #1980]
	movw	r3, #:lower16:_nftl_discard_logic_page
	movt	r5, #:upper16:_nftl_nand_write_page
	movt	r4, #:upper16:_nftl_nand_is_blk_good
	movt	lr, #:upper16:_nftl_nand_mark_bad_blk
	movt	ip, #:upper16:_nftl_nand_copy_page
	movt	r1, #:upper16:_nftl_nand_read_logic_page
	movt	r2, #:upper16:_nftl_nand_write_logic_page
	movt	r3, #:upper16:_nftl_discard_logic_page
	str	r5, [r0, #1984]
	str	r4, [r0, #1992]
	str	lr, [r0, #1996]
	str	ip, [r0, #1988]
	str	r1, [r0, #2004]
	str	r2, [r0, #2000]
	str	r3, [r0, #2008]
	pop	{r4, r5, pc}
	.size	nftl_ops_init, .-nftl_ops_init
	.align	2
	.global	current_block_at_last_page
	.syntax unified
	.arm
	.type	current_block_at_last_page, %function
current_block_at_last_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldr	r3, [r0, #4]
	ldrh	r0, [r1, #28]
	ldrh	r3, [r3, #20]
	sub	r3, r3, #1
	sub	r0, r3, r0
	clz	r0, r0
	lsr	r0, r0, #5
	bx	lr
	.size	current_block_at_last_page, .-current_block_at_last_page
	.align	2
	.global	current_block_at_first_page
	.syntax unified
	.arm
	.type	current_block_at_first_page, %function
current_block_at_first_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	ldrh	r0, [r1, #28]
	clz	r0, r0
	lsr	r0, r0, #5
	bx	lr
	.size	current_block_at_first_page, .-current_block_at_first_page
	.align	2
	.global	fill_no_use_data_to_page
	.syntax unified
	.arm
	.type	fill_no_use_data_to_page, %function
fill_no_use_data_to_page:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #4]
	push	{r4, r5, r6, lr}
	add	r5, r0, #292
	ldrh	r2, [r5, #28]
	mov	r4, r0
	ldrh	r3, [r3, #20]
	sub	r3, r3, #1
	cmp	r2, r3
	beq	.L3934
	cmp	r2, #0
	beq	.L3904
	mov	r1, r5
	bl	write_phy_page_map_to_current_block
	cmp	r0, #0
	bne	.L3935
.L3911:
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	bne	.L3936
.L3929:
	mov	r0, #8
	pop	{r4, r5, r6, pc}
.L3934:
	mov	r1, r5
	bl	write_phy_page_map_to_current_block
	cmp	r0, #0
	bne	.L3937
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	beq	.L3900
	add	r1, r4, #324
	mov	r0, r4
	bl	write_phy_page_map_to_current_block
	cmp	r0, #0
	beq	.L3900
	movw	r0, #:lower16:.LC387
	movt	r0, #:upper16:.LC387
	bl	NAND_Print
.L3900:
	mov	r1, r5
	mov	r0, r4
	bl	get_new_current_block
	cmp	r0, #0
	bne	.L3938
	ldrb	r3, [r4, #17]	@ zero_extendqisi2
	tst	r3, #1
	beq	.L3904
	add	r1, r4, #324
	mov	r0, r4
	bl	get_new_current_block
	cmp	r0, #0
	bne	.L3939
.L3904:
	mov	r0, #0
	pop	{r4, r5, r6, pc}
.L3935:
	movw	r0, #:lower16:.LC387
	movt	r0, #:upper16:.LC387
	bl	NAND_Print
	b	.L3911
.L3937:
	movw	r0, #:lower16:.LC387
	movt	r0, #:upper16:.LC387
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, pc}
.L3938:
	movw	r0, #:lower16:.LC388
	movt	r0, #:upper16:.LC388
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, pc}
.L3939:
	movw	r0, #:lower16:.LC389
	movt	r0, #:upper16:.LC389
	bl	NAND_Print
	mov	r0, #1
	pop	{r4, r5, r6, pc}
.L3936:
	add	r1, r4, #324
	mov	r0, r4
	bl	write_phy_page_map_to_current_block
	cmp	r0, #0
	beq	.L3929
	movw	r0, #:lower16:.LC387
	movt	r0, #:upper16:.LC387
	bl	NAND_Print
	b	.L3929
	.size	fill_no_use_data_to_page, .-fill_no_use_data_to_page
	.align	2
	.global	nftl_write_end
	.syntax unified
	.arm
	.type	nftl_write_end, %function
nftl_write_end:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:.LANCHOR0
	push	{r4, r5, r6, lr}
	movt	r3, #:upper16:.LANCHOR0
	ldr	r6, [r3, #12]
	cmp	r6, #0
	beq	.L3951
.L3944:
	ldr	r5, [r6, #12]
	mov	r4, #8
.L3943:
	mov	r0, r5
	bl	fill_no_use_data_to_page
	cmp	r0, #0
	beq	.L3942
	subs	r4, r4, #1
	bne	.L3943
.L3942:
	ldr	r6, [r6, #8]
	cmp	r6, #0
	bne	.L3944
.L3951:
	mov	r0, #0
	pop	{r4, r5, r6, pc}
	.size	nftl_write_end, .-nftl_write_end
	.align	2
	.global	fill_no_use_data_to_current_block
	.syntax unified
	.arm
	.type	fill_no_use_data_to_current_block, %function
fill_no_use_data_to_current_block:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r3, [r0, #4]
	ldrh	r3, [r3, #20]
	cmp	r3, #0
	beq	.L3964
	push	{r4, r5, r6, lr}
	mov	r5, r0
	mov	r4, #0
.L3956:
	mov	r0, r5
	add	r4, r4, #1
	bl	fill_no_use_data_to_page
	cmp	r0, #0
	popeq	{r4, r5, r6, pc}
	ldr	r3, [r5, #4]
	ldrh	r3, [r3, #20]
	cmp	r3, r4
	bgt	.L3956
	mov	r0, #1
	pop	{r4, r5, r6, pc}
.L3964:
	mov	r0, #1
	bx	lr
	.size	fill_no_use_data_to_current_block, .-fill_no_use_data_to_current_block
	.align	2
	.global	done_static_wear_leveling
	.syntax unified
	.arm
	.type	done_static_wear_leveling, %function
done_static_wear_leveling:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r1, [r0, #580]
	push	{r4, r5, r6, lr}
	cmp	r1, #0
	moveq	r3, #2
	beq	.L3989
	ldrb	r3, [r0, #17]	@ zero_extendqisi2
	tst	r3, #1
	bne	.L3994
.L3967:
	ldr	r3, [r0, #292]
	cmp	r1, r3
	beq	.L3968
	ldr	r3, [r0, #200]
	mov	r4, r0
	cmp	r3, #0
	bne	.L3972
	b	.L3973
.L3995:
	ldr	r3, [r3, #12]
	cmp	r3, #0
	beq	.L3973
.L3972:
	cmp	r1, r3
	bne	.L3995
	movw	r0, #:lower16:.LC451
	ldrh	r2, [r1, #6]
	movt	r0, #:upper16:.LC451
	ldrh	r1, [r1]
	bl	NAND_Print
	ldr	r2, [r4, #1968]
	mov	r5, #1
	movw	r6, #578
	mov	r0, r4
	strh	r5, [r4, r6]	@ movhi
	ldr	r3, [r2, #132]
	add	r3, r3, r5
	str	r3, [r2, #132]
	bl	fill_no_use_data_to_current_block
	movw	r2, #65535
	ldr	r1, [r4, #580]
	mov	r0, r4
	bl	garbage_collect_first
	cmp	r0, #0
	bne	.L3996
	ldr	r3, [r4, #172]
	ldr	r2, [r4, #580]
	strh	r0, [r4, r6]	@ movhi
	cmp	r3, #0
	bne	.L3978
	b	.L3974
.L3997:
	ldr	r3, [r3, #20]
	cmp	r3, #0
	beq	.L3974
.L3978:
	cmp	r2, r3
	bne	.L3997
	mov	r2, #0
	mov	r3, r2
	str	r2, [r4, #580]
.L3989:
	mov	r0, r3
	pop	{r4, r5, r6, pc}
.L3994:
	ldr	r3, [r0, #324]
	cmp	r1, r3
	bne	.L3967
.L3968:
	mov	r2, #0
	mov	r3, #2
	str	r2, [r0, #580]
	mov	r0, r3
	pop	{r4, r5, r6, pc}
.L3973:
	movw	r0, #:lower16:.LC450
	ldrh	r1, [r1]
	movt	r0, #:upper16:.LC450
	bl	NAND_Print
	mov	r2, #0
	mov	r3, #2
	str	r2, [r4, #580]
	mov	r0, r3
	pop	{r4, r5, r6, pc}
.L3974:
	movw	r0, #:lower16:.LC452
	ldrh	r1, [r2]
	movt	r0, #:upper16:.LC452
	bl	NAND_Print
	mov	r3, #0
	str	r3, [r4, #580]
	b	.L3989
.L3996:
	ldr	r3, [r4, #580]
	movw	r0, #:lower16:.LC449
	movt	r0, #:upper16:.LC449
	ldrh	r1, [r3]
	bl	NAND_Print
	mov	r2, #0
	mov	r3, r5
	str	r2, [r4, #580]
	b	.L3989
	.size	done_static_wear_leveling, .-done_static_wear_leveling
	.align	2
	.global	do_static_wear_leveling
	.syntax unified
	.arm
	.type	do_static_wear_leveling, %function
do_static_wear_leveling:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	ldr	r2, [r0, #1972]
	ldrh	r2, [r2, #6]
	cmp	r2, #0
	beq	.L4004
	add	r2, r0, #384
	ldrh	r2, [r2]
	cmp	r2, #0
	beq	.L4002
	ldr	r2, [r0, #580]
	cmp	r2, #0
	beq	.L4006
.L4004:
	mov	r0, #0
	bx	lr
.L4002:
	mov	r0, #1
	bx	lr
.L4006:
	push	{r4, lr}
	mov	r4, r0
	bl	check_static_wear_leveling.part.9
	cmp	r0, #0
	bne	.L4001
	mov	r0, r4
	pop	{r4, lr}
	b	done_static_wear_leveling
.L4001:
	mov	r0, #0
	pop	{r4, pc}
	.size	do_static_wear_leveling, .-do_static_wear_leveling
	.align	2
	.global	fill_no_use_page_to_current_block
	.syntax unified
	.arm
	.type	fill_no_use_page_to_current_block, %function
fill_no_use_page_to_current_block:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	cmp	r1, #0
	ble	.L4018
	push	{r4, r5, r6, lr}
	mov	r5, r0
	mov	r6, r1
	mov	r4, #0
.L4010:
	mov	r0, r5
	add	r4, r4, #1
	bl	fill_no_use_data_to_page
	cmp	r0, #0
	popeq	{r4, r5, r6, pc}
	cmp	r6, r4
	bne	.L4010
	mov	r0, #1
	pop	{r4, r5, r6, pc}
.L4018:
	mov	r0, #1
	bx	lr
	.size	fill_no_use_page_to_current_block, .-fill_no_use_page_to_current_block
	.align	2
	.global	nand_before_shutdown
	.syntax unified
	.arm
	.type	nand_before_shutdown, %function
nand_before_shutdown:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.size	nand_before_shutdown, .-nand_before_shutdown
	.global	read_mapping_page
	.global	m9_1z_8g
	.global	m9_1z_16g
	.global	m9_1y
	.global	m9_read_retry_reg_adr_1y
	.global	m9_read_retry_cmd_1y
	.global	m9_read_retry_exit_cmd_1y
	.global	m9_read_retry_enable_cmd_1y
	.global	m9_read_retry_reg_cnt
	.global	m9_read_retry_cycle
	.global	m9_read_retry_mode
	.global	m2_lsb_mode_reg_num
	.global	m2_lsb_mode_val
	.global	m2_lsb_mode_default_val
	.global	m2_lsb_mode_reg_adr
	.global	m2_read_retry_reg_adr_2
	.global	m2_read_retry_reg_adr_1
	.global	m2_read_retry_reg_adr
	.global	m2_read_retry_reg_num
	.global	m2_read_retry_cycle
	.global	m2_read_retry_mode
	.global	m4_p1_3
	.global	m4_p1_2
	.global	m4_p1_1
	.global	m4_read_retry_cycle
	.global	m4_read_retry_mode
	.global	m1_read_retry_reg_adr
	.global	m1_read_retry_reg_num
	.global	m1_read_retry_cycle
	.global	m1_read_retry_mode
	.global	m6_read_retry_exit_cmd
	.global	m6_read_retry_last_cmd
	.global	m6_p1_3
	.global	m6_p1_2
	.global	m6_read_retry_addr_2
	.global	m6_read_retry_cmd_2
	.global	m6_p1_1
	.global	m6_read_retry_addr_1
	.global	m6_read_retry_cmd_1
	.global	m6_read_retry_end_cmd
	.global	m6_read_retry_start_cmd
	.global	m6_read_retry_reg_cnt
	.global	m6_read_retry_cycle
	.global	m6_read_retry_mode
	.global	m7_1y
	.global	m7_read_retry_reg_adr_1y
	.global	m7_read_retry_cmd_1y
	.global	m7_read_retry_exit_cmd_1y
	.global	m7_read_retry_enable_cmd_1y
	.global	m7_24
	.global	m7_read_retry_reg_adr_24
	.global	m7_read_retry_cmd_24
	.global	m7_19_high
	.global	m7_19_low
	.global	m7_read_retry_reg_adr_19
	.global	m7_read_retry_cmd_19
	.global	m7_read_retry_disable_cmd_19
	.global	m7_read_retry_enable_cmd_19
	.global	m7_acti_start_addr
	.global	m7_acti_start_cmd
	.global	m7_read_retry_reg_cnt
	.global	m7_read_retry_cycle
	.global	m7_read_retry_mode
	.global	para1
	.global	para0
	.global	m3_lsb_mode_reg_num
	.global	m3_lsb_mode_val
	.global	m3_lsb_mode_default_val
	.global	m3_lsb_mode_reg_adr
	.global	m3_read_retry_val
	.global	m3_read_retry_reg_adr
	.global	m3_read_retry_reg_num
	.global	m3_read_retry_cycle
	.global	m3_read_retry_mode
	.global	m5_p1
	.global	m5_read_retry_cmd
	.global	m5_read_retry_addr
	.global	m5_read_retry_cycle
	.global	m5_read_retry_mode
	.global	nftl_blk_head
	.global	mod
	.global	com
	.section	.rodata
	.align	2
	.set	.LANCHOR2,. + 0
	.type	nand_crc16_table, %object
	.size	nand_crc16_table, 512
nand_crc16_table:
	.short	0
	.short	-16191
	.short	-15999
	.short	320
	.short	-15615
	.short	960
	.short	640
	.short	-15807
	.short	-14847
	.short	1728
	.short	1920
	.short	-14527
	.short	1280
	.short	-14911
	.short	-15231
	.short	1088
	.short	-13311
	.short	3264
	.short	3456
	.short	-12991
	.short	3840
	.short	-12351
	.short	-12671
	.short	3648
	.short	2560
	.short	-13631
	.short	-13439
	.short	2880
	.short	-14079
	.short	2496
	.short	2176
	.short	-14271
	.short	-10239
	.short	6336
	.short	6528
	.short	-9919
	.short	6912
	.short	-9279
	.short	-9599
	.short	6720
	.short	7680
	.short	-8511
	.short	-8319
	.short	8000
	.short	-8959
	.short	7616
	.short	7296
	.short	-9151
	.short	5120
	.short	-11071
	.short	-10879
	.short	5440
	.short	-10495
	.short	6080
	.short	5760
	.short	-10687
	.short	-11775
	.short	4800
	.short	4992
	.short	-11455
	.short	4352
	.short	-11839
	.short	-12159
	.short	4160
	.short	-4095
	.short	12480
	.short	12672
	.short	-3775
	.short	13056
	.short	-3135
	.short	-3455
	.short	12864
	.short	13824
	.short	-2367
	.short	-2175
	.short	14144
	.short	-2815
	.short	13760
	.short	13440
	.short	-3007
	.short	15360
	.short	-831
	.short	-639
	.short	15680
	.short	-255
	.short	16320
	.short	16000
	.short	-447
	.short	-1535
	.short	15040
	.short	15232
	.short	-1215
	.short	14592
	.short	-1599
	.short	-1919
	.short	14400
	.short	10240
	.short	-5951
	.short	-5759
	.short	10560
	.short	-5375
	.short	11200
	.short	10880
	.short	-5567
	.short	-4607
	.short	11968
	.short	12160
	.short	-4287
	.short	11520
	.short	-4671
	.short	-4991
	.short	11328
	.short	-7167
	.short	9408
	.short	9600
	.short	-6847
	.short	9984
	.short	-6207
	.short	-6527
	.short	9792
	.short	8704
	.short	-7487
	.short	-7295
	.short	9024
	.short	-7935
	.short	8640
	.short	8320
	.short	-8127
	.short	-24575
	.short	24768
	.short	24960
	.short	-24255
	.short	25344
	.short	-23615
	.short	-23935
	.short	25152
	.short	26112
	.short	-22847
	.short	-22655
	.short	26432
	.short	-23295
	.short	26048
	.short	25728
	.short	-23487
	.short	27648
	.short	-21311
	.short	-21119
	.short	27968
	.short	-20735
	.short	28608
	.short	28288
	.short	-20927
	.short	-22015
	.short	27328
	.short	27520
	.short	-21695
	.short	26880
	.short	-22079
	.short	-22399
	.short	26688
	.short	30720
	.short	-18239
	.short	-18047
	.short	31040
	.short	-17663
	.short	31680
	.short	31360
	.short	-17855
	.short	-16895
	.short	32448
	.short	32640
	.short	-16575
	.short	32000
	.short	-16959
	.short	-17279
	.short	31808
	.short	-19455
	.short	29888
	.short	30080
	.short	-19135
	.short	30464
	.short	-18495
	.short	-18815
	.short	30272
	.short	29184
	.short	-19775
	.short	-19583
	.short	29504
	.short	-20223
	.short	29120
	.short	28800
	.short	-20415
	.short	20480
	.short	-28479
	.short	-28287
	.short	20800
	.short	-27903
	.short	21440
	.short	21120
	.short	-28095
	.short	-27135
	.short	22208
	.short	22400
	.short	-26815
	.short	21760
	.short	-27199
	.short	-27519
	.short	21568
	.short	-25599
	.short	23744
	.short	23936
	.short	-25279
	.short	24320
	.short	-24639
	.short	-24959
	.short	24128
	.short	23040
	.short	-25919
	.short	-25727
	.short	23360
	.short	-26367
	.short	22976
	.short	22656
	.short	-26559
	.short	-30719
	.short	18624
	.short	18816
	.short	-30399
	.short	19200
	.short	-29759
	.short	-30079
	.short	19008
	.short	19968
	.short	-28991
	.short	-28799
	.short	20288
	.short	-29439
	.short	19904
	.short	19584
	.short	-29631
	.short	17408
	.short	-31551
	.short	-31359
	.short	17728
	.short	-30975
	.short	18368
	.short	18048
	.short	-31167
	.short	-32255
	.short	17088
	.short	17280
	.short	-31935
	.short	16640
	.short	-32319
	.short	-32639
	.short	16448
	.type	__func__.8425, %object
	.size	__func__.8425, 9
__func__.8425:
	.ascii	"init_nrc\000"
	.space	3
.LC1:
	.byte	83
	.byte	83
	.byte	83
	.byte	83
	.byte	83
	.byte	83
	.byte	83
	.byte	83
	.byte	83
	.space	3
	.type	para0, %object
	.size	para0, 56
para0:
	.short	0
	.short	0
	.short	0
	.short	0
	.short	0
	.short	6
	.short	10
	.short	6
	.short	0
	.short	-3
	.short	-7
	.short	-8
	.short	0
	.short	-6
	.short	-13
	.short	-15
	.short	0
	.short	-11
	.short	-20
	.short	-23
	.short	0
	.short	0
	.short	-26
	.short	-30
	.short	0
	.short	0
	.short	-32
	.short	-37
	.type	para1, %object
	.size	para1, 56
para1:
	.short	0
	.short	0
	.short	0
	.short	0
	.short	0
	.short	6
	.short	10
	.short	6
	.short	0
	.short	-3
	.short	-7
	.short	-8
	.short	0
	.short	-6
	.short	-13
	.short	-15
	.short	0
	.short	-9
	.short	-20
	.short	-23
	.short	0
	.short	0
	.short	-26
	.short	-30
	.short	0
	.short	0
	.short	-32
	.short	-37
	.type	__func__.8675, %object
	.size	__func__.8675, 18
__func__.8675:
	.ascii	"nand_dbg_phy_read\000"
	.space	2
	.type	__func__.8688, %object
	.size	__func__.8688, 23
__func__.8688:
	.ascii	"nand_dbg_zone_phy_read\000"
	.space	1
	.type	__func__.8699, %object
	.size	__func__.8699, 24
__func__.8699:
	.ascii	"nand_dbg_zone_phy_write\000"
	.type	__func__.8708, %object
	.size	__func__.8708, 19
__func__.8708:
	.ascii	"nand_dbg_phy_write\000"
	.space	1
	.type	__FUNCTION__.8803, %object
	.size	__FUNCTION__.8803, 25
__FUNCTION__.8803:
	.ascii	"nand_power_off_ecc_error\000"
	.space	3
	.type	__func__.8517, %object
	.size	__func__.8517, 23
__func__.8517:
	.ascii	"torture_block_and_mark\000"
	.space	1
	.type	__func__.8965, %object
	.size	__func__.8965, 19
__func__.8965:
	.ascii	"get_logic_page_map\000"
	.space	1
.LC2:
	.byte	115
	.byte	117
	.byte	110
	.byte	120
	.byte	105
	.space	3
	.type	mod, %object
	.size	mod, 6
mod:
	.byte	109
	.byte	111
	.byte	100
	.byte	101
	.byte	108
	.byte	0
	.space	2
	.type	__func__.8777, %object
	.size	__func__.8777, 19
__func__.8777:
	.ascii	"write_data_to_nand\000"
	.space	1
.LC3:
	.byte	44
	.byte	100
	.byte	68
	.byte	50
	.byte	-91
	.byte	-1
	.byte	-1
	.byte	-1
.LC4:
	.byte	44
	.byte	-124
	.byte	68
	.byte	50
	.byte	-86
	.byte	4
	.byte	0
	.byte	0
.LC0:
	.byte	97
	.byte	108
	.byte	108
	.byte	119
	.byte	105
	.byte	110
	.byte	110
	.byte	101
	.byte	114
	.space	3
	.type	com, %object
	.size	com, 11
com:
	.byte	99
	.byte	111
	.byte	109
	.byte	112
	.byte	97
	.byte	116
	.byte	105
	.byte	98
	.byte	108
	.byte	101
	.byte	0
	.data
	.align	2
	.set	.LANCHOR1,. + 0
	.type	m7_read_retry_disable_cmd_19, %object
	.size	m7_read_retry_disable_cmd_19, 1
m7_read_retry_disable_cmd_19:
	.byte	-42
	.space	3
	.type	m5_read_retry_cmd, %object
	.size	m5_read_retry_cmd, 4
m5_read_retry_cmd:
	.byte	-95
	.byte	-95
	.byte	-95
	.byte	-95
	.type	m5_read_retry_addr, %object
	.size	m5_read_retry_addr, 4
m5_read_retry_addr:
	.byte	-89
	.byte	-92
	.byte	-91
	.byte	-90
	.type	m5_p1, %object
	.size	m5_p1, 60
m5_p1:
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	5
	.byte	10
	.byte	0
	.byte	0
	.byte	40
	.byte	0
	.byte	-20
	.byte	-40
	.byte	-19
	.byte	-11
	.byte	-19
	.byte	-26
	.byte	10
	.byte	15
	.byte	5
	.byte	0
	.byte	15
	.byte	10
	.byte	-5
	.byte	-20
	.byte	-24
	.byte	-17
	.byte	-24
	.byte	-36
	.byte	-15
	.byte	-5
	.byte	-2
	.byte	-16
	.byte	10
	.byte	0
	.byte	-5
	.byte	-20
	.byte	-48
	.byte	-30
	.byte	-48
	.byte	-62
	.byte	20
	.byte	15
	.byte	-5
	.byte	-20
	.byte	-24
	.byte	-5
	.byte	-24
	.byte	-36
	.byte	30
	.byte	20
	.byte	-5
	.byte	-20
	.byte	-5
	.byte	-1
	.byte	-5
	.byte	-8
	.byte	7
	.byte	12
	.byte	2
	.byte	0
	.type	m7_acti_start_cmd, %object
	.size	m7_acti_start_cmd, 2
m7_acti_start_cmd:
	.byte	59
	.byte	-71
	.space	2
	.type	m7_acti_start_addr, %object
	.size	m7_acti_start_addr, 9
m7_acti_start_addr:
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.byte	8
	.byte	9
	.byte	10
	.byte	11
	.byte	12
	.space	3
	.type	m7_read_retry_cmd_19, %object
	.size	m7_read_retry_cmd_19, 3
m7_read_retry_cmd_19:
	.byte	83
	.byte	83
	.byte	83
	.space	1
	.type	m7_read_retry_cmd_1y, %object
	.size	m7_read_retry_cmd_1y, 1
m7_read_retry_cmd_1y:
	.byte	-17
	.space	3
	.type	m7_19_low, %object
	.size	m7_19_low, 48
m7_19_low:
	.byte	-16
	.byte	0
	.byte	-16
	.byte	-32
	.byte	0
	.byte	-32
	.byte	-48
	.byte	0
	.byte	-48
	.byte	16
	.byte	0
	.byte	16
	.byte	32
	.byte	0
	.byte	32
	.byte	48
	.byte	0
	.byte	48
	.byte	-64
	.byte	0
	.byte	-48
	.byte	0
	.byte	0
	.byte	16
	.byte	0
	.byte	0
	.byte	32
	.byte	16
	.byte	0
	.byte	32
	.byte	-80
	.byte	0
	.byte	-48
	.byte	-96
	.byte	0
	.byte	-48
	.byte	-112
	.byte	0
	.byte	-48
	.byte	-80
	.byte	0
	.byte	-64
	.byte	-96
	.byte	0
	.byte	-64
	.byte	-112
	.byte	0
	.byte	-64
	.type	m7_19_high, %object
	.size	m7_19_high, 60
m7_19_high:
	.byte	0
	.byte	-16
	.byte	0
	.byte	15
	.byte	-32
	.byte	0
	.byte	15
	.byte	-48
	.byte	0
	.byte	14
	.byte	-32
	.byte	0
	.byte	14
	.byte	-48
	.byte	0
	.byte	13
	.byte	-16
	.byte	0
	.byte	13
	.byte	-32
	.byte	0
	.byte	13
	.byte	-48
	.byte	0
	.byte	1
	.byte	16
	.byte	0
	.byte	2
	.byte	32
	.byte	0
	.byte	2
	.byte	16
	.byte	0
	.byte	3
	.byte	32
	.byte	0
	.byte	15
	.byte	0
	.byte	0
	.byte	14
	.byte	-16
	.byte	0
	.byte	13
	.byte	-64
	.byte	0
	.byte	15
	.byte	-16
	.byte	0
	.byte	1
	.byte	0
	.byte	0
	.byte	2
	.byte	0
	.byte	0
	.byte	13
	.byte	-80
	.byte	0
	.byte	12
	.byte	-96
	.byte	0
	.type	m7_read_retry_reg_adr_19, %object
	.size	m7_read_retry_reg_adr_19, 9
m7_read_retry_reg_adr_19:
	.byte	4
	.byte	5
	.byte	7
	.space	6
	.space	3
	.type	m7_read_retry_reg_adr_24, %object
	.size	m7_read_retry_reg_adr_24, 9
m7_read_retry_reg_adr_24:
	.byte	4
	.byte	5
	.byte	7
	.space	6
	.space	3
	.type	m7_24, %object
	.size	m7_24, 27
m7_24:
	.byte	0
	.byte	-16
	.byte	0
	.byte	0
	.byte	-32
	.byte	0
	.byte	-1
	.byte	-16
	.byte	-16
	.byte	-18
	.byte	-32
	.byte	-32
	.byte	-34
	.byte	-48
	.byte	-48
	.byte	-51
	.byte	-64
	.byte	-64
	.byte	1
	.byte	0
	.byte	0
	.byte	2
	.byte	0
	.byte	0
	.byte	3
	.byte	0
	.byte	0
	.space	1
	.type	m7_read_retry_reg_adr_1y, %object
	.size	m7_read_retry_reg_adr_1y, 1
m7_read_retry_reg_adr_1y:
	.byte	17
	.space	3
	.type	m7_1y, %object
	.size	m7_1y, 76
m7_1y:
	.byte	4
	.byte	0
	.byte	124
	.byte	124
	.byte	8
	.byte	0
	.byte	4
	.byte	4
	.byte	124
	.byte	124
	.byte	0
	.byte	124
	.byte	124
	.byte	124
	.byte	0
	.byte	0
	.byte	12
	.byte	0
	.byte	120
	.byte	120
	.byte	16
	.byte	0
	.byte	8
	.byte	8
	.byte	124
	.byte	124
	.byte	120
	.byte	120
	.byte	124
	.byte	124
	.byte	4
	.byte	4
	.byte	124
	.byte	124
	.byte	120
	.byte	116
	.byte	20
	.byte	4
	.byte	4
	.byte	0
	.byte	120
	.byte	120
	.byte	0
	.byte	124
	.byte	120
	.byte	120
	.byte	120
	.byte	120
	.byte	120
	.byte	120
	.byte	120
	.byte	116
	.byte	120
	.byte	120
	.byte	4
	.byte	0
	.byte	120
	.byte	116
	.byte	120
	.byte	116
	.byte	120
	.byte	116
	.byte	116
	.byte	112
	.byte	120
	.byte	116
	.byte	120
	.byte	120
	.byte	120
	.byte	112
	.byte	120
	.byte	116
	.byte	120
	.byte	112
	.byte	112
	.byte	112
	.type	m6_read_retry_start_cmd, %object
	.size	m6_read_retry_start_cmd, 2
m6_read_retry_start_cmd:
	.byte	92
	.byte	-59
	.space	2
	.type	m6_read_retry_cmd_2, %object
	.size	m6_read_retry_cmd_2, 5
m6_read_retry_cmd_2:
	.byte	85
	.byte	85
	.byte	85
	.byte	85
	.byte	85
	.space	3
	.type	m6_read_retry_exit_cmd, %object
	.size	m6_read_retry_exit_cmd, 1
m6_read_retry_exit_cmd:
	.byte	-1
	.space	3
	.type	m6_read_retry_last_cmd, %object
	.size	m6_read_retry_last_cmd, 1
m6_read_retry_last_cmd:
	.byte	-77
	.space	3
	.type	m6_read_retry_cmd_1, %object
	.size	m6_read_retry_cmd_1, 4
m6_read_retry_cmd_1:
	.byte	85
	.byte	85
	.byte	85
	.byte	85
	.type	m6_read_retry_end_cmd, %object
	.size	m6_read_retry_end_cmd, 2
m6_read_retry_end_cmd:
	.byte	38
	.byte	93
	.space	2
	.type	m6_read_retry_addr_2, %object
	.size	m6_read_retry_addr_2, 5
m6_read_retry_addr_2:
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.byte	13
	.space	3
	.type	m6_read_retry_addr_1, %object
	.size	m6_read_retry_addr_1, 4
m6_read_retry_addr_1:
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.type	m6_p1_2, %object
	.size	m6_p1_2, 40
m6_p1_2:
	.byte	4
	.byte	4
	.byte	124
	.byte	126
	.byte	0
	.byte	0
	.byte	124
	.byte	120
	.byte	120
	.byte	0
	.byte	124
	.byte	118
	.byte	116
	.byte	114
	.byte	0
	.byte	8
	.byte	8
	.byte	0
	.byte	0
	.byte	0
	.byte	11
	.byte	126
	.byte	118
	.byte	116
	.byte	0
	.byte	16
	.byte	118
	.byte	114
	.byte	112
	.byte	0
	.byte	2
	.byte	0
	.byte	126
	.byte	124
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.type	m6_p1_3, %object
	.size	m6_p1_3, 55
m6_p1_3:
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	2
	.byte	4
	.byte	2
	.byte	0
	.byte	0
	.byte	124
	.byte	0
	.byte	124
	.byte	124
	.byte	0
	.byte	122
	.byte	0
	.byte	122
	.byte	122
	.byte	0
	.byte	120
	.byte	2
	.byte	120
	.byte	122
	.byte	0
	.byte	126
	.byte	4
	.byte	126
	.byte	122
	.byte	0
	.byte	118
	.byte	4
	.byte	118
	.byte	120
	.byte	0
	.byte	4
	.byte	4
	.byte	4
	.byte	118
	.byte	0
	.byte	6
	.byte	10
	.byte	6
	.byte	2
	.byte	0
	.byte	116
	.byte	124
	.byte	116
	.byte	118
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.space	1
	.type	m6_p1_1, %object
	.size	m6_p1_1, 28
m6_p1_1:
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	4
	.byte	4
	.byte	4
	.byte	4
	.byte	124
	.byte	124
	.byte	124
	.byte	124
	.byte	120
	.byte	120
	.byte	120
	.byte	120
	.byte	116
	.byte	116
	.byte	116
	.byte	116
	.byte	8
	.byte	8
	.byte	8
	.byte	8
	.byte	-1
	.byte	-1
	.byte	-1
	.byte	-1
	.type	m4_p1_2, %object
	.size	m4_p1_2, 12
m4_p1_2:
	.byte	0
	.byte	0
	.byte	0
	.byte	1
	.byte	2
	.byte	3
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.byte	8
	.byte	12
	.type	m4_p1_1, %object
	.size	m4_p1_1, 10
m4_p1_1:
	.byte	0
	.byte	0
	.byte	0
	.byte	1
	.byte	2
	.byte	3
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.space	2
	.type	m4_p1_3, %object
	.size	m4_p1_3, 7
m4_p1_3:
	.byte	1
	.byte	2
	.byte	3
	.byte	0
	.byte	1
	.byte	2
	.byte	3
	.space	1
	.type	m9_read_retry_cmd_1y, %object
	.size	m9_read_retry_cmd_1y, 1
m9_read_retry_cmd_1y:
	.byte	-17
	.space	3
	.type	m9_1z_16g, %object
	.size	m9_1z_16g, 128
m9_1z_16g:
	.byte	124
	.byte	0
	.byte	4
	.byte	0
	.byte	4
	.byte	0
	.byte	0
	.byte	124
	.byte	120
	.byte	0
	.byte	8
	.byte	4
	.byte	8
	.byte	0
	.byte	124
	.byte	120
	.byte	124
	.byte	124
	.byte	4
	.byte	0
	.byte	0
	.byte	124
	.byte	0
	.byte	124
	.byte	120
	.byte	124
	.byte	8
	.byte	4
	.byte	4
	.byte	124
	.byte	116
	.byte	120
	.byte	4
	.byte	4
	.byte	4
	.byte	0
	.byte	8
	.byte	4
	.byte	0
	.byte	124
	.byte	0
	.byte	4
	.byte	8
	.byte	4
	.byte	12
	.byte	4
	.byte	124
	.byte	120
	.byte	124
	.byte	4
	.byte	12
	.byte	8
	.byte	120
	.byte	120
	.byte	4
	.byte	0
	.byte	124
	.byte	120
	.byte	0
	.byte	124
	.byte	116
	.byte	120
	.byte	112
	.byte	120
	.byte	8
	.byte	8
	.byte	4
	.byte	0
	.byte	4
	.byte	8
	.byte	8
	.byte	4
	.byte	12
	.byte	8
	.byte	12
	.byte	8
	.byte	0
	.byte	0
	.byte	12
	.byte	8
	.byte	4
	.byte	0
	.byte	120
	.byte	116
	.byte	124
	.byte	124
	.byte	120
	.byte	116
	.byte	120
	.byte	120
	.byte	120
	.byte	116
	.byte	8
	.byte	8
	.byte	0
	.byte	124
	.byte	116
	.byte	116
	.byte	0
	.byte	124
	.byte	120
	.byte	116
	.byte	124
	.byte	120
	.byte	112
	.byte	116
	.byte	116
	.byte	116
	.byte	124
	.byte	116
	.byte	112
	.byte	112
	.byte	12
	.byte	12
	.byte	8
	.byte	4
	.byte	16
	.byte	12
	.byte	12
	.byte	8
	.byte	112
	.byte	112
	.byte	112
	.byte	116
	.byte	116
	.byte	112
	.byte	112
	.byte	112
	.type	m9_1z_8g, %object
	.size	m9_1z_8g, 124
m9_1z_8g:
	.byte	4
	.byte	4
	.byte	120
	.byte	120
	.byte	4
	.byte	4
	.byte	124
	.byte	116
	.byte	0
	.byte	0
	.byte	4
	.byte	120
	.byte	4
	.byte	4
	.byte	124
	.byte	124
	.byte	0
	.byte	0
	.byte	0
	.byte	124
	.byte	0
	.byte	0
	.byte	0
	.byte	116
	.byte	8
	.byte	8
	.byte	4
	.byte	120
	.byte	124
	.byte	124
	.byte	4
	.byte	120
	.byte	124
	.byte	124
	.byte	0
	.byte	124
	.byte	4
	.byte	4
	.byte	124
	.byte	112
	.byte	124
	.byte	124
	.byte	116
	.byte	116
	.byte	0
	.byte	0
	.byte	120
	.byte	112
	.byte	12
	.byte	12
	.byte	8
	.byte	120
	.byte	120
	.byte	120
	.byte	124
	.byte	124
	.byte	4
	.byte	4
	.byte	8
	.byte	4
	.byte	120
	.byte	120
	.byte	8
	.byte	120
	.byte	124
	.byte	124
	.byte	120
	.byte	112
	.byte	120
	.byte	120
	.byte	112
	.byte	108
	.byte	0
	.byte	0
	.byte	116
	.byte	108
	.byte	8
	.byte	8
	.byte	0
	.byte	116
	.byte	124
	.byte	124
	.byte	120
	.byte	108
	.byte	0
	.byte	0
	.byte	4
	.byte	4
	.byte	116
	.byte	116
	.byte	116
	.byte	108
	.byte	120
	.byte	120
	.byte	124
	.byte	112
	.byte	12
	.byte	12
	.byte	0
	.byte	116
	.byte	4
	.byte	4
	.byte	12
	.byte	8
	.byte	120
	.byte	120
	.byte	124
	.byte	116
	.byte	120
	.byte	120
	.byte	112
	.byte	104
	.byte	8
	.byte	8
	.byte	0
	.byte	112
	.byte	16
	.byte	16
	.byte	12
	.byte	120
	.byte	0
	.byte	0
	.byte	12
	.byte	8
	.type	m9_1y, %object
	.size	m9_1y, 124
m9_1y:
	.byte	124
	.byte	0
	.byte	0
	.byte	124
	.byte	4
	.byte	0
	.byte	124
	.byte	120
	.byte	120
	.byte	0
	.byte	120
	.byte	116
	.byte	8
	.byte	124
	.byte	0
	.byte	124
	.byte	0
	.byte	124
	.byte	124
	.byte	120
	.byte	124
	.byte	124
	.byte	120
	.byte	116
	.byte	0
	.byte	124
	.byte	116
	.byte	112
	.byte	0
	.byte	120
	.byte	0
	.byte	124
	.byte	0
	.byte	120
	.byte	124
	.byte	120
	.byte	0
	.byte	120
	.byte	120
	.byte	116
	.byte	0
	.byte	120
	.byte	116
	.byte	112
	.byte	0
	.byte	120
	.byte	112
	.byte	108
	.byte	0
	.byte	4
	.byte	4
	.byte	0
	.byte	0
	.byte	4
	.byte	0
	.byte	124
	.byte	12
	.byte	4
	.byte	124
	.byte	120
	.byte	12
	.byte	4
	.byte	120
	.byte	116
	.byte	16
	.byte	8
	.byte	0
	.byte	124
	.byte	16
	.byte	8
	.byte	4
	.byte	0
	.byte	120
	.byte	116
	.byte	120
	.byte	116
	.byte	120
	.byte	116
	.byte	116
	.byte	112
	.byte	120
	.byte	116
	.byte	112
	.byte	108
	.byte	120
	.byte	116
	.byte	108
	.byte	104
	.byte	120
	.byte	112
	.byte	120
	.byte	116
	.byte	120
	.byte	112
	.byte	116
	.byte	112
	.byte	120
	.byte	112
	.byte	108
	.byte	104
	.byte	120
	.byte	112
	.byte	112
	.byte	108
	.byte	120
	.byte	108
	.byte	112
	.byte	108
	.byte	120
	.byte	108
	.byte	108
	.byte	104
	.byte	120
	.byte	108
	.byte	104
	.byte	100
	.byte	116
	.byte	104
	.byte	108
	.byte	104
	.byte	116
	.byte	104
	.byte	104
	.byte	100
	.type	m9_read_retry_reg_adr_1y, %object
	.size	m9_read_retry_reg_adr_1y, 1
m9_read_retry_reg_adr_1y:
	.byte	17
	.space	3
	.type	m9_read_retry_exit_cmd_1y, %object
	.size	m9_read_retry_exit_cmd_1y, 1
m9_read_retry_exit_cmd_1y:
	.byte	-1
	.space	3
	.type	m9_read_retry_enable_cmd_1y, %object
	.size	m9_read_retry_enable_cmd_1y, 1
m9_read_retry_enable_cmd_1y:
	.byte	93
	.space	3
	.type	m2_read_retry_reg_adr_2, %object
	.size	m2_read_retry_reg_adr_2, 8
m2_read_retry_reg_adr_2:
	.byte	-80
	.byte	-79
	.byte	-78
	.byte	-77
	.byte	-76
	.byte	-75
	.byte	-74
	.byte	-73
	.type	m2_read_retry_reg_adr_1, %object
	.size	m2_read_retry_reg_adr_1, 8
m2_read_retry_reg_adr_1:
	.byte	-52
	.byte	-65
	.byte	-86
	.byte	-85
	.byte	-51
	.byte	-83
	.byte	-82
	.byte	-81
	.type	m7_read_retry_exit_cmd_1y, %object
	.size	m7_read_retry_exit_cmd_1y, 1
m7_read_retry_exit_cmd_1y:
	.byte	-1
	.space	3
	.type	m7_read_retry_enable_cmd_1y, %object
	.size	m7_read_retry_enable_cmd_1y, 1
m7_read_retry_enable_cmd_1y:
	.byte	93
	.space	3
	.type	m7_read_retry_cmd_24, %object
	.size	m7_read_retry_cmd_24, 3
m7_read_retry_cmd_24:
	.byte	83
	.byte	83
	.byte	83
	.space	1
	.type	m7_read_retry_enable_cmd_19, %object
	.size	m7_read_retry_enable_cmd_19, 1
m7_read_retry_enable_cmd_19:
	.byte	-74
	.section	.rodata.str1.4,"aMS",%progbits,1
	.align	2
.LC5:
	.ascii	"[NE]free list: \012\000"
	.space	3
.LC6:
	.ascii	"[NE]block NO:%4d; erase:%3d; \000"
	.space	2
.LC7:
	.ascii	"count:%4d; \000"
.LC8:
	.ascii	"info:0x%x\012\000"
	.space	1
.LC9:
	.ascii	"[NE]invalid_page_count list: \012\000"
	.space	1
.LC10:
	.ascii	"count:%4d; invalid:%3d;\000"
.LC11:
	.ascii	"[ND]swl: static WL need to do(%d : %d)\012\000"
.LC12:
	.ascii	"[ND]swl: min_ec_blk_no = %d,\012\000"
	.space	2
.LC13:
	.ascii	"[ND]swl: zone->blocks = %d)\012\000"
	.space	3
.LC14:
	.ascii	"[NE]nand_cache_read error  NULL!\012\000"
	.space	2
.LC15:
	.ascii	"[NE]error1 node %d,%d,%d!\012\000"
	.space	1
.LC16:
	.ascii	"[NE]smart->version:%x;\012\000"
.LC17:
	.ascii	"[NE]smart->total_recv_read_sectors:%lld;\012\000"
	.space	2
.LC18:
	.ascii	"[NE]smart->total_recv_write_sectors:%lld;\012\000"
	.space	1
.LC19:
	.ascii	"[NE]smart->total_real_read_pages:%lld;\012\000"
.LC20:
	.ascii	"[NE]smart->total_real_write_pages:%lld;\012\000"
	.space	3
.LC21:
	.ascii	"[NE]smart->total_recv_discard_sectors:%lld;\012\000"
	.space	3
.LC22:
	.ascii	"[NE]smart->total_real_discard_sectors:%lld;\012\000"
	.space	3
.LC23:
	.ascii	"[NE]smart->total_recv_read_claim_pages:%d;\012\000"
.LC24:
	.ascii	"[NE]smart->total_gc_times:%d;\012\000"
	.space	1
.LC25:
	.ascii	"[NE]smart->total_gc_pages:%d;\012\000"
	.space	1
.LC26:
	.ascii	"[NE]smart->total_wl_times:%d;\012\000"
	.space	1
.LC27:
	.ascii	"[NE]smart->total_wl_gc_times:%d;\012\000"
	.space	2
.LC28:
	.ascii	"[NE]smart->total_real_read_error_pages:%d;\012\000"
.LC29:
	.ascii	"[NE]smart->total_real_write_error_pages:%d;\012\000"
	.space	3
.LC30:
	.ascii	"[NE]smart->total_normal_power_cycles:%x;\012\000"
	.space	2
.LC31:
	.ascii	"[NE]smart->total_unusual_power_cycles:%x;\012\000"
	.space	1
.LC32:
	.ascii	"[NE]smart->max_block_erase_times:%d;\012\000"
	.space	2
.LC33:
	.ascii	"[NE]smart->min_block_erase_times:%d;\012\000"
	.space	2
.LC34:
	.ascii	"[NE]smart->max_block_used_counter:%d;\012\000"
	.space	1
.LC35:
	.ascii	"[NE]smart->min_block_used_counter:%d;\012\000"
	.space	1
.LC36:
	.ascii	"[NE]smart->read_reclaim_utc:%d;\012\000"
	.space	3
.LC37:
	.ascii	"[NE]smart->cross_talk_times:%d;\012\000"
	.space	3
.LC38:
	.ascii	"[NE]smart->read_reclaim_zone_no:%d;\012\000"
	.space	3
.LC39:
	.ascii	"[NE]smart->read_reclaim_page_no:%d;\012\000"
	.space	3
.LC40:
	.ascii	"[ND]nand3.x \012\000"
	.space	2
.LC41:
	.ascii	"[ND]zone->nand_chip->blk_per_chip: %d \012\000"
.LC42:
	.ascii	"[ND]zone->nand_chip->bytes_per_page: %d \012\000"
	.space	2
.LC43:
	.ascii	"[ND]zone->nand_chip->pages_per_blk: %d \012\000"
	.space	3
.LC44:
	.ascii	"[ND]zone->nand_chip->max_erase_times: %d \012\000"
	.space	1
.LC45:
	.ascii	"[ND]zone->nand_chip->support_read_reclaim: %d \012\000"
.LC46:
	.ascii	"[ND]zone->test: %d \012\000"
	.space	3
.LC47:
	.ascii	"[ND]zone->zone_no: %d \012\000"
.LC48:
	.ascii	"[ND]zone->zone_attr: %d \012\000"
	.space	2
.LC49:
	.ascii	"[ND]zone->blocks: %d \012\000"
	.space	1
.LC50:
	.ascii	"[ND]zone->bad_block: %d \012\000"
	.space	2
.LC51:
	.ascii	"[ND]zone->logic_cap_in_sects: %d \012\000"
	.space	1
.LC52:
	.ascii	"[ND]zone->backup_cap_in_sects: %d \012\000"
.LC53:
	.ascii	"[ND]zone->free_block_num: %d \012\000"
	.space	1
.LC54:
	.ascii	"[ND]zone->gc_strategy.start_gc_free_blocks: %d \012"
	.ascii	"\000"
	.space	3
.LC55:
	.ascii	"[ND]zone->gc_strategy.stop_gc_free_blocks: %d \012\000"
.LC56:
	.ascii	"[ND]zone->gc_strategy.gc_page: %d \012\000"
.LC57:
	.ascii	"[ND]zone->gc_strategy.process: %d \012\000"
.LC58:
	.ascii	"[ND]zone->prio_gc.prio_type : %d \012\000"
	.space	1
.LC59:
	.ascii	"[ND]zone->zone_start_phy_block->phy_block.Block_NO:"
	.ascii	" %d \012\000"
	.space	3
.LC60:
	.ascii	"[ND]zone->zone_end_phy_block->phy_block.Block_NO: %"
	.ascii	"d \012\000"
	.space	1
.LC61:
	.ascii	"[ND]zone->zone_phy_page_map_for_gc: %x \012\000"
	.space	3
.LC62:
	.ascii	"[ND]zone->current_block.user_info: %x \012\000"
.LC63:
	.ascii	"[ND]zone->current_block.block_info: %x \012\000"
	.space	3
.LC64:
	.ascii	"[ND]zone->current_block.block_info->phy_block.Block"
	.ascii	"_NO: %d \012\000"
	.space	3
.LC65:
	.ascii	"[ND]zone->current_block.page_used: %d \012\000"
.LC66:
	.ascii	"[ND]zone->current_block.block_info->block_used_coun"
	.ascii	"t: %d \012\000"
	.space	1
.LC67:
	.ascii	"[NE] zone->recv_free_block: %d %d!!\012\000"
	.space	3
.LC68:
	.ascii	"[ND]zone->assist_block.user_info: %x \012\000"
	.space	1
.LC69:
	.ascii	"[ND]zone->assist_block.block_info: %x \012\000"
.LC70:
	.ascii	"[ND]zone->assist_block.block_info->phy_block.Block_"
	.ascii	"NO: %d \012\000"
.LC71:
	.ascii	"[ND]zone->assist_block.page_used: %d \012\000"
	.space	1
.LC72:
	.ascii	"[ND]zone->assist_block.block_info->block_used_count"
	.ascii	": %d \012\000"
	.space	2
.LC73:
	.ascii	"[ND]zone->read_reclaim_complete: %x \012\000"
	.space	2
.LC74:
	.ascii	"[ND]zone->temp_page_buf: %x \012\000"
	.space	2
.LC75:
	.ascii	"[ND]zone->max_erase_num: %d \012\000"
	.space	2
.LC76:
	.ascii	"[ND]zone->cache.cache_totals: %x \012\000"
	.space	1
.LC77:
	.ascii	"[ND]zone->wl_gc_count: %d \012\000"
.LC78:
	.ascii	"[ND]zone->wl_gc_page_num: %d \012\000"
	.space	1
.LC79:
	.ascii	"[ND]zone->cfg->nftl_dont_use_cache: %x \012\000"
	.space	3
.LC80:
	.ascii	"[ND]zone->cfg->nftl_use_cache_sort: %x \012\000"
	.space	3
.LC81:
	.ascii	"[ND]zone->cfg->nftl_support_gc_read_reclaim: %x \012"
	.ascii	"\000"
	.space	2
.LC82:
	.ascii	"[ND]zone->cfg->nftl_support_wear_leveling: %x \012\000"
.LC83:
	.ascii	"[ND]zone->cfg->nftl_need_erase: %x \012\000"
	.space	3
.LC84:
	.ascii	"[ND]zone->cfg->nftl_min_free_block_num: %x \012\000"
	.space	3
.LC85:
	.ascii	"[ND]zone->cfg->nftl_gc_threshold_free_block_num: %x"
	.ascii	" \012\000"
	.space	2
.LC86:
	.ascii	"[ND]zone->cfg->nftl_min_free_block: %x \012\000"
	.space	3
.LC87:
	.ascii	"[ND]zone->cfg->nftl_gc_threshold_ratio_numerator: %"
	.ascii	"x \012\000"
	.space	1
.LC88:
	.ascii	"[ND]zone->cfg->nftl_gc_threshold_ratio_denominator:"
	.ascii	" %x \012\000"
	.space	3
.LC89:
	.ascii	"[ND]zone->cfg->nftl_max_cache_num: %d \012\000"
.LC90:
	.ascii	"[ND]zone->cfg->nftl_max_cache_write_num: %d \012\000"
	.space	2
.LC91:
	.ascii	"[ND]zone->cfg->nftl_cross_talk: %x \012\000"
	.space	3
.LC92:
	.ascii	"[ND]zone->cfg->nftl_read_claim_interval: %d \012\000"
	.space	2
.LC93:
	.ascii	"exception case wb %04x %04x, len: %d\012\000"
	.space	2
.LC94:
	.ascii	"[ND]cap: 0x%08x.\012\000"
	.space	2
.LC95:
	.ascii	"[NE]parameter error %d,%d !\012\000"
	.space	3
.LC96:
	.ascii	"[NE]malloc size too large %d!\012\000"
	.space	1
.LC97:
	.ascii	"%s %dfail 1\012\000"
	.space	3
.LC98:
	.ascii	"%s %dfail 2\012\000"
	.space	3
.LC99:
	.ascii	"%s %dfail 3\012\000"
	.space	3
.LC100:
	.ascii	"%s %dfail 4\012\000"
	.space	3
.LC101:
	.ascii	"pages_per_blk: %d, %p, %p, %p, %p\012\000"
	.space	1
.LC102:
	.ascii	"samsung read retry conut: %d !\012\000"
.LC103:
	.ascii	"rr value %x %x %x %x!\012\000"
	.space	1
.LC104:
	.ascii	"m5 set readretry error ! %x \012\000"
	.space	2
.LC105:
	.ascii	"m5 retry!\012\000"
	.space	1
.LC106:
	.ascii	"m5 ReadRetry ok! ch =%d, chip = %d  block = %d, pag"
	.ascii	"e = %d, RetryCount = %d  \012\000"
	.space	2
.LC107:
	.ascii	" m5_special_init m5_read_retry_mode:%d m5_read_retr"
	.ascii	"y_cycle :%d \012\000"
	.space	3
.LC108:
	.ascii	" m5_special_init error m5_read_retry_mode:%d m5_rea"
	.ascii	"d_retry_cycle :%d \012\000"
	.space	1
.LC109:
	.ascii	" m5_special_exit \012\000"
	.space	1
.LC110:
	.ascii	" m0 special init \012\000"
	.space	1
.LC111:
	.ascii	" m0 special exit \012\000"
	.space	1
.LC112:
	.ascii	"m3_lsb_exit\012\000"
	.space	3
.LC113:
	.ascii	"m7 activation seq fail 0x%x\012\000"
	.space	3
.LC114:
	.ascii	"sandisk read retry conut: %d !\012\000"
.LC115:
	.ascii	"m7 retry!\012\000"
	.space	1
.LC116:
	.ascii	"m7 ReadRetry ok! ch =%d, chip = %d  block = %d, pag"
	.ascii	"e = %d, RetryCount = %d  \012\000"
	.space	2
.LC117:
	.ascii	"retry spare all 0xff! ch =%d, chip = %d  block = %d"
	.ascii	", page = %d, RetryCount = %d  \012\000"
	.space	1
.LC118:
	.ascii	" m7_special_init m7_read_retry_mode:%d m7_read_retr"
	.ascii	"y_cycle :%d m7_read_retry_reg_cnt %d \012\000"
	.space	2
.LC119:
	.ascii	" m7_special_init error m7_read_retry_mode:%d m7_rea"
	.ascii	"d_retry_cycle :%d m7_read_retry_reg_cnt %d \012\000"
.LC120:
	.ascii	" m7_special_exit \012\000"
	.space	1
.LC121:
	.ascii	"toshiba read retry conut: %d !\012\000"
.LC122:
	.ascii	"toshiba vender_pre_condition error!\012\000"
	.space	3
.LC123:
	.ascii	"m6 rr value 3 %x %x %x %x!\012\000"
.LC124:
	.ascii	"m6 retry!\012\000"
	.space	1
.LC125:
	.ascii	"m6 ReadRetry ok! ch =%d, chip = %d  block = %d, pag"
	.ascii	"e = %d, RetryCount = %d  \012\000"
	.space	2
.LC126:
	.ascii	" m6_special_init m6_read_retry_mode :%d m6_read_ret"
	.ascii	"ry_cycle :%d m6_read_retry_reg_cnt :%d \012\000"
.LC127:
	.ascii	" m6_special_init error m6_read_retry_mode :%d m6_re"
	.ascii	"ad_retry_cycle :%d m6_read_retry_reg_cnt :%d \012\000"
	.space	2
.LC128:
	.ascii	" m6_special_exit \012\000"
	.space	1
.LC129:
	.ascii	"fatal err -0, wrong input parameter, ch: %d  chip: "
	.ascii	"%d/%d  block: %d/%d \012\000"
	.space	3
.LC130:
	.ascii	"m1 write page FF wrong\012\000"
.LC131:
	.ascii	"m3 retry param:\000"
.LC132:
	.ascii	"\012\000"
	.space	2
.LC133:
	.ascii	" %x \000"
	.space	3
.LC134:
	.ascii	"m0 retry!\012\000"
	.space	1
.LC135:
	.ascii	"m3 ReadRetry ok! ch =%d, chip = %d  block = %d, pag"
	.ascii	"e = %d, RetryCount = %d  \012\000"
	.space	2
.LC136:
	.ascii	"set retry default value: \000"
	.space	2
.LC137:
	.ascii	" %x\000"
.LC138:
	.ascii	"m3_get_default_param: ch: %d, chip: %d, value: 0x%x"
	.ascii	" 0x%x 0x%x 0x%x \012\000"
	.space	3
.LC139:
	.ascii	"NFC_ReadRetryInit, unknown read retry mode 0x%x\012"
	.ascii	"\000"
	.space	3
.LC140:
	.ascii	" m3_special_init m3_read_retry_mode: %d m3_read_ret"
	.ascii	"ry_cycle:%d m3_read_retry_reg_num:%d\012\000"
	.space	3
.LC141:
	.ascii	" m3_special_init error m3_read_retry_mode: %d m3_re"
	.ascii	"ad_retry_cycle:%d m3_read_retry_reg_num:%d\012\000"
	.space	1
.LC142:
	.ascii	"m3_readretry_exit. \012\000"
	.space	3
.LC143:
	.ascii	" m3_special_exit \012\000"
	.space	1
.LC144:
	.ascii	"_major_check_byte, wrong input para, level %d, len "
	.ascii	"%d\012\000"
	.space	1
.LC145:
	.ascii	"%d:  '1'-'0' : %d - %d\012\000"
.LC146:
	.ascii	"get bit %d failed!\012\000"
.LC147:
	.ascii	"_get_read_retry_parameters, get rr count failed!\012"
	.ascii	"\000"
	.space	2
.LC148:
	.ascii	"rr cnt: %d\012\000"
.LC149:
	.ascii	"_get_read_retry_parameters, get rr reg count failed"
	.ascii	"!\012\000"
	.space	2
.LC150:
	.ascii	"rr reg cnt: %d\012\000"
.LC151:
	.ascii	"read retry value from otp error: rr_cnt %d rr_reg_c"
	.ascii	"nt %d!\012\000"
	.space	1
.LC152:
	.ascii	"_get_read_retry_parameters, get the %d-th byte of r"
	.ascii	"r table failed!\012\000"
.LC153:
	.ascii	"%02x \000"
	.space	2
.LC154:
	.ascii	"start get read retry param from: ce %d, rb %d...\012"
	.ascii	"\000"
	.space	2
.LC155:
	.ascii	"invalid buffer for otp info!\012\000"
	.space	2
.LC156:
	.ascii	"hynix OTP RegCount value error: 0x%x, 0x%x \012\000"
	.space	3
.LC157:
	.ascii	"otp_info_hynix_16nm : allocate memory fail\012\000"
.LC158:
	.ascii	"m1 CH %d chip %d get otp info failed!\012\000"
	.space	1
.LC159:
	.ascii	"m1 CH %d chip %d get read retry cfg from otp info f"
	.ascii	"ailed!\012\000"
	.space	1
.LC160:
	.ascii	"_vender_get_param_otp_hynix time %d!\012\000"
	.space	2
.LC161:
	.ascii	"[PHY_GetDefaultParam]:data_ff_buf malloc fail\012\000"
	.space	1
.LC162:
	.ascii	"nctrl: %d, chip %d, block %d, page 0, oob: 0x%x, 0x"
	.ascii	"%x, 0x%x, 0x%x\012\000"
	.space	1
.LC163:
	.ascii	"otp data check error!\012\000"
	.space	1
.LC164:
	.ascii	"0x%x \000"
	.space	2
.LC165:
	.ascii	"erase chip %d, block %d error\012\000"
	.space	1
.LC166:
	.ascii	"write chip %d, block %d, page %d error\012\000"
.LC167:
	.ascii	"[PHY_DBG] repair ch %d chip %d otp value end\012\000"
	.space	2
.LC168:
	.ascii	"find good otp value in ch: %d, chip %d, block %d \012"
	.ascii	"\000"
	.space	1
.LC169:
	.ascii	"ch %d, chip %d Read Retry value Table from nand otp"
	.ascii	" block:\012\000"
.LC170:
	.ascii	"[PHY_DBG] ch %d, can't get right otp value from nan"
	.ascii	"d otp blocks, then use otp command\012\000"
	.space	1
.LC171:
	.ascii	"m1_readretry_exit. \012\000"
	.space	3
.LC172:
	.ascii	"m1 retry!\012\000"
	.space	1
.LC173:
	.ascii	"m1 ReadRetry ok! ch =%d, chip = %d  block = %d, pag"
	.ascii	"e = %d, RetryCount = %d  \012\000"
	.space	2
.LC174:
	.ascii	" m1_special_init m1_read_retry_mode: %d m1_read_ret"
	.ascii	"ry_cycle:%d m1_read_retry_reg_num:%d\012\000"
	.space	3
.LC175:
	.ascii	" m1_special_init error m1_read_retry_mode: %d m1_re"
	.ascii	"ad_retry_cycle:%d m1_read_retry_reg_num:%d\012\000"
	.space	1
.LC176:
	.ascii	" m1_special_exit \012\000"
	.space	1
.LC177:
	.ascii	"set feature(addr %d) p0: %d readretry Configuration"
	.ascii	" failed!\012\000"
	.space	3
.LC178:
	.ascii	"micron read retry cannot support!\012\000"
	.space	1
.LC179:
	.ascii	"micron read retry conut: %d !\012\000"
	.space	1
.LC180:
	.ascii	"m4 retry!\012\000"
	.space	1
.LC181:
	.ascii	"m4 ReadRetry ok! ch =%d, chip = %d  block = %d, pag"
	.ascii	"e = %d, RetryCount = %d  \012\000"
	.space	2
.LC182:
	.ascii	" m4_special_init m4_read_retry_mode:%d m4_read_retr"
	.ascii	"y_cycle:%d \012\000"
.LC183:
	.ascii	" m4_special_init error m4_read_retry_mode:%d m4_rea"
	.ascii	"d_retry_cycle:%d \012\000"
	.space	2
.LC184:
	.ascii	" m4_special_exit \012\000"
	.space	1
.LC185:
	.ascii	"m2_lsb_exit\012\000"
	.space	3
.LC186:
	.ascii	"otp copy %d is ok!\012\000"
.LC187:
	.ascii	"otp copy failed!\012\000"
	.space	2
.LC188:
	.ascii	"_vender_get_param_otp_hynix error!\012\000"
.LC189:
	.ascii	"m2 get default param error !\012\000"
	.space	2
.LC190:
	.ascii	"[PHY_DBG] repair ch %d otp value end\012\000"
	.space	2
.LC191:
	.ascii	"m2_readretry_exit. \012\000"
	.space	3
.LC192:
	.ascii	"retry para:\000"
.LC193:
	.ascii	"m2 retry!\012\000"
	.space	1
.LC194:
	.ascii	"sdata:0x%x %x %x %x \000"
	.space	3
.LC195:
	.ascii	"m2 ReadRetry ok! ch =%d, chip = %d  block = %d, pag"
	.ascii	"e = %d, RetryCount = %d  \012\000"
	.space	2
.LC196:
	.ascii	" m2_special_init m2_read_retry_mode: %d m2_read_ret"
	.ascii	"ry_cycle:%d m2_read_retry_reg_num:%d\012\000"
	.space	3
.LC197:
	.ascii	" m2_special_init error m2_read_retry_mode: %d m2_re"
	.ascii	"ad_retry_cycle:%d m2_read_retry_reg_num:%d\012\000"
	.space	1
.LC198:
	.ascii	" m2_special_exit \012\000"
	.space	1
.LC199:
	.ascii	"m9 retry mode err : %d !\012\000"
	.space	2
.LC200:
	.ascii	"sandisk A19 read retry conut: %d !\012\000"
.LC201:
	.ascii	"m9 retry!\012\000"
	.space	1
.LC202:
	.ascii	"m9 set readretry error\012\000"
.LC203:
	.ascii	"m9 ReadRetry ok! ch =%d, chip = %d  block = %d, pag"
	.ascii	"e = %d, RetryCount = %d  \012\000"
	.space	2
.LC204:
	.ascii	"m9 ReadRetry ok with ecc limit! ch =%d, chip = %d  "
	.ascii	"block = %d, page = %d, RetryCount = %d  \012\000"
	.space	3
.LC205:
	.ascii	" m9_special_init m9_read_retry_mode:%d m9_read_retr"
	.ascii	"y_cycle :%d m9_read_retry_reg_cnt %d \012\000"
	.space	2
.LC206:
	.ascii	" m9_special_init error m9_read_retry_mode:%d m9_rea"
	.ascii	"d_retry_cycle :%d m9_read_retry_reg_cnt %d \012\000"
.LC207:
	.ascii	" m9_special_exit \012\000"
	.space	1
.LC208:
	.ascii	"_setup_nand_toggle_vendor_specific_feature() failed"
	.ascii	"!\012\000"
	.space	2
.LC209:
	.ascii	"[NE]block_used_count list: \012\000"
	.space	3
.LC210:
	.ascii	"[NE]block NO: %d \000"
	.space	2
.LC211:
	.ascii	"[NE]block_used_count: %d \012\000"
	.space	1
.LC212:
	.ascii	"[NE]invalid_page_count more than pages per blk 0 %d"
	.ascii	" %d!!\012\000"
	.space	2
.LC213:
	.ascii	"[NE]invalid_page_count more than pages per blk 1!!\012"
	.ascii	"\000"
.LC214:
	.ascii	"[NE]mtd size changed !\012\000"
.LC215:
	.ascii	"[ND]before second %d %d.\012\000"
	.space	2
.LC216:
	.ascii	"[NE]this zone not enough data block!!\012\000"
	.space	1
.LC217:
	.ascii	"[NE]this zone not enough free block %d , %d!!\012\000"
	.space	1
.LC218:
	.ascii	"[NE]NAND EVENT:do_write_error erase error1!\012\000"
	.space	3
.LC219:
	.ascii	"[NE]NAND EVENT:do_write_error erase error2!\012\000"
	.space	3
.LC220:
	.ascii	"[NE]do block_last_page_ecc_error!!\012\000"
.LC221:
	.ascii	"[NE]====no memory!!!!!=====\012\000"
	.space	3
.LC222:
	.ascii	"[NE]current_block_ecc_error invalid page data!!\012"
	.ascii	"\000"
	.space	3
.LC223:
	.ascii	"[NE]current_block_ecc_error no free block!!!\012\000"
	.space	2
.LC224:
	.ascii	"[NE]do block_last_page_ecc_error end!!\012\000"
.LC225:
	.ascii	"[NE]current_block_ecc_error unkown page data!!\012\000"
.LC226:
	.ascii	"[NE]%s:malloc 4096 fail!\012\000"
	.space	2
.LC227:
	.ascii	"[NE]%x %x %x %x %x \000"
.LC228:
	.ascii	"%x %x %x %x %x!!\012\000"
	.space	2
.LC229:
	.ascii	"buf:%x %x %x %x %x \000"
.LC230:
	.ascii	"%x %x %x %x %x %x!!\012\000"
	.space	3
.LC231:
	.ascii	"[NE]%s:malloc 8192 fail!\012\000"
	.space	2
.LC232:
	.ascii	"zoneNO:%d total_pages: %d\012\000"
	.space	1
.LC233:
	.ascii	"MEMSET bytes: %d\012\000"
	.space	2
.LC234:
	.ascii	"exception case bad block\012\000"
	.space	2
.LC235:
	.ascii	"exception case wbc\012\000"
.LC236:
	.ascii	"exception case gd\012\000"
	.space	1
.LC237:
	.ascii	"%.2x \000"
	.space	2
.LC238:
	.ascii	"%s %d, flag = %d\012\000"
	.space	2
.LC239:
	.ascii	"OOB :\000"
	.space	2
.LC240:
	.ascii	" rppm nouse page %d!!\012\000"
	.space	1
.LC241:
	.ascii	"[NE] power off ecc error 1!!\012\000"
	.space	2
.LC242:
	.ascii	"[NE]invalid logic_page block:%d page:%d,logic_page "
	.ascii	"0x%x!!\012\000"
	.space	1
.LC243:
	.ascii	"[NE]power_down page %d!!\012\000"
	.space	2
.LC244:
	.ascii	"[NE]recover_phy_page_mapping ecc error block:%d;pag"
	.ascii	"e:%d!!\012\000"
	.space	1
.LC245:
	.ascii	" rppm invalid page %d!!\012\000"
	.space	3
.LC246:
	.ascii	"[NE]recover_phy_page_mapping unkown page data %d!!\012"
	.ascii	"\000"
.LC247:
	.ascii	"[NE]ret:%d read fail %d %d,read one more time!!\012"
	.ascii	"\000"
	.space	3
.LC248:
	.ascii	"[NE]ret:%d read fail %d %d,%d times,don't read more"
	.ascii	"!!\012\000"
	.space	1
.LC249:
	.ascii	"[NE] power off ecc error 3!!\012\000"
	.space	2
.LC250:
	.ascii	"[NE]recover_block_phy_page_mapping error page %d %d"
	.ascii	"!!\012\000"
	.space	1
.LC251:
	.ascii	"[NE]get_used_page_num ecc error block:%d page:%d!\012"
	.ascii	"\000"
	.space	1
.LC252:
	.ascii	"[NE] power off ecc error 4!!\012\000"
	.space	2
.LC253:
	.ascii	"[NE]get_used_page_num used %d %d!\012\000"
	.space	1
.LC254:
	.ascii	"[NE]gupnnc ecc err b:%d p:%d!\012\000"
	.space	1
.LC255:
	.ascii	"[NE]gupnnc power off ecc err!!\012\000"
.LC256:
	.ascii	"[NE]cross talk block:%d total_good_num %d good_page"
	.ascii	"_num: %d!\012\000"
	.space	2
.LC257:
	.ascii	"[NE]erase_block block %d!\012\000"
	.space	1
.LC258:
	.ascii	"[NE]erase_block block fail %d!\012\000"
.LC259:
	.ascii	"[NE] adjust free list fail!!\012\000"
	.space	2
.LC260:
	.ascii	"[NE] adjust free list : %d %d!!\012\000"
	.space	3
.LC261:
	.ascii	"%s write b@%d p@%d err\012\000"
.LC262:
	.ascii	"%s read b@%d p@%d err\012\000"
	.space	1
.LC263:
	.ascii	"%s b@%d p@%d patt %02x checking main data fail\012\000"
.LC264:
	.ascii	"%s b@%d p@%d patt %02x checking spare data fail\012"
	.ascii	"\000"
	.space	3
.LC265:
	.ascii	"[ND]build_zone_list_first.\012\000"
.LC266:
	.ascii	"[NE]no block %d\012\000"
	.space	3
.LC267:
	.ascii	"%s:logic_page:0x%x >= total_pages:0x%x\012\000"
.LC268:
	.ascii	"[NE]logic_page_map NULL!!\012\000"
	.space	1
.LC269:
	.ascii	"[NE]invalid_page_count more than pages_per_blk! :%d"
	.ascii	"\012\000"
	.space	3
.LC270:
	.ascii	"[NE]inv P b:%d p:%d,lp %x!!\012\000"
	.space	3
.LC271:
	.ascii	"[NE]build invalid_page_count more than pages_per_bl"
	.ascii	"k!!\012\000"
.LC272:
	.ascii	"[NE] zone_param_init malloc_logic_page_map error %d"
	.ascii	"\012\000"
	.space	3
.LC273:
	.ascii	"[NE] zone_param_init error1\012\000"
	.space	3
.LC274:
	.ascii	"[NE] zone_param_init error2\012\000"
	.space	3
.LC275:
	.ascii	"[NE] zone_param_init error3\012\000"
	.space	3
.LC276:
	.ascii	"[NE] zone_param_init error4\012\000"
	.space	3
.LC277:
	.ascii	"[NE] zone_param_init error5\012\000"
	.space	3
.LC278:
	.ascii	"[NE]READ_RECLAIM!\012\000"
	.space	1
.LC279:
	.ascii	"[NE]fill block %d\012\000"
	.space	1
.LC280:
	.ascii	"[ND]  bad block: %d\012\000"
	.space	3
.LC281:
	.ascii	"[NE]first page ecc error ! %d!\012\000"
.LC282:
	.ascii	"[NE]first page last page ecc error ! %d!\012\000"
	.space	2
.LC283:
	.ascii	"last page spare data: %02x %02x %02x %02x %02x %02x"
	.ascii	" %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\000"
	.space	2
.LC284:
	.ascii	"[ND]bad block: %d\012\000"
	.space	1
.LC285:
	.ascii	"[NE] ftl end block! %d!\012\000"
	.space	3
.LC286:
	.ascii	"[NE]do nothing\012\000"
.LC287:
	.ascii	"[NE]unkown page 0 spare data %x,%x,%x,%x,%x,%x,%x!!"
	.ascii	"\012\000"
	.space	3
.LC288:
	.ascii	"[NE]last page ok ! %d!\012\000"
.LC289:
	.ascii	"[NE]last page blank ! %d!\012\000"
	.space	1
.LC290:
	.ascii	"[NE]last page unkown ! %d!\012\000"
.LC291:
	.ascii	"[NE]paramter error!\012\000"
	.space	3
.LC292:
	.ascii	"[NE]read sector:0x%x,len:0x%x\012\000"
	.space	1
.LC293:
	.ascii	"[NE]nand_sector_cache_read 1!\012\000"
	.space	1
.LC294:
	.ascii	"[NE]write sector:0x%x,len:0x%x\012\000"
.LC295:
	.ascii	"[NE]%s:malloc 512 fail!\012\000"
	.space	3
.LC296:
	.ascii	"[NE]nand_sector_cache_write 1!\012\000"
.LC297:
	.ascii	"[NE]nand_sector_cache_write 2!\012\000"
.LC298:
	.ascii	"[NE]nand_flush_write_cache write error\012\000"
.LC299:
	.ascii	"[NE]flush write cache to nand no cache!\012\000"
	.space	3
.LC300:
	.ascii	"[NE]ECC_LIMIT happened 10! page:%d cache: %d!\012\000"
	.space	1
.LC301:
	.ascii	"[NE]flush write cache to nand error!\012\000"
	.space	2
.LC302:
	.ascii	"[NE]nand_cache_write error  NULL!\012\000"
	.space	1
.LC303:
	.ascii	"[NE]error1 bitmap %x!\012\000"
	.space	1
.LC304:
	.ascii	"[NE]ECC_LIMIT happened 12! page:%d!\012\000"
	.space	3
.LC305:
	.ascii	"[NE]error1 flush write cache to nand\012\000"
	.space	2
.LC306:
	.ascii	"[NE]error2 flush write cache to nand\012\000"
	.space	2
.LC307:
	.ascii	"[NE]ECC_LIMIT happened 11! page:%d!\012\000"
	.space	3
.LC308:
	.ascii	"[ND]flush cache %d %d!\012\000"
.LC309:
	.ascii	"[NE]nand_discard paramter error!\012\000"
	.space	2
.LC310:
	.ascii	"[NE]dicard sector:0x%x,len:0x%x\012\000"
	.space	3
.LC311:
	.ascii	"[NE]page_map NULL!!\012\000"
	.space	3
.LC312:
	.ascii	"%4d:<%4d %3d><%d>\012\000"
	.space	1
.LC313:
	.ascii	"[NE]no free block\012\000"
	.space	1
.LC314:
	.ascii	"[NE]do_write_error erase error!\012\000"
	.space	3
.LC315:
	.ascii	"[NE]last block can not used to logic page %d!!\012\000"
.LC316:
	.ascii	"[NE]last block can not used to power down page %d!!"
	.ascii	"\012\000"
	.space	3
.LC317:
	.ascii	"[NE]write_phy_data_to_current_block write error %d!"
	.ascii	"\012\000"
	.space	3
.LC318:
	.ascii	"[NE]do_write_error error\012\000"
	.space	2
.LC319:
	.ascii	"[NE]invalid_page_count more than pages_per_blk at w"
	.ascii	"rite logic data to current block:%d!!\012\000"
	.space	2
.LC320:
	.ascii	"[NE]write_phy_data_to_current_block error2\012\000"
.LC321:
	.ascii	"[NE]write phy data to current block error2\012\000"
.LC322:
	.ascii	"[NE]write logic data to current block error\012\000"
	.space	3
.LC323:
	.ascii	"[NE]no free block!!!!\012\000"
	.space	1
.LC324:
	.ascii	"[ND]swl: new_cur_blk->alloc ok(max_ec): block_no = "
	.ascii	"%d; ec = %d; \012\000"
	.space	2
.LC325:
	.ascii	"[NE]new_block_init_for_write erase error!\012\000"
	.space	1
.LC326:
	.ascii	"[ND]cross_talk_rebuild_current_block fail 1\012\000"
	.space	3
.LC327:
	.ascii	"[ND]cross_talk_rebuild_current_block!! %d,%d,%d!\012"
	.ascii	"\000"
	.space	2
.LC328:
	.ascii	"[ND]cross_talk_rebuild_current_block fail 2\012\000"
	.space	3
.LC329:
	.ascii	"[ND]cross_talk_rebuild_current_block!! %d,%d,%d,%d,"
	.ascii	"%d,%d!\012\000"
	.space	1
.LC330:
	.ascii	"[ND]cross_talk_rebuild_current_block fail 3\012\000"
	.space	3
.LC331:
	.ascii	"[ND]corss talk rebuild 1 %d %d %d!\012\000"
.LC332:
	.ascii	"[ND]corss talk rebuild 2 %d %d %d!\012\000"
.LC333:
	.ascii	"[NE] get free block fail!\012\000"
	.space	1
.LC334:
	.ascii	"[NE]something is error here 14!\012\000"
	.space	3
.LC335:
	.ascii	"[NE]page lost %d %d\012\000"
	.space	3
.LC336:
	.ascii	"[NE]cross_talk_rebuild_current_block fail 1\012\000"
	.space	3
.LC337:
	.ascii	"[ND]nctrcb1 %d,%d,%d!\012\000"
	.space	1
.LC338:
	.ascii	"[ND]no_cross_talk_rebuild_current_block fail 2\012\000"
.LC339:
	.ascii	"[ND]nctrcb2 %d,%d,%d,%d,%d,%d!\012\000"
.LC340:
	.ascii	"[ND]no_cross_talk_rebuild_current_block fail 4\012\000"
.LC341:
	.ascii	"[NE]no_cross_talk_rebuild_current_block erase block"
	.ascii	" fail %d!\012\000"
	.space	2
.LC342:
	.ascii	"[NE]something is error here 16!\012\000"
	.space	3
.LC343:
	.ascii	"[NE]do current_block_ecc_error %d!!\012\000"
	.space	3
.LC344:
	.ascii	"[NE] power off ecc error 2!!\012\000"
	.space	2
.LC345:
	.ascii	"[NE]current_block_ecc ok page %d %d!!\012\000"
	.space	1
.LC346:
	.ascii	"[NE]current_block_ecc_error page %d!!\012\000"
	.space	1
.LC347:
	.ascii	"[NE]do current_block_ecc_error end %d %d!!\012\000"
.LC348:
	.ascii	"[NE]current_block_ecc_error invalid page data %d!!\012"
	.ascii	"\000"
.LC349:
	.ascii	"[NE]current_block_ecc_error unkown page data %d!!\012"
	.ascii	"\000"
	.space	1
.LC350:
	.ascii	"[NE]NAND_EVENT: 2 current used block found %d,%d!\012"
	.ascii	"\000"
	.space	1
.LC351:
	.ascii	"[NE]no crosstalk:2 block used count diff %d,%d %d,%"
	.ascii	"d !\012\000"
.LC352:
	.ascii	"[NE]NAND_EVENT: muti current used block found1 %d %"
	.ascii	"d!\012\000"
	.space	1
.LC353:
	.ascii	"[NE]NAND_EVENT: muti current used block found2 %d %"
	.ascii	"d!\012\000"
	.space	1
.LC354:
	.ascii	"[NE]NAND_EVENT: muti current used block found3 %d %"
	.ascii	"d!\012\000"
	.space	1
.LC355:
	.ascii	"[NE]last page unkown ecc ok %d,%d!!\012\000"
	.space	3
.LC356:
	.ascii	"[NE]last page ecc error %d,%d!!\012\000"
	.space	3
.LC357:
	.ascii	"[ND]something strange %d,%d!!\012\000"
	.space	1
.LC358:
	.ascii	"[ND]all block full!!\012\000"
	.space	2
.LC359:
	.ascii	"[ND]corss talk rebuild 0 %d %d!!\012\000"
	.space	2
.LC360:
	.ascii	"[NE]start here error! \012\000"
.LC361:
	.ascii	"[NE]muti current used block 1st %d %d!\012\000"
.LC362:
	.ascii	"[NE]muti current used block 2nd %d %d!\012\000"
.LC363:
	.ascii	"[NE]something is error here 7!\012\000"
.LC364:
	.ascii	"[NE]p_phy_block_info_last %d %d!\012\000"
	.space	2
.LC365:
	.ascii	"[NE]muti current used block 1 %d %d!\012\000"
	.space	2
.LC366:
	.ascii	"[NE]muti current used block 2 %d %d!\012\000"
	.space	2
.LC367:
	.ascii	"[NE]current_block ecc error %d!\012\000"
	.space	3
.LC368:
	.ascii	"[NE]error! no free block!\012\000"
	.space	1
.LC369:
	.ascii	"[ND]recover %d %d\012\000"
	.space	1
.LC370:
	.ascii	"[NE]error!!\012\000"
	.space	3
.LC371:
	.ascii	"[NE]new_block_init_for_write error here!\012\000"
	.space	2
.LC372:
	.ascii	"[NE]something is error here 10!\012\000"
	.space	3
.LC373:
	.ascii	"[NE]something is error here 11!\012\000"
	.space	3
.LC374:
	.ascii	"[NE]something is error here 12 %d,%d!\012\000"
	.space	1
.LC375:
	.ascii	"[NE]something is error here 17 %d,%d!\012\000"
	.space	1
.LC376:
	.ascii	"[ND]get a new free block\012\000"
	.space	2
.LC377:
	.ascii	"[NE]no free block to use!\012\000"
	.space	1
.LC378:
	.ascii	"[NE]NAND_EVENT: last page spare data %x,%x,%x,%x,%x"
	.ascii	",%x,%x!!\012\000"
	.space	3
.LC379:
	.ascii	"[NE]current used block :%d\012\000"
.LC380:
	.ascii	"[NE]get_current_used_block fail!\012\000"
	.space	2
.LC381:
	.ascii	"[ND]free block nums is %d !\012\000"
	.space	3
.LC382:
	.ascii	"[NE]get_new_current_block error3!\012\000"
	.space	1
.LC383:
	.ascii	"[NE]get_new_current_block error4!\012\000"
	.space	1
.LC384:
	.ascii	"[NE]logic_page_map_temp NULL!!\012\000"
.LC385:
	.ascii	"[NE]invaild_page num error:[%d] [%d]  used_page:[%d"
	.ascii	"]!!\012\000"
.LC386:
	.ascii	"[NE]write_phy_data_to_current_block error1\012\000"
.LC387:
	.ascii	"[NE]write_phy_page_map_to_current_block error!\012\000"
.LC388:
	.ascii	"[NE]get_new_current_block error1!\012\000"
	.space	1
.LC389:
	.ascii	"[NE]get_new_current_block error2!\012\000"
	.space	1
.LC390:
	.ascii	"[NE]do after write error1!\012\000"
.LC391:
	.ascii	"[NE]do_before_write error!\012\000"
.LC392:
	.ascii	"[NE]write data to page error!\012\000"
	.space	1
.LC393:
	.ascii	"[NE]do after write error2!\012\000"
.LC394:
	.ascii	"[NE]garbage_collect_first none!!\012\000"
	.space	2
.LC395:
	.ascii	"[NE]garbage_collect_first PHY_MAPPING_PAGE ecc err "
	.ascii	"block:%d !!\012\000"
.LC396:
	.ascii	"[NE]garbage_collect_first invaild_page num error!!\012"
	.ascii	"\000"
.LC397:
	.ascii	"[NE]garbage_collect_first read a logic page error %"
	.ascii	"d %d %d %d!!\012\000"
	.space	3
.LC398:
	.ascii	"[NE]%x %x %x %x %x !!\012\000"
	.space	1
.LC399:
	.ascii	"[NE]%x %x %x %x %x!!\012\000"
	.space	2
.LC400:
	.ascii	"[NE]nand_write_logic_page_no_gc gc fail!!\012\000"
	.space	1
.LC401:
	.ascii	"[NE]erase ecc error block\012\000"
	.space	1
.LC402:
	.ascii	"[NE]garbage_collect_first wrong!!\012\000"
	.space	1
.LC403:
	.ascii	"[NE]garbage_collect_first cannot find PHY_MAPPING_P"
	.ascii	"AGE %d %d %d %d!!\012\000"
	.space	2
.LC404:
	.ascii	"[NE]garbage_collect_first cannot find PHY_MAPPING_P"
	.ascii	"AGE %x %x %x %x !!\012\000"
	.space	1
.LC405:
	.ascii	"[NE]garbage_collect_first cannot find PHY_MAPPING_P"
	.ascii	"AGE %x %x %x %x %x!!\012\000"
	.space	3
.LC406:
	.ascii	"[NE]garbage_collect_first rebuild %d,%d!!\012\000"
	.space	1
.LC407:
	.ascii	"[ND]no block to prio gc\012\000"
	.space	3
.LC408:
	.ascii	"[NE]do_prio_gc garbage_collect_first fail:%d, type:"
	.ascii	"%d\012\000"
	.space	1
.LC409:
	.ascii	"[ND]prio gc fail2:%d, type:%d\012\000"
	.space	1
.LC410:
	.ascii	"[ND]no need gc:%d, type:%d\012\000"
.LC411:
	.ascii	"[ND]prio gc fail1:%d, type:%d\012\000"
	.space	1
.LC412:
	.ascii	"[NE]read a page: %d READ_RECLAIM!\012\000"
	.space	1
.LC413:
	.ascii	"[NE]gc_one ok!\012\000"
.LC414:
	.ascii	"[NE]gc_one error!\012\000"
	.space	1
.LC415:
	.ascii	"[NE]gc all end\012\000"
.LC416:
	.ascii	"[NE] gc_win_block gc_one error \012\000"
	.space	3
.LC417:
	.ascii	"[NE]not power on gc %d!\012\000"
	.space	3
.LC418:
	.ascii	"[NE]emergency power on gc %d !\012\000"
.LC419:
	.ascii	"[NE]power on gc %d!\012\000"
	.space	3
.LC420:
	.ascii	"[ND]first\012\000"
	.space	1
.LC421:
	.ascii	"[NE]init_zone_after_first_scan error\012\000"
	.space	2
.LC422:
	.ascii	"[NE]second_scan_all_blocks error\012\000"
	.space	2
.LC423:
	.ascii	"[ND]nftl ok!\012\000"
	.space	2
.LC424:
	.ascii	"build_zone_list start\012\000"
	.space	1
.LC425:
	.ascii	"nftl_cache_init start\012\000"
	.space	1
.LC426:
	.ascii	"=================================2================="
	.ascii	"======================\012\000"
	.space	1
.LC427:
	.ascii	"nand_find_zone_table end\012\000"
	.space	2
.LC428:
	.ascii	"[ND]==========================\012\000"
.LC429:
	.ascii	"[NE]not enough blocks %d!\012\000"
	.space	1
.LC430:
	.ascii	"[ND]nftl start:%d,%d\012\000"
	.space	2
.LC431:
	.ascii	"[NE] zone_param_init fail!\012\000"
.LC432:
	.ascii	"[NE] build_zone_list_first fail!\012\000"
	.space	2
.LC433:
	.ascii	"[NE] build_zone_list fail!\012\000"
.LC434:
	.ascii	"try again\012\000"
	.space	1
.LC435:
	.ascii	"000\000"
.LC436:
	.ascii	"7\000"
	.space	2
.LC437:
	.ascii	"3\000"
	.space	2
.LC438:
	.ascii	"sun8iw7 nftl version %s.%s.%s  %s-%s\012\000"
	.space	2
.LC439:
	.ascii	"09:56\000"
	.space	2
.LC440:
	.ascii	"2021.05.24\000"
	.space	1
.LC441:
	.ascii	"[ND]max_erase_times = %i\012\000"
	.space	2
.LC442:
	.ascii	"[NE]nftl_initialize failed\012\000"
.LC443:
	.ascii	"[ND]nftl_add ok\012\000"
	.space	3
.LC444:
	.ascii	"[NE]nftl_build_all error!\012\000"
	.space	1
.LC445:
	.ascii	"[NE]garbage_collect phy_block null\012\000"
.LC446:
	.ascii	"[ND]garbage too frequency! %d:%d\012\000"
	.space	2
.LC447:
	.ascii	"[NE]garbage_collect_first wrong! %d\012\000"
	.space	3
.LC448:
	.ascii	"[NE]error nand_write_logic_page_no_gc\012\000"
	.space	1
.LC449:
	.ascii	"[NE]swl: garbage_collect_first fail:%d\012\000"
.LC450:
	.ascii	"[ND]swl: the block(%d) for static WL is not in inva"
	.ascii	"lid_list\012\000"
	.space	3
.LC451:
	.ascii	"[ND]swl: done_static_wear_leveling(min_ec) block:%d"
	.ascii	", erase_count:%d\012\000"
	.space	3
.LC452:
	.ascii	"[NE]swl: last->check fail:%d\012\000"
	.bss
	.align	2
	.set	.LANCHOR0,. + 0
	.type	m7_toggle_mode_flag, %object
	.size	m7_toggle_mode_flag, 4
m7_toggle_mode_flag:
	.space	4
	.type	nftl_blk_head, %object
	.size	nftl_blk_head, 72
nftl_blk_head:
	.space	72
	.type	m5_read_retry_mode, %object
	.size	m5_read_retry_mode, 1
m5_read_retry_mode:
	.space	1
	.type	m5_read_retry_cycle, %object
	.size	m5_read_retry_cycle, 1
m5_read_retry_cycle:
	.space	1
	.type	m3_read_retry_mode, %object
	.size	m3_read_retry_mode, 1
m3_read_retry_mode:
	.space	1
	.type	m3_read_retry_cycle, %object
	.size	m3_read_retry_cycle, 1
m3_read_retry_cycle:
	.space	1
	.type	m3_read_retry_reg_num, %object
	.size	m3_read_retry_reg_num, 1
m3_read_retry_reg_num:
	.space	1
	.type	m3_lsb_mode_reg_num, %object
	.size	m3_lsb_mode_reg_num, 1
m3_lsb_mode_reg_num:
	.space	1
	.space	2
	.type	m3_lsb_mode_reg_adr, %object
	.size	m3_lsb_mode_reg_adr, 5
m3_lsb_mode_reg_adr:
	.space	5
	.space	3
	.type	m3_lsb_mode_val, %object
	.size	m3_lsb_mode_val, 5
m3_lsb_mode_val:
	.space	5
	.type	m7_read_retry_mode, %object
	.size	m7_read_retry_mode, 1
m7_read_retry_mode:
	.space	1
	.type	m7_read_retry_cycle, %object
	.size	m7_read_retry_cycle, 1
m7_read_retry_cycle:
	.space	1
	.type	m7_read_retry_reg_cnt, %object
	.size	m7_read_retry_reg_cnt, 1
m7_read_retry_reg_cnt:
	.space	1
	.type	m7_sclk1_bak, %object
	.size	m7_sclk1_bak, 4
m7_sclk1_bak:
	.space	4
	.type	m7_sclk0_bak, %object
	.size	m7_sclk0_bak, 4
m7_sclk0_bak:
	.space	4
	.type	m6_read_retry_mode, %object
	.size	m6_read_retry_mode, 1
m6_read_retry_mode:
	.space	1
	.type	m6_read_retry_cycle, %object
	.size	m6_read_retry_cycle, 1
m6_read_retry_cycle:
	.space	1
	.type	m6_read_retry_reg_cnt, %object
	.size	m6_read_retry_reg_cnt, 1
m6_read_retry_reg_cnt:
	.space	1
	.space	1
	.type	m6_sclk1_bak, %object
	.size	m6_sclk1_bak, 4
m6_sclk1_bak:
	.space	4
	.type	m6_sclk0_bak, %object
	.size	m6_sclk0_bak, 4
m6_sclk0_bak:
	.space	4
	.type	m6_retry_flag, %object
	.size	m6_retry_flag, 4
m6_retry_flag:
	.space	4
	.type	m6_toggle_mode_flag, %object
	.size	m6_toggle_mode_flag, 4
m6_toggle_mode_flag:
	.space	4
	.type	m3_read_retry_reg_adr, %object
	.size	m3_read_retry_reg_adr, 4
m3_read_retry_reg_adr:
	.space	4
	.type	m3_read_retry_val, %object
	.size	m3_read_retry_val, 56
m3_read_retry_val:
	.space	56
	.type	m3_lsb_mode_default_val, %object
	.size	m3_lsb_mode_default_val, 5
m3_lsb_mode_default_val:
	.space	5
	.type	m1_read_retry_mode, %object
	.size	m1_read_retry_mode, 1
m1_read_retry_mode:
	.space	1
	.type	m1_read_retry_reg_num, %object
	.size	m1_read_retry_reg_num, 1
m1_read_retry_reg_num:
	.space	1
	.space	1
	.type	m1_read_retry_reg_adr, %object
	.size	m1_read_retry_reg_adr, 4
m1_read_retry_reg_adr:
	.space	4
	.type	m1_read_retry_cycle, %object
	.size	m1_read_retry_cycle, 1
m1_read_retry_cycle:
	.space	1
	.space	3
	.type	m4_p1, %object
	.size	m4_p1, 4
m4_p1:
	.space	4
	.type	m4_read_retry_mode, %object
	.size	m4_read_retry_mode, 1
m4_read_retry_mode:
	.space	1
	.type	m4_read_retry_cycle, %object
	.size	m4_read_retry_cycle, 1
m4_read_retry_cycle:
	.space	1
	.type	m2_read_retry_mode, %object
	.size	m2_read_retry_mode, 1
m2_read_retry_mode:
	.space	1
	.type	m2_read_retry_cycle, %object
	.size	m2_read_retry_cycle, 1
m2_read_retry_cycle:
	.space	1
	.type	m2_read_retry_reg_num, %object
	.size	m2_read_retry_reg_num, 1
m2_read_retry_reg_num:
	.space	1
	.type	m2_lsb_mode_reg_num, %object
	.size	m2_lsb_mode_reg_num, 1
m2_lsb_mode_reg_num:
	.space	1
	.space	2
	.type	m2_lsb_mode_reg_adr, %object
	.size	m2_lsb_mode_reg_adr, 4
m2_lsb_mode_reg_adr:
	.space	4
	.type	m2_lsb_mode_val, %object
	.size	m2_lsb_mode_val, 4
m2_lsb_mode_val:
	.space	4
	.type	m2_lsb_mode_default_val, %object
	.size	m2_lsb_mode_default_val, 4
m2_lsb_mode_default_val:
	.space	4
	.type	m2_read_retry_reg_adr, %object
	.size	m2_read_retry_reg_adr, 8
m2_read_retry_reg_adr:
	.space	8
	.type	m9_read_retry_mode, %object
	.size	m9_read_retry_mode, 1
m9_read_retry_mode:
	.space	1
	.type	m9_read_retry_cycle, %object
	.size	m9_read_retry_cycle, 1
m9_read_retry_cycle:
	.space	1
	.type	m9_read_retry_reg_cnt, %object
	.size	m9_read_retry_reg_cnt, 1
m9_read_retry_reg_cnt:
	.space	1
	.space	1
	.type	m9_sclk1_bak, %object
	.size	m9_sclk1_bak, 4
m9_sclk1_bak:
	.space	4
	.type	m9_sclk0_bak, %object
	.size	m9_sclk0_bak, 4
m9_sclk0_bak:
	.space	4
	.type	read_mapping_page, %object
	.size	read_mapping_page, 4
read_mapping_page:
	.space	4
	.ident	"GCC: (Linaro GCC 5.3-2016.05) 5.3.1 20160412"
	.section	.note.GNU-stack,"",%progbits
