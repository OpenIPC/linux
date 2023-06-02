#ifndef __FH_DMAC_PLAT_H__
#define __FH_DMAC_PLAT_H__

/****************************************************************************
 * #include section
 *   add #include here if any
 ***************************************************************************/
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/printk.h>
/****************************************************************************
 * #define section
 *   add constant #define here if any
 ***************************************************************************/

#define FH_AXI_DMA_PRINT(fmt, ...) printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)

#define SOC_CACHE_LINE_SIZE		32
#define AXI_DMA_NULL			(0)
#define AXI_DMA_ASSERT(n)		BUG_ON(!(n))

#define axi_dma_lock_init(p, name)	mutex_init(p)
#define _axi_dma_lock(p, t) mutex_lock(p)
#define axi_dma_unlock(p) mutex_unlock(p)
#define axi_dma_trylock(p) mutex_trylock(p)
#define axi_dma_memset(s, c, cnt) memset(s, c, cnt)
#define axi_dma_malloc(s) 	kzalloc(s, GFP_KERNEL)
#define axi_dma_free(s) 	kfree(s)
#define axi_dma_min(d0,d1)		min(d0, d1)
#define AXI_DMA_TICK_PER_SEC		100


/****************************************************************************
 * ADT section
 *   add Abstract Data Type definition here
 ***************************************************************************/
typedef signed   char                  FH_SINT8;
typedef signed   short                 FH_SINT16;
typedef signed   int                   FH_SINT32;
typedef unsigned char                  FH_UINT8;
typedef unsigned short                 FH_UINT16;
typedef unsigned int                   FH_UINT32;
typedef signed   int                   FH_ERR;

typedef struct mutex					axi_dma_lock_t;
typedef struct list_head				axi_dma_list;

#define axi_dma_list_init(n) 			    INIT_LIST_HEAD(n)
#define axi_dma_list_insert_before(to, new)	list_add_tail(new, to)
#define axi_dma_list_remove(n)			    list_del(n)
#define axi_dma_scanf                       sprintf


/**
 * struct fh_dma_platform_data - Controller configuration parameters
 * @nr_channels: Number of channels supported by hardware (max 8)
 * @is_private: The device channels should be marked as private and not for
 *	by the general purpose DMA channel allocator.
 * @chan_allocation_order: Allocate channels starting from 0 or 7
 * @chan_priority: Set channel priority increasing from 0 to 7 or 7 to 0.
 * @block_size: Maximum block size supported by the controller
 * @nr_masters: Number of AHB masters supported by the controller
 * @data_width: Maximum data width supported by hardware per AHB master
 *		(0 - 8bits, 1 - 16bits, ..., 5 - 256bits)
 * @sd: slave specific data. Used for configuring channels
 * @sd_count: count of slave data structures passed.
 */
struct fh_dma_platform_data {
	unsigned int	nr_channels;
	bool		is_private;
#define CHAN_ALLOCATION_ASCENDING	0	/* zero to seven */
#define CHAN_ALLOCATION_DESCENDING	1	/* seven to zero */
	unsigned char	chan_allocation_order;
#define CHAN_PRIORITY_ASCENDING		0	/* chan0 highest */
#define CHAN_PRIORITY_DESCENDING	1	/* chan7 highest */
	unsigned char	chan_priority;
	unsigned short	block_size;
	unsigned char	nr_masters;
	unsigned char	data_width[4];
	char *clk_name;
};


struct fh_axi_dma_platform_data {
#define CHAN_PRIORITY_ASCENDING		0	/* chan0 highest */
#define CHAN_PRIORITY_DESCENDING	1	/* chan7 highest */
	unsigned char	chan_priority;
	char *clk_name;
};

#endif
