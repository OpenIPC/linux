// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hi3516CV200 / Hi3518EV20x Clock and Reset Generator Driver
 *
 * Copyright (c) 2016 HiSilicon Technologies Co., Ltd.
 *
 * Ported from the 4.9 vendor BSP (clk-hi3518ev20x.c) onto the modern
 * crg- platform_driver scaffold, mirroring crg-hi3516cv300.c. Keeps the
 * vendor DT (hi3518ev20x.dtsi / hi3516cv200.dtsi) compatible without
 * modification. V2 generation — ARM926EJ-S, ARMv5TE.
 *
 * SoC sibling list (all share this CRG layout): Hi3516CV200,
 * Hi3518EV200, Hi3518EV201.
 */

#include <dt-bindings/clock/hi3516cv200-clock.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include "clk.h"
#include "crg.h"
#include "reset.h"

/* ---------------- CRG block (hisilicon,hi3516cv200-clock) ---------------- */

static const struct hisi_fixed_rate_clock hi3516cv200_fixed_rate_clks[] = {
	{ HI3516CV200_FIXED_3M,      "3m",      NULL, 0, 3000000,    },
	{ HI3516CV200_FIXED_6M,      "6m",      NULL, 0, 6000000,    },
	{ HI3516CV200_FIXED_24M,     "24m",     NULL, 0, 24000000,   },
	{ HI3516CV200_FIXED_25M,     "25m",     NULL, 0, 25000000,   },
	{ HI3516CV200_FIXED_27M,     "27m",     NULL, 0, 27000000,   },
	{ HI3516CV200_FIXED_37P125M, "37.125m", NULL, 0, 37125000,   },
	{ HI3516CV200_FIXED_49P5M,   "49.5m",   NULL, 0, 49500000,   },
	{ HI3516CV200_FIXED_50M,     "50m",     NULL, 0, 50000000,   },
	{ HI3516CV200_FIXED_54M,     "54m",     NULL, 0, 54000000,   },
	{ HI3516CV200_FIXED_74P25M,  "74.25m",  NULL, 0, 74250000,   },
	{ HI3516CV200_FIXED_99M,     "99m",     NULL, 0, 99000000,   },
	{ HI3516CV200_FIXED_125M,    "125m",    NULL, 0, 125000000,  },
	{ HI3516CV200_FIXED_148P5M,  "148.5m",  NULL, 0, 148500000,  },
	{ HI3516CV200_FIXED_198M,    "198m",    NULL, 0, 198000000,  },
	{ HI3516CV200_FIXED_200M,    "200m",    NULL, 0, 200000000,  },
	{ HI3516CV200_FIXED_250M,    "250m",    NULL, 0, 250000000,  },
	{ HI3516CV200_FIXED_297M,    "297m",    NULL, 0, 297000000,  },
	{ HI3516CV200_FIXED_300M,    "300m",    NULL, 0, 300000000,  },
	{ HI3516CV200_FIXED_396M,    "396m",    NULL, 0, 396000000,  },
	{ HI3516CV200_FIXED_540M,    "540m",    NULL, 0, 540000000,  },
	{ HI3516CV200_FIXED_594M,    "594m",    NULL, 0, 594000000,  },
	{ HI3516CV200_FIXED_600M,    "600m",    NULL, 0, 600000000,  },
	{ HI3516CV200_FIXED_650M,    "650m",    NULL, 0, 650000000,  },
	{ HI3516CV200_FIXED_750M,    "750m",    NULL, 0, 750000000,  },
	{ HI3516CV200_FIXED_1188M,   "1188m",   NULL, 0, 1188000000, },
};

static const char *const uart_mux_p[] = { "24m", "6m" };
static const char *const fmc_mux_p[]  = { "24m", "74.25m", "125m", "148.5m", "198m" };
static const char *const mmc_mux_p[]  = { "49.5m" };
static const char *const eth_mux_p[]  = { "297m", "375m" };

static u32 uart_mux_table[] = { 0, 1 };
static u32 fmc_mux_table[]  = { 0, 1, 2, 3, 4 };
static u32 mmc_mux_table[]  = { 0 };
static u32 eth_mux_table[]  = { 0, 1 };

static const struct hisi_mux_clock hi3516cv200_mux_clks[] = {
	{ HI3516CV200_UART_MUX, "uart_mux", uart_mux_p, ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0xe4, 19, 1, 0, uart_mux_table, },
	{ HI3516CV200_FMC_MUX,  "fmc_mux",  fmc_mux_p,  ARRAY_SIZE(fmc_mux_p),
		CLK_SET_RATE_PARENT, 0xc0, 2,  3, 0, fmc_mux_table,  },
	{ HI3516CV200_MMC0_MUX, "mmc0_mux", mmc_mux_p,  ARRAY_SIZE(mmc_mux_p),
		CLK_SET_RATE_PARENT, 0xc4, 4,  2, 0, mmc_mux_table,  },
	{ HI3516CV200_MMC1_MUX, "mmc1_mux", mmc_mux_p,  ARRAY_SIZE(mmc_mux_p),
		CLK_SET_RATE_PARENT, 0xc4, 12, 2, 0, mmc_mux_table,  },
	{ HI3516CV200_ETH_MUX,  "eth_mux",  eth_mux_p,  ARRAY_SIZE(eth_mux_p),
		CLK_SET_RATE_PARENT, 0xec, 2,  1, 0, eth_mux_table,  },
};

static const struct hisi_divider_clock hi3516cv200_div_clks[] = {
	{ HI3516CV200_SYSAPB_CLK, "sysapb", "50m", 0, 0x30, 0, 1, 0, NULL, },
};

static const struct hisi_gate_clock hi3516cv200_gate_clks[] = {
	/* uart */
	{ HI3516CV200_UART0_CLK, "clk_uart0", "uart_mux", CLK_SET_RATE_PARENT, 0xe4, 15, 0, },
	{ HI3516CV200_UART1_CLK, "clk_uart1", "uart_mux", CLK_SET_RATE_PARENT, 0xe4, 16, 0, },
	{ HI3516CV200_UART2_CLK, "clk_uart2", "uart_mux", CLK_SET_RATE_PARENT, 0xe4, 17, 0, },
	{ HI3516CV200_UART3_CLK, "clk_uart3", "uart_mux", CLK_SET_RATE_PARENT, 0xe4, 18, 0, },
	/* spi */
	{ HI3516CV200_SPI0_CLK,  "clk_spi0",  "50m",      CLK_SET_RATE_PARENT, 0xe4, 13, 0, },
	{ HI3516CV200_SPI1_CLK,  "clk_spi1",  "50m",      CLK_SET_RATE_PARENT, 0xe4, 14, 0, },
	/* fmc + mmc */
	{ HI3516CV200_FMC_CLK,   "clk_fmc",   "fmc_mux",  CLK_SET_RATE_PARENT, 0xc0, 1,  0, },
	{ HI3516CV200_MMC0_CLK,  "clk_mmc0",  "mmc0_mux", CLK_SET_RATE_PARENT, 0xc4, 1,  0, },
	{ HI3516CV200_MMC1_CLK,  "clk_mmc1",  "mmc1_mux", CLK_SET_RATE_PARENT, 0xc4, 9,  0, },
	/* ethernet */
	{ HI3516CV200_ETH_CLK,   "clk_eth",   "eth_mux",  0,                   0xec, 1,  0, },
	/* dmac */
	{ HI3516CV200_DMAC_CLK,  "clk_dmac",  NULL,       0,                   0xd8, 5,  0, },
	/* usb (both UTMI and HRST gate bits) */
	{ HI3516CV200_USB2_CTRL_UTMI0_REQ, "clk_usb2_utmi0_req", NULL,
		CLK_SET_RATE_PARENT, 0xb8, 11, 1, },
	{ HI3516CV200_USB2_HRST_REQ,       "clk_usb2_hrst_req",  NULL,
		CLK_SET_RATE_PARENT, 0xb8, 12, 1, },
};

static struct hisi_clock_data *hi3516cv200_clk_register(struct platform_device *pdev)
{
	struct hisi_clock_data *clk_data;
	int ret;

	clk_data = hisi_clk_alloc(pdev, HI3516CV200_CRG_NR_CLKS);
	if (!clk_data)
		return ERR_PTR(-ENOMEM);

	ret = hisi_clk_register_fixed_rate(hi3516cv200_fixed_rate_clks,
			ARRAY_SIZE(hi3516cv200_fixed_rate_clks), clk_data);
	if (ret)
		return ERR_PTR(ret);

	ret = hisi_clk_register_mux(hi3516cv200_mux_clks,
			ARRAY_SIZE(hi3516cv200_mux_clks), clk_data);
	if (ret)
		goto unregister_fixed_rate;

	ret = hisi_clk_register_divider(hi3516cv200_div_clks,
			ARRAY_SIZE(hi3516cv200_div_clks), clk_data);
	if (ret)
		goto unregister_mux;

	ret = hisi_clk_register_gate(hi3516cv200_gate_clks,
			ARRAY_SIZE(hi3516cv200_gate_clks), clk_data);
	if (ret)
		goto unregister_divider;

	ret = of_clk_add_provider(pdev->dev.of_node,
			of_clk_src_onecell_get, &clk_data->clk_data);
	if (ret)
		goto unregister_gate;

	return clk_data;

unregister_gate:
	hisi_clk_unregister_gate(hi3516cv200_gate_clks,
			ARRAY_SIZE(hi3516cv200_gate_clks), clk_data);
unregister_divider:
	/* hisi_clk_unregister_divider() helper not exported; rely on probe cleanup. */
unregister_mux:
	hisi_clk_unregister_mux(hi3516cv200_mux_clks,
			ARRAY_SIZE(hi3516cv200_mux_clks), clk_data);
unregister_fixed_rate:
	hisi_clk_unregister_fixed_rate(hi3516cv200_fixed_rate_clks,
			ARRAY_SIZE(hi3516cv200_fixed_rate_clks), clk_data);
	return ERR_PTR(ret);
}

static void hi3516cv200_clk_unregister(struct platform_device *pdev)
{
	struct hisi_crg_dev *crg = platform_get_drvdata(pdev);

	of_clk_del_provider(pdev->dev.of_node);

	hisi_clk_unregister_gate(hi3516cv200_gate_clks,
			ARRAY_SIZE(hi3516cv200_gate_clks), crg->clk_data);
	hisi_clk_unregister_mux(hi3516cv200_mux_clks,
			ARRAY_SIZE(hi3516cv200_mux_clks), crg->clk_data);
	hisi_clk_unregister_fixed_rate(hi3516cv200_fixed_rate_clks,
			ARRAY_SIZE(hi3516cv200_fixed_rate_clks), crg->clk_data);
}

static const struct hisi_crg_funcs hi3516cv200_crg_funcs = {
	.register_clks = hi3516cv200_clk_register,
	.unregister_clks = hi3516cv200_clk_unregister,
};

/* SP804 dual-timer is wired up before platform_driver probing. Register
 * the minimum set of clocks that early consumers (SP804, PL011 console,
 * FEMAC ethernet) demand via CLK_OF_DECLARE. Same approach cv300 uses. */
static const struct hisi_fixed_rate_clock hi3516cv200_crg_early_fixed[] = {
	{ HI3516CV200_UART0_CLK,  "uart0_early",  NULL, 0, 24000000, },
	{ HI3516CV200_UART1_CLK,  "uart1_early",  NULL, 0, 24000000, },
	{ HI3516CV200_UART2_CLK,  "uart2_early",  NULL, 0, 24000000, },
	{ HI3516CV200_UART3_CLK,  "uart3_early",  NULL, 0, 24000000, },
	{ HI3516CV200_ETH_CLK,    "eth_early",    NULL, 0, 50000000, },
};

/* Minimal no-op reset controller — femac and other drivers may request
 * reset_control on probe; without a registered provider, ethernet
 * defers forever. Mirror cv300 with explicit 2-cell xlate so
 * reset_controller_register doesn't force of_reset_n_cells = 1. */

static int hi3516cv200_crg_reset_noop(struct reset_controller_dev *rcdev,
				      unsigned long id)
{
	return 0;
}

static const struct reset_control_ops hi3516cv200_crg_reset_ops = {
	.reset    = hi3516cv200_crg_reset_noop,
	.assert   = hi3516cv200_crg_reset_noop,
	.deassert = hi3516cv200_crg_reset_noop,
};

static int hi3516cv200_crg_reset_xlate(struct reset_controller_dev *rcdev,
				       const struct of_phandle_args *spec)
{
	return (spec->args[0] << 8) | (spec->args[1] & 0xff);
}

static struct reset_controller_dev hi3516cv200_crg_reset_rcdev = {
	.ops              = &hi3516cv200_crg_reset_ops,
	.nr_resets        = 0x40000,
	.of_reset_n_cells = 2,
	.of_xlate         = hi3516cv200_crg_reset_xlate,
};

/*
 * PERI_CRG12 bus_sc_sel [0] selects the bus clock, 24 MHz (0) or 198 MHz (1),
 * and the system APB clock is the bus clock divided by four. The boot loader
 * makes the choice (the Hi3518E V200 U-Boot selects 198 MHz, so 49.5 MHz), so
 * the early APB clock reads the select back instead of assuming a rate.
 */
#define HI3516CV200_PERI_CRG12		0x30
#define HI3516CV200_BUS_SC_SEL		BIT(0)

static void __init hi3516cv200_crg_early_apb(struct hisi_clock_data *clk_data)
{
	struct hisi_fixed_rate_clock apb = {
		HI3516CV200_SYSAPB_CLK, "sysapb_early", NULL, 0, 0,
	};
	u32 sel = readl(clk_data->base + HI3516CV200_PERI_CRG12);

	apb.fixed_rate = (sel & HI3516CV200_BUS_SC_SEL ? 198000000 : 24000000) / 4;
	if (hisi_clk_register_fixed_rate(&apb, 1, clk_data))
		pr_err("hi3516cv200-clk: failed to register the early APB clock\n");
}

static void __init hi3516cv200_crg_early_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;

	pr_info("hi3516cv200-clk: CLK_OF_DECLARE early init\n");
	clk_data = hisi_clk_init(np, HI3516CV200_CRG_NR_CLKS);
	if (!clk_data) {
		pr_err("hi3516cv200-clk: hisi_clk_init FAILED\n");
		return;
	}
	hisi_clk_register_fixed_rate(hi3516cv200_crg_early_fixed,
		ARRAY_SIZE(hi3516cv200_crg_early_fixed), clk_data);
	hi3516cv200_crg_early_apb(clk_data);

	hi3516cv200_crg_reset_rcdev.of_node = np;
	hi3516cv200_crg_reset_rcdev.owner = THIS_MODULE;
	if (reset_controller_register(&hi3516cv200_crg_reset_rcdev) < 0)
		pr_err("hi3516cv200-clk: reset controller register failed\n");
}
CLK_OF_DECLARE(hi3516cv200_clk_early, "hisilicon,hi3518ev20x-clock",
		hi3516cv200_crg_early_init);

/* ---------------- sys controller block (sysctrl@20050000) ---------------- */

/*
 * The SP804 count clocks are selected by SC_CTRL (sysctrl + 0x0): bit 16 (timer
 * 0), 18 (timer 1), 20 (timer 2) and 22 (timer 3) choose between the 3 MHz
 * reference (0) and the system APB clock (1). The boot loader may leave either
 * value (the Hi3518E V200 U-Boot sets bit 16), so model the select as a
 * read-only mux (SC_CTRL is write-protected by SC_LOCKEN) and let the read-back
 * of the register decide the rate. Timer init runs at
 * start_kernel, long before platform driver probing, so the mux and its parents
 * are registered from the sysctrl early init. The 3 MHz parent gets its own
 * early name; the CRG platform driver registers the "3m" clock later.
 */
static const char *const hi3516cv200_timer_mux_p[] = { "3m_early", "sysapb_early" };
static u32 hi3516cv200_timer_mux_table[] = { 0, 1 };

static const struct hisi_mux_clock hi3516cv200_sysctrl_early_mux[] = {
	{ HI3516CV200_TIME0_0_CLK, "timer0_0_early", hi3516cv200_timer_mux_p,
		ARRAY_SIZE(hi3516cv200_timer_mux_p), 0, 0x0, 16, 1,
		CLK_MUX_READ_ONLY,
		hi3516cv200_timer_mux_table, },
	{ HI3516CV200_TIME0_1_CLK, "timer0_1_early", hi3516cv200_timer_mux_p,
		ARRAY_SIZE(hi3516cv200_timer_mux_p), 0, 0x0, 18, 1,
		CLK_MUX_READ_ONLY,
		hi3516cv200_timer_mux_table, },
	{ HI3516CV200_TIME1_2_CLK, "timer1_2_early", hi3516cv200_timer_mux_p,
		ARRAY_SIZE(hi3516cv200_timer_mux_p), 0, 0x0, 20, 1,
		CLK_MUX_READ_ONLY,
		hi3516cv200_timer_mux_table, },
	{ HI3516CV200_TIME1_3_CLK, "timer1_3_early", hi3516cv200_timer_mux_p,
		ARRAY_SIZE(hi3516cv200_timer_mux_p), 0, 0x0, 22, 1,
		CLK_MUX_READ_ONLY,
		hi3516cv200_timer_mux_table, },
};

static void __init hi3516cv200_sysctrl_early_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;

	pr_info("hi3516cv200-sys: CLK_OF_DECLARE early init\n");
	clk_data = hisi_clk_init(np, HI3516CV200_SC_NR_CLKS);
	if (!clk_data) {
		pr_err("hi3516cv200-sys: hisi_clk_init FAILED\n");
		return;
	}
	if (IS_ERR(clk_register_fixed_rate(NULL, "3m_early", NULL, 0, 3000000))) {
		pr_err("hi3516cv200-sys: failed to register the early 3 MHz clock\n");
		goto err_del_provider;
	}
	if (hisi_clk_register_mux(hi3516cv200_sysctrl_early_mux,
				  ARRAY_SIZE(hi3516cv200_sysctrl_early_mux),
				  clk_data)) {
		pr_err("hi3516cv200-sys: failed to register the timer clock selects\n");
		goto err_del_provider;
	}
	return;

err_del_provider:
	/* no provider at all is better than one with missing timer clocks */
	of_clk_del_provider(np);
}
CLK_OF_DECLARE(hi3516cv200_sysctrl_early, "hisilicon,hi3518ev20x-sysctrl",
		hi3516cv200_sysctrl_early_init);

/* ---------------- platform driver (post-CLK_OF_DECLARE finalize) -------- */

static const struct of_device_id hi3516cv200_crg_match_table[] = {
	{ .compatible = "hisilicon,hi3518ev20x-clock", .data = &hi3516cv200_crg_funcs },
	{ }
};
MODULE_DEVICE_TABLE(of, hi3516cv200_crg_match_table);

static int hi3516cv200_crg_probe(struct platform_device *pdev)
{
	struct hisi_crg_dev *crg;

	crg = devm_kmalloc(&pdev->dev, sizeof(*crg), GFP_KERNEL);
	if (!crg)
		return -ENOMEM;

	crg->funcs = of_device_get_match_data(&pdev->dev);
	if (!crg->funcs)
		return -ENOENT;

	crg->rstc = hisi_reset_init(pdev);
	if (!crg->rstc)
		return -ENOMEM;

	crg->clk_data = crg->funcs->register_clks(pdev);
	if (IS_ERR(crg->clk_data)) {
		hisi_reset_exit(crg->rstc);
		return PTR_ERR(crg->clk_data);
	}

	platform_set_drvdata(pdev, crg);
	return 0;
}

static void hi3516cv200_crg_remove(struct platform_device *pdev)
{
	struct hisi_crg_dev *crg = platform_get_drvdata(pdev);

	hisi_reset_exit(crg->rstc);
	crg->funcs->unregister_clks(pdev);
}

static struct platform_driver hi3516cv200_crg_driver = {
	.probe  = hi3516cv200_crg_probe,
	.remove = hi3516cv200_crg_remove,
	.driver = {
		.name = "hi3516cv200-crg",
		.of_match_table = hi3516cv200_crg_match_table,
	},
};

static int __init hi3516cv200_crg_init(void)
{
	return platform_driver_register(&hi3516cv200_crg_driver);
}
core_initcall(hi3516cv200_crg_init);

static void __exit hi3516cv200_crg_exit(void)
{
	platform_driver_unregister(&hi3516cv200_crg_driver);
}
module_exit(hi3516cv200_crg_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("HiSilicon Hi3516CV200 / Hi3518EV20x CRG Driver");
