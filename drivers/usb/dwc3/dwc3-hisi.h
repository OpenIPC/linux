/*
 * dwc3-hisi.h
 *
 * Dwc3 private headerfile for Hisilicon.
 *
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef USB_INCLUDE_DWC3_HISI_H
#define USB_INCLUDE_DWC3_HISI_H

struct hi_priv {
	void __iomem *peri_crg;
	void __iomem *sys_ctrl;
	void __iomem *misc_ctrl;
	unsigned int speed_id;
};

struct dwc3_hisi {
	struct device *dev;
	struct clk **clks;
	int num_clocks;
	void __iomem *ctrl_base;
	void __iomem *crg_base;
	struct reset_control *port_rst;
	u32 crg_offset;
	u32 crg_ctrl_def_mask;
	u32 crg_ctrl_def_val;
};

extern int usb_get_max_speed(struct device *dev);
extern void hisi_dwc3_exited(void);

#define DEV_NODE_FLAG0	0
#define DEV_NODE_FLAG1	1
#define DEV_NODE_FLAG2	2

#define U_LEVEL2	200

#endif
