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
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/kobject.h>
#include <linux/device.h>

#include "axdev_log.h"
#include "base.h"
/*****************************************************************************/
/**** axdev bus  ****/
/*****************************************************************************/
static void axdev_bus_release(struct device *dev)
{
	//printk("axdev bus release\n");
	return;
}

struct device axdev_bus = {
	.init_name = "axdev",
	.release = axdev_bus_release
};


/*bus match & uevent*/
static int axdev_match(struct device *dev, struct device_driver *drv)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	return (strncmp(pdev->devfs_name, drv->name, sizeof(pdev->devfs_name)) == 0);
}

static int axdev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	add_uevent_var(env, "MODALIAS=axdev:%s", pdev->devfs_name);
	return 0;
}

/*****************************************************************************/
//pm methods
static int axdev_pm_prepare(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);

	if (!pdrv->ops || !pdrv->ops->pm_prepare) {
		return 0;
	}

	return pdrv->ops->pm_prepare(pdev);
}

static void axdev_pm_complete(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);

	if (!pdrv->ops || !pdrv->ops->pm_complete) {
		return;
	}

	pdrv->ops->pm_complete(pdev);
}

static int axdev_pm_suspend(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);

	if (!pdrv->ops || !pdrv->ops->pm_suspend) {
		return 0;
	}

	return pdrv->ops->pm_suspend(pdev);
}

static int axdev_pm_resume(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);

	if (!pdrv->ops || !pdrv->ops->pm_resume) {
		return 0;
	}

	return pdrv->ops->pm_resume(pdev);
}

static int axdev_pm_freeze(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);
	if (!pdrv->ops || !pdrv->ops->pm_freeze) {
		return 0;
	}

	return pdrv->ops->pm_freeze(pdev);
}

static int axdev_pm_thaw(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);

	if (!pdrv->ops || !pdrv->ops->pm_thaw) {
		return 0;
	}

	return pdrv->ops->pm_thaw(pdev);
}

static int axdev_pm_poweroff(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);
	if (!pdrv->ops || !pdrv->ops->pm_poweroff) {
		return 0;
	}

	return pdrv->ops->pm_poweroff(pdev);
}

static int axdev_pm_restore(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);

	if (!pdrv->ops || !pdrv->ops->pm_restore) {
		return 0;
	}

	return pdrv->ops->pm_restore(pdev);
}

static int axdev_pm_suspend_noirq(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);

	if (!pdrv->ops || !pdrv->ops->pm_suspend_noirq) {
		return 0;
	}

	return pdrv->ops->pm_suspend_noirq(pdev);
}

static int axdev_pm_resume_noirq(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);

	if (!pdrv->ops || !pdrv->ops->pm_resume_noirq) {
		return 0;
	}

	return pdrv->ops->pm_resume_noirq(pdev);
}

static int axdev_pm_freeze_noirq(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);

	if (!pdrv->ops || !pdrv->ops->pm_freeze_noirq) {
		return 0;
	}

	return pdrv->ops->pm_freeze_noirq(pdev);
}

static int axdev_pm_thaw_noirq(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);

	if (!pdrv->ops || !pdrv->ops->pm_thaw_noirq) {
		return 0;
	}

	return pdrv->ops->pm_thaw_noirq(pdev);
}

static int axdev_pm_poweroff_noirq(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);

	if (!pdrv->ops || !pdrv->ops->pm_poweroff_noirq) {
		return 0;
	}

	return pdrv->ops->pm_poweroff_noirq(pdev);
}

static int axdev_pm_restore_noirq(struct device *dev)
{
	struct axdev_device *pdev = to_axdev_device(dev);
	struct axdev_driver *pdrv = to_axdev_driver(dev->driver);

	if (!pdrv->ops || !pdrv->ops->pm_restore_noirq) {
		return 0;
	}

	return pdrv->ops->pm_restore_noirq(pdev);
}

static struct dev_pm_ops axdev_bus_pm_ops = {
	.prepare = axdev_pm_prepare,
	.complete = axdev_pm_complete,

	//with irq
	.suspend = axdev_pm_suspend,
	.resume = axdev_pm_resume,

	.freeze = axdev_pm_freeze,
	.thaw = axdev_pm_thaw,
	.poweroff = axdev_pm_poweroff,
	.restore = axdev_pm_restore,

	//with noirq
	.suspend_noirq = axdev_pm_suspend_noirq,
	.resume_noirq = axdev_pm_resume_noirq,
	.freeze_noirq = axdev_pm_freeze_noirq,
	.thaw_noirq = axdev_pm_thaw_noirq,
	.poweroff_noirq = axdev_pm_poweroff_noirq,
	.restore_noirq = axdev_pm_restore_noirq,
};

struct bus_type axdev_bus_type = {
	.name = "axosalBus",
	//.dev_attrs    = axdev_dev_attrs,
	.match = axdev_match,
	.uevent = axdev_uevent,
	.pm = &axdev_bus_pm_ops,
};

int axdev_bus_init(void)
{
	int ret;
	ret = device_register(&axdev_bus);
	if (ret)
		return ret;

	ret = bus_register(&axdev_bus_type);
	if (ret)
		goto error;

	return 0;
error:

	device_unregister(&axdev_bus);
	return ret;
}

void axdev_bus_exit(void)
{
	bus_unregister(&axdev_bus_type);
	device_unregister(&axdev_bus);
}

static void axdev_device_release(struct device *dev)
{
}

extern struct file_operations axdev_fops;

int axdev_device_register(struct axdev_device *pdev)
{
	dev_t dev_id;
	int rval = 0;

	/*step 1/6, generate dev_id by major and minor of device */
	dev_id = MKDEV(pdev->major, pdev->minor);
	/*step 2/6, register dev_id and dev_name to device FW */
	rval = register_chrdev_region(dev_id, 1, pdev->devfs_name);
	if (rval) {
		printk("failed to get dev region for %s.\n",
				pdev->devfs_name);
		return rval;
	}

	/*step 3/6,only initialzie cdev */
	cdev_init(&pdev->cdev, &axdev_fops);
	pdev->cdev.owner = THIS_MODULE;
	/*step 4/6,add cdev to device FW, dev_id mapped to cdev */
	rval = cdev_add(&pdev->cdev, dev_id, 1);
	if (rval) {
		printk("cdev_add failed for %s, error = %d.\n",
			      pdev->devfs_name, rval);
		goto FAIL;
	}

	/*step 5/6,initialize device's parameter manually */
	pdev->device.devt = dev_id;
	pdev->device.parent = NULL;
	dev_set_drvdata(&pdev->device, NULL);

	//dev_set_name(&pdev->device,pdev->devfs_name );
	dev_set_name(&pdev->device, "%s", pdev->devfs_name);
	pdev->device.release = axdev_device_release;
	pdev->device.bus = &axdev_bus_type;

	pdev->devt = dev_id;
	/*step 6/6,create a device and register it with sysfs */
	rval = device_register(&pdev->device);
	if (rval) {
		printk("device_register failed for %s, error = %d.\n",pdev->devfs_name, rval);
		goto FAIL1;
	}

	return rval;
FAIL1:
	cdev_del(&pdev->cdev);
FAIL:
	unregister_chrdev_region(dev_id, 1);
	return rval;
}

void axdev_device_unregister(struct axdev_device *pdev)
{
	printk("begin to unregister axmeida-device\n");

	device_unregister(&pdev->device);
	cdev_del(&pdev->cdev);
	unregister_chrdev_region(pdev->devt, 1);
}

struct axdev_driver *axdev_driver_register(const char *name,struct module *owner,struct axdev_ops *pmops)
{
	int ret;
	struct axdev_driver *pdrv;

	if ((name == NULL) /*|| (owner == NULL) ||(ops == NULL) */ )
		return ERR_PTR(-EINVAL);

	pdrv = kzalloc(sizeof(struct axdev_driver) + strnlen(name, HIMIDIA_MAX_DEV_NAME_LEN), GFP_KERNEL);
	if (!pdrv)
		return ERR_PTR(-ENOMEM);

	/*init driver object */
	strncpy(pdrv->name, name, strnlen(name, HIMIDIA_MAX_DEV_NAME_LEN));

	pdrv->ops = pmops;
	pdrv->driver.name = pdrv->name;
	pdrv->driver.owner = THIS_MODULE;
	pdrv->driver.bus = &axdev_bus_type;

	ret = driver_register(&pdrv->driver);
	if (ret) {
		printk("Error, Failed to register driver[%s] \n", name);
		kfree(pdrv);
		return ERR_PTR(ret);
	}

	return pdrv;
}

void axdev_driver_unregister(struct axdev_driver *pdrv)
{
	if (pdrv) {
		driver_unregister(&pdrv->driver);
		kfree(pdrv);
	}
}
