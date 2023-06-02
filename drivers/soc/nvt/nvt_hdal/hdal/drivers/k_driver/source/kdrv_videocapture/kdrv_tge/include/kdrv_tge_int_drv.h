#ifndef _KDRV_TGE_INT_DRV_H_
#define _KDRV_TGE_INT_DRV_H_
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          2

#define WRITE_REG(Value, Addr)  iowrite32(Value, Addr)
#define READ_REG(Addr)          ioread32(Addr)

typedef struct tge_module_info {
	struct completion tge_completion;
	struct semaphore tge_sem;
	struct clk* pclk[MODULE_CLK_NUM];
	struct tasklet_struct tge_tasklet;
	void __iomem* io_addr[MODULE_REG_NUM];
    int iinterrupt_id[MODULE_IRQ_NUM];
    wait_queue_head_t tge_wait_queue;
	spinlock_t tge_spinlock;
} TGE_MODULE_INFO, *PTGE_MODULE_INFO;

int nvt_tge_drv_open(PTGE_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_tge_drv_release(PTGE_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_tge_drv_init(PTGE_MODULE_INFO pmodule_info);
int nvt_tge_drv_remove(PTGE_MODULE_INFO pmodule_info);
int nvt_tge_drv_suspend(TGE_MODULE_INFO* pmodule_info);
int nvt_tge_drv_resume(TGE_MODULE_INFO* pmodule_info);
int nvt_tge_drv_ioctl(unsigned char ucIF, TGE_MODULE_INFO* pmodule_info, unsigned int uiCmd, unsigned long ulArg);
int nvt_tge_drv_write_reg(PTGE_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_tge_drv_read_reg(PTGE_MODULE_INFO pmodule_info, unsigned long addr);
#endif //_KDRV_TGE_INT_DRV_H_

