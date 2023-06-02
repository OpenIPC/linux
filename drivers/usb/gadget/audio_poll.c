#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/poll.h>

#include "f_uac.h"


#define init_MUTEX(LOCKNAME)        sema_init(LOCKNAME, 1)
#define DEVICE_NAME                 "uac_audio"
#define UAC_STREAM_OFF	            0
#define UAC_STREAM_ON	            1
#define UAC_IOC_SEND_AUDIO	        0x499
#define UAC_IOC_GET_STREAM_STATUS   0x599



static struct class        *cdev_class;
static struct cdev          dev_c;
static dev_t                dev;


static struct semaphore     sem;
static wait_queue_head_t    outq;

static int gStreamFlag;
static int gStreamStatus    = UAC_STREAM_OFF;
static int gStaFlag;


/*#define UAC_DEV_USE_LOCK*/
static int lock(void)
{
#ifdef UAC_DEV_USE_LOCK
	return down_interruptible(&sem);
#else
	return 0;
#endif
}

static void unlock(void)
{
#ifdef UAC_DEV_USE_LOCK
	up(&sem);
#endif
}


void wake_up_app(void)
{
	if (gStreamStatus == UAC_STREAM_ON) {
		lock();
		gStreamFlag = 1;
		unlock();
		wake_up_interruptible(&outq);
	}
}


void uac_stream(int on)
{
	lock();
	gStreamStatus = on;
	gStaFlag = 1;
	unlock();
	wake_up_interruptible(&outq);
}


static int audio_ops_open(struct inode *node, struct file *filp)
{
	audio_dev_open();
	return 0;
}



static long audio_ops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case UAC_IOC_SEND_AUDIO:
		return audio_send_data((void *)arg);
		break;

	case UAC_IOC_GET_STREAM_STATUS:
		return gStreamStatus;
		break;
	}
	return 0;
}


static unsigned int audio_ops_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	poll_wait(filp, &outq, wait);
	lock();
	if (gStreamFlag) {
		if (gStreamStatus == UAC_STREAM_ON) {
			/*Write Stream*/
			mask |= POLLOUT | POLLWRNORM;
		}
		gStreamFlag = 0;
	}

	if (gStaFlag) {
		/*Read Status*/
		mask |= POLLIN | POLLRDNORM;
		gStaFlag = 0;
	}
	unlock();
	return mask;
}



struct file_operations audio_device_fops = {
poll:
		audio_ops_poll,
open :
		audio_ops_open,
unlocked_ioctl :
		audio_ops_ioctl,
};



int __init audio_device_init(void)
{
	int ret, err;
	ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	if (ret)
		printk("audio device register failure");
	else {
		cdev_init(&dev_c, &audio_device_fops);
		err = cdev_add(&dev_c, dev, 1);
		if (err) {
			printk(KERN_NOTICE "error %d adding FC_dev\n", err);
			unregister_chrdev_region(dev, 1);
			return err;
		} else
			printk("device register success!\n");

		cdev_class = class_create(THIS_MODULE, DEVICE_NAME);
		if (IS_ERR(cdev_class)) {
			printk("ERR:cannot create a cdev_class\n");
			unregister_chrdev_region(dev, 1);
			return -1;
		}
		device_create(cdev_class, NULL, dev, 0, DEVICE_NAME);

		init_MUTEX(&sem);
		init_waitqueue_head(&outq);
	}
	return ret;
}


void __exit audio_device_exit(void)
{
	device_destroy(cdev_class, dev);
	class_destroy(cdev_class);
	unregister_chrdev_region(dev, 1);
	/*printk("device");*/
}


