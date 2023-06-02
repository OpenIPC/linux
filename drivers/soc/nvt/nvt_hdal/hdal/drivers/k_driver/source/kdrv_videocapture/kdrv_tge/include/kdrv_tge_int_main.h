#ifndef _KDRV_TGE_INT_MAIN_H_
#define _KDRV_TGE_INT_MAIN_H_
#include <linux/cdev.h>
#include <linux/types.h>
#include "kdrv_tge_int_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "kdrv_tge"

typedef struct tge_drv_info {
	TGE_MODULE_INFO module_info;
	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource* presource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

    // proc entries
    struct proc_dir_entry *pproc_module_root;
    struct proc_dir_entry *pproc_help_entry;
    struct proc_dir_entry *pproc_cmd_entry;
} TGE_DRV_INFO, *PTGE_DRV_INFO;


#endif //_KDRV_TGE_INT_MAIN_H_