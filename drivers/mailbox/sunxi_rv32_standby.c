// SPDX-License-Identifier: GPL-2.0
/*
 * sunxi's RV32 Mailbox Standby Driver
 *
 * Copyright (C) 2021 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/elf.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/mailbox_client.h>

#define MBOX_NAME			"mbox-chan"

#define PM_RV32_CLK_REG1			(0x02001000 | 0x0d04)
#define PM_RV32_CLK_REG2			(0x02001000 | 0x0d0C)
#define PM_RV32_CLK_REG3			(0x02001000 | 0x0d00)
#define PM_RV32_CLK_REG4			(0x06010000 | 0x0204)

#define PM_RV32_POWER_SUSPEND	0xf3f30101
#define PM_RV32_POWER_ACK		0xf3f30102
#define PM_RV32_POWER_DEAD		0xf3f30103
#define PM_RV32_POWER_RESUME	0xf3f30204

#define SUSPEND_TIMEOUT			msecs_to_jiffies(100)
#define RESUME_TIMEOUT			msecs_to_jiffies(100)

struct sunxi_rv32_power {
	struct device *dev;
	struct mbox_chan *chan;
	struct mbox_client client;
	struct platform_device *pdev;
	const char *firmware;
	uint32_t boot;

	struct completion complete;
	struct completion ack;
	void __iomem *clk1;
	void __iomem *clk2;
	void __iomem *clk3;
	void __iomem *clk4;
#if defined(CONFIG_DEBUG_FS) && defined(CONFIG_PM_SLEEP)
	int   status;
	struct dentry *dir;
#endif
};

static void sunxi_mb_rx_callback(struct mbox_client *cl, void *data)
{
	u32 tmp = *(uint32_t *)data;
	struct sunxi_rv32_power *ddata = dev_get_drvdata(cl->dev);

	switch (tmp) {

	case PM_RV32_POWER_ACK:
		complete(&ddata->ack);
		break;

	case PM_RV32_POWER_DEAD:
		complete(&ddata->complete);
		break;

	default:
		dev_warn(ddata->dev, "Undown data(0x%08x)\n", tmp);
	}
}

static int sunxi_request_mbox(struct sunxi_rv32_power *ddata)
{
	struct device *dev = ddata->dev;

	ddata->client.rx_callback = sunxi_mb_rx_callback;
	ddata->client.tx_block = true;
	ddata->client.tx_done = NULL;
	ddata->client.dev = ddata->dev;

	ddata->chan = mbox_request_channel_byname(&ddata->client, MBOX_NAME);
	if (IS_ERR(ddata->chan)) {
		dev_warn(dev, "cannot get %s channel (ret=%ld)\n", MBOX_NAME,
						PTR_ERR(ddata->chan));
			goto err_probe;
	}

	return 0;

err_probe:
	return -EPROBE_DEFER;
}

static void sunxi_rv32_free_mbox(struct sunxi_rv32_power *ddata)
{
	if (ddata->chan)
		mbox_free_channel(ddata->chan);
	ddata->chan = NULL;
}

#ifdef CONFIG_PM_SLEEP
static void sunxi_e907_get_boot(struct sunxi_rv32_power *ddata)
{
	ddata->boot = readl(ddata->clk4);
	if (ddata->boot == 0)
		dev_warn(ddata->dev, "fail to get e907 boot addr\n");
	dev_dbg(ddata->dev, "Get boot addr:0x%08x\n", ddata->boot);
}

static void sunxi_e907_clk_config(struct sunxi_rv32_power *ddata, bool status)
{
	uint32_t reg_val;

	if (status) {
		/* turn off clk and asserd cpu */
		reg_val = (0x16aa << 16 | 0x4);
		writel(reg_val, ddata->clk1);

		/* Reset Cpu */
		reg_val = (1 << 16) | (1 << 0);
		writel(reg_val, ddata->clk2);

		/* Set Cpu boot addr */
		writel(ddata->boot, ddata->clk4);

		reg_val = (0x16aa << 16 | 0x4);
		writel(reg_val, ddata->clk1);
		/* Set e907 freq to 600MHz */
		reg_val = (3 << 24) | (1 << 8);
		writel(reg_val, ddata->clk3);
		/* turn on clk and deassert cpu */
		reg_val = (0x16aa << 16 | 0x07);
		writel(reg_val, ddata->clk1);
	} else {
		/* turn off clk and asserd cpu */
		reg_val = (0x16aa << 16);
		writel(reg_val, ddata->clk1);
	}
}

static int sunxi_rv32_power_suspend(struct device *dev)
{
	int ret = 0;
	u32 data = PM_RV32_POWER_SUSPEND;
	struct sunxi_rv32_power *ddata = dev_get_drvdata(dev);

	ret = mbox_send_message(ddata->chan, &data);
	if (ret < 0) {
		dev_err(dev, "fail to send mbox msg\n");
		return ret;
	}

	/* save e907 boot addr */
	sunxi_e907_get_boot(ddata);

	/* wait e907 ack this msg */
	ret = wait_for_completion_timeout(&ddata->ack, SUSPEND_TIMEOUT);
	if (!ret) {
		dev_err(dev, "timeout wait rv32 ack\n");
		return 0;
	}

	return 0;
}

static int sunxi_rv32_power_suspend_late(struct device *dev)
{
	int ret = 0;
	struct sunxi_rv32_power *ddata = dev_get_drvdata(dev);

	/* wait e907 send dead msg */
	ret = wait_for_completion_timeout(&ddata->complete, SUSPEND_TIMEOUT);
	if (!ret)
		dev_err(dev, "timeout wait rv32 suspend\n");

	sunxi_e907_clk_config(ddata, false);
#if defined(CONFIG_DEBUG_FS) && defined(CONFIG_PM_SLEEP)
	ddata->status = 1;
#endif
	return 0;
}

static int sunxi_rv32_power_resume(struct device *dev)
{
	struct sunxi_rv32_power *ddata = dev_get_drvdata(dev);
	int ret = 0;
	u32 data = PM_RV32_POWER_RESUME;

	sunxi_e907_clk_config(ddata, true);
	udelay(10);

	ret = mbox_send_message(ddata->chan, &data);
	if (ret < 0) {
		dev_err(dev, "fail to send mbox msg\n");
		return ret;
	}

	/* wait e907 ack this msg */
	ret = wait_for_completion_timeout(&ddata->ack, RESUME_TIMEOUT);
	if (!ret) {
		dev_err(dev, "timeout wait rv32 resume\n");
		return -EBUSY;
	}

#if defined(CONFIG_DEBUG_FS) && defined(CONFIG_PM_SLEEP)
	ddata->status = 0;
#endif
	return 0;
}

static struct dev_pm_ops sunxi_rv32_pm_ops = {
	.suspend = sunxi_rv32_power_suspend,
	.suspend_late = sunxi_rv32_power_suspend_late,
	.resume = sunxi_rv32_power_resume,
};
#endif

#if defined(CONFIG_DEBUG_FS) && defined(CONFIG_PM_SLEEP)
static const char *status_string[2] = {
	"resume", "suspend"
};

static ssize_t sunxi_status_read(struct file *filp, char __user *userbuf,
				size_t count, loff_t *ppos)
{
	struct sunxi_rv32_power *ddata = filp->private_data;
	int status = 0;
	int len;

	status = ddata->status == 0 ? 0 : 1;
	len = strlen(status_string[status]);

	return simple_read_from_buffer(userbuf, count, ppos,
					status_string[status], len);
}

static ssize_t sunxi_status_write(struct file *filp, const char __user *userbuf,
				 size_t count, loff_t *ppos)
{
	struct sunxi_rv32_power *ddata = filp->private_data;
	char buf[16];
	int ret = 0;

	if (count > sizeof(buf) || count <= 0)
		return -EINVAL;

	ret = copy_from_user(buf, userbuf, count);
	if (ret)
		return -EFAULT;

	buf[count - 1] = '\0';

	if (!strncmp(buf, status_string[0], count)) {
		if (ddata->status != 0) {
			dev_dbg(ddata->dev, "resume device.\n");
			sunxi_rv32_power_resume(ddata->dev);
		} else
			dev_dbg(ddata->dev, "device already resume\n");
	} else if (!strncmp(buf, status_string[1], count)) {
		if (ddata->status != 1) {
			dev_dbg(ddata->dev, "suspend device.\n");
			sunxi_rv32_power_suspend(ddata->dev);
			sunxi_rv32_power_suspend_late(ddata->dev);
		} else
			dev_dbg(ddata->dev, "device already suspend\n");
	}

	return count;
}

static const struct file_operations sunxi_power_status_ops = {
	.write = sunxi_status_write,
	.read = sunxi_status_read,
	.open = simple_open,
};

void sunxi_rv32_create_file(struct sunxi_rv32_power *ddata)
{
	debugfs_create_file("status", 0600, ddata->dir,
			    ddata, &sunxi_power_status_ops);
}
#endif

static const struct of_device_id sunxi_rv32_match[] = {
	{ .compatible = "allwinner,sunxi-e907-standby" },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_rv32_match);

static int sunxi_rv32_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_rv32_power *ddata;
	int ret;

	ddata = devm_kzalloc(dev, sizeof(*ddata), GFP_KERNEL);
	if (!ddata)
		return -ENOMEM;

	ddata->dev = dev;
	ddata->pdev = pdev;
	ddata->boot = 0;

	ret = of_property_read_string(dev->of_node, "firmware", &ddata->firmware);
	if (ret) {
		dev_err(dev, "failed to read firmware property\n");
		return ret;
	}
	dev_dbg(dev, "firmware name:%s\n", ddata->firmware);

	ddata->clk1 = devm_ioremap(dev, PM_RV32_CLK_REG1, 4);
	if (!ddata->clk1) {
		dev_err(dev, "ioremap failed\r\n");
		ret = -ENOMEM;
		goto out;
	}

	ddata->clk2 = devm_ioremap(dev, PM_RV32_CLK_REG2, 4);
	if (!ddata->clk2) {
		dev_err(dev, "ioremap failed\r\n");
		ret = -ENOMEM;
		goto out;
	}

	ddata->clk3 = devm_ioremap(dev, PM_RV32_CLK_REG3, 4);
	if (!ddata->clk3) {
		dev_err(dev, "ioremap failed\r\n");
		ret = -ENOMEM;
		goto out;
	}

	ddata->clk4 = devm_ioremap(dev, PM_RV32_CLK_REG4, 4);
	if (!ddata->clk4) {
		dev_err(dev, "ioremap failed\r\n");
		ret = -ENOMEM;
		goto out;
	}

	ret = sunxi_request_mbox(ddata);
	if (ret < 0) {
		dev_err(dev, "sunxi_rv32_request_mbox failed\n");
		goto out;
	}

#if defined(CONFIG_DEBUG_FS) && defined(CONFIG_PM_SLEEP)
	if (debugfs_initialized()) {
		ddata->dir = debugfs_create_dir(KBUILD_MODNAME, NULL);
		if (!ddata->dir)
			dev_err(dev, "can't create debugfs dir\n");
		else
			sunxi_rv32_create_file(ddata);
	}
#endif
	init_completion(&ddata->ack);
	init_completion(&ddata->complete);
	platform_set_drvdata(pdev, ddata);

	return 0;
out:
	return ret;
}

static int sunxi_rv32_remove(struct platform_device *pdev)
{
	struct sunxi_rv32_power *ddata = platform_get_drvdata(pdev);

#if defined(CONFIG_DEBUG_FS) && defined(CONFIG_PM_SLEEP)
	debugfs_remove(ddata->dir);
#endif
	reinit_completion(&ddata->ack);
	reinit_completion(&ddata->complete);
	sunxi_rv32_free_mbox(ddata);

	return 0;
}

static struct platform_driver sunxi_rv32_standby_driver = {
	.probe = sunxi_rv32_probe,
	.remove = sunxi_rv32_remove,
	.driver = {
		.name = "sunxi-rv32-standby",
		.of_match_table = sunxi_rv32_match,
#ifdef CONFIG_PM_SLEEP
		.pm = &sunxi_rv32_pm_ops,
#endif
	},
};

static int __init sunxi_rv32_standby_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_rv32_standby_driver);

	return ret;
}

static void __exit sunxi_rv32_standby_exit(void)
{
	platform_driver_unregister(&sunxi_rv32_standby_driver);
}

device_initcall(sunxi_rv32_standby_init);
module_exit(sunxi_rv32_standby_exit);

MODULE_DESCRIPTION("RV32 Mailbox Standby Driver");
MODULE_AUTHOR("lijiajian <lijiajian@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
