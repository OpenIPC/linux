/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __DTS_XMORCA_CLOCK_H
#define __DTS_XMORCA_CLOCK_H

/* fixed rate clocks */
#define XMORCA_FIXED_400K          1
#define XMORCA_FIXED_1600K         2
#define XMORCA_FIXED_3M            3
#define XMORCA_FIXED_24M           4
#define XMORCA_FIXED_25M           5
#define XMORCA_FIXED_50M           6
#define XMORCA_FIXED_74M           7
#define XMORCA_FIXED_90M           8
#define XMORCA_FIXED_99M           9
#define XMORCA_FIXED_100M          10
#define XMORCA_FIXED_112M          11
#define XMORCA_FIXED_148P5M        12
#define XMORCA_FIXED_150M          13
#define XMORCA_FIXED_198M          14
#define XMORCA_FIXED_200M          15
#define XMORCA_FIXED_297M          16
#define XMORCA_FIXED_300M          17
#define XMORCA_FIXED_396M          18
#define XMORCA_FIXED_458M          19
#define XMORCA_FIXED_594M          20
#define XMORCA_FIXED_792M          21
#define XMORCA_FIXED_917M          22
#define XMORCA_FIXED_1188M         23
#define XMORCA_FIXED_1375M         24
#define XMORCA_FIXED_1500M         25

/* mux clocks */
#define XMORCA_SYSAPB_CLK          40
#define XMORCA_FMC_MUX             41
#define XMORCA_UART0_MUX           42
#define XMORCA_UART1_MUX           43
#define XMORCA_UART2_MUX           44
#define XMORCA_UART3_MUX           45
#define XMORCA_UART4_MUX           46
#define XMORCA_UART5_MUX           47
#define XMORCA_UART_MUX            48
#define XMORCA_MMC0_MUX            49
#define XMORCA_MMC1_MUX            50
#define XMORCA_MMC2_MUX            51
#define XMORCA_ETH_MUX             52
#define XMORCA_CPU_MUX             53
#define XMORCA_USB2_MUX            54
#define XMORCA_SSP_MUX             55
/* gate clocks */
#define XMORCA_UART0_CLK           70
#define XMORCA_UART1_CLK           71
#define XMORCA_UART2_CLK           72
#define XMORCA_FMC_CLK             73
#define XMORCA_ETH0_CLK            74
#define XMORCA_EDMAC_AXICLK        75
#define XMORCA_EDMAC_CLK           76
#define XMORCA_SPI0_CLK            77
#define XMORCA_SPI1_CLK            78
#define XMORCA_MMC0_CLK            79
#define XMORCA_MMC1_CLK            80
#define XMORCA_MMC2_CLK            81
#define XMORCA_I2C0_CLK            82
#define XMORCA_I2C1_CLK            83
#define XMORCA_I2C2_CLK            84
#define XMORCA_I2C3_CLK            85
#define XMORCA_I2C4_CLK            86
#define XMORCA_I2C5_CLK            87
#define XMORCA_I2C6_CLK            88
#define XMORCA_I2C7_CLK            89
#define XMORCA_SPI2_CLK            90
#define XMORCA_SPI3_CLK            91
#define XMORCA_UART3_CLK           92
#define XMORCA_UART4_CLK           93
#define XMORCA_USB2_BUS_CLK        94
#define XMORCA_USB2_REF_CLK        95
#define XMORCA_USB2_UTMI_CLK       96
#define XMORCA_LOTUS_PHY_APB_CLK   97
#define XMORCA_LOTUS_PHY_REF_CLK   98
#define XMORCA_USB2_PHY_XO_CLK     99

#define XMORCA_NR_CLKS         256
#define XMORCA_NR_RSTS         256

#endif  /* __DTS_XMORCA_CLOCK_H */
