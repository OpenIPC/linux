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

#define PAD_SD1_IO1    0
#define PAD_SD1_IO0    1
#define PAD_SD1_IO5    2
#define PAD_SD1_IO4    3
#define PAD_SD1_IO3    4
#define PAD_SD1_IO2    5
#define PAD_SD1_IO6    6
#define PAD_UART1_RX   7
#define PAD_UART1_TX   8
#define PAD_SPI0_CZ    9
#define PAD_SPI0_CK    10
#define PAD_SPI0_DI    11
#define PAD_SPI0_DO    12
#define PAD_PWM0       13
#define PAD_PWM1       14
#define PAD_SD_CLK     15
#define PAD_SD_CMD     16
#define PAD_SD_D0      17
#define PAD_SD_D1      18
#define PAD_SD_D2      19
#define PAD_SD_D3      20
#define PAD_USB_CID    21
#define PAD_PM_SD_CDZ  22
#define PAD_PM_IRIN    23
#define PAD_PM_UART_RX 24
#define PAD_PM_UART_TX 25
#define PAD_PM_GPIO0   26
#define PAD_PM_GPIO1   27
#define PAD_PM_GPIO2   28
#define PAD_PM_GPIO3   29
#define PAD_PM_GPIO4   30
#define PAD_PM_GPIO7   31
#define PAD_PM_GPIO8   32
#define PAD_PM_GPIO9   33
#define PAD_PM_SPI_CZ  34
#define PAD_PM_SPI_DI  35
#define PAD_PM_SPI_WPZ 36
#define PAD_PM_SPI_DO  37
#define PAD_PM_SPI_CK  38
#define PAD_PM_SPI_HLD 39
#define PAD_PM_LED0    40
#define PAD_PM_LED1    41
#define PAD_FUART_RX   42
#define PAD_FUART_TX   43
#define PAD_FUART_CTS  44
#define PAD_FUART_RTS  45
#define PAD_GPIO0      46
#define PAD_GPIO1      47
#define PAD_GPIO2      48
#define PAD_GPIO3      49
#define PAD_GPIO4      50
#define PAD_GPIO5      51
#define PAD_GPIO6      52
#define PAD_GPIO7      53
#define PAD_GPIO14     54
#define PAD_GPIO15     55
#define PAD_I2C0_SCL   56
#define PAD_I2C0_SDA   57
#define PAD_I2C1_SCL   58
#define PAD_I2C1_SDA   59
#define PAD_SR_IO00    60
#define PAD_SR_IO01    61
#define PAD_SR_IO02    62
#define PAD_SR_IO03    63
#define PAD_SR_IO04    64
#define PAD_SR_IO05    65
#define PAD_SR_IO06    66
#define PAD_SR_IO07    67
#define PAD_SR_IO08    68
#define PAD_SR_IO09    69
#define PAD_SR_IO10    70
#define PAD_SR_IO11    71
#define PAD_SR_IO12    72
#define PAD_SR_IO13    73
#define PAD_SR_IO14    74
#define PAD_SR_IO15    75
#define PAD_SR_IO16    76
#define PAD_SR_IO17    77
#define PAD_SAR_GPIO3  78
#define PAD_SAR_GPIO2  79
#define PAD_SAR_GPIO1  80
#define PAD_SAR_GPIO0  81

// add manually for misc pads here
#define PAD_ETH_RN 82
#define PAD_ETH_RP 83
#define PAD_ETH_TN 84
#define PAD_ETH_TP 85

#define GPIO_NR     86
#define PAD_UNKNOWN 0xFFFF

#endif // #ifndef ___GPIO_H
