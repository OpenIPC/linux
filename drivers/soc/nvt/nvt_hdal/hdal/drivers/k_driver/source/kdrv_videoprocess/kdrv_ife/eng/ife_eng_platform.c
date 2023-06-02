#if defined (__LINUX)
#include <linux/interrupt.h>
#else
#include "interrupt.h"
#endif

#include "ife_eng.h"
#include "ife_eng_int_platform.h"
#include "ife_eng_int_dbg.h"

UINT32 _ife_reg_io_base = 0;

static ID     flg_id_ife;
static SEM_HANDLE semid_ife;



ER ife_eng_platform_sem_signal(void)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(semid_ife);
#else
	SEM_SIGNAL(semid_ife);
	return E_OK;
#endif
}

ER ife_eng_platform_sem_wait(void)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(semid_ife);
#else
	return SEM_WAIT(semid_ife);
#endif
}

VOID ife_eng_platform_prepare_clk(IFE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_prepare(p_eng->pclk);
	}
#elif defined (__FREERTOS)
	pll_enableSystemReset(IFE_RSTN);
	pll_disableSystemReset(IFE_RSTN);
#else
#endif
}

VOID ife_eng_platform_unprepare_clk(IFE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_unprepare(p_eng->pclk);
	}
#elif defined (__FREERTOS)
#else
#endif
}

VOID ife_eng_platform_enable_clk(IFE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_enable(p_eng->pclk);
	}
#elif defined (__FREERTOS)
	pll_enableClock(IFE_CLK);
#else
#endif
}

VOID ife_eng_platform_disable_clk(IFE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_disable(p_eng->pclk);
	}
#elif defined (__FREERTOS)
	pll_disableClock(IFE_CLK);
#else
#endif
}

INT32 ife_eng_platform_set_clk_rate(IFE_ENG_HANDLE *p_eng)
{
#if 0	// todo: check clk usage
#if defined (__LINUX)
	struct clk *parent_clk;
	UINT32 source_clk;

	if (IS_ERR(p_eng->pclk)) {
		DBG_ERR("get clk fail...0x%p\r\n", p_eng->pclk);
		return E_SYS;
	}

	source_clk = p_eng->clock_rate;
	if (ife_eng_platform_get_chip_id() == CHIP_NA51055) {
		if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
		} else {
			parent_clk = clk_get(NULL, "pll13");
		}
	} else {
		if (source_clk == 320) {
			parent_clk = clk_get(NULL, "fix320m");
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
		} else {
			parent_clk = clk_get(NULL, "pll13");
		}
	}

	clk_set_parent(p_eng->pclk, parent_clk);
	clk_put(parent_clk);

#elif defined (__FREERTOS )
	UINT32 selected_clock;;
	UINT32 source_clk;

	// Turn on power
	pmc_turnonPower(PMC_MODULE_IFE);
	// select clock
	source_clk = p_eng->clock_rate;
	if (ife_eng_platform_get_chip_id() == CHIP_NA51055) {
		if (source_clk == 240) {
			selected_clock = PLL_CLKSEL_IFE_240;
		} else {
			selected_clock = PLL_CLKSEL_IFE_PLL13;

			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}

	} else {
		if (source_clk == 320) {
			selected_clock = PLL_CLKSEL_IFE_320;

			if (pll_getPLLEn(PLL_ID_FIXED320) == FALSE) {
				pll_setPLLEn(PLL_ID_FIXED320, TRUE);
			}
		} else if (source_clk == 240) {
			selected_clock = PLL_CLKSEL_IFE_240;
		} else {
			selected_clock = PLL_CLKSEL_IFE_PLL13;

			if (pll_getPLLEn(PLL_ID_13) == FALSE) {
				pll_setPLLEn(PLL_ID_13, TRUE);
			}
		}
	}

	pll_setClockRate(PLL_CLKSEL_IFE, selected_clock);
#else
#endif
#endif

#if defined (__LINUX)
	struct clk *parent_clk;
	struct clk *ife_sram_clk;
	UINT32 source_clk;

	if (IS_ERR(p_eng->pclk)) {
		DBG_ERR("get clk fail...0x%p\r\n", p_eng->pclk);
		return E_SYS;
	}

	source_clk = p_eng->clock_rate;
	if (ife_eng_platform_get_chip_id() == CHIP_NA51089) {
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

	/* sram_sel */
	parent_clk = clk_get(NULL, "ife_sram_ife");
	if (IS_ERR(parent_clk))
	    DBG_ERR("getting parent_clk error\n");

	/* Get ife sram control node*/
	ife_sram_clk = clk_get(NULL, "ife_shared_sram");
	if (IS_ERR(ife_sram_clk))
	    DBG_ERR("getting ife_sram_clk error\n");

	/* Set clk select*/
	clk_set_parent(ife_sram_clk, parent_clk);

#elif defined (__FREERTOS )
	UINT32 source_clk, select_clk;

	source_clk = p_eng->clock_rate;

	if (ife_eng_platform_get_chip_id() == CHIP_NA51089) {
		if (source_clk == 192) {
			select_clk = PLL_CLKSEL_IFE_192;
		} else if (source_clk == 240) {
			select_clk = PLL_CLKSEL_IFE_240;
		} else if (source_clk == 96) {
			select_clk = PLL_CLKSEL_IFE_96;
		} else {
			select_clk = PLL_CLKSEL_IFE_PLL12;
			if (pll_getPLLEn(PLL_ID_12) == FALSE) {
				pll_setPLLEn(PLL_ID_12, TRUE);
			}
		}
	} else {
		select_clk = PLL_CLKSEL_IFE_240;
	}

	pll_setClockRate(PLL_CLKSEL_IFE, select_clk);

	/* set sram sel for ife */
	pll_setClockRate(PLL_CLKSEL_IFE_SRAM, PLL_CLKSEL_IFE_SRAM_IFE);
#endif

	return E_OK;
}

VOID ife_eng_platform_disable_sram_shutdown(IFE_ENG_HANDLE *p_eng)
{
	if (p_eng->chip_id == KDRV_CHIP0 &&
		p_eng->eng_id == KDRV_VIDEOPROCS_IFE_ENGINE0) {
		nvt_disable_sram_shutdown(IFE_SD);
	}
}

VOID ife_eng_platform_enable_sram_shutdown(IFE_ENG_HANDLE *p_eng)
{
	if (p_eng->chip_id == KDRV_CHIP0 &&
		p_eng->eng_id == KDRV_VIDEOPROCS_IFE_ENGINE0) {
		nvt_enable_sram_shutdown(IFE_SD);
	}
}

ER ife_eng_platform_flg_clear(FLGPTN flg)
{
	return clr_flg(flg_id_ife, flg);
}
//---------------------------------------------------------------

ER ife_eng_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg)
{
	ER status;

	status = wai_flg(p_flgptn, flg_id_ife, flg, TWF_CLR | TWF_ORW);

	return status;
}
//---------------------------------------------------------------

ER ife_eng_platform_flg_set(FLGPTN flg)
{
	return iset_flg(flg_id_ife, flg);
}
//---------------------------------------------------------------

static irqreturn_t ife_eng_platform_isr(int irq, void *param)
{
	ife_eng_isr_hw_reg((IFE_ENG_HANDLE *)param);
	return IRQ_HANDLED;
}

VOID ife_eng_platform_request_irq(IFE_ENG_HANDLE *p_eng)
{
	request_irq(p_eng->irq_id, ife_eng_platform_isr, IRQF_TRIGGER_HIGH, p_eng->name, p_eng);
}

VOID ife_eng_platform_release_irq(IFE_ENG_HANDLE *p_eng)
{
	free_irq(p_eng->irq_id, p_eng);
}

VOID ife_eng_platform_create_resource(VOID)
{
	OS_CONFIG_FLAG(flg_id_ife);
	SEM_CREATE(semid_ife, 1);
}

VOID ife_eng_platform_release_resource(VOID)
{
	rel_flg(flg_id_ife);
	SEM_DESTROY(semid_ife);
}

void *IFE_ENG_MALLOC(UINT32 size)
{
	void *p_buf = NULL;

#if defined(__LINUX)
	p_buf = kmalloc(size, GFP_KERNEL);
#elif defined(__FREERTOS)
	p_buf = malloc(size);
#endif

	return p_buf;
}

void IFE_ENG_FREE(void *ptr)
{
#if defined(__LINUX)
	kfree(ptr);
#elif defined(__FREERTOS)
	free(ptr);
#endif
}

UINT32 ife_eng_platform_get_chip_id(VOID)
{
	static UINT32 chip_id = 0;

	if (chip_id == 0) {
		chip_id = nvt_get_chip_id();
	}

	return chip_id;
}
