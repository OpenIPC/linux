/*
    Novatek protected header file of NT98520's driver.

    The header file for Novatek protected APIs of NT98520's driver.

    @file       pwm_protected.h
    @ingroup    mIDriver
    @note       For Novatek internal reference, don't export to agent or customer.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _NVT_PWM_PROTECTED_H
#define _NVT_PWM_PROTECTED_H

#ifdef __KERNEL__
#include "kwrap/type.h"
#else
#include "type.h"
#endif

/*
    PWM debug port select

    @note: Internal usage

    @param ui_debug_sel      debug select

    @return E_OK
*/
extern ER pwm_debug_sel(UINT32 ui_debug_sel);
extern void pwm_setdestination(UINT32 ui_pwm_id, UINT32 ui_config);
#endif

