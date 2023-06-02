/*!
*****************************************************************************
** \file        arch/arm/mach-gk710xs/include/mach/gpio.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef __MACH_GPIO_H_
#define __MACH_GPIO_H_

#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <asm-generic/gpio.h>
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <plat/gk_gpio.h>

//*****************************************************************************
//*****************************************************************************
//** Defines and Macros
//*****************************************************************************
//*****************************************************************************
#define GPIO0_BASE                      (GK_VA_GPIO0)
#define GPIO0_BANK0_BASE                (GK_VA_GPIO0)
#define GPIO0_BANK1_BASE                (GK_VA_GPIO0)
#define GPIO0_BANK0_PLL_IOCTRL_BASE     (GK_VA_RCT + 0x0230)            /* read/write */
#define GPIO0_BANK1_PLL_IOCTRL_BASE     (GK_VA_RCT + 0x0230 - 0x100)    /* read/write */

// for bank
#define REG_GPIO_IS_LOW_OFFSET          (0x0000) /* read/write */
#define REG_GPIO_IS_HIGH_OFFSET         (0x0004) /* read/write */
#define REG_GPIO_IBE_LOW_OFFSET         (0x0008) /* read/write */
#define REG_GPIO_IBE_HIGH_OFFSET        (0x000C) /* read/write */
#define REG_GPIO_IEV_LOW_OFFSET         (0x0010) /* read/write */
#define REG_GPIO_IEV_HIGH_OFFSET        (0x0014) /* read/write */
#define REG_GPIO_IE_LOW_OFFSET          (0x0018) /* read/write */
#define REG_GPIO_IE_HIGH_OFFSET         (0x001C) /* read/write */
#define REG_GPIO_IC_LOW_OFFSET          (0x0020) /* write */
#define REG_GPIO_IC_HIGH_OFFSET         (0x0024) /* write */
#define REG_GPIO_RIS_LOW_OFFSET         (0x0028) /* read */
#define REG_GPIO_RIS_HIGH_OFFSET        (0x002C) /* read */
#define REG_GPIO_MIS_LOW_OFFSET         (0x0030) /* read */
#define REG_GPIO_MIS_HIGH_OFFSET        (0x0034) /* read */
#define REG_GPIO_DIN_LOW_OFFSET         (0x0040) /* read */
#define REG_GPIO_DIN_HIGH_OFFSET        (0x0044) /* read */
#define REG_GPIO_OUTPUT_CFG_OFFSET      (0x0100) /* write */
#define REG_GPIO_INPUT_CFG_OFFSET       (0x0200) /* write */

// for all bank
#define REG_GPIO_INT_EN_OFFSET          (0x0038) /* write */
#define REG_GPIO_PER_SEL_OFFSET         (0x003C) /* read/write */

//*****************************************************************************
//*****************************************************************************
//** Enumerated types
//*****************************************************************************
//*****************************************************************************

/////add:
//#if defined(CONFIG_MACH_GK7102S_ELEC_SC1135_V1_00)
//#include "gpio_elec_sc1135_v1.h"
//#endif
/*!
*******************************************************************************
**
** \brief All available GPIO pins named by number.
**
** \sa GPIO_Open()
** \sa GPIO_OpenFunctionMode()
** \sa GPIO_SetType()
**
*******************************************************************************
*/

//*****************************************************************************
//*****************************************************************************
//** Data Structures
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//** Global Data
//*****************************************************************************
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
#ifdef __cplusplus
extern "C" {
#endif

extern int __init gk_init_gpio(void);

#ifdef __cplusplus
}
#endif


#endif /* __MACH_GPIO_H_ */

