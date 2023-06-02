////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (��MStar Confidential Information��) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// file    mdrv_gpio_io.c
/// @brief  GPIO Driver Interface
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
//#include "MsCommon.h"
//#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <asm/io.h>
#include <linux/semaphore.h>


#include <linux/err.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm.h>

//#include "mst_devid.h"

#include "mdrv_gpio_io.h"
#include "mhal_gpio_reg.h"
#include "mhal_gpio.h"
#include "mdrv_gpio.h"
#include "ms_platform.h"
#include "gpio.h"


//#include "mdrv_probe.h"

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
#define GPIO_DBG_ENABLE              0

#if GPIO_DBG_ENABLE
#define GPIO_DBG(_f)                 (_f)
#else
#define GPIO_DBG(_f)
#endif

#if 0
#define LINE_DBG()                  printf("GPIO %d\n", __LINE__)
#else
#define LINE_DBG()
#endif

#define GPIO_PRINT(fmt, args...)        //printk("\n[GPIO][%05d] " fmt, __LINE__, ## args)

typedef struct
{
    S32                          s32MajorGPIO;
    S32                          s32MinorGPIO;
    struct cdev                 cDevice;
    struct file_operations      GPIOFop;
    struct fasync_struct        *async_queue; /* asynchronous readers */
} GPIO_ModHandle_t;


#define MOD_GPIO_DEVICE_COUNT         1
#define MOD_GPIO_NAME                 "ModGPIO"

#define MDRV_NAME_GPIO                  "gpio"
#define MDRV_MAJOR_GPIO                 0x9b
#define MDRV_MINOR_GPIO                 0x00

//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------

static struct device *dev;

//static struct class *gpio_class;

static int mstar_gpio_request(struct gpio_chip *chip, unsigned offset)
{
    MDrv_GPIO_Pad_Set(offset);
    GPIO_PRINT("[mstar-gpio]mstar_gpio_request offset=%d\n",offset);
    return 0;
}

static void mstar_gpio_free(struct gpio_chip *chip, unsigned offset)
{
    GPIO_PRINT("[mstar-gpio]mstar_gpio_free\n");
}

static void mstar_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
    if(value==0)
        MDrv_GPIO_Pull_Low(offset);
    else
        MDrv_GPIO_Pull_High(offset);
    GPIO_PRINT("[mstar-gpio]mstar_gpio_set\n");
}

static int mstar_gpio_get(struct gpio_chip *chip, unsigned offset)
{
    GPIO_PRINT("[mstar-gpio]mstar_gpio_get\n");
    return MDrv_GPIO_Pad_Read(offset);
}

static int mstar_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
    GPIO_PRINT("[mstar-gpio]mstar_gpio_direction_input\n");
    MDrv_GPIO_Pad_Odn(offset);
    return 0;
}

static int mstar_gpio_direction_output(struct gpio_chip *chip, unsigned offset,
                    int value)
{
    MDrv_GPIO_Pad_Oen(offset);
    if(value==0)
        MDrv_GPIO_Pull_Low(offset);
    else
        MDrv_GPIO_Pull_High(offset);
    GPIO_PRINT("[mstar-gpio]mstar_gpio_direction_output\n");
    return 0;
}

static int mstar_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
    int irq = MDrv_GPIO_To_Irq(offset);
    //MDrv_Enable_GPIO_INT(offset);
    //GPIO_PRINT("[mstar-gpio]mstar_gpio_to_irq,but not set reg\n");
    pr_err("[Mstar GPIO] gpio(%d) to irq(%d)\n", offset, irq);
    return irq;
}

static struct gpio_chip mstar_gpio_chip = {
    .label          = "gpio",
    .request        = mstar_gpio_request,
    .free           = mstar_gpio_free,
    .direction_input    = mstar_gpio_direction_input,
    .get            = mstar_gpio_get,
    .direction_output   = mstar_gpio_direction_output,
    .set            = mstar_gpio_set,
    .to_irq         = mstar_gpio_to_irq,
    .base           = 0,
};


static const struct of_device_id mstar_gpio_of_match[] = {
    { .compatible = "mstar,gpio" },
    { },
};

static int mstar_gpio_probe(struct platform_device *pdev)
{
    const struct of_device_id *match;
    int ret;
/*
    struct resource *res;
    void __iomem *base;
    int gpionum;
    struct device_node  *node = pdev->dev.of_node;
*/
    dev = &pdev->dev;
    GPIO_PRINT("\n++[mstar-gpio]mstar_gpio_probe start\n");
/*
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    base = (void *)(IO_ADDRESS(res->start));
    gPadBaseAddr=(U32)base;
    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    base = (void *)(IO_ADDRESS(res->start));
    gChipBaseAddr=(U32)base;
    res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
    base = (void *)(IO_ADDRESS(res->start));
    gPmGpioBaseAddr=(U32)base;
    res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
    base = (void *)(IO_ADDRESS(res->start));
    gPmSleepBaseAddr=(U32)base;
    GPIO_PRINT("gPadBaseAddr=%x\n",gPadBaseAddr);
    GPIO_PRINT("gChipBaseAddr=%x\n",gChipBaseAddr);
    GPIO_PRINT("gPmGpioBaseAddr=%x\n",gPmGpioBaseAddr);
*/
    match = of_match_device(mstar_gpio_of_match, &pdev->dev);
    if (!match) {
        printk("Error:[mstar-gpio] No device match found\n");
        return -ENODEV;
    }
//    of_property_read_u32(node, "gpio-num", &gpionum);

    mstar_gpio_chip.ngpio = GPIO_NR;
    mstar_gpio_chip.of_node = pdev->dev.of_node;
    ret = gpiochip_add(&mstar_gpio_chip);
    if (ret < 0) {
        printk("[mstar-gpio]gpio_add err\n");
        return ret;
    }

    GPIO_PRINT("--[mstar-gpio]mstar_gpio_probe end\n");

    MDrv_GPIO_Init();
    printk(KERN_WARNING"GPIO: probe end");
    return 0;
}

static struct platform_driver mstar_gpio_driver = {
    .driver     = {
        .name   = "gpio",
        .owner  = THIS_MODULE,
        .of_match_table = mstar_gpio_of_match,
    },
    .probe      = mstar_gpio_probe,
};



void __mod_gpio_init(void)
{
    //GPIO chiptop initialization
    MDrv_GPIO_Init();
}


static int __init mstar_gpio_init(void)
{
    return platform_driver_register(&mstar_gpio_driver);
}
postcore_initcall(mstar_gpio_init);

MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("GPIO driver");
MODULE_LICENSE("GPL");
