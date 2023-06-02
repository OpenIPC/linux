#ifndef __MODULE_PLATFORM_H_
#define __MODULE_PLATFORM_H__

#include "comm/ddr_arb.h"
#if (defined __UITRON || defined __ECOS)
#include <mach/fmem.h>
#elif defined(__FREERTOS)
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/spinlock.h>
#include <kwrap/nvt_type.h>
#include <kwrap/task.h>
#include <string.h>
#include "rcw_macro.h"
#include "io_address.h"
#include "dma_protected.h"
#include "cache_protected.h"
#include "pll_protected.h"
#include "interrupt.h"
#include "top.h"
#define _EMULATION_             (0)	// no emu macro in linux
					// manually defined here
#define _FPGA_EMULATION_	(0)
#define _EMULATION_ON_CPU2_	(0)
#define DRV_SUPPORT_IST		(1)

#define DDR_ARB_BOTTOMHALF_KTHREAD	(0)
#define DDR_ARB_BOTTOMHALF_TASKLET	(1)
#define DDR_ARB_BOTTOMHALF_SEL		(DDR_ARB_BOTTOMHALF_TASKLET)

#define DMA_PRI_BIT_MASK        (0x03)
#define MY_PANIC DBG_ISR

#else
#include <plat/top.h>
#include <linux/vmalloc.h>
#include "ddr_arb_drv.h"
#define _EMULATION_             (0)	// no emu macro in linux
					// manually defined here
#define _FPGA_EMULATION_	(0)
#define _EMULATION_ON_CPU2_	(0)
#define DRV_SUPPORT_IST		(1)

#define DDR_ARB_BOTTOMHALF_KTHREAD	(0)
#define DDR_ARB_BOTTOMHALF_TASKLET	(1)
#define DDR_ARB_BOTTOMHALF_SEL		(DDR_ARB_BOTTOMHALF_TASKLET)

#define DMA_PRI_BIT_MASK        (0x03)
#define DMA_WPCH_COUNT	159

#define FLGPTN_DDR_ARB0  0x01
//#define FLGPTN_GRAPHIC2 0x02
#define MY_PANIC panic


//extern UINT32 _REGIOBASE;
extern UINT32 IOADDR_DDR_ARB_REG_BASE;

extern UINT32 IOADDR_DDR_ARB_PROT_REG_BASE;

//THREAD_HANDLE m_ddr_arb_tsk_id = 0;	// arb ist
extern volatile DRV_CB          pf_ist_cbs_ddr_arb[DDR_ARB_COUNT];
#endif
#include <kwrap/cpu.h>

#define __MODULE__    rtos_mau
#define __DBGLVL__ NVT_DBG_USER
#include <kwrap/debug.h>
extern unsigned int rtos_mau_debug_level;


extern void ddr_arb_platform_clk_enable(void);
extern void ddr_arb_platform_clk_disable(void);
extern void ddr_arb_platform_clk_set_freq(UINT32 freq);
extern void ddr_arb_platform_clk_get_freq(UINT32 *p_freq);
extern void ddr_arb_platform_flg_clear(FLGPTN flg);
extern void ddr_arb_platform_flg_set(FLGPTN flg);
extern void ddr_arb_platform_flg_wait(FLGPTN flg);
extern FLGPTN ddr_arb_platform_flg_check(FLGPTN chkptn);
extern ER ddr_arb_platform_sem_wait(void);
extern ER ddr_arb_platform_sem_signal(void);
extern UINT32 ddr_arb_platform_spin_lock(void);
extern void ddr_arb_platform_spin_unlock(UINT32 flags);
extern void ddr_arb_platform_sram_enable(void);
extern void ddr_arb_platform_int_enable(void);
extern void ddr_arb_platform_int_disable(void);
extern UINT32 ddr_arb_platform_dma_is_cacheable(UINT32 addr);
extern UINT32 ddr_arb_platform_dma_flush_dev2mem_width_neq_loff(UINT32 addr, UINT32 size);
extern UINT32 ddr_arb_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size);
extern UINT32 ddr_arb_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size);
extern BOOL ddr_arb_platform_is_valid_va(UINT32 addr);
extern UINT32 ddr_arb_platform_va2pa(UINT32 addr);
extern void ddr_arb_platform_delay_ms(UINT32 ms);
#if !(defined __UITRON || defined __ECOS)
extern void arb_isr(void);
#if defined __FREERTOS
extern void ddr_arb_platform_create_resource(void);
#else
//extern int DBG_ERR(const char *fmt, ...);
//extern int DBG_WRN(const char *fmt, ...);
//extern int DBG_IND(const char *fmt, ...);
extern void ddr_arb_platform_set_ist_event(UINT32 events);
#if (DDR_ARB_BOTTOMHALF_SEL == DDR_ARB_BOTTOMHALF_TASKLET)
extern int ddr_arb_platform_ist(UINT32 event);
#endif

extern void ddr_arb_platform_create_resource(MODULE_INFO *pmodule_info);
extern void ddr_arb_platform_release_resource(void);
#endif
#endif
#endif
