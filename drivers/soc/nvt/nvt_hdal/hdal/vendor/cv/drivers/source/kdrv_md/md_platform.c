/*
    MDBC module driver

    NT98520 MDBC driver.

    @file       md_platform.c
    @ingroup    mCVAIMD

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "md_platform.h"

static SEM_HANDLE SEMID_MDBC;
#if defined(__FREERTOS)
static ID 		  FLG_ID_MDBC;
#else
static FLGPTN     FLG_ID_MDBC;
#endif

static UINT32 mdbc_freq_mhz=0;
#if defined(__FREERTOS)
#else
struct clk *mdbc_clk[MD_CLK_NUM];
UINT32 _MDBC_REG_BASE_ADDR;
#endif
static UINT32 g_uiMdbcTrueClock;

ER md_platform_sem_signal(void)
{
#if defined __UITRON || defined __ECOS
    return sig_sem(SEMID_MDBC);
#else
    SEM_SIGNAL(SEMID_MDBC);
    return E_OK;
#endif
}

ER md_platform_sem_wait(void)
{
#if defined __UITRON || defined __ECOS
    return wai_sem(SEMID_MDBC);
#else
    return SEM_WAIT(SEMID_MDBC);
#endif
}

ER md_platform_flg_rel(VOID)
{
	return rel_flg(FLG_ID_MDBC);
}
ER md_platform_flg_clear (FLGPTN flg)
{
    return clr_flg(FLG_ID_MDBC, flg);
}
ER md_platform_flg_set (FLGPTN flg)
{
    return iset_flg(FLG_ID_MDBC, flg);
}
ER md_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg)
{
    return wai_flg(p_flgptn, FLG_ID_MDBC, flg, TWF_ORW | TWF_CLR);
}

void md_platform_int_enable(void)
{
	#if defined __UITRON || defined __ECOS
	drv_enableInt(DRV_INT_MDBC);
	#endif
}

void md_platform_int_disable(void)
{
	#if defined __UITRON || defined __ECOS
	drv_disableInt(DRV_INT_MDBC);
	#endif
}

void md_platform_disable_sram_shutdown(void)
{
#if defined __UITRON || defined __ECOS
	pinmux_disable_sram_shutdown(MDBC_SD);
#elif defined(__FREERTOS)
#if defined(_BSP_NA51089_)
    nvt_disable_sram_shutdown(MDBC_SD);
#endif
#else
    nvt_disable_sram_shutdown(MDBC_SD);
#endif
}
void md_platform_enable_sram_shutdown(void)
{
#if defined __UITRON || defined __ECOS
	pinmux_enableSramShutDown(MDBC_SD);
#elif defined(__FREERTOS)
#if defined(_BSP_NA51089_)
    nvt_enable_sram_shutdown(MDBC_SD);
#endif
#else
	nvt_enable_sram_shutdown(MDBC_SD);
#endif
}

void md_platform_enable_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableClock(MDBC_CLK);
#else
	clk_enable(mdbc_clk[0]);
#endif
}

void md_platform_disable_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_disableClock(MDBC_CLK);
#else
	clk_disable(mdbc_clk[0]);
#endif
}

void md_platform_prepare_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableSystemReset(MDBC_RSTN);
	pll_disableSystemReset(MDBC_RSTN);
#else
	clk_prepare(mdbc_clk[0]);
#endif
}

void md_platform_unprepare_clk(void)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
    //pll_enableSystemReset(MDBC_RSTN);
#else
	clk_unprepare(mdbc_clk[0]);
#endif
}

ER md_platform_set_clk_rate(UINT32 uiClock)
{
#ifdef __KERNEL__
    struct clk *parent_clk;
    if (IS_ERR(mdbc_clk[0])) {
		DBG_ERR("MDBC: get clk fail...0x%p\r\n", mdbc_clk[0]);
		return E_SYS;
	}

    if (uiClock >= MDBC_CLK_240) {
		if (uiClock > MDBC_CLK_PLL13)DBG_WRN("MDBC: Input frequency %d round to 240\r\n", uiClock);
        g_uiMdbcTrueClock = 240;
		parent_clk = clk_get(NULL, "fix240m");
    }
    else if (uiClock >= MDBC_CLK_192) {
		if (uiClock > MDBC_CLK_192)DBG_WRN("MDBC: input frequency %d round to 192MHz\r\n", uiClock);
		g_uiMdbcTrueClock = 192;
		parent_clk = clk_get(NULL, "fix192m");
	}
	else if (uiClock >= MDBC_CLK_96) {
		if (uiClock > MDBC_CLK_96)DBG_WRN("MDBC: input frequency %d round to 96MHz\r\n", uiClock);
		g_uiMdbcTrueClock = 96;
		parent_clk = clk_get(NULL, "fix96m");
	}
	else {
		DBG_WRN("MDBC: input frequency %d round to 240MHz\r\n", uiClock);
		g_uiMdbcTrueClock = 240;
		parent_clk = clk_get(NULL, "fix240m");
	}

	clk_set_parent(mdbc_clk[0], parent_clk);
	clk_put(parent_clk);
#else
	static UINT32 md_true_clock = PLL_CLKSEL_MDBC_240;
	if (uiClock >= 240) {
		if (uiClock > 240)DBG_ERR("Input frequency %d round to 240\r\n", uiClock);
		g_uiMdbcTrueClock = 240;
		md_true_clock = PLL_CLKSEL_MDBC_240;
	} else if (uiClock >= 192) {
		if (uiClock > 192)DBG_ERR("Input frequency %d round to 192\r\n", uiClock);
		g_uiMdbcTrueClock = 192;
		md_true_clock = PLL_CLKSEL_MDBC_192;
	} else if (uiClock >= 96) {
		if (uiClock > 96)DBG_ERR("Input frequency %d round to 96\r\n", uiClock);
		g_uiMdbcTrueClock = 240;
		md_true_clock = PLL_CLKSEL_MDBC_96;
	} else {
		DBG_ERR("Illegal input frequency %d\r\n", uiClock);
		return E_SYS;
	}
    pll_setClockRate(PLL_CLKSEL_MDBC, md_true_clock);

#endif
	return E_OK;
}

UINT32 md_platform_get_clk_rate(void)
{
	//return g_uiMdbcTrueClock;
	return mdbc_freq_mhz;
}

UINT32 md_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
   	UINT32 ret = 0;
    vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);///< invalidate
    ret = 0;
	return ret;
}

UINT32 md_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
    UINT32 ret = 0;
    vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);///< clean
    ret = 0;
	return ret;
}

UINT32 md_platform_dma_flush(UINT32 addr, UINT32 size)
{
    UINT32 ret = 0;
    vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
    ret = 0;
	return ret;
}

UINT32 md_platform_va2pa(UINT32 addr)
{
	VOS_ADDR rt;
    rt = vos_cpu_get_phy_addr(addr);
	if (rt == VOS_ADDR_INVALID) {
		DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)addr);
		return 0;
	}
	return rt;
}

#if !(defined __UITRON || defined __ECOS)
#if defined __FREERTOS
static int is_create = 0;
irqreturn_t md_platform_isr(int irq, void *devid)
{
	mdbc_isr();
	return IRQ_HANDLED;
}

void md_platform_create_resource(UINT32 clk_freq)
{
	if (!is_create) {
		OS_CONFIG_FLAG(FLG_ID_MDBC);
		SEM_CREATE(SEMID_MDBC, 1);

		request_irq(INT_ID_MDBC, md_platform_isr, IRQF_TRIGGER_HIGH, "mdbc", 0);
		mdbc_freq_mhz = clk_freq / MDBC_1M_HZ;
		is_create = 1;
	}
}
#else
void md_platform_create_resource(MD_MODULE_INFO *pmodule_info, UINT32 clk_freq)
{
	OS_CONFIG_FLAG(FLG_ID_MDBC);
	SEM_CREATE(SEMID_MDBC, 1);

	mdbc_clk[0] = pmodule_info->pclk[0];
	mdbc_freq_mhz = clk_freq / MDBC_1M_HZ;
	_MDBC_REG_BASE_ADDR = (UINT32)pmodule_info->io_addr[0];
}
#endif
void md_platform_release_resource(void)
{
	rel_flg(FLG_ID_MDBC);
	SEM_DESTROY(SEMID_MDBC);
#if defined __FREERTOS
    is_create = 0;
#endif
}
#endif




//@}
