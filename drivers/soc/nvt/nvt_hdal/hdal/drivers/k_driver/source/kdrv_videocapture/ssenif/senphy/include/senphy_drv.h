#ifndef __SENPHY_DRV_H__
#define __SENPHY_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          0
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          0

#define WRITE_REG(value, addr)  iowrite32(value, addr)
#define READ_REG(addr)          ioread32(addr)

typedef struct senphy_module_info {
	void __iomem *io_addr[MODULE_REG_NUM];
} SENPHY_MODULE_INFO, *PSENPHY_MODULE_INFO;

int nvt_senphy_drv_open(PSENPHY_MODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_senphy_drv_release(PSENPHY_MODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_senphy_drv_init(PSENPHY_MODULE_INFO pmodule_info);
int nvt_senphy_drv_remove(PSENPHY_MODULE_INFO pmodule_info);
int nvt_senphy_drv_suspend(SENPHY_MODULE_INFO *pmodule_info);
int nvt_senphy_drv_resume(SENPHY_MODULE_INFO *pmodule_info);
int nvt_senphy_drv_ioctl(unsigned char uc_if, SENPHY_MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg);
int nvt_senphy_drv_write_reg(PSENPHY_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_senphy_drv_read_reg(PSENPHY_MODULE_INFO pmodule_info, unsigned long addr);
#endif

