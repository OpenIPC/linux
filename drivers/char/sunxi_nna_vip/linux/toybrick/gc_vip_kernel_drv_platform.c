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
#include "gc_vip_platform_config.h"
#include <stdarg.h>
#include <linux/time.h>
#include <linux/mm.h>
#include <linux/clk.h>
#include <linux/devfreq.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <soc/rockchip/rockchip_ipa.h>
#include <linux/pm_runtime.h>

static int enableClock(struct device *dev, const char *id)
{
    int ret = -1;
#if vpmdENABLE_DEBUG_LOG > 1
    int rate = 0;
#endif

    struct clk *clk_npu = devm_clk_get(dev, id);
    if (IS_ERR(clk_npu)) {
        PRINTK(KERN_WARNING "vipcore: cannot get %s\n", id);
        return -1;
    }

    ret = clk_prepare_enable(clk_npu);
    if (ret < 0) {
        PRINTK(KERN_WARNING "vipcore: cannot enable %s\n", id);
        return ret;
    }

#if vpmdENABLE_DEBUG_LOG > 1
    rate = clk_get_rate(clk_npu);
    if (!rate) {
        PRINTK(KERN_WARNING "get %s clk rate failed: %d\n", id, rate);
    } else {
        PRINTK(KERN_INFO "get %s clk rate = %d\n", id, rate);
    }
#endif

    return 0;
}

static int disableClock(struct device *dev, const char *id)
{
    struct clk *clk_npu = devm_clk_get(dev, id);
    if (IS_ERR(clk_npu)) {
        PRINTK(KERN_WARNING "vipcore: cannot get %s\n", id);
        return -1;
    }

    clk_disable_unprepare(clk_npu);

    return 0;
}

static vip_status_e set_vip_power_clk(struct platform_device *pdev)
{
    vip_int32_t ret = -1;
    pm_runtime_enable(&pdev->dev);
    ret = pm_runtime_get_sync(&pdev->dev);
    if (ret < 0) {
        PRINTK("vipcore: cannot get pm runtime, ret = %d\n", ret);
        return VIP_ERROR_IO;
    }

    ret = enableClock(&pdev->dev, "sclk_npu");
    if (ret < 0) {
        PRINTK("vipcore: failed to enable clock sclk\n");
        return VIP_ERROR_IO;
    }

    ret = enableClock(&pdev->dev, "aclk_npu");
    if (ret < 0) {
        PRINTK("vipcore: failed to enable clock aclk\n");
        return VIP_ERROR_IO;
    }

    ret = enableClock(&pdev->dev, "hclk_npu");
    if (ret < 0) {
        PRINTK("vipcore: failed to enable clock hclk\n");
        return VIP_ERROR_IO;
    }

    return VIP_SUCCESS;
}


static const struct of_device_id vipcore_dev_match[] = {
    {
        .compatible = "rockchip,npu"
    },
    { },
};

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
@brief platfrom(vendor) initialize. prepare environmnet for running VIP hardware.
@param pdrv vip drvier device object.
*/
vip_int32_t gckvip_drv_platform_init(
    gckvip_driver_t *kdriver
    )
{
    kdriver->pdrv->driver.of_match_table = vipcore_dev_match;

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
     you can overwrite insmod command line in gckvip_drv_adjust_param() function.
@param kdriver, vip device object.
*/
vip_int32_t gckvip_drv_adjust_param(
    gckvip_driver_t *kdriver
    )
{
    struct platform_device *pdev = kdriver->pdev;
    if (VIP_NULL == pdev) {
        PRINTK("platform device is NULL \n");
        return -1;
    }

    if (0 == kdriver->cpu_physical) {
        kdriver->cpu_physical = VIDEO_MEMORY_HEAP_BASE_ADDRESS;
    }
    if (0 == kdriver->vip_memsize) {
        kdriver->vip_memsize = VIDEO_MEMORY_HEAP_SIZE;
    }

    kdriver->irq_line[0] = platform_get_irq(pdev, 0);
    PRINTK("vipcore irq number is %d.\n", kdriver->irq_line[0]);
    if (kdriver->irq_line[0] < 0) {
        PRINTK("get vipcore irq resource error\n");
        return -1;
    }
    /* IRQ line */
    if (0 == kdriver->irq_line[0]) {
        kdriver->irq_line[0] = IRQ_LINE_NUMBER;
    }

    /* VIP AHB register memory base physical address */
    if (0 == kdriver->vip_reg_phy[0]) {
        kdriver->vip_reg_phy[0] = AHB_REGISTER_BASE_ADDRESS;
    }

    /* VIP AHB register memory size */
    if (0 == kdriver->vip_reg_size[0]) {
        kdriver->vip_reg_size[0] = AHB_REGISTER_SIZE;
    }

    if (0 == kdriver->axi_sram_base) {
        kdriver->axi_sram_base = AXI_SRAM_BASE_ADDRESS;
    }
    if (0 == kdriver->axi_sram_size) {
        kdriver->axi_sram_size = AXI_SRAM_SIZE;
    }
    if (0 == kdriver->vip_sram_size) {
        kdriver->vip_sram_size = VIP_SRAM_SIZE;
    }

    kdriver->device_core_number[0] = 1;
    kdriver->core_count = 1;

    kdriver->sys_heap_size = SYSTEM_MEMORY_HEAP_SIZE;

    return 0;
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
    vip_int32_t ret = 0;
    struct platform_device *pdev = kdriver->pdev;
    PRINTK("vipcore, device init..\n");
    if (VIP_NULL == pdev) {
        PRINTK("platform device is NULL \n");
        return -1;
    }

    /* power on VIP and make sure power stable before this function return */
    ret = set_vip_power_clk(pdev);
    gckvip_os_delay(2);

    return ret;
}

/*
@brief customer SOC un-initialize. power off
@param kdriver, vip device object.
*/
vip_int32_t gckvip_drv_device_uninit(
    gckvip_driver_t *kdriver,
    vip_uint32_t core
    )
{
    struct platform_device *pdev = kdriver->pdev;
    vip_int32_t ret = -1;
    PRINTK("vipcore, device un-init..\n");
    if (VIP_NULL == pdev) {
        PRINTK("platform device is NULL \n");
        return -1;
    }

    disableClock(&pdev->dev, "sclk_npu");
    disableClock(&pdev->dev, "aclk_npu");
    disableClock(&pdev->dev, "hclk_npu");

    ret = pm_runtime_put(&pdev->dev);
    if (ret < 0) {
        PRINTK(KERN_WARNING "vipcore: cannot put pm runtime, ret = %d\n", ret);
        return ret;
    }

    pm_runtime_disable(&pdev->dev);

    return ret;
}
