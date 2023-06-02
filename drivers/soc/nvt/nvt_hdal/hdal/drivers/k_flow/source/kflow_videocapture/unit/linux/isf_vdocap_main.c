#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <asm/signal.h>
#include <kwrap/dev.h>

#include "isf_vdocap_drv.h"
#include "isf_vdocap_reg.h"
#include "isf_vdocap_main.h"
#include "isf_vdocap_proc.h"
#include "../isf_vdocap_int.h"
#include "../isf_vdocap_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id vdocap_match_table[] = {
	{   .compatible = "nvt,isf_vdocap"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int isf_vdocap_open(struct inode *inode, struct file *file);
static int isf_vdocap_release(struct inode *inode, struct file *file);
static long isf_vdocap_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int isf_vdocap_probe(struct platform_device *p_dev);
static int isf_vdocap_suspend(struct platform_device *p_dev, pm_message_t state);
static int isf_vdocap_resume(struct platform_device *p_dev);
static int isf_vdocap_remove(struct platform_device *p_dev);
int __init isf_vdocap_module_init(void);
void __exit isf_vdocap_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int isf_vdocap_open(struct inode *inode, struct file *file)
{
	ISF_VDOCAP_DRV_INFO *p_drv_info;

	p_drv_info = container_of(inode->i_cdev, ISF_VDOCAP_DRV_INFO, cdev);

	p_drv_info = container_of(inode->i_cdev, ISF_VDOCAP_DRV_INFO, cdev);
	file->private_data = p_drv_info;

	if (isf_vdocap_drv_open(&p_drv_info->module_info, MINOR(inode->i_rdev))) {
		DBG_ERR("failed to open driver\n");
		return -1;
	}

	return 0;
}

static int isf_vdocap_release(struct inode *inode, struct file *file)
{
	ISF_VDOCAP_DRV_INFO *p_drv_info;

	p_drv_info = container_of(inode->i_cdev, ISF_VDOCAP_DRV_INFO, cdev);
	isf_vdocap_drv_release(&p_drv_info->module_info, MINOR(inode->i_rdev));
	return 0;
}

static long isf_vdocap_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PISF_VDOCAP_DRV_INFO p_drv;

	inode = file_inode(filp);
	p_drv = filp->private_data;

	return isf_vdocap_drv_ioctl(MINOR(inode->i_rdev), &p_drv->module_info, cmd, arg);
}

static int isf_vdocap_mmap(struct file *file, struct vm_area_struct *vma)
{
#if 0
	ISF_VDOCAP_CTRL *p_ctrl = vdocap_get_ctrl();
#endif

	int er = 0;
#if 0
	PISF_VDOCAP_DRV_INFO p_drv;
	unsigned long pfn_start;
	unsigned long virt_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long size = vma->vm_end - vma->vm_start;

	p_drv = file->private_data;
	pfn_start = (virt_to_phys(p_ctrl->ipc.p_cfg) >> PAGE_SHIFT) + vma->vm_pgoff;
	virt_start = (unsigned long)p_ctrl->ipc.p_cfg + offset;

	DBG_IND("phy: 0x%lx, offset: 0x%lx, size: 0x%lx, vma_start=%lx\n", pfn_start << PAGE_SHIFT, offset, size, vma->vm_start);

	er = remap_pfn_range(vma, vma->vm_start, pfn_start, size, vma->vm_page_prot);
	if (er)
		DBG_ERR("%s: remap_pfn_range failed at [0x%lx  0x%lx]\n",
				__func__, vma->vm_start, vma->vm_end);
	else
		DBG_IND("%s: map 0x%lx to 0x%lx, size: 0x%lx\n", __func__, virt_start,
				vma->vm_start, size);
#endif
	return er;
}

struct file_operations isf_vdocap_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_vdocap_open,
	.release = isf_vdocap_release,
	.mmap    = isf_vdocap_mmap,
	.unlocked_ioctl = isf_vdocap_ioctl,
	.llseek  = no_llseek,
};

static int isf_vdocap_probe(struct platform_device *p_dev)
{
	ISF_VDOCAP_DRV_INFO *p_drv_info;
#if 0
	const struct of_device_id *match;
#endif
	int ret = 0;
	unsigned char ucloop;

	DBG_IND("%s\n", p_dev->name);

	p_drv_info = kzalloc(sizeof(ISF_VDOCAP_DRV_INFO), GFP_KERNEL);
	if (!p_drv_info) {
		DBG_ERR("failed to allocate memory\n");
		return -ENOMEM;
	}

	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&p_drv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		DBG_ERR("Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	DBG_IND("DevID Major:%d minor:%d\n" \
			, MAJOR(p_drv_info->dev_id), MINOR(p_drv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&p_drv_info->cdev, &isf_vdocap_fops);
	p_drv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&p_drv_info->cdev, p_drv_info->dev_id, MODULE_MINOR_COUNT)) {
		DBG_ERR("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	p_drv_info->pmodule_class = class_create(THIS_MODULE, MODULE_NAME);
	if (IS_ERR(p_drv_info->pmodule_class)) {
		DBG_ERR("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		p_drv_info->p_device[ucloop] = device_create(p_drv_info->pmodule_class, NULL
									   , MKDEV(MAJOR(p_drv_info->dev_id), (ucloop + MINOR(p_drv_info->dev_id))), NULL
									   , MODULE_NAME"%d", ucloop);

		if (IS_ERR(p_drv_info->p_device[ucloop])) {
			DBG_ERR("failed in creating device%d.\n", ucloop);
			#if (MODULE_MINOR_COUNT > 1) //CID 131215 (#1 of 1): Logically dead code (DEADCODE)
			for (; ucloop > 0 ; ucloop--) {
				device_unregister(p_drv_info->p_device[ucloop - 1]);
			}
			#endif
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	ret = isf_vdocap_proc_init(p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DEV;
	}

	ret = isf_vdocap_drv_init(&p_drv_info->module_info);

	platform_set_drvdata(p_dev, p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	isf_vdocap_proc_remove(p_drv_info);

FAIL_DEV:
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(p_drv_info->p_device[ucloop]);
	}

FAIL_CLASS:
	class_destroy(p_drv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&p_drv_info->cdev);
	vos_unregister_chrdev_region(p_drv_info->dev_id, MODULE_MINOR_COUNT);

FAIL_FREE_REMAP:

	kfree(p_drv_info);
	//coverity[UNUSED_VALUE]
	p_drv_info = NULL;
	return ret;
}

static int isf_vdocap_remove(struct platform_device *p_dev)
{
	PISF_VDOCAP_DRV_INFO p_drv_info;
	unsigned char ucloop;

	DBG_IND("\n");

	p_drv_info = platform_get_drvdata(p_dev);

	isf_vdocap_drv_remove(&p_drv_info->module_info);

	isf_vdocap_proc_remove(p_drv_info);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(p_drv_info->p_device[ucloop]);
	}

	class_destroy(p_drv_info->pmodule_class);
	cdev_del(&p_drv_info->cdev);
	vos_unregister_chrdev_region(p_drv_info->dev_id, MODULE_MINOR_COUNT);

	kfree(p_drv_info);
	//coverity[UNUSED_VALUE]
	p_drv_info = NULL;
	return 0;
}

static int isf_vdocap_suspend(struct platform_device *p_dev, pm_message_t state)
{
	PISF_VDOCAP_DRV_INFO p_drv_info;;

	DBG_IND("start\n");

	p_drv_info = platform_get_drvdata(p_dev);
	isf_vdocap_drv_suspend(&p_drv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}


static int isf_vdocap_resume(struct platform_device *p_dev)
{
	PISF_VDOCAP_DRV_INFO p_drv_info;;

	DBG_IND("start\n");

	p_drv_info = platform_get_drvdata(p_dev);
	isf_vdocap_drv_resume(&p_drv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}

static struct platform_driver isf_vdocap_driver = {
	.driver = {
		.name = "isf_vdocap",
		.owner = THIS_MODULE,
		.of_match_table = vdocap_match_table,
	},
	.probe = isf_vdocap_probe,
	.remove = isf_vdocap_remove,
	.suspend = isf_vdocap_suspend,
	.resume = isf_vdocap_resume
};

#if defined(_GROUP_KO_)
#undef __init
#undef __exit
#undef module_init
#undef module_exit
#define __init
#define __exit
#define module_init(x)
#define module_exit(x)
#endif

int __init isf_vdocap_module_init(void)
{
	int ret;

	DBG_IND("\n");
	ret = platform_driver_register(&isf_vdocap_driver);

	return 0;
}

void __exit isf_vdocap_module_exit(void)
{
	DBG_IND("\n");
	platform_driver_unregister(&isf_vdocap_driver);
}

module_init(isf_vdocap_module_init);
module_exit(isf_vdocap_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("isf_vdocap driver");
MODULE_LICENSE("GPL");
