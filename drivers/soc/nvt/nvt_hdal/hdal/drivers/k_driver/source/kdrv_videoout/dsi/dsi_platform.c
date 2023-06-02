#ifdef __KERNEL__
#include <linux/clk.h>
#else
#include "pll.h"
#include "pll_protected.h"
#endif
#include <kwrap/type.h>
#include <kwrap/util.h>
#include "dsi_platform.h"

void dsi_platform_clk_set_freq(unsigned long ui_config)
{
#ifdef __KERNEL__
	struct clk *dsipll_clk;

	dsipll_clk = clk_get(NULL, "pll11");
	clk_prepare(dsipll_clk);
	//clk_disable(dsipll_clk);
	clk_set_rate(dsipll_clk, ui_config); //unit: Hz
	clk_enable(dsipll_clk);
	clk_put(dsipll_clk);
#else
	/*div = (UINT32) (dsi_src_clk/(ui_config/1000000));
	if(div == 0)
	{
		div++;
	}*/
	pll_setPLLEn(PLL_ID_11, FALSE);
	pll_setPLL(PLL_ID_11, (ui_config / 1000000) / 12 * 131072);
	pll_setPLLEn(PLL_ID_11, TRUE);
	//DBG_ERR("PLL11 = 0x%x\r\n", (unsigned int)(ui_config / 1000000) / 12 * 131072);
#endif

}

unsigned long dsi_platform_clk_get_freq(void)
{
	UINT32 div;
	unsigned long dsi_src_clk;

#ifdef __KERNEL__
	struct clk *dsipll_clk;

	dsipll_clk = clk_get(NULL, "pll11");
	clk_prepare(dsipll_clk);
	dsi_src_clk = clk_get_rate(dsipll_clk);
	div = 1;
	return (dsi_src_clk / div);
#else
	dsi_src_clk = pll_getPLLFreq(PLL_ID_11);
	div = 1;
	return (dsi_src_clk / div);
#endif

}

unsigned long dsi_platform_clk_get_lp_freq(void)
{
#ifdef __KERNEL__
	struct clk *parent;
	struct clk *dsilp_clk;
	unsigned long parent_rate = 0;

	dsilp_clk = clk_get(NULL, "f0840000.dsi");
	parent = clk_get_parent(dsilp_clk);
	parent_rate = clk_get_rate(parent);
	return parent_rate;
#else
	if (pll_getClockRate(PLL_CLKSEL_DSI_LPSRC)) {
		return 80000000;
	} else {
		return 60000000;
	}
#endif

}

