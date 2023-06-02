/*
    ISE module driver

    NT98520 ISE internal header file.

    @file       ise_eng_platform.h
    @ingroup    mIIPPISE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _ISE_ENG_INT_PLATFORM_H_
#define _ISE_ENG_INT_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif


//-------------------------------------------------------------------------
#if defined (__LINUX)

#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/export.h>
#include <linux/dma-mapping.h> // header file Dma(cache handle)
#include <linux/slab.h>

#include "mach/rcw_macro.h"
#include "mach/nvt-io.h"
#include <mach/fmem.h>
#include <mach/rcw_macro.h>
#include <plat-na51089/nvt-sramctl.h>

#define ISE_ENG_SETREG(ofs, value)   iowrite32(value, (void*)(ofs))
#define ISE_ENG_GETREG(ofs)          ioread32((void*)(ofs))

#define ISE_ENG_MALLOC(size)	kmalloc((size), GFP_KERNEL)
#define ISE_ENG_FREE(ptr)		kfree(ptr)

//=========================================================================
#elif defined (__FREERTOS)

#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include "rcw_macro.h"
#include "io_address.h"
#include "malloc.h"

#if defined(_BSP_NA51089_)
#include "nvt-sramctl.h"
#endif

#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
#include "dma_protected.h"

#define ISE_ENG_SETREG(ofs, value)   OUTW((ofs), value)
#define ISE_ENG_GETREG(ofs)          INW(ofs)

#define ISE_ENG_MALLOC(size)	malloc((size))
#define ISE_ENG_FREE(ptr)		free(ptr)

#endif

#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include "kwrap/cpu.h"
#include "kwrap/nvt_type.h"
#include "ise_eng_int_dbg.h"
#include "ise_eng.h"


//-------------------------------------------------------------------------
#define ISE_ENG_REG_NUMS  44    // for NT98520

VOID ise_eng_platform_int_enable(VOID);
VOID ise_eng_platform_int_disable(VOID);
VOID ise_eng_platform_enable_clk(ISE_ENG_HANDLE *p_eng);
VOID ise_eng_platform_disable_clk(ISE_ENG_HANDLE *p_eng);
VOID ise_eng_platform_prepare_clk(ISE_ENG_HANDLE *p_eng);
VOID ise_eng_platform_unprepare_clk(ISE_ENG_HANDLE *p_eng);
VOID ise_eng_platform_disable_sram_shutdown(ISE_ENG_HANDLE *p_eng);
VOID ise_eng_platform_enable_sram_shutdown(ISE_ENG_HANDLE *p_eng);
INT32 ise_eng_platform_set_clk_rate(ISE_ENG_HANDLE *p_eng);
UINT32 ise_eng_platform_get_clk_rate(VOID);

#ifdef __cplusplus
}
#endif


#endif // _ISE_ENG_PLATFORM_H_


