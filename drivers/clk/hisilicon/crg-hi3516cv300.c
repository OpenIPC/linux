// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * HiSilicon Hi3516C V300 / Hi3516E V100 clock and reset generator (CRG) and
 * system controller clocks.
 *
 * The CRG layout comes from the PERI_CRGn register descriptions of the
 * Hi3516C V300/Hi3516E V100 data sheet (chapter 3.2, system control, CRG).
 * Both SoCs are described by that one data sheet, so there is a single table.
 * The SP804 timer count clock selects live in the system controller, so it is
 * a second clock provider. Both providers register early (the CRG with
 * CLK_OF_DECLARE_DRIVER, the system controller with CLK_OF_DECLARE) because
 * the SP804 clocksource is probed from time_init(), long before platform
 * drivers run (the ARM926 has no architected timer); CLK_OF_DECLARE_DRIVER()
 * does not mark the CRG node populated, so a platform device is still created
 * for it and the platform driver below binds to it and registers the reset
 * controller.
 *
 * The PLLs, the CPU, DDR and system APB selects and the timer count clock
 * selects are only read: the boot loader sets them. The fixed module clocks
 * that the data sheet prints are fixed dividers of the decoded VPLL and EPLL
 * outputs and of the crystal (hi3516cv300_fixed_factor_clks), so their rates
 * follow the registers. The reset controller only accepts the soft reset bits
 * that the data sheet lists (hi3516cv300_resets): a device tree cannot make it
 * write another register or bit of the block, and the CPU core, DDR test and PLL
 * modulator resets are not among them.
 */

#include <dt-bindings/clock/hi3516cv300-clock.h>
#include <linux/bitfield.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "clk.h"
#include "reset.h"

#define HI3516CV300_XTAL_RATE		24000000

/*
 * CRG registers: PERI_CRGn is at offset n * 4, and the three PLLs are register
 * pairs (PERI_CRG_PLL0/1 APLL, 4/5 VPLL, 8/9 EPLL).
 */
#define PERI_CRG_PLL0			0x00
#define PERI_CRG_PLL1			0x04
#define PERI_CRG_PLL4			0x10
#define PERI_CRG_PLL8			0x20
#define PERI_CRG10			0x28
#define PERI_CRG11			0x2c
#define PERI_CRG12			0x30
#define PERI_CRG13			0x34
#define PERI_CRG14			0x38
#define PERI_CRG16			0x40
#define PERI_CRG18			0x48
#define PERI_CRG23			0x5c
#define PERI_CRG24			0x60
#define PERI_CRG27			0x6c
#define PERI_CRG31			0x7c
#define PERI_CRG35			0x8c
#define PERI_CRG46			0xb8
#define PERI_CRG48			0xc0
#define PERI_CRG49			0xc4
#define PERI_CRG50			0xc8
#define PERI_CRG53			0xd4
#define PERI_CRG54			0xd8
#define PERI_CRG55			0xdc
#define PERI_CRG57			0xe4
#define PERI_CRG58			0xe8
#define PERI_CRG59			0xec
#define PERI_CRG64			0x100

/* the gates of the two boot media */
#define PERI_CRG48_FMC_CKEN		BIT(1)
#define PERI_CRG49_EMMC_CKEN		BIT(17)
#define PERI_CRG11_MIPI_CKEN		BIT(15)
#define PERI_CRG12_CPU_SC_SEL		GENMASK(5, 4)

/* system controller registers */
#define SC_CTRL				0x0
#define SC_LOCKEN			0x44

/*
 * PLL register pairs: PERI_CRG_PLL0/1 APLL, 4/5 VPLL, 8/9 EPLL. FOUTVCO =
 * Fin * (fbdiv + frac / 2^24) / refdiv, FOUTPOSTDIV = VCO / (postdiv1 *
 * postdiv2); frac only counts with dsmpd cleared, bypass passes the
 * reference through to the POSTDIV output. The data sheet calls the bit "clock
 * frequency division bypass" (it bypasses the divider stage), the VCO has its
 * own power-down bit, and the VCO / 2 tap of the CPU select is taken before
 * the dividers. So that tap is decoded without the bypass bit; the data sheet
 * does not say what it carries in bypass, this is inferred from the field
 * name and the structure of the PLL, and the probe says so when it matters.
 */
#define PLL_PD				BIT(23)
#define PLL_POSTDIVPD			BIT(21)
#define PLL_POSTDIV2			GENMASK(30, 28)
#define PLL_POSTDIV1			GENMASK(26, 24)
#define PLL_FRAC			GENMASK(23, 0)
#define PLL_BYPASS			BIT(26)
#define PLL_DSMPD			BIT(24)
#define PLL_REFDIV			GENMASK(17, 12)
#define PLL_FBDIV			GENMASK(11, 0)

/* VCO range of the data sheet (Table 3-4): above 800 MHz, up to 2.3 GHz */
#define PLL_VCO_MIN			800000000UL
#define PLL_VCO_MAX			2300000000UL

struct hi3516cv300_pll {
	struct clk_hw hw;
	void __iomem *reg;	/* first of the two registers */
	unsigned int vco_div;	/* 0: POSTDIV output, else VCO / vco_div */
};

#define to_hi3516cv300_pll(_hw)	container_of(_hw, struct hi3516cv300_pll, hw)

/*
 * VCO frequency of the PLL at @reg, 0 if it is powered down or misprogrammed.
 * FOUTVCOPD (bit 22) is not tested: it only switches off the raw VCO output
 * tap, not the post-divided clock (see the Hi3516C V200 driver, whose boot
 * loader runs the CPU from a PLL with it set).
 */
static u64 hi3516cv300_pll_vco(void __iomem *reg, unsigned long parent_rate)
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

/* POSTDIV1 * POSTDIV2 of the PLL at @reg, 0 if one of them is 0 */
static u32 hi3516cv300_pll_postdiv(void __iomem *reg)
{
	u32 a = readl(reg);

	return FIELD_GET(PLL_POSTDIV1, a) * FIELD_GET(PLL_POSTDIV2, a);
}

/* Output rate of the POSTDIV output of the PLL at @reg, 0 if it is off */
static unsigned long hi3516cv300_pll_rate(void __iomem *reg,
					  unsigned long parent_rate)
{
	u32 b = readl(reg + 4);
	u32 div = hi3516cv300_pll_postdiv(reg);

	if (b & PLL_BYPASS)
		return parent_rate;
	if ((b & PLL_POSTDIVPD) || !div)
		return 0;

	return div_u64(hi3516cv300_pll_vco(reg, parent_rate), div);
}

static unsigned long hi3516cv300_pll_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	struct hi3516cv300_pll *pll = to_hi3516cv300_pll(hw);

	if (pll->vco_div)
		return div_u64(hi3516cv300_pll_vco(pll->reg, parent_rate),
			       pll->vco_div);

	return hi3516cv300_pll_rate(pll->reg, parent_rate);
}

static const struct clk_ops hi3516cv300_pll_ops = {
	.recalc_rate = hi3516cv300_pll_recalc_rate,
};

/*
 * The APLL is registered twice: its POSTDIV output and, for code 2 of the CPU
 * select, its VCO divided by 2.
 */
static const struct {
	unsigned int id;
	const char *name;
	unsigned long offset;
	unsigned int vco_div;
} hi3516cv300_plls[] = {
	{ HI3516CV300_APLL_CLK, "apll", PERI_CRG_PLL0, 0, },
	{ HI3516CV300_VPLL_CLK, "vpll", PERI_CRG_PLL4, 0, },
	{ HI3516CV300_EPLL_CLK, "epll", PERI_CRG_PLL8, 0, },
	{ HI3516CV300_APLL_VCO2_CLK, "apll_vco2", PERI_CRG_PLL0, 2, },
};

static void hi3516cv300_unregister_plls(struct hisi_clock_data *clk_data, int n)
{
	while (n--) {
		struct clk **clk = &clk_data->clk_data.clks[hi3516cv300_plls[n].id];
		struct clk_hw *hw = __clk_get_hw(*clk);

		*clk = NULL;
		clk_hw_unregister(hw);
		kfree(to_hi3516cv300_pll(hw));
	}
}

static int __init hi3516cv300_register_plls(struct hisi_clock_data *clk_data)
{
	static const char *const parent_names[] = { "24m" };
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(hi3516cv300_plls); i++) {
		struct clk_init_data init = {
			.name = hi3516cv300_plls[i].name,
			.ops = &hi3516cv300_pll_ops,
			.parent_names = parent_names,
			.num_parents = 1,
		};
		struct hi3516cv300_pll *pll;
		struct clk *clk;

		pll = kzalloc_obj(*pll);
		if (!pll) {
			ret = -ENOMEM;
			goto err_unregister;
		}

		pll->reg = clk_data->base + hi3516cv300_plls[i].offset;
		pll->vco_div = hi3516cv300_plls[i].vco_div;
		pll->hw.init = &init;
		clk = clk_register(NULL, &pll->hw);
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			kfree(pll);
			goto err_unregister;
		}

		clk_data->clk_data.clks[hi3516cv300_plls[i].id] = clk;
	}

	return 0;

err_unregister:
	hi3516cv300_unregister_plls(clk_data, i);

	return ret;
}

/*
 * The APLL feeds the CPU and the DDR: only its VCO range, its bypass and its
 * lock are checked, its rate is read by the clock core. The VPLL is also the
 * 594 MHz source of the CPU select.
 *
 * The module clocks derived from the VPLL and the EPLL follow the rates
 * decoded here (see hi3516cv300_fixed_factor_clks), so what is checked for
 * those two PLLs is that they run, are locked and are not bypassed, that the
 * VCO is in range, and that the output is the one of the data sheet clock
 * tree (the dividers of the module clocks are those of that operating point,
 * the data sheet says the configuration of these PLLs is fixed). A powered
 * down or misprogrammed PLL makes its module clocks read 0 Hz, and that is
 * said here.
 */
#define PLL_STATUS_APLL_LOCK		BIT(0)
#define PLL_STATUS_VPLL_LOCK		BIT(1)
#define PLL_STATUS_EPLL_LOCK		BIT(2)

static void __init hi3516cv300_check_plls(struct device_node *np,
					  struct hisi_clock_data *clk_data)
{
	static const struct {
		const char *name;
		unsigned long offset;
		u32 lock;
		unsigned long out;	/* Hz of the POSTDIV output of the data sheet */
		const char *clocks;	/* module clocks derived from it */
	} feeds[] = {
		{ "VPLL", PERI_CRG_PLL4, PLL_STATUS_VPLL_LOCK, 594000000,
			"27, 37.125, 49.5, 54, 74.25, 99, 148.5, 198 and 297 MHz", },
		{ "EPLL", PERI_CRG_PLL8, PLL_STATUS_EPLL_LOCK, 500000000,
			"25, 50, 83.3, 100, 125, 166.6 and 250 MHz", },
	};
	u32 status = readl(clk_data->base + PERI_CRG58);
	u64 vco = hi3516cv300_pll_vco(clk_data->base + PERI_CRG_PLL0,
				       HI3516CV300_XTAL_RATE);
	int i;

	if (!(status & PLL_STATUS_APLL_LOCK))
		pr_warn("%pOF: APLL is not locked\n", np);
	if (!vco)
		pr_warn("%pOF: APLL is powered down or has a zero reference divider\n",
			np);
	else if (vco <= PLL_VCO_MIN || vco > PLL_VCO_MAX)
		pr_warn("%pOF: APLL VCO runs at %llu Hz, outside the range above %lu Hz up to %lu Hz of the data sheet\n",
			np, vco, PLL_VCO_MIN, PLL_VCO_MAX);
	if (readl(clk_data->base + PERI_CRG_PLL1) & PLL_BYPASS) {
		pr_warn("%pOF: APLL is bypassed, the CPU and DDR clocks derived from its POSTDIV output run at the 24 MHz reference\n",
			np);
		if (FIELD_GET(PERI_CRG12_CPU_SC_SEL,
			      readl(clk_data->base + PERI_CRG12)) == 2)
			pr_warn("%pOF: the CPU runs from the APLL VCO / 2 tap, reported as VCO / 2: the data sheet does not say what that tap carries in bypass\n",
				np);
	} else if (vco && !hi3516cv300_pll_rate(clk_data->base + PERI_CRG_PLL0,
					       HI3516CV300_XTAL_RATE))
		pr_warn("%pOF: the APLL POSTDIV output is off or misprogrammed, the CPU and DDR clocks derived from it do not run\n",
			np);

	for (i = 0; i < ARRAY_SIZE(feeds); i++) {
		void __iomem *reg = clk_data->base + feeds[i].offset;
		unsigned long out = hi3516cv300_pll_rate(reg, HI3516CV300_XTAL_RATE);

		vco = hi3516cv300_pll_vco(reg, HI3516CV300_XTAL_RATE);

		if (!(status & feeds[i].lock))
			pr_warn("%pOF: %s is not locked\n", np, feeds[i].name);
		if (readl(reg + 4) & PLL_BYPASS) {
			pr_warn("%pOF: %s is bypassed, the %s MHz clocks derived from it run at fractions of the 24 MHz reference\n",
				np, feeds[i].name, feeds[i].clocks);
			continue;
		}
		if (!out) {
			pr_warn("%pOF: %s is powered down or misprogrammed, the %s MHz clocks derived from it read 0 Hz\n",
				np, feeds[i].name, feeds[i].clocks);
			continue;
		}
		if (vco <= PLL_VCO_MIN || vco > PLL_VCO_MAX)
			pr_warn("%pOF: %s VCO runs at %llu Hz, outside the range above %lu Hz up to %lu Hz of the data sheet\n",
				np, feeds[i].name, vco, PLL_VCO_MIN, PLL_VCO_MAX);
		if (out != feeds[i].out)
			pr_warn("%pOF: %s output runs at %lu Hz, not the %lu Hz of the data sheet clock tree: the %s MHz clocks derived from it follow it and differ from the data sheet\n",
				np, feeds[i].name, out, feeds[i].out, feeds[i].clocks);
	}
}

/*
 * Rates of the module clock selects as listed by the data sheet (Table 3-2,
 * and section 12.3.2 for the 100 MHz of the SPI controllers). The only fixed
 * rate clocks are the crystal and the RMII clock input of the Ethernet pad (the
 * pad is an input, 50 MHz by the definition of RMII).
 */
static const struct hisi_fixed_rate_clock hi3516cv300_fixed_rate_clks[] = {
	{ HI3516CV300_FIXED_24M, "24m", NULL, 0, HI3516CV300_XTAL_RATE, },
	{ HI3516CV300_FIXED_RMII_PAD, "rmii_pad", NULL, 0, 50000000, },
};

/*
 * The rates the data sheet lists are modeled as fixed dividers of the clock
 * they come from, so that they follow the registers: the 3 and 6 MHz clocks
 * (the SP804 reference and the UART and PWM low rate) are dividers of the 24
 * MHz crystal, the rest are dividers of the VPLL and EPLL outputs that
 * hi3516cv300_pll_rate() decodes. The data sheet prints the rates but not the
 * divider each one comes from, so the ratios are those of the printed rates
 * to the POSTDIV outputs at the operating point of the data sheet clock tree
 * (VPLL 594 MHz, VCO 1188 MHz; EPLL 500 MHz, VCO 1500 MHz). Each rate has
 * exactly one integer divider of one of the two outputs, so the assignment is
 * unambiguous; that the POSTDIV output and not the VCO is the tap is inferred
 * (594 MHz is also the VPLL output that the CPU select offers). The 83.3 and
 * 166.6 MHz that the data sheet prints are 500 / 6 and 500 / 3 MHz rounded:
 * the clocks read 83.333 and 166.667 MHz.
 */
static const struct hisi_fixed_factor_clock hi3516cv300_fixed_factor_clks[] = {
	{ HI3516CV300_FIXED_3M, "3m", "24m", 1, 8, 0, },
	{ HI3516CV300_FIXED_6M, "6m", "24m", 1, 4, 0, },
	{ HI3516CV300_FIXED_25M, "25m", "epll", 1, 20, 0, },
	{ HI3516CV300_FIXED_50M, "50m", "epll", 1, 10, 0, },
	{ HI3516CV300_FIXED_83P3M, "83.3m", "epll", 1, 6, 0, },
	{ HI3516CV300_FIXED_100M, "100m", "epll", 1, 5, 0, },
	{ HI3516CV300_FIXED_125M, "125m", "epll", 1, 4, 0, },
	{ HI3516CV300_FIXED_166P6M, "166.6m", "epll", 1, 3, 0, },
	{ HI3516CV300_FIXED_250M, "250m", "epll", 1, 2, 0, },
	{ HI3516CV300_FIXED_27M, "27m", "vpll", 1, 22, 0, },
	{ HI3516CV300_FIXED_37P125M, "37.125m", "vpll", 1, 16, 0, },
	{ HI3516CV300_FIXED_49P5M, "49.5m", "vpll", 1, 12, 0, },
	{ HI3516CV300_FIXED_54M, "54m", "vpll", 1, 11, 0, },
	{ HI3516CV300_FIXED_74P25M, "74.25m", "vpll", 1, 8, 0, },
	{ HI3516CV300_FIXED_99M, "99m", "vpll", 1, 6, 0, },
	{ HI3516CV300_FIXED_148P5M, "148.5m", "vpll", 1, 4, 0, },
	{ HI3516CV300_FIXED_198M, "198m", "vpll", 1, 3, 0, },
	{ HI3516CV300_FIXED_297M, "297m", "vpll", 1, 2, 0, },
};

/*
 * PERI_CRG12: cpu_sc_sel [5:4], ddr_sc_sel [3:2], ddr_clk_div [9:8] and
 * apb_sc_sel [0]. All are the boot loader's choice and read only here: a
 * switch needs the PLL lock and the handshake of PERI_CRG55 (cpu_sc_seled,
 * ddr_sc_seled and apb_sc_seled, which the init code compares with the
 * selects), and the whole chip hangs off the APB select. The reset value of
 * all of them is the 24 MHz crystal.
 */
static const char *const cpu_mux_p[] = { "24m", "apll", "apll_vco2", "vpll" };
static const char *const ddr_mux_p[] = { "24m", "ddr_div", "250m", "198m" };
static const char *const apb_mux_p[] = { "24m", "50m" };

static const struct hisi_divider_clock hi3516cv300_core_div_clks[] = {
	{ HI3516CV300_DDR_DIV, "ddr_div", "apll", 0, PERI_CRG12, 8, 2,
		CLK_DIVIDER_READ_ONLY, NULL, },
};

static const struct hisi_mux_clock hi3516cv300_core_mux_clks[] = {
	{ HI3516CV300_APB_CLK, "apb", apb_mux_p, ARRAY_SIZE(apb_mux_p),
		0, PERI_CRG12, 0, 1, CLK_MUX_READ_ONLY, NULL, },
	{ HI3516CV300_CPU_MUX, "cpu_mux", cpu_mux_p, ARRAY_SIZE(cpu_mux_p),
		0, PERI_CRG12, 4, 2, CLK_MUX_READ_ONLY, NULL, },
	{ HI3516CV300_DDR_MUX, "ddr_mux", ddr_mux_p, ARRAY_SIZE(ddr_mux_p),
		0, PERI_CRG12, 2, 2, CLK_MUX_READ_ONLY, NULL, },
};

/* PERI_CRG57 uart_clksel [19]: 24 or 6 MHz (no APB option on this chip) */
static const char *const uart_mux_p[] = { "24m", "6m" };
/* PERI_CRG48 fmc_clksel [4:2]: 5 and 7 are reserved, 6 is an alias of 4 */
static const char *const fmc_mux_p[] = {
	"24m", "83.3m", "148.5m", "198m", "297m", "297m"
};

static u32 fmc_mux_table[] = { 0, 1, 2, 3, 4, 6 };

/*
 * PERI_CRG49 sdio0_clksel [5:4], sdio1_clksel [13:12], emmc_clksel [21:20] and
 * PERI_CRG50 sdio2_clksel [5:4]: 49.5 MHz (code 0) only, except the eMMC,
 * which also has 99 MHz (code 0) and 49.5 MHz (code 2)
 */
static const char *const sdio_mux_p[] = { "49.5m" };
static const char *const emmc_mux_p[] = { "99m", "49.5m" };
static u32 emmc_mux_table[] = { 0, 2 };

/* PERI_CRG14 pwm_clksel [3:2]: 3 MHz, 50 MHz, 24 MHz (3 is an alias of 2) */
static const char *const pwm_mux_p[] = { "3m", "50m", "24m", "24m" };
/* PERI_CRG11 sensor_clksel [25:23]: 1X0 is 24 MHz, 1X1 is 25 MHz */
static const char *const sensor_mux_p[] = {
	"74.25m", "37.125m", "54m", "27m", "24m", "25m", "24m", "25m"
};

/* PERI_CRG11 vi0_sc_sel [4:2]: 5 to 7 are reserved */
static const char *const viu_mux_p[] = {
	"83.3m", "125m", "148.5m", "198m", "250m"
};

/* PERI_CRG16 vedu_clksel [11:10], PERI_CRG18 vps_clksel [11:10] */
static const char *const vedu_mux_p[] = { "166.6m", "198m" };
static const char *const vpss_mux_p[] = { "148.5m", "198m", "250m" };
/* PERI_CRG23 vgs_clksel [11:10], PERI_CRG27 ive_clksel [3:2] */
static const char *const vgs_mux_p[] = { "198m", "250m", "297m" };
static const char *const ive_mux_p[] = { "198m", "250m", "297m" };
/* PERI_CRG31 cipher_clksel [2] */
static const char *const cipher_mux_p[] = { "198m", "148.5m" };
/*
 * PERI_CRG59 ethcore_clksel [7], ethrmii_clksel [2], ethphy_clksel [5:4]. The
 * core select is 99 MHz after reset, but a boot loader that used the network
 * may have switched it to 54 MHz; both work, so it is not forced. The clock
 * core reads the select back and nothing is written unless a device tree
 * asks for it (assigned-clock-parents).
 */
static const char *const eth_core_mux_p[] = { "99m", "54m" };
static const char *const eth_rmii_mux_p[] = { "50m", "rmii_pad" };
static const char *const eth_phy_mux_p[] = { "25m", "24m", "27m", "27m" };

static u32 sdio_mux_table[] = { 0 };
static u32 viu_mux_table[] = { 0, 1, 2, 3, 4 };
static u32 vedu_mux_table[] = { 0, 1 };
static u32 vpss_mux_table[] = { 0, 1, 2 };
static u32 vgs_mux_table[] = { 0, 1, 2 };
static u32 ive_mux_table[] = { 0, 1, 2 };

static const struct hisi_mux_clock hi3516cv300_mux_clks[] = {
	{ HI3516CV300_UART_MUX, "uart_mux", uart_mux_p, ARRAY_SIZE(uart_mux_p),
		0, PERI_CRG57, 19, 1, 0, NULL, },
	{ HI3516CV300_FMC_MUX, "fmc_mux", fmc_mux_p, ARRAY_SIZE(fmc_mux_p),
		0, PERI_CRG48, 2, 3, 0, fmc_mux_table, },
	{ HI3516CV300_MMC0_MUX, "mmc0_mux", sdio_mux_p, ARRAY_SIZE(sdio_mux_p),
		0, PERI_CRG49, 4, 2, 0, sdio_mux_table, },
	{ HI3516CV300_MMC1_MUX, "mmc1_mux", sdio_mux_p, ARRAY_SIZE(sdio_mux_p),
		0, PERI_CRG49, 12, 2, 0, sdio_mux_table, },
	{ HI3516CV300_MMC2_MUX, "mmc2_mux", emmc_mux_p, ARRAY_SIZE(emmc_mux_p),
		0, PERI_CRG49, 20, 2, 0, emmc_mux_table, },
	{ HI3516CV300_MMC3_MUX, "mmc3_mux", sdio_mux_p, ARRAY_SIZE(sdio_mux_p),
		0, PERI_CRG50, 4, 2, 0, sdio_mux_table, },
	{ HI3516CV300_PWM_MUX, "pwm_mux", pwm_mux_p, ARRAY_SIZE(pwm_mux_p),
		0, PERI_CRG14, 2, 2, 0, NULL, },
	{ HI3516CV300_SENSOR_MUX, "sensor_mux", sensor_mux_p,
		ARRAY_SIZE(sensor_mux_p), 0, PERI_CRG11, 23, 3, 0, NULL, },
	{ HI3516CV300_VIU_MUX, "viu_mux", viu_mux_p, ARRAY_SIZE(viu_mux_p),
		0, PERI_CRG11, 2, 3, 0, viu_mux_table, },
	{ HI3516CV300_VEDU_MUX, "vedu_mux", vedu_mux_p, ARRAY_SIZE(vedu_mux_p),
		0, PERI_CRG16, 10, 2, 0, vedu_mux_table, },
	{ HI3516CV300_VPSS_MUX, "vpss_mux", vpss_mux_p, ARRAY_SIZE(vpss_mux_p),
		0, PERI_CRG18, 10, 2, 0, vpss_mux_table, },
	{ HI3516CV300_VGS_MUX, "vgs_mux", vgs_mux_p, ARRAY_SIZE(vgs_mux_p),
		0, PERI_CRG23, 10, 2, 0, vgs_mux_table, },
	{ HI3516CV300_IVE_MUX, "ive_mux", ive_mux_p, ARRAY_SIZE(ive_mux_p),
		0, PERI_CRG27, 2, 2, 0, ive_mux_table, },
	{ HI3516CV300_CIPHER_MUX, "cipher_mux", cipher_mux_p,
		ARRAY_SIZE(cipher_mux_p), 0, PERI_CRG31, 2, 1, 0, NULL, },
	{ HI3516CV300_ETH_CORE_MUX, "eth_core_mux", eth_core_mux_p,
		ARRAY_SIZE(eth_core_mux_p), 0, PERI_CRG59, 7, 1, 0, NULL, },
	{ HI3516CV300_ETH_RMII_MUX, "eth_rmii_mux", eth_rmii_mux_p,
		ARRAY_SIZE(eth_rmii_mux_p), 0, PERI_CRG59, 2, 1, 0, NULL, },
	{ HI3516CV300_ETH_PHY_MUX, "eth_phy_mux", eth_phy_mux_p,
		ARRAY_SIZE(eth_phy_mux_p), 0, PERI_CRG59, 4, 2, 0, NULL, },
};

/* PERI_CRG11 isp_clksel [17]: the VI working clock or its half */
static const struct hisi_divider_clock hi3516cv300_div_clks[] = {
	{ HI3516CV300_ISP_DIV, "isp_div", "viu_mux", 0, PERI_CRG11, 17, 1, 0,
		NULL, },
};

/*
 * Clock gates (set = enabled). Blocks whose input rate the data sheet does not
 * state have no parent and report 0 Hz. The I2C blocks have no gate (they run
 * from the APB clock), and neither has the LSADC. The soft reset bits of the
 * same registers belong to the reset controller. The gates behind a module
 * clock select pass a rate request on to the select, so that clk_set_rate() of
 * a consumer (the SPI NOR driver of the FMC, an MMC host) picks the closest
 * input. A gate nobody uses is switched off by the clock core after boot,
 * except the gates of the two boot media, FMC and eMMC: the data sheet says
 * the chip boots from either (BOOT_SEL0 and BOOT_SEL1, section 1.4) and that a
 * global soft reset leaves the clock circuits alone (Table 3-1), and it does
 * not say that the boot ROM switches the gate of its boot medium on again. The
 * medium the machine booted from is behind the gate and nothing in the device
 * tree claims the gate yet, so the gate is left as the boot loader had it
 * instead of being switched off behind the medium. This is a precaution, not a
 * measured need: the boards this was run on boot from SPI NOR. The SD card
 * (SDIO0), USB and UART download paths of the boot ROM are update modes and
 * not boot media, so their gates are not protected. The MIPI gate is kept for
 * another reason, see clk_mipi below.
 */
static const struct hisi_gate_clock hi3516cv300_gate_clks[] = {
	/* PERI_CRG57 */
	{ HI3516CV300_UART0_CLK, "clk_uart0", "uart_mux", 0, PERI_CRG57, 15, 0, },
	{ HI3516CV300_UART1_CLK, "clk_uart1", "uart_mux", 0, PERI_CRG57, 16, 0, },
	{ HI3516CV300_UART2_CLK, "clk_uart2", "uart_mux", 0, PERI_CRG57, 17, 0, },
	/* the SPI controllers run from 100 MHz (data sheet 12.3.2) */
	{ HI3516CV300_SPI0_CLK, "clk_spi0", "100m", 0, PERI_CRG57, 13, 0, },
	{ HI3516CV300_SPI1_CLK, "clk_spi1", "100m", 0, PERI_CRG57, 14, 0, },
	{ HI3516CV300_IR_CLK, "clk_ir", NULL, 0, PERI_CRG57, 12, 0, },
	/*
	 * PERI_CRG48, PERI_CRG49, PERI_CRG50, PERI_CRG59. FMC and eMMC (MMC2)
	 * are the boot media, see above.
	 */
	{ HI3516CV300_FMC_CLK, "clk_fmc", "fmc_mux",
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, PERI_CRG48, 1, 0, },
	{ HI3516CV300_MMC0_CLK, "clk_mmc0", "mmc0_mux", CLK_SET_RATE_PARENT, PERI_CRG49, 1, 0, },
	{ HI3516CV300_MMC1_CLK, "clk_mmc1", "mmc1_mux", CLK_SET_RATE_PARENT, PERI_CRG49, 9, 0, },
	{ HI3516CV300_MMC2_CLK, "clk_mmc2", "mmc2_mux",
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, PERI_CRG49, 17, 0, },
	{ HI3516CV300_MMC3_CLK, "clk_mmc3", "mmc3_mux", CLK_SET_RATE_PARENT, PERI_CRG50, 1, 0, },
	{ HI3516CV300_ETH_CLK, "clk_eth", "eth_core_mux", CLK_SET_RATE_PARENT, PERI_CRG59, 1, 0, },
	/*
	 * PERI_CRG46: bus, OHCI 48 and 12 MHz, OTG UTMI, host PHY, UTMI0 and
	 * PHY reference clock gates; the USB resets are bits 8, 9, 11 to 14
	 * and 17 of the same register
	 */
	{ HI3516CV300_USB2_BUS_CLK, "clk_usb2_bus", NULL, 0, PERI_CRG46, 0, 0, },
	{ HI3516CV300_USB2_OHCI48M_CLK, "clk_usb2_ohci48m", NULL, 0, PERI_CRG46, 1, 0, },
	{ HI3516CV300_USB2_OHCI12M_CLK, "clk_usb2_ohci12m", NULL, 0, PERI_CRG46, 2, 0, },
	{ HI3516CV300_USB2_OTG_UTMI_CLK, "clk_usb2_otg_utmi", NULL, 0, PERI_CRG46, 3, 0, },
	{ HI3516CV300_USB2_HST_PHY_CLK, "clk_usb2_hst_phy", NULL, 0, PERI_CRG46, 4, 0, },
	{ HI3516CV300_UTMI0_CLK, "clk_usb2_utmi0", NULL, 0, PERI_CRG46, 5, 0, },
	{ HI3516CV300_USB2_PHY_CLK, "clk_usb2_phy", "24m", 0, PERI_CRG46, 7, 0, },
	/* PERI_CRG54 dmac_cken, PERI_CRG14 pwm_cken */
	{ HI3516CV300_DMAC_CLK, "clk_dmac", NULL, 0, PERI_CRG54, 5, 0, },
	{ HI3516CV300_PWM_CLK, "clk_pwm", "pwm_mux", CLK_SET_RATE_PARENT, PERI_CRG14, 1, 0, },
	/*
	 * PERI_CRG11 sensor_cken [26], vi0_cken [0], mipi_cken [15], isp_cken
	 * [18]. The vendor media modules set the rate of the VI, ISP, VEDU,
	 * VPSS, VGS and IVE gates (clk_set_rate() of the VI clock, then of the
	 * ISP clock) and the OpenIPC module loader passes the rates as module
	 * parameters, so those gates pass a request on to their select. The
	 * ISP clock reaches isp_div, which only picks the VI rate or half of
	 * it: the VI clock is set first.
	 *
	 * Nothing claims the MIPI gate: the vendor mipi_rx module only toggles
	 * the PERI_CRG11 reset bits and never enables mipi_cken. The clock
	 * core would switch it off after boot if the boot loader left it on
	 * (the stock U-Boot of the Hi3516C V300 cameras leaves PERI_CRG11 at
	 * 0), and a MIPI or LVDS sensor would then have no RX clock, so it is
	 * left as it is.
	 */
	{ HI3516CV300_SENSOR_CLK, "clk_sensor", "sensor_mux",
		CLK_SET_RATE_PARENT, PERI_CRG11, 26, 0, },
	{ HI3516CV300_MIPI_CLK, "clk_mipi", NULL, CLK_IGNORE_UNUSED,
		PERI_CRG11, 15, 0, },
	{ HI3516CV300_ISP_CLK, "clk_isp", "isp_div", CLK_SET_RATE_PARENT,
		PERI_CRG11, 18, 0, },
	{ HI3516CV300_VIU_CLK, "clk_viu", "viu_mux", CLK_SET_RATE_PARENT,
		PERI_CRG11, 0, 0, },
	/* media blocks: cken is bit 1 of PERI_CRG16/18/23/24/27/35 */
	{ HI3516CV300_VEDU_CLK, "clk_vedu", "vedu_mux", CLK_SET_RATE_PARENT,
		PERI_CRG16, 1, 0, },
	{ HI3516CV300_VPSS_CLK, "clk_vpss", "vpss_mux", CLK_SET_RATE_PARENT,
		PERI_CRG18, 1, 0, },
	{ HI3516CV300_VGS_CLK, "clk_vgs", "vgs_mux", CLK_SET_RATE_PARENT,
		PERI_CRG23, 1, 0, },
	{ HI3516CV300_JPGE_CLK, "clk_jpge", "198m", 0, PERI_CRG24, 1, 0, },
	{ HI3516CV300_IVE_CLK, "clk_ive", "ive_mux", CLK_SET_RATE_PARENT,
		PERI_CRG27, 1, 0, },
	{ HI3516CV300_AIAO_CLK, "clk_aiao", NULL, 0, PERI_CRG35, 1, 0, },
	/*
	 * Crypto engines: PERI_CRG31 cipher_cken, PERI_CRG27 hash_cken,
	 * PERI_CRG64 trng, rsa and klad cken. The data sheet gives no rate for
	 * the hash, TRNG, RSA and key ladder clocks.
	 */
	{ HI3516CV300_CIPHER_CLK, "clk_cipher", "cipher_mux",
		CLK_SET_RATE_PARENT, PERI_CRG31, 1, 0, },
	{ HI3516CV300_HASH_CLK, "clk_hash", NULL, 0, PERI_CRG27, 5, 0, },
	{ HI3516CV300_TRNG_CLK, "clk_trng", NULL, 0, PERI_CRG64, 3, 0, },
	{ HI3516CV300_RSA_CLK, "clk_rsa", NULL, 0, PERI_CRG64, 5, 0, },
	{ HI3516CV300_KLAD_CLK, "clk_klad", NULL, 0, PERI_CRG64, 1, 0, },
};

/*
 * Selects that have reserved codes: the boot loader may leave one in a state
 * the tables cannot describe. Say so instead of handing out a clock with no
 * parent. @valid has a bit set for every code the data sheet defines.
 */
static const struct {
	const char *name;
	unsigned long offset;
	u8 shift;
	u8 mask;
	u32 valid;
} hi3516cv300_reserved_selects[] = {
	{ "FMC clock", PERI_CRG48, 2, 0x7, 0x5f, },
	{ "SDIO0 clock", PERI_CRG49, 4, 0x3, 0x1, },
	{ "SDIO0 clock mode", PERI_CRG49, 7, 0x1, 0x1, },
	{ "SDIO1 clock", PERI_CRG49, 12, 0x3, 0x1, },
	{ "SDIO1 clock mode", PERI_CRG49, 15, 0x1, 0x1, },
	{ "eMMC clock", PERI_CRG49, 20, 0x3, 0x5, },
	{ "eMMC clock mode", PERI_CRG49, 23, 0x1, 0x1, },
	{ "SDIO2 clock", PERI_CRG50, 4, 0x3, 0x1, },
	{ "SDIO2 clock mode", PERI_CRG50, 7, 0x1, 0x1, },
	{ "VI clock", PERI_CRG11, 2, 0x7, 0x1f, },
	{ "VEDU clock", PERI_CRG16, 10, 0x3, 0x3, },
	{ "VPSS clock", PERI_CRG18, 10, 0x3, 0x7, },
	{ "VGS clock", PERI_CRG23, 10, 0x3, 0x7, },
	{ "IVE clock", PERI_CRG27, 2, 0x3, 0x7, },
};

/*
 * PERI_CRG12 holds the CPU, DDR and APB selects and PERI_CRG55 reports which
 * clock each of them has switched to; a difference means a switch did not
 * complete. PERI_CRG10 cpuclk_skipcfg [16:12] drops clock beats of the CPU
 * (it runs at 32 - N out of 32); the CPU rate the clock core reports ignores
 * it, so a non-zero value is worth a message.
 */
#define PERI_CRG10_CPUCLK_SKIPCFG	GENMASK(16, 12)
#define PERI_CRG12_DDR_SC_SEL		GENMASK(3, 2)
#define PERI_CRG12_APB_SC_SEL		BIT(0)

static void __init hi3516cv300_check_selects(struct device_node *np,
					     struct hisi_clock_data *clk_data)
{
	u32 sel = readl(clk_data->base + PERI_CRG12);
	u32 seled = readl(clk_data->base + PERI_CRG55);
	u32 skip = readl(clk_data->base + PERI_CRG10);
	u32 fmc = readl(clk_data->base + PERI_CRG48);
	u32 emmc = readl(clk_data->base + PERI_CRG49);
	u32 mipi = readl(clk_data->base + PERI_CRG11);
	int i;

	/* the boot media gates are never switched off as unused: say how they were left */
	pr_info("%pOF: the clock gates of the boot media and of the MIPI RX are not switched off as unused (FMC %s, eMMC %s, MIPI %s)\n",
		np, fmc & PERI_CRG48_FMC_CKEN ? "on" : "off",
		emmc & PERI_CRG49_EMMC_CKEN ? "on" : "off",
		mipi & PERI_CRG11_MIPI_CKEN ? "on" : "off");

	if (FIELD_GET(PERI_CRG12_CPU_SC_SEL, sel) !=
	    FIELD_GET(PERI_CRG12_CPU_SC_SEL, seled))
		pr_warn("%pOF: CPU clock select is %lu but the CRG reports %lu\n",
			np, FIELD_GET(PERI_CRG12_CPU_SC_SEL, sel),
			FIELD_GET(PERI_CRG12_CPU_SC_SEL, seled));
	if (FIELD_GET(PERI_CRG12_DDR_SC_SEL, sel) !=
	    FIELD_GET(PERI_CRG12_DDR_SC_SEL, seled))
		pr_warn("%pOF: DDR clock select is %lu but the CRG reports %lu\n",
			np, FIELD_GET(PERI_CRG12_DDR_SC_SEL, sel),
			FIELD_GET(PERI_CRG12_DDR_SC_SEL, seled));
	if ((sel & PERI_CRG12_APB_SC_SEL) != (seled & PERI_CRG12_APB_SC_SEL))
		pr_warn("%pOF: APB clock select is %u but the CRG reports %u\n",
			np, !!(sel & PERI_CRG12_APB_SC_SEL),
			!!(seled & PERI_CRG12_APB_SC_SEL));
	if (FIELD_GET(PERI_CRG10_CPUCLK_SKIPCFG, skip))
		pr_warn("%pOF: the CPU clock skips %lu of 32 beats (PERI_CRG10 %#x), the CPU rate is lower than reported\n",
			np, FIELD_GET(PERI_CRG10_CPUCLK_SKIPCFG, skip), skip);

	for (i = 0; i < ARRAY_SIZE(hi3516cv300_reserved_selects); i++) {
		u32 code = (readl(clk_data->base +
				  hi3516cv300_reserved_selects[i].offset) >>
			    hi3516cv300_reserved_selects[i].shift) &
			   hi3516cv300_reserved_selects[i].mask;

		if (!(hi3516cv300_reserved_selects[i].valid & BIT(code)))
			pr_warn("%pOF: %s select holds the reserved code %u\n",
				np, hi3516cv300_reserved_selects[i].name, code);
	}
}

/*
 * A clock id that no table registers must not give the caller a silent dummy
 * clock (a NULL clk is a valid no-op clock to the framework): hand out an error.
 */
static void __init hi3516cv300_fill_holes(struct hisi_clock_data *clk_data,
					  int n)
{
	int i;

	for (i = 0; i < n; i++)
		if (!clk_data->clk_data.clks[i])
			clk_data->clk_data.clks[i] = ERR_PTR(-ENOENT);
}

static void __init hi3516cv300_clk_free(struct device_node *np,
					struct hisi_clock_data *clk_data)
{
	of_clk_del_provider(np);
	iounmap(clk_data->base);
	kfree(clk_data->clk_data.clks);
	kfree(clk_data);
}

static void __init hi3516cv300_crg_clk_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;

	clk_data = hisi_clk_init(np, HI3516CV300_CRG_NR_CLKS);
	if (!clk_data)
		return;

	/* each helper cleans up after itself when its own group fails */
	if (hisi_clk_register_fixed_rate(hi3516cv300_fixed_rate_clks,
					 ARRAY_SIZE(hi3516cv300_fixed_rate_clks), clk_data))
		goto err_provider;
	if (hi3516cv300_register_plls(clk_data))
		goto err_fixed_rate;
	if (hisi_clk_register_fixed_factor(hi3516cv300_fixed_factor_clks,
					   ARRAY_SIZE(hi3516cv300_fixed_factor_clks),
					   clk_data))
		goto err_plls;
	if (hisi_clk_register_divider(hi3516cv300_core_div_clks,
				      ARRAY_SIZE(hi3516cv300_core_div_clks), clk_data))
		goto err_fixed_factor;
	if (hisi_clk_register_mux(hi3516cv300_core_mux_clks,
				  ARRAY_SIZE(hi3516cv300_core_mux_clks), clk_data))
		goto err_core_div;
	if (hisi_clk_register_mux(hi3516cv300_mux_clks,
				  ARRAY_SIZE(hi3516cv300_mux_clks), clk_data))
		goto err_core_mux;
	if (hisi_clk_register_divider(hi3516cv300_div_clks,
				      ARRAY_SIZE(hi3516cv300_div_clks), clk_data))
		goto err_mux;
	if (hisi_clk_register_gate(hi3516cv300_gate_clks,
				   ARRAY_SIZE(hi3516cv300_gate_clks), clk_data))
		goto err_div;

	hi3516cv300_fill_holes(clk_data, HI3516CV300_CRG_NR_CLKS);
	hi3516cv300_check_plls(np, clk_data);
	hi3516cv300_check_selects(np, clk_data);

	return;

err_div:
	hisi_clk_unregister_divider(hi3516cv300_div_clks,
				    ARRAY_SIZE(hi3516cv300_div_clks), clk_data);
err_mux:
	hisi_clk_unregister_mux(hi3516cv300_mux_clks,
				ARRAY_SIZE(hi3516cv300_mux_clks), clk_data);
err_core_mux:
	hisi_clk_unregister_mux(hi3516cv300_core_mux_clks,
				ARRAY_SIZE(hi3516cv300_core_mux_clks), clk_data);
err_core_div:
	hisi_clk_unregister_divider(hi3516cv300_core_div_clks,
				    ARRAY_SIZE(hi3516cv300_core_div_clks), clk_data);
err_fixed_factor:
	hisi_clk_unregister_fixed_factor(hi3516cv300_fixed_factor_clks,
					 ARRAY_SIZE(hi3516cv300_fixed_factor_clks),
					 clk_data);
err_plls:
	hi3516cv300_unregister_plls(clk_data, ARRAY_SIZE(hi3516cv300_plls));
err_fixed_rate:
	hisi_clk_unregister_fixed_rate(hi3516cv300_fixed_rate_clks,
				       ARRAY_SIZE(hi3516cv300_fixed_rate_clks), clk_data);
err_provider:
	pr_err("%pOF: failed to register all clocks\n", np);
	/* no provider at all is better than one with missing clocks */
	hi3516cv300_clk_free(np, clk_data);
}

CLK_OF_DECLARE_DRIVER(hi3516cv300_crg, "hisilicon,hi3516cv300-crg", hi3516cv300_crg_clk_init);

/* ---------------- system controller block (sysctrl@12020000) ---------------- */

/*
 * The SP804 count clocks are selected by SC_CTRL (sysctrl + 0x0): bit 16 (timer
 * 0), 18 (timer 1), 20 (timer 2) and 22 (timer 3) choose between the 3 MHz
 * reference (0) and the system APB clock (1, "bus (50 MHz) clock" in the data
 * sheet). The boot loader may leave either value, so model the select as a
 * read-only mux (SC_CTRL is write-protected by SC_LOCKEN) and let the
 * read-back of the register decide the rate. Timer init runs at start_kernel,
 * long before platform driver probing, so the mux is registered from the
 * sysctrl early init; its parents come from the CRG early init, which comes
 * first in the device tree.
 */
static const char *const hi3516cv300_timer_mux_p[] = { "3m", "apb" };
static u32 hi3516cv300_timer_mux_table[] = { 0, 1 };

static const struct hisi_mux_clock hi3516cv300_sysctrl_mux[] = {
	{ HI3516CV300_TIME00_CLK, "timer00", hi3516cv300_timer_mux_p,
		ARRAY_SIZE(hi3516cv300_timer_mux_p), 0, SC_CTRL, 16, 1,
		CLK_MUX_READ_ONLY,
		hi3516cv300_timer_mux_table, },
	{ HI3516CV300_TIME01_CLK, "timer01", hi3516cv300_timer_mux_p,
		ARRAY_SIZE(hi3516cv300_timer_mux_p), 0, SC_CTRL, 18, 1,
		CLK_MUX_READ_ONLY,
		hi3516cv300_timer_mux_table, },
	{ HI3516CV300_TIME10_CLK, "timer10", hi3516cv300_timer_mux_p,
		ARRAY_SIZE(hi3516cv300_timer_mux_p), 0, SC_CTRL, 20, 1,
		CLK_MUX_READ_ONLY,
		hi3516cv300_timer_mux_table, },
	{ HI3516CV300_TIME11_CLK, "timer11", hi3516cv300_timer_mux_p,
		ARRAY_SIZE(hi3516cv300_timer_mux_p), 0, SC_CTRL, 22, 1,
		CLK_MUX_READ_ONLY,
		hi3516cv300_timer_mux_table, },
};

/*
 * SC_CTRL bit 23 is the SP805 watchdog count clock select in the vendor
 * driver, but the bits [31:23] are reserved in the data sheet, so no clock is
 * registered for it and it is never written. Say so if the boot loader set it.
 */
#define SC_CTRL_WDT_SEL			BIT(23)

/*
 * SC_LOCKEN write protects SC_CTRL and SC_SYSRES: it reads 0 when they can be
 * written and 1 when they are locked (data sheet 3.5, SC_LOCKEN). The device
 * tree reboots through SC_SYSRES, so a locked register makes a reboot do
 * nothing. The driver never writes the unlock key.
 */

static void __init hi3516cv300_sysctrl_clk_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;
	u32 ctrl, lock;

	clk_data = hisi_clk_init(np, HI3516CV300_SYS_NR_CLKS);
	if (!clk_data)
		return;

	if (hisi_clk_register_mux(hi3516cv300_sysctrl_mux,
				  ARRAY_SIZE(hi3516cv300_sysctrl_mux), clk_data)) {
		pr_err("%pOF: failed to register the timer clocks\n", np);
		/* no provider at all is better than one with missing timer clocks */
		hi3516cv300_clk_free(np, clk_data);
		return;
	}

	hi3516cv300_fill_holes(clk_data, HI3516CV300_SYS_NR_CLKS);

	lock = readl(clk_data->base + SC_LOCKEN);
	if (lock)
		pr_warn("%pOF: SC_LOCKEN reads %#x: SC_CTRL and SC_SYSRES are write protected, so a reboot through SC_SYSRES will not work\n",
			np, lock);

	ctrl = readl(clk_data->base + SC_CTRL);
	if (ctrl & SC_CTRL_WDT_SEL)
		pr_warn("%pOF: SC_CTRL bit 23 is set (%#x): it is reserved in the data sheet, the watchdog count clock is not modeled\n",
			np, ctrl);
}

CLK_OF_DECLARE(hi3516cv300_sysctrl, "hisilicon,hi3516cv300-sysctrl",
	       hi3516cv300_sysctrl_clk_init);
/* old name of the same compatible, still used by existing device trees */
CLK_OF_DECLARE(hi3516cv300_sys, "hisilicon,hi3516cv300-sys",
	       hi3516cv300_sysctrl_clk_init);

/* ---------------- platform driver (reset controller) ---------------- */

/*
 * The soft reset bits of the CRG by register, from the PERI_CRGn descriptions
 * of the data sheet. A reset specifier of a device tree is only accepted when
 * it names one of them. Left out on purpose: the ARM core reset (PERI_CRG10
 * bit 5), because resetting the CPU a kernel runs on never returns, the DDR
 * test reset (PERI_CRG54 bit 2), because the memory controller must not be
 * touched from the kernel, and the spread spectrum modulator resets of the
 * three PLLs (PERI_CRG66, 68 and 70), which are not module resets.
 */
static const struct hisi_reset_bits hi3516cv300_resets[] = {
	{ PERI_CRG11, GENMASK(11, 8) | BIT(13) | BIT(14) | BIT(16) | BIT(27) },
	{ PERI_CRG13, BIT(0) },
	{ PERI_CRG14, BIT(0) },
	{ PERI_CRG16, BIT(0) },
	{ PERI_CRG18, BIT(0) },
	{ PERI_CRG23, BIT(0) },
	{ PERI_CRG24, BIT(0) },
	{ PERI_CRG27, BIT(0) | BIT(4) },
	{ PERI_CRG31, BIT(0) | BIT(3) },
	{ PERI_CRG35, BIT(0) },
	{ PERI_CRG46, GENMASK(9, 8) | GENMASK(14, 11) | BIT(17) },
	{ PERI_CRG48, BIT(0) },
	{ PERI_CRG49, BIT(0) | BIT(8) | BIT(16) },
	{ PERI_CRG50, BIT(0) },
	{ PERI_CRG53, BIT(7) },
	{ PERI_CRG54, BIT(4) },
	{ PERI_CRG57, BIT(2) | GENMASK(9, 4) | BIT(23) | BIT(25) | BIT(28) },
	{ PERI_CRG59, BIT(0) | BIT(3) },
	{ PERI_CRG64, BIT(0) | BIT(2) | BIT(4) },
};

static int hi3516cv300_crg_probe(struct platform_device *pdev)
{
	/* the gates in this register block share it with the reset bits */
	return PTR_ERR_OR_ZERO(hisi_reset_init_checked(pdev, hisi_clk_get_lock(),
						       hi3516cv300_resets,
						       ARRAY_SIZE(hi3516cv300_resets)));
}

static const struct of_device_id hi3516cv300_crg_match_table[] = {
	{ .compatible = "hisilicon,hi3516cv300-crg" },
	{ }
};
MODULE_DEVICE_TABLE(of, hi3516cv300_crg_match_table);

static struct platform_driver hi3516cv300_crg_driver = {
	.probe = hi3516cv300_crg_probe,
	.driver = {
		.name = "hi3516cv300-crg",
		.of_match_table = hi3516cv300_crg_match_table,
		.suppress_bind_attrs = true,
	},
};
builtin_platform_driver(hi3516cv300_crg_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HiSilicon Hi3516C V300 / Hi3516E V100 CRG driver");
