#ifndef __TSE_DRV_H__
#define __TSE_DRV_H__

#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define TSE_IRQ_NUM          1
#define TSE_REG_NUM          1
#define TSE_CLK_NUM          1

typedef struct module_info {
	struct clk *pclk[TSE_CLK_NUM];
	void __iomem *io_addr[TSE_REG_NUM];
	int interrupt_id[TSE_IRQ_NUM];
} MODULE_INFO, *PMODULE_INFO;

int nvt_tse_drv_open(PMODULE_INFO pmodule_info, unsigned char if_id);
int nvt_tse_drv_release(PMODULE_INFO pmodule_info, unsigned char if_id);
int nvt_tse_drv_init(PMODULE_INFO pmodule_info);
int nvt_tse_drv_remove(PMODULE_INFO pmodule_info);
int nvt_tse_drv_suspend(MODULE_INFO *pmodule_info);
int nvt_tse_drv_resume(MODULE_INFO *pmodule_info);
int nvt_tse_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long argc);
#endif //__TSE_DRV_H__

