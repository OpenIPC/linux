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
#include <linux/tty.h>
#include <linux/remoteproc.h>
#include <linux/rpmsg.h>

extern int rsh_init_driver(void);
extern int rsh_init_device(int index, void *rpdev);

extern void rsh_cb(int index, void *data, int len);

int do_rsh_write(void *rpdev, const unsigned char *buf, int count)
{
	while (rpmsg_trysend(rpdev, (char*)buf, count))
		;

	return count;
}

static void rpmsg_rsh_cb(struct rpmsg_channel *rpdev, void *data,
				int len, void *priv, u32 src)
{
	rsh_cb((int)priv, data, len);
}

static int rpmsg_rsh_probe(struct rpmsg_channel *rpdev)
{
	int ret = 0;
	struct rpmsg_ns_msg nsm;
	int index;

	nsm.addr = rpdev->dst;
	memcpy(nsm.name, rpdev->id.name, RPMSG_NAME_SIZE);
	nsm.flags = 0;

	if (!strcmp(rpdev->id.name, "rsh_ca9_b"))
		index = 0;
	else if (!strcmp(rpdev->id.name, "rsh_arm11"))
		index = 1;
	else if (!strcmp(rpdev->id.name, "rsh_ca9_b"))
		index = 2;
	else 
		return -1;

	rpdev->ept->priv = (void*)index;

	rpmsg_sendto(rpdev, &nsm, sizeof(nsm), rpdev->dst);

	rsh_init_device(index, rpdev);

	return ret;
}

static void rpmsg_rsh_remove(struct rpmsg_channel *rpdev)
{
}

static struct rpmsg_device_id rpmsg_rsh_id_table[] = {
	{ .name	= "rsh_ca9_b", }, /* H: CA9-A, C: CA9-B */
	{ .name	= "rsh_arm11", }, /* H: CA9-A, C: ARM11 */
	{ .name	= "rsh_ca9_a", }, /* H: CA9-B, C: ARM11 */
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_rsh_id_table);

static struct rpmsg_driver rpmsg_rsh_driver =
{
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= rpmsg_rsh_id_table,
	.probe		= rpmsg_rsh_probe,
	.callback	= rpmsg_rsh_cb,
	.remove		= rpmsg_rsh_remove,
};

static int __init rpmsg_rsh_init(void)
{
	rsh_init_driver();

	return register_rpmsg_driver(&rpmsg_rsh_driver);
}

static void __exit rpmsg_rsh_fini(void)
{
	unregister_rpmsg_driver(&rpmsg_rsh_driver);
}

module_init(rpmsg_rsh_init);
module_exit(rpmsg_rsh_fini);

MODULE_DESCRIPTION("RPMSG Remote Shell");
