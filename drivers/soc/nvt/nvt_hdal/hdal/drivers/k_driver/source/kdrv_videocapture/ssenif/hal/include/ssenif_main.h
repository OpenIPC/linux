#ifndef __MODULE_MAIN_H__
#define __MODULE_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "ssenif_drv.h"


#define SLVSEC_MINOR_ID      0
#define SLVSEC_MINOR_COUNT   1
#define SLVSEC_NAME          "nvt_slvsec"

typedef struct dal_ssenif_drv_info {
	SSENIF_MODULE_INFO module_info;

	//struct class *pmodule_class;
	//struct device *slvsec_pdevice[SLVSEC_MINOR_COUNT];
	//struct resource *slvsec_presource[SLVSEC_REG_NUM];
	//struct cdev cdev;
	//dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} SSENIF_DRV_INFO, *PSSENIF_DRV_INFO;


#endif
