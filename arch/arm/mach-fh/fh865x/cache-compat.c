/*
 * The FH885xV200 SDK ships its media drivers as objects to link against
 * the kernel (media_support/make_drivers), built for an ARMv6-only kernel,
 * where cache maintenance compiles to direct calls of the v6 routines.
 * FH865x runs the same media code on a Cortex-A7 (its media_process
 * carries chip_fh865x), so give those objects the two entry points they
 * call, backed by this CPU's own cache operations.
 *
 * Those objects also inline kmalloc() for 32-byte cache lines, so they
 * index kmalloc_caches[5] (kmalloc-32) and [1] (kmalloc-96). With the
 * Cortex-A7's 64-byte lines neither cache exists and the slots are NULL;
 * point them at the next larger caches. The kernel's own kmalloc() never
 * picks these slots, as KMALLOC_MIN_SIZE is 64 here.
 */
#include <linux/export.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>

void v6_flush_kern_cache_all(void)
{
	__cpuc_flush_kern_all();
}
EXPORT_SYMBOL(v6_flush_kern_cache_all);

void v6_flush_kern_dcache_area(void *addr, size_t size)
{
	__cpuc_flush_dcache_area(addr, size);
}
EXPORT_SYMBOL(v6_flush_kern_dcache_area);

static int __init fh865x_kmalloc_compat(void)
{
	if (!kmalloc_caches[5])
		kmalloc_caches[5] = kmalloc_caches[6];
	if (!kmalloc_caches[1])
		kmalloc_caches[1] = kmalloc_caches[7];
	return 0;
}
core_initcall(fh865x_kmalloc_compat);
