#ifndef __MODULE_DRV_H__
#define __MODULE_DRV_H__

#if defined __LINUX
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>
#endif
#include <kwrap/spinlock.h>
#include <kwrap/semaphore.h>

typedef struct module_info {

} MODULE_INFO, *PMODULE_INFO;

int nvt_ctl_sie_drv_release(PMODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_ctl_sie_drv_init(PMODULE_INFO pmodule_info);
int nvt_ctl_sie_drv_remove(PMODULE_INFO pmodule_info);
int nvt_ctl_sie_drv_suspend(MODULE_INFO *pmodule_info);
int nvt_ctl_sie_drv_resume(MODULE_INFO *pmodule_info);
#endif

