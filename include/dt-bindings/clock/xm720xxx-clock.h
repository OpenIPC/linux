/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef __DTS_XM720XXX_CLOCK_H
#define __DTS_XM720XXX_CLOCK_H

/* clk in XM720XXX CRG */
/* fixed rate clocks */
#define XM720XXX_FIXED_100K      1
#define XM720XXX_FIXED_400K      2
#define XM720XXX_FIXED_3M        3
#define XM720XXX_FIXED_6M        4
#define XM720XXX_FIXED_12M       5
#define XM720XXX_FIXED_24M       6
#define XM720XXX_FIXED_25M       7
#define XM720XXX_FIXED_50M       8
#define XM720XXX_FIXED_83P3M     9
#define XM720XXX_FIXED_90M       10
#define XM720XXX_FIXED_100M      11
#define XM720XXX_FIXED_112M      12
#define XM720XXX_FIXED_125M      13
#define XM720XXX_FIXED_148P5M    14
#define XM720XXX_FIXED_150M      15
#define XM720XXX_FIXED_200M      16
#define XM720XXX_FIXED_250M      17
#define XM720XXX_FIXED_300M      18
#define XM720XXX_FIXED_324M      19
#define XM720XXX_FIXED_342M      20
#define XM720XXX_FIXED_375M      21
#define XM720XXX_FIXED_400M      22
#define XM720XXX_FIXED_448M      23
#define XM720XXX_FIXED_500M      24
#define XM720XXX_FIXED_540M      25
#define XM720XXX_FIXED_600M      26
#define XM720XXX_FIXED_750M      27
#define XM720XXX_FIXED_1000M     28
#define XM720XXX_FIXED_1500M     29

/* mux clocks */
#define XM720XXX_SYSAXI_CLK      30
#define XM720XXX_SYSAPB_CLK      31
#define XM720XXX_FMC_MUX         32
#define XM720XXX_UART_MUX        33
#define XM720XXX_MMC0_MUX        34
#define XM720XXX_MMC1_MUX        35
#define XM720XXX_MMC2_MUX        36
#define XM720XXX_ETH_MUX         37
#define XM720XXX_USB2_MUX        80
/* gate clocks */
#define XM720XXX_UART0_CLK       40
#define XM720XXX_UART1_CLK       41
#define XM720XXX_UART2_CLK       42
#define XM720XXX_FMC_CLK         43
#define XM720XXX_ETH0_CLK        44
#define XM720XXX_EDMAC_AXICLK    45
#define XM720XXX_EDMAC_CLK       46
#define XM720XXX_SPI0_CLK        48
#define XM720XXX_SPI1_CLK        49
#define XM720XXX_MMC0_CLK        50
#define XM720XXX_MMC1_CLK        51
#define XM720XXX_MMC2_CLK        52
#define XM720XXX_I2C0_CLK        53
#define XM720XXX_I2C1_CLK        54
#define XM720XXX_I2C2_CLK        55
#define XM720XXX_USB2_BUS_CLK        81
#define XM720XXX_USB2_REF_CLK        82
#define XM720XXX_USB2_UTMI_CLK       83
#define XM720XXX_USB2_PHY_APB_CLK    84
#define XM720XXX_USB2_PHY_PLL_CLK    85
#define XM720XXX_USB2_PHY_XO_CLK     86

#define XM720XXX_NR_CLKS         256
#define XM720XXX_NR_RSTS         256

#endif  /* __DTS_XM720XXX_CLOCK_H */
