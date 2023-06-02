#ifndef __IVE_DRV_H__
#define __IVE_DRV_H__

#if defined(__FREERTOS)
#else
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>
#endif
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"


#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          1

#if defined(__FREERTOS)
#else
#define WRITE_REG(Value, Addr)  iowrite32(Value, Addr)
#define READ_REG(Addr)          ioread32(Addr)

typedef struct module_info {
	struct completion ive_completion;
	struct vk_semaphore ive_sem;
	struct clk* pclk[MODULE_CLK_NUM];
	struct tasklet_struct ive_tasklet;
	void __iomem* io_addr[MODULE_REG_NUM];
    int iinterrupt_id[MODULE_IRQ_NUM];
    wait_queue_head_t ive_wait_queue;
	vk_spinlock_t ive_spinlock;
}MODULE_INFO, *PMODULE_INFO;
#endif

#if defined(__FREERTOS)
#else
extern struct clk *p_ive_clk[MODULE_CLK_NUM];
extern UINT32 ive_freq_from_dtsi[MODULE_CLK_NUM];

int nvt_ive_drv_open(PMODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_ive_drv_release(PMODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_ive_drv_init(PMODULE_INFO pmodule_info);
int nvt_ive_drv_remove(PMODULE_INFO pmodule_info);
int nvt_ive_drv_suspend(MODULE_INFO* pmodule_info);
int nvt_ive_drv_resume(MODULE_INFO* pmodule_info);
int nvt_ive_drv_ioctl(unsigned char ucIF, MODULE_INFO* pmodule_info, unsigned int uiCmd, unsigned long ulArg);
int nvt_ive_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_ive_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr);
#endif
#endif //#ifndef __IVE_DRV_H__

