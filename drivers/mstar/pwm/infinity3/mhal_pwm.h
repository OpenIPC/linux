//-----------------------------------------------------------------------------
//
// Copyright (c) 2008 MStar Semiconductor, Inc.  All rights reserved.
//
//-----------------------------------------------------------------------------
// FILE
//      columbus_pwm.h
//
// DESCRIPTION
//      Define the PWM controller registers
//
// HISTORY
//      2008.5.13       Bryan Fan      Initial Version
//      2008.6.12       Bryan Fan      Revised for Code Review
//      2008.6.27       Bryan Fan      Revised for new Code Review
//      2008.07.02      Bryan Fan      Revised for new Code Review
//
//-----------------------------------------------------------------------------

#ifndef __PWM_H
#define __PWM_H

#include <linux/clk.h>
#include <linux/pwm.h>
#include "ms_platform.h"
#include "registers.h"
#include "mdrv_types.h"

#define MS_PWM_INFO(x, args...)    printk(x, ##args)
#define MS_PWM_DBG(x, args...)     printk(x, ##args)

struct mstar_pwm_chip {
	struct pwm_chip chip;
	struct clk *clk;
	void __iomem *base;
    u32 *pad_ctrl;
};

//------------------------------------------------------------------------------
//  Constants
//------------------------------------------------------------------------------
//Common PWM registers
#define u16REG_PWM_SHIFT_L      (0x0 << 2)
#define u16REG_PWM_SHIFT_H      (0x1 << 2)
#define u16REG_PWM_DUTY_L       (0x2 << 2)
#define u16REG_PWM_DUTY_H       (0x3 << 2)
#define u16REG_PWM_PERIOD_L     (0x4 << 2) //reg_pwm0_period
#define u16REG_PWM_PERIOD_H     (0x5 << 2)
#define u16REG_PWM_DIV          (0x6 << 2)
#define u16REG_PWM_CTRL         (0x7 << 2)
    #define VDBEN_SW_BIT           0
    #define DBEN_BIT               1
    #define DIFF_P_EN_BIT          2
    #define SHIFT_GAT_BIT          3
    #define POLARITY_BIT           4

#define u16REG_PWM_SHIFT2       (0x8 << 2)
#define u16REG_PWM_DUTY2        (0x9 << 2)
#define u16REG_PWM_SHIFT3       (0xA << 2)
#define u16REG_PWM_DUTY3        (0xB << 2)
#define u16REG_PWM_SHIFT4       (0xC << 2)
#define u16REG_PWM_DUTY4        (0xD << 2)

#define u16REG_SW_RESET         (0x7F << 2)

//------------------------------------------------------------------------------
//  Export Functions
//------------------------------------------------------------------------------
//void DrvBoostInit(void);
//void DrvBoostReset(void);
//void DrvPWMInit(U8 u8Id);
//void DrvPWMReset(void);
//void DrvBacklightSet(U8 u8Level, U8 u8IsSave);
//U8 DrvBacklightGet(void);
//void DrvBacklightOn(void);
//void DrvBacklightOff(void);
//void DrvPWMSetEn(U8 u8Id, U8 u8Val);
void DrvPWMSetPeriod(struct mstar_pwm_chip *ms_chip, U8 u8Id, U32 u32Val);
void DrvPWMSetDuty(struct mstar_pwm_chip *ms_chip, U8 u8Id, U32 u32Val);
void DrvPWMEnable(struct mstar_pwm_chip *ms_chip, U8 u8Id, U8 u8Val);
void DrvPWMSetPolarity(struct mstar_pwm_chip *ms_chip, U8 u8Id, U8 u8Val);
void DrvPWMPadSet(U8 u8Id, U8 u8Val);

//-----------------------------------------------------------------------------



#endif  //__PWM_H
