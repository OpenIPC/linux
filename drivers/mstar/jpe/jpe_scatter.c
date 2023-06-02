#include <linux/kernel.h>

#include "mdrv_jpe.h"
#include "jpe_scatter.h"

#if SCATTER_USED

struct jpe_scatter* jpe_scatter_create(struct device *dev, unsigned long addr, u32 size, enum dma_data_direction dir)
{
    int i, j, k;
    struct page **pages = NULL;
    u32 page_num;
    u32 offset, scatter_size;
    int entries;
    int page_write = (dir==DMA_TO_DEVICE) ? 0 : 1;

    struct jpe_scatter *scatter;

    // Allocate container
    scatter = kcalloc(1, sizeof(struct jpe_scatter), GFP_KERNEL);
    if(scatter == NULL)
        return NULL;

    // Init values
    scatter->addr = addr;
    scatter->dir = dir;
    scatter->orig_size = size;
    // INIT_LIST_HEAD(&scatter->list);

    // Cacluate the offset
    offset = addr & ~PAGE_MASK;
    page_num = (offset + size + PAGE_SIZE-1) / PAGE_SIZE;
    //addr &= PAGE_MASK;

    // Alloacte page lists
    pages = kcalloc(1, sizeof(struct page*)*page_num, GFP_KERNEL);
    if(pages == NULL)
        goto ERROR;

    // Get pages
    down_read(&current->mm->mmap_sem);
    entries = get_user_pages(current, current->mm, addr&PAGE_MASK, page_num, page_write, 0, pages, NULL);
    up_read(&current->mm->mmap_sem);
    if(entries <= 0) {
        JPE_MSG(JPE_MSG_DEBUG, "0x%08lX has no user pages, assume the addr is physical address\n", addr);

        scatter->entries = 0; // Zero means the address is physical address

        // Allocate & init state
        scatter->state = kmalloc(sizeof(JpeState_e), GFP_KERNEL);
        if(scatter->state == NULL)
            goto ERROR;
        scatter->state[0] = JPE_IDLE_STATE;

        goto RETURN_1;
    }

    // Allocate & init scatter list
    scatter->scatter = kmalloc(sizeof(struct scatterlist)*entries, GFP_KERNEL);
    if(scatter->scatter == NULL)
        goto ERROR;
    sg_init_table(scatter->scatter, entries);

#if 0 // debug
    for(i=0; i<entries; i++) {
         JPE_MSG(JPE_MSG_DEBUG, "pages[%d] = 0x%X\n", i,  page_to_phys(pages[i]) );
    }
#endif

    // Check all pages and find out the consecution of pages
    k = 0;
    for(i=0; i<entries; i=j, k++) {
        // Next paage is consecutive?
        for(j=i+1; j<entries; j++) {
            JPE_MSG(JPE_MSG_DEBUG, "%d, %d, 0x%X vs 0x%X + 0x%lX = 0x%lX\n", i, j, page_to_phys(pages[j]), page_to_phys(pages[i]), PAGE_SIZE*(j-i), page_to_phys(pages[i]) + PAGE_SIZE*(j-i));
            if(page_to_phys(pages[j]) != page_to_phys(pages[i]) + PAGE_SIZE*(j-i)) {
                break;
            #if 0 // This is a test to generate small fragments of scatter
            } else {
                break;
            #endif
            }

        }

        JPE_MSG(JPE_MSG_DEBUG, "Add one scatter(%d ~ %d)\n", i, j);

         // Calculate the scatter size which would be mapped
        scatter_size = PAGE_SIZE*(j-i) - offset;
        if(size > scatter_size) {
            size -= scatter_size;
        } else {
            scatter_size = size;
            size = 0;
        }

        // Set consecutive pages into one scatter entry
        sg_set_page(scatter->scatter+k, pages[i], scatter_size, offset);

        offset = 0; // only fist scatter has offset
    }

    // Scatter list is finish
    scatter->entries = k;
    sg_mark_end(scatter->scatter+k);

    // Allocate & init state array
    scatter->state = kmalloc(sizeof(JpeState_e)*k, GFP_KERNEL);
    if(scatter->state == NULL)
        goto ERROR;
    for(i=0; i<k; i++) {
        scatter->state[i] = JPE_IDLE_STATE;
    }

    // Dma map, and we may need to handle return values...
    dma_map_sg(dev, scatter->scatter, scatter->entries, scatter->dir);

    goto RETURN_2;

////////////////////////////////
RETURN_1:
    if(scatter->scatter != NULL)
        kfree(scatter->scatter);

RETURN_2:
    if(pages != NULL)
        kfree(pages);

    JPE_MSG(JPE_MSG_DEBUG, "jpe_sccater_create() cretaed success\n");

    return scatter;

ERROR:
    if(pages != NULL)
        kfree(pages);

    if(scatter->scatter != NULL)
        kfree(scatter->scatter);

    if(scatter->state != NULL)
        kfree(scatter->state);

    kfree(scatter);

    return NULL;
}

static void jpe_scatter_unmap(struct device *dev, struct jpe_scatter *scatter)
{
    // umap and free scatter list
    if(scatter->scatter != NULL) {
        dma_unmap_sg(dev, scatter->scatter, scatter->entries, scatter->dir);
        kfree(scatter->scatter);
        scatter->scatter = NULL;
        scatter->entries = 0;
    }
}

void jpe_scatter_release(struct device *dev, struct jpe_scatter *scatter)
{
    // umap and free scatter list
    jpe_scatter_unmap(dev, scatter);

    // Free status array
    if(scatter->state != NULL)
       kfree(scatter->state);

    kfree(scatter);
}

void jpe_scatter_list_add(struct list_head *scatter_head, struct jpe_scatter *scatter)
{
    list_add_tail(&scatter->list, scatter_head);
}

void jpe_scatter_list_release(struct device *dev, struct list_head *scatter_head)
{
    struct jpe_scatter *entry, *temp;

    // release all entries
    list_for_each_entry_safe(entry, temp, scatter_head, list) {
        list_del(&entry->list);
        jpe_scatter_release(dev, entry);
    }
}

int jpe_scatter_frag_get(struct list_head *scatter_head, unsigned long *addr, unsigned long *size)
{
    int i;
    struct jpe_scatter *entry;
    struct scatterlist *sg;

    list_for_each_entry(entry, scatter_head, list) {
        // Is a physical address?
        if(entry->entries == 0) {
            if(entry->state[0] == JPE_IDLE_STATE) {
                entry->state[0] = JPE_BUSY_STATE;
                *addr = entry->addr;
                *size = entry->orig_size;
                JPE_MSG(JPE_MSG_DEBUG, "return phy addr 0x%08lX, size %ld\n", *addr, *size);
                return 0;
            }

        } else { // scatter fragments
            // for(i=0; i<entry->entries; i++) {
            for_each_sg(entry->scatter, sg, entry->entries, i) {
                //JPE_MSG(JPE_MSG_DEBUG, "state[%d] = %X\n", i, entry->state[i]);
                if(entry->state[i] == JPE_IDLE_STATE) {
                    entry->state[i] = JPE_BUSY_STATE;
                    *addr = sg_dma_address(sg);
                    *size = sg_dma_len(sg);
                    JPE_MSG(JPE_MSG_DEBUG, "return frag addr 0x%08lX, size %ld\n", *addr, *size);
                    return 0;
                }
            }
        }
    }

    JPE_MSG(JPE_MSG_ERR, "jpe_scatter_frag_get() did not find any idel buffer/fragment\n");

    // Did not found any idle fragment
    return -ENOMEM;
}

int jpe_scatter_frag_set(struct device *dev, struct list_head *scatter_head, unsigned long addr, unsigned long size, JpeState_e new_state)
{
    int i;
    struct jpe_scatter *entry, *temp;
    struct scatterlist *sg;

    JPE_MSG(JPE_MSG_DEBUG, "addr 0x%08lX status set as %X\n", addr, new_state);

    list_for_each_entry_safe(entry, temp, scatter_head, list) {
        // Is a physical address?
        if(entry->entries == 0) {
            if(entry->addr == addr) {
                entry->output_size = size;
                entry->state[0] = new_state;
                return 0;
            }

        } else { // scatter fragments
            // for(i=0; i<entry->entries; i++) {
            for_each_sg(entry->scatter, sg, entry->entries, i) {
                // JPE_MSG(JPE_MSG_DEBUG, "fragment addr %08X = %08lX\n", sg_dma_address(sg), addr);
                if(sg_dma_address(sg) == addr) {
                    // If frame done or all fragments are filled, release the scatter
                    if(new_state == JPE_FRAME_DONE_STATE ||
                       (new_state == JPE_OUTBUF_FULL_STATE && i == entry->entries - 1/*sg_is_last(sg_next(sg))*/) )  {
                        jpe_scatter_unmap(dev, entry);  // Unmap the DMA addr
                        entry->entries = 0;             // Not scatter anyomre, so set entry as 0
                        entry->output_size += size;
                        entry->state[0] = new_state;    // Update new state for the access latter by jpe_scatter_ready_buf_get
                        return 0;
                    } else {
                        // Update statue only
                        entry->output_size += size;
                        entry->state[i] = new_state;
                        return 0;
                    }
                }
            }
        }
    }

    JPE_MSG(JPE_MSG_ERR, "jpe_scatter_frag_set() did not find the address 0x%08lX\n", addr);

    // Did not found the address
    return -ENOMEM;
}

int jpe_scatter_ready_buf_get(struct device *dev, struct list_head *scatter_head, unsigned long *addr, unsigned long *orig_size, unsigned long *output_size, JpeState_e *state)
{
    struct jpe_scatter *entry, *temp;

    list_for_each_entry_safe(entry, temp, scatter_head, list) {
        // Is ready buffer?
        if(entry->entries != 0 || (entry->state[0] != JPE_FRAME_DONE_STATE && entry->state[0] != JPE_OUTBUF_FULL_STATE) )
            continue;

        // Assign return infomation
        *addr = entry->addr;
        *orig_size = entry->orig_size;
        *output_size = entry->output_size;
        *state = entry->state[0];

        // Remove the entry from the list
        list_del(&entry->list);

        return 0;
    }

    JPE_MSG(JPE_MSG_ERR, "jpe_scatter_ready_buf_get() found no ready buffer\n");

    // Did not found the address
    return -ENOMEM;
}

bool jpe_scatter_ready_buf_check(struct device *dev, struct list_head *scatter_head)
{
    struct jpe_scatter *entry, *temp;

    list_for_each_entry_safe(entry, temp, scatter_head, list) {
        // Is ready buffer?
        if(entry->entries != 0 || (entry->state[0] != JPE_FRAME_DONE_STATE && entry->state[0] != JPE_OUTBUF_FULL_STATE) )
            continue;

        return 1;
    }

    // Did not found the address
    return 0;
}

#endif

#if 0
static void scatter_test(struct device *dev, u32 addr, u32 size)
{
    struct jpe_scatter *scatter;
    struct list_head   scatter_head;
    int i;
    unsigned long frag_addr, frag_size;

    JPE_MSG(JPE_MSG_ERR, "%s start, addr = 0x%X (0x%X), size = %d\n", __func__, addr, virt_to_phys((void*)addr), size);

    INIT_LIST_HEAD(&scatter_head);

    // Create a scatter
    scatter = jpe_sccater_create(dev, addr, size, DMA_BIDIRECTIONAL);
    if(scatter == NULL) {
        JPE_MSG(JPE_MSG_ERR, "jpe_sccater_create() return NULL!!\n");
        return;
    }

    jpe_scatter_list_add(&scatter_head, scatter);

    JPE_MSG(JPE_MSG_ERR, "Parese all fragments in all buffers added\n" );
    for(i=0; ; i++) {
        // Get a idle fragment
        if(jpe_scatter_frag_get(&scatter_head, &frag_addr, &frag_size, JPE_BUSY_STATE) != 0) {
            JPE_MSG(JPE_MSG_ERR, "Run out fragments...\n");
            break;
        }

        JPE_MSG(JPE_MSG_ERR, "sg[%d] frag = %08lX, %ld\n", i, frag_addr, frag_size );

        // Set the fragment is full
        if(jpe_scatter_frag_set(dev, &scatter_head, frag_addr, JPE_OUTBUF_FULL_STATE) != 0) {
            JPE_MSG(JPE_MSG_ERR, "Can't set fragment?? Is hould no happen in test\n");
            break;
        }
    }

    if(jpe_scatter_ready_buf_get(dev, &scatter_head, &frag_addr, &frag_size) == 0) {
        JPE_MSG(JPE_MSG_ERR, "Get ready addr = 0x%08lX, size = %ld\n", frag_addr, frag_size);
    } else {
        JPE_MSG(JPE_MSG_ERR, "Can't get ready buffer!!\n");
    }

    jpe_scatter_list_release(dev, &scatter_head);

    JPE_MSG(JPE_MSG_ERR, "%s end\n", __func__);
}
#endif
