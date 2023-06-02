#ifndef __ISF_VDOOUT_MAIN_H
#define __ISF_VDOOUT_MAIN_H
#include <linux/cdev.h>
#include <linux/types.h>
#include "isf_vdoout_drv.h"

#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "isf_vdoout"

typedef struct _ISF_VDOOUT_DRV_INFO {
	ISF_VDOOUT_INFO module_info;

	struct class *pmodule_class;
	struct device *p_device[MODULE_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *p_proc_module_root;
	struct proc_dir_entry *p_proc_help_entry;
	struct proc_dir_entry *p_proc_cmd_entry;
	struct proc_dir_entry *p_proc_info_entry;
} ISF_VDOOUT_DRV_INFO, *PISF_VDOOUT_DRV_INFO;


#endif
