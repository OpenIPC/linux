#include "tge_platform.h"

// debug level
#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
unsigned int tge_debug_level = (NVT_DBG_IND | NVT_DBG_WRN | NVT_DBG_ERR);
#else
#endif

#if defined __UITRON || defined __ECOS
#elif defined(__FREERTOS)
//#define MODULE_CLK_NUM 1
#else
static struct clk* tge_clk[2];
UINT32 _TGE_REG_BASE_ADDR;
#endif



static SEM_HANDLE SEMID_TGE;
static ID     FLG_ID_TGE;

ER tge_platform_flg_clear(FLGPTN flg)
{
	clr_flg(FLG_ID_TGE, flg);
	return E_OK;
}
ER tge_platform_flg_set(FLGPTN flg)
{
	iset_flg(FLG_ID_TGE, flg);
	return E_OK;
}
ER tge_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg)
{
	wai_flg(p_flgptn, FLG_ID_TGE, flg, TWF_CLR | TWF_ORW);
	return E_OK;
}
ER tge_platform_sem_wait(void)
{
	ER erReturn = E_OK;

	#if defined __UITRON || defined __ECOS
	return wai_sem(SEMID_TGE);
	#else
	erReturn = SEM_WAIT(SEMID_TGE);
	#endif

	return erReturn;
}

ER tge_platform_sem_signal(void)
{
	#if defined __UITRON || defined __ECOS
	return sig_sem(SEMID_TGE);
	#else
	SEM_SIGNAL(SEMID_TGE);
	return E_OK;
	#endif
}
void tge_platform_enable_clk(void)
{
	#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableClock(TGE_CLK);
	#else
	clk_enable(tge_clk[0]);
	clk_enable(tge_clk[1]);
	#endif
}
void tge_platform_disable_clk(void)
{
	#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_disableClock(TGE_CLK);
	#else
	clk_disable(tge_clk[0]);
	clk_disable(tge_clk[1]);
	#endif
}
void tge_platform_prepare_clk(void)
{
	#if defined __UITRON || defined __ECOS || defined __FREERTOS
	#else
	clk_prepare(tge_clk[0]);
	clk_prepare(tge_clk[1]);
	#endif
}
void tge_platform_unprepare_clk(void)
{
	#if defined __UITRON || defined __ECOS || defined __FREERTOS
	#else
	#endif
}
ER tge_setClock(TGE_CLKSRC_SEL ClkSrc)
{
	#if defined __UITRON || defined __ECOS || defined __FREERTOS
	// for CLK
	if (ClkSrc == CLKSRC_PXCLK) {
		pll_setClockRate(PLL_CLKSEL_TGE, PLL_CLKSEL_TGE_PXCLKSRC_PXCLKPAD);
	} else if (ClkSrc == CLKSRC_MCLK) {
		pll_setClockRate(PLL_CLKSEL_TGE, PLL_CLKSEL_TGE_PXCLKSRC_MCLK);
		pll_enableClock(SIE_MCLK);
	} else if (ClkSrc == CLKSRC_MCLK2) {
		//pll_setClockRate(PLL_CLKSEL_TGE, PLL_CLKSEL_TGE_PXCLKSRC_MCLK2);
	}
	#else
	struct clk *parent_clk = NULL;

	// for CLK
	if (ClkSrc == CLKSRC_PXCLK) {
		parent_clk = clk_get(NULL, "sie_io_pxclk");
	} else if (ClkSrc == CLKSRC_MCLK) {
		parent_clk = clk_get(NULL, "f0c00000.siemck");
	} else if (ClkSrc == CLKSRC_MCLK2) {
		nvt_dbg(ERR, "TGE CLK1 is not support MCLK2 !!\r\n");
	}

	clk_enable(tge_clk[0]);

	if (parent_clk == NULL){
		nvt_dbg(IND, "tge_setClock  fail!\r\n");
		return E_SYS;
	}

	if (!IS_ERR(parent_clk))
		clk_set_parent(tge_clk[0], parent_clk);

	clk_put(parent_clk);
	#endif

	return E_OK;
}
ER tge_setClock2(TGE_CLKSRC_SEL ClkSrc2)
{
	#if defined __UITRON || defined __ECOS || defined __FREERTOS
	// for CLK2
	if (ClkSrc2 == CLKSRC_PXCLK) {
		pll_setClockRate(PLL_CLKSEL_TGE2, PLL_CLKSEL_TGE2_PXCLKSRC_PXCLKPAD);
	} else if (ClkSrc2 == CLKSRC_MCLK) {
		//pll_setClockRate(PLL_CLKSEL_TGE2, PLL_CLKSEL_TGE2_PXCLKSRC_MCLK);
	} else if (ClkSrc2 == CLKSRC_MCLK2) {
		pll_setClockRate(PLL_CLKSEL_TGE2, PLL_CLKSEL_TGE2_PXCLKSRC_MCLK2);
		pll_enableClock(SIE_MCLK2);
	}
	#else
	struct clk *parent_clk = NULL;

	// for CLK2
	if (ClkSrc2 == CLKSRC_PXCLK) {
		parent_clk = clk_get(NULL, "sie2_io_pxclk");
	} else if (ClkSrc2 == CLKSRC_MCLK) {
		nvt_dbg(ERR, "TGE CLK2 is not support MCLK !!\r\n");
	} else if (ClkSrc2 == CLKSRC_MCLK2) {
		parent_clk = clk_get(NULL, "f0c00000.siemk2");
	}

	clk_enable(tge_clk[1]);

	if (parent_clk == NULL){
		nvt_dbg(IND, "tge_setClock2  fail!\r\n");
		return E_SYS;
	}

	if (!IS_ERR(parent_clk))
		clk_set_parent(tge_clk[1], parent_clk);

	clk_put(parent_clk);
	#endif

	return E_OK;
}
void tge_platform_set_clk_rate(TGE_OPENOBJ *pObjCB)
{
	tge_setClock(pObjCB->TgeClkSel);
	tge_setClock2(pObjCB->TgeClkSel2);
}
#if !(defined __UITRON || defined __ECOS)
#if defined __FREERTOS
irqreturn_t tge_platform_isr(int irq, void *devid)
{
	tge_isr();
	return IRQ_HANDLED;
}
void tge_platform_create_resource(void)
{
	OS_CONFIG_FLAG(FLG_ID_TGE);
	SEM_CREATE(SEMID_TGE,1);

	request_irq(INT_ID_TGE, tge_platform_isr, IRQF_TRIGGER_HIGH, "tge", 0);

	nvt_dbg(ERR, "tge_platform_create_resource\r\n");
}
#else
void tge_platform_create_resource(TGE_MODULE_INFO *pmodule_info)
{
    OS_CONFIG_FLAG(FLG_ID_TGE);
	SEM_CREATE(SEMID_TGE,1);

	_TGE_REG_BASE_ADDR = (UINT32)pmodule_info->io_addr[0];

	//nvt_dbg(ERR, "base addr = 0x%08x\r\n", _TGE_REG_BASE_ADDR);

	tge_clk[0] = clk_get(NULL, "f0cc0000.tge");
	tge_clk[1] = clk_get(NULL, "f0cc0000.tge2");
}
#endif
void tge_platform_release_resource(void)
{
	rel_flg(FLG_ID_TGE);
	SEM_DESTROY(SEMID_TGE);
}

#endif

