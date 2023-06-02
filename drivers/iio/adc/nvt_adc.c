/*
 * Copyright (C) 2016 Novatek MicroElectronics Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/thermal.h>

#include <plat/adc_reg.h>
#include <plat/efuse_protected.h>
#include <plat/top.h>

#define DRV_VERSION "1.02.070"

#ifdef CONFIG_NVT_FPGA_EMULATION
#define CALI_VDDADC 3300
#else
#define CALI_VDDADC 2700
#endif
#define CALI_OFFSET 0

#define DEFAULT_THERMAL_LEVEL 259
#define DEFAULT_THERMAL_528_LEVEL 231

#define ADC_CHANNEL(num, id)                    \
	{                           \
		.type = IIO_VOLTAGE,                \
		.indexed = 1,                   \
		.channel = num,                 \
		.address = num,                 \
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),   \
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),\
		.scan_index = num,              \
		.scan_type = {                  \
			.sign = 's',                \
			.realbits = 9,             \
			.storagebits = 9,          \
			.endianness = IIO_CPU,          \
		},                      \
		.datasheet_name = id,			\
	}

static uint nvt_adc_poll_mode = 1;
module_param(nvt_adc_poll_mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(nvt_adc_poll_mode, "1:poll mode, 0:irq mode");

static uint nvt_adc_pow2div = 1;
module_param(nvt_adc_pow2div, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(nvt_adc_pow2div, "adc sample clock pow2 div");

static uint nvt_adc_plusdiv = 0;
module_param(nvt_adc_plusdiv, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(nvt_adc_plusdiv, "adc sample clock fine tune plus div");

struct nvt_adc_state {
	u32 sample_rate;
	struct completion data_complete;
	u32 interrupt_status;
	u32 base;
	struct clk *clk;
	unsigned long trim_value;
	u32 cali_value;
};

static u32 adc_getreg(struct nvt_adc_state *adc, u32 offset)
{
	return nvt_readl(adc->base + offset);
}

static void adc_setreg(struct nvt_adc_state *adc, u32 offset, u32 value)
{
	nvt_writel(value, adc->base + offset);
}

static int adc_chk_enable(struct nvt_adc_state *adc)
{
	union ADC_CTRL_REG ctrl_reg;

	ctrl_reg.reg = adc_getreg(adc, ADC_CTRL_REG_OFS);

	return ctrl_reg.bit.adc_en;
}

static void adc_set_enable(struct nvt_adc_state *adc, int input)
{
	union ADC_CTRL_REG ctrl_reg;

	ctrl_reg.reg = adc_getreg(adc, ADC_CTRL_REG_OFS);
	ctrl_reg.bit.adc_en = input;
	adc_setreg(adc, ADC_CTRL_REG_OFS, ctrl_reg.reg);
}

static void adc_set_default_hw(struct nvt_adc_state *adc)
{
	union ADC_CTRL_REG ctrl_reg;
	union ADC_INTCTRL_REG intctrl_reg;

	intctrl_reg.reg = 0xFF;
	adc_setreg(adc, ADC_INTCTRL_REG_OFS, intctrl_reg.reg);

	ctrl_reg.reg = adc_getreg(adc, ADC_CTRL_REG_OFS);
	ctrl_reg.bit.ain0_mode = 1;
	ctrl_reg.bit.ain1_mode = 1;
	ctrl_reg.bit.ain2_mode = 1;
	ctrl_reg.bit.ain3_mode = 1;
	ctrl_reg.bit.clkdiv = nvt_adc_pow2div;
	ctrl_reg.bit.sampavg = 0;
	ctrl_reg.bit.extsamp_cnt = nvt_adc_plusdiv;
	ctrl_reg.bit.adc_en = 0;
	adc_setreg(adc, ADC_CTRL_REG_OFS, ctrl_reg.reg);
}

static u32 nvt_adc_get_value(struct nvt_adc_state *adc, u8 channel)
{
	u16 adc_data;

	adc_data = adc_getreg(adc, ADC_AIN0_DATA_REG_OFS + channel*4);
	return (adc_data+CALI_OFFSET)*adc->cali_value/511;
}

static int nvt_adc_read_raw(struct iio_dev *iio,
				struct iio_chan_spec const *channel, int *value,
				int *shift, long mask)
{
	struct nvt_adc_state *adc = iio_priv(iio);

	if (!adc_chk_enable(adc))
		adc_set_enable(adc, 1);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if(nvt_adc_poll_mode == 0)
			wait_for_completion(&adc->data_complete);

		*value = nvt_adc_get_value(adc, channel->address);
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		printk("IIO_CHAN_INFO_SCALE\n");
		return IIO_VAL_FRACTIONAL_LOG2;

	default:
		break;
	}

	return -EINVAL;
}

static irqreturn_t nvt_adc_irq(int irq, void *devid)
{
	struct nvt_adc_state *adc = devid;

	adc->interrupt_status = adc_getreg(adc, ADC_STATUS_REG_OFS);

	if (adc->interrupt_status) {
		adc_setreg(adc, ADC_STATUS_REG_OFS, adc->interrupt_status);
		complete(&adc->data_complete);
		return IRQ_HANDLED;
	} else
		return IRQ_NONE;
}

static ssize_t nvt_adc_enable(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t len)
{
	struct iio_dev *iio = dev_to_iio_dev(dev);
	struct nvt_adc_state *adc  = iio_priv(iio);
	unsigned long input;
	int ret;

	ret = kstrtoul(buf, 10, &input);
	if (ret < 0)
		return ret;
	
	adc_set_enable(adc, input);

	return len;
}

static ssize_t nvt_adc_is_enable(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct iio_dev *iio = dev_to_iio_dev(dev);
	struct nvt_adc_state *adc  = iio_priv(iio);

	return sprintf(buf, "%d\n", adc_chk_enable(adc));
}

static ssize_t nvt_adc_get_poll_mode(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	return sprintf(buf, "%d\n", nvt_adc_poll_mode);
}

static u32 nvt_adc_set_frequency(struct nvt_adc_state *adc, u32 frequency)
{
	int ret = 0;
	union ADC_CTRL_REG ctrl_reg;

	if (frequency > 38461) {
		pr_err("\nmax frequency is 38461 Hz, set value is %d\n", frequency);
		ret = -1;
		return ret;
	}

	ctrl_reg.reg = adc_getreg(adc, ADC_CTRL_REG_OFS);

	if (ctrl_reg.bit.adc_en) {
		pr_err("\nplease disable adc first\n");
		ret = -1;
		return ret;
	}

	nvt_adc_pow2div = ( clk_get_rate(adc->clk) / (frequency * 328));

	nvt_adc_plusdiv = ( (clk_get_rate(adc->clk) / (2 * (nvt_adc_pow2div + 1))) / (frequency * 4) - 26);

	adc->sample_rate = (clk_get_rate(adc->clk) / (2 * (nvt_adc_pow2div + 1))) / (4 * (26 + nvt_adc_plusdiv));

	pr_info("\nset frequency is %d\n", adc->sample_rate);

	ctrl_reg.bit.clkdiv = nvt_adc_pow2div;
	ctrl_reg.bit.extsamp_cnt = nvt_adc_plusdiv;
	adc_setreg(adc, ADC_CTRL_REG_OFS, ctrl_reg.reg);

	return ret;
}

static ssize_t nvt_adc_set_in_clk(struct device *dev,
		struct device_attribute *attr,
		const char *buf,
		size_t len)
{
	struct iio_dev *iio = dev_to_iio_dev(dev);
	struct nvt_adc_state *adc  = iio_priv(iio);
	unsigned long input;
	int ret;

	ret = kstrtoul(buf, 10, &input);
	if (ret < 0)
		return ret;

	nvt_adc_set_frequency(adc, input);

	return len;
}

static ssize_t nvt_adc_get_in_clk(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct iio_dev *iio = dev_to_iio_dev(dev);
	struct nvt_adc_state *adc  = iio_priv(iio);

	return sprintf(buf, "%d\n", adc->sample_rate);
}

static const struct iio_chan_spec nvt_adc_channels[] = {
	IIO_CHAN_SOFT_TIMESTAMP(6),
	ADC_CHANNEL(0, "adc0"),
	ADC_CHANNEL(1, "adc1"),
	ADC_CHANNEL(2, "adc2"),
};

/* for consumer drivers */
static struct iio_map nvt_adc_default_maps[] = {
	IIO_MAP("adc0", "nvt-adc0", "adc0"),
	IIO_MAP("adc1", "nvt-adc1", "adc1"),
	IIO_MAP("adc2", "nvt-adc2", "adc2"),
	{},
};

static IIO_DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, nvt_adc_is_enable, nvt_adc_enable, 0);
static IIO_DEVICE_ATTR(poll_mode, S_IRUGO, nvt_adc_get_poll_mode, 0, 0);
static IIO_DEVICE_ATTR(frequency, S_IRUGO | S_IWUSR, nvt_adc_get_in_clk, nvt_adc_set_in_clk, 0);

static struct attribute *nvt_adc_attributes[] = {
	&iio_dev_attr_enable.dev_attr.attr,
	&iio_dev_attr_poll_mode.dev_attr.attr,
	&iio_dev_attr_frequency.dev_attr.attr,
	NULL,
};

static const struct attribute_group nvt_attribute_group = {
	.attrs = nvt_adc_attributes,
};

static const struct iio_info nvt_adc_info = {
	.read_raw = &nvt_adc_read_raw,
	.attrs = &nvt_attribute_group,
};

static void thermal_get_trimdata(struct nvt_adc_state *adc)
{
	u16 data = 0x0;

	if (efuse_readParamOps(EFUSE_THERMAL_TRIM_DATA, &data)) {
		if (nvt_get_chip_id() == CHIP_NA51055)
			adc->trim_value = DEFAULT_THERMAL_LEVEL;
		else
			adc->trim_value = DEFAULT_THERMAL_528_LEVEL;
	} else
		adc->trim_value = data;
}

static int thermal_get_temp(struct thermal_zone_device *thermal, int *temp)
{
	struct nvt_adc_state *adc = thermal->devdata;
	union THERMAL_SENSOR_CONFIGURE_REG thermal_reg;
	union ADC_CTRL_REG ctrl_reg;
	signed long temp_value;

	ctrl_reg.reg = adc_getreg(adc, ADC_CTRL_REG_OFS);
	if (!ctrl_reg.bit.adc_en) {
		ctrl_reg.bit.adc_en = 1;
		adc_setreg(adc, ADC_CTRL_REG_OFS, ctrl_reg.reg);
	}

	thermal_reg.reg = adc_getreg(adc, THERMAL_SENSOR_CONFIGURE_REG_OFS);
	temp_value = thermal_reg.bit.ain_avg_out;

	if (nvt_get_chip_id() == CHIP_NA51055) {
		*temp = (temp_value - adc->trim_value)*13;
		*temp = *temp/10 + 30;
	} else {
		*temp = (temp_value - adc->trim_value)*1319;
		*temp = *temp/1000 + 30;
	}

	return 0;
}

static struct thermal_zone_device_ops ops = {
	.get_temp = thermal_get_temp,
};

#ifdef CONFIG_OF
static const struct of_device_id nvt_adc_of_dt_ids[] = {
	{ .compatible = "nvt,nvt_adc", },
	{},
};

MODULE_DEVICE_TABLE(of, nvt_adc_of_dt_ids);
#endif

static int nvt_adc_probe(struct platform_device *pdev)
{
	struct iio_dev *iio;
	struct nvt_adc_state *adc;
	int err = 0, irq;
	struct resource *res;
	struct thermal_zone_device *thermal_zone = NULL;
#ifdef CONFIG_OF
	const struct of_device_id *of_id;

	of_id = of_match_device(nvt_adc_of_dt_ids, &pdev->dev);
	if (!of_id) {
		dev_err(&pdev->dev, "[NVT ADC] OF not found\n");
		return -EINVAL;
	}

	//pr_info("%s %d: of_id->compatible = %s\n", __func__, __LINE__,  of_id->compatible);
#endif

	adc = kzalloc(sizeof(struct nvt_adc_state), GFP_KERNEL);
	if (!adc)
		return -ENOMEM;

	iio = iio_device_alloc(sizeof(*adc));
	if (!iio)
		return -ENOMEM;

	adc = iio_priv(iio);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("%s: get IO resource fail\n", __func__);
		goto iio_free;
	}

	adc->base = (u32)devm_ioremap_resource(&pdev->dev, res);
	if (!adc->base)
		goto iio_free;

	adc->clk = clk_get(&pdev->dev, dev_name(&pdev->dev));
	if (!IS_ERR(adc->clk)) {
		clk_prepare(adc->clk);
		clk_enable(adc->clk);
	} else {
		pr_err("%s: %s clk not found\n", __func__, dev_name(&pdev->dev));
		goto iio_free;
	}

	if(nvt_adc_poll_mode == 0) {
		irq = platform_get_irq(pdev, 0);
		if (irq < 0) {
			pr_err("%s: get irq fail %d\n", __func__, irq);
			goto iio_free;
		}

		init_completion(&adc->data_complete);

		err = request_irq(irq, nvt_adc_irq, 0, pdev->name, adc);
		if (err < 0) {
		dev_err(&pdev->dev, "failed to request IRQ\n");
			goto iio_free;
		}
	}

	if (of_property_read_u32(pdev->dev.of_node, "cali_value", &adc->cali_value))
		adc->cali_value = CALI_VDDADC;

	adc->sample_rate = (clk_get_rate(adc->clk) / (2 * (nvt_adc_pow2div + 1))) / (4 * (26 + nvt_adc_plusdiv));

	adc_set_default_hw(adc);

	iio->name = "nvt_adc";
	iio->modes = INDIO_DIRECT_MODE;
	iio->info = &nvt_adc_info;
	iio->dev.parent = &pdev->dev;
	iio->dev.of_node = pdev->dev.of_node;
	iio->channels = (void *)&nvt_adc_channels;
	iio->num_channels = sizeof(nvt_adc_channels)/sizeof(nvt_adc_channels[0]);
	err = iio_map_array_register(iio, nvt_adc_default_maps);
	if (err < 0)
		goto iio_free;

	err = iio_device_register(iio);
	if(err)
		goto iio_free;

	pr_info("%s: %s done\n", __func__, dev_name(&pdev->dev));


	thermal_zone = thermal_zone_device_register("nvt_thermal", 0, 0,
				   adc, &ops, NULL, 0, 0);
	if (IS_ERR(thermal_zone)) {
		dev_err(&pdev->dev, "thermal zone device is NULL\n");
		return PTR_ERR(thermal_zone);
	}

	thermal_get_trimdata(adc);

	dev_info(&thermal_zone->device, "Thermal Sensor probe\n");

	platform_set_drvdata(pdev, thermal_zone);

	return 0;

iio_free:
	iio_device_free(iio);

	return err;
}

static int nvt_adc_remove(struct platform_device *pdev)
{
	struct iio_dev *iio = platform_get_drvdata(pdev);;

	iio_device_unregister(iio);
	iio_device_free(iio);

	return 0;
}


static struct platform_driver nvt_adc_driver = {
	.driver = {
		.name = "nvt_adc",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = nvt_adc_of_dt_ids,
#endif
	},
	.probe = nvt_adc_probe,
	.remove = nvt_adc_remove,
};

static int __init nvt_adc_init_driver(void)
{
	return platform_driver_register(&nvt_adc_driver);
}
module_init(nvt_adc_init_driver);

static void __exit nvt_adc_exit_driver(void)
{
	platform_driver_unregister(&nvt_adc_driver);
}
module_exit(nvt_adc_exit_driver);

MODULE_AUTHOR("Novatek");
MODULE_VERSION(DRV_VERSION);
MODULE_DESCRIPTION("NVT ADC driver");
MODULE_LICENSE("GPL v2");
