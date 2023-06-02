#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <kwrap/dev.h>

#include "isf_vdodec_drv.h"
#include "isf_vdodec_reg.h"
#include "isf_vdodec_main.h"
#include "isf_vdodec_proc.h"

#include "isf_vdodec_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#define __DBGLVL_DEFAULT__  NVT_DBG_WRN
unsigned int isf_vdodec_mn_debug_level = __DBGLVL_DEFAULT__;
module_param_named(debug_level_isf_vdodec_mn, isf_vdodec_mn_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug_level_isf_vdodec_mn, "vdodec driver debug level");

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id vdodec_match_table[] = {
	{   .compatible = "nvt,isf_vdodec"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int isf_vdodec_open(struct inode *inode, struct file *file);
static int isf_vdodec_release(struct inode *inode, struct file *file);
static long isf_vdodec_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int isf_vdodec_probe(struct platform_device *p_dev);
static int isf_vdodec_suspend(struct platform_device *p_dev, pm_message_t state);
static int isf_vdodec_resume(struct platform_device *p_dev);
static int isf_vdodec_remove(struct platform_device *p_dev);
int __init isf_vdodec_module_init(void);
void __exit isf_vdodec_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int isf_vdodec_open(struct inode *inode, struct file *file)
{
	ISF_VDODEC_DRV_INFO *p_drv_info;

	p_drv_info = container_of(inode->i_cdev, ISF_VDODEC_DRV_INFO, cdev);

	p_drv_info = container_of(inode->i_cdev, ISF_VDODEC_DRV_INFO, cdev);
	file->private_data = p_drv_info;

	if (isf_vdodec_drv_open(&p_drv_info->module_info, MINOR(inode->i_rdev))) {
		DBG_ERR("failed to open driver\n");
		return -1;
	}

	return 0;
}

static int isf_vdodec_release(struct inode *inode, struct file *file)
{
	ISF_VDODEC_DRV_INFO *p_drv_info;

	p_drv_info = container_of(inode->i_cdev, ISF_VDODEC_DRV_INFO, cdev);
	isf_vdodec_drv_release(&p_drv_info->module_info, MINOR(inode->i_rdev));
	return 0;
}

static long isf_vdodec_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PISF_VDODEC_DRV_INFO p_drv;

	inode = file_inode(filp);
	p_drv = filp->private_data;

	return isf_vdodec_drv_ioctl(MINOR(inode->i_rdev), &p_drv->module_info, cmd, arg);
}

static int isf_vdodec_mmap(struct file *file, struct vm_area_struct *vma)
{
#if 0
	ISF_VDODEC_CTRL *p_ctrl = vdodec_get_ctrl();
#endif

	int er = 0;
#if 0
	PISF_VDODEC_DRV_INFO p_drv;
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

struct file_operations isf_vdodec_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_vdodec_open,
	.release = isf_vdodec_release,
	.mmap    = isf_vdodec_mmap,
	.unlocked_ioctl = isf_vdodec_ioctl,
	.llseek  = no_llseek,
};

static int isf_vdodec_probe(struct platform_device *p_dev)
{
	ISF_VDODEC_DRV_INFO *p_drv_info;
#if 0
	const struct of_device_id *match;
#endif
	int ret = 0;
	unsigned char ucloop;

	DBG_IND("%s\n", p_dev->name);

	p_drv_info = kzalloc(sizeof(ISF_VDODEC_DRV_INFO), GFP_KERNEL);
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
	cdev_init(&p_drv_info->cdev, &isf_vdodec_fops);
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

			#if (MODULE_MINOR_COUNT > 1) //CID 132881 (#1 of 1): Logically dead code (DEADCODE)
			for (; ucloop > 0 ; ucloop--) {
				device_unregister(p_drv_info->p_device[ucloop - 1]);
			}
			#endif
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	ret = isf_vdodec_proc_init(p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DEV;
	}

	ret = isf_vdodec_drv_init(&p_drv_info->module_info);

	platform_set_drvdata(p_dev, p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	isf_vdodec_proc_remove(p_drv_info);

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
	//p_drv_info = NULL;
	return ret;
}

static int isf_vdodec_remove(struct platform_device *p_dev)
{
	PISF_VDODEC_DRV_INFO p_drv_info;
	unsigned char ucloop;

	DBG_IND("\n");

	p_drv_info = platform_get_drvdata(p_dev);

	isf_vdodec_drv_remove(&p_drv_info->module_info);

	isf_vdodec_proc_remove(p_drv_info);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(p_drv_info->p_device[ucloop]);
	}

	class_destroy(p_drv_info->pmodule_class);
	cdev_del(&p_drv_info->cdev);
	vos_unregister_chrdev_region(p_drv_info->dev_id, MODULE_MINOR_COUNT);

	kfree(p_drv_info);
	//p_drv_info = NULL;
	return 0;
}

static int isf_vdodec_suspend(struct platform_device *p_dev, pm_message_t state)
{
	PISF_VDODEC_DRV_INFO p_drv_info;;

	DBG_IND("start\n");

	p_drv_info = platform_get_drvdata(p_dev);
	isf_vdodec_drv_suspend(&p_drv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}


static int isf_vdodec_resume(struct platform_device *p_dev)
{
	PISF_VDODEC_DRV_INFO p_drv_info;;

	DBG_IND("start\n");

	p_drv_info = platform_get_drvdata(p_dev);
	isf_vdodec_drv_resume(&p_drv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}

static struct platform_driver isf_vdodec_driver = {
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = vdodec_match_table,
	},
	.probe = isf_vdodec_probe,
	.remove = isf_vdodec_remove,
	.suspend = isf_vdodec_suspend,
	.resume = isf_vdodec_resume
};

int __init isf_vdodec_module_init(void)
{
	int ret;

	DBG_IND("\n");
	ret = platform_driver_register(&isf_vdodec_driver);

	return 0;
}

void __exit isf_vdodec_module_exit(void)
{
	DBG_IND("\n");
	platform_driver_unregister(&isf_vdodec_driver);
}

module_init(isf_vdodec_module_init);
module_exit(isf_vdodec_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("isf_vdodec driver");
MODULE_LICENSE("GPL");
