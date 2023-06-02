#ifndef _SIM_VDOCDC_MEM_H_
#define _SIM_VDOCDC_MEM_H_

#if defined(__LINUX)
#include <comm/nvtmem.h>
#endif

#include "kwrap/type.h"

#ifdef __FREERTOS
struct nvt_fmem_mem_info_t {
	UINT32 vaddr;
	UINT32 size;
	UINT32 ddr_addr;
	UINT32 ddr_size;
};
#endif

void *vdocdc_get_mem(struct nvt_fmem_mem_info_t *pinfo, unsigned int size);
void vdocdc_free_mem(void *handle);

#endif // _SIM_VDOCDC_MEM_H_
