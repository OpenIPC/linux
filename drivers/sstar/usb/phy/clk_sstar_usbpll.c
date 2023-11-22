/*
 * clk_sstar_usbpll.c- Sigmastar
 *
 * Copyright (c) [2019~2021] SigmaStar Technology.
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

#include <linux/platform_device.h>
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <io.h>

struct clk_sstar_usbpll
{
    void __iomem * base;
    struct device *dev;
    struct clk_hw  clk_hw;
};

static int sstar_usbpll_prepare(struct clk_hw *hw)
{
    struct clk_sstar_usbpll *usbpll = (struct clk_sstar_usbpll *)container_of(hw, struct clk_sstar_usbpll, clk_hw);
    struct device *          dev    = usbpll->dev;

    if (of_device_is_compatible(dev->of_node, "sstar,generic-usbpll"))
    {
#if 0
    	if (INREG16(usbpll->base + (0xe << 2)) & BIT(0))
    	{
        	return 0;
    	}
#endif
        CLRREG16(usbpll->base + (0x0 << 2), BIT(1));
        CLRREG16(usbpll->base + (0x0 << 2), BIT(4));
        CLRREG16(usbpll->base + (0x0 << 2), BIT(5));
        SETREG16(usbpll->base + (0xe << 2), BIT(0));
    }
    else
    {
        // upll_init.txt
        // enable UPLL; wait around 100us for PLL to be stable

        /*  setting scalar div for 240 MHZ
            reg[14:6] = 4'hb
            scalar_div[6:2] = 2, scalar_div[1:0] = 4, scalar_div = 2*4 = 8
        */
        OUTREG16(usbpll->base + (0x22 << 2), 0x2c0);

        // enable UPLL d2s & vco_en
        OUTREG8(usbpll->base + (0x27 << 2), 0xbc);

        // enable UPLL div clock
        // OUTREG16(base + (0x20 << 2), 0x8f00);
        // OUTREG16(base + (0x21 << 2), 0x022b);

        // UPLL power on
        OUTREG16(usbpll->base + (0x00 << 2), 0x00c0);
        OUTREG8(usbpll->base + (0x07 << 2), 0x11);
    }

    mdelay(1);
    dev_info(usbpll->dev, "%s\n", __func__);
    return 0;
}

static const struct clk_ops sstar_usbpll_ops = {
    .prepare = sstar_usbpll_prepare,
};

static int sstar_usbpll_probe(struct platform_device *pdev)
{
    struct clk_sstar_usbpll *usbpll;
    struct clk_init_data     init;
    const char *             clk_name;
    struct resource *        mem;
    int                      ret;

    usbpll = devm_kzalloc(&pdev->dev, sizeof(*usbpll), GFP_KERNEL);
    if (!usbpll)
        return -ENOMEM;

    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (IS_ERR(mem))
        return PTR_ERR(mem);
    dev_info(&pdev->dev, "resource[name: %s; start: 0x%08llx; end: 0x08%llx]\n", mem->name, mem->start, mem->end);

    usbpll->base = devm_ioremap_resource(&pdev->dev, mem);
    if (IS_ERR(usbpll->base))
        return PTR_ERR(usbpll->base);

    init.num_parents = of_clk_get_parent_count(pdev->dev.of_node);

    clk_name = pdev->dev.of_node->name;

    init.name         = clk_name;
    init.ops          = &sstar_usbpll_ops;
    init.parent_names = NULL;
    init.flags        = 0;

    usbpll->clk_hw.init = &init;
    ret                 = devm_clk_hw_register(&pdev->dev, &usbpll->clk_hw);
    if (ret)
        return ret;
    ret = of_clk_add_hw_provider(pdev->dev.of_node, of_clk_hw_simple_get, &usbpll->clk_hw);
    if (ret)
    {
        return ret;
    }
    usbpll->dev = &pdev->dev;
    dev_info(&pdev->dev, "sstar_usbpll_probe\n");
    return 0;
}

static int sstar_usbpll_remove(struct platform_device *pdev)
{
    of_clk_del_provider(pdev->dev.of_node);
    dev_info(&pdev->dev, "sstar_usbpll_remove\n");
    return 0;
}

static const struct of_device_id sstar_usbpll_ids[] = {
    {
        .compatible = "sstar,infinity7-usbpll",
    },
    {
        .compatible = "sstar,generic-usbpll",
    },
    {/* Sentinel */},
};

static struct platform_driver sstar_usbpll_driver = {
    .driver =
        {
            .name           = "sstar-usbpll",
            .of_match_table = sstar_usbpll_ids,
        },
    .probe  = sstar_usbpll_probe,
    .remove = sstar_usbpll_remove,
};

module_platform_driver(sstar_usbpll_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Driver for the Analog Devices' USB clock generator");
MODULE_SOFTDEP("post: usb3-phy");
