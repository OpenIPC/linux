/*
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/err.h>
#include <linux/remoteproc.h>

static void rpmsg_echo_cb(struct rpmsg_channel *rpdev, void *data, int len,
			void *priv, u32 src)
{
	printk("[ %20s ] recv msg: [%s] from 0x%x and len %d\n",
	       __func__, (const char*)data, src, len);

	/* Echo the recved message back */
	rpmsg_send(rpdev, data, len);
}

static int rpmsg_echo_probe(struct rpmsg_channel *rpdev)
{
	int ret = 0;
	struct rpmsg_ns_msg nsm;

	nsm.addr = rpdev->dst;
	memcpy(nsm.name, rpdev->id.name, RPMSG_NAME_SIZE);
	nsm.flags = 0;

	rpmsg_send(rpdev, &nsm, sizeof(nsm));

	return ret;
}

static void rpmsg_echo_remove(struct rpmsg_channel *rpdev)
{
}

static struct rpmsg_device_id rpmsg_echo_id_table[] = {
	{ .name	= "echo_ca9_b", },
	{ .name	= "echo_arm11", },
	{ .name	= "echo_cortex", },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_echo_id_table);

static struct rpmsg_driver rpmsg_echo_driver = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= rpmsg_echo_id_table,
	.probe		= rpmsg_echo_probe,
	.callback	= rpmsg_echo_cb,
	.remove		= rpmsg_echo_remove,
};

static int __init rpmsg_echo_init(void)
{
	return register_rpmsg_driver(&rpmsg_echo_driver);
}

static void __exit rpmsg_echo_fini(void)
{
	unregister_rpmsg_driver(&rpmsg_echo_driver);
}

module_init(rpmsg_echo_init);
module_exit(rpmsg_echo_fini);

MODULE_DESCRIPTION("RPMSG Echo Server");
