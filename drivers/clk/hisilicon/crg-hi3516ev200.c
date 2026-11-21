// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * HiSilicon Hi3516E V200 / Hi3516E V300 clock and reset generator (CRG) and
 * system controller clocks.
 *
 * Cortex-A7 generation with the renumbered PERI_CRGn map (PLLs at 0x00 to
 * 0x1c, bus selects at 0x80, USB at 0x140, FMC at 0x144, ETH at 0x16c, crypto
 * at 0x1a0, UART, I2C and SPI at 0x1b8 and 0x1bc, MMC at 0x1f4 and 0x22c).
 * The Hi3516E V300 data sheet (Issue 00B06, chapter 3, CRG) describes every
 * register used here. There is no data sheet register chapter for the
 * Hi3516E V200: it is taken to have the same registers where its vendor
 * code (boot code and device tree) agrees with the Hi3516E V300 sheet, and the
 * places where that is only an inference say so below.
 *
 * Evidence tags in the comments:
 *   DS:   the register, bit and code are in the Hi3516E V300 data sheet.
 *   VEN:  vendor code only (the vendor clock driver, U-Boot or device tree).
 *   INF:  not in the Hi3516E V300 sheet or not stated for the Hi3516E V200:
 *         deduced from a sibling data sheet or from vendor code. Not
 *         confirmed on hardware.
 * Test status: the clock model and the Ethernet MAC and FE PHY resets were
 * booted on a Hi3516E V200 camera. The other resets of the checked table,
 * the eMMC and USB paths, the FOUTVCOPD check firing, the CPU capacity node
 * on an SMP build and the Hi3516E V300 have not been tested on hardware.
 *
 * Boot loader state is never assumed. The CPU, DDR and bus selects need a
 * clock switch that no data sheet register confirms, so they are read only:
 * the clock framework reports what the hardware holds and the probe prints
 * what it found and warns about a value that the tables cannot describe. The
 * UART, FMC, MMC, PWM and Ethernet selects are programmed through the clock
 * framework by their consumers and read back otherwise. The PLLs are only
 * decoded, and the module clocks that the data sheet prints as fixed rates
 * are fixed dividers of the decoded VCO outputs (and of the crystal), so their
 * rates follow the PLL registers; the probe says when a PLL is not running or
 * is not at the operating point of the data sheet.
 *
 * Both providers register early (the CRG with CLK_OF_DECLARE_DRIVER, the
 * system controller too) because the SP804 timers can be probed from
 * time_init(), long before platform drivers run. CLK_OF_DECLARE_DRIVER() does
 * not mark the nodes populated, so a platform device is still created for
 * each: the platform driver below binds to the CRG and registers the reset
 * controller, and the reboot driver binds to the system controller. The reset
 * controller only accepts the soft reset bits of hi3516ev200_resets.
 */

#include <dt-bindings/clock/hi3516ev200-clock.h>
#include <linux/bitfield.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "clk.h"
#include "reset.h"

#define HI3516EV200_CRG_NR_CLKS		120
#define HI3516EV200_SC_NR_CLKS		4

#define HI3516EV200_XTAL_RATE		24000000

/*
 * PLL register pairs: PERI_CRG_PLL0/1 APLL, PERI_CRG_PLL6/7 VPLL (DS).
 * FOUTVCO = Fin * (fbdiv + frac / 2^24) / refdiv, FOUTPOSTDIV = FOUTVCO /
 * (postdiv1 * postdiv2); frac only counts with dsmpd cleared, bypass passes
 * the reference through. The data sheet requires 800 MHz < FOUTVCO <= 3.2 GHz.
 */
#define PLL_PD				BIT(23)
#define PLL_FOUTVCOPD			BIT(22)
#define PLL_POSTDIVPD			BIT(21)
#define PLL_POSTDIV2			GENMASK(30, 28)
#define PLL_POSTDIV1			GENMASK(26, 24)
#define PLL_FRAC			GENMASK(23, 0)
#define PLL_BYPASS			BIT(26)
#define PLL_DSMPD			BIT(24)
#define PLL_REFDIV			GENMASK(17, 12)
#define PLL_FBDIV			GENMASK(11, 0)

#define PLL_VCO_MIN			800000000ULL
#define PLL_VCO_MAX			3200000000ULL

struct hi3516ev200_pll {
	struct clk_hw hw;
	void __iomem *reg;	/* first of the two registers */
	bool vco;		/* the VCO frequency, else the POSTDIV output */
};

#define to_hi3516ev200_pll(_hw)	container_of(_hw, struct hi3516ev200_pll, hw)

/*
 * VCO frequency of the PLL at @reg, 0 if it is powered down or misprogrammed.
 * FOUTVCOPD (bit 22, it switches off the raw VCO output tap that the module
 * clocks divide) is not tested here: hi3516ev200_check_plls() warns about it.
 */
static u64 hi3516ev200_pll_vco(void __iomem *reg, unsigned long parent_rate)
{
	u32 a = readl(reg);
	u32 b = readl(reg + 4);
	u32 refdiv = FIELD_GET(PLL_REFDIV, b);
	u64 vco;

	if ((b & PLL_PD) || !refdiv)
		return 0;

	vco = (u64)parent_rate * FIELD_GET(PLL_FBDIV, b);
	if (!(b & PLL_DSMPD))
		vco += ((u64)parent_rate * FIELD_GET(PLL_FRAC, a)) >> 24;

	return div_u64(vco, refdiv);
}

/* Output rate of the PLL at @reg, 0 if it is powered down or misprogrammed */
static unsigned long hi3516ev200_pll_rate(void __iomem *reg,
					  unsigned long parent_rate)
{
	u32 a = readl(reg);
	u32 b = readl(reg + 4);
	u32 div1 = FIELD_GET(PLL_POSTDIV1, a);
	u32 div2 = FIELD_GET(PLL_POSTDIV2, a);

	if (b & PLL_BYPASS)
		return parent_rate;
	if ((b & PLL_POSTDIVPD) || !div1 || !div2)
		return 0;

	return div_u64(hi3516ev200_pll_vco(reg, parent_rate), div1 * div2);
}

static unsigned long hi3516ev200_pll_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	struct hi3516ev200_pll *pll = to_hi3516ev200_pll(hw);

	if (pll->vco)
		return hi3516ev200_pll_vco(pll->reg, parent_rate);

	return hi3516ev200_pll_rate(pll->reg, parent_rate);
}

static const struct clk_ops hi3516ev200_pll_ops = {
	.recalc_rate = hi3516ev200_pll_recalc_rate,
};

#define CRG_APLL			0x00
#define CRG_VPLL			0x18

/*
 * Each PLL is registered twice: its POSTDIV output (the A7 clock is the APLL
 * one) and its VCO, the tap the module clocks below divide: 600, 360 and 200
 * MHz are not integer fractions of the 900 MHz POSTDIV output of the APLL.
 */
static const struct {
	unsigned int id;
	const char *name;
	unsigned long offset;
	bool vco;
} hi3516ev200_plls[] = {
	{ HI3516EV200_APLL_CLK, "apll", CRG_APLL, false, },
	{ HI3516EV200_VPLL_CLK, "vpll", CRG_VPLL, false, },
	{ HI3516EV200_APLL_VCO_CLK, "apll_vco", CRG_APLL, true, },
	{ HI3516EV200_VPLL_VCO_CLK, "vpll_vco", CRG_VPLL, true, },
};

static void hi3516ev200_unregister_plls(struct hisi_clock_data *clk_data, int n)
{
	while (n--) {
		struct clk **clk = &clk_data->clk_data.clks[hi3516ev200_plls[n].id];
		struct clk_hw *hw = __clk_get_hw(*clk);

		*clk = NULL;
		clk_hw_unregister(hw);
		kfree(to_hi3516ev200_pll(hw));
	}
}

static int __init hi3516ev200_register_plls(struct hisi_clock_data *clk_data)
{
	static const char *const parent_names[] = { "24m" };
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(hi3516ev200_plls); i++) {
		struct clk_init_data init = {
			.name = hi3516ev200_plls[i].name,
			.ops = &hi3516ev200_pll_ops,
			.parent_names = parent_names,
			.num_parents = 1,
		};
		struct hi3516ev200_pll *pll;
		struct clk *clk;

		pll = kzalloc_obj(*pll);
		if (!pll) {
			ret = -ENOMEM;
			goto err_unregister;
		}

		pll->reg = clk_data->base + hi3516ev200_plls[i].offset;
		pll->vco = hi3516ev200_plls[i].vco;
		pll->hw.init = &init;
		clk = clk_register(NULL, &pll->hw);
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			kfree(pll);
			goto err_unregister;
		}

		clk_data->clk_data.clks[hi3516ev200_plls[i].id] = clk;
	}

	return 0;

err_unregister:
	hi3516ev200_unregister_plls(clk_data, i);

	return ret;
}

/*
 * Fixed rates: the crystal, and the 100 kHz and 400 kHz of the card
 * identification mode of the MMC selects, which the data sheet prints as rates
 * and does not derive from anything.
 */
static const struct hisi_fixed_rate_clock hi3516ev200_fixed_rate_clks[] = {
	{ HI3516EV200_FIXED_100K, "100k", NULL, 0, 100000, },
	{ HI3516EV200_FIXED_400K, "400k", NULL, 0, 400000, },
	{ HI3516EV200_FIXED_24M, "24m", NULL, 0, HI3516EV200_XTAL_RATE, },
};

/*
 * Module clocks named by the select enums of the data sheet, which prints them
 * as rates ("all PLL configurations are fixed"). They are fixed dividers of the
 * clock they come from, so that they follow the registers: 3 MHz divides the
 * crystal, the rest divide the VCO of the APLL (1800 MHz after reset: 600, 450,
 * 360, 300, 200, 150, 112.5, 100, 90, 50 and 25 MHz) and of the VPLL (2376 MHz
 * after reset: 54 MHz). The data sheet prints the rates but not the divider or
 * the tap of each one: the ratios are those of the printed rates to the VCO
 * rates at the reset values of the PLL registers (every rate has exactly one
 * integer divider of one of the two VCOs, and 600, 360 and 200 MHz are not
 * integer fractions of the POSTDIV outputs), so the VCO as the tap is an
 * inference. A PLL that is powered down or misprogrammed makes its clocks read
 * 0 Hz, and the probe says so.
 */
static const struct hisi_fixed_factor_clock hi3516ev200_fixed_factor_clks[] = {
	{ HI3516EV200_FIXED_3M, "3m", "24m", 1, 8, 0, },
	{ HI3516EV200_FIXED_25M, "25m", "apll_vco", 1, 72, 0, },
	{ HI3516EV200_FIXED_50M, "50m", "apll_vco", 1, 36, 0, },
	{ HI3516EV200_FIXED_54M, "54m", "vpll_vco", 1, 44, 0, },
	{ HI3516EV200_FIXED_90M, "90m", "apll_vco", 1, 20, 0, },
	{ HI3516EV200_FIXED_100M, "100m", "apll_vco", 1, 18, 0, },
	{ HI3516EV200_FIXED_112M5, "112.5m", "apll_vco", 1, 16, 0, },
	{ HI3516EV200_FIXED_150M, "150m", "apll_vco", 1, 12, 0, },
	{ HI3516EV200_FIXED_200M, "200m", "apll_vco", 1, 9, 0, },
	{ HI3516EV200_FIXED_300M, "300m", "apll_vco", 1, 6, 0, },
	{ HI3516EV200_FIXED_360M, "360m", "apll_vco", 1, 5, 0, },
	{ HI3516EV200_FIXED_450M, "450m", "apll_vco", 1, 4, 0, },
	{ HI3516EV200_FIXED_600M, "600m", "apll_vco", 1, 3, 0, },
};

/*
 * PERI_CRG32 (DS, 0x80): a7_cksel [1:0] (00 24 MHz, 01 900 MHz, 11 600 MHz),
 * ddr_cksel [5:3] (000 24 MHz, 001 450 MHz, 011 300 MHz, others cannot be
 * selected), sysaxi_cksel [6] (24 or 200 MHz), syscfg_cksel [8] (24 MHz; the
 * clock table of the data sheet adds 100 MHz, INF: code 1), sysapb_cksel [10]
 * (24 or 50 MHz). The 900 MHz of the A7 is the APLL POSTDIV output itself, so
 * it is the APLL clock and the CPU rate follows the PLL registers.
 *
 * The switch has no readback register in the Hi3516E V300 sheet (the vendor
 * boot code of the Hi3516E V200 polls 0x1e0 for it), so these selects are
 * read only: the boot loader's choice is reported, never changed.
 */
#define CRG_SYSSEL			0x080
#define CRG_SYSSEL_A7			GENMASK(1, 0)
#define CRG_SYSSEL_DDR			GENMASK(5, 3)
#define CRG_SYSSEL_AXI			BIT(6)
#define CRG_SYSSEL_CFG			BIT(8)
#define CRG_SYSSEL_APB			BIT(10)

static const char *const a7_mux_p[] = { "24m", "apll", "600m" };
static const u32 a7_mux_table[] = { 0, 1, 3 };
static const char *const ddr_mux_p[] = { "24m", "450m", "300m" };
static const u32 ddr_mux_table[] = { 0, 1, 3 };
static const char *const sysaxi_mux_p[] = { "24m", "200m" };
static const char *const syscfg_mux_p[] = { "24m", "100m" };
static const char *const apb_mux_p[] = { "24m", "50m" };

static const struct hisi_mux_clock hi3516ev200_core_mux_clks[] = {
	{ HI3516EV200_A7_MUX, "a7_mux", a7_mux_p, ARRAY_SIZE(a7_mux_p),
		0, CRG_SYSSEL, 0, 2, CLK_MUX_READ_ONLY, a7_mux_table, },
	{ HI3516EV200_DDR_MUX, "ddr_mux", ddr_mux_p, ARRAY_SIZE(ddr_mux_p),
		0, CRG_SYSSEL, 3, 3, CLK_MUX_READ_ONLY, ddr_mux_table, },
	{ HI3516EV200_SYSAXI_CLK, "sysaxi_mux", sysaxi_mux_p,
		ARRAY_SIZE(sysaxi_mux_p), 0, CRG_SYSSEL, 6, 1,
		CLK_MUX_READ_ONLY, NULL, },
	{ HI3516EV200_SYSCFG_MUX, "syscfg_mux", syscfg_mux_p,
		ARRAY_SIZE(syscfg_mux_p), 0, CRG_SYSSEL, 8, 1,
		CLK_MUX_READ_ONLY, NULL, },
	/* GPIO, timers, watchdog, RTC, I2C and the AMBA bus clocks follow it */
	{ HI3516EV200_SYSAPB_CLK, "apb", apb_mux_p, ARRAY_SIZE(apb_mux_p),
		0, CRG_SYSSEL, 10, 1, CLK_MUX_READ_ONLY, NULL, },
};

/*
 * Module clock selects.
 *
 * PERI_CRG111 (0x1bc) uartN_cksel [18..20]: 0 is 24 MHz (DS). Code 1 is 3 MHz
 * in the Hi3516E V300 sheet, 6 MHz in the vendor clock drivers of both chips
 * and in the Hi3516A V300 sheet. INF: the rate of code 1 is unverified, measure
 * a baud rate before trusting it (the probe warns when a select holds 1).
 * PERI_CRG81 (0x144) fmc_cksel [4:2] (DS text, the RDL enum is truncated):
 * 24, 100, 150, 200 MHz, 300 and 360 MHz in DDR mode only, others reserved. The
 * clock at the SPI pins is half (SDR mode) or a quarter (DDR mode) of the
 * selected rate (DS), a user of this clock must account for that.
 * PERI_CRG125 (0x1f4) emmc_cksel and PERI_CRG139 (0x22c) sdio1_cksel [26:24]
 * (DS): 100 kHz, 400 kHz, 25, 50 MHz, and 90, 112.5, 150 MHz for the eMMC only.
 * The SDIO1 select is registered three bits wide with four parents, so that a
 * reserved code is seen as one and a write always leaves a valid code.
 * PERI_CRG111 pwm_cksel [9:8] (DS): 3 MHz, 50 MHz, 1X is 24 MHz.
 * PERI_CRG91 (0x16c) ethcore_cksel [7] (DS): 0 100 MHz, 1 54 MHz.
 */
static const char *const uart_mux_p[] = { "24m", "3m" };

static const char *const fmc_mux_p[] = {
	"24m", "100m", "150m", "200m", "300m", "360m"
};

static const char *const mmc0_mux_p[] = {
	"100k", "400k", "25m", "50m", "90m", "112.5m", "150m"
};

static const char *const mmc1_mux_p[] = { "100k", "400k", "25m", "50m" };
static const char *const pwm_mux_p[] = { "3m", "50m", "24m", "24m" };
static const char *const eth_core_mux_p[] = { "100m", "54m" };

#define CRG_UARTSEL			0x1bc
#define CRG_FMC				0x144
#define CRG_MMC0			0x1f4
#define CRG_MMC1			0x22c
#define CRG_ETH				0x16c

static const struct hisi_mux_clock hi3516ev200_mux_clks[] = {
	{ HI3516EV200_UART0_MUX, "uart0_mux", uart_mux_p, ARRAY_SIZE(uart_mux_p),
		0, CRG_UARTSEL, 18, 1, 0, NULL, },
	{ HI3516EV200_UART1_MUX, "uart1_mux", uart_mux_p, ARRAY_SIZE(uart_mux_p),
		0, CRG_UARTSEL, 19, 1, 0, NULL, },
	{ HI3516EV200_UART2_MUX, "uart2_mux", uart_mux_p, ARRAY_SIZE(uart_mux_p),
		0, CRG_UARTSEL, 20, 1, 0, NULL, },
	{ HI3516EV200_FMC_MUX, "fmc_mux", fmc_mux_p, ARRAY_SIZE(fmc_mux_p),
		0, CRG_FMC, 2, 3, 0, NULL, },
	{ HI3516EV200_MMC0_MUX, "mmc0_mux", mmc0_mux_p, ARRAY_SIZE(mmc0_mux_p),
		0, CRG_MMC0, 24, 3, 0, NULL, },
	{ HI3516EV200_MMC1_MUX, "mmc1_mux", mmc1_mux_p, ARRAY_SIZE(mmc1_mux_p),
		0, CRG_MMC1, 24, 3, 0, NULL, },
	{ HI3516EV200_PWM_MUX, "pwm_mux", pwm_mux_p, ARRAY_SIZE(pwm_mux_p),
		0, 0x1bc, 8, 2, 0, NULL, },
	{ HI3516EV200_ETH_CORE_MUX, "eth_core_mux", eth_core_mux_p,
		ARRAY_SIZE(eth_core_mux_p), 0, CRG_ETH, 7, 1, 0, NULL, },
};

/*
 * Clock gates (set = enabled). The soft reset bits of the same registers
 * belong to the reset controller. A gate behind a select passes a rate request
 * on to the select, so that clk_set_rate() of a consumer (the SPI NOR driver
 * of the FMC, an MMC host) picks the closest input. The clock core switches
 * off every gate that no consumer holds at the end of boot: a block whose
 * driver or device tree node does not name its clock loses it.
 */
static const struct hisi_gate_clock hi3516ev200_gate_clks[] = {
	/* PERI_CRG110 (DS): UART0..2 gates [0..2], I2C0..2 gates [11..13] */
	{ HI3516EV200_UART0_CLK, "clk_uart0", "uart0_mux", 0, 0x1b8, 0, 0, },
	{ HI3516EV200_UART1_CLK, "clk_uart1", "uart1_mux", 0, 0x1b8, 1, 0, },
	{ HI3516EV200_UART2_CLK, "clk_uart2", "uart2_mux", 0, 0x1b8, 2, 0, },
	/* the I2C reference clock is PCLK, 50 MHz (DS), taken as SYSAPB (INF) */
	{ HI3516EV200_I2C0_CLK, "clk_i2c0", "apb", 0, 0x1b8, 11, 0, },
	{ HI3516EV200_I2C1_CLK, "clk_i2c1", "apb", 0, 0x1b8, 12, 0, },
	{ HI3516EV200_I2C2_CLK, "clk_i2c2", "apb", 0, 0x1b8, 13, 0, },
	/* PERI_CRG111 (DS): SPI0/1 gates [12/13], reference clock 100 MHz (DS) */
	{ HI3516EV200_SPI0_CLK, "clk_spi0", "100m", 0, 0x1bc, 12, 0, },
	{ HI3516EV200_SPI1_CLK, "clk_spi1", "100m", 0, 0x1bc, 13, 0, },
	/*
	 * PWM gate [7], OTP gate [1] (DS). LSADC gate [23]: the sheet names the bit
	 * ls_adc_cksel with the reset text of a soft reset (a typesetting error), the
	 * Hi3516A V300 sheet has the gate there (INF). Since the meaning of the bit
	 * is inferred, the clock core must not clear it as unused
	 * (CLK_IGNORE_UNUSED): only a consumer that names the clock writes it. OTP
	 * and LSADC rates are not stated.
	 *
	 * The PWM gate must never be cleared as unused, whatever the data sheet
	 * says: the vendor boot loader starts PWM0 as the core voltage control of
	 * the adaptive voltage scaling (SC_PM_PWM_SEL, duty from the HPM word) and
	 * the SoC runs from that output for as long as nothing else drives it.
	 * Stopping the PWM clock freezes the output at whatever level it had at that
	 * moment, so the core voltage moves by an amount that depends on the phase
	 * of the cycle: after a power-up, with no PWM driver to claim the clock,
	 * that crashed the kernel at random (NULL pointer faults or a hard hang
	 * within a millisecond of the unused clocks being disabled).
	 */
	{ HI3516EV200_PWM_CLK, "clk_pwm", "pwm_mux",
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x1bc, 7, 0, },
	{ HI3516EV200_OTP_CLK, "clk_otp", NULL, 0, 0x1bc, 1, 0, },
	{ HI3516EV200_LSADC_CLK, "clk_lsadc", NULL, CLK_IGNORE_UNUSED, 0x1bc, 23, 0, },
	/*
	 * PERI_CRG81 fmc_cken [1], PERI_CRG125 emmc_cken [28], PERI_CRG139
	 * sdio1_cken [28]. The boot ROM reads the flash through the FMC (SPI NOR
	 * or NAND) or the eMMC, as the boot mode pins say, and the data sheet does
	 * not say that a reboot restores the gate of the boot medium, so the FMC
	 * and eMMC gates are never switched off as unused, whether or not a driver
	 * holds them: the gate of the medium that the machine booted from is left
	 * as the boot loader had it. The SDIO1 gate is not a boot medium. This is a
	 * precaution, not a measured need.
	 */
	{ HI3516EV200_FMC_CLK, "clk_fmc", "fmc_mux",
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, CRG_FMC, 1, 0, },
	{ HI3516EV200_MMC0_CLK, "clk_mmc0", "mmc0_mux",
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, CRG_MMC0, 28, 0, },
	{ HI3516EV200_MMC1_CLK, "clk_mmc1", "mmc1_mux", CLK_SET_RATE_PARENT, CRG_MMC1, 28, 0, },
	/*
	 * PERI_CRG91 eth_cken [1], fephy_cken [2] (DS of the Hi3516E V300). On a
	 * Hi3516E V200 camera, clearing bit 2 makes the internal PHY stop
	 * answering on MDIO and setting it again restores it.
	 */
	{ HI3516EV200_ETH0_CLK, "clk_eth", "eth_core_mux", CLK_SET_RATE_PARENT, CRG_ETH, 1, 0, },
	{ HI3516EV200_ETH_FEPHY_CLK, "clk_eth_fephy", NULL, 0, CRG_ETH, 2, 0, },
	/*
	 * PERI_CRG80 (DS): bus [8], ref [9], utmi [12], PHY crystal [2], PHY APB
	 * [11]. The free clock select [13] (UTMI clock or a free 60 MHz clock) is not
	 * modeled: the rate of the UTMI clock is not stated.
	 */
	{ HI3516EV200_USB2_BUS_CLK, "clk_usb2_bus", NULL, 0, 0x140, 8, 0, },
	{ HI3516EV200_USB2_REF_CLK, "clk_usb2_ref", NULL, 0, 0x140, 9, 0, },
	{ HI3516EV200_USB2_UTMI_CLK, "clk_usb2_utmi", NULL, 0, 0x140, 12, 0, },
	{ HI3516EV200_USB2_PHY_XO_CLK, "clk_usb2_phy_xo", "24m", 0, 0x140, 2, 0, },
	{ HI3516EV200_USB2_PHY_APB_CLK, "clk_usb2_phy_apb", "apb", 0, 0x140, 11, 0, },
	/* PERI_CRG101 (DS): DMAC APB gate [1], AXI gate [2] */
	{ HI3516EV200_EDMAC_CLK, "clk_edmac_apb", "apb", 0, 0x194, 1, 0, },
	{ HI3516EV200_EDMAC_AXICLK, "clk_edmac_axi", "sysaxi_mux", 0, 0x194, 2, 0, },
	/* PERI_CRG104 (DS): KLAD gate [1], TRNG [3], RSA [5], SPAcc [9] */
	{ HI3516EV200_KLAD_CLK, "clk_klad", NULL, 0, 0x1a0, 1, 0, },
	{ HI3516EV200_TRNG_CLK, "clk_trng", NULL, 0, 0x1a0, 3, 0, },
	{ HI3516EV200_RSA_CLK, "clk_rsa", NULL, 0, 0x1a0, 5, 0, },
	{ HI3516EV200_SPACC_CLK, "clk_spacc", NULL, 0, 0x1a0, 9, 0, },
};

/*
 * USB PHY PLL reference gate, bit 4 of PERI_CRG80. The vendor code of both
 * chips toggles it (VEN) although the Hi3516E V300 sheet marks the bit
 * reserved. It is provided for both because the device trees of both chips
 * give it to the USB PHY node, as the clock drivers it replaces did. INF:
 * confirm on the Hi3516E V200 and V300 dumps of 0x140. Since the meaning of the
 * bit is inferred, the clock core must not clear it as unused
 * (CLK_IGNORE_UNUSED): only the PHY driver that names the clock writes it.
 */
static const struct hisi_gate_clock hi3516ev200_extra_gates[] = {
	{ HI3516EV200_USB2_PHY_PLL_CLK, "clk_usb2_phy_pll", NULL,
		CLK_IGNORE_UNUSED, 0x140, 4, 0, },
};

struct hi3516ev200_crg_chip {
	const char *name;
	const struct hisi_gate_clock *extra_gates;
	int num_extra_gates;
};

static const struct hi3516ev200_crg_chip hi3516ev200_chip = {
	.name = "Hi3516E V200",
	.extra_gates = hi3516ev200_extra_gates,
	.num_extra_gates = ARRAY_SIZE(hi3516ev200_extra_gates),
};

static const struct hi3516ev200_crg_chip hi3516ev300_chip = {
	.name = "Hi3516E V300",
	.extra_gates = hi3516ev200_extra_gates,
	.num_extra_gates = ARRAY_SIZE(hi3516ev200_extra_gates),
};

/*
 * Read back what the boot loader left and say so when it is not what the
 * tables assume. Nothing is written.
 *
 * PERI_CRG_PLL122 (0x1e8): apll_lock [0], vpll_lock [2] (DS; the vendor boot
 * code of the Hi3516E V200 waits for 5, that is both bits).
 */
#define CRG_PLL_LOCK			0x1e8
#define CRG_PLL_LOCK_APLL		BIT(0)
#define CRG_PLL_LOCK_VPLL		BIT(2)

static void __init hi3516ev200_check_plls(struct device_node *np,
					  struct hisi_clock_data *clk_data)
{
	static const struct {
		unsigned long offset;
		u64 vco;
		u32 lock;
		const char *name;
	} pll[] = {
		{ CRG_APLL, 1800000000ULL, CRG_PLL_LOCK_APLL, "APLL", },
		{ CRG_VPLL, 2376000000ULL, CRG_PLL_LOCK_VPLL, "VPLL", },
	};
	u32 status = readl(clk_data->base + CRG_PLL_LOCK);
	int i;

	for (i = 0; i < ARRAY_SIZE(pll); i++) {
		void __iomem *reg = clk_data->base + pll[i].offset;
		u64 vco = hi3516ev200_pll_vco(reg, HI3516EV200_XTAL_RATE);

		/*
		 * The VCO decode ignores the bypass bit, so a bypassed PLL
		 * passes the VCO checks below: say so, because its POSTDIV
		 * clock (the A7 clock for the APLL) then reads the 24 MHz
		 * reference. The data sheet does not say that the bypass
		 * reaches the module clocks, which are modeled as dividers
		 * of the VCO and keep their rates.
		 */
		if (readl(reg + 4) & PLL_BYPASS)
			pr_warn("%pOF: %s is bypassed, its POSTDIV clock reads the 24 MHz reference (the module clocks divide the VCO and are not changed)\n",
				np, pll[i].name);
		else if (!hi3516ev200_pll_rate(reg, HI3516EV200_XTAL_RATE))
			pr_warn("%pOF: %s POSTDIV output is off or misprogrammed\n",
				np, pll[i].name);
		/*
		 * FOUTVCOPD powers down the raw VCO output that the module
		 * clocks divide (inferred tap), while the VCO decode does not
		 * look at it: the clocks would report rates of a tap that is off.
		 */
		if (readl(reg + 4) & PLL_FOUTVCOPD)
			pr_warn("%pOF: %s VCO output is powered down (FOUTVCOPD), the module clocks derived from it may not run although they report a rate\n",
				np, pll[i].name);
		if (!(status & pll[i].lock))
			pr_warn("%pOF: %s is not locked (PLL status %#x)\n",
				np, pll[i].name, status);
		if (!vco)
			pr_warn("%pOF: %s is powered down or misprogrammed, the module clocks derived from it read 0 Hz\n",
				np, pll[i].name);
		else if (vco <= PLL_VCO_MIN || vco > PLL_VCO_MAX)
			pr_warn("%pOF: %s VCO at %llu Hz is outside the range the data sheet allows\n",
				np, pll[i].name, vco);
		else if (vco != pll[i].vco)
			pr_warn("%pOF: %s VCO runs at %llu Hz, not the %llu Hz of the data sheet: the module clocks run at other rates than their names say\n",
				np, pll[i].name, vco, pll[i].vco);
	}
}

static void __init hi3516ev200_check_selects(struct device_node *np,
					     struct hisi_clock_data *clk_data)
{
	u32 sel = readl(clk_data->base + CRG_SYSSEL);
	u32 uart = readl(clk_data->base + CRG_UARTSEL);
	u32 fmc = FIELD_GET(GENMASK(4, 2), readl(clk_data->base + CRG_FMC));
	u32 mmc0 = FIELD_GET(GENMASK(26, 24), readl(clk_data->base + CRG_MMC0));
	u32 mmc1 = FIELD_GET(GENMASK(26, 24), readl(clk_data->base + CRG_MMC1));
	u32 a7 = FIELD_GET(CRG_SYSSEL_A7, sel);
	u32 ddr = FIELD_GET(CRG_SYSSEL_DDR, sel);
	int i;

	/* the gates of the boot media are never switched off as unused: say what
	 * they were left at
	 */
	pr_info("%pOF: the FMC clock gate is %s and the eMMC clock gate is %s, both are left as they are (the boot ROM reads the flash through one of them)\n",
		np, readl(clk_data->base + CRG_FMC) & BIT(1) ? "on" : "off",
		readl(clk_data->base + CRG_MMC0) & BIT(28) ? "on" : "off");
	pr_info("%pOF: A7 select %u, DDR select %u, SYSAXI %s, SYSCFG %s (100 MHz is INF), SYSAPB %s MHz (PERI_CRG32 %#x)\n",
		np, a7, ddr, sel & CRG_SYSSEL_AXI ? "200" : "24",
		sel & CRG_SYSSEL_CFG ? "100" : "24",
		sel & CRG_SYSSEL_APB ? "50" : "24", sel);

	if (a7 == 2)
		pr_warn("%pOF: A7 clock select holds the reserved code 2 (PERI_CRG32 %#x)\n",
			np, sel);
	if (a7 == 0)
		pr_warn("%pOF: the A7 runs from the 24 MHz crystal (PERI_CRG32 %#x)\n",
			np, sel);
	if (ddr != 0 && ddr != 1 && ddr != 3)
		pr_warn("%pOF: DDR clock select %u cannot be selected (PERI_CRG32 %#x)\n",
			np, ddr, sel);
	if (!(sel & CRG_SYSSEL_APB))
		pr_warn("%pOF: SYSAPB runs from the 24 MHz crystal, the timers and the AMBA bus clocks are slow\n",
			np);
	if (fmc >= ARRAY_SIZE(fmc_mux_p))
		pr_warn("%pOF: FMC clock select holds the reserved code %u\n",
			np, fmc);
	if (mmc0 >= ARRAY_SIZE(mmc0_mux_p))
		pr_warn("%pOF: eMMC clock select holds the reserved code %u\n",
			np, mmc0);
	if (mmc1 >= ARRAY_SIZE(mmc1_mux_p))
		pr_warn("%pOF: SDIO1 clock select holds the reserved code %u, SDIO1 has no more than 50 MHz\n",
			np, mmc1);
	for (i = 0; i < 3; i++)
		if (uart & BIT(18 + i))
			pr_warn("%pOF: UART%d clock select is 1: taken as 3 MHz (data sheet), the vendor driver says 6 MHz, unverified\n",
				np, i);
}

/*
 * A clock id that no table registers must not give the caller a silent dummy
 * clock (a NULL clk is a valid no-op clock to the framework): hand out an error.
 */
static void __init hi3516ev200_fill_holes(struct hisi_clock_data *clk_data, int n)
{
	int i;

	for (i = 0; i < n; i++)
		if (!clk_data->clk_data.clks[i])
			clk_data->clk_data.clks[i] = ERR_PTR(-ENOENT);
}

static void __init hi3516ev200_crg_clk_init(struct device_node *np,
					    const struct hi3516ev200_crg_chip *chip)
{
	struct hisi_clock_data *clk_data;

	clk_data = hisi_clk_init(np, HI3516EV200_CRG_NR_CLKS);
	if (!clk_data)
		return;

	/* each helper cleans up after itself when its own group fails */
	if (hisi_clk_register_fixed_rate(hi3516ev200_fixed_rate_clks,
					 ARRAY_SIZE(hi3516ev200_fixed_rate_clks), clk_data))
		goto err_provider;
	if (hi3516ev200_register_plls(clk_data))
		goto err_fixed_rate;
	if (hisi_clk_register_fixed_factor(hi3516ev200_fixed_factor_clks,
					   ARRAY_SIZE(hi3516ev200_fixed_factor_clks),
					   clk_data))
		goto err_plls;
	if (hisi_clk_register_mux(hi3516ev200_core_mux_clks,
				  ARRAY_SIZE(hi3516ev200_core_mux_clks), clk_data))
		goto err_fixed_factor;
	if (hisi_clk_register_mux(hi3516ev200_mux_clks,
				  ARRAY_SIZE(hi3516ev200_mux_clks), clk_data))
		goto err_core_mux;
	if (hisi_clk_register_gate(hi3516ev200_gate_clks,
				   ARRAY_SIZE(hi3516ev200_gate_clks), clk_data))
		goto err_mux;
	if (hisi_clk_register_gate(chip->extra_gates, chip->num_extra_gates, clk_data))
		goto err_gate;

	hi3516ev200_fill_holes(clk_data, HI3516EV200_CRG_NR_CLKS);
	pr_info("%pOF: %s CRG\n", np, chip->name);
	hi3516ev200_check_plls(np, clk_data);
	hi3516ev200_check_selects(np, clk_data);

	return;

err_gate:
	hisi_clk_unregister_gate(hi3516ev200_gate_clks,
				 ARRAY_SIZE(hi3516ev200_gate_clks), clk_data);
err_mux:
	hisi_clk_unregister_mux(hi3516ev200_mux_clks,
				ARRAY_SIZE(hi3516ev200_mux_clks), clk_data);
err_core_mux:
	hisi_clk_unregister_mux(hi3516ev200_core_mux_clks,
				ARRAY_SIZE(hi3516ev200_core_mux_clks), clk_data);
err_fixed_factor:
	hisi_clk_unregister_fixed_factor(hi3516ev200_fixed_factor_clks,
					 ARRAY_SIZE(hi3516ev200_fixed_factor_clks),
					 clk_data);
err_plls:
	hi3516ev200_unregister_plls(clk_data, ARRAY_SIZE(hi3516ev200_plls));
err_fixed_rate:
	hisi_clk_unregister_fixed_rate(hi3516ev200_fixed_rate_clks,
				       ARRAY_SIZE(hi3516ev200_fixed_rate_clks), clk_data);
err_provider:
	pr_err("%pOF: failed to register all clocks\n", np);
	/* no provider at all is better than one with missing clocks */
	of_clk_del_provider(np);
	iounmap(clk_data->base);
	kfree(clk_data->clk_data.clks);
	kfree(clk_data);
}

static void __init hi3516ev200_crg_init(struct device_node *np)
{
	hi3516ev200_crg_clk_init(np, &hi3516ev200_chip);
}

static void __init hi3516ev300_crg_init(struct device_node *np)
{
	hi3516ev200_crg_clk_init(np, &hi3516ev300_chip);
}

CLK_OF_DECLARE_DRIVER(hi3516ev200_crg, "hisilicon,hi3516ev200-crg", hi3516ev200_crg_init);
CLK_OF_DECLARE_DRIVER(hi3516ev300_crg, "hisilicon,hi3516ev300-crg", hi3516ev300_crg_init);
/* old names of the same compatibles, still used by existing device trees */
CLK_OF_DECLARE_DRIVER(hi3516ev200_clock, "hisilicon,hi3516ev200-clock", hi3516ev200_crg_init);
CLK_OF_DECLARE_DRIVER(hi3516ev300_clock, "hisilicon,hi3516ev300-clock", hi3516ev300_crg_init);

/* ---------------- system controller (sysctrl@12020000) ---------------- */

/*
 * SC_CTRL (sysctrl + 0x0) timeren0ov..timeren3ov, bits 16..19 (DS): the count
 * clock of each SP804 timer, 0 the 3 MHz clock, 1 the bus clock (50 MHz,
 * SYSAPB). INF for the Hi3516E V200: its vendor code proves bit 16 only.
 *
 * SC_CTRL is write protected while SC_LOCKEN reads non-zero (DS: reading it
 * returns the lock state, 0 is unlocked). The driver never writes the unlock
 * key: when the register is locked the selects are registered read only, so
 * that the rate the framework reports is the one the hardware has, and the
 * probe says so, also because a locked SC_SYSRES makes the reboot do nothing.
 */
#define SC_CTRL				0x00
#define SC_LOCKEN			0x44

static const char *const hi3516ev200_timer_mux_p[] = { "3m", "apb" };

static const struct hisi_mux_clock hi3516ev200_sysctrl_mux[] = {
	{ HI3516EV200_TIMER0, "timer0", hi3516ev200_timer_mux_p,
		ARRAY_SIZE(hi3516ev200_timer_mux_p), 0, SC_CTRL, 16, 1, 0, NULL, },
	{ HI3516EV200_TIMER1, "timer1", hi3516ev200_timer_mux_p,
		ARRAY_SIZE(hi3516ev200_timer_mux_p), 0, SC_CTRL, 17, 1, 0, NULL, },
	{ HI3516EV200_TIMER2, "timer2", hi3516ev200_timer_mux_p,
		ARRAY_SIZE(hi3516ev200_timer_mux_p), 0, SC_CTRL, 18, 1, 0, NULL, },
	{ HI3516EV200_TIMER3, "timer3", hi3516ev200_timer_mux_p,
		ARRAY_SIZE(hi3516ev200_timer_mux_p), 0, SC_CTRL, 19, 1, 0, NULL, },
};

static void __init hi3516ev200_sysctrl_clk_init(struct device_node *np)
{
	struct hisi_mux_clock mux[ARRAY_SIZE(hi3516ev200_sysctrl_mux)];
	struct hisi_clock_data *clk_data;
	int i;

	clk_data = hisi_clk_init(np, HI3516EV200_SC_NR_CLKS);
	if (!clk_data)
		return;

	memcpy(mux, hi3516ev200_sysctrl_mux, sizeof(mux));
	if (readl(clk_data->base + SC_LOCKEN)) {
		pr_warn("%pOF: SC_LOCKEN is locked, the timer clock selects in SC_CTRL are read only and a reboot through SC_SYSRES will not work\n",
			np);
		for (i = 0; i < ARRAY_SIZE(mux); i++)
			mux[i].mux_flags |= CLK_MUX_READ_ONLY;
	}

	if (hisi_clk_register_mux(mux, ARRAY_SIZE(mux), clk_data)) {
		pr_err("%pOF: failed to register the timer clocks\n", np);
		/* no provider at all is better than one with missing timer clocks */
		of_clk_del_provider(np);
		iounmap(clk_data->base);
		kfree(clk_data->clk_data.clks);
		kfree(clk_data);
		return;
	}

	hi3516ev200_fill_holes(clk_data, HI3516EV200_SC_NR_CLKS);
}

CLK_OF_DECLARE_DRIVER(hi3516ev200_sysctrl, "hisilicon,hi3516ev200-sysctrl",
		      hi3516ev200_sysctrl_clk_init);
CLK_OF_DECLARE_DRIVER(hi3516ev300_sysctrl, "hisilicon,hi3516ev300-sysctrl",
		      hi3516ev200_sysctrl_clk_init);

/* ---------------- platform driver (reset controller) ---------------- */

/*
 * The soft reset bits of the CRG by register, from the PERI_CRGn descriptions
 * of the Hi3516E V300 data sheet (DS) and, where the Hi3516E V200 vendor device
 * tree uses a bit that the sheet does not list, from that code (VEN). A reset
 * specifier of a device tree is only accepted when it names one of them. Left
 * out on purpose: the ARM core, debug, L2 and CoreSight resets (PERI_CRG30),
 * because resetting the CPU a kernel runs on never returns, the DDR resets
 * (PERI_CRG31) and the DDR test reset (PERI_CRG102), because the memory
 * controller must not be touched from the kernel, the I2C3 reset of PERI_CRG110
 * (the chip has no I2C3) and the two resets that only the vendor register map
 * has (the motion detection and eFuse resets), which the data sheet does not
 * list. The reset bit of the sample DLL of the MMC controllers (bit 29 of
 * PERI_CRG125 and PERI_CRG139, VEN) is reserved in the data sheet; the vendor
 * device tree gives it to the MMC nodes as their DLL reset.
 */
static const struct hisi_reset_bits hi3516ev200_resets[] = {
	{ 0x0a4, BIT(0) },				/* VEDU */
	{ 0x0a8, BIT(0) },				/* VPSS */
	{ 0x0ac, BIT(0) },				/* VGS */
	{ 0x0b0, BIT(0) },				/* JPGE */
	{ 0x0bc, BIT(0) },				/* IVE */
	{ 0x0f0, BIT(6) | BIT(1) },			/* sensor 0 */
	{ 0x0f4, BIT(12) | BIT(10) },			/* VICAP, VI port 0 */
	{ 0x0f8, BIT(6) | BIT(4) },			/* MIPI */
	{ 0x0fc, BIT(5) },				/* VIPROC */
	{ 0x100, GENMASK(17, 16) | GENMASK(13, 12) },	/* ISP 0 and 1 */
	{ 0x108, BIT(0) },				/* VDP */
	{ 0x140, BIT(10) | BIT(3) | BIT(1) | BIT(0) },	/* USB 2.0 and its PHY */
	{ 0x144, BIT(0) },				/* FMC */
	{ 0x16c, BIT(3) | BIT(0) },			/* FE PHY, Ethernet */
	{ 0x190, BIT(1) },				/* GZIP */
	{ 0x194, BIT(0) },				/* DMAC */
	{ 0x19c, BIT(4) | BIT(3) | BIT(0) },		/* audio codec, AIAO */
	{ 0x1a0, BIT(8) | BIT(4) | BIT(2) | BIT(0) },	/* crypto engines */
	{ 0x1b8, GENMASK(21, 19) | GENMASK(7, 5) },	/* I2C0 to I2C2, UARTs */
	{ 0x1bc, BIT(16) | BIT(15) | BIT(6) | BIT(4) | BIT(2) },	/* SPI, PWM, LSADC */
	{ 0x1c0, BIT(2) },				/* SHPM */
	{ 0x1f4, BIT(29) | BIT(27) | BIT(23) | BIT(22) },	/* eMMC */
	{ 0x22c, BIT(29) | BIT(27) | BIT(23) | BIT(22) },	/* SDIO1 */
};

static int hi3516ev200_crg_probe(struct platform_device *pdev)
{
	/* the gates in this register block share it with the reset bits */
	return PTR_ERR_OR_ZERO(hisi_reset_init_checked(pdev, hisi_clk_get_lock(),
						       hi3516ev200_resets,
						       ARRAY_SIZE(hi3516ev200_resets)));
}

static const struct of_device_id hi3516ev200_crg_match_table[] = {
	{ .compatible = "hisilicon,hi3516ev200-crg" },
	{ .compatible = "hisilicon,hi3516ev300-crg" },
	{ .compatible = "hisilicon,hi3516ev200-clock" },
	{ .compatible = "hisilicon,hi3516ev300-clock" },
	{ }
};
MODULE_DEVICE_TABLE(of, hi3516ev200_crg_match_table);

static struct platform_driver hi3516ev200_crg_driver = {
	.probe = hi3516ev200_crg_probe,
	.driver = {
		.name = "hi3516ev200-crg",
		.of_match_table = hi3516ev200_crg_match_table,
		.suppress_bind_attrs = true,
	},
};
builtin_platform_driver(hi3516ev200_crg_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HiSilicon Hi3516E V200 / V300 CRG driver");
