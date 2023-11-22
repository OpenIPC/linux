/*
 * mhal_gpio.c- Sigmastar
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
#include <linux/kernel.h>
#include <linux/irqdomain.h>

#include "mhal_gpio.h"
#include "ms_platform.h"
#include "gpio.h"
#include "irqs.h"
#include "padmux.h"
#include "mhal_pinmux.h"
#include <registers.h>

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define GPIO_DBG 0
#if GPIO_DBG
#define GPIO_ERR(args...) printk(args)
#else
#define GPIO_ERR(args...)
#endif

typedef struct Special_GpioInfo
{
    U16 padID;
    U32 base;
    U16 offset;
    U16 mask;
    U16 val;
} S_GpioInfo;

#define _CONCAT(a, b) a##b
#define CONCAT(a, b)  _CONCAT(a, b)

#define GPIO0_PAD  PAD_SD1_IO1
#define GPIO0_NAME "PAD_SD1_IO1"
#define GPIO0_OEN  0x103E00, BIT2
#define GPIO0_OUT  0x103E00, BIT1
#define GPIO0_IN   0x103E00, BIT0
#define GPIO0_DRV  0x103E00, BIT7
#define GPIO0_PE   0x000000, BIT0
#define GPIO0_PS   0x000000, BIT0

#define GPIO1_PAD  PAD_SD1_IO0
#define GPIO1_NAME "PAD_SD1_IO0"
#define GPIO1_OEN  0x103E02, BIT2
#define GPIO1_OUT  0x103E02, BIT1
#define GPIO1_IN   0x103E02, BIT0
#define GPIO1_DRV  0x103E02, BIT7
#define GPIO1_PE   0x000000, BIT0
#define GPIO1_PS   0x000000, BIT0

#define GPIO2_PAD  PAD_SD1_IO5
#define GPIO2_NAME "PAD_SD1_IO5"
#define GPIO2_OEN  0x103E04, BIT2
#define GPIO2_OUT  0x103E04, BIT1
#define GPIO2_IN   0x103E04, BIT0
#define GPIO2_DRV  0x103E04, BIT7
#define GPIO2_PE   0x000000, BIT0
#define GPIO2_PS   0x000000, BIT0

#define GPIO3_PAD  PAD_SD1_IO4
#define GPIO3_NAME "PAD_SD1_IO4"
#define GPIO3_OEN  0x103E06, BIT2
#define GPIO3_OUT  0x103E06, BIT1
#define GPIO3_IN   0x103E06, BIT0
#define GPIO3_DRV  0x103E06, BIT7
#define GPIO3_PE   0x000000, BIT0
#define GPIO3_PS   0x000000, BIT0

#define GPIO4_PAD  PAD_SD1_IO3
#define GPIO4_NAME "PAD_SD1_IO3"
#define GPIO4_OEN  0x103E08, BIT2
#define GPIO4_OUT  0x103E08, BIT1
#define GPIO4_IN   0x103E08, BIT0
#define GPIO4_DRV  0x103E08, BIT7
#define GPIO4_PE   0x000000, BIT0
#define GPIO4_PS   0x000000, BIT0

#define GPIO5_PAD  PAD_SD1_IO2
#define GPIO5_NAME "PAD_SD1_IO2"
#define GPIO5_OEN  0x103E0A, BIT2
#define GPIO5_OUT  0x103E0A, BIT1
#define GPIO5_IN   0x103E0A, BIT0
#define GPIO5_DRV  0x103E0A, BIT7
#define GPIO5_PE   0x000000, BIT0
#define GPIO5_PS   0x000000, BIT0

#define GPIO6_PAD  PAD_SD1_IO6
#define GPIO6_NAME "PAD_SD1_IO6"
#define GPIO6_OEN  0x103E0C, BIT2
#define GPIO6_OUT  0x103E0C, BIT1
#define GPIO6_IN   0x103E0C, BIT0
#define GPIO6_DRV  0x103E0C, BIT7
#define GPIO6_PE   0x000000, BIT0
#define GPIO6_PS   0x000000, BIT0

#define GPIO7_PAD  PAD_UART1_RX
#define GPIO7_NAME "PAD_UART1_RX"
#define GPIO7_OEN  0x103E0E, BIT2
#define GPIO7_OUT  0x103E0E, BIT1
#define GPIO7_IN   0x103E0E, BIT0
#define GPIO7_DRV  0x103E0E, BIT7
#define GPIO7_PE   0x000000, BIT0
#define GPIO7_PS   0x000000, BIT0

#define GPIO8_PAD  PAD_UART1_TX
#define GPIO8_NAME "PAD_UART1_TX"
#define GPIO8_OEN  0x103E10, BIT2
#define GPIO8_OUT  0x103E10, BIT1
#define GPIO8_IN   0x103E10, BIT0
#define GPIO8_DRV  0x103E10, BIT7
#define GPIO8_PE   0x000000, BIT0
#define GPIO8_PS   0x000000, BIT0

#define GPIO9_PAD  PAD_SPI0_CZ
#define GPIO9_NAME "PAD_SPI0_CZ"
#define GPIO9_OEN  0x103E12, BIT2
#define GPIO9_OUT  0x103E12, BIT1
#define GPIO9_IN   0x103E12, BIT0
#define GPIO9_DRV  0x103E12, BIT7 | BIT8
#define GPIO9_PE   0x000000, BIT0
#define GPIO9_PS   0x000000, BIT0

#define GPIO10_PAD  PAD_SPI0_CK
#define GPIO10_NAME "PAD_SPI0_CK"
#define GPIO10_OEN  0x103E14, BIT2
#define GPIO10_OUT  0x103E14, BIT1
#define GPIO10_IN   0x103E14, BIT0
#define GPIO10_DRV  0x103E14, BIT7 | BIT8
#define GPIO10_PE   0x000000, BIT0
#define GPIO10_PS   0x000000, BIT0

#define GPIO11_PAD  PAD_SPI0_DI
#define GPIO11_NAME "PAD_SPI0_DI"
#define GPIO11_OEN  0x103E16, BIT2
#define GPIO11_OUT  0x103E16, BIT1
#define GPIO11_IN   0x103E16, BIT0
#define GPIO11_DRV  0x103E16, BIT7 | BIT8
#define GPIO11_PE   0x000000, BIT0
#define GPIO11_PS   0x000000, BIT0

#define GPIO12_PAD  PAD_SPI0_DO
#define GPIO12_NAME "PAD_SPI0_DO"
#define GPIO12_OEN  0x103E18, BIT2
#define GPIO12_OUT  0x103E18, BIT1
#define GPIO12_IN   0x103E18, BIT0
#define GPIO12_DRV  0x103E18, BIT7 | BIT8
#define GPIO12_PE   0x000000, BIT0
#define GPIO12_PS   0x000000, BIT0

#define GPIO13_PAD  PAD_PWM0
#define GPIO13_NAME "PAD_PWM0"
#define GPIO13_OEN  0x103E1A, BIT2
#define GPIO13_OUT  0x103E1A, BIT1
#define GPIO13_IN   0x103E1A, BIT0
#define GPIO13_DRV  0x103E1A, BIT7
#define GPIO13_PE   0x000000, BIT0
#define GPIO13_PS   0x000000, BIT0

#define GPIO14_PAD  PAD_PWM1
#define GPIO14_NAME "PAD_PWM1"
#define GPIO14_OEN  0x103E1C, BIT2
#define GPIO14_OUT  0x103E1C, BIT1
#define GPIO14_IN   0x103E1C, BIT0
#define GPIO14_DRV  0x103E1C, BIT7
#define GPIO14_PE   0x000000, BIT0
#define GPIO14_PS   0x000000, BIT0

#define GPIO15_PAD  PAD_SD_CLK
#define GPIO15_NAME "PAD_SD_CLK"
#define GPIO15_OEN  0x103E1E, BIT2
#define GPIO15_OUT  0x103E1E, BIT1
#define GPIO15_IN   0x103E1E, BIT0
#define GPIO15_DRV  0x103E1E, BIT7
#define GPIO15_PE   0x000000, BIT0
#define GPIO15_PS   0x000000, BIT0

#define GPIO16_PAD  PAD_SD_CMD
#define GPIO16_NAME "PAD_SD_CMD"
#define GPIO16_OEN  0x103E20, BIT2
#define GPIO16_OUT  0x103E20, BIT1
#define GPIO16_IN   0x103E20, BIT0
#define GPIO16_DRV  0x103E20, BIT7
#define GPIO16_PE   0x000000, BIT0
#define GPIO16_PS   0x000000, BIT0

#define GPIO17_PAD  PAD_SD_D0
#define GPIO17_NAME "PAD_SD_D0"
#define GPIO17_OEN  0x103E22, BIT2
#define GPIO17_OUT  0x103E22, BIT1
#define GPIO17_IN   0x103E22, BIT0
#define GPIO17_DRV  0x103E22, BIT7
#define GPIO17_PE   0x000000, BIT0
#define GPIO17_PS   0x000000, BIT0

#define GPIO18_PAD  PAD_SD_D1
#define GPIO18_NAME "PAD_SD_D1"
#define GPIO18_OEN  0x103E24, BIT2
#define GPIO18_OUT  0x103E24, BIT1
#define GPIO18_IN   0x103E24, BIT0
#define GPIO18_DRV  0x103E24, BIT7
#define GPIO18_PE   0x000000, BIT0
#define GPIO18_PS   0x000000, BIT0

#define GPIO19_PAD  PAD_SD_D2
#define GPIO19_NAME "PAD_SD_D2"
#define GPIO19_OEN  0x103E26, BIT2
#define GPIO19_OUT  0x103E26, BIT1
#define GPIO19_IN   0x103E26, BIT0
#define GPIO19_DRV  0x103E26, BIT7
#define GPIO19_PE   0x000000, BIT0
#define GPIO19_PS   0x000000, BIT0

#define GPIO20_PAD  PAD_SD_D3
#define GPIO20_NAME "PAD_SD_D3"
#define GPIO20_OEN  0x103E28, BIT2
#define GPIO20_OUT  0x103E28, BIT1
#define GPIO20_IN   0x103E28, BIT0
#define GPIO20_DRV  0x103E28, BIT7
#define GPIO20_PE   0x000000, BIT0
#define GPIO20_PS   0x000000, BIT0

#define GPIO21_PAD  PAD_USB_CID
#define GPIO21_NAME "PAD_USB_CID"
#define GPIO21_OEN  0x103E2A, BIT2
#define GPIO21_OUT  0x103E2A, BIT1
#define GPIO21_IN   0x103E2A, BIT0
#define GPIO21_DRV  0x000000, BIT0
#define GPIO21_PE   0x000000, BIT0
#define GPIO21_PS   0x000000, BIT0

#define GPIO22_PAD  PAD_PM_SD_CDZ
#define GPIO22_NAME "PAD_PM_SD_CDZ"
#define GPIO22_OEN  0x103E2C, BIT2
#define GPIO22_OUT  0x103E2C, BIT1
#define GPIO22_IN   0x103E2C, BIT0
#define GPIO22_DRV  0x103E2C, BIT7
#define GPIO22_PE   0x000000, BIT0
#define GPIO22_PS   0x000000, BIT0

#define GPIO23_PAD  PAD_PM_IRIN
#define GPIO23_NAME "PAD_PM_IRIN"
#define GPIO23_OEN  0x103E2E, BIT2
#define GPIO23_OUT  0x103E2E, BIT1
#define GPIO23_IN   0x103E2E, BIT0
#define GPIO23_DRV  0x103E2E, BIT7
#define GPIO23_PE   0x000000, BIT0
#define GPIO23_PS   0x000000, BIT0

#define GPIO24_PAD  PAD_PM_UART_RX
#define GPIO24_NAME "PAD_PM_UART_RX"
#define GPIO24_OEN  0x103E32, BIT2
#define GPIO24_OUT  0x103E32, BIT1
#define GPIO24_IN   0x103E32, BIT0
#define GPIO24_DRV  0x103E32, BIT7
#define GPIO24_PE   0x000000, BIT0
#define GPIO24_PS   0x000000, BIT0

#define GPIO25_PAD  PAD_PM_UART_TX
#define GPIO25_NAME "PAD_PM_UART_TX"
#define GPIO25_OEN  0x103E34, BIT2
#define GPIO25_OUT  0x103E34, BIT1
#define GPIO25_IN   0x103E34, BIT0
#define GPIO25_DRV  0x103E34, BIT7
#define GPIO25_PE   0x000000, BIT0
#define GPIO25_PS   0x000000, BIT0

#define GPIO26_PAD  PAD_PM_GPIO0
#define GPIO26_NAME "PAD_PM_GPIO0"
#define GPIO26_OEN  0x103E36, BIT2
#define GPIO26_OUT  0x103E36, BIT1
#define GPIO26_IN   0x103E36, BIT0
#define GPIO26_DRV  0x103E36, BIT7
#define GPIO26_PE   0x000000, BIT0
#define GPIO26_PS   0x000000, BIT0

#define GPIO27_PAD  PAD_PM_GPIO1
#define GPIO27_NAME "PAD_PM_GPIO1"
#define GPIO27_OEN  0x103E38, BIT2
#define GPIO27_OUT  0x103E38, BIT1
#define GPIO27_IN   0x103E38, BIT0
#define GPIO27_DRV  0x103E38, BIT7
#define GPIO27_PE   0x000000, BIT0
#define GPIO27_PS   0x000000, BIT0

#define GPIO28_PAD  PAD_PM_GPIO2
#define GPIO28_NAME "PAD_PM_GPIO2"
#define GPIO28_OEN  0x103E3A, BIT2
#define GPIO28_OUT  0x103E3A, BIT1
#define GPIO28_IN   0x103E3A, BIT0
#define GPIO28_DRV  0x103E3A, BIT7
#define GPIO28_PE   0x000000, BIT0
#define GPIO28_PS   0x000000, BIT0

#define GPIO29_PAD  PAD_PM_GPIO3
#define GPIO29_NAME "PAD_PM_GPIO3"
#define GPIO29_OEN  0x103E3C, BIT2
#define GPIO29_OUT  0x103E3C, BIT1
#define GPIO29_IN   0x103E3C, BIT0
#define GPIO29_DRV  0x103E3C, BIT7
#define GPIO29_PE   0x000000, BIT0
#define GPIO29_PS   0x000000, BIT0

#define GPIO30_PAD  PAD_PM_GPIO4
#define GPIO30_NAME "PAD_PM_GPIO4"
#define GPIO30_OEN  0x103E3E, BIT2
#define GPIO30_OUT  0x103E3E, BIT1
#define GPIO30_IN   0x103E3E, BIT0
#define GPIO30_DRV  0x000000, BIT0
#define GPIO30_PE   0x000000, BIT0
#define GPIO30_PS   0x000000, BIT0

#define GPIO31_PAD  PAD_PM_GPIO7
#define GPIO31_NAME "PAD_PM_GPIO7"
#define GPIO31_OEN  0x103E40, BIT2
#define GPIO31_OUT  0x103E40, BIT1
#define GPIO31_IN   0x103E40, BIT0
#define GPIO31_DRV  0x103E40, BIT7
#define GPIO31_PE   0x000000, BIT0
#define GPIO31_PS   0x000000, BIT0

#define GPIO32_PAD  PAD_PM_GPIO8
#define GPIO32_NAME "PAD_PM_GPIO8"
#define GPIO32_OEN  0x103E42, BIT2
#define GPIO32_OUT  0x103E42, BIT1
#define GPIO32_IN   0x103E42, BIT0
#define GPIO32_DRV  0x103E42, BIT7
#define GPIO32_PE   0x000000, BIT0
#define GPIO32_PS   0x000000, BIT0

#define GPIO33_PAD  PAD_PM_GPIO9
#define GPIO33_NAME "PAD_PM_GPIO9"
#define GPIO33_OEN  0x103E44, BIT2
#define GPIO33_OUT  0x103E44, BIT1
#define GPIO33_IN   0x103E44, BIT0
#define GPIO33_DRV  0x103E44, BIT7
#define GPIO33_PE   0x000000, BIT0
#define GPIO33_PS   0x000000, BIT0

#define GPIO34_PAD  PAD_PM_SPI_CZ
#define GPIO34_NAME "PAD_PM_SPI_CZ"
#define GPIO34_OEN  0x103E46, BIT2
#define GPIO34_OUT  0x103E46, BIT1
#define GPIO34_IN   0x103E46, BIT0
#define GPIO34_DRV  0x103E46, BIT7
#define GPIO34_PE   0x000000, BIT0
#define GPIO34_PS   0x000000, BIT0

#define GPIO35_PAD  PAD_PM_SPI_DI
#define GPIO35_NAME "PAD_PM_SPI_DI"
#define GPIO35_OEN  0x103E48, BIT2
#define GPIO35_OUT  0x103E48, BIT1
#define GPIO35_IN   0x103E48, BIT0
#define GPIO35_DRV  0x103E48, BIT7
#define GPIO35_PE   0x000000, BIT0
#define GPIO35_PS   0x000000, BIT0

#define GPIO36_PAD  PAD_PM_SPI_WPZ
#define GPIO36_NAME "PAD_PM_SPI_WPZ"
#define GPIO36_OEN  0x103E4A, BIT2
#define GPIO36_OUT  0x103E4A, BIT1
#define GPIO36_IN   0x103E4A, BIT0
#define GPIO36_DRV  0x103E4A, BIT7
#define GPIO36_PE   0x000000, BIT0
#define GPIO36_PS   0x000000, BIT0

#define GPIO37_PAD  PAD_PM_SPI_DO
#define GPIO37_NAME "PAD_PM_SPI_DO"
#define GPIO37_OEN  0x103E4C, BIT2
#define GPIO37_OUT  0x103E4C, BIT1
#define GPIO37_IN   0x103E4C, BIT0
#define GPIO37_DRV  0x103E4C, BIT7
#define GPIO37_PE   0x000000, BIT0
#define GPIO37_PS   0x000000, BIT0

#define GPIO38_PAD  PAD_PM_SPI_CK
#define GPIO38_NAME "PAD_PM_SPI_CK"
#define GPIO38_OEN  0x103E4E, BIT2
#define GPIO38_OUT  0x103E4E, BIT1
#define GPIO38_IN   0x103E4E, BIT0
#define GPIO38_DRV  0x103E4E, BIT7
#define GPIO38_PE   0x000000, BIT0
#define GPIO38_PS   0x000000, BIT0

#define GPIO39_PAD  PAD_PM_SPI_HLD
#define GPIO39_NAME "PAD_PM_SPI_HLD"
#define GPIO39_OEN  0x103E50, BIT2
#define GPIO39_OUT  0x103E50, BIT1
#define GPIO39_IN   0x103E50, BIT0
#define GPIO39_DRV  0x103E50, BIT7
#define GPIO39_PE   0x000000, BIT0
#define GPIO39_PS   0x000000, BIT0

#define GPIO40_PAD  PAD_PM_LED0
#define GPIO40_NAME "PAD_PM_LED0"
#define GPIO40_OEN  0x103E52, BIT2
#define GPIO40_OUT  0x103E52, BIT1
#define GPIO40_IN   0x103E52, BIT0
#define GPIO40_DRV  0x103E52, BIT7
#define GPIO40_PE   0x000000, BIT0
#define GPIO40_PS   0x000000, BIT0

#define GPIO41_PAD  PAD_PM_LED1
#define GPIO41_NAME "PAD_PM_LED1"
#define GPIO41_OEN  0x103E54, BIT2
#define GPIO41_OUT  0x103E54, BIT1
#define GPIO41_IN   0x103E54, BIT0
#define GPIO41_DRV  0x103E54, BIT7
#define GPIO41_PE   0x000000, BIT0
#define GPIO41_PS   0x000000, BIT0

#define GPIO42_PAD  PAD_FUART_RX
#define GPIO42_NAME "PAD_FUART_RX"
#define GPIO42_OEN  0x103E62, BIT2
#define GPIO42_OUT  0x103E62, BIT1
#define GPIO42_IN   0x103E62, BIT0
#define GPIO42_DRV  0x103E62, BIT7
#define GPIO42_PE   0x103E62, BIT6
#define GPIO42_PS   0x103E62, BIT11

#define GPIO43_PAD  PAD_FUART_TX
#define GPIO43_NAME "PAD_FUART_TX"
#define GPIO43_OEN  0x103E64, BIT2
#define GPIO43_OUT  0x103E64, BIT1
#define GPIO43_IN   0x103E64, BIT0
#define GPIO43_DRV  0x103E64, BIT7
#define GPIO43_PE   0x103E64, BIT6
#define GPIO43_PS   0x103E64, BIT11

#define GPIO44_PAD  PAD_FUART_CTS
#define GPIO44_NAME "PAD_FUART_CTS"
#define GPIO44_OEN  0x103E66, BIT2
#define GPIO44_OUT  0x103E66, BIT1
#define GPIO44_IN   0x103E66, BIT0
#define GPIO44_DRV  0x103E66, BIT7
#define GPIO44_PE   0x103E66, BIT6
#define GPIO44_PS   0x103E66, BIT11

#define GPIO45_PAD  PAD_FUART_RTS
#define GPIO45_NAME "PAD_FUART_RTS"
#define GPIO45_OEN  0x103E68, BIT2
#define GPIO45_OUT  0x103E68, BIT1
#define GPIO45_IN   0x103E68, BIT0
#define GPIO45_DRV  0x103E68, BIT7
#define GPIO45_PE   0x103E68, BIT6
#define GPIO45_PS   0x103E68, BIT11

#define GPIO46_PAD  PAD_GPIO0
#define GPIO46_NAME "PAD_GPIO0"
#define GPIO46_OEN  0x103E6A, BIT2
#define GPIO46_OUT  0x103E6A, BIT1
#define GPIO46_IN   0x103E6A, BIT0
#define GPIO46_DRV  0x103E6A, BIT7
#define GPIO46_PE   0x103E6A, BIT6
#define GPIO46_PS   0x103E6A, BIT11

#define GPIO47_PAD  PAD_GPIO1
#define GPIO47_NAME "PAD_GPIO1"
#define GPIO47_OEN  0x103E6C, BIT2
#define GPIO47_OUT  0x103E6C, BIT1
#define GPIO47_IN   0x103E6C, BIT0
#define GPIO47_DRV  0x103E6C, BIT7
#define GPIO47_PE   0x103E6C, BIT6
#define GPIO47_PS   0x103E6C, BIT11

#define GPIO48_PAD  PAD_GPIO2
#define GPIO48_NAME "PAD_GPIO2"
#define GPIO48_OEN  0x103E6E, BIT2
#define GPIO48_OUT  0x103E6E, BIT1
#define GPIO48_IN   0x103E6E, BIT0
#define GPIO48_DRV  0x103E6E, BIT7
#define GPIO48_PE   0x103E6E, BIT6
#define GPIO48_PS   0x103E6E, BIT11

#define GPIO49_PAD  PAD_GPIO3
#define GPIO49_NAME "PAD_GPIO3"
#define GPIO49_OEN  0x103E70, BIT2
#define GPIO49_OUT  0x103E70, BIT1
#define GPIO49_IN   0x103E70, BIT0
#define GPIO49_DRV  0x103E70, BIT7
#define GPIO49_PE   0x103E70, BIT6
#define GPIO49_PS   0x103E70, BIT11

#define GPIO50_PAD  PAD_GPIO4
#define GPIO50_NAME "PAD_GPIO4"
#define GPIO50_OEN  0x103E72, BIT2
#define GPIO50_OUT  0x103E72, BIT1
#define GPIO50_IN   0x103E72, BIT0
#define GPIO50_DRV  0x103E72, BIT7
#define GPIO50_PE   0x103E72, BIT6
#define GPIO50_PS   0x103E72, BIT11

#define GPIO51_PAD  PAD_GPIO5
#define GPIO51_NAME "PAD_GPIO5"
#define GPIO51_OEN  0x103E74, BIT2
#define GPIO51_OUT  0x103E74, BIT1
#define GPIO51_IN   0x103E74, BIT0
#define GPIO51_DRV  0x103E74, BIT7
#define GPIO51_PE   0x103E74, BIT6
#define GPIO51_PS   0x103E74, BIT11

#define GPIO52_PAD  PAD_GPIO6
#define GPIO52_NAME "PAD_GPIO6"
#define GPIO52_OEN  0x103E76, BIT2
#define GPIO52_OUT  0x103E76, BIT1
#define GPIO52_IN   0x103E76, BIT0
#define GPIO52_DRV  0x103E76, BIT7
#define GPIO52_PE   0x103E76, BIT6
#define GPIO52_PS   0x103E76, BIT11

#define GPIO53_PAD  PAD_GPIO7
#define GPIO53_NAME "PAD_GPIO7"
#define GPIO53_OEN  0x103E78, BIT2
#define GPIO53_OUT  0x103E78, BIT1
#define GPIO53_IN   0x103E78, BIT0
#define GPIO53_DRV  0x103E78, BIT7
#define GPIO53_PE   0x103E78, BIT6
#define GPIO53_PS   0x103E78, BIT11

#define GPIO54_PAD  PAD_GPIO14
#define GPIO54_NAME "PAD_GPIO14"
#define GPIO54_OEN  0x103E7A, BIT2
#define GPIO54_OUT  0x103E7A, BIT1
#define GPIO54_IN   0x103E7A, BIT0
#define GPIO54_DRV  0x103E7A, BIT7
#define GPIO54_PE   0x103E7A, BIT6
#define GPIO54_PS   0x103E7A, BIT11

#define GPIO55_PAD  PAD_GPIO15
#define GPIO55_NAME "PAD_GPIO15"
#define GPIO55_OEN  0x103E7C, BIT2
#define GPIO55_OUT  0x103E7C, BIT1
#define GPIO55_IN   0x103E7C, BIT0
#define GPIO55_DRV  0x103E7C, BIT7
#define GPIO55_PE   0x103E7C, BIT6
#define GPIO55_PS   0x103E7C, BIT11

#define GPIO56_PAD  PAD_I2C0_SCL
#define GPIO56_NAME "PAD_I2C0_SCL"
#define GPIO56_OEN  0x103E7E, BIT2
#define GPIO56_OUT  0x103E7E, BIT1
#define GPIO56_IN   0x103E7E, BIT0
#define GPIO56_DRV  0x103E7E, BIT7
#define GPIO56_PE   0x103E7E, BIT6
#define GPIO56_PS   0x103E7E, BIT11

#define GPIO57_PAD  PAD_I2C0_SDA
#define GPIO57_NAME "PAD_I2C0_SDA"
#define GPIO57_OEN  0x103E80, BIT2
#define GPIO57_OUT  0x103E80, BIT1
#define GPIO57_IN   0x103E80, BIT0
#define GPIO57_DRV  0x103E80, BIT7
#define GPIO57_PE   0x103E80, BIT6
#define GPIO57_PS   0x103E80, BIT11

#define GPIO58_PAD  PAD_I2C1_SCL
#define GPIO58_NAME "PAD_I2C1_SCL"
#define GPIO58_OEN  0x103E82, BIT2
#define GPIO58_OUT  0x103E82, BIT1
#define GPIO58_IN   0x103E82, BIT0
#define GPIO58_DRV  0x103E82, BIT7
#define GPIO58_PE   0x103E82, BIT6
#define GPIO58_PS   0x103E82, BIT11

#define GPIO59_PAD  PAD_I2C1_SDA
#define GPIO59_NAME "PAD_I2C1_SDA"
#define GPIO59_OEN  0x103E84, BIT2
#define GPIO59_OUT  0x103E84, BIT1
#define GPIO59_IN   0x103E84, BIT0
#define GPIO59_DRV  0x103E84, BIT7
#define GPIO59_PE   0x103E84, BIT6
#define GPIO59_PS   0x103E84, BIT11

#define GPIO60_PAD  PAD_SR_IO00
#define GPIO60_NAME "PAD_SR_IO00"
#define GPIO60_OEN  0x103E86, BIT2
#define GPIO60_OUT  0x103E86, BIT1
#define GPIO60_IN   0x103E86, BIT0
#define GPIO60_DRV  0x103E86, BIT7
#define GPIO60_PE   0x103E86, BIT6
#define GPIO60_PS   0x103E86, BIT11

#define GPIO61_PAD  PAD_SR_IO01
#define GPIO61_NAME "PAD_SR_IO01"
#define GPIO61_OEN  0x103E88, BIT2
#define GPIO61_OUT  0x103E88, BIT1
#define GPIO61_IN   0x103E88, BIT0
#define GPIO61_DRV  0x103E88, BIT7
#define GPIO61_PE   0x103E88, BIT6
#define GPIO61_PS   0x103E88, BIT11

#define GPIO62_PAD  PAD_SR_IO02
#define GPIO62_NAME "PAD_SR_IO02"
#define GPIO62_OEN  0x103E8A, BIT2
#define GPIO62_OUT  0x103E8A, BIT1
#define GPIO62_IN   0x103E8A, BIT0
#define GPIO62_DRV  0x103E8A, BIT7
#define GPIO62_PE   0x103E8A, BIT6
#define GPIO62_PS   0x103E8A, BIT11

#define GPIO63_PAD  PAD_SR_IO03
#define GPIO63_NAME "PAD_SR_IO03"
#define GPIO63_OEN  0x103E8C, BIT2
#define GPIO63_OUT  0x103E8C, BIT1
#define GPIO63_IN   0x103E8C, BIT0
#define GPIO63_DRV  0x103E8C, BIT7
#define GPIO63_PE   0x103E8C, BIT6
#define GPIO63_PS   0x103E8C, BIT11

#define GPIO64_PAD  PAD_SR_IO04
#define GPIO64_NAME "PAD_SR_IO04"
#define GPIO64_OEN  0x103E8E, BIT2
#define GPIO64_OUT  0x103E8E, BIT1
#define GPIO64_IN   0x103E8E, BIT0
#define GPIO64_DRV  0x103E8E, BIT7
#define GPIO64_PE   0x103E8E, BIT6
#define GPIO64_PS   0x103E8E, BIT11

#define GPIO65_PAD  PAD_SR_IO05
#define GPIO65_NAME "PAD_SR_IO05"
#define GPIO65_OEN  0x103E90, BIT2
#define GPIO65_OUT  0x103E90, BIT1
#define GPIO65_IN   0x103E90, BIT0
#define GPIO65_DRV  0x103E90, BIT7
#define GPIO65_PE   0x103E90, BIT6
#define GPIO65_PS   0x103E90, BIT11

#define GPIO66_PAD  PAD_SR_IO06
#define GPIO66_NAME "PAD_SR_IO06"
#define GPIO66_OEN  0x103E92, BIT2
#define GPIO66_OUT  0x103E92, BIT1
#define GPIO66_IN   0x103E92, BIT0
#define GPIO66_DRV  0x103E92, BIT7
#define GPIO66_PE   0x103E92, BIT6
#define GPIO66_PS   0x103E92, BIT11

#define GPIO67_PAD  PAD_SR_IO07
#define GPIO67_NAME "PAD_SR_IO07"
#define GPIO67_OEN  0x103E94, BIT2
#define GPIO67_OUT  0x103E94, BIT1
#define GPIO67_IN   0x103E94, BIT0
#define GPIO67_DRV  0x103E94, BIT7
#define GPIO67_PE   0x103E94, BIT6
#define GPIO67_PS   0x103E94, BIT11

#define GPIO68_PAD  PAD_SR_IO08
#define GPIO68_NAME "PAD_SR_IO08"
#define GPIO68_OEN  0x103E96, BIT2
#define GPIO68_OUT  0x103E96, BIT1
#define GPIO68_IN   0x103E96, BIT0
#define GPIO68_DRV  0x103E96, BIT7
#define GPIO68_PE   0x103E96, BIT6
#define GPIO68_PS   0x103E96, BIT11

#define GPIO69_PAD  PAD_SR_IO09
#define GPIO69_NAME "PAD_SR_IO09"
#define GPIO69_OEN  0x103E98, BIT2
#define GPIO69_OUT  0x103E98, BIT1
#define GPIO69_IN   0x103E98, BIT0
#define GPIO69_DRV  0x103E98, BIT7
#define GPIO69_PE   0x103E98, BIT6
#define GPIO69_PS   0x103E98, BIT11

#define GPIO70_PAD  PAD_SR_IO10
#define GPIO70_NAME "PAD_SR_IO10"
#define GPIO70_OEN  0x103E9A, BIT2
#define GPIO70_OUT  0x103E9A, BIT1
#define GPIO70_IN   0x103E9A, BIT0
#define GPIO70_DRV  0x103E9A, BIT7
#define GPIO70_PE   0x103E9A, BIT6
#define GPIO70_PS   0x103E9A, BIT11

#define GPIO71_PAD  PAD_SR_IO11
#define GPIO71_NAME "PAD_SR_IO11"
#define GPIO71_OEN  0x103E9C, BIT2
#define GPIO71_OUT  0x103E9C, BIT1
#define GPIO71_IN   0x103E9C, BIT0
#define GPIO71_DRV  0x103E9C, BIT7
#define GPIO71_PE   0x103E9C, BIT6
#define GPIO71_PS   0x103E9C, BIT11

#define GPIO72_PAD  PAD_SR_IO12
#define GPIO72_NAME "PAD_SR_IO12"
#define GPIO72_OEN  0x103E9E, BIT2
#define GPIO72_OUT  0x103E9E, BIT1
#define GPIO72_IN   0x103E9E, BIT0
#define GPIO72_DRV  0x103E9E, BIT7
#define GPIO72_PE   0x103E9E, BIT6
#define GPIO72_PS   0x103E9E, BIT11

#define GPIO73_PAD  PAD_SR_IO13
#define GPIO73_NAME "PAD_SR_IO13"
#define GPIO73_OEN  0x103EA0, BIT2
#define GPIO73_OUT  0x103EA0, BIT1
#define GPIO73_IN   0x103EA0, BIT0
#define GPIO73_DRV  0x103EA0, BIT7
#define GPIO73_PE   0x103EA0, BIT6
#define GPIO73_PS   0x103EA0, BIT11

#define GPIO74_PAD  PAD_SR_IO14
#define GPIO74_NAME "PAD_SR_IO14"
#define GPIO74_OEN  0x103EA2, BIT2
#define GPIO74_OUT  0x103EA2, BIT1
#define GPIO74_IN   0x103EA2, BIT0
#define GPIO74_DRV  0x103EA2, BIT7
#define GPIO74_PE   0x103EA2, BIT6
#define GPIO74_PS   0x103EA2, BIT11

#define GPIO75_PAD  PAD_SR_IO15
#define GPIO75_NAME "PAD_SR_IO15"
#define GPIO75_OEN  0x103EA4, BIT2
#define GPIO75_OUT  0x103EA4, BIT1
#define GPIO75_IN   0x103EA4, BIT0
#define GPIO75_DRV  0x103EA4, BIT7
#define GPIO75_PE   0x103EA4, BIT6
#define GPIO75_PS   0x103EA4, BIT11

#define GPIO76_PAD  PAD_SR_IO16
#define GPIO76_NAME "PAD_SR_IO16"
#define GPIO76_OEN  0x103EA6, BIT2
#define GPIO76_OUT  0x103EA6, BIT1
#define GPIO76_IN   0x103EA6, BIT0
#define GPIO76_DRV  0x103EA6, BIT7
#define GPIO76_PE   0x103EA6, BIT6
#define GPIO76_PS   0x103EA6, BIT11

#define GPIO77_PAD  PAD_SR_IO17
#define GPIO77_NAME "PAD_SR_IO17"
#define GPIO77_OEN  0x103EA8, BIT2
#define GPIO77_OUT  0x103EA8, BIT1
#define GPIO77_IN   0x103EA8, BIT0
#define GPIO77_DRV  0x103EA8, BIT7
#define GPIO77_PE   0x103EA8, BIT6
#define GPIO77_PS   0x103EA8, BIT11

#define GPIO78_PAD  PAD_SAR_GPIO3
#define GPIO78_NAME "PAD_SAR_GPIO3"
#define GPIO78_OEN  0x103EAA, BIT2
#define GPIO78_OUT  0x103EAA, BIT1
#define GPIO78_IN   0x103EAA, BIT0
#define GPIO78_DRV  0x000000, BIT0
#define GPIO78_PE   0x000000, BIT0
#define GPIO78_PS   0x000000, BIT0

#define GPIO79_PAD  PAD_SAR_GPIO2
#define GPIO79_NAME "PAD_SAR_GPIO2"
#define GPIO79_OEN  0x103EAC, BIT2
#define GPIO79_OUT  0x103EAC, BIT1
#define GPIO79_IN   0x103EAC, BIT0
#define GPIO79_DRV  0x000000, BIT0
#define GPIO79_PE   0x000000, BIT0
#define GPIO79_PS   0x000000, BIT0

#define GPIO80_PAD  PAD_SAR_GPIO1
#define GPIO80_NAME "PAD_SAR_GPIO1"
#define GPIO80_OEN  0x103EAE, BIT2
#define GPIO80_OUT  0x103EAE, BIT1
#define GPIO80_IN   0x103EAE, BIT0
#define GPIO80_DRV  0x000000, BIT0
#define GPIO80_PE   0x000000, BIT0
#define GPIO80_PS   0x000000, BIT0

#define GPIO81_PAD  PAD_SAR_GPIO0
#define GPIO81_NAME "PAD_SAR_GPIO0"
#define GPIO81_OEN  0x103EB0, BIT2
#define GPIO81_OUT  0x103EB0, BIT1
#define GPIO81_IN   0x103EB0, BIT0
#define GPIO81_DRV  0x000000, BIT0
#define GPIO81_PE   0x000000, BIT0
#define GPIO81_PS   0x000000, BIT0

// add manually for misc pads here
#define GPIO82_PAD  PAD_ETH_RN
#define GPIO82_NAME "PAD_ETH_RN"
#define GPIO82_OEN  0x151AE2, BIT4
#define GPIO82_OUT  0x151AE4, BIT0
#define GPIO82_IN   0x151AE4, BIT4
#define GPIO82_DRV  0x000000, BIT0
#define GPIO82_PE   0x000000, BIT0
#define GPIO82_PS   0x000000, BIT0

#define GPIO83_PAD  PAD_ETH_RP
#define GPIO83_NAME "PAD_ETH_RP"
#define GPIO83_OEN  0x151AE2, BIT5
#define GPIO83_OUT  0x151AE4, BIT1
#define GPIO83_IN   0x151AE4, BIT5
#define GPIO83_DRV  0x000000, BIT0
#define GPIO83_PE   0x000000, BIT0
#define GPIO83_PS   0x000000, BIT0

#define GPIO84_PAD  PAD_ETH_TN
#define GPIO84_NAME "PAD_ETH_TN"
#define GPIO84_OEN  0x151AE2, BIT6
#define GPIO84_OUT  0x151AE4, BIT2
#define GPIO84_IN   0x151AE4, BIT6
#define GPIO84_DRV  0x000000, BIT0
#define GPIO84_PE   0x000000, BIT0
#define GPIO84_PS   0x000000, BIT0

#define GPIO85_PAD  PAD_ETH_TP
#define GPIO85_NAME "PAD_ETH_TP"
#define GPIO85_OEN  0x151AE2, BIT7
#define GPIO85_OUT  0x151AE4, BIT3
#define GPIO85_IN   0x151AE4, BIT7
#define GPIO85_DRV  0x000000, BIT0
#define GPIO85_PE   0x000000, BIT0
#define GPIO85_PS   0x000000, BIT0

U32 gChipBaseAddr    = 0xFD203C00;
U32 gPmSleepBaseAddr = 0xFD001C00;
U32 gSarBaseAddr     = 0xFD002800;
U32 gRIUBaseAddr     = 0xFD000000;

#define MHal_CHIPTOP_REG(addr)  (*(volatile U16 *)(gChipBaseAddr + (addr << 1)))
#define MHal_PM_SLEEP_REG(addr) (*(volatile U16 *)(gPmSleepBaseAddr + (addr << 1)))
#define MHal_SAR_GPIO_REG(addr) (*(volatile U16 *)(gSarBaseAddr + (addr << 1)))
#define MHal_RIU_REG(addr)      (*(volatile U16 *)(gRIUBaseAddr + (addr << 1)))

#define REG_ALL_PAD_IN 0xA1

//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------

static int _gpi_to_irq_table[] = {

    INT_GPI_FIQ_PAD_SD0_CDZ,    INT_GPI_FIQ_PAD_SD0_D1,     INT_GPI_FIQ_PAD_SD0_D0,    INT_GPI_FIQ_PAD_SD0_CLK,
    INT_GPI_FIQ_PAD_SD0_CMD,    INT_GPI_FIQ_PAD_SD0_D3,     INT_GPI_FIQ_PAD_SD0_D2,    INT_GPI_FIQ_PAD_UART1_RX,
    INT_GPI_FIQ_PAD_UART1_TX,   INT_GPI_FIQ_PAD_SPI0_CZ,    INT_GPI_FIQ_PAD_SPI0_CK,   INT_GPI_FIQ_PAD_SPI0_DI,
    INT_GPI_FIQ_PAD_SPI0_DO,    INT_GPI_FIQ_PAD_PWM0,       INT_GPI_FIQ_PAD_PWM1,      INT_GPI_FIQ_PAD_SD_CLK,
    INT_GPI_FIQ_PAD_SD_CMD,     INT_GPI_FIQ_PAD_SD_D0,      INT_GPI_FIQ_PAD_SD_D1,     INT_GPI_FIQ_PAD_SD_D2,
    INT_GPI_FIQ_PAD_SD_D3,      INT_GPI_FIQ_PAD_USB_CID,    INT_GPI_FIQ_PAD_PM_SD_CDZ, INT_GPI_FIQ_PAD_PM_IRIN,
    INT_GPI_FIQ_PAD_PM_UART_RX, INT_GPI_FIQ_PAD_PM_UART_TX, INT_GPI_FIQ_PAD_PM_GPIO0,  INT_GPI_FIQ_PAD_PM_GPIO1,
    INT_GPI_FIQ_PAD_PM_GPIO2,   INT_GPI_FIQ_PAD_PM_GPIO3,   INT_GPI_FIQ_PAD_PM_GPIO4,  INT_GPI_FIQ_PAD_PM_GPIO7,
    INT_GPI_FIQ_PAD_PM_GPIO8,   INT_GPI_FIQ_PAD_PM_GPIO9,   INT_GPI_FIQ_PAD_PM_SPI_CZ, INT_GPI_FIQ_PAD_PM_SPI_DI,
    INT_GPI_FIQ_PAD_PM_SPI_WPZ, INT_GPI_FIQ_PAD_PM_SPI_DO,  INT_GPI_FIQ_PAD_PM_SPI_CK, INT_GPI_FIQ_PAD_PM_SPI_HLD,
    INT_GPI_FIQ_PAD_PM_LED0,    INT_GPI_FIQ_PAD_PM_LED1,    INT_GPI_FIQ_PAD_FUART_RX,  INT_GPI_FIQ_PAD_FUART_TX,
    INT_GPI_FIQ_PAD_FUART_CTS,  INT_GPI_FIQ_PAD_FUART_RTS,  INT_GPI_FIQ_PAD_GPIO0,     INT_GPI_FIQ_PAD_GPIO1,
    INT_GPI_FIQ_PAD_GPIO2,      INT_GPI_FIQ_PAD_GPIO3,      INT_GPI_FIQ_PAD_GPIO4,     INT_GPI_FIQ_PAD_GPIO5,
    INT_GPI_FIQ_PAD_GPIO6,      INT_GPI_FIQ_PAD_GPIO7,      INT_GPI_FIQ_PAD_GPIO14,    INT_GPI_FIQ_PAD_GPIO15,
    INT_GPI_FIQ_PAD_I2C0_SCL,   INT_GPI_FIQ_PAD_I2C0_SDA,   INT_GPI_FIQ_PAD_I2C1_SCL,  INT_GPI_FIQ_PAD_I2C1_SDA,
    INT_GPI_FIQ_PAD_SR_IO00,    INT_GPI_FIQ_PAD_SR_IO01,    INT_GPI_FIQ_PAD_SR_IO02,   INT_GPI_FIQ_PAD_SR_IO03,
    INT_GPI_FIQ_PAD_SR_IO04,    INT_GPI_FIQ_PAD_SR_IO05,    INT_GPI_FIQ_PAD_SR_IO06,   INT_GPI_FIQ_PAD_SR_IO07,
    INT_GPI_FIQ_PAD_SR_IO08,    INT_GPI_FIQ_PAD_SR_IO09,    INT_GPI_FIQ_PAD_SR_IO10,   INT_GPI_FIQ_PAD_SR_IO11,
    INT_GPI_FIQ_PAD_SR_IO12,    INT_GPI_FIQ_PAD_SR_IO13,    INT_GPI_FIQ_PAD_SR_IO14,   INT_GPI_FIQ_PAD_SR_IO15,
    INT_GPI_FIQ_PAD_SR_IO16,    INT_GPI_FIQ_PAD_SR_IO17,    INT_GPI_FIQ_PAD_SAR_GPIO3, INT_GPI_FIQ_PAD_SAR_GPIO2,
    INT_GPI_FIQ_PAD_SAR_GPIO1,  INT_GPI_FIQ_PAD_SAR_GPIO0

};

static const struct gpio_setting
{
    U8  p_name[32];
    U32 r_oen;
    U16 m_oen;
    U32 r_out;
    U16 m_out;
    U32 r_in;
    U16 m_in;
    U32 r_drv;
    U16 m_drv;
    U32 r_pe;
    U16 m_pe;
    U32 r_ps;
    U16 m_ps;
} gpio_table[] = {
#define __GPIO__(_x_)                                                                                        \
    {                                                                                                        \
        CONCAT(CONCAT(GPIO, _x_), _NAME), CONCAT(CONCAT(GPIO, _x_), _OEN), CONCAT(CONCAT(GPIO, _x_), _OUT),  \
            CONCAT(CONCAT(GPIO, _x_), _IN), CONCAT(CONCAT(GPIO, _x_), _DRV), CONCAT(CONCAT(GPIO, _x_), _PE), \
            CONCAT(CONCAT(GPIO, _x_), _PS)                                                                   \
    }
#define __GPIO(_x_) __GPIO__(_x_)

    //
    // !! WARNING !! DO NOT MODIFIY !!!!
    //
    // These defines order must match following
    // 1. the PAD name in GPIO excel
    // 2. the perl script to generate the package header file
    //

    __GPIO(0),  __GPIO(1),  __GPIO(2),  __GPIO(3),  __GPIO(4),  __GPIO(5),  __GPIO(6),  __GPIO(7),  __GPIO(8),
    __GPIO(9),  __GPIO(10), __GPIO(11), __GPIO(12), __GPIO(13), __GPIO(14), __GPIO(15), __GPIO(16), __GPIO(17),
    __GPIO(18), __GPIO(19), __GPIO(20), __GPIO(21), __GPIO(22), __GPIO(23), __GPIO(24), __GPIO(25), __GPIO(26),
    __GPIO(27), __GPIO(28), __GPIO(29), __GPIO(30), __GPIO(31), __GPIO(32), __GPIO(33), __GPIO(34), __GPIO(35),
    __GPIO(36), __GPIO(37), __GPIO(38), __GPIO(39), __GPIO(40), __GPIO(41), __GPIO(42), __GPIO(43), __GPIO(44),
    __GPIO(45), __GPIO(46), __GPIO(47), __GPIO(48), __GPIO(49), __GPIO(50), __GPIO(51), __GPIO(52), __GPIO(53),
    __GPIO(54), __GPIO(55), __GPIO(56), __GPIO(57), __GPIO(58), __GPIO(59), __GPIO(60), __GPIO(61), __GPIO(62),
    __GPIO(63), __GPIO(64), __GPIO(65), __GPIO(66), __GPIO(67), __GPIO(68), __GPIO(69), __GPIO(70), __GPIO(71),
    __GPIO(72), __GPIO(73), __GPIO(74), __GPIO(75), __GPIO(76), __GPIO(77), __GPIO(78), __GPIO(79), __GPIO(80),
    __GPIO(81), __GPIO(82), __GPIO(83), __GPIO(84), __GPIO(85),

};

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------

void MHal_GPIO_Init(void)
{
    MHal_CHIPTOP_REG(REG_ALL_PAD_IN) &= ~BIT7;
}

void MHal_GPIO_Pad_Set(U8 u8IndexGPIO)
{
    HalPadSetVal(u8IndexGPIO, PINMUX_FOR_GPIO_MODE);
}

U8 MHal_GPIO_PadGroupMode_Set(U32 u32PadMode)
{
    return HalPadSetMode(u32PadMode);
}

U8 MHal_GPIO_PadVal_Set(U8 u8IndexGPIO, U32 u32PadMode)
{
    return HalPadSetVal((U32)u8IndexGPIO, u32PadMode);
}

U8 MHal_GPIO_PadVal_Get(U8 u8IndexGPIO, U32 *u32PadMode)
{
    return HalPadGetVal((U32)u8IndexGPIO, u32PadMode);
}

void MHal_GPIO_VolVal_Set(U8 u8Group, U32 u32Mode)
{
    HalGPIOSetVol((U32)u8Group, u32Mode);
}

U8 MHal_GPIO_PadVal_Check(U8 u8IndexGPIO, U32 u32PadMode)
{
    return HalPadCheckVal((U32)u8IndexGPIO, u32PadMode);
}

U8 MHal_GPIO_Pad_Oen(U8 u8IndexGPIO)
{
    U32 u32PadMode;
    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    HalPadGetVal(u8IndexGPIO, &u32PadMode);
    if (u32PadMode == PINMUX_FOR_GPIO_MODE)
    {
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_oen) &= (~gpio_table[u8IndexGPIO].m_oen);
        return 0;
    }
    else
    {
        GPIO_ERR("GPIO[%d] not in GPIO MODE\n", u8IndexGPIO);
        return 1;
    }
}

U8 MHal_GPIO_Pad_Odn(U8 u8IndexGPIO)
{
    U32 u32PadMode;
    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    HalPadGetVal(u8IndexGPIO, &u32PadMode);
    if (u32PadMode == PINMUX_FOR_GPIO_MODE)
    {
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_oen) |= gpio_table[u8IndexGPIO].m_oen;
        return 0;
    }
    else
    {
        GPIO_ERR("GPIO[%d] not in GPIO MODE\n", u8IndexGPIO);
        return 1;
    }
}

U8 MHal_GPIO_Pad_Level(U8 u8IndexGPIO, U8 *u8PadLevel)
{
    U32 u32PadMode;
    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    HalPadGetVal(u8IndexGPIO, &u32PadMode);
    if (u32PadMode == PINMUX_FOR_GPIO_MODE)
    {
        *u8PadLevel = ((MHal_RIU_REG(gpio_table[u8IndexGPIO].r_in) & gpio_table[u8IndexGPIO].m_in) ? 1 : 0);
        return 0;
    }
    else
    {
        GPIO_ERR("GPIO[%d] not in GPIO MODE\n", u8IndexGPIO);
        return 1;
    }
}

U8 MHal_GPIO_Pad_InOut(U8 u8IndexGPIO, U8 *u8PadInOut)
{
    U32 u32PadMode;
    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    HalPadGetVal(u8IndexGPIO, &u32PadMode);
    if (u32PadMode == PINMUX_FOR_GPIO_MODE)
    {
        *u8PadInOut = ((MHal_RIU_REG(gpio_table[u8IndexGPIO].r_oen) & gpio_table[u8IndexGPIO].m_oen) ? 1 : 0);
        return 0;
    }
    else
    {
        GPIO_ERR("GPIO[%d] not in GPIO MODE\n", u8IndexGPIO);
        return 1;
    }
}

U8 MHal_GPIO_Pull_High(U8 u8IndexGPIO)
{
    U32 u32PadMode;
    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    HalPadGetVal(u8IndexGPIO, &u32PadMode);
    if (u32PadMode == PINMUX_FOR_GPIO_MODE)
    {
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_out) |= gpio_table[u8IndexGPIO].m_out;
        return 0;
    }
    else
    {
        GPIO_ERR("GPIO[%d] not in GPIO MODE\n", u8IndexGPIO);
        return 1;
    }
}

U8 MHal_GPIO_Pull_Low(U8 u8IndexGPIO)
{
    U32 u32PadMode;
    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    HalPadGetVal(u8IndexGPIO, &u32PadMode);
    if (u32PadMode == PINMUX_FOR_GPIO_MODE)
    {
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_out) &= (~gpio_table[u8IndexGPIO].m_out);
        return 0;
    }
    else
    {
        GPIO_ERR("GPIO[%d] not in GPIO MODE\n", u8IndexGPIO);
        return 1;
    }
}

U8 MHal_GPIO_Pull_Up(U8 u8IndexGPIO)
{
    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    else if (gpio_table[u8IndexGPIO].r_pe)
    {
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_pe) |= gpio_table[u8IndexGPIO].m_pe;
        if (gpio_table[u8IndexGPIO].r_ps)
        {
            MHal_RIU_REG(gpio_table[u8IndexGPIO].r_ps) |= gpio_table[u8IndexGPIO].m_ps;
        }
    }
    else
    {
        return 1; // no support pull up
    }
    return 0;
}

U8 MHal_GPIO_Pull_Down(U8 u8IndexGPIO)
{
    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    else if (gpio_table[u8IndexGPIO].r_pe)
    {
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_pe) |= gpio_table[u8IndexGPIO].m_pe;
        if (gpio_table[u8IndexGPIO].r_ps)
        {
            MHal_RIU_REG(gpio_table[u8IndexGPIO].r_ps) &= ~gpio_table[u8IndexGPIO].m_ps;
        }
    }
    else
    {
        return 1; // no support pull down
    }
    return 0;
}

U8 MHal_GPIO_Pull_Off(U8 u8IndexGPIO)
{
    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    else if (gpio_table[u8IndexGPIO].r_pe)
    {
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_pe) &= ~gpio_table[u8IndexGPIO].m_pe;
    }
    else
    {
        return 1; // no support pull enable
    }
    return 0;
}

U8 MHal_GPIO_Pull_status(U8 u8IndexGPIO, U8 *u8PullStatus)
{
    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    else if (gpio_table[u8IndexGPIO].r_pe)
    {
        if (MHal_RIU_REG(gpio_table[u8IndexGPIO].r_pe) & gpio_table[u8IndexGPIO].m_pe)
        {
            if (MHal_RIU_REG(gpio_table[u8IndexGPIO].r_ps) & gpio_table[u8IndexGPIO].m_ps)
                *u8PullStatus = MHAL_PULL_UP;
            else
                *u8PullStatus = MHAL_PULL_DOWN;
        }
        else
            *u8PullStatus = MHAL_PULL_OFF;
    }
    else
    {
        return 1;
    }
    return 0;
}

U8 MHal_GPIO_Set_High(U8 u8IndexGPIO)
{
    U32 u32PadMode;
    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    HalPadGetVal(u8IndexGPIO, &u32PadMode);
    if (u32PadMode == PINMUX_FOR_GPIO_MODE)
    {
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_oen) &= (~gpio_table[u8IndexGPIO].m_oen);
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_out) |= gpio_table[u8IndexGPIO].m_out;
        return 0;
    }
    else
    {
        GPIO_ERR("GPIO[%d] not in GPIO MODE\n", u8IndexGPIO);
        return 1;
    }
}

U8 MHal_GPIO_Set_Low(U8 u8IndexGPIO)
{
    U32 u32PadMode;
    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    HalPadGetVal(u8IndexGPIO, &u32PadMode);
    if (u32PadMode == PINMUX_FOR_GPIO_MODE)
    {
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_oen) &= (~gpio_table[u8IndexGPIO].m_oen);
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_out) &= (~gpio_table[u8IndexGPIO].m_out);
        return 0;
    }
    else
    {
        GPIO_ERR("GPIO[%d] not in GPIO MODE\n", u8IndexGPIO);
        return 1;
    }
}

U8 MHal_GPIO_Drv_Set(U8 u8IndexGPIO, U8 u8Level)
{
    U8  u8Lsb   = 0;
    U16 u16Mask = 0;

    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    else if (gpio_table[u8IndexGPIO].r_drv)
    {
        u16Mask = gpio_table[u8IndexGPIO].m_drv;

        if (!u16Mask)
        {
            return 1;
        }

        // Calculate LSB by dichotomy
        if ((u16Mask & 0xFF) == 0)
        {
            u16Mask >>= 8;
            u8Lsb += 8;
        }
        if ((u16Mask & 0xF) == 0)
        {
            u16Mask >>= 4;
            u8Lsb += 4;
        }
        if ((u16Mask & 0x3) == 0)
        {
            u16Mask >>= 2;
            u8Lsb += 2;
        }
        if ((u16Mask & 0x1) == 0)
        {
            u8Lsb += 1;
        }

        if (u8Level > (gpio_table[u8IndexGPIO].m_drv >> u8Lsb))
        {
            return 1;
        }
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_drv) &= ~gpio_table[u8IndexGPIO].m_drv;
        MHal_RIU_REG(gpio_table[u8IndexGPIO].r_drv) |= ((u8Level << u8Lsb) & gpio_table[u8IndexGPIO].m_drv);
    }
    else
    {
        return 1; // no support set driving
    }
    return 0;
}

U8 MHal_GPIO_Drv_Get(U8 u8IndexGPIO, U8 *u8Level)
{
    U8  u8Lsb   = 0;
    U16 u16Mask = 0;

    if (u8IndexGPIO >= GPIO_NR)
    {
        return 1;
    }
    else if (gpio_table[u8IndexGPIO].r_drv)
    {
        u16Mask = gpio_table[u8IndexGPIO].m_drv;

        // Calculate LSB by dichotomy
        if ((u16Mask & 0xFF) == 0)
        {
            u16Mask >>= 8;
            u8Lsb += 8;
        }
        if ((u16Mask & 0xF) == 0)
        {
            u16Mask >>= 4;
            u8Lsb += 4;
        }
        if ((u16Mask & 0x3) == 0)
        {
            u16Mask >>= 2;
            u8Lsb += 2;
        }
        if ((u16Mask & 0x1) == 0)
        {
            u8Lsb += 1;
        }
        *u8Level = ((MHal_RIU_REG(gpio_table[u8IndexGPIO].r_drv) & gpio_table[u8IndexGPIO].m_drv) >> u8Lsb);
        return 0;
    }
    else
    {
        return 1;
    }
}

static int PMSLEEP_GPIO_To_Irq(U8 u8IndexGPIO)
{
    return -1;
}

int GPI_GPIO_To_Irq(U8 u8IndexGPIO)
{
    if ((u8IndexGPIO < PAD_SD1_IO1) || (u8IndexGPIO > PAD_SAR_GPIO0))
        return -1;
    else
        return _gpi_to_irq_table[u8IndexGPIO - PAD_SD1_IO1];
}

// MHal_GPIO_To_Irq return any virq
int MHal_GPIO_To_Irq(U8 u8IndexGPIO)
{
    struct device_node *intr_node;
    struct irq_domain * intr_domain;
    struct irq_fwspec   fwspec;
    int                 hwirq, virq = -1;

    if ((hwirq = PMSLEEP_GPIO_To_Irq(u8IndexGPIO)) >= 0)
    {
        // get virtual irq number for request_irq
        intr_node   = of_find_compatible_node(NULL, NULL, "sstar,pm-intc");
        intr_domain = irq_find_host(intr_node);
        if (!intr_domain)
            return -ENXIO;

        fwspec.param_count = 1;
        fwspec.param[0]    = hwirq;
        fwspec.fwnode      = of_node_to_fwnode(intr_node);
        virq               = irq_create_fwspec_mapping(&fwspec);
    }
    else if ((hwirq = GPI_GPIO_To_Irq(u8IndexGPIO)) >= 0)
    {
        // get virtual irq number for request_irq
        intr_node   = of_find_compatible_node(NULL, NULL, "sstar,gpi-intc");
        intr_domain = irq_find_host(intr_node);
        if (!intr_domain)
            return -ENXIO;

        fwspec.param_count = 1;
        fwspec.param[0]    = hwirq;
        fwspec.fwnode      = of_node_to_fwnode(intr_node);
        virq               = irq_create_fwspec_mapping(&fwspec);
    }

    return virq;
}

U8 MHal_GPIO_Get_CheckCount(void)
{
    return HalPadCheckInfoCount();
}

void *MHal_GPIO_Get_CheckInfo(U8 u8Index)
{
    return HalPadCheckInfoGet(u8Index);
}

U8 MHal_GPIO_NameToNum(U8 *pu8Name, U8 *GpioIndex)
{
    u8 index;
    for (index = 0; index < (sizeof(gpio_table) / sizeof(gpio_table[0])); index++)
    {
        if (!strcmp(gpio_table[index].p_name, pu8Name))
        {
            *GpioIndex = index;
            return 0;
        }
    }
    return 1;
}

U8 MHal_GPIO_PadModeToVal(U8 *pu8Mode, U8 *u8Index)
{
    return HalPadModeToVal(pu8Mode, u8Index);
}

U32 *MHal_GPIO_PadModeToPadIndex(U32 u32Mode)
{
    return HalPadModeToPadIndex(u32Mode);
}

int MHal_GPIO_Set_Debounce(U8 u8IndexGPIO, U32 debounce)
{
    U32 hwirq;
    U32 glhrm_num;

    if ((debounce > 170584) || ((debounce < 83)))
    {
        printk("[%s] Gpio%d's debounce is out of range\r\n", __FUNCTION__, u8IndexGPIO);
        return -EINVAL;
    }

    if ((u8IndexGPIO >= PAD_SD1_IO1) && (u8IndexGPIO <= PAD_SAR_GPIO0))
    {
        hwirq = GPI_GPIO_To_Irq(u8IndexGPIO);
        SETREG16((BASE_REG_GPI_INT2_PA + REG_ID_00 + (hwirq / 16) * 4), (1 << (hwirq % 16)));
        glhrm_num = debounce * 12 / 1000;
        OUTREG16((BASE_REG_GPI_INT2_PA + REG_ID_10), glhrm_num);
        return 0;
    }
    else
    {
        return -EINVAL;
    }
}
