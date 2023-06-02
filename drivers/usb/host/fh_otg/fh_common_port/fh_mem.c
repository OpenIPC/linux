/* Memory Debugging */
#ifdef FH_DEBUG_MEMORY

#include "fh_os.h"
#include "fh_list.h"

struct allocation {
	void *addr;
	void *ctx;
	char *func;
	int line;
	uint32_t size;
	int dma;
	FH_CIRCLEQ_ENTRY(allocation) entry;
};

FH_CIRCLEQ_HEAD(allocation_queue, allocation);

struct allocation_manager {
	void *mem_ctx;
	struct allocation_queue allocations;

	/* statistics */
	int num;
	int num_freed;
	int num_active;
	uint32_t total;
	uint32_t cur;
	uint32_t max;
};

static struct allocation_manager *manager = NULL;

static int add_allocation(void *ctx, uint32_t size, char const *func, int line, void *addr,
			  int dma)
{
	struct allocation *a;

	FH_ASSERT(manager != NULL, "manager not allocated");

	a = __FH_ALLOC_ATOMIC(manager->mem_ctx, sizeof(*a));
	if (!a) {
		return -FH_E_NO_MEMORY;
	}

	a->func = __FH_ALLOC_ATOMIC(manager->mem_ctx, FH_STRLEN(func) + 1);
	if (!a->func) {
		__FH_FREE(manager->mem_ctx, a);
		return -FH_E_NO_MEMORY;
	}

	FH_MEMCPY(a->func, func, FH_STRLEN(func) + 1);
	a->addr = addr;
	a->ctx = ctx;
	a->line = line;
	a->size = size;
	a->dma = dma;
	FH_CIRCLEQ_INSERT_TAIL(&manager->allocations, a, entry);

	/* Update stats */
	manager->num++;
	manager->num_active++;
	manager->total += size;
	manager->cur += size;

	if (manager->max < manager->cur) {
		manager->max = manager->cur;
	}

	return 0;
}

static struct allocation *find_allocation(void *ctx, void *addr)
{
	struct allocation *a;

	FH_CIRCLEQ_FOREACH(a, &manager->allocations, entry) {
		if (a->ctx == ctx && a->addr == addr) {
			return a;
		}
	}

	return NULL;
}

static void free_allocation(void *ctx, void *addr, char const *func, int line)
{
	struct allocation *a = find_allocation(ctx, addr);

	if (!a) {
		FH_ASSERT(0,
			   "Free of address %p that was never allocated or already freed %s:%d",
			   addr, func, line);
		return;
	}

	FH_CIRCLEQ_REMOVE(&manager->allocations, a, entry);

	manager->num_active--;
	manager->num_freed++;
	manager->cur -= a->size;
	__FH_FREE(manager->mem_ctx, a->func);
	__FH_FREE(manager->mem_ctx, a);
}

int fh_memory_debug_start(void *mem_ctx)
{
	FH_ASSERT(manager == NULL, "Memory debugging has already started\n");

	if (manager) {
		return -FH_E_BUSY;
	}

	manager = __FH_ALLOC(mem_ctx, sizeof(*manager));
	if (!manager) {
		return -FH_E_NO_MEMORY;
	}

	FH_CIRCLEQ_INIT(&manager->allocations);
	manager->mem_ctx = mem_ctx;
	manager->num = 0;
	manager->num_freed = 0;
	manager->num_active = 0;
	manager->total = 0;
	manager->cur = 0;
	manager->max = 0;

	return 0;
}

void fh_memory_debug_stop(void)
{
	struct allocation *a;

	fh_memory_debug_report();

	FH_CIRCLEQ_FOREACH(a, &manager->allocations, entry) {
		FH_ERROR("Memory leaked from %s:%d\n", a->func, a->line);
		free_allocation(a->ctx, a->addr, NULL, -1);
	}

	__FH_FREE(manager->mem_ctx, manager);
}

void fh_memory_debug_report(void)
{
	struct allocation *a;

	FH_PRINTF("\n\n\n----------------- Memory Debugging Report -----------------\n\n");
	FH_PRINTF("Num Allocations = %d\n", manager->num);
	FH_PRINTF("Freed = %d\n", manager->num_freed);
	FH_PRINTF("Active = %d\n", manager->num_active);
	FH_PRINTF("Current Memory Used = %d\n", manager->cur);
	FH_PRINTF("Total Memory Used = %d\n", manager->total);
	FH_PRINTF("Maximum Memory Used at Once = %d\n", manager->max);
	FH_PRINTF("Unfreed allocations:\n");

	FH_CIRCLEQ_FOREACH(a, &manager->allocations, entry) {
		FH_PRINTF("    addr=%p, size=%d from %s:%d, DMA=%d\n",
			   a->addr, a->size, a->func, a->line, a->dma);
	}
}

/* The replacement functions */
void *fh_alloc_debug(void *mem_ctx, uint32_t size, char const *func, int line)
{
	void *addr = __FH_ALLOC(mem_ctx, size);

	if (!addr) {
		return NULL;
	}

	if (add_allocation(mem_ctx, size, func, line, addr, 0)) {
		__FH_FREE(mem_ctx, addr);
		return NULL;
	}

	return addr;
}

void *fh_alloc_atomic_debug(void *mem_ctx, uint32_t size, char const *func,
			     int line)
{
	void *addr = __FH_ALLOC_ATOMIC(mem_ctx, size);

	if (!addr) {
		return NULL;
	}

	if (add_allocation(mem_ctx, size, func, line, addr, 0)) {
		__FH_FREE(mem_ctx, addr);
		return NULL;
	}

	return addr;
}

void fh_free_debug(void *mem_ctx, void *addr, char const *func, int line)
{
	free_allocation(mem_ctx, addr, func, line);
	__FH_FREE(mem_ctx, addr);
}

void *fh_dma_alloc_debug(void *dma_ctx, uint32_t size, fh_dma_t *dma_addr,
			  char const *func, int line)
{
	void *addr = __FH_DMA_ALLOC(dma_ctx, size, dma_addr);

	if (!addr) {
		return NULL;
	}

	if (add_allocation(dma_ctx, size, func, line, addr, 1)) {
		__FH_DMA_FREE(dma_ctx, size, addr, *dma_addr);
		return NULL;
	}

	return addr;
}

void *fh_dma_alloc_atomic_debug(void *dma_ctx, uint32_t size,
				 fh_dma_t *dma_addr, char const *func, int line)
{
	void *addr = __FH_DMA_ALLOC_ATOMIC(dma_ctx, size, dma_addr);

	if (!addr) {
		return NULL;
	}

	if (add_allocation(dma_ctx, size, func, line, addr, 1)) {
		__FH_DMA_FREE(dma_ctx, size, addr, *dma_addr);
		return NULL;
	}

	return addr;
}

void fh_dma_free_debug(void *dma_ctx, uint32_t size, void *virt_addr,
			fh_dma_t dma_addr, char const *func, int line)
{
	free_allocation(dma_ctx, virt_addr, func, line);
	__FH_DMA_FREE(dma_ctx, size, virt_addr, dma_addr);
}

#endif /* FH_DEBUG_MEMORY */
