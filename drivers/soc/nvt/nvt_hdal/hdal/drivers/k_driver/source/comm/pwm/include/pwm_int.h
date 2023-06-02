/*
    Internal header file for PWM module.

    This file is the internal header file that defines the variables, structures
    and function prototypes of PWM module.

    @file       pwm_int.h
    @ingroup    mIOPWM
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/

#ifndef _PWM_INT_H
#define _PWM_INT_H

#include "pwm_platform.h"

/*
    @name   PWM Micro step step per operation unit

    @note   for
*/
typedef enum {
	TOTAL_008_STEP_PER_OPERATION_UNIT = 0x3,
	TOTAL_016_STEP_PER_OPERATION_UNIT,
	TOTAL_032_STEP_PER_OPERATION_UNIT,
	TOTAL_064_STEP_PER_OPERATION_UNIT,
	TOTAL_128_STEP_PER_OPERATION_UNIT,

	STEP_PER_OPERATION_UNIT_CNT,
	ENUM_DUMMY4WORD(PWM_MS_STEP_PER_UNIT)
} PWM_MS_STEP_PER_UNIT;

/*
    @name   PWM Micro step step per operation unit

    @note   for
*/
typedef enum {
	TOTAL_1_GRAD_PER_STEP = 0x0,
	TOTAL_2_GRAD_PER_STEP,
	TOTAL_4_GRAD_PER_STEP,
	TOTAL_8_GRAD_PER_STEP,

	ENUM_DUMMY4WORD(PWM_MS_GRAD_PER_STEP)
} PWM_MS_GRAD_PER_STEP;

typedef enum {
	PWM_STOP = 0x0,
	PWM_RUNNING,

	ENUM_DUMMY4WORD(PWM_OPERATING)
} PWM_OPERATING;

/*
    @name PWM number ID

    PWM number ID for PWM driver API
*/
typedef enum {
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

	PWMID_CCNT_NO_0,
	PWMID_CCNT_NO_1,
	PWMID_CCNT_NO_2,

	PWMID_NO_TOTAL_CNT,

	ENUM_DUMMY4WORD(PWM_ID_NO)
} PWM_ID_NO;

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
#define PWM_INT_STS_ALL             0x00000FFF

#define PWM_INT_MS_STS_0            0x00000001
#define PWM_INT_MS_STS_1            0x00000002
#define PWM_INT_MS_STS_2            0x00000004
#define PWM_INT_MS_STS_3            0x00000008
#define PWM_INT_MS_STS_4            0x00000010
#define PWM_INT_MS_STS_5            0x00000020
#define PWM_INT_MS_STS_6            0x00000040
#define PWM_INT_MS_STS_7            0x00000080
#define PWM_INT_MS_STS_ALL          0x000000FF

#define PWM_INT_CCNT0_EDGE_STS      0x00000001
#define PWM_INT_CCNT0_TRIG_STS      0x00000002
#define PWM_INT_CCNT0_TOUT_STS      0x00000004

#define PWM_INT_CCNT1_EDGE_STS      0x00000001
#define PWM_INT_CCNT1_TRIG_STS      0x00000002
#define PWM_INT_CCNT1_TOUT_STS      0x00000004

#define PWM_INT_CCNT2_EDGE_STS      0x00000001
#define PWM_INT_CCNT2_TRIG_STS      0x00000002
#define PWM_INT_CCNT2_TOUT_STS      0x00000004

#define PWM_INT_CCNT_STS_ALL        0x00000777
#define PWM_INT_CLKDIV_STS_ALL      0x00030003


#define PWM_INVALID_ID              0x80000000


// bit map definition of PWM control & interrupt enable register
#define PWM_INTE_MASK               0x00000FFF
#define MS_INTE_MASK                0x000000FF
#define CCNT_INTE_MASK              0x00000777

//#define PWM_EN_MASK               0x8FFF0000
//#define PWM_STOP_MASK             0x00008FFF
#define PWM_EN_SHIFT                16
#define PWM_MS_START_MASK           0x000000FF
//#define PWM_PWMCH_BITS              12
#define PWM_ALLCH_BITS              12
#define PWM_PER_CH_OFFSET           8
#define PWM_DGPIO5                  5


#define PWM_CCNT0_BITS_START        0
#define PWM_CCNT1_BITS_START        4
#define PWM_CCNT2_BITS_START        8

#define PWM_CCNT_BITS_CNT           3

#define PWM_CCNT0_EDGE              0x00000001
#define PWM_CCNT0_TRIG              0x00000002
#define PWM_CCNT0_TOUT              0x00000004

#define PWM_CCNT1_EDGE              0x00000010
#define PWM_CCNT1_TRIG              0x00000020
#define PWM_CCNT1_TOUT              0x00000040

#define PWM_CCNT2_EDGE              0x00000100
#define PWM_CCNT2_TRIG              0x00000200
#define PWM_CCNT2_TOUT              0x00000400

#endif


