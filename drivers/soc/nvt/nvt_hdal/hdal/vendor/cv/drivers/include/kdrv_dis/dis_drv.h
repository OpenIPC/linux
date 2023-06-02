#ifndef __DIS_DRV_H__
#define __DIS_DRV_H__

#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#if defined(__LINUX)
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>
#endif

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          1


#if defined(__FREERTOS)
#define WRITE_REG(VALUE, ADDR)  OUTW((ADDR), (VALUE))
#define READ_REG(ADDR)          INW((ADDR))
#else
#define WRITE_REG(VALUE, ADDR)  iowrite32((VALUE), (ADDR))
#define READ_REG(ADDR)          ioread32((ADDR))
#endif


extern struct clk* dis_clk[MODULE_CLK_NUM];
typedef struct dis_module_info {
#if defined(__FREERTOS)
#else
    struct completion dis_completion;
    struct vk_semaphore dis_sem;
#endif
    struct clk *p_clk[MODULE_CLK_NUM];
#if defined(__FREERTOS)
#else
    struct tasklet_struct dis_tasklet;
    void __iomem *io_addr[MODULE_REG_NUM];
#endif
    int iinterrupt_id[MODULE_IRQ_NUM];
#if defined(__FREERTOS)
#else
    wait_queue_head_t dis_wait_queue;
    vk_spinlock_t dis_spinlock;
#endif
} DIS_MODULE_INFO, *PDIS_MODULE_INFO;

int nvt_dis_drv_open(PDIS_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_dis_drv_release(PDIS_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_dis_drv_init(PDIS_MODULE_INFO pmodule_info);
int nvt_dis_drv_remove(PDIS_MODULE_INFO pmodule_info);
int nvt_dis_drv_suspend(DIS_MODULE_INFO *p_module_info);
int nvt_dis_drv_resume(DIS_MODULE_INFO *p_module_info);
int nvt_dis_drv_ioctl(unsigned char ucIF, DIS_MODULE_INFO *p_module_info, unsigned int uiCmd, unsigned long ulArg);
int nvt_dis_drv_write_reg(PDIS_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_dis_drv_read_reg(PDIS_MODULE_INFO pmodule_info, unsigned long addr);
#endif

