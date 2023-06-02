/*
 * Driver for the Novatek pinmux
 *
 * Copyright (c) 2019, NOVATEK MICROELECTRONIC CORPORATION.  All rights reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "nvt_pinmux.h"

u32 top_reg_addr = 0;
extern u32 top_reg_addr;

static PINMUX_FUNC_ID id_restore = 0x0;
static u32 pinmux_restore = 0x0;

static void nvt_pinmux_show_conflict(struct nvt_pinctrl_info *info)
{
	int i;

	pinmux_parsing(info);
	for (i = 0; i < PIN_FUNC_MAX; i++) {
		pr_err("pinmux %-2d config 0x%x\n", i, info->top_pinmux[i].config);
	}

	panic("###### Conflicted Pinmux Setting ######\n");
}


int nvt_pinmux_capture(PIN_GROUP_CONFIG *pinmux_config, int count)
{
	struct nvt_pinctrl_info *info;
	int i, j, ret = E_OK;

	if (in_interrupt() || in_atomic() || irqs_disabled())
		info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_ATOMIC);
	else
		info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_KERNEL);
	if (!info) {
		pr_err("nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}

	if (top_reg_addr) {
		info->top_base = (void*) top_reg_addr;
		pinmux_parsing(info);

		for (j = 0; j < count; j++) {
			for (i = 0; i < PIN_FUNC_MAX; i++) {
				if (i == pinmux_config[j].pin_function)
					pinmux_config[j].config = info->top_pinmux[i].config;
			}
		}
	} else {
		pr_err("invalid pinmux address\n");
		ret = -ENOMEM;
	}

	kfree(info);

	return ret;
}
EXPORT_SYMBOL(nvt_pinmux_capture);

int nvt_pinmux_update(PIN_GROUP_CONFIG *pinmux_config, int count)
{
	struct nvt_pinctrl_info *info;
	int i, j, ret = E_OK;

	if (in_interrupt() || in_atomic() || irqs_disabled())
		info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_ATOMIC);
	else
		info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_KERNEL);
	if (!info) {
		pr_err("nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}

	if (top_reg_addr) {
		info->top_base = (void*) top_reg_addr;
		pinmux_parsing(info);

		for (j = 0; j < count; j++) {
			for (i = 0; i < PIN_FUNC_MAX; i++) {
				if (i == pinmux_config[j].pin_function)
					info->top_pinmux[i].config = pinmux_config[j].config;
			}
		}

		ret = pinmux_init(info);
		if (ret == E_OK)
			ret = pinmux_set_config(id_restore, pinmux_restore);
	} else {
		pr_err("invalid pinmux address\n");
		ret = -ENOMEM;
	}

	kfree(info);

	return ret;
}
EXPORT_SYMBOL(nvt_pinmux_update);


int pinmux_set_config(PINMUX_FUNC_ID id, u32 pinmux)
{
	struct nvt_pinctrl_info *info;
	int ret;

	if (in_interrupt() || in_atomic() || irqs_disabled())
		info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_ATOMIC);
	else
		info = kmalloc(sizeof(struct nvt_pinctrl_info), GFP_KERNEL);
	if (!info) {
		pr_err("nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}

	if (top_reg_addr) {
		info->top_base = (void*) top_reg_addr;
		ret = pinmux_set_host(info, id, pinmux);

		if (id <= PINMUX_FUNC_ID_LCD2) {
			id_restore = id;
			pinmux_restore = pinmux;
		}
	} else {
		pr_err("invalid pinmux address\n");
		ret = -ENOMEM;
	}

	kfree(info);

	return ret;
}
EXPORT_SYMBOL(pinmux_set_config);

int nvt_pinmux_probe(struct platform_device *pdev)
{
	struct nvt_pinctrl_info *info;
	struct nvt_gpio_info *gpio;
	struct resource *nvt_mem_base;
	struct device_node *top_np;
	u32 value;
	u32 pad_config[4] = {0, 0, 0, 0};
	u32 gpio_config[2] = {0, 0};
	int nr_pinmux = 0, nr_pad = 0, nr_gpio = 0;

	info = devm_kzalloc(&pdev->dev, sizeof(struct nvt_pinctrl_info), GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "nvt pinmux mem alloc fail\n");
		return -ENOMEM;
	}

	for_each_child_of_node(pdev->dev.of_node,  top_np) {
		if (!of_get_property(top_np, "gpio_config", NULL))
			continue;

		nr_gpio++;
	}

	gpio = devm_kzalloc(&pdev->dev, nr_gpio * sizeof(struct nvt_gpio_info), GFP_KERNEL);
	if (!gpio) {
		dev_err(&pdev->dev, "nvt gpio mem alloc fail\n");
		return -ENOMEM;
	}

	nr_gpio = 0;

	nvt_mem_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	info->top_base = devm_ioremap_resource(&pdev->dev, nvt_mem_base);
	if (IS_ERR(info->top_base)) {
		dev_err(&pdev->dev, "fail to get pinmux mem base\n");
		return -ENOMEM;
	}

	top_reg_addr = (u32) info->top_base;

	nvt_mem_base = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	info->pad_base = devm_ioremap_resource(&pdev->dev, nvt_mem_base);
	if (IS_ERR(info->pad_base)) {
		dev_err(&pdev->dev, "fail to get pad mem base\n");
		return -ENOMEM;
	}

	nvt_mem_base = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	info->gpio_base = devm_ioremap_resource(&pdev->dev, nvt_mem_base);
	if (IS_ERR(info->gpio_base)) {
		dev_err(&pdev->dev, "fail to get gpio mem base\n");
		return -ENOMEM;
	}

	for_each_child_of_node(pdev->dev.of_node, top_np) {
		if (!of_property_read_u32(top_np, "pinmux", &value)) {
			info->top_pinmux[nr_pinmux].pin_function = nr_pinmux;
			info->top_pinmux[nr_pinmux].config = value;
			nr_pinmux++;
		}

		if (!of_property_read_u32_array(top_np, "pad_config", pad_config, 4)) {
			info->pad[nr_pad].pad_ds_pin = pad_config[0];
			info->pad[nr_pad].driving = pad_config[1];
			info->pad[nr_pad].pad_gpio_pin = pad_config[2];
			info->pad[nr_pad].direction = pad_config[3];
			nr_pad++;
		}

		if (!of_property_read_u32_array(top_np, "gpio_config", gpio_config, 2)) {
			gpio[nr_gpio].gpio_pin = gpio_config[0];
			gpio[nr_gpio].direction = gpio_config[1];
			nr_gpio++;
		}
	}

	if (nr_pinmux == 0)
		return -ENOMEM;

	pinmux_preset(info);

	pad_preset(info);

	if (pinmux_init(info))
		nvt_pinmux_show_conflict(info);

	if (nr_pad)
		pad_init(info, nr_pad);

	if (nr_gpio)
		gpio_init(gpio, nr_gpio, info);

	if(nvt_pinmux_proc_init())
		return -ENOMEM;

	return 0;
}

int nvt_pinctrl_remove(struct platform_device *pdev)
{

	return 0;
}

static const struct of_device_id nvt_pinctrl_of_match[] = {
	{ .compatible = "nvt,nvt_top", },
	{ },
};

static struct platform_driver nvt_pinctrl_driver = {
	.driver = {
		.name = "nvt-pinctrl",
		.of_match_table = nvt_pinctrl_of_match,
	},
	.probe = nvt_pinmux_probe,
	.remove = nvt_pinctrl_remove,
};

static int __init nvt_pinctrl_init(void)
{
	return platform_driver_register(&nvt_pinctrl_driver);
}
arch_initcall(nvt_pinctrl_init);

static void __exit nvt_pinctrl_exit(void)
{
	platform_driver_unregister(&nvt_pinctrl_driver);
}
module_exit(nvt_pinctrl_exit);

MODULE_DESCRIPTION("Novatek pinctrl driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.03.031");
MODULE_DEVICE_TABLE(of, nvt_pinctrl_of_match);
