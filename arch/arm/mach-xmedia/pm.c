#include <linux/suspend.h>
#include <linux/io.h>
#include <asm/cacheflush.h>

void xmedia_sys_suspend(void);

extern unsigned long xmedia_sys_suspend_sz;
static void __iomem *iram_base;

#define XMEDIA_IRAM_BASE 0x04010000
#define IRAM_SIZE (64 * 1024)

static int xmedia_pm_enter(suspend_state_t state)
{
	void (*xmedia_suspend_ptr) (void);
	printk("xmedia_pm_enter xmedia_sys_suspend_sz  = %lu iram_base = %p\r\n", xmedia_sys_suspend_sz, iram_base);

	memcpy((void *) iram_base, &xmedia_sys_suspend, xmedia_sys_suspend_sz);

	flush_icache_range((unsigned long)iram_base,
		(unsigned long)(iram_base) + xmedia_sys_suspend_sz);

	xmedia_suspend_ptr = (void *) iram_base;
	flush_cache_all();

	(void) xmedia_suspend_ptr();

	return 0;
}


static const struct platform_suspend_ops xmedia_pm_ops = {
	.valid	= suspend_valid_only_mem,
	.enter	= xmedia_pm_enter,
};

static int __init xmedia_pm_init(void)
{
	iram_base = __arm_ioremap_exec(XMEDIA_IRAM_BASE, IRAM_SIZE, false);
	memset(iram_base, 0, IRAM_SIZE);

	suspend_set_ops(&xmedia_pm_ops);

	return 0;
}
arch_initcall(xmedia_pm_init);
