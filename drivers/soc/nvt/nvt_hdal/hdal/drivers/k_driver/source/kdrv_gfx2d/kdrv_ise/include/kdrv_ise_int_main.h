#ifndef __KDRV_ISE_INT_MAIN_H__
#define __KDRV_ISE_INT_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "kdrv_ise_int_drv.h"


#define NVT_KDRV_ISE_MINOR_ID      0
#define NVT_KDRV_ISE_MINOR_COUNT   1
#define NVT_KDRV_ISE_NAME          "kdrv_ise"

typedef struct _nvt_kdrv_ise_drv_info {
	NVT_KDRV_ISE_INFO m_info;

	struct class *pclass;
	struct device *pdevice[NVT_KDRV_ISE_MINOR_COUNT];
	struct resource *presource[NVT_KDRV_ISE_REG_NUM];
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} NVT_KDRV_ISE_DRV_INFO, *PNVT_KDRV_ISE_DRV_INFO;


#endif
