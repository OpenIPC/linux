/* SPDX-License-Identifier: GPL-2.0 */
#ifndef USB_INCLUDE_DWC3_LOTUS_H
#define USB_INCLUDE_DWC3_LOTUS_H

struct dwc3_host {
	struct device *dev;
	struct clk **clks;
	int num_clocks;
	void __iomem *ctrl_base;
	void __iomem *crg_base;
	void __iomem *misc_base;
	struct reset_control *port_rst;
	u8 is_usb_host;
	u32 crg_offset;
	u32 crg_ctrl_def_mask;
	u32 crg_ctrl_def_val;
};

enum usb_scenes {
	/* U3 CTRL connect U3/U2 PHY */
	LOTUS_USB_SCENE2 = 0,
	/* U3 CTRL connect U3 PHY, U2 CTRL connect U2 PHY */
	LOTUS_USB_SCENE1 = 1,
};

enum usb_ctrl_types {
	LOTUS_DWC_USB2 = 0,			/* usb2 controller */
	LOTUS_DWC_USB3 = 1,			/* usb3 controller */
	LOTUS_USB_CTRL_UNKNOWN = 2,	/* unknown controller */
};

static const char *const usb_ctrl_names[] = {
	[LOTUS_DWC_USB2] = "lotus,dwcusb2",
	[LOTUS_DWC_USB3] = "lotus,dwcusb3",
};

#define DEV_NODE_FLAG0	0
#define DEV_NODE_FLAG1	1
#define DEV_NODE_FLAG2	2

void lotus_u2_ctrl_clk_init(void __iomem *crg_base);
void lotus_u2_ctrl_clk_exit(void __iomem *crg_base);
void lotus_u2_ctrl_clk_resume(void __iomem *crg_base);
void lotus_u2_ctrl_misc_config(struct dwc3_host *host);

#endif	/* end of USB_INCLUDE_DWC3_LOTUS_H */
