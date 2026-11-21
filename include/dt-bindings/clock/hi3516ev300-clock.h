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

#ifndef __DTS_HI3516EV300_CLOCK_H
#define __DTS_HI3516EV300_CLOCK_H

/*
 * The Hi3516E V300 uses the clock identifiers of the Hi3516E V200 header,
 * hi3516ev200-clock.h. These are the names this header used to define for
 * clocks that still exist, kept so that device trees written with them still
 * build. The names of clocks that do not exist (fixed rates that no select
 * uses, the third MMC clock and select, the USB select) and the table sizes
 * are gone and their numbers are not reused. Do not use these names in new
 * device trees.
 */
#include <dt-bindings/clock/hi3516ev200-clock.h>

#define HI3516EV300_FIXED_100K          HI3516EV200_FIXED_100K
#define HI3516EV300_FIXED_400K          HI3516EV200_FIXED_400K
#define HI3516EV300_FIXED_3M            HI3516EV200_FIXED_3M
#define HI3516EV300_FIXED_24M           HI3516EV200_FIXED_24M
#define HI3516EV300_FIXED_25M           HI3516EV200_FIXED_25M
#define HI3516EV300_FIXED_50M           HI3516EV200_FIXED_50M
#define HI3516EV300_FIXED_90M           HI3516EV200_FIXED_90M
#define HI3516EV300_FIXED_100M          HI3516EV200_FIXED_100M
#define HI3516EV300_FIXED_112M          HI3516EV200_FIXED_112M
#define HI3516EV300_FIXED_150M          HI3516EV200_FIXED_150M
#define HI3516EV300_FIXED_200M          HI3516EV200_FIXED_200M
#define HI3516EV300_FIXED_300M          HI3516EV200_FIXED_300M
#define HI3516EV300_FIXED_600M          HI3516EV200_FIXED_600M
#define HI3516EV300_SYSAXI_CLK          HI3516EV200_SYSAXI_CLK
#define HI3516EV300_SYSAPB_CLK          HI3516EV200_SYSAPB_CLK
#define HI3516EV300_FMC_MUX             HI3516EV200_FMC_MUX
#define HI3516EV300_UART_MUX            HI3516EV200_UART_MUX
#define HI3516EV300_MMC0_MUX            HI3516EV200_MMC0_MUX
#define HI3516EV300_MMC1_MUX            HI3516EV200_MMC1_MUX
#define HI3516EV300_ETH_MUX             HI3516EV200_ETH_MUX
#define HI3516EV300_UART0_CLK           HI3516EV200_UART0_CLK
#define HI3516EV300_UART1_CLK           HI3516EV200_UART1_CLK
#define HI3516EV300_UART2_CLK           HI3516EV200_UART2_CLK
#define HI3516EV300_FMC_CLK             HI3516EV200_FMC_CLK
#define HI3516EV300_ETH0_CLK            HI3516EV200_ETH0_CLK
#define HI3516EV300_EDMAC_AXICLK        HI3516EV200_EDMAC_AXICLK
#define HI3516EV300_EDMAC_CLK           HI3516EV200_EDMAC_CLK
#define HI3516EV300_SPI0_CLK            HI3516EV200_SPI0_CLK
#define HI3516EV300_SPI1_CLK            HI3516EV200_SPI1_CLK
#define HI3516EV300_MMC0_CLK            HI3516EV200_MMC0_CLK
#define HI3516EV300_MMC1_CLK            HI3516EV200_MMC1_CLK
#define HI3516EV300_I2C0_CLK            HI3516EV200_I2C0_CLK
#define HI3516EV300_I2C1_CLK            HI3516EV200_I2C1_CLK
#define HI3516EV300_I2C2_CLK            HI3516EV200_I2C2_CLK
#define HI3516EV300_USB2_BUS_CLK        HI3516EV200_USB2_BUS_CLK
#define HI3516EV300_USB2_REF_CLK        HI3516EV200_USB2_REF_CLK
#define HI3516EV300_USB2_UTMI_CLK       HI3516EV200_USB2_UTMI_CLK
#define HI3516EV300_USB2_PHY_APB_CLK    HI3516EV200_USB2_PHY_APB_CLK
#define HI3516EV300_USB2_PHY_PLL_CLK    HI3516EV200_USB2_PHY_PLL_CLK
#define HI3516EV300_USB2_PHY_XO_CLK     HI3516EV200_USB2_PHY_XO_CLK

#endif  /* __DTS_HI3516EV300_CLOCK_H */
