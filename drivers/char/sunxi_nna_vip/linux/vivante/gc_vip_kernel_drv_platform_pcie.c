/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2017 - 2022 Vivante Corporation
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
*    Copyright (C) 2017 - 2022 Vivante Corporation
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
#include "gc_vip_platform_config_pcie.h"

#define MAX_PCIE_BAR    6


#if defined (USE_LINUX_PCIE_DEVICE)
static const struct pci_device_id vivpcie_ids[] = {
  {
    .class = 0x000000,
    .class_mask = 0x000000,
    .vendor = 0x10ee,
    .device = 0x7012,
    .subvendor = PCI_ANY_ID,
    .subdevice = PCI_ANY_ID,
    .driver_data = 0
  }, { /* End: all zeroes */ }
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

/*
@brief convert CPU physical to VIP physical.
@param cpu_physical. the physical address of CPU domain.
*/
vip_address_t gckvip_drv_get_vipphysical(
    vip_address_t cpu_physical
    )
{
    vip_address_t vip_hysical = cpu_physical;

    return vip_hysical;
}

/*
@brief convert VIP physical to CPU physical.
@param vip_physical. the physical address of VIP domain.
*/
vip_address_t gckvip_drv_get_cpuphysical(
    vip_address_t vip_physical
    )
{
    vip_address_t cpu_hysical = vip_physical;

    return cpu_hysical;
}

/*
@brief 1. platfrom(vendor) initialize. prepare environmnet for running VIP hardware.
       2. initialzie linux platform device.
       3. this function should be called before probe device/driver.
          so many variables not initialize in kdriver. such as kdriver->pdev.
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
       have removed device/driver, so kdriver->pdev can't be used in this function.
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
        this function called after probe device/driver, so kdriver->pdev can be used.
@param kdriver, vip device object.
*/
vip_int32_t gckvip_drv_adjust_param(
    gckvip_driver_t *kdriver
    )
{
    vip_int32_t ret = 0;
    vip_uint32_t core = 0;
#if defined (USE_LINUX_PCIE_DEVICE)
    vip_uint32_t i = 0;
    vip_uint32_t read = 0;
    phy_address_t bar_addr[MAX_PCIE_BAR];
    vip_uint32_t bar_size[MAX_PCIE_BAR];
    struct pci_dev *pdev = kdriver->pdev;

    pci_read_config_dword(pdev, 0, &read);
    PRINTK("vipcore, device id 0x%x\n", read);
    PRINTK("vipcore, PCI irq num=%d\n", pdev->irq);
    pci_read_config_dword(pdev, 0x3C, &read);
    PRINTK("vipcore, PCI info 0x%08x\n", read);
    for (i = 0; i < MAX_PCIE_BAR; i++) {
        query_pcie_bar_nfo(pdev, &bar_addr[i], &bar_size[i], i);
        PRINTK("vipcore, BAR[%d] addr=0x%"PRIx64", size=0x%x\n", i, bar_addr[i], bar_size[i]);
        kdriver->pci_bar_base[i] = bar_addr[i];
        kdriver->pci_bar_size[i] = bar_size[i];
    }
#endif
    /* video memory heap base physical address */
    if (0 == kdriver->cpu_physical) {
        kdriver->cpu_physical = VIDEO_MEMORY_HEAP_BASE_ADDRESS;
    }

    /* video memory heap size */
    if (0 == kdriver->vip_memsize) {
        kdriver->vip_memsize = VIDEO_MEMORY_HEAP_SIZE;
    }

    /* AXI SRAM base physical address */
    if (0 == kdriver->axi_sram_base) {
        kdriver->axi_sram_base = AXI_SRAM_BASE_ADDRESS;
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
            kdriver->irq_line[core] = IRQ_LINE_NUMBER[core];
        }

        /* VIP AHB register memory base physical address */
        if (0 == kdriver->vip_reg_phy[core]) {
            kdriver->vip_reg_phy[core] = AHB_REGISTER_BASE_ADDRESS[core];
        }

        /* VIP AHB register memory size */
        if (0 == kdriver->vip_reg_size[core]) {
            kdriver->vip_reg_size[core] = AHB_REGISTER_SIZE[core];
        }

        #if defined (USE_LINUX_PCIE_DEVICE)
        /* the index of pci bars */
        if (0 == kdriver->pci_bars[core]) {
            kdriver->pci_bars[core] = PCI_BARS[core];
        }
        kdriver->vip_reg_phy[core] = kdriver->pci_bar_base[kdriver->pci_bars[core]] + kdriver->reg_offset[core];
        #endif
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
