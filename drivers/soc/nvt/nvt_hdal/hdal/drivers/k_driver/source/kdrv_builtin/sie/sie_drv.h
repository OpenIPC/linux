#ifndef __SIE_DRV_H__
#define __SIE_DRV_H__

#if defined (__LINUX)
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/completion.h>
#endif
#include <kwrap/spinlock.h>
#include <kwrap/semaphore.h>

#define MODULE_IRQ_NUM          3
#define MODULE_REG_NUM          3
#define MODULE_CLK_NUM          3

#define WRITE_REG(Value, Addr)  iowrite32(Value, Addr)
#define READ_REG(Addr)          ioread32(Addr)

typedef struct sie_module_info {
	struct completion sie_completion;
	struct vk_semaphore sie_sem;
	struct clk* pclk[MODULE_CLK_NUM];
	struct tasklet_struct sie_tasklet;
	void __iomem* io_addr[MODULE_REG_NUM];
    int iinterrupt_id[MODULE_IRQ_NUM];
    wait_queue_head_t sie_wait_queue;
	vk_spinlock_t sie_spinlock;
}SIE_MODULE_INFO, *PSIE_MODULE_INFO;

int nvt_sie_drv_open(PSIE_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_sie_drv_release(PSIE_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_sie_drv_init(PSIE_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_sie_drv_remove(PSIE_MODULE_INFO pmodule_info);
int nvt_sie_drv_suspend(SIE_MODULE_INFO* pmodule_info);
int nvt_sie_drv_resume(SIE_MODULE_INFO* pmodule_info);
int nvt_sie_drv_write_reg(PSIE_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_sie_drv_read_reg(PSIE_MODULE_INFO pmodule_info, unsigned long addr);
#endif

