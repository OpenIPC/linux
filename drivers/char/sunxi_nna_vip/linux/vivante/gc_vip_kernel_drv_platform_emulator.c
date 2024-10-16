/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2017 - 2021 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2017 - 2021 Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
*
*****************************************************************************/

#include "gc_vip_kernel_drv.h"
#include "gc_vip_kernel_drv_platform_emulator.h"

#define MAX_PCIE_BAR                  6
#define USE_PCIE_MSI                  1
#define MAX_MSI_NUM                  32


static phy_address_t video_memory_heap_base = 0;
static phy_address_t video_memory_heap_size = 0;


struct _gcsPCIEBarInfo
{
    resource_size_t io_base;
    iomem_t virt_base;
    size_t size;
};

static struct _gcsPCIEBarInfo bar[MAX_PCIE_BAR];


#if defined (USE_LINUX_PCIE_DEVICE)
static const struct pci_device_id vivpcie_ids[] = {
  {
    .class = 0,
    .class_mask = 0,
    .vendor = VIV_PCI_VENDOR_ID,
    .device = VIV_PCI_DEVICE_ID,
    .subvendor = PCI_ANY_ID,
    .subdevice = PCI_ANY_ID,
    .driver_data = 0
  },{ /* End: all zeroes */ }
};

void query_pcie_bar_nfo(
    struct pci_dev *pdev,
    phy_address_t *bar_addr,
    vip_uint32_t *bar_size,
    vip_uint32_t bar_num
    )
{
    vip_uint32_t addr = 0;
    vip_uint32_t size = 0;

    /* Read the bar address */
    if (pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar_num * 0x4, &addr) < 0) {
        return;
    }

    /* Read the bar size */
    if (pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar_num * 0x4, 0xffffffff) < 0) {
        return;
    }

    if (pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar_num * 0x4, &size) < 0) {
        return;
    }

    size &= 0xfffffff0;
    size  = ~size;
    size += 1;

    /* Write back the bar address */
    if (pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0 + bar_num * 0x4, addr) < 0) {
        return;
    }

    PRINTK_D("Bar%d addr=0x%x size=0x%x", bar_num, addr, size);

    *bar_addr = addr;
    *bar_size = size;
}
#endif

static int get_bar_resource(struct pci_dev *pdev, uint32_t id, bool virtual)
{
    int ret = 0;

    if (!pdev || id > 5) {
        return -EINVAL;
    }

    bar[id].io_base = pci_resource_start(pdev, id);
    bar[id].size = pci_resource_len(pdev, id);

    printk("vipcore, bar=%d, base=0x%llx, size=0x%lx\n", id, bar[id].io_base, bar[id].size);

    return ret;
}

/*
@brief convert CPU physical to VIP physical.
@param cpu_physical. the physical address of CPU domain.
*/
vip_address_t gckvip_drv_get_vipphysical(
    vip_address_t cpu_physical
    )
{
    vip_address_t cpu_base_physical = bar[PCI_BAR_4].io_base;
    vip_uint32_t vipMemSize = video_memory_heap_size;
    vip_address_t vip_physical = 0;
    vip_address_t vip_PhysBaseAddr = 0;

    if (cpu_physical >= cpu_base_physical && cpu_physical < (cpu_base_physical + vipMemSize)) {
        vip_physical = cpu_physical - cpu_base_physical + vip_PhysBaseAddr;
    }
    else {
        vip_physical = cpu_physical;
    }

    return vip_physical;
}

/*
@brief convert VIP physical to CPU physical.
@param vip_physical. the physical address of VIP domain.
*/
vip_address_t gckvip_drv_get_cpuphysical(
    vip_address_t vip_physical
    )
{
    vip_address_t cpu_base_physical = bar[PCI_BAR_4].io_base;
    vip_uint32_t vipMemSize = video_memory_heap_size;
    vip_address_t vip_PhysBaseAddr = 0;
    vip_address_t cpu_hysical = 0;

    if (vip_physical >= vip_PhysBaseAddr && vip_physical < (vip_PhysBaseAddr + vipMemSize)) {
        cpu_hysical = vip_physical - vip_PhysBaseAddr + cpu_base_physical;
    }
    else {
        cpu_hysical = vip_physical;
    }

    return cpu_hysical;
}

/*
@brief 1. platfrom(vendor) initialize. prepare environmnet for running VIP hardware.
       2. initialzie linux platform device.
       3. this function should be called before probe device/driver.
          so many variable not initialize in kdriver.
@param pdrv vip drvier device object.
*/
vip_int32_t gckvip_drv_platform_init(
    gckvip_driver_t *kdriver
    )
{
#if defined (USE_LINUX_PCIE_DEVICE)
    kdriver->pdrv->id_table = vivpcie_ids;
#endif
    return 0;
}

/*
@brief 1. platfrom(vendor) un-initialize.
       2. uninitialzie linux platform device.
@param pdrv vip drvier device object.
*/
vip_int32_t gckvip_drv_platform_uninit(
    gckvip_driver_t *kdriver
    )
{
    return 0;
}

/*
@brief adjust parameters of vip devices. such as irq, SRAM, video memory heap.
        You can overwrite insmod command line in gckvip_drv_adjust_param() function.
        Verisilicon don't overwrite insmod command line parameters.
@param kdriver, vip device object.
*/
vip_int32_t gckvip_drv_adjust_param(
    gckvip_driver_t *kdriver
    )
{
    vip_int32_t ret = 0;
    vip_uint32_t core = 0;
    static u64 dma_mask;
    struct pci_dev *pdev = kdriver->pdev;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
    dma_mask = DMA_BIT_MASK(64);
#else
    dma_mask = DMA_64BIT_MASK;
#endif
    ret = pci_set_dma_mask(pdev, dma_mask);
    if (ret) {
        ret = pci_set_consistent_dma_mask(pdev, dma_mask);
        if (ret) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
            dma_mask = DMA_BIT_MASK(32);
#else
            dma_mask = DMA_32BIT_MASK;
#endif
            printk("vipcore: Failed to set 64 bit dma mask.\n");
            ret = pci_set_dma_mask(pdev, dma_mask);
            if (ret) {
                ret = pci_set_consistent_dma_mask(pdev, dma_mask);
                if (ret) {
                    printk("vipcore: Failed to set DMA mask.\n");
                    goto error_out;
                }
            }
        }
    }

    ret = get_bar_resource(pdev, PCI_BAR_0, true);
    if (ret) {
        printk("vipcore: Failed to read BAR 0\n");
        goto error_out;
    }

    ret = get_bar_resource(pdev, PCI_BAR_2, true);
    if (ret) {
        printk("vipcore: Failed to read BAR 2\n");
        goto error_out;
    }

    ret = get_bar_resource(pdev, PCI_BAR_4, false);
    if (ret) {
        printk("vipcore: Failed to read BAR 4\n");
        goto error_out;
    }

#if USE_PCIE_MSI
    ret = pci_alloc_irq_vectors(pdev, 1, MAX_MSI_NUM, PCI_IRQ_MSI);
    if (ret < 1) {
        printk("vipcore: Failed to allocate irq vectors.\n");
        goto error_out;
    }
#endif

    /* video memory heap base physical address */
    if (0 == kdriver->cpu_physical) {
        kdriver->cpu_physical = VIDEO_MEMORY_HEAP_BASE_ADDRESS;
    }
    video_memory_heap_base = kdriver->cpu_physical;

    /* video memory heap size */
    if (0 == kdriver->vip_memsize) {
        kdriver->vip_memsize = VIDEO_MEMORY_HEAP_SIZE;
    }
    video_memory_heap_size = kdriver->vip_memsize;

    printk("vipcore: video memory heap cpu physical=0x%llx, size=0x%x\n",
            kdriver->cpu_physical, kdriver->vip_memsize);
    printk("vipcore: video memory heap cpu base=0x%llx\n", bar[PCI_BAR_4].io_base);

    /* AXI SRAM base physical address */
    if (0 == kdriver->axi_sram_base) {
        kdriver->axi_sram_base = bar[PCI_BAR_4].io_base
                                + S0_DDR_RESERVE_SIZE + EDMA_LINK_TABLE_RESERVE_SIZE;
    }

    /* the size of AXI-SRAM */
    if (0 == kdriver->axi_sram_size) {
        kdriver->axi_sram_size = AXI_SRAM_SIZE;
    }

    /* VIP SRAM base physical address */
    if (0 == kdriver->vip_sram_size) {
        kdriver->vip_sram_size = VIP_SRAM_SIZE;
    }

    /* the size of VIP-SRAM */
    if (0 == kdriver->vip_sram_base) {
        kdriver->vip_sram_base = VIP_SRAM_BASE_ADDRESS;
    }

    for(core = 0; core < vpmdCORE_COUNT; core++) {
        /* IRQ line */
        if (0 == kdriver->irq_line[core]) {
            kdriver->irq_line[core] = pci_irq_vector(pdev, 0);
            printk("vipcore: core%d irq line=%d\n", core, kdriver->irq_line[core]);
        }

        /* VIP AHB register memory base physical address */
        if (0 == kdriver->vip_reg_phy[core]) {
            kdriver->vip_reg_phy[core] = bar[PCI_BAR_2].io_base + 0x6000000;
        }

        printk("vipcore: register address=0x%x\n", kdriver->vip_reg_phy[core]);

        /* VIP AHB register memory size */
        if (0 == kdriver->vip_reg_size[core]) {
            kdriver->vip_reg_size[core] = AHB_REGISTER_SIZE[core];
        }

        kdriver->vip_reg[core] = ioremap_nocache(kdriver->vip_reg_phy[core], kdriver->vip_reg_size[core]);
        printk("vipcore: register logical=0x%"PRPx"\n", kdriver->vip_reg[core]);

        #if defined (USE_LINUX_PCIE_DEVICE)
        /* the index of pci bars */
        if (0 == kdriver->pci_bars[core]) {
            kdriver->pci_bars[core] = PCI_BAR_2;
        }
        #endif

        {
        vip_uint32_t cid = readl((uint8_t *)kdriver->vip_reg[core] + 0x30);
        printk("vipcore: cid=0x%x\n", cid);
        }

    }

    /* core number for vip */
    if (0 == kdriver->core_count) {
        kdriver->core_count = vpmdCORE_COUNT;
    }
    /* caculate logical device */
    {
        vip_uint32_t  iter = 0, core_num = 0;
        for (iter = 0; iter < vpmdCORE_COUNT; iter++) {
            kdriver->device_core_number[iter] = LOGIC_DEVICES_WITH_CORE[iter];
            core_num += LOGIC_DEVICES_WITH_CORE[iter];
        }
        if (core_num < vpmdCORE_COUNT) {
            kdriver->core_count = core_num;
        }
    }

    /* the reserved memory is comes from DTS at boot time */
    kdriver->reserved_mem_type = GCKVIP_RESERVED_MEM_BOOT_TIME;
    kdriver->sys_heap_size = SYSTEM_MEMORY_HEAP_SIZE;

error_out:
    return ret;
}

/*
@brief release resources created in gckvip_drv_adjust_param if neeed.
        this function called before remove device/driver, so kdriver->pdev can be used.
@param kdriver, vip device object.
*/
vip_int32_t gckvip_drv_unadjust_param(
    gckvip_driver_t *kdriver
    )
{
    struct pci_dev *pdev = kdriver->pdev;
    vip_uint32_t core = 0;

#if USE_PCIE_MSI
    pci_free_irq_vectors(pdev);
#endif

    for(core = 0; core < vpmdCORE_COUNT; core++) {
        if (kdriver->vip_reg[core] != VIP_NULL) {
            iounmap((void*)kdriver->vip_reg[core]);
            kdriver->vip_reg[core] = VIP_NULL;
        }
    }

    return 0;
}

/*
@brief customer SOC initialize. power on and rise up clock for VIP hardware
@param kdriver, vip device object.
*/
vip_int32_t gckvip_drv_device_init(
    gckvip_driver_t *kdriver,
    vip_uint32_t core
    )
{
    PRINTK_I("gckvip_drv_device_init..power on core%d.\n", core);
    /* power on VIP */

    /* make sure power stable before this function return */
    gckvip_os_delay(2);

    return 0;
}

/*
@brief customer SOC un-initialize. power off and cut down clock.
@param kdriver, vip device object.
*/
vip_int32_t gckvip_drv_device_uninit(
    gckvip_driver_t *kdriver,
    vip_uint32_t core
    )
{
     PRINTK_I("gckvip_drv_device_uninit.. power off core%d\n", core);
     /* power off all cores */
     return 0;
}
