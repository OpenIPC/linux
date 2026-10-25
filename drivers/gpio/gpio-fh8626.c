// SPDX-License-Identifier: GPL-2.0
/* Fullhan FH8626V100 GPIO banks. */
#include <linux/bitops.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#define FH_GPIO_DATA		0x00
#define FH_GPIO_DIRECTION	0x04
#define FH_GPIO_INT_ENABLE	0x30
#define FH_GPIO_INT_MASK		0x34
#define FH_GPIO_INT_TYPE		0x38
#define FH_GPIO_INT_POLARITY	0x3c
#define FH_GPIO_INT_STATUS	0x40
#define FH_GPIO_DEBOUNCE		0x48
#define FH_GPIO_INT_EOI		0x4c
#define FH_GPIO_INPUT		0x50
#define FH_GPIO_INT_BOTHEDGE	0x68

struct fh8626_gpio {
	struct gpio_chip gc;
	struct irq_chip irqchip;
	void __iomem *base;
	u32 both_edges;
	int parent_irq;
};

static void fh8626_gpio_toggle_edge(struct fh8626_gpio *gpio, u32 mask)
{
	u32 polarity = readl(gpio->base + FH_GPIO_INT_POLARITY);
	u32 input = readl(gpio->base + FH_GPIO_INPUT);

	polarity = (polarity & ~mask) | (~input & mask);
	writel(polarity, gpio->base + FH_GPIO_INT_POLARITY);
}

static void fh8626_gpio_irq_ack(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct fh8626_gpio *gpio = gpiochip_get_data(gc);
	unsigned long flags;
	u32 mask = BIT(irqd_to_hwirq(d));

	spin_lock_irqsave(&gc->bgpio_lock, flags);
	writel(mask, gpio->base + FH_GPIO_INT_EOI);
	if (gpio->both_edges & mask)
		fh8626_gpio_toggle_edge(gpio, mask);
	spin_unlock_irqrestore(&gc->bgpio_lock, flags);
}

static void fh8626_gpio_irq_mask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct fh8626_gpio *gpio = gpiochip_get_data(gc);
	unsigned long flags;
	u32 mask;

	spin_lock_irqsave(&gc->bgpio_lock, flags);
	mask = readl(gpio->base + FH_GPIO_INT_MASK);
	writel(mask | BIT(irqd_to_hwirq(d)), gpio->base + FH_GPIO_INT_MASK);
	spin_unlock_irqrestore(&gc->bgpio_lock, flags);
}

static void fh8626_gpio_irq_unmask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct fh8626_gpio *gpio = gpiochip_get_data(gc);
	unsigned long flags;
	u32 mask;

	spin_lock_irqsave(&gc->bgpio_lock, flags);
	mask = readl(gpio->base + FH_GPIO_INT_MASK);
	writel(mask & ~BIT(irqd_to_hwirq(d)), gpio->base + FH_GPIO_INT_MASK);
	spin_unlock_irqrestore(&gc->bgpio_lock, flags);
}

static int fh8626_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct fh8626_gpio *gpio = gpiochip_get_data(gc);
	unsigned long flags;
	u32 mask = BIT(irqd_to_hwirq(d));
	u32 level, polarity;

	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
	case IRQ_TYPE_EDGE_FALLING:
	case IRQ_TYPE_EDGE_BOTH:
	case IRQ_TYPE_LEVEL_HIGH:
	case IRQ_TYPE_LEVEL_LOW:
		break;
	default:
		return -EINVAL;
	}

	spin_lock_irqsave(&gc->bgpio_lock, flags);
	level = readl(gpio->base + FH_GPIO_INT_TYPE);
	polarity = readl(gpio->base + FH_GPIO_INT_POLARITY);
	if (type & IRQ_TYPE_EDGE_BOTH)
		level |= mask;
	else
		level &= ~mask;
	if (type == IRQ_TYPE_EDGE_RISING || type == IRQ_TYPE_LEVEL_HIGH)
		polarity |= mask;
	else
		polarity &= ~mask;

	gpio->both_edges &= ~mask;
	writel(level, gpio->base + FH_GPIO_INT_TYPE);
	writel(polarity, gpio->base + FH_GPIO_INT_POLARITY);
	if (type == IRQ_TYPE_EDGE_BOTH) {
		gpio->both_edges |= mask;
		fh8626_gpio_toggle_edge(gpio, mask);
	}
	spin_unlock_irqrestore(&gc->bgpio_lock, flags);

	irq_set_handler_locked(d, type & IRQ_TYPE_EDGE_BOTH ?
			       handle_edge_irq : handle_level_irq);
	return 0;
}

static void fh8626_gpio_irq_handler(struct irq_desc *desc)
{
	struct gpio_chip *gc = irq_desc_get_handler_data(desc);
	struct fh8626_gpio *gpio = gpiochip_get_data(gc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	unsigned long pending;
	unsigned int bit, irq;

	chained_irq_enter(chip, desc);
	pending = readl(gpio->base + FH_GPIO_INT_STATUS);
	for_each_set_bit(bit, &pending, gc->ngpio) {
		irq = irq_find_mapping(gc->irq.domain, bit);
		if (irq)
			generic_handle_irq(irq);
		else
			writel(BIT(bit), gpio->base + FH_GPIO_INT_EOI);
	}
	chained_irq_exit(chip, desc);
}

static void fh8626_gpio_irq_stop(void *data)
{
	struct fh8626_gpio *gpio = data;

	writel(~0U, gpio->base + FH_GPIO_INT_MASK);
	writel(0, gpio->base + FH_GPIO_INT_ENABLE);
	irq_set_chained_handler_and_data(gpio->parent_irq, NULL, NULL);
	synchronize_irq(gpio->parent_irq);
}

static int fh8626_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fh8626_gpio *gpio;
	struct resource *res;
	int ret;

	gpio = devm_kzalloc(dev, sizeof(*gpio), GFP_KERNEL);
	if (!gpio)
		return -ENOMEM;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gpio->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(gpio->base))
		return PTR_ERR(gpio->base);
	gpio->parent_irq = platform_get_irq(pdev, 0);
	if (gpio->parent_irq < 0)
		return gpio->parent_irq;

	ret = bgpio_init(&gpio->gc, dev, 4, gpio->base + FH_GPIO_INPUT,
			 gpio->base + FH_GPIO_DATA, NULL,
			 gpio->base + FH_GPIO_DIRECTION, NULL, 0);
	if (ret)
		return ret;
	gpio->gc.base = -1;
	gpio->gc.label = dev_name(dev);
	gpio->gc.parent = dev;
	gpio->gc.owner = THIS_MODULE;
	gpio->gc.of_node = dev->of_node;

	writel(0, gpio->base + FH_GPIO_INT_ENABLE);
	writel(~0U, gpio->base + FH_GPIO_INT_MASK);
	writel(0, gpio->base + FH_GPIO_INT_BOTHEDGE);
	writel(0, gpio->base + FH_GPIO_DEBOUNCE);
	writel(~0U, gpio->base + FH_GPIO_INT_EOI);

	ret = devm_gpiochip_add_data(dev, &gpio->gc, gpio);
	if (ret)
		return ret;
	gpio->irqchip.name = "fh8626-gpio";
	gpio->irqchip.irq_ack = fh8626_gpio_irq_ack;
	gpio->irqchip.irq_mask = fh8626_gpio_irq_mask;
	gpio->irqchip.irq_unmask = fh8626_gpio_irq_unmask;
	gpio->irqchip.irq_set_type = fh8626_gpio_irq_set_type;
	ret = gpiochip_irqchip_add(&gpio->gc, &gpio->irqchip, 0,
				   handle_bad_irq, IRQ_TYPE_NONE);
	if (ret)
		return ret;
	gpiochip_set_chained_irqchip(&gpio->gc, &gpio->irqchip,
				    gpio->parent_irq, fh8626_gpio_irq_handler);
	ret = devm_add_action_or_reset(dev, fh8626_gpio_irq_stop, gpio);
	if (ret)
		return ret;
	writel(~0U, gpio->base + FH_GPIO_INT_ENABLE);
	return 0;
}

static const struct of_device_id fh8626_gpio_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-gpio" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh8626_gpio_of_match);

static struct platform_driver fh8626_gpio_driver = {
	.probe = fh8626_gpio_probe,
	.driver = {
		.name = "fh8626-gpio",
		.of_match_table = fh8626_gpio_of_match,
	},
};
module_platform_driver(fh8626_gpio_driver);

MODULE_DESCRIPTION("Fullhan FH8626V100 GPIO controller");
MODULE_LICENSE("GPL");
