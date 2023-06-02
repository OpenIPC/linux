/**
    Type declaration

    Type declaration.

    @file       Type.h
    @ingroup    mICOM
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/

#ifndef _TYPE_H
#define _TYPE_H

#ifdef __KERNEL__
#include "nvt_type.h"
#include "platform.h"       //include syntax cross different platform: ARM, or MIPS

#elif defined __ECOS
#include "kwrap/nvt_type.h"
//#include "cyg/compat/uitron/uit_type.h"
#include "kwrap/platform.h"       //include syntax cross different platform: ARM, or MIPS
#else
#include "kwrap/nvt_type.h"
#include "kwrap/platform.h"       //include syntax cross different platform: ARM, or MIPS
#endif

#define __ARM_CORTEX__ 0
/**
    @addtogroup mICOM
*/
//@{
//------------------------------------------------------------------------------
// Typedefs
//------------------------------------------------------------------------------
#ifdef __ECOS
#define VOID void
#else
typedef void VOID;
#endif

#define UBITFIELD       unsigned int    /* Unsigned bit field */
#define BITFIELD        signed int      /* Signed bit field */

// Compatible for Linux kernel type
#ifdef __FREERTOS
#if defined(__LP64__) || defined(_LP64)
typedef UINT64 dma_addr_t;
typedef UINT64 phys_addr_t;
STATIC_ASSERT(sizeof(long) == 8); //make sure LP64 define is correct
#else
typedef UINT32 dma_addr_t;
typedef UINT32 phys_addr_t;
STATIC_ASSERT(sizeof(long) == 4); //make sure LP64 define is correct
#endif
#endif //#ifdef __FREERTOS

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
// NULL
#ifndef NULL
#define NULL                ((void *)0)
#endif

#define NO_TASK_LOCKED      0           ///< The resource is not locked by any task
#define TASK_LOCKED         1           ///< The resource is locked by some task

#define DISABLE             0           ///< Feature is disabled
#define ENABLE              1           ///< Feature is enabled

#define OFF                 0           ///< Feature is OFF
#define ON                  1           ///< Feature is ON

/**
    @name Align (round off)

    Round Off to 32, 16, 8 and 4

    @note Example: ALIGN_ROUND_32(32) = 32, ALIGN_ROUND_32(47) = 32, ALIGN_ROUND_32(48) = 64
*/
//@{
#define ALIGN_ROUND_64(a)       ALIGN_ROUND(a, 64)  ///< Round Off to 64
#define ALIGN_ROUND_32(a)       ALIGN_ROUND(a, 32)  ///< Round Off to 32
#define ALIGN_ROUND_16(a)       ALIGN_ROUND(a, 16)  ///< Round Off to 16
#define ALIGN_ROUND_8(a)        ALIGN_ROUND(a, 8)   ///< Round Off to 8
#define ALIGN_ROUND_4(a)        ALIGN_ROUND(a, 4)   ///< Round Off to 4
//@}

/**
    @name Align (round up)

    Round Up to 32, 16, 8 and 4.

    @note Example: ALIGN_CEIL_32(32) = 32, ALIGN_CEIL_32(33) = 64, ALIGN_CEIL_32(63) = 64
*/
//@{
#define ALIGN_CEIL_64(a)        ALIGN_CEIL(a, 64)   ///< Round Up to 64
#define ALIGN_CEIL_32(a)        ALIGN_CEIL(a, 32)   ///< Round Up to 32
#define ALIGN_CEIL_16(a)        ALIGN_CEIL(a, 16)   ///< Round Up to 16
#define ALIGN_CEIL_8(a)         ALIGN_CEIL(a, 8)    ///< Round Up to 8
#define ALIGN_CEIL_4(a)         ALIGN_CEIL(a, 4)    ///< Round Up to 4
//@}

/**
    @name Align (round down)

    Round Down to 32, 16, 8 and 4.

    @note Example: ALIGN_FLOOR_32(32) = 32, ALIGN_FLOOR_32(33) = 32, ALIGN_FLOOR_32(63) = 32
*/
//@{
#define ALIGN_FLOOR_64(a)       ALIGN_FLOOR(a, 64)  ///< Round down to 64
#define ALIGN_FLOOR_32(a)       ALIGN_FLOOR(a, 32)  ///< Round down to 32
#define ALIGN_FLOOR_16(a)       ALIGN_FLOOR(a, 16)  ///< Round down to 16
#define ALIGN_FLOOR_8(a)        ALIGN_FLOOR(a, 8)   ///< Round down to 8
#define ALIGN_FLOOR_4(a)        ALIGN_FLOOR(a, 4)   ///< Round down to 4
//@}

/**
    Convert four characters into a four-character code.
*/
//@{
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((UINT32)(UINT8)(ch0) | ((UINT32)(UINT8)(ch1) << 8) | ((UINT32)(UINT8)(ch2) << 16) | ((UINT32)(UINT8)(ch3) << 24))   ///<
//@}

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
//@}

//------------------------------------------------------------------------------
// Data Structures
//------------------------------------------------------------------------------
/**
    Structure for point

    Structure for point.
*/
typedef struct {
	UINT32  x;      ///< X coordinate of the point
	UINT32  y;      ///< Y coordinate of the point
} UPOINT, *PUPOINT;

/**
    Structure for rectangle size

    Structure for rectangle size.
*/
typedef struct {
	UINT32  w;      ///< Rectangle width
	UINT32  h;      ///< Rectangle height
} USIZE, *PUSIZE;

/**
    Structure for rectangle

    Structure for rectangle.
*/
typedef struct {
	UINT32  x;      ///< X coordinate of the top-left point of the rectangle
	UINT32  y;      ///< Y coordinate of the top-left point of the rectangle
	UINT32  w;      ///< Rectangle width
	UINT32  h;      ///< Rectangle height
} URECT, *PURECT;

/**
    Structure for point

    Structure for point.
*/
typedef struct {
	INT32  x;       ///< X coordinate of the point
	INT32  y;       ///< Y coordinate of the point
} IPOINT, *PIPOINT;

/**
    Structure for rectangle size

    Structure for rectangle size.
*/
typedef struct {
	INT32  w;       ///< Rectangle width
	INT32  h;       ///< Rectangle height
} ISIZE, *PISIZE;

/**
    Structure for rectangle

    Structure for rectangle.
*/
typedef struct {
	INT32  x;       ///< X coordinate of the top-left point of the rectangle
	INT32  y;       ///< Y coordinate of the top-left point of the rectangle
	INT32  w;       ///< Rectangle width
	INT32  h;       ///< Rectangle height
} IRECT, *PIRECT;

/**
    Structure for memory range

    Structure for memory range.
*/
typedef struct _MEM_RANGE {
	UINT32 addr;    ///< Memory buffer starting address
	UINT32 size;    ///< Memory buffer size
} MEM_RANGE, *PMEM_RANGE;
//@}

#endif
