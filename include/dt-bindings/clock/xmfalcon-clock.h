/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __DTS_XMFALCON_CLOCK_H
#define __DTS_XMFALCON_CLOCK_H

/* fixed rate clocks */
#define XMFALCON_FIXED_400K          1
#define XMFALCON_FIXED_1600K         2
#define XMFALCON_FIXED_3M            3
#define XMFALCON_FIXED_24M           4
#define XMFALCON_FIXED_25M           5
#define XMFALCON_FIXED_50M           6
#define XMFALCON_FIXED_90M           7
#define XMFALCON_FIXED_99M           8
#define XMFALCON_FIXED_100M          9
#define XMFALCON_FIXED_112M          10
#define XMFALCON_FIXED_148P5M        11
#define XMFALCON_FIXED_150M          12
#define XMFALCON_FIXED_198M          13
#define XMFALCON_FIXED_200M          14
#define XMFALCON_FIXED_297M          15
#define XMFALCON_FIXED_300M          16
#define XMFALCON_FIXED_396M          17
#define XMFALCON_FIXED_458M          18
#define XMFALCON_FIXED_594M          19
#define XMFALCON_FIXED_792M          20
#define XMFALCON_FIXED_917M          21
#define XMFALCON_FIXED_1188M         22
#define XMFALCON_FIXED_1375M         23
#define XMFALCON_FIXED_1500M         24
#define XMFALCON_FIXED_1000M         25

/* mux clocks */
#define XMFALCON_SYSAPB_CLK          40
#define XMFALCON_FMC_MUX             41
#define XMFALCON_UART0_MUX           42
#define XMFALCON_UART1_MUX           43
#define XMFALCON_UART2_MUX           44
#define XMFALCON_UART3_MUX           45
#define XMFALCON_UART4_MUX           46
#define XMFALCON_UART5_MUX           47
#define XMFALCON_UART_MUX            48
#define XMFALCON_MMC0_MUX            49
#define XMFALCON_MMC1_MUX            50
#define XMFALCON_MMC2_MUX            51
#define XMFALCON_ETH_MUX             52
#define XMFALCON_CPU_MUX             53
#define XMFALCON_USB2_MUX            54

/* gate clocks */
#define XMFALCON_UART0_CLK           70
#define XMFALCON_UART1_CLK           71
#define XMFALCON_UART2_CLK           72
#define XMFALCON_FMC_CLK             73
#define XMFALCON_ETH0_CLK            74
#define XMFALCON_EDMAC_AXICLK        75
#define XMFALCON_EDMAC_CLK           76
#define XMFALCON_SPI0_CLK            77
#define XMFALCON_SPI1_CLK            78
#define XMFALCON_MMC0_CLK            79
#define XMFALCON_MMC1_CLK            80
#define XMFALCON_MMC2_CLK            81
#define XMFALCON_I2C0_CLK            82
#define XMFALCON_I2C1_CLK            83
#define XMFALCON_I2C2_CLK            84
#define XMFALCON_I2C3_CLK            85
#define XMFALCON_I2C4_CLK            86
#define XMFALCON_I2C5_CLK            87
#define XMFALCON_I2C6_CLK            88
#define XMFALCON_I2C7_CLK            89
#define XMFALCON_SPI2_CLK            90
#define XMFALCON_SPI3_CLK            91
#define XMFALCON_UART3_CLK           92
#define XMFALCON_UART4_CLK           93
#define XMFALCON_USB2_BUS_CLK        94
#define XMFALCON_USB2_REF_CLK        95
#define XMFALCON_USB2_UTMI_CLK       96
#define XMFALCON_LOTUS_PHY_APB_CLK   97
#define XMFALCON_LOTUS_PHY_REF_CLK   98
#define XMFALCON_USB2_PHY_XO_CLK     99

#define XMFALCON_NR_CLKS         256
#define XMFALCON_NR_RSTS         256

#endif  /* __DTS_XMFALCON_CLOCK_H */
