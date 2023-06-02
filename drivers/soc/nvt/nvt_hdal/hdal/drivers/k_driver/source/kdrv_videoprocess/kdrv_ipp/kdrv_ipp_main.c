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
#include "kdrv_ipp_int_dbg.h"
#include "kdrv_ipp_int_main.h"
#include "kdrv_ipp_int_proc.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
//#ifdef DEBUG
#if 0
unsigned int kdrv_ipp_debug_level = NVT_DBG_WRN;
module_param_named(kdrv_ipp_debug_level, kdrv_ipp_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(kdrv_ipp_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================
#if 0
static struct of_device_id kdrv_ipp_match_table[] = {
	{   .compatible = "nvt,kdrv_kdrv_ipp"},
	{}
};
#endif
static KDRV_IPP_DRV_INFO *pdrv_ipp_info = NULL;

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

int __init nvt_kdrv_ipp_module_init(void);
void __exit nvt_kdrv_ipp_module_exit(void);

//=============================================================================
// function define
//=============================================================================

int __init nvt_kdrv_ipp_module_init(void)
{

	int ret;

	DBG_IND("\n");

	pdrv_ipp_info = kzalloc(sizeof(KDRV_IPP_DRV_INFO), GFP_KERNEL);
	if (!pdrv_ipp_info) {
		DBG_ERR("failed to allocate memory\n");
		return -ENOMEM;
	}

	ret = nvt_kdrv_ipp_proc_init(pdrv_ipp_info);
	if (ret) {
		DBG_ERR("failed in creating proc.\n");

		kfree(pdrv_ipp_info);

		pdrv_ipp_info = NULL;
		return -1;
	}

	DBG_IND("nvt_kdrv_ipp_module_init, done\r\n");

	return 0;
}

void __exit nvt_kdrv_ipp_module_exit(void)
{
	int ret;

	ret = nvt_kdrv_ipp_proc_remove(pdrv_ipp_info);
	kfree(pdrv_ipp_info);
	pdrv_ipp_info = NULL;
	DBG_IND("nvt_kdrv_ipp_module_exit...\n");
}

module_init(nvt_kdrv_ipp_module_init);
module_exit(nvt_kdrv_ipp_module_exit);


MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("kdrv_ipp k-driver");
MODULE_LICENSE("GPL");

