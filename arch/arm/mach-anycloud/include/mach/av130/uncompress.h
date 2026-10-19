/*
 * linux/arch/arm/mach-anycloud/include/mach/uncompress.h
 *
 * Copyright (C) 2018 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
 *
 * Author: Feilong Dong <dong_feilong@anyka.com>
 *         Donghua Cao  <cao_donghua@anyka.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __UNCOMPRESS_H_
#define __UNCOMPRESS_H_

#include <asm/sizes.h>
#include <mach/map.h>

#ifdef CONFIG_MACH_AK3918AV100_FPGA
#define BAUD_RATE                   2000000
#else
#define BAUD_RATE                   115200
#endif

#define ENDDING_OFFSET              60

#undef REG32
#define REG32(_reg)                 (*(volatile unsigned long *)(_reg))

#define CLK_CORE_PLL_CTRL           (AK_PA_SYSCTRL + 0x08)

/* L2 buffer address */
#define UART0_TXBUF_ADDR            (0x48000000 + 0x1000) //0x48001000
#define UART0_RXBUF_ADDR            (0x48000000 + 0x1080)

/* L2 buffer control register */
#define L2BUF_CONF2_REG             REG32(0x20140000 + 0x008C) //0x2014008c
#define UART0_TXBUF_CLR_BIT         16
#define UART0_RXBUF_CLR_BIT         17


#if defined(CONFIG_MACH_AK37D)
/* pullup/pulldown configure registers */
#define PPU_PPD1_REG            REG32(AK_PA_SYSCTRL + 0x194) //0x08000194
#define TXD0_PU_BIT             1
#define RXD0_PU_BIT             0
/*********** Shared pin control reigsters ********/
#define SRDPIN_CTRL1_REG        REG32(AK_PA_SYSCTRL + 0x178) //0x08000178
#define UART0_RXD               0
#define UART0_TXD               2

#elif defined(CONFIG_MACH_AK39EV330)
/* pullup/pulldown configure registers */
#define PPU_PPD1_REG            REG32(AK_PA_SYSCTRL + 0x80)  //0x08000080
#define TXD0_PU_BIT             2
#define RXD0_PU_BIT             1
/*********** Shared pin control reigsters ********/
#define SRDPIN_CTRL1_REG        REG32(AK_PA_SYSCTRL + 0x74) //0x08000074
#define UART0_RXD               2
#define UART0_TXD               3

#elif defined(CONFIG_MACH_AK37E)
/* pullup/pulldown configure registers */
#define PPU_PPD1_REG            REG32(AK_PA_SYSCTRL + 0x1E4) //0x080001E4
#define TXD0_PU_BIT             23
#define RXD0_PU_BIT             22
/*********** Shared pin control reigsters ********/
#define SRDPIN_CTRL1_REG        REG32(AK_PA_SYSCTRL + 0x164) //0x08000164
#define UART0_RXD               6
#define UART0_TXD               9

#elif defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L) || \
    defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A)

/* 
 * pupd_en_reg1 0x0800,0268
 * GPIO51/uart0_txd  pupd_en_reg1[19]
 * GPIO52/uart0_rxd  pupd_en_reg1[20]  
 */
#define PPU_PPD1_REG            REG32(AK_PA_SYSCTRL + 0x268)
#define TXD0_PU_BIT             19
#define RXD0_PU_BIT             20
/* 
 * funmux_reg5 0x0800,018C 
 * GPIO51/uart0_txd  funcmux_reg5[8:6]  
 * GPIO52/uart0_rxd  funcmux_reg5[11:9] 
 */
#define SRDPIN_CTRL1_REG        REG32(AK_PA_SYSCTRL + 0x18c) 
#define UART0_RXD               9
#define UART0_TXD               6

#endif

/* Clock control register */
#define CLK_CTRL_REG1           REG32(AK_PA_SYSCTRL + 0x1C) //0x0800001C
#define UART0_CLKEN_BIT         7   //0x0800,001C


/** ************ UART registers *****************************/
#define UART0_CONF1_REG         REG32(0x20130000 + 0x00) //0x20130000
#define UART0_CONF2_REG         REG32(0x20130000 + 0x04)
#define UART0_DATA_CONF_REG     REG32(0x20130000 + 0x08)
#define UART0_BUF_THRE_REG      REG32(0x20130000 + 0x0C)
#define UART0_BUF_RX_REG        REG32(0x20130000 + 0x10)
#define UART0_BUF_RX_BACKUP_REG REG32(0x20130000 + 0x14)
#define UART0_BUF_STOPBIT_REG   REG32(0x20130000 + 0x18)

/* bit define of UARTx_CONF1_REG */
#define CTS_SEL_BIT             18
#define RTS_SEL_BIT             19
#define PORT_ENABLE_BIT         21  //0: disable, 1:enable
#define TX_STATUS_CLR_BIT       28
#define RX_STATUS_CLR_BIT       29

/* bit define of UARTx_CONF2_REG */
#define TX_COUNT_BIT            4
#define TX_COUNT_VALID_BIT      16
#define TX_END_BIT              19
#define TX_END_MASK             (1 << TX_END_BIT)
    
#define UART_TXBUF_CLR_BIT      UART0_TXBUF_CLR_BIT
// GPIO 1&2, 0x0800,0074 bit[1]&bit[2] == 1&1
#define SRDPIN_UART_RXTX_BIT    ((1 << UART0_RXD)|(1 << UART0_TXD))
#define RXD_PU_BIT              RXD0_PU_BIT
#define TXD_PU_BIT              TXD0_PU_BIT
#define UART_CLKEN_BIT          UART0_CLKEN_BIT
#define UART_TXBUF_ADDR         UART0_TXBUF_ADDR
#define UART_CONF1_REG          UART0_CONF1_REG
#define UART_CONF2_REG          UART0_CONF2_REG
#define UART_DATA_CONF_REG      UART0_DATA_CONF_REG
#define UART_BUF_STOPBIT_REG    UART0_BUF_STOPBIT_REG

#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
static unsigned int __uidiv(unsigned int num, unsigned int den)
{
    unsigned int i;

    if (den == 1)
        return num;

    i = 1;
    while (den * i < num)
        i++;

    return i-1;
}
#endif

/************* For MACH_AK37D/MACH_AK39EV330/MACH_AK37E ****************/
#if defined(CONFIG_MACH_AK37D) || defined(CONFIG_MACH_AK39EV330) || \
    defined(CONFIG_MACH_AK37E)

static unsigned long __get_asic_pll_clk(void)
{
#ifdef CONFIG_MACH_AK37E
    unsigned long pll_m, pll_n, pll_od;
    unsigned long asic_pll_clk;
    unsigned long regval;

    regval = REG32(CLK_CORE_PLL_CTRL);
    pll_od = (regval & (0x3 << 12)) >> 12;
    pll_n = (regval & (0xf << 8)) >> 8;
    pll_m = regval & 0xff;

    asic_pll_clk = (24 * pll_m)/(pll_n * (1 << pll_od)); // clk unit: MHz
    if ((pll_od >= 1) && ((pll_n >= 1) && (pll_n <= 24))
         && ((pll_m >= 2)))
        return asic_pll_clk;

    return 0;
#endif

#if defined(CONFIG_MACH_AK37D) || defined(CONFIG_MACH_AK39EV330)
    unsigned long pll_m, pll_n, pll_od;
    unsigned long asic_pll_clk;
    unsigned long regval;

    regval = REG32(CLK_CORE_PLL_CTRL);
    pll_od = (regval & (0x3 << 12)) >> 12;
    pll_n = (regval & (0xf << 8)) >> 8;
    pll_m = regval & 0xff;

    asic_pll_clk = (12 * pll_m)/(pll_n * (1 << pll_od)); // clk unit: MHz

    if ((pll_od >= 1) && ((pll_n >= 2) && (pll_n <= 6)) 
         && ((pll_m >= 84) && (pll_m <= 254)))
        return asic_pll_clk;

    return 0;
#endif
}

/**
*@brief: get_vclk
*@param[in] void
*@return: unsigned long
**/
static unsigned long __get_vclk(void)
{
    unsigned long regval;
    unsigned long div;
    
    regval = REG32(CLK_CORE_PLL_CTRL);

#ifdef CONFIG_MACH_AK37E
    div = (regval & (0x3 << 17)) >> 17;
    return __get_asic_pll_clk()/(2*(div + 1));
#endif

#if defined(CONFIG_MACH_AK37D) || defined(CONFIG_MACH_AK39EV330)
    div = (regval & (0x7 << 17)) >> 17;
    if (div == 0)
        return __get_asic_pll_clk() >> 1;
    
    return __get_asic_pll_clk() >> div;
#endif

    return 0;
}

static unsigned long __get_asic_clk(void)
{
    unsigned long regval;
    unsigned long div;
    
    regval = REG32(CLK_CORE_PLL_CTRL);
    div = regval & (1 << 24);
    if (div == 0) 
        return __get_vclk();
    
    return __get_vclk() >> 1;
}

#endif

/***************************** For MACH_AK3918AV100 *************************/
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L) || \
    defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A)

#define MHz 1000000UL

#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
#define UART_BASE(x) (((x) < 2)?(0x20130000 + (x) * 0x8000):(0x201a0000))
#define UART_START  UART_BASE(0)
#endif

#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
/**
*@brief: get pll vclk
*@param[in] unsigned long reg
*@return: unsigned long
**/
static unsigned long ak_get_pll_clk(unsigned long reg)
{
    u32 pll_nf, pll_nr, pll_od;
    u32 pll_clk;
    u32 regval;

    regval = REG32(reg);
    pll_nf = (regval & 0x1fff);
    pll_nr = (regval >> 13) & 0x3f;
    pll_od = (regval >> 19) & 0xf;

    pll_nf++;
    pll_nr++;
    pll_od++;

    pll_clk = 24 * pll_nf /pll_nr/pll_od; // clk unit: MHz

    return pll_clk * MHz;

}

/**
*@brief: get pll2 clk
*@param[in] void
*@return: unsigned long
**/
static unsigned long get_pll2_clk(void)
{
#ifdef CONFIG_MACH_AK3918AV100_FPGA
    //FPGA use default clock 60M
    return 60;
#else
    return ak_get_pll_clk(CLK_CORE_PLL_CTRL);
#endif
}

/*
 * asic_clk is gclk.
 */ 
static unsigned long __get_asic_clk(void)
{
#ifdef CONFIG_MACH_AK3918AV100_FPGA
    /* return pll2 clk */
    return get_pll2_clk();
#else
    unsigned long regval;
    unsigned long gclk_div_num_cfg;

    regval = REG32(CLK_CORE_PLL_CTRL);
    gclk_div_num_cfg = (regval & (0x3 << 23)) >> 23;
    
    return get_pll2_clk()/(2*(gclk_div_num_cfg + 1));
#endif
}
#endif
#endif

static void uart_init(void)
{
#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
    unsigned int asic_clk, clk_div;
    /* adjust DMA priority */
    //REG32(0x21000018) = 0x00FFFFFF;

    /* enable uart clock control */
    CLK_CTRL_REG1 &= ~(0x1 << UART_CLKEN_BIT);

    /* configuration shared pins to UART0  */
    SRDPIN_CTRL1_REG |= SRDPIN_UART_RXTX_BIT;

    /* configuration uart pin pullup disable */
    PPU_PPD1_REG |= (0x1 << RXD_PU_BIT) | (0x1 << TXD_PU_BIT);

    // set uart baud rate
    asic_clk = __get_asic_clk();

    clk_div = __uidiv(asic_clk, BAUD_RATE) - 1;

#if defined(CONFIG_MACH_AK37E) || defined(CONFIG_MACH_AK3918AV100) || \
    defined(CONFIG_MACH_AK3918EV300L) || defined(CONFIG_MACH_AK3918AV130) || \
    defined(CONFIG_MACH_KM01A)
    UART_CONF1_REG &= ~((0x1 << TX_STATUS_CLR_BIT) |
        (0x1 << RX_STATUS_CLR_BIT) | 0xFFFF);
#endif

#if defined(CONFIG_MACH_AK37D) || defined(CONFIG_MACH_AK39EV330) 
    UART_CONF1_REG &= ~((0x1 << TX_STATUS_CLR_BIT) |
        (0x1 << RX_STATUS_CLR_BIT) | 0xFF);
#endif

    UART_CONF1_REG |= (0x1 << TX_STATUS_CLR_BIT) |
        (0x1 << RX_STATUS_CLR_BIT) | clk_div;

#ifdef CONFIG_UART1_INIT
    /* Disable flow control */
    UART_CONF1_REG |= (0x1 << CTS_SEL_BIT) | (0x1 << RTS_SEL_BIT);
#endif
    UART_BUF_STOPBIT_REG = (0x1F << 16) | (0x1 << 0);

    /* enable uart port */
    UART_CONF1_REG |= (0x1 << PORT_ENABLE_BIT);
#endif
}

/* print a char to uart */
static void putch(int c)
{
#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
    /* Clear uart tx buffer */
    L2BUF_CONF2_REG   |= (0x1 << UART_TXBUF_CLR_BIT);

    /* write char to uart buffer */
    REG32(UART_TXBUF_ADDR) = (unsigned long)c;
    REG32(UART_TXBUF_ADDR + ENDDING_OFFSET) = (unsigned long)'\0';
    
    /* Clear uart tx count register */
    UART_CONF1_REG |= (0x1 << TX_STATUS_CLR_BIT);

    /* Send buffer, each time only send 1 byte */
    UART_CONF2_REG |= (1 << TX_COUNT_BIT) | (0x1 << TX_COUNT_VALID_BIT);

    /* Wait for finish */
    while((UART_CONF2_REG & TX_END_MASK) == 0) {
    }
#else
    u32 value;

    REG32(UART_START + 0x0018) |= (1<<17) | (1<<7);

    value = REG32(UART_START + 0x000c) & ~(0xff << 16);
    value |= c << 16;
    REG32(UART_START + 0x000c) = value;

    /* Wait for finish */
    while((REG32(UART_START + 0x0018) & (1<<17)) == 0) {
    }
#endif
}

static inline void flush(void)
{
}

static inline void arch_decomp_setup(void)
{
    uart_init();
}

/* nothing to do */
#define arch_decomp_wdog()

/*
 * The SDK renamed the decompressor's putc() to putch() in its own
 * boot/compressed/misc.c; mainline's misc.c still calls putc().
 */
static inline void putc(int c)
{
	putch(c);
}

#endif  /* __UNCOMPRESS_H_ */
