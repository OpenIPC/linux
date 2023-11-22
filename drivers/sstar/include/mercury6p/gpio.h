/*
 * gpio.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#ifndef ___GPIO_H
#define ___GPIO_H

#define PAD_UART_RX2     0
#define PAD_UART_TX2     1
#define PAD_GPIO11       2
#define PAD_ETH_LED0     3
#define PAD_ETH_LED1     4
#define PAD_FUART_RX     5
#define PAD_FUART_TX     6
#define PAD_FUART_CTS    7
#define PAD_FUART_RTS    8
#define PAD_I2C3_SCL     9
#define PAD_I2C3_SDA     10
#define PAD_IRIN         11
#define PAD_SATA_GPIO0   12
#define PAD_SATA_GPIO1   13
#define PAD_PWM0         14
#define PAD_PWM1         15
#define PAD_SD0_GPIO0    16
#define PAD_SD0_VCTRL    17
#define PAD_SD0_CDZ      18
#define PAD_SD0_D1       19
#define PAD_SD0_D0       20
#define PAD_SD0_CLK      21
#define PAD_SD0_CMD      22
#define PAD_SD0_D3       23
#define PAD_SD0_D2       24
#define PAD_GPIO0        25
#define PAD_GPIO1        26
#define PAD_GPIO2        27
#define PAD_GPIO3        28
#define PAD_GPIO4        29
#define PAD_GPIO5        30
#define PAD_EMMC_D0      31
#define PAD_EMMC_D1      32
#define PAD_EMMC_D2      33
#define PAD_EMMC_D3      34
#define PAD_EMMC_D4      35
#define PAD_EMMC_D5      36
#define PAD_EMMC_D6      37
#define PAD_EMMC_D7      38
#define PAD_EMMC_DS      39
#define PAD_EMMC_CMD     40
#define PAD_EMMC_CLK     41
#define PAD_EMMC_RSTN    42
#define PAD_OUTP_RX0_CH0 43
#define PAD_OUTN_RX0_CH0 44
#define PAD_OUTP_RX0_CH1 45
#define PAD_OUTN_RX0_CH1 46
#define PAD_OUTP_RX0_CH2 47
#define PAD_OUTN_RX0_CH2 48
#define PAD_OUTP_RX0_CH3 49
#define PAD_OUTN_RX0_CH3 50
#define PAD_OUTP_RX0_CH4 51
#define PAD_OUTN_RX0_CH4 52
#define PAD_OUTP_RX0_CH5 53
#define PAD_OUTN_RX0_CH5 54
#define PAD_OUTP_RX1_CH0 55
#define PAD_OUTN_RX1_CH0 56
#define PAD_OUTP_RX1_CH1 57
#define PAD_OUTN_RX1_CH1 58
#define PAD_OUTP_RX1_CH2 59
#define PAD_OUTN_RX1_CH2 60
#define PAD_OUTP_RX1_CH3 61
#define PAD_OUTN_RX1_CH3 62
#define PAD_OUTP_RX1_CH4 63
#define PAD_OUTN_RX1_CH4 64
#define PAD_OUTP_RX1_CH5 65
#define PAD_OUTN_RX1_CH5 66
#define PAD_VIF_IO00     67
#define PAD_VIF_IO01     68
#define PAD_VIF_IO02     69
#define PAD_VIF_IO03     70
#define PAD_VIF_IO04     71
#define PAD_VIF_IO05     72
#define PAD_VIF_IO06     73
#define PAD_VIF_IO07     74
#define PAD_VIF_IO08     75
#define PAD_VIF_IO09     76
#define PAD_VIF_IO10     77
#define PAD_VIF_IO11     78
#define PAD_VIF_IO12     79
#define PAD_VIF_IO13     80
#define PAD_VIF_IO14     81
#define PAD_VIF_IO15     82
#define PAD_VIF_IO16     83
#define PAD_VIF_IO17     84
#define PAD_RGMII1_RSTN  85
#define PAD_RGMII1_MCLK  86
#define PAD_RGMII1_RXCLK 87
#define PAD_RGMII1_RXCTL 88
#define PAD_RGMII1_RXD0  89
#define PAD_RGMII1_RXD1  90
#define PAD_RGMII1_RXD2  91
#define PAD_RGMII1_RXD3  92
#define PAD_RGMII1_TXCLK 93
#define PAD_RGMII1_TXCTL 94
#define PAD_RGMII1_TXD0  95
#define PAD_RGMII1_TXD1  96
#define PAD_RGMII1_TXD2  97
#define PAD_RGMII1_TXD3  98
#define PAD_RGMII1_MDIO  99
#define PAD_RGMII1_MDC   100
#define PAD_RGMII0_RSTN  101
#define PAD_RGMII0_RXCLK 102
#define PAD_RGMII0_RXCTL 103
#define PAD_RGMII0_RXD0  104
#define PAD_RGMII0_RXD1  105
#define PAD_RGMII0_RXD2  106
#define PAD_RGMII0_RXD3  107
#define PAD_RGMII0_TXCLK 108
#define PAD_RGMII0_TXCTL 109
#define PAD_RGMII0_TXD0  110
#define PAD_RGMII0_TXD1  111
#define PAD_RGMII0_TXD2  112
#define PAD_RGMII0_TXD3  113
#define PAD_RGMII0_MDIO  114
#define PAD_RGMII0_MDC   115
#define PAD_HDMITX_SCL   116
#define PAD_HDMITX_SDA   117
#define PAD_HDMITX_HPD   118
#define PAD_HSYNC        119
#define PAD_VSYNC        120
#define PAD_SPI_WPZ      121
#define PAD_SPI_DO       122
#define PAD_SPI_CZ       123
#define PAD_SPI_HLD      124
#define PAD_SPI_CK       125
#define PAD_SPI_DI       126
#define PAD_I2C0_SCL     127
#define PAD_I2C0_SDA     128
#define PAD_UART_RX      129
#define PAD_UART_TX      130
#define PAD_UART_RX1     131
#define PAD_UART_TX1     132
#define PAD_GPIO6        133
#define PAD_GPIO7        134
#define PAD_GPIO8        135
#define PAD_GPIO9        136
#define PAD_GPIO10       137
#define PAD_RGMII0_MCLK  138
#define PAD_SAR_GPIO0    139
#define PAD_SAR_GPIO1    140
#define PAD_SAR_GPIO2    141
#define PAD_SAR_GPIO3    142

#define GPIO_NR     143
#define PAD_UNKNOWN 0xFFFF

#endif // #ifndef ___GPIO_H
