#if defined(__LINUX)
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
#elif defined(__FREERTOS)
#endif
#include "ctl_ipp_drv.h"
#include "ctl_ipp_main.h"
#include "ctl_ipp_proc.h"
#include "ctl_ipp_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
//#ifdef DEBUG
#if 0
unsigned int ctl_ipp_debug_level = NVT_DBG_WRN;
module_param_named(ctl_ipp_debug_level, ctl_ipp_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ctl_ipp_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
static CTL_IPP_DRV_INFO *pctl_ipp_info;

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

int __init nvt_ctl_ipp_init(void);
void __exit nvt_ctl_ipp_exit(void);

//=============================================================================
// function define
//=============================================================================
int __init nvt_ctl_ipp_init(void)
{
	int ret;

	pctl_ipp_info = kzalloc(sizeof(CTL_IPP_DRV_INFO), GFP_KERNEL);
	if (!pctl_ipp_info) {
		CTL_IPP_DBG_ERR("failed to allocate memory\n");
		return -ENOMEM;
	}

	ret = nvt_ctl_ipp_proc_init(pctl_ipp_info);
	if (ret) {
		CTL_IPP_DBG_ERR("failed in creating proc.\n");
		goto FAIL_INIT;
	}

	ret = nvt_ctl_ipp_drv_init(&pctl_ipp_info->module_info);
	if (ret) {
		CTL_IPP_DBG_ERR("failed in ipp drv init.\n");
		goto FAIL_PROC;
	}

	return ret;

FAIL_PROC:
	nvt_ctl_ipp_proc_remove(pctl_ipp_info);

FAIL_INIT:
	kfree(pctl_ipp_info);
	return 0;
}

void __exit nvt_ctl_ipp_exit(void)
{
	nvt_ctl_ipp_drv_release(&pctl_ipp_info->module_info, 0);

	nvt_ctl_ipp_proc_remove(pctl_ipp_info);

	kfree(pctl_ipp_info);
}

module_init(nvt_ctl_ipp_init);
module_exit(nvt_ctl_ipp_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("ctl_ipp driver");
MODULE_LICENSE("GPL");
