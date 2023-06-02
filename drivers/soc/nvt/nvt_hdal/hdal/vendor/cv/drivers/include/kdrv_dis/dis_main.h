#ifndef __DIS_MAIN_H__
#define __DIS_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "dis_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "kdrv_dis"

typedef struct dis_drv_info {
	DIS_MODULE_INFO module_info;

	struct class *p_module_class;
	struct device *p_device[MODULE_MINOR_COUNT];
	struct resource *p_resource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *p_proc_module_root;
	struct proc_dir_entry *p_proc_help_entry;
	struct proc_dir_entry *p_proc_cmd_entry;
} DIS_DRV_INFO, *PDIS_DRV_INFO;


#endif
