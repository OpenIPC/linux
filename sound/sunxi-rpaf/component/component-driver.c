/*
 * sound\sunxi-rpaf\component\rpaf-comp-driver.c
 * (C) Copyright 2019-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/etherdevice.h>
#include <linux/crypto.h>
#include <linux/miscdevice.h>
#include <linux/capability.h>

#include <sound/aw_rpaf/component-driver.h>
#include <sound/aw_rpaf/component-core.h>

static ssize_t show_rpaf_status(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct snd_soc_rpaf_info *rpaf_info = dev_get_drvdata(dev);
	int count = 0;

	count += sprintf(buf, "[%s] dump audio status:\n", rpaf_info->name);

	return count;
}

static ssize_t store_rpaf_status(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct snd_soc_rpaf_info *rpaf_info = dev_get_drvdata(dev);
	int ret = 0;
	int rw_flag = 0;
	int input_status_group = 0;
	int input_status_cmd = 0;

	ret = sscanf(buf, "%d,%d,%d", &rw_flag, &input_status_group,
				&input_status_cmd);
	dev_info(dev, "[%s] ret:%d, rw_flag:%d, status_group:%d, status_cmd:%d\n",
			rpaf_info->name, ret, rw_flag, input_status_group,
			input_status_cmd);
	ret = count;

	return ret;
}

static DEVICE_ATTR(status_debug, 0644, show_rpaf_status, store_rpaf_status);

static struct attribute *rpaf_dev_debug_attrs[] = {
	&dev_attr_status_debug.attr,
	NULL,
};

static struct attribute_group rpaf_dev_debug_attr_group = {
	.name   = "audio_debug",
	.attrs  = rpaf_dev_debug_attrs,
};

static int snd_soc_rpaf_misc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct snd_soc_rpaf_misc_priv *rpaf_misc_priv = NULL;
	unsigned int temp_val = 0;
	int ret = 0;

	rpaf_misc_priv = devm_kzalloc(dev, sizeof(struct snd_soc_rpaf_misc_priv),
								GFP_KERNEL);
	if (IS_ERR_OR_NULL(rpaf_misc_priv)) {
		dev_err(dev, "kzalloc for snd_soc_rpaf_misc_priv failed.\n");
		return -ENOMEM;
	}
	rpaf_misc_priv->dev = dev;
	dev_set_drvdata(&pdev->dev, rpaf_misc_priv);

	ret = of_property_read_u32(np, "dsp_id", &temp_val);
	if (ret < 0) {
		dev_err(dev, "cannot get dsp_id number!\n");
		goto err_prop_read_dsp_id;
	}
	rpaf_misc_priv->dsp_id = temp_val;
	memset(rpaf_misc_priv->name, 0, sizeof(rpaf_misc_priv->name));
	snprintf(rpaf_misc_priv->name, sizeof(rpaf_misc_priv->name),
			"rpaf_dsp%d",
			rpaf_misc_priv->dsp_id);

	ret  = sysfs_create_group(&pdev->dev.kobj, &rpaf_dev_debug_attr_group);
	if (ret < 0) {
		dev_warn(&pdev->dev, "failed to create attr group\n");
		goto err_sysfs_create;
	}

	ret = snd_soc_rpaf_misc_register_device(&pdev->dev, rpaf_misc_priv->dsp_id);
	if (ret < 0) {
		dev_err(&pdev->dev, "dsp_id(%d) deregister dev error(%d).\n",
					rpaf_misc_priv->dsp_id, ret);
			goto err_misc_register_device;
	}

	dev_info(&pdev->dev, "[%s] probe finished!\n", rpaf_misc_priv->name);

	return 0;
err_misc_register_device:
	sysfs_remove_group(&pdev->dev.kobj, &rpaf_dev_debug_attr_group);
err_sysfs_create:
err_prop_read_dsp_id:
	devm_kfree(&pdev->dev, rpaf_misc_priv);
	return ret;
}

static int snd_soc_rpaf_misc_remove(struct platform_device *pdev)
{
	struct snd_soc_rpaf_misc_priv *rpaf_misc_priv = dev_get_drvdata(&pdev->dev);
	int ret = 0;

	if (IS_ERR_OR_NULL(rpaf_misc_priv)) {
		dev_err(&pdev->dev, "rpaf_misc_priv is null.\n");
			return -EFAULT;
	}

	ret = snd_soc_rpaf_misc_deregister_device(&pdev->dev, rpaf_misc_priv->dsp_id);
	if (ret < 0) {
		dev_err(&pdev->dev, "dsp_id(%d) deregister dev error(%d).\n",
				rpaf_misc_priv->dsp_id, ret);
		return ret;
	}

	sysfs_remove_group(&pdev->dev.kobj, &rpaf_dev_debug_attr_group);

	dev_info(&pdev->dev, "[%s] remove finished!\n", rpaf_misc_priv->name);

	devm_kfree(&pdev->dev, rpaf_misc_priv);
	dev_set_drvdata(&pdev->dev, NULL);

	return ret;
}

static const struct of_device_id snd_soc_rpaf_misc_ids[] = {
	{ .compatible = "allwinner,rpaf-dsp0" },
	{ .compatible = "allwinner,rpaf-dsp1" },
	{}
};

static struct platform_driver snd_soc_rpaf_misc_driver = {
	.probe	= snd_soc_rpaf_misc_probe,
	.remove	= snd_soc_rpaf_misc_remove,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "sunxi-rpaf-dsp",
		.of_match_table	= snd_soc_rpaf_misc_ids,
	},
};
module_platform_driver(snd_soc_rpaf_misc_driver);

MODULE_DESCRIPTION("sunxi dsp remote processor audio framework driver");
MODULE_AUTHOR("yumingfeng@allwinnertech.com");
MODULE_LICENSE("GPL");
