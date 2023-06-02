#ifndef __DMA_GM_H
#define __DMA_GM_H

enum dma_kind {
	APB_DMA,
	AHB_DMA,
	AXI_DMA,
};

/** 
* @brief memory copy used by DMA
* 
* @style: choose DMA style (APB, AHB, AXI)
* @dest: DMA memory copy's destination address
* @src: DMA memory copy's source address 
* @len: DMA memory copy's memory length 
* 
* @return 0: success, other: failed 
*/
int dma_memcpy(enum dma_kind style, dma_addr_t dest, dma_addr_t src, size_t len);

/** 
* @brief memory set used by DMA
* 
* @style: choose DMA style (APB, AHB, AXI)
* @dest: DMA memory set's destination address
* @value: DMA memory set's value
* @len: DMA memory set's memory length 
* 
* @return 0: success, other: failed 
*/
int dma_memset(enum dma_kind style, dma_addr_t dest, int value, size_t len);
#endif /* __DMA_GM_H */
