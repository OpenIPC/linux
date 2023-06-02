#ifndef __DSI_DRV_H__
#define __DSI_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          1

#define WRITE_REG(value, addr)  iowrite32(value, addr)
#define READ_REG(addr)          ioread32(addr)

typedef struct module_info {
	struct completion dsi_completion;
	struct semaphore dsi_sem;
	struct clk *pclk[MODULE_CLK_NUM];
	struct tasklet_struct dsi_tasklet;
	void __iomem *io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
	wait_queue_head_t dsi_wait_queue;
	spinlock_t dsi_spinlock;
} MODULE_INFO, *PMODULE_INFO;

int nvt_dsi_drv_open(PMODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_dsi_drv_release(PMODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_dsi_drv_init(PMODULE_INFO pmodule_info);
int nvt_dsi_drv_remove(PMODULE_INFO pmodule_info);
int nvt_dsi_drv_suspend(MODULE_INFO *pmodule_info);
int nvt_dsi_drv_resume(MODULE_INFO *pmodule_info);
int nvt_dsi_drv_ioctl(unsigned char uc_if, MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg);
int nvt_dsi_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_dsi_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr);
#endif

