/*
 * drivers/pinctrl/ambarella/pinctrl-amb.c
 *
 * History:
 *	2013/12/18 - [Cao Rongrong] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/syscore_ops.h>
#include <linux/irqdomain.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqchip/chained_irq.h>
#include <plat/iav_helper.h>
#include <plat/pinctrl.h>

#if defined(CONFIG_PM)
struct amb_gpio_regs {
	u32 irq_wake_mask;
	u32 data;
	u32 dir;
	u32 is;
	u32 ibe;
	u32 iev;
	u32 ie;
	u32 afsel;
	u32 mask;
};

struct amb_gpio_regs amb_gpio_pm[GPIO_INSTANCES];
#endif

struct amb_gpio_chip {
	int				irq[GPIO_INSTANCES];
	void __iomem			*regbase[GPIO_INSTANCES];
	void __iomem			*iomux_base;
	struct gpio_chip		*gc;
	struct irq_domain		*domain;
	struct ambarella_service	gpio_service;
};

static int ambarella_gpio_service(void *arg, void *result);

/* gpiolib gpio_request callback function */
static int amb_gpio_request(struct gpio_chip *gc, unsigned pin)
{
	return pinctrl_request_gpio(gc->base + pin);
}

/* gpiolib gpio_set callback function */
static void amb_gpio_free(struct gpio_chip *gc, unsigned pin)
{
	pinctrl_free_gpio(gc->base + pin);
}

/* gpiolib gpio_free callback function */
static void amb_gpio_set(struct gpio_chip *gc, unsigned pin, int value)
{
	struct amb_gpio_chip *amb_gpio = dev_get_drvdata(gc->dev);
	void __iomem *regbase;
	u32 bank, offset, mask;

	bank = PINID_TO_BANK(pin);
	offset = PINID_TO_OFFSET(pin);
	regbase = amb_gpio->regbase[bank];
	mask = (0x1 << offset);

	amba_writel(regbase + GPIO_MASK_OFFSET, mask);
	if (value == GPIO_LOW)
		mask = 0;
	amba_writel(regbase + GPIO_DATA_OFFSET, mask);
}

/* gpiolib gpio_get callback function */
static int amb_gpio_get(struct gpio_chip *gc, unsigned pin)
{
	struct amb_gpio_chip *amb_gpio = dev_get_drvdata(gc->dev);
	void __iomem *regbase;
	u32 bank, offset, mask, data;

	bank = PINID_TO_BANK(pin);
	offset = PINID_TO_OFFSET(pin);
	regbase = amb_gpio->regbase[bank];
	mask = (0x1 << offset);

	amba_writel(regbase + GPIO_MASK_OFFSET, mask);
	data = amba_readl(regbase + GPIO_DATA_OFFSET);
	data = (data >> offset) & 0x1;

	return (data ? GPIO_HIGH : GPIO_LOW);
}

/* gpiolib gpio_direction_input callback function */
static int amb_gpio_direction_input(struct gpio_chip *gc, unsigned pin)
{
	return pinctrl_gpio_direction_input(gc->base + pin);
}

/* gpiolib gpio_direction_output callback function */
static int amb_gpio_direction_output(struct gpio_chip *gc,
		unsigned pin, int value)
{
	int rval;

	rval = pinctrl_gpio_direction_output(gc->base + pin);
	if (rval < 0)
		return rval;

	amb_gpio_set(gc, pin, value);

	return 0;
}

/* gpiolib gpio_to_irq callback function */
static int amb_gpio_to_irq(struct gpio_chip *gc, unsigned pin)
{
	struct amb_gpio_chip *amb_gpio = dev_get_drvdata(gc->dev);

	return irq_create_mapping(amb_gpio->domain, pin);
}

static void amb_gpio_dbg_show(struct seq_file *s, struct gpio_chip *gc)
{
	struct amb_gpio_chip *amb_gpio = dev_get_drvdata(gc->dev);
	void __iomem *iomux_base = amb_gpio->iomux_base;
	void __iomem *regbase;
	u32 afsel = 0, data = 0, dir = 0, mask = 0;
	u32 iomux0 = 0, iomux1 = 0, iomux2 = 0, alt = 0;
	u32 i, bank, offset;

	for (i = 0; i < gc->ngpio; i++) {
		offset = PINID_TO_OFFSET(i);
		if (offset == 0) {
			bank = PINID_TO_BANK(i);
			regbase = amb_gpio->regbase[bank];

			afsel = amba_readl(regbase + GPIO_AFSEL_OFFSET);
			dir = amba_readl(regbase + GPIO_DIR_OFFSET);
			mask = amba_readl(regbase + GPIO_MASK_OFFSET);
			amba_writel(regbase + GPIO_MASK_OFFSET, ~afsel);
			data = amba_readl(regbase + GPIO_DATA_OFFSET);
			amba_writel(regbase + GPIO_MASK_OFFSET, mask);

			seq_printf(s, "\nGPIO[%d]:\t[%d - %d]\n",
				bank, i, i + GPIO_BANK_SIZE - 1);
			seq_printf(s, "GPIO_BASE:\t0x%08X\n", (u32)regbase);
			seq_printf(s, "GPIO_AFSEL:\t0x%08X\n", afsel);
			seq_printf(s, "GPIO_DIR:\t0x%08X\n", dir);
			seq_printf(s, "GPIO_MASK:\t0x%08X:0x%08X\n", mask, ~afsel);
			seq_printf(s, "GPIO_DATA:\t0x%08X\n", data);

			if (iomux_base != NULL) {
				iomux0 = amba_readl(iomux_base + bank * 12);
				iomux1 = amba_readl(iomux_base + bank * 12 + 4);
				iomux2 = amba_readl(iomux_base + bank * 12 + 8);
				seq_printf(s, "IOMUX_REG%d_0:\t0x%08X\n", bank, iomux0);
				seq_printf(s, "IOMUX_REG%d_1:\t0x%08X\n", bank, iomux1);
				seq_printf(s, "IOMUX_REG%d_2:\t0x%08X\n", bank, iomux2);
			}
		}

		seq_printf(s, " gpio-%-3d", gc->base + i);
		if (iomux_base != NULL) {
			alt = ((iomux2 >> offset) & 1) << 2;
			alt |= ((iomux1 >> offset) & 1) << 1;
			alt |= ((iomux0 >> offset) & 1) << 0;
			if (alt != 0)
				seq_printf(s, " [HW  ] (alt%d)\n", alt);
			else {
				const char *label = gpiochip_is_requested(gc, i);
				label = label ? : "";
				seq_printf(s, " [GPIO] (%-20.20s) %s %s\n", label,
					(dir & (1 << offset)) ? "out" : "in ",
					(data & (1 << offset)) ? "hi" : "lo");
			}
		} else {
			if (afsel & (1 << offset)) {
				seq_printf(s, " [HW  ]\n");
			} else {
				const char *label = gpiochip_is_requested(gc, i);
				label = label ? : "";
				seq_printf(s, " [GPIO] (%-20.20s) %s %s\n", label,
					(dir & (1 << offset)) ? "out" : "in ",
					(data & (1 << offset)) ? "hi" : "lo");
			}
		}
	}
}

static struct gpio_chip amb_gc = {
	.label			= "ambarella-gpio",
	.base			= 0,
	.ngpio			= AMBGPIO_SIZE,
	.request		= amb_gpio_request,
	.free			= amb_gpio_free,
	.direction_input	= amb_gpio_direction_input,
	.direction_output	= amb_gpio_direction_output,
	.get			= amb_gpio_get,
	.set			= amb_gpio_set,
	.to_irq			= amb_gpio_to_irq,
	.dbg_show		= amb_gpio_dbg_show,
	.owner			= THIS_MODULE,
};

static void amb_gpio_irq_enable(struct irq_data *data)
{
	struct amb_gpio_chip *amb_gpio = dev_get_drvdata(amb_gc.dev);
	void __iomem *regbase = irq_data_get_irq_chip_data(data);
	void __iomem *iomux_base = amb_gpio->iomux_base;
	u32 i, bank, offset, val;

	bank = PINID_TO_BANK(data->hwirq);
	offset = PINID_TO_OFFSET(data->hwirq);

	/* make sure the pin is in gpio input mode */
	if (!gpiochip_is_requested(&amb_gc, data->hwirq)) {
		amba_clrbitsl(regbase + GPIO_AFSEL_OFFSET, 0x1 << offset);
		amba_clrbitsl(regbase + GPIO_DIR_OFFSET, 0x1 << offset);

		if (iomux_base) {
			for (i = 0; i < 3; i++) {
				val = amba_readl(iomux_base + IOMUX_REG_OFFSET(bank, i));
				val &= (~(0x1 << offset));
				amba_writel(iomux_base + IOMUX_REG_OFFSET(bank, i), val);
			}
			amba_writel(iomux_base + IOMUX_CTRL_SET_OFFSET, 0x1);
			amba_writel(iomux_base + IOMUX_CTRL_SET_OFFSET, 0x0);
		}
	}

	amba_writel(regbase + GPIO_IC_OFFSET, 0x1 << offset);
	amba_setbitsl(regbase + GPIO_IE_OFFSET, 0x1 << offset);
}

static void amb_gpio_irq_disable(struct irq_data *data)
{
	void __iomem *regbase = irq_data_get_irq_chip_data(data);
	u32 offset = PINID_TO_OFFSET(data->hwirq);

	amba_clrbitsl(regbase + GPIO_IE_OFFSET, 0x1 << offset);
	amba_writel(regbase + GPIO_IC_OFFSET, 0x1 << offset);
}

static void amb_gpio_irq_ack(struct irq_data *data)
{
	void __iomem *regbase = irq_data_get_irq_chip_data(data);
	u32 offset = PINID_TO_OFFSET(data->hwirq);

	amba_writel(regbase + GPIO_IC_OFFSET, 0x1 << offset);
}

static void amb_gpio_irq_mask(struct irq_data *data)
{
	void __iomem *regbase = irq_data_get_irq_chip_data(data);
	u32 offset = PINID_TO_OFFSET(data->hwirq);

	amba_clrbitsl(regbase + GPIO_IE_OFFSET, 0x1 << offset);
}

static void amb_gpio_irq_mask_ack(struct irq_data *data)
{
	void __iomem *regbase = irq_data_get_irq_chip_data(data);
	u32 offset = PINID_TO_OFFSET(data->hwirq);

	amba_clrbitsl(regbase + GPIO_IE_OFFSET, 0x1 << offset);
	amba_writel(regbase + GPIO_IC_OFFSET, 0x1 << offset);
}

static void amb_gpio_irq_unmask(struct irq_data *data)
{
	void __iomem *regbase = irq_data_get_irq_chip_data(data);
	u32 offset = PINID_TO_OFFSET(data->hwirq);

	amba_setbitsl(regbase + GPIO_IE_OFFSET, 0x1 << offset);
}

static int amb_gpio_irq_set_type(struct irq_data *data, unsigned int type)
{
	void __iomem *regbase = irq_data_get_irq_chip_data(data);
	struct irq_desc *desc = irq_to_desc(data->irq);
	u32 offset = PINID_TO_OFFSET(data->hwirq);
	u32 mask, bit, sense, bothedges, event;

	mask = ~(0x1 << offset);
	bit = (0x1 << offset);
	sense = amba_readl(regbase + GPIO_IS_OFFSET);
	bothedges = amba_readl(regbase + GPIO_IBE_OFFSET);
	event = amba_readl(regbase + GPIO_IEV_OFFSET);

	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		sense &= mask;
		bothedges &= mask;
		event |= bit;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		sense &= mask;
		bothedges &= mask;
		event &= mask;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		sense &= mask;
		bothedges |= bit;
		event &= mask;
		desc->handle_irq = handle_edge_irq;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		sense |= bit;
		bothedges &= mask;
		event |= bit;
		desc->handle_irq = handle_level_irq;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		sense |= bit;
		bothedges &= mask;
		event &= mask;
		desc->handle_irq = handle_level_irq;
		break;
	default:
		pr_err("%s: irq[%d] type[%d] fail!\n",
			__func__, data->irq, type);
		return -EINVAL;
	}

	amba_writel(regbase + GPIO_IS_OFFSET, sense);
	amba_writel(regbase + GPIO_IBE_OFFSET, bothedges);
	amba_writel(regbase + GPIO_IEV_OFFSET, event);
	/* clear obsolete irq */
	amba_writel(regbase + GPIO_IC_OFFSET, 0x1 << offset);

	return 0;
}

static int amb_gpio_irq_set_wake(struct irq_data *data, unsigned int on)
{
#if defined(CONFIG_PM)
	u32 bank = PINID_TO_BANK(data->hwirq);
	u32 offset = PINID_TO_OFFSET(data->hwirq);

	if (on) {
		amb_gpio_pm[bank].irq_wake_mask |= (1 << offset);
	} else {
		amb_gpio_pm[bank].irq_wake_mask &= ~(1 << offset);
	}
#endif
	return 0;
}

static struct irq_chip amb_gpio_irqchip = {
	.name		= "GPIO",
	.irq_enable	= amb_gpio_irq_enable,
	.irq_disable	= amb_gpio_irq_disable,
	.irq_ack	= amb_gpio_irq_ack,
	.irq_mask	= amb_gpio_irq_mask,
	.irq_mask_ack	= amb_gpio_irq_mask_ack,
	.irq_unmask	= amb_gpio_irq_unmask,
	.irq_set_type	= amb_gpio_irq_set_type,
	.irq_set_wake	= amb_gpio_irq_set_wake,
	.flags		= IRQCHIP_SET_TYPE_MASKED | IRQCHIP_MASK_ON_SUSPEND,
};

static int amb_gpio_irqdomain_map(struct irq_domain *d,
			unsigned int irq, irq_hw_number_t hwirq)
{
	struct amb_gpio_chip *amb_gpio;

	amb_gpio = (struct amb_gpio_chip *)d->host_data;

	irq_set_chip_and_handler(irq, &amb_gpio_irqchip, handle_level_irq);
	irq_set_chip_data(irq, amb_gpio->regbase[PINID_TO_BANK(hwirq)]);
	set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);

	return 0;
}

const struct irq_domain_ops amb_gpio_irq_domain_ops = {
	.map = amb_gpio_irqdomain_map,
	.xlate = irq_domain_xlate_twocell,
};

static void amb_gpio_handle_irq(unsigned int irq, struct irq_desc *desc)
{
	struct irq_chip *irqchip;
	struct amb_gpio_chip *amb_gpio;
	u32 i, gpio_mis, gpio_hwirq, gpio_irq;

	irqchip = irq_desc_get_chip(desc);
	chained_irq_enter(irqchip, desc);

	amb_gpio = irq_get_handler_data(irq);

	/* find the GPIO bank generating this irq */
	for (i = 0; i < GPIO_INSTANCES; i++) {
		if (amb_gpio->irq[i] == irq)
			break;
	}

	if (i == GPIO_INSTANCES)
		return;

	gpio_mis = amba_readl(amb_gpio->regbase[i] + GPIO_MIS_OFFSET);
	if (gpio_mis) {
		gpio_hwirq = i * GPIO_BANK_SIZE + ffs(gpio_mis) - 1;
		gpio_irq = irq_find_mapping(amb_gpio->domain, gpio_hwirq);
		generic_handle_irq(gpio_irq);
	}

	chained_irq_exit(irqchip, desc);
}

static int amb_gpio_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *parent;
	struct amb_gpio_chip *amb_gpio;
	int i, rval;

	amb_gpio = devm_kzalloc(&pdev->dev, sizeof(*amb_gpio), GFP_KERNEL);
	if (!amb_gpio) {
		dev_err(&pdev->dev, "failed to allocate memory for private data\n");
		return -ENOMEM;
	}

	parent = of_get_parent(np);

	for (i = 0; i < GPIO_INSTANCES; i++) {
		amb_gpio->regbase[i] = of_iomap(parent, i);
		if (amb_gpio->regbase[i] == NULL) {
			dev_err(&pdev->dev, "devm_ioremap() failed\n");
			return -ENOMEM;
		}

		amba_writel(amb_gpio->regbase[i] + GPIO_ENABLE_OFFSET, 0xffffffff);

		amb_gpio->irq[i] = irq_of_parse_and_map(np, i);
		if (amb_gpio->irq[i] == 0) {
			dev_err(&pdev->dev, "no irq for gpio[%d]!\n", i);
			return -ENXIO;
		}
	}

	/* iomux_base will get NULL if not existed */
	amb_gpio->iomux_base = of_iomap(parent, i);

	of_node_put(parent);

	amb_gpio->gc = &amb_gc;
	amb_gpio->gc->dev = &pdev->dev;
	rval = gpiochip_add(amb_gpio->gc);
	if (rval) {
		dev_err(&pdev->dev,
			"failed to register gpio_chip %s\n", amb_gpio->gc->label);
		return rval;
	}

	/* Initialize GPIO irq */
	amb_gpio->domain = irq_domain_add_linear(np, amb_gpio->gc->ngpio,
					&amb_gpio_irq_domain_ops, amb_gpio);
	if (!amb_gpio->domain) {
		pr_err("%s: Failed to create irqdomain\n", np->full_name);
		return -ENOSYS;
	}

	for (i = 0; i < GPIO_INSTANCES; i++) {
		irq_set_irq_type(amb_gpio->irq[i], IRQ_TYPE_LEVEL_HIGH);
		irq_set_handler_data(amb_gpio->irq[i], amb_gpio);
		irq_set_chained_handler(amb_gpio->irq[i], amb_gpio_handle_irq);
	}

	platform_set_drvdata(pdev, amb_gpio);

	dev_info(&pdev->dev, "Ambarella GPIO driver registered\n");

	/* register ambarella gpio service for private operation */
	amb_gpio->gpio_service.service = AMBARELLA_SERVICE_GPIO;
	amb_gpio->gpio_service.func = ambarella_gpio_service;
	ambarella_register_service(&amb_gpio->gpio_service);

	return 0;
}

#ifdef CONFIG_PM
static int amb_gpio_irq_suspend(void)
{
	struct amb_gpio_chip *amb_gpio;
	struct amb_gpio_regs *pm;
	void __iomem *regbase;
	int i;

	amb_gpio = dev_get_drvdata(amb_gc.dev);
	if (amb_gpio == NULL) {
		pr_err("No device for ambarella gpio irq\n");
		return -ENODEV;
	}

	for (i = 0; i < GPIO_INSTANCES; i++) {
		regbase = amb_gpio->regbase[i];
		pm = &amb_gpio_pm[i];
		pm->afsel = amba_readl(regbase + GPIO_AFSEL_OFFSET);
		pm->dir = amba_readl(regbase + GPIO_DIR_OFFSET);
		pm->is = amba_readl(regbase + GPIO_IS_OFFSET);
		pm->ibe = amba_readl(regbase + GPIO_IBE_OFFSET);
		pm->iev = amba_readl(regbase + GPIO_IEV_OFFSET);
		pm->ie = amba_readl(regbase + GPIO_IE_OFFSET);
		pm->mask = ~pm->afsel;
		amba_writel(regbase + GPIO_MASK_OFFSET, pm->mask);
		pm->data = amba_readl(regbase + GPIO_DATA_OFFSET);

		if (pm->irq_wake_mask) {
			amba_writel(regbase + GPIO_IE_OFFSET, pm->irq_wake_mask);
			pr_info("gpio_irq[%p]: irq_wake[0x%08X]\n",
						regbase, pm->irq_wake_mask);
		}
	}

	return 0;
}

static void amb_gpio_irq_resume(void)
{
	struct amb_gpio_chip *amb_gpio;
	struct amb_gpio_regs *pm;
	void __iomem *regbase;
	int i;

	amb_gpio = dev_get_drvdata(amb_gc.dev);
	if (amb_gpio == NULL) {
		pr_err("No device for ambarella gpio irq\n");
		return;
	}

	for (i = 0; i < GPIO_INSTANCES; i++) {
		regbase = amb_gpio->regbase[i];
		pm = &amb_gpio_pm[i];
		amba_writel(regbase + GPIO_AFSEL_OFFSET, pm->afsel);
		amba_writel(regbase + GPIO_DIR_OFFSET, pm->dir);
		amba_writel(regbase + GPIO_MASK_OFFSET, pm->mask);
		amba_writel(regbase + GPIO_DATA_OFFSET, pm->data);
		amba_writel(regbase + GPIO_IS_OFFSET, pm->is);
		amba_writel(regbase + GPIO_IBE_OFFSET, pm->ibe);
		amba_writel(regbase + GPIO_IEV_OFFSET, pm->iev);
		amba_writel(regbase + GPIO_IE_OFFSET, pm->ie);
		amba_writel(regbase + GPIO_ENABLE_OFFSET, 0xffffffff);
	}
}

struct syscore_ops amb_gpio_irq_syscore_ops = {
	.suspend	= amb_gpio_irq_suspend,
	.resume		= amb_gpio_irq_resume,
};

#endif

static const struct of_device_id amb_gpio_dt_match[] = {
	{ .compatible = "ambarella,gpio" },
	{},
};
MODULE_DEVICE_TABLE(of, amb_gpio_dt_match);

static struct platform_driver amb_gpio_driver = {
	.probe	= amb_gpio_probe,
	.driver	= {
		.name	= "ambarella-gpio",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(amb_gpio_dt_match),
	},
};

static int __init amb_gpio_drv_register(void)
{
#ifdef CONFIG_PM
	register_syscore_ops(&amb_gpio_irq_syscore_ops);
#endif

	return platform_driver_register(&amb_gpio_driver);
}
postcore_initcall(amb_gpio_drv_register);

MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella SoC GPIO driver");
MODULE_LICENSE("GPL v2");

static int ambarella_gpio_service(void *arg, void *result)
{
	struct ambsvc_gpio *gpio_svc = arg;
	u32 *value = result;
	int rval = 0;

	BUG_ON(!gpio_svc);

	switch (gpio_svc->svc_id) {
	case AMBSVC_GPIO_REQUEST:
	{
		rval = gpio_request(gpio_svc->gpio, "gpio");
		break;
	}
	case AMBSVC_GPIO_OUTPUT:
		rval = gpio_direction_output(gpio_svc->gpio, gpio_svc->value);
		break;

	case AMBSVC_GPIO_INPUT:
		rval = gpio_direction_input(gpio_svc->gpio);
		if (rval >= 0 && value)
			*value = gpio_get_value_cansleep(gpio_svc->gpio);
		break;

	case AMBSVC_GPIO_FREE:
		gpio_free(gpio_svc->gpio);
		break;

	case AMBSVC_GPIO_TO_IRQ:
		*value = gpio_to_irq(gpio_svc->gpio);
		break;

	default:
		pr_err("%s: Invalid gpio service (%d)\n", __func__, gpio_svc->svc_id);
		rval = -EINVAL;
		break;
	}

	return rval;
}

