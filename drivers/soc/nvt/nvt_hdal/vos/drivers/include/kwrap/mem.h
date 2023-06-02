#ifndef _VOS_MEM_H_
#define _VOS_MEM_H_
#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <kwrap/nvt_type.h>

typedef void* VOS_MEM_CMA_HDL;

typedef enum _VOS_MEM_CMA_TYPE {
	VOS_MEM_CMA_TYPE_CACHE = 1,         ///< linux: dma_alloc_from_contiguous / freertos: malloc
	VOS_MEM_CMA_TYPE_NONCACHE,          ///< linux: dma_alloc_coherent        / freertos: Not supported
	VOS_MEM_CMA_TYPE_BUFFER,            ///< linux: dma_alloc_writecombine    / freertos: Not supported
}VOS_MEM_CMA_TYPE;

struct vos_mem_cma_info_t {
    VOS_MEM_CMA_TYPE        type;       ///< allocate type, refer to #VOS_FMEM_ALLOC_TYPE
    ULONG                   reserved1;
    ULONG                   paddr;      ///< physical address
    ULONG                   reserved2;
    ULONG                   reserved3;
    ULONG                   vaddr;      ///< virtual address
    ULONG                   size;       ///< size
};

void rtos_mem_init(void *param);
void rtos_mem_exit(void);

int vos_mem_init_cma_info(struct vos_mem_cma_info_t *p_info, VOS_MEM_CMA_TYPE type, UINT32 size);
VOS_MEM_CMA_HDL vos_mem_alloc_from_cma(struct vos_mem_cma_info_t *p_info);
int vos_mem_release_from_cma(VOS_MEM_CMA_HDL cma_hdl);

#ifdef __cplusplus
}
#endif

#endif /* _VOS_MEM_H_ */

