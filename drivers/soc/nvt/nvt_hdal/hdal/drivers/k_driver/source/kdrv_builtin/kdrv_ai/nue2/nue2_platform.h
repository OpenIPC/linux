/*
    NUE2 module driver

    NT98520 NUE2 registers header file.

    @file       nue2_platform.h
    @ingroup    mIIPPNUE2
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _NUE2_PLATFORM_H
#define _NUE2_PLATFORM_H

#include "kwrap/type.h"
#include "kdrv_ai.h"
#if defined(__FREERTOS)
#if defined(_BSP_NA51055_)
#include "rtos_na51055/nvt-sramctl.h"
#include "rtos_na51055/top.h"
#endif
#if defined(_BSP_NA51089_)
#include "rtos_na51089/nvt-sramctl.h"
#include "rtos_na51089/top.h"
#endif
#include "rcw_macro.h"
#include "kwrap/flag.h"
#include "interrupt.h"
#else
#if defined(_BSP_NA51055_)
#include <plat-na51055/nvt-sramctl.h>
#include <plat-na51055/top.h>
#endif //#if defined(__FREERTOS)
#if defined(_BSP_NA51089_)
#include <plat-na51089/nvt-sramctl.h>
#include <plat-na51089/top.h>
#endif
#include "mach/rcw_macro.h"
#endif
#include "nue2_dbg.h"

#define NUE2_AI_FLOW  ENABLE

#if (NUE2_AI_FLOW == ENABLE)
#define NUE2_SYS_VFY_EN DISABLE
#else
#define NUE2_SYS_VFY_EN ENABLE
#endif

#if defined(__FREERTOS)
#define EXPORT_SYMBOL(a)
#define MODULE_AUTHOR(a)
#define MODULE_LICENSE(a)
#endif

#if defined(__FREERTOS)
#define PROF                            DISABLE
#define PROF_START()
#define PROF_END(msg)
#else
#define PROF                            DISABLE
#if PROF
static struct timeval tstart, tend;
static INT32 is_setmode = 0;
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
#define NUE2_SETDATA(ofs, value, addr)   OUTW((addr + ofs), value)
#define NUE2_GETDATA(ofs, addr)          INW(addr + ofs)
#else
#define NUE2_SETDATA(ofs, value, addr)   iowrite32(value, (VOID*)(addr + ofs))
#define NUE2_GETDATA(ofs, addr)          ioread32((VOID*)(addr + ofs))
#endif

#define NUE2_1M_HZ  1000000

#define INT_ID_NUE2  			(13)

VOID nue2_pt_request_irq(VOID *isr_handler);
ER nue2_pt_set_clock_rate(UINT32 clock);
VOID nue2_pt_clk_disable(VOID);
VOID nue2_pt_clk_enable(VOID);
VOID nue2_pt_rel_flg(VOID);
VOID nue2_pt_create_resource(VOID *parm, UINT32 clk_freq);
VOID nue2_pt_wai_flg(FLGPTN *uiFlag, UINT32 flag_id);
VOID nue2_pt_clr_flg(UINT32 flag_id, BOOL cb_flg);
VOID nue2_pt_iset_flg(UINT32 flag_id);
UINT32 nue2_pt_va2pa(UINT32 addr);
VOID nue2_pt_ch_enable(VOID);
VOID nue2_pt_ch_disable(VOID);
VOID nue2_pt_dma_test(UINT8 is_en);
UINT32 nue2_pt_get_clock_rate(VOID);
VOID nue2_pt_get_engine_idle(VOID);

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
VOID nue2_pt_dma_flush_dev2mem(UINT32 addr, UINT32 size);
VOID nue2_pt_dma_flush_mem2dev(UINT32 addr, UINT32 size);
VOID nue2_pt_dma_flush(UINT32 addr, UINT32 size);
#endif

#if (NUE2_SYS_VFY_EN == ENABLE)
UINT32 nue2_pt_pa2va_remap(UINT32 pa, UINT32 sz, UINT8 is_mem2dev);
VOID nue2_pt_pa2va_unmap(UINT32 va, UINT32 pa);
#endif

#endif
