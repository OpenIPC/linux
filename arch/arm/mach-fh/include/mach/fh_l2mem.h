#ifndef __MACH_L2MEM_H
#define __MACH_L2MEM_H

/*
 * L2MEM allocations return a CPU virtual address, or NULL on error.
 * If a DMA address is requested and the L2MEM supports DMA, its
 * mapped address is also returned.
 *
 * Errors include L2MEM memory not being available, and requesting
 * DMA mapped L2MEM on systems which don't allow that.
 */

extern void *l2mem_alloc(size_t len);
extern void *l2mem_dma_alloc(size_t len, dma_addr_t *dma);
extern void l2mem_free(void *addr, size_t len);

#endif /* __MACH_SRAL2MEM*/
