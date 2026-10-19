/*
 * AK3918AV130 keeps the vendor's own copy of this header, from the
 * AnyCloud39AV130 SDK, in mach/av130/: its register map and IRQ layout
 * share nothing with EV200/EV330 beyond the names.
 */
#ifdef CONFIG_MACH_AK3918AV130
#include <mach/av130/uncompress.h>
#else
/*
 * linux/arch/arm/mach-ak39/include/mach/uncompress.h
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

#define BAUD_RATE  					115200
#define ENDDING_OFFSET				60

#undef REG32
#define REG32(_reg)             	(*(volatile unsigned long *)(_reg))

#define CLK_CORE_PLL_CTRL			(AK_PA_SYSCTRL + 0x08)

/* L2 buffer address */
#define UART0_TXBUF_ADDR			(0x48000000 + 0x1000) //0x48001000
#define UART0_RXBUF_ADDR        	(0x48000000 + 0x1080)

/* L2 buffer control register */
#define L2BUF_CONF2_REG         	REG32(0x20140000 + 0x008C) //0x2014008c
#define UART0_TXBUF_CLR_BIT     	16
#define UART0_RXBUF_CLR_BIT     	17


#if defined(CONFIG_MACH_AK37D)
/* pullup/pulldown configure registers */
#define PPU_PPD1_REG           	REG32(AK_PA_SYSCTRL + 0x194) //0x08000194
#define TXD0_PU_BIT             1
#define RXD0_PU_BIT             0
/*********** Shared pin control reigsters ********/
#define SRDPIN_CTRL1_REG     	REG32(AK_PA_SYSCTRL + 0x178) //0x08000178
#define UART0_RXD				0
#define UART0_TXD				2

#elif defined(CONFIG_MACH_AK39EV330)
/* pullup/pulldown configure registers */
#define PPU_PPD1_REG           	REG32(AK_PA_SYSCTRL + 0x80)  //0x08000080
#define TXD0_PU_BIT             2
#define RXD0_PU_BIT             1
/*********** Shared pin control reigsters ********/
#define SRDPIN_CTRL1_REG     	REG32(AK_PA_SYSCTRL + 0x74) //0x08000074
#define UART0_RXD				2
#define UART0_TXD				3

#elif defined(CONFIG_MACH_AK3918)
/*
 * Aimer39 AK3918/AK3916: the console pair is SHAREPIN_CON1 bits 14 and 15,
 * pulled up through PULL_REG0 bits 19 and 20 (the Aimer39 SDK's
 * mach-ak39/include/mach/uncompress.h).
 */
#define PPU_PPD1_REG           	REG32(AK_PA_SYSCTRL + 0x80)  //0x08000080
#define TXD0_PU_BIT             20
#define RXD0_PU_BIT             19
/*********** Shared pin control reigsters ********/
#define SRDPIN_CTRL1_REG     	REG32(AK_PA_SYSCTRL + 0x74) //0x08000074
#define UART0_RXD				14
#define UART0_TXD				15

#elif defined(CONFIG_MACH_AK3918EV200)
/*
 * UART0 console pads are 1 and 2. PULL_REG0 (== PPU_PPD1_REG) is
 * pad-indexed: pad1 -> bit1, pad2 -> bit2.
 */
#define PPU_PPD1_REG           	REG32(AK_PA_SYSCTRL + 0x80)  //0x08000080
#define TXD0_PU_BIT             2
#define RXD0_PU_BIT             1
/*********** Shared pin control reigsters ********/
#define SRDPIN_CTRL1_REG     	REG32(AK_PA_SYSCTRL + 0x74) //0x08000074
#define UART0_RXD				1
#define UART0_TXD				2

#elif defined(CONFIG_MACH_AK37E)
/* pullup/pulldown configure registers */
#define PPU_PPD1_REG           	REG32(AK_PA_SYSCTRL + 0x1E4) //0x080001E4
#define TXD0_PU_BIT             23
#define RXD0_PU_BIT             22
/*********** Shared pin control reigsters ********/
#define SRDPIN_CTRL1_REG     	REG32(AK_PA_SYSCTRL + 0x164) //0x08000164
#define UART0_RXD				6
#define UART0_TXD				9

#endif

/* Clock control register */
#define CLK_CTRL_REG1			REG32(AK_PA_SYSCTRL + 0x1C) //0x0800001C
#define UART0_CLKEN_BIT			7	//0x0800,001C


/** ************ UART registers *****************************/
#define UART0_CONF1_REG			REG32(0x20130000 + 0x00) //0x20130000
#define UART0_CONF2_REG			REG32(0x20130000 + 0x04)
#define UART0_DATA_CONF_REG		REG32(0x20130000 + 0x08)
#define UART0_BUF_THRE_REG		REG32(0x20130000 + 0x0C)
#define UART0_BUF_RX_REG		REG32(0x20130000 + 0x10)
#define UART0_BUF_RX_BACKUP_REG	REG32(0x20130000 + 0x14)
#define UART0_BUF_STOPBIT_REG	REG32(0x20130000 + 0x18)

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
#define SRDPIN_UART_RXTX_BIT    ((1 << UART0_RXD)|(1 << UART0_TXD)) // GPIO 1&2, 0x0800,0074 bit[1]&bit[2] == 1&1
#define RXD_PU_BIT              RXD0_PU_BIT
#define TXD_PU_BIT              TXD0_PU_BIT
#define UART_CLKEN_BIT			UART0_CLKEN_BIT
#define UART_TXBUF_ADDR         UART0_TXBUF_ADDR
#define UART_CONF1_REG          UART0_CONF1_REG
#define UART_CONF2_REG          UART0_CONF2_REG
#define UART_DATA_CONF_REG      UART0_DATA_CONF_REG
#define UART_BUF_STOPBIT_REG	UART0_BUF_STOPBIT_REG

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

static unsigned long __get_asic_pll_clk(void)
{
	unsigned long pll_m, pll_n, pll_od;
	unsigned long asic_pll_clk;
	unsigned long regval;

	regval = REG32(CLK_CORE_PLL_CTRL);
	pll_od = (regval & (0x3 << 12)) >> 12;
	pll_n = (regval & (0xf << 8)) >> 8;
	pll_m = regval & 0xff;

#ifdef CONFIG_MACH_AK37E
	asic_pll_clk = (24 * pll_m)/(pll_n * (1 << pll_od)); // clk unit: MHz
	if ((pll_od >= 1) && ((pll_n >= 1) && (pll_n <= 24))
		 && ((pll_m >= 2)))
		return asic_pll_clk;
#endif

#if defined(CONFIG_MACH_AK37D) || defined(CONFIG_MACH_AK39EV330) || defined(CONFIG_AK3918_FAMILY)
	asic_pll_clk = (12 * pll_m)/(pll_n * (1 << pll_od)); // clk unit: MHz

	if ((pll_od >= 1) && ((pll_n >= 2) && (pll_n <= 6)) 
		 && ((pll_m >= 84) && (pll_m <= 254)))
		return asic_pll_clk;
#endif

	return 0;
}

static unsigned long __get_vclk(void)
{
	unsigned long regval;
	unsigned long div;
	
	regval = REG32(CLK_CORE_PLL_CTRL);

#ifdef CONFIG_MACH_AK37E
	div = (regval & (0x3 << 17)) >> 17;
	return __get_asic_pll_clk()/(2*(div + 1));
#endif

#if defined(CONFIG_MACH_AK37D) || defined(CONFIG_MACH_AK39EV330) || defined(CONFIG_AK3918_FAMILY)
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

static void uart_init(void)
{
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
	asic_clk = __get_asic_clk()*1000000;
	clk_div = __uidiv(asic_clk, BAUD_RATE) - 1;
#ifdef CONFIG_MACH_AK37E
	UART_CONF1_REG &= ~((0x1 << TX_STATUS_CLR_BIT) | (0x1 << RX_STATUS_CLR_BIT) | 0xFFFF);
#endif
#if defined(CONFIG_MACH_AK37D) || defined(CONFIG_MACH_AK39EV330) || defined(CONFIG_AK3918_FAMILY)
	UART_CONF1_REG &= ~((0x1 << TX_STATUS_CLR_BIT) | (0x1 << RX_STATUS_CLR_BIT) | 0xFF);
#endif
	UART_CONF1_REG |= (0x1 << TX_STATUS_CLR_BIT) | (0x1 << RX_STATUS_CLR_BIT) | clk_div;
	
#ifdef CONFIG_UART1_INIT
	/* Disable flow control */
	UART_CONF1_REG |= (0x1 << CTS_SEL_BIT) | (0x1 << RTS_SEL_BIT);
#endif
	UART_BUF_STOPBIT_REG = (0x1F << 16) | (0x1 << 0);

	/* enable uart port */
	UART_CONF1_REG |= (0x1 << PORT_ENABLE_BIT);
}


/* print a char to uart */
void putc(char c)
{
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

#endif   /* __UNCOMPRESS_H_ */
#endif /* CONFIG_MACH_AK3918AV130 */
