/**
 * History:
 *    2012/07/30 - [Tzu-Jung Lee] created file
 *
 * Copyright 2008-2015 Ambarella Inc.  All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/rpmsg.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/remoteproc.h>

#include <plat/rpdev.h>

static void rpdev_echo_cb(struct rpdev *rpdev, void *data, int len,
				 void *priv, u32 src)
{
	printk("[ %20s ] recv message: [%s]\n", __func__, (char *)data);
}

static void rpdev_echo_init_work(struct work_struct *work)
{
	struct rpdev *rpdev;
	char *str = "Hello from CA9-B";
	char buf[64];
	int i;

	rpdev = rpdev_alloc("echo_ca9_b", 0, rpdev_echo_cb, NULL);

	rpdev_register(rpdev, "ca9_a_and_b");

	printk("[ %20s ] send message: [%s]\n", __func__, str);

	for (i = 0; i < 512; i++) {
		sprintf(buf, "%s  %d", str, i);
		printk("try sending: %s   OK\n", buf);
		while (rpdev_trysend(rpdev, buf, strlen(buf) + 1) != 0) {
			printk("retry sending: %s\n", buf);
			msleep(10);
		}
	}

	for (i = 0; i < 512; i++) {
		sprintf(buf, "%s %d (wait)", str, i);
		printk("sending: %s   OK\n", buf);
		rpdev_send(rpdev, buf, strlen(buf) + 1);
	}
}

static struct work_struct work;

static int rpdev_echo_init(void)
{
	INIT_WORK(&work, rpdev_echo_init_work);
	schedule_work(&work);
	return 0;
}

static void rpdev_echo_fini(void)
{
}

module_init(rpdev_echo_init);
module_exit(rpdev_echo_fini);
