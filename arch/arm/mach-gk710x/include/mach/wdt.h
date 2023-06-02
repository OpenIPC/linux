/*!
*****************************************************************************
** \file        arch/arm/mach-gk7101/include/mach/wdt.h
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

#ifndef __MACH_WDT_H
#define __MACH_WDT_H

/* ==============================================*/

/****************************************************/
/*          Capabilities based on chip revision                            */
/****************************************************/

/* On A5S WDT_RST_L_REG can not be cleared.      */
/* When is more then 0xff. To work around we set */
/* WDT_RST_L_REG to 0x0 before WDT start because */
/* it is preserved during soft reset, if it's    */
/* still 0x0 we could know the reset is from WDT */

#define RCT_WDT_RESET_VAL   0

/****************************************************/
/*          Controller registers definitions                                  */
/****************************************************/

#define WDOG_REG(x)             (GK_VA_WDT + (x))

#define WDOG_STATUS_OFFSET      0x0c
#define WDOG_RELOAD_OFFSET      0x10
#define WDOG_RESTART_OFFSET     0x14
#define WDOG_CONTROL_OFFSET     0x00
#define WDOG_TIMEOUT_OFFSET     0x04
#define WDOG_CLR_TMO_OFFSET     0x08
#define WDOG_RST_WD_OFFSET      0x18

#define WDOG_STATUS_REG         WDOG_REG(WDOG_STATUS_OFFSET)
#define WDOG_RELOAD_REG         WDOG_REG(WDOG_RELOAD_OFFSET)
#define WDOG_RESTART_REG        WDOG_REG(WDOG_RESTART_OFFSET)
#define WDOG_CONTROL_REG        WDOG_REG(WDOG_CONTROL_OFFSET)
#define WDOG_TIMEOUT_REG        WDOG_REG(WDOG_TIMEOUT_OFFSET)
#define WDOG_CLR_TMO_REG        WDOG_REG(WDOG_CLR_TMO_OFFSET)
#define WDOG_RST_WD_REG         WDOG_REG(WDOG_RST_WD_OFFSET)

/* Bit field definition of watch dog timer control register */
#define WDOG_CTR_RST_EN         0x00000004
#define WDOG_CTR_INT_EN         0x00000002
#define WDOG_CTR_EN             0x00000001

/* WDOG_RESTART_REG only works with magic 0x4755. */
/* Set this value would transferred the value in  */
/* WDOG_RELOAD_REG into WDOG_STATUS_REG and would */
/* not trigger the underflow event.           */
#define WDT_RESTART_VAL         0X4755

/* ==============================================*/
#ifndef __ASSEMBLER__

extern struct platform_device   gk_wdt;

#endif /* __ASSEMBLER__ */
/* ==============================================*/

#endif /* __REGS_WDT_H */

