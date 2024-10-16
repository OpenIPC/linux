#define DEBUG
/*
 * Remote processor messaging - sample client driver
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Copyright (C) 2011 Google, Inc.
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
#include <linux/ktime.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include "linux/types.h"

#define DATA_LEN_PER		(512 - 16)
#define DATA_LEN	(2 * 1024 * 4 * DATA_LEN_PER)

static uint8_t buf[DATA_LEN_PER];

struct instance_data {
	struct delayed_work work;
	struct rpmsg_device *rd;
	uint32_t tx_count;
	uint32_t rx_count;
	uint32_t delay_time;
	spinlock_t busy_lock;
	uint32_t remote_is_busy;
	int is_unbound;
	long long rx_start;
	long long rx_end;
};

static void work_func(struct work_struct *work)
{
	int ret;

	struct instance_data *idata = container_of(to_delayed_work(work),
					struct instance_data, work);
	struct rpmsg_device *rd = idata->rd;

	idata->tx_count = 0;
	while (!idata->is_unbound) {

		if (idata->remote_is_busy)
			break;

		*((unsigned long *)buf) = idata->tx_count;
		ret = rpmsg_trysend(rd->ept, buf, sizeof(buf));
		if (!ret)
			idata->tx_count += 1;

		if (idata->tx_count >= (DATA_LEN / DATA_LEN_PER))
			break;

		udelay(idata->delay_time);
	}
}

static int rpmsg_sample_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct instance_data *idata;
	struct rpmsg_device *rd;
	long long speed;

	idata = dev_get_drvdata(&rpdev->dev);
	rd = idata->rd;

	if (len == 4) {
		/* full */
		if ((*(uint32_t *)data) == 0x6c6c7566) {
			if (idata->remote_is_busy == 0) {
				if (idata->delay_time < 1000)
					idata->delay_time *= 2;
				else
					idata->delay_time += 100;

				spin_lock(&idata->busy_lock);
				idata->remote_is_busy = 1;
				spin_unlock(&idata->busy_lock);
				dev_err(&rd->dev, "to fast, send again(%d)\n",
								idata->delay_time);
			}
			return 0;
		}

		if ((*(uint32_t *)data) == 0x74706d65) {
			if (idata->remote_is_busy) {
				spin_lock(&idata->busy_lock);
				idata->remote_is_busy = 0;
				spin_unlock(&idata->busy_lock);
				schedule_delayed_work(&idata->work, msecs_to_jiffies(500));
			}
			return 0;
		}
		return 0;
	}

	/* first recv data we record start time */
	if (idata->rx_count == 0)
		idata->rx_start = ktime_to_us(ktime_get_real());

	idata->rx_count += len;

	/* reacv enought data wo calculate this speed */
	if (idata->rx_count >= DATA_LEN) {
		idata->rx_end = ktime_to_us(ktime_get_real());

		speed = idata->rx_end - idata->rx_start;
		dev_info(&rd->dev, "Recv size=%d B,time=%lld us\n",
						idata->rx_count, speed);
		idata->rx_count = 0;

		/* start to send data */
		schedule_delayed_work(&idata->work, msecs_to_jiffies(500));
	}

	return 0;
}

static int rpmsg_sample_probe(struct rpmsg_device *rpdev)
{
	struct instance_data *idata;

	idata = devm_kzalloc(&rpdev->dev, sizeof(*idata), GFP_KERNEL);
	if (!idata)
		return -ENOMEM;

	idata->rd = rpdev;
	idata->rx_count = 0;
	idata->is_unbound = 0;
	idata->delay_time = 1;
	idata->remote_is_busy = 0;
	spin_lock_init(&idata->busy_lock);
	INIT_DELAYED_WORK(&idata->work, work_func);

	dev_set_drvdata(&rpdev->dev, idata);

	/* wo need to announce the new ept to remote */
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;

	return 0;
}

static void rpmsg_sample_remove(struct rpmsg_device *rpdev)
{
	struct instance_data *idata;
	idata = dev_get_drvdata(&rpdev->dev);

	dev_info(&rpdev->dev, "rpmsg send data len: %d\n", idata->tx_count * DATA_LEN_PER);
	dev_info(&rpdev->dev, "rpmsg sample client driver is removed\n");
	idata->is_unbound = 1;
	cancel_delayed_work_sync(&idata->work);
}

static struct rpmsg_device_id rpmsg_driver_sample_id_table[] = {
	{ .name	= "sunxi,rpmsg-speed-test" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_sample_id_table);

static struct rpmsg_driver rpmsg_client_test = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= rpmsg_driver_sample_id_table,
	.probe		= rpmsg_sample_probe,
	.callback	= rpmsg_sample_cb,
	.remove		= rpmsg_sample_remove,
};
module_rpmsg_driver(rpmsg_client_test);

MODULE_DESCRIPTION("Remote processor messaging sample client driver");
MODULE_LICENSE("GPL v2");
