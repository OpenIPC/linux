// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * HiSilicon Hi3536D V100 clock and reset generator (CRG) and system
 * controller clocks.
 *
 * The CRG layout comes from the PERI_CRGn register descriptions of the
 * Hi3536D V100 data sheet (chapter 3, system, clock). The SP804 timer count
 * clock selects live in the system controller, so it is a second clock
 * provider. Both providers register early (the CRG with
 * CLK_OF_DECLARE_DRIVER, the system controller with CLK_OF_DECLARE) because
 * the SP804 clocksource is probed from time_init(), long before platform
 * drivers run: the Cortex-A7 generic timer of this SoC is not documented, so
 * the SP804 is the only tick source. CLK_OF_DECLARE_DRIVER() does not mark the
 * CRG node populated, so a platform device is still created for it and the
 * platform driver below binds to it and registers the reset controller.
 *
 * The Hi3536E reports the same SCSYSID as the Hi3536D V100 and is handled by
 * this driver too. It has no data sheet; the OpenIPC U-Boot programs other PLL
 * values for it (a faster CPU and DDR clock). The VPLL2 operating point of
 * that boot code (VCO 2592 MHz) is not specific to the E: a Hi3536D V100
 * board (a Xiongmai NVR) with its stock boot loader shows the same one. A
 * driver that only reads the PLLs reports all of that correctly. The sysctrl
 * init prints the chip version byte, which reads 0x01 on that D board; the
 * OpenIPC U-Boot reports 0x02 for a Hi3536E (not measured here).
 *
 * Nothing is assumed about what the boot loader left. The PLLs and the core
 * clock selects of PERI_CRG20 are only read (a switch needs the PLL lock and
 * the PERI_CRG21 handshake, and the whole chip hangs off the bus selects), and
 * the init code checks what it read: the selects against the handshake, the
 * lock and operating range of every PLL, and that the PLLs that feed the
 * module clocks run at all. The module clocks that the data sheet prints as
 * fixed rates are dividers of the decoded PLL outputs (or of the crystal), so
 * their rates follow what the PLL registers say, see
 * hi3536dv100_fixed_factor_clks. The module selects that a consumer depends
 * on are writable and are chosen by the device tree.
 *
 * The reset controller only accepts the reset bits that the data sheet lists
 * (hi3536dv100_resets): a device tree cannot make it write another register or
 * bit of the block, and the DDR resets are not among them.
 *
 * Read back, not modeled: usb2_phy_clk_sel (PERI_CRG46 bit 20: 0 = the USB
 * PHY provides its 48 MHz, free and UTMI clocks, 1 = the CRG provides them)
 * and ssc_bypass_clk_sel (PERI_CRG35 bit 19: 0 = the VPLL0 POSTDIV clock, 1 =
 * a clock sent from the VDP). The data sheet gives no rate for the CRG or VDP
 * clocks they can select, so the gates behind them have no parent, and
 * hi3536dv100_check_selects() warns when either select holds 1, the state
 * that nothing in this driver describes. The DDR, DDRTEST and test clock
 * gates are not registered, the kernel never touches them.
 */

#include <dt-bindings/clock/hi3536dv100-clock.h>
#include <linux/bitfield.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "clk.h"
#include "reset.h"

#define HI3536DV100_XTAL_RATE		24000000

/* CRG registers by the names of the data sheet */
#define PERI_CRG_PLL0			0x00
#define PERI_CRG_PLL2			0x08
#define PERI_CRG_PLL4			0x10
#define PERI_CRG_PLL6			0x18
#define PERI_CRG_PLL8			0x20
#define PERI_CRG_PLL10			0x28
#define PERI_CRG20			0x50
#define PERI_CRG21			0x54
#define PERI_CRG34			0x88
#define PERI_CRG35			0x8c
#define PERI_CRG36			0x90
#define PERI_CRG37			0x94
#define PERI_CRG38			0x98
#define PERI_CRG39			0x9c
#define PERI_CRG40			0xa0
#define PERI_CRG41			0xa4
#define PERI_CRG42			0xa8
#define PERI_CRG43			0xac
#define PERI_CRG44			0xb0
#define PERI_CRG45			0xb4
#define PERI_CRG46			0xb8
#define PERI_CRG47			0xbc
#define PERI_CRG48			0xc0
#define PERI_CRG49			0xc4
#define PERI_CRG50			0xc8
#define PERI_CRG51			0xcc
#define PERI_CRG52			0xd0
#define PERI_CRG53			0xd4
#define PERI_CRG54			0xd8
#define PERI_CRG55			0xdc
#define PERI_CRG56			0xe0
#define PERI_CRG57			0xe4
#define PERI_CRG58			0xe8

/*
 * PLL register pairs: PERI_CRG_PLL0/1 APLL, 2/3 VPLL0, 4/5 VPLL1, 6/7 VPLL2,
 * 8/9 EPLL, 10/11 DPLL. FOUTVCO = Fin * (fbdiv + frac / 2^24) / refdiv,
 * FOUTPOSTDIV = VCO / (postdiv1 * postdiv2); frac only counts with dsmpd
 * cleared, bypass passes the reference through.
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

/* the VCO of every PLL must run between 600 MHz and 2.4 GHz */
#define PLL_VCO_MIN			600000000ULL
#define PLL_VCO_MAX			2400000000ULL

struct hi3536dv100_pll {
	struct clk_hw hw;
	void __iomem *reg;	/* first of the two registers */
};

#define to_hi3536dv100_pll(_hw)	container_of(_hw, struct hi3536dv100_pll, hw)

/*
 * VCO frequency of the PLL at @reg, 0 if it is powered down or misprogrammed.
 * FOUTVCOPD (bit 22) is not tested: it only switches off the raw VCO output
 * tap, not the post-divided clock.
 */
static u64 hi3536dv100_pll_vco(void __iomem *reg, unsigned long parent_rate)
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
static unsigned long hi3536dv100_pll_rate(void __iomem *reg,
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

	return div_u64(hi3536dv100_pll_vco(reg, parent_rate), div1 * div2);
}

static unsigned long hi3536dv100_pll_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	struct hi3536dv100_pll *pll = to_hi3536dv100_pll(hw);

	return hi3536dv100_pll_rate(pll->reg, parent_rate);
}

static const struct clk_ops hi3536dv100_pll_ops = {
	.recalc_rate = hi3536dv100_pll_recalc_rate,
};

/*
 * PERI_CRG52 is the lock status of the six PLLs, one bit each, and the ssmod
 * field is the spread spectrum register of the PLL. The last field is the
 * highest output the data sheet allows for the PLL (0: none stated).
 */
#define PLL_LOCK_APLL			BIT(0)
#define PLL_LOCK_VPLL0			BIT(1)
#define PLL_LOCK_VPLL1			BIT(2)
#define PLL_LOCK_VPLL2			BIT(3)
#define PLL_LOCK_EPLL			BIT(4)
#define PLL_LOCK_DPLL			BIT(5)

static const struct {
	unsigned int id;
	const char *name;
	const char *label;	/* for the messages */
	unsigned long offset;
	u32 lock;
	unsigned long ssmod;	/* spread spectrum register */
	unsigned long max_rate;
} hi3536dv100_plls[] = {
	{ HI3536DV100_APLL_CLK, "apll", "APLL", PERI_CRG_PLL0, PLL_LOCK_APLL,
	  PERI_CRG56, 1100000000, },
	{ HI3536DV100_VPLL0_CLK, "vpll0", "VPLL0", PERI_CRG_PLL2, PLL_LOCK_VPLL0,
	  PERI_CRG53, 0, },
	{ HI3536DV100_VPLL1_CLK, "vpll1", "VPLL1", PERI_CRG_PLL4, PLL_LOCK_VPLL1,
	  PERI_CRG54, 800000000, },
	{ HI3536DV100_VPLL2_CLK, "vpll2", "VPLL2", PERI_CRG_PLL6, PLL_LOCK_VPLL2,
	  PERI_CRG55, 0, },
	{ HI3536DV100_EPLL_CLK, "epll", "EPLL", PERI_CRG_PLL8, PLL_LOCK_EPLL,
	  PERI_CRG57, 0, },
	{ HI3536DV100_DPLL_CLK, "dpll", "DPLL", PERI_CRG_PLL10, PLL_LOCK_DPLL,
	  PERI_CRG58, 0, },
};

static void hi3536dv100_unregister_plls(struct hisi_clock_data *clk_data, int n)
{
	while (n--) {
		struct clk **clk = &clk_data->clk_data.clks[hi3536dv100_plls[n].id];
		struct clk_hw *hw = __clk_get_hw(*clk);

		*clk = NULL;
		clk_hw_unregister(hw);
		kfree(to_hi3536dv100_pll(hw));
	}
}

static int __init hi3536dv100_register_plls(struct hisi_clock_data *clk_data)
{
	static const char *const parent_names[] = { "24m" };
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(hi3536dv100_plls); i++) {
		struct clk_init_data init = {
			.name = hi3536dv100_plls[i].name,
			.ops = &hi3536dv100_pll_ops,
			.parent_names = parent_names,
			.num_parents = 1,
		};
		struct hi3536dv100_pll *pll;
		struct clk *clk;

		pll = kzalloc_obj(*pll);
		if (!pll) {
			ret = -ENOMEM;
			goto err_unregister;
		}

		pll->reg = clk_data->base + hi3536dv100_plls[i].offset;
		pll->hw.init = &init;
		clk = clk_register(NULL, &pll->hw);
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			kfree(pll);
			goto err_unregister;
		}

		clk_data->clk_data.clks[hi3536dv100_plls[i].id] = clk;
	}

	return 0;

err_unregister:
	hi3536dv100_unregister_plls(clk_data, i);

	return ret;
}

/*
 * PERI_CRG20 selects the A7, DDR, system bus, media APB and system APB
 * clocks and PERI_CRG21 reads back the clock each select is switched to. A
 * select that disagrees with its read-back is a switch in progress or a
 * failed one, and every rate derived from it is wrong; a select on the
 * crystal means the boot loader never switched the clock to its PLL. All
 * fields are read only here, the clocks they feed are described by what the
 * registers say.
 */
#define SOC_SEL_A7			GENMASK(7, 6)
#define SOC_SEL_DDR			GENMASK(5, 4)
#define SOC_SEL_SYSBUS			GENMASK(3, 2)
#define SOC_SEL_MDAAPB			BIT(1)
#define SOC_SEL_SYSAPB			BIT(0)
#define SOC_SEL_ALL			0xffU

static void __init hi3536dv100_check_core_selects(struct device_node *np,
						  struct hisi_clock_data *clk_data)
{
	u32 sel = readl(clk_data->base + PERI_CRG20);
	u32 seled = readl(clk_data->base + PERI_CRG21);
	u32 status = readl(clk_data->base + PERI_CRG52);

	if ((sel ^ seled) & SOC_SEL_ALL)
		pr_warn("%pOF: PERI_CRG20 selects %#x but PERI_CRG21 reports %#x: a clock switch is not complete, the rates below may be wrong\n",
			np, sel & SOC_SEL_ALL, seled & SOC_SEL_ALL);

	switch (FIELD_GET(SOC_SEL_A7, sel)) {
	case 0:
		pr_warn("%pOF: the A7 runs from the crystal (24 MHz), the boot loader did not switch it to the APLL\n",
			np);
		break;
	case 1:
		if (!(status & PLL_LOCK_APLL))
			pr_warn("%pOF: the A7 runs from the APLL, which is not locked\n", np);
		break;
	default:
		pr_warn("%pOF: the A7 clock select holds the reserved code %lu\n",
			np, FIELD_GET(SOC_SEL_A7, sel));
	}

	switch (FIELD_GET(SOC_SEL_DDR, sel)) {
	case 0:
		pr_warn("%pOF: the DDR runs from the crystal (24 MHz), the boot loader did not switch it to the DPLL\n",
			np);
		break;
	case 1:
		if (!(status & PLL_LOCK_DPLL))
			pr_warn("%pOF: the DDR runs from the DPLL, which is not locked\n", np);
		break;
	default:
		pr_warn("%pOF: the DDR clock select holds the reserved code %lu\n",
			np, FIELD_GET(SOC_SEL_DDR, sel));
	}

	switch (FIELD_GET(SOC_SEL_SYSBUS, sel)) {
	case 0:
		pr_warn("%pOF: the system bus runs from the crystal (24 MHz), not at 202.5 MHz\n",
			np);
		break;
	case 2:
		break;
	default:
		pr_warn("%pOF: the system bus clock select holds the reserved code %lu (the vendor driver lists 250 and 300 MHz for codes 1 and 3), its rate is not known\n",
			np, FIELD_GET(SOC_SEL_SYSBUS, sel));
	}

	if (!(sel & SOC_SEL_MDAAPB))
		pr_warn("%pOF: the media APB runs from the crystal (24 MHz), not at 100 MHz\n", np);
	if (!(sel & SOC_SEL_SYSAPB))
		pr_warn("%pOF: the system APB runs from the crystal (24 MHz), not at 50 MHz: the UART, timer and GPIO clocks derive from it\n",
			np);
}

/*
 * Read back every PLL: a PLL that is running but not locked, a VCO outside
 * the range the data sheet allows, postdividers in the wrong order, an output
 * above the stated maximum, and an active spread spectrum modulator (which
 * makes the rate read here the unmodulated center). Report each, instead of
 * trusting whatever the boot loader programmed.
 *
 * The module clocks derived from the EPLL and the VPLL2 follow the rates
 * decoded here (see hi3536dv100_fixed_factor_clks), so what is checked for
 * those two PLLs is only that they run: a powered down or misprogrammed one
 * makes its module clocks read 0 Hz, and that is said here.
 */
#define SSMOD_CKEN			BIT(0)
#define SSMOD_DISABLE			BIT(2)

static void __init hi3536dv100_check_plls(struct device_node *np,
					  struct hisi_clock_data *clk_data)
{
	static const struct {
		const char *name;
		unsigned long offset;
		const char *clocks;
	} feeds[] = {
		{ "VPLL2", PERI_CRG_PLL6, "27, 54 and 202.5 MHz" },
		{ "EPLL", PERI_CRG_PLL8, "25, 50, 83.3, 100, 150 and 500 MHz" },
	};
	u32 status = readl(clk_data->base + PERI_CRG52);
	int i;

	for (i = 0; i < ARRAY_SIZE(hi3536dv100_plls); i++) {
		void __iomem *reg = clk_data->base + hi3536dv100_plls[i].offset;
		const char *name = hi3536dv100_plls[i].label;
		u64 vco = hi3536dv100_pll_vco(reg, HI3536DV100_XTAL_RATE);
		unsigned long out = hi3536dv100_pll_rate(reg, HI3536DV100_XTAL_RATE);
		u32 a = readl(reg);
		u32 b = readl(reg + 4);
		u32 ss = readl(clk_data->base + hi3536dv100_plls[i].ssmod);

		/* a powered down PLL is not an error, it is just not a clock */
		if (b & PLL_PD)
			continue;

		if (!(status & hi3536dv100_plls[i].lock))
			pr_warn("%pOF: the %s is running but not locked\n", np, name);
		if (b & PLL_BYPASS)
			pr_warn("%pOF: the %s is bypassed, the clocks derived from it run at the 24 MHz reference\n",
				np, name);
		if (!(b & PLL_BYPASS) && (vco < PLL_VCO_MIN || vco > PLL_VCO_MAX))
			pr_warn("%pOF: the %s VCO runs at %llu Hz, outside the 600 MHz to 2.4 GHz of the data sheet\n",
				np, name, vco);
		if (FIELD_GET(PLL_POSTDIV1, a) < FIELD_GET(PLL_POSTDIV2, a))
			pr_warn("%pOF: the %s has postdiv1 %lu below postdiv2 %lu, the data sheet requires the reverse\n",
				np, name, FIELD_GET(PLL_POSTDIV1, a),
				FIELD_GET(PLL_POSTDIV2, a));
		if (hi3536dv100_plls[i].max_rate && out > hi3536dv100_plls[i].max_rate)
			pr_warn("%pOF: the %s runs at %lu Hz, above the data sheet maximum of %lu Hz\n",
				np, name, out, hi3536dv100_plls[i].max_rate);
		if ((ss & SSMOD_CKEN) && !(ss & SSMOD_DISABLE))
			pr_warn("%pOF: the %s spread spectrum modulator is on (register %#x), the rate it reports is the unmodulated one\n",
				np, name, ss);
	}

	for (i = 0; i < ARRAY_SIZE(feeds); i++) {
		void __iomem *reg = clk_data->base + feeds[i].offset;

		if (!hi3536dv100_pll_rate(reg, HI3536DV100_XTAL_RATE))
			pr_warn("%pOF: the %s is powered down or misprogrammed, the %s clocks derived from it read 0 Hz\n",
				np, feeds[i].name, feeds[i].clocks);
	}
}

/*
 * Module selects that have reserved codes: the boot loader may leave one in a
 * state the table cannot describe. Say so instead of handing out a clock with
 * no parent or a wrong rate. The two selects that are not modeled are read
 * back too, see the comment at the top of the file.
 */
#define FMC_CKEN			BIT(1)	/* PERI_CRG48 */
#define FMC_CKSEL			GENMASK(3, 2)
#define SATA_REFCLK_SEL			GENMASK(3, 2)	/* PERI_CRG44 */
#define UART_CKSEL			GENMASK(19, 18)	/* PERI_CRG51 */
#define USB2_PHY_CLK_SEL		BIT(20)	/* PERI_CRG46 */
#define SSC_BYPASS_CLK_SEL		BIT(19)	/* PERI_CRG35 */

static void __init hi3536dv100_check_selects(struct device_node *np,
					     struct hisi_clock_data *clk_data)
{
	u32 fmc = readl(clk_data->base + PERI_CRG48);
	u32 uart = readl(clk_data->base + PERI_CRG51);
	u32 sata = readl(clk_data->base + PERI_CRG44);
	u32 usb = readl(clk_data->base + PERI_CRG46);
	u32 hdmi = readl(clk_data->base + PERI_CRG35);

	/* the FMC gate is never switched off as unused: say what it was left at */
	pr_info("%pOF: the FMC clock gate is %s and is not switched off as unused (the boot flash is behind it)\n",
		np, fmc & FMC_CKEN ? "on" : "off");
	if (FIELD_GET(FMC_CKSEL, fmc) == 3)
		pr_warn("%pOF: the FMC clock select holds the reserved code 3\n", np);
	if (FIELD_GET(UART_CKSEL, uart) == 3)
		pr_warn("%pOF: the UART clock select holds the reserved code 3\n", np);
	if (FIELD_GET(SATA_REFCLK_SEL, sata) != 1 &&
	    FIELD_GET(SATA_REFCLK_SEL, sata) != 2)
		pr_warn("%pOF: the SATA PHY reference clock select holds the reserved code %lu\n",
			np, FIELD_GET(SATA_REFCLK_SEL, sata));
	if (usb & USB2_PHY_CLK_SEL)
		pr_warn("%pOF: the USB 2.0 PHY clock select (PERI_CRG46 bit 20) is set: the CRG is to provide the PHY 48 MHz, free and UTMI clocks, which this driver does not model (no rate is known)\n",
			np);
	if (hdmi & SSC_BYPASS_CLK_SEL)
		pr_warn("%pOF: the HDMI SSC bypass clock select (PERI_CRG35 bit 19) is set: the clock comes from the VDP, which this driver does not model (no rate is known)\n",
			np);
}

/*
 * The crystal and the two Ethernet pad clocks are the only clocks that do not
 * come from a PLL. The RMII pad clock is 50 MHz because RMII is defined that
 * way; the data sheet gives no rate. The MII clock follows the link speed, so
 * it has no rate.
 */
static const struct hisi_fixed_rate_clock hi3536dv100_fixed_rate_clks[] = {
	{ HI3536DV100_FIXED_24M, "24m", NULL, 0, HI3536DV100_XTAL_RATE, },
	/* inputs of the Ethernet interface */
	{ HI3536DV100_MII_PAD_CLK, "mii_pad", NULL, 0, 0, },
	{ HI3536DV100_RMII_PAD_CLK, "rmii_pad", NULL, 0, 50000000, },
};

/*
 * Rates of the module clock selects as listed by the data sheet, modeled as
 * fixed dividers of the clock they come from so that they follow the registers:
 * the 2 MHz UART clock and the 3 MHz timer and watchdog count clock are
 * dividers of the 24 MHz crystal, the rest of the EPLL and VPLL2 outputs that
 * hi3536dv100_pll_rate() decodes. The data sheet prints the rates but not the
 * divider each one comes from, so the ratios are those of the printed rates to
 * the POSTDIV outputs at the reset operating point (EPLL 500 MHz, VCO 1500 MHz;
 * VPLL2 324 MHz, VCO 1620 MHz); 83.3 MHz is EPLL FOUT3, 1500 / 18 MHz, which
 * is the same as 500 / 6. That the POSTDIV output and not the VCO is the tap is
 * inferred: the boot code of a Hi3536D V100 board programs the VPLL2 with a
 * 2592 MHz VCO and the same 324 MHz POSTDIV output, and the 25 to 150 MHz
 * clocks are integer fractions of the 500 MHz EPLL output. The measured 50 MHz
 * system APB clock (the EPLL derived one) agrees with it.
 *
 * The 202.5 MHz system bus and VDP clock is not an integer fraction of the
 * 324 MHz POSTDIV output, nor of a 2592 MHz VCO (it is the 1620 MHz VCO / 8), so
 * its source cannot be established: hi3536dv100_register_sysbus_clk() registers
 * it with the printed rate only at that operating point and with no rate
 * otherwise.
 */
static const struct hisi_fixed_factor_clock hi3536dv100_fixed_factor_clks[] = {
	{ HI3536DV100_FIXED_2M, "2m", "24m", 1, 12, 0, },
	{ HI3536DV100_FIXED_3M, "3m", "24m", 1, 8, 0, },
	{ HI3536DV100_FIXED_25M, "25m", "epll", 1, 20, 0, },
	{ HI3536DV100_FIXED_50M, "50m", "epll", 1, 10, 0, },
	{ HI3536DV100_FIXED_83P3M, "83.3m", "epll", 1, 6, 0, },
	{ HI3536DV100_FIXED_100M, "100m", "epll", 1, 5, 0, },
	{ HI3536DV100_FIXED_150M, "150m", "epll", 3, 10, 0, },
	{ HI3536DV100_FIXED_500M, "500m", "epll", 1, 1, 0, },
	{ HI3536DV100_FIXED_27M, "27m", "vpll2", 1, 12, 0, },
	{ HI3536DV100_FIXED_54M, "54m", "vpll2", 1, 6, 0, },
};

#define HI3536DV100_SYSBUS_RATE		202500000
#define HI3536DV100_SYSBUS_VPLL2_VCO	1620000000ULL

static int __init hi3536dv100_register_sysbus_clk(struct device_node *np,
						  struct hisi_clock_data *clk_data)
{
	struct hisi_fixed_rate_clock sysbus = {
		HI3536DV100_FIXED_202P5M, "202.5m", NULL, 0, 0,
	};
	u64 vco = hi3536dv100_pll_vco(clk_data->base + PERI_CRG_PLL6,
				      HI3536DV100_XTAL_RATE);

	if (vco == HI3536DV100_SYSBUS_VPLL2_VCO)
		sysbus.fixed_rate = HI3536DV100_SYSBUS_RATE;
	else
		pr_warn("%pOF: the VPLL2 VCO runs at %llu Hz, not the %llu Hz of the data sheet clock tree: the divider of the 202.5 MHz system bus and VDP clock is not known, so it reads 0 Hz\n",
			np, vco, HI3536DV100_SYSBUS_VPLL2_VCO);

	return hisi_clk_register_fixed_rate(&sysbus, 1, clk_data);
}

/*
 * PERI_CRG20: a7_sc_sel [7:6] (0 = 24 MHz, 1 = APLL POSTDIV), ddr_sc_sel [5:4]
 * (0 = 24 MHz, 1 = DPLL POSTDIV), sysbus_sc_sel [3:2] (0 = 24 MHz, 2 = 202.5
 * MHz), mdaapb_sc_sel [1] (0 = 24 MHz, 1 = 100 MHz) and sysapb_sc_sel [0] (0 =
 * 24 MHz, 1 = 50 MHz). Read only, see hi3536dv100_check_core_selects().
 */
static const char *const a7_mux_p[] = { "24m", "apll" };
static const char *const ddr_mux_p[] = { "24m", "dpll" };
static const char *const sysbus_mux_p[] = { "24m", "202.5m" };
static const char *const mdaapb_mux_p[] = { "24m", "100m" };
static const char *const sysapb_mux_p[] = { "24m", "50m" };
static const u32 sysbus_mux_table[] = { 0, 2 };

static const struct hisi_mux_clock hi3536dv100_core_mux_clks[] = {
	{ HI3536DV100_A7_MUX, "a7_mux", a7_mux_p, ARRAY_SIZE(a7_mux_p),
		0, PERI_CRG20, 6, 2, CLK_MUX_READ_ONLY, NULL, },
	{ HI3536DV100_DDR_MUX, "ddr_mux", ddr_mux_p, ARRAY_SIZE(ddr_mux_p),
		0, PERI_CRG20, 4, 2, CLK_MUX_READ_ONLY, NULL, },
	{ HI3536DV100_SYSBUS_MUX, "sysbus_mux", sysbus_mux_p,
		ARRAY_SIZE(sysbus_mux_p), 0, PERI_CRG20, 2, 2, CLK_MUX_READ_ONLY,
		sysbus_mux_table, },
	{ HI3536DV100_MDAAPB_MUX, "mdaapb_mux", mdaapb_mux_p,
		ARRAY_SIZE(mdaapb_mux_p), 0, PERI_CRG20, 1, 1, CLK_MUX_READ_ONLY, NULL, },
	{ HI3536DV100_SYSAPB_CLK, "apb", sysapb_mux_p, ARRAY_SIZE(sysapb_mux_p),
		0, PERI_CRG20, 0, 1, CLK_MUX_READ_ONLY, NULL, },
};

/* PERI_CRG51 uart_cksel [19:18]: APB, 24 MHz, 2 MHz (3 is reserved) */
static const char *const uart_mux_p[] = { "apb", "24m", "2m" };
/* PERI_CRG48 fmc_cksel [3:2]: 24 MHz, 83.3 MHz, 150 MHz (3 is reserved) */
static const char *const fmc_mux_p[] = { "24m", "83.3m", "150m" };
/* PERI_CRG49 ethcore_cksel [7]: 54 MHz or 100 MHz */
static const char *const eth_core_mux_p[] = { "54m", "100m" };
/*
 * PERI_CRG49 eth_rmiick_sel [2]: the RMII clock, internal (50 MHz) or the
 * RMII_CLK pad. rmii_mode [8]: MII (the MII pad clocks) or RMII (the RMII
 * clock). ext_ethphy_cksel [5:4]: the clock the PHY output gives an external
 * PHY, 25 MHz, 24 MHz, any other value 27 MHz. All three are writable so
 * that a board can choose them; the boot loader's value is only read back.
 */
static const char *const eth_rmii_mux_p[] = { "50m", "rmii_pad" };
static const char *const eth_if_mux_p[] = { "mii_pad", "eth_rmii_mux" };
static const char *const eth_phy_mux_p[] = { "25m", "24m", "27m", "27m" };
/* PERI_CRG44 sataphy0_refclk_sel [3:2]: 25 MHz (1) or 100 MHz (2) */
static const char *const sata_ref_mux_p[] = { "25m", "100m" };
static const u32 sata_ref_mux_table[] = { 1, 2 };
/* PERI_CRG34 vdp_cksel [6]: 202.5 MHz or 100 MHz */
static const char *const vdp_mux_p[] = { "202.5m", "100m" };
/*
 * PERI_CRG43 aiao_core_cksel [3]: 100 MHz or 50 MHz; aiao_cksel [2]: the
 * VPLL1 POSTDIV output or 500 MHz
 */
static const char *const aiao_core_mux_p[] = { "100m", "50m" };
static const char *const aiao_mclk_mux_p[] = { "vpll1", "500m" };

static const struct hisi_mux_clock hi3536dv100_mux_clks[] = {
	{ HI3536DV100_UART_MUX, "uart_mux", uart_mux_p, ARRAY_SIZE(uart_mux_p),
		0, PERI_CRG51, 18, 2, 0, NULL, },
	{ HI3536DV100_FMC_MUX, "fmc_mux", fmc_mux_p, ARRAY_SIZE(fmc_mux_p),
		0, PERI_CRG48, 2, 2, 0, NULL, },
	{ HI3536DV100_ETH_CORE_MUX, "eth_core_mux", eth_core_mux_p,
		ARRAY_SIZE(eth_core_mux_p), 0, PERI_CRG49, 7, 1, 0, NULL, },
	{ HI3536DV100_ETH_PHY_MUX, "eth_phy_mux", eth_phy_mux_p,
		ARRAY_SIZE(eth_phy_mux_p), 0, PERI_CRG49, 4, 2, 0, NULL, },
	{ HI3536DV100_ETH_RMII_MUX, "eth_rmii_mux", eth_rmii_mux_p,
		ARRAY_SIZE(eth_rmii_mux_p), 0, PERI_CRG49, 2, 1, 0, NULL, },
	{ HI3536DV100_ETH_IF_MUX, "eth_if_mux", eth_if_mux_p,
		ARRAY_SIZE(eth_if_mux_p), 0, PERI_CRG49, 8, 1, 0, NULL, },
	{ HI3536DV100_SATA_REF_MUX, "sata_ref_mux", sata_ref_mux_p,
		ARRAY_SIZE(sata_ref_mux_p), 0, PERI_CRG44, 2, 2, 0,
		sata_ref_mux_table, },
	{ HI3536DV100_VDP_MUX, "vdp_mux", vdp_mux_p, ARRAY_SIZE(vdp_mux_p),
		0, PERI_CRG34, 6, 1, 0, NULL, },
	{ HI3536DV100_AIAO_CORE_MUX, "aiao_core_mux", aiao_core_mux_p,
		ARRAY_SIZE(aiao_core_mux_p), 0, PERI_CRG43, 3, 1, 0, NULL, },
	{ HI3536DV100_AIAO_MCLK_MUX, "aiao_mclk_mux", aiao_mclk_mux_p,
		ARRAY_SIZE(aiao_mclk_mux_p), 0, PERI_CRG43, 2, 1, 0, NULL, },
};

/*
 * Clock gates (set = enabled). Blocks whose input rate the data sheet does not
 * state have no parent and report 0 Hz. The soft reset bits of the same
 * registers belong to the reset controller. The gates behind a module clock
 * select pass a rate request on to the select, so that clk_set_rate() of a
 * consumer (the SPI NOR driver of the FMC) picks the closest input. A gate
 * nobody uses is switched off by the clock core after boot, except the FMC
 * gate: the flash the machine boots from is behind it and nothing in the
 * device tree claims the gate yet, so it is left as the boot loader had it
 * instead of being switched off behind the flash. The data sheet reset value
 * of the gate is on, and a soft reset restoring it is shown for PERI_CRG49
 * only (the FMC gate itself was on before and after), so this is a
 * precaution, not a known need. The DDR gates of PERI_CRG33, the DDRTEST gate
 * and the test clock enable are not listed: the kernel must never switch them
 * off.
 */
static const struct hisi_gate_clock hi3536dv100_gate_clks[] = {
	/* PERI_CRG51 */
	{ HI3536DV100_UART0_CLK, "clk_uart0", "uart_mux", 0, PERI_CRG51, 15, 0, },
	{ HI3536DV100_UART1_CLK, "clk_uart1", "uart_mux", 0, PERI_CRG51, 16, 0, },
	{ HI3536DV100_UART2_CLK, "clk_uart2", "uart_mux", 0, PERI_CRG51, 17, 0, },
	{ HI3536DV100_IR_CLK, "clk_ir", NULL, 0, PERI_CRG51, 12, 0, },
	/* PERI_CRG48, PERI_CRG49 eth_cken and int_fephy_cken, PERI_CRG50, PERI_CRG42 */
	{ HI3536DV100_FMC_CLK, "clk_fmc", "fmc_mux",
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, PERI_CRG48, 1, 0, },
	{ HI3536DV100_ETH_CLK, "clk_eth", "eth_core_mux", CLK_SET_RATE_PARENT, PERI_CRG49, 1, 0, },
	{ HI3536DV100_ETH_FEPHY_CLK, "clk_eth_fephy", NULL, 0, PERI_CRG49, 10, 0, },
	{ HI3536DV100_DMAC_CLK, "clk_dmac", NULL, 0, PERI_CRG50, 5, 0, },
	{ HI3536DV100_CIPHER_CLK, "clk_cipher", NULL, 0, PERI_CRG42, 1, 0, },
	/* PERI_CRG44 sataphy0_ref_cken, PERI_CRG45 */
	{ HI3536DV100_SATA_PHY_REF_CLK, "clk_sata_phy_ref", "sata_ref_mux", 0, PERI_CRG44, 0, 0, },
	{ HI3536DV100_SATA_BUS_CLK, "clk_sata_bus", NULL, 0, PERI_CRG45, 0, 0, },
	{ HI3536DV100_SATA_RX0_CLK, "clk_sata_rx0", NULL, 0, PERI_CRG45, 1, 0, },
	{ HI3536DV100_SATA_CKO_ALIVE_CLK, "clk_sata_cko_alive", NULL, 0, PERI_CRG45, 2, 0, },
	{ HI3536DV100_SATA_TX0_CLK, "clk_sata_tx0", NULL, 0, PERI_CRG45, 3, 0, },
	/* PERI_CRG46, PERI_CRG47 usb2_phy_cken */
	{ HI3536DV100_USB2_BUS_CLK, "clk_usb2_bus", NULL, 0, PERI_CRG46, 0, 0, },
	{ HI3536DV100_USB2_OHCI48M_CLK, "clk_usb2_ohci48m", NULL, 0, PERI_CRG46, 1, 0, },
	{ HI3536DV100_USB2_OHCI12M_CLK, "clk_usb2_ohci12m", NULL, 0, PERI_CRG46, 2, 0, },
	{ HI3536DV100_USB2_HST_PHY_CLK, "clk_usb2_hst_phy", NULL, 0, PERI_CRG46, 4, 0, },
	{ HI3536DV100_USB2_UTMI0_CLK, "clk_usb2_utmi0", NULL, 0, PERI_CRG46, 5, 0, },
	{ HI3536DV100_USB2_UTMI1_CLK, "clk_usb2_utmi1", NULL, 0, PERI_CRG46, 6, 0, },
	{ HI3536DV100_USB2_PHY_CLK, "clk_usb2_phy", NULL, 0, PERI_CRG47, 0, 0, },
	/* media: cken is bit 1 of PERI_CRG37-40, bit 0 of PERI_CRG41, bits 1, 11 of PERI_CRG43 */
	{ HI3536DV100_TDE_CLK, "clk_tde", NULL, 0, PERI_CRG37, 1, 0, },
	{ HI3536DV100_VGS_CLK, "clk_vgs", NULL, 0, PERI_CRG38, 1, 0, },
	{ HI3536DV100_JPGE_CLK, "clk_jpge", NULL, 0, PERI_CRG39, 1, 0, },
	{ HI3536DV100_JPGD_CLK, "clk_jpgd", NULL, 0, PERI_CRG40, 1, 0, },
	{ HI3536DV100_VDH_CLK, "clk_vdh", NULL, 0, PERI_CRG41, 0, 0, },
	{ HI3536DV100_AIAO_CLK, "clk_aiao", "aiao_core_mux", 0, PERI_CRG43, 1, 0, },
	{ HI3536DV100_CODEC_CLK, "clk_codec", NULL, 0, PERI_CRG43, 11, 0, },
	/* PERI_CRG34 vou_cken [5], vou_hd0_cken [4], vou_hcken [3], hd_dac_cken [2] */
	{ HI3536DV100_VOU_CLK, "clk_vou", "vdp_mux", 0, PERI_CRG34, 5, 0, },
	{ HI3536DV100_VOU_HD0_CLK, "clk_vou_hd0", NULL, 0, PERI_CRG34, 4, 0, },
	{ HI3536DV100_VOU_BUS_CLK, "clk_vou_bus", NULL, 0, PERI_CRG34, 3, 0, },
	{ HI3536DV100_HD_DAC_CLK, "clk_hd_dac", NULL, 0, PERI_CRG34, 2, 0, },
	/* PERI_CRG35 pxl_cken [16], ssc_in_cken [0], ssc_bypass_cken [1], osc_24m_cken [2] */
	{ HI3536DV100_HDMI_PXL_CLK, "clk_hdmi_pxl", NULL, 0, PERI_CRG35, 16, 0, },
	{ HI3536DV100_HDMI_SSC_IN_CLK, "clk_hdmi_ssc_in", NULL, 0, PERI_CRG35, 0, 0, },
	{ HI3536DV100_HDMI_SSC_BYPASS_CLK, "clk_hdmi_ssc_bypass", NULL, 0, PERI_CRG35, 1, 0, },
	{ HI3536DV100_HDMI_OSC24M_CLK, "clk_hdmi_osc24m", NULL, 0, PERI_CRG35, 2, 0, },
	/* PERI_CRG36 phy_tmds_cken */
	{ HI3536DV100_HDMI_TMDS_CLK, "clk_hdmi_tmds", NULL, 0, PERI_CRG36, 0, 0, },
};

/*
 * A clock id that no table registers must not give the caller a silent dummy
 * clock (a NULL clk is a valid no-op clock to the framework): hand out an error.
 */
static void __init hi3536dv100_fill_holes(struct hisi_clock_data *clk_data, int n)
{
	int i;

	for (i = 0; i < n; i++)
		if (!clk_data->clk_data.clks[i])
			clk_data->clk_data.clks[i] = ERR_PTR(-ENOENT);
}

/* undo hisi_clk_init(), the provider is removed first */
static void __init hi3536dv100_clk_free(struct device_node *np,
					struct hisi_clock_data *clk_data)
{
	of_clk_del_provider(np);
	iounmap(clk_data->base);
	kfree(clk_data->clk_data.clks);
	kfree(clk_data);
}

/* the groups of hi3536dv100_crg_clk_init() registered so far, in order */
enum hi3536dv100_crg_stage {
	HI3536DV100_STAGE_NONE,
	HI3536DV100_STAGE_FIXED_RATE,
	HI3536DV100_STAGE_PLLS,
	HI3536DV100_STAGE_FIXED_FACTOR,
	HI3536DV100_STAGE_SYSBUS,
	HI3536DV100_STAGE_CORE_MUX,
	HI3536DV100_STAGE_MUX,
};

/*
 * Take the clocks back in reverse order. The provider goes first, so that no
 * consumer can look up a clock that is being unregistered.
 */
static void __init hi3536dv100_crg_unwind(struct device_node *np,
					  struct hisi_clock_data *clk_data,
					  enum hi3536dv100_crg_stage stage)
{
	of_clk_del_provider(np);

	switch (stage) {
	case HI3536DV100_STAGE_MUX:
		hisi_clk_unregister_mux(hi3536dv100_mux_clks,
					ARRAY_SIZE(hi3536dv100_mux_clks), clk_data);
		fallthrough;
	case HI3536DV100_STAGE_CORE_MUX:
		hisi_clk_unregister_mux(hi3536dv100_core_mux_clks,
					ARRAY_SIZE(hi3536dv100_core_mux_clks), clk_data);
		fallthrough;
	case HI3536DV100_STAGE_SYSBUS:
		clk_unregister_fixed_rate(clk_data->clk_data.clks[HI3536DV100_FIXED_202P5M]);
		fallthrough;
	case HI3536DV100_STAGE_FIXED_FACTOR:
		hisi_clk_unregister_fixed_factor(hi3536dv100_fixed_factor_clks,
						 ARRAY_SIZE(hi3536dv100_fixed_factor_clks),
						 clk_data);
		fallthrough;
	case HI3536DV100_STAGE_PLLS:
		hi3536dv100_unregister_plls(clk_data, ARRAY_SIZE(hi3536dv100_plls));
		fallthrough;
	case HI3536DV100_STAGE_FIXED_RATE:
		hisi_clk_unregister_fixed_rate(hi3536dv100_fixed_rate_clks,
					       ARRAY_SIZE(hi3536dv100_fixed_rate_clks),
					       clk_data);
		fallthrough;
	case HI3536DV100_STAGE_NONE:
		break;
	}

	/* the provider is gone already: deleting it again does nothing */
	hi3536dv100_clk_free(np, clk_data);
}

static void __init hi3536dv100_crg_clk_init(struct device_node *np)
{
	enum hi3536dv100_crg_stage stage = HI3536DV100_STAGE_NONE;
	struct hisi_clock_data *clk_data;

	clk_data = hisi_clk_init(np, HI3536DV100_CRG_NR_CLKS);
	if (!clk_data)
		return;

	/* each helper cleans up after itself when its own group fails */
	if (hisi_clk_register_fixed_rate(hi3536dv100_fixed_rate_clks,
					 ARRAY_SIZE(hi3536dv100_fixed_rate_clks), clk_data))
		goto err;
	stage = HI3536DV100_STAGE_FIXED_RATE;
	if (hi3536dv100_register_plls(clk_data))
		goto err;
	stage = HI3536DV100_STAGE_PLLS;
	if (hisi_clk_register_fixed_factor(hi3536dv100_fixed_factor_clks,
					   ARRAY_SIZE(hi3536dv100_fixed_factor_clks),
					   clk_data))
		goto err;
	stage = HI3536DV100_STAGE_FIXED_FACTOR;
	if (hi3536dv100_register_sysbus_clk(np, clk_data))
		goto err;
	stage = HI3536DV100_STAGE_SYSBUS;
	if (hisi_clk_register_mux(hi3536dv100_core_mux_clks,
				  ARRAY_SIZE(hi3536dv100_core_mux_clks), clk_data))
		goto err;
	stage = HI3536DV100_STAGE_CORE_MUX;
	if (hisi_clk_register_mux(hi3536dv100_mux_clks,
				  ARRAY_SIZE(hi3536dv100_mux_clks), clk_data))
		goto err;
	stage = HI3536DV100_STAGE_MUX;
	if (hisi_clk_register_gate(hi3536dv100_gate_clks,
				   ARRAY_SIZE(hi3536dv100_gate_clks), clk_data))
		goto err;

	hi3536dv100_fill_holes(clk_data, HI3536DV100_CRG_NR_CLKS);
	hi3536dv100_check_core_selects(np, clk_data);
	hi3536dv100_check_plls(np, clk_data);
	hi3536dv100_check_selects(np, clk_data);

	return;

err:
	pr_err("%pOF: failed to register all clocks\n", np);
	/* no provider at all is better than one with missing clocks */
	hi3536dv100_crg_unwind(np, clk_data, stage);
}

CLK_OF_DECLARE_DRIVER(hi3536dv100_crg, "hisilicon,hi3536dv100-crg", hi3536dv100_crg_clk_init);

/* ---------------- system controller block (sysctrl@12050000) ---------------- */

/*
 * The SP804 count clocks are selected by SC_CTRL (sysctrl + 0x0): bits 16
 * (timer 0), 18 (1), 20 (2), 22 (3), 25 (4), 27 (5), 29 (6) and 31 (7) choose
 * between the 3 MHz reference (0) and the system APB clock (1). The boot
 * loader may leave either value, so model the select as a read-only mux
 * (SC_CTRL is write-protected by SC_LOCKEN) and let the read-back of the
 * register decide the rate. Timer init runs at start_kernel, long before
 * platform driver probing, so the mux is registered from the sysctrl early
 * init; its parents come from the CRG early init, which comes first in the
 * device tree.
 */
#define SC_CTRL				0x0

static const char *const hi3536dv100_timer_mux_p[] = { "3m", "apb" };
static const u32 hi3536dv100_timer_mux_table[] = { 0, 1 };

#define HI3536DV100_TIMER_MUX(_id, _name, _shift)			\
	{ _id, _name, hi3536dv100_timer_mux_p,				\
		ARRAY_SIZE(hi3536dv100_timer_mux_p), 0, SC_CTRL, _shift, 1,	\
		CLK_MUX_READ_ONLY, hi3536dv100_timer_mux_table, }

static const struct hisi_mux_clock hi3536dv100_sysctrl_mux[] = {
	HI3536DV100_TIMER_MUX(HI3536DV100_TIME0_0_CLK, "timer0_0", 16),
	HI3536DV100_TIMER_MUX(HI3536DV100_TIME0_1_CLK, "timer0_1", 18),
	HI3536DV100_TIMER_MUX(HI3536DV100_TIME1_2_CLK, "timer1_2", 20),
	HI3536DV100_TIMER_MUX(HI3536DV100_TIME1_3_CLK, "timer1_3", 22),
	HI3536DV100_TIMER_MUX(HI3536DV100_TIME2_4_CLK, "timer2_4", 25),
	HI3536DV100_TIMER_MUX(HI3536DV100_TIME2_5_CLK, "timer2_5", 27),
	HI3536DV100_TIMER_MUX(HI3536DV100_TIME3_6_CLK, "timer3_6", 29),
	HI3536DV100_TIMER_MUX(HI3536DV100_TIME3_7_CLK, "timer3_7", 31),
};

/*
 * SC_LOCKEN (offset 0x44) write protects SC_CTRL and SC_SYSRES: it reads 0 when
 * the registers can be written and 1 when they are locked (the Hi3516E V300 and
 * Hi3519 V101 data sheets say so; the register description of the Hi3536D
 * V100 does not state the values, so the same is assumed; a Hi3536D V100 board
 * reads 0 and reboots through SC_SYSRES, which confirms that 0 is unlocked, the
 * locked value was not seen). The timer selects
 * above are read only anyway, but the reboot of the device tree writes SC_SYSRES
 * through this system controller, and a locked register makes it do nothing.
 * The driver does not write the unlock key: the protection is the boot loader's
 * to give up.
 */
#define SC_LOCKEN			0x44

/*
 * SC_SYSID0..3 (offsets 0xee0, 0xee4, 0xee8 and 0xeec) hold one byte each and
 * read 0x00, 0xd1, 0x36 and 0x35 on the Hi3536D V100: the chip identifier
 * 0x3536d100. The OpenIPC U-Boot reports the same identifier on the Hi3536E.
 * The top byte of SC_SYSID0 is the chip version, which reads 0x01 on a Hi3536D
 * V100 (a Xiongmai NVR board); the OpenIPC U-Boot reports 0x02 for the Hi3536E
 * (not measured here). It is only printed, nothing depends on it. A different
 * identifier means this device tree is for another chip.
 */
#define SC_SYSID0			0xee0
#define SC_SYSID_CHIP			0x3536d100
#define SC_SYSID0_VERSION		GENMASK(31, 24)
#define SC_SYSID_BYTE			GENMASK(7, 0)

static void __init hi3536dv100_check_chip_id(struct device_node *np,
					     struct hisi_clock_data *clk_data)
{
	u32 sysid0 = readl(clk_data->base + SC_SYSID0);
	u32 id = (readl(clk_data->base + SC_SYSID0 + 12) & SC_SYSID_BYTE) << 24 |
		 (readl(clk_data->base + SC_SYSID0 + 8) & SC_SYSID_BYTE) << 16 |
		 (readl(clk_data->base + SC_SYSID0 + 4) & SC_SYSID_BYTE) << 8 |
		 (sysid0 & SC_SYSID_BYTE);

	if (id != SC_SYSID_CHIP)
		pr_warn("%pOF: chip identifier %#x is not the %#x of a Hi3536D V100 or Hi3536E\n",
			np, id, SC_SYSID_CHIP);
	else
		pr_info("%pOF: chip identifier %#x, chip version %lu\n",
			np, id, FIELD_GET(SC_SYSID0_VERSION, sysid0));
}

static void __init hi3536dv100_sysctrl_clk_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;
	u32 lock;

	clk_data = hisi_clk_init(np, HI3536DV100_SC_NR_CLKS);
	if (!clk_data)
		return;

	if (hisi_clk_register_mux(hi3536dv100_sysctrl_mux,
				  ARRAY_SIZE(hi3536dv100_sysctrl_mux), clk_data)) {
		pr_err("%pOF: failed to register the timer clocks\n", np);
		/* no provider at all is better than one with missing timer clocks */
		hi3536dv100_clk_free(np, clk_data);
		return;
	}

	hi3536dv100_fill_holes(clk_data, HI3536DV100_SC_NR_CLKS);
	hi3536dv100_check_chip_id(np, clk_data);

	lock = readl(clk_data->base + SC_LOCKEN);
	if (lock)
		pr_warn("%pOF: SC_LOCKEN reads %#x: SC_CTRL and SC_SYSRES are write protected, so a reboot through SC_SYSRES will not work\n",
			np, lock);
}

CLK_OF_DECLARE(hi3536dv100_sysctrl, "hisilicon,hi3536dv100-sysctrl",
	       hi3536dv100_sysctrl_clk_init);

/* ---------------- platform driver (reset controller) ---------------- */

/*
 * The soft reset bits of the CRG by register, from the PERI_CRGn descriptions
 * (the bits are listed in include/dt-bindings/clock/hi3536dv100-clock.h). A
 * reset specifier of a device tree is only accepted when it names one of them.
 * The DDR resets of PERI_CRG33 (0x84) and the DDRTEST reset (PERI_CRG50 bit 2)
 * are left out on purpose: resetting the memory controller from the kernel
 * would take down the memory it runs from.
 */
static const struct hisi_reset_bits hi3536dv100_resets[] = {
	{ PERI_CRG34, BIT(0) },
	{ PERI_CRG35, BIT(6) | BIT(7) | BIT(9) },
	{ PERI_CRG36, BIT(4) | BIT(5) },
	{ PERI_CRG37, BIT(0) },
	{ PERI_CRG38, BIT(0) },
	{ PERI_CRG39, BIT(0) },
	{ PERI_CRG40, BIT(0) },
	{ PERI_CRG41, BIT(4) | BIT(5) | BIT(6) },
	{ PERI_CRG42, BIT(0) },
	{ PERI_CRG43, BIT(0) | BIT(10) },
	{ PERI_CRG44, BIT(1) },
	{ PERI_CRG45, GENMASK(11, 8) },
	{ PERI_CRG46, GENMASK(14, 12) | BIT(16) },
	{ PERI_CRG47, GENMASK(4, 1) },
	{ PERI_CRG48, BIT(0) },
	{ PERI_CRG49, BIT(0) | BIT(3) | BIT(9) },
	{ PERI_CRG50, BIT(4) },
	{ PERI_CRG51, BIT(1) | BIT(3) | BIT(4) | GENMASK(9, 7) },
};

static int hi3536dv100_crg_probe(struct platform_device *pdev)
{
	/* the gates in this register block share it with the reset bits */
	return PTR_ERR_OR_ZERO(hisi_reset_init_checked(pdev, hisi_clk_get_lock(),
						       hi3536dv100_resets,
						       ARRAY_SIZE(hi3536dv100_resets)));
}

static const struct of_device_id hi3536dv100_crg_match_table[] = {
	{ .compatible = "hisilicon,hi3536dv100-crg" },
	{ }
};
MODULE_DEVICE_TABLE(of, hi3536dv100_crg_match_table);

static struct platform_driver hi3536dv100_crg_driver = {
	.probe = hi3536dv100_crg_probe,
	.driver = {
		.name = "hi3536dv100-crg",
		.of_match_table = hi3536dv100_crg_match_table,
		.suppress_bind_attrs = true,
	},
};
builtin_platform_driver(hi3536dv100_crg_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HiSilicon Hi3536D V100 CRG driver");
