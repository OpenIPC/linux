/*
 * Copyright (C) 2008 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

//#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>

#include <media/davinci/imp_hw_if.h>
#include <media/davinci/imp_previewer.h>

#include <mach/cputype.h>

#define DRIVERNAME  "DaVinciPreviewer"

struct device *prev_dev;

/* prev_device structure */
static struct prev_device prevdevice;

/* instance of the imp interface */
static struct imp_hw_interface *imp_hw_if;

/* Functions */
int previewer_open(struct inode *inode, struct file *filp)
{
	struct prev_device *device = &prevdevice;
	struct prev_fh *fh;
	int i;

	if (filp->f_flags & O_NONBLOCK) {
		dev_err
		    (prev_dev,
		     "previewer_open: device cannot be "
			"opened in non-blocked mode\n");
		return -EBUSY;
	}
	/* allocate memory for a the file handle */

	fh = kmalloc(sizeof(struct prev_fh), GFP_KERNEL);
	if (fh == NULL)
		return -ENOMEM;

	mutex_lock(&device->lock);
	if (!device->users) {
		device->chan->mode = IMP_MODE_INVALID;
		device->chan->primary_user = 0;
		device->chan->chained = 0;
		/* Preview channel has maximum priority
		   since it is used with a capture application
		   and needs to achieve required fps throughput
		 */
		device->chan->config = NULL;
		device->chan->user_config_size = 0;
		device->chan->user_config = NULL;
		device->chan->config_state = STATE_NOT_CONFIGURED;
		device->chan->in_numbufs = 0;
		device->chan->out_numbuf1s = 0;
		device->chan->out_numbuf2s = 0;
		for (i = 0; i < MAX_BUFFERS; i++) {
			device->chan->in_bufs[i] = NULL;
			device->chan->out_buf1s[i] = NULL;
			device->chan->out_buf2s[i] = NULL;
		}
		device->chan->priority = MAX_PRIORITY;
		init_completion(&(device->chan->channel_sem));
		device->chan->channel_sem.done = 0;
		mutex_init(&(device->chan->lock));
	}
	device->users++;
	mutex_unlock(&device->lock);
	/* store the pointer of prev_params in private_data member of file
	   and params member of prev_device */
	filp->private_data = fh;
	fh->chan = device->chan;
	fh->primary_user = 0;

	return 0;
}

int previewer_release(struct inode *inode, struct file *filp)
{
	/* get the configuratin from private_date member of file */
	struct prev_fh *fh;
	struct imp_logical_channel *chan;
	struct prev_device *device = &prevdevice;
	unsigned int state;
	unsigned int mode;

	mutex_lock(&device->lock);
	fh = (struct prev_fh *)filp->private_data;
	chan = fh->chan;

	/* if hw is busy in continuous mode, and this is primary
	   user, we can not close the device
	 */
	state = imp_hw_if->get_hw_state();
	mode = imp_hw_if->get_preview_oper_mode();

	if ((device->users != 1) && (fh->primary_user)) {
		dev_err(prev_dev,
			"Close other instances before"
			" this primary user instance\n");
		mutex_unlock(&device->lock);
		return -EBUSY;
	}

	device->users--;
	if (fh->primary_user) {
		/* call free_buffers to free memory allocated to buffers */
		imp_common_free_buffers(prev_dev, chan);
		chan->primary_user = 0;
		if (chan->config_state == STATE_CONFIGURED)
			kfree(chan->user_config);
	}

	kfree(filp->private_data);
	/* Assign null to private_data member of file and params
	   member of device */
	filp->private_data = NULL;
	mutex_unlock(&device->lock);
	return 0;
}

int previewer_mmap(struct file *filp, struct vm_area_struct *vma)
{
	/* get the address of global object of prev_device structure */
	struct prev_device *device = &prevdevice;
	return (imp_common_mmap(filp, vma, device->chan));
}

int previewer_doioctl(struct file *file,
		      unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct prev_fh *fh = (struct prev_fh *)file->private_data;
	struct imp_logical_channel *chan = fh->chan;
	unsigned int mode, user_mode;

	if (ISNULL(chan)) {
		dev_err(prev_dev, "channel ptr is null\n");
		return -EFAULT;
	}

	if (ISNULL((void *)arg)) {
		dev_err(prev_dev, "arg ptr is null\n");
		return -EFAULT;
	}


	mode = imp_hw_if->get_preview_oper_mode();

	switch (cmd) {
	case PREV_QUERYBUF:
	case PREV_REQBUF:
	case PREV_S_PARAM:
	case PREV_PREVIEW:
	case PREV_S_CONFIG:
		{
			if (!fh->primary_user)
				return -EACCES;
		}
		break;
	}

	switch (cmd) {
	case PREV_QUERYBUF:
	case PREV_REQBUF:
	case PREV_PREVIEW:
		{
			if (chan->mode == PREV_MODE_CONTINUOUS)
				return -EACCES;
		}
		break;
	}

	/* switch according value of cmd */
	switch (cmd) {

	case PREV_S_OPER_MODE:
		{
			user_mode = *((unsigned long *)arg);
			if (chan->mode != IMP_MODE_INVALID) {
				dev_err(prev_dev,
					"Mode set for this channel already\n");
				ret = -EINVAL;
				goto ERROR;
			}

			/* primary user trying to set mode */
			if (user_mode >= IMP_MODE_INVALID) {
				dev_err(prev_dev, "Invalid mode\n");
				ret = -EINVAL;
				goto ERROR;
			}

			if (user_mode != mode) {
				dev_err(prev_dev,
					"Operation mode doesn't match"
					" with current hw mode\n");
				ret = -EINVAL;
				goto ERROR;
			}

			if (mutex_lock_interruptible(&chan->lock)) {
				ret = -EINTR;
				goto ERROR;
			}

			chan->mode = mode;
			chan->primary_user = 1;
			fh->primary_user = 1;
			mutex_unlock(&chan->lock);
			dev_dbg(prev_dev,
				"PREV_S_OPER_MODE: Operation mode set to %d\n",
				user_mode);
		}
		break;
		/* if case is to query for buffer address */
	case PREV_G_OPER_MODE:
		{
			*(unsigned long *)arg = chan->mode;
			dev_dbg(prev_dev,
				"PREV_G_OPER_MODE: mode = %d\n", chan->mode);
		}
		break;

	case PREV_ENUM_CAP:
		{
			struct prev_cap *cap = (struct prev_cap *)arg;
			struct prev_module_if *module_if;
			dev_dbg(prev_dev, "PREV_ENUM_CAP:\n");

			module_if =
			    imp_hw_if->prev_enum_modules(prev_dev, cap->index);

			if (ISNULL(module_if)) {
				dev_dbg(prev_dev,
					"PREV_ENUM_CAP - Last module \n");
				ret = -EINVAL;
				goto ERROR;
			} else {
				strcpy(cap->version, module_if->version);
				cap->module_id = module_if->module_id;
				cap->control = module_if->control;
				cap->path = module_if->path;
				strcpy(cap->module_name,
				       module_if->module_name);
			}
		}
		break;

		/* if case is to set configuration parameters */
	case PREV_S_PARAM:
		{
			struct prev_module_param *module_param =
			    (struct prev_module_param *)arg;
			struct prev_module_if *module_if;

			dev_dbg(prev_dev, "PREV_S_PARAM:\n");

			if (chan->config_state != STATE_CONFIGURED) {
				dev_err(prev_dev, "Channel not configured\n");
				ret = -EINVAL;
				goto ERROR;
			}

			module_if =
			    imp_get_module_interface(prev_dev,
						     module_param->module_id);
			if (ISNULL(module_if)) {
				dev_err(prev_dev, "Invalid module id\n");
				ret = -EINVAL;
				goto ERROR;
			} else {
				if (strcmp
				    (module_if->version,
				     module_param->version)) {
					dev_err(prev_dev,
						"Invalid module version\n");
					ret = -EINVAL;
					goto ERROR;
				}
				/* we have a valid */
				ret = module_if->set(prev_dev,
						     module_param->
						     param, module_param->len);
				if (ret < 0) {
					dev_err(prev_dev,
						"error in PREV_S_PARAM\n");
					goto ERROR;
				}
			}
		}
		break;
		/* if case is to get configuration parameters */
	case PREV_G_PARAM:
		{
			struct prev_module_param *module_param =
			    (struct prev_module_param *)arg;
			struct prev_module_if *module_if;

			dev_dbg(prev_dev, "PREV_G_PARAM:\n");

			if (ISNULL(module_param)) {
				ret = -EINVAL;
				goto ERROR;
			}
			module_if =
			    imp_get_module_interface(prev_dev,
						     module_param->module_id);
			if (ISNULL(module_if)) {
				dev_err(prev_dev, "Invalid module id\n");
				ret = -EINVAL;
				goto ERROR;
			} else {
				if (strcmp
				    (module_if->version,
				     module_param->version)) {
					dev_err(prev_dev,
						"Invalid module version\n");
					ret = -EINVAL;
					goto ERROR;
				}

				ret = module_if->get(prev_dev,
						     module_param->param,
						     module_param->len);
				if (ret < 0) {
					dev_err(prev_dev,
						"error in PREV_G_PARAM\n");
					goto ERROR;
				}
			}
		}
		break;

	case PREV_S_CONFIG:
		{
			dev_dbg(prev_dev, "PREV_S_CONFIG:\n");
			if (mutex_lock_interruptible(&(chan->lock))) {
				ret = -EINTR;
				goto ERROR;
			}

			ret =
			    imp_set_preview_config(prev_dev, chan,
						   (struct prev_channel_config
						    *)arg);
			mutex_unlock(&(chan->lock));
		}
		break;

	case PREV_G_CONFIG:
		{
			struct prev_channel_config *user_config =
			    (struct prev_channel_config *)arg;

			dev_dbg(prev_dev, "PREV_G_CONFIG:\n");
			if (ISNULL(user_config->config)) {
				ret = -EINVAL;
				dev_err(prev_dev, "error in PREV_GET_CONFIG\n");
				goto ERROR;
			}

			ret =
			    imp_get_preview_config(prev_dev, chan, user_config);
		}
		break;
	case PREV_S_CONTROL:
		{
			struct prev_module_if *module_if;
			struct prev_control *control_param =
			    (struct prev_control *)arg;

			dev_dbg(prev_dev, "PREV_S_CONTROL:\n");
			if (ISNULL(control_param)) {
				ret = -EINVAL;
				goto ERROR;
			}

			if (chan->mode != IMP_MODE_CONTINUOUS) {
				dev_err(prev_dev,
					"Control operation allowed"
					" only in continuous mode\n");
				ret = -EINVAL;
				goto ERROR;
			}
			module_if =
			    imp_get_module_interface(prev_dev,
						     control_param->module_id);
			if (ISNULL(module_if)) {
				dev_err(prev_dev, "Invalid module id\n");
				ret = -EINVAL;
				goto ERROR;
			} else {
				if (strcmp
				    (module_if->version,
				     control_param->version)) {
					dev_err(prev_dev,
						"Invalid module version\n");
					ret = -EINVAL;
					goto ERROR;
				}
			}

			if (mutex_lock_interruptible(&chan->lock)) {
				ret = -EINTR;
				goto ERROR;
			}

			ret = module_if->set(prev_dev,
					     control_param->param,
					     control_param->len);
			if (ret < 0) {
				dev_err(prev_dev,
					"error in handling PREV_SET_CONTROL\n");
			}
			mutex_unlock(&(chan->lock));
		}
		break;
	case PREV_G_CONTROL:
		{
			struct prev_module_if *module_if;
			struct prev_control *control_param =
			    (struct prev_control *)arg;

			dev_dbg(prev_dev, "PREV_G_CONTROL:\n");
			if (ISNULL(control_param)) {
				ret = -EINVAL;
				goto ERROR;
			}
			module_if =
			    imp_get_module_interface(prev_dev,
						     control_param->module_id);
			if (ISNULL(module_if)) {
				dev_err(prev_dev, "Invalid module id\n");
				ret = -EINVAL;
				goto ERROR;
			} else {
				if (strcmp
				    (module_if->version,
				     control_param->version)) {
					dev_err(prev_dev,
						"Invalid module version\n");
					ret = -EINVAL;
					goto ERROR;
				}
			}
			ret = module_if->get(prev_dev,
					     control_param->param,
					     control_param->len);
			if (ret < 0) {
				dev_err(prev_dev,
					"error in handling PREV_GET_CONTROL\n");
			}
		}
		break;

	case PREV_QUERYBUF:
		{
			dev_dbg(prev_dev, "PREV_QUERYBUF:\n");
			if (mutex_lock_interruptible(&chan->lock)) {
				ret = -EINTR;
				goto ERROR;
			}

			/* call query buffer which will return buffer address */
			ret = imp_common_query_buffer(prev_dev,
						      chan,
						      (struct imp_buffer *)arg);
			mutex_unlock(&(chan->lock));
		}
		break;

		/* if case is to request buffers */
	case PREV_REQBUF:
		{
			dev_dbg(prev_dev, "PREV_REQBUF:\n");
			if (mutex_lock_interruptible(&chan->lock)) {
				ret = -EINTR;
				goto ERROR;
			}

			/* call request buffer to allocate buffers */
			ret = imp_common_request_buffer(prev_dev,
							chan,
							(struct imp_reqbufs *)
							arg);
			mutex_unlock(&(chan->lock));
		}
		break;

		/* if the case is to do previewing */
	case PREV_PREVIEW:
		{
			dev_dbg(prev_dev, "PREV_PREVIEW:\n");
			if (mutex_lock_interruptible(&chan->lock)) {
				ret = -EINTR;
				goto ERROR;
			}
			ret =
			    imp_common_start_preview(prev_dev, chan,
						     (struct imp_convert *)arg);
			mutex_unlock(&(chan->lock));
		}
		break;
#ifdef CONFIG_IMP_DEBUG
	case PREV_DUMP_HW_CONFIG:
		{
			dev_dbg(prev_dev, "PREV_DUMP_HW_CONFIG:\n");
			if (mutex_lock_interruptible(&chan->lock)) {
				ret = -EINTR;
				goto ERROR;
			}
			if (imp_hw_if->dump_hw_config)
				imp_hw_if->dump_hw_config();
			mutex_unlock(&(chan->lock));
		}
		break;
#endif
	default:
		dev_err(prev_dev, "previewer_ioctl: Invalid Command Value\n");
		ret = -EINVAL;
	}

ERROR:
	return ret;
}

static long previewer_ioctl(struct file *file,
			   unsigned int cmd, unsigned long arg)
{
	int ret;
	char sbuf[128];
	void *mbuf = NULL;
	void *parg = NULL;

	dev_dbg(prev_dev, "Start of previewer ioctl\n");

	/*  Copy arguments into temp kernel buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_NONE:
		parg = NULL;
		break;
	case _IOC_READ:
	case _IOC_WRITE:
	case (_IOC_WRITE | _IOC_READ):
		if (_IOC_SIZE(cmd) <= sizeof(sbuf)) {
			parg = sbuf;
		} else {
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
	ret = previewer_doioctl(file, cmd, (unsigned long)parg);
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

	dev_dbg(prev_dev, "End of previewer ioctl\n");
	return ret;
}

static void previewer_platform_release(struct device *device)
{
	/* This is called when the reference count goes to zero */
}

static int previewer_probe(struct device *device)
{
	prev_dev = device;
	return 0;
}

static int previewer_remove(struct device *device)
{
	return 0;
}

/* global variable of type file_operations containing function
pointers of file operations */
static struct file_operations prev_fops = {
	.owner = THIS_MODULE,
	.open = previewer_open,
	.release = previewer_release,
	.mmap = previewer_mmap,
	.unlocked_ioctl = previewer_ioctl,
};

/* global variable of type cdev to register driver to the kernel */
static struct cdev cdev;

/* global variable which keeps major and minor number of the driver in it */
static dev_t dev;

static struct class *prev_class;

static struct platform_device previewer_device = {
	.name = "davinci_previewer",
	.id = 2,
	.dev = {
		.release = previewer_platform_release,
	}
};

static struct device_driver previewer_driver = {
	.name = "davinci_previewer",
	.bus = &platform_bus_type,
	.probe = previewer_probe,
	.remove = previewer_remove
};

int __init previewer_init(void)
{
	int result;
	struct imp_logical_channel *chan_config = NULL;

	/* Register the driver in the kernel */
	/* dynamically get the major number for the driver using
	   alloc_chrdev_region function */
	result = alloc_chrdev_region(&dev, 0, 1, DRIVERNAME);

	/* if it fails return error */
	if (result < 0) {
		printk(KERN_ERR "DaVinciPreviewer: Module intialization \
		failed. could not register character device\n");
		return -ENODEV;
	}

	/* initialize cdev with file operations */
	cdev_init(&cdev, &prev_fops);
	cdev.owner = THIS_MODULE;
	cdev.ops = &prev_fops;

	/* add cdev to the kernel */
	result = cdev_add(&cdev, dev, 1);

	if (result) {
		unregister_chrdev_region(dev, 1);
		printk(KERN_ERR
		       "DaVinciPreviewer: Error adding "
			"DavinciPreviewer .. error no:%d\n",
		       result);
		return -EINVAL;
	}

	/* register character driver to the kernel */
	register_chrdev(MAJOR(dev), DRIVERNAME, &prev_fops);

	/* register driver as a platform driver */
	if (driver_register(&previewer_driver) != 0) {
		unregister_chrdev_region(dev, 1);
		cdev_del(&cdev);
		return -EINVAL;
	}

	/* Register the drive as a platform device */
	if (platform_device_register(&previewer_device) != 0) {
		driver_unregister(&previewer_driver);
		unregister_chrdev_region(dev, 1);
		unregister_chrdev(MAJOR(dev), DRIVERNAME);
		cdev_del(&cdev);
		return -EINVAL;
	}

	prev_class = class_create(THIS_MODULE, "davinci_previewer");
	if (!prev_class) {
		printk(KERN_NOTICE
		       "previewer_init: error in creating device class\n");
		driver_unregister(&previewer_driver);
		platform_device_unregister(&previewer_device);
		unregister_chrdev_region(dev, 1);
		unregister_chrdev(MAJOR(dev), DRIVERNAME);
		cdev_del(&cdev);
		return -EIO;
	}

	device_create(prev_class, NULL, dev, NULL, "davinci_previewer");

	prevdevice.users = 0;
	chan_config = kmalloc(sizeof(struct imp_logical_channel), GFP_KERNEL);
	if (ISNULL(chan_config)) {
		unregister_chrdev_region(dev, 1);
		device_destroy(prev_class, dev);
		class_destroy(prev_class);
		driver_unregister(&previewer_driver);
		platform_device_unregister(&previewer_device);
		cdev_del(&cdev);
		unregister_chrdev(MAJOR(dev), DRIVERNAME);
		return -ENOMEM;
	}
	prevdevice.chan = chan_config;
	prevdevice.chan->priority = MAX_PRIORITY;
	prevdevice.chan->type = IMP_PREVIEWER;
	if (!cpu_is_davinci_dm644x())
		imp_init_serializer();
	/* initialize mutex to 0 */
	mutex_init(&(prevdevice.lock));
	mutex_init(&(prevdevice.chan->lock));
	imp_hw_if = imp_get_hw_if();
	printk(KERN_NOTICE "davinci_previewer initialized\n");
	return 0;
}

void __exit previewer_cleanup(void)
{
	/* remove major number allocated to this driver */
	unregister_chrdev_region(dev, 1);

	/* remove simple class device */
	device_destroy(prev_class, dev);

	/* destroy simple class */
	class_destroy(prev_class);

	/* Remove platform driver */
	driver_unregister(&previewer_driver);

	/* remove platform device */
	platform_device_unregister(&previewer_device);

	cdev_del(&cdev);

	/* unregistering the driver from the kernel */
	unregister_chrdev(MAJOR(dev), DRIVERNAME);

	kfree(prevdevice.chan);
}

module_init(previewer_init)
module_exit(previewer_cleanup)

MODULE_LICENSE("GPL");
