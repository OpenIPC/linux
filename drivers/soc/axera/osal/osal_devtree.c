/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/kmod.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include "linux/platform_device.h"
#include "linux/device.h"

#include "osal_ax.h"

#include "osal_dev_ax.h"
#include "axdev.h"
#include "axdev_log.h"


int AX_OSAL_DEV_of_property_read_string(void * pdev, const char *propname,const char **out_string)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	struct device *dev = &pvdev->dev;
	return of_property_read_string(dev->of_node, propname, out_string);
}

EXPORT_SYMBOL(AX_OSAL_DEV_of_property_read_string);

bool AX_OSAL_DEV_of_property_read_bool(void * pdev, const char * propname)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	struct device *dev = &pvdev->dev;
	return of_property_read_bool(dev->of_node, propname);
}

EXPORT_SYMBOL(AX_OSAL_DEV_of_property_read_bool);

int AX_OSAL_DEV_of_property_read_u32(void * pdev, const char *propname, unsigned int * out_value)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	struct device *dev = &pvdev->dev;
	return of_property_read_u32(dev->of_node, propname, out_value);
}

EXPORT_SYMBOL(AX_OSAL_DEV_of_property_read_u32);

int AX_OSAL_DEV_of_property_read_s32(void * pdev, const char * propname, int * out_value)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	struct device *dev = &pvdev->dev;
	return of_property_read_s32(dev->of_node, propname, out_value);
}

EXPORT_SYMBOL(AX_OSAL_DEV_of_property_read_s32);
