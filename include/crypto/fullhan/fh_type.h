/******************************************************************************
 Copyright (C), 2001-2011, Fullhan Tech. Co., Ltd.
******************************************************************************
File Name     : fh_type.h
Version       : Initial Draft
Author        : Fullhan multimedia software group
Created       : 2005/4/23
Last Modified :
Description   : Common data types of the system.
Function List :
History       :
******************************************************************************/
#ifndef __FH_TYPE_H__
#define __FH_TYPE_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/* #include "types/type_def.h" */
/*--------------------------------------------------------------------------------------------------------------*
 * Defintion of basic data types. The data types are applicable to both the application layer and kernel codes. *
 *--------------------------------------------------------------------------------------------------------------*/
/*************************** Structure Definition ****************************/
/** \addtogroup      Common_TYPE */
/** @{ */  /** <!-- [Common_TYPE] */

typedef unsigned char           FH_U8;
typedef unsigned char           FH_UCHAR;
typedef unsigned short          FH_U16;
typedef unsigned int            FH_U32;

typedef char                    FH_S8;
typedef short                   FH_S16;
typedef int                     FH_S32;

#ifndef _M_IX86
typedef unsigned long long      FH_U64;
typedef long long               FH_S64;
#else
typedef __int64                 FH_U64;
typedef __int64                 FH_S64;
#endif

typedef char                    FH_CHAR;
typedef char*                   FH_PCHAR;

typedef float                   FH_FLOAT;
typedef double                  FH_DOUBLE;
typedef void                    FH_VOID;

typedef unsigned long           FH_SIZE_T;
typedef unsigned long           FH_LENGTH_T;

typedef FH_U32                  FH_HANDLE;

typedef FH_U32		        FH_PHYS_ADDR_T;
typedef FH_U32                  FH_VIRT_ADDR_T;
/*----------------------------------------------*
 * Constant Definition                          *
 *----------------------------------------------*/
typedef enum {
    FH_FALSE    = 0,
    FH_TRUE     = 1,
} FH_BOOL;

#ifndef NULL
#define NULL             0L
#endif
#define FH_NULL          0L
#define FH_NULL_PTR      0L

#define FH_SUCCESS       0
#define FH_FAILURE       (-1)

#define FH_INVALID_HANDLE (0xffffffff)

#define FH_INVALID_PTS   (0xffffffff)
#define FH_INVALID_TIME   (0xffffffff)

#define FH_OS_LINUX      0xabcd
#define FH_OS_WIN32      0xcdef

#ifdef _WIN32
#define FH_OS_TYPE      FH_OS_WIN32
#else
#define __OS_LINUX__
#define FH_OS_TYPE      FH_OS_LINUX
#endif

#ifdef ADVCA_SUPPORT
#define __INIT__
#define __EXIT__
#else
#define __INIT__  __init
#define __EXIT__  __exit
#endif

/**

define of FH_HANDLE :
bit31                                                           bit0
  |<----   16bit --------->|<---   8bit    --->|<---  8bit   --->|
  |--------------------------------------------------------------|
  |      FH_MOD_ID_E       |  mod defined data |     chnID       |
  |--------------------------------------------------------------|

mod defined data: private data define by each module(for example: sub-mod id), usually, set to 0.
*/

#define FH_HANDLE_MAKEHANDLE(mod, privatedata, chnid)  (FH_HANDLE)( (((mod)& 0xffff) << 16) | ((((privatedata)& 0xff) << 8) ) | (((chnid) & 0xff)) )

#define FH_HANDLE_GET_MODID(handle)    (((handle) >> 16) & 0xffff)
#define FH_HANDLE_GET_PriDATA(handle)  (((handle) >> 8) & 0xff)
#define FH_HANDLE_GET_CHNID(handle)    (((handle)) & 0xff)


#define UNUSED(x) ((x)=(x))



/** @} */  /** <!-- ==== Structure Definition end ==== */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __FH_TYPE_H__ */

