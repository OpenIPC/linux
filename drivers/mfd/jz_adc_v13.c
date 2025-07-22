/* drivers/mfd/jz_adc.c
 *
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 *      http://www.ingenic.com
 *      Sun Jiwei<jwsun@ingenic.cn>
 * JZ4780 SOC ADC device core
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This driver is designed to control the usage of the ADC block between
 * the touchscreen and any other drivers that may need to use it, such as
 * the hwmon driver.
 */

#include <linux/err.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mfd/core.h>

#include <linux/jz_adc.h>
#include <irq.h>

#define JZ_REG_ADC_ENABLE       0x00
#define JZ_REG_ADC_CTRL         0x08
#define JZ_REG_ADC_STATUS       0x0c

#define JZ_REG_ADC_AUX_BASE	0x10
#define JZ_REG_ADC_CLKDIV	0x20
/*
 *the following registeres is for touchscreen<junyang@ingenic.cn>
 */
#define JZ_REG_ADC_SAME		0x10
#define JZ_REG_ADC_WAIT		0x14
#define JZ_REG_ADC_TCH		0x18
#define JZ_REG_ADC_CMD		0x24
#define CLKDIV		12
#define CLKDIV_US       2
#define CLKDIV_MS       200

enum {
	JZ_ADC_IRQ_AUX = 0,
	JZ_ADC_IRQ_AUX1,
	JZ_ADC_IRQ_AUX2,
	JZ_ADC_IRQ_AUX3,
	JZ_ADC_IRQ_AUX4,
	JZ_ADC_IRQ_AUX5,
	JZ_ADC_IRQ_AUX6,
	JZ_ADC_IRQ_AUX7,
};

#define JZ_ADC_IRQ_NUM	1

#if ( JZ_ADC_IRQ_NUM > SADC_NR_IRQS )
#error "SADC module get error irq number!"
#endif

struct jz_adc {
	struct resource *mem;
	void __iomem *base;

	int irq;
	int irq_base;

	struct clk *clk;
	atomic_t clk_ref;

	spinlock_t lock;
};

static inline void jz_adc_irq_set_masked(struct jz_adc *adc, int irq,
		bool masked)
{
	unsigned long flags;
	uint8_t val;

	irq -= adc->irq_base;

	spin_lock_irqsave(&adc->lock, flags);

	val = readb(adc->base + JZ_REG_ADC_CTRL);
	if (masked) {
		val |= BIT(irq);
	}
	else {
		val &= ~BIT(irq);
	}
	writeb(val, adc->base + JZ_REG_ADC_CTRL);

	spin_unlock_irqrestore(&adc->lock, flags);
}

static void jz_adc_irq_mask(struct irq_data *data)
{
	struct jz_adc *adc = irq_data_get_irq_chip_data(data);
	jz_adc_irq_set_masked(adc, data->irq, true);
}

static void jz_adc_irq_unmask(struct irq_data *data)
{
	struct jz_adc *adc = irq_data_get_irq_chip_data(data);
	jz_adc_irq_set_masked(adc, data->irq, false);
}

static void jz_adc_irq_ack(struct irq_data *data)
{
	struct jz_adc *adc = irq_data_get_irq_chip_data(data);
	unsigned int irq = data->irq - adc->irq_base;
	writeb(BIT(irq), adc->base + JZ_REG_ADC_STATUS);
}

static struct irq_chip jz_adc_irq_chip = {
	.name = "jz-adc",
	.irq_mask = jz_adc_irq_mask,
	.irq_disable = jz_adc_irq_mask,
	.irq_unmask = jz_adc_irq_unmask,
	.irq_ack = jz_adc_irq_ack,
};

static void jz_adc_irq_demux(unsigned int irq, struct irq_desc *desc)
{
	struct jz_adc *adc = irq_desc_get_handler_data(desc);
	uint8_t status;
	unsigned int i;

	status = readb(adc->base + JZ_REG_ADC_STATUS);

	for (i = 0; i < SADC_NR_IRQS; i++) {
		if (status & BIT(i)) {
			generic_handle_irq(adc->irq_base + i);
		}
	}
}

static inline void jz_adc_enable(struct jz_adc *adc)
{
	uint16_t val;

	if (atomic_inc_return(&adc->clk_ref) == 1) {
		val = readw(adc->base + JZ_REG_ADC_ENABLE);
		val &= ~BIT(15);
		writew(val, adc->base + JZ_REG_ADC_ENABLE);
	}
}

static inline void jz_adc_disable(struct jz_adc *adc)
{
	uint16_t val;

	if (atomic_dec_return(&adc->clk_ref) == 0) {
		val = readw(adc->base + JZ_REG_ADC_ENABLE);
		val |= BIT(15);
		writew(val, adc->base + JZ_REG_ADC_ENABLE);
	}
}

static inline void jz_adc_set_enabled(struct jz_adc *adc, int engine,
		bool enabled)
{
	unsigned long flags;
	uint16_t val;

	spin_lock_irqsave(&adc->lock, flags);

	val = readw(adc->base + JZ_REG_ADC_ENABLE);
	if (enabled) {
		val |= BIT(engine);
	}
	else {
		val &= ~BIT(engine);
	}
	writew(val, adc->base + JZ_REG_ADC_ENABLE);

	spin_unlock_irqrestore(&adc->lock, flags);
}

static int jz_adc_cell_enable(struct platform_device *pdev)
{
	struct jz_adc *adc = dev_get_drvdata(pdev->dev.parent);

	/* jz_adc_enable(adc); */
	/* msleep(5); */
	jz_adc_set_enabled(adc, pdev->id, true);

	return 0;
}

static int jz_adc_cell_disable(struct platform_device *pdev)
{
	struct jz_adc *adc = dev_get_drvdata(pdev->dev.parent);

	jz_adc_set_enabled(adc, pdev->id, false);
	/* jz_adc_disable(adc); */

	return 0;
}


int adc_write_reg(struct device *dev ,uint8_t addr_offset,uint32_t mask,uint32_t val)
{
	struct jz_adc *adc = dev_get_drvdata(dev);
	unsigned long flags;
	uint32_t value;
	if(!adc)
		return -ENODEV;
	spin_lock_irqsave(&adc->lock,flags);

	value = readl(adc->base + addr_offset);
	value &= ~mask;
	value |= val;
	writel(value,adc->base + addr_offset);
	spin_unlock_irqrestore(&adc->lock,flags);
	return 0;
}
uint32_t adc_read_reg(struct device *dev,uint8_t addr_offset)
{
	struct jz_adc *adc = dev_get_drvdata(dev);
	unsigned long flags;
	uint32_t ret;
	if(!adc)
		return -ENODEV;
	spin_lock_irqsave(&adc->lock,flags);
	ret = readl(adc->base + addr_offset);
	spin_unlock_irqrestore(&adc->lock,flags);
	return ret;
}


static void jz_adc_clk_div(struct jz_adc *adc, const unsigned char clkdiv,
		const unsigned char clkdiv_us, const unsigned short clkdiv_ms)
{
	unsigned int val;

	val = clkdiv | (clkdiv_us << 8) | (clkdiv_ms << 16);
	writel(val, adc->base + JZ_REG_ADC_CLKDIV);
}

static struct resource jz_aux_resources[] = {
	{
		.start = JZ_ADC_IRQ_AUX,
		.flags = IORESOURCE_IRQ,
	},
	{
		.start	= JZ_REG_ADC_AUX_BASE,
		.end	= JZ_REG_ADC_AUX_BASE + 1,
		.flags	= IORESOURCE_MEM,
	},
};



static struct mfd_cell jz_adc_cells[] = {
	{
		.id = 0,
		.name = "jz-aux",
		.num_resources = ARRAY_SIZE(jz_aux_resources),
		.resources = jz_aux_resources,

		.enable	= jz_adc_cell_enable,
		.disable = jz_adc_cell_disable,
	},
};



static int jz_adc_probe(struct platform_device *pdev)
{
	int ret;
	struct jz_adc *adc;
	struct resource *mem_base;
	int irq;
	unsigned char clkdiv, clkdiv_us;
	unsigned short clkdiv_ms;

	adc = kmalloc(sizeof(*adc), GFP_KERNEL);
	if (!adc) {
		dev_err(&pdev->dev, "Failed to allocate driver structre\n");
		return -ENOMEM;
	}

	adc->irq = platform_get_irq(pdev, 0);
	if (adc->irq < 0) {
		ret = adc->irq;
		dev_err(&pdev->dev, "Failed to get platform irq: %d\n", ret);
		goto err_free;
	}

	adc->irq_base = platform_get_irq(pdev, 1);
	if (adc->irq_base < 0) {
		ret = adc->irq_base;
		dev_err(&pdev->dev, "Failed to get irq base: %d\n", ret);
		goto err_free;
	}

	mem_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_base) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "Failed to get platform mmio resource");
		goto err_free;
	}

	adc->mem = request_mem_region(mem_base->start, JZ_REG_ADC_STATUS,
			pdev->name);
	if (!adc->mem) {
		ret = -EBUSY;
		dev_err(&pdev->dev, "Failed to request mmio memory region\n");
		goto err_free;
	}

	adc->base = ioremap_nocache(adc->mem->start, resource_size(adc->mem));
	if (!adc->base) {
		ret = -EBUSY;
		dev_err(&pdev->dev, "Failed to ioremap mmio memory\n");
		goto err_release_mem_region;
	}

	adc->clk = clk_get(&pdev->dev, "sadc");
	if (IS_ERR(adc->clk)) {
		ret = PTR_ERR(adc->clk);
		dev_err(&pdev->dev, "Failed to get clock: %d\n", ret);
		goto err_iounmap;
	}

	spin_lock_init(&adc->lock);
	atomic_set(&adc->clk_ref, 0);

	platform_set_drvdata(pdev, adc);

	for (irq = adc->irq_base; irq < adc->irq_base + SADC_NR_IRQS; ++irq) {
		irq_set_chip_data(irq, adc);
		irq_set_chip_and_handler(irq, &jz_adc_irq_chip,
				handle_level_irq);
	}

	irq_set_handler_data(adc->irq, adc);
	irq_set_chained_handler(adc->irq, jz_adc_irq_demux);

	clk_enable(adc->clk);

	writew(0x8000, adc->base + JZ_REG_ADC_ENABLE);
	writew(0xffff, adc->base + JZ_REG_ADC_CTRL);
	msleep(5);
	writew(0x0000, adc->base + JZ_REG_ADC_ENABLE);

	clkdiv = CLKDIV - 1;
	clkdiv_us = CLKDIV_US - 1;
	clkdiv_ms = CLKDIV_MS - 1;

	jz_adc_clk_div(adc, clkdiv, clkdiv_us, clkdiv_ms);

	ret = mfd_add_devices(&pdev->dev, 0, jz_adc_cells,
			ARRAY_SIZE(jz_adc_cells), mem_base, adc->irq_base,NULL);
	if (ret < 0) {
		goto err_clk_put;
	}

	printk("jz SADC driver registeres over!\n");

	return 0;

err_clk_put:
	clk_put(adc->clk);
err_iounmap:
	platform_set_drvdata(pdev, NULL);
	iounmap(adc->base);
err_release_mem_region:
	release_mem_region(adc->mem->start, resource_size(adc->mem));
err_free:
	kfree(adc);

	return ret;
}

static int jz_adc_remove(struct platform_device *pdev)
{
	struct jz_adc *adc = platform_get_drvdata(pdev);

	clk_disable(adc->clk);
	mfd_remove_devices(&pdev->dev);

	irq_set_handler_data(adc->irq, NULL);
	irq_set_chained_handler(adc->irq, NULL);

	iounmap(adc->base);
	release_mem_region(adc->mem->start, resource_size(adc->mem));

	clk_put(adc->clk);

	platform_set_drvdata(pdev, NULL);

	kfree(adc);

	return 0;
}

struct platform_driver jz_adc_driver = {
	.probe	= jz_adc_probe,
	.remove	= jz_adc_remove,
	.driver = {
		.name	= "jz-adc",
		.owner	= THIS_MODULE,
	},
};

static int __init jz_adc_init(void)
{
	return platform_driver_register(&jz_adc_driver);
}
module_init(jz_adc_init);

static void __exit jz_adc_exit(void)
{
	platform_driver_unregister(&jz_adc_driver);
}
module_exit(jz_adc_exit);

MODULE_DESCRIPTION("JZ SOC ADC driver");
MODULE_AUTHOR("Guo Xu<xu.guo@ingenic.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:T15-adc");
