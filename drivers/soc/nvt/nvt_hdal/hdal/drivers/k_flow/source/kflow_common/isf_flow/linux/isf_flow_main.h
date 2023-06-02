#ifndef __ISF_FLOW_MAIN_H
#define __ISF_FLOW_MAIN_H
#include <linux/cdev.h>
#include <linux/types.h>
#include "isf_flow_drv.h"

#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   8
#define MODULE_NAME          "isf_flow"

typedef struct _ISF_FLOW_DRV_INFO {
	MODULE_INFO module_info;

	struct class *pmodule_class;
	struct device *p_device[MODULE_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *p_proc_module_root;
	struct proc_dir_entry *p_proc_help_entry;
	struct proc_dir_entry *p_proc_cmd_entry;
	
	char minor_base;
	int pid[MODULE_MINOR_COUNT];

} ISF_FLOW_DRV_INFO, *PISF_FLOW_DRV_INFO;


#endif
