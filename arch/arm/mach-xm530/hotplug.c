/******************************************************************************
 *    COPYRIGHT (C) 2013 Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-12-18
 *
******************************************************************************/

#include <linux/kernel.h>
#include <linux/smp.h>
#include <linux/completion.h>
#include <mach/hardware.h>
#include <asm/cacheflush.h>
#include <asm/delay.h>
#include <asm/io.h>



/*****************************************************************************/

static inline void xm530_scu_power_off(int cpu)
{
	writel(1, __io_address(SYS_CTRL_BASE + REG_SYS_SOFT_RSTEN));
	writel(0XCA11C100, __io_address(SYS_CTRL_BASE + REG_CPU1_SOFT_RST));
	writel(0, __io_address(SYS_CTRL_BASE + REG_SYS_SOFT_RSTEN));
}


/*****************************************************************************/

void xm530_cpu_die(unsigned int cpu)
{
	flush_cache_all();
	xm530_scu_power_off(cpu);
	BUG();
}
/*****************************************************************************/
/*
 * copy startup code to sram, and flash cache.
 * @start_addr: slave start phy address
 * @jump_addr: slave jump phy address
 */
void set_scu_boot_addr(unsigned int start_addr, unsigned int jump_addr)
{
	unsigned int *virtaddr;
	unsigned int *p_virtaddr;

	writel(1, __io_address(SYS_CTRL_BASE + REG_SC_REMAP));	
	while(readl(__io_address(SYS_CTRL_BASE + REG_SC_REMAP)) == 0);


	p_virtaddr = virtaddr = ioremap(start_addr, PAGE_SIZE);

	*p_virtaddr++ = 0xe51ff004; /* ldr  pc, [pc, #-4] */
	*p_virtaddr++ = jump_addr;  /* pc jump phy address */

	smp_wmb();
	__cpuc_flush_dcache_area((void *)virtaddr,
		(size_t)((char *)p_virtaddr - (char *)virtaddr));
	outer_clean_range(__pa(virtaddr), __pa(p_virtaddr));

	iounmap(virtaddr);
}
/*****************************************************************************/

void xm530_scu_power_up(int cpu)
{
	writel(1, __io_address(SYS_CTRL_BASE + REG_SYS_SOFT_RSTEN));
	if(readl(__io_address(SYS_CTRL_BASE + REG_CPU1_SOFT_RST)) == 1)
	{
		writel(0XCA11C100, __io_address(SYS_CTRL_BASE + REG_CPU1_SOFT_RST));
		udelay(1);
	}
	writel(1, __io_address(SYS_CTRL_BASE + REG_CPU1_SOFT_RST));
	writel(0, __io_address(SYS_CTRL_BASE + REG_SYS_SOFT_RSTEN));
}

