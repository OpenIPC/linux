/*!
*****************************************************************************
** \file        arch/arm/mach-gk7101/include/plat/highres_timer.h
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
#ifndef __PLAT_HIGHRES_TIMER_H
#define __PLAT_HIGHRES_TIMER_H

extern void highres_timer_init(struct hrtimer *timer, clockid_t clock_id, enum hrtimer_mode mode);
extern int highres_timer_start(struct hrtimer *timer, ktime_t tim, const enum hrtimer_mode mode);
extern int highres_timer_cancel(struct hrtimer *timer);

#endif

