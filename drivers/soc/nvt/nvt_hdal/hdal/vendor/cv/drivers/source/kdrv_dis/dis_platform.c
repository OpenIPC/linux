#include "dis_platform.h"

#if defined(__LINUX)
#include <linux/clk.h>
#endif


#define DIS_MODULE_CLK_NUM 1

#if defined(__FREERTOS)
ID 		  FLG_ID_DIS;
#else
FLGPTN     FLG_ID_DIS;
#endif

#if defined(__FREERTOS)
#else
extern struct clk* dis_clk[DIS_MODULE_CLK_NUM];
#endif

#if defined(__FREERTOS)
static UINT32 g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
#else
#endif

VOID dis_platform_request_irq(VOID *isr_handler)
{
#if defined(__FREERTOS)
	irq_handler_t isr_func = (irq_handler_t) isr_handler;
    request_irq(INT_ID_DIS, isr_func, IRQF_TRIGGER_HIGH, "dis", 0);
#endif
	return;
}

void dis_platform_clk_enable(VOID)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableClock(DIS_CLK);
#else
	clk_prepare(dis_clk[0]);
	clk_enable(dis_clk[0]);
#endif
}

void dis_platform_clk_disable(VOID)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_disableClock(DIS_CLK);
#else
	clk_disable(dis_clk[0]);
	clk_unprepare(dis_clk[0]);
#endif
}

void dis_platform_prepare_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableSystemReset(DIS_RSTN);
	pll_disableSystemReset(DIS_RSTN);
#else
	clk_prepare(dis_clk[0]);
#endif
}

void dis_platform_unprepare_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
    //pll_enableSystemReset(DIS_RSTN);
#else
	clk_unprepare(dis_clk[0]);
#endif
}
VOID dis_platform_flg_sel(VOID)
{	
	rel_flg(FLG_ID_DIS);
}

ER dis_platform_flg_clear(UINT32 flag_id)
{
	return clr_flg(FLG_ID_DIS, flag_id);
}

void dis_platform_flg_set(UINT32 flag_id)
{
	iset_flg(FLG_ID_DIS, flag_id);
}

void dis_platform_flg_wait(FLGPTN *uiFlag, UINT32 flag_id)
{
	wai_flg(uiFlag, FLG_ID_DIS, flag_id, TWF_CLR | TWF_ORW);
}

/**
	DIS Create Resource
*/
VOID dis_platform_create_resource(VOID)
{
	OS_CONFIG_FLAG(FLG_ID_DIS);
}


UINT32 dis_platform_va2pa(UINT32 addr)
{
	VOS_ADDR rt;
    rt = vos_cpu_get_phy_addr(addr);
	if (rt == VOS_ADDR_INVALID) {
		DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)addr);
		return 0;
	}
	return rt;

}

UINT32 dis_platform_dma_flush(UINT32 addr, UINT32 size)
{
	//nvt_dbg(IND, "cache flush: addr(0x%x) size(0x%x)\r\n", addr, size);
    vos_cpu_dcache_sync((VOS_ADDR)addr, size, VOS_DMA_FROM_DEVICE);
    return 0;
}

/**
    DIS set operation clock

    Set DIS clock selection.

    @param[in] clock setting.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER dis_platform_set_clock_rate(UINT32 uiClock)
{
#if defined __UITRON || defined __ECOS
	if(uiClock >= 480) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_480;
		if(uiClock > 480) DBG_ERR("Input clock frequency %d round to 480\r\n", uiClock);
	} else if(uiClock >= 360) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_PLL13;	// 360MHz
		if(pll_get_pll_enable(PLL_ID_13) == FALSE) pll_set_pll_enable(PLL_ID_13, TRUE);
		if(uiClock > 360) DBG_ERR("Input clock frequency %d round to 360\r\n", uiClock);
	} else if(uiClock >= 320) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_320;
		if(pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		if(uiClock > 320) DBG_ERR("Input clock frequency %d round to 320\r\n", uiClock);
	} else if(uiClock >= 240) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		if(uiClock > 240) DBG_ERR("Input clock frequency %d round to 240\r\n", uiClock);
	} else {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		DBG_ERR("Input clock frequency %d is too small, set to 240 instead\r\n", uiClock);
	}
	pll_setClockRate(PLL_CLKSEL_DIS, g_uiDisTrueClock);
#elif defined(__FREERTOS)
#if defined(_BSP_NA51055_)
	if(uiClock >= 480) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_480;
		if(uiClock > 480) DBG_ERR("Input clock frequency %d round to 480\r\n", uiClock);
	} else if(uiClock >= 360) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_PLL13;	// 360MHz
		if(pll_get_pll_enable(PLL_ID_13) == FALSE) pll_set_pll_enable(PLL_ID_13, TRUE);
		if(uiClock > 360) DBG_ERR("Input clock frequency %d round to 360\r\n", uiClock);
	} else if(uiClock >= 320) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_320;
		if(pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		if(uiClock > 320) DBG_ERR("Input clock frequency %d round to 320\r\n", uiClock);
	} else if(uiClock >= 240) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		if(uiClock > 240) DBG_ERR("Input clock frequency %d round to 240\r\n", uiClock);
	} else {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		DBG_ERR("Input clock frequency %d is too small, set to 240 instead\r\n", uiClock);
	}
	pll_setClockRate(PLL_CLKSEL_DIS, g_uiDisTrueClock);
#elif defined(_BSP_NA51089_)
	if(uiClock >= 240) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		if(uiClock > 240) DBG_ERR("Input clock frequency %d round to 240\r\n", uiClock);
	} else if(uiClock >= 192) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_192;
		if(uiClock > 192) DBG_ERR("Input clock frequency %d round to 192\r\n", uiClock);
	} else if(uiClock >= 96) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_96;
		if(uiClock > 96) DBG_ERR("Input clock frequency %d round to 96\r\n", uiClock);
	} else {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_96;
		DBG_ERR("Input clock frequency %d is too small, set to 96 instead\r\n", uiClock);
	}
	pll_setClockRate(PLL_CLKSEL_DIS, g_uiDisTrueClock);
#endif

#else
#if 0
	if(uiClock >= 480) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_480;
		if(uiClock > 480) DBG_ERR("Input clock frequency %d round to 480\r\n", uiClock);
	} else if(uiClock >= 360) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_PLL13;	// 360MHz
		if(pll_get_pll_enable(PLL_ID_13) == FALSE) pll_set_pll_enable(PLL_ID_13, TRUE);
		if(uiClock > 360) DBG_ERR("Input clock frequency %d round to 360\r\n", uiClock);
	} else if(uiClock >= 320) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_320;
		if(pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		if(uiClock > 320) DBG_ERR("Input clock frequency %d round to 320\r\n", uiClock);
	} else if(uiClock >= 240) {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		if(uiClock > 240) DBG_ERR("Input clock frequency %d round to 240\r\n", uiClock);
	} else {
		g_uiDisTrueClock = PLL_CLKSEL_DIS_240;
		DBG_ERR("Input clock frequency %d is too small, set to 240 instead\r\n", uiClock);
	}
	pll_setClockRate(PLL_CLKSEL_DIS, g_uiDisTrueClock);

#else
	struct clk *pclk;

	if(uiClock >= 480) {
		pclk = clk_get(NULL, "fix480m");
		if(uiClock > 480) DBG_ERR("Input clock frequency %d round to 480\r\n", uiClock);
	} else if(uiClock >= 360) {
		pclk = clk_get(NULL, "pll13");
		if(uiClock > 360) DBG_ERR("Input clock frequency %d round to 360\r\n", uiClock);
	} else if(uiClock >= 320) {
		pclk = clk_get(NULL, "pllf320");
		if(uiClock > 320) DBG_ERR("Input clock frequency %d round to 320\r\n", uiClock);
	} else if(uiClock >= 240) {
		pclk = clk_get(NULL, "fix240m");
		if(uiClock > 240) DBG_ERR("Input clock frequency %d round to 240\r\n", uiClock);
	} else {
		pclk = clk_get(NULL, "fix240m");
		DBG_ERR("Input clock frequency %d is too small, set to 240 instead\r\n", uiClock);
	}

	if (IS_ERR(pclk)) {
		printk("%s: get source clock fail\r\n", __func__);
	}

	clk_set_parent(dis_clk[0], pclk);
	clk_put(pclk);
#endif

#endif //#if defined(__FREERTOS)

	return E_OK;
}


//@}

UINT32 dis_pt_pa2va_remap(UINT32 pa, UINT32 sz)
{
    UINT32 va = 0;

#if defined(__FREERTOS)
    va = pa;
#else

    if (sz == 0) {
        return va;
    }
    if (pfn_valid(__phys_to_pfn(pa))) {
        va = (UINT32)__va(pa);
    } else {
        va = (UINT32)ioremap(pa, PAGE_ALIGN(sz));
    }
    fmem_dcache_sync((UINT32 *)va, sz, DMA_BIDIRECTIONAL);
#endif
    return va;
} 
