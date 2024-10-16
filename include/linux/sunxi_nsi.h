/*
 * SUNXI MBUS support
 *
 * Copyright (C) 2015 AllWinnertech Ltd.
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

#ifndef __LINUX_SUNXI_MBUS_H
#define __LINUX_SUNXI_MBUS_H

#include <linux/types.h>

/* MBUS PMU ids */
enum nsi_pmu {
	MBUS_PMU_CPU = 0,

#if (defined CONFIG_ARCH_SUN50IW10)
	MBUS_PMU_GPU = 1,
	MBUS_PMU_SD1 = 2,
	MBUS_PMU_MSTG = 3,
	MBUS_PMU_GMAC0 = 4,
	MBUS_PMU_GMAC1 = 5,
	MBUS_PMU_USB0 = 6,
	MBUS_PMU_USB1 = 7,
	MBUS_PMU_NDFC = 8,
	MBUS_PMU_DMAC = 9,
	MBUS_PMU_CE = 10,
	MBUS_PMU_DE0 = 11,
	MBUS_PMU_DE1 = 12,
	MBUS_PMU_VE = 13,
	MBUS_PMU_CSI = 14,
	MBUS_PMU_ISP = 15,
	MBUS_PMU_G2D = 16,
	MBUS_PMU_EINK = 17,
	MBUS_PMU_IOMMU = 18,
	MBUS_PMU_SYS_CPU = 19,
	MBUS_PMU_TOTAL = 20,
	MBUS_PMU_MAX = 21,
#endif
};

#if (defined CONFIG_ARCH_SUN50IW10)
static const char *const pmu_name[] = {
	"cpu", "gpu", "sd1", "mstg", "gmac0", "gmac1", "usb0", "usb1", "ndfc",
	"dmac", "ce", "de0", "de1", "ve", "csi", "isp", "g2d", "eink", "iommu",
	"sys_cpu", "total",
};
#endif

#define get_name(n)      pmu_name[n]

#ifdef CONFIG_SUNXI_NSI
extern int nsi_port_setpri(enum nsi_pmu port, unsigned int pri);
extern int nsi_port_setqos(enum nsi_pmu port, unsigned int qos);
extern bool nsi_probed(void);
#endif

#define nsi_disable_port_by_index(dev) \
	nsi_port_control_by_index(dev, false)
#define nsi_enable_port_by_index(dev) \
	nsi_port_control_by_index(dev, true)

#endif
