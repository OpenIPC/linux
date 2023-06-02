#ifndef __ISF_AUDDEC_MAIN_H
#define __ISF_AUDDEC_MAIN_H
#include <linux/cdev.h>
#include <linux/types.h>
#include "isf_auddec_drv.h"

#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "isf_auddec"

typedef struct _ISF_AUDDEC_DRV_INFO {
	ISF_AUDDEC_INFO module_info;

	struct class *pmodule_class;
	struct device *p_device[MODULE_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *p_proc_module_root;
	struct proc_dir_entry *p_proc_info_entry;
	struct proc_dir_entry *p_proc_cmd_entry;
	struct proc_dir_entry *p_proc_help_entry;
} ISF_AUDDEC_DRV_INFO, *PISF_AUDDEC_DRV_INFO;


#endif
