// SPDX-License-Identifier: GPL-2.0-only
/*
 * HiSilicon Hi3520D family clock and reset generator (CRG) and system
 * controller clocks.
 *
 * The SP804 timer and watchdog count clock selects live in the system
 * controller, so it is a second clock provider. Both providers register
 * early (CLK_OF_DECLARE) because the SP804 clocksource is probed from
 * time_init(), long before platform drivers run. Reset lines use the generic
 * HiSilicon reset controller (offset, bit specifiers) from a platform driver.
 *
 * The APLL is only read: the A9 and the bus clocks run from it, so the
 * bootloader's setting stays. The EPLL outputs (50, 75 and 112.5 MHz) are fixed
 * rates.
 *
 * Register descriptions: https://hisilicon.randomcpu.com/?p=hi3520dv100.CRG
 * and https://hisilicon.randomcpu.com/?p=hi3520dv100.SYS_CTRL
 */

#include <dt-bindings/clock/hi3520d-clock.h>
#include <linux/bitfield.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "clk.h"
#include "reset.h"

#define HI3520D_CRG_NR_CLKS		80

/* internal clock IDs (not in the binding): VICAP channel clock dividers */
#define HI3520D_VICAP_DIV0_CLK		70
#define HI3520D_VICAP_DIV1_CLK		71
#define HI3520D_VICAP_DIV4_CLK		72
#define HI3520D_VICAP_DIV5_CLK		73
#define HI3520D_SYSCTRL_NR_CLKS		16

#define HI3520D_XTAL_RATE		24000000

/* PERI_CRG0/1: APLL, read only */
#define CRG_APLL_A			0x00
#define CRG_APLL_B			0x04
#define APLL_BYPASS			BIT(30)
#define APLL_POSTDIV2			GENMASK(29, 27)
#define APLL_POSTDIV1			GENMASK(26, 24)
#define APLL_FRAC			GENMASK(23, 0)
#define APLL_DSMPD			BIT(22)
#define APLL_REFDIV			GENMASK(17, 12)
#define APLL_FBDIV			GENMASK(11, 0)

struct hi3520d_apll {
	struct clk_hw hw;
	void __iomem *base;
};

#define to_hi3520d_apll(_hw)	container_of(_hw, struct hi3520d_apll, hw)

/*
 * FOUTVCO = Fin * (fbdiv + frac / 2^24) / refdiv, FOUTPOSTDIV = VCO / (div1 * div2).
 * The A9 and the bus clocks run from it; the bootloader sets it, so it is only
 * read here.
 */
static unsigned long hi3520d_apll_recalc_rate(struct clk_hw *hw,
					      unsigned long parent_rate)
{
	struct hi3520d_apll *apll = to_hi3520d_apll(hw);
	u32 a = readl(apll->base + CRG_APLL_A);
	u32 b = readl(apll->base + CRG_APLL_B);
	u32 refdiv = FIELD_GET(APLL_REFDIV, b);
	u32 div1 = FIELD_GET(APLL_POSTDIV1, a);
	u32 div2 = FIELD_GET(APLL_POSTDIV2, a);
	u64 vco;

	if ((a & APLL_BYPASS) || !refdiv || !div1 || !div2)
		return parent_rate;

	vco = (u64)parent_rate * FIELD_GET(APLL_FBDIV, b);
	if (!(b & APLL_DSMPD))
		vco += ((u64)parent_rate * FIELD_GET(APLL_FRAC, a)) >> 24;

	return div_u64(vco, refdiv * div1 * div2);
}

static const struct clk_ops hi3520d_apll_ops = {
	.recalc_rate = hi3520d_apll_recalc_rate,
};

static int __init hi3520d_register_apll(struct hisi_clock_data *clk_data)
{
	static const char *const parent_names[] = { "24m" };
	struct clk_init_data init = {
		.name = "apll",
		.ops = &hi3520d_apll_ops,
		.parent_names = parent_names,
		.num_parents = 1,
	};
	struct hi3520d_apll *apll;
	struct clk *clk;

	apll = kzalloc_obj(*apll);
	if (!apll)
		return -ENOMEM;

	apll->base = clk_data->base;
	apll->hw.init = &init;
	clk = clk_register(NULL, &apll->hw);
	if (IS_ERR(clk)) {
		kfree(apll);
		return PTR_ERR(clk);
	}

	clk_data->clk_data.clks[HI3520D_APLL_CLK] = clk;
	return 0;
}

static const struct hisi_fixed_rate_clock hi3520d_fixed_rate_clks[] = {
	{ HI3520D_FIXED_3M, "3m", NULL, 0, 3000000, },
	{ HI3520D_FIXED_2M, "2m", NULL, 0, 2000000, },
	{ HI3520D_FIXED_24M, "24m", NULL, 0, HI3520D_XTAL_RATE, },
	/* EPLL derived (VCO 1350 MHz at reset, EPLL is not tracked) */
	{ HI3520D_FIXED_50M, "50m", NULL, 0, 50000000, },
	{ HI3520D_FIXED_75M, "75m", NULL, 0, 75000000, },
	{ HI3520D_FIXED_112P5M, "112.5m", NULL, 0, 112500000, },
};

/* PERI_CRG10[3:2] freqmode_a9_sys: 0b11 gives A9:AXI = 1:1, anything else 2:1 */
static struct clk_div_table hi3520d_axi_div_table[] = {
	{ 0, 2 }, { 1, 2 }, { 2, 2 }, { 3, 1 }, { }
};

/*
 * PERI_CRG11 vichN_divsel (N = 0, 1, 4, 5): 0b00 = /2, 0b01 = /4, 0b10 = /1
 * (vendor hi3520D_sys.ko codes; the data sheet only lists /2 and /4). The
 * input is the pad clock, or the on-chip 24 MHz test clock with vi_selftest
 * set; the model takes the 24 MHz one, which is the only rate the driver
 * uses it for.
 */
static struct clk_div_table hi3520d_vicap_div_table[] = {
	{ 0, 2 }, { 1, 4 }, { 2, 1 }, { }
};

static const struct hisi_divider_clock hi3520d_divider_clks[] = {
	{ HI3520D_AXI_CLK, "axi", "apll", 0, 0x28, 2, 2, 0,
		hi3520d_axi_div_table, },
	{ HI3520D_VICAP_DIV0_CLK, "clk_vicap_div0", "24m", 0, 0x2c, 30, 2, 0,
		hi3520d_vicap_div_table, },
	{ HI3520D_VICAP_DIV1_CLK, "clk_vicap_div1", "24m", 0, 0x2c, 28, 2, 0,
		hi3520d_vicap_div_table, },
	{ HI3520D_VICAP_DIV4_CLK, "clk_vicap_div4", "24m", 0, 0x2c, 26, 2, 0,
		hi3520d_vicap_div_table, },
	{ HI3520D_VICAP_DIV5_CLK, "clk_vicap_div5", "24m", 0, 0x2c, 24, 2, 0,
		hi3520d_vicap_div_table, },
};

/* AXI:APB is 4:1 */
static const struct hisi_fixed_factor_clock hi3520d_fixed_factor_clks[] = {
	{ HI3520D_APB_CLK, "apb", "axi", 1, 4, CLK_SET_RATE_PARENT, },
};

/*
 * UART reference: PERI_CRG57[14], 0 = APB, 1 = 2 MHz. The data sheet has this
 * inverted; measured on hardware.
 */
static const char *const uart_mux_p[] = { "apb", "2m" };
/*
 * SFC2X source, PERI_CRG48[3:2]: 0b00 = 24 MHz, 0b01 = 75 MHz, 0b10 = 24 MHz
 * (alias). 0b11 is reserved in the data sheet; the boot code leaves it
 * selected (observed after boot, flash works), rate unverified, modelled as
 * 112.5 MHz.
 */
static const char *const sfc_mux_p[] = { "24m", "75m", "24m", "112.5m" };
static u32 sfc_mux_table[] = { 0, 1, 2, 3 };
static const char *const pwm_mux_p[] = { "3m", "24m" };

static const struct hisi_mux_clock hi3520d_mux_clks[] = {
	{ HI3520D_UART_MUX, "uart_mux", uart_mux_p, ARRAY_SIZE(uart_mux_p),
		0, 0xe4, 14, 1, 0, NULL, },
	{ HI3520D_SFC_MUX, "sfc_mux", sfc_mux_p, ARRAY_SIZE(sfc_mux_p),
		0, 0xc0, 2, 2, 0, sfc_mux_table, },
	{ HI3520D_PWM_MUX, "pwm_mux", pwm_mux_p, ARRAY_SIZE(pwm_mux_p),
		0, 0x38, 2, 1, 0, NULL, },
};

/*
 * Clock gates (set = enabled). Blocks whose input rate is not modelled have
 * no parent and report 0 Hz: clk_eth, clk_sata_phy/0/1, clk_scd, clk_efuse,
 * clk_ddrtest, clk_cipher, the VOU/HDMI/VEDU/SED/VPSS/TDE/JPGE/JPGD/VAPU/
 * VOIE/VCMP gates, the VICAP gates and the AIO gates (clk_aio_h, clk_aip,
 * clk_aop0/1).
 */
static const struct hisi_gate_clock hi3520d_gate_clks[] = {
	{ HI3520D_UART0_CLK, "clk_uart0", "uart_mux", 0, 0xe4, 15, 0, },
	{ HI3520D_UART1_CLK, "clk_uart1", "uart_mux", 0, 0xe4, 16, 0, },
	{ HI3520D_UART2_CLK, "clk_uart2", "uart_mux", 0, 0xe4, 17, 0, },
	{ HI3520D_UART3_CLK, "clk_uart3", "uart_mux", 0, 0xe4, 18, 0, },
	{ HI3520D_SSP_CLK, "clk_ssp", "apb", 0, 0xe4, 13, 0, },
	{ HI3520D_IR_CLK, "clk_ir", "apb", 0, 0xe4, 12, 0, },
	/*
	 * The boot flash holds the root file system and the SFC driver does
	 * not claim this clock yet, so the unused-clock cleanup must not stop it.
	 */
	{ HI3520D_SFC_CLK, "clk_sfc", "sfc_mux", CLK_IS_CRITICAL, 0xc0, 1, 0, },
	{ HI3520D_ETH_CLK, "clk_eth", NULL, 0, 0xcc, 1, 0, },
	{ HI3520D_USB_CLK, "clk_usb", "24m", 0, 0xb8, 7, 0, },
	/* sata_cken: PHY and bus clock of the SATA block */
	{ HI3520D_SATA_CLK, "clk_sata", NULL, 0, 0xb4, 10, 0, },
	{ HI3520D_SATA0_CLK, "clk_sata0", NULL, 0, 0xb4, 9, 0, },
	{ HI3520D_SATA1_CLK, "clk_sata1", NULL, 0, 0xb4, 8, 0, },
	{ HI3520D_PWM_CLK, "clk_pwm", "pwm_mux", 0, 0x38, 1, 0, },
	{ HI3520D_SCD_CLK, "clk_scd", NULL, 0, 0xd4, 3, 0, },
	{ HI3520D_EFUSE_CLK, "clk_efuse", NULL, 0, 0xd8, 1, 0, },
	{ HI3520D_DDRTEST_CLK, "clk_ddrtest", NULL, 0, 0xd8, 3, 0, },
	{ HI3520D_CIPHER_CLK, "clk_cipher", NULL, 0, 0x7c, 1, 0, },
	{ HI3520D_VOU_H_CLK, "clk_vou_h", NULL, 0, 0x34, 8, 0, },
	{ HI3520D_VOU_HD_CLK, "clk_vou_hd", NULL, 0, 0x34, 9, 0, },
	{ HI3520D_VOU_SD0_CLK, "clk_vou_sd0", NULL, 0, 0x34, 10, 0, },
	{ HI3520D_VOU_SD1_CLK, "clk_vou_sd1", NULL, 0, 0x34, 11, 0, },
	{ HI3520D_HDDAC_CLK, "clk_hddac", NULL, 0, 0x34, 6, 0, },
	{ HI3520D_SDDAC_CLK, "clk_sddac", NULL, 0, 0x34, 7, 0, },
	{ HI3520D_HDMI_H_CLK, "clk_hdmi_h", NULL, 0, 0x3c, 2, 0, },
	{ HI3520D_HDMI_ID_CLK, "clk_hdmi_id", NULL, 0, 0x3c, 3, 0, },
	{ HI3520D_VEDU0_CLK, "clk_vedu0", NULL, 0, 0x40, 1, 0, },
	{ HI3520D_SED_CLK, "clk_sed", NULL, 0, 0x40, 11, 0, },
	{ HI3520D_VPSS_CLK, "clk_vpss", NULL, 0, 0x48, 1, 0, },
	{ HI3520D_TDE_CLK, "clk_tde", NULL, 0, 0x58, 1, 0, },
	{ HI3520D_JPGE_CLK, "clk_jpge", NULL, 0, 0x60, 1, 0, },
	{ HI3520D_JPGD_CLK, "clk_jpgd", NULL, 0, 0x64, 1, 0, },
	/* no select or divider in PERI_CRG26: fixed EPLL VCO / 12 (data sheet table 3-3) */
	{ HI3520D_MDU_CLK, "clk_mdu", "112.5m", 0, 0x68, 1, 0, },
	{ HI3520D_VAPU_CLK, "clk_vapu", NULL, 0, 0x6c, 1, 0, },
	{ HI3520D_VOIE_CLK, "clk_voie", NULL, 0, 0x70, 1, 0, },
	{ HI3520D_VCMP_CLK, "clk_vcmp", NULL, 0, 0x74, 1, 0, },
	{ HI3520D_AIO_H_CLK, "clk_aio_h", NULL, 0, 0x8c, 1, 0, },
	{ HI3520D_AIP_CLK, "clk_aip", NULL, 0, 0x90, 1, 0, },
	{ HI3520D_AOP0_CLK, "clk_aop0", NULL, 0, 0x94, 1, 0, },
	{ HI3520D_AOP1_CLK, "clk_aop1", NULL, 0, 0x98, 1, 0, },
	/* PERI_CRG11: vi_hcken, per-channel vi_cken_ch[15:8] */
	{ HI3520D_VICAP_H_CLK, "clk_vicap_h", NULL, 0, 0x2c, 1, 0, },
	{ HI3520D_VICAP_CH0_CLK, "clk_vicap_ch0", "clk_vicap_div0",
		CLK_SET_RATE_PARENT, 0x2c, 8, 0, },
	{ HI3520D_VICAP_CH1_CLK, "clk_vicap_ch1", "clk_vicap_div1",
		CLK_SET_RATE_PARENT, 0x2c, 9, 0, },
	{ HI3520D_VICAP_CH2_CLK, "clk_vicap_ch2", NULL, 0, 0x2c, 10, 0, },
	{ HI3520D_VICAP_CH3_CLK, "clk_vicap_ch3", NULL, 0, 0x2c, 11, 0, },
	{ HI3520D_VICAP_CH4_CLK, "clk_vicap_ch4", "clk_vicap_div4",
		CLK_SET_RATE_PARENT, 0x2c, 12, 0, },
	{ HI3520D_VICAP_CH5_CLK, "clk_vicap_ch5", "clk_vicap_div5",
		CLK_SET_RATE_PARENT, 0x2c, 13, 0, },
	{ HI3520D_VICAP_CH6_CLK, "clk_vicap_ch6", NULL, 0, 0x2c, 14, 0, },
	{ HI3520D_VICAP_CH7_CLK, "clk_vicap_ch7", NULL, 0, 0x2c, 15, 0, },
	/*
	 * vi_selftest: not a gate in hardware but the switch that feeds the
	 * VICAP from the on-chip 24 MHz test clock instead of the pad clocks
	 * (color bar generator). Modelled as a gate so that the capture driver
	 * can request it while it runs the test pattern.
	 */
	{ HI3520D_VICAP_TEST_CLK, "clk_vicap_test", "24m", 0, 0x2c, 22, 0, },
};

static void __init hi3520d_crg_clk_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;

	clk_data = hisi_clk_init(np, HI3520D_CRG_NR_CLKS);
	if (!clk_data)
		return;

	hisi_clk_register_fixed_rate(hi3520d_fixed_rate_clks,
				     ARRAY_SIZE(hi3520d_fixed_rate_clks), clk_data);

	if (hi3520d_register_apll(clk_data))
		pr_err("%s: failed to register the APLL\n", __func__);

	hisi_clk_register_divider(hi3520d_divider_clks,
				  ARRAY_SIZE(hi3520d_divider_clks), clk_data);
	hisi_clk_register_fixed_factor(hi3520d_fixed_factor_clks,
				       ARRAY_SIZE(hi3520d_fixed_factor_clks), clk_data);
	hisi_clk_register_mux(hi3520d_mux_clks,
			      ARRAY_SIZE(hi3520d_mux_clks), clk_data);
	hisi_clk_register_gate(hi3520d_gate_clks,
			       ARRAY_SIZE(hi3520d_gate_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(hi3520d_crg, "hisilicon,hi3520d-crg", hi3520d_crg_clk_init);

/*
 * SC_CTRL (offset 0): timerNov selects the SP804 count clock, 1 = APB, 0 = 3 MHz;
 * wdogenov (bit 23) does the same for the SP805.
 */
static const char *const sc_count_p[] = { "3m", "apb" };

#define SC_TIMER_MUX(_id, _name, _bit)					\
	{ _id, _name, sc_count_p, ARRAY_SIZE(sc_count_p), 0, 0x0, _bit, 1, 0, NULL, }

static const struct hisi_mux_clock hi3520d_sysctrl_mux_clks[] = {
	SC_TIMER_MUX(HI3520D_TIMER0_CLK, "timer0", 16),
	SC_TIMER_MUX(HI3520D_TIMER1_CLK, "timer1", 18),
	SC_TIMER_MUX(HI3520D_TIMER2_CLK, "timer2", 20),
	SC_TIMER_MUX(HI3520D_TIMER3_CLK, "timer3", 22),
	SC_TIMER_MUX(HI3520D_TIMER4_CLK, "timer4", 25),
	SC_TIMER_MUX(HI3520D_TIMER5_CLK, "timer5", 27),
	SC_TIMER_MUX(HI3520D_TIMER6_CLK, "timer6", 29),
	SC_TIMER_MUX(HI3520D_TIMER7_CLK, "timer7", 31),
	SC_TIMER_MUX(HI3520D_WDT_CLK, "wdt", 23),
};

static void __init hi3520d_sysctrl_clk_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;

	clk_data = hisi_clk_init(np, HI3520D_SYSCTRL_NR_CLKS);
	if (!clk_data)
		return;

	hisi_clk_register_mux(hi3520d_sysctrl_mux_clks,
			      ARRAY_SIZE(hi3520d_sysctrl_mux_clks), clk_data);
}

CLK_OF_DECLARE_DRIVER(hi3520d_sysctrl, "hisilicon,hi3520d-sysctrl",
		      hi3520d_sysctrl_clk_init);

static int hi3520d_crg_probe(struct platform_device *pdev)
{
	if (!hisi_reset_init(pdev))
		return -ENOMEM;

	return 0;
}

static const struct of_device_id hi3520d_crg_match_table[] = {
	{ .compatible = "hisilicon,hi3520d-crg" },
	{ }
};
MODULE_DEVICE_TABLE(of, hi3520d_crg_match_table);

static struct platform_driver hi3520d_crg_driver = {
	.probe = hi3520d_crg_probe,
	.driver = {
		.name = "hi3520d-crg",
		.of_match_table = hi3520d_crg_match_table,
		.suppress_bind_attrs = true,
	},
};
builtin_platform_driver(hi3520d_crg_driver);
