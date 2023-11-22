/*
 * mhal_pinmux.c- Sigmastar
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
#include <linux/mm.h>
#include "ms_platform.h"
#include "mdrv_types.h"
#include "mhal_gpio.h"
#include "padmux.h"
#include "gpio.h"

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define BASE_RIU_PA    IO_ADDRESS(0x1F000000)
#define PMSLEEP_BANK   0x000E00
#define PM_SAR_BANK    0x001400
#define ALBANY1_BANK   0x151500
#define ALBANY2_BANK   0x151600
#define CHIPTOP_BANK   0x101E00
#define PADTOP_BANK    0x103C00
#define PADGPIO_BANK   0x103E00
#define PM_PADTOP_BANK 0x003F00
#define UTMI0_BANK     0x142100
#define UTMI1_BANK     0x142900
#define PADGPIO2_BANK  0x110400

#define _GPIO_W_WORD(addr, val)                                    \
    {                                                              \
        (*(volatile u16*)(unsigned long long)(addr)) = (u16)(val); \
    }
#define _GPIO_W_WORD_MASK(addr, val, mask)                                                    \
    {                                                                                         \
        (*(volatile u16*)(unsigned long long)(addr)) =                                        \
            ((*(volatile u16*)(unsigned long long)(addr)) & ~(mask)) | ((u16)(val) & (mask)); \
    }
#define _GPIO_R_BYTE(addr)            (*(volatile u8*)(unsigned long long)(addr))
#define _GPIO_R_WORD_MASK(addr, mask) ((*(volatile u16*)(unsigned long long)(addr)) & (mask))

#define GET_BASE_ADDR_BY_BANK(x, y) ((x) + ((y) << 1))
#define _RIUA_8BIT(bank, offset)    GET_BASE_ADDR_BY_BANK(BASE_RIU_PA, bank) + (((offset) & ~1) << 1) + ((offset)&1)
#define _RIUA_16BIT(bank, offset)   GET_BASE_ADDR_BY_BANK(BASE_RIU_PA, bank) + ((offset) << 2)

/* please put GPIO_GEN marco to here start */

#define REG_EJ_MODE               0x60
#define REG_EJ_MODE_MASK          BIT7 | BIT8 | BIT9
#define REG_DLA_EJ_MODE           0x71
#define REG_DLA_EJ_MODE_MASK      BIT0 | BIT1 | BIT2
#define REG_CA7_EJ_MODE           0x4c
#define REG_CA7_EJ_MODE_MASK      BIT0 | BIT1 | BIT2
#define REG_DSP_EJ_MODE           0x4c
#define REG_DSP_EJ_MODE_MASK      BIT4 | BIT5 | BIT6
#define REG_TEST_IN_MODE          0x12
#define REG_TEST_IN_MODE_MASK     BIT0 | BIT1
#define REG_TEST_OUT_MODE         0x12
#define REG_TEST_OUT_MODE_MASK    BIT4 | BIT5
#define REG_SPI_GPIO              0x35
#define REG_SPI_GPIO_MASK         BIT0
#define REG_SPIWPN_GPIO           0x35
#define REG_SPIWPN_GPIO_MASK      BIT4
#define REG_SPICSZ1_GPIO          0x35
#define REG_SPICSZ1_GPIO_MASK     BIT2
#define REG_SPICSZ2_MODE          0x66
#define REG_SPICSZ2_MODE_MASK     BIT0 | BIT1
#define REG_SPIHOLDN_MODE         0x52
#define REG_SPIHOLDN_MODE_MASK    BIT0
#define REG_UART_IS_GPIO          0x4b
#define REG_UART_IS_GPIO_MASK     BIT0
#define REG_IR_IN_MODE            0x45
#define REG_IR_IN_MODE_MASK       BIT4
#define REG_I2C0_MODE             0x6f
#define REG_I2C0_MODE_MASK        BIT0 | BIT1 | BIT2
#define REG_I2C1_MODE             0x53
#define REG_I2C1_MODE_MASK        BIT0 | BIT1 | BIT2 | BIT3 | BIT4
#define REG_I2C2_MODE             0x6f
#define REG_I2C2_MODE_MASK        BIT8 | BIT9 | BIT10 | BIT11
#define REG_I2C3_MODE             0x73
#define REG_I2C3_MODE_MASK        BIT0 | BIT1 | BIT2
#define REG_I2C4_MODE             0x73
#define REG_I2C4_MODE_MASK        BIT4 | BIT5 | BIT6
#define REG_I2C5_MODE             0x73
#define REG_I2C5_MODE_MASK        BIT8 | BIT9 | BIT10
#define REG_I2C6_MODE             0x44
#define REG_I2C6_MODE_MASK        BIT0 | BIT1 | BIT2
#define REG_I2C7_MODE             0x44
#define REG_I2C7_MODE_MASK        BIT4 | BIT5 | BIT6
#define REG_I2C8_MODE             0x44
#define REG_I2C8_MODE_MASK        BIT8 | BIT9 | BIT10
#define REG_I2C9_MODE             0x44
#define REG_I2C9_MODE_MASK        BIT12 | BIT13 | BIT14
#define REG_I2C10_MODE            0x45
#define REG_I2C10_MODE_MASK       BIT0 | BIT1 | BIT2
#define REG_SPI0_MODE             0x68
#define REG_SPI0_MODE_MASK        BIT0 | BIT1 | BIT2
#define REG_SPI0_CZ2_MODE         0x70
#define REG_SPI0_CZ2_MODE_MASK    BIT0 | BIT1
#define REG_SPI1_MODE             0x68
#define REG_SPI1_MODE_MASK        BIT4 | BIT5
#define REG_SPI2_MODE             0x68
#define REG_SPI2_MODE_MASK        BIT8 | BIT9
#define REG_SPI3_MODE             0x47
#define REG_SPI3_MODE_MASK        BIT0 | BIT1 | BIT2
#define REG_FUART_MODE            0x6e
#define REG_FUART_MODE_MASK       BIT8 | BIT9 | BIT10 | BIT11
#define REG_FUART_2W_MODE         0x6e
#define REG_FUART_2W_MODE_MASK    BIT12 | BIT13 | BIT14 | BIT15
#define REG_UART0_MODE            0x6d
#define REG_UART0_MODE_MASK       BIT0 | BIT1
#define REG_UART1_MODE            0x6d
#define REG_UART1_MODE_MASK       BIT4 | BIT5 | BIT6
#define REG_UART2_MODE            0x6d
#define REG_UART2_MODE_MASK       BIT8 | BIT9 | BIT10
#define REG_UART3_MODE            0x6d
#define REG_UART3_MODE_MASK       BIT12 | BIT13 | BIT14
#define REG_UART4_MODE            0x49
#define REG_UART4_MODE_MASK       BIT0 | BIT1 | BIT2
#define REG_UART5_MODE            0x49
#define REG_UART5_MODE_MASK       BIT4 | BIT5 | BIT6
#define REG_SD0_MODE              0x67
#define REG_SD0_MODE_MASK         BIT8 | BIT9
#define REG_SD0_CDZ_MODE          0x67
#define REG_SD0_CDZ_MODE_MASK     BIT10 | BIT11
#define REG_SD1_MODE              0x67
#define REG_SD1_MODE_MASK         BIT12 | BIT13
#define REG_SD1_CDZ_MODE          0x68
#define REG_SD1_CDZ_MODE_MASK     BIT12 | BIT13
#define REG_SD2_MODE              0x4d
#define REG_SD2_MODE_MASK         BIT0 | BIT1
#define REG_SD2_CDZ_MODE          0x4d
#define REG_SD2_CDZ_MODE_MASK     BIT4 | BIT5
#define REG_EMMC_8B_MODE          0x61
#define REG_EMMC_8B_MODE_MASK     BIT2 | BIT3
#define REG_EMMC_4B_MODE          0x61
#define REG_EMMC_4B_MODE_MASK     BIT0 | BIT1
#define REG_EMMC_RST_MODE         0x61
#define REG_EMMC_RST_MODE_MASK    BIT4 | BIT5
#define REG_PWM0_MODE             0x65
#define REG_PWM0_MODE_MASK        BIT0 | BIT1 | BIT2
#define REG_PWM1_MODE             0x65
#define REG_PWM1_MODE_MASK        BIT4 | BIT5 | BIT6
#define REG_PWM2_MODE             0x65
#define REG_PWM2_MODE_MASK        BIT8 | BIT9 | BIT10
#define REG_PWM3_MODE             0x65
#define REG_PWM3_MODE_MASK        BIT12 | BIT13 | BIT14
#define REG_PWM4_MODE             0x66
#define REG_PWM4_MODE_MASK        BIT0 | BIT1 | BIT2
#define REG_PWM5_MODE             0x66
#define REG_PWM5_MODE_MASK        BIT4 | BIT5 | BIT6
#define REG_PWM6_MODE             0x66
#define REG_PWM6_MODE_MASK        BIT8 | BIT9 | BIT10
#define REG_PWM7_MODE             0x66
#define REG_PWM7_MODE_MASK        BIT12 | BIT13 | BIT14
#define REG_PWM8_MODE             0x67
#define REG_PWM8_MODE_MASK        BIT0 | BIT1 | BIT2
#define REG_PWM9_MODE             0x67
#define REG_PWM9_MODE_MASK        BIT4 | BIT5 | BIT6
#define REG_PWM10_MODE            0x74
#define REG_PWM10_MODE_MASK       BIT0 | BIT1 | BIT2
#define REG_PWM11_MODE            0x45
#define REG_PWM11_MODE_MASK       BIT8 | BIT9 | BIT10
#define REG_I2S0_MCK_MODE         0x62
#define REG_I2S0_MCK_MODE_MASK    BIT0 | BIT1 | BIT2 | BIT3
#define REG_I2S1_MCK_MODE         0x62
#define REG_I2S1_MCK_MODE_MASK    BIT4 | BIT5 | BIT6 | BIT7
#define REG_I2S0_RX_MODE          0x62
#define REG_I2S0_RX_MODE_MASK     BIT8 | BIT9
#define REG_I2S1_RX_MODE          0x46
#define REG_I2S1_RX_MODE_MASK     BIT0 | BIT1
#define REG_I2S2_RX_MODE          0x46
#define REG_I2S2_RX_MODE_MASK     BIT8 | BIT9
#define REG_I2S3_RX_MODE          0x46
#define REG_I2S3_RX_MODE_MASK     BIT12 | BIT13
#define REG_I2S0_TX_MODE          0x62
#define REG_I2S0_TX_MODE_MASK     BIT12 | BIT13 | BIT14
#define REG_I2S1_TX_MODE          0x46
#define REG_I2S1_TX_MODE_MASK     BIT4 | BIT5
#define REG_I2S0_RXTX_MODE        0x4f
#define REG_I2S0_RXTX_MODE_MASK   BIT0 | BIT1
#define REG_I2S1_RXTX_MODE        0x4f
#define REG_I2S1_RXTX_MODE_MASK   BIT4 | BIT5 | BIT6
#define REG_DMIC_MODE             0x60
#define REG_DMIC_MODE_MASK        BIT0 | BIT1 | BIT2 | BIT3
#define REG_DMIC_4CH_MODE         0x79
#define REG_DMIC_4CH_MODE_MASK    BIT0 | BIT1 | BIT2 | BIT3
#define REG_SR0_MIPI_MODE         0x69
#define REG_SR0_MIPI_MODE_MASK    BIT0 | BIT1
#define REG_SR1_MIPI_MODE         0x69
#define REG_SR1_MIPI_MODE_MASK    BIT2 | BIT3
#define REG_SR2_MIPI_MODE         0x69
#define REG_SR2_MIPI_MODE_MASK    BIT4 | BIT5
#define REG_SR3_MIPI_MODE         0x69
#define REG_SR3_MIPI_MODE_MASK    BIT6 | BIT7
#define REG_SR00_MCLK_MODE        0x6a
#define REG_SR00_MCLK_MODE_MASK   BIT0 | BIT1
#define REG_SR01_MCLK_MODE        0x6a
#define REG_SR01_MCLK_MODE_MASK   BIT2 | BIT3
#define REG_SR02_MCLK_MODE        0x6a
#define REG_SR02_MCLK_MODE_MASK   BIT4 | BIT5
#define REG_SR03_MCLK_MODE        0x6a
#define REG_SR03_MCLK_MODE_MASK   BIT6 | BIT7
#define REG_SR10_MCLK_MODE        0x6a
#define REG_SR10_MCLK_MODE_MASK   BIT8 | BIT9
#define REG_SR11_MCLK_MODE        0x6a
#define REG_SR11_MCLK_MODE_MASK   BIT10 | BIT11
#define REG_SR12_MCLK_MODE        0x6a
#define REG_SR12_MCLK_MODE_MASK   BIT12 | BIT13
#define REG_SR13_MCLK_MODE        0x6a
#define REG_SR13_MCLK_MODE_MASK   BIT14 | BIT15
#define REG_SR00_RST_MODE         0x54
#define REG_SR00_RST_MODE_MASK    BIT0 | BIT1
#define REG_SR01_RST_MODE         0x54
#define REG_SR01_RST_MODE_MASK    BIT2 | BIT3
#define REG_SR02_RST_MODE         0x54
#define REG_SR02_RST_MODE_MASK    BIT4 | BIT5
#define REG_SR03_RST_MODE         0x54
#define REG_SR03_RST_MODE_MASK    BIT6 | BIT7
#define REG_SR10_RST_MODE         0x54
#define REG_SR10_RST_MODE_MASK    BIT8 | BIT9
#define REG_SR11_RST_MODE         0x54
#define REG_SR11_RST_MODE_MASK    BIT10 | BIT11
#define REG_SR12_RST_MODE         0x54
#define REG_SR12_RST_MODE_MASK    BIT12 | BIT13
#define REG_SR13_RST_MODE         0x54
#define REG_SR13_RST_MODE_MASK    BIT14 | BIT15
#define REG_SR00_PDN_MODE         0x51
#define REG_SR00_PDN_MODE_MASK    BIT0 | BIT1
#define REG_SR01_PDN_MODE         0x51
#define REG_SR01_PDN_MODE_MASK    BIT2 | BIT3
#define REG_SR02_PDN_MODE         0x51
#define REG_SR02_PDN_MODE_MASK    BIT4 | BIT5
#define REG_SR03_PDN_MODE         0x51
#define REG_SR03_PDN_MODE_MASK    BIT6 | BIT7
#define REG_SR10_PDN_MODE         0x51
#define REG_SR10_PDN_MODE_MASK    BIT8 | BIT9
#define REG_SR11_PDN_MODE         0x51
#define REG_SR11_PDN_MODE_MASK    BIT10
#define REG_SR12_PDN_MODE         0x51
#define REG_SR12_PDN_MODE_MASK    BIT12
#define REG_SR13_PDN_MODE         0x51
#define REG_SR13_PDN_MODE_MASK    BIT14
#define REG_SR0_PCLK_MODE         0x52
#define REG_SR0_PCLK_MODE_MASK    BIT0
#define REG_SR1_PCLK_MODE         0x52
#define REG_SR1_PCLK_MODE_MASK    BIT2
#define REG_SR2_PCLK_MODE         0x52
#define REG_SR2_PCLK_MODE_MASK    BIT4
#define REG_SR3_PCLK_MODE         0x52
#define REG_SR3_PCLK_MODE_MASK    BIT6
#define REG_SR0_SYNC_MODE         0x52
#define REG_SR0_SYNC_MODE_MASK    BIT8
#define REG_SR1_SYNC_MODE         0x52
#define REG_SR1_SYNC_MODE_MASK    BIT10
#define REG_SR2_SYNC_MODE         0x52
#define REG_SR2_SYNC_MODE_MASK    BIT12
#define REG_SR3_SYNC_MODE         0x52
#define REG_SR3_SYNC_MODE_MASK    BIT14 | BIT15
#define REG_SR0_BT601_MODE        0x6b
#define REG_SR0_BT601_MODE_MASK   BIT0
#define REG_SR1_BT601_MODE        0x6b
#define REG_SR1_BT601_MODE_MASK   BIT4
#define REG_SR2_BT601_MODE        0x6b
#define REG_SR2_BT601_MODE_MASK   BIT8
#define REG_SR3_BT601_MODE        0x6b
#define REG_SR3_BT601_MODE_MASK   BIT12
#define REG_SR0_BT656_MODE        0x4a
#define REG_SR0_BT656_MODE_MASK   BIT0
#define REG_SR1_BT656_MODE        0x4a
#define REG_SR1_BT656_MODE_MASK   BIT2
#define REG_SR2_BT656_MODE        0x4a
#define REG_SR2_BT656_MODE_MASK   BIT4
#define REG_SR3_BT656_MODE        0x4a
#define REG_SR3_BT656_MODE_MASK   BIT6
#define REG_SR4_BT656_MODE        0x4a
#define REG_SR4_BT656_MODE_MASK   BIT8
#define REG_SR5_BT656_MODE        0x4a
#define REG_SR5_BT656_MODE_MASK   BIT10
#define REG_SR6_BT656_MODE        0x4a
#define REG_SR6_BT656_MODE_MASK   BIT12
#define REG_SR7_BT656_MODE        0x4a
#define REG_SR7_BT656_MODE_MASK   BIT14
#define REG_SR0_BT1120_MODE       0x48
#define REG_SR0_BT1120_MODE_MASK  BIT0 | BIT1
#define REG_SR1_BT1120_MODE       0x48
#define REG_SR1_BT1120_MODE_MASK  BIT4 | BIT5
#define REG_SR2_BT1120_MODE       0x48
#define REG_SR2_BT1120_MODE_MASK  BIT8 | BIT9 | BIT10
#define REG_SR3_BT1120_MODE       0x48
#define REG_SR3_BT1120_MODE_MASK  BIT12 | BIT13
#define REG_MIPI_TX_MODE          0x64
#define REG_MIPI_TX_MODE_MASK     BIT0 | BIT1
#define REG_ISP0_IR_OUT_MODE      0x4c
#define REG_ISP0_IR_OUT_MODE_MASK BIT8 | BIT9
#define REG_ISP1_IR_OUT_MODE      0x4c
#define REG_ISP1_IR_OUT_MODE_MASK BIT12 | BIT13
#define REG_SR0_SLAVE_MODE        0x4e
#define REG_SR0_SLAVE_MODE_MASK   BIT0 | BIT1 | BIT2 | BIT3
#define REG_SR1_SLAVE_MODE        0x4e
#define REG_SR1_SLAVE_MODE_MASK   BIT4 | BIT5 | BIT6 | BIT7
#define REG_TTL24_MODE            0x6c
#define REG_TTL24_MODE_MASK       BIT8 | BIT9
#define REG_TTL16_MODE            0x6c
#define REG_TTL16_MODE_MASK       BIT0 | BIT1
#define REG_RGB8_MODE             0x74
#define REG_RGB8_MODE_MASK        BIT8 | BIT9
#define REG_RGB16_MODE            0x74
#define REG_RGB16_MODE_MASK       BIT12
#define REG_LCD_MCU8_MODE         0x78
#define REG_LCD_MCU8_MODE_MASK    BIT0 | BIT1
#define REG_LCD_MCU16_MODE        0x78
#define REG_LCD_MCU16_MODE_MASK   BIT2
#define REG_LCD_MCU18_MODE        0x78
#define REG_LCD_MCU18_MODE_MASK   BIT4
#define REG_BT656_OUT_MODE        0x60
#define REG_BT656_OUT_MODE_MASK   BIT4 | BIT5
#define REG_BT601_OUT_MODE        0x72
#define REG_BT601_OUT_MODE_MASK   BIT4 | BIT5
#define REG_BT1120_OUT_MODE       0x72
#define REG_BT1120_OUT_MODE_MASK  BIT0 | BIT1
#define REG_VGA_HSYNC_MODE        0x77
#define REG_VGA_HSYNC_MODE_MASK   BIT8
#define REG_VGA_VSYNC_MODE        0x77
#define REG_VGA_VSYNC_MODE_MASK   BIT9
#define REG_SATA0_LED_MODE        0x7b
#define REG_SATA0_LED_MODE_MASK   BIT0 | BIT1
#define REG_SATA1_LED_MODE        0x7b
#define REG_SATA1_LED_MODE_MASK   BIT4 | BIT5
#define REG_MII0_MODE             0x50
#define REG_MII0_MODE_MASK        BIT0
#define REG_MII1_MODE             0x50
#define REG_MII1_MODE_MASK        BIT1
#define REG_GPHY0_REF_MODE        0x38
#define REG_GPHY0_REF_MODE_MASK   BIT0 | BIT1
#define REG_GPHY1_REF_MODE        0x38
#define REG_GPHY1_REF_MODE_MASK   BIT4 | BIT5
#define REG_OTP_TEST              0x64
#define REG_OTP_TEST_MASK         BIT8

#define REG_IRIN_GPIO_MODE            0x00
#define REG_IRIN_GPIO_MODE_MASK       BIT3
#define REG_UART_RX_GPIO_MODE         0x01
#define REG_UART_RX_GPIO_MODE_MASK    BIT3
#define REG_UART_TX_GPIO_MODE         0x02
#define REG_UART_TX_GPIO_MODE_MASK    BIT3
#define REG_UART_RX1_GPIO_MODE        0x03
#define REG_UART_RX1_GPIO_MODE_MASK   BIT3
#define REG_UART_TX1_GPIO_MODE        0x04
#define REG_UART_TX1_GPIO_MODE_MASK   BIT3
#define REG_UART_RX2_GPIO_MODE        0x05
#define REG_UART_RX2_GPIO_MODE_MASK   BIT3
#define REG_UART_TX2_GPIO_MODE        0x06
#define REG_UART_TX2_GPIO_MODE_MASK   BIT3
#define REG_UART_RX3_GPIO_MODE        0x07
#define REG_UART_RX3_GPIO_MODE_MASK   BIT3
#define REG_UART_TX3_GPIO_MODE        0x08
#define REG_UART_TX3_GPIO_MODE_MASK   BIT3
#define REG_SPI_CZ_GPIO_MODE          0x09
#define REG_SPI_CZ_GPIO_MODE_MASK     BIT3
#define REG_SPI_DI_GPIO_MODE          0x0A
#define REG_SPI_DI_GPIO_MODE_MASK     BIT3
#define REG_SPI_WPZ_GPIO_MODE         0x0B
#define REG_SPI_WPZ_GPIO_MODE_MASK    BIT3
#define REG_SPI_DO_GPIO_MODE          0x0C
#define REG_SPI_DO_GPIO_MODE_MASK     BIT3
#define REG_SPI_CK_GPIO_MODE          0x0D
#define REG_SPI_CK_GPIO_MODE_MASK     BIT3
#define REG_SPI_HLD_GPIO_MODE         0x0E
#define REG_SPI_HLD_GPIO_MODE_MASK    BIT3
#define REG_SD0_GPIO1_GPIO_MODE       0x0F
#define REG_SD0_GPIO1_GPIO_MODE_MASK  BIT3
#define REG_SD0_GPIO0_GPIO_MODE       0x10
#define REG_SD0_GPIO0_GPIO_MODE_MASK  BIT3
#define REG_SD0_CDZ_GPIO_MODE         0x11
#define REG_SD0_CDZ_GPIO_MODE_MASK    BIT3
#define REG_SD0_D1_GPIO_MODE          0x12
#define REG_SD0_D1_GPIO_MODE_MASK     BIT3
#define REG_SD0_D0_GPIO_MODE          0x13
#define REG_SD0_D0_GPIO_MODE_MASK     BIT3
#define REG_SD0_CLK_GPIO_MODE         0x14
#define REG_SD0_CLK_GPIO_MODE_MASK    BIT3
#define REG_SD0_CMD_GPIO_MODE         0x15
#define REG_SD0_CMD_GPIO_MODE_MASK    BIT3
#define REG_SD0_D3_GPIO_MODE          0x16
#define REG_SD0_D3_GPIO_MODE_MASK     BIT3
#define REG_SD0_D2_GPIO_MODE          0x17
#define REG_SD0_D2_GPIO_MODE_MASK     BIT3
#define REG_FUART_RX_GPIO_MODE        0x18
#define REG_FUART_RX_GPIO_MODE_MASK   BIT3
#define REG_FUART_TX_GPIO_MODE        0x19
#define REG_FUART_TX_GPIO_MODE_MASK   BIT3
#define REG_FUART_CTS_GPIO_MODE       0x1A
#define REG_FUART_CTS_GPIO_MODE_MASK  BIT3
#define REG_FUART_RTS_GPIO_MODE       0x1B
#define REG_FUART_RTS_GPIO_MODE_MASK  BIT3
#define REG_I2C1_SCL_GPIO_MODE        0x1C
#define REG_I2C1_SCL_GPIO_MODE_MASK   BIT3
#define REG_I2C1_SDA_GPIO_MODE        0x1D
#define REG_I2C1_SDA_GPIO_MODE_MASK   BIT3
#define REG_I2C2_SCL_GPIO_MODE        0x1E
#define REG_I2C2_SCL_GPIO_MODE_MASK   BIT3
#define REG_I2C2_SDA_GPIO_MODE        0x1F
#define REG_I2C2_SDA_GPIO_MODE_MASK   BIT3
#define REG_I2C3_SCL_GPIO_MODE        0x20
#define REG_I2C3_SCL_GPIO_MODE_MASK   BIT3
#define REG_I2C3_SDA_GPIO_MODE        0x21
#define REG_I2C3_SDA_GPIO_MODE_MASK   BIT3
#define REG_SPI0_DO_GPIO_MODE         0x22
#define REG_SPI0_DO_GPIO_MODE_MASK    BIT3
#define REG_SPI0_DI_GPIO_MODE         0x23
#define REG_SPI0_DI_GPIO_MODE_MASK    BIT3
#define REG_SPI0_CK_GPIO_MODE         0x24
#define REG_SPI0_CK_GPIO_MODE_MASK    BIT3
#define REG_SPI0_CZ_GPIO_MODE         0x25
#define REG_SPI0_CZ_GPIO_MODE_MASK    BIT3
#define REG_SPI0_CZ1_GPIO_MODE        0x26
#define REG_SPI0_CZ1_GPIO_MODE_MASK   BIT3
#define REG_SPI1_DO_GPIO_MODE         0x27
#define REG_SPI1_DO_GPIO_MODE_MASK    BIT3
#define REG_SPI1_DI_GPIO_MODE         0x28
#define REG_SPI1_DI_GPIO_MODE_MASK    BIT3
#define REG_SPI1_CK_GPIO_MODE         0x29
#define REG_SPI1_CK_GPIO_MODE_MASK    BIT3
#define REG_SPI1_CZ_GPIO_MODE         0x2A
#define REG_SPI1_CZ_GPIO_MODE_MASK    BIT3
#define REG_PWM0_GPIO_MODE            0x2B
#define REG_PWM0_GPIO_MODE_MASK       BIT3
#define REG_PWM1_GPIO_MODE            0x2C
#define REG_PWM1_GPIO_MODE_MASK       BIT3
#define REG_PWM2_GPIO_MODE            0x2D
#define REG_PWM2_GPIO_MODE_MASK       BIT3
#define REG_PWM3_GPIO_MODE            0x2E
#define REG_PWM3_GPIO_MODE_MASK       BIT3
#define REG_GPIO0_GPIO_MODE           0x2F
#define REG_GPIO0_GPIO_MODE_MASK      BIT3
#define REG_GPIO1_GPIO_MODE           0x30
#define REG_GPIO1_GPIO_MODE_MASK      BIT3
#define REG_GPIO2_GPIO_MODE           0x31
#define REG_GPIO2_GPIO_MODE_MASK      BIT3
#define REG_GPIO3_GPIO_MODE           0x32
#define REG_GPIO3_GPIO_MODE_MASK      BIT3
#define REG_GPIO4_GPIO_MODE           0x33
#define REG_GPIO4_GPIO_MODE_MASK      BIT3
#define REG_GPIO5_GPIO_MODE           0x34
#define REG_GPIO5_GPIO_MODE_MASK      BIT3
#define REG_GPIO6_GPIO_MODE           0x35
#define REG_GPIO6_GPIO_MODE_MASK      BIT3
#define REG_GPIO7_GPIO_MODE           0x36
#define REG_GPIO7_GPIO_MODE_MASK      BIT3
#define REG_BT1120_D0_GPIO_MODE       0x37
#define REG_BT1120_D0_GPIO_MODE_MASK  BIT3
#define REG_BT1120_D1_GPIO_MODE       0x38
#define REG_BT1120_D1_GPIO_MODE_MASK  BIT3
#define REG_BT1120_D2_GPIO_MODE       0x39
#define REG_BT1120_D2_GPIO_MODE_MASK  BIT3
#define REG_BT1120_D3_GPIO_MODE       0x3A
#define REG_BT1120_D3_GPIO_MODE_MASK  BIT3
#define REG_BT1120_D4_GPIO_MODE       0x3B
#define REG_BT1120_D4_GPIO_MODE_MASK  BIT3
#define REG_BT1120_D5_GPIO_MODE       0x3C
#define REG_BT1120_D5_GPIO_MODE_MASK  BIT3
#define REG_BT1120_D6_GPIO_MODE       0x3D
#define REG_BT1120_D6_GPIO_MODE_MASK  BIT3
#define REG_BT1120_D7_GPIO_MODE       0x3E
#define REG_BT1120_D7_GPIO_MODE_MASK  BIT3
#define REG_BT1120_CLK_GPIO_MODE      0x3F
#define REG_BT1120_CLK_GPIO_MODE_MASK BIT3
#define REG_BT1120_D8_GPIO_MODE       0x40
#define REG_BT1120_D8_GPIO_MODE_MASK  BIT3
#define REG_BT1120_D9_GPIO_MODE       0x41
#define REG_BT1120_D9_GPIO_MODE_MASK  BIT3
#define REG_BT1120_D10_GPIO_MODE      0x42
#define REG_BT1120_D10_GPIO_MODE_MASK BIT3
#define REG_BT1120_D11_GPIO_MODE      0x43
#define REG_BT1120_D11_GPIO_MODE_MASK BIT3
#define REG_BT1120_D12_GPIO_MODE      0x44
#define REG_BT1120_D12_GPIO_MODE_MASK BIT3
#define REG_BT1120_D13_GPIO_MODE      0x45
#define REG_BT1120_D13_GPIO_MODE_MASK BIT3
#define REG_BT1120_D14_GPIO_MODE      0x46
#define REG_BT1120_D14_GPIO_MODE_MASK BIT3
#define REG_BT1120_D15_GPIO_MODE      0x47
#define REG_BT1120_D15_GPIO_MODE_MASK BIT3

#define REG_BT1120_D0_EXT_EN_MODE       0x36
#define REG_BT1120_D0_EXT_EN_MODE_MASK  BIT0
#define REG_BT1120_D1_EXT_EN_MODE       0x36
#define REG_BT1120_D1_EXT_EN_MODE_MASK  BIT1
#define REG_BT1120_D2_EXT_EN_MODE       0x36
#define REG_BT1120_D2_EXT_EN_MODE_MASK  BIT2
#define REG_BT1120_D3_EXT_EN_MODE       0x36
#define REG_BT1120_D3_EXT_EN_MODE_MASK  BIT3
#define REG_BT1120_D4_EXT_EN_MODE       0x36
#define REG_BT1120_D4_EXT_EN_MODE_MASK  BIT4
#define REG_BT1120_D5_EXT_EN_MODE       0x36
#define REG_BT1120_D5_EXT_EN_MODE_MASK  BIT5
#define REG_BT1120_D6_EXT_EN_MODE       0x36
#define REG_BT1120_D6_EXT_EN_MODE_MASK  BIT6
#define REG_BT1120_D7_EXT_EN_MODE       0x36
#define REG_BT1120_D7_EXT_EN_MODE_MASK  BIT7
#define REG_BT1120_CLK_EXT_EN_MODE      0x36
#define REG_BT1120_CLK_EXT_EN_MODE_MASK BIT8
#define REG_BT1120_D8_EXT_EN_MODE       0x36
#define REG_BT1120_D8_EXT_EN_MODE_MASK  BIT9
#define REG_BT1120_D9_EXT_EN_MODE       0x36
#define REG_BT1120_D9_EXT_EN_MODE_MASK  BIT10
#define REG_BT1120_D10_EXT_EN_MODE      0x36
#define REG_BT1120_D10_EXT_EN_MODE_MASK BIT11
#define REG_BT1120_D11_EXT_EN_MODE      0x36
#define REG_BT1120_D11_EXT_EN_MODE_MASK BIT12
#define REG_BT1120_D12_EXT_EN_MODE      0x36
#define REG_BT1120_D12_EXT_EN_MODE_MASK BIT13
#define REG_BT1120_D13_EXT_EN_MODE      0x36
#define REG_BT1120_D13_EXT_EN_MODE_MASK BIT14
#define REG_BT1120_D14_EXT_EN_MODE      0x36
#define REG_BT1120_D14_EXT_EN_MODE_MASK BIT15
#define REG_BT1120_D15_EXT_EN_MODE      0x37
#define REG_BT1120_D15_EXT_EN_MODE_MASK BIT0

#define REG_BT1120_GPIO0_GPIO_MODE      0x48
#define REG_BT1120_GPIO0_GPIO_MODE_MASK BIT3
#define REG_BT1120_GPIO1_GPIO_MODE      0x49
#define REG_BT1120_GPIO1_GPIO_MODE_MASK BIT3
#define REG_BT1120_GPIO2_GPIO_MODE      0x4A
#define REG_BT1120_GPIO2_GPIO_MODE_MASK BIT3
#define REG_SR_GPIO0_GPIO_MODE          0x4B
#define REG_SR_GPIO0_GPIO_MODE_MASK     BIT3
#define REG_SR_GPIO1_GPIO_MODE          0x4C
#define REG_SR_GPIO1_GPIO_MODE_MASK     BIT3
#define REG_SR_GPIO2_GPIO_MODE          0x4D
#define REG_SR_GPIO2_GPIO_MODE_MASK     BIT3
#define REG_SR_GPIO3_GPIO_MODE          0x4E
#define REG_SR_GPIO3_GPIO_MODE_MASK     BIT3
#define REG_SR_GPIO4_GPIO_MODE          0x4F
#define REG_SR_GPIO4_GPIO_MODE_MASK     BIT3
#define REG_SR_GPIO5_GPIO_MODE          0x50
#define REG_SR_GPIO5_GPIO_MODE_MASK     BIT3
#define REG_SR_GPIO6_GPIO_MODE          0x51
#define REG_SR_GPIO6_GPIO_MODE_MASK     BIT3
#define REG_SR_GPIO7_GPIO_MODE          0x52
#define REG_SR_GPIO7_GPIO_MODE_MASK     BIT3
#define REG_SR_GPIO8_GPIO_MODE          0x53
#define REG_SR_GPIO8_GPIO_MODE_MASK     BIT3
#define REG_SR_GPIO9_GPIO_MODE          0x54
#define REG_SR_GPIO9_GPIO_MODE_MASK     BIT3
#define REG_SR_I2CM_SCL_GPIO_MODE       0x55
#define REG_SR_I2CM_SCL_GPIO_MODE_MASK  BIT3
#define REG_SR_I2CM_SDA_GPIO_MODE       0x56
#define REG_SR_I2CM_SDA_GPIO_MODE_MASK  BIT3
#define REG_SR_RST0_GPIO_MODE           0x11
#define REG_SR_RST0_GPIO_MODE_MASK      BIT3
#define REG_SR_MCLK0_GPIO_MODE          0x12
#define REG_SR_MCLK0_GPIO_MODE_MASK     BIT3
#define REG_SR_RST1_GPIO_MODE           0x13
#define REG_SR_RST1_GPIO_MODE_MASK      BIT3
#define REG_SR_MCLK1_GPIO_MODE          0x14
#define REG_SR_MCLK1_GPIO_MODE_MASK     BIT3
#define REG_SR_RST2_GPIO_MODE           0x15
#define REG_SR_RST2_GPIO_MODE_MASK      BIT3
#define REG_SR_MCLK2_GPIO_MODE          0x16
#define REG_SR_MCLK2_GPIO_MODE_MASK     BIT3
#define REG_SR_RST3_GPIO_MODE           0x17
#define REG_SR_RST3_GPIO_MODE_MASK      BIT3
#define REG_SR_MCLK3_GPIO_MODE          0x18
#define REG_SR_MCLK3_GPIO_MODE_MASK     BIT3
#define REG_SR_RST4_GPIO_MODE           0x19
#define REG_SR_RST4_GPIO_MODE_MASK      BIT3
#define REG_SR_MCLK4_GPIO_MODE          0x1A
#define REG_SR_MCLK4_GPIO_MODE_MASK     BIT3
#define REG_SR_RST5_GPIO_MODE           0x1B
#define REG_SR_RST5_GPIO_MODE_MASK      BIT3
#define REG_SR_MCLK5_GPIO_MODE          0x1C
#define REG_SR_MCLK5_GPIO_MODE_MASK     BIT3
#define REG_SR_RST6_GPIO_MODE           0x1D
#define REG_SR_RST6_GPIO_MODE_MASK      BIT3
#define REG_SR_MCLK6_GPIO_MODE          0x1E
#define REG_SR_MCLK6_GPIO_MODE_MASK     BIT3
#define REG_SR_RST7_GPIO_MODE           0x1F
#define REG_SR_RST7_GPIO_MODE_MASK      BIT3
#define REG_SR_MCLK7_GPIO_MODE          0x20
#define REG_SR_MCLK7_GPIO_MODE_MASK     BIT3
#define REG_SR_GPIO10_GPIO_MODE         0x21
#define REG_SR_GPIO10_GPIO_MODE_MASK    BIT3
#define REG_SR_GPIO11_GPIO_MODE         0x22
#define REG_SR_GPIO11_GPIO_MODE_MASK    BIT3
#define REG_I2S0_RX_BCK_GPIO_MODE       0x23
#define REG_I2S0_RX_BCK_GPIO_MODE_MASK  BIT3
#define REG_I2S0_RX_WCK_GPIO_MODE       0x24
#define REG_I2S0_RX_WCK_GPIO_MODE_MASK  BIT3
#define REG_I2S0_RX_DI_GPIO_MODE        0x25
#define REG_I2S0_RX_DI_GPIO_MODE_MASK   BIT3
#define REG_I2S1_RX_BCK_GPIO_MODE       0x26
#define REG_I2S1_RX_BCK_GPIO_MODE_MASK  BIT3
#define REG_I2S1_RX_WCK_GPIO_MODE       0x27
#define REG_I2S1_RX_WCK_GPIO_MODE_MASK  BIT3
#define REG_I2S1_RX_DI_GPIO_MODE        0x28
#define REG_I2S1_RX_DI_GPIO_MODE_MASK   BIT3
#define REG_I2S2_RX_BCK_GPIO_MODE       0x29
#define REG_I2S2_RX_BCK_GPIO_MODE_MASK  BIT3
#define REG_I2S2_RX_WCK_GPIO_MODE       0x2A
#define REG_I2S2_RX_WCK_GPIO_MODE_MASK  BIT3
#define REG_I2S2_RX_DI_GPIO_MODE        0x2B
#define REG_I2S2_RX_DI_GPIO_MODE_MASK   BIT3
#define REG_I2S3_RX_BCK_GPIO_MODE       0x2C
#define REG_I2S3_RX_BCK_GPIO_MODE_MASK  BIT3
#define REG_I2S3_RX_WCK_GPIO_MODE       0x2D
#define REG_I2S3_RX_WCK_GPIO_MODE_MASK  BIT3
#define REG_I2S3_RX_DI_GPIO_MODE        0x2E
#define REG_I2S3_RX_DI_GPIO_MODE_MASK   BIT3
#define REG_I2S0_TX_BCK_GPIO_MODE       0x2F
#define REG_I2S0_TX_BCK_GPIO_MODE_MASK  BIT3
#define REG_I2S0_TX_WCK_GPIO_MODE       0x30
#define REG_I2S0_TX_WCK_GPIO_MODE_MASK  BIT3
#define REG_I2S0_TX_DO_GPIO_MODE        0x31
#define REG_I2S0_TX_DO_GPIO_MODE_MASK   BIT3
#define REG_I2S1_TX_BCK_GPIO_MODE       0x32
#define REG_I2S1_TX_BCK_GPIO_MODE_MASK  BIT3
#define REG_I2S1_TX_WCK_GPIO_MODE       0x33
#define REG_I2S1_TX_WCK_GPIO_MODE_MASK  BIT3
#define REG_I2S1_TX_DO_GPIO_MODE        0x34
#define REG_I2S1_TX_DO_GPIO_MODE_MASK   BIT3
#define REG_I2S_MCLK0_GPIO_MODE         0x35
#define REG_I2S_MCLK0_GPIO_MODE_MASK    BIT3
#define REG_I2S_MCLK1_GPIO_MODE         0x36
#define REG_I2S_MCLK1_GPIO_MODE_MASK    BIT3
#define REG_HDMITX_HPD_GPIO_MODE        0x37
#define REG_HDMITX_HPD_GPIO_MODE_MASK   BIT3
#define REG_HDMITX_SDA_GPIO_MODE        0x38
#define REG_HDMITX_SDA_GPIO_MODE_MASK   BIT3
#define REG_HDMITX_SCL_GPIO_MODE        0x39
#define REG_HDMITX_SCL_GPIO_MODE_MASK   BIT3
#define REG_VSYNC_OUT_GPIO_MODE         0x3A
#define REG_VSYNC_OUT_GPIO_MODE_MASK    BIT3
#define REG_HSYNC_OUT_GPIO_MODE         0x3B
#define REG_HSYNC_OUT_GPIO_MODE_MASK    BIT3
#define REG_GPIO8_GPIO_MODE             0x3C
#define REG_GPIO8_GPIO_MODE_MASK        BIT3
#define REG_GPIO9_GPIO_MODE             0x3D
#define REG_GPIO9_GPIO_MODE_MASK        BIT3
#define REG_GPIO10_GPIO_MODE            0x3E
#define REG_GPIO10_GPIO_MODE_MASK       BIT3
#define REG_GPIO11_GPIO_MODE            0x3F
#define REG_GPIO11_GPIO_MODE_MASK       BIT3
#define REG_GPIO12_GPIO_MODE            0x40
#define REG_GPIO12_GPIO_MODE_MASK       BIT3
#define REG_GPIO13_GPIO_MODE            0x41
#define REG_GPIO13_GPIO_MODE_MASK       BIT3
#define REG_GPIO14_GPIO_MODE            0x42
#define REG_GPIO14_GPIO_MODE_MASK       BIT3
#define REG_GPIO15_GPIO_MODE            0x43
#define REG_GPIO15_GPIO_MODE_MASK       BIT3
#define REG_GPIO16_GPIO_MODE            0x44
#define REG_GPIO16_GPIO_MODE_MASK       BIT3
#define REG_GPIO17_GPIO_MODE            0x45
#define REG_GPIO17_GPIO_MODE_MASK       BIT3
#define REG_SD1_GPIO0_GPIO_MODE         0x46
#define REG_SD1_GPIO0_GPIO_MODE_MASK    BIT3
#define REG_SD1_GPIO1_GPIO_MODE         0x47
#define REG_SD1_GPIO1_GPIO_MODE_MASK    BIT3
#define REG_SD1_CDZ_GPIO_MODE           0x48
#define REG_SD1_CDZ_GPIO_MODE_MASK      BIT3
#define REG_SD1_D1_GPIO_MODE            0x49
#define REG_SD1_D1_GPIO_MODE_MASK       BIT3
#define REG_SD1_D0_GPIO_MODE            0x4A
#define REG_SD1_D0_GPIO_MODE_MASK       BIT3
#define REG_SD1_CLK_GPIO_MODE           0x4B
#define REG_SD1_CLK_GPIO_MODE_MASK      BIT3
#define REG_SD1_CMD_GPIO_MODE           0x4C
#define REG_SD1_CMD_GPIO_MODE_MASK      BIT3
#define REG_SD1_D3_GPIO_MODE            0x4D
#define REG_SD1_D3_GPIO_MODE_MASK       BIT3
#define REG_SD1_D2_GPIO_MODE            0x4E
#define REG_SD1_D2_GPIO_MODE_MASK       BIT3
#define REG_EMMC_D0_GPIO_MODE           0x4F
#define REG_EMMC_D0_GPIO_MODE_MASK      BIT3
#define REG_EMMC_D1_GPIO_MODE           0x50
#define REG_EMMC_D1_GPIO_MODE_MASK      BIT3
#define REG_EMMC_D2_GPIO_MODE           0x51
#define REG_EMMC_D2_GPIO_MODE_MASK      BIT3
#define REG_EMMC_D3_GPIO_MODE           0x52
#define REG_EMMC_D3_GPIO_MODE_MASK      BIT3
#define REG_EMMC_D4_GPIO_MODE           0x53
#define REG_EMMC_D4_GPIO_MODE_MASK      BIT3
#define REG_EMMC_D5_GPIO_MODE           0x54
#define REG_EMMC_D5_GPIO_MODE_MASK      BIT3
#define REG_EMMC_D6_GPIO_MODE           0x55
#define REG_EMMC_D6_GPIO_MODE_MASK      BIT3
#define REG_EMMC_D7_GPIO_MODE           0x56
#define REG_EMMC_D7_GPIO_MODE_MASK      BIT3
#define REG_EMMC_CLK_GPIO_MODE          0x57
#define REG_EMMC_CLK_GPIO_MODE_MASK     BIT3
#define REG_EMMC_CMD_GPIO_MODE          0x58
#define REG_EMMC_CMD_GPIO_MODE_MASK     BIT3
#define REG_EMMC_RST_GPIO_MODE          0x59
#define REG_EMMC_RST_GPIO_MODE_MASK     BIT3
#define REG_RGMII0_TXCLK_GPIO_MODE      0x5A
#define REG_RGMII0_TXCLK_GPIO_MODE_MASK BIT3
#define REG_RGMII0_TXD0_GPIO_MODE       0x5B
#define REG_RGMII0_TXD0_GPIO_MODE_MASK  BIT3
#define REG_RGMII0_TXD1_GPIO_MODE       0x5C
#define REG_RGMII0_TXD1_GPIO_MODE_MASK  BIT3
#define REG_RGMII0_TXD2_GPIO_MODE       0x5D
#define REG_RGMII0_TXD2_GPIO_MODE_MASK  BIT3
#define REG_RGMII0_TXD3_GPIO_MODE       0x5E
#define REG_RGMII0_TXD3_GPIO_MODE_MASK  BIT3
#define REG_RGMII0_TXCTL_GPIO_MODE      0x5F
#define REG_RGMII0_TXCTL_GPIO_MODE_MASK BIT3
#define REG_RGMII0_RXCLK_GPIO_MODE      0x60
#define REG_RGMII0_RXCLK_GPIO_MODE_MASK BIT3
#define REG_RGMII0_RXD0_GPIO_MODE       0x61
#define REG_RGMII0_RXD0_GPIO_MODE_MASK  BIT3
#define REG_RGMII0_RXD1_GPIO_MODE       0x62
#define REG_RGMII0_RXD1_GPIO_MODE_MASK  BIT3
#define REG_RGMII0_RXD2_GPIO_MODE       0x63
#define REG_RGMII0_RXD2_GPIO_MODE_MASK  BIT3
#define REG_RGMII0_RXD3_GPIO_MODE       0x64
#define REG_RGMII0_RXD3_GPIO_MODE_MASK  BIT3
#define REG_RGMII0_RXCTL_GPIO_MODE      0x65
#define REG_RGMII0_RXCTL_GPIO_MODE_MASK BIT3
#define REG_RGMII0_IO0_GPIO_MODE        0x66
#define REG_RGMII0_IO0_GPIO_MODE_MASK   BIT3
#define REG_RGMII0_IO1_GPIO_MODE        0x67
#define REG_RGMII0_IO1_GPIO_MODE_MASK   BIT3
#define REG_RGMII0_MDC_GPIO_MODE        0x68
#define REG_RGMII0_MDC_GPIO_MODE_MASK   BIT3
#define REG_RGMII0_MDIO_GPIO_MODE       0x69
#define REG_RGMII0_MDIO_GPIO_MODE_MASK  BIT3
#define REG_RGMII1_TXCLK_GPIO_MODE      0x6A
#define REG_RGMII1_TXCLK_GPIO_MODE_MASK BIT3
#define REG_RGMII1_TXD0_GPIO_MODE       0x6B
#define REG_RGMII1_TXD0_GPIO_MODE_MASK  BIT3
#define REG_RGMII1_TXD1_GPIO_MODE       0x6C
#define REG_RGMII1_TXD1_GPIO_MODE_MASK  BIT3
#define REG_RGMII1_TXD2_GPIO_MODE       0x6D
#define REG_RGMII1_TXD2_GPIO_MODE_MASK  BIT3
#define REG_RGMII1_TXD3_GPIO_MODE       0x6E
#define REG_RGMII1_TXD3_GPIO_MODE_MASK  BIT3
#define REG_RGMII1_TXCTL_GPIO_MODE      0x6F
#define REG_RGMII1_TXCTL_GPIO_MODE_MASK BIT3
#define REG_RGMII1_RXCLK_GPIO_MODE      0x70
#define REG_RGMII1_RXCLK_GPIO_MODE_MASK BIT3
#define REG_RGMII1_RXD0_GPIO_MODE       0x71
#define REG_RGMII1_RXD0_GPIO_MODE_MASK  BIT3
#define REG_RGMII1_RXD1_GPIO_MODE       0x72
#define REG_RGMII1_RXD1_GPIO_MODE_MASK  BIT3
#define REG_RGMII1_RXD2_GPIO_MODE       0x73
#define REG_RGMII1_RXD2_GPIO_MODE_MASK  BIT3
#define REG_RGMII1_RXD3_GPIO_MODE       0x74
#define REG_RGMII1_RXD3_GPIO_MODE_MASK  BIT3
#define REG_RGMII1_RXCTL_GPIO_MODE      0x75
#define REG_RGMII1_RXCTL_GPIO_MODE_MASK BIT3
#define REG_RGMII1_IO0_GPIO_MODE        0x76
#define REG_RGMII1_IO0_GPIO_MODE_MASK   BIT3
#define REG_RGMII1_IO1_GPIO_MODE        0x77
#define REG_RGMII1_IO1_GPIO_MODE_MASK   BIT3
#define REG_RGMII1_MDC_GPIO_MODE        0x78
#define REG_RGMII1_MDC_GPIO_MODE_MASK   BIT3
#define REG_RGMII1_MDIO_GPIO_MODE       0x79
#define REG_RGMII1_MDIO_GPIO_MODE_MASK  BIT3

#define REG_RGMII0_TXCLK_EXT_EN_MODE      0x42
#define REG_RGMII0_TXCLK_EXT_EN_MODE_MASK BIT0
#define REG_RGMII0_TXD0_EXT_EN_MODE       0x42
#define REG_RGMII0_TXD0_EXT_EN_MODE_MASK  BIT1
#define REG_RGMII0_TXD1_EXT_EN_MODE       0x42
#define REG_RGMII0_TXD1_EXT_EN_MODE_MASK  BIT2
#define REG_RGMII0_TXD2_EXT_EN_MODE       0x42
#define REG_RGMII0_TXD2_EXT_EN_MODE_MASK  BIT3
#define REG_RGMII0_TXD3_EXT_EN_MODE       0x42
#define REG_RGMII0_TXD3_EXT_EN_MODE_MASK  BIT4
#define REG_RGMII0_TXCTL_EXT_EN_MODE      0x42
#define REG_RGMII0_TXCTL_EXT_EN_MODE_MASK BIT5
#define REG_RGMII0_RXCLK_EXT_EN_MODE      0x42
#define REG_RGMII0_RXCLK_EXT_EN_MODE_MASK BIT6
#define REG_RGMII0_RXD0_EXT_EN_MODE       0x42
#define REG_RGMII0_RXD0_EXT_EN_MODE_MASK  BIT7
#define REG_RGMII0_RXD1_EXT_EN_MODE       0x42
#define REG_RGMII0_RXD1_EXT_EN_MODE_MASK  BIT8
#define REG_RGMII0_RXD2_EXT_EN_MODE       0x42
#define REG_RGMII0_RXD2_EXT_EN_MODE_MASK  BIT9
#define REG_RGMII0_RXD3_EXT_EN_MODE       0x42
#define REG_RGMII0_RXD3_EXT_EN_MODE_MASK  BIT10
#define REG_RGMII0_RXCTL_EXT_EN_MODE      0x42
#define REG_RGMII0_RXCTL_EXT_EN_MODE_MASK BIT11
#define REG_RGMII0_IO0_EXT_EN_MODE        0x42
#define REG_RGMII0_IO0_EXT_EN_MODE_MASK   BIT12
#define REG_RGMII0_IO1_EXT_EN_MODE        0x42
#define REG_RGMII0_IO1_EXT_EN_MODE_MASK   BIT13
#define REG_RGMII0_MDC_EXT_EN_MODE        0x42
#define REG_RGMII0_MDC_EXT_EN_MODE_MASK   BIT14
#define REG_RGMII0_MDIO_EXT_EN_MODE       0x42
#define REG_RGMII0_MDIO_EXT_EN_MODE_MASK  BIT15
#define REG_RGMII1_TXCLK_EXT_EN_MODE      0x43
#define REG_RGMII1_TXCLK_EXT_EN_MODE_MASK BIT0
#define REG_RGMII1_TXD0_EXT_EN_MODE       0x43
#define REG_RGMII1_TXD0_EXT_EN_MODE_MASK  BIT1
#define REG_RGMII1_TXD1_EXT_EN_MODE       0x43
#define REG_RGMII1_TXD1_EXT_EN_MODE_MASK  BIT2
#define REG_RGMII1_TXD2_EXT_EN_MODE       0x43
#define REG_RGMII1_TXD2_EXT_EN_MODE_MASK  BIT3
#define REG_RGMII1_TXD3_EXT_EN_MODE       0x43
#define REG_RGMII1_TXD3_EXT_EN_MODE_MASK  BIT4
#define REG_RGMII1_TXCTL_EXT_EN_MODE      0x43
#define REG_RGMII1_TXCTL_EXT_EN_MODE_MASK BIT5
#define REG_RGMII1_RXCLK_EXT_EN_MODE      0x43
#define REG_RGMII1_RXCLK_EXT_EN_MODE_MASK BIT6
#define REG_RGMII1_RXD0_EXT_EN_MODE       0x43
#define REG_RGMII1_RXD0_EXT_EN_MODE_MASK  BIT7
#define REG_RGMII1_RXD1_EXT_EN_MODE       0x43
#define REG_RGMII1_RXD1_EXT_EN_MODE_MASK  BIT8
#define REG_RGMII1_RXD2_EXT_EN_MODE       0x43
#define REG_RGMII1_RXD2_EXT_EN_MODE_MASK  BIT9
#define REG_RGMII1_RXD3_EXT_EN_MODE       0x43
#define REG_RGMII1_RXD3_EXT_EN_MODE_MASK  BIT10
#define REG_RGMII1_RXCTL_EXT_EN_MODE      0x43
#define REG_RGMII1_RXCTL_EXT_EN_MODE_MASK BIT11
#define REG_RGMII1_IO0_EXT_EN_MODE        0x43
#define REG_RGMII1_IO0_EXT_EN_MODE_MASK   BIT12
#define REG_RGMII1_IO1_EXT_EN_MODE        0x43
#define REG_RGMII1_IO1_EXT_EN_MODE_MASK   BIT13
#define REG_RGMII1_MDC_EXT_EN_MODE        0x43
#define REG_RGMII1_MDC_EXT_EN_MODE_MASK   BIT14
#define REG_RGMII1_MDIO_EXT_EN_MODE       0x43
#define REG_RGMII1_MDIO_EXT_EN_MODE_MASK  BIT15

#define REG_SAR_GPIO0_GPIO_MODE      0x11
#define REG_SAR_GPIO0_GPIO_MODE_MASK BIT0
#define REG_SAR_GPIO1_GPIO_MODE      0x11
#define REG_SAR_GPIO1_GPIO_MODE_MASK BIT1
#define REG_SAR_GPIO2_GPIO_MODE      0x11
#define REG_SAR_GPIO2_GPIO_MODE_MASK BIT2
#define REG_SAR_GPIO3_GPIO_MODE      0x11
#define REG_SAR_GPIO3_GPIO_MODE_MASK BIT3

/* please put GPIO_GEN marco to here end */

/* for misc pad which must be added by hand start */

/* EMAC : ALBANY1_BANK */
#define REG_ATOP_RX_INOFF      0x69
#define REG_ATOP_RX_INOFF_MASK BIT14

/* EMAC : ALBANY2_BANK */
#define REG_ETH_GPIO_EN         0x71
#define REG_ETH_GPIO_EN_RN_MASK BIT0
#define REG_ETH_GPIO_EN_RP_MASK BIT1
#define REG_ETH_GPIO_EN_TN_MASK BIT2
#define REG_ETH_GPIO_EN_TP_MASK BIT3

/* UTMI0 : UTMI0_BANK */
#define REG_UTMI0_FL_XVR_PDN         0x00
#define REG_UTMI0_FL_XVR_PDN_MASK    BIT12
#define REG_UTMI0_REG_PDN            0x00
#define REG_UTMI0_REG_PDN_MASK       BIT15 // 1: power doen 0: enable
#define REG_UTMI0_CLK_EXTRA0_EN      0x04
#define REG_UTMI0_CLK_EXTRA0_EN_MASK BIT7 // 1: power down 0: enable
#define REG_UTMI0_GPIO_EN            0x1f
#define REG_UTMI0_GPIO_EN_MASK       BIT14

/* UTMI1 : UTMI1_BANK */
#define REG_UTMI1_FL_XVR_PDN         0x00
#define REG_UTMI1_FL_XVR_PDN_MASK    BIT12
#define REG_UTMI1_REG_PDN            0x00
#define REG_UTMI1_REG_PDN_MASK       BIT15 // 1: power doen 0: enable
#define REG_UTMI1_CLK_EXTRA0_EN      0x04
#define REG_UTMI1_CLK_EXTRA0_EN_MASK BIT7 // 1: power down 0: enable
#define REG_UTMI1_GPIO_EN            0x1f
#define REG_UTMI1_GPIO_EN_MASK       BIT14

/* for misc pad which must be added by hand end */

//-------------------- configuration -----------------
#define ENABLE_CHECK_ALL_PAD_CONFLICT 1

//==============================================================================
//
//                              STRUCTURE
//
//==============================================================================

typedef struct stPadMux
{
    U16 padID;
    U32 base;
    U16 offset;
    U16 mask;
    U16 val;
    U16 mode;
} ST_PadMuxInfo;

typedef struct stPadMode
{
    U8  u8PadName[32];
    U64 u64ModeRIU;
    U16 u16ModeMask;
    U16 u16ModeVal;
} ST_PadModeInfo;

typedef struct stPadCheck
{
    U16 base;
    U16 offset;
    U16 mask;
    U16 val;
    U16 regval;
} ST_PadCheckInfo;

typedef struct stPadCheckVal
{
    U8                infocount;
    struct stPadCheck infos[64];
} ST_PadCheckVal;

typedef struct stPadMuxEntry
{
    U32                  size;
    const ST_PadMuxInfo* padmux;
} ST_PadMuxEntry;

static S32 Pad_Mode[GPIO_NR] = {[0 ... GPIO_NR - 1] = PINMUX_FOR_UNKNOWN_MODE};
//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

ST_PadCheckVal m_stPadCheckVal;

const ST_PadMuxInfo irin_tbl[] = {
    {PAD_IRIN, PADGPIO_BANK, REG_IRIN_GPIO_MODE, REG_IRIN_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_IRIN, PM_PADTOP_BANK, REG_SPICSZ2_MODE, REG_SPICSZ2_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SPICSZ2_MODE_3},
    {PAD_IRIN, PADTOP_BANK, REG_IR_IN_MODE, REG_IR_IN_MODE_MASK, BIT4, PINMUX_FOR_IR_IN_MODE_1},
    {PAD_IRIN, PADTOP_BANK, REG_SATA0_LED_MODE, REG_SATA0_LED_MODE_MASK, BIT0, PINMUX_FOR_SATA0_LED_MODE_1},
};
const ST_PadMuxInfo uart_rx_tbl[] = {
    {PAD_UART_RX, PADGPIO_BANK, REG_UART_RX_GPIO_MODE, REG_UART_RX_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_UART_RX, PADTOP_BANK, REG_UART_IS_GPIO, REG_UART_IS_GPIO_MASK, 0, PINMUX_FOR_UART_IS_GPIO_0},
    {PAD_UART_RX, PADTOP_BANK, REG_UART0_MODE, REG_UART0_MODE_MASK, BIT0, PINMUX_FOR_UART0_MODE_1},
};
const ST_PadMuxInfo uart_tx_tbl[] = {
    {PAD_UART_TX, PADGPIO_BANK, REG_UART_TX_GPIO_MODE, REG_UART_TX_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_UART_TX, PADTOP_BANK, REG_UART_IS_GPIO, REG_UART_IS_GPIO_MASK, 0, PINMUX_FOR_UART_IS_GPIO_0},
    {PAD_UART_TX, PADTOP_BANK, REG_UART0_MODE, REG_UART0_MODE_MASK, BIT0, PINMUX_FOR_UART0_MODE_1},
};
const ST_PadMuxInfo uart_rx1_tbl[] = {
    {PAD_UART_RX1, PADGPIO_BANK, REG_UART_RX1_GPIO_MODE, REG_UART_RX1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_UART_RX1, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_UART_RX1, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_UART_RX1, PADTOP_BANK, REG_UART1_MODE, REG_UART1_MODE_MASK, BIT4, PINMUX_FOR_UART1_MODE_1},
    {PAD_UART_RX1, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_DMIC_MODE_3},
    {PAD_UART_RX1, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_DMIC_4CH_MODE_3},
};
const ST_PadMuxInfo uart_tx1_tbl[] = {
    {PAD_UART_TX1, PADGPIO_BANK, REG_UART_TX1_GPIO_MODE, REG_UART_TX1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_UART_TX1, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_UART_TX1, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_UART_TX1, PADTOP_BANK, REG_UART1_MODE, REG_UART1_MODE_MASK, BIT4, PINMUX_FOR_UART1_MODE_1},
    {PAD_UART_TX1, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_DMIC_MODE_3},
    {PAD_UART_TX1, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_DMIC_4CH_MODE_3},
};
const ST_PadMuxInfo uart_rx2_tbl[] = {
    {PAD_UART_RX2, PADGPIO_BANK, REG_UART_RX2_GPIO_MODE, REG_UART_RX2_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_UART_RX2, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT9 | BIT7, PINMUX_FOR_EJ_MODE_5},
    {PAD_UART_RX2, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_DLA_EJ_MODE_5},
    {PAD_UART_RX2, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_CA7_EJ_MODE_5},
    {PAD_UART_RX2, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_DSP_EJ_MODE_5},
    {PAD_UART_RX2, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_UART_RX2, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_UART_RX2, PADTOP_BANK, REG_I2C4_MODE, REG_I2C4_MODE_MASK, BIT6, PINMUX_FOR_I2C4_MODE_4},
    {PAD_UART_RX2, PADTOP_BANK, REG_SPI2_MODE, REG_SPI2_MODE_MASK, BIT9, PINMUX_FOR_SPI2_MODE_2},
    {PAD_UART_RX2, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10 | BIT9, PINMUX_FOR_FUART_MODE_6},
    {PAD_UART_RX2, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT14 | BIT13, PINMUX_FOR_FUART_2W_MODE_6},
    {PAD_UART_RX2, PADTOP_BANK, REG_UART2_MODE, REG_UART2_MODE_MASK, BIT8, PINMUX_FOR_UART2_MODE_1},
    {PAD_UART_RX2, PADTOP_BANK, REG_PWM0_MODE, REG_PWM0_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_PWM0_MODE_3},
    {PAD_UART_RX2, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_DMIC_4CH_MODE_3},
    {PAD_UART_RX2, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT3 | BIT1 | BIT0,
     PINMUX_FOR_SR0_SLAVE_MODE_11},
    {PAD_UART_RX2, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT6, PINMUX_FOR_SR1_SLAVE_MODE_12},
};
const ST_PadMuxInfo uart_tx2_tbl[] = {
    {PAD_UART_TX2, PADGPIO_BANK, REG_UART_TX2_GPIO_MODE, REG_UART_TX2_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_UART_TX2, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT9 | BIT7, PINMUX_FOR_EJ_MODE_5},
    {PAD_UART_TX2, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_DLA_EJ_MODE_5},
    {PAD_UART_TX2, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_CA7_EJ_MODE_5},
    {PAD_UART_TX2, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_DSP_EJ_MODE_5},
    {PAD_UART_TX2, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_UART_TX2, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_UART_TX2, PADTOP_BANK, REG_I2C4_MODE, REG_I2C4_MODE_MASK, BIT6, PINMUX_FOR_I2C4_MODE_4},
    {PAD_UART_TX2, PADTOP_BANK, REG_SPI2_MODE, REG_SPI2_MODE_MASK, BIT9, PINMUX_FOR_SPI2_MODE_2},
    {PAD_UART_TX2, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10 | BIT9, PINMUX_FOR_FUART_MODE_6},
    {PAD_UART_TX2, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT14 | BIT13, PINMUX_FOR_FUART_2W_MODE_6},
    {PAD_UART_TX2, PADTOP_BANK, REG_UART2_MODE, REG_UART2_MODE_MASK, BIT8, PINMUX_FOR_UART2_MODE_1},
    {PAD_UART_TX2, PADTOP_BANK, REG_PWM1_MODE, REG_PWM1_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_PWM1_MODE_3},
    {PAD_UART_TX2, PADTOP_BANK, REG_I2S3_RX_MODE, REG_I2S3_RX_MODE_MASK, BIT13, PINMUX_FOR_I2S3_RX_MODE_2},
    {PAD_UART_TX2, PADTOP_BANK, REG_I2S1_TX_MODE, REG_I2S1_TX_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_I2S1_TX_MODE_3},
    {PAD_UART_TX2, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT3 | BIT1 | BIT0,
     PINMUX_FOR_SR0_SLAVE_MODE_11},
    {PAD_UART_TX2, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT6, PINMUX_FOR_SR1_SLAVE_MODE_12},
};
const ST_PadMuxInfo uart_rx3_tbl[] = {
    {PAD_UART_RX3, PADGPIO_BANK, REG_UART_RX3_GPIO_MODE, REG_UART_RX3_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_UART_RX3, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT9 | BIT7, PINMUX_FOR_EJ_MODE_5},
    {PAD_UART_RX3, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_DLA_EJ_MODE_5},
    {PAD_UART_RX3, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_CA7_EJ_MODE_5},
    {PAD_UART_RX3, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_DSP_EJ_MODE_5},
    {PAD_UART_RX3, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_UART_RX3, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_UART_RX3, PADTOP_BANK, REG_I2C8_MODE, REG_I2C8_MODE_MASK, BIT9, PINMUX_FOR_I2C8_MODE_2},
    {PAD_UART_RX3, PADTOP_BANK, REG_SPI2_MODE, REG_SPI2_MODE_MASK, BIT9, PINMUX_FOR_SPI2_MODE_2},
    {PAD_UART_RX3, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10 | BIT9, PINMUX_FOR_FUART_MODE_6},
    {PAD_UART_RX3, PADTOP_BANK, REG_UART3_MODE, REG_UART3_MODE_MASK, BIT12, PINMUX_FOR_UART3_MODE_1},
    {PAD_UART_RX3, PADTOP_BANK, REG_PWM2_MODE, REG_PWM2_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_PWM2_MODE_3},
    {PAD_UART_RX3, PADTOP_BANK, REG_I2S0_MCK_MODE, REG_I2S0_MCK_MODE_MASK, BIT3, PINMUX_FOR_I2S0_MCK_MODE_8},
    {PAD_UART_RX3, PADTOP_BANK, REG_I2S3_RX_MODE, REG_I2S3_RX_MODE_MASK, BIT13, PINMUX_FOR_I2S3_RX_MODE_2},
    {PAD_UART_RX3, PADTOP_BANK, REG_I2S1_TX_MODE, REG_I2S1_TX_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_I2S1_TX_MODE_3},
    {PAD_UART_RX3, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT3 | BIT1 | BIT0,
     PINMUX_FOR_SR0_SLAVE_MODE_11},
    {PAD_UART_RX3, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT6, PINMUX_FOR_SR1_SLAVE_MODE_12},
    {PAD_UART_RX3, PADTOP_BANK, REG_SATA0_LED_MODE, REG_SATA0_LED_MODE_MASK, BIT1, PINMUX_FOR_SATA0_LED_MODE_2},
    {PAD_UART_RX3, PADTOP_BANK, REG_SATA1_LED_MODE, REG_SATA1_LED_MODE_MASK, BIT5, PINMUX_FOR_SATA1_LED_MODE_2},
};
const ST_PadMuxInfo uart_tx3_tbl[] = {
    {PAD_UART_TX3, PADGPIO_BANK, REG_UART_TX3_GPIO_MODE, REG_UART_TX3_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_UART_TX3, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT9 | BIT7, PINMUX_FOR_EJ_MODE_5},
    {PAD_UART_TX3, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_DLA_EJ_MODE_5},
    {PAD_UART_TX3, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_CA7_EJ_MODE_5},
    {PAD_UART_TX3, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_DSP_EJ_MODE_5},
    {PAD_UART_TX3, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_UART_TX3, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_UART_TX3, PADTOP_BANK, REG_I2C8_MODE, REG_I2C8_MODE_MASK, BIT9, PINMUX_FOR_I2C8_MODE_2},
    {PAD_UART_TX3, PADTOP_BANK, REG_SPI2_MODE, REG_SPI2_MODE_MASK, BIT9, PINMUX_FOR_SPI2_MODE_2},
    {PAD_UART_TX3, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10 | BIT9, PINMUX_FOR_FUART_MODE_6},
    {PAD_UART_TX3, PADTOP_BANK, REG_UART3_MODE, REG_UART3_MODE_MASK, BIT12, PINMUX_FOR_UART3_MODE_1},
    {PAD_UART_TX3, PADTOP_BANK, REG_PWM3_MODE, REG_PWM3_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_PWM3_MODE_3},
    {PAD_UART_TX3, PADTOP_BANK, REG_I2S1_MCK_MODE, REG_I2S1_MCK_MODE_MASK, BIT7 | BIT4, PINMUX_FOR_I2S1_MCK_MODE_9},
    {PAD_UART_TX3, PADTOP_BANK, REG_I2S3_RX_MODE, REG_I2S3_RX_MODE_MASK, BIT13, PINMUX_FOR_I2S3_RX_MODE_2},
    {PAD_UART_TX3, PADTOP_BANK, REG_I2S1_TX_MODE, REG_I2S1_TX_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_I2S1_TX_MODE_3},
    {PAD_UART_TX3, PADTOP_BANK, REG_SATA1_LED_MODE, REG_SATA1_LED_MODE_MASK, BIT4, PINMUX_FOR_SATA1_LED_MODE_1},
};
const ST_PadMuxInfo spi_cz_tbl[] = {
    {PAD_SPI_CZ, PADGPIO_BANK, REG_SPI_CZ_GPIO_MODE, REG_SPI_CZ_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI_CZ, PMSLEEP_BANK, REG_SPICSZ1_GPIO, REG_SPICSZ1_GPIO_MASK, 0, PINMUX_FOR_SPICSZ1_GPIO_0},
    {PAD_SPI_CZ, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT1, PINMUX_FOR_SPI3_MODE_2},
};
const ST_PadMuxInfo spi_di_tbl[] = {
    {PAD_SPI_DI, PADGPIO_BANK, REG_SPI_DI_GPIO_MODE, REG_SPI_DI_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI_DI, PMSLEEP_BANK, REG_SPI_GPIO, REG_SPI_GPIO_MASK, 0, PINMUX_FOR_SPI_GPIO_0},
    {PAD_SPI_DI, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT1, PINMUX_FOR_SPI3_MODE_2},
};
const ST_PadMuxInfo spi_wpz_tbl[] = {
    {PAD_SPI_WPZ, PADGPIO_BANK, REG_SPI_WPZ_GPIO_MODE, REG_SPI_WPZ_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI_WPZ, PMSLEEP_BANK, REG_SPIWPN_GPIO, REG_SPIWPN_GPIO_MASK, 0, PINMUX_FOR_SPIWPN_GPIO_0},
};
const ST_PadMuxInfo spi_do_tbl[] = {
    {PAD_SPI_DO, PADGPIO_BANK, REG_SPI_DO_GPIO_MODE, REG_SPI_DO_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI_DO, PMSLEEP_BANK, REG_SPI_GPIO, REG_SPI_GPIO_MASK, 0, PINMUX_FOR_SPI_GPIO_0},
    {PAD_SPI_DO, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT1, PINMUX_FOR_SPI3_MODE_2},
};
const ST_PadMuxInfo spi_ck_tbl[] = {
    {PAD_SPI_CK, PADGPIO_BANK, REG_SPI_CK_GPIO_MODE, REG_SPI_CK_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI_CK, PMSLEEP_BANK, REG_SPI_GPIO, REG_SPI_GPIO_MASK, 0, PINMUX_FOR_SPI_GPIO_0},
    {PAD_SPI_CK, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT1, PINMUX_FOR_SPI3_MODE_2},
};
const ST_PadMuxInfo spi_hld_tbl[] = {
    {PAD_SPI_HLD, PADGPIO_BANK, REG_SPI_HLD_GPIO_MODE, REG_SPI_HLD_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI_HLD, PM_PADTOP_BANK, REG_SPIHOLDN_MODE, REG_SPIHOLDN_MODE_MASK, 0, PINMUX_FOR_SPIHOLDN_MODE_0},
};
const ST_PadMuxInfo sd0_gpio1_tbl[] = {
    {PAD_SD0_GPIO1, PADGPIO_BANK, REG_SD0_GPIO1_GPIO_MODE, REG_SD0_GPIO1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD0_GPIO1, PADTOP_BANK, REG_UART0_MODE, REG_UART0_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_UART0_MODE_3},
};
const ST_PadMuxInfo sd0_gpio0_tbl[] = {
    {PAD_SD0_GPIO0, PADGPIO_BANK, REG_SD0_GPIO0_GPIO_MODE, REG_SD0_GPIO0_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD0_GPIO0, PADTOP_BANK, REG_UART0_MODE, REG_UART0_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_UART0_MODE_3},
};
const ST_PadMuxInfo sd0_cdz_tbl[] = {
    {PAD_SD0_CDZ, PADGPIO_BANK, REG_SD0_CDZ_GPIO_MODE, REG_SD0_CDZ_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD0_CDZ, PADTOP_BANK, REG_SD0_CDZ_MODE, REG_SD0_CDZ_MODE_MASK, BIT10, PINMUX_FOR_SD0_CDZ_MODE_1},
    {PAD_SD0_CDZ, PADTOP_BANK, REG_EMMC_RST_MODE, REG_EMMC_RST_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_EMMC_RST_MODE_3},
};
const ST_PadMuxInfo sd0_d1_tbl[] = {
    {PAD_SD0_D1, PADGPIO_BANK, REG_SD0_D1_GPIO_MODE, REG_SD0_D1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD0_D1, PADTOP_BANK, REG_SD0_MODE, REG_SD0_MODE_MASK, BIT8, PINMUX_FOR_SD0_MODE_1},
    {PAD_SD0_D1, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_EMMC_4B_MODE_3},
};
const ST_PadMuxInfo sd0_d0_tbl[] = {
    {PAD_SD0_D0, PADGPIO_BANK, REG_SD0_D0_GPIO_MODE, REG_SD0_D0_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD0_D0, PADTOP_BANK, REG_SD0_MODE, REG_SD0_MODE_MASK, BIT8, PINMUX_FOR_SD0_MODE_1},
    {PAD_SD0_D0, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_EMMC_4B_MODE_3},
};
const ST_PadMuxInfo sd0_clk_tbl[] = {
    {PAD_SD0_CLK, PADGPIO_BANK, REG_SD0_CLK_GPIO_MODE, REG_SD0_CLK_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD0_CLK, PADTOP_BANK, REG_SD0_MODE, REG_SD0_MODE_MASK, BIT8, PINMUX_FOR_SD0_MODE_1},
    {PAD_SD0_CLK, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_EMMC_4B_MODE_3},
};
const ST_PadMuxInfo sd0_cmd_tbl[] = {
    {PAD_SD0_CMD, PADGPIO_BANK, REG_SD0_CMD_GPIO_MODE, REG_SD0_CMD_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD0_CMD, PADTOP_BANK, REG_SD0_MODE, REG_SD0_MODE_MASK, BIT8, PINMUX_FOR_SD0_MODE_1},
    {PAD_SD0_CMD, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_EMMC_4B_MODE_3},
};
const ST_PadMuxInfo sd0_d3_tbl[] = {
    {PAD_SD0_D3, PADGPIO_BANK, REG_SD0_D3_GPIO_MODE, REG_SD0_D3_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD0_D3, PADTOP_BANK, REG_SD0_MODE, REG_SD0_MODE_MASK, BIT8, PINMUX_FOR_SD0_MODE_1},
    {PAD_SD0_D3, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_EMMC_4B_MODE_3},
};
const ST_PadMuxInfo sd0_d2_tbl[] = {
    {PAD_SD0_D2, PADGPIO_BANK, REG_SD0_D2_GPIO_MODE, REG_SD0_D2_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD0_D2, PADTOP_BANK, REG_SD0_MODE, REG_SD0_MODE_MASK, BIT8, PINMUX_FOR_SD0_MODE_1},
    {PAD_SD0_D2, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_EMMC_4B_MODE_3},
};
const ST_PadMuxInfo fuart_rx_tbl[] = {
    {PAD_FUART_RX, PADGPIO_BANK, REG_FUART_RX_GPIO_MODE, REG_FUART_RX_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_FUART_RX, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT7, PINMUX_FOR_EJ_MODE_1},
    {PAD_FUART_RX, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT0, PINMUX_FOR_DLA_EJ_MODE_1},
    {PAD_FUART_RX, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT0, PINMUX_FOR_CA7_EJ_MODE_1},
    {PAD_FUART_RX, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT4, PINMUX_FOR_DSP_EJ_MODE_1},
    {PAD_FUART_RX, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT8, PINMUX_FOR_FUART_MODE_1},
    {PAD_FUART_RX, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT12, PINMUX_FOR_FUART_2W_MODE_1},
    {PAD_FUART_RX, PADTOP_BANK, REG_UART1_MODE, REG_UART1_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_UART1_MODE_3},
    {PAD_FUART_RX, PADTOP_BANK, REG_UART3_MODE, REG_UART3_MODE_MASK, BIT14 | BIT12, PINMUX_FOR_UART3_MODE_5},
    {PAD_FUART_RX, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_FUART_RX, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
};
const ST_PadMuxInfo fuart_tx_tbl[] = {
    {PAD_FUART_TX, PADGPIO_BANK, REG_FUART_TX_GPIO_MODE, REG_FUART_TX_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_FUART_TX, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT7, PINMUX_FOR_EJ_MODE_1},
    {PAD_FUART_TX, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT0, PINMUX_FOR_DLA_EJ_MODE_1},
    {PAD_FUART_TX, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT0, PINMUX_FOR_CA7_EJ_MODE_1},
    {PAD_FUART_TX, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT4, PINMUX_FOR_DSP_EJ_MODE_1},
    {PAD_FUART_TX, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT8, PINMUX_FOR_FUART_MODE_1},
    {PAD_FUART_TX, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT12, PINMUX_FOR_FUART_2W_MODE_1},
    {PAD_FUART_TX, PADTOP_BANK, REG_UART1_MODE, REG_UART1_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_UART1_MODE_3},
    {PAD_FUART_TX, PADTOP_BANK, REG_UART3_MODE, REG_UART3_MODE_MASK, BIT14 | BIT12, PINMUX_FOR_UART3_MODE_5},
    {PAD_FUART_TX, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_FUART_TX, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
};
const ST_PadMuxInfo fuart_cts_tbl[] = {
    {PAD_FUART_CTS, PADGPIO_BANK, REG_FUART_CTS_GPIO_MODE, REG_FUART_CTS_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_FUART_CTS, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT7, PINMUX_FOR_EJ_MODE_1},
    {PAD_FUART_CTS, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT0, PINMUX_FOR_DLA_EJ_MODE_1},
    {PAD_FUART_CTS, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT0, PINMUX_FOR_CA7_EJ_MODE_1},
    {PAD_FUART_CTS, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT4, PINMUX_FOR_DSP_EJ_MODE_1},
    {PAD_FUART_CTS, PADTOP_BANK, REG_I2C9_MODE, REG_I2C9_MODE_MASK, BIT14, PINMUX_FOR_I2C9_MODE_4},
    {PAD_FUART_CTS, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT8, PINMUX_FOR_FUART_MODE_1},
    {PAD_FUART_CTS, PADTOP_BANK, REG_UART2_MODE, REG_UART2_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_UART2_MODE_3},
    {PAD_FUART_CTS, PADTOP_BANK, REG_UART4_MODE, REG_UART4_MODE_MASK, BIT1, PINMUX_FOR_UART4_MODE_2},
    {PAD_FUART_CTS, PADTOP_BANK, REG_I2S0_MCK_MODE, REG_I2S0_MCK_MODE_MASK, BIT3 | BIT1 | BIT0,
     PINMUX_FOR_I2S0_MCK_MODE_11},
    {PAD_FUART_CTS, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_FUART_CTS, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
};
const ST_PadMuxInfo fuart_rts_tbl[] = {
    {PAD_FUART_RTS, PADGPIO_BANK, REG_FUART_RTS_GPIO_MODE, REG_FUART_RTS_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_FUART_RTS, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT7, PINMUX_FOR_EJ_MODE_1},
    {PAD_FUART_RTS, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT0, PINMUX_FOR_DLA_EJ_MODE_1},
    {PAD_FUART_RTS, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT0, PINMUX_FOR_CA7_EJ_MODE_1},
    {PAD_FUART_RTS, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT4, PINMUX_FOR_DSP_EJ_MODE_1},
    {PAD_FUART_RTS, PADTOP_BANK, REG_I2C9_MODE, REG_I2C9_MODE_MASK, BIT14, PINMUX_FOR_I2C9_MODE_4},
    {PAD_FUART_RTS, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT8, PINMUX_FOR_FUART_MODE_1},
    {PAD_FUART_RTS, PADTOP_BANK, REG_UART2_MODE, REG_UART2_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_UART2_MODE_3},
    {PAD_FUART_RTS, PADTOP_BANK, REG_UART4_MODE, REG_UART4_MODE_MASK, BIT1, PINMUX_FOR_UART4_MODE_2},
    {PAD_FUART_RTS, PADTOP_BANK, REG_I2S1_MCK_MODE, REG_I2S1_MCK_MODE_MASK, BIT6 | BIT5, PINMUX_FOR_I2S1_MCK_MODE_6},
    {PAD_FUART_RTS, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_FUART_RTS, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
};
const ST_PadMuxInfo i2c1_scl_tbl[] = {
    {PAD_I2C1_SCL, PADGPIO_BANK, REG_I2C1_SCL_GPIO_MODE, REG_I2C1_SCL_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_I2C1_SCL, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT0, PINMUX_FOR_I2C1_MODE_1},
    {PAD_I2C1_SCL, PADTOP_BANK, REG_PWM8_MODE, REG_PWM8_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_PWM8_MODE_3},
};
const ST_PadMuxInfo i2c1_sda_tbl[] = {
    {PAD_I2C1_SDA, PADGPIO_BANK, REG_I2C1_SDA_GPIO_MODE, REG_I2C1_SDA_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_I2C1_SDA, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT0, PINMUX_FOR_I2C1_MODE_1},
    {PAD_I2C1_SDA, PADTOP_BANK, REG_PWM9_MODE, REG_PWM9_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_PWM9_MODE_5},
};
const ST_PadMuxInfo i2c2_scl_tbl[] = {
    {PAD_I2C2_SCL, PADGPIO_BANK, REG_I2C2_SCL_GPIO_MODE, REG_I2C2_SCL_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_I2C2_SCL, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT8, PINMUX_FOR_I2C2_MODE_1},
    {PAD_I2C2_SCL, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10, PINMUX_FOR_FUART_MODE_4},
    {PAD_I2C2_SCL, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT14, PINMUX_FOR_FUART_2W_MODE_4},
    {PAD_I2C2_SCL, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT3 | BIT0, PINMUX_FOR_SR0_SLAVE_MODE_9},
    {PAD_I2C2_SCL, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT5, PINMUX_FOR_SR1_SLAVE_MODE_10},
};
const ST_PadMuxInfo i2c2_sda_tbl[] = {
    {PAD_I2C2_SDA, PADGPIO_BANK, REG_I2C2_SDA_GPIO_MODE, REG_I2C2_SDA_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_I2C2_SDA, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT8, PINMUX_FOR_I2C2_MODE_1},
    {PAD_I2C2_SDA, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10, PINMUX_FOR_FUART_MODE_4},
    {PAD_I2C2_SDA, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT14, PINMUX_FOR_FUART_2W_MODE_4},
    {PAD_I2C2_SDA, PADTOP_BANK, REG_PWM11_MODE, REG_PWM11_MODE_MASK, BIT10 | BIT9, PINMUX_FOR_PWM11_MODE_6},
    {PAD_I2C2_SDA, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT2, PINMUX_FOR_DMIC_4CH_MODE_4},
    {PAD_I2C2_SDA, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT3 | BIT0, PINMUX_FOR_SR0_SLAVE_MODE_9},
    {PAD_I2C2_SDA, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT5, PINMUX_FOR_SR1_SLAVE_MODE_10},
};
const ST_PadMuxInfo i2c3_scl_tbl[] = {
    {PAD_I2C3_SCL, PADGPIO_BANK, REG_I2C3_SCL_GPIO_MODE, REG_I2C3_SCL_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_I2C3_SCL, PADTOP_BANK, REG_I2C3_MODE, REG_I2C3_MODE_MASK, BIT0, PINMUX_FOR_I2C3_MODE_1},
    {PAD_I2C3_SCL, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10, PINMUX_FOR_FUART_MODE_4},
    {PAD_I2C3_SCL, PADTOP_BANK, REG_UART3_MODE, REG_UART3_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_UART3_MODE_3},
    {PAD_I2C3_SCL, PADTOP_BANK, REG_PWM10_MODE, REG_PWM10_MODE_MASK, BIT2, PINMUX_FOR_PWM10_MODE_4},
    {PAD_I2C3_SCL, PADTOP_BANK, REG_I2S0_MCK_MODE, REG_I2S0_MCK_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_I2S0_MCK_MODE_6},
    {PAD_I2C3_SCL, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT2, PINMUX_FOR_DMIC_MODE_4},
    {PAD_I2C3_SCL, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT2, PINMUX_FOR_DMIC_4CH_MODE_4},
};
const ST_PadMuxInfo i2c3_sda_tbl[] = {
    {PAD_I2C3_SDA, PADGPIO_BANK, REG_I2C3_SDA_GPIO_MODE, REG_I2C3_SDA_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_I2C3_SDA, PADTOP_BANK, REG_I2C3_MODE, REG_I2C3_MODE_MASK, BIT0, PINMUX_FOR_I2C3_MODE_1},
    {PAD_I2C3_SDA, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10, PINMUX_FOR_FUART_MODE_4},
    {PAD_I2C3_SDA, PADTOP_BANK, REG_UART3_MODE, REG_UART3_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_UART3_MODE_3},
    {PAD_I2C3_SDA, PADTOP_BANK, REG_PWM11_MODE, REG_PWM11_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_PWM11_MODE_5},
    {PAD_I2C3_SDA, PADTOP_BANK, REG_I2S1_MCK_MODE, REG_I2S1_MCK_MODE_MASK, BIT6 | BIT5 | BIT4,
     PINMUX_FOR_I2S1_MCK_MODE_7},
    {PAD_I2C3_SDA, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT2, PINMUX_FOR_DMIC_MODE_4},
    {PAD_I2C3_SDA, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT2, PINMUX_FOR_DMIC_4CH_MODE_4},
};
const ST_PadMuxInfo spi0_do_tbl[] = {
    {PAD_SPI0_DO, PADGPIO_BANK, REG_SPI0_DO_GPIO_MODE, REG_SPI0_DO_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI0_DO, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_SPI0_DO, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_SPI0_DO, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT0, PINMUX_FOR_SPI0_MODE_1},
    {PAD_SPI0_DO, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SR0_SLAVE_MODE_3},
    {PAD_SPI0_DO, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT6 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_13},
};
const ST_PadMuxInfo spi0_di_tbl[] = {
    {PAD_SPI0_DI, PADGPIO_BANK, REG_SPI0_DI_GPIO_MODE, REG_SPI0_DI_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI0_DI, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_SPI0_DI, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_SPI0_DI, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT0, PINMUX_FOR_SPI0_MODE_1},
    {PAD_SPI0_DI, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SR0_SLAVE_MODE_3},
    {PAD_SPI0_DI, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT6 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_13},
};
const ST_PadMuxInfo spi0_ck_tbl[] = {
    {PAD_SPI0_CK, PADGPIO_BANK, REG_SPI0_CK_GPIO_MODE, REG_SPI0_CK_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI0_CK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_SPI0_CK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_SPI0_CK, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT0, PINMUX_FOR_SPI0_MODE_1},
    {PAD_SPI0_CK, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SR0_SLAVE_MODE_3},
    {PAD_SPI0_CK, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT6 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_13},
};
const ST_PadMuxInfo spi0_cz_tbl[] = {
    {PAD_SPI0_CZ, PADGPIO_BANK, REG_SPI0_CZ_GPIO_MODE, REG_SPI0_CZ_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI0_CZ, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_SPI0_CZ, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_SPI0_CZ, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT0, PINMUX_FOR_SPI0_MODE_1},
};
const ST_PadMuxInfo spi0_cz1_tbl[] = {
    {PAD_SPI0_CZ1, PADGPIO_BANK, REG_SPI0_CZ1_GPIO_MODE, REG_SPI0_CZ1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI0_CZ1, PADTOP_BANK, REG_SPI0_CZ2_MODE, REG_SPI0_CZ2_MODE_MASK, BIT0, PINMUX_FOR_SPI0_CZ2_MODE_1},
};
const ST_PadMuxInfo spi1_do_tbl[] = {
    {PAD_SPI1_DO, PADGPIO_BANK, REG_SPI1_DO_GPIO_MODE, REG_SPI1_DO_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI1_DO, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_SPI1_DO, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_SPI1_DO, PADTOP_BANK, REG_SPI1_MODE, REG_SPI1_MODE_MASK, BIT4, PINMUX_FOR_SPI1_MODE_1},
};
const ST_PadMuxInfo spi1_di_tbl[] = {
    {PAD_SPI1_DI, PADGPIO_BANK, REG_SPI1_DI_GPIO_MODE, REG_SPI1_DI_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI1_DI, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_SPI1_DI, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_SPI1_DI, PADTOP_BANK, REG_SPI1_MODE, REG_SPI1_MODE_MASK, BIT4, PINMUX_FOR_SPI1_MODE_1},
};
const ST_PadMuxInfo spi1_ck_tbl[] = {
    {PAD_SPI1_CK, PADGPIO_BANK, REG_SPI1_CK_GPIO_MODE, REG_SPI1_CK_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI1_CK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_SPI1_CK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_SPI1_CK, PADTOP_BANK, REG_SPI1_MODE, REG_SPI1_MODE_MASK, BIT4, PINMUX_FOR_SPI1_MODE_1},
};
const ST_PadMuxInfo spi1_cz_tbl[] = {
    {PAD_SPI1_CZ, PADGPIO_BANK, REG_SPI1_CZ_GPIO_MODE, REG_SPI1_CZ_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SPI1_CZ, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_SPI1_CZ, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_SPI1_CZ, PADTOP_BANK, REG_SPI1_MODE, REG_SPI1_MODE_MASK, BIT4, PINMUX_FOR_SPI1_MODE_1},
};
const ST_PadMuxInfo pwm0_tbl[] = {
    {PAD_PWM0, PADGPIO_BANK, REG_PWM0_GPIO_MODE, REG_PWM0_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_PWM0, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_EJ_MODE_6},
    {PAD_PWM0, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_DLA_EJ_MODE_6},
    {PAD_PWM0, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_CA7_EJ_MODE_6},
    {PAD_PWM0, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT6 | BIT5, PINMUX_FOR_DSP_EJ_MODE_6},
    {PAD_PWM0, PADTOP_BANK, REG_SPI2_MODE, REG_SPI2_MODE_MASK, BIT8, PINMUX_FOR_SPI2_MODE_1},
    {PAD_PWM0, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_FUART_MODE_5},
    {PAD_PWM0, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT14 | BIT12, PINMUX_FOR_FUART_2W_MODE_5},
    {PAD_PWM0, PADTOP_BANK, REG_UART0_MODE, REG_UART0_MODE_MASK, BIT1, PINMUX_FOR_UART0_MODE_2},
    {PAD_PWM0, PADTOP_BANK, REG_UART5_MODE, REG_UART5_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_UART5_MODE_3},
    {PAD_PWM0, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT0, PINMUX_FOR_SD2_MODE_1},
    {PAD_PWM0, PADTOP_BANK, REG_PWM0_MODE, REG_PWM0_MODE_MASK, BIT0, PINMUX_FOR_PWM0_MODE_1},
    {PAD_PWM0, PADTOP_BANK, REG_I2S0_MCK_MODE, REG_I2S0_MCK_MODE_MASK, BIT2 | BIT1 | BIT0, PINMUX_FOR_I2S0_MCK_MODE_7},
    {PAD_PWM0, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT3 | BIT1, PINMUX_FOR_SR0_SLAVE_MODE_10},
    {PAD_PWM0, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT5 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_11},
};
const ST_PadMuxInfo pwm1_tbl[] = {
    {PAD_PWM1, PADGPIO_BANK, REG_PWM1_GPIO_MODE, REG_PWM1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_PWM1, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_EJ_MODE_6},
    {PAD_PWM1, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_DLA_EJ_MODE_6},
    {PAD_PWM1, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_CA7_EJ_MODE_6},
    {PAD_PWM1, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT6 | BIT5, PINMUX_FOR_DSP_EJ_MODE_6},
    {PAD_PWM1, PADTOP_BANK, REG_SPI2_MODE, REG_SPI2_MODE_MASK, BIT8, PINMUX_FOR_SPI2_MODE_1},
    {PAD_PWM1, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_FUART_MODE_5},
    {PAD_PWM1, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT14 | BIT12, PINMUX_FOR_FUART_2W_MODE_5},
    {PAD_PWM1, PADTOP_BANK, REG_UART0_MODE, REG_UART0_MODE_MASK, BIT1, PINMUX_FOR_UART0_MODE_2},
    {PAD_PWM1, PADTOP_BANK, REG_UART5_MODE, REG_UART5_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_UART5_MODE_3},
    {PAD_PWM1, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT0, PINMUX_FOR_SD2_MODE_1},
    {PAD_PWM1, PADTOP_BANK, REG_PWM1_MODE, REG_PWM1_MODE_MASK, BIT4, PINMUX_FOR_PWM1_MODE_1},
    {PAD_PWM1, PADTOP_BANK, REG_I2S1_MCK_MODE, REG_I2S1_MCK_MODE_MASK, BIT7, PINMUX_FOR_I2S1_MCK_MODE_8},
    {PAD_PWM1, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_DMIC_4CH_MODE_5},
    {PAD_PWM1, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT3 | BIT1, PINMUX_FOR_SR0_SLAVE_MODE_10},
    {PAD_PWM1, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT5 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_11},
};
const ST_PadMuxInfo pwm2_tbl[] = {
    {PAD_PWM2, PADGPIO_BANK, REG_PWM2_GPIO_MODE, REG_PWM2_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_PWM2, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_EJ_MODE_6},
    {PAD_PWM2, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_DLA_EJ_MODE_6},
    {PAD_PWM2, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_CA7_EJ_MODE_6},
    {PAD_PWM2, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT6 | BIT5, PINMUX_FOR_DSP_EJ_MODE_6},
    {PAD_PWM2, PADTOP_BANK, REG_I2C8_MODE, REG_I2C8_MODE_MASK, BIT10, PINMUX_FOR_I2C8_MODE_4},
    {PAD_PWM2, PADTOP_BANK, REG_SPI2_MODE, REG_SPI2_MODE_MASK, BIT8, PINMUX_FOR_SPI2_MODE_1},
    {PAD_PWM2, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_FUART_MODE_5},
    {PAD_PWM2, PADTOP_BANK, REG_UART4_MODE, REG_UART4_MODE_MASK, BIT0, PINMUX_FOR_UART4_MODE_1},
    {PAD_PWM2, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT0, PINMUX_FOR_SD2_MODE_1},
    {PAD_PWM2, PADTOP_BANK, REG_PWM2_MODE, REG_PWM2_MODE_MASK, BIT8, PINMUX_FOR_PWM2_MODE_1},
    {PAD_PWM2, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_DMIC_MODE_5},
    {PAD_PWM2, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_DMIC_4CH_MODE_5},
    {PAD_PWM2, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT3 | BIT1, PINMUX_FOR_SR0_SLAVE_MODE_10},
    {PAD_PWM2, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT5 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_11},
};
const ST_PadMuxInfo pwm3_tbl[] = {
    {PAD_PWM3, PADGPIO_BANK, REG_PWM3_GPIO_MODE, REG_PWM3_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_PWM3, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_EJ_MODE_6},
    {PAD_PWM3, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_DLA_EJ_MODE_6},
    {PAD_PWM3, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_CA7_EJ_MODE_6},
    {PAD_PWM3, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT6 | BIT5, PINMUX_FOR_DSP_EJ_MODE_6},
    {PAD_PWM3, PADTOP_BANK, REG_I2C8_MODE, REG_I2C8_MODE_MASK, BIT10, PINMUX_FOR_I2C8_MODE_4},
    {PAD_PWM3, PADTOP_BANK, REG_SPI2_MODE, REG_SPI2_MODE_MASK, BIT8, PINMUX_FOR_SPI2_MODE_1},
    {PAD_PWM3, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_FUART_MODE_5},
    {PAD_PWM3, PADTOP_BANK, REG_UART4_MODE, REG_UART4_MODE_MASK, BIT0, PINMUX_FOR_UART4_MODE_1},
    {PAD_PWM3, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT0, PINMUX_FOR_SD2_MODE_1},
    {PAD_PWM3, PADTOP_BANK, REG_PWM3_MODE, REG_PWM3_MODE_MASK, BIT12, PINMUX_FOR_PWM3_MODE_1},
    {PAD_PWM3, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_DMIC_MODE_5},
    {PAD_PWM3, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_DMIC_4CH_MODE_5},
    {PAD_PWM3, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT3 | BIT0, PINMUX_FOR_SR0_SLAVE_MODE_9},
    {PAD_PWM3, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT5, PINMUX_FOR_SR1_SLAVE_MODE_10},
};
const ST_PadMuxInfo gpio0_tbl[] = {
    {PAD_GPIO0, PADGPIO_BANK, REG_GPIO0_GPIO_MODE, REG_GPIO0_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO0, PADTOP_BANK, REG_I2C10_MODE, REG_I2C10_MODE_MASK, BIT1, PINMUX_FOR_I2C10_MODE_2},
    {PAD_GPIO0, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT0, PINMUX_FOR_SPI3_MODE_1},
    {PAD_GPIO0, PADTOP_BANK, REG_UART1_MODE, REG_UART1_MODE_MASK, BIT5, PINMUX_FOR_UART1_MODE_2},
    {PAD_GPIO0, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT0, PINMUX_FOR_SD2_MODE_1},
    {PAD_GPIO0, PADTOP_BANK, REG_PWM4_MODE, REG_PWM4_MODE_MASK, BIT0, PINMUX_FOR_PWM4_MODE_1},
    {PAD_GPIO0, PADTOP_BANK, REG_I2S0_MCK_MODE, REG_I2S0_MCK_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_I2S0_MCK_MODE_5},
    {PAD_GPIO0, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT3, PINMUX_FOR_DMIC_4CH_MODE_8},
    {PAD_GPIO0, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
};
const ST_PadMuxInfo gpio1_tbl[] = {
    {PAD_GPIO1, PADGPIO_BANK, REG_GPIO1_GPIO_MODE, REG_GPIO1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO1, PADTOP_BANK, REG_I2C10_MODE, REG_I2C10_MODE_MASK, BIT1, PINMUX_FOR_I2C10_MODE_2},
    {PAD_GPIO1, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT0, PINMUX_FOR_SPI3_MODE_1},
    {PAD_GPIO1, PADTOP_BANK, REG_UART1_MODE, REG_UART1_MODE_MASK, BIT5, PINMUX_FOR_UART1_MODE_2},
    {PAD_GPIO1, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT0, PINMUX_FOR_SD2_MODE_1},
    {PAD_GPIO1, PADTOP_BANK, REG_PWM5_MODE, REG_PWM5_MODE_MASK, BIT4, PINMUX_FOR_PWM5_MODE_1},
    {PAD_GPIO1, PADTOP_BANK, REG_PWM7_MODE, REG_PWM7_MODE_MASK, BIT14 | BIT13 | BIT12, PINMUX_FOR_PWM7_MODE_7},
    {PAD_GPIO1, PADTOP_BANK, REG_PWM8_MODE, REG_PWM8_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_PWM8_MODE_5},
    {PAD_GPIO1, PADTOP_BANK, REG_I2S1_MCK_MODE, REG_I2S1_MCK_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_I2S1_MCK_MODE_5},
    {PAD_GPIO1, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT3, PINMUX_FOR_DMIC_MODE_8},
    {PAD_GPIO1, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT3, PINMUX_FOR_DMIC_4CH_MODE_8},
    {PAD_GPIO1, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
};
const ST_PadMuxInfo gpio2_tbl[] = {
    {PAD_GPIO2, PADGPIO_BANK, REG_GPIO2_GPIO_MODE, REG_GPIO2_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO2, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT0, PINMUX_FOR_SPI3_MODE_1},
    {PAD_GPIO2, PADTOP_BANK, REG_UART2_MODE, REG_UART2_MODE_MASK, BIT9, PINMUX_FOR_UART2_MODE_2},
    {PAD_GPIO2, PADTOP_BANK, REG_SD2_CDZ_MODE, REG_SD2_CDZ_MODE_MASK, BIT4, PINMUX_FOR_SD2_CDZ_MODE_1},
    {PAD_GPIO2, PADTOP_BANK, REG_PWM6_MODE, REG_PWM6_MODE_MASK, BIT8, PINMUX_FOR_PWM6_MODE_1},
    {PAD_GPIO2, PADTOP_BANK, REG_PWM9_MODE, REG_PWM9_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_PWM9_MODE_3},
    {PAD_GPIO2, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT3, PINMUX_FOR_DMIC_MODE_8},
    {PAD_GPIO2, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT3, PINMUX_FOR_DMIC_4CH_MODE_8},
    {PAD_GPIO2, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
};
const ST_PadMuxInfo gpio3_tbl[] = {
    {PAD_GPIO3, PADGPIO_BANK, REG_GPIO3_GPIO_MODE, REG_GPIO3_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO3, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT0, PINMUX_FOR_SPI3_MODE_1},
    {PAD_GPIO3, PADTOP_BANK, REG_UART2_MODE, REG_UART2_MODE_MASK, BIT9, PINMUX_FOR_UART2_MODE_2},
    {PAD_GPIO3, PADTOP_BANK, REG_PWM7_MODE, REG_PWM7_MODE_MASK, BIT12, PINMUX_FOR_PWM7_MODE_1},
};
const ST_PadMuxInfo gpio4_tbl[] = {
    {PAD_GPIO4, PADGPIO_BANK, REG_GPIO4_GPIO_MODE, REG_GPIO4_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO4, PM_PADTOP_BANK, REG_SPICSZ2_MODE, REG_SPICSZ2_MODE_MASK, BIT0, PINMUX_FOR_SPICSZ2_MODE_1},
    {PAD_GPIO4, PADTOP_BANK, REG_I2C8_MODE, REG_I2C8_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_I2C8_MODE_3},
    {PAD_GPIO4, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT1, PINMUX_FOR_SPI0_MODE_2},
    {PAD_GPIO4, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_FUART_MODE_3},
    {PAD_GPIO4, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_FUART_2W_MODE_3},
    {PAD_GPIO4, PADTOP_BANK, REG_PWM8_MODE, REG_PWM8_MODE_MASK, BIT0, PINMUX_FOR_PWM8_MODE_1},
    {PAD_GPIO4, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT5, PINMUX_FOR_I2S1_RXTX_MODE_2},
    {PAD_GPIO4, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_SR0_SLAVE_MODE_6},
    {PAD_GPIO4, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT6 | BIT5 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_7},
    {PAD_GPIO4, PADTOP_BANK, REG_SATA0_LED_MODE, REG_SATA0_LED_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SATA0_LED_MODE_3},
};
const ST_PadMuxInfo gpio5_tbl[] = {
    {PAD_GPIO5, PADGPIO_BANK, REG_GPIO5_GPIO_MODE, REG_GPIO5_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO5, PM_PADTOP_BANK, REG_SPICSZ2_MODE, REG_SPICSZ2_MODE_MASK, BIT1, PINMUX_FOR_SPICSZ2_MODE_2},
    {PAD_GPIO5, PADTOP_BANK, REG_I2C8_MODE, REG_I2C8_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_I2C8_MODE_3},
    {PAD_GPIO5, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT1, PINMUX_FOR_SPI0_MODE_2},
    {PAD_GPIO5, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_FUART_MODE_3},
    {PAD_GPIO5, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_FUART_2W_MODE_3},
    {PAD_GPIO5, PADTOP_BANK, REG_PWM9_MODE, REG_PWM9_MODE_MASK, BIT4, PINMUX_FOR_PWM9_MODE_1},
    {PAD_GPIO5, PADTOP_BANK, REG_I2S2_RX_MODE, REG_I2S2_RX_MODE_MASK, BIT9, PINMUX_FOR_I2S2_RX_MODE_2},
    {PAD_GPIO5, PADTOP_BANK, REG_I2S0_TX_MODE, REG_I2S0_TX_MODE_MASK, BIT13, PINMUX_FOR_I2S0_TX_MODE_2},
    {PAD_GPIO5, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT5, PINMUX_FOR_I2S1_RXTX_MODE_2},
    {PAD_GPIO5, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_DMIC_4CH_MODE_6},
    {PAD_GPIO5, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_SR0_SLAVE_MODE_6},
    {PAD_GPIO5, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT6 | BIT5 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_7},
    {PAD_GPIO5, PADTOP_BANK, REG_SATA1_LED_MODE, REG_SATA1_LED_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_SATA1_LED_MODE_3},
};
const ST_PadMuxInfo gpio6_tbl[] = {
    {PAD_GPIO6, PADGPIO_BANK, REG_GPIO6_GPIO_MODE, REG_GPIO6_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO6, PADTOP_BANK, REG_I2C9_MODE, REG_I2C9_MODE_MASK, BIT13, PINMUX_FOR_I2C9_MODE_2},
    {PAD_GPIO6, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT1, PINMUX_FOR_SPI0_MODE_2},
    {PAD_GPIO6, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_FUART_MODE_3},
    {PAD_GPIO6, PADTOP_BANK, REG_PWM10_MODE, REG_PWM10_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_PWM10_MODE_6},
    {PAD_GPIO6, PADTOP_BANK, REG_I2S2_RX_MODE, REG_I2S2_RX_MODE_MASK, BIT9, PINMUX_FOR_I2S2_RX_MODE_2},
    {PAD_GPIO6, PADTOP_BANK, REG_I2S0_TX_MODE, REG_I2S0_TX_MODE_MASK, BIT13, PINMUX_FOR_I2S0_TX_MODE_2},
    {PAD_GPIO6, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT5, PINMUX_FOR_I2S1_RXTX_MODE_2},
    {PAD_GPIO6, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_DMIC_MODE_6},
    {PAD_GPIO6, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_DMIC_4CH_MODE_6},
    {PAD_GPIO6, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_SR0_SLAVE_MODE_6},
    {PAD_GPIO6, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT6 | BIT5 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_7},
};
const ST_PadMuxInfo gpio7_tbl[] = {
    {PAD_GPIO7, PADGPIO_BANK, REG_GPIO7_GPIO_MODE, REG_GPIO7_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO7, PADTOP_BANK, REG_I2C9_MODE, REG_I2C9_MODE_MASK, BIT13, PINMUX_FOR_I2C9_MODE_2},
    {PAD_GPIO7, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT1, PINMUX_FOR_SPI0_MODE_2},
    {PAD_GPIO7, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_FUART_MODE_3},
    {PAD_GPIO7, PADTOP_BANK, REG_PWM11_MODE, REG_PWM11_MODE_MASK, BIT10 | BIT9 | BIT8, PINMUX_FOR_PWM11_MODE_7},
    {PAD_GPIO7, PADTOP_BANK, REG_I2S2_RX_MODE, REG_I2S2_RX_MODE_MASK, BIT9, PINMUX_FOR_I2S2_RX_MODE_2},
    {PAD_GPIO7, PADTOP_BANK, REG_I2S0_TX_MODE, REG_I2S0_TX_MODE_MASK, BIT13, PINMUX_FOR_I2S0_TX_MODE_2},
    {PAD_GPIO7, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT5, PINMUX_FOR_I2S1_RXTX_MODE_2},
    {PAD_GPIO7, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_DMIC_MODE_6},
    {PAD_GPIO7, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_DMIC_4CH_MODE_6},
};
const ST_PadMuxInfo bt1120_d0_tbl[] = {
    {PAD_BT1120_D0, PADTOP_BANK, REG_BT1120_D0_EXT_EN_MODE, REG_BT1120_D0_EXT_EN_MODE_MASK, BIT0, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D0, PADGPIO_BANK, REG_BT1120_D0_GPIO_MODE, REG_BT1120_D0_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D0, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D0, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D0, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D0, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT8, PINMUX_FOR_RGB8_MODE_1},
    {PAD_BT1120_D0, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D0, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_D0, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D0, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D0, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT656_OUT_MODE_1},
    {PAD_BT1120_D0, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT601_OUT_MODE_1},
    {PAD_BT1120_D0, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d1_tbl[] = {
    {PAD_BT1120_D1, PADTOP_BANK, REG_BT1120_D1_EXT_EN_MODE, REG_BT1120_D1_EXT_EN_MODE_MASK, BIT1, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D1, PADGPIO_BANK, REG_BT1120_D1_GPIO_MODE, REG_BT1120_D1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D1, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D1, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D1, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D1, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT8, PINMUX_FOR_RGB8_MODE_1},
    {PAD_BT1120_D1, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D1, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_D1, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D1, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D1, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT656_OUT_MODE_1},
    {PAD_BT1120_D1, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT601_OUT_MODE_1},
    {PAD_BT1120_D1, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d2_tbl[] = {
    {PAD_BT1120_D2, PADTOP_BANK, REG_BT1120_D2_EXT_EN_MODE, REG_BT1120_D2_EXT_EN_MODE_MASK, BIT2, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D2, PADGPIO_BANK, REG_BT1120_D2_GPIO_MODE, REG_BT1120_D2_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D2, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D2, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D2, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D2, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT8, PINMUX_FOR_RGB8_MODE_1},
    {PAD_BT1120_D2, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D2, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_D2, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D2, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D2, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT656_OUT_MODE_1},
    {PAD_BT1120_D2, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT601_OUT_MODE_1},
    {PAD_BT1120_D2, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d3_tbl[] = {
    {PAD_BT1120_D3, PADTOP_BANK, REG_BT1120_D3_EXT_EN_MODE, REG_BT1120_D3_EXT_EN_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D3, PADGPIO_BANK, REG_BT1120_D3_GPIO_MODE, REG_BT1120_D3_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D3, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D3, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D3, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D3, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT8, PINMUX_FOR_RGB8_MODE_1},
    {PAD_BT1120_D3, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D3, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_D3, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D3, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D3, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT656_OUT_MODE_1},
    {PAD_BT1120_D3, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT601_OUT_MODE_1},
    {PAD_BT1120_D3, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d4_tbl[] = {
    {PAD_BT1120_D4, PADTOP_BANK, REG_BT1120_D4_EXT_EN_MODE, REG_BT1120_D4_EXT_EN_MODE_MASK, BIT4, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D4, PADGPIO_BANK, REG_BT1120_D4_GPIO_MODE, REG_BT1120_D4_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D4, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D4, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D4, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D4, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT8, PINMUX_FOR_RGB8_MODE_1},
    {PAD_BT1120_D4, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D4, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_D4, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D4, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D4, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT656_OUT_MODE_1},
    {PAD_BT1120_D4, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT601_OUT_MODE_1},
    {PAD_BT1120_D4, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d5_tbl[] = {
    {PAD_BT1120_D5, PADTOP_BANK, REG_BT1120_D5_EXT_EN_MODE, REG_BT1120_D5_EXT_EN_MODE_MASK, BIT5, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D5, PADGPIO_BANK, REG_BT1120_D5_GPIO_MODE, REG_BT1120_D5_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D5, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D5, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D5, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D5, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT8, PINMUX_FOR_RGB8_MODE_1},
    {PAD_BT1120_D5, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D5, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_D5, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D5, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D5, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT656_OUT_MODE_1},
    {PAD_BT1120_D5, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT601_OUT_MODE_1},
    {PAD_BT1120_D5, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d6_tbl[] = {
    {PAD_BT1120_D6, PADTOP_BANK, REG_BT1120_D6_EXT_EN_MODE, REG_BT1120_D6_EXT_EN_MODE_MASK, BIT6, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D6, PADGPIO_BANK, REG_BT1120_D6_GPIO_MODE, REG_BT1120_D6_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D6, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D6, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D6, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D6, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT8, PINMUX_FOR_RGB8_MODE_1},
    {PAD_BT1120_D6, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D6, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_D6, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D6, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D6, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT656_OUT_MODE_1},
    {PAD_BT1120_D6, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT601_OUT_MODE_1},
    {PAD_BT1120_D6, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d7_tbl[] = {
    {PAD_BT1120_D7, PADTOP_BANK, REG_BT1120_D7_EXT_EN_MODE, REG_BT1120_D7_EXT_EN_MODE_MASK, BIT7, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D7, PADGPIO_BANK, REG_BT1120_D7_GPIO_MODE, REG_BT1120_D7_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D7, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D7, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D7, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D7, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT8, PINMUX_FOR_RGB8_MODE_1},
    {PAD_BT1120_D7, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D7, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_D7, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D7, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D7, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT656_OUT_MODE_1},
    {PAD_BT1120_D7, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT601_OUT_MODE_1},
    {PAD_BT1120_D7, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_clk_tbl[] = {
    {PAD_BT1120_CLK, PADTOP_BANK, REG_BT1120_CLK_EXT_EN_MODE, REG_BT1120_CLK_EXT_EN_MODE_MASK, BIT8,
     PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_CLK, PADGPIO_BANK, REG_BT1120_CLK_GPIO_MODE, REG_BT1120_CLK_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_CLK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_CLK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_CLK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_CLK, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT8, PINMUX_FOR_RGB8_MODE_1},
    {PAD_BT1120_CLK, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_CLK, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_CLK, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_CLK, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_CLK, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT656_OUT_MODE_1},
    {PAD_BT1120_CLK, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT601_OUT_MODE_1},
    {PAD_BT1120_CLK, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d8_tbl[] = {
    {PAD_BT1120_D8, PADTOP_BANK, REG_BT1120_D8_EXT_EN_MODE, REG_BT1120_D8_EXT_EN_MODE_MASK, BIT9, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D8, PADGPIO_BANK, REG_BT1120_D8_GPIO_MODE, REG_BT1120_D8_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D8, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D8, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D8, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D8, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT8, PINMUX_FOR_RGB8_MODE_1},
    {PAD_BT1120_D8, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D8, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_D8, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D8, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D8, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT656_OUT_MODE_2},
    {PAD_BT1120_D8, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT601_OUT_MODE_1},
    {PAD_BT1120_D8, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d9_tbl[] = {
    {PAD_BT1120_D9, PADTOP_BANK, REG_BT1120_D9_EXT_EN_MODE, REG_BT1120_D9_EXT_EN_MODE_MASK, BIT10,
     PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D9, PADGPIO_BANK, REG_BT1120_D9_GPIO_MODE, REG_BT1120_D9_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D9, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D9, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D9, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D9, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT8, PINMUX_FOR_RGB8_MODE_1},
    {PAD_BT1120_D9, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D9, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_D9, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D9, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D9, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT656_OUT_MODE_2},
    {PAD_BT1120_D9, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT601_OUT_MODE_1},
    {PAD_BT1120_D9, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d10_tbl[] = {
    {PAD_BT1120_D10, PADTOP_BANK, REG_BT1120_D10_EXT_EN_MODE, REG_BT1120_D10_EXT_EN_MODE_MASK, BIT11,
     PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D10, PADGPIO_BANK, REG_BT1120_D10_GPIO_MODE, REG_BT1120_D10_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D10, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D10, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D10, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D10, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT8, PINMUX_FOR_RGB8_MODE_1},
    {PAD_BT1120_D10, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D10, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_D10, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D10, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D10, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT656_OUT_MODE_2},
    {PAD_BT1120_D10, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT4, PINMUX_FOR_BT601_OUT_MODE_1},
    {PAD_BT1120_D10, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d11_tbl[] = {
    {PAD_BT1120_D11, PADTOP_BANK, REG_BT1120_D11_EXT_EN_MODE, REG_BT1120_D11_EXT_EN_MODE_MASK, BIT12,
     PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D11, PADGPIO_BANK, REG_BT1120_D11_GPIO_MODE, REG_BT1120_D11_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D11, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D11, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D11, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D11, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D11, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT0, PINMUX_FOR_LCD_MCU8_MODE_1},
    {PAD_BT1120_D11, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D11, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D11, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT656_OUT_MODE_2},
    {PAD_BT1120_D11, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d12_tbl[] = {
    {PAD_BT1120_D12, PADTOP_BANK, REG_BT1120_D12_EXT_EN_MODE, REG_BT1120_D12_EXT_EN_MODE_MASK, BIT13,
     PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D12, PADGPIO_BANK, REG_BT1120_D12_GPIO_MODE, REG_BT1120_D12_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D12, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D12, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D12, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D12, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D12, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D12, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D12, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT656_OUT_MODE_2},
    {PAD_BT1120_D12, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d13_tbl[] = {
    {PAD_BT1120_D13, PADTOP_BANK, REG_BT1120_D13_EXT_EN_MODE, REG_BT1120_D13_EXT_EN_MODE_MASK, BIT14,
     PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D13, PADGPIO_BANK, REG_BT1120_D13_GPIO_MODE, REG_BT1120_D13_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D13, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D13, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D13, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D13, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D13, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D13, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D13, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT656_OUT_MODE_2},
    {PAD_BT1120_D13, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d14_tbl[] = {
    {PAD_BT1120_D14, PADTOP_BANK, REG_BT1120_D14_EXT_EN_MODE, REG_BT1120_D14_EXT_EN_MODE_MASK, BIT15,
     PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D14, PADGPIO_BANK, REG_BT1120_D14_GPIO_MODE, REG_BT1120_D14_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D14, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D14, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D14, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D14, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D14, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D14, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D14, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT656_OUT_MODE_2},
    {PAD_BT1120_D14, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_d15_tbl[] = {
    {PAD_BT1120_D15, PADTOP_BANK, REG_BT1120_D15_EXT_EN_MODE, REG_BT1120_D15_EXT_EN_MODE_MASK, BIT0,
     PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D15, PADGPIO_BANK, REG_BT1120_D15_GPIO_MODE, REG_BT1120_D15_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_D15, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_D15, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_D15, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_D15, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_D15, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_D15, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_D15, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT656_OUT_MODE_2},
    {PAD_BT1120_D15, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT0, PINMUX_FOR_BT1120_OUT_MODE_1},
};
const ST_PadMuxInfo bt1120_gpio0_tbl[] = {
    {PAD_BT1120_GPIO0, PADGPIO_BANK, REG_BT1120_GPIO0_GPIO_MODE, REG_BT1120_GPIO0_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_GPIO0, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_GPIO0, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_GPIO0, PADTOP_BANK, REG_I2C3_MODE, REG_I2C3_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_I2C3_MODE_6},
    {PAD_BT1120_GPIO0, PADTOP_BANK, REG_I2C8_MODE, REG_I2C8_MODE_MASK, BIT8, PINMUX_FOR_I2C8_MODE_1},
    {PAD_BT1120_GPIO0, PADTOP_BANK, REG_UART2_MODE, REG_UART2_MODE_MASK, BIT10, PINMUX_FOR_UART2_MODE_4},
    {PAD_BT1120_GPIO0, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_GPIO0, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_GPIO0, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_GPIO0, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_BT1120_GPIO0, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT656_OUT_MODE_2},
};
const ST_PadMuxInfo bt1120_gpio1_tbl[] = {
    {PAD_BT1120_GPIO1, PADGPIO_BANK, REG_BT1120_GPIO1_GPIO_MODE, REG_BT1120_GPIO1_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_GPIO1, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_GPIO1, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_GPIO1, PADTOP_BANK, REG_I2C3_MODE, REG_I2C3_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_I2C3_MODE_6},
    {PAD_BT1120_GPIO1, PADTOP_BANK, REG_I2C8_MODE, REG_I2C8_MODE_MASK, BIT8, PINMUX_FOR_I2C8_MODE_1},
    {PAD_BT1120_GPIO1, PADTOP_BANK, REG_UART2_MODE, REG_UART2_MODE_MASK, BIT10, PINMUX_FOR_UART2_MODE_4},
    {PAD_BT1120_GPIO1, PADTOP_BANK, REG_I2S0_MCK_MODE, REG_I2S0_MCK_MODE_MASK, BIT1, PINMUX_FOR_I2S0_MCK_MODE_2},
    {PAD_BT1120_GPIO1, PADTOP_BANK, REG_SR10_MCLK_MODE, REG_SR10_MCLK_MODE_MASK, BIT9 | BIT8,
     PINMUX_FOR_SR10_MCLK_MODE_3},
    {PAD_BT1120_GPIO1, PADTOP_BANK, REG_SR10_RST_MODE, REG_SR10_RST_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_SR10_RST_MODE_3},
    {PAD_BT1120_GPIO1, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_GPIO1, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_GPIO1, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_GPIO1, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
};
const ST_PadMuxInfo bt1120_gpio2_tbl[] = {
    {PAD_BT1120_GPIO2, PADGPIO_BANK, REG_BT1120_GPIO2_GPIO_MODE, REG_BT1120_GPIO2_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_BT1120_GPIO2, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_BT1120_GPIO2, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_BT1120_GPIO2, PADTOP_BANK, REG_I2S1_MCK_MODE, REG_I2S1_MCK_MODE_MASK, BIT5, PINMUX_FOR_I2S1_MCK_MODE_2},
    {PAD_BT1120_GPIO2, PADTOP_BANK, REG_SR10_PDN_MODE, REG_SR10_PDN_MODE_MASK, BIT9, PINMUX_FOR_SR10_PDN_MODE_2},
    {PAD_BT1120_GPIO2, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_BT1120_GPIO2, PADTOP_BANK, REG_RGB16_MODE, REG_RGB16_MODE_MASK, BIT12, PINMUX_FOR_RGB16_MODE_1},
    {PAD_BT1120_GPIO2, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_BT1120_GPIO2, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
};
const ST_PadMuxInfo sr_gpio0_tbl[] = {
    {PAD_SR_GPIO0, PADGPIO_BANK, REG_SR_GPIO0_GPIO_MODE, REG_SR_GPIO0_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_GPIO0, PADTOP_BANK, REG_PWM0_MODE, REG_PWM0_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_PWM0_MODE_5},
    {PAD_SR_GPIO0, PADTOP_BANK, REG_SR10_RST_MODE, REG_SR10_RST_MODE_MASK, BIT9, PINMUX_FOR_SR10_RST_MODE_2},
    {PAD_SR_GPIO0, PADTOP_BANK, REG_SR13_RST_MODE, REG_SR13_RST_MODE_MASK, BIT15, PINMUX_FOR_SR13_RST_MODE_2},
    {PAD_SR_GPIO0, PADTOP_BANK, REG_SR10_PDN_MODE, REG_SR10_PDN_MODE_MASK, BIT8, PINMUX_FOR_SR10_PDN_MODE_1},
    {PAD_SR_GPIO0, PADTOP_BANK, REG_SR3_SYNC_MODE, REG_SR3_SYNC_MODE_MASK, BIT15, PINMUX_FOR_SR3_SYNC_MODE_2},
    {PAD_SR_GPIO0, PADTOP_BANK, REG_ISP0_IR_OUT_MODE, REG_ISP0_IR_OUT_MODE_MASK, BIT8, PINMUX_FOR_ISP0_IR_OUT_MODE_1},
    {PAD_SR_GPIO0, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT0, PINMUX_FOR_SR0_SLAVE_MODE_1},
    {PAD_SR_GPIO0, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT5, PINMUX_FOR_SR1_SLAVE_MODE_2},
};
const ST_PadMuxInfo sr_gpio1_tbl[] = {
    {PAD_SR_GPIO1, PADGPIO_BANK, REG_SR_GPIO1_GPIO_MODE, REG_SR_GPIO1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_GPIO1, PADTOP_BANK, REG_PWM1_MODE, REG_PWM1_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_PWM1_MODE_5},
    {PAD_SR_GPIO1, PADTOP_BANK, REG_SR10_MCLK_MODE, REG_SR10_MCLK_MODE_MASK, BIT9, PINMUX_FOR_SR10_MCLK_MODE_2},
    {PAD_SR_GPIO1, PADTOP_BANK, REG_SR13_MCLK_MODE, REG_SR13_MCLK_MODE_MASK, BIT15, PINMUX_FOR_SR13_MCLK_MODE_2},
    {PAD_SR_GPIO1, PADTOP_BANK, REG_SR11_PDN_MODE, REG_SR11_PDN_MODE_MASK, BIT10, PINMUX_FOR_SR11_PDN_MODE_1},
    {PAD_SR_GPIO1, PADTOP_BANK, REG_SR3_SYNC_MODE, REG_SR3_SYNC_MODE_MASK, BIT15, PINMUX_FOR_SR3_SYNC_MODE_2},
    {PAD_SR_GPIO1, PADTOP_BANK, REG_ISP1_IR_OUT_MODE, REG_ISP1_IR_OUT_MODE_MASK, BIT12, PINMUX_FOR_ISP1_IR_OUT_MODE_1},
    {PAD_SR_GPIO1, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT0, PINMUX_FOR_SR0_SLAVE_MODE_1},
    {PAD_SR_GPIO1, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT5, PINMUX_FOR_SR1_SLAVE_MODE_2},
};
const ST_PadMuxInfo sr_gpio2_tbl[] = {
    {PAD_SR_GPIO2, PADGPIO_BANK, REG_SR_GPIO2_GPIO_MODE, REG_SR_GPIO2_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_GPIO2, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_SR_GPIO2, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_SR_GPIO2, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT9 | BIT8, PINMUX_FOR_I2C2_MODE_11},
    {PAD_SR_GPIO2, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10, PINMUX_FOR_I2C2_MODE_12},
    {PAD_SR_GPIO2, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT8, PINMUX_FOR_I2C2_MODE_13},
    {PAD_SR_GPIO2, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9, PINMUX_FOR_I2C2_MODE_14},
    {PAD_SR_GPIO2, PADTOP_BANK, REG_I2C4_MODE, REG_I2C4_MODE_MASK, BIT4, PINMUX_FOR_I2C4_MODE_1},
    {PAD_SR_GPIO2, PADTOP_BANK, REG_PWM2_MODE, REG_PWM2_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_PWM2_MODE_5},
    {PAD_SR_GPIO2, PADTOP_BANK, REG_SR12_PDN_MODE, REG_SR12_PDN_MODE_MASK, BIT12, PINMUX_FOR_SR12_PDN_MODE_1},
    {PAD_SR_GPIO2, PADTOP_BANK, REG_SR0_PCLK_MODE, REG_SR0_PCLK_MODE_MASK, BIT0, PINMUX_FOR_SR0_PCLK_MODE_1},
    {PAD_SR_GPIO2, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT0, PINMUX_FOR_SR0_SLAVE_MODE_1},
    {PAD_SR_GPIO2, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT5, PINMUX_FOR_SR1_SLAVE_MODE_2},
};
const ST_PadMuxInfo sr_gpio3_tbl[] = {
    {PAD_SR_GPIO3, PADGPIO_BANK, REG_SR_GPIO3_GPIO_MODE, REG_SR_GPIO3_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_GPIO3, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_SR_GPIO3, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_SR_GPIO3, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT9 | BIT8, PINMUX_FOR_I2C2_MODE_11},
    {PAD_SR_GPIO3, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10, PINMUX_FOR_I2C2_MODE_12},
    {PAD_SR_GPIO3, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT8, PINMUX_FOR_I2C2_MODE_13},
    {PAD_SR_GPIO3, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9, PINMUX_FOR_I2C2_MODE_14},
    {PAD_SR_GPIO3, PADTOP_BANK, REG_I2C4_MODE, REG_I2C4_MODE_MASK, BIT4, PINMUX_FOR_I2C4_MODE_1},
    {PAD_SR_GPIO3, PADTOP_BANK, REG_PWM3_MODE, REG_PWM3_MODE_MASK, BIT14 | BIT12, PINMUX_FOR_PWM3_MODE_5},
    {PAD_SR_GPIO3, PADTOP_BANK, REG_SR00_PDN_MODE, REG_SR00_PDN_MODE_MASK, BIT1, PINMUX_FOR_SR00_PDN_MODE_2},
    {PAD_SR_GPIO3, PADTOP_BANK, REG_SR13_PDN_MODE, REG_SR13_PDN_MODE_MASK, BIT14, PINMUX_FOR_SR13_PDN_MODE_1},
    {PAD_SR_GPIO3, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT4, PINMUX_FOR_SR1_SLAVE_MODE_1},
};
const ST_PadMuxInfo sr_gpio4_tbl[] = {
    {PAD_SR_GPIO4, PADGPIO_BANK, REG_SR_GPIO4_GPIO_MODE, REG_SR_GPIO4_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_GPIO4, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2, PINMUX_FOR_I2C1_MODE_12},
    {PAD_SR_GPIO4, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_SR_GPIO4, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_15},
    {PAD_SR_GPIO4, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT8, PINMUX_FOR_I2C2_MODE_9},
    {PAD_SR_GPIO4, PADTOP_BANK, REG_I2C3_MODE, REG_I2C3_MODE_MASK, BIT1, PINMUX_FOR_I2C3_MODE_2},
    {PAD_SR_GPIO4, PADTOP_BANK, REG_PWM4_MODE, REG_PWM4_MODE_MASK, BIT2, PINMUX_FOR_PWM4_MODE_4},
    {PAD_SR_GPIO4, PADTOP_BANK, REG_SR11_RST_MODE, REG_SR11_RST_MODE_MASK, BIT11, PINMUX_FOR_SR11_RST_MODE_2},
    {PAD_SR_GPIO4, PADTOP_BANK, REG_SR1_PCLK_MODE, REG_SR1_PCLK_MODE_MASK, BIT2, PINMUX_FOR_SR1_PCLK_MODE_1},
    {PAD_SR_GPIO4, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT4, PINMUX_FOR_SR1_SLAVE_MODE_1},
};
const ST_PadMuxInfo sr_gpio5_tbl[] = {
    {PAD_SR_GPIO5, PADGPIO_BANK, REG_SR_GPIO5_GPIO_MODE, REG_SR_GPIO5_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_GPIO5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2, PINMUX_FOR_I2C1_MODE_12},
    {PAD_SR_GPIO5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_SR_GPIO5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_15},
    {PAD_SR_GPIO5, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT8, PINMUX_FOR_I2C2_MODE_9},
    {PAD_SR_GPIO5, PADTOP_BANK, REG_I2C3_MODE, REG_I2C3_MODE_MASK, BIT1, PINMUX_FOR_I2C3_MODE_2},
    {PAD_SR_GPIO5, PADTOP_BANK, REG_PWM5_MODE, REG_PWM5_MODE_MASK, BIT6, PINMUX_FOR_PWM5_MODE_4},
    {PAD_SR_GPIO5, PADTOP_BANK, REG_SR11_MCLK_MODE, REG_SR11_MCLK_MODE_MASK, BIT11, PINMUX_FOR_SR11_MCLK_MODE_2},
    {PAD_SR_GPIO5, PADTOP_BANK, REG_SR01_PDN_MODE, REG_SR01_PDN_MODE_MASK, BIT3, PINMUX_FOR_SR01_PDN_MODE_2},
    {PAD_SR_GPIO5, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT4, PINMUX_FOR_SR1_SLAVE_MODE_1},
};
const ST_PadMuxInfo sr_gpio6_tbl[] = {
    {PAD_SR_GPIO6, PADGPIO_BANK, REG_SR_GPIO6_GPIO_MODE, REG_SR_GPIO6_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT8 | BIT7, PINMUX_FOR_EJ_MODE_3},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_DLA_EJ_MODE_3},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_CA7_EJ_MODE_3},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_DSP_EJ_MODE_3},
    {PAD_SR_GPIO6, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_SR_GPIO6, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2, PINMUX_FOR_I2C1_MODE_12},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_15},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT9, PINMUX_FOR_I2C2_MODE_2},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT8, PINMUX_FOR_I2C2_MODE_9},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT9 | BIT8, PINMUX_FOR_I2C2_MODE_11},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9, PINMUX_FOR_I2C2_MODE_14},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_I2C5_MODE, REG_I2C5_MODE_MASK, BIT8, PINMUX_FOR_I2C5_MODE_1},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_PWM6_MODE, REG_PWM6_MODE_MASK, BIT10, PINMUX_FOR_PWM6_MODE_4},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_PWM10_MODE, REG_PWM10_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_PWM10_MODE_3},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_SR00_PDN_MODE, REG_SR00_PDN_MODE_MASK, BIT0, PINMUX_FOR_SR00_PDN_MODE_1},
    {PAD_SR_GPIO6, PADTOP_BANK, REG_SR2_PCLK_MODE, REG_SR2_PCLK_MODE_MASK, BIT4, PINMUX_FOR_SR2_PCLK_MODE_1},
};
const ST_PadMuxInfo sr_gpio7_tbl[] = {
    {PAD_SR_GPIO7, PADGPIO_BANK, REG_SR_GPIO7_GPIO_MODE, REG_SR_GPIO7_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT8 | BIT7, PINMUX_FOR_EJ_MODE_3},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_DLA_EJ_MODE_3},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_CA7_EJ_MODE_3},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_DSP_EJ_MODE_3},
    {PAD_SR_GPIO7, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_SR_GPIO7, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2, PINMUX_FOR_I2C1_MODE_12},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_15},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT9, PINMUX_FOR_I2C2_MODE_2},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT8, PINMUX_FOR_I2C2_MODE_9},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT9 | BIT8, PINMUX_FOR_I2C2_MODE_11},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9, PINMUX_FOR_I2C2_MODE_14},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_I2C5_MODE, REG_I2C5_MODE_MASK, BIT8, PINMUX_FOR_I2C5_MODE_1},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_PWM7_MODE, REG_PWM7_MODE_MASK, BIT14, PINMUX_FOR_PWM7_MODE_4},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_PWM11_MODE, REG_PWM11_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_PWM11_MODE_3},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_SR01_PDN_MODE, REG_SR01_PDN_MODE_MASK, BIT2, PINMUX_FOR_SR01_PDN_MODE_1},
    {PAD_SR_GPIO7, PADTOP_BANK, REG_SR02_PDN_MODE, REG_SR02_PDN_MODE_MASK, BIT5, PINMUX_FOR_SR02_PDN_MODE_2},
};
const ST_PadMuxInfo sr_gpio8_tbl[] = {
    {PAD_SR_GPIO8, PADGPIO_BANK, REG_SR_GPIO8_GPIO_MODE, REG_SR_GPIO8_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT8 | BIT7, PINMUX_FOR_EJ_MODE_3},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_DLA_EJ_MODE_3},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_CA7_EJ_MODE_3},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_DSP_EJ_MODE_3},
    {PAD_SR_GPIO8, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_SR_GPIO8, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT2, PINMUX_FOR_I2C1_MODE_4},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT1, PINMUX_FOR_I2C1_MODE_10},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2, PINMUX_FOR_I2C1_MODE_12},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_15},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_SR12_RST_MODE, REG_SR12_RST_MODE_MASK, BIT13, PINMUX_FOR_SR12_RST_MODE_2},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_SR00_PDN_MODE, REG_SR00_PDN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SR00_PDN_MODE_3},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_SR02_PDN_MODE, REG_SR02_PDN_MODE_MASK, BIT4, PINMUX_FOR_SR02_PDN_MODE_1},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_SR3_PCLK_MODE, REG_SR3_PCLK_MODE_MASK, BIT6, PINMUX_FOR_SR3_PCLK_MODE_1},
    {PAD_SR_GPIO8, PADTOP_BANK, REG_ISP0_IR_OUT_MODE, REG_ISP0_IR_OUT_MODE_MASK, BIT9, PINMUX_FOR_ISP0_IR_OUT_MODE_2},
};
const ST_PadMuxInfo sr_gpio9_tbl[] = {
    {PAD_SR_GPIO9, PADGPIO_BANK, REG_SR_GPIO9_GPIO_MODE, REG_SR_GPIO9_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT8 | BIT7, PINMUX_FOR_EJ_MODE_3},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_DLA_EJ_MODE_3},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_CA7_EJ_MODE_3},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_DSP_EJ_MODE_3},
    {PAD_SR_GPIO9, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1, PINMUX_FOR_TEST_IN_MODE_2},
    {PAD_SR_GPIO9, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5, PINMUX_FOR_TEST_OUT_MODE_2},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT2, PINMUX_FOR_I2C1_MODE_4},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT1, PINMUX_FOR_I2C1_MODE_10},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2, PINMUX_FOR_I2C1_MODE_12},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_15},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_SR12_MCLK_MODE, REG_SR12_MCLK_MODE_MASK, BIT13, PINMUX_FOR_SR12_MCLK_MODE_2},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_SR01_PDN_MODE, REG_SR01_PDN_MODE_MASK, BIT3 | BIT2, PINMUX_FOR_SR01_PDN_MODE_3},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_SR03_PDN_MODE, REG_SR03_PDN_MODE_MASK, BIT6, PINMUX_FOR_SR03_PDN_MODE_1},
    {PAD_SR_GPIO9, PADTOP_BANK, REG_ISP1_IR_OUT_MODE, REG_ISP1_IR_OUT_MODE_MASK, BIT13, PINMUX_FOR_ISP1_IR_OUT_MODE_2},
};
const ST_PadMuxInfo sr_i2cm_scl_tbl[] = {
    {PAD_SR_I2CM_SCL, PADGPIO_BANK, REG_SR_I2CM_SCL_GPIO_MODE, REG_SR_I2CM_SCL_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_I2C0_MODE, REG_I2C0_MODE_MASK, BIT0, PINMUX_FOR_I2C0_MODE_1},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT1, PINMUX_FOR_I2C1_MODE_2},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT1, PINMUX_FOR_I2C1_MODE_10},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_11},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2, PINMUX_FOR_I2C1_MODE_12},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT0, PINMUX_FOR_I2C1_MODE_13},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0,
     PINMUX_FOR_I2C1_MODE_15},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_PWM10_MODE, REG_PWM10_MODE_MASK, BIT0, PINMUX_FOR_PWM10_MODE_1},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_SR13_RST_MODE, REG_SR13_RST_MODE_MASK, BIT14, PINMUX_FOR_SR13_RST_MODE_1},
    {PAD_SR_I2CM_SCL, PADTOP_BANK, REG_SR02_PDN_MODE, REG_SR02_PDN_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_SR02_PDN_MODE_3},
};
const ST_PadMuxInfo sr_i2cm_sda_tbl[] = {
    {PAD_SR_I2CM_SDA, PADGPIO_BANK, REG_SR_I2CM_SDA_GPIO_MODE, REG_SR_I2CM_SDA_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_I2C0_MODE, REG_I2C0_MODE_MASK, BIT0, PINMUX_FOR_I2C0_MODE_1},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT1, PINMUX_FOR_I2C1_MODE_2},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT1, PINMUX_FOR_I2C1_MODE_10},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_11},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2, PINMUX_FOR_I2C1_MODE_12},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT0, PINMUX_FOR_I2C1_MODE_13},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0,
     PINMUX_FOR_I2C1_MODE_15},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_PWM11_MODE, REG_PWM11_MODE_MASK, BIT8, PINMUX_FOR_PWM11_MODE_1},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_SR13_MCLK_MODE, REG_SR13_MCLK_MODE_MASK, BIT14, PINMUX_FOR_SR13_MCLK_MODE_1},
    {PAD_SR_I2CM_SDA, PADTOP_BANK, REG_SR03_PDN_MODE, REG_SR03_PDN_MODE_MASK, BIT7, PINMUX_FOR_SR03_PDN_MODE_2},
};
const ST_PadMuxInfo sr_rst0_tbl[] = {
    {PAD_SR_RST0, PADGPIO2_BANK, REG_SR_RST0_GPIO_MODE, REG_SR_RST0_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_RST0, PADTOP_BANK, REG_SR00_RST_MODE, REG_SR00_RST_MODE_MASK, BIT0, PINMUX_FOR_SR00_RST_MODE_1},
    {PAD_SR_RST0, PADTOP_BANK, REG_SR03_RST_MODE, REG_SR03_RST_MODE_MASK, BIT7, PINMUX_FOR_SR03_RST_MODE_2},
    {PAD_SR_RST0, PADTOP_BANK, REG_SR1_BT656_MODE, REG_SR1_BT656_MODE_MASK, BIT2, PINMUX_FOR_SR1_BT656_MODE_1},
    {PAD_SR_RST0, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_RST0, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_RST0, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo sr_mclk0_tbl[] = {
    {PAD_SR_MCLK0, PADGPIO2_BANK, REG_SR_MCLK0_GPIO_MODE, REG_SR_MCLK0_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_MCLK0, PADTOP_BANK, REG_SR00_MCLK_MODE, REG_SR00_MCLK_MODE_MASK, BIT0, PINMUX_FOR_SR00_MCLK_MODE_1},
    {PAD_SR_MCLK0, PADTOP_BANK, REG_SR03_MCLK_MODE, REG_SR03_MCLK_MODE_MASK, BIT7, PINMUX_FOR_SR03_MCLK_MODE_2},
    {PAD_SR_MCLK0, PADTOP_BANK, REG_SR1_BT656_MODE, REG_SR1_BT656_MODE_MASK, BIT2, PINMUX_FOR_SR1_BT656_MODE_1},
    {PAD_SR_MCLK0, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_MCLK0, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_MCLK0, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo sr_rst1_tbl[] = {
    {PAD_SR_RST1, PADGPIO2_BANK, REG_SR_RST1_GPIO_MODE, REG_SR_RST1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_RST1, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_SR_RST1, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_SR_RST1, PADTOP_BANK, REG_I2C3_MODE, REG_I2C3_MODE_MASK, BIT2, PINMUX_FOR_I2C3_MODE_4},
    {PAD_SR_RST1, PADTOP_BANK, REG_SR01_RST_MODE, REG_SR01_RST_MODE_MASK, BIT2, PINMUX_FOR_SR01_RST_MODE_1},
    {PAD_SR_RST1, PADTOP_BANK, REG_SR1_BT656_MODE, REG_SR1_BT656_MODE_MASK, BIT2, PINMUX_FOR_SR1_BT656_MODE_1},
    {PAD_SR_RST1, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_RST1, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_RST1, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo sr_mclk1_tbl[] = {
    {PAD_SR_MCLK1, PADGPIO2_BANK, REG_SR_MCLK1_GPIO_MODE, REG_SR_MCLK1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_MCLK1, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_SR_MCLK1, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_SR_MCLK1, PADTOP_BANK, REG_I2C3_MODE, REG_I2C3_MODE_MASK, BIT2, PINMUX_FOR_I2C3_MODE_4},
    {PAD_SR_MCLK1, PADTOP_BANK, REG_SR01_MCLK_MODE, REG_SR01_MCLK_MODE_MASK, BIT2, PINMUX_FOR_SR01_MCLK_MODE_1},
    {PAD_SR_MCLK1, PADTOP_BANK, REG_SR1_BT656_MODE, REG_SR1_BT656_MODE_MASK, BIT2, PINMUX_FOR_SR1_BT656_MODE_1},
    {PAD_SR_MCLK1, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_MCLK1, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_MCLK1, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo sr_rst2_tbl[] = {
    {PAD_SR_RST2, PADGPIO2_BANK, REG_SR_RST2_GPIO_MODE, REG_SR_RST2_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_RST2, PADTOP_BANK, REG_SR02_RST_MODE, REG_SR02_RST_MODE_MASK, BIT4, PINMUX_FOR_SR02_RST_MODE_1},
    {PAD_SR_RST2, PADTOP_BANK, REG_SR02_RST_MODE, REG_SR02_RST_MODE_MASK, BIT5, PINMUX_FOR_SR02_RST_MODE_2},
    {PAD_SR_RST2, PADTOP_BANK, REG_SR1_BT656_MODE, REG_SR1_BT656_MODE_MASK, BIT2, PINMUX_FOR_SR1_BT656_MODE_1},
    {PAD_SR_RST2, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_RST2, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_RST2, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo sr_mclk2_tbl[] = {
    {PAD_SR_MCLK2, PADGPIO2_BANK, REG_SR_MCLK2_GPIO_MODE, REG_SR_MCLK2_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_MCLK2, PADTOP_BANK, REG_SR02_MCLK_MODE, REG_SR02_MCLK_MODE_MASK, BIT4, PINMUX_FOR_SR02_MCLK_MODE_1},
    {PAD_SR_MCLK2, PADTOP_BANK, REG_SR02_MCLK_MODE, REG_SR02_MCLK_MODE_MASK, BIT5, PINMUX_FOR_SR02_MCLK_MODE_2},
    {PAD_SR_MCLK2, PADTOP_BANK, REG_SR1_BT656_MODE, REG_SR1_BT656_MODE_MASK, BIT2, PINMUX_FOR_SR1_BT656_MODE_1},
    {PAD_SR_MCLK2, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_MCLK2, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_MCLK2, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo sr_rst3_tbl[] = {
    {PAD_SR_RST3, PADGPIO2_BANK, REG_SR_RST3_GPIO_MODE, REG_SR_RST3_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_RST3, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_SR_RST3, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_SR_RST3, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT10, PINMUX_FOR_I2C2_MODE_4},
    {PAD_SR_RST3, PADTOP_BANK, REG_SR03_RST_MODE, REG_SR03_RST_MODE_MASK, BIT6, PINMUX_FOR_SR03_RST_MODE_1},
    {PAD_SR_RST3, PADTOP_BANK, REG_SR1_BT656_MODE, REG_SR1_BT656_MODE_MASK, BIT2, PINMUX_FOR_SR1_BT656_MODE_1},
    {PAD_SR_RST3, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_RST3, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_RST3, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo sr_mclk3_tbl[] = {
    {PAD_SR_MCLK3, PADGPIO2_BANK, REG_SR_MCLK3_GPIO_MODE, REG_SR_MCLK3_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_MCLK3, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_SR_MCLK3, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_SR_MCLK3, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT10, PINMUX_FOR_I2C2_MODE_4},
    {PAD_SR_MCLK3, PADTOP_BANK, REG_SR03_MCLK_MODE, REG_SR03_MCLK_MODE_MASK, BIT6, PINMUX_FOR_SR03_MCLK_MODE_1},
    {PAD_SR_MCLK3, PADTOP_BANK, REG_SR1_BT656_MODE, REG_SR1_BT656_MODE_MASK, BIT2, PINMUX_FOR_SR1_BT656_MODE_1},
    {PAD_SR_MCLK3, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_MCLK3, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_MCLK3, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo sr_rst4_tbl[] = {
    {PAD_SR_RST4, PADGPIO2_BANK, REG_SR_RST4_GPIO_MODE, REG_SR_RST4_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_RST4, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT0, PINMUX_FOR_I2C1_MODE_13},
    {PAD_SR_RST4, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_SR_RST4, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_SR_RST4, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT9, PINMUX_FOR_I2C2_MODE_10},
    {PAD_SR_RST4, PADTOP_BANK, REG_I2C3_MODE, REG_I2C3_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2C3_MODE_3},
    {PAD_SR_RST4, PADTOP_BANK, REG_PWM4_MODE, REG_PWM4_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_PWM4_MODE_5},
    {PAD_SR_RST4, PADTOP_BANK, REG_SR01_RST_MODE, REG_SR01_RST_MODE_MASK, BIT3, PINMUX_FOR_SR01_RST_MODE_2},
    {PAD_SR_RST4, PADTOP_BANK, REG_SR10_RST_MODE, REG_SR10_RST_MODE_MASK, BIT8, PINMUX_FOR_SR10_RST_MODE_1},
    {PAD_SR_RST4, PADTOP_BANK, REG_SR0_SYNC_MODE, REG_SR0_SYNC_MODE_MASK, BIT8, PINMUX_FOR_SR0_SYNC_MODE_1},
    {PAD_SR_RST4, PADTOP_BANK, REG_SR1_BT656_MODE, REG_SR1_BT656_MODE_MASK, BIT2, PINMUX_FOR_SR1_BT656_MODE_1},
    {PAD_SR_RST4, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_RST4, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo sr_mclk4_tbl[] = {
    {PAD_SR_MCLK4, PADGPIO2_BANK, REG_SR_MCLK4_GPIO_MODE, REG_SR_MCLK4_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT0, PINMUX_FOR_I2C1_MODE_13},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT9, PINMUX_FOR_I2C2_MODE_10},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_I2C3_MODE, REG_I2C3_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2C3_MODE_3},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_PWM5_MODE, REG_PWM5_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_PWM5_MODE_5},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_SR01_MCLK_MODE, REG_SR01_MCLK_MODE_MASK, BIT3, PINMUX_FOR_SR01_MCLK_MODE_2},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_SR10_MCLK_MODE, REG_SR10_MCLK_MODE_MASK, BIT8, PINMUX_FOR_SR10_MCLK_MODE_1},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_SR0_SYNC_MODE, REG_SR0_SYNC_MODE_MASK, BIT8, PINMUX_FOR_SR0_SYNC_MODE_1},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_SR0_BT656_MODE, REG_SR0_BT656_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT656_MODE_1},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_MCLK4, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo sr_rst5_tbl[] = {
    {PAD_SR_RST5, PADGPIO2_BANK, REG_SR_RST5_GPIO_MODE, REG_SR_RST5_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_RST5, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT8, PINMUX_FOR_EJ_MODE_2},
    {PAD_SR_RST5, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT1, PINMUX_FOR_DLA_EJ_MODE_2},
    {PAD_SR_RST5, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT1, PINMUX_FOR_CA7_EJ_MODE_2},
    {PAD_SR_RST5, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT5, PINMUX_FOR_DSP_EJ_MODE_2},
    {PAD_SR_RST5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_6},
    {PAD_SR_RST5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT0, PINMUX_FOR_I2C1_MODE_13},
    {PAD_SR_RST5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_SR_RST5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_SR_RST5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_SR_RST5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_SR_RST5, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_I2C2_MODE_3},
    {PAD_SR_RST5, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT9, PINMUX_FOR_I2C2_MODE_10},
    {PAD_SR_RST5, PADTOP_BANK, REG_PWM6_MODE, REG_PWM6_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_PWM6_MODE_5},
    {PAD_SR_RST5, PADTOP_BANK, REG_SR11_RST_MODE, REG_SR11_RST_MODE_MASK, BIT10, PINMUX_FOR_SR11_RST_MODE_1},
    {PAD_SR_RST5, PADTOP_BANK, REG_SR1_SYNC_MODE, REG_SR1_SYNC_MODE_MASK, BIT10, PINMUX_FOR_SR1_SYNC_MODE_1},
    {PAD_SR_RST5, PADTOP_BANK, REG_SR0_BT656_MODE, REG_SR0_BT656_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT656_MODE_1},
    {PAD_SR_RST5, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_RST5, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_RST5, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT1, PINMUX_FOR_SR0_SLAVE_MODE_2},
    {PAD_SR_RST5, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_SR1_SLAVE_MODE_3},
    {PAD_SR_RST5, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo sr_mclk5_tbl[] = {
    {PAD_SR_MCLK5, PADGPIO2_BANK, REG_SR_MCLK5_GPIO_MODE, REG_SR_MCLK5_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT8, PINMUX_FOR_EJ_MODE_2},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT1, PINMUX_FOR_DLA_EJ_MODE_2},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT1, PINMUX_FOR_CA7_EJ_MODE_2},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT5, PINMUX_FOR_DSP_EJ_MODE_2},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_6},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT0, PINMUX_FOR_I2C1_MODE_13},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_I2C2_MODE_3},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT9, PINMUX_FOR_I2C2_MODE_10},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_PWM7_MODE, REG_PWM7_MODE_MASK, BIT14 | BIT12, PINMUX_FOR_PWM7_MODE_5},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT0, PINMUX_FOR_DMIC_4CH_MODE_1},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_SR11_MCLK_MODE, REG_SR11_MCLK_MODE_MASK, BIT10, PINMUX_FOR_SR11_MCLK_MODE_1},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_SR1_SYNC_MODE, REG_SR1_SYNC_MODE_MASK, BIT10, PINMUX_FOR_SR1_SYNC_MODE_1},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_SR0_BT656_MODE, REG_SR0_BT656_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT656_MODE_1},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT1, PINMUX_FOR_SR0_SLAVE_MODE_2},
    {PAD_SR_MCLK5, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_SR1_SLAVE_MODE_3},
};
const ST_PadMuxInfo sr_rst6_tbl[] = {
    {PAD_SR_RST6, PADGPIO2_BANK, REG_SR_RST6_GPIO_MODE, REG_SR_RST6_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_RST6, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT8, PINMUX_FOR_EJ_MODE_2},
    {PAD_SR_RST6, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT1, PINMUX_FOR_DLA_EJ_MODE_2},
    {PAD_SR_RST6, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT1, PINMUX_FOR_CA7_EJ_MODE_2},
    {PAD_SR_RST6, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT5, PINMUX_FOR_DSP_EJ_MODE_2},
    {PAD_SR_RST6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_3},
    {PAD_SR_RST6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_11},
    {PAD_SR_RST6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT0, PINMUX_FOR_I2C1_MODE_13},
    {PAD_SR_RST6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_SR_RST6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_SR_RST6, PADTOP_BANK, REG_PWM8_MODE, REG_PWM8_MODE_MASK, BIT2, PINMUX_FOR_PWM8_MODE_4},
    {PAD_SR_RST6, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT0, PINMUX_FOR_DMIC_MODE_1},
    {PAD_SR_RST6, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT0, PINMUX_FOR_DMIC_4CH_MODE_1},
    {PAD_SR_RST6, PADTOP_BANK, REG_SR00_RST_MODE, REG_SR00_RST_MODE_MASK, BIT1, PINMUX_FOR_SR00_RST_MODE_2},
    {PAD_SR_RST6, PADTOP_BANK, REG_SR12_RST_MODE, REG_SR12_RST_MODE_MASK, BIT12, PINMUX_FOR_SR12_RST_MODE_1},
    {PAD_SR_RST6, PADTOP_BANK, REG_SR2_SYNC_MODE, REG_SR2_SYNC_MODE_MASK, BIT12, PINMUX_FOR_SR2_SYNC_MODE_1},
    {PAD_SR_RST6, PADTOP_BANK, REG_SR0_BT656_MODE, REG_SR0_BT656_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT656_MODE_1},
    {PAD_SR_RST6, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_RST6, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_RST6, PADTOP_BANK, REG_ISP0_IR_OUT_MODE, REG_ISP0_IR_OUT_MODE_MASK, BIT9 | BIT8,
     PINMUX_FOR_ISP0_IR_OUT_MODE_3},
    {PAD_SR_RST6, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT1, PINMUX_FOR_SR0_SLAVE_MODE_2},
    {PAD_SR_RST6, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_SR1_SLAVE_MODE_3},
};
const ST_PadMuxInfo sr_mclk6_tbl[] = {
    {PAD_SR_MCLK6, PADGPIO2_BANK, REG_SR_MCLK6_GPIO_MODE, REG_SR_MCLK6_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT8, PINMUX_FOR_EJ_MODE_2},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT1, PINMUX_FOR_DLA_EJ_MODE_2},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT1, PINMUX_FOR_CA7_EJ_MODE_2},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT5, PINMUX_FOR_DSP_EJ_MODE_2},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_3},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_11},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT0, PINMUX_FOR_I2C1_MODE_13},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_PWM9_MODE, REG_PWM9_MODE_MASK, BIT6 | BIT5 | BIT4, PINMUX_FOR_PWM9_MODE_7},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT0, PINMUX_FOR_DMIC_MODE_1},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT0, PINMUX_FOR_DMIC_4CH_MODE_1},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_SR00_MCLK_MODE, REG_SR00_MCLK_MODE_MASK, BIT1, PINMUX_FOR_SR00_MCLK_MODE_2},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_SR12_MCLK_MODE, REG_SR12_MCLK_MODE_MASK, BIT12, PINMUX_FOR_SR12_MCLK_MODE_1},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_SR2_SYNC_MODE, REG_SR2_SYNC_MODE_MASK, BIT12, PINMUX_FOR_SR2_SYNC_MODE_1},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_SR0_BT656_MODE, REG_SR0_BT656_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT656_MODE_1},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
    {PAD_SR_MCLK6, PADTOP_BANK, REG_ISP1_IR_OUT_MODE, REG_ISP1_IR_OUT_MODE_MASK, BIT13 | BIT12,
     PINMUX_FOR_ISP1_IR_OUT_MODE_3},
};
const ST_PadMuxInfo sr_rst7_tbl[] = {
    {PAD_SR_RST7, PADGPIO2_BANK, REG_SR_RST7_GPIO_MODE, REG_SR_RST7_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_RST7, PADTOP_BANK, REG_I2C0_MODE, REG_I2C0_MODE_MASK, BIT1, PINMUX_FOR_I2C0_MODE_2},
    {PAD_SR_RST7, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_SR_RST7, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_SR_RST7, PADTOP_BANK, REG_PWM10_MODE, REG_PWM10_MODE_MASK, BIT1, PINMUX_FOR_PWM10_MODE_2},
    {PAD_SR_RST7, PADTOP_BANK, REG_SR13_RST_MODE, REG_SR13_RST_MODE_MASK, BIT15 | BIT14, PINMUX_FOR_SR13_RST_MODE_3},
    {PAD_SR_RST7, PADTOP_BANK, REG_SR3_SYNC_MODE, REG_SR3_SYNC_MODE_MASK, BIT14, PINMUX_FOR_SR3_SYNC_MODE_1},
    {PAD_SR_RST7, PADTOP_BANK, REG_SR0_BT656_MODE, REG_SR0_BT656_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT656_MODE_1},
    {PAD_SR_RST7, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_RST7, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
};
const ST_PadMuxInfo sr_mclk7_tbl[] = {
    {PAD_SR_MCLK7, PADGPIO2_BANK, REG_SR_MCLK7_GPIO_MODE, REG_SR_MCLK7_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_MCLK7, PADTOP_BANK, REG_I2C0_MODE, REG_I2C0_MODE_MASK, BIT1, PINMUX_FOR_I2C0_MODE_2},
    {PAD_SR_MCLK7, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_SR_MCLK7, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_SR_MCLK7, PADTOP_BANK, REG_PWM11_MODE, REG_PWM11_MODE_MASK, BIT9, PINMUX_FOR_PWM11_MODE_2},
    {PAD_SR_MCLK7, PADTOP_BANK, REG_SR13_MCLK_MODE, REG_SR13_MCLK_MODE_MASK, BIT15 | BIT14,
     PINMUX_FOR_SR13_MCLK_MODE_3},
    {PAD_SR_MCLK7, PADTOP_BANK, REG_SR3_SYNC_MODE, REG_SR3_SYNC_MODE_MASK, BIT14, PINMUX_FOR_SR3_SYNC_MODE_1},
    {PAD_SR_MCLK7, PADTOP_BANK, REG_SR0_BT656_MODE, REG_SR0_BT656_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT656_MODE_1},
    {PAD_SR_MCLK7, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_MCLK7, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
};
const ST_PadMuxInfo sr_gpio10_tbl[] = {
    {PAD_SR_GPIO10, PADGPIO2_BANK, REG_SR_GPIO10_GPIO_MODE, REG_SR_GPIO10_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_GPIO10, PADTOP_BANK, REG_I2C0_MODE, REG_I2C0_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_I2C0_MODE_5},
    {PAD_SR_GPIO10, PADTOP_BANK, REG_I2C8_MODE, REG_I2C8_MODE_MASK, BIT10 | BIT9, PINMUX_FOR_I2C8_MODE_6},
    {PAD_SR_GPIO10, PADTOP_BANK, REG_UART1_MODE, REG_UART1_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_UART1_MODE_5},
    {PAD_SR_GPIO10, PADTOP_BANK, REG_UART3_MODE, REG_UART3_MODE_MASK, BIT14 | BIT13, PINMUX_FOR_UART3_MODE_6},
    {PAD_SR_GPIO10, PADTOP_BANK, REG_SR0_BT656_MODE, REG_SR0_BT656_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT656_MODE_1},
    {PAD_SR_GPIO10, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
    {PAD_SR_GPIO10, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT1, PINMUX_FOR_SR0_BT1120_MODE_2},
};
const ST_PadMuxInfo sr_gpio11_tbl[] = {
    {PAD_SR_GPIO11, PADGPIO2_BANK, REG_SR_GPIO11_GPIO_MODE, REG_SR_GPIO11_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SR_GPIO11, PADTOP_BANK, REG_I2C0_MODE, REG_I2C0_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_I2C0_MODE_5},
    {PAD_SR_GPIO11, PADTOP_BANK, REG_I2C8_MODE, REG_I2C8_MODE_MASK, BIT10 | BIT9, PINMUX_FOR_I2C8_MODE_6},
    {PAD_SR_GPIO11, PADTOP_BANK, REG_UART1_MODE, REG_UART1_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_UART1_MODE_5},
    {PAD_SR_GPIO11, PADTOP_BANK, REG_UART3_MODE, REG_UART3_MODE_MASK, BIT14 | BIT13, PINMUX_FOR_UART3_MODE_6},
    {PAD_SR_GPIO11, PADTOP_BANK, REG_SR0_BT656_MODE, REG_SR0_BT656_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT656_MODE_1},
    {PAD_SR_GPIO11, PADTOP_BANK, REG_SR0_BT1120_MODE, REG_SR0_BT1120_MODE_MASK, BIT0, PINMUX_FOR_SR0_BT1120_MODE_1},
};
const ST_PadMuxInfo i2s0_rx_bck_tbl[] = {
    {PAD_I2S0_RX_BCK, PADGPIO2_BANK, REG_I2S0_RX_BCK_GPIO_MODE, REG_I2S0_RX_BCK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S0_RX_BCK, PADTOP_BANK, REG_I2S0_RX_MODE, REG_I2S0_RX_MODE_MASK, BIT8, PINMUX_FOR_I2S0_RX_MODE_1},
    {PAD_I2S0_RX_BCK, PADTOP_BANK, REG_I2S0_RXTX_MODE, REG_I2S0_RXTX_MODE_MASK, BIT0, PINMUX_FOR_I2S0_RXTX_MODE_1},
    {PAD_I2S0_RX_BCK, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT2 | BIT1 | BIT0,
     PINMUX_FOR_DMIC_4CH_MODE_7},
    {PAD_I2S0_RX_BCK, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT6 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_5},
    {PAD_I2S0_RX_BCK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_I2S0_RX_BCK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S0_RX_BCK, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_I2S0_RX_BCK, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_I2S0_RX_BCK, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s0_rx_wck_tbl[] = {
    {PAD_I2S0_RX_WCK, PADGPIO2_BANK, REG_I2S0_RX_WCK_GPIO_MODE, REG_I2S0_RX_WCK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S0_RX_WCK, PADTOP_BANK, REG_I2S0_MCK_MODE, REG_I2S0_MCK_MODE_MASK, BIT3 | BIT2, PINMUX_FOR_I2S0_MCK_MODE_12},
    {PAD_I2S0_RX_WCK, PADTOP_BANK, REG_I2S0_RX_MODE, REG_I2S0_RX_MODE_MASK, BIT8, PINMUX_FOR_I2S0_RX_MODE_1},
    {PAD_I2S0_RX_WCK, PADTOP_BANK, REG_I2S0_RXTX_MODE, REG_I2S0_RXTX_MODE_MASK, BIT0, PINMUX_FOR_I2S0_RXTX_MODE_1},
    {PAD_I2S0_RX_WCK, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT6 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_5},
    {PAD_I2S0_RX_WCK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_I2S0_RX_WCK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S0_RX_WCK, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_I2S0_RX_WCK, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_I2S0_RX_WCK, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s0_rx_di_tbl[] = {
    {PAD_I2S0_RX_DI, PADGPIO2_BANK, REG_I2S0_RX_DI_GPIO_MODE, REG_I2S0_RX_DI_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S0_RX_DI, PADTOP_BANK, REG_I2S0_RX_MODE, REG_I2S0_RX_MODE_MASK, BIT8, PINMUX_FOR_I2S0_RX_MODE_1},
    {PAD_I2S0_RX_DI, PADTOP_BANK, REG_I2S0_RXTX_MODE, REG_I2S0_RXTX_MODE_MASK, BIT0, PINMUX_FOR_I2S0_RXTX_MODE_1},
    {PAD_I2S0_RX_DI, PADTOP_BANK, REG_I2S0_RXTX_MODE, REG_I2S0_RXTX_MODE_MASK, BIT1, PINMUX_FOR_I2S0_RXTX_MODE_2},
    {PAD_I2S0_RX_DI, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT6 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_5},
    {PAD_I2S0_RX_DI, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_I2S0_RX_DI, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S0_RX_DI, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_I2S0_RX_DI, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_I2S0_RX_DI, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s1_rx_bck_tbl[] = {
    {PAD_I2S1_RX_BCK, PADGPIO2_BANK, REG_I2S1_RX_BCK_GPIO_MODE, REG_I2S1_RX_BCK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S1_RX_BCK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S1_RX_BCK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2C0_MODE, REG_I2C0_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_I2C0_MODE_6},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3, PINMUX_FOR_I2C1_MODE_8},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0,
     PINMUX_FOR_I2C1_MODE_15},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT8, PINMUX_FOR_I2C2_MODE_13},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9, PINMUX_FOR_I2C2_MODE_14},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2C4_MODE, REG_I2C4_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_I2C4_MODE_3},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2C5_MODE, REG_I2C5_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_I2C5_MODE_3},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2C6_MODE, REG_I2C6_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2C6_MODE_3},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2S1_RX_MODE, REG_I2S1_RX_MODE_MASK, BIT0, PINMUX_FOR_I2S1_RX_MODE_1},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2S0_RXTX_MODE, REG_I2S0_RXTX_MODE_MASK, BIT1, PINMUX_FOR_I2S0_RXTX_MODE_2},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT4, PINMUX_FOR_I2S1_RXTX_MODE_1},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT2 | BIT0,
     PINMUX_FOR_SR0_SLAVE_MODE_5},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_I2S1_RX_BCK, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s1_rx_wck_tbl[] = {
    {PAD_I2S1_RX_WCK, PADGPIO2_BANK, REG_I2S1_RX_WCK_GPIO_MODE, REG_I2S1_RX_WCK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S1_RX_WCK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S1_RX_WCK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2C0_MODE, REG_I2C0_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_I2C0_MODE_6},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3, PINMUX_FOR_I2C1_MODE_8},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0,
     PINMUX_FOR_I2C1_MODE_15},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT8, PINMUX_FOR_I2C2_MODE_13},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9, PINMUX_FOR_I2C2_MODE_14},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2C4_MODE, REG_I2C4_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_I2C4_MODE_3},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2C5_MODE, REG_I2C5_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_I2C5_MODE_3},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2C6_MODE, REG_I2C6_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2C6_MODE_3},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2S1_RX_MODE, REG_I2S1_RX_MODE_MASK, BIT0, PINMUX_FOR_I2S1_RX_MODE_1},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2S0_RXTX_MODE, REG_I2S0_RXTX_MODE_MASK, BIT1, PINMUX_FOR_I2S0_RXTX_MODE_2},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT4, PINMUX_FOR_I2S1_RXTX_MODE_1},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT2 | BIT0,
     PINMUX_FOR_SR0_SLAVE_MODE_5},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S1_RX_WCK, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
};
const ST_PadMuxInfo i2s1_rx_di_tbl[] = {
    {PAD_I2S1_RX_DI, PADGPIO2_BANK, REG_I2S1_RX_DI_GPIO_MODE, REG_I2S1_RX_DI_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S1_RX_DI, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S1_RX_DI, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT0, PINMUX_FOR_I2C1_MODE_9},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0,
     PINMUX_FOR_I2C1_MODE_15},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT10 | BIT9 | BIT8, PINMUX_FOR_I2C2_MODE_7},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT8, PINMUX_FOR_I2C2_MODE_13},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9, PINMUX_FOR_I2C2_MODE_14},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_I2C5_MODE, REG_I2C5_MODE_MASK, BIT10, PINMUX_FOR_I2C5_MODE_4},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_I2C6_MODE, REG_I2C6_MODE_MASK, BIT2, PINMUX_FOR_I2C6_MODE_4},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_I2S1_RX_MODE, REG_I2S1_RX_MODE_MASK, BIT0, PINMUX_FOR_I2S1_RX_MODE_1},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_I2S0_RXTX_MODE, REG_I2S0_RXTX_MODE_MASK, BIT1, PINMUX_FOR_I2S0_RXTX_MODE_2},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT4, PINMUX_FOR_I2S1_RXTX_MODE_1},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT2 | BIT0,
     PINMUX_FOR_SR0_SLAVE_MODE_5},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S1_RX_DI, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
};
const ST_PadMuxInfo i2s2_rx_bck_tbl[] = {
    {PAD_I2S2_RX_BCK, PADGPIO2_BANK, REG_I2S2_RX_BCK_GPIO_MODE, REG_I2S2_RX_BCK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S2_RX_BCK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S2_RX_BCK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT0, PINMUX_FOR_I2C1_MODE_9},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0,
     PINMUX_FOR_I2C1_MODE_15},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_19},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT10 | BIT9 | BIT8, PINMUX_FOR_I2C2_MODE_7},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT8, PINMUX_FOR_I2C2_MODE_13},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9, PINMUX_FOR_I2C2_MODE_14},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9 | BIT8,
     PINMUX_FOR_I2C2_MODE_15},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_I2C5_MODE, REG_I2C5_MODE_MASK, BIT10, PINMUX_FOR_I2C5_MODE_4},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_I2C6_MODE, REG_I2C6_MODE_MASK, BIT2, PINMUX_FOR_I2C6_MODE_4},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_UART2_MODE, REG_UART2_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_UART2_MODE_5},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT13, PINMUX_FOR_SD1_MODE_2},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT1, PINMUX_FOR_SD2_MODE_2},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_PWM0_MODE, REG_PWM0_MODE_MASK, BIT1, PINMUX_FOR_PWM0_MODE_2},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_I2S2_RX_MODE, REG_I2S2_RX_MODE_MASK, BIT8, PINMUX_FOR_I2S2_RX_MODE_1},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_I2S0_TX_MODE, REG_I2S0_TX_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_I2S0_TX_MODE_3},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT2 | BIT1 | BIT0,
     PINMUX_FOR_SR0_SLAVE_MODE_7},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7, PINMUX_FOR_SR1_SLAVE_MODE_8},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_I2S2_RX_BCK, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s2_rx_wck_tbl[] = {
    {PAD_I2S2_RX_WCK, PADGPIO2_BANK, REG_I2S2_RX_WCK_GPIO_MODE, REG_I2S2_RX_WCK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S2_RX_WCK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S2_RX_WCK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0,
     PINMUX_FOR_I2C1_MODE_15},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11, PINMUX_FOR_I2C2_MODE_8},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT8, PINMUX_FOR_I2C2_MODE_13},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9 | BIT8,
     PINMUX_FOR_I2C2_MODE_15},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_SPI0_CZ2_MODE, REG_SPI0_CZ2_MODE_MASK, BIT1, PINMUX_FOR_SPI0_CZ2_MODE_2},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_UART2_MODE, REG_UART2_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_UART2_MODE_5},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT13, PINMUX_FOR_SD1_MODE_2},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT1, PINMUX_FOR_SD2_MODE_2},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_PWM1_MODE, REG_PWM1_MODE_MASK, BIT5, PINMUX_FOR_PWM1_MODE_2},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_I2S2_RX_MODE, REG_I2S2_RX_MODE_MASK, BIT8, PINMUX_FOR_I2S2_RX_MODE_1},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_I2S0_TX_MODE, REG_I2S0_TX_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_I2S0_TX_MODE_3},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT2 | BIT1 | BIT0,
     PINMUX_FOR_SR0_SLAVE_MODE_7},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7, PINMUX_FOR_SR1_SLAVE_MODE_8},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_I2S2_RX_WCK, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s2_rx_di_tbl[] = {
    {PAD_I2S2_RX_DI, PADGPIO2_BANK, REG_I2S2_RX_DI_GPIO_MODE, REG_I2S2_RX_DI_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S2_RX_DI, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S2_RX_DI, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0,
     PINMUX_FOR_I2C1_MODE_15},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11, PINMUX_FOR_I2C2_MODE_8},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT8, PINMUX_FOR_I2C2_MODE_13},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9 | BIT8,
     PINMUX_FOR_I2C2_MODE_15},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_I2C7_MODE, REG_I2C7_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_I2C7_MODE_3},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SPI0_MODE_3},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT13, PINMUX_FOR_SD1_MODE_2},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT1, PINMUX_FOR_SD2_MODE_2},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_PWM2_MODE, REG_PWM2_MODE_MASK, BIT9, PINMUX_FOR_PWM2_MODE_2},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_I2S2_RX_MODE, REG_I2S2_RX_MODE_MASK, BIT8, PINMUX_FOR_I2S2_RX_MODE_1},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_I2S0_TX_MODE, REG_I2S0_TX_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_I2S0_TX_MODE_3},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_I2S1_RXTX_MODE_3},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT2 | BIT1 | BIT0,
     PINMUX_FOR_SR0_SLAVE_MODE_7},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7, PINMUX_FOR_SR1_SLAVE_MODE_8},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S2_RX_DI, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s3_rx_bck_tbl[] = {
    {PAD_I2S3_RX_BCK, PADGPIO2_BANK, REG_I2S3_RX_BCK_GPIO_MODE, REG_I2S3_RX_BCK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S3_RX_BCK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S3_RX_BCK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0,
     PINMUX_FOR_I2C1_MODE_15},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9 | BIT8,
     PINMUX_FOR_I2C2_MODE_15},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_I2C7_MODE, REG_I2C7_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_I2C7_MODE_3},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SPI0_MODE_3},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT13, PINMUX_FOR_SD1_MODE_2},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT1, PINMUX_FOR_SD2_MODE_2},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_PWM3_MODE, REG_PWM3_MODE_MASK, BIT13, PINMUX_FOR_PWM3_MODE_2},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_I2S0_RX_MODE, REG_I2S0_RX_MODE_MASK, BIT9, PINMUX_FOR_I2S0_RX_MODE_2},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_I2S3_RX_MODE, REG_I2S3_RX_MODE_MASK, BIT12, PINMUX_FOR_I2S3_RX_MODE_1},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_I2S1_RXTX_MODE_3},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_I2S3_RX_BCK, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s3_rx_wck_tbl[] = {
    {PAD_I2S3_RX_WCK, PADGPIO2_BANK, REG_I2S3_RX_WCK_GPIO_MODE, REG_I2S3_RX_WCK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S3_RX_WCK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S3_RX_WCK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0,
     PINMUX_FOR_I2C1_MODE_15},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT0, PINMUX_FOR_I2C1_MODE_17},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9 | BIT8,
     PINMUX_FOR_I2C2_MODE_15},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_I2C7_MODE, REG_I2C7_MODE_MASK, BIT6, PINMUX_FOR_I2C7_MODE_4},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SPI0_MODE_3},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT13, PINMUX_FOR_SD1_MODE_2},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT1, PINMUX_FOR_SD2_MODE_2},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_PWM4_MODE, REG_PWM4_MODE_MASK, BIT1, PINMUX_FOR_PWM4_MODE_2},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_I2S0_RX_MODE, REG_I2S0_RX_MODE_MASK, BIT9, PINMUX_FOR_I2S0_RX_MODE_2},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_I2S3_RX_MODE, REG_I2S3_RX_MODE_MASK, BIT12, PINMUX_FOR_I2S3_RX_MODE_1},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_I2S1_RXTX_MODE_3},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_I2S3_RX_WCK, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s3_rx_di_tbl[] = {
    {PAD_I2S3_RX_DI, PADGPIO2_BANK, REG_I2S3_RX_DI_GPIO_MODE, REG_I2S3_RX_DI_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S3_RX_DI, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S3_RX_DI, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S3_RX_DI, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9 | BIT8,
     PINMUX_FOR_I2C2_MODE_15},
    {PAD_I2S3_RX_DI, PADTOP_BANK, REG_I2C7_MODE, REG_I2C7_MODE_MASK, BIT6, PINMUX_FOR_I2C7_MODE_4},
    {PAD_I2S3_RX_DI, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SPI0_MODE_3},
    {PAD_I2S3_RX_DI, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT13, PINMUX_FOR_SD1_MODE_2},
    {PAD_I2S3_RX_DI, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT1, PINMUX_FOR_SD2_MODE_2},
    {PAD_I2S3_RX_DI, PADTOP_BANK, REG_PWM5_MODE, REG_PWM5_MODE_MASK, BIT5, PINMUX_FOR_PWM5_MODE_2},
    {PAD_I2S3_RX_DI, PADTOP_BANK, REG_I2S0_RX_MODE, REG_I2S0_RX_MODE_MASK, BIT9, PINMUX_FOR_I2S0_RX_MODE_2},
    {PAD_I2S3_RX_DI, PADTOP_BANK, REG_I2S3_RX_MODE, REG_I2S3_RX_MODE_MASK, BIT12, PINMUX_FOR_I2S3_RX_MODE_1},
    {PAD_I2S3_RX_DI, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_I2S1_RXTX_MODE_3},
    {PAD_I2S3_RX_DI, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S3_RX_DI, PADTOP_BANK, REG_LCD_MCU18_MODE, REG_LCD_MCU18_MODE_MASK, BIT4, PINMUX_FOR_LCD_MCU18_MODE_1},
    {PAD_I2S3_RX_DI, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s0_tx_bck_tbl[] = {
    {PAD_I2S0_TX_BCK, PADGPIO2_BANK, REG_I2S0_TX_BCK_GPIO_MODE, REG_I2S0_TX_BCK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S0_TX_BCK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S0_TX_BCK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S0_TX_BCK, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_SD1_MODE_3},
    {PAD_I2S0_TX_BCK, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SD2_MODE_3},
    {PAD_I2S0_TX_BCK, PADTOP_BANK, REG_PWM4_MODE, REG_PWM4_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_PWM4_MODE_3},
    {PAD_I2S0_TX_BCK, PADTOP_BANK, REG_I2S0_TX_MODE, REG_I2S0_TX_MODE_MASK, BIT12, PINMUX_FOR_I2S0_TX_MODE_1},
    {PAD_I2S0_TX_BCK, PADTOP_BANK, REG_I2S1_TX_MODE, REG_I2S1_TX_MODE_MASK, BIT5, PINMUX_FOR_I2S1_TX_MODE_2},
    {PAD_I2S0_TX_BCK, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s0_tx_wck_tbl[] = {
    {PAD_I2S0_TX_WCK, PADGPIO2_BANK, REG_I2S0_TX_WCK_GPIO_MODE, REG_I2S0_TX_WCK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S0_TX_WCK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S0_TX_WCK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S0_TX_WCK, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_SD1_MODE_3},
    {PAD_I2S0_TX_WCK, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SD2_MODE_3},
    {PAD_I2S0_TX_WCK, PADTOP_BANK, REG_PWM5_MODE, REG_PWM5_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_PWM5_MODE_3},
    {PAD_I2S0_TX_WCK, PADTOP_BANK, REG_I2S0_TX_MODE, REG_I2S0_TX_MODE_MASK, BIT12, PINMUX_FOR_I2S0_TX_MODE_1},
    {PAD_I2S0_TX_WCK, PADTOP_BANK, REG_I2S1_TX_MODE, REG_I2S1_TX_MODE_MASK, BIT5, PINMUX_FOR_I2S1_TX_MODE_2},
    {PAD_I2S0_TX_WCK, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s0_tx_do_tbl[] = {
    {PAD_I2S0_TX_DO, PADGPIO2_BANK, REG_I2S0_TX_DO_GPIO_MODE, REG_I2S0_TX_DO_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S0_TX_DO, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S0_TX_DO, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S0_TX_DO, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT2, PINMUX_FOR_SPI0_MODE_4},
    {PAD_I2S0_TX_DO, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_SD1_MODE_3},
    {PAD_I2S0_TX_DO, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SD2_MODE_3},
    {PAD_I2S0_TX_DO, PADTOP_BANK, REG_PWM6_MODE, REG_PWM6_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_PWM6_MODE_3},
    {PAD_I2S0_TX_DO, PADTOP_BANK, REG_I2S0_TX_MODE, REG_I2S0_TX_MODE_MASK, BIT12, PINMUX_FOR_I2S0_TX_MODE_1},
    {PAD_I2S0_TX_DO, PADTOP_BANK, REG_I2S1_TX_MODE, REG_I2S1_TX_MODE_MASK, BIT5, PINMUX_FOR_I2S1_TX_MODE_2},
    {PAD_I2S0_TX_DO, PADTOP_BANK, REG_I2S0_RXTX_MODE, REG_I2S0_RXTX_MODE_MASK, BIT0, PINMUX_FOR_I2S0_RXTX_MODE_1},
    {PAD_I2S0_TX_DO, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT6, PINMUX_FOR_I2S1_RXTX_MODE_4},
    {PAD_I2S0_TX_DO, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s1_tx_bck_tbl[] = {
    {PAD_I2S1_TX_BCK, PADGPIO2_BANK, REG_I2S1_TX_BCK_GPIO_MODE, REG_I2S1_TX_BCK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S1_TX_BCK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_I2S1_TX_BCK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9 | BIT8,
     PINMUX_FOR_I2C2_MODE_15},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_I2C7_MODE, REG_I2C7_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_I2C7_MODE_5},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT2, PINMUX_FOR_SPI0_MODE_4},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_SD1_MODE_3},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SD2_MODE_3},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_PWM6_MODE, REG_PWM6_MODE_MASK, BIT9, PINMUX_FOR_PWM6_MODE_2},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_PWM7_MODE, REG_PWM7_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_PWM7_MODE_3},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_I2S1_RX_MODE, REG_I2S1_RX_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2S1_RX_MODE_3},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_I2S1_TX_MODE, REG_I2S1_TX_MODE_MASK, BIT4, PINMUX_FOR_I2S1_TX_MODE_1},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT6, PINMUX_FOR_I2S1_RXTX_MODE_4},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT3, PINMUX_FOR_SR0_SLAVE_MODE_8},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_9},
    {PAD_I2S1_TX_BCK, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s1_tx_wck_tbl[] = {
    {PAD_I2S1_TX_WCK, PADGPIO2_BANK, REG_I2S1_TX_WCK_GPIO_MODE, REG_I2S1_TX_WCK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9 | BIT8,
     PINMUX_FOR_I2C2_MODE_15},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_I2C7_MODE, REG_I2C7_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_I2C7_MODE_5},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT2, PINMUX_FOR_SPI0_MODE_4},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_SD1_MODE_3},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SD2_MODE_3},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_PWM7_MODE, REG_PWM7_MODE_MASK, BIT13, PINMUX_FOR_PWM7_MODE_2},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_PWM8_MODE, REG_PWM8_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_PWM8_MODE_6},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_I2S1_RX_MODE, REG_I2S1_RX_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2S1_RX_MODE_3},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_I2S1_TX_MODE, REG_I2S1_TX_MODE_MASK, BIT4, PINMUX_FOR_I2S1_TX_MODE_1},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT6, PINMUX_FOR_I2S1_RXTX_MODE_4},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT3, PINMUX_FOR_SR0_SLAVE_MODE_8},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_9},
    {PAD_I2S1_TX_WCK, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s1_tx_do_tbl[] = {
    {PAD_I2S1_TX_DO, PADGPIO2_BANK, REG_I2S1_TX_DO_GPIO_MODE, REG_I2S1_TX_DO_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_I2S1_TX_DO, PADTOP_BANK, REG_SPI0_MODE, REG_SPI0_MODE_MASK, BIT2, PINMUX_FOR_SPI0_MODE_4},
    {PAD_I2S1_TX_DO, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_SD1_MODE_3},
    {PAD_I2S1_TX_DO, PADTOP_BANK, REG_SD2_MODE, REG_SD2_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SD2_MODE_3},
    {PAD_I2S1_TX_DO, PADTOP_BANK, REG_PWM8_MODE, REG_PWM8_MODE_MASK, BIT1, PINMUX_FOR_PWM8_MODE_2},
    {PAD_I2S1_TX_DO, PADTOP_BANK, REG_PWM9_MODE, REG_PWM9_MODE_MASK, BIT6, PINMUX_FOR_PWM9_MODE_4},
    {PAD_I2S1_TX_DO, PADTOP_BANK, REG_I2S1_RX_MODE, REG_I2S1_RX_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2S1_RX_MODE_3},
    {PAD_I2S1_TX_DO, PADTOP_BANK, REG_I2S1_TX_MODE, REG_I2S1_TX_MODE_MASK, BIT4, PINMUX_FOR_I2S1_TX_MODE_1},
    {PAD_I2S1_TX_DO, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT4, PINMUX_FOR_I2S1_RXTX_MODE_1},
    {PAD_I2S1_TX_DO, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT6, PINMUX_FOR_I2S1_RXTX_MODE_4},
    {PAD_I2S1_TX_DO, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT3, PINMUX_FOR_SR0_SLAVE_MODE_8},
    {PAD_I2S1_TX_DO, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT4,
     PINMUX_FOR_SR1_SLAVE_MODE_9},
    {PAD_I2S1_TX_DO, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s_mclk0_tbl[] = {
    {PAD_I2S_MCLK0, PADGPIO2_BANK, REG_I2S_MCLK0_GPIO_MODE, REG_I2S_MCLK0_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_I2S_MCLK0, PADTOP_BANK, REG_I2C9_MODE, REG_I2C9_MODE_MASK, BIT12, PINMUX_FOR_I2C9_MODE_1},
    {PAD_I2S_MCLK0, PADTOP_BANK, REG_SD1_CDZ_MODE, REG_SD1_CDZ_MODE_MASK, BIT13, PINMUX_FOR_SD1_CDZ_MODE_2},
    {PAD_I2S_MCLK0, PADTOP_BANK, REG_SD2_CDZ_MODE, REG_SD2_CDZ_MODE_MASK, BIT5, PINMUX_FOR_SD2_CDZ_MODE_2},
    {PAD_I2S_MCLK0, PADTOP_BANK, REG_PWM6_MODE, REG_PWM6_MODE_MASK, BIT10 | BIT9, PINMUX_FOR_PWM6_MODE_6},
    {PAD_I2S_MCLK0, PADTOP_BANK, REG_PWM9_MODE, REG_PWM9_MODE_MASK, BIT5, PINMUX_FOR_PWM9_MODE_2},
    {PAD_I2S_MCLK0, PADTOP_BANK, REG_I2S0_MCK_MODE, REG_I2S0_MCK_MODE_MASK, BIT0, PINMUX_FOR_I2S0_MCK_MODE_1},
    {PAD_I2S_MCLK0, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_I2S_MCLK0, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo i2s_mclk1_tbl[] = {
    {PAD_I2S_MCLK1, PADGPIO2_BANK, REG_I2S_MCLK1_GPIO_MODE, REG_I2S_MCLK1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_I2S_MCLK1, PADTOP_BANK, REG_I2C9_MODE, REG_I2C9_MODE_MASK, BIT12, PINMUX_FOR_I2C9_MODE_1},
    {PAD_I2S_MCLK1, PADTOP_BANK, REG_SPI0_CZ2_MODE, REG_SPI0_CZ2_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SPI0_CZ2_MODE_3},
    {PAD_I2S_MCLK1, PADTOP_BANK, REG_SD1_CDZ_MODE, REG_SD1_CDZ_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_SD1_CDZ_MODE_3},
    {PAD_I2S_MCLK1, PADTOP_BANK, REG_SD2_CDZ_MODE, REG_SD2_CDZ_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_SD2_CDZ_MODE_3},
    {PAD_I2S_MCLK1, PADTOP_BANK, REG_PWM10_MODE, REG_PWM10_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_PWM10_MODE_5},
    {PAD_I2S_MCLK1, PADTOP_BANK, REG_I2S1_MCK_MODE, REG_I2S1_MCK_MODE_MASK, BIT4, PINMUX_FOR_I2S1_MCK_MODE_1},
    {PAD_I2S_MCLK1, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo hdmitx_hpd_tbl[] = {
    {PAD_HDMITX_HPD, PADGPIO2_BANK, REG_HDMITX_HPD_GPIO_MODE, REG_HDMITX_HPD_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_HDMITX_HPD, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_HDMITX_HPD, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_HDMITX_HPD, PADTOP_BANK, REG_PWM11_MODE, REG_PWM11_MODE_MASK, BIT10, PINMUX_FOR_PWM11_MODE_4},
    {PAD_HDMITX_HPD, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo hdmitx_sda_tbl[] = {
    {PAD_HDMITX_SDA, PADGPIO2_BANK, REG_HDMITX_SDA_GPIO_MODE, REG_HDMITX_SDA_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_HDMITX_SDA, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_HDMITX_SDA, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_HDMITX_SDA, PADTOP_BANK, REG_I2C0_MODE, REG_I2C0_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2C0_MODE_3},
    {PAD_HDMITX_SDA, PADTOP_BANK, REG_I2C4_MODE, REG_I2C4_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_I2C4_MODE_5},
    {PAD_HDMITX_SDA, PADTOP_BANK, REG_I2C10_MODE, REG_I2C10_MODE_MASK, BIT0, PINMUX_FOR_I2C10_MODE_1},
    {PAD_HDMITX_SDA, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo hdmitx_scl_tbl[] = {
    {PAD_HDMITX_SCL, PADGPIO2_BANK, REG_HDMITX_SCL_GPIO_MODE, REG_HDMITX_SCL_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_HDMITX_SCL, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_HDMITX_SCL, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_HDMITX_SCL, PADTOP_BANK, REG_I2C0_MODE, REG_I2C0_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2C0_MODE_3},
    {PAD_HDMITX_SCL, PADTOP_BANK, REG_I2C4_MODE, REG_I2C4_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_I2C4_MODE_5},
    {PAD_HDMITX_SCL, PADTOP_BANK, REG_I2C10_MODE, REG_I2C10_MODE_MASK, BIT0, PINMUX_FOR_I2C10_MODE_1},
    {PAD_HDMITX_SCL, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo vsync_out_tbl[] = {
    {PAD_VSYNC_OUT, PADGPIO2_BANK, REG_VSYNC_OUT_GPIO_MODE, REG_VSYNC_OUT_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_VSYNC_OUT, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_VSYNC_OUT, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_VSYNC_OUT, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT2 | BIT1 | BIT0, PINMUX_FOR_DMIC_MODE_7},
    {PAD_VSYNC_OUT, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT2 | BIT1 | BIT0,
     PINMUX_FOR_DMIC_4CH_MODE_7},
    {PAD_VSYNC_OUT, PADTOP_BANK, REG_VGA_VSYNC_MODE, REG_VGA_VSYNC_MODE_MASK, BIT9, PINMUX_FOR_VGA_VSYNC_MODE_1},
    {PAD_VSYNC_OUT, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo hsync_out_tbl[] = {
    {PAD_HSYNC_OUT, PADGPIO2_BANK, REG_HSYNC_OUT_GPIO_MODE, REG_HSYNC_OUT_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_HSYNC_OUT, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_HSYNC_OUT, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_HSYNC_OUT, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT2 | BIT1 | BIT0, PINMUX_FOR_DMIC_MODE_7},
    {PAD_HSYNC_OUT, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT2 | BIT1 | BIT0,
     PINMUX_FOR_DMIC_4CH_MODE_7},
    {PAD_HSYNC_OUT, PADTOP_BANK, REG_VGA_HSYNC_MODE, REG_VGA_HSYNC_MODE_MASK, BIT8, PINMUX_FOR_VGA_HSYNC_MODE_1},
    {PAD_HSYNC_OUT, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo gpio8_tbl[] = {
    {PAD_GPIO8, PADGPIO2_BANK, REG_GPIO8_GPIO_MODE, REG_GPIO8_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO8, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT9, PINMUX_FOR_EJ_MODE_4},
    {PAD_GPIO8, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT2, PINMUX_FOR_DLA_EJ_MODE_4},
    {PAD_GPIO8, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT2, PINMUX_FOR_CA7_EJ_MODE_4},
    {PAD_GPIO8, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT6, PINMUX_FOR_DSP_EJ_MODE_4},
    {PAD_GPIO8, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_GPIO8, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_GPIO8, PADTOP_BANK, REG_I2C0_MODE, REG_I2C0_MODE_MASK, BIT2, PINMUX_FOR_I2C0_MODE_4},
    {PAD_GPIO8, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT2 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_7},
    {PAD_GPIO8, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_GPIO8, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_GPIO8, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_GPIO8, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT9 | BIT8, PINMUX_FOR_I2C2_MODE_11},
    {PAD_GPIO8, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10, PINMUX_FOR_I2C2_MODE_12},
    {PAD_GPIO8, PADTOP_BANK, REG_I2C4_MODE, REG_I2C4_MODE_MASK, BIT5, PINMUX_FOR_I2C4_MODE_2},
    {PAD_GPIO8, PADTOP_BANK, REG_I2C6_MODE, REG_I2C6_MODE_MASK, BIT0, PINMUX_FOR_I2C6_MODE_1},
    {PAD_GPIO8, PADTOP_BANK, REG_UART5_MODE, REG_UART5_MODE_MASK, BIT5, PINMUX_FOR_UART5_MODE_2},
    {PAD_GPIO8, PADTOP_BANK, REG_SD0_MODE, REG_SD0_MODE_MASK, BIT9, PINMUX_FOR_SD0_MODE_2},
    {PAD_GPIO8, PADTOP_BANK, REG_PWM0_MODE, REG_PWM0_MODE_MASK, BIT2, PINMUX_FOR_PWM0_MODE_4},
    {PAD_GPIO8, PADTOP_BANK, REG_I2S0_MCK_MODE, REG_I2S0_MCK_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2S0_MCK_MODE_3},
    {PAD_GPIO8, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT1, PINMUX_FOR_DMIC_4CH_MODE_2},
    {PAD_GPIO8, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_GPIO8, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_GPIO8, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_GPIO8, PADTOP_BANK, REG_MII0_MODE, REG_MII0_MODE_MASK, BIT0, PINMUX_FOR_MII0_MODE_1},
    {PAD_GPIO8, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo gpio9_tbl[] = {
    {PAD_GPIO9, PADGPIO2_BANK, REG_GPIO9_GPIO_MODE, REG_GPIO9_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO9, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT9, PINMUX_FOR_EJ_MODE_4},
    {PAD_GPIO9, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT2, PINMUX_FOR_DLA_EJ_MODE_4},
    {PAD_GPIO9, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT2, PINMUX_FOR_CA7_EJ_MODE_4},
    {PAD_GPIO9, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT6, PINMUX_FOR_DSP_EJ_MODE_4},
    {PAD_GPIO9, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_GPIO9, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_GPIO9, PADTOP_BANK, REG_I2C0_MODE, REG_I2C0_MODE_MASK, BIT2, PINMUX_FOR_I2C0_MODE_4},
    {PAD_GPIO9, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT2 | BIT1 | BIT0, PINMUX_FOR_I2C1_MODE_7},
    {PAD_GPIO9, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_GPIO9, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_GPIO9, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_GPIO9, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT9 | BIT8, PINMUX_FOR_I2C2_MODE_11},
    {PAD_GPIO9, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10, PINMUX_FOR_I2C2_MODE_12},
    {PAD_GPIO9, PADTOP_BANK, REG_I2C4_MODE, REG_I2C4_MODE_MASK, BIT5, PINMUX_FOR_I2C4_MODE_2},
    {PAD_GPIO9, PADTOP_BANK, REG_I2C6_MODE, REG_I2C6_MODE_MASK, BIT0, PINMUX_FOR_I2C6_MODE_1},
    {PAD_GPIO9, PADTOP_BANK, REG_UART5_MODE, REG_UART5_MODE_MASK, BIT5, PINMUX_FOR_UART5_MODE_2},
    {PAD_GPIO9, PADTOP_BANK, REG_SD0_MODE, REG_SD0_MODE_MASK, BIT9, PINMUX_FOR_SD0_MODE_2},
    {PAD_GPIO9, PADTOP_BANK, REG_PWM1_MODE, REG_PWM1_MODE_MASK, BIT6, PINMUX_FOR_PWM1_MODE_4},
    {PAD_GPIO9, PADTOP_BANK, REG_I2S1_MCK_MODE, REG_I2S1_MCK_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_I2S1_MCK_MODE_3},
    {PAD_GPIO9, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT1, PINMUX_FOR_DMIC_MODE_2},
    {PAD_GPIO9, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT1, PINMUX_FOR_DMIC_4CH_MODE_2},
    {PAD_GPIO9, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT6, PINMUX_FOR_SR1_SLAVE_MODE_4},
    {PAD_GPIO9, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_GPIO9, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_GPIO9, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_GPIO9, PADTOP_BANK, REG_MII0_MODE, REG_MII0_MODE_MASK, BIT0, PINMUX_FOR_MII0_MODE_1},
    {PAD_GPIO9, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo gpio10_tbl[] = {
    {PAD_GPIO10, PADGPIO2_BANK, REG_GPIO10_GPIO_MODE, REG_GPIO10_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO10, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT9, PINMUX_FOR_EJ_MODE_4},
    {PAD_GPIO10, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT2, PINMUX_FOR_DLA_EJ_MODE_4},
    {PAD_GPIO10, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT2, PINMUX_FOR_CA7_EJ_MODE_4},
    {PAD_GPIO10, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT6, PINMUX_FOR_DSP_EJ_MODE_4},
    {PAD_GPIO10, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_GPIO10, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_GPIO10, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_I2C1_MODE_5},
    {PAD_GPIO10, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_GPIO10, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_GPIO10, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_GPIO10, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_I2C2_MODE_5},
    {PAD_GPIO10, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT9 | BIT8, PINMUX_FOR_I2C2_MODE_11},
    {PAD_GPIO10, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10, PINMUX_FOR_I2C2_MODE_12},
    {PAD_GPIO10, PADTOP_BANK, REG_I2C5_MODE, REG_I2C5_MODE_MASK, BIT9, PINMUX_FOR_I2C5_MODE_2},
    {PAD_GPIO10, PADTOP_BANK, REG_I2C6_MODE, REG_I2C6_MODE_MASK, BIT1, PINMUX_FOR_I2C6_MODE_2},
    {PAD_GPIO10, PADTOP_BANK, REG_I2C7_MODE, REG_I2C7_MODE_MASK, BIT4, PINMUX_FOR_I2C7_MODE_1},
    {PAD_GPIO10, PADTOP_BANK, REG_UART4_MODE, REG_UART4_MODE_MASK, BIT2, PINMUX_FOR_UART4_MODE_4},
    {PAD_GPIO10, PADTOP_BANK, REG_SD0_MODE, REG_SD0_MODE_MASK, BIT9, PINMUX_FOR_SD0_MODE_2},
    {PAD_GPIO10, PADTOP_BANK, REG_PWM2_MODE, REG_PWM2_MODE_MASK, BIT10, PINMUX_FOR_PWM2_MODE_4},
    {PAD_GPIO10, PADTOP_BANK, REG_I2S1_RX_MODE, REG_I2S1_RX_MODE_MASK, BIT1, PINMUX_FOR_I2S1_RX_MODE_2},
    {PAD_GPIO10, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT1, PINMUX_FOR_DMIC_MODE_2},
    {PAD_GPIO10, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT1, PINMUX_FOR_DMIC_4CH_MODE_2},
    {PAD_GPIO10, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT6, PINMUX_FOR_SR1_SLAVE_MODE_4},
    {PAD_GPIO10, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_GPIO10, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_GPIO10, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_GPIO10, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_GPIO10, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
    {PAD_GPIO10, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo gpio11_tbl[] = {
    {PAD_GPIO11, PADGPIO2_BANK, REG_GPIO11_GPIO_MODE, REG_GPIO11_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO11, PADTOP_BANK, REG_EJ_MODE, REG_EJ_MODE_MASK, BIT9, PINMUX_FOR_EJ_MODE_4},
    {PAD_GPIO11, PADTOP_BANK, REG_DLA_EJ_MODE, REG_DLA_EJ_MODE_MASK, BIT2, PINMUX_FOR_DLA_EJ_MODE_4},
    {PAD_GPIO11, PADTOP_BANK, REG_CA7_EJ_MODE, REG_CA7_EJ_MODE_MASK, BIT2, PINMUX_FOR_CA7_EJ_MODE_4},
    {PAD_GPIO11, PADTOP_BANK, REG_DSP_EJ_MODE, REG_DSP_EJ_MODE_MASK, BIT6, PINMUX_FOR_DSP_EJ_MODE_4},
    {PAD_GPIO11, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_GPIO11, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_GPIO11, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_I2C1_MODE_5},
    {PAD_GPIO11, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_GPIO11, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_GPIO11, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4 | BIT1, PINMUX_FOR_I2C1_MODE_18},
    {PAD_GPIO11, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_I2C2_MODE_5},
    {PAD_GPIO11, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT9 | BIT8, PINMUX_FOR_I2C2_MODE_11},
    {PAD_GPIO11, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10, PINMUX_FOR_I2C2_MODE_12},
    {PAD_GPIO11, PADTOP_BANK, REG_I2C5_MODE, REG_I2C5_MODE_MASK, BIT9, PINMUX_FOR_I2C5_MODE_2},
    {PAD_GPIO11, PADTOP_BANK, REG_I2C6_MODE, REG_I2C6_MODE_MASK, BIT1, PINMUX_FOR_I2C6_MODE_2},
    {PAD_GPIO11, PADTOP_BANK, REG_I2C7_MODE, REG_I2C7_MODE_MASK, BIT4, PINMUX_FOR_I2C7_MODE_1},
    {PAD_GPIO11, PADTOP_BANK, REG_UART4_MODE, REG_UART4_MODE_MASK, BIT2, PINMUX_FOR_UART4_MODE_4},
    {PAD_GPIO11, PADTOP_BANK, REG_SD0_MODE, REG_SD0_MODE_MASK, BIT9, PINMUX_FOR_SD0_MODE_2},
    {PAD_GPIO11, PADTOP_BANK, REG_PWM3_MODE, REG_PWM3_MODE_MASK, BIT14, PINMUX_FOR_PWM3_MODE_4},
    {PAD_GPIO11, PADTOP_BANK, REG_I2S1_RX_MODE, REG_I2S1_RX_MODE_MASK, BIT1, PINMUX_FOR_I2S1_RX_MODE_2},
    {PAD_GPIO11, PADTOP_BANK, REG_SR1_SLAVE_MODE, REG_SR1_SLAVE_MODE_MASK, BIT6, PINMUX_FOR_SR1_SLAVE_MODE_4},
    {PAD_GPIO11, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_GPIO11, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_GPIO11, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_GPIO11, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_GPIO11, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo gpio12_tbl[] = {
    {PAD_GPIO12, PADGPIO2_BANK, REG_GPIO12_GPIO_MODE, REG_GPIO12_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO12, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT0, PINMUX_FOR_TEST_IN_MODE_1},
    {PAD_GPIO12, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT4, PINMUX_FOR_TEST_OUT_MODE_1},
    {PAD_GPIO12, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_GPIO12, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_GPIO12, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT10 | BIT9, PINMUX_FOR_I2C2_MODE_6},
    {PAD_GPIO12, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10, PINMUX_FOR_I2C2_MODE_12},
    {PAD_GPIO12, PADTOP_BANK, REG_I2C7_MODE, REG_I2C7_MODE_MASK, BIT5, PINMUX_FOR_I2C7_MODE_2},
    {PAD_GPIO12, PADTOP_BANK, REG_SPI1_MODE, REG_SPI1_MODE_MASK, BIT5, PINMUX_FOR_SPI1_MODE_2},
    {PAD_GPIO12, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT9, PINMUX_FOR_FUART_MODE_2},
    {PAD_GPIO12, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT13, PINMUX_FOR_FUART_2W_MODE_2},
    {PAD_GPIO12, PADTOP_BANK, REG_UART5_MODE, REG_UART5_MODE_MASK, BIT4, PINMUX_FOR_UART5_MODE_1},
    {PAD_GPIO12, PADTOP_BANK, REG_SD0_MODE, REG_SD0_MODE_MASK, BIT9, PINMUX_FOR_SD0_MODE_2},
    {PAD_GPIO12, PADTOP_BANK, REG_PWM4_MODE, REG_PWM4_MODE_MASK, BIT2 | BIT1, PINMUX_FOR_PWM4_MODE_6},
    {PAD_GPIO12, PADTOP_BANK, REG_I2S1_RX_MODE, REG_I2S1_RX_MODE_MASK, BIT1, PINMUX_FOR_I2S1_RX_MODE_2},
    {PAD_GPIO12, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT2, PINMUX_FOR_SR0_SLAVE_MODE_4},
    {PAD_GPIO12, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_GPIO12, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_GPIO12, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_GPIO12, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_GPIO12, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo gpio13_tbl[] = {
    {PAD_GPIO13, PADGPIO2_BANK, REG_GPIO13_GPIO_MODE, REG_GPIO13_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO13, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_GPIO13, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_GPIO13, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT10 | BIT9, PINMUX_FOR_I2C2_MODE_6},
    {PAD_GPIO13, PADTOP_BANK, REG_I2C2_MODE, REG_I2C2_MODE_MASK, BIT11 | BIT10, PINMUX_FOR_I2C2_MODE_12},
    {PAD_GPIO13, PADTOP_BANK, REG_I2C7_MODE, REG_I2C7_MODE_MASK, BIT5, PINMUX_FOR_I2C7_MODE_2},
    {PAD_GPIO13, PADTOP_BANK, REG_SPI1_MODE, REG_SPI1_MODE_MASK, BIT5, PINMUX_FOR_SPI1_MODE_2},
    {PAD_GPIO13, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT9, PINMUX_FOR_FUART_MODE_2},
    {PAD_GPIO13, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT13, PINMUX_FOR_FUART_2W_MODE_2},
    {PAD_GPIO13, PADTOP_BANK, REG_UART5_MODE, REG_UART5_MODE_MASK, BIT4, PINMUX_FOR_UART5_MODE_1},
    {PAD_GPIO13, PADTOP_BANK, REG_SD0_MODE, REG_SD0_MODE_MASK, BIT9, PINMUX_FOR_SD0_MODE_2},
    {PAD_GPIO13, PADTOP_BANK, REG_PWM5_MODE, REG_PWM5_MODE_MASK, BIT6 | BIT5, PINMUX_FOR_PWM5_MODE_6},
    {PAD_GPIO13, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT2, PINMUX_FOR_SR0_SLAVE_MODE_4},
    {PAD_GPIO13, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_GPIO13, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_GPIO13, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_GPIO13, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_GPIO13, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo gpio14_tbl[] = {
    {PAD_GPIO14, PADGPIO2_BANK, REG_GPIO14_GPIO_MODE, REG_GPIO14_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO14, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_GPIO14, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_GPIO14, PADTOP_BANK, REG_I2C3_MODE, REG_I2C3_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_I2C3_MODE_5},
    {PAD_GPIO14, PADTOP_BANK, REG_SPI1_MODE, REG_SPI1_MODE_MASK, BIT5, PINMUX_FOR_SPI1_MODE_2},
    {PAD_GPIO14, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT9, PINMUX_FOR_FUART_MODE_2},
    {PAD_GPIO14, PADTOP_BANK, REG_UART3_MODE, REG_UART3_MODE_MASK, BIT14, PINMUX_FOR_UART3_MODE_4},
    {PAD_GPIO14, PADTOP_BANK, REG_UART4_MODE, REG_UART4_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_UART4_MODE_5},
    {PAD_GPIO14, PADTOP_BANK, REG_SD0_CDZ_MODE, REG_SD0_CDZ_MODE_MASK, BIT11, PINMUX_FOR_SD0_CDZ_MODE_2},
    {PAD_GPIO14, PADTOP_BANK, REG_PWM6_MODE, REG_PWM6_MODE_MASK, BIT10 | BIT9 | BIT8, PINMUX_FOR_PWM6_MODE_7},
    {PAD_GPIO14, PADTOP_BANK, REG_I2S0_MCK_MODE, REG_I2S0_MCK_MODE_MASK, BIT2, PINMUX_FOR_I2S0_MCK_MODE_4},
    {PAD_GPIO14, PADTOP_BANK, REG_SR0_SLAVE_MODE, REG_SR0_SLAVE_MODE_MASK, BIT2, PINMUX_FOR_SR0_SLAVE_MODE_4},
    {PAD_GPIO14, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_GPIO14, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_GPIO14, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_GPIO14, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_GPIO14, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo gpio15_tbl[] = {
    {PAD_GPIO15, PADGPIO2_BANK, REG_GPIO15_GPIO_MODE, REG_GPIO15_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO15, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1, PINMUX_FOR_I2C1_MODE_14},
    {PAD_GPIO15, PADTOP_BANK, REG_I2C1_MODE, REG_I2C1_MODE_MASK, BIT4, PINMUX_FOR_I2C1_MODE_16},
    {PAD_GPIO15, PADTOP_BANK, REG_I2C3_MODE, REG_I2C3_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_I2C3_MODE_5},
    {PAD_GPIO15, PADTOP_BANK, REG_SPI1_MODE, REG_SPI1_MODE_MASK, BIT5, PINMUX_FOR_SPI1_MODE_2},
    {PAD_GPIO15, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT9, PINMUX_FOR_FUART_MODE_2},
    {PAD_GPIO15, PADTOP_BANK, REG_UART3_MODE, REG_UART3_MODE_MASK, BIT14, PINMUX_FOR_UART3_MODE_4},
    {PAD_GPIO15, PADTOP_BANK, REG_UART4_MODE, REG_UART4_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_UART4_MODE_5},
    {PAD_GPIO15, PADTOP_BANK, REG_PWM7_MODE, REG_PWM7_MODE_MASK, BIT14 | BIT13, PINMUX_FOR_PWM7_MODE_6},
    {PAD_GPIO15, PADTOP_BANK, REG_I2S1_MCK_MODE, REG_I2S1_MCK_MODE_MASK, BIT6, PINMUX_FOR_I2S1_MCK_MODE_4},
    {PAD_GPIO15, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_GPIO15, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9, PINMUX_FOR_TTL24_MODE_2},
    {PAD_GPIO15, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_TTL24_MODE_3},
    {PAD_GPIO15, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT0, PINMUX_FOR_TTL16_MODE_1},
    {PAD_GPIO15, PADTOP_BANK, REG_LCD_MCU16_MODE, REG_LCD_MCU16_MODE_MASK, BIT2, PINMUX_FOR_LCD_MCU16_MODE_1},
    {PAD_GPIO15, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo gpio16_tbl[] = {
    {PAD_GPIO16, PADGPIO2_BANK, REG_GPIO16_GPIO_MODE, REG_GPIO16_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO16, PADTOP_BANK, REG_I2C5_MODE, REG_I2C5_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_I2C5_MODE_5},
    {PAD_GPIO16, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10 | BIT9 | BIT8, PINMUX_FOR_FUART_MODE_7},
    {PAD_GPIO16, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT14 | BIT13 | BIT12,
     PINMUX_FOR_FUART_2W_MODE_7},
    {PAD_GPIO16, PADTOP_BANK, REG_UART2_MODE, REG_UART2_MODE_MASK, BIT10 | BIT9, PINMUX_FOR_UART2_MODE_6},
    {PAD_GPIO16, PADTOP_BANK, REG_UART3_MODE, REG_UART3_MODE_MASK, BIT13, PINMUX_FOR_UART3_MODE_2},
    {PAD_GPIO16, PADTOP_BANK, REG_UART5_MODE, REG_UART5_MODE_MASK, BIT6, PINMUX_FOR_UART5_MODE_4},
    {PAD_GPIO16, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT3, PINMUX_FOR_EMMC_8B_MODE_2},
    {PAD_GPIO16, PADTOP_BANK, REG_PWM8_MODE, REG_PWM8_MODE_MASK, BIT2 | BIT1 | BIT0, PINMUX_FOR_PWM8_MODE_7},
    {PAD_GPIO16, PADTOP_BANK, REG_I2S0_MCK_MODE, REG_I2S0_MCK_MODE_MASK, BIT3 | BIT0, PINMUX_FOR_I2S0_MCK_MODE_9},
    {PAD_GPIO16, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT3 | BIT0, PINMUX_FOR_DMIC_MODE_9},
    {PAD_GPIO16, PADTOP_BANK, REG_MII1_MODE, REG_MII1_MODE_MASK, BIT1, PINMUX_FOR_MII1_MODE_1},
    {PAD_GPIO16, PADTOP_BANK, REG_OTP_TEST, REG_OTP_TEST_MASK, BIT8, PINMUX_FOR_OTP_TEST_1},
};
const ST_PadMuxInfo gpio17_tbl[] = {
    {PAD_GPIO17, PADGPIO2_BANK, REG_GPIO17_GPIO_MODE, REG_GPIO17_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_GPIO17, PADTOP_BANK, REG_I2C5_MODE, REG_I2C5_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_I2C5_MODE_5},
    {PAD_GPIO17, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10 | BIT9 | BIT8, PINMUX_FOR_FUART_MODE_7},
    {PAD_GPIO17, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT14 | BIT13 | BIT12,
     PINMUX_FOR_FUART_2W_MODE_7},
    {PAD_GPIO17, PADTOP_BANK, REG_UART2_MODE, REG_UART2_MODE_MASK, BIT10 | BIT9, PINMUX_FOR_UART2_MODE_6},
    {PAD_GPIO17, PADTOP_BANK, REG_UART3_MODE, REG_UART3_MODE_MASK, BIT13, PINMUX_FOR_UART3_MODE_2},
    {PAD_GPIO17, PADTOP_BANK, REG_UART5_MODE, REG_UART5_MODE_MASK, BIT6, PINMUX_FOR_UART5_MODE_4},
    {PAD_GPIO17, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT3, PINMUX_FOR_EMMC_8B_MODE_2},
    {PAD_GPIO17, PADTOP_BANK, REG_PWM9_MODE, REG_PWM9_MODE_MASK, BIT6 | BIT5, PINMUX_FOR_PWM9_MODE_6},
    {PAD_GPIO17, PADTOP_BANK, REG_I2S1_MCK_MODE, REG_I2S1_MCK_MODE_MASK, BIT7 | BIT5, PINMUX_FOR_I2S1_MCK_MODE_10},
    {PAD_GPIO17, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT3 | BIT0, PINMUX_FOR_DMIC_MODE_9},
    {PAD_GPIO17, PADTOP_BANK, REG_MII1_MODE, REG_MII1_MODE_MASK, BIT1, PINMUX_FOR_MII1_MODE_1},
};
const ST_PadMuxInfo sd1_gpio0_tbl[] = {
    {PAD_SD1_GPIO0, PADGPIO2_BANK, REG_SD1_GPIO0_GPIO_MODE, REG_SD1_GPIO0_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD1_GPIO0, PADTOP_BANK, REG_I2C9_MODE, REG_I2C9_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_I2C9_MODE_3},
    {PAD_SD1_GPIO0, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10 | BIT9 | BIT8, PINMUX_FOR_FUART_MODE_7},
    {PAD_SD1_GPIO0, PADTOP_BANK, REG_UART1_MODE, REG_UART1_MODE_MASK, BIT6, PINMUX_FOR_UART1_MODE_4},
    {PAD_SD1_GPIO0, PADTOP_BANK, REG_UART4_MODE, REG_UART4_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_UART4_MODE_3},
    {PAD_SD1_GPIO0, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT3, PINMUX_FOR_EMMC_8B_MODE_2},
    {PAD_SD1_GPIO0, PADTOP_BANK, REG_I2S0_TX_MODE, REG_I2S0_TX_MODE_MASK, BIT14, PINMUX_FOR_I2S0_TX_MODE_4},
};
const ST_PadMuxInfo sd1_gpio1_tbl[] = {
    {PAD_SD1_GPIO1, PADGPIO2_BANK, REG_SD1_GPIO1_GPIO_MODE, REG_SD1_GPIO1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD1_GPIO1, PADTOP_BANK, REG_I2C9_MODE, REG_I2C9_MODE_MASK, BIT13 | BIT12, PINMUX_FOR_I2C9_MODE_3},
    {PAD_SD1_GPIO1, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT10 | BIT9 | BIT8, PINMUX_FOR_FUART_MODE_7},
    {PAD_SD1_GPIO1, PADTOP_BANK, REG_UART1_MODE, REG_UART1_MODE_MASK, BIT6, PINMUX_FOR_UART1_MODE_4},
    {PAD_SD1_GPIO1, PADTOP_BANK, REG_UART4_MODE, REG_UART4_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_UART4_MODE_3},
    {PAD_SD1_GPIO1, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT3, PINMUX_FOR_EMMC_8B_MODE_2},
    {PAD_SD1_GPIO1, PADTOP_BANK, REG_I2S0_MCK_MODE, REG_I2S0_MCK_MODE_MASK, BIT3 | BIT1, PINMUX_FOR_I2S0_MCK_MODE_10},
    {PAD_SD1_GPIO1, PADTOP_BANK, REG_I2S0_TX_MODE, REG_I2S0_TX_MODE_MASK, BIT14, PINMUX_FOR_I2S0_TX_MODE_4},
};
const ST_PadMuxInfo sd1_cdz_tbl[] = {
    {PAD_SD1_CDZ, PADGPIO2_BANK, REG_SD1_CDZ_GPIO_MODE, REG_SD1_CDZ_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD1_CDZ, PADTOP_BANK, REG_SPI1_MODE, REG_SPI1_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_SPI1_MODE_3},
    {PAD_SD1_CDZ, PADTOP_BANK, REG_SD1_CDZ_MODE, REG_SD1_CDZ_MODE_MASK, BIT12, PINMUX_FOR_SD1_CDZ_MODE_1},
    {PAD_SD1_CDZ, PADTOP_BANK, REG_EMMC_RST_MODE, REG_EMMC_RST_MODE_MASK, BIT5, PINMUX_FOR_EMMC_RST_MODE_2},
    {PAD_SD1_CDZ, PADTOP_BANK, REG_I2S0_TX_MODE, REG_I2S0_TX_MODE_MASK, BIT14, PINMUX_FOR_I2S0_TX_MODE_4},
    {PAD_SD1_CDZ, PADTOP_BANK, REG_I2S0_RXTX_MODE, REG_I2S0_RXTX_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2S0_RXTX_MODE_3},
};
const ST_PadMuxInfo sd1_d1_tbl[] = {
    {PAD_SD1_D1, PADGPIO2_BANK, REG_SD1_D1_GPIO_MODE, REG_SD1_D1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD1_D1, PADTOP_BANK, REG_SPI1_MODE, REG_SPI1_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_SPI1_MODE_3},
    {PAD_SD1_D1, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT12, PINMUX_FOR_SD1_MODE_1},
    {PAD_SD1_D1, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT3, PINMUX_FOR_EMMC_8B_MODE_2},
    {PAD_SD1_D1, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT1, PINMUX_FOR_EMMC_4B_MODE_2},
    {PAD_SD1_D1, PADTOP_BANK, REG_I2S0_RX_MODE, REG_I2S0_RX_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_I2S0_RX_MODE_3},
    {PAD_SD1_D1, PADTOP_BANK, REG_I2S0_RXTX_MODE, REG_I2S0_RXTX_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2S0_RXTX_MODE_3},
};
const ST_PadMuxInfo sd1_d0_tbl[] = {
    {PAD_SD1_D0, PADGPIO2_BANK, REG_SD1_D0_GPIO_MODE, REG_SD1_D0_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD1_D0, PADTOP_BANK, REG_SPI1_MODE, REG_SPI1_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_SPI1_MODE_3},
    {PAD_SD1_D0, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT12, PINMUX_FOR_SD1_MODE_1},
    {PAD_SD1_D0, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT3, PINMUX_FOR_EMMC_8B_MODE_2},
    {PAD_SD1_D0, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT1, PINMUX_FOR_EMMC_4B_MODE_2},
    {PAD_SD1_D0, PADTOP_BANK, REG_I2S0_RX_MODE, REG_I2S0_RX_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_I2S0_RX_MODE_3},
    {PAD_SD1_D0, PADTOP_BANK, REG_I2S0_RXTX_MODE, REG_I2S0_RXTX_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2S0_RXTX_MODE_3},
};
const ST_PadMuxInfo sd1_clk_tbl[] = {
    {PAD_SD1_CLK, PADGPIO2_BANK, REG_SD1_CLK_GPIO_MODE, REG_SD1_CLK_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD1_CLK, PADTOP_BANK, REG_SPI1_MODE, REG_SPI1_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_SPI1_MODE_3},
    {PAD_SD1_CLK, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT12, PINMUX_FOR_SD1_MODE_1},
    {PAD_SD1_CLK, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT3, PINMUX_FOR_EMMC_8B_MODE_2},
    {PAD_SD1_CLK, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT1, PINMUX_FOR_EMMC_4B_MODE_2},
    {PAD_SD1_CLK, PADTOP_BANK, REG_I2S0_RX_MODE, REG_I2S0_RX_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_I2S0_RX_MODE_3},
    {PAD_SD1_CLK, PADTOP_BANK, REG_I2S0_RXTX_MODE, REG_I2S0_RXTX_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2S0_RXTX_MODE_3},
};
const ST_PadMuxInfo sd1_cmd_tbl[] = {
    {PAD_SD1_CMD, PADGPIO2_BANK, REG_SD1_CMD_GPIO_MODE, REG_SD1_CMD_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD1_CMD, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT12, PINMUX_FOR_SD1_MODE_1},
    {PAD_SD1_CMD, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT3, PINMUX_FOR_EMMC_8B_MODE_2},
    {PAD_SD1_CMD, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT1, PINMUX_FOR_EMMC_4B_MODE_2},
    {PAD_SD1_CMD, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT5, PINMUX_FOR_I2S1_RXTX_MODE_2},
    {PAD_SD1_CMD, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_I2S1_RXTX_MODE_3},
    {PAD_SD1_CMD, PADTOP_BANK, REG_I2S1_RXTX_MODE, REG_I2S1_RXTX_MODE_MASK, BIT6, PINMUX_FOR_I2S1_RXTX_MODE_4},
    {PAD_SD1_CMD, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT3 | BIT0, PINMUX_FOR_DMIC_4CH_MODE_9},
};
const ST_PadMuxInfo sd1_d3_tbl[] = {
    {PAD_SD1_D3, PADGPIO2_BANK, REG_SD1_D3_GPIO_MODE, REG_SD1_D3_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD1_D3, PADTOP_BANK, REG_I2C8_MODE, REG_I2C8_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_I2C8_MODE_5},
    {PAD_SD1_D3, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT12, PINMUX_FOR_SD1_MODE_1},
    {PAD_SD1_D3, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT3, PINMUX_FOR_EMMC_8B_MODE_2},
    {PAD_SD1_D3, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT1, PINMUX_FOR_EMMC_4B_MODE_2},
    {PAD_SD1_D3, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT3 | BIT1, PINMUX_FOR_DMIC_MODE_10},
    {PAD_SD1_D3, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT3 | BIT0, PINMUX_FOR_DMIC_4CH_MODE_9},
};
const ST_PadMuxInfo sd1_d2_tbl[] = {
    {PAD_SD1_D2, PADGPIO2_BANK, REG_SD1_D2_GPIO_MODE, REG_SD1_D2_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_SD1_D2, PADTOP_BANK, REG_I2C8_MODE, REG_I2C8_MODE_MASK, BIT10 | BIT8, PINMUX_FOR_I2C8_MODE_5},
    {PAD_SD1_D2, PADTOP_BANK, REG_SD1_MODE, REG_SD1_MODE_MASK, BIT12, PINMUX_FOR_SD1_MODE_1},
    {PAD_SD1_D2, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT3, PINMUX_FOR_EMMC_8B_MODE_2},
    {PAD_SD1_D2, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT1, PINMUX_FOR_EMMC_4B_MODE_2},
    {PAD_SD1_D2, PADTOP_BANK, REG_DMIC_MODE, REG_DMIC_MODE_MASK, BIT3 | BIT1, PINMUX_FOR_DMIC_MODE_10},
    {PAD_SD1_D2, PADTOP_BANK, REG_DMIC_4CH_MODE, REG_DMIC_4CH_MODE_MASK, BIT3 | BIT0, PINMUX_FOR_DMIC_4CH_MODE_9},
};
const ST_PadMuxInfo emmc_d0_tbl[] = {
    {PAD_EMMC_D0, PADGPIO2_BANK, REG_EMMC_D0_GPIO_MODE, REG_EMMC_D0_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_EMMC_D0, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_EMMC_D0, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_EMMC_D0, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT2, PINMUX_FOR_EMMC_8B_MODE_1},
    {PAD_EMMC_D0, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT0, PINMUX_FOR_EMMC_4B_MODE_1},
};
const ST_PadMuxInfo emmc_d1_tbl[] = {
    {PAD_EMMC_D1, PADGPIO2_BANK, REG_EMMC_D1_GPIO_MODE, REG_EMMC_D1_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_EMMC_D1, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_EMMC_D1, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_EMMC_D1, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT2, PINMUX_FOR_EMMC_8B_MODE_1},
    {PAD_EMMC_D1, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT0, PINMUX_FOR_EMMC_4B_MODE_1},
};
const ST_PadMuxInfo emmc_d2_tbl[] = {
    {PAD_EMMC_D2, PADGPIO2_BANK, REG_EMMC_D2_GPIO_MODE, REG_EMMC_D2_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_EMMC_D2, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_EMMC_D2, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_EMMC_D2, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT2, PINMUX_FOR_EMMC_8B_MODE_1},
    {PAD_EMMC_D2, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT0, PINMUX_FOR_EMMC_4B_MODE_1},
};
const ST_PadMuxInfo emmc_d3_tbl[] = {
    {PAD_EMMC_D3, PADGPIO2_BANK, REG_EMMC_D3_GPIO_MODE, REG_EMMC_D3_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_EMMC_D3, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_EMMC_D3, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_EMMC_D3, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT2, PINMUX_FOR_EMMC_8B_MODE_1},
    {PAD_EMMC_D3, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT0, PINMUX_FOR_EMMC_4B_MODE_1},
};
const ST_PadMuxInfo emmc_d4_tbl[] = {
    {PAD_EMMC_D4, PADGPIO2_BANK, REG_EMMC_D4_GPIO_MODE, REG_EMMC_D4_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_EMMC_D4, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_EMMC_D4, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_EMMC_D4, PADTOP_BANK, REG_I2C10_MODE, REG_I2C10_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2C10_MODE_3},
    {PAD_EMMC_D4, PADTOP_BANK, REG_SPI2_MODE, REG_SPI2_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_SPI2_MODE_3},
    {PAD_EMMC_D4, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT2, PINMUX_FOR_EMMC_8B_MODE_1},
};
const ST_PadMuxInfo emmc_d5_tbl[] = {
    {PAD_EMMC_D5, PADGPIO2_BANK, REG_EMMC_D5_GPIO_MODE, REG_EMMC_D5_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_EMMC_D5, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_EMMC_D5, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_EMMC_D5, PADTOP_BANK, REG_I2C10_MODE, REG_I2C10_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_I2C10_MODE_3},
    {PAD_EMMC_D5, PADTOP_BANK, REG_SPI2_MODE, REG_SPI2_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_SPI2_MODE_3},
    {PAD_EMMC_D5, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT2, PINMUX_FOR_EMMC_8B_MODE_1},
};
const ST_PadMuxInfo emmc_d6_tbl[] = {
    {PAD_EMMC_D6, PADGPIO2_BANK, REG_EMMC_D6_GPIO_MODE, REG_EMMC_D6_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_EMMC_D6, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_EMMC_D6, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_EMMC_D6, PADTOP_BANK, REG_SPI2_MODE, REG_SPI2_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_SPI2_MODE_3},
    {PAD_EMMC_D6, PADTOP_BANK, REG_UART5_MODE, REG_UART5_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_UART5_MODE_5},
    {PAD_EMMC_D6, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT2, PINMUX_FOR_EMMC_8B_MODE_1},
};
const ST_PadMuxInfo emmc_d7_tbl[] = {
    {PAD_EMMC_D7, PADGPIO2_BANK, REG_EMMC_D7_GPIO_MODE, REG_EMMC_D7_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_EMMC_D7, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_EMMC_D7, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_EMMC_D7, PADTOP_BANK, REG_SPI2_MODE, REG_SPI2_MODE_MASK, BIT9 | BIT8, PINMUX_FOR_SPI2_MODE_3},
    {PAD_EMMC_D7, PADTOP_BANK, REG_UART5_MODE, REG_UART5_MODE_MASK, BIT6 | BIT4, PINMUX_FOR_UART5_MODE_5},
    {PAD_EMMC_D7, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT2, PINMUX_FOR_EMMC_8B_MODE_1},
};
const ST_PadMuxInfo emmc_clk_tbl[] = {
    {PAD_EMMC_CLK, PADGPIO2_BANK, REG_EMMC_CLK_GPIO_MODE, REG_EMMC_CLK_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_EMMC_CLK, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_EMMC_CLK, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_EMMC_CLK, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT2, PINMUX_FOR_EMMC_8B_MODE_1},
    {PAD_EMMC_CLK, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT0, PINMUX_FOR_EMMC_4B_MODE_1},
};
const ST_PadMuxInfo emmc_cmd_tbl[] = {
    {PAD_EMMC_CMD, PADGPIO2_BANK, REG_EMMC_CMD_GPIO_MODE, REG_EMMC_CMD_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_EMMC_CMD, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_EMMC_CMD, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_EMMC_CMD, PADTOP_BANK, REG_EMMC_8B_MODE, REG_EMMC_8B_MODE_MASK, BIT2, PINMUX_FOR_EMMC_8B_MODE_1},
    {PAD_EMMC_CMD, PADTOP_BANK, REG_EMMC_4B_MODE, REG_EMMC_4B_MODE_MASK, BIT0, PINMUX_FOR_EMMC_4B_MODE_1},
};
const ST_PadMuxInfo emmc_rst_tbl[] = {
    {PAD_EMMC_RST, PADGPIO2_BANK, REG_EMMC_RST_GPIO_MODE, REG_EMMC_RST_GPIO_MODE_MASK, BIT3, PINMUX_FOR_GPIO_MODE},
    {PAD_EMMC_RST, CHIPTOP_BANK, REG_TEST_IN_MODE, REG_TEST_IN_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_TEST_IN_MODE_3},
    {PAD_EMMC_RST, CHIPTOP_BANK, REG_TEST_OUT_MODE, REG_TEST_OUT_MODE_MASK, BIT5 | BIT4, PINMUX_FOR_TEST_OUT_MODE_3},
    {PAD_EMMC_RST, PADTOP_BANK, REG_EMMC_RST_MODE, REG_EMMC_RST_MODE_MASK, BIT4, PINMUX_FOR_EMMC_RST_MODE_1},
};
const ST_PadMuxInfo rgmii0_txclk_tbl[] = {
    {PAD_RGMII0_TXCLK, PADTOP_BANK, REG_RGMII0_TXCLK_EXT_EN_MODE, REG_RGMII0_TXCLK_EXT_EN_MODE_MASK, BIT0,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_TXCLK, PADGPIO2_BANK, REG_RGMII0_TXCLK_GPIO_MODE, REG_RGMII0_TXCLK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_TXCLK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_TXCLK, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_TXCLK, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT9, PINMUX_FOR_RGB8_MODE_2},
    {PAD_RGMII0_TXCLK, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_TXCLK, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_BT656_OUT_MODE_3},
    {PAD_RGMII0_TXCLK, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT601_OUT_MODE_2},
    {PAD_RGMII0_TXCLK, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_txd0_tbl[] = {
    {PAD_RGMII0_TXD0, PADTOP_BANK, REG_RGMII0_TXD0_EXT_EN_MODE, REG_RGMII0_TXD0_EXT_EN_MODE_MASK, BIT1,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_TXD0, PADGPIO2_BANK, REG_RGMII0_TXD0_GPIO_MODE, REG_RGMII0_TXD0_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_TXD0, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_TXD0, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_TXD0, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT9, PINMUX_FOR_RGB8_MODE_2},
    {PAD_RGMII0_TXD0, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_TXD0, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_BT656_OUT_MODE_3},
    {PAD_RGMII0_TXD0, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT601_OUT_MODE_2},
    {PAD_RGMII0_TXD0, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_txd1_tbl[] = {
    {PAD_RGMII0_TXD1, PADTOP_BANK, REG_RGMII0_TXD1_EXT_EN_MODE, REG_RGMII0_TXD1_EXT_EN_MODE_MASK, BIT2,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_TXD1, PADGPIO2_BANK, REG_RGMII0_TXD1_GPIO_MODE, REG_RGMII0_TXD1_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_TXD1, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_TXD1, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_TXD1, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT9, PINMUX_FOR_RGB8_MODE_2},
    {PAD_RGMII0_TXD1, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_TXD1, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_BT656_OUT_MODE_3},
    {PAD_RGMII0_TXD1, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT601_OUT_MODE_2},
    {PAD_RGMII0_TXD1, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_txd2_tbl[] = {
    {PAD_RGMII0_TXD2, PADTOP_BANK, REG_RGMII0_TXD2_EXT_EN_MODE, REG_RGMII0_TXD2_EXT_EN_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_TXD2, PADGPIO2_BANK, REG_RGMII0_TXD2_GPIO_MODE, REG_RGMII0_TXD2_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_TXD2, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_TXD2, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_TXD2, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT9, PINMUX_FOR_RGB8_MODE_2},
    {PAD_RGMII0_TXD2, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_TXD2, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_BT656_OUT_MODE_3},
    {PAD_RGMII0_TXD2, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT601_OUT_MODE_2},
    {PAD_RGMII0_TXD2, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_txd3_tbl[] = {
    {PAD_RGMII0_TXD3, PADTOP_BANK, REG_RGMII0_TXD3_EXT_EN_MODE, REG_RGMII0_TXD3_EXT_EN_MODE_MASK, BIT4,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_TXD3, PADGPIO2_BANK, REG_RGMII0_TXD3_GPIO_MODE, REG_RGMII0_TXD3_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_TXD3, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_TXD3, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_TXD3, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT9, PINMUX_FOR_RGB8_MODE_2},
    {PAD_RGMII0_TXD3, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_TXD3, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_BT656_OUT_MODE_3},
    {PAD_RGMII0_TXD3, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT601_OUT_MODE_2},
    {PAD_RGMII0_TXD3, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_txctl_tbl[] = {
    {PAD_RGMII0_TXCTL, PADTOP_BANK, REG_RGMII0_TXCTL_EXT_EN_MODE, REG_RGMII0_TXCTL_EXT_EN_MODE_MASK, BIT5,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_TXCTL, PADGPIO2_BANK, REG_RGMII0_TXCTL_GPIO_MODE, REG_RGMII0_TXCTL_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_TXCTL, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_TXCTL, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_TXCTL, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT9, PINMUX_FOR_RGB8_MODE_2},
    {PAD_RGMII0_TXCTL, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_TXCTL, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_BT656_OUT_MODE_3},
    {PAD_RGMII0_TXCTL, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT601_OUT_MODE_2},
    {PAD_RGMII0_TXCTL, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_rxclk_tbl[] = {
    {PAD_RGMII0_RXCLK, PADTOP_BANK, REG_RGMII0_RXCLK_EXT_EN_MODE, REG_RGMII0_RXCLK_EXT_EN_MODE_MASK, BIT6,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_RXCLK, PADGPIO2_BANK, REG_RGMII0_RXCLK_GPIO_MODE, REG_RGMII0_RXCLK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_RXCLK, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_RXCLK, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_RXCLK, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT9, PINMUX_FOR_RGB8_MODE_2},
    {PAD_RGMII0_RXCLK, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_RXCLK, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_BT656_OUT_MODE_3},
    {PAD_RGMII0_RXCLK, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT601_OUT_MODE_2},
    {PAD_RGMII0_RXCLK, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_rxd0_tbl[] = {
    {PAD_RGMII0_RXD0, PADTOP_BANK, REG_RGMII0_RXD0_EXT_EN_MODE, REG_RGMII0_RXD0_EXT_EN_MODE_MASK, BIT7,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_RXD0, PADGPIO2_BANK, REG_RGMII0_RXD0_GPIO_MODE, REG_RGMII0_RXD0_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_RXD0, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_RXD0, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_RXD0, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT9, PINMUX_FOR_RGB8_MODE_2},
    {PAD_RGMII0_RXD0, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_RXD0, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_BT656_OUT_MODE_3},
    {PAD_RGMII0_RXD0, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT601_OUT_MODE_2},
    {PAD_RGMII0_RXD0, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_rxd1_tbl[] = {
    {PAD_RGMII0_RXD1, PADTOP_BANK, REG_RGMII0_RXD1_EXT_EN_MODE, REG_RGMII0_RXD1_EXT_EN_MODE_MASK, BIT8,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_RXD1, PADGPIO2_BANK, REG_RGMII0_RXD1_GPIO_MODE, REG_RGMII0_RXD1_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_RXD1, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_RXD1, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_RXD1, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT9, PINMUX_FOR_RGB8_MODE_2},
    {PAD_RGMII0_RXD1, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_RXD1, PADTOP_BANK, REG_BT656_OUT_MODE, REG_BT656_OUT_MODE_MASK, BIT5 | BIT4,
     PINMUX_FOR_BT656_OUT_MODE_3},
    {PAD_RGMII0_RXD1, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT601_OUT_MODE_2},
    {PAD_RGMII0_RXD1, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_rxd2_tbl[] = {
    {PAD_RGMII0_RXD2, PADTOP_BANK, REG_RGMII0_RXD2_EXT_EN_MODE, REG_RGMII0_RXD2_EXT_EN_MODE_MASK, BIT9,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_RXD2, PADGPIO2_BANK, REG_RGMII0_RXD2_GPIO_MODE, REG_RGMII0_RXD2_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_RXD2, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_RXD2, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_RXD2, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT9, PINMUX_FOR_RGB8_MODE_2},
    {PAD_RGMII0_RXD2, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_RXD2, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT601_OUT_MODE_2},
    {PAD_RGMII0_RXD2, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_rxd3_tbl[] = {
    {PAD_RGMII0_RXD3, PADTOP_BANK, REG_RGMII0_RXD3_EXT_EN_MODE, REG_RGMII0_RXD3_EXT_EN_MODE_MASK, BIT10,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_RXD3, PADGPIO2_BANK, REG_RGMII0_RXD3_GPIO_MODE, REG_RGMII0_RXD3_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_RXD3, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_RXD3, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_RXD3, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT9, PINMUX_FOR_RGB8_MODE_2},
    {PAD_RGMII0_RXD3, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_RXD3, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT601_OUT_MODE_2},
    {PAD_RGMII0_RXD3, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_rxctl_tbl[] = {
    {PAD_RGMII0_RXCTL, PADTOP_BANK, REG_RGMII0_RXCTL_EXT_EN_MODE, REG_RGMII0_RXCTL_EXT_EN_MODE_MASK, BIT11,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_RXCTL, PADGPIO2_BANK, REG_RGMII0_RXCTL_GPIO_MODE, REG_RGMII0_RXCTL_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_RXCTL, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_RXCTL, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_RXCTL, PADTOP_BANK, REG_RGB8_MODE, REG_RGB8_MODE_MASK, BIT9, PINMUX_FOR_RGB8_MODE_2},
    {PAD_RGMII0_RXCTL, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_RXCTL, PADTOP_BANK, REG_BT601_OUT_MODE, REG_BT601_OUT_MODE_MASK, BIT5, PINMUX_FOR_BT601_OUT_MODE_2},
    {PAD_RGMII0_RXCTL, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_io0_tbl[] = {
    {PAD_RGMII0_IO0, PADTOP_BANK, REG_RGMII0_IO0_EXT_EN_MODE, REG_RGMII0_IO0_EXT_EN_MODE_MASK, BIT12,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_IO0, PADGPIO2_BANK, REG_RGMII0_IO0_GPIO_MODE, REG_RGMII0_IO0_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_IO0, PADTOP_BANK, REG_I2C10_MODE, REG_I2C10_MODE_MASK, BIT2, PINMUX_FOR_I2C10_MODE_4},
    {PAD_RGMII0_IO0, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT2, PINMUX_FOR_SPI3_MODE_4},
    {PAD_RGMII0_IO0, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT11, PINMUX_FOR_FUART_MODE_8},
    {PAD_RGMII0_IO0, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT15, PINMUX_FOR_FUART_2W_MODE_8},
    {PAD_RGMII0_IO0, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_IO0, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_IO0, PADTOP_BANK, REG_LCD_MCU8_MODE, REG_LCD_MCU8_MODE_MASK, BIT1, PINMUX_FOR_LCD_MCU8_MODE_2},
    {PAD_RGMII0_IO0, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
    {PAD_RGMII0_IO0, PADTOP_BANK, REG_MII0_MODE, REG_MII0_MODE_MASK, BIT0, PINMUX_FOR_MII0_MODE_1},
    {PAD_RGMII0_IO0, PADTOP_BANK, REG_GPHY0_REF_MODE, REG_GPHY0_REF_MODE_MASK, BIT0, PINMUX_FOR_GPHY0_REF_MODE_1},
};
const ST_PadMuxInfo rgmii0_io1_tbl[] = {
    {PAD_RGMII0_IO1, PADTOP_BANK, REG_RGMII0_IO1_EXT_EN_MODE, REG_RGMII0_IO1_EXT_EN_MODE_MASK, BIT13,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_IO1, PADGPIO2_BANK, REG_RGMII0_IO1_GPIO_MODE, REG_RGMII0_IO1_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_IO1, PADTOP_BANK, REG_I2C10_MODE, REG_I2C10_MODE_MASK, BIT2, PINMUX_FOR_I2C10_MODE_4},
    {PAD_RGMII0_IO1, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT2, PINMUX_FOR_SPI3_MODE_4},
    {PAD_RGMII0_IO1, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT11, PINMUX_FOR_FUART_MODE_8},
    {PAD_RGMII0_IO1, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT15, PINMUX_FOR_FUART_2W_MODE_8},
    {PAD_RGMII0_IO1, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_IO1, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_IO1, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
    {PAD_RGMII0_IO1, PADTOP_BANK, REG_MII0_MODE, REG_MII0_MODE_MASK, BIT0, PINMUX_FOR_MII0_MODE_1},
    {PAD_RGMII0_IO1, PADTOP_BANK, REG_GPHY0_REF_MODE, REG_GPHY0_REF_MODE_MASK, BIT1, PINMUX_FOR_GPHY0_REF_MODE_2},
};
const ST_PadMuxInfo rgmii0_mdc_tbl[] = {
    {PAD_RGMII0_MDC, PADTOP_BANK, REG_RGMII0_MDC_EXT_EN_MODE, REG_RGMII0_MDC_EXT_EN_MODE_MASK, BIT14,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_MDC, PADGPIO2_BANK, REG_RGMII0_MDC_GPIO_MODE, REG_RGMII0_MDC_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_MDC, PADTOP_BANK, REG_I2C6_MODE, REG_I2C6_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_I2C6_MODE_5},
    {PAD_RGMII0_MDC, PADTOP_BANK, REG_I2C10_MODE, REG_I2C10_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_I2C10_MODE_5},
    {PAD_RGMII0_MDC, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT2, PINMUX_FOR_SPI3_MODE_4},
    {PAD_RGMII0_MDC, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT11, PINMUX_FOR_FUART_MODE_8},
    {PAD_RGMII0_MDC, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_MDC, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_MDC, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii0_mdio_tbl[] = {
    {PAD_RGMII0_MDIO, PADTOP_BANK, REG_RGMII0_MDIO_EXT_EN_MODE, REG_RGMII0_MDIO_EXT_EN_MODE_MASK, BIT15,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_MDIO, PADGPIO2_BANK, REG_RGMII0_MDIO_GPIO_MODE, REG_RGMII0_MDIO_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII0_MDIO, PADTOP_BANK, REG_I2C6_MODE, REG_I2C6_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_I2C6_MODE_5},
    {PAD_RGMII0_MDIO, PADTOP_BANK, REG_I2C10_MODE, REG_I2C10_MODE_MASK, BIT2 | BIT0, PINMUX_FOR_I2C10_MODE_5},
    {PAD_RGMII0_MDIO, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT2, PINMUX_FOR_SPI3_MODE_4},
    {PAD_RGMII0_MDIO, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT11, PINMUX_FOR_FUART_MODE_8},
    {PAD_RGMII0_MDIO, PADTOP_BANK, REG_TTL24_MODE, REG_TTL24_MODE_MASK, BIT8, PINMUX_FOR_TTL24_MODE_1},
    {PAD_RGMII0_MDIO, PADTOP_BANK, REG_TTL16_MODE, REG_TTL16_MODE_MASK, BIT1, PINMUX_FOR_TTL16_MODE_2},
    {PAD_RGMII0_MDIO, PADTOP_BANK, REG_BT1120_OUT_MODE, REG_BT1120_OUT_MODE_MASK, BIT1, PINMUX_FOR_BT1120_OUT_MODE_2},
};
const ST_PadMuxInfo rgmii1_txclk_tbl[] = {
    {PAD_RGMII1_TXCLK, PADTOP_BANK, REG_RGMII1_TXCLK_EXT_EN_MODE, REG_RGMII1_TXCLK_EXT_EN_MODE_MASK, BIT0,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_TXCLK, PADGPIO2_BANK, REG_RGMII1_TXCLK_GPIO_MODE, REG_RGMII1_TXCLK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo rgmii1_txd0_tbl[] = {
    {PAD_RGMII1_TXD0, PADTOP_BANK, REG_RGMII1_TXD0_EXT_EN_MODE, REG_RGMII1_TXD0_EXT_EN_MODE_MASK, BIT1,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_TXD0, PADGPIO2_BANK, REG_RGMII1_TXD0_GPIO_MODE, REG_RGMII1_TXD0_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo rgmii1_txd1_tbl[] = {
    {PAD_RGMII1_TXD1, PADTOP_BANK, REG_RGMII1_TXD1_EXT_EN_MODE, REG_RGMII1_TXD1_EXT_EN_MODE_MASK, BIT2,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_TXD1, PADGPIO2_BANK, REG_RGMII1_TXD1_GPIO_MODE, REG_RGMII1_TXD1_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo rgmii1_txd2_tbl[] = {
    {PAD_RGMII1_TXD2, PADTOP_BANK, REG_RGMII1_TXD2_EXT_EN_MODE, REG_RGMII1_TXD2_EXT_EN_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_TXD2, PADGPIO2_BANK, REG_RGMII1_TXD2_GPIO_MODE, REG_RGMII1_TXD2_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo rgmii1_txd3_tbl[] = {
    {PAD_RGMII1_TXD3, PADTOP_BANK, REG_RGMII1_TXD3_EXT_EN_MODE, REG_RGMII1_TXD3_EXT_EN_MODE_MASK, BIT4,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_TXD3, PADGPIO2_BANK, REG_RGMII1_TXD3_GPIO_MODE, REG_RGMII1_TXD3_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo rgmii1_txctl_tbl[] = {
    {PAD_RGMII1_TXCTL, PADTOP_BANK, REG_RGMII1_TXCTL_EXT_EN_MODE, REG_RGMII1_TXCTL_EXT_EN_MODE_MASK, BIT5,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_TXCTL, PADGPIO2_BANK, REG_RGMII1_TXCTL_GPIO_MODE, REG_RGMII1_TXCTL_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo rgmii1_rxclk_tbl[] = {
    {PAD_RGMII1_RXCLK, PADTOP_BANK, REG_RGMII1_RXCLK_EXT_EN_MODE, REG_RGMII1_RXCLK_EXT_EN_MODE_MASK, BIT6,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_RXCLK, PADGPIO2_BANK, REG_RGMII1_RXCLK_GPIO_MODE, REG_RGMII1_RXCLK_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo rgmii1_rxd0_tbl[] = {
    {PAD_RGMII1_RXD0, PADTOP_BANK, REG_RGMII1_RXD0_EXT_EN_MODE, REG_RGMII1_RXD0_EXT_EN_MODE_MASK, BIT7,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_RXD0, PADGPIO2_BANK, REG_RGMII1_RXD0_GPIO_MODE, REG_RGMII1_RXD0_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo rgmii1_rxd1_tbl[] = {
    {PAD_RGMII1_RXD1, PADTOP_BANK, REG_RGMII1_RXD1_EXT_EN_MODE, REG_RGMII1_RXD1_EXT_EN_MODE_MASK, BIT8,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_RXD1, PADGPIO2_BANK, REG_RGMII1_RXD1_GPIO_MODE, REG_RGMII1_RXD1_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo rgmii1_rxd2_tbl[] = {
    {PAD_RGMII1_RXD2, PADTOP_BANK, REG_RGMII1_RXD2_EXT_EN_MODE, REG_RGMII1_RXD2_EXT_EN_MODE_MASK, BIT9,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_RXD2, PADGPIO2_BANK, REG_RGMII1_RXD2_GPIO_MODE, REG_RGMII1_RXD2_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo rgmii1_rxd3_tbl[] = {
    {PAD_RGMII1_RXD3, PADTOP_BANK, REG_RGMII1_RXD3_EXT_EN_MODE, REG_RGMII1_RXD3_EXT_EN_MODE_MASK, BIT10,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_RXD3, PADGPIO2_BANK, REG_RGMII1_RXD3_GPIO_MODE, REG_RGMII1_RXD3_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo rgmii1_rxctl_tbl[] = {
    {PAD_RGMII1_RXCTL, PADTOP_BANK, REG_RGMII1_RXCTL_EXT_EN_MODE, REG_RGMII1_RXCTL_EXT_EN_MODE_MASK, BIT11,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_RXCTL, PADGPIO2_BANK, REG_RGMII1_RXCTL_GPIO_MODE, REG_RGMII1_RXCTL_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo rgmii1_io0_tbl[] = {
    {PAD_RGMII1_IO0, PADTOP_BANK, REG_RGMII1_IO0_EXT_EN_MODE, REG_RGMII1_IO0_EXT_EN_MODE_MASK, BIT12,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_IO0, PADGPIO2_BANK, REG_RGMII1_IO0_GPIO_MODE, REG_RGMII1_IO0_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_IO0, PADTOP_BANK, REG_I2C9_MODE, REG_I2C9_MODE_MASK, BIT14 | BIT12, PINMUX_FOR_I2C9_MODE_5},
    {PAD_RGMII1_IO0, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SPI3_MODE_3},
    {PAD_RGMII1_IO0, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT11 | BIT8, PINMUX_FOR_FUART_MODE_9},
    {PAD_RGMII1_IO0, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT15 | BIT12, PINMUX_FOR_FUART_2W_MODE_9},
    {PAD_RGMII1_IO0, PADTOP_BANK, REG_MII1_MODE, REG_MII1_MODE_MASK, BIT1, PINMUX_FOR_MII1_MODE_1},
    {PAD_RGMII1_IO0, PADTOP_BANK, REG_GPHY1_REF_MODE, REG_GPHY1_REF_MODE_MASK, BIT4, PINMUX_FOR_GPHY1_REF_MODE_1},
};
const ST_PadMuxInfo rgmii1_io1_tbl[] = {
    {PAD_RGMII1_IO1, PADTOP_BANK, REG_RGMII1_IO1_EXT_EN_MODE, REG_RGMII1_IO1_EXT_EN_MODE_MASK, BIT13,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_IO1, PADGPIO2_BANK, REG_RGMII1_IO1_GPIO_MODE, REG_RGMII1_IO1_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_IO1, PADTOP_BANK, REG_I2C9_MODE, REG_I2C9_MODE_MASK, BIT14 | BIT12, PINMUX_FOR_I2C9_MODE_5},
    {PAD_RGMII1_IO1, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SPI3_MODE_3},
    {PAD_RGMII1_IO1, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT11 | BIT8, PINMUX_FOR_FUART_MODE_9},
    {PAD_RGMII1_IO1, PADTOP_BANK, REG_FUART_2W_MODE, REG_FUART_2W_MODE_MASK, BIT15 | BIT12, PINMUX_FOR_FUART_2W_MODE_9},
    {PAD_RGMII1_IO1, PADTOP_BANK, REG_MII1_MODE, REG_MII1_MODE_MASK, BIT1, PINMUX_FOR_MII1_MODE_1},
    {PAD_RGMII1_IO1, PADTOP_BANK, REG_GPHY1_REF_MODE, REG_GPHY1_REF_MODE_MASK, BIT5, PINMUX_FOR_GPHY1_REF_MODE_2},
};
const ST_PadMuxInfo rgmii1_mdc_tbl[] = {
    {PAD_RGMII1_MDC, PADTOP_BANK, REG_RGMII1_MDC_EXT_EN_MODE, REG_RGMII1_MDC_EXT_EN_MODE_MASK, BIT14,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_MDC, PADGPIO2_BANK, REG_RGMII1_MDC_GPIO_MODE, REG_RGMII1_MDC_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_MDC, PADTOP_BANK, REG_I2C7_MODE, REG_I2C7_MODE_MASK, BIT6 | BIT5, PINMUX_FOR_I2C7_MODE_6},
    {PAD_RGMII1_MDC, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SPI3_MODE_3},
    {PAD_RGMII1_MDC, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT11 | BIT8, PINMUX_FOR_FUART_MODE_9},
};
const ST_PadMuxInfo rgmii1_mdio_tbl[] = {
    {PAD_RGMII1_MDIO, PADTOP_BANK, REG_RGMII1_MDIO_EXT_EN_MODE, REG_RGMII1_MDIO_EXT_EN_MODE_MASK, BIT15,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_MDIO, PADGPIO2_BANK, REG_RGMII1_MDIO_GPIO_MODE, REG_RGMII1_MDIO_GPIO_MODE_MASK, BIT3,
     PINMUX_FOR_GPIO_MODE},
    {PAD_RGMII1_MDIO, PADTOP_BANK, REG_I2C7_MODE, REG_I2C7_MODE_MASK, BIT6 | BIT5, PINMUX_FOR_I2C7_MODE_6},
    {PAD_RGMII1_MDIO, PADTOP_BANK, REG_SPI3_MODE, REG_SPI3_MODE_MASK, BIT1 | BIT0, PINMUX_FOR_SPI3_MODE_3},
    {PAD_RGMII1_MDIO, PADTOP_BANK, REG_FUART_MODE, REG_FUART_MODE_MASK, BIT11 | BIT8, PINMUX_FOR_FUART_MODE_9},
};
const ST_PadMuxInfo sar_gpio0_tbl[] = {
    {PAD_SAR_GPIO0, PM_SAR_BANK, REG_SAR_GPIO0_GPIO_MODE, REG_SAR_GPIO0_GPIO_MODE_MASK, 0, PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo sar_gpio1_tbl[] = {
    {PAD_SAR_GPIO1, PM_SAR_BANK, REG_SAR_GPIO1_GPIO_MODE, REG_SAR_GPIO1_GPIO_MODE_MASK, 0, PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo sar_gpio2_tbl[] = {
    {PAD_SAR_GPIO2, PM_SAR_BANK, REG_SAR_GPIO2_GPIO_MODE, REG_SAR_GPIO2_GPIO_MODE_MASK, 0, PINMUX_FOR_GPIO_MODE},
};
const ST_PadMuxInfo sar_gpio3_tbl[] = {
    {PAD_SAR_GPIO3, PM_SAR_BANK, REG_SAR_GPIO3_GPIO_MODE, REG_SAR_GPIO3_GPIO_MODE_MASK, 0, PINMUX_FOR_GPIO_MODE},
};

const ST_PadMuxEntry m_stPadMuxEntry[] = {
    {(sizeof(irin_tbl) / sizeof(ST_PadMuxInfo)), irin_tbl},
    {(sizeof(uart_rx_tbl) / sizeof(ST_PadMuxInfo)), uart_rx_tbl},
    {(sizeof(uart_tx_tbl) / sizeof(ST_PadMuxInfo)), uart_tx_tbl},
    {(sizeof(uart_rx1_tbl) / sizeof(ST_PadMuxInfo)), uart_rx1_tbl},
    {(sizeof(uart_tx1_tbl) / sizeof(ST_PadMuxInfo)), uart_tx1_tbl},
    {(sizeof(uart_rx2_tbl) / sizeof(ST_PadMuxInfo)), uart_rx2_tbl},
    {(sizeof(uart_tx2_tbl) / sizeof(ST_PadMuxInfo)), uart_tx2_tbl},
    {(sizeof(uart_rx3_tbl) / sizeof(ST_PadMuxInfo)), uart_rx3_tbl},
    {(sizeof(uart_tx3_tbl) / sizeof(ST_PadMuxInfo)), uart_tx3_tbl},
    {(sizeof(spi_cz_tbl) / sizeof(ST_PadMuxInfo)), spi_cz_tbl},
    {(sizeof(spi_di_tbl) / sizeof(ST_PadMuxInfo)), spi_di_tbl},
    {(sizeof(spi_wpz_tbl) / sizeof(ST_PadMuxInfo)), spi_wpz_tbl},
    {(sizeof(spi_do_tbl) / sizeof(ST_PadMuxInfo)), spi_do_tbl},
    {(sizeof(spi_ck_tbl) / sizeof(ST_PadMuxInfo)), spi_ck_tbl},
    {(sizeof(spi_hld_tbl) / sizeof(ST_PadMuxInfo)), spi_hld_tbl},
    {(sizeof(sd0_gpio1_tbl) / sizeof(ST_PadMuxInfo)), sd0_gpio1_tbl},
    {(sizeof(sd0_gpio0_tbl) / sizeof(ST_PadMuxInfo)), sd0_gpio0_tbl},
    {(sizeof(sd0_cdz_tbl) / sizeof(ST_PadMuxInfo)), sd0_cdz_tbl},
    {(sizeof(sd0_d1_tbl) / sizeof(ST_PadMuxInfo)), sd0_d1_tbl},
    {(sizeof(sd0_d0_tbl) / sizeof(ST_PadMuxInfo)), sd0_d0_tbl},
    {(sizeof(sd0_clk_tbl) / sizeof(ST_PadMuxInfo)), sd0_clk_tbl},
    {(sizeof(sd0_cmd_tbl) / sizeof(ST_PadMuxInfo)), sd0_cmd_tbl},
    {(sizeof(sd0_d3_tbl) / sizeof(ST_PadMuxInfo)), sd0_d3_tbl},
    {(sizeof(sd0_d2_tbl) / sizeof(ST_PadMuxInfo)), sd0_d2_tbl},
    {(sizeof(fuart_rx_tbl) / sizeof(ST_PadMuxInfo)), fuart_rx_tbl},
    {(sizeof(fuart_tx_tbl) / sizeof(ST_PadMuxInfo)), fuart_tx_tbl},
    {(sizeof(fuart_cts_tbl) / sizeof(ST_PadMuxInfo)), fuart_cts_tbl},
    {(sizeof(fuart_rts_tbl) / sizeof(ST_PadMuxInfo)), fuart_rts_tbl},
    {(sizeof(i2c1_scl_tbl) / sizeof(ST_PadMuxInfo)), i2c1_scl_tbl},
    {(sizeof(i2c1_sda_tbl) / sizeof(ST_PadMuxInfo)), i2c1_sda_tbl},
    {(sizeof(i2c2_scl_tbl) / sizeof(ST_PadMuxInfo)), i2c2_scl_tbl},
    {(sizeof(i2c2_sda_tbl) / sizeof(ST_PadMuxInfo)), i2c2_sda_tbl},
    {(sizeof(i2c3_scl_tbl) / sizeof(ST_PadMuxInfo)), i2c3_scl_tbl},
    {(sizeof(i2c3_sda_tbl) / sizeof(ST_PadMuxInfo)), i2c3_sda_tbl},
    {(sizeof(spi0_do_tbl) / sizeof(ST_PadMuxInfo)), spi0_do_tbl},
    {(sizeof(spi0_di_tbl) / sizeof(ST_PadMuxInfo)), spi0_di_tbl},
    {(sizeof(spi0_ck_tbl) / sizeof(ST_PadMuxInfo)), spi0_ck_tbl},
    {(sizeof(spi0_cz_tbl) / sizeof(ST_PadMuxInfo)), spi0_cz_tbl},
    {(sizeof(spi0_cz1_tbl) / sizeof(ST_PadMuxInfo)), spi0_cz1_tbl},
    {(sizeof(spi1_do_tbl) / sizeof(ST_PadMuxInfo)), spi1_do_tbl},
    {(sizeof(spi1_di_tbl) / sizeof(ST_PadMuxInfo)), spi1_di_tbl},
    {(sizeof(spi1_ck_tbl) / sizeof(ST_PadMuxInfo)), spi1_ck_tbl},
    {(sizeof(spi1_cz_tbl) / sizeof(ST_PadMuxInfo)), spi1_cz_tbl},
    {(sizeof(pwm0_tbl) / sizeof(ST_PadMuxInfo)), pwm0_tbl},
    {(sizeof(pwm1_tbl) / sizeof(ST_PadMuxInfo)), pwm1_tbl},
    {(sizeof(pwm2_tbl) / sizeof(ST_PadMuxInfo)), pwm2_tbl},
    {(sizeof(pwm3_tbl) / sizeof(ST_PadMuxInfo)), pwm3_tbl},
    {(sizeof(gpio0_tbl) / sizeof(ST_PadMuxInfo)), gpio0_tbl},
    {(sizeof(gpio1_tbl) / sizeof(ST_PadMuxInfo)), gpio1_tbl},
    {(sizeof(gpio2_tbl) / sizeof(ST_PadMuxInfo)), gpio2_tbl},
    {(sizeof(gpio3_tbl) / sizeof(ST_PadMuxInfo)), gpio3_tbl},
    {(sizeof(gpio4_tbl) / sizeof(ST_PadMuxInfo)), gpio4_tbl},
    {(sizeof(gpio5_tbl) / sizeof(ST_PadMuxInfo)), gpio5_tbl},
    {(sizeof(gpio6_tbl) / sizeof(ST_PadMuxInfo)), gpio6_tbl},
    {(sizeof(gpio7_tbl) / sizeof(ST_PadMuxInfo)), gpio7_tbl},
    {(sizeof(bt1120_d0_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d0_tbl},
    {(sizeof(bt1120_d1_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d1_tbl},
    {(sizeof(bt1120_d2_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d2_tbl},
    {(sizeof(bt1120_d3_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d3_tbl},
    {(sizeof(bt1120_d4_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d4_tbl},
    {(sizeof(bt1120_d5_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d5_tbl},
    {(sizeof(bt1120_d6_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d6_tbl},
    {(sizeof(bt1120_d7_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d7_tbl},
    {(sizeof(bt1120_clk_tbl) / sizeof(ST_PadMuxInfo)), bt1120_clk_tbl},
    {(sizeof(bt1120_d8_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d8_tbl},
    {(sizeof(bt1120_d9_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d9_tbl},
    {(sizeof(bt1120_d10_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d10_tbl},
    {(sizeof(bt1120_d11_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d11_tbl},
    {(sizeof(bt1120_d12_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d12_tbl},
    {(sizeof(bt1120_d13_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d13_tbl},
    {(sizeof(bt1120_d14_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d14_tbl},
    {(sizeof(bt1120_d15_tbl) / sizeof(ST_PadMuxInfo)), bt1120_d15_tbl},
    {(sizeof(bt1120_gpio0_tbl) / sizeof(ST_PadMuxInfo)), bt1120_gpio0_tbl},
    {(sizeof(bt1120_gpio1_tbl) / sizeof(ST_PadMuxInfo)), bt1120_gpio1_tbl},
    {(sizeof(bt1120_gpio2_tbl) / sizeof(ST_PadMuxInfo)), bt1120_gpio2_tbl},
    {(sizeof(sr_gpio0_tbl) / sizeof(ST_PadMuxInfo)), sr_gpio0_tbl},
    {(sizeof(sr_gpio1_tbl) / sizeof(ST_PadMuxInfo)), sr_gpio1_tbl},
    {(sizeof(sr_gpio2_tbl) / sizeof(ST_PadMuxInfo)), sr_gpio2_tbl},
    {(sizeof(sr_gpio3_tbl) / sizeof(ST_PadMuxInfo)), sr_gpio3_tbl},
    {(sizeof(sr_gpio4_tbl) / sizeof(ST_PadMuxInfo)), sr_gpio4_tbl},
    {(sizeof(sr_gpio5_tbl) / sizeof(ST_PadMuxInfo)), sr_gpio5_tbl},
    {(sizeof(sr_gpio6_tbl) / sizeof(ST_PadMuxInfo)), sr_gpio6_tbl},
    {(sizeof(sr_gpio7_tbl) / sizeof(ST_PadMuxInfo)), sr_gpio7_tbl},
    {(sizeof(sr_gpio8_tbl) / sizeof(ST_PadMuxInfo)), sr_gpio8_tbl},
    {(sizeof(sr_gpio9_tbl) / sizeof(ST_PadMuxInfo)), sr_gpio9_tbl},
    {(sizeof(sr_i2cm_scl_tbl) / sizeof(ST_PadMuxInfo)), sr_i2cm_scl_tbl},
    {(sizeof(sr_i2cm_sda_tbl) / sizeof(ST_PadMuxInfo)), sr_i2cm_sda_tbl},
    {(sizeof(sr_rst0_tbl) / sizeof(ST_PadMuxInfo)), sr_rst0_tbl},
    {(sizeof(sr_mclk0_tbl) / sizeof(ST_PadMuxInfo)), sr_mclk0_tbl},
    {(sizeof(sr_rst1_tbl) / sizeof(ST_PadMuxInfo)), sr_rst1_tbl},
    {(sizeof(sr_mclk1_tbl) / sizeof(ST_PadMuxInfo)), sr_mclk1_tbl},
    {(sizeof(sr_rst2_tbl) / sizeof(ST_PadMuxInfo)), sr_rst2_tbl},
    {(sizeof(sr_mclk2_tbl) / sizeof(ST_PadMuxInfo)), sr_mclk2_tbl},
    {(sizeof(sr_rst3_tbl) / sizeof(ST_PadMuxInfo)), sr_rst3_tbl},
    {(sizeof(sr_mclk3_tbl) / sizeof(ST_PadMuxInfo)), sr_mclk3_tbl},
    {(sizeof(sr_rst4_tbl) / sizeof(ST_PadMuxInfo)), sr_rst4_tbl},
    {(sizeof(sr_mclk4_tbl) / sizeof(ST_PadMuxInfo)), sr_mclk4_tbl},
    {(sizeof(sr_rst5_tbl) / sizeof(ST_PadMuxInfo)), sr_rst5_tbl},
    {(sizeof(sr_mclk5_tbl) / sizeof(ST_PadMuxInfo)), sr_mclk5_tbl},
    {(sizeof(sr_rst6_tbl) / sizeof(ST_PadMuxInfo)), sr_rst6_tbl},
    {(sizeof(sr_mclk6_tbl) / sizeof(ST_PadMuxInfo)), sr_mclk6_tbl},
    {(sizeof(sr_rst7_tbl) / sizeof(ST_PadMuxInfo)), sr_rst7_tbl},
    {(sizeof(sr_mclk7_tbl) / sizeof(ST_PadMuxInfo)), sr_mclk7_tbl},
    {(sizeof(sr_gpio10_tbl) / sizeof(ST_PadMuxInfo)), sr_gpio10_tbl},
    {(sizeof(sr_gpio11_tbl) / sizeof(ST_PadMuxInfo)), sr_gpio11_tbl},
    {(sizeof(i2s0_rx_bck_tbl) / sizeof(ST_PadMuxInfo)), i2s0_rx_bck_tbl},
    {(sizeof(i2s0_rx_wck_tbl) / sizeof(ST_PadMuxInfo)), i2s0_rx_wck_tbl},
    {(sizeof(i2s0_rx_di_tbl) / sizeof(ST_PadMuxInfo)), i2s0_rx_di_tbl},
    {(sizeof(i2s1_rx_bck_tbl) / sizeof(ST_PadMuxInfo)), i2s1_rx_bck_tbl},
    {(sizeof(i2s1_rx_wck_tbl) / sizeof(ST_PadMuxInfo)), i2s1_rx_wck_tbl},
    {(sizeof(i2s1_rx_di_tbl) / sizeof(ST_PadMuxInfo)), i2s1_rx_di_tbl},
    {(sizeof(i2s2_rx_bck_tbl) / sizeof(ST_PadMuxInfo)), i2s2_rx_bck_tbl},
    {(sizeof(i2s2_rx_wck_tbl) / sizeof(ST_PadMuxInfo)), i2s2_rx_wck_tbl},
    {(sizeof(i2s2_rx_di_tbl) / sizeof(ST_PadMuxInfo)), i2s2_rx_di_tbl},
    {(sizeof(i2s3_rx_bck_tbl) / sizeof(ST_PadMuxInfo)), i2s3_rx_bck_tbl},
    {(sizeof(i2s3_rx_wck_tbl) / sizeof(ST_PadMuxInfo)), i2s3_rx_wck_tbl},
    {(sizeof(i2s3_rx_di_tbl) / sizeof(ST_PadMuxInfo)), i2s3_rx_di_tbl},
    {(sizeof(i2s0_tx_bck_tbl) / sizeof(ST_PadMuxInfo)), i2s0_tx_bck_tbl},
    {(sizeof(i2s0_tx_wck_tbl) / sizeof(ST_PadMuxInfo)), i2s0_tx_wck_tbl},
    {(sizeof(i2s0_tx_do_tbl) / sizeof(ST_PadMuxInfo)), i2s0_tx_do_tbl},
    {(sizeof(i2s1_tx_bck_tbl) / sizeof(ST_PadMuxInfo)), i2s1_tx_bck_tbl},
    {(sizeof(i2s1_tx_wck_tbl) / sizeof(ST_PadMuxInfo)), i2s1_tx_wck_tbl},
    {(sizeof(i2s1_tx_do_tbl) / sizeof(ST_PadMuxInfo)), i2s1_tx_do_tbl},
    {(sizeof(i2s_mclk0_tbl) / sizeof(ST_PadMuxInfo)), i2s_mclk0_tbl},
    {(sizeof(i2s_mclk1_tbl) / sizeof(ST_PadMuxInfo)), i2s_mclk1_tbl},
    {(sizeof(hdmitx_hpd_tbl) / sizeof(ST_PadMuxInfo)), hdmitx_hpd_tbl},
    {(sizeof(hdmitx_sda_tbl) / sizeof(ST_PadMuxInfo)), hdmitx_sda_tbl},
    {(sizeof(hdmitx_scl_tbl) / sizeof(ST_PadMuxInfo)), hdmitx_scl_tbl},
    {(sizeof(vsync_out_tbl) / sizeof(ST_PadMuxInfo)), vsync_out_tbl},
    {(sizeof(hsync_out_tbl) / sizeof(ST_PadMuxInfo)), hsync_out_tbl},
    {(sizeof(gpio8_tbl) / sizeof(ST_PadMuxInfo)), gpio8_tbl},
    {(sizeof(gpio9_tbl) / sizeof(ST_PadMuxInfo)), gpio9_tbl},
    {(sizeof(gpio10_tbl) / sizeof(ST_PadMuxInfo)), gpio10_tbl},
    {(sizeof(gpio11_tbl) / sizeof(ST_PadMuxInfo)), gpio11_tbl},
    {(sizeof(gpio12_tbl) / sizeof(ST_PadMuxInfo)), gpio12_tbl},
    {(sizeof(gpio13_tbl) / sizeof(ST_PadMuxInfo)), gpio13_tbl},
    {(sizeof(gpio14_tbl) / sizeof(ST_PadMuxInfo)), gpio14_tbl},
    {(sizeof(gpio15_tbl) / sizeof(ST_PadMuxInfo)), gpio15_tbl},
    {(sizeof(gpio16_tbl) / sizeof(ST_PadMuxInfo)), gpio16_tbl},
    {(sizeof(gpio17_tbl) / sizeof(ST_PadMuxInfo)), gpio17_tbl},
    {(sizeof(sd1_gpio0_tbl) / sizeof(ST_PadMuxInfo)), sd1_gpio0_tbl},
    {(sizeof(sd1_gpio1_tbl) / sizeof(ST_PadMuxInfo)), sd1_gpio1_tbl},
    {(sizeof(sd1_cdz_tbl) / sizeof(ST_PadMuxInfo)), sd1_cdz_tbl},
    {(sizeof(sd1_d1_tbl) / sizeof(ST_PadMuxInfo)), sd1_d1_tbl},
    {(sizeof(sd1_d0_tbl) / sizeof(ST_PadMuxInfo)), sd1_d0_tbl},
    {(sizeof(sd1_clk_tbl) / sizeof(ST_PadMuxInfo)), sd1_clk_tbl},
    {(sizeof(sd1_cmd_tbl) / sizeof(ST_PadMuxInfo)), sd1_cmd_tbl},
    {(sizeof(sd1_d3_tbl) / sizeof(ST_PadMuxInfo)), sd1_d3_tbl},
    {(sizeof(sd1_d2_tbl) / sizeof(ST_PadMuxInfo)), sd1_d2_tbl},
    {(sizeof(emmc_d0_tbl) / sizeof(ST_PadMuxInfo)), emmc_d0_tbl},
    {(sizeof(emmc_d1_tbl) / sizeof(ST_PadMuxInfo)), emmc_d1_tbl},
    {(sizeof(emmc_d2_tbl) / sizeof(ST_PadMuxInfo)), emmc_d2_tbl},
    {(sizeof(emmc_d3_tbl) / sizeof(ST_PadMuxInfo)), emmc_d3_tbl},
    {(sizeof(emmc_d4_tbl) / sizeof(ST_PadMuxInfo)), emmc_d4_tbl},
    {(sizeof(emmc_d5_tbl) / sizeof(ST_PadMuxInfo)), emmc_d5_tbl},
    {(sizeof(emmc_d6_tbl) / sizeof(ST_PadMuxInfo)), emmc_d6_tbl},
    {(sizeof(emmc_d7_tbl) / sizeof(ST_PadMuxInfo)), emmc_d7_tbl},
    {(sizeof(emmc_clk_tbl) / sizeof(ST_PadMuxInfo)), emmc_clk_tbl},
    {(sizeof(emmc_cmd_tbl) / sizeof(ST_PadMuxInfo)), emmc_cmd_tbl},
    {(sizeof(emmc_rst_tbl) / sizeof(ST_PadMuxInfo)), emmc_rst_tbl},
    {(sizeof(rgmii0_txclk_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_txclk_tbl},
    {(sizeof(rgmii0_txd0_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_txd0_tbl},
    {(sizeof(rgmii0_txd1_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_txd1_tbl},
    {(sizeof(rgmii0_txd2_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_txd2_tbl},
    {(sizeof(rgmii0_txd3_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_txd3_tbl},
    {(sizeof(rgmii0_txctl_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_txctl_tbl},
    {(sizeof(rgmii0_rxclk_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_rxclk_tbl},
    {(sizeof(rgmii0_rxd0_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_rxd0_tbl},
    {(sizeof(rgmii0_rxd1_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_rxd1_tbl},
    {(sizeof(rgmii0_rxd2_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_rxd2_tbl},
    {(sizeof(rgmii0_rxd3_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_rxd3_tbl},
    {(sizeof(rgmii0_rxctl_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_rxctl_tbl},
    {(sizeof(rgmii0_io0_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_io0_tbl},
    {(sizeof(rgmii0_io1_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_io1_tbl},
    {(sizeof(rgmii0_mdc_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_mdc_tbl},
    {(sizeof(rgmii0_mdio_tbl) / sizeof(ST_PadMuxInfo)), rgmii0_mdio_tbl},
    {(sizeof(rgmii1_txclk_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_txclk_tbl},
    {(sizeof(rgmii1_txd0_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_txd0_tbl},
    {(sizeof(rgmii1_txd1_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_txd1_tbl},
    {(sizeof(rgmii1_txd2_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_txd2_tbl},
    {(sizeof(rgmii1_txd3_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_txd3_tbl},
    {(sizeof(rgmii1_txctl_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_txctl_tbl},
    {(sizeof(rgmii1_rxclk_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_rxclk_tbl},
    {(sizeof(rgmii1_rxd0_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_rxd0_tbl},
    {(sizeof(rgmii1_rxd1_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_rxd1_tbl},
    {(sizeof(rgmii1_rxd2_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_rxd2_tbl},
    {(sizeof(rgmii1_rxd3_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_rxd3_tbl},
    {(sizeof(rgmii1_rxctl_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_rxctl_tbl},
    {(sizeof(rgmii1_io0_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_io0_tbl},
    {(sizeof(rgmii1_io1_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_io1_tbl},
    {(sizeof(rgmii1_mdc_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_mdc_tbl},
    {(sizeof(rgmii1_mdio_tbl) / sizeof(ST_PadMuxInfo)), rgmii1_mdio_tbl},
    {(sizeof(sar_gpio0_tbl) / sizeof(ST_PadMuxInfo)), sar_gpio0_tbl},
    {(sizeof(sar_gpio1_tbl) / sizeof(ST_PadMuxInfo)), sar_gpio1_tbl},
    {(sizeof(sar_gpio2_tbl) / sizeof(ST_PadMuxInfo)), sar_gpio2_tbl},
    {(sizeof(sar_gpio3_tbl) / sizeof(ST_PadMuxInfo)), sar_gpio3_tbl},
};

static const ST_PadModeInfo m_stPadModeInfoTbl[] = {
    {"GPIO_MODE", 0, 0, 0},
    {"EJ_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_EJ_MODE), REG_EJ_MODE_MASK, BIT7},
    {"EJ_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_EJ_MODE), REG_EJ_MODE_MASK, BIT8},
    {"EJ_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_EJ_MODE), REG_EJ_MODE_MASK, BIT8 | BIT7},
    {"EJ_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_EJ_MODE), REG_EJ_MODE_MASK, BIT9},
    {"EJ_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_EJ_MODE), REG_EJ_MODE_MASK, BIT9 | BIT7},
    {"EJ_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_EJ_MODE), REG_EJ_MODE_MASK, BIT9 | BIT8},
    {"DLA_EJ_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_DLA_EJ_MODE), REG_DLA_EJ_MODE_MASK, BIT0},
    {"DLA_EJ_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_DLA_EJ_MODE), REG_DLA_EJ_MODE_MASK, BIT1},
    {"DLA_EJ_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_DLA_EJ_MODE), REG_DLA_EJ_MODE_MASK, BIT1 | BIT0},
    {"DLA_EJ_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_DLA_EJ_MODE), REG_DLA_EJ_MODE_MASK, BIT2},
    {"DLA_EJ_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_DLA_EJ_MODE), REG_DLA_EJ_MODE_MASK, BIT2 | BIT0},
    {"DLA_EJ_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_DLA_EJ_MODE), REG_DLA_EJ_MODE_MASK, BIT2 | BIT1},
    {"CA7_EJ_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_CA7_EJ_MODE), REG_CA7_EJ_MODE_MASK, BIT0},
    {"CA7_EJ_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_CA7_EJ_MODE), REG_CA7_EJ_MODE_MASK, BIT1},
    {"CA7_EJ_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_CA7_EJ_MODE), REG_CA7_EJ_MODE_MASK, BIT1 | BIT0},
    {"CA7_EJ_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_CA7_EJ_MODE), REG_CA7_EJ_MODE_MASK, BIT2},
    {"CA7_EJ_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_CA7_EJ_MODE), REG_CA7_EJ_MODE_MASK, BIT2 | BIT0},
    {"CA7_EJ_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_CA7_EJ_MODE), REG_CA7_EJ_MODE_MASK, BIT2 | BIT1},
    {"DSP_EJ_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_DSP_EJ_MODE), REG_DSP_EJ_MODE_MASK, BIT4},
    {"DSP_EJ_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_DSP_EJ_MODE), REG_DSP_EJ_MODE_MASK, BIT5},
    {"DSP_EJ_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_DSP_EJ_MODE), REG_DSP_EJ_MODE_MASK, BIT5 | BIT4},
    {"DSP_EJ_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_DSP_EJ_MODE), REG_DSP_EJ_MODE_MASK, BIT6},
    {"DSP_EJ_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_DSP_EJ_MODE), REG_DSP_EJ_MODE_MASK, BIT6 | BIT4},
    {"DSP_EJ_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_DSP_EJ_MODE), REG_DSP_EJ_MODE_MASK, BIT6 | BIT5},
    {"TEST_IN_MODE_1", _RIUA_16BIT(CHIPTOP_BANK, REG_TEST_IN_MODE), REG_TEST_IN_MODE_MASK, BIT0},
    {"TEST_IN_MODE_2", _RIUA_16BIT(CHIPTOP_BANK, REG_TEST_IN_MODE), REG_TEST_IN_MODE_MASK, BIT1},
    {"TEST_IN_MODE_3", _RIUA_16BIT(CHIPTOP_BANK, REG_TEST_IN_MODE), REG_TEST_IN_MODE_MASK, BIT1 | BIT0},
    {"TEST_OUT_MODE_1", _RIUA_16BIT(CHIPTOP_BANK, REG_TEST_OUT_MODE), REG_TEST_OUT_MODE_MASK, BIT4},
    {"TEST_OUT_MODE_2", _RIUA_16BIT(CHIPTOP_BANK, REG_TEST_OUT_MODE), REG_TEST_OUT_MODE_MASK, BIT5},
    {"TEST_OUT_MODE_3", _RIUA_16BIT(CHIPTOP_BANK, REG_TEST_OUT_MODE), REG_TEST_OUT_MODE_MASK, BIT5 | BIT4},
    {"SPI_GPIO_0", _RIUA_16BIT(PMSLEEP_BANK, REG_SPI_GPIO), REG_SPI_GPIO_MASK, 0},
    {"SPIWPN_GPIO_0", _RIUA_16BIT(PMSLEEP_BANK, REG_SPIWPN_GPIO), REG_SPIWPN_GPIO_MASK, 0},
    {"SPICSZ1_GPIO_0", _RIUA_16BIT(PMSLEEP_BANK, REG_SPICSZ1_GPIO), REG_SPICSZ1_GPIO_MASK, 0},
    {"SPICSZ2_MODE_1", _RIUA_16BIT(PM_PADTOP_BANK, REG_SPICSZ2_MODE), REG_SPICSZ2_MODE_MASK, BIT0},
    {"SPICSZ2_MODE_2", _RIUA_16BIT(PM_PADTOP_BANK, REG_SPICSZ2_MODE), REG_SPICSZ2_MODE_MASK, BIT1},
    {"SPICSZ2_MODE_3", _RIUA_16BIT(PM_PADTOP_BANK, REG_SPICSZ2_MODE), REG_SPICSZ2_MODE_MASK, BIT1 | BIT0},
    {"SPIHOLDN_MODE_0", _RIUA_16BIT(PM_PADTOP_BANK, REG_SPIHOLDN_MODE), REG_SPIHOLDN_MODE_MASK, 0},
    {"UART_IS_GPIO_0", _RIUA_16BIT(PADTOP_BANK, REG_UART_IS_GPIO), REG_UART_IS_GPIO_MASK, 0},
    {"IR_IN_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_IR_IN_MODE), REG_IR_IN_MODE_MASK, BIT4},
    {"I2C0_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2C0_MODE), REG_I2C0_MODE_MASK, BIT0},
    {"I2C0_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2C0_MODE), REG_I2C0_MODE_MASK, BIT1},
    {"I2C0_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2C0_MODE), REG_I2C0_MODE_MASK, BIT1 | BIT0},
    {"I2C0_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2C0_MODE), REG_I2C0_MODE_MASK, BIT2},
    {"I2C0_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2C0_MODE), REG_I2C0_MODE_MASK, BIT2 | BIT0},
    {"I2C0_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_I2C0_MODE), REG_I2C0_MODE_MASK, BIT2 | BIT1},
    {"I2C1_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT0},
    {"I2C1_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT1},
    {"I2C1_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT1 | BIT0},
    {"I2C1_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT2},
    {"I2C1_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT2 | BIT0},
    {"I2C1_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT2 | BIT1},
    {"I2C1_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT2 | BIT1 | BIT0},
    {"I2C1_MODE_8", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT3},
    {"I2C1_MODE_9", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT3 | BIT0},
    {"I2C1_MODE_10", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT3 | BIT1},
    {"I2C1_MODE_11", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT3 | BIT1 | BIT0},
    {"I2C1_MODE_12", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT3 | BIT2},
    {"I2C1_MODE_13", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT0},
    {"I2C1_MODE_14", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1},
    {"I2C1_MODE_15", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT3 | BIT2 | BIT1 | BIT0},
    {"I2C1_MODE_16", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT4},
    {"I2C1_MODE_17", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT4 | BIT0},
    {"I2C1_MODE_18", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT4 | BIT1},
    {"I2C1_MODE_19", _RIUA_16BIT(PADTOP_BANK, REG_I2C1_MODE), REG_I2C1_MODE_MASK, BIT4 | BIT1 | BIT0},
    {"I2C2_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT8},
    {"I2C2_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT9},
    {"I2C2_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT9 | BIT8},
    {"I2C2_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT10},
    {"I2C2_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT10 | BIT8},
    {"I2C2_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT10 | BIT9},
    {"I2C2_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT10 | BIT9 | BIT8},
    {"I2C2_MODE_8", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT11},
    {"I2C2_MODE_9", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT11 | BIT8},
    {"I2C2_MODE_10", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT11 | BIT9},
    {"I2C2_MODE_11", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT11 | BIT9 | BIT8},
    {"I2C2_MODE_12", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT11 | BIT10},
    {"I2C2_MODE_13", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT8},
    {"I2C2_MODE_14", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9},
    {"I2C2_MODE_15", _RIUA_16BIT(PADTOP_BANK, REG_I2C2_MODE), REG_I2C2_MODE_MASK, BIT11 | BIT10 | BIT9 | BIT8},
    {"I2C3_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2C3_MODE), REG_I2C3_MODE_MASK, BIT0},
    {"I2C3_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2C3_MODE), REG_I2C3_MODE_MASK, BIT1},
    {"I2C3_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2C3_MODE), REG_I2C3_MODE_MASK, BIT1 | BIT0},
    {"I2C3_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2C3_MODE), REG_I2C3_MODE_MASK, BIT2},
    {"I2C3_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2C3_MODE), REG_I2C3_MODE_MASK, BIT2 | BIT0},
    {"I2C3_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_I2C3_MODE), REG_I2C3_MODE_MASK, BIT2 | BIT1},
    {"I2C4_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2C4_MODE), REG_I2C4_MODE_MASK, BIT4},
    {"I2C4_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2C4_MODE), REG_I2C4_MODE_MASK, BIT5},
    {"I2C4_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2C4_MODE), REG_I2C4_MODE_MASK, BIT5 | BIT4},
    {"I2C4_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2C4_MODE), REG_I2C4_MODE_MASK, BIT6},
    {"I2C4_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2C4_MODE), REG_I2C4_MODE_MASK, BIT6 | BIT4},
    {"I2C5_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2C5_MODE), REG_I2C5_MODE_MASK, BIT8},
    {"I2C5_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2C5_MODE), REG_I2C5_MODE_MASK, BIT9},
    {"I2C5_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2C5_MODE), REG_I2C5_MODE_MASK, BIT9 | BIT8},
    {"I2C5_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2C5_MODE), REG_I2C5_MODE_MASK, BIT10},
    {"I2C5_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2C5_MODE), REG_I2C5_MODE_MASK, BIT10 | BIT8},
    {"I2C6_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2C6_MODE), REG_I2C6_MODE_MASK, BIT0},
    {"I2C6_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2C6_MODE), REG_I2C6_MODE_MASK, BIT1},
    {"I2C6_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2C6_MODE), REG_I2C6_MODE_MASK, BIT1 | BIT0},
    {"I2C6_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2C6_MODE), REG_I2C6_MODE_MASK, BIT2},
    {"I2C6_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2C6_MODE), REG_I2C6_MODE_MASK, BIT2 | BIT0},
    {"I2C7_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2C7_MODE), REG_I2C7_MODE_MASK, BIT4},
    {"I2C7_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2C7_MODE), REG_I2C7_MODE_MASK, BIT5},
    {"I2C7_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2C7_MODE), REG_I2C7_MODE_MASK, BIT5 | BIT4},
    {"I2C7_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2C7_MODE), REG_I2C7_MODE_MASK, BIT6},
    {"I2C7_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2C7_MODE), REG_I2C7_MODE_MASK, BIT6 | BIT4},
    {"I2C7_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_I2C7_MODE), REG_I2C7_MODE_MASK, BIT6 | BIT5},
    {"I2C8_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2C8_MODE), REG_I2C8_MODE_MASK, BIT8},
    {"I2C8_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2C8_MODE), REG_I2C8_MODE_MASK, BIT9},
    {"I2C8_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2C8_MODE), REG_I2C8_MODE_MASK, BIT9 | BIT8},
    {"I2C8_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2C8_MODE), REG_I2C8_MODE_MASK, BIT10},
    {"I2C8_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2C8_MODE), REG_I2C8_MODE_MASK, BIT10 | BIT8},
    {"I2C8_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_I2C8_MODE), REG_I2C8_MODE_MASK, BIT10 | BIT9},
    {"I2C9_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2C9_MODE), REG_I2C9_MODE_MASK, BIT12},
    {"I2C9_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2C9_MODE), REG_I2C9_MODE_MASK, BIT13},
    {"I2C9_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2C9_MODE), REG_I2C9_MODE_MASK, BIT13 | BIT12},
    {"I2C9_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2C9_MODE), REG_I2C9_MODE_MASK, BIT14},
    {"I2C9_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2C9_MODE), REG_I2C9_MODE_MASK, BIT14 | BIT12},
    {"I2C10_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2C10_MODE), REG_I2C10_MODE_MASK, BIT0},
    {"I2C10_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2C10_MODE), REG_I2C10_MODE_MASK, BIT1},
    {"I2C10_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2C10_MODE), REG_I2C10_MODE_MASK, BIT1 | BIT0},
    {"I2C10_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2C10_MODE), REG_I2C10_MODE_MASK, BIT2},
    {"I2C10_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2C10_MODE), REG_I2C10_MODE_MASK, BIT2 | BIT0},
    {"SPI0_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SPI0_MODE), REG_SPI0_MODE_MASK, BIT0},
    {"SPI0_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SPI0_MODE), REG_SPI0_MODE_MASK, BIT1},
    {"SPI0_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SPI0_MODE), REG_SPI0_MODE_MASK, BIT1 | BIT0},
    {"SPI0_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_SPI0_MODE), REG_SPI0_MODE_MASK, BIT2},
    {"SPI0_CZ2_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SPI0_CZ2_MODE), REG_SPI0_CZ2_MODE_MASK, BIT0},
    {"SPI0_CZ2_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SPI0_CZ2_MODE), REG_SPI0_CZ2_MODE_MASK, BIT1},
    {"SPI0_CZ2_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SPI0_CZ2_MODE), REG_SPI0_CZ2_MODE_MASK, BIT1 | BIT0},
    {"SPI1_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SPI1_MODE), REG_SPI1_MODE_MASK, BIT4},
    {"SPI1_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SPI1_MODE), REG_SPI1_MODE_MASK, BIT5},
    {"SPI1_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SPI1_MODE), REG_SPI1_MODE_MASK, BIT5 | BIT4},
    {"SPI2_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SPI2_MODE), REG_SPI2_MODE_MASK, BIT8},
    {"SPI2_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SPI2_MODE), REG_SPI2_MODE_MASK, BIT9},
    {"SPI2_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SPI2_MODE), REG_SPI2_MODE_MASK, BIT9 | BIT8},
    {"SPI3_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SPI3_MODE), REG_SPI3_MODE_MASK, BIT0},
    {"SPI3_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SPI3_MODE), REG_SPI3_MODE_MASK, BIT1},
    {"SPI3_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SPI3_MODE), REG_SPI3_MODE_MASK, BIT1 | BIT0},
    {"SPI3_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_SPI3_MODE), REG_SPI3_MODE_MASK, BIT2},
    {"FUART_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_FUART_MODE), REG_FUART_MODE_MASK, BIT8},
    {"FUART_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_FUART_MODE), REG_FUART_MODE_MASK, BIT9},
    {"FUART_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_FUART_MODE), REG_FUART_MODE_MASK, BIT9 | BIT8},
    {"FUART_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_FUART_MODE), REG_FUART_MODE_MASK, BIT10},
    {"FUART_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_FUART_MODE), REG_FUART_MODE_MASK, BIT10 | BIT8},
    {"FUART_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_FUART_MODE), REG_FUART_MODE_MASK, BIT10 | BIT9},
    {"FUART_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_FUART_MODE), REG_FUART_MODE_MASK, BIT10 | BIT9 | BIT8},
    {"FUART_MODE_8", _RIUA_16BIT(PADTOP_BANK, REG_FUART_MODE), REG_FUART_MODE_MASK, BIT11},
    {"FUART_MODE_9", _RIUA_16BIT(PADTOP_BANK, REG_FUART_MODE), REG_FUART_MODE_MASK, BIT11 | BIT8},
    {"FUART_2W_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_FUART_2W_MODE), REG_FUART_2W_MODE_MASK, BIT12},
    {"FUART_2W_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_FUART_2W_MODE), REG_FUART_2W_MODE_MASK, BIT13},
    {"FUART_2W_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_FUART_2W_MODE), REG_FUART_2W_MODE_MASK, BIT13 | BIT12},
    {"FUART_2W_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_FUART_2W_MODE), REG_FUART_2W_MODE_MASK, BIT14},
    {"FUART_2W_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_FUART_2W_MODE), REG_FUART_2W_MODE_MASK, BIT14 | BIT12},
    {"FUART_2W_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_FUART_2W_MODE), REG_FUART_2W_MODE_MASK, BIT14 | BIT13},
    {"FUART_2W_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_FUART_2W_MODE), REG_FUART_2W_MODE_MASK, BIT14 | BIT13 | BIT12},
    {"FUART_2W_MODE_8", _RIUA_16BIT(PADTOP_BANK, REG_FUART_2W_MODE), REG_FUART_2W_MODE_MASK, BIT15},
    {"FUART_2W_MODE_9", _RIUA_16BIT(PADTOP_BANK, REG_FUART_2W_MODE), REG_FUART_2W_MODE_MASK, BIT15 | BIT12},
    {"UART0_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_UART0_MODE), REG_UART0_MODE_MASK, BIT0},
    {"UART0_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_UART0_MODE), REG_UART0_MODE_MASK, BIT1},
    {"UART0_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_UART0_MODE), REG_UART0_MODE_MASK, BIT1 | BIT0},
    {"UART1_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_UART1_MODE), REG_UART1_MODE_MASK, BIT4},
    {"UART1_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_UART1_MODE), REG_UART1_MODE_MASK, BIT5},
    {"UART1_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_UART1_MODE), REG_UART1_MODE_MASK, BIT5 | BIT4},
    {"UART1_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_UART1_MODE), REG_UART1_MODE_MASK, BIT6},
    {"UART1_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_UART1_MODE), REG_UART1_MODE_MASK, BIT6 | BIT4},
    {"UART2_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_UART2_MODE), REG_UART2_MODE_MASK, BIT8},
    {"UART2_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_UART2_MODE), REG_UART2_MODE_MASK, BIT9},
    {"UART2_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_UART2_MODE), REG_UART2_MODE_MASK, BIT9 | BIT8},
    {"UART2_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_UART2_MODE), REG_UART2_MODE_MASK, BIT10},
    {"UART2_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_UART2_MODE), REG_UART2_MODE_MASK, BIT10 | BIT8},
    {"UART2_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_UART2_MODE), REG_UART2_MODE_MASK, BIT10 | BIT9},
    {"UART3_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_UART3_MODE), REG_UART3_MODE_MASK, BIT12},
    {"UART3_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_UART3_MODE), REG_UART3_MODE_MASK, BIT13},
    {"UART3_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_UART3_MODE), REG_UART3_MODE_MASK, BIT13 | BIT12},
    {"UART3_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_UART3_MODE), REG_UART3_MODE_MASK, BIT14},
    {"UART3_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_UART3_MODE), REG_UART3_MODE_MASK, BIT14 | BIT12},
    {"UART3_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_UART3_MODE), REG_UART3_MODE_MASK, BIT14 | BIT13},
    {"UART4_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_UART4_MODE), REG_UART4_MODE_MASK, BIT0},
    {"UART4_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_UART4_MODE), REG_UART4_MODE_MASK, BIT1},
    {"UART4_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_UART4_MODE), REG_UART4_MODE_MASK, BIT1 | BIT0},
    {"UART4_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_UART4_MODE), REG_UART4_MODE_MASK, BIT2},
    {"UART4_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_UART4_MODE), REG_UART4_MODE_MASK, BIT2 | BIT0},
    {"UART5_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_UART5_MODE), REG_UART5_MODE_MASK, BIT4},
    {"UART5_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_UART5_MODE), REG_UART5_MODE_MASK, BIT5},
    {"UART5_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_UART5_MODE), REG_UART5_MODE_MASK, BIT5 | BIT4},
    {"UART5_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_UART5_MODE), REG_UART5_MODE_MASK, BIT6},
    {"UART5_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_UART5_MODE), REG_UART5_MODE_MASK, BIT6 | BIT4},
    {"SD0_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SD0_MODE), REG_SD0_MODE_MASK, BIT8},
    {"SD0_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SD0_MODE), REG_SD0_MODE_MASK, BIT9},
    {"SD0_CDZ_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SD0_CDZ_MODE), REG_SD0_CDZ_MODE_MASK, BIT10},
    {"SD0_CDZ_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SD0_CDZ_MODE), REG_SD0_CDZ_MODE_MASK, BIT11},
    {"SD1_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SD1_MODE), REG_SD1_MODE_MASK, BIT12},
    {"SD1_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SD1_MODE), REG_SD1_MODE_MASK, BIT13},
    {"SD1_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SD1_MODE), REG_SD1_MODE_MASK, BIT13 | BIT12},
    {"SD1_CDZ_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SD1_CDZ_MODE), REG_SD1_CDZ_MODE_MASK, BIT12},
    {"SD1_CDZ_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SD1_CDZ_MODE), REG_SD1_CDZ_MODE_MASK, BIT13},
    {"SD1_CDZ_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SD1_CDZ_MODE), REG_SD1_CDZ_MODE_MASK, BIT13 | BIT12},
    {"SD2_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SD2_MODE), REG_SD2_MODE_MASK, BIT0},
    {"SD2_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SD2_MODE), REG_SD2_MODE_MASK, BIT1},
    {"SD2_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SD2_MODE), REG_SD2_MODE_MASK, BIT1 | BIT0},
    {"SD2_CDZ_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SD2_CDZ_MODE), REG_SD2_CDZ_MODE_MASK, BIT4},
    {"SD2_CDZ_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SD2_CDZ_MODE), REG_SD2_CDZ_MODE_MASK, BIT5},
    {"SD2_CDZ_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SD2_CDZ_MODE), REG_SD2_CDZ_MODE_MASK, BIT5 | BIT4},
    {"EMMC_8B_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_EMMC_8B_MODE), REG_EMMC_8B_MODE_MASK, BIT2},
    {"EMMC_8B_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_EMMC_8B_MODE), REG_EMMC_8B_MODE_MASK, BIT3},
    {"EMMC_4B_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_EMMC_4B_MODE), REG_EMMC_4B_MODE_MASK, BIT0},
    {"EMMC_4B_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_EMMC_4B_MODE), REG_EMMC_4B_MODE_MASK, BIT1},
    {"EMMC_4B_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_EMMC_4B_MODE), REG_EMMC_4B_MODE_MASK, BIT1 | BIT0},
    {"EMMC_RST_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_EMMC_RST_MODE), REG_EMMC_RST_MODE_MASK, BIT4},
    {"EMMC_RST_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_EMMC_RST_MODE), REG_EMMC_RST_MODE_MASK, BIT5},
    {"EMMC_RST_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_EMMC_RST_MODE), REG_EMMC_RST_MODE_MASK, BIT5 | BIT4},
    {"PWM0_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_PWM0_MODE), REG_PWM0_MODE_MASK, BIT0},
    {"PWM0_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_PWM0_MODE), REG_PWM0_MODE_MASK, BIT1},
    {"PWM0_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_PWM0_MODE), REG_PWM0_MODE_MASK, BIT1 | BIT0},
    {"PWM0_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_PWM0_MODE), REG_PWM0_MODE_MASK, BIT2},
    {"PWM0_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_PWM0_MODE), REG_PWM0_MODE_MASK, BIT2 | BIT0},
    {"PWM1_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_PWM1_MODE), REG_PWM1_MODE_MASK, BIT4},
    {"PWM1_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_PWM1_MODE), REG_PWM1_MODE_MASK, BIT5},
    {"PWM1_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_PWM1_MODE), REG_PWM1_MODE_MASK, BIT5 | BIT4},
    {"PWM1_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_PWM1_MODE), REG_PWM1_MODE_MASK, BIT6},
    {"PWM1_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_PWM1_MODE), REG_PWM1_MODE_MASK, BIT6 | BIT4},
    {"PWM2_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_PWM2_MODE), REG_PWM2_MODE_MASK, BIT8},
    {"PWM2_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_PWM2_MODE), REG_PWM2_MODE_MASK, BIT9},
    {"PWM2_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_PWM2_MODE), REG_PWM2_MODE_MASK, BIT9 | BIT8},
    {"PWM2_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_PWM2_MODE), REG_PWM2_MODE_MASK, BIT10},
    {"PWM2_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_PWM2_MODE), REG_PWM2_MODE_MASK, BIT10 | BIT8},
    {"PWM3_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_PWM3_MODE), REG_PWM3_MODE_MASK, BIT12},
    {"PWM3_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_PWM3_MODE), REG_PWM3_MODE_MASK, BIT13},
    {"PWM3_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_PWM3_MODE), REG_PWM3_MODE_MASK, BIT13 | BIT12},
    {"PWM3_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_PWM3_MODE), REG_PWM3_MODE_MASK, BIT14},
    {"PWM3_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_PWM3_MODE), REG_PWM3_MODE_MASK, BIT14 | BIT12},
    {"PWM4_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_PWM4_MODE), REG_PWM4_MODE_MASK, BIT0},
    {"PWM4_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_PWM4_MODE), REG_PWM4_MODE_MASK, BIT1},
    {"PWM4_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_PWM4_MODE), REG_PWM4_MODE_MASK, BIT1 | BIT0},
    {"PWM4_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_PWM4_MODE), REG_PWM4_MODE_MASK, BIT2},
    {"PWM4_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_PWM4_MODE), REG_PWM4_MODE_MASK, BIT2 | BIT0},
    {"PWM4_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_PWM4_MODE), REG_PWM4_MODE_MASK, BIT2 | BIT1},
    {"PWM5_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_PWM5_MODE), REG_PWM5_MODE_MASK, BIT4},
    {"PWM5_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_PWM5_MODE), REG_PWM5_MODE_MASK, BIT5},
    {"PWM5_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_PWM5_MODE), REG_PWM5_MODE_MASK, BIT5 | BIT4},
    {"PWM5_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_PWM5_MODE), REG_PWM5_MODE_MASK, BIT6},
    {"PWM5_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_PWM5_MODE), REG_PWM5_MODE_MASK, BIT6 | BIT4},
    {"PWM5_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_PWM5_MODE), REG_PWM5_MODE_MASK, BIT6 | BIT5},
    {"PWM6_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_PWM6_MODE), REG_PWM6_MODE_MASK, BIT8},
    {"PWM6_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_PWM6_MODE), REG_PWM6_MODE_MASK, BIT9},
    {"PWM6_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_PWM6_MODE), REG_PWM6_MODE_MASK, BIT9 | BIT8},
    {"PWM6_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_PWM6_MODE), REG_PWM6_MODE_MASK, BIT10},
    {"PWM6_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_PWM6_MODE), REG_PWM6_MODE_MASK, BIT10 | BIT8},
    {"PWM6_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_PWM6_MODE), REG_PWM6_MODE_MASK, BIT10 | BIT9},
    {"PWM6_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_PWM6_MODE), REG_PWM6_MODE_MASK, BIT10 | BIT9 | BIT8},
    {"PWM7_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_PWM7_MODE), REG_PWM7_MODE_MASK, BIT12},
    {"PWM7_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_PWM7_MODE), REG_PWM7_MODE_MASK, BIT13},
    {"PWM7_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_PWM7_MODE), REG_PWM7_MODE_MASK, BIT13 | BIT12},
    {"PWM7_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_PWM7_MODE), REG_PWM7_MODE_MASK, BIT14},
    {"PWM7_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_PWM7_MODE), REG_PWM7_MODE_MASK, BIT14 | BIT12},
    {"PWM7_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_PWM7_MODE), REG_PWM7_MODE_MASK, BIT14 | BIT13},
    {"PWM7_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_PWM7_MODE), REG_PWM7_MODE_MASK, BIT14 | BIT13 | BIT12},
    {"PWM8_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_PWM8_MODE), REG_PWM8_MODE_MASK, BIT0},
    {"PWM8_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_PWM8_MODE), REG_PWM8_MODE_MASK, BIT1},
    {"PWM8_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_PWM8_MODE), REG_PWM8_MODE_MASK, BIT1 | BIT0},
    {"PWM8_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_PWM8_MODE), REG_PWM8_MODE_MASK, BIT2},
    {"PWM8_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_PWM8_MODE), REG_PWM8_MODE_MASK, BIT2 | BIT0},
    {"PWM8_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_PWM8_MODE), REG_PWM8_MODE_MASK, BIT2 | BIT1},
    {"PWM8_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_PWM8_MODE), REG_PWM8_MODE_MASK, BIT2 | BIT1 | BIT0},
    {"PWM9_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_PWM9_MODE), REG_PWM9_MODE_MASK, BIT4},
    {"PWM9_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_PWM9_MODE), REG_PWM9_MODE_MASK, BIT5},
    {"PWM9_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_PWM9_MODE), REG_PWM9_MODE_MASK, BIT5 | BIT4},
    {"PWM9_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_PWM9_MODE), REG_PWM9_MODE_MASK, BIT6},
    {"PWM9_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_PWM9_MODE), REG_PWM9_MODE_MASK, BIT6 | BIT4},
    {"PWM9_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_PWM9_MODE), REG_PWM9_MODE_MASK, BIT6 | BIT5},
    {"PWM9_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_PWM9_MODE), REG_PWM9_MODE_MASK, BIT6 | BIT5 | BIT4},
    {"PWM10_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_PWM10_MODE), REG_PWM10_MODE_MASK, BIT0},
    {"PWM10_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_PWM10_MODE), REG_PWM10_MODE_MASK, BIT1},
    {"PWM10_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_PWM10_MODE), REG_PWM10_MODE_MASK, BIT1 | BIT0},
    {"PWM10_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_PWM10_MODE), REG_PWM10_MODE_MASK, BIT2},
    {"PWM10_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_PWM10_MODE), REG_PWM10_MODE_MASK, BIT2 | BIT0},
    {"PWM10_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_PWM10_MODE), REG_PWM10_MODE_MASK, BIT2 | BIT1},
    {"PWM11_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_PWM11_MODE), REG_PWM11_MODE_MASK, BIT8},
    {"PWM11_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_PWM11_MODE), REG_PWM11_MODE_MASK, BIT9},
    {"PWM11_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_PWM11_MODE), REG_PWM11_MODE_MASK, BIT9 | BIT8},
    {"PWM11_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_PWM11_MODE), REG_PWM11_MODE_MASK, BIT10},
    {"PWM11_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_PWM11_MODE), REG_PWM11_MODE_MASK, BIT10 | BIT8},
    {"PWM11_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_PWM11_MODE), REG_PWM11_MODE_MASK, BIT10 | BIT9},
    {"PWM11_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_PWM11_MODE), REG_PWM11_MODE_MASK, BIT10 | BIT9 | BIT8},
    {"I2S0_MCK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_MCK_MODE), REG_I2S0_MCK_MODE_MASK, BIT0},
    {"I2S0_MCK_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_MCK_MODE), REG_I2S0_MCK_MODE_MASK, BIT1},
    {"I2S0_MCK_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_MCK_MODE), REG_I2S0_MCK_MODE_MASK, BIT1 | BIT0},
    {"I2S0_MCK_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_MCK_MODE), REG_I2S0_MCK_MODE_MASK, BIT2},
    {"I2S0_MCK_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_MCK_MODE), REG_I2S0_MCK_MODE_MASK, BIT2 | BIT0},
    {"I2S0_MCK_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_MCK_MODE), REG_I2S0_MCK_MODE_MASK, BIT2 | BIT1},
    {"I2S0_MCK_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_MCK_MODE), REG_I2S0_MCK_MODE_MASK, BIT2 | BIT1 | BIT0},
    {"I2S0_MCK_MODE_8", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_MCK_MODE), REG_I2S0_MCK_MODE_MASK, BIT3},
    {"I2S0_MCK_MODE_9", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_MCK_MODE), REG_I2S0_MCK_MODE_MASK, BIT3 | BIT0},
    {"I2S0_MCK_MODE_10", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_MCK_MODE), REG_I2S0_MCK_MODE_MASK, BIT3 | BIT1},
    {"I2S0_MCK_MODE_11", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_MCK_MODE), REG_I2S0_MCK_MODE_MASK, BIT3 | BIT1 | BIT0},
    {"I2S0_MCK_MODE_12", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_MCK_MODE), REG_I2S0_MCK_MODE_MASK, BIT3 | BIT2},
    {"I2S1_MCK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_MCK_MODE), REG_I2S1_MCK_MODE_MASK, BIT4},
    {"I2S1_MCK_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_MCK_MODE), REG_I2S1_MCK_MODE_MASK, BIT5},
    {"I2S1_MCK_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_MCK_MODE), REG_I2S1_MCK_MODE_MASK, BIT5 | BIT4},
    {"I2S1_MCK_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_MCK_MODE), REG_I2S1_MCK_MODE_MASK, BIT6},
    {"I2S1_MCK_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_MCK_MODE), REG_I2S1_MCK_MODE_MASK, BIT6 | BIT4},
    {"I2S1_MCK_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_MCK_MODE), REG_I2S1_MCK_MODE_MASK, BIT6 | BIT5},
    {"I2S1_MCK_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_MCK_MODE), REG_I2S1_MCK_MODE_MASK, BIT6 | BIT5 | BIT4},
    {"I2S1_MCK_MODE_8", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_MCK_MODE), REG_I2S1_MCK_MODE_MASK, BIT7},
    {"I2S1_MCK_MODE_9", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_MCK_MODE), REG_I2S1_MCK_MODE_MASK, BIT7 | BIT4},
    {"I2S1_MCK_MODE_10", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_MCK_MODE), REG_I2S1_MCK_MODE_MASK, BIT7 | BIT5},
    {"I2S0_RX_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_RX_MODE), REG_I2S0_RX_MODE_MASK, BIT8},
    {"I2S0_RX_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_RX_MODE), REG_I2S0_RX_MODE_MASK, BIT9},
    {"I2S0_RX_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_RX_MODE), REG_I2S0_RX_MODE_MASK, BIT9 | BIT8},
    {"I2S1_RX_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_RX_MODE), REG_I2S1_RX_MODE_MASK, BIT0},
    {"I2S1_RX_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_RX_MODE), REG_I2S1_RX_MODE_MASK, BIT1},
    {"I2S1_RX_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_RX_MODE), REG_I2S1_RX_MODE_MASK, BIT1 | BIT0},
    {"I2S2_RX_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2S2_RX_MODE), REG_I2S2_RX_MODE_MASK, BIT8},
    {"I2S2_RX_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2S2_RX_MODE), REG_I2S2_RX_MODE_MASK, BIT9},
    {"I2S3_RX_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2S3_RX_MODE), REG_I2S3_RX_MODE_MASK, BIT12},
    {"I2S3_RX_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2S3_RX_MODE), REG_I2S3_RX_MODE_MASK, BIT13},
    {"I2S0_TX_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_TX_MODE), REG_I2S0_TX_MODE_MASK, BIT12},
    {"I2S0_TX_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_TX_MODE), REG_I2S0_TX_MODE_MASK, BIT13},
    {"I2S0_TX_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_TX_MODE), REG_I2S0_TX_MODE_MASK, BIT13 | BIT12},
    {"I2S0_TX_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_TX_MODE), REG_I2S0_TX_MODE_MASK, BIT14},
    {"I2S1_TX_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_TX_MODE), REG_I2S1_TX_MODE_MASK, BIT4},
    {"I2S1_TX_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_TX_MODE), REG_I2S1_TX_MODE_MASK, BIT5},
    {"I2S1_TX_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_TX_MODE), REG_I2S1_TX_MODE_MASK, BIT5 | BIT4},
    {"I2S0_RXTX_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_RXTX_MODE), REG_I2S0_RXTX_MODE_MASK, BIT0},
    {"I2S0_RXTX_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_RXTX_MODE), REG_I2S0_RXTX_MODE_MASK, BIT1},
    {"I2S0_RXTX_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2S0_RXTX_MODE), REG_I2S0_RXTX_MODE_MASK, BIT1 | BIT0},
    {"I2S1_RXTX_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_RXTX_MODE), REG_I2S1_RXTX_MODE_MASK, BIT4},
    {"I2S1_RXTX_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_RXTX_MODE), REG_I2S1_RXTX_MODE_MASK, BIT5},
    {"I2S1_RXTX_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_RXTX_MODE), REG_I2S1_RXTX_MODE_MASK, BIT5 | BIT4},
    {"I2S1_RXTX_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_I2S1_RXTX_MODE), REG_I2S1_RXTX_MODE_MASK, BIT6},
    {"DMIC_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_MODE), REG_DMIC_MODE_MASK, BIT0},
    {"DMIC_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_MODE), REG_DMIC_MODE_MASK, BIT1},
    {"DMIC_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_MODE), REG_DMIC_MODE_MASK, BIT1 | BIT0},
    {"DMIC_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_MODE), REG_DMIC_MODE_MASK, BIT2},
    {"DMIC_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_MODE), REG_DMIC_MODE_MASK, BIT2 | BIT0},
    {"DMIC_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_MODE), REG_DMIC_MODE_MASK, BIT2 | BIT1},
    {"DMIC_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_MODE), REG_DMIC_MODE_MASK, BIT2 | BIT1 | BIT0},
    {"DMIC_MODE_8", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_MODE), REG_DMIC_MODE_MASK, BIT3},
    {"DMIC_MODE_9", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_MODE), REG_DMIC_MODE_MASK, BIT3 | BIT0},
    {"DMIC_MODE_10", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_MODE), REG_DMIC_MODE_MASK, BIT3 | BIT1},
    {"DMIC_4CH_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_4CH_MODE), REG_DMIC_4CH_MODE_MASK, BIT0},
    {"DMIC_4CH_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_4CH_MODE), REG_DMIC_4CH_MODE_MASK, BIT1},
    {"DMIC_4CH_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_4CH_MODE), REG_DMIC_4CH_MODE_MASK, BIT1 | BIT0},
    {"DMIC_4CH_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_4CH_MODE), REG_DMIC_4CH_MODE_MASK, BIT2},
    {"DMIC_4CH_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_4CH_MODE), REG_DMIC_4CH_MODE_MASK, BIT2 | BIT0},
    {"DMIC_4CH_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_4CH_MODE), REG_DMIC_4CH_MODE_MASK, BIT2 | BIT1},
    {"DMIC_4CH_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_4CH_MODE), REG_DMIC_4CH_MODE_MASK, BIT2 | BIT1 | BIT0},
    {"DMIC_4CH_MODE_8", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_4CH_MODE), REG_DMIC_4CH_MODE_MASK, BIT3},
    {"DMIC_4CH_MODE_9", _RIUA_16BIT(PADTOP_BANK, REG_DMIC_4CH_MODE), REG_DMIC_4CH_MODE_MASK, BIT3 | BIT0},
    {"SR0_MIPI_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR0_MIPI_MODE), REG_SR0_MIPI_MODE_MASK, BIT0},
    {"SR0_MIPI_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR0_MIPI_MODE), REG_SR0_MIPI_MODE_MASK, BIT1},
    {"SR1_MIPI_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR1_MIPI_MODE), REG_SR1_MIPI_MODE_MASK, BIT2},
    {"SR1_MIPI_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR1_MIPI_MODE), REG_SR1_MIPI_MODE_MASK, BIT3},
    {"SR2_MIPI_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR2_MIPI_MODE), REG_SR2_MIPI_MODE_MASK, BIT4},
    {"SR2_MIPI_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR2_MIPI_MODE), REG_SR2_MIPI_MODE_MASK, BIT5},
    {"SR3_MIPI_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR3_MIPI_MODE), REG_SR3_MIPI_MODE_MASK, BIT6},
    {"SR3_MIPI_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR3_MIPI_MODE), REG_SR3_MIPI_MODE_MASK, BIT7},
    {"SR00_MCLK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR00_MCLK_MODE), REG_SR00_MCLK_MODE_MASK, BIT0},
    {"SR00_MCLK_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR00_MCLK_MODE), REG_SR00_MCLK_MODE_MASK, BIT1},
    {"SR01_MCLK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR01_MCLK_MODE), REG_SR01_MCLK_MODE_MASK, BIT2},
    {"SR01_MCLK_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR01_MCLK_MODE), REG_SR01_MCLK_MODE_MASK, BIT3},
    {"SR02_MCLK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR02_MCLK_MODE), REG_SR02_MCLK_MODE_MASK, BIT4},
    {"SR02_MCLK_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR02_MCLK_MODE), REG_SR02_MCLK_MODE_MASK, BIT5},
    {"SR03_MCLK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR03_MCLK_MODE), REG_SR03_MCLK_MODE_MASK, BIT6},
    {"SR03_MCLK_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR03_MCLK_MODE), REG_SR03_MCLK_MODE_MASK, BIT7},
    {"SR10_MCLK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR10_MCLK_MODE), REG_SR10_MCLK_MODE_MASK, BIT8},
    {"SR10_MCLK_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR10_MCLK_MODE), REG_SR10_MCLK_MODE_MASK, BIT9},
    {"SR10_MCLK_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SR10_MCLK_MODE), REG_SR10_MCLK_MODE_MASK, BIT9 | BIT8},
    {"SR11_MCLK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR11_MCLK_MODE), REG_SR11_MCLK_MODE_MASK, BIT10},
    {"SR11_MCLK_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR11_MCLK_MODE), REG_SR11_MCLK_MODE_MASK, BIT11},
    {"SR12_MCLK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR12_MCLK_MODE), REG_SR12_MCLK_MODE_MASK, BIT12},
    {"SR12_MCLK_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR12_MCLK_MODE), REG_SR12_MCLK_MODE_MASK, BIT13},
    {"SR13_MCLK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR13_MCLK_MODE), REG_SR13_MCLK_MODE_MASK, BIT14},
    {"SR13_MCLK_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR13_MCLK_MODE), REG_SR13_MCLK_MODE_MASK, BIT15},
    {"SR13_MCLK_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SR13_MCLK_MODE), REG_SR13_MCLK_MODE_MASK, BIT15 | BIT14},
    {"SR00_RST_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR00_RST_MODE), REG_SR00_RST_MODE_MASK, BIT0},
    {"SR00_RST_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR00_RST_MODE), REG_SR00_RST_MODE_MASK, BIT1},
    {"SR01_RST_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR01_RST_MODE), REG_SR01_RST_MODE_MASK, BIT2},
    {"SR01_RST_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR01_RST_MODE), REG_SR01_RST_MODE_MASK, BIT3},
    {"SR02_RST_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR02_RST_MODE), REG_SR02_RST_MODE_MASK, BIT4},
    {"SR02_RST_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR02_RST_MODE), REG_SR02_RST_MODE_MASK, BIT5},
    {"SR03_RST_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR03_RST_MODE), REG_SR03_RST_MODE_MASK, BIT6},
    {"SR03_RST_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR03_RST_MODE), REG_SR03_RST_MODE_MASK, BIT7},
    {"SR10_RST_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR10_RST_MODE), REG_SR10_RST_MODE_MASK, BIT8},
    {"SR10_RST_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR10_RST_MODE), REG_SR10_RST_MODE_MASK, BIT9},
    {"SR10_RST_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SR10_RST_MODE), REG_SR10_RST_MODE_MASK, BIT9 | BIT8},
    {"SR11_RST_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR11_RST_MODE), REG_SR11_RST_MODE_MASK, BIT10},
    {"SR11_RST_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR11_RST_MODE), REG_SR11_RST_MODE_MASK, BIT11},
    {"SR12_RST_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR12_RST_MODE), REG_SR12_RST_MODE_MASK, BIT12},
    {"SR12_RST_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR12_RST_MODE), REG_SR12_RST_MODE_MASK, BIT13},
    {"SR13_RST_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR13_RST_MODE), REG_SR13_RST_MODE_MASK, BIT14},
    {"SR13_RST_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR13_RST_MODE), REG_SR13_RST_MODE_MASK, BIT15},
    {"SR13_RST_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SR13_RST_MODE), REG_SR13_RST_MODE_MASK, BIT15 | BIT14},
    {"SR00_PDN_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR00_PDN_MODE), REG_SR00_PDN_MODE_MASK, BIT0},
    {"SR00_PDN_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR00_PDN_MODE), REG_SR00_PDN_MODE_MASK, BIT1},
    {"SR00_PDN_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SR00_PDN_MODE), REG_SR00_PDN_MODE_MASK, BIT1 | BIT0},
    {"SR01_PDN_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR01_PDN_MODE), REG_SR01_PDN_MODE_MASK, BIT2},
    {"SR01_PDN_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR01_PDN_MODE), REG_SR01_PDN_MODE_MASK, BIT3},
    {"SR01_PDN_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SR01_PDN_MODE), REG_SR01_PDN_MODE_MASK, BIT3 | BIT2},
    {"SR02_PDN_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR02_PDN_MODE), REG_SR02_PDN_MODE_MASK, BIT4},
    {"SR02_PDN_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR02_PDN_MODE), REG_SR02_PDN_MODE_MASK, BIT5},
    {"SR02_PDN_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SR02_PDN_MODE), REG_SR02_PDN_MODE_MASK, BIT5 | BIT4},
    {"SR03_PDN_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR03_PDN_MODE), REG_SR03_PDN_MODE_MASK, BIT6},
    {"SR03_PDN_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR03_PDN_MODE), REG_SR03_PDN_MODE_MASK, BIT7},
    {"SR10_PDN_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR10_PDN_MODE), REG_SR10_PDN_MODE_MASK, BIT8},
    {"SR10_PDN_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR10_PDN_MODE), REG_SR10_PDN_MODE_MASK, BIT9},
    {"SR11_PDN_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR11_PDN_MODE), REG_SR11_PDN_MODE_MASK, BIT10},
    {"SR12_PDN_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR12_PDN_MODE), REG_SR12_PDN_MODE_MASK, BIT12},
    {"SR13_PDN_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR13_PDN_MODE), REG_SR13_PDN_MODE_MASK, BIT14},
    {"SR0_PCLK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR0_PCLK_MODE), REG_SR0_PCLK_MODE_MASK, BIT0},
    {"SR1_PCLK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR1_PCLK_MODE), REG_SR1_PCLK_MODE_MASK, BIT2},
    {"SR2_PCLK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR2_PCLK_MODE), REG_SR2_PCLK_MODE_MASK, BIT4},
    {"SR3_PCLK_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR3_PCLK_MODE), REG_SR3_PCLK_MODE_MASK, BIT6},
    {"SR0_SYNC_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR0_SYNC_MODE), REG_SR0_SYNC_MODE_MASK, BIT8},
    {"SR1_SYNC_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SYNC_MODE), REG_SR1_SYNC_MODE_MASK, BIT10},
    {"SR2_SYNC_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR2_SYNC_MODE), REG_SR2_SYNC_MODE_MASK, BIT12},
    {"SR3_SYNC_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR3_SYNC_MODE), REG_SR3_SYNC_MODE_MASK, BIT14},
    {"SR3_SYNC_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR3_SYNC_MODE), REG_SR3_SYNC_MODE_MASK, BIT15},
    {"SR0_BT601_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR0_BT601_MODE), REG_SR0_BT601_MODE_MASK, BIT0},
    {"SR1_BT601_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR1_BT601_MODE), REG_SR1_BT601_MODE_MASK, BIT4},
    {"SR2_BT601_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR2_BT601_MODE), REG_SR2_BT601_MODE_MASK, BIT8},
    {"SR3_BT601_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR3_BT601_MODE), REG_SR3_BT601_MODE_MASK, BIT12},
    {"SR0_BT656_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR0_BT656_MODE), REG_SR0_BT656_MODE_MASK, BIT0},
    {"SR1_BT656_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR1_BT656_MODE), REG_SR1_BT656_MODE_MASK, BIT2},
    {"SR2_BT656_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR2_BT656_MODE), REG_SR2_BT656_MODE_MASK, BIT4},
    {"SR3_BT656_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR3_BT656_MODE), REG_SR3_BT656_MODE_MASK, BIT6},
    {"SR4_BT656_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR4_BT656_MODE), REG_SR4_BT656_MODE_MASK, BIT8},
    {"SR5_BT656_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR5_BT656_MODE), REG_SR5_BT656_MODE_MASK, BIT10},
    {"SR6_BT656_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR6_BT656_MODE), REG_SR6_BT656_MODE_MASK, BIT12},
    {"SR7_BT656_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR7_BT656_MODE), REG_SR7_BT656_MODE_MASK, BIT14},
    {"SR0_BT1120_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR0_BT1120_MODE), REG_SR0_BT1120_MODE_MASK, BIT0},
    {"SR0_BT1120_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR0_BT1120_MODE), REG_SR0_BT1120_MODE_MASK, BIT1},
    {"SR1_BT1120_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR1_BT1120_MODE), REG_SR1_BT1120_MODE_MASK, BIT4},
    {"SR1_BT1120_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR1_BT1120_MODE), REG_SR1_BT1120_MODE_MASK, BIT5},
    {"SR2_BT1120_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR2_BT1120_MODE), REG_SR2_BT1120_MODE_MASK, BIT8},
    {"SR2_BT1120_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR2_BT1120_MODE), REG_SR2_BT1120_MODE_MASK, BIT9},
    {"SR2_BT1120_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SR2_BT1120_MODE), REG_SR2_BT1120_MODE_MASK, BIT9 | BIT8},
    {"SR2_BT1120_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_SR2_BT1120_MODE), REG_SR2_BT1120_MODE_MASK, BIT10},
    {"SR3_BT1120_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR3_BT1120_MODE), REG_SR3_BT1120_MODE_MASK, BIT12},
    {"SR3_BT1120_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR3_BT1120_MODE), REG_SR3_BT1120_MODE_MASK, BIT13},
    {"MIPI_TX_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_MIPI_TX_MODE), REG_MIPI_TX_MODE_MASK, BIT0},
    {"MIPI_TX_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_MIPI_TX_MODE), REG_MIPI_TX_MODE_MASK, BIT1},
    {"ISP0_IR_OUT_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_ISP0_IR_OUT_MODE), REG_ISP0_IR_OUT_MODE_MASK, BIT8},
    {"ISP0_IR_OUT_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_ISP0_IR_OUT_MODE), REG_ISP0_IR_OUT_MODE_MASK, BIT9},
    {"ISP0_IR_OUT_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_ISP0_IR_OUT_MODE), REG_ISP0_IR_OUT_MODE_MASK, BIT9 | BIT8},
    {"ISP1_IR_OUT_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_ISP1_IR_OUT_MODE), REG_ISP1_IR_OUT_MODE_MASK, BIT12},
    {"ISP1_IR_OUT_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_ISP1_IR_OUT_MODE), REG_ISP1_IR_OUT_MODE_MASK, BIT13},
    {"ISP1_IR_OUT_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_ISP1_IR_OUT_MODE), REG_ISP1_IR_OUT_MODE_MASK, BIT13 | BIT12},
    {"SR0_SLAVE_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR0_SLAVE_MODE), REG_SR0_SLAVE_MODE_MASK, BIT0},
    {"SR0_SLAVE_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR0_SLAVE_MODE), REG_SR0_SLAVE_MODE_MASK, BIT1},
    {"SR0_SLAVE_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SR0_SLAVE_MODE), REG_SR0_SLAVE_MODE_MASK, BIT1 | BIT0},
    {"SR0_SLAVE_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_SR0_SLAVE_MODE), REG_SR0_SLAVE_MODE_MASK, BIT2},
    {"SR0_SLAVE_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_SR0_SLAVE_MODE), REG_SR0_SLAVE_MODE_MASK, BIT2 | BIT0},
    {"SR0_SLAVE_MODE_6", _RIUA_16BIT(PADTOP_BANK, REG_SR0_SLAVE_MODE), REG_SR0_SLAVE_MODE_MASK, BIT2 | BIT1},
    {"SR0_SLAVE_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_SR0_SLAVE_MODE), REG_SR0_SLAVE_MODE_MASK, BIT2 | BIT1 | BIT0},
    {"SR0_SLAVE_MODE_8", _RIUA_16BIT(PADTOP_BANK, REG_SR0_SLAVE_MODE), REG_SR0_SLAVE_MODE_MASK, BIT3},
    {"SR0_SLAVE_MODE_9", _RIUA_16BIT(PADTOP_BANK, REG_SR0_SLAVE_MODE), REG_SR0_SLAVE_MODE_MASK, BIT3 | BIT0},
    {"SR0_SLAVE_MODE_10", _RIUA_16BIT(PADTOP_BANK, REG_SR0_SLAVE_MODE), REG_SR0_SLAVE_MODE_MASK, BIT3 | BIT1},
    {"SR0_SLAVE_MODE_11", _RIUA_16BIT(PADTOP_BANK, REG_SR0_SLAVE_MODE), REG_SR0_SLAVE_MODE_MASK, BIT3 | BIT1 | BIT0},
    {"SR1_SLAVE_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SLAVE_MODE), REG_SR1_SLAVE_MODE_MASK, BIT4},
    {"SR1_SLAVE_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SLAVE_MODE), REG_SR1_SLAVE_MODE_MASK, BIT5},
    {"SR1_SLAVE_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SLAVE_MODE), REG_SR1_SLAVE_MODE_MASK, BIT5 | BIT4},
    {"SR1_SLAVE_MODE_4", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SLAVE_MODE), REG_SR1_SLAVE_MODE_MASK, BIT6},
    {"SR1_SLAVE_MODE_5", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SLAVE_MODE), REG_SR1_SLAVE_MODE_MASK, BIT6 | BIT4},
    {"SR1_SLAVE_MODE_7", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SLAVE_MODE), REG_SR1_SLAVE_MODE_MASK, BIT6 | BIT5 | BIT4},
    {"SR1_SLAVE_MODE_8", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SLAVE_MODE), REG_SR1_SLAVE_MODE_MASK, BIT7},
    {"SR1_SLAVE_MODE_9", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SLAVE_MODE), REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT4},
    {"SR1_SLAVE_MODE_10", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SLAVE_MODE), REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT5},
    {"SR1_SLAVE_MODE_11", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SLAVE_MODE), REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT5 | BIT4},
    {"SR1_SLAVE_MODE_12", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SLAVE_MODE), REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT6},
    {"SR1_SLAVE_MODE_13", _RIUA_16BIT(PADTOP_BANK, REG_SR1_SLAVE_MODE), REG_SR1_SLAVE_MODE_MASK, BIT7 | BIT6 | BIT4},
    {"TTL24_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_TTL24_MODE), REG_TTL24_MODE_MASK, BIT8},
    {"TTL24_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_TTL24_MODE), REG_TTL24_MODE_MASK, BIT9},
    {"TTL24_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_TTL24_MODE), REG_TTL24_MODE_MASK, BIT9 | BIT8},
    {"TTL16_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_TTL16_MODE), REG_TTL16_MODE_MASK, BIT0},
    {"TTL16_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_TTL16_MODE), REG_TTL16_MODE_MASK, BIT1},
    {"RGB8_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_RGB8_MODE), REG_RGB8_MODE_MASK, BIT8},
    {"RGB8_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_RGB8_MODE), REG_RGB8_MODE_MASK, BIT9},
    {"RGB16_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_RGB16_MODE), REG_RGB16_MODE_MASK, BIT12},
    {"LCD_MCU8_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_LCD_MCU8_MODE), REG_LCD_MCU8_MODE_MASK, BIT0},
    {"LCD_MCU8_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_LCD_MCU8_MODE), REG_LCD_MCU8_MODE_MASK, BIT1},
    {"LCD_MCU16_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_LCD_MCU16_MODE), REG_LCD_MCU16_MODE_MASK, BIT2},
    {"LCD_MCU18_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_LCD_MCU18_MODE), REG_LCD_MCU18_MODE_MASK, BIT4},
    {"BT656_OUT_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_BT656_OUT_MODE), REG_BT656_OUT_MODE_MASK, BIT4},
    {"BT656_OUT_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_BT656_OUT_MODE), REG_BT656_OUT_MODE_MASK, BIT5},
    {"BT656_OUT_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_BT656_OUT_MODE), REG_BT656_OUT_MODE_MASK, BIT5 | BIT4},
    {"BT601_OUT_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_BT601_OUT_MODE), REG_BT601_OUT_MODE_MASK, BIT4},
    {"BT601_OUT_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_BT601_OUT_MODE), REG_BT601_OUT_MODE_MASK, BIT5},
    {"BT1120_OUT_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_BT1120_OUT_MODE), REG_BT1120_OUT_MODE_MASK, BIT0},
    {"BT1120_OUT_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_BT1120_OUT_MODE), REG_BT1120_OUT_MODE_MASK, BIT1},
    {"VGA_HSYNC_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_VGA_HSYNC_MODE), REG_VGA_HSYNC_MODE_MASK, BIT8},
    {"VGA_VSYNC_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_VGA_VSYNC_MODE), REG_VGA_VSYNC_MODE_MASK, BIT9},
    {"SATA0_LED_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SATA0_LED_MODE), REG_SATA0_LED_MODE_MASK, BIT0},
    {"SATA0_LED_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SATA0_LED_MODE), REG_SATA0_LED_MODE_MASK, BIT1},
    {"SATA0_LED_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SATA0_LED_MODE), REG_SATA0_LED_MODE_MASK, BIT1 | BIT0},
    {"SATA1_LED_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_SATA1_LED_MODE), REG_SATA1_LED_MODE_MASK, BIT4},
    {"SATA1_LED_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_SATA1_LED_MODE), REG_SATA1_LED_MODE_MASK, BIT5},
    {"SATA1_LED_MODE_3", _RIUA_16BIT(PADTOP_BANK, REG_SATA1_LED_MODE), REG_SATA1_LED_MODE_MASK, BIT5 | BIT4},
    {"MII0_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_MII0_MODE), REG_MII0_MODE_MASK, BIT0},
    {"MII1_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_MII1_MODE), REG_MII1_MODE_MASK, BIT1},
    {"GPHY0_REF_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_GPHY0_REF_MODE), REG_GPHY0_REF_MODE_MASK, BIT0},
    {"GPHY0_REF_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_GPHY0_REF_MODE), REG_GPHY0_REF_MODE_MASK, BIT1},
    {"GPHY1_REF_MODE_1", _RIUA_16BIT(PADTOP_BANK, REG_GPHY1_REF_MODE), REG_GPHY1_REF_MODE_MASK, BIT4},
    {"GPHY1_REF_MODE_2", _RIUA_16BIT(PADTOP_BANK, REG_GPHY1_REF_MODE), REG_GPHY1_REF_MODE_MASK, BIT5},
    {"OTP_TEST_1", _RIUA_16BIT(PADTOP_BANK, REG_OTP_TEST), REG_OTP_TEST_MASK, BIT8},
};

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : _HalCheckPin
//  Description :
//------------------------------------------------------------------------------
static S32 _HalCheckPin(U32 padID)
{
    if (GPIO_NR <= padID)
    {
        return FALSE;
    }
    return TRUE;
}

#if 0
static void _HalSARGPIOWriteRegBit(u32 u32RegOffset, bool bEnable, U8 u8BitMsk)
{
    if (bEnable)
        _GPIO_R_BYTE(_RIUA_8BIT(PM_SAR_BANK, u32RegOffset)) |= u8BitMsk;
    else
        _GPIO_R_BYTE(_RIUA_8BIT(PM_SAR_BANK, u32RegOffset)) &= (~u8BitMsk);
}
#endif

void _HalPadDisablePadMux(U32 u32PadModeID)
{
    if (_GPIO_R_WORD_MASK(m_stPadModeInfoTbl[u32PadModeID].u64ModeRIU, m_stPadModeInfoTbl[u32PadModeID].u16ModeMask))
    {
        _GPIO_W_WORD_MASK(m_stPadModeInfoTbl[u32PadModeID].u64ModeRIU, 0, m_stPadModeInfoTbl[u32PadModeID].u16ModeMask);
    }
}

void _HalPadEnablePadMux(U32 u32PadModeID)
{
    _GPIO_W_WORD_MASK(m_stPadModeInfoTbl[u32PadModeID].u64ModeRIU, m_stPadModeInfoTbl[u32PadModeID].u16ModeVal,
                      m_stPadModeInfoTbl[u32PadModeID].u16ModeMask);
}

static S32 HalPadSetMode_General(U32 u32PadID, U32 u32Mode)
{
    U64 u64RegAddr   = 0;
    U16 u16RegVal    = 0;
    U8  u8ModeIsFind = 0;
    U16 i            = 0;

    for (i = 0; i < m_stPadMuxEntry[u32PadID].size; i++)
    {
        u64RegAddr = _RIUA_16BIT(m_stPadMuxEntry[u32PadID].padmux[i].base, m_stPadMuxEntry[u32PadID].padmux[i].offset);
        if (u32Mode == m_stPadMuxEntry[u32PadID].padmux[i].mode)
        {
            u16RegVal = _GPIO_R_WORD_MASK(u64RegAddr, 0xFFFF);
            u16RegVal &= ~(m_stPadMuxEntry[u32PadID].padmux[i].mask);
            u16RegVal |= m_stPadMuxEntry[u32PadID].padmux[i].val; // CHECK Multi-Pad Mode
            _GPIO_W_WORD_MASK(u64RegAddr, u16RegVal, 0xFFFF);
            u8ModeIsFind       = 1;
            Pad_Mode[u32PadID] = u32Mode;
#if (ENABLE_CHECK_ALL_PAD_CONFLICT == 0)
            break;
#endif
        }
        else
        {
            if ((u32Mode == PINMUX_FOR_GPIO_MODE) && (m_stPadMuxEntry[u32PadID].padmux[i].mode > PRIORITY_GREATER_GPIO))
                continue;
            u16RegVal = _GPIO_R_WORD_MASK(u64RegAddr, m_stPadMuxEntry[u32PadID].padmux[i].mask);
            if (u16RegVal == m_stPadMuxEntry[u32PadID].padmux[i].val)
            {
                printk(KERN_INFO "[Padmux]reset PAD%d(reg 0x%x:%x; mask0x%x) t0 %s (org: %s)\n", u32PadID,
                       m_stPadMuxEntry[u32PadID].padmux[i].base, m_stPadMuxEntry[u32PadID].padmux[i].offset,
                       m_stPadMuxEntry[u32PadID].padmux[i].mask, m_stPadModeInfoTbl[u32Mode].u8PadName,
                       m_stPadModeInfoTbl[m_stPadMuxEntry[u32PadID].padmux[i].mode].u8PadName);
                if (m_stPadMuxEntry[u32PadID].padmux[i].val != 0)
                {
                    _GPIO_W_WORD_MASK(u64RegAddr, 0, m_stPadMuxEntry[u32PadID].padmux[i].mask);
                }
                else
                {
                    _GPIO_W_WORD_MASK(u64RegAddr, m_stPadMuxEntry[u32PadID].padmux[i].mask,
                                      m_stPadMuxEntry[u32PadID].padmux[i].mask);
                }
            }
        }
    }
    return (u8ModeIsFind) ? 0 : 1;
}

//#define PAD_PM_IRIN_PATCH               // m6 should patch because hardware design
//------------------------------------------------------------------------------
//  Function    : HalPadSetVal
//  Description :
//------------------------------------------------------------------------------
S32 HalPadSetVal(U32 u32PadID, U32 u32Mode)
{
    if (FALSE == _HalCheckPin(u32PadID))
    {
        return 1;
    }
    return HalPadSetMode_General(u32PadID, u32Mode);
}

//------------------------------------------------------------------------------
//  Function    : HalPadSetVal
//  Description :
//------------------------------------------------------------------------------
S32 HalPadGetVal(U32 u32PadID, U32* u32Mode)
{
    if (FALSE == _HalCheckPin(u32PadID))
    {
        return 1;
    }
    *u32Mode = Pad_Mode[u32PadID];
    return 0;
}

//------------------------------------------------------------------------------
//  Function    :set GPIO voltage value
//  Description :only for i7, 3.3V is not allowed when MODE = 1
//------------------------------------------------------------------------------
void HalGPIOSetVol(U32 u32Group, U32 u32Mode)
{
    if (u32Mode)
    {
        SETREG16((0x1F000000) + (0x00103c00 << 1) + (0x04 << 2), BIT0 << u32Group);
        printk("\33[1;31m[ATTENTION],[MODE 1]in Group %d,IO voltage set to 3.3V is NOT allow!\33[0m", u32Group);
    }
    else
    {
        CLRREG16((0x1F000000) + (0x00103c00 << 1) + (0x04 << 2), BIT0 << u32Group);
        // printk("\33[1;31m[ATTENTION],[MODE 0]in Group %d,IO voltage can set to 3.3V or 1.8V\33[0m", u32Group);
    }
}
//------------------------------------------------------------------------------
//  Function    : HalPadSet
//  Description :
//------------------------------------------------------------------------------
S32 HalPadSetMode(U32 u32Mode)
{
    U32 u32PadID;
    U16 k = 0;
    U16 i = 0;

    for (k = 0; k < sizeof(m_stPadMuxEntry) / sizeof(struct stPadMuxEntry); k++)
    {
        for (i = 0; i < m_stPadMuxEntry[k].size; i++)
        {
            if (u32Mode == m_stPadMuxEntry[k].padmux[i].mode)
            {
                u32PadID = m_stPadMuxEntry[k].padmux[i].padID;
                if (u32PadID >= GPIO_NR)
                {
                    return 1;
                }
                if (HalPadSetMode_General(u32PadID, u32Mode))
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

S32 _HalPadCheckDisablePadMux(U32 u32PadModeID)
{
    U16 u16RegVal = 0;

    u16RegVal =
        _GPIO_R_WORD_MASK(m_stPadModeInfoTbl[u32PadModeID].u64ModeRIU, m_stPadModeInfoTbl[u32PadModeID].u16ModeMask);

    if (u16RegVal != 0)
    {
        m_stPadCheckVal.infos[m_stPadCheckVal.infocount].base =
            (U16)((m_stPadModeInfoTbl[u32PadModeID].u64ModeRIU & 0xFFFF00) >> 9);
        m_stPadCheckVal.infos[m_stPadCheckVal.infocount].offset =
            (U16)((m_stPadModeInfoTbl[u32PadModeID].u64ModeRIU & 0x1FF) >> 2);
        m_stPadCheckVal.infos[m_stPadCheckVal.infocount].mask   = m_stPadModeInfoTbl[u32PadModeID].u16ModeMask;
        m_stPadCheckVal.infos[m_stPadCheckVal.infocount].val    = 0;
        m_stPadCheckVal.infos[m_stPadCheckVal.infocount].regval = u16RegVal;
        m_stPadCheckVal.infocount++;
        return -1;
    }
    return 0;
}

S32 _HalPadCheckEnablePadMux(U32 u32PadModeID)
{
    U16 u16RegVal = 0;

    u16RegVal =
        _GPIO_R_WORD_MASK(m_stPadModeInfoTbl[u32PadModeID].u64ModeRIU, m_stPadModeInfoTbl[u32PadModeID].u16ModeMask);

    if (u16RegVal != m_stPadModeInfoTbl[u32PadModeID].u16ModeVal)
    {
        m_stPadCheckVal.infos[m_stPadCheckVal.infocount].base =
            (U16)((m_stPadModeInfoTbl[u32PadModeID].u64ModeRIU & 0xFFFF00) >> 9);
        m_stPadCheckVal.infos[m_stPadCheckVal.infocount].offset =
            (U16)((m_stPadModeInfoTbl[u32PadModeID].u64ModeRIU & 0x1FF) >> 2);
        m_stPadCheckVal.infos[m_stPadCheckVal.infocount].mask   = m_stPadModeInfoTbl[u32PadModeID].u16ModeMask;
        m_stPadCheckVal.infos[m_stPadCheckVal.infocount].val    = m_stPadModeInfoTbl[u32PadModeID].u16ModeVal;
        m_stPadCheckVal.infos[m_stPadCheckVal.infocount].regval = u16RegVal;
        m_stPadCheckVal.infocount++;
        return -1;
    }
    return 0;
}

static S32 HalPadCheckMode_General(U32 u32PadID, U32 u32Mode)
{
    U64 u64RegAddr   = 0;
    U16 u16RegVal    = 0;
    U8  u8ModeIsFind = 0;
    U8  u8ModeIsErr  = 0;
    U16 i            = 0;

    for (i = 0; i < m_stPadMuxEntry[u32PadID].size; i++)
    {
        if (u32PadID == m_stPadMuxEntry[u32PadID].padmux[i].padID)
        {
            u64RegAddr =
                _RIUA_16BIT(m_stPadMuxEntry[u32PadID].padmux[i].base, m_stPadMuxEntry[u32PadID].padmux[i].offset);

            if (u32Mode == m_stPadMuxEntry[u32PadID].padmux[i].mode)
            {
                u16RegVal = _GPIO_R_WORD_MASK(u64RegAddr, 0xFFFF);
                u16RegVal &= (m_stPadMuxEntry[u32PadID].padmux[i].mask);
                if (u16RegVal != m_stPadMuxEntry[u32PadID].padmux[i].val) // CHECK Multi-Pad Mode
                {
                    u8ModeIsErr++;

                    m_stPadCheckVal.infos[m_stPadCheckVal.infocount].base =
                        (U16)(m_stPadMuxEntry[u32PadID].padmux[i].base >> 8);
                    m_stPadCheckVal.infos[m_stPadCheckVal.infocount].offset =
                        m_stPadMuxEntry[u32PadID].padmux[i].offset;
                    m_stPadCheckVal.infos[m_stPadCheckVal.infocount].mask   = m_stPadMuxEntry[u32PadID].padmux[i].mask;
                    m_stPadCheckVal.infos[m_stPadCheckVal.infocount].val    = m_stPadMuxEntry[u32PadID].padmux[i].val;
                    m_stPadCheckVal.infos[m_stPadCheckVal.infocount].regval = u16RegVal;
                    m_stPadCheckVal.infocount++;
                }

                u8ModeIsFind = 1;
#if (ENABLE_CHECK_ALL_PAD_CONFLICT == 0)
                break;
#endif
            }
            else
            {
                if (u32Mode == PINMUX_FOR_GPIO_MODE)
                    continue;
                u16RegVal = _GPIO_R_WORD_MASK(u64RegAddr, m_stPadMuxEntry[u32PadID].padmux[i].mask);
                if (u16RegVal == m_stPadMuxEntry[u32PadID].padmux[i].val)
                {
                    u8ModeIsErr++;

                    m_stPadCheckVal.infos[m_stPadCheckVal.infocount].base =
                        (U16)(m_stPadMuxEntry[u32PadID].padmux[i].base >> 8);
                    m_stPadCheckVal.infos[m_stPadCheckVal.infocount].offset =
                        m_stPadMuxEntry[u32PadID].padmux[i].offset;
                    m_stPadCheckVal.infos[m_stPadCheckVal.infocount].mask   = m_stPadMuxEntry[u32PadID].padmux[i].mask;
                    m_stPadCheckVal.infos[m_stPadCheckVal.infocount].val    = m_stPadMuxEntry[u32PadID].padmux[i].val;
                    m_stPadCheckVal.infos[m_stPadCheckVal.infocount].regval = u16RegVal;
                    m_stPadCheckVal.infocount++;
                }
            }
        }
    }

    return (u8ModeIsFind && !u8ModeIsErr) ? 0 : 1;
}

S32 HalPadCheckVal(U32 u32PadID, U32 u32Mode)
{
    if (FALSE == _HalCheckPin(u32PadID))
    {
        return 1;
    }

    memset(&m_stPadCheckVal, 0, sizeof(m_stPadCheckVal));
    return HalPadCheckMode_General(u32PadID, u32Mode);
}

U8 HalPadCheckInfoCount(void)
{
    return m_stPadCheckVal.infocount;
}

void* HalPadCheckInfoGet(U8 u8Index)
{
    return (void*)&m_stPadCheckVal.infos[u8Index];
}

U8 HalPadModeToVal(U8* pu8Mode, U8* u8Index)
{
    U16 index;
    for (index = 0; index < (sizeof(m_stPadModeInfoTbl) / sizeof(m_stPadModeInfoTbl[0])); index++)
    {
        if (!strcmp(m_stPadModeInfoTbl[index].u8PadName, pu8Mode))
        {
            *u8Index = index;
            return 0;
        }
    }
    return 1;
}

static U32 u32PadID[GPIO_NR + 1] = {0};

U32* HalPadModeToPadIndex(U32 u32Mode)
{
    U16 k     = 0;
    U16 i     = 0;
    U16 Count = 0;

    for (k = 0; k < sizeof(m_stPadMuxEntry) / sizeof(struct stPadMuxEntry); k++)
    {
        for (i = 0; i < m_stPadMuxEntry[k].size; i++)
        {
            if (u32Mode == m_stPadMuxEntry[k].padmux[i].mode)
            {
                u32PadID[Count] = m_stPadMuxEntry[k].padmux[i].padID;
                Count += 1;
            }
        }
    }
    u32PadID[Count] = PAD_UNKNOWN;
    return u32PadID;
}
