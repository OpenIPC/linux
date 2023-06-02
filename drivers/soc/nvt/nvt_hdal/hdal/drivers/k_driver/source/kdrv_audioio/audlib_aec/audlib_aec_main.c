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

#include "audlib_aec_drv.h"
#include "audlib_aec_main.h"
#include "audlib_aec_dbg.h"

#include "kdrv_audioio/audlib_aec.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int audlib_aec_debug_level = NVT_DBG_WRN;
module_param_named(audlib_aec_debug_level, audlib_aec_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audlib_aec_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static AUDLIB_DRV_INFO *paudlib_drv_info;

//=============================================================================
// function declaration
//=============================================================================
int __init nvt_audlib_aec_module_init(void);
void __exit nvt_audlib_aec_module_exit(void);

//=============================================================================
// function define
//=============================================================================

static struct platform_driver nvt_audlib_driver = {
	.driver = {
		.name   = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

int __init nvt_audlib_aec_module_init(void)
{
	int ret;

	//nvt_dbg(WRN, "\n");

	paudlib_drv_info = kzalloc(sizeof(AUDLIB_DRV_INFO), GFP_KERNEL);
	if (!paudlib_drv_info) {
		printk("failed to allocate memory\n");
		goto FAIL_INIT;
	}

	nvt_audlib_aec_drv_init(&paudlib_drv_info->module_info);

	kdrv_audlib_aec_init();

	ret = platform_driver_register(&nvt_audlib_driver);

	return ret;

FAIL_INIT:
	kfree(paudlib_drv_info);
	return -ENOMEM;
}

void __exit nvt_audlib_aec_module_exit(void)
{

	nvt_audlib_aec_drv_remove(&paudlib_drv_info->module_info);

	kfree(paudlib_drv_info);

	//nvt_dbg(WRN, "\n");
	platform_driver_unregister(&nvt_audlib_driver);
}

module_init(nvt_audlib_aec_module_init);
module_exit(nvt_audlib_aec_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("audlib_aec driver");
MODULE_LICENSE("GPL");
