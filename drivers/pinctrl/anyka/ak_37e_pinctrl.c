/*
 * Driver for AK37E GPIO (pinctrl + GPIO)
 *
 * Copyright (C) 2018 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
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

#include <dt-bindings/pinctrl/ak_37e_pinctrl.h>


/*
* funcmux_reg: AK_SHAREPIN_CON0 + (GPIO/10)*4 + (GPIO%10)*3
* pad_drv_reg: AK_GPIO_DRIVE_CON0 + (GPIO/16)*4 + (GPIO%16)*2 !!Special!!
* pad_ie_reg: AK_GPIO_IE_CON0 + (GPIO/32)*4 + (GPIO%32) !!Special!!
* pupd_en_reg: AK_PPU_PPD_EN0 + (GPIO/32)*4 + (GPIO%32)
* pupd_sel_reg: AK_PPU_PPD_SEL0 + (GPIO/32)*4 + (GPIO%32)
* pad_sl_reg: AK_GPIO_SLEW_RATE0 + (GPIO/32)*4 + (GPIO%32)
*/

#define MODULE_NAME             "ak-pinctrl"
#define AK37E_NUM_GPIOS         97
#define AK37E_NUM_BANKS         1

#define AK_PULLUPDOWN_DISABLE   0
#define AK_PULLUPDOWN_ENABLE    1

#define ePIN_AS_GPIO_IN         0 // pin as gpio in
#define ePIN_AS_GPIO_OUT        1 // pin as gpio out

#define AK_GPIO_OUT_LOW         0
#define AK_GPIO_OUT_HIGH        1

#define AK_GPIO_CONF_INVALID    (-1)

#define AK_GPIO_DIR1            0x00
#define AK_GPIO_OUT1            0x14
#define AK_GPIO_INPUT1          0x28
#define AK_GPIO_INPUT3          0x34
#define AK_GPIO_INT_MASK1       0x3c
#define AK_GPIO_INT_MASK3       0x48
#define AK_GPIO_INT_MODE1       0x50
#define AK_GPIO_INT_MODE3       0x5C
#define AK_GPIO_INTP1           0x64
#define AK_GPIO_INTP3           0x70
#define AK_GPIO_EDGE_STATUS1    0x78
#define AK_GPIO_EDGE_STATUS3    0x84

#define AK_GPIO_DIR_BASE(pin)           (((pin)>>5)*4 + AK_GPIO_DIR1)
#define AK_GPIO_OUT_BASE(pin)           (((pin)>>5)*4 + AK_GPIO_OUT1)
#define AK_GPIO_IN_BASE(pin)            (((pin)>>5)*4 + AK_GPIO_INPUT1)
#define AK_GPIO_INTEN_BASE(pin)         (((pin)>>5)*4 + AK_GPIO_INT_MASK1)
#define AK_GPIO_INTM_BASE(pin)          (((pin)>>5)*4 + AK_GPIO_INT_MODE1)
#define AK_GPIO_INTPOL_BASE(pin)        (((pin)>>5)*4 + AK_GPIO_INTP1)
#define AK_GPIO_INTEDGE_BASE(pin)       (((pin)>>5)*4 + AK_GPIO_EDGE_STATUS1)

#define AK_GPIO_REG_SHIFT(pin)          ((pin) % 32)
#define AK_GPI_REG_SHIFT(pin)           ((pin) - 95 + 11)

#define AK_ANALOG_CTRL_REG2     (AK_VA_SYSCTRL + 0x0A0)
#define AK_ANALOG_CTRL_REG3     (AK_VA_SYSCTRL + 0x0A4)

#define AK_SHAREPIN_CON0        (AK_VA_SYSCTRL + 0x15C)
#define AK_SHAREPIN_CON1        (AK_VA_SYSCTRL + 0x160)
#define AK_SHAREPIN_CON2        (AK_VA_SYSCTRL + 0x164)
#define AK_SHAREPIN_CON3        (AK_VA_SYSCTRL + 0x168)
#define AK_SHAREPIN_CON4        (AK_VA_SYSCTRL + 0x16C)
#define AK_SHAREPIN_CON5        (AK_VA_SYSCTRL + 0x170)
#define AK_SHAREPIN_CON6        (AK_VA_SYSCTRL + 0x174)
#define AK_SHAREPIN_CON7        (AK_VA_SYSCTRL + 0x178)
#define AK_SHAREPIN_CON8        (AK_VA_SYSCTRL + 0x17C)
#define AK_SHAREPIN_CON9        (AK_VA_SYSCTRL + 0x180)

#define AK_GPIO_DRIVE_CON0      (AK_VA_SYSCTRL + 0x18C)
#define AK_GPIO_DRIVE_CON1      (AK_VA_SYSCTRL + 0x190)
#define AK_GPIO_DRIVE_CON2      (AK_VA_SYSCTRL + 0x194)
#define AK_GPIO_DRIVE_CON3      (AK_VA_SYSCTRL + 0x198)
#define AK_GPIO_DRIVE_CON4      (AK_VA_SYSCTRL + 0x19C)
#define AK_GPIO_DRIVE_CON5      (AK_VA_SYSCTRL + 0x1A0)

#define AK_PPU_PPD_EN0          (AK_VA_SYSCTRL + 0x1D0)
#define AK_PPU_PPD_EN1          (AK_VA_SYSCTRL + 0x1D4)
#define AK_PPU_PPD_EN2          (AK_VA_SYSCTRL + 0x1D8)

#define AK_PPU_PPD_SEL0         (AK_VA_SYSCTRL + 0x1E4)
#define AK_PPU_PPD_SEL1         (AK_VA_SYSCTRL + 0x1E8)
#define AK_PPU_PPD_SEL2         (AK_VA_SYSCTRL + 0x1EC)

#define AK_GPIO_SLEW_RATE0      (AK_VA_SYSCTRL + 0x220)
#define AK_GPIO_SLEW_RATE1      (AK_VA_SYSCTRL + 0x224)
#define AK_GPIO_SLEW_RATE2      (AK_VA_SYSCTRL + 0x228)

#define AK_GPIO_IE_CON0         (AK_VA_SYSCTRL + 0x234)
#define AK_GPIO_IE_CON1         (AK_VA_SYSCTRL + 0x238)
#define AK_GPIO_IE_CON2         (AK_VA_SYSCTRL + 0x23C)

#define AK_GPIO_PAD_ST_CON0     (AK_VA_SYSCTRL + 0x244)

#define AK_INTERRUPT_STATUS     (AK_VA_SYSCTRL + 0x30)
#define AK_ALWAYS_ON_PMU_CTRL0  (AK_VA_SYSCTRL + 0xDC)
#define AK_ALWAYS_ON_PMU_CTRL1  (AK_VA_SYSCTRL + 0xE0)

#define AK_GPIO_BASE            (AK_VA_SUBCTRL + 0x70000)
#define AK_GPIO_DIR_SEL_REG0    (AK_GPIO_BASE + 0x00)
#define AK_GPIO_DIR_SEL_REG1    (AK_GPIO_BASE + 0x04)
#define AK_GPIO_DIR_SEL_REG2    (AK_GPIO_BASE + 0x08)

#define AK_GPIO_GROUP          (32)
#define AK_GPIO_OFFSET(pin)    ((pin) % AK_GPIO_GROUP)
#define AK_GPIO_DIR_REG(pin)   (AK_GPIO_DIR_SEL_REG0 + ((pin)/AK_GPIO_GROUP)*4)

#define AK_SHAREPIN_CON_GROUP            (10)
#define AK_SHAREPIN_CON_REG(pin)         (AK_SHAREPIN_CON0 + \
                                         (pin/AK_SHAREPIN_CON_GROUP)*4)
#define AK_SHAREPIN_CON_REG_OFFSET(pin)  ((pin%AK_SHAREPIN_CON_GROUP)*3)

#define AK_GPIO_DRIVE_CON_GROUP          (16)
#define AK_GPIO_DRIVE_CON_REG(pin)       (AK_GPIO_DRIVE_CON0 + \
                                         (pin/AK_GPIO_DRIVE_CON_GROUP)*4)

#define AK_PPU_PPD_EN_GROUP              (32)
#define AK_PPU_PPD_EN_REG(pin)           (AK_PPU_PPD_EN0 + \
                                         (pin/AK_PPU_PPD_EN_GROUP)*4)

#define AK_PPU_PPD_SEL_GROUP             (32)
#define AK_PPU_PPD_SEL_REG(pin)          (AK_PPU_PPD_SEL0 + \
                                         (pin/AK_PPU_PPD_SEL_GROUP)*4)

#define AK_GPIO_SLEW_RATE_GROUP          (32)
#define AK_GPIO_SLEW_RATE_REG(pin)       (AK_GPIO_SLEW_RATE0 + \
                                         (pin/AK_GPIO_SLEW_RATE_GROUP)*4)

#define AK_GPIO_IE_GROUP                 (32)
#define AK_GPIO_IE_REG(pin)              (AK_GPIO_IE_CON0 + \
                                         (pin/AK_GPIO_IE_GROUP)*4)

#define AK37E_GPIO_PIN(a)                PINCTRL_PIN(a, "gpio" #a)

#define DRIVE_STRENGTH(strength)         (strength)

#undef AK_GPIO_IO_CONFIRMS
#ifdef AK_GPIO_IO_CONFIRMS
#define ak_log_print(fmt, arg...) pr_info(fmt, ##arg)
#else
#define ak_log_print(fmt, arg...) pr_debug(fmt, ##arg)
#endif

struct gpio_sharepin {
    int pin;

    int8_t pad_drv_offset;
    int8_t pad_ie_offset;
    int8_t pupd_en_offset;
    int8_t pupd_sel_offset;
    int8_t pad_sl_offset;
    int8_t func_mux_max;
    int8_t gpio_cfg;
};

struct ak_pinctrl {
    struct device *dev;
    struct clk *clk;
    void __iomem *gpio_base;
    int irq;

    unsigned int irq_type[AK37E_NUM_GPIOS];
    int fsel[AK37E_NUM_GPIOS];

    struct pinctrl_dev *pctl_dev;
    struct irq_domain *irq_domain;
    struct gpio_chip gc;
    struct pinctrl_gpio_range gpio_range;

    spinlock_t lock;
};

static struct pinctrl_pin_desc ak_gpio_pins[] = {
    AK37E_GPIO_PIN(0),
    AK37E_GPIO_PIN(1),
    AK37E_GPIO_PIN(2),
    AK37E_GPIO_PIN(3),
    AK37E_GPIO_PIN(4),
    AK37E_GPIO_PIN(5),
    AK37E_GPIO_PIN(6),
    AK37E_GPIO_PIN(7),
    AK37E_GPIO_PIN(8),
    AK37E_GPIO_PIN(9),
    AK37E_GPIO_PIN(10),
    AK37E_GPIO_PIN(11),
    AK37E_GPIO_PIN(12),
    AK37E_GPIO_PIN(13),
    AK37E_GPIO_PIN(14),
    AK37E_GPIO_PIN(15),
    AK37E_GPIO_PIN(16),
    AK37E_GPIO_PIN(17),
    AK37E_GPIO_PIN(18),
    AK37E_GPIO_PIN(19),
    AK37E_GPIO_PIN(20),
    AK37E_GPIO_PIN(21),
    AK37E_GPIO_PIN(22),
    AK37E_GPIO_PIN(23),
    AK37E_GPIO_PIN(24),
    AK37E_GPIO_PIN(25),
    AK37E_GPIO_PIN(26),
    AK37E_GPIO_PIN(27),
    AK37E_GPIO_PIN(28),
    AK37E_GPIO_PIN(29),
    AK37E_GPIO_PIN(30),
    AK37E_GPIO_PIN(31),
    AK37E_GPIO_PIN(32),
    AK37E_GPIO_PIN(33),
    AK37E_GPIO_PIN(34),
    AK37E_GPIO_PIN(35),
    AK37E_GPIO_PIN(36),
    AK37E_GPIO_PIN(37),
    AK37E_GPIO_PIN(38),
    AK37E_GPIO_PIN(39),
    AK37E_GPIO_PIN(40),
    AK37E_GPIO_PIN(41),
    AK37E_GPIO_PIN(42),
    AK37E_GPIO_PIN(43),
    AK37E_GPIO_PIN(44),
    AK37E_GPIO_PIN(45),
    AK37E_GPIO_PIN(46),
    AK37E_GPIO_PIN(47),
    AK37E_GPIO_PIN(48),
    AK37E_GPIO_PIN(49),
    AK37E_GPIO_PIN(50),
    AK37E_GPIO_PIN(51),
    AK37E_GPIO_PIN(52),
    AK37E_GPIO_PIN(53),
    AK37E_GPIO_PIN(54),
    AK37E_GPIO_PIN(55),
    AK37E_GPIO_PIN(56),
    AK37E_GPIO_PIN(57),
    AK37E_GPIO_PIN(58),
    AK37E_GPIO_PIN(59),
    AK37E_GPIO_PIN(60),
    AK37E_GPIO_PIN(61),
    AK37E_GPIO_PIN(62),
    AK37E_GPIO_PIN(63),
    AK37E_GPIO_PIN(64),
    AK37E_GPIO_PIN(65),
    AK37E_GPIO_PIN(66),
    AK37E_GPIO_PIN(67),
    AK37E_GPIO_PIN(68),
    AK37E_GPIO_PIN(69),
    AK37E_GPIO_PIN(70),
    AK37E_GPIO_PIN(71),
    AK37E_GPIO_PIN(72),
    AK37E_GPIO_PIN(73),
    AK37E_GPIO_PIN(74),
    AK37E_GPIO_PIN(75),
    AK37E_GPIO_PIN(76),
    AK37E_GPIO_PIN(77),
    AK37E_GPIO_PIN(78),
    AK37E_GPIO_PIN(79),
    AK37E_GPIO_PIN(80),
    AK37E_GPIO_PIN(81),
    AK37E_GPIO_PIN(82),
    AK37E_GPIO_PIN(83),
    AK37E_GPIO_PIN(84),
    AK37E_GPIO_PIN(85),
    AK37E_GPIO_PIN(86),
    AK37E_GPIO_PIN(87),
    AK37E_GPIO_PIN(88),
    AK37E_GPIO_PIN(89),
    AK37E_GPIO_PIN(90),
    AK37E_GPIO_PIN(91),
    AK37E_GPIO_PIN(92),
    AK37E_GPIO_PIN(93),
    AK37E_GPIO_PIN(94),
    AK37E_GPIO_PIN(95),
    AK37E_GPIO_PIN(96),
};

/* one pin per group */
static const char * const ak_gpio_groups[] = {
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
};

static const char * const ak_funcs[5] = {
    [0] = "default func",
    [1] = "func1",
    [2] = "func2",
    [3] = "func3",
    [4] = "func4",
};

#define CFG_DRV_EN              (0x01)
#define CFG_INPUT_EN            (0x02)
#define CFG_PUPD_EN             (0x04)
#define CFG_PUPD_SEL            (0x08)
#define CFG_SLEW_RATE           (0x10)

#define CFG_ALL_DISABLE         (0x0)
#define CFG_ALL_EN              (CFG_DRV_EN|CFG_INPUT_EN|CFG_PUPD_EN|\
                                 CFG_PUPD_SEL|CFG_SLEW_RATE)

//share pin func config in AK37Exx
static struct gpio_sharepin ak_sharepin[] = {
    /* PIN pad_drv_offset pad_ie_offset pupd_en_offset
    *pupd_sel_offset pad_sl_offset gpio_config */
    /* GPIO0, RMII0_MDIO, SD2_D1, I2S0_DIN, TWI0_SCL */
    {0, 0, 0, 0, 0, 0, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO1, RMII0_MDC, SD2_D0, I2S0_LRCLK, TWI0_SDA */
    {1, 2, 1, 1, 1, 1, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO2, RMII0_RXER, SD2_MCLK, I2S0_BCLK, PWM5 */
    {2, 4, 2, 2, 2, 2, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO3, RMII0_RXDV, SD2_CMD, I2S0_MCLK, PWM4 */
    {3, 6, 3, 3, 3, 3, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO4, RMII0_RXD0, SD2_D3, I2S0_OUT */
    {4, 8, 4, 4, 4, 4, 4, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO5, RMII0_RXD1, SD2_D2, PWM3 */
    {5, 10, 5, 5, 5, 5, 4, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO6, OPCLK2, TWI3_SCL, PWM2, I2S1_LRCLK */
    {6, 12, 6, 6, 6, 6, 5, CFG_ALL_EN},
    /* GPIO7, RMII0_TXD0, TWI3_SDA, PWM1, I2S1_BCLK */
    {7, 14, 7, 7, 7, 7, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO8, RMII0_TXD1, UART3_RXD, PDM_CLK, I2S1_MCLK */
    {8, 16, 8, 8, 8, 8, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO9, RMII0_TXEN, UART3_TXD, PDM_DATA, I2S1_DIN */
    {9, 18, 9, 9, 9, 9, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO10, SPI0_CS0 */
    {10, 20, 10, 10, 10, 10, 2, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO11, SPI0_HOLD */
    {11, 22, 11, 11, 11, 11, 2, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO12, SPI0_DIN */
    {12, 24, 12, 12, 12, 12, 2, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO13, SPI0_SCLK */
    {13, 26, 13, 13, 13, 13, 2, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO14, SPI0_WP */
    {14, 28, 14, 14, 14, 14, 2, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO15, SPI0_DOUT */
    {15, 30, 15, 15, 15, 15, 2, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO16, SPI1_CS0, SD1_D1, TWI1_SCL, PWM1 */
    {16, 0, 16, 16, 16, 16, 5, CFG_PUPD_EN},
    /* GPIO17, SPI1_DIN(IO1), SD1_D0, TWI1_SDA, PWM4 */
    {17, 2, 17, 17, 17, 17, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO18, SPI1_SCLK, SD1_MCLK */
    {18, 4, 18, 18, 18, 18, 3, CFG_PUPD_EN},
    /* GPIO19, SPI1_DOUT(IO0), SD1_CMD */
    {19, 6, 19, 19, 19, 19, 3, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO20, SPI1_WP(IO2), SD1_D3, UART2_RXD, PWM3 */
    {20, 8, 20, 20, 20, 20, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO21, SPI1_HOLD(IO3), SD1_D2, UART2_TXD, PWM2 */
    {21, 10, 21, 21, 21, 21, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO22, UART0_RXD, PWM4 */
    {22, 12, 22, 22, 22, 22, 3, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO23, UART0_TXD, PWM5 */
    {23, 14, 23, 23, 23, 23, 3, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO24, UART1_RXD, SPI2_CS0, PWM3, RGB_D20 */
    {24, 14, 24, 24, 24, 24, 5, CFG_ALL_EN},
    /* GPIO25, UART1_TXD, SPI2_SCLK, PWM2, RGB_D21 */
    {25, 14, 25, 25, 25, 25, 5,
    (CFG_DRV_EN|CFG_INPUT_EN|CFG_PUPD_EN|CFG_SLEW_RATE)},
    /* GPIO26, UART1_CTS, SPI2_DIN(IO1), PWM0, RGB_D22 */
    {26, 14, 26, 26, 26, 26, 5,
    (CFG_DRV_EN|CFG_INPUT_EN|CFG_PUPD_EN|CFG_SLEW_RATE)},
    /* GPIO27, UART1_RTS, SPI2_DOUT(IO0), PWM5, RGB_D23 */
    {27, 14, 27, 27, 27, 27, 5,
    (CFG_DRV_EN|CFG_INPUT_EN|CFG_PUPD_EN|CFG_SLEW_RATE)},
    /* GPIO28, UART2_RXD, PWM1, SPI2_CS1, RGB_D19 */
    {28, 14, 28, 28, 28, 28, 5,
    (CFG_DRV_EN|CFG_INPUT_EN|CFG_PUPD_EN|CFG_SLEW_RATE)},
    /* GPIO29, UART2_TXD, PWM3, SPI1_CS1, RGB_D18 */
    {29, 14, 29, 29, 29, 29, 5,
    (CFG_DRV_EN|CFG_INPUT_EN|CFG_PUPD_EN|CFG_SLEW_RATE)},
    /* GPIO30, TWI0_SCL */
    {30, 28, 30, 30, 30, 30, 2, CFG_ALL_EN},
    /* GPIO31, TWI0_SDA */
    {31, 30, 31, 31, 31, 31, 2, CFG_ALL_EN},
    /* GPIO32, TWI1_SCL */
    {32, 0, 0, 0, 0, 0, 2, CFG_ALL_EN},
    /* GPIO33, TWI1_SDA */
    {33, 2, 1, 1, 1, 1, 2, CFG_ALL_EN},
    /* GPIO34, RGB_VOGATE, MPU_RD# */
    {34, 4, 2, 2, 2, 2, 3, CFG_ALL_EN},
    /* GPIO35, RGB_VOVSYNC, MPU_A0 */
    {35, 6, 3, 3, 3, 3, 3, CFG_ALL_EN},
    /* GPIO36, RGB_VOHSYNC, MPU_CS# */
    {36, 8, 4, 4, 4, 4, 3, CFG_ALL_EN},
    /* GPIO37, RGB_VOPCLK, MPU_WR# */
    {37, 10, 5, 5, 5, 5, 3, CFG_ALL_EN},
    /* GPIO38, RGB_D0, MPU_D0 */
    {38, 12, 6, 6, 6, 6, 3, CFG_ALL_EN},
    /* GPIO39, RGB_D1, MPU_D1 */
    {39, 14, 7, 7, 7, 7, 3, CFG_ALL_EN},
    /* GPIO40, RGB_D2, MPU_D2 */
    {40, 16, 8, 8, 8, 8, 3, CFG_ALL_EN},
    /* GPIO41, RGB_D3, MPU_D3 */
    {41, 18, 9, 9, 9, 9, 3, CFG_ALL_EN},
    /* GPIO42, RGB_D4, MPU_D4 */
    {42, 20, 10, 10, 10, 10, 3, CFG_ALL_EN},
    /* GPIO43, RGB_D5, MPU_D5 */
    {43, 22, 11, 11, 11, 11, 4, CFG_ALL_EN},
    /* GPIO44, RGB_D6, MPU_D6 */
    {44, 24, 12, 12, 12, 12, 4, CFG_ALL_EN},
    /* GPIO45, RGB_D7, MPU_D7 */
    {45, 26, 13, 13, 13, 13, 4, CFG_ALL_EN},
    /* GPIO46, RGB_D8, MPU_D8, TWI2_SCL */
    {46, 28, 14, 14, 14, 14, 4, CFG_ALL_EN},
    /* GPIO47, RGB_D9, MPU_D9, TWI2_SDA */
    {47, 30, 15, 15, 15, 15, 4, CFG_ALL_EN},
    /* GPIO48, RGB_D10, MPU_D10, JTAG_RSTN */
    {48, 0, 16, 16, 16, 16, 4, CFG_ALL_EN},
    /* GPIO49, RGB_D11, MPU_D11, JTAG_TDI */
    {49, 2, 17, 17, 17, 17, 4, CFG_ALL_EN},
    /* GPIO50, RGB_D12, MPU_D12, JTAG_TMS, I2S1_LRCLK */
    {50, 4, 18, 18, 18, 18, 5, CFG_ALL_EN},
    /* GPIO51, RGB_D13, MPU_D13, JTAG_TCLK, I2S1_BCLK */
    {51, 6, 19, 19, 19, 19, 5, CFG_ALL_EN},
    /* GPIO52, RGB_D14, MPU_D14, JTAG_RTCK, I2S1_MCLK */
    {52, 8, 20, 20, 20, 20, 5, CFG_ALL_EN},
    /* GPIO53, RGB_D15, MPU_D15, JTAG_TDO, I2S1_DIN */
    {53, 10, 21, 21, 21, 21, 5, CFG_ALL_EN},
    /* GPIO54, RGB_D16, MPU_D16, PDM_CLK, TWI3_SCL */
    {54, 12, 22, 22, 22, 22, 5, CFG_ALL_EN},
    /* GPIO55, RGB_D17, MPU_D17, PDM_DATA, TWI3_SDA */
    {55, 12, 23, 23, 23, 23, 5, CFG_ALL_EN},
    /* GPIO56, CSI0_SCLK, UART3_RXD */
    {56, 16, 24, 24, 24, 24, 3, CFG_ALL_EN},
    /* GPIO57, CSI0_PCLK, UART3_TXD */
    {57, 18, 25, 25, 25, 25, 3, CFG_ALL_EN},
    /* GPIO58, CSI0_HSYNC, RMII1_MDIO, SD2_D1 */
    {58, 20, 26, 26, 26, 26, 4, CFG_ALL_EN},
    /* GPIO59, CSI0_VSYNC, RMII1_MDC, SD2_D0 */
    {59, 22, 27, 27, 27, 27, 4, CFG_ALL_EN},
    /* GPIO60, CSI0_D0, RMII1_RXER, SD2_MCLK */
    {60, 24, 28, 28, 28, 28, 4, CFG_ALL_EN},
    /* GPIO61, CSI0_D1, RMII1_RXDV, SD2_CMD */
    {61, 26, 29, 29, 29, 29, 4, CFG_ALL_EN},
    /* GPIO62, CSI0_D2, RMII1_RXD0, SD2_D3 */
    {62, 28, 30, 30, 30, 30, 4, CFG_ALL_EN},
    /* GPIO63, CSI0_D3, RMII1_RXD1, SD2_D2 */
    {63, 30, 31, 31, 31, 31, 4, CFG_ALL_EN},
    /* GPIO64, CSI0_D4, OPCLK1, TWI2_SCL, I2S1_LRCLK */
    {64, 0, 0, 0, 0, 0, 5, CFG_ALL_EN},
    /* GPIO65, CSI0_D5, RMII1_RXD0, TWI2_SDA, I2S1_BCLK */
    {65, 2, 1, 1, 1, 1, 5, CFG_ALL_EN},
    /* GPIO66, CSI0_D6, RMII1_TXD1, PDM_CLK, I2S1_MCLK */
    {66, 4, 2, 2, 2, 2, 5, CFG_ALL_EN},
    /* GPIO67, CSI0_D7, RMII1_TXEN, PDM_DATA, I2S1_DIN */
    {67, 6, 3, 3, 3, 3, 5, CFG_ALL_EN},
    /* GPIO68, CSI1_SCLK, SD0_D2, SPI2_CS0 */
    {68, 8, 4, 4, 4, 4, 4, CFG_ALL_EN},
    /* GPIO69, CSI1_PCLK, SD0_D3, SPI2_HOLD(IO3) */
    {69, 10, 5, 5, 5, 5, 4, CFG_ALL_EN},
    /* GPIO70, CSI1_HSYNC, SD0_D4, SPI2_DIN(IO1), UART1_RXD */
    {70, 12, 6, 6, 6, 6, 5, CFG_ALL_EN},
    /* GPIO71, CSI1_VSYNC, SD0_CMD, SPI2_SCLK, UART1_TXD */
    {71, 14, 7, 7, 7, 7, 5, CFG_ALL_EN},
    /* GPIO72, CSI1_D0, SD0_D5, SPI2_WP(IO2), UART1_CTS */
    {72, 16, 8, 8, 8, 8, 5, CFG_ALL_EN},
    /* GPIO73, CSI1_D1, SD0_CLK, SPI2_DOUT(IO0), UART1_RTS */
    {73, 18, 9, 9, 9, 9, 5, CFG_ALL_EN},
    /* GPIO74, CSI1_D2, SD0_D6, RGB_D23, SPI2_CS1 */
    {74, 20, 10, 10, 10, 10, 5, CFG_ALL_EN},
    /* GPIO75, CSI1_D3, SD0_D7, RGB_D22, I2S0_DIN */
    {75, 22, 11, 11, 11, 11, 5, CFG_ALL_EN},
    /* GPIO76, CSI1_D4, SD0_D1, RGB_D21, I2S0_LRCLK */
    {76, 24, 12, 12, 12, 12, 5, CFG_ALL_EN},
    /* GPIO77, CSI1_D5, SD0_D0, RGB_D20, I2S0_BCLK */
    {77, 26, 13, 13, 13, 13, 5, CFG_ALL_EN},
    /* GPIO78, CSI1_D6, TWI2_SCL, RGB_D19, I2S0_MCLK */
    {78, 28, 14, 14, 14, 14, 5, CFG_ALL_EN},
    /* GPIO79, CSI1_D7, TWI2_SDA, RGB_D18, I2S0_DOUT */
    {79, 30, 15, 15, 15, 15, 5, CFG_ALL_EN},
    /* Not support */
    {80, 0, 16, 16, 16, 16, AK_GPIO_CONF_INVALID, CFG_ALL_DISABLE},
    /* Not support */
    {81, 2, 17, 17, 17, 17, AK_GPIO_CONF_INVALID, CFG_ALL_DISABLE},
    /* GPIO82, PWM0, SPI0_CS1, SPI1_CS1 */
    {82, 4, 18, 18, 18, 18, 4, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO83, TWI2_SCL, SPI2_CS0, UART2_RXD, I2S1_LRCLK */
    {83, 6, 19, 19, 19, 19, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO84, TWI2_SDA, SPI2_SCLK, UART2_TXD, I2S1_BCLK */
    {84, 8, 20, 20, 20, 20, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO85, TWI3_SCL, SPI2_DIN(IO1), UART3_RXD, I2S1_MCLK */
    {85, 10, 21, 21, 21, 21, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* GPIO86, TWI3_SDA, SPI2_DOUT(IO0), UART3_TXD, I2S1_DIN */
    {86, 12, 22, 22, 22, 22, 5, (CFG_INPUT_EN|CFG_PUPD_EN)},
    /* Not support */
    {87, 14, 23, 23, 23, 23, AK_GPIO_CONF_INVALID, CFG_ALL_DISABLE},
    /* Not support */
    {88, 16, 24, 24, 24, 24, AK_GPIO_CONF_INVALID, CFG_ALL_DISABLE},
    /* Not support */
    {89, 18, 25, 25, 25, 25, AK_GPIO_CONF_INVALID, CFG_ALL_DISABLE},
    /* Not support */
    {90, 20, 26, 26, 26, 26, AK_GPIO_CONF_INVALID, CFG_ALL_DISABLE},
    /* Not support */
    {91, 22, 27, 27, 27, 27, AK_GPIO_CONF_INVALID, CFG_ALL_DISABLE},
    /* Not support */
    {92, 24, 28, 28, 28, 28, AK_GPIO_CONF_INVALID, CFG_ALL_DISABLE},
    /* Not support */
    {93, 26, 29, 29, 29, 29, AK_GPIO_CONF_INVALID, CFG_ALL_DISABLE},
    /* Not support */
    {94, 28, 30, 30, 30, 30, AK_GPIO_CONF_INVALID, CFG_ALL_DISABLE},
    /* MIC_P, GPI0 */
    {95, 0, 0, 0, 0, 0, 2, CFG_INPUT_EN},
    /* MIC_N, GPI1 */
    {96, 0, 0, 0, 0, 0, 2, CFG_INPUT_EN},
};

static const char * const irq_type_names[] = {
    [IRQ_TYPE_NONE] = "none",
    [IRQ_TYPE_EDGE_RISING] = "edge-rising",
    [IRQ_TYPE_EDGE_FALLING] = "edge-falling",
    [IRQ_TYPE_LEVEL_HIGH] = "level-high",
    [IRQ_TYPE_LEVEL_LOW] = "level-low",
};

/**
 * ak_request_edge_trigger_irq:
 *    allocate an interrupt line base on current gpio status
 *  @gpio: the irq related gpio
 *  @irq: Interrupt line to allocate
 *  @handler: Function to be called when the IRQ occurs.
 *      Primary handler for threaded interrupts
 *      If NULL and thread_fn != NULL the default
 *      primary handler is installed
 *  @name: An ascii name for the claiming device
 *  @dev: A cookie passed back to the handler function
 *
 * return 0 if successfully. others will be error.
 */
int ak_request_edge_trigger_irq(int gpio, unsigned int irq,
    irq_handler_t handler, const char *name, void *dev)
{
    u32 val, direction;
    void __iomem * reg_direction = AK_GPIO_BASE + AK_GPIO_DIR_BASE(gpio);
    void __iomem * reg_data = AK_GPIO_BASE + AK_GPIO_IN_BASE(gpio);
    unsigned int bit = AK_GPIO_REG_SHIFT(gpio);
    int ret = 0;

    if ((gpio < XGPIO_000) || (gpio > XGPIO_086))
        return -EINVAL;

    /*
    * If not input, return err.
    * direction = 0 means output mode
    * direction = 1 means input mode
    */
    direction = ((__raw_readl(reg_direction) & (1 << bit)) ? 0 : 1);
    if (direction == 0)
        return -EINVAL;

    val = (__raw_readl(reg_data) & (1 << bit)) ? 1 : 0;
    if (val) {
        ret = request_irq(irq, handler,
            IRQF_TRIGGER_FALLING|IRQF_ONESHOT, name, dev);
    } else {
        ret = request_irq(irq, handler,
            IRQF_TRIGGER_RISING|IRQF_ONESHOT, name, dev);
    }

    return 0;
}
EXPORT_SYMBOL(ak_request_edge_trigger_irq);

/**
 * ak_exchange_irq_edge_trigger_unlocked:
 *    exchange the trigger config base on the current gpio status
 *    if the gpio is high, then config it to falling edge trigger
 *    if the gpio is low, then config it to rising edge trigger
 *    please NOTE this function do to disable the cpu's irq.
 *    It is better to disable the cpu's irq before call this function
 *  @gpio: the irq related gpio
 *
 * return 0 if successfully. others will be error.
 */
int ak_exch_irq_edge_trig_unlocked(int gpio)
{
    u32 val, reg_val;
    void __iomem * reg_data = AK_GPIO_BASE + AK_GPIO_IN_BASE(gpio);
    void __iomem * reg_polarity = AK_GPIO_BASE + AK_GPIO_INTPOL_BASE(gpio);
    unsigned int bit = AK_GPIO_REG_SHIFT(gpio);

    if ((gpio < XGPIO_000) || (gpio > XGPIO_086))
        return -EINVAL;

    val = (__raw_readl(reg_data) & (1 << bit)) ? 1 : 0;

    reg_val = __raw_readl(reg_polarity);
    if (val) {
        reg_val |= (1 << bit);
    } else {
        reg_val &= ~(1 << bit);
    }
    __raw_writel(reg_val, reg_polarity);

    return 0;

}
EXPORT_SYMBOL(ak_exch_irq_edge_trig_unlocked);

static inline int pin_gpi_groups(int pin)
{
    return ((pin == 95) || (pin == 96)) ? 1 : 0;
}

static int pin_set_share(struct ak_pinctrl *pc, int pin, int sel)
{
    u32 regval;
    int offset = AK_SHAREPIN_CON_REG_OFFSET(pin);

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID) {
        printk(KERN_ERR"%s pin_%d not support\n", __func__, pin);
        return -EINVAL;
    }

    if (sel >= ak_sharepin[pin].func_mux_max) {
        printk(KERN_ERR"%s pin_%d max %d but sel %d\n", 
              __func__, pin, ak_sharepin[pin].func_mux_max, sel);
        return -EINVAL;
    }

    /*
    * Specail Case: only 2 GPI pins
    * AK_ANALOG_CTRL_REG3
    * [25]:GPI0
    * [26]:GPI1
    */
    if (pin_gpi_groups(pin)) {
        regval = __raw_readl(AK_ANALOG_CTRL_REG3);
        regval &= ~(0x1 << (pin - 95 + 25));
        regval |= ((sel & 0x1) << (pin - 95 + 25));
        __raw_writel(regval, AK_ANALOG_CTRL_REG3);
        ak_log_print("PIN_%d: FUNC(%d) 0x%p:0x%x\n",
                    pin, sel, AK_ANALOG_CTRL_REG3,
        __raw_readl(AK_ANALOG_CTRL_REG3));
    } else {
        regval = __raw_readl(AK_SHAREPIN_CON_REG(pin));
        regval &= ~(0x7 << offset);
        regval |= ((sel & 0x7) << offset);
        __raw_writel(regval, AK_SHAREPIN_CON_REG(pin));
        ak_log_print("PIN_%d: FUNC(%d) 0x%p:0x%x\n",
                    pin, sel, AK_SHAREPIN_CON_REG(pin),
        __raw_readl(AK_SHAREPIN_CON_REG(pin)));
    }

    pc->fsel[pin] = sel;
    return 0;
}

/**
*@brief: set pin input enable
*@param[in] int pin
*@param[in] int enable
*@return: int
**/
static int pin_set_input_enable(int pin, int enable)
{
    u32 regval;

    if (pin_gpi_groups(pin)) {
        regval = __raw_readl(AK_ANALOG_CTRL_REG3);
        if (enable)
            regval |= (0x1 << (pin - 95 + 25));
        else
            regval &= ~(0x1 << (pin - 95 + 25));
        __raw_writel(regval, AK_ANALOG_CTRL_REG3);

        return 0;
    }

    if (!(ak_sharepin[pin].gpio_cfg & CFG_INPUT_EN)) {
        return -EPERM;
    }

    regval = __raw_readl(AK_GPIO_IE_REG(pin));
    if (enable) {
        regval |= (0x1 << ak_sharepin[pin].pad_ie_offset);
    } else {
        regval &= ~(0x1 << ak_sharepin[pin].pad_ie_offset);
    }

    __raw_writel(regval, AK_GPIO_IE_REG(pin));
    ak_log_print("PIN_%d: Input_EN(%d) 0x%p:0x%x\n",
                pin, enable, AK_GPIO_IE_REG(pin),
            __raw_readl(AK_GPIO_IE_REG(pin)));

    return 0;
}

/**
*@brief: get pin input enable
*@param[in] int pin
*@return: int
**/
static int pin_get_input_enable(int pin)
{
    u32 regval;

    if (!(ak_sharepin[pin].gpio_cfg & CFG_INPUT_EN)) {
        return -EPERM;
    }

    if (pin_gpi_groups(pin)) {
        regval = __raw_readl(AK_ANALOG_CTRL_REG3);
        regval &= (0x1 << (pin - 95 + 25));
        return (regval ? 1 : 0);
    }

    regval = __raw_readl(AK_GPIO_IE_REG(pin));

    ak_log_print("%s 0x%p:0x%x\n",
                __func__, AK_GPIO_IE_REG(pin), regval);

    regval &= (0x1 << ak_sharepin[pin].pad_ie_offset);

    return (regval ? 1:0);
}

/**
*@brief: set pin drive
*@param[in] int pin
*@param[in] int strength
*@return: int
**/
static int pin_set_drive(int pin, int strength)
{
    u32 regval;
    void __iomem * pad_drv_reg = AK_GPIO_DRIVE_CON_REG(pin);

    if (!(ak_sharepin[pin].gpio_cfg & CFG_DRV_EN)) {
        return -EPERM;
    }

    if (strength > 3)
        return -EINVAL;

    /* !! Specail case !!*/
    if ((pin >= XGPIO_024) && (pin <= XGPIO_029)) {
        pad_drv_reg = AK_GPIO_DRIVE_CON3;
    }

    regval = __raw_readl(pad_drv_reg);
    regval &= ~(0x3 << ak_sharepin[pin].pad_drv_offset);
    regval |= ((strength & 0x3) << ak_sharepin[pin].pad_drv_offset);

    __raw_writel(regval, pad_drv_reg);

    ak_log_print("PIN_%d: DRV(%d) 0x%p:0x%x\n",
                pin, ak_sharepin[pin].pad_drv_offset,
            pad_drv_reg, __raw_readl(pad_drv_reg));

    return 0;
}

/**
*@brief: get pin drive
*@param[in] int pin
*@return: int
**/
static int pin_get_drive(int pin)
{
    u32 regval;
    void __iomem * pad_drv_reg = AK_GPIO_DRIVE_CON_REG(pin);

    if (!(ak_sharepin[pin].gpio_cfg & CFG_DRV_EN)) {
        return -EPERM;
    }

    if (pin_gpi_groups(pin)) {
        return -EPERM;
    }

    /* !! Specail case !!*/
    if ((pin >= XGPIO_024) && (pin <= XGPIO_029)) {
        pad_drv_reg = AK_GPIO_DRIVE_CON3;
    }

    regval = __raw_readl(pad_drv_reg);

    ak_log_print("%s 0x%p:0x%x\n",
                __func__, pad_drv_reg, regval);

    regval &= (0x3 << ak_sharepin[pin].pad_drv_offset);
    regval = regval >> ak_sharepin[pin].pad_drv_offset;

    return DRIVE_STRENGTH(regval);
}

/**
*@brief: set pin pull polarity
*@param[in] struct ak_pinctrl *pc
*@param[in] int pin
*@param[in] int pullup
*@return: int
**/
static int pin_set_pull_polarity(struct ak_pinctrl *pc, int pin, int pullup)
{
    u32 regval;

    if (!(ak_sharepin[pin].gpio_cfg & CFG_PUPD_SEL)) {
        return -EPERM;
    }

    regval = __raw_readl(AK_PPU_PPD_SEL_REG(pin));
    if (pullup) {
        regval &= (~(0x1 << ak_sharepin[pin].pupd_sel_offset));
    } else {
        regval |= (0x1 << ak_sharepin[pin].pupd_sel_offset);
    }

    __raw_writel(regval, AK_PPU_PPD_SEL_REG(pin));

    ak_log_print("PIN_%d: PULL_pol(%d) 0x%p:0x%x\n",
                pin, pullup,
                AK_PPU_PPD_SEL_REG(pin), __raw_readl(AK_PPU_PPD_SEL_REG(pin)));

    return 0;
}

/**
*@brief: set pin pull polarity
*@param[in] struct ak_pinctrl *pc
*@param[in] int pin
*@return: int
**/
static int pin_get_pull_polarity(struct ak_pinctrl *pc, int pin)
{
    u32 regval;

    if (!(ak_sharepin[pin].gpio_cfg & CFG_PUPD_SEL)) {
        return -EPERM;
    }

    if (pin_gpi_groups(pin)) {
        return -EPERM;
    }

    regval = __raw_readl(AK_PPU_PPD_SEL_REG(pin));

    ak_log_print("%s 0x%p:0x%x\n",
                __func__, AK_PPU_PPD_SEL_REG(pin), regval);

    regval &= (0x1 << ak_sharepin[pin].pupd_sel_offset);

    return (regval ? 0:1);
}

/**
*@brief: set pin pull enable
*@param[in] struct ak_pinctrl *pc
*@param[in] int pin
*@param[in] int enable
*@return: int
**/
static int pin_set_pull_enable(struct ak_pinctrl *pc, int pin, int enable)
{
    u32 regval;

    if (!(ak_sharepin[pin].gpio_cfg & CFG_PUPD_EN)) {
        return -EPERM;
    }

    regval = __raw_readl(AK_PPU_PPD_EN_REG(pin));
    if (enable) {
        regval |= (0x1 << ak_sharepin[pin].pupd_en_offset);
    } else {
        regval &= (~(0x1 << ak_sharepin[pin].pupd_en_offset));
    }

    __raw_writel(regval, AK_PPU_PPD_EN_REG(pin));

    ak_log_print("PIN_%d: PULL_EN(%d) 0x%p:0x%x\n", pin, enable,
        AK_PPU_PPD_EN_REG(pin), __raw_readl(AK_PPU_PPD_EN_REG(pin)));

    return 0;
}

static int pin_get_pull_enable(struct ak_pinctrl *pc, int pin)
{
    u32 regval;

    if (!(ak_sharepin[pin].gpio_cfg & CFG_PUPD_EN)) {
        return -EPERM;
    }

    if (pin_gpi_groups(pin)) {
        return -EPERM;
    }

    regval = __raw_readl(AK_PPU_PPD_EN_REG(pin));

    ak_log_print("%s 0x%p:0x%x\n",
                __func__, AK_PPU_PPD_EN_REG(pin), regval);

    regval &= (0x1 << ak_sharepin[pin].pupd_en_offset);

    return (regval ? 1:0);
}

static int pin_set_slew_rate(int pin, int fast)
{
    u32 regval;

    if (!(ak_sharepin[pin].gpio_cfg & CFG_SLEW_RATE)) {
        return -EPERM;
    }

    regval = __raw_readl(AK_GPIO_SLEW_RATE_REG(pin));
    if (fast) {
        regval |= (0x1 << ak_sharepin[pin].pad_sl_offset);
    } else {
        regval &= (~(0x1 << ak_sharepin[pin].pad_sl_offset));
    }
    __raw_writel(regval, AK_GPIO_SLEW_RATE_REG(pin));

    ak_log_print("PIN_%d: SR(%d) 0x%p:0x%x\n", pin, fast,
            AK_GPIO_SLEW_RATE_REG(pin),
            __raw_readl(AK_GPIO_SLEW_RATE_REG(pin)));

    return 0;
}

static int pin_get_slew_rate(int pin)
{
    u32 regval;

    if (!(ak_sharepin[pin].gpio_cfg & CFG_SLEW_RATE)) {
        return -EPERM;
    }

    if (pin_gpi_groups(pin)) {
        return -EPERM;
    }

    regval = __raw_readl(AK_GPIO_SLEW_RATE_REG(pin));

    ak_log_print("%s 0x%p:0x%x\n",
                __func__, AK_GPIO_SLEW_RATE_REG(pin), regval);

    regval &= (0x1 << ak_sharepin[pin].pad_sl_offset);

    return (regval ? 1 : 0);
}

static int gpio_set_share(struct ak_pinctrl *pc, unsigned offset)
{
    unsigned long flags;

    local_irq_save(flags);
    pin_set_share(pc, offset, 0); /*func0 for gpio */
    local_irq_restore(flags);

    return 0;
}

static inline unsigned int ak_pinctrl_fsel_get(
        struct ak_pinctrl *pc, unsigned pin)
{
    int status = pc->fsel[pin];

    return status;
}

static inline void ak_pinctrl_fsel_set(
        struct ak_pinctrl *pc, unsigned pin,
        unsigned sel)
{
    unsigned long flags;

    local_irq_save(flags);
    pin_set_share(pc, pin, sel);
    local_irq_restore(flags);
}

static int ak_gpio_set_drive(struct gpio_chip *chip,
            unsigned offset, int strength)
{
    return pin_set_drive(offset, strength);
}

static int ak_gpio_get_drive(struct gpio_chip *chip, unsigned offset)
{
    return pin_get_drive(offset);
}

/*
* gpiolib-sysfs.c
* -- pull_polarity_store
* ---- gpiod_set_pull_polarity
* IF pullup = 1 means pull up.
* else pullup = 0 mean pulldown
* or negative if any error
*/
static int ak_gpio_set_pull_polarity(struct gpio_chip *chip,
            unsigned offset, int pullup)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);

    return pin_set_pull_polarity(pc, offset, pullup);
}

/*
* gpiolib-sysfs.c
* -- pull_polarity_show
* ---- gpiod_get_pull_polarity
* IF return > 0 means pullup
* else return 0 mean pulldown
* or negative if any error
*/
static int ak_gpio_get_pull_polarity(struct gpio_chip *chip,
            unsigned offset)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);

    return pin_get_pull_polarity(pc, offset);
}

/*
* gpiolib-sysfs.c
* -- pull_enable_store
* ---- gpiod_set_pull_enable
*/
static int ak_gpio_set_pull_enable(struct gpio_chip *chip,
            unsigned offset, int enable)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);

    return pin_set_pull_enable(pc, offset, enable);
}

/*
* gpiolib-sysfs.c
* -- pull_enable_show
* ---- gpiod_get_pull_enable
*/
static int ak_gpio_get_pull_enable(struct gpio_chip *chip,
    unsigned offset)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);

    return pin_get_pull_enable(pc, offset);
}

/*
* gpiolib-sysfs.c
* -- input_enable_store
* ---- gpiod_set_input_enable
*/
static int ak_gpio_set_input_enable(struct gpio_chip *chip,
            unsigned offset, int enable)
{
    return pin_set_input_enable(offset, enable);
}

/*
* gpiolib-sysfs.c
* -- input_enable_show
* ---- gpiod_get_input_enable
*/
static int ak_gpio_get_input_enable(struct gpio_chip *chip,
    unsigned offset)
{
    return pin_get_input_enable(offset);
}

/*
* gpiolib-sysfs.c
* -- slew_rate_store
* ---- gpiod_set_slew_rate
*/
static int ak_gpio_set_slew_rate(struct gpio_chip *chip,
            unsigned offset, int fast)
{
    return pin_set_slew_rate(offset, fast);
}

/*
* gpiolib-sysfs.c
* -- slew_rate_show
* ---- gpiod_get_slew_rate
*/
static int ak_gpio_get_slew_rate(struct gpio_chip *chip, unsigned offset)
{
    return pin_get_slew_rate(offset);
}

static inline void ak_pinctrl_fsel_reset(
        struct ak_pinctrl *pc, unsigned pin,
        unsigned fsel)
{
    unsigned long flags;
    u32 regval;
    int cur = pc->fsel[pin];
    int offset = AK_SHAREPIN_CON_REG_OFFSET(pin);

    if (cur == -1)
        return;

    if (ak_sharepin[pin].func_mux_max == AK_GPIO_CONF_INVALID)
        return;

    local_irq_save(flags);
    if (pin_gpi_groups(pin)) {
        regval = __raw_readl(AK_ANALOG_CTRL_REG3);
        regval &= ~(0x1 << (pin - 95 + 25));
        __raw_writel(regval, AK_ANALOG_CTRL_REG3);
    } else {
        regval = __raw_readl(AK_SHAREPIN_CON_REG(pin));
        regval &= ~(0x7 << offset);
        __raw_writel(regval, AK_SHAREPIN_CON_REG(pin));
    }
    local_irq_restore(flags);

    if (pin_gpi_groups(pin)) {
        ak_log_print("PIN_%d fsel reset 0x%p:0x%x\n",
            pin, AK_ANALOG_CTRL_REG3,
            __raw_readl(AK_ANALOG_CTRL_REG3));
    } else {
        ak_log_print("PIN_%d fsel reset 0x%p:0x%x\n",
            pin, AK_SHAREPIN_CON_REG(pin),
            __raw_readl(AK_SHAREPIN_CON_REG(pin)));
    }

    pc->fsel[pin] = -1;
}

static int ak_gpio_direction_input(struct gpio_chip *chip,
    unsigned offset)
{
    return pinctrl_gpio_direction_input(chip->base + offset);
}

/*
* if return 0 means output
* else any positive value means input
*/
static int ak_gpio_get_direction(struct gpio_chip *chip,
    unsigned offset)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);
    void __iomem * reg = pc->gpio_base + AK_GPIO_DIR_BASE(offset);
    unsigned int bit = AK_GPIO_REG_SHIFT(offset);

    if (pin_gpi_groups(offset))
        return 1;

    return (__raw_readl(reg) & (1 << bit)) ? 0:1;
}

static int ak_gpio_get(struct gpio_chip *chip, unsigned offset)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);
    void __iomem * reg = pc->gpio_base + AK_GPIO_IN_BASE(offset);
    void __iomem * reg_direction = pc->gpio_base + AK_GPIO_DIR_BASE(offset);
    unsigned int bit = AK_GPIO_REG_SHIFT(offset);
    unsigned int direction = 1;

    if (pin_gpi_groups(offset)) {
        reg = pc->gpio_base + AK_GPIO_INPUT3;
        bit = AK_GPI_REG_SHIFT(offset);
    } else {
        direction = ((__raw_readl(reg_direction) & (1 << bit)) ? 0:1);
        if (direction) {
            reg = pc->gpio_base + AK_GPIO_IN_BASE(offset);
        } else {
            reg = pc->gpio_base + AK_GPIO_OUT_BASE(offset);
        }
    }

    ak_log_print("GPIO_%d(%s): VAL(%d) 0x%p:0x%x\n",
        offset, direction ? "IN" : "OUT",
        ((__raw_readl(reg) & (1 << bit)) ? 1:0), reg, __raw_readl(reg));

    return (__raw_readl(reg) & (1 << bit)) ? 1:0;
}

static void ak_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
    struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);
    void __iomem *reg = pc->gpio_base + AK_GPIO_OUT_BASE(offset);
    unsigned int bit = AK_GPIO_REG_SHIFT(offset);
    unsigned long flags;
    u32 regval;

    if (pin_gpi_groups(offset))
        return;

    local_irq_save(flags);
    if (AK_GPIO_OUT_LOW == value) {
        regval = __raw_readl(reg);
        regval &= ~(1 << bit);
        __raw_writel(regval, reg);
    } else if (AK_GPIO_OUT_HIGH == value) {
        regval = __raw_readl(reg);
        regval |= (1 << bit);
        __raw_writel(regval, reg);
    }
    local_irq_restore(flags);

    ak_log_print("PIN_%d: OUT_VAL(%d) 0x%p:0x%x\n",
        offset, value, reg,
        __raw_readl(reg));
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
    .ngpio = AK37E_NUM_GPIOS,
    .can_sleep = false,
};

static void ak_pinctrl_irq_handler(struct irq_desc *desc)
{
    struct irq_chip *chip = irq_desc_get_chip(desc);
    struct ak_pinctrl *pc = irq_desc_get_handler_data(desc);
    unsigned long irq_status;
    int i, pin_irq;

    /*
    * AK37E max GPIO count is 97, gpio[0:96]
    * GPIO[00:94]: Normal GPIO
    * GPIO[95:96]: GPI[0:1]
    */
    for (i = 0; i < 4; i++) {
        int off = 0;
        chained_irq_enter(chip, desc);
        irq_status = __raw_readl(pc->gpio_base + AK_GPIO_EDGE_STATUS1 + i * 4);
        for_each_set_bit(off, &irq_status, 32) {
            /* check all int status and handle the Edge/level interrupt */
            if (i <= 2)
                pin_irq = irq_find_mapping(pc->irq_domain, i * 32 + off);
            else
                pin_irq = irq_find_mapping(pc->irq_domain, 95 + (off - 11));
            generic_handle_irq(pin_irq);
        }
        chained_irq_exit(chip, desc);
    }
}

static void ak_gpio_irq_enable(struct irq_data *data)
{
    struct ak_pinctrl *pc = irq_data_get_irq_chip_data(data);
    unsigned pin = irqd_to_hwirq(data);
    void __iomem *reg = pc->gpio_base + AK_GPIO_INTEN_BASE(pin);
    unsigned bit = AK_GPIO_REG_SHIFT(pin);
    unsigned long flags;
    u32 regval;

    if (pin_gpi_groups(pin)) {
        reg = pc->gpio_base + AK_GPIO_INT_MASK3;
        bit = AK_GPI_REG_SHIFT(pin);
    }

    spin_lock_irqsave(&pc->lock, flags);
    regval = __raw_readl(reg);
    regval |= (1 << bit);
    __raw_writel(regval, reg);
    spin_unlock_irqrestore(&pc->lock, flags);
}

static void ak_gpio_irq_disable(struct irq_data *data)
{
    struct ak_pinctrl *pc = irq_data_get_irq_chip_data(data);
    unsigned pin = irqd_to_hwirq(data);
    void __iomem *reg = pc->gpio_base + AK_GPIO_INTEN_BASE(pin);
    unsigned bit = AK_GPIO_REG_SHIFT(pin);
    unsigned long flags;
    u32 regval;

    if (pin_gpi_groups(pin)) {
        reg = pc->gpio_base + AK_GPIO_INT_MASK3;
        bit = AK_GPI_REG_SHIFT(pin);
    }

    spin_lock_irqsave(&pc->lock, flags);
    regval = __raw_readl(reg);
    regval &= ~(1 << bit);
    __raw_writel(regval, reg);
    spin_unlock_irqrestore(&pc->lock, flags);
}

/* slower path for reconfiguring IRQ type */
static int __ak_gpio_irq_set_type_enabled(struct ak_pinctrl *pc,
    unsigned offset, unsigned int type)
{
    u32 regval;
    void __iomem *reg_inten = pc->gpio_base + AK_GPIO_INTEN_BASE(offset);
    void __iomem *reg_intm  = pc->gpio_base + AK_GPIO_INTM_BASE(offset);
    void __iomem *reg_intp  = pc->gpio_base + AK_GPIO_INTPOL_BASE(offset);
    int bit = AK_GPIO_REG_SHIFT(offset);
    /*
    * reg_inten/AK_GPIO_INTEN_BASE
    * GPIO Interrupt Enable Register.
    * write 1 to enable the specific gpio's interrupt
    * write 0 to disable the specific gpio's interrupt
    *
    * reg_intm/AK_GPIO_INTM_BASE
    * GPIO Interrupt Mode Select Register.Interrupt trigger mode
    * write 1 to make edge-trigger
    * write 0 to make level-trigger
    *
    * reg_intp/AK_GPIO_INTPOL_BASE
    * GPIO Interrupt Polarity Select Register.Interrupt "input" polarity
    * Edge-trigger/reg_intm to 0 with specific bit.
    * write 0 to rising edge
    * write 1 to falling edge
    *
    * Level-trigger/reg_intm to 0 with specific bit.
    * write 0 to active high
    * write 1 to active low
    */

    if (pin_gpi_groups(offset)) {
        bit = AK_GPI_REG_SHIFT(offset);
        reg_inten = pc->gpio_base + AK_GPIO_INT_MASK3;
        reg_intm = pc->gpio_base + AK_GPIO_INT_MODE3;
        reg_intp = pc->gpio_base + AK_GPIO_INTP3;
    }

    switch (type) {
        case IRQ_TYPE_NONE:
            if (pc->irq_type[offset] != type) {
                regval = __raw_readl(reg_inten);
                regval &= ~(1 << bit);
                __raw_writel(regval, reg_inten);
                pc->irq_type[offset] = type;
            }
            break;
        case IRQ_TYPE_EDGE_RISING:
        case IRQ_TYPE_EDGE_FALLING:
            if (pc->irq_type[offset] != type) {
                /* Make the edge-trigger */
                regval = __raw_readl(reg_intm);
                regval |= (1 << bit);
                __raw_writel(regval, reg_intm);

                /* Config the trigger way*/
                regval = __raw_readl(reg_intp);
                if (IRQ_TYPE_EDGE_RISING == type)
                    regval &= ~(1 << bit);
                else
                    regval |= (1 << bit);
                __raw_writel(regval, reg_intp);
                pc->irq_type[offset] = type;
            }
            break;

        case IRQ_TYPE_LEVEL_HIGH:
        case IRQ_TYPE_LEVEL_LOW:
            if (pc->irq_type[offset] != type) {
                /* Make the level-trigger */
                regval = __raw_readl(reg_intm);
                regval &= ~(1 << bit);
                __raw_writel(regval, reg_intm);

                /* Config the trigger way*/
                regval = __raw_readl(reg_intp);
                if (IRQ_TYPE_LEVEL_HIGH == type)
                    regval &= ~(1 << bit);
                else
                    regval |= (1 << bit);
                __raw_writel(regval, reg_intp);
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

    /*
    * Clear the IRQ status
    * GPI[0:1] need to be special handled
    */
    if (pin_gpi_groups(pin))
        __raw_readl(pc->gpio_base + AK_GPIO_EDGE_STATUS3);
    else
        __raw_readl(pc->gpio_base + AK_GPIO_INTEDGE_BASE(pin));
    //__raw_writel(0 << bit,pc->gpio_base + AK_GPIO_INTEDGE_BASE(pin));

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
}/* end of func */

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
    void __iomem *reg = pc->gpio_base + AK_GPIO_DIR_BASE(offset);
    unsigned int bit = AK_GPIO_REG_SHIFT(offset);
    int gpio_cfg = input ?
        ePIN_AS_GPIO_IN : ePIN_AS_GPIO_OUT;

    if (pin_gpi_groups(offset))
        return -EPERM;

    local_irq_save(flags);

    if (ePIN_AS_GPIO_IN == gpio_cfg) {
        regval = __raw_readl(reg);
        regval &= ~(1 << bit);
        __raw_writel(regval, reg);
    } else if (ePIN_AS_GPIO_OUT == gpio_cfg){
        regval = __raw_readl(reg);
        regval |= (1 << bit);
        __raw_writel(regval, reg);
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
    u8 pupd, drive, ie, slew;
    struct ak_pinctrl *pc = pinctrl_dev_get_drvdata(pctldev);

    if (num_configs != 1)
        return -EINVAL;

    /*
    * configs
    * bit[0] pull up or pull down selection
    * bit[4] pull up or pull down enable
    * bit[9:8] drive
    * bit[16]:input enable
    * bit[24]: slew rate
    */
    /* for config value bit[31:24]--slew rate, bit[23:16]--ie,
    bit[15:8]--drive, bit[7:0]--pupd */
    pupd = (configs[0] & 0xFF);
    drive = ((configs[0]>>8) & 0xFF);
    ie = ((configs[0]>>16) & 0xFF);
    slew = ((configs[0]>>24) & 0xFF);

    local_irq_save(flags);
    pin_set_pull_enable(pc, pin, (pupd & 0x10) ? 1:0);
    pin_set_pull_polarity(pc, pin, (pupd & 0x01) ? 0:1);
    pin_set_drive(pin, drive & 0x3);
    pin_set_input_enable(pin, ie & 0x1);
    pin_set_slew_rate(pin, slew & 0x1);
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
    .npins = AK37E_NUM_GPIOS,
};

#ifdef CONFIG_PM
#define AK_WAKEUP_GPIO_CNT              25
static int ak_wakeup_gpio[AK_WAKEUP_GPIO_CNT] = {-1};
static int wakeup_gpio_cnt = 0;

u32 ak_ppu_ppd_en0, ak_ppu_ppd_en1,ak_ppu_ppd_en2;
u32 ak_gpio_dir_sel_reg0, ak_gpio_dir_sel_reg1,ak_gpio_dir_sel_reg2;


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
        pr_info("%s GPIO#%d\n",
                __func__, ak_wakeup_gpio[gpio_index]);
    }

    /* store value*/
    ak_ppu_ppd_en0 = __raw_readl(AK_PPU_PPD_EN0);
    ak_ppu_ppd_en1 = __raw_readl(AK_PPU_PPD_EN1);
    ak_ppu_ppd_en2 = __raw_readl(AK_PPU_PPD_EN2);

    ak_gpio_dir_sel_reg0 = __raw_readl(AK_GPIO_DIR_SEL_REG0);
    ak_gpio_dir_sel_reg1 = __raw_readl(AK_GPIO_DIR_SEL_REG1);
    ak_gpio_dir_sel_reg2 = __raw_readl(AK_GPIO_DIR_SEL_REG2);

    np = of_find_node_by_name(NULL, "lcdc");
    if (np && of_device_is_available(np)) {
        reset_pin = of_get_named_gpio(np, "reset-pins", 0);
        power_pin = of_get_named_gpio(np, "pwr-gpio", 0);
        backlight_pin = of_get_named_gpio(np, "backlight-gpio", 0);
        pr_info("lcd reset_pin %d, power_pin %d, backlight_pin %d\n",
                reset_pin, power_pin, backlight_pin);

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
        if ((i != reset_pin) && (i != power_pin) && (i != backlight_pin)) {
            regval = __raw_readl(AK_PPU_PPD_EN_REG(i));
            regval &= ~(0x1 << ak_sharepin[i].pupd_en_offset);
            __raw_writel(regval, AK_PPU_PPD_EN_REG(i));

            regval = __raw_readl(AK_GPIO_DIR_REG(i));
            regval &= ~(0x1 << AK_GPIO_OFFSET(i));
            __raw_writel(regval, AK_GPIO_DIR_REG(i));
        }
    }

    return 0;
}

static void ak_gpio_resume(void)
{
    int gpio_index = 0;
    
    /*
    * When resume, agaist the suspend
    * PPU_PPD default state: 0xFFFFFFFF
    */
    for (gpio_index = 0; gpio_index < wakeup_gpio_cnt; gpio_index++) {
        pr_info("%s GPIO#%d\n", __func__, ak_wakeup_gpio[gpio_index]);
    }

    __raw_writel(ak_ppu_ppd_en0, AK_PPU_PPD_EN0);
    __raw_writel(ak_ppu_ppd_en1, AK_PPU_PPD_EN1);
    __raw_writel(ak_ppu_ppd_en2, AK_PPU_PPD_EN2);

    __raw_writel(ak_gpio_dir_sel_reg0, AK_GPIO_DIR_SEL_REG0);
    __raw_writel(ak_gpio_dir_sel_reg1, AK_GPIO_DIR_SEL_REG1);
    __raw_writel(ak_gpio_dir_sel_reg2, AK_GPIO_DIR_SEL_REG2);


}
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

    BUILD_BUG_ON(ARRAY_SIZE(ak_gpio_pins) != AK37E_NUM_GPIOS);

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

    pc->irq_domain = irq_domain_add_linear(np, AK37E_NUM_GPIOS,
            &irq_domain_simple_ops, pc);
    if (!pc->irq_domain) {
        dev_err(dev, "could not create IRQ domain\n");
        ret = -ENOMEM;
        goto clk_err;
    }

    for (i = 0; i < AK37E_NUM_GPIOS; i++) {
        int irq = irq_create_mapping(pc->irq_domain, i);
        //pr_debug("i = %d irq = %d\n", i, irq);
        pc->irq_type[i] = 0;
        pc->fsel[i] = -1;
        irq_set_chip_and_handler(irq, &ak_gpio_irq_chip,
                handle_level_irq);
        irq_set_chip_data(irq, pc);
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
                ret = of_property_read_u32_index(np, "wakeup-mode", i,
                    &wakeup_mode);
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
        pr_info("%s wakeup GPIO#(%d)\n",
                __func__, ak_wakeup_gpio[gpio_index]);
    }
    
#endif
    register_syscore_ops(&ak_gpio_syscore_ops);

    return 0;

clk_err:
    clk_disable_unprepare(pc->clk);
    clk_put(pc->clk);

    return ret;
}/* end of func */

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
    { .compatible = "anyka,ak37e-gpio" },
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

MODULE_DESCRIPTION("AK37E sharepin control driver");
MODULE_AUTHOR("Anyka Microelectronic");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.02");
