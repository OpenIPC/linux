#include <plat/rpdev.h>

#include <linux/module.h>
#include <linux/platform_device.h>

extern struct platform_device ambveth_device;
extern void ambveth_enqueue(void *priv, void *data, int len);

struct rpdev *g_rpdev;

int ambveth_do_send(void *data, int len)
{
	return rpdev_trysend(g_rpdev, data, len);
}

static void rpdev_veth_client_cb(struct rpdev *rpdev, void *data, int len,
				 void *priv, u32 src)
{
	ambveth_enqueue(priv, data, len);
}

static void rpdev_veth_client_init_work(struct work_struct *work)
{
	struct rpdev *rpdev;

	platform_device_register(&ambveth_device);

	rpdev = rpdev_alloc("veth_ca9_b", 0, rpdev_veth_client_cb, &ambveth_device);

	rpdev_register(rpdev, "ca9_a_and_b");
	g_rpdev = rpdev;
}

static struct work_struct work;

static int rpdev_veth_client_init(void)
{
	INIT_WORK(&work, rpdev_veth_client_init_work);
	schedule_work(&work);
	return 0;
}

static void rpdev_veth_client_fini(void)
{
}

module_init(rpdev_veth_client_init);
module_exit(rpdev_veth_client_fini);

MODULE_DESCRIPTION("RPMSG VETH");
