/*
 * Anyka AK39 SAR-ADC1 general-purpose inputs (AIN0, AIN1) as IIO channels.
 *
 * The conversion itself is mach-anycloud's ak_adc1.c, which the ADC keys
 * and the battery driver already share: it powers the converter up, samples
 * one channel and powers it down again, under its own lock, and answers in
 * millivolts against the 3.3 V AVCC. This only gives userspace a way to ask,
 * as in_voltage0_input and in_voltage1_input, so that a camera's photocell
 * (on AIN0 on the HeimVision PE3020-W, where the stock 3.4 kernel offered it
 * as /sys/kernel/ain/ain0) can decide day and night.
 *
 * The AK3918EV300 converts differently -- started, polled, 12 bits against
 * 3.0 V, with an analog enable per input -- and its node says so with
 * "anyka,ak3918ev300-sar-adc" (anycloud_ak3918ev300.dtsi). It offers AIN0
 * only, the one input whose enable is known (ak_adc1.c has the sequence).
 *
 * Pads: on the EV330 / EV300 pin controller AIN0 is not shared with a GPIO,
 * so it needs no pin state; AIN1 shares GPIO 5 (function 5 in
 * ak_ev330_pinctrl.c), and a board that wires something to it gives this
 * node a pinctrl-0 selecting that function, which the driver core applies
 * before probe.
 *
 * Licensed under GPLv2.
 */

#include <linux/iio/iio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#include <mach/ak_adc1.h>

#define AK39_SARADC_CHANNEL(ch) {					\
	.type = IIO_VOLTAGE,						\
	.indexed = 1,							\
	.channel = (ch),						\
	.info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),		\
	.datasheet_name = "AIN" #ch,					\
}

static const struct iio_chan_spec ak39_saradc_channels[] = {
	AK39_SARADC_CHANNEL(AK_ADC1_AD0),
	AK39_SARADC_CHANNEL(AK_ADC1_AD1),
};

static const struct iio_chan_spec ak3918ev300_saradc_channels[] = {
	AK39_SARADC_CHANNEL(AK_ADC1_AD0),
};

struct ak39_saradc_variant {
	const struct iio_chan_spec *channels;
	int num_channels;
	bool ev300;
};

static const struct ak39_saradc_variant ak39ev330_variant = {
	.channels = ak39_saradc_channels,
	.num_channels = ARRAY_SIZE(ak39_saradc_channels),
};

static const struct ak39_saradc_variant ak3918ev300_variant = {
	.channels = ak3918ev300_saradc_channels,
	.num_channels = ARRAY_SIZE(ak3918ev300_saradc_channels),
	.ev300 = true,
};

static int ak39_saradc_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	const struct ak39_saradc_variant **v = iio_priv(indio_dev);
	unsigned int mv;
	int ret;

	if (mask != IIO_CHAN_INFO_PROCESSED)
		return -EINVAL;

	if ((*v)->ev300) {
		ret = adc1_ev300_read(chan->channel, &mv);
		if (ret)
			return ret;
		*val = mv;
		return IIO_VAL_INT;
	}
	*val = adc1_read_channel(chan->channel);
	return IIO_VAL_INT;
}

static const struct iio_info ak39_saradc_info = {
	.read_raw = ak39_saradc_read_raw,
	.driver_module = THIS_MODULE,
};

static int ak39_saradc_probe(struct platform_device *pdev)
{
	const struct ak39_saradc_variant *variant;
	struct iio_dev *indio_dev;

	variant = of_device_get_match_data(&pdev->dev);
	if (!variant)
		return -ENODEV;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(variant));
	if (!indio_dev)
		return -ENOMEM;
	*(const struct ak39_saradc_variant **)iio_priv(indio_dev) = variant;

	if (variant->ev300)
		adc1_ev300_init();
	else
		adc1_init();

	indio_dev->name = dev_name(&pdev->dev);
	indio_dev->dev.parent = &pdev->dev;
	indio_dev->dev.of_node = pdev->dev.of_node;
	indio_dev->info = &ak39_saradc_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = variant->channels;
	indio_dev->num_channels = variant->num_channels;

	return devm_iio_device_register(&pdev->dev, indio_dev);
}

static const struct of_device_id ak39_saradc_of_match[] = {
	{ .compatible = "anyka,ak39ev330-sar-adc", .data = &ak39ev330_variant },
	{ .compatible = "anyka,ak3918ev300-sar-adc",
	  .data = &ak3918ev300_variant },
	{ }
};
MODULE_DEVICE_TABLE(of, ak39_saradc_of_match);

static struct platform_driver ak39_saradc_driver = {
	.probe = ak39_saradc_probe,
	.driver = {
		.name = "ak39-saradc",
		.of_match_table = ak39_saradc_of_match,
	},
};
module_platform_driver(ak39_saradc_driver);

MODULE_DESCRIPTION("Anyka AK39 SAR ADC general-purpose inputs");
MODULE_LICENSE("GPL v2");
