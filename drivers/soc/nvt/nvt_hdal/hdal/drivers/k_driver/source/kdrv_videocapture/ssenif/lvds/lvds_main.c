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

#include "lvds_drv.h"
#include "lvds_reg.h"
#include "lvds_main.h"
#include "lvds_proc.h"
#include "lvds_dbg.h"
#include "../lvds.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int lvds_debug_level = NVT_DBG_WRN;
module_param_named(lvds_debug_level, lvds_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(lvds_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id lvds_match_table[] = {
	{   .compatible = "nvt,nvt_lvds"},
	{}
};

extern void                 ssenif_hook_lvds(void *object);
extern void                 ssenif_hook_lvds2(void *object);

//=============================================================================
// function declaration
//=============================================================================
static int nvt_lvds_open(struct inode *inode, struct file *file);
static int nvt_lvds_release(struct inode *inode, struct file *file);
static long nvt_lvds_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_lvds_probe(struct platform_device *pdev);
static int nvt_lvds_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_lvds_resume(struct platform_device *pdev);
static int nvt_lvds_remove(struct platform_device *pdev);
int __init nvt_lvds_module_init(void);
void __exit nvt_lvds_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int nvt_lvds_open(struct inode *inode, struct file *file)
{
	LVDS_DRV_INFO *pdrv_info;

	lvds_api("%s\n", __func__);

	pdrv_info = container_of(inode->i_cdev, LVDS_DRV_INFO, cdev);

	pdrv_info = container_of(inode->i_cdev, LVDS_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (nvt_lvds_drv_open(&pdrv_info->module_info, MINOR(inode->i_rdev))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_lvds_release(struct inode *inode, struct file *file)
{
	LVDS_DRV_INFO *pdrv_info;

	lvds_api("%s\n", __func__);

	pdrv_info = container_of(inode->i_cdev, LVDS_DRV_INFO, cdev);

	nvt_lvds_drv_release(&pdrv_info->module_info, MINOR(inode->i_rdev));
	return 0;
}

static long nvt_lvds_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PLVDS_DRV_INFO pdrv;

	lvds_api("%s\n", __func__);

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return nvt_lvds_drv_ioctl(MINOR(inode->i_rdev), &pdrv->module_info, cmd, arg);
}

struct file_operations nvt_lvds_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_lvds_open,
	.release = nvt_lvds_release,
	.unlocked_ioctl = nvt_lvds_ioctl,
	.llseek  = no_llseek,
};

static int nvt_lvds_probe(struct platform_device *pdev)
{
	LVDS_DRV_INFO *pdrv_info;//info;
	const struct of_device_id *match;
	int ret = 0;
	unsigned char ucloop;

	nvt_dbg(IND, "%s\n", pdev->name);

	match = of_match_device(lvds_match_table, &pdev->dev);
	if (!match) {
		nvt_dbg(ERR, "Platform device not found \n");
		return -EINVAL;
	}

	pdrv_info = kzalloc(sizeof(LVDS_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		nvt_dbg(ERR, "failed to allocate memory\n");
		return -ENOMEM;
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		pdrv_info->presource[ucloop] = platform_get_resource(pdev, IORESOURCE_MEM, ucloop);
		if (pdrv_info->presource[ucloop] == NULL) {
			nvt_dbg(ERR, "No IO memory resource defined:%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (!request_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]), pdev->name)) {
			nvt_dbg(ERR, "failed to request memory resource%d\n", ucloop);
			for (; ucloop > 0 ;) {
				ucloop -= 1;
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			}
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		pdrv_info->module_info.io_addr[ucloop] = ioremap_nocache(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		lvds_api("start 0x%08X  mapped to 0x%08X\n", (unsigned int)pdrv_info->presource[ucloop]->start, (unsigned int)pdrv_info->module_info.io_addr[ucloop]);
		if (pdrv_info->module_info.io_addr[ucloop] == NULL) {
			nvt_dbg(ERR, "ioremap() failed in module%d\n", ucloop);
			for (; ucloop > 0 ;) {
				ucloop -= 1;
				iounmap(pdrv_info->module_info.io_addr[ucloop]);
			}
			ret = -ENODEV;
			goto FAIL_FREE_RES;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_IRQ_NUM; ucloop++) {
		pdrv_info->module_info.iinterrupt_id[ucloop] = platform_get_irq(pdev, ucloop);
		nvt_dbg(IND, "IRQ %d. ID%d\n", ucloop, pdrv_info->module_info.iinterrupt_id[ucloop]);
		if (pdrv_info->module_info.iinterrupt_id[ucloop] < 0) {
			nvt_dbg(ERR, "No IRQ resource defined\n");
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	//Get clock source
	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++) {
		char  pstr1[16];

		snprintf(pstr1, 16, "%08x.lvds", pdrv_info->presource[ucloop]->start);

		pdrv_info->module_info.pclk[ucloop] = clk_get(&pdev->dev, pstr1);

		if (IS_ERR(pdrv_info->module_info.pclk[ucloop])) {
			nvt_dbg(ERR, "faile to get clock%d source (%s)\n", ucloop, pstr1);
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		nvt_dbg(ERR, "Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	nvt_dbg(IND, "DevID Major:%d minor:%d\n" \
			, MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_lvds_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, MODULE_MINOR_COUNT)) {
		nvt_dbg(ERR, "Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pdrv_info->pmodule_class = class_create(THIS_MODULE, MODULE_NAME);
	if (IS_ERR(pdrv_info->pmodule_class)) {
		nvt_dbg(ERR, "failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
									 , MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
									 , MODULE_NAME"%d", ucloop);

		if (IS_ERR(pdrv_info->pdevice[ucloop])) {
			nvt_dbg(ERR, "failed in creating device%d.\n", ucloop);

			for (; ucloop > 0 ; ucloop--) {
				device_unregister(pdrv_info->pdevice[ucloop - 1]);
			}
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

#if !defined(CONFIG_NVT_SMALL_HDAL)
	ret = nvt_lvds_proc_init(pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DEV;
	}
#endif

	ret = nvt_lvds_drv_init(&pdrv_info->module_info);

	platform_set_drvdata(pdev, pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
#if !defined(CONFIG_NVT_SMALL_HDAL)
	nvt_lvds_proc_remove(pdrv_info);

FAIL_DEV:
#endif

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
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	}

FAIL_FREE_BUF:
	kfree(pdrv_info);
	return ret;
}

static int nvt_lvds_remove(struct platform_device *pdev)
{
	PLVDS_DRV_INFO pdrv_info;
	unsigned char ucloop;

	nvt_dbg(IND, "\n");

	pdrv_info = platform_get_drvdata(pdev);

	nvt_lvds_drv_remove(&pdrv_info->module_info);

#if !defined(CONFIG_NVT_SMALL_HDAL)
	nvt_lvds_proc_remove(pdrv_info);
#endif

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		iounmap(pdrv_info->module_info.io_addr[ucloop]);
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	}

	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++) {
		clk_put(pdrv_info->module_info.pclk[ucloop]);
	}

	kfree(pdrv_info);
	return 0;
}

static int nvt_lvds_suspend(struct platform_device *pdev, pm_message_t state)
{
	PLVDS_DRV_INFO pdrv_info;

	nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_lvds_drv_suspend(&pdrv_info->module_info);

	nvt_dbg(IND, "finished\n");
	return 0;
}


static int nvt_lvds_resume(struct platform_device *pdev)
{
	PLVDS_DRV_INFO pdrv_info;

	nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_lvds_drv_resume(&pdrv_info->module_info);

	nvt_dbg(IND, "finished\n");
	return 0;
}

static struct platform_driver nvt_lvds_driver = {
	.driver = {
		.name   = "nvt_lvds",
		.owner = THIS_MODULE,
		.of_match_table = lvds_match_table,
	},
	.probe      = nvt_lvds_probe,
	.remove     = nvt_lvds_remove,
	.suspend = nvt_lvds_suspend,
	.resume = nvt_lvds_resume
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

int __init nvt_lvds_module_init(void)
{
	int ret;

	nvt_dbg(IND, "\n");

	ret = platform_driver_register(&nvt_lvds_driver);

	ssenif_hook_lvds((void *) lvds_get_drv_object(LVDS_ID_LVDS));
	ssenif_hook_lvds2((void *) lvds_get_drv_object(LVDS_ID_LVDS2));

	return 0;
}

void __exit nvt_lvds_module_exit(void)
{
	nvt_dbg(IND, "\n");

	platform_driver_unregister(&nvt_lvds_driver);

	ssenif_hook_lvds(NULL);
	ssenif_hook_lvds2(NULL);

}

module_init(nvt_lvds_module_init);
module_exit(nvt_lvds_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("lvds driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.00.000");

