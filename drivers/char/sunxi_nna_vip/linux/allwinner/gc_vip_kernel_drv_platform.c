/****************************************************************************
*
*	The MIT License (MIT)
*
*	Copyright (c) 2017 - 2021 Vivante Corporation
*
*	Permission is hereby granted, free of charge, to any person obtaining a
*	copy of this software and associated documentation files (the "Software"),
*	to deal in the Software without restriction, including without limitation
*	the rights to use, copy, modify, merge, publish, distribute, sublicense,
*	and/or sell copies of the Software, and to permit persons to whom the
*	Software is furnished to do so, subject to the following conditions:
*
*	The above copyright notice and this permission notice shall be included in
*	all copies or substantial portions of the Software.
*
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*	DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*	The GPL License (GPL)
*
*	Copyright (C) 2017 - 2021 Vivante Corporation
*
*	This program is free software; you can redistribute it and/or
*	modify it under the terms of the GNU General Public License
*	as published by the Free Software Foundation; either version 2
*	of the License, or (at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program;
*
*****************************************************************************
*
*	Note: This software is released under dual MIT and GPL licenses. A
*	recipient may use this file under the terms of either the MIT license or
*	GPL License. If you wish to use only one license not the other, you can
*	indicate your decision by deleting one of the above license notices in your
*	version of this file.
*
*****************************************************************************/

#include "../../gc_vip_kernel_port.h"
#include "../../gc_vip_kernel.h"
#include "../gc_vip_kernel_drv.h"
#include "../../gc_vip_kernel_heap.h"
#include "gc_vip_platform_config.h"
#include <stdarg.h>
#include <linux/time.h>
#include <linux/mm.h>
#include <linux/clk.h>
#include <linux/devfreq.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/pm_opp.h>

typedef enum _clk_status {
	CLK_ON  = 0x0,
	CLK_OFF = 0x1,
} clk_status_e;

static vip_status_e set_vip_power_clk(struct platform_device *pdev, uint32_t status)
{
	__attribute__((unused)) struct device *dev = &(pdev->dev);
	switch (status) {
	case CLK_ON:
		printk("%s ON\n", __func__);
#ifdef CONFIG_SUNXI_PM_DOMAINS
		 pm_runtime_get_sync(dev);
#endif
		break;
	case CLK_OFF:
#ifdef CONFIG_SUNXI_PM_DOMAINS
		pm_runtime_put(dev);
#endif
		break;
	default:
		printk("Unsupport clk status");
		break;
	}
	return VIP_SUCCESS;
}

/*
* @brief convert CPU physical to VIP physical.
* @param cpu_physical. the physical address of CPU domain.
* */

vip_address_t gckvip_drv_get_vipphysical(
				    vip_address_t cpu_physical
					    )
{
	vip_address_t vip_hysical = cpu_physical;
	 return vip_hysical;
}

/*
 * @brief convert VIP physical to CPU physical.
 * @param vip_physical. the physical address of VIP domain.
 * */

vip_address_t gckvip_drv_get_cpuphysical(
				    vip_address_t vip_physical
					    )
{
	vip_address_t cpu_hysical = vip_physical;
	return cpu_hysical;
}



static const struct of_device_id vipcore_dev_match[] = {
	{
		.compatible = "allwinner,npu"
	},
	{ },
};

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
	struct device *dev = &(kdriver->pdev->dev);
	if (dev != NULL) {
		printk("%s %d SUCCESS\n", __func__, __LINE__);
#ifdef CONFIG_SUNXI_PM_DOMAINS
		pm_runtime_disable(dev);
#endif
	} else {
		printk("%s %d ERR\n", __func__, __LINE__);
	}
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
	struct device *dev = &(kdriver->pdev->dev);
	struct clk *mclk;
	struct clk *pclk;
	unsigned long rate, real_rate;
	unsigned int mod_clk;
	int ret;
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
		kdriver->irq_line[0] = IRQ_LINE_NUMBER[0];
	}

	/* VIP AHB register memory base physical address */
	if (0 == kdriver->vip_reg_phy[0]) {
		kdriver->vip_reg_phy[0] = AHB_REGISTER_BASE_ADDRESS[0];
	}

	/* VIP AHB register memory size */
	if (0 == kdriver->vip_reg_size[0]) {
		kdriver->vip_reg_size[0] = AHB_REGISTER_SIZE[0];
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
	if (0 == kdriver->vip_sram_base) {
		kdriver->vip_sram_base = VIP_SRAM_BASE_ADDRESS;
	}

	kdriver->device_core_number[0] = 1;
	kdriver->core_count = 1;

	kdriver->sys_heap_size = SYSTEM_MEMORY_HEAP_SIZE;
	if (dev != NULL) {
		printk("%s %d SUCCESS\n", __func__, __LINE__);
		mclk = of_clk_get(pdev->dev.of_node, 0);
		pclk = of_clk_get(pdev->dev.of_node, 1);
		ret = of_property_read_u32(pdev->dev.of_node, "clock-frequency", &mod_clk);
		if (!ret) {
			if (!IS_ERR_OR_NULL(pclk)) {
				rate = clk_round_rate(pclk, mod_clk);
				if (clk_set_rate(pclk, rate)) {
					pr_err("clk_set_rate:%ld  mod_clk:%d failed\n", rate, mod_clk);
					return -1;
				}
				real_rate = clk_get_rate(pclk);
				printk("Want set pclk rate(%d) support(%ld) real(%ld)\n", mod_clk, rate, real_rate);
				ret  = clk_set_parent(mclk, pclk);
				if (ret != 0) {
					pr_err("clk_set_parent() failed! return\n");
					return -1;
				}
			}
			rate = clk_round_rate(mclk, mod_clk);
			if (clk_set_rate(mclk, rate)) {
				pr_err("clk_set_rate:%ld  mod_clk:%d failed\n", rate, mod_clk);
				return -1;
			}
			real_rate = clk_get_rate(mclk);
			printk("Want set mclk rate(%d) support(%ld) real(%ld)\n", mod_clk, rate, real_rate);
		} else {
			if (!IS_ERR_OR_NULL(pclk)) {
				printk("%s rate:%d\n", __func__, mod_clk);
				ret  = clk_set_parent(mclk, pclk);
				if (ret != 0) {
					pr_err("clk_set_parent() failed! return\n");
					return -1;
				}
			}
		}
#ifdef CONFIG_SUNXI_PM_DOMAINS
		pm_runtime_enable(dev);
#else
		if (clk_prepare_enable(mclk)) {
			printk("Couldn't enable module clock\n");
			return -EBUSY;
		}
#endif
	} else {
		printk("%s %d ERR\n", __func__, __LINE__);
	}

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
	ret = set_vip_power_clk(pdev, CLK_ON);
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
	/*xiepeng disable clock test*/
	ret = 0;
	ret = set_vip_power_clk(pdev, CLK_OFF);

	return ret;
}
