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

#include "md_drv.h"
#include "md_main.h"
#include "md_proc.h"
#include "md_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int md_debug_level = NVT_DBG_WRN;
module_param_named(md_debug_level, md_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(md_debug_level, "Debug message level");
#endif

UINT32 md_freq_from_dtsi[MD_CLK_NUM];

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id md_match_table[] = {
	{	.compatible = "nvt,kdrv_md"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_md_open(struct inode *inode, struct file *file);
static int nvt_md_release(struct inode *inode, struct file *file);
static long nvt_md_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_md_probe(struct platform_device * pdev);
static int nvt_md_suspend( struct platform_device * pdev, pm_message_t state);
static int nvt_md_resume( struct platform_device * pdev);
static int nvt_md_remove(struct platform_device *pdev);
int __init nvt_md_module_init(void);
void __exit nvt_md_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int nvt_md_open(struct inode *inode, struct file *file)
{
	MD_DRV_INFO* pdrv_info;

	pdrv_info = container_of(vos_icdev(inode), MD_DRV_INFO, cdev);

	pdrv_info = container_of(vos_icdev(inode), MD_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (nvt_md_drv_open(&pdrv_info->module_info, MINOR(vos_irdev(inode)))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}

    return 0;
}

static int nvt_md_release(struct inode *inode, struct file *file)
{
	MD_DRV_INFO* pdrv_info;
	pdrv_info = container_of(vos_icdev(inode), MD_DRV_INFO, cdev);

	nvt_md_drv_release(&pdrv_info->module_info, MINOR(vos_irdev(inode)));
    return 0;
}

static long nvt_md_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PMD_DRV_INFO pdrv;

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return nvt_md_drv_ioctl(MINOR(vos_irdev(inode)), &pdrv->module_info, cmd, arg);
}

struct file_operations nvt_md_fops = {
    .owner   = THIS_MODULE,
    .open    = nvt_md_open,
    .release = nvt_md_release,
    .unlocked_ioctl = nvt_md_ioctl,
    .llseek  = no_llseek,
};

static int nvt_md_probe(struct platform_device * pdev)
{
    MD_DRV_INFO* pdrv_info;//info;
	const struct of_device_id *match;
    int ret = 0;
	unsigned char ucloop;

	//nvt_dbg(IND, "1\r\n");

    //nvt_dbg(IND, "kdrv %s\n", pdev->name);

	match = of_match_device(md_match_table, &pdev->dev);
	if (!match){
        nvt_dbg(ERR, "Platform device not found \n");
		return -EINVAL;
	}

    pdrv_info = kzalloc(sizeof(MD_DRV_INFO), GFP_KERNEL);
    if (!pdrv_info) {
        nvt_dbg(ERR, "failed to allocate memory\n");
        return -ENOMEM;
    }

    //nvt_dbg(IND, "2\r\n");

	for (ucloop = 0 ; ucloop < MD_REG_NUM ; ucloop++) {
		pdrv_info->presource[ucloop] = platform_get_resource(pdev, IORESOURCE_MEM, ucloop);
		if (pdrv_info->presource[ucloop] == NULL) {
			nvt_dbg(ERR, "No IO memory resource defined:%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	//nvt_dbg(IND, "3\r\n");

	for (ucloop = 0 ; ucloop < MD_REG_NUM ; ucloop++) {
		nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (!request_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]), pdev->name)) {
			nvt_dbg(ERR, "failed to request memory resource%d\n", ucloop);
			while (ucloop >= 0) {
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
				ucloop--;
			}
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	//nvt_dbg(IND, "4\r\n");

	for (ucloop = 0 ; ucloop < MD_REG_NUM ; ucloop++) {
		pdrv_info->module_info.io_addr[ucloop] = ioremap_nocache(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
		if (pdrv_info->module_info.io_addr[ucloop] == NULL) {
			nvt_dbg(ERR, "ioremap() failed in module%d\n", ucloop);
			while (ucloop >= 0) {
			    iounmap(pdrv_info->module_info.io_addr[ucloop]);
				ucloop--;
			}
			ret = -ENODEV;
			goto FAIL_FREE_RES;
		}
	}

	//nvt_dbg(IND, "5\r\n");

	for (ucloop = 0 ; ucloop < MD_IRQ_NUM; ucloop++) {
		pdrv_info->module_info.iinterrupt_id[ucloop] = platform_get_irq(pdev, ucloop);
		nvt_dbg(IND, "IRQ %d. ID%d\n", ucloop,pdrv_info->module_info.iinterrupt_id[ucloop]);
		if (pdrv_info->module_info.iinterrupt_id[ucloop] < 0) {
			nvt_dbg(ERR, "No IRQ resource defined\n");
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}



	//Get clock source

	for (ucloop = 0 ; ucloop < MD_CLK_NUM; ucloop++) {
		//char  pstr1[16];
		//snprintf(pstr1, 16, "%08x.ai", pdrv_info->presource[ucloop]->start);
        //pdrv_info->module_info.pclk[ucloop] = clk_get(&pdev->dev, pstr1);
		pdrv_info->module_info.pclk[ucloop] = clk_get(&pdev->dev, dev_name(&pdev->dev));
		if (IS_ERR(pdrv_info->module_info.pclk[ucloop])) {
			nvt_dbg(ERR, "failed to get clock%d source\n", ucloop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}
	ret = of_property_read_u32_array(pdev->dev.of_node, "clock-frequency", md_freq_from_dtsi, MD_CLK_NUM);
	if(ret != 0){
		nvt_dbg(ERR, "of_property_read_u32_array failed.\n");
	}


	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MD_MINOR_COUNT, MD_NAME)) {
		nvt_dbg(ERR, "Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	nvt_dbg(IND, "DevID Major:%d minor:%d\n" \
		        , MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_md_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, MD_MINOR_COUNT)) {
		nvt_dbg(ERR, "Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

    pdrv_info->pmodule_class = class_create(THIS_MODULE, "kdrv_md");
	if(IS_ERR(pdrv_info->pmodule_class)) {
		nvt_dbg(ERR, "failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MD_MINOR_COUNT ) ; ucloop++) {
		pdrv_info->pdevice[ucloop] = device_create(pdrv_info->pmodule_class, NULL
			, MKDEV(MAJOR(pdrv_info->dev_id), (ucloop + MINOR(pdrv_info->dev_id))), NULL
			, MD_NAME"%d", ucloop);

		if(IS_ERR(pdrv_info->pdevice[ucloop])) {
			nvt_dbg(ERR, "failed in creating device%d.\n", ucloop);

			for (; ucloop > 0 ; ucloop--)
				device_unregister(pdrv_info->pdevice[ucloop - 1]);
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}
#if !defined(CONFIG_NVT_SMALL_HDAL)
	ret = nvt_md_proc_init(pdrv_info);
	if(ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DEV;
	}
#endif
	ret = nvt_md_drv_init(&pdrv_info->module_info);

    platform_set_drvdata(pdev, pdrv_info);
	if(ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
#if !defined(CONFIG_NVT_SMALL_HDAL)
	nvt_md_proc_remove(pdrv_info);

FAIL_DEV:
#endif
	for (ucloop = 0 ; ucloop < (MD_MINOR_COUNT ) ; ucloop++)
		device_unregister(pdrv_info->pdevice[ucloop]);

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MD_MINOR_COUNT);

FAIL_FREE_REMAP:
	for (ucloop = 0 ; ucloop < MD_REG_NUM ; ucloop++)
	    iounmap(pdrv_info->module_info.io_addr[ucloop]);

FAIL_FREE_RES:
	for (ucloop = 0 ; ucloop < MD_REG_NUM ; ucloop++)
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));

FAIL_FREE_BUF:
    kfree(pdrv_info);
	pdrv_info = NULL;
    return ret;
}

static int nvt_md_remove(struct platform_device *pdev)
{
	PMD_DRV_INFO pdrv_info;
	unsigned char ucloop;

    nvt_dbg(IND, "\n");

	pdrv_info = platform_get_drvdata(pdev);

	nvt_md_drv_remove(&pdrv_info->module_info);
#if !defined(CONFIG_NVT_SMALL_HDAL)
	nvt_md_proc_remove(pdrv_info);
#endif
    for (ucloop = 0 ; ucloop < MD_CLK_NUM; ucloop++)
		clk_put(pdrv_info->module_info.pclk[ucloop]);

	for (ucloop = 0 ; ucloop < (MD_MINOR_COUNT) ; ucloop++)
		device_unregister(pdrv_info->pdevice[ucloop]);

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MD_MINOR_COUNT);

	for (ucloop = 0 ; ucloop < MD_REG_NUM ; ucloop++)
	    iounmap(pdrv_info->module_info.io_addr[ucloop]);

	for (ucloop = 0 ; ucloop < MD_REG_NUM ; ucloop++)
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));

    kfree(pdrv_info);
	pdrv_info = NULL;
	return 0;
}

static int nvt_md_suspend( struct platform_device * pdev, pm_message_t state)
{
	PMD_DRV_INFO pdrv_info;

    nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_md_drv_suspend(&pdrv_info->module_info);

    nvt_dbg(IND, "finished\n");
	return 0;
}


static int nvt_md_resume( struct platform_device * pdev)
{
	PMD_DRV_INFO pdrv_info;

    nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_md_drv_resume(&pdrv_info->module_info);

    nvt_dbg(IND, "finished\n");
	return 0;
}

static struct platform_driver nvt_md_driver = {
    .driver = {
		        .name   = "kdrv_md",
				.owner = THIS_MODULE,
				.of_match_table = md_match_table,
		      },
    .probe      = nvt_md_probe,
    .remove     = nvt_md_remove,
	.suspend = nvt_md_suspend,
	.resume = nvt_md_resume
};

int __init nvt_md_module_init(void)
{
	int ret;

    nvt_dbg(IND, "\n");
	ret = platform_driver_register(&nvt_md_driver);

	return 0;
}

void __exit nvt_md_module_exit(void)
{
    nvt_dbg(IND, "\n");
	platform_driver_unregister(&nvt_md_driver);
}

module_init(nvt_md_module_init);
module_exit(nvt_md_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("AI driver");
MODULE_LICENSE("GPL");
