/*
 * gpi-irqs.h- Sigmastar
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
/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/

#define GPI_FIQ_START 0

#define INT_GPI_FIQ_PAD_SD0_CDZ   (GPI_FIQ_START + 0)
#define INT_GPI_FIQ_PAD_SD0_D1    (GPI_FIQ_START + 1)
#define INT_GPI_FIQ_PAD_SD0_D0    (GPI_FIQ_START + 2)
#define INT_GPI_FIQ_PAD_SD0_CLK   (GPI_FIQ_START + 3)
#define INT_GPI_FIQ_PAD_SD0_CMD   (GPI_FIQ_START + 4)
#define INT_GPI_FIQ_PAD_SD0_D3    (GPI_FIQ_START + 5)
#define INT_GPI_FIQ_PAD_SD0_D2    (GPI_FIQ_START + 6)
#define INT_GPI_FIQ_PAD_UART1_RX  (GPI_FIQ_START + 7)
#define INT_GPI_FIQ_PAD_UART1_TX  (GPI_FIQ_START + 8)
#define INT_GPI_FIQ_PAD_SPI0_CZ   (GPI_FIQ_START + 9)
#define INT_GPI_FIQ_PAD_SPI0_CK   (GPI_FIQ_START + 10)
#define INT_GPI_FIQ_PAD_SPI0_DI   (GPI_FIQ_START + 11)
#define INT_GPI_FIQ_PAD_SPI0_DO   (GPI_FIQ_START + 12)
#define INT_GPI_FIQ_PAD_PWM0      (GPI_FIQ_START + 13)
#define INT_GPI_FIQ_PAD_PWM1      (GPI_FIQ_START + 14)
#define INT_GPI_FIQ_PAD_SD_CLK    (GPI_FIQ_START + 15)
#define INT_GPI_FIQ_PAD_SD_CMD    (GPI_FIQ_START + 16)
#define INT_GPI_FIQ_PAD_SD_D0     (GPI_FIQ_START + 17)
#define INT_GPI_FIQ_PAD_SD_D1     (GPI_FIQ_START + 18)
#define INT_GPI_FIQ_PAD_SD_D2     (GPI_FIQ_START + 19)
#define INT_GPI_FIQ_PAD_SD_D3     (GPI_FIQ_START + 20)
#define INT_GPI_FIQ_PAD_USB_CID   (GPI_FIQ_START + 21)
#define INT_GPI_FIQ_PAD_PM_SD_CDZ (GPI_FIQ_START + 22)
#define INT_GPI_FIQ_PAD_PM_IRIN   (GPI_FIQ_START + 23)
// the irq num is not continuous completely ,24 are not used temporary
#define INT_GPI_FIQ_PAD_PM_UART_RX (GPI_FIQ_START + 25)
#define INT_GPI_FIQ_PAD_PM_UART_TX (GPI_FIQ_START + 26)
#define INT_GPI_FIQ_PAD_PM_GPIO0   (GPI_FIQ_START + 27)
#define INT_GPI_FIQ_PAD_PM_GPIO1   (GPI_FIQ_START + 28)
#define INT_GPI_FIQ_PAD_PM_GPIO2   (GPI_FIQ_START + 29)
#define INT_GPI_FIQ_PAD_PM_GPIO3   (GPI_FIQ_START + 30)
#define INT_GPI_FIQ_PAD_PM_GPIO4   (GPI_FIQ_START + 31)
#define INT_GPI_FIQ_PAD_PM_GPIO7   (GPI_FIQ_START + 32)
#define INT_GPI_FIQ_PAD_PM_GPIO8   (GPI_FIQ_START + 33)
#define INT_GPI_FIQ_PAD_PM_GPIO9   (GPI_FIQ_START + 34)
#define INT_GPI_FIQ_PAD_PM_SPI_CZ  (GPI_FIQ_START + 35)
#define INT_GPI_FIQ_PAD_PM_SPI_DI  (GPI_FIQ_START + 36)
#define INT_GPI_FIQ_PAD_PM_SPI_WPZ (GPI_FIQ_START + 37)
#define INT_GPI_FIQ_PAD_PM_SPI_DO  (GPI_FIQ_START + 38)
#define INT_GPI_FIQ_PAD_PM_SPI_CK  (GPI_FIQ_START + 39)
#define INT_GPI_FIQ_PAD_PM_SPI_HLD (GPI_FIQ_START + 40)
#define INT_GPI_FIQ_PAD_PM_LED0    (GPI_FIQ_START + 41)
#define INT_GPI_FIQ_PAD_PM_LED1    (GPI_FIQ_START + 42)
// the irq num is not continuous completely ,43-48 are not used temporary
#define INT_GPI_FIQ_PAD_FUART_RX  (GPI_FIQ_START + 49)
#define INT_GPI_FIQ_PAD_FUART_TX  (GPI_FIQ_START + 50)
#define INT_GPI_FIQ_PAD_FUART_CTS (GPI_FIQ_START + 51)
#define INT_GPI_FIQ_PAD_FUART_RTS (GPI_FIQ_START + 52)
#define INT_GPI_FIQ_PAD_GPIO0     (GPI_FIQ_START + 53)
#define INT_GPI_FIQ_PAD_GPIO1     (GPI_FIQ_START + 54)
#define INT_GPI_FIQ_PAD_GPIO2     (GPI_FIQ_START + 55)
#define INT_GPI_FIQ_PAD_GPIO3     (GPI_FIQ_START + 56)
#define INT_GPI_FIQ_PAD_GPIO4     (GPI_FIQ_START + 57)
#define INT_GPI_FIQ_PAD_GPIO5     (GPI_FIQ_START + 58)
#define INT_GPI_FIQ_PAD_GPIO6     (GPI_FIQ_START + 59)
#define INT_GPI_FIQ_PAD_GPIO7     (GPI_FIQ_START + 60)
#define INT_GPI_FIQ_PAD_GPIO14    (GPI_FIQ_START + 61)
#define INT_GPI_FIQ_PAD_GPIO15    (GPI_FIQ_START + 62)
#define INT_GPI_FIQ_PAD_I2C0_SCL  (GPI_FIQ_START + 63)
#define INT_GPI_FIQ_PAD_I2C0_SDA  (GPI_FIQ_START + 64)
#define INT_GPI_FIQ_PAD_I2C1_SCL  (GPI_FIQ_START + 65)
#define INT_GPI_FIQ_PAD_I2C1_SDA  (GPI_FIQ_START + 66)
#define INT_GPI_FIQ_PAD_SR_IO00   (GPI_FIQ_START + 67)
#define INT_GPI_FIQ_PAD_SR_IO01   (GPI_FIQ_START + 68)
#define INT_GPI_FIQ_PAD_SR_IO02   (GPI_FIQ_START + 69)
#define INT_GPI_FIQ_PAD_SR_IO03   (GPI_FIQ_START + 70)
#define INT_GPI_FIQ_PAD_SR_IO04   (GPI_FIQ_START + 71)
#define INT_GPI_FIQ_PAD_SR_IO05   (GPI_FIQ_START + 72)
#define INT_GPI_FIQ_PAD_SR_IO06   (GPI_FIQ_START + 73)
#define INT_GPI_FIQ_PAD_SR_IO07   (GPI_FIQ_START + 74)
#define INT_GPI_FIQ_PAD_SR_IO08   (GPI_FIQ_START + 75)
#define INT_GPI_FIQ_PAD_SR_IO09   (GPI_FIQ_START + 76)
#define INT_GPI_FIQ_PAD_SR_IO10   (GPI_FIQ_START + 77)
#define INT_GPI_FIQ_PAD_SR_IO11   (GPI_FIQ_START + 78)
#define INT_GPI_FIQ_PAD_SR_IO12   (GPI_FIQ_START + 79)
#define INT_GPI_FIQ_PAD_SR_IO13   (GPI_FIQ_START + 80)
#define INT_GPI_FIQ_PAD_SR_IO14   (GPI_FIQ_START + 81)
#define INT_GPI_FIQ_PAD_SR_IO15   (GPI_FIQ_START + 82)
#define INT_GPI_FIQ_PAD_SR_IO16   (GPI_FIQ_START + 83)
#define INT_GPI_FIQ_PAD_SR_IO17   (GPI_FIQ_START + 84)
#define INT_GPI_FIQ_PAD_SAR_GPIO3 (GPI_FIQ_START + 85)
#define INT_GPI_FIQ_PAD_SAR_GPIO2 (GPI_FIQ_START + 86)
#define INT_GPI_FIQ_PAD_SAR_GPIO1 (GPI_FIQ_START + 87)
#define INT_GPI_FIQ_PAD_SAR_GPIO0 (GPI_FIQ_START + 88)
#define GPI_FIQ_END               (GPI_FIQ_START + 89)

#define GPI_GPIC_START    (GPI_FIQ_START + 89)
#define INT_GPI_FIQ_GPIC0 (GPI_FIQ_START + 90)
#define INT_GPI_FIQ_GPIC1 (GPI_FIQ_START + 91)
#define GPI_GPIC_END      (GPI_FIQ_START + 92)

#define GPI_FIQ_NUM  (GPI_FIQ_END - GPI_FIQ_START)
#define GPI_GPIC_NUM (GPI_GPIC_END - GPI_GPIC_START)

#define GPI_IRQ_START       0
#define INT_GPI_IRQ_DUMMY00 (GPI_IRQ_START + 0)
#define INT_GPI_IRQ_DUMMY01 (GPI_IRQ_START + 1)
#define INT_GPI_IRQ_DUMMY02 (GPI_IRQ_START + 2)
#define INT_GPI_IRQ_DUMMY03 (GPI_IRQ_START + 3)
#define INT_GPI_IRQ_DUMMY04 (GPI_IRQ_START + 4)
#define INT_GPI_IRQ_DUMMY05 (GPI_IRQ_START + 5)
#define INT_GPI_IRQ_DUMMY06 (GPI_IRQ_START + 6)
#define INT_GPI_IRQ_DUMMY07 (GPI_IRQ_START + 7)
#define GPI_IRQ_END         (GPI_IRQ_START + 8)
#define GPI_IRQ_NUM         (GPI_IRQ_END - GPI_IRQ_START)
