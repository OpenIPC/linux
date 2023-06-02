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
    ISP_INT_MAX,
} ISP_INT_STATUS;

typedef enum {
    INT2_VDOS_EVERYLINE     = 0,
    INT2_VDOS_LINE0         = 1,
    INT2_VDOS_LINE1         = 2,
    INT2_VDOS_LINE2         = 3,
    ISP_INT2_MAX,
} ISP_INT2_STATUS;

typedef enum {
    INT3_AE_WIN0_DONE         = 0,
    INT3_AE_WIN1_DONE         = 1,
    INT3_AE_BLK_ROW_INT_DONE  = 2,
    INT3_MENULOAD_DONE        = 3,
    INT3_SW_INT_INPUT_DONE    = 4,
    INT3_SW_INT_OUTPUT_DONE   = 5,
    ISP_INT3_MAX,
} ISP_INT3_STATUS;

//Memnuload IQ Table
typedef enum {
    IQ_MEM_ALSC_RGAIN       = 0,
    IQ_MEM_ALSC_GGAIN       = 1,
    IQ_MEM_ALSC_BGAIN       = 2,
    IQ_MEM_GAMMA12TO10_RTBL = 3,
    IQ_MEM_GAMMA12TO10_GTBL = 4,
    IQ_MEM_GAMMA12TO10_BTBL = 5,
    IQ_MEM_DEFECT_PIXEL_POS = 6,
    IQ_MEM_GAMMA10TO12_RTBL = 7,
    IQ_MEM_GAMMA10TO12_GTBL = 8,
    IQ_MEM_GAMMA10TO12_BTBL = 9,
    IQ_MEM_GAMMA_CORR_RTBL  = 10,
    IQ_MEM_GAMMA_CORR_GTBL  = 11,
    IQ_MEM_GAMMA_CORR_BTBL  = 12,

    IQ_MEM_NUM = 13,

} IQ_MEM_ID;

#define VSYNC_BIT   9
#define MIPI_BIT    0
#define ISP_SHIFTBITS(a)      (0x01<<(a))
#define ISP_CHECKBITS(a, b)   ((a) & ((u32)0x01 << (b)))
#define ISP_SETBIT(a, b)      (a) |= (((u32)0x01 << (b)))
#define ISP_CLEARBIT(a, b)    (a) &= (~((u32)0x01 << (b)))

#define IQ_LEN_ALSC_GAIN            (4209)
#define IQ_LEN_GAMMA_12TO10         (256)
#define IQ_LEN_DEFECT_PIXEL         (2048)
#define IQ_LEN_GAMMA_10TO12         (256)
#define IQ_LEN_GAMMA_CORRECT        (256)


void HalInitRegs(void **pRegs);
void HalISPMaskInt(void);
void HalISPMaskIntAll(void);
void HalISPClearMaskAll(void);
void HalISPClearIntAll(void);
void HalISPDisableInt(void);
void HalISPEnableInt(void);

void HalISPMaskClear1(u32);
void HalISPMaskClear2(u32);
void HalISPMaskClear3(u32);

void HalISPClearInt(volatile u32 *clear, ISP_INT_STATUS int_num);

void HalISPMaskInt1(u32 mask);
void HalISPMaskInt2(u32 mask);
void HalISPMaskInt3(u32 mask);

void HalISPClearMask1(ISP_INT_STATUS int_num);
void HalISPClearMask2(ISP_INT2_STATUS int_num);
void HalISPClearMask3(ISP_INT3_STATUS int_num);

u32 HalISPGetIntStatus1(void);
u32 HalISPGetIntStatus2(void);
u32 HalISPGetIntStatus3(void);

void HalISPClkEnable(bool enable);
u32 HalISPGetRawInt(void);
u32 HalISPGetVsyncPol(void);
u32 HalISPGetMIPI(void);

void HalISPGetAFStat(volatile unsigned long *Stat);

void HalISPGetVDOSPitch(volatile int *pPitch);
void HalISPGetVDOSSize(volatile int *pSize, volatile int Pitch);
void HalISPGetVDOSData(volatile unsigned long *pAddr, volatile int Size);

void HalISPSetAeBaseAddr(volatile unsigned long Addr);
void HalISPSetAwbBaseAddr(volatile unsigned long Addr);
void HalISPSetMotBaseAddr(volatile unsigned long Addr);
void HalISPSetDnrFbAddr(unsigned long phys_addr, int id);

void HalISPMLoadWriteData(volatile unsigned int Sram_Id,volatile unsigned long Addr);
void HalISPMLoadWriteAllTable(volatile unsigned long Addr, volatile unsigned long *offset);
void HalISPMLoadReadData(volatile unsigned int Sram_Id, volatile unsigned short Offset, volatile unsigned short *Data);
u8 HalISPGetFrameDoneCount(void);
void IspReset(void);
void IspDisable(void);
void IspInputEnable(u32 enable);
void IspAsyncEnable(u32 enable);
void HalISPSetOBC(int u4OBC_a, int u4OBC_b);
void HalISPSetRGBCCM(const s16 *ccm_coff);
void HalISPSetYUVCCM(const s16 *ccm_coff);
//// CSI ////
typedef enum
{
  RPT_INT_LINE   = 0,
  RPT_INT_FRAME   = 1,
  RPT_INT_VC0  = 6,
  RPT_INT_VC1  = 5,
  RPT_INT_VC2  = 4,
  RPT_INT_VC4  = 3
}CSI_RPT_INT_STATUS;

//// CSI Error////
typedef enum
{
  ERR_INT_DT          =  0,
  ERR_INT_PA_LENS     =  1,
  ERR_INT_PH_LENS     =  2,
  ERR_INT_ECC_ONEBIT  =  3,
  ERR_INT_FRAME_START =  4,
  ERR_INT_FRAME_END   =  5,
  ERR_INT_ECC_TWOBIT  =  6,
  ERR_INT_CRC         =  7,
  ERR_INT_PA_WC_EQ0   =  8,
  ERR_INT_RAW10_LENS  =  9,
  ERR_INT_CON_FE      = 10,
  ERR_INT_CON_FS      = 11,
  ERR_INT_LE          = 12,
  ERR_INT_LS          = 13,
  ERR_INT_OVERRUN     = 14,
  CSI_ERR_INT_MAX = 15,
}CSI_ERR_INT_STATUS;

void* HalCsi_Open(void* reg_base);
void HalCsi_Close(void **handle);
void HalCsi_RegInit(void* handle);
void HalCsi_RegUninit(void* handle);
void HalCsi_ClrRptInt(void* handle);
u32 HalCsi_GetRptIntStatus(void* handle);
void HalCsi_MaskRptInt(void* handle,u32 mask);
void HalCsi_ClearRptInt(void* handle, u32 clear);
u32 HalCsi_GetErrorIntStatus(void* handle);
void HalCsi_MaskErrorInt(void* handle,u32 mask);
void HalCsi_ClearErrorInt(void* handle, u32 clear);
void HalCsi_ErrIntMaskSet(void* handle,u32 mask);
u32 HalCsi_ErrIntMaskGet(void* handle);
#endif //__HAL_ISP_H__
