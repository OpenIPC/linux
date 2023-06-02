#ifndef __SIE_PLATFORM_H_
#define __SIE_PLATFORM_H__

#if defined(__FREERTOS)
#include "string.h"
#include <stdlib.h>
#include "rcw_macro.h"
#include "io_address.h"
#define EXPORT_SYMBOL(a)
#if defined(_BSP_NA51089_)
#include "nvt-sramctl.h"
#endif
#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
#include "dma_protected.h"
#define SIE_REG_ADDR(addr,ofs)        (addr+(ofs))
#define SIE_SETREG(addr,ofs, value)   OUTW((addr + ofs), (value))
#define SIE_GETREG(addr,ofs)          INW(addr + ofs)
#define MODULE_CLK_NUM          3
#else
#include <linux/slab.h>
#include <plat/nvt-sramctl.h>
#include <linux/clk-provider.h>
#include <mach/fmem.h>
#include "sie_drv.h"
#define SIE_REG_ADDR(addr,ofs)        (addr+(ofs))
#define SIE_SETREG(addr,ofs, value)   sie_platform_set_reg(addr, ofs, value)
#define SIE_GETREG(addr,ofs)          ioread32((void*)(addr + ofs))
#endif
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include "kwrap/util.h"
#include "kwrap/cpu.h"
#include "siep_lib.h"
#include "kwrap/debug.h"

#define PLL5_DRV_MAX_FREQ       399000000
#define PLL10_DRV_MAX_FREQ      399000000
#define PLL12_DRV_MAX_FREQ      399000000
#define FREQ_480M           	480000000
#define FREQ_320M           	320000000
#define FREQ_192M           	192000000
#define SIE_MAX_CLK_FREQ        320000000
#define IGNORE_SIE_DRV_MIN_FREQ 0x95279527

#if (defined(_NVT_EMULATION_) || defined(_NVT_FPGA_) || defined(CONFIG_NVT_FPGA_EMULATION))
#define SIE_DRVWAIT_FLG_TIMEOUT_MS 	1000000000
#else
#define SIE_DRVWAIT_FLG_TIMEOUT_MS 	100
#endif

extern UINT32 _SIE_REG_BASE_ADDR_SET[MODULE_CLK_NUM];


#define FLGPTN_SIE_FRAMEEND     FLGPTN_BIT(0)
#define FLGPTN_SIE_BP1          FLGPTN_BIT(1)
#define FLGPTN_SIE_BP2          FLGPTN_BIT(2)
#define FLGPTN_SIE_BP3          FLGPTN_BIT(3)
#define FLGPTN_SIE_ACTST        FLGPTN_BIT(4)
#define FLGPTN_SIE_CRPST        FLGPTN_BIT(5)
#define FLGPTN_SIE_DO0_END      FLGPTN_BIT(6)
#define FLGPTN_SIE_DO1_END      FLGPTN_BIT(7)
#define FLGPTN_SIE_DO2_END      FLGPTN_BIT(8)
#define FLGPTN_SIE_DO3_END      FLGPTN_BIT(9)
#define FLGPTN_SIE_DO4_END      FLGPTN_BIT(10)
#define FLGPTN_SIE_DO5_END      FLGPTN_BIT(11)
#define FLGPTN_SIE_ACTEND       FLGPTN_BIT(12)
#define FLGPTN_SIE_CRPEND       FLGPTN_BIT(13)
#define FLGPTN_SIE_MSHTR        FLGPTN_BIT(14)
#define FLGPTN_SIE_MD_HIT       FLGPTN_BIT(15)

#define FLGPTN_SIE_CRPEND_VDLATISR       FLGPTN_BIT(31) // crop end sts after sie_reg_vdlatch_isr_cb

extern void sie_platform_set_clk_rate(SIE_ENGINE_ID id, SIE_OPENOBJ *pObjCB);
extern ER sie_setPxClock(SIE_ENGINE_ID id, SIE_PXCLKSRC PxClkSel);
extern ER sie1_setPxClock(SIE_PXCLKSRC PxClkSel);
extern ER sie2_setPxClock(SIE_PXCLKSRC PxClkSel);
extern ER sie3_setPxClock(SIE_PXCLKSRC PxClkSel);
extern ER sie_setClock(SIE_ENGINE_ID id, SIE_CLKSRC_SEL ClkSrc, UINT32 uiClkRate);
extern ER sie_getClock(SIE_ENGINE_ID id, SIE_CLKSRC_SEL *ClkSrc, UINT32 *uiClkRate);
extern ER sie1_setClock(SIE_CLKSRC_SEL ClkSrc, UINT32 uiClkRate);
extern ER sie2_setClock(SIE_CLKSRC_SEL ClkSrc, UINT32 uiClkRate);
extern ER sie3_setClock(SIE_CLKSRC_SEL ClkSrc, UINT32 uiClkRate);

extern void sie_isr(SIE_ENGINE_ID id);
#if defined __FREERTOS
extern void sie_platform_create_resource(void);
#else
extern void sie_platform_create_resource(SIE_MODULE_INFO *pmodule_info);
#endif
extern void sie_platform_release_resource(void);

extern ER sie_platform_flg_clear(SIE_ENGINE_ID id, FLGPTN flg);
extern ER sie_platform_flg_set(SIE_ENGINE_ID id, FLGPTN flg);
extern ER sie_platform_flg_wait(SIE_ENGINE_ID id, PFLGPTN p_flgptn, FLGPTN flg);
extern ER sie_platform_sem_wait(SIE_ENGINE_ID id);
extern ER sie_platform_sem_signal(SIE_ENGINE_ID id);
extern void sie_platform_prepare_clk(SIE_ENGINE_ID id);
extern void sie_platform_unprepare_clk(SIE_ENGINE_ID id);
extern void sie_platform_enable_clk(SIE_ENGINE_ID id);
extern void sie_platform_disable_clk(SIE_ENGINE_ID id);
extern ER sie_setmclock(SIE_MCLKSRC_SEL MClkSrc, UINT32 uiClkRate, BOOL MClkEn);
extern ER sie_setmclock2(SIE_MCLKSRC_SEL MClkSrc, UINT32 uiClkRate, BOOL MClkEn);
extern ER sie_setmclock3(SIE_MCLKSRC_SEL MClkSrc, UINT32 uiClkRate, BOOL MClkEn);
extern void sensor_set_pll5_rate(UINT32 rate);
extern UINT32 sensor_get_pll5_rate(void);
extern void sensor_set_pll10_rate(UINT32 rate);
extern UINT32 sensor_get_pll10_rate(void);
extern void sensor_set_pll12_rate(UINT32 rate);
extern UINT32 sensor_get_pll12_rate(void);
extern ER sensor_register_pll5_rate(UINT32 rate, UINT32 min_sie_freq);
extern ER sensor_register_pll10_rate(UINT32 rate, UINT32 min_sie_freq);
extern ER sensor_register_pll12_rate(UINT32 rate, UINT32 min_sie_freq);
extern UINT32 sie_calculate_mclk_lcm(void);
extern void sie_platform_disable_sram_shutdown(SIE_ENGINE_ID id);
extern void sie_platform_enable_sram_shutdown(SIE_ENGINE_ID id);
extern unsigned long sie_platform_spin_lock(UINT32 type);
extern void sie_platform_spin_unlock(UINT32 type, unsigned long flags);
extern UINT32 sie_platform_va2pa(UINT32 addr);
extern UINT32 sie_platform_dma_flush_dev2mem(UINT32 addr, UINT32 size, UINT32 type);
extern UINT32 sie_platform_dma_flush_mem2dev(UINT32 addr, UINT32 size);

extern UINT32 sie_power_saving_mode;

#ifndef __SIE_PLATFORM_REG_LOG_API__
#define __SIE_PLATFORM_REG_LOG_API__

/*
	log register api
*/
typedef struct {
	UINT32 ofs;
	UINT32 val;
} SIE_REG_INFO;

typedef struct {
	UINT32 cnt;
	SIE_REG_INFO *p_reg;
} SIE_REG_LOG_RESULT;

void sie_platform_set_reg(UINT32 addr, UINT32 ofs, UINT32 val);
INT32 sie_platform_set_reg_log_enable(UINT32 id, UINT32 en);
INT32 sie_platform_set_reg_log_disable(UINT32 id, UINT32 b_freebuf);
SIE_REG_LOG_RESULT sie_platform_get_reg_log(UINT32 id);
#endif

#endif
