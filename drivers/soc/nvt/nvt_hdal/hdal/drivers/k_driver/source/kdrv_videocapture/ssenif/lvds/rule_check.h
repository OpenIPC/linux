#ifndef _RULE_CHECK_H_
#define _RULE_CHECK_H_

#ifdef __KERNEL__

#define LVDS_DELAY_US_POLL(x)	vos_util_delay_us_polling((x))
#define LVDS_DELAY_MS(x)		vos_util_delay_ms((x))
#define LVDS_ENABLE_CLK(x)		clk_enable(plvds_mod_info->pclk[x])
#define LVDS_DISABLE_CLK(x)		clk_disable(plvds_mod_info->pclk[x])

#else
#include <kwrap/task.h>
#include <comm/timer.h>

#define LVDS_DELAY_US_POLL(x)	vos_util_delay_us_polling(x)
#define LVDS_DELAY_MS(x)		vos_util_delay_ms((x))
#define LVDS_ENABLE_CLK(x)		pll_enable_clock((x))
#define LVDS_DISABLE_CLK(x)		pll_disable_clock((x))

#define SW_TIMER_OPEN			//SwTimer_Open
#define SW_TIMER_CFG			//SwTimer_Cfg
#define SW_TIMER_START			//SwTimer_Start
#define SW_TIMER_STOP			//SwTimer_Stop
#define SW_TIMER_CLOSE			//SwTimer_Close

#endif








#endif
