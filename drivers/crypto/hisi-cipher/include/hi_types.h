/*
 * Copyright (c) 2018 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __HI_TYPES_H__
#define __HI_TYPES_H__

#include <linux/types.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/*--------------------------------------------------------------------------------------------------------------*
 * Defintion of basic data types. The data types are applicable to both the application layer and kernel codes. *
 *--------------------------------------------------------------------------------------------------------------*/
/*************************** Structure Definition ****************************/
/** \addtogroup      Common_TYPE */
/** @{ */  /** <!-- [Common_TYPE] */

typedef unsigned char           HI_U8;
typedef unsigned char           HI_UCHAR;
typedef unsigned short          HI_U16;
typedef unsigned int            HI_U32;
typedef unsigned long           HI_ULONG;

typedef signed char             HI_S8;
typedef short                   HI_S16;
typedef int                     HI_S32;
typedef long                    HI_SLONG;

#ifndef _M_IX86
typedef unsigned long long      HI_U64;
typedef long long               HI_S64;
#else
typedef __int64                 HI_U64;
typedef __int64                 HI_S64;
#endif

typedef char                    HI_CHAR;
typedef char*                   HI_PCHAR;

typedef unsigned long           HI_UL;
typedef signed long             HI_SL;

typedef float                   HI_FLOAT;
typedef double                  HI_DOUBLE;

typedef void                    HI_VOID;

typedef unsigned long           HI_SIZE_T;
typedef unsigned long           HI_LENGTH_T;

typedef HI_U32                  HI_HANDLE;
typedef uintptr_t               HI_UINTPTR_T;

/** Constant Definition */
typedef enum
{
    HI_FALSE    = 0,
    HI_TRUE     = 1,
} HI_BOOL;

#ifndef NULL
#define NULL                0L
#endif

#define HI_NULL             0L
#define HI_NULL_PTR         0L

#define HI_SUCCESS          0
#define HI_FAILURE          (-1)
#define HI_INVALID_HANDLE   (0xffffffff)

typedef unsigned char           hi_uchar;
typedef unsigned char           hi_u8;
typedef unsigned short          hi_u16;
typedef unsigned int            hi_u32;
typedef unsigned long long      hi_u64;
typedef unsigned long           hi_ulong;

typedef char                    hi_char;
typedef signed char             hi_s8;
typedef short                   hi_s16;
typedef int                     hi_s32;
typedef long long               hi_s64;
typedef long                    hi_slong;

typedef float                   hi_float;
typedef double                  hi_double;

typedef void                    hi_void;

typedef unsigned long           hi_size_t;
typedef unsigned long           hi_length_t;

typedef hi_u32                  hi_handle;

typedef HI_BOOL                 hi_bool;
typedef HI_UINTPTR_T            hi_uintptr_t;

/**

define of HI_HANDLE :
bit31                                                           bit0
  |<----   16bit --------->|<---   8bit    --->|<---  8bit   --->|
  |--------------------------------------------------------------|
  |      HI_MOD_ID_E       |  mod defined data |     chnID       |
  |--------------------------------------------------------------|

mod defined data: private data define by each module(for example: sub-mod id), usually, set to 0.
*/

#define HI_HANDLE_MAKEHANDLE(mod, privatedata, chnid)  (hi_handle)( (((mod)& 0xffff) << 16) | ((((privatedata)& 0xff) << 8) ) | (((chnid) & 0xff)) )

#define HI_HANDLE_GET_MODID(handle)     (((handle) >> 16) & 0xffff)
#define HI_HANDLE_GET_PriDATA(handle)   (((handle) >> 8) & 0xff)
#define HI_HANDLE_GET_CHNID(handle)     (((handle)) & 0xff)


/** @} */  /** <!-- ==== Structure Definition end ==== */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __HI_TYPE_H__ */

