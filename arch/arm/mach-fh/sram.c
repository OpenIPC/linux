#include <linux/module.h>
#include <linux/init.h>
#include <linux/genalloc.h>
#include <asm-generic/sizes.h>
#include <mach/sram.h>
#include <mach/chip.h>
#include <mach/io.h>

static struct gen_pool *sram_pool;

void *sram_alloc(size_t len, dma_addr_t *dma)
{
	unsigned long vaddr;

	if (!sram_pool)
		return NULL;

	vaddr = gen_pool_alloc(sram_pool, len);
	if (!vaddr)
		return NULL;

	return (void *)vaddr;
}
EXPORT_SYMBOL(sram_alloc);

void sram_free(void *addr, size_t len)
{
	gen_pool_free(sram_pool, (unsigned long) addr, len);
}
EXPORT_SYMBOL(sram_free);


/*
 * REVISIT This supports CPU and DMA access to/from SRAM, but it
 * doesn't (yet?) support some other notable uses of SRAM:  as TCM
 * for data and/or instructions; and holding code needed to enter
 * and exit suspend states (while DRAM can't be used).
 */
static int __init sram_init(void)
{
	int status = 0;

	sram_pool = gen_pool_create(ilog2(SRAM_GRANULARITY), -1);
	if (!sram_pool)
		status = -ENOMEM;

	if (sram_pool)
		status = gen_pool_add(sram_pool, VA_RAM_REG_BASE, SRAM_SIZE, -1);
	WARN_ON(status < 0);
	return status;
}
core_initcall(sram_init);

