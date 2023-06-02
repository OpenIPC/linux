
#ifndef _IPE_ENG_INT_PLATFORM_H_
#define _IPE_ENG_INT_PLATFORM_H_

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

extern UINT32 _ipe_reg_io_base;

#define _IPE_REG_BASE_ADDR      _ipe_reg_io_base
#define IPE_REG_ADDR(ofs)       (_IPE_REG_BASE_ADDR+(ofs))
#define IPE_SETREG(ofs, value)  iowrite32(value, (void*)(_IPE_REG_BASE_ADDR + ofs))
#define IPE_GETREG(ofs)         ioread32((void *)(_IPE_REG_BASE_ADDR + ofs))


#define IPE_ENG_SETREG(ofs, value)   iowrite32(value, (void*)(ofs))
#define IPE_ENG_GETREG(ofs)          ioread32((void*)(ofs))

//=========================================================================
#elif defined (__FREERTOS)

#include "string.h"
#include <stdlib.h>
#include "rcw_macro.h"
#include "io_address.h"
#include "malloc.h"
#include "nvt-sramctl.h"
#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
#include "dma_protected.h"

extern UINT32 _ipe_reg_io_base;

#define _IPE_REG_BASE_ADDR       _ipe_reg_io_base
#define IPE_REG_ADDR(ofs)        (_IPE_REG_BASE_ADDR+(ofs))
#define IPE_SETREG(ofs, value)   OUTW((_IPE_REG_BASE_ADDR + ofs), value)
#define IPE_GETREG(ofs)          INW(_IPE_REG_BASE_ADDR + ofs)


#define IPE_ENG_SETREG(ofs, value)   OUTW((ofs), value)
#define IPE_ENG_GETREG(ofs)          INW(ofs)

#endif

#include "plat/top.h"
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include "kwrap/cpu.h"
#include "kwrap/nvt_type.h"
#include "ipe_eng.h"

#define FLGPTN_IPE_FRAME_END        FLGPTN_BIT(1)
#define FLGPTN_IPE_STRIPE_END       FLGPTN_BIT(2)
#define FLGPTN_IPE_FRAME_START      FLGPTN_BIT(3)
#define FLGPTN_IPE_YUV_OUT_DONE     FLGPTN_BIT(4)
#define FLGPTN_IPE_UD_GMA_DONE      FLGPTN_BIT(5)
#define FLGPTN_IPE_DFG_OUT_DONE     FLGPTN_BIT(6)
#define FLGPTN_IPE_VA_OUT_DONE      FLGPTN_BIT(7)
#define FLGPTN_IPE_LL_DONE          FLGPTN_BIT(8)
#define FLGPTN_IPE_LL_JOB_DONE      FLGPTN_BIT(9)
#define FLGPTN_IPE_LL_ERR           FLGPTN_BIT(10)
#define FLGPTN_IPE_LL_ERR2          FLGPTN_BIT(11)
#define FLGPTN_IPE_FRAME_ERR        FLGPTN_BIT(12)


//-------------------------------------------------------------------------
extern ER ipe_eng_platform_sem_wait(void);
extern ER ipe_eng_platform_sem_signal(void);

extern VOID ipe_eng_platform_enable_clk(IPE_ENG_HANDLE *p_eng);
extern VOID ipe_eng_platform_disable_clk(IPE_ENG_HANDLE *p_eng);
extern VOID ipe_eng_platform_prepare_clk(IPE_ENG_HANDLE *p_eng);
extern VOID ipe_eng_platform_unprepare_clk(IPE_ENG_HANDLE *p_eng);
extern VOID ipe_eng_platform_disable_sram_shutdown(IPE_ENG_HANDLE *p_eng);
extern VOID ipe_eng_platform_enable_sram_shutdown(IPE_ENG_HANDLE *p_eng);
extern VOID ipe_eng_platform_request_irq(IPE_ENG_HANDLE *p_eng);
extern VOID ipe_eng_platform_release_irq(IPE_ENG_HANDLE *p_eng);
extern INT32 ipe_eng_platform_set_clk_rate(IPE_ENG_HANDLE *p_eng);
extern UINT32 ipe_eng_platform_get_clk_rate(VOID);
extern UINT32 ipe_eng_platform_get_chip_id(VOID);

extern ER ipe_eng_platform_flg_clear(FLGPTN flg);
extern ER ipe_eng_platform_flg_set(FLGPTN flg);
extern ER ipe_eng_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg);

extern VOID ipe_eng_platform_create_resource(VOID);
extern VOID ipe_eng_platform_release_resource(VOID);

#if defined (_NVT_EMULATION_)
extern BOOL ipe_end_time_out_status;
#endif

void *IPE_ENG_MALLOC(UINT32 size);
void IPE_ENG_FREE(void *ptr);


#ifdef __cplusplus
}
#endif


#endif // _IPE_ENG_INT_PLATFORM_H_


