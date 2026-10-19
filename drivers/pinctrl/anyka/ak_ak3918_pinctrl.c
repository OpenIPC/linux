/*
 * Driver for AK3918/AK3916 (Aimer39) GPIO (pinctrl + GPIO), derived from the
 * AK3918EV200 one: two GPIO banks of 32 instead of three, and the Aimer39
 * SDK's pad-mux and pull tables.
 *
 * Copyright (C) 2020 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
 * Copyright (C) 2026 Alex Zhang <alex@osqdu.org>
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
#include <linux/of_irq.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <dt-bindings/pinctrl/ak_37e_pinctrl.h>

#include <mach/map.h>

#define MODULE_NAME "ak-pinctrl-ak3918"
/* AK3918/AK3916 have pads 0..63, two banks of 32 */
#define AK37D_NUM_GPIOS 64
#define AK37D_NUM_BANKS 1

#define	AK_PULLUPDOWN_DISABLE	0
#define	AK_PULLUPDOWN_ENABLE	1

#define	ePIN_AS_GPIO_IN  0
#define	ePIN_AS_GPIO_OUT 1

#define	AK_GPIO_OUT_LOW			0
#define	AK_GPIO_OUT_HIGH		1

#define AK_GPIO_DIR1			0x00
#define AK_GPIO_OUT1			0x08
#define AK_GPIO_INPUT1         	0x10
#define AK_GPIO_INT_MASK1      	0x18
#define AK_GPIO_INT_MODE1      	0x20
#define AK_GPIO_INTP1         	0x28
#define AK_GPIO_EDGE_STATUS1	0x30

#define AK_GPIO_DIR_BASE(pin)			(((pin)>>5)*4 + AK_GPIO_DIR1)
#define AK_GPIO_OUT_BASE(pin)			(((pin)>>5)*4 + AK_GPIO_OUT1)
#define AK_GPIO_IN_BASE(pin)			(((pin)>>5)*4 + AK_GPIO_INPUT1)
#define AK_GPIO_INTEN_BASE(pin)			(((pin)>>5)*4 + AK_GPIO_INT_MASK1)
#define AK_GPIO_INTM_BASE(pin)			(((pin)>>5)*4 + AK_GPIO_INT_MODE1)
#define AK_GPIO_INTPOL_BASE(pin)		(((pin)>>5)*4 + AK_GPIO_INTP1)
#define AK_GPIO_INTEDGE_BASE(pin)		(((pin)>>5)*4 + AK_GPIO_EDGE_STATUS1)

#define AK_GPIO_REG_SHIFT(pin)	((pin) % 32)

/*Analog Control Register 3 (0x080000A4)*/
#define AK_ANALOG_CTRL_REG3		(AK_VA_SYSCTRL + 0x0A4)

#define AK_PERI_CHN_CLOCK_REG0	(AK_VA_SYSCTRL + 0x014)

#define AK_DUAL_SENSOR_SYNC_CFG_REG0	(AK_VA_CAMERA + 0x06c)

/* Three share-pin and three pull registers: no CON3/PULL_REG3 as on EV200. */
#define AK_SHAREPIN_CON0		(AK_VA_SYSCTRL + 0x074)
#define AK_SHAREPIN_CON1		(AK_VA_SYSCTRL + 0x078)
#define AK_SHAREPIN_CON2		(AK_VA_SYSCTRL + 0x07c)

#define AK_PULL_REG0			(AK_VA_SYSCTRL + 0x080)
#define AK_PULL_REG1			(AK_VA_SYSCTRL + 0x084)
#define AK_PULL_REG2			(AK_VA_SYSCTRL + 0x088)

#define AK37D_GPIO_PIN(a) PINCTRL_PIN(a, "gpio" #a)

#define DVP_DATA_MAP_REG0	(AK_VA_CAMERA + 0x060)
#define DVP_DATA_MAP_REG1	(AK_VA_CAMERA + 0x064)

#define PIN_MAP_FLAG	(0x1)
#define SHIFT_FLAG		(24)
#define SHIFT_NEW		(12)
#define SHIFT_OLD		(0)
#define PACK_PIN_MAP(pin_new, pin_old) ((PIN_MAP_FLAG << SHIFT_FLAG) |\
		((pin_new) << SHIFT_NEW) |\
		((pin_old) << SHIFT_OLD))
#define UNPACK_FLAG(config)		((config) >> SHIFT_FLAG)
#define UNPACK_PIN_MAP_NEW(config)		(((config) >> SHIFT_NEW) & (~((~0UL) << (SHIFT_FLAG - SHIFT_NEW))))
#define UNPACK_PIN_MAP_OLD(config)		(((config) >> SHIFT_OLD) & (~((~0UL) << (SHIFT_NEW - SHIFT_OLD))))

struct gpio_sharepin {
	int pin;

	void __iomem *reg0;
	unsigned int reg0_mask;
	void __iomem *reg1;
	unsigned int reg1_mask;
	void __iomem *reg2;
	unsigned int reg2_mask;

	int func0;
	void __iomem *func0_reg;
	unsigned int func0_mask;
	unsigned int func0_value;

	int func1;
	void __iomem *func1_reg;
	unsigned int func1_mask;
	unsigned int func1_value;

	int func2;
	void __iomem *func2_reg;
	unsigned int func2_mask;
	unsigned int func2_value;

	void __iomem *pupd_en_reg;
	int sel_bit;
	int is_pullup;
};

struct ak_pinctrl {
	struct device *dev;
	struct clk *clk;
	void __iomem *gpio_base;
	void __iomem *mipi0_base;
	int irq;

	unsigned int irq_type[AK37D_NUM_GPIOS];
	int fsel[AK37D_NUM_GPIOS];

	struct pinctrl_dev *pctl_dev;
	struct irq_domain *irq_domain;
	struct gpio_chip gc;
	struct pinctrl_gpio_range gpio_range;

	int GPI_MIPI0_pull_polarity;
	int mipi0_lanes_num;
	int ttl_io_voltage;

	spinlock_t lock;
};

static struct pinctrl_pin_desc ak_gpio_pins[] = {
	AK37D_GPIO_PIN(0),
	AK37D_GPIO_PIN(1),
	AK37D_GPIO_PIN(2),
	AK37D_GPIO_PIN(3),
	AK37D_GPIO_PIN(4),
	AK37D_GPIO_PIN(5),
	AK37D_GPIO_PIN(6),
	AK37D_GPIO_PIN(7),
	AK37D_GPIO_PIN(8),
	AK37D_GPIO_PIN(9),
	AK37D_GPIO_PIN(10),
	AK37D_GPIO_PIN(11),
	AK37D_GPIO_PIN(12),
	AK37D_GPIO_PIN(13),
	AK37D_GPIO_PIN(14),
	AK37D_GPIO_PIN(15),
	AK37D_GPIO_PIN(16),
	AK37D_GPIO_PIN(17),
	AK37D_GPIO_PIN(18),
	AK37D_GPIO_PIN(19),
	AK37D_GPIO_PIN(20),
	AK37D_GPIO_PIN(21),
	AK37D_GPIO_PIN(22),
	AK37D_GPIO_PIN(23),
	AK37D_GPIO_PIN(24),
	AK37D_GPIO_PIN(25),
	AK37D_GPIO_PIN(26),
	AK37D_GPIO_PIN(27),
	AK37D_GPIO_PIN(28),
	AK37D_GPIO_PIN(29),
	AK37D_GPIO_PIN(30),
	AK37D_GPIO_PIN(31),
	AK37D_GPIO_PIN(32),
	AK37D_GPIO_PIN(33),
	AK37D_GPIO_PIN(34),
	AK37D_GPIO_PIN(35),
	AK37D_GPIO_PIN(36),
	AK37D_GPIO_PIN(37),
	AK37D_GPIO_PIN(38),
	AK37D_GPIO_PIN(39),
	AK37D_GPIO_PIN(40),
	AK37D_GPIO_PIN(41),
	AK37D_GPIO_PIN(42),
	AK37D_GPIO_PIN(43),
	AK37D_GPIO_PIN(44),
	AK37D_GPIO_PIN(45),
	AK37D_GPIO_PIN(46),
	AK37D_GPIO_PIN(47),
	AK37D_GPIO_PIN(48),
	AK37D_GPIO_PIN(49),
	AK37D_GPIO_PIN(50),
	AK37D_GPIO_PIN(51),
	AK37D_GPIO_PIN(52),
	AK37D_GPIO_PIN(53),
	AK37D_GPIO_PIN(54),
	AK37D_GPIO_PIN(55),
	AK37D_GPIO_PIN(56),
	AK37D_GPIO_PIN(57),
	AK37D_GPIO_PIN(58),
	AK37D_GPIO_PIN(59),
	AK37D_GPIO_PIN(60),
	AK37D_GPIO_PIN(61),
	AK37D_GPIO_PIN(62),
	AK37D_GPIO_PIN(63),
};

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
};

static const char * const ak_funcs[5] = {
	[0] = "default func",
	[1] = "func1",
	[2] = "func2",
	[3] = "func3",
	[4] = "func4",
};

static struct gpio_sharepin ak_sharepin[] = {
/*
 * AK3918/AK3916 (Aimer39) pad-mux + pull table, generated from the Aimer39
 * SDK's arch/arm/mach-ak39/ak39-gpio.c (sharepin_cfg_gpio1[] for
 * SHAREPIN_CON0 at 0x74, the pin-8 offset rule of g_ak39_setpin_as_gpio()
 * for CON1 at 0x78, sharepin_cfg_gpio3[] for CON2 at 0x7c, pupd_cfg_info[]
 * for the three pull registers at 0x80/0x84/0x88). Field encoding as in the
 * EV200 table: the function number is the field value, 0 is GPIO -- except
 * pads 8-11, whose CON1 bit is GPIO when set (gpio_set_share() selects 1).
 * Pads 49, 58, 59, 60, 61, 62, 63 have no share field and are GPIO only; pads 49 have no pull.
 */
	{0, AK_SHAREPIN_CON0, (1<<0), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 0, 1},
	{1, AK_SHAREPIN_CON0, (1<<14), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 19, 1},
	{2, AK_SHAREPIN_CON0, (1<<15), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 20, 1},
	{3, AK_SHAREPIN_CON0, (1<<1), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 1, 0},
	{4, AK_SHAREPIN_CON0, (0x3<<16), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 21, 1},
	{5, AK_SHAREPIN_CON0, (0x3<<18), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 22, 1},
	{6, AK_SHAREPIN_CON0, (0x3<<20), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 23, 1},
	{7, AK_SHAREPIN_CON0, (0x3<<22), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 24, 1},
	{8, AK_SHAREPIN_CON1, (1<<0), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 4, 0},
	{9, AK_SHAREPIN_CON1, (1<<1), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 5, 0},
	{10, AK_SHAREPIN_CON1, (1<<2), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 7, 0},
	{11, AK_SHAREPIN_CON1, (1<<3), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 8, 0},
	{12, AK_SHAREPIN_CON1, (1<<4), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 10, 0},
	{13, AK_SHAREPIN_CON1, (1<<5), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 11, 0},
	{14, AK_SHAREPIN_CON1, (1<<6), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 14, 0},
	{15, AK_SHAREPIN_CON1, (1<<7), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 15, 0},
	{16, AK_SHAREPIN_CON1, (1<<8), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 16, 0},
	{17, AK_SHAREPIN_CON1, (1<<9), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 17, 0},
	{18, AK_SHAREPIN_CON1, (1<<10), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 18, 0},
	{19, AK_SHAREPIN_CON1, (1<<11), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 20, 0},
	{20, AK_SHAREPIN_CON1, (1<<12), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 21, 0},
	{21, AK_SHAREPIN_CON1, (1<<13), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 22, 0},
	{22, AK_SHAREPIN_CON1, (1<<14), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 23, 0},
	{23, AK_SHAREPIN_CON1, (1<<15), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 25, 0},
	{24, AK_SHAREPIN_CON1, (1<<16), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG1, 26, 0},
	{25, AK_SHAREPIN_CON2, (1<<0), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 0, 1},
	{26, AK_SHAREPIN_CON2, (1<<1), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 1, 1},
	{27, AK_SHAREPIN_CON2, (1<<25), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 20, 1},
	{28, AK_SHAREPIN_CON2, (1<<26), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 21, 1},
	{29, AK_SHAREPIN_CON2, (0x3<<2), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 2, 1},
	{30, AK_SHAREPIN_CON2, (0x3<<4), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 3, 1},
	{31, AK_SHAREPIN_CON2, (1<<6), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 4, 1},
	{32, AK_SHAREPIN_CON2, (1<<7), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 5, 1},
	{33, AK_SHAREPIN_CON2, (1<<8), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 6, 1},
	{34, AK_SHAREPIN_CON2, (1<<9), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 7, 1},
	{35, AK_SHAREPIN_CON2, (1<<9), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 8, 1},
	{36, AK_SHAREPIN_CON2, (1<<9), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 9, 1},
	{37, AK_SHAREPIN_CON2, (0x3<<10), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 10, 1},
	{38, AK_SHAREPIN_CON2, (0x3<<10), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 11, 1},
	{39, AK_SHAREPIN_CON2, (0x3<<12), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 12, 1},
	{40, AK_SHAREPIN_CON2, (0x3<<14), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 13, 1},
	{41, AK_SHAREPIN_CON2, (1<<16), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 14, 1},
	{42, AK_SHAREPIN_CON2, (1<<17), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 15, 1},
	{43, AK_SHAREPIN_CON2, (0x3<<18), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 16, 1},
	{44, AK_SHAREPIN_CON2, (0x3<<20), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 17, 1},
	{45, AK_SHAREPIN_CON2, (0x3<<22), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 18, 1},
	{46, AK_SHAREPIN_CON2, (0x3<<22), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG2, 19, 1},
	{47, AK_SHAREPIN_CON0, (0x3<<2), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 2, 0},
	{48, AK_SHAREPIN_CON0, (1<<4), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 3, 0},
	{49, NULL, 0, NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		NULL, 0, 0},
	{50, AK_SHAREPIN_CON0, (1<<6), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 5, 1},
	{51, AK_SHAREPIN_CON0, (1<<7), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 6, 1},
	{52, AK_SHAREPIN_CON0, (1<<8), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 7, 1},
	{53, AK_SHAREPIN_CON0, (1<<9), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 8, 1},
	{54, AK_SHAREPIN_CON0, (1<<10), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 9, 0},
	{55, AK_SHAREPIN_CON0, (1<<11), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 10, 0},
	{56, AK_SHAREPIN_CON0, (1<<12), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 11, 1},
	{57, AK_SHAREPIN_CON0, (1<<13), NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 12, 1},
	{58, NULL, 0, NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 13, 1},
	{59, NULL, 0, NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 14, 0},
	{60, NULL, 0, NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 15, 0},
	{61, NULL, 0, NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 16, 0},
	{62, NULL, 0, NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 17, 1},
	{63, NULL, 0, NULL, 0, NULL, 0,
		-1, NULL, 0, 0, -1, NULL, 0, 0, -1, NULL, 0, 0,
		AK_PULL_REG0, 18, 1},
};

static const char * const irq_type_names[] = {
	[IRQ_TYPE_NONE] = "none",
	[IRQ_TYPE_EDGE_RISING] = "edge-rising",
	[IRQ_TYPE_EDGE_FALLING] = "edge-falling",
	[IRQ_TYPE_LEVEL_HIGH] = "level-high",
	[IRQ_TYPE_LEVEL_LOW] = "level-low",
};

static int mipi_pins_set_share_ex(struct ak_pinctrl *pc, int pin, int sel)
{
	int mipi_lanes_num;
	void __iomem *mipi_base;
	int mode;/*0:dvp; 1:mipi; 2:gpi or others*/

	if (pin >= 70 && pin <= 75) {
		mipi_base = pc->mipi0_base;
		mipi_lanes_num = pc->mipi0_lanes_num;

		if (pin >= 73 && pin <= 75) {
			switch (sel) {
				case 0:
					mode = 0;
					break;
				case 1:
					mode = 2;
					break;
				case 2:
					mode = 1;
					break;
				default:
					mode = -1;
					break;
			}
		} else {
			switch (sel) {
				case 0:
					mode = 0;
					break;
				case 1:
				case 2:
					mode = 2;
					break;
				case 3:
					mode = 1;
					break;
				default:
					mode = -1;
					break;
			}
		}

		if (mode == 0) {
			/*dvp mode*/
			__raw_writeb(0x7d, mipi_base);
			__raw_writeb(0x3f, mipi_base + 0x20);
			__raw_writeb(0x01, mipi_base + 0xb3);
			__raw_writeb(pc->ttl_io_voltage & 0xff, mipi_base + 0xb8);
		} else if (mode == 1) {
			/*mipi mode*/
			__raw_writeb(0x7d, mipi_base);
			if (mipi_lanes_num == 1)
				__raw_writeb(0xf8, mipi_base + 0xe0);
			else if (mipi_lanes_num == 2) {
				__raw_writeb(0xf9, mipi_base + 0xe0);
			} else {
				pr_debug("%s err mipi_lanes_num:%d, pin:%d\n", __func__, mipi_lanes_num, pin);
				return -1;
			}
		} else if (mode == 2) {
			/*GPI mode*/
			__raw_writeb(0x7d, mipi_base);
			__raw_writeb(0x3f, mipi_base + 0x20);
			__raw_writeb(0x01, mipi_base + 0xb3);
		} else {
			pr_err("%s don't support sel:%d, pin:%d\n", __func__, sel, pin);
			return -1;
		}
	}

	return 0;
}

static int mipi_pins_set_pull_polarity(struct ak_pinctrl *pc, int pin, int pullup)
{
	int done = 0;
	int *p_pull_polarity;
	void __iomem *mipi_base;

	if (pin >= 70 && pin <= 75) {
		mipi_base = pc->mipi0_base;
		p_pull_polarity = &pc->GPI_MIPI0_pull_polarity;

		if (pullup) {
			__raw_writeb(0x80, mipi_base + 0x0d);//enable pullup
			__raw_writeb(0x02, mipi_base + 0xb8);//disable pulldown
			*p_pull_polarity = 1;
		} else {
			__raw_writeb(0xb0, mipi_base + 0x0d);//disable pullup
			__raw_writeb(0x04, mipi_base + 0xb8);//enable pulldown
			*p_pull_polarity = 0;
		}
		done = 1;
	}

	return (done ? 0:-1);
}

static int mipi_pins_get_pull_polarity(struct ak_pinctrl *pc, int pin, int *polarity)
{
	unsigned int valup, valdown;
	int *p_pull_polarity;
	void __iomem *mipi_base;

	if (pin >= 70 && pin <= 75) {
		mipi_base = pc->mipi0_base;
		p_pull_polarity = &pc->GPI_MIPI0_pull_polarity;

		valup = __raw_readb(mipi_base + 0x0d);
		valdown = __raw_readb(mipi_base + 0xb8);

		if (valup == 0x80)
			*polarity = 1;
		else if (valdown == 0x04)
			*polarity = 0;
		else
			*polarity = 0;

		return 0;
	}

	return -1;
}

static int mipi_pins_set_pull_enable(struct ak_pinctrl *pc, int pin, int enable)
{
	int done = 0;

	if (pin >= 70 && pin <= 75) {
		int *p_pull_polarity;
		void __iomem *mipi_base;

		mipi_base = pc->mipi0_base;
		p_pull_polarity = &pc->GPI_MIPI0_pull_polarity;

		if (enable) {
			if (*p_pull_polarity) {
				__raw_writeb(0x80, mipi_base + 0x0d);//enable pullup
				__raw_writeb(0x02, mipi_base + 0xb8);//disable pulldown
			} else {
				__raw_writeb(0xb0, mipi_base + 0x0d);//disable pullup
				__raw_writeb(0x04, mipi_base + 0xb8);//enable pulldown
			}
		} else {
			__raw_writeb(0xb0, mipi_base + 0x0d);//disable pullup
			__raw_writeb(0x02, mipi_base + 0xb8);//disable pulldown
		}

		done = 1;
	}

	return (done ? 0:-1);
}

static int mipi_pins_get_pull_enable(struct ak_pinctrl *pc, int pin, int *enable)
{
	unsigned int valup, valdown;
	int *p_pull_polarity;
	void __iomem *mipi_base;

	if (pin >= 70 && pin <= 75) {
		mipi_base = pc->mipi0_base;
		p_pull_polarity = &pc->GPI_MIPI0_pull_polarity;

		valup = __raw_readb(mipi_base + 0x0d);
		valdown = __raw_readb(mipi_base + 0xb8);

		if (valup == 0xb0 && valdown == 0x02)
			*enable = 0;
		else
			*enable = 1;
		return 0;
	}

	return -1;
}

static int set_reg0_sel(int pin, int sel)
{
	int reg0_mask_bits = 0;
	int offset = 0;
	unsigned int sel_tmp = sel;
	unsigned int mask = ak_sharepin[pin].reg0_mask;
	unsigned int regval;

	if (!ak_sharepin[pin].reg0)
		return 0;

	regval = __raw_readl(ak_sharepin[pin].reg0);
	while (mask) {
		if (mask & 0x1) {
			regval |= (sel_tmp & 0x1) << offset;
			sel_tmp >>= 1;
			reg0_mask_bits++;
		}
		mask >>= 1;
		offset++;
	}

	if (sel < (1<<reg0_mask_bits)) {
		__raw_writel(regval, ak_sharepin[pin].reg0);
		return 0;
	}

	return -1;
}

static int pin_set_share(struct ak_pinctrl *pc, int pin, int sel)
{
	unsigned int regval;

	if (ak_sharepin[pin].reg0) {
		regval = __raw_readl(ak_sharepin[pin].reg0);
		regval &= ~ak_sharepin[pin].reg0_mask;
		__raw_writel(regval, ak_sharepin[pin].reg0);
	}
	if (ak_sharepin[pin].reg1) {
		regval = __raw_readl(ak_sharepin[pin].reg1);
		regval &= ~ak_sharepin[pin].reg1_mask;
		__raw_writel(regval, ak_sharepin[pin].reg1);
	}
	if (ak_sharepin[pin].reg2) {
		regval = __raw_readl(ak_sharepin[pin].reg2);
		regval &= ~ak_sharepin[pin].reg2_mask;
		__raw_writel(regval, ak_sharepin[pin].reg2);
	}

	set_reg0_sel(pin, sel);

	if ((ak_sharepin[pin].func0 == sel) &&
			ak_sharepin[pin].func0_reg) {
		regval = __raw_readl(ak_sharepin[pin].func0_reg);
		regval &= ~ak_sharepin[pin].func0_mask;
		regval |= ak_sharepin[pin].func0_value;
		__raw_writel(regval, ak_sharepin[pin].func0_reg);
	}
	if ((ak_sharepin[pin].func1 == sel) &&
			ak_sharepin[pin].func1_reg) {
		regval = __raw_readl(ak_sharepin[pin].func1_reg);
		regval &= ~ak_sharepin[pin].func1_mask;
		regval |= ak_sharepin[pin].func1_value;
		__raw_writel(regval, ak_sharepin[pin].func1_reg);
	}
	if ((ak_sharepin[pin].func2 == sel) &&
			ak_sharepin[pin].func2_reg) {
		regval = __raw_readl(ak_sharepin[pin].func2_reg);
		regval &= ~ak_sharepin[pin].func2_mask;
		regval |= ak_sharepin[pin].func2_value;
		__raw_writel(regval, ak_sharepin[pin].func2_reg);
	}

	mipi_pins_set_share_ex(pc, pin, sel);

	pc->fsel[pin] = sel;
	return 0;
}

static int pin_set_pull_polarity(struct ak_pinctrl *pc, int pin, int pullup)
{
	/* Only the MIPI pins support both pull-up and pull-down; every
	 * other pin supports just one of the two.
	 */

	mipi_pins_set_pull_polarity(pc, pin, pullup);

	return 0;
}

static int pin_get_pull_polarity(struct ak_pinctrl *pc, int pin)
{
	int polarity;
	unsigned int regval;

	if (ak_sharepin[pin].pupd_en_reg) {
		regval = __raw_readl(ak_sharepin[pin].pupd_en_reg);
		regval &= (1<<ak_sharepin[pin].sel_bit);
		return ((!regval && ak_sharepin[pin].is_pullup) ? 0:1);
	}

	if (!mipi_pins_get_pull_polarity(pc, pin, &polarity))
		return polarity;

	pr_err("%s can't get pin:%d pull polarity\n", __func__, pin);
	return 0;
}

static int pin_set_pull_enable(struct ak_pinctrl *pc, int pin, int enable)
{
	int done = 0;
	unsigned int regval;

	if (ak_sharepin[pin].pupd_en_reg) {
		regval = __raw_readl(ak_sharepin[pin].pupd_en_reg);
		regval &= ~(1<<ak_sharepin[pin].sel_bit);
		if (!enable)
			regval |= (1<<ak_sharepin[pin].sel_bit);
		__raw_writel(regval, ak_sharepin[pin].pupd_en_reg);
		done = 1;
	}

	if (!mipi_pins_set_pull_enable(pc, pin, enable))
		done = 1;

	if (!done)
		;
	return 0;
}

static int pin_get_pull_enable(struct ak_pinctrl *pc, int pin)
{
	int enable;
	unsigned int regval;

	if (ak_sharepin[pin].pupd_en_reg) {
		regval = __raw_readl(ak_sharepin[pin].pupd_en_reg);
		regval &= (1<<ak_sharepin[pin].sel_bit);
		return (regval ? 0:1);
	}

	if (!mipi_pins_get_pull_enable(pc, pin, &enable))
		return enable;

	pr_err("%s can't get pin:%d pull enable\n", __func__, pin);
	return 0;
}

static int gpio_set_share(struct ak_pinctrl *pc, unsigned offset)
{
	int sel;
	unsigned long flags;

	/* Pads 8-11 are GPIO with their CON1 bit set; every other pad
	 * with its field clear. */
	if (8 <= offset && offset <= 11)
		sel = 1;
	else
		sel = 0;

	local_irq_save(flags);
	pin_set_share(pc, offset, sel);
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

static int ak_gpio_set_drive(struct gpio_chip *chip, unsigned offset, int strength)
{
	return 0;
}

static int ak_gpio_get_drive(struct gpio_chip *chip, unsigned offset)
{
	return 0;
}

static int ak_gpio_set_pull_polarity(struct gpio_chip *chip, unsigned offset, int pullup)
{
	struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);
	pin_set_pull_polarity(pc, offset, pullup);

	return 0;
}

static int ak_gpio_get_pull_polarity(struct gpio_chip *chip, unsigned offset)
{
	struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);
	return pin_get_pull_polarity(pc, offset);
}

static int ak_gpio_set_pull_enable(struct gpio_chip *chip, unsigned offset, int enable)
{
	struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);
	pin_set_pull_enable(pc, offset, enable);
	return 0;
}

static int ak_gpio_get_pull_enable(struct gpio_chip *chip, unsigned offset)
{
	struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);
	return pin_get_pull_enable(pc, offset);
}

static int ak_gpio_set_input_enable(struct gpio_chip *chip, unsigned offset, int enable)
{
	return 0;
}

static int ak_gpio_get_input_enable(struct gpio_chip *chip, unsigned offset)
{
	return 0;
}

static int ak_gpio_set_slew_rate(struct gpio_chip *chip, unsigned offset, int fast)
{
	return 0;
}

static int ak_gpio_get_slew_rate(struct gpio_chip *chip, unsigned offset)
{
	return 0;
}

static inline void ak_pinctrl_fsel_reset(
		struct ak_pinctrl *pc, unsigned pin,
		unsigned fsel)
{
	int cur = pc->fsel[pin];

	if (cur == -1)
		return;

	pc->fsel[pin] = -1;
}


static int ak_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	return pinctrl_gpio_direction_input(chip->base + offset);
}

static int ak_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);
	void __iomem * reg = pc->gpio_base + AK_GPIO_IN_BASE(offset);
	unsigned int bit = AK_GPIO_REG_SHIFT(offset);

	return (__raw_readl(reg) & (1<<bit)) ? 1:0;
}

static void ak_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct ak_pinctrl *pc = dev_get_drvdata(chip->dev);
	void __iomem *reg = pc->gpio_base + AK_GPIO_OUT_BASE(offset);
	unsigned int bit = AK_GPIO_REG_SHIFT(offset);
	unsigned long flags;
	unsigned int regval;

	local_irq_save(flags);
	if (AK_GPIO_OUT_LOW == value) {
		regval = __raw_readl(reg);
		regval &= ~(1<<bit);
		__raw_writel(regval, reg);
	} else if (AK_GPIO_OUT_HIGH == value) {
		regval = __raw_readl(reg);
		regval |= (1<<bit);
		__raw_writel(regval, reg);
	}
	local_irq_restore(flags);
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
	.ngpio = AK37D_NUM_GPIOS,
	.can_sleep = false,
};

static void ak_pinctrl_irq_handler(struct irq_desc *desc)
{
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct ak_pinctrl *pc = irq_desc_get_handler_data(desc);
	unsigned long irq_status;
	int i;

	/*
	 * Two banks on this part, and the domain maps only their 64 pins: a
	 * third status word (EV200's layout) would hand generic_handle_irq()
	 * an unmapped IRQ. One enter/exit pair per invocation.
	 */
	chained_irq_enter(chip, desc);
	for (i = 0; i < AK37D_NUM_GPIOS / 32; i++) {
		int off = 0;

		irq_status = __raw_readl(pc->gpio_base + AK_GPIO_EDGE_STATUS1 + i * 4);
		for_each_set_bit(off, &irq_status, 32) {
			int pin_irq = irq_find_mapping(pc->irq_domain, i * 32 + off);

			if (pin_irq)
				generic_handle_irq(pin_irq);
		}
	}
	chained_irq_exit(chip, desc);
}

static void ak_gpio_irq_enable(struct irq_data *data)
{
	struct ak_pinctrl *pc = irq_data_get_irq_chip_data(data);
	unsigned pin = irqd_to_hwirq(data);
	void __iomem *reg = pc->gpio_base + AK_GPIO_INTEN_BASE(pin);
	unsigned bit = AK_GPIO_REG_SHIFT(pin);
	unsigned long flags;

	unsigned int regval;

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

	unsigned int regval;

	spin_lock_irqsave(&pc->lock, flags);
	regval = __raw_readl(reg);
	regval &= ~(1 << bit);
	__raw_writel(regval, reg);
	spin_unlock_irqrestore(&pc->lock, flags);
}

#define GPIO_BASE	0xF00D0000
int ak_request_edge_trigger_irq(int gpio, unsigned int irq,
	irq_handler_t handler, const char *name, void *dev)
{
	u32 val, direction;
	void __iomem * reg_direction = GPIO_BASE + AK_GPIO_DIR_BASE(gpio);
	void __iomem * reg_data = GPIO_BASE + AK_GPIO_IN_BASE(gpio);
	unsigned int bit = AK_GPIO_REG_SHIFT(gpio);
	int ret = 0;

	if ((gpio < XGPIO_000) || (gpio > XGPIO_086))
		return -EINVAL;

	/*
	* direction = 0 means output mode
	* direction = 1 means input mode
	*/
	direction = ((__raw_readl(reg_direction) & (1 << bit)) ? 0 : 1);
	if (direction == 0)
		return -EINVAL;

	val = (__raw_readl(reg_data) & (1 << bit)) ? 1 : 0;
	if (val) {
		ret = request_irq(irq, handler, IRQF_TRIGGER_FALLING|IRQF_ONESHOT, name, dev);
	} else {
		ret = request_irq(irq, handler, IRQF_TRIGGER_RISING|IRQF_ONESHOT, name, dev);
	}

	return 0;
}
EXPORT_SYMBOL(ak_request_edge_trigger_irq);
int ak_exchange_irq_edge_trigger_unlocked(int gpio)
{
	u32 val, reg_val;
	void __iomem * reg_data = GPIO_BASE + AK_GPIO_IN_BASE(gpio);
	void __iomem * reg_polarity = GPIO_BASE + AK_GPIO_INTPOL_BASE(gpio);
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
EXPORT_SYMBOL(ak_exchange_irq_edge_trigger_unlocked);
static int __ak_gpio_irq_set_type_enabled(struct ak_pinctrl *pc,
	unsigned offset, unsigned int type)
{
	unsigned int regval;
	void __iomem *reg_inten = pc->gpio_base + AK_GPIO_INTEN_BASE(offset);
	void __iomem *reg_intm  = pc->gpio_base + AK_GPIO_INTM_BASE(offset);
	void __iomem *reg_intp  = pc->gpio_base + AK_GPIO_INTPOL_BASE(offset);

	int bit = AK_GPIO_REG_SHIFT(offset);
 
	switch (type) {
	case IRQ_TYPE_NONE:
		if (pc->irq_type[offset] != type) {
			regval = __raw_readl(reg_inten);
			regval &= ~(1<<bit);
			__raw_writel(regval, reg_inten);
			pc->irq_type[offset] = type;
		}
		break;

	case IRQ_TYPE_EDGE_RISING:
	case IRQ_TYPE_EDGE_FALLING:
		if (pc->irq_type[offset] != type) {
			regval = __raw_readl(reg_intm);
			regval |= (1<<bit);
			__raw_writel(regval, reg_intm);

			regval = __raw_readl(reg_intp);
			if (IRQ_TYPE_EDGE_RISING == type)
				regval &= ~(1<<bit);
			else
				regval |= (1<<bit);
			__raw_writel(regval, reg_intp);
			pc->irq_type[offset] = type;
		}
		break;

	case IRQ_TYPE_LEVEL_HIGH:
	case IRQ_TYPE_LEVEL_LOW:
		if (pc->irq_type[offset] != type) {
			regval = __raw_readl(reg_intm);
			regval &= ~(1<<bit);
			__raw_writel(regval, reg_intm);

			regval = __raw_readl(reg_intp);
			if (IRQ_TYPE_LEVEL_HIGH == type)
				regval &= ~(1<<bit);
			else
				regval |= (1<<bit);
			__raw_writel(regval, reg_intp);
			pc->irq_type[offset] = type;
		}
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

static int ak_gpio_irq_set_type(struct irq_data *data, unsigned int type)
{
	struct ak_pinctrl *pc = irq_data_get_irq_chip_data(data);
	unsigned pin = irqd_to_hwirq(data);

	unsigned long flags;
	int ret;

	spin_lock_irqsave(&pc->lock, flags);

	ret = __ak_gpio_irq_set_type_enabled(pc, pin, type);

	if (type & IRQ_TYPE_EDGE_BOTH)
		irq_set_handler_locked(data, handle_edge_irq);
	else
		irq_set_handler_locked(data, handle_level_irq);
	spin_unlock_irqrestore(&pc->lock, flags);

	return ret;
}


static void ak_gpio_irq_ack(struct irq_data *d)
{
	struct ak_pinctrl *pc = irq_data_get_irq_chip_data(d);
	unsigned pin = irqd_to_hwirq(d);

	/* Clear the IRQ status*/
    __raw_readl(pc->gpio_base + AK_GPIO_INTEDGE_BASE(pin));

}

static struct irq_chip ak_gpio_irq_chip = {
	.name = "ak_gpio_edge",
    .irq_ack	= ak_gpio_irq_ack,
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
		struct pinctrl_map **maps, int num_pin_maps)
{
	struct pinctrl_map *map = *maps;
	unsigned long *configs;
	unsigned int pin_old, pin_new;
	int find_pin_map = 0;
	int err;
	int i;

	for (i = 0; i < num_pin_maps; i++) {
		err = of_property_read_u32_index(np, "anyka,map",
				i * 2, &pin_new);
		if (err) {
			pr_err("%s get anyka,maps i:%d fail\n", __func__, i);
			return -EINVAL;
		}

		if (pin_new == pin) {
			err = of_property_read_u32_index(np, "anyka,map",
					i * 2 + 1, &pin_old);
			if (err) {
				pr_err("%s get anyka,maps i+1:%d fail\n", __func__, i+1);
				return -EINVAL;
			}

			find_pin_map = 1;
			break;
		}
	}

	configs = kzalloc((find_pin_map + 1) * sizeof(*configs), GFP_KERNEL);
	if (!configs)
		return -ENOMEM;
	configs[0] = pull;

	map->type = PIN_MAP_TYPE_CONFIGS_PIN;
	map->data.configs.group_or_pin = ak_gpio_pins[pin].name;
	map->data.configs.configs = configs;
	map->data.configs.num_configs = 1;
	(*maps)++;

	if (find_pin_map) {
		configs[1] = PACK_PIN_MAP(pin_new, pin_old);
		map->data.configs.num_configs += 1;
		pr_debug("%s pin_new:%d, pin_old:%d\n", __func__, pin_new, pin_old);
	}

	return 0;
}

static int ak_pctl_dt_node_to_map(struct pinctrl_dev *pctldev,
		struct device_node *np,
		struct pinctrl_map **map, unsigned *num_maps)
{
	struct ak_pinctrl *pc = pinctrl_dev_get_drvdata(pctldev);
	struct property *pins, *funcs, *pulls, *pin_maps;
	int num_pins, num_funcs, num_pulls, maps_per_pin, num_pin_maps;
	struct pinctrl_map *maps, *cur_map;
	int i, err;
	unsigned int pin, func, pull;

	pins = of_find_property(np, "anyka,pins", NULL);
	if (!pins) {
		dev_err(pc->dev, "%s: missing anyka,pins property\n",
				of_node_full_name(np));
		return -EINVAL;
	}

	funcs = of_find_property(np, "anyka,function", NULL);
	pulls = of_find_property(np, "anyka,pull", NULL);
	pin_maps = of_find_property(np, "anyka,map", NULL);

	if (!funcs && !pulls) {
		dev_err(pc->dev,
			"%s: neither anyka,function nor anyka,pull specified\n",
			of_node_full_name(np));
		return -EINVAL;
	}

	num_pins = pins->length / 4;
	num_funcs = funcs ? (funcs->length / 4) : 0;
	num_pulls = pulls ? (pulls->length / 4) : 0;
	num_pin_maps = pin_maps ? (pin_maps->length / 4) : 0;

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

	if (num_pin_maps % 2) {
		pr_err("anyka,maps is not in pairs\n");
		return -EINVAL;
	}

	num_pin_maps /= 2;

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
							pull, &cur_map, num_pin_maps);
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

	ak_pinctrl_fsel_reset(pc, offset, pc->fsel[offset]);
}

static int ak_pmx_gpio_set_direction(struct pinctrl_dev *pctldev,
		struct pinctrl_gpio_range *range,
		unsigned offset,
		bool input)
{
	struct ak_pinctrl *pc = pinctrl_dev_get_drvdata(pctldev);
	unsigned long flags;
	unsigned int regval;
	void __iomem *reg = pc->gpio_base + AK_GPIO_DIR_BASE(offset);
	unsigned int bit = AK_GPIO_REG_SHIFT(offset);
	int gpio_cfg = input ?
		ePIN_AS_GPIO_IN : ePIN_AS_GPIO_OUT;

	local_irq_save(flags);

	if (ePIN_AS_GPIO_IN == gpio_cfg){
		regval = __raw_readl(reg);
		regval &= ~(1<<bit);
		__raw_writel(regval, reg);
	}else if (ePIN_AS_GPIO_OUT == gpio_cfg){
		regval = __raw_readl(reg);
		regval |= (1<<bit);
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

static int set_dvp_data_map(int new, int old)
{
	/*0x20000060, 0x20000064*/
	unsigned long value;
	void __iomem *old_vaddr = NULL;
	int old_offset;
	int new_value = -1;
	int i;
	int gpio_to_dvp[][2] = {
		{6, 0},
		{7, 1},
		{8, 2},
		{9, 3},
		{68, 4},
		{69, 5},
		{75, 6},
		{74, 7},
		{71, 8},
		{70, 9},
		{72, 10},
		{73, 11},
	};

	for (i = 0; i < sizeof(gpio_to_dvp)/sizeof(gpio_to_dvp[0][0])/2; i++) {
		if (gpio_to_dvp[i][0] == old) {
			int data = gpio_to_dvp[i][1];

			/*4bits for a map, so a reg for 8 maps*/
			old_offset = ((data % 8) * 4);
			old_vaddr = (data >= 8) ? DVP_DATA_MAP_REG1:DVP_DATA_MAP_REG0;
			break;
		}
	}

	if (!old_vaddr) {
		pr_err("%s donot find old:%d\n", __func__, old);
		return -EINVAL;
	}

	for (i = 0; i < sizeof(gpio_to_dvp)/sizeof(gpio_to_dvp[0][0])/2; i++) {
		if (gpio_to_dvp[i][0] == new) {
			new_value = gpio_to_dvp[i][1];
			break;
		}
	}

	if (new_value < 0) {
		pr_err("%s donot find new:%d\n", __func__, new);
		return -EINVAL;
	}

	value = __raw_readl(old_vaddr);
	value &= ~(0xf << old_offset);	/*4bits for a map*/
	value |= new_value << old_offset;
	__raw_writel(value, old_vaddr);

	return 0;
}

static int set_pin_maps_cfg(struct ak_pinctrl *pc, int pin, unsigned long config)
{
	int new, old;
	int ret = 0;

	if (UNPACK_FLAG(config) != PIN_MAP_FLAG) {
		pr_err("%s is not pin map flag\n", __func__);
		return -EINVAL;
	}

	new = UNPACK_PIN_MAP_NEW(config);
	old = UNPACK_PIN_MAP_OLD(config);

	if (new != pin) {
		pr_err("%s is current pin, new:%d, pin:%d\n", __func__, new, pin);
		return -EINVAL;
	}

	pr_debug("%s new:%d, old:%d\n", __func__, new, old);

	if ((68 <= old && old <= 75) ||
			(6 <= old && old <= 9)) {
		ret = set_dvp_data_map(new, old);
	} else {
		pr_err("%s old:%d not support pin map\n", __func__, old);
		return -EINVAL;
	}

	return ret;
}

static int ak_pinconf_set(struct pinctrl_dev *pctldev,
			unsigned pin, unsigned long *configs,
			unsigned num_configs)
{
	int ret = 0;
	unsigned long flags;
	u8 pupd;
	struct ak_pinctrl *pc = pinctrl_dev_get_drvdata(pctldev);

	if ((num_configs == 0) || (num_configs > 2))
		return -EINVAL;

	/* for config value bit[7:0]--pupd */
	pupd = (configs[0] & 0xFF);

	local_irq_save(flags);
	pin_set_pull_enable(pc, pin, (pupd & 0x10) ? 1:0);
	pin_set_pull_polarity(pc, pin, (pupd & 0x01) ? 0:1);

	if (num_configs == 2) {
		ret = set_pin_maps_cfg(pc, pin, configs[1]);
		pr_debug("%s num_configs:%d, pin:%d\n", __func__, num_configs, pin);
	}
	local_irq_restore(flags);

	return ret;
}

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
	.npins = AK37D_NUM_GPIOS,
};

static int ak_pinctrl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct ak_pinctrl *pc;
	struct resource iomem;
	int ret, i;

	BUILD_BUG_ON(ARRAY_SIZE(ak_gpio_pins) != AK37D_NUM_GPIOS);

	pc = devm_kzalloc(dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;

	platform_set_drvdata(pdev, pc);
	pc->dev = dev;

	ret= of_address_to_resource(np, 0, &iomem);
	if (ret) {
		dev_err(dev, "could not get IO memory\n");
		return ret;
	}

	pc->gpio_base = devm_ioremap_resource(dev, &iomem);
	if (IS_ERR(pc->gpio_base))
		return PTR_ERR(pc->gpio_base);

	ret= of_address_to_resource(np, 1, &iomem);
	if (ret) {
		dev_err(dev, "could not get mipi0 memory\n");
		return ret;
	}

	pc->mipi0_base = devm_ioremap_resource(dev, &iomem);
	if (IS_ERR(pc->mipi0_base))
		return PTR_ERR(pc->mipi0_base);

	ret= of_address_to_resource(np, 2, &iomem);
	if (ret) {
		dev_err(dev, "could not get mipi1 memory\n");
		return ret;
	}
	pc->gc = ak_gpio_chip;
	pc->gc.dev = dev;
	pc->gc.of_node = np;

	pc->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(pc->clk)) {
		ret = PTR_ERR(pc->clk);
		return ret;
	}
	/*
	 * The clock is already enabled by the bootloader; calling
	 * clk_prepare_enable() here would reset the module and clear the
	 * pin state configured since u-boot.
	 */

	pc->irq = platform_get_irq(pdev, 0);
	if (pc->irq < 0)
		return pc->irq;

	pc->irq_domain = irq_domain_add_linear(np, AK37D_NUM_GPIOS,
			&irq_domain_simple_ops, pc);
	if (!pc->irq_domain) {
		dev_err(dev, "could not create IRQ domain\n");
		return -ENOMEM;
	}

	for (i = 0; i < AK37D_NUM_GPIOS; i++) {
		int irq = irq_create_mapping(pc->irq_domain, i);
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
		goto irq_err;
	}

	pc->pctl_dev = pinctrl_register(&ak_pinctrl_desc, dev, pc);
	if (IS_ERR(pc->pctl_dev)) {
		gpiochip_remove(&pc->gc);
		ret = PTR_ERR(pc->pctl_dev);
		goto irq_err;
	}

	pc->gpio_range = ak_pinctrl_gpio_range;
	pc->gpio_range.base = pc->gc.base;
	pc->gpio_range.gc = &pc->gc;
	pinctrl_add_gpio_range(pc->pctl_dev, &pc->gpio_range);

	return 0;

	/*
	 * No clk_disable_unprepare()/clk_put() on the way out: this driver
	 * never enabled the clock (U-Boot left it running, see above), and
	 * devm_clk_get() releases its reference itself.
	 */
irq_err:
	irq_set_chained_handler_and_data(pc->irq, NULL, NULL);
	for (i = 0; i < AK37D_NUM_GPIOS; i++)
		irq_dispose_mapping(irq_find_mapping(pc->irq_domain, i));
	irq_domain_remove(pc->irq_domain);

	return ret;
}

static int ak_pinctrl_remove(struct platform_device *pdev)
{
	struct ak_pinctrl *pc = platform_get_drvdata(pdev);

	pinctrl_unregister(pc->pctl_dev);
	gpiochip_remove(&pc->gc);

	return 0;
}

static const struct of_device_id ak_pinctrl_match[] = {
	{ .compatible = "anyka,ak3918-gpio" },
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

MODULE_DESCRIPTION("ak3918ev200 sharepin control driver");
MODULE_AUTHOR("anyka");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.00");
