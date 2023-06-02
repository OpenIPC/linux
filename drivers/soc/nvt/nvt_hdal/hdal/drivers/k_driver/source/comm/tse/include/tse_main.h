#ifndef __TSE_MAIN_H__
#define __TSE_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "tse_drv.h"


#define TSE_MINOR_ID      0
#define TSE_MINOR_COUNT   1
#define TSE_NAME          "nvt_drv_tse"
#define TSE_VERSION		  "1.0.00"

typedef struct tse_drv_info {
	MODULE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[TSE_MINOR_COUNT];
	struct resource *presource[TSE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_cmd_entry;
} TSE_DRV_INFO, *PTSE_DRV_INFO;


#endif //__TSE_MAIN_H__