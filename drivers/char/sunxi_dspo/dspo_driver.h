/*
 * dspo_driver.h
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _DSPO_DRIVER_H
#define _DSPO_DRIVER_H

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include "dspo_device.h"
#include <linux/dspo_drv.h>
#include <asm/uaccess.h>


struct dspo_resource_t {
	struct device *dev;
	struct mutex mutex;
	struct dspo_device_t *p_dspo;
	bool opened;
	u64 sunxi_dspo_dma_mask;
	atomic_t user_cnt;
	struct class *dspo_class;
	struct cdev *dspo_cdev;
	dev_t dev_id;
};


#endif /*End of file*/
