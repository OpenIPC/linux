#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <asm/signal.h>
#include <kwrap/dev.h>
//#include "uitron_wrapper/type.h"
#include "ai_drv.h"
#include "ai_main.h"
#if defined(__FREERTOS)
#include "kwrap/debug.h"
#else
#include "kdrv_ai_dbg.h"
#endif
#include "kdrv_ai.h"
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#include "kdrv_ai_version.h"
#else
#include "ai_proc.h"
#include "../../include/kdrv_ai_version.h"
#endif
#include "kdrv_builtin.h"
//#include <mach/ftpmu010.h>

#define KDRV_AI_BUILTIN_MAGIC_NUM 0x8abc1688

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
unsigned int ai_net_supported_num = 1;
#else
#if defined(CONFIG_NVT_SMALL_HDAL)
unsigned int ai_net_supported_num = 1;
#else
unsigned int ai_net_supported_num = 16;
#endif
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
module_param_named(ai_net_supported_num, ai_net_supported_num, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ai_net_supported_num, "set ai_net_supported_number");

#ifdef DEBUG
unsigned int ai_debug_level = (NVT_DBG_IND | NVT_DBG_WRN | NVT_DBG_ERR);
module_param_named(ai_debug_level, ai_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ai_debug_level, "Debug message level");
#endif

struct clk* ai_clk[MODULE_CLK_NUM];
UINT32 ai_freq_from_dtsi[MODULE_CLK_NUM];
UINT32 is_ai_builtin_on=KDRV_AI_BUILTIN_MAGIC_NUM; //0:off, 1:on, others:don't care
UINT32 is_ai_on=KDRV_AI_BUILTIN_MAGIC_NUM; //0:off, 1:on, others:don't care
UINT32 is_ai_enable_dtsi=0;
UINT32 do_ai_init=0;
//=============================================================================
// Global variable
//=============================================================================
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
static struct of_device_id ai_match_table[] = {
	{   .compatible = "nvt,kdrv_ai_builtin"},
	{}
};
#else
static struct of_device_id ai_match_table[] = {
	{   .compatible = "nvt,kdrv_ai"},
	{}
};
#endif

#if 0
static int ai_fd = 0;
#endif

//=============================================================================
// function declaration
//=============================================================================
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
static int nvt_ai_probe(struct platform_device *pdev);
static int nvt_ai_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_ai_resume(struct platform_device *pdev);
static int nvt_ai_remove(struct platform_device *pdev);
#else
static int nvt_ai_open(struct inode *inode, struct file *file);
static int nvt_ai_release(struct inode *inode, struct file *file);
static long nvt_ai_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int nvt_ai_probe(struct platform_device *pdev);
static int nvt_ai_suspend(struct platform_device *pdev, pm_message_t state);
static int nvt_ai_resume(struct platform_device *pdev);
static int nvt_ai_remove(struct platform_device *pdev);
#endif
int __init nvt_ai_module_init(void);
void __exit nvt_ai_module_exit(void);

//=============================================================================
// function define
//=============================================================================
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
static int nvt_ai_open(struct inode *inode, struct file *file)
{
	AI_DRV_INFO *pdrv_info;

	if ((inode == NULL) || (file == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}

	pdrv_info = container_of(vos_icdev(inode), AI_DRV_INFO, cdev);
	file->private_data = pdrv_info;

	if (pdrv_info == NULL) {
		nvt_dbg(ERR, "invalid driver input\n");
		return -EINVAL;
	}

	if (nvt_ai_drv_open(&pdrv_info->module_info, MINOR(vos_irdev(inode)))) {
		nvt_dbg(ERR, "failed to open driver\n");
		return -1;
	}

	return 0;
}

static int nvt_ai_release(struct inode *inode, struct file *file)
{
	AI_DRV_INFO *pdrv_info;
	if ((inode == NULL) || (file == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}

	pdrv_info = container_of(vos_icdev(inode), AI_DRV_INFO, cdev);
	if (pdrv_info == NULL) {
		nvt_dbg(ERR, "invalid driver input\n");
		return -EINVAL;
	}

	nvt_ai_drv_release(&pdrv_info->module_info, MINOR(vos_irdev(inode)));
	return 0;
}

static long nvt_ai_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct inode *inode;
	PAI_DRV_INFO pdrv;

	if (filp == NULL) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}

	inode = file_inode(filp);
	if (inode == NULL) {
		nvt_dbg(ERR, "invalid file_inode\n");
		return -EINVAL;
	}


	pdrv = filp->private_data;

	return nvt_ai_drv_ioctl(MINOR(vos_irdev(inode)), &pdrv->module_info, cmd, arg);
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)

#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
struct file_operations nvt_ai_fops = {
	.owner   = THIS_MODULE,
	.llseek  = no_llseek,
};
#else
struct file_operations nvt_ai_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ai_open,
	.release = nvt_ai_release,
	.unlocked_ioctl = nvt_ai_ioctl,
	.llseek  = no_llseek,
};
#endif

/*
 * PMU registers
 */
#if 0
static pmuReg_t pmu_ai_reg[] = {
	//CNN & NUE & NUE2
	// todo: update global registers
	/* reg_off, bit_masks,              lock_bits,              init_val,                   init_mask */
	{ 0x50, ((0x1 << 0) | (0x1 << 13)), ((0x1 << 0) | (0x1 << 13)), ((0x1 << 0) | (0x1 << 13)), ((0x1 << 0) | (0x1 << 13))   }, // IP ARESETN controlled. 0:reset, 1:release
	{ 0x60, ((0x1 << 9) | (0x1 << 28)), ((0x1 << 9) | (0x1 << 28)), ((0x0 << 9) | (0x0 << 28)), ((0x1 << 9) | (0x1 << 28))   }, // gating clock-ACLK0 - 0:release, 1:gating
	{ 0x74, ((0x1 << 1) | (0x1 << 18)), ((0x1 << 1) | (0x1 << 18)), ((0x0 << 1) | (0x0 << 18)), ((0x1 << 1) | (0x1 << 18))   }, // gating clock-PCLK1 - 0:release, 1:gating
	{ 0x78, (0x1 << 6), (0x1 << 6), (0x0 << 6), (0x1 << 6)   },
	/*
	MClk CkCtrl
	[15: 0]: gated period
	[17:16]: gated mode
	[   18]: gated set
	[   31]: svmclk Select. 0: sys_aclk0, 1: sys_aclk1*/
	{ 0x150, (0x1 << 31), (0x1 << 31), (0x0 << 31), (0x1 << 31)   },
	{ 0x164, (0x1 << 31), (0x1 << 31), (0x0 << 31), (0x1 << 31)   },
};


static pmuRegInfo_t pmu_ai_reg_info = {
	"kdrv_ai",
	ARRAY_SIZE(pmu_ai_reg),
	ATTR_TYPE_NONE,
	&pmu_ai_reg[0]
};
#endif

static int nvt_handle_fb_ai_on_off(VOID)
{
	int ret = 0;
	struct device_node *top;

	top = of_find_compatible_node(NULL, NULL, "nvt,kdrv_ai_builtin");
	if (top) {
			if (!of_property_read_u32(top, "ai_enable", &is_ai_builtin_on)) {
				//do nn thing
			} else {
				is_ai_builtin_on = KDRV_AI_BUILTIN_MAGIC_NUM;
			}
	} else {
		is_ai_builtin_on = KDRV_AI_BUILTIN_MAGIC_NUM;
	}

	top = of_find_compatible_node(NULL, NULL, "nvt,kdrv_ai");
    if (top) {
            if (!of_property_read_u32(top, "ai_enable", &is_ai_on)) {
				//do no thing
            } else {
                is_ai_on = KDRV_AI_BUILTIN_MAGIC_NUM;
            }
    } else {
        is_ai_on = KDRV_AI_BUILTIN_MAGIC_NUM;
    }

	
	if ((is_ai_builtin_on == KDRV_AI_BUILTIN_MAGIC_NUM) || (is_ai_on == KDRV_AI_BUILTIN_MAGIC_NUM)) {
		is_ai_enable_dtsi = 0;
	} else {
		is_ai_enable_dtsi = 1;
	}

	do_ai_init = 0;
	if (is_ai_enable_dtsi == 1) {
		if ((is_ai_builtin_on == 1) && (is_ai_on == 1)) {
			nvt_dbg(ERR, "KDRV_AI: Error, built_ai and ai can't be both opened at dtsi(ai_enable).\n");
			ret = -ENODEV;
			goto EXIT_ERR;
		} else {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
			if (is_ai_builtin_on == 1) {
				do_ai_init = 1;
			}
#else
			if (is_ai_on == 1) {
				do_ai_init = 1;
			}
#endif
		}
	}

EXIT_ERR:

	return ret;
}

static int nvt_ai_probe(struct platform_device *pdev)
{
	AI_DRV_INFO *pdrv_info;
	const struct of_device_id *match;
	int ret = 0;
	int ucloop;

	
	if (pdev == NULL) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}
	//nvt_dbg(IND, "kdrv %s\n", pdev->name);

	match = of_match_device(ai_match_table, &pdev->dev);
	if (!match) {
		nvt_dbg(ERR, "Platform device not found \n");
		return -EINVAL;
	}

	pdrv_info = kzalloc(sizeof(AI_DRV_INFO), GFP_KERNEL);
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

	ret = nvt_handle_fb_ai_on_off();
	if (ret != 0) {
		nvt_dbg(ERR, "KDRV_AI: Error, to do nvt_handle_fb_ai_on_off().\n");
		ret = -ENODEV;
		goto FAIL_FREE_BUF;
	}

	//nvt_dbg(ERR, "KDRV_AI: is_builtin_ai=%d, is_ai=%d\r\n", is_ai_builtin_on, is_ai_on);

	if (is_ai_enable_dtsi == 1) {
		if (do_ai_init == 1) {
			for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
				//nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
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
		}
	} else {
		if (kdrv_builtin_is_fastboot()) {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
			for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
				//nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
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
#else
#endif
		} else {
			for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
				//nvt_dbg(IND, "%d. resource:0x%x size:0x%x\n", ucloop, pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
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
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
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

	for (ucloop = 0 ; ucloop < MODULE_IRQ_NUM; ucloop++) {
		pdrv_info->module_info.iinterrupt_id[ucloop] = platform_get_irq(pdev, ucloop);
		//nvt_dbg(IND, "IRQ %d. ID%d\n", ucloop, pdrv_info->module_info.iinterrupt_id[ucloop]);
		if (pdrv_info->module_info.iinterrupt_id[ucloop] < 0) {
			nvt_dbg(ERR, "No IRQ resource defined\n");
			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}

#if 1
	//Get clock source
	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++) {
		char pstr1[64];
		
		//pdrv_info->module_info.pclk[ucloop] = clk_get(&pdev->dev, dev_name(&pdev->dev));
		snprintf(pstr1, 16, "%08x.ai", pdrv_info->presource[ucloop]->start);
		//DBG_IND("resource-start: %s, %s\r\n", pstr1, dev_name(&pdev->dev));
		ai_clk[ucloop] = clk_get(&pdev->dev, pstr1/*dev_name(&pdev->dev)*/);
		if (IS_ERR(pdrv_info->module_info.pclk[ucloop])) {
			nvt_dbg(ERR, "faile to get clock%d source\n", ucloop);

			ret = -ENODEV;
			goto FAIL_FREE_REMAP;
		}
	}
	//nvt_dbg(IND, " get clock source\r\n");

	if (of_property_read_u32_array(pdev->dev.of_node, "clock-frequency", ai_freq_from_dtsi, MODULE_CLK_NUM)) {
		nvt_dbg(IND, "%s_%d, no dtsi input for clock-frequency.\r\n", __FUNCTION__, __LINE__);
	}
	
	//printk("clk %d %d %d %d\n", (unsigned int) ai_freq_from_dtsi[0], (unsigned int) ai_freq_from_dtsi[1], 
	//							(unsigned int) ai_freq_from_dtsi[2], (unsigned int) ai_freq_from_dtsi[3]);

#else
	ai_fd = ftpmu010_register_reg(&pmu_ai_reg_info);
	if (ai_fd < 0) {
		panic("%s, ftpmu010_register_reg kdrv ai fail", __func__);
	}
#endif
	//Dynamic to allocate Device ID
	if (vos_alloc_chrdev_region(&pdrv_info->dev_id, MODULE_MINOR_COUNT, MODULE_NAME)) {
		nvt_dbg(ERR, "Can't get device ID\n");
		ret = -ENODEV;
		goto FAIL_FREE_REMAP;
	}

	//nvt_dbg(IND, "DevID Major:%d minor:%d\n", MAJOR(pdrv_info->dev_id), MINOR(pdrv_info->dev_id));

	/* Register character device for the volume */
	cdev_init(&pdrv_info->cdev, &nvt_ai_fops);
	pdrv_info->cdev.owner = THIS_MODULE;

	if (cdev_add(&pdrv_info->cdev, pdrv_info->dev_id, MODULE_MINOR_COUNT)) {
		nvt_dbg(ERR, "Can't add cdev\n");
		ret = -ENODEV;
		goto FAIL_CDEV;
	}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
	pdrv_info->pmodule_class = class_create(THIS_MODULE, "kdrv_ai_builtin");
#else
	pdrv_info->pmodule_class = class_create(THIS_MODULE, "kdrv_ai");
#endif
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

			while (ucloop >= 0) {
				device_unregister(pdrv_info->pdevice[ucloop]);
				ucloop--;
			}
			ret = -ENODEV;
			goto FAIL_CLASS;
		}
	}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
	ret = nvt_ai_proc_init(pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DEV;
	}
#endif

	ret = nvt_ai_drv_init(&pdrv_info->module_info);

	platform_set_drvdata(pdev, pdrv_info);
	if (ret) {
		nvt_dbg(ERR, "failed in creating proc.\n");
		goto FAIL_DRV_INIT;
	}

	return ret;


FAIL_DRV_INIT:
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
	nvt_ai_proc_remove(pdrv_info);
#endif

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
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

	if (is_ai_enable_dtsi == 1) {
		if (do_ai_init == 1) {
			for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			}
		}
	} else {
		if (kdrv_builtin_is_fastboot()) {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
			for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			}
#else
#endif
		} else {
			for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			}
		}
	}

FAIL_FREE_BUF:
	kfree(pdrv_info);
	return ret;
}

static int nvt_ai_remove(struct platform_device *pdev)
{
	PAI_DRV_INFO pdrv_info;
	unsigned char ucloop;
	if (pdev == NULL) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}

	nvt_dbg(IND, "\n");

	pdrv_info = platform_get_drvdata(pdev);

	nvt_ai_drv_remove(&pdrv_info->module_info);

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
	nvt_ai_proc_remove(pdrv_info);
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

	if (is_ai_enable_dtsi == 1) {
		if (do_ai_init == 1) {
			for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			}
		}
	} else {
		if (kdrv_builtin_is_fastboot()) {
#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
			for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			}
#else
#endif
		} else {
			for (ucloop = 0 ; ucloop < MODULE_REG_NUM ; ucloop++) {
				release_mem_region(pdrv_info->presource[ucloop]->start, resource_size(pdrv_info->presource[ucloop]));
			}
		}
	}

#if 0
	ftpmu010_deregister_reg(ai_fd);
#endif

	kfree(pdrv_info);
	return 0;
}

static int nvt_ai_suspend(struct platform_device *pdev, pm_message_t state)
{
	PAI_DRV_INFO pdrv_info;

	if (pdev == NULL) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}
	nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ai_drv_suspend(&pdrv_info->module_info);

	nvt_dbg(IND, "finished\n");
	return 0;
}


static int nvt_ai_resume(struct platform_device *pdev)
{
	PAI_DRV_INFO pdrv_info;;

	if (pdev == NULL) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}
	nvt_dbg(IND, "start\n");

	pdrv_info = platform_get_drvdata(pdev);
	nvt_ai_drv_resume(&pdrv_info->module_info);

	nvt_dbg(IND, "finished\n");
	return 0;
}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
static struct platform_driver nvt_ai_driver = {
	.driver = {
		.name   = "kdrv_ai_builtin",
		.owner = THIS_MODULE,
		.of_match_table = ai_match_table,
	},
	.probe      = nvt_ai_probe,
	.remove     = nvt_ai_remove,
	.suspend = nvt_ai_suspend,
	.resume = nvt_ai_resume
};
#else
static struct platform_driver nvt_ai_driver = {
	.driver = {
		.name   = "kdrv_ai",
		.owner = THIS_MODULE,
		.of_match_table = ai_match_table,
	},
	.probe      = nvt_ai_probe,
	.remove     = nvt_ai_remove,
	.suspend = nvt_ai_suspend,
	.resume = nvt_ai_resume
};
#endif

int __init nvt_ai_module_init(void)
{
	int ret;
	
	ret = platform_driver_register(&nvt_ai_driver);
	return 0;
}

void __exit nvt_ai_module_exit(void)
{
	platform_driver_unregister(&nvt_ai_driver);
}

#if (KDRV_AI_MINI_FOR_FASTBOOT == 1)
//module_init(nvt_ai_module_init);
//module_exit(nvt_ai_module_exit);
#else
module_init(nvt_ai_module_init);
module_exit(nvt_ai_module_exit);
#endif

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("AI driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(KDRV_AI_IMPL_VERSION);
