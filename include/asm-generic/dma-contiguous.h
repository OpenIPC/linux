#ifndef ASM_DMA_CONTIGUOUS_H
#define ASM_DMA_CONTIGUOUS_H

#ifdef __KERNEL__
#ifdef CONFIG_CMA

#include <linux/device.h>
#include <linux/dma-contiguous.h>

/* struct device keeps its non-CMA layout (see include/linux/device.h), so
 * the areas reserved for one device are kept in drivers/base/dma-contiguous.c */
struct cma *dev_get_cma_area(struct device *dev);
void dev_set_cma_area(struct device *dev, struct cma *cma);

#endif
#endif

#endif
