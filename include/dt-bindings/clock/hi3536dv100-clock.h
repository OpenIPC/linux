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

#ifndef __DTS_HI3536DV100_CLOCK_H
#define __DTS_HI3536DV100_CLOCK_H

/* clk in Hi3536D CRG */
/* fixed rate clocks */
#define HI3536DV100_FIXED_3M        1
#define HI3536DV100_FIXED_6M        2
#define HI3536DV100_FIXED_12M       3
#define HI3536DV100_FIXED_24M       4
#define HI3536DV100_FIXED_50M       5
#define HI3536DV100_FIXED_83P3M     6
#define HI3536DV100_FIXED_100M      7
#define HI3536DV100_FIXED_125M      8
#define HI3536DV100_FIXED_148P5M    9
#define HI3536DV100_FIXED_150M      10
#define HI3536DV100_FIXED_200M      11
#define HI3536DV100_FIXED_250M      12
#define HI3536DV100_FIXED_300M      13
#define HI3536DV100_FIXED_324M      14
#define HI3536DV100_FIXED_342M      15
#define HI3536DV100_FIXED_375M      16
#define HI3536DV100_FIXED_400M      17
#define HI3536DV100_FIXED_448M      18
#define HI3536DV100_FIXED_500M      19
#define HI3536DV100_FIXED_540M      20
#define HI3536DV100_FIXED_600M      21
#define HI3536DV100_FIXED_750M      22
#define HI3536DV100_FIXED_1500M     23

/* mux clocks */
#define HI3536DV100_SYSAXI_CLK      24
#define HI3536DV100_SYSAPB_CLK      25
#define HI3536DV100_FMC_MUX         26
#define HI3536DV100_UART_MUX        27

/* gate clocks */
#define HI3536DV100_UART0_CLK       28
#define HI3536DV100_UART1_CLK       29
#define HI3536DV100_UART2_CLK       30
#define HI3536DV100_FMC_CLK         31
#define HI3536DV100_ETH0_CLK        32
#define HI3536DV100_ETH0_PHY_CLK    33
#define HI3536DV100_USB2_BUS_CLK    34
#define HI3536DV100_USB2_CLK        35
#define HI3536DV100_SATA_CLK        36
#define HI3536DV100_DMAC_CLK        37

#define HI3536DV100_CRG_NR_CLKS     40
#define HI3536DV100_CRG_NR_RSTS     0x200

/* clock in system control */
/* mux clocks */
#define HI3536DV100_TIME0_0_CLK     1
#define HI3536DV100_TIME0_1_CLK     2
#define HI3536DV100_TIME1_2_CLK     3
#define HI3536DV100_TIME1_3_CLK     4
#define HI3536DV100_TIME2_4_CLK     5
#define HI3536DV100_TIME2_5_CLK     6
#define HI3536DV100_TIME3_6_CLK     7
#define HI3536DV100_TIME3_7_CLK     8

#define HI3536DV100_SC_NR_CLKS      10

#endif  /* __DTS_HI3536DV100_CLOCK_H */
