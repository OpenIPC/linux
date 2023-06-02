#ifndef _VOS_CPU_H_
#define _VOS_CPU_H_
#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <kwrap/nvt_type.h>

#define VOS_ADDR_INVALID    ((VOS_ADDR)-1)

#if VOS_LEGACY_ARCH32
#define VOS_ALIGN_BYTES     64
typedef UINT32 VOS_ADDR;
#else
#define VOS_ALIGN_BYTES     128
typedef ULONG VOS_ADDR;
#endif

#define VOS_IS_ALIGNED(x)   (0 == ((x) & (VOS_ALIGN_BYTES - 1)))


typedef enum _VOS_DMA_DIRECTION {
	VOS_DMA_BIDIRECTIONAL = 0,            ///< clean and invalidate (vaddr and len should be aligned)
	VOS_DMA_TO_DEVICE = 1,                ///< clean (vaddr and len should be aligned)
	VOS_DMA_FROM_DEVICE = 2,              ///< invalidate (vaddr and len should be aligned)
	VOS_DMA_NONE = 3,                     ///< none
	VOS_DMA_BIDIRECTIONAL_NON_ALIGN = 4 , ///< clean and invalidate (skip alignment check)
	VOS_DMA_TO_DEVICE_NON_ALIGN = 5,      ///< clean (skip alignment check)
	VOS_DMA_FROM_DEVICE_NON_ALIGN = 6,    ///< invalidate (skip alignment check)
} VOS_DMA_DIRECTION;

void rtos_cpu_init(void *param);
void rtos_cpu_exit(void);

VOS_ADDR vos_cpu_get_phy_addr(VOS_ADDR vaddr); //NOTE: return VOS_ADDR_INVALID for failure

void vos_cpu_dcache_sync(VOS_ADDR vaddr, UINT32 len, VOS_DMA_DIRECTION dir);
void vos_cpu_dcache_sync_vb(VOS_ADDR vaddr, UINT32 len, VOS_DMA_DIRECTION dir); //for video buffer, skip flush by default
int vos_cpu_dcache_sync_by_cpu(VOS_ADDR vaddr, UINT32 len, VOS_DMA_DIRECTION dir, UINT cache_op_cpu_id);

void vos_cpu_enable_watch(int i, UINT32 addr, UINT32 size);

void vos_cpu_disable_watch(int i);

#ifdef __cplusplus
}
#endif

#endif /* _VOS_CPU_H_ */

