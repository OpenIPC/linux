/*!
*****************************************************************************
** \file        arch/arm/mach-gk7101/include/mach/uncompress.h
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

#ifndef __MACH_UNCOMPRESS_H
#define __MACH_UNCOMPRESS_H

/* ==========================================================================*/

/* ==========================================================================*/
#ifndef __ASSEMBLER__

#include <mach/uart.h>
#include <mach/io.h>
#include <mach/rct.h>
#include CONFIG_GK_CHIP_INCLUDE_FILE

typedef void* (*hal_function_t) (unsigned long, unsigned long, unsigned long, unsigned long, unsigned long) ;

struct hw_ops *g_hw;

#define PA_UART0                    GK_PA_UART0
#define UART_RB_OFFSET              0x04
#define UART_TH_OFFSET              0x04
#define UART_DLL_OFFSET             0x04
#define UART_IE_OFFSET              0x00
#define UART_DLH_OFFSET             0x00
#define UART_II_OFFSET              0x08
#define UART_FC_OFFSET              0x08
#define UART_LC_OFFSET              0x18
#define UART_MC_OFFSET              0x0c
#define UART_LS_OFFSET              0x14
#define UART_MS_OFFSET              0x10
#define UART_SC_OFFSET              0x1c    /* Byte */
#define UART_SRR_OFFSET             0x88


#define UART_LS_REG     ((PA_UART0 + UART_LS_OFFSET))
#define UART_TH_REG     ((PA_UART0 + UART_TH_OFFSET))
#define UART_RB_REG     ((PA_UART0 + UART_RB_OFFSET))
#define UART_SRR_REG    ((PA_UART0 + UART_SRR_OFFSET))

#define UART_LC_REG     ((PA_UART0 + UART_LC_OFFSET))
#define UART_DLL_REG    ((PA_UART0 + UART_DLL_OFFSET))
#define UART_DLH_REG    ((PA_UART0 + UART_DLH_OFFSET))

/* ==========================================================================*/

static inline void putc(int c)
{
    while (!(gk_uart_readl(UART_LS_REG) & UART_LS_TEMT));
    gk_uart_writel(UART_TH_REG, c);
}

static inline void flush(void)
{
    unsigned int dump;

    while (gk_uart_readl(UART_LS_REG) & UART_LS_DR)
    {
        dump = gk_uart_readl(UART_RB_REG);
    }
}





static inline void arch_decomp_setup(void)
{

    hal_function_t hal_init = (hal_function_t) (*(volatile u32 *)CONFIG_U2K_HAL_ADDR);

    g_hw = (struct hw_ops *)hal_init (0, 0, 0x90000000, 0xa0000000, 0);

}

#define arch_decomp_wdog()

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif /* __MACH_UNCOMPRESS_H */
