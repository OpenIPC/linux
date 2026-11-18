/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _LINUX_AXDEV_DEVICE_H_
#define _LINUX_AXDEV_DEVICE_H_

#include <linux/module.h>
#include <linux/major.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include "osal_list.h"

#define AXDEV_DEVICE_MAJOR  218
#define AXDEV_DYNAMIC_MINOR 2048

extern unsigned short ax_dev_minors[AXDEV_DYNAMIC_MINOR / 8];

struct axdev_device;

struct axdev_ops {
	//pm methos
	int (*pm_prepare) (struct axdev_device *);
	void (*pm_complete) (struct axdev_device *);

	int (*pm_suspend) (struct axdev_device *);
	int (*pm_resume) (struct axdev_device *);

	int (*pm_freeze) (struct axdev_device *);
	int (*pm_thaw) (struct axdev_device *);
	int (*pm_poweroff) (struct axdev_device *);
	int (*pm_restore) (struct axdev_device *);

	int (*pm_suspend_late) (struct axdev_device *);
	int (*pm_resume_early) (struct axdev_device *);
	int (*pm_freeze_late) (struct axdev_device *);
	int (*pm_thaw_early) (struct axdev_device *);
	int (*pm_poweroff_late) (struct axdev_device *);
	int (*pm_restore_early) (struct axdev_device *);

	int (*pm_suspend_noirq) (struct axdev_device *);
	int (*pm_resume_noirq) (struct axdev_device *);

	int (*pm_freeze_noirq) (struct axdev_device *);
	int (*pm_thaw_noirq) (struct axdev_device *);
	int (*pm_poweroff_noirq) (struct axdev_device *);
	int (*pm_restore_noirq) (struct axdev_device *);
};

#define HIMIDIA_MAX_DEV_NAME_LEN 32

struct axdev_driver {
	struct device_driver driver;
	struct axdev_ops *ops;
	char name[1];
};

#define to_axdev_driver(drv)    \
    container_of((drv), struct axdev_driver, driver)

struct axdev_device {
	struct osal_list_head list;
	char devfs_name[HIMIDIA_MAX_DEV_NAME_LEN];
	unsigned int major;
	unsigned int minor;
	struct device device;
	struct module *owner;
	const struct file_operations *fops;
	struct axdev_ops *drvops;

	/*for internal use */
	struct axdev_driver *driver;
	struct class *axdev_class;
	struct cdev cdev;
	dev_t devt;
};

#define to_axdev_device(dev) \
    container_of((dev), struct axdev_device, device)

int axdev_register(struct axdev_device *pdev);

int axdev_unregister(struct axdev_device *pdev);

#define MODULE_ALIAS_AXDEV(minor) \
    MODULE_ALIAS("axdev-char-major-" __stringify(AXDEV_DEVICE_MAJOR) \
    "-" __stringify(minor))

#endif /*_LINUX_AXDEV_DEVICE_H_*/
