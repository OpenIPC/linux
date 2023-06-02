/**
	@brief Source file of vendor net flow sample.

	@file kflow_ai_net_platform.c

	@ingroup kflow ai net mem map

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/cpu.h"

#include "kflow_ai_net/kflow_ai_net_platform.h"

#if 0 //defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#else
#include "mach/fmem.h" //for fmem_lookup_pa, PAGE_ALIGN
#include <asm/io.h>  /* for ioremap and iounmap, pfn_valid, __phys_to_pfn */
#endif

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#include "kflow_common/nvtmpp.h"
#else
#include "kflow_common/ms/ms_core.h"
#endif

UINT32 nvt_ai_va2pa(UINT32 addr)
{
	if (addr == 0) {
		return addr;
	} else {
#if 0 //defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		return nvtmpp_sys_va2pa(addr);   //--> why this cause random fail or hang in nue2 layer of resnet18, fc_slice5
#else
		return fmem_lookup_pa(addr);
#endif
	}
}

UINT32 nvt_ai_pa2va_remap(UINT32 pa, UINT32 sz)
{
	UINT32 va = 0;
	if (sz == 0) {
		return va;
	}
#if 0 //defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	va = nvtmpp_sys_pa2va(pa);
#else
	if (pfn_valid(__phys_to_pfn(pa))) {
		va = (UINT32)__va(pa);
	} else {
		va = (UINT32)ioremap(pa, PAGE_ALIGN(sz));
	}
#endif
	//fmem_dcache_sync((UINT32 *)va, sz, DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync(va, sz, VOS_DMA_TO_DEVICE); ///< cache clean - output to engine's input
	return va;
}

UINT32 nvt_ai_pa2va_remap_wo_sync(UINT32 pa, UINT32 sz)
{
	UINT32 va = 0;
#if defined(__LINUX)
	if (sz == 0) {
		return va;
	}
	if (pfn_valid(__phys_to_pfn(pa))) {
		va = (UINT32)__va(pa);
	} else {
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		va = (UINT32)nvtmpp_sys_pa2va(pa);
#else
		va = ms_user_pa_to_va(pa, 0);
#endif
		if (va == 0) {
			va = (UINT32)ioremap(pa, PAGE_ALIGN(sz));
		}
	}
#else
	va = pa;
#endif

	return va;
}


VOID nvt_ai_pa2va_unmap(UINT32 va, UINT32 pa)
{
	if (va == 0) {
		return;
	}
#if 0 //defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#else
	if (!pfn_valid(__phys_to_pfn(pa))) {
		iounmap((VOID *)va);
	}
#endif
}
