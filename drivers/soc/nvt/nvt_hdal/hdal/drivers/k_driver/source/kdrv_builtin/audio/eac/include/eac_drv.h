#ifndef __EAC_DRV_H__
#define __EAC_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#include "eac_platform.h"

#define WRITE_REG(value, addr)  iowrite32(value, addr)
#define READ_REG(addr)          ioread32(addr)

typedef struct eac_module_info {
	void __iomem *io_addr[EAC_REG_NUM];
} EAC_MODULE_INFO, *PEAC_MODULE_INFO;

int nvt_eac_drv_open(PEAC_MODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_eac_drv_release(PEAC_MODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_eac_drv_init(PEAC_MODULE_INFO pmodule_info);
int nvt_eac_drv_remove(PEAC_MODULE_INFO pmodule_info);
int nvt_eac_drv_suspend(EAC_MODULE_INFO *pmodule_info);
int nvt_eac_drv_resume(EAC_MODULE_INFO *pmodule_info);
int nvt_eac_drv_ioctl(unsigned char uc_if, EAC_MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg);
int nvt_eac_drv_write_reg(PEAC_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_eac_drv_read_reg(PEAC_MODULE_INFO pmodule_info, unsigned long addr);
#endif

