#ifndef _FASTBOOT_MAIN_H__
#define _FASTBOOT_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>

#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "fastboot"

typedef struct fastboot_drv_info {
	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_cmd_entry;;
} FASTBOOT_DRV_INFO, *PFASTBOOT_DRV_INFO;

#endif

