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
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/clk.h>

#include "osal_lib_ax.h"

#include "base.h"
#include "axdev.h"
static OSAL_LIST_HEAD(axdev_list);
static DEFINE_MUTEX(axdev_sem);
#define CLASS_NAME "ax_osal_class"
struct class *g_axdev_class;

static int axdev_open(struct inode *inode, struct file *file)
{
	unsigned minor = iminor(inode);
	//unsigned major = imajor(inode);

	struct axdev_device *c = NULL;
	int err = -ENODEV;
	const struct file_operations *old_fops, *new_fops = NULL;

	mutex_lock(&axdev_sem);
	osal_list_for_each_entry(c, &axdev_list, list) {
		if (c->minor == minor) {
			new_fops = fops_get(c->fops);
			break;
		}
	}

	if (!new_fops) {
		mutex_unlock(&axdev_sem);
		request_module("char-major-%d-%d", AXDEV_DEVICE_MAJOR, minor);
		mutex_lock(&axdev_sem);

		osal_list_for_each_entry(c, &axdev_list, list) {
			if (c->minor == minor) {
				new_fops = fops_get(c->fops);
				break;
			}
		}

		if (!new_fops)
			goto fail;
	}

	err = 0;
	old_fops = file->f_op;
	file->f_op = new_fops;
	if (file->f_op->open) {
		file->private_data = c;
		err = file->f_op->open(inode, file);
		if (err) {
			fops_put(file->f_op);
			file->private_data = NULL;
			file->f_op = fops_get(old_fops);
		}
	}

	fops_put(old_fops);
fail:
	mutex_unlock(&axdev_sem);
	return err;
}

struct file_operations axdev_fops = {
	.owner = THIS_MODULE,
	.open = axdev_open,
};

int axdev_register(struct axdev_device *axdev)
{
	struct axdev_device *ptmp = NULL;
	struct axdev_driver *pdrv = NULL;

	int err = 0;

	mutex_lock(&axdev_sem);

	/*step 1/7 check minor of axdev-deivce, if it has been registed */
	osal_list_for_each_entry(ptmp, &axdev_list, list) {
		if (ptmp->minor == axdev->minor) {
			mutex_unlock(&axdev_sem);
			printk("Func[%s], conflict with axdev minor[id =%d]\n",__func__, axdev->minor);
			return -EBUSY;
		}
	}

	/*step 2/7 get auto minor of axdev-device */
	if (axdev->minor == AXDEV_DYNAMIC_MINOR) {
		int i = AXDEV_DYNAMIC_MINOR - 1;
		while (--i >= 0)
			if ((ax_dev_minors[i >> 3] & (1 << (i & 7))) == 0)
				break;
		if (i < 0) {
			printk("Func[%s], Failed to get axdev minor[id =%d]\n",__func__, i);
			mutex_unlock(&axdev_sem);
			return -EBUSY;
		}
		axdev->minor = i;
	}

	if (axdev->minor < AXDEV_DYNAMIC_MINOR)
		ax_dev_minors[axdev->minor >> 3] |= 1 << (axdev->minor & 7);

	/*step 3/7 set class of axdev-device */
	axdev->axdev_class = g_axdev_class;

	/*step 4/7 register axdev-device */
	err = axdev_device_register(axdev);
	if (err < 0) {
		ax_dev_minors[axdev->minor >> 3] &= ~(1 << (axdev->minor & 7));
		printk("Func[%s], ERROR, Failed to register device of axdev [minor id=%d]\n",__func__, axdev->minor);
		goto out;
	}

	/*step 5/7 register driver of axdev-device */
	pdrv = axdev_driver_register(axdev->devfs_name, axdev->owner,axdev->drvops);
	if (IS_ERR(pdrv)) {
		axdev_device_unregister(axdev);
		ax_dev_minors[axdev->minor >> 3] &= ~(1 << (axdev->minor & 7));

		err = PTR_ERR(pdrv);
		printk("Func[%s], ERROR, Failed to register driver of axdev [minor id=%d]\n",__func__, axdev->minor);
		goto out;
	}

	/*step 6/7 attach to driver of axdev */
	axdev->driver = pdrv;
	/*step 7/7 insert new axdev-device to device list */
	osal_list_add(&axdev->list, &axdev_list);

out:
	mutex_unlock(&axdev_sem);
	return err;
}

EXPORT_SYMBOL(axdev_register);

int axdev_unregister(struct axdev_device *axdev)
{
	struct axdev_device *ptmp = NULL, *_ptmp = NULL;
	if (osal_list_empty(&axdev->list))
		return -EINVAL;

	mutex_lock(&axdev_sem);
	osal_list_for_each_entry_safe(ptmp, _ptmp, &axdev_list, list) {

		/*if found, unregister device & driver */
		if (ptmp->minor == axdev->minor) {
			osal_list_del(&axdev->list);
			axdev_driver_unregister(axdev->driver);
			axdev->driver = NULL;
			axdev_device_unregister(axdev);
			ax_dev_minors[axdev->minor >> 3] &= ~(1 << (axdev->minor & 7));
			break;
		}
	}
	mutex_unlock(&axdev_sem);
	return 0;
}

EXPORT_SYMBOL(axdev_unregister);

extern void osal_device_init(void);

static int axdev_init(void)
{
	int ret;
	osal_device_init();
	/*step 1/ create bus of axdev */
	ret = axdev_bus_init();
	if (ret) {
		printk("Func [%s] failed to axdev_bus_init\n", __func__);
		goto err0;
	}

	/*step 1/ create class of axdev */
	g_axdev_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(g_axdev_class)) {
		printk("%s: failed to create class\n", __func__);
		goto err0;
	}

	return 0;
err0:
	axdev_bus_exit();
	return ret;
}

static void __exit axdev_exit(void)
{
	if (!osal_list_empty(&axdev_list)) {
		printk("!!! Module axdev: sub module in list\n");
		return;
	}

	pr_info("destroy class [%s] \n", CLASS_NAME);
	class_destroy(g_axdev_class);
	g_axdev_class = NULL;
	axdev_bus_exit();

}

module_init(axdev_init);
module_exit(axdev_exit);

MODULE_AUTHOR("Axera");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");
