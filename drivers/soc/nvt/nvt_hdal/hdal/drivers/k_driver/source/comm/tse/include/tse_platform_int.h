/*
    TS MUXER/TS DEMUXER/HWCOPY Engine Integration module internal driver header

    @file       tse_platform_int.h
    @ingroup    mIDrvMisc_TSE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/



/*
    @addtogroup mIDrvMisc_TSE
*/
//@{

#ifndef _TSE_PLATFORM_INT_H
#define _TSE_PLATFORM_INT_H

#if (defined __UITRON || defined __ECOS)
#include "Type.h"
#include "dma_protected.h"
#include "DrvCommon.h"
#include "DrvConfig.h"
#elif defined(__FREERTOS)
#include "comm/tse.h"
#include "rcw_macro.h"
#include "kwrap/spinlock.h"
#include "kwrap/nvt_type.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/cpu.h"
#include "tse_dbg.h"
#include "io_address.h"
#include "pll.h"
#include "pll_protected.h"
#include "dma_protected.h"
#include "interrupt.h"
#include "top.h"
#include "plat/nvt-sramctl.h"
#define EXPORT_SYMBOL(a)
#else
#include "comm/tse.h"
#include "mach/rcw_macro.h"
#include "kwrap/spinlock.h"
#include "kwrap/nvt_type.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/cpu.h"
#include "tse_drv.h"
#include "tse_dbg.h"
#include <mach/fmem.h>
#include <plat/nvt-sramctl.h>
#include <plat/top.h>
#endif



#if (defined __UITRON || defined __ECOS)
#elif (defined __FREERTOS)
irqreturn_t tse_platform_isr(int irq, void *devid);
#else
void tse_platform_set_resource(MODULE_INFO *pmodule_info);
void tse_platform_release_resource(void);
void tse_platform_isr(void);
#endif

#endif //_TSE_PLATFORM_INT_H
