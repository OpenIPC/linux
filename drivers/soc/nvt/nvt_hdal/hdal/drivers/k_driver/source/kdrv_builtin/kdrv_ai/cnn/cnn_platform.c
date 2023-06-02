/*
    cnn2 module driver

    NT98520 CNN driver.

    @file       cnn2_platform.c
    @ingroup    mIIPPcnn2

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "cnn_platform.h"
#include "cnn_lib.h"
#include "cnn_reg.h"
#include "cnn_int.h"

#define CNN_MODULE_CLK_NUM 2


static SEM_HANDLE semid_cnn1;
static SEM_HANDLE semid_cnn2;

static SEM_HANDLE semid_cnn_lib1;
static SEM_HANDLE semid_cnn_lib2;

#if defined(__FREERTOS)
static ID     	  flag_id_cnn1;
static ID     	  flag_id_cnn2;
#else
static FLGPTN     flag_id_cnn1;
static FLGPTN     flag_id_cnn2;
#endif

static UINT32 cnn_freq_mhz = 0;
static UINT32 cnn2_freq_mhz = 0;

#if defined(__FREERTOS)
#else
static struct clk *cnn_clk[CNN_MODULE_CLK_NUM];
#endif

#if defined(__FREERTOS)
static UINT32 g_cnn_true_clock = PLL_CLKSEL_CNN_240;
#else
#endif


VOID cnn_pt_sram_switch(BOOL cnn_id) {
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


VOID cnn_pt_clk_enable(BOOL cnn_id)
{
#if defined(__FREERTOS)
	if (cnn_id == 0){
		pll_enableClock(CNN_CLK);
	} else {
		pll_enableClock(CNN2_CLK);
	}
#else
	clk_prepare(cnn_clk[cnn_id]);
	clk_enable(cnn_clk[cnn_id]);
#endif
}

VOID cnn_pt_clk_disable(BOOL cnn_id)
{
#if defined(__FREERTOS)
	if (cnn_id == 0){
		pll_disableClock(CNN_CLK);
	} else {
		pll_disableClock(CNN2_CLK);
	}
#else
	clk_disable(cnn_clk[cnn_id]);
	clk_unprepare(cnn_clk[cnn_id]);
#endif
}


/**
	CNN Create Resource
*/
#if defined __FREERTOS
irqreturn_t cnn_platform_isr1(int irq, void *devid)
{
	cnn_isr(0);
	return IRQ_HANDLED;
}
irqreturn_t cnn_platform_isr2(int irq, void *devid)
{
	cnn_isr(1);
	return IRQ_HANDLED;
}

void cnn_pt_create_resource(BOOL cnn_id, UINT32 clk_freq)
{

	if (cnn_id == 0) {
		OS_CONFIG_FLAG(flag_id_cnn1);
		SEM_CREATE(semid_cnn1, 1);
		SEM_CREATE(semid_cnn_lib1, 1);
		request_irq(INT_ID_CNN, cnn_platform_isr1, IRQF_TRIGGER_HIGH, "cnn", 0);
	} else {
		OS_CONFIG_FLAG(flag_id_cnn2);
		SEM_CREATE(semid_cnn2, 1);
		SEM_CREATE(semid_cnn_lib2, 1);
		request_irq(INT_ID_CNN2, cnn_platform_isr2, IRQF_TRIGGER_HIGH, "cnn", 0);
	}
	if (cnn_id == 0) {
		cnn_freq_mhz = clk_freq / CNN_1M_HZ;
	}
	else {
		cnn2_freq_mhz = clk_freq / CNN_1M_HZ;
	}
}
#else
VOID cnn_pt_create_resource(BOOL cnn_id, VOID *parm, UINT32 clk_freq)
{
	struct clk* module_clk = (struct clk*) parm;

	if (cnn_id == 0) {
		OS_CONFIG_FLAG(flag_id_cnn1);
		SEM_CREATE(semid_cnn1, 1);
		SEM_CREATE(semid_cnn_lib1, 1);
	} else {
		OS_CONFIG_FLAG(flag_id_cnn2);
		SEM_CREATE(semid_cnn2, 1);
		SEM_CREATE(semid_cnn_lib2, 1);
	}
	cnn_clk[cnn_id] = module_clk;
	if (cnn_id == 0) {
		cnn_freq_mhz = clk_freq / CNN_1M_HZ;
	}
	else {
		cnn2_freq_mhz = clk_freq / CNN_1M_HZ;
	}
}
#endif

VOID cnn_pt_release_resource(BOOL cnn_id)
{
	if (cnn_id == 0) {
		rel_flg(flag_id_cnn1);
		SEM_DESTROY(semid_cnn1);
		SEM_DESTROY(semid_cnn_lib1);
	} else {
		rel_flg(flag_id_cnn2);
		SEM_DESTROY(semid_cnn2);
		SEM_DESTROY(semid_cnn_lib2);
	}
}

VOID cnn_pt_rel_flg(BOOL cnn_id)
{
	if (cnn_id == 0) {
		rel_flg(flag_id_cnn1);
	} else {
		rel_flg(flag_id_cnn2);
	}
}

ER cnn_pt_sem_wait(BOOL cnn_id)
{
	ER erReturn = E_OK;
	#if defined __UITRON || defined __ECOS
	if (cnn_id == 0){
		return wai_sem(semid_cnn1);
	} else {
		return wai_sem(semid_cnn2);
	}
	#else
	if (cnn_id == 0){
		erReturn = SEM_WAIT(semid_cnn1);
	} else {
		erReturn = SEM_WAIT(semid_cnn2);
	}
	#endif

	return erReturn;
}

VOID cnn_pt_sem_signal(BOOL cnn_id)
{
	#if defined __UITRON || defined __ECOS
	if (cnn_id == 0){
		return sig_sem(semid_cnn1);
	} else {
		return sig_sem(semid_cnn2);
	}
	#else
	if (cnn_id == 0){
		SEM_SIGNAL(semid_cnn1);
	} else {
		SEM_SIGNAL(semid_cnn2);
	}
	#endif
}

ER cnn_pt_lib_sem_wait(BOOL cnn_id)
{
	ER erReturn = E_OK;
	#if defined __UITRON || defined __ECOS
	if (cnn_id == 0){
		return wai_sem(semid_cnn_lib1);
	} else {
		return wai_sem(semid_cnn_lib2);
	}
	#else
	if (cnn_id == 0){
		erReturn = SEM_WAIT(semid_cnn_lib1);
	} else {
		erReturn = SEM_WAIT(semid_cnn_lib2);
	}
	#endif

	return erReturn;
}

VOID cnn_pt_lib_sem_signal(BOOL cnn_id)
{
	#if defined __UITRON || defined __ECOS
	if (cnn_id == 0){
		return sig_sem(semid_cnn_lib1);
	} else {
		return sig_sem(semid_cnn_lib2);
	}
	#else
	if (cnn_id == 0){
		SEM_SIGNAL(semid_cnn_lib1);
	} else {
		SEM_SIGNAL(semid_cnn_lib2);
	}
	#endif
}

VOID cnn_pt_wai_flg(BOOL cnn_id, FLGPTN *uiFlag, UINT32 flag_id)
{
	if (cnn_id == 0) {
		wai_flg(uiFlag, flag_id_cnn1, flag_id, TWF_CLR | TWF_ORW);
	} else {
		wai_flg(uiFlag, flag_id_cnn2, flag_id, TWF_CLR | TWF_ORW);
	}
}

VOID cnn_pt_clr_flg(BOOL cnn_id, UINT32 flag_id, BOOL cb_flg)
{
#if defined(__FREERTOS)
	if (cb_flg == 0) {
		if (cnn_id == 0) {
			clr_flg(flag_id_cnn1, flag_id);
		}
		else {
			clr_flg(flag_id_cnn2, flag_id);
		}
	}
	else {
		if (cnn_id == 0) {
			iclr_flg(flag_id_cnn1, flag_id);
		}
		else {
			iclr_flg(flag_id_cnn2, flag_id);
		}

	}
#else
	if (cnn_id == 0) {
		clr_flg(flag_id_cnn1, flag_id);
	}
	else {
		clr_flg(flag_id_cnn2, flag_id);
	}
#endif
}

VOID cnn_pt_iset_flg(BOOL cnn_id, UINT32 flag_id) 
{
	if (cnn_id == 0) {
		iset_flg(flag_id_cnn1, flag_id);
	} else {
		iset_flg(flag_id_cnn2, flag_id);
	}
}

/**
    @addtogroup mIIPPCNN
*/
//@{

UINT32 cnn_pt_va2pa(UINT32 addr)
{
    return vos_cpu_get_phy_addr(addr);
}

VOID cnn_pt_dma_flush_dev2mem(UINT32 addr, UINT32 size)
{
    vos_cpu_dcache_sync(addr, size, VOS_DMA_FROM_DEVICE);
    return;
}

VOID cnn_pt_dma_flush_mem2dev(UINT32 addr, UINT32 size)
{
    vos_cpu_dcache_sync(addr, size, VOS_DMA_TO_DEVICE);
    return;
}

VOID cnn_pt_dma_flush(UINT32 addr, UINT32 size)
{
    vos_cpu_dcache_sync(addr, size, VOS_DMA_BIDIRECTIONAL);
    return;
}

/**
CNN Set Clock Rate

Set current clock rate

@param[in] clock cnn clock rate

@return
- @b E_OK: Done with no error.
- Others: Error occured.
*/
ER cnn_pt_set_clock_rate(BOOL cnn_id, UINT32 clock)
{
#if defined(__FREERTOS)

#if defined(_BSP_NA51055_)
	
	//DBG_ERR("cnn_pt_set_clock_rate input frequency %d\r\n", clock);

	if (clock >= 600) {
        //DBG_ERR("Input frequency %d round to 600\r\n", clock);
		if(cnn_id == 0){
			g_cnn_true_clock = PLL_CLKSEL_CNN_PLL10;
		} else {
			g_cnn_true_clock = PLL_CLKSEL_CNN2_PLL10;
		}
		if(pll_get_pll_enable(PLL_ID_10) == FALSE) {
			pll_set_pll_enable(PLL_ID_10, TRUE);
		}
	} else if (clock >= 480) {
		//DBG_ERR("Input frequency %d round to 480\r\n", clock);
		if(cnn_id == 0){
			g_cnn_true_clock = PLL_CLKSEL_CNN_480;
		} else {
			g_cnn_true_clock = PLL_CLKSEL_CNN2_480;
		}
	} else if (clock >= 320) {
		//DBG_ERR("Input frequency %d round to 320\r\n", clock);
		if(cnn_id == 0){
			g_cnn_true_clock = PLL_CLKSEL_CNN_320;
		} else {
			g_cnn_true_clock = PLL_CLKSEL_CNN2_320;
		}
		if (pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) {
			pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		}
	} else if (clock >= 240) {
		//DBG_ERR("Input frequency %d round to 240\r\n", clock);
		if(cnn_id == 0){
			g_cnn_true_clock = PLL_CLKSEL_CNN_240;
		} else {
			g_cnn_true_clock = PLL_CLKSEL_CNN2_240;
		}
	} else {
		DBG_ERR("Illegal input frequency %d\r\n", clock);
		return E_SYS;
	}

	if  (cnn_id == 0) {
		pll_setClockRate(PLL_CLKSEL_CNN, g_cnn_true_clock);
	} else {
		pll_setClockRate(PLL_CLKSEL_CNN2, g_cnn_true_clock);
	}

#endif
#if defined(_BSP_NA51089_)
	//DBG_ERR("cnn_pt_set_clock_rate input frequency %d\r\n", clock);

	if (clock >= 600) {
		//DBG_ERR("Input frequency %d round to 600\r\n", clock);
		if (cnn_id == 0) {
			g_cnn_true_clock = PLL_CLKSEL_CNN_PLL9;
		}
		else {
			//g_cnn_true_clock = PLL_CLKSEL_CNN2_PLL10;
		}
		if (pll_get_pll_enable(PLL_ID_9) == FALSE) {
			pll_set_pll_enable(PLL_ID_9, TRUE);
		}
	}
	else if (clock >= 480) {
		//DBG_ERR("Input frequency %d round to 480\r\n", clock);
		if (cnn_id == 0) {
			g_cnn_true_clock = PLL_CLKSEL_CNN_PLL12;
		}
		else {
			//g_cnn_true_clock = PLL_CLKSEL_CNN2_480;
		}
		if (pll_get_pll_enable(PLL_ID_12) == FALSE) {
			pll_set_pll_enable(PLL_ID_12, TRUE);
		}
	}
	else if (clock >= 320) {
		//DBG_ERR("Input frequency %d round to 320\r\n", clock);
		if (cnn_id == 0) {
			g_cnn_true_clock = PLL_CLKSEL_CNN_320;
		}
		else {
			g_cnn_true_clock = PLL_CLKSEL_CNN2_320;
		}
		/*if (pll_get_pll_enable(PLL_ID_FIXED320) == FALSE) {
			pll_set_pll_enable(PLL_ID_FIXED320, TRUE);
		}*/
	}
	else if (clock >= 240) {
		//DBG_ERR("Input frequency %d round to 240\r\n", clock);
		if (cnn_id == 0) {
			g_cnn_true_clock = PLL_CLKSEL_CNN_240;
		}
		else {
			g_cnn_true_clock = PLL_CLKSEL_CNN2_240;
		}
	}
	else {
		DBG_ERR("Illegal input frequency %d\r\n", clock);
		return E_SYS;
	}

	if (cnn_id == 0) {
		pll_setClockRate(PLL_CLKSEL_CNN, g_cnn_true_clock);
	}
	else {
		pll_setClockRate(PLL_CLKSEL_CNN2, g_cnn_true_clock);
	}

#endif
#else
#if 1
	struct clk *pclk;

	if (clock >= 600) {
		pclk = clk_get(NULL, "pll9");
		if (clock > 600) {
			DBG_ERR("cnn_pt_set_clock_rate() input frequency %d round to 600\r\n", clock);
		}
	} else if (clock >= 480) {
		//cnn_clock = 480;
		pclk = clk_get(NULL, "pll12");
		if (clock > 480) {
			DBG_ERR("cnn_pt_set_clock_rate() input frequency %d round to 480\r\n", clock);
		}
	} else if (clock >= 320) {
		//cnn_clock = 320;
		pclk = clk_get(NULL, "pllf320");
		if (clock > 320) {
			DBG_ERR("cnn_pt_set_clock_rate() input frequency %d round to 320\r\n", clock);
		}
	} else if (clock >= 240) {
		//cnn_clock = 240;
		pclk = clk_get(NULL, "fix240m");
		if (clock > 240) {
			DBG_ERR("cnn_pt_set_clock_rate() input frequency %d round to 240\r\n", clock);
		}
	} else {
		DBG_ERR("cnn_setClockRate() illegal input frequency %u, set as 240\r\n", clock);
		pclk = clk_get(NULL, "fix240m");
	}

	if (IS_ERR(pclk)) {
		printk("%s: get source fix480m fail\r\n", __func__);
	}

	clk_set_parent(cnn_clk[cnn_id], pclk);
	clk_put(pclk);
#endif
#endif //#if defined(__FREERTOS)

	return E_OK;
}

/**
CNN get operation clock

Get CNN clock selection.

@param None.

@return UINT32 CNN operation clock rate.
*/
UINT32 cnn_pt_get_clock_rate(BOOL cnn_id)
{
	if (cnn_id == 0) {
		return cnn_freq_mhz;
	}
	else {
		return cnn2_freq_mhz;
	}
}
//@}
