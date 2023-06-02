/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __DTS_HI3516A_CLOCK_H
#define __DTS_HI3516A_CLOCK_H

/* clk in CRG */
/* fixed rate clocks */
#define HI3516A_INNER_CLK_OFFSET 64
#define HI3516A_FIXED_3M        65
#define HI3516A_FIXED_6M        66
#define HI3516A_FIXED_13P5M     67
#define HI3516A_FIXED_24M       68
#define HI3516A_FIXED_25M       69
#define HI3516A_FIXED_27M       70
#define HI3516A_FIXED_37P125M   71
#define HI3516A_FIXED_50M       72
#define HI3516A_FIXED_74P25M    73
#define HI3516A_FIXED_75M       74
#define HI3516A_FIXED_99M       75
#define HI3516A_FIXED_100M      76
#define HI3516A_FIXED_125M      77
#define HI3516A_FIXED_145M      78
#define HI3516A_FIXED_148P5M    79
#define HI3516A_FIXED_150M      80
#define HI3516A_FIXED_194M      81
#define HI3516A_FIXED_198M      82
#define HI3516A_FIXED_200M      83
#define HI3516A_FIXED_229M      84
#define HI3516A_FIXED_237M      85
#define HI3516A_FIXED_242M      86
#define HI3516A_FIXED_250M      87
#define HI3516A_FIXED_297M      88
#define HI3516A_FIXED_300M      89
#define HI3516A_FIXED_333M      90
#define HI3516A_FIXED_400M      91
#define HI3516A_FIXED_500M      92
#define HI3516A_FIXED_594M      93
#define HI3516A_FIXED_600M      94
#define HI3516A_FIXED_726P25M   95
#define HI3516A_FIXED_750M      96
#define HI3516A_FIXED_900M      97
#define HI3516A_FIXED_1000M     98
#define HI3516A_FIXED_1188M     99

/* mux clocks */
#define HI3516A_SYSAXI_CLK      0
#define HI3516A_SNOR_MUX        1
#define HI3516A_SNAND_MUX       2
#define HI3516A_NAND_MUX        3
#define HI3516A_UART_MUX        4
#define HI3516A_ETH_PHY_MUX     5
#define HI3516A_A7_MUX          6
#define HI3516A_MMC0_MUX        7
#define HI3516A_MMC1_MUX        8
#define HI3516A_USB2_CTRL_UTMI0_REQ     9
#define HI3516A_USB2_HRST_REQ           10

/* gate clocks */
#define HI3516A_SNOR_CLK        15
#define HI3516A_SNAND_CLK       16
#define HI3516A_NAND_CLK        17
#define HI3516A_UART0_CLK       18
#define HI3516A_UART1_CLK       19
#define HI3516A_UART2_CLK       20
#define HI3516A_UART3_CLK       21
#define HI3516A_ETH_CLK         22
#define HI3516A_ETH_MACIF_CLK   23
#define HI3516A_MMC0_CLK        24
#define HI3516A_MMC1_CLK        25
#define HI3516A_SPI0_CLK        26
#define HI3516A_SPI1_CLK        27
#define HI3516A_DMAC_CLK        28

/* pll clock */
#define HI3516A_APLL_CLK        30

#define HI3516A_CRG_NR_CLKS     128
#define HI3516A_CRG_NR_RSTS     0x12c

/* clock in system control */
/* mux clocks */
#define HI3516A_TIME0_0_CLK     1
#define HI3516A_TIME0_1_CLK     2
#define HI3516A_TIME1_2_CLK     3
#define HI3516A_TIME1_3_CLK     4

#define HI3516A_SYS_NR_CLKS     10

#endif  /* __DTS_HI3516A_CLOCK_H */
