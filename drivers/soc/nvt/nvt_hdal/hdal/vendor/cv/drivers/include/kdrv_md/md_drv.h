#ifndef __MD_DRV_H__
#define __MD_DRV_H__
#include <linux/io.h>
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MD_IRQ_NUM          1
#define MD_REG_NUM          1
#define MD_CLK_NUM          1

#define WRITE_REG(Value, Addr)  iowrite32(Value, Addr)
#define READ_REG(Addr)          ioread32(Addr)
extern UINT32 md_freq_from_dtsi[MD_CLK_NUM];
typedef struct md_module_info {
	struct completion md_completion;
	struct vk_semaphore md_sem;
	struct clk* pclk[MD_CLK_NUM];
	struct tasklet_struct md_tasklet;
	void __iomem* io_addr[MD_REG_NUM];
    int iinterrupt_id[MD_IRQ_NUM];
    wait_queue_head_t md_wait_queue;
	vk_spinlock_t md_spinlock;
}MD_MODULE_INFO, *PMD_MODULE_INFO;

extern struct clk *mdbc_clk[MD_CLK_NUM];

int nvt_md_drv_open(PMD_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_md_drv_release(PMD_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_md_drv_init(PMD_MODULE_INFO pmodule_info);
int nvt_md_drv_remove(PMD_MODULE_INFO pmodule_info);
int nvt_md_drv_suspend(MD_MODULE_INFO* pmodule_info);
int nvt_md_drv_resume(MD_MODULE_INFO* pmodule_info);
int nvt_md_drv_ioctl(unsigned char ucIF, MD_MODULE_INFO* pmodule_info, unsigned int uiCmd, unsigned long ulArg);
int nvt_md_drv_write_reg(PMD_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_md_drv_read_reg(PMD_MODULE_INFO pmodule_info, unsigned long addr);
#endif

