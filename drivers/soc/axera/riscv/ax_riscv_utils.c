/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/sched/clock.h>

#define SW_INT0_BASE            0x2310000
#define SW_INT1_BASE            0x2310040
#define SW_INT2_BASE            0x2310080
#define SW_INT3_BASE            0x23100c0
#define SW_INT_TRIGGER_OFFSET   0x10

#define INTERRUPT_UMASK_GROUP_0    0x2340284
#define INTERRUPT_UMASK_GROUP_1    0x2340290
#define INTERRUPT_UMASK_GROUP_2    0x234029c
#define INTERRUPT_UMASK_GROUP_3    0x23402a8
#define INTERRUPT_VECTOR_MAX    113

static u32 sw_int_base_addr[] = {
    SW_INT0_BASE,
    SW_INT1_BASE,
    SW_INT2_BASE,
    SW_INT3_BASE
};

static u32 interrupt_umask_addr[] = {
    INTERRUPT_UMASK_GROUP_0,
    INTERRUPT_UMASK_GROUP_1,
    INTERRUPT_UMASK_GROUP_2,
    INTERRUPT_UMASK_GROUP_3
};

int ax_riscv_utils_sw_int_trigger(u32 group, u32 channel)
{
    u32 *vir_base;
    unsigned long phy_base = sw_int_base_addr[group];
    u32 trigger_value = 1 << channel;

    vir_base = ioremap(phy_base, 0x1000);
    if (vir_base == NULL) {
        pr_err("remap vir_base fail\n");
        return -1;
    }
    *(vir_base + (SW_INT_TRIGGER_OFFSET / 4)) = trigger_value;
    iounmap(vir_base);

    return 0;
}

int ax_riscv_utils_interrupt_umask(int vector)
{
    int mask_index;
    int mask_shift;
    u32 umask_paddr;
    u32 *umask_vaddr;

    if (vector < 0 || vector >= INTERRUPT_VECTOR_MAX) {
        pr_err("vector %d out of range\n", vector);
        return -1;
    }

    mask_index = vector / 32;
    mask_shift = vector % 32;
    umask_paddr = interrupt_umask_addr[mask_index];

    umask_vaddr = ioremap(umask_paddr, 0x100);
    if (umask_vaddr == NULL) {
        pr_err("remap 0x%x fail\n", umask_paddr);
        return -1;
    }
    *(umask_vaddr) |= (1 << mask_shift);
    iounmap(umask_vaddr);

    return 0;
}

int ax_riscv_utils_get_dts_reg(const char *dts_path, u64 *addr, u64 *size)
{
    struct device_node *dts_node;

    if (dts_path == NULL || addr == NULL || size == NULL) {
        pr_err("params error\n");
        return -1;
    }

    dts_node = of_find_node_by_path(dts_path);
    if (!dts_node) {
        return -1;
    }
    if (of_property_read_u64(dts_node, "reg", addr) ||
        of_property_read_u64_index(dts_node, "reg", 1, size)) {
        return -1;
    }

    return 0;
}

u64 ax_riscv_utils_get_microseconds(void)
{
    u64 current_microsecond = 0;
    u64 tmp_remainder = 0;

    current_microsecond = sched_clock();

    tmp_remainder = do_div(current_microsecond, 1000);

    return current_microsecond;
}
