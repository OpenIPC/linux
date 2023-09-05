/*
* mdrv_dip_io_st.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: karl.xiao <karl.xiao@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef _MDRV_DIP_IO_ST_H
#define _MDRV_DIP_IO_ST_H

#include "mhal_divp_datatype.h"

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
} DIP_DEST_FMT_TYPE;


typedef enum
{
    DIP_TRIGGER_LOOP = 0,
    DIP_TRIGGER_ONCE,
} DIP_TRIGGER_TYPE;

//=============================================================================
// struct
//=============================================================================
typedef struct
{
    unsigned char bEn;
    unsigned long  u32DIPW_Signal_PID;
} DIP_INTERRUPT_CONFIG;

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
} DIP_CONFIG, *PDIP_CONFIG;


typedef struct
{
    unsigned long u32BuffAddress;
    unsigned long u32C_BuffAddress;
    unsigned char bTrig;
} DIP_WONCE_BASE_CONFIG, *PDIP_WONCE_BASE_CONFIG;


typedef struct
{
    unsigned char u8DIPW_Status;
} DIP_INTR_STATUS, *PDIP_INTR_STATUS;

typedef struct
{
    MS_BOOL bEnable;
    MS_U16 u16WinHStart;
    MS_U16 u16WinHSize;
    MS_U16 u16WinVStart;
    MS_U16 u16WinVSize;

    MS_U16 u16ColorR;
    MS_U16 u16ColorG;
    MS_U16 u16ColorB;
} DIP_COVER_PROPERTY;

typedef struct
{
    MS_U32 eId;
    MS_U16 u16MaxWidth;
    MS_U16 u16MaxHeight;
    MS_U32 Ctxid;
} IOCTL_DIVP_Instance_t, *pIOCTL_DIVP_Instance_t;

typedef struct
{
    MS_U32 Ctxid;
    MHAL_DIVP_InputInfo_t stInputInfo;
    MHAL_DIVP_OutPutInfo_t stOutputInfo;
} IOCTL_DIVP_ProcessDramInfo_t, *pIOCTL_DIVP_ProcessDramInfo_t;

typedef struct
{
    MS_U32 Ctxid;
    MHAL_DIVP_CaptureInfo_t stCaptureInfo;
} IOCTL_DIVP_CaptureInfo_t, *pIOCTL_DIVP_CaptureInfo_t;

typedef struct
{
    MS_U32 Ctxid;
    MHAL_DIVP_TnrLevel_e eTnrLevel;//TNR level
} IOCTL_DIVP_TnrLevel_t, *pIOCTL_DIVP_TnrLevel_t;

typedef struct
{
    MS_U32 Ctxid;
    MHAL_DIVP_DiType_e eDiType;//DI type
} IOCTL_DIVP_DiType_t, *pIOCTL_DIVP_DiType_t;

typedef struct
{
    MS_U32 Ctxid;
    MHAL_DIVP_Rotate_e eRotateType;//rotate angle
} IOCTL_DIVP_Rotate_t, *pIOCTL_DIVP_Rotate_t;

typedef struct
{
    MS_U32 Ctxid;
    MHAL_DIVP_Window_t stCropWin;//crop information
} IOCTL_DIVP_Window_t, *pIOCTL_DIVP_Window_t;

typedef struct
{
    MS_U32 Ctxid;
    MHAL_DIVP_Mirror_t stMirror;
} IOCTL_DIVP_Mirror_t, *pIOCTL_DIVP_Mirror_t;

typedef struct
{
    MS_U64 u64BufAddr[3];
    MS_U32 u32BufSize;
    MS_U16 u16Width;
    MS_U16 u16Height;
    MHAL_DIVP_PixelFormat_e eFmt;
} IOCTL_DIVP_Buffer_Config_t, *pIOCTL_DIVP_Buffer_Config_t;

typedef struct
{
    MS_U32 Ctxid;
    DIP_COVER_PROPERTY stCoverProperty;
} IOCTL_DIVP_Cover_t, *pIOCTL_DIVP_Cover_t;

//=============================================================================

//=============================================================================



#endif //_MDRV_GFLIP_IO_H



