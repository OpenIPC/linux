/****************************************************************************
*
*    Copyright (c) 2005 - 2020 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License version 2 as
*    published by the Free Software Foundation.
*

*****************************************************************************/

#include "gc_hal_kernel_linux.h"
#include "gc_hal_kernel_platform.h"
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/pm_runtime.h>
#include <linux/pm_opp.h>

gceSTATUS _AdjustParam(IN gcsPLATFORM *Platform, OUT gcsMODULE_PARAMETERS *Args)
{
	struct platform_device *pdev = Platform->device;
	int irqLine = platform_get_irq_byname(pdev, "galcore");
	int ret;
	struct clk *mclk;
	struct clk *pclk;
	unsigned long rate, real_rate;
	unsigned int mod_clk;
	struct device *dev = &(pdev->dev);
	printk("irq line = %d\n", irqLine);
	printk("####################enter _AdjustParam "
		   "#################################\n");
	printk("galcore irq number is %d.\n", irqLine);
	if (irqLine < 0) {
		printk("get galcore irq resource error\n");
		irqLine = platform_get_irq(pdev, 0);
		printk("galcore irq number is %d\n", irqLine);
	}
	if (irqLine < 0)
		return gcvSTATUS_OUT_OF_RESOURCES;

	Args->irqs[gcvCORE_MAJOR] = irqLine;
	// Args->irqs[gcvCORE_MAJOR] = 65;
	printk("xp galcore irq number is %d.\n", Args->irqs[gcvCORE_MAJOR]);
	// Args->contiguousBase = 0x41000000;
	// Args->contiguousSize = 0x1000000;

	// Args->extSRAMBases[0] = 0xFF000000;

	// Args->registerBases[0] = 0xFF100000;
	// Args->registerSizes[0] = 0x20000;
	Args->registerBases[0] = 0x03050000;
	Args->registerSizes[0] = 0x20000;
	printk("%s %d SUCCESS\n", __func__, __LINE__);
	mclk = of_clk_get(pdev->dev.of_node, 0);
	pclk = of_clk_get(pdev->dev.of_node, 1);
	ret = of_property_read_u32(pdev->dev.of_node, "clock-frequency", &mod_clk);
	if (!ret) {
		if (!IS_ERR_OR_NULL(pclk)) {
			rate = clk_round_rate(pclk, mod_clk);
			if (clk_set_rate(pclk, rate)) {
				pr_err("clk_set_rate:%ld  mod_clk:%d failed\n", rate, mod_clk);
				return -1;
			}
			real_rate = clk_get_rate(pclk);
			printk("Want set pclk rate(%d) support(%ld) real(%ld)\n", mod_clk, rate, real_rate);
			ret  = clk_set_parent(mclk, pclk);
			if (ret != 0) {
				pr_err("clk_set_parent() failed! return\n");
				return -1;
			}
		}
		rate = clk_round_rate(mclk, mod_clk);
		if (clk_set_rate(mclk, rate)) {
			pr_err("clk_set_rate:%ld  mod_clk:%d failed\n", rate, mod_clk);
			return -1;
		}
		real_rate = clk_get_rate(mclk);
		printk("Want set mclk rate(%d) support(%ld) real(%ld)\n", mod_clk, rate, real_rate);
	} else {
		if (!IS_ERR_OR_NULL(pclk)) {
			printk("%s rate:%d\n", __func__, mod_clk);
			ret  = clk_set_parent(mclk, pclk);
			if (ret != 0) {
				pr_err("clk_set_parent() failed! return\n");
				return -1;
			}
		}
	}
#ifdef CONFIG_SUNXI_PM_DOMAINS
		pm_runtime_enable(dev);
#else
		if (clk_prepare_enable(mclk)) {
			printk("Couldn't enable module clock\n");
			return -EBUSY;
		}
#endif
	return gcvSTATUS_OK;
}


gceSTATUS _SetPower(IN gcsPLATFORM *Platform, IN gctUINT32 DevIndex, IN gceCORE GPU, IN gctBOOL Enable)
{
	struct platform_device *pdev = Platform->device;
	struct device *dev = &(pdev->dev);
	switch (Enable) {
	case gcvTRUE:
		printk("%s %d ON\n", __func__, GPU);
#ifdef CONFIG_SUNXI_PM_DOMAINS
		pm_runtime_get_sync(dev);
#endif
		break;
	case gcvFALSE:
		printk("%s %d OFF\n", __func__, GPU);
#ifdef CONFIG_SUNXI_PM_DOMAINS
		pm_runtime_put(dev);
#endif
		break;
	default:
		printk("Unsupport clk status");
		break;
	}

	return gcvSTATUS_OK;
}


gceSTATUS _SetClock(IN gcsPLATFORM *Platform, IN gctUINT32 DevIndex, IN gceCORE GPU, IN gctBOOL Enable)
{
#if 0
	void *ccmu_vaddr = ioremap(0x02001000, 0xff0);
	printk("#enter _SetClock#\n");
	writel(0x80000000, ccmu_vaddr + 0x6e0);
	writel(0x10001, ccmu_vaddr + 0x6ec);
	iounmap(ccmu_vaddr);
#else
#endif
	return gcvSTATUS_OK;
}

gceSTATUS _GetPower(IN gcsPLATFORM *Platform)
{
	printk("enter _GetPower \n");
	return gcvSTATUS_OK;
}

gceSTATUS _PutPower(IN gcsPLATFORM *Platform)
{
	printk("enter _PutPower \n");
	return gcvSTATUS_OK;
}

static struct _gcsPLATFORM_OPERATIONS default_ops = {
	.adjustParam = _AdjustParam,
	.getPower = _GetPower,
	.putPower = _PutPower,
	.setPower = _SetPower,
	.setClock = _SetClock,
};

static struct _gcsPLATFORM default_platform = {
	.name = __FILE__, .ops = &default_ops,
};

static struct platform_device *default_dev;

static const struct of_device_id galcore_dev_match[] = {
	{.compatible = "allwinner,npu"}, {},
};

int gckPLATFORM_Init(struct platform_driver *pdrv,
			 struct _gcsPLATFORM **platform)
{
	printk("enter gckPLATFORM_Init from allwinenertech\n");
	pdrv->driver.of_match_table = galcore_dev_match;

	*platform = &default_platform;

	return 0;
}

int gckPLATFORM_Terminate(struct _gcsPLATFORM *platform)
{
	if (default_dev) {
		platform_device_unregister(default_dev);
		default_dev = NULL;
	}

	return 0;
}
