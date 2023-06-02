/* *
 * Copyright (C) 2009 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/major.h>
#include <linux/sched.h>
#include <media/v4l2-device.h>
#include <media/davinci/dm365_a3_hw.h>
#include <media/davinci/vpss.h>


/*Global structure for device */
static struct af_device *af_dev_configptr;

/* For registeration of character device */
static struct cdev c_dev;

/* device structure to make entry in device */
static struct class *af_class;
static dev_t dev;
struct device *afdev;

/* inline function to free reserver pages  */
inline void af_free_pages(unsigned long addr, unsigned long bufsize)
{
	unsigned long tempaddr;
	unsigned long size;
	tempaddr = addr;
	if (!addr)
		return;
	size = PAGE_SIZE << (get_order(bufsize));
	while (size > 0) {
		ClearPageReserved(virt_to_page(addr));
		addr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	free_pages(tempaddr, get_order(bufsize));
}

/* Function to check paxel parameters */
static int af_validate_parameters(void)
{
	int result = 0;
	dev_dbg(afdev, "E\n");
	/* Check horizontal Count */
	if ((af_dev_configptr->config->paxel_config.hz_cnt <
	     AF_PAXEL_HORIZONTAL_COUNT_MIN) ||
	   (af_dev_configptr->config->paxel_config.hz_cnt >
		AF_PAXEL_HORIZONTAL_COUNT_MAX)) {
		dev_err(afdev, "\n Invalid Parameters");
		dev_err(afdev, "\n Paxel Horizontal Count is incorrect");
		result = -EINVAL;
	}
	/* Check Vertical Count */
	if ((af_dev_configptr->config->paxel_config.vt_cnt <
	     AF_PAXEL_VERTICAL_COUNT_MIN) ||
	    (af_dev_configptr->config->paxel_config.vt_cnt >
		AF_PAXEL_VERTICAL_COUNT_MAX)) {
		dev_err(afdev, "\n Invalid Parameters");
		dev_err(afdev, "\n Paxel Vertical Count is incorrect");
		result = -EINVAL;
	}
	/* Check line increment */
	if ((NOT_EVEN ==
	     CHECK_EVEN(af_dev_configptr->config->paxel_config.line_incr)) ||
	   (af_dev_configptr->config->paxel_config.line_incr <
		AF_LINE_INCR_MIN) ||
	   (af_dev_configptr->config->paxel_config.line_incr >
		AF_LINE_INCR_MAX)) {
		dev_err(afdev, "\n Invalid Parameters");
		dev_err(afdev, "\n Paxel Line Increment is incorrect");
		result = -EINVAL;
	}
	if (af_dev_configptr->config->fv_sel == AF_HFV_AND_VFV) {
		if ((NOT_EVEN ==
			CHECK_EVEN(af_dev_configptr->config->\
				paxel_config.column_incr)) ||
			(af_dev_configptr->config->paxel_config.column_incr <
				AF_COLUMN_INCR_MIN) ||
			(af_dev_configptr->config->paxel_config.column_incr >
				AF_COLUMN_INCR_MAX)) {
			dev_err(afdev, "\n Invalid Parameters");
			dev_err(afdev, "\n Paxel Column Increment is"
					"incorrect");
			result = -EINVAL;
		}
	}
	/* Check width */
	if ((NOT_EVEN ==
	     CHECK_EVEN(af_dev_configptr->config->paxel_config.width)) ||
	    (af_dev_configptr->config->paxel_config.width < AF_WIDTH_MIN) ||
	    (af_dev_configptr->config->paxel_config.width > AF_WIDTH_MAX)) {
		dev_err(afdev, "\n Invalid Parameters");
		dev_err(afdev, "\n Paxel Width is incorrect");
		result = -EINVAL;
	}
	/* Check Height */
	if ((NOT_EVEN ==
	     CHECK_EVEN(af_dev_configptr->config->paxel_config.height)) ||
	    (af_dev_configptr->config->paxel_config.height < AF_HEIGHT_MIN) ||
	    (af_dev_configptr->config->paxel_config.height > AF_HEIGHT_MAX)) {
		dev_err(afdev, "\n Invalid Parameters");
		dev_err(afdev, "\n Paxel Height is incorrect");
		result = -EINVAL;
	}
	/* Check Horizontal Start */
	if ((NOT_EVEN ==
	     CHECK_EVEN(af_dev_configptr->config->paxel_config.hz_start)) ||
	    (af_dev_configptr->config->paxel_config.hz_start <
		(af_dev_configptr->config->iir_config.hz_start_pos + 2)) ||
	    (af_dev_configptr->config->paxel_config.hz_start <
		AF_HZSTART_MIN) ||
	    (af_dev_configptr->config->paxel_config.hz_start >
		AF_HZSTART_MAX)) {
		dev_err(afdev, "\n Invalid Parameters");
		dev_err(afdev, "\n Paxel horizontal start is  incorrect");
		result = -EINVAL;
	}
	/* Check Vertical Start */
	if ((af_dev_configptr->config->paxel_config.vt_start <
		AF_VTSTART_MIN) ||
	   (af_dev_configptr->config->paxel_config.vt_start >
		AF_VTSTART_MAX)) {
		dev_err(afdev, "\n Invalid Parameters");
		dev_err(afdev, "\n Paxel vertical start is  incorrect");
		result = -EINVAL;
	}
	/* Check Threshold  */
	if ((af_dev_configptr->config->hmf_config.threshold > AF_MEDTH_MAX) &&
	   (af_dev_configptr->config->hmf_config.enable == H3A_AF_ENABLE)) {
		dev_err(afdev, "\n Invalid Parameters");
		dev_err(afdev,
			"\n Horizontal Median Filter Threshold is incorrect");
		result = -EINVAL;
	}
	/* Check IIRSH start */
	if (af_dev_configptr->config->iir_config.hz_start_pos > AF_IIRSH_MAX) {
		dev_err(afdev, "\n Invalid Parameters");
		dev_err(afdev,
			"\n IIR FITLER  horizontal start position incorrect");
		result = -EINVAL;
	}
	/* Verify ALaw */
	if ((af_dev_configptr->config->alaw_enable < H3A_AF_DISABLE) ||
	   (af_dev_configptr->config->alaw_enable > H3A_AF_ENABLE)) {
		dev_err(afdev, "\n Invalid Parameters");
		dev_err(afdev, "\n ALaw Setting is incorrect");
		result = -EINVAL;
	}
	/* Verify Horizontal Median Filter Setting */
	if ((af_dev_configptr->config->hmf_config.enable < H3A_AF_DISABLE) ||
	   (af_dev_configptr->config->hmf_config.enable > H3A_AF_ENABLE)) {
		dev_err(afdev, "\n Invalid Parameters");
		dev_err(afdev,
			"\n Horizontal Median Filter Setting is incorrect");
		result = -EINVAL;
	}
	/* Check RGB position if HFV used */
	if (af_dev_configptr->config->fv_sel == AF_HFV_ONLY &&
	   ((af_dev_configptr->config->rgb_pos < GR_GB_BAYER) ||
	   (af_dev_configptr->config->rgb_pos > RB_GG_CUSTOM))) {
		dev_err(afdev, "\n Invalid Parameters");
		dev_err(afdev, "\n RGB Position Setting is incorrect");
		result = -EINVAL;
	}
	if (af_dev_configptr->config->fv_sel == AF_HFV_AND_VFV) {
		/* Check for threshold values */
		if (af_dev_configptr->config->fir_config.hfv_thr1 >
			AF_HFV_THR_MAX ||
			af_dev_configptr->config->fir_config.hfv_thr2 >
			AF_HFV_THR_MAX) {
			dev_err(afdev, "\n Invalid Parameters");
			dev_err(afdev, "\n HFV FIR 1 or FIR 2 Threshold"
					" incorrect");
			result = -EINVAL;
		}
		if (af_dev_configptr->config->fir_config.vfv_thr1 >
			AF_VFV_THR_MAX ||
			af_dev_configptr->config->fir_config.vfv_thr2 >
			AF_VFV_THR_MAX) {
			dev_err(afdev, "\n Invalid Parameters");
			dev_err(afdev, "\n VFV FIR 1 or FIR 2 Threshold"
				" incorrect");
			result = -EINVAL;
		}
	}
	dev_dbg(afdev, "L\n");
	return result;
}

/* Function to perform hardware set up */
static int af_hardware_setup(void)
{
	int result;

	/* Size for buffer in bytes */
	int buff_size;
	unsigned long adr, size;
	unsigned int busyaf;
	dev_dbg(afdev, "E\n");

	/* Get the value of PCR register */
	busyaf = af_get_hw_state();

	/* If busy bit is 1 then busy lock registers caanot be configured */
	if (busyaf == 1) {
		/* Hardware cannot be configure while engine is busy */
		dev_err(afdev, "AF_register_setup_ERROR : Engine Busy");
		dev_err(afdev, "\n Configuration cannot be done ");
		return -EBUSY;
	}

	/* Check IIR Coefficient and start Values */
	result = af_validate_parameters();
	if (result < 0)
		return result;

	/* Compute buffer size */
	if (af_dev_configptr->config->fv_sel == AF_HFV_ONLY)
		buff_size =
			(af_dev_configptr->config->paxel_config.hz_cnt) *
			(af_dev_configptr->config->paxel_config.vt_cnt) *
			AF_PAXEL_SIZE_HF_ONLY;
	else
		buff_size =
			(af_dev_configptr->config->paxel_config.hz_cnt) *
			(af_dev_configptr->config->paxel_config.vt_cnt) *
			AF_PAXEL_SIZE_HF_VF;

	/* Deallocate the previosu buffers free old buffers */
	if (af_dev_configptr->buff_old)
		af_free_pages((unsigned long)af_dev_configptr->buff_old,
			      af_dev_configptr->size_paxel);

	/* Free current buffer */
	if (af_dev_configptr->buff_curr)
		af_free_pages((unsigned long)af_dev_configptr->buff_curr,
			      af_dev_configptr->size_paxel);

	/* Free application buffers */
	if (af_dev_configptr->buff_app)
		af_free_pages((unsigned long)af_dev_configptr->buff_app,
			      af_dev_configptr->size_paxel);

	/*
	 * Reallocate the buffer as per new paxel configurations
	 * Allocate memory for old buffer
	 */
	af_dev_configptr->buff_old =
	    (void *)__get_free_pages(GFP_KERNEL | GFP_DMA,
				     get_order(buff_size));

	if (af_dev_configptr->buff_old == NULL)
		return -ENOMEM;

	/* allocate the memory for storing old statistics */
	adr = (unsigned long)af_dev_configptr->buff_old;
	size = PAGE_SIZE << (get_order(buff_size));
	while (size > 0) {
		/*
		 * make sure the frame buffers
		 * are never swapped out of memory
		 */
		SetPageReserved(virt_to_page(adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	/* Allocate memory for current buffer */
	af_dev_configptr->buff_curr =
	    (void *)__get_free_pages(GFP_KERNEL | GFP_DMA,
				     get_order(buff_size));

	/* Free the previously allocated buffer */
	if (af_dev_configptr->buff_curr == NULL) {
		if (af_dev_configptr->buff_old)
			af_free_pages((unsigned long)af_dev_configptr->
				      buff_old, buff_size);
		return -ENOMEM;
	}

	adr = (unsigned long)af_dev_configptr->buff_curr;
	size = PAGE_SIZE << (get_order(buff_size));
	while (size > 0) {
		/*
		 * make sure the frame buffers
		 * are never swapped out of memory
		 */
		SetPageReserved(virt_to_page(adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	/* Allocate memory for old buffer */
	af_dev_configptr->buff_app =
	    (void *)__get_free_pages(GFP_KERNEL | GFP_DMA,
				     get_order(buff_size));

	if (af_dev_configptr->buff_app == NULL) {

		/* Free the previously allocated buffer */
		if (af_dev_configptr->buff_curr)
			af_free_pages((unsigned long)af_dev_configptr->
				      buff_curr, buff_size);
		/* Free the previously allocated buffer */
		if (af_dev_configptr->buff_old)
			af_free_pages((unsigned long)af_dev_configptr->
				      buff_old, buff_size);
		return -ENOMEM;
	}

	adr = (unsigned long)af_dev_configptr->buff_app;
	size = PAGE_SIZE << (get_order(buff_size));
	while (size > 0) {
		/*
		 * make sure the frame buffers
		 * are never swapped out of memory
		 */
		SetPageReserved(virt_to_page(adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	result = af_register_setup(afdev, af_dev_configptr);
	if (result < 0)
		return result;
	af_dev_configptr->size_paxel = buff_size;

	/* Set configuration flag to indicate HW setup done */
	af_dev_configptr->af_config = H3A_AF_CONFIG;

	dev_dbg(afdev, "L\n");
	return 0;
}

/*
 * This function called when driver is opened.It creates Channel
 * Configuration Structure
 */
static int af_open(struct inode *inode, struct file *filp)
{
	dev_dbg(afdev, "E\n");

	/* Return if device is in use */
	if (af_dev_configptr->in_use == AF_IN_USE)
		return -EBUSY;
	af_dev_configptr->config = NULL;

	/* Allocate memory for Device Structure */
	af_dev_configptr->config = kmalloc(sizeof(struct af_configuration)
						, GFP_KERNEL);
	if (af_dev_configptr->config == NULL) {
		dev_err(afdev, "Error : Kmalloc fail\n");
		return -ENOMEM;
	}

	/* Initialize the wait queue */
	init_waitqueue_head(&(af_dev_configptr->af_wait_queue));

	/* Driver is in use */
	af_dev_configptr->in_use = AF_IN_USE;

	/* Hardware is not set up */
	af_dev_configptr->af_config = H3A_AF_CONFIG_NOT_DONE;
	af_dev_configptr->buffer_filled = 0;

	/* Initialize the mutex */
	mutex_init(&(af_dev_configptr->read_blocked));
	dev_dbg(afdev, "L\n");
	return 0;
}

/*
 * This function called when driver is closed.
 * It will deallocate all the buffers.
 */
static int af_release(struct inode *inode, struct file *filp)
{
	dev_dbg(afdev, "E\n");

	af_engine_setup(afdev, 0);
	/* free current buffer */
	if (af_dev_configptr->buff_curr)
		af_free_pages((unsigned long)af_dev_configptr->buff_curr,
			      af_dev_configptr->size_paxel);

	/* Free old buffer */
	if (af_dev_configptr->buff_old)
		af_free_pages((unsigned long)af_dev_configptr->buff_old,
			      af_dev_configptr->size_paxel);

	/* Free application buffer */
	if (af_dev_configptr->buff_app)
		af_free_pages((unsigned long)af_dev_configptr->buff_app,
			      af_dev_configptr->size_paxel);

	/* Release memory for configuration structure of this channel */
	af_dev_configptr->buff_curr = NULL;
	af_dev_configptr->buff_old = NULL;
	af_dev_configptr->buff_app = NULL;
	kfree(af_dev_configptr->config);
	af_dev_configptr->config = NULL;

	/* Device is not in use */
	af_dev_configptr->in_use = AF_NOT_IN_USE;

	dev_dbg(afdev, "L\n");

	return 0;
}
static void af_platform_release(struct device *device)
{
	/* This is called when the reference count goes to zero */
}
static int af_probe(struct device *device)
{
	afdev = device;
	return 0;
}

static int af_remove(struct device *device)
{
	return 0;
}

/*
 * This function will process IOCTL commands sent by the application and
 * control the device IO operations.
 */
static long af_ioctl(struct file *filep,
		    unsigned int cmd, unsigned long arg)
{
	struct af_configuration afconfig = *(af_dev_configptr->config);
	int result = 0;
	dev_dbg(afdev, "E\n");

	/* Block the mutex while ioctl is called */
	result = mutex_lock_interruptible(&af_dev_configptr->read_blocked);
	if (result)
		return result;

	/* Extract the type and number bitfields, and don't decode wrong cmds */
	if (_IOC_TYPE(cmd) != AF_MAGIC_NO) {
		mutex_unlock(&af_dev_configptr->read_blocked);
		return -ENOTTY;
	}

	if (_IOC_NR(cmd) > AF_IOC_MAXNR) {
		mutex_unlock(&af_dev_configptr->read_blocked);
		return -ENOTTY;
	}

	/* Use 'access_ok' to validate user space pointer */
	if (_IOC_DIR(cmd) & _IOC_READ)
		result =
		    !access_ok(VERIFY_WRITE, (void __user *)arg,
			       _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		result =
		    !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (result) {
		mutex_unlock(&af_dev_configptr->read_blocked);
		return -EFAULT;
	}

	switch (cmd) {

		/*
		 * This ioctl is used to perform hardware
		 * set up for AF Engine. It will configure all the registers.
		 */
	case AF_S_PARAM:
		if (copy_from_user(af_dev_configptr->config,
				   (struct af_configuration *)arg,
				   sizeof(struct af_configuration))) {
			/* Release the semaphore */
			mutex_unlock(&af_dev_configptr->read_blocked);
			return -EFAULT;
		}

		/* Call AF_hardware_setup to perform register configuration */
		result = af_hardware_setup();
		if (!result) {
			result = af_dev_configptr->size_paxel;
		} else {
			dev_err(afdev, "Error : AF_S_PARAM failed");
			*(af_dev_configptr->config) = afconfig;
		}
		break;

		/* This ioctl will get the paramters from application */
	case AF_G_PARAM:
		/* Check if Hardware is configured or not */
		if (af_dev_configptr->af_config == H3A_AF_CONFIG) {
			if (copy_to_user((struct af_configuration *)arg,
					 af_dev_configptr->config,
					 sizeof(struct af_configuration))) {
				mutex_unlock(&af_dev_configptr->read_blocked);
				return -EFAULT;
			} else
				result = af_dev_configptr->size_paxel;

		} else {
			dev_dbg(afdev, "Error : AF Hardware not configured.");
			result = -EINVAL;
		}

		break;

		/*
		 * This ioctl will enable AF Engine if hardware configuration
		 * is done
		 */
	case AF_ENABLE:
		/* Check if hardware is configured or not */
		if (af_dev_configptr->af_config == H3A_AF_CONFIG_NOT_DONE) {
			dev_err(afdev, "Error :  AF Hardware not configured.");
			result = -EINVAL;
		} else
			af_engine_setup(afdev, 1);
		break;

		/* This ioctl will disable AF Engine */
	case AF_DISABLE:
		af_engine_setup(afdev, 0);
		break;

	default:
		dev_err(afdev, "Error : Invalid IOCTL!");
		result = -ENOTTY;
		break;
	}

	/* Before returning increment semaphore */
	mutex_unlock(&af_dev_configptr->read_blocked);
	dev_dbg(afdev, "L\n");
	return result;
}

/* Function will return the statistics to user */
ssize_t af_read(struct file *filep, char *kbuff, size_t size, loff_t *offset)
{
	void *buff_temp;
	int result = 0;
	int ret;
	dev_dbg(afdev, "E\n");

	ret = mutex_trylock(&(af_dev_configptr->read_blocked));
	if (!ret) {
		dev_err(afdev, "\n Read Call : busy");
		return -EBUSY;
	}

	/*
	 * If no of bytes specified by the user is less than that of buffer
	 * return error
	 */
	if (size < af_dev_configptr->size_paxel) {
		dev_err(afdev, "\n Error : Invalid buffer size");
		mutex_unlock(&(af_dev_configptr->read_blocked));
		return -1;
	}

	/*
	 * The value of bufffer_filled flag determines
	 * the status of statistics
	 */
	if (af_dev_configptr->buffer_filled == 0) {
		dev_dbg(afdev, "Read call is blocked .......................");
		/*
		 * Block the read call until new statistics are available
		 * or timer expires Decrement the semaphore count
		 */
		wait_event_interruptible_timeout(af_dev_configptr->
						 af_wait_queue,
						 af_dev_configptr->
						 buffer_filled, AF_TIMEOUT);
		dev_dbg(afdev,
			"\n Read Call Unblocked..........................");
	}
	if (af_dev_configptr->buffer_filled == 1) {
		/*
		 * New Statistics are available. Disable the interrupts while
		 * swapping the buffers
		 */
		dev_dbg(afdev, "\n Reading.............................");
		disable_irq(3);

		af_dev_configptr->buffer_filled = 0;

		/* Swap application buffer and old buffer */
		buff_temp = af_dev_configptr->buff_old;
		af_dev_configptr->buff_old = af_dev_configptr->buff_app;
		af_dev_configptr->buff_app = buff_temp;

		dev_dbg(afdev, "\n Reading Done.............................");

		/* Enable the interrupts  once swapping is done */
		enable_irq(3);

		/*
		 * New Statistics are not availaible copy the application
		 * buffer to user. Return the entire statistics to user
		 */
		if (copy_to_user(kbuff, af_dev_configptr->buff_app,
				 af_dev_configptr->size_paxel)) {
			/* Release the semaphore in case of fault */
			mutex_unlock(&(af_dev_configptr->read_blocked));
			return -EFAULT;
		} else
			result = af_dev_configptr->size_paxel;
	}

	/* Release the Mutex */
	mutex_unlock(&(af_dev_configptr->read_blocked));
	dev_dbg(afdev, "\n Read APPLICATION  BUFFER %d",
		*((int *)((af_dev_configptr->buff_app))));
	dev_dbg(afdev, "L\n");
	return result;
}

/* This function will handle the H3A interrupt. */
static irqreturn_t af_isr(int irq, void *dev_id)
{
	/* Temporary buffer for swapping */
	void *buff_temp;
	int enaf;

	dev_dbg(afdev, "E\n");

	/* Get the value of PCR register */
	enaf = af_get_enable();

	/* If AF Engine has enabled, interrupt is not for AF */
	if (!enaf)
		return IRQ_RETVAL(IRQ_NONE);

	/*
	 * Service  the Interrupt.  Set buffer filled flag to indicate
	 * statistics are available. Swap current buffer and old buffer
	 */
	if (af_dev_configptr) {
		buff_temp = af_dev_configptr->buff_curr;
		af_dev_configptr->buff_curr = af_dev_configptr->buff_old;
		af_dev_configptr->buff_old = buff_temp;

		/* Set AF Buf st to current register address */
		if (af_dev_configptr->buff_curr)
			af_set_address(afdev, (unsigned long)
			       virt_to_phys(af_dev_configptr->buff_curr));

		/* Wake up read as new statistics are available */
		af_dev_configptr->buffer_filled = 1;
		wake_up(&(af_dev_configptr->af_wait_queue));
		dev_dbg(afdev, "L\n");
		return IRQ_RETVAL(IRQ_HANDLED);
	}
	return IRQ_RETVAL(IRQ_NONE);
}

/* File Operation Structure */
static const struct file_operations af_fops = {
	.owner = THIS_MODULE,
	.open = af_open,
	.unlocked_ioctl = af_ioctl,
	.read = af_read,
	.release = af_release
};
static struct platform_device afdevice = {
	.name = "dm365_af",
	.id = 2,
	.dev = {
		.release = af_platform_release,
		}
};

static struct device_driver af_driver = {
	.name = "dm365_af",
	.bus = &platform_bus_type,
	.probe = af_probe,
	.remove = af_remove,
};

/* Function to register the AF character device driver. */
#define DRIVERNAME  "DM365AF"
int __init af_init(void)
{
	int err;
	int result = 0;

	/*
	 * Register the driver in the kernel
	 * dynmically get the major number for the driver using
	 * alloc_chrdev_region function
	 */
	result = alloc_chrdev_region(&dev, 0, 1, DRIVERNAME);

	if (result < 0) {
		printk(KERN_ERR "Error :  Could not register character device");
		return -ENODEV;
	}
	printk(KERN_INFO "af major#: %d, minor# %d\n", MAJOR(dev), MINOR(dev));
	/* allocate memory for device structure and initialize it with 0 */
	af_dev_configptr =
	    kmalloc(sizeof(struct af_device), GFP_KERNEL);
	if (!af_dev_configptr) {
		printk(KERN_ERR "Error : kmalloc fail");
		unregister_chrdev_region(dev, AF_NR_DEVS);
		return -ENOMEM;
	}
	/* Initialize character device */
	cdev_init(&c_dev, &af_fops);
	c_dev.owner = THIS_MODULE;
	c_dev.ops = &af_fops;
	err = cdev_add(&c_dev, dev, 1);
	if (err) {
		printk(KERN_ERR "Error : Error in  Adding Davinci AF");
		unregister_chrdev_region(dev, AF_NR_DEVS);
		kfree(af_dev_configptr);
		return -err;
	}
	/* register driver as a platform driver */
	if (driver_register(&af_driver) != 0) {
		unregister_chrdev_region(dev, 1);
		cdev_del(&c_dev);
		return -EINVAL;
	}

	/* Register the drive as a platform device */
	if (platform_device_register(&afdevice) != 0) {
		driver_unregister(&af_driver);
		unregister_chrdev_region(dev, 1);
		cdev_del(&c_dev);
		return -EINVAL;
	}
	af_class = class_create(THIS_MODULE, "dm365_af");
	if (!af_class) {
		printk(KERN_ERR "af_init: error in creating device class\n");
		driver_unregister(&af_driver);
		platform_device_unregister(&afdevice);
		unregister_chrdev_region(dev, 1);
		unregister_chrdev(MAJOR(dev), DRIVERNAME);
		cdev_del(&c_dev);
		return -EINVAL;
	}
	/* register device class */
	device_create(af_class, NULL, dev, NULL, "dm365_af");

	/* Set up the Interrupt handler for H3AINT interrupt */
	result = request_irq(3, af_isr, IRQF_SHARED, "dm365_h3a_af",
			     (void *)af_dev_configptr);

	if (result != 0) {
		printk(KERN_ERR "Error : Request IRQ Failed");
		unregister_chrdev_region(dev, AF_NR_DEVS);
		kfree(af_dev_configptr);
		device_destroy(af_class, dev);
		class_destroy(af_class);
		driver_unregister(&af_driver);
		platform_device_unregister(&afdevice);
		cdev_del(&c_dev);
		return result;
	}

	/* Initialize device structure */
	memset((unsigned char *)af_dev_configptr, 0, sizeof(struct af_device));

	af_dev_configptr->in_use = AF_NOT_IN_USE;
	af_dev_configptr->buffer_filled = 0;
	printk(KERN_ERR "AF Driver initialized\n");
	return 0;
}

/*
 * This function is called by the kernel while unloading the driver.
 * It will unregister character device driver
 */
void __exit af_cleanup(void)
{
	/* Return if driver is busy */
	if (af_dev_configptr->in_use == AF_IN_USE) {
		printk(KERN_ERR "Error : Driver in use. Can't remove.");
		return;
	}
	free_irq(3, af_dev_configptr);
	/* Free device structure */
	kfree(af_dev_configptr);

	unregister_chrdev_region(dev, AF_NR_DEVS);

	driver_unregister(&af_driver);

	device_destroy(af_class, dev);

	class_destroy(af_class);

	platform_device_unregister(&afdevice);
	/* unregistering the driver from the kernel */
	cdev_del(&c_dev);

}

module_init(af_init)
module_exit(af_cleanup)
/* Module License */
MODULE_LICENSE("GPL");
