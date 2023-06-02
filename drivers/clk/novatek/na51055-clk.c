/**
    NVT clock management module for NA51055 SoC
    @file na51055-clk.c

    Copyright Novatek Microelectronics Corp. 2019. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <linux/spinlock.h>
#include <linux/of_address.h>
#include <plat/cg-reg.h>
#include "nvt-im-clk.h"
#include "nvt-im-maxfreq.h"
#include <linux/of.h>
#include <mach/rcw_macro.h>
#include <plat/top.h>
#include <plat/efuse_protected.h>

#define IOADDR_CG_REG_SIZE 0x12A0

#define DEFAULT_OSC_IN_CLK_FREQ 12000000
#define DEFAULT_PLL_DIV_VALUE   131072
#define DEFAULT_PLL3_RATIO    5133653  /* 0x4E5555 (470MHz for DMA1) */
#define DEFAULT_PLL4_RATIO    2271914  /* 0x22AAAA (208MHz for SSPLL) */
#define DEFAULT_PLL5_RATIO    2621440  /* 0x280000 (240MHz for Sensor) */
#define DEFAULT_PLL6_RATIO    3276800  /* 0x320000 (300MHz for IDE/ETH) */
#define DEFAULT_PLL7_RATIO    3758096  /* 0x395810 (344.064MHz for Audio) */
#define DEFAULT_PLL8_RATIO    10485760 /* 0xA00000 (960MHz for CPU) */
#define DEFAULT_PLL9_RATIO    3059346  /* 0x2EAAAA (280MHz for IDE/ETH) */
#define DEFAULT_PLL10_RATIO   6553600  /* 0x640000 (600MHz for CNN) */
#define DEFAULT_PLL11_RATIO   5242880  /* 0x500000 (480MHz for DSI) */
#define DEFAULT_PLL12_RATIO   2621440  /* 0x280000 (240MHz for Sensor2) */
#define DEFAULT_PLL13_RATIO   3932160  /* 0x3C0000 (360MHz for IPP) */
#define DEFAULT_PLL14_RATIO   5133653  /* 0x215555 (470MHz for DMA2) */
#define DEFAULT_PLL15_RATIO   3932160  /* 0x3C0000 (360MHz for H264/H265) */
#define DEFAULT_PLLF320_RATIO 3495253  /* 0x355555 (320MHz) */

#define DEFAULT528_PLL3_RATIO   2547712  /* 0x26E000 (233.25MHz for DMA1) */
#define DEFAULT528_PLL8_RATIO   1365333  /* 0x14D555 (125MHz(x8) for CPU) */
#define DEFAULT528_PLL9_RATIO   3276800  /* 0x320000 (300MHz for IDE/ETH) */
#define DEFAULT528_PLL14_RATIO  2547712  /* 0x26E000 (233.25MHz for DMA2) */
#define DEFAULT528_PLL15_RATIO  3713706  /* 0x38AAAA (340MHz for H264/H265) */
#define DEFAULT528_PLL16_RATIO  2730666  /* 0x29AAAA (250MHz for ETH) */
#define DEFAULT528_PLL17_RATIO  5821781  /* 0x58D555 (533MHz for IPP2) */
#define DEFAULT528_PLL18_RATIO  4369066  /* 0x42AAAA (400MHz for Sensor3) */

static void __iomem *remap_base = NULL;
static spinlock_t cg_lock;

static int fastboot_determination(void)
{
	u32 m_fastboot = 0x0;
	struct device_node* of_node = of_find_node_by_path("/fastboot");

	if (of_node) {
		of_property_read_u32(of_node, "enable", &m_fastboot);
	}

	return m_fastboot;
}

static int logoboot_determination(void)
{
	u32 m_logo = 0x0;
	struct device_node* of_node = of_find_node_by_path("/logo");

	if (of_node) {
		of_property_read_u32(of_node, "enable", &m_logo);
	}

	return m_logo;
}

#define CPU_CLK_SEL_PLL8 1
#define CPU_CLK_SEL_480M 2

/**
 * Get frequency of peripheral clock
 */
unsigned long get_periph_clk(void)
{
	int cpu_clk_sel;
	u32 cpu_clk;

	cpu_clk_sel = readl(remap_base + CG_SYS_CLK_RATE_REG_OFFSET) & 0x3;

	switch (cpu_clk_sel) {
	case CPU_CLK_SEL_PLL8: {
#ifdef CONFIG_NVT_FPGA_EMULATION
		cpu_clk = 20000000;
#else
		struct clk *pll8_clk = clk_get(NULL, "pll8");

		cpu_clk = clk_get_rate(pll8_clk);

		if (nvt_get_chip_id() != CHIP_NA51055)
			cpu_clk *= 8;
#endif
		break;
	}
	case CPU_CLK_SEL_480M:
#ifdef CONFIG_NVT_FPGA_EMULATION
		cpu_clk = 48000000;
#else
		cpu_clk = 480000000;
#endif
		break;
	default:
#ifdef CONFIG_NVT_FPGA_EMULATION
		cpu_clk = 24000000;
#else
		cpu_clk = 80000000;
#endif
		break;
	}

	return (cpu_clk/8);
}

static void __init nvt_periph_clk_init(struct device_node *node)
{
	struct clk *periph_clk;

	periph_clk = clk_register_fixed_rate(NULL,
			"periph_clk",
			NULL,
			0,
			get_periph_clk());
	clk_register_clkdev(periph_clk, NULL, "periph_clk_dev");
	of_clk_add_provider(node, of_clk_src_simple_get, periph_clk);
}

CLK_OF_DECLARE(nvt_periph_clk, "nvt,periph_clk", nvt_periph_clk_init);

static struct nvt_fixed_rate_clk novatek_fixed_rate_clk[] __initdata = {
	FIXED_RATE_CONF("osc_in", 12000000),
#ifdef CONFIG_NVT_FPGA_EMULATION
	FIXED_RATE_CONF("fix20m", 20000000),
	FIXED_RATE_CONF("fix480m", 48000000),
	FIXED_RATE_CONF("fix240m", 24000000),
	FIXED_RATE_CONF("fix192m", 12000000),
#else
	FIXED_RATE_CONF("fix480m", 480000000),
	FIXED_RATE_CONF("fix240m", 240000000),
	FIXED_RATE_CONF("fix192m", 192000000),
#endif
	FIXED_RATE_CONF("fix160m", 160000000),
	FIXED_RATE_CONF("fix120m", 120000000),
	FIXED_RATE_CONF("fix96m", 96000000),
	FIXED_RATE_CONF("fix80m", 80000000),
	FIXED_RATE_CONF("fix60m", 60000000),
	FIXED_RATE_CONF("fix48m", 48000000),
	FIXED_RATE_CONF("fix24m", 24000000),
	FIXED_RATE_CONF("fix16M", 16000000),
	FIXED_RATE_CONF("fix12.288m", 12288000),
	FIXED_RATE_CONF("fix11.2896m", 11289600),
	FIXED_RATE_CONF("fix8.192m", 8192000),
	FIXED_RATE_CONF("fix3m", 3000000),
	FIXED_RATE_CONF("sie1_pxclkpad", 108000000),
	FIXED_RATE_CONF("sie2_pxclkpad", 108000000),
	FIXED_RATE_CONF("sie3_pxclkpad", 108000000),
	FIXED_RATE_CONF("vx1_1x", 48000000),
	FIXED_RATE_CONF("vx1_2x", 96000000),
	FIXED_RATE_CONF("PAUTOGATING", 0),
	FIXED_RATE_CONF("fix32.768k", 32768),
	FIXED_RATE_CONF("fix32k", 32000),
};

static struct nvt_pll_clk novatek_pll[] __initdata = {
	PLL_CONF("pll3", DEFAULT_PLL3_RATIO, CG_PLL3_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT3),
	PLL_CONF("pll4", DEFAULT_PLL4_RATIO, CG_PLL4_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT4),
	PLL_CONF("pll5", DEFAULT_PLL5_RATIO, CG_PLL5_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT5),
	PLL_CONF("pll6", DEFAULT_PLL6_RATIO, CG_PLL6_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT6),
	PLL_CONF("pll7", DEFAULT_PLL7_RATIO, CG_PLL7_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT7),
	PLL_CONF("pll8", DEFAULT_PLL8_RATIO, CG_PLL8_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT8),
	PLL_CONF("pll9", DEFAULT_PLL9_RATIO, CG_PLL9_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT9),
	PLL_CONF("pll10", DEFAULT_PLL10_RATIO, CG_PLL10_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT10),
	PLL_CONF("pll11", DEFAULT_PLL11_RATIO, CG_PLL11_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT11),
	PLL_CONF("pll12", DEFAULT_PLL12_RATIO, CG_PLL12_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT12),
	PLL_CONF("pll13", DEFAULT_PLL13_RATIO, CG_PLL13_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT13),
	PLL_CONF("pll14", DEFAULT_PLL14_RATIO, CG_PLL14_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT14),
	PLL_CONF("pll15", DEFAULT_PLL15_RATIO, CG_PLL15_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT15),
	PLL_CONF("pllf320", DEFAULT_PLLF320_RATIO, CG_PLLF320_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT24),
};

static struct nvt_pll_clk novatek_pll528[] __initdata = {
	PLL_CONF("pll3", DEFAULT528_PLL3_RATIO, CG528_PLL3_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT3),
	PLL_CONF("pll4", DEFAULT_PLL4_RATIO, CG528_PLL4_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT4),
	PLL_CONF("pll5", DEFAULT_PLL5_RATIO, CG528_PLL5_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT5),
	PLL_CONF("pll6", DEFAULT_PLL6_RATIO, CG528_PLL6_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT6),
	PLL_CONF("pll7", DEFAULT_PLL7_RATIO, CG528_PLL7_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT7),
	PLL_CONF("pll8", DEFAULT528_PLL8_RATIO, CG528_PLL8_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT8),
	PLL_CONF("pll9", DEFAULT528_PLL9_RATIO, CG528_PLL9_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT9),
	PLL_CONF("pll10", DEFAULT_PLL10_RATIO, CG528_PLL10_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT10),
	PLL_CONF("pll11", DEFAULT_PLL11_RATIO, CG528_PLL11_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT11),
	PLL_CONF("pll12", DEFAULT_PLL12_RATIO, CG528_PLL12_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT12),
	PLL_CONF("pll13", DEFAULT_PLL13_RATIO, CG528_PLL13_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT13),
	PLL_CONF("pll14", DEFAULT528_PLL14_RATIO, CG528_PLL14_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT14),
	PLL_CONF("pll15", DEFAULT528_PLL15_RATIO, CG528_PLL15_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT15),
	PLL_CONF("pll16", DEFAULT528_PLL16_RATIO, CG528_PLL16_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT16),
	PLL_CONF("pll17", DEFAULT528_PLL17_RATIO, CG528_PLL17_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT17),
	PLL_CONF("pll18", DEFAULT528_PLL18_RATIO, CG528_PLL18_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT18),
	PLL_CONF("pllf320", DEFAULT_PLLF320_RATIO, CG528_PLLF320_CLK_RATE0_REG_OFFSET,
		 CG_PLL_EN_REG_OFFSET, BIT24),
};

static struct nvt_composite_gate_clk novatek_cgate_clk[] __initdata = {
	COMP_GATE_CONF("f0220000.i2c", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG1_OFFSET, BIT4, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT4, DO_RESET,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT22, DO_AUTOGATING),
	COMP_GATE_CONF("f0350000.i2c2", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG1_OFFSET, BIT5, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT5, DO_RESET,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT23, DO_AUTOGATING),
	COMP_GATE_CONF("f03a0000.i2c3", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG1_OFFSET, BIT31, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT31, DO_RESET,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT24, DO_AUTOGATING),
	COMP_GATE_CONF("f03b0000.i2c4", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG1_OFFSET, BIT26, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT20, DO_RESET,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT22, DO_AUTOGATING),
	COMP_GATE_CONF("f03c0000.i2c5", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG1_OFFSET, BIT27, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT21, DO_RESET,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT23, DO_AUTOGATING),
	COMP_GATE_CONF("f0050000.wdt", "osc_in", 12000000, 0, 0, 0,
		       CG_CLK_EN_REG1_OFFSET, BIT17, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT17, NOT_RESET,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT1, DO_AUTOGATING),
	COMP_GATE_CONF("f0290000.uart", "fix24m", 24000000, 0, 0, 0,
		       CG_CLK_EN_REG1_OFFSET, BIT10, DO_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT10, NOT_RESET,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT28, DO_AUTOGATING),
	COMP_GATE_CONF("f0300000.uart", "fix480m", 48000000,
		       CG_UART_CLK_DIV_REG0_OFFSET, BIT0, WID8,
		       CG_CLK_EN_REG1_OFFSET, BIT11, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT11, NOT_RESET,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT29, DO_AUTOGATING),
	COMP_GATE_CONF("f0310000.uart", "fix480m", 48000000,
		       CG_UART_CLK_DIV_REG0_OFFSET, BIT8, WID8,
		       CG_CLK_EN_REG1_OFFSET, BIT22, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT22, NOT_RESET,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT16, DO_AUTOGATING),
	COMP_GATE_CONF("f0380000.uart", "fix480m", 48000000,
		       CG_UART_CLK_DIV_REG0_OFFSET, BIT16, WID8,
		       CG_CLK_EN_REG1_OFFSET, BIT23, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT4, NOT_RESET,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT17, DO_AUTOGATING),
	COMP_GATE_CONF("f03e0000.uart", "fix480m", 48000000,
		       CG_UART_CLK_DIV_REG0_OFFSET, BIT24, WID8,
		       CG_CLK_EN_REG1_OFFSET, BIT24, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT5, NOT_RESET,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT18, DO_AUTOGATING),
	COMP_GATE_CONF("f03f0000.uart", "fix480m", 48000000,
		       CG_PERI_CLK_DIV_REG2_OFFSET, BIT16, WID8,
		       CG_CLK_EN_REG1_OFFSET, BIT25, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT6, NOT_RESET,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT19, DO_AUTOGATING),
	COMP_GATE_CONF("f0260000.adc", "fix16M", 16000000, 0, 0, 0,
		       CG_CLK_EN_REG1_OFFSET, BIT13, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT13, NOT_RESET,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT31, DO_AUTOGATING),
	COMP_GATE_CONF("f0230000.spi", "fix192m", 24000000,
		       CG_SPI_CLK_DIV_REG0_OFFSET, BIT0, WID11,
		       CG_CLK_EN_REG1_OFFSET, BIT6, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT6, NOT_RESET,
		       CG_CLK_AUTO_GATING_REG0_OFFSET, BIT24, DO_AUTOGATING),
	COMP_GATE_CONF("f0320000.spi", "fix192m", 24000000,
		       CG_SPI_CLK_DIV_REG0_OFFSET, BIT16, WID11,
		       CG_CLK_EN_REG1_OFFSET, BIT7, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT7, NOT_RESET,
		       CG_CLK_AUTO_GATING_REG0_OFFSET, BIT25, DO_AUTOGATING),
	COMP_GATE_CONF("f0340000.spi", "fix192m", 24000000,
		       CG_SPI_CLK_DIV_REG1_OFFSET, BIT0, WID11,
		       CG_CLK_EN_REG1_OFFSET, BIT8, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT8, NOT_RESET,
		       CG_CLK_AUTO_GATING_REG0_OFFSET, BIT26, DO_AUTOGATING),
    COMP_GATE_CONF("f0360000.spi", "fix192m", 24000000,
		       CG_SPI_CLK_DIV_REG1_OFFSET, BIT16, WID11,
		       CG_CLK_EN_REG2_OFFSET, BIT11, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT11, NOT_RESET,
		       CG_CLK_AUTO_GATING_REG0_OFFSET, BIT22, DO_AUTOGATING),
	COMP_GATE_CONF("f03d0000.spi", "fix192m", 24000000,
		       CG_SPI_CLK_DIV_REG2_OFFSET, BIT0, WID11,
		       CG_CLK_EN_REG2_OFFSET, BIT12, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT12, NOT_RESET,
		       CG_CLK_AUTO_GATING_REG0_OFFSET, BIT23, DO_AUTOGATING),
	COMP_GATE_CONF("f0390000.sdp", "fix96m", 96000000, 0, 0, 0,
		       CG_CLK_EN_REG3_OFFSET, BIT16, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT20, NOT_RESET,
		       0, 0, 0),
#ifdef CONFIG_NVT_FPGA_EMULATION
#ifdef CONFIG_MTD_SPINAND
	COMP_GATE_CONF("f0400000.nand", "fix480m", 6000000,
		       CG_PERI_CLK_DIV_REG1_OFFSET, BIT12, WID6,
		       CG_CLK_EN_REG1_OFFSET, BIT0, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT0, NOT_RESET,
		       CG_CLK_AUTO_GATING_REG0_OFFSET, BIT19, NOT_AUTOGATING),
#else
	COMP_GATE_CONF("f0400000.nor", "fix480m", 6000000,
		       CG_PERI_CLK_DIV_REG1_OFFSET, BIT12, WID6,
		       CG_CLK_EN_REG1_OFFSET, BIT0, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT0, NOT_RESET,
		       CG_CLK_AUTO_GATING_REG0_OFFSET, BIT19, NOT_AUTOGATING),
#endif
#else
#ifdef CONFIG_MTD_SPINAND
	COMP_GATE_CONF("f0400000.nand", "fix480m", 48000000,
		       CG_PERI_CLK_DIV_REG1_OFFSET, BIT12, WID6,
		       CG_CLK_EN_REG1_OFFSET, BIT0, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT0, NOT_RESET,
		       CG_CLK_AUTO_GATING_REG0_OFFSET, BIT19, NOT_AUTOGATING),
#else
	COMP_GATE_CONF("f0400000.nor", "fix480m", 48000000,
		       CG_PERI_CLK_DIV_REG1_OFFSET, BIT12, WID6,
		       CG_CLK_EN_REG1_OFFSET, BIT0, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT0, NOT_RESET,
		       CG_CLK_AUTO_GATING_REG0_OFFSET, BIT19, NOT_AUTOGATING),
#endif
#endif
	COMP_GATE_CONF("f0240000.sif", "fix96m", 96000000, 0, 0, 0,
		       CG_CLK_EN_REG1_OFFSET, BIT9, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT9, NOT_RESET,
		       CG_CLK_AUTO_GATING_REG0_OFFSET, BIT27, NOT_AUTOGATING),
	COMP_GATE_CONF("pwm_clk.8", "fix120m", 120000000,
		       CG_PWM_CLK_RATE_REG1_OFFSET, BIT0, WID14,
		       CG_CLK_EN_REG3_OFFSET, BIT8, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT8, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("pwm_clk.9", "fix120m", 120000000,
		       CG_PWM_CLK_RATE_REG1_OFFSET, BIT16, WID14,
		       CG_CLK_EN_REG3_OFFSET, BIT9, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT8, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("pwm_clk.10", "fix120m", 120000000,
		       CG_PWM_CLK_RATE_REG2_OFFSET, BIT0, WID14,
		       CG_CLK_EN_REG3_OFFSET, BIT10, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT8, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("pwm_clk.11", "fix120m", 120000000,
		       CG_PWM_CLK_RATE_REG2_OFFSET, BIT16, WID14,
		       CG_CLK_EN_REG3_OFFSET, BIT11, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT8, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("ccnt_clk.0", "fix3m", 3000000,
		       0, 0, 0,
		       CG_CLK_EN_REG2_OFFSET, BIT8, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT8, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("ccnt_clk.1", "fix3m", 3000000,
		       0, 0, 0,
		       CG_CLK_EN_REG2_OFFSET, BIT9, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT8, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("ccnt_clk.2", "fix3m", 3000000,
		       0, 0, 0,
		       CG_CLK_EN_REG2_OFFSET, BIT10, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT8, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("f02a0000.vx1", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG2_OFFSET, BIT14, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT21, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("f02a0000.vx1ml", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG2_OFFSET, BIT23, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT14, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("f02a0000.vx1sbl", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG2_OFFSET, BIT20, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT20, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("f03f0000.vx1ml", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG2_OFFSET, BIT24, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT24, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("f03f0000.vx1sbl", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG2_OFFSET, BIT22, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT22, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("f0270000.lvds", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG2_OFFSET, BIT1, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT1, NOT_RESET,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT3, DO_AUTOGATING),
	COMP_GATE_CONF("f0370000.lvds", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG2_OFFSET, BIT2, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT2, NOT_RESET,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT4, DO_AUTOGATING),
	COMP_GATE_CONF("f0600000.usb20", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG1_OFFSET, BIT19, NOT_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT19, DO_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("f0040000.timer", "fix3m", 3000000,
		       0, 0, 0,
		       CG_CLK_EN_REG1_OFFSET, BIT18, DO_ENABLE,
		       CG_SYS_RESET_REG1_OFFSET, BIT18, NOT_RESET,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT0, DO_AUTOGATING),
	COMP_GATE_CONF("f0820000.hdmitx", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG0_OFFSET, BIT22, NOT_ENABLE,
		       CG_SYS_RESET_REG0_OFFSET, BIT22, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("f0820000.cec", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG0_OFFSET, BIT21, NOT_ENABLE,
		       CG_SYS_RESET_REG0_OFFSET, BIT21, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("f0830000.tv", "fix48m", 48000000, 0, 0, 0,
		       CG_CLK_EN_REG0_OFFSET, BIT18, NOT_ENABLE,
		       CG_SYS_RESET_REG0_OFFSET, BIT18, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("f00b0000.drtc", "osc_in", 12000000, 0, 0, 0,
		       CG_CLK_EN_REG2_OFFSET, BIT22, DO_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT22, NOT_RESET,
		       0, 0, 0),
	COMP_GATE_CONF("pll4_ss", "osc_in", 0,
		       CG_PLL4_SPREAD_SPECTRUM_REG0_OFFSET, BIT4, 0,
		       CG_PLL4_SPREAD_SPECTRUM_REG1_OFFSET, BIT4, 0,
		       CG_PLL4_SPREAD_SPECTRUM_REG2_OFFSET, BIT4, 0,
		       CG_PLL4_SPREAD_SPECTRUM_REG3_OFFSET, BIT4, 0),
	COMP_GATE_CONF("pll4_ss_528", "osc_in", 0,
		       CG528_PLL4_SPREAD_SPECTRUM_REG0_OFFSET, BIT4, 0,
		       CG528_PLL4_SPREAD_SPECTRUM_REG1_OFFSET, BIT4, 0,
		       CG528_PLL4_SPREAD_SPECTRUM_REG2_OFFSET, BIT4, 0,
		       CG528_PLL4_SPREAD_SPECTRUM_REG3_OFFSET, BIT4, 0),
	COMP_GATE_CONF("ipe_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT2, DO_AUTOGATING),
	COMP_GATE_CONF("ime_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT3, DO_AUTOGATING),
	COMP_GATE_CONF("dis_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT4, DO_AUTOGATING),
	COMP_GATE_CONF("cnn2_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT5, DO_AUTOGATING),
	COMP_GATE_CONF("dce_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT6, DO_AUTOGATING),
	COMP_GATE_CONF("ife_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT7, DO_AUTOGATING),
	COMP_GATE_CONF("graphic_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT8, DO_AUTOGATING),
	COMP_GATE_CONF("graphic2_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT9, DO_AUTOGATING),
	COMP_GATE_CONF("ide_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT10, DO_AUTOGATING),
	COMP_GATE_CONF("nue_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT11, DO_AUTOGATING),
	COMP_GATE_CONF("nue2_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT12, DO_AUTOGATING),
	COMP_GATE_CONF("mdbc_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT13, DO_AUTOGATING),
	COMP_GATE_CONF("ife2_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT14, DO_AUTOGATING),
	COMP_GATE_CONF("jpeg_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT15, DO_AUTOGATING),
	COMP_GATE_CONF("h265_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT16, DO_AUTOGATING),
	COMP_GATE_CONF("dai_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT17, DO_AUTOGATING),
	COMP_GATE_CONF("eac_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT18, DO_AUTOGATING),
	COMP_GATE_CONF("nand_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT19, DO_AUTOGATING),
	COMP_GATE_CONF("sdio_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT20, DO_AUTOGATING),
	COMP_GATE_CONF("sdio2_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT21, DO_AUTOGATING),
	COMP_GATE_CONF("spi_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT24, DO_AUTOGATING),
	COMP_GATE_CONF("spi2_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT25, DO_AUTOGATING),
	COMP_GATE_CONF("spi3_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT26, DO_AUTOGATING),
	COMP_GATE_CONF("spi4_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT30, DO_AUTOGATING),
	COMP_GATE_CONF("spi5_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT31, DO_AUTOGATING),
	COMP_GATE_CONF("sif_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT27, DO_AUTOGATING),
	COMP_GATE_CONF("uart_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT28, DO_AUTOGATING),
	COMP_GATE_CONF("remote_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT30, DO_AUTOGATING),
	COMP_GATE_CONF("mipidsi_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT6, DO_AUTOGATING),
	COMP_GATE_CONF("ise_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT8, DO_AUTOGATING),
	COMP_GATE_CONF("pwm_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT11, DO_AUTOGATING),
	COMP_GATE_CONF("sdio3_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT15, DO_AUTOGATING),
	COMP_GATE_CONF("tge_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT20, DO_AUTOGATING),
	COMP_GATE_CONF("sdp_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT21, NOT_AUTOGATING),
	COMP_GATE_CONF("tse_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT25, DO_AUTOGATING),
	COMP_GATE_CONF("affine_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT26, DO_AUTOGATING),
	COMP_GATE_CONF("ive_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT28, DO_AUTOGATING),
	COMP_GATE_CONF("senphy_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT29, DO_AUTOGATING),
	COMP_GATE_CONF("gpio_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG2_OFFSET, BIT0, DO_AUTOGATING),
	COMP_GATE_CONF("intc_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG2_OFFSET, BIT1, DO_AUTOGATING),
	COMP_GATE_CONF("dma_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG2_OFFSET, BIT3, DO_AUTOGATING),
	COMP_GATE_CONF("dma2_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG2_OFFSET, BIT4, DO_AUTOGATING),
	COMP_GATE_CONF("vpe_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG2_OFFSET, BIT31, NOT_AUTOGATING),
	COMP_GATE_CONF("sde_pclk", "PAUTOGATING", 0,
		       0, 0, 0,
		       0, 0, 0,
		       0, 0, 0,
		       CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT2, DO_AUTOGATING),
};

static struct nvt_composite_group_pwm_clk novatek_cgpwm_clk[] __initdata = {
	COMP_GPWM_CONF("fix120m", 120000000,
		       CG_PWM_CLK_RATE_REG0_OFFSET, BIT0, WID14,
		       CG_CLK_EN_REG3_OFFSET, BIT0, BIT3, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT8),
	COMP_GPWM_CONF("fix120m", 120000000,
		       CG_PWM_CLK_RATE_REG0_OFFSET, BIT16, WID14,
		       CG_CLK_EN_REG3_OFFSET, BIT4, BIT7, NOT_ENABLE,
		       CG_SYS_RESET_REG2_OFFSET, BIT8),
};

static const char *sdio_clk_parent[] __initconst = {
	"fix192m", "fix480m", "reserved", "pll4"
};

static const char *apb_clk_parent[] __initconst = {
	"fix48m", "fix60m", "fix80m", "fix120m"
};

static const char *tse_clk_parent[] __initconst = {
	"fix240m", "pllf320", "reserved", "pll9"
};

static const char *crypto_clk_parent[] __initconst = {
	"fix240m", "pllf320", "reserved", "pll9"
};

static const char *rsa_clk_parent[] __initconst = {
	"fix240m", "pllf320", "reserved", "pll9"
};

static const char *hash_clk_parent[] __initconst = {
	"fix240m", "pllf320", "reserved", "pll9"
};

static const char *sie_clk_parent[] __initconst = {
	"fix480m", "reserved", "pll5", "pll13", "pll12", "pllf320", "fix192m", "pll10"
};

static const char *sie2_clk_parent[] __initconst = {
	"fix480m", "reserved", "pll5", "pll13", "pll12", "pllf320", "fix192m", "pll10"
};

static const char *sie3_clk_parent[] __initconst = {
	"fix480m", "reserved", "pll5", "pll13", "pll12", "pllf320", "fix192m", "pll10"
};

static const char *sie4_clk_parent[] __initconst = {
	"fix480m", "reserved", "pll5", "pll13", "pll12", "pllf320", "fix192m", "pll10"
};

static const char *sie5_clk_parent[] __initconst = {
	"fix480m", "reserved", "pll5", "pll13", "pll12", "pllf320", "fix192m", "pll10"
};

static const char *ime_clk_parent[] __initconst = {
	"fix240m", "pllf320", "reserved", "pll13"
};

static const char *ife2_clk_parent[] __initconst = {
	"fix240m", "pllf320", "reserved", "pll13"
};

static const char *ise_clk_parent[] __initconst = {
	"fix240m", "pllf320", "fix480m", "pll13"
};

static const char *sde_clk_parent[] __initconst = {
	"fix240m", "pllf320", "fix480m", "pll13"
};

static const char *siemclk_clk_parent[] __initconst = {
	"fix480m", "pll5", "pll10", "pll12"
};

static const char *siemclk2_clk_parent[] __initconst = {
	"fix480m", "pll5", "pll10", "pll12"
};

static const char *siemclk3_clk_parent[] __initconst = {
	"fix480m", "pll5", "pll18", "pll12"
};

static const char *ife_clk_parent[] __initconst = {
	"fix240m", "pllf320", "reserved", "pll13"
};

static const char *cnn_clk_parent[] __initconst = {
	"fix240m", "pllf320", "fix480m", "pll10"
};

static const char *cnn2_clk_parent[] __initconst = {
	"fix240m", "pllf320", "fix480m", "pll10"
};

static const char *nue2_clk_parent[] __initconst = {
	"fix240m", "pllf320", "fix480m", "pll13"
};

static const char *nue_clk_parent[] __initconst = {
	"fix240m", "pllf320", "fix480m", "pll10"
};

static const char *md_clk_parent[] __initconst = {
	"fix240m", "pllf320", "reserved", "pll13"
};

static const char *dis_clk_parent[] __initconst = {
	"fix240m", "pllf320", "fix480m", "pll13"
};

static const char *ipe_clk_parent[] __initconst = {
	"fix240m", "pllf320", "reserved", "pll13"
};

static const char *dce_clk_parent[] __initconst = {
	"fix240m", "pllf320", "reserved", "pll13"
};

static const char *ive_clk_parent[] __initconst = {
	"fix240m", "pllf320", "fix480m", "pll13"
};

#ifdef CONFIG_NVT_FPGA_EMULATION
static const char *ide_clk_parent[] __initconst = {
	"fix480m", "fix24m", "pll4", "pll9"
};
#else
static const char *ide_clk_parent[] __initconst = {
	"fix480m", "pll6", "pll4", "pll9"
};
#endif

static const char *mi_clk_parent[] __initconst = {
	"fix192m", "fix240m"
};

static const char *csi_clk_parent[] __initconst = {
	"fix60m", "fix120m"
};

static const char *grph_clk_parent[] __initconst = {
	"fix240m", "pllf320", "fix480m", "pll13"
};

static const char *affine_clk_parent[] __initconst = {
	"fix240m", "pllf320", "fix480m", "pll13"
};

static const char *jpg_clk_parent[] __initconst = {
	"fix240m", "pllf320", "fix480m", "pll9"
};

static const char *h26x_clk_parent[] __initconst = {
    "fix240m", "pllf320", "pll15", "pll13"
};

static const char *eacdac_clk_parent[] __initconst = {
	"pll7"
};

static const char *dai_clk_parent[] __initconst = {
	"f0640000.eacdac"
};

static const char *sie_io_pxclk_clk_parent[] __initconst = {
	"sie1_pxclkpad", "sie2_pxclkpad"
};

static const char *sie2_io_pxclk_clk_parent[] __initconst = {
	"sie2_pxclkpad", "sie1_pxclkpad"
};

static const char *sie4_io_pxclk_clk_parent[] __initconst = {
	"sie1_pxclkpad", "sie2_pxclkpad"
};

static const char *sie1_pxclk_clk_parent[] __initconst = {
	"sie_io_pxclk", "f0c00000.siemck"
};

static const char *sie2_pxclk_clk_parent[] __initconst = {
	"sie2_io_pxclk", "f0c00000.siemck"
};

static const char *sie3_pxclk_clk_parent[] __initconst = {
	"sie2_io_pxclk", "f0c00000.siemck"
};

static const char *sie4_pxclk_clk_parent[] __initconst = {
	"sie4_io_pxclk", "f0c00000.siemck"
};

static const char *tge_clk_parent[] __initconst = {
	"sie_io_pxclk", "f0c00000.siemck"
};

static const char *tge2_clk_parent[] __initconst = {
	"sie2_io_pxclk", "f0c00000.siemk2"
};

static const char *special_clk_parent[] __initconst = {
	"fix480m", "pll4", "pll10", "pll14"
};

static const char *remote_clk_parent[] __initconst = {
	"fix32.768k", "fix32k",
};

static const char *rng_clk_parent[] __initconst = {
	"fix160m", "fix240m"
};

static const char *rngro_clk_parent[] __initconst = {
	"osc_in", "pll4"
};

static const char *vpe_clk_parent[] __initconst = {
	"fix240m", "pllf320", "pll17", "pll13"
};

static const char *eth_root_parent[] __initconst = {
        "pll6", "pll16"
};

static struct nvt_composite_mux_clk novatek_cmux_clk[] __initdata = {
	COMP_MUX_CONF("f0420000.mmc", sdio_clk_parent, 312500, 0,
		      CG_PERI_CLK_RATE_REG0_OFFSET, BIT4, WID2,
		      CG_SDIO_CLK_DIV_REG_OFFSET, BIT0, WID11,
		      CG_CLK_EN_REG1_OFFSET, BIT2, NOT_ENABLE,
		      CG_SYS_RESET_REG1_OFFSET, BIT2, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT20, DO_AUTOGATING),
	COMP_MUX_CONF("f0500000.mmc", sdio_clk_parent, 312500, 0,
		      CG_PERI_CLK_RATE_REG0_OFFSET, BIT8, WID2,
		      CG_SDIO_CLK_DIV_REG_OFFSET, BIT16, WID11,
		      CG_CLK_EN_REG1_OFFSET, BIT3, NOT_ENABLE,
		      CG_SYS_RESET_REG1_OFFSET, BIT3, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT21, NOT_AUTOGATING),
	COMP_MUX_CONF("f0510000.mmc", sdio_clk_parent, 312500, 0,
		      CG_PERI_CLK_RATE_REG1_OFFSET, BIT0, WID2,
		      CG_PERI_CLK_DIV_REG1_OFFSET, BIT0, WID11,
		      CG_CLK_EN_REG1_OFFSET, BIT14, NOT_ENABLE,
		      CG_SYS_RESET_REG1_OFFSET, BIT14, DO_RESET,
		      CG_CLK_AUTO_GATING_REG1_OFFSET, BIT15, NOT_AUTOGATING),
	COMP_MUX_CONF("f0620000.crypto", crypto_clk_parent, 320000000, 1,
		      CG_CODEC_CLK_RATE_REG_OFFSET, BIT20, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT23, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT23, NOT_RESET,
		      0, 0, 0),
	COMP_MUX_CONF("f06a0000.rsa", rsa_clk_parent, 320000000, 1,
		      CG_CODEC_CLK_RATE_REG_OFFSET, BIT22, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG2_OFFSET, BIT26, NOT_ENABLE,
		      CG_SYS_RESET_REG2_OFFSET, BIT26, NOT_RESET,
		      0, 0, 0),
	COMP_MUX_CONF("f0700000.hash", hash_clk_parent, 320000000, 1,
		      CG_PERI_CLK_RATE_REG1_OFFSET, BIT16, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG2_OFFSET, BIT27, NOT_ENABLE,
		      CG_SYS_RESET_REG2_OFFSET, BIT27, NOT_RESET,
		      0, 0, 0),
	COMP_MUX_CONF("f0650000.tse", tse_clk_parent, 240000000, 0,
		      CG_PERI_CLK_RATE_REG1_OFFSET, BIT4, WID2, 0, 0, 0,
		      CG_CLK_EN_REG2_OFFSET, BIT13, NOT_ENABLE,
		      CG_SYS_RESET_REG2_OFFSET, BIT13, DO_RESET,
		      CG_CLK_AUTO_GATING_REG1_OFFSET, BIT25, DO_AUTOGATING),
	COMP_MUX_CONF("f0c00000.sie", sie_clk_parent, 240000000, 0,
		      CG_CODEC_CLK_RATE_REG_OFFSET, BIT24, WID3,
		      CG_IPP_CLK_DIV_REG0_OFFSET, BIT16, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT4, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT4, DO_RESET,
		      0, 0, 0),
	COMP_MUX_CONF("f0d20000.sie", sie2_clk_parent, 240000000, 0,
		      CG_CODEC_CLK_RATE_REG_OFFSET, BIT28, WID3,
		      CG_IPP_CLK_DIV_REG0_OFFSET, BIT24, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT5, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT5, DO_RESET,
		      0, 0, 0),
	COMP_MUX_CONF("f0d30000.sie", sie3_clk_parent, 240000000, 0,
		      CG_CODEC_CLK_RATE_REG_OFFSET, BIT16, WID3,
		      CG_PERI_CLK_DIV_REG0_OFFSET, BIT8, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT17, NOT_ENABLE,
		      CG_SYS_RESET_REG2_OFFSET, BIT9, DO_RESET,
		      0, 0, 0),
	COMP_MUX_CONF("f0d40000.sie", sie4_clk_parent, 240000000, 0,
		      CG_PERI_CLK_RATE_REG3_OFFSET, BIT0, WID3,
		      CG_PERI_CLK_RATE_REG3_OFFSET, BIT16, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT18, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT18, DO_RESET,
		      0, 0, 0),
	COMP_MUX_CONF("f0d80000.sie", sie5_clk_parent, 240000000, 0,
		      CG_PERI_CLK_RATE_REG3_OFFSET, BIT4, WID3,
		      CG_PERI_CLK_RATE_REG3_OFFSET, BIT24, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT19, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT19, DO_RESET,
		      0, 0, 0),
	COMP_MUX_CONF("f0c40000.ime", ime_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG0_OFFSET, BIT12, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT9, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT9, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT3, DO_AUTOGATING),
	COMP_MUX_CONF("f0d00000.ife2", ife2_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT4, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT15, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT15, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT14, DO_AUTOGATING),
	COMP_MUX_CONF("f0c90000.ise", ise_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG0_OFFSET, BIT24, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT11, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT11, DO_RESET,
		      CG_CLK_AUTO_GATING_REG1_OFFSET, BIT8, DO_AUTOGATING),
	COMP_MUX_CONF("f0d90000.sde", sde_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG0_OFFSET, BIT18, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT20, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT20, DO_RESET,
		      CG_CLK_AUTO_GATING_REG1_OFFSET, BIT2, DO_AUTOGATING),		  		  
	COMP_MUX_CONF("f0c00000.siemck", siemclk_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT8, WID2,
		      CG_IPP_CLK_DIV_REG0_OFFSET, BIT0, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT2, NOT_ENABLE,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("f0c00000.siemk2", siemclk2_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT10, WID2,
		      CG_IPP_CLK_DIV_REG0_OFFSET, BIT8, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT3, NOT_ENABLE,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("f0c00000.siemk3", siemclk3_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT12, WID2,
		      CG_VIDEO_CLK_DIV_REG_OFFSET, BIT16, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT10, NOT_ENABLE,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("f0c70000.ife", ife_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT0, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT13, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT13, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT7, DO_AUTOGATING),
	COMP_MUX_CONF("f0cb0000.ai", cnn_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG0_OFFSET, BIT0, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG3_OFFSET, BIT23, NOT_ENABLE,
		      CG_SYS_RESET_REG2_OFFSET, BIT17, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT1, DO_AUTOGATING),
	COMP_MUX_CONF("f0c60000.ai", nue_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT28, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG3_OFFSET, BIT20, NOT_ENABLE,
		      CG_SYS_RESET_REG2_OFFSET, BIT14, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT11, DO_AUTOGATING),
	COMP_MUX_CONF("f0d50000.ai", nue2_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT30, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG3_OFFSET, BIT21, NOT_ENABLE,
		      CG_SYS_RESET_REG2_OFFSET, BIT15, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT12, DO_AUTOGATING),
	COMP_MUX_CONF("f0d60000.ai", cnn2_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG0_OFFSET, BIT16, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG3_OFFSET, BIT24, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT10, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT5, NOT_AUTOGATING),
	COMP_MUX_CONF("f0c10000.md", md_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG0_OFFSET, BIT20, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG3_OFFSET, BIT22, NOT_ENABLE,
		      CG_SYS_RESET_REG2_OFFSET, BIT16, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT13, DO_AUTOGATING),
	COMP_MUX_CONF("f0c50000.dis", dis_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG0_OFFSET, BIT8, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT8, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT8, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT4, DO_AUTOGATING),
	COMP_MUX_CONF("f0c20000.dce", dce_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG0_OFFSET, BIT28, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT14, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT14, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT6, DO_AUTOGATING),
	COMP_MUX_CONF("f0d70000.ive", ive_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG0_OFFSET, BIT2, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG3_OFFSET, BIT25, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT12, DO_RESET,
		      CG_CLK_AUTO_GATING_REG1_OFFSET, BIT28, DO_AUTOGATING),
	COMP_MUX_CONF("f0c30000.ipe", ipe_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG0_OFFSET, BIT4, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT7, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT7, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT2, DO_AUTOGATING),
	COMP_MUX_CONF("f0800000.ide", ide_clk_parent, 297000000, 1,
		      CG_PERI_CLK_RATE_REG0_OFFSET, BIT16, WID2,
		      CG_VIDEO_CLK_DIV_REG_OFFSET, BIT0, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT16, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT16, NOT_RESET,
		      CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT10, DO_AUTOGATING),
	COMP_MUX_CONF("f0800000.ideif", ide_clk_parent, 297000000, 1,
		      CG_PERI_CLK_RATE_REG0_OFFSET, BIT16, WID2,
		      CG_VIDEO_CLK_DIV_REG_OFFSET, BIT8, WID8,
		      0, 0, 0,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("f0840000.dsi", ide_clk_parent, 60000000, 0,
		      CG_PERI_CLK_RATE_REG0_OFFSET, BIT20, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG2_OFFSET, BIT3, NOT_ENABLE,
		      CG_SYS_RESET_REG2_OFFSET, BIT3, DO_RESET,
		      CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT6, DO_AUTOGATING),
	COMP_MUX_CONF("f0280000.csi", csi_clk_parent, 60000000, 0,
		      CG_PERI_CLK_RATE_REG0_OFFSET, BIT12, WID1,
		      0, 0, 0,
		      CG_CLK_EN_REG2_OFFSET, BIT1, NOT_ENABLE,
		      CG_SYS_RESET_REG2_OFFSET, BIT1, NOT_RESET,
		      CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT3, DO_AUTOGATING),
	COMP_MUX_CONF("f0330000.csi", csi_clk_parent, 60000000, 0,
		      CG_PERI_CLK_RATE_REG0_OFFSET, BIT15, WID1,
		      0, 0, 0,
		      CG_CLK_EN_REG2_OFFSET, BIT2, NOT_ENABLE,
		      CG_SYS_RESET_REG2_OFFSET, BIT2, NOT_RESET,
		      CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT4, DO_AUTOGATING),
	COMP_MUX_CONF("f0c80000.grph", grph_clk_parent, 240000000, 0,
		      CG_CODEC_CLK_RATE_REG_OFFSET, BIT8, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT27, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT27, DO_RESET,
		      CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT8, DO_AUTOGATING),
	COMP_MUX_CONF("f0d10000.grph", grph_clk_parent, 240000000, 0,
		      CG_CODEC_CLK_RATE_REG_OFFSET, BIT12, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT28, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT28, DO_RESET,
		      CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT9, DO_AUTOGATING),
	COMP_MUX_CONF("f0ca0000.affine", affine_clk_parent, 240000000, 0,
			  CG_CODEC_CLK_RATE_REG_OFFSET, BIT10, WID2,
			  0, 0, 0,
			  CG_CLK_EN_REG0_OFFSET, BIT25, NOT_ENABLE,
			  CG_SYS_RESET_REG0_OFFSET, BIT25, DO_RESET,
			  CG_APB_CLK_AUTO_GATING_REG1_OFFSET, BIT26, DO_AUTOGATING),
	COMP_MUX_CONF("f0a00000.jpg", jpg_clk_parent, 320000000, 0,
		      CG_CODEC_CLK_RATE_REG_OFFSET, BIT0, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT26, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT26, NOT_RESET,
		      CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT15, DO_AUTOGATING),
	COMP_MUX_CONF("f0a10000.h26x", h26x_clk_parent, 320000000, 0,
		      CG_CODEC_CLK_RATE_REG_OFFSET, BIT4, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT24, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT24, NOT_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT16, DO_AUTOGATING),
	COMP_MUX_CONF("f0640000.eacdac", eacdac_clk_parent, 12288000, 0,
		      0, 0, 0,
		      CG_PERI_CLK_DIV_REG0_OFFSET, BIT24, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT31, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT30, NOT_RESET,
		      CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT18, NOT_AUTOGATING),
	COMP_MUX_CONF("f0630000.dai", dai_clk_parent, 12288000, 0,
		      CG_PERI_CLK_RATE_REG0_OFFSET, BIT28, WID2,
		      CG_PERI_CLK_DIV_REG0_OFFSET, BIT16, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT29, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT29, DO_RESET,
		      CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT17, DO_AUTOGATING),
	COMP_MUX_CONF("f0640000.eac", dai_clk_parent, 12288000, 0,
		      0, 0, 0,
		      CG_PERI_CLK_DIV_REG0_OFFSET, BIT16, WID8,
		      CG_CLK_EN_REG1_OFFSET, BIT20, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT30, DO_RESET,
		      CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT18, DO_AUTOGATING),
	COMP_MUX_CONF("f0640000.eacadc", dai_clk_parent, 12288000, 0,
		      0, 0, 0,
		      CG_PERI_CLK_DIV_REG0_OFFSET, BIT16, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT30, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT30, NOT_RESET,
		      CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT18, DO_AUTOGATING),
	COMP_MUX_CONF("sie_io_pxclk", sie_io_pxclk_clk_parent, 108000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT14, WID1,
		      0, 0, 0,
		      0, 0, 0,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("sie2_io_pxclk", sie2_io_pxclk_clk_parent, 108000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT15, WID1,
		      0, 0, 0,
		      0, 0, 0,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("sie4_io_pxclk", sie4_io_pxclk_clk_parent, 108000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT20, WID1,
		      0, 0, 0,
		      0, 0, 0,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("sie1_pxclk", sie1_pxclk_clk_parent, 108000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT16, WID1,
		      0, 0, 0,
		      CG_CLK_EN_REG2_OFFSET, BIT4, NOT_ENABLE,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("sie2_pxclk", sie2_pxclk_clk_parent, 108000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT17, WID1,
		      0, 0, 0,
		      CG_CLK_EN_REG2_OFFSET, BIT5, NOT_ENABLE,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("sie3_pxclk", sie3_pxclk_clk_parent, 108000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT18, WID1,
		      0, 0, 0,
		      CG_CLK_EN_REG2_OFFSET, BIT6, NOT_ENABLE,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("sie4_pxclk", sie4_pxclk_clk_parent, 108000000, 0,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT21, WID1,
		      0, 0, 0,
		      CG_CLK_EN_REG2_OFFSET, BIT7, NOT_ENABLE,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("f0810000.mi", mi_clk_parent, 1920000000, 0,
		      CG_PERI_CLK_RATE_REG0_OFFSET, BIT22, WID2,
		      CG_VIDEO_CLK_DIV_REG_OFFSET, BIT16, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT20, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT20, DO_RESET,
		      CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT12, DO_AUTOGATING),
	COMP_MUX_CONF("f0cc0000.tge", tge_clk_parent, 240000000, 1,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT22, WID1,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT6, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT6, DO_RESET,
		      0, 0, 0),
	COMP_MUX_CONF("f0cc0000.tge2", tge2_clk_parent, 240000000, 1,
		      CG_IPP_CLK_RATE_REG1_OFFSET, BIT23, WID1,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT6, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT6, DO_RESET,
		      0, 0, 0),
	COMP_MUX_CONF("sp_clk", special_clk_parent, 25000000, 1,
		      CG_PERI_CLK_RATE_REG1_OFFSET, BIT8, WID2,
		      CG_PERI_CLK_DIV_REG0_OFFSET, BIT0, WID8,
		      CG_CLK_EN_REG0_OFFSET, BIT12, NOT_ENABLE,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("remote_clk_sel0", remote_clk_parent, 32768, 0,
		      0, 0, 0,
		      0, 0, 0,
		      CG_PERI_CLK_RATE_REG1_OFFSET, BIT21, NOT_ENABLE,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("remote_clk_sel1", remote_clk_parent, 32768, 0,
		      0, 0, 0,
		      0, 0, 0,
		      CG_CLK_EN_REG1_OFFSET, BIT15, NOT_ENABLE,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("remote_528", remote_clk_parent, 32768, 0,
		      0, 0, 0,
		      0, 0, 0,
		      CG_CLK_EN_REG1_OFFSET, BIT12, NOT_ENABLE,
		      CG_SYS_RESET_REG1_OFFSET, BIT12, NOT_RESET,
		      CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT30, DO_AUTOGATING),
	COMP_MUX_CONF("f0250000.remote", remote_clk_parent, 32768, 0,
		      CG_PERI_CLK_RATE_REG1_OFFSET, BIT21, WID1,
		      0, 0, 0,
		      CG_CLK_EN_REG1_OFFSET, BIT12, NOT_ENABLE,
		      CG_SYS_RESET_REG1_OFFSET, BIT12, NOT_RESET,
		      CG_APB_CLK_AUTO_GATING_REG0_OFFSET, BIT30, DO_AUTOGATING),
	COMP_MUX_CONF("f0680000.rng", rng_clk_parent, 160000000, 0,
		      CG_PERI_CLK_RATE_REG1_OFFSET, BIT18, WID1,
		      CG_VIDEO_CLK_DIV_REG_OFFSET, BIT24, WID8,
		      CG_CLK_EN_REG2_OFFSET, BIT25, NOT_ENABLE,
		      CG_SYS_RESET_REG2_OFFSET, BIT25, NOT_RESET,
		      0, 0, 0),
	COMP_MUX_CONF("f0680000.rngro", rngro_clk_parent, 24000000, 0,
		      CG_PERI_CLK_RATE_REG1_OFFSET, BIT19, WID1,
		      CG_PERI_CLK_DIV_REG2_OFFSET, BIT0, WID8,
		      CG_CLK_EN_REG2_OFFSET, BIT28, NOT_ENABLE,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("f0cd0000.vpe", vpe_clk_parent, 240000000, 0,
		      CG_IPP_CLK_RATE_REG0_OFFSET, BIT22, WID2,
		      0, 0, 0,
		      CG_CLK_EN_REG0_OFFSET, BIT21, NOT_ENABLE,
		      CG_SYS_RESET_REG0_OFFSET, BIT21, DO_RESET,
		      CG_CLK_AUTO_GATING_REG0_OFFSET, BIT31, NOT_AUTOGATING),
	COMP_MUX_CONF("siemck_common", siemclk_clk_parent, 240000000, 0,
		      SIEMCLK_COMM_CLK_RATE_REG0_OFFSET, BIT8, WID2,
		      SIEMCLK_COMM_CLK_DIV_REG0_OFFSET, BIT0, WID8,
		      SIEMCLK_COMM_CLK_EN_REG0_OFFSET, BIT2, NOT_ENABLE,
		      0, 0, 0,
		      0, 0, 0),
	COMP_MUX_CONF("f02b0000.eth", eth_root_parent, 50000000, 0,
			0, 0, 0,					// clk mux
			0, 0, 0,					// clk div
			CG_CLK_EN_REG1_OFFSET, BIT29, NOT_ENABLE,
			CG_SYS_RESET_REG1_OFFSET, BIT29, NOT_RESET,
			CG_CLK_AUTO_GATING_REG1_OFFSET, BIT27, DO_AUTOGATING),
	COMP_MUX_CONF("ext_phy_clk", eth_root_parent, 25000000, 0,
		       0, 0, 0,						// clk mux
		       0, 0, 0,						// clk div
		       CG_CLK_EN_REG2_OFFSET, BIT24, NOT_ENABLE,
		       0, 0, 0,
		       0, 0, 0),
};

static struct nvt_clk_gating_init novatek_init_gating_clk[] __initdata = {
	COMP_GATING_CONF(CG_CLK_AUTO_GATING_REG0_OFFSET, 0x0F01FFFF),
	COMP_GATING_CONF(CG_CLK_AUTO_GATING_REG1_OFFSET, 0x01E00900),
	COMP_GATING_CONF(CG_APB_CLK_AUTO_GATING_REG0_OFFSET, 0xFFC7FFFE),
	COMP_GATING_CONF(CG_APB_CLK_AUTO_GATING_REG1_OFFSET, 0x3731095B),
	COMP_GATING_CONF(CG_APB_CLK_AUTO_GATING_REG2_OFFSET, 0x1B),
};

static int novatek_misc_clk_register(void)
{
	struct clk *clk;
	struct clk_hw *clk_hw;
	int ret = 0;


	clk =
	    clk_register_mux(NULL, "apb_clk", apb_clk_parent,
			     ARRAY_SIZE(apb_clk_parent),
			     CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT |
			     CLK_GET_RATE_NOCACHE,
			     remap_base + CG_SYS_CLK_RATE_REG_OFFSET, 8, 2, 0,
			     &cg_lock);
	if (IS_ERR(clk)) {
		pr_err("%s: failed to register clock hardware \"apb_clk\"\n",
		       __func__);
		ret = -EPERM;
	}
	if (clk_register_clkdev(clk, "apb_clk", NULL)) {
		pr_err("%s: failed to register clock device \"apb_clk\"\n",
		       __func__);
		ret = -EPERM;
	} else {
		clk_hw = __clk_get_hw(clk);
		ret = clk_prepare_enable(clk);
		if (ret < 0)
			pr_err("apb_clk prepare & enable failed!\n");
	}

	if (nvt_get_chip_id() == CHIP_NA51055) {
		clk = clk_register_fixed_factor(NULL, "pll6div6", "pll6", 0, 1, 6);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock hardware \"pll6div6\"\n",
			       __func__);
			ret = -EPERM;
		} else {
			ret = clk_prepare_enable(clk);
			if (ret < 0)
				pr_err("pll6div6 prepare & enable failed!\n");
		}
	}

	return ret;
}

static int novatek_eth_clk_register(void)
{
	struct clk *clk;
	int ret = 0;

	if (nvt_get_chip_id() != CHIP_NA51055) {
                struct clk *eth_clk;

                eth_clk = clk_get(NULL, "f02b0000.eth");
                if (IS_ERR(eth_clk )) {
                        pr_err("%s: failed to get eth clock\n", __func__);
                        ret = -EPERM;
                } else {
                        clk = clk_get(NULL, "pll16");
                        if (IS_ERR(clk)) {
                                pr_err("%s: failed to get pll16\n", __func__);
                                ret = -EPERM;
                        }
                        clk_set_parent(eth_clk, clk);
                        clk_put(clk);
                }
                clk_put(eth_clk);

		eth_clk = clk_get(NULL, "ext_phy_clk");
                if (IS_ERR(eth_clk )) {
                        pr_err("%s: failed to get eth clock\n", __func__);
                        ret = -EPERM;
                } else {
                        clk = clk_get(NULL, "pll16");
                        if (IS_ERR(clk)) {
                                pr_err("%s: failed to get pll16\n", __func__);
                                ret = -EPERM;
                        }
                        clk_set_parent(eth_clk, clk);
                        clk_put(clk);
                }
                clk_put(eth_clk);

        }

        return ret;

}

static int max_freq_node = 0;
static int freq_index = 0;
static void nvt_get_max_frequency(struct nvt_composite_mux_clk composite_mux_clks[], int cmux_array_size,
				struct nvt_composite_gate_clk composite_gate_clks[], int cgate_array_size)
{
	int i = 0, j = 0;

	otp_platform_earily_create_resource();
	if (efuse_check_available_extend(EFUSE_ABILITY_CLK_FREQ , 0x0) == TRUE)
		freq_index = 1;
	else
		freq_index = 0;
	otp_platform_earily_release_resource();

	if (nvt_get_chip_id() == CHIP_NA51055) {
		max_freq_node = ARRAY_SIZE(nvt_fixed_max_freq);

		for (i = 0; i < cmux_array_size; i++) {
			composite_mux_clks[i].max_rate = 0;
			for (j = 0; j < max_freq_node; j++) {
				if (strncmp(composite_mux_clks[i].name, nvt_fixed_max_freq[j].name, strlen(composite_mux_clks[i].name)) == 0) {
					composite_mux_clks[i].max_rate = nvt_fixed_max_freq[j].max_freq[freq_index];
				}
			}
		}

		for (i = 0; i < cgate_array_size; i++) {
			composite_gate_clks[i].max_rate = 0;
			for (j = 0; j < max_freq_node; j++) {
				if (strncmp(composite_gate_clks[i].name, nvt_fixed_max_freq[j].name, strlen(composite_gate_clks[i].name)) == 0) {
					composite_gate_clks[i].max_rate = nvt_fixed_max_freq[j].max_freq[freq_index];
				}
			}
		}
	} else {
		max_freq_node = ARRAY_SIZE(nvt_fixed_max_freq_528);

		for (i = 0; i < cmux_array_size; i++) {
			composite_mux_clks[i].max_rate = 0;
			for (j = 0; j < max_freq_node; j++) {
				if (strncmp(composite_mux_clks[i].name, nvt_fixed_max_freq_528[j].name, strlen(composite_mux_clks[i].name)) == 0) {
					composite_mux_clks[i].max_rate = nvt_fixed_max_freq_528[j].max_freq[freq_index];
				}
			}
		}

		for (i = 0; i < cgate_array_size; i++) {
			composite_gate_clks[i].max_rate = 0;
			for (j = 0; j < max_freq_node; j++) {
				if (strncmp(composite_gate_clks[i].name, nvt_fixed_max_freq_528[j].name, strlen(composite_gate_clks[i].name)) == 0) {
					composite_gate_clks[i].max_rate = nvt_fixed_max_freq_528[j].max_freq[freq_index];
				}
			}
		}
	}
}

int nvt_get_max_freq_node(void)
{
	return max_freq_node;
}

void nvt_get_max_freq_info(int index, char *name, int *freq)
{
	if (nvt_get_chip_id() == CHIP_NA51055) {
		strncpy(name, nvt_fixed_max_freq[index].name, CLK_NAME_STR_SIZE);
		*freq = nvt_fixed_max_freq[index].max_freq[freq_index];
	} else {
		strncpy(name, nvt_fixed_max_freq_528[index].name, CLK_NAME_STR_SIZE);
		*freq = nvt_fixed_max_freq_528[index].max_freq[freq_index];
	}
}

static void nvt_get_keeper_array(struct nvt_composite_mux_clk composite_mux_clks[], int cmux_array_size,
				struct nvt_composite_gate_clk composite_gate_clks[], int cgate_array_size)
{
	struct device_node *node, *keeper_node;
	int i, j, keeper_count = 0;
	struct nvt_clk_rate_keeper *keeper;
	const char *name_ptr = NULL;

	keeper = kzalloc(CLK_NAME_STR_CNT * sizeof(struct nvt_clk_rate_keeper), GFP_KERNEL);
	node = of_find_node_by_path("/cg_keeper@0");
	if (node) {
		for_each_child_of_node(node, keeper_node) {
			name_ptr = keeper[keeper_count].name;
			if (!of_property_read_string(keeper_node, "keeper_name", &name_ptr)) {
				strncpy(keeper[keeper_count].name, name_ptr, CLK_NAME_STR_SIZE);
				keeper_count++;
				if (keeper_count == CLK_NAME_STR_CNT) {
					pr_err("exceed keeper limitation");
					break;
				}
			}
		}
	}

	for (i = 0; i < cmux_array_size; i++) {
		composite_mux_clks[i].keep_rate = 0;
		for (j = 0; j < keeper_count; j++) {
			if (strncmp(composite_mux_clks[i].name, keeper[j].name, strlen(keeper[j].name)) == 0) {
				composite_mux_clks[i].keep_rate = 1;
			}
		}
	}

	for (i = 0; i < cgate_array_size; i++) {
		composite_gate_clks[i].keep_rate = 0;
		for (j = 0; j < keeper_count; j++) {
			if (strncmp(composite_gate_clks[i].name, keeper[j].name, strlen(keeper[j].name)) == 0) {
				composite_gate_clks[i].keep_rate = 1;
			}
		}
	}
}

static void __init novatek_clock_init(struct device_node *node)
{
	int ret;
	int fastboot_init = 0;
	int logoboot_init = 0;

	pr_info("novatek_clock_init\n");

	fastboot_init = fastboot_determination();

	nvt_fastboot_detect(fastboot_init);

	logoboot_init = logoboot_determination();

	nvt_logoboot_detect(logoboot_init);

	spin_lock_init(&cg_lock);

	remap_base = of_iomap(node, 0);
	if (IS_ERR((void *)remap_base))
		pr_err("Failed to remap CKG registers!\n");
	else
		nvt_cg_base_remap(remap_base);

	nvt_init_clk_auto_gating(novatek_init_gating_clk,
				ARRAY_SIZE(novatek_init_gating_clk), &cg_lock);

	ret =
	    nvt_fixed_rate_clk_register(novatek_fixed_rate_clk,
					ARRAY_SIZE(novatek_fixed_rate_clk));
	if (ret < 0)
		pr_err("Failed to register fixed rate clk!\n");

	if (nvt_get_chip_id() != CHIP_NA51055) {
		ret =
		    nvt_pll_clk_register(novatek_pll528, ARRAY_SIZE(novatek_pll528),
					 DEFAULT_PLL_DIV_VALUE, &cg_lock);
	} else {
		ret =
		    nvt_pll_clk_register(novatek_pll, ARRAY_SIZE(novatek_pll),
					 DEFAULT_PLL_DIV_VALUE, &cg_lock);
	}

	if (ret < 0)
		pr_err("Failed to register pll clk!\n");

	ret = novatek_misc_clk_register();
	if (ret < 0)
		pr_err("Failed to register misc clk!\n");

	nvt_get_max_frequency(novatek_cmux_clk, ARRAY_SIZE(novatek_cmux_clk),
			novatek_cgate_clk, ARRAY_SIZE(novatek_cgate_clk));

	nvt_get_keeper_array(novatek_cmux_clk, ARRAY_SIZE(novatek_cmux_clk),
			novatek_cgate_clk, ARRAY_SIZE(novatek_cgate_clk));

	ret =
	    nvt_composite_gate_clk_register(novatek_cgate_clk,
					    ARRAY_SIZE
					    (novatek_cgate_clk), &cg_lock);
	if (ret < 0)
		pr_err("Failed to register composite gate clk!\n");

	ret =
	    nvt_composite_group_pwm_clk_register(novatek_cgpwm_clk,
						 ARRAY_SIZE
						 (novatek_cgpwm_clk), &cg_lock);
	if (ret < 0)
		pr_err("Failed to register pwm clk!\n");

	ret =
	    nvt_composite_mux_clk_register(novatek_cmux_clk,
					   ARRAY_SIZE
					   (novatek_cmux_clk), &cg_lock);
	if (ret < 0)
		pr_err("Failed to register composite mux clk!\n");


	ret = novatek_eth_clk_register();
	if (ret < 0)
		pr_err("Failed to setup eth clk!\n");

}

CLK_OF_DECLARE(nvt_core_clk, "nvt,core_clk", novatek_clock_init);
