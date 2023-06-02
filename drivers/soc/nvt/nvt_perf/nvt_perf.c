#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include "nvt_perf.h"
#include "nvt_perf_dbg.h"

#define PERF_DEV_NAME      "nvt_perf"
#define PERF_DRV_VERSION   "00.00.04"

unsigned int nvt_perf_debug_level = NVT_PERF_DBG_WRN | NVT_PERF_DBG_ERR;
extern int do_cache_perf(void);

static long perf_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	switch (cmd) {

	case NVT_PERF_CACHE:
		do_cache_perf();
		break;

	default:
		nvt_dbg(ERR, "unknow ioctl type:0x%x\n", cmd);
	}

	return 0;
}

static struct file_operations nvt_perf_fops = {
	.owner		    = THIS_MODULE,
    .unlocked_ioctl = perf_file_ioctl,
};

static struct miscdevice nvt_perf_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= PERF_DEV_NAME,
	.fops		= &nvt_perf_fops,
};

int __init nvt_perf_init(void)
{
	int ret = 0;
	nvt_dbg(ERR, "NVT_PERF version:%s\n", PERF_DRV_VERSION);


	if ((ret = misc_register(&nvt_perf_misc_device))){
		nvt_dbg(ERR, "Failed to register misc device '%s'\n", nvt_perf_misc_device.name);
		goto ERR_MISC;
	}

ERR_MISC:
	return ret;
}

void __exit nvt_perf_exit(void)
{
	misc_deregister(&nvt_perf_misc_device);
}

module_init(nvt_perf_init);
module_exit(nvt_perf_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("nvt perf");
MODULE_LICENSE("Proprietary");
