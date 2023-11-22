/*
 * phy_sstar_u3phy.c- Sigmastar
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
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/usb/phy.h>
#include "phy_sstar_u3phy.h"
#include <io.h>

extern int  sstar_utmi_port_init(struct device *dev, struct sstar_phy_port *phy_port, struct device_node *np);
extern int  sstar_pipe_port_init(struct device *dev, struct sstar_phy_port *phy_port, struct device_node *np);
extern void sstar_utmi_port_remove(void *data);
extern void sstar_pipe_port_remove(void *data);

static int sstar_u3phy_on_disconnect(struct usb_phy *usb_phy, enum usb_device_speed speed)
{
    struct sstar_u3phy *   u3phy = container_of(usb_phy, struct sstar_u3phy, usb_phy);
    struct sstar_phy_port *u3phy_port;

    int index;

    dev_info(u3phy->dev, "%s device has disconnected\n", (speed == USB_SPEED_SUPER) ? "U3" : "UW/U2/U1.1/U1");

    if (speed <= USB_SPEED_HIGH)
    {
        for (index = 0; U3PHY_PORT_NUM > index; index++)
        {
            u3phy_port = &u3phy->u3phy_port[index];

            if (USB_SPEED_SUPER > u3phy_port->speed)
            {
                phy_reset(u3phy_port->phy);
                phy_power_off(u3phy_port->phy);
                phy_init(u3phy_port->phy);
                phy_power_on(u3phy_port->phy);
                SETREG16(u3phy_port->reg + (0x16 << 2), 0x0010);
                SETREG16(u3phy_port->reg + (0x17 << 2), 0x0000);
                OUTREG8(u3phy_port->reg + (0x15 << 2), 0x80);
            }
        }
    }
#if 0
    if (speed == USB_SPEED_SUPER)
        atomic_notifier_call_chain(&usb_phy->notifier, 0, NULL);
#endif
    return 0;
}

static int sstar_u3phy_on_connect(struct usb_phy *usb_phy, enum usb_device_speed speed)
{
    struct sstar_u3phy *   u3phy = container_of(usb_phy, struct sstar_u3phy, usb_phy);
    struct sstar_phy_port *u3phy_port;

    int index;

    dev_info(u3phy->dev, "%s device has connected\n", (speed == USB_SPEED_SUPER) ? "U3" : "UW/U2/U1.1/U1");

    if (speed <= USB_SPEED_HIGH)
    {
        for (index = 0; U3PHY_PORT_NUM > index; index++)
        {
            u3phy_port = &u3phy->u3phy_port[index];

            if (USB_SPEED_SUPER > u3phy_port->speed)
            {
                SETREG16(u3phy_port->reg + (0x16 << 2), 0x210);
                SETREG16(u3phy_port->reg + (0x17 << 2), 0x8100);
                OUTREG8(u3phy_port->reg + (0x15 << 2), 0x62);
            }
        }
    }
#if 0
    if (speed == USB_SPEED_SUPER)
        atomic_notifier_call_chain(&usb_phy->notifier, 0, NULL);
#endif
    return 0;
}

__maybe_unused static int sstar_u3phy_on_init(struct usb_phy *usb_phy)
{
    struct sstar_u3phy *u3phy = container_of(usb_phy, struct sstar_u3phy, usb_phy);
    dev_info(u3phy->dev, "%s\n", __func__);
    return 0;
}

__maybe_unused static void sstar_u3phy_on_shutdown(struct usb_phy *usb_phy)
{
    struct sstar_u3phy *u3phy = container_of(usb_phy, struct sstar_u3phy, usb_phy);
    dev_info(u3phy->dev, "%s\n", __func__);
    return;
}

static int sstar_u3phy_port_init(struct device *dev, struct sstar_phy_port *u3phy_port, struct device_node *np)
{
    int ret;

    dev_info(dev, "u3phy port initialize, name: %s.\n", np->name);

    if (!of_prop_cmp("pipe", np->name))
    {
        ret = sstar_pipe_port_init(dev, u3phy_port, np);
        if (ret)
            return ret;

        ret = devm_add_action_or_reset(dev, sstar_pipe_port_remove, u3phy_port);
    }
    else
    {
        ret = sstar_utmi_port_init(dev, u3phy_port, np);
        if (ret)
            return ret;

        ret = devm_add_action_or_reset(dev, sstar_utmi_port_remove, u3phy_port);
    }

    return ret;
}

struct phy_data generic_u3phy_data_v1 = {
    .revision = 1,
};

static const struct of_device_id sstar_u3phy_dt_ids[] = {
    {.compatible = "sstar,infinity7-u3phy"},
    {.compatible = "sstar,generic-u3phy"},
    {
        .compatible = "sstar,generic-u3phy-v1.0",
        .data       = &generic_u3phy_data_v1,
    },
    {/* Sentinel */},
};

static int sstar_u3phy_probe(struct platform_device *pdev)
{
    struct device *            dev = &pdev->dev;
    struct device_node *       np  = dev->of_node;
    struct device_node *       child_np;
    struct sstar_u3phy *       pst_u3phy;
    struct phy_provider *      provider;
    const struct of_device_id *of_match_id;

    unsigned int index;
    int          ret;

    dev_info(dev, "sstar_u3phy_probe\n");
    of_match_id = of_match_device(sstar_u3phy_dt_ids, dev);
    if (!of_match_id)
    {
        dev_err(dev, "missing match\n");
        return -EINVAL;
    }

    pst_u3phy = devm_kzalloc(dev, sizeof(*pst_u3phy), GFP_KERNEL);

    if (!pst_u3phy)
    {
        dev_err(dev, "devm_kzalloc failed for u3phy structure.\n");
        return -ENOMEM;
    }

    index = 0;
    for_each_available_child_of_node(np, child_np)
    {
        struct sstar_phy_port *u3phy_port = &pst_u3phy->u3phy_port[index];

        u3phy_port->index    = index;
        u3phy_port->phy_data = (struct phy_data *)of_match_id->data;
        ret                  = sstar_u3phy_port_init(dev, u3phy_port, child_np);

        if (ret)
        {
            dev_err(dev, "u3phy port init failed,ret(%d)\n", ret);
            goto put_child;
        }

        /* to prevent out of boundary */
        if (++index >= U3PHY_PORT_NUM)
            break;
    }

    provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);

    if (IS_ERR_OR_NULL(provider))
        goto put_child;

    pst_u3phy->dev                       = dev;
    pst_u3phy->usb_phy.dev               = pst_u3phy->dev;
    pst_u3phy->usb_phy.notify_connect    = sstar_u3phy_on_connect;
    pst_u3phy->usb_phy.notify_disconnect = sstar_u3phy_on_disconnect;
    usb_add_phy(&pst_u3phy->usb_phy, USB_PHY_TYPE_USB3);
    ATOMIC_INIT_NOTIFIER_HEAD(&pst_u3phy->usb_phy.notifier);
    platform_set_drvdata(pdev, pst_u3phy);
    return 0;

put_child:
    of_node_put(child_np);
    devm_kfree(dev, pst_u3phy);
    return ret;
}

static int sstar_u3phy_remove(struct platform_device *pdev)
{
    struct sstar_u3phy *pst_u3phy = platform_get_drvdata(pdev);
    usb_remove_phy(&pst_u3phy->usb_phy);

    return 0;
}

static struct platform_driver sstar_u3phy_driver = {
    .probe  = sstar_u3phy_probe,
    .remove = sstar_u3phy_remove,
    .driver =
        {
            .name           = "sstar-u3phy",
            .of_match_table = sstar_u3phy_dt_ids,
        },
};

module_platform_driver(sstar_u3phy_driver);
MODULE_SOFTDEP("pre: usbpll");
MODULE_DESCRIPTION("SSTAR SSUSB PHY driver");
MODULE_LICENSE("GPL v2");
