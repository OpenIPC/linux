/*
    MDBC module driver

    NT98520 MDBC registers header file.

    @file       md_platform.h
    @ingroup    mCVAIMD
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _MD_PLATFORM_H
#define _MD_PLATFORM_H

#if defined(__FREERTOS)
#include "rcw_macro.h"
#if defined(_BSP_NA51089_)
#include "nvt-sramctl.h"
#include "top.h"
#endif
#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
#include <kwrap/nvt_type.h>
#include "comm/timer.h"
#include <string.h>

#define _MDBC_REG_BASE_ADDR       0xf0c10000
#define MDBC_REG_ADDR(ofs)        (_MDBC_REG_BASE_ADDR+(ofs))
#define MDBC_SETREG(ofs, value)   OUTW((void*)(_MDBC_REG_BASE_ADDR + ofs),value)
#define MDBC_GETREG(ofs)          INW((void*)(_MDBC_REG_BASE_ADDR + ofs))
#else
/*
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/random.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
//#include <mach/rcw_macro.h>
#include <mach/fmem.h>
#include <plat-na51055/nvt-sramctl.h>
#include <plat-na51055/top.h>
#include <asm/io.h>
#include <asm/div64.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/timer.h>
*/

#include <plat-na51055/nvt-sramctl.h>
#include <plat-na51055/top.h>
#include "mach/rcw_macro.h"

//#include "md_dbg.h"
#include "md_drv.h"
#include "mdbc_int.h"
extern UINT32 _MDBC_REG_BASE_ADDR;
#define MDBC_REG_ADDR(ofs)        (_MDBC_REG_BASE_ADDR+(ofs))
#define MDBC_SETREG(ofs, value)   iowrite32(value, (void*)(_MDBC_REG_BASE_ADDR + ofs))
#define MDBC_GETREG(ofs)          ioread32((void*)(_MDBC_REG_BASE_ADDR + ofs))
#endif

#include "kwrap/debug.h"
#include "kdrv_type.h"
#include "kwrap/type.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include <kwrap/cpu.h>
#include "kwrap/semaphore.h"
#include <kwrap/spinlock.h>

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
#define MDBC_SETDATA(ofs, value, addr)   OUTW((addr + ofs), value)
#define MDBC_GETDATA(ofs, addr)          INW(addr + ofs)
#else
#define MDBC_SETDATA(ofs, value, addr)   iowrite32(value, (VOID*)(addr + ofs))
#define MDBC_GETDATA(ofs, addr)          ioread32((VOID*)(addr + ofs))
#endif

#define MDBC_1M_HZ  1000000

#define FLGPTN_MDBC_FRAMEEND     FLGPTN_BIT(0)
#define FLGPTN_MDBC_LLEND        FLGPTN_BIT(4)
#define FLGPTN_MDBC_LLERROR      FLGPTN_BIT(5)

extern ER md_platform_sem_signal(void);
extern ER md_platform_sem_wait(void);
extern ER md_platform_flg_clear (FLGPTN flg);
extern ER md_platform_flg_set (FLGPTN flg);
extern ER md_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg);
extern ER md_platform_flg_rel(void);

extern void md_platform_int_enable(void);
extern void md_platform_int_disable(void);

extern void md_platform_disable_sram_shutdown(void);
extern void md_platform_enable_sram_shutdown(void);

extern void md_platform_enable_clk(void);
extern void md_platform_disable_clk(void);
extern void md_platform_prepare_clk(void);
extern void md_platform_unprepare_clk(void);
extern ER md_platform_set_clk_rate(UINT32 uiClock);
extern UINT32 md_platform_get_clk_rate(void);

UINT32 md_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size);
UINT32 md_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size);
UINT32 md_platform_dma_flush(UINT32 addr, UINT32 size);
UINT32 md_platform_va2pa(UINT32 addr);

#if !(defined __UITRON || defined __ECOS)
extern void mdbc_isr(void);
#if defined __FREERTOS
extern void md_platform_create_resource(UINT32 clk_freq);
#else
extern void md_platform_create_resource(MD_MODULE_INFO *pmodule_info, UINT32 clk_freq);
#endif
extern void md_platform_release_resource(void);
#endif

#endif

