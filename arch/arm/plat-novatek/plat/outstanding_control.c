/**
    NVT outstanding control function
    This file will control outstanding value
    @file       outstanding_control.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include <linux/of.h>
#include <linux/spinlock.h>
#include <mach/rcw_macro.h>
#include <mach/nvt_type.h>

static void __iomem *remap_base, *remap_base2;
#define NVT_OSDG_SETREG(ofs, value)     writel((value), (remap_base+(ofs)))
#define NVT_OSDG_GETREG(ofs)            readl(remap_base+(ofs))
#define NVT_OSDG2_SETREG(ofs, value)    writel((value), (remap_base2+(ofs)))
#define NVT_OSDG2_GETREG(ofs)           readl(remap_base2+(ofs))

#ifdef __KERNEL__
static DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   spin_unlock_irqrestore(&my_lock, flags)
#endif

static int __init nvt_outstanding_control(void)
{
	struct device_node *node, *osdg_node;
	u32 value[2] = {};
	u32 reg_address = 0x0;
	u32 dram_number = 0x0;
	int count = 0;
	unsigned long flags = 0x0;

	node = of_find_node_by_path("/outstanding@0");
	if (node) {
		of_property_read_u32(node, "dram_number", &dram_number);

		if (dram_number == 0)
			return 0;

		for_each_child_of_node(node, osdg_node) {
			if (!of_property_read_u32(osdg_node, "base_addr", &reg_address)) {
				if ((dram_number > 1) && count) {
					remap_base2 = ioremap_nocache(reg_address, 0x10);
					if (unlikely(remap_base2 == 0)) {
						pr_err("%s fails: ioremap_nocache fail\n", __FUNCTION__);
						return -1;
					}
				} else {
					remap_base = ioremap_nocache(reg_address, 0x10);
					if (unlikely(remap_base == 0)) {
						pr_err("%s fails: ioremap_nocache fail\n", __FUNCTION__);
						return -1;
					}
				}
				count++;
			}
		}

		for_each_child_of_node(node, osdg_node) {
			if (!of_property_read_u32_array(osdg_node, "osdg_config", value, 2)) {
				if (value[0] != 0) {
					pr_err("%s fails: not support offset 0x%x\n", __FUNCTION__, value[0]);
					return -1;
				}
				if (value[1] >= 0x8000) {
					pr_err("%s fails: not support setting value 0x%x\n", __FUNCTION__, value[1]);
					return -1;
				}				
				loc_cpu(flags);
				if (dram_number > 1) {
					NVT_OSDG_SETREG(value[0], value[1]);
					NVT_OSDG2_SETREG(value[0], value[1]);
				} else {
					NVT_OSDG_SETREG(value[0], value[1]);
				}
				unl_cpu(flags);
			}
		}
	}

	return 0;
}

arch_initcall_sync(nvt_outstanding_control);