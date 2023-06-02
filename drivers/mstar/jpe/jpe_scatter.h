#ifndef __JPE_SCATTER_H__
#define __JPE_SCATTER_H__

#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/list.h>

#include "mdrv_jpe_io_st.h"

#define SCATTER_USED    (0)

#if SCATTER_USED
struct jpe_scatter
{
    struct list_head        list;           // list head
    unsigned long           addr;           // Address form userspace, it maybe a virtual or physical one
    unsigned long           orig_size;      // Original buffer Size
    unsigned long           output_size;    // Output Size
    struct scatterlist      *scatter;       // Scatterlist pointer
    JpeState_e              *state;         // A array to keep the state of fragments in a scatter list
    int                     entries;        // How many entries (fragments) in the scatter list
    enum dma_data_direction dir;            // Direction of data
};

struct jpe_scatter* jpe_scatter_create(struct device *dev, unsigned long addr, u32 size, enum dma_data_direction dir);
void jpe_scatter_release(struct device *dev, struct jpe_scatter *scatter);
void jpe_scatter_list_add(struct list_head *scatter_head, struct jpe_scatter *scatter);
void jpe_scatter_list_release(struct device *dev, struct list_head *scatter_head);
int jpe_scatter_frag_get(struct list_head *scatter_head, unsigned long *addr, unsigned long *size);
int jpe_scatter_frag_set(struct device *dev, struct list_head *scatter_head, unsigned long addr, unsigned long size, JpeState_e new_state);
int jpe_scatter_ready_buf_get(struct device *dev, struct list_head *scatter_head, unsigned long *addr, unsigned long *orig_size, unsigned long *proc_size, JpeState_e *state);
bool jpe_scatter_ready_buf_check(struct device *dev, struct list_head *scatter_head);
#endif

#endif //__JPE_SCATTER_H__
