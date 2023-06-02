#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <asm-generic/gpio.h>
#include <mach/fh_gpio_plat.h>

/*
 * GPIO Direction
 */
#define GPIO_DIR_INPUT          0
#define GPIO_DIR_OUTPUT         1

/*
 * GPIO interrupt type
 */
#define GPIO_INT_TYPE_LEVEL     0
#define GPIO_INT_TYPE_EDGE      1

/*
 * GPIO interrupt polarity
 */
#define GPIO_INT_POL_LOW        0
#define GPIO_INT_POL_HIGH       1

#define OFFSET_GPIO_SWPORTA_DR         (0x0000)
#define OFFSET_GPIO_SWPORTA_DDR        (0x0004)
#define OFFSET_GPIO_PORTA_CTL          (0x0008)
#define OFFSET_GPIO_SWPORTB_DR         (0x000C)
#define OFFSET_GPIO_SWPORTB_DDR        (0x0010)
#define OFFSET_GPIO_PORTB_CTL          (0x0014)
#define OFFSET_GPIO_INTEN              (0x0030)
#define OFFSET_GPIO_INTMASK            (0x0034)
#define OFFSET_GPIO_INTTYPE_LEVEL      (0x0038)
#define OFFSET_GPIO_INT_POLARITY       (0x003C)
#define OFFSET_GPIO_INTSTATUS          (0x0040)
#define OFFSET_GPIO_RAWINTSTATUS       (0x0044)
#define OFFSET_GPIO_DEBOUNCE           (0x0048)
#define OFFSET_GPIO_PORTA_EOI          (0x004C)
#define OFFSET_GPIO_EXT_PORTA          (0x0050)
#define OFFSET_GPIO_EXT_PORTB          (0x0054)
#define OFFSET_GPIO_INT_BOTH           (0x0068)

struct fh_gpio_chip *fh_gpio0, *fh_gpio1, *fh_gpio2, *fh_gpio3;

static inline void FH_GPIO_SetValue(void __iomem *base, int bit, int val)
{
	unsigned int reg;

	reg = readl(base + OFFSET_GPIO_SWPORTA_DR);
	reg = val ? (reg | (1 << bit)) : (reg & ~(1 << bit));
	writel(reg, base + OFFSET_GPIO_SWPORTA_DR);
}

static inline int FH_GPIO_GetValue(void __iomem *base, int bit)
{
	return (readl(base + OFFSET_GPIO_EXT_PORTA) >> bit) & 0x1;
}

static inline void FH_GPIO_SetDirection(void __iomem *base, int bit, int dir)
{
	unsigned int reg;

	reg = readl(base + OFFSET_GPIO_SWPORTA_DDR);
	reg = dir ? reg | (1 << bit) : reg & ~(1 << bit);
	writel(reg, base + OFFSET_GPIO_SWPORTA_DDR);
}

static inline int FH_GPIO_GetDirection(void __iomem *base, int bit)
{
	return (readl(base + OFFSET_GPIO_SWPORTA_DDR) >> bit) & 0x1;
}

static inline void FH_GPIOB_SetValue(void __iomem *base, int bit, int val)
{
	unsigned int reg;

	reg = readl(base + OFFSET_GPIO_SWPORTB_DR);
	reg = val ? (reg | (1 << bit)) : (reg & ~(1 << bit));
	writel(reg, base + OFFSET_GPIO_SWPORTB_DR);
}

static inline int FH_GPIOB_GetValue(void __iomem *base, int bit)
{
	return (readl(base + OFFSET_GPIO_EXT_PORTB) >> bit) & 0x1;
}

static inline void FH_GPIOB_SetDirection(void __iomem *base, int bit, int dir)
{
	unsigned int reg;

	reg = readl(base + OFFSET_GPIO_SWPORTB_DDR);
	reg = dir ? reg | (1 << bit) : reg & ~(1 << bit);
	writel(reg, base + OFFSET_GPIO_SWPORTB_DDR);
}

static inline int FH_GPIOB_GetDirection(void __iomem *base, int bit)
{
	return (readl(base + OFFSET_GPIO_SWPORTB_DDR) >> bit) & 0x1;
}

static inline void FH_GPIO_EnableDebounce(void __iomem *base, int bit, int bool)
{
	unsigned int reg;

	reg = readl(base + OFFSET_GPIO_DEBOUNCE);
	reg = bool ? reg | (1 << bit) : reg & ~(1 << bit);
	writel(reg, base + OFFSET_GPIO_DEBOUNCE);
}

static inline void FH_GPIO_SetInterruptType(void __iomem *base, int bit,
		int type)
{
	unsigned int reg;

	reg = readl(base + OFFSET_GPIO_INTTYPE_LEVEL);
	reg = type ? reg | (1 << bit) : reg & ~(1 << bit);
	writel(reg, base + OFFSET_GPIO_INTTYPE_LEVEL);
}

static inline void FH_GPIO_SetInterruptPolarity(void __iomem *base, int bit,
		int pol)
{
	unsigned int reg;

	reg = readl(base + OFFSET_GPIO_INT_POLARITY);
	reg = pol ? reg | (1 << bit) : reg & ~(1 << bit);
	writel(reg, base + OFFSET_GPIO_INT_POLARITY);
}

static inline void FH_GPIO_Set_BothInterrupt_enable(void __iomem *base, int bit)
{
	unsigned int reg;

	reg = readl(base + OFFSET_GPIO_INT_BOTH);
	reg |= (1 << bit);
	writel(reg, base + OFFSET_GPIO_INT_BOTH);
}

static inline void FH_GPIO_Set_BothInterrupt_disable(void __iomem *base,
	 int bit)
{
	unsigned int reg;

	reg = readl(base + OFFSET_GPIO_INT_BOTH);
	reg &= ~(1 << bit);
	writel(reg, base + OFFSET_GPIO_INT_BOTH);
}

static inline void FH_GPIO_EnableInterruptMask(void __iomem *base, int bit,
		int bool)
{
	unsigned int reg;

	reg = readl(base + OFFSET_GPIO_INTMASK);
	reg = bool ? reg | (1 << bit) : reg & ~(1 << bit);
	writel(reg, base + OFFSET_GPIO_INTMASK);
}

static inline void FH_GPIO_EnableInterrupt(void __iomem *base, int bit, int bool)
{
	unsigned int reg;

	reg = readl(base + OFFSET_GPIO_INTEN);
	reg = bool ? reg | (1 << bit) : reg & ~(1 << bit);
	writel(reg, base + OFFSET_GPIO_INTEN);
}

static inline void FH_GPIO_SetEnableInterrupts(void __iomem *base,
		unsigned int val)
{
	writel(val, base + OFFSET_GPIO_INTEN);
}

static inline unsigned int FH_GPIO_GetEnableInterrupts(void __iomem *base)
{
	return readl(base + OFFSET_GPIO_INTEN);
}

static inline unsigned int FH_GPIO_GetInterruptStatus(void __iomem *base)
{
	return readl(base + OFFSET_GPIO_INTSTATUS);
}

static inline void FH_GPIO_ClearInterrupt(void __iomem *base, int bit)
{
	unsigned int reg;

	reg = readl(base + OFFSET_GPIO_PORTA_EOI);
	reg |= (1 << bit);
	writel(reg, base + OFFSET_GPIO_PORTA_EOI);
}


static inline void __iomem *gpio_to_base(unsigned int gpio)
{
	void __iomem *base;
	gpio = gpio / 32;

	switch (gpio) {
	case 0:
		base = fh_gpio0->base;
		break;
	case 1:
		base = fh_gpio1->base;
		break;
	case 2:
		base = fh_gpio2->base;
		break;
	case 3:
		base = fh_gpio3->base;
		break;
	default:
		pr_err("ERROR: incorrect GPIO num\n");
		base = NULL;
		break;
	}

	return base;
}

static int _set_gpio_irq_type(unsigned int gpio, unsigned int type)
{
	u32 int_type, int_polarity;
	u32 bit = gpio % 32;
	void __iomem *base;
	struct fh_gpio_chip *fh_gpio = NULL;

	switch (gpio / 32) {
	case 0:
		fh_gpio = fh_gpio0;
		break;
	case 1:
		fh_gpio = fh_gpio1;
		break;
	case 2:
		fh_gpio = fh_gpio2;
		break;
	case 3:
		fh_gpio = fh_gpio3;
		break;
	default:
		return -EINVAL;
	}

	base = gpio_to_base(gpio);
	switch (type & IRQF_TRIGGER_MASK) {
	case IRQ_TYPE_EDGE_BOTH:
		if (!fh_gpio->type) {
			int_type = GPIO_INT_TYPE_EDGE;
			/* toggle trigger */
			if (FH_GPIO_GetValue(base, bit))
				int_polarity = GPIO_INT_POL_LOW;
			else
				int_polarity = GPIO_INT_POL_HIGH;
		}
		break;
	case IRQ_TYPE_EDGE_RISING:
		int_type = GPIO_INT_TYPE_EDGE;
		int_polarity = GPIO_INT_POL_HIGH;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		int_type = GPIO_INT_TYPE_EDGE;
		int_polarity = GPIO_INT_POL_LOW;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		int_type = GPIO_INT_TYPE_LEVEL;
		int_polarity = GPIO_INT_POL_HIGH;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		int_type = GPIO_INT_TYPE_LEVEL;
		int_polarity = GPIO_INT_POL_LOW;
		break;
	case IRQ_TYPE_NONE:
		return 0;
	default:
		return -EINVAL;
	}

	if ((type & IRQF_TRIGGER_MASK) == IRQ_TYPE_EDGE_BOTH) {
		if (fh_gpio->type)
			FH_GPIO_Set_BothInterrupt_enable(base, bit);
		else {
			FH_GPIO_SetInterruptType(base, bit, int_type);
			FH_GPIO_SetInterruptPolarity(base, bit, int_polarity);
		}
	} else {
		FH_GPIO_Set_BothInterrupt_disable(base, bit);
		FH_GPIO_SetInterruptType(base, bit, int_type);
		FH_GPIO_SetInterruptPolarity(base, bit, int_polarity);
	}

	return 0;
}

int fh_set_gpio_irq(struct gpio_irq_info *info)
{
	void __iomem *base;

	base = gpio_to_base(info->irq_gpio);
	return _set_gpio_irq_type(info->irq_gpio, info->irq_type);
}
EXPORT_SYMBOL(fh_set_gpio_irq);

void fh_irq_enable(unsigned int gpio)
{
	void __iomem *base;
	int gpio_num = gpio % 32;

	base = gpio_to_base(gpio);

	FH_GPIO_EnableInterrupt(base, gpio_num, 1);
}
EXPORT_SYMBOL(fh_irq_enable);

void fh_irq_disable(unsigned int gpio)
{
	void __iomem *base;
	int gpio_num = gpio % 32;

	base = gpio_to_base(gpio);
	FH_GPIO_EnableInterrupt(base, gpio_num, 0);
}
EXPORT_SYMBOL(fh_irq_disable);

void fh_clear_gpio_irq(int gpio_id)
{
	void __iomem *base;
	int gpio_num = gpio_id % 32;

	base = gpio_to_base(gpio_id);
	FH_GPIO_ClearInterrupt(base, gpio_num);
}
EXPORT_SYMBOL(fh_clear_gpio_irq);

static inline void __iomem *irq_to_controller(struct irq_data *d)
{
	struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d);

	return fh_gpio->base;
}

static void gpio_irq_ack(struct irq_data *d)
{
	void __iomem *base;

	unsigned int gpio_bit = d->hwirq;
	/* struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d); */

	base = irq_to_controller(d);
	FH_GPIO_ClearInterrupt(base, gpio_bit);
}

static void gpio_irq_enable(struct irq_data *d)
{
	void __iomem *base;

	unsigned gpio_bit = d->hwirq;
	/* struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d); */

	base = irq_to_controller(d);
	FH_GPIO_EnableInterrupt(base, gpio_bit, 1);
}

static void gpio_irq_disable(struct irq_data *d)
{
	void __iomem *base;

	unsigned gpio_bit = d->hwirq;
	/* struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d); */

	base = irq_to_controller(d);
	FH_GPIO_EnableInterrupt(base, gpio_bit, 0);
}

static void gpio_irq_mask(struct irq_data *d)
{
	void __iomem *base;

	unsigned gpio_bit = d->hwirq;
	/* struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d); */

	base = irq_to_controller(d);
	FH_GPIO_EnableInterruptMask(base, gpio_bit, 1);
}

static void gpio_irq_unmask(struct irq_data *d)
{
	void __iomem *base;

	unsigned int gpio_bit = d->hwirq;
	/* struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d); */

	base = irq_to_controller(d);
	FH_GPIO_EnableInterruptMask(base, gpio_bit, 0);
}

static int gpio_irq_type(struct irq_data *d, unsigned int type)
{
	void __iomem *base;
	unsigned int gpio;

	unsigned gpio_bit = d->hwirq;
	struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d);

	base = irq_to_controller(d);
	gpio = gpio_bit + fh_gpio->chip.base;
	return _set_gpio_irq_type(gpio, type);
}

#ifdef CONFIG_PM

static int gpio_irq_set_wake(struct irq_data *d, unsigned value)
{
	int gpio = d->hwirq;
	struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d);

	if (value)
		fh_gpio->gpio_wakeups |= (1 << gpio);
	else
		fh_gpio->gpio_wakeups &= ~(1 << gpio);

	return 0;
}

void fh_gpio_irq_suspend(void)
{
	fh_gpio0->gpio_backups = FH_GPIO_GetEnableInterrupts(fh_gpio0->base);
	fh_gpio1->gpio_backups = FH_GPIO_GetEnableInterrupts(fh_gpio1->base);

	FH_GPIO_SetEnableInterrupts(fh_gpio0->base, fh_gpio0->gpio_wakeups);
	FH_GPIO_SetEnableInterrupts(fh_gpio1->base, fh_gpio1->gpio_wakeups);
}

void fh_gpio_irq_resume(void)
{
	FH_GPIO_SetEnableInterrupts(fh_gpio0->base, fh_gpio0->gpio_backups);
	FH_GPIO_SetEnableInterrupts(fh_gpio1->base, fh_gpio1->gpio_backups);
}

#else
#define gpio_irq_set_wake   NULL
#endif

static struct irq_chip gpio_irqchip = {
	.name           = "FH_GPIO_INTC",
	.irq_ack        = gpio_irq_ack,
	.irq_enable     = gpio_irq_enable,
	.irq_disable    = gpio_irq_disable,
	.irq_mask       = gpio_irq_mask,
	.irq_unmask     = gpio_irq_unmask,
	.irq_set_type   = gpio_irq_type,
	.irq_set_wake   = gpio_irq_set_wake,
};

static void gpio_toggle_trigger(unsigned int gpio, unsigned int offs)
{
	u32 int_polarity;
	int gpio_num = gpio % 32;
	void __iomem *base = gpio_to_base(gpio);

	if (FH_GPIO_GetValue(base, offs))
		int_polarity = GPIO_INT_POL_LOW;
	else
		int_polarity = GPIO_INT_POL_HIGH;

	FH_GPIO_SetInterruptPolarity(base, gpio_num, int_polarity);
}

static void gpio_irq_handler(struct irq_desc *desc)
{
	struct irq_data *irqdata = irq_desc_get_irq_data(desc);
	struct irq_chip *irqchip = irq_data_get_irq_chip(irqdata);
	struct fh_gpio_chip *fh_gpio = irq_desc_get_handler_data(desc);
	u32 irq_status;
	int gpio_num;

	chained_irq_enter(irqchip, desc);
	irq_status = FH_GPIO_GetInterruptStatus(fh_gpio->base);

	if (unlikely(irq_status == 0)) {
		pr_err("gpio irq status is zero.\n");
		return;
	}

	/* temporarily mask (level sensitive) parent IRQ */
	irqchip->irq_mask(irqdata);

	gpio_num = fls(irq_status) - 1;

	FH_GPIO_ClearInterrupt(fh_gpio->base, gpio_num);

	generic_handle_irq(irq_find_mapping(fh_gpio->irq_domain, gpio_num));

	if ((irq_get_trigger_type(irq_find_mapping(fh_gpio->irq_domain, gpio_num)) & IRQ_TYPE_SENSE_MASK) == IRQ_TYPE_EDGE_BOTH)
		if (!fh_gpio->type)
			gpio_toggle_trigger((fh_gpio->chip.base + gpio_num), gpio_num);

	irqchip->irq_unmask(irqdata);

	chained_irq_exit(irqchip, desc);
	/* now it may re-trigger */
}

/*
* This lock class tells lockdep that GPIO irqs are in a different
* category than their parents, so it won't report 0 recursion.
*/
static struct lock_class_key gpio_lock_class;

static int fh_gpio_irq_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hwirq)
{
	int ret;

	ret = irq_set_chip_data(irq, d->host_data);
	if (ret < 0)
		return ret;
	irq_set_lockdep_class(irq, &gpio_lock_class);
	irq_set_chip_and_handler(irq, &gpio_irqchip, handle_simple_irq);
	irq_set_noprobe(irq);

	return 0;

}

static void fh_gpio_irq_unmap(struct irq_domain *d, unsigned int irq)
{
	irq_set_chip_and_handler(irq, NULL, NULL);
	irq_set_chip_data(irq, NULL);
}

static struct irq_domain_ops fh_gpio_irq_ops = {
	.map = fh_gpio_irq_map,
	.unmap = fh_gpio_irq_unmap,
	.xlate = irq_domain_xlate_twocell,
};

static int fh_gpio_irq_create_domain(struct gpio_chip *c, unsigned offset)
{
	struct fh_gpio_chip *chip;

	chip = container_of(c, struct fh_gpio_chip, chip);
	return irq_create_mapping(chip->irq_domain, offset);
}

static int chip_to_irq(struct gpio_chip *c, unsigned offset)
{
	struct fh_gpio_chip *chip;

	chip = container_of(c, struct fh_gpio_chip, chip);
	if (offset > chip->chip.ngpio)
		return -ENXIO;
	return irq_find_mapping(chip->irq_domain, offset);
}

static int chip_gpio_get(struct gpio_chip *c, unsigned offset)
{
	u32 bit = offset % 32;
	struct fh_gpio_chip *chip;

	chip = container_of(c, struct fh_gpio_chip, chip);
	if (offset / 32)
		return FH_GPIOB_GetValue(chip->base, bit);
	else
		return FH_GPIO_GetValue(chip->base, bit);
}

static void chip_gpio_set(struct gpio_chip *c, unsigned int offset, int val)
{
	u32 bit = offset % 32;
	struct fh_gpio_chip *chip;

	chip = container_of(c, struct fh_gpio_chip, chip);
	if (offset / 32)
		FH_GPIOB_SetValue(chip->base, bit, val);
	else
		FH_GPIO_SetValue(chip->base, bit, val);
}

static int chip_direction_input(struct gpio_chip *c, unsigned offset)
{
	u32 bit = offset % 32;
	unsigned long flags;
	struct fh_gpio_chip *chip;

	chip = container_of(c, struct fh_gpio_chip, chip);
	spin_lock_irqsave(&chip->lock, flags);
	if (offset / 32)
		FH_GPIOB_SetDirection(chip->base, bit, GPIO_DIR_INPUT);
	else
		FH_GPIO_SetDirection(chip->base, bit, GPIO_DIR_INPUT);
	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static int chip_direction_output(struct gpio_chip *c, unsigned offset, int val)
{
	u32 bit = offset % 32;
	unsigned long flags;
	struct fh_gpio_chip *chip;

	chip = container_of(c, struct fh_gpio_chip, chip);
	spin_lock_irqsave(&chip->lock, flags);
	if (offset / 32) {
		FH_GPIOB_SetDirection(chip->base, bit, GPIO_DIR_OUTPUT);
		FH_GPIOB_SetValue(chip->base, bit, val);
	} else {
		FH_GPIO_SetDirection(chip->base, bit, GPIO_DIR_OUTPUT);
		FH_GPIO_SetValue(chip->base, bit, val);
	}
	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static int chip_gpio_set_debounce(struct gpio_chip *c, unsigned offset,
	unsigned int debounce)
{
	u32 bit = offset % 32;
	unsigned long flags;
	char db_clk_name[16] = {0};
	struct clk *gpio_clk = NULL;
	int ret = 0;
	struct fh_gpio_chip *chip;
	bool enabled = !!debounce;
	unsigned int clk_rate = 0;

	sprintf(db_clk_name, "gpio%d_gbclk", (offset / 32));
	gpio_clk = clk_get(NULL, db_clk_name);
	if (IS_ERR(gpio_clk))
		return PTR_ERR(gpio_clk);

	clk_rate = 1000000UL / debounce;

	ret = clk_set_rate(gpio_clk, clk_rate);
	if (ret) {
		pr_err("Set GPIO Debounce Clk fail\n");
		return ret;
	}

	ret = clk_prepare_enable(gpio_clk);
	if (ret) {
		pr_err("Set GPIO Debounce Clk fail\n");
		return ret;
	}

	chip = container_of(c, struct fh_gpio_chip, chip);
	spin_lock_irqsave(&chip->lock, flags);
	FH_GPIO_EnableDebounce(chip->base, bit, enabled);
	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

void fh_gpio_set(int gpio_id, int value)
{
	__gpio_set_value(gpio_id, value);
}
EXPORT_SYMBOL(fh_gpio_set);

int fh_gpio_get(int gpio_id, int *value)
{
	*value = __gpio_get_value(gpio_id);
	return 0;
}
EXPORT_SYMBOL(fh_gpio_get);

int fh_gpio_reset(int gpio_id)
{
	return 0;
}
EXPORT_SYMBOL(fh_gpio_reset);

static void fh_gpio_irq_init(struct platform_device *pdev)
{
	int i, gpio_irq;
	struct fh_gpio_chip *plat_data;

	plat_data = pdev->dev.platform_data;

	for (i = 0; i < 32; i++) {
		gpio_irq = fh_gpio_irq_create_domain(&plat_data->chip, i);
		irq_set_lockdep_class(gpio_irq, &gpio_lock_class);
		irq_set_chip_and_handler(gpio_irq, &gpio_irqchip, handle_simple_irq);
		irq_set_chip_data(gpio_irq, plat_data);
	}

	irq_set_chained_handler_and_data(plat_data->irq, gpio_irq_handler, plat_data);
}

static struct of_device_id const fh_gpio_of_match[] = {
	{ .compatible = "fh,fh-gpio" },
	{}
};
MODULE_DEVICE_TABLE(of, fh_gpio_of_match);

static int fh_gpio_probe(struct platform_device *pdev)
{
	int err = -EIO;
	int id;
	struct fh_gpio_chip *plat_data;
#ifdef CONFIG_OF
	int ngpio;
	const struct of_device_id *match;
	struct device_node *np = pdev->dev.of_node;
#else
	struct resource *res;
#endif

#ifdef CONFIG_OF
	match = of_match_device(fh_gpio_of_match, &pdev->dev);
	if (!match) {
		pr_info("Failed to find gpio controller\n");
		return -ENODEV;
	}

	plat_data = devm_kzalloc(&pdev->dev, sizeof(*plat_data), GFP_KERNEL);
	if (!plat_data)
		return -ENOMEM;

	plat_data->chip.of_node = np;
	/* set up the driver-specific struct */
	of_property_read_u32(np, "ngpio", &ngpio);
	plat_data->chip.ngpio = ngpio;
	of_property_read_u32(np, "base", &plat_data->chip.base);
	of_property_read_u32(np, "id", &id);
	of_property_read_u32(np, "trigger-type", &plat_data->type);

	plat_data->irq = irq_of_parse_and_map(np, 0);
	if (plat_data->irq < 0) {
		dev_err(&pdev->dev, "gpio interrupt is not available.\n");
		return plat_data->irq;
	}

	plat_data->base = of_iomap(np, 0);
	if (plat_data->base == NULL) {
		err = -ENXIO;
		goto release_irq;
	}

	plat_data->irq_domain = irq_domain_add_linear(np, plat_data->chip.ngpio, &fh_gpio_irq_ops, plat_data);

	if (!plat_data->irq_domain) {
		dev_err(&pdev->dev, "Couldn't allocate IRQ domain\n");
		err = -ENXIO;
		goto release_io;
	}
#else
	plat_data = pdev->dev.platform_data;

	id = pdev->id;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "can't fetch device resource info\n");
		return err;
	}

	plat_data->irq  = irq_create_mapping(NULL, platform_get_irq(pdev, 0));
	if (plat_data->irq < 0) {
		dev_err(&pdev->dev, "gpio interrupt is not available.\n");
		return plat_data->irq;
	}

	plat_data->base = ioremap(res->start, resource_size(res));
	if (plat_data->base == NULL) {
		err = -ENXIO;
		goto release_irq;
	}

	plat_data->irq_domain = irq_domain_add_linear(NULL,
		plat_data->chip.ngpio, &fh_gpio_irq_ops, plat_data);

	if (!plat_data->irq_domain) {
		dev_err(&pdev->dev, "Couldn't allocate IRQ domain\n");
		err = -ENXIO;
		goto release_io;
	}
#endif

	plat_data->chip.direction_input = chip_direction_input;
	plat_data->chip.direction_output = chip_direction_output;
	plat_data->chip.get = chip_gpio_get;
	plat_data->chip.set = chip_gpio_set;
	plat_data->chip.to_irq = chip_to_irq;
	plat_data->chip.set_debounce = chip_gpio_set_debounce;

	switch (id) {
	case 0:
		fh_gpio0 = plat_data;
		plat_data->chip.names = NULL;
		break;
	case 1:
		fh_gpio1 = plat_data;
		plat_data->chip.names = NULL;
		break;
	case 2:
		fh_gpio2 = plat_data;
		plat_data->chip.names = NULL;
		break;
	case 3:
		fh_gpio3 = plat_data;
		plat_data->chip.names = NULL;
		break;
	default:
		dev_err(&pdev->dev, "Unknown GPIO Controller\n");
		err = -ENXIO;
		goto release_domain;
	}

	plat_data->pdev = pdev;
	spin_lock_init(&plat_data->lock);
	pdev->dev.platform_data = plat_data;
	/* finally, register with the generic GPIO API */
	err = gpiochip_add(&plat_data->chip);
	if (err) {
		pr_err("GPIO support load fail.\n");
		goto release_domain;
	}

	fh_gpio_irq_init(pdev);
	pr_debug("GPIO support successfully loaded.\n\tBase Addr: 0x%p\n",
		plat_data->base);

	return 0;

release_domain:
	irq_domain_remove(plat_data->irq_domain);
release_io:
	iounmap(plat_data->base);
release_irq:
	irq_dispose_mapping(plat_data->irq);
	free_irq(plat_data->irq, plat_data);

	return err;
}

static int fh_gpio_remove(struct platform_device *pdev)
{
	struct fh_gpio_chip *plat_data;

	plat_data = pdev->dev.platform_data;

	gpiochip_remove(&plat_data->chip);
	irq_domain_remove(plat_data->irq_domain);
	iounmap(plat_data->base);

	return 0;
}

static struct platform_driver fh_gpio_driver = {
	.driver = {
		.name = GPIO_NAME,
		.owner = THIS_MODULE,
		.of_match_table = fh_gpio_of_match,
	},
	.probe = fh_gpio_probe,
	.remove = fh_gpio_remove,
};

static int __init fh_gpio_init(void)
{
	return platform_driver_register(&fh_gpio_driver);
}

static void __exit fh_gpio_exit(void)
{
	platform_driver_unregister(&fh_gpio_driver);
}

module_init(fh_gpio_init);
module_exit(fh_gpio_exit);

MODULE_AUTHOR("QIN");
MODULE_DESCRIPTION("Fullhan GPIO device driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform: FH");
