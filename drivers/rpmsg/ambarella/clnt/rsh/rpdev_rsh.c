/**
 * History:
 *    2012/09/15 - [Tzu-Jung Lee] created file
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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/remoteproc.h>

#include <plat/rpdev.h>

extern int rsh_init_driver(void);
extern int rsh_init_device(int index, void *rpdev);

extern void rsh_cb(int index, void *data, int len);

int do_rsh_write(void *rpdev, const unsigned char *buf, int count)
{
	while (rpdev_trysend(rpdev, (char*)buf, count))
		;

	return count;
}

static void rpdev_rsh_cb(struct rpdev *rpdev, void *data, int len,
			 void *priv, u32 src)
{
	rsh_cb((int)priv, data, len);
}

static void rpdev_rsh_init_work(struct work_struct *work)
{
	struct rpdev *rpdev;
	int index = 0;

	rpdev = rpdev_alloc("rsh_ca9_b", 0, rpdev_rsh_cb, NULL);

	rsh_init_driver();
	rpdev->priv = (void*)index;

	rpdev_register(rpdev, "ca9_a_and_b");

	rsh_init_device(index, rpdev);
}

static struct work_struct work;

static int rpdev_rsh_init(void)
{
	INIT_WORK(&work, rpdev_rsh_init_work);
	schedule_work(&work);

        return 0;
}

static void rpdev_rsh_fini(void)
{
}

module_init(rpdev_rsh_init);
module_exit(rpdev_rsh_fini);

MODULE_DESCRIPTION("RPMSG Remote Shell");
