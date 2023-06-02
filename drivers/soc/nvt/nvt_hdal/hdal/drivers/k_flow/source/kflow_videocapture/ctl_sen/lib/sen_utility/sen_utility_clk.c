/*
    Sensor Interface kflow library - sensor clock utility

    Sensor Interface KFLOW library.

    @file       sen_utility_clk.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.
*/
#include "sen_utility_clk_int.h"

INT32 __sen_uti_set_clk(CTL_SEN_UTI_CLK param_id, VOID *p_param)
{
	INT32 rt = CTL_SEN_E_OK;

	switch (param_id) {
	case CTL_SEN_UTI_CLK_SIEMCLK_ENABLE: {
#ifndef __KERNEL__

			if (((UINT32)p_param) > 0) {
				pll_enable_clock(SIE_MCLK);
			} else {
				pll_disable_clock(SIE_MCLK);
			}
#else
			struct clk *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_MCLK);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			if (((UINT32)p_param) > 0) {
				clk_prepare(src_clk);
				clk_enable(src_clk);
			} else {
				clk_unprepare(src_clk);
				clk_disable(src_clk);
			}

			clk_put(src_clk);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_SIEMCLK2_ENABLE: {
#ifndef __KERNEL__

			if (((UINT32)p_param) > 0) {
				pll_enable_clock(SIE_MCLK2);
			} else {
				pll_disable_clock(SIE_MCLK2);
			}
#else
			struct clk *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_MCLK2);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			if (((UINT32)p_param) > 0) {
				clk_prepare(src_clk);
				clk_enable(src_clk);
			} else {
				clk_unprepare(src_clk);
				clk_disable(src_clk);
			}

			clk_put(src_clk);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_SIEMCLK3_ENABLE: {
#ifndef __KERNEL__

			if (((UINT32)p_param) > 0) {
				pll_enable_clock(SIE_MCLK3);
			} else {
				pll_disable_clock(SIE_MCLK3);
			}
#else
			struct clk *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_MCLK3);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			if (((UINT32)p_param) > 0) {
				clk_prepare(src_clk);
				clk_enable(src_clk);
			} else {
				clk_unprepare(src_clk);
				clk_disable(src_clk);
			}

			clk_put(src_clk);
#endif
		}
		break;

	case CTL_SEN_UTI_CLK_SPCLK_ENABLE: {
#ifndef __KERNEL__

			if (((UINT32)p_param) > 0) {
				pll_enable_clock(SP_CLK);
			} else {
				pll_disable_clock(SP_CLK);
			}
#else
			struct clk *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_SPCLK);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			if (((UINT32)p_param) > 0) {
				clk_prepare(src_clk);
				clk_enable(src_clk);
			} else {
				clk_unprepare(src_clk);
				clk_disable(src_clk);
			}

			clk_put(src_clk);
#endif
		}
		break;

	case CTL_SEN_UTI_CLK_SPCLK2_ENABLE: {
#ifndef __KERNEL__

			if (((UINT32)p_param) > 0) {
				pll_enable_clock(SP2_CLK);
			} else {
				pll_disable_clock(SP2_CLK);
			}
#else
			struct clk *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_SPCLK2);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			if (((UINT32)p_param) > 0) {
				clk_prepare(src_clk);
				clk_enable(src_clk);
			} else {
				clk_unprepare(src_clk);
				clk_disable(src_clk);
			}

			clk_put(src_clk);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_SIEMCLK_SOURCE: {
#ifndef __KERNEL__
			if (((UINT32)p_param) == CTL_SEN_CLKSRC_480) {
				pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_480);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_320) {
				pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_320);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL5) {
				pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL5);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL12) {
				pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL12);
			} else {
				CTL_SEN_DBG_ERR("not support %d=%d\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_IN_PARAM;
			}
#else
			struct clk *parent_clk = NULL, *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_MCLK);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			if (((UINT32)p_param) == CTL_SEN_CLKSRC_480) {
				parent_clk = clk_get(NULL, CG_STR_FIX480);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_320) {
				parent_clk = clk_get(NULL, CG_STR_FIX320);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL5) {
				parent_clk = clk_get(NULL, CG_STR_PLL5);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL12) {
				parent_clk = clk_get(NULL, CG_STR_PLL12);
			} else {
				CTL_SEN_DBG_ERR("not support %d=%d\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_IN_PARAM;
			}

			if (!IS_ERR(parent_clk)) {
				clk_set_parent(src_clk, parent_clk);
			} else {
				CTL_SEN_DBG_ERR("%d,%d: get parent clk fail\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_IN_PARAM;
			}

			clk_put(parent_clk);
			clk_put(src_clk);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_SIEMCLK2_SOURCE: {
#ifndef __KERNEL__
			if (((UINT32)p_param) == CTL_SEN_CLKSRC_480) {
				pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_480);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_320) {
				pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_320);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL5) {
				pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_PLL5);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL12) {
				pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_PLL12);
			} else {
				CTL_SEN_DBG_ERR("not support %d=%d\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_IN_PARAM;
			}
#else
			struct clk *parent_clk = NULL, *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_MCLK2);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			if (((UINT32)p_param) == CTL_SEN_CLKSRC_480) {
				parent_clk = clk_get(NULL, CG_STR_FIX480);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_320) {
				parent_clk = clk_get(NULL, CG_STR_FIX320);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL5) {
				parent_clk = clk_get(NULL, CG_STR_PLL5);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL12) {
				parent_clk = clk_get(NULL, CG_STR_PLL12);
			} else {
				CTL_SEN_DBG_ERR("not support %d=%d\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_IN_PARAM;
			}

			if (!IS_ERR(parent_clk)) {
				clk_set_parent(src_clk, parent_clk);
			} else {
				CTL_SEN_DBG_ERR("%d,%d: get parent clk fail\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_SYS;
			}

			clk_put(parent_clk);
			clk_put(src_clk);
#endif
		}
		break;

	case CTL_SEN_UTI_CLK_SIEMCLK3_SOURCE: {
#ifndef __KERNEL__
			if (((UINT32)p_param) == CTL_SEN_CLKSRC_480) {
				pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_480);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_320) {
				pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_320);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL5) {
				pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_PLL5);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL12) {
				pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_PLL12);
			} else {
				CTL_SEN_DBG_ERR("not support %d=%d\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_IN_PARAM;
			}
#else
			struct clk *parent_clk = NULL, *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_MCLK3);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			if (((UINT32)p_param) == CTL_SEN_CLKSRC_480) {
				parent_clk = clk_get(NULL, CG_STR_FIX480);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_320) {
				parent_clk = clk_get(NULL, CG_STR_FIX320);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL5) {
				parent_clk = clk_get(NULL, CG_STR_PLL5);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL12) {
				parent_clk = clk_get(NULL, CG_STR_PLL12);
			} else {
				CTL_SEN_DBG_ERR("not support %d=%d\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_IN_PARAM;
			}

			if (!IS_ERR(parent_clk)) {
				clk_set_parent(src_clk, parent_clk);
			} else {
				CTL_SEN_DBG_ERR("%d,%d: get parent clk fail\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_SYS;
			}

			clk_put(parent_clk);
			clk_put(src_clk);
#endif
		}
		break;

	case CTL_SEN_UTI_CLK_SPCLK_SOURCE: {
#ifndef __KERNEL__
			if (((UINT32)p_param) == CTL_SEN_CLKSRC_480) {
				pll_set_clock_rate(PLL_CLKSEL_SP, PLL_CLKSEL_SP_480);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL5) {
				pll_set_clock_rate(PLL_CLKSEL_SP, PLL_CLKSEL_SP_PLL5);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL6) {
				pll_set_clock_rate(PLL_CLKSEL_SP, PLL_CLKSEL_SP_PLL6);
			} else {
				CTL_SEN_DBG_ERR("not support %d=%d\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_IN_PARAM;
			}
#else
			struct clk *parent_clk = NULL, *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_SPCLK);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
			}

			if (((UINT32)p_param) == CTL_SEN_CLKSRC_480) {
				parent_clk = clk_get(NULL, CG_STR_FIX480);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL5) {
				parent_clk = clk_get(NULL, CG_STR_PLL5);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL6) {
				parent_clk = clk_get(NULL, CG_STR_PLL6);
			} else {
				CTL_SEN_DBG_ERR("not support %d=%d\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_IN_PARAM;
			}

			if (!IS_ERR(parent_clk)) {
				clk_set_parent(src_clk, parent_clk);
			} else {
				CTL_SEN_DBG_ERR("%d,%d: get parent clk fail\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_SYS;
			}

			clk_put(parent_clk);
			clk_put(src_clk);
#endif
		}
		break;

	case CTL_SEN_UTI_CLK_SPCLK2_SOURCE: {
#ifndef __KERNEL__
			if (((UINT32)p_param) == CTL_SEN_CLKSRC_480) {
				pll_set_clock_rate(PLL_CLKSEL_SP2, PLL_CLKSEL_SP2_480);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL5) {
				pll_set_clock_rate(PLL_CLKSEL_SP2, PLL_CLKSEL_SP2_PLL5);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL6) {
				pll_set_clock_rate(PLL_CLKSEL_SP2, PLL_CLKSEL_SP2_PLL6);
			} else {
				CTL_SEN_DBG_ERR("not support %d=%d\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_IN_PARAM;
			}
#else
			struct clk *parent_clk = NULL, *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_SPCLK2);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
			}

			if (((UINT32)p_param) == CTL_SEN_CLKSRC_480) {
				parent_clk = clk_get(NULL, CG_STR_FIX480);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL5) {
				parent_clk = clk_get(NULL, CG_STR_PLL5);
			} else if (((UINT32)p_param) == CTL_SEN_CLKSRC_PLL6) {
				parent_clk = clk_get(NULL, CG_STR_PLL6);
			} else {
				CTL_SEN_DBG_ERR("not support %d=%d\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_IN_PARAM;
			}

			if (!IS_ERR(parent_clk)) {
				clk_set_parent(src_clk, parent_clk);
			} else {
				CTL_SEN_DBG_ERR("%d,%d: get parent clk fail\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_SYS;
			}

			clk_put(parent_clk);
			clk_put(src_clk);
#endif
		}
		break;

	case CTL_SEN_UTI_CLK_SIEMCLK_FREQUENCY: {
#ifndef __KERNEL__
			pll_set_clock_freq(SIEMCLK_FREQ, (UINT32) p_param);
#else
			struct clk *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_MCLK);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			clk_set_rate(src_clk, (unsigned long)p_param);

			clk_put(src_clk);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_SIEMCLK2_FREQUENCY: {
#ifndef __KERNEL__
			pll_set_clock_freq(SIEMCLK2_FREQ, (UINT32) p_param);
#else
			struct clk *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_MCLK2);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			clk_set_rate(src_clk, (unsigned long)p_param);

			clk_put(src_clk);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_SIEMCLK3_FREQUENCY: {
#ifndef __KERNEL__
			pll_set_clock_freq(SIEMCLK3_FREQ, (UINT32) p_param);
#else
			struct clk *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_MCLK3);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			clk_set_rate(src_clk, (unsigned long)p_param);

			clk_put(src_clk);
#endif
		}
		break;

	case CTL_SEN_UTI_CLK_SPCLK_FREQUENCY: {
#ifndef __KERNEL__
			pll_set_clock_freq(SPCLK_FREQ, (UINT32) p_param);
#else
			struct clk *src_clk = NULL;

			src_clk = clk_get(NULL, CG_STR_SPCLK);
			if (IS_ERR(src_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			clk_set_rate(src_clk, (unsigned long)p_param);

			clk_put(src_clk);
#endif
		}
		break;


	case CTL_SEN_UTI_CLK_PLL5_ENABLE: {
#ifndef __KERNEL__
			pll_set_pll_enable(PLL_ID_5, ((UINT32) p_param) > 0);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_PLL6_ENABLE: {
#ifndef __KERNEL__
			pll_set_pll_enable(PLL_ID_6, ((UINT32) p_param) > 0);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_PLL12_ENABLE: {
#ifndef __KERNEL__
			pll_set_pll_enable(PLL_ID_12, ((UINT32) p_param) > 0);
#endif
		}
		break;

	case CTL_SEN_UTI_CLK_PLL5_FREQUENCY: {
#ifndef __KERNEL__
			pll_set_pll_freq(PLL_ID_5, (UINT32) p_param);
#else
			struct clk *parent_clk = NULL;

			parent_clk = clk_get(NULL, CG_STR_PLL5);
			if (IS_ERR(parent_clk)) {
				CTL_SEN_DBG_ERR("%d,%d: get parent clk fail\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_SYS;
			}

			clk_set_rate(parent_clk, (unsigned long)p_param);
			clk_put(parent_clk);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_PLL6_FREQUENCY: {
#ifndef __KERNEL__
			pll_set_pll_freq(PLL_ID_6, (UINT32) p_param);
#else
			struct clk *parent_clk = NULL;

			parent_clk = clk_get(NULL, CG_STR_PLL6);
			if (IS_ERR(parent_clk)) {
				CTL_SEN_DBG_ERR("%d,%d: get parent clk fail\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_SYS;
			}

			clk_set_rate(parent_clk, (unsigned long)p_param);
			clk_put(parent_clk);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_PLL12_FREQUENCY: {
#ifndef __KERNEL__
			pll_set_pll_freq(PLL_ID_12, (UINT32) p_param);
#else
			struct clk *parent_clk = NULL;

			parent_clk = clk_get(NULL, CG_STR_PLL12);
			if (IS_ERR(parent_clk)) {
				CTL_SEN_DBG_ERR("%d,%d: get parent clk fail\r\n", (UINT32)param_id, (UINT32)p_param);
				rt = CTL_SEN_E_SYS;
			}

			clk_set_rate(parent_clk, (unsigned long)p_param);
			clk_put(parent_clk);
#endif
		}
		break;
	default:
		CTL_SEN_DBG_ERR("Err param_id 0x%08X\r\n", param_id);
		return CTL_SEN_E_IN_PARAM;
	}
	return rt;
}

INT32 __sen_uti_get_clk(CTL_SEN_UTI_CLK param_id, VOID *p_param)
{
	UINT32 *ret;
	INT32 rt = CTL_SEN_E_OK;

	ret = (UINT32 *)p_param;

	switch (param_id) {
	case CTL_SEN_UTI_CLK_PLL5_FREQUENCY: {
#ifndef __KERNEL__
			*ret = pll_get_pll_freq(PLL_ID_5);
#if 0 //(KDRV_CG_READY == DISABLE)
			{
				DBG_WRN("simulation get PLL5=297000000\r\n");
				*ret = 297000000;
			}
#endif
#else
			struct clk *parent_clk = NULL;

			parent_clk = clk_get(NULL, CG_STR_PLL5);
			if (IS_ERR(parent_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			*ret = clk_get_rate(parent_clk);
			clk_put(parent_clk);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_PLL6_FREQUENCY: {
#ifndef __KERNEL__
			*ret = pll_get_pll_freq(PLL_ID_6);
#else
			struct clk *parent_clk = NULL;

			parent_clk = clk_get(NULL, CG_STR_PLL6);
			if (IS_ERR(parent_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			*ret = clk_get_rate(parent_clk);
			clk_put(parent_clk);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_PLL12_FREQUENCY: {
#ifndef __KERNEL__
			*ret = pll_get_pll_freq(PLL_ID_12);
#else
			struct clk *parent_clk = NULL;

			parent_clk = clk_get(NULL, CG_STR_PLL12);
			if (IS_ERR(parent_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			*ret = clk_get_rate(parent_clk);
			clk_put(parent_clk);
#endif
		}
		break;

	case CTL_SEN_UTI_CLK_PLL5_ENABLE: {
#ifndef __KERNEL__
			*ret = pll_get_pll_enable(PLL_ID_5);
#else
			struct clk *parent_clk = NULL;

			parent_clk = clk_get(NULL, CG_STR_PLL5);
			if (IS_ERR(parent_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			*ret = __clk_is_enabled(parent_clk);

			clk_put(parent_clk);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_PLL6_ENABLE: {
#ifndef __KERNEL__
			*ret = pll_get_pll_enable(PLL_ID_6);
#else
			struct clk *parent_clk = NULL;

			parent_clk = clk_get(NULL, CG_STR_PLL6);
			if (IS_ERR(parent_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			*ret = __clk_is_enabled(parent_clk);

			clk_put(parent_clk);
#endif
		}
		break;
	case CTL_SEN_UTI_CLK_PLL12_ENABLE: {
#ifndef __KERNEL__
			*ret = pll_get_pll_enable(PLL_ID_12);
#else
			struct clk *parent_clk = NULL;

			parent_clk = clk_get(NULL, CG_STR_PLL12);
			if (IS_ERR(parent_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			*ret = __clk_is_enabled(parent_clk);

			clk_put(parent_clk);
#endif
		}
		break;

	case CTL_SEN_UTI_CLK_MIPILVDS_FREQUENCY: {
#ifndef __KERNEL__
			if (pll_get_clock_rate(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_60) {
				*ret = CTL_SEN_60M_HZ;
			} else if (pll_get_clock_rate(PLL_CLKSEL_MIPI_LVDS) == PLL_CLKSEL_MIPI_LVDS_120) {
				*ret = CTL_SEN_120M_HZ;
			} else {
				CTL_SEN_DBG_ERR("%d: get clk outofrange 0x%.8x\r\n", (UINT32)param_id, (unsigned int)pll_get_clock_rate(PLL_CLKSEL_MIPI_LVDS));
				*ret = CTL_SEN_60M_HZ;
				rt = CTL_SEN_E_SYS;
			}
#else
			struct clk *parent;
			struct clk *csi_clk;

			csi_clk = clk_get(NULL, CG_STR_CSI);
			if (IS_ERR(csi_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			parent = clk_get_parent(csi_clk);
			*ret = clk_get_rate(parent);
			clk_put(csi_clk);
#endif
		}
		break;

	case CTL_SEN_UTI_CLK_MIPILVDS2_FREQUENCY: {
#ifndef __KERNEL__
			if (pll_get_clock_rate(PLL_CLKSEL_MIPI_LVDS2) == PLL_CLKSEL_MIPI_LVDS2_60) {
				*ret = CTL_SEN_60M_HZ;
			} else if (pll_get_clock_rate(PLL_CLKSEL_MIPI_LVDS2) == PLL_CLKSEL_MIPI_LVDS2_120) {
				*ret = CTL_SEN_120M_HZ;
			} else {
				CTL_SEN_DBG_ERR("%d: get clk outofrange 0x%.8x\r\n", (UINT32)param_id, (unsigned int)pll_get_clock_rate(PLL_CLKSEL_MIPI_LVDS2));
				*ret = CTL_SEN_60M_HZ;
				rt = CTL_SEN_E_SYS;
			}
#else
			struct clk *parent;
			struct clk *csi_clk;

			csi_clk = clk_get(NULL, CG_STR_CSI2);
			if (IS_ERR(csi_clk)) {
				CTL_SEN_DBG_ERR("%d: get src clk fail\r\n", (UINT32)param_id);
				rt = CTL_SEN_E_SYS;
			}

			parent = clk_get_parent(csi_clk);
			*ret = clk_get_rate(parent);
			clk_put(csi_clk);
#endif
		}
		break;

	default:
		CTL_SEN_DBG_ERR("Err param_id 0x%08X\r\n", param_id);
		return CTL_SEN_E_IN_PARAM;
	}
	return rt;
}
