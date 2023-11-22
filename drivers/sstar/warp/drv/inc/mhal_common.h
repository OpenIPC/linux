/*
 * mhal_common.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#ifndef _MHAL_COMMON_H_
#define _MHAL_COMMON_H_

//-------------------------------------------------------------------------------------------------
//  System Data Type
//-------------------------------------------------------------------------------------------------

/// data type unsigned char, data length 1 byte
typedef unsigned char MS_U8; // 1 byte
/// data type unsigned short, data length 2 byte
typedef unsigned short MS_U16; // 2 bytes
/// data type unsigned int, data length 4 byte
typedef unsigned int MS_U32; // 4 bytes
/// data type unsigned int, data length 8 byte
typedef unsigned long long MS_U64; // 8 bytes
/// data type signed char, data length 1 byte
typedef signed char MS_S8; // 1 byte
/// data type signed short, data length 2 byte
typedef signed short MS_S16; // 2 bytes
/// data type signed int, data length 4 byte
typedef signed int MS_S32; // 4 bytes
/// data type signed int, data length 8 byte
typedef signed long long MS_S64; // 8 bytes
/// data type float, data length 4 byte
typedef float MS_FLOAT; // 4 bytes
/// data type hardware physical address
typedef unsigned long long MS_PHYADDR; // 32bit physical address
/// definition for MS_BOOL
typedef unsigned char MS_BOOL;

#ifdef NULL
#undef NULL
#endif
#define NULL 0

//-------------------------------------------------------------------------------------------------
//  Software Data Type
//-------------------------------------------------------------------------------------------------

/// definition for VOID
typedef void VOID;
/// definition for FILEID
typedef MS_S32 FILEID;

typedef MS_U64 MS_PHY;

//#ifndef true
/// definition for true
//#define true                        1
/// definition for false
//#define false                       0
//#endif

#if !defined(TRUE) && !defined(FALSE)
/// definition for TRUE
#define TRUE 1
/// definition for FALSE
#define FALSE 0
#endif

#if defined(ENABLE) && (ENABLE != 1)
#warning ENALBE is not 1
#else
#define ENABLE 1
#endif

#if defined(DISABLE) && (DISABLE != 0)
#warning DISABLE is not 0
#else
#define DISABLE 0
#endif

typedef MS_U16 MHAL_AUDIO_DEV;

//-------------------------------------------------------------------------------------------------
// MHAL Interface Return  Value Define
//-------------------------------------------------------------------------------------------------

#define MHAL_SUCCESS (0)
#define MHAL_FAILURE (-1)
#define MHAL_ERR_ID  (0x80000000L + 0x20000000L)

/******************************************************************************
|----------------------------------------------------------------|
| 1 |   APP_ID   |   MOD_ID    | ERR_LEVEL |   ERR_ID            |
|----------------------------------------------------------------|
|<--><--7bits----><----8bits---><--3bits---><------13bits------->|
******************************************************************************/

#define MHAL_DEF_ERR(module, level, errid) ((MS_S32)((MHAL_ERR_ID) | ((module) << 16) | ((level) << 13) | (errid)))

typedef enum
{
    E_MHAL_ERR_LEVEL_INFO,    /* informational                                */
    E_MHAL_ERR_LEVEL_WARNING, /* warning conditions                           */
    E_MHAL_ERR_LEVEL_ERROR,   /* error conditions                             */
    E_MHAL_ERR_LEVEL_BUTT
} MHAL_ErrLevel_e;

typedef enum
{
    E_MHAL_ERR_INVALID_DEVID = 1, /* invlalid device ID                           */
    E_MHAL_ERR_INVALID_CHNID = 2, /* invlalid channel ID                          */
    E_MHAL_ERR_ILLEGAL_PARAM = 3, /* at lease one parameter is illagal
                                   * eg, an illegal enumeration value             */
    E_MHAL_ERR_EXIST      = 4,    /* resource exists                              */
    E_MHAL_ERR_UNEXIST    = 5,    /* resource unexists                            */
    E_MHAL_ERR_NULL_PTR   = 6,    /* using a NULL point                           */
    E_MHAL_ERR_NOT_CONFIG = 7,    /* try to enable or initialize system, device
                                   ** or channel, before configing attribute       */
    E_MHAL_ERR_NOT_SUPPORT = 8,   /* operation or type is not supported by NOW    */
    E_MHAL_ERR_NOT_PERM    = 9,   /* operation is not permitted
                                   ** eg, try to change static attribute           */
    E_MHAL_ERR_NOMEM        = 12, /* failure caused by malloc memory              */
    E_MHAL_ERR_NOBUF        = 13, /* failure caused by malloc buffer              */
    E_MHAL_ERR_BUF_EMPTY    = 14, /* no data in buffer                            */
    E_MHAL_ERR_BUF_FULL     = 15, /* no buffer for new data                       */
    E_MHAL_ERR_SYS_NOTREADY = 16, /* System is not ready,maybe not initialed or
                                   ** loaded. Returning the error code when opening
                                   ** a device file failed.                        */
    E_MHAL_ERR_BADADDR = 17,      /* bad address,
                                   ** eg. used for copy_from_user & copy_to_user   */
    E_MHAL_ERR_BUSY = 18,         /* resource is busy,
                                   ** eg. destroy a venc chn without unregister it */
    E_MHAL_ERR_BUTT = 63,         /* maxium code, private error code of all modules
                                   ** must be greater than it                      */
} MHAL_ErrCode_e;

#endif // _MS_TYPES_H_
