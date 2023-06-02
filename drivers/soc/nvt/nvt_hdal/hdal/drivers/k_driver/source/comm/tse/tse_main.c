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

#include "tse_drv.h"
#include "tse_reg.h"
#include "tse_main.h"
#include "tse_proc.h"
#include "tse_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
extern unsigned int tse_debug_level;
module_param_named(tse_debug_level, tse_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(tse_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id tse_match_table[] = {
	{	.compatible = "nvt,nvt_tse"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_tse_open(struct inode *inode, struct file *file);
static int nvt_tse_release(struct inode *inode, struct file *file);
static long nvt_tse_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_tse_probe(struct platform_device *pdev);
static int nvt_tse_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_tse_resume(struct platform_device *pdev);
static int nvt_tse_remove(struct platform_device *pdev);
int __init nvt_tse_module_init(void);
void __exit nvt_tse_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int nvt_tse_open(struct inode *inode, struct file *file)
{
	TSE_DRV_INFO *pdrv_info;

	pdrv_info = container_of(vos_icdev(inode), TSE_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (nvt_tse_drv_open(&pdrv_info->module_info, MINOR(vos_irdev(inode)))) {
		DBG_ERR("failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_tse_release(struct inode *inode, struct file *file)
{
	TSE_DRV_INFO *pdrv_info;

	pdrv_info = container_of(vos_icdev(inode), TSE_DRV_INFO, cdev);
	nvt_tse_drv_release(&pdrv_info->module_info, MINOR(vos_irdev(inode)));
	return 0;
}

static long nvt_tse_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PTSE_DRV_INFO pdrv;

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return nvt_tse_drv_ioctl(MINOR(vos_irdev(inode)), &pdrv->module_info, cmd, arg);
}

struct file_operations nvt_tse_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_tse_open,
	.release = nvt_tse_release,
	.unlocked_ioctl = nvt_tse_ioctl,
	.llseek  = no_llseek,
};

static int nvt_tse_probe(struct platform_device *pdev)
{
	TSE_DRV_INFO *pdrv_info;
	const struct of_device_id *match;
	int ret = 0;
	int ucloop;

	DBG_IND("start %s\n", pdev->name);

	match = of_match_device(tse_match_table, &pdev->dev);
	if (!match) {
		DBG_ERR("Platform device not found \n");
		return -EINVAL;
	}

	pdrv_info = kzalloc(sizeof(TSE_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		DBG_ERR("failed to allocate memory\n");
		return -ENOMEM;
	}

	for (ucloop = 0 ; ucloop < TSE_REG_NUM ; ucloop++) {
		pdrv_info->presource[ucloop] = platform_get_resource(pdev, IORESOURCE_MEM, ucloop);
		if (pdrv_info->presource[ucloop] == NULL) {
			DBG_ERR("No IO memory resource defined:%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < TSE_REG_NUM ; ucloop++) {
		DBG_IND("%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (!request_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]), pdev->name)) {
			DBG_ERR("failed to request memory resource%d\n", ucloop);
			for (; ucloop >= 0 ; ucloop --) {
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			}
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < TSE_REG_NUM ; ucloop++) {
		pdrv_info->module_info.io_addr[ucloop] = ioremap_nocache(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (pdrv_info->module_info.io_addr[ucloop] == NULL) {
			DBG_ERR("ioremap() failed in module%d\n", ucloop);
			for (; ucloop >= 0 ; ucloop --) {
			    iounmap(pdrv_info->module_info.io_addr[ucloop]);
			}
			ret = -ENODEV;
			goto FAIL_FREE_RES;
		}
	}

	for (ucloop = 0 ; ucloop < TSE_IRQ_NUM; ucloop++) {
		pdrv_info->module_info.interrupt_id[ucloop] = platform_get_irq(pdev, ucloop);
		DBG_IND("IRQ %d. ID%d\n", ucloop, pdrv_info->module_info.interrupt_id[ucloop]);
		if (pdrv_info->module_info.interrupt_id[ucloop] < 0) {
			DBG_ERR("No IRQ resource defined\n");
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	//Get clock source
	for (ucloop = 0 ; ucloop < TSE_CLK_NUM; ucloop++) {
		pdrv_info->module_info.pclk[ucloop] = clk_get(&pdev->dev, dev_name(&pdev->dev));
		DBG_IND("clk %d. def rate = %ld\n", ucloop, clk_get_rate(pdrv_info->module_info.pclk[ucloop]));
		if (IS_ERR(pdrv_info->module_info.pclk[ucloop])) {
			DBG_ERR("faile to get clock%d source\n", ucloop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, TSE_MINOR_COUNT, TSE_NAME)) {
		DBG_ERR("Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	DBG_IND("DevID Major:%d minor:%d\n", MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_tse_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, TSE_MINOR_COUNT)) {
		DBG_ERR("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pdrv_info->pmodule_class = class_create(THIS_MODULE, TSE_NAME);
	if (IS_ERR(pdrv_info->pmodule_class)) {
		DBG_ERR("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (TSE_MINOR_COUNT) ; ucloop++) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
			, MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
			, TSE_NAME"%d", ucloop);

		if (IS_ERR(pdrv_info->pdevice[ucloop])) {
			DBG_ERR("failed in creating device%d.\n", ucloop);

			for (; ucloop >= 0; ucloop --)
				device_unregister(pdrv_info->pdevice[ucloop]);
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	ret = nvt_tse_proc_init(pdrv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DEV;
	}

	ret = nvt_tse_drv_init(&pdrv_info->module_info);

	platform_set_drvdata(pdev, pdrv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}
	DBG_IND("finished\n");

	return ret;

FAIL_DRV_INIT:
	nvt_tse_proc_remove(pdrv_info);

FAIL_DEV:
	for (ucloop = 0 ; ucloop < (TSE_MINOR_COUNT) ; ucloop++)
		device_unregister(pdrv_info->pdevice[ucloop]);

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, TSE_MINOR_COUNT);

FAIL_FREE_REMAP:
	for (ucloop = 0 ; ucloop < TSE_REG_NUM ; ucloop++)
		iounmap(pdrv_info->module_info.io_addr[ucloop]);

FAIL_FREE_RES:
	for (ucloop = 0 ; ucloop < TSE_REG_NUM ; ucloop++)
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));

FAIL_FREE_BUF:
	kfree(pdrv_info);

	return ret;
}

static int nvt_tse_remove(struct platform_device *pdev)
{
	PTSE_DRV_INFO pdrv_info;
	unsigned char ucloop;

	DBG_IND("start\n");

	pdrv_info = platform_get_drvdata(pdev);

	nvt_tse_drv_remove(&pdrv_info->module_info);

	nvt_tse_proc_remove(pdrv_info);

	for (ucloop = 0 ; ucloop < TSE_CLK_NUM; ucloop++) {
		clk_put(pdrv_info->module_info.pclk[ucloop]);
	}

	for (ucloop = 0 ; ucloop < (TSE_MINOR_COUNT) ; ucloop++)
		device_unregister(pdrv_info->pdevice[ucloop]);

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, TSE_MINOR_COUNT);

	for (ucloop = 0 ; ucloop < TSE_REG_NUM ; ucloop++)
	    iounmap(pdrv_info->module_info.io_addr[ucloop]);

	for (ucloop = 0 ; ucloop < TSE_REG_NUM ; ucloop++)
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));

	kfree(pdrv_info);

	DBG_IND("finished\n");
	return 0;
}

static int nvt_tse_suspend(struct platform_device *pdev, pm_message_t state)
{
	PTSE_DRV_INFO pdrv_info;

	DBG_IND("start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_tse_drv_suspend(&pdrv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}

static int nvt_tse_resume(struct platform_device *pdev)
{
	PTSE_DRV_INFO pdrv_info;;

	DBG_IND("start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_tse_drv_resume(&pdrv_info->module_info);

	DBG_IND("finished\n");
	return 0;
}

static struct platform_driver nvt_tse_driver = {
	.driver = {
				.name   = "nvt_tse",
				.owner = THIS_MODULE,
				.of_match_table = tse_match_table,
				},
	.probe = nvt_tse_probe,
	.remove = nvt_tse_remove,
	.suspend = nvt_tse_suspend,
	.resume = nvt_tse_resume
};

int __init nvt_tse_module_init(void)
{
	int ret;

	DBG_DUMP("%s init, ver: %s %s %s\n", TSE_NAME, TSE_VERSION, __DATE__, __TIME__);
	DBG_WRN("start\n");
	ret = platform_driver_register(&nvt_tse_driver);
	DBG_WRN("finished\n");
	return 0;
}

void __exit nvt_tse_module_exit(void)
{
	DBG_WRN("start\n");
	platform_driver_unregister(&nvt_tse_driver);
	DBG_WRN("finished\n");
}

module_init(nvt_tse_module_init);
module_exit(nvt_tse_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("tse driver");
MODULE_LICENSE("GPL");
