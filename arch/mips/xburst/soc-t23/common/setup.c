/*
 * JZSOC Clock and Power Manager
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/ioport.h>

#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <mach/platform.h>

void __init cpm_reset(void)
{
#ifndef CONFIG_FPGA_TEST
	unsigned long clkgr = cpm_inl(CPM_CLKGR);
	unsigned long clkgr1 = cpm_inl(CPM_CLKGR1);

	clkgr &= ~(1 << 28	/* DES */
		| 1 << 26	/* TVE */
		| 1 << 13	/* SADC */
		);
	cpm_outl(clkgr, CPM_CLKGR);

	clkgr1 &= ~(1 << 3);
	cpm_outl(clkgr1, CPM_CLKGR1);
	/* TODO set default clkgr here */
#endif
}

int __init setup_init(void)
{
	cpm_reset();
	/* Set bus priority here */
	*(volatile unsigned int *)0xb34f0240 = 0x00010003;
	*(volatile unsigned int *)0xb34f0244 = 0x00010003;

	return 0;
}
void __cpuinit jzcpu_timer_setup(void);
void __cpuinit jz_clocksource_init(void);
void __init init_all_clk(void);
/* used by linux-mti code */

void __init plat_mem_setup(void)
{
	/* use IO_BASE, so that we can use phy addr on hard manual
	 * directly with in(bwlq)/out(bwlq) in io.h.
	 */
	set_io_port_base(IO_BASE);
	ioport_resource.start	= 0x00000000;
	ioport_resource.end	= 0xffffffff;
	iomem_resource.start	= 0x00000000;
	iomem_resource.end	= 0xffffffff;
	setup_init();
	init_all_clk();

	return;
}

void __init plat_time_init(void)
{
	jzcpu_timer_setup();
	jz_clocksource_init();
}
