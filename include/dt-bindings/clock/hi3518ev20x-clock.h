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

#ifndef __DTS_HI3518EV20X_CLOCK_H
#define __DTS_HI3518EV20X_CLOCK_H

/* clk in CRG */
/* fixed rate clocks */
#define HI3518EV20X_INNER_CLK_OFFSET 64
#define HI3518EV20X_FIXED_3M        65
#define HI3518EV20X_FIXED_6M        66
#define HI3518EV20X_FIXED_24M       67
#define HI3518EV20X_FIXED_25M       68
#define HI3518EV20X_FIXED_27M       69
#define HI3518EV20X_FIXED_37P125M   70
#define HI3518EV20X_FIXED_49P5M     71
#define HI3518EV20X_FIXED_50M       72
#define HI3518EV20X_FIXED_54M       73
#define HI3518EV20X_FIXED_74P25M    74
#define HI3518EV20X_FIXED_99M       75
#define HI3518EV20X_FIXED_125M      76
#define HI3518EV20X_FIXED_148P5M	77
#define HI3518EV20X_FIXED_198M      78
#define HI3518EV20X_FIXED_200M      79
#define HI3518EV20X_FIXED_250M      80
#define HI3518EV20X_FIXED_297M      81
#define HI3518EV20X_FIXED_300M      82
#define HI3518EV20X_FIXED_396M      83
#define HI3518EV20X_FIXED_540M      84
#define HI3518EV20X_FIXED_594M      85
#define HI3518EV20X_FIXED_600M      86
#define HI3518EV20X_FIXED_650M      87
#define HI3518EV20X_FIXED_750M      88
#define HI3518EV20X_FIXED_1188M     89

/* mux clocks */
#define HI3518EV20X_SYSAPB_CLK		0
#define HI3518EV20X_FMC_MUX			1
#define HI3518EV20X_UART_MUX        2
#define HI3518EV20X_ETH_MUX			3
#define HI3518EV20X_USB2_CTRL_UTMI0_REQ	4
#define HI3518EV20X_USB2_HRST_REQ	5
#define HI3518EV20X_MMC0_MUX		6
#define HI3518EV20X_MMC1_MUX		7

/* gate clocks */
#define HI3518EV20X_FMC_CLK			20
#define HI3518EV20X_UART0_CLK		21
#define HI3518EV20X_UART1_CLK		22
#define HI3518EV20X_UART2_CLK		23
#define HI3518EV20X_UART3_CLK		24
#define HI3518EV20X_ETH_CLK			25
#define HI3518EV20X_MMC0_CLK		26
#define HI3518EV20X_MMC1_CLK		27
#define HI3518EV20X_SPI0_CLK		28
#define HI3518EV20X_SPI1_CLK		29
#define HI3518EV20X_DMAC_CLK		30

#define HI3518EV20X_CRG_NR_CLKS     128
#define HI3518EV20X_CRG_NR_RSTS     128

/* clock in system control */
/* mux clocks */
#define HI3518EV20X_TIME0_0_CLK     1
#define HI3518EV20X_TIME0_1_CLK     2
#define HI3518EV20X_TIME1_2_CLK     3
#define HI3518EV20X_TIME1_3_CLK     4

#define HI3518EV20X_SC_NR_CLKS     4

#endif  /* __DTS_HI3518EV20X_CLOCK_H */
