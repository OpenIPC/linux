//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party`s software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar`s confidential information and you agree to keep MStar`s
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer`s product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (!¡±MStar Confidential Information!¡L) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
//==============================================================================
#ifndef DRV_SCLDMA_ST_H
#define DRV_SCLDMA_ST_H


//-------------------------------------------------------------------------------------------------
//  DEFINE
//-------------------------------------------------------------------------------------------------
#define SCLDMA_DBG_Thread                   0
#define DoubleBufferDefaultSet              0  // 0 close 1 open
#define DoubleBufferStatus                  gbDBStatus
#define ENABLE_RING_DB                      1
#define SCLDMA_IRQ_EN                       0
#define ENABLE_ACTIVEID_ISR                 1
#define SCLDMA_BUFFER_QUEUE_OFFSET            sizeof(ST_SCLDMA_FRAME_BUFFER_CONFIG)
#define SCLDMA_IRQ_EVENT_ID_MSK                 0xFF000000
#define SCLDMA_IRQ_EVENT_CLIENT_MSK             0x000000FF
#define SCLDMA_IRQ_EVENT_CLEAR_IRQNUM_MSK       0x0000FF00
#define SCLDMA_IRQ_EVENT_CLEAR_MODE_MSK         0x00FF0000

#define GET_SCLDMA_IRQ_EVENT_CLIENT(x)              (x & SCLDMA_IRQ_EVENT_CLIENT_MSK)
#define GET_SCLDMA_IRQ_EVENT_CLEAR_IRQNUM(x)        ((x & SCLDMA_IRQ_EVENT_CLEAR_IRQNUM_MSK) >> 8)
#define GET_SCLDMA_IRQ_EVENT_CLEAR_MODE(x)          ((x & SCLDMA_IRQ_EVENT_CLEAR_MODE_MSK) >> 16)

#define SET_SCLDMA_IRQ_EVENT_PENDING(client)        (E_SCLDMA_IRQ_EVENT_PENDING |  \
                                                     (client & 0x000000FF))

#define SET_SCLDMA_IRQ_EVENT_CLEAR(client, mode, irqnum)    (E_SCLDMA_IRQ_EVENT_CLEAR | \
                                                             (client & 0xFF)          | \
                                                             ((irqnum & 0xFF) << 8)   | \
                                                             ((mode & 0xFF)<<16))

//-------------------------------------------------------------------------------------------------
//  ENUM
//-------------------------------------------------------------------------------------------------

typedef enum
{
    E_SCLDMA_ID_1_W,
    E_SCLDMA_ID_2_W,
    E_SCLDMA_ID_3_W,
    E_SCLDMA_ID_3_R,
    E_SCLDMA_ID_PNL_R,
    E_SCLDMA_ID_MAX,    //scldma ID type,I1 has 1_W,2_W,3_R,3_R,PNL_R
}EN_SCLDMA_ID_TYPE;

typedef enum
{
    E_SCLDMA_FRM_W,
    E_SCLDMA_SNP_W,
    E_SCLDMA_IMI_W,
    E_SCLDMA_FRM_R,
    E_SCLDMA_DBG_R,
    E_SCLDMA_IMI_R,
    E_SCLDMA_RW_NUM,    //scldma rwmode, I1 has FRM,SNP,IMI RW
}EN_SCLDMA_RW_MODE_TYPE;

typedef enum
{
    EN_SCLDMA_DB_STATUS_NEED_OFF,
    EN_SCLDMA_DB_STATUS_NEED_ON,
    EN_SCLDMA_DB_STATUS_KEEP,
    EN_SCLDMA_DB_STATUS_NUM,    //double buffer switcher status, 3 choise
}EN_SCLDMA_DB_STATUS_TYPE;

typedef enum
{
    E_SCLDMA_1_FRM_W = 0,
    E_SCLDMA_1_SNP_W = 1,
    E_SCLDMA_1_IMI_W = 2,
    E_SCLDMA_2_FRM_W = 3,
    E_SCLDMA_2_IMI_W = 4,
    E_SCLDMA_3_FRM_R = 5,
    E_SCLDMA_3_FRM_W = 6,
    E_SCLDMA_4_FRM_R = 7,
    E_SCLDMA_3_IMI_R = 8,
    E_SCLDMA_CLIENT_NUM,    //scldma client ,I1 has dma_1:3 cli dma_2 :2cli dma_3:In/Out debug:pnl &dma_3_IMIR
}EN_SCLDMA_CLIENT_TYPE;


typedef enum
{
    E_SCLDMA_BUF_MD_RING,
    E_SCLDMA_BUF_MD_SINGLE,
    E_SCLDMA_BUF_MD_SWRING,
    E_SCLDMA_BUF_MD_NUM,    //scldma DMA write mode,I1 has 2 type: Ring(HW),Single(HW+SW)
}EN_SCLDMA_BUFFER_MODE_TYPE;

typedef enum
{
    E_SCLDMA_COLOR_YUV422,
    E_SCLDMA_COLOR_YUV420,
    E_SCLDMA_COLOR_NUM,    //I1 dma has 2 color type 422,420
}EN_SCLDMA_COLOR_TYPE;

typedef enum
{
    E_SCLDMA_VS_ID_SC    = 0,
    E_SCLDMA_VS_ID_AFF   = 1,
    E_SCLDMA_VS_ID_LDC   = 2,
    E_SCLDMA_VS_ID_SC3   = 3,
    E_SCLDMA_VS_ID_DISP  = 4,
    E_SCLDMA_VS_ID_NUM   = 5,   //scl handle vsync type,I1 can handle in AFF,LDC,SC3,DISP, suggest sc1 use HW mode ,sc3 use regen mode
}EN_SCLDMA_VS_ID_TYPE;



typedef enum
{
    E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC = 0,
    E_SCLDMA_VS_TRIG_MODE_HW_DELAY    = 1,
    E_SCLDMA_VS_TRIG_MODE_SWTRIGGER   = 2,
    E_SCLDMA_VS_TRIG_MODE_DISP_FM_END = 3,
    E_SCLDMA_VS_TRIG_MODE_NUM         = 4,
}EN_SCLDMA_VS_TRIG_MODE_TYPE;


typedef enum
{
    E_SCLDMA_REF_VS_REF_MODE_FALLING = 0,
    E_SCLDMA_REF_VS_REF_MODE_RASING  = 1,
    E_SCLDMA_REF_VS_REF_MODE_NUM     = 2,
}EN_SCLDMA_REGEN_VS_REF_MODE_TYPE;


typedef enum
{
    E_SCLDMA_IRQ_EVENT_TIMER   = 0x01000000,
    E_SCLDMA_IRQ_EVENT_PENDING = 0x02000000,
    E_SCLDMA_IRQ_EVENT_CLEAR   = 0x03000000,
    E_SCLDMA_IRQ_EVENT_ALL     = 0x0FFFFFFF,
}EN_SCLDMA_IRQ_EVENT_TYPE;

typedef enum
{
    E_SCLDMA_IRQ_MODE_PENDING  = 0x00,
    E_SCLDMA_IRQ_MODE_ACTIVE   = 0x01,
    E_SCLDMA_IRQ_MODE_ACTIVE_N = 0x02,
    E_SCLDMA_IRQ_MODE_SIDONE   = 0x03,
    E_SCLDMA_IRQ_MODE_END      = 0x04,
    E_SCLDMA_IRQ_MODE_DONE     = 0x06,
    E_SCLDMA_IRQ_MODE_ALL      = 0x07,
}EN_SCLDMA_IRQ_MODE_TYPE;

typedef enum
{
    E_SCLDMA_ACTIVE_BUFFER_OMX          = 0xF0, // OMX buffer status ,driver reserve.
    E_SCLDMA_ACTIVE_BUFFER_SCL          = 0x03, // SCL buffer status, save active/done buffer idx
    E_SCLDMA_ACTIVE_BUFFER_OFF          = 0x04, // already not use
    E_SCLDMA_ACTIVE_BUFFER_ACT          = 0x08, // already not use
    E_SCLDMA_ACTIVE_BUFFER_SCLANDFLAG   = 0x0F, // like E_SCLDMA_ACTIVE_BUFFER_SCL
    E_SCLDMA_ACTIVE_BUFFER_OMX_FLAG     = 0x10, // it's represent OMX is update empty buffer to buffer
    E_SCLDMA_ACTIVE_BUFFER_OMX_TRIG     = 0x20, // it's represent dma turn off and driver update to OMX
    E_SCLDMA_ACTIVE_BUFFER_OMX_RINGFULL = 0x40, // it's represent dma already done one buffer, but OMX isn't to receive.
}EN_SCLDMA_ACTIVE_BUFFER_TYPE;

//bit0 next time off , bit 1 blanking ,bit 2 DMAonoff, bit3 no DMA on ,bit4 ever DMA on ,but already off or open again
typedef enum
{
    E_SCLDMA_FLAG_NEXT_ON   = 0x01, // for non double buffer mode, if active wait for blanking
    E_SCLDMA_FLAG_BLANKING  = 0x02, // if reset dma,active id is temporarily state, need to handle until first frame done.
    E_SCLDMA_FLAG_DMATRIGON = 0x03, // already not use
    E_SCLDMA_FLAG_DMAOFF    = 0x04, // ref E_SCLDMA_ACTIVE_BUFFER_OFF,dma off
    E_SCLDMA_FLAG_ACTIVE    = 0x08, // ref E_SCLDMA_ACTIVE_BUFFER_ACT,dma act
    E_SCLDMA_FLAG_EVERDMAON = 0x10, // like E_SCLDMA_ACTIVE_BUFFER_OMX_RINGFULL
    E_SCLDMA_FLAG_FRMIN     = 0x20, // this frame is integrate.
    E_SCLDMA_FLAG_FRMDONE   = 0x40, // dma done.(idle)
}EN_SCLDMA_FLAG_TYPE;

typedef enum
{
    E_SCLDMA_ISR_LOG_ISPOFF     = 0x1,
    E_SCLDMA_ISR_LOG_SNPONLY    = 0x2,
    E_SCLDMA_ISR_LOG_SNPISR     = 0x3,
    E_SCLDMA_ISR_LOG_ISPON      = 0x4,
    E_SCLDMA_ISR_LOG_SC1ON      = 0x5,
    E_SCLDMA_ISR_LOG_SC1OFF     = 0x6,
    E_SCLDMA_ISR_LOG_SC2ON      = 0x7,
    E_SCLDMA_ISR_LOG_SC2OFF     = 0x8,
    E_SCLDMA_ISR_LOG_SC1A       = 0x9,
    E_SCLDMA_ISR_LOG_SC1N       = 0xA,
    E_SCLDMA_ISR_LOG_SC2A       = 0xB,
    E_SCLDMA_ISR_LOG_SC2N       = 0xC,
    E_SCLDMA_ISR_LOG_FRMEND     = 0xD,//isr log status
}EN_SCLDMA_ISR_LOG_TYPE;


//-------------------------------------------------------------------------------------------------
//  Structure
//-------------------------------------------------------------------------------------------------
typedef struct
{
    MS_U8 u8RPoint;//R-OMX
    MS_U8 u8WPoint;//W-SCL
    MS_U8 u8LPoint;// L-last time scl
    MS_BOOL bRWequal;
    MS_BOOL bRPointChange;
    EN_SCLDMA_CLIENT_TYPE enClientType;
}ST_SCLDMA_POINT_CONFIG;
typedef struct
{
    MS_BOOL flag;
    MS_S32 s32Taskid;
    MSOS_ST_TASKSTRUCT sttask;
}ST_SCLDMA_THREAD_CONFIG;

typedef struct
{
    MS_U32 u32RIUBase;
}ST_SCLDMA_INIT_CONFIG;

typedef struct
{
    MS_U32 u32IRQNum;
}ST_SCLDMA_SUSPEND_RESUME_CONFIG;

typedef struct
{
    MS_U8 btsBase_0   : 1;
    MS_U8 btsBase_1   : 1;
    MS_U8 btsBase_2   : 1;
    MS_U8 btsBase_3   : 1;
    MS_U8 btsReserved : 4;
}ST_SCLDMA_RW_FLGA_TYPE;

typedef struct
{
    union
    {
        MS_U8 u8Flag;
        ST_SCLDMA_RW_FLGA_TYPE bvFlag;
    };

    EN_SCLDMA_RW_MODE_TYPE enRWMode;
    EN_SCLDMA_COLOR_TYPE enColor;
    EN_SCLDMA_BUFFER_MODE_TYPE enBuffMode;
    MS_U32 u32Base_Y[4];
    MS_U32 u32Base_C[4];
    MS_U8  u8MaxIdx;
    MS_U16 u16Width;
    MS_U16 u16Height;
}ST_SCLDMA_RW_CONFIG;

typedef struct
{
    MSOS_ST_CLK* idclk;
    MSOS_ST_CLK* fclk1;
    MSOS_ST_CLK* fclk2;
    MSOS_ST_CLK* odclk;
}ST_SCLDMA_CLK_CONFIG;

typedef struct
{
    MS_BOOL bEn;
    EN_SCLDMA_RW_MODE_TYPE enRWMode;
    ST_SCLDMA_CLK_CONFIG *stclk;
}ST_SCLDMA_ONOFF_CONFIG;

typedef struct
{
    EN_SCLDMA_VS_TRIG_MODE_TYPE enTrigMd;
    EN_SCLDMA_REGEN_VS_REF_MODE_TYPE enVsRefMd;
    MS_U16 u16Vs_Width;
    MS_U16 u16Vs_St;
}ST_SCLDMA_VS_CONFIG;


typedef struct
{
    MS_BOOL bDone;
    EN_SCLDMA_RW_MODE_TYPE enRWMode;
}ST_SCLDMA_DONE_CONFIG;

typedef struct
{
    MS_U8 u8ActiveBuffer;
    EN_SCLDMA_RW_MODE_TYPE enRWMode;
    ST_SCLDMA_ONOFF_CONFIG stOnOff;
    MS_U8       u8ISPcount;
    MS_U32      u32FRMDoneTime;
}ST_SCLDMA_ACTIVE_BUFFER_CONFIG;
typedef struct
{
    unsigned char   u8FrameAddrIdx;       ///< ID of Frame address
    unsigned long   u32FrameAddr;         ///< Frame Address
    unsigned char   u8ISPcount;           ///< ISP counter
    unsigned short  u16FrameWidth;         ///< Frame Width
    unsigned short  u16FrameHeight;        ///< Frame Height
    unsigned long long   u64FRMDoneTime;       ///< Time of FRMDone
}__attribute__ ((__packed__))ST_SCLDMA_FRAME_BUFFER_CONFIG;

typedef struct
{
    EN_SCLDMA_RW_MODE_TYPE enRWMode;
    EN_SCLDMA_ID_TYPE enID;
    unsigned char bUsed;
    unsigned char bFull;
    unsigned char u8Bufferflag;
    ST_SCLDMA_FRAME_BUFFER_CONFIG* pstHead;
    ST_SCLDMA_FRAME_BUFFER_CONFIG* pstTail;
    ST_SCLDMA_FRAME_BUFFER_CONFIG* pstWrite;
    ST_SCLDMA_FRAME_BUFFER_CONFIG* pstWriteAlready;
    ST_SCLDMA_FRAME_BUFFER_CONFIG* pstRead;
    unsigned char  u8InQueueCount;
    unsigned char  u8NextActiveId;
    unsigned char  u8AccessId;
}ST_SCLDMA_BUFFER_QUEUE_CONFIG;

typedef struct
{
    ST_SCLDMA_VS_CONFIG stVsCfg[E_SCLDMA_VS_ID_NUM];
    EN_SCLDMA_BUFFER_MODE_TYPE enBuffMode[E_SCLDMA_CLIENT_NUM];
    MS_BOOL bDMAOnOff[E_SCLDMA_CLIENT_NUM];
    MS_BOOL bDMAidx[E_SCLDMA_CLIENT_NUM];
    MS_BOOL bMaxid[E_SCLDMA_CLIENT_NUM];
    MS_BOOL bDmaflag[E_SCLDMA_CLIENT_NUM];//bit0 next time off , bit 1 blanking ,bit 2 DMAonoff, bit3 no DMA on
    MS_U64 u64mask;
    EN_SCLDMA_COLOR_TYPE enColor[E_SCLDMA_CLIENT_NUM];
    MS_U32 u32Base_Y[E_SCLDMA_CLIENT_NUM][4];
    MS_U32 u32Base_C[E_SCLDMA_CLIENT_NUM][4];
    MS_BOOL u16FrameWidth[E_SCLDMA_CLIENT_NUM];
    MS_BOOL u16FrameHeight[E_SCLDMA_CLIENT_NUM];
#if SCLDMA_IRQ_EN
    MS_S32 s32IrqTaskid;
    MS_S32 s32IrqEventId;
    MS_S32 s32IrqTimerId;
#endif
}ST_SCLDMA_INFO_TYPE;
typedef struct
{
    MS_U16 u16DMAcount;
    MS_U16 u16DMAH;
    MS_U16 u16DMAV;
    MS_U32 u32Trigcount;
    EN_SCLDMA_COLOR_TYPE enColor;
    EN_SCLDMA_BUFFER_MODE_TYPE enBuffMode;
    MS_U32 u32Base_Y[4];
    MS_U32 u32Base_C[4];
    MS_U8  u8MaxIdx;
    MS_U8  bDMAEn;
}ST_SCLDMA_ATTR_TYPE;
//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  extern Variable
//-------------------------------------------------------------------------------------------------
/////////////////
/// ScldmaInfo
/// use in Drvscldma and Drvsclirq
/// record all DMA client status
////////////////
extern ST_SCLDMA_INFO_TYPE  gstScldmaInfo;
extern MS_BOOL gbDBStatus;
#endif
