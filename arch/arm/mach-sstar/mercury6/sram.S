/*
* sram.S- Sigmastar
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
/*------------------------------------------------------------------------------
    Function Code
-------------------------------------------------------------------------------*/
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/asm-offsets.h>
#include <asm/hardware/cache-l2x0.h>

#define ARCH_us(x) (6*x)
    .align 3
.globl sram_suspend_imi
.globl v7_cpu_resume

/* A macro about using arch timer to delay
    inputs :
    t : us to delay
    l : For different branch naming
*/
.macro arch_usdelay t, l
    ldr   r8, =ARCH_us(\t)      /*delay tms*/
    mrrc  p15, 1, r9, r10, c14
    add   r9, r9, r8            /*Target tick*/
delay_retry\l:
    isb sy
    mrrc  p15, 1, r11, r12, c14
    cmp   r11, r9
    blt   delay_retry\l         /* jump to delay_retry in case r0 is bigger (N==1) */
.endm

ENTRY(sram_suspend_imi)

    // 04. LPDDR2 enter self-refresh
    //wriu -w   0x1012e0,0x0000
    //wriu -w   0x101246,0xFFFE
    //wriu -w   0x101266,0xFFFF
    //wriu -w   0x101286,0xFFFF
    //wriu -w   0x1012A6,0xFFFF
    //wriu -w   0x1011E6,0x00FE
    //wait 20

    ldr   r1, =0xFD000000
    ldr   r3, =0x101200
    ldr   r4, =0x101100
    ldr   r5, =0x101000
    add   r2, r1, r3, lsl #1
    ldr   r0, =0x0000
    str   r0, [r2, #0xE0 << 1]
    ldr   r0, =0xFFFE
    str   r0, [r2, #0x46 << 1]
    ldr   r0, =0xFFFF
    str   r0, [r2, #0x66 << 1]
    str   r0, [r2, #0x86 << 1]
    str   r0, [r2, #0xA6 << 1]
    add   r2, r1, r4, lsl #1
    ldr   r0, =0x00FE
    str   r0, [r2, #0xE6 << 1]
    //delay
    arch_usdelay 1000,__LINE__

    //Pre-charge all then one refresh
    //wriu -w   0x101218  0x0400
    add   r2, r1, r3, lsl #1
    ldr   r0, =0x0400
    str   r0, [r2, #0x18 << 1]
    //wriu -b   0x101200  0x30 0x20
    ldrb  r0, [r2, #0x00]
    and   r0, r0, #0xCF
    orr   r0, r0, #0x20
    strb  r0, [r2, #0x00]
    //wriu -b   0x101201  0x1e 0x04
    ldrb  r0, [r2, #0x01]
    and   r0, r0, #0xE1
    orr   r0, r0, #0x04
    strb  r0, [r2, #0x01]
    //wriu -b   0x101201  0x01 0x00
    ldrb  r0, [r2, #0x01]
    and   r0, r0, #0xFE
    strb  r0, [r2, #0x01]
    //wriu -b   0x101201  0x01 0x01
    ldrb  r0, [r2, #0x01]
    orr   r0, r0, #0x01
    strb  r0, [r2, #0x01]
    //wriu -b   0x101201  0x01 0x00
    ldrb  r0, [r2, #0x01]
    and   r0, r0, #0xFE
    strb  r0, [r2, #0x01]
    //wriu -b   0x101201  0x1e 0x02
    ldrb  r0, [r2, #0x01]
    and   r0, r0, #0xE1
    orr   r0, r0, #0x02
    strb  r0, [r2, #0x01]
    //wriu -b   0x101201  0x01 0x00
    ldrb  r0, [r2, #0x01]
    and   r0, r0, #0xFE
    strb  r0, [r2, #0x01]
    //wriu -b   0x101201  0x01 0x01
    ldrb  r0, [r2, #0x01]
    orr   r0, r0, #0x01
    strb  r0, [r2, #0x01]
    //wriu -b   0x101201  0x01 0x00
    ldrb  r0, [r2, #0x01]
    and   r0, r0, #0xFE
    strb  r0, [r2, #0x01]
    //delay, DO NOT REMOVE THIS DELAY!!!!
    arch_usdelay 1000,__LINE__

    //wriu -w   0x101200  0x202e
    ldr   r0, =0x202e
    str   r0, [r2, #0x00 << 1]
    //wait 10
    //delay
    arch_usdelay 1000,__LINE__

    // 05. AN power down
    //wriu -b 0x101203 0xF0 0xF0
    ldrb  r0, [r2, #0x05]
    orr   r0, r0, #0xF0
    strb  r0, [r2, #0x05]
    //wriu -b 0x101000 0x08 0x08
    //wriu -b 0x101000 0x10 0x10
    add   r2, r1, r5, lsl #1
    ldrb  r0, [r2, #0x00 << 1]
    orr   r0, r0, #0x08
    strb  r0, [r2, #0x00 << 1]
    ldrb  r0, [r2, #0x00 << 1]
    orr   r0, r0, #0x10
    strb  r0, [r2, #0x00 << 1]
    //wriu -b 0x101054 0x70 0x70
    ldrb  r0, [r2, #0xA8]
    orr   r0, r0, #0x70
    strb  r0, [r2, #0xA8]
    //wriu -b 0x101008 0x3F 0x00
    ldrb  r0, [r2, #0x10]
    and   r0, r0, #0xC0
    strb  r0, [r2, #0x10]

     // 06. power down
    //wriu -w   0x00003408 0x01
    //wriu -w   0x00003400 0x20
    // Turn-on ISO flow
    //wriu -w   0x00003406 0x01
    //wait 1
    //wriu -w   0x00003406 0x03
    //wait 1
    //wriu -w   0x00003406 0x07
    //wait 1
    //wriu -w   0x00003406 0x05
    //wait 1
    //wriu -w   0x00003406 0x01
    //wait 1
    //wriu -w   0x00003406 0x00
    //wait 1
    //wriu -w   0x00003400 0x00
    //wriu -w   0x0000341E 0x00
    // Turn-on ISO flow
    //wriu -w   0x00003406 0x01
    //wait 1
    //wriu -w   0x00003406 0x03
    //wait 1
    //wriu -w   0x00003406 0x07
    //wait 1
    //wriu -w   0x00003406 0x05
    //wait 1
    //wriu -w   0x00003406 0x01
    //wait 1
    //wriu -w   0x00003406 0x00
    //wait 1
    //---------------------------------

    ldr   r1, =0xFD000000
    ldr   r3, =0x152400
    add   r2, r1, r3, lsl #1

    // timer1 DEF:x0000 (x480/32K=36 ms)
    //ldr   r0, =0x0480
    //str   r0, [r2, #0x52 << 1]

    //[MUST] t2+t3+t4 > 55ms from scope measurment
    // timer2 DEF:x0080 (x0080/32K=4 ms)
    ldr   r0, =0x0000
    str   r0, [r2, #0x54 << 1]

    // timer3 DEF:x0010 (x8000/32K=1024 ms)
    ldr   r0, =0x0C80 // 100ms (64ms auto wakeup)
    str   r0, [r2, #0x56 << 1]

    // timer4 DEF:x0080 (x0080/32K=4 ms)
    //ldr   r0, =0xFFFF
    //str   r0, [r2, #0x58 << 1]

    // enter u3 mode
    ldr   r0, =0x0437
    str   r0, [r2, #0x50 << 1]

    //----------------------------------
start:
    ldr   r1, =0xFD000000
    ldr   r3, =0x003400
    add   r2, r1, r3, lsl #1

    // sel fro
    //ldr   r0, =0x0001
    //str   r0, [r2, #0x50 << 1]

    // power down
    ldr   r0, =0x0000
    str   r0, [r2, #0x00 << 1]

    ldr   r0, =0x0004
    str   r0, [r2, #0x1E << 1]

	ldr   r0, =0x0011
    str   r0, [r2, #0x40 << 1]

rtc_iso:
	ldr   r0, =0x0001
	str   r0, [r2, #0x60 << 1]

1:
	ldr   r0, [r2, #0x62 << 1]
	and   r0, r0, 0x0001
	teq   r0, #0x0001
	bne   1b

	ldr   r0, =0x0001
	str   r0, [r2, #0x64 << 1]

2:
	ldr   r0, [r2, #0x62 << 1]
	and   r0, r0, 0x0001
	teq   r0, #0x0001
	beq   2b

    nop
    nop
    nop
    nop


ENDPROC(sram_suspend_imi)
.ltorg