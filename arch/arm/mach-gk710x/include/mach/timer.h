/*!
*****************************************************************************
** \file        arch/arm/mach-gk7101/include/mach/timer.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef __MACH_TIMER_H__
#define __MACH_TIMER_H__

#include <mach/hardware.h>

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#define INTERVAL_TIMER_INSTANCES    3

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/
#define TIMER_BASE                  GK_VA_TIMER
#define TIMER_REG(x)                (GK_VA_TIMER + (x))

#define TIMER_CTR_OFFSET            0x0c

#define TIMER1_STATUS_OFFSET        0x00
#define TIMER1_MATCH1_OFFSET        0x04
#define TIMER1_MATCH2_OFFSET        0x08

#define TIMER2_STATUS_OFFSET        0x14
#define TIMER2_MATCH1_OFFSET        0x18
#define TIMER2_MATCH2_OFFSET        0x1c

#define TIMER3_STATUS_OFFSET        0x20
#define TIMER3_MATCH1_OFFSET        0x24
#define TIMER3_MATCH2_OFFSET        0x28

#define TIMER1_RELOAD_OFFSET        0x30
#define TIMER2_RELOAD_OFFSET        0x34
#define TIMER3_RELOAD_OFFSET        0x38

#define TIMER1_STATUS_REG           TIMER_REG(TIMER1_STATUS_OFFSET)
#define TIMER1_RELOAD_REG           TIMER_REG(TIMER1_RELOAD_OFFSET)
#define TIMER1_MATCH1_REG           TIMER_REG(TIMER1_MATCH1_OFFSET)
#define TIMER1_MATCH2_REG           TIMER_REG(TIMER1_MATCH2_OFFSET)
#define TIMER2_STATUS_REG           TIMER_REG(TIMER2_STATUS_OFFSET)
#define TIMER2_RELOAD_REG           TIMER_REG(TIMER2_RELOAD_OFFSET)
#define TIMER2_MATCH1_REG           TIMER_REG(TIMER2_MATCH1_OFFSET)
#define TIMER2_MATCH2_REG           TIMER_REG(TIMER2_MATCH2_OFFSET)
#define TIMER3_STATUS_REG           TIMER_REG(TIMER3_STATUS_OFFSET)
#define TIMER3_RELOAD_REG           TIMER_REG(TIMER3_RELOAD_OFFSET)
#define TIMER3_MATCH1_REG           TIMER_REG(TIMER3_MATCH1_OFFSET)
#define TIMER3_MATCH2_REG           TIMER_REG(TIMER3_MATCH2_OFFSET)

#define TIMER_CTR_REG               TIMER_REG(TIMER_CTR_OFFSET)


/* Bit field definition of timer control register */
#define TIMER_CTR_EN1               0x00000400
#define TIMER_CTR_EN2               0x00000200
#define TIMER_CTR_EN3               0x00000100

#define TIMER_CTR_CSL1              0x00000040
#define TIMER_CTR_CSL2              0x00000020
#define TIMER_CTR_CSL3              0x00000010

#define TIMER_CTR_OF1               0x00000004
#define TIMER_CTR_OF2               0x00000002
#define TIMER_CTR_OF3               0x00000001

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

extern struct sys_timer gk_sys_timer;
extern int gk_init_timer(void) __init;

extern int get_hwtimer_output_ticks(u64 *out_tick);

#ifdef __cplusplus
}
#endif
#endif