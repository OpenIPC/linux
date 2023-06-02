/**
    NVT SRAM Contrl
    This file will Enable and disable SRAM shutdown
    @file       nvt-sramctl.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <plat/nvt-sramctl.h>
#include <linux/spinlock.h>

static void __iomem *remap_base;
#define NVT_SRAM_SETREG(ofs, value)	writel((value), (remap_base+(ofs)))
#define NVT_SRAM_GETREG(ofs)		readl(remap_base+(ofs))

#ifdef __KERNEL__
static DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   spin_unlock_irqrestore(&my_lock, flags)
#endif

void nvt_disable_sram_shutdown(SRAM_SD id)
{
	unsigned long flags;
	u32 reg_data, reg_ofs;

	reg_ofs = (id >> 5) << 2;

	loc_cpu(flags);

	reg_data = NVT_SRAM_GETREG(reg_ofs);

	reg_data &= ~(1 << (id & 0x1F));

	NVT_SRAM_SETREG(reg_ofs, reg_data);

	unl_cpu(flags);
}

void nvt_enable_sram_shutdown(SRAM_SD id)
{
	unsigned long flags;    
	u32 reg_data, reg_ofs;

	reg_ofs = (id >> 5) << 2;

	loc_cpu(flags);

	reg_data = NVT_SRAM_GETREG(reg_ofs);

	reg_data |= (1 << (id & 0x1F));

	NVT_SRAM_SETREG(reg_ofs, reg_data);

	unl_cpu(flags);
}

static int __init nvt_init_sram_membase(void)
{
	remap_base = ioremap_nocache(NVT_SRAM_PHY_BASE, 0x1000);
	if (unlikely(remap_base == 0)) {
		pr_err("%s fails: ioremap_nocache fail\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

core_initcall(nvt_init_sram_membase);
EXPORT_SYMBOL(nvt_enable_sram_shutdown);
EXPORT_SYMBOL(nvt_disable_sram_shutdown);