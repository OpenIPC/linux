/*
 * padmux.h- Sigmastar
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
#ifndef ___PADMUX_H
#define ___PADMUX_H

// PADMUX MODE INDEX MARCO START
#define PINMUX_FOR_GPIO_MODE 0x00

#define PINMUX_FOR_EJ_MODE_1         0x1
#define PINMUX_FOR_EJ_MODE_2         0x2
#define PINMUX_FOR_TEST_IN_MODE_1    0x3
#define PINMUX_FOR_TEST_IN_MODE_2    0x4
#define PINMUX_FOR_TEST_IN_MODE_3    0x5
#define PINMUX_FOR_TEST_OUT_MODE_1   0x6
#define PINMUX_FOR_TEST_OUT_MODE_2   0x7
#define PINMUX_FOR_TEST_OUT_MODE_3   0x8
#define PINMUX_FOR_IR_IN_MODE_1      0x9
#define PINMUX_FOR_I2C0_MODE_1       0xa
#define PINMUX_FOR_I2C0_MODE_2       0xb
#define PINMUX_FOR_I2C0_MODE_3       0xc
#define PINMUX_FOR_I2C0_MODE_4       0xd
#define PINMUX_FOR_I2C1_MODE_1       0xe
#define PINMUX_FOR_I2C1_MODE_2       0xf
#define PINMUX_FOR_I2C1_MODE_3       0x10
#define PINMUX_FOR_I2C1_MODE_4       0x11
#define PINMUX_FOR_I2C2_MODE_1       0x12
#define PINMUX_FOR_I2C2_MODE_2       0x13
#define PINMUX_FOR_I2C2_MODE_3       0x14
#define PINMUX_FOR_SPI0_MODE_1       0x15
#define PINMUX_FOR_SPI0_MODE_2       0x16
#define PINMUX_FOR_SPI0_MODE_3       0x17
#define PINMUX_FOR_SPI0_MODE_4       0x18
#define PINMUX_FOR_SPI0_CZ1_MODE_1   0x19
#define PINMUX_FOR_SPI0_CZ1_MODE_2   0x1a
#define PINMUX_FOR_SPI1_MODE_1       0x1b
#define PINMUX_FOR_SPI1_MODE_2       0x1c
#define PINMUX_FOR_SPI1_MODE_3       0x1d
#define PINMUX_FOR_SPI1_MODE_4       0x1e
#define PINMUX_FOR_SPI1_CZ1_MODE_1   0x1f
#define PINMUX_FOR_SPI1_CZ1_MODE_2   0x20
#define PINMUX_FOR_SPI1_CZ1_MODE_3   0x21
#define PINMUX_FOR_FUART_MODE_1      0x22
#define PINMUX_FOR_FUART_MODE_2      0x23
#define PINMUX_FOR_FUART_MODE_3      0x24
#define PINMUX_FOR_FUART_MODE_4      0x25
#define PINMUX_FOR_FUART_2W_MODE_1   0x26
#define PINMUX_FOR_FUART_2W_MODE_2   0x27
#define PINMUX_FOR_FUART_2W_MODE_3   0x28
#define PINMUX_FOR_FUART_2W_MODE_4   0x29
#define PINMUX_FOR_UART0_MODE_1      0x2a
#define PINMUX_FOR_UART0_MODE_2      0x2b
#define PINMUX_FOR_UART1_MODE_1      0x2c
#define PINMUX_FOR_UART1_MODE_2      0x2d
#define PINMUX_FOR_UART1_MODE_3      0x2e
#define PINMUX_FOR_UART1_MODE_4      0x2f
#define PINMUX_FOR_SD0_MODE_1        0x30
#define PINMUX_FOR_SD0_CDZ_MODE_1    0x31
#define PINMUX_FOR_SDIO_MODE_1       0x32
#define PINMUX_FOR_SDIO_MODE_2       0x33
#define PINMUX_FOR_SDIO_MODE_3       0x34
#define PINMUX_FOR_SDIO_MODE_4       0x35
#define PINMUX_FOR_SDIO_CDZ_MODE_1   0x36
#define PINMUX_FOR_SDIO_RST_MODE_1   0x37
#define PINMUX_FOR_SDIO_CDZ_MODE_2   0x38
#define PINMUX_FOR_SDIO_RST_MODE_2   0x39
#define PINMUX_FOR_SDIO_CDZ_MODE_3   0x3a
#define PINMUX_FOR_SDIO_RST_MODE_3   0x3b
#define PINMUX_FOR_EMMC_4B_MODE_1    0x3c
#define PINMUX_FOR_EMMC_RST_MODE_1   0x3d
#define PINMUX_FOR_PWM0_MODE_1       0x3e
#define PINMUX_FOR_PWM0_MODE_2       0x3f
#define PINMUX_FOR_PWM0_MODE_3       0x40
#define PINMUX_FOR_PWM0_MODE_4       0x41
#define PINMUX_FOR_PWM1_MODE_1       0x42
#define PINMUX_FOR_PWM1_MODE_2       0x43
#define PINMUX_FOR_PWM1_MODE_3       0x44
#define PINMUX_FOR_PWM1_MODE_4       0x45
#define PINMUX_FOR_PWM2_MODE_1       0x46
#define PINMUX_FOR_PWM2_MODE_2       0x47
#define PINMUX_FOR_PWM2_MODE_3       0x48
#define PINMUX_FOR_PWM2_MODE_4       0x49
#define PINMUX_FOR_PWM3_MODE_1       0x4a
#define PINMUX_FOR_PWM3_MODE_2       0x4b
#define PINMUX_FOR_PWM3_MODE_3       0x4c
#define PINMUX_FOR_PWM3_MODE_4       0x4d
#define PINMUX_FOR_PWM4_MODE_1       0x4e
#define PINMUX_FOR_PWM4_MODE_2       0x4f
#define PINMUX_FOR_PWM4_MODE_3       0x50
#define PINMUX_FOR_PWM5_MODE_1       0x51
#define PINMUX_FOR_PWM5_MODE_2       0x52
#define PINMUX_FOR_PWM5_MODE_3       0x53
#define PINMUX_FOR_PWM6_MODE_1       0x54
#define PINMUX_FOR_PWM6_MODE_2       0x55
#define PINMUX_FOR_PWM6_MODE_3       0x56
#define PINMUX_FOR_PWM7_MODE_1       0x57
#define PINMUX_FOR_PWM7_MODE_2       0x58
#define PINMUX_FOR_PWM7_MODE_3       0x59
#define PINMUX_FOR_PWM8_MODE_1       0x5a
#define PINMUX_FOR_PWM8_MODE_2       0x5b
#define PINMUX_FOR_PWM8_MODE_3       0x5c
#define PINMUX_FOR_PWM9_MODE_1       0x5d
#define PINMUX_FOR_PWM9_MODE_2       0x5e
#define PINMUX_FOR_PWM9_MODE_3       0x5f
#define PINMUX_FOR_PWM9_MODE_4       0x60
#define PINMUX_FOR_PWM10_MODE_1      0x61
#define PINMUX_FOR_PWM10_MODE_2      0x62
#define PINMUX_FOR_PWM10_MODE_3      0x63
#define PINMUX_FOR_PWM10_MODE_4      0x64
#define PINMUX_FOR_LED0_MODE_1       0x65
#define PINMUX_FOR_LED0_MODE_2       0x66
#define PINMUX_FOR_LED0_MODE_3       0x67
#define PINMUX_FOR_LED0_MODE_4       0x68
#define PINMUX_FOR_LED1_MODE_1       0x69
#define PINMUX_FOR_LED1_MODE_2       0x6a
#define PINMUX_FOR_LED1_MODE_3       0x6b
#define PINMUX_FOR_LED1_MODE_4       0x6c
#define PINMUX_FOR_I2S_MCK_MODE_1    0x6d
#define PINMUX_FOR_I2S_MCK_MODE_2    0x6e
#define PINMUX_FOR_I2S_MCK_MODE_3    0x6f
#define PINMUX_FOR_I2S_RX_MODE_1     0x70
#define PINMUX_FOR_I2S_RX_MODE_2     0x71
#define PINMUX_FOR_I2S_RX_MODE_3     0x72
#define PINMUX_FOR_I2S_RX_MODE_4     0x73
#define PINMUX_FOR_DMIC_4CH_MODE_1   0x74
#define PINMUX_FOR_DMIC_4CH_MODE_2   0x75
#define PINMUX_FOR_DMIC_4CH_MODE_3   0x76
#define PINMUX_FOR_DMIC_6CH_MODE_1   0x77
#define PINMUX_FOR_DMIC_6CH_MODE_2   0x78
#define PINMUX_FOR_DMIC_6CH_MODE_3   0x79
#define PINMUX_FOR_DMIC_6CH_MODE_4   0x7a
#define PINMUX_FOR_DMIC_6CH_MODE_5   0x7b
#define PINMUX_FOR_ETH_MODE_1        0x7c
#define PINMUX_FOR_ETH_MODE_2        0x7d
#define PINMUX_FOR_ETH_MODE_3        0x7e
#define PINMUX_FOR_SR0_MIPI_MODE_1   0x7f
#define PINMUX_FOR_SR0_MIPI_MODE_2   0x80
#define PINMUX_FOR_SR0_MIPI_MODE_3   0x81
#define PINMUX_FOR_SR0_MIPI_MODE_4   0x82
#define PINMUX_FOR_SR0_MODE_1        0x83
#define PINMUX_FOR_SR0_MODE_2        0x84
#define PINMUX_FOR_SR00_MCLK_MODE_1  0x85
#define PINMUX_FOR_SR00_MCLK_MODE_2  0x86
#define PINMUX_FOR_SR01_MCLK_MODE_1  0x87
#define PINMUX_FOR_SR01_MCLK_MODE_2  0x88
#define PINMUX_FOR_SR01_MCLK_MODE_3  0x89
#define PINMUX_FOR_SR00_RST_MODE_1   0x8a
#define PINMUX_FOR_SR00_RST_MODE_2   0x8b
#define PINMUX_FOR_SR00_RST_MODE_3   0x8c
#define PINMUX_FOR_SR01_RST_MODE_1   0x8d
#define PINMUX_FOR_SR01_RST_MODE_2   0x8e
#define PINMUX_FOR_SR00_PDN_MODE_1   0x8f
#define PINMUX_FOR_SR00_PDN_MODE_2   0x90
#define PINMUX_FOR_SR_HSYNC_MODE_1   0x91
#define PINMUX_FOR_SR_VSYNC_MODE_1   0x92
#define PINMUX_FOR_SR0_PCLK_MODE_1   0x93
#define PINMUX_FOR_SR0_BT656_MODE_1  0x94
#define PINMUX_FOR_SR0_BT656_MODE_2  0x95
#define PINMUX_FOR_BT656_OUT_MODE_1  0x96
#define PINMUX_FOR_BT656_OUT_MODE_2  0x97
#define PINMUX_FOR_BT1120_OUT_MODE_1 0x98
#define PINMUX_FOR_BT1120_OUT_MODE_2 0x99
#define PINMUX_FOR_TTL24_MODE_1      0x9a
#define PINMUX_FOR_TTL16_MODE_1      0x9b
#define PINMUX_FOR_RGB8_MODE_1       0x9c
#define PINMUX_FOR_RGB8_MODE_2       0x9d
#define PINMUX_FOR_OTP_TEST_1        0x9e
#define PINMUX_FOR_UART_IS_GPIO_0    0x9f
#define PINMUX_FOR_SPICSZ2_GPIO_1    0xa0
#define PINMUX_FOR_SPI_GPIO_0        0xa1
#define PINMUX_FOR_SPIWPN_GPIO_0     0xa2
#define PINMUX_FOR_SPICSZ1_GPIO_0    0xa3
#define PINMUX_FOR_SPIHOLDN_MODE_0   0xa4
#define PINMUX_FOR_SAR_MODE_0        0xa5
#define PINMUX_FOR_SAR_MODE_1        0xa6
#define PINMUX_FOR_SAR_MODE_2        0xa7
#define PINMUX_FOR_SAR_MODE_3        0xa8
#define PINMUX_FOR_SPI_EXT_EN_MODE_0 0xa9
#define PINMUX_FOR_SPI_EXT_EN_MODE_1 0xaa
#define PINMUX_FOR_SPI_EXT_EN_MODE_2 0xab
#define PINMUX_FOR_SPI_EXT_EN_MODE_3 0xac
#define PINMUX_FOR_SPI_EXT_EN_MODE_4 0xad
#define PINMUX_FOR_SPI_EXT_EN_MODE_5 0xae
#define PINMUX_FOR_SPI_EXT_EN_MODE_6 0xaf
// PADMUX MODE INDEX MARCO END

// add manually for misc pads here
#define PINMUX_FOR_ETH_MODE 0xb0

#define PRIORITY_GREATER_GPIO   PINMUX_FOR_EJ_MODE_2
#define PINMUX_FOR_UNKNOWN_MODE 0xFFFF

#endif // ___PADMUX_H
