/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

 Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
Sigmastar Technology Corp. and be kept in strict confidence
(Sigmastar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of Sigmastar Confidential
Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
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
#include <linux/gpio.h>
#include <linux/interrupt.h>

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("SStar GPIO IRQ Test");
MODULE_LICENSE("GPL");

int pin = 0;
module_param(pin, int, 0);
int level = 0;
module_param(level, int, 0);

s32 gpio_set_success = 0;
u32 gpio_irq_num = 0;

irqreturn_t gpio_test_isr(int irq, void *dev_instance)
{
    CamOsTimespec_t ptRes;
    CamOsGetMonotonicTime(&ptRes);
    printk("%s  [%d.%09d]\n", __func__, ptRes.nSec, ptRes.nNanoSec);

    return IRQ_NONE;
}

static int __init GpioIrqTestInit(void)
{
    gpio_set_success = 0;

    if(gpio_request(pin, "gpio_irq_test") < 0)
    {
        printk("request gpio[%d] failed...\n", pin);
        return 0;
    }

    if (gpio_direction_input(pin) < 0) {
        printk("gpio_direction_input[%d] failed...\n", pin);
        return 0;
    }

    gpio_irq_num = gpio_to_irq(pin);
    if (request_irq(gpio_irq_num, gpio_test_isr, (level==0)? IRQF_TRIGGER_FALLING : IRQF_TRIGGER_RISING, "PAD", NULL))
    {
        printk(KERN_ERR"cannot allocate irq\n");
        return 0;
    }

    gpio_set_success = 1;

    return 0;
}

static void __exit GpioIrqTestExit(void)
{
    if (gpio_set_success)
        free_irq(gpio_irq_num, NULL);
    gpio_free(pin);
}

module_init(GpioIrqTestInit);
module_exit(GpioIrqTestExit);
