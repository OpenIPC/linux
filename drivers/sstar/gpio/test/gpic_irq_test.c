/*
 * gpic_irq_test.c- Sigmastar
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

///////////////////////////////////////////////////////////////////////////////
/// @file      gpio_irq_test.c
/// @brief     GPIO IRQ Test Code for Linux Kernel Space
///////////////////////////////////////////////////////////////////////////////

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/types.h>
#include <linux/sched.h>
#include "cam_os_wrapper.h"
#include "mdrv_gpic.h"
#include <linux/gpio.h>
#include <linux/interrupt.h>

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("SStar GPIC IRQ Test");
MODULE_LICENSE("GPL");

int port = 0;
module_param(port, int, 0);
int pin = 0;
module_param(pin, int, 0);
int target = 0;
module_param(target, int, 0);

void gpic_test_isr(void *para)
{
    u16             count;
    void *          handler;
    CamOsTimespec_t ptRes;
    CamOsGetMonotonicTime(&ptRes);
    handler = camdriver_gpic_get_handler(port);
    if (!handler)
        printk("get gpic handler err\r\n");
    if (camdriver_gpic_get_count(handler, &count))
        printk("get gpic count err\r\n");
    printk("%s  [%lld.%09lld] count is %d\n", __func__, ptRes.nSec, ptRes.nNanoSec, count);
}

static int __init GpicIrqTestInit(void)
{
    void *    handler;
    GpicCfg_t cfg;

    handler = camdriver_gpic_get_handler(port);
    if (!handler)
        return -1;

    cfg.pad      = pin;
    cfg.dir      = GPIC_UP_COUNT;
    cfg.edge     = GPIC_FAILING_EDGE;
    cfg.init     = 0;
    cfg.target   = target;
    cfg.callback = gpic_test_isr;
    camdriver_gpic_config(handler, &cfg);
    camdriver_gpic_enable(handler);

    return 0;
}

static void __exit GpicIrqTestExit(void)
{
    void *handler;
    handler = camdriver_gpic_get_handler(port);
    if (!handler)
        return;
    camdriver_gpic_disable(handler);
}

module_init(GpicIrqTestInit);
module_exit(GpicIrqTestExit);
