// SPDX-License-Identifier: GPL-2.0-only
/*
 * IIO ADC driver for AXERA ADC
 *
 * Copyright (C) 2023 AXERA
 *
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/iio/iio.h>
#include <linux/iio/driver.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/export.h>
#include <linux/interrupt.h>

#define AX_ADC_BASE		0x2000000
#define AX_ADC_LEN		0x100000
#define AX_ADC_MA_EN		0x4
#define AX_ADC_MA_POR_EN	0x8
#define AX_ADC_MA_CTRL		0x10
#define AX_ADC_MA_POR_CTRL	0x14
#define AX_ADC_CTRL		0x18
#define AX_ADC_CLK_EN		0x1C	/*clk enable */
#define AX_ADC_CLK_SELECT	0x20	/*clk select */
#define AX_ADC_RSTN		0x24
#define AX_ADC_FILTER_VOL_SEL	0x2C
#define AX_ADC_FILTER_VOL_EN	0x30
#define AX_ADC_MON_EN		0xC8
#define AX_ADC_MON_CH		0xC4
#define AX_ADC_MON_INTERVAL	0xCC
#define AX_ADC_CHAN0_LOW	0xDC
#define AX_ADC_CHAN0_HIGH	0xE0
#define AX_ADC_CHAN1_LOW	0xE4
#define AX_ADC_CHAN1_HIGH	0xE8
#define AX_ADC_CHAN2_LOW	0xEC
#define AX_ADC_CHAN2_HIGH	0xF0
#define AX_ADC_CHAN3_LOW	0xF4
#define AX_ADC_CHAN3_HIGH	0xF8
#define AX_ADC_INT_MASK		0x104
#define AX_ADC_INT_CLR		0x108
#define AX_ADC_INT_STS		0x110

#define AX_ADC_DATA_CHANNEL0	0xA0
#define AX_ADC_DATA_CHANNEL1	0xA4
#define AX_ADC_DATA_CHANNEL2	0xA8
#define AX_ADC_DATA_CHANNEL3	0xAC

#define AX_ADC_FILTER_DATA_CHANNEL0	0xB4
#define AX_ADC_FILTER_DATA_CHANNEL1	0xB8
#define AX_ADC_FILTER_DATA_CHANNEL2	0xBC
#define AX_ADC_FILTER_DATA_CHANNEL3	0xC0
#define AX_ADC_SEL		(0xf << 10 )
#define AX_ADC_EN		BIT(0)

struct ax_adc {
	void __iomem *base;
	struct device *dev;
	spinlock_t lock;
	struct clk *clk;
	int irq;
};
static struct ax_adc *ax_adc = NULL;
#define AX_ADC_CHAN(_idx) {				\
	.type = IIO_VOLTAGE,					\
	.indexed = 1,						\
	.channel = _idx,					\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
}

static const struct iio_chan_spec ax_adc_iio_channels[] = {
	AX_ADC_CHAN(0),
	AX_ADC_CHAN(1),
	AX_ADC_CHAN(2),
	AX_ADC_CHAN(3),
};

static int ax_adc_read_chan(struct ax_adc *adc, unsigned int ch)
{
	int val;
	void __iomem *regs;
	regs = adc->base;
	switch (ch) {
	case 0:
		val = (0x3ff & readl(regs + AX_ADC_FILTER_DATA_CHANNEL0));
		break;
	case 1:
		val = (0x3ff & readl(regs + AX_ADC_FILTER_DATA_CHANNEL1));
		break;
	case 2:
		val = (0x3ff & readl(regs + AX_ADC_FILTER_DATA_CHANNEL2));
		break;
	case 3:
		val = (0x3ff & readl(regs + AX_ADC_FILTER_DATA_CHANNEL3));
		break;
	}
	return val;
}

static int ax_adc_of_xlate(struct iio_dev *iio_dev,
				const struct of_phandle_args *iiospec)
{
	int i;

	if (!iiospec->args_count)
		return -EINVAL;

	for (i = 0; i < iio_dev->num_channels; ++i)
		if (iio_dev->channels[i].channel == iiospec->args[0])
			return i;

	return -EINVAL;
}

static int ax_adc_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	struct ax_adc *adc = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		spin_lock(&adc->lock);
		*val = ax_adc_read_chan(adc, chan->channel);
		spin_unlock(&adc->lock);
		if (*val < 0)
			return *val;

		return IIO_VAL_INT;
	}

	return -EINVAL;
}

static const struct iio_info ax_adc_info = {
	.read_raw = ax_adc_read_raw,
	.of_xlate = ax_adc_of_xlate,
};

static int ax_adc_irq_unmask(struct ax_adc *adc, unsigned int int_status)
{
	void __iomem *regs = adc->base;
	unsigned int val;

	val = readl(regs + AX_ADC_INT_MASK);
	val |= int_status;
	writel(val, regs + AX_ADC_INT_MASK);

	return 0;
}

static int ax_adc_irq_mask(struct ax_adc *adc, unsigned int int_status)
{
	void __iomem *regs = adc->base;
	unsigned int val;

	val = readl(regs + AX_ADC_INT_MASK);
	val &= ~int_status;
	writel(val, regs + AX_ADC_INT_MASK);

	return 0;
}

static int ax_adc_irq_clr(struct ax_adc *adc, unsigned int int_status)
{
	void __iomem *regs = adc->base;

	writel(int_status, regs + AX_ADC_INT_CLR);

	return 0;
}

static irqreturn_t axera_adc_alarm_irq(int irq, void *dev)
{
	struct ax_adc *data = dev;
	unsigned int int_status;
	int i;
	void __iomem *regs = data->base;

	int_status = readl(regs + AX_ADC_INT_STS);
	int_status = int_status & 0xff;
	if (!int_status)
		return IRQ_HANDLED;
	ax_adc_irq_mask(data, int_status);

	for (i = 0; i < 8; i++) {
		if ((1 << i) & int_status) {
			if (i % 2)
				pr_debug("%s: enter,chan%d high irq\n", __func__, i);
			else
				pr_debug("%s: enter,chan%d low irq\n", __func__, i);
		}
	}

	ax_adc_irq_unmask(data, int_status);
	ax_adc_irq_clr(data, int_status);

	return IRQ_HANDLED;
}

int ax_adc_set_chan_int(int chan, int high_or_low, int val)
{
	void __iomem *regs = ax_adc->base;

	if ((chan < 4) && high_or_low) {
		ax_adc_irq_mask(ax_adc, 1 << ((chan * 2) + 1));
		writel(val, regs + (((chan * 2) + 1) * 0x4) + AX_ADC_CHAN0_LOW);
		ax_adc_irq_unmask(ax_adc, 1 << ((chan * 2) + 1));
	} else if ((chan >= 0) && (chan < 4)) {
		ax_adc_irq_mask(ax_adc, 1 << (chan * 2));
		writel(val, regs + ((chan * 2) * 0x4) + AX_ADC_CHAN0_LOW);
		ax_adc_irq_unmask(ax_adc, 1 << (chan * 2));

	} else
		return -1;

	return 0;
}
EXPORT_SYMBOL(ax_adc_set_chan_int);

static int ax_adc_probe(struct platform_device *pdev)
{
	struct iio_dev *indio_dev;
	struct ax_adc *adc;
	int ret;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*adc));
	if (!indio_dev)
		return -ENOMEM;

	platform_set_drvdata(pdev, indio_dev);
	adc = iio_priv(indio_dev);
	adc->dev = &pdev->dev;
	spin_lock_init(&adc->lock);

	adc->base = ioremap(AX_ADC_BASE, AX_ADC_LEN);
	if (IS_ERR(adc->base))
		return PTR_ERR(adc->base);

	indio_dev->name = dev_name(&pdev->dev);
	indio_dev->dev.parent = &pdev->dev;
	indio_dev->dev.of_node = pdev->dev.of_node;
	indio_dev->info = &ax_adc_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = ax_adc_iio_channels;
	indio_dev->num_channels = ARRAY_SIZE(ax_adc_iio_channels);

	adc->irq = platform_get_irq(pdev, 0);
	if (adc->irq < 0) {
		dev_err(&pdev->dev, "ax620e_adc_probe error\n");
		return adc->irq;
	}
	if (adc->irq) {
		ret = devm_request_irq(&pdev->dev, adc->irq, axera_adc_alarm_irq,
				IRQF_SHARED, "axera_adc", adc);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request alarm irq: %d\n", ret);
			return ret;
		}
	}

	ret = iio_device_register(indio_dev);
	if (ret) {
		dev_err(&pdev->dev, "unable to register device\n");
		return ret;
	}
	ax_adc = adc;

	return 0;
}

static int ax_adc_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);

	iio_device_unregister(indio_dev);

	return 0;
}

static const struct of_device_id ax_adc_match[] = {
	{ .compatible = "axera,ax620e-adc" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ax_adc_match);

static struct platform_driver ax_adc_driver = {
	.probe	= ax_adc_probe,
	.remove	= ax_adc_remove,
	.driver	= {
		.name = "ax-adc",
		.of_match_table = ax_adc_match,
	},
};
module_platform_driver(ax_adc_driver);

MODULE_DESCRIPTION("AXERA ADC driver");
MODULE_AUTHOR("AXERA");
MODULE_LICENSE("GPL v2");
