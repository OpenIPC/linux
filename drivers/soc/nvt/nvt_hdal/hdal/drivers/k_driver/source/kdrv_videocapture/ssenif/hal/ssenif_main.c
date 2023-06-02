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

#include "ssenif_drv.h"
#include "ssenif_reg.h"
#include "ssenif_main.h"
#include "ssenif_proc.h"
#include "ssenif_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int ssenif_debug_level = NVT_DBG_WRN;
module_param_named(ssenif_debug_level, ssenif_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ssenif_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static SSENIF_DRV_INFO *pssenif_drv_info;

//=============================================================================
// function declaration
//=============================================================================
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

int __init nvt_ssenif_module_init(void);
void __exit nvt_ssenif_module_exit(void);


#if 0
static struct of_device_id ssenif_match_table[] = {
	{   .compatible = "nvt,nvt_ssenif"},
	{}
};

static int nvt_ssenif_open(struct inode *inode, struct file *file);
static int nvt_ssenif_release(struct inode *inode, struct file *file);
static long nvt_ssenif_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_ssenif_probe(struct platform_device *pdev);
static int nvt_ssenif_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_ssenif_resume(struct platform_device *pdev);
static int nvt_ssenif_remove(struct platform_device *pdev);
//=============================================================================
// function define
//=============================================================================
static int nvt_ssenif_open(struct inode *inode, struct file *file)
{
	SSENIF_DRV_INFO *pdrv_info;

	ssenif_api("%s\n", __func__);

	pdrv_info = container_of(inode->i_cdev, SSENIF_DRV_INFO, cdev);

	pdrv_info = container_of(inode->i_cdev, SSENIF_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (nvt_ssenif_drv_open(&pdrv_info->module_info, MINOR(inode->i_rdev))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_ssenif_release(struct inode *inode, struct file *file)
{
	SSENIF_DRV_INFO *pdrv_info;

	ssenif_api("%s\n", __func__);

	pdrv_info = container_of(inode->i_cdev, SSENIF_DRV_INFO, cdev);

	nvt_ssenif_drv_release(&pdrv_info->module_info, MINOR(inode->i_rdev));
	return 0;
}

static long nvt_ssenif_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PSSENIF_DRV_INFO pdrv;

	ssenif_api("%s\n", __func__);

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return nvt_ssenif_drv_ioctl(MINOR(inode->i_rdev), &pdrv->module_info, cmd, arg);
}

struct file_operations nvt_ssenif_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ssenif_open,
	.release = nvt_ssenif_release,
	.unlocked_ioctl = nvt_ssenif_ioctl,
	.llseek  = no_llseek,
};

static int nvt_ssenif_probe(struct platform_device *pdev)
{
	SSENIF_DRV_INFO *pdrv_info;//info;
	const struct of_device_id *match;
	int ret = 0;
	unsigned char ucloop;

	nvt_dbg(IND, "%s\n", pdev->name);

	ssenif_api("%s\n", __func__);

	match = of_match_device(ssenif_match_table, &pdev->dev);
	if (!match) {
		printk("Platform device not found \n");
		return -EINVAL;
	}

	pdrv_info = kzalloc(sizeof(SSENIF_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		printk("failed to allocate memory\n");
		return -ENOMEM;
	}

	/*
	    Allocate REGISTER space
	*/
	for (ucloop = 0 ; ucloop < SLVSEC_REG_NUM ; ucloop++) {
		pdrv_info->slvsec_presource[ucloop] = platform_get_resource(pdev, IORESOURCE_MEM, ucloop);
		if (pdrv_info->slvsec_presource[ucloop] == NULL) {
			printk("No IO memory resource defined:%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < SLVSEC_REG_NUM ; ucloop++) {
		nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->slvsec_presource[ucloop]->start, resource_size(pdrv_info->slvsec_presource[ucloop]));
		if (!request_mem_region(pdrv_info->slvsec_presource[ucloop]->start, resource_size(pdrv_info->slvsec_presource[ucloop]), pdev->name)) {
			printk("failed to request memory resource%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < SLVSEC_REG_NUM ; ucloop++) {
		pdrv_info->module_info.slvsec_io_addr[ucloop] = ioremap_nocache(pdrv_info->slvsec_presource[ucloop]->start, resource_size(pdrv_info->slvsec_presource[ucloop]));
		ssenif_api("start 0x%08X  mapped to 0x%08X\n", (unsigned int)pdrv_info->slvsec_presource[ucloop]->start, (unsigned int)pdrv_info->module_info.slvsec_io_addr[ucloop]);
		if (pdrv_info->module_info.slvsec_io_addr[ucloop] == NULL) {
			printk("ioremap() failed in module%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_RES;
		}
	}


	/*
	    Allocate IRQ
	*/
	for (ucloop = 0 ; ucloop < SLVSEC_IRQ_NUM; ucloop++) {
		pdrv_info->module_info.slvsec_iinterrupt_id[ucloop] = platform_get_irq(pdev, ucloop);
		nvt_dbg(IND, "IRQ %d. ID%d\n", ucloop, pdrv_info->module_info.slvsec_iinterrupt_id[ucloop]);
		if (pdrv_info->module_info.slvsec_iinterrupt_id[ucloop] < 0) {
			nvt_dbg(ERR, "No IRQ resource defined\n");
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	/*
	    Allocate CLOCKs
	*/
	for (ucloop = 0 ; ucloop < SLVSEC_CLK_NUM; ucloop++) {
		char  pstr1[16];

		snprintf(pstr1, 16, "%08x.slvsec", pdrv_info->slvsec_presource[0]->start);

		pdrv_info->module_info.slvsec_pclk[ucloop] = clk_get(&pdev->dev, pstr1);

		if (IS_ERR(pdrv_info->module_info.slvsec_pclk[ucloop])) {
			nvt_dbg(ERR, "faile to get clock%d source (%s)\n", ucloop, pstr1);
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
		nvt_dbg(IND, "CLK %d. Name %s\n", ucloop, pstr1);
	}



	/*
	    char dev register
	*/
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, SLVSEC_MINOR_COUNT, SLVSEC_NAME)) {
		printk("Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	nvt_dbg(IND, "DevID Major:%d minor:%d\n" \
			, MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));



	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_ssenif_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, SLVSEC_MINOR_COUNT)) {
		printk("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pdrv_info->pmodule_class = class_create(THIS_MODULE, SLVSEC_NAME);
	if (IS_ERR(pdrv_info->pmodule_class)) {
		printk("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (SLVSEC_MINOR_COUNT) ; ucloop++) {
		pdrv_info->slvsec_pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
											, MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
											, SLVSEC_NAME"%d", ucloop);

		if (IS_ERR(pdrv_info->slvsec_pdevice[ucloop])) {
			printk("failed in creating device%d.\n", ucloop);

			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

	ret = nvt_ssenif_proc_init(pdrv_info);
	if (ret) {
		printk("failed in creating proc.\n");
		goto FAIL_DEV;
	}

	ret = nvt_ssenif_drv_init(&pdrv_info->module_info);

	platform_set_drvdata(pdev, pdrv_info);
	if (ret) {
		printk("failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	nvt_ssenif_proc_remove(pdrv_info);

FAIL_DEV:
	for (ucloop = 0 ; ucloop < (SLVSEC_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->slvsec_pdevice[ucloop]);
	}

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, SLVSEC_MINOR_COUNT);

FAIL_FREE_REMAP:
	for (ucloop = 0 ; ucloop < SLVSEC_REG_NUM ; ucloop++) {
		iounmap(pdrv_info->module_info.slvsec_io_addr[ucloop]);
	}

FAIL_FREE_RES:
	for (ucloop = 0 ; ucloop < SLVSEC_REG_NUM ; ucloop++) {
		release_mem_region(pdrv_info->slvsec_presource[ucloop]->start, resource_size(pdrv_info->slvsec_presource[ucloop]));
	}

FAIL_FREE_BUF:
	kfree(pdrv_info);
	return ret;
}

static int nvt_ssenif_remove(struct platform_device *pdev)
{
	PSSENIF_DRV_INFO pdrv_info;
	unsigned char ucloop;

	nvt_dbg(IND, "\n");

	ssenif_api("%s\n", __func__);

	pdrv_info = platform_get_drvdata(pdev);

	nvt_ssenif_drv_remove(&pdrv_info->module_info);

	nvt_ssenif_proc_remove(pdrv_info);

	for (ucloop = 0 ; ucloop < (SLVSEC_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->slvsec_pdevice[ucloop]);
	}

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, SLVSEC_MINOR_COUNT);

	for (ucloop = 0 ; ucloop < SLVSEC_REG_NUM ; ucloop++) {
		iounmap(pdrv_info->module_info.slvsec_io_addr[ucloop]);
	}

	for (ucloop = 0 ; ucloop < SLVSEC_REG_NUM ; ucloop++) {
		release_mem_region(pdrv_info->slvsec_presource[ucloop]->start, resource_size(pdrv_info->slvsec_presource[ucloop]));
	}

	for (ucloop = 0 ; ucloop < SLVSEC_CLK_NUM; ucloop++) {
		clk_put(pdrv_info->module_info.slvsec_pclk[ucloop]);
	}

	kfree(pdrv_info);
	return 0;
}

static int nvt_ssenif_suspend(struct platform_device *pdev, pm_message_t state)
{
	PSSENIF_DRV_INFO pdrv_info;

	nvt_dbg(IND, "start\n");

	ssenif_api("%s\n", __func__);

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ssenif_drv_suspend(&pdrv_info->module_info);

	nvt_dbg(IND, "finished\n");
	return 0;
}

static int nvt_ssenif_resume(struct platform_device *pdev)
{
	PSSENIF_DRV_INFO pdrv_info;

	nvt_dbg(IND, "start\n");

	ssenif_api("%s\n", __func__);

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ssenif_drv_resume(&pdrv_info->module_info);

	nvt_dbg(IND, "finished\n");
	return 0;
}
#endif

static struct platform_driver nvt_ssenif_driver = {
	.driver = {
		.name   = "nvt_ssenif",
		.owner = THIS_MODULE,
	},
};

int __init nvt_ssenif_module_init(void)
{
	int ret;

	nvt_dbg(IND, "\n");

	pssenif_drv_info = kzalloc(sizeof(SSENIF_DRV_INFO), GFP_KERNEL);
	if (!pssenif_drv_info) {
		printk("failed to allocate memory\n");
	}

#if !defined(CONFIG_NVT_SMALL_HDAL)
	ret = nvt_ssenif_proc_init(pssenif_drv_info);
	if (ret) {
		printk("failed in creating proc.\n");
	}
#endif

	ret = platform_driver_register(&nvt_ssenif_driver);

	nvt_dbg(IND, "ssenif module_init\n");
	return ret;
}

void __exit nvt_ssenif_module_exit(void)
{
	nvt_dbg(IND, "\n");

	platform_driver_unregister(&nvt_ssenif_driver);

#if !defined(CONFIG_NVT_SMALL_HDAL)
	nvt_ssenif_proc_remove(pssenif_drv_info);
#endif

	kfree(pssenif_drv_info);
	pssenif_drv_info = NULL;

	nvt_dbg(IND, "ssenif module_exit\n");
}

module_init(nvt_ssenif_module_init);
module_exit(nvt_ssenif_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("kdrv ssenif driver");
MODULE_LICENSE("GPL");
