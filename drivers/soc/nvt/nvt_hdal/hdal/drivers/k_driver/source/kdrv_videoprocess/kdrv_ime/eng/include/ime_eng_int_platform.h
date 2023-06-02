/*
    IME module driver

    NT98520 IME internal header file.

    @file       ime_eng_int_platform.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _IME_ENG_INT_PLATFORM_H_
#define _IME_ENG_INT_PLATFORM_H_

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

#include "mach/rcw_macro.h"
#include "mach/nvt-io.h"
#include <mach/fmem.h>
#include <mach/rcw_macro.h>
#include <plat-na51055/nvt-sramctl.h>

#include "plat/top.h"
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include "kwrap/cpu.h"
#include "kwrap/nvt_type.h"


extern UINT32 _ime_reg_io_base;


#define _IME_REG_BASE_ADDR      _ime_reg_io_base
#define IME_REG_ADDR(ofs)        (_IME_REG_BASE_ADDR+(ofs))
#define IME_SETREG(ofs, value)   iowrite32(value, (void*)(_IME_REG_BASE_ADDR + ofs))
#define IME_GETREG(ofs)          ioread32((void *)(_IME_REG_BASE_ADDR + ofs))


#define IME_ENG_SETREG(ofs, value)   iowrite32(value, (void*)(ofs))
#define IME_ENG_GETREG(ofs)          ioread32((void*)(ofs))

//=========================================================================
#elif defined (__FREERTOS)

#include "string.h"
#include <stdlib.h>
#include "rcw_macro.h"
#include "io_address.h"
#include "nvt-sramctl.h"
#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
#include "dma_protected.h"

#include "plat/top.h"
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include "kwrap/cpu.h"
#include "kwrap/nvt_type.h"

extern UINT32 _ime_reg_io_base;



#define _IME_REG_BASE_ADDR       _ime_reg_io_base  //(0xF0C40000)
#define IME_REG_ADDR(ofs)        (_IME_REG_BASE_ADDR+(ofs))
#define IME_SETREG(ofs, value)   OUTW((_IME_REG_BASE_ADDR + ofs), value)
#define IME_GETREG(ofs)          INW(_IME_REG_BASE_ADDR + ofs)


#define IME_ENG_SETREG(ofs, value)   OUTW((ofs), value)
#define IME_ENG_GETREG(ofs)          INW(ofs)
#else


extern UINT32 _ime_reg_io_base;

#define _IME_REG_BASE_ADDR       _ime_reg_io_base   //(0xF0C40000)
#define IME_REG_ADDR(ofs)        (_IME_REG_BASE_ADDR+(ofs))
#define IME_SETREG(ofs, value)   OUTW((_IME_REG_BASE_ADDR + ofs), value)
#define IME_GETREG(ofs)          INW(_IME_REG_BASE_ADDR + ofs)


#define IME_ENG_SETREG(ofs, value)   OUTW((ofs), value)
#define IME_ENG_GETREG(ofs)          INW(ofs)

#endif

//-------------------------------------------------------------------------


#include "ime_eng.h"
#include "ime_eng_int_dbg.h"


#define FLGPTN_IME_LL_END           FLGPTN_BIT(0)
#define FLGPTN_IME_LL_ERR           FLGPTN_BIT(1)
#define FLGPTN_IME_LL_RED_LATE      FLGPTN_BIT(2)
#define FLGPTN_IME_LL_JOB_END       FLGPTN_BIT(3)
#define FLGPTN_IME_BP1              FLGPTN_BIT(4)
#define FLGPTN_IME_BP2              FLGPTN_BIT(5)
#define FLGPTN_IME_BP3              FLGPTN_BIT(6)
#define FLGPTN_IME_TMNR_SLICE_END   FLGPTN_BIT(7)
#define FLGPTN_IME_TMNR_MOT_END     FLGPTN_BIT(8)
#define FLGPTN_IME_TMNR_MV_END      FLGPTN_BIT(9)
#define FLGPTN_IME_TMNR_STA_END     FLGPTN_BIT(10)
#define FLGPTN_IME_P1_ENC_OVF       FLGPTN_BIT(11)
#define FLGPTN_IME_TMNR_ENC_OVF     FLGPTN_BIT(12)
#define FLGPTN_IME_TMNR_DEC_ERR     FLGPTN_BIT(13)
#define FLGPTN_IME_FRAME_ERR        FLGPTN_BIT(14)
#define FLGPTN_IME_FRAME_START      FLGPTN_BIT(29)
#define FLGPTN_IME_STRP_END         FLGPTN_BIT(30)
#define FLGPTN_IME_FRAME_END        FLGPTN_BIT(31)



extern ER ime_eng_platform_sem_wait(VOID);
extern ER ime_eng_platform_sem_signal(VOID);

VOID ime_eng_platform_enable_clk(IME_ENG_HANDLE *p_eng);
VOID ime_eng_platform_disable_clk(IME_ENG_HANDLE *p_eng);
VOID ime_eng_platform_prepare_clk(IME_ENG_HANDLE *p_eng);
VOID ime_eng_platform_unprepare_clk(IME_ENG_HANDLE *p_eng);
VOID ime_eng_platform_disable_sram_shutdown(IME_ENG_HANDLE *p_eng);
VOID ime_eng_platform_enable_sram_shutdown(IME_ENG_HANDLE *p_eng);
VOID ime_eng_platform_request_irq(IME_ENG_HANDLE *p_eng);
VOID ime_eng_platform_release_irq(IME_ENG_HANDLE *p_eng);
INT32 ime_eng_platform_set_clk_rate(IME_ENG_HANDLE *p_eng);
UINT32 ime_eng_platform_get_clk_rate(VOID);
UINT32 ime_eng_platform_get_chip_id(VOID);

extern ER ime_eng_platform_flg_clear(FLGPTN flg);

extern ER ime_eng_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg);

extern ER ime_eng_platform_flg_set(FLGPTN flg);

extern VOID ime_eng_platform_create_resource(VOID);

extern VOID ime_eng_platform_release_resource(VOID);

void *IME_ENG_MALLOC(UINT32 size);
void IME_ENG_FREE(void *ptr);


#ifdef __cplusplus
}
#endif


#endif


