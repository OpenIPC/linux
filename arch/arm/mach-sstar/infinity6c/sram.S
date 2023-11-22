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
#include "mailbox_str.h"

#include "mmu_v7.S"

#define ARCH_us(x) (6*x)
    .align 3
.globl sram_suspend_imi
.globl v7_cpu_resume

#define RESUME_C  0xFD200804

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

////WREG
.macro  WREG    reg_addr32, vall
    ldr     r7, =\reg_addr32
    ldr     r6, =\vall
    str     r6, [r7]
    b 1011f
    .ltorg
1011:
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

////WREG_E
.macro  WREG_E  reg_addr32, val
    ldr   r7, =\reg_addr32
    ldr   r6, =\val
    strb  r6, [r7]
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

////WREG_W_MSK
.macro  WREG_W_MSK  reg, mask, val
    ldr   r7, =\reg
    ldr   r8, =\mask
    ldr   r9, =\val
    ldr   r6, [r7]
    bic   r6, r8
    orr   r6, r9
    str   r6, [r7]
.endm

////MBOX
.macro  MBOX  val
    WREG_W  0xfd200800, \val
.endm


ENTRY(sram_suspend_imi)
    MBOX SUSPEND_SRAM_0
    mov r0, #0x1 << 30
    vmsr fpexc, r0

    ldr      r1, =RESUME_C
    ldrh     r0, [r1,#0x08]
    ldrh     r2, [r1,#0x0C]
    add      r2, r2,r0,lsl #16
    mov      sp, r2

    // Save TTBR0
    mmu_get_mmu_table_addr r0
    ldr      r1, =RESUME_C
    strh     r0, [r1,#0x0C]
    lsr      r0, r0, #16
    strh     r0, [r1,#0x08]

    mmu_table_init
    v7_dcache_flush_all __LINE__
    switch_mmu_table_to_imi

    WREG_B    0xFD204404, 0x04, 0x04

    // set debug flag
    MBOX SUSPEND_SRAM_0

    // 1. DDR enter self-refresh
    WREG_W    0xFD2025C0, 0x0000
    WREG_W    0xFD2025C0, 0x0000
    //wriu -w   0x161000  0x0230
    WREG_W    0xFD2C2000, 0x0230
    //wriu -w   0x161002  0x0230
    WREG_W    0xFD2C2004, 0x0230
    //wriu -w   0x161004  0x0230
    WREG_W    0xFD2C2008, 0x0230
    //wriu -w   0x16103e  0x0001
    WREG_W    0xFD2C207C, 0x0001
    //wriu -w   0x161040  0x0230
    WREG_W    0xFD2C2080, 0x0230
    //wriu -w   0x161042  0x0230
    WREG_W    0xFD2C2084, 0x0230
    //wriu -W   0x161044  0x0230
    WREG_W    0xFD2C2088, 0x0230
    //wriu -w   0x16107e  0x0001
    WREG_W    0xFD2C20FC, 0x0001

    MBOX SUSPEND_SRAM_1
    //delay
    arch_usdelay 1000,__LINE__
    MBOX SUSPEND_SRAM_2

    ldr   r1, =0xFD000000
    ldr   r3, =0x101200
    ldr   r4, =0x101100
    ldr   r5, =0x101000
    add   r2, r1, r3, lsl #1
    ldr   r0, =0x0000

    // Pre-charge all then one refresh
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
    MBOX SUSPEND_SRAM_3
    //wriu -b   0x101201  0x01 0x00
    ldrb  r0, [r2, #0x01]
    and   r0, r0, #0xFE
    strb  r0, [r2, #0x01]
    //wriu -b   0x101201  0x01 0x01
    MBOX SUSPEND_SRAM_4
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
    MBOX SUSPEND_SRAM_5
    ldrb  r0, [r2, #0x01]
    orr   r0, r0, #0x01
    strb  r0, [r2, #0x01]
    //wriu -b   0x101201  0x01 0x00
    ldrb  r0, [r2, #0x01]
    and   r0, r0, #0xFE
    strb  r0, [r2, #0x01]
    //delay, DO NOT REMOVE THIS DELAY!!!!
    arch_usdelay 1000,__LINE__
    MBOX SUSPEND_SRAM_6
    //Enter self-refresh0
    //wriu -w   0x101200  0x202e
    ldr   r0, =0x202e
    str   r0, [r2, #0x00 << 1]
    //delay
    arch_usdelay 1000,__LINE__

    // AN power down
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

    MBOX SUSPEND_SRAM_7

    //wriu -b 0x101054 0x70 0x70
    ldrb  r0, [r2, #0xA8]
    orr   r0, r0, #0x70
    strb  r0, [r2, #0xA8]
    //wriu -b 0x101008 0x3F 0x00
    ldrb  r0, [r2, #0x10]
    and   r0, r0, #0xC0
    strb  r0, [r2, #0x10]
    MBOX SUSPEND_SRAM_8

    //Switch miu clock to xtal clock (24M)
    //Write_bank: 1038, offset: 0x17 0x10 //sel miu to 24M
    WREG_W_MSK  0xfd20705c, 0x0010, 0x0000
    // Note: 20020923 update -> per designer's comment, no need to set ddr sync clock.
    //Switch ddr sync clock to xtal clock (12M)
    //Write_bank: 1038, offset: 0x19 0x08 //sel ddr sync to 12M
    //WREG_W_MSK  0xfd207064, 0x000F, 0x0008
    //Switch mcu clock to xtal/2 clock (12M)
    //Write_bank: 1038, offset: 0x01 0x00 //sel mcu to 12M
    WREG_W_MSK  0xfd207004, 0x0020, 0x0000
    //Modify CPU clk source to xtal
    //switch L3 bridge clock to xtal (reg_clk_miu2x_sel:0)
    WREG_B    0xFD204404, 0x80, 0x00
    //  bank: 1020, offset: 0x6a, bit0 set0x01 // ?? remove
    WREG_W_MSK  0xfd2041A8, 0x0001, 0x0001
    //  bank: 1020, offset: 0x7d, bit0 set0x01 // clock cpu select debug clock or xtal
    WREG_W_MSK  0xfd2041F4, 0x0001, 0x0001
    //  bank: 1020, offset: 0x7e, bit0 set0x01 // debug clock cpu select PLL/2 or MPLL
    WREG_W_MSK  0xfd2041F8, 0x0001, 0x0001
    //  bank: 1020, offset: 0x7c, bit0 set0x00 // clock cpu select PLL
    WREG_W_MSK  0xfd2041F0, 0x0001, 0x0000
#if defined(CONFIG_SS_PM_VAD_LOOP_MODE) || defined(CONFIG_SS_PM_VAD_INT_MODE)
    //Change UPLL clk source to 24M_CORE
    //UPLL: bank: 1420, offset: 0x00, bit7 set0x1, enable UPLL xtal mode
    WREG_W_MSK    0xFD284000, 0x0080, 0x0080  //    wriu    0x00142000 0xf2
    // Enable CLKO_UPLL_AUDIO_HV
    //UPLL: bank: 1420, offset: 0x07, bit0~7 set0x10
    WREG_W_MSK    0xFD28401C, 0x00FF, 0x0010  //    wriu    0x00142000 0xf2
#endif

    //wriu -b 0x10108b 0x02 0x02 // [9] treeLDO PD
    WREG_W_MSK    0xFD202114, 0x0200 0x0200
    //wriu -b 0x101033 0x40 0x40 // [15:14] MasterPLL  pd
    WREG_W_MSK    0xFD202064, 0x4000 0x4000
    //wriu -b 0x101033 0x80 0x80 // [15:14] SlavePLL  pd
#if 1 // CRITICAL: DO NOT enable this, power down SlavePLL cauese DDR unstable after resume ???!!!
    WREG_W_MSK    0xFD202064, 0x8000 0x8000
#endif

    //wriu -b 0x10108b 0x02 0x02 // PD tree LDO
    WREG_W_MSK    0xFD202114, 0x0200 0x0200
    //wriu -b 0x101088 0xff 0x00 // PD RX LDO
    WREG_W_MSK    0xFD202110, 0x00FF 0x0000
    //wriu -b 0x10100e 0x01 0x00 // en_mask off
    WREG_W_MSK    0xFD20201C, 0x0001 0x0000
    //wriu -b 0x101019 0x60 0x60 // GPIO_DFT input mode
    WREG_W_MSK    0xFD202030, 0x6000 0x6000

#if CONFIG_SS_PM_POWER_SAVING_SCHEME
#if 0 //for AVDD_SAR power measurement
    // set strap pin to input
    WREG_W_MSK    0xFD2078E4, 0x00FF 0x00ff  //wriu    0x00103c72 0xff
    WREG_W_MSK    0xFD207C88, 0x00FF 0x00dc  //wriu    0x00103e44 0xdc
    WREG_W_MSK    0xFD207C8C, 0x00FF 0x00dc  //wriu    0x00103e46 0xdc
    WREG_W_MSK    0xFD207C90, 0x00FF 0x00dc  //wriu    0x00103e48 0xdc
    WREG_W_MSK    0xFD207C94, 0x00FF 0x00dc  //wriu    0x00103e4a 0xdc
    WREG_W_MSK    0xFD207C98, 0x00FF 0x00dc  //wriu    0x00103e4c 0xdc
    WREG_W_MSK    0xFD207C9C, 0x00FF 0x00dc  //wriu    0x00103e4e 0xdc
    WREG_W_MSK    0xFD207CA0, 0x00FF 0x00dc  //wriu    0x00103e50 0xdc
#endif

#ifndef CONFIG_SS_PM_VAD_LOOP_MODE
    // AUDIO power down --audio_pd.txt
    WREG_B    0xFD20680C, 0xFF, 0xFF  //wriu 0x00103406 0xFF
    WREG_B    0xFD20680D, 0xFF, 0xFF  //wriu 0x00103407 0xFF
    WREG_B    0xFD20682C, 0xAD, 0xAD  //wriu 0x00103416 0xAD
    WREG_B    0xFD20680D, 0x01, 0x01  //wriu 0x00103407 0x01
#endif

    MBOX SUSPEND_SRAM_9

    // ETH power down
    //_MHal_EMAC_albany_clk_off

    MBOX SUSPEND_SRAM_A

    // USB2.0 P1 power down
    WREG_W_MSK    0xFD284400, 0x00FF, 0x00FF //    wriu    0x00142200 0xff
    WREG_W_MSK    0xFD284404, 0x00FF, 0x0000 //    wriu    0x00142202 0x00
    WREG_W_MSK    0xFD284210, 0x00FF, 0x000F //    wriu    0x00142108 0x0f
    WREG_W_MSK    0xFD284210, 0xFF00, 0x0400 //    wriu    0x00142109 0x04
    WREG_W_MSK    0xFD284240, 0x00FF, 0x00A1 //    wriu    0x00142120 0xa1
    WREG_W_MSK    0xFD284240, 0xFF00, 0x8000 //    wriu    0x00142121 0x80
    WREG_W_MSK    0xFD284244, 0x00FF, 0x0088 //    wriu    0x00142122 0x88
    WREG_W_MSK    0xFD284244, 0xFF00, 0x2000 //    wriu    0x00142123 0x20
    WREG_W_MSK    0xFD284200, 0x00FF, 0x0007 //    wriu    0x00142100 0x07
    WREG_W_MSK    0xFD284200, 0xFF00, 0xFF00 //    wriu    0x00142101 0xff
    WREG_W_MSK    0xFD284418, 0x00FF, 0x0040 //    wriu    0x0014220c 0x40
    WREG_W_MSK    0xFD284418, 0xFF00, 0x0000 //    wriu    0x0014220d 0x00
    // form Analog RD
    WREG_W_MSK    0xFD284210, 0x00FF, 0x0080  //wriu 0x00142108 0x80
    WREG_W_MSK    0xFD284210, 0xFF00, 0x0000  //wriu 0x00142109 0x00
    WREG_W_MSK    0xFD284220, 0x00FF, 0x0000  //wriu 0x00142110 0x00
#ifndef CONFIG_SS_PM_USB_SUSPEND_MODE
    // upll_pd.txt
    //WREG_W_MSK    0xFD223624, 0x00FF, 0x0000  //    wriu    0x00111b12 0x00 // reg_pd_xal_hv
    // mpll power down
    // 8: reg_pd_mpll
    // 9: reg_pd_mpll_clk_adc_vco_div2
    // 10: reg_pd_mpll_clk_adc_vco_div2_2
    // 11: reg_pd_mpll_clk_adc_vco_div_2_3
    // 12: reg_pd_digclk
    //WREG_W_MSK    0xFD206004, 0xFF00, 0x0000  //    wriu    0x00103003 0x00
    //set upll power down to 1
    WREG_W_MSK    0xFD284000, 0x00FF, 0x00F2  //    wriu    0x00142000 0xf2
    WREG_W_MSK    0xFD284000, 0xFF00, 0x1100  //    wriu    0x00142001 0x11
    WREG_W_MSK    0xFD284000, 0x00FF, 0x00b2  //    wriu    0x00142000 0xb2
    WREG_W_MSK    0xFD284000, 0xFF00, 0x1100  //    wriu    0x00142001 0x11
    WREG_W_MSK    0xFD284000, 0x00FF, 0x00b2  //    wriu    0x00142000 0xb2
    WREG_W_MSK    0xFD284000, 0xFF00, 0x1100  //    wriu    0x00142001 0x11
    WREG_W_MSK    0xFD284000, 0x00FF, 0x0082  //    wriu    0x00142000 0x82
    WREG_W_MSK    0xFD284000, 0xFF00, 0x1000  //    wriu    0x00142001 0x10
    WREG_W_MSK    0xFD284004, 0x00FF, 0x0098  //    wriu    0x00142002 0x98
    WREG_W_MSK    0xFD284004, 0xFF00, 0x1400  //    wriu    0x00142003 0x14
    WREG_W_MSK    0xFD284004, 0x00FF, 0x0098  //    wriu    0x00142002 0x98
    WREG_W_MSK    0xFD284004, 0xFF00, 0x1400  //    wriu    0x00142003 0x14
    WREG_W_MSK    0xFD28401C, 0x00FF, 0x0011  //    wriu    0x0014200e 0x11
    WREG_W_MSK    0xFD28401C, 0xFF00, 0x0000  //    wriu    0x0014200f 0x00
#endif
    MBOX SUSPEND_SRAM_B
   //MIPIRX
#if 1
    WREG_W_MSK 0xFD2A7664, 0x00ff, 0x0000
    WREG_W_MSK 0xFD2A7664, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A7004, 0x00ff, 0x0003
    WREG_W_MSK 0xFD2A7004, 0xff00, 0x1000
    WREG_W_MSK 0xFD2A7000, 0x00ff, 0x0043
    WREG_W_MSK 0xFD2A7000, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A7E04, 0x00ff, 0x0007
    WREG_W_MSK 0xFD2A7E04, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A7070, 0x00ff, 0x0000
    WREG_W_MSK 0xFD2A7070, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A7000, 0x00ff, 0x0043
    WREG_W_MSK 0xFD2A7000, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A780C, 0x00ff, 0x0000
    WREG_W_MSK 0xFD2A780C, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A700C, 0x00ff, 0x0012
    WREG_W_MSK 0xFD2A700C, 0xff00, 0x0f00
    WREG_W_MSK 0xFD2A700C, 0x00ff, 0x0012
    WREG_W_MSK 0xFD2A700C, 0xff00, 0x0f00
    WREG_W_MSK 0xFD2A7000, 0x00ff, 0x0043
    WREG_W_MSK 0xFD2A7000, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A7804, 0x00ff, 0x0000
    WREG_W_MSK 0xFD2A7804, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A7000, 0x00ff, 0x0043
    WREG_W_MSK 0xFD2A7000, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A7004, 0x00ff, 0x0003
    WREG_W_MSK 0xFD2A7004, 0xff00, 0x1000
    WREG_W_MSK 0xFD2A7004, 0x00ff, 0x0003
    WREG_W_MSK 0xFD2A7004, 0xff00, 0x1000
    WREG_W_MSK 0xFD2A70A0, 0x00ff, 0x0020
    WREG_W_MSK 0xFD2A70A0, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A70A0, 0x00ff, 0x0020
    WREG_W_MSK 0xFD2A70A0, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A708C, 0x00ff, 0x0020
    WREG_W_MSK 0xFD2A708C, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A7080, 0x00ff, 0x0020
    WREG_W_MSK 0xFD2A7080, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A7030, 0x00ff, 0x0020
    WREG_W_MSK 0xFD2A7030, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A7020, 0x00ff, 0x0020
    WREG_W_MSK 0xFD2A7020, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A7010, 0x00ff, 0x0020
    WREG_W_MSK 0xFD2A7010, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A700C, 0x00ff, 0x0002
    WREG_W_MSK 0xFD2A700C, 0xff00, 0x0f00
    WREG_W_MSK 0xFD2A700C, 0x00ff, 0x0082
    WREG_W_MSK 0xFD2A700C, 0xff00, 0x0f00
    WREG_W_MSK 0xFD2A7000, 0x00ff, 0x0043
    WREG_W_MSK 0xFD2A7000, 0xff00, 0x3f00
    WREG_W_MSK 0xFD2A7078, 0x00ff, 0x0000
    WREG_W_MSK 0xFD2A7078, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A705c, 0x00ff, 0x0000
    WREG_W_MSK 0xFD2A705c, 0xff00, 0x0000
    WREG_W_MSK 0xFD2A70A4, 0x00ff, 0x0000
    WREG_W_MSK 0xFD2A70A4, 0xff00, 0x0800
    WREG_W_MSK 0xFD2A7090, 0x00ff, 0x0000
    WREG_W_MSK 0xFD2A7090, 0xff00, 0x0800
    WREG_W_MSK 0xFD2A7084, 0x00ff, 0x0000
    WREG_W_MSK 0xFD2A7084, 0xff00, 0x0800
    WREG_W_MSK 0xFD2A7034, 0x00ff, 0x0000
    WREG_W_MSK 0xFD2A7034, 0xff00, 0x0800
    WREG_W_MSK 0xFD2A7024, 0x00ff, 0x0000
    WREG_W_MSK 0xFD2A7024, 0xff00, 0x0800
    WREG_W_MSK 0xFD2A7014, 0x00ff, 0x0000
    WREG_W_MSK 0xFD2A7014, 0xff00, 0x0800

#endif
    // SAR power down - ememory_deep_standby.txt (OTP deep standby)
    WREG_W_MSK    0xFD203004, 0x00FF, 0x0008  //wriu    0x00101802 0x00
    WREG_W_MSK    0xFD203004, 0xFF00, 0x8000  //wriu    0x00101803 0x80
    WREG_W_MSK    0xFD203000, 0x00FF, 0x0000  //wriu    0x00101800 0x00
    WREG_W_MSK    0xFD203000, 0xFF00, 0x9500  //wriu    0x00101801 0x95
    WREG_W_MSK    0xFD203000, 0x00FF, 0x0005  //wriu    0x00101800 0x05
    WREG_W_MSK    0xFD203004, 0x00FF, 0x0000  //wriu    0x00101802 0x00
    WREG_W_MSK    0xFD203004, 0x00FF, 0x0001  //wriu    0x00101802 0x01
    WREG_W_MSK    0xFD203004, 0x00FF, 0x0000  //wriu    0x00101802 0x00
    MBOX SUSPEND_SRAM_C

    //Power down PLL(mpll/miupll/cpupll/lpll), only AUPLL is alive
    //For imi clk: bank: 103f, offset: 0x20, set0 for clk_miu change to clk_boot
    WREG_W_MSK  0xfd207E80, 0x00FF, 0x0000
    //LPLL: bank: 1037, offset: 0x40, bit15 set1
    WREG_W_MSK  0xfd206F00, 0x8000, 0x8000
    //IPUPLL: bank: 1035, offset: 0x11, bit8 set1
    WREG_W_MSK  0xfd206A44, 0x0100, 0x0100
    //MIUPLL: bank: 1031, offset: 0x01, bit8 set1
    WREG_W_MSK  0xfd206204, 0x0100, 0x0100
    //SYNTH PLL: bank: 1039, offset: 0x36, bit0 set1
    WREG_W_MSK  0xfd2072D8, 0x0001, 0x0001

    //MIPSPLL: bank: 1032, offset: 0x11, bit8 set1
    WREG_W_MSK  0xfd206444, 0x0100, 0x0100
    //MPLL: bank: 1030, offset: 0x01, bit8 set1
    WREG_W_MSK  0xfd206004, 0x0100, 0x0100

    MBOX SUSPEND_SRAM_D


    // xtal_pd.txt - analog xtal power down from HW RD
    //wriu    0x00111b12 0xff
    //wriu    0x00111b13 0x00
    WREG    0xFD223624, 0x00FF

#if CONFIG_SS_PM_VAD_INT_MODE // XTAL gating - VAD INT mode
    // 9.  Gate xtal to wait vad_interrupt
    // Write_bank: 101e, offset: 0x26, bit0 set1 //reg_vad_gpi_en
    // Write_bank: 101e, offset: 0x27, bit0 set0 //xtal usb suspend mode
    WREG_W_MSK    0xFD203C98, 0x0101 0x0001
    // Write_bank: 101e, offset: 0x2d, bit0 set1 //open wakeup interrupt to int_ctrl
    WREG_W_MSK    0xFD203CB0, 0x0100 0x0100
#endif

    MBOX SUSPEND_SRAM_E

#if CONFIG_SS_PM_USB_SUSPEND_MODE // XTAL gating - USB suspend mode
    // USB suspend setting :
    // 1.  USB Suspend Function Enable
    //  Set 0x142500[8] = 1
    WREG_W_MSK    0xFD284A00, 0x0100 0x0100
    // 2.  USB Suspend IRQ Enable
    //  Set 0x142505[8] = 1
    WREG_W_MSK    0xFD284A14, 0x0100 0x0100
    // 3.  USB Resume IRQ Enable
    //  Set 0x142305[2] = 1
    WREG_W_MSK    0xFD284614, 0x0004 0x0004
    // 4.  USB Reset IRQ Enable
    //  Set 0x142305[3] = 1
    WREG_W_MSK    0xFD284614, 0x0008 0x0008
    // Enter power saving setting :
    //  Write_bank: 1009, offset: 0x54, 0xfdff //int_ctrl fiq masl
    //  Write_bank: 101e, offset: 0x26, 0x0000 //reg_vad_gpi , mask VAD int src
    //  Write_bank: 101e, offset: 0x27, bit0 set1 //reg_xtal_usb_suspend_mode
    WREG_W_MSK    0xFD203C98, 0x0101 0x0100
    //  Write_bank: 101e, offset: 0x2e, set xx //set debounce time (base on wakeup 
    WREG_W_MSK    0xFD203CB8, 0x00FF 0x0010
#endif

#endif

    WREG   0xfd2008B4, 0x0000

wait_event:
    MBOX SUSPEND_SRAM_F
#if defined(CONFIG_SS_PM_VAD_INT_MODE) || defined(CONFIG_SS_PM_USB_SUSPEND_MODE)
    // Write_bank: 101e, offset: 0x24, set 0x1111 //gating for 12/24Mhz
    WREG    0xFD203C90, 0x1111
    WREG    0xFD203C90, 0x1111
    WREG    0xFD203C90, 0x1111
#endif

#if !defined(CONFIG_SS_PM_VAD_INT_MODE) && !defined(CONFIG_SS_PM_USB_SUSPEND_MODE)
    wfi
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
#endif

#if CONFIG_SS_PM_VAD_INT_MODE || CONFIG_SS_PM_USB_SUSPEND_MODE
    // Write_bank: 101e, offset: 0x24, set 0x1111 //gating for 12/24Mhz
    WREG    0xFD203C90, 0x0000
    WREG    0xFD203C90, 0x0000
    WREG    0xFD203C90, 0x0000
#endif

    MBOX WAKEUP_SRAM_0

    ldr r1, =0xfd2008B4
    ldr r2, [r1, #0x0]
    add r2,r2,#1
    str r2 ,[r1]
    nop
    nop
    nop
    b     human_detection



human_detection:

    // latch audio-in end address
    //WREG_W  0xFD2A0BE4, 0xc020
    //WREG_W  0xFD2A0BE4, 0x4020

#if CONFIG_SS_PM_VAD_INT_MODE
    // Audio power up
    WREG_W_MSK  0xFD20680C, 0xFFFF, 0xE712
    // Enable Audio In
    WREG_W_MSK  0xFD2A0BE4, 0x8000, 0x8000
    WREG_W_MSK  0xFD2A0AA4, 0x8000, 0x8000
#endif

    MBOX WAKEUP_SRAM_3
    ldr      r5, =0xF9008000 //run_lib and enable pll
    blx r5

    MBOX WAKEUP_SRAM_4

#ifndef CONFIG_SS_PM_POWER_SAVING_DEBUG_MODE
#if defined(CONFIG_SS_PM_VAD_LOOP_MODE) || defined(CONFIG_SS_PM_VAD_INT_MODE)
    WREG_W 0xFD207a8C, 0xFFFF //clear gpi

    v7_dcache_flush_all __LINE__

    // re-trigger Audio
    WREG_W_MSK  0xFD2A0AA4, 0x8000, 0x8000

    RREG_W r0, 0xFD2008B8
    cmp r0, #0
    beq wait_event
#endif
#endif

low_pwr_resume:
    MBOX WAKEUP_SRAM_5
    nop
    nop
    nop
    nop
    v7_dcache_flush_all __LINE__

    MBOX    WAKEUP_SRAM_7

    // switch mmu table back to saved miu address
    ldr      r1, =RESUME_C
    ldrh     r0, [r1,#0x08]
    ldrh     r2, [r1,#0x0C]
    add      r2, r2,r0,lsl #16
    switch_mmu_table_to_miu r2

    MBOX    WAKEUP_SRAM_8

    ldr      r1, =RESUME_C
    ldrh     r0, [r1,#0x00]
    ldrh     r4, [r1,#0x04]
    add      r4, r4,r0,lsl #16

    MBOX    WAKEUP_SRAM_9

    bx r4  //jump to sram_resume_imi

ENDPROC(sram_suspend_imi)
.ltorg
