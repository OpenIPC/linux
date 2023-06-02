/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/

#ifndef BASE_TYPE_H
#define BASE_TYPE_H
/* INF3: remove include files */
#if 0
#include <stdint.h>
#include <stdio.h>
#ifndef NDEBUG
#include <assert.h>
#endif
#endif

/* INF3: types */
#if 0
typedef int8_t    i8;
typedef uint8_t   u8;
typedef int16_t   i16;
typedef uint16_t  u16;
typedef int32_t   i32;
typedef uint32_t  u32;
typedef int64_t   i64;
typedef uint64_t  u64;
#else
typedef   signed char       i8;
typedef unsigned char       u8;
typedef   signed short      i16;
typedef unsigned short      u16;
typedef   signed int        i32;
typedef unsigned int        u32;
typedef   signed long long  i64;
typedef unsigned long long  u64;
#endif
#define INLINE inline

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else /*  */
#define NULL    ((void *)0)
#endif /*  */
#endif

typedef       short               Short;
typedef       int                 Int;
typedef       unsigned int        UInt;


#ifndef __cplusplus
/*enum {
  false = 0,
  true  = 1
};
typedef _Bool           bool;*/
/* INF3: conflict with kernel bollean
typedef enum
{
  false = 0,
  true  = 1
} bool;
*/
enum
{
  OK  = 0,
  NOK = -1
};
#endif

/* ASSERT */
/* INF3: assert() */
#ifndef NDEBUG
//#define ASSERT(x) assert(x)
#define ASSERT(x)
#else
#define ASSERT(x)
#endif

#endif
