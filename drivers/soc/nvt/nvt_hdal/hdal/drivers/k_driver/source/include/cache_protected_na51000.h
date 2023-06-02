/*
    Novatek protected header file of NT96650's driver.

    The header file for Novatek protected APIs of NT96650's driver.

    @file       cache_protected.h
    @ingroup    mIDriver
    @note       For Novatek internal reference, don't export to agent or customer.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _NVT_CACHE_PROTECTED_H
#define _NVT_CACHE_PROTECTED_H

#include <kwrap/nvt_type.h>
//#include "timer.h"
//#include "cc.h"

#define CA53_DCACHE_SIZE                32768
#define CA53_ICACHE_SIZE                32768
#define CACHE_FLUSHALL_BOUNDARY         (CA53_DCACHE_SIZE<<1)

#define CACHE_SET_NUMBER_SHIFT          5           //64 bytes per set

extern void     cpu_invalidateICacheAll(void);
extern void     cpu_invalidateICache(UINT32 addr);
extern void     cpu_invalidateICacheBlock(UINT32 start, UINT32 end);
extern void     cpu_invalidateDCacheBlock(UINT32 start, UINT32 end);
extern void     cpu_invalidateDCache(UINT32 addr);
extern void     cpu_invalidateDCacheAll(void);
extern void     cpu_cleanDCache(UINT32 addr);
extern void     cpu_cleanDCacheBlock(UINT32 start, UINT32 end);
extern void     cpu_cleanInvalidateDCacheAll(void);
extern void     cpu_cleanInvalidateDCache(UINT32 addr);
extern void     cpu_cleanInvalidateDCacheBlock(UINT32 start, UINT32 end);
extern void     cpu_initCacheConfiguration(void);
extern void     cpu_showCacheConfiguration(void);
#endif

