#ifndef __MODULE_PLATFORM_H__
#define __MODULE_PLATFORM_H__


#if (defined __UITRON || defined __ECOS)
#define __MODULE__    kdrv_dis
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*" // *=All, [mark1]=CustomClass
#include "DebugModule.h"
#include "frammap/frammap_if.h"
#include "interrupt.h"
#include "top.h"
#include "dma.h"
#include "SysKer.h"
#include "Type.h"

#elif defined(__FREERTOS)
#include "pll_protected.h"
#include "interrupt.h"
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "rcw_macro.h"
#include "kwrap/error_no.h"
#include "dma_protected.h"
#include "FileSysTsk.h"
#if defined(_BSP_NA51089_)
#include "rtos_na51089/nvt-sramctl.h"
#endif
#define EXPORT_SYMBOL(a)
#define MODULE_AUTHOR(a)
#define MODULE_LICENSE(a)

#elif defined(__LINUX)
#include <comm/nvtmem.h>

#include <mach/rcw_macro.h>

#include <plat-na51089/nvt-sramctl.h>

#endif

#include "dis_dbg.h"
#include "kdrv_type.h"
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/cpu.h"
#include "kwrap/task.h"
#include "kwrap/platform.h"
#include "kwrap/util.h"
#include "kwrap/cpu.h"
#include "kwrap/mem.h"


#if defined(__FREERTOS)
extern ID 		  FLG_ID_DIS;
#else
extern FLGPTN     FLG_ID_DIS;
#endif


#if defined(__FREERTOS) //defined __UITRON || defined __ECOS
#define DIS_SETREG(ofs,value)   	OUTW(IOADDR_DIS_REG_BASE + (ofs),value)
#define DIS_GETREG(ofs)         	INW(IOADDR_DIS_REG_BASE + (ofs))
#define DIS_GETDATA(addr, ofs)  	INW(addr + (ofs))
#else
#define DIS_SETREG(ofs, value)      iowrite32(value, (void*)(IOADDR_DIS_REG_BASE + ofs))
#define DIS_GETREG(ofs)             ioread32((void*)(IOADDR_DIS_REG_BASE + ofs))
#define DIS_GETDATA(addr, ofs)  	ioread32((void*)(addr + ofs))
#endif


extern VOID dis_platform_request_irq(VOID *isr_handler);


extern void dis_platform_clk_enable(VOID);
extern void dis_platform_clk_disable(VOID);
extern void dis_platform_prepare_clk(void);
extern void dis_platform_unprepare_clk(void);

extern void dis_platform_flg_rel(VOID);
extern ER   dis_platform_flg_clear(UINT32 flag_id);
extern void dis_platform_flg_set(UINT32 flag_id);
extern void dis_platform_flg_wait(FLGPTN *uiFlag, UINT32 flag_id);

extern UINT32 dis_platform_dma_flush(UINT32 addr, UINT32 size);
extern UINT32 dis_platform_va2pa(UINT32 addr);

extern VOID   dis_platform_create_resource(VOID);
extern ER     dis_platform_set_clock_rate(UINT32 clock);

extern UINT32 dis_pt_pa2va_remap(UINT32 pa, UINT32 sz);

#endif
