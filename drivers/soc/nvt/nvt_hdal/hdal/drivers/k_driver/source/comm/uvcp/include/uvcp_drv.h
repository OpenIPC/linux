#ifndef __UVCP_DRV_H__
#define __UVCP_DRV_H__
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

typedef struct uvcp_module_info {
	struct clk *pclk[MODULE_CLK_NUM];
	void __iomem *io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
} UVCP_MODULE_INFO, *PUVCP_MODULE_INFO;

int nvt_uvcp_drv_open(PUVCP_MODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_uvcp_drv_release(PUVCP_MODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_uvcp_drv_init(PUVCP_MODULE_INFO pmodule_info);
int nvt_uvcp_drv_remove(PUVCP_MODULE_INFO pmodule_info);
//int nvt_uvcp_drv_suspend(UVCP_MODULE_INFO *pmodule_info);
//int nvt_uvcp_drv_resume(UVCP_MODULE_INFO *pmodule_info);
int nvt_uvcp_drv_ioctl(unsigned char uc_if, UVCP_MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg);
#endif

