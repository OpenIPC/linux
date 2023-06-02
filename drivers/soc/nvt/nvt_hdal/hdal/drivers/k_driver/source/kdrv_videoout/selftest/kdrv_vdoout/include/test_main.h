#ifndef __MODULE_MAIN_H__
#define __MODULE_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "test_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "nvt_test"

typedef struct test_drv_info {
	TEST_MODULE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;

	struct task_struct 	*emu_thread;

	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} TEST_DRV_INFO, *PTEST_DRV_INFO;


#endif
