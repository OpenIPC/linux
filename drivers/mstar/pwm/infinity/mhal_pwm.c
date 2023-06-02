// $Change: 626442 $
//-----------------------------------------------------------------------------
//
// Copyright (c) 2008 MStar Semiconductor, Inc.  All rights reserved.
//
//-----------------------------------------------------------------------------
// FILE
//      pwm.c
//
// DESCRIPTION
//      PWM HW control codes.
//
// HISTORY
//      2008.05.15      Bryan Fan      Initial Version
//      2008.6.12       Bryan Fan      Revised for Code Review
//      2008.6.26       Bryan Fan      Revised for new Code Review
//
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------

#include <linux/kernel.h>
#include "ms_platform.h"
#include "ms_types.h"
#include "infinity/registers.h"
#include "mhal_pwm.h"

//------------------------------------------------------------------------------
//  Variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Local Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  External Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Global Functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//  Function:   DrvPWMSetDuty
//
//  Description
//      Set Duty value
//
//  Parameters
//      u8Id:    [in] PWM ID
//      u16Val:  [in] Duty value
//
//  Return Value
//      None
//

void DrvPWMSetDuty( U8 u8Id, U16 u16Val )
{
    U32 u32Period;
    U16 u16Duty;

    u16Val = u16Val & PWM_DUTY_MASK;

    if( PWM0 == u8Id )
    {
        u32Period = INREG16(BASE_REG_PMSLEEP_PA + u16REG_PWM0_PERIOD);
        u16Duty=u32Period*u16Val/100;
        OUTREG16(BASE_REG_PMSLEEP_PA + u16REG_PWM0_DUTY, u16Duty);
    }
    else if( PWM1 == u8Id )
    {
        u32Period = INREG16(BASE_REG_PMSLEEP_PA + u16REG_PWM1_PERIOD);
        u16Duty=u32Period*u16Val/100;
        OUTREG16(BASE_REG_PMSLEEP_PA + u16REG_PWM1_DUTY, u16Duty);
    }
    else if( PWM2 == u8Id )
    {
        u32Period = INREG16(BASE_REG_PMSLEEP_PA + u16REG_PWM2_PERIOD);
        u16Duty=u32Period*u16Val/100;
        OUTREG16(BASE_REG_PMSLEEP_PA + u16REG_PWM2_DUTY, u16Duty);
    }
    else if( PWM3 == u8Id )
    {
        u32Period = INREG16(BASE_REG_PMSLEEP_PA + u16REG_PWM3_PERIOD);
        u16Duty=u32Period*u16Val/100;
        OUTREG16(BASE_REG_PMSLEEP_PA + u16REG_PWM3_DUTY, u16Duty);
    }
    else
    {
        printk(KERN_INFO "DrvPWMSetDuty error!!!! (%x, %x)\r\n", u8Id, u16Val);
    }
}

//------------------------------------------------------------------------------
//
//  Function:   DrvPWMSetPeriod
//
//  Description
//      Set Period value
//
//  Parameters
//      u8Id:    [in] PWM ID
//      u16Val:  [in] Period value
//
//  Return Value
//      None
//

void DrvPWMSetPeriod( U8 u8Id, U16 u16Val )
{
    U32 u32TempValue;

    u32TempValue=DEFAULT_PWM_CLK/u16Val;
    u16Val = u32TempValue & PWM_PERIOD_MASK;

    printk(KERN_INFO "DrvPWMSetPeriod !!!! (%x, %x)\r\n", u32TempValue, u16Val);

    if( PWM0 == u8Id )
    {
        OUTREG16(BASE_REG_PMSLEEP_PA + u16REG_PWM0_PERIOD, u16Val);
    }
    else if( PWM1 == u8Id )
    {
        OUTREG16(BASE_REG_PMSLEEP_PA + u16REG_PWM1_PERIOD, u16Val);
    }
    else if( PWM2 == u8Id )
    {
        OUTREG16(BASE_REG_PMSLEEP_PA + u16REG_PWM2_PERIOD, u16Val);
    }
    else if( PWM3 == u8Id )
    {
        OUTREG16(BASE_REG_PMSLEEP_PA + u16REG_PWM3_PERIOD, u16Val);
    }
    else
    {
        printk(KERN_ERR "void DrvPWMSetPeriod error!!!! (%x, %x)\r\n", u8Id, u16Val);
    }

}

//------------------------------------------------------------------------------
//
//  Function:   DrvPWMSetPolarity
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

void DrvPWMSetPolarity( U8 u8Id, U8 u8Val )
{
    if( PWM0 == u8Id )
    {
        OUTREGMSK16( BASE_REG_PMSLEEP_PA + u16REG_PWM0_DIV, (u8Val<<8), PWM_CTRL_POLARITY );
    }
    else if( PWM1 == u8Id )
    {
        OUTREGMSK16( BASE_REG_PMSLEEP_PA + u16REG_PWM1_DIV, (u8Val<<8), PWM_CTRL_POLARITY );
    }
    else if( PWM2 == u8Id )
    {
        OUTREGMSK16( BASE_REG_PMSLEEP_PA + u16REG_PWM2_DIV, (u8Val<<8), PWM_CTRL_POLARITY );
    }
    else if( PWM3 == u8Id )
    {
        OUTREGMSK16( BASE_REG_PMSLEEP_PA + u16REG_PWM3_DIV, (u8Val<<8), PWM_CTRL_POLARITY );
    }
    else
    {
        printk(KERN_ERR "void DrvPWMSetPolarity error!!!! (%x, %x)\r\n", u8Id, u8Val);
    }

}

void DrvPWMSetFreqDiv( U8 u8Id, U8 u8Val )
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
        printk(KERN_ERR "void DrvPWMSetDiv error!!!! (%x, %x)\r\n", u8Id, u8Val);
    }

}

//------------------------------------------------------------------------------
//
//  Function:   DrvPWMSetDben
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

void DrvPWMSetDben( U8 u8Id, U8 u8Val )
{
    if( PWM0 == u8Id )
    {
        OUTREGMSK16(  BASE_REG_PMSLEEP_PA + u16REG_PWM0_DIV, (u8Val<<12), PWM_CTRL_DBEN );
    }
    else if( PWM1 == u8Id )
    {
        OUTREGMSK16(  BASE_REG_PMSLEEP_PA + u16REG_PWM1_DIV, (u8Val<<12), PWM_CTRL_DBEN );
    }
    else if( PWM2 == u8Id )
    {
        OUTREGMSK16(  BASE_REG_PMSLEEP_PA + u16REG_PWM2_DIV, (u8Val<<12), PWM_CTRL_DBEN );
    }
    else if( PWM3 == u8Id )
    {
        OUTREGMSK16(  BASE_REG_PMSLEEP_PA + u16REG_PWM3_DIV, (u8Val<<12), PWM_CTRL_DBEN );
    }
    else
    {
        printk(KERN_ERR "void DrvPWMSetDben error!!!! (%x, %x)\r\n", u8Id, u8Val);
    }

}

void DrvPWMEnable( U8 u8Id, U8 u8Val )
{
    if( PWM0 == u8Id )
    {
        //reg_pwm0_mode = BIT[1:0]
        OUTREGMSK16(BASE_REG_CHIPTOP_PA + REG_ID_07, u8Val, 0x3);
    }
    else if( PWM1 == u8Id )
    {
        //reg_pwm1_mode=BIT[3:2]
        OUTREGMSK16(BASE_REG_CHIPTOP_PA + REG_ID_07, (u8Val<<2), 0xC);
    }
    else if( PWM2 == u8Id )
    {
        //reg_pwm2_mode=BIT[4]
        OUTREGMSK16(BASE_REG_CHIPTOP_PA + REG_ID_07, (u8Val<<4), 0x10);
    }
    else if( PWM3 == u8Id )
    {
        //reg_pwm3_mode=BIT[6]
        OUTREGMSK16(BASE_REG_CHIPTOP_PA + REG_ID_07, (u8Val<<6), 0x40);
    }
    else
    {
        printk(KERN_ERR "void DrvPWMEnable error!!!! (%x, %x)\r\n", u8Id, u8Val);
    }
}


//------------------------------------------------------------------------------
//
//  Function:   DrvPWMInit
//
//  Description
//      PWM init function
//
//  Parameters
//      u8CustLevel:   [in] Cust Level
//
//  Return Value
//      None
//------------------------------------------------------------------------------
void DrvPWMInit( U8 u8Id )
{
    //printk(KERN_INFO "+DrvPWMInit\r\n");

    DrvPWMSetFreqDiv( u8Id, DEFAULT_DIV_CNT );
    //DrvPWMSetPeriod( u8Id, DEFAULT_PERIOD );

    DrvPWMSetPolarity( u8Id, DEFAULT_POLARITY );

    DrvPWMSetDben( u8Id, DEFAULT_DBEN );

    //printk(KERN_INFO "-DrvPWMInit\r\n");
}

