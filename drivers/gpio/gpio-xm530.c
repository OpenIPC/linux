/*
 * Xiongmai XM530/XM550 pad GPIO.
 *
 * The pad controller at 0x10020000 has one 32-bit register per pad, which
 * is both its pin mux and its GPIO (mach/platform.h, and the vendor
 * kmotor.ko, which reads the input level back from bit 13):
 *
 *   bits 1-3   MUX1..MUX3; all clear is plain GPIO
 *   bits 7:6   drive strength
 *   bit 10     output enable
 *   bit 11     output level
 *   bit 12     input enable
 *   bit 13     input level
 *
 * Pad N is GPIO N. A lab XM550 populates pads 0..99; the words after them
 * are control registers. Vendor code writes whole words, so every update
 * here is read-modify-write and leaves the drive strength alone.
 *
 * Two kinds of pad are not ours to hand out:
 *  - one whose MUX bits are set belongs to another function (SD, Ethernet,
 *    I2C, the sensor bus). That is checked at request and again at every
 *    write, because sdio0_sd is a module that muxes its pads when it loads,
 *    possibly after a GPIO was requested;
 *  - one an in-tree driver drives as a plain pad through gpio_write(), with
 *    no MUX bit to show for it: SD card detect, power and write protect
 *    (49, 50, 51, sdio0_sd.c) and the Ethernet PHY reset (59,
 *    stmmac_main.c). Those are reserved outright.
 *
 * There is no GPIO interrupt.
 */

#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>

#define XM530_PAD_BASE		0x10020000
#define XM530_PADS		100

#define PAD_MUX			(7 << 1)
#define PAD_OUT_EN		(1 << 10)
#define PAD_OUT_HIGH		(1 << 11)
#define PAD_IN_EN		(1 << 12)
#define PAD_IN_LEVEL		(1 << 13)

/* Pads in-tree drivers drive directly, with no MUX bit set. */
static const u8 xm530_reserved_pads[] = { 49, 50, 51, 59 };

static void __iomem *xm530_pads;
static DEFINE_SPINLOCK(xm530_pad_lock);

/* -EBUSY, and nothing written, once the pad has been muxed to something
 * else -- even if it was a plain pad when the GPIO was requested. */
static int xm530_pad_update(unsigned offset, u32 clear, u32 set)
{
	unsigned long flags;
	int ret = 0;
	u32 v;

	spin_lock_irqsave(&xm530_pad_lock, flags);
	v = readl(xm530_pads + offset * 4);
	if (v & PAD_MUX)
		ret = -EBUSY;
	else
		writel((v & ~clear) | set, xm530_pads + offset * 4);
	spin_unlock_irqrestore(&xm530_pad_lock, flags);
	return ret;
}

static int xm530_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(xm530_reserved_pads); i++)
		if (offset == xm530_reserved_pads[i])
			return -EBUSY;
	if (readl(xm530_pads + offset * 4) & PAD_MUX)
		return -EBUSY;
	return 0;
}

static int xm530_gpio_direction_input(struct gpio_chip *chip,
				      unsigned offset)
{
	return xm530_pad_update(offset, PAD_OUT_EN, PAD_IN_EN);
}

static int xm530_gpio_direction_output(struct gpio_chip *chip,
				       unsigned offset, int value)
{
	return xm530_pad_update(offset, PAD_IN_EN | PAD_OUT_HIGH,
				PAD_OUT_EN | (value ? PAD_OUT_HIGH : 0));
}

static int xm530_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	return !!(readl(xm530_pads + offset * 4) & PAD_IN_LEVEL);
}

static void xm530_gpio_set(struct gpio_chip *chip, unsigned offset,
			   int value)
{
	xm530_pad_update(offset, PAD_OUT_HIGH, value ? PAD_OUT_HIGH : 0);
}

static struct gpio_chip xm530_gpio_chip = {
	.label			= "xm530-pads",
	.owner			= THIS_MODULE,
	.request		= xm530_gpio_request,
	.direction_input	= xm530_gpio_direction_input,
	.direction_output	= xm530_gpio_direction_output,
	.get			= xm530_gpio_get,
	.set			= xm530_gpio_set,
	.base			= 0,
	.ngpio			= XM530_PADS,
};

static int __init xm530_gpio_init(void)
{
	int ret;

	xm530_pads = ioremap(XM530_PAD_BASE, XM530_PADS * 4);
	if (!xm530_pads)
		return -ENOMEM;
	ret = gpiochip_add(&xm530_gpio_chip);
	if (ret)
		iounmap(xm530_pads);
	return ret;
}
subsys_initcall(xm530_gpio_init);
