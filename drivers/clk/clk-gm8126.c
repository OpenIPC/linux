// SPDX-License-Identifier: GPL-2.0-only
/*
 * Grain Media GM8126 clocks, as the boot loader left them: the rates are
 * read from the PMU once. The gates are those the vendor kernel's and
 * modules' PMU tables name; a set bit stops the clock.
 *
 * PLL1 reads back shifted left by one from where it is written (vendor
 * U-Boot board/gm/gm8126).
 */

#include <linux/bitfield.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <dt-bindings/clock/grainmedia,gm8126.h>

#define PMU_CLKSEL		0x28
#define   CLKSEL_UART_PLL2	BIT(15)
#define PMU_PLL1		0x30
#define   PLL1_MUL		GENMASK(12, 4)
#define   PLL1_DIV		GENMASK(17, 13)
#define   PLL1_HCLK		GENMASK(26, 25)
#define   PLL1_FCLK		GENMASK(28, 27)
#define PMU_PLL2		0x34
#define   PLL2_MUL		GENMASK(11, 3)
#define   PLL2_DIV		GENMASK(16, 12)
#define PMU_DIV			0x78
#define   DIV_SSP0		GENMASK(5, 0)
#define   DIV_UART		GENMASK(23, 18)
#define PMU_DIV1		0x74
#define   DIV1_SDC		GENMASK(3, 0)
#define PMU_GATE_AHB		0x38
#define PMU_GATE_APB		0x3c

#define GM8126_OSC		30000000UL

static DEFINE_SPINLOCK(gm8126_gate_lock);

static const struct {
	const char *name, *parent;
	u8 id, reg, bit;
} gm8126_gates[] = {
	{ "h264e",   "ahb",      GM8126_CLK_H264E,   PMU_GATE_AHB, 4 },
	{ "mcp100",  "ahb",      GM8126_CLK_MCP100,  PMU_GATE_AHB, 6 },
	{ "mac_ahb", "ahb",      GM8126_CLK_MAC_AHB, PMU_GATE_AHB, 8 },
	{ "mac",     "mac_ahb",  GM8126_CLK_MAC,     PMU_GATE_AHB, 10 },
	{ "dmac",    "ahb",      GM8126_CLK_DMAC,    PMU_GATE_AHB, 11 },
	{ "sdc_ahb", "sdc_div",  GM8126_CLK_SDC_AHB, PMU_GATE_AHB, 12 },
	{ "scaler",  "ahb",      GM8126_CLK_SCALER,  PMU_GATE_AHB, 14 },
	{ "di3d",    "ahb",      GM8126_CLK_DI3D,    PMU_GATE_AHB, 20 },
	{ "ssp0",    "ssp0_div", GM8126_CLK_SSP0,    PMU_GATE_APB, 5 },
	{ "sdc",     "sdc_ahb",  GM8126_CLK_SDC,     PMU_GATE_APB, 8 },
	{ "gpio0",   "apb",      GM8126_CLK_GPIO0,   PMU_GATE_APB, 9 },
	{ "gpio1",   "apb",      GM8126_CLK_GPIO1,   PMU_GATE_APB, 10 },
	{ "gpio2",   "apb",      GM8126_CLK_GPIO2,   PMU_GATE_APB, 11 },
	{ "i2c",     "apb",      GM8126_CLK_I2C,     PMU_GATE_APB, 13 },
	{ "wdt",     "apb",      GM8126_CLK_WDT,     PMU_GATE_APB, 15 },
	{ "rtc",     "apb",      GM8126_CLK_RTC,     PMU_GATE_APB, 16 },
	{ "adc",     "apb",      GM8126_CLK_ADC,     PMU_GATE_APB, 25 },
};

static unsigned long pll(u32 mul, u32 div)
{
	return GM8126_OSC / (div ?: 1) * mul;
}

static void __init gm8126_clk_init(struct device_node *np)
{
	static const char * const names[] = {
		"pll1", "pll2", "cpu", "ahb", "apb", "ssp0_div", "uart",
		"sdc_div",
	};
	unsigned long rate[ARRAY_SIZE(names)], p1, p2;
	struct clk_hw_onecell_data *data;
	void __iomem *base;
	u32 v1, v2, div;
	int i;

	base = of_iomap(np, 0);
	data = kzalloc(struct_size(data, hws, GM8126_CLK_NUM), GFP_KERNEL);
	if (!base || !data) {
		pr_err("%pOF: no registers or memory\n", np);
		return;
	}
	v1 = readl(base + PMU_PLL1);
	v2 = readl(base + PMU_PLL2);
	div = readl(base + PMU_DIV);
	p1 = pll(FIELD_GET(PLL1_MUL, v1), FIELD_GET(PLL1_DIV, v1));
	p2 = pll(FIELD_GET(PLL2_MUL, v2), FIELD_GET(PLL2_DIV, v2));

	rate[GM8126_CLK_PLL1] = p1;
	rate[GM8126_CLK_PLL2] = p2;
	switch (FIELD_GET(PLL1_FCLK, v1)) {
	case 0:
		rate[GM8126_CLK_CPU] = p1;
		break;
	case 1:
		rate[GM8126_CLK_CPU] = p1 / 2;
		break;
	case 2:
		rate[GM8126_CLK_CPU] = p1 / 3 * 2;
		break;
	default:
		rate[GM8126_CLK_CPU] = p2;
	}
	switch (FIELD_GET(PLL1_HCLK, v1)) {
	case 0:
		rate[GM8126_CLK_AHB] = p1;
		break;
	case 1:
		rate[GM8126_CLK_AHB] = p1 / 2;
		break;
	case 2:
		rate[GM8126_CLK_AHB] = p1 / 3;
		break;
	default:
		rate[GM8126_CLK_AHB] = p2 / 2;
	}
	rate[GM8126_CLK_APB] = rate[GM8126_CLK_AHB] / 2;
	rate[GM8126_CLK_SSP0_DIV] = p2 / (FIELD_GET(DIV_SSP0, div) + 1);
	rate[GM8126_CLK_UART] = (readl(base + PMU_CLKSEL) & CLKSEL_UART_PLL2 ?
				 p2 : GM8126_OSC) /
				(FIELD_GET(DIV_UART, div) + 1);
	/* the vendor's: 100 MHz from PLL1 at 800 MHz, less above it */
	i = p1 > 800000000 ? 8 : 7;
	writel((readl(base + PMU_DIV1) & ~DIV1_SDC) | i, base + PMU_DIV1);
	rate[GM8126_CLK_SDC_DIV] = p1 / (i + 1);

	data->num = GM8126_CLK_NUM;
	for (i = 0; i < ARRAY_SIZE(names); i++)
		data->hws[i] = clk_hw_register_fixed_rate(NULL, names[i], NULL,
							  0, rate[i]);
	for (i = 0; i < ARRAY_SIZE(gm8126_gates); i++)
		data->hws[gm8126_gates[i].id] =
			clk_hw_register_gate(NULL, gm8126_gates[i].name,
					     gm8126_gates[i].parent, 0,
					     base + gm8126_gates[i].reg,
					     gm8126_gates[i].bit,
					     CLK_GATE_SET_TO_DISABLE,
					     &gm8126_gate_lock);
	of_clk_add_hw_provider(np, of_clk_hw_onecell_get, data);
}
CLK_OF_DECLARE(gm8126, "grainmedia,gm8126-pmu", gm8126_clk_init);
