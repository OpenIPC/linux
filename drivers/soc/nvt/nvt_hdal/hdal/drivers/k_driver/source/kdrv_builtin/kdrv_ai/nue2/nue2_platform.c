/*
    NUE2 module driver

    NT98520 NUE2 driver.

    @file       nue2_platform.c
    @ingroup    mIIPPNUE2

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "nue2_platform.h"
#if defined(__FREERTOS)
#include "pll.h"
#include "pll_protected.h"
#else
#include <linux/clk.h>
#include "mach/fmem.h" //for fmem_lookup_pa, PAGE_ALIGN
#endif
#include "kwrap/task.h"
#include "kwrap/cpu.h"
#include "kwrap/flag.h"
#include "kwrap/mem.h"
#include "kwrap/semaphore.h"
#include "nue2_lib.h"
#include "nue2_reg.h"
#include "nue2_int.h"

#define NUE2_MODULE_CLK_NUM 1

#if defined(__FREERTOS)
static ID 		  FLG_ID_NUE2;
#else
static FLGPTN     FLG_ID_NUE2;
#endif

static UINT32 nue2_freq_mhz=0;
#if defined(__FREERTOS)
#else
static struct clk *nue2_clk[NUE2_MODULE_CLK_NUM];
#endif

#if defined(__FREERTOS)
static UINT32 g_nue2_true_clock = PLL_CLKSEL_NUE2_240;
static UINT8 g_nue2_int_en=1;
#else
#endif

#if defined(__FREERTOS)
VOID nue2_pt_interrupt_en(UINT8 is_en)
{
	g_nue2_int_en = is_en;
	return;
}
#endif

VOID nue2_pt_request_irq(VOID *isr_handler)
{
#if defined(__FREERTOS)
	if (isr_handler && (g_nue2_int_en == 1)) {
		irq_handler_t isr_func = (irq_handler_t) isr_handler;
    	request_irq((INT_ID_NUE2 + INT_GIC_SPI_START_ID), isr_func, IRQF_TRIGGER_HIGH, "nue2", 0);
	}
#endif
	return;
}

VOID nue2_pt_clk_enable(VOID)
{
#if defined(__FREERTOS)
	pll_enableClock(NUE2_CLK);
#else
	clk_prepare(nue2_clk[0]);
	clk_enable(nue2_clk[0]);
#endif
}

VOID nue2_pt_clk_disable(VOID)
{
#if defined(__FREERTOS)
	pll_disableClock(NUE2_CLK);
#else
	clk_disable(nue2_clk[0]);
	clk_unprepare(nue2_clk[0]);
#endif
}


/**
	NUE2 Create Resource
*/
VOID nue2_pt_create_resource(VOID *parm, UINT32 clk_freq)
{
#if defined(__FREERTOS)
#else
	struct clk* module_clk = (struct clk*) parm;
#endif

	OS_CONFIG_FLAG(FLG_ID_NUE2);

#if defined(__FREERTOS)
#else
	nue2_clk[0] = module_clk;
#endif
	nue2_freq_mhz = clk_freq / NUE2_1M_HZ;
}

VOID nue2_pt_rel_flg(VOID)
{
	rel_flg(FLG_ID_NUE2);
}

VOID nue2_pt_wai_flg(FLGPTN *uiFlag, UINT32 flag_id)
{
	wai_flg(uiFlag, FLG_ID_NUE2, flag_id, TWF_CLR | TWF_ORW);
}

VOID nue2_pt_clr_flg(UINT32 flag_id, BOOL cb_flg)
{
#if defined(__FREERTOS)
	if (cb_flg == 0) {
		clr_flg(FLG_ID_NUE2, flag_id);
	}
	else {
		iclr_flg(FLG_ID_NUE2, flag_id);
	}
#else
	clr_flg(FLG_ID_NUE2, flag_id);
#endif
}

VOID nue2_pt_iset_flg(UINT32 flag_id) 
{
	iset_flg(FLG_ID_NUE2, flag_id);
}

/**
    @addtogroup mIIPPNUE2
*/
//@{

UINT32 nue2_pt_va2pa(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
    return dma_getPhyAddr(addr);
#elif defined (__FREERTOS)
    return vos_cpu_get_phy_addr(addr);
#else
	return vos_cpu_get_phy_addr(addr);
#endif
}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
VOID nue2_pt_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
    vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);
    return;
}

VOID nue2_pt_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
    vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);
    return;
}

VOID nue2_pt_dma_flush(UINT32 addr, UINT32 size)
{
    vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
    return;
}
#endif

/**
    NUE2 get operation clock

    Get NUE2 clock selection.

    @param[in] clock setting.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
UINT32 nue2_pt_get_clock_rate(VOID)
{
	return nue2_freq_mhz;
}


/**
    NUE2 set operation clock

    Set NUE2 clock selection.

    @param[in] clock setting.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue2_pt_set_clock_rate(UINT32 clock)
{
#if defined(__FREERTOS)

#if defined(_BSP_NA51055_)
	if (clock >= 600) {
        DBG_ERR("Input frequency %d round to 600\r\n", clock);
        g_nue2_true_clock = PLL_CLKSEL_NUE2_PLL13;
	} else if (clock > 480) {
		DBG_ERR("Input frequency %d round to 480\r\n", clock);
		g_nue2_true_clock = PLL_CLKSEL_NUE2_480;
	} else if (clock == 480) {
		g_nue2_true_clock = PLL_CLKSEL_NUE2_480;
	} else if (clock > 320) {
		DBG_ERR("Input frequency %d round to 320\r\n", clock);
		g_nue2_true_clock = PLL_CLKSEL_NUE2_320;
		if (pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) {
			pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		}
	} else if (clock == 320) {
		g_nue2_true_clock = PLL_CLKSEL_NUE2_320;
		if (pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) {
			pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		}
	} else if (clock > 240) {
		DBG_ERR("Input frequency %d round to 240\r\n", clock);
		g_nue2_true_clock = PLL_CLKSEL_NUE2_240;
	} else if (clock == 240) {
		g_nue2_true_clock = PLL_CLKSEL_NUE2_240;
	} else {
		DBG_ERR("Illegal input frequency %d\r\n", clock);
		return E_SYS;
	}
	pll_setClockRate(PLL_CLKSEL_NUE2, g_nue2_true_clock);

#endif
#if defined(_BSP_NA51089_)
	if (clock >= 600) {
        DBG_ERR("Input frequency %d round to 600\r\n", clock);
        g_nue2_true_clock = PLL_CLKSEL_NUE2_PLL13;
	} else if (clock > 480) {
		DBG_ERR("Input frequency %d round to 480\r\n", clock);
		g_nue2_true_clock = PLL_CLKSEL_NUE2_480;
	} else if (clock == 480) {
		g_nue2_true_clock = PLL_CLKSEL_NUE2_480;
	} else if (clock > 320) {
		DBG_ERR("Input frequency %d round to 320\r\n", clock);
		g_nue2_true_clock = PLL_CLKSEL_NUE2_320;
	} else if (clock == 320) {
		g_nue2_true_clock = PLL_CLKSEL_NUE2_320;
	} else if (clock > 240) {
		DBG_ERR("Input frequency %d round to 240\r\n", clock);
		g_nue2_true_clock = PLL_CLKSEL_NUE2_240;
	} else if (clock == 240) {
		g_nue2_true_clock = PLL_CLKSEL_NUE2_240;
	} else {
		DBG_ERR("Illegal input frequency %d\r\n", clock);
		return E_SYS;
	}
	pll_setClockRate(PLL_CLKSEL_NUE2, g_nue2_true_clock);

#endif


#else
#if 0
	if (clock >= 600) {
		DBG_ERR("Input frequency %d round to 600\r\n", clock);
		g_nue2_true_clock = PLL_CLKSEL_NUE2_PLL13;
	} else if (clock > 480) {
		DBG_ERR("Input frequency %d round to 480\r\n", clock);
		g_nue2_true_clock = PLL_CLKSEL_NUE2_480;
	} else if (clock == 480) {
		g_nue2_true_clock = PLL_CLKSEL_NUE2_480;
	} else if (clock > 320) {
		DBG_ERR("Input frequency %d round to 320\r\n", clock);
		g_nue2_true_clock = PLL_CLKSEL_NUE2_320;
		if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
			pll_setPLLEn(PLL_ID_FIXED320, TRUE);
		}
	} else if (clock == 320) {
		g_nue2_true_clock = PLL_CLKSEL_NUE2_320;
		if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
			pll_setPLLEn(PLL_ID_FIXED320, TRUE);
		}
	} else if (clock > 240) {
		DBG_ERR("Input frequency %d round to 240\r\n", clock);
		g_nue2_true_clock = PLL_CLKSEL_NUE2_240;
	} else if (clock == 240) {
		g_nue2_true_clock = PLL_CLKSEL_NUE2_240;
	} else {
		DBG_ERR("Illegal input frequency %d\r\n", clock);
		return E_SYS;
	}
#else
	struct clk *pclk;

	if (clock >= 600) {
        pclk = clk_get(NULL, "pll13");
        if (clock > 600) {
            DBG_ERR("nue_set_clock_rate() input frequency %d round to 600\r\n", clock);
        }
    } else if (clock >= 480) {
		//nue_clock = 480;
		pclk = clk_get(NULL, "fix480m");
		if (clock > 480) {
			DBG_ERR("nue2_set_clock_rate() input frequency %d round to 480\r\n", clock);
		}
	} else if (clock >= 320) {
		//nue_clock = 320;
		pclk = clk_get(NULL, "pllf320");
		if (clock > 320) {
			DBG_ERR("nue2_set_clock_rate() input frequency %d round to 320\r\n", clock);
		}
	} else if (clock >= 240) {
		//nue_clock = 240;
		pclk = clk_get(NULL, "fix240m");
		if (clock > 240) {
			DBG_ERR("nue2_set_clock_rate() input frequency %d round to 240\r\n", clock);
		}
	} else {
		DBG_ERR("nue2_setClockRate() illegal input frequency %u, set as 240\r\n", clock);
		pclk = clk_get(NULL, "fix240m");
	}

	if (IS_ERR(pclk)) {
		printk("%s: get source fix480m fail\r\n", __func__);
	}

	clk_set_parent(nue2_clk[0], pclk);
	clk_put(pclk);
#endif

#endif //#if defined(__FREERTOS)

	return E_OK;
}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
VOID nue2_pt_ch_enable(VOID)
{
	//DMA proposal
	nvt_dbg(ERR, "%s_%d:TODO\r\n", __FUNCTION__, __LINE__);
	return;
}

VOID nue2_pt_ch_disable(VOID)
{
	//DMA proposal
	nvt_dbg(ERR, "%s_%d:TODO\r\n", __FUNCTION__, __LINE__);
	return;
}

VOID nue2_pt_dma_test(UINT8 is_en)
{
	nvt_dbg(ERR, "%s_%d:TODO\r\n", __FUNCTION__, __LINE__);

    if (is_en == ENABLE) {
        //dma_channel_enable(M4_NUE2);
    } else {
        //dma_channel_disable(M4_NUE2);
    }
	return;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

VOID nue2_pt_get_engine_idle(VOID)
{
#if defined(__FREERTOS)
    unsigned int count = 0;
#else
    const INT32 time_limit = 3000000;
    struct timeval s_time, e_time;
    INT32 dur_time = 0; // (us)

    do_gettimeofday(&s_time);
#endif
    
#if defined(__FREERTOS)
    while (nue2_get_engine_idle() == 0) {
#else
    while ((nue2_get_engine_idle() == 0) && (dur_time < time_limit)) {
#endif
#if defined(__FREERTOS)
        vos_task_delay_ms(10);
        count++;
        if (count > 1000) {
            nvt_dbg(WRN, "disable dma time out\r\n");
            break;
        }
#else
        do_gettimeofday(&e_time);
        dur_time = (e_time.tv_sec - s_time.tv_sec) * 1000000 + (e_time.tv_usec - s_time.tv_usec);
#endif
    }

#if defined(__FREERTOS)
#else
    if (dur_time > time_limit) {
        nvt_dbg(WRN, "disable dma time out\r\n");
    }
#endif

    return;
}

#if (NUE2_SYS_VFY_EN == ENABLE)
UINT32 nue2_pt_pa2va_remap(UINT32 pa, UINT32 sz, UINT8 is_mem2dev)
{
#if defined(__FREERTOS)
    return pa;
#else
    UINT32 va = 0;
    if (sz == 0) {
        return va;
    }
#if 0 //defined(_BSP_NA51055_)
    va = nvtmpp_sys_pa2va(pa);
#else
    if (pfn_valid(__phys_to_pfn(pa))) {
        va = (UINT32)__va(pa);
    } else {
        va = (UINT32)ioremap(pa, PAGE_ALIGN(sz));
    }
#endif
    //fmem_dcache_sync((UINT32 *)va, sz, DMA_BIDIRECTIONAL);
    if (is_mem2dev == 1) {
        vos_cpu_dcache_sync(va, sz, VOS_DMA_TO_DEVICE); ///< cache clean - output to engine's input
    } else if (is_mem2dev == 0) {
        vos_cpu_dcache_sync(va, sz, VOS_DMA_FROM_DEVICE); ///< cache clean - output from engine
    } else {
        //Do not cache sync.
    }

    return va;
#endif
}

VOID nue2_pt_pa2va_unmap(UINT32 va, UINT32 pa)
{
#if defined(__FREERTOS)
    return;
#else
    if (va == 0) {
        return;
    }
#if 0 //defined(_BSP_NA51055_)
#else
    if (!pfn_valid(__phys_to_pfn(pa))) {
        iounmap((VOID *)va);
    }
#endif
#endif
}
#endif //#if (NUE2_SYS_VFY_EN == ENABLE)

//@}
