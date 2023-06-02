#ifndef __MODULE_MAIN_H__
#define __MODULE_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "graphic_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   2
#define MODULE_NAME          "nvt_graphic"

typedef struct xxx_drv_info {
	MODULE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource *presource[MODULE_REG_NUM];
	struct cdev cdev;
	struct device *dev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} XXX_DRV_INFO, *PXXX_DRV_INFO;


#endif
