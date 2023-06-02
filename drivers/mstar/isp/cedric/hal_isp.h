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


#ifndef __HAL_ISP_H__
#define __HAL_ISP_H__
#include "ms_platform.h"
/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/
#define RIUBASE_ISP_OFFSET_0    (0xA8300<<2)
#define RIUBASE_ISP_OFFSET_1    (0xA8380<<2)
#define RIUBASE_ISP_OFFSET_2    (0xA8400<<2)
#define RIUBASE_ISP_OFFSET_3    (0xA8480<<2)
#define RIUBASE_ISP_OFFSET_4    (0xA8500<<2)
#define RIUBASE_ISP_OFFSET_5    (0xA8580<<2)
#define RIUBASE_ISP_OFFSET_6    (0xA8600<<2)
#define RIUBASE_ISP_OFFSET_7    (0xA8680<<2)

#define INFINITY_MIU0_BASE      0x20000000

#define IO_PHYS                 0x1F000000
#define IO_VIRT                 (IO_PHYS+IO_OFFSET)//from IO_ADDRESS(x)
#define IO_OFFSET               (MS_IO_OFFSET)
#define IO_SIZE                 0x00400000

#define RIUBASE_ISP_0  (IO_VIRT + RIUBASE_ISP_OFFSET_0)
#define RIUBASE_ISP_1  (IO_VIRT + RIUBASE_ISP_OFFSET_1)
#define RIUBASE_ISP_2  (IO_VIRT + RIUBASE_ISP_OFFSET_2)
#define RIUBASE_ISP_3  (IO_VIRT + RIUBASE_ISP_OFFSET_3)
#define RIUBASE_ISP_4  (IO_VIRT + RIUBASE_ISP_OFFSET_4)
#define RIUBASE_ISP_5  (IO_VIRT + RIUBASE_ISP_OFFSET_5)
#define RIUBASE_ISP_6  (IO_VIRT + RIUBASE_ISP_OFFSET_6)
#define RIUBASE_ISP_7  (IO_VIRT + RIUBASE_ISP_OFFSET_7)

typedef enum {
    INT_SR_VREF_RISING      = 0,
    INT_SR_VREF_FALLING     = 1,
    INT_STROBE_DONE         = 2,
    INT_DB_UPDATE_DONE      = 3,
    INT_AF_DONE             = 4,
    INT_ISP_FIFO_FULL       = 5,
    INT_ISP_BUSY            = 6,
    INT_ISP_IDLE            = 7,
    INT_AWB_DONE            = 8,
    INT_WDMA_DONE           = 9,
    INT_RDMA_DONE           = 10,
    INT_WDMA_FIFO_FULL      = 11,
    INT_PAD_VSYNC_RISING    = 12,
    INT_PAD_VSYNC_FALLING   = 13,
    INT_ISPIF_VSYNC         = 14,
    INT_AE_DONE             = 15,
} ISP_INT_STATUS;

typedef enum {
    INT2_VDOS_EVERYLINE     = 0,
    INT2_VDOS_LINE0         = 1,
    INT2_VDOS_LINE1         = 2,
    INT2_VDOS_LINE2         = 3
} ISP_INT2_STATUS;

#define VSYNC_BIT   9
#define MIPI_BIT    0
#define ISP_CHECKBITS(a, b) ((a) & ((u32)0x01 << (b)))
#define ISP_SETBIT(a, b)    (a) |= (((u32)0x01 << (b)))
#define ISP_CLEARBIT(a, b)  (a) &= (~((u32)0x01 << (b)))


void HalISPMaskInt(void);
void HalISPMaskIntAll(void);

void HalISPDisableInt(void);

void HalISPEnableInt(void);

void HalISPMaskClear1(u32);
void HalISPMaskClear2(u32);
void HalISPClearInt(volatile u32 *clear, ISP_INT_STATUS int_num);
void HalISPClearInt2(volatile u32 *clear, ISP_INT2_STATUS int_num);
void HalISPClearMask(ISP_INT_STATUS int_num);
void HalISPClearMask2(ISP_INT_STATUS int_num);
void HalISPClearMaskAll(void);
void HalISPClearIntAll(void);

void HalISPClkEnable(bool enable);
u32 HalISPGetIntStatus(void);
u32 HalISPGetIntStatus2(void);
u32 HalISPGetRawInt(void);
u32 HalISPGetVsyncPol(void);
u32 HalISPGetMIPI(void);
void HalISPGetAEStatSize(volatile int *size);
void HalISPGetAEStat(volatile unsigned long *Stat);
void HalISPGetAEHistSize(volatile int *size);
void HalISPGetAEHist(volatile unsigned long *Stat);
void HalISPGetAWBStatSize(volatile int *size);
void HalISPGetAWBStat(volatile unsigned long *Stat);
void HalISPGetAFStatSize(volatile int *size);
void HalISPGetAFStat(volatile unsigned long *Stat);

#if 1  // FIXJASON
//void HalISPGetVDOSWidth(volatile int *pWidth);
//void HalISPGetVDOSHight(volatile int *pHeight);
void HalISPGetVDOSPitch(volatile int *pPitch);
void HalISPGetVDOSSize(volatile int *pSize, volatile int Pitch);
void HalISPGetVDOSData(volatile unsigned long *pAddr, volatile int Size);
#endif



#endif //__HAL_ISP_H__

