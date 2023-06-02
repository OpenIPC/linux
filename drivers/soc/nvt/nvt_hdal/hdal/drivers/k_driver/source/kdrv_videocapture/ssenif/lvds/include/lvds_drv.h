#ifndef __LVDS_DRV_H__
#define __LVDS_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          2
#define MODULE_REG_NUM          2
#define MODULE_CLK_NUM          2

#define LVDS_INT_MODE_TASKLET	0
#define LVDS_INT_MODE_ISR		1
#define LVDS_INT_MODE			LVDS_INT_MODE_ISR

#define WRITE_REG(value, addr)  iowrite32(value, addr)
#define READ_REG(addr)          ioread32(addr)

typedef struct lvds_module_info {
	struct clk *pclk[MODULE_CLK_NUM];
#if (LVDS_INT_MODE == LVDS_INT_MODE_TASKLET)
	struct tasklet_struct lvds_tasklet[MODULE_IRQ_NUM];
#endif
	void __iomem *io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
} LVDS_MODULE_INFO, *PLVDS_MODULE_INFO;

int nvt_lvds_drv_open(PLVDS_MODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_lvds_drv_release(PLVDS_MODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_lvds_drv_init(PLVDS_MODULE_INFO pmodule_info);
int nvt_lvds_drv_remove(PLVDS_MODULE_INFO pmodule_info);
int nvt_lvds_drv_suspend(LVDS_MODULE_INFO *pmodule_info);
int nvt_lvds_drv_resume(LVDS_MODULE_INFO *pmodule_info);
int nvt_lvds_drv_ioctl(unsigned char uc_if, LVDS_MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg);
int nvt_lvds_drv_write_reg(PLVDS_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_lvds_drv_read_reg(PLVDS_MODULE_INFO pmodule_info, unsigned long addr);
#endif

