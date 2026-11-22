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

#ifndef __DTS_HI3516EV200_CLOCK_H
#define __DTS_HI3516EV200_CLOCK_H

/*
 * Clock identifiers of the Hi3516E V200 and Hi3516E V300 CRG
 * (hisilicon,hi3516ev200-crg, hisilicon,hi3516ev300-crg) and of their
 * system controller (hisilicon,hi3516ev200-sysctrl,
 * hisilicon,hi3516ev300-sysctrl). Both chips use this header. An identifier
 * that is not provided gives an error.
 *
 * Earlier versions of this header also defined identifiers for clocks that do
 * not exist: FIXED_6M (4), FIXED_12M (5), FIXED_83P3M (9), FIXED_125M (13),
 * FIXED_148P5M (14), FIXED_250M (17), FIXED_324M (19), FIXED_342M (20),
 * FIXED_375M (21), FIXED_400M (22), FIXED_448M (23), FIXED_500M (24),
 * FIXED_540M (25), FIXED_750M (27), FIXED_1000M (28), FIXED_1500M (29),
 * MMC2_MUX (36), MMC2_CLK (52) and USB2_MUX (80). They are gone and their
 * numbers are not reused.
 *
 * The counts HI3516EV200_NR_CLKS and HI3516EV200_NR_RSTS (256 each) are gone
 * too, and so are HI3516EV300_NR_CLKS and HI3516EV300_NR_RSTS (256 each) of
 * hi3516ev300-clock.h. They were not identifiers: the driver sizes its clock
 * table itself, and a reset is named by the register offset and the bit of the
 * specifier, not by a number from these headers.
 */

/* fixed rates */
#define HI3516EV200_FIXED_100K      1
#define HI3516EV200_FIXED_400K      2
#define HI3516EV200_FIXED_3M        3
#define HI3516EV200_FIXED_24M       6
#define HI3516EV200_FIXED_25M       7
#define HI3516EV200_FIXED_50M       8
#define HI3516EV200_FIXED_90M       10
#define HI3516EV200_FIXED_100M      11
#define HI3516EV200_FIXED_112M5     12	/* 112.5 MHz */
#define HI3516EV200_FIXED_112M      HI3516EV200_FIXED_112M5
#define HI3516EV200_FIXED_150M      15
#define HI3516EV200_FIXED_200M      16
#define HI3516EV200_FIXED_300M      18
#define HI3516EV200_FIXED_600M      26
#define HI3516EV200_FIXED_54M       90
#define HI3516EV200_FIXED_360M      91
#define HI3516EV200_FIXED_450M      92

/* PLLs */
#define HI3516EV200_APLL_CLK        100
#define HI3516EV200_VPLL_CLK        101
#define HI3516EV200_APLL_VCO_CLK    118	/* VCO, divided by the module clocks */
#define HI3516EV200_VPLL_VCO_CLK    119

/* CPU, DDR and bus selects, read only */
#define HI3516EV200_A7_MUX          102
#define HI3516EV200_DDR_MUX         103
#define HI3516EV200_SYSCFG_MUX      104
#define HI3516EV200_SYSAXI_CLK      30
#define HI3516EV200_SYSAPB_CLK      31

/* module clock selects */
#define HI3516EV200_FMC_MUX         32
#define HI3516EV200_UART0_MUX       33
#define HI3516EV200_UART_MUX        HI3516EV200_UART0_MUX
#define HI3516EV200_UART1_MUX       105
#define HI3516EV200_UART2_MUX       106
#define HI3516EV200_MMC0_MUX        34
#define HI3516EV200_MMC1_MUX        35
#define HI3516EV200_ETH_MUX         37
#define HI3516EV200_ETH_CORE_MUX    HI3516EV200_ETH_MUX
#define HI3516EV200_PWM_MUX         107

/* gates */
#define HI3516EV200_UART0_CLK       40
#define HI3516EV200_UART1_CLK       41
#define HI3516EV200_UART2_CLK       42
#define HI3516EV200_FMC_CLK         43
#define HI3516EV200_ETH0_CLK        44
#define HI3516EV200_EDMAC_AXICLK    45
#define HI3516EV200_EDMAC_CLK       46
#define HI3516EV200_SPI0_CLK        48
#define HI3516EV200_SPI1_CLK        49
#define HI3516EV200_MMC0_CLK        50
#define HI3516EV200_MMC1_CLK        51
#define HI3516EV200_I2C0_CLK        53
#define HI3516EV200_I2C1_CLK        54
#define HI3516EV200_I2C2_CLK        55
#define HI3516EV200_USB2_BUS_CLK        81
#define HI3516EV200_USB2_REF_CLK        82
#define HI3516EV200_USB2_UTMI_CLK       83
#define HI3516EV200_USB2_PHY_APB_CLK    84
#define HI3516EV200_USB2_PHY_PLL_CLK    85
#define HI3516EV200_USB2_PHY_XO_CLK     86
#define HI3516EV200_PWM_CLK         110
#define HI3516EV200_OTP_CLK         111
#define HI3516EV200_LSADC_CLK       112
#define HI3516EV200_ETH_FEPHY_CLK   113
#define HI3516EV200_SPACC_CLK       114
#define HI3516EV200_KLAD_CLK        115
#define HI3516EV200_RSA_CLK         116
#define HI3516EV200_TRNG_CLK        117

/* system controller: SP804 timer count clock selects */
#define HI3516EV200_TIMER0          0
#define HI3516EV200_TIMER1          1
#define HI3516EV200_TIMER2          2
#define HI3516EV200_TIMER3          3

#endif  /* __DTS_HI3516EV200_CLOCK_H */
