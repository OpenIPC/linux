#ifndef __KDRV_IPE_INT_MAIN_H__
#define __KDRV_IPE_INT_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "kdrv_ipe_int_init.h"


typedef struct {
	struct device *p_device[NVT_KDRV_IPE_TOTAL_CH_COUNT];
	struct class *pclass;
	struct cdev cdev;
	dev_t dev_id;

	// proc entries
	struct proc_dir_entry *pproc_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} NVT_KDRV_IPE_DRV_INFO, *PNVT_KDRV_IPE_DRV_INFO;


#endif //__KDRV_IPE_INT_MAIN_H__