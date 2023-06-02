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
#include "kdrv_ime_int_main.h"
#include "kdrv_ime_int_proc.h"
#include "kdrv_ime_int_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================

//=============================================================================
// Global variable
//=============================================================================
#if 0
static struct of_device_id kdrv_ime_match_table[] = {
	{   .compatible = "nvt,kdrv_ime"},
	{}
};
#endif

//=============================================================================
// function declaration
//=============================================================================
#if 0
static int nvt_kdrv_ime_open(struct inode *inode, struct file *file);
static int nvt_kdrv_ime_release(struct inode *inode, struct file *file);
static long nvt_kdrv_ime_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_kdrv_ime_probe(struct platform_device *pdev);
static int nvt_kdrv_ime_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_kdrv_ime_resume(struct platform_device *pdev);
static int nvt_kdrv_ime_remove(struct platform_device *pdev);
#endif
int __init nvt_kdrv_ime_module_init(void);
void __exit nvt_kdrv_ime_module_exit(void);

#if 0
#endif
//=============================================================================
// function define
//=============================================================================
#if 0
static int nvt_kdrv_ime_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int nvt_kdrv_ime_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long nvt_kdrv_ime_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return 0;
}

struct file_operations nvt_kdrv_ime_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_kdrv_ime_open,
	.release = nvt_kdrv_ime_release,
	.unlocked_ioctl = nvt_kdrv_ime_ioctl,
	.llseek  = no_llseek,
};
#endif

#if 0
#endif

#if 0
static int nvt_kdrv_ime_probe_get_resource(struct platform_device *p_dev, NVT_KDRV_IME_DRV_INFO *p_drv_info, int idx)
{
	NVT_KDRV_IME_RESOURCE dev_resource;
	struct resource *p_resource;
	int ret = 0;

	memset((void *)&dev_resource, 0, sizeof(NVT_KDRV_IME_RESOURCE));

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	p_drv_info->p_device[idx] = device_create(p_drv_info->pclass, NULL
									 , MKDEV(MAJOR(p_drv_info->dev_id), (idx + MINOR(p_drv_info->dev_id))), NULL
									 , NVT_KDRV_IME_NAME"%d", idx);

	if (IS_ERR(p_drv_info->p_device[idx])) {
		DBG_ERR("failed in creating device%d.\n", (int)idx);
		ret = -ENODEV;
		goto FAIL_NONE;
	}

	/* get resource */
	p_resource = platform_get_resource(p_dev, IORESOURCE_MEM, idx);
	if (p_resource == NULL) {
		DBG_ERR("No IO memory resource defined:%d\n", (int)idx);
		ret = -ENODEV;
		goto FAIL_DEVICE_UNREG;
	}

	if (!request_mem_region(p_resource->start, resource_size(p_resource), p_dev->name)) {
		DBG_ERR("failed to request memory resource%d\n", (int)idx);
		ret = -ENODEV;
		goto FAIL_DEVICE_UNREG;
	}

	dev_resource.phy_io_base = p_resource->start;
	dev_resource.phy_io_size = resource_size(p_resource);
	dev_resource.p_io_addr = ioremap_nocache(dev_resource.phy_io_base, dev_resource.phy_io_size);
	if (dev_resource.p_io_addr == NULL) {
		DBG_ERR("ioremap() failed in module%d\n", (int)idx);
		ret = -ENODEV;
		goto FAIL_RELEASE_MEM;
	}

	/* get irq id */
	#if 0
	dev_resource.irq_id = platform_get_irq(p_dev, idx);
	if (dev_resource.irq_id < 0) {
		DBG_ERR("No IRQ resource defined\n");
		ret = -ENODEV;
		goto FAIL_IOUNMAP;
	}
	#endif

	/* get clock resource */
	dev_resource.p_mclk = clk_get(&p_dev->dev, dev_name(&p_dev->dev));
	if (IS_ERR(dev_resource.p_mclk)) {
		DBG_ERR("faile to get clock%d source\n", (int)idx);
		ret = -ENODEV;
		goto FAIL_IOUNMAP;
	}

	/* set resource */
	kdrv_ime_resource_set(&dev_resource, idx);

	return ret;

FAIL_IOUNMAP:
	iounmap(dev_resource.p_io_addr);

FAIL_RELEASE_MEM:
	release_mem_region(p_resource->start, resource_size(p_resource));

FAIL_DEVICE_UNREG:
	device_unregister(p_drv_info->p_device[idx]);

FAIL_NONE:
	return ret;
}

static int nvt_kdrv_ime_remove_rls_resource(struct platform_device *p_dev, NVT_KDRV_IME_DRV_INFO *p_drv_info, int idx)
{
	NVT_KDRV_IME_RESOURCE *p_dev_resource;

	p_dev_resource = kdrv_ime_resource_get(idx);
	if (p_dev_resource) {
		release_mem_region(p_dev_resource->phy_io_base, p_dev_resource->phy_io_size);
		iounmap(p_dev_resource->p_io_addr);
		clk_put(p_dev_resource->p_mclk);
	}
	device_unregister(p_drv_info->p_device[idx]);

	return 0;
}

static int nvt_kdrv_ime_probe(struct platform_device *pdev)
{
	NVT_KDRV_IME_DRV_INFO *pdrv_info = NULL;//info;
	const struct of_device_id *match;
	int ret = 0;
	unsigned char ucloop;

	match = of_match_device(kdrv_ime_match_table, &pdev->dev);
	if (!match) {
		DBG_ERR("Platform device not found \n");
		return -EINVAL;
	}

	pdrv_info = kzalloc(sizeof(NVT_KDRV_IME_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		DBG_ERR("failed to allocate memory\n");
		return -ENOMEM;
	}

	/* Dynamic to allocate Device ID */
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, NVT_KDRV_IME_TOTAL_CH_COUNT, NVT_KDRV_IME_NAME)) {
		DBG_ERR("Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_BUF;
	}

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_kdrv_ime_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, NVT_KDRV_IME_TOTAL_CH_COUNT)) {
		DBG_ERR("Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	pdrv_info->pclass = class_create(THIS_MODULE, "nvt_kdrv_ime");
	if (IS_ERR(pdrv_info->pclass)) {
		DBG_ERR("failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	ret = nvt_kdrv_ime_proc_init(pdrv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		goto FAIL_CLASS;
	}

	/* alloc kdrv_ime_init memory */
	kdrv_ime_resource_init();

	/* request resources */
	for (ucloop = 0 ; ucloop < NVT_KDRV_IME_TOTAL_CH_COUNT; ucloop++) {
		ret = nvt_kdrv_ime_probe_get_resource(pdev, pdrv_info, ucloop);
		if (ret) {
			while (ucloop > 0) {
				ucloop--;
				nvt_kdrv_ime_remove_rls_resource(pdev, pdrv_info, ucloop);
			}
			ret = -ENODEV;
			goto FAIL_PROC;
		}
	}
	platform_set_drvdata(pdev, pdrv_info);

	return ret;

FAIL_PROC:
	nvt_kdrv_ime_proc_remove(pdrv_info);

FAIL_CLASS:
	class_destroy(pdrv_info->pclass);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, NVT_KDRV_IME_MINOR_COUNT);

FAIL_FREE_BUF:
	if (pdrv_info != NULL) {
		kfree(pdrv_info);

		// coverity[assigned_pointer]
		pdrv_info = NULL;
	}

	return ret;
}

static int nvt_kdrv_ime_remove(struct platform_device *pdev)
{
	PNVT_KDRV_IME_DRV_INFO pdrv_info = NULL;
	unsigned char ucloop;

	pdrv_info = platform_get_drvdata(pdev);

	for (ucloop = 0 ; ucloop < NVT_KDRV_IME_TOTAL_CH_COUNT; ucloop++) {
		nvt_kdrv_ime_remove_rls_resource(pdev, pdrv_info, ucloop);
	}
	kdrv_ime_resource_uninit();

	nvt_kdrv_ime_proc_remove(pdrv_info);
	class_destroy(pdrv_info->pclass);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, NVT_KDRV_IME_TOTAL_CH_COUNT);

	// coverity[check_after_deref]
	if (pdrv_info != NULL) {
		kfree(pdrv_info);

		// coverity[assigned_pointer]
		pdrv_info = NULL;
	}

	return 0;
}

static int nvt_kdrv_ime_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}


static int nvt_kdrv_ime_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver nvt_kdrv_ime_driver = {
	.driver = {
		.name = "kdrv_ime",
		.owner = THIS_MODULE,
		.of_match_table = kdrv_ime_match_table,
	},
	.probe = nvt_kdrv_ime_probe,
	.remove = nvt_kdrv_ime_remove,
	.suspend = nvt_kdrv_ime_suspend,
	.resume = nvt_kdrv_ime_resume
};
#endif

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

NVT_KDRV_IME_DRV_INFO *pdrv_info = NULL;
int __init nvt_kdrv_ime_module_init(void)
{
	int ret;

	//platform_driver_register(&nvt_kdrv_ime_driver);
	pdrv_info = kzalloc(sizeof(NVT_KDRV_IME_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		DBG_ERR("failed to allocate memory\n");
		return -ENOMEM;
	}
	ret = nvt_kdrv_ime_proc_init(pdrv_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");
		kfree(pdrv_info);
	}

	return ret;
}

void __exit nvt_kdrv_ime_module_exit(void)
{
	//platform_driver_unregister(&nvt_kdrv_ime_driver);
	nvt_kdrv_ime_proc_remove(pdrv_info);
	kfree(pdrv_info);
}

module_init(nvt_kdrv_ime_module_init);
module_exit(nvt_kdrv_ime_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("ime k-driver");
MODULE_LICENSE("GPL");
