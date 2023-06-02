#ifndef __IVE_PROC_H_
#define __IVE_PROC_H__
#include "ive_main.h"

#if !defined(CONFIG_NVT_SMALL_HDAL)
int nvt_ive_proc_init(PIVE_DRV_INFO pdrv_info);
int nvt_ive_proc_remove(PIVE_DRV_INFO pdrv_info);
#endif

#endif
