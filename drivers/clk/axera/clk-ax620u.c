// SPDX-License-Identifier: GPL-2.0
/*
 * Clock provider for the Axera AX620U ("axera,ax620x") camera SoC.
 *
 * Its devicetree hands every peripheral <&xxx_clk N> from nine syscon
 * clock blocks (ap/common/cpu/dispc/flash/isp/mm/vpu clk). The register
 * layout of those blocks differs from AX620E's (clk-ax620e.c, which binds
 * the same compatibles), and the vendor AX620U clock driver is not in the
 * AX620E SDK. Until it is, keep the hardware exactly as the bootloader
 * configured it: each index resolves to a fixed-rate, always-on clock
 * whose rate comes from "axera,stub-rate" on the block (default 24 MHz,
 * the crystal). Gating and rate changes are no-ops.
 */
#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/slab.h>

static void __init ax620u_clk_init(struct device_node *np)
{
	struct clk_hw *hw;
	u32 rate = 24000000;

	of_property_read_u32(np, "axera,stub-rate", &rate);
	hw = clk_hw_register_fixed_rate(NULL, np->full_name, NULL, 0, rate);
	if (IS_ERR(hw)) {
		pr_err("%pOF: clock: %ld\n", np, PTR_ERR(hw));
		return;
	}
	of_clk_add_hw_provider(np, of_clk_hw_simple_get, hw);
}

CLK_OF_DECLARE(ax620u_ap_clk, "axera,ax620x-ap-clk", ax620u_clk_init);
CLK_OF_DECLARE(ax620u_common_clk, "axera,ax620x-common-clk", ax620u_clk_init);
CLK_OF_DECLARE(ax620u_cpu_clk, "axera,ax620x-cpu-clk", ax620u_clk_init);
CLK_OF_DECLARE(ax620u_dispc_clk, "axera,ax620x-dispc-clk", ax620u_clk_init);
CLK_OF_DECLARE(ax620u_flash_clk, "axera,ax620x-flash-clk", ax620u_clk_init);
CLK_OF_DECLARE(ax620u_isp_clk, "axera,ax620x-isp-clk", ax620u_clk_init);
CLK_OF_DECLARE(ax620u_mm_clk, "axera,ax620x-mm-clk", ax620u_clk_init);
CLK_OF_DECLARE(ax620u_vpu_clk, "axera,ax620x-vpu-clk", ax620u_clk_init);
