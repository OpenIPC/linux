#if defined (__LINUX)
#include <linux/interrupt.h>
#else
#include "interrupt.h"
#endif
#include "ipe_eng_int_platform.h"
#include "ipe_eng_int.h"
#include "ipe_eng_int_dbg.h"
#include "ipe_eng.h"

UINT32 _ipe_reg_io_base = 0;

static SEM_HANDLE semid_ipe;
static ID flg_id_ipe;


ER ipe_eng_platform_sem_wait(void)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(semid_ipe);
#else
	return SEM_WAIT(semid_ipe);
#endif
}

ER ipe_eng_platform_sem_signal(void)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(semid_ipe);
#else
	SEM_SIGNAL(semid_ipe);
	return E_OK;
#endif
}



VOID ipe_eng_platform_prepare_clk(IPE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_prepare(p_eng->pclk);
	}
#else
	pll_enableSystemReset(IPE_RSTN);
	pll_disableSystemReset(IPE_RSTN);
#endif
}

VOID ipe_eng_platform_unprepare_clk(IPE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_unprepare(p_eng->pclk);
	}
#endif
}

VOID ipe_eng_platform_enable_clk(IPE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_enable(p_eng->pclk);
	}
#else
	pll_enableClock(IPE_CLK);
#endif
}

VOID ipe_eng_platform_disable_clk(IPE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	if (IS_ERR(p_eng->pclk)) {
		DBG_WRN("get clk fail 0x%p\r\n", p_eng->pclk);
	} else {
		clk_disable(p_eng->pclk);
	}
#else
	pll_disableClock(IPE_CLK);
#endif
}

INT32 ipe_eng_platform_set_clk_rate(IPE_ENG_HANDLE *p_eng)
{
#if defined (__LINUX)
	struct clk *parent_clk;
	UINT32 source_clk;

	if (IS_ERR(p_eng->pclk)) {
		DBG_ERR("get clk fail...0x%p\r\n", p_eng->pclk);
		return E_SYS;
	}

	source_clk = p_eng->clock_rate;
	if (ipe_eng_platform_get_chip_id() == CHIP_NA51089) {
		if (source_clk == 192) {
			parent_clk = clk_get(NULL, "fix192m");
			p_eng->clock_rate = 192;
		} else if (source_clk == 240) {
			parent_clk = clk_get(NULL, "fix240m");
			p_eng->clock_rate = 240;
		} else if (source_clk == 96) {
			parent_clk = clk_get(NULL, "fix96m");
			p_eng->clock_rate = 96;
		} else {
			parent_clk = clk_get(NULL, "pll12");
			p_eng->clock_rate = 0;
		}
	} else {
		parent_clk = clk_get(NULL, "fix240m");
		p_eng->clock_rate = 240;
	}

	clk_set_parent(p_eng->pclk, parent_clk);
	clk_put(parent_clk);

#elif defined (__FREERTOS )
	UINT32 selected_clock;;
	UINT32 source_clk;

	// Turn on power
	pmc_turnonPower(PMC_MODULE_IPE);

	// select clock
	source_clk = p_eng->clock_rate;
	if (ipe_eng_platform_get_chip_id() == CHIP_NA51089) {
		if (source_clk == 192) {
			selected_clock = PLL_CLKSEL_IPE_192;
		} else if (source_clk == 240) {
			selected_clock = PLL_CLKSEL_IPE_240;
		} else if (source_clk == 96) {
			selected_clock = PLL_CLKSEL_IPE_96;
		} else {
			selected_clock = PLL_CLKSEL_IPE_PLL12;

			if (pll_getPLLEn(PLL_ID_12) == FALSE) {
				pll_setPLLEn(PLL_ID_12, TRUE);
			}
		}

	} else {
		selected_clock = PLL_CLKSEL_IPE_240;
	}

	pll_setClockRate(PLL_CLKSEL_IPE, selected_clock);
#endif

#if 0
#if defined (__LINUX)

#elif defined (__FREERTOS )
	pll_setClockRate(PLL_CLKSEL_IPE, PLL_CLKSEL_IPE_240);
#endif
#endif
	return E_OK;
}

VOID ipe_eng_platform_disable_sram_shutdown(IPE_ENG_HANDLE *p_eng)
{
	if (p_eng->chip_id == KDRV_CHIP0 &&
		p_eng->eng_id == KDRV_VIDEOPROCS_IPE_ENGINE0) {
		nvt_disable_sram_shutdown(IPE_SD);
	}
}

VOID ipe_eng_platform_enable_sram_shutdown(IPE_ENG_HANDLE *p_eng)
{
	if (p_eng->chip_id == KDRV_CHIP0 &&
		p_eng->eng_id == KDRV_VIDEOPROCS_IPE_ENGINE0) {
		nvt_enable_sram_shutdown(IPE_SD);
	}
}



ER ipe_eng_platform_flg_clear(FLGPTN flg)
{
	return clr_flg(flg_id_ipe, flg);
}

ER ipe_eng_platform_flg_set(FLGPTN flg)
{
	return iset_flg(flg_id_ipe, flg);
}

ER ipe_eng_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg)
{
	ER status;

	status = wai_flg(p_flgptn, flg_id_ipe, flg, TWF_CLR | TWF_ORW);

	return status;
}


static irqreturn_t ipe_eng_platform_isr(int irq, void *param)
{
	ipe_eng_isr_hw_reg((IPE_ENG_HANDLE *)param);
	return IRQ_HANDLED;
}

VOID ipe_eng_platform_request_irq(IPE_ENG_HANDLE *p_eng)
{
	request_irq(p_eng->irq_id, ipe_eng_platform_isr, IRQF_TRIGGER_HIGH, p_eng->name, p_eng);
}

VOID ipe_eng_platform_release_irq(IPE_ENG_HANDLE *p_eng)
{
	free_irq(p_eng->irq_id, p_eng);
}

VOID ipe_eng_platform_create_resource(VOID)
{
	OS_CONFIG_FLAG(flg_id_ipe);
	SEM_CREATE(semid_ipe, 1);
}

VOID ipe_eng_platform_release_resource(VOID)
{
	rel_flg(flg_id_ipe);
	SEM_DESTROY(semid_ipe);
}

void *IPE_ENG_MALLOC(UINT32 size)
{
	void *p_buf = NULL;

#if defined(__LINUX)
	p_buf = kmalloc(size, GFP_KERNEL);
#elif defined(__FREERTOS)
	p_buf = malloc(size);
#endif

	return p_buf;
}

void IPE_ENG_FREE(void *ptr)
{
#if defined(__LINUX)
	kfree(ptr);
#elif defined(__FREERTOS)
	free(ptr);
#endif
}

UINT32 ipe_eng_platform_get_chip_id(VOID)
{
	static UINT32 chip_id = 0;

	if (chip_id == 0) {
		chip_id = nvt_get_chip_id();
	}

	return chip_id;
}

