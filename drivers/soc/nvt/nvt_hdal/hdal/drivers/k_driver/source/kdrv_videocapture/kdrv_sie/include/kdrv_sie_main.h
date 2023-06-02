#ifndef __KDRV_SIE_MAIN_H__
#define __KDRV_SIE_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "sie_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "kdrv_sie"

typedef struct kdrv_sie_drv_info {
	SIE_MODULE_INFO module_info;
	struct class *pmodule_class;
	struct cdev cdev;
	dev_t dev_id;

    // proc entries
    struct proc_dir_entry *pproc_module_root;
    struct proc_dir_entry *pproc_help_entry;
    struct proc_dir_entry *pproc_cmd_entry;
} KDRV_SIE_DRV_INFO, *PKDRV_SIE_DRV_INFO;


#endif
