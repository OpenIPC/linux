#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <linux/io.h>
#include <mach/platform.h>


#define ioaddr_sys(x)	IO_ADDRESS(SYS_CTRL_BASE + (x))
static inline void arch_idle(void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks
	 */
	cpu_do_idle();
}

static inline void arch_reset(char mode, const char *cmd)
{
	writel(1, ioaddr_sys(REG_SYS_SOFT_RSTEN));
	writel(0xca110000, ioaddr_sys(REG_SYS_SOFT_RST));
}
#endif
