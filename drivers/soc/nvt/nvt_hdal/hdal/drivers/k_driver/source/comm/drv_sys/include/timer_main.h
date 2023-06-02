#ifndef __MODULE_MAIN_H__
#define __MODULE_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "timer_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "nvt_timer_module"

typedef struct xxx_drv_info {
	MODULE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource *presource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
	struct proc_dir_entry *pproc_silence_entry;

	struct proc_dir_entry *pproc_ddr_root;
	struct proc_dir_entry *pproc_dram1_entry;
	struct proc_dir_entry *pproc_dram2_entry;

	struct proc_dir_entry *pproc_heavload_module_root;
	struct proc_dir_entry *pproc_heavload_help_entry;
	struct proc_dir_entry *pproc_heavload_cmd_entry;
} XXX_DRV_INFO, *PXXX_DRV_INFO;


#endif
