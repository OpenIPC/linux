/*
    Protected header for timer module

    This file is the header file that define the API and data type for timer
    module.

    @file       timer_protected.h
    @ingroup    miDrvTimer_Timer
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _TIMER_PROTECTED_H
#define _TIMER_PROTECTED_H

#if defined __UITRON || defined __ECOS
#include "Type.h"
#include "timer.h"
#elif defined __FREERTOS
#include <comm/timer.h>
#else
#include "mach/rcw_macro.h"
#include "kwrap/type.h"
#include "comm/timer.h"
#endif

/*
    Open timer ISR.

    Same as timer_open() except this API will invoke callback in ISR instead of IST.

    @note Please DON'T use this API. Only LEADER PERMIITED application can use this API.

    @param[out] p_id            Timer ID that is allocated
    @param[in] callback         Timer expired callback function. Assign NULL if callback is not required.

    @return Operation status
	- @b E_OK   : Everything is OK
	- @b E_PAR  : Parameters are not valid
	- @b E_SYS  : Maximum number of timers is exceeded
*/
extern ER timer_open_isr(PTIMER_ID p_id, DRV_CB callback);

#endif




