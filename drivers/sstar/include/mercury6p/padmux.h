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

#define PINMUX_FOR_EJ_MODE_1             0x1
#define PINMUX_FOR_EJ_MODE_2             0x2
#define PINMUX_FOR_EJ_MODE_3             0x3
#define PINMUX_FOR_DLA_EJ_MODE_1         0x4
#define PINMUX_FOR_DLA_EJ_MODE_2         0x5
#define PINMUX_FOR_TEST_IN_MODE_1        0x6
#define PINMUX_FOR_TEST_IN_MODE_2        0x7
#define PINMUX_FOR_TEST_IN_MODE_3        0x8
#define PINMUX_FOR_TEST_OUT_MODE_1       0x9
#define PINMUX_FOR_TEST_OUT_MODE_2       0xa
#define PINMUX_FOR_TEST_OUT_MODE_3       0xb
#define PINMUX_FOR_UART_IS_GPIO_0        0xc
#define PINMUX_FOR_IR_IN_MODE_1          0xd
#define PINMUX_FOR_I2C0_MODE_1           0xe
#define PINMUX_FOR_I2C0_MODE_2           0xf
#define PINMUX_FOR_I2C0_MODE_3           0x10
#define PINMUX_FOR_I2C1_MODE_1           0x11
#define PINMUX_FOR_I2C1_MODE_2           0x12
#define PINMUX_FOR_I2C1_MODE_3           0x13
#define PINMUX_FOR_I2C1_MODE_4           0x14
#define PINMUX_FOR_I2C2_MODE_1           0x15
#define PINMUX_FOR_I2C2_MODE_2           0x16
#define PINMUX_FOR_I2C2_MODE_3           0x17
#define PINMUX_FOR_I2C2_MODE_4           0x18
#define PINMUX_FOR_I2C2_MODE_5           0x19
#define PINMUX_FOR_I2C3_MODE_1           0x1a
#define PINMUX_FOR_I2C3_MODE_2           0x1b
#define PINMUX_FOR_I2C3_MODE_3           0x1c
#define PINMUX_FOR_I2C3_MODE_4           0x1d
#define PINMUX_FOR_SPI0_MODE_1           0x1e
#define PINMUX_FOR_SPI0_MODE_2           0x1f
#define PINMUX_FOR_SPI0_MODE_3           0x20
#define PINMUX_FOR_SPI0_CZ1_MODE_1       0x21
#define PINMUX_FOR_SPI0_CZ1_MODE_2       0x22
#define PINMUX_FOR_FUART_MODE_1          0x23
#define PINMUX_FOR_FUART_MODE_2          0x24
#define PINMUX_FOR_FUART_MODE_3          0x25
#define PINMUX_FOR_FUART_MODE_4          0x26
#define PINMUX_FOR_FUART_2W_MODE_1       0x27
#define PINMUX_FOR_FUART_2W_MODE_2       0x28
#define PINMUX_FOR_FUART_2W_MODE_3       0x29
#define PINMUX_FOR_FUART_2W_MODE_4       0x2a
#define PINMUX_FOR_UART0_MODE_1          0x2b
#define PINMUX_FOR_UART0_MODE_2          0x2c
#define PINMUX_FOR_UART1_MODE_1          0x2d
#define PINMUX_FOR_UART1_MODE_2          0x2e
#define PINMUX_FOR_UART1_MODE_3          0x2f
#define PINMUX_FOR_UART1_MODE_4          0x30
#define PINMUX_FOR_UART2_MODE_1          0x31
#define PINMUX_FOR_UART2_MODE_2          0x32
#define PINMUX_FOR_SD0_MODE_1            0x33
#define PINMUX_FOR_SD0_CDZ_MODE_1        0x34
#define PINMUX_FOR_EMMC_8B_MODE_1        0x35
#define PINMUX_FOR_EMMC_4B_MODE_1        0x36
#define PINMUX_FOR_EMMC_RST_MODE_1       0x37
#define PINMUX_FOR_EMMC_AS_SD_CDZ_MODE_1 0x38
#define PINMUX_FOR_PWM0_MODE_1           0x39
#define PINMUX_FOR_PWM0_MODE_2           0x3a
#define PINMUX_FOR_PWM0_MODE_3           0x3b
#define PINMUX_FOR_PWM1_MODE_1           0x3c
#define PINMUX_FOR_PWM1_MODE_2           0x3d
#define PINMUX_FOR_PWM1_MODE_3           0x3e
#define PINMUX_FOR_LED0_MODE_1           0x3f
#define PINMUX_FOR_LED0_MODE_2           0x40
#define PINMUX_FOR_LED0_MODE_3           0x41
#define PINMUX_FOR_LED1_MODE_1           0x42
#define PINMUX_FOR_LED1_MODE_2           0x43
#define PINMUX_FOR_I2S0_MCK_MODE_1       0x44
#define PINMUX_FOR_I2S0_MCK_MODE_2       0x45
#define PINMUX_FOR_I2S0_RX_MODE_1        0x46
#define PINMUX_FOR_I2S0_RX_MODE_2        0x47
#define PINMUX_FOR_I2S0_TX_MODE_1        0x48
#define PINMUX_FOR_I2S0_TX_MODE_2        0x49
#define PINMUX_FOR_I2S0_RXTX_MODE_1      0x4a
#define PINMUX_FOR_I2S0_RXTX_MODE_2      0x4b
#define PINMUX_FOR_DMIC_4CH_MODE_1       0x4c
#define PINMUX_FOR_DMIC_4CH_MODE_2       0x4d
#define PINMUX_FOR_SR0_MIPI_MODE_1       0x4e
#define PINMUX_FOR_SR0_MIPI_MODE_2       0x4f
#define PINMUX_FOR_SR0_MIPI_MODE_3       0x50
#define PINMUX_FOR_SR1_MIPI_MODE_1       0x51
#define PINMUX_FOR_SR1_MIPI_MODE_2       0x52
#define PINMUX_FOR_SR1_MIPI_MODE_3       0x53
#define PINMUX_FOR_SR00_MCLK_MODE_1      0x54
#define PINMUX_FOR_SR00_MCLK_MODE_2      0x55
#define PINMUX_FOR_SR00_MCLK_MODE_3      0x56
#define PINMUX_FOR_SR00_MCLK_MODE_4      0x57
#define PINMUX_FOR_SR01_MCLK_MODE_1      0x58
#define PINMUX_FOR_SR10_MCLK_MODE_1      0x59
#define PINMUX_FOR_SR10_MCLK_MODE_2      0x5a
#define PINMUX_FOR_SR11_MCLK_MODE_1      0x5b
#define PINMUX_FOR_SR00_RST_MODE_1       0x5c
#define PINMUX_FOR_SR00_RST_MODE_2       0x5d
#define PINMUX_FOR_SR01_RST_MODE_1       0x5e
#define PINMUX_FOR_SR01_RST_MODE_2       0x5f
#define PINMUX_FOR_SR10_RST_MODE_1       0x60
#define PINMUX_FOR_SR10_RST_MODE_2       0x61
#define PINMUX_FOR_SR11_RST_MODE_1       0x62
#define PINMUX_FOR_SR11_RST_MODE_2       0x63
#define PINMUX_FOR_SR00_PDN_MODE_1       0x64
#define PINMUX_FOR_SR10_PDN_MODE_1       0x65
#define PINMUX_FOR_SR0_PCLK_MODE_1       0x66
#define PINMUX_FOR_SR1_PCLK_MODE_1       0x67
#define PINMUX_FOR_SR2_PCLK_MODE_1       0x68
#define PINMUX_FOR_SR3_PCLK_MODE_1       0x69
#define PINMUX_FOR_SR0_BT656_MODE_1      0x6a
#define PINMUX_FOR_SR1_BT656_MODE_1      0x6b
#define PINMUX_FOR_SR2_BT656_MODE_1      0x6c
#define PINMUX_FOR_SR3_BT656_MODE_1      0x6d
#define PINMUX_FOR_SR0_BT1120_MODE_1     0x6e
#define PINMUX_FOR_SR0_BT1120_MODE_2     0x6f
#define PINMUX_FOR_SR1_BT1120_MODE_1     0x70
#define PINMUX_FOR_SR1_BT1120_MODE_2     0x71
#define PINMUX_FOR_TTL24_MODE_1          0x72
#define PINMUX_FOR_VGA_HSYNC_MODE_1      0x73
#define PINMUX_FOR_VGA_VSYNC_MODE_1      0x74
#define PINMUX_FOR_SATA0_LED_MODE_1      0x75
#define PINMUX_FOR_SATA1_LED_MODE_1      0x76
#define PINMUX_FOR_GPHY0_REF_MODE_1      0x77
#define PINMUX_FOR_GPHY0_REF_MODE_2      0x78
#define PINMUX_FOR_GPHY1_REF_MODE_1      0x79
#define PINMUX_FOR_GPHY1_REF_MODE_2      0x7a
#define PINMUX_FOR_RGMII0_MODE_1         0x7b
#define PINMUX_FOR_RGMII1_MODE_1         0x7c
#define PINMUX_FOR_OTP_TEST_1            0x7d
#define PINMUX_FOR_SPI_GPIO_0            0x7e
#define PINMUX_FOR_SPIWPN_GPIO_0         0x7f
#define PINMUX_FOR_SPICSZ1_GPIO_0        0x80
#define PINMUX_FOR_SPICSZ2_MODE_1        0x81
#define PINMUX_FOR_SPIHOLDN_MODE_0       0x82
// PADMUX MODE INDEX MARCO END

// add manually for misc pads here
#define PINMUX_FOR_ETH_MODE            0x1c0
#define PINMUX_FOR_USB0_MODE           0x1c1
#define PINMUX_FOR_USB1_MODE           0x1c2
#define PINMUX_FOR_SPI_WPZ_EXT_EN_MODE 0x1c3
#define PINMUX_FOR_SPI_DO_EXT_EN_MODE  0x1c4
#define PINMUX_FOR_SPI_CZ_EXT_EN_MODE  0x1c5
#define PINMUX_FOR_SPI_HLD_EXT_EN_MODE 0x1c6
#define PINMUX_FOR_SPI_CK_EXT_EN_MODE  0x1c7
#define PINMUX_FOR_SPI_DI_EXT_EN_MODE  0x1c8

#define PRIORITY_GREATER_GPIO   PINMUX_FOR_EJ_MODE_3
#define PINMUX_FOR_UNKNOWN_MODE 0xFFFF

#endif // ___PADMUX_H
