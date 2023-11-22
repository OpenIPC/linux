/*
 * phy_sstar_port.h- Sigmastar
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

#ifndef __PHY_SSTAR_PORT_H__
#define __PHY_SSTAR_PORT_H__

int sstar_port_init(struct device *dev, struct device_node *np, const struct phy_ops *port_ops,
                    struct sstar_phy_port *port);
int sstar_port_deinit(struct sstar_phy_port *port);

#endif
