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
#include <asm-generic/gpio.h>
#include <mach/gpio.h>

struct fh_gpio_chip *fh_gpio0, *fh_gpio1;

static inline void __iomem* gpio_to_base(unsigned int gpio)
{
    if (gpio >= 32 && gpio < 64)
    {
        return fh_gpio1->base;
    }
    else if(gpio < 32)
    {
        return fh_gpio0->base;
    }
    else
    {
        pr_err("ERROR: incorrect GPIO num\n");
        return NULL;
    }
}

static int _set_gpio_irq_type(unsigned int gpio, unsigned int type)
{
    u32 int_type, int_polarity;
    u32 bit = gpio % 32;
    void __iomem* base;
    base = gpio_to_base(gpio);

    switch (type & IRQF_TRIGGER_MASK) {
    case IRQ_TYPE_EDGE_BOTH:
        int_type = GPIO_INT_TYPE_EDGE;
        // toggle trigger
        if (FH_GPIO_GetValue((u32)base, bit))
            int_polarity = GPIO_INT_POL_LOW;
        else
            int_polarity = GPIO_INT_POL_HIGH;
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
    FH_GPIO_SetInterruptType((u32)base, bit, int_type);
    FH_GPIO_SetInterruptPolarity((u32)base, bit, int_polarity);
    return 0;
}

int fh_set_gpio_irq(struct gpio_irq_info * info)
{
    void __iomem* base;
    base = gpio_to_base(info->irq_gpio);

    return _set_gpio_irq_type(info->irq_gpio, info->irq_type);
}
EXPORT_SYMBOL(fh_set_gpio_irq);

void fh_irq_enable(unsigned int gpio)
{
    void __iomem* base;
    int gpio_num = gpio % 32;
    base = gpio_to_base(gpio);

    FH_GPIO_EnableInterrupt((u32)base, gpio_num, TRUE);
}
EXPORT_SYMBOL(fh_irq_enable);

void fh_irq_disable(unsigned int gpio)
{
    void __iomem* base;
    int gpio_num = gpio % 32;
    base = gpio_to_base(gpio);

    FH_GPIO_EnableInterrupt((u32)base, gpio_num, FALSE);
}
EXPORT_SYMBOL(fh_irq_disable);

void fh_clear_gpio_irq(int gpio_id)
{
    void __iomem* base;
    int gpio_num = gpio_id % 32;
    base = gpio_to_base(gpio_id);

    FH_GPIO_ClearInterrupt((u32)base, gpio_num);
}
EXPORT_SYMBOL(fh_clear_gpio_irq);


static inline void __iomem* irq_to_controller(struct irq_data* d)
{
    struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d);

    if (likely(d->irq >= NR_INTERNAL_IRQS))
        return fh_gpio->base;
    pr_err("irq num: %d is not a gpio irq!\n", d->irq);
    return 0;
}

static void gpio_irq_ack(struct irq_data* d)
{
    void __iomem* base;
    struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d);
    base = irq_to_controller(d);

    FH_GPIO_ClearInterrupt((u32)base, d->irq - NR_INTERNAL_IRQS - fh_gpio->chip.base);
}

static void gpio_irq_enable(struct irq_data *d)
{
    void __iomem* base;
    struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d);
    base = irq_to_controller(d);

    FH_GPIO_EnableInterrupt((u32)base, d->irq - NR_INTERNAL_IRQS - fh_gpio->chip.base, TRUE);
}

static void gpio_irq_disable(struct irq_data *d)
{
    void __iomem* base;
    struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d);
    base = irq_to_controller(d);

    FH_GPIO_EnableInterrupt((u32)base, d->irq - NR_INTERNAL_IRQS - fh_gpio->chip.base, FALSE);
}

static void gpio_irq_mask(struct irq_data *d)
{
    void __iomem* base;
    struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d);
    base = irq_to_controller(d);

    FH_GPIO_EnableInterruptMask((u32)base, d->irq - NR_INTERNAL_IRQS - fh_gpio->chip.base, TRUE);
}

static void gpio_irq_unmask(struct irq_data *d)
{
    void __iomem* base;
    struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d);
    base = irq_to_controller(d);

    FH_GPIO_EnableInterruptMask((u32)base, d->irq - NR_INTERNAL_IRQS - fh_gpio->chip.base, FALSE);
}

static int gpio_irq_type(struct irq_data *d, unsigned int type)
{
    void __iomem* base;
    base = irq_to_controller(d);

    return _set_gpio_irq_type(d->irq - NR_INTERNAL_IRQS, type);
}

#ifdef CONFIG_PM

static int gpio_irq_set_wake(struct irq_data *d, unsigned value)
{
    struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(d);

    if (unlikely(d->irq >= NR_IRQS))
        return -EINVAL;

    if (value)
        fh_gpio->gpio_wakeups |= (1 << (d->irq - NR_INTERNAL_IRQS - fh_gpio->chip.base));
    else
        fh_gpio->gpio_wakeups &= ~(1 << (d->irq - NR_INTERNAL_IRQS - fh_gpio->chip.base));

    return 0;
}

void fh_gpio_irq_suspend(void)
{
    fh_gpio0->gpio_backups = FH_GPIO_GetEnableInterrupts((u32)fh_gpio0->base);
    fh_gpio1->gpio_backups = FH_GPIO_GetEnableInterrupts((u32)fh_gpio1->base);

    FH_GPIO_SetEnableInterrupts((u32)fh_gpio0->base, fh_gpio0->gpio_wakeups);
    FH_GPIO_SetEnableInterrupts((u32)fh_gpio1->base, fh_gpio1->gpio_wakeups);
}

void fh_gpio_irq_resume(void)
{
    FH_GPIO_SetEnableInterrupts((u32)fh_gpio0->base, fh_gpio0->gpio_backups);
    FH_GPIO_SetEnableInterrupts((u32)fh_gpio1->base, fh_gpio1->gpio_backups);
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
    void __iomem* base = gpio_to_base(gpio);

    if (FH_GPIO_GetValue((u32)base, gpio))
        int_polarity = GPIO_INT_POL_LOW;
    else
        int_polarity = GPIO_INT_POL_HIGH;

    printk(">>>>> do trigger gpio=%d, set polarity=%x\n", offs, int_polarity);
    FH_GPIO_SetInterruptPolarity((u32)base, gpio_num, int_polarity);
}

static inline u32 irq_get_trigger_type(unsigned int irq)
{
    struct irq_data *d = irq_get_irq_data(irq);
    return d ? irqd_get_trigger_type(d) : 0;
}

static void gpio_irq_handler(unsigned int irq, struct irq_desc *desc)
{
    struct irq_data *irqdata = irq_desc_get_irq_data(desc);
    struct irq_chip *irqchip = irq_data_get_irq_chip(irqdata);
    struct fh_gpio_chip *fh_gpio = irq_data_get_irq_chip_data(irqdata);
    u32 irq_status;
    int gpio_num, gpio;

    irq_status = FH_GPIO_GetInterruptStatus((u32)fh_gpio->base);

    if (unlikely(irq_status == 0)) {
        pr_err("gpio irq status is zero.\n");
        return;
    }

    /* temporarily mask (level sensitive) parent IRQ */
    irqchip->irq_mask(irqdata);

    gpio_num = fls(irq_status) - 1;

    FH_GPIO_ClearInterrupt((u32)fh_gpio->base, gpio_num);

    gpio = gpio_num + fh_gpio->chip.base;

    generic_handle_irq(gpio_to_irq(gpio));

    if ((irq_get_trigger_type(gpio_to_irq(gpio)) & IRQ_TYPE_SENSE_MASK)
            == IRQ_TYPE_EDGE_BOTH)
        gpio_toggle_trigger(gpio, gpio_num);

    irqchip->irq_unmask(irqdata);
    /* now it may re-trigger */
}

/*
 * This lock class tells lockdep that GPIO irqs are in a different
 * category than their parents, so it won't report false recursion.
 */
static struct lock_class_key gpio_lock_class;

static void fh_gpio_irq_init(struct platform_device *pdev)
{
    int i, gpio_irq;
    struct fh_gpio_chip *plat_data;

    plat_data = pdev->dev.platform_data;

    for (i = 0; i < 32; i++) {
        gpio_irq = i + NR_INTERNAL_IRQS + 32 * pdev->id;
        irq_set_lockdep_class(gpio_irq, &gpio_lock_class);
        irq_set_chip_and_handler(gpio_irq, &gpio_irqchip, handle_simple_irq);
        set_irq_flags(gpio_irq, IRQF_VALID);
        irq_set_chip_data(gpio_irq, plat_data);
    }

    irq_set_chip_data(plat_data->irq, plat_data);
    irq_set_chained_handler(plat_data->irq, gpio_irq_handler);
    enable_irq_wake(plat_data->irq);
}

static int chip_to_irq(struct gpio_chip *c, unsigned offset)
{
    struct fh_gpio_chip* chip;
    chip = container_of(c, struct fh_gpio_chip, chip);
    return offset + NR_INTERNAL_IRQS + chip->chip.base;
}

static int chip_gpio_get(struct gpio_chip *c, unsigned offset)
{
    u32 bit = offset % 32;
    struct fh_gpio_chip* chip;
    chip = container_of(c, struct fh_gpio_chip, chip);

    if(offset / 32)
	    return FH_GPIOB_GetValue((u32)chip->base, bit);
    else
	    return FH_GPIO_GetValue((u32)chip->base, bit);
}

static void chip_gpio_set(struct gpio_chip *c, unsigned offset, int val)
{
    u32 bit = offset % 32;
    struct fh_gpio_chip* chip;
    chip = container_of(c, struct fh_gpio_chip, chip);
    if(offset / 32)
	    FH_GPIOB_SetValue((u32)chip->base, bit, val);
    else
	    FH_GPIO_SetValue((u32)chip->base, bit, val);
}

static int chip_direction_input(struct gpio_chip *c, unsigned offset)
{
    u32 bit = offset % 32;
    unsigned long flags;
    struct fh_gpio_chip* chip;
    chip = container_of(c, struct fh_gpio_chip, chip);
    spin_lock_irqsave(&chip->lock, flags);
    if(offset / 32)
	    FH_GPIOB_SetDirection((u32)chip->base, bit, GPIO_DIR_INPUT);
    else
	    FH_GPIO_SetDirection((u32)chip->base, bit, GPIO_DIR_INPUT);
    spin_unlock_irqrestore(&chip->lock, flags);

    return 0;
}

static int chip_direction_output(struct gpio_chip *c, unsigned offset, int val)
{
    u32 bit = offset % 32;
    unsigned long flags;
    struct fh_gpio_chip* chip;
    chip = container_of(c, struct fh_gpio_chip, chip);

    spin_lock_irqsave(&chip->lock, flags);
    if(offset / 32)
    {
	    FH_GPIOB_SetDirection((u32)chip->base, bit, GPIO_DIR_OUTPUT);
	    FH_GPIOB_SetValue((u32)chip->base, bit, val);
    }
    else
    {
	    FH_GPIO_SetDirection((u32)chip->base, bit, GPIO_DIR_OUTPUT);
	    FH_GPIO_SetValue((u32)chip->base, bit, val);
    }
    spin_unlock_irqrestore(&chip->lock, flags);

    return 0;
}

void fh_gpio_set(int gpio_id, int value)
{
    __gpio_set_value(gpio_id, value);
}
EXPORT_SYMBOL(fh_gpio_set);

int fh_gpio_get(int gpio_id, int* value)
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

static int __devinit fh_gpio_probe(struct platform_device *pdev)
{
    struct resource *res;
    int err = -EIO;
    struct fh_gpio_chip *plat_data;

    /* There are two ways to get the GPIO base address; one is by
     * fetching it from MSR_LBAR_GPIO, the other is by reading the
     * PCI BAR info.  The latter method is easier (especially across
     * different architectures), so we'll stick with that for now.  If
     * it turns out to be unreliable in the face of crappy BIOSes, we
     * can always go back to using MSRs.. */

    plat_data = pdev->dev.platform_data;
    plat_data->chip.get = chip_gpio_get;
    plat_data->chip.set = chip_gpio_set;
    plat_data->chip.direction_input = chip_direction_input;
    plat_data->chip.direction_output = chip_direction_output;
    plat_data->chip.to_irq = chip_to_irq;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "can't fetch device resource info\n");
        goto done;
    }

    if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
        dev_err(&pdev->dev, "can't request region\n");
        goto done;
    }

    /* set up the driver-specific struct */
    plat_data->base = ioremap(res->start, resource_size(res));

    if(pdev->id)
        fh_gpio1 = plat_data;
    else
        fh_gpio0 = plat_data;

    plat_data->pdev = pdev;
    spin_lock_init(&plat_data->lock);
    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (!res) {
        pr_err("%s: ERROR: getting resource failed"
               "cannot get IORESOURCE_MEM\n", __func__);
        goto release_region;
    }
    plat_data->irq = res->start;

    /* finally, register with the generic GPIO API */
    err = gpiochip_add(&plat_data->chip);
    if (err) {
        pr_err("GPIO support load fail.\n");
        goto release_region;
    }

    fh_gpio_irq_init(pdev);
    pr_debug("GPIO support successfully loaded.\n\tBase Addr: 0x%p\n",
         plat_data->base);

    return 0;

release_region:
    release_region(res->start, resource_size(res));
done:
    return err;
}

static int __devexit fh_gpio_remove(struct platform_device *pdev)
{
    struct resource *r;
    int err;
    struct fh_gpio_chip *plat_data;

    plat_data = pdev->dev.platform_data;
    err = gpiochip_remove(&plat_data->chip);
    if (err) {
        dev_err(&pdev->dev, "unable to remove gpio_chip\n");
        return err;
    }

    iounmap(plat_data->base);

    r = platform_get_resource(pdev, IORESOURCE_IO, 0);
    release_region(r->start, resource_size(r));
    return 0;
}

static struct platform_driver fh_gpio_driver = {
    .driver = {
        .name = GPIO_NAME,
        .owner = THIS_MODULE,
    },
    .probe = fh_gpio_probe,
    .remove = __devexit_p(fh_gpio_remove),
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
MODULE_DESCRIPTION("FH GPIO Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform: FH");
