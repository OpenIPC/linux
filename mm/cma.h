#ifndef __MM_CMA_H__
#define __MM_CMA_H__

#include <linux/sched.h>

struct cma {
	unsigned long   base_pfn;
	unsigned long   count;
	unsigned long   *bitmap;
	unsigned int order_per_bit; /* Order of pages represented by one bit */
	struct mutex    lock;
#ifdef CONFIG_CMA_DEBUGFS
	struct hlist_head mem_head;
	spinlock_t mem_head_lock;
#endif
#ifdef CONFIG_CMA_BUFFER_LIST
	struct list_head buffer_list;
	struct mutex	list_lock;
#endif
};

#ifdef CONFIG_CMA_BUFFER_LIST
extern int cma_buffer_list_add(struct cma *cma, unsigned long pfn, int count);
extern void cma_buffer_list_del(struct cma *cma, unsigned long pfn, int count);
#else
#define cma_buffer_list_add(cma, pfn, count) { }
#define cma_buffer_list_del(cma, pfn, count) { }
#endif

extern struct cma cma_areas[MAX_CMA_AREAS];
extern unsigned cma_area_count;

static unsigned long cma_bitmap_maxno(struct cma *cma)
{
	return cma->count >> cma->order_per_bit;
}

#endif
