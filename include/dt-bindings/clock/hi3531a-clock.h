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

#ifndef __DTS_HI3531A_CLOCK_H
#define __DTS_HI3531A_CLOCK_H

#include <dt-bindings/interrupt-controller/irq.h>

/* interrupt specifier cell 0 */
#define GIC_SPI 0
#define GIC_PPI 1

/* clk in hi3531A CRG */
/* fixed rate clocks */
#define HI3531A_INNER_CLK_OFFSET    64
#define HI3531A_FIXED_2M            65
#define HI3531A_FIXED_2P02M         66
#define HI3531A_FIXED_2P5M          67
#define HI3531A_FIXED_3M            68
#define HI3531A_FIXED_24M           69
#define HI3531A_FIXED_25M           70
#define HI3531A_FIXED_27M           71
#define HI3531A_FIXED_37P125M       72
#define HI3531A_FIXED_37P5M         73
#define HI3531A_FIXED_40P5M         74
#define HI3531A_FIXED_48M           75
#define HI3531A_FIXED_50M           76
#define HI3531A_FIXED_54M           77
#define HI3531A_FIXED_59P2M         78
#define HI3531A_FIXED_60M           79
#define HI3531A_FIXED_62P5M         80
#define HI3531A_FIXED_74P25M        81
#define HI3531A_FIXED_75M           82
#define HI3531A_FIXED_83P3M         83
#define HI3531A_FIXED_100M          84
#define HI3531A_FIXED_125M          85
#define HI3531A_FIXED_150M          86
#define HI3531A_FIXED_187P5M        87
#define HI3531A_FIXED_200M          88
#define HI3531A_FIXED_250M          89
#define HI3531A_FIXED_300M          90
#define HI3531A_FIXED_324M          91
#define HI3531A_FIXED_355M          92
#define HI3531A_FIXED_400M          93
#define HI3531A_FIXED_433M          94
#define HI3531A_FIXED_500M          95
#define HI3531A_FIXED_750M          96
#define HI3531A_FIXED_800M          97
#define HI3531A_FIXED_1000M         98
#define HI3531A_FIXED_1420M         99
#define HI3531A_FIXED_1500M         100

/* mux clocks */
#define HI3531A_PERIAXI_CLK         0
#define HI3531A_SYSAXI_CLK          1
#define HI3531A_NFC_MUX             2
#define HI3531A_FMC_MUX             3
#define HI3531A_UART_MUX            4
#define HI3531A_ETH_PHY_MUX         5

/* gate clocks */
#define HI3531A_UART0_CLK           20
#define HI3531A_UART1_CLK           21
#define HI3531A_UART2_CLK           22
#define HI3531A_UART3_CLK           23
#define HI3531A_UART4_CLK           24
#define HI3531A_NFC_CLK             25
#define HI3531A_FMC_CLK             26
#define HI3531A_ETH_CLK             27
#define HI3531A_ETH_MACIF_CLK       28
#define HI3531A_SPI0_CLK            29
#define HI3531A_DMAC_CLK            30

#define HI3531A_CRG_NR_CLKS         60
#define HI3531A_CRG_NR_RSTS         0x250

/* clock in system control */
/* mux clocks */
#define HI3531A_TIME0_0_CLK         1
#define HI3531A_TIME0_1_CLK         2
#define HI3531A_TIME1_2_CLK         3
#define HI3531A_TIME1_3_CLK         4
#define HI3531A_TIME2_4_CLK         5
#define HI3531A_TIME2_5_CLK         6
#define HI3531A_TIME3_6_CLK         7
#define HI3531A_TIME3_7_CLK         8

#define HI3531A_SYS_NR_CLKS     10

#endif  /* __DTS_HI3531A_CLOCK_H */
