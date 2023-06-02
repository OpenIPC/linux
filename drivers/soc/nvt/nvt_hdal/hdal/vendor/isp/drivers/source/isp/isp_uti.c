#if defined(__FREERTOS)
#include <stdlib.h>
#else
#include <linux/slab.h>
#endif
#include "kwrap/type.h"

//=============================================================================
// define
//=============================================================================

//=============================================================================
// global
//=============================================================================

//=============================================================================
// function declaration
//=============================================================================

//=============================================================================
// extern functions
//=============================================================================
void *isp_uti_mem_alloc(UINT32 mem_size)
{
	#if defined(__FREERTOS)
	return malloc(mem_size);
	#else
	return kzalloc(mem_size, GFP_KERNEL);
	#endif
}

void isp_uti_mem_free(void *mem_addr)
{
	#if defined(__FREERTOS)
	free(mem_addr);
	#else
	kfree(mem_addr);
	#endif
}

#if defined(__KERNEL__)
EXPORT_SYMBOL(isp_uti_mem_alloc);
EXPORT_SYMBOL(isp_uti_mem_free);
#endif


