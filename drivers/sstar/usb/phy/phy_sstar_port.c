/*
 * phy_sstar_port.c- Sigmastar
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

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/module.h>
//#include <linux/resource.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
//#include <registers.h>
#include "phy_sstar_u3phy.h"

int sstar_port_init(struct device *dev, struct device_node *np, const struct phy_ops *port_ops,
                    struct sstar_phy_port *port)
{
    struct resource res;
    struct phy *    phy;

    void __iomem *reg;
    int           ret;

    dev_info(dev, "%s port init.\n", np->name);

    phy = devm_phy_create(dev, np, port_ops);
    if (IS_ERR(phy))
    {
        dev_err(dev, "failed to create phy\n");
        return PTR_ERR(phy);
    }

    ret = clk_bulk_get_all(&phy->dev, &port->clks);
    if (ret < 0)
        goto err_free;
    dev_info(&phy->dev, "%s, there are %d clocks\n", np->name, ret);

    port->num_clocks = ret;

    if (0 < ret)
    {
        ret = clk_bulk_prepare_enable(port->num_clocks, port->clks);
        if (ret)
            goto err_free;
    }

    ret = of_address_to_resource(np, 0, &res);
    if (ret)
    {
        dev_err(&phy->dev, "failed to get address resource(np-%s)\n", np->name);
        goto err_free;
    }
    dev_info(&phy->dev, "%s resource.start = 0x%llx resource.end = 0x%llx\n", np->name, res.start, res.end);
    reg = devm_ioremap_resource(&phy->dev, &res);
    if (IS_ERR(reg))
    {
        ret = PTR_ERR(reg);
        goto err_free;
    }

    port->reg = reg;

    phy_set_drvdata(phy, port);
    port->phy = phy;
    port->dev = &phy->dev;
    dev_info(&phy->dev, "%s resource init done.\n", np->name);
    return 0;

err_free:
    devm_phy_destroy(&phy->dev, phy);
    return ret;
}
EXPORT_SYMBOL(sstar_port_init);

int sstar_port_deinit(struct sstar_phy_port *port)
{
    clk_bulk_disable_unprepare(port->num_clocks, port->clks);
    clk_bulk_put_all(port->num_clocks, port->clks);
    port->num_clocks = 0;
    return 0;
}
EXPORT_SYMBOL(sstar_port_deinit);
