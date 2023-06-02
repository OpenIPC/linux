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

#ifndef __DTS_HI3531D_CLOCK_H
#define __DTS_HI3531D_CLOCK_H

/* clk in hi3531D CRG */
/* fixed rate clocks */
#define HI3531D_FIXED_3M		1
#define HI3531D_FIXED_6M		2
#define HI3531D_FIXED_12M		3
#define HI3531D_FIXED_15M		4
#define HI3531D_FIXED_24M		5
#define HI3531D_FIXED_44M		6
#define HI3531D_FIXED_49P5		7
#define HI3531D_FIXED_50M		8
#define HI3531D_FIXED_54M		9
#define HI3531D_FIXED_75M		10
#define HI3531D_FIXED_83P3M		11
#define HI3531D_FIXED_99M		12
#define HI3531D_FIXED_100M		13
#define HI3531D_FIXED_125M		14
#define HI3531D_FIXED_148P5M	15
#define HI3531D_FIXED_150M		16
#define HI3531D_FIXED_200M		17
#define HI3531D_FIXED_250M		18
#define HI3531D_FIXED_300M		19
#define HI3531D_FIXED_324M		20
#define HI3531D_FIXED_342M		21
#define HI3531D_FIXED_375M		22
#define HI3531D_FIXED_400M		23
#define HI3531D_FIXED_448M		24
#define HI3531D_FIXED_500M		25
#define HI3531D_FIXED_540M		26
#define HI3531D_FIXED_600M		27
#define HI3531D_FIXED_750M		28
#define HI3531D_FIXED_1500M		29

/* mux clocks */
#define HI3531D_PERIAXI_CLK		30
#define HI3531D_SYSAXI_CLK		31
#define HI3531D_NFC_MUX			32
#define HI3531D_FMC_MUX			33
#define HI3531D_UART_MUX		34
#define HI3531D_VIU_MUX		    58

/* gate clocks */
#define HI3531D_UART0_CLK		35
#define HI3531D_UART1_CLK		36
#define HI3531D_UART2_CLK		37
#define HI3531D_UART3_CLK		38
#define HI3531D_UART4_CLK		39
#define HI3531D_SPI0_CLK		40
#define HI3531D_I2C0_CLK		41
#define HI3531D_I2C1_CLK		42
#define HI3531D_NFC_CLK			43
#define HI3531D_FMC_CLK			44
#define HI3531D_ETH_CLK			50
#define HI3531D_ETH_MACIF_CLK	51
#define HI3531D_USB2_BUS_CLK	52
#define HI3531D_UTMI0_CLK		53
#define HI3531D_USB2_CLK		54
#define HI3531D_DMAC_CLK		55
#define HI3531D_SATA_CLK		56
#define HI3531D_VIU_CLK         57
/* #define HI3531D_VIU_MUX		58 */

#define HI3531D_CRG_NR_CLKS		60
#define HI3531D_CRG_NR_RSTS		0x250

/* clock in system control */
/* mux clocks */
#define HI3531D_TIME0_0_CLK		1
#define HI3531D_TIME0_1_CLK		2
#define HI3531D_TIME1_2_CLK		3
#define HI3531D_TIME1_3_CLK		4
#define HI3531D_TIME2_4_CLK		5
#define HI3531D_TIME2_5_CLK		6
#define HI3531D_TIME3_6_CLK		7
#define HI3531D_TIME3_7_CLK		8

#define HI3531D_SYS_NR_CLKS		10
#define HI3531D_SYS_NR_RSTS		0x10
#endif	/* __DTS_HI3531D_CLOCK_H */
