// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020, allwinnertech.
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/clk/sunxi.h>
#include <linux/regulator/consumer.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <uapi/linux/dsp/dsp_debug.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/dma.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#define DSP_DEV_NUM		(1)

#define DSP_ERR(fmt, arg...)	pr_err("[linux:dsp:err]- fun:%s() line:%d - "fmt, __func__, __LINE__, ##arg)
#define DSP_INFO(fmt, arg...)	pr_info("[linux:dsp:info]- "fmt, ##arg)

#define DSP_MAX_NUM_OF_DEVICES		(2)

struct dsp_debug_dev_t{
	int dsp_id;
	int major;
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct dsp_sharespace_t *dsp_sharespace;
};

struct dsp_debug_dev_t *dsp_debug_dev[DSP_MAX_NUM_OF_DEVICES];

static ssize_t dsp_debug_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct dsp_debug_dev_t *pdata = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE, "dsp_id = %d\n"
			"dsp_sharespace>dsp_write_size = 0x%x\n"
			"dsp_sharespace>dsp_write_size = 0x%x\n"
			"dsp_sharespace>arm_write_addr = 0x%x\n"
			"dsp_sharespace>arm_write_size = 0x%x\n"
			"dsp_sharespace>dsp_log_addr = 0x%x\n"
			"dsp_sharespace>dsp_log_size = 0x%x\n"
			"dsp_sharespace>mmap_phy_addr = 0x%x\n"
			"dsp_sharespace>mmap_phy_size = 0x%x\n"
			"dsp_sharespace->value.log_head_addr = 0x%x\n"
			"dsp_sharespace->value.log_head_size = 0x%x\n",
			pdata->dsp_id,
			pdata->dsp_sharespace->dsp_write_addr,
			pdata->dsp_sharespace->dsp_write_size,
			pdata->dsp_sharespace->arm_write_addr,
			pdata->dsp_sharespace->arm_write_size,
			pdata->dsp_sharespace->dsp_log_addr,
			pdata->dsp_sharespace->dsp_log_size,
			pdata->dsp_sharespace->mmap_phy_addr,
			pdata->dsp_sharespace->mmap_phy_size,
			pdata->dsp_sharespace->value.log_head_addr,
			pdata->dsp_sharespace->value.log_head_size);
}

static struct device_attribute dsp_debug_info_attr =
		__ATTR(info, S_IRUGO, dsp_debug_info_show, NULL);

static void dsp_debug_sysfs(struct platform_device *pdev)
{
	device_create_file(&pdev->dev, &dsp_debug_info_attr);
}

static void dsp_debug_remove_sysfs(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &dsp_debug_info_attr);
}

static int32_t check_addr_valid(uint32_t addr)
{
	int ret = 0;
	if ((addr & (~0xFFFU)) != addr)
		ret = -1;

	return ret;
}

static int sharespace_check_addr(struct dsp_sharespace_t *msg)
{
	int ret = 1;

	ret = check_addr_valid(msg->dsp_write_addr);
	if (ret < 0) {
		DSP_ERR("dsp_write_addr fail to check\n");
		return ret;
	}

	ret = check_addr_valid(msg->dsp_write_addr + msg->dsp_write_size);
	if (ret < 0) {
		DSP_ERR("dsp_write_size fail to check\n");
		return ret;
	}

	ret = check_addr_valid(msg->arm_write_addr);
	if (ret < 0) {
		DSP_ERR("arm_write_addr fail to check\n");
		return ret;
	}

	ret = check_addr_valid(msg->arm_write_addr + msg->arm_write_size);
	if (ret < 0) {
		DSP_ERR("dsp_write_size fail to check\n");
		return ret;
	}

	ret = check_addr_valid(msg->dsp_log_addr);
	if (ret < 0) {
		DSP_ERR("dsp_log_addr fail to check\n");
		return ret;
	}

	ret = check_addr_valid(msg->dsp_log_addr + msg->dsp_log_size);
	if (ret < 0) {
		DSP_ERR("dsp_log_size fail to check\n");
		return ret;
	}

    return ret;
}

static int dsp_debug_dev_open(struct inode *inode, struct file *file)
{
	struct dsp_debug_dev_t *dsp_dev = container_of(inode->i_cdev, struct dsp_debug_dev_t, cdev);

	file->private_data = dsp_dev;
	return 0;
}

static int dsp_debug_dev_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct dsp_debug_dev_t *dsp_dev = file->private_data;

	vma->vm_flags |= VM_IO;
	/* addr 4K align */
	vma->vm_pgoff = dsp_dev->dsp_sharespace->mmap_phy_addr >> PAGE_SHIFT;
	if (remap_pfn_range(vma,
			vma->vm_start,
			vma->vm_pgoff,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot)) {
		DSP_ERR("sharespace mmap fail\n");
		return -EAGAIN;
	}
	return 0;
}

static long dsp_debug_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 1;
	struct dsp_debug_dev_t *dsp_dev = file->private_data;

	switch (cmd) {
	case CMD_REFRESH_LOG_HEAD_ADDR:
		dsp_dev->dsp_sharespace->value.log_head_addr = dsp_dev->dsp_sharespace->dsp_log_addr;
		break;

	case CMD_READ_DEBUG_MSG:
		ret = copy_to_user((void __user *) arg, (void *)dsp_dev->dsp_sharespace,\
					sizeof(struct dsp_sharespace_t));
		if (ret < 0) {
			DSP_ERR("copy dsp_sharespace msg to user fail\n");
			ret = -EFAULT;
		}
		break;

	case CMD_WRITE_DEBUG_MSG:
		ret = copy_from_user((void *)dsp_dev->dsp_sharespace, (void __user *) arg,\
					sizeof(struct dsp_sharespace_t));
		if (ret < 0) {
			DSP_ERR("copy dsp_sharespace msg to user fail\n");
			return -EFAULT;
		}
		break;

	default:
		break;
	}
	return ret;
}

static struct file_operations dsp_debug_fops = {
	.owner = THIS_MODULE,
	.open = dsp_debug_dev_open,
	.mmap = dsp_debug_dev_mmap,
	.unlocked_ioctl = dsp_debug_dev_ioctl,
};

static int dsp_debug_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	uint32_t regdata[8];
	int ret = 0;
	int dsp_id = 0;
	struct dsp_debug_dev_t *pdebugdev = NULL;
	char str[30];

	DSP_INFO("dsp sharespace info\n");
	/* copy msg from dts */
	if (np == NULL) {
		DSP_ERR("sharespace fail to get of_node\n");
		goto err0;
	}

	/* find dsp id */
	ret = of_property_read_u32(np, "dsp_id", &dsp_id);
	if (ret < 0) {
		DSP_ERR("reg property failed to read\n");
		goto err0;
	}

	if (dsp_id >= DSP_MAX_NUM_OF_DEVICES) {
		DSP_ERR("dsp_id value err\n");
		goto err0;
	}

	if (dsp_debug_dev[dsp_id] != NULL) {
		DSP_ERR("dsp_id has been used\n");
		goto err0;
	}

	dsp_debug_dev[dsp_id] = kzalloc(sizeof(struct dsp_debug_dev_t), GFP_KERNEL);
	if (dsp_debug_dev[dsp_id] == NULL) {
		DSP_ERR("dsp_debug_dev failed to alloc mem");
		goto err0;
	}
	pdebugdev = dsp_debug_dev[dsp_id];
	pdebugdev->dsp_id = dsp_id;

	pdebugdev->dsp_sharespace = kzalloc(sizeof(struct dsp_sharespace_t), GFP_KERNEL);
	if (pdebugdev->dsp_sharespace == NULL) {
		DSP_ERR("dsp_sharespace failed to alloc mem");
		goto err1;
	}

	memset(regdata, 0, sizeof(regdata));
	ret = of_property_read_u32_array(np, "reg", regdata, 8);
	if (ret < 0) {
		DSP_ERR("reg property failed to read\n");
		goto err2;
	}

	pdebugdev->dsp_sharespace->dsp_write_addr = regdata[0];
	pdebugdev->dsp_sharespace->arm_write_addr = regdata[2];
	pdebugdev->dsp_sharespace->dsp_log_addr = regdata[4];
	pdebugdev->dsp_sharespace->dsp_write_size = regdata[1];
	pdebugdev->dsp_sharespace->arm_write_size = regdata[3];
	pdebugdev->dsp_sharespace->dsp_log_size = regdata[5];

	pdebugdev->dsp_sharespace->arm_read_dsp_log_addr = pdebugdev->dsp_sharespace->dsp_log_addr;
	pdebugdev->dsp_sharespace->value.log_head_addr = pdebugdev->dsp_sharespace->dsp_log_addr;
	DSP_INFO("log_head_addr %x\n", pdebugdev->dsp_sharespace->value.log_head_addr);


	/* check msg value valid */
	ret = sharespace_check_addr(pdebugdev->dsp_sharespace);
	if (ret < 0) {
		DSP_ERR("sharespace addr failed to check\n");
		goto err2;
	}

	memset(str, 0, sizeof(str));
	sprintf(str, "dsp_debug%d", pdebugdev->dsp_id);
	/* set dev mun */
	if (pdebugdev->major) {
		pdebugdev->devid = MKDEV(pdebugdev->major, 0);
		register_chrdev_region(pdebugdev->devid, DSP_DEV_NUM, str);
	} else {
		alloc_chrdev_region(&pdebugdev->devid, 0, DSP_DEV_NUM, str);
		pdebugdev->major = MAJOR(pdebugdev->devid);
	}

	/* add dev  */
	cdev_init(&pdebugdev->cdev, &dsp_debug_fops);
	cdev_add(&pdebugdev->cdev, pdebugdev->devid, DSP_DEV_NUM);

	/* create class */
	pdebugdev->class = class_create(THIS_MODULE, str);
	if (IS_ERR(pdebugdev->class)) {
		DSP_ERR("class failed to create\n");
		goto err3;
	}

	/* create device */
	pdebugdev->device = device_create(pdebugdev->class,\
							NULL, pdebugdev->devid,\
							NULL, str);
	if (IS_ERR(pdebugdev->device)) {
		DSP_ERR("device failed to create\n");
		goto err4;
	}

	platform_set_drvdata(pdev, pdebugdev);
	dsp_debug_sysfs(pdev);
	DSP_INFO("dsp%d, dev name = %s, probe ok\n", pdebugdev->dsp_id, str);
	return 0;

err4:
	class_destroy(pdebugdev->class);
err3:
	unregister_chrdev_region(pdebugdev->devid, DSP_DEV_NUM);
	cdev_del(&pdebugdev->cdev);
err2:
	kfree(pdebugdev->dsp_sharespace);
err1:
	kfree(pdebugdev);
err0:

	return 0;
}

static int dsp_debug_remove(struct platform_device *pdev)
{
	struct dsp_debug_dev_t *pdebugdev = platform_get_drvdata(pdev);
	DSP_INFO("dsp%d, debug remove info\n", pdebugdev->dsp_id);
	dsp_debug_remove_sysfs(pdev);
	cdev_del(&pdebugdev->cdev);
	unregister_chrdev_region(pdebugdev->devid, DSP_DEV_NUM);
	device_destroy(pdebugdev->class, pdebugdev->devid);
	class_destroy(pdebugdev->class);
	kfree(pdebugdev->dsp_sharespace);
	kfree(pdebugdev);
	DSP_INFO("debug remove ok\n");
	return 0;
}

static const struct of_device_id dsp_debug_match[] = {
	{ .compatible = "allwinner,sun8iw20p1-dsp-share-space" },
	{ .compatible = "allwinner,sun50iw11p1-dsp-share-space" },
	{},
};

MODULE_DEVICE_TABLE(of, dsp_debug_match);

static struct platform_driver dsp_debug_driver = {
	.probe   = dsp_debug_probe,
	.remove  = dsp_debug_remove,
	.driver = {
		.name	= "dsp_debug",
		.owner	= THIS_MODULE,
		.pm		= NULL,
		.of_match_table = dsp_debug_match,
	},
};

static int __init dsp_debug_init(void)
{
	return platform_driver_register(&dsp_debug_driver);
}
fs_initcall_sync(dsp_debug_init);

static void __exit dsp_debug_exit(void)
{
	platform_driver_unregister(&dsp_debug_driver);
}


module_exit(dsp_debug_exit);
//module_param_named(debug, debug_mask, int, 0664);

MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("dsp debug module");
MODULE_ALIAS("platform:dsp_debug");
MODULE_LICENSE("GPL v2");
