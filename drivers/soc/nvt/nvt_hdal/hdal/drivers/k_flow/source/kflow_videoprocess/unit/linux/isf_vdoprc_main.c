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

#include "isf_vdoprc_drv.h"
#include "isf_vdoprc_reg.h"
#include "isf_vdoprc_main.h"
#include "isf_vdoprc_proc.h"
#include "../isf_vdoprc_int.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
#define __MODULE__    	  isf_vdoprc_mn
#define __DBGLVL__		  8
#include "kwrap/debug.h"
unsigned int isf_vdoprc_mn_debug_level = NVT_DBG_WRN;
module_param_named(isf_vdoprc_mn_debug_level, isf_vdoprc_mn_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_mn_debug_level, "vdoprc debug level");
#endif

extern unsigned int isf_vdoprc_debug_level;
module_param_named(isf_vdoprc_debug_level, isf_vdoprc_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_debug_level, "vdoprc debug level");

extern unsigned int isf_vdoprc_d_debug_level;
module_param_named(isf_vdoprc_d_debug_level, isf_vdoprc_d_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_d_debug_level, "vdoprc1 debug level");

extern unsigned int isf_vdoprc_o_debug_level;
module_param_named(isf_vdoprc_o_debug_level, isf_vdoprc_o_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_o_debug_level, "vdoprc_o debug level");

extern unsigned int isf_vdoprc_ox_debug_level;
module_param_named(isf_vdoprc_ox_debug_level, isf_vdoprc_ox_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_ox_debug_level, "vdoprc_ox debug level");

extern unsigned int isf_vdoprc_ext_debug_level;
module_param_named(isf_vdoprc_ext_debug_level, isf_vdoprc_ext_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_ext_debug_level, "vdoprc_ext debug level");

#if (USE_VPE == ENABLE)
extern unsigned int isf_vdoprc_ov_debug_level;
module_param_named(isf_vdoprc_ov_debug_level, isf_vdoprc_ov_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_ov_debug_level, "vdoprc_ov debug level");
#endif
#if (USE_ISE == ENABLE)
extern unsigned int isf_vdoprc_ise_debug_level;
module_param_named(isf_vdoprc_ise_debug_level, isf_vdoprc_ise_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_ise_debug_level, "vdoprc_ise debug level");
#endif

extern unsigned int isf_vdoprc_p_debug_level;
module_param_named(isf_vdoprc_p_debug_level, isf_vdoprc_p_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_p_debug_level, "vdoprc_p debug level");

extern unsigned int isf_vdoprc_pl_debug_level;
module_param_named(isf_vdoprc_pl_debug_level, isf_vdoprc_pl_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_pl_debug_level, "vdoprc_pl debug level");

extern unsigned int isf_vdoprc_i_debug_level;
module_param_named(isf_vdoprc_i_debug_level, isf_vdoprc_i_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_i_debug_level, "vdoprc_i debug level");

#if (USE_VPE == ENABLE)
extern unsigned int isf_vdoprc_iv_debug_level;
module_param_named(isf_vdoprc_iv_debug_level, isf_vdoprc_iv_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_iv_debug_level, "vdoprc_iv debug level");
#endif

#if (USE_ISE == ENABLE)
extern unsigned int isf_vdoprc_ii_debug_level;
module_param_named(isf_vdoprc_ii_debug_level, isf_vdoprc_ii_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc_ii_debug_level, "vdoprc_ii debug level");
#endif

extern unsigned int isf_vdoprc0_debug_level;
module_param_named(isf_vdoprc0_debug_level, isf_vdoprc0_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc0_debug_level, "vdoprc0 debug level");

extern unsigned int isf_vdoprc1_debug_level;
module_param_named(isf_vdoprc1_debug_level, isf_vdoprc1_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc1_debug_level, "vdoprc1 debug level");

extern unsigned int isf_vdoprc2_debug_level;
module_param_named(isf_vdoprc2_debug_level, isf_vdoprc2_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc2_debug_level, "vdoprc2 debug level");

extern unsigned int isf_vdoprc3_debug_level;
module_param_named(isf_vdoprc3_debug_level, isf_vdoprc3_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc3_debug_level, "vdoprc3 debug level");

extern unsigned int isf_vdoprc4_debug_level;
module_param_named(isf_vdoprc4_debug_level, isf_vdoprc4_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc4_debug_level, "vdoprc4 debug level");

extern unsigned int isf_vdoprc5_debug_level;
module_param_named(isf_vdoprc5_debug_level, isf_vdoprc5_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc5_debug_level, "vdoprc5 debug level");

extern unsigned int isf_vdoprc6_debug_level;
module_param_named(isf_vdoprc6_debug_level, isf_vdoprc6_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc6_debug_level, "vdoprc6 debug level");

extern unsigned int isf_vdoprc7_debug_level;
module_param_named(isf_vdoprc7_debug_level, isf_vdoprc7_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc7_debug_level, "vdoprc7 debug level");

extern unsigned int isf_vdoprc8_debug_level;
module_param_named(isf_vdoprc8_debug_level, isf_vdoprc8_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc8_debug_level, "vdoprc8 debug level");

extern unsigned int isf_vdoprc9_debug_level;
module_param_named(isf_vdoprc9_debug_level, isf_vdoprc9_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc9_debug_level, "vdoprc9 debug level");

extern unsigned int isf_vdoprc10_debug_level;
module_param_named(isf_vdoprc10_debug_level, isf_vdoprc10_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc10_debug_level, "vdoprc10 debug level");

extern unsigned int isf_vdoprc11_debug_level;
module_param_named(isf_vdoprc11_debug_level, isf_vdoprc11_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc11_debug_level, "vdoprc11 debug level");

extern unsigned int isf_vdoprc12_debug_level;
module_param_named(isf_vdoprc12_debug_level, isf_vdoprc12_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc12_debug_level, "vdoprc12 debug level");

extern unsigned int isf_vdoprc13_debug_level;
module_param_named(isf_vdoprc13_debug_level, isf_vdoprc13_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc13_debug_level, "vdoprc13 debug level");

extern unsigned int isf_vdoprc14_debug_level;
module_param_named(isf_vdoprc14_debug_level, isf_vdoprc14_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc14_debug_level, "vdoprc14 debug level");

extern unsigned int isf_vdoprc15_debug_level;
module_param_named(isf_vdoprc15_debug_level, isf_vdoprc15_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdoprc15_debug_level, "vdoprc15 debug level");

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id vdoprc_match_table[] = {
	{   .compatible = "nvt,isf_vdoprc"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int isf_vdoprc_open(struct inode *inode, struct file *file);
static int isf_vdoprc_release(struct inode *inode, struct file *file);
static long isf_vdoprc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int isf_vdoprc_probe(struct platform_device *p_dev);
static int isf_vdoprc_suspend(struct platform_device *p_dev, pm_message_t state);
static int isf_vdoprc_resume(struct platform_device *p_dev);
static int isf_vdoprc_remove(struct platform_device *p_dev);
int __init isf_vdoprc_module_init(void);
void __exit isf_vdoprc_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int isf_vdoprc_open(struct inode *inode, struct file *file)
{
	ISF_VDOPRC_DRV_INFO *p_drv_info;

	p_drv_info = container_of(inode->i_cdev, ISF_VDOPRC_DRV_INFO, cdev);

	p_drv_info = container_of(inode->i_cdev, ISF_VDOPRC_DRV_INFO, cdev);
	file->private_data = p_drv_info;

	if (isf_vdoprc_drv_open(&p_drv_info->module_info, MINOR(inode->i_rdev))) {
		DBG_ERR("failed to open driver\n");
		return -1;
	}

	return 0;
}

static int isf_vdoprc_release(struct inode *inode, struct file *file)
{
	ISF_VDOPRC_DRV_INFO *p_drv_info;

	p_drv_info = container_of(inode->i_cdev, ISF_VDOPRC_DRV_INFO, cdev);
	isf_vdoprc_drv_release(&p_drv_info->module_info, MINOR(inode->i_rdev));
	return 0;
}

static long isf_vdoprc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PISF_VDOPRC_DRV_INFO p_drv;

	inode = file_inode(filp);
	p_drv = filp->private_data;

	return isf_vdoprc_drv_ioctl(MINOR(inode->i_rdev), &p_drv->module_info, cmd, arg);
}

static int isf_vdoprc_mmap(struct file *file, struct vm_area_struct *vma)
{
#if 0
	ISF_VDOPRC_CTRL *p_ctrl = vdoprc_get_ctrl();
#endif

	int er = 0;
#if 0
	PISF_VDOPRC_DRV_INFO p_drv;
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

struct file_operations isf_vdoprc_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_vdoprc_open,
	.release = isf_vdoprc_release,
	.mmap    = isf_vdoprc_mmap,
	.unlocked_ioctl = isf_vdoprc_ioctl,
	.llseek  = no_llseek,
};

static int isf_vdoprc_probe(struct platform_device *p_dev)
{
	ISF_VDOPRC_DRV_INFO *p_drv_info;
#if 0
	const struct of_device_id *match;
#endif
	int ret = 0;
	int ucloop;

	DBG_IND("%s\n", p_dev->name);

	p_drv_info = kzalloc(sizeof(ISF_VDOPRC_DRV_INFO), GFP_KERNEL);
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
	cdev_init(&p_drv_info->cdev, &isf_vdoprc_fops);
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

			#if (MODULE_MINOR_COUNT > 1)
				for (;ucloop > 0; ucloop--) {
					device_unregister(p_drv_info->p_device[ucloop - 1]);
				}
			#endif
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	ret = isf_vdoprc_proc_init(p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DEV;
	}

	ret = isf_vdoprc_drv_init(&p_drv_info->module_info);

	platform_set_drvdata(p_dev, p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	isf_vdoprc_proc_remove(p_drv_info);

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
	p_drv_info = NULL;
	platform_set_drvdata(p_dev, p_drv_info);
	return ret;
}

static int isf_vdoprc_remove(struct platform_device *p_dev)
{
	PISF_VDOPRC_DRV_INFO p_drv_info;
	unsigned char ucloop;

	DBG_IND("\n");

	p_drv_info = platform_get_drvdata(p_dev);

	isf_vdoprc_drv_remove(&p_drv_info->module_info);

	isf_vdoprc_proc_remove(p_drv_info);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(p_drv_info->p_device[ucloop]);
	}

	class_destroy(p_drv_info->pmodule_class);
	cdev_del(&p_drv_info->cdev);
	vos_unregister_chrdev_region(p_drv_info->dev_id, MODULE_MINOR_COUNT);

	kfree(p_drv_info);
	p_drv_info = NULL;
	platform_set_drvdata(p_dev, p_drv_info);
	return 0;
}

static int isf_vdoprc_suspend(struct platform_device *p_dev, pm_message_t state)
{
	PISF_VDOPRC_DRV_INFO p_drv_info;;

	DBG_IND("start\n");

	p_drv_info = platform_get_drvdata(p_dev);
	isf_vdoprc_drv_suspend(&p_drv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}


static int isf_vdoprc_resume(struct platform_device *p_dev)
{
	PISF_VDOPRC_DRV_INFO p_drv_info;;

	DBG_IND("start\n");

	p_drv_info = platform_get_drvdata(p_dev);
	isf_vdoprc_drv_resume(&p_drv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}

static struct platform_driver isf_vdoprc_driver = {
	.driver = {
		.name = "isf_vdoprc",
		.owner = THIS_MODULE,
		.of_match_table = vdoprc_match_table,
	},
	.probe = isf_vdoprc_probe,
	.remove = isf_vdoprc_remove,
	.suspend = isf_vdoprc_suspend,
	.resume = isf_vdoprc_resume
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

int __init isf_vdoprc_module_init(void)
{
	int ret;

	DBG_IND("\n");
	ret = platform_driver_register(&isf_vdoprc_driver);

	return 0;
}

void __exit isf_vdoprc_module_exit(void)
{
	DBG_IND("\n");
	platform_driver_unregister(&isf_vdoprc_driver);
}

module_init(isf_vdoprc_module_init);
module_exit(isf_vdoprc_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("isf_vdoprc driver");
MODULE_LICENSE("GPL");
