/**
    NVT pll preset function
    This file will preset PLL value
    @file       pll_preset.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include <linux/clk.h>

static int __init nvt_preset_pll(void)
{
	struct clk* pll10_clk, *pll0_clk, *pll8_clk, *pll_clk;

	pll10_clk = clk_get(NULL, "pll10");
	if (!IS_ERR(pll10_clk))
		clk_set_rate(pll10_clk, 270000000);
	else
		pr_err("*** %s Get PLL10 Clock error\n", __func__);

	pll0_clk = clk_get(NULL, "pll0");
	if (!IS_ERR(pll0_clk))
		clk_set_rate(pll0_clk, 500000000);
	else
		pr_err("*** %s Get PLL0 Clock error\n", __func__);

	pll8_clk = clk_get(NULL, "pll8");
	if (!IS_ERR(pll8_clk))
		clk_set_rate(pll8_clk, 465000000);
	else
		pr_err("*** %s Get PLL8 Clock error\n", __func__);


	printk("------------------------------\r\n");
	pll10_clk = clk_get(NULL, "pll0");
	pll0_clk  = clk_get(NULL, "pll1");
	pll8_clk  = clk_get(NULL, "pll13");
	pll_clk   = clk_get(NULL, "pll4");
	printk("AXI0=%d AXI1=%d AXI2=%d HCLK=%d\r\n",(int)(clk_get_rate(pll10_clk)+100)/1000000
								,(int)(clk_get_rate(pll0_clk)+100)/1000000
								,(int)(clk_get_rate(pll8_clk)+100)/1000000
								,(int)(clk_get_rate(pll_clk)+100)/1000000);

	pll10_clk = clk_get(NULL, "pll16");
	pll0_clk  = clk_get(NULL, "pll3");
	pll8_clk  = clk_get(NULL, "pll15");
	pll_clk   = clk_get(NULL, "pll14");
	printk("CPU=%d DRAM=%d DSP=%d CODEC=%d\r\n",(int)((clk_get_rate(pll10_clk)+100)/1000000)*8
								,(int)((clk_get_rate(pll0_clk)+100)/1000000)*8
								,(int)((clk_get_rate(pll8_clk)+100)/1000000)
								,(int)(clk_get_rate(pll_clk)+100)/1000000);

	pll10_clk = clk_get(NULL, "pll9");
	pll0_clk  = clk_get(NULL, "pll10");
	pll8_clk  = clk_get(NULL, "pll11");
	pll_clk   = clk_get(NULL, "pll2");
	printk("DISP0=%d DISP1=%d DISP2=%d CNN=%d\r\n",(int)(clk_get_rate(pll10_clk)+100)/1000000
								,(int)(clk_get_rate(pll0_clk)+100)/1000000
								,(int)(clk_get_rate(pll8_clk)+100)/1000000
								,(int)(clk_get_rate(pll_clk)+100)/1000000);

	pll10_clk = clk_get(NULL, "pll8");
	pll0_clk  = clk_get(NULL, "pll12");
	printk("MPLL8(VCAP)=%d SSP=%d\r\n",(int)(clk_get_rate(pll10_clk)+100)/1000000
									  ,(int)(clk_get_rate(pll0_clk)+100)/1000000);
	printk("------------------------------\r\n");



	return 0;
}

arch_initcall_sync(nvt_preset_pll);
