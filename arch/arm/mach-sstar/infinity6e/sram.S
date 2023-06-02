/*
* sram.S- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: Karl.Xiao <Karl.Xiao@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
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
    ldr   r8, =ARCH_us(\t)      /* delay tms */
    mrrc  p15, 0, r9, r10, cr14
    adds  r9, r9, r8            /* Target tick */
    adc   r10, r10, #0x00       /* Add with carry instruction */
delay_retry\l:
    isb sy
    mrrc  p15, 0, r11, r12, cr14
    cmp   r12, r10              /* compare MSB part*/
    blt   delay_retry\l         /* jump to delay_retry in case r10 is bigger (N==1) */
    cmp   r11, r9               /* compare LSB part*/
    blt   delay_retry\l         /* jump to delay_retry in case r9 is bigger (N==1) */
.endm

////RREG_B
.macro  RREG_B  ret, reg
    ldr   r7, =\reg
    ldrb  \ret, [r7]
.endm

////RREG_W
.macro  RREG_W  ret, reg
    ldr   r7, =\reg
    ldr   \ret, [r7]
.endm

////WREG_B
.macro  WREG_B  reg, mask, val
    ldr   r7, =\reg
    ldr   r8, =\mask
    ldr   r9, =\val
    ldrb  r6, [r7]
    bic   r6, r8
    orr   r6, r9
    strb  r6, [r7]
.endm

////WREG_W
.macro  WREG_W  reg, val
    ldr   r6, =\reg
    ldr   r7, =\val
    str   r7, [r6]
.endm

////MBOX
.macro  MBOX  val
    WREG_W  0xfd200800, \val
.endm

ENTRY(sram_suspend_imi)
    stmfd  sp!, {r0-r10, lr}

miu_mask:
    // MBOX    0x5001
    // 04. LPDDR2 enter self-refresh
    WREG_W  0xfd2025c0, 0x0000      //wriu -w   0x1012e0,0x0000
    WREG_W  0xfd20248c, 0xFFFE      //wriu -w   0x101246,0xFFFE
    WREG_W  0xfd2024cc, 0xFFFF      //wriu -w   0x101266,0xFFFF
    WREG_W  0xfd20250c, 0xFFFF      //wriu -w   0x101286,0xFFFF
    WREG_W  0xfd20254c, 0xFFFF      //wriu -w   0x1012A6,0xFFFF
    WREG_W  0xfd2023cc, 0x00FE      //wriu -w   0x1011E6,0x00FE
    // wait 20
    arch_usdelay 100,__LINE__

dram_precharge_all:
    // MBOX    0x5002
    // Pre-charge all then one refresh
    WREG_W  0xfd202430, 0x0400      //wriu -w   0x101218  0x0400
    WREG_B  0xfd202400, 0x30, 0x20  //wriu -b   0x101200  0x30 0x20
    WREG_B  0xfd202401, 0x1e, 0x04  //wriu -b   0x101201  0x1e 0x04
    WREG_B  0xfd202401, 0x01, 0x00  //wriu -b   0x101201  0x01 0x00
    WREG_B  0xfd202401, 0x01, 0x01  //wriu -b   0x101201  0x01 0x01
    WREG_B  0xfd202401, 0x01, 0x00  //wriu -b   0x101201  0x01 0x00
    WREG_B  0xfd202401, 0x1e, 0x02  //wriu -b   0x101201  0x1e 0x02
    WREG_B  0xfd202401, 0x01, 0x00  //wriu -b   0x101201  0x01 0x00
    WREG_B  0xfd202401, 0x01, 0x01  //wriu -b   0x101201  0x01 0x01
    WREG_B  0xfd202401, 0x01, 0x00  //wriu -b   0x101201  0x01 0x00
    // delay, DO NOT REMOVE THIS DELAY!!!!
    arch_usdelay 100,__LINE__

dram_enter_self_refresh:
    // MBOX    0x5003
    WREG_W  0xfd202400, 0x202e      //wriu -w   0x101200  0x202e
    // wait 10
    arch_usdelay 10,__LINE__

ana_pwr_down:
    // MBOX    0x5004
    // 05. AN power down
    WREG_B  0xfd202405, 0xf0, 0xf0  //wriu -b   0x101203 0xF0 0xF0
    WREG_B  0xfd202000, 0x08, 0x08  //wriu -b   0x101000 0x08 0x08
    WREG_B  0xfd202000, 0x10, 0x10  //wriu -b   0x101000 0x10 0x10
    WREG_B  0xfd202001, 0x60, 0x60  //wriu -b   0x101001 0x60 0x60 // sel gpio=1
    WREG_B  0xfd202031, 0x20, 0x20  //wriu -b   0x101019 0x20 0x20 // set gpio_dft_mode 1
    WREG_B  0xfd202031, 0x60, 0x60  //wriu -b   0x101019 0x60 0x60 // sel gpio_dft_mode
    // ATOP_PD
    WREG_B  0xfd2010a8, 0x70, 0x70  //wriu -b   0x101054 0x70 0x70
    // rx disable
    WREG_B  0xfd202010, 0x3f, 0x00  //wriu -b   0x101008 0x3f 0x00

#if IS_ENABLED(CONFIG_SS_LOWPWR_STR)
clk_to_xtal:
    // MBOX    0x5005
    // ckg_mcu switch to xtal
    WREG_B  0xfd207004, 0x10, 0x00  //wriu -b   0x103802 0x10 0x00
    // ckg_miu switch to xtal
    WREG_B  0xfd20705c, 0x10, 0x00  //wriu -b   0x10382e 0x10 0x00
    // ckg_ddr_sync switch to xtal
    WREG_B  0xfd207064, 0x0c, 0x08  //wriu -b   0x103832 0x0c 0x08
    // ARM clock switch to xtal
    WREG_B  0xfd2041f0, 0x01, 0x00  //wriu -b   0x1020f8 0x01 0x00

pll_pwr_down:
    // MBOX    0x5006
    // armpll power down 0x103222 bit[8:7]
    // bit[7] reg_mipspll_en_cpuclk = 0
    // bit[8] reg_mipspll_pd = 1
    WREG_B  0xfd206444, 0x80, 0x00  //wriu -b   0x103222 0x80 0x00
    WREG_B  0xfd206445, 0x01, 0x01  //wriu -b   0x103223 0x01 0x01
    // miupll power down 0x103102 bit[8]
    // bit[8] reg_miu_128bus_pll_pd = 1
    WREG_B  0xfd206205, 0x01, 0x01  //wriu -b   0x103103 0x01 0x01

wait_event:
    // MBOX    0x5007
#if !IS_ENABLED(CONFIG_SS_USB_LOWPWR_SUSPEND)
    // as an example: polling PAD_PM_I2CM_SCL till low
    RREG_B  r0, 0xfd007e44
    and     r0, r0, #0x01
    cmp     r0, #0
#else
    // polling usb link state till not suspend
    RREG_W  r0, 0xfd34470C
    mov     r0, r0, lsr #18
    and     r0, r0, #0x0F
    cmp     r0, #0x03 // stil in U3/suspend state?
    beq     wait_event
    bl      low_pwr_resume
#endif

#else
chip_pwr_down:
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

    // [MUST] t2+t3+t4 > 55ms from scope measurment
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
    ldr   r0, =0x0000
    str   r0, [r2, #0x1E << 1]

rtc_iso:
    // ISO b000
    ldr   r0, =0x0000
    str   r0, [r2, #0x06 << 1]
    //delay 1ms
    ldr   r7, =0x0B
1:
    subs  r7, r7, #1
    beq   rtc_iso
    arch_usdelay 100,__LINE__

    ldr   r0, [r2, #0x10 << 1]
    and   r0, r0, #0x08
    cmp   r0, #0
    bne   1b

    // ISO b001
    ldr   r0, =0x0001
    str   r0, [r2, #0x06 << 1]
    //delay
    ldr   r7, =0x0B
2:
    subs  r7, r7, #1
    beq   rtc_iso
    arch_usdelay 100,__LINE__

    ldr   r0, [r2, #0x10 << 1]
    and   r0, r0, #0x08
    cmp   r0, #8
    bne   2b

    // ISO b011
    ldr   r0, =0x0003
    str   r0, [r2, #0x06 << 1]
    //delay
    ldr   r7, =0x0B
3:
    subs  r7, r7, #1
    beq   rtc_iso
    arch_usdelay 100,__LINE__

    ldr   r0, [r2, #0x10 << 1]
    and   r0, r0, #0x08
    cmp   r0, #0
    bne   3b

    // ISO b111
    ldr   r0, =0x0007
    str   r0, [r2, #0x06 << 1]
    //delay
    ldr   r7, =0x0B
4:
    subs  r7, r7, #1
    beq   rtc_iso
    arch_usdelay 100,__LINE__

    ldr   r0, [r2, #0x10 << 1]
    and   r0, r0, #0x08
    cmp   r0, #8
    bne   4b

    // ISO b101
    ldr   r0, =0x0005
    str   r0, [r2, #0x06 << 1]
    //delay
    ldr   r7, =0x0B
5:
    subs  r7, r7, #1
    beq   rtc_iso
    arch_usdelay 100,__LINE__

    ldr   r0, [r2, #0x10 << 1]
    and   r0, r0, #0x08
    cmp   r0, #0
    bne   5b

    // ISO b001
    ldr   r0, =0x0001
    str   r0, [r2, #0x06 << 1]
    //delay
    ldr   r7, =0x0B
6:
    subs  r7, r7, #1
    beq   rtc_iso
    arch_usdelay 100,__LINE__

    ldr   r0, [r2, #0x10 << 1]
    and   r0, r0, #0x08
    cmp   r0, #8
    bne   6b

    // ISO b000
    ldr   r0, =0x0000
    str   r0, [r2, #0x06 << 1]
    //delay
    ldr   r7, =0x1B
7:
    subs  r7, r7, #1
    beq   rtc_iso
    arch_usdelay 100,__LINE__

    ldr   r0, [r2, #0x10 << 1]
    and   r0, r0, #0x08
    cmp   r0, #0
    bne   7b
#endif

#if IS_ENABLED(CONFIG_SS_LOWPWR_STR)

//--- [LOW POWER MODE RESUME] ---
low_pwr_resume:
    // MBOX    0xA001
    nop
    nop
    nop
    nop

pll_pwr_on:
    // MBOX    0xA002
    // miupll power on 0x103102 bit[8]
    // bit[8] reg_miu_128bus_pll_pd = 0
    WREG_B  0xfd206205, 0x01, 0x00  //wriu -b   0x103103 0x01 0x00
    arch_usdelay 250,__LINE__
    // MBOX    0xA003
    // armpll power on 0x103222 bit[8:7]
    // bit[8] reg_mipspll_pd = 0
    // bit[7] reg_mipspll_en_cpuclk = 1
    WREG_B  0xfd206445, 0x01, 0x00  //wriu -b   0x103223 0x01 0x00
    WREG_B  0xfd206444, 0x80, 0x80  //wriu -b   0x103222 0x80 0x80

clk_restore:
    // MBOX    0xA004
    // ckg_mcu switch to 432M
    WREG_B  0xfd207004, 0x10, 0x10  //wriu -b   0x103802 0x10 0x10
    // ckg_miu switch to miupll
    WREG_B  0xfd20705c, 0x10, 0x10  //wriu -b   0x103817 0x10 0x10
    // ckg_ddr_sync switch to 432M
    WREG_B  0xfd207064, 0x0c, 0x00  //wriu -b   0x103819 0x0c 0x00
    // ARM clock switch to armpll
    WREG_B  0xfd2041f0, 0x01, 0x01  //wriu -b   0x1020f8 0x01 0x01

ana_pwr_on:
    // MBOX    0xA005
    // AN power on
    //Not sel gpio mode
    WREG_B  0xfd202000, 0x08, 0x08  //wriu -b   0x101000 0x08 0x08 // gpio oenz=1
    WREG_B  0xfd202000, 0x10, 0x00  //wriu -b   0x101000 0x10 0x00 // sel gpio=0
    WREG_B  0xfd202001, 0x60, 0x00  //wriu -b   0x101001 0x60 0x00 // sel gpio=0
    WREG_B  0xfd202031, 0x20, 0x20  //wriu -b   0x101019 0x20 0x20 // set gpio_dft_mode 1
    WREG_B  0xfd202031, 0x60, 0x20  //wriu -b   0x101019 0x60 0x20 // sel gpio_dft_mode=0

    arch_usdelay 100,__LINE__

    // Disable OENZ
    WREG_B  0xfd202405, 0xf0, 0x00  //wriu -b   0x101203 0xf0 0x00
    // ATOP_PD on
    WREG_B  0xfd2010a8, 0x70, 0x00  //wriu -b   0x101054 0x70 0x00
    // rx enable
    WREG_B  0xfd202010, 0x3f, 0x3f  //wriu -b   0x101008 0x3f 0x3f // DDR3
    //-----------
    // DQSM RST
    //-----------
    WREG_W  0xfd20203c, 0x0005      //wriu -w   0x10101e  0x0005
    WREG_W  0xfd20203c, 0x000f      //wriu -w   0x10101e  0x000f
    WREG_W  0xfd20203c, 0x0005      //wriu -w   0x10101e  0x0005

dram_exit_self_refresh:
    // MBOX    0xA006
    //---------------------------
    // Exit self-refresh
    //---------------------------
    WREG_B  0xfd202401, 0x20, 0x00  //wriu -b   0x101201  0x20 0x00 //[13]   : reg_self_refresh  = 0
    WREG_W  0xfd2023cc, 0x00fe      //wriu -w   0x1011e6  0x00fe

    //------------------------------------------
    // Insert Refresh cmd & Trun on auto-refresh
    //------------------------------------------
    WREG_B  0xfd202401, 0x1e, 0x02  //wriu -b   0x101201  0x1e 0x02 //[12:9] : reg_single_cmd    = 1 (refresh)
    WREG_B  0xfd202401, 0x01, 0x00  //wriu -b   0x101201  0x01 0x00 //[8]    : reg_single_cmd_en = 0
    WREG_B  0xfd202401, 0x01, 0x01  //wriu -b   0x101201  0x01 0x01 //[8]    : reg_single_cmd_en = 1
    WREG_B  0xfd202401, 0x01, 0x00  //wriu -b   0x101201  0x01 0x00 //[8]    : reg_single_cmd_en = 0
    WREG_B  0xfd202400, 0x30, 0x00  //wriu -b   0x101200  0x30 0x00 //[05]   : reg_auto_ref_off  = 0
                                                                    //[04]   : reg_odt           = 0
    arch_usdelay 100,__LINE__

    //---------------------------
    // Enable ODT
    //---------------------------
    // WREG_B  0xfd202400, 0x10, 0x10  //wriu -b   0x101200  0x10 0x10 //[04]   : reg_odt           = 1
    // arch_usdelay 100,__LINE__

miu_unmask:
    // MBOX    0xA007
    //-------------------------
    // unmask all
    //-------------------------
    // wriu -w 0x10121e  0x8c08 //DC : [3]for mi2ip handshake enable
    // CLEAR above mark by jim
    WREG_W  0xfd20243c, 0x8c08

    WREG_W  0xfd20248c, 0x0000      //wriu -w   0x101246  0x0000
    WREG_W  0xfd2024cc, 0x0000      //wriu -w   0x101266  0x0000
    WREG_W  0xfd20250c, 0x0000      //wriu -w   0x101286  0x0000
    WREG_W  0xfd20254c, 0x0000      //wriu -w   0x1012A6  0x0000
    WREG_W  0xfd2023cc, 0x0000      //wriu -w   0x1011e6  0x0000

    // MBOX    0xA008
    ldmfd  sp!, {r0-r10, pc}
#endif

ENDPROC(sram_suspend_imi)
.ltorg
