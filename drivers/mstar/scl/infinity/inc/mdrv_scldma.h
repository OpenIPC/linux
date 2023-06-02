////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _MDRV_SCLDMA_H
#define _MDRV_SCLDMA_H


//-------------------------------------------------------------------------------------------------
//  Defines & Enum
//-------------------------------------------------------------------------------------------------
#define MDRV_SCLDMA_BLOCK_POLL_EN       0
#define MDRV_SCLDMA_BLOCK_POLL_ISR_EN   1
#define BUFFER_BE_ALLOCATED_MAX         4
#define MDRV_SCLDMA_BUFFER_QUEUE_OFFSET   sizeof(ST_MDRV_SCLDMA_FRAME_BUFFER_CONFIG)///<Buffer Queue Size For OMX
#define PARSING_SCLDMA_IOBUFMD(x)       (x== E_MDRV_SCLDMA_BUFFER_MD_RING  ? "RING" : \
                                       x==E_MDRV_SCLDMA_BUFFER_MD_SINGLE? "SINGLE" : \
                                       x==E_MDRV_SCLDMA_BUFFER_MD_SWRING? "SWRING " : \
                                                                   "UNKNOWN")

#define PARSING_SCLDMA_IOCOLOR(x)       (x==E_MDRV_SCLDMA_COLOR_YUV422 ? "YUV422" : \
                                       x==E_MDRV_SCLDMA_COLOR_YUV420 ? "YUV420" : \
                                                                  "UNKNOWN")
typedef enum
{
    E_MDRV_SCLDMA_ID_1,
    E_MDRV_SCLDMA_ID_2,
    E_MDRV_SCLDMA_ID_3,
    E_MDRV_SCLDMA_ID_PNL,
    E_MDRV_SCLDMA_ID_NUM,
}EN_MDRV_SCLDMA_ID_TYPE;

typedef enum
{
    E_MDRV_SCLDMA_ACTIVE_BUFFER_OMX         = 0x10,
    E_MDRV_SCLDMA_ACTIVE_BUFFER_OFF         = 0x20,
    E_MDRV_SCLDMA_ACTIVE_BUFFER_RINGFULL    = 0x40,
}EN_MDRV_SCLDMA_ACTIVE_BUFFER_TYPE;

typedef enum
{
    E_MDRV_SCLDMA_BUFFER_MD_RING,   ///< buffer mode: RING
    E_MDRV_SCLDMA_BUFFER_MD_SINGLE, ///< bufer mode : single
    E_MDRV_SCLDMA_BUFFER_MD_SWRING, ///< bufer mode : sw control ring mode
    E_MDRV_SCLDMA_BUFFER_MD_NUM,    ///< The max number of buffer mode
}EN_MDRV_SCLDMA_BUFFER_MODE_TYPE;

typedef enum
{
    E_MDRV_SCLDMA_COLOR_YUV422, ///< color format: 422Pack
    E_MDRV_SCLDMA_COLOR_YUV420, ///< color format: YCSep420
	E_MDRV_SCLDMA_COLOR_NUM,    ///< The max number of color format
}EN_MDRV_SCLDMA_COLOR_TYPE;

typedef enum
{
     E_MDRV_SCLDMA_MEM_FRM  = 0,  ///< memory type: FRM
     E_MDRV_SCLDMA_MEM_SNP  = 1,  ///< memory type: SNP
     E_MDRV_SCLDMA_MEM_IMI  = 2,  ///< memory type: IMI
	 E_MDRV_SCLDMA_MEM_NUM  = 3,  ///< The max number of memory type
}EN_MDRV_SCLDMA_MEM_TYPE;
typedef struct
{
    unsigned char  btsBase_0   : 1;  ///< base0 flag
    unsigned char  btsBase_1   : 1;  ///< base1 flag
    unsigned char  btsBase_2   : 1;  ///< base2 flag
    unsigned char  btsBase_3   : 1;  ///< Base3 flag
    unsigned char  btsReserved : 4;  ///< reserved
}ST_MDRV_SCLDMA_BUFFER_FLAG_BIT;
typedef enum
{
    EN_MDRV_SCLDMA_BUFFER_QUEUE_TYPE_PEEKQUEUE,    ///< buffer queue type: user only to get information
    EN_MDRV_SCLDMA_BUFFER_QUEUE_TYPE_DEQUEUE,      ///< buffer queue type: user can set Read information
    EN_MDRV_SCLDMA_BUFFER_QUEUE_TYPE_ENQUEUE,      ///< TODO : buffer queue type: user can set Write information(not use)
    EN_MDRV_SCLDMA_BUFFER_QUEUE_TYPE_NUM,          ///< buffer type: totally
}EN_MDRV_SCLDMA_USED_BUFFER_QUEUE_TYPE;

//-------------------------------------------------------------------------------------------------
//  Structure
//-------------------------------------------------------------------------------------------------
typedef struct
{
    unsigned long u32Riubase;
}ST_MDRV_SCLDMA_INIT_CONFIG;
typedef struct
{
    void* idclk;
    void* fclk1;
    void* fclk2;
    void* odclk;
}ST_MDRV_SCLDMA_CLK_CONFIG;

typedef struct
{
    unsigned char       bEn;
    EN_MDRV_SCLDMA_MEM_TYPE   enMemType;
    ST_MDRV_SCLDMA_CLK_CONFIG *stclk;
}ST_MDRV_SCLDMA_TRIGGER_CONFIG;
typedef struct
{
    union
    {
        unsigned char  u8Flag;
        ST_MDRV_SCLDMA_BUFFER_FLAG_BIT bvFlag;   ///< buffer flag
    };

    EN_MDRV_SCLDMA_MEM_TYPE   enMemType;         ///< memory type
    EN_MDRV_SCLDMA_COLOR_TYPE enColorType;       ///< color type
    EN_MDRV_SCLDMA_BUFFER_MODE_TYPE enBufMDType; ///< buffer mode

    unsigned short      u16BufNum;    ///< number of buffer
    unsigned long       u32Base_Y[BUFFER_BE_ALLOCATED_MAX]; ///< base address of Y
    unsigned long       u32Base_C[BUFFER_BE_ALLOCATED_MAX]; ///< base address of CbCr
    unsigned short      u16Width;     ///< width of buffer
    unsigned short      u16Height;    ///< height of buffer
} ST_MDRV_SCLDMA_BUFFER_CONFIG;

typedef struct
{
    unsigned char bDone;
}ST_MDRV_SCLDMA_BUFFER_DONE_CONFIG;
typedef struct
{
    EN_MDRV_SCLDMA_MEM_TYPE   enMemType;
    unsigned char u8ActiveBuffer;
    ST_MDRV_SCLDMA_TRIGGER_CONFIG stOnOff;
    unsigned char   u8ISPcount;
    unsigned long   u32FRMDoneTime;
}ST_MDRV_SCLDMA_ACTIVE_BUFFER_CONFIG;
typedef struct
{
    unsigned short u16DMAcount;
    unsigned long u32Trigcount;
    unsigned short u16DMAH;
    unsigned short u16DMAV;
    EN_MDRV_SCLDMA_COLOR_TYPE enColorType;       ///< color type
    EN_MDRV_SCLDMA_BUFFER_MODE_TYPE enBufMDType; ///< buffer mode
    unsigned short u16BufNum;    ///< number of buffer
    unsigned long u32Base_Y[BUFFER_BE_ALLOCATED_MAX]; ///< base address of Y
    unsigned long u32Base_C[BUFFER_BE_ALLOCATED_MAX]; ///< base address of CbCr
    unsigned char bDMAEn;
}ST_MDRV_SCLDMA_ATTR_TYPE;

typedef struct
{
    unsigned char   u8FrameAddrIdx;       ///< ID of Frame address
    unsigned long   u32FrameAddr;         ///< Frame Address
    unsigned char   u8ISPcount;           ///< ISP counter
    unsigned short   u16FrameWidth;         ///< Frame Width
    unsigned short   u16FrameHeight;        ///< Frame Height
    unsigned long long   u64FRMDoneTime;  ///< Time of FRMDone
}__attribute__ ((__packed__))ST_MDRV_SCLDMA_FRAME_BUFFER_CONFIG;

typedef struct
{
    EN_MDRV_SCLDMA_MEM_TYPE   enMemType; ///< memory type
    EN_MDRV_SCLDMA_USED_BUFFER_QUEUE_TYPE  enUsedType;
    ST_MDRV_SCLDMA_FRAME_BUFFER_CONFIG stRead;
    unsigned char  u8InQueueCount;
    unsigned char  u8EnqueueIdx;
}__attribute__ ((__packed__))ST_MDRV_SCLDMA_BUFFER_QUEUE_CONFIG;


//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------
#ifndef _MDRV_SCLDMA_C
#define INTERFACE extern
#else
#define INTERFACE
#endif
INTERFACE unsigned char MDrv_SCLDMA_Init(EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_INIT_CONFIG *pCfg);
INTERFACE unsigned char MDrv_SCLDMA_SetDMAReadClientConfig
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_BUFFER_CONFIG *pCfg);
INTERFACE unsigned char MDrv_SCLDMA_SetDMAReadClientTrigger
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_TRIGGER_CONFIG *pCfg);
INTERFACE unsigned char MDrv_SCLDMA_SetDMAWriteClientConfig
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_BUFFER_CONFIG *pCfg);
INTERFACE unsigned char MDrv_SCLDMA_SetDMAWriteClientTrigger
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_TRIGGER_CONFIG *pCfg);
INTERFACE unsigned char MDrv_SCLDMA_GetInBufferDoneEvent
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, EN_MDRV_SCLDMA_MEM_TYPE enMemType, ST_MDRV_SCLDMA_BUFFER_DONE_CONFIG *pCfg);
INTERFACE unsigned char MDrv_SCLDMA_GetOutBufferDoneEvent
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, EN_MDRV_SCLDMA_MEM_TYPE enMemType, ST_MDRV_SCLDMA_BUFFER_DONE_CONFIG *pCfg);
INTERFACE unsigned char MDrv_SCLDMA_GetDMAReadBufferActiveIdx
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_ACTIVE_BUFFER_CONFIG *pstCfg);
INTERFACE unsigned char MDrv_SCLDMA_GetDMAWriteBufferAcitveIdx
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_ACTIVE_BUFFER_CONFIG *pstCfg);
INTERFACE unsigned char MDrv_SCLDMA_BufferQueueHandle
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_BUFFER_QUEUE_CONFIG *pstCfg);
INTERFACE ST_MDRV_SCLDMA_ATTR_TYPE MDrv_SCLDMA_GetDMAInformationByClient
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID,EN_MDRV_SCLDMA_MEM_TYPE enMemType,unsigned char bYC);
INTERFACE void MDrv_SCLDMA_ResetTrigCountByClient
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID,EN_MDRV_SCLDMA_MEM_TYPE enMemType,unsigned char bYC);
INTERFACE void MDrv_SCLDMA_ResetTrigCountAllClient(void);
INTERFACE void MDrv_SCLDMA_Sys_Init(unsigned char bEn);
INTERFACE void MDrv_SCLDMA_Release(EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID,ST_MDRV_SCLDMA_CLK_CONFIG *stclkcfg);
INTERFACE void MDrv_SCLDMA_ClkClose(ST_MDRV_SCLDMA_CLK_CONFIG* stclk);
INTERFACE unsigned char MDrv_SCLDMA_Suspend(EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID);
INTERFACE void MDrv_SCLDMA_SetPollWait(void *filp, void *pWaitQueueHead, void *pstPollQueue);
INTERFACE unsigned char MDrv_SCLDMA_Resume(EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID);
INTERFACE wait_queue_head_t * MDrv_SCLDMA_GetWaitQueueHead(EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID);
INTERFACE void MDrv_SCLDMA_SetForceCloseDMAClient
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID,EN_MDRV_SCLDMA_MEM_TYPE enMemType,unsigned char bYC,unsigned char bEn);
INTERFACE unsigned char MDrv_SCLDMA_GetDoubleBufferStatus(void);
INTERFACE void MDrv_SCLDMA_SetDoubleBufferConfig(unsigned char bEn);

#undef INTERFACE
#endif
