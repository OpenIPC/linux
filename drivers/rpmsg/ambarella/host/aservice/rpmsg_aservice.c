/*
 * rpmsg audio service driver
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
#include <linux/mm.h>
#include <linux/wait.h>

#define DEVICE_NAME "aservice"

enum aio_cmd {
  AIO_CMD_SETUP = 0,
  AIO_CMD_START = 1,
  AIO_CMD_STOP = 2,
  AIO_CMD_CLOSE = 3,
  AIO_CMD_NONE = 0xFF
};

enum aio_type {
	ENCODER = 0,
	DECODER,
	XCODER,
	BYPASS
};

enum aio_dir {
	ARM_TO_LINUX = 0,
	LINUX_TO_ARM,
	BIDRECTION,
};

enum aio_encoder {
	AAC_ENC = 0,
	AC3_ENC,
	MP2_ENC
};

enum aio_decoder {
	AAC_DEC = 0,
	AC3_DEC,
	MP2_DEC
}; 

//test aservice structure

typedef struct aio_buf_s {
	u32 command;
  	u8 *arm_buf_base;    /**< ARM side buffer write pointer >*/
  	u32 arm_buf_limit;    /**< ARM side buffer write pointer >*/
  	u32 arm_buf_wptr;    /**< ARM side buffer write pointer >*/
	u32 arm_buf_rptr;   /**< ARM side buffer read pointer >*/
	u32 arm_buf_vaddr;	/** <Linux side buffer virtual address (mmap), for iTron side debug or information > */

	u8 *linux_buf_base;    /**< ARM side buffer write pointer >*/
	u32 linux_buf_limit;    /**< ARM side buffer write pointer >*/
	u32 linux_buf_wptr;	/**< Linux side buffer write pointer >*/
	u32 linux_buf_rptr;	/**< Linux side buffer read pointer >*/
	u32 linux_buf_vaddr;	/** <Linux side buffer virtual address (mmap), for iTron side debug or information > */
	u32 decoder;
	u32 encoder;
	u32 aio_type;
	u32 aio_direction;
	u32 self;
} aio_buf_t;

struct aservice_ret_msg {
	s32 rval;
};

struct aservice_struct {
        spinlock_t              lock;
        struct rpmsg_channel    *rpdev;
        struct miscdevice fdev;
	aio_buf_t *aio_buf;
	u32 finish_flag;
};

//init waiting queue
static DECLARE_WAIT_QUEUE_HEAD(msgq);
static struct aservice_struct g_aservice;

static int device_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &g_aservice;

	return 0;
}

static ssize_t device_write(struct file *filp, const char *buf,
			    size_t count, loff_t * off)
{
	struct aservice_struct *aservice = filp->private_data;

	printk("Enter devide write\n");
	//might deal with different write msgs
	aservice->finish_flag = 1;
	wake_up_interruptible(&msgq);
	return count;
}

static ssize_t device_read(struct file *filp, char *buf,
			   size_t len, loff_t * offset)
{
	struct aservice_struct *aservice = filp->private_data;
	aio_buf_t *aio_buf = aservice->aio_buf;
	int rval;

	printk("Enter device read\n");

	if (aio_buf != NULL) {
		switch(aio_buf->command) {
			case AIO_CMD_SETUP:
			case AIO_CMD_START:
                        case AIO_CMD_STOP:
                        case AIO_CMD_CLOSE: {
                                printk("Command! %d\n", aio_buf->command);
				rval = copy_to_user(buf, aio_buf, sizeof(aio_buf_t));
				if(rval)
				{
					printk("copy to user failed %d %d\n", rval, sizeof(aio_buf_t));
					return -1;
				}
				aio_buf->command = AIO_CMD_NONE;
				break;
			}
			case AIO_CMD_NONE: //no command
				return -1;
				break;
		}
		return 0;
	}
	else
		return -1;
}

static int device_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int device_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct aservice_struct *aservice = filp->private_data;
	aio_buf_t *aio_buf = aservice->aio_buf;
	int rval = 0;
	unsigned long size;

	size = vma->vm_end - vma->vm_start; //size from userspace

	printk("Doing mmap, size = %lu\n", size);

	vma->vm_pgoff = (aio_buf->self) >> PAGE_SHIFT;

	rval = remap_pfn_range(vma,
			vma->vm_start,
			vma->vm_pgoff,
			size,
			vma->vm_page_prot);
	
	if (rval < 0)
		printk("We got some error\n");

	//mmap this size of memory based on memory address from uItron side
	//maybe more memory management if there is only one kernel device
	return rval;
}

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.mmap = device_mmap,
	.release = device_release
};

static int aservice_init(struct aservice_struct *aservice)
{
	spin_lock_init(&aservice->lock);

	printk("RPMSG aservice [ARM11] now is ready at /dev/%s\n", DEVICE_NAME);

	aservice->fdev.name = DEVICE_NAME;
	aservice->fdev.fops = &fops;
	aservice->aio_buf = NULL;
	aservice->finish_flag = 0;
	misc_register(&aservice->fdev);

	return 0;
}

static void rpmsg_aservice_cb(struct rpmsg_channel *rpdev, void *data,
				int len, void *priv, u32 src)
{
	struct aservice_struct *aservice = priv; 
	struct aservice_ret_msg msg;
	aio_buf_t *aio_buf_test;

	aio_buf_t *aio_buf = data;
	aservice->aio_buf = data;

	printk("recieve msg : rpmsg_aservice_cb +\n");
	
	printk("length %d\n", len);
	printk("My address is %08X\n", (u32)aio_buf);
        printk("command %d\n",aio_buf->command);
        printk("buf base %08X\n",(u32)aio_buf->arm_buf_base);    /**< ARM side buffer write pointer >*/
        printk("buf limit %08X\n",aio_buf->arm_buf_limit);    /**< ARM side buffer write pointer >*/
        printk("wptr %08X\n",aio_buf->arm_buf_wptr);    /**< ARM side buffer write pointer >*/
        printk("rptr %08X\n",aio_buf->arm_buf_rptr);   /**< ARM side buffer read pointer >*/
        printk("buf vaddr %08X\n",aio_buf->arm_buf_vaddr);      /** <Linux side buffer virtual address (mmap), for iTron side debug or information > */

        printk("buf base %08X\n",(u32)aio_buf->linux_buf_base);    /**< ARM side buffer write pointer >*/
        printk("buf limit %08X\n",aio_buf->linux_buf_limit);    /**< ARM side buffer write pointer >*/
        printk("buf wptr %08X\n",aio_buf->linux_buf_wptr);     /**< Linux side buffer write pointer >*/
        printk("rptr %08X\n",aio_buf->linux_buf_rptr);     /**< Linux side buffer read pointer >*/
        printk("buf vaddr %08X\n",aio_buf->linux_buf_vaddr);    /** <Linux side buffer virtual address (mmap), for iTron side debug or information > */

	printk("encode %d\n",aio_buf->encoder);
	printk("decode %d\n",aio_buf->decoder);
	printk("type %d\n",aio_buf->aio_type);
	printk("dir %d\n",aio_buf->aio_direction);	
	printk("self %08X\n", aio_buf->self);
	
	aio_buf_test = (aio_buf_t *)phys_to_virt(aio_buf->self);
	printk("aio buf test to %08X\n", (u32)aio_buf_test);
        printk("command %d\n",aio_buf_test->command);
        printk("buf base %08X\n",(u32)aio_buf_test->arm_buf_base);    /**< ARM side buffer write pointer >*/
        printk("buf limit %08X\n",aio_buf_test->arm_buf_limit);    /**< ARM side buffer write pointer >*/
        printk("wptr %08X\n",aio_buf_test->arm_buf_wptr);    /**< ARM side buffer write pointer >*/
        printk("rptr %08X\n",aio_buf_test->arm_buf_rptr);   /**< ARM side buffer read pointer >*/
        printk("buf vaddr %08X\n",aio_buf_test->arm_buf_vaddr);      /** <Linux side buffer virtual address (mmap), for iTron side debug or information > */

	printk("Let's write something\n");
	aio_buf_test->command = 2;
	aio_buf_test->arm_buf_base = 0xDEADBEEF;
	aio_buf_test->arm_buf_limit = 0x2000;
	aio_buf_test->arm_buf_wptr = 0x12345678;
	aio_buf_test->arm_buf_rptr = 0x87654321;
	aio_buf_test->arm_buf_vaddr = 0x11111111;

        printk("command %d\n",aio_buf_test->command);
        printk("buf base %08X\n",(u32)aio_buf_test->arm_buf_base);    /**< ARM side buffer write pointer >*/
        printk("buf limit %08X\n",aio_buf_test->arm_buf_limit);    /**< ARM side buffer write pointer >*/
        printk("wptr %08X\n",aio_buf_test->arm_buf_wptr);    /**< ARM side buffer write pointer >*/
        printk("rptr %08X\n",aio_buf_test->arm_buf_rptr);   /**< ARM side buffer read pointer >*/
        printk("buf vaddr %08X\n",aio_buf_test->arm_buf_vaddr);      /** <Linux side buffer virtual address (mmap), for iTron side debug or information > */

	switch(aio_buf->command)
	{
		case AIO_CMD_SETUP:
			printk("setup command\n");
			break;
		case AIO_CMD_START:
			printk("start command\n");
			break;
		case AIO_CMD_STOP:
                        printk("start command\n");
                        break;
		case AIO_CMD_CLOSE:
			printk("close command\n");
		default:
			printk("Bad command\n");
			break;  
	}

	msg.rval = -1;
	printk("msg val %d\n", msg.rval);
	wait_event_interruptible(msgq, aservice->finish_flag != 0);
	if (aservice->finish_flag)
		aservice->finish_flag = 0;
	printk("waiting queue done, send back to uItron\n");
	//return a msg done, will using wait queue for application handshaking
	rpmsg_sendto(rpdev, &msg, sizeof(msg), rpdev->dst);
	
}

static int rpmsg_aservice_probe(struct rpmsg_channel *rpdev)
{
	int ret = 0;
	struct rpmsg_ns_msg nsm;
	struct aservice_struct *aservice = &g_aservice;

	nsm.addr = rpdev->dst;
	memcpy(nsm.name, rpdev->id.name, RPMSG_NAME_SIZE);
	nsm.flags = 0;

	aservice_init(aservice);
	aservice->rpdev = rpdev;

	rpdev->ept->priv = aservice;

	rpmsg_sendto(rpdev, &nsm, sizeof(nsm), rpdev->dst);

	return ret;
}

static void rpmsg_aservice_remove(struct rpmsg_channel *rpdev)
{
	struct aservice_struct *aservice = &g_aservice;
	misc_deregister(&aservice->fdev);
}

static struct rpmsg_device_id rpmsg_aservice_id_table[] = {
	{ .name	= "aservice_arm11", },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_aservice_id_table);

static struct rpmsg_driver rpmsg_aservice_driver = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= rpmsg_aservice_id_table,
	.probe		= rpmsg_aservice_probe,
	.callback	= rpmsg_aservice_cb,
	.remove		= rpmsg_aservice_remove,
};

static int __init rpmsg_aservice_init(void)
{
	return register_rpmsg_driver(&rpmsg_aservice_driver);
}

static void __exit rpmsg_aservice_fini(void)
{
	unregister_rpmsg_driver(&rpmsg_aservice_driver);
}

module_init(rpmsg_aservice_init);
module_exit(rpmsg_aservice_fini);

MODULE_DESCRIPTION("RPMSG audio service");
