#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/err.h>
#include <linux/remoteproc.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include <linux/kfifo.h>
#include <linux/delay.h>

#include <plat/ambcache.h>

typedef struct xnfs_info_s {

	char		*addr;
	int		size;
	int		count;
	void		*priv;
} xnfs_info_t;

struct xnfs_struct {

	int			id;
	struct kfifo		fifo;
	spinlock_t		lock;
	wait_queue_head_t	queue;
	struct rpmsg_channel	*rpdev;
};

static struct xnfs_struct g_wnfs;
static struct xnfs_struct g_rnfs;

ssize_t xnfs_read(char *buf, size_t len)
{
	struct xnfs_struct *xnfs = &g_wnfs;
	xnfs_info_t info;
	int n = 0;
	char *va;
	struct timespec ts, ts1, ts2;

	getnstimeofday(&ts1);
	wait_event_interruptible(xnfs->queue,
				 kfifo_out_locked(&xnfs->fifo, &info,
						  sizeof(info), &xnfs->lock));

	getnstimeofday(&ts2);
	if (!info.size) {
		rpmsg_send(xnfs->rpdev, &n, sizeof(n));
		return 0;
	}

	va = (void *)ambarella_phys_to_virt((unsigned long)info.addr);
	n = info.size * info.count;
	ts = timespec_sub(ts2, ts1);

	printk(KERN_DEBUG "rpmsg: Linux got 0x%08x Bytes for %lu.%09lu Seconds\n",
	       info.count, ts.tv_sec, ts.tv_nsec);

	ambcache_flush_range((void *)va, n);
	memcpy(buf, va, n);

	/* ack to release fread of iTRON side */
	rpmsg_send(xnfs->rpdev, &n, sizeof(n));

	return n;
}

ssize_t xnfs_write(char *buf, size_t len)
{
	struct xnfs_struct *xnfs = &g_rnfs;
	char *va;
	int size;
	xnfs_info_t info;

	wait_event_interruptible(xnfs->queue,
				 kfifo_out_locked(&xnfs->fifo, &info,
						  sizeof(info), &xnfs->lock));

	va = (void *)ambarella_phys_to_virt((unsigned long)info.addr);
	size = info.size * info.count;

	BUG_ON(size < len);

	if (len > 0) {

		memcpy(va, buf, len);
		ambcache_clean_range((void *)va, len);
	}
	if (len == 0)
		printk(KERN_DEBUG "rpmsg: uItron got 0 bytes\n");

	rpmsg_send(xnfs->rpdev, &len, sizeof(len));

	return len;
}

static void rpmsg_xnfs_cb(struct rpmsg_channel *rpdev, void *data,
			  int len, void *priv, u32 src)
{
	struct xnfs_struct *xnfs = priv;

	BUG_ON(len != sizeof(xnfs_info_t));
	kfifo_in_locked(&xnfs->fifo, (char *)data, len, &xnfs->lock);
	wake_up_interruptible(&xnfs->queue);
}

static int xnfs_init(struct xnfs_struct *xnfs)
{
	int ret;

	spin_lock_init(&xnfs->lock);
	init_waitqueue_head(&xnfs->queue);

	ret = kfifo_alloc(&xnfs->fifo, 4096 * 16, GFP_KERNEL);
	if (ret)
		return ret;

	return 0;
}

static struct rpmsg_device_id rpmsg_xnfs_id_table[] = {
	{ .name = "rnfs_arm11", },
	{ .name	= "wnfs_arm11", },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_xnfs_id_table);

static int rpmsg_xnfs_probe(struct rpmsg_channel *rpdev)
{
	int ret = 0;
	struct rpmsg_ns_msg nsm;
	struct xnfs_struct *xnfs = NULL;

	nsm.addr = rpdev->dst;
	memcpy(nsm.name, rpdev->id.name, RPMSG_NAME_SIZE);
	nsm.flags = 0;

	if (!strcmp(rpdev->id.name, rpmsg_xnfs_id_table[0].name)) {
		xnfs = &g_rnfs;
		xnfs->id = 0;
		printk("RPMSG Ready from NFS Server [ARM11] is ready\n");

	} else if (!strcmp(rpdev->id.name, rpmsg_xnfs_id_table[1].name)) {
		xnfs = &g_wnfs;
		xnfs->id = 1;
		printk("RPMSG Write to NFS Server [ARM11] is ready\n");
	}

	xnfs_init(xnfs);
	xnfs->rpdev = rpdev;

	rpdev->ept->priv = xnfs;

	rpmsg_send(rpdev, &nsm, sizeof(nsm));

	return ret;
}

static void rpmsg_xnfs_remove(struct rpmsg_channel *rpdev)
{
}

struct rpmsg_driver rpmsg_xnfs_driver = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= rpmsg_xnfs_id_table,
	.probe		= rpmsg_xnfs_probe,
	.callback	= rpmsg_xnfs_cb,
	.remove		= rpmsg_xnfs_remove,
};
