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
#define DEVICE_NAME_SPK             "uac_speaker"
#define UAC_STREAM_OFF	            0
#define UAC_STREAM_ON	            1
#define UAC_IOC_SEND_AUDIO	        0x499
#define UAC_IOC_RECV_AUDIO	        0x549
#define UAC_IOC_GET_MIC_STREAM_STATUS   0x599
#define UAC_IOC_GET_MIC_SIMPLE_STATUS   0x619
#define UAC_IOC_GET_SPK_SIMPLE_STATUS   0x629
#define UAC_IOC_GET_SPK_STREAM_STATUS   0x649

#define UAC_IOC_GET_MIC_VOLUME_STATUS   0x650
#define UAC_IOC_GET_MIC_MUTE_STATUS     0x651
#define UAC_IOC_GET_SPK_VOLUME_STATUS   0x652
#define UAC_IOC_GET_SPK_MUTE_STATUS     0x653



static struct class        *cdev_class;
static struct cdev          dev_c;
static dev_t                dev;

static struct semaphore     sem;
static wait_queue_head_t    outq;

static int gStreamFlag;
static int gStreamStatus    = UAC_STREAM_OFF;
static int gStaFlag;

#ifdef CONFIG_ENABLE_SPEAKER_DESC
static struct class        *cdev_class_spk;
static struct cdev          dev_c_spk;
static dev_t                spk_dev;
static wait_queue_head_t    spkq;
static int gStreamFlag_spk;
static int gStreamStatus_spk    = UAC_STREAM_OFF;
static int gStaFlag_spk;
#endif

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

void uac_mic_FU(void)
{
	lock();
	gStaFlag = 1;
	unlock();
	wake_up_interruptible(&outq);
}

#ifdef CONFIG_ENABLE_SPEAKER_DESC
void wake_up_app_spk(void)
{
	if (gStreamStatus_spk == UAC_STREAM_ON) {
		lock();
		gStreamFlag_spk = 1;
		unlock();
		wake_up_interruptible(&spkq);
	}
}

void uac_stream_spk(int on)
{
	lock();
	gStreamStatus_spk = on;
	gStaFlag_spk = 1;
	unlock();
	wake_up_interruptible(&spkq);
}

void uac_spk_FU(void)
{
	lock();
	gStaFlag_spk = 1;
	unlock();
	wake_up_interruptible(&spkq);
}
#endif

static int audio_ops_open(struct inode *node, struct file *filp)
{
	return 0;
}

static long audio_ops_ioctl(struct file *filp,
							unsigned int cmd,
							unsigned long arg)
{
	switch (cmd) {
	case UAC_IOC_SEND_AUDIO:
		return audio_send_data((void *)arg);

#ifdef CONFIG_ENABLE_SPEAKER_DESC
	case UAC_IOC_RECV_AUDIO:
		return audio_recv_data((struct f_audio_buf *)arg);

	case UAC_IOC_GET_SPK_STREAM_STATUS:
		return gStreamStatus_spk;

	case UAC_IOC_GET_SPK_SIMPLE_STATUS:
		return audio_spk_simple_rate();

	case UAC_IOC_GET_SPK_VOLUME_STATUS:
		return audio_spk_volume();

	case UAC_IOC_GET_SPK_MUTE_STATUS:
		return audio_spk_mute();
#endif
	case UAC_IOC_GET_MIC_STREAM_STATUS:
		return gStreamStatus;

	case UAC_IOC_GET_MIC_SIMPLE_STATUS:
		return audio_mic_simple_rate();

	case UAC_IOC_GET_MIC_VOLUME_STATUS:
		return audio_mic_volume();

	case UAC_IOC_GET_MIC_MUTE_STATUS:
		return audio_mic_mute();

	default:
		pr_err("[UAC] %s audio ioctl cmd %x error!\n", __func__, cmd);
		return -1;
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

#ifdef CONFIG_ENABLE_SPEAKER_DESC
static int speaker_ops_open(struct inode *node, struct file *filp)
{
	return 0;
}

static unsigned int speaker_ops_poll(struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;
	poll_wait(filp, &spkq, wait);
	lock();
	if (gStreamFlag_spk) {
		if (gStreamStatus_spk == UAC_STREAM_ON) {
			/*Write Stream*/
			mask |= POLLOUT | POLLWRNORM; 
		}
		gStreamFlag_spk = 0;
	}

	if (gStaFlag_spk) {
		/*Read Status*/
		mask |= POLLIN | POLLRDNORM;
		gStaFlag_spk = 0;
	}
	unlock();
	return mask;
}

struct file_operations speaker_device_fops = {
poll:
		speaker_ops_poll,
open :
		speaker_ops_open,
unlocked_ioctl :
		audio_ops_ioctl,
};
#endif

int audio_device_init(void)
{
	int ret, err;
	/* mic */
	ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	if (ret)
		pr_err("audio device register failure");
	else {
		cdev_init(&dev_c, &audio_device_fops);
		err = cdev_add(&dev_c, dev, 1);
		if (err) {
			pr_err("error %d adding FC_dev\n", err);
			unregister_chrdev_region(dev, 1);
			return err;
		}


		cdev_class = class_create(THIS_MODULE, DEVICE_NAME);
		if (IS_ERR(cdev_class)) {
			pr_err("ERR:cannot create a cdev_class\n");
			unregister_chrdev_region(dev, 1);
			return -1;
		}
		device_create(cdev_class, NULL, dev, 0, DEVICE_NAME);

		init_MUTEX(&sem);
		init_waitqueue_head(&outq);
	}

#ifdef CONFIG_ENABLE_SPEAKER_DESC
	/* speaker */
	ret = alloc_chrdev_region(&spk_dev, 0, 1, DEVICE_NAME_SPK);
	if (ret)
		printk("speaker device register failure");
	else {
		cdev_init(&dev_c_spk, &speaker_device_fops);
		err = cdev_add(&dev_c_spk, spk_dev, 1);
		if (err) {
			printk(KERN_NOTICE "error %d adding FC_dev\n", err);
			unregister_chrdev_region(spk_dev, 1);
			return err;
		} else
			printk("speaker device register success!\n");

		cdev_class_spk = class_create(THIS_MODULE, DEVICE_NAME_SPK);
		if (IS_ERR(cdev_class_spk)) {
			printk("ERR:cannot create a cdev_class_spk\n");
			unregister_chrdev_region(spk_dev, 1);
			return -1;
		}
		device_create(cdev_class_spk,
					NULL,
					spk_dev,
					0,
					DEVICE_NAME_SPK);

		init_waitqueue_head(&spkq);
	}
#endif
	return ret;
}


void audio_device_exit(void)
{
	device_destroy(cdev_class, dev);
	class_destroy(cdev_class);
	unregister_chrdev_region(dev, 1);
#ifdef CONFIG_ENABLE_SPEAKER_DESC
	device_destroy(cdev_class_spk, spk_dev);
	class_destroy(cdev_class_spk);
	unregister_chrdev_region(spk_dev, 1);
#endif
	/*printk("device");*/
}


