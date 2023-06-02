/*
 * Allwinner sun50iw10p1 SoCs clk driver.
 *
 * Copyright(c) 2012-2016 Allwinnertech Co., Ltd.
 * Author: huanghuafeng <huafenghuang@allwinnertech.com>
 *
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* the clk table is not right ,please fix me when ic come back */

#include "clk-sun50iw11.h"
struct sunxi_clk_factor_freq factor_pllcpu_tbl[] = {
PLLCPU(16,    408000000U),
};

/* PLLPERIPH0(n, d1, d2, freq)	F_N8X8_D1V1X1_D2V0X1 */
struct sunxi_clk_factor_freq factor_pllperiph0_tbl[] = {
PLLPERIPH0(99,     0,     0,     600000000U),
};

static unsigned int pllcpu_max, pllperiph0_max;

#define PLL_MAX_ASSIGN(name) (pll##name##_max = \
	factor_pll##name##_tbl[ARRAY_SIZE(factor_pll##name##_tbl)-1].freq)

void sunxi_clk_factor_initlimits(void)
{
	PLL_MAX_ASSIGN(cpu);
	PLL_MAX_ASSIGN(periph0);
}
