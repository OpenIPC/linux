/**
    HW power utility.

    HW power utility.

    @file       HwPower.h
    @ingroup    mIUtilHWPower
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/

#ifndef _HWPOWER_H
#define _HWPOWER_H

#if defined(__FREERTOS) // only support on freertos

/**
    @addtogroup mIUtilHWPower
*/
//@{


#define POWER_STATE_SRC     0 ///< power on source (POWER_ID_XXX)
#define POWER_STATE_FIRST   1 ///< power on first time (power lost)

/**
     Check state

     Check state on power on

     @param[in] stateID     state index.

     @return value.

     EX:
     hwpower_get_poweron_state(POWER_STATE_SRC);
     hwpower_get_poweron_state(POWER_STATE_FIRST);
*/
UINT32 hwpower_get_poweron_state(UINT32 stateID);

#define POWER_ID_PSW1       0 ///< power switch 1, power off timer
#define POWER_ID_PSW2       1 ///< power switch 2
#define POWER_ID_PSW3       2 ///< power switch 3
#define POWER_ID_PSW4       3 ///< power switch 4
#define POWER_ID_HWRT       4 ///< h/w reset timer (power alarm timer)
#define POWER_ID_SWRT       5 ///< s/w reset timer (watch dog timer)

/**
     Get power key

     Get power key

     @param[in] pwrID       power index.

     @return value.

     EX:
     hwpower_get_power_key(POWER_ID_PSW1);
     hwpower_get_power_key(POWER_ID_PSW2);
     hwpower_get_power_key(POWER_ID_PSW3);
     hwpower_get_power_key(POWER_ID_PSW4);
     hwpower_get_power_key(POWER_ID_HWRT);
     hwpower_get_power_key(POWER_ID_SWRT);
*/
UINT32 hwpower_get_power_key(UINT32 pwrID);

/**
     Set power key

     Set power key

     @param[in] pwrID       power index.
     @param[in] value       value.

     EX:
     hwpower_set_power_key(POWER_ID_PSW1, 0xf0); //keep power on - clear power off timer
     hwpower_set_power_key(POWER_ID_PSW1, 0xff); //power off NOW
     hwpower_set_power_key(POWER_ID_HWRT, 0); //disable h/w reset timer
     hwpower_set_power_key(POWER_ID_HWRT, 1); //enable h/w reset timer
     hwpower_set_power_key(POWER_ID_HWRT, 0xf0); //keep power on - clear h/w reset timer
     hwpower_set_power_key(POWER_ID_HWRT, 0xff); //h/w reset NOW
     hwpower_set_power_key(POWER_ID_SWRT, 0); //disable s/w reset timer
     hwpower_set_power_key(POWER_ID_SWRT, 1); //enable s/w reset timer
     hwpower_set_power_key(POWER_ID_SWRT, 0xf0); //keep power on - clear s/w reset timer
     hwpower_set_power_key(POWER_ID_SWRT, 0xff); //s/w reset NOW
*/
void hwpower_set_power_key(UINT32 pwrID, UINT32 value);


//@}

#endif
#endif //_HWPOWER_H

