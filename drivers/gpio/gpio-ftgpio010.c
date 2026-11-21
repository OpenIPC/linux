/*
 * Copyright (C) 2010 GM Corp. (http://www.grain-media.com)
 *
 * Grain Media - GPIOlib support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/spinlock.h>
#include <asm/mach/irq.h>
#include <asm/div64.h>

#include <mach/platform/gpio.h>
#include <mach/ftpmu010.h>

#define GM_GPIO010_DEBUG	0
#define GM_GPIO010_TEST		0

#define GM_GPIO010_GPIODATAOUT		0x00
#define GM_GPIO010_GPIODATAIN		0x04
#define GM_GPIO010_PINDIR		0x08
#define GM_GPIO010_PINBYPASS		0x0C
#define GM_GPIO010_GPIODATASET		0x10
#define GM_GPIO010_GPIODATACLEAR	0x14
#define GM_GPIO010_PINPULLENABLE	0x18
#define GM_GPIO010_PINPULLTYPE		0x1C
#define GM_GPIO010_INTRENABLE		0x20
#define GM_GPIO010_INTRRAWSTATE		0x24
#define GM_GPIO010_INTRMASKEDSTATE	0x28
#define GM_GPIO010_INTRMASK		0x2C
#define GM_GPIO010_INTRCLEAR		0x30
#define GM_GPIO010_INTRTRIGGER		0x34
#define GM_GPIO010_INTRBOTH		0x38
#define GM_GPIO010_INTRRISENEG		0x3C
#define GM_GPIO010_BOUNCEENABLE		0x40
#define GM_GPIO010_BOUNCEPERSCALE	0x44
#define GM_GPIO010_REVISIONNUM		0x48

struct gm_gpio_chip {
	const char *name;
	void __iomem *start_vaddr;
	struct resource *res;
	int irq;
	int irq_base;		/* first pin interrupt, 0 when there are none */
	spinlock_t lock;	/* interrupt register read-modify-write */
	struct gpio_chip gc;
	struct platform_device *platform_dev;
	int id;
};

/*
 *  set pin as input
 */
static int gm_gpio_direction_input(struct gpio_chip *gc, unsigned pin)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);
	u32 pin_dir = 0;

	if (unlikely(gc == NULL)) {
		printk("%s fails: gc is NULL\n", __FUNCTION__);
		return -1;
	}
	//set input
	pin_dir =
	    (ioread32(chip->start_vaddr + GM_GPIO010_PINDIR) & (~(1 << pin)));
	iowrite32(pin_dir, chip->start_vaddr + GM_GPIO010_PINDIR);

	return 0;
}

/*
 *  set pin as output
 */
static int gm_gpio_direction_output(struct gpio_chip *gc, unsigned pin, int val)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);
	u32 pin_dir = 0;
	u32 pin_level = 0;

	if (unlikely(gc == NULL)) {
		printk("%s fails: gc is NULL\n", __FUNCTION__);
		return -1;
	}
	//set output
	pin_dir =
	    (ioread32(chip->start_vaddr + GM_GPIO010_PINDIR) | (1 << pin));
	iowrite32(pin_dir, chip->start_vaddr + GM_GPIO010_PINDIR);

	//set pin level
	pin_level = ioread32(chip->start_vaddr + GM_GPIO010_GPIODATAOUT);

	if (val != 0) {
		pin_level |= (1 << pin);
	} else {
		pin_level &= (~(1 << pin));
	}

	iowrite32(pin_level, chip->start_vaddr + GM_GPIO010_GPIODATAOUT);

	return 0;
}

/*
 *  return the level of pin
 */
static int gm_gpio_get_value(struct gpio_chip *gc, unsigned pin)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);
	u32 pin_dir = 0;
	u32 pin_level = 0;

	if (unlikely(gc == NULL)) {
		printk("%s fails: gc is NULL\n", __FUNCTION__);
		return -1;
	}

	pin_dir = ioread32(chip->start_vaddr + GM_GPIO010_PINDIR) & (1 << pin);

	if (pin_dir != 0) {	//output
		pin_level =
		    ioread32(chip->start_vaddr +
			     GM_GPIO010_GPIODATAOUT) & (1 << pin);
	} else {		//input
		pin_level =
		    ioread32(chip->start_vaddr +
			     GM_GPIO010_GPIODATAIN) & (1 << pin);

	}

	return pin_level;
}

/*
 *  set output pin level
 */
static void gm_gpio_set_value(struct gpio_chip *gc, unsigned pin, int val)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);
	u32 pin_dir = 0;
	u32 pin_level = 0;

	if (unlikely(gc == NULL)) {
		printk("%s fails: gc is NULL\n", __FUNCTION__);
		return;
	}

	pin_dir = ioread32(chip->start_vaddr + GM_GPIO010_PINDIR) & (1 << pin);

	if (likely(pin_dir != 0)) {	//only set level when output
		pin_level =
		    ioread32(chip->start_vaddr + GM_GPIO010_GPIODATAOUT);
		if (val != 0) {
			pin_level |= (1 << pin);
		} else {
			pin_level &= (~(1 << pin));
		}
		iowrite32(pin_level,
			  chip->start_vaddr + GM_GPIO010_GPIODATAOUT);
	} else {
		printk("%s fail: you should only set output pin.\n",
		       __FUNCTION__);
		return;
	}
}

/*
 *  return GPIO irq, in our platform, all gpio share the same irq number
 */
static int gm_gpio_to_irq(struct gpio_chip *gc, unsigned pin)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);

	if (unlikely(gc == NULL)) {
		printk("%s fails: gc is NULL\n", __FUNCTION__);
		return -1;
	}

	if (chip->irq_base)
		return chip->irq_base + pin;
	return chip->irq;
}

static int gm_int_clr(struct gpio_chip *gc, unsigned pin)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);
	u32 int_st = ioread32(chip->start_vaddr + GM_GPIO010_INTRCLEAR);

	if (unlikely(gc == NULL)) {
		printk("%s fails: gc is NULL\n", __FUNCTION__);
		return -1;
	}

	int_st |= (1 << pin);
	iowrite32(int_st, chip->start_vaddr + GM_GPIO010_INTRCLEAR);

	return 0;
}

static int xgm_set_or_clr(void __iomem * address, const int pin,
			  const u32 condition, const u32 compare)
{
	u32 tmp = ioread32(address);
	u32 pin_offset = (1 << pin);

	if (condition == compare) {
		tmp |= pin_offset;
	} else {
		tmp &= ~pin_offset;
	}

	iowrite32(tmp, address);

	return 0;
}

static int gm_int_setup(struct gpio_chip *gc, unsigned pin,
			const struct gpio_interrupt_mode *mode)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);

	xgm_set_or_clr(chip->start_vaddr + GM_GPIO010_INTRTRIGGER,
		       pin, mode->trigger_method, GPIO_INT_TRIGGER_LEVEL);

	xgm_set_or_clr(chip->start_vaddr + GM_GPIO010_INTRBOTH,
		       pin, mode->trigger_edge_nr, GPIO_INT_BOTH_EDGE);

	xgm_set_or_clr(chip->start_vaddr + GM_GPIO010_INTRRISENEG,
		       pin, mode->trigger_rise_neg, GPIO_INT_FALLING);
	return 0;
}

static int gm_int_enable(struct gpio_chip *gc, unsigned pin)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);
	u32 int_enable = ioread32(chip->start_vaddr + GM_GPIO010_INTRENABLE);

	int_enable |= (1 << pin);
	iowrite32(int_enable, chip->start_vaddr + GM_GPIO010_INTRENABLE);

	return 0;
}

static int gm_int_disable(struct gpio_chip *gc, unsigned pin)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);
	u32 int_enable = ioread32(chip->start_vaddr + GM_GPIO010_INTRENABLE);

	int_enable &= (~(1 << pin));
	iowrite32(int_enable, chip->start_vaddr + GM_GPIO010_INTRENABLE);

	return 0;
}

static int gm_int_check(struct gpio_chip *gc, unsigned pin)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);
	u32 int_status = ioread32(chip->start_vaddr + GM_GPIO010_INTRRAWSTATE);
	int ret = 0;

	if (int_status & (1 << pin))
		ret = 1;
	return ret;
}

static int gm_int_mask(struct gpio_chip *gc, unsigned pin)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);
	u32 int_enable = ioread32(chip->start_vaddr + GM_GPIO010_INTRMASK);

	int_enable |= (1 << pin);
	iowrite32(int_enable, chip->start_vaddr + GM_GPIO010_INTRMASK);

	return 0;
}

static int gm_int_unmask(struct gpio_chip *gc, unsigned pin)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);
	u32 int_enable = ioread32(chip->start_vaddr + GM_GPIO010_INTRMASK);

	int_enable &= (~(1 << pin));
	iowrite32(int_enable, chip->start_vaddr + GM_GPIO010_INTRMASK);

	return 0;
}

/*
 * Hardware debounce: a pin's input must stay stable for one debounce step
 * before the controller takes it. The step is the APB clock divided by the
 * bank's prescaler (24 bits), which all 32 pins of the bank share, so the
 * bank has one debounce time: the last non-zero request sets it for every
 * debounced pin. debounce is in microseconds; 0 turns it off for the pin.
 */
static int gm_gpio_set_debounce(struct gpio_chip *gc, unsigned pin,
				unsigned debounce)
{
	struct gm_gpio_chip *chip = container_of(gc, struct gm_gpio_chip, gc);
	unsigned long flags;
	u32 val;

	if (debounce) {
		u64 div = (u64)APB_CLK_IN * debounce;

		do_div(div, 1000000);
		if (div == 0)
			div = 1;
		if (div > 0xffffff)
			return -EINVAL;
		iowrite32((u32)div, chip->start_vaddr + GM_GPIO010_BOUNCEPERSCALE);
	}

	spin_lock_irqsave(&chip->lock, flags);
	val = ioread32(chip->start_vaddr + GM_GPIO010_BOUNCEENABLE);
	if (debounce)
		val |= 1 << pin;
	else
		val &= ~(1 << pin);
	iowrite32(val, chip->start_vaddr + GM_GPIO010_BOUNCEENABLE);
	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

#ifdef GM_GPIO_IRQ_BASE
/*
 * Pin interrupts. The bank's interrupt line is shared by its 32 pins; each
 * pin gets its own Linux interrupt (GM_GPIO_IRQ_BASE + gpio number), so
 * request_irq() and the sysfs "edge" file work per pin. The controller
 * does edge (rising, falling, both) and level (high, low) triggers in
 * hardware.
 */
static void gm_gpio_irq_rmw(struct gm_gpio_chip *chip, u32 reg, u32 bit,
			    int set)
{
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&chip->lock, flags);
	val = ioread32(chip->start_vaddr + reg);
	if (set)
		val |= bit;
	else
		val &= ~bit;
	iowrite32(val, chip->start_vaddr + reg);
	spin_unlock_irqrestore(&chip->lock, flags);
}

static u32 gm_gpio_irq_bit(struct irq_data *d)
{
	struct gm_gpio_chip *chip = irq_data_get_irq_chip_data(d);

	return 1 << (d->irq - chip->irq_base);
}

static void gm_gpio_irq_ack(struct irq_data *d)
{
	struct gm_gpio_chip *chip = irq_data_get_irq_chip_data(d);

	iowrite32(gm_gpio_irq_bit(d), chip->start_vaddr + GM_GPIO010_INTRCLEAR);
}

static void gm_gpio_irq_mask(struct irq_data *d)
{
	struct gm_gpio_chip *chip = irq_data_get_irq_chip_data(d);

	gm_gpio_irq_rmw(chip, GM_GPIO010_INTRENABLE, gm_gpio_irq_bit(d), 0);
}

static void gm_gpio_irq_unmask(struct irq_data *d)
{
	struct gm_gpio_chip *chip = irq_data_get_irq_chip_data(d);
	u32 bit = gm_gpio_irq_bit(d);

	gm_gpio_irq_rmw(chip, GM_GPIO010_INTRMASK, bit, 0);
	gm_gpio_irq_rmw(chip, GM_GPIO010_INTRENABLE, bit, 1);
}

static int gm_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct gm_gpio_chip *chip = irq_data_get_irq_chip_data(d);
	u32 bit = gm_gpio_irq_bit(d);
	int level, both = 0, low = 0;

	switch (type & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_EDGE_BOTH:
		level = 0;
		both = 1;
		break;
	case IRQ_TYPE_EDGE_RISING:
		level = 0;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		level = 0;
		low = 1;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		level = 1;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		level = 1;
		low = 1;
		break;
	default:
		return -EINVAL;
	}

	gm_gpio_irq_rmw(chip, GM_GPIO010_INTRTRIGGER, bit, level);
	gm_gpio_irq_rmw(chip, GM_GPIO010_INTRBOTH, bit, both);
	gm_gpio_irq_rmw(chip, GM_GPIO010_INTRRISENEG, bit, low);
	__irq_set_handler_locked(d->irq, level ? handle_level_irq : handle_edge_irq);

	/* a trigger change can latch a stale event */
	iowrite32(bit, chip->start_vaddr + GM_GPIO010_INTRCLEAR);
	return 0;
}

static struct irq_chip gm_gpio_irq_chip = {
	.name		= "ftgpio010",
	.irq_ack	= gm_gpio_irq_ack,
	.irq_mask	= gm_gpio_irq_mask,
	.irq_unmask	= gm_gpio_irq_unmask,
	.irq_set_type	= gm_gpio_irq_set_type,
};

static void gm_gpio_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	struct gm_gpio_chip *chip = irq_desc_get_handler_data(desc);
	struct irq_chip *parent = irq_desc_get_chip(desc);
	u32 pending;

	chained_irq_enter(parent, desc);
	pending = ioread32(chip->start_vaddr + GM_GPIO010_INTRMASKEDSTATE);
	while (pending) {
		int pin = __ffs(pending);

		pending &= ~(1 << pin);
		generic_handle_irq(chip->irq_base + pin);
	}
	chained_irq_exit(parent, desc);
}

static void gm_gpio_irq_init(struct gm_gpio_chip *chip)
{
	int pin;

	if (chip->id < 0 ||
	    (chip->id + 1) * GM_GPIO_NR_PIN_PER_PORT > GM_GPIO_NR_IRQS)
		return;

	chip->irq_base = GM_GPIO_IRQ_BASE + chip->id * GM_GPIO_NR_PIN_PER_PORT;

	for (pin = 0; pin < GM_GPIO_NR_PIN_PER_PORT; pin++) {
		int irq = chip->irq_base + pin;

		irq_set_chip_and_handler(irq, &gm_gpio_irq_chip,
					 handle_edge_irq);
		irq_set_chip_data(irq, chip);
		set_irq_flags(irq, IRQF_VALID);
	}

	irq_set_handler_data(chip->irq, chip);
	irq_set_chained_handler(chip->irq, gm_gpio_irq_handler);

	printk(KERN_INFO "%s: pin interrupts %d-%d, debounce clock %u Hz\n",
	       chip->name, chip->irq_base,
	       chip->irq_base + GM_GPIO_NR_PIN_PER_PORT - 1, APB_CLK_IN);
}
#else
static inline void gm_gpio_irq_init(struct gm_gpio_chip *chip)
{
}
#endif /* GM_GPIO_IRQ_BASE */

/*
 *  init chip->gc
 */
static void gm_gpio_setup(struct gm_gpio_chip *chip)
{
	struct gpio_chip *gc = NULL;

	gc = &chip->gc;

	gc->direction_input = gm_gpio_direction_input;
	gc->direction_output = gm_gpio_direction_output;
	gc->get = gm_gpio_get_value;
	gc->set = gm_gpio_set_value;
	gc->to_irq = gm_gpio_to_irq;
	gc->set_debounce = gm_gpio_set_debounce;
	gc->int_clr = gm_int_clr;
	gc->int_setup = gm_int_setup;
	gc->int_enable = gm_int_enable;
	gc->int_disable = gm_int_disable;
	gc->int_mask = gm_int_mask;
	gc->int_unmask = gm_int_unmask;
	gc->int_check = gm_int_check;
	gc->can_sleep = 0;
	gc->base = chip->id * GM_GPIO_NR_PIN_PER_PORT;
	gc->ngpio = GM_GPIO_NR_PIN_PER_PORT;
	gc->label = chip->name;
	gc->dev = &chip->platform_dev->dev;
	gc->owner = THIS_MODULE;
}

#if GM_GPIO010_DEBUG
static void dump_info(const struct gm_gpio_chip *chip)
{
	printk("\n************************************************\n");
	printk("Dump GM GPIO related info\n");
	printk("resource->start = %x\n", chip->res->start);
	printk("resource->end = %x\n", chip->res->end);
	printk("resource->name = %s\n", chip->res->name);
	printk("chip->name = %s\n", chip->name);
	printk("chip->start_vaddr = %x\n", (unsigned)chip->start_vaddr);
	printk("chip->irq = %d\n", chip->irq);
	printk("gc->can_sleep = %d\n", chip->gc.can_sleep);
	printk("gc->base = %d\n", chip->gc.base);
	printk("gc->ngpio = %d\n", chip->gc.ngpio);
	printk("gc->label = %s\n", chip->gc.label);
	printk("************************************************\n");
}

static void dump_GPIO010_reg(const struct gm_gpio_chip *chip)
{
	printk("GM_GPIO010_PINDIR 	== %x\n",
	       ioread32(chip->start_vaddr + GM_GPIO010_PINDIR));
	printk("GM_GPIO010_GPIODATAOUT == %x\n",
	       ioread32(chip->start_vaddr + GM_GPIO010_GPIODATAOUT));
	printk("GM_GPIO010_GPIODATAIN  == %x\n",
	       ioread32(chip->start_vaddr + GM_GPIO010_GPIODATAIN));
}
#endif //end of GM_GPIO010_DEBUG

#if GM_GPIO010_TEST
static void test_dedicate_gpio_request(struct gpio_chip *gc)
{
	gpio_request(0, "gpio0");
	gpio_request(1, "gpio1");
	gpio_request(2, "gpio2");
	gpio_request(3, "gpio3");
}

static void test_set_dedicate_gpio_high(void)
{
	gpio_direction_output(0, 1);
	gpio_direction_output(1, 1);
	gpio_direction_output(2, 1);
	gpio_direction_output(3, 1);

	if (gpio_get_value(0) == 0) {
		printk("%s fail: gpio 0 fail\n", __FUNCTION__);
	}

	if (gpio_get_value(1) == 0) {
		printk("%s fail: gpio 1 fail\n", __FUNCTION__);
	}

	if (gpio_get_value(2) == 0) {
		printk("%s fail: gpio 2 fail\n", __FUNCTION__);
	}

	if (gpio_get_value(3) == 0) {
		printk("%s fail: gpio 3 fail\n", __FUNCTION__);
	}

	printk("GPIO test output HIGH OK\n");
}

static void test_set_dedicate_gpio_low(void)
{
	gpio_direction_output(0, 0);
	gpio_direction_output(1, 0);
	gpio_direction_output(2, 0);
	gpio_direction_output(3, 0);

	if (gpio_get_value(0) == 1) {
		printk("%s fail: gpio 0 fail\n", __FUNCTION__);
	}

	if (gpio_get_value(1) == 1) {
		printk("%s fail: gpio 1 fail\n", __FUNCTION__);
	}

	if (gpio_get_value(2) == 1) {
		printk("%s fail: gpio 2 fail\n", __FUNCTION__);
	}

	if (gpio_get_value(3) == 1) {
		printk("%s fail: gpio 3 fail\n", __FUNCTION__);
	}

	printk("GPIO test output LOW OK\n");
}

static void test_set_dedicate_gpio_input(void)
{
	gpio_direction_input(0);
	gpio_direction_input(1);
	gpio_direction_input(2);
	gpio_direction_input(3);

	//test input setting should investigate the CKT
}

static irqreturn_t gm_test_gpio_isr(int irq, void *dev_id)
{
	if (gpio_interrupt_check(0))
		printk("gpio isr\n");
	gpio_interrupt_clear(0);

	return IRQ_HANDLED;
}

struct fake_dev {
	int foo;
};

static void test_gpio_interrupt(struct fake_dev *dev)
{
	struct gpio_interrupt_mode mode = {
		.trigger_method = GPIO_INT_TRIGGER_EDGE,
		.trigger_edge_nr = GPIO_INT_SINGLE_EDGE,
		.trigger_rise_neg = GPIO_INT_FALLING
	};

	gpio_direction_input(0);
	gpio_interrupt_enable(0);
	gpio_interrupt_setup(0, &mode);

	if (request_irq
	    (gpio_to_irq(0), gm_test_gpio_isr, IRQF_SHARED, "gpio-0",
	     dev) < 0) {
		printk("%s fail: request_irq not OK!!\n", __FUNCTION__);
	}
}
#endif //end of GM_GPIO010_TEST

static int ftgpio010_probe(struct platform_device *pdev)
{
	struct gm_gpio_chip *chip = NULL;
	struct resource *res = NULL;
	int ret = -1;
	int irq = -1;

	chip = kzalloc(sizeof(struct gm_gpio_chip), GFP_KERNEL);
	if (unlikely(chip == NULL)) {
		printk("%s fail: could not allocate memory\n", __FUNCTION__);
		ret = -ENOMEM;
		goto err_alloc;
	}
	//get the gpio controller's memory I/O, this will be pyhsical address
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(res == NULL)) {
		printk("%s fail: can't get resource\n", __FUNCTION__);
		return -ENXIO;
	}

	if (unlikely((irq = platform_get_irq(pdev, 0)) < 0)) {
		printk("%s fail: can't get irq\n", __FUNCTION__);
		return irq;
	}

	chip->irq = irq;

	//optional, but this is better for future debug
	chip->res =
	    request_mem_region(res->start, (res->end - res->start + 1),
			       dev_name(&pdev->dev));
	if (unlikely(chip->res == NULL)) {
		printk("%s fail: could not reserve memory region\n",
		       __FUNCTION__);
		ret = -ENOMEM;
		goto err_req_mem;
	}
	//map pyhsical address to virtual address for program use easier
	chip->start_vaddr =
	    ioremap_nocache(res->start, (res->end - res->start + 1));
	if (unlikely(chip->start_vaddr == NULL)) {
		printk("%s fail: counld not do ioremap\n", __FUNCTION__);
		ret = -ENOMEM;
		goto err_ioremap;
	}

	chip->name = kstrdup(dev_name(&pdev->dev), GFP_KERNEL);
	if (unlikely(chip->name == NULL)) {
		printk("%s fail: kstrdup not work\n", __FUNCTION__);
		ret = -ENOMEM;
		goto err_kstrdup;
	}

	chip->platform_dev = pdev;
	chip->id = pdev->id;
	spin_lock_init(&chip->lock);

	//bind related fields to gpio_chip
	gm_gpio_setup(chip);

	//bind to platform, for future remove easier
	platform_set_drvdata(pdev, chip);

	ret = gpiochip_add(&chip->gc);
	if (ret) {
		printk("%s fail: gpiochip_add not OK\n", __FUNCTION__);
		goto err_gpioadd;
	}

	gm_gpio_irq_init(chip);

	printk(KERN_INFO "probe %s OK, at 0x%p\n", chip->name, chip->start_vaddr);

#if GM_GPIO010_DEBUG
	dump_info(chip);
#endif
#if GM_GPIO010_TEST
	test_dedicate_gpio_request(&chip->gc);
	test_set_dedicate_gpio_high();
#if GM_GPIO010_DEBUG
	dump_GPIO010_reg(chip);
#endif
	test_set_dedicate_gpio_low();
#if GM_GPIO010_DEBUG
	dump_GPIO010_reg(chip);
#endif
	test_set_dedicate_gpio_input();
#if GM_GPIO010_DEBUG
	dump_GPIO010_reg(chip);
#endif
	{
		struct fake_dev dev;
		test_gpio_interrupt(&dev);
	}
#if GM_GPIO010_DEBUG
	dump_GPIO010_reg(chip);
#endif
#endif //end of GM_GPIO010_TEST
	return ret;

//god... don't be here... @_@
err_gpioadd:
	kfree(chip->name);
err_kstrdup:
	iounmap(chip->start_vaddr);
err_ioremap:
	release_resource(res);
err_req_mem:
	kfree(chip);
err_alloc:
	return ret;
}

static int __devexit ftgpio010_remove(struct platform_device *pdev)
{
	struct gm_gpio_chip *chip = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	iounmap(chip->start_vaddr);
	release_resource(chip->res);
	kfree(chip->name);
	kfree(chip);

	return 0;
}

static struct platform_driver ftgpio010_driver = {
	.probe = ftgpio010_probe,
	.remove = __devexit_p(ftgpio010_remove),
	.driver = {
		   .name = "ftgpio010",
		   .owner = THIS_MODULE,
		   },
};

/******************************************************************************
 * initialization / finalization
 *****************************************************************************/
static int __init ftgpio010_init(void)
{
	return platform_driver_register(&ftgpio010_driver);
}

static void __exit ftgpio010_exit(void)
{
	platform_driver_unregister(&ftgpio010_driver);
}

module_init(ftgpio010_init);
module_exit(ftgpio010_exit);

MODULE_AUTHOR("Mars Cheng <mars_ch@faraday-tech.com>");
MODULE_LICENSE("GPL");
