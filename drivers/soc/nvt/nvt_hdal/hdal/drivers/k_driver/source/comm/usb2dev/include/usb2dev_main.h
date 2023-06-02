#ifndef __USB2DEV_MAIN_H__
#define __USB2DEV_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "usb2dev_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "nvt_usb2dev"

typedef struct usb2dev_drv_info {
	USB2DEV_MODULE_INFO module_info;

	struct class *pmodule_class;
	struct device *pdevice[MODULE_MINOR_COUNT];
	struct resource *presource[MODULE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
	struct proc_dir_entry *pproc_u2state_entry;
} USB2DEV_DRV_INFO, *PUSB2DEV_DRV_INFO;


#endif
