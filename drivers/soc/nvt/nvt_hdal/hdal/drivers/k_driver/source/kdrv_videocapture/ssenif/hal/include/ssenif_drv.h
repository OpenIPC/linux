#ifndef __MODULE_DRV_H__
#define __MODULE_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define SLVSEC_IRQ_NUM          0
#define SLVSEC_REG_NUM          0
#define SLVSEC_CLK_NUM          0

#define WRITE_REG(value, addr)  iowrite32(value, addr)
#define READ_REG(addr)          ioread32(addr)


typedef struct ssenif_module_info {
	//void __iomem *slvsec_io_addr[SLVSEC_REG_NUM];

	//struct clk *slvsec_pclk[SLVSEC_CLK_NUM];

	//int slvsec_iinterrupt_id[SLVSEC_IRQ_NUM];
	int dummy;
} SSENIF_MODULE_INFO, *PSSENIF_MODULE_INFO;



int nvt_ssenif_drv_open(PSSENIF_MODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_ssenif_drv_release(PSSENIF_MODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_ssenif_drv_init(PSSENIF_MODULE_INFO pmodule_info);
int nvt_ssenif_drv_remove(PSSENIF_MODULE_INFO pmodule_info);
int nvt_ssenif_drv_suspend(SSENIF_MODULE_INFO *pmodule_info);
int nvt_ssenif_drv_resume(SSENIF_MODULE_INFO *pmodule_info);
int nvt_ssenif_drv_ioctl(unsigned char uc_if, SSENIF_MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg);
int nvt_ssenif_drv_write_reg(PSSENIF_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_ssenif_drv_read_reg(PSSENIF_MODULE_INFO pmodule_info, unsigned long addr);

#endif

