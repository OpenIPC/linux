#ifndef __USB_CHRGDET_MAIN_H__
#define __USB_CHRGDET_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "usb_chrgdet_drv.h"


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "nvt_usb_chrgdet"

typedef struct usb_chrgdet_drv_info {
	USB_CHRGDET_MODULE_INFO module_info;

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
} USB_CHRGDET_DRV_INFO, *PUSB_CHRGDET_DRV_INFO;


#endif
