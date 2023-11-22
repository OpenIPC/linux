/*
 * sstar_dwc3_of_simple.c- Sigmastar
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/io.h>
#include "core.h"
#include "io.h"
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <sstar-dwc3-of-simple.h>
#include "sstar-dwc3-debugfs.h"

extern unsigned long lx_mem_size;

static int dwc3_of_simple_sstar_parse_dt(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct regmap *drd_ctrl;
    int            x2a_off[4] = {0};
    int            ret        = 0;

    do
    {
        if (!device_property_read_u32_array(dev, "sstar,x2a-addr-off", x2a_off, ARRAY_SIZE(x2a_off)))
        {
#if IS_ENABLED(CONFIG_USB_DWC3_HOST)
            int i, reg;

            drd_ctrl = syscon_regmap_lookup_by_phandle(dev->of_node, "sstar,usb3drd-mac-syscon");

            if (IS_ERR(drd_ctrl))
            {
                dev_err(dev, "remap fail %ld\n", PTR_ERR(drd_ctrl));
                break;
            }
            reg = 0x50;
            for (i = 0; i < (sizeof x2a_off / sizeof(int)); i++)
            {
                dev_info(dev, "x2a-addr-off[0]: 0x%08x\n", x2a_off[i]);
                if ((ret = regmap_write(drd_ctrl, (reg << 2), x2a_off[i] & 0xffff))
                    || (ret = regmap_write(drd_ctrl, ((reg + 1) << 2), (x2a_off[i] >> 16) & 0x00ff)))
                {
                    dev_err(dev, "address mapping fail\n");
                    break;
                }
                reg += 2;
            }
            if (ret)
            {
                dev_err(dev, "reg = %d: address mapping fail(%d)\n", reg, ret);
                break;
            }
#endif
        }
        if (device_property_read_bool(dev, "sstar,force-mcm-enabled"))
        {
            drd_ctrl = syscon_regmap_lookup_by_phandle(dev->of_node, "sstar,usb3drd-mcm-syscon");
            if (!IS_ERR(drd_ctrl))
            {
                regmap_update_bits(drd_ctrl, (0x0 << 2), 0xff, 0x40);
            }
        }

        drd_ctrl = syscon_regmap_lookup_by_phandle(dev->of_node, "sstar,usb3drd-gp-syscon");
        if (IS_ERR(drd_ctrl))
        {
            dev_err(dev, "regmap lookup fail %ld\n", PTR_ERR(drd_ctrl));
            break;
        }
        ret = regmap_clear_bits(drd_ctrl, (0x21 << 2), BIT(1));
        if (ret)
        {
            dev_err(dev, "unmask interrupt fail(%d)\n", ret);
            break;
        }
        return 0;
    } while (0);

    return ret;
}

static int dwc3_of_simple_probe(struct platform_device *pdev)
{
    struct dwc3_of_simple * simple;
    struct device *         dev = &pdev->dev;
    struct device_node *    np  = dev->of_node, *child;
    struct platform_device *child_pdev;
    struct dwc3 *           dwc;

    int ret;
    dev_info(dev, "%s start\n", __func__);
    simple = devm_kzalloc(dev, sizeof(*simple), GFP_KERNEL);
    if (!simple)
    {
        dev_err(dev, "devm_kzalloc fail\n");
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, simple);
    simple->dev = dev;

    ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));
    if (ret)
    {
        return ret;
    }

#ifdef CONFIG_TEST_ALL_LINUX_MMU_MAP_ADDRESS
    if (of_device_is_compatible(np, "sstar,infinity7-dwc3"))
    {
        dma_direct_set_offset(dev, CONFIG_MIU0_BUS_BASE, CONFIG_TEST_ALL_LINUX_MMU_MAP_ADDRESS, lx_mem_size);
    }
    else
    {
        dma_direct_set_offset(dev, CONFIG_MIU0_BUS_BASE, CONFIG_TEST_ALL_LINUX_MMU_MAP_ADDRESS + (128 << 10),
                              lx_mem_size);
    }
#else
    dma_direct_set_offset(dev, CONFIG_MIU0_BUS_BASE, 0, lx_mem_size);
#endif

    // dma_direct_set_offset(dev, CONFIG_MIU0_BUS_BASE, CONFIG_TEST_ALL_LINUX_MMU_MAP_ADDRESS, lx_mem_size);

    ret = dwc3_of_simple_sstar_parse_dt(pdev);

    if (ret)
    {
        dev_err(dev, "dwc3_of_simple_sstar_parse_dt fail\n");
        return ret;
    }

    ret = clk_bulk_get_all(simple->dev, &simple->clks);
    if (ret < 0)
        goto err_clk_put;

    simple->num_clocks = ret;
    ret                = clk_bulk_prepare_enable(simple->num_clocks, simple->clks);
    if (ret)
        goto err_clk_put;

    ret = of_platform_populate(np, NULL, NULL, dev);
    if (ret)
        goto err_clk_put;

    child = of_get_child_by_name(np, "dwc3");
    if (!child)
    {
        dev_err(dev, "failed to find dwc3 core node\n");
        ret = -ENODEV;
        goto err_clk_put;
    }

    child_pdev = of_find_device_by_node(child);
    if (!child_pdev)
    {
        dev_err(dev, "failed to find dwc3 core device\n");
        ret = -ENODEV;
        goto err_clk_put;
    }

    dwc = platform_get_drvdata(child_pdev);
    if (!dwc)
    {
        dev_dbg(dev, "failed to get drvdata dwc3\n");
        ret = -EPROBE_DEFER;
        goto err_clk_put;
    }

    simple->dwc = dwc;
    pm_runtime_set_active(dev);
    pm_runtime_enable(dev);
    pm_runtime_get_sync(dev);
#if IS_ENABLED(CONFIG_USB_DWC3_HOST)
    sstar_dwc3_debugfs_init(simple);
#endif
    return 0;

err_clk_put:
    clk_bulk_disable_unprepare(simple->num_clocks, simple->clks);
    clk_bulk_put_all(simple->num_clocks, simple->clks);
    dev_err(dev, "%s fail\n", __func__);
    return ret;
}

static void __dwc3_of_simple_teardown(struct dwc3_of_simple *simple)
{
    of_platform_depopulate(simple->dev);

    clk_bulk_disable_unprepare(simple->num_clocks, simple->clks);
    clk_bulk_put_all(simple->num_clocks, simple->clks);
    simple->num_clocks = 0;

    pm_runtime_disable(simple->dev);
    pm_runtime_put_noidle(simple->dev);
    pm_runtime_set_suspended(simple->dev);
#if IS_ENABLED(CONFIG_USB_DWC3_HOST)
    sstar_dwc3_debugfs_exit(simple);
#endif
}

static int dwc3_of_simple_remove(struct platform_device *pdev)
{
    struct dwc3_of_simple *simple = platform_get_drvdata(pdev);
    __dwc3_of_simple_teardown(simple);

    return 0;
}

static void dwc3_of_simple_shutdown(struct platform_device *pdev)
{
    struct dwc3_of_simple *simple = platform_get_drvdata(pdev);

    __dwc3_of_simple_teardown(simple);
}

static int __maybe_unused dwc3_of_simple_runtime_suspend(struct device *dev)
{
    struct dwc3_of_simple *simple = dev_get_drvdata(dev);

    clk_bulk_disable(simple->num_clocks, simple->clks);

    return 0;
}

static int __maybe_unused dwc3_of_simple_runtime_resume(struct device *dev)
{
    struct dwc3_of_simple *simple = dev_get_drvdata(dev);

    return clk_bulk_enable(simple->num_clocks, simple->clks);
}

static int __maybe_unused dwc3_of_simple_suspend(struct device *dev)
{
    // struct dwc3_of_simple *simple = dev_get_drvdata(dev);
    return 0;
}

static int __maybe_unused dwc3_of_simple_resume(struct device *dev)
{
    // struct dwc3_of_simple *simple = dev_get_drvdata(dev);
    return 0;
}

static const struct dev_pm_ops dwc3_of_simple_dev_pm_ops = {
    SET_SYSTEM_SLEEP_PM_OPS(dwc3_of_simple_suspend, dwc3_of_simple_resume)
        SET_RUNTIME_PM_OPS(dwc3_of_simple_runtime_suspend, dwc3_of_simple_runtime_resume, NULL)};

static const struct of_device_id of_dwc3_simple_match[] = {
    {.compatible = "sstar,generic-dwc3"},
    {.compatible = "sstar,infinity7-dwc3"},
    {/* Sentinel */},
};
MODULE_DEVICE_TABLE(of, of_dwc3_simple_match);

static struct platform_driver dwc3_of_simple_driver = {
    .probe    = dwc3_of_simple_probe,
    .remove   = dwc3_of_simple_remove,
    .shutdown = dwc3_of_simple_shutdown,
    .driver =
        {
            .name           = "sstar-dwc3-of-simple",
            .of_match_table = of_dwc3_simple_match,
            .pm             = &dwc3_of_simple_dev_pm_ops,
        },
};

module_platform_driver(dwc3_of_simple_driver);
MODULE_SOFTDEP("pre: DWC3");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Sstar DesignWare USB3 OF Simple Glue Layer");
MODULE_AUTHOR("Raul Wang <raul.wang@isgmastar.com.tw>");
