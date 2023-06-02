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
#include "ctl_sie_drv.h"
#include "ctl_sie_main.h"
#include "ctl_sie_proc.h"
#include "ctl_sie_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int ctl_sie_debug_level = NVT_DBG_WRN;
module_param_named(ctl_sie_debug_level, ctl_sie_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ctl_sie_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static CTL_SIE_DRV_INFO *pctl_sie_info;

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

int __init nvt_ctl_sie_init(void);
void __exit nvt_ctl_sie_exit(void);

//=============================================================================
// function define
//=============================================================================
int __init nvt_ctl_sie_init(void)
{
	int ret;

	pctl_sie_info = kzalloc(sizeof(CTL_SIE_DRV_INFO), GFP_KERNEL);
	if (!pctl_sie_info) {
		ctl_sie_dbg_err("allocate mem fail\n");
		return -ENOMEM;
	}

	ret = nvt_ctl_sie_proc_init(pctl_sie_info);
	if (ret) {
		ctl_sie_dbg_err("creat proc. fail\n");
		goto FAIL_INIT;
	}

	ret = nvt_ctl_sie_drv_init(&pctl_sie_info->module_info);
	if (ret) {
		ctl_sie_dbg_err("sie drv init fail\n");
		goto FAIL_PROC;
	}

	return ret;

FAIL_PROC:
	nvt_ctl_sie_proc_remove(pctl_sie_info);

FAIL_INIT:
	kfree(pctl_sie_info);
	pctl_sie_info = NULL;
	return 0;
}

void __exit nvt_ctl_sie_exit(void)
{
	nvt_ctl_sie_drv_release(&pctl_sie_info->module_info, 0);

	nvt_ctl_sie_proc_remove(pctl_sie_info);

	kfree(pctl_sie_info);
	pctl_sie_info = NULL;
}

module_init(nvt_ctl_sie_init);
module_exit(nvt_ctl_sie_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("ctl_sie driver");
MODULE_LICENSE("GPL");
