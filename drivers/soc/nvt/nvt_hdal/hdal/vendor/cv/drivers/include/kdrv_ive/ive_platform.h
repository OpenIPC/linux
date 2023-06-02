/*
    IVE module driver

    NT98520 IVE registers header file.

    @file       ive_platform.h
    @ingroup    mCVAIMD
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _IVE_PLATFORM_H
#define _IVE_PLATFORM_H

#include "kwrap/type.h"
#if defined(__FREERTOS)
#include <string.h>
#if defined(_BSP_NA51089_)
#include "rtos_na51089/nvt-sramctl.h"
#include "rtos_na51089/top.h"
#endif
#include "rcw_macro.h"
#include "kwrap/flag.h"
#include "interrupt.h"
#else
#if defined(_BSP_NA51089_)
#include <plat-na51089/nvt-sramctl.h>
#include <plat-na51089/top.h>
#endif //#if defined(__FREERTOS)
#include "mach/rcw_macro.h"
#endif
#include "ive_dbg.h"
#include "kdrv_ive_int.h"

#if defined(__FREERTOS)
extern UINT32 _IVE_REG_BASE_ADDR;
#define IVE_REG_ADDR(ofs)        (_IVE_REG_BASE_ADDR+(ofs))
#define IVE_SETREG(ofs, value)   OUTW((VOID*)(_IVE_REG_BASE_ADDR + ofs),value)
#define IVE_GETREG(ofs)          INW((VOID*)(_IVE_REG_BASE_ADDR + ofs))
#else
extern UINT32 _IVE_REG_BASE_ADDR;
#define IVE_REG_ADDR(ofs)        (_IVE_REG_BASE_ADDR+(ofs))
#define IVE_SETREG(ofs, value)   iowrite32(value, (VOID*)(_IVE_REG_BASE_ADDR + ofs))
#define IVE_GETREG(ofs)          ioread32((VOID*)(_IVE_REG_BASE_ADDR + ofs))
#endif

#define IVE_WORK_FLOW 		   ENABLE
#define IVE_BASE_ADDR          (0xf0d70000)
#define INT_ID_IVE             (53)
#define SEM_MAX_CNT			   (1)
#define IVE_MEASURE_TIME 	   (0)

#if (IVE_WORK_FLOW == ENABLE)
#define IVE_SYS_VFY_EN         DISABLE
#else
#define IVE_SYS_VFY_EN         ENABLE
#endif

#if defined(__FREERTOS)
#define PMODULE_INFO UINT32
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

#define IVE_CLK_VAL_240 240
#define IVE_CLK_VAL_480 480
#define IVE_CLK_VAL_320 320
#define IVE_CLK_VAL_PLL13 270

#define IVE_1M_HZ  (1000000)

// ==================================================================
// Flag
// ==================================================================
#if defined(__FREERTOS)
extern ID  kdrv_ive_flag_id[KDRV_IVE_FLAG_COUNT];
#else
extern UINT32 kdrv_ive_flag_id[KDRV_IVE_FLAG_COUNT];
#endif

#if defined __UITRON || defined __ECOS
extern KDRV_IVE_SEM_TABLE kdrv_ive_semtbl[KDRV_IVE_SEMAPHORE_COUNT];
#elif defined(__FREERTOS)
extern KDRV_IVE_SEM_TABLE kdrv_ive_semtbl[KDRV_IVE_SEMAPHORE_COUNT];
#else
extern KDRV_IVE_SEM_TABLE kdrv_ive_semtbl[KDRV_IVE_SEMAPHORE_COUNT];
#endif

typedef void (* kdrv_ive_frm_end_t)(KDRV_IVE_HANDLE* p_handle, BOOL flag);
typedef void (* kdrv_ive_frm_end_t)(KDRV_IVE_HANDLE* p_handle, BOOL flag);
typedef void (* kdrv_ive_sem_t)(KDRV_IVE_HANDLE* p_handle, BOOL flag);
typedef KDRV_IVE_HANDLE* (* kdrv_ive_entry_id_conv2_handle_t)(UINT32 entry_id);
typedef ER (* kdrv_ive_set_mode_t)(UINT8 id);
typedef void (* kdrv_ive_clear_timeout_flag_t)(KDRV_IVE_HANDLE* p_handle);

extern UINT32 g_uiIveClockRateDefine;

extern ER ive_platform_sem_signal(VOID);
extern ER ive_platform_sem_wait(VOID);
extern ER ive_platform_flg_clear (UINT32 flg);
extern ER ive_platform_flg_set (UINT32 flg);
extern ER ive_platform_flg_wait(PFLGPTN p_flgptn, UINT32 flg);
extern ER ive_platform_flg_rel(VOID);
extern VOID ive_platform_int_enable(VOID);
extern VOID ive_platform_int_disable(VOID);
extern VOID ive_platform_disable_sram_shutdown(VOID);
extern VOID ive_platform_enable_sram_shutdown(VOID);
extern VOID ive_platform_enable_clk(VOID);
extern VOID ive_platform_disable_clk(VOID);
extern VOID ive_platform_prepare_clk(VOID);
extern VOID ive_platform_unprepare_clk(VOID);
extern ER ive_platform_set_clk_rate(UINT32 uiClock);
extern UINT32 ive_platform_get_clk_rate(VOID);
extern UINT32 ive_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size);
extern UINT32 ive_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size);
extern UINT32 ive_platform_dma_flush(UINT32 addr, UINT32 size);
extern UINT32 ive_platform_va2pa(UINT32 addr);
extern VOID ive_platform_create_resource(VOID *parm, UINT32 clk_freq);
extern VOID ive_isr(VOID);
extern ER ive_platform_sem_destroy(VOID);
extern VOID ive_platform_create_resource_config(UINT32 idx);
extern VOID ive_platform_rel_id_config(UINT32 idx);
extern VOID ive_platform_rel_sem_config(UINT32 idx);
#if defined(__FREERTOS)
#define ive_platform_isr_cb (0)
extern UINT32 ive_platform_get_clock_freq_from_dtsi(UINT8 clk_idx);
#else
extern VOID ive_platform_isr_cb(UINT32 intstatus, VOID *ive_trig_hdl);
#endif
extern VOID ive_platform_release_resource(VOID *parm);
extern VOID ive_platform_set_func_frm_end(kdrv_ive_frm_end_t kdrv_ive_frm_end);
extern kdrv_ive_frm_end_t ive_platform_get_func_frm_end(VOID);
extern VOID ive_platform_set_func_sem(kdrv_ive_sem_t kdrv_ive_sem);
extern kdrv_ive_sem_t ive_platform_get_func_sem(VOID);
extern VOID ive_platform_set_timer(INT32 channel, KDRV_IVE_TRIGGER_PARAM *p_ive_param);
extern VOID ive_platform_close_timer(INT32 channel);
extern VOID ive_platform_kdrv_ive_frm_end(KDRV_IVE_HANDLE* p_handle, BOOL flag);
extern VOID ive_platform_interrupt_en(UINT32 is_en);
extern ER ive_platform_enableInt(UINT32 uiIntrEn);
extern UINT32 ive_platform_pa2va_remap(UINT32 pa, UINT32 sz, UINT8 is_mem2dev);
extern VOID ive_platform_pa2va_unmap(UINT32 va, UINT32 pa);
extern UINT32 ive_platform_get_clock_rate(VOID);
extern ER ive_platform_flg_wait_timeout(PFLGPTN p_flgptn, UINT32 flg, UINT32 timeout_ms);

#endif

