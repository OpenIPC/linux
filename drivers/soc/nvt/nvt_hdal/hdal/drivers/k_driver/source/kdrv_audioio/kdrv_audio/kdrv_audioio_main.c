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
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/random.h>
#include <asm/signal.h>

#include "kdrv_audioio_drv.h"
#include "kdrv_audioio_main.h"
#include "kdrv_audioio_dbg.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
unsigned int kdrv_audioio_debug_level = NVT_DBG_ERR;
module_param_named(kdrv_audioio_debug_level, kdrv_audioio_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(kdrv_audioio_debug_level, "Debug message level");
#endif

//=============================================================================
// Global variable
//=============================================================================

//=============================================================================
// function declaration
//=============================================================================
int __init nvt_kdrv_audioio_module_init(void);
void __exit nvt_kdrv_audioio_module_exit(void);

//=============================================================================
// function define
//=============================================================================




static struct platform_driver nvt_kdrv_audioio_driver = {
	.driver = {
		.name   = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

int __init nvt_kdrv_audioio_module_init(void)
{
	int ret;

	nvt_dbg(WRN, "\n");
	ret = platform_driver_register(&nvt_kdrv_audioio_driver);

	return 0;
}

void __exit nvt_kdrv_audioio_module_exit(void)
{
	nvt_dbg(WRN, "\n");
	platform_driver_unregister(&nvt_kdrv_audioio_driver);
}

module_init(nvt_kdrv_audioio_module_init);
module_exit(nvt_kdrv_audioio_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("audlib_emu driver");
MODULE_LICENSE("GPL");
