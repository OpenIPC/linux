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

#include "ide_drv.h"
#include "ide_reg.h"
#include "ide_main.h"
#include "ide_proc.h"
#include "ide_dbg.h"
#include "dispctrl.h"
#include "ide.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int ide_debug_level = (NVT_DBG_ERR);
module_param_named(ide_debug_level, ide_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ide_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id ide_match_table[] = {
	{   .compatible = "nvt,nvt_ide"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_ide_open(struct inode *inode, struct file *file);
static int nvt_ide_release(struct inode *inode, struct file *file);
static long nvt_ide_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_ide_probe(struct platform_device *pdev);
static int nvt_ide_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_ide_resume(struct platform_device *pdev);
static int nvt_ide_remove(struct platform_device *pdev);
int __init nvt_ide_module_init(void);
void __exit nvt_ide_module_exit(void);
//=============================================================================
// function define
//=============================================================================
static int nvt_ide_open(struct inode *inode, struct file *file)
{
	IDE_DRV_INFO *pdrv_info;

	pdrv_info = container_of(inode->i_cdev, IDE_DRV_INFO, cdev);

	pdrv_info = container_of(inode->i_cdev, IDE_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (nvt_ide_drv_open(&pdrv_info->module_info, MINOR(inode->i_rdev))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_ide_release(struct inode *inode, struct file *file)
{
	IDE_DRV_INFO *pdrv_info;

	pdrv_info = container_of(inode->i_cdev, IDE_DRV_INFO, cdev);

	nvt_ide_drv_release(&pdrv_info->module_info, MINOR(inode->i_rdev));
	return 0;
}

static long nvt_ide_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PIDE_DRV_INFO pdrv;

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return nvt_ide_drv_ioctl(MINOR(inode->i_rdev), &pdrv->module_info, cmd, arg);
}

struct file_operations nvt_ide_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ide_open,
	.release = nvt_ide_release,
	.unlocked_ioctl = nvt_ide_ioctl,
	.llseek  = no_llseek,
};

static int nvt_ide_probe(struct platform_device *pdev)
{
	IDE_DRV_INFO *pdrv_info;//info;
	const struct of_device_id *match;
	int ret = 0;
	unsigned char ucloop;

	nvt_dbg(IND, "%s\n", pdev->name);

	match = of_match_device(ide_match_table, &pdev->dev);
	if (!match) {
		nvt_dbg(ERR, "Platform device not found \n");
		return -EINVAL;
	}

	pdrv_info = kzalloc(sizeof(IDE_DRV_INFO), GFP_KERNEL);
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
			for ( ; ucloop > 0; ) {
				ucloop -= 1;
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			}
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		pdrv_info->module_info.io_addr[ucloop] = ioremap_nocache(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (pdrv_info->module_info.io_addr[ucloop] == NULL) {
			nvt_dbg(ERR, "ioremap() failed in module%d\n", ucloop);
			//for ( ; ucloop > 0; ) {
				//ucloop -= 1;
			    //iounmap(pdrv_info->module_info.io_addr[ucloop]);
			//}
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
	for (ucloop = 0 ; ucloop < 1; ucloop++) {
		pdrv_info->module_info.pclk[ucloop] = clk_get(&pdev->dev, dev_name(&pdev->dev));
		if (IS_ERR(pdrv_info->module_info.pclk[ucloop])) {
			nvt_dbg(ERR, "faile to get clock%d source\n", ucloop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	//Get ide output interface clock
	for (ucloop = 0 ; ucloop < 1; ucloop++) {
		pdrv_info->module_info.ifclk[ucloop] = clk_get(&pdev->dev, "f0800000.ideif");
		if (IS_ERR(pdrv_info->module_info.ifclk[ucloop])) {
			nvt_dbg(ERR, "faile to get clock%d source\n", ucloop);

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

	nvt_dbg(IND, "dev_id Major:%d minor:%d\n" \
			, MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_ide_fops);
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

			for (; ucloop > 0 ; ucloop--)
				device_unregister(pdrv_info->pdevice[ucloop - 1]);
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	ret = nvt_ide_proc_init(pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DEV;
	}

	ret = nvt_ide_drv_init(&pdrv_info->module_info);

	platform_set_drvdata(pdev, pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in driver init.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	nvt_ide_proc_remove(pdrv_info);

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
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	}

FAIL_FREE_BUF:
	kfree(pdrv_info);
	pdrv_info = NULL;
	platform_set_drvdata(pdev, pdrv_info);
	return ret;
}

static int nvt_ide_remove(struct platform_device *pdev)
{
	PIDE_DRV_INFO pdrv_info;
	unsigned char ucloop;

	nvt_dbg(IND, "\n");

	pdrv_info = platform_get_drvdata(pdev);

	nvt_ide_drv_remove(&pdrv_info->module_info);

	nvt_ide_proc_remove(pdrv_info);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++)
		clk_put(pdrv_info->module_info.pclk[ucloop]);

	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++)
		clk_put(pdrv_info->module_info.ifclk[ucloop]);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		iounmap(pdrv_info->module_info.io_addr[ucloop]);
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	}

	kfree(pdrv_info);
	pdrv_info = NULL;
	platform_set_drvdata(pdev, pdrv_info);
	return 0;
}

static int nvt_ide_suspend(struct platform_device *pdev, pm_message_t state)
{
	PIDE_DRV_INFO pdrv_info;;

	nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ide_drv_suspend(&pdrv_info->module_info);

	nvt_dbg(IND, "finished\n");
	return 0;
}


static int nvt_ide_resume(struct platform_device *pdev)
{
	PIDE_DRV_INFO pdrv_info;;

	nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ide_drv_resume(&pdrv_info->module_info);

	nvt_dbg(IND, "finished\n");
	return 0;
}

static struct platform_driver nvt_ide_driver = {
	.driver = {
		.name   = "nvt_ide",
		.owner = THIS_MODULE,
		.of_match_table = ide_match_table,
	},
	.probe      = nvt_ide_probe,
	.remove     = nvt_ide_remove,
	.suspend = nvt_ide_suspend,
	.resume = nvt_ide_resume
};

int __init nvt_ide_module_init(void)
{
	int ret;

	nvt_dbg(WRN, "\n");
	ret = platform_driver_register(&nvt_ide_driver);

	return 0;
}

void __exit nvt_ide_module_exit(void)
{
	nvt_dbg(WRN, "\n");
	platform_driver_unregister(&nvt_ide_driver);
}

module_init(nvt_ide_module_init);
module_exit(nvt_ide_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("ide driver");
MODULE_LICENSE("GPL");
