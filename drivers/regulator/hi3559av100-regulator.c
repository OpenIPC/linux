/*
 * Device driver for regulators in hi3559a IC
 *
 * Copyright (c) <2011-2015> HiSilicon Technologies Co., Ltd.
 *              http://www.hisilicon.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#define DRVNAME "hi3559a-regulator"
#define pr_fmt(fmt) DRVNAME ": " fmt

#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <asm/delay.h>

#define HISI_GPU_REGULATOR_MIN		0x19f
#define HISI_GPU_REGULATOR_MAX		0x01
#define PWM_DUTY_MASK			0xffff0000
#define PWM_PERIOD_MASK			0xffff

struct hi3559a_gpu_regulator {
	struct regulator_desc desc;
	struct regulator_dev *regdev;
	void __iomem *base;
	int min_uV;
	int max_uV;
};

static int hi3559a_gpu_regulator_get_voltage(struct regulator_dev *regdev)
{
	struct hi3559a_gpu_regulator *reg = rdev_get_drvdata(regdev);
	u32 duty, volt;
	unsigned int vmax, vmin;

	vmax = reg->max_uV / 1000;
	vmin = reg->min_uV / 1000;

	duty = readl(reg->base);
	duty = (duty >> 16);

	if (duty > HISI_GPU_REGULATOR_MIN || duty < HISI_GPU_REGULATOR_MAX)
		return 0;

	volt = reg->max_uV - (duty * 1000 * (vmax - vmin) + (vmax - vmin)/2) / 416 ;

	return volt;
}

static int hi3559a_gpu_regulator_set_voltage(struct regulator_dev *regdev,
					     int min_uV, int max_uV,
					      unsigned *selector)
{
	struct hi3559a_gpu_regulator *reg = rdev_get_drvdata(regdev);
	u32 duty, value, mask;
	unsigned int vmax, vmin;

	vmax = reg->max_uV / 1000;
	vmin = reg->min_uV / 1000;

	duty = ((vmax - min_uV/1000) * 416 - (vmax - vmin)/2) / (vmax - vmin);

	value = readl(reg->base);
	mask = PWM_DUTY_MASK;
	value &= ~mask;
	value |= (duty << 16);

	writel(value, reg->base);

	udelay(1000);
	return 0;
}

static struct regulator_ops hi3559a_gpu_regulator_ops = {
	.get_voltage = hi3559a_gpu_regulator_get_voltage,
	.set_voltage = hi3559a_gpu_regulator_set_voltage,
};

static int hi3559a_regulator_probe(struct platform_device *pdev)
{
	struct hi3559a_gpu_regulator *reg;
	struct regulator_init_data *init_data;
	struct regulator_config config = {0};
	struct resource *res;
	char *pname;

	reg = devm_kzalloc(&pdev->dev, sizeof(*reg), GFP_KERNEL);
	if (!reg) {
		dev_err(&pdev->dev, "Unable to malloc memory\n");
		return -ENOMEM;
	}

	pname = "base-address";
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, pname);
	if (!res) {
		dev_err(&pdev->dev, "get resource %s failed\n", pname);
		return -ENODEV;
	}

	reg->base = devm_ioremap_nocache(&pdev->dev, res->start,
					 resource_size(res));
	if (!reg->base) {
		devm_kfree(&pdev->dev, reg);
		dev_err(&pdev->dev, "Unable to map '%s'\n", pname);
		return -ENOMEM;
	}

	reg->desc.name = dev_name(&pdev->dev);
	reg->desc.type = REGULATOR_VOLTAGE;
	reg->desc.owner = THIS_MODULE;
	reg->desc.continuous_voltage_range = true;

	init_data = of_get_regulator_init_data(&pdev->dev, pdev->dev.of_node, &reg->desc);
	if (!init_data) {
		devm_iounmap(&pdev->dev, reg->base);
		devm_kfree(&pdev->dev, reg);
		return -EINVAL;
	}

	init_data->constraints.apply_uV = 0;
	reg->min_uV = init_data->constraints.min_uV;
	reg->max_uV = init_data->constraints.max_uV;

	if (reg->min_uV >= reg->max_uV) {
		devm_iounmap(&pdev->dev, reg->base);
		devm_kfree(&pdev->dev, reg);
		dev_err(&pdev->dev, "max_uV(%d) should bigger than min_uV(%d)\n", reg->max_uV, reg->min_uV);
		return -EINVAL;
	}

	reg->desc.ops = &hi3559a_gpu_regulator_ops;

	config.dev = &pdev->dev;
	config.init_data = init_data;
	config.driver_data = reg;
	config.of_node = pdev->dev.of_node;

	reg->regdev = devm_regulator_register(&pdev->dev, &reg->desc, &config);
	if (IS_ERR(reg->regdev)) {
		devm_iounmap(&pdev->dev, reg->base);
		devm_kfree(&pdev->dev, reg);
		dev_err(&pdev->dev, "devm_regulator_register failed\n");
		return PTR_ERR(reg->regdev);
	}

	platform_set_drvdata(pdev, reg);

	return 0;
}

static const struct of_device_id hi3559a_regulator_of_match[] = {
	{ .compatible = "hisilicon,hi3559a-volt", },
	{ }
};

static struct platform_driver hi3559a_regulator_driver = {
	.probe = hi3559a_regulator_probe,
	.driver = {
		.name = DRVNAME,
		.owner = THIS_MODULE,
		.of_match_table = hi3559a_regulator_of_match,
	},
};

module_platform_driver(hi3559a_regulator_driver);

MODULE_AUTHOR("Hisilicon GPU Team");
MODULE_DESCRIPTION("hi3559a regulator");
MODULE_LICENSE("GPLv2");
MODULE_ALIAS("platform:hi3559a-regulator");
