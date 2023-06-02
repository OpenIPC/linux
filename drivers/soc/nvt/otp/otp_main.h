#ifndef __MODULE_OTP_MAIN_H__
#define __MODULE_OTP_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "otp_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "nvt_otp"

typedef struct otp_drv_info {
	MODULE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource *presource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_avl_entry;
	struct proc_dir_entry *pproc_otp_entry;

	struct proc_dir_entry *pproc_otp_root;
//	struct proc_dir_entry *pproc_dram1_entry;
//	struct proc_dir_entry *pproc_dram2_entry;
//	struct proc_dir_entry *pproc_dram1_heavyload_entry;
//	struct proc_dir_entry *pproc_dram2_heavyload_entry;
} OTP_DRV_INFO, *POTP_DRV_INFO;


#endif
