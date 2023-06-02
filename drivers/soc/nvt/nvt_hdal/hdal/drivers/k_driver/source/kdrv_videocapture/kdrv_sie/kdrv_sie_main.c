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
#include "sie_drv.h"
#include "kdrv_sie_main.h"
#include "kdrv_sie_proc.h"
#include "kdrv_sie_int.h"
#include "sie_dbg.h"
#include <comm/nvtmem.h>


//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int kdrv_sie_debug_level = (NVT_DBG_IND | NVT_DBG_WRN | NVT_DBG_ERR);
module_param_named(kdrv_sie_debug_level, kdrv_sie_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(kdrv_sie_debug_level, "Debug message level");
#endif
//=============================================================================
// Global variable
//=============================================================================
static KDRV_SIE_DRV_INFO *kdrv_sie_drv_info;

//=============================================================================
// function declaration
//=============================================================================
int __init nvt_kdrv_sie_module_init(void);
void __exit nvt_kdrv_sie_module_exit(void);

#if 0
#endif
//=============================================================================
// function define
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

int __init nvt_kdrv_sie_init(void)
{
	int ret;

	kdrv_sie_drv_info = kzalloc(sizeof(KDRV_SIE_DRV_INFO), GFP_KERNEL);
	if (!kdrv_sie_drv_info) {
		kdrv_sie_dbg_err("allocate mem fail\n");
		return -ENOMEM;
	}

	ret = nvt_kdrv_sie_proc_init(kdrv_sie_drv_info);
	if(ret) {
		kdrv_sie_dbg_err("failed in creating proc.\n");
		goto FAIL_PROC;
	}

	kdrv_sie_init();

	return ret;

FAIL_PROC:
	nvt_kdrv_sie_proc_remove(kdrv_sie_drv_info);
	kfree(kdrv_sie_drv_info);
	kdrv_sie_drv_info = NULL;
	return 0;
}

void __exit nvt_kdrv_sie_exit(void)
{
	nvt_kdrv_sie_proc_remove(kdrv_sie_drv_info);
	kdrv_sie_uninit();

	kfree(kdrv_sie_drv_info);
	kdrv_sie_drv_info = NULL;
}

module_init(nvt_kdrv_sie_init);
module_exit(nvt_kdrv_sie_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("SIE Kdriver");
MODULE_LICENSE("GPL");
