#ifndef __MODULE_DRV_H__
#define __MODULE_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          1

#define WRITE_REG(Value, Addr)  iowrite32(Value, Addr)
#define READ_REG(Addr)          ioread32(Addr)

typedef struct sdp_module_info {
	struct semaphore sdp_sem;
	struct clk* pclk[MODULE_CLK_NUM];
	void __iomem* io_addr[MODULE_REG_NUM];
    int iinterrupt_id[MODULE_IRQ_NUM];
    wait_queue_head_t sdp_wait_queue;
	spinlock_t sdp_spinlock;
}SDP_MODULE_INFO, *PSDP_MODULE_INFO;

int nvt_sdp_drv_open(PSDP_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_sdp_drv_release(PSDP_MODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_sdp_drv_init(PSDP_MODULE_INFO pmodule_info);
int nvt_sdp_drv_remove(PSDP_MODULE_INFO pmodule_info);
int nvt_sdp_drv_suspend(SDP_MODULE_INFO* pmodule_info);
int nvt_sdp_drv_resume(SDP_MODULE_INFO* pmodule_info);
/*
int nvt_sif_drv_ioctl(unsigned char ucIF, MODULE_INFO* pmodule_info, unsigned int uiCmd, unsigned long ulArg);
int nvt_xxx_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_xxx_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr);*/

int nvt_sdp_drv_testopen(void);
#endif

