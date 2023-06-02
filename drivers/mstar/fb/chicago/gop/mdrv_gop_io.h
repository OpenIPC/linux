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

#ifndef _MDRV_GOP_IO_H
#define _MDRV_GOP_IO_H

//=============================================================================
// Includs
//=============================================================================

//=============================================================================
// Defines
//=============================================================================
// library information
#define MSIF_GOP_LIB_CODE               {'G','O','P','\0'}
#define MSIF_GOP_BUILDNUM               {'_','0','1','\0'}
#define MSIF_GOP_LIBVER                 (2)
#define MSIF_GOP_CHANGELIST             (677450)

//IO Ctrl defines:
#define GOP_IOC_SET_BASE_ADDR_NR        (0)
#define GOP_IOC_SET_MODE_NR             (1)
#define GOP_IOC_SET_CONSTANT_ALPHA_NR   (2)
#define GOP_IOC_SET_COLOR_KEY_NR        (3)
#define GOP_IOC_MAX_NR                  (4)


// use 'm' as magic number
#define GOP_IOC_MAGIC                   ('3')
#define GOP_IOC_SET_BASE_ADDR           _IOWR(GOP_IOC_MAGIC,  GOP_IOC_SET_BASE_ADDR_NR,         GOP_SET_BASE_ADDR_CONFIG)
#define GOP_IOC_SET_MODE                _IOWR(GOP_IOC_MAGIC,  GOP_IOC_SET_MODE_NR,              GOP_SETMODE_CONFIG)
#define GOP_IOC_SET_CONSTANT_ALPHA      _IOWR(GOP_IOC_MAGIC,  GOP_IOC_SET_CONSTANT_ALPHA_NR,    GOP_SET_CONSTANT_ALPHA_CONFIG)
#define GOP_IOC_SET_COLOR_KEY           _IOWR(GOP_IOC_MAGIC,  GOP_IOC_SET_COLOR_KEY_NR,         GOP_SET_COLOR_KEY_CONFIG)
//=============================================================================
// enum
//=============================================================================
typedef enum
{
    E_GOP_0,
    E_GOP_1,
    E_GOP_2,
    E_GOP_3,
    E_GOP_MAX,
}E_GOP_NUM_TYPE;


typedef enum
{
    E_GOP_SRC_RGB565,
    E_GOP_SRC_ARGB8888,
    E_GOP_SRC_ABGR8888,
}E_GOP_SRC_FORMAT_TYPE;
//=============================================================================
// struct
//=============================================================================
#ifndef _MS_TYPES_H_
typedef unsigned char               MS_U8;
typedef unsigned short              MS_U16;
typedef unsigned long               MS_U32;
typedef unsigned long long          MS_U64;
typedef unsigned char               MS_BOOL;
typedef signed short                MS_S16;
#endif

typedef struct
{
    E_GOP_NUM_TYPE enGopNum;
    MS_U32 u32Base;
}GOP_SET_BASE_ADDR_CONFIG;


typedef struct
{
    E_GOP_NUM_TYPE enGopNum;
    E_GOP_SRC_FORMAT_TYPE enSrcFormat;
    MS_U16 u16width;
    MS_U16 u16height;
    MS_BOOL bInterlace;
    MS_U32 u32BaseAddr;
    MS_BOOL bYUVOutput;
}GOP_SETMODE_CONFIG;


typedef struct
{
    E_GOP_NUM_TYPE enGopNum;
    MS_BOOL bEn;
    MS_U8 u8Alpha;
}GOP_SET_CONSTANT_ALPHA_CONFIG;


typedef struct
{
    E_GOP_NUM_TYPE enGopNum;
    MS_BOOL bEn;
    MS_U8 u8R;
    MS_U8 u8G;
    MS_U8 u8B;
}GOP_SET_COLOR_KEY_CONFIG;
#endif //_MDRV_GFLIP_IO_H



