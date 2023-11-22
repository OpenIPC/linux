/*
 * mdrv_wakeup.c- Sigmastar
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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/miscdevice.h> /* for MODULE_ALIAS_MISCDEV */
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of_platform.h>
#include "ms_types.h"
#include "ms_platform.h"
#include "mdrv_gpio_io.h"
#include "mdrv_gpio.h"
#include "ms_platform.h"
#include "mdrv_wakeup.h"
#include "padmux.h"
#include <linux/of_irq.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>
#include "irqs.h"

#define TIMERTEST 0

struct mdrv_wakeup_info
{
    unsigned int wakeSource[32];
    unsigned int gate_xtal;
    unsigned int interrupts;
    unsigned int source_num;
};

struct mdrv_wakeup_info info;

static const struct of_device_id wakeup_gpio_of_match[] = {
    {.compatible = "sstar,wakeup-gpio"},
    {},
};

static irqreturn_t sstar_gpio_wake_interrupt(int irq, void *dev_id)
{
    return IRQ_HANDLED;
}
#if TIMERTEST
irqreturn_t timer_int_service(int irq, void *dummy)
{
    printk("timer    0_int \r\n");
    return IRQ_HANDLED;
}
#endif

static int sstar_wakeup_probe(struct platform_device *pdev)
{
    const struct of_device_id *match;
    int                        ret, i, j;
    struct resource *          res;

#if TIMERTEST
    S32                 s32_ret = -1;
    struct device_node *intr_node;
    int                 virq = -1;
    struct irq_domain * intr_domain;
    struct irq_fwspec   fwspec;

    intr_node   = of_find_compatible_node(NULL, NULL, "sstar,main-intc");
    intr_domain = irq_find_host(intr_node);
#endif

    match = of_match_device(wakeup_gpio_of_match, &pdev->dev);

    if (!match)
    {
        printk("Err:[gpio] No dev found\n");
        return -ENODEV;
    }
    device_init_wakeup(&pdev->dev, 1);
    i = of_property_read_u32(pdev->dev.of_node, "gate_xtal", &info.gate_xtal);
    j = of_property_read_u32(pdev->dev.of_node, "source_num", &info.source_num);

    if (i | j | of_property_read_u32_array(pdev->dev.of_node, "wakeup_source", info.wakeSource, info.source_num))
    {
        printk("Err: Could not get dts [wakeup] option!\n");
        return 1;
    }
    printk("[wakeup source] HW gate_xtal:%d SourceNum:%d\r\n", info.gate_xtal, info.source_num);
    printk("[wakeup source] WakeupSource:");

    for (i = 0; i < info.source_num; i++)
    {
        printk("%d ", info.wakeSource[i]);
    }
    printk("\n");

    if (info.gate_xtal)
    {
        res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
        if (res == NULL)
        {
            printk("####no irq resource defined\n");
            ret = -ENODEV;
        }

        ret = request_irq(res->start, &sstar_gpio_wake_interrupt, IRQF_TRIGGER_RISING | IRQF_SHARED, "wakeup", pdev);
        if (ret)
        {
            printk(KERN_ERR " No interrupt for wake\n");
        }
        enable_irq_wake(res->start);
    }
    else
    {
        for (i = 0; i < info.source_num; i++)
        {
            ret = gpio_request(info.wakeSource[i], "wake-source");

            if (ret < 0)
            {
                printk(KERN_ERR " Could not request wake GPIO: %i\n", info.wakeSource[i]);
                return -ENODEV;
            }
            ret = request_irq(gpio_to_irq(info.wakeSource[i]), &sstar_gpio_wake_interrupt,
                              IRQF_TRIGGER_RISING | IRQF_SHARED, "wakeup_source", pdev);

            if (ret)
            {
                gpio_free(info.wakeSource[i]);
                printk(KERN_ERR " No interrupt for wake GPIO: %i\n", info.wakeSource[i]);
                return -ENODEV;
            }
            gpio_direction_input(info.wakeSource[i]);
            enable_irq_wake(gpio_to_irq(info.wakeSource[i]));
        }
    }
#if TIMERTEST

    if (!intr_domain)
    {
        printk(KERN_ERR "timer_mod : irq_find_host fail \n");
        return -ENXIO;
    }

    {
        fwspec.param_count = 3;
        fwspec.param[0]    = GIC_SPI;
        fwspec.param[1]    = INT_FIQ_TIMER_0;
        fwspec.param[2]    = IRQ_TYPE_NONE;
        fwspec.fwnode      = of_node_to_fwnode(intr_node);
        virq               = irq_create_fwspec_mapping(&fwspec);
        s32_ret = request_irq(virq, timer_int_service, IRQF_SHARED, "timer_int_service", (void *)timer_int_service);
        if (s32_ret)
        {
            printk(KERN_ERR "timer_modules Err: request_irq fail %d \n", s32_ret);
        }
        else
        {
            // TimerDbg(KERN_ERR"timer_modules: request_irq successful \n");
        }
        *(unsigned short volatile *)(0xFD00604C) = 0x200; // set timer0 max
        *(unsigned short volatile *)(0xFD006040) = 0x101; // set timer0 enable_int/
        enable_irq_wake(virq);
    }
#endif
    return ret;
}
extern U8 MDrv_GPIO_PadVal_Set(U8 u8IndexGPIO, U32 u32PadMode);

static int sstar_wakeup_suspend(struct platform_device *dev, pm_message_t state)
{
    struct resource *res;
    int              i;

    if (info.gate_xtal) // USB&GPIO
    {
        if ((info.gate_xtal == 2) || (info.gate_xtal == 1))
        { // GPIO
            for (i = 0; i < info.source_num; i++)
            {
                MDrv_GPIO_PadVal_Set(info.wakeSource[i], PINMUX_FOR_GPIO_MODE);
                gpio_direction_input(info.wakeSource[i]);
                //                OUTREGMSK16(BASE_REG_CHIPTOP_PA + REG_WAKEUP_XTAL_GPIO_EN+
                //                ((info.wakeSource[i]/16)<<2), (1 << (info.wakeSource[i] % 16)), (1 <<
                //                (info.wakeSource[i] % 16)));
            }
        }
        res = platform_get_resource(dev, IORESOURCE_IRQ, 0);
        enable_irq_wake(res->start);
        /*Enable wakeup xtal interrupt*/
        OUTREGMSK16(BASE_REG_CHIPTOP_PA + XTAL_GATE, REG_WAKEUP_XTAL_IRQ_EN, 0x80);

        if ((info.gate_xtal == 1) || (info.gate_xtal == 3))                              // USB
            OUTREGMSK16(BASE_REG_CHIPTOP_PA + XTAL_GATE, REG_WAKEUP_XTAL_USB_EN, 0x100); /*ToDo USB en*/

        OUTREGMSK16(BASE_REG_CHIPTOP_PA + XTAL_GATE, REG_WAKEUP_XTAL_IRQ_CLR, 0x20);
    }
    else // normal flow
    {
        for (i = 0; i < info.source_num; i++)
        {
            MDrv_GPIO_PadVal_Set(info.wakeSource[i], PINMUX_FOR_GPIO_MODE);
            gpio_direction_input(info.wakeSource[i]);
            enable_irq_wake(gpio_to_irq(info.wakeSource[i]));
        }
    }
    return 0;
}

static int sstar_wakeup_resume(struct platform_device *dev)
{
    struct resource *res;
    int              i;

    if (info.gate_xtal)
    {
        OUTREGMSK16(BASE_REG_CHIPTOP_PA + XTAL_GATE, REG_WAKEUP_XTAL_IRQ_CLR, 0x20);
        res = platform_get_resource(dev, IORESOURCE_IRQ, 0);
        disable_irq(res->start); /*pm.c resume_irq.c: if (desc->istate & IRQS_SUSPENDED)*/
        enable_irq(res->start);
    }
    else
    {
        for (i = 0; i < info.source_num; i++)
        {
            disable_irq(gpio_to_irq(info.wakeSource[i])); /*pm.c resume_irq.c: if (desc->istate & IRQS_SUSPENDED)*/
            enable_irq(gpio_to_irq(info.wakeSource[i]));
        }
    }
    return 0;
}

static struct platform_driver wakeupdriver = {
    .driver =
        {
            .name           = "wakeup",
            .owner          = THIS_MODULE,
            .of_match_table = wakeup_gpio_of_match,
        },
    .probe = sstar_wakeup_probe,
#ifdef CONFIG_PM
    .suspend = sstar_wakeup_suspend,
    .resume  = sstar_wakeup_resume,
#endif
};
module_platform_driver(wakeupdriver);

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("sstar wakeup Device Driver");
MODULE_LICENSE("GPL");
