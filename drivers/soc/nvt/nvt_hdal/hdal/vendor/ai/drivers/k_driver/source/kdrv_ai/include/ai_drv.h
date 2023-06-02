#ifndef __AI_DRV_H__
#define __AI_DRV_H__

#if defined(__FREERTOS)
#include "kwrap/type.h"
#else
#include <linux/io.h>
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>
#endif

#define MODULE_IRQ_NUM          3
#define MODULE_REG_NUM          3
#define MODULE_CLK_NUM          3

#if defined(__FREERTOS)
#define WRITE_REG(VALUE, ADDR)  OUTW((ADDR), (VALUE))
#define READ_REG(ADDR)          INW((ADDR))
#else
#define WRITE_REG(VALUE, ADDR)  iowrite32((VALUE), (ADDR))
#define READ_REG(ADDR)          ioread32((ADDR))
#endif

extern struct clk* ai_clk[MODULE_CLK_NUM];
extern UINT32 ai_freq_from_dtsi[MODULE_CLK_NUM];
typedef struct ai_info {
#if defined(__FREERTOS)
#else
    struct completion ai_completion;
    struct vk_semaphore ai_sem;
#endif
    struct clk *pclk[MODULE_CLK_NUM];
#if defined(__FREERTOS)
#else
    struct tasklet_struct ai_tasklet;
    void __iomem *io_addr[MODULE_REG_NUM];
#endif
    int iinterrupt_id[MODULE_IRQ_NUM];
#if defined(__FREERTOS)
#else
    wait_queue_head_t ai_wait_queue;
    vk_spinlock_t ai_spinlock;
#endif
} AI_INFO, *PAI_INFO;


#ifdef __FREERTOS
struct nvt_fmem_mem_info_t {
    UINT32 vaddr;
    UINT32 size;
    UINT32 ddr_addr;
    UINT32 ddr_size;
};
#endif

int nvt_ai_drv_open(PAI_INFO pmodule_info, unsigned char if_id);
int nvt_ai_drv_release(PAI_INFO pmodule_info, unsigned char if_id);
int nvt_ai_drv_init(AI_INFO *pmodule_info);
int nvt_ai_drv_remove(PAI_INFO pmodule_info);
int nvt_ai_drv_suspend(AI_INFO *pmodule_info);
int nvt_ai_drv_resume(AI_INFO *pmodule_info);
int nvt_ai_drv_ioctl(unsigned char if_id, AI_INFO *pmodule_info, unsigned int cmd, unsigned long argc);
int nvt_ai_drv_write_reg(PAI_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_ai_drv_read_reg(PAI_INFO pmodule_info, unsigned long addr);
#endif

