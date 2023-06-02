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

#include "isf_flow_drv.h"
#include "isf_flow_reg.h"
#include "isf_flow_main.h"
#include "isf_flow_proc.h"
#include "../isf_flow_int.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
#define __MODULE__    	  isf_flow_mn
#define __DBGLVL__		  NVT_DBG_WRN
#include "kwrap/debug.h"
unsigned int isf_flow_mn_debug_level = __DBGLVL__;
module_param_named(isf_flow_mn_debug_level, isf_flow_mn_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_mn_debug_level, "stream debug level");
#endif

extern unsigned int isf_flow_debug_level;
module_param_named(isf_flow_debug_level, isf_flow_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_debug_level, "flow debug level");

extern unsigned int isf_flow_c_debug_level;
module_param_named(isf_flow_c_debug_level, isf_flow_c_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_c_debug_level, "flow_c debug level");

extern unsigned int isf_flow_abs_debug_level;
module_param_named(isf_flow_abs_debug_level, isf_flow_abs_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_abs_debug_level, "flow debug level");

extern unsigned int isf_flow_afm_debug_level;
module_param_named(isf_flow_afm_debug_level, isf_flow_afm_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_afm_debug_level, "flow debug level");

extern unsigned int isf_flow_cmm_debug_level;
module_param_named(isf_flow_cmm_debug_level, isf_flow_cmm_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_cmm_debug_level, "flow debug level");

extern unsigned int isf_flow_vbs_debug_level;
module_param_named(isf_flow_vbs_debug_level, isf_flow_vbs_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_vbs_debug_level, "flow debug level");

extern unsigned int isf_flow_vfm_debug_level;
module_param_named(isf_flow_vfm_debug_level, isf_flow_vfm_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_vfm_debug_level, "flow debug level");

extern unsigned int isf_flow_dg_debug_level;
module_param_named(isf_flow_dg_debug_level, isf_flow_dg_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_dg_debug_level, "flow debug level");

extern unsigned int isf_flow_q_debug_level;
module_param_named(isf_flow_q_debug_level, isf_flow_q_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_q_debug_level, "flow_q debug level");

extern unsigned int isf_flow_pd_debug_level;
module_param_named(isf_flow_pd_debug_level, isf_flow_pd_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_pd_debug_level, "flow debug level");

extern unsigned int isf_flow_pp_debug_level;
module_param_named(isf_flow_pp_debug_level, isf_flow_pp_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_pp_debug_level, "flow debug level");

extern unsigned int isf_flow_ps_debug_level;
module_param_named(isf_flow_ps_debug_level, isf_flow_ps_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_ps_debug_level, "flow debug level");



//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id stream_match_table[] = {
	{   .compatible = "nvt,isf_flow"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int isf_flow_open2(struct inode *inode, struct file *file);
static int isf_flow_release(struct inode *inode, struct file *file);
static long isf_flow_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int isf_flow_probe(struct platform_device *p_dev);
static int isf_flow_suspend(struct platform_device *p_dev, pm_message_t state);
static int isf_flow_resume(struct platform_device *p_dev);
static int isf_flow_remove(struct platform_device *p_dev);
int __init isf_flow_module_init(void);
void __exit isf_flow_module_exit(void);

//=============================================================================
// function define
//=============================================================================
#include <asm/current.h>

static int isf_flow_open2(struct inode *inode, struct file *file)
{
	ISF_FLOW_DRV_INFO *p_drv_info;

	p_drv_info = container_of(inode->i_cdev, ISF_FLOW_DRV_INFO, cdev);

	p_drv_info = container_of(inode->i_cdev, ISF_FLOW_DRV_INFO, cdev);
	file->private_data = p_drv_info;

	//printk("<<open>> %s%d, pid =%d, process = %s\n", 
	//	MODULE_NAME, (MINOR(inode->i_rdev) - p_drv_info->minor_base),
	//	(int) task_pid_nr(current), current->comm);

	p_drv_info->pid[(MINOR(inode->i_rdev) - p_drv_info->minor_base)] = (int) task_pid_nr(current);
		
	if (isf_flow_drv_open(&p_drv_info->module_info, MINOR(inode->i_rdev) - p_drv_info->minor_base)) {
		DBG_ERR("failed to open driver\n");
		return -1;
	}

	return 0;
}

static int isf_flow_release(struct inode *inode, struct file *file)
{
	ISF_FLOW_DRV_INFO *p_drv_info;

	p_drv_info = container_of(inode->i_cdev, ISF_FLOW_DRV_INFO, cdev);

	//printk("<<release>> %s%d\n", MODULE_NAME, (MINOR(inode->i_rdev) - p_drv_info->minor_base));
	isf_flow_drv_release(&p_drv_info->module_info, MINOR(inode->i_rdev) - p_drv_info->minor_base);
	return 0;
}

static int isf_flow_flush(struct file *file, fl_owner_t id)
{
	//isf_flow_drv_close();
	return 0;
}

static long isf_flow_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PISF_FLOW_DRV_INFO p_drv_info;

	inode = file_inode(filp);
	p_drv_info = filp->private_data;
	
	//printk("<<ioctl>> %s%d\n", MODULE_NAME, (MINOR(inode->i_rdev) - p_drv_info->minor_base));
	return isf_flow_drv_ioctl(MINOR(inode->i_rdev) - p_drv_info->minor_base, &p_drv_info->module_info, cmd, arg);
}

static int isf_flow_mmap(struct file *file, struct vm_area_struct *vma)
{
#if 0
	ISF_FLOW_CTRL *p_ctrl = stream_get_ctrl();
#endif

	int er = 0;
#if 0
	PISF_FLOW_DRV_INFO p_drv_info;
	unsigned long pfn_start;
	unsigned long virt_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long size = vma->vm_end - vma->vm_start;

	p_drv_info = file->private_data;
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

struct file_operations isf_flow_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_flow_open2,
	.release = isf_flow_release,
	.flush   = isf_flow_flush,
	.mmap    = isf_flow_mmap,
	.unlocked_ioctl = isf_flow_ioctl,
	.llseek  = no_llseek,
};

static int isf_flow_probe(struct platform_device *p_dev)
{
	ISF_FLOW_DRV_INFO *p_drv_info;
#if 0
	const struct of_device_id *match;
#endif
	int ret = 0;
	int ucloop;

	DBG_IND("%s\n", p_dev->name);

	p_drv_info = kzalloc(sizeof(ISF_FLOW_DRV_INFO), GFP_KERNEL);
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
	cdev_init(&p_drv_info->cdev, &isf_flow_fops);
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

	p_drv_info->minor_base = MINOR(p_drv_info->dev_id);
	
	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		if (MODULE_MINOR_COUNT == 1) {
		p_drv_info->p_device[ucloop] = device_create(p_drv_info->pmodule_class, NULL
									   , MKDEV(MAJOR(p_drv_info->dev_id), (ucloop + MINOR(p_drv_info->dev_id))), NULL
									   , MODULE_NAME);
		} else {
		p_drv_info->p_device[ucloop] = device_create(p_drv_info->pmodule_class, NULL
									   , MKDEV(MAJOR(p_drv_info->dev_id), (ucloop + MINOR(p_drv_info->dev_id))), NULL
									   , MODULE_NAME"%d", ucloop);
		}
		//printk("<<register>> %s%d\n", MODULE_NAME, ucloop);
		
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

	ret = isf_flow_proc_init(p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DEV;
	}

	ret = isf_flow_drv_init(&p_drv_info->module_info);

	platform_set_drvdata(p_dev, p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	isf_flow_proc_remove(p_drv_info);

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

static int isf_flow_remove(struct platform_device *p_dev)
{
	PISF_FLOW_DRV_INFO p_drv_info;
	unsigned char ucloop;

	DBG_IND("\n");

	p_drv_info = platform_get_drvdata(p_dev);

	isf_flow_drv_remove(&p_drv_info->module_info);

	isf_flow_proc_remove(p_drv_info);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(p_drv_info->p_device[ucloop]);
		//printk("<<unregister>> %s%d\n", MODULE_NAME, ucloop);
	}

	class_destroy(p_drv_info->pmodule_class);
	cdev_del(&p_drv_info->cdev);
	vos_unregister_chrdev_region(p_drv_info->dev_id, MODULE_MINOR_COUNT);

	kfree(p_drv_info);
	p_drv_info = NULL;
	platform_set_drvdata(p_dev, p_drv_info);
	return 0;
}

static int isf_flow_suspend(struct platform_device *p_dev, pm_message_t state)
{
	PISF_FLOW_DRV_INFO p_drv_info;;

	DBG_IND("start\n");

	p_drv_info = platform_get_drvdata(p_dev);
	isf_flow_drv_suspend(&p_drv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}


static int isf_flow_resume(struct platform_device *p_dev)
{
	PISF_FLOW_DRV_INFO p_drv_info;;

	DBG_IND("start\n");

	p_drv_info = platform_get_drvdata(p_dev);
	isf_flow_drv_resume(&p_drv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}

static struct platform_driver isf_flow_driver = {
	.driver = {
		.name = "isf_flow",
		.owner = THIS_MODULE,
		.of_match_table = stream_match_table,
	},
	.probe = isf_flow_probe,
	.remove = isf_flow_remove,
	.suspend = isf_flow_suspend,
	.resume = isf_flow_resume
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

int __init isf_flow_module_init(void)
{
	int ret;

	DBG_IND("\n");
	ret = platform_driver_register(&isf_flow_driver);

	return 0;
}

void __exit isf_flow_module_exit(void)
{
	DBG_IND("\n");
	platform_driver_unregister(&isf_flow_driver);
}

module_init(isf_flow_module_init);
module_exit(isf_flow_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("isf_flow driver");
MODULE_LICENSE("GPL");
