/*
 * Driver for AK3918AV100/AK3918EV300L GPIO (pinctrl + GPIO)
 *
 * Copyright (C) 2021 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
 *
 * This driver is inspired by:
 * pinctrl-bcm2835.c, please see original file for copyright information
 * pinctrl-tegra.c, please see original file for copyright information
 *
 * Author:
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/bitmap.h>
#include <linux/bug.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/syscore_ops.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/clk.h>

#include <mach/map.h>

#if defined(CONFIG_MACH_AK3918AV100)
#include <dt-bindings/pinctrl/ak3918av100_pinctrl.h>
#elif defined(CONFIG_MACH_KM01A)
#include <dt-bindings/pinctrl/km01a_pinctrl.h>
#elif defined(CONFIG_MACH_AK3918EV300L)
#include <dt-bindings/pinctrl/ak3918ev300l_pinctrl.h>
#elif defined(CONFIG_MACH_AK3918AV130)
#include <dt-bindings/pinctrl/ak3918av130_pinctrl.h>
#endif

//define RAW_DEBUG 1
//#define AK_GPIO_DEBUG 1

#define MODULE_NAME              "ak-pinctrl"
#define AK_NUM_GPIOS             105

#define AK_GPIO_DIR0_REG         0x00
#define AK_GPIO_OUT0_REG         0x14
#define AK_GPIO_INPUT0_REG       0x28
#define AK_GPIO_INTEN0_REG       0x3c
#define AK_GPIO_INT_MODE0_REG    0x50
#define AK_GPIO_INTPOL0_REG      0x64
#define AK_GPIO_EDGE_STAT0_REG   0x78
#define AK_GPIO_DBLEDGE_EN0_REG  0x8c

#define AK_GPIO_DIR_REG(pin)           (((pin)>>5)*4 + AK_GPIO_DIR0_REG)
#define AK_GPIO_OUT_REG(pin)           (((pin)>>5)*4 + AK_GPIO_OUT0_REG)
#define AK_GPIO_IN_REG(pin)            (((pin)>>5)*4 + AK_GPIO_INPUT0_REG)
#define AK_GPIO_INTEN_REG(pin)         (((pin)>>5)*4 + AK_GPIO_INTEN0_REG)
#define AK_GPIO_INT_MODE_REG(pin)      (((pin)>>5)*4 + AK_GPIO_INT_MODE0_REG)
#define AK_GPIO_INTPOL_BASE(pin)       (((pin)>>5)*4 + AK_GPIO_INTPOL0_REG)
#define AK_GPIO_INTEDGE_BASE(pin)      (((pin)>>5)*4 + AK_GPIO_EDGE_STAT0_REG)
#define AK_GPIO_DBLEDGE_EN_BASE(pin)   (((pin)>>5)*4 + AK_GPIO_DBLEDGE_EN0_REG)

#define AK_GPIO_OUT_LOW         0
#define AK_GPIO_OUT_HIGH        1

#define AK_GPIO_PIN(a)          PINCTRL_PIN(a, "gpio" #a)

#define AK_GPIO_CONF_INVALID    (-1)

#define AK_CHIP_VERSION_REG              (AK_VA_SYSCTRL + 0x0)

#define AK_PIN_CON0_REG                  (AK_VA_SYSCTRL + 0x178)

#define AK_PIN_CON_REG(pin)              (AK_PIN_CON0_REG + \
                                         (ak_sharepin[(pin)].funcmux_reg_no)*4)

#define AK_PIN_CON_REG_OFFSET(pin)       (ak_sharepin[(pin)].funcmux_reg_offset)

#define AK_PIN_DRV_CON0_REG              (AK_VA_SYSCTRL + 0x1a4)

#define AK_PIN_DRV_CON_GROUP             (16)
#define AK_PIN_DRV_CON_REG(pin)          (AK_PIN_DRV_CON0_REG + \
                                         ((pin)/AK_PIN_DRV_CON_GROUP)*4)

#define AK_PIN_IE_CON0_REG               (AK_VA_SYSCTRL + 0x1c0)

#define AK_PIN_IE_GROUP                  (32)
#define AK_PIN_IE_REG(pin)               (AK_PIN_IE_CON0_REG + \
                                         ((pin)/AK_PIN_IE_GROUP)*4)

#define AK_PIN_PUPD_EN0_REG              (AK_VA_SYSCTRL + 0x264)

#define AK_PIN_PUPD_EN_GROUP             (32)
#define AK_PIN_PUPD_EN_REG(pin)          (AK_PIN_PUPD_EN0_REG + \
                                         ((pin)/AK_PIN_PUPD_EN_GROUP)*4)

#define AK_PIN_PUPD_SEL0_REG             (AK_VA_SYSCTRL + 0x274)

#define AK_PIN_PUPD_SEL_GROUP            (32)
#define AK_PIN_PUPD_SEL_REG(pin)         (AK_PIN_PUPD_SEL0_REG + \
                                         ((pin)/AK_PIN_PUPD_SEL_GROUP)*4)

#define AK_PIN_SLEW_RATE0_REG            (AK_VA_SYSCTRL + 0x1d0)

#define AK_PIN_SLEW_RATE_GROUP           (32)
#define AK_PIN_SLEW_RATE_REG(pin)        (AK_PIN_SLEW_RATE0_REG + \
                                         ((pin)/AK_PIN_SLEW_RATE_GROUP)*4)

#ifdef CONFIG_MACH_AK3918EV300L
#define AK_PIN_MIPI_REG                  (AK_VA_SYSCTRL + 0x238)
#endif

#define AK_PIN_PUPD_DISABLE     0
#define AK_PIN_PUPD_ENABLE      1

#define AK_PIN_CFG_DRV_EN       (0x01)
#define AK_PIN_CFG_INPUT_EN     (0x02)
#define AK_PIN_CFG_PUPD_EN      (0x04)
#define AK_PIN_CFG_PUPD_SEL     (0x08)
#define AK_PIN_CFG_SLEW_RATE    (0x10)

#define AK_PIN_CFG_ALL_DISABLE  (0x0)
#define AK_PIN_CFG_ALL_ENABLE   (AK_PIN_CFG_DRV_EN|AK_PIN_CFG_INPUT_EN|\
                   AK_PIN_CFG_PUPD_EN|AK_PIN_CFG_PUPD_SEL|AK_PIN_CFG_SLEW_RATE)

#ifdef AK_GPIO_DEBUG
#define ak_log_print(fmt, arg...) pr_info(fmt, ##arg)
#else
#define ak_log_print(fmt, arg...) pr_debug(fmt, ##arg)
#endif

#define DRIVE_STRENGTH(strength)         (strength)

#define AK_GPIO_REG_SHIFT(pin)          ((pin) % 32)

#define AK_PIN_AS_GPIO_IN         0 // pin as gpio in
#define AK_PIN_AS_GPIO_OUT        1 // pin as gpio out

struct gpio_sharepin {
    int pin;
    int funcmux_reg_no;
    int funcmux_reg_offset;
    int8_t gpio_func_no;
    int8_t pad_drv_offset;
    int8_t pad_ie_offset;
    int8_t pupd_en_offset;
    int8_t pupd_sel_offset;
    int8_t pad_sl_offset;
    int8_t func_mux_max;
    int8_t gpio_cfg;
};

static struct pinctrl_pin_desc ak_gpio_pins[AK_NUM_GPIOS] = {
    AK_GPIO_PIN(0),
    AK_GPIO_PIN(1),
    AK_GPIO_PIN(2),
    AK_GPIO_PIN(3),
    AK_GPIO_PIN(4),
    AK_GPIO_PIN(5),
    AK_GPIO_PIN(6),
    AK_GPIO_PIN(7),
    AK_GPIO_PIN(8),
    AK_GPIO_PIN(9),
    AK_GPIO_PIN(10),
    AK_GPIO_PIN(11),
    AK_GPIO_PIN(12),
    AK_GPIO_PIN(13),
    AK_GPIO_PIN(14),
    AK_GPIO_PIN(15),
    AK_GPIO_PIN(16),
    AK_GPIO_PIN(17),
    AK_GPIO_PIN(18),
    AK_GPIO_PIN(19),
    AK_GPIO_PIN(20),
    AK_GPIO_PIN(21),
    AK_GPIO_PIN(22),
    AK_GPIO_PIN(23),
    AK_GPIO_PIN(24),
    AK_GPIO_PIN(25),
    AK_GPIO_PIN(26),
    AK_GPIO_PIN(27),
    AK_GPIO_PIN(28),
    AK_GPIO_PIN(29),
    AK_GPIO_PIN(30),
    AK_GPIO_PIN(31),
    AK_GPIO_PIN(32),
    AK_GPIO_PIN(33),
    AK_GPIO_PIN(34),
    AK_GPIO_PIN(35),
    AK_GPIO_PIN(36),
    AK_GPIO_PIN(37),
    AK_GPIO_PIN(38),
    AK_GPIO_PIN(39),
    AK_GPIO_PIN(40),
    AK_GPIO_PIN(41),
    AK_GPIO_PIN(42),
    AK_GPIO_PIN(43),
    AK_GPIO_PIN(44),
    AK_GPIO_PIN(45),
    AK_GPIO_PIN(46),
    AK_GPIO_PIN(47),
    AK_GPIO_PIN(48),
    AK_GPIO_PIN(49),
    AK_GPIO_PIN(50),
    AK_GPIO_PIN(51),
    AK_GPIO_PIN(52),
    AK_GPIO_PIN(53),
    AK_GPIO_PIN(54),
    AK_GPIO_PIN(55),
    AK_GPIO_PIN(56),
    AK_GPIO_PIN(57),
    AK_GPIO_PIN(58),
    AK_GPIO_PIN(59),
    AK_GPIO_PIN(60),
    AK_GPIO_PIN(61),
    AK_GPIO_PIN(62),
    AK_GPIO_PIN(63),
    AK_GPIO_PIN(64),
    AK_GPIO_PIN(65),
    AK_GPIO_PIN(66),
    AK_GPIO_PIN(67),
    AK_GPIO_PIN(68),
    AK_GPIO_PIN(69),
    AK_GPIO_PIN(70),
    AK_GPIO_PIN(71),
    AK_GPIO_PIN(72),
    AK_GPIO_PIN(73),
    AK_GPIO_PIN(74),
    AK_GPIO_PIN(75),
    AK_GPIO_PIN(76),
    AK_GPIO_PIN(77),
    AK_GPIO_PIN(78),
    AK_GPIO_PIN(79),
    AK_GPIO_PIN(80),
    AK_GPIO_PIN(81),
    AK_GPIO_PIN(82),
    AK_GPIO_PIN(83),
    AK_GPIO_PIN(84),
    AK_GPIO_PIN(85),
    AK_GPIO_PIN(86),
    AK_GPIO_PIN(87),
    AK_GPIO_PIN(88),
    AK_GPIO_PIN(89),
    AK_GPIO_PIN(90),
    AK_GPIO_PIN(91),
    AK_GPIO_PIN(92),
    AK_GPIO_PIN(93),
    AK_GPIO_PIN(94),
    AK_GPIO_PIN(95),
    AK_GPIO_PIN(96),
    AK_GPIO_PIN(97),
    AK_GPIO_PIN(98),
    AK_GPIO_PIN(99),
    AK_GPIO_PIN(100),
    AK_GPIO_PIN(101),
    AK_GPIO_PIN(102),
    AK_GPIO_PIN(103),
    AK_GPIO_PIN(104),
};

static const char * const ak_funcs[8] = {
    [0] = "default func",
    [1] = "func1",
    [2] = "func2",
    [3] = "func3",
    [4] = "func4",
    [5] = "func5",
    [6] = "func6",
    [7] = "func7",
};

#if defined(CONFIG_MACH_AK3918AV130)
static struct gpio_sharepin ak_sharepin[AK_NUM_GPIOS] = {
    /* GPIO0, UART1_TXD, SPI1_CLK, OPCLK1 */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO1, UART1_RXD, SPI1_DIN(IO1), RMII_RXER */
    {1, 0, 3, 0, 2, 1, 1, 1, 1, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO2, PWM3, SPI1_DOUT, RMII_RXDV, UART1_MX0 */
    {2, 0, 6, 0, 4, 2, 2, 2, 2, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO3, SPI1_CS, RMII_MDIO, UART1_MX1 */
    {3, 0, 9, 0, 6, 3, 3, 3, 3, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO4, TWI2_SCL, SD1_D3, SPI2_CS1, RMII_TXEN */
    {4, 0, 12, 0, 8, 4, 4, 4, 4, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO5, TWI2_SDA, SD1_D2, RMII_MDC */
    {5, 0, 15, 0, 10, 5, 5, 5, 5, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO6, TWI0_SDA, SD1_D1, RMII_RXD1*/
    {6, 0, 18, 0, 12, 6, 6, 6, 6, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO7, TWI0_SCL, SD1_D0, RMII_TXD0 */
    {7, 0, 21, 0, 14, 7, 7, 7, 7, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO8, SD1_MCLK, SPI2_CLK, RMII_TXD1 */
    {8, 0, 24, 0, 16, 8, 8, 8, 8, 6, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO9, CSI_HSYNC, SD1_CMD, SPI2_DIN(IO1), SWITCH0_SEL */
    {9, 0, 27, 0, 18, 9, 9, 9, 9, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO10, CSI_VSYNC, SPI2_DOUT(IO0), TWI1_SCL, SWITCH1_SEL */
    {10, 1, 0, 0, 20, 10, 10, 10, 10, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO11, CSI1_SCLK, PWM2, SPI2_CS0, RMII_RXD0, TWI1_SDA, SDIO1_CMD */
    {11, 1, 3, 0, 22, 11, 11, 11, 11, 8, AK_PIN_CFG_ALL_ENABLE},

    {12, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {13, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {14, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {15, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},

    /* GPIO16, SD1_MCLK, PWM0, UART2_RXD, PWM_CSI3 */
    {16, 1, 18, 0, 0, 16, 16, 16, 16, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO17, CSI_DO, SPI1_CS, SD1_CMD, PWM3, UART2_TXD, CSI3_SCLK, SWITCH1_SEL */
    {17, 1, 21, 0, 2, 17, 17, 17, 17, 8, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO18, TWI2_SCL, CSI_D1, SPI1_CLK, SWITCH0_SEL, UART2_MX0 */
    {18, 1, 24, 0, 4, 18, 18, 18, 18, 6, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO19, TWI2_SDA, CSI_D2, SPI1_DOUT(IO0), SWITCH1_SEL, UART2_MX1 */
    {19, 1, 27, 0, 6, 19, 19, 19, 19, 6, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO20, TWI0_SDA, CSI_D3 */
    {20, 2, 0, 0, 8, 20, 20, 20, 20, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO21, TWI0_SCL, CSI_D5 */
    {21, 2, 3, 0, 10, 21, 21, 21, 21, 3, AK_PIN_CFG_ALL_ENABLE},
    /* CSI0_CLK, CSI_D6, GPIO22 */
    {22, 2, 6, 0, 12, 22, 22, 22, 22, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO23, CSI_D4, CSI2_SCLK, SWITCH0_SEL */
    {23, 2, 9, 0, 14, 23, 23, 23, 23, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO24, SPI1_CS, SD1_D1 */
    {24, 2, 12, 0, 16, 24, 24, 24, 24, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO25, SPI1_CLK, SD1_D0 */
    {25, 2, 15, 0, 18, 25, 25, 25, 25, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO26, SPI1_DOUT(IO0), SD1_CMD */
    {26, 2, 18, 0, 20, 26, 26, 26, 26, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO27, SPI_DIN(IO1), SD1_MCLK, PWM1 */
    {27, 2, 21, 0, 22, 27, 27, 27, 27, 4, AK_PIN_CFG_ALL_ENABLE},

    {28, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {29, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},

    /* GPIO30, CSI1_SCLK, PWM0 */
    {30, 3, 0, 0, 28, 30, 30, 30, 30, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO31, SD0_D2, JTAG0_TDI, eMMC_D3, SD1_D1, JTAG1_TDI */
    {31, 3, 3, 0, 30, 31, 31, 31, 31, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO32, SD0_D3, JTAG0_RSTN, eMMC_D0, SD1_D0, JTAG1_RSTN */
    {32, 3, 6, 0, 0, 0, 0, 0, 0, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO33, SD0_CMD, eMMC_CMD, SD1_CMD */
    {33, 3, 9, 0, 2, 1, 1, 1, 1, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO34, SD0_MCLK, JTAG0_TCK, eMMC_MCLK, SD1_MCLK, JTAG1_TCK */
    {34, 3, 12, 0, 4, 2, 2, 2, 2, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO35, SD0_D0, JTAG0_TMS, eMMC_D1, SD1_D3, JTAG1_TMS */
    {35, 3, 15, 0, 6, 3, 3, 3, 3, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO36, SD0_D1, JTAG0_TDO, eMMC_D2, SD1_D2, JTAG1_TDO */
    {36, 3, 18, 0, 8, 4, 4, 4, 4, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO37, PWM_CSI3, UART1_MX1 */
    {37, 3, 21, 0, 10, 5, 5, 5, 5, 3, AK_PIN_CFG_ALL_ENABLE},

    {38, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {39, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {40, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {41, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},

    /* GPIO42, eMMC_D3, SPI0_WP(IO1) */
    {42, 4, 6, 0, 20, 10, 10, 10, 10, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO43, eMMC_D0, SPI0_DIN(IO1) */
    {43, 4, 9, 0, 22, 11, 11, 11, 11, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO44, eMMC_D1, SFC_CS */
    {44, 4, 12, 0, 24, 12, 12, 12, 12, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO45, eMMC_D2, SPI0_HOLD(IO3) */
    {45, 4, 15, 0, 26, 13, 13, 13, 13, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO46, eMMC_MCLK, SPI0_CLK */
    {46, 4, 18, 0, 28, 14, 14, 14, 14, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO47, eMMC_CMD, SPI0_DOUT(IO0 */
    {47, 4, 21, 0, 30, 15, 15, 15, 15, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO48, PWM0, UART1_TXD, TWI1_SCL */
    {48, 4, 24, 0, 0, 16, 16, 16, 16, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO49, PWM1, UART1_RXD, TWI1_SDA */
    {49, 4, 27, 0, 2, 17, 17, 17, 17, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO50, SWITCH1_SEL, PWM_CSI1 */
    {50, 5, 0, 0, 4, 18, 18, 18, 18, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO51, UART0_TXD */
    {51, 5, 3, 0, 6, 19, 19, 19, 19, 2, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO52, UART0_RXD */
    {52, 5, 6, 0, 8, 20, 20, 20, 20, 2, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO53, PWM_CSI0, CSI2_SCLK */
    {53, 5, 9, 0, 10, 21, 21, 21, 21, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO54, PWM_CSI1, CSI3_SCLK */
    {54, 5, 12, 0, 12, 22, 22, 22, 22, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO55, SPI2_CS0, UART2_RXD, PWM3, SWITCH0_SEL */
    {55, 5, 15, 0, 14, 23, 23, 23, 23, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO56, SPI2_CLK, UART2_TXD, PWM4, SWITCH1_SEL */
    {56, 5, 18, 0, 16, 24, 24, 24, 24, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO57, SPI2_DIN(IO1), UART2_MX0, TWI1_SCL */
    {57, 5, 21, 0, 18, 25, 25, 25, 25, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO58, SPI2_CS1, UART2_MX1, TWI1_SDA, SD1_CMD, SPI2_DATA3 */
    {58, 5, 24, 0, 20, 26, 26, 26, 26, 6, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO59, SPI2_DOUT(IO0), PWM1, PWM_CSI2, SPI2_CLK */
    {59, 5, 27, 0, 22, 27, 27, 27, 27, 6, AK_PIN_CFG_ALL_ENABLE},

    {60, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {61, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {62, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {63, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {64, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},

    /* GPIO65, SPI2_CS0, JTAG0_RSTN, UART2_RXD, SD1_MCLK, JTAG1_RSTN */
    {65, 6, 15, 0, 2, 1, 1, 1, 1, 6, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO66, SPI2_CLK, JTAG0_TCK, UART2_TXD, SD1_D2, JTAG1_TCK */
    {66, 6, 18, 0, 4, 2, 2, 2, 2, 6, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO67, SPI2_DIN(IO1), JTAG0_TMS, PWM2, UART2_MX0, JTAG1_TMS, SD1_D3 */
    {67, 6, 21, 0, 6, 3, 3, 3, 3, 7, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO68, SPI2_CS1, JTAG0_TDI, PWM3, JTAG1_TDI, SD1_D0, SPI2_D2 */
    {68, 6, 24, 0, 8, 4, 4, 4, 4, 7, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO69, SPI2_DOUT(IO0), JTAG0_TDO, UART2_MX1, SD1_D1, JTAG1_TDO */
    {69, 6, 27, 0, 10, 5, 5, 5, 5, 6, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO70, AIN1 */
    {70, 7, 0, 0, 12, 6, 6, 6, 6, 2, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO71, AIN0 */
    {71, 7, 3, 0, 14, 7, 7, 7, 7, 2, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO72, MIPI_RX_L4P */
    {72, 7, 6, 0, 16, 8, 8, 8, 8, 2, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO73, MIPI_RX_L4N */
    {73, 7, 9, 0, 18, 9, 9, 9, 9, 2, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO74, MIPI_RX_L5P */
    {74, 7, 12, 0, 20, 10, 10, 10, 10, 2, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO75, MIPI_RX_L5N */
    {75, 7, 15, 0, 22, 11, 11, 11, 11, 2, AK_PIN_CFG_ALL_ENABLE},

    {76, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {77, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},

    /* GPIO78, MIPI_RX_L1P */
    {78, 7, 24, 0, 28, 14, 14, 14, 14, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO79, MIPI_RX_L1N */
    {79, 7, 27, 0, 30, 15, 15, 15, 15, 3, AK_PIN_CFG_ALL_ENABLE},


    /* PWR_SEQ, GPIO80 */
    //{80, x, x, x, 16, 16, 16, 16, 16, AK_PIN_CFG_ALL_ENABLE},
    /* PWR_WAKEUP, GPIO81 */
    //{81, x, x, x, 17, 17, 17, 17, 17, AK_PIN_CFG_ALL_ENABLE},
    {80, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {81, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {82, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {83, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {84, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {85, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {86, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {87, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {88, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {89, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},

    {90, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {91, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {92, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {93, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {94, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {95, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {96, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {97, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},

    /* GPIO98, SWITCH0_SEL, PWM_CSI1, UART1_MX0 */
    {98, 9, 24, 0, 4, 2, 2, 2, 2, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO99, MIPI_RX_L0P, CSI_D7 */
    {99, 9, 27, 0, 6, 3, 3, 3, 3, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO100, MIPI_RX_L0N, CSI_PCLK */
    {100, 10, 0, 0, 8, 4, 4, 4, 4, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO101, MIPI_RX_L2P, CSI_D9 */
    {101, 10, 3, 0, 10, 5, 5, 5, 5, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO102, MIPI_RX_L2N, CSI_D8 */
    {102, 10, 6, 0, 12, 6, 6, 6, 6, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO103, MIPI_RX_L3P */
    {103, 10, 9, 0, 14, 7, 7, 7, 7, 2, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO104, MIPI_RX_L3N */
    {104, 10, 12, 0, 16, 8, 8, 8, 8, 2, AK_PIN_CFG_ALL_ENABLE},
};

#else
//share pin func config in AK3918AV100
static struct gpio_sharepin ak_sharepin[AK_NUM_GPIOS] = {
    /* PIN funcmux_reg_no funcmux_reg_offset gpio_func_no
    *pad_drv_offset pad_ie_offset pupd_en_offset pupd_sel_offset 
    *pad_sl_offset func_mux_max gpio_cfg */
    /* GPIO0, UART1_TXD, SPI1_CLK, OPCLK1 */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO1, UART1_RXD, SPI1_DIN, RMII_RXER */
    {1, 0, 3, 0, 2, 1, 1, 1, 1, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO2, PWM3, SPI1_DOUT, RMII_RXDV */
    {2, 0, 6, 0, 4, 2, 2, 2, 2, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO3, SPI1_CS, RMII_MDIO */
    {3, 0, 9, 0, 6, 3, 3, 3, 3, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO4, TWI2_SCL, SDIO1_DATA3, SPI2_CS1, RMII_TXEN */
    {4, 0, 12, 0, 8, 4, 4, 4, 4, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO5, TWI2_SDA, SDIO1_DATA2, RMII_MDC */
    {5, 0, 15, 0, 10, 5, 5, 5, 5, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO6, TWI0_SDA, SDIO1_DATA1, RMII_RXD1 */
    {6, 0, 18, 0, 12, 6, 6, 6, 6, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO7, TWI0_SCL, SDIO1_DATA0, RMII_TXD0 */
    {7, 0, 21, 0, 14, 7, 7, 7, 7, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO8, SDIO1_MCLK, SPI2_CLK, RMII_TXD1 */
    {8, 0, 24, 0, 16, 8, 8, 8, 8, 6, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO9, CSI_HSYNC, SDIO1_CMD, SPI2_DIN */
    {9, 0, 27, 0, 18, 9, 9, 9, 9, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO10, CSI_VSYNC, SPI2_DOUT, TWI1_SCL */
    {10, 1, 0, 0, 20, 10, 10, 10, 10, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO11, CSI0_SCLK, PWM2, SPI2_CS0, RMII_RXD0, TWI1_SDA */
    {11, 1, 3, 0, 22, 11, 11, 11, 11, 7, AK_PIN_CFG_ALL_ENABLE},
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A)
    /* GPIO12, SDIO1_DATA3, PWM4 */
    {12, 1, 6, 0, 24, 12, 12, 12, 12, 3, AK_PIN_CFG_ALL_ENABLE},
#elif defined(CONFIG_MACH_AK3918EV300L)
    {12, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
#endif
    {13, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {14, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {15, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    /* GPIO16, SDIO1_MCLK, PWM0, UART2_RXD */
    {16, 1, 21, 0, 0, 16, 16, 16, 16, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO17, CSI_D0, SPI1_CS, SDIO1_CMD, PWM3, UART2_TXD */
    {17, 1, 24, 0, 2, 17, 17, 17, 17, 6, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO18, TWI2_SCL, CSI_D1, SPI1_CLK */
    {18, 1, 27, 0, 4, 18, 18, 18, 18, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO19, TWI2_SDA, CSI_D2, SPI1_DOUT */
    {19, 2, 0, 0, 6, 19, 19, 19, 19, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO20, TWI0_SDA, CSI_D3 */
    {20, 2, 3, 0, 8, 20, 20, 20, 20, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO21, TWI0_SCL, CSI_D5 */
    {21, 2, 6, 0, 10, 21, 21, 21, 21, 3, AK_PIN_CFG_ALL_ENABLE},
    /* CSI0_SCLK, CSI_D6, GPIO22 */
    {22, 2, 9, 3, 12, 22, 22, 22, 22, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO23, CSI_D4 */
    {23, 2, 12, 0, 14, 23, 23, 23, 23, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO24, SPI1_CS, SDIO1_DATA1 */
    {24, 2, 18, 0, 16, 24, 24, 24, 24, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO25, SPI1_CLK, SDIO1_DATA0 */
    {25, 2, 21, 0, 18, 25, 25, 25, 25, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO26, SPI1_DOUT, SDIO1_CMD */
    {26, 2, 24, 0, 20, 26, 26, 26, 26, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO27, SPI1_DIN, SDIO1_MCLK, PWM1 */
    {27, 2, 27, 0, 22, 27, 27, 27, 27, 4, AK_PIN_CFG_ALL_ENABLE},

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A)
    /* PIN funcmux_reg_no funcmux_reg_offset gpio_func_no pad_drv_offset 
    *pad_ie_offset pupd_en_offset pupd_sel_offset
    *pad_sl_offset func_mux_max gpio_cfg */

    {28, 3, 0, 0,   24, 28,     28, 28, 28, 3, AK_PIN_CFG_ALL_ENABLE},
    {29, 3, 3, 0,   26, 29,     29, 29, 29, 2, AK_PIN_CFG_ALL_ENABLE},

    /* GPIO30, CSI1_SCLK, PWM0 */
    {30, 2, 15, 0, 28, 30, 30, 30, 30, 3, AK_PIN_CFG_ALL_ENABLE},
#elif defined(CONFIG_MACH_AK3918EV300L)
    {28, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {29, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {30, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
#endif
    /* GPIO31, SDIO0_DATA2, JTAG_TDI, EMMC_DATA3, SDIO1_DATA1 */
    {31, 3, 6, 0, 30, 31, 31, 31, 31, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO32, SDIO0_DATA3, JTAG_RSTN, EMMC_DATA0, SDIO1_DATA0 */
    {32, 3, 9, 0, 0, 0, 0, 0, 0, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO33, SDIO0_CMD, EMMC_CMD, SDIO1_CMD */
    {33, 3, 12, 0, 2, 1, 1, 1, 1, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO34, SDIO0_MCLK, JTAG_TCLK, EMMC_MCLK, SDIO1_MCLK */
    {34, 3, 15, 0, 4, 2, 2, 2, 2, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO35, SDIO0_DATA0, JTAG_TMS, EMMC_DATA1, SDIO1_DATA3 */
    {35, 3, 18, 0, 6, 3, 3, 3, 3, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO36, SDIO0_DATA1, JTAG_TDO, EMMC_DATA2, SDIO1_DATA2 */
    {36, 3, 21, 0, 8, 4, 4, 4, 4, 5, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO37 */
    {37, 0, 0, 0, 10, 5, 5, 5, 5, 1, AK_PIN_CFG_ALL_ENABLE},
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A)
    /* GPIO38, EMMC_DATA7, SPI1_CS, SDIO1_DATA3 */
    {38, 3, 27, 0, 12, 6, 6, 6, 6, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO39, EMMC_DATA6, SPI1_CLK, SDIO1_DATA2 */
    {39, 4, 0, 0, 14, 7, 7, 7, 7, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO40, EMMC_DATA5, SPI1_DOUT, TWI1_SCL */
    {40, 4, 3, 0, 16, 8, 8, 8, 8, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO41, EMMC_DATA4, SPI1_DIN, TWI1_SDA */
    {41, 4, 6, 0, 18, 9, 9, 9, 9, 4, AK_PIN_CFG_ALL_ENABLE},
#elif defined(CONFIG_MACH_AK3918EV300L)
    {38, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {39, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {40, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {41, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
#endif
    /* GPIO42, EMMC_DATA3, SPI0_WP */
    {42, 4, 9, 0, 20, 10, 10, 10, 10, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO43, EMMC_DATA0, SPI0_DIN */
    {43, 4, 12, 0, 22, 11, 11, 11, 11, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO44, EMMC_DATA1, SPI0_CS */
    {44, 4, 15, 0, 24, 12, 12, 12, 12, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO45, EMMC_DATA2, SPI0_HOLD */
    {45, 4, 18, 0, 26, 13, 13, 13, 13, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO46, EMMC_MCLK, SPI0_CLK */
    {46, 4, 21, 0, 28, 14, 14, 14, 14, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO47, EMMC_CMD, SPI0_DOUT */
    {47, 4, 24, 0, 30, 15, 15, 15, 15, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO48, PWM0, UART1_TXD, TWI1_SCL */
    {48, 4, 27, 0, 0, 16, 16, 16, 16, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO49, PWM1, UART1_RXD, TWI1_SDA */
    {49, 5, 0, 0, 2, 17, 17, 17, 17, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO50 */
    {50, 0, 0, 0, 4, 18, 18, 18, 18, 1, AK_PIN_CFG_ALL_ENABLE},
#if defined(CONFIG_MACH_AK3918AV130)
    /* GPIO51, UART0_TXD */
    {51, 5, 3, 0, 6, 19, 19, 19, 19, 2, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO52, UART0_RXD */
    {52, 5, 6, 0, 8, 20, 20, 20, 20, 2, AK_PIN_CFG_ALL_ENABLE},
#else   
    /* GPIO51, UART0_TXD */
    {51, 5, 6, 0, 6, 19, 19, 19, 19, 2, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO52, UART0_RXD */
    {52, 5, 9, 0, 8, 20, 20, 20, 20, 2, AK_PIN_CFG_ALL_ENABLE},
#endif
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A)
    /* GPIO53, PWM_CSI0 */
    {53, 5, 12, 0, 10, 21, 21, 21, 21, 2, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO54, PWM_CSI1 */
    {54, 5, 15, 0, 12, 22, 22, 22, 22, 2, AK_PIN_CFG_ALL_ENABLE},
#elif defined(CONFIG_MACH_AK3918EV300L)
    {53, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {54, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
#endif
    /* GPIO55, SPI2_CS0, UART2_RXD, PWM3 */
    {55, 5, 18, 0, 14, 23, 23, 23, 23, 4, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO56, SPI2_CLK, UART2_TXD, PWM4 */
    {56, 5, 21, 0, 16, 24, 24, 24, 24, 4, AK_PIN_CFG_ALL_ENABLE},
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A)
    /* GPIO57, SPI2_DIN, UART2_CTS, TWI1_SCL */
    {57, 5, 24, 0, 18, 25, 25, 25, 25, 4, AK_PIN_CFG_ALL_ENABLE},
#elif defined(CONFIG_MACH_AK3918EV300L)
    {57, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
#endif
    /* GPIO58, SPI2_CS1, UART2_RTS, TWI1_SDA, SDIO1_CMD */
    {58, 5, 27, 0, 20, 26, 26, 26, 26, 5, AK_PIN_CFG_ALL_ENABLE},
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A)
    /* GPIO59, SPI2_DOUT, PWM1 */
    {59, 6, 0, 0, 22, 27, 27, 27, 27, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO60, TWI1_SCL, I2S_MCLK */
    {60, 6, 3, 0, 24, 28, 28, 28, 28, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO61, TWI1_SDA, I2S_BCLK */
    {61, 6, 6, 0, 26, 29, 29, 29, 29, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO62, UART1_TXD, I2S_LRCLK */
    {62, 6, 9, 0, 28, 30, 30, 30, 30, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO63, UART1_RXD, I2S_DOUT */
    {63, 6, 12, 0, 30, 31, 31, 31, 31, 3, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO64, PWM2, I2S_DIN */
    {64, 6, 15, 0, 0, 0, 0, 0, 0, 3, AK_PIN_CFG_ALL_ENABLE},
#elif defined(CONFIG_MACH_AK3918EV300L)
    {59, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {60, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {61, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {62, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {63, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {64, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
#endif
    /* JTAG_RSTN, SPI2_CS0, GPIO65, UART2_RXD, I2S_MCLK, SDIO1_MCLK */
    {65, 6, 18, 2, 2, 1, 1, 1, 1, 6, AK_PIN_CFG_ALL_ENABLE},
    /* JTAG_TCK, SPI2_CLK, GPIO66, UART2_TXD, I2S_BCLK, SDIO1_DATA2 */
    {66, 6, 21, 2, 4, 2, 2, 2, 2, 6, AK_PIN_CFG_ALL_ENABLE},
    /* JTAG_TMS, SPI2_DIN, GPIO67, PWM2, UART2_CTS, I2S_LRCLK, SDIO1_DATA3 */
    {67, 6, 24, 2, 6, 3, 3, 3, 3, 7, AK_PIN_CFG_ALL_ENABLE},
    /* JTAG_TDI, SPI2_CS1, GPIO68, PWM3, I2S_DOUT, SDIO1_DATA0 */
    {68, 6, 27, 2, 8, 4, 4, 4, 4, 7, AK_PIN_CFG_ALL_ENABLE},
    /* JTAG_TDO, SPI2_DOUT, GPIO69, UART2_RTS, I2S_DIN, SDIO1_DATA1 */
    {69, 7, 0, 2, 10, 5, 5, 5, 5, 6, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO70, AIN1 */
    {70, 8, 24, 0, 12, 6, 6, 6, 6, 2, AK_PIN_CFG_ALL_ENABLE},
    /* GPIO71, AIN0 */
    {71, 8, 27, 0, 14, 7, 7, 7, 7, 2, AK_PIN_CFG_ALL_ENABLE},
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A)
    /* MIPI_RX_L4P, CSI_D4, GPIO72 */
    {72, 9, 24, 2, 16, 8, 8, 8, 8, 3, AK_PIN_CFG_ALL_ENABLE},
    /* MIPI_RX_L4N, CSI_D1, GPIO73 */
    {73, 9, 27, 2, 18, 9, 9, 9, 9, 3, AK_PIN_CFG_ALL_ENABLE},
    /* MIPI_RX_L5P, CSI_D2, GPIO74 */
    {74, 10, 0, 2, 20, 10, 10, 10, 10, 3, AK_PIN_CFG_ALL_ENABLE},
    /* MIPI_RX_L5N, CSI_D0, GPIO75 */
    {75, 10, 3, 2, 22, 11, 11, 11, 11, 3, AK_PIN_CFG_ALL_ENABLE},
    {76, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    /* GPIO77, PWM3 */
    {77, 10, 6, 0, 26, 13, 13, 13, 13, 2, AK_PIN_CFG_ALL_ENABLE},
    /* MIPI_RX_L1P, CSI_D6, GPIO78 */
    {78, 9, 6, 2, 28, 14, 14, 14, 14, 3, AK_PIN_CFG_ALL_ENABLE},
    /* MIPI_RX_L1N, CSI_D5, GPIO79 */
    {79, 9, 9, 2, 30, 15, 15, 15, 15, 3, AK_PIN_CFG_ALL_ENABLE},
    /* PWR_SEQ, GPIO80 */
    {80, 0, 0, 1, 0, 0, 0, 0, 0, 2,
    AK_PIN_CFG_DRV_EN|AK_PIN_CFG_PUPD_EN|
    AK_PIN_CFG_PUPD_SEL|AK_PIN_CFG_SLEW_RATE},
    /* PWR_WAKEUP, GPIO81 */
    {81, 0, 0, 1, 0, 0, 0, 0, 0, 2,
    AK_PIN_CFG_DRV_EN|AK_PIN_CFG_PUPD_EN|
    AK_PIN_CFG_PUPD_SEL|AK_PIN_CFG_SLEW_RATE},
    
    /* PIN funcmux_reg_no funcmux_reg_offset gpio_func_no pad_drv_offset 
    pad_ie_offset pupd_en_offset pupd_sel_offset
    *pad_sl_offset func_mux_max gpio_cfg */
    {82, 7, 3, 1,   4, 18, 18, 18, 18, 2,  AK_PIN_CFG_ALL_ENABLE},
    {83, 7, 6, 1,   6, 19, 19, 19, 19, 2,  AK_PIN_CFG_ALL_ENABLE},
    {84, 7, 9, 1,   8, 20, 20, 20, 20, 2,  AK_PIN_CFG_ALL_ENABLE},
    {85, 7, 12, 1,  10, 21, 21, 21, 21, 2, AK_PIN_CFG_ALL_ENABLE},
    {86, 7, 15, 1,  12, 22, 22, 22, 22, 2, AK_PIN_CFG_ALL_ENABLE},
    {87, 7, 18, 1,  14, 23, 23, 23, 23, 2, AK_PIN_CFG_ALL_ENABLE},
    {88, 7, 21, 1,  16, 24, 24, 24, 24, 2, AK_PIN_CFG_ALL_ENABLE},
    {89, 7, 24, 1,  18, 25, 25, 25, 25, 2, AK_PIN_CFG_ALL_ENABLE},
    {90, 7, 27, 1,  20, 26, 26, 26, 26, 2, AK_PIN_CFG_ALL_ENABLE},
    {91, 8, 0,  1,  22, 27, 27, 27, 27, 2, AK_PIN_CFG_ALL_ENABLE},  
#elif defined(CONFIG_MACH_AK3918EV300L)
    {72, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {73, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {74, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {75, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {76, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {77, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {78, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {79, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {80, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {81, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {82, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {83, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {84, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {85, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {86, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {87, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {88, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {89, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {90, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {91, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
#endif

    {92, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {93, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {94, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {95, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {96, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    {97, 0, 0, 0, 0, 0, 0, 0, 0, AK_GPIO_CONF_INVALID, AK_PIN_CFG_ALL_DISABLE},
    /* SYS_RSTN_OUT, GPIO98 */
    {98, 8, 21, 1, 4, 2, 2, 2, 2, 2, AK_PIN_CFG_ALL_ENABLE},
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A)
    /* MIPI_RX_L0P, CSI_D7, GPIO99 */
    {99, 9, 0, 2, 6, 3, 3, 3, 3, 3, AK_PIN_CFG_ALL_ENABLE},
    /* MIPI_RX_L0N, CSI_CLK, GPIO100 */
    {100, 9, 3, 2, 8, 4, 4, 4, 4, 3, AK_PIN_CFG_ALL_ENABLE},
    /* MIPI_RX_L2P, CSI_D9, GPIO101 */
    {101, 9, 12, 2, 10, 5, 5, 5, 5, 3, AK_PIN_CFG_ALL_ENABLE},
    /* MIPI_RX_L2N, CSI_D8, GPIO102 */
    {102, 9, 15, 2, 12, 6, 6, 6, 6, 3, AK_PIN_CFG_ALL_ENABLE},
    /* MIPI_RX_L3P, CSI_D10, GPIO103 */
    {103, 9, 18, 2, 14, 7, 7, 7, 7, 3, AK_PIN_CFG_ALL_ENABLE},
    /* MIPI_RX_L3N, CSI_D11, GPIO104 */
    {104, 9, 21, 2, 16, 8, 8, 8, 8, 3, AK_PIN_CFG_ALL_ENABLE},
#elif defined(CONFIG_MACH_AK3918EV300L)
    /* PIN funcmux_reg_no funcmux_reg_offset gpio_func_no pad_drv_offset
    *pad_ie_offset pupd_en_offset pupd_sel_offset
    *pad_sl_offset func_mux_max gpio_cfg */
    /* MIPI_RX_L0P, CSI_D7, GPIO99 */
    {99, 9, 0, 2,   0, 0, 6,  12, 0, 3,
        (AK_PIN_CFG_INPUT_EN|AK_PIN_CFG_PUPD_EN|AK_PIN_CFG_PUPD_SEL)},
    /* MIPI_RX_L0N, CSI_CLK, GPIO100 */
    {100, 9, 3, 2,  0, 1, 7,  13, 0, 3,
        (AK_PIN_CFG_INPUT_EN|AK_PIN_CFG_PUPD_EN|AK_PIN_CFG_PUPD_SEL)},
    /* MIPI_RX_L2P, CSI_D9, GPIO101 */
    {101, 9, 12, 2, 0, 2, 8,  14, 0, 3,
        (AK_PIN_CFG_INPUT_EN|AK_PIN_CFG_PUPD_EN|AK_PIN_CFG_PUPD_SEL)},
    /* MIPI_RX_L2N, CSI_D8, GPIO102 */
    {102, 9, 15, 2, 0, 3, 9,  15, 0, 3,
        (AK_PIN_CFG_INPUT_EN|AK_PIN_CFG_PUPD_EN|AK_PIN_CFG_PUPD_SEL)},
    /* MIPI_RX_L3P, CSI_D10, GPIO103 */
    {103, 9, 18, 2, 0, 4, 10, 16, 0, 3,
        (AK_PIN_CFG_INPUT_EN|AK_PIN_CFG_PUPD_EN|AK_PIN_CFG_PUPD_SEL)},
    /* MIPI_RX_L3N, CSI_D11, GPIO104 */
    {104, 9, 21, 2, 0, 5, 11, 17, 0, 3,
        (AK_PIN_CFG_INPUT_EN|AK_PIN_CFG_PUPD_EN|AK_PIN_CFG_PUPD_SEL)},
#endif
};
#endif

struct ak_pinctrl {
    struct device *dev;
    struct clk *clk;
    void __iomem *gpio_base;
    u32 gpio_base_pa;
    int irq;

    unsigned int irq_type[AK_NUM_GPIOS];
    int fsel[AK_NUM_GPIOS];

    struct pinctrl_dev *pctl_dev;
    struct irq_domain *irq_domain;
    struct gpio_chip gc;
    struct pinctrl_gpio_range gpio_range;

    spinlock_t lock;
};

static void __iomem *ak_gpio_base;
static u32 ak_gpio_base_pa;

/* one pin per group */
static const char * const ak_gpio_groups[AK_NUM_GPIOS] = {
    "gpio0",
    "gpio1",
    "gpio2",
    "gpio3",
    "gpio4",
    "gpio5",
    "gpio6",
    "gpio7",
    "gpio8",
    "gpio9",
    "gpio10",
    "gpio11",
    "gpio12",
    "gpio13",
    "gpio14",
    "gpio15",
    "gpio16",
    "gpio17",
    "gpio18",
    "gpio19",
    "gpio20",
    "gpio21",
    "gpio22",
    "gpio23",
    "gpio24",
    "gpio25",
    "gpio26",
    "gpio27",
    "gpio28",
    "gpio29",
    "gpio30",
    "gpio31",
    "gpio32",
    "gpio33",
    "gpio34",
    "gpio35",
    "gpio36",
    "gpio37",
    "gpio38",
    "gpio39",
    "gpio40",
    "gpio41",
    "gpio42",
    "gpio43",
    "gpio44",
    "gpio45",
    "gpio46",
    "gpio47",
    "gpio48",
    "gpio49",
    "gpio50",
    "gpio51",
    "gpio52",
    "gpio53",
    "gpio54",
    "gpio55",
    "gpio56",
    "gpio57",
    "gpio58",
    "gpio59",
    "gpio60",
    "gpio61",
    "gpio62",
    "gpio63",
    "gpio64",
    "gpio65",
    "gpio66",
    "gpio67",
    "gpio68",
    "gpio69",
    "gpio70",
    "gpio71",
    "gpio72",
    "gpio73",
    "gpio74",
    "gpio75",
    "gpio76",
    "gpio77",
    "gpio78",
    "gpio79",
    "gpio80",
    "gpio81",
    "gpio82",
    "gpio83",
    "gpio84",
    "gpio85",
    "gpio86",
    "gpio87",
    "gpio88",
    "gpio89",
    "gpio90",
    "gpio91",
    "gpio92",
    "gpio93",
    "gpio94",
    "gpio95",
    "gpio96",
    "gpio97",
    "gpio98",
    "gpio99",
    "gpio100",
    "gpio101",
    "gpio102",
    "gpio103",
    "gpio104",
};

static const char * const irq_type_names[] = {
    [IRQ_TYPE_NONE] = "none",
    [IRQ_TYPE_EDGE_RISING] = "edge-rising",
    [IRQ_TYPE_EDGE_FALLING] = "edge-falling",
    [IRQ_TYPE_LEVEL_HIGH] = "level-high",
    [IRQ_TYPE_LEVEL_LOW] = "level-low",
    [IRQ_TYPE_EDGE_BOTH] = "edge-double",
};

#ifdef RAW_DEBUG
static inline u32 RAW_READL_DBG(const volatile void __iomem *addr)
{
    u32 ret;
    ret = __raw_readl(addr);
    if ((((u32)addr) & ~0xfff) == (u32)AK_VA_SYSCTRL) {
        pr_info("RAW_READL VA 0x%x PA 0x%x return 0x%x\n",
                (u32)addr, (u32)AK_PA_SYSCTRL + (((u32)addr) & 0xfff), ret);
    } else if ((((u32)addr) & ~0xfff) == (u32)ak_gpio_base) {
        pr_info("RAW_READL VA 0x%x PA 0x%x return 0x%x\n",
                (u32)addr, (u32)ak_gpio_base_pa + (((u32)addr) & 0xfff), ret);
    } else {
        pr_err("RAW_READL VA 0x%x error!\n", (u32)addr);
    }
    return ret;
}

static inline void RAW_WRITEL_DBG(u32 val, volatile void __iomem *addr)
{
    if ((((u32)addr) & ~0xfff) == (u32)AK_VA_SYSCTRL) {
        pr_info("RAW_WRITEL VA 0x%x PA 0x%x val 0x%x\n",
                (u32)addr, (u32)AK_PA_SYSCTRL + (((u32)addr) & 0xfff), val);
    } else if ((((u32)addr) & ~0xfff) == (u32)ak_gpio_base) {
        pr_info("RAW_WRITEL VA 0x%x PA 0x%x val 0x%x\n",
                (u32)addr, (u32)ak_gpio_base_pa + (((u32)addr) & 0xfff), val);
    } else {
        pr_err("RAW_WRITEL VA 0x%x error!\n", (u32)addr);
    }
    __raw_writel(val, addr);
}
#else
#define RAW_READL_DBG(reg)       __raw_readl(reg)
#define RAW_WRITEL_DBG(val, reg) (__raw_writel(val, reg), __raw_readl(AK_VA_SYSCTRL))
#endif

static inline int pin_pmc(int pin)
{
#if defined(CONFIG_MACH_AK3918AV130)
    return 0;
#else 
    return ((pin == 80) || (pin == 81)) ? 1 : 0;
#endif
}

#ifdef CONFIG_MACH_AK3918EV300L
static inline int pin_mipi(int pin)
{
    return ((pin > 98) && (pin <= 104)) ? 1 : 0;
}
#endif

static void disable_gpio_retention(void)
{
    u32 regval;
#if defined(CONFIG_MACH_AK3918AV130)
    regval = RAW_READL_DBG(AK_VA_SYSCTRL+0x260);
    regval &= ~(1UL << 0);
    RAW_WRITEL_DBG(regval, AK_VA_SYSCTRL+0x260);
#else
    regval = RAW_READL_DBG(AK_PIN_PUPD_SEL0_REG+3*4);
    regval &= ~(1UL << 31);
    RAW_WRITEL_DBG(regval, AK_PIN_PUPD_SEL0_REG+3*4);
#endif
}

static int pin_set_share(struct ak_pinctrl *pc, int pin, int sel)
{
    u32 regval;
    int offset = AK_PIN_CON_REG_OFFSET(pin);

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return -EINVAL;
    }

    if (sel >= ak_sharepin[pin].func_mux_max) {
        pr_err("%s pin_%d max %d but sel %d\n",
                __func__, pin, ak_sharepin[pin].func_mux_max, sel);
        return -EINVAL;
    }

    if (pin_pmc(pin)) {
        return -ENODEV;
    } else if (ak_sharepin[pin].func_mux_max > 1) { // skip GPIO37 and GPIO50
        regval = RAW_READL_DBG(AK_PIN_CON_REG(pin));
        regval &= ~(0x7 << offset);
        regval |= ((sel & 0x7) << offset);
        RAW_WRITEL_DBG(regval, AK_PIN_CON_REG(pin));
        ak_log_print("PIN_%d: FUNC(%d) 0x%p:0x%x\n",
                pin, sel, AK_PIN_CON_REG(pin),
            RAW_READL_DBG(AK_PIN_CON_REG(pin)));
    }

    pc->fsel[pin] = sel;
    return 0;
}

static int pin_set_input_enable(int pin, int enable)
{
    u32 regval;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return -EINVAL;
    }

    if (!(ak_sharepin[pin].gpio_cfg & AK_PIN_CFG_INPUT_EN)) {
        return -EPERM;
    }

#ifdef CONFIG_MACH_AK3918EV300L
    if (pin_mipi(pin)) {
        regval = RAW_READL_DBG(AK_PIN_MIPI_REG);
        if (enable) {
            regval |= (0x1 << ak_sharepin[pin].pad_ie_offset);
        } else {
            regval &= ~(0x1 << ak_sharepin[pin].pad_ie_offset);
        }

        RAW_WRITEL_DBG(regval, AK_PIN_MIPI_REG);
        ak_log_print("PIN_%d: Input_EN(%d) 0x%p:0x%x\n",
                pin, enable, AK_PIN_MIPI_REG,
                RAW_READL_DBG(AK_PIN_MIPI_REG));
        return 0;
    }
#endif

    regval = RAW_READL_DBG(AK_PIN_IE_REG(pin));
    if (enable) {
        regval |= (0x1 << ak_sharepin[pin].pad_ie_offset);
    } else {
        regval &= ~(0x1 << ak_sharepin[pin].pad_ie_offset);
    }

    RAW_WRITEL_DBG(regval, AK_PIN_IE_REG(pin));
    ak_log_print("PIN_%d: Input_EN(%d) 0x%p:0x%x\n",
            pin, enable, AK_PIN_IE_REG(pin),
            RAW_READL_DBG(AK_PIN_IE_REG(pin)));

    return 0;
}

static int pin_get_input_enable(int pin)
{
    u32 regval;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return -EINVAL;
    }

    if (!(ak_sharepin[pin].gpio_cfg & AK_PIN_CFG_INPUT_EN)) {
        return -EPERM;
    }

#ifdef CONFIG_MACH_AK3918EV300L
    if (pin_mipi(pin)) {
        regval = RAW_READL_DBG(AK_PIN_MIPI_REG);
        ak_log_print("%s 0x%p:0x%x\n",
                __func__, AK_PIN_MIPI_REG, regval);
        regval &= (0x1 << ak_sharepin[pin].pad_ie_offset);
        return (regval ? 1:0);
    }
#endif

    regval = RAW_READL_DBG(AK_PIN_IE_REG(pin));

    ak_log_print("%s 0x%p:0x%x\n",
            __func__, AK_PIN_IE_REG(pin), regval);

    regval &= (0x1 << ak_sharepin[pin].pad_ie_offset);

    return (regval ? 1:0);
}

static int pin_set_drive(int pin, int strength)
{
    u32 regval;
    void __iomem * pad_drv_reg = AK_PIN_DRV_CON_REG(pin);

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return -EINVAL;
    }

    if (!(ak_sharepin[pin].gpio_cfg & AK_PIN_CFG_DRV_EN)) {
        return -EPERM;
    }

    if (strength > 3)
        return -EINVAL;

    if (pin_pmc(pin)) {
        return -ENODEV;
    }

    regval = RAW_READL_DBG(pad_drv_reg);
    regval &= ~(0x3 << ak_sharepin[pin].pad_drv_offset);
    regval |= ((strength & 0x3) << ak_sharepin[pin].pad_drv_offset);

    RAW_WRITEL_DBG(regval, pad_drv_reg);

    ak_log_print("PIN_%d: DRV(%d) 0x%p:0x%x\n",
            pin, ak_sharepin[pin].pad_drv_offset,
            pad_drv_reg, RAW_READL_DBG(pad_drv_reg));

    return 0;
}

static int pin_get_drive(int pin)
{
    u32 regval;
    void __iomem * pad_drv_reg = AK_PIN_DRV_CON_REG(pin);

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return -EINVAL;
    }

    if (!(ak_sharepin[pin].gpio_cfg & AK_PIN_CFG_DRV_EN)) {
        return -EPERM;
    }

    if (pin_pmc(pin)) {
        return -ENODEV;
    }

    regval = RAW_READL_DBG(pad_drv_reg);

    ak_log_print("%s 0x%p:0x%x\n", __func__, pad_drv_reg, regval);

    regval &= (0x3 << ak_sharepin[pin].pad_drv_offset);
    regval = regval >> ak_sharepin[pin].pad_drv_offset;

    return DRIVE_STRENGTH(regval);
}

/**
*
*@brief: set pin pull polarity
*@param[in] struct ak_pinctrl *pc
*@param[in] int pin
*@param[in] int pullup
*@return: int
*
**/
static int pin_set_pull_polarity(struct ak_pinctrl *pc, int pin,
    int pullup)
{
    u32 regval;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return -EINVAL;
    }

    if (!(ak_sharepin[pin].gpio_cfg & AK_PIN_CFG_PUPD_SEL)) {
        return -EPERM;
    }

    if (pin_pmc(pin)) {
        return -ENODEV;
    }

#ifdef CONFIG_MACH_AK3918EV300L
    if (pin_mipi(pin)) {
        regval = RAW_READL_DBG(AK_PIN_MIPI_REG);
        if (pullup) {
            regval |= (0x1 << ak_sharepin[pin].pupd_sel_offset);
        } else {
            regval &= (~(0x1 << ak_sharepin[pin].pupd_sel_offset));
        }
        RAW_WRITEL_DBG(regval, AK_PIN_MIPI_REG);
        ak_log_print("PIN_%d: PULL_pol(%d) 0x%p:0x%x\n",
                pin, pullup,
                AK_PIN_MIPI_REG, RAW_READL_DBG(AK_PIN_MIPI_REG));
        return 0;
    }
#endif

    regval = RAW_READL_DBG(AK_PIN_PUPD_SEL_REG(pin));
    if (pullup) {
        regval |= (0x1 << ak_sharepin[pin].pupd_sel_offset);
    } else {
        regval &= (~(0x1 << ak_sharepin[pin].pupd_sel_offset));
    }

    RAW_WRITEL_DBG(regval, AK_PIN_PUPD_SEL_REG(pin));

    ak_log_print("PIN_%d: PULL_pol(%d) 0x%p:0x%x\n",
        pin, pullup,
        AK_PIN_PUPD_SEL_REG(pin), RAW_READL_DBG(AK_PIN_PUPD_SEL_REG(pin)));

    return 0;
}

static int pin_get_pull_polarity(struct ak_pinctrl *pc, int pin)
{
    u32 regval;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return -EINVAL;
    }

    if (!(ak_sharepin[pin].gpio_cfg & AK_PIN_CFG_PUPD_SEL)) {
        return -EPERM;
    }

    if (pin_pmc(pin)) {
        return -ENODEV;
    }

#ifdef CONFIG_MACH_AK3918EV300L
    if (pin_mipi(pin)) {
        regval = RAW_READL_DBG(AK_PIN_MIPI_REG);
        ak_log_print("%s 0x%p:0x%x\n", __func__, AK_PIN_MIPI_REG, regval);
        regval &= (0x1 << ak_sharepin[pin].pupd_sel_offset);
        return (regval ? 1:0);
    }
#endif

    regval = RAW_READL_DBG(AK_PIN_PUPD_SEL_REG(pin));

    ak_log_print("%s 0x%p:0x%x\n",
        __func__, AK_PIN_PUPD_SEL_REG(pin), regval);

    regval &= (0x1 << ak_sharepin[pin].pupd_sel_offset);

    return (regval ? 1:0);
}

/**
*
*@brief: pin_set_pull_enable
*@param[in] struct ak_pinctrl *pc
*@param[in] int pin
*@return: int
*
**/
static int pin_set_pull_enable(struct ak_pinctrl *pc, int pin, int enable)
{
    u32 regval;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return -EINVAL;
    }

    if (!(ak_sharepin[pin].gpio_cfg & AK_PIN_CFG_PUPD_EN)) {
        return -EPERM;
    }

    if (pin_pmc(pin)) {
        return -ENODEV;
    }

#ifdef CONFIG_MACH_AK3918EV300L
    if (pin_mipi(pin)) {
        regval = RAW_READL_DBG(AK_PIN_MIPI_REG);
        if (enable) {
            regval |= (0x1 << ak_sharepin[pin].pupd_en_offset);
        } else {
            regval &= (~(0x1 << ak_sharepin[pin].pupd_en_offset));
        }
        RAW_WRITEL_DBG(regval, AK_PIN_MIPI_REG);
        ak_log_print("PIN_%d: PULL_EN(%d) 0x%p:0x%x\n",
                pin, enable,
                AK_PIN_MIPI_REG, RAW_READL_DBG(AK_PIN_MIPI_REG));
        return 0;
    }
#endif

    if (RAW_READL_DBG(AK_CHIP_VERSION_REG) == 0x53533501
        && (pin == 70 || pin == 71))
        enable = !enable;

    regval = RAW_READL_DBG(AK_PIN_PUPD_EN_REG(pin));
    if (enable) {
        regval |= (0x1 << ak_sharepin[pin].pupd_en_offset);
    } else {
        regval &= (~(0x1 << ak_sharepin[pin].pupd_en_offset));
    }
    RAW_WRITEL_DBG(regval, AK_PIN_PUPD_EN_REG(pin));

    ak_log_print("PIN_%d: PULL_EN(%d) 0x%p:0x%x\n",
        pin, enable,
        AK_PIN_PUPD_EN_REG(pin), RAW_READL_DBG(AK_PIN_PUPD_EN_REG(pin)));

    return 0;
}

static int pin_get_pull_enable(struct ak_pinctrl *pc, int pin)
{
    u32 regval;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return -EINVAL;
    }

    if (!(ak_sharepin[pin].gpio_cfg & AK_PIN_CFG_PUPD_EN)) {
        return -EPERM;
    }

    if (pin_pmc(pin)) {
        return -ENODEV;
    }

#ifdef CONFIG_MACH_AK3918EV300L
    if (pin_mipi(pin)) {
        regval = RAW_READL_DBG(AK_PIN_MIPI_REG);
        ak_log_print("%s 0x%p:0x%x\n", __func__, AK_PIN_MIPI_REG, regval);
        regval &= (0x1 << ak_sharepin[pin].pupd_en_offset);
        return (regval ? 1:0);
    }
#endif

    regval = RAW_READL_DBG(AK_PIN_PUPD_EN_REG(pin));

    ak_log_print("%s 0x%p:0x%x\n", __func__, AK_PIN_PUPD_EN_REG(pin), regval);

    regval &= (0x1 << ak_sharepin[pin].pupd_en_offset);

    return (regval ? 1:0);
}

static int pin_set_slew_rate(int pin, int fast)
{
    u32 regval;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return -EINVAL;
    }

    if (!(ak_sharepin[pin].gpio_cfg & AK_PIN_CFG_SLEW_RATE)) {
        return -EPERM;
    }

    if (pin_pmc(pin)) {
        return -ENODEV;
    }

    regval = RAW_READL_DBG(AK_PIN_SLEW_RATE_REG(pin));
    if (fast) {
        regval &= (~(0x1 << ak_sharepin[pin].pad_sl_offset));
    } else {
        regval |= (0x1 << ak_sharepin[pin].pad_sl_offset);
    }
    RAW_WRITEL_DBG(regval, AK_PIN_SLEW_RATE_REG(pin));

    ak_log_print("PIN_%d: SR(%d) 0x%p:0x%x\n", pin, fast,
            AK_PIN_SLEW_RATE_REG(pin),
            RAW_READL_DBG(AK_PIN_SLEW_RATE_REG(pin)));

    return 0;
}

static int pin_get_slew_rate(int pin)
{
    u32 regval;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return -EINVAL;
    }

    if (!(ak_sharepin[pin].gpio_cfg & AK_PIN_CFG_SLEW_RATE)) {
        return -EPERM;
    }

    if (pin_pmc(pin)) {
        return -ENODEV;
    }

    regval = RAW_READL_DBG(AK_PIN_SLEW_RATE_REG(pin));

    ak_log_print("%s 0x%p:0x%x\n", __func__, AK_PIN_SLEW_RATE_REG(pin), regval);

    regval &= (0x1 << ak_sharepin[pin].pad_sl_offset);

    return (regval ? 0 : 1);
}
static inline int gpio_set_share(struct ak_pinctrl *pc, unsigned int offset)
{
    unsigned long flags;

    local_irq_save(flags);
    pin_set_share(pc, offset, ak_sharepin[offset].gpio_func_no);
    local_irq_restore(flags);

    return 0;
}

static inline unsigned int ak_pinctrl_fsel_get(struct ak_pinctrl *pc,
                    unsigned int pin)
{
    int status = pc->fsel[pin];

    return status;
}

static inline void ak_pinctrl_fsel_set(struct ak_pinctrl *pc,
                    unsigned int pin, unsigned int sel)
{
    unsigned long flags;

    local_irq_save(flags);
    pin_set_share(pc, pin, sel);
    local_irq_restore(flags);
}

/**
*
*@brief: ak_gpio_set_drive
*@param[in] struct gpio_chip *chip
*@param[in] int pin
*@return: int
*
**/
static inline int ak_gpio_set_drive(struct gpio_chip *chip,
                    unsigned int offset, int strength)
{
    return pin_set_drive(offset, strength);
}

/**
*
*@brief: ak_gpio_get_drive
*@param[in] struct gpio_chip *chip
*@param[in] int offset
*@return: int
*
**/
static inline int ak_gpio_get_drive(struct gpio_chip *chip,
                    unsigned int offset)
{
    return pin_get_drive(offset);
}

/*
*
* gpiolib-sysfs.c
* -- pull_polarity_store
* ---- gpiod_set_pull_polarity
* IF pullup = 1 means pull up.
* else pullup = 0 mean pulldown
* or negative if any error
*
*/
/**
*@brief: ak_gpio_set_pull_polarity
*@param[in] struct gpio_chip *chip
*@param[in] int offset
*@param[in] int pullup
*@return: int
**/
static int ak_gpio_set_pull_polarity(struct gpio_chip *chip,
                    unsigned offset, int pullup)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);

    return pin_set_pull_polarity(pc, offset, pullup);
}

/*
*
* gpiolib-sysfs.c
* -- pull_polarity_show
* ---- gpiod_get_pull_polarity
* IF return > 0 means pullup
* else return 0 mean pulldown
* or negative if any error
*
*/
/**
*@brief: ak_gpio_get_pull_polarity
*@param[in] struct gpio_chip *chip
*@param[in] int offset
*@return: int
**/
static int ak_gpio_get_pull_polarity(struct gpio_chip *chip,
                    unsigned offset)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);

    return pin_get_pull_polarity(pc, offset);
}

/*
*
* gpiolib-sysfs.c
* -- pull_enable_store
* ---- gpiod_set_pull_enable
*
*/
/**
*@brief: ak_gpio_set_pull_enable
*@param[in] struct gpio_chip *chip
*@param[in] int offset
*@param[in] int enable
*@return: int
**/
static int ak_gpio_set_pull_enable(struct gpio_chip *chip,
                    unsigned offset, int enable)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);

    return pin_set_pull_enable(pc, offset, enable);
}

/*
*
* gpiolib-sysfs.c
* -- pull_enable_show
* ---- gpiod_get_pull_enable
*
*/
/**
*@brief: ak_gpio_get_pull_enable
*@param[in] struct gpio_chip *chip
*@param[in] int offset
*@return: int
**/
static int ak_gpio_get_pull_enable(struct gpio_chip *chip,
                    unsigned offset)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);

    return pin_get_pull_enable(pc, offset);
}

/*
*
* gpiolib-sysfs.c
* -- input_enable_store
* ---- gpiod_set_input_enable
*
*/
/**
*@brief: ak_gpio_set_input_enable
*@param[in] struct gpio_chip *chip
*@param[in] int offset
*@param[in] int enable
*@return: int
**/
static int ak_gpio_set_input_enable(struct gpio_chip *chip,
                    unsigned offset, int enable)
{
    return pin_set_input_enable(offset, enable);
}

/*
*
* gpiolib-sysfs.c
* -- input_enable_show
* ---- gpiod_get_input_enable
*
*/
/**
*
*@brief: ak_gpio_get_input_enable
*@param[in] struct gpio_chip *chip
*@param[in] int offset
*@return: int
*
**/
static int ak_gpio_get_input_enable(struct gpio_chip *chip,
    unsigned offset)
{
    return pin_get_input_enable(offset);
}

/*
*
* gpiolib-sysfs.c
* -- slew_rate_store
* ---- gpiod_set_slew_rate
*
*/
/**
*
*@brief: ak_gpio_set_slew_rate
*@param[in] struct gpio_chip *chip
*@param[in] int offset
*@param[in] int fast
*@return: int
*
**/
static int ak_gpio_set_slew_rate(struct gpio_chip *chip,
                    unsigned offset, int fast)
{
    return pin_set_slew_rate(offset, fast);
}

/*
*
* gpiolib-sysfs.c
* -- slew_rate_show
* ---- gpiod_get_slew_rate
*
*/
/**
*
*@brief: ak_gpio_get_slew_rate
*@param[in] struct gpio_chip *chip
*@param[in] int offset
*@return: int
*
**/
static int ak_gpio_get_slew_rate(struct gpio_chip *chip, unsigned offset)
{
    return pin_get_slew_rate(offset);
}

/**
*
*@brief: ak_pinctrl_fsel_reset
*@param[in] struct ak_pinctrl *pc
*@param[in] unsigned pin
*@param[in] unsigned fsel
*@return: int
*
**/
static inline void ak_pinctrl_fsel_reset(struct ak_pinctrl *pc,
                    unsigned pin, unsigned fsel)
{
    unsigned long flags;
    u32 regval;
    int cur = pc->fsel[pin];
    int offset = AK_PIN_CON_REG_OFFSET(pin);

    if (cur == -1)
        return;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID)
        return;

    if (pin_pmc(pin))
        return;

    if (ak_sharepin[pin].func_mux_max > 1) { // skip GPIO37 and GPIO50
        local_irq_save(flags);
        regval = RAW_READL_DBG(AK_PIN_CON_REG(pin));
        regval &= ~(0x7 << offset);
        RAW_WRITEL_DBG(regval, AK_PIN_CON_REG(pin));
        local_irq_restore(flags);

        ak_log_print("PIN_%d fsel reset 0x%p:0x%x\n", pin, AK_PIN_CON_REG(pin),
                RAW_READL_DBG(AK_PIN_CON_REG(pin)));
    }

    pc->fsel[pin] = -1;
}

/**
*
*@brief: ak_gpio_direction_input
*@param[in] struct gpio_chip *chip
*@param[in] int offset
*@return: int
*
**/
static int ak_gpio_direction_input(struct gpio_chip *chip,
    unsigned offset)
{
    return pinctrl_gpio_direction_input(chip->base + offset);
}

/**
*
*@brief: ak_gpio_get_direction
*@param[in] struct gpio_chip *chip
*@param[in] int offset
*@return: int
*
**/
/*
* if return 0 means output
* else any positive value means input
*/
static int ak_gpio_get_direction(struct gpio_chip *chip, unsigned offset)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);
    void __iomem * reg = pc->gpio_base + AK_GPIO_DIR_REG(offset);
    unsigned int bit = AK_GPIO_REG_SHIFT(offset);

    if (pin_pmc(offset)) {
        return -ENODEV;
    }

    return (RAW_READL_DBG(reg) & (1 << bit)) ? 0:1;
}

static int ak_gpio_get(struct gpio_chip *chip, unsigned offset)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);
    void __iomem * reg = pc->gpio_base + AK_GPIO_IN_REG(offset);
    void __iomem * reg_direction = pc->gpio_base + AK_GPIO_DIR_REG(offset);
    unsigned int bit = AK_GPIO_REG_SHIFT(offset);
    unsigned int direction = 1;

    if (pin_pmc(offset)) {
        return -ENODEV;
    } else {
        direction = ((RAW_READL_DBG(reg_direction) & (1 << bit)) ? 0:1);
        if (direction) {
            reg = pc->gpio_base + AK_GPIO_IN_REG(offset);
        } else {
            reg = pc->gpio_base + AK_GPIO_OUT_REG(offset);
        }
    }

    ak_log_print("GPIO_%d(%s): VAL(%d) 0x%p:0x%x\n",
        offset, direction ? "IN" : "OUT",
        ((RAW_READL_DBG(reg) & (1 << bit)) ? 1:0), reg, RAW_READL_DBG(reg));

    return (RAW_READL_DBG(reg) & (1 << bit)) ? 1:0;
}

static void ak_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);
    void __iomem *reg = pc->gpio_base + AK_GPIO_OUT_REG(offset);
    unsigned int bit = AK_GPIO_REG_SHIFT(offset);
    unsigned long flags;
    u32 regval;

    if (pin_pmc(offset)) {
        return;
    }

    local_irq_save(flags);
    if (AK_GPIO_OUT_LOW == value) {
        regval = RAW_READL_DBG(reg);
        regval &= ~(1 << bit);
        RAW_WRITEL_DBG(regval, reg);
    } else if (AK_GPIO_OUT_HIGH == value) {
        regval = RAW_READL_DBG(reg);
        regval |= (1 << bit);
        RAW_WRITEL_DBG(regval, reg);
    }
    local_irq_restore(flags);

    ak_log_print("PIN_%d: OUT_VAL(%d) 0x%p:0x%x\n", offset, value, reg,
        RAW_READL_DBG(reg));
}

static int ak_gpio_direction_output(struct gpio_chip *chip,
                    unsigned offset, int value)
{
    ak_gpio_set(chip, offset, value);
    return pinctrl_gpio_direction_output(chip->base + offset);
}

static int ak_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);

    return irq_linear_revmap(pc->irq_domain, offset);
}

static struct gpio_chip ak_gpio_chip = {
    .label = MODULE_NAME,
    .owner = THIS_MODULE,
    .request = gpiochip_generic_request,
    .free = gpiochip_generic_free,
    .get_direction = ak_gpio_get_direction,
    .direction_input = ak_gpio_direction_input,
    .direction_output = ak_gpio_direction_output,
    .get = ak_gpio_get,
    .set = ak_gpio_set,
    .set_drive = ak_gpio_set_drive,
    .get_drive = ak_gpio_get_drive,
    .set_pull_polarity = ak_gpio_set_pull_polarity,
    .get_pull_polarity = ak_gpio_get_pull_polarity,
    .set_pull_enable = ak_gpio_set_pull_enable,
    .get_pull_enable = ak_gpio_get_pull_enable,
    .set_input_enable = ak_gpio_set_input_enable,
    .get_input_enable = ak_gpio_get_input_enable,
    .set_slew_rate = ak_gpio_set_slew_rate,
    .get_slew_rate = ak_gpio_get_slew_rate,
    .to_irq = ak_gpio_to_irq,
    .base = 0,
    .ngpio = AK_NUM_GPIOS,
    .can_sleep = false,
};

static void ak_pinctrl_irq_handler(struct irq_desc *desc)
{
    struct irq_chip *chip = irq_desc_get_chip(desc);
    struct ak_pinctrl *pc = irq_desc_get_handler_data(desc);
    unsigned long irq_status;
    int i, pin_irq;

    for (i = 0; i < 4; i++) {
        int off = 0;
        chained_irq_enter(chip, desc);
        irq_status = RAW_READL_DBG(pc->gpio_base +
                            AK_GPIO_EDGE_STAT0_REG + i * 4);
        for_each_set_bit(off, &irq_status, 32) {
            if (i * 32 + off >= AK_NUM_GPIOS)
                break;

            if (pin_pmc(i * 32 + off)) {
                continue;
            }

            pin_irq = irq_find_mapping(pc->irq_domain, i * 32 + off);
            generic_handle_irq(pin_irq);
        }
        chained_irq_exit(chip, desc);
    }
}

static void ak_gpio_irq_enable(struct irq_data *data)
{
    struct ak_pinctrl *pc = irq_data_get_irq_chip_data(data);
    unsigned pin = irqd_to_hwirq(data);
    void __iomem *reg = pc->gpio_base + AK_GPIO_INTEN_REG(pin);
    unsigned bit = AK_GPIO_REG_SHIFT(pin);
    unsigned long flags;
    u32 regval;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return;
    }

    if (pin_pmc(pin)) {
        return;
    }

    spin_lock_irqsave(&pc->lock, flags);
    regval = RAW_READL_DBG(reg);
    regval |= (1 << bit);
    RAW_WRITEL_DBG(regval, reg);
    spin_unlock_irqrestore(&pc->lock, flags);
}

static void ak_gpio_irq_disable(struct irq_data *data)
{
    struct ak_pinctrl *pc = irq_data_get_irq_chip_data(data);
    unsigned pin = irqd_to_hwirq(data);
    void __iomem *reg = pc->gpio_base + AK_GPIO_INTEN_REG(pin);
    unsigned bit = AK_GPIO_REG_SHIFT(pin);
    unsigned long flags;
    u32 regval;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return;
    }

    if (pin_pmc(pin)) {
        return;
    }

    spin_lock_irqsave(&pc->lock, flags);
    regval = RAW_READL_DBG(reg);
    regval &= ~(1 << bit);
    RAW_WRITEL_DBG(regval, reg);
    spin_unlock_irqrestore(&pc->lock, flags);
}

/* slower path for reconfiguring IRQ type */
static int __ak_gpio_irq_set_type_enabled(struct ak_pinctrl *pc,
    unsigned offset, unsigned int type)
{
    u32 regval;
    void __iomem *reg_inten = pc->gpio_base + AK_GPIO_INTEN_REG(offset);
    void __iomem *reg_intm  = pc->gpio_base + AK_GPIO_INT_MODE_REG(offset);
    void __iomem *reg_intp  = pc->gpio_base + AK_GPIO_INTPOL_BASE(offset);
    void __iomem *reg_inten_double = pc->gpio_base +
                            AK_GPIO_DBLEDGE_EN_BASE(offset);
    int bit = AK_GPIO_REG_SHIFT(offset);
    /*
    * reg_inten/AK_GPIO_INTEN_REG
    * GPIO Interrupt Enable Register.
    * write 1 to enable the specific gpio's interrupt
    * write 0 to disable the specific gpio's interrupt
    *
    * reg_intm/AK_GPIO_INT_MODE_REG
    * GPIO Interrupt Mode Select Register.Interrupt trigger mode
    * write 1 to make edge-trigger
    * write 0 to make level-trigger
    *
    * reg_intp/AK_GPIO_INTPOL_BASE
    * GPIO Interrupt Polarity Select Register.Interrupt "input" polarity
    * Edge-trigger
    * write 0 to rising edge
    * write 1 to falling edge
    *
    * Level-trigger
    * write 0 to active high
    * write 1 to active low
    */

    if (ak_sharepin[offset].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, offset);
        return -ENODEV;
    }

    if (pin_pmc(offset)) {
        return -ENODEV;
    }

    switch (type) {
        case IRQ_TYPE_NONE:
            if (pc->irq_type[offset] != type) {
                regval = RAW_READL_DBG(reg_inten);
                regval &= ~(1 << bit);
                RAW_WRITEL_DBG(regval, reg_inten);

                regval = RAW_READL_DBG(reg_inten_double);
                regval &= ~(1 << bit);
                RAW_WRITEL_DBG(regval, reg_inten_double);

                pc->irq_type[offset] = type;
            }
            break;
        case IRQ_TYPE_EDGE_RISING:
        case IRQ_TYPE_EDGE_FALLING:
            if (pc->irq_type[offset] != type) {
                /* Make the edge-trigger */
                regval = RAW_READL_DBG(reg_intm);
                regval |= (1 << bit);
                RAW_WRITEL_DBG(regval, reg_intm);

                regval = RAW_READL_DBG(reg_inten_double);
                regval &= ~(1 << bit);
                RAW_WRITEL_DBG(regval, reg_inten_double);

                /* Config the trigger way*/
                regval = RAW_READL_DBG(reg_intp);
                if (IRQ_TYPE_EDGE_RISING == type)
                    regval &= ~(1 << bit);
                else
                    regval |= (1 << bit);
                RAW_WRITEL_DBG(regval, reg_intp);
                pc->irq_type[offset] = type;
            }
            break;
        case IRQ_TYPE_EDGE_BOTH:
            if (pc->irq_type[offset] != type) {
                /* Make the edge-trigger */
                regval = RAW_READL_DBG(reg_intm);
                regval |= (1 << bit);
                RAW_WRITEL_DBG(regval, reg_intm);

                regval = RAW_READL_DBG(reg_inten_double);
                regval |= (1 << bit);
                RAW_WRITEL_DBG(regval, reg_inten_double);

                pc->irq_type[offset] = type;
            }
            break;

        case IRQ_TYPE_LEVEL_HIGH:
        case IRQ_TYPE_LEVEL_LOW:
            if (pc->irq_type[offset] != type) {
                /* Make the level-trigger */
                regval = RAW_READL_DBG(reg_intm);
                regval &= ~(1 << bit);
                RAW_WRITEL_DBG(regval, reg_intm);

                /* Config the trigger way*/
                regval = RAW_READL_DBG(reg_intp);
                if (IRQ_TYPE_LEVEL_HIGH == type)
                    regval &= ~(1 << bit);
                else
                    regval |= (1 << bit);
                RAW_WRITEL_DBG(regval, reg_intp);
                pc->irq_type[offset] = type;
            }
            break;

        default:
            return -EINVAL;
    }
    return 0;
}
/*end of __ak_gpio_irq_set_type_enabled*/

static int ak_gpio_irq_set_type(struct irq_data *data, unsigned int type)
{
    struct ak_pinctrl *pc = irq_data_get_irq_chip_data(data);
    unsigned pin = irqd_to_hwirq(data);
    unsigned long flags;
    int ret;

    spin_lock_irqsave(&pc->lock, flags);

    /*
    * First irq_set_type called by __setup_irq,
    * and then irq_enable  called by __setup_irq.
    * refer to kernel/irq/manage.c.  fixed by zhang zhipeng (2019/5/3)
    */
    ret = __ak_gpio_irq_set_type_enabled(pc, pin, type);

    if (type & IRQ_TYPE_EDGE_BOTH)
        irq_set_handler_locked(data, handle_edge_irq);
    else
        irq_set_handler_locked(data, handle_level_irq);
    spin_unlock_irqrestore(&pc->lock, flags);

    return ret;
}

/*
 *  irq_ack called by handle_edge_irq when using edge trigger mode.
 *  Author:zhang zhipeng
 *  date: 2019-5-3
 */
static void ak_gpio_irq_ack(struct irq_data *d)
{
    struct ak_pinctrl *pc = irq_data_get_irq_chip_data(d);
    unsigned pin = irqd_to_hwirq(d);

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return;
    }

    /*
    * Clear the IRQ status
    */
    if (pin_pmc(pin)) {
        return;
    } else {
        RAW_READL_DBG(pc->gpio_base + AK_GPIO_INTEDGE_BASE(pin));
    }
}

static struct irq_chip ak_gpio_irq_chip = {
    .name = "ak_gpio_edge",
    .irq_ack    = ak_gpio_irq_ack,
    .irq_enable = ak_gpio_irq_enable,
    .irq_disable = ak_gpio_irq_disable,
    .irq_set_type = ak_gpio_irq_set_type,
    .irq_mask = ak_gpio_irq_disable,
    .irq_mask_ack = ak_gpio_irq_disable,
    .irq_unmask = ak_gpio_irq_enable,
};

static int ak_pctl_get_groups_count(struct pinctrl_dev *pctldev)
{
    return ARRAY_SIZE(ak_gpio_groups);
}

static const char *ak_pctl_get_group_name(struct pinctrl_dev *pctldev,
        unsigned selector)
{
    return ak_gpio_groups[selector];
}

static int ak_pctl_get_group_pins(struct pinctrl_dev *pctldev,
        unsigned selector,
        const unsigned **pins,
        unsigned *num_pins)
{
    *pins = &ak_gpio_pins[selector].number;
    *num_pins = 1;

    return 0;
}

static void ak_pctl_pin_dbg_show(struct pinctrl_dev *pctldev,
        struct seq_file *s,
        unsigned offset)
{
    struct ak_pinctrl *pc = pinctrl_dev_get_drvdata(pctldev);
    unsigned int fsel = ak_pinctrl_fsel_get(pc, offset);
    const char *fname = ak_funcs[fsel];
    int irq = irq_find_mapping(pc->irq_domain, offset);

    seq_printf(s, "function %s; irq %d (%s)",
        fname, irq, irq_type_names[pc->irq_type[offset]]);
}

static void ak_pctl_dt_free_map(struct pinctrl_dev *pctldev,
        struct pinctrl_map *maps, unsigned num_maps)
{
    int i;

    for (i = 0; i < num_maps; i++)
        if (maps[i].type == PIN_MAP_TYPE_CONFIGS_PIN)
            kfree(maps[i].data.configs.configs);

    kfree(maps);
}

static int ak_pctl_dt_node_to_map_func(struct ak_pinctrl *pc,
        struct device_node *np, u32 pin, u32 fnum,
        struct pinctrl_map **maps)
{
    struct pinctrl_map *map = *maps;

    if (fnum >= ARRAY_SIZE(ak_funcs)) {
        dev_err(pc->dev, "%s: invalid anyka,function %d\n",
            of_node_full_name(np), fnum);
        return -EINVAL;
    }

    map->type = PIN_MAP_TYPE_MUX_GROUP;
    map->data.mux.group = ak_gpio_groups[pin];
    map->data.mux.function = ak_funcs[fnum];
    (*maps)++;

    return 0;
}

static int ak_pctl_dt_node_to_map_pull(struct ak_pinctrl *pc,
        struct device_node *np, u32 pin, u32 pull,
        struct pinctrl_map **maps)
{
    struct pinctrl_map *map = *maps;
    unsigned long *configs;

    configs = kzalloc(sizeof(*configs), GFP_KERNEL);
    if (!configs)
        return -ENOMEM;
    configs[0] = pull;

    map->type = PIN_MAP_TYPE_CONFIGS_PIN;
    map->data.configs.group_or_pin = ak_gpio_pins[pin].name;
    map->data.configs.configs = configs;
    map->data.configs.num_configs = 1;
    (*maps)++;

    return 0;
}

static int ak_pctl_dt_node_to_map(struct pinctrl_dev *pctldev,
        struct device_node *np,
        struct pinctrl_map **map, unsigned *num_maps)
{
    struct ak_pinctrl *pc = pinctrl_dev_get_drvdata(pctldev);
    struct property *pins, *funcs, *pulls;
    int num_pins, num_funcs, num_pulls, maps_per_pin;
    struct pinctrl_map *maps, *cur_map;
    int i, err;
    u32 pin, func, pull;

    pins = of_find_property(np, "anyka,pins", NULL);
    if (!pins) {
        dev_err(pc->dev, "%s: missing anyka,pins property\n",
                of_node_full_name(np));
        return -EINVAL;
    }

    funcs = of_find_property(np, "anyka,function", NULL);
    pulls = of_find_property(np, "anyka,pull", NULL);

    if (!funcs && !pulls) {
        dev_err(pc->dev,
            "%s: neither anyka,function nor anyka,pull specified\n",
            of_node_full_name(np));
        return -EINVAL;
    }

    num_pins = pins->length / 4;
    num_funcs = funcs ? (funcs->length / 4) : 0;
    num_pulls = pulls ? (pulls->length / 4) : 0;

    if (num_funcs > 1 && num_funcs != num_pins) {
        dev_err(pc->dev,
            "%s: anyka,function must have 1 or %d entries\n",
            of_node_full_name(np), num_pins);
        return -EINVAL;
    }

    if (num_pulls > 1 && num_pulls != num_pins) {
        dev_err(pc->dev,
            "%s: anyka,pull must have 1 or %d entries\n",
            of_node_full_name(np), num_pins);
        return -EINVAL;
    }

    maps_per_pin = 0;
    if (num_funcs)
        maps_per_pin++;
    if (num_pulls)
        maps_per_pin++;
    cur_map = maps = kzalloc(num_pins * maps_per_pin * sizeof(*maps),
                GFP_KERNEL);
    if (!maps)
        return -ENOMEM;

    for (i = 0; i < num_pins; i++) {
        err = of_property_read_u32_index(np, "anyka,pins", i, &pin);
        if (err)
            goto out;
        if (pin >= ARRAY_SIZE(ak_gpio_pins)) {
            dev_err(pc->dev, "%s: invalid anyka,pins value %d\n",
                of_node_full_name(np), pin);
            err = -EINVAL;
            goto out;
        }
        if (num_funcs) {
            err = of_property_read_u32_index(np, "anyka,function",
                    (num_funcs > 1) ? i : 0, &func);
            if (err)
                goto out;
            err = ak_pctl_dt_node_to_map_func(pc, np, pin,
                            func, &cur_map);
            if (err)
                goto out;
        }
        if (num_pulls) {
            err = of_property_read_u32_index(np, "anyka,pull",
                    (num_pulls > 1) ? i : 0, &pull);
            if (err)
                goto out;
            err = ak_pctl_dt_node_to_map_pull(pc, np, pin,
                            pull, &cur_map);
            if (err)
                goto out;
        }
    }

    *map = maps;
    *num_maps = num_pins * maps_per_pin;
    return 0;

out:
    kfree(maps);
    return err;
}
/*end of ak_pctl_dt_node_to_map*/

static const struct pinctrl_ops ak_pctl_ops = {
    .get_groups_count = ak_pctl_get_groups_count,
    .get_group_name = ak_pctl_get_group_name,
    .get_group_pins = ak_pctl_get_group_pins,
    .pin_dbg_show = ak_pctl_pin_dbg_show,
    .dt_node_to_map = ak_pctl_dt_node_to_map,
    .dt_free_map = ak_pctl_dt_free_map,
};

static int ak_pmx_get_functions_count(struct pinctrl_dev *pctldev)
{
    return ARRAY_SIZE(ak_funcs);
}

static const char *ak_pmx_get_function_name(struct pinctrl_dev *pctldev,
        unsigned selector)
{
    return ak_funcs[selector];
}

static int ak_pmx_get_function_groups(struct pinctrl_dev *pctldev,
        unsigned selector,
        const char * const **groups,
        unsigned * const num_groups)
{
    *groups = ak_gpio_groups;
    *num_groups = ARRAY_SIZE(ak_gpio_groups);

    return 0;
}

static int ak_pmx_set(struct pinctrl_dev *pctldev,
        unsigned func_selector,
        unsigned group_selector)
{
    struct ak_pinctrl *pc = pinctrl_dev_get_drvdata(pctldev);

    ak_pinctrl_fsel_set(pc, group_selector, func_selector);
    return 0;
}

static void ak_pmx_gpio_disable_free(struct pinctrl_dev *pctldev,
        struct pinctrl_gpio_range *range,
        unsigned offset)
{
    struct ak_pinctrl *pc = pinctrl_dev_get_drvdata(pctldev);

    /* disable by setting sharepin cfg to default value */
    ak_pinctrl_fsel_reset(pc, offset, pc->fsel[offset]);
}

static int ak_pmx_gpio_set_direction(struct pinctrl_dev *pctldev,
        struct pinctrl_gpio_range *range,
        unsigned offset,
        bool input)
{
    struct ak_pinctrl *pc = pinctrl_dev_get_drvdata(pctldev);
    unsigned long flags;
    u32 regval;
    void __iomem *reg = pc->gpio_base + AK_GPIO_DIR_REG(offset);
    unsigned int bit = AK_GPIO_REG_SHIFT(offset);
    int gpio_cfg = input ?
        AK_PIN_AS_GPIO_IN : AK_PIN_AS_GPIO_OUT;

    if (ak_sharepin[offset].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, offset);
        return -ENODEV;
    }

    if (pin_pmc(offset))
        return -ENODEV;

    local_irq_save(flags);

    if (AK_PIN_AS_GPIO_IN == gpio_cfg) {
        regval = RAW_READL_DBG(reg);
        regval &= ~(1 << bit);
        RAW_WRITEL_DBG(regval, reg);
    } else if (AK_PIN_AS_GPIO_OUT == gpio_cfg){
        regval = RAW_READL_DBG(reg);
        regval |= (1 << bit);
        RAW_WRITEL_DBG(regval, reg);
    }

    gpio_set_share(pc, offset);

    local_irq_restore(flags);

    return 0;
}

static const struct pinmux_ops ak_pmx_ops = {
    .get_functions_count = ak_pmx_get_functions_count,
    .get_function_name = ak_pmx_get_function_name,
    .get_function_groups = ak_pmx_get_function_groups,
    .set_mux = ak_pmx_set,
    .gpio_disable_free = ak_pmx_gpio_disable_free,
    .gpio_set_direction = ak_pmx_gpio_set_direction,
};

static int ak_pinconf_get(struct pinctrl_dev *pctldev,
            unsigned pin, unsigned long *config)
{
    /* No way to read back config in HW */
    return -ENOTSUPP;
}

static int ak_pinconf_set(struct pinctrl_dev *pctldev,
            unsigned pin, unsigned long *configs,
            unsigned num_configs)
{
    unsigned long flags;
    u8 pupd, drive, ie, slew, dir, output;
    struct ak_pinctrl *pc = pinctrl_dev_get_drvdata(pctldev);

    if (num_configs != 1)
        return -EINVAL;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        pr_err("%s pin_%d not support\n", __func__, pin);
        return -ENODEV;
    }

    /*
    * configs
    * bit[0] pull up or pull down selection
    * bit[4] pull up or pull down enable
    * bit[9:8] drive
    * bit[12:15] gpio direction
    *   bit[12]: direction. 0 is output, 1 is input
    *   bit[15]: enable direction. 1 is enable
    * bit[16]:input enable
    * bit[20-23]: gpio output level
    *   bit[20]: 0 is low level, 1 is high level
    *   bit[23]: enable gpio output level. 1 is enable
    * bit[24]: slew rate
    */
    pupd = (configs[0] & 0xFF);
    drive = ((configs[0]>>8) & 0x0F);
    dir = ((configs[0]>>12) & 0x0F);
    ie = ((configs[0]>>16) & 0x0F);
    output = ((configs[0]>>20) & 0x0F);
    slew = ((configs[0]>>24) & 0xFF);

    local_irq_save(flags);
    pin_set_pull_enable(pc, pin, (pupd & 0x10) ? 1:0);
    pin_set_pull_polarity(pc, pin, (pupd & 0x01) ? 0:1);
    pin_set_drive(pin, drive & 0x3);
    pin_set_input_enable(pin, ie & 0x1);
    pin_set_slew_rate(pin, slew & 0x1);

    if (dir & 0x8) { // gpio direction enable?
        if (dir & 1) {
            ak_gpio_direction_input(&pc->gc, pin);
        } else {
            pinctrl_gpio_direction_output(pc->gc.base + pin);
        }
    }
    if ((output & 0x8) && (dir & 0x8) && ((dir & 1) == 0)) {
            ak_gpio_set(&pc->gc, pin, (output & 1));
    }
    local_irq_restore(flags);

    return 0;
}

/* for gpio PU/PD config */
static const struct pinconf_ops ak_pinconf_ops = {
    .pin_config_get = ak_pinconf_get,
    .pin_config_set = ak_pinconf_set,
};

static struct pinctrl_desc ak_pinctrl_desc = {
    .name = MODULE_NAME,
    .pins = ak_gpio_pins,
    .npins = ARRAY_SIZE(ak_gpio_pins),
    .pctlops = &ak_pctl_ops,
    .pmxops = &ak_pmx_ops,
    .confops = &ak_pinconf_ops,
    .owner = THIS_MODULE,
};

static struct pinctrl_gpio_range ak_pinctrl_gpio_range = {
    .name = MODULE_NAME,
    .npins = AK_NUM_GPIOS,
};

#ifdef CONFIG_PM

#define AK_WAKEUP_GPIO_CNT              32

static int ak_wakeup_gpio[AK_WAKEUP_GPIO_CNT] = {-1};
static int wakeup_gpio_cnt = 0;

u32 ak_ppu_ppd_en[4];
u32 ak_gpio_dir_sel_reg[4];

#ifdef CONFIG_MACH_KM01A
struct ak_reg_value {
    void *addr;
    u32 val;
};

static struct ak_reg_value ak_pinctrl_regs[] = {
    {AK_PIN_CON0_REG, 0},
    {AK_PIN_CON0_REG + 1 * 4, 0},
    {AK_PIN_CON0_REG + 2 * 4, 0},
    {AK_PIN_CON0_REG + 3 * 4, 0},
    {AK_PIN_CON0_REG + 4 * 4, 0},
    {AK_PIN_CON0_REG + 5 * 4, 0},
    {AK_PIN_CON0_REG + 6 * 4, 0},
    {AK_PIN_CON0_REG + 7 * 4, 0},
    {AK_PIN_CON0_REG + 8 * 4, 0},
    {AK_PIN_CON0_REG + 9 * 4, 0},
    {AK_PIN_CON0_REG + 10 * 4, 0},
    {AK_PIN_DRV_CON0_REG, 0},
    {AK_PIN_DRV_CON0_REG + 1 * 4, 0},
    {AK_PIN_DRV_CON0_REG + 2 * 4, 0},
    {AK_PIN_DRV_CON0_REG + 3 * 4, 0},
    {AK_PIN_DRV_CON0_REG + 4 * 4, 0},
    {AK_PIN_IE_CON0_REG, 0},
    {AK_PIN_IE_CON0_REG + 1 * 4, 0},
    {AK_PIN_IE_CON0_REG + 2 * 4, 0},
    {AK_PIN_IE_CON0_REG + 3 * 4, 0},
    {AK_PIN_SLEW_RATE0_REG, 0},
    {AK_PIN_SLEW_RATE0_REG + 1 * 4, 0},
    {AK_PIN_SLEW_RATE0_REG + 2 * 4, 0},
    {AK_PIN_SLEW_RATE0_REG + 3 * 4, 0},
    {AK_PIN_PUPD_EN0_REG, 0},
    {AK_PIN_PUPD_EN0_REG + 1 * 4, 0},
    {AK_PIN_PUPD_EN0_REG + 2 * 4, 0},
    {AK_PIN_PUPD_EN0_REG + 3 * 4, 0},
    {AK_PIN_PUPD_SEL0_REG, 0},
    {AK_PIN_PUPD_SEL0_REG + 1 * 4, 0},
    {AK_PIN_PUPD_SEL0_REG + 2 * 4, 0},
    {AK_PIN_PUPD_SEL0_REG + 3 * 4, 0},
    {AK_VA_SYSCTRL + 0x284, 0},
    {AK_VA_SYSCTRL + 0x284 + 1 * 4, 0},
    {AK_VA_SYSCTRL + 0x284 + 2 * 4, 0},
    {AK_VA_SYSCTRL + 0x284 + 3 * 4, 0},
    {AK_VA_SYSCTRL + 0x270, 0},
    {AK_VA_SYSCTRL + 0x280, 0},
    {AK_VA_SYSCTRL + 0x290, 0},
    {NULL, 0},
};

static void ak_save_reg_values(struct ak_reg_value *reg)
{
    while (reg->addr) {
        reg->val = __raw_readl(reg->addr);
        reg++;
    }
}

static void ak_resotre_reg_values(struct ak_reg_value *reg)
{
    while (reg->addr) {
        __raw_writel(reg->val, reg->addr);
        reg++;
    }
}

#if 0
static void ak_pinctrl_dump(void)
{
    struct ak_reg_value *p;
    p = ak_pinctrl_regs;
    while (p->addr) {
        pr_info("[0x%p]: 0x%x\n", p->addr, p->val);
        p++;
    }
}
#endif

static int ak_gpio_suspend(void)
{
    struct ak_reg_value *regs = ak_pinctrl_regs;
    ak_save_reg_values(regs);
    return 0;
}

static void ak_gpio_resume(void)
{
    struct ak_reg_value *regs = ak_pinctrl_regs;
    ak_resotre_reg_values(regs);
}

#elif defined (CONFIG_MACH_AK3918AV130)

#define ak_gpio_suspend NULL
#define ak_gpio_resume      NULL


#else

static int ak_gpio_suspend(void)
{
    u32 regval;
    int gpio_index = 0;
    struct device_node *np;
    int reset_pin, power_pin, backlight_pin;
    int i;

    /*
    * When suspend make all gpio input and disable pull-up/down
    */
    for (gpio_index = 0; gpio_index < wakeup_gpio_cnt; gpio_index++) {
        pr_info("%s GPIO#%d\n", __func__, ak_wakeup_gpio[gpio_index]);
    }

    /* store value*/
    for (i = 0; i < 4; i++) {
        ak_ppu_ppd_en[i] = RAW_READL_DBG(AK_PIN_PUPD_EN_REG(i));
        ak_gpio_dir_sel_reg[i] = RAW_READL_DBG(AK_GPIO_DIR_REG(i));
    }

    for (i = 0; i < ARRAY_SIZE(ak_sharepin); i++) {
        /* check gpio index i whether as wakeup gpio or not */
        for (gpio_index = 0; gpio_index < wakeup_gpio_cnt; gpio_index++){
            if (i == ak_wakeup_gpio[gpio_index]){
                break;/* here find wakeup gpio */
            }
        }

        if (gpio_index != wakeup_gpio_cnt){
            continue; /* here find wakeup gpio, continue find next */
        }

        /* make other gpio suspend */
        regval = RAW_READL_DBG(AK_PIN_PUPD_EN_REG(i));
        regval &= ~(0x1 << ak_sharepin[i].pupd_en_offset);
        RAW_WRITEL_DBG(regval, AK_PIN_PUPD_EN_REG(i));


        regval = RAW_READL_DBG(AK_GPIO_DIR_REG(i));
        regval &= ~(0x1 << AK_GPIO_OFFSET(i));
        RAW_WRITEL_DBG(regval, AK_GPIO_DIR_REG(i));
    }

    return 0;
}

static void ak_gpio_resume(void)
{
    int gpio_index = 0;

    for (gpio_index = 0; gpio_index < wakeup_gpio_cnt; gpio_index++) {
        pr_info("%s GPIO#%d\n", __func__, ak_wakeup_gpio[gpio_index]);
    }

    for (i = 0; i < 4; i++) {
        RAW_WRITEL_DBG(ak_ppu_ppd_en[i], AK_PIN_PUPD_EN_REG(i));
        RAW_WRITEL_DBG(ak_gpio_dir_sel_reg[i], AK_GPIO_DIR_REG(i));
    }
}

#endif

#else
#define ak_gpio_suspend NULL
#define ak_gpio_resume      NULL
#endif

struct syscore_ops ak_gpio_syscore_ops = {
    .suspend    = ak_gpio_suspend,
    .resume     = ak_gpio_resume,
};

static int ak_pinctrl_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *np = dev->of_node;
    struct ak_pinctrl *pc;
    struct resource iomem;
    int ret, i;
#ifdef CONFIG_PM
    int wakeup_mode_cnt = 0, wakeup_mode = 0;
    int gpio_index = 0;
#endif

    BUILD_BUG_ON(ARRAY_SIZE(ak_gpio_pins) != AK_NUM_GPIOS);

    dev_info(dev, "%s %d\n",__func__,__LINE__);

    pc = devm_kzalloc(dev, sizeof(*pc), GFP_KERNEL);
    if (!pc)
        return -ENOMEM;

    platform_set_drvdata(pdev, pc);
    pc->dev = dev;

    /* gpio registers */
    ret= of_address_to_resource(np, 0, &iomem);
    if (ret) {
        dev_err(dev, "could not get IO memory\n");
        return ret;
    }

    pc->gpio_base = devm_ioremap_resource(dev, &iomem);
    if (IS_ERR(pc->gpio_base))
        return PTR_ERR(pc->gpio_base);
    pc->gpio_base_pa = iomem.start;
    ak_gpio_base = pc->gpio_base;
    ak_gpio_base_pa = pc->gpio_base_pa;
    dev_info(dev, "gpio_base 0x%x gpio_base_pa 0x%x\n",
            (u32)(pc->gpio_base), (u32)(pc->gpio_base_pa));

    pc->gc = ak_gpio_chip;
    pc->gc.dev = dev;
    pc->gc.of_node = np;

    pc->clk = devm_clk_get(&pdev->dev, NULL);
    if (IS_ERR(pc->clk)) {
        ret = PTR_ERR(pc->clk);
        return ret;
    }

    clk_prepare_enable(pc->clk);

    pc->irq = platform_get_irq(pdev, 0);
    if (pc->irq < 0) {
        ret = pc->irq;
        goto clk_err;
    }

    pr_info("ak_pinctrl_probe irq: %d\n", pc->irq);

    pc->irq_domain = irq_domain_add_linear(np, AK_NUM_GPIOS,
            &irq_domain_simple_ops, pc);
    if (!pc->irq_domain) {
        dev_err(dev, "could not create IRQ domain\n");
        ret = -ENOMEM;
        goto clk_err;
    }

    for (i = 0; i < ARRAY_SIZE(ak_sharepin); i++) {
        int irq, pin;

        /*
        * These special pins cannot be interrupted:
        *   GPIO[15:13] GPIO[28:29] GPIO[76:76] GPIO[82:97]
        * So, don't map them!
        */
        if(ak_sharepin[i].func_mux_max == AK_GPIO_CONF_INVALID) {
            continue;
        }

        pin = ak_sharepin[i].pin;
        irq = irq_create_mapping(pc->irq_domain, pin);
        pr_debug("pin = %d irq = %d\n", pin, irq);
        pc->irq_type[pin] = 0;
        pc->fsel[pin] = -1;
        irq_set_chip_and_handler(irq, &ak_gpio_irq_chip,
                handle_level_irq);
        irq_set_chip_data(irq, pc);

        // let all pin default to GPIO
#if !defined(CONFIG_ANYKA_FASTSYS)
        pin_set_share(pc, i, ak_sharepin[i].gpio_func_no);
#endif
    }

    irq_set_chained_handler_and_data(pc->irq,
                         ak_pinctrl_irq_handler, pc);

    ret = gpiochip_add(&pc->gc);
    if (ret) {
        dev_err(dev, "could not add GPIO chip\n");
        goto clk_err;
    }

    pc->pctl_dev = pinctrl_register(&ak_pinctrl_desc, dev, pc);
    if (IS_ERR(pc->pctl_dev)) {
        gpiochip_remove(&pc->gc);
        ret = PTR_ERR(pc->pctl_dev);
        goto clk_err;
    }

    pc->gpio_range = ak_pinctrl_gpio_range;
    pc->gpio_range.base = pc->gc.base;
    pc->gpio_range.gc = &pc->gc;
    pinctrl_add_gpio_range(pc->pctl_dev, &pc->gpio_range);

#ifdef CONFIG_PM
    np = of_find_node_by_name(NULL, "pm_standby");
    if (np && of_device_is_available(np)) {
        wakeup_mode_cnt = of_property_count_u32_elems(np, "wakeup-mode");
        if (wakeup_mode_cnt > 0) {
            for (i = 0; i < wakeup_mode_cnt; i++) {
                ret = of_property_read_u32_index(np,
                            "wakeup-mode", i, &wakeup_mode);
                if (ret) {
                    for (gpio_index=0; gpio_index<AK_WAKEUP_GPIO_CNT;
                        gpio_index++){
                        ak_wakeup_gpio[gpio_index] = -1;
                    }
                    break;
                } else if (wakeup_mode == 0x1) {
                    wakeup_gpio_cnt = of_gpio_named_count(np, "wakeup-gpio");
                    if (wakeup_gpio_cnt < 0){
                        pr_info("%s, line:%d, ERR: can not get wakeup-gpio \
                            cnt!\n", __func__, __LINE__);
                        
                        for (gpio_index=0; gpio_index<AK_WAKEUP_GPIO_CNT;
                            gpio_index++){
                            ak_wakeup_gpio[gpio_index] = -1;
                        }
                        continue;
                    }

                    /* get wakeup gpio index */
                    for (gpio_index = 0; gpio_index < wakeup_gpio_cnt;
                        gpio_index++) {
                        ret = of_get_named_gpio(np, "wakeup-gpio", gpio_index);
                        ak_wakeup_gpio[gpio_index] = ret;
                    }
                }
            }
        } else {
            for (gpio_index=0; gpio_index<AK_WAKEUP_GPIO_CNT; gpio_index++){
                ak_wakeup_gpio[gpio_index] = -1;
            }
        }
    } else {
        for (gpio_index=0; gpio_index<AK_WAKEUP_GPIO_CNT; gpio_index++){
            ak_wakeup_gpio[gpio_index] = -1;
        }
    }

    for (gpio_index = 0; gpio_index < wakeup_gpio_cnt; gpio_index++) {
        pr_info("%s wakeup GPIO#(%d)\n", __func__, ak_wakeup_gpio[gpio_index]);
    }
    
#endif
    register_syscore_ops(&ak_gpio_syscore_ops);

    disable_gpio_retention();

    return 0;

clk_err:
    clk_disable_unprepare(pc->clk);
    clk_put(pc->clk);

    return ret;
}
/*end of ak_pinctrl_probe*/

static int ak_pinctrl_remove(struct platform_device *pdev)
{
    struct ak_pinctrl *pc = platform_get_drvdata(pdev);

    unregister_syscore_ops(&ak_gpio_syscore_ops);

    pinctrl_unregister(pc->pctl_dev);
    gpiochip_remove(&pc->gc);
    clk_disable_unprepare(pc->clk);
    clk_put(pc->clk);

    return 0;
}

static const struct of_device_id ak_pinctrl_match[] = {
    { .compatible = "anyka,ak3918av100-gpio" },
    { .compatible = "anyka,ak3918ev300l-gpio" },
    { .compatible = "anyka,km01a-gpio" },
    { .compatible = "anyka,ak3918av130-gpio" },
    {}
};
MODULE_DEVICE_TABLE(of, ak_pinctrl_match);

static struct platform_driver ak_pinctrl_driver = {
    .probe = ak_pinctrl_probe,
    .remove = ak_pinctrl_remove,
    .driver = {
        .name = MODULE_NAME,
        .of_match_table = ak_pinctrl_match,
    },
};

static int __init ak_pinctrl_init(void)
{
    return platform_driver_register(&ak_pinctrl_driver);
}

static void __exit ak_pinctrl_exit(void)
{
    platform_driver_unregister(&ak_pinctrl_driver);
}

subsys_initcall(ak_pinctrl_init);
module_exit(ak_pinctrl_exit);

MODULE_DESCRIPTION("AK3918AV100/AK3918EV300L sharepin control driver");
MODULE_AUTHOR("Anyka");
MODULE_LICENSE("GPL");
MODULE_VERSION("2.0.00");
