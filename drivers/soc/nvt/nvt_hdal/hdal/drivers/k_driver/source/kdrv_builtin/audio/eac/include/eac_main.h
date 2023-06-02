#ifndef __EAC_MAIN_H__
#define __EAC_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "eac_drv.h"


#define EAC_MINOR_ID			0
#define EAC_MINOR_COUNT			1
#define MODULE_NAME				"nvt_eac"

typedef struct eac_drv_info {
	EAC_MODULE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[EAC_MINOR_COUNT];
	struct resource *presource[EAC_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} EAC_DRV_INFO, *PEAC_DRV_INFO;


#endif
