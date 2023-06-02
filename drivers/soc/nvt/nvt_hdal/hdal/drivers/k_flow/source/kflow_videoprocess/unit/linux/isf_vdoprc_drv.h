#ifndef __ISF_VDOPRC_DRV_H
#define __ISF_VDOPRC_DRV_H
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          0
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          0

typedef struct module_info {
	int reversed;
} MODULE_INFO, *PMODULE_INFO;

int isf_vdoprc_drv_open(PMODULE_INFO pmodule_info, unsigned char minor);
int isf_vdoprc_drv_release(PMODULE_INFO pmodule_info, unsigned char minor);
int isf_vdoprc_drv_init(PMODULE_INFO pmodule_info);
int isf_vdoprc_drv_remove(PMODULE_INFO pmodule_info);
int isf_vdoprc_drv_suspend(PMODULE_INFO pmodule_info);
int isf_vdoprc_drv_resume(PMODULE_INFO pmodule_info);
int isf_vdoprc_drv_ioctl(unsigned char minor, PMODULE_INFO pmodule_info, unsigned int cmd_id, unsigned long arg);
#endif

