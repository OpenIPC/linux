/*
 * rpmsg write to nfs server driver
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/err.h>
#include <linux/remoteproc.h>

#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/kfifo.h>

#include <linux/virtio_ring.h>
#include <linux/sched.h>
#include <linux/delay.h>

#define DEVICE_NAME "wnfs"


typedef int redev_nfs_FILE;
typedef struct nfs_fwrite_info_s
{
	int* addr;
	int size;
	int count;
	redev_nfs_FILE *file;
}
nfs_fwrite_info_t;

struct wnfs_struct {
	struct kfifo		fifo;
	spinlock_t		lock;
	struct rpmsg_channel	*rpdev;
	struct miscdevice fdev;
        nfs_fwrite_info_t unfinished;
};

static struct wnfs_struct g_wnfs;

static int device_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &g_wnfs;

	return 0;
}

static ssize_t device_write(struct file *filp, const char *buf,
			    size_t count, loff_t * off)
{
	struct wnfs_struct *wnfs = filp->private_data;
	struct rpmsg_channel *rpdev = wnfs->rpdev;

	rpmsg_sendto(rpdev, (void *)buf, count, rpdev->dst);

	return count;
}

int wnfs_test_counter = 0;
static ssize_t device_read(struct file *filp, char *buf,
			   size_t len, loff_t * offset)
{
	struct wnfs_struct *wnfs = filp->private_data;
	int bytes_read = 0;
	// char c;
	char *vaddr;
	nfs_fwrite_info_t nfs_fwrite_info;
	int copy_size;
	int unfinished_size;
	int ret;
	int len_remain = len;

	//printk(" ====================== device_read %d ======================\n", wnfs_test_counter);

//wnfs_test_counter ++;
if(wnfs_test_counter > 2000)
{
	printk("device_read: force 0\n");
	wnfs_test_counter = 0;
	return 0;
}

// 	while (kfifo_out_locked(&wnfs->fifo, &c, sizeof(c), &wnfs->lock)) {
//
// 		*buf++ = c;
// 		bytes_read++;
// 	}

//         blocking = filp->??

	while(len_remain > 0) {
		switch (wnfs->unfinished.count) {
			case 0:
	                //reload
// 	                        while(1) {
				ret = kfifo_out_locked(&wnfs->fifo, &nfs_fwrite_info, sizeof(nfs_fwrite_info_t), &wnfs->lock);
				//printk("reload: info %d/%d\n", ret, sizeof(nfs_fwrite_info_t));
// 					//if ret==not ready, sleep
// 					if(ret < sizeof(nfs_fwrite_info_t))
// 						if(blocking)
// 							sleep();
// 						else return -EAGAIN;
// 					else
// 						break;
// 				}
                                if(ret < sizeof(nfs_fwrite_info_t)) {
					msleep(5);
                                        //printk("reload: not ready %d\n", ret);
					break;
				} else {
                                        if(nfs_fwrite_info.size == 0xdeadbeef) {
						printk("nfs_fwrite_info.size == 0xdeadbeef");
						wnfs_test_counter = 2000000;
						rpmsg_sendto(wnfs->rpdev, &bytes_read, sizeof(int), wnfs->rpdev->dst); //ack to release fread of iTRON side
						return 0;
					}
					memcpy(&(wnfs->unfinished), &nfs_fwrite_info, sizeof(nfs_fwrite_info_t));
					//printk("reload: x%x %d %d\n", wnfs->unfinished.addr, wnfs->unfinished.size, wnfs->unfinished.count);
				}

			default:
			//fill-up buf
				unfinished_size = wnfs->unfinished.size * wnfs->unfinished.count;
				copy_size = (len_remain > unfinished_size) ? unfinished_size : len_remain;
				if(copy_size>0)
				{
					vaddr = (void*)ambarella_phys_to_virt((unsigned long)wnfs->unfinished.addr);
					memcpy(buf, vaddr, copy_size);
					bytes_read += copy_size;
					buf += copy_size;
					unfinished_size -= copy_size;

					if(unfinished_size > 0) {
						wnfs->unfinished.count = unfinished_size / wnfs->unfinished.size;
						wnfs->unfinished.addr = (int*)((unsigned int)wnfs->unfinished.addr + copy_size);
					} else {
						memset(&(wnfs->unfinished), 0, sizeof(nfs_fwrite_info_t));
						rpmsg_sendto(wnfs->rpdev, &bytes_read, sizeof(int), wnfs->rpdev->dst); //ack to release fread of iTRON side
						//printk("ack\n");
					}
					len_remain -= copy_size;
				}
                                //printk("fillup: %d/%d \n", bytes_read, len);
		}
	}

// 	while (kfifo_out_locked(&wnfs->fifo, &nfs_fwrite_info, sizeof(nfs_fwrite_info_t), &wnfs->lock)) {
// // 		vaddr = ambarella_phys_to_virt(nfs_fwrite_info.addr);
// //                 bytes_read += nfs_fwrite_info.size*nfs_fwrite_info.count;
// //                 memcpy(buf, vaddr, nfs_fwrite_info.size*nfs_fwrite_info.count);
// //                 buf += bytes_read;
// 		//printk("device_read: x%x x%x\n", vaddr, bytes_read);
// 		bytes_read = len;
// 	}

        //rpmsg_sendto(g_wnfs.rpdev, &bytes_read, sizeof(int), g_wnfs.rpdev->dst);
        //printk("device_read: final bytes_read %d/%d \n", bytes_read, len);

	return bytes_read;
}

static int device_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

static int wnfs_init(struct wnfs_struct *wnfs)
{
	int ret;

	spin_lock_init(&wnfs->lock);

        ret = kfifo_alloc(&wnfs->fifo, 4096*16, GFP_KERNEL);
        if (ret)
                return ret;

	printk("RPMSG Write to NFS Server [ARM11] now is ready at /dev/%s\n", DEVICE_NAME);

	wnfs->fdev.name = DEVICE_NAME;
	wnfs->fdev.fops = &fops;
        memset(&(wnfs->unfinished), 0, sizeof(nfs_fwrite_info_t));
	misc_register(&wnfs->fdev);

	return 0;
}

static void rpmsg_wnfs_cb(struct rpmsg_channel *rpdev, void *data,
				int len, void *priv, u32 src)
{
	struct wnfs_struct *wnfs = priv;
	int n;
        //printk("rpmsg_wnfs_cb +\n");

	n = kfifo_in_locked(&wnfs->fifo, (char *)data, len, &wnfs->lock);
	//printk("rpmsg_wnfs_cb -:%d\n", n);
//rpmsg_sendto(rpdev, &len, sizeof(int), rpdev->dst);
}

static int rpmsg_wnfs_probe(struct rpmsg_channel *rpdev)
{
	int ret = 0;
	struct rpmsg_ns_msg nsm;
	struct wnfs_struct *wnfs = &g_wnfs;

	nsm.addr = rpdev->dst;
	memcpy(nsm.name, rpdev->id.name, RPMSG_NAME_SIZE);
	nsm.flags = 0;

	wnfs_init(wnfs);
	wnfs->rpdev = rpdev;

	rpdev->ept->priv = wnfs;

	rpmsg_sendto(rpdev, &nsm, sizeof(nsm), rpdev->dst);

	return ret;
}

static void rpmsg_wnfs_remove(struct rpmsg_channel *rpdev)
{
	struct wnfs_struct *wnfs = &g_wnfs;
	misc_deregister(&wnfs->fdev);
}

static struct rpmsg_device_id rpmsg_wnfs_id_table[] = {
	{ .name	= "wnfs_arm11", },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_wnfs_id_table);

static struct rpmsg_driver rpmsg_wnfs_driver = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= rpmsg_wnfs_id_table,
	.probe		= rpmsg_wnfs_probe,
	.callback	= rpmsg_wnfs_cb,
	.remove		= rpmsg_wnfs_remove,
};

static int __init rpmsg_wnfs_init(void)
{
	return register_rpmsg_driver(&rpmsg_wnfs_driver);
}

static void __exit rpmsg_wnfs_fini(void)
{
	unregister_rpmsg_driver(&rpmsg_wnfs_driver);
}

module_init(rpmsg_wnfs_init);
module_exit(rpmsg_wnfs_fini);

MODULE_DESCRIPTION("RPMSG Write to NFS Server");
