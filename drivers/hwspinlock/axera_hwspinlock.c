// AXERA-License-Identifier: GPL-2.0
/*
 * AXERA Hwspinlock driver
 *
 * Copyright (c) 2022, AXERA Corporation.
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
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/hwspinlock.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include "hwspinlock_internal.h"
/* hwspinlock registers definition */
#define SPINLOCK_MASTER_ID_SHIFT        (8)
#define SPINLOCK_COMMON_REG_NUM_SHIFT   (3)
#define SPINLOCK_MASTER_ID_UNLOCK_SHIFT (16)
#define CPU_MASTER_ID                   (0)
#define AXERA_HWLOCKS_NUM               (32)
/* hwspinlock operate */
#define HWSPINLOCK_LOCK_ADDR(_X_)	((CPU_MASTER_ID << SPINLOCK_MASTER_ID_SHIFT) | (_X_ << SPINLOCK_COMMON_REG_NUM_SHIFT))
#define HWSPINLOCK_UNLOCK_ADDR(_X_)	((CPU_MASTER_ID << SPINLOCK_MASTER_ID_SHIFT) | (_X_ << SPINLOCK_COMMON_REG_NUM_SHIFT) | 0x4)
#define HWSPINLOCK_UNLOCK_CMD		((((CPU_MASTER_ID << 12) | (CPU_MASTER_ID << 8) | (CPU_MASTER_ID << 4) | CPU_MASTER_ID) << SPINLOCK_MASTER_ID_UNLOCK_SHIFT) | 0xFFFF)
struct ax_hwspinlock_dev {
	void __iomem *base;
	struct hwspinlock_device bank;
};
#define FLASH_SYS_BASE 0x10030000
/* try to lock the hardware spinlock */
static int ax_hwspinlock_trylock(struct hwspinlock *lock)
{
	struct ax_hwspinlock_dev *ax_hwlock =
		dev_get_drvdata(lock->bank->dev);
	int user_id, lock_id;
	lock_id = hwlock_to_id(lock);
	/* get the hardware spinlock master/user id */
	user_id = readl((ax_hwlock->base) + HWSPINLOCK_LOCK_ADDR(lock_id));
	if (!user_id) {
		return 1;
	}
	return 0;
}
/* unlock the hardware spinlock */
static void ax_hwspinlock_unlock(struct hwspinlock *lock)
{
	struct ax_hwspinlock_dev *ax_hwlock =
		dev_get_drvdata(lock->bank->dev);
	int lock_id;
	lock_id = hwlock_to_id(lock);
	writel(HWSPINLOCK_UNLOCK_CMD, (ax_hwlock->base) + HWSPINLOCK_UNLOCK_ADDR(lock_id));
}
/* The specs recommended below number as the retry delay time */
static void ax_hwspinlock_relax(struct hwspinlock *lock)
{
	ndelay(10);
}

static const struct hwspinlock_ops ax_hwspinlock_ops = {
	.trylock = ax_hwspinlock_trylock,
	.unlock = ax_hwspinlock_unlock,
	.relax = ax_hwspinlock_relax,
};
static void ax_hwspinlock_disable(void)
{
	void __iomem *base;

	base = ioremap(FLASH_SYS_BASE, 0x10000);
	writel(1 << 23, base + 0x4014);
	writel(1 << 19, base + 0x8008);
	iounmap(base);
}

static void ax_hwspinlock_enable(void)
{
	void __iomem *base;

	base = ioremap(FLASH_SYS_BASE, 0x10000);
	writel(1 << 19, base + 0x4008);
	writel(1 << 23, base + 0x8014);
	iounmap(base);
}
static int ax_hwspinlock_probe(struct platform_device *pdev)
{
	struct ax_hwspinlock_dev *ax_hwlock;
	struct hwspinlock *lock;
	struct resource *res;
	int i;
	if (!pdev->dev.of_node)
		return -ENODEV;
	ax_hwlock = devm_kzalloc(&pdev->dev,
				   sizeof(struct ax_hwspinlock_dev) +
				   AXERA_HWLOCKS_NUM * sizeof(*lock),
				   GFP_KERNEL);
	if (!ax_hwlock)
		return -ENOMEM;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ax_hwlock->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ax_hwlock->base))
		return PTR_ERR(ax_hwlock->base);

	ax_hwspinlock_enable();

	for (i = 0; i < AXERA_HWLOCKS_NUM; i++) {
		lock = &ax_hwlock->bank.lock[i];
	}
	platform_set_drvdata(pdev, ax_hwlock);
	dev_set_drvdata(&pdev->dev, ax_hwlock);
	dev_info(&pdev->dev, "hwspinlock done \n");
	return devm_hwspin_lock_register(&pdev->dev, &ax_hwlock->bank,
					 &ax_hwspinlock_ops, 0,
					 AXERA_HWLOCKS_NUM);
}


#ifdef CONFIG_PM_SLEEP
static int ax_hwspinlock_suspend(struct device *dev)
{
	ax_hwspinlock_disable();
	return 0;
}

static int ax_hwspinlock_resume(struct device *dev)
{
	ax_hwspinlock_enable();
	return 0;
}
#endif
static SIMPLE_DEV_PM_OPS(ax_hwspinlock_pm_ops, ax_hwspinlock_suspend, ax_hwspinlock_resume);
static const struct of_device_id ax_hwspinlock_of_match[] = {
	{ .compatible = "ax,hwspinlock-r1p0", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ax_hwspinlock_of_match);
static struct platform_driver ax_hwspinlock_driver = {
	.probe = ax_hwspinlock_probe,
	.driver = {
		.name = "ax_hwspinlock",
		.pm = &ax_hwspinlock_pm_ops,
		.of_match_table = ax_hwspinlock_of_match,
	},
};
module_platform_driver(ax_hwspinlock_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Hardware spinlock driver for Axera");