/*
    NUE module driver

    NT98520 NUE driver.

    @file       nue_platform.c
    @ingroup    mIIPPNUE

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "nue_platform.h"
#include "nue_lib.h"
#include "nue_reg.h"
#include "nue_int.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#if defined(__FREERTOS)
#include "pll.h"
#include "pll_protected.h"
#else
#include <linux/clk.h>
#endif


#if defined(__FREERTOS)
static ID 		  FLG_ID_NUE;
#else
static FLGPTN     FLG_ID_NUE;
#endif

static UINT32 nue_freq_mhz = 0;
#if defined(__FREERTOS)
#else
#define NUE_MODULE_CLK_NUM 1
static struct clk *nue_clk[NUE_MODULE_CLK_NUM];
#endif

#if defined(__FREERTOS)
#else
#if PROF
static struct timeval tstart, tend;
static INT32 is_setmode = 0;
#else
#endif
#endif //#if defined(__FREERTOS)

#if defined(__FREERTOS)
static UINT32 g_nue_true_clock = PLL_CLKSEL_NUE_240;
#else
#endif


VOID nue_pt_sram_switch(VOID) {
#if defined(__FREERTOS)
	UINT32 CNN_SRAM_SRC;
	CNN_SRAM_SRC = PLL_CLKSEL_CNN_SRAM_CNN;
	pll_setClockRate(PLL_CLKSEL_CNN_SRAM, CNN_SRAM_SRC);
#else
	struct clk *parent_clk, *cnn_sram_clk;
	/* Get parent clk
	* The name options are "cnn_sram_cnn", "cnn_sram_dce", "cnn_sram_nue"
	*/
	parent_clk = clk_get(NULL, "cnn_sram_cnn");
	if (IS_ERR(parent_clk))
		pr_err("getting parent_clk error\n");
	/* Get cnn sram control node*/
	cnn_sram_clk = clk_get(NULL, "cnn_shared_sram");
	if (IS_ERR(cnn_sram_clk))
		pr_err("getting cnn_sram_clk error\n");
	/* Set clk select*/
	clk_set_parent(cnn_sram_clk, parent_clk);
#endif
}

VOID nue_pt_request_irq(VOID *isr_handler)
{
#if defined(__FREERTOS)
#if (NUE_TEST_INTERRUPT_EN == ENABLE)
	irq_handler_t isr_func = (irq_handler_t) isr_handler;
    request_irq((INT_ID_NUE + INT_GIC_SPI_START_ID), isr_func, IRQF_TRIGGER_HIGH, "nue", 0);
#endif
#endif
    return;
}


/**
    NUE get engine idle

    Get NUE engine idle

    @return None.
*/
VOID nue_pt_get_engine_idle(VOID)
{
#if defined(__FREERTOS)
    unsigned int count = 0;
#else
    const INT32 time_limit = 3000000;
    struct timeval s_time, e_time;
    INT32 dur_time = 0; // (us)

    do_gettimeofday(&s_time);
#endif

    nue_set_dma_disable(1);
#if defined(__FREERTOS)
    while (nue_get_engine_idle() == 0) {
#else
    while ((nue_get_engine_idle() == 0) && (dur_time < time_limit)) {
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

/**
    NUE set is_setmode

    Set NUE is_setmode

    @return None.
*/
#if PROF
VOID nue_pt_set_is_setmode(INT32 set_value)
{
	is_setmode = set_value;
}
#endif

/**
    NUE enable clock

    Enable NUE engine clock

    @return None.
*/
VOID nue_pt_clk_enable(VOID)
{
#if defined(__FREERTOS)
	pll_enableClock(NUE_CLK);
#else
	clk_prepare(nue_clk[0]);
	clk_enable(nue_clk[0]);
#endif

}

/**
    NUE disable clock

    Disable NUE engine clock

    @return None.
*/
VOID nue_pt_clk_disable(VOID)
{
#if defined(__FREERTOS)
	pll_disableClock(NUE_CLK);
#else
	clk_disable(nue_clk[0]);
	clk_unprepare(nue_clk[0]);
#endif

}

/**
	NUE Create Resource
*/
VOID nue_pt_create_resource(VOID *parm, UINT32 clk_freq)
{
#if defined(__FREERTOS)
#else
	struct clk* module_clk = (struct clk*) parm;
#endif

	OS_CONFIG_FLAG(FLG_ID_NUE);

#if defined(__FREERTOS)
#else
	nue_clk[0] = module_clk;
#endif
	nue_freq_mhz = clk_freq / NUE_1M_HZ;

}

/**
    NUE release flag

    Release NUE interrupt flag

    @return None.
*/
VOID nue_pt_rel_flg(VOID)
{
	rel_flg(FLG_ID_NUE);
}

/**
    NUE wait flag

    Wait NUE interrupt flag

    @return None.
*/
VOID nue_pt_wai_flg(FLGPTN *uiFlag, UINT32 flag_id)
{
	wai_flg(uiFlag, FLG_ID_NUE, flag_id, TWF_CLR | TWF_ORW);
}

/**
    NUE clear flag

    Clear NUE interrupt flag

    @return None.
*/
VOID nue_pt_clr_flg(UINT32 flag_id, BOOL cb_flg)
{
#if defined(__FREERTOS)
	if (cb_flg == 0) {
		clr_flg(FLG_ID_NUE, flag_id);
	}
	else {
		iclr_flg(FLG_ID_NUE, flag_id);
	}
#else
	clr_flg(FLG_ID_NUE, flag_id);
#endif
}

/**
    NUE set flag

    Set NUE interrupt flag

    @return None.
*/
VOID nue_pt_iset_flg(UINT32 flag_id) 
{
	iset_flg(FLG_ID_NUE, flag_id);
}

/*
    NUE while loop routine

    NUE while loop routine

    @return None.
*/
VOID nue_pt_loop_frameend(VOID)
{
	ER er_return;
	UINT32 nue_int_status = 0;
#if defined(__FREERTOS)
    unsigned int count = 0;
#else
    const INT32 time_limit = 3000000;
    struct timeval s_time, e_time;
    INT32 dur_time = 0; // (us)

	do_gettimeofday(&s_time);
#endif

	nue_int_status = nue_get_intr_status();

	er_return = E_OK;	
#if defined(__FREERTOS)
	while ((nue_int_status & NUE_INT_ROUTEND) != NUE_INT_ROUTEND) {
#else
	while (((nue_int_status & NUE_INT_ROUTEND) != NUE_INT_ROUTEND)  && (dur_time < time_limit) ) {
#endif
		nue_int_status = nue_get_intr_status();

#if defined(__FREERTOS)
        vos_task_delay_ms(10);
        count++;
		if ((count % 100) == 0) {
			nvt_dbg(WRN, "NUE loop delay count=%d\r\n", count);
		}
        if (count > 1000) {
            nvt_dbg(WRN, "NUE frame end wait too long(timeout).\r\n");
			er_return = nue_pause();
			if (er_return != E_OK) {
				nvt_dbg(ERR, "NUE Error to do nue_pause. sts=%d\r\n", (int) er_return);
			}
            er_return = nue_close();
			if (er_return != E_OK) {
                nvt_dbg(ERR, "NUE Error to do nue_close. sts=%d\r\n", (int) er_return);
            }
            er_return = nue_open(NULL);
			if (er_return != E_OK) {
                nvt_dbg(ERR, "NUE Error to do nue_open. sts=%d\r\n", (int) er_return);
            }
            break;
        }
#else
        do_gettimeofday(&e_time);
        dur_time = (e_time.tv_sec - s_time.tv_sec) * 1000000 + (e_time.tv_usec - s_time.tv_usec);
#endif

	}
}

/**
    NUE set operation clock

    Set NUE clock selection.

    @param[in] clock setting.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER nue_pt_set_clock_rate(UINT32 clock)
{
#if defined(__FREERTOS)

#if defined(_BSP_NA51055_)
	if (clock >= 600) {
		clock = 480;	//Case 8073 will be failed to wait the interrupt status at clock=600.
	}

	if (clock >= 600) {
        DBG_ERR("Input frequency %d round to 600\r\n", clock);
        g_nue_true_clock = PLL_CLKSEL_NUE_PLL10;
		if(pll_get_pll_enable(PLL_ID_10) == FALSE) {
			pll_set_pll_enable(PLL_ID_10, TRUE);
		}
	} else if (clock > 480) {
		DBG_ERR("Input frequency %d round to 480\r\n", clock);
		g_nue_true_clock = PLL_CLKSEL_NUE_480;
	} else if (clock == 480) {
		g_nue_true_clock = PLL_CLKSEL_NUE_480;
	} else if (clock > 320) {
		DBG_ERR("Input frequency %d round to 320\r\n", clock);
		g_nue_true_clock = PLL_CLKSEL_NUE_320;
		if (pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) {
			pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		}
	} else if (clock == 320) {
		g_nue_true_clock = PLL_CLKSEL_NUE_320;
		if (pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) {
			pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		}
	} else if (clock > 240) {
		DBG_ERR("Input frequency %d round to 240\r\n", clock);
		g_nue_true_clock = PLL_CLKSEL_NUE_240;
	} else if (clock == 240) {
		g_nue_true_clock = PLL_CLKSEL_NUE_240;
	} else {
		DBG_ERR("Illegal input frequency %d\r\n", clock);
		return E_SYS;
	}
	pll_setClockRate(PLL_CLKSEL_NUE, g_nue_true_clock);

#endif

#if defined(_BSP_NA51089_)
	if (clock >= 600) {
		clock = 480;	//Case 8073 will be failed to wait the interrupt status at clock=600.
	}

	if (clock >= 600) {
        DBG_ERR("Input frequency %d round to 600\r\n", clock);
        g_nue_true_clock = PLL_CLKSEL_CNN_PLL9;
		if(pll_get_pll_enable(PLL_ID_9) == FALSE) {
			pll_set_pll_enable(PLL_ID_9, TRUE);
		}
	} else if (clock > 480) {
		DBG_ERR("Input frequency %d round to 480\r\n", clock);
		g_nue_true_clock = PLL_CLKSEL_CNN_PLL12;
		if (pll_get_pll_enable(PLL_ID_12) == FALSE) {
			pll_set_pll_enable(PLL_ID_12, TRUE);
		}
	} else if (clock == 480) {
		g_nue_true_clock = PLL_CLKSEL_CNN_PLL12;
		if (pll_get_pll_enable(PLL_ID_12) == FALSE) {
			pll_set_pll_enable(PLL_ID_12, TRUE);
		}
	} else if (clock > 320) {
		DBG_ERR("Input frequency %d round to 320\r\n", clock);
		g_nue_true_clock = PLL_CLKSEL_CNN_320;
		/*if (pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) {
			pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		}*/
	} else if (clock == 320) {
		g_nue_true_clock = PLL_CLKSEL_CNN_320;
		/*if (pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) {
			pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		}*/
	} else if (clock > 240) {
		DBG_ERR("Input frequency %d round to 240\r\n", clock);
		g_nue_true_clock = PLL_CLKSEL_CNN_240;
	} else if (clock == 240) {
		g_nue_true_clock = PLL_CLKSEL_CNN_240;
	} else {
		DBG_ERR("Illegal input frequency %d\r\n", clock);
		return E_SYS;
	}
	pll_setClockRate(PLL_CLKSEL_NUE, g_nue_true_clock);

#endif


#else

#if 0
	if (clock >= 600) {
		DBG_ERR("Input frequency %d round to 600\r\n", clock);
		g_nue_true_clock = PLL_CLKSEL_NUE_PLL10;
	} else if (clock > 480) {
		DBG_ERR("Input frequency %d round to 480\r\n", clock);
		g_nue_true_clock = PLL_CLKSEL_NUE_480;
	} else if (clock == 480) {
		g_nue_true_clock = PLL_CLKSEL_NUE_480;
	} else if (clock > 320) {
		DBG_ERR("Input frequency %d round to 320\r\n", clock);
		g_nue_true_clock = PLL_CLKSEL_NUE_320;
		if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
			pll_setPLLEn(PLL_ID_FIXED320, TRUE);
		}
	} else if (clock == 320) {
		g_nue_true_clock = PLL_CLKSEL_NUE_320;
		if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
			pll_setPLLEn(PLL_ID_FIXED320, TRUE);
		}
	} else if (clock > 240) {
		DBG_ERR("Input frequency %d round to 240\r\n", clock);
		g_nue_true_clock = PLL_CLKSEL_NUE_240;
	} else if (clock == 240) {
		g_nue_true_clock = PLL_CLKSEL_NUE_240;
	} else {
		DBG_ERR("Illegal input frequency %d\r\n", clock);
		return E_SYS;
	}
#else
	struct clk *pclk;
	if (clock >= 600) {
		pclk = clk_get(NULL, "pll9");
		if (clock > 600) {
			DBG_ERR("nue_pt_set_clock_rate() input frequency %d round to 600\r\n", clock);
		}
	} else if (clock >= 480) {
		//nue_clock = 480;
		pclk = clk_get(NULL, "pll12");
		if (clock > 480) {
			DBG_ERR("nue_pt_set_clock_rate() input frequency %d round to 480\r\n", clock);
		}
	} else if (clock >= 320) {
		//nue_clock = 320;
		pclk = clk_get(NULL, "pllf320");
		if (clock > 320) {
			DBG_ERR("nue_pt_set_clock_rate() input frequency %d round to 320\r\n", clock);
		}
	} else if (clock >= 240) {
		//nue_clock = 240;
		pclk = clk_get(NULL, "fix240m");
		if (clock > 240) {
			DBG_ERR("nue_pt_set_clock_rate() input frequency %d round to 240\r\n", clock);
		}
	} else {
		DBG_ERR("nue_setClockRate() illegal input frequency %u, set as 240\r\n", clock);
		pclk = clk_get(NULL, "fix240m");
	}

	if (IS_ERR(pclk)) {
		printk("%s: get source pll12 fail\r\n", __func__);
	}

	clk_set_parent(nue_clk[0], pclk);
	clk_put(pclk);
#endif

#endif //#if defined(__FREERTOS)

	return E_OK;
}

/**
	NUE va to pa

	Get physical address from virtual address

	@return address
*/

UINT32 nue_pt_va2pa(UINT32 addr)
{
	VOS_ADDR rt;
    rt = vos_cpu_get_phy_addr(addr);
	if (rt == VOS_ADDR_INVALID) {
		DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)addr);
		return 0;
	}
	return rt;
}

VOID nue_pt_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
    vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);
    return;
}

VOID nue_pt_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
    vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);
    return;
}

VOID nue_pt_dma_flush(UINT32 addr, UINT32 size)
{
    vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
    return;
}

/**
    NUE get operation clock

    Get NUE clock selection.

    @param[in] clock setting.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
UINT32 nue_pt_get_clock_rate(VOID)
{
	return nue_freq_mhz;
}

