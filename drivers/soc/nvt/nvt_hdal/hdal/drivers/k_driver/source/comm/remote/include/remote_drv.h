#ifndef _REMOTE_DRV_H_
#define _REMOTE_DRV_H_
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>
#include "remote_ioctl.h"

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          1

#define WRITE_REG(VALUE, ADDR)  iowrite32(VALUE, ADDR)
#define READ_REG(ADDR)          ioread32(ADDR)

typedef struct remote_module_info {
	struct completion remote_completion;
	struct semaphore remote_sem;
	struct clk* pclk[MODULE_CLK_NUM];
	struct tasklet_struct remote_tasklet;
	void __iomem* io_addr[MODULE_REG_NUM];
    int iinterrupt_id[MODULE_IRQ_NUM];
	wait_queue_head_t remote_wait_queue;
	spinlock_t remote_spinlock;
} REMOTE_MODULE_INFO, *PREMOTE_MODULE_INFO;

int nvt_remote_drv_open(PREMOTE_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_remote_drv_release(PREMOTE_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_remote_drv_init(PREMOTE_MODULE_INFO pmodule_info);
int nvt_remote_drv_remove(PREMOTE_MODULE_INFO pmodule_info);
int nvt_remote_drv_suspend(REMOTE_MODULE_INFO* pmodule_info);
int nvt_remote_drv_resume(REMOTE_MODULE_INFO* pmodule_info);
int nvt_remote_drv_ioctl(unsigned char ucIF, REMOTE_MODULE_INFO* pmodule_info, unsigned int uiCmd, unsigned long ulArg);
int nvt_remote_drv_write_reg(PREMOTE_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_remote_drv_read_reg(PREMOTE_MODULE_INFO pmodule_info, unsigned long addr);
int nvt_remote_drv_set_en(BOOL en);
int nvt_remote_drv_set_config(REMOTE_CONFIG_INFO *config_info);
int nvt_remote_drv_set_interrupt_en(REMOTE_INTERRUPT int_en);
#endif
