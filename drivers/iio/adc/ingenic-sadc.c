// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 auxiliary ADC
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

/*
 * Ingenic SADC Register Offsets (matching vendor jz_adc_v13.c / jz_adc.h)
 */
#define SADC_ENABLE		0x00	/* JZ_REG_ADC_ENABLE / SADC_ADENA_OFFSET */
#define SADC_CTRL		0x08	/* JZ_REG_ADC_CTRL / SADC_ADCTRL_OFFSET */
#define SADC_STATUS		0x0c	/* JZ_REG_ADC_STATUS / SADC_ADSTATE_OFFSET */
#define SADC_AUX_DATA		0x10	/* JZ_REG_ADC_AUX_BASE */
#define SADC_CLKDIV		0x20	/* JZ_REG_ADC_CLKDIV */

/* SADC_ENABLE bits */
#define SADC_SLEEP		BIT(15)	/* Sleep mode control (16-bit access) */
#define SADC_AUXEN		BIT(0)	/* ADENA_AUXEN: AUX channel enable */

/* SADC_STATUS bits */
#define SADC_STATUS_ARDY	BIT(0)	/* ADSTATE_ARDY: AUX data ready flag */

/* SADC_CTRL interrupt mask bits */
#define SADC_CTRL_ARDYM		BIT(0)	/* ADCTRL_ARDYM: AUX ready interrupt mask */
#define SADC_CTRL_MASK_ALL	GENMASK(7, 0)	/* 0xff: mask all 8-bit IRQs */
#define SADC_CTRL_MASK_ALL_16	GENMASK(15, 0)	/* 0xffff: mask all 16-bit IRQs */

/* SADC_AUX_DATA conversion output mask */
#define SADC_DATA_MASK		GENMASK(11, 0)	/* ADADAT_ADATA_MASK: 12-bit ADC data */

/* SADC_CLKDIV clock divider configuration */
#define SADC_CLKDIV_DIV		12
#define SADC_CLKDIV_US		2
#define SADC_CLKDIV_MS		200
#define SADC_CLKDIV_VAL		(((SADC_CLKDIV_MS - 1) << 16) | \
				 ((SADC_CLKDIV_US - 1) << 8) | \
				 (SADC_CLKDIV_DIV - 1))

struct ingenic_sadc {
	void __iomem *base;
	struct clk *clk;
	struct mutex lock;
	struct completion completion;
	int irq;
};

static const struct iio_chan_spec ingenic_sadc_channels[] = {
	{
		.type = IIO_VOLTAGE,
		.indexed = 1,
		.channel = 0,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE),
		.datasheet_name = "AUX0",
	},
};

static irqreturn_t ingenic_sadc_irq(int irq, void *dev_id)
{
	struct ingenic_sadc *sadc = dev_id;
	u8 status = readb(sadc->base + SADC_STATUS);

	if (status & SADC_STATUS_ARDY) {
		writeb(SADC_CTRL_MASK_ALL, sadc->base + SADC_CTRL);
		writeb(SADC_STATUS_ARDY, sadc->base + SADC_STATUS);
		complete(&sadc->completion);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static void ingenic_sadc_mask_and_drain(struct ingenic_sadc *sadc)
{
	writeb(SADC_CTRL_MASK_ALL, sadc->base + SADC_CTRL);
	synchronize_irq(sadc->irq);
	writew(0, sadc->base + SADC_ENABLE);
	writeb(SADC_STATUS_ARDY, sadc->base + SADC_STATUS);
}

static int ingenic_sadc_read_channel(struct ingenic_sadc *sadc, int channel, int *val)
{
	long ret;

	if (channel != 0)
		return -EINVAL;

	ret = mutex_lock_interruptible(&sadc->lock);
	if (ret)
		return ret;

	/* CTRL bits mask interrupts; drain a previous conversion first. */
	ingenic_sadc_mask_and_drain(sadc);
	reinit_completion(&sadc->completion);

	writeb(SADC_CTRL_MASK_ALL & ~SADC_CTRL_ARDYM, sadc->base + SADC_CTRL);
	writew(SADC_AUXEN, sadc->base + SADC_ENABLE);

	ret = wait_for_completion_interruptible_timeout(&sadc->completion,
						       msecs_to_jiffies(500));
	if (ret > 0) {
		*val = readl(sadc->base + SADC_AUX_DATA) & SADC_DATA_MASK;
		ret = 0;
	} else if (!ret) {
		ret = -ETIMEDOUT;
	}

	ingenic_sadc_mask_and_drain(sadc);
	mutex_unlock(&sadc->lock);
	return ret;
}

static int ingenic_sadc_read_raw(struct iio_dev *indio_dev,
				 struct iio_chan_spec const *chan,
				 int *val, int *val2, long mask)
{
	struct ingenic_sadc *sadc = iio_priv(indio_dev);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = ingenic_sadc_read_channel(sadc, chan->channel, val);
		if (ret)
			return ret;
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		*val = 1800;
		*val2 = 12;
		return IIO_VAL_FRACTIONAL_LOG2;

	default:
		return -EINVAL;
	}
}

static const struct iio_info ingenic_sadc_info = {
	.read_raw = ingenic_sadc_read_raw,
};

static void ingenic_sadc_hw_init(struct ingenic_sadc *sadc)
{
	writew(SADC_SLEEP, sadc->base + SADC_ENABLE);
	writew(SADC_CTRL_MASK_ALL_16, sadc->base + SADC_CTRL);
	msleep(5);
	writew(0, sadc->base + SADC_ENABLE);
	writel(SADC_CLKDIV_VAL, sadc->base + SADC_CLKDIV);
	writeb(SADC_STATUS_ARDY, sadc->base + SADC_STATUS);
}

static int ingenic_sadc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct iio_dev *indio_dev;
	struct ingenic_sadc *sadc;
	struct resource *res;
	int ret;

	indio_dev = devm_iio_device_alloc(dev, sizeof(*sadc));
	if (!indio_dev)
		return -ENOMEM;

	sadc = iio_priv(indio_dev);
	mutex_init(&sadc->lock);
	init_completion(&sadc->completion);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sadc->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(sadc->base))
		return PTR_ERR(sadc->base);

	sadc->clk = devm_clk_get(dev, "sadc");
	if (IS_ERR(sadc->clk)) {
		dev_err(dev, "failed to get sadc clock: %ld\n", PTR_ERR(sadc->clk));
		return PTR_ERR(sadc->clk);
	}

	ret = clk_prepare_enable(sadc->clk);
	if (ret) {
		dev_err(dev, "failed to enable sadc clock: %d\n", ret);
		return ret;
	}

	ingenic_sadc_hw_init(sadc);

	sadc->irq = platform_get_irq(pdev, 0);
	if (sadc->irq < 0) {
		dev_err(dev, "failed to get IRQ: %d\n", sadc->irq);
		ret = sadc->irq;
		goto err_clk;
	}

	ret = devm_request_irq(dev, sadc->irq, ingenic_sadc_irq, 0,
			       dev_name(dev), sadc);
	if (ret) {
		dev_err(dev, "failed to request IRQ %d: %d\n", sadc->irq, ret);
		goto err_clk;
	}

	indio_dev->name = "ingenic-sadc";
	indio_dev->info = &ingenic_sadc_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = ingenic_sadc_channels;
	indio_dev->num_channels = ARRAY_SIZE(ingenic_sadc_channels);

	platform_set_drvdata(pdev, indio_dev);

	ret = iio_device_register(indio_dev);
	if (ret) {
		dev_err(dev, "failed to register IIO device: %d\n", ret);
		goto err_clk;
	}

	return 0;

err_clk:
	writew(SADC_SLEEP, sadc->base + SADC_ENABLE);
	clk_disable_unprepare(sadc->clk);
	return ret;
}

static void ingenic_sadc_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);
	struct ingenic_sadc *sadc = iio_priv(indio_dev);

	iio_device_unregister(indio_dev);

	writeb(SADC_CTRL_MASK_ALL, sadc->base + SADC_CTRL);
	writew(SADC_SLEEP, sadc->base + SADC_ENABLE);
	writeb(SADC_STATUS_ARDY, sadc->base + SADC_STATUS);
	clk_disable_unprepare(sadc->clk);
}

static const struct of_device_id ingenic_sadc_of_match[] = {
	{ .compatible = "ingenic,t23-sadc" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ingenic_sadc_of_match);

static struct platform_driver ingenic_sadc_driver = {
	.probe	= ingenic_sadc_probe,
	.remove	= ingenic_sadc_remove,
	.driver	= {
		.name		= "ingenic-sadc",
		.of_match_table	= ingenic_sadc_of_match,
	},
};
module_platform_driver(ingenic_sadc_driver);

MODULE_AUTHOR("Ingenic Semiconductor Co., Ltd.");
MODULE_AUTHOR("Difan Zhang");
MODULE_DESCRIPTION("Ingenic T23 auxiliary ADC");
MODULE_LICENSE("GPL");
