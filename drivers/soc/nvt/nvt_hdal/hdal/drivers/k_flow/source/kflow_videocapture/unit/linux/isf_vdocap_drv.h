#ifndef __ISF_VDOCAP_DRV_H
#define __ISF_VDOCAP_DRV_H
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          0
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          0

typedef struct _MODULE_INFO {
	int reversed;
} MODULE_INFO, *PMODULE_INFO;

int isf_vdocap_drv_open(MODULE_INFO *pmodule_info, unsigned char minor);
int isf_vdocap_drv_release(MODULE_INFO *pmodule_info, unsigned char minor);
int isf_vdocap_drv_init(MODULE_INFO *pmodule_info);
int isf_vdocap_drv_remove(MODULE_INFO *pmodule_info);
int isf_vdocap_drv_suspend(MODULE_INFO *pmodule_info);
int isf_vdocap_drv_resume(MODULE_INFO *pmodule_info);
int isf_vdocap_drv_ioctl(unsigned char minor, MODULE_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg);
#endif

