/*
    DCE module driver

    NT98520 DCE internal header file.

    @file       dce_eng_platform.h
    @ingroup    mIIPPDCE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _DCE_ENG_PLATFORM_H_
#define _DCE_ENG_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#define pmc_turnonPower(a)
#define pmc_turnoffPower(a)

//-------------------------------------------------------------------------
#if defined (__LINUX)

#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/export.h>
#include <linux/dma-mapping.h> // header file Dma(cache handle)
#include <linux/slab.h>
#include <linux/interrupt.h>

#include "mach/rcw_macro.h"
#include "mach/nvt-io.h"
#include <mach/fmem.h>
#include <mach/rcw_macro.h>
#include <plat-na51055/nvt-sramctl.h>

#define DCE_ENG_SETREG(ofs, value)   iowrite32(value, (void*)(ofs))
#define DCE_ENG_GETREG(ofs)          ioread32((void*)(ofs))

//=========================================================================
#elif defined (__FREERTOS)

#include "string.h"
#include <stdlib.h>
#include "rcw_macro.h"
#include "io_address.h"
#include "malloc.h"
#include "interrupt.h"
#include "nvt-sramctl.h"
#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
#include "dma_protected.h"

#define DCE_ENG_SETREG(ofs, value)   OUTW((ofs), value)
#define DCE_ENG_GETREG(ofs)          INW(ofs)

#endif

#include "plat/top.h"
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include "kwrap/cpu.h"
#include "kwrap/nvt_type.h"
#include "dce_eng.h"

// interrupt flag
#define FLGPTN_DCE_FRAMESTART   FLGPTN_BIT(0)
#define FLGPTN_DCE_FRAMEEND     FLGPTN_BIT(1)
#define FLGPTN_DCE_CSTPEND      FLGPTN_BIT(2)
#define FLGPTN_DCE_OVFL1        FLGPTN_BIT(3)
#define FLGPTN_DCE_OVFL2        FLGPTN_BIT(4)
#define FLGPTN_DCE_LL_DONE      FLGPTN_BIT(5)
#define FLGPTN_DCE_LL_JOB_END   FLGPTN_BIT(6)

//-------------------------------------------------------------------------
VOID dce_eng_platform_int_enable(VOID);
VOID dce_eng_platform_int_disable(VOID);
VOID dce_eng_platform_enable_clk(DCE_ENG_HANDLE *p_eng);
VOID dce_eng_platform_disable_clk(DCE_ENG_HANDLE *p_eng);
VOID dce_eng_platform_prepare_clk(DCE_ENG_HANDLE *p_eng);
VOID dce_eng_platform_unprepare_clk(DCE_ENG_HANDLE *p_eng);
VOID dce_eng_platform_disable_sram_shutdown(DCE_ENG_HANDLE *p_eng);
VOID dce_eng_platform_enable_sram_shutdown(DCE_ENG_HANDLE *p_eng);
INT32 dce_eng_platform_set_clk_rate(DCE_ENG_HANDLE *p_eng);
UINT32 dce_eng_platform_get_clk_rate(VOID);
UINT32 dce_eng_platform_get_chip_id(VOID);
UINT64 dce_eng_do_64b_div(UINT64 dividend, UINT64 divisor);

VOID dce_eng_platform_create_resource(VOID);
VOID dce_eng_platform_release_resource(VOID);
ER dce_eng_platform_sem_wait(VOID);
ER dce_eng_platform_sem_signal(VOID);
ER dce_eng_platform_flg_clear(FLGPTN flg);
ER dce_eng_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg);
ER dce_eng_platform_flg_set(FLGPTN flg);
VOID dce_eng_platform_request_irq(DCE_ENG_HANDLE *p_eng);
VOID dce_eng_platform_release_irq(DCE_ENG_HANDLE *p_eng);

#if defined (_NVT_EMULATION_)
extern BOOL dce_end_time_out_status;
#endif

void *DCE_ENG_MALLOC(UINT32 size);
void DCE_ENG_FREE(void *ptr);

#ifdef __cplusplus
}
#endif


#endif // _DCE_ENG_PLATFORM_H_


