#ifndef __CTL_SEN_DRV_H__
#define __CTL_SEN_DRV_H__
#include "kflow_videocapture/ctl_sen.h"

#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
#else
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>
#endif

typedef struct ctl_sen_info {
} CTL_SEN_MODULE_INFO, *PCTL_SEN_MODULE_INFO;

int nvt_ctl_sen_drv_open(PCTL_SEN_MODULE_INFO pctl_sen_info, unsigned char uc_if);
int nvt_ctl_sen_drv_release(PCTL_SEN_MODULE_INFO pctl_sen_info, unsigned char uc_if);
int nvt_ctl_sen_drv_init(PCTL_SEN_MODULE_INFO pctl_sen_info);
#endif

