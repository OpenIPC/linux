/*
    CNN module driver

    NT98520 CNN registers header file.

    @file       cnn_platform.h
    @ingroup    mIIPPCNN
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _CNN_PLATFORM_H
#define _CNN_PLATFORM_H

#include "kdrv_ai.h"
#if defined(__FREERTOS)
#include "rcw_macro.h"
#if defined(_BSP_NA51055_)
#include "rtos_na51055/nvt-sramctl.h"
#include "rtos_na51055/top.h"
#endif
#if defined(_BSP_NA51089_)
#include "rtos_na51089/nvt-sramctl.h"
#include "rtos_na51089/top.h"
#endif
#include "kwrap/debug.h"
#include "kwrap/task.h"
#include "pll.h"
#include "pll_protected.h"
#include "comm/timer.h"
#include "kwrap/type.h"
#include "interrupt.h"

#else
#include "mach/rcw_macro.h"
#include <plat/nvt-sramctl.h>
#include <plat/top.h>
#include "cnn_dbg.h"
#include <linux/clk.h>
#endif

#define CNN_1M_HZ  1000000

#include "cnn_lmt.h"
#include "cnn_lib.h"
#include "kwrap/cpu.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "comm/ddr_arb.h"

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


ER cnn_pt_set_clock_rate(BOOL cnn_id, UINT32 clock);
UINT32 cnn_pt_get_clock_rate(BOOL cnn_id);

VOID cnn_pt_clk_disable(BOOL cnn_id);
VOID cnn_pt_clk_enable(BOOL cnn_id);
VOID cnn_pt_rel_flg(BOOL cnn_id);

#if defined(__FREERTOS)
irqreturn_t cnn_platform_isr1(int irq, void *devid);
irqreturn_t cnn_platform_isr2(int irq, void *devid);
void cnn_pt_create_resource(BOOL cnn_id, UINT32 clk_freq);
#else
VOID cnn_pt_create_resource(BOOL cnn_id, VOID *parm, UINT32 clk_freq);
#endif
VOID cnn_pt_release_resource(BOOL cnn_id);
VOID cnn_pt_wai_flg(BOOL cnn_id, FLGPTN *uiFlag, UINT32 flag_id);
VOID cnn_pt_clr_flg(BOOL cnn_id, UINT32 flag_id, BOOL cb_flg);
VOID cnn_pt_iset_flg(BOOL cnn_id, UINT32 flag_id);
UINT32 cnn_pt_va2pa(UINT32 addr);
VOID cnn_pt_dma_flush_dev2mem(UINT32 addr, UINT32 size);
VOID cnn_pt_dma_flush_mem2dev(UINT32 addr, UINT32 size);
VOID cnn_pt_dma_flush(UINT32 addr, UINT32 size);
ER cnn_pt_sem_wait(BOOL cnn_id);
VOID cnn_pt_sem_signal(BOOL cnn_id);
ER cnn_pt_lib_sem_wait(BOOL cnn_id);
VOID cnn_pt_lib_sem_signal(BOOL cnn_id);
VOID cnn_pt_sram_switch(BOOL cnn_id);

#endif
