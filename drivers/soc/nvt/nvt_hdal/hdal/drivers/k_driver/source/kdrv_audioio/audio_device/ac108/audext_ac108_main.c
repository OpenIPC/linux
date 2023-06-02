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

#include "audext_ac108_drv.h"
#include "audext_ac108_main.h"
#include "audext_ac108_dbg.h"
#include "AudioCodec.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int audext_ac108_debug_level = NVT_DBG_WRN;
module_param_named(audext_ac108_debug_level, audext_ac108_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audext_ac108_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static AUDLIB_DRV_INFO *paudlib_drv_info;

//=============================================================================
// function declaration
//=============================================================================
int __init nvt_audext_ac108_module_init(void);
void __exit nvt_audext_ac108_module_exit(void);

//=============================================================================
// function define
//=============================================================================

static struct platform_driver nvt_audlib_driver = {
	.driver = {
		.name   = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

int __init nvt_audext_ac108_module_init(void)
{
	int ret;

	nvt_dbg(WRN, "\n");

	paudlib_drv_info = kzalloc(sizeof(AUDLIB_DRV_INFO), GFP_KERNEL);
	if (!paudlib_drv_info) {
		printk("failed to allocate memory\n");
		goto FAIL_INIT;
	}

	nvt_audext_ac108_drv_init(&paudlib_drv_info->module_info);

	ret = platform_driver_register(&nvt_audlib_driver);

	#if 0
	{
		AUDIO_CODEC_FUNC audcodecfunc;

		aud_ext_ac108_get_fp(&audcodecfunc);
		audcodecfunc.init(NULL);
	}
	#endif

	return ret;

FAIL_INIT:
	kfree(paudlib_drv_info);
	paudlib_drv_info = NULL;
	return -ENOMEM;
}

void __exit nvt_audext_ac108_module_exit(void)
{

	nvt_audext_ac108_drv_remove(&paudlib_drv_info->module_info);

	kfree(paudlib_drv_info);
	paudlib_drv_info = NULL;

	nvt_dbg(WRN, "\n");
	platform_driver_unregister(&nvt_audlib_driver);

}

module_init(nvt_audext_ac108_module_init);
module_exit(nvt_audext_ac108_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("external audio codec AC108-TDM4CH driver");
MODULE_LICENSE("GPL");
