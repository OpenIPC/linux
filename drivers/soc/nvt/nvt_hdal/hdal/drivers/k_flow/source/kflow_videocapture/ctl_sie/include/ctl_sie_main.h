#ifndef __MODULE_MAIN_H__
#define __MODULE_MAIN_H__
#include <linux/cdev.h>
#include "ctl_sie_drv.h"
#include <kwrap/type.h>


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "nvt_ctl_sie"

typedef struct ctl_sie_drv_info {
	MODULE_INFO module_info;

	struct class *pmodule_class;
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
	struct proc_dir_entry *pproc_info_entry;

	struct seq_file *info_seq_file;
} CTL_SIE_DRV_INFO, *PCTL_SIE_DRV_INFO;


#endif
