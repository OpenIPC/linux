#ifdef __KERNEL__
#include "kwrap/type.h"
#include "kwrap/debug.h"
#include <linux/clk.h>
#include "kdrv_builtin/kdrv_builtin.h"
#include "kdrv_builtin/audcap_builtin.h"

void _dai_enableclk_platform(void)
{
	struct clk *dai_clk, *source_clk;

	dai_clk = clk_get(NULL, "f0630000.dai");
	if (IS_ERR(dai_clk)) {
		DBG_ERR("failed to get dai clk\n");
	}
	clk_enable(dai_clk);

	source_clk = clk_get(NULL, "pll7");
	if (IS_ERR(source_clk)) {
		DBG_ERR("failed to get pll7 clk\n");
	}
	clk_set_parent(dai_clk, source_clk);

	clk_put(dai_clk);
	clk_put(source_clk);
}

void _dai_disableclk_platform(void)
{
	struct clk *dai_clk;

	dai_clk = clk_get(NULL, "f0630000.dai");
	if (IS_ERR(dai_clk)) {
		DBG_ERR("failed to get dai clk\n");
	}
	clk_disable(dai_clk);
	clk_put(dai_clk);
}

void _dai_setclkrate_platform(unsigned long clkrate)
{
	struct clk *dai_clk;

	dai_clk = clk_get(NULL, "f0630000.dai");
	if (IS_ERR(dai_clk)) {
		DBG_ERR("failed to get dai clk\n");
	}
	clk_set_rate(dai_clk, clkrate);
	clk_put(dai_clk);
}




#endif
