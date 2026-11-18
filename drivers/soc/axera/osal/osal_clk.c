/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/device.h>

#include <linux/clk-provider.h>
#include <linux/clk/clk-conf.h>
#include <linux/clkdev.h>

#include "linux/platform_device.h"
#include "linux/device.h"

#include "osal_ax.h"
#include "osal_dev_ax.h"
#include "axdev.h"
#include "axdev_log.h"
#include "osal_lib_ax.h"

void *AX_OSAL_DEV_devm_clk_get(void * pdev, const char *id)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	return devm_clk_get(&pvdev->dev, id);
}

EXPORT_SYMBOL(AX_OSAL_DEV_devm_clk_get);

void AX_OSAL_DEV_devm_clk_put(void * pdev, void * pclk)
{
	struct platform_device *pvdev;
	struct clk *clk;
	pvdev = (struct platform_device *)pdev;
	clk = (struct clk *)pclk;
	devm_clk_put(&pvdev->dev, clk);
}

EXPORT_SYMBOL(AX_OSAL_DEV_devm_clk_put);

void AX_OSAL_DEV_clk_disable(void * pclk)
{
	struct clk *clk = (struct clk *)pclk;
	clk_disable(clk);
}

EXPORT_SYMBOL(AX_OSAL_DEV_clk_disable);

int AX_OSAL_DEV_clk_enable(void * pclk)
{
	struct clk *clk = (struct clk *)pclk;
	return clk_enable(clk);
}

EXPORT_SYMBOL(AX_OSAL_DEV_clk_enable);

bool AX_OSAL_DEV_clk_is_enabled(void * pclk)
{
	struct clk *clk = (struct clk *)pclk;
	return __clk_is_enabled(clk);
}

EXPORT_SYMBOL(AX_OSAL_DEV_clk_is_enabled);

int AX_OSAL_DEV_clk_prepare_enable(void * pclk)
{
	struct clk *clk = (struct clk *)pclk;
	return clk_prepare_enable(clk);
}

EXPORT_SYMBOL(AX_OSAL_DEV_clk_prepare_enable);

int AX_OSAL_DEV_clk_set_rate(void * pclk, unsigned long rate)
{
	struct clk *clk = (struct clk *)pclk;
	return clk_set_rate(clk, rate);
}

EXPORT_SYMBOL(AX_OSAL_DEV_clk_set_rate);

unsigned long AX_OSAL_DEV_clk_get_rate(void * pclk)
{
	struct clk *clk = (struct clk *)pclk;
	return clk_get_rate(clk);
}

EXPORT_SYMBOL(AX_OSAL_DEV_clk_get_rate);

void AX_OSAL_DEV_clk_disable_unprepare(void * pclk)
{
	struct clk *clk = (struct clk *)pclk;
	clk_disable_unprepare(clk);
}

EXPORT_SYMBOL(AX_OSAL_DEV_clk_disable_unprepare);
