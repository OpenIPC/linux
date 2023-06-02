/*
    Internal header file for timer module.

    This file is the internal header file that defines the variables, structures
    and function prototypes of timer module.

    @file       timer_int.h
    @ingroup    miDrvTimer_Timer
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

#ifndef _TIMER_INT_H
#define _TIMER_INT_H

#if defined __UITRON || defined __ECOS
#include "DrvCommon.h"
#include "timer.h"
#include "timer_protected.h"
#elif defined __FREERTOS
#include "include/timer_platform.h"
#include <kwrap/nvt_type.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include "comm/timer.h"
#include "comm/timer_protected.h"
#include <kwrap/debug.h>
#else
#include "timer_platform.h"
#include "timer_dbg.h"
#include "mach/rcw_macro.h"
#include "kwrap/type.h"
#include "comm/timer.h"
#include "comm/timer_protected.h"
#endif

#define TIMER_SETREG(ofs,value)         OUTW(IOADDR_TIMER_REG_BASE+(ofs),(value))
#define TIMER_GETREG(ofs)               INW(IOADDR_TIMER_REG_BASE+(ofs))
#define TIMER_GET_REGOFFSET(ID)         ((ID) << 4)

// Register setting
#define TIMER_REG_STS                   1

#define TIMER_REG_INTEN_DISABLE         0
#define TIMER_REG_INTEN_ENABLE          1

#define TIMER_REG_MODE_ONE_SHOT         0
#define TIMER_REG_MODE_FREE_RUN         1

#define TIMER_REG_CLKSRC_DIV0           0
#define TIMER_REG_CLKSRC_DIV1           1

#define TIMER_REG_EN_DISABLE            0
#define TIMER_REG_EN_ENABLE             1
#define TIMER_REG_EN_RELOAD             1

#define TIMER_DIVIDER_MIN               0
#define TIMER_DIVIDER_MAX               255

// For debug purpose, enable or disable logging owner task ID when timer_open()
#define TIMER_LOG_OWNER                 DISABLE
//#define TIMER_LOG_OWNER                 ENABLE

// Choose timer 0 for system timer ID
#define TIMER_SYSTIMER_ID               TIMER_0

// First and last timer ID and available timer number
#if (_EMULATION_ == ENABLE)
#if (_EMULATION_MULTI_CPU_ == ENABLE)
#if (_EMULATION_ON_CPU2_ == ENABLE)
// Multiple CPU emulation, CPU1 own timer 16 ~ 19
#define TIMER_FIRST_ID      TIMER_16
#define TIMER_LAST_ID       TIMER_19
#else
// Multiple CPU emulation, CPU1 own timer 1 ~ 15
#define TIMER_FIRST_ID      TIMER_1
#define TIMER_LAST_ID       TIMER_15
#endif
#else
// Single CPU emulation, CPU1 own all timers
#define TIMER_FIRST_ID          TIMER_1
#define TIMER_LAST_ID           TIMER_19
#endif
#else
// Release code, CPU1 own timer 1 ~ 15 (***Need confimr***)
#define TIMER_FIRST_ID              TIMER_1
#define TIMER_LAST_ID               TIMER_15
#endif

#if (_FPGA_EMULATION_ == ENABLE)
/*
FPGA Real chip
pll2*2 CLK480 ==> 480 is use pll2 @ FPGA Y 12 fixed @ 24MHz
pll1/2 CLK192
pll1*2 PLL2
x PLL3
pll1*2 PLL4
pll1*2 PLL5
pll1 PLL6
pll3 PLL7
pll2*2 PLL8
pll1*2/5 PLL9
pll1*2/4 PLL10
pll1*2 PLL11
pll3*5 PLL12
pll1*2/3 PLL13
x PLL14

*/
// OSC * 2 / 160
#define TIMER_SOURCE_CLOCK              (_FPGA_PLL_OSC_ / 80)
#else
// 3 MHz (480 MHz / 160)
#define TIMER_SOURCE_CLOCK              3000000
#endif

typedef enum {
	TIMER_DRV_STATE_CLOSE,
	TIMER_DRV_STATE_OPEN,
	TIMER_DRV_STATE_PLAY,
	ENUM_DUMMY4WORD(TIMER_DRV_STATE)
} TIMER_DRV_STATE;

// Register default value
#define TIMER_DST_REG_DEFAULT           0x00000000
#define TIMER_INT_REG_DEFAULT           0x00000000
#define TIMER_STATUS_REG_DEFAULT        0x00000000
#define TIMER_CLKDIV_REG_DEFAULT        0x00010202
#define TIMER_CTRLX_REG_DEFAULT         0x00000000
#define TIMER_TARGETX_REG_DEFAULT       0xFFFFFFFF
#define TIMER_COUNTERX_REG_DEFAULT      0x00000000
#define TIMER_RELOADX_REG_DEFAULT       0x00000000

/*
    Timer register default value

    Timer register default value.
*/
typedef struct {
	UINT32  uiOffset;
	UINT32  uiValue;
	CHAR    *pName;
} TIMER_REG_DEFAULT;

#endif
