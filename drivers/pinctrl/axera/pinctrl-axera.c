/*
 * Copyright (C) 2022 AXERA Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>
#include <linux/gpio.h>

#include "../core.h"
#include "../pinctrl-utils.h"
#include "../pinmux.h"
#include "pinctrl-axera.h"
#include "pinctrl-ax620e.h"

struct ax_pinctrl {
	struct pinctrl_dev *pctldev;
	struct device *dev;
	void __iomem *base;
	void __iomem *base2;
	spinlock_t lock;
	struct axera_pinctrl_soc_info *info;
};

static u32 ax_pinctrl_readl(struct ax_pinctrl *axpctl, u32 offset)
{
	if (offset < SECOND_OFFSET)
		return readl(axpctl->base + offset);
	else
		return readl(axpctl->base2 + offset - SECOND_OFFSET);
}

static void ax_pinctrl_writel(struct ax_pinctrl *axpctl, u32 offset, u32 val)
{
	if (offset < SECOND_OFFSET)
		writel(val, axpctl->base + offset);
	else
		writel(val, axpctl->base2 + offset - SECOND_OFFSET);
}

static int ax_dt_node_to_map(struct pinctrl_dev *pctldev,
			     struct device_node *np_config,
			     struct pinctrl_map **map, u32 *num_maps)
{
	return pinconf_generic_dt_node_to_map(pctldev, np_config, map,
					      num_maps, PIN_MAP_TYPE_INVALID);
}

static const struct pinctrl_ops ax_pinctrl_ops = {
	.dt_node_to_map = ax_dt_node_to_map,
	.dt_free_map = pinctrl_utils_free_map,
	.get_groups_count = pinctrl_generic_get_group_count,
	.get_group_name = pinctrl_generic_get_group_name,
	.get_group_pins = pinctrl_generic_get_group_pins,
};

static int ax_set_mux(struct pinctrl_dev *pctldev, unsigned int func_selector,
		      unsigned int group_selector)
{
	struct ax_pinctrl *axpctl = pinctrl_dev_get_drvdata(pctldev);
	struct axera_pinctrl_soc_info *info = axpctl->info;
	const struct pinctrl_pin_desc *pindesc = info->pins + group_selector;
	struct axera_pin_data *data = pindesc->drv_data;
	struct axera_mux_desc *mux;
	u32 offset;
	struct function_desc *func;
	unsigned long flags;
	u32 val;
	/* Skip reserved pin */
	if (!data)
		return -EINVAL;

	mux = data->muxes;
	offset = data->offset;

	func = pinmux_generic_get_function(pctldev, func_selector);
	if (!func)
		return -EINVAL;

	while (mux->name) {
		if (strcmp(mux->name, func->name) == 0)
			break;
		mux++;
	}
	if (!(mux->name))
		pr_err("%s : pinctrl function not found\n", __func__);

	/* Found mux value to be written */
	spin_lock_irqsave(&axpctl->lock, flags);
	val = ax_pinctrl_readl(axpctl, offset);
	val &= ~FUNCTION_SELECT_BIT_CLEAR;
	val |= (mux->muxval) << FUNCTION_SELECT;
	ax_pinctrl_writel(axpctl, offset, val);
	spin_unlock_irqrestore(&axpctl->lock, flags);
	pr_debug("%s val:%x is mux->name:%s group_selector:%d  func_selector:%d \
	          data->offset:%x\n", __func__, val, mux->name, group_selector, func_selector, data->offset);
	return 0;
}

/****************request gpio******************/
int axera_request_gpio(struct pinctrl_dev *pctldev, struct pinctrl_gpio_range *range,
		       unsigned offset)
{
	if (range == NULL) {
		pr_err("%s: pinctrl request gpio failed\n", __func__);
		return -1;
	}
	pr_debug("%s, %d, range->base:%d range->pin_base:%d offset:%d\n",
		 __func__, __LINE__, range->base, range->pin_base, range->base);
	gpio_request(range->base, NULL);
	return 0;
}

/****************free gpio******************/
void axera_free_gpio(struct pinctrl_dev *pctldev, struct pinctrl_gpio_range *range, unsigned offset)
{
	if (range == NULL) {
		pr_err("%s: pinctrl free gpio failed\n", __func__);
		return;
	}
	gpio_free(range->base);
	pr_debug("%s, %d, range->base:%d offset:%d\n", __func__, __LINE__,
		 range->base, offset);
	return;
}

/****************set gpio direction******************/
int axera_gpio_direction(struct pinctrl_dev *pctldev, struct pinctrl_gpio_range *range,
			 unsigned offset, bool input)
{
	int p_or_n;
	if (range == NULL) {
		pr_err("%s: pinctrl change gpio_direction failed\n", __func__);
		return -1;
	}
	pr_debug("%s, %d, range->base:%d,offset:%d input:%d\n", __func__, __LINE__, range->base,
		 offset, input);
	p_or_n = (range->pin_base) % 2;
	if (input == true)
		return gpio_direction_input(range->base);
	else if (input == false)
		return gpio_direction_output(range->base, !input);

	pr_err("%s is failed\n", __func__);

	return 0;
}

static const struct pinmux_ops ax_pinmux_ops = {
	.get_functions_count = pinmux_generic_get_function_count,
	.get_function_name = pinmux_generic_get_function_name,
	.get_function_groups = pinmux_generic_get_function_groups,
	.set_mux = ax_set_mux,
	.gpio_request_enable = axera_request_gpio,
	.gpio_disable_free = axera_free_gpio,
	.gpio_set_direction = axera_gpio_direction,
	.strict = true,
};

static int ax_pin_config_get(struct pinctrl_dev *pctldev, unsigned int pin, unsigned long *config)
{
	struct ax_pinctrl *axpctl = pinctrl_dev_get_drvdata(pctldev);
	struct axera_pinctrl_soc_info *info = axpctl->info;
	const struct pinctrl_pin_desc *pindesc = info->pins + pin;
	struct axera_pin_data *data = pindesc->drv_data;
	enum pin_config_param param = pinconf_to_config_param(*config);
	u32 val;
	u32 arg = 0;
	/* Skip reserved pin */
	if (!data)
		return -EINVAL;
	val = ax_pinctrl_readl(axpctl, data->offset);
	switch (param) {
	case PIN_CONFIG_BIAS_PULL_DOWN:
		if ((val & AX_PULL_DOWN) != AX_PULL_DOWN)
			return -EINVAL;
		arg = 1;
		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		if ((((data->offset) >= THM_AIN3_OFFSET) && ((data->offset) <= SD_PWR_EN_OFFSET)) ||
		    (((data->offset) >= MICP_L_D_OFFSET) && ((data->offset) <= MICP_R_D_OFFSET))) {
			if ((val & (0x3 << AX_PULL_DOWN_BIT)) != (0x3 << AX_PULL_DOWN_BIT))
				return -EINVAL;
			arg = 1;
		} else {
			if ((val & AX_PULL_UP) != val)
				return -EINVAL;
			arg = 1;
		}
		break;
	case PIN_CONFIG_BIAS_DISABLE:
		if (val & (0x3 << AX_PULL_DOWN_BIT))
			return -EINVAL;
		arg = 0;
		break;
	case PIN_CONFIG_DRIVE_STRENGTH:
		arg = val & AX_DRIVE_STRENGTH;
		break;
	case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
		if (val & (AX_SCHMITT_ENABLE))
			arg = 1;
		break;
	default:
		return -ENOTSUPP;
	}

	*config = pinconf_to_config_packed(param, arg);
	return 0;
}

static int ax_pin_config_set(struct pinctrl_dev *pctldev, unsigned int pin,
			     unsigned long *configs, unsigned int num_configs)
{
	struct ax_pinctrl *axpctl = pinctrl_dev_get_drvdata(pctldev);
	struct axera_pinctrl_soc_info *info = axpctl->info;
	const struct pinctrl_pin_desc *pindesc = info->pins + pin;
	struct axera_pin_data *data = pindesc->drv_data;
	enum pin_config_param param;
	u32 val, arg;
	int i;
	/* Skip reserved pin */
	if (!data)
		return -EINVAL;

	val = ax_pinctrl_readl(axpctl, data->offset);

	for (i = 0; i < num_configs; i++) {
		param = pinconf_to_config_param(configs[i]);
		arg = pinconf_to_config_argument(configs[i]);
		switch (param) {
		case PIN_CONFIG_BIAS_PULL_DOWN:
			val &= ~AX_PULL_UP;
			val |= AX_PULL_DOWN;
			break;
		case PIN_CONFIG_BIAS_PULL_UP:
			if ((((data->offset) >= THM_AIN3_OFFSET) && ((data->offset) <= SD_PWR_EN_OFFSET)) ||
				(((data->offset) >= MICP_L_D_OFFSET) && ((data->offset) <= MICP_R_D_OFFSET))) {
				val |= AX_PULL_SE;
				val |= AX_PULL_EN;
			} else {
				val &= ~AX_PULL_DOWN;
				val |= AX_PULL_UP;
			}
			break;
		case PIN_CONFIG_BIAS_DISABLE:
			val &= ~AX_PULL_DOWN;
			val &= ~AX_PULL_UP;
			break;
		case PIN_CONFIG_DRIVE_STRENGTH:
			val &= ~AX_DRIVE_STRENGTH;
			val |= arg;
			break;
		case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
			val &= ~AX_SCHMITT_ENABLE;
			val |= (arg << AX_SCHMITT_ENABLE_BIT);
			break;
		default:
			return -ENOTSUPP;
		}

	}
	pr_debug("%s is param:%d val:%x data->offset:%x\n", __func__, param, val, data->offset);
	ax_pinctrl_writel(axpctl, data->offset, val);
	return 0;
}

static const struct pinconf_ops ax_pinconf_ops = {
	.pin_config_set = ax_pin_config_set,
	.pin_config_get = ax_pin_config_get,
	.is_generic = true,
};

static int ax_pinctrl_build_state(struct platform_device *pdev)
{
	struct ax_pinctrl *axpctl = platform_get_drvdata(pdev);
	struct axera_pinctrl_soc_info *info = axpctl->info;
	struct pinctrl_dev *pctldev = axpctl->pctldev;
	struct function_desc *functions;
	int i, ngroups, nfunctions;
	struct group_desc *groups;
	struct axera_mux_desc *mux = NULL;
	u32 count = 0;

	/* Every single pin composes a group */
	ngroups = info->npins;
	groups = devm_kcalloc(&pdev->dev, ngroups, sizeof(*groups), GFP_KERNEL);
	if (!groups)
		return -ENOMEM;
	for (i = 0; i < ngroups; i++) {
		const struct pinctrl_pin_desc *pindesc = info->pins + i;
		struct group_desc *group = groups + i;

		group->name = pindesc->name;
		group->pins = (int *)&pindesc->number;
		group->num_pins = 1;
		radix_tree_insert(&pctldev->pin_group_tree, i, group);
	}

	pctldev->num_groups = ngroups;

	/* Build function list from pin mux functions */
	functions = kcalloc(FUNCTION_MAX, sizeof(*functions), GFP_KERNEL);
	if (!functions)
		return -ENOMEM;
	nfunctions = 0;

	for (i = 0; i < info->npins; i++) {
		const struct pinctrl_pin_desc *pindesc = info->pins + i;
		struct axera_pin_data *data = pindesc->drv_data;

		/* Reserved pins do not have a drv_data at all */
		if (!data)
			continue;

		/* Loop over all muxes for the pin */
		mux = data->muxes;
		count = data->num;
		while (count) {
			functions->name = mux->name;
			functions->num_group_names = 1;
			functions->group_names = devm_kcalloc(&pdev->dev,
								 functions->num_group_names,
								 sizeof(*functions->group_names),
								 GFP_KERNEL);
			*(functions->group_names) = pindesc->name;
			radix_tree_insert(&pctldev->pin_function_tree, nfunctions++, functions);
			mux++;
			functions++;
			count--;
		}
	}

	pctldev->num_functions = nfunctions;
	pr_debug("%s  nfunctions:%d\n", __func__, nfunctions);

	return 0;
}

int axera_pinctrl_init(struct platform_device *pdev, struct axera_pinctrl_soc_info *info)
{
	struct pinctrl_desc *pctldesc;
	struct ax_pinctrl *axpctl;
	struct resource *res;
	int ret;

	axpctl = devm_kzalloc(&pdev->dev, sizeof(*axpctl), GFP_KERNEL);
	if (!axpctl)
		return -ENOMEM;

	spin_lock_init(&axpctl->lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	axpctl->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(axpctl->base))
		return PTR_ERR(axpctl->base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	axpctl->base2 = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(axpctl->base2))
		return PTR_ERR(axpctl->base2);

	axpctl->dev = &pdev->dev;
	axpctl->info = info;

	pctldesc = devm_kzalloc(&pdev->dev, sizeof(*pctldesc), GFP_KERNEL);
	if (!pctldesc)
		return -ENOMEM;

	pctldesc->name = dev_name(&pdev->dev);
	pctldesc->owner = THIS_MODULE;
	pctldesc->pins = info->pins;
	pctldesc->npins = info->npins;
	pctldesc->pctlops = &ax_pinctrl_ops;
	pctldesc->pmxops = &ax_pinmux_ops;
	pctldesc->confops = &ax_pinconf_ops;

	axpctl->pctldev = devm_pinctrl_register(&pdev->dev, pctldesc, axpctl);
	if (IS_ERR(axpctl->pctldev)) {
		ret = PTR_ERR(axpctl->pctldev);
		dev_err(&pdev->dev, "failed to register pinctrl: %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, axpctl);

	ret = ax_pinctrl_build_state(pdev);
	if (ret) {
		dev_err(&pdev->dev, "failed to build state: %d\n", ret);
		return ret;
	}
	dev_info(&pdev->dev, "initialized pinctrl driver\n");
	return 0;
}
