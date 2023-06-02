#include <linux/rpmsg.h>

#include <linux/module.h>
#include <linux/platform_device.h>

extern struct platform_device ambveth_device;
extern void ambveth_enqueue(void *priv, void *data, int len);

static struct rpmsg_channel *g_rpdev;

int ambveth_do_send(void *data, int len)
{
	return rpmsg_trysend(g_rpdev, data, len);
}

static void rpmsg_veth_server_cb(struct rpmsg_channel *rpdev, void *data, int len,
			void *priv, u32 src)
{
	ambveth_enqueue(priv, data, len);
}

static int rpmsg_veth_server_probe(struct rpmsg_channel *rpdev)
{
	int ret = 0;
	struct rpmsg_ns_msg nsm;

	platform_device_register(&ambveth_device);
	rpdev->ept->priv = &ambveth_device;

	nsm.addr = rpdev->dst;
	memcpy(nsm.name, rpdev->id.name, RPMSG_NAME_SIZE);
	nsm.flags = 0;

	g_rpdev = rpdev;
	rpmsg_send(rpdev, &nsm, sizeof(nsm));

	return ret;
}

static void rpmsg_veth_server_remove(struct rpmsg_channel *rpdev)
{
}

static struct rpmsg_device_id rpmsg_veth_server_id_table[] = {
	{ .name	= "veth_ca9_b", },
	{ .name	= "veth_arm11", },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_veth_server_id_table);

struct rpmsg_driver rpmsg_veth_server_driver = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= rpmsg_veth_server_id_table,
	.probe		= rpmsg_veth_server_probe,
	.callback	= rpmsg_veth_server_cb,
	.remove		= rpmsg_veth_server_remove,
};

static int __init rpmsg_veth_server_init(void)
{
	return register_rpmsg_driver(&rpmsg_veth_server_driver);
}

static void __exit rpmsg_veth_server_fini(void)
{
	unregister_rpmsg_driver(&rpmsg_veth_server_driver);
}

module_init(rpmsg_veth_server_init);
module_exit(rpmsg_veth_server_fini);

MODULE_DESCRIPTION("RPMSG VETH");
