////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2017 MStar Semiconductor, Inc.
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

#ifndef __HAL_JPE_OPS_H__
#define __HAL_JPE_OPS_H__

#include "hal_jpe_defs.h"

#if defined(__linux__)  //#if defined(__linux__)
#define JPE_DEV_STATE_NULL  -1
#define JPE_DEV_STATE_IDLE   0
#define JPE_DEV_STATE_BUSY   1
#else
#define JPE_DEV_STATE_NULL  -1
#define JPE_DEV_STATE_IDLE   1  // 0
#define JPE_DEV_STATE_BUSY   0  // 1
#endif

#define JPE_CTX_STATE_NULL  -1
#define JPE_CTX_STATE_IDLE   0
#define JPE_CTX_STATE_BUSY   1

#define JPE_USER_DATA_SIZE  (4*1024)

typedef enum
{
    JPE_IBUF_ROW_MODE   = 0x0,
    JPE_IBUF_FRAME_MODE = 0x1
} JpeInBufMode_e;

typedef enum
{
    JPE_COLOR_PLAN_LUMA     = 0,
    JPE_COLOR_PLAN_CHROMA   = 1,
    JPE_COLOR_PLAN_MAX      = 2
} JPE_COLOR_PLAN_e;

typedef enum
{
    JPE_RAW_YUYV = 0x0,
    JPE_RAW_YVYU = 0x1,
    JPE_RAW_NV12 = 0x3,
    JPE_RAW_NV21 = 0x4,
} JpeRawFormat_e;

typedef enum
{
    JPE_CODEC_JPEG          = 0x1,
    JPE_CODEC_H263I         = 0x2,  //! obsolete
    JPE_CODEC_ENCODE_DCT    = 0x4,  //! obsolete
} JpeCodecFormat_e;

typedef enum
{
    JPE_IDLE_STATE          = 0,
    JPE_BUSY_STATE          = 1,
    JPE_FRAME_DONE_STATE    = 2,
    JPE_OUTBUF_FULL_STATE   = 3,
    JPE_INBUF_FULL_STATE    = 4
} JpeState_e;

typedef struct
{
    u32 u32JpeId;
    u32 nRefYLogAddrAlign[2];
    u32 nRefCLogAddrAlign[2];
    u32 nOutBufSLogAddrAlign;
    u8 nSclHandShakeSupport;
    u8 nCodecSupport;
    u8 nBufferModeSupport;
} JpeCaps_t;

typedef struct
{
    unsigned long nAddr;
    unsigned long nOrigSize;
    unsigned long nOutputSize;
    JpeState_e eState;
} JpeBitstreamInfo_t;

typedef struct
{
    unsigned long nAddr;
    unsigned long nSize;
} JpeBufInfo_t;

typedef struct
{
    JpeInBufMode_e   eInBufMode;
    JpeRawFormat_e   eRawFormat;
    JpeCodecFormat_e eCodecFormat;

    u32 nWidth;
    u32 nHeight;
    u16 YQTable[64];
    u16 CQTable[64];
    u16 nQScale;

    JpeBufInfo_t InBuf[JPE_COLOR_PLAN_MAX];
    JpeBufInfo_t OutBuf;

    u32 nJpeOutBitOffset;

} JpeCfg_t;


typedef struct
{
    JpeHalHandle_t tJpeHalHandle;
    JpeCfg_t       tJpeCfg;
    JpeDevStatus_e eJpeDevStatus;
    u32            nEncodeSize;
} JpeHandle_t;


typedef struct JpeEncOutbuf_t
{
    unsigned long   nAddr;
    unsigned long   nOrigSize;              // Original buffer Size
    unsigned long   nOutputSize;            // Output Size
    JpeState_e      eState;
} JpeEncOutbuf_t;


typedef struct JpeOpsCB_t JpeOpsCB_t;
struct JpeOpsCB_t
{
    char   jpename[16];
    void* (*jpeJob)(JpeOpsCB_t*);                               //! get mhve_job object.
    JPE_IOC_RET_STATUS_e(*init)(JpeOpsCB_t*, JpeCfg_t*);        //! initialize.
    JPE_IOC_RET_STATUS_e(*getCaps)(JpeOpsCB_t*, JpeCaps_t*);    //! get capabilities.
    JPE_IOC_RET_STATUS_e(*setConf)(JpeOpsCB_t*, JpeCfg_t*);     //! apply config from JpegCfg
    JPE_IOC_RET_STATUS_e(*getConf)(JpeOpsCB_t*, JpeCfg_t*);     //! query config.
    void (*release)(JpeOpsCB_t*);                               //! release this object.
};


typedef struct JpeOps_t
{
    JpeOpsCB_t          ops;
    JpeEncOutbuf_t     *pEncOutBuf;
    JpeHalHandle_t      tJpeHalHandle;
    unsigned short      nClkSelect;
} JpeOps_t;

JpeOpsCB_t* JpeOpsAcquire(int id);

#endif //__HAL_JPE_OPS_H__
