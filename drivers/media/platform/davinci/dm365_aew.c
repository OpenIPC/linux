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

/* Global structure */
static struct class *aew_class;
struct aew_device *aew_dev_configptr;
struct device *aewdev;
/* device structure to make entry in device*/
static dev_t dev;

/* For registeration of charatcer device*/
static struct cdev c_dev;

int aew_validate_parameters(void)
{
	int result = 0;

	/* Check horizontal Count */
	if ((aew_dev_configptr->config->window_config.hz_cnt <
	     AEW_WINDOW_HORIZONTAL_COUNT_MIN)
	    || (aew_dev_configptr->config->window_config.hz_cnt >
		AEW_WINDOW_HORIZONTAL_COUNT_MAX)) {
		dev_err(aewdev, "\n Horizontal Count is incorrect");
		result = -EINVAL;
	}
	/* Check Vertical Count */
	if ((aew_dev_configptr->config->window_config.vt_cnt <
	     AEW_WINDOW_VERTICAL_COUNT_MIN)
	    || (aew_dev_configptr->config->window_config.vt_cnt >
		AEW_WINDOW_VERTICAL_COUNT_MAX)) {
		dev_err(aewdev, "\n Vertical Count is incorrect");
		result = -EINVAL;
	}
	/* Check line increment */
	if ((NOT_EVEN ==
	     CHECK_EVEN(aew_dev_configptr->config->window_config.
			    hz_line_incr))
	    || (aew_dev_configptr->config->window_config.hz_line_incr <
		AEW_HZ_LINEINCR_MIN)
	    || (aew_dev_configptr->config->window_config.hz_line_incr >
		AEW_HZ_LINEINCR_MAX)) {
		dev_err(aewdev, "\n Invalid Parameters");
		dev_err(aewdev, "\n Horizontal Line Increment is incorrect");
		result = -EINVAL;
	}
	/* Check line increment */
	if ((NOT_EVEN ==
	     CHECK_EVEN(aew_dev_configptr->config->window_config.
			    vt_line_incr))
	    || (aew_dev_configptr->config->window_config.vt_line_incr <
		AEW_VT_LINEINCR_MIN)
	    || (aew_dev_configptr->config->window_config.vt_line_incr >
		AEW_VT_LINEINCR_MAX)) {
		dev_err(aewdev, "\n Invalid Parameters");
		dev_err(aewdev, "\n Vertical Line Increment is incorrect");
		result = -EINVAL;
	}
	/* Check width */
	if ((NOT_EVEN ==
	     CHECK_EVEN(aew_dev_configptr->config->window_config.width))
	    || (aew_dev_configptr->config->window_config.width <
		AEW_WIDTH_MIN)
	    || (aew_dev_configptr->config->window_config.width >
		AEW_WIDTH_MAX)) {
		dev_err(aewdev, "\n Width is incorrect");

		result = -EINVAL;
	}
	/* Check Height */
	if ((NOT_EVEN ==
	     CHECK_EVEN(aew_dev_configptr->config->window_config.height))
	    || (aew_dev_configptr->config->window_config.height <
		AEW_HEIGHT_MIN)
	    || (aew_dev_configptr->config->window_config.height >
		AEW_HEIGHT_MAX)) {
		dev_err(aewdev, "\n height incorrect");
		result = -EINVAL;
	}
	/* Check Horizontal Start */
	if ((aew_dev_configptr->config->window_config.hz_start <
	     AEW_HZSTART_MIN)
	    || (aew_dev_configptr->config->window_config.hz_start >
		AEW_HZSTART_MAX)) {
		dev_err(aewdev, "\n horizontal start is  incorrect");
		result = -EINVAL;
	}
	if ((aew_dev_configptr->config->window_config.vt_start >
	     AEW_VTSTART_MAX)) {
		dev_err(aewdev, "\n Vertical start is  incorrect");
		result = -EINVAL;
	}
	if ((aew_dev_configptr->config->alaw_enable > H3A_AEW_ENABLE)
	    || (aew_dev_configptr->config->alaw_enable < H3A_AEW_DISABLE)) {
		dev_err(aewdev, "\n A Law setting is incorrect");
		result = -EINVAL;
	}
	if (aew_dev_configptr->config->saturation_limit > AEW_AVELMT_MAX) {
		dev_err(aewdev, "\n Saturation Limit is incorrect");
		result = -EINVAL;
	}
	/* Check Black Window Height */
	if (NOT_EVEN ==
	    CHECK_EVEN(aew_dev_configptr->config->blackwindow_config.height)
	    || (aew_dev_configptr->config->blackwindow_config.height <
		AEW_BLKWINHEIGHT_MIN)
	    || (aew_dev_configptr->config->blackwindow_config.height >
		AEW_BLKWINHEIGHT_MAX)) {
		dev_err(aewdev, "\n Black Window height incorrect");
		result = -EINVAL;
	}
	/* Check Black Window Height */
	if ((NOT_EVEN ==
	     CHECK_EVEN(aew_dev_configptr->config->blackwindow_config.
			    height))
	    || (aew_dev_configptr->config->blackwindow_config.vt_start <
		AEW_BLKWINVTSTART_MIN)
	    || (aew_dev_configptr->config->blackwindow_config.vt_start >
		AEW_BLKWINVTSTART_MAX)) {
		dev_err(aewdev, "\n Black Window vertical Start is incorrect");
		result = -EINVAL;
	}

	if (aew_dev_configptr->config->out_format < AEW_OUT_SUM_OF_SQUARES ||
	    aew_dev_configptr->config->out_format > AEW_OUT_SUM_ONLY) {
		dev_err(aewdev, "\n Invalid out_format");
		result = -EINVAL;
	}

	if (aew_dev_configptr->config->sum_shift > AEW_SUMSHIFT_MAX) {
		dev_err(aewdev, "\n sum_shift param is invalid, max = %d",
			AEW_SUMSHIFT_MAX);
		result = -EINVAL;
	}

	return result;
}

/* inline function to free reserver pages  */
inline void aew_free_pages(unsigned long addr, unsigned long bufsize)
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

/* Function to perform hardware Configuration */
int aew_hardware_setup(void)
{
	int result;
	/* Size for buffer in bytes */
	int buff_size = 0;
	unsigned long adr;
	unsigned long size;
	unsigned int busyaew;
	int window_count;
	/* Get the value of PCR register */
	busyaew = aew_get_hw_state();

	/* If H3A Engine is busy then return */
	if (busyaew == 1) {
		dev_err(aewdev, "\n Error : AEW Engine is busy");
		return -EBUSY;
	}

	result = aew_validate_parameters();
	dev_dbg(aewdev, "Result =  %d\n", result);
	if (result < 0) {
		dev_err(aewdev, "Error : Parameters are incorrect \n");
		return result;
	}

	/* Deallocate the previously allocated buffers */
	if (aew_dev_configptr->buff_old)
		aew_free_pages((unsigned long)aew_dev_configptr->buff_old,
			       aew_dev_configptr->size_window);

	if (aew_dev_configptr->buff_curr)
		aew_free_pages((unsigned long)aew_dev_configptr->
			       buff_curr, aew_dev_configptr->size_window);

	if (aew_dev_configptr->buff_app)
		aew_free_pages((unsigned long)aew_dev_configptr->
			       buff_app, aew_dev_configptr->size_window);

	/*
	 * Allocat the buffers as per the new buffer size
	 * Allocate memory for old buffer
	 */
	/*if (aew_dev_configptr->config->out_format == AEW_OUT_SUM_ONLY)
		buff_size = (aew_dev_configptr->config->window_config.hz_cnt) *
			    (aew_dev_configptr->config->window_config.vt_cnt) *
				AEW_WINDOW_SIZE_SUM_ONLY;
	else
		buff_size = (aew_dev_configptr->config->window_config.hz_cnt) *
			    (aew_dev_configptr->config->window_config.vt_cnt) *
				AEW_WINDOW_SIZE;
	*/

	window_count= (aew_dev_configptr->config->window_config.hz_cnt) *
				  (aew_dev_configptr->config->window_config.vt_cnt + 1);
	if (aew_dev_configptr->config->out_format == AEW_OUT_SUM_ONLY){
		int w=0, cb=0, ec=0, ext=0, win8=0, hor_cont=0;
		while (w < (window_count)) {
			if (win8 == 8){
				cb++;
				win8 = 0;
				ext++;
			} else {
				if (hor_cont == aew_dev_configptr->config->window_config.hz_cnt){
					if (((ext) % 2) != 0){
						ec++;
					}
					ext=0;
					hor_cont=0;
				} else{
					w++;
					win8++;
					hor_cont++;
					ext++;
				}
			}
		}
		buff_size = (cb + 1 + ec + window_count)* AEW_WINDOW_SIZE_SUM_ONLY;

	}else
		buff_size = ((aew_dev_configptr->config->window_config.hz_cnt) *
			    (aew_dev_configptr->config->window_config.vt_cnt + 1) +
				(aew_dev_configptr->config->window_config.hz_cnt) *
			    (aew_dev_configptr->config->window_config.vt_cnt + 1) *
				1/8 )*AEW_WINDOW_SIZE;

	aew_dev_configptr->buff_old =
	    (void *)__get_free_pages(GFP_KERNEL | GFP_DMA,
				     get_order(buff_size));

	if (aew_dev_configptr->buff_old == NULL)
		return -ENOMEM;

	/* Make pges reserved so that they will be swapped out */
	adr = (unsigned long)aew_dev_configptr->buff_old;
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
	aew_dev_configptr->buff_curr =
	    (void *)__get_free_pages(GFP_KERNEL | GFP_DMA,
				     get_order(buff_size));

	if (aew_dev_configptr->buff_curr == NULL) {

		/*Free all  buffer that are allocated */
		if (aew_dev_configptr->buff_old)
			aew_free_pages((unsigned long)aew_dev_configptr->
				       buff_old, buff_size);
		return -ENOMEM;
	}

	/* Make pges reserved so that they will be swapped out */
	adr = (unsigned long)aew_dev_configptr->buff_curr;
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

	/* Allocate memory for application buffer */
	aew_dev_configptr->buff_app =
	    (void *)__get_free_pages(GFP_KERNEL | GFP_DMA,
				     get_order(buff_size));

	if (aew_dev_configptr->buff_app == NULL) {
		/* Free all  buffer that were allocated previously */
		if (aew_dev_configptr->buff_old)
			aew_free_pages((unsigned long)aew_dev_configptr->
				       buff_old, buff_size);
		if (aew_dev_configptr->buff_curr)
			aew_free_pages((unsigned long)aew_dev_configptr->
				       buff_curr, buff_size);
		return -ENOMEM;
	}

	/* Make pages reserved so that they will be swapped out */
	adr = (unsigned long)aew_dev_configptr->buff_app;
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

	/* Set the registers */
	aew_register_setup(aewdev, aew_dev_configptr);
	aew_dev_configptr->size_window = buff_size;
	aew_dev_configptr->aew_config = H3A_AEW_CONFIG;

	return 0;
}

/* This Function is called when driver is opened */
static int aew_open(struct inode *inode, struct file *filp)
{
	/* Return if Device is in use (Single Channel Support is provided) */
	if (aew_dev_configptr->in_use == AEW_IN_USE)
		return -EBUSY;

	/* Set the aew_dev_configptr structure */
	aew_dev_configptr->config = NULL;

	/* Allocate memory for configuration  structure of this channel */
	aew_dev_configptr->config = (struct aew_configuration *)
	    kmalloc(sizeof(struct aew_configuration), GFP_KERNEL);

	if (aew_dev_configptr->config == NULL) {
		dev_err(aewdev, "Error : Kmalloc fail\n");
		return -ENOMEM;
	}

	/* Initiaze the wait queue */
	init_waitqueue_head(&(aew_dev_configptr->aew_wait_queue));

	/* Device is in use */
	aew_dev_configptr->in_use = AEW_IN_USE;

	/* No Hardware Set up done */
	aew_dev_configptr->aew_config = H3A_AEW_CONFIG_NOT_DONE;

	/* No statistics are available */
	aew_dev_configptr->buffer_filled = 0;

	/* Set Window Size to 0 */
	aew_dev_configptr->size_window = 0;

	/* Initialize the mutex */
	mutex_init(&(aew_dev_configptr->read_blocked));

	return 0;
}

static void aew_platform_release(struct device *device)
{
	/* This is called when the reference count goes to zero */
}

static int aew_probe(struct device *device)
{
	aewdev = device;
	return 0;
}

static int aew_remove(struct device *device)
{
	return 0;
}

/* This Function is called when driver is closed */
static int aew_release(struct inode *inode, struct file *filp)
{
	aew_engine_setup(aewdev, 0);
	/* The Application has closed device so device is not in use */
	aew_dev_configptr->in_use = AEW_NOT_IN_USE;

	/* Release memory for configuration structure of this channel */
	kfree(aew_dev_configptr->config);

	/* Free Old Buffer */
	if (aew_dev_configptr->buff_old)
		aew_free_pages((unsigned long)aew_dev_configptr->buff_old,
			       aew_dev_configptr->size_window);

	/* Free Current Buffer */
	if (aew_dev_configptr->buff_curr)
		aew_free_pages((unsigned long)aew_dev_configptr->
			       buff_curr, aew_dev_configptr->size_window);

	/* Free Application Buffer */
	if (aew_dev_configptr->buff_app)
		aew_free_pages((unsigned long)aew_dev_configptr->buff_app,
			       aew_dev_configptr->size_window);

	aew_dev_configptr->buff_old = NULL;
	aew_dev_configptr->buff_curr = NULL;
	aew_dev_configptr->config = NULL;
	aew_dev_configptr->buff_app = NULL;

	return 0;
}

/*
 * This function will process IOCTL commands sent by the application and
 * control the devices IO operations.
 */
static long aew_ioctl(struct file *filep,
		     unsigned int cmd, unsigned long arg)
{
	/* Stores Previous Configurations */
	struct aew_configuration aewconfig = *(aew_dev_configptr->config);
	int result = 0;

	/* Decrement the semaphore */
	result = mutex_lock_interruptible(&aew_dev_configptr->read_blocked);
	if (result)
		return result;

	/*
	 * Extract the type and number bitfields, and don't decode wrong cmds:
	 * verify the magic number
	 */
	if (_IOC_TYPE(cmd) != AEW_MAGIC_NO) {
		mutex_unlock(&aew_dev_configptr->read_blocked);
		return -ENOTTY;
	}

	/* verify the command number */
	if (_IOC_NR(cmd) > AEW_IOC_MAXNR) {
		/* Release mutex in case of fault */
		mutex_unlock(&aew_dev_configptr->read_blocked);
		return -ENOTTY;
	}

	/* check for the permission of the operation */
	if (_IOC_DIR(cmd) & _IOC_READ)
		result =
		    !access_ok(VERIFY_WRITE, (void __user *)arg,
			       _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		result =
		    !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (result) {
		/* Release mutex in case of fault */
		mutex_unlock(&aew_dev_configptr->read_blocked);
		return -EFAULT;
	}

	/* Switch according to IOCTL command */
	switch (cmd) {
		/*
		 * This ioctl is used to perform hardware set up
		 * and will set all the registers for AF engine
		 */
	case AEW_S_PARAM:
		/* Copy config structure passed by user */
		if (copy_from_user(aew_dev_configptr->config,
				   (struct aew_configuration *)arg,
				   sizeof(struct aew_configuration))) {
			*(aew_dev_configptr->config) = aewconfig;
			mutex_unlock(&aew_dev_configptr->read_blocked);
			return -EFAULT;
		}
		/* Call aew_hardware_setup to perform register configuration */
		result = aew_hardware_setup();
		if (!result) {
			/*
			 * Hardware Set up is successful
			 * Return the no of bytes required for buffer
			 */
			result = aew_dev_configptr->size_window;
		} else {
			/* Change Configuration Structure to original */
			*(aew_dev_configptr->config) = aewconfig;
			dev_err(aewdev, "Error : AEW_S_PARAM  failed\n");
		}
		break;

		/* This ioctl is used to return parameters in user space */
	case AEW_G_PARAM:
		if (aew_dev_configptr->aew_config == H3A_AEW_CONFIG) {
			if (copy_to_user
			    ((struct aew_configuration *)arg,
			     aew_dev_configptr->config,
			     sizeof(struct aew_configuration))) {
				mutex_unlock(&aew_dev_configptr->read_blocked);
				return -EFAULT;
			} else
				result = aew_dev_configptr->size_window;
		} else {
			dev_err(aewdev,
				"Error : AEW Hardware is not configured.\n");
			result = -EINVAL;
		}
		break;

		/* This ioctl is used to enable AEW Engine */
	case AEW_ENABLE:
		/* Enable AEW Engine if Hardware set up is done */
		if (aew_dev_configptr->aew_config == H3A_AEW_CONFIG_NOT_DONE) {
			dev_err(aewdev,
				"Error : AEW Hardware is not configured.\n");
			result = -EINVAL;
		} else{
			/* Enable AF Engine */
			aew_engine_setup(aewdev, 1);
		}break;

		/* This ioctl is used to disable AEW Engine */
	case AEW_DISABLE:
		/* Disable AEW Engine */
		aew_engine_setup(aewdev, 0);
		break;

		/* Invalid Command */
	default:
		dev_err(aewdev, "Error: It should not come here!!\n");
		result = -ENOTTY;
		break;
	}

	/* Release the mutex */
	mutex_unlock(&aew_dev_configptr->read_blocked);

	return result;
}

/* This function will return statistics to user */
static ssize_t aew_read(struct file *filep, char *kbuff,
			size_t size, loff_t *offset)
{
	void *buffer_temp;
	int result = 0;
	int ret;

	/* Semaphore will return immediately if read call is busy */
	ret = mutex_trylock(&(aew_dev_configptr->read_blocked));
	if (!ret) {
		dev_dbg(aewdev, "Read Call : busy  : %d\n", ret);
		return -EBUSY;
	}
	/* First Check the size given by user */
	if (size < aew_dev_configptr->size_window) {
		/*
		 * Return Failure to applicaiton
		 * if size is less than required size
		 */
		dev_dbg(aewdev, "Error : Invalid size of buffer\n");
		mutex_unlock(&(aew_dev_configptr->read_blocked));
		return -EINVAL;
	}

	/*
	 * The value of buffer_filled flag determines
	 * the status of statistics
	 */
	if (aew_dev_configptr->buffer_filled == 0) {
		/* Decrement the semaphore */
		dev_dbg(aewdev, "READ CALL IS BLOCKED............\n");
		/* Block the read call */
		wait_event_interruptible_timeout(aew_dev_configptr->
						 aew_wait_queue,
						 aew_dev_configptr->
						 buffer_filled, AEW_TIMEOUT);
		dev_dbg(aewdev, "Read Call is unbloked and waking up.......\n");
		dev_dbg(aewdev, "Buffer Filled.... %d\n",
			aew_dev_configptr->buffer_filled);
	}

	if (aew_dev_configptr->buffer_filled == 1) {
		/* Disable the interrupts and then swap the buffers */
		dev_dbg(aewdev, "READING............\n");
		disable_irq(6);
		/* New Statistics are availaible */
		aew_dev_configptr->buffer_filled = 0;

		/* Swap application buffer and old buffer */
		buffer_temp = aew_dev_configptr->buff_old;
		aew_dev_configptr->buff_old = aew_dev_configptr->buff_app;
		aew_dev_configptr->buff_app = buffer_temp;

		/* Interrupts are enabled */
		enable_irq(6);
		/*
		 * Copy the entire statistics located in application
		 * buffer to user space
		 */
		if (copy_to_user(kbuff, aew_dev_configptr->buff_app,
				 aew_dev_configptr->size_window)) {
			dev_err(aewdev, "Error : Read Fault\n");
			mutex_unlock(&(aew_dev_configptr->read_blocked));
			return -EFAULT;
		} else{
			result = aew_dev_configptr->size_window;
		}
		dev_dbg(aewdev, "Reading Done........................\n");
	}

	dev_dbg(aewdev, "APP BUFF VALUE %x\n",
		(*((unsigned int *)(aew_dev_configptr->buff_app))));

	/* release the mutex */
	mutex_unlock(&(aew_dev_configptr->read_blocked));

	return result;
}

/* This function will handle interrupt generated by H3A Engine. */
static irqreturn_t aew_isr(int irq, void *dev_id)
{
	/* EN AF Bit */
	unsigned int enaew;
	/* Temporary Buffer for Swapping */
	void *buffer_temp;

	/* Get the value of PCR register */
	enaew = aew_get_enable();

	/* If AEW engine is not enabled, interrupt is not for AEW */
	if (!enaew)
		return IRQ_RETVAL(IRQ_NONE);

	/*
	 * Interrupt is generated by AEW, so Service the Interrupt
	 * Swap current buffer and old buffer
	 */
	if (aew_dev_configptr) {
		buffer_temp = aew_dev_configptr->buff_curr;
		aew_dev_configptr->buff_curr = aew_dev_configptr->buff_old;
		aew_dev_configptr->buff_old = buffer_temp;

		/* Set the AEWBUFSTAT REgister to current buffer Address */
		aew_set_address(aewdev, (unsigned
			 long)(virt_to_phys(aew_dev_configptr->buff_curr)));

		/*
		 * Set buffer filled flag to indicate statistics are available
		 */
		aew_dev_configptr->buffer_filled = 1;

		/* new statistics are available. Wake up the read call */
		wake_up(&(aew_dev_configptr->aew_wait_queue));

		return IRQ_RETVAL(IRQ_HANDLED);
	}
	return IRQ_RETVAL(IRQ_NONE);
}

/* file Operation Structure*/
static const struct file_operations aew_fops = {
	.owner = THIS_MODULE,
	.open = aew_open,
	.read = aew_read,
	.unlocked_ioctl = aew_ioctl,
	.release = aew_release,
};
static struct platform_device aewdevice = {
	.name = "dm365_aew",
	.id = 2,
	.dev = {
		.release = aew_platform_release,
		}
};

static struct device_driver aew_driver = {
	.name = "dm365_aew",
	.bus = &platform_bus_type,
	.probe = aew_probe,
	.remove = aew_remove,
};

#define DRIVERNAME  "DM365AEW"
/* Function to register the AF character device driver. */
int __init aew_init(void)
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
	printk(KERN_INFO "aew major#: %d, minor# %d\n", MAJOR(dev), MINOR(dev));
	/* allocate memory for device structure and initialize it with 0 */
	aew_dev_configptr =
	    kmalloc(sizeof(struct aew_device), GFP_KERNEL);
	if (!aew_dev_configptr) {
		printk(KERN_ERR "Error : kmalloc fail");
		unregister_chrdev_region(dev, AEW_NR_DEVS);
		return -ENOMEM;

	}

	/* Initialize character device */
	cdev_init(&c_dev, &aew_fops);
	c_dev.owner = THIS_MODULE;
	c_dev.ops = &aew_fops;
	err = cdev_add(&c_dev, dev, 1);
	if (err) {
		printk(KERN_ERR "Error : Error in  Adding Davinci AEW");
		unregister_chrdev_region(dev, AEW_NR_DEVS);
		kfree(aew_dev_configptr);
		return -err;
	}
	/* register driver as a platform driver */
	if (driver_register(&aew_driver) != 0) {
		unregister_chrdev_region(dev, 1);
		cdev_del(&c_dev);
		return -EINVAL;
	}

	/* Register the drive as a platform device */
	if (platform_device_register(&aewdevice) != 0) {
		driver_unregister(&aew_driver);
		unregister_chrdev_region(dev, 1);
		cdev_del(&c_dev);
		return -EINVAL;
	}

	aew_class = class_create(THIS_MODULE, "dm365_aew");
	if (!aew_class) {
		printk(KERN_ERR "aew_init: error in creating device class\n");
		driver_unregister(&aew_driver);
		platform_device_unregister(&aewdevice);
		unregister_chrdev_region(dev, 1);
		unregister_chrdev(MAJOR(dev), DRIVERNAME);
		cdev_del(&c_dev);
		return -EINVAL;
	}

	device_create(aew_class, NULL, dev, NULL, "dm365_aew");

	/* Set up the Interrupt handler for H3AINT interrupt */
	result = request_irq(6, aew_isr, IRQF_SHARED, "dm365_h3a_aew",
			     (void *)aew_dev_configptr);

	if (result != 0) {
		printk(KERN_ERR "Error : Request IRQ Failed");
		unregister_chrdev_region(dev, AEW_NR_DEVS);
		device_destroy(aew_class, dev);
		class_destroy(aew_class);
		kfree(aew_dev_configptr);
		driver_unregister(&aew_driver);
		platform_device_unregister(&aewdevice);
		cdev_del(&c_dev);
		return result;
	}

	/* Initialize device structure */
	memset(aew_dev_configptr, 0, sizeof(struct aew_device));

	aew_dev_configptr->in_use = AEW_NOT_IN_USE;
	aew_dev_configptr->buffer_filled = 0;
	printk(KERN_NOTICE "AEW Driver initialized\n");
	return 0;
}

/*
 * This Function is called by the kernel while unloading the driver
 * This will unregister the Character Device Driver
 */
void __exit aew_cleanup(void)
{
	/* Device is in use */
	if (aew_dev_configptr->in_use == AEW_IN_USE) {
		printk(KERN_ERR "Error : Driver in use");
		return;
	}

	free_irq(6, aew_dev_configptr);
	/* Free device structure */
	kfree(aew_dev_configptr);
	aew_dev_configptr = NULL;
	unregister_chrdev_region(dev, AEW_NR_DEVS);

	driver_unregister(&aew_driver);

	device_destroy(aew_class, dev);

	class_destroy(aew_class);

	platform_device_unregister(&aewdevice);

	cdev_del(&c_dev);

	/* unregistering the driver from the kernel */
	unregister_chrdev(MAJOR(dev), DRIVERNAME);
}

module_init(aew_init)
module_exit(aew_cleanup)
MODULE_LICENSE("GPL");
