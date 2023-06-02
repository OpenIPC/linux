////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (MStar Confidential Information) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __DRV_ISP_C__
#define __DRV_ISP_C__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/stringify.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/dcache.h>
#include <linux/namei.h>
#include <linux/sysfs.h>

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/io.h>

#include <cedric/irqs.h>
#include <mdrv_msys_io_st.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include "ms_msys.h"
#include "hal_isp.h"

/*--------------------------------------------------------------------------*/

//////////////////////////////////////////////////////////

/*--------------------------------------------------------------------------*/
/* CONSTANT DEFINITION                                                      */
/*--------------------------------------------------------------------------*/
#define ISP_DBG 0
#if ISP_DBG
#define DBG_INFO(fmt, _args...)  \
    do {   \
      printk(KERN_WARNING "[ISP] ""%s(%d): " fmt "\n", __FUNCTION__, __LINE__, ##_args);  \
    } while (0)
#else
#define DBG_INFO(fmt, arg...) {}
#endif


/*--------------------------------------------------------------------------*/
/* MACRO DEFINITION                                                         */
/*--------------------------------------------------------------------------*/
#define ISP_BIN_ATTR(_a,_b) \
    struct bin_attribute _a##_attr = { \
        .attr = { .name =  __stringify(_a), \
            .mode = _b, }, \
            .size = 0,  \
            .read = _a##_read,  \
            .write = _a##_write, \
}

#define C_IN(lock)       local_irq_save(flags)
#define C_OUT(lock)      local_irq_restore(flags)

/*--------------------------------------------------------------------------*/
/* DATA TYPE DEFINITION                                                     */
/*--------------------------------------------------------------------------*/

typedef struct {
    unsigned int u4PhyAddr;
    unsigned int u4VirAddr;
    unsigned long u4Stat[9*2];
} EIS_STS_t;

typedef struct EIS_VDOS_t {
    int width;
    int height;
    int pitch;
    int size;
    //int phyAddr;
    unsigned char data[5*1024*1024];
} eEIS_VDOS_t;

typedef struct {
    u32             frame_cnt;
    u8              sysfs_int;
    u8              vsync_start;
    u8              vsync_end;
    u8              ae;
    u8              awb;
    u8              af;
    u8              isp_busy;
    u8              isp_idle;
    u8              isp_fifofull;
    u8              wdma_fifofull;
    u8              rdma_done;
    u8              wdma_done;
    u8              eis;

    void              *pISPRegs[8];
    unsigned long   AE_Lum[64];   // 8x8 windows
    unsigned long   AE_Hist[64];  // 8x8 windows
    unsigned long   AF_Stat[60];  // 5 windows
    unsigned long   AWB_Stat[80]; // 5 windows

    EIS_STS_t       EIS_Stat;

    // FIXJASON
    eEIS_VDOS_t  EIS_VDOS;

} ISP_INT_DATA;

static volatile ISP_INT_DATA    isp_int;
//static DEFINE_SPINLOCK(isp_lock);

/*--------------------------------------------------------------------------*/
/* VARIABLE DEFINITION                                                      */
/*--------------------------------------------------------------------------*/

irqreturn_t isp_ISR(int num, void *priv);
static int isp_probe(struct platform_device* pdev);
static int isp_suspend(struct platform_device *pdev, pm_message_t state);
static int isp_resume(struct platform_device *pdev);
int isp_create_bin_file(struct device *dev);

static struct of_device_id isp_dt_ids[] = {
    {.compatible = "isp"},
    {}
};
MODULE_DEVICE_TABLE(of, isp_dt_ids);

#if 1
static struct platform_device *isp_device;
#else
void *ISP_REGS[] = {0x1F2A0C00,0x1F2A0E00,0x1F2A1000,0x1F2A1200,0x1F2A1400,0x1F2A1600,0x1F2A1800,0x1F2A1A00};
static struct platform_device isp_device =
{
        .name           = "isp",
        .id             = -1,
        .num_resources  = ARRAY_SIZE(ISP_REGS),
        .resource       = ISP_REGS,
};
#endif

static struct platform_driver isp_driver =
{
        .probe   = isp_probe,
        .suspend = isp_suspend,
        .resume  = isp_resume,
        .driver  = {
                .name = "isp",
                .owner = THIS_MODULE,
                .of_match_table = of_match_ptr(isp_dt_ids),
        },
};



static DECLARE_WAIT_QUEUE_HEAD(isp_wq_WDMA_FIFO_FULL);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_ISP_FIFO_FULL);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_RDMA_DONE);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_WDMA_DONE);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_ISP_IDLE);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_ISP_BUSY);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_VSTART);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_VEND);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_ae);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_awb);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_af);
static DECLARE_WAIT_QUEUE_HEAD(isp_wq_eis);  // FIXJASON, still not used

static struct kobject *kobj_isp = NULL; //, *kobj_vs_sr, *kobj_ve_sr,
//*kobj_ae, *kobj_awb, *kobj_af, *kobj_busy, *kobj_idle;


/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTION PROTOTYPE DECLARATION AND DEFINITION                      */
/*--------------------------------------------------------------------------*/

extern int msys_request_dmem(MSYS_DMEM_INFO *mem_info);
static int isp_probe(struct platform_device* pdev) {
  int irq, i;
  unsigned int u4Banks[8];
  printk(KERN_NOTICE "[ISP] = isp_probe\n");

#if 1
  of_property_read_u32_array(pdev->dev.of_node, "banks", (unsigned int*)u4Banks, 8);
  for (i = 0; i < 8; i++) {
    printk(KERN_INFO "[ISP:%d] IO remap addr:0x%08x\n", i, (unsigned int)ioremap(u4Banks[i], 0x200));
    isp_int.pISPRegs[i] = (void*)ioremap(u4Banks[i], 0x200);
  }
#else
  struct resource *r;
  for (i = 0; i < 8; i++) {
    r = platform_get_resource(pdev, IORESOURCE_MEM, i);
    printk(KERN_INFO "[ISP:%d of %d] IO remap addr:0x%08x\n", i, pdev->num_resources, (unsigned int)ioremap(r->start, resource_size(r)));
    isp_int.pISPRegs[i] = (void *)ioremap(r->start, resource_size(r));
  }
#endif
  irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
  printk(KERN_INFO "[ISP] Request IRQ: %d\n", irq);
  if (request_irq(irq, isp_ISR, IRQ_TYPE_EDGE_RISING, "isp interrupt", (void*)&isp_int) == 0)
  {
    printk(KERN_INFO "[ISP] isp interrupt registered\n");
  }
  else
  {
    printk(KERN_INFO "[ISP] isp interrupt failed");
  }

  SETREG8((0x1F000000+(0x112B00*2)+(0x16*4)),0x01);      //set GPIO28 to GPIO function
  CLRREG16((0x1F000000+(0x101A00*2)+(0x12*4)),(0x0001<<12)); //set GPIO28 pad mux to GPIO
  CLRREG16((0x1F000000+(0x102B00*2)+(0x12*4)),(0x0001<<9));  //GPIO28 OEN
  SETREG16((0x1F000000+(0x102B00*2)+(0x12*4)),(0x0001<<8));  //GPIO28 pull ligh
  isp_device = pdev;

  return 0;
}

static int isp_suspend(struct platform_device *pdev, pm_message_t state) {
    printk(KERN_NOTICE "[ISP] = isp_suspend\n");
    if (!pdev)
        DBG_INFO("isp_suspend with NULL pdev %d", isp_int.frame_cnt);

    isp_int.sysfs_int = false;
    HalISPClkEnable(false);
    return 0;
}

static int isp_resume(struct platform_device *pdev) {
    HalISPDisableInt();
    HalISPClkEnable(true);
    isp_int.sysfs_int = false;
    printk(KERN_NOTICE "[ISP] = isp_resume\n");
    return 0;
}

irqreturn_t isp_ISR(int num, void *priv) {
    volatile u32 clear = 0;
    volatile u32 clear2 = 0;
    volatile u32 status;
    volatile u32 status2;
    volatile u32 vsync_pos;
    volatile u32 mipi_if;

    HalISPMaskInt();
    status = HalISPGetIntStatus();
    status2 = HalISPGetIntStatus2();
    vsync_pos = HalISPGetVsyncPol();
    mipi_if = HalISPGetMIPI();

    if (status == 0 && status2 == 0) {
        printk(KERN_ERR "[ISP] False interrupt? mask1 0x%04x, mask2 0x%04x\n", status, status2);
        HalISPMaskClear1(0xffff);
        HalISPMaskClear2(0xffff);
        //while(1);
        return IRQ_HANDLED;
    }
    //    else
    //        printk(KERN_ERR "[ISP] True interrupt, mask1 0x%04x, mask2 0x%04x\n", status, status2);

    if (isp_int.sysfs_int == false) {
        DBG_INFO("isp_int.sysfs_int == false");
        HalISPDisableInt();
        return IRQ_HANDLED;
    }

    //    DBG_INFO("raw status 0x%04x, 0x%04x, status2 0x%04x, mipi 0x%04x, vsync_pol 0x%04x, frame %d, sysfs %d\n",
    //            HalISPGetRawInt(), HalISPGetIntStatus(), status2,
    //            mipi_if, vsync_pos, isp_int.frame_cnt,
    //            isp_int.sysfs_int);

    //return IRQ_HANDLED;

    ////////////////////////////////////////////
    //             Vsync Start                //
    ////////////////////////////////////////////
    if (mipi_if) {
        if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_ISPIF_VSYNC)) {
            HalISPClearInt(&clear, INT_ISPIF_VSYNC);
            DBG_INFO("MIPI VSYNC ACTIVE %d", isp_int.frame_cnt);
            isp_int.vsync_start = true;
            isp_int.frame_cnt++;
            wake_up_interruptible_all(&isp_wq_VSTART);
        }
    }
    else {
        // parallel
#if 0
        //        if ((ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_RISING) && vsync_pos == true) ||
        //                (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_FALLING) && vsync_pos == false)){
        //
        //            if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_RISING) && vsync_pos == true) {
        //                DBG_INFO("Parallel INT_PAD_VSYNC_RISING %d", isp_int.frame_cnt);
        //                HalISPClearInt(&clear, INT_PAD_VSYNC_FALLING);
        //            } else if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_FALLING) && vsync_pos == false) {
        //                DBG_INFO("Parallel INT_PAD_VSYNC_FALLING %d", isp_int.frame_cnt);
        //                HalISPClearInt(&clear, INT_PAD_VSYNC_RISING);
        //            }
#else
        if ((ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_RISING) && vsync_pos == true) ||
                (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_FALLING) && vsync_pos == false)){
            //            if ((ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_RISING) && vsync_pos == true) ||
            //                    (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_FALLING) && vsync_pos == false)){

            //            if (ISP_CHECKBITS(HalISPGetRawInt(), INT_PAD_VSYNC_RISING)) {
            //                HalISPClearInt(&clear, INT_PAD_VSYNC_FALLING);
            //            } else if (ISP_CHECKBITS(HalISPGetRawInt(), INT_PAD_VSYNC_FALLING)) {
            //                DBG_INFO("Parallel INT_PAD_VSYNC_FALLING %d", isp_int.frame_cnt);
            //                HalISPClearInt(&clear, INT_PAD_VSYNC_RISING);
            //            }
#endif
            //            printk(KERN_ERR "raw status 0x%04x, 0x%04x, status2 0x%04x, vsync start %d\n",
            //                    HalISPGetRawInt(), HalISPGetIntStatus(), status2, isp_int.vsync_start);

            isp_int.frame_cnt++;
            if (isp_int.vsync_start == false) {
                if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_RISING) && vsync_pos == true) {
                    if (isp_int.frame_cnt%600==0)
                        DBG_INFO("Parallel INT_PAD_VSYNC_RISING %d", isp_int.frame_cnt);
                    HalISPClearInt(&clear, INT_PAD_VSYNC_RISING);
                } else if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_FALLING) && vsync_pos == false) {
                    if (isp_int.frame_cnt%600==0)
                        DBG_INFO("Parallel INT_PAD_VSYNC_FALLING %d", isp_int.frame_cnt);
                    HalISPClearInt(&clear, INT_PAD_VSYNC_FALLING);
                }
                isp_int.vsync_start = true;
                wake_up_interruptible_all(&isp_wq_VSTART);
            } else {
                //DBG_INFO("Parallel Vsync Start ---FALSE---");
            }
        }
    }


    //////////////////////////////////////////////
    //           statistics                     //
    //////////////////////////////////////////////
    if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_AE_DONE)) {
        DBG_INFO("INT_AE_DONE %d", isp_int.frame_cnt);
        HalISPGetAEStat(isp_int.AE_Lum);
        HalISPGetAEHist(isp_int.AE_Hist);
        HalISPClearInt(&clear, INT_AE_DONE);
        isp_int.ae = true;
        wake_up_interruptible_all(&isp_wq_ae);
    }

    if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_AWB_DONE)) {
        DBG_INFO("INT_AWB_DONE %d", isp_int.frame_cnt);
        HalISPGetAWBStat(isp_int.AWB_Stat);
        HalISPClearInt(&clear, INT_AWB_DONE);
        isp_int.awb = true;
        wake_up_interruptible_all(&isp_wq_awb);
    }

    if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_AF_DONE)) {
        DBG_INFO("INT_AF_DONE %d", isp_int.frame_cnt);
        HalISPGetAFStat(isp_int.AF_Stat);
        HalISPClearInt(&clear, INT_AF_DONE);
        isp_int.af = true;
        wake_up_interruptible_all(&isp_wq_af);
    }

    ////////////////////////////////////////////
    //             ISP statis                 //
    ////////////////////////////////////////////
    if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_ISP_BUSY)) {
        HalISPClearInt(&clear, INT_ISP_BUSY);
        DBG_INFO("INT_ISP_BUSY %d", isp_int.frame_cnt);
        isp_int.isp_busy = true;
        wake_up_interruptible_all(&isp_wq_ISP_BUSY);
    }
    if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_ISP_IDLE)) {
        HalISPClearInt(&clear, INT_ISP_IDLE);
        DBG_INFO("INT_ISP_IDLE %d", isp_int.frame_cnt);
        isp_int.isp_idle = true;
        wake_up_interruptible_all(&isp_wq_ISP_IDLE);
    }

    ////////////////////////////////////////////
    //             Vsync end                  //
    ////////////////////////////////////////////

#if 0
    //        if (!mipi_if ) {
    //            if ((ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_RISING) && vsync_pos == false) ||
    //                    (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_FALLING) && vsync_pos == true)) {
    //                if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_RISING) && vsync_pos == false)
    //                    HalISPClearInt(&clear, INT_PAD_VSYNC_FALLING);
    //                if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_FALLING) && vsync_pos == true)
    //                    HalISPClearInt(&clear, INT_PAD_VSYNC_RISING);
#else
    if (!mipi_if ) {
        if ((ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_RISING) && vsync_pos == false) ||
                (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_FALLING) && vsync_pos == true)) {
            //            if ((ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_RISING) && vsync_pos == false) ||
            //                    (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_FALLING) && vsync_pos == true)) {

#endif
            //            printk(KERN_ERR "raw status 0x%04x, 0x%04x, status2 0x%04x, vsync end %d\n",
            //                    HalISPGetRawInt(), HalISPGetIntStatus(), status2, isp_int.vsync_end);

            if (isp_int.vsync_end == false) {
                if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_RISING) && vsync_pos == false) {
                    HalISPClearInt(&clear, INT_PAD_VSYNC_RISING);
                    if (isp_int.frame_cnt%600==0)
                        DBG_INFO("Parallel Vsync End INT_PAD_VSYNC_RISING %d", isp_int.frame_cnt);
                }
                else if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_PAD_VSYNC_FALLING) && vsync_pos == true) {
                    HalISPClearInt(&clear, INT_PAD_VSYNC_FALLING);
                    if (isp_int.frame_cnt%600==0)
                        DBG_INFO("Parallel Vsync End INT_PAD_VSYNC_FALLING %d", isp_int.frame_cnt);
                }
                isp_int.vsync_end = true;
                wake_up_interruptible_all(&isp_wq_VEND);
            } else {
                //DBG_INFO("Parallel Vsync End ---FALSE---");
            }
        }
    }


    ////////////////////////////////////////////
    //             DMA                        //
    ////////////////////////////////////////////
    if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_WDMA_DONE) && isp_int.wdma_done == false) {
        HalISPClearInt(&clear, INT_WDMA_DONE);
        HalISPClearInt(&clear, INT_PAD_VSYNC_RISING);
        HalISPClearInt(&clear, INT_PAD_VSYNC_FALLING);
        DBG_INFO("INT_WDMA_DONE %d", isp_int.frame_cnt);
        isp_int.wdma_done = true;
        wake_up_interruptible_all(&isp_wq_WDMA_DONE);
    } else {
        //DBG_INFO("INT_WDMA_DONE %d, --FALSE--", isp_int.frame_cnt);
    }

    if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_RDMA_DONE) && isp_int.rdma_done == false) {
        HalISPClearInt(&clear, INT_RDMA_DONE);
        DBG_INFO("INT_RDMA_DONE %d", isp_int.frame_cnt);
        isp_int.rdma_done = true;
        wake_up_interruptible_all(&isp_wq_RDMA_DONE);
    } else {
        //DBG_INFO("INT_RDMA_DONE %d, --FALSE--", isp_int.frame_cnt);
    }

    ////////////////////////////////////////////
    //             FIFO Full                  //
    ////////////////////////////////////////////
    if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_ISP_FIFO_FULL)
            //&& isp_int.isp_fifofull == false
            )
    {
        HalISPClearInt(&clear, INT_ISP_FIFO_FULL);
        DBG_INFO("INT_ISP_FIFO_FULL %d", isp_int.frame_cnt);
        isp_int.isp_fifofull = true;
        wake_up_interruptible_all(&isp_wq_ISP_FIFO_FULL);
    } else {
        //DBG_INFO("INT_ISP_FIFO_FULL %d, --FALSE--", isp_int.frame_cnt);
    }

    if (ISP_CHECKBITS(HalISPGetIntStatus(), INT_WDMA_FIFO_FULL) &&
            isp_int.wdma_fifofull == false) {
        HalISPClearInt(&clear, INT_WDMA_FIFO_FULL);
        DBG_INFO("INT_WDMA_FIFO_FULL %d", isp_int.frame_cnt);
        isp_int.wdma_fifofull = true;
        wake_up_interruptible_all(&isp_wq_WDMA_FIFO_FULL);
    } else {
        //DBG_INFO("INT_WDMA_FIFO_FULL %d, --FALSE--", isp_int.frame_cnt);
    }

    if (ISP_CHECKBITS(HalISPGetIntStatus2(), INT2_VDOS_EVERYLINE)) {
        HalISPClearInt2(&clear2, INT2_VDOS_EVERYLINE);
        DBG_INFO("INT2_VDOS_EVERYLINE %d", isp_int.frame_cnt);
    }
    if (ISP_CHECKBITS(HalISPGetIntStatus2(), INT2_VDOS_LINE0)) {
        HalISPClearInt2(&clear2, INT2_VDOS_LINE0);
        DBG_INFO("INT2_VDOS_LINE0 %d", isp_int.frame_cnt);
    }
    if (ISP_CHECKBITS(HalISPGetIntStatus2(), INT2_VDOS_LINE1)) {
        HalISPClearInt2(&clear2, INT2_VDOS_LINE1);
        DBG_INFO("INT2_VDOS_LINE1 %d", isp_int.frame_cnt);
    }
    if (ISP_CHECKBITS(HalISPGetIntStatus2(), INT2_VDOS_LINE2)) {
        HalISPClearInt2(&clear2, INT2_VDOS_LINE2);
        DBG_INFO("INT2_VDOS_LINE2 %d", isp_int.frame_cnt);

        //HalISPGetVDOSWidth( &(isp_int.EIS_VDOS.width) );
        //HalISPGetVDOSHight( &(isp_int.EIS_VDOS.height) );
        HalISPGetVDOSPitch( &(isp_int.EIS_VDOS.pitch) );
        HalISPGetVDOSSize( &(isp_int.EIS_VDOS.size), isp_int.EIS_VDOS.pitch );
        HalISPGetVDOSData( (unsigned long*)isp_int.EIS_VDOS.data, isp_int.EIS_VDOS.size );
        //printk(KERN_INFO "isp_int.EIS_VDOS.width(%d) isp_int.EIS_VDOS.height(%d)\n", isp_int.EIS_VDOS.width, isp_int.EIS_VDOS.height);
        //printk(KERN_INFO "isp_int.EIS_VDOS.pitch(%d) isp_int.EIS_VDOS.size(%d)\n", isp_int.EIS_VDOS.pitch, isp_int.EIS_VDOS.size);

        isp_int.eis = true;
        wake_up_interruptible_all(&isp_wq_eis);
    }

    HalISPMaskClear1(clear);
    HalISPMaskClear2(clear2);
    if (isp_int.frame_cnt%600==0) {
        DBG_INFO("end of ISR, status:0x%04x, clear:0x%04x", HalISPGetIntStatus(), clear);
    }

    return IRQ_HANDLED;
}

static int isp_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    int data_size = 0;
    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        DBG_INFO("buff in %d, out %d\n", size, data_size);
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    }
    return data_size;
}

static int isp_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
        MSYS_DMEM_INFO EIS_MemInfo;
    if (buf && size >= 1) {
        memset((void*)&isp_int, 0, sizeof(ISP_INT_DATA));
        isp_int.sysfs_int = (buf[0] & 0x01);

        EIS_MemInfo.length = sizeof(unsigned long)*9*2;
        msys_request_dmem(&EIS_MemInfo);
        isp_int.EIS_Stat.u4PhyAddr = EIS_MemInfo.phys;
        isp_int.EIS_Stat.u4VirAddr = EIS_MemInfo.kvirt;

        printk(KERN_NOTICE "[isp_write] P:%x, V:%x\n",
                isp_int.EIS_Stat.u4PhyAddr,
                isp_int.EIS_Stat.u4VirAddr);
    } else {
        memset((void*)&isp_int, 0, sizeof(ISP_INT_DATA));
        isp_int.sysfs_int = false;
    }

    if (isp_int.sysfs_int == true)
        HalISPEnableInt();

    DBG_INFO("[%s] isp_int.sysfs_int %d\n", __FUNCTION__,
            isp_int.sysfs_int);

    return size;
}

static int vs_sr_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_VSTART, isp_int.vsync_start == true))
    {
        C_IN(isp_lock);
        isp_int.vsync_start = false;
        C_OUT(isp_lock);
        return -ERESTARTSYS;
    }
    C_IN(isp_lock);
    isp_int.vsync_start = false;
    C_OUT(isp_lock);

    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        //DBG_INFO("buff in %d, out %d", size, data_size);
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;

    // FIXJASON
    // jason print something...

    return data_size;
}

static int vs_sr_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

static int ve_sr_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    int data_size = 4;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_VEND, isp_int.vsync_end == true))
    {
        C_IN(isp_lock);
        isp_int.vsync_end = false;
        C_OUT(isp_lock);
        return -ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.vsync_end = false;
    C_OUT(isp_lock);

    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        //DBG_INFO("buff in %d, out %d", size, data_size);
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;


    return data_size;
}
static int ve_sr_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}


static int ae_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    int lum_size, hist_size;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_ae, isp_int.ae == true))
    {
        C_IN(isp_lock);
        isp_int.ae = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.ae = false;
    C_OUT(isp_lock);

    HalISPGetAEStatSize(&lum_size);
    HalISPGetAEHistSize(&hist_size);
    if (buf && size >= lum_size+hist_size) {
        memcpy((void *)buf, (void *)isp_int.AE_Lum, lum_size);
        memcpy((void *)(buf+lum_size), (void *)isp_int.AE_Hist, hist_size);
        //DBG_INFO("buff in %d, out %d", size, lum_size+hist_size);
    } else
        lum_size = hist_size = 0;

    return lum_size+hist_size;
}
static int ae_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

static int awb_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    u32 awb_size;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_awb, isp_int.awb == true))
    {
        C_IN(isp_lock);
        isp_int.awb = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.awb = false;
    C_OUT(isp_lock);



    HalISPGetAWBStatSize(&awb_size);
    if (buf && size >= awb_size) {
        //DBG_INFO("buff in %d, out %d", size, awb_size);
        memcpy((void *)buf, (void *)isp_int.AWB_Stat, awb_size);
    } else
        awb_size = 0;

    return awb_size;
}
static int awb_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

static int af_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    u32 af_size;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_af, isp_int.af == true))
    {
        C_IN(isp_lock);
        isp_int.af = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.af = false;
    C_OUT(isp_lock);

    HalISPGetAFStatSize(&af_size);
    if (buf && size >= af_size) {
        //DBG_INFO("buff in %d, out %d", size, af_size);
        memcpy((void *)buf, (void *)isp_int.AF_Stat, af_size);
    } else
        af_size = 0;

    return af_size;
}
static int af_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

static int busy_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_ISP_BUSY, isp_int.isp_busy == true))
    {
        C_IN(isp_lock);
        isp_int.isp_busy = false;
        C_OUT(isp_lock);

        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.isp_busy = false;
    C_OUT(isp_lock);



    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        //DBG_INFO("buff in %d, out %d", size, data_size);
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;


    return data_size;
}
static int busy_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

static int idle_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {

    int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_ISP_IDLE, isp_int.isp_idle == true))
    {
        C_IN(isp_lock);
        isp_int.isp_idle = false;
        C_OUT(isp_lock);

        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.isp_idle = false;
    C_OUT(isp_lock);



    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        //DBG_INFO("buff in %d, out %d", size, data_size);
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;


    return data_size;
}
static int idle_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

/// isp fifo full
static int isp_fifofull_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {

    int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_ISP_FIFO_FULL, isp_int.isp_fifofull == true))
    {

        C_IN(isp_lock);
        isp_int.isp_fifofull = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.isp_fifofull = false;
    C_OUT(isp_lock);



    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        //DBG_INFO("buff in %d, out %d", size, data_size);
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;

    return data_size;
}
static int isp_fifofull_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

// wdma fifo full
static int wdma_fifofull_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {

    int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_WDMA_FIFO_FULL, isp_int.wdma_fifofull == true))
    {
        C_IN(isp_lock);
        isp_int.wdma_fifofull = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.wdma_fifofull = false;
    C_OUT(isp_lock);



    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        //DBG_INFO("buff in %d, out %d", size, data_size);
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;

    return data_size;
}
static int wdma_fifofull_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}


// wdma done
static int wdma_done_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_WDMA_DONE, isp_int.wdma_done == true))
    {
        C_IN(isp_lock);
        isp_int.wdma_done = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.wdma_done = false;
    C_OUT(isp_lock);



    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        //DBG_INFO("buff in %d, out %d", size, data_size);
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;


    return data_size;
}
static int wdma_done_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

// rdma done
static int rdma_done_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {

    int data_size = 0;
    unsigned long flags;
    if (wait_event_interruptible(isp_wq_RDMA_DONE, isp_int.rdma_done == true))
    {
        C_IN(isp_lock);
        isp_int.rdma_done = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }
    C_IN(isp_lock);
    isp_int.rdma_done = false;
    C_OUT(isp_lock);


    data_size = sizeof(isp_int.frame_cnt);
    if (buf && size >= data_size) {
        //DBG_INFO("buff in %d, out %d", size, data_size);
        memcpy((void *)buf, (void *)&isp_int.frame_cnt, data_size);
    } else
        data_size = 0;


    return data_size;
}
static int rdma_done_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {
    DBG_INFO();
    return size;
}

// eis vdos
static int eis_read(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {  // FIXJASON
    unsigned long flags;
    isp_int.EIS_VDOS.size = 0;
    if (wait_event_interruptible(isp_wq_eis, isp_int.eis == true))
    {
        C_IN(isp_lock);
        isp_int.eis = false;
        C_OUT(isp_lock);
        return - ERESTARTSYS;
    }

    C_IN(isp_lock);
    isp_int.eis = false;
    C_OUT(isp_lock);

    HalISPGetVDOSPitch( &(isp_int.EIS_VDOS.pitch) );
    HalISPGetVDOSSize( &(isp_int.EIS_VDOS.size), isp_int.EIS_VDOS.pitch );
    if (buf && size >= isp_int.EIS_VDOS.size) {
        //DBG_INFO("buff in %d, out %d", size, isp_int.EIS_VDOS.size);
        memcpy((void *)buf, (void *)isp_int.EIS_VDOS.data, isp_int.EIS_VDOS.size);
    } else
        isp_int.EIS_VDOS.size = 0;

    return isp_int.EIS_VDOS.size;
}
static int eis_write(struct file *fd, struct kobject *kobj, struct bin_attribute *attr, char *buf, loff_t offset, size_t size) {  // FIXJASON
    DBG_INFO();
    return size;
}



static ISP_BIN_ATTR(isp, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(vs_sr, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(ve_sr, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(ae, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(awb, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(af, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(busy, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(idle, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(isp_fifofull, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(wdma_fifofull, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(wdma_done, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(rdma_done, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
static ISP_BIN_ATTR(eis, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);  // FIXJASON

int isp_create_bin_file(struct device *dev)
{
    int error = -EINVAL;
    if (dev != NULL) {
      kobj_isp = &dev->kobj;
      printk(KERN_INFO "[%s] start", __FUNCTION__);

      error = sysfs_create_link(&dev->parent->kobj, kobj_isp, "isp0");
      error = sysfs_create_bin_file(kobj_isp, &isp_attr);
      error = sysfs_create_bin_file(kobj_isp, &vs_sr_attr);
      error = sysfs_create_bin_file(kobj_isp, &ve_sr_attr);
      error = sysfs_create_bin_file(kobj_isp, &ae_attr);
      error = sysfs_create_bin_file(kobj_isp, &awb_attr);
      error = sysfs_create_bin_file(kobj_isp, &af_attr);
      error = sysfs_create_bin_file(kobj_isp, &busy_attr);
      error = sysfs_create_bin_file(kobj_isp, &idle_attr);
      error = sysfs_create_bin_file(kobj_isp, &isp_fifofull_attr);
      error = sysfs_create_bin_file(kobj_isp, &wdma_fifofull_attr);
      error = sysfs_create_bin_file(kobj_isp, &wdma_done_attr);
      error = sysfs_create_bin_file(kobj_isp, &rdma_done_attr);
      error = sysfs_create_bin_file(kobj_isp, &eis_attr);  // FIXJASON
    }
    printk(KERN_INFO "[%s] end", __FUNCTION__);
    return error;
}

void isp_remove_bin_file(void)
{
    sysfs_remove_bin_file(kobj_isp, &isp_attr);
    sysfs_remove_bin_file(kobj_isp, &vs_sr_attr);
    sysfs_remove_bin_file(kobj_isp, &ve_sr_attr);
    sysfs_remove_bin_file(kobj_isp, &ae_attr);
    sysfs_remove_bin_file(kobj_isp, &awb_attr);
    sysfs_remove_bin_file(kobj_isp, &af_attr);
    sysfs_remove_bin_file(kobj_isp, &busy_attr);
    sysfs_remove_bin_file(kobj_isp, &idle_attr);
    sysfs_remove_bin_file(kobj_isp, &isp_fifofull_attr);
    sysfs_remove_bin_file(kobj_isp, &wdma_fifofull_attr);
    sysfs_remove_bin_file(kobj_isp, &wdma_done_attr);
    sysfs_remove_bin_file(kobj_isp, &rdma_done_attr);
    sysfs_remove_bin_file(kobj_isp, &eis_attr);  // FIXJASON

    //    DBG_INFO("[%s] end", __FUNCTION__);
}


static int __init mstar_isp_init(void)
{
    int ret;
    memset((void*)&isp_int, 0, sizeof(isp_int));
    isp_int.sysfs_int = false;

    printk(KERN_INFO "[ISP] %s\n", __FUNCTION__);
    ret = platform_driver_register(&isp_driver);
    if(!ret)
    {
      printk(KERN_INFO "[ISP] register driver success");
      //ret = platform_device_register(&isp_device);
      //ret = platform_device_register(isp_device);
      if(!ret)
      {
        printk(KERN_INFO "[ISP] register device success");
//        isp_create_bin_file(&isp_device.dev);
        isp_create_bin_file(&isp_device->dev);
//        printk(KERN_INFO "[ISP] Request IRQ: %d\n", INT_IRQ_ISP);
//        if (request_irq(INT_IRQ_ISP, isp_ISR, IRQ_TYPE_EDGE_RISING, "isp interrupt", (void*)&isp_int) == 0)
//        {
//          printk(KERN_INFO "[ISP] isp interrupt registered\n");
//        }
//        else
//        {
//          printk(KERN_INFO "[ISP] isp interrupt failed");
//        }
      } else {
        printk(KERN_INFO "[ISP] register device fail");
        platform_driver_unregister(&isp_driver);
      }
    }

    return 0;
}


static void __exit mstar_isp_exit(void)
{
    printk(KERN_INFO "[ISP] exit");
    platform_driver_unregister(&isp_driver);
    isp_remove_bin_file();
}

module_init(mstar_isp_init);
module_exit(mstar_isp_exit);

#endif//__DRV_ISP_C__
