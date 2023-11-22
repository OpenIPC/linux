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

#define PAD_PM_UART_RX1 0
#define PAD_PM_UART_TX1 1
#define PAD_PM_UART_RX  2
#define PAD_PM_UART_TX  3
#define PAD_PM_I2CM_SCL 4
#define PAD_PM_I2CM_SDA 5
#define PAD_PM_GPIO0    6
#define PAD_PM_GPIO1    7
#define PAD_PM_GPIO2    8
#define PAD_PM_GPIO3    9
#define PAD_PM_GPIO4    10
#define PAD_PM_GPIO5    11
#define PAD_PM_GPIO6    12
#define PAD_PM_GPIO7    13
#define PAD_PM_SPI_CZ   14
#define PAD_PM_SPI_CK   15
#define PAD_PM_SPI_DI   16
#define PAD_PM_SPI_DO   17
#define PAD_PM_SPI_WPZ  18
#define PAD_PM_SPI_HLD  19
#define PAD_PM_IRIN     20
#define PAD_SAR_GPIO0   21
#define PAD_SAR_GPIO1   22
#define PAD_SAR_GPIO2   23
#define PAD_SAR_GPIO3   24
#define PAD_SD0_CDZ     25
#define PAD_SD0_D1      26
#define PAD_SD0_D0      27
#define PAD_SD0_CLK     28
#define PAD_SD0_CMD     29
#define PAD_SD0_D3      30
#define PAD_SD0_D2      31
#define PAD_FUART_RX    32
#define PAD_FUART_TX    33
#define PAD_FUART_CTS   34
#define PAD_FUART_RTS   35
#define PAD_I2C0_SCL    36
#define PAD_I2C0_SDA    37
#define PAD_ETH_LED0    38
#define PAD_ETH_LED1    39
#define PAD_SPI0_DO     40
#define PAD_SPI0_DI     41
#define PAD_SPI0_CK     42
#define PAD_SPI0_CZ     43
#define PAD_GPIO0       44
#define PAD_GPIO1       45
#define PAD_GPIO2       46
#define PAD_GPIO3       47
#define PAD_GPIO4       48
#define PAD_GPIO5       49
#define PAD_GPIO6       50
#define PAD_GPIO7       51
#define PAD_SR0_IO00    52
#define PAD_SR0_IO01    53
#define PAD_SR0_IO02    54
#define PAD_SR0_IO03    55
#define PAD_SR0_IO04    56
#define PAD_SR0_IO05    57
#define PAD_SR0_IO06    58
#define PAD_SR0_IO07    59
#define PAD_SR0_IO08    60
#define PAD_SR0_IO09    61
#define PAD_SR0_IO10    62
#define PAD_SR0_IO11    63
#define PAD_SR0_IO12    64
#define PAD_SR0_IO13    65
#define PAD_SR0_IO14    66
#define PAD_SR0_IO15    67
#define PAD_SR0_IO16    68
#define PAD_SR0_IO17    69
#define PAD_SR0_IO18    70
#define PAD_SR0_IO19    71
#define PAD_SR1_IO00    72
#define PAD_SR1_IO01    73
#define PAD_SR1_IO02    74
#define PAD_SR1_IO03    75
#define PAD_SR1_IO04    76
#define PAD_SR1_IO05    77
#define PAD_SR1_IO06    78
#define PAD_SR1_IO07    79
#define PAD_SR1_IO08    80
#define PAD_SR1_IO09    81
#define PAD_SR1_IO10    82
#define PAD_SR1_IO11    83
#define PAD_SR1_IO12    84
#define PAD_SR1_IO13    85
#define PAD_SR1_IO14    86
#define PAD_SR1_IO15    87
#define PAD_SR1_IO16    88
#define PAD_SR1_IO17    89
#define PAD_SR1_IO18    90
#define PAD_SR1_IO19    91
#define PAD_TX0_IO00    92
#define PAD_TX0_IO01    93
#define PAD_TX0_IO02    94
#define PAD_TX0_IO03    95
#define PAD_TX0_IO04    96
#define PAD_TX0_IO05    97
#define PAD_TX0_IO06    98
#define PAD_TX0_IO07    99
#define PAD_TX0_IO08    100
#define PAD_TX0_IO09    101
#define PAD_TTL0        102
#define PAD_TTL1        103
#define PAD_TTL2        104
#define PAD_TTL3        105
#define PAD_TTL4        106
#define PAD_TTL5        107
#define PAD_TTL6        108
#define PAD_TTL7        109
#define PAD_TTL8        110
#define PAD_TTL9        111
#define PAD_TTL10       112
#define PAD_TTL11       113
#define PAD_TTL12       114
#define PAD_TTL13       115
#define PAD_TTL14       116
#define PAD_TTL15       117
#define PAD_TTL16       118
#define PAD_TTL17       119
#define PAD_TTL18       120
#define PAD_TTL19       121
#define PAD_TTL20       122
#define PAD_TTL21       123
#define PAD_TTL22       124
#define PAD_TTL23       125
#define PAD_TTL24       126
#define PAD_TTL25       127
#define PAD_TTL26       128
#define PAD_TTL27       129
#define PAD_SATA_GPIO   130
#define PAD_HDMITX_HPD  131
#define PAD_HDMITX_SDA  132
#define PAD_HDMITX_SCL  133
#define PAD_VSYNC_OUT   134
#define PAD_HSYNC_OUT   135
#define PAD_GPIO8       136
#define PAD_GPIO9       137
#define PAD_GPIO10      138
#define PAD_GPIO11      139
#define PAD_GPIO12      140
#define PAD_GPIO13      141
#define PAD_GPIO14      142
#define PAD_GPIO15      143
#define PAD_GPIO16      144
#define PAD_SD_GPIO0    145
#define PAD_SD1_GPIO0   146
#define PAD_SD1_GPIO1   147
#define PAD_SD1_CDZ     148
#define PAD_SD1_D1      149
#define PAD_SD1_D0      150
#define PAD_SD1_CLK     151
#define PAD_SD1_CMD     152
#define PAD_SD1_D3      153
#define PAD_SD1_D2      154
#define PAD_ETH_RN      155
#define PAD_ETH_RP      156
#define PAD_ETH_TN      157
#define PAD_ETH_TP      158
#define PAD_DM_P0       159
#define PAD_DP_P0       160
#define PAD_DM_P1       161
#define PAD_DP_P1       162

#define GPIO_NR     163
#define PAD_UNKNOWN 0xFFFF

#endif // #ifndef ___GPIO_H
