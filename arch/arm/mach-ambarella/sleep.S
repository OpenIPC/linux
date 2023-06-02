/*
 * arch/arm/mach-ambarella/sleep.S
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2014-2016, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/cache.h>
#include <mach/hardware.h>
#include <plat/drctl.h>
#include <plat/rct.h>
#include <plat/uart.h>

	.text

ENTRY(ambarella_finish_suspend)
	isb
	dsb
	mov r0, #0
	ldr	r1, =(DRAMC_BASE + DRAM_DDRC_OFFSET)
	ldr	r2, =ANA_PWR_REG

	teq	r0, r0
	bl _ambarella_finish_suspend
	teq	pc,	r0
	bl _ambarella_finish_suspend

ENDPROC(ambarella_finish_suspend)

	/* _ambarella_finish_suspend will be called twice. first is to makesure the
	 * instructions loading in the I-Cache, and the second is to disconnect cpu
	 * with dram, enter self refresh, and power down PWC.
	 */
ENTRY(_ambarella_finish_suspend)
	ldrne	r3, [r1, #DDRC_CTL_OFFSET]
	bicne	r3, r3, #0x1
	strne	r3, [r1, #DDRC_CTL_OFFSET]
	movne	r3, #0x80000000
	strne	r3, [r1, #DDRC_SELF_REF_OFFSET]

	movne	r3, #ANA_PWR_POWER_DOWN
	strne	r3, [r2]
	moveq	pc, lr
	b		.

ENDPROC(_ambarella_finish_suspend)

ENTRY(ambarella_optimize_suspend)
	dsb
	isb

	mov	r1, #0
	cmp	r0, r1
	beq	suspend_with_pwc

	mov	r1, r0
	mov	r2, #0x1f
	bic	r0, r2			/* r0 gpio bank base */
	and	r1, r2			/* r1 gpio offset */

	ldr	r2, ambarella_suspend_trigger_signal
	lsl	r2, r1
	str	r2, [r0, #0]

	/* delay 100ms: 0x249F00 is about 0x250000 */
	mov	r3, #0x250000

	mov	r4,	#RCT_BUS_BASE
	orr	r4, #RCT_OFFSET
	orr	r4,	#RCT_TIMER_OFFSET
	ldr	r5,	[r4]
	add	r5, r5, r3
rct_time_loop:
	ldr	r6,	[r4]
	cmp	r5, r6
	bhi	rct_time_loop

	ldr	r2, ambarella_suspend_trigger_signal
	eor r2, r2, #1
	lsl	r2, r1
	str	r2, [r0, #0]

	b	self_refresh

suspend_with_pwc:
	mov	r0,	#RCT_BUS_BASE
	orr	r0, #RCT_OFFSET
	orr	r0, #ANA_PWR_OFFSET

	/* Generate the PD signal */
	mov	r1, #ANA_PWR_POWER_DOWN
	str	r1, [r0]

self_refresh:
	/* CPU use the delay interval between power-down signal PD
	   and PWC_RSTOB to execute the following code */

	mov	r0, #DRAMC_BASE
	orr	r0, r0, #DRAM_DDRC_OFFSET

	ldr	r1, [r0, #DDRC_CTL_OFFSET]
	bic	r1, r1, #0x1
	str	r1, [r0, #DDRC_CTL_OFFSET]
	mov	r1, #0x80000000
	str	r1, [r0, #DDRC_SELF_REF_OFFSET]
1:
	ldr	r1, [r0, #DDRC_SELF_REF_OFFSET]
	tst	r1, #0x10000000
	beq	1b

	/* Wait for PWC_RSTOB signal to power down cpu */
	b	.

ENDPROC(ambarella_optimize_suspend)

.globl ambarella_suspend_trigger_signal
ambarella_suspend_trigger_signal:
	.word	0x00000000

ENTRY(ambarella_optimize_suspend_sz)
	.word 	. - ambarella_optimize_suspend

#if (CHIP_REV == S3)
#define	IOMUX_UART0_PIN_CFG	0x00000030
#else
#define	IOMUX_UART0_PIN_CFG	0x00000180
#endif

ENTRY(ambarella_cpu_resume)
	/*
	 * init UART(115200, 8N1) to avoid deadloop in printk.
	 * MMU is not enabled yet, so we use physical address here.
	 */
	ldr	r0, =RCT_PHYS_BASE
	mov	r1, #0x1
	str	r1, [r0, #CG_UART_OFFSET]

	ldr	r0, =(APB_PHYS_BASE + UART_OFFSET)
	mov	r1, #0x1
	str	r1, [r0, #UART_SRR_OFFSET]
	mov	r1, #0x0
	str	r1, [r0, #UART_SRR_OFFSET]
	mov	r1, #0x80
	str	r1, [r0, #UART_LC_OFFSET]
	ldr	r1, =(REF_CLK_FREQ/16/115200)
	str	r1, [r0, #UART_DLL_OFFSET]
	mov	r1, #0x00
	str	r1, [r0, #UART_DLH_OFFSET]
	mov	r1, #0x03
	str	r1, [r0, #UART_LC_OFFSET]

	/* configure Tx/Rx pin as hw mode, it's chip specific. */
#if (IOMUX_SUPPORT > 0)
	/* gpio39/40 are used for UART0_rx/tx */
	ldr	r0, =(APB_PHYS_BASE + IOMUX_OFFSET)
	/* read-modify-write */
	ldr	r1, [r0, #IOMUX_REG1_0_OFFSET]
	orr	r1, r1, #IOMUX_UART0_PIN_CFG
	str	r1, [r0, #IOMUX_REG1_0_OFFSET]
	/* read-modify-write */
	ldr	r1, [r0, #IOMUX_REG1_1_OFFSET]
	bic	r1, r1, #IOMUX_UART0_PIN_CFG
	str	r1, [r0, #IOMUX_REG1_1_OFFSET]
	/* read-modify-write */
	ldr	r1, [r0, #IOMUX_REG1_2_OFFSET]
	bic	r1, r1, #IOMUX_UART0_PIN_CFG
	str	r1, [r0, #IOMUX_REG1_2_OFFSET]
	ldr	r1, =0x00000001
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
	ldr	r1, =0x00000000
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
#else
	/* gpio14/15 are used for UART0_tx/rx */
	ldr	r0, =(APB_PHYS_BASE + GPIO0_OFFSET)
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	orr	r1, #0x0000c000
	str	r1, [r0, #GPIO_AFSEL_OFFSET]

#if (CHIP_REV == S2E)
	ldr	r0, =RCT_PHYS_BASE
	mov r1, #UART_CLK_SRC_IDSP
	str	r1, [r0, #UART_CLK_SRC_SEL_OFFSET]
#endif

#endif
	/* jump to generic resume */
	b	cpu_resume
ENDPROC(ambarella_cpu_resume)

