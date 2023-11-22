/*
 * mhal_pwm.c- Sigmastar
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
#include "mhal_pwm.h"
#include "gpio.h"
#include <linux/of_irq.h>
#include <linux/jiffies.h>

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------
#define BASE_REG_NULL 0xFFFFFFFF

typedef struct
{
    u32 u32Adr;
    u32 u32Val;
    u32 u32Msk;
} regSet_t;

typedef struct
{
    u32      u32PadId;
    regSet_t regSet[2];
} pwmPadTbl_t;

static U8   _pwmEnSatus[PWM_NUM]                     = {0};
static U32  _pwmPeriod[PWM_NUM]                      = {0};
static U8   _pwmPolarity[PWM_NUM]                    = {0};
static U32  _pwmDuty[PWM_NUM][PWM_SHIFT_ARG_MAX_NUM] = {{0}}; // end  ( hardware support 0~4 set of duty )
static U8   _pwmDutyArgNum[PWM_NUM]                  = {0};
static U32  _pwmShft[PWM_NUM][PWM_SHIFT_ARG_MAX_NUM] = {{0}}; // begin ( hardware support 0~4 set of shift )
static U8   _pwmShftArgNum[PWM_NUM]                  = {0};
static U32  _pwmFreq[PWM_NUM]                        = {0};
static bool isSync                                   = 1; // isSync=0 --> need to sync register data from mem
#ifdef CONFIG_PWM_NEW
static U32 _pwmPeriod_ns[PWM_NUM] = {0};
static U32 _pwmDuty_ns[PWM_NUM]   = {0};
#endif
static U32 _pwmDiv[PWM_NUM] = {0};
static U16 clk_pwm_div[7]   = {1, 2, 4, 8, 32, 64, 128};

static U64 _pwmTimeStop                 = 0;
static U64 _pwmTimeStart[PWM_GROUP_NUM] = {0};
static U32 _pwmTotalRounds[PWM_NUM]     = {0};

/**
 * MHal_PWMGetGrpAddr - get the bank addr and offset addr of different PWM channel
 * @sstar_chip: struct sstar_pwm_chip
 * @u32addr: bank address
 * @u32PwmOffs: offset address
 * @u8Id: PWM channel index
 *
 * Returns None.
 */

static void MHal_PWMGetGrpAddr(struct sstar_pwm_chip *sstar_chip, U32 *u32addr, U32 *u32PwmOffs, U8 u8Id)
{
    if (u8Id >= PWM_NUM)
        return;

    *u32addr    = (U32)sstar_chip->base;
    *u32PwmOffs = (u8Id < 4) ? (u8Id * 0x80) : ((4 * 0x80) + ((u8Id - 4) * 0x40));
}

/**
 * MHal_PWM_MemToReg - set the memery data to reg address
 * @sstar_chip: struct sstar_pwm_chip
 * @u8GrpId: PWM group index
 *
 * Returns None.
 */

void MHal_PWM_MemToReg(struct sstar_pwm_chip *sstar_chip, U8 u8GrpId)
{
    U8 pwmId, idx;
    for (idx = 0; idx < PWM_PER_GROUP; idx++)
    {
        pwmId = (u8GrpId * PWM_PER_GROUP) + idx; // idx+(groupid*PWM_PER_GROUP)
        if (pwmId < PWM_NUM)
        {
            MHal_PWMDiv(sstar_chip, pwmId, _pwmDiv[pwmId]);
            MHal_PWMSetPolarityExToReg(sstar_chip, pwmId, _pwmPolarity[pwmId]);
            MHal_PWMSetPeriodExToReg(sstar_chip, pwmId, _pwmPeriod[pwmId]);
            MHal_PWMSetBeginToReg(sstar_chip, pwmId, _pwmShftArgNum[pwmId]);
            MHal_PWMSetEndToReg(sstar_chip, pwmId, _pwmDutyArgNum[pwmId]);
        }
    }
    SSTAR_PWM_DBG("\r\n[PWM] %s Sync:%d\r\n", __func__, isSync);
}

/**
 * MHal_PWM_SetSyncFlag - set the sync flag or not
 * @bSync: 1: set the sync flag, 0: not
 *
 * Returns None.
 */

void MHal_PWM_SetSyncFlag(bool bSync)
{
    isSync = bSync;
    SSTAR_PWM_DBG("[PWM] %s Sync:%d\r\n", __func__, isSync);
}

/**
 * MHal_PWM_GetSyncFlag - get the sync flag status
 * @void
 *
 * Returns 1: set the sync flag, 0: not
 */

bool MHal_PWM_GetSyncFlag(void)
{
    return isSync;
}

/**
 * MHal_PWM_SetClock - enable PWM clock
 * @sstar_chip: struct sstar_pwm_chip
 *
 * Returns None
 */

void MHal_PWM_SetClock(struct sstar_pwm_chip *sstar_chip)
{
    clk_prepare_enable(sstar_chip->clk);
}

/**
 * MHal_PWMInit - PWM channel initialization
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel index
 *
 * Returns None
 */

void MHal_PWMInit(struct sstar_pwm_chip *sstar_chip, U8 u8Id)
{
    U32 reset, u32Reg = 0, u32Period = 0, U32PwmAddr = 0, u32PwmOffs = 0;

    if (PWM_NUM <= u8Id)
        return;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    reset = INREG16(U32PwmAddr + u16REG_SW_RESET) & (BIT0 << u8Id);

#ifdef CONFIG_PWM_NEW
    MHal_PWMGetConfig(sstar_chip, u8Id, NULL, &u32Period);
#else
    MHal_PWMGetPeriod(sstar_chip, u8Id, &u32Period);
#endif

    u32Reg = INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_PERIOD_L)
             | ((INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_PERIOD_H) & 0x3) << 16);

    if (u32Reg == 0x1)
        MHal_PWMDutyQE0(sstar_chip, 0);
    else
        MHal_PWMDutyQE0(sstar_chip, 1);

    if ((0 == reset) && (u32Period))
    {
        _pwmEnSatus[u8Id] = 1;
    }
    else
    {
        MHal_PWMEnable(sstar_chip, u8Id, 0);
    }
}

#ifdef CONFIG_PWM_NEW

/**
 * MHal_PWMSetConfig - set PWM period & duty & div
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel index
 * @duty: PWM duty, in nanosecond
 * @period: PWM period, in nanosecond
 *
 * The duty in percentage is @duty / @period
 *
 * Returns None
 */

void MHal_PWMSetConfig(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 duty, U32 period)
{
    U8  i;
    U16 u16Div     = 0;
    U32 common     = 0;
    U32 pwmclk     = 0;
    U32 gcd        = 0;
    U32 divisor    = 0;
    U32 dividend   = 0;
    U32 periodmax  = 0;
    U32 u32Period  = 0x00000000;
    U32 u32Duty    = 0x00000000;
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    if (u8Id >= PWM_NUM)
        return;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);

    pwmclk = sstar_chip->clk_freq;

    divisor  = 1000000000;
    dividend = pwmclk;
    while ((gcd = dividend % divisor))
    {
        dividend = divisor;
        divisor  = gcd;
    }
    gcd = divisor;
    pwmclk /= gcd;
    common = 1000000000 / gcd;

    /*      select   div       */
    for (i = 0; i < (sizeof(clk_pwm_div) / sizeof(U16)); i++)
    {
        periodmax = (clk_pwm_div[i] * 262144 / pwmclk) * common;
        if (period < periodmax)
        {
            u16Div = clk_pwm_div[i];
            break;
        }
    }

    /*      select   period       */
    if (period < (0xFFFFFFFF / pwmclk))
    {
        u32Period = (pwmclk * period) / (u16Div * common);
        if (((pwmclk * period) % (u16Div * common)) > (u16Div * common / 2))
        {
            u32Period++;
        }
        _pwmPeriod_ns[u8Id] = (u32Period * u16Div * common) / pwmclk;
    }
    else
    {
        u32Period = (period / u16Div) * pwmclk / common;
        u32Period++;
        _pwmPeriod_ns[u8Id] = (u32Period * common / pwmclk) * u16Div;
    }

    /*      select   duty       */
    if (duty == 0)
    {
        if (_pwmEnSatus[u8Id])
        {
            SETREG16(U32PwmAddr + u16REG_SW_RESET, BIT0 << u8Id);
        }
    }
    else
    {
        if (_pwmEnSatus[u8Id])
        {
            CLRREG16(U32PwmAddr + u16REG_SW_RESET, BIT0 << u8Id);
        }
    }

    if (duty < (0xFFFFFFFF / pwmclk))
    {
        u32Duty = (pwmclk * duty) / (u16Div * common);
        if ((((pwmclk * duty) % (u16Div * common)) > (u16Div * common / 2)) || (u32Duty == 0))
        {
            u32Duty++;
        }
        _pwmDuty_ns[u8Id] = (u32Duty * u16Div * common) / pwmclk;
    }
    else
    {
        u32Duty = (duty / u16Div) * pwmclk / common;
        u32Duty++;
        _pwmPeriod_ns[u8Id] = (u32Duty * common / pwmclk) * u16Div;
    }

    /*      set  div period duty       */
    u16Div--;
    u32Period--;
    u32Duty--;
    SSTAR_PWM_INFO("clk=%d, u16Div=%d u32Duty=0x%x u32Period=0x%x\n", sstar_chip->clk_freq, u16Div, u32Duty, u32Period);
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DIV, (u16Div & 0xFFFF));
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_PERIOD_L, (u32Period & 0xFFFF));
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_PERIOD_H, ((u32Period >> 16) & 0x3));
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_L, (u32Duty & 0xFFFF));
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_H, ((u32Duty >> 16) & 0x3));
    _pwmDiv[u8Id] = u16Div;
}

/**
 * MHal_PWMGetConfig - get PWM period & duty
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel index
 * @duty: PWM duty, in nanosecond
 * @period: PWM period, in nanosecond
 *
 * Returns None
 */

void MHal_PWMGetConfig(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 *Duty, U32 *Period)
{
    U16 u16Div     = 0;
    U32 u32Duty    = 0;
    U32 u32Period  = 0;
    U32 U32PwmAddr = 0;
    U32 pwmclk     = 0;
    U32 common     = 0;
    U32 gcd        = 0;
    U32 divisor    = 0;
    U32 dividend   = 0;
    U32 u32PwmOffs = 0;
    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);

    if (u8Id >= PWM_NUM)
        return;

    pwmclk = sstar_chip->clk_freq;

    divisor  = 1000000000;
    dividend = pwmclk;
    while ((gcd = dividend % divisor))
    {
        dividend = divisor;
        divisor  = gcd;
    }
    gcd = divisor;
    pwmclk /= gcd;
    common = 1000000000 / gcd;

    u16Div = INREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DIV);
    u16Div++;

    if (Period != NULL)
    {
        if (_pwmPeriod_ns[u8Id] == 0)
        {
            u32Period = INREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_PERIOD_L)
                        | ((INREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_PERIOD_H) & 0x3) << 16);
            if (u32Period)
            {
                u32Period++;
            }
            _pwmPeriod_ns[u8Id] = (u32Period * u16Div * common) / pwmclk;
        }
        *Period = _pwmPeriod_ns[u8Id];
    }

    if (Duty != NULL)
    {
        if (_pwmDuty_ns[u8Id] == 0)
        {
            u32Duty = INREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_L)
                      | ((INREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_H) & 0x3) << 16);
            if (u32Duty)
            {
                u32Duty++;
            }
            _pwmDuty_ns[u8Id] = (u32Duty * u16Div * common) / pwmclk;
        }
        *Duty = _pwmDuty_ns[u8Id];
    }
}

#else

/**
 * MHal_PWMSetDuty - set PWM duty
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel index
 * @u32Val: PWM duty, in percentage
 *
 * Returns None
 */

void MHal_PWMSetDuty(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Val)
{
    U32 u32Period = 0x00000000;
    U32 u32Duty = 0x00000000;
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;
    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);

    if (PWM_NUM <= u8Id)
        return;

    // MHal_PWM_SetClock(sstar_chip);
    if (_pwmEnSatus[u8Id])
    {
        if (0 == u32Val)
            OUTREGMSK16(U32PwmAddr + u16REG_SW_RESET, (BIT0 << u8Id), BIT0 << u8Id);
    }
    if (u32Val == 0)
    {
        u32Duty = 0;
    }
    else
    {
        if (u32Val > 100)
        {
            u32Val = 100;
        }
        if (_pwmPeriod[u8Id] == 0)
        {
            SSTAR_PWM_ERR("pwm period need to be set first\n");
            return;
        }
        u32Period = _pwmPeriod[u8Id];
        u32Period++;
        u32Duty = ((u32Period * u32Val) / 100);
        if (u32Duty)
        {
            u32Duty--;
        }
    }
    SSTAR_PWM_INFO("clk=%d, u32Duty=x%x\n", sstar_chip->clk_freq, u32Duty);
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_L, (u32Duty & 0xFFFF));
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_H, ((u32Duty >> 16) & 0x3));

    if (_pwmEnSatus[u8Id])
    {
        U32 reset = INREG16(U32PwmAddr + u16REG_SW_RESET) & (BIT0 << u8Id);
        if (u32Val && reset)
        {
            CLRREG16(U32PwmAddr + u16REG_SW_RESET, BIT0 << u8Id);
        }
    }
}

/**
 * MHal_PWMGetDuty - get PWM duty
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel index
 * @pu32Val: PWM duty, in percentage
 *
 * Returns None
 */

void MHal_PWMGetDuty(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 *pu32Val)
{
    U32 u32Period, u32Duty, U32PwmAddr = 0;
    U32 u32PwmOffs = 0;
    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);

    *pu32Val = 0;
    if (PWM_NUM <= u8Id)
        return;
    u32Duty = INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DUTY_L)
              | ((INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DUTY_H) & 0x3) << 16);
    if (u32Duty)
    {
        u32Period = _pwmPeriod[u8Id];
        if (u32Period)
        {
            *pu32Val = ((u32Duty + 1) * 100) / (u32Period + 1);
        }
    }
}

/**
 * MHal_PWMSetPeriod - set PWM period
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel index
 * @u32Val: PWM period, in Hz
 *
 * Returns None
 */

void MHal_PWMSetPeriod(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Val)
{
    U8 i;
    U16 u16Div = 0;
    U32 pwmclk = 0;
    U32 periodmax = 0;
    U32 u32Period = 0;
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;
    U32 u32FrqHz = u32Val;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    // MHal_PWM_SetClock(sstar_chip);
    pwmclk = sstar_chip->clk_freq;
    /*      select   div       */
    for (i = 0; i < (sizeof(clk_pwm_div) / sizeof(U16)); i++)
    {
        periodmax = (pwmclk / (clk_pwm_div[i] * 262144));
        if (u32FrqHz > periodmax)
        {
            u16Div = clk_pwm_div[i];
            break;
        }
    }

    u32Period = ((pwmclk / u16Div) / u32FrqHz);

    //[APN] range 2<=Period<=262144
    if (u32Period < 2)
    {
        u32Period = 2;
    }
    else if (u32Period > 262144)
    {
        u32Period = 262144;
    }

    //[APN] PWM _PERIOD= (REG_PERIOD+1)
    u16Div--;
    u32Period--;

    SSTAR_PWM_INFO("clk=%d, u32Period=0x%x\n", pwmclk, u32Period);
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DIV, (u16Div & 0xFFFF));
    OUTREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_PERIOD_L, (u32Period & 0xFFFF));
    OUTREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_PERIOD_H, ((u32Period >> 16) & 0x3));

    _pwmDiv[u8Id] = u16Div;
    _pwmPeriod[u8Id] = u32Period;
}

/**
 * MHal_PWMGetPeriod - get PWM period
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel index
 * @pu32Val: PWM period, in Hz
 *
 * Returns None
 */

void MHal_PWMGetPeriod(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 *pu32Val)
{
    U16 u16Div = 0;
    U32 u32Period, u32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    MHal_PWMGetGrpAddr(sstar_chip, &u32PwmAddr, &u32PwmOffs, u8Id);

    u32Period = INREG16(u32PwmAddr + u32PwmOffs + u16REG_PWM_PERIOD_L)
                | ((INREG16(u32PwmAddr + u32PwmOffs + u16REG_PWM_PERIOD_H) & 0x3) << 16);
    u16Div = INREG16(u32PwmAddr + (u32PwmOffs) + u16REG_PWM_DIV);
    u16Div++;
    if ((0 == _pwmPeriod[u8Id]) && (u32Period))
    {
        _pwmPeriod[u8Id] = u32Period;
    }

    *pu32Val = 0;
    if (u32Period)
    {
        *pu32Val = sstar_chip->clk_freq / ((u32Period + 1) * u16Div);
    }
}
#endif

/**
 * MHal_PWMSetPolarity - set PWM period
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel index
 * @u8Val: PWM polarity, 0: normal, 1: inversed
 *
 * Returns None
 */

void MHal_PWMSetPolarity(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val)
{
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;
    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    OUTREGMSK16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_CTRL, (u8Val << POLARITY_BIT), (0x1 << POLARITY_BIT));
}

/**
 * MHal_PWMGetPolarity - get PWM period
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel index
 * @pu8Val: PWM polarity, 1: inversed, 0: normal
 *
 * Returns None
 */

void MHal_PWMGetPolarity(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 *pu8Val)
{
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;
    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    *pu8Val = (INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_CTRL) & (0x1 << POLARITY_BIT)) ? 1 : 0;
}

/**
 * MHal_PWMSetDben - set PWM Dben
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel index
 * @pu8Val: 1: on, 0: off,
 *
 * Returns None
 */

void MHal_PWMSetDben(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val)
{
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    OUTREGMSK16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_CTRL, (u8Val << DBEN_BIT), (0x1 << DBEN_BIT));
    OUTREGMSK16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_CTRL, (u8Val << VDBEN_SW_BIT), (0x1 << VDBEN_SW_BIT));
}

/**
 * MHal_PWMEnable - enable PWM to generate waveform
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel index
 * @u8Val: 1: enable, 0: disable
 *
 * Returns None
 */

void MHal_PWMEnable(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val)
{
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;
    U32 u32DutyL   = 0;
    U32 u32DutyH   = 0;

    if (PWM_NUM <= u8Id)
        return;
    MHal_PWMSetDben(sstar_chip, u8Id, 1);

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    u32DutyL = INREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_L);
    u32DutyH = INREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_H);

    if (u8Val)
    {
        if (u32DutyL || u32DutyH)
        {
            CLRREG16(U32PwmAddr + u16REG_SW_RESET, BIT0 << u8Id);
        }
        else
        {
            SETREG16(U32PwmAddr + u16REG_SW_RESET, BIT0 << u8Id);
        }
    }
    else
    {
        SETREG16(U32PwmAddr + u16REG_SW_RESET, BIT0 << u8Id);
    }
    _pwmEnSatus[u8Id] = u8Val;
}

/**
 * MHal_PWMEnableGet - get PWM enable status
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel index
 * @pu8Val: 1: enable, 0: disable
 *
 * Returns None
 */

void MHal_PWMEnableGet(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 *pu8Val)
{
    *pu8Val = 0;
    if (PWM_NUM <= u8Id)
        return;
    *pu8Val = _pwmEnSatus[u8Id];
}

/**
 * MHal_PWMPadSet - set PWM enable status
 *
 * Returns None
 */

void MHal_PWMPadSet(U8 u8Id, U8 u8Val) {}

/**
 * MHal_PWMPadSet - judge this chip is support motor interface or not
 * @void:
 *
 * Returns 1: support, 0: unsupport
 */

int MHal_PWMGroupCap(void)
{
    return (PWM_GROUP_NUM) ? 1 : 0;
}

/**
 * MHal_PWMGroupJoin - enable PWM channel sync mode
 * @sstar_chip: struct sstar_pwm_chip
 * @u8PWMId: PWM channel index
 * @u8Val: 1: enable, 0: disable
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMGroupJoin(struct sstar_pwm_chip *sstar_chip, U8 u8PWMId, U8 u8Val)
{
    if (PWM_NUM <= u8PWMId)
        return 0;
    if (u8Val)
    {
        SETREG16(sstar_chip->base + REG_GROUP_JOIN, (1 << (u8PWMId + REG_GROUP_JOIN_SHFT)));
    }
    else
    {
        //[interrupt function]
        // Each group bit0 must enable for interrupt function
        // please see sync mode description for detail
        // SW owner default need to enable h74 bit0, bit4, bit8
        if (!(u8PWMId % 4))
        {
            SSTAR_PWM_ERR("[%s][%d] Always enable BIT_%2d for sync mode!\r\n", __FUNCTION__, __LINE__, u8PWMId);
            return 0;
        }
        else
        {
            CLRREG16(sstar_chip->base + REG_GROUP_JOIN, (1 << (u8PWMId + REG_GROUP_JOIN_SHFT)));
        }
    }
    return 1;
}

/**
 * MHal_PWMGroupEnable - enable PWM group mode
 * @sstar_chip: struct sstar_pwm_chip
 * @u8GroupId: PWM group index
 * @u8Val: 1: enable, 0: disable
 *
 * Enable this API will set group_enable mode and clear reset mode
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMGroupEnable(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U8 u8Val)
{
    u32 i;
    U32 u32JoinMask;
    U32 u32GroupRound = 0;
    U64 u64TimeVal    = 0;

    if (PWM_GROUP_NUM <= u8GroupId)
        return 0;

    if (!MHal_PWM_GetSyncFlag())
    {
        MHal_PWM_MemToReg(sstar_chip, u8GroupId);
        SSTAR_PWM_DBG("MEM to REG done !\n");
    }

    u32JoinMask = ((u8GroupId == 2) ? 0x7 : 0xF) << ((u8GroupId << 2) + REG_GROUP_JOIN_SHFT);
    u32JoinMask &= INREG16(sstar_chip->base + REG_GROUP_JOIN) & u32JoinMask;
    u32JoinMask |= 1 << (u8GroupId + PWM_NUM);

    if (u8Val)
    {
        _pwmTimeStart[u8GroupId] = jiffies;
        SETREG16(sstar_chip->base + REG_GROUP_ENABLE, (1 << (u8GroupId + REG_GROUP_ENABLE_SHFT)));
        CLRREG16(sstar_chip->base + u16REG_SW_RESET, u32JoinMask);
        MHal_PWM_SetSyncFlag(1); // dont need to sync until new data in
    }
    else
    {
        CLRREG16(sstar_chip->base + REG_GROUP_ENABLE, (1 << (u8GroupId + REG_GROUP_ENABLE_SHFT)));
        SETREG16(sstar_chip->base + u16REG_SW_RESET, u32JoinMask);
        _pwmTimeStop = jiffies;
        for (i = u8GroupId * 4; i < (u8GroupId * 4 + 4); i++)
        {
            if (i >= PWM_NUM)
                break;
            u32GroupRound = INREG16(sstar_chip->base + ((u8GroupId * 0x80) + 0x40)) & 0xFFFF;
            u64TimeVal    = ((_pwmTimeStop - _pwmTimeStart[u8GroupId]) * _pwmFreq[i]) / HZ;
            u64TimeVal    = (u64TimeVal > u32GroupRound) ? u32GroupRound : u64TimeVal;
            if (u32GroupRound)
            {
                u64TimeVal += _pwmTotalRounds[i];
                _pwmTotalRounds[i] = u64TimeVal;
            }
        }
    }

    return 1;
}

/**
 * MHal_PWMGroupIsEnable - get PWM group mode enable status
 * @sstar_chip: struct sstar_pwm_chip
 * @u8GroupId: PWM group index
 * @pu8Val: 1: enable, 0: disable
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMGroupIsEnable(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U8 *pu8Val)
{
    *pu8Val = 0;
    if (PWM_GROUP_NUM <= u8GroupId)
        return 0;
    *pu8Val = (INREG16(sstar_chip->base + REG_GROUP_ENABLE) >> (u8GroupId + REG_GROUP_ENABLE_SHFT)) & 0x1;
    return 1;
}

/**
 * MHal_PWMGroupClearRoundNum - clear the round count in PWM group mode
 * @sstar_chip: struct sstar_pwm_chip
 * @buf_start: start of information buf
 * @buf_end: end of information buf
 *
 * Returns group round num information buf

 */

void MHal_PWMGroupClearRoundNum(U8 u8GroupId)
{
    int i = 0;

    if (PWM_GROUP_NUM <= u8GroupId)
        return;

    for (i = u8GroupId * 4; i < (u8GroupId * 4 + 4); i++)
    {
        if (i >= PWM_NUM)
            break;
        _pwmTotalRounds[i] = 0;
    }
    SSTAR_PWM_INFO("[%s][%d] Clear group%hhu round count!\r\n", __FUNCTION__, __LINE__, u8GroupId);
}

/**
 * MHal_PWMGroupGetRoundNum - get the round count in PWM group mode
 * @sstar_chip: struct sstar_pwm_chip
 * @buf_start: start of information buf
 * @buf_end: end of information buf
 *
 * Returns group round num information buf

 */

int MHal_PWMGroupGetRoundNum(struct sstar_pwm_chip *sstar_chip, char *buf_start, char *buf_end)
{
    int   i;
    char *str            = buf_start;
    char *end            = buf_end;
    U64   u64TimeVal     = 0;
    U32   u32GroupRound  = 0;
    U8    u8EnableStatus = 0;
    U8    u8StopValue    = 0;
    U8    u8StopSatus    = 0;

    _pwmTimeStop = jiffies;

    u8StopValue = INREG16((sstar_chip->base + REG_GROUP_STOP));
    for (i = 0; i < PWM_NUM; i++)
    {
        u32GroupRound = INREG16(sstar_chip->base + ((((i / PWM_PER_GROUP) * 0x80) + 0x40))) & 0xFFFF;
        u64TimeVal    = ((_pwmTimeStop - _pwmTimeStart[i / PWM_PER_GROUP]) * _pwmFreq[i]) / HZ;
        u64TimeVal    = (u64TimeVal > u32GroupRound) ? u32GroupRound : u64TimeVal;
        MHal_PWMGroupIsEnable(sstar_chip, i / PWM_PER_GROUP, &u8EnableStatus);
        u8StopSatus = (u8StopValue >> (i / PWM_PER_GROUP)) & 0x1;
        if (u8EnableStatus && !u8StopSatus)
        {
            u64TimeVal += _pwmTotalRounds[i];
        }
        else
        {
            u64TimeVal = _pwmTotalRounds[i];
        }
        str += scnprintf(str, end - str, "PWM%d\t Round = \t%lld\n", i, u64TimeVal);
    }

    return (str - buf_start);
}

/**
 * MHal_PWMGroupSetRound - set the round number in PWM group mode
 * @sstar_chip: struct sstar_pwm_chip
 * @u8GroupId: PWM group index
 * @u16Val: round number
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMGroupSetRound(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U16 u16Val)
{
    U32 u32Reg;

    if (PWM_GROUP_NUM <= u8GroupId)
        return 0;
    u32Reg = (u8GroupId << 0x7) + 0x40;

    if (!INREG16(sstar_chip->base + u32Reg) || u16Val == 0)
    {
        OUTREG16(sstar_chip->base + u32Reg, u16Val);
        _pwmTimeStart[u8GroupId] = jiffies;
        SSTAR_PWM_INFO("[%s L%d] GrpId:%hhu round set %hu!\n", __FUNCTION__, __LINE__, u8GroupId, u16Val);
    }
    else
    {
        SSTAR_PWM_INFO("[%s L%d] GrpId:%hhu round is busy!\n", __FUNCTION__, __LINE__, u8GroupId);
    }
    return 1;
}

/**
 * MHal_PWMGroupStop - enable the stop function in PWM group mode
 * @sstar_chip: struct sstar_pwm_chip
 * @u8GroupId: PWM group index
 * @u8Val: 1: enable, 0: disable
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMGroupStop(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U8 u8Val)
{
    int i;
    U64 u64TimeVal    = 0;
    U32 u32GroupRound = 0;

    if (PWM_GROUP_NUM <= u8GroupId)
        return 0;

    if (u8Val)
    {
        SETREG16(sstar_chip->base + REG_GROUP_STOP, (1 << (u8GroupId + REG_GROUP_STOP_SHFT)));
        _pwmTimeStop = jiffies;
        for (i = u8GroupId * 4; i < (u8GroupId * 4 + 4); i++)
        {
            if (i >= PWM_NUM)
                break;
            u32GroupRound = INREG16(sstar_chip->base + ((u8GroupId * 0x80) + 0x40)) & 0xFFFF;
            u64TimeVal    = ((_pwmTimeStop - _pwmTimeStart[u8GroupId]) * _pwmFreq[i]) / HZ;
            u64TimeVal    = (u64TimeVal > u32GroupRound) ? u32GroupRound : u64TimeVal;
            if (u32GroupRound)
            {
                u64TimeVal += _pwmTotalRounds[i];
                _pwmTotalRounds[i] = u64TimeVal;
            }
        }
    }
    else
    {
        CLRREG16(sstar_chip->base + REG_GROUP_STOP, (1 << (u8GroupId + REG_GROUP_STOP_SHFT)));
    }

    return 1;
}

/**
 * MHal_PWMGroupHold - enable the hold function in PWM group mode
 * @sstar_chip: struct sstar_pwm_chip
 * @u8GroupId: PWM group index
 * @u8Val: 1: enable, 0: disable
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMGroupHold(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U8 u8Val)
{
    if (PWM_GROUP_NUM <= u8GroupId)
        return 0;

    if (u8Val)
        SETREG16(sstar_chip->base + REG_GROUP_HOLD, (1 << (u8GroupId + REG_GROUP_HOLD_SHFT)));
    else
        CLRREG16(sstar_chip->base + REG_GROUP_HOLD, (1 << (u8GroupId + REG_GROUP_HOLD_SHFT)));

    return 1;
}

/**
 * MHal_PWMGroupGetHoldM1 - get the enable status of hold mdoe_1
 * function in PWM group mode
 * @sstar_chip: struct sstar_pwm_chip
 *
 * Returns 1: enable, 0: disable
 */

int MHal_PWMGroupGetHoldM1(struct sstar_pwm_chip *sstar_chip)
{
    return INREG16(sstar_chip->base + REG_GROUP_HOLD_MODE1);
}

/**
 * MHal_PWMGroupHoldM1 - set the hold mdoe_1 function in PWM group mode
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Val: 1: enable hold mode_1, 0: disable hold mode_1
 *
 * Returns 1
 */

int MHal_PWMGroupHoldM1(struct sstar_pwm_chip *sstar_chip, U8 u8Val)
{
    if (u8Val)
    {
        SETREG16(sstar_chip->base + REG_GROUP_HOLD_MODE1, 1);
        SSTAR_PWM_INFO("[%s L%d] hold mode1 en!(keep low)\n", __FUNCTION__, __LINE__);
    }
    else
    {
        CLRREG16(sstar_chip->base + REG_GROUP_HOLD_MODE1, 0);
        SSTAR_PWM_INFO("[%s L%d] hold mode1 dis!\n", __FUNCTION__, __LINE__);
    }
    return 1;
}

/**
 * MHal_PWMDutyQE0 - enable the duty_qe0 function
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Val: 1: enable, 0: disable
 *
 * Returns None
 */

void MHal_PWMDutyQE0(struct sstar_pwm_chip *sstar_chip, U8 u8Val)
{
    if (u8Val)
        SETREG16(sstar_chip->base + REG_PWM_DUTY_QE0, (1 << REG_PWM_DUTY_QE0_SHFT));
    else
        CLRREG16(sstar_chip->base + REG_PWM_DUTY_QE0, (1 << REG_PWM_DUTY_QE0_SHFT));
}

/**
 * MHal_PWMGetOutput - get the status of output
 * @sstar_chip: struct sstar_pwm_chip
 * @pu8Output: output status
 *
 * Returns 1
 */

int MHal_PWMGetOutput(struct sstar_pwm_chip *sstar_chip, U8 *pu8Output)
{
    *pu8Output = INREG16(sstar_chip->base + REG_PWM_OUT);
    SSTAR_PWM_INFO("[%s L%d] output:x%x\n", __FUNCTION__, __LINE__, *pu8Output);
    return 1;
}

#ifdef CONFIG_PWM_NEW

/**
 * MHal_PWMSetEnd - set the end of a high level within a period
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel
 * @u8DutyId: the index in _pwmDuty[] array
 * @u32Val: PWM duty in nanosecond
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMSetEnd(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8DutyId, U32 u32Val)
{
    U16 u16Div   = 0;
    U32 common   = 0;
    U32 pwmclk   = 0;
    U32 gcd      = 0;
    U32 divisor  = 0;
    U32 dividend = 0;
    U32 duty     = u32Val;
    U32 u32Duty  = 0;
    if (u8Id >= PWM_NUM)
        return 0;
    pwmclk   = sstar_chip->clk_freq;
    divisor  = 1000000000;
    dividend = pwmclk;
    while ((gcd = dividend % divisor))
    {
        dividend = divisor;
        divisor  = gcd;
    }
    gcd = divisor;
    pwmclk /= gcd;
    common = 1000000000 / gcd;
    u16Div = (_pwmDiv[u8Id] + 1);
    if (duty < (0xFFFFFFFF / pwmclk))
    {
        u32Duty = (pwmclk * duty) / (u16Div * common);
        if ((((pwmclk * duty) % (u16Div * common)) > (u16Div * common / 2)) || (u32Duty == 0))
        {
            u32Duty++;
        }
        _pwmDuty_ns[u8Id] = (u32Duty * u16Div * common) / pwmclk;
    }
    else
    {
        u32Duty = (duty / u16Div) * pwmclk / common;
        u32Duty++;
        _pwmDuty_ns[u8Id] = (u32Duty * common / pwmclk) * u16Div;
    }
    u32Duty--;
    _pwmDutyArgNum[u8Id]     = u8DutyId + 1;
    _pwmDuty[u8Id][u8DutyId] = u32Duty;
    return 1;
}
#else

/**
 * MHal_PWMSetEnd - set the end of a high level within a period
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel
 * @u8DutyId: the index in _pwmDuty[] array
 * @u32Val: PWM duty in thousandths
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMSetEnd(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8DutyId, U32 u32Val)
{
    U32 u32Period;
    U32 u32Duty;

    if (PWM_NUM <= u8Id)
        return 0;

    if (_pwmPeriod[u8Id] == 0)
    {
        SSTAR_PWM_ERR("pwm period need to be set first\n");
        return 0;
    }

    u32Period = _pwmPeriod[u8Id];
    u32Period++;
    u32Duty = ((u32Period * u32Val) / 1000);
    if (u32Duty)
    {
        u32Duty--;
    }
    _pwmDutyArgNum[u8Id] = u8DutyId + 1;
    _pwmDuty[u8Id][u8DutyId] = u32Duty;
    MHal_PWM_SetSyncFlag(0);

    if (u32Duty & 0xFFFC0000)
    {
        SSTAR_PWM_ERR("[%s][%d] too large duty 0x%08x (18 bits in max)\n", __FUNCTION__, __LINE__, u32Duty);
    }
    return 1;
}
#endif
/**
 * MHal_PWMSetEndToReg - set the end of PWM duty information to reg address
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel
 * @u8DutyArgNum: the number of PWM duty arg
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMSetEndToReg(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8DutyArgNum)
{
    U8  arg_no;
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);

    for (arg_no = 0; arg_no < u8DutyArgNum; arg_no++)
    {
        if (0 == arg_no)
        {
            OUTREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DUTY_L, (_pwmDuty[u8Id][arg_no] & 0xFFFF));
            OUTREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DUTY_H, ((_pwmDuty[u8Id][arg_no] >> 16) & 0x0003));
        }
        else
        {
            OUTREG16(U32PwmAddr + u32PwmOffs + (arg_no << 3) + 28, (_pwmDuty[u8Id][arg_no] & 0xFFFF));
        }
    }
    return 1;
}

#ifdef CONFIG_PWM_NEW

/**
 * MHal_PWMSetBegin - set the begin of a high level within a period
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel
 * @u8ShftId: the index in _pwmDuty[] array
 * @u32Val: PWM duty in nanosecond
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMSetBegin(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8ShftId, U32 u32Val)
{
    U16 u16Div   = 0;
    U32 common   = 0;
    U32 pwmclk   = 0;
    U32 gcd      = 0;
    U32 divisor  = 0;
    U32 dividend = 0;
    U32 shift    = u32Val;
    U32 u32Shift = 0;
    if (u8Id >= PWM_NUM)
        return 0;
    pwmclk   = sstar_chip->clk_freq;
    divisor  = 1000000000;
    dividend = pwmclk;
    while ((gcd = dividend % divisor))
    {
        dividend = divisor;
        divisor  = gcd;
    }
    gcd = divisor;
    pwmclk /= gcd;
    common = 1000000000 / gcd;
    u16Div = (_pwmDiv[u8Id] + 1);
    if (shift < (0xFFFFFFFF / pwmclk))
    {
        u32Shift = (pwmclk * shift) / (u16Div * common);
        if ((((pwmclk * shift) % (u16Div * common)) > (u16Div * common / 2)) || (u32Shift == 0))
        {
            u32Shift++;
        }
    }
    else
    {
        u32Shift = (shift / u16Div) * pwmclk / common;
        u32Shift++;
    }
    u32Shift--;
    _pwmShftArgNum[u8Id]     = u8ShftId + 1;
    _pwmShft[u8Id][u8ShftId] = u32Shift;
    MHal_PWM_SetSyncFlag(0);
    return 1;
}
#else

/**
 * MHal_PWMSetBegin - set the begin of a high level within a period
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel
 * @u8ShftId: the index in _pwmDuty[] array
 * @u32Val: PWM duty in thousandths
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMSetBegin(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8ShftId, U32 u32Val)
{
    U32 u32Period;
    U32 u32Shft;

    if (PWM_NUM <= u8Id)
        return 0;

    if (_pwmPeriod[u8Id] == 0)
    {
        SSTAR_PWM_ERR("pwm period need to be set first\n");
        return 0;
    }

    u32Period = _pwmPeriod[u8Id];
    u32Period++;
    u32Shft = ((u32Period * u32Val) / 1000);
    if (u32Shft)
    {
        u32Shft--;
    }
    _pwmShftArgNum[u8Id] = u8ShftId + 1;
    _pwmShft[u8Id][u8ShftId] = u32Shft;
    MHal_PWM_SetSyncFlag(0);

    if (u32Shft & 0xFFFC0000)
    {
        SSTAR_PWM_ERR("[%s][%d] too large shift 0x%08x (18 bits in max)\n", __FUNCTION__, __LINE__, u32Shft);
    }

    return 1;
}
#endif
/**
 * MHal_PWMSetBeginToReg - set the begin of PWM duty information to reg address
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel
 * @u8ShftArgNum: the number of PWM duty arg
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMSetBeginToReg(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8ShftArgNum)
{
    U8  arg_no;
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);

    for (arg_no = 0; arg_no < u8ShftArgNum; arg_no++)
    {
        if (0 == arg_no)
        {
            OUTREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_SHIFT_L, (_pwmShft[u8Id][arg_no] & 0xFFFF));
            OUTREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_SHIFT_H, ((_pwmShft[u8Id][arg_no] >> 16) & 0x0003));
        }
        else
        {
            OUTREG16(U32PwmAddr + u32PwmOffs + (arg_no << 3) + 24, (_pwmShft[u8Id][arg_no] & 0xFFFF));
        }
    }
    return 1;
}

/**
 * MHal_PWMSetPolarityEx - save the PWM polarity information to _pwmPolarity[] array
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel
 * @u8Val: 1: enable, 0: disable
 *
 * Returns 1
 */

int MHal_PWMSetPolarityEx(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val)
{
    if (PWM_NUM <= u8Id)
        return 0;
    _pwmPolarity[u8Id] = u8Val;
    MHal_PWM_SetSyncFlag(0);
    return 1;
}

/**
 * MHal_PWMSetPolarityExToReg - enable the PWM polarity by reg address
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel
 * @u8Val: 1: enable, 0: disable
 *
 * Returns 1
 */

int MHal_PWMSetPolarityExToReg(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val)
{
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    if (u8Val)
        SETREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_CTRL, (0x1 << POLARITY_BIT));
    else
        CLRREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_CTRL, (0x1 << POLARITY_BIT));
    return 1;
}

#ifdef CONFIG_PWM_NEW

/**
 * MHal_PWMSetPeriodEx - save the PWM period information to _pwmPeriod[] array
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel
 * @u32Val: PWM period in nanosecond
 *
 * Returns None
 */

void MHal_PWMSetPeriodEx(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Val)
{
    U8  i;
    U16 u16Div    = 0;
    U32 common    = 0;
    U32 pwmclk    = 0;
    U32 gcd       = 0;
    U32 divisor   = 0;
    U32 dividend  = 0;
    U32 periodmax = 0;
    U32 u32Period = 0x00000000;
    U32 period    = u32Val;
    if (u8Id >= PWM_NUM)
        return;
    pwmclk   = sstar_chip->clk_freq;
    divisor  = 1000000000;
    dividend = pwmclk;
    while ((gcd = dividend % divisor))
    {
        dividend = divisor;
        divisor  = gcd;
    }
    gcd = divisor;
    pwmclk /= gcd;
    common = 1000000000 / gcd;

    /*      select   div       */
    for (i = 0; i < (sizeof(clk_pwm_div) / sizeof(U16)); i++)
    {
        periodmax = (clk_pwm_div[i] * 262144 / pwmclk) * common;
        if (period < periodmax)
        {
            u16Div = clk_pwm_div[i];
            break;
        }
    }

    /*      select   period       */
    if (period < (0xFFFFFFFF / pwmclk))
    {
        u32Period = (pwmclk * period) / (u16Div * common);
        if (((pwmclk * period) % (u16Div * common)) > (u16Div * common / 2))
        {
            u32Period++;
        }
        _pwmPeriod_ns[u8Id] = (u32Period * u16Div * common) / pwmclk;
    }
    else
    {
        u32Period = (period / u16Div) * pwmclk / common;
        u32Period++;
        _pwmPeriod_ns[u8Id] = (u32Period * common / pwmclk) * u16Div;
    }
    u16Div--;
    u32Period--;
    _pwmDiv[u8Id]    = u16Div;
    _pwmPeriod[u8Id] = u32Period;
    _pwmFreq[u8Id]   = 1000000000 / u32Val;
    MHal_PWM_SetSyncFlag(0);
    pr_err("reg=0x%08X clk=%d, period=0x%x\n",
           (U32)(sstar_chip->base + ((u8Id < 4) ? (u8Id * 0x80) : (u8Id * 0x40 + 4 * 0x40)) + u16REG_PWM_PERIOD_L),
           sstar_chip->clk_freq, u32Period);
}

#else

/**
 * MHal_PWMSetPeriodEx - save the PWM period information to _pwmPeriod[] array
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel
 * @u32Val: PWM period in Hz
 *
 * Returns None
 */

void MHal_PWMSetPeriodEx(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Val)
{
    U8 i;
    U16 u16Div = 0;
    U32 pwmclk = 0;
    U32 periodmax = 0;
    U32 u32Period = 0;
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;
    U32 u32FrqHz = u32Val;

    if (u8Id >= PWM_NUM)
        return;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);

    pwmclk = sstar_chip->clk_freq;

    /*      select   div       */
    for (i = 0; i < (sizeof(clk_pwm_div) / sizeof(U16)); i++)
    {
        periodmax = (pwmclk / (clk_pwm_div[i] * 262144));
        if (u32FrqHz > periodmax)
        {
            u16Div = clk_pwm_div[i];
            break;
        }
    }

    u32Period = ((pwmclk / u16Div) / u32FrqHz);

    //[APN] range 2<=Period<=262144
    if (u32Period < 2)
    {
        u32Period = 2;
    }
    else if (u32Period > 262144)
    {
        u32Period = 262144;
    }

    //[APN] PWM _PERIOD= (REG_PERIOD+1)
    u16Div--;
    u32Period--;
    _pwmDiv[u8Id] = u16Div;
    _pwmPeriod[u8Id] = u32Period;
    _pwmFreq[u8Id] = u32Val;
    MHal_PWM_SetSyncFlag(0);

    SSTAR_PWM_INFO("clk=%d, u32Period=0x%x\n", pwmclk, u32Period);
}
#endif

/**
 * MHal_PWMSetPeriodExToReg - enable the PWM period by reg address
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel
 * @u32Period: PWM period in reg mode
 *
 * Returns 1
 */

void MHal_PWMSetPeriodExToReg(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Period)
{
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    OUTREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_PERIOD_L, (u32Period & 0xFFFF));
    OUTREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_PERIOD_H, ((u32Period >> 16) & 0x3));
}

/**
 * MHal_PWMDiv - set the PWM frequency division number
 * @sstar_chip: struct sstar_pwm_chip
 * @u8Id: PWM channel
 * @u8Val: divide parameter
 *
 * Returns 1: success, 0: fail
 */

int MHal_PWMDiv(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val)
{
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    if (u8Id >= PWM_NUM)
        return 0;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    OUTREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DIV, u8Val);
    SSTAR_PWM_DBG("mhal MHal_PWMDiv done !\n");

    return 1;
}

/**
 * MHal_PWMGroupInfo - get the PWM config information in group mode
 * @sstar_chip: struct sstar_pwm_chip
 * @buf_start: start of information buf
 * @buf_end: end of information buf
 *
 * Returns group information buf
 */

int MHal_PWMGroupInfo(struct sstar_pwm_chip *sstar_chip, char *buf_start, char *buf_end)
{
    char *str = buf_start;
    char *end = buf_end;
    int   i;
    // U32 tmp;
    U32 u32Period, u32Polarity;
    U32 u32Shft0;
    U32 u32Duty0;
    U32 u32SyncStatus;
    U32 u32ResetStatus;
    U32 u32GroupEnable, u32GroupReset, u32GroupHold, u32GroupStop, u32GroupRound;
    U32 clk = sstar_chip->clk_freq;
    U32 u32Div;
    U32 u32PwmOffs;
    U32 U32PwmAddr;
    if (0 == MHal_PWMGroupCap())
    {
        str += scnprintf(str, end - str, "This chip does not support motor interface\n");
        return (str - buf_start);
    }

    str += scnprintf(str, end - str, "================================================\n");
    str += scnprintf(str, end - str, "HoldM1\t\t%d\n", INREG16(sstar_chip->base + REG_GROUP_HOLD_MODE1));
    for (i = 0; i < PWM_GROUP_NUM; i++)
    {
        U32 pwmIdx;
        U32 j;

        // group enable
        u32GroupEnable = (INREG16(sstar_chip->base + REG_GROUP_ENABLE) >> i) & 0x1;
        // group reset
        u32GroupReset = (INREG16(sstar_chip->base + u16REG_SW_RESET) >> (i + PWM_NUM)) & 0x1;
        // hold
        u32GroupHold = (INREG16(sstar_chip->base + REG_GROUP_HOLD) >> (i + REG_GROUP_HOLD_SHFT)) & 0x1;
        // stop
        u32GroupStop = (INREG16(sstar_chip->base + REG_GROUP_STOP) >> (i + REG_GROUP_STOP_SHFT)) & 0x1;
        // round
        u32GroupRound = INREG16(sstar_chip->base + ((i << 0x7) + 0x40)) & 0xFFFF;

        str += scnprintf(str, end - str, "Group %d\n", i);
        pwmIdx = (i << 2);
        str += scnprintf(str, end - str, "\tmember\t\t");
        for (j = pwmIdx; j < pwmIdx + 4; j++)
        {
            if (j < PWM_NUM)
            {
                str += scnprintf(str, end - str, "%d ", j);
            }
        }
        str += scnprintf(str, end - str, "\n");
        str += scnprintf(str, end - str, "\tenable status\t%d\n", u32GroupEnable);
        str += scnprintf(str, end - str, "\tReset status\t%d\n", u32GroupReset);
        str += scnprintf(str, end - str, "\tHold\t\t%d\n", u32GroupHold);
        str += scnprintf(str, end - str, "\tStop\t\t%d\n", u32GroupStop);
        str += scnprintf(str, end - str, "\tRound\t\t%d\n", u32GroupRound);
    }

    str += scnprintf(str, end - str, "================================================\n");

    for (i = 0; i < PWM_NUM; i++)
    {
        MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, i);

        // Polarity
        u32Polarity = (INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_CTRL) >> POLARITY_BIT) & 0x1;
        // Period
        u32Period = INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_PERIOD_L)
                    | ((INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_PERIOD_H) & 0x3) << 16);
        // Shift
        u32Shft0 = INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_SHIFT_L)
                   | ((INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_SHIFT_H) & 0x3) << 16);
        // Duty
        u32Duty0 = INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DUTY_L)
                   | ((INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DUTY_H) & 0x3) << 16);
        u32Duty0 -= u32Shft0;
        // sync mode status
        u32SyncStatus = (INREG16(U32PwmAddr + REG_GROUP_JOIN) >> (i + REG_GROUP_JOIN_SHFT)) & 0x1;
        // rest status
        u32ResetStatus = (INREG16(U32PwmAddr + u16REG_SW_RESET) >> i) & BIT0;
        u32Div         = INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DIV); // workaround

        // output to buffer
        str += scnprintf(str, end - str, "Pwm %d\n", i);
        str += scnprintf(str, end - str, "\tPad\t\t0x%08x\n", sstar_chip->pad_ctrl[i]);
        str += scnprintf(str, end - str, "\tSync status\t%d\n", u32SyncStatus);
        str += scnprintf(str, end - str, "\tReset status\t%d\n", u32ResetStatus);
        str += scnprintf(str, end - str, "\tPolarity\t%d\n", u32Polarity);

        u32Period++;
        u32Shft0++;
        u32Duty0++;
        u32Shft0  = (1000 * u32Shft0) / u32Period;
        u32Duty0  = (1000 * u32Duty0) / u32Period;
        u32Period = clk / u32Period / (u32Div + 1);

        str += scnprintf(str, end - str, "\tPeriod\t\t%d\n", u32Period);
        str += scnprintf(str, end - str, "\tBegin\t\t%d\n", u32Shft0);
        str += scnprintf(str, end - str, "\tEnd\t\t%d\n", u32Duty0);
    }
    return (str - buf_start);
}

/**
 * PWM_IRQ - PWM interrupt service function
 * @irq: interrupt number
 * @data: sstar_pwm_chip
 *
 * Returns None
 */

irqreturn_t PWM_IRQ(int irq, void *data)
{
#if 1
    volatile u16           u16_Events = 0x0000;
    volatile u16           gid        = 0x0000;
    struct sstar_pwm_chip *sstar_chip = (struct sstar_pwm_chip *)data;

    u16_Events = INREG16(sstar_chip->base + REG_GROUP_INT);
    if ((u16_Events & PWM_INT_GRP_MASK))
    {
        for (gid = 0; gid < PWM_GROUP_NUM; gid++)
        {
            // one time enable one bit
            if ((u16_Events >> REG_GROUP_HOLD_INT_SHFT) & (1 << gid))
            {
                if (!MHal_PWMGroupGetHoldM1(sstar_chip))
                { // isr fill parameter only for HoldM0
                    MHal_PWM_MemToReg(sstar_chip, gid);
                }
                MHal_PWMGroupHold(sstar_chip, gid, 0); // For clear hold event;
                MHal_PWM_SetSyncFlag(1);
                SSTAR_PWM_INFO("hold_int GrpId:%d(event:x%x)\n", gid, u16_Events);
            }
        }
    }
    if ((u16_Events >> REG_GROUP_RUND_INT_SHFT))
    {
        for (gid = 0; gid < PWM_GROUP_NUM; gid++) // PWM_PER_GROUP??
        {
            if ((u16_Events >> REG_GROUP_RUND_INT_SHFT) & (1 << gid))
            {
                if (MHal_PWM_GetSyncFlag())
                {                                              // no new data
                    MHal_PWMGroupEnable(sstar_chip, gid, 0);   // For clear round event;
                    MHal_PWMGroupSetRound(sstar_chip, gid, 0); // set round=0
                }
                else
                { // have new data
                    MHal_PWMGroupEnable(sstar_chip, gid, 0);
                    MHal_PWMGroupEnable(sstar_chip, gid, 1);
                }
                SSTAR_PWM_INFO("round_int GrpId:%d\n", gid);
            }
        }
    }
#endif
    return IRQ_HANDLED;
}
