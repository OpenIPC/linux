// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Axera Inc.
 */

#include <linux/bitops.h>
#include <linux/gpio/driver.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/property.h>
#include <linux/reset.h>
#include <linux/clk.h>

#define GPIO_SWPORTA_DR			BIT(0)
#define GPIO_SWPORTA_DDR		BIT(1)
#define GPIO_SOFT_HAR_MODE		BIT(2)
#define GPIO_INTEN			BIT(3)
#define GPIO_INTMASK			BIT(4)
#define GPIO_INTTYPE_LEVEL		BIT(5)
#define GPIO_INT_POLARITY		BIT(6)
#define GPIO_PORTA_DEBOUNCE		BIT(7)
#define GPIO_PORTA_EOI			BIT(8)
#define GPIO_INT_BOTHEDGE		BIT(9)

#define GPIO_PORTA0_FUNC		0x4
#define GPIO_INTSTATUS_SECURE		0x84
#define GPIO_RAW_INTSTATUS_SECURE	0x88
#define GPIO_EXT_PORTA			0x8c
#define GPIO_ID_CODE			0x90
#define GPIO_LS_SYNC			0x94
#define GPIO_VER_ID_CODE		0x98
#define GPIO_CONFIG_REG2		0x9c
#define GPIO_CONFIG_REG1		0xa0
#define GPIO_INTSTATUS_NSECURE		0xa4
#define GPIO_RAW_INTSTATUS_NSECURE	0xa8

#define GPIO_NSECURE_MODE_VALUE		0x0
/* We have 1 banks GPIOs and each bank contain 32 GPIOs */
#define AX_GPIO_BANK_NR	1
#define AX_GPIO_BANK_MASK	GENMASK(31, 0)

#define PERI_SYS_BASE			0x4870000
#define PERI_SYS_BASE_LEN		0x100
#define CLK_SOURCE			24000000	/*24M*/
#define CLK_MUX_0_SET			0xA8	/*clk source set,bit2,0 32k,1,24m*/
#define CLK_MUX_0_CLR			0xAC
#define CLK_SOURCE_BIT			BIT(2)
#define CLK_EB_1_SET			0xB8	/*clk set,bit4 - 7*/
#define CLK_EB_1_CLR			0xBC
#define CLK_BIT(x)			BIT(4 + x)
#define CLK_EB_2_SET			0xC0	/*pclk set,bit13 - 16*/
#define CLK_EB_2_CLR			0xC4
#define PCLK_BIT(x)			BIT(13 + x)

void __iomem *gpio_clk_reg;
static int source_set_flag;

struct ax_gpio {
	struct gpio_chip chip;
	struct device *dev;
	struct irq_chip init_irq_chip;
	void __iomem *base;
	spinlock_t lock;
	struct reset_control *prst;
	struct reset_control *rst;
	int irq;
	int gpio_clk_id;
};

static int gpio_base;


static inline void __iomem *ax_gpio_bank_base(struct ax_gpio *ax_gpio,
						unsigned int offset)
{
	void __iomem * reg = ((ax_gpio->base) + (offset + 1) * GPIO_PORTA0_FUNC);
	return reg;
}

static void ax_gpio_write(struct gpio_chip *chip, unsigned int offset, int val)
{
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	void __iomem *base = ax_gpio_bank_base(ax_gpio, offset);
	writel_relaxed(val, base);
}

static int ax_gpio_read(struct gpio_chip *chip, unsigned int offset)
{
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	void __iomem *base = ax_gpio_bank_base(ax_gpio, offset);
	return readl_relaxed(base);
}

static int ax_gpio_request(struct gpio_chip *chip, unsigned int offset)
{
	return 0;
}

static void ax_gpio_free(struct gpio_chip *chip, unsigned int offset)
{
}

static int ax_gpio_direction_input(struct gpio_chip *chip,
				     unsigned int offset)
{
	unsigned int val;
	unsigned long flags;
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	spin_lock_irqsave(&ax_gpio->lock, flags);
	val = ax_gpio_read(chip, offset);
	val &= (~(GPIO_SWPORTA_DDR));
	ax_gpio_write(chip, offset, val);
	spin_unlock_irqrestore(&ax_gpio->lock, flags);
	return 0;
}

static int ax_gpio_direction_output(struct gpio_chip *chip,
				      unsigned int offset, int value)
{
	unsigned int val;
	unsigned long flags;
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	spin_lock_irqsave(&ax_gpio->lock, flags);
	val = ax_gpio_read(chip, offset);
	val |= (GPIO_SWPORTA_DDR);
	ax_gpio_write(chip, offset, val);
	if(value == 0){
		val = ax_gpio_read(chip, offset);
		val &= (~(GPIO_SWPORTA_DR));
		ax_gpio_write(chip, offset, val);
		spin_unlock_irqrestore(&ax_gpio->lock, flags);
		return 0;
	} else {
		val = ax_gpio_read(chip, offset);
		val |= (GPIO_SWPORTA_DR);
		ax_gpio_write(chip, offset, val);
		spin_unlock_irqrestore(&ax_gpio->lock, flags);
		return 0;
	}
	spin_unlock_irqrestore(&ax_gpio->lock, flags);
	return 0;
}

static int ax_gpio_get(struct gpio_chip *chip, unsigned int offset)
{
	unsigned int val;
	unsigned long flags;
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	void __iomem *base = ax_gpio->base;
	spin_lock_irqsave(&ax_gpio->lock, flags);
	val = ax_gpio_read(chip, offset);
	if (val & GPIO_SWPORTA_DDR){
		spin_unlock_irqrestore(&ax_gpio->lock, flags);
		return (val & GPIO_SWPORTA_DR);
	}
	val = readl_relaxed(base + GPIO_EXT_PORTA);
	val = (val >> offset);
	val &= 1;
	spin_unlock_irqrestore(&ax_gpio->lock, flags);
	return val;
}

static void ax_gpio_set(struct gpio_chip *chip, unsigned int offset,
			  int value)
{
	unsigned int val;
	unsigned long flags;
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	spin_lock_irqsave(&ax_gpio->lock, flags);
	if(value == 0){
		val = ax_gpio_read(chip, offset);
		val &= (~GPIO_SWPORTA_DR);
		val |= GPIO_SWPORTA_DDR;
		ax_gpio_write(chip, offset, val);
		spin_unlock_irqrestore(&ax_gpio->lock, flags);
		return;
	} else {
		val = ax_gpio_read(chip, offset);
		val |= GPIO_SWPORTA_DR;
		val |= GPIO_SWPORTA_DDR;
		ax_gpio_write(chip, offset, val);
		spin_unlock_irqrestore(&ax_gpio->lock, flags);
		return;
	}
}

static void ax_gpio_irq_mask(struct irq_data *data)
{
	unsigned long flags;
	struct gpio_chip *chip = irq_data_get_irq_chip_data(data);
	u32 offset = irqd_to_hwirq(data);
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	unsigned int val;
	spin_lock_irqsave(&ax_gpio->lock, flags);
	val = ax_gpio_read(chip, offset);
	val |= GPIO_INTMASK;
	ax_gpio_write(chip, offset, val);
	spin_unlock_irqrestore(&ax_gpio->lock, flags);
}

static void ax_gpio_irq_ack(struct irq_data *data)
{
	unsigned long flags;
	struct gpio_chip *chip = irq_data_get_irq_chip_data(data);
	u32 offset = irqd_to_hwirq(data);
	unsigned int val;
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	spin_lock_irqsave(&ax_gpio->lock, flags);
	val = ax_gpio_read(chip, offset);
	val |= GPIO_PORTA_EOI;
	ax_gpio_write(chip, offset, val);

	val = ax_gpio_read(chip, offset);
	val &= (~GPIO_PORTA_EOI);
	ax_gpio_write(chip, offset, val);
	spin_unlock_irqrestore(&ax_gpio->lock, flags);

}
static void ax_gpio_irq_unmask(struct irq_data *data)
{
	struct gpio_chip *chip = irq_data_get_irq_chip_data(data);
	u32 offset = irqd_to_hwirq(data);
	unsigned int val;
	val = ax_gpio_read(chip, offset);
	val &= (~GPIO_INTMASK);
	ax_gpio_write(chip, offset, val);
}

static int ax_gpio_irq_set_type(struct irq_data *data,
				  unsigned int flow_type)
{
	unsigned long flags;
	struct gpio_chip *chip = irq_data_get_irq_chip_data(data);
	u32 offset = irqd_to_hwirq(data);
	unsigned int val;
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	spin_lock_irqsave(&ax_gpio->lock, flags);
	switch (flow_type) {
	case IRQ_TYPE_EDGE_RISING:
		val = ax_gpio_read(chip, offset);
		val |= GPIO_INTTYPE_LEVEL;//1 is edge, 0 is vol;
		ax_gpio_write(chip, offset, val);
		val = ax_gpio_read(chip, offset);
		val &= (~GPIO_INT_BOTHEDGE);//0 is oneedge, 1 is bothedge;
		ax_gpio_write(chip, offset, val);
		val = ax_gpio_read(chip, offset);
		val |= GPIO_INT_POLARITY;//1 is high/rising, 0 is low/falling;
		ax_gpio_write(chip, offset, val);
		irq_set_handler_locked(data, handle_edge_irq);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		val = ax_gpio_read(chip, offset);
		val |= GPIO_INTTYPE_LEVEL;//1 is edge, 0 is vol;
		ax_gpio_write(chip, offset, val);
		val = ax_gpio_read(chip, offset);
		val &= (~GPIO_INT_BOTHEDGE);//0 is oneedge, 1 is bothedge;
		ax_gpio_write(chip, offset, val);
		val = ax_gpio_read(chip, offset);
		val &= (~GPIO_INT_POLARITY);//1 is high/rising, 0 is low/falling;
		ax_gpio_write(chip, offset, val);
		irq_set_handler_locked(data, handle_edge_irq);
		break;
	case IRQ_TYPE_EDGE_BOTH:
		val = ax_gpio_read(chip, offset);
		val |= GPIO_INTTYPE_LEVEL;//1 is edge, 0 is vol;
		ax_gpio_write(chip, offset, val);
		val = ax_gpio_read(chip, offset);
		val |= GPIO_INT_BOTHEDGE;//0 is oneedge, 1 is bothedge;
		ax_gpio_write(chip, offset, val);
		irq_set_handler_locked(data, handle_edge_irq);
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		val = ax_gpio_read(chip, offset);
		val &= (~GPIO_INTTYPE_LEVEL);//1 is edge, 0 is vol;
		ax_gpio_write(chip, offset, val);
		val = ax_gpio_read(chip, offset);
		val |= GPIO_INT_POLARITY;//1 is high/rising, 0 is low/falling;
		ax_gpio_write(chip, offset, val);
		irq_set_handler_locked(data, handle_level_irq);
		break;
	case IRQ_TYPE_LEVEL_LOW:
		val = ax_gpio_read(chip, offset);
		val &= (~GPIO_INTTYPE_LEVEL);//1 is edge, 0 is vol;
		ax_gpio_write(chip, offset, val);
		val = ax_gpio_read(chip, offset);
		val &= (~GPIO_INT_POLARITY);//1 is high/rising, 0 is low/falling;
		ax_gpio_write(chip, offset, val);
		irq_set_handler_locked(data, handle_level_irq);
		break;
	default:
		spin_unlock_irqrestore(&ax_gpio->lock, flags);
		return -EINVAL;
	}
	spin_unlock_irqrestore(&ax_gpio->lock, flags);
	return 0;
}
static void ax_irq_enable(struct irq_data *data)
{
	unsigned long flags;
	struct gpio_chip *chip = irq_data_get_irq_chip_data(data);
	u32 offset = irqd_to_hwirq(data);
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	unsigned int val;
	spin_lock_irqsave(&ax_gpio->lock, flags);
	val = ax_gpio_read(chip, offset);
	val |= GPIO_INTEN;
	val &= (~GPIO_INTMASK);
	ax_gpio_write(chip, offset, val);
	spin_unlock_irqrestore(&ax_gpio->lock, flags);
}

static void ax_irq_disable(struct irq_data *data)
{
	unsigned long flags;
	struct gpio_chip *chip = irq_data_get_irq_chip_data(data);
	u32 offset = irqd_to_hwirq(data);
	unsigned int val;
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	spin_lock_irqsave(&ax_gpio->lock, flags);
	val = ax_gpio_read(chip, offset);
	val |= GPIO_INTMASK;
	val &= (~GPIO_INTEN);
	ax_gpio_write(chip, offset, val);
	spin_unlock_irqrestore(&ax_gpio->lock, flags);

}
static void ax_gpio_irq_handler(struct irq_desc *desc)
{
	unsigned long flags;
	struct gpio_chip *chip = irq_desc_get_handler_data(desc);
	struct irq_chip *ic = irq_desc_get_chip(desc);
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	unsigned long reg;
	u32 n;
	void __iomem *base = ax_gpio->base;
	spin_lock_irqsave(&ax_gpio->lock, flags);
	reg = readl_relaxed(base + GPIO_INTSTATUS_NSECURE) & AX_GPIO_BANK_MASK;
	spin_unlock_irqrestore(&ax_gpio->lock, flags);
	chained_irq_enter(ic, desc);

	for_each_set_bit(n, &reg, chip->ngpio)
		generic_handle_irq(irq_find_mapping(chip->irq.domain, n));
	chained_irq_exit(ic, desc);


}

static void ax_disable_debounce(struct gpio_chip *chip, unsigned int offset)
{
	unsigned int val;

	val = ax_gpio_read(chip, offset);
	val &= ~(GPIO_PORTA_DEBOUNCE);
	ax_gpio_write(chip, offset, val);
}

static void ax_enable_debounce(struct gpio_chip *chip, unsigned int offset, int arg)
{
	unsigned int val;
	if(arg){
		val = ax_gpio_read(chip, offset);
		val |= GPIO_PORTA_DEBOUNCE;
		ax_gpio_write(chip, offset, val);
	}
	else
		ax_disable_debounce(chip, offset);
}

static int ax_gpio_set_config(struct gpio_chip *chip, unsigned int offset,
				  unsigned long config)
{
	unsigned long flags;
	unsigned long param = pinconf_to_config_param(config);
	u32 arg = pinconf_to_config_argument(config);
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	spin_lock_irqsave(&ax_gpio->lock, flags);
	if (param == PIN_CONFIG_INPUT_DEBOUNCE)
		ax_enable_debounce(chip, offset, arg);
	spin_unlock_irqrestore(&ax_gpio->lock, flags);
	return 0;
}

static int ax_gpio_get_direction(struct gpio_chip *chip, unsigned int offset)
{
	unsigned int val;
	unsigned long flags;
	struct ax_gpio *ax_gpio = gpiochip_get_data(chip);
	spin_lock_irqsave(&ax_gpio->lock, flags);
	val = ax_gpio_read(chip, offset);
	val &= (GPIO_SWPORTA_DDR);
	spin_unlock_irqrestore(&ax_gpio->lock, flags);
	return val ? 0:1;
}

static void __attribute__((unused)) ax_assert_reset(void *data)
{
	struct ax_gpio *gpio = data;

	reset_control_assert(gpio->prst);
	reset_control_assert(gpio->rst);
}

static void ax_deassert_reset(void *data)
{
	struct ax_gpio *gpio = data;

	reset_control_deassert(gpio->prst);
	reset_control_deassert(gpio->rst);
}

static void ax_gpio_clk(int ax_clk_id, bool on)
{
	if (on) {
		writel(PCLK_BIT(ax_clk_id), gpio_clk_reg + CLK_EB_2_SET);
		writel(CLK_BIT(ax_clk_id), gpio_clk_reg + CLK_EB_1_SET);
	} else {
		writel(PCLK_BIT(ax_clk_id), gpio_clk_reg + CLK_EB_2_CLR);
		writel(CLK_BIT(ax_clk_id), gpio_clk_reg + CLK_EB_1_CLR);
	}
}

static int ax_gpio_probe(struct platform_device *pdev)
{
	struct gpio_irq_chip *irq;
	struct ax_gpio *ax_gpio;
	int gpio_nums, clk_id;
	struct resource *res;

	ax_gpio = devm_kzalloc(&pdev->dev, sizeof(*ax_gpio), GFP_KERNEL);
	if (!ax_gpio)
		return -ENOMEM;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ax_gpio->irq = platform_get_irq(pdev, 0);
	if (ax_gpio->irq < 0)
		return ax_gpio->irq;

	ax_gpio->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ax_gpio->base))
		return PTR_ERR(ax_gpio->base);
	writel_relaxed(GPIO_NSECURE_MODE_VALUE, ax_gpio->base);
	spin_lock_init(&ax_gpio->lock);
	of_property_read_u32_index(pdev->dev.of_node, "ax,ngpios", 0, &gpio_nums);
	ax_gpio->dev = &pdev->dev;
	ax_gpio->chip.label = dev_name(&pdev->dev);
	ax_gpio->chip.ngpio = gpio_nums;
	ax_gpio->chip.base = gpio_base;
	gpio_base += ax_gpio->chip.ngpio;
	ax_gpio->chip.parent = &pdev->dev;
	ax_gpio->chip.of_node = pdev->dev.of_node;
	ax_gpio->chip.request = ax_gpio_request;
	ax_gpio->chip.free = ax_gpio_free;
	ax_gpio->chip.get = ax_gpio_get;
	ax_gpio->chip.set = ax_gpio_set;
	ax_gpio->chip.get_direction = ax_gpio_get_direction;
	ax_gpio->chip.direction_input = ax_gpio_direction_input;
	ax_gpio->chip.direction_output = ax_gpio_direction_output;
	ax_gpio->chip.set_config = ax_gpio_set_config;

	ax_gpio->init_irq_chip.name = "ax-gpio";
	ax_gpio->init_irq_chip.irq_ack = ax_gpio_irq_ack;
	ax_gpio->init_irq_chip.irq_mask = ax_gpio_irq_mask;
	ax_gpio->init_irq_chip.irq_unmask = ax_gpio_irq_unmask;
	ax_gpio->init_irq_chip.irq_set_type = ax_gpio_irq_set_type;
	ax_gpio->init_irq_chip.irq_enable = ax_irq_enable;
	ax_gpio->init_irq_chip.irq_disable = ax_irq_disable;
	ax_gpio->init_irq_chip.flags = IRQCHIP_SKIP_SET_WAKE;

	irq = &ax_gpio->chip.irq;
	irq->chip = &ax_gpio->init_irq_chip;
	irq->handler = handle_bad_irq;
	irq->default_type = IRQ_TYPE_NONE;
	irq->parent_handler = ax_gpio_irq_handler;
	irq->parent_handler_data = ax_gpio;
	irq->parents = devm_kcalloc(&pdev->dev, 1, sizeof(*irq->parents), GFP_KERNEL);
	if (!irq->parents)
		return -ENOMEM;
	irq->num_parents = 1;
	irq->parents = &ax_gpio->irq;
	/* Optional interface clock */
	device_property_read_u32(&pdev->dev, "ax_clk_id", &clk_id);
	ax_gpio->gpio_clk_id = clk_id;
	if (source_set_flag == 0) {
		gpio_clk_reg = ioremap(PERI_SYS_BASE, PERI_SYS_BASE_LEN);
		writel(CLK_SOURCE_BIT, gpio_clk_reg + CLK_MUX_0_SET);
		source_set_flag = 1;
	}

	ax_gpio->prst =  devm_reset_control_get_optional(ax_gpio->dev, "gpio_prst");
	if (IS_ERR(ax_gpio->prst)) {
		dev_err(ax_gpio->dev, "Cannot get preset descriptor\n");
		return -ENODEV;
	}
	ax_gpio->rst =  devm_reset_control_get_optional(ax_gpio->dev, "gpio_rst");
	if (IS_ERR(ax_gpio->rst)) {
		dev_err(ax_gpio->dev, "Cannot get reset descriptor\n");
		return -ENODEV;
	}
	ax_deassert_reset(ax_gpio);
	ax_gpio_clk(clk_id, true);
	pr_debug("%s enabled\n",__func__);
	dev_set_drvdata(&pdev->dev, ax_gpio);
	return devm_gpiochip_add_data(&pdev->dev, &ax_gpio->chip, ax_gpio);
}

#ifdef CONFIG_PM_SLEEP
static int ax_gpio_suspend(struct device *dev)
{
	return 0;
}

static int ax_gpio_resume(struct device *dev)
{
	return 0;
}
#endif
static SIMPLE_DEV_PM_OPS(ax_gpio_pm_ops, ax_gpio_suspend, ax_gpio_resume);
static const struct of_device_id ax_gpio_of_match[] = {
	{ .compatible = "axera,ax-apb-gpio", },
	{ /* end of list */ }
};
MODULE_DEVICE_TABLE(of, ax_gpio_of_match);

static struct platform_driver ax_gpio_driver = {
	.probe = ax_gpio_probe,
	.driver = {
		.name = "ax-gpio",
		.pm = &ax_gpio_pm_ops,
		.of_match_table	= ax_gpio_of_match,
	},
};

module_platform_driver_probe(ax_gpio_driver, ax_gpio_probe);

MODULE_DESCRIPTION("Axera GPIO driver");
MODULE_LICENSE("GPL v2");
