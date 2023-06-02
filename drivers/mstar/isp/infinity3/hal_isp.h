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

/////////////// MIU MenuLoad ///////////
#define GAMMA_TBL_SIZE                 (256)
#define ALSC_TBL_SIZE                  (4209)
#define DEFECTPIX_TBL_SIZE             (512)
#define FPN_OFFSET_TBL_SIZE            (2816)

#define MLOAD_DMA_SIZE (4096*4)
extern dma_addr_t mload_dma_addr;
extern u8  *mload_virt_addr;

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
    INT3_HIT_LINE_COUNT1      = 6,
    INT3_HIT_LINE_COUNT2      = 7,
    INT3_HIT_LINE_COUNT3      = 8,
    INT3_HDR_HISTO_DONE       = 9,
    INT3_RGBIR_HISTO_DONE     = 10,
    INT3_AWB_ROW_DONE         = 11,
    INT3_HISTO_ROW_DONE       = 12,
    INT3_RESERVED0            = 13,
    INT3_RESERVED1            = 14,
    INT3_RESERVED2            = 15,
    ISP_INT3_MAX,
} ISP_INT3_STATUS;

//Memnuload IQ Table
typedef enum {
  eMLOAD_ID_ALSC_R_TBL    = 0,
  eMLOAD_ID_ALSC_G_TBL    = 1,
  eMLOAD_ID_ALSC_B_TBL    = 2,

  eMLOAD_ID_LN_GMA12TO10_R = 3,
  eMLOAD_ID_LN_GMA12TO10_G = 4,
  eMLOAD_ID_LN_GMA12TO10_B = 5,

  eMLOAD_ID_DP_TBL=6,

  eMLOAD_ID_LN_GMA10TO10_R = 10,
  eMLOAD_ID_LN_GMA10TO10_G = 11,
  eMLOAD_ID_LN_GMA10TO10_B = 12,

  eMLOAD_ID_FPN_OFFSET     = 14,

  eMLOAD_ID_NUM            = 15

} ISP_MLOAD_ID;

typedef struct
{
    u16 alsc_r[ALSC_TBL_SIZE] __attribute__((aligned(16)));
    u16 alsc_g[ALSC_TBL_SIZE] __attribute__((aligned(16)));
    u16 alsc_b[ALSC_TBL_SIZE] __attribute__((aligned(16)));
    u16 gamma12to10_r[GAMMA_TBL_SIZE] __attribute__((aligned(16)));
    u16 gamma12to10_g[GAMMA_TBL_SIZE] __attribute__((aligned(16)));
    u16 gamma12to10_b[GAMMA_TBL_SIZE] __attribute__((aligned(16)));
    u16 dpc[DEFECTPIX_TBL_SIZE] __attribute__((aligned(16)));
    u16 gamma10to10_r[GAMMA_TBL_SIZE] __attribute__((aligned(16)));
    u16 gamma10to10_g[GAMMA_TBL_SIZE] __attribute__((aligned(16)));
    u16 gamma10to10_b[GAMMA_TBL_SIZE] __attribute__((aligned(16)));
    u16 fpn[FPN_OFFSET_TBL_SIZE] __attribute__((aligned(16)));
    char dummy[16] __attribute__((aligned(16)));
}__attribute__((aligned(16))) MLoadLayout;

#ifndef member_size
#define member_size(type, member) sizeof(((type *)0)->member)
#endif
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define VSYNC_BIT   9
#define MIPI_BIT    0
#define ISP_SHIFTBITS(a)      (0x01<<(a))
#define ISP_CHECKBITS(a, b)   ((a) & ((u32)0x01 << (b)))
#define ISP_SETBIT(a, b)      (a) |= (((u32)0x01 << (b)))
#define ISP_CLEARBIT(a, b)    (a) &= (~((u32)0x01 << (b)))

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

void HalISPSetAeBaseAddr(unsigned long Addr,unsigned int size);
void HalISPSetAwbBaseAddr(unsigned long Addr,unsigned int size);
void HalISPSetMotBaseAddr(unsigned long Addr,unsigned int size);
void HalISPSetDnrFbAddr(unsigned long phys_addr, int id);
void HalISPSetAfBaseAddr(unsigned long Addr,unsigned int size);
void HalISPSetHistoBaseAddr(unsigned long Addr,unsigned int size);
void HalISPSetRgbIRBaseAddr(unsigned long Addr,unsigned int size);
void HalISPSetDnrUbound(unsigned long Addr);

void HalISPMLoadWriteData(ISP_MLOAD_ID Sram_Id,volatile unsigned long Addr, size_t size);
void HalISPMLoadWriteAllTable(volatile unsigned long Addr);
void HalISPMLoadReadData(ISP_MLOAD_ID Sram_Id, volatile unsigned short *table, size_t size);
u8 HalISPGetFrameDoneCount(void);
void IspReset(void);
void IspDisable(void);
void IspInputEnable(u32 enable);
void IspAsyncEnable(u32 enable);
void HalISPSetOBC(int u4OBC_a, int u4OBC_b);
void HalIspSetAEDgain(u32 enable,u32 gain);
void HalISPSetYUVCCM(const s16 *ccm_coff);
void HalISPGetImageSize(u32* width,u32* height);
void HalISPGetAeBlkSize(u32* x,u32* y);
u32 HalISPGetRotEn(void);
u32 HalISPWdmaTrigger(isp_ioctl_trigger_wdma_attr wdma_attr);
u32 HalISPVDOSInit(void);

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
u8 * mloadInit(size_t mloadLayoutSize);
void mloadDeInit(void *dmaBuf, size_t mloadLayoutSize);
#endif //__HAL_ISP_H__
