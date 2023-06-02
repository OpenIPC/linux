/**
    NVT type definition header file
    This file will handle the type define
    @file       nvt-type.h
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#ifndef __ASM_ARCH_NVT_IVOT_NVT_TYPE_H
#define __ASM_ARCH_NVT_IVOT_NVT_TYPE_H

#include <asm/types.h>

typedef int					BOOLEAN;
typedef u64					UINT64;     ///< Unsigned 64 bits data type
typedef s64					INT64;      ///< Signed 64 bits data type
typedef u32					UINT32;     ///< Unsigned 32 bits data type
typedef s32					INT32;      ///< Signed 32 bits data type
typedef u16					UINT16;     ///< Unsigned 16 bits data type
typedef s16					INT16;      ///< Signed 16 bits data type
typedef u8 					UINT8;      ///< Unsigned 8 bits data type
typedef s8 					INT8;       ///< Signed 8 bits data type
/*
typedef unsigned long long	UINT64;
typedef long long         	INT64;
typedef unsigned int      	UINT32;
typedef int               	INT32;
typedef unsigned short    	UINT16;
typedef short             	INT16;
typedef unsigned char     	UINT8;
typedef char              	INT8;
*/
typedef unsigned int		UINT;
typedef int                 INT;
typedef char                CHAR;
typedef unsigned int        BOOL;
typedef int                 ER;             ///< Error code




/**
    @name Assert macros
*/
//@{
#define ASSERT_CONCAT_(a, b)    a##b
#define ASSERT_CONCAT(a, b)     ASSERT_CONCAT_(a, b)

#if defined(__COUNTER__)

#define STATIC_ASSERT(expr) \
enum { ASSERT_CONCAT(FAILED_STATIC_ASSERT_, __COUNTER__) = 1/(expr) }

#else

// This might cause compile error when writing STATIC_ASSERT at the same line
// in two (or more) files and one file include another one.
#define STATIC_ASSERT(expr) \
enum { ASSERT_CONCAT(FAILED_STATIC_ASSERT_, __LINE__) = 1/(expr) }

#endif

/**
    Macro to generate dummy element for enum type to expand enum size to word (4 bytes)
*/
//@{
#define ENUM_DUMMY4WORD(name)   E_##name = 0x10000000
//@}

/**
    @name Align macros

    Align floor, align round, and align ceil

    @note base must be 2^n, where n = 1, 2, 3, ...
*/
//@{
#define ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                   ///< Align Floor
#define ALIGN_ROUND(value, base)  ALIGN_FLOOR((value) + ((base)/2), base)   ///< Align Round
#define ALIGN_CEIL(value, base)   ALIGN_FLOOR((value) + ((base)-1), base)   ///< Align Ceil

#endif /* __ASM_ARCH_NVT_IVOT_NVT_TYPE_H */
