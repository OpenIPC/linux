/**
    CPU module driver.

    This file is the driver of CPU module.

    @file       Cache.h
    @ingroup    mIDrvSys_Core
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#ifndef _CACHE_H
#define _CACHE_H

#include <stdint.h>
#include <kwrap/nvt_type.h>


/**
    @addtogroup mIDrvSys_Core
*/
//@{

/**
    @name CPU cache line size

    Define CPU I/D cache line size
*/
//@{
#define CPU_ICACHE_LINE_SIZE       (64)    ///< Instruction cache line size (unit: byte)
#define CPU_DCACHE_LINE_SIZE       (64)    ///< Data cache line size (unit: byte)
//@}

//extern ER       cpu_lockDownDCache(UINT32 start, UINT32 end) __attribute__((deprecated("CA53 NOT supports cache lock")));
//extern ER       cpu_lockDownICache(UINT32 start, UINT32 end) __attribute__((deprecated("CA53 NOT supports cache lock")));

#endif

//@}
