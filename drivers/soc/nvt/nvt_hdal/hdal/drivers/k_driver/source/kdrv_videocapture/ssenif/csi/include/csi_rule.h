#ifndef _CSI_RULE_H_
#define _CSI_RULE_H_
#include <kwrap/util.h>
#include "csi_platform.h"

#ifdef __KERNEL__
//#include "csi_drv.h"
//extern PMODULE_INFO pcsi_mod_info;
//#define CSI_ENABLE_PLLCLK(x)   clk_enable(pcsi_mod_info->pclk[x])
//#define CSI_DISABLE_PLLCLK(x)  clk_disable(pcsi_mod_info->pclk[x])
#define CSI_ENABLE_DRVINT(x)
#define CSI_DISABLE_DRVINT(x)
#define CSI_ISENABLE_PLLCLK(x) 1
#define CSI_SET_PLLCLKRATE(x, y)
#define CSI_GET_PLLCLKRATE(x)  0
#define CSI_GET_PLLCLKFREQ(x, y)


#define MIPI_LVDS_CLK          0
#define MIPI_LVDS2_CLK         0
#define MIPI_LVDS3_CLK         0
#define MIPI_LVDS4_CLK         0
#define MIPI_LVDS5_CLK         0
#define MIPI_LVDS6_CLK         0
#define MIPI_LVDS7_CLK         0
#define MIPI_LVDS8_CLK         0

#else
#include "comm/timer.h"

//#define CSI_ENABLE_DRVINT(x)    drv_enableInt((x))
//#define CSI_DISABLE_DRVINT(x)   drv_disableInt((x))
#define CSI_ISENABLE_PLLCLK(x)  pll_is_clock_enabled((x))
#define CSI_SET_PLLCLKRATE      pll_set_clock_rate
#define CSI_GET_PLLCLKRATE      pll_get_clock_rate
#define CSI_GET_PLLCLKFREQ      pll_get_clock_freq


#define SW_TIMER_OPEN           SwTimer_Open
#define SW_TIMER_CFG            SwTimer_Cfg
#define SW_TIMER_START          SwTimer_Start
#define SW_TIMER_STOP           SwTimer_Stop
#define SW_TIMER_CLOSE          SwTimer_Close
//#define PERF_MARK               Perf_Mark
//#define PERF_GETDURATION        Perf_GetDuration

#endif
#define CSI_DELAY_US(x)        vos_util_delay_us((x))
#define CSI_DELAY_MS(x)        vos_util_delay_ms((x))

#endif
