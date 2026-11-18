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
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/kmod.h>
#include <linux/fs.h>

#include <linux/wait.h>

#include "linux/platform_device.h"
#include "linux/device.h"
#include "osal_ax.h"
#include "osal_dev_ax.h"
#include "axdev.h"
#include "axdev_log.h"
#include "osal_lib_ax.h"

static int axera_osal_common_probe(struct platform_device *pdev)
{
	struct platform_driver *axera_osal_platform_drv = NULL;
	struct AX_PLATFORM_DRIVER *axera_pdrv = NULL;
	struct device_driver *drv = pdev->dev.driver;
	if (drv == NULL)
		return -ENODEV;
	axera_osal_platform_drv = to_platform_driver(drv);
	if (axera_osal_platform_drv == NULL)
		return -EINVAL;
	axera_pdrv = (struct AX_PLATFORM_DRIVER *)axera_osal_platform_drv->axera_driver_ptr;
	if (axera_pdrv == NULL)
		return -EINVAL;
	if (axera_pdrv->probe == NULL)
		return -EINVAL;
	axera_pdrv->probe(pdev);
	return 0;
}

static int axera_osal_common_remove(struct platform_device *pdev)
{
	struct platform_driver *axera_osal_platform_drv = NULL;
	struct AX_PLATFORM_DRIVER *axera_pdrv = NULL;
	struct device_driver *drv = pdev->dev.driver;
	if (drv == NULL)
		return -EINVAL;
	axera_osal_platform_drv = to_platform_driver(drv);
	if (axera_osal_platform_drv == NULL)
		return -EINVAL;
	axera_pdrv = (struct AX_PLATFORM_DRIVER *)axera_osal_platform_drv->axera_driver_ptr;
	if (axera_pdrv == NULL)
		return -EINVAL;
	if (axera_pdrv->remove == NULL)
		return -EINVAL;
	axera_pdrv->remove(pdev);
	return 0;
}

static int axera_osal_common_pm_suspend(struct device *dev)
{
	struct platform_driver *axera_osal_platform_drv = NULL;
	struct AX_PLATFORM_DRIVER *axera_pdrv = NULL;
	struct platform_device *pdev = to_platform_device(dev);
	struct device_driver *drv = pdev->dev.driver;

	if (IS_ERR_OR_NULL(drv))
		return -ENODEV;
	axera_osal_platform_drv = to_platform_driver(drv);
	if (IS_ERR_OR_NULL(axera_osal_platform_drv))
		return -EINVAL;
	axera_pdrv = (struct AX_PLATFORM_DRIVER *)axera_osal_platform_drv->axera_driver_ptr;
	if (IS_ERR_OR_NULL(axera_pdrv))
		return -EINVAL;
	if (IS_ERR_OR_NULL(axera_pdrv->suspend))
		return -EINVAL;
	axera_pdrv->suspend(pdev);
	return 0;
}

static int axera_osal_common_pm_resume(struct device *dev)
{
	struct platform_driver *axera_osal_platform_drv = NULL;
	struct AX_PLATFORM_DRIVER *axera_pdrv = NULL;
	struct platform_device *pdev = to_platform_device(dev);
	struct device_driver *drv = pdev->dev.driver;

	if (IS_ERR_OR_NULL(drv))
		return -ENODEV;
	axera_osal_platform_drv = to_platform_driver(drv);
	if (IS_ERR_OR_NULL(axera_osal_platform_drv))
		return -EINVAL;
	axera_pdrv = (struct AX_PLATFORM_DRIVER *)axera_osal_platform_drv->axera_driver_ptr;
	if (IS_ERR_OR_NULL(axera_pdrv))
		return -EINVAL;
	if (IS_ERR_OR_NULL(axera_pdrv->resume))
		return -EINVAL;
	axera_pdrv->resume(pdev);
	return 0;
}

static int axera_osal_common_pm_suspend_noirq(struct device *dev)
{
	struct platform_driver *axera_osal_platform_drv = NULL;
	struct AX_PLATFORM_DRIVER *axera_pdrv = NULL;
	struct platform_device *pdev = to_platform_device(dev);
	struct device_driver *drv = pdev->dev.driver;

	if (IS_ERR_OR_NULL(drv))
		return -ENODEV;
	axera_osal_platform_drv = to_platform_driver(drv);
	if (IS_ERR_OR_NULL(axera_osal_platform_drv))
		return -EINVAL;
	axera_pdrv = (struct AX_PLATFORM_DRIVER *)axera_osal_platform_drv->axera_driver_ptr;
	if (IS_ERR_OR_NULL(axera_pdrv))
		return -EINVAL;
	if (IS_ERR_OR_NULL(axera_pdrv->suspend_noirq))
		return -EINVAL;
	axera_pdrv->suspend_noirq(pdev);
	return 0;
}

static int axera_osal_common_pm_resume_early(struct device *dev)
{
	struct platform_driver *axera_osal_platform_drv = NULL;
	struct AX_PLATFORM_DRIVER *axera_pdrv = NULL;
	struct platform_device *pdev = to_platform_device(dev);
	struct device_driver *drv = pdev->dev.driver;

	if (IS_ERR_OR_NULL(drv))
		return -ENODEV;
	axera_osal_platform_drv = to_platform_driver(drv);
	if (IS_ERR_OR_NULL(axera_osal_platform_drv))
		return -EINVAL;
	axera_pdrv = (struct AX_PLATFORM_DRIVER *)axera_osal_platform_drv->axera_driver_ptr;
	if (IS_ERR_OR_NULL(axera_pdrv))
		return -EINVAL;
	if (IS_ERR_OR_NULL(axera_pdrv->resume_early))
		return -EINVAL;
	axera_pdrv->resume_early(pdev);
	return 0;
}

static int axera_osal_common_pm_suspend_late(struct device *dev)
{
	struct platform_driver *axera_osal_platform_drv = NULL;
	struct AX_PLATFORM_DRIVER *axera_pdrv = NULL;
	struct platform_device *pdev = to_platform_device(dev);
	struct device_driver *drv = pdev->dev.driver;

	if (IS_ERR_OR_NULL(drv))
		return -ENODEV;
	axera_osal_platform_drv = to_platform_driver(drv);
	if (IS_ERR_OR_NULL(axera_osal_platform_drv))
		return -EINVAL;
	axera_pdrv = (struct AX_PLATFORM_DRIVER *)axera_osal_platform_drv->axera_driver_ptr;
	if (IS_ERR_OR_NULL(axera_pdrv))
		return -EINVAL;
	if (IS_ERR_OR_NULL(axera_pdrv->suspend_late))
		return -EINVAL;
	axera_pdrv->suspend_late(pdev);
	return 0;
}

static int axera_osal_common_pm_resume_noirq(struct device *dev)
{
	struct platform_driver *axera_osal_platform_drv = NULL;
	struct AX_PLATFORM_DRIVER *axera_pdrv = NULL;
	struct platform_device *pdev = to_platform_device(dev);
	struct device_driver *drv = pdev->dev.driver;

	if (IS_ERR_OR_NULL(drv))
		return -ENODEV;
	axera_osal_platform_drv = to_platform_driver(drv);
	if (IS_ERR_OR_NULL(axera_osal_platform_drv))
		return -EINVAL;
	axera_pdrv = (struct AX_PLATFORM_DRIVER *)axera_osal_platform_drv->axera_driver_ptr;
	if (IS_ERR_OR_NULL(axera_pdrv))
		return -EINVAL;
	if (IS_ERR_OR_NULL(axera_pdrv->resume_noirq))
		return -EINVAL;
	axera_pdrv->resume_noirq(pdev);
	return 0;
}



int AX_OSAL_DEV_platform_driver_register(void * drv)
{
	struct dev_pm_ops *axera_pm_ops;
	struct platform_driver *axera_osal_platform_drv = NULL;
	struct AX_PLATFORM_DRIVER *axera_pdrv = (struct AX_PLATFORM_DRIVER *)drv;
	if (axera_pdrv == NULL)
		return -EINVAL;
	axera_osal_platform_drv = kzalloc(sizeof(struct platform_driver), GFP_KERNEL);
	if (axera_osal_platform_drv == NULL)
		return -ENOMEM;
	axera_pdrv->axera_ptr = axera_osal_platform_drv;
	axera_osal_platform_drv->probe = axera_osal_common_probe;
	axera_osal_platform_drv->remove = axera_osal_common_remove;
	axera_osal_platform_drv->driver.name = axera_pdrv->driver.name;
	axera_osal_platform_drv->driver.of_match_table = axera_pdrv->driver.of_match_table;
#ifdef CONFIG_PM
	if(axera_pdrv->suspend != NULL || axera_pdrv->resume != NULL ||
		axera_pdrv->suspend_noirq != NULL || axera_pdrv->resume_noirq != NULL ||
		axera_pdrv->suspend_late != NULL || axera_pdrv->resume_early != NULL ) {
		axera_pm_ops = kzalloc(sizeof(struct dev_pm_ops), GFP_KERNEL);
		if(axera_pm_ops == NULL)
			return -ENOMEM;
		if(axera_pdrv->suspend != NULL)
			axera_pm_ops->suspend = axera_osal_common_pm_suspend;
		if(axera_pdrv->resume != NULL)
			axera_pm_ops->resume = axera_osal_common_pm_resume;

		if(axera_pdrv->suspend_noirq != NULL)
			axera_pm_ops->suspend_noirq = axera_osal_common_pm_suspend_noirq;
		if(axera_pdrv->resume_noirq != NULL)
			axera_pm_ops->resume_noirq = axera_osal_common_pm_resume_noirq;

		if(axera_pdrv->suspend_late != NULL)
			axera_pm_ops->suspend_late = axera_osal_common_pm_suspend_late;
		if(axera_pdrv->resume_early != NULL)
			axera_pm_ops->resume_early = axera_osal_common_pm_resume_early;

		axera_osal_platform_drv->driver.pm = axera_pm_ops;
	} else {
		axera_osal_platform_drv->driver.pm = NULL;
	}
#endif
	axera_osal_platform_drv->axera_driver_ptr = drv;
	return __platform_driver_register(axera_osal_platform_drv, THIS_MODULE);
}

EXPORT_SYMBOL(AX_OSAL_DEV_platform_driver_register);

void AX_OSAL_DEV_platform_driver_unregister(void * drv)
{
	struct AX_PLATFORM_DRIVER *axera_pdrv = (struct AX_PLATFORM_DRIVER *)drv;
	struct platform_driver *axera_osal_platform_drv = axera_pdrv->axera_ptr;
	if (axera_osal_platform_drv == NULL) {
		printk("platform_driver axera_osal_platform_drv is invalid\n");
		return;
	}

	platform_driver_unregister(axera_osal_platform_drv);

	if(axera_osal_platform_drv->driver.pm != NULL)
		kfree(axera_osal_platform_drv->driver.pm);
	kfree(axera_osal_platform_drv);
}

EXPORT_SYMBOL(AX_OSAL_DEV_platform_driver_unregister);

int AX_OSAL_DEV_platform_get_resource_byname(void * dev, u32 type, const char * name,
						struct AXERA_RESOURCE * res)
{
	struct resource *res_tmp = NULL;
	res_tmp = platform_get_resource_byname((struct platform_device *)dev, type, name);
	if (!res_tmp) {
		return -EINVAL;
	} else {
		AX_OSAL_LIB_memset(res, 0, sizeof(struct AXERA_RESOURCE));
		res->start = res_tmp->start;
		res->end = res_tmp->end;
		AX_OSAL_LIB_memcpy(res->name, res_tmp->name, AX_OSAL_LIB_strlen(res_tmp->name));
		res->flags = res_tmp->flags;
		res->desc = res_tmp->desc;
	}
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_DEV_platform_get_resource_byname);

int AX_OSAL_DEV_platform_get_resource(void * dev, u32 type, u32 num, struct AXERA_RESOURCE * res)
{
	struct resource *res_tmp = NULL;
	res_tmp = platform_get_resource((struct platform_device *)dev, type, num);
	if (!res_tmp) {
		return -EINVAL;
	} else {
		AX_OSAL_LIB_memset(res, 0, sizeof(struct AXERA_RESOURCE));
		res->start = res_tmp->start;
		res->end = res_tmp->end;
		AX_OSAL_LIB_memcpy(res->name, res_tmp->name, AX_OSAL_LIB_strlen(res_tmp->name));
		res->flags = res_tmp->flags;
		res->desc = res_tmp->desc;
	}
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_DEV_platform_get_resource);

int AX_OSAL_DEV_platform_get_irq(void * dev, u32 num)
{
	return platform_get_irq((struct platform_device *)dev, num);
}

EXPORT_SYMBOL(AX_OSAL_DEV_platform_get_irq);

int AX_OSAL_DEV_platform_get_irq_byname(void * dev, const char * name)
{
	return platform_get_irq_byname((struct platform_device *)dev, name);
}

EXPORT_SYMBOL(AX_OSAL_DEV_platform_get_irq_byname);

unsigned long AX_OSAL_DEV_resource_size(const struct AXERA_RESOURCE *res)
{
	return res->end - res->start + 1;
}

EXPORT_SYMBOL(AX_OSAL_DEV_resource_size);

void *AX_OSAL_DEV_platform_get_drvdata(void * pdev)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	return platform_get_drvdata(pvdev);
}

EXPORT_SYMBOL(AX_OSAL_DEV_platform_get_drvdata);

void AX_OSAL_DEV_platform_set_drvdata(void * pdev, void * data)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	platform_set_drvdata(pvdev, data);
}

EXPORT_SYMBOL(AX_OSAL_DEV_platform_set_drvdata);

int AX_OSAL_DEV_platform_irq_count(void * pdev)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	return platform_irq_count(pvdev);
}

EXPORT_SYMBOL(AX_OSAL_DEV_platform_irq_count);

void *AX_OSAL_DEV_to_platform_device(void *dev)
{
	void *pdev = (void *)to_platform_device((struct device *)dev);
	return pdev;
}

EXPORT_SYMBOL(AX_OSAL_DEV_to_platform_device);

void *AX_OSAL_DEV_to_platform_driver(void *drv)
{
	void *pdrv = (void *)to_platform_driver((struct device_driver *)drv);
	return pdrv;
}

EXPORT_SYMBOL(AX_OSAL_DEV_to_platform_driver);
