/**
	@file       pwm.h
	@ingroup    mIDrvIO_PWM

	@brief      Header file for PWM module
	            This file is the header file that define the API for PWM module

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.

*/
#ifndef _PWM_H
#define _PWM_H

#include <linux/types.h>
#include "top.h"


/**
	@addtogroup mIDrvIO_PWM
*/
//@{

struct nvt_pwm_platform_data {
	uint32_t	clk_src;	/* clk source */
};



/**
	PWM type (PWM or Micro step)
*/
typedef enum
{
	PWM_TYPE_PWM = 0x0,             ///< PWM
	//PWM_TYPE_MICROSTEP,             ///< Micro step
	//PWM_TYPE_CCNT,                  ///< CCNT
	//PWM_TYPE_CNT,
	ENUM_DUMMY4WORD(PWM_TYPE)
} PWM_TYPE;


/**
    PWM clock divid

    @note for pwm_pwmConfigClockDiv()
*/
typedef enum
{
	PWM0_3_CLKDIV   = 0x0,          ///< PWM0~PWM3 clock divid
	PWM4_7_CLKDIV,                  ///< PWM4~PWM7 clock divid
	PWM8_11_CLKDIV,                 ///< PWM8~PWM11 clock divid
	PWM12_CLKDIV    = 12,           ///< PWM12 clock divid
	PWM13_CLKDIV,                   ///< PWM13 clock divid
	PWM14_CLKDIV,                   ///< PWM14 clock divid
	PWM15_CLKDIV,                   ///< PWM15 clock divid
	PWM16_CLKDIV,                   ///< PWM16 clock divid
	PWM17_CLKDIV,                   ///< PWM17 clock divid
	PWM18_CLKDIV,                   ///< PWM18 clock divid
	PWM19_CLKDIV,                   ///< PWM19 clock divid
	ENUM_DUMMY4WORD(PWM_CLOCK_DIV)
} PWM_CLOCK_DIV;


/**
	PWM configuration structure

	@note for pwm_set()
*/
typedef struct
{
	u32  base_period;               ///< Base period, how many PWM clock per period, 2 ~ 255
                                        ///< @note rise <= fall <= base_period
	u32  rise;                      ///< Rising at which clock
                                        ///< @note rise <= fall <= base_period
	u32  fall;                      ///< Falling at which clock
                                        ///< @note rise <= fall <= base_period
	u32  cycle;                     ///< Output cycle, 0 ~ 65535
                                        ///< - @b PWM_FREE_RUN: Free Run
                                        ///< - @b Others: How many cycles (PWM will stop after output the cycle counts)
	u32  inv;                       ///< Invert PWM output signal or not
                                        ///< - @b PWM_SIGNAL_NORMAL: Don't invert PWM output signal
                                        ///< - @b PWM_SIGNAL_INVERT: Invert PWM output signal
} PWM_CFG, *PPWM_CFG;




/**
	@name PWM ID

	PWM ID for PWM driver API

	@note for pwm_open(), pwm_set(), pwm_setCCNT(), pwm_setCCNTToutEN(), pwm_wait(), pwm_stop(), pwm_en(), pwm_reload(),\n
              pwm_ms_set(), pwm_ms_stop(), pwm_ms_en(), pwm_getCycleNumber(), pwm_ccntGetCurrentVal(), pwm_ccntGetCurrentVal(),\n
              pwm_ccntEnable()
*/
//@{
#define PWMID_0                 0x00000001  ///< PWM ID 0
#define PWMID_1                 0x00000002  ///< PWM ID 1
#define PWMID_2                 0x00000004  ///< PWM ID 2
#define PWMID_3                 0x00000008  ///< PWM ID 3
#define PWMID_4                 0x00000010  ///< PWM ID 4
#define PWMID_5                 0x00000020  ///< PWM ID 5
#define PWMID_6                 0x00000040  ///< PWM ID 6
#define PWMID_7                 0x00000080  ///< PWM ID 7
#define PWMID_8                 0x00000100  ///< PWM ID 8
#define PWMID_9                 0x00000200  ///< PWM ID 9
#define PWMID_10                0x00000400  ///< PWM ID 10
#define PWMID_11                0x00000800  ///< PWM ID 11
#define PWMID_12                0x00001000  ///< PWM ID 12
#define PWMID_13                0x00002000  ///< PWM ID 13
#define PWMID_14                0x00004000  ///< PWM ID 14
#define PWMID_15                0x00008000  ///< PWM ID 15
#define PWMID_16                0x00010000  ///< PWM ID 16
#define PWMID_17                0x00020000  ///< PWM ID 17
#define PWMID_18                0x00040000  ///< PWM ID 18
#define PWMID_19                0x00080000  ///< PWM ID 19


#endif
