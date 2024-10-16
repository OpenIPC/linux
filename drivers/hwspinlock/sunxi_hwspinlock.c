/*
 * SUNXI hardware spinlock driver
 *
 * Copyright (C) 2015 Allwinnertech - http://www.allwinnertech.com
 *
 * Contact: Feng Xia <xiafeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/hwspinlock.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/err.h>

#include "hwspinlock_internal.h"

/* hardware spinlock register list */
#define	LOCK_SYS_STATUS_REG             (0x0000)
#define	LOCK_STATUS_REG                 (0x0010)
#define	LOCK_IRQ_EN_REG                 (0x0020)
#define LOCK_IRQ_PEND_REG               (0x0040)
#define LOCK_BASE_OFFSET                (0x0100)
#define LOCK_BASE_ID                    (0)

/* Possible values of SPINLOCK_LOCK_REG */
#define SPINLOCK_NOTTAKEN               (0)     /* free */
#define SPINLOCK_TAKEN                  (1)     /* locked */

static int sunxi_hwspinlock_trylock(struct hwspinlock *lock)
{
	void __iomem *lock_addr = lock->priv;

	/* attempt to acquire the lock by reading its value */
	return (SPINLOCK_NOTTAKEN == readl(lock_addr));
}

static void sunxi_hwspinlock_unlock(struct hwspinlock *lock)
{
	void __iomem *lock_addr = lock->priv;

	/* release the lock by writing 0 to it */
	writel(SPINLOCK_NOTTAKEN, lock_addr);
}

/*
 * relax the SUNXI interconnect while spinning on it.
 *
 * The specs recommended that the retry delay time will be
 * just over half of the time that a requester would be
 * expected to hold the lock.
 *
 * The number below is taken from an hardware specs example,
 * obviously it is somewhat arbitrary.
 */
static void sunxi_hwspinlock_relax(struct hwspinlock *lock)
{
	ndelay(50);
}

static const struct hwspinlock_ops sunxi_hwspinlock_ops = {
	.trylock = sunxi_hwspinlock_trylock,
	.unlock = sunxi_hwspinlock_unlock,
	.relax = sunxi_hwspinlock_relax,
};

struct sunxi_hwspinlock_device {
	struct hwspinlock_device *bank;
	struct clk *bus_clk;
	int num_locks;
	void __iomem *base_addr;
	struct platform_device *pdev;
	struct resource *res;

};

static int sunxi_hwspinlock_clk_init(struct sunxi_hwspinlock_device *chip)
{
	return clk_prepare_enable(chip->bus_clk);
}

static void sunxi_hwspinlock_clk_deinit(struct sunxi_hwspinlock_device *chip)
{
	clk_disable_unprepare(chip->bus_clk);
}

static int sunxi_hwspinlock_resource_get(struct sunxi_hwspinlock_device *chip)
{
	int err;

	struct device *dev = &chip->pdev->dev;

	chip->res = platform_get_resource(chip->pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(chip->res)) {
		dev_err(dev, "failed to get IORESOURCE_MEM\n");
		return PTR_ERR(chip->res);
	}

	chip->base_addr = devm_ioremap_resource(dev, chip->res);
	if (IS_ERR(chip->base_addr)) {
		dev_err(dev, "failed to ioremap\n");
		return PTR_ERR(chip->base_addr);
	}

	err = of_property_read_u32(dev->of_node, "num-locks", &chip->num_locks);
	if (err || (&chip->num_locks == 0))
		return -ENODEV;

	chip->bus_clk = of_clk_get(dev->of_node, 0);
	if (IS_ERR(chip->bus_clk))
		return -ENODEV;

	return 0;
}

static int sunxi_hwspinlock_probe(struct platform_device *pdev)
{
	struct sunxi_hwspinlock_device *chip;
	struct hwspinlock_device *bank;
	struct hwspinlock *hwlock;
	int i, err;
	struct device *dev;

	chip = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_hwspinlock_device), GFP_KERNEL);
	if (!chip) {
		err = -ENOMEM;
		goto err1;
	}

	chip->pdev = pdev;
	dev = &pdev->dev;

	err = sunxi_hwspinlock_resource_get(chip);
	if (err) {
		dev_err(dev, "failed to get resource\n");
		err = -ENOMEM;
		goto err1;
	}

	bank = devm_kzalloc(dev, sizeof(*bank) + chip->num_locks * sizeof(*hwlock), GFP_KERNEL);
	if (!bank) {
		err = -ENOMEM;
		goto err1;
	}

	chip->bank = bank;

	err = sunxi_hwspinlock_clk_init(chip);
	if (err) {
		err = -ENOMEM;
		goto err1;
	}

	platform_set_drvdata(pdev, chip);

	for (i = 0, hwlock = &bank->lock[0]; i < chip->num_locks; i++, hwlock++)
		hwlock->priv = chip->base_addr + LOCK_BASE_OFFSET + sizeof(u32) * i;

	/*
	 * runtime PM will make sure the clock of this module is
	 * enabled iff at least one lock is requested
	 */
	pm_runtime_enable(dev);

	err = hwspin_lock_register(bank, dev, &sunxi_hwspinlock_ops,
			LOCK_BASE_ID, chip->num_locks);
	if (err)
		goto err0;

	pr_info("sunxi hwspinlock vbase:0x%p\n", chip->base_addr);

	return 0;

err0:
	pm_runtime_disable(dev);
err1:
	return err;
}

static int sunxi_hwspinlock_remove(struct platform_device *pdev)
{
	int err;
	struct sunxi_hwspinlock_device *chip = platform_get_drvdata(pdev);
	struct hwspinlock_device *bank = chip->bank;

	err = hwspin_lock_unregister(bank);
	if (err) {
		dev_err(&pdev->dev, "%s failed: %d\n", __func__, err);
		return err;
	}

	sunxi_hwspinlock_clk_deinit(chip);

	pm_runtime_disable(&pdev->dev);

	return 0;
}

static const struct of_device_id sunxi_hwspinlock_of_match[] = {
	{ .compatible = "allwinner,sunxi-hwspinlock", },
	{ /* end */ },
};
MODULE_DEVICE_TABLE(of, sunxi_hwspinlock_of_match);

static struct platform_driver sunxi_hwspinlock_driver = {
	.probe		= sunxi_hwspinlock_probe,
	.remove		= sunxi_hwspinlock_remove,
	.driver		= {
		.name	= "sunxi-hwspinlock",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sunxi_hwspinlock_of_match),
	},
};

static int __init sunxi_hwspinlock_init(void)
{
	return platform_driver_register(&sunxi_hwspinlock_driver);
}
/* board init code might need to reserve hwspinlocks for predefined purposes */
postcore_initcall(sunxi_hwspinlock_init);

static void __exit sunxi_hwspinlock_exit(void)
{
	platform_driver_unregister(&sunxi_hwspinlock_driver);
}
module_exit(sunxi_hwspinlock_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Hardware spinlock driver for SUNXI");
MODULE_AUTHOR("Feng Xia <xiafeng@allwinnertech.com>");
