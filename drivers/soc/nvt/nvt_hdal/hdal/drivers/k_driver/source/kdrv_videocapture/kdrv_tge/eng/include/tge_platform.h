#ifndef __TGE_PLATFORM_H_
#define __TGE_PLATFORM_H__

#if (defined __UITRON || defined __ECOS)
#elif defined(__FREERTOS)
#include "string.h"
#include <stdlib.h>
#include "rcw_macro.h"
#include "io_address.h"
//#if defined(_BSP_NA51055_)
#include "nvt-sramctl.h"
//#endif
#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"
#include "dma_protected.h"
#include "kwrap/type.h"
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/spinlock.h>
#include <kwrap/cpu.h>
#include <kwrap/type.h>
#include "kdrv_videocapture/kdrv_tge.h"
#include "tge_lib.h"
#include "tge_dbg.h"
#define _TGE_REG_BASE_ADDR       IOADDR_TGE_REG_BASE
#define TGE_REG_ADDR(ofs)        (_TGE_REG_BASE_ADDR+(ofs))
#define TGE_SETREG(ofs, value)   OUTW((_TGE_REG_BASE_ADDR + ofs), (value))
#define TGE_GETREG(ofs)          INW(_TGE_REG_BASE_ADDR + ofs)
#else
#include <plat/nvt-sramctl.h>
#include "kwrap/semaphore.h"
#include <kwrap/spinlock.h>
#include "kwrap/flag.h"
#include <mach/fmem.h>
#include "tge_drv.h"
#include "tge_lib.h"
#include "tge_dbg.h"
#include "kdrv_videocapture/kdrv_tge.h"
extern UINT32 _TGE_REG_BASE_ADDR;
#define TGE_REG_ADDR(ofs)        (_TGE_REG_BASE_ADDR+(ofs))
#define TGE_SETREG(ofs, value)   iowrite32(value, (void*)(_TGE_REG_BASE_ADDR + ofs))
#define TGE_GETREG(ofs)          ioread32((void*)(_TGE_REG_BASE_ADDR + ofs))
#endif



#define FLGPTN_TGE_FRAMEEND      FLGPTN_BIT(0)
#define FLGPTN_TGE_FRAMEEND2     FLGPTN_BIT(1)
#define FLGPTN_TGE_FRAMEEND3     FLGPTN_BIT(2)
#define FLGPTN_TGE_FRAMEEND4     FLGPTN_BIT(3)
#define FLGPTN_TGE_FRAMEEND5     FLGPTN_BIT(4)
#define FLGPTN_TGE_FRAMEEND6     FLGPTN_BIT(5)
#define FLGPTN_TGE_FRAMEEND7     FLGPTN_BIT(6)
#define FLGPTN_TGE_FRAMEEND8     FLGPTN_BIT(7)
#define FLGPTN_TGE_VD_BP1        FLGPTN_BIT(8)
#define FLGPTN_TGE_VD2_BP1       FLGPTN_BIT(9)
#define FLGPTN_TGE_VD3_BP1       FLGPTN_BIT(10)
#define FLGPTN_TGE_VD4_BP1       FLGPTN_BIT(11)
#define FLGPTN_TGE_VD5_BP1       FLGPTN_BIT(12)
#define FLGPTN_TGE_VD6_BP1       FLGPTN_BIT(13)
#define FLGPTN_TGE_VD7_BP1       FLGPTN_BIT(14)
#define FLGPTN_TGE_VD8_BP1       FLGPTN_BIT(15)

#if !(defined __UITRON || defined __ECOS)
extern void tge_isr(void);
#if defined __FREERTOS
extern void tge_platform_create_resource(void);
#else
extern void tge_platform_create_resource(TGE_MODULE_INFO *pmodule_info);
#endif
extern void tge_platform_release_resource(void);
#endif

extern void tge_platform_set_clk_rate(TGE_OPENOBJ *pObjCB);
extern ER tge_setClock(TGE_CLKSRC_SEL ClkSrc);
extern ER tge_setClock2(TGE_CLKSRC_SEL ClkSrc);
extern void tge_platform_prepare_clk(void);
extern void tge_platform_unprepare_clk(void);
extern void tge_platform_enable_clk(void);
extern void tge_platform_disable_clk(void);
extern ER tge_platform_flg_clear(FLGPTN flg);
extern ER tge_platform_flg_set(FLGPTN flg);
extern ER tge_platform_flg_wait(PFLGPTN p_flgptn, FLGPTN flg);
extern ER tge_platform_sem_wait(void);
extern ER tge_platform_sem_signal(void);


#endif
