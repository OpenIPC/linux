/*
    Timer module driver

    This file is the driver of timer module

    @file       timer_export.c
    @ingroup    miDrvTimer_Timer
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/****************************************************************************/
/*                                                                          */
/*  Todo: Modify register accessing code to the macro in RCWMacro.h         */
/*                                                                          */
/****************************************************************************/

#include "timer.h"
#include <linux/export.h>

EXPORT_SYMBOL(timer_open);
EXPORT_SYMBOL(timer_open_auto_close);
EXPORT_SYMBOL(timer_close);
EXPORT_SYMBOL(timer_cfg);
EXPORT_SYMBOL(timer_pause_play);
EXPORT_SYMBOL(timer_wait_timeup);
EXPORT_SYMBOL(timer_check_timeup);
EXPORT_SYMBOL(timer_reload);
EXPORT_SYMBOL(timer_set_config);
EXPORT_SYMBOL(timer_get_config);
EXPORT_SYMBOL(timer_get_sys_timer_id);
EXPORT_SYMBOL(timer_get_current_count);
EXPORT_SYMBOL(timer_get_target_count);
