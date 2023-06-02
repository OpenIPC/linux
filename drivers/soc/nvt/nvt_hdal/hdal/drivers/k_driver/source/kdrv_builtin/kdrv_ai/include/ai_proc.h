#ifndef __AI_PROC_H_
#define __AI_PROC_H__
#include "ai_main.h"
#ifdef __KERNEL__
int nvt_ai_proc_init(PAI_DRV_INFO pdrv_info);
int nvt_ai_proc_remove(PAI_DRV_INFO pdrv_info);
#endif

#endif