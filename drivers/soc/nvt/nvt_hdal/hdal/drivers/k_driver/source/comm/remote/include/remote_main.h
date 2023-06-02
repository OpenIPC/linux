#ifndef _REMOTE_MAIN_H_
#define _REMOTE_MAIN_H_
#include <linux/cdev.h>
#include <linux/types.h>
#include "remote_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "nvt_remote"

typedef struct remote_drv_info {
	REMOTE_MODULE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource* presource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

    // proc entries
    struct proc_dir_entry *pproc_module_root;
    struct proc_dir_entry *pproc_help_entry;
    struct proc_dir_entry *pproc_cmd_entry;
} REMOTE_DRV_INFO, *PREMOTE_DRV_INFO;


#endif