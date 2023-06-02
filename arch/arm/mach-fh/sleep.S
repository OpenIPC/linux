#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/chip.h>
#include <mach/io.h>
#include <mach/ddrc.h>

#ifdef CONFIG_PM
#define PMU_MASK_SWITCH_PLL0	0x1
#define PMU_MASK_DDR_SEL		0x1000000
#define PMU_MASK_DDR_DIV		0xff
#define PMU_MASK_PLL1_PDN		0x80000000


	.macro wait_ddrc_idle
1:	ldr	r3, [r1, #OFFSET_DENAL_CTL_57]
	tst	r3, #DDRC_CONTROLLER_BUSY
	bne	1b
	.endm


	.macro enable_ddrc_selfrefresh
	ldr r3, .fh_ddrc_cmd_en_self_refresh
	str	r3, [r1, #OFFSET_DENAL_CTL_31]
	.endm


	.macro wait_ddrc_cke
1:	ldr	r3, [r1, #OFFSET_DENAL_CTL_97]
	tst	r3, #DDRC_CKE_STATUS
	bne	1b
	.endm


	.macro disable_ddrc_selfrefresh
	ldr r3, .fh_ddrc_cmd_dis_self_refresh
	str	r3, [r1, #OFFSET_DENAL_CTL_31]
	.endm


	.macro ddr_to_pll0
	ldr r3, [r2, #REG_PMU_CLK_SEL]
	bic	r3, r3, #PMU_MASK_DDR_SEL
	str	r3, [r2, #REG_PMU_CLK_SEL]
	.endm


	.macro ddr_to_pll1
	ldr r3, [r2, #REG_PMU_CLK_SEL]
	orr	r3, r3, #PMU_MASK_DDR_SEL
	str	r3, [r2, #REG_PMU_CLK_SEL]
	.endm

#if 1
	.macro ddr_dec_feq
	ldr r3, [r2, #REG_PMU_CLK_DIV1]
	orr	r3, r3, #PMU_MASK_DDR_DIV
	str	r3, [r2, #REG_PMU_CLK_SEL]
	.endm


	.macro ddr_inc_feq
	ldr r3, [r2, #REG_PMU_CLK_DIV1]
	bic	r3, r3, #PMU_MASK_DDR_DIV
	orr	r3, r3, #0x1
	str	r3, [r2, #REG_PMU_CLK_SEL]
	.endm


	.macro pll1_power_down
	ldr r3, [r2, #REG_PMU_PLL1_CTRL]
	bic	r3, r3, #PMU_MASK_PLL1_PDN
	str	r3, [r2, #REG_PMU_PLL1_CTRL]
	.endm


	.macro pll1_power_on
	ldr r3, [r2, #REG_PMU_PLL1_CTRL]
	orr	r3, r3, #PMU_MASK_PLL1_PDN
	str	r3, [r2, #REG_PMU_PLL1_CTRL]
	.endm
#endif

	.text
ENTRY(fh_cpu_suspend)

	stmfd	sp!, {r0-r12, lr}		@ save registers on stack

	/*

	 * Register usage:
	 *  R1 = Base address of DDRC
	 *  R2 = Base register for PMU
	 *  R3 = temporary register
	 *  R4 = temporary register
	 *
	 *  R9 = Test address
	 */

	ldr	r1, .fh_va_base_ddrc
	ldr	r2, .fh_va_base_pmu
	ldr r9, .fh_va_test_addr

	wait_ddrc_idle
	enable_ddrc_selfrefresh
	wait_ddrc_cke

	@ddr_dec_feq
	ddr_to_pll0

	@pll1_power_down

	mcr p15, 0, r0, c7, c10, 4		@ Data Synchronization Barrier operation
	mcr	p15, 0, r0, c7, c0, 4		@ Wait-for-Interrupt

	@pll1_power_on

    @ddr_inc_feq
	ddr_to_pll1

	disable_ddrc_selfrefresh

    ldmfd   sp!, {r0-r12, pc}

ENDPROC(fh_cpu_suspend)

.fh_va_base_ddrc:
	.word VA_DDRC_REG_BASE

.fh_va_base_pmu:
	.word VA_PMU_REG_BASE

.fh_va_test_addr:
	.word 0xc03efef0

.fh_ddrc_cmd_en_self_refresh:
    .word 0x3000a01

.fh_ddrc_cmd_dis_self_refresh:
    .word 0x3000101

ENTRY(fh_cpu_suspend_sz)
	.word	. - fh_cpu_suspend
ENDPROC(fh_cpu_suspend_sz)
#endif
