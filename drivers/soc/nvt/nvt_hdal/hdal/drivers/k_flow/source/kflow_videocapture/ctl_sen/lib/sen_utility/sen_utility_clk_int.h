#ifndef _SEN_UTILITY_CLK_INT_H_
#define _SEN_UTILITY_CLK_INT_H_

#include "sen_int.h"
#include "sen_dbg_infor_int.h"

#define CG_STR_MCLK     "f0c00000.siemck"
#define CG_STR_MCLK2    "f0c00000.siemk2"
#define CG_STR_MCLK3    "f0c00000.siemk3"
#if KDRV_CG_READY
#else
#include "../../../../../../k_driver/source/include/pll_protected.h"
#define CG_STR_SPCLK    ""
#define CG_STR_SPCLK2   ""
#endif

#define CG_STR_FIX480   "fix480m"
#define CG_STR_FIX320   "fix320m"
#define CG_STR_PLL5     "pll5"
#define CG_STR_PLL6     "pll6"
#define CG_STR_PLL12    "pll12"
#define CG_STR_CSI    	"f0280000.csi"
#define CG_STR_CSI2    	"f0330000.csi"

typedef enum {
	CTL_SEN_UTI_CLK_SIEMCLK_ENABLE,          ///< [set] Set SIEMCLK ENABLE/DISABLE
	CTL_SEN_UTI_CLK_SIEMCLK_SOURCE,          ///< [set] Set SIEMCLK Souce. Please use CTL_SEN_CLKSRC_SEL
	CTL_SEN_UTI_CLK_SIEMCLK_FREQUENCY,       ///< [set] Set SIEMCLK Frequency. Unit in Hertz.

	CTL_SEN_UTI_CLK_SIEMCLK2_ENABLE,         ///< [set] Set SIEMCLK2 ENABLE/DISABLE
	CTL_SEN_UTI_CLK_SIEMCLK2_SOURCE,         ///< [set] Set SIEMCLK2 Souce. Please use CTL_SEN_CLKSRC_SEL
	CTL_SEN_UTI_CLK_SIEMCLK2_FREQUENCY,      ///< [set] Set SIEMCLK2 Frequency. Unit in Hertz.

	CTL_SEN_UTI_CLK_SIEMCLK3_ENABLE,         ///< [set] Set SIEMCLK3 ENABLE/DISABLE
	CTL_SEN_UTI_CLK_SIEMCLK3_SOURCE,         ///< [set] Set SIEMCLK3 Souce. Please use CTL_SEN_CLKSRC_SEL
	CTL_SEN_UTI_CLK_SIEMCLK3_FREQUENCY,      ///< [set] Set SIEMCLK3 Frequency. Unit in Hertz.

	CTL_SEN_UTI_CLK_SPCLK_ENABLE,            ///< [set] Set SPCLK ENABLE/DISABLE
	CTL_SEN_UTI_CLK_SPCLK_SOURCE,            ///< [set] Set SPCLK Souce. Please use CTL_SEN_CLKSRC_SEL
	CTL_SEN_UTI_CLK_SPCLK_FREQUENCY,         ///< [set] Set SPCLK Frequency. Unit in Hertz.

	CTL_SEN_UTI_CLK_SPCLK2_ENABLE,           ///< [set] Set SPCLK2 ENABLE/DISABLE
	CTL_SEN_UTI_CLK_SPCLK2_SOURCE,           ///< [set] Set SPCLK2 Souce. Please use CTL_SEN_CLKSRC_SEL
	CTL_SEN_UTI_CLK_SPCLK2_FREQUENCY,        ///< [set] Set SPCLK2 Frequency. Unit in Hertz.


	CTL_SEN_UTI_CLK_PLL5_ENABLE,             ///< [set/get] Set PLL5 ENABLE/DISABLE
	CTL_SEN_UTI_CLK_PLL5_FREQUENCY,          ///< [set/get] Set PLL5 Frequency

	CTL_SEN_UTI_CLK_PLL6_ENABLE,             ///< [set/get] Set PLL6 ENABLE/DISABLE
	CTL_SEN_UTI_CLK_PLL6_FREQUENCY,          ///< [set/get] Set PLL6 Frequency

	CTL_SEN_UTI_CLK_PLL12_ENABLE,            ///< [set/get] Set PLL12 ENABLE/DISABLE
	CTL_SEN_UTI_CLK_PLL12_FREQUENCY,         ///< [set/get] Set PLL12 Frequency

	CTL_SEN_UTI_CLK_MIPILVDS_FREQUENCY,      ///< [get] Get MIPI_LVDS Frequency
	CTL_SEN_UTI_CLK_MIPILVDS2_FREQUENCY,     ///< [get] Get MIPI_LVDS2 Frequency

} CTL_SEN_UTI_CLK;

INT32 __sen_uti_set_clk(CTL_SEN_UTI_CLK param_id, VOID *p_param);
#define sen_uti_set_clk(param_id, p_param) __sen_uti_set_clk((CTL_SEN_UTI_CLK)(param_id), (VOID *)(p_param))
INT32 __sen_uti_get_clk(CTL_SEN_UTI_CLK param_id, VOID *p_param);
#define sen_uti_get_clk(param_id, p_param) __sen_uti_get_clk((CTL_SEN_UTI_CLK)(param_id), (VOID *)(p_param))

#endif //_SEN_UTILITY_CLK_INT_H_
