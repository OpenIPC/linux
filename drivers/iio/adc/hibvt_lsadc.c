/*
 * Hisilicon BVT Low Speed (LS) A/D Converter
 * Copyright (C) 2018 HiSilicon Technologies Co., Ltd.
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
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/regulator/consumer.h>
#include <linux/iio/iio.h>

/* hisilicon bvt adc registers definitions */
#define HIBVT_LSADC_CONFIG		0x00
#define HIBVT_CONFIG_DEGLITCH		BIT(17)
#define HIBVT_CONFIG_RESET		BIT(15)
#define HIBVT_CONFIG_MODE		BIT(13)
#define HIBVT_CONFIG_CHN3		BIT(11)
#define HIBVT_CONFIG_CHN2		BIT(10)
#define HIBVT_CONFIG_CHN1		BIT(9)
#define HIBVT_CONFIG_CHN0		BIT(8)

#define HIBVT_LSADC_TIMESCAN	0x08
#define HIBVT_LSADC_INTEN		0x10
#define HIBVT_LSADC_INTSTATUS	0x14
#define HIBVT_LSADC_INTCLR		0x18
#define HIBVT_LSADC_START		0x1C
#define HIBVT_LSADC_STOP		0x20
#define HIBVT_LSADC_ACTBIT		0x24
#define HIBVT_LSADC_CHNDATA		0x2C

#define HIBVT_LSADC_CON_EN		(1u << 0)
#define HIBVT_LSADC_CON_DEN		(0u << 0)

#define HIBVT_LSADC_NUM_BITS	10
#define HIBVT_LSADC_CHN_MASK	0xF

/* fix clk:3000000, default tscan set 10ms */
#define HIBVT_LSADC_TSCAN_MS	(10*3000)

#define HIBVT_LSADC_TIMEOUT		msecs_to_jiffies(100)

/* default voltage scale for every channel <mv> */
static int g_hibvt_lsadc_voltage[] = {
	1800, 1800, 1800, 1800
};

struct hibvt_lsadc {
	void __iomem		*regs;
	struct completion	completion;
	struct reset_control	*reset;
	const struct hibvt_lsadc_data	*data;
	unsigned int		cur_chn;
	unsigned int		value;
};

struct hibvt_lsadc_data {
	int				num_bits;
	const struct iio_chan_spec	*channels;
	int				num_channels;

	void (*clear_irq)(struct hibvt_lsadc *info, int mask);
	void (*start_conv)(struct hibvt_lsadc *info);
	void (*stop_conv)(struct hibvt_lsadc *info);
};

static int hibvt_lsadc_read_raw(struct iio_dev *indio_dev,
				    struct iio_chan_spec const *chan,
				    int *val, int *val2, long mask)
{
	struct hibvt_lsadc *info = iio_priv(indio_dev);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		mutex_lock(&indio_dev->mlock);

		reinit_completion(&info->completion);

		/* Select the channel to be used */
		info->cur_chn = chan->channel;

		if (info->data->start_conv)
			info->data->start_conv(info);

		if (!wait_for_completion_timeout(&info->completion,
							HIBVT_LSADC_TIMEOUT)) {
			if (info->data->stop_conv)
				info->data->stop_conv(info);
			mutex_unlock(&indio_dev->mlock);
			return -ETIMEDOUT;
		}

		*val = info->value;
		mutex_unlock(&indio_dev->mlock);
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_SCALE:
		*val = g_hibvt_lsadc_voltage[chan->channel];
		*val2 = info->data->num_bits;
		return IIO_VAL_FRACTIONAL_LOG2;
	default:
		return -EINVAL;
	}
}

static irqreturn_t hibvt_lsadc_isr(int irq, void *dev_id)
{
	struct hibvt_lsadc *info = (struct hibvt_lsadc *)dev_id;
	int mask;

	mask = readl(info->regs + HIBVT_LSADC_INTSTATUS);
	if ((mask & HIBVT_LSADC_CHN_MASK) == 0)
		return IRQ_NONE;

	/* Clear irq */
	mask &= HIBVT_LSADC_CHN_MASK;
	if (info->data->clear_irq)
		info->data->clear_irq(info, mask);

	/* Read value */
	info->value = readl(info->regs +
		HIBVT_LSADC_CHNDATA + (info->cur_chn << 2));
	info->value &= GENMASK(info->data->num_bits - 1, 0);

	/* stop adc */
	if (info->data->stop_conv)
		info->data->stop_conv(info);

	complete(&info->completion);

	return IRQ_HANDLED;
}

static const struct iio_info hibvt_lsadc_iio_info = {
	.read_raw = hibvt_lsadc_read_raw,
	.driver_module = THIS_MODULE,
};

#define HIBVT_LSADC_CHANNEL(_index, _id) {      \
	.type = IIO_VOLTAGE,                \
	.indexed = 1,						\
	.channel = _index,					\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |  \
			BIT(IIO_CHAN_INFO_SCALE),   \
	.datasheet_name = _id,              \
}

static const struct iio_chan_spec hibvt_lsadc_iio_channels[] = {
	HIBVT_LSADC_CHANNEL(0, "adc0"),
	HIBVT_LSADC_CHANNEL(1, "adc1"),
	HIBVT_LSADC_CHANNEL(2, "adc2"),
	HIBVT_LSADC_CHANNEL(3, "adc3"),
};

static void hibvt_lsadc_clear_irq(struct hibvt_lsadc *info, int mask)
{
	writel(mask, info->regs + HIBVT_LSADC_INTCLR);
}

static void hibvt_lsadc_start_conv(struct hibvt_lsadc *info)
{
	unsigned int con;

	/* set number bit */
	con = GENMASK(info->data->num_bits - 1, 0);
	writel(con, (info->regs + HIBVT_LSADC_ACTBIT));

	/* config */
	con = readl(info->regs + HIBVT_LSADC_CONFIG);
	con &= ~HIBVT_CONFIG_RESET;
	con |= (HIBVT_CONFIG_DEGLITCH | HIBVT_CONFIG_MODE);
	con &= ~(HIBVT_CONFIG_CHN0 | HIBVT_CONFIG_CHN1 | 
		HIBVT_CONFIG_CHN2 | HIBVT_CONFIG_CHN3);
	con |= (HIBVT_CONFIG_CHN0 << info->cur_chn);
	writel(con, (info->regs + HIBVT_LSADC_CONFIG));

	/* set timescan */
	writel(HIBVT_LSADC_TSCAN_MS, (info->regs + HIBVT_LSADC_TIMESCAN));

	/* clear interrupt */
	writel(HIBVT_LSADC_CHN_MASK, info->regs + HIBVT_LSADC_INTCLR);

	/* enable interrupt */
	writel(HIBVT_LSADC_CON_EN, (info->regs + HIBVT_LSADC_INTEN));

	/* start scan */
	writel(HIBVT_LSADC_CON_EN, (info->regs + HIBVT_LSADC_START));
}

static void hibvt_lsadc_stop_conv(struct hibvt_lsadc *info)
{
	/* reset the timescan */
	writel(HIBVT_LSADC_CON_DEN, (info->regs + HIBVT_LSADC_TIMESCAN));

	/* disable interrupt */
	writel(HIBVT_LSADC_CON_DEN, (info->regs + HIBVT_LSADC_INTEN));

	/* stop scan */
	writel(HIBVT_LSADC_CON_EN, (info->regs + HIBVT_LSADC_STOP));
}

static const struct hibvt_lsadc_data lsadc_data = {
	.num_bits = HIBVT_LSADC_NUM_BITS,
	.channels = hibvt_lsadc_iio_channels,
	.num_channels = ARRAY_SIZE(hibvt_lsadc_iio_channels),

	.clear_irq = hibvt_lsadc_clear_irq,
	.start_conv = hibvt_lsadc_start_conv,
	.stop_conv = hibvt_lsadc_stop_conv,
};

static const struct of_device_id hibvt_lsadc_match[] = {
	{
		.compatible = "hisilicon,hi3519av100-lsadc",
		.data = &lsadc_data,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hibvt_lsadc_match);

/* Reset LSADC Controller */
static void hibvt_lsadc_reset_controller(struct reset_control *reset)
{
	reset_control_assert(reset);
	usleep_range(10, 20);
	reset_control_deassert(reset);
}

static int hibvt_lsadc_probe(struct platform_device *pdev)
{
	struct hibvt_lsadc *info = NULL;
	struct device_node *np = pdev->dev.of_node;
	struct iio_dev *indio_dev = NULL;
	struct resource	*mem;
	const struct of_device_id *match;
	int ret;
	int irq;

	if (!np)
		return -ENODEV;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*info));
	if (!indio_dev) {
		dev_err(&pdev->dev, "failed allocating iio device\n");
		return -ENOMEM;
	}
	info = iio_priv(indio_dev);

	match = of_match_device(hibvt_lsadc_match, &pdev->dev);
	info->data = match->data;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	info->regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(info->regs))
		return PTR_ERR(info->regs);

	/*
	 * The reset should be an optional property, as it should work
	 * with old devicetrees as well
	 */
	info->reset = devm_reset_control_get(&pdev->dev, "lsadc-crg");
	if (IS_ERR(info->reset)) {
		ret = PTR_ERR(info->reset);
		if (ret != -ENOENT)
			return ret;

		dev_dbg(&pdev->dev, "no reset control found\n");
		info->reset = NULL;
	}

	init_completion(&info->completion);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return irq;
	}

	ret = devm_request_irq(&pdev->dev, irq, hibvt_lsadc_isr,
			       IRQF_SHARED, dev_name(&pdev->dev), info);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed requesting irq %d\n", irq);
		return ret;
	}

	if (info->reset)
		hibvt_lsadc_reset_controller(info->reset);

	platform_set_drvdata(pdev, indio_dev);

	indio_dev->name = dev_name(&pdev->dev);
	indio_dev->dev.parent = &pdev->dev;
	indio_dev->dev.of_node = pdev->dev.of_node;
	indio_dev->info = &hibvt_lsadc_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE;

	indio_dev->channels = info->data->channels;
	indio_dev->num_channels = info->data->num_channels;

	ret = devm_iio_device_register(&pdev->dev, indio_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed register iio device\n");
		return ret;
	}

	return 0;
}

static struct platform_driver hibvt_lsadc_driver = {
	.probe		= hibvt_lsadc_probe,
	.driver		= {
		.name	= "hibvt-lsadc",
		.of_match_table = hibvt_lsadc_match,
	},
};

module_platform_driver(hibvt_lsadc_driver);

MODULE_AUTHOR("Allen Liu <liurenzhong@hisilicon.com>");
MODULE_DESCRIPTION("hisilicon BVT LSADC driver");
MODULE_LICENSE("GPL v2");
