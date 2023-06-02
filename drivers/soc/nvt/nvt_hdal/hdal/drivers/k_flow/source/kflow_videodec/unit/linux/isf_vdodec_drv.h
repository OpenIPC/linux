#ifndef __ISF_VDODEC_DRV_H
#define __ISF_VDODEC_DRV_H
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          0
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          0

typedef struct _ISF_VDODEC_INFO {
	int reversed;
} ISF_VDODEC_INFO, *PISF_VDODEC_INFO;

int isf_vdodec_drv_open(ISF_VDODEC_INFO *pmodule_info, unsigned char minor);
int isf_vdodec_drv_release(ISF_VDODEC_INFO *pmodule_info, unsigned char minor);
int isf_vdodec_drv_init(ISF_VDODEC_INFO *pmodule_info);
int isf_vdodec_drv_remove(ISF_VDODEC_INFO *pmodule_info);
int isf_vdodec_drv_suspend(ISF_VDODEC_INFO *pmodule_info);
int isf_vdodec_drv_resume(ISF_VDODEC_INFO *pmodule_info);
int isf_vdodec_drv_ioctl(unsigned char minor, ISF_VDODEC_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg);
#endif

