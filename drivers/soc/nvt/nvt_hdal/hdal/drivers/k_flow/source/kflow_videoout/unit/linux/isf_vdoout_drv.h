#ifndef __ISF_VDOOUT_DRV_H
#define __ISF_VDOOUT_DRV_H
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

typedef struct _ISF_VDOOUT_INFO {
	int reversed;
} ISF_VDOOUT_INFO, *PISF_VDOOUT_INFO;

int isf_vdoout_drv_open(ISF_VDOOUT_INFO *pmodule_info, unsigned char minor);
int isf_vdoout_drv_release(ISF_VDOOUT_INFO *pmodule_info, unsigned char minor);
int isf_vdoout_drv_init(ISF_VDOOUT_INFO *pmodule_info);
int isf_vdoout_drv_remove(ISF_VDOOUT_INFO *pmodule_info);
int isf_vdoout_drv_suspend(ISF_VDOOUT_INFO *pmodule_info);
int isf_vdoout_drv_resume(ISF_VDOOUT_INFO *pmodule_info);
int isf_vdoout_drv_ioctl(unsigned char minor, ISF_VDOOUT_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg);
#endif

