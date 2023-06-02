#ifndef __ISF_AUDOUT_MAIN_H__
#define __ISF_AUDOUT_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "../include/isf_audout_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "isf_audout"

typedef struct ISF_AUDOUT_DRV_INFO {
	ISF_AUDOUT_INFO module_info;

	struct class *p_module_class;
	struct device *p_device[MODULE_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *p_proc_module_root;
	struct proc_dir_entry *p_proc_help_entry;
	struct proc_dir_entry *p_proc_cmd_entry;
	struct proc_dir_entry *p_proc_info_entry;
} ISF_AUDOUT_DRV_INFO, *PISF_AUDOUT_DRV_INFO;


#endif
