#ifndef __KDRV_ISE_INT_DRV_H__
#define __KDRV_ISE_INT_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>
#include "kwrap/type.h"


#define NVT_KDRV_ISE_IRQ_NUM          1
#define NVT_KDRV_ISE_REG_NUM          1
#define NVT_KDRV_ISE_CLK_NUM          1

#define WRITE_REG(Value, Addr)  iowrite32(Value, Addr)
#define READ_REG(Addr)          ioread32(Addr)

typedef struct _nvt_kdrv_ise_info {
	struct clk *pclk[NVT_KDRV_ISE_CLK_NUM];
	void __iomem *io_addr[NVT_KDRV_ISE_REG_NUM];
	int iinterrupt_id[NVT_KDRV_ISE_IRQ_NUM];
} NVT_KDRV_ISE_INFO, *PNVT_KDRV_ISE_INFO;

int nvt_kdrv_ise_drv_open(PNVT_KDRV_ISE_INFO pnvt_kdrv_ise_info, unsigned char ucIF);
int nvt_kdrv_ise_drv_release(PNVT_KDRV_ISE_INFO pnvt_kdrv_ise_info, unsigned char ucIF);
int nvt_kdrv_ise_drv_init(PNVT_KDRV_ISE_INFO pnvt_kdrv_ise_info);
int nvt_kdrv_ise_drv_remove(PNVT_KDRV_ISE_INFO pnvt_kdrv_ise_info);
int nvt_kdrv_ise_drv_suspend(NVT_KDRV_ISE_INFO *pnvt_kdrv_ise_info);
int nvt_kdrv_ise_drv_resume(NVT_KDRV_ISE_INFO *pnvt_kdrv_ise_info);
//int nvt_kdrv_ise_drv_ioctl(unsigned char ucIF, NVT_KDRV_ISE_INFO *pnvt_kdrv_ise_info, unsigned int uiCmd, unsigned long ulArg);

#endif

