#include <kwrap/type.h>
#include <kwrap/util.h>
#include "csi_platform.h"
#include "csi_reg.h"
#include "csi_int.h"


#ifdef __KERNEL__
#include "csi_drv.h"
extern PMODULE_INFO pcsi_mod_info;
//#define CSI_ENABLE_PLLCLK(x)   clk_enable(pcsi_mod_info->pclk[x])
//#define CSI_DISABLE_PLLCLK(x)  clk_disable(pcsi_mod_info->pclk[x])
static struct completion csi_completion;
static struct completion csi2_completion;
#else
//#define CSI_ENABLE_PLLCLK(x)    pll_enable_clock((x))
//#define CSI_DISABLE_PLLCLK(x)   pll_disable_clock((x))
#endif

void csi_platform_clk_enable(CSI_ID id) 
{
#ifdef __KERNEL__
	printk("csi_platform_clk_enable....\r\n");
	clk_enable(pcsi_mod_info->pclk[id]);
#else
	pll_enable_clock((MIPI_LVDS_CLK+id));
#endif
}

void csi_platform_clk_disable(CSI_ID id)
{
#ifdef __KERNEL__
	printk("csi_platform_clk_disable....\r\n");
	clk_disable(pcsi_mod_info->pclk[id]);
#else
	pll_disable_clock((MIPI_LVDS_CLK+id));
#endif
}

void csi_platform_clk_get_freq(CSI_ID id, unsigned long *p_clk)
{
#ifdef __KERNEL__
	struct clk *parent;
	struct clk *csi_clk;
	unsigned long parent_rate = 0;	

	if (id == CSI_ID_CSI) {
		csi_clk = clk_get(NULL, "f0280000.csi");
	} else {
		csi_clk = clk_get(NULL, "f0330000.csi");
	}
	parent = clk_get_parent(csi_clk);
	parent_rate = clk_get_rate(parent);
	*p_clk = parent_rate;
	clk_put(csi_clk);
#else
#endif
}

#ifdef __KERNEL__
void csi_platform_init_completion(CSI_ID id)
{
	if (id == CSI_ID_CSI)
		init_completion(&csi_completion);
	else
		init_completion(&csi2_completion);
}

void csi_platform_reinit_completion(CSI_ID id)
{
	if (id == CSI_ID_CSI)
		reinit_completion(&csi_completion);
	else
		reinit_completion(&csi2_completion);
}

void csi_platform_wait_completion(CSI_ID id)
{
	if (id == CSI_ID_CSI)
		wait_for_completion(&csi_completion);
	else
		wait_for_completion(&csi2_completion);
}

unsigned long csi_platform_wait_completion_timeout(CSI_ID id, unsigned long timeout)
{
	if (id == CSI_ID_CSI)
		return wait_for_completion_timeout(&csi_completion, msecs_to_jiffies(timeout));
	else
		return wait_for_completion_timeout(&csi2_completion, msecs_to_jiffies(timeout));
}

void csi_platform_complete(CSI_ID id)
{
	if (id == CSI_ID_CSI)
		complete(&csi_completion);
	else
		complete(&csi2_completion);
}
#endif
