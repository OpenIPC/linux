#ifndef __KDRV_IME_MAIN_H__
#define __KDRV_IME_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>
#include "kdrv_ime_int_init.h"

typedef struct {
	// proc entries
	struct proc_dir_entry *pproc_root;
	struct proc_dir_entry *pproc_help_entry;
	struct proc_dir_entry *pproc_cmd_entry;
} NVT_KDRV_IME_DRV_INFO, *PNVT_KDRV_IME_DRV_INFO;


#endif
