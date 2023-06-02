/*
	Internal header file for PWM module.

	This file is the internal header file that defines the variables, structures
	and function prototypes of PWM module.

	@file       pwm_int.h
	@ingroup    mIOPWM
	@note       Nothing.

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef _PWM_INT_H
#define _PWM_INT_H

//#include <rcw_macro.h>

typedef enum
{
	PWM_STOP = 0x0,
	PWM_RUNNING,

	ENUM_DUMMY4WORD(PWM_OPERATING)
}PWM_OPERATING;

/*
	@name PWM number ID

	PWM number ID for PWM driver API
*/
typedef enum
{
	PWMID_NO_0 = 0x0,
	PWMID_NO_1,
	PWMID_NO_2,
	PWMID_NO_3,

	PWMID_NO_4,
	PWMID_NO_5,
	PWMID_NO_6,
	PWMID_NO_7,

	PWMID_NO_8,
	PWMID_NO_9,
	PWMID_NO_10,
	PWMID_NO_11,

        PWMID_NO_12,
        PWMID_NO_13,
        PWMID_NO_14,
        PWMID_NO_15,

        PWMID_NO_16,
        PWMID_NO_17,
        PWMID_NO_18,
        PWMID_NO_19,


	PWMID_NO_TOTAL_CNT,

	ENUM_DUMMY4WORD(PWM_ID_NO)
}PWM_ID_NO;

#define PWM_INT_STS_0               0x00000001
#define PWM_INT_STS_1               0x00000002
#define PWM_INT_STS_2               0x00000004
#define PWM_INT_STS_3               0x00000008
#define PWM_INT_STS_4               0x00000010
#define PWM_INT_STS_5               0x00000020
#define PWM_INT_STS_6               0x00000040
#define PWM_INT_STS_7               0x00000080
#define PWM_INT_STS_8               0x00000100
#define PWM_INT_STS_9               0x00000200
#define PWM_INT_STS_10              0x00000400
#define PWM_INT_STS_11              0x00000800
#define PWM_INT_STS_12              0x00001000
#define PWM_INT_STS_13              0x00002000
#define PWM_INT_STS_14              0x00004000
#define PWM_INT_STS_15              0x00008000
#define PWM_INT_STS_16              0x00010000
#define PWM_INT_STS_17              0x00020000
#define PWM_INT_STS_18              0x00040000
#define PWM_INT_STS_19              0x00080000
#define PWM_INT_STS_ALL             0x000FFFFF

#define PWM_INT_CLKDIV_STS_ALL      0x00070007

#define PWM_INVALID_ID              0x80000000


// bit map definition of PWM control & interrupt enable register
#define PWM_INTE_MASK               0x00000FFF

#define PWM_EN_SHIFT                16
#define PWM_PWMCH_BITS              12
#define PWM_ALLCH_BITS              12
#define PWM_PER_CH_OFFSET           8

#endif



