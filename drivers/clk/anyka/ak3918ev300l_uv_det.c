/*
 * ak3918av100 uv_sen driver
 *
 * Copyright (C) 2021 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
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
 */

#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <mach/map.h>

#define AK_VA_UV_SEN_REG (AK_VA_SYSCTRL + 0x00DC)

static void __init of_ak_uv_det_pd(struct device_node* np)
{
    int ret;
    u32 regval;
    unsigned int threshold = 0;
    const char* threshold_table[]
        = { "2.79V ~ 3.0V", "2.65V ~ 2.86V", "2.55V ~ 2.78V", "2.46V ~ 2.68V" };

    ret = of_property_read_u32(np, "threshold", &threshold);
    if (ret < 0) {
        pr_err("Could not read threshold property of_uv_det_pd\n");
        return;
    }

    /*
     * PLL1 Bandwidth Adjustment Configuration (Address: 0x0800,00DC)
     * [2:1] UV_SEN
     * The chip reset(high to low level) threshold and the corresponding reset
     * release(low to high level) threshold 0: 2.79V ~ 3.0V 1: 2.65V ~ 2.86V
     *    2: 2.55V ~ 2.78V
     *    3: 2.46V ~ 2.68V
     */

    /* check the threshold range */
    if (threshold > 3) {
        pr_err("threshold error!\n");
        return;
    }
    pr_info("threshold: %s\n", threshold_table[threshold]);

    /* write to register */
    regval = __raw_readl(AK_VA_UV_SEN_REG);
    regval &= ~(0x3 << 1);
    regval |= threshold << 1;
    __raw_writel(regval, AK_VA_UV_SEN_REG);
}

CLK_OF_DECLARE(
    ak3918ev300l_uv_det, "anyka,ak3918ev300l-uv_det_pd", of_ak_uv_det_pd);
