#ifndef __PWM_PLATFORM_H_
#define __PWM_PLATFORM_H__


#if (defined __UITRON || defined __ECOS)
#include "interrupt.h"
#include "pll.h"
#include "top.h"
#include "DrvCommon.h"
#include "nvtDrvProtected.h"
#include "pad.h"
#include "gpio.h"
#include "pwm.h"
#include "pwm_int.h"

#elif defined(__FREERTOS)
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/spinlock.h>
#include <kwrap/nvt_type.h>
#include "pwm_dbg.h"
#include "rcw_macro.h"
#include "io_address.h"
#include "comm/pwm.h"
#include "pwm_int.h"
#include "interrupt.h"
#include "pll.h"
#include "pll_protected.h"

#else
#include <linux/clk.h>
#include <mach/rcw_macro.h>
//#include <linux/spinlock.h>
#include "kwrap/spinlock.h"
#include "kwrap/type.h"//a header for basic variable type
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
//#include "frammap/frammap_if.h"
#include <plat-na51089/nvt-sramctl.h>
#include "pwm_drv.h"
#include "pwm_dbg.h"
#include "comm/pwm.h"
#include "comm/drvdump.h"
#include "pwm_int.h"

extern UINT32 IOADDR_PWM_REG_BASE;
#endif

#if defined __FREERTOS || defined __KERNEL__
#define _EMULATION_             (0)	// no emu macro in linux
#define _EMULATION_ON_CPU2_     (0)


#define	FLG_ID_PWM 0
#define	FLG_ID_PWM2 1
#define	FLG_ID_PWM3 2

// manually defined here
#define FLGPTN_PWM0 FLGPTN_BIT(0)
#define FLGPTN_PWM1 FLGPTN_BIT(1)
#define FLGPTN_PWM2 FLGPTN_BIT(2)
#define FLGPTN_PWM3 FLGPTN_BIT(3)
#define FLGPTN_PWM4 FLGPTN_BIT(4)
#define FLGPTN_PWM5 FLGPTN_BIT(5)
#define FLGPTN_PWM6 FLGPTN_BIT(6)
#define FLGPTN_PWM7 FLGPTN_BIT(7)
#define FLGPTN_PWM8 FLGPTN_BIT(8)
#define FLGPTN_PWM9 FLGPTN_BIT(9)
#define FLGPTN_PWM10 FLGPTN_BIT(10)
#define FLGPTN_PWM11 FLGPTN_BIT(11)

#define FLGPTN_PWM_MS0 FLGPTN_BIT(0)
#define FLGPTN_PWM_MS1 FLGPTN_BIT(1)
#define FLGPTN_PWM_MS2 FLGPTN_BIT(2)
#define FLGPTN_PWM_MS3 FLGPTN_BIT(3)
#define FLGPTN_PWM_MS4 FLGPTN_BIT(4)
#define FLGPTN_PWM_MS5 FLGPTN_BIT(5)
#define FLGPTN_PWM_MS6 FLGPTN_BIT(6)
#define FLGPTN_PWM_MS7 FLGPTN_BIT(7)

#define FLGPTN_PWM_00_03_CLKDIV_LOAD_DONE FLGPTN_BIT(0)

#define FLGPTN_PWM_00_03_TGT_CNT_DONE FLGPTN_BIT(0)

#define FLGPTN_PWM_CCNT0_EDGE FLGPTN_BIT(0)
#define FLGPTN_PWM_CCNT0_TRIG FLGPTN_BIT(1)
#define FLGPTN_PWM_CCNT0_TOUT FLGPTN_BIT(2)

#define FLGPTN_PWM_CCNT1_EDGE FLGPTN_BIT(0)

#define FLGPTN_PWM_CCNT2_EDGE FLGPTN_BIT(0)

#endif

#define PWM_PWMCH_BITS              12
#define PWM_MSCH_BITS               8
#define PWM_CCNTCH_BITS             3

#define PWM_SETREG(ofs, value)   OUTW(IOADDR_PWM_REG_BASE + (ofs), value)
#define PWM_GETREG(ofs)         INW(IOADDR_PWM_REG_BASE + (ofs))

extern ER pwm_platform_sem_wait(PWM_TYPE pwm_type, UINT32 id);
extern ER pwm_platform_sem_signal(PWM_TYPE pwm_type, UINT32 id);
extern ER pwm_platform_flg_clear(ID id, FLGPTN flg);
extern ER pwm_platform_flg_set(ID id, FLGPTN flg);
extern ER pwm_platform_flg_wait(PFLGPTN p_flgptn, ID id, FLGPTN flg);
extern UINT32 pwm_platform_spin_lock(void);
extern void pwm_platform_spin_unlock(UINT32 flags);
extern void pwm_platform_int_enable(void);
extern void pwm_platform_int_disable(void);
extern void pwm_platform_set_pinmux(UINT32 id, BOOL en);
extern void pwm_platform_disable_clk(UINT32 id);
extern void pwm_platform_enable_clk(UINT32 id);
extern void pwm_platform_set_clk_rate(UINT32 id, UINT32 div);
extern UINT32 pwm_platform_get_clk_rate(UINT32 id, UINT32 *div);

#if !(defined __UITRON || defined __ECOS)
//extern int DBG_ERR(const char *fmt, ...);
//extern int DBG_WRN(const char *fmt, ...);
//extern int DBG_IND(const char *fmt, ...);
#if defined __FREERTOS
extern void pwm_platform_create_resource(void);
extern UINT32 pwm_open_dest;
extern void pwm_platform_request_irq(PWM_DEST pwm_dest);
extern void pwm_platform_free_irq(PWM_DEST pwm_dest);
#else
extern void pwm_platform_create_resource(MODULE_INFO *pmodule_info);
#endif
extern void pwm_isr(void);
extern void pwm_isr2(void);
extern void pwm_platform_release_resource(void);
#endif


#endif
