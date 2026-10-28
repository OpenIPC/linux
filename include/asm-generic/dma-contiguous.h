#ifndef ASM_DMA_CONTIGUOUS_H
#define ASM_DMA_CONTIGUOUS_H

#ifdef __KERNEL__
#ifdef CONFIG_CMA

#include <linux/device.h>
#include <linux/dma-contiguous.h>

/* One CMA area for all devices: struct device keeps its non-CMA layout
 * (see include/linux/device.h) */
static inline struct cma *dev_get_cma_area(struct device *dev)
{
	return dma_contiguous_default_area;
}

static inline void dev_set_cma_area(struct device *dev, struct cma *cma)
{
	WARN_ONCE(dev, "CMA: no per-device areas on this kernel\n");
	if (!dev && !dma_contiguous_default_area)
		dma_contiguous_default_area = cma;
}

#endif
#endif

#endif
