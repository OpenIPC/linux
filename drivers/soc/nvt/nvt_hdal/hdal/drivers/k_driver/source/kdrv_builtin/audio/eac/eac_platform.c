#ifdef __KERNEL__
#include "kwrap/type.h"
#include "kwrap/debug.h"
#include <linux/clk.h>
#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/audcap_builtin.h"

void _eac_enableclk_platform(BOOL b_en)
{
	struct clk *p_eac_clk;

	p_eac_clk = clk_get(NULL, "f0640000.eac");

	if (IS_ERR(p_eac_clk)) {
		nvt_dbg(ERR, "failed to get eac clk\n");
	}

	if (b_en) {
		clk_enable(p_eac_clk);
	} else {
		clk_disable(p_eac_clk);
	}
	clk_put(p_eac_clk);
}

void _eac_enableadcclk_platform(BOOL b_en)
{
	struct clk *p_eac_adc_clk;

	p_eac_adc_clk = clk_get(NULL, "f0640000.eacadc");
	if (IS_ERR(p_eac_adc_clk)) {
		nvt_dbg(ERR, "failed to get eac-ad clk\n");
	}

	if (b_en) {
		clk_enable(p_eac_adc_clk);
	} else {
		clk_disable(p_eac_adc_clk);
	}
	clk_put(p_eac_adc_clk);
}

void _eac_enabledacclk_platform(BOOL b_en)
{
	struct clk *p_eac_dac_clk;

	p_eac_dac_clk = clk_get(NULL, "f0640000.eacdac");
	if (IS_ERR(p_eac_dac_clk)) {
		nvt_dbg(ERR, "failed to get eac-da clk\n");
	}

	if (b_en) {
		clk_enable(p_eac_dac_clk);
	} else {
		clk_disable(p_eac_dac_clk);
	}
	clk_put(p_eac_dac_clk);
}

void _eac_setdacclkrate_platform(unsigned long clkrate)
{
	struct clk *eacdac_clk;

	eacdac_clk = clk_get(NULL, "f0640000.eacdac");
	if (IS_ERR(eacdac_clk)) {
		DBG_ERR("failed to get eacdac clk\n");
	}
	clk_set_rate(eacdac_clk, clkrate);
	clk_put(eacdac_clk);
}





#endif
