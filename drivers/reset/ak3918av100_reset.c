
/*
 * ak3918av100 reset driver
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

#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <mach/map.h>

#define BANK_NUM (2)

struct ak_reset {
    struct reset_controller_dev rcdev;
    void __iomem* base[BANK_NUM];
    spinlock_t lock;
};

/*
 * @BRIEF        ak_reset_update
 * @PARAM[in]    *rcdev
 * @PARAM[in]    id
 * @PARAM[in]    assert
 * @RETURN       int
 * @RETVAL       sucess:0
 */
static int ak_reset_update(
    struct reset_controller_dev* rcdev, unsigned long id, bool assert)
{
    struct ak_reset* ak_reset = container_of(rcdev, struct ak_reset, rcdev);
    unsigned long flags;
    int bank = id / BITS_PER_LONG;
    int offset = id % BITS_PER_LONG;
    u32 val;

    spin_lock_irqsave(&ak_reset->lock, flags);
    val = __raw_readl(ak_reset->base[bank]);

    if (assert)
        val |= BIT(offset);
    else
        val &= ~BIT(offset);

    __raw_writel(val, ak_reset->base[bank]);

    val = __raw_readl(AK_VA_SYSCTRL); // flush 0x08000000
    spin_unlock_irqrestore(&ak_reset->lock, flags);

    pr_debug(
        "exit %s: assert = %d, id = %lu, bank =%d, offset =%d, 0x%p: 0x%x\n",
        __func__, assert, id, bank, offset, ak_reset->base[bank],
        __raw_readl(ak_reset->base[bank]));

    return 0;
}

/*
 * @BRIEF        ak_reset_assert
 * @PARAM[in]    *rcdev
 * @PARAM[in]    id
 * @RETURN       int
 * @RETVAL       ak_reset_update(rcdev, id, true)
 */
static int ak_reset_assert(struct reset_controller_dev* rcdev, unsigned long id)
{
    return ak_reset_update(rcdev, id, true);
}

static int ak_reset_deassert(
    struct reset_controller_dev* rcdev, unsigned long id)
{
    return ak_reset_update(rcdev, id, false);
}

static int ak_reset_status(struct reset_controller_dev* rcdev, unsigned long id)
{
    struct ak_reset* ak_reset = container_of(rcdev, struct ak_reset, rcdev);
    int bank = id / BITS_PER_LONG;
    int offset = id % BITS_PER_LONG;
    u32 val;

    val = __raw_readl(ak_reset->base[bank]);

    return !!(val & BIT(offset));
}

static struct reset_control_ops ak_reset_ops = {
    .assert = ak_reset_assert,
    .deassert = ak_reset_deassert,
    .status = ak_reset_status,
};

static int ak_reset_probe(struct platform_device* pdev)
{
    struct ak_reset* ak_reset;
    struct resource* res;
    int index;

    ak_reset = devm_kzalloc(&pdev->dev, sizeof(*ak_reset), GFP_KERNEL);
    if (!ak_reset)
        return -ENOMEM;

    platform_set_drvdata(pdev, ak_reset);

    for (index = 0; index < BANK_NUM; index++) {
        res = platform_get_resource(pdev, IORESOURCE_MEM, index);
        ak_reset->base[index] = devm_ioremap_resource(&pdev->dev, res);
        if (IS_ERR(ak_reset->base[index]))
            return PTR_ERR(ak_reset->base[index]);
    }

    spin_lock_init(&ak_reset->lock);
    ak_reset->rcdev.ops = &ak_reset_ops;
    ak_reset->rcdev.owner = THIS_MODULE;
    ak_reset->rcdev.of_node = pdev->dev.of_node;
    ak_reset->rcdev.of_reset_n_cells = 1;
    ak_reset->rcdev.nr_resets = BANK_NUM
        * BITS_PER_LONG; // resource_size(res) / 4 * BITS_PER_LONG; //64;

    return reset_controller_register(&ak_reset->rcdev);
}

static int ak_reset_remove(struct platform_device* pdev)
{
    struct ak_reset* ak_reset = platform_get_drvdata(pdev);

    reset_controller_unregister(&ak_reset->rcdev);

    return 0;
}

static const struct of_device_id ak_reset_dt_ids[] = {
    {
        .compatible = "anyka,ak3918av100-reset",
    },
    {
        .compatible = "anyka,ak3918ev300l-reset",
    },
    {
        .compatible = "anyka,ak3918av130-reset",
    },
    {
        .compatible = "anyka,km01a-reset",
    },
    {},
};
MODULE_DEVICE_TABLE(of, ak_reset_dt_ids);

static struct platform_driver ak_reset_driver = {
    .probe = ak_reset_probe,
    .remove = ak_reset_remove,
    .driver = {
        .name = "ak-reset",
        .of_match_table = ak_reset_dt_ids,
    },
};
#ifdef CONFIG_SYS_FAST_LAUNCH
module1_platform_driver(ak_reset_driver);
#else
module_platform_driver(ak_reset_driver);
#endif

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Anyka reset controller driver");
MODULE_AUTHOR("Anyka Microelectronic");
MODULE_VERSION("V2.0.00");
