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

#include "dis_drv.h"
#include "dis_main.h"
#include "dis_proc.h"
#include "dis_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int g_dis_debug_level = NVT_DBG_ERR;
module_param_named(g_dis_debug_level, g_dis_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(g_dis_debug_level, "Debug message level");
#endif

struct clk* dis_clk[MODULE_CLK_NUM];
//=============================================================================
// Global variable
//=============================================================================
static struct of_device_id g_dis_match_table[] = {
	{   .compatible = "nvt,kdrv_dis"},
	{}
};

//=============================================================================
// function declaration
//=============================================================================
static int nvt_dis_open(struct inode *p_inode, struct file *p_file);
static int nvt_dis_release(struct inode *p_inode, struct file *p_file);
static long nvt_dis_ioctl(struct file *p_filp, unsigned int cmd, unsigned long arg);
static int nvt_dis_probe(struct platform_device *p_dev);
static int nvt_dis_suspend(struct platform_device *p_dev, pm_message_t state);
static int nvt_dis_resume(struct platform_device *p_dev);
static int nvt_dis_remove(struct platform_device *p_dev);
int __init nvt_dis_module_init(void);
void __exit nvt_dis_module_exit(void);

//=============================================================================
// function define
//=============================================================================
static int nvt_dis_open(struct inode *p_inode, struct file *p_file)
{
	DIS_DRV_INFO *pdrv_info;

	if ((p_inode == NULL) || (p_file == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}

	pdrv_info = container_of(vos_icdev(p_inode), DIS_DRV_INFO, cdev);
	p_file->private_data = pdrv_info;

	if (pdrv_info == NULL) {
		nvt_dbg(ERR, "invalid driver input\n");
		return -EINVAL;
	}

	if (nvt_dis_drv_open(&pdrv_info->module_info, MINOR(vos_irdev(p_inode)))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_dis_release(struct inode *p_inode, struct file *p_file)
{
	DIS_DRV_INFO *pdrv_info;
	if ((p_inode == NULL) || (p_file == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}
	pdrv_info = container_of(vos_icdev(p_inode), DIS_DRV_INFO, cdev);

	if (pdrv_info == NULL) {
		nvt_dbg(ERR, "invalid driver input\n");
		return -EINVAL;
	}

	nvt_dis_drv_release(&pdrv_info->module_info, MINOR(vos_irdev(p_inode)));
	return 0;
}

static long nvt_dis_ioctl(struct file *p_filp, unsigned int cmd, unsigned long arg)
{
	struct inode *p_inode;
	PDIS_DRV_INFO pdrv;

	if (p_filp == NULL) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}

	p_inode = file_inode(p_filp);

	
	if (p_inode == NULL) {
		nvt_dbg(ERR, "invalid file_inode\n");
		return -EINVAL;
	}
	
	pdrv = p_filp->private_data;

	return nvt_dis_drv_ioctl(MINOR(vos_irdev(p_inode)), &pdrv->module_info, cmd, arg);
}

struct file_operations nvt_dis_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_dis_open,
	.release = nvt_dis_release,
	.unlocked_ioctl = nvt_dis_ioctl,
	.llseek  = no_llseek,
};

static int nvt_dis_probe(struct platform_device *p_dev)
{
	DIS_DRV_INFO *p_drv_info;//info;
	const struct of_device_id *match;
	int ret = 0;
	unsigned char ucloop;

	if (p_dev == NULL) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}

	//nvt_dbg(IND, "kdrv %s\n", p_dev->name);

	match = of_match_device(g_dis_match_table, &p_dev->dev);
	if (!match) {
		nvt_dbg(ERR, "Platform device not found \n");
		return -EINVAL;
	}

	p_drv_info = kzalloc(sizeof(DIS_DRV_INFO), GFP_KERNEL);
	if (!p_drv_info) {
		nvt_dbg(ERR, "failed to allocate memory\n");
		return -ENOMEM;
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		p_drv_info->p_resource[ucloop] = platform_get_resource(p_dev, IORESOURCE_MEM, ucloop);
		if (p_drv_info->p_resource[ucloop] == NULL) {
			nvt_dbg(ERR, "No IO memory resource defined:%d\n", ucloop);
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		//nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", ucloop, p_drv_info->p_resource[ucloop]->start, resource_size(p_drv_info->p_resource[ucloop]));
		if (!request_mem_region(p_drv_info->p_resource[ucloop]->start, resource_size(p_drv_info->p_resource[ucloop]), p_dev->name)) {
			nvt_dbg(ERR, "failed to request memory resource%d\n", ucloop);
			#if (MODULE_REG_NUM > 1)
			for (; ucloop > 0 ;) {
				ucloop -= 1;
				release_mem_region(p_drv_info->p_resource[ucloop]->start, resource_size(p_drv_info->p_resource[ucloop]));
			}
			#endif
			ret = -ENODEV;
			goto FAIL_FREE_BUF;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
		p_drv_info->module_info.io_addr[ucloop] = ioremap_nocache(p_drv_info->p_resource[ucloop]->start, resource_size(p_drv_info->p_resource[ucloop]));
		if (p_drv_info->module_info.io_addr[ucloop] == NULL) {
			nvt_dbg(ERR, "ioremap() failed in module%d\n", ucloop);
			#if (MODULE_REG_NUM > 1)
			for (; ucloop > 0 ;) {
				ucloop -= 1;
				iounmap(p_drv_info->module_info.io_addr[ucloop]);
			}
			#endif
			ret = -ENODEV;
			goto FAIL_FREE_RES;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_IRQ_NUM; ucloop++) {
		p_drv_info->module_info.iinterrupt_id[ucloop] = platform_get_irq(p_dev, ucloop);
		//nvt_dbg(IND, "IRQ %d. ID%d\n", ucloop, p_drv_info->module_info.iinterrupt_id[ucloop]);
		if (p_drv_info->module_info.iinterrupt_id[ucloop] < 0) {
			nvt_dbg(ERR, "No IRQ resource defined\n");
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}



	//Get clock source
	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++) {
		char  pstr1[16];
		snprintf(pstr1, 16, "%08x.dis", p_drv_info->p_resource[ucloop]->start);
		//pdrv_info->module_info.pclk[ucloop] = clk_get(&pdev->dev, dev_name(&pdev->dev));
		dis_clk[ucloop] = clk_get(&p_dev->dev, pstr1);
		if (IS_ERR(p_drv_info->module_info.p_clk[ucloop])) {
			nvt_dbg(ERR, "faile to get clock%d source\n", ucloop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&p_drv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		nvt_dbg(ERR, "Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	//nvt_dbg(IND, "DevID Major:%d minor:%d\n" , MAJOR(p_drv_info->dev_id), MINOR(p_drv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&p_drv_info->cdev, &nvt_dis_fops);
	p_drv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&p_drv_info->cdev, p_drv_info->dev_id, MODULE_MINOR_COUNT)) {
		nvt_dbg(ERR, "Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	p_drv_info->p_module_class = class_create(THIS_MODULE, "kdrv_dis");
	if (IS_ERR(p_drv_info->p_module_class)) {
		nvt_dbg(ERR, "failed in creating class.\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

	/* register your own device in sysfs, and this will cause udev to create corresponding device node */
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++) {
		p_drv_info->p_device[ucloop] = device_create(p_drv_info->p_module_class, NULL
									 , MKDEV(MAJOR(p_drv_info->dev_id), (ucloop + MINOR(p_drv_info->dev_id))), NULL
									 , MODULE_NAME"%d", ucloop);

		if (IS_ERR(p_drv_info->p_device[ucloop])) {
			nvt_dbg(ERR, "failed in creating device%d.\n", ucloop);
			#if (MODULE_REG_NUM > 1)
			for (; ucloop > 0 ; ucloop--)
				device_unregister(p_drv_info->p_device[ucloop - 1]);
			#endif
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}
#if !defined(CONFIG_NVT_SMALL_HDAL)
	ret = nvt_dis_proc_init(p_drv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DEV;
	}
#endif
	ret = nvt_dis_drv_init(&p_drv_info->module_info);

	platform_set_drvdata(p_dev, p_drv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;

FAIL_DRV_INIT:
#if !defined(CONFIG_NVT_SMALL_HDAL)
	nvt_dis_proc_remove(p_drv_info);
FAIL_DEV:
#endif
	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++)
		device_unregister(p_drv_info->p_device[ucloop]);

FAIL_CLASS:
	class_destroy(p_drv_info->p_module_class);

FAIL_CDEV:
	cdev_del(&p_drv_info->cdev);
	vos_unregister_chrdev_region(p_drv_info->dev_id, MODULE_MINOR_COUNT);

FAIL_FREE_REMAP:
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
		iounmap(p_drv_info->module_info.io_addr[ucloop]);

FAIL_FREE_RES:
	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
		release_mem_region(p_drv_info->p_resource[ucloop]->start, resource_size(p_drv_info->p_resource[ucloop]));

FAIL_FREE_BUF:
	kfree(p_drv_info);
	p_drv_info = NULL;
	platform_set_drvdata(p_dev, p_drv_info);
	return ret;
}

static int nvt_dis_remove(struct platform_device *p_dev)
{
	PDIS_DRV_INFO pdrv_info;
	unsigned char ucloop;

	if (p_dev == NULL) {		
		nvt_dbg(ERR, "invalid input\n");		
		return -EINVAL;	
	}

	pdrv_info = platform_get_drvdata(p_dev);

	nvt_dis_drv_remove(&pdrv_info->module_info);
#if !defined(CONFIG_NVT_SMALL_HDAL)
	nvt_dis_proc_remove(pdrv_info);
#endif

	for (ucloop = 0 ; ucloop < (MODULE_MINOR_COUNT) ; ucloop++)
		device_unregister(pdrv_info->p_device[ucloop]);

	class_destroy(pdrv_info->p_module_class);
	cdev_del(&pdrv_info->cdev);
	vos_unregister_chrdev_region(pdrv_info->dev_id, MODULE_MINOR_COUNT);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
		iounmap(pdrv_info->module_info.io_addr[ucloop]);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++)
		release_mem_region(pdrv_info->p_resource[ucloop]->start, resource_size(pdrv_info->p_resource[ucloop]));

	kfree(pdrv_info);
	pdrv_info = NULL;
	platform_set_drvdata(p_dev, pdrv_info);
	return 0;
}

static int nvt_dis_suspend(struct platform_device *p_dev, pm_message_t state)
{
	PDIS_DRV_INFO pdrv_info;

	if (p_dev == NULL) {		
		nvt_dbg(ERR, "invalid input\n");		
		return -EINVAL;	
	}

	pdrv_info = platform_get_drvdata(p_dev);
	nvt_dis_drv_suspend(&pdrv_info->module_info);

	return 0;
}


static int nvt_dis_resume(struct platform_device *p_dev)
{
	PDIS_DRV_INFO pdrv_info;;

	if (p_dev == NULL) {		
		nvt_dbg(ERR, "invalid input\n");		
		return -EINVAL;	
	}

	pdrv_info = platform_get_drvdata(p_dev);
	nvt_dis_drv_resume(&pdrv_info->module_info);

	return 0;
}

static struct platform_driver nvt_dis_driver = {
	.driver = {
		.name   = "kdrv_dis",
		.owner = THIS_MODULE,
		.of_match_table = g_dis_match_table,
	},
	.probe      = nvt_dis_probe,
	.remove     = nvt_dis_remove,
	.suspend = nvt_dis_suspend,
	.resume = nvt_dis_resume
};

int __init nvt_dis_module_init(void)
{
	int ret;

	ret = platform_driver_register(&nvt_dis_driver);

	return 0;
}

void __exit nvt_dis_module_exit(void)
{
	platform_driver_unregister(&nvt_dis_driver);
}

module_init(nvt_dis_module_init);
module_exit(nvt_dis_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("DIS driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.00.000");
