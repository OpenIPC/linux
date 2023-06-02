#ifndef __FMEM_H
#define __FMEM_H
#include <linux/mm.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <linux/list.h>
#include <linux/dma-direction.h>

#define MAX_DDR_CHUNKS 2   /* include system memory */

typedef struct g_page_info_s
{
    int         nr_node;        /* number of active DDRs */
    int         block_sz;       /* page allocation size once */
    int         node_sz[MAX_DDR_CHUNKS];    /* the size for the DDR node */
    dma_addr_t  phy_start[MAX_DDR_CHUNKS];  /* the min physical start */
    struct list_head list[MAX_DDR_CHUNKS];  /* page node list */
} g_page_info_t;

/* page node for each pages allocation */
typedef struct {
    struct page     *page;
    dma_addr_t      phy_start;
    unsigned int    size;   /* the real size */
    int             ddr_id; /* belong to which DDR */
    struct list_head list;
} page_node_t;

void fmem_get_pageinfo(g_page_info_t **data_ptr);
int  fmem_give_pages(int node_id, unsigned int give_sz);
void fmem_fixup_memory(struct tag *tag, char **cmdline, struct meminfo *mi);

void fmem_free_ex(size_t size, void *cpu_addr, dma_addr_t handle);
void *fmem_alloc_ex(size_t size, dma_addr_t * dma_handle, unsigned long flags, int ddr_id);

/**
 * @brief to resolve the virtual address (including direct mapping, ioremap or user space address to
 *      its real physical address.
 *
 * @parm vaddr indicates any virtual address
 *
 * @return  >= 0 for success, 0xFFFFFFFF for fail
 */
phys_addr_t fmem_lookup_pa(unsigned int vaddr);

/*
 *  * @This function is used to read CPU id and pci id
 *   * @Return value: 0 for success, -1 for fail
 *    */
typedef enum {
    FMEM_CPU_FA726 = 0,
    FMEM_CPU_FA626,
    FMEM_CPU_UNKNOWN,
} fmem_cpu_id_t;

typedef enum {
    FMEM_PCI_HOST = 0,
    FMEM_PCI_DEV0,
} fmem_pci_id_t;

int fmem_get_identifier(fmem_pci_id_t *pci_id, fmem_cpu_id_t *cpu_id);

/* @this function is a data cache operation function,
 * @parm: vaddr: any virtual address
 * @parm: dir will be:
 * DMA_BIDIRECTIONAL = 0, it means flush operation.
 * DMA_TO_DEVICE = 1, it means clean operation.
 * DMA_FROM_DEVICE = 2, it means invalidate operation.
 * DMA_NONE = 3,
 */
void fmem_dcache_sync(void *vaddr, u32 len, enum dma_data_direction dir);

/* @this function is used to set outbound window of EP
 * @parm: phy_addr: start physical address
 * @parm: size: window size
 * return value: 0 for success, -1 for fail.
 * Note: This function is only called in RC.
 */
int fmem_set_ep_outbound_win(u32 phy_addr, u32 size);

/* @this function is used to translate local axi address to pcie address
 * @parm: axi_phy_addr indicates local axi address
 * return value: PCIe address. 0xFFFFFFFF if fail.
 * Note: This function is only called in RC.
 */
u32 fmem_get_pcie_addr(u32 axi_phy_addr);

/* @this function is used to translate pcie to local axi address
 * @parm: pcie_phy_addr indicates pcie address
 * return value: local axi address. 0xFFFFFFFF if fail.
 * Note: This function is only called in RC.
 */
u32 fmem_get_axi_addr(u32 pcie_phy_addr);

#endif /* __FMEM_H */



