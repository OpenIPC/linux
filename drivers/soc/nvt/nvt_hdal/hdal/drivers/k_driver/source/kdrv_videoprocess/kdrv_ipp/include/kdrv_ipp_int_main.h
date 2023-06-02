#ifndef __KDRV_IPP_INT_MAIN_H__
#define __KDRV_IPP_INT_MAIN_H__

#include <linux/cdev.h>
#include <linux/types.h>


#define KDRV_IPP_MINOR_ID      0
#define KDRV_IPP_MINOR_COUNT   1
#define KDRV_IPP_NAME          "kdrv_ipp"

typedef struct kdrv_ipp_drv_info {
	struct class *pclass;
	struct device *pdevice[KDRV_IPP_MINOR_COUNT];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_kdrv_ipp_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} KDRV_IPP_DRV_INFO, *PKDRV_IPP_DRV_INFO;


#endif

