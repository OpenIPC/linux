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

#include "display_obj_drv.h"
#include "display_obj_main.h"
#include "display_obj_proc.h"
#include "display_obj_dbg.h"
#include "display_obj_platform.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int dispobj_debug_level = (NVT_DBG_ERR);
module_param_named(dispobj_debug_level, dispobj_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dispobj_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id dispobj_match_table[] = {
	{   .compatible = "nvt,nvt_dispobj"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_dispobj_open(struct inode *inode, struct file *file);
static int nvt_dispobj_release(struct inode *inode, struct file *file);
static long nvt_dispobj_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_dispobj_probe(struct platform_device *pdev);
static int nvt_dispobj_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_dispobj_resume(struct platform_device *pdev);
static int nvt_dispobj_remove(struct platform_device *pdev);
int __init nvt_dispobj_module_init(void);
void __exit nvt_dispobj_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int nvt_dispobj_open(struct inode *inode, struct file *file)
{
	DISPOBJ_DRV_INFO *pdrv_info;

	pdrv_info = container_of(inode->i_cdev, DISPOBJ_DRV_INFO, cdev);

	pdrv_info = container_of(inode->i_cdev, DISPOBJ_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (nvt_dispobj_drv_open(&pdrv_info->module_info, MINOR(inode->i_rdev))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_dispobj_release(struct inode *inode, struct file *file)
{
	DISPOBJ_DRV_INFO *pdrv_info;

	pdrv_info = container_of(inode->i_cdev, DISPOBJ_DRV_INFO, cdev);

	nvt_dispobj_drv_release(&pdrv_info->module_info, MINOR(inode->i_rdev));
	return 0;
}

static long nvt_dispobj_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PDISPOBJ_DRV_INFO pdrv;

	inode = file_inode(filp);
	pdrv = filp->private_data;

	return nvt_dispobj_drv_ioctl(MINOR(inode->i_rdev), &pdrv->module_info, cmd, arg);
}

struct file_operations nvt_dispobj_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_dispobj_open,
	.release = nvt_dispobj_release,
	.unlocked_ioctl = nvt_dispobj_ioctl,
	.llseek  = no_llseek,
};

static int nvt_dispobj_probe(struct platform_device *pdev)
{
	DISPOBJ_DRV_INFO *pdrv_info;//info;
	const struct of_device_id *match;
	int ret = 0;
	unsigned char ucloop;
	struct clk *source_clk;
	struct device_node* of_node = of_find_node_by_path("/ide@f0800000");
	PDISP_OBJ p_disp_obj;
	DISPCTRL_PARAM  disp_ctrl = {0};
	p_disp_obj = disp_get_display_object(DISP_1);
	p_disp_obj->open();

	//nvt_dbg(IND, "%s\n", pdev->name);

	match = of_match_device(dispobj_match_table, &pdev->dev);
	if (!match) {
		nvt_dbg(ERR, "Platform device not found \n");
		return -EINVAL;
	}

	pdrv_info = kzalloc(sizeof(DISPOBJ_DRV_INFO), GFP_KERNEL);
	if (!pdrv_info) {
		nvt_dbg(ERR, "failed to allocate memory\n");
		return -ENOMEM;
	}
/*
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
	*/
	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		nvt_dbg(ERR, "Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_BUF;
	}

	//nvt_dbg(IND, "dev_id Major:%d minor:%d\n", MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_dispobj_fops);
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
#if (MODULE_REG_NUM > 1)
			for (; ucloop > 0 ; ucloop--)
				device_unregister(pdrv_info->pdevice[ucloop - 1]);
#endif
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}


	//printk("node = %d\r\n", (int)of_node);
	if (of_node) {
		int clock_source = 0;
		int source_freq = 0;
		if (!of_property_read_u32(of_node, "clock-source", &clock_source)) {
			disp_ctrl.SEL.SET_SRCCLK.src_clk = (DISPCTRL_SRCCLK)clock_source;
			p_disp_obj->disp_ctrl(DISPCTRL_SET_SRCCLK, &disp_ctrl);
			//printk("clock_source = %d\r\n", (int)clock_source);
		}

		if (!of_property_read_u32(of_node, "source-frequency", &source_freq)) {
			p_disp_obj->disp_ctrl(DISPCTRL_GET_SRCCLK, &disp_ctrl);
			clock_source = (int)disp_ctrl.SEL.GET_SRCCLK.src_clk;
			//printk("clock_source = %d\r\n", (int)clock_source);
			if (clock_source == 0) {
				source_clk = clk_get(NULL, "fix480m");
				if (IS_ERR(source_clk)) {
					DBG_ERR("dispobj get clk source err\n");
				}
				DBG_ERR("ide clk source 0 can not chanege freq\n");
				//clk_set_rate(source_clk, 324000000);
			} else if (clock_source == 1) {
				source_clk = clk_get(NULL, "pll6");
				if (IS_ERR(source_clk)) {
					DBG_ERR("dispobj get clk source err\n");
				}
				clk_set_rate(source_clk, source_freq);
			} else if (clock_source == 2) {
				source_clk = clk_get(NULL, "pll4");
				if (IS_ERR(source_clk)) {
					DBG_ERR("dispobj get clk source err\n");
				}
				clk_set_rate(source_clk, source_freq);
			} else if (clock_source == 3) {
				//printk("pll9\r\n");
				source_clk = clk_get(NULL, "pll9");
				if (IS_ERR(source_clk)) {
					DBG_ERR("dispobj get clk source err\n");
				}
				clk_set_rate(source_clk, source_freq);
			} else {
				DBG_ERR("dispobj clk source %d not support\n", clock_source);
			}
			//printk("source freq = %d\r\n", (int)source_freq);
		}

	}
	p_disp_obj->close();


	ret = nvt_dispobj_proc_init(pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DEV;
	}

	ret = nvt_dispobj_drv_init(&pdrv_info->module_info);

	platform_set_drvdata(pdev, pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
	nvt_dispobj_proc_remove(pdrv_info);

FAIL_DEV:
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}

FAIL_CLASS:
	class_destroy(pdrv_info->pmodule_class);

FAIL_CDEV:
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);
	/*
	FAIL_FREE_REMAP:
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
		iounmap(pdrv_info->module_info.io_addr[ucloop]);

FAIL_FREE_RES:
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	}
*/
FAIL_FREE_BUF:
	kfree(pdrv_info);
	pdrv_info = NULL;
	platform_set_drvdata(pdev, pdrv_info);
	return ret;
}

static int nvt_dispobj_remove(struct platform_device *pdev)
{
	PDISPOBJ_DRV_INFO pdrv_info;
	unsigned char ucloop;

	nvt_dbg(IND, "\n");

	pdrv_info = platform_get_drvdata(pdev);

	nvt_dispobj_drv_remove(&pdrv_info->module_info);

	nvt_dispobj_proc_remove(pdrv_info);

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		device_unregister(pdrv_info->pdevice[ucloop]);
	}

	class_destroy(pdrv_info->pmodule_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);
	/*
	    for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
			iounmap(pdrv_info->module_info.io_addr[ucloop]);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
	}
*/
	kfree(pdrv_info);
	pdrv_info = NULL;
	platform_set_drvdata(pdev, pdrv_info);
	return 0;
}

static int nvt_dispobj_suspend(struct platform_device *pdev, pm_message_t state)
{
	PDISPOBJ_DRV_INFO pdrv_info;;

	nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_dispobj_drv_suspend(&pdrv_info->module_info);

	nvt_dbg(IND, "finished\n");
	return 0;
}


static int nvt_dispobj_resume(struct platform_device *pdev)
{
	PDISPOBJ_DRV_INFO pdrv_info;;

	nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_dispobj_drv_resume(&pdrv_info->module_info);

	nvt_dbg(IND, "finished\n");
	return 0;
}

static struct platform_driver nvt_dispobj_driver = {
	.driver = {
		.name   = "nvt_dispobj",
		.owner = THIS_MODULE,
		.of_match_table = dispobj_match_table,
	},
	.probe      = nvt_dispobj_probe,
	.remove     = nvt_dispobj_remove,
	.suspend = nvt_dispobj_suspend,
	.resume = nvt_dispobj_resume
};

int __init nvt_dispobj_module_init(void)
{
	int ret;

	nvt_dbg(WRN, "\n");
	ret = platform_driver_register(&nvt_dispobj_driver);

	return 0;
}

void __exit nvt_dispobj_module_exit(void)
{
	nvt_dbg(WRN, "\n");
	platform_driver_unregister(&nvt_dispobj_driver);
}

module_init(nvt_dispobj_module_init);
module_exit(nvt_dispobj_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("dispobj driver");
MODULE_LICENSE("GPL");
