// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * HiSilicon Hi3516C V200 / Hi3518E V20x clock and reset generator (CRG) and
 * system controller clocks.
 *
 * The CRG layout comes from the PERI_CRGn register descriptions of the
 * Hi3518E V20x / Hi3516C V200 data sheet (system control, CRG). The SP804 timer count
 * clock selects live in the system controller, so it is a second clock
 * provider. Both providers register early (the CRG with
 * CLK_OF_DECLARE_DRIVER, the system controller with CLK_OF_DECLARE) because
 * the SP804 clocksource is probed from time_init(), long before platform
 * drivers run; the CRG node stays unpopulated so that the platform driver
 * below can still bind to it and register the reset controller.
 *
 * Hi3516C V200 and Hi3518E V200/V201 share this CRG (same data sheet chapter),
 * so there is a single table. The PLLs are only read: the boot loader sets
 * them, and the fixed module clocks derived from them are plain fixed rates.
 */

#include <dt-bindings/clock/hi3516cv200-clock.h>
#include <linux/bitfield.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "clk.h"
#include "reset.h"

#define HI3516CV200_XTAL_RATE		24000000

/*
 * PLL register pairs: PERI_CRG0/1 APLL, 4/5 VPLL, 8/9 EPLL. FOUTVCO =
 * Fin * (fbdiv + frac / 2^24) / refdiv, FOUTPOSTDIV = VCO / (postdiv1 * postdiv2);
 * frac only counts with dsmpd cleared, bypass passes the reference through.
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

struct hi3516cv200_pll {
	struct clk_hw hw;
	void __iomem *reg;	/* first of the two registers */
};

#define to_hi3516cv200_pll(_hw)	container_of(_hw, struct hi3516cv200_pll, hw)

/*
 * VCO frequency of the PLL at @reg, 0 if it is powered down or misprogrammed.
 * FOUTVCOPD (bit 22) is not tested: it only switches off the raw VCO output
 * tap, not the post-divided clock; it is set on the APLL of a camera that runs
 * from it at 540 MHz.
 */
static u64 hi3516cv200_pll_vco(void __iomem *reg, unsigned long parent_rate)
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
static unsigned long hi3516cv200_pll_rate(void __iomem *reg,
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

	return div_u64(hi3516cv200_pll_vco(reg, parent_rate), div1 * div2);
}

static unsigned long hi3516cv200_pll_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	struct hi3516cv200_pll *pll = to_hi3516cv200_pll(hw);

	return hi3516cv200_pll_rate(pll->reg, parent_rate);
}

static const struct clk_ops hi3516cv200_pll_ops = {
	.recalc_rate = hi3516cv200_pll_recalc_rate,
};

static const struct {
	unsigned int id;
	const char *name;
	unsigned long offset;
} hi3516cv200_plls[] = {
	{ HI3516CV200_APLL_CLK, "apll", 0x00, },
	{ HI3516CV200_VPLL_CLK, "vpll", 0x10, },
	{ HI3516CV200_EPLL_CLK, "epll", 0x20, },
};

static void hi3516cv200_unregister_plls(struct hisi_clock_data *clk_data, int n)
{
	while (n--) {
		struct clk **clk = &clk_data->clk_data.clks[hi3516cv200_plls[n].id];
		struct clk_hw *hw = __clk_get_hw(*clk);

		*clk = NULL;
		clk_hw_unregister(hw);
		kfree(to_hi3516cv200_pll(hw));
	}
}

static int __init hi3516cv200_register_plls(struct hisi_clock_data *clk_data)
{
	static const char *const parent_names[] = { "24m" };
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(hi3516cv200_plls); i++) {
		struct clk_init_data init = {
			.name = hi3516cv200_plls[i].name,
			.ops = &hi3516cv200_pll_ops,
			.parent_names = parent_names,
			.num_parents = 1,
		};
		struct hi3516cv200_pll *pll;
		struct clk *clk;

		pll = kzalloc_obj(*pll);
		if (!pll) {
			ret = -ENOMEM;
			goto err_unregister;
		}

		pll->reg = clk_data->base + hi3516cv200_plls[i].offset;
		pll->hw.init = &init;
		clk = clk_register(NULL, &pll->hw);
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			kfree(pll);
			goto err_unregister;
		}

		clk_data->clk_data.clks[hi3516cv200_plls[i].id] = clk;
	}

	return 0;

err_unregister:
	hi3516cv200_unregister_plls(clk_data, i);

	return ret;
}

/*
 * The fixed module clocks below are dividers of the VPLL and EPLL, which the
 * boot loader programs. Read both back (output rate and lock status) and say
 * so if they are not what the table assumes, instead of reporting wrong rates
 * for every consumer. The APLL feeds the CPU; only its lock is checked, its
 * rate is read by the clock core.
 */
#define PERI_CRG_PLL_STATUS		0xe8
#define PLL_STATUS_APLL_LOCK		BIT(0)
#define PLL_STATUS_VPLL_LOCK		BIT(1)
#define PLL_STATUS_EPLL_LOCK		BIT(2)

static void __init hi3516cv200_check_plls(struct device_node *np,
					  struct hisi_clock_data *clk_data)
{
	static const struct {
		unsigned long offset;
		unsigned long rate;
		u32 lock;
		const char *name;
	} assumed[] = {
		{ 0x10, 594000000, PLL_STATUS_VPLL_LOCK, "VPLL", },
		{ 0x20, 500000000, PLL_STATUS_EPLL_LOCK, "EPLL", },
	};
	u32 status = readl(clk_data->base + PERI_CRG_PLL_STATUS);
	int i;

	if (!(status & PLL_STATUS_APLL_LOCK))
		pr_warn("%pOF: APLL is not locked\n", np);

	for (i = 0; i < ARRAY_SIZE(assumed); i++) {
		unsigned long rate = hi3516cv200_pll_rate(clk_data->base + assumed[i].offset,
							  HI3516CV200_XTAL_RATE);

		if (rate != assumed[i].rate)
			pr_warn("%pOF: %s runs at %lu Hz, the fixed module clocks assume %lu Hz\n",
				np, assumed[i].name, rate, assumed[i].rate);
		if (!(status & assumed[i].lock))
			pr_warn("%pOF: %s is not locked\n", np, assumed[i].name);
	}
}

/*
 * Selects that have reserved codes, or that the device tree assumes a value
 * for: the boot loader may leave one in a state the table cannot describe.
 * Say so instead of handing out a clock with no parent or a wrong rate.
 */
static void __init hi3516cv200_check_selects(struct device_node *np,
					     struct hisi_clock_data *clk_data)
{
	u32 mmc = readl(clk_data->base + 0xc4);
	u32 i2c = readl(clk_data->base + 0xe4);

	if (FIELD_GET(GENMASK(11, 10), mmc) > 2)
		pr_warn("%pOF: MMC0 clock select holds the reserved code %lu\n",
			np, FIELD_GET(GENMASK(11, 10), mmc));
	if (FIELD_GET(GENMASK(3, 2), mmc))
		pr_warn("%pOF: MMC1 clock select holds the reserved code %lu\n",
			np, FIELD_GET(GENMASK(3, 2), mmc));
	if (FIELD_GET(GENMASK(27, 26), i2c))
		pr_warn("%pOF: I2C clock select is %lu, the device tree gives the I2C controllers the APB clock\n",
			np, FIELD_GET(GENMASK(27, 26), i2c));
}

/*
 * Rates of the module clock selects as listed by the data sheet. They are
 * dividers of the VPLL (VCO 1188 MHz at reset: 198, 148.5, 99, 74.25 and
 * 49.5 MHz) and the EPLL (VCO 1000 MHz: 125 and 50 MHz) that are not modeled.
 */
static const struct hisi_fixed_rate_clock hi3516cv200_fixed_rate_clks[] = {
	{ HI3516CV200_FIXED_3M, "3m", NULL, 0, 3000000, },
	{ HI3516CV200_FIXED_6M, "6m", NULL, 0, 6000000, },
	{ HI3516CV200_FIXED_24M, "24m", NULL, 0, HI3516CV200_XTAL_RATE, },
	{ HI3516CV200_FIXED_25M, "25m", NULL, 0, 25000000, },
	{ HI3516CV200_FIXED_27M, "27m", NULL, 0, 27000000, },
	{ HI3516CV200_FIXED_37P125M, "37.125m", NULL, 0, 37125000, },
	{ HI3516CV200_FIXED_49P5M, "49.5m", NULL, 0, 49500000, },
	{ HI3516CV200_FIXED_50M, "50m", NULL, 0, 50000000, },
	{ HI3516CV200_FIXED_54M, "54m", NULL, 0, 54000000, },
	{ HI3516CV200_FIXED_74P25M, "74.25m", NULL, 0, 74250000, },
	{ HI3516CV200_FIXED_99M, "99m", NULL, 0, 99000000, },
	{ HI3516CV200_FIXED_125M, "125m", NULL, 0, 125000000, },
	{ HI3516CV200_FIXED_148P5M, "148.5m", NULL, 0, 148500000, },
	{ HI3516CV200_FIXED_198M, "198m", NULL, 0, 198000000, },
};

/*
 * PERI_CRG12: cpu_sc_sel [8] (0 = 24 MHz, 1 = APLL POSTDIV) and bus_sc_sel [0]
 * (0 = 24 MHz, 1 = 198 MHz). Both are the boot loader's choice and read only
 * here: a switch needs the PLL lock and the clk_cpu_seled / clk_bus_seled
 * handshake of PERI_CRG55, and the whole chip hangs off the bus mux. The bus
 * mux is what the APB clock below follows.
 */
static const char *const cpu_mux_p[] = { "24m", "apll" };
static const char *const bus_mux_p[] = { "24m", "198m" };

static const struct hisi_mux_clock hi3516cv200_core_mux_clks[] = {
	{ HI3516CV200_CPU_MUX, "cpu_mux", cpu_mux_p, ARRAY_SIZE(cpu_mux_p),
		0, 0x30, 8, 1, CLK_MUX_READ_ONLY, NULL, },
	{ HI3516CV200_BUS_MUX, "bus_mux", bus_mux_p, ARRAY_SIZE(bus_mux_p),
		0, 0x30, 0, 1, CLK_MUX_READ_ONLY, NULL, },
};

/* System APB = bus / 4 (49.5 MHz at 198 MHz); GPIO, timers, watchdog and RTC run from it */
static const struct hisi_fixed_factor_clock hi3516cv200_fixed_factor_clks[] = {
	{ HI3516CV200_SYSAPB_CLK, "apb", "bus_mux", 1, 4, 0, },
};

/* PERI_CRG57 uart_cksel [20:19] and i2c_cksel [27:26]: APB, 24 MHz, 6 MHz (3 is an alias of 2) */
static const char *const uart_mux_p[] = { "apb", "24m", "6m", "6m" };
static const char *const i2c_mux_p[] = { "apb", "24m", "6m", "6m" };
/* PERI_CRG48 sfc_cksel [3:2]: 24 MHz, 148.5 MHz, 125 MHz, 198 MHz */
static const char *const fmc_mux_p[] = { "24m", "148.5m", "125m", "198m" };
/*
 * PERI_CRG49 sdxc0_cksel [11:10]: 99, 74.25, 49.5 MHz; sdxc1_cksel [3:2]: 49.5 only.
 * The data sheet marks code 3 of sdxc0_cksel (and codes 1 to 3 of sdxc1_cksel)
 * reserved. The vendor U-Boot header defines SDIO0_CLK_SEL_148_5M for code 3,
 * but no vendor code selects it (the boot code programs 49.5 MHz) and the data
 * sheet does not back it, so a select that holds it has no parent here and the
 * select check below says so.
 */
static const char *const mmc0_mux_p[] = { "99m", "74.25m", "49.5m" };
static const char *const mmc1_mux_p[] = { "49.5m" };
/* PERI_CRG14 pwm_cksel [3:2]: 3 MHz, 50 MHz, 24 MHz (3 is an alias of 2) */
static const char *const pwm_mux_p[] = { "3m", "50m", "24m", "24m" };
/* PERI_CRG59 ethcore_cksel [7] */
static const char *const eth_core_mux_p[] = { "99m", "54m" };
/* PERI_CRG11 sensor_cksel [18:16] (sensor MCLK): 1X0 is 24 MHz, 1X1 is 25 MHz */
static const char *const sensor_mux_p[] = {
	"74.25m", "37.125m", "54m", "27m", "24m", "25m", "24m", "25m"
};

/* PERI_CRG11 vi0_sc_sel [2] */
static const char *const vi_mux_p[] = { "99m", "148.5m" };

static const struct hisi_mux_clock hi3516cv200_mux_clks[] = {
	{ HI3516CV200_UART_MUX, "uart_mux", uart_mux_p, ARRAY_SIZE(uart_mux_p),
		0, 0xe4, 19, 2, 0, NULL, },
	{ HI3516CV200_I2C_MUX, "i2c_mux", i2c_mux_p, ARRAY_SIZE(i2c_mux_p),
		0, 0xe4, 26, 2, 0, NULL, },
	{ HI3516CV200_FMC_MUX, "fmc_mux", fmc_mux_p, ARRAY_SIZE(fmc_mux_p),
		0, 0xc0, 2, 2, 0, NULL, },
	{ HI3516CV200_MMC0_MUX, "mmc0_mux", mmc0_mux_p, ARRAY_SIZE(mmc0_mux_p),
		0, 0xc4, 10, 2, 0, NULL, },
	{ HI3516CV200_MMC1_MUX, "mmc1_mux", mmc1_mux_p, ARRAY_SIZE(mmc1_mux_p),
		0, 0xc4, 2, 2, 0, NULL, },
	{ HI3516CV200_PWM_MUX, "pwm_mux", pwm_mux_p, ARRAY_SIZE(pwm_mux_p),
		0, 0x38, 2, 2, 0, NULL, },
	{ HI3516CV200_ETH_CORE_MUX, "eth_core_mux", eth_core_mux_p,
		ARRAY_SIZE(eth_core_mux_p), 0, 0xec, 7, 1, 0, NULL, },
	{ HI3516CV200_SENSOR_MUX, "sensor_mux", sensor_mux_p, ARRAY_SIZE(sensor_mux_p),
		0, 0x2c, 16, 3, 0, NULL, },
	{ HI3516CV200_VI_MUX, "vi_mux", vi_mux_p, ARRAY_SIZE(vi_mux_p),
		0, 0x2c, 2, 1, 0, NULL, },
};

/*
 * Clock gates (set = enabled). Blocks whose input rate the data sheet does not
 * state have no parent and report 0 Hz. The I2C blocks have no gate. The soft
 * reset bits of the same registers belong to the reset controller. The gates
 * behind a module clock select pass a rate request on to the select, so that
 * clk_set_rate() of a consumer (the SPI NOR driver of the FMC, an MMC host)
 * picks the closest input. A gate nobody uses is switched off by the clock
 * core after boot: none of them is needed by the kernel itself.
 */
static const struct hisi_gate_clock hi3516cv200_gate_clks[] = {
	/* PERI_CRG57 */
	{ HI3516CV200_UART0_CLK, "clk_uart0", "uart_mux", 0, 0xe4, 15, 0, },
	{ HI3516CV200_UART1_CLK, "clk_uart1", "uart_mux", 0, 0xe4, 16, 0, },
	{ HI3516CV200_UART2_CLK, "clk_uart2", "uart_mux", 0, 0xe4, 17, 0, },
	{ HI3516CV200_SPI0_CLK, "clk_spi0", "apb", 0, 0xe4, 13, 0, },
	{ HI3516CV200_SPI1_CLK, "clk_spi1", "apb", 0, 0xe4, 14, 0, },
	{ HI3516CV200_IR_CLK, "clk_ir", NULL, 0, 0xe4, 12, 0, },
	/* PERI_CRG48, PERI_CRG49, PERI_CRG59 */
	{ HI3516CV200_FMC_CLK, "clk_fmc", "fmc_mux", CLK_SET_RATE_PARENT, 0xc0, 1, 0, },
	{ HI3516CV200_MMC0_CLK, "clk_mmc0", "mmc0_mux", CLK_SET_RATE_PARENT, 0xc4, 9, 0, },
	{ HI3516CV200_MMC1_CLK, "clk_mmc1", "mmc1_mux", CLK_SET_RATE_PARENT, 0xc4, 1, 0, },
	{ HI3516CV200_ETH_CLK, "clk_eth", "eth_core_mux", CLK_SET_RATE_PARENT, 0xec, 1, 0, },
	/* PERI_CRG46 usb_cken; the USB resets are bits 0, 1, 2, 5 and 8 of the same register */
	{ HI3516CV200_USB2_CLK, "clk_usb", NULL, 0, 0xb8, 7, 0, },
	/* PERI_CRG54 dma_cken, PERI_CRG31 cipher_cken, PERI_CRG14 pwm_cken */
	{ HI3516CV200_DMAC_CLK, "clk_dmac", NULL, 0, 0xd8, 5, 0, },
	{ HI3516CV200_CIPHER_CLK, "clk_cipher", NULL, 0, 0x7c, 1, 0, },
	{ HI3516CV200_PWM_CLK, "clk_pwm", "pwm_mux", CLK_SET_RATE_PARENT, 0x38, 1, 0, },
	/* PERI_CRG11 sensor_cken [19], vi0_cken [0], mipi_cken [14] */
	{ HI3516CV200_SENSOR_CLK, "clk_sensor", "sensor_mux", 0, 0x2c, 19, 0, },
	{ HI3516CV200_VI_CLK, "clk_vi", "vi_mux", 0, 0x2c, 0, 0, },
	{ HI3516CV200_MIPI_CLK, "clk_mipi", NULL, 0, 0x2c, 14, 0, },
	/* media blocks: cken is bit 1 of PERI_CRG22/23/24/18/35, bit 13 of PERI_CRG16 */
	{ HI3516CV200_TDE_CLK, "clk_tde", NULL, 0, 0x58, 1, 0, },
	{ HI3516CV200_VGS_CLK, "clk_vgs", NULL, 0, 0x5c, 1, 0, },
	{ HI3516CV200_JPGE_CLK, "clk_jpge", NULL, 0, 0x60, 1, 0, },
	{ HI3516CV200_VEDU_CLK, "clk_vedu", NULL, 0, 0x40, 13, 0, },
	{ HI3516CV200_VPSS_CLK, "clk_vpss", NULL, 0, 0x48, 1, 0, },
	{ HI3516CV200_AIAO_CLK, "clk_aiao", NULL, 0, 0x8c, 1, 0, },
};

static void __init hi3516cv200_crg_clk_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;

	clk_data = hisi_clk_init(np, HI3516CV200_CRG_NR_CLKS);
	if (!clk_data)
		return;

	/* each helper cleans up after itself when its own group fails */
	if (hisi_clk_register_fixed_rate(hi3516cv200_fixed_rate_clks,
					 ARRAY_SIZE(hi3516cv200_fixed_rate_clks), clk_data))
		goto err_provider;
	if (hi3516cv200_register_plls(clk_data))
		goto err_fixed_rate;
	if (hisi_clk_register_mux(hi3516cv200_core_mux_clks,
				  ARRAY_SIZE(hi3516cv200_core_mux_clks), clk_data))
		goto err_plls;
	if (hisi_clk_register_fixed_factor(hi3516cv200_fixed_factor_clks,
					   ARRAY_SIZE(hi3516cv200_fixed_factor_clks), clk_data))
		goto err_core_mux;
	if (hisi_clk_register_mux(hi3516cv200_mux_clks,
				  ARRAY_SIZE(hi3516cv200_mux_clks), clk_data))
		goto err_fixed_factor;
	if (hisi_clk_register_gate(hi3516cv200_gate_clks,
				   ARRAY_SIZE(hi3516cv200_gate_clks), clk_data))
		goto err_mux;

	hi3516cv200_check_plls(np, clk_data);
	hi3516cv200_check_selects(np, clk_data);

	return;

err_mux:
	hisi_clk_unregister_mux(hi3516cv200_mux_clks,
				ARRAY_SIZE(hi3516cv200_mux_clks), clk_data);
err_fixed_factor:
	hisi_clk_unregister_fixed_factor(hi3516cv200_fixed_factor_clks,
					 ARRAY_SIZE(hi3516cv200_fixed_factor_clks), clk_data);
err_core_mux:
	hisi_clk_unregister_mux(hi3516cv200_core_mux_clks,
				ARRAY_SIZE(hi3516cv200_core_mux_clks), clk_data);
err_plls:
	hi3516cv200_unregister_plls(clk_data, ARRAY_SIZE(hi3516cv200_plls));
err_fixed_rate:
	hisi_clk_unregister_fixed_rate(hi3516cv200_fixed_rate_clks,
				       ARRAY_SIZE(hi3516cv200_fixed_rate_clks), clk_data);
err_provider:
	pr_err("%pOF: failed to register all clocks\n", np);
	/* no provider at all is better than one with missing clocks */
	of_clk_del_provider(np);
	iounmap(clk_data->base);
	kfree(clk_data->clk_data.clks);
	kfree(clk_data);
}

CLK_OF_DECLARE_DRIVER(hi3518ev200_crg, "hisilicon,hi3518ev200-crg", hi3516cv200_crg_clk_init);
/* deprecated alias, still used by hi3516cv200.dtsi until the device tree patch of this series */
CLK_OF_DECLARE_DRIVER(hi3518ev20x_crg, "hisilicon,hi3518ev20x-clock", hi3516cv200_crg_clk_init);

/* ---------------- sys controller block (sysctrl@20050000) ---------------- */

/*
 * The SP804 count clocks are selected by SC_CTRL (sysctrl + 0x0): bit 16 (timer
 * 0), 18 (timer 1), 20 (timer 2) and 22 (timer 3) choose between the 3 MHz
 * reference (0) and the system APB clock (1). The boot loader may leave either
 * value (the Hi3518E V200 U-Boot sets bit 16), so model the select as a
 * read-only mux (SC_CTRL is write-protected by SC_LOCKEN) and let the read-back
 * of the register decide the rate. Timer init runs at
 * start_kernel, long before platform driver probing, so the mux is registered
 * from the sysctrl early init; its parents come from the CRG early init, which
 * comes first in the device tree.
 */
static const char *const hi3516cv200_timer_mux_p[] = { "3m", "apb" };
static u32 hi3516cv200_timer_mux_table[] = { 0, 1 };

static const struct hisi_mux_clock hi3516cv200_sysctrl_mux[] = {
	{ HI3516CV200_TIME0_0_CLK, "timer0_0", hi3516cv200_timer_mux_p,
		ARRAY_SIZE(hi3516cv200_timer_mux_p), 0, 0x0, 16, 1,
		CLK_MUX_READ_ONLY,
		hi3516cv200_timer_mux_table, },
	{ HI3516CV200_TIME0_1_CLK, "timer0_1", hi3516cv200_timer_mux_p,
		ARRAY_SIZE(hi3516cv200_timer_mux_p), 0, 0x0, 18, 1,
		CLK_MUX_READ_ONLY,
		hi3516cv200_timer_mux_table, },
	{ HI3516CV200_TIME1_2_CLK, "timer1_2", hi3516cv200_timer_mux_p,
		ARRAY_SIZE(hi3516cv200_timer_mux_p), 0, 0x0, 20, 1,
		CLK_MUX_READ_ONLY,
		hi3516cv200_timer_mux_table, },
	{ HI3516CV200_TIME1_3_CLK, "timer1_3", hi3516cv200_timer_mux_p,
		ARRAY_SIZE(hi3516cv200_timer_mux_p), 0, 0x0, 22, 1,
		CLK_MUX_READ_ONLY,
		hi3516cv200_timer_mux_table, },
};

static void __init hi3516cv200_sysctrl_clk_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;

	clk_data = hisi_clk_init(np, HI3516CV200_SC_NR_CLKS);
	if (!clk_data)
		return;

	if (hisi_clk_register_mux(hi3516cv200_sysctrl_mux,
				  ARRAY_SIZE(hi3516cv200_sysctrl_mux), clk_data)) {
		pr_err("%pOF: failed to register the timer clocks\n", np);
		/* no provider at all is better than one with missing timer clocks */
		of_clk_del_provider(np);
		iounmap(clk_data->base);
		kfree(clk_data->clk_data.clks);
		kfree(clk_data);
	}
}

CLK_OF_DECLARE(hi3518ev200_sysctrl, "hisilicon,hi3518ev200-sysctrl",
	       hi3516cv200_sysctrl_clk_init);
/* deprecated alias, as above */
CLK_OF_DECLARE(hi3518ev20x_sysctrl, "hisilicon,hi3518ev20x-sysctrl",
	       hi3516cv200_sysctrl_clk_init);

/* ---------------- platform driver (reset controller) ---------------- */

static int hi3516cv200_crg_probe(struct platform_device *pdev)
{
	/* the gates in this register block share it with the reset bits */
	return PTR_ERR_OR_ZERO(hisi_reset_init_shared(pdev, hisi_clk_get_lock()));
}

static const struct of_device_id hi3516cv200_crg_match_table[] = {
	{ .compatible = "hisilicon,hi3518ev200-crg" },
	{ .compatible = "hisilicon,hi3518ev20x-clock" },	/* deprecated alias */
	{ }
};
MODULE_DEVICE_TABLE(of, hi3516cv200_crg_match_table);

static struct platform_driver hi3516cv200_crg_driver = {
	.probe = hi3516cv200_crg_probe,
	.driver = {
		.name = "hi3516cv200-crg",
		.of_match_table = hi3516cv200_crg_match_table,
		.suppress_bind_attrs = true,
	},
};
builtin_platform_driver(hi3516cv200_crg_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HiSilicon Hi3516C V200 / Hi3518E V20x CRG driver");
