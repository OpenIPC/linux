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
#include <asm/signal.h>

int __init dummy_module_init(void)
{
	return 0;
}

void __exit dummy_module_exit(void)
{
}

module_init(dummy_module_init);
module_exit(dummy_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("dummy sample driver");
MODULE_LICENSE("Proprietary");
