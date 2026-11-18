/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/kmod.h>
#include <linux/fs.h>

#include <linux/wait.h>

#include "linux/platform_device.h"
#include "linux/device.h"
#include "osal_ax.h"
#include "osal_dev_ax.h"
#include "axdev.h"
#include "axdev_log.h"
#include "osal_lib_ax.h"
#define DRVAL_DEBUG 0

static DEFINE_MUTEX(ax_dev_sem);

#define GET_FILE(file) do\
{\
    if (__get_file(file) < 0)\
        return -1;\
}while(0)

#define PUT_FILE(file) do\
{\
    if (__put_file(file) < 0)\
        return -1;\
}while(0)

typedef struct osal_axera_dev {
	struct AX_DEV osal_dev;
	struct axdev_device axdev_dev;
} osal_axera_dev_t;

spinlock_t f_lock;

void osal_device_init(void)
{
	spin_lock_init(&f_lock);
}

static int __get_file(struct file *file)
{
	AX_DEV_PRIVATE_DATA_T *pdata;

	spin_lock(&f_lock);
	pdata = (AX_DEV_PRIVATE_DATA_T *) (file->private_data);
	if (pdata == NULL) {
		spin_unlock(&f_lock);
		return -1;
	}

	pdata->f_ref_cnt++;
	spin_unlock(&f_lock);

	return 0;
}

static int __put_file(struct file *file)
{
	AX_DEV_PRIVATE_DATA_T *pdata;

	spin_lock(&f_lock);
	pdata = file->private_data;
	if (pdata == NULL) {
		spin_unlock(&f_lock);
		return -1;
	}

	pdata->f_ref_cnt--;
	spin_unlock(&f_lock);

	return 0;
}

static int osal_open(struct inode *inode, struct file *file)
{
	struct axdev_device *axdev;
	osal_axera_dev_t *axera_dev;
	AX_DEV_PRIVATE_DATA_T *pdata;
	unsigned minor = iminor(inode);
	unsigned major = imajor(inode);

	pdata = NULL;
	/*
	   if (!capable(CAP_SYS_RAWIO) || !capable(CAP_SYS_ADMIN))
	   return -EPERM;
	 */

	AXDEV_LOG_DEBUG(" OSAL OPEN (major = %u)(minor = %u). \n", major, minor);

	//axdev = getaxdev(inode);
	axdev = (struct axdev_device *)file->private_data;
	if (axdev == NULL) {
		printk("%s - get axdev device error!\n", __func__);
		return -1;
	}
	axera_dev = osal_container_of(axdev, struct osal_axera_dev, axdev_dev);
	pdata = (AX_DEV_PRIVATE_DATA_T *) kmalloc(sizeof(AX_DEV_PRIVATE_DATA_T), GFP_KERNEL);
	if (pdata == NULL) {
		printk("%s - kmalloc error!\n", __func__);
		return -1;
	}

	memset(pdata, 0, sizeof(AX_DEV_PRIVATE_DATA_T));

	file->private_data = pdata;
	pdata->dev = &(axera_dev->osal_dev);
	pdata->f_flags = file->f_flags;
	pdata->file = file;

	//pdata->data = __waitpoll_create()
	/*
	   if (axera_dev->osal_dev.fops->open != NULL)
	   return axera_dev->osal_dev.fops->open((void *) & (pdata->data));
	 */
	if (axera_dev->osal_dev.fops->open != NULL)
		return axera_dev->osal_dev.fops->open((void *)pdata);

	return 0;
}

static ssize_t osal_read(struct file *file, char __user * buf, size_t size, loff_t * offset)
{
	AX_DEV_PRIVATE_DATA_T *pdata = file->private_data;
	int ret = 0;

	GET_FILE(file);

	if (pdata->dev->fops->read != NULL) {
		ret = pdata->dev->fops->read(buf, (int)size, (long *)offset, (void *)pdata);
	}

	PUT_FILE(file);
	return ret;
}

static ssize_t osal_write(struct file *file, const char __user * buf, size_t size, loff_t * offset)
{
	AX_DEV_PRIVATE_DATA_T *pdata = file->private_data;
	int ret = 0;

	GET_FILE(file);
	if (pdata->dev->fops->write != NULL) {
		ret = pdata->dev->fops->write(buf, (int)size, (long *)offset, (void *)pdata);
	}
	PUT_FILE(file);
	return ret;
}

static loff_t osal_llseek(struct file *file, loff_t offset, int whence)
{
	AX_DEV_PRIVATE_DATA_T *pdata = file->private_data;
	int ret = 0;

	GET_FILE(file);
	if (DRVAL_DEBUG)
		printk("%s - file->private_data=%p!\n", __func__, pdata);

	if (whence == SEEK_SET) {
		if (pdata->dev->fops->llseek != NULL) {
			ret = pdata->dev->fops->llseek((long)offset, AX_OSAL_SEEK_SET, (void *)pdata);
		}
	} else if (whence == SEEK_CUR) {
		if (pdata->dev->fops->llseek != NULL) {
			ret = pdata->dev->fops->llseek((long)offset, AX_OSAL_SEEK_CUR, (void *)pdata);
		}
	} else if (whence == SEEK_END) {
		if (pdata->dev->fops->llseek != NULL) {
			ret = pdata->dev->fops->llseek((long)offset, AX_OSAL_SEEK_END, (void *)pdata);
		}
	}

	PUT_FILE(file);
	return (loff_t) ret;
}

static int osal_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	AX_DEV_PRIVATE_DATA_T *pdata = file->private_data;

	GET_FILE(file);

	if (DRVAL_DEBUG)
		printk("%s - file->private_data=%p!\n", __func__, pdata);

	if ((pdata == NULL) || (pdata->dev == NULL) || (pdata->dev->fops == NULL)) {
		printk("WARNING !!!! MEMORY LEAK. you are woring to invoke AX_OSAL_DEV_destroydev early  ,Func:%s - \n",__func__);
		return 0;
	}

	if (pdata->dev->fops->release != NULL)
		ret = pdata->dev->fops->release((void *)pdata);

	if (ret != 0) {
		PUT_FILE(file);
		printk("%s - release failed!\n", __func__);
		return ret;
	}

	PUT_FILE(file);
	spin_lock(&f_lock);
	if (pdata->f_ref_cnt != 0) {
		printk("%s - release failed!\n", __func__);
		spin_unlock(&f_lock);
		return -1;
	}
	kfree(file->private_data);
	file->private_data = NULL;
	spin_unlock(&f_lock);

	return 0;
}

static long __osal_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	AX_DEV_PRIVATE_DATA_T *pdata = file->private_data;

	if (DRVAL_DEBUG) {
		printk("%s - file->private_data=%p, cmd = %u!\n", __func__, pdata, cmd);
	}

	if (pdata->dev->fops->unlocked_ioctl != NULL) {
		ret = pdata->dev->fops->unlocked_ioctl(cmd, arg, (void *)pdata);
	}

	return ret;
}

static long osal_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	GET_FILE(file);

	ret = __osal_unlocked_ioctl(file, cmd, arg);
	PUT_FILE(file);

	return ret;
}

static unsigned int osal_poll(struct file *file, struct poll_table_struct *table)
{
	AX_DEV_PRIVATE_DATA_T *pdata = file->private_data;
	struct AX_POLL t;
	unsigned int ret = 0;

	GET_FILE(file);

	if (DRVAL_DEBUG)
		printk("%s - table=%p, file=%p!\n", __func__, table, file);
	t.poll_table = table;
	t.data = file;
	/*device fw, enhance to add API of poll_wait */
	t.wait = &(pdata->dev->dev_wait);
	/*
	   if (pdata->dev->fops->poll != NULL)
	   ret = pdata->dev->fops->poll(&t, (void *) & (pdata->data));
	 */
	if (pdata->dev->fops->poll != NULL)
		ret = pdata->dev->fops->poll(&t, (void *)pdata);

	PUT_FILE(file);
	return ret;
}

static int osal_mmap(struct file *file, struct vm_area_struct *vm)
{
	struct AX_VM osal_vm;
	AX_DEV_PRIVATE_DATA_T *pdata = file->private_data;
	osal_vm.vm = vm;

	if (DRVAL_DEBUG)
		printk("%s - start=%lx, end=%lx!, off=%lx\n", __func__, vm->vm_start, vm->vm_end,
			    vm->vm_pgoff);

	if (pdata->dev->fops->mmap != NULL)
		return pdata->dev->fops->mmap(&osal_vm, vm->vm_start, vm->vm_end, vm->vm_pgoff, (void *)pdata);

	return 0;
}

static struct file_operations s_osal_fops = {
	.owner = THIS_MODULE,
	.open = osal_open,
	.read = osal_read,
	.write = osal_write,
	.llseek = osal_llseek,
	.unlocked_ioctl = osal_unlocked_ioctl,
	.release = osal_release,
	.poll = osal_poll,
	.mmap = osal_mmap,
};

static int osal_pm_prepare(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_prepare)
		return axera_dev->osal_dev.osal_pmops->pm_prepare(&(axera_dev->osal_dev));
	return 0;
}

static void osal_pm_complete(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_complete)
		axera_dev->osal_dev.osal_pmops->pm_complete(&(axera_dev->osal_dev));
}

static int osal_pm_suspend(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_suspend)
		return axera_dev->osal_dev.osal_pmops->pm_suspend(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_resume(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_resume)
		return axera_dev->osal_dev.osal_pmops->pm_resume(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_freeze(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_freeze)
		return axera_dev->osal_dev.osal_pmops->pm_freeze(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_thaw(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_thaw)
		return axera_dev->osal_dev.osal_pmops->pm_thaw(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_poweroff(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_poweroff)
		return axera_dev->osal_dev.osal_pmops->pm_poweroff(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_restore(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_restore)
		return axera_dev->osal_dev.osal_pmops->pm_restore(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_suspend_late(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_suspend_late)
		return axera_dev->osal_dev.osal_pmops->pm_suspend_late(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_resume_early(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_resume_early)
		return axera_dev->osal_dev.osal_pmops->pm_resume_early(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_freeze_late(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_freeze_late)
		return axera_dev->osal_dev.osal_pmops->pm_freeze_late(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_thaw_early(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_thaw_early)
		return axera_dev->osal_dev.osal_pmops->pm_thaw_early(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_poweroff_late(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_poweroff_late)
		return axera_dev->osal_dev.osal_pmops->pm_poweroff_late(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_restore_early(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_restore_early)
		return axera_dev->osal_dev.osal_pmops->pm_restore_early(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_suspend_noirq(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_suspend_noirq)
		return axera_dev->osal_dev.osal_pmops->pm_suspend_noirq(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_resume_noirq(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_resume_noirq)
		return axera_dev->osal_dev.osal_pmops->pm_resume_noirq(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_freeze_noirq(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_freeze_noirq)
		return axera_dev->osal_dev.osal_pmops->pm_freeze_noirq(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_thaw_noirq(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_thaw_noirq)
		return axera_dev->osal_dev.osal_pmops->pm_thaw_noirq(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_poweroff_noirq(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_poweroff_noirq)
		return axera_dev->osal_dev.osal_pmops->pm_poweroff_noirq(&(axera_dev->osal_dev));
	return 0;
}

static int osal_pm_restore_noirq(struct axdev_device *axdev)
{
	osal_axera_dev_t *axera_dev = container_of(axdev, struct osal_axera_dev, axdev_dev);
	if (axera_dev->osal_dev.osal_pmops && axera_dev->osal_dev.osal_pmops->pm_restore_noirq)
		return axera_dev->osal_dev.osal_pmops->pm_restore_noirq(&(axera_dev->osal_dev));
	return 0;
}

static struct axdev_ops s_osal_pmops = {
	.pm_prepare = osal_pm_prepare,
	.pm_complete = osal_pm_complete,
	.pm_suspend = osal_pm_suspend,
	.pm_resume = osal_pm_resume,
	.pm_freeze = osal_pm_freeze,
	.pm_thaw = osal_pm_thaw,
	.pm_poweroff = osal_pm_poweroff,
	.pm_restore = osal_pm_restore,
	.pm_suspend_late = osal_pm_suspend_late,
	.pm_resume_early = osal_pm_resume_early,
	.pm_freeze_late = osal_pm_freeze_late,
	.pm_thaw_early = osal_pm_thaw_early,
	.pm_poweroff_late = osal_pm_poweroff_late,
	.pm_restore_early = osal_pm_restore_early,
	.pm_suspend_noirq = osal_pm_suspend_noirq,
	.pm_resume_noirq = osal_pm_resume_noirq,
	.pm_freeze_noirq = osal_pm_freeze_noirq,
	.pm_thaw_noirq = osal_pm_thaw_noirq,
	.pm_poweroff_noirq = osal_pm_poweroff_noirq,
	.pm_restore_noirq = osal_pm_restore_noirq,
};

unsigned short ax_dev_minors[AXDEV_DYNAMIC_MINOR / 8];

AX_DEV_T *AX_OSAL_DEV_createdev(char * name)
{
	osal_axera_dev_t *pdev;
	if (name == NULL) {
		printk("%s - parameter invalid!\n", __func__);
		return NULL;
	}
	/*step 1/4. allocate memory for cost dev */
	pdev = (osal_axera_dev_t *) kmalloc(sizeof(osal_axera_dev_t), GFP_KERNEL);
	if (pdev == NULL) {
		printk("%s - kmalloc error!\n", __func__);
		return NULL;
	}
	memset(pdev, 0, sizeof(osal_axera_dev_t));

	/*step 2/4. get device name, and set to OSAL Device */
	strncpy(pdev->osal_dev.name, name, sizeof(pdev->osal_dev.name) - 1);

	/*step 3/4. get cost dev, and set to OSAL Device */
	pdev->osal_dev.dev = pdev;

	/*step 4/4. get cost dev, and set to OSAL Device */
	return &(pdev->osal_dev);
}

EXPORT_SYMBOL(AX_OSAL_DEV_createdev);

int AX_OSAL_DEV_destroydev(AX_DEV_T * osal_dev)
{
	osal_axera_dev_t *pdev;
	if (osal_dev == NULL) {
		pr_info("%s - parameter invalid!\n", __func__);
		return -1;
	}
	pdev = osal_dev->dev;
	if (pdev == NULL) {
		pr_info("%s - parameter invalid!\n", __func__);
		return -1;
	}
	kfree(pdev);
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_DEV_destroydev);

unsigned int AX_OSAL_DEV_get_minor(void)
{

	int i = AXDEV_DYNAMIC_MINOR - 1;
	unsigned int minor = 0;
	mutex_lock(&ax_dev_sem);
	while (--i >= 0) {
		if ((ax_dev_minors[i >> 3] & (1 << (i & 7))) == 0)
			break;
	}

	if (i < 0) {
		mutex_unlock(&ax_dev_sem);
		return -EBUSY;
	}
	minor = i;
	if (minor < AXDEV_DYNAMIC_MINOR) {
		ax_dev_minors[minor >> 3] |= 1 << (minor & 7);
	}

	mutex_unlock(&ax_dev_sem);
	return i;
}

unsigned int AX_OSAL_DEV_release_minor(unsigned int minor)
{
	if (minor >= AXDEV_DYNAMIC_MINOR) {
		return -1;
	}

	mutex_lock(&ax_dev_sem);
	ax_dev_minors[minor >> 3] &= ~(1 << (minor & 7));
	mutex_unlock(&ax_dev_sem);

	return 0;
}

static int __waitqueue_init(struct AX_WAIT *osal_wait)
{
	if (osal_wait == NULL) {
		printk("%s error osal_wait == NULL\n", __func__);
		return -1;
	}

	return AX_OSAL_SYNC_waitqueue_init(osal_wait);
}

int AX_OSAL_DEV_device_register(AX_DEV_T * osal_dev)
{
	int ret = 0;
	struct axdev_device *axdev;
	if (osal_dev == NULL || osal_dev->fops == NULL) {
		printk("%s parameter error\n", __func__);
		return -1;
	}

	if (__waitqueue_init(&osal_dev->dev_wait)) {
		printk("%s failed !!!\n", __func__);
		return -1;
	}

	/*step 1/5 get axdev-device by osal-device  */
	axdev = &(((osal_axera_dev_t *) (osal_dev->dev))->axdev_dev);

	/*step 2/5 get minor of axdev-device  */
	if (osal_dev->minor != 0)
		axdev->minor = osal_dev->minor;
	else
		axdev->minor = AXDEV_DYNAMIC_MINOR;

	/*step 3/5 initialize axdev-device */
	axdev->owner = THIS_MODULE;
	axdev->fops = &s_osal_fops;
	axdev->drvops = &s_osal_pmops;
	axdev->major = AXDEV_DEVICE_MAJOR;

	/*step 4/5 get name of axdev-device */
	strncpy(axdev->devfs_name, osal_dev->name, sizeof(axdev->devfs_name) - 1);

	/*step 5/5 register axdev-device */
	ret = axdev_register(axdev);
	if (ret) {
		printk("%s Failed to register axdev \n", __func__);
	}
	osal_dev->minor = axdev->minor;

	return ret;
}

EXPORT_SYMBOL(AX_OSAL_DEV_device_register);

void AX_OSAL_DEV_device_unregister(AX_DEV_T * osal_dev)
{
	if (osal_dev == NULL) {
		printk("%s error osal_dev == NULL\n", __func__);
		return;
	}
	AX_OSAL_SYNC_wait_destroy(&osal_dev->dev_wait);

	axdev_unregister((struct axdev_device *)&(((osal_axera_dev_t *) (osal_dev->dev))->axdev_dev));

	return;
}

EXPORT_SYMBOL(AX_OSAL_DEV_device_unregister);

void AX_OSAL_DEV_poll_wait(AX_POLL_T * table, AX_WAIT_T * wait)
{
	if (DRVAL_DEBUG)
		printk("%s - call poll_wait +!, table=%p, file=%p\n", __func__, table->poll_table,table->data);

	poll_wait((struct file *)table->data, (wait_queue_head_t *) (wait->wait), table->poll_table);

	if (DRVAL_DEBUG)
		printk("%s call poll_wait \n", __func__);

	return;
}

EXPORT_SYMBOL(AX_OSAL_DEV_poll_wait);

void AX_OSAL_DEV_pgprot_noncached(AX_VM_T * vm)
{
	struct vm_area_struct *v = (struct vm_area_struct *)(vm->vm);
	v->vm_page_prot = pgprot_writecombine(v->vm_page_prot);
	return;
}

EXPORT_SYMBOL(AX_OSAL_DEV_pgprot_noncached);

void AX_OSAL_DEV_pgprot_cached(AX_VM_T * vm)
{
	struct vm_area_struct *v = (struct vm_area_struct *)(vm->vm);

#ifdef CONFIG_ARM64
	v->vm_page_prot = __pgprot(pgprot_val(v->vm_page_prot)
				   | PTE_VALID | PTE_DIRTY | PTE_AF);
#else

	v->vm_page_prot = __pgprot(pgprot_val(v->vm_page_prot) | L_PTE_PRESENT
				   | L_PTE_YOUNG | L_PTE_DIRTY | L_PTE_MT_DEV_CACHED);
#endif

	return;
}

EXPORT_SYMBOL(AX_OSAL_DEV_pgprot_cached);

void AX_OSAL_DEV_pgprot_writecombine(AX_VM_T * vm)
{
	struct vm_area_struct *v = (struct vm_area_struct *)(vm->vm);
	v->vm_page_prot = pgprot_writecombine(v->vm_page_prot);

	return;
}

EXPORT_SYMBOL(AX_OSAL_DEV_pgprot_writecombine);

void AX_OSAL_DEV_pgprot_stronglyordered(AX_VM_T * vm)
{
	struct vm_area_struct *v = (struct vm_area_struct *)(vm->vm);

#ifdef CONFIG_ARM64
	v->vm_page_prot = pgprot_device(v->vm_page_prot);
#else
	v->vm_page_prot = pgprot_stronglyordered(v->vm_page_prot);
#endif

	return;
}

EXPORT_SYMBOL(AX_OSAL_DEV_pgprot_stronglyordered);

int AX_OSAL_DEV_remap_pfn_range(AX_VM_T * vm, unsigned long addr, unsigned long pfn, unsigned long size)
{
	struct vm_area_struct *v = (struct vm_area_struct *)(vm->vm);
	if (0 == size) {
		return -EPERM;
	}
	return remap_pfn_range(v, addr, pfn, size, v->vm_page_prot);
}

EXPORT_SYMBOL(AX_OSAL_DEV_remap_pfn_range);

int AX_OSAL_DEV_io_remap_pfn_range(AX_VM_T * vm, unsigned long addr, unsigned long pfn, unsigned long size)
{
	struct vm_area_struct *v = (struct vm_area_struct *)(vm->vm);
	v->vm_flags |= VM_IO;
	if (0 == size) {
		return -EPERM;
	}
	return io_remap_pfn_range(v, addr, pfn, size, v->vm_page_prot);
}

EXPORT_SYMBOL(AX_OSAL_DEV_io_remap_pfn_range);

void *AX_OSAL_DEV_to_dev(AX_DEV_T *ax_dev)
{
	if (ax_dev)
		return &((((osal_axera_dev_t *) (ax_dev->dev))->axdev_dev).device);
	else
		return NULL;
}

EXPORT_SYMBOL(AX_OSAL_DEV_to_dev);