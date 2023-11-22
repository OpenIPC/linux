/*
 * mhal_pwm.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef __PWM_H
#define __PWM_H

#include <linux/clk.h>
#include <linux/pwm.h>
#include "ms_platform.h"
#include "registers.h"
#include "mdrv_types.h"

#if 1
#define MS_PWM_INFO(x, args...) printk(x, ##args)
#define MS_PWM_DBG(x, args...)  printk(x, ##args)
#else
#define MS_PWM_INFO(x, args...) \
    {                           \
    }
#define MS_PWM_DBG(x, args...) \
    {                          \
    }
#endif

struct sstar_pwm_chip
{
    struct pwm_chip chip;
    struct clk *    clk;
    void __iomem *  base;
    u32 *           pad_ctrl;
    int             irq;
};

//------------------------------------------------------------------------------
//  Constants
//------------------------------------------------------------------------------
#define PWM_GROUP_NUM (3)
#define PWM_PER_GROUP (4)
#define PWM_NUM       (12)
// Common PWM registers
#define PWM_SHIFT_ARG_MAX_NUM (4)
#define u16REG_PWM_SHIFT_L    (0x0 << 2)
#define u16REG_PWM_SHIFT_H    (0x1 << 2)
#define u16REG_PWM_DUTY_L     (0x2 << 2)
#define u16REG_PWM_DUTY_H     (0x3 << 2)
#define u16REG_PWM_PERIOD_L   (0x4 << 2) // reg_pwm0_period
#define u16REG_PWM_PERIOD_H   (0x5 << 2)
#define u16REG_PWM_DIV        (0x6 << 2)
#define u16REG_PWM_CTRL       (0x7 << 2)
#define VDBEN_SW_BIT          0
#define DBEN_BIT              1
#define DIFF_P_EN_BIT         2
#define SHIFT_GAT_BIT         3
#define POLARITY_BIT          4

#define u16REG_PWM_SHIFT2 (0x8 << 2)
#define u16REG_PWM_DUTY2  (0x9 << 2)
#define u16REG_PWM_SHIFT3 (0xA << 2)
#define u16REG_PWM_DUTY3  (0xB << 2)
#define u16REG_PWM_SHIFT4 (0xC << 2)
#define u16REG_PWM_DUTY4  (0xD << 2)

#define REG_GROUP_HOLD      (0x71 << 2)
#define REG_GROUP_HOLD_SHFT (0x0)

#define REG_GROUP_STOP      (0x72 << 2)
#define REG_GROUP_STOP_SHFT (0x0)

#define REG_GROUP_ENABLE      (0x73 << 2)
#define REG_GROUP_ENABLE_SHFT (0x0)

#define REG_GROUP_JOIN      (0x74 << 2)
#define REG_GROUP_JOIN_SHFT (0x0)

//+++[Only4I6e]
#define REG_GROUP_INT           (0x75 << 2)
#define REG_GROUP_HOLD_INT_SHFT (0x0)
#define REG_GROUP_RUND_INT_SHFT (0x3)
#define PWM_INT_GRP0            (BIT0)
#define PWM_INT_GRP1            (BIT1)
#define PWM_INT_GRP2            (BIT2)
#define PWM_INT_GRP_MASK        (0x07) // GRP0~GRP2

#define REG_PWM_DUTY_QE0      (0x76 << 2)
#define REG_PWM_DUTY_QE0_SHFT (0x0)

#define REG_GROUP_HOLD_MODE1    (0x77 << 2)
#define REG_GROUP_HALD_MD1_SHFT (0x0)

#define REG_PWM_OUT      (0x7E << 2)
#define REG_PWM_OUT_SHFT (0x0)
//---[Only4I6e]

#define u16REG_SW_RESET (0x7F << 2)

// 86MHz related definitions
#define CLOCK_SRC_86MHZ   86000000
#define PMSLEEP_86MHz_VAL 0x5
#define PMSLEEP_86MHz_POS 12
#define DIGPM_86MHz_POS   5
//------------------------------------------------------------------------------
//  Export Functions
//------------------------------------------------------------------------------
void MHal_PWMInit(struct sstar_pwm_chip *sstar_chip, U8 u8Id);
#ifdef CONFIG_PWM_NEW
void MHal_PWMSetConfig(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 duty, U32 period);
void MHal_PWMGetConfig(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 *Duty, U32 *Period);
#else
void MHal_PWMSetPeriod(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Val);
void MHal_PWMGetPeriod(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 *pu32Val);
void MHal_PWMSetDuty(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Val);
void MHal_PWMGetDuty(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 *pu32Val);
#endif
void MHal_PWMEnable(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val);
void MHal_PWMEnableGet(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 *pu8Val);
void MHal_PWMSetPolarity(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val);
void MHal_PWMGetPolarity(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 *pu8Val);
void MHal_PWMPadSet(U8 u8Id, U8 u8Val);
void MHal_PWMSetDben(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val);

int  MHal_PWMSetBegin(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8DutyId, U32 u32Val);
int  MHal_PWMSetEnd(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8ShftId, U32 u32Val);
int  MHal_PWMSetPolarityEx(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val);
void MHal_PWMSetPeriodEx(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Val);
int  MHal_PWMDiv(struct sstar_pwm_chip *sstar_chip, U8 u8Id,
                 U8 u8Val); // workaround for group hold/round malfunctional when div is zero

int MHal_PWMGroupCap(void);
int MHal_PWMGroupJoin(struct sstar_pwm_chip *sstar_chip, U8 u8PWMId, U8 u8Val);
int MHal_PWMGroupEnable(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U8 u8Val);
int MHal_PWMGroupIsEnable(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U8 *pu8Val);
int MHal_PWMGroupGetRoundNum(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U16 *pu16Val);
int MHal_PWMGroupShowRoundNum(struct sstar_pwm_chip *sstar_chip, char *buf_start, char *buf_end);
int MHal_PWMGroupSetRound(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U16 u16Val);
int MHal_PWMGroupStop(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U8 u8Val);
int MHal_PWMGroupHold(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U8 u8Val);
int MHal_PWMGroupInfo(struct sstar_pwm_chip *sstar_chip, char *buf_start, char *buf_end);

int  MHal_PWMSetEndToReg(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8DutyId);
int  MHal_PWMSetBeginToReg(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8ShftId);
int  MHal_PWMSetPolarityExToReg(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val);
void MHal_PWMSetPeriodExToReg(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Period);

int MHal_PWMGroupGetHoldM1(struct sstar_pwm_chip *sstar_chip);
int MHal_PWMGroupHoldM1(struct sstar_pwm_chip *sstar_chip, U8 u8Val);
int MHal_PWMDutyQE0(struct sstar_pwm_chip *sstar_chip, U8 u8Val);
int MHal_PWMGetOutput(struct sstar_pwm_chip *sstar_chip, U8 *pu8Output);

//-----------------------------------------------------------------------------

#endif //__PWM_H
