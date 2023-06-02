/*
    NUE module driver

    NT98520 NUE registers header file.

    @file       nue_platform.h
    @ingroup    mIIPPNUE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _NUE_PLATFORM_H
#define _NUE_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "kdrv_ai.h"
#if defined(__FREERTOS)
#if defined(_BSP_NA51055_)
#include "rtos_na51055/nvt-sramctl.h"
#endif
#if defined(_BSP_NA51089_)
#include "rtos_na51089/nvt-sramctl.h"
#endif
#include "kwrap/type.h"
#include "kwrap/debug.h"
#include "kwrap/type.h"
#include "kwrap/cpu.h"
#include "kwrap/flag.h"
#include "comm/timer.h"
#include "kwrap/task.h"
#include "pll.h"
#include "pll_protected.h"
#include "interrupt.h"
#else
//#include <linux/module.h>
//#include <linux/kernel.h>
//#include <asm/io.h>
//#include <linux/uaccess.h>
//#include <linux/delay.h>
//#include <linux/timer.h>
#include "kwrap/cpu.h"
//#include "mach/fmem.h"
#include "nue_dbg.h"
#if defined(_BSP_NA51055_)
#include <plat-na51055/nvt-sramctl.h>
#endif
#if defined(_BSP_NA51089_)
#include <plat-na51089/nvt-sramctl.h>
#endif

//
#include "mach/rcw_macro.h"
#endif

#include    "nue_lmt.h"
#include    "nue_lib.h"

#define NUE_AI_FLOW  ENABLE

#if defined(__FREERTOS)
#define EXPORT_SYMBOL(a)
#define MODULE_AUTHOR(a)
#define MODULE_LICENSE(a)
#endif

#if defined(__FREERTOS) //defined __UITRON || defined __ECOS
#define NUE_SETDATA(ofs, value, addr)   OUTW((addr + ofs), value)
#define NUE_GETDATA(ofs, addr)          INW(addr + ofs)
#else
#define NUE_SETDATA(ofs, value, addr)   iowrite32(value, (VOID*)(addr + ofs))
#define NUE_GETDATA(ofs, addr)          ioread32((VOID*)(addr + ofs))
#endif


#if defined(__FREERTOS)
#define PROF                            DISABLE
#define PROF_START()
#define PROF_END(msg)
#else
#define PROF                            DISABLE
#if PROF
#define PROF_START()    do_gettimeofday(&tstart);
#define PROF_END(msg) \
	if (is_setmode == 0) { \
		do_gettimeofday(&tend); \
		printk("%s time (us): %lu\r\n", msg, \
			   (tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec)); \
	}
#else
#define PROF_START()
#define PROF_END(msg)
#endif
#endif //#if defined(__FREERTOS)

#if defined(__FREERTOS)
#define NUE_TEST_INTERRUPT_EN ENABLE
#else
#define NUE_TEST_INTERRUPT_EN DISABLE
#endif
#define INT_ID_NUE  			(14)

#define NUE_VOS_ALIGN_BYTES 64
#define NUE_VOS_IS_ALIGNED(x) (0 == (x & (NUE_VOS_ALIGN_BYTES - 1)))

#define NUE_1M_HZ  1000000

VOID nue_pt_get_engine_idle(VOID);
#if PROF
VOID nue_pt_set_is_setmode(INT32 set_value);
#endif
VOID nue_pt_clk_enable(VOID);
VOID nue_pt_clk_disable(VOID);
VOID nue_pt_create_resource(VOID *parm, UINT32 clk_freq);
VOID nue_pt_rel_flg(VOID);
VOID nue_pt_wai_flg(FLGPTN *uiFlag, UINT32 flag_id);
VOID nue_pt_clr_flg(UINT32 flag_id, BOOL cb_flg);
VOID nue_pt_iset_flg(UINT32 flag_id);
VOID nue_pt_loop_frameend(VOID);
ER nue_pt_set_clock_rate(UINT32 clock);
UINT32 nue_pt_va2pa(UINT32 addr);
VOID nue_pt_request_irq(VOID *isr_handler);
VOID nue_pt_dma_flush_dev2mem(UINT32 addr, UINT32 size);
VOID nue_pt_dma_flush_mem2dev(UINT32 addr, UINT32 size);
VOID nue_pt_dma_flush(UINT32 addr, UINT32 size);
UINT32 nue_pt_get_clock_rate(VOID);
VOID nue_pt_sram_switch(VOID);

#ifdef __cplusplus
}
#endif

#endif
