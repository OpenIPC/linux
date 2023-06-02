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
#include <linux/mm.h>
#include "fastboot_main.h"
#include "fastboot_proc.h"

#include <kwrap/debug.h>

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


FASTBOOT_DRV_INFO *gp_drv_info = NULL;

int __init fastboot_module_init(void)
{
	gp_drv_info = kzalloc(sizeof(FASTBOOT_DRV_INFO), GFP_KERNEL);
	if (!gp_drv_info) {
		DBG_ERR("failed to allocate memory\n");
		return -ENOMEM;
	}
	fastboot_proc_init(gp_drv_info);
	return 0;
}

void __exit fastboot_module_exit(void)
{
	if (gp_drv_info) {
		fastboot_proc_exit(gp_drv_info);
		kfree(gp_drv_info);
		gp_drv_info = NULL;
	}
}

module_init(fastboot_module_init);
module_exit(fastboot_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("nvtmpp driver");
MODULE_LICENSE("GPL");
