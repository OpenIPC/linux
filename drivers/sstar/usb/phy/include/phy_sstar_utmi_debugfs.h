/*
 * phy_sstar_utmi_debugfs.h- Sigmastar
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

#ifndef __PHY_SSTAR_UTMI_DEBUGFS_H__
#ifdef CONFIG_DEBUG_FS
extern void sstar_phy_utmi_debugfs_init(struct sstar_phy_port *port);
extern void sstar_phy_utmi_debugfs_exit(struct sstar_phy_port *port);
#else
static inline void sstar_phy_utmi_debugfs_init(struct sstar_phy_port *port) {}
static inline void sstar_phy_utmi_debugfs_exit(struct sstar_phy_port *port) {}
#endif
#endif
