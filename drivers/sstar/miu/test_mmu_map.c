/*
 * test_mmu_map.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#include "mdrv_miu.h"
#include <asm/io.h>

#define E_MMU_PGSZ_(x) E_MMU_PGSZ_##x
#define E_MMU_PGSZ(x)  E_MMU_PGSZ_(x)

#define MMU_PAGE_SIZE_FUCTION_(x) MMU_PAGE_SIZE_##x
#define MMU_PAGE_SIZE_FUCTION(x)  MMU_PAGE_SIZE_FUCTION_(x)

#define MMU_ADDR_TO_REGION_MACRO_(x) MMU_ADDR_TO_REGION_##x
#define MMU_ADDR_TO_REGION_MACRO(x)  MMU_ADDR_TO_REGION_MACRO_(x)

#define MMU_ADDR_TO_ENTRY_FUNCTION_(x) MMU_ADDR_TO_ENTRY_##x
#define MMU_ADDR_TO_ENTRY_FUNCTION(x)  MMU_ADDR_TO_ENTRY_FUNCTION_(x)

#define MMU_ADDR_TO_ENTRY_FUCTION(x)  MMU_ADDR_TO_ENTRY_FUNCTION(CONFIG_TEST_ALL_LINUX_MMU_PAGE_SIZE)(x)
#define MMU_ADDR_TO_REGION_FUCTION(x) MMU_ADDR_TO_REGION_MACRO(CONFIG_TEST_ALL_LINUX_MMU_PAGE_SIZE)(x)
#define MMU_MAP_ADDRESS \
    (CONFIG_TEST_ALL_LINUX_MMU_MAP_ADDRESS + MMU_PAGE_SIZE_FUCTION(CONFIG_TEST_ALL_LINUX_MMU_PAGE_SIZE))
#define MMU_MAP_SIZE \
    (MMU_PAGE_SIZE_FUCTION(CONFIG_TEST_ALL_LINUX_MMU_PAGE_SIZE) * CONFIG_TEST_ALL_LINUX_MMU_ENTRY_COUNT)
#define MMU_MAP_SIZE_MASK \
    (MMU_PAGE_SIZE_FUCTION(CONFIG_TEST_ALL_LINUX_MMU_PAGE_SIZE) * CONFIG_TEST_ALL_LINUX_MMU_ENTRY_COUNT - 1)

u64 miummu_test_phys_to_MIU(u64 x)
{
    u64 miu_offs = (((x - CONFIG_MIU0_BUS_BASE) & MMU_MAP_SIZE_MASK) + MMU_MAP_ADDRESS);

    if (x < CONFIG_MIU0_BUS_BASE)
    {
        printk(
            "\n\33[1;31m cpu_phys=0x%llx is not a correct cpu physical address \n"
            "must be greater than miu_basae=0x%llx \33[0m",
            x, (u64)CONFIG_MIU0_BUS_BASE);
        dump_stack();
    }

    return miu_offs;
}

u64 miummu_test_MIU_to_phys(u64 x)
{
    u64 phys_offs = (((x - MMU_MAP_ADDRESS) & MMU_MAP_SIZE_MASK) + CONFIG_MIU0_BUS_BASE);
    return phys_offs;
}

extern struct ms_chip *ms_chip_get(void);
#include "_ms_private.h"

static int __init sstarmmu_test_map_init(void)
{
    unsigned long long start;
    unsigned long long size;
    unsigned long long VirtAddr = 0, MiuAddr = 0, PhysAddr = 0, VPAAddr = 0;
    unsigned short     nRegion, nReplaceRegion;
    unsigned int       u32PageSize = 0;
    int                i;
    struct ms_chip *   chip = ms_chip_get();

    start    = CONFIG_MIU0_BUS_BASE;
    size     = MMU_MAP_SIZE;
    VirtAddr = (unsigned long)phys_to_virt(start);
    PhysAddr = start;
    MiuAddr  = 0;
    VPAAddr  = MMU_MAP_ADDRESS;
    printk("\33[1;31m [%s] VirtAddr = 0x%llX , PhysAddr= 0x%llX, MiuAddr= 0x%llX \33[0m", __FUNCTION__,
           (unsigned long long)VirtAddr, (unsigned long long)PhysAddr, (unsigned long long)MiuAddr);

    MDrv_MMU_SetPageSize(E_MMU_PGSZ(CONFIG_TEST_ALL_LINUX_MMU_PAGE_SIZE));
    u32PageSize = MMU_PAGE_SIZE_FUCTION(CONFIG_TEST_ALL_LINUX_MMU_PAGE_SIZE);

    nRegion        = MMU_ADDR_TO_REGION_FUCTION((unsigned long long)VPAAddr);
    nReplaceRegion = MMU_ADDR_TO_REGION_FUCTION((unsigned long long)MiuAddr);
    MDrv_MMU_SetRegion(nRegion, nReplaceRegion);
    printk("\33[1;31m [%s] nRegion = 0x%X , nReplaceRegion = 0x%X , poutMiuPtr = 0x%llX \33[0m", __FUNCTION__, nRegion,
           nReplaceRegion, (unsigned long long)VPAAddr);

    for (i = 0; i < (size / u32PageSize); i++)
    {
        MDrv_MMU_Map(MMU_ADDR_TO_ENTRY_FUCTION((unsigned long long)(VPAAddr + i * u32PageSize)),
                     MMU_ADDR_TO_ENTRY_FUCTION((unsigned long long)(MiuAddr + i * u32PageSize)));
    }

    MDrv_MMU_Enable(1);

    chip->phys_to_miu = miummu_test_phys_to_MIU;
    chip->miu_to_phys = miummu_test_MIU_to_phys;

    return 0;
}

core_initcall(sstarmmu_test_map_init);
