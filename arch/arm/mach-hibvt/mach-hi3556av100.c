/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef CONFIG_SMP
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/delay.h>

#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>
#include <asm/mach/map.h>

void hi3556av100_set_cpu(int cpu, bool enable)
{
    void *crg_base;
    unsigned int val;

    crg_base = ioremap_nocache(0x04510000, 0x1000);

    if (enable) {
        val = readl_relaxed(crg_base + 0xcc);
        val &= ~((0x1 << 25) | (0x1 << 1));
        writel_relaxed(val, crg_base + 0xcc);
    }

    iounmap(crg_base);
    crg_base = NULL;
}

void hi35xx_set_cpu(unsigned int cpu, bool enable)
{
    hi3556av100_set_cpu(cpu, enable);
}

static void __init hi3556av100_smp_prepare_cpus(unsigned int max_cpus)
{
}

static void hi3556av100_set_boot_addr(phys_addr_t start_addr, phys_addr_t jump_addr)
{
    void __iomem *virt;

    /* virt = phys_to_virt(start_addr); */
    virt = ioremap_nocache(start_addr, 0x1000);

    writel_relaxed(0xe51ff004, virt);
    writel_relaxed(jump_addr, virt + 4);

    iounmap(virt);
}

static int hi3556av100_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
    phys_addr_t jumpaddr;
    unsigned int remap_reg_value = 0;
    struct device_node *node;

    jumpaddr = virt_to_phys(secondary_startup);
    hi3556av100_set_boot_addr(0x04200000, jumpaddr);

    hi3556av100_set_cpu(cpu, true);

    return 0;
}

static const struct smp_operations hi3556av100_smp_ops __initconst = {
    .smp_prepare_cpus   = hi3556av100_smp_prepare_cpus,
    .smp_boot_secondary = hi3556av100_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
    // .cpu_die      = hi3xxx_cpu_die,
    // .cpu_kill     = hi3xxx_cpu_kill,
#endif
};

CPU_METHOD_OF_DECLARE(hi3556av100_smp, "hisilicon,hi3556av100-smp", &hi3556av100_smp_ops);
#endif
