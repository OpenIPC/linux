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

#include "ddr_arb_platform.h"
#include "ddr_arb_drv.h"
#include "ddr_arb_reg.h"
#include "ddr_arb_main.h"
#include "ddr_arb_proc.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int ddr_debug_level = NVT_DBG_ERR;
module_param_named(ddr_debug_level, ddr_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ddr_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id xxx_match_table[] = {
	{	.compatible = "nvt,nvt_arb"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_ddr_arb_open(struct inode *inode, struct file *file);
static int nvt_ddr_arb_release(struct inode *inode, struct file *file);
static long nvt_ddr_arb_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_ddr_arb_probe(struct platform_device *pdev);
static int nvt_ddr_arb_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_ddr_arb_resume(struct platform_device *pdev);
static int nvt_ddr_arb_remove(struct platform_device *pdev);
int __init nvt_ddr_arb_module_init(void);
void __exit nvt_ddr_arb_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int nvt_ddr_arb_open(struct inode *inode, struct file *file)
{
	XXX_DRV_INFO *pdrv_info;

	pdrv_info = container_of(inode->i_cdev, XXX_DRV_INFO, cdev);

	pdrv_info = container_of(inode->i_cdev, XXX_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (nvt_ddr_arb_drv_open(&pdrv_info->module_info, MINOR(inode->i_rdev))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_ddr_arb_release(struct inode *inode, struct file *file)
{
	XXX_DRV_INFO *pdrv_info;

	pdrv_info = container_of(inode->i_cdev, XXX_DRV_INFO, cdev);
	nvt_ddr_arb_drv_release(&pdrv_info->module_info, MINOR(inode->i_rdev));
	return 0;
}

static long nvt_ddr_arb_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PXXX_DRV_INFO pdrv;

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return nvt_ddr_arb_drv_ioctl(MINOR(inode->i_rdev), &pdrv->module_info, cmd, arg);
}

struct file_operations nvt_ddr_arb_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ddr_arb_open,
	.release = nvt_ddr_arb_release,
	.unlocked_ioctl = nvt_ddr_arb_ioctl,
	.llseek  = no_llseek,
};

static int nvt_ddr_arb_probe(struct platform_device *pdev)
{
	XXX_DRV_INFO *pdrv_info;
	const struct of_device_id *match;
	int ret = 0;
	int loop;

	nvt_dbg(IND, "%s\n", pdev->name);

	match = of_match_device(xxx_match_table, &pdev->dev);
	if (!match) {
		nvt_dbg(ERR, "Platform device not found \n");
		return -EINVAL;
	}

	pdrv_info = kzalloc(sizeof(XXX_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		nvt_dbg(ERR, "failed to allocate memory\n");
		return -ENOMEM;
	}

	for (loop = 0 ; loop < MODULE_REG_NUM ; loop++) {
		pdrv_info->presource[loop] = platform_get_resource(pdev, IORESOURCE_MEM, loop);
		if (pdrv_info->presource[loop] == NULL) {
			nvt_dbg(ERR, "No IO memory resource defined:%d\n", loop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (loop = 0 ; loop < MODULE_REG_NUM ; loop++) {
		nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", loop, pdrv_info->presource[loop]->start, resource_size(pdrv_info->presource[loop]));
		if (!request_mem_region(pdrv_info->presource[loop]->start, resource_size(pdrv_info->presource[loop]), pdev->name)) {
			nvt_dbg(ERR, "failed to request memory resource%d\n", loop);
#if (MODULE_REG_NUM > 1)
			for (; loop > 0 ;) {
				loop -= 1;
				release_mem_region(pdrv_info->presource[loop]->start, resource_size(pdrv_info->presource[loop]));
			}
#endif
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (loop = 0 ; loop < MODULE_REG_NUM ; loop++) {
		pdrv_info->module_info.io_addr[loop] = ioremap_nocache(pdrv_info->presource[loop]->start, resource_size(pdrv_info->presource[loop]));
		if (pdrv_info->module_info.io_addr[loop] == NULL) {
			nvt_dbg(ERR, "ioremap() failed in module%d\n", loop);
#if (MODULE_REG_NUM > 1)
			for (; loop > 0 ;) {
				loop -= 1;
			    iounmap(pdrv_info->module_info.io_addr[loop]);
			}
#endif
			ret = -ENODEV;
			goto FAIL_FREE_RES;
		}
	}

	for (loop = 0 ; loop < MODULE_IRQ_NUM; loop++) {
		pdrv_info->module_info.iinterrupt_id[loop] = platform_get_irq(pdev, loop);
		nvt_dbg(IND, "IRQ %d. ID%d\n", loop, pdrv_info->module_info.iinterrupt_id[loop]);
		if (pdrv_info->module_info.iinterrupt_id[loop] < 0) {
			nvt_dbg(ERR, "No IRQ resource defined\n");
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	//Get clock source
#if 0
	for (loop = 0 ; loop < MODULE_CLK_NUM; loop++) {
		nvt_dbg(ERR, "try clk name %s\n", dev_name(&pdev->dev));
		pdrv_info->module_info.pclk[loop] = clk_get(&pdev->dev, dev_name(&pdev->dev));
		if (IS_ERR(pdrv_info->module_info.pclk[loop])) {
			nvt_dbg(ERR, "faile to get clock%d source\n", loop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}
#endif

	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		nvt_dbg(ERR, "Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	nvt_dbg(IND, "DevID Major:%d minor:%d\n", MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_ddr_arb_fops);
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
	for (loop = 0 ; loop < (MODULE_MINOR_COUNT) ; loop++) {
		pdrv_info->pdevice[loop] = device_create(pdrv_info->pmodule_class, NULL
			, MKDEV(MAJOR(pdrv_info->dev_id), (loop + MINOR(pdrv_info->dev_id))), NULL
			, MODULE_NAME"%d", loop);

		if (IS_ERR(pdrv_info->pdevice[loop])) {
			nvt_dbg(ERR, "failed in creating device%d.\n", loop);

#if (MODULE_MINOR_COUNT > 1)
			for (; loop > 0 ; loop--) {
				device_unregister(pdrv_info->pdevice[loop - 1]);
			}
#endif
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	ret = nvt_ddr_arb_proc_init(pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DEV;
	}

	ret = nvt_ddr_arb_drv_init(&pdrv_info->module_info);

	platform_set_drvdata(pdev, pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating drvdata.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	nvt_ddr_arb_proc_remove(pdrv_info);

FAIL_DEV:
	for (loop = 0 ; loop < (MODULE_MINOR_COUNT) ; loop++)
		device_unregister(pdrv_info->pdevice[loop]);

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

FAIL_FREE_REMAP:
	for (loop = 0 ; loop < MODULE_REG_NUM ; loop++)
		iounmap(pdrv_info->module_info.io_addr[loop]);

FAIL_FREE_RES:
	for (loop = 0 ; loop < MODULE_REG_NUM ; loop++)
		release_mem_region(pdrv_info->presource[loop]->start, resource_size(pdrv_info->presource[loop]));

FAIL_FREE_BUF:
	kfree(pdrv_info);
	pdrv_info = NULL;
	return ret;
}

static int nvt_ddr_arb_remove(struct platform_device *pdev)
{
	PXXX_DRV_INFO pdrv_info;
	unsigned char ucloop;

	nvt_dbg(IND, "\n");

	pdrv_info = platform_get_drvdata(pdev);

	nvt_ddr_arb_drv_remove(&pdrv_info->module_info);

	nvt_ddr_arb_proc_remove(pdrv_info);

	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++)
		clk_put(pdrv_info->module_info.pclk[ucloop]);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++)
		device_unregister(pdrv_info->pdevice[ucloop]);

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
	    iounmap(pdrv_info->module_info.io_addr[ucloop]);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));

	kfree(pdrv_info);
	pdrv_info = NULL;
	return 0;
}

static int nvt_ddr_arb_suspend(struct platform_device *pdev, pm_message_t state)
{
	PXXX_DRV_INFO pdrv_info;;

	nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ddr_arb_drv_suspend(&pdrv_info->module_info);

	nvt_dbg(IND, "finished\n");
	return 0;
}


static int nvt_ddr_arb_resume(struct platform_device *pdev)
{
	PXXX_DRV_INFO pdrv_info;;

	nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ddr_arb_drv_resume(&pdrv_info->module_info);

	nvt_dbg(IND, "finished\n");
	return 0;
}

static struct platform_driver nvt_ddr_arb_driver = {
	.driver = {
				.name   = "nvt_arb",
				.owner = THIS_MODULE,
				.of_match_table = xxx_match_table,
				},
	.probe      = nvt_ddr_arb_probe,
	.remove     = nvt_ddr_arb_remove,
	.suspend = nvt_ddr_arb_suspend,
	.resume = nvt_ddr_arb_resume
};

int __init nvt_ddr_arb_module_init(void)
{
	int ret;

	nvt_dbg(WRN, "\n");

	ret = platform_driver_register(&nvt_ddr_arb_driver);

	return 0;
}

void __exit nvt_ddr_arb_module_exit(void)
{
	nvt_dbg(WRN, "\n");
	platform_driver_unregister(&nvt_ddr_arb_driver);
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

module_init(nvt_ddr_arb_module_init);
module_exit(nvt_ddr_arb_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("nvt ddr arbiter driver");
MODULE_LICENSE("GPL");
