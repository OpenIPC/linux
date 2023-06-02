/*
    TS MUXER/TS DEMUXER/HWCOPY Engine Integration module driver

    @file       tse_platform.c
    @ingroup    mIDrvMisc_TSE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#include "tse_platform_int.h"
#include "tse_int.h"

#if (defined __UITRON || defined __ECOS)
#else

#define FLGPTN_TSE FLGPTN_BIT(0)
static SEM_HANDLE tse_sem_id;

#if (!defined __FREERTOS)
static struct clk *eng_clk;
#endif
static vk_spinlock_t set_cfg_lock;
static UINT32 tse_reg_base;
static ID tse_flg_id;
static unsigned int init_flag = 0;

#if (defined __FREERTOS)
void tse_platform_set_resource(void)
#else
void tse_platform_set_resource(MODULE_INFO *pmodule_info)
#endif
{
	if (init_flag == 1) {
		return;
	}
	init_flag = 1;

#if (defined __FREERTOS)
	tse_reg_base = IOADDR_TSE_REG_BASE;
	request_irq(INT_ID_TSE, tse_platform_isr, IRQF_TRIGGER_HIGH, "tse", 0);
#else
	eng_clk = pmodule_info->pclk[0];
	tse_reg_base = (UINT32)pmodule_info->io_addr[0];
#endif

	//coverity[side_effect_free]
	vk_spin_lock_init(&set_cfg_lock);
	vos_flag_create(&tse_flg_id, NULL, "tse_flag_id");
	vos_sem_create(&tse_sem_id, 1, "tse_sem_id");
}

void tse_platform_release_resource(void)
{
	if (init_flag == 0) {
		return;
	}
	init_flag = 0;

	vos_flag_destroy(tse_flg_id);
	vos_sem_destroy(tse_sem_id);

#if (defined __FREERTOS)
	free_irq(INT_ID_TSE, 0);
#endif
}

#if (defined __FREERTOS)
irqreturn_t tse_platform_isr(int irq, void *devid)
{
	tse_isr();
	return IRQ_HANDLED;
}

#else
void tse_platform_isr(void)
{
	tse_isr();
}
#endif
#endif

UINT32 tse_platform_attach(UINT32 clk)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)

	pll_setClkAutoGating(TSE_M_GCLK);
	//pll_clearClkAutoGating(TSE_M_GCLK);
    pll_setPclkAutoGating(TSE_GCLK);

	// Select TSE Module Source clock.
	switch (clk) {
		case 320:
			pll_setClockRate(PLL_CLKSEL_TSE, PLL_CLKSEL_TSE_320);
			break;

		case 300:
			if (pll_getPLLEn(PLL_ID_9) == DISABLE) {
				pll_setPLLEn(PLL_ID_9, ENABLE);
			}
			pll_setClockRate(PLL_CLKSEL_TSE, PLL_CLKSEL_TSE_PLL9);
			break;

		default:
			pll_setClockRate(PLL_CLKSEL_TSE, PLL_CLKSEL_TSE_240);
			break;
	}

	// Enable TSE Module Source clock.
	pll_enableClock(TSE_CLK);

	// Disable TSE Module SRAM Shut Down
	nvt_disable_sram_shutdown(TSE_SD);

	return 0;
#else
	struct clk *src_clk = NULL;

	/****** how to replace ?? ******/
	//pll_clearClkAutoGating(TSE_M_GCLK);
    //pll_clearPclkAutoGating(TSE_M_GCLK);
    /****** how to replace ?? ******/

	// Select TSE Module Source clock.
	switch (clk) {

		case 320:
			src_clk = clk_get(NULL, "pllf320");
			break;

		case 300:
			src_clk = clk_get(NULL, "pll9");
			break;

		default:
			src_clk = clk_get(NULL, "fix240m");
			break;
	}

	clk_set_parent(eng_clk, src_clk);
	clk_put(src_clk);

	// Enable TSE Module Source clock.
	clk_enable(eng_clk);

	nvt_disable_sram_shutdown(TSE_SD);
	return 0;
#endif
}

UINT32 tse_platform_detach(void)
{
#if (defined __UITRON || defined __ECOS || defined __FREERTOS)
	// Enable TSE Module SRAM Shut Down
	nvt_enable_sram_shutdown(TSE_SD);
	// clear the interrupt flag
	tse_platform_clr_flg();

	// Disable TSE Module Source clock.
	pll_disableClock(TSE_CLK);

	return 0;
#else
	// Enable TSE Module SRAM Shut Down
	nvt_enable_sram_shutdown(TSE_SD);

	// clear the interrupt flag
	tse_platform_clr_flg();

	// Disable TSE Module Source clock.
	clk_disable(eng_clk);
	return 0;
#endif
}

UINT32 tse_platform_get_reg_base(void)
{
#if (defined __UITRON || defined __ECOS)
	return IOADDR_TSE_REG_BASE;
#else
	return tse_reg_base;
#endif
}

UINT32 tse_platform_get_phy_addr(UINT32 addr)
{
	VOS_ADDR rt;

	rt = vos_cpu_get_phy_addr(addr);
	if (rt == VOS_ADDR_INVALID) {
		DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)addr);
		return 0;
	}
	return rt;
}

UINT32 tse_platform_disable_int(void)
{
#if (defined __UITRON || defined __ECOS)
	// Disable interrupt
	drv_disableInt(DRV_INT_TSE);
#else
#endif
	return 0;
}

UINT32 tse_platform_enable_int(void)
{
#if (defined __UITRON || defined __ECOS)
	drv_enableInt(DRV_INT_TSE);
#else
#endif
	return 0;
}

UINT32 tse_platform_cache_sync_dma_to_dev(UINT32 addr, UINT32 size)
{
	vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);
	return 0;
}

UINT32 tse_platform_cache_sync_dma_from_dev(UINT32 addr, UINT32 size)
{
	vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);
	return 0;
}

void tse_platform_clr_flg(void)
{
	vos_flag_iclr(tse_flg_id, FLGPTN_TSE);
}

void tse_platform_set_flg(void)
{
	vos_flag_iset(tse_flg_id, FLGPTN_TSE);
}

void tse_platform_wai_flg(void)
{
	FLGPTN uiFlag;
	vos_flag_wait(&uiFlag, tse_flg_id, FLGPTN_TSE, TWF_ORW | TWF_CLR);
}

unsigned long tse_platform_loc_cpu(void)
{
#if (defined __UITRON || defined __ECOS)
	loc_cpu();
	return 0;
#else
	unsigned long flags;
	vk_spin_lock_irqsave(&set_cfg_lock, flags);
	return flags;
#endif
}

void tse_platform_unl_cpu(unsigned long flag)
{
#if (defined __UITRON || defined __ECOS)
	unl_cpu();
#else
	vk_spin_unlock_irqrestore(&set_cfg_lock, flag);
#endif
}

UINT32 tse_platform_wait_sem(void)
{
	vos_sem_wait(tse_sem_id);
	return 0;
}

UINT32 tse_platform_sig_sem(void)
{
	vos_sem_sig(tse_sem_id);
	return 0;
}
