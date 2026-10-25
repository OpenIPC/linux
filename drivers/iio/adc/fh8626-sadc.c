// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 eight-channel 12-bit SAR ADC
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/iio/iio.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#define FH_SADC_DRIVER_NAME		"fh8626-sadc"

#define FH_SADC_REG_MODE		0x00
/* Write 1 for a single conversion. */
#define FH_SADC_REG_START		0x04
#define FH_SADC_REG_EN			0x08
#define FH_SADC_REG_SCAN		0x0c
#define FH_SADC_REG_CFG			0x10 /* LPC / Glitch Enable */
#define FH_SADC_REG_POWER		0x14 /* Scan Delta / Power Count */
/* Channel Act Bit & Continuous Config */
#define FH_SADC_REG_CH_CFG		0x18
#define FH_SADC_REG_CONT_TIME		0x1c
#define FH_SADC_REG_GLITCH_TIME		0x20
#define FH_SADC_REG_TRIGGER		0x24 /* 1: start, 0: stop */
#define FH_SADC_REG_INT_STATUS		0x28 /* W1C */
#define FH_SADC_REG_DATA(_ch)		(0x2c + (((_ch) / 2) * 4))
#define FH_SADC_REG_FILTER_CFG		0x54
#define FH_SADC_REG_FILTER_MASK		0x58
#define FH_SADC_REG_CONT_STATUS		0x5c

#define FH_SADC_EN_GLOBAL		BIT(12)
#define FH_SADC_EN_CH(_ch)		BIT(_ch)
#define FH_SADC_CH_CFG_ACT_MASK		GENMASK(19, 8)
#define FH_SADC_FILTER_CFG_DEFAULT	0x00020202

#define FH_SADC_INT_SINGLE_DONE		BIT(0)
#define FH_SADC_INT_CONT_DONE		BIT(16)
#define FH_SADC_INT_GLITCH		BIT(20)
#define FH_SADC_INT_HIT_DATA_MASK	GENMASK(31, 24)

#define FH_SADC_DATA_MASK		GENMASK(11, 0)
#define FH_SADC_NUM_CHANNELS		8
#define FH_SADC_TIMEOUT			msecs_to_jiffies(100)
/* Full scale (4095 counts) corresponds to 3300 mV. */
#define FH_SADC_SCALE_NUM		3300
#define FH_SADC_SCALE_DEN		4095

struct fh_sadc {
	struct device		*dev;
	void __iomem		*regs;
	int			irq;
	struct clk		*clk;
	struct completion	completion;
	/* Serialises single conversions; the converter has one result latch. */
	struct mutex		lock;
};

static irqreturn_t fh_sadc_isr(int irq, void *dev_id)
{
	struct fh_sadc *priv = dev_id;
	u32 status = readl_relaxed(priv->regs + FH_SADC_REG_INT_STATUS);

	if (status & FH_SADC_INT_SINGLE_DONE) {
		writel_relaxed(0, priv->regs + FH_SADC_REG_TRIGGER);
		writel_relaxed(status, priv->regs + FH_SADC_REG_INT_STATUS);
		complete(&priv->completion);
		return IRQ_HANDLED;
	}

	if (status) {
		writel_relaxed(status, priv->regs + FH_SADC_REG_INT_STATUS);
		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

static int fh_sadc_read_raw(struct iio_dev *indio_dev,
			    struct iio_chan_spec const *chan,
			    int *val, int *val2, long mask)
{
	struct fh_sadc *priv = iio_priv(indio_dev);
	u32 data;
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		mutex_lock(&priv->lock);

		writel_relaxed(FH_SADC_EN_GLOBAL | FH_SADC_EN_CH(chan->channel),
			       priv->regs + FH_SADC_REG_EN);

		writel_relaxed(FH_SADC_FILTER_CFG_DEFAULT,
			       priv->regs + FH_SADC_REG_FILTER_CFG);
		writel_relaxed(0xffffffff,
			       priv->regs + FH_SADC_REG_FILTER_MASK);

		/*
		 * Drain late completions from a timed-out conversion before
		 * rearming.
		 */
		writel_relaxed(0xffffffff, priv->regs + FH_SADC_REG_INT_STATUS);
		synchronize_irq(priv->irq);
		reinit_completion(&priv->completion);

		writel(1, priv->regs + FH_SADC_REG_TRIGGER);
		writel(1, priv->regs + FH_SADC_REG_START);

		ret = wait_for_completion_timeout(&priv->completion,
						  FH_SADC_TIMEOUT);
		if (!ret) {
			writel_relaxed(0, priv->regs + FH_SADC_REG_TRIGGER);
			writel_relaxed(0, priv->regs + FH_SADC_REG_EN);
			mutex_unlock(&priv->lock);
			dev_err(priv->dev,
				"conversion timed out on channel %d\n",
				chan->channel);
			return -ETIMEDOUT;
		}

		/*
		 * Each data register holds two channels: the even channel in
		 * bits [31:16] and the odd channel in bits [15:0].
		 */
		data = readl_relaxed(priv->regs +
				     FH_SADC_REG_DATA(chan->channel));
		if (chan->channel & 1)
			*val = data & FH_SADC_DATA_MASK;
		else
			*val = (data >> 16) & FH_SADC_DATA_MASK;

		writel_relaxed(0, priv->regs + FH_SADC_REG_EN);

		mutex_unlock(&priv->lock);
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		*val = FH_SADC_SCALE_NUM;
		*val2 = FH_SADC_SCALE_DEN;
		return IIO_VAL_FRACTIONAL;

	default:
		return -EINVAL;
	}
}

static const struct iio_info fh_sadc_iio_info = {
	.read_raw = fh_sadc_read_raw,
};

#define FH_SADC_CHANNEL(_chan) {				\
	.type = IIO_VOLTAGE,					\
	.indexed = 1,						\
	.channel = _chan,					\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),	\
}

static const struct iio_chan_spec fh_sadc_channels[] = {
	FH_SADC_CHANNEL(0),
	FH_SADC_CHANNEL(1),
	FH_SADC_CHANNEL(2),
	FH_SADC_CHANNEL(3),
	FH_SADC_CHANNEL(4),
	FH_SADC_CHANNEL(5),
	FH_SADC_CHANNEL(6),
	FH_SADC_CHANNEL(7),
};

static void fh_sadc_power_off(void *data)
{
	struct fh_sadc *priv = data;

	writel_relaxed(0, priv->regs + FH_SADC_REG_TRIGGER);
	writel_relaxed(0, priv->regs + FH_SADC_REG_EN);
	writel(0xffffffff, priv->regs + FH_SADC_REG_INT_STATUS);
	clk_disable_unprepare(priv->clk);
}

static int fh_sadc_probe(struct platform_device *pdev)
{
	struct iio_dev *indio_dev;
	struct fh_sadc *priv;
	struct resource *res;
	int ret;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*priv));
	if (!indio_dev)
		return -ENOMEM;

	priv = iio_priv(indio_dev);
	priv->dev = &pdev->dev;
	init_completion(&priv->completion);
	mutex_init(&priv->lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->regs))
		return PTR_ERR(priv->regs);

	priv->irq = platform_get_irq(pdev, 0);
	if (priv->irq < 0) {
		if (priv->irq != -EPROBE_DEFER)
			dev_err(&pdev->dev, "failed to get IRQ: %d\n",
				priv->irq);
		return priv->irq;
	}

	priv->clk = devm_clk_get(&pdev->dev, "sadc");
	if (IS_ERR(priv->clk)) {
		ret = PTR_ERR(priv->clk);
		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev, "failed to get clock: %d\n", ret);
		return ret;
	}
	ret = clk_prepare_enable(priv->clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to enable clock: %d\n", ret);
		return ret;
	}
	/* Registered before the IRQ so it runs after the IRQ is released. */
	ret = devm_add_action_or_reset(&pdev->dev, fh_sadc_power_off, priv);
	if (ret)
		return ret;

	/* Bits [19:8] enable channel activity. */
	writel_relaxed(readl_relaxed(priv->regs + FH_SADC_REG_CH_CFG) |
		       FH_SADC_CH_CFG_ACT_MASK,
		       priv->regs + FH_SADC_REG_CH_CFG);
	writel_relaxed(FH_SADC_EN_GLOBAL, priv->regs + FH_SADC_REG_EN);
	writel_relaxed(FH_SADC_FILTER_CFG_DEFAULT,
		       priv->regs + FH_SADC_REG_FILTER_CFG);
	writel_relaxed(0xffffffff, priv->regs + FH_SADC_REG_FILTER_MASK);
	writel(0xffffffff, priv->regs + FH_SADC_REG_INT_STATUS);

	ret = devm_request_irq(&pdev->dev, priv->irq, fh_sadc_isr,
			       0, dev_name(&pdev->dev), priv);
	if (ret) {
		dev_err(&pdev->dev, "failed to request IRQ %d: %d\n",
			priv->irq, ret);
		return ret;
	}

	indio_dev->name = FH_SADC_DRIVER_NAME;
	indio_dev->dev.parent = &pdev->dev;
	indio_dev->dev.of_node = pdev->dev.of_node;
	indio_dev->info = &fh_sadc_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = fh_sadc_channels;
	indio_dev->num_channels = ARRAY_SIZE(fh_sadc_channels);

	return devm_iio_device_register(&pdev->dev, indio_dev);
}

static const struct of_device_id fh_sadc_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-adc" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh_sadc_of_match);

static struct platform_driver fh_sadc_driver = {
	.probe		= fh_sadc_probe,
	.driver		= {
		.name		= FH_SADC_DRIVER_NAME,
		.of_match_table	= fh_sadc_of_match,
	},
};
module_platform_driver(fh_sadc_driver);

MODULE_AUTHOR("Difan Zhang");
MODULE_DESCRIPTION("Fullhan FH8626 SAR ADC IIO Driver");
MODULE_LICENSE("GPL v2");
