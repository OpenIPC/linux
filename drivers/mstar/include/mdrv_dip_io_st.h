///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
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
///////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// @file   mdrv_sca_io.h
// @brief  GFlip KMD Driver Interface
// @author MStar Semiconductor Inc.
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MDRV_DIP_IO_ST_H
#define _MDRV_DIP_IO_ST_H


//=============================================================================
// enum
//=============================================================================
typedef enum
{
    DIP_DEST_FMT_YC422,
    DIP_DEST_FMT_RGB565,
    DIP_DEST_FMT_ARGB8888,
    DIP_DEST_FMT_YC420_MVOP,
    DIP_DEST_FMT_YC420_MFE,
}DIP_DEST_FMT_TYPE;


typedef enum
{
    DIP_TRIGGER_LOOP = 0,
    DIP_TRIGGER_ONCE,
}DIP_TRIGGER_TYPE;

//=============================================================================
// struct
//=============================================================================
typedef struct
{
    unsigned char bEn;
    unsigned long  u32DIPW_Signal_PID;
}DIP_INTERRUPT_CONFIG;

typedef struct
{
    unsigned short x;
    unsigned short y;    ///<start y of the window
    unsigned short width;
    unsigned short height;
} DIP_WINDOW_TYPE, *PDIP_WINDOW_TYPE;


typedef struct
{
    DIP_DEST_FMT_TYPE enDestFmtType;
    unsigned char bClipEn;
    DIP_WINDOW_TYPE stClipWin;
    unsigned char u8FrameNum;
    unsigned long u32BuffAddress;
    unsigned long u32BuffSize;
    unsigned long u32C_BuffAddress;
    unsigned long u32C_BuffSize;
    unsigned short u16Width;
    unsigned short u16Height;
    unsigned char bTriggle;
    DIP_TRIGGER_TYPE enTrigMode;
}DIP_CONFIG, *PDIP_CONFIG;


typedef struct
{
    unsigned long u32BuffAddress;
    unsigned long u32C_BuffAddress;
    unsigned char bTrig;
}DIP_WONCE_BASE_CONFIG, *PDIP_WONCE_BASE_CONFIG;


typedef struct
{
    unsigned char u8DIPW_Status;
}DIP_INTR_STATUS, *PDIP_INTR_STATUS;

//=============================================================================

//=============================================================================



#endif //_MDRV_GFLIP_IO_H



