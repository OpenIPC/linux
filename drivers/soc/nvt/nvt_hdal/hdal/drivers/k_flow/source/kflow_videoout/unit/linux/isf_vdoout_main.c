#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <kwrap/dev.h>

#include "isf_vdoout_drv.h"
#include "isf_vdoout_main.h"
#include "isf_vdoout_proc.h"
#include "../isf_vdoout_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id vdoout_match_table[] = {
	{   .compatible = "nvt,isf_vdoout"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int isf_vdoout_open(struct inode *inode, struct file *file);
static int isf_vdoout_release(struct inode *inode, struct file *file);
static long isf_vdoout_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int isf_vdoout_probe(struct platform_device *p_dev);
static int isf_vdoout_suspend(struct platform_device *p_dev, pm_message_t state);
static int isf_vdoout_resume(struct platform_device *p_dev);
static int isf_vdoout_remove(struct platform_device *p_dev);
int __init isf_vdoout_module_init(void);
void __exit isf_vdoout_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int isf_vdoout_open(struct inode *inode, struct file *file)
{
	ISF_VDOOUT_DRV_INFO *p_drv_info;

	p_drv_info = container_of(inode->i_cdev, ISF_VDOOUT_DRV_INFO, cdev);

	p_drv_info = container_of(inode->i_cdev, ISF_VDOOUT_DRV_INFO, cdev);
	file->private_data = p_drv_info;

	if (isf_vdoout_drv_open(&p_drv_info->module_info, MINOR(inode->i_rdev))) {
		DBG_ERR("failed to open driver\n");
		return -1;
	}

	return 0;
}

static int isf_vdoout_release(struct inode *inode, struct file *file)
{
	ISF_VDOOUT_DRV_INFO *p_drv_info;

	p_drv_info = container_of(inode->i_cdev, ISF_VDOOUT_DRV_INFO, cdev);
	isf_vdoout_drv_release(&p_drv_info->module_info, MINOR(inode->i_rdev));
	return 0;
}

static long isf_vdoout_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PISF_VDOOUT_DRV_INFO p_drv;

	inode = file_inode(filp);
	p_drv = filp->private_data;

	return isf_vdoout_drv_ioctl(MINOR(inode->i_rdev), &p_drv->module_info, cmd, arg);
}



struct file_operations isf_vdoout_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_vdoout_open,
	.release = isf_vdoout_release,
	.unlocked_ioctl = isf_vdoout_ioctl,
	.llseek  = no_llseek,
};

static int isf_vdoout_probe(struct platform_device *p_dev)
{
	ISF_VDOOUT_DRV_INFO *p_drv_info;
	int ret = 0;
	unsigned char ucloop;

	p_drv_info = kzalloc(sizeof(ISF_VDOOUT_DRV_INFO), GFP_KERNEL);
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
	cdev_init(&p_drv_info->cdev, &isf_vdoout_fops);
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
			#if (MODULE_MINOR_COUNT > 1) //CID 131664 (#1 of 1): Logically dead code (DEADCODE)
			for (; ucloop > 0 ; ucloop--) {
				device_unregister(p_drv_info->p_device[ucloop - 1]);
			}
			#endif
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	ret = isf_vdoout_proc_init(p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DEV;
	}

	ret = isf_vdoout_drv_init(&p_drv_info->module_info);

	platform_set_drvdata(p_dev, p_drv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	isf_vdoout_proc_remove(p_drv_info);

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

static int isf_vdoout_remove(struct platform_device *p_dev)
{
	PISF_VDOOUT_DRV_INFO p_drv_info;
	unsigned char ucloop;

	DBG_IND("\n");

	p_drv_info = platform_get_drvdata(p_dev);

	isf_vdoout_drv_remove(&p_drv_info->module_info);

	isf_vdoout_proc_remove(p_drv_info);

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

static int isf_vdoout_suspend(struct platform_device *p_dev, pm_message_t state)
{
	PISF_VDOOUT_DRV_INFO p_drv_info;;

	DBG_IND("start\n");

	p_drv_info = platform_get_drvdata(p_dev);
	isf_vdoout_drv_suspend(&p_drv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}


static int isf_vdoout_resume(struct platform_device *p_dev)
{
	PISF_VDOOUT_DRV_INFO p_drv_info;;

	DBG_IND("start\n");

	p_drv_info = platform_get_drvdata(p_dev);
	isf_vdoout_drv_resume(&p_drv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}

static struct platform_driver isf_vdoout_driver = {
	.driver = {
		.name = "isf_vdoout",
		.owner = THIS_MODULE,
		.of_match_table = vdoout_match_table,
	},
	.probe = isf_vdoout_probe,
	.remove = isf_vdoout_remove,
	.suspend = isf_vdoout_suspend,
	.resume = isf_vdoout_resume
};

int __init isf_vdoout_module_init(void)
{
	int ret;

	DBG_IND("\n");
	ret = platform_driver_register(&isf_vdoout_driver);

	return 0;
}

void __exit isf_vdoout_module_exit(void)
{
	DBG_IND("\n");
	platform_driver_unregister(&isf_vdoout_driver);
}

module_init(isf_vdoout_module_init);
module_exit(isf_vdoout_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("isf_vdoout driver");
MODULE_LICENSE("GPL");
