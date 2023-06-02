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
#include <asm/signal.h>
#include <kwrap/dev.h>

#include "usb_chrgdet_drv.h"
//#include "usb_chrgdet_reg.h"
#include "usb_chrgdet_main.h"
#include "usb_chrgdet_proc.h"
#include "usb_chrgdet_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int usb_chrgdet_debug_level = NVT_DBG_WRN;
module_param_named(usb_chrgdet_debug_level, usb_chrgdet_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(usb_chrgdet_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id usb_chrgdet_match_table[] = {
	{   .compatible = "nvt,nvt_usb_chrgdet"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_usb_chrgdet_open(struct inode *inode, struct file *file);
static int nvt_usb_chrgdet_release(struct inode *inode, struct file *file);
static long nvt_usb_chrgdet_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_usb_chrgdet_probe(struct platform_device *pdev);
static int nvt_usb_chrgdet_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_usb_chrgdet_resume(struct platform_device *pdev);
static int nvt_usb_chrgdet_remove(struct platform_device *pdev);
int __init nvt_usb_chrgdet_module_init(void);
void __exit nvt_usb_chrgdet_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int nvt_usb_chrgdet_open(struct inode *inode, struct file *file)
{
	USB_CHRGDET_DRV_INFO *pdrv_info;

	usb_chrgdet_api("%s\n", __func__);

	pdrv_info = container_of(inode->i_cdev, USB_CHRGDET_DRV_INFO, cdev);

	pdrv_info = container_of(inode->i_cdev, USB_CHRGDET_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (nvt_usb_chrgdet_drv_open(&pdrv_info->module_info, MINOR(inode->i_rdev))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_usb_chrgdet_release(struct inode *inode, struct file *file)
{
	USB_CHRGDET_DRV_INFO *pdrv_info;

	usb_chrgdet_api("%s\n", __func__);

	pdrv_info = container_of(inode->i_cdev, USB_CHRGDET_DRV_INFO, cdev);

	nvt_usb_chrgdet_drv_release(&pdrv_info->module_info, MINOR(inode->i_rdev));
	return 0;
}

static long nvt_usb_chrgdet_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PUSB_CHRGDET_DRV_INFO pdrv;

	usb_chrgdet_api("%s\n", __func__);

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return nvt_usb_chrgdet_drv_ioctl(MINOR(inode->i_rdev), &pdrv->module_info, cmd, arg);
}

struct file_operations nvt_usb_chrgdet_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_usb_chrgdet_open,
	.release = nvt_usb_chrgdet_release,
	.unlocked_ioctl = nvt_usb_chrgdet_ioctl,
	.llseek  = no_llseek,
};

static int nvt_usb_chrgdet_probe(struct platform_device *pdev)
{
	USB_CHRGDET_DRV_INFO *pdrv_info;//info;
	const struct of_device_id *match;
	int ret = 0;
	unsigned char ucloop;

	nvt_dbg(IND, "%s\n", pdev->name);

	usb_chrgdet_api("%s\n", __func__);

	match = of_match_device(usb_chrgdet_match_table, &pdev->dev);
	if (!match) {
		printk("Platform device not found \n");
		return -EINVAL;
	}

	pdrv_info = kzalloc(sizeof(USB_CHRGDET_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		printk("failed to allocate memory\n");
		return -ENOMEM;
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		pdrv_info->presource[ucloop] = platform_get_resource(pdev, IORESOURCE_MEM, ucloop);
		if (pdrv_info->presource[ucloop] == NULL) {
			printk("No IO memory resource defined:%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		pdrv_info->module_info.io_addr[ucloop] = ioremap_nocache(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		usb_chrgdet_api("start 0x%08X  mapped to 0x%08X\n", (unsigned int)pdrv_info->presource[ucloop]->start, (unsigned int)pdrv_info->module_info.io_addr[ucloop]);
		if (pdrv_info->module_info.io_addr[ucloop] == NULL) {
			printk("ioremap() failed in module%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_RES;
		}
	}

	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		printk("Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	nvt_dbg(IND, "DevID Major:%d minor:%d\n" \
			, MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_usb_chrgdet_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, MODULE_MINOR_COUNT)) {
		printk("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pdrv_info->pmodule_class = class_create(THIS_MODULE, MODULE_NAME);
	if (IS_ERR(pdrv_info->pmodule_class)) {
		printk("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
									 , MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
									 , MODULE_NAME"%d", ucloop);

		if (IS_ERR(pdrv_info->pdevice[ucloop])) {
			printk("failed in creating device%d.\n", ucloop);
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	ret = nvt_usb_chrgdet_proc_init(pdrv_info);
	if (ret) {
		printk("failed in creating proc.\n");
		goto FAIL_DEV;
	}

	ret = nvt_usb_chrgdet_drv_init(&pdrv_info->module_info);

	platform_set_drvdata(pdev, pdrv_info);
	if (ret) {
		printk("failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	nvt_usb_chrgdet_proc_remove(pdrv_info);

FAIL_DEV:
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

FAIL_FREE_REMAP:
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		iounmap(pdrv_info->module_info.io_addr[ucloop]);
	}

FAIL_FREE_RES:
FAIL_FREE_BUF:
	kfree(pdrv_info);
	//coverity[assigned_pointer]
	pdrv_info = NULL;
	return ret;
}

static int nvt_usb_chrgdet_remove(struct platform_device *pdev)
{
	PUSB_CHRGDET_DRV_INFO pdrv_info;
	unsigned char ucloop;

	nvt_dbg(IND, "\n");

	usb_chrgdet_api("%s\n", __func__);

	pdrv_info = platform_get_drvdata(pdev);

	nvt_usb_chrgdet_drv_remove(&pdrv_info->module_info);

	nvt_usb_chrgdet_proc_remove(pdrv_info);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		iounmap(pdrv_info->module_info.io_addr[ucloop]);
	}

	kfree(pdrv_info);
	//coverity[assigned_pointer]
	pdrv_info = NULL;
	return 0;
}

static int nvt_usb_chrgdet_suspend(struct platform_device *pdev, pm_message_t state)
{
	//PUSB_CHRGDET_DRV_INFO pdrv_info;
	//nvt_dbg(IND, "start\n");
	usb_chrgdet_api("%s\n", __func__);
	//pdrv_info = platform_get_drvdata(pdev);
	//nvt_usb_chrgdet_drv_suspend(&pdrv_info->module_info);
	//nvt_dbg(IND, "finished\n");
	return 0;
}


static int nvt_usb_chrgdet_resume(struct platform_device *pdev)
{
	//PUSB_CHRGDET_DRV_INFO pdrv_info;
	//nvt_dbg(IND, "start\n");
	usb_chrgdet_api("%s\n", __func__);
	//pdrv_info = platform_get_drvdata(pdev);
	//nvt_usb_chrgdet_drv_resume(&pdrv_info->module_info);
	//nvt_dbg(IND, "finished\n");
	return 0;
}

static struct platform_driver nvt_usb_chrgdet_driver = {
	.driver = {
		.name   = "nvt_usb_chrgdet",
		.owner = THIS_MODULE,
		.of_match_table = usb_chrgdet_match_table,
	},
	.probe      = nvt_usb_chrgdet_probe,
	.remove     = nvt_usb_chrgdet_remove,
	.suspend = nvt_usb_chrgdet_suspend,
	.resume = nvt_usb_chrgdet_resume
};

int __init nvt_usb_chrgdet_module_init(void)
{
	int ret;

	nvt_dbg(IND, "\n");

	usb_chrgdet_api("%s\n", __func__);

	ret = platform_driver_register(&nvt_usb_chrgdet_driver);

	return 0;
}

void __exit nvt_usb_chrgdet_module_exit(void)
{
	nvt_dbg(IND, "\n");

	usb_chrgdet_api("%s\n", __func__);

	platform_driver_unregister(&nvt_usb_chrgdet_driver);

}

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

module_init(nvt_usb_chrgdet_module_init);
module_exit(nvt_usb_chrgdet_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("usb_chrgdet driver");
MODULE_LICENSE("GPL");
