/*
    DCE module driver

    NT96520 DCE module driver.

    @file       dce_eng_platform.c
    @ingroup    mIIPPDCE
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

//---------------------------------------------------------------
#include "dce_eng_int_platform.h"
#include "dce_eng_int_dbg.h"

static SEM_HANDLE semid_dce;
static ID flg_id_dce;

// debug level
unsigned int dce_eng_debug_level = NVT_DBG_WRN;

static irqreturn_t dce_eng_platform_isr(int irq, void *param)
{
	dce_eng_isr((DCE_ENG_HANDLE *)param);
	return IRQ_HANDLED;
}

ER dce_eng_platform_sem_wait(VOID)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(semid_dce);
#else
	return SEM_WAIT(semid_dce);
#endif
}

ER dce_eng_platform_sem_signal(VOID)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(semid_dce);
#else
	SEM_SIGNAL(semid_dce);
	return E_OK;
#endif
}

VOID dce_eng_platform_prepare_clk(DCE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_prepare(p_eng->pclk);
	}
#else
	pll_enableSystemReset(DCE_RSTN);
	pll_disableSystemReset(DCE_RSTN);
#endif
}


VOID dce_eng_platform_unprepare_clk(DCE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_unprepare(p_eng->pclk);
	}
#endif
}

VOID dce_eng_platform_enable_clk(DCE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_enable(p_eng->pclk);
	}
#else
	pll_enableClock(DCE_CLK);
#endif
}

VOID dce_eng_platform_disable_clk(DCE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_disable(p_eng->pclk);
	}
#else
	pll_disableClock(DCE_CLK);
#endif
}

INT32 dce_eng_platform_set_clk_rate(DCE_ENG_HANDLE *p_eng)
{

#if defined (__LINUX)
	struct clk *parent_clk;
	UINT32 source_clk;

	if (IS_ERR(p_eng->pclk)) {
		DBG_ERR("get clk fail...0x%p\r\n", p_eng->pclk);
		return E_SYS;
	}

	source_clk = p_eng->clock_rate;
	if (dce_eng_platform_get_chip_id() == CHIP_NA51089) {
		if (source_clk == 192) {
			parent_clk = clk_get(NULL, "fix192m");
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
		} else if (source_clk == 96) {
			parent_clk = clk_get(NULL, "fix96m");
		} else {
			parent_clk = clk_get(NULL, "pll12");
		}
	} else {
		parent_clk = clk_get(NULL, "fix240m");
	}

	clk_set_parent(p_eng->pclk, parent_clk);
	clk_put(parent_clk);

#elif defined (__FREERTOS )
	UINT32 selected_clock;
	UINT32 source_clk;

	// Turn on power
	// select clock
	source_clk = p_eng->clock_rate;
	if (dce_eng_platform_get_chip_id() == CHIP_NA51089) {
		if (source_clk == 192) {
			selected_clock = PLL_CLKSEL_DCE_192;
		} else if (source_clk == 240) {
			selected_clock = PLL_CLKSEL_DCE_240;
		} else if (source_clk == 96) {
			selected_clock = PLL_CLKSEL_DCE_96;
		} else {
			selected_clock = PLL_CLKSEL_DCE_PLL12;

			if (pll_getPLLEn(PLL_ID_12) == FALSE) {
				pll_setPLLEn(PLL_ID_12, TRUE);
			}
		}

	} else {
		selected_clock = PLL_CLKSEL_DCE_240;
	}

	pll_setClockRate(PLL_CLKSEL_DCE, selected_clock);
#else
#endif

	return E_OK;
}

VOID dce_eng_platform_disable_sram_shutdown(DCE_ENG_HANDLE *p_eng)
{
	if (p_eng->chip_id == KDRV_CHIP0 &&
		p_eng->eng_id == KDRV_VIDEOPROCS_DCE_ENGINE0) {
		nvt_disable_sram_shutdown(DCE_SD);
	}
}

VOID dce_eng_platform_enable_sram_shutdown(DCE_ENG_HANDLE *p_eng)
{
	if (p_eng->chip_id == KDRV_CHIP0 &&
		p_eng->eng_id == KDRV_VIDEOPROCS_DCE_ENGINE0) {
		nvt_enable_sram_shutdown(DCE_SD);
	}
}

VOID dce_eng_platform_create_resource(VOID)
{
	OS_CONFIG_FLAG(flg_id_dce);
	SEM_CREATE(semid_dce, 1);
}

VOID dce_eng_platform_release_resource(VOID)
{
	rel_flg(flg_id_dce);
	SEM_DESTROY(semid_dce);
}

VOID dce_eng_platform_request_irq(DCE_ENG_HANDLE *p_eng)
{
	request_irq(p_eng->irq_id, dce_eng_platform_isr, IRQF_TRIGGER_HIGH, p_eng->name, p_eng);
}

VOID dce_eng_platform_release_irq(DCE_ENG_HANDLE *p_eng)
{
	free_irq(p_eng->irq_id, p_eng);
}

ER dce_eng_platform_flg_clear(FLGPTN flg)
{
	return clr_flg(flg_id_dce, flg);
}

ER dce_eng_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg)
{
	ER status;

	status = wai_flg(p_flgptn, flg_id_dce, flg, TWF_CLR | TWF_ORW);

	return status;
}

ER dce_eng_platform_flg_set(FLGPTN flg)
{
	return iset_flg(flg_id_dce, flg);
}

void *DCE_ENG_MALLOC(UINT32 size)
{
	void *p_buf = NULL;

#if defined(__LINUX)
	p_buf = kmalloc(size, GFP_KERNEL);
#elif defined(__FREERTOS)
	p_buf = malloc(size);
#endif

	return p_buf;
}

void DCE_ENG_FREE(void *ptr)
{
#if defined(__LINUX)
	kfree(ptr);
#elif defined(__FREERTOS)
	free(ptr);
#endif
}

UINT32 dce_eng_platform_get_chip_id(VOID)
{
	static UINT32 chip_id = 0;

	if (chip_id == 0) {
		chip_id = nvt_get_chip_id();
	}

	return chip_id;
}

UINT64 dce_eng_do_64b_div(UINT64 dividend, UINT64 divisor)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	dividend = (dividend / divisor);
#else
	do_div(dividend, divisor);
#endif
	return dividend;
}
