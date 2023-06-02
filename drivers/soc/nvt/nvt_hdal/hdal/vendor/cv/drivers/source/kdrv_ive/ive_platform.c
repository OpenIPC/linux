/*
    IVE module driver

    NT98520 IVE driver.

    @file       ive_platform.c
    @ingroup    mCVAIMD

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "ive_platform.h"

#if defined(__FREERTOS)
#include <rtosfdt.h>
#include <stdio.h>
#include <libfdt.h>
#include <compiler.h>
#include "ive_drv.h"

#include "pll.h"
#include "pll_protected.h"
#else
#include <linux/clk.h>
#include <linux/hrtimer.h>
#include "mach/fmem.h" //for fmem_lookup_pa, PAGE_ALIGN
#endif
#include "kwrap/cpu.h"
#include "kwrap/flag.h"
#include "kwrap/util.h"
#include "kwrap/mem.h"
#include "kwrap/semaphore.h"
#include "ive_lib.h"
#include "ive_reg.h"
#include "ive_int.h"


static SEM_HANDLE SEMID_IVE;
#if defined(__FREERTOS)
static ID 		  FLG_ID_IVE;
#else
static FLGPTN     FLG_ID_IVE;
#endif

#define IVE_CLK_NUM 1


#if defined(__FREERTOS)
#else
struct clk *ive_clk[IVE_CLK_NUM]={0};
#endif

#if defined(__FREERTOS)
UINT32 g_uiIveClockRateDefine = PLL_CLKSEL_IVE_240;
#endif

// ==================================================================
// Flag
// ==================================================================
#if defined(__FREERTOS)
ID  kdrv_ive_flag_id[KDRV_IVE_FLAG_COUNT] = {0};
#else
UINT32 kdrv_ive_flag_id[KDRV_IVE_FLAG_COUNT] = {0};
#endif


#if defined __UITRON || defined __ECOS
KDRV_IVE_SEM_TABLE kdrv_ive_semtbl[KDRV_IVE_SEMAPHORE_COUNT] = {
	// Cnt > 0, Max >= Cnt, ExtInfo & Attr is not referenced now
	// The sequence must be sync to enum member in DRV_SEMAPHORE
//  Semaphore ID,       Attr    Counter     Max counter
	{0,    0,      1,          SEM_MAX_CNT},   	//SEMID_KDRV_RHE,
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IVE,
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IVE,
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IPE,
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IME,
	{0,    0,      1,          SEM_MAX_CNT},    //SEMID_KDRV_IFE2,
};
#elif defined(__FREERTOS)
KDRV_IVE_SEM_TABLE kdrv_ive_semtbl[KDRV_IVE_SEMAPHORE_COUNT] = {
	// Cnt > 0, Max >= Cnt, ExtInfo & Attr is not referenced now
	// The sequence must be sync to enum member in DRV_SEMAPHORE
//  Semaphore ID,       Attr    Counter     Max counter
	{0,    0,      1,          SEM_MAX_CNT},   	//SEMID_KDRV_RHE,
};
#else
KDRV_IVE_SEM_TABLE kdrv_ive_semtbl[KDRV_IVE_SEMAPHORE_COUNT];
#endif

#if defined __UITRON || defined __ECOS
static BOOL g_ive_is_open_timer[KDRV_IVE_HANDLE_MAX_NUM] = {TRUE};
static SWTIMER_ID   g_ive_timer_id[KDRV_IVE_HANDLE_MAX_NUM];

#elif defined(__FREERTOS)
unsigned int ive_clock_freq[MODULE_CLK_NUM];
#else
static struct hrtimer git_ive_htimer[KDRV_IVE_HANDLE_MAX_NUM];
static ktime_t g_ive_kt_periode[KDRV_IVE_HANDLE_MAX_NUM];
#endif

static UINT32 ive_freq_mhz=0;
static UINT32 g_ive_interrupt_en = 1;

extern VOID ive_isr(VOID);

#if defined(__FREERTOS)
UINT32 ive_platform_get_clock_freq_from_dtsi(UINT8 clk_idx)
{
	unsigned char *fdt_addr = (unsigned char *)fdt_get_base();
	char path[64] = {0};
	INT nodeoffset;
	UINT32 *cell = NULL;
	UINT32 idx;

	for(idx = 0; idx < MODULE_CLK_NUM; idx++) {
		ive_clock_freq[idx] = 0;
	}
	sprintf(path,"/ive@f0d70000");
	nodeoffset = fdt_path_offset((const void*)fdt_addr, path);

	cell = (UINT32 *)fdt_getprop((const void*)fdt_addr, nodeoffset, "clock-frequency", NULL);
	if (cell) {
		ive_clock_freq[0] = be32_to_cpu(cell[0]);
	}

	//DBG_ERR("IVE_RTOS: ive_clock(%d)\r\n", ive_clock_freq[0]);	

	return ive_clock_freq[clk_idx];
}
#endif

VOID ive_platform_kdrv_ive_frm_end(KDRV_IVE_HANDLE* p_handle, BOOL flag)
{
	if (flag == TRUE) {
		set_flg(p_handle->flag_id, KDRV_IVE_FMD);
	} else {
		clr_flg(p_handle->flag_id, KDRV_IVE_FMD);
	}
}

ER ive_platform_sem_signal(VOID)
{
#if defined __UITRON || defined __ECOS
    return sig_sem(SEMID_IVE);
#else
    SEM_SIGNAL(SEMID_IVE);
    return E_OK;
#endif
}

ER ive_platform_sem_destroy(VOID)
{
#if defined __UITRON || defined __ECOS
	DBG_ERR("Error, TODO: %s_%d\r\n", __FUNCTION__, __LINE__);	
#else
	SEM_DESTROY(SEMID_IVE);
    return E_OK;
#endif
}

ER ive_platform_sem_wait(VOID)
{
#if defined __UITRON || defined __ECOS
    return wai_sem(SEMID_IVE);
#else
    return SEM_WAIT(SEMID_IVE);
#endif
}

ER ive_platform_flg_rel(VOID)
{
	return rel_flg(FLG_ID_IVE);
}

ER ive_platform_flg_clear(UINT32 flg)
{
    return clr_flg(FLG_ID_IVE, flg);
}

ER ive_platform_flg_set(UINT32 flg)
{
    return iset_flg(FLG_ID_IVE, flg);
}

ER ive_platform_flg_wait(PFLGPTN p_flgptn, UINT32 flg)
{
    return wai_flg(p_flgptn, FLG_ID_IVE, flg, TWF_ORW | TWF_CLR);
}

ER ive_platform_flg_wait_timeout(PFLGPTN p_flgptn, UINT32 flg, UINT32 timeout_ms)
{
	ER erReturn = E_OK;
	INT32 ret;

    //return wai_flg(p_flgptn, FLG_ID_IVE, flg, TWF_ORW | TWF_CLR);
	ret = vos_flag_wait_timeout(p_flgptn, FLG_ID_IVE, flg, (TWF_ORW | TWF_CLR), vos_util_msec_to_tick(timeout_ms));
    if (ret < 0) {
		erReturn = E_PAR;
		goto err_exit;
    }

err_exit:

	return erReturn;
}

VOID ive_platform_int_enable(VOID)
{
	#if defined __UITRON || defined __ECOS
	drv_enableInt(DRV_INT_IVE);
	#endif
}

VOID ive_platform_int_disable(VOID)
{
	#if defined __UITRON || defined __ECOS
	drv_disableInt(DRV_INT_IVE);
	#endif
}

VOID ive_platform_disable_sram_shutdown(VOID)
{
#if defined __UITRON || defined __ECOS
	pinmux_disable_sram_shutdown(IVE_SD);
#elif defined(__FREERTOS)
#if defined(_BSP_NA51055_)
    nvt_disable_sram_shutdown(IVE_SD);
#endif
#else
    nvt_disable_sram_shutdown(IVE_SD);
#endif
}

VOID ive_platform_enable_sram_shutdown(VOID)
{
#if defined __UITRON || defined __ECOS
	pinmux_enableSramShutDown(IVE_SD);
#elif defined(__FREERTOS)
#if defined(_BSP_NA51055_)
    nvt_enable_sram_shutdown(IVE_SD);
#endif
#else
	nvt_enable_sram_shutdown(IVE_SD);
#endif
}

VOID ive_platform_enable_clk(VOID)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_enableSystemReset(IVE_RSTN);
    pll_disableSystemReset(IVE_RSTN);
	pll_enableClock(IVE_CLK);
#else
	clk_prepare(ive_clk[0]);
    clk_enable(ive_clk[0]);
#endif
}

VOID ive_platform_disable_clk(VOID)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	pll_disableClock(IVE_CLK);
#else
	clk_disable(ive_clk[0]);
	clk_unprepare(ive_clk[0]);
#endif
}

/**
    IVE get operation clock

    Get IVE clock selection.

    @param[in] clock setting.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
UINT32 ive_platform_get_clock_rate(VOID)
{
	return ive_freq_mhz;
}

ER ive_platform_set_clk_rate(UINT32 uiClock)
{
	ER erReturn = E_OK;

#if defined(__FREERTOS)
	if (uiClock >= IVE_CLK_VAL_480) {
		g_uiIveClockRateDefine = PLL_CLKSEL_IVE_480;
		if (uiClock != IVE_CLK_VAL_480) {
			DBG_ERR("Can't support %d. Clock rate is set to %d MHz.\r\n", uiClock, IVE_CLK_VAL_480);
		}
	} else if (uiClock < IVE_CLK_VAL_480 && uiClock >= IVE_CLK_VAL_320) {
		g_uiIveClockRateDefine = PLL_CLKSEL_IVE_320;
		if (g_uiIveClockRateDefine == PLL_CLKSEL_IVE_320) {
			if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
				pll_setPLLEn(PLL_ID_FIXED320, TRUE);
			}
		}
		if (uiClock != IVE_CLK_VAL_320) {
			DBG_ERR("Can't support %d. Clock rate is set to %d MHz.\r\n", uiClock, IVE_CLK_VAL_320);
		}
	} else if (uiClock < IVE_CLK_VAL_320 && uiClock >= IVE_CLK_VAL_PLL13) {
		g_uiIveClockRateDefine = PLL_CLKSEL_IVE_PLL13;
		if (g_uiIveClockRateDefine == PLL_CLKSEL_IVE_PLL13) {
			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}
		if (uiClock != IVE_CLK_VAL_PLL13) {
			DBG_ERR("Can't support %d. Clock rate is set to %d MHz.\r\n", uiClock, IVE_CLK_VAL_PLL13);
		}
	} else if (uiClock < IVE_CLK_VAL_PLL13 && uiClock >= IVE_CLK_VAL_240) {
		g_uiIveClockRateDefine = PLL_CLKSEL_IVE_240;
		if (uiClock != IVE_CLK_VAL_240) {
			DBG_ERR("Can't support %d. Clock rate is set to %d MHz.\r\n", uiClock, IVE_CLK_VAL_240);
		}
	} else if (uiClock < IVE_CLK_VAL_240) {
		g_uiIveClockRateDefine = PLL_CLKSEL_IVE_240;
		DBG_ERR("Can't support %d. Clock rate is set to %d MHz.\r\n", uiClock, IVE_CLK_VAL_240);
	}
	pll_setClockRate(PLL_CLKSEL_IVE, g_uiIveClockRateDefine);

#else
	struct clk *source_clk = NULL;

	if (uiClock >= IVE_CLK_VAL_480) {
		source_clk = clk_get(NULL, "fix480m");
		if (uiClock != IVE_CLK_VAL_480) {
			DBG_ERR("Can't support %d. Clock rate is set to %d MHz.\r\n", uiClock, IVE_CLK_VAL_480);
		}
	} else if (uiClock < IVE_CLK_VAL_480 && uiClock >= IVE_CLK_VAL_320) {
		source_clk = clk_get(NULL, "pllf320");
		if (uiClock != IVE_CLK_VAL_320) {
			DBG_ERR("Can't support %d. Clock rate is set to %d MHz.\r\n", uiClock, IVE_CLK_VAL_320);
		}
	} else if (uiClock < IVE_CLK_VAL_320 && uiClock >= IVE_CLK_VAL_240) {
		source_clk = clk_get(NULL, "fix240m");
		if (uiClock != IVE_CLK_VAL_PLL13) {
			DBG_ERR("Can't support %d. Clock rate is set to %d MHz.\r\n", uiClock, IVE_CLK_VAL_PLL13);
		}
	} else if (uiClock < IVE_CLK_VAL_240) {
		source_clk = clk_get(NULL, "fix240m");
		DBG_ERR("Can't support %d. Clock rate is set to %d MHz.\r\n", uiClock, IVE_CLK_VAL_240);
	}

	if (ive_clk[0] == 0) {
		DBG_ERR("Error to get ive_clk=0. \r\n");
	} else {
		clk_set_parent(ive_clk[0], source_clk);
		clk_put(source_clk);
	}
#endif

	return erReturn;
}

UINT32 ive_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
   	UINT32 ret = 0;

    vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);
    ret = 0;
	return ret;
}

UINT32 ive_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
    UINT32 ret = 0;

    vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);
    ret = 0;
	return ret;
}

UINT32 ive_platform_dma_flush(UINT32 addr, UINT32 size)
{
   	UINT32 ret = 0;

    vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
    ret = 0;
	return ret;
}

UINT32 ive_platform_va2pa(UINT32 addr)
{
#if defined __UITRON || defined __ECOS
	return dma_getPhyAddr(addr);
#elif defined(__FREERTOS)
	VOS_ADDR rt;
    rt = vos_cpu_get_phy_addr(addr);
	if (rt == VOS_ADDR_INVALID) {
		DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)addr);
		return 0;
	}
	return rt;
#else
	VOS_ADDR rt;
    rt = vos_cpu_get_phy_addr(addr);
	if (rt == VOS_ADDR_INVALID) {
		DBG_ERR("addr(0x%.8x) conv fail\r\n", (int)addr);
		return 0;
	}
	return rt;
#endif
}

UINT32 ive_platform_pa2va_remap(UINT32 pa, UINT32 sz, UINT8 is_mem2dev)
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

VOID ive_platform_pa2va_unmap(UINT32 va, UINT32 pa)
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

void ive_platform_isr(int irq, VOID *devid)
{
	ive_isr();
	return;
}

VOID ive_platform_interrupt_en(UINT32 is_en)
{
	g_ive_interrupt_en = is_en;
}

VOID ive_platform_create_resource(VOID *parm, UINT32 clk_freq)
{
#if defined(__FREERTOS)
	irq_handler_t isr_func;

	if (g_ive_interrupt_en) {
		isr_func = (irq_handler_t) ive_platform_isr;
	} else {
		isr_func = 0;
	}

	if (parm == NULL) {
		OS_CONFIG_FLAG(FLG_ID_IVE);
		SEM_CREATE(SEMID_IVE, 1);
		ive_set_base_addr((UINT32) (IVE_BASE_ADDR));
		kdrv_ive_install_id();
		kdrv_ive_init();
		if (g_ive_interrupt_en) {
			request_irq((INT_ID_IVE  + INT_GIC_SPI_START_ID), isr_func, IRQF_TRIGGER_HIGH, "ive", 0);
		}
	}
#else
	if (parm) {
		OS_CONFIG_FLAG(FLG_ID_IVE);
		SEM_CREATE(SEMID_IVE, 1);
		ive_clk[0] = (struct clk*) parm;
	}
#endif
	if (ive_freq_mhz == 0) {
		ive_freq_mhz = clk_freq / IVE_1M_HZ;
	}
}

VOID ive_platform_release_resource(VOID *parm)
{
	ER erReturn = E_OK;

#if defined(__FREERTOS)
	if (parm == NULL) {
		erReturn = ive_platform_flg_rel();
		if (erReturn != E_OK) {
			DBG_ERR("Error! %s_%d: ive_platform_flg_rel(). %d\n", __FUNCTION__, __LINE__, erReturn);
		}

		erReturn = ive_platform_sem_destroy();
		if (erReturn != E_OK) {
			DBG_ERR("Error! %s_%d: ive_platform_sem_destroy() %d\n", __FUNCTION__, __LINE__, erReturn);
		}
		kdrv_ive_uninstall_id();
	}
#else
	if (parm) {
		erReturn = ive_platform_flg_rel();
		if (erReturn != E_OK) {
			DBG_ERR("Error! %s_%d: ive_platform_flg_rel(). %d\n", __FUNCTION__, __LINE__, erReturn);
		}

		erReturn = ive_platform_sem_destroy();
		if (erReturn != E_OK) {
			DBG_ERR("Error! %s_%d: ive_platform_sem_destroy() %d\n", __FUNCTION__, __LINE__, erReturn);
		}
	}
#endif

}

VOID ive_platform_create_resource_config(UINT32 idx)
{

#if defined __UITRON || defined __ECOS
	OS_CONFIG_SEMPHORE(kdrv_ive_semtbl[idx].semphore_id, kdrv_ive_semtbl[idx].attribute, kdrv_ive_semtbl[idx].counter, kdrv_ive_semtbl[idx].max_counter);
#else
	SEM_CREATE(kdrv_ive_semtbl[idx].semphore_id, SEM_MAX_CNT); //both linux and RTOS
#endif

}

VOID ive_platform_rel_id_config(UINT32 idx)
{
#if defined __UITRON || defined __ECOS
#else
	rel_flg(kdrv_ive_flag_id[idx]);
#endif
}

VOID ive_platform_rel_sem_config(UINT32 idx)
{
#if defined __UITRON || defined __ECOS
#else
	SEM_DESTROY(kdrv_ive_semtbl[idx].semphore_id);
#endif
}

#if defined(__FREERTOS)
/*
    IVE get clock rate

    Get actual clock rate
*/
UINT32 ive_getIntClockRate(VOID)
{
	UINT32 uiActualClockRate = 0;
	switch (g_uiIveClockRateDefine) {
	case PLL_CLKSEL_IVE_240:
		uiActualClockRate = IVE_CLK_VAL_240;
		break;
	case PLL_CLKSEL_IVE_480:
		uiActualClockRate = IVE_CLK_VAL_480;
		break;
	case PLL_CLKSEL_IVE_320: //sc
		uiActualClockRate = IVE_CLK_VAL_320;
		break;
	case PLL_CLKSEL_IVE_PLL13:
		uiActualClockRate = IVE_CLK_VAL_PLL13;
		break;
	default:
		uiActualClockRate = 0;
	}

	return uiActualClockRate;
}

/**
    IVE get current clock rate definition

    Get the corresponding definition for current clock rate

    @return
        - Current clock definition
*/
UINT32 ive_getClockRate(VOID)
{
	return ive_getIntClockRate();
}
#endif  //#if defined(__FREERTOS)

#if defined(__FREERTOS)
#else
VOID ive_platform_isr_cb(UINT32 intstatus, VOID *ive_trig_hdl)
{
	KDRV_IVE_HANDLE *p_handle;

	p_handle = (KDRV_IVE_HANDLE *) ive_trig_hdl;

	DBG_IND("intstatus %d\r\n", intstatus);
	if ((intstatus & IVE_INT_FRM_END) || (intstatus & IVE_INT_LLEND)) {
		ive_platform_kdrv_ive_frm_end(p_handle, TRUE);
		DBG_IND("Set frame end flag\r\n");
	}
	if(intstatus & IVE_INT_LLERROR) {
		DBG_ERR("ERROR : Linking List ERROR\r\n");
		ive_ll_pause();
		ive_platform_kdrv_ive_frm_end(p_handle, TRUE);
	}

	if (p_handle->isrcb_fp != NULL) {
		p_handle->isrcb_fp((UINT32)p_handle, intstatus, NULL, NULL);
	}
}
#endif

#if defined __UITRON || defined __ECOS
void ive_platform_kdrv_ive_timeout_cb(UINT32 timer_id)
{
	set_flg(ive_getTriHdl_flagId(), KDRV_IVE_TIMEOUT);
}
#elif defined(__FREERTOS)
#else
enum hrtimer_restart ive_platform_nvt_ive_syscall_timercb(struct hrtimer *timer)
{
	set_flg(ive_getTriHdl_flagId(), KDRV_IVE_TIMEOUT);
    DBG_IND("Set time out flag\r\n");
	return HRTIMER_NORESTART;
}
#endif

VOID ive_platform_set_timer(INT32 channel, KDRV_IVE_TRIGGER_PARAM *p_ive_param)
{
#if defined __UITRON || defined __ECOS
	if (SwTimer_Open(&g_ive_timer_id[channel], ive_platform_kdrv_ive_timeout_cb) != E_OK) {
		DBG_ERR("open timer fail\r\n");
		g_ive_is_open_timer[channel] = FALSE;
	} else {
		SwTimer_Cfg(g_ive_timer_id[channel], p_ive_param->time_out_ms, SWTIMER_MODE_ONE_SHOT);
		SwTimer_Start(g_ive_timer_id[channel]);
	}
#elif defined(__FREERTOS)
#else
	g_ive_kt_periode[channel] = ktime_set(0, p_ive_param->time_out_ms * 1000000); //seconds,nanoseconds
	hrtimer_init(&git_ive_htimer[channel], CLOCK_REALTIME, HRTIMER_MODE_REL);
	git_ive_htimer[channel].function = ive_platform_nvt_ive_syscall_timercb;
	hrtimer_start(&git_ive_htimer[channel], g_ive_kt_periode[channel], HRTIMER_MODE_REL);
#endif
}

VOID ive_platform_close_timer(INT32 channel)
{
#if defined __UITRON || defined __ECOS
	if (g_ive_is_open_timer[channel]) {
		SwTimer_Stop(g_ive_timer_id[channel]);
		SwTimer_Close(g_ive_timer_id[channel]);
	}
#elif defined(__FREERTOS)
#else
	hrtimer_cancel(&git_ive_htimer[channel]);
#endif
}

ER ive_platform_enableInt(UINT32 uiIntrEn)
{
	if (g_ive_interrupt_en) {
		ive_setIntrEnableReg(uiIntrEn | IVE_INTE_ALL);
	}

    return E_OK;
}

