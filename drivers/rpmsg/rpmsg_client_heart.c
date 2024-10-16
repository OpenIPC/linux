/*
 * Remote processor messaging - sample heartbeat client driver
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Copyright (C) 2011 Allwinner, Inc.
 *
 * Ohad Ben-Cohen <ohad@wizery.com>
 * Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/workqueue.h>
#include <linux/remoteproc.h>
#include <linux/timer.h>
#include "linux/types.h"

#define HEART_RATE		(2 * HZ)
#define TICK_INTERVAL	100

extern int sunxi_rproc_report_crash(char *name, enum rproc_crash_type type);

struct rpmsg_client_heartbeat {
	struct rpmsg_device *rpdev;
	struct timer_list timer;
	uint32_t tick;
	char master[32];
	u8 sleep;
};

struct hearbeat_packet {
	char name[32];
	uint32_t tick;
};

static void time_out_handler(unsigned long arg)
{
	struct rpmsg_client_heartbeat *idata =
			(struct rpmsg_client_heartbeat *)arg;

	if (!arg)
		return;

	if (idata->sleep)
		return;

	sunxi_rproc_report_crash(idata->master, RPROC_WATCHDOG);
}

static int rpmsg_heartbeat_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct rpmsg_client_heartbeat *idata = dev_get_drvdata(&rpdev->dev);
	struct hearbeat_packet *pack = data;

	dev_dbg(&rpdev->dev, "%s heartbeat: %d\n", pack->name, pack->tick);

	if (idata->master[0] == '\0') {
		memcpy(idata->master, pack->name, 32);
		idata->master[31] = '\0';
	}

	if (idata->sleep) {
		dev_err(&rpdev->dev, "%s invalid heartbeat\n", pack->name);
		return 0;
	}

	if (idata->tick != pack->tick)
		idata->tick = pack->tick;

	idata->tick++;
	idata->tick %= TICK_INTERVAL;
	mod_timer(&idata->timer, jiffies + HEART_RATE);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int sunxi_rpmsg_hearbeat_suspend(struct device *dev)
{
	struct rpmsg_client_heartbeat *idata = NULL;
	idata = dev_get_drvdata(dev);

	idata->sleep = 1;
	del_timer_sync(&idata->timer);

	return 0;
}

static int sunxi_rpmsg_hearbeat_resume(struct device *dev)
{
	struct rpmsg_client_heartbeat *idata = NULL;
	idata = dev_get_drvdata(dev);

	idata->timer.function = time_out_handler;
	idata->timer.data = (unsigned long)idata;
	idata->timer.expires = jiffies + 10 * HEART_RATE;

	add_timer(&idata->timer);

	idata->sleep = 0;

	return 0;
}

static struct dev_pm_ops sunxi_rpmsg_hearbeat_pm_ops = {
	.suspend = sunxi_rpmsg_hearbeat_suspend,
	.resume = sunxi_rpmsg_hearbeat_resume,
};
#endif

static int rpmsg_heartbeat_probe(struct rpmsg_device *rpdev)
{
	struct rpmsg_client_heartbeat *idata;

	idata = devm_kzalloc(&rpdev->dev, sizeof(*idata), GFP_KERNEL);
	if (!idata)
		return -ENOMEM;

	idata->rpdev = rpdev;
	idata->tick = 0;

	init_timer(&idata->timer);
	idata->timer.function = time_out_handler;
	idata->timer.data = (unsigned long)idata;
	idata->timer.expires = 3 * HEART_RATE;
	idata->sleep = 0;

	dev_set_drvdata(&rpdev->dev, idata);
	/* wo need to announce the new ept to remote */
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;

	add_timer(&idata->timer);

	return 0;
}

static void rpmsg_heartbeat_remove(struct rpmsg_device *rpdev)
{
	struct rpmsg_client_heartbeat *idata = dev_get_drvdata(&rpdev->dev);

	dev_info(&rpdev->dev, "%s is removed\n", dev_name(&rpdev->dev));
	del_timer_sync(&idata->timer);
}

static struct rpmsg_device_id rpmsg_driver_hearbeat_id_table[] = {
	{ .name	= "sunxi,rpmsg_heartbeat" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_hearbeat_id_table);

static struct rpmsg_driver rpmsg_sample_client = {
	.drv = {
		.name	= KBUILD_MODNAME,
#ifdef CONFIG_PM_SLEEP
		.pm = &sunxi_rpmsg_hearbeat_pm_ops,
#endif
	},
	.id_table	= rpmsg_driver_hearbeat_id_table,
	.probe		= rpmsg_heartbeat_probe,
	.callback	= rpmsg_heartbeat_cb,
	.remove		= rpmsg_heartbeat_remove,
};
module_rpmsg_driver(rpmsg_sample_client);

MODULE_DESCRIPTION("Remote processor messaging sample client driver");
MODULE_LICENSE("GPL v2");
