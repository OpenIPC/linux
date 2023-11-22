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
static U32 _pwmDiv[PWM_NUM]       = {0};
static U32 _pwmPeriod_ns[PWM_NUM] = {0};
static U32 _pwmDuty_ns[PWM_NUM]   = {0};
static U16 clk_pwm_div[7]         = {1, 2, 4, 8, 32, 64, 128};
#endif

//------------------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  External Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Global Functions
//------------------------------------------------------------------------------
static void MHal_PWMGetGrpAddr(struct sstar_pwm_chip *sstar_chip, U32 *u32addr, U32 *u32PwmOffs, U8 u8Id)
{
    if (u8Id >= PWM_NUM)
        return;

    *u32addr    = (U32)sstar_chip->base;
    *u32PwmOffs = (u8Id < 4) ? (u8Id * 0x80) : ((4 * 0x80) + ((u8Id - 4) * 0x40));
}

void MHal__PWM_MemToReg(struct sstar_pwm_chip *sstar_chip, U8 u8GrpId)
{
    U8 pwmId, idx;
    for (idx = 0; idx < PWM_PER_GROUP; idx++)
    {
        pwmId = (u8GrpId * PWM_PER_GROUP) + idx; // idx+(groupid*4)
        if (pwmId < PWM_NUM)
        {
            MHal_PWMSetPolarityExToReg(sstar_chip, pwmId, _pwmPolarity[pwmId]);
            MHal_PWMSetPeriodExToReg(sstar_chip, pwmId, _pwmPeriod[pwmId]);
            MHal_PWMSetBeginToReg(sstar_chip, pwmId, _pwmShftArgNum[pwmId]);
            MHal_PWMSetEndToReg(sstar_chip, pwmId, _pwmDutyArgNum[pwmId]);
        }
    }
    // printk(KERN_ERR "\r\n[P2R]Sync:%d\r\n", isSync);
}

void MHal__PWM_SetSyncFlag(bool bSync)
{
    isSync = bSync;
    // printk(KERN_ERR "Sync:%d\r\n", isSync);
}

bool MHal__PWM_GetSyncFlag(void)
{
    return isSync;
}

void MHal__PWM_SetClock(struct sstar_pwm_chip *sstar_chip)
{
    clk_prepare_enable(sstar_chip->clk);
}

void MHal_PWMInit(struct sstar_pwm_chip *sstar_chip, U8 u8Id)
{
    U32 reset, u32Period = 0, U32PwmAddr = 0, u32PwmOffs = 0;

    if (PWM_NUM <= u8Id)
        return;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    reset = INREG16(U32PwmAddr + u16REG_SW_RESET) & (BIT0 << ((u8Id == 10) ? 0 : u8Id));

#ifdef CONFIG_PWM_NEW
    MHal_PWMGetConfig(sstar_chip, u8Id, NULL, &u32Period);
    MHal_PWMDutyQE0(sstar_chip, 0);
#else
    MHal_PWMGetPeriod(sstar_chip, u8Id, &u32Period);
#endif

    if ((0 == reset) && (u32Period))
    {
        _pwmEnSatus[u8Id] = 1;
    }
    else
    {
        MHal_PWMEnable(sstar_chip, u8Id, 0);
    }
}

//------------------------------------------------------------------------------
//
//  Function:   MHal_PWMSetDuty
//
//  Description
//      Set Duty value
//
//  Parameters
//      u8Id:    [in] PWM ID
//      u16Val:  [in] Duty value (percentage)
//
//  Return Value
//      None
//
#ifdef CONFIG_PWM_NEW
void MHal_PWMSetConfig(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 duty, U32 period)
{
    U8  i;
    U16 u16Div     = 0;
    U32 common     = 0;
    U32 pwmclk     = 0;
    U32 periodmax  = 0;
    U32 u32Period  = 0x00000000;
    U32 u32Duty    = 0x00000000;
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    if (u8Id >= PWM_NUM)
        return;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);

    pwmclk = (U32)(clk_get_rate(sstar_chip->clk));

    switch (pwmclk)
    {
        case 12000000:
            pwmclk = 3;
            common = 250;
            break;
        default:
            pwmclk = 3;
            common = 250;
    }

    /*      select   div       */
    for (i = 0; i < (sizeof(clk_pwm_div) / sizeof(U16)); i++)
    {
        periodmax = (clk_pwm_div[i] * 262144 / pwmclk) * common;
        if (period < periodmax)
        {
            u16Div        = clk_pwm_div[i];
            _pwmDiv[u8Id] = clk_pwm_div[i];
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
            SETREG16(U32PwmAddr + u16REG_SW_RESET, BIT0 << ((u8Id == 10) ? 0 : u8Id));
        }
    }
    else
    {
        if (_pwmEnSatus[u8Id])
        {
            CLRREG16(U32PwmAddr + u16REG_SW_RESET, BIT0 << ((u8Id == 10) ? 0 : u8Id));
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
    pr_err("clk=%d, u16Div=%d u32Duty=0x%x u32Period=0x%x\n", (U32)(clk_get_rate(sstar_chip->clk)), u16Div, u32Duty,
           u32Period);
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DIV, (u16Div & 0xFFFF));
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_PERIOD_L, (u32Period & 0xFFFF));
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_PERIOD_H, ((u32Period >> 16) & 0x3));
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_L, (u32Duty & 0xFFFF));
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_H, ((u32Duty >> 16) & 0x3));
}

void MHal_PWMGetConfig(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 *Duty, U32 *Period)
{
    U16 u16Div     = 0;
    U32 u32Duty    = 0;
    U32 u32Period  = 0;
    U32 U32PwmAddr = 0;
    U32 pwmclk     = 0;
    U32 common     = 0;
    U32 u32PwmOffs = 0;
    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);

    if (u8Id >= PWM_NUM)
        return;

    pwmclk = (U32)(clk_get_rate(sstar_chip->clk));

    switch (pwmclk)
    {
        case 12000000:
            pwmclk = 3;
            common = 250;
            break;
        default:
            pwmclk = 3;
            common = 250;
    }

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
void MHal_PWMSetDuty(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Val)
{
    U32 u32Period = 0x00000000;
    U32 u32Duty = 0x00000000;
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;
    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);

    if (PWM_NUM <= u8Id)
        return;

    MHal__PWM_SetClock(sstar_chip);
    if (_pwmEnSatus[u8Id])
    {
        if (0 == u32Val)
            OUTREGMSK16(U32PwmAddr + u16REG_SW_RESET, BIT0 << ((u8Id == 10) ? 0 : u8Id),
                        BIT0 << ((u8Id == 10) ? 0 : u8Id));
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
        u32Period = _pwmPeriod[u8Id];
        u32Period++;
        u32Duty = ((u32Period * u32Val) / 100);
        if (((u32Period * u32Val) % 100) < 50)
        {
            u32Duty--;
        }
    }
    pr_err("reg=x%08X(x%x) clk=%d, u32Duty=x%x\n", (U32)(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_L),
           ((U32)(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_L) & 0xFFFFFF) >> 9, (U32)(clk_get_rate(sstar_chip->clk)),
           u32Duty);
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_L, (u32Duty & 0xFFFF));
    OUTREG16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_DUTY_H, ((u32Duty >> 16) & 0x3));

    if (_pwmEnSatus[u8Id])
    {
        U32 reset = INREG16(U32PwmAddr + u16REG_SW_RESET) & (BIT0 << ((u8Id == 10) ? 0 : u8Id));
        if (u32Val && reset)
            CLRREG16(U32PwmAddr + u16REG_SW_RESET, BIT0 << ((u8Id == 10) ? 0 : u8Id));
    }
}

void MHal_PWMGetDuty(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 *pu32Val)
{
    U32 u32Duty, U32PwmAddr = 0;
    U32 u32PwmOffs = 0;
    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);

    *pu32Val = 0;
    if (PWM_NUM <= u8Id)
        return;
    u32Duty = INREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_DUTY_L)
              | ((INREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_DUTY_H) & 0x3) << 16);
    if (u32Duty)
    {
        U32 u32Period = _pwmPeriod[u8Id];
        // MHal_PWMGetPeriod(sstar_chip, u8Id, &u32Period);
        if (u32Period)
        {
            *pu32Val = (u32Duty * 100) / u32Period;
        }
    }
}

//------------------------------------------------------------------------------
//
//  Function:   MHal_PWMSetPeriod
//
//  Description
//      Set Period value
//
//  Parameters
//      u8Id:    [in] PWM ID
//      u16Val:  [in] Period value (hz)
//
//  Return Value
//      None
//
void MHal_PWMSetPeriod(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Val)
{
    U32 u32Period = 0x00000000;
    U32 U32PwmAddr = 0;
    U32 u32FrqHz = u32Val;
    U32 u32PwmOffs = 0;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    MHal__PWM_SetClock(sstar_chip);
    u32Period = (U32)(clk_get_rate(sstar_chip->clk)) / u32FrqHz;

    //[APN] range 2<=Period<=262144
    if (u32Period < 100)
    {
        u32Period = 100;
    }
    else if (u32Period > 262144)
    {
        u32Period = 262144;
    }
    //[APN] PWM _PERIOD= (REG_PERIOD+1)
    u32Period--;

    pr_err("reg=0x%08X clk=%d, period=0x%x\n", (U32)(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_PERIOD_L),
           (U32)(clk_get_rate(sstar_chip->clk)), u32Period);

    OUTREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_PERIOD_L, (u32Period & 0xFFFF));
    OUTREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_PERIOD_H, ((u32Period >> 16) & 0x3));
    // OUTREG16(sstar_chip->base + (u8Id*0x80) + u16REG_PWM_DIV, (u16Div & 0xFFFF));

    _pwmPeriod[u8Id] = u32Period;
}

void MHal_PWMGetPeriod(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 *pu32Val)
{
    U32 u32Period, u32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    MHal_PWMGetGrpAddr(sstar_chip, &u32PwmAddr, &u32PwmOffs, u8Id);

    u32Period = INREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_PERIOD_L)
                | ((INREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_PERIOD_H) & 0x3) << 16);
    if ((0 == _pwmPeriod[u8Id]) && (u32Period))
    {
        _pwmPeriod[u8Id] = u32Period;
    }
    // u16Div = INREG16(sstar_chip->base + (u8Id*0x80) + u16REG_PWM_DIV) + 1;

    *pu32Val = 0;
    if (u32Period)
    {
        *pu32Val = (U32)(clk_get_rate(sstar_chip->clk)) / (u32Period + 1);
    }
}
#endif
//------------------------------------------------------------------------------
//
//  Function:   MHal_PWMSetPolarity
//
//  Description
//      Set Polarity value
//
//  Parameters
//      u8Id:   [in] PWM ID
//      u8Val:  [in] Polarity value
//
//  Return Value
//      None
//
void MHal_PWMSetPolarity(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val)
{
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;
    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    OUTREGMSK16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_CTRL, (u8Val << POLARITY_BIT), (0x1 << POLARITY_BIT));
}

void MHal_PWMGetPolarity(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 *pu8Val)
{
    *pu8Val = (INREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_CTRL) & (0x1 << POLARITY_BIT)) ? 1 : 0;
}

#if 0
void MHal_PWMSetFreqDiv( U8 u8Id, U8 u8Val )
{
    if( PWM0 == u8Id )
    {
        OUTREGMSK16( BASE_REG_PMSLEEP_PA + u16REG_PWM0_DIV, u8Val, PWM_CTRL_DIV_MSAK );
    }
    else if( PWM1 == u8Id )
    {
        OUTREGMSK16( BASE_REG_PMSLEEP_PA + u16REG_PWM1_DIV, u8Val, PWM_CTRL_DIV_MSAK );
    }
    else if( PWM2 == u8Id )
    {
        OUTREGMSK16( BASE_REG_PMSLEEP_PA + u16REG_PWM2_DIV, u8Val, PWM_CTRL_DIV_MSAK );
    }
    else if( PWM3 == u8Id )
    {
        OUTREGMSK16( BASE_REG_PMSLEEP_PA + u16REG_PWM3_DIV, u8Val, PWM_CTRL_DIV_MSAK );
    }
    else
    {
        printk(KERN_ERR "void MHal_PWMSetDiv error!!!! (%x, %x)\r\n", u8Id, u8Val);
    }

}
#endif

//------------------------------------------------------------------------------
//
//  Function:   MHal_PWMSetDben
//
//  Description
//      Enable/Disable Dben function
//
//  Parameters
//      u8Id:   [in] PWM ID
//      u8Val:  [in] On/Off value
//
//  Return Value
//      None
//

void MHal_PWMSetDben(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val)
{
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    OUTREGMSK16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_CTRL, (u8Val << DBEN_BIT), (0x1 << DBEN_BIT));
    OUTREGMSK16(U32PwmAddr + (u32PwmOffs) + u16REG_PWM_CTRL, (u8Val << VDBEN_SW_BIT), (0x1 << VDBEN_SW_BIT));
}

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

void MHal_PWMEnableGet(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 *pu8Val)
{
    *pu8Val = 0;
    if (PWM_NUM <= u8Id)
        return;
    *pu8Val = _pwmEnSatus[u8Id];
}

// void MHal_PWMPad_dump(void)
//{
//     int i;
//     for (i = 0; i < pwmNum; i++)
//     {
//         pwmPadTbl_t* pTbl = padTbl[i];
//         printk("[%s][%d] %d ------------------------------\n", __FUNCTION__, __LINE__, i);
//         while (1)
//         {
//             regSet_t* pRegSet = pTbl->regSet;
//             printk("[%s][%d]     ******************************\n", __FUNCTION__, __LINE__);
//             printk("[%s][%d]         pad Id = %d\n", __FUNCTION__, __LINE__, pTbl->u32PadId);
//             printk("[%s][%d]         (reg, val, msk) = (0x%08x, 0x%08x, 0x%08x)\n", __FUNCTION__, __LINE__,
//             pRegSet[0].u32Adr, pRegSet[0].u32Val, pRegSet[0].u32Msk); printk("[%s][%d]         (reg, val, msk) =
//             (0x%08x, 0x%08x, 0x%08x)\n", __FUNCTION__, __LINE__, pRegSet[1].u32Adr, pRegSet[1].u32Val,
//             pRegSet[1].u32Msk); if (PAD_UNKNOWN == pTbl->u32PadId)
//             {
//                 break;
//             }
//             pTbl++;
//         }
//     }
// }

void MHal_PWMPadSet(U8 u8Id, U8 u8Val)
{
    /*
      pwmPadTbl_t* pTbl = NULL;
      if (PWM_NUM <= u8Id)
      {
          printk(KERN_ERR "[%s][%d] void MHal_PWMEnable error!!!! (%x, %x)\r\n", __FUNCTION__, __LINE__, u8Id, u8Val);
          return;
      }
      printk("[%s][%d] (pwmId, padId) = (%d, %d)\n", __FUNCTION__, __LINE__, u8Id, u8Val);
      pTbl = padTbl[u8Id];
      while (1)
      {
          if (u8Val == pTbl->u32PadId)
          {
              regSet_t* pRegSet = pTbl->regSet;
              U32 Reg0=(pRegSet[0].u32Adr&0xFFFFFF), Reg1=(pRegSet[1].u32Adr&0xFFFFFF);
              printk("[%s][%d]PDTOP x%x(x%x_x%x), x%x(x%x_x%x)\n", __FUNCTION__, __LINE__,
                                                                    pRegSet[0].u32Adr, Reg0>>9,
      (Reg0-((Reg0>>9)<<9))>>2, pRegSet[1].u32Adr, Reg1>>9, (Reg1-((Reg1>>9)<<9))>>2); if (BASE_REG_NULL !=
      pRegSet[0].u32Adr)
              {
                  OUTREGMSK16(pRegSet[0].u32Adr, pRegSet[0].u32Val, pRegSet[0].u32Msk);
              }
              if (BASE_REG_NULL != pRegSet[1].u32Adr)
              {
                  OUTREGMSK16(pRegSet[1].u32Adr, pRegSet[1].u32Val, pRegSet[1].u32Msk);
              }
              MHal__PWM_SetClock();
              break;
          }
          if (PAD_UNKNOWN == pTbl->u32PadId)
          {
              printk(KERN_ERR "[%s][%d] void MHal_PWMEnable error!!!! (%x, %x)\r\n", __FUNCTION__, __LINE__, u8Id,
      u8Val); break;
          }
          pTbl++;
      }
      */
}

int MHal_PWMGroupCap(void)
{
    return (PWM_GROUP_NUM) ? 1 : 0;
}

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
            printk(KERN_WARNING "[%s][%d] Always enable BIT_%2d for sync mode!\r\n", __FUNCTION__, __LINE__, u8PWMId);
            return 0;
        }
        else
        {
            CLRREG16(sstar_chip->base + REG_GROUP_JOIN, (1 << (u8PWMId + REG_GROUP_JOIN_SHFT)));
        }
    }
    return 1;
}

int MHal_PWMGroupEnable(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U8 u8Val)
{
    U32 u32JoinMask;

    if (PWM_GROUP_NUM <= u8GroupId)
        return 0;

    if (!MHal__PWM_GetSyncFlag())
    {
        MHal__PWM_MemToReg(sstar_chip, u8GroupId);
        printk("MEM to REG done !\n");
    }

    u32JoinMask = ((u8GroupId == 2) ? 0x7 : 0xF) << ((u8GroupId << 2) + REG_GROUP_JOIN_SHFT);
    u32JoinMask &= INREG16(sstar_chip->base + REG_GROUP_JOIN) & u32JoinMask;
    u32JoinMask |= 1 << (u8GroupId + PWM_NUM);

    if (u8Val)
    {
        SETREG16(sstar_chip->base + REG_GROUP_ENABLE, (1 << (u8GroupId + REG_GROUP_ENABLE_SHFT)));
        CLRREG16(sstar_chip->base + u16REG_SW_RESET, u32JoinMask);
        MHal__PWM_SetSyncFlag(1); // dont need to sync until new data in
    }
    else
    {
        CLRREG16(sstar_chip->base + REG_GROUP_ENABLE, (1 << (u8GroupId + REG_GROUP_ENABLE_SHFT)));
        SETREG16(sstar_chip->base + u16REG_SW_RESET, u32JoinMask);
    }
    return 1;
}

int MHal_PWMGroupIsEnable(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U8 *pu8Val)
{
    *pu8Val = 0;
    if (PWM_GROUP_NUM <= u8GroupId)
        return 0;
    *pu8Val = (INREG16(sstar_chip->base + REG_GROUP_ENABLE) >> (u8GroupId + REG_GROUP_ENABLE_SHFT)) & 0x1;
    return 1;
}

int MHal_PWMGroupGetRoundNum(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U16 *pu16Val)
{
    U32 u32Reg;

    if (PWM_GROUP_NUM <= u8GroupId)
        return 0;
    u32Reg   = (u8GroupId << 0x7) + 0x40;
    *pu16Val = INREG16(sstar_chip->base + u32Reg) & 0xFFFF;
    return 1;
}

int MHal_PWMGroupSetRound(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U16 u16Val)
{
    U32 u32Reg;

    if (PWM_GROUP_NUM <= u8GroupId)
        return 0;
    u32Reg = (u8GroupId << 0x7) + 0x40;
    OUTREG16(sstar_chip->base + u32Reg, u16Val);
    return 1;
}

int MHal_PWMGroupStop(struct sstar_pwm_chip *sstar_chip, U8 u8GroupId, U8 u8Val)
{
    if (PWM_GROUP_NUM <= u8GroupId)
        return 0;

    if (u8Val)
        SETREG16(sstar_chip->base + REG_GROUP_STOP, (1 << (u8GroupId + REG_GROUP_STOP_SHFT)));
    else
        CLRREG16(sstar_chip->base + REG_GROUP_STOP, (1 << (u8GroupId + REG_GROUP_STOP_SHFT)));

    return 1;
}

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

//+++[Only4I6e]
int MHal_PWMGroupGetHoldM1(struct sstar_pwm_chip *sstar_chip)
{
#if 1
    return INREG16(sstar_chip->base + REG_GROUP_HOLD_MODE1);
#else
    // printk("\n[WARN][%s L%d] Only4i6e\n", __FUNCTION__, __LINE__);
    return 1;
#endif
}

int MHal_PWMGroupHoldM1(struct sstar_pwm_chip *sstar_chip, U8 u8Val)
{
#if 1
    if (u8Val)
    {
        SETREG16(sstar_chip->base + REG_GROUP_HOLD_MODE1, 1);
        printk("[%s L%d] hold mode1 en!(keep low)\n", __FUNCTION__, __LINE__);
    }
    else
    {
        CLRREG16(sstar_chip->base + REG_GROUP_HOLD_MODE1, 0);
        printk("[%s L%d] hold mode1 dis!\n", __FUNCTION__, __LINE__);
    }
#else
    // printk("\n[WARN][%s L%d] Only4i6e\n", __FUNCTION__, __LINE__);
#endif
    return 1;
}

void MHal_PWMDutyQE0(struct sstar_pwm_chip *sstar_chip, U8 u8Val)
{
    if (u8Val)
        SETREG16(sstar_chip->base + REG_PWM_DUTY_QE0, (1 << REG_PWM_DUTY_QE0_SHFT));
    else
        CLRREG16(sstar_chip->base + REG_PWM_DUTY_QE0, (1 << REG_PWM_DUTY_QE0_SHFT));
}

int MHal_PWMGetOutput(struct sstar_pwm_chip *sstar_chip, U8 *pu8Output)
{
#if 1
    *pu8Output = INREG16(sstar_chip->base + REG_PWM_OUT);
    printk("[%s L%d] output:x%x\n", __FUNCTION__, __LINE__, *pu8Output);
#else
    // printk("\n[WARN][%s L%d] Only4i6e\n", __FUNCTION__, __LINE__);
#endif
    return 1;
}
//---[Only4I6e]

int MHal_PWMSetEnd(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8DutyId, U32 u32Val)
{
    U32 u32Period;
    U32 u32Duty;

    if (PWM_NUM <= u8Id)
        return 0;

    // u32Period = INREG16(sstar_chip->base + (u8Id*0x80) + u16REG_PWM_PERIOD_L) + ((INREG16(sstar_chip->base +
    // (u8Id*0x80) + u16REG_PWM_PERIOD_H)<<16));
    u32Period                = _pwmPeriod[u8Id];
    u32Duty                  = ((u32Period * u32Val) / 1000);
    _pwmDutyArgNum[u8Id]     = u8DutyId;
    _pwmDuty[u8Id][u8DutyId] = u32Duty;
    MHal__PWM_SetSyncFlag(0);

    if (u32Duty & 0xFFFC0000)
    {
        printk("[%s][%d] too large duty 0x%08x (18 bits in max)\n", __FUNCTION__, __LINE__, u32Duty);
    }
    return 1;
}

int MHal_PWMSetEndToReg(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8DutyArgNum)
{
    U8 arg_no;
    for (arg_no = 0; arg_no < u8DutyArgNum; arg_no++)
    {
        if (0 == arg_no)
        {
            OUTREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_DUTY_L, (_pwmDuty[u8Id][arg_no] & 0xFFFF));
            OUTREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_DUTY_H, ((_pwmDuty[u8Id][arg_no] >> 16) & 0x0003));
        }
        else
        {
            OUTREG16(sstar_chip->base + (u8Id * 0x80) + (arg_no << 3) + 28, (_pwmDuty[u8Id][arg_no] & 0xFFFF));
        }
    }
    return 1;
}

int MHal_PWMSetBegin(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8ShftId, U32 u32Val)
{
    U32 u32Period;
    U32 u32Shft;

    if (PWM_NUM <= u8Id)
        return 0;

    u32Period                = _pwmPeriod[u8Id];
    u32Shft                  = ((u32Period * u32Val) / 1000);
    _pwmShftArgNum[u8Id]     = u8ShftId;
    _pwmShft[u8Id][u8ShftId] = u32Shft;
    MHal__PWM_SetSyncFlag(0);

    if (u32Shft & 0xFFFC0000)
    {
        printk("[%s][%d] too large shift 0x%08x (18 bits in max)\n", __FUNCTION__, __LINE__, u32Shft);
    }

    return 1;
}

int MHal_PWMSetBeginToReg(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8ShftArgNum)
{
    U8 arg_no;
    for (arg_no = 0; arg_no < u8ShftArgNum; arg_no++)
    {
        if (0 == arg_no)
        {
            OUTREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_SHIFT_L, (_pwmShft[u8Id][arg_no] & 0xFFFF));
            OUTREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_SHIFT_H, ((_pwmShft[u8Id][arg_no] >> 16) & 0x0003));
        }
        else
        {
            OUTREG16(sstar_chip->base + (u8Id * 0x80) + (arg_no << 3) + 24, (_pwmShft[u8Id][arg_no] & 0xFFFF));
        }
    }
    return 1;
}

int MHal_PWMSetPolarityEx(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val)
{
    if (PWM_NUM <= u8Id)
        return 0;
    _pwmPolarity[u8Id] = u8Val;
    MHal__PWM_SetSyncFlag(0);
    return 1;
}

int MHal_PWMSetPolarityExToReg(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val)
{
    if (u8Val)
        SETREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_CTRL, (0x1 << POLARITY_BIT));
    else
        CLRREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_CTRL, (0x1 << POLARITY_BIT));
    return 1;
}

void MHal_PWMSetPeriodEx(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Val)
{
    U32 u32Period;
    U32 u32Div;
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    if (u8Id >= PWM_NUM)
        return;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    u32Div = INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DIV); // workaround

    u32Period = (U32)(clk_get_rate(sstar_chip->clk)) / u32Val;
    u32Period /= (u32Div + 1); // workaround

    //[APN] range 2<=Period<=262144
    if (u32Period < 2)
        u32Period = 2;
    if (u32Period > 262144)
        u32Period = 262144;

    //[APN] PWM _PERIOD= (REG_PERIOD+1)
    u32Period--;
    _pwmPeriod[u8Id] = u32Period;
    _pwmFreq[u8Id]   = u32Val;
    MHal__PWM_SetSyncFlag(0);

    pr_err("reg=0x%08X clk=%d, period=0x%x\n", (U32)(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_PERIOD_L),
           (U32)(clk_get_rate(sstar_chip->clk)), u32Period);
}

void MHal_PWMSetPeriodExToReg(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U32 u32Period)
{
    OUTREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_PERIOD_L, (u32Period & 0xFFFF));
    OUTREG16(sstar_chip->base + (u8Id * 0x80) + u16REG_PWM_PERIOD_H, ((u32Period >> 16) & 0x3));
}

#if 0
int MHal_PWMSetMPluse(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val)
{
    if (PWM_NUM <= u8Id)
        return 0;
    if (u8Val)
        SETREG16(sstar_chip->base + (u8Id*0x80) + u16REG_PWM_CTRL, (0x1<<DIFF_P_EN_BIT));
    else
        CLRREG16(sstar_chip->base + (u8Id*0x80) + u16REG_PWM_CTRL, (0x1<<DIFF_P_EN_BIT));
    return 1;
}
#endif

int MHal_PWMDiv(struct sstar_pwm_chip *sstar_chip, U8 u8Id, U8 u8Val)
{
    U32 U32PwmAddr = 0;
    U32 u32PwmOffs = 0;

    if (u8Id >= PWM_NUM)
        return 0;

    MHal_PWMGetGrpAddr(sstar_chip, &U32PwmAddr, &u32PwmOffs, u8Id);
    OUTREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DIV, u8Val);
    printk("mhal  MHal_PWMDiv done !\n");

    return 1;
}

int MHal_PWMGroupInfo(struct sstar_pwm_chip *sstar_chip, char *buf_start, char *buf_end)
{
    char *str = buf_start;
    char *end = buf_end;
    int   i;
    // U32 tmp;
    U32 u32Period, u32Polarity; // , u32MPluse;
    // U32 u32Shft0, u32Shft1, u32Shft2, u32Shft3;
    U32 u32Shft0;
    // U32 u32Duty0, u32Duty1, u32Duty2, u32Duty3;
    U32 u32Duty0;
    U32 u32SyncStatus;
    U32 u32ResetStatus;
    U32 u32GroupEnable, u32GroupReset, u32GroupHold, u32GroupStop, u32GroupRound;
    U32 clk = (U32)clk_get_rate(sstar_chip->clk);
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
        // u32MPluse = (INREG16(sstar_chip->base + (i*0x80) + u16REG_PWM_CTRL) >> DIFF_P_EN_BIT) & 0x1;
        // Period
#if 0
        if ((tmp = INREG16(sstar_chip->base + (i*0x80) + u16REG_PWM_PERIOD_H)))
            printk("[%s][%d] pwmId %d period_h is not zero (0x%08x)\n", __FUNCTION__, __LINE__, i, tmp);
#endif

        u32Period = INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_PERIOD_L)
                    | ((INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_PERIOD_H) & 0x3) << 16);
        // Shift
#if 0
        if ((tmp = INREG16(sstar_chip->base + (i*0x80) + u16REG_PWM_SHIFT_H)))
            printk("[%s][%d] pwmId %d shift_h is not zero (0x%08x)\n", __FUNCTION__, __LINE__, i, tmp);
#endif
        u32Shft0 = INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_SHIFT_L)
                   | ((INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_SHIFT_H) & 0x3) << 16);
        // u32Shft1 = INREG16(sstar_chip->base + (i*0x80) + u16REG_PWM_SHIFT2);
        // u32Shft2 = INREG16(sstar_chip->base + (i*0x80) + u16REG_PWM_SHIFT3);
        // u32Shft3 = INREG16(sstar_chip->base + (i*0x80) + u16REG_PWM_SHIFT4);
        // Duty
#if 0
        if ((tmp = INREG16(sstar_chip->base + (i*0x80) + u16REG_PWM_DUTY_H)))
            printk("[%s][%d] pwmId %d duty_h is not zero (0x%08x)\n", __FUNCTION__, __LINE__, i, tmp);
#endif
        u32Duty0 = INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DUTY_L)
                   | ((INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DUTY_H) & 0x3) << 16);
        u32Duty0 -= u32Shft0;
        // u32Duty1 = INREG16(sstar_chip->base + (i*0x80) + u16REG_PWM_DUTY2);
        // u32Duty2 = INREG16(sstar_chip->base + (i*0x80) + u16REG_PWM_DUTY3);
        // u32Duty3 = INREG16(sstar_chip->base + (i*0x80) + u16REG_PWM_DUTY4);
        // sync mode status
        u32SyncStatus = (INREG16(U32PwmAddr + REG_GROUP_JOIN) >> (i + REG_GROUP_JOIN_SHFT)) & 0x1;
        // rest status
        u32ResetStatus = (INREG16(U32PwmAddr + u16REG_SW_RESET) >> ((i == 10) ? 0 : i)) & BIT0;
        u32Div         = INREG16(U32PwmAddr + u32PwmOffs + u16REG_PWM_DIV); // workaround

        // output to buffer
        str += scnprintf(str, end - str, "Pwm %d\n", i);
        str += scnprintf(str, end - str, "\tPad\t\t0x%08x\n", sstar_chip->pad_ctrl[i]);
        str += scnprintf(str, end - str, "\tSync status\t%d\n", u32SyncStatus);
        str += scnprintf(str, end - str, "\tReset status\t%d\n", u32ResetStatus);
        str += scnprintf(str, end - str, "\tPolarity\t%d\n", u32Polarity);
#if 0
        str += scnprintf(str, end - str, "\tPeriod\t\t0x%08x\n", u32Period);
        str += scnprintf(str, end - str, "\tBegin\t\t0x%08x\n", u32Shft0);
        str += scnprintf(str, end - str, "\tEnd\t\t0x%08x\n", u32Duty0);
#endif
        u32Period++;
        u32Shft0++;
        u32Duty0++;
        u32Shft0 = (1000 * u32Shft0) / u32Period;
        u32Duty0 = (1000 * u32Duty0) / u32Period;
        // u32Period = ((u32Div+1)*clk)/u32Period;
        u32Period = clk / u32Period / (u32Div + 1);

        str += scnprintf(str, end - str, "\tPeriod\t\t%d\n", u32Period);
        // str += scnprintf(str, end - str, "\tBegin\t\t0x%08x 0x%08x 0x%08x 0x%08x\n", u32Shft0, u32Shft1, u32Shft2,
        // u32Shft3); str += scnprintf(str, end - str, "\tEnd\t\t0x%08x 0x%08x 0x%08x 0x%08x\n", u32Duty0, u32Duty1,
        // u32Duty2, u32Duty3);
        str += scnprintf(str, end - str, "\tBegin\t\t%d\n", u32Shft0);
        str += scnprintf(str, end - str, "\tEnd\t\t%d\n", u32Duty0);
    }
    // str += scnprintf(str, end - str, "This is a test\n");
    return (str - buf_start);
}

irqreturn_t PWM_IRQ(int irq, void *data)
{
#if 1 // Only4i6e
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
                    MHal__PWM_MemToReg(sstar_chip, gid);
                }
                MHal_PWMGroupHold(sstar_chip, gid, 0); // For clear hold event;
                MHal__PWM_SetSyncFlag(1);
                pr_err("hold_int GrpId:%d(event:x%x)\n", gid, u16_Events);
            }
        }
    }
    if ((u16_Events >> REG_GROUP_RUND_INT_SHFT))
    {
        for (gid = 0; gid < PWM_GROUP_NUM; gid++)
        {
            if ((u16_Events >> REG_GROUP_RUND_INT_SHFT) & (1 << gid))
            {
                if (MHal__PWM_GetSyncFlag())
                {                                              // no new data
                    MHal_PWMGroupEnable(sstar_chip, gid, 0);   // For clear round event;
                    MHal_PWMGroupSetRound(sstar_chip, gid, 0); // set round=0
                }
                else
                { // have new data
                    MHal_PWMGroupEnable(sstar_chip, gid, 0);
                    MHal_PWMGroupEnable(sstar_chip, gid, 1);
                }
                pr_err("round_int GrpId:%d\n", gid);
            }
        }
    }
#endif
    return IRQ_NONE;
}
