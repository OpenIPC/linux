/* *
 * Copyright (C) 2008-2009 Texas Instruments Inc
 *
 * This program is free software you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not,write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
//#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/platform_device.h>

#include <media/davinci/imp_hw_if.h>
#include <media/davinci/imp_resizer.h>

#include <mach/cputype.h>

#define	DRIVER_NAME	"DaVinciResizer"

/*device structure shared across all instances*/
struct rsz_device rsz_dev;

/* For registeration of	charatcer device*/
static struct cdev c_dev;
/* device structure	to make	entry in device*/
static dev_t dev;
/* for holding device entry*/
static struct device *rsz_device;

/* instance of the imp interface */
static struct imp_hw_interface *imp_hw_if;
/*
=====================rsz_open===========================
This function creates a channels.
*/
static int rsz_open(struct inode *inode, struct file *filp)
{
	struct imp_logical_channel *rsz_conf_chan;
	int i, mode, ret;

	if (filp->f_flags & O_NONBLOCK) {
		dev_err
		    (rsz_device,
		     "rsz_open: device cannot be opened in non-blocked mode\n");
		return -EBUSY;
	}

	mode = imp_hw_if->get_resize_oper_mode();

	ret = mutex_lock_interruptible(&rsz_dev.lock);
	if (ret)
		return ret;
	if ((mode == IMP_MODE_CONTINUOUS) ||
	    ((mode == IMP_MODE_SINGLE_SHOT) && (!imp_hw_if->serialize()))) {
		if (rsz_dev.users != 0) {
			dev_err(rsz_device,
				"\n mode doesn't allow multiple instances\n");
			mutex_unlock(&rsz_dev.lock);
			return -EBUSY;
		}
	}

	/* if usage counter is greater than maximum supported channels
	   return error */
	if (rsz_dev.users >= MAX_CHANNELS) {
		dev_err(rsz_device,
			"\n modules usage count is greater than supported ");
		mutex_unlock(&rsz_dev.lock);
		return -EBUSY;
	}

	rsz_dev.users++;
	mutex_unlock(&rsz_dev.lock);
	/* allocate     memory for a new configuration */
	rsz_conf_chan = kmalloc(sizeof(struct imp_logical_channel), GFP_KERNEL);

	if (rsz_conf_chan == NULL) {
		dev_err(rsz_device,
			"\n cannot allocate memory ro channel config");
		return -ENOMEM;
	}

	rsz_conf_chan->config_state = STATE_NOT_CONFIGURED;
	rsz_conf_chan->mode = IMP_MODE_INVALID;
	rsz_conf_chan->primary_user = 0;
	rsz_conf_chan->chained = 0;
	rsz_conf_chan->config = NULL;
	rsz_conf_chan->user_config = NULL;
	rsz_conf_chan->user_config_size = 0;

	/* Set priority to lowest for that configuration channel */
	rsz_conf_chan->priority = MIN_PRIORITY;

	/* Set the channel type to resize */
	rsz_conf_chan->type = IMP_RESIZER;

	for (i = 0; i < MAX_BUFFERS; i++) {
		rsz_conf_chan->in_bufs[i] = NULL;
		rsz_conf_chan->out_buf1s[i] = NULL;
		rsz_conf_chan->out_buf2s[i] = NULL;
	}
	rsz_conf_chan->in_numbufs = 0;
	rsz_conf_chan->out_numbuf1s = 0;
	rsz_conf_chan->out_numbuf2s = 0;

	dev_dbg(rsz_device, "Initializing	of channel done	\n");

	/* Initializing of application mutex */
	init_completion(&(rsz_conf_chan->channel_sem));
	rsz_conf_chan->channel_sem.done = 0;
	mutex_init(&(rsz_conf_chan->lock));
	/* taking the configuartion     structure in private data */
	filp->private_data = rsz_conf_chan;


	return 0;

}

/*
=====================rsz_release===========================
 The Function	is used	to release the number of resources occupied
 by the channel
*/
static int rsz_release(struct inode *inode, struct file *filp)
{
	struct imp_logical_channel *rsz_conf_chan =
	    (struct imp_logical_channel *)filp->private_data;


	/* Lock the channel */
	mutex_lock(&(rsz_conf_chan->lock));

	/* It will free all the input and output buffers */
	imp_common_free_buffers(rsz_device, rsz_conf_chan);

	if (rsz_conf_chan->config_state == STATE_CONFIGURED) {
		if (!rsz_conf_chan->chained) {
			imp_hw_if->dealloc_config_block(rsz_device,
							rsz_conf_chan->config);
		}
		kfree(rsz_conf_chan->user_config);
	}
	mutex_unlock(&(rsz_conf_chan->lock));
	mutex_lock(&rsz_dev.lock);
	rsz_dev.users--;
	kfree(filp->private_data);
	filp->private_data = NULL;
	mutex_unlock(&rsz_dev.lock);
	return 0;
}				/*  End     of function     resizer_release */

/*
=====================rsz_mmap===========================
Function to map device memory into user	space
 */
static int rsz_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct imp_logical_channel *chan =
	    (struct imp_logical_channel *)filp->private_data;
	return (imp_common_mmap(filp, vma, chan));
}				/*     End     of Function     resizer_mmap */

/*
=====================rsz_ioctl===========================
This function	will process IOCTL commands sent by
the application	and
control the device IO operations.
*/
static int rsz_doioctl(struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	int ret = 0, prio;
	unsigned mode, user_mode;
	/*get the configuratin of this channel from
	   private_date member of file */
	struct imp_logical_channel *rsz_conf_chan =
	    (struct imp_logical_channel *)file->private_data;

	if (ISNULL(rsz_conf_chan)) {
		dev_err(rsz_device, "channel ptr is null\n");
		return -EFAULT;
	}

	if (ISNULL((void *)arg)) {
		dev_err(rsz_device, "arg ptr is null\n");
		return -EFAULT;
	}

	mode = imp_hw_if->get_resize_oper_mode();
	switch (cmd) {
	case RSZ_QUERYBUF:
	case RSZ_REQBUF:
	case RSZ_RESIZE:
	case RSZ_RECONFIG:
		{
			if (mode == IMP_MODE_CONTINUOUS)
				return -EACCES;
		}
		break;
	}

	switch (cmd) {
	case RSZ_S_OPER_MODE:
		{
			dev_dbg(rsz_device, "RSZ_S_OPER_MODE:\n");
			user_mode = *((unsigned long *)arg);
			if (rsz_conf_chan->mode != IMP_MODE_INVALID) {
				dev_err(rsz_device,
					"Mode set for this channel already\n");
				ret = -EINVAL;
				goto ERROR;
			}

			if (user_mode >= IMP_MODE_INVALID) {
				dev_err(rsz_device, "Invalid mode\n");
				ret = -EINVAL;
				goto ERROR;
			}

			if (user_mode != mode) {
				dev_err(rsz_device,
					"Operation mode doesn't match"
					" with current hw mode\n");
				ret = -EINVAL;
				goto ERROR;
			}

			ret = mutex_lock_interruptible(&(rsz_conf_chan->lock));
			if (!ret) {
				rsz_conf_chan->mode = mode;
				mutex_unlock(&(rsz_conf_chan->lock));
			}
			dev_dbg(rsz_device,
				"RSZ_S_OPER_MODE: Operation mode set to %d",
				user_mode);
		}
		break;
		/* if case is to query for buffer address */
	case RSZ_G_OPER_MODE:
		{
			dev_dbg(rsz_device, "RSZ_G_OPER_MODE:\n");
			*((unsigned long *)arg) = rsz_conf_chan->mode;
			dev_dbg(rsz_device,
				"RSZ_G_OPER_MODE: mode = %d",
				rsz_conf_chan->mode);
		}
		break;

	case RSZ_S_CONFIG:
		{
			dev_dbg(rsz_device, "RSZ_S_CONFIG:\n");
			ret = mutex_lock_interruptible(&(rsz_conf_chan->lock));
			if (!ret) {
				ret = imp_set_resizer_config(rsz_device,
						     rsz_conf_chan,
						     (struct rsz_channel_config
						      *)arg);
				mutex_unlock(&(rsz_conf_chan->lock));
			}
		}
		break;

	case RSZ_G_CONFIG:
		{
			struct rsz_channel_config *user_config =
			    (struct rsz_channel_config *)arg;

			dev_dbg(rsz_device, "RSZ_G_CONFIG:%d:%d:%d\n",
				user_config->oper_mode, user_config->chain,
				user_config->len);
			if (ISNULL(user_config->config)) {
				ret = -EINVAL;
				dev_err(rsz_device,
					"error in PREV_GET_CONFIG\n");
				goto ERROR;
			}
			ret =
			    imp_get_resize_config(rsz_device, rsz_conf_chan,
						  user_config);
		}
		break;

	case RSZ_QUERYBUF:
		{
			dev_dbg(rsz_device, "RSZ_QUERYBUF:\n");
			ret = mutex_lock_interruptible(&(rsz_conf_chan->lock));
			if (!ret) {
				ret = imp_common_query_buffer(rsz_device,
						      rsz_conf_chan,
						      (struct imp_buffer *)arg);
				mutex_unlock(&(rsz_conf_chan->lock));
			}
		}
		break;

		/* if case is to request buffers */
	case RSZ_REQBUF:
		{
			dev_dbg(rsz_device, "RSZ_REQBUF:\n");
			ret = mutex_lock_interruptible(&(rsz_conf_chan->lock));
			if (!ret) {
				ret = imp_common_request_buffer(rsz_device,
							rsz_conf_chan,
							(struct imp_reqbufs *)
							arg);
				mutex_unlock(&(rsz_conf_chan->lock));
			}
		}
		break;
		/* if the case is to do resize */
	case RSZ_S_PRIORITY:
		{
			prio = *((unsigned long *)arg);

			dev_dbg(rsz_device, "RSZ_S_PRIORITY: priority = %d\n",
				prio);
			/* Check the prioroty range and assign the priority */
			if (prio > MAX_PRIORITY || prio < MIN_PRIORITY) {
				ret = -EINVAL;
				goto ERROR;
			} else {
				ret = mutex_lock_interruptible(
					&(rsz_conf_chan->lock));
				if (!ret) {
					rsz_conf_chan->priority = prio;
					mutex_unlock(&(rsz_conf_chan->lock));
				}
			}
			dev_dbg(rsz_device, "\n resizer_Priority:end");
		}
		break;
		/* This ioctl is used to get the priority of
		   the current logic channel */
	case RSZ_G_PRIORITY:
		{
			dev_dbg(rsz_device, "RSZ_S_PRIORITY: \n");
			/* Get the priority     from the channel */
			*((unsigned long *)arg) = rsz_conf_chan->priority;
		}
		break;

	case RSZ_RESIZE:
		{
			dev_dbg(rsz_device, "RSZ_RESIZE: \n");
			ret = mutex_lock_interruptible(&(rsz_conf_chan->lock));
			if (!ret) {
				ret = imp_common_start_resize(rsz_device,
						      rsz_conf_chan,
						      (struct imp_convert *)
						      arg);
				mutex_unlock(&(rsz_conf_chan->lock));
			}
		}
		break;

	case RSZ_RECONFIG:
		{
			dev_dbg(rsz_device, "RSZ_RECONFIG: \n");
			ret = mutex_lock_interruptible(&(rsz_conf_chan->lock));
			if (!ret) {
				ret = imp_common_reconfig_resizer(rsz_device,
						(struct rsz_reconfig *)arg,
						 rsz_conf_chan);
				mutex_unlock(&(rsz_conf_chan->lock));
			}
		}
		break;

#ifdef CONFIG_IMP_DEBUG
	case RSZ_DUMP_HW_CONFIG:
		{
			dev_dbg(rsz_device, "RSZ_DUMP_HW_CONFIG: \n");
			ret = mutex_lock_interruptible(&(rsz_conf_chan->lock));
			if (!ret) {
				if (imp_hw_if->dump_hw_config)
					imp_hw_if->dump_hw_config();
				mutex_unlock(&(rsz_conf_chan->lock));
			}
		}
		break;
#endif
	default:
		dev_dbg(rsz_device, "resizer_ioctl: Invalid Command Value");
		ret = -EINVAL;
	}

ERROR:
	return ret;
}				/*End of function IOCTL */

static long rsz_ioctl(struct file *file,
		     unsigned int cmd, unsigned long arg)
{
	int ret;
	char sbuf[128];
	void *mbuf = NULL;
	void *parg = NULL;

	dev_dbg(rsz_device, "Start of resizer ioctl\n");

	/*  Copy arguments into temp kernel buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_NONE:
		parg = NULL;
		break;
	case _IOC_READ:
	case _IOC_WRITE:
	case (_IOC_WRITE | _IOC_READ):
		if (_IOC_SIZE(cmd) <= sizeof(sbuf))
			parg = sbuf;
		else {
			/* too big to allocate from stack */
			mbuf = kmalloc(_IOC_SIZE(cmd), GFP_KERNEL);
			if (ISNULL(mbuf))
				return -ENOMEM;
			parg = mbuf;
		}

		ret = -EFAULT;
		if (_IOC_DIR(cmd) & _IOC_WRITE)
			if (copy_from_user(parg, (void __user *)arg,
					   _IOC_SIZE(cmd)))
				goto out;
		break;
	}

	/* call driver */
	ret = rsz_doioctl(file, cmd, (unsigned long)parg);
	if (ret == -ENOIOCTLCMD)
		ret = -EINVAL;

	/*  Copy results into user buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_READ:
	case (_IOC_WRITE | _IOC_READ):
		if (copy_to_user((void __user *)arg, parg, _IOC_SIZE(cmd)))
			ret = -EFAULT;
		break;
	}
out:
	kfree(mbuf);

	dev_dbg(rsz_device, "End of previewer ioctl\n");
	return ret;
}

static struct file_operations rsz_fops = {
	.owner = THIS_MODULE,
	.open = rsz_open,
	.release = rsz_release,
	.mmap = rsz_mmap,
	.unlocked_ioctl = rsz_ioctl,
};

static void resizer_platform_release(struct device *device)
{
	/* This is called when the reference count goes to zero */
}

static int resizer_probe(struct device *device)
{
	rsz_device = device;
	return 0;
}
static int resizer_remove(struct device *device)
{
	return 0;
}
static struct class *rsz_class;

static struct platform_device resizer_device = {
	.name = "davinci_resizer",
	.id = 2,
	.dev = {
		.release = resizer_platform_release,
	}
};

static struct device_driver resizer_driver = {
	.name = "davinci_resizer",
	.bus = &platform_bus_type,
	.probe = resizer_probe,
	.remove = resizer_remove,
};

/*
=====================rsz_init===========================
function to	register resizer character driver
*/
static __init int rsz_init(void)
{

	int result;

	/* Register     the     driver in the kernel */

	result = alloc_chrdev_region(&dev, 0, 1, DRIVER_NAME);
	if (result < 0) {
		printk(KERN_ERR
		       "DaVinciresizer: could not register character device");
		return -ENODEV;
	}
	/* Initialize of character device */
	cdev_init(&c_dev, &rsz_fops);
	c_dev.owner = THIS_MODULE;
	c_dev.ops = &rsz_fops;

	/* addding character device */
	result = cdev_add(&c_dev, dev, 1);

	if (result) {
		printk(KERN_ERR
		       "DaVinciresizer:Error %d adding"
			" Davinciresizer ..error no:",
		       result);
		unregister_chrdev_region(dev, 1);
		return result;
	}

	/* registeration of     character device */
	register_chrdev(MAJOR(dev), DRIVER_NAME, &rsz_fops);

	/* register driver as a platform driver */
	if (driver_register(&resizer_driver) != 0) {
		unregister_chrdev_region(dev, 1);
		cdev_del(&c_dev);
		return -EINVAL;
	}

	/* Register the drive as a platform device */
	if (platform_device_register(&resizer_device) != 0) {
		driver_unregister(&resizer_driver);
		unregister_chrdev_region(dev, 1);
		unregister_chrdev(MAJOR(dev), DRIVER_NAME);
		cdev_del(&c_dev);
		return -EINVAL;
	}

	rsz_class = class_create(THIS_MODULE, "davinci_resizer");

	if (!rsz_class) {

		platform_device_unregister(&resizer_device);
		cdev_del(&c_dev);
		unregister_chrdev(MAJOR(dev), DRIVER_NAME);

		return -EIO;
	}

	device_create(rsz_class, NULL, dev, NULL, "davinci_resizer");

	rsz_dev.users = 0;

	mutex_init(&rsz_dev.lock);
	/* Initialize the serializer */
	imp_init_serializer();
	imp_hw_if = imp_get_hw_if();
	printk(KERN_NOTICE "davinci_resizer initialized\n");
	return 0;
}				/* End   of function  resizer_init */

/*
=====================rsz_cleanup===========================
Function	is called by the kernel. It	unregister the device.
*/
static void __exit rsz_cleanup(void)
{

	unregister_chrdev_region(dev, 1);

	device_destroy(rsz_class, dev);

	class_destroy(rsz_class);

	driver_unregister(&resizer_driver);

	/* remove platform device */
	platform_device_unregister(&resizer_device);

	cdev_del(&c_dev);

	/* unregistering the driver     from the kernel */
	unregister_chrdev(MAJOR(dev), DRIVER_NAME);

}				/* End  of function   resizer_cleanup */

module_init(rsz_init);
module_exit(rsz_cleanup);

MODULE_LICENSE("GPL");
