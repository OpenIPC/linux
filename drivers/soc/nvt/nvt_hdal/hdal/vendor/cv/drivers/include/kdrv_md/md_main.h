#ifndef __MD_MAIN_H__
#define __MD_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "md_drv.h"


#define MD_MINOR_ID      0
#define MD_MINOR_COUNT   1
#define MD_NAME          "kdrv_md"

typedef struct md_drv_info {
	MD_MODULE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[MD_MINOR_COUNT];
	struct resource* presource[MD_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

    // proc entries
    struct proc_dir_entry *pproc_module_root;
    struct proc_dir_entry *pproc_help_entry;
    struct proc_dir_entry *pproc_cmd_entry;
} MD_DRV_INFO, *PMD_DRV_INFO;


#endif
