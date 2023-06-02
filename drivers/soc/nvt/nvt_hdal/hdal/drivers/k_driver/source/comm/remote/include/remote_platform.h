#ifndef _REMOTE_PLATFORM_H_
#define _REMOTE_PLATFORM_H_

#include "comm/remote.h"
#include "plat/top.h"

#if defined __UITRON || defined __ECOS
#include "DrvCommon.h"
#include "pll.h"
#include "pll_protected.h"
#include "top.h"
#include "clock.h"
#include "ist.h"

#else // __LINUX || __FREERTOS
#include "kwrap/semaphore.h"
#include "kwrap/spinlock.h"
#include "comm/driver.h"

#if defined(_BSP_NA51000_)
#include "mach/rcw_macro.h"
#else
#include "kwrap/error_no.h"
#include "rcw_macro.h"
#endif

#if defined __LINUX
#include <linux/clk.h>
#include "remote_drv.h"
#include "remote_dbg.h"

#elif defined __FREERTOS
#define __MODULE__ remote
#include "kwrap/debug.h"
#include "plat/pll.h"
#include "pll_protected.h"
#include "io_address.h"
#include "plat/interrupt.h"

#endif
#endif

#if defined(_NVT_FPGA_)
#define RM_CYCLE_TIME_RTC			31250     // 31250 Hz => 1/31250=32.00 us (Y2(16M)/512=31250 Hz)
#define RM_CYCLE_TIME_OSC			32000     // 32000 Hz => 1/32000=31.25 us (Y1(12M)/375=32000 Hz)
#define RM_CYCLE_TIME_3M			300000    // 300000 Hz => 1/300000=3.33 us (Y23(24M)/80=300000 Hz)
#else
#define RM_CYCLE_TIME_RTC			32768     // 32768 Hz => 1/32768=30.52 us
#define RM_CYCLE_TIME_OSC			32000     // 32000 HZ => 1/32000=31.25 us
#define RM_CYCLE_TIME_3M			3000000   // 3000000 Hz => 1/3000000=0.333 us
#endif

extern UINT64 guiRemoteCycleTime;
extern UINT32 guiRemoteStatus;

#if defined __UITRON || defined __ECOS
#define pf_ist_cbs_remote_platform pfIstCB_Remote

#else // __LINUX || __FREERTOS
#if defined(_NVT_EMULATION_)
#define _EMULATION_ ENABLE
#else
#define _EMULATION_ DISABLE
#endif
#if defined(_NVT_FPGA_)
#define _FPGA_EMULATION_ ENABLE
#else
#define _FPGA_EMULATION_ DISABLE
#endif
#define DRV_SUPPORT_IST 0

extern UINT32 _REGIOBASE;
extern volatile DRV_CB pf_ist_cbs_remote;
#define pf_ist_cbs_remote_platform pf_ist_cbs_remote

#if defined __LINUX
#elif defined __FREERTOS
#endif
#endif


ER remote_platform_sem_wait(void);
ER remote_platform_sem_signal(void);
UINT32 remote_platform_spin_lock(void);
void remote_platform_spin_unlock(UINT32 flags);
void remote_platform_int_enable(void);
void remote_platform_int_disable(void);
void remote_platform_set_ist_event(UINT32 events);

ER remote_platform_create_resource(void *pmodule_info); // REMOTE_MODULE_INFO* on linux, NULL on other platform
void remote_platform_release_resource(void);
void remote_platform_clock_enable(void);
void remote_platform_clock_disable(void);
ER remote_platform_select_clock_source(REMOTE_CLK_SRC_SEL src_type);

#endif
