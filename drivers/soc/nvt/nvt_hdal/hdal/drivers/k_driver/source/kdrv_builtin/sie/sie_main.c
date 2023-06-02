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
#include "kwrap/debug.h"
#include "kwrap/type.h"
#include "sie_drv.h"
#include "sie_main.h"
#include "sie_platform.h"
#include <kdrv_builtin/kdrv_builtin.h>

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int sie_debug_level = (NVT_DBG_IND | NVT_DBG_WRN | NVT_DBG_ERR);
module_param_named(sie_debug_level, sie_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sie_debug_level, "Debug message level");
#endif
//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id sie_match_table[] = {
	{	.compatible = "nvt,drv_sie"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_sie_open(struct inode *inode, struct file *file);
static int nvt_sie_release(struct inode *inode, struct file *file);
static int nvt_sie_probe(struct platform_device * pdev);
static int nvt_sie_suspend( struct platform_device * pdev, pm_message_t state);
static int nvt_sie_resume( struct platform_device * pdev);
static int nvt_sie_remove(struct platform_device *pdev);
int __init nvt_sie_module_init(void);
void __exit nvt_sie_module_exit(void);
UINT32 sie_power_saving_mode = FALSE;

#if 0
#endif
//=============================================================================
// function define
//=============================================================================
static int nvt_sie_open(struct inode *inode, struct file *file)
{
	SIE_DRV_INFO* pdrv_info = container_of(vos_icdev(inode), SIE_DRV_INFO, cdev);

	DBG_ERR("\r\n\r\n --> nvt_sie_open \r\n\r\n");
	file->private_data = pdrv_info;
	if (nvt_sie_drv_open(&pdrv_info->module_info, MINOR(vos_irdev(inode)))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}
    return 0;
}

static int nvt_sie_release(struct inode *inode, struct file *file)
{
	SIE_DRV_INFO* pdrv_info = container_of(vos_icdev(inode), SIE_DRV_INFO, cdev);

	return nvt_sie_drv_release(&pdrv_info->module_info, MINOR(vos_irdev(inode)));
}

struct file_operations nvt_sie_fops = {
    .owner   = THIS_MODULE,
    .open    = nvt_sie_open,
    .release = nvt_sie_release,
    // Note: platform combination
    //.unlocked_ioctl = nvt_sie_ioctl,
    .llseek  = no_llseek,
};

static int nvt_sie_probe(struct platform_device * pdev)
{
    SIE_DRV_INFO* pdrv_info;//info;
	const struct of_device_id *match;
    int rt = 0;
	unsigned char ucloop;

	//ret = nvtmem_version_register(pdev, "0.20.55.11");
    //nvt_dbg(IND, "%s\n", pdev->name);
	match = of_match_device(sie_match_table, &pdev->dev);
	if (!match) {
        nvt_dbg(ERR, "Platform device not found \n");
		return -EINVAL;
	}

    pdrv_info = kzalloc(sizeof(SIE_DRV_INFO), GFP_KERNEL);
    if (!pdrv_info) {
        nvt_dbg(ERR, "failed to allocate memory\n");
        return -ENOMEM;
    }

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		pdrv_info->presource[ucloop] = platform_get_resource(pdev, IORESOURCE_MEM, ucloop);
		if (pdrv_info->presource[ucloop] == NULL) {
			nvt_dbg(ERR, "No IO memory resource defined:%d\n", ucloop);
			rt = -1;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		//nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (!request_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]), pdev->name)) {
			nvt_dbg(ERR, "failed to request memory resource%d\n", ucloop);
			for (; ucloop > 0 ;) {
				ucloop -= 1;
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			}
			rt = -1;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		pdrv_info->module_info.io_addr[ucloop] = ioremap_nocache(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (pdrv_info->module_info.io_addr[ucloop] == NULL) {
			nvt_dbg(ERR, "ioremap() failed in module%d\n", ucloop);
			//while (ucloop >= 0) {
			    iounmap(pdrv_info->module_info.io_addr[ucloop]);
			//	ucloop -= 1;
			//}
			rt = -1;
			goto FAIL_FREE_RES;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_IRQ_NUM; ucloop++) {
		pdrv_info->module_info.iinterrupt_id[ucloop] = platform_get_irq(pdev, ucloop);
		//nvt_dbg(IND, "IRQ %d. ID%d\n", ucloop,pdrv_info->module_info.iinterrupt_id[ucloop]);
		if (pdrv_info->module_info.iinterrupt_id[ucloop] < 0) {
			nvt_dbg(ERR, "No IRQ resource defined\n");
			rt = -1;
			goto FAIL_FREE_REMAP;
		}
	}

	//Get clock source
	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++) {
		char pstr1[64];
		//pdrv_info->module_info.pclk[ucloop] = clk_get(&pdev->dev, dev_name(&pdev->dev));
		snprintf(pstr1, 16, "%08x.sie", pdrv_info->presource[ucloop]->start);
		//DBG_IND("resource-start: %s, %s\r\n", pstr1, dev_name(&pdev->dev));
		if (IS_ERR(pdrv_info->module_info.pclk[ucloop])) {
			nvt_dbg(ERR, "faile to get clock%d source\n", ucloop);
			rt = -1;
			goto FAIL_FREE_REMAP;
		}
	}

	of_property_read_u32(pdev->dev.of_node, "power_saving", &sie_power_saving_mode);

	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		nvt_dbg(ERR, "Can't get device ID\n");
		rt = -1;
		goto FAIL_FREE_REMAP;
	}
#if 0
	nvt_dbg(IND, "DevID Major:%d minor:%d\n" \
		        , MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));
#endif
	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_sie_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, MODULE_MINOR_COUNT)) {
		nvt_dbg(ERR, "Can't add cdev\n");
		rt = -1;
		goto FAIL_CDEV;
	}

    pdrv_info->pmodule_class = class_create(THIS_MODULE, "kdrv_sie");
	if(IS_ERR(pdrv_info->pmodule_class)) {
		nvt_dbg(ERR, "failed in creating class.\n");
		rt = -1;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT ) ; ucloop++) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
			, MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
			, MODULE_NAME"%d", ucloop);

		if(IS_ERR(pdrv_info->pdevice[ucloop])) {
			nvt_dbg(ERR, "failed in creating device%d.\n", ucloop);

            #if 0
			//for (; ucloop > 0 ; ucloop--)
			//	device_unregister(pdrv_info->pdevice[ucloop - 1]);
			#else
			//while (ucloop >= 0) {
				device_unregister(pdrv_info->pdevice[ucloop]);
			//	ucloop--;
			//}
			#endif
			rt = -1;
			goto FAIL_CLASS;
		}
	}

	rt = nvt_sie_drv_init(&pdrv_info->module_info, 0);
    platform_set_drvdata(pdev, pdrv_info);
	if(rt) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return rt;

FAIL_DRV_INIT:
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT ) ; ucloop++)
		device_unregister(pdrv_info->pdevice[ucloop]);

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

FAIL_FREE_REMAP:
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
	    iounmap(pdrv_info->module_info.io_addr[ucloop]);

FAIL_FREE_RES:
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));

FAIL_FREE_BUF:
    kfree(pdrv_info);
    //pdrv_info = NULL;
    return rt;
}

static int nvt_sie_remove(struct platform_device *pdev)
{
	PSIE_DRV_INFO pdrv_info;
	unsigned char ucloop;

	pdrv_info = platform_get_drvdata(pdev);
	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++)
		clk_put(pdrv_info->module_info.pclk[ucloop]);

	nvt_sie_drv_remove(&pdrv_info->module_info);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT ) ; ucloop++)
		device_unregister(pdrv_info->pdevice[ucloop]);

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
	    iounmap(pdrv_info->module_info.io_addr[ucloop]);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));

    //if (pdrv_info != NULL) {
    	kfree(pdrv_info);
    	//pdrv_info = NULL;
    //}
	return 0;
}

static int nvt_sie_suspend( struct platform_device * pdev, pm_message_t state)
{
	PSIE_DRV_INFO pdrv_info;;

	pdrv_info = platform_get_drvdata(pdev);
	return nvt_sie_drv_suspend(&pdrv_info->module_info);
}


static int nvt_sie_resume( struct platform_device * pdev)
{
	PSIE_DRV_INFO pdrv_info;;

	pdrv_info = platform_get_drvdata(pdev);
	return nvt_sie_drv_resume(&pdrv_info->module_info);
}

static struct platform_driver nvt_sie_driver = {
    .driver = {
		        .name   = "drv_sie",
				.owner = THIS_MODULE,
				.of_match_table = sie_match_table,
		      },
    .probe      = nvt_sie_probe,
    .remove     = nvt_sie_remove,
	.suspend = nvt_sie_suspend,
	.resume = nvt_sie_resume
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

int __init nvt_sie_module_init(void)
{
#if defined(__KERNEL__)
	UINT32 i;
	int fastboot = kdrv_builtin_is_fastboot();

	if (!fastboot) {
		for (i=0;i<MODULE_REG_NUM;i++) {
			sie_platform_set_reg_log_enable(i,ENABLE);	//force enable reg. log
		}
	}
#endif
	return platform_driver_register(&nvt_sie_driver);
}

void __exit nvt_sie_module_exit(void)
{
	platform_driver_unregister(&nvt_sie_driver);
}

module_init(nvt_sie_module_init);
module_exit(nvt_sie_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("SIE driver");
MODULE_LICENSE("GPL");
