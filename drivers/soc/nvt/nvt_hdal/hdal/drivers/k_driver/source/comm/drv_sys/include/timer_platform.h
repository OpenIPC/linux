#ifndef __MODULE_PLATFORM_H_
#define __MODULE_PLATFORM_H__
//#include "graphic_main.h"

#include "comm/timer.h"
#if (defined __UITRON || defined __ECOS)
#include "frammap/frammap_if.h"

#else

#include "timer_drv.h"

#ifdef _NVT_EMULATION_
#define _EMULATION_             ENABLE
#else
#define _EMULATION_             DISABLE
#endif

#if defined(_NVT_FPGA_) || defined(CONFIG_NVT_FPGA_EMULATION)
#ifndef _FPGA_PLL_OSC_
#define _FPGA_PLL_OSC_  24000000
#endif
#define _FPGA_EMULATION_        ENABLE
#else
#define _FPGA_EMULATION_        DISABLE
#endif

#define _EMULATION_MULTI_CPU_   DISABLE

#define _EMULATION_ON_CPU2_	(0)
#define DRV_SUPPORT_IST		(1)

#define TIMER_BOTTOMHALF_KTHREAD	(0)
#define TIMER_BOTTOMHALF_TASKLET	(1)
#if defined __FREERTOS
#define TIMER_BOTTOMHALF_SEL		(TIMER_BOTTOMHALF_TASKLET)
#else
#define TIMER_BOTTOMHALF_SEL		(TIMER_BOTTOMHALF_KTHREAD)
#endif

#define FLGPTN_TIMER0  0x01
//#define FLGPTN_GRAPHIC2 0x02
#if !defined __FREERTOS
extern UINT32 _REGIOBASE;
extern UINT32 IOADDR_TIMER_REG_BASE;
#else
#include "plat/interrupt.h"
#include "plat/pll.h"
#endif
//extern UINT32 IOADDR_GRAPHIC2_REG_BASE;

//THREAD_HANDLE m_timer_tsk_id = 0;	// timer ist
extern volatile DRV_CB          pf_ist_cbs_timer[TIMER_NUM];
#endif

extern void timer_platform_clk_enable(void);
extern void timer_platform_clk_disable(void);
extern void timer_platform_clk_set_freq(UINT32 freq);
extern void timer_platform_clk_get_freq(UINT32 *p_freq);
extern void timer_platform_flg_clear(FLGPTN flg);
extern void timer_platform_flg_set(FLGPTN flg);
extern void timer_platform_flg_wait(FLGPTN flg);
extern FLGPTN timer_platform_flg_check(FLGPTN chkptn);
extern ER timer_platform_sem_wait(void);
extern ER timer_platform_sem_signal(void);
extern UINT32 timer_platform_spin_lock(void);
extern void timer_platform_spin_unlock(UINT32 flags);
extern void timer_platform_sram_enable(void);
extern void timer_platform_int_enable(void);
extern void timer_platform_int_disable(void);
extern UINT32 timer_platform_dma_is_cacheable(UINT32 addr);
extern UINT32 timer_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size);
extern UINT32 timer_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size);
extern UINT32 timer_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size);
extern BOOL timer_platform_is_valid_va(UINT32 addr);
extern UINT32 timer_platform_va2pa(UINT32 addr);
extern void timer_platform_set_ist_event(UINT32 events);
#if !(defined __UITRON || defined __ECOS)
//extern int DBG_ERR(const char *fmt, ...);
//extern int DBG_WRN(const char *fmt, ...);
//extern int DBG_IND(const char *fmt, ...);
extern int timer_isr(void);
//extern void graph2_isr(void);
#if (TIMER_BOTTOMHALF_SEL == TIMER_BOTTOMHALF_TASKLET)
extern int timer_platform_ist(UINT32 event);
#endif
#endif

extern void timer_open_sys_timer(void);

#if defined __FREERTOS
extern void timer_platform_create_resource(void);
#else
extern void timer_platform_create_resource(MODULE_INFO *pmodule_info);
#endif
extern void timer_platform_release_resource(void);

#ifdef CONFIG_NOVATEK_TIMER
#include <linux/interrupt.h>
#define MAX_TIMER_NUM 20
extern void __iomem *nvttmr_base;
extern spinlock_t nvttmr_lock;
extern u32 nvttmr_in_use;
#endif

#endif
