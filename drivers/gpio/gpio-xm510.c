/*
 * Xiongmai XM510 GPIO.
 *
 * The block at 0x10020000 is banked, eight pins a port, ports 0..8
 * (the vendor's sdio1_sd.c helpers):
 *
 *   +P*4                 mux word of port P: pin b is a GPIO while bits b,
 *                        8+b and 16+b (its three functions) are all clear
 *   +0x40 + P*0x10 + 0   data: writes set the output levels, reads give the
 *                        pins
 *   +0x40 + P*0x10 + 4   output enable, 1 = output
 *
 * Pin b of port P is GPIO P*8 + b. +0xD0, just past port 8, is the MAC's
 * speed select (stmmac_main.c), not a port. Vendor code writes whole words,
 * so every update here is read-modify-write under one lock.
 *
 * Two kinds of pin are not ours to hand out:
 *  - one muxed to a function (SD, Ethernet, the sensor bus): checked at
 *    request and again at every write. The mux words are never written here;
 *  - one an in-tree driver takes over by writing the block directly, with
 *    none of this driver's locking. Those are reserved outright:
 *    - all of port 8: stmmac_main.c resets the PHY on its pin 1 every time
 *      eth0 opens, writing the port's output enable and data as whole words;
 *    - all of port 5: sdio1_sd.c rewrites its mux word whole when the card
 *      slot powers up, and sdio2_wifi.c powers the Wi-Fi chip from pin 4 of
 *      it (its default value=0x54);
 *    - port 6 pins 0-3: sdio2_wifi.c muxes them to its slot when it loads.
 *    A board that moves the Wi-Fi power or SD detect pin elsewhere with
 *    those modules' value= parameter owns that pin itself.
 *
 * There is no GPIO interrupt.
 */

#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#define XM510_GPIO_BASE		0x10020000
#define XM510_GPIO_SIZE		0xD0
#define XM510_GPIO_PORTS	9

#define MUX(port)		((port) * 4)
#define DATA(port)		(0x40 + (port) * 0x10)
#define OUT_EN(port)		(0x44 + (port) * 0x10)

/* Pins in-tree drivers write directly, a bit per pin, per port. */
static const u8 xm510_reserved[XM510_GPIO_PORTS] = {
	[5] = 0xff,	/* sdio1_sd: the slot; sdio2_wifi: Wi-Fi power */
	[6] = 0x0f,	/* sdio2_wifi: the slot */
	[8] = 0xff,	/* stmmac_main: the PHY reset, written as whole words */
};

static void __iomem *xm510_gpio;
static DEFINE_SPINLOCK(xm510_gpio_lock);

static u32 pin_functions(unsigned bit)
{
	return (1 << bit) | (1 << (8 + bit)) | (1 << (16 + bit));
}

static bool pin_muxed(unsigned offset)
{
	return readl(xm510_gpio + MUX(offset / 8)) & pin_functions(offset % 8);
}

/* Set or clear the pin's bit in one of its port's words. -EBUSY, and
 * nothing written, once the pin has been muxed to something else -- even
 * if it was a GPIO when it was requested. */
static int xm510_gpio_update(unsigned offset, unsigned reg, int on)
{
	const u32 bit = 1 << (offset % 8);
	unsigned long flags;
	int ret = 0;
	u32 v;

	spin_lock_irqsave(&xm510_gpio_lock, flags);
	if (pin_muxed(offset)) {
		ret = -EBUSY;
	} else {
		v = readl(xm510_gpio + reg);
		writel(on ? v | bit : v & ~bit, xm510_gpio + reg);
	}
	spin_unlock_irqrestore(&xm510_gpio_lock, flags);
	return ret;
}

static int xm510_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	if ((xm510_reserved[offset / 8] & (1 << (offset % 8))) ||
	    pin_muxed(offset))
		return -EBUSY;
	return 0;
}

static int xm510_gpio_direction_input(struct gpio_chip *chip,
				      unsigned offset)
{
	return xm510_gpio_update(offset, OUT_EN(offset / 8), 0);
}

static int xm510_gpio_direction_output(struct gpio_chip *chip,
				       unsigned offset, int value)
{
	/* The level first, so the pin never drives the wrong one. */
	int ret = xm510_gpio_update(offset, DATA(offset / 8), value);

	return ret ? ret : xm510_gpio_update(offset, OUT_EN(offset / 8), 1);
}

static int xm510_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	return !!(readl(xm510_gpio + DATA(offset / 8)) & (1 << (offset % 8)));
}

static void xm510_gpio_set(struct gpio_chip *chip, unsigned offset,
			   int value)
{
	xm510_gpio_update(offset, DATA(offset / 8), value);
}

static struct gpio_chip xm510_gpio_chip = {
	.label			= "xm510-gpio",
	.owner			= THIS_MODULE,
	.request		= xm510_gpio_request,
	.direction_input	= xm510_gpio_direction_input,
	.direction_output	= xm510_gpio_direction_output,
	.get			= xm510_gpio_get,
	.set			= xm510_gpio_set,
	.base			= 0,
	.ngpio			= XM510_GPIO_PORTS * 8,
};

static int __init xm510_gpio_init(void)
{
	int ret;

	/* Mapped, not claimed: sdio1_sd and stmmac_main request their own
	 * words of this block, and would fail if it were ours. */
	xm510_gpio = ioremap(XM510_GPIO_BASE, XM510_GPIO_SIZE);
	if (!xm510_gpio)
		return -ENOMEM;
	ret = gpiochip_add(&xm510_gpio_chip);
	if (ret)
		iounmap(xm510_gpio);
	return ret;
}
subsys_initcall(xm510_gpio_init);
