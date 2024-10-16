/*
 * drivers/usb/sunxi_usb/usbc/usbc_phy.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * daniel, 2009.10.21
 *
 * usb common ops.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include  "usbc_i.h"
#include "linux/sunxi-sid.h"

/**
 * define USB PHY controller reg bit
 */

/* Common Control Bits for Both PHYs */
#define  USBC_PHY_PLL_BW			0x03
#define  USBC_PHY_RES45_CAL_EN			0x0c

/* Private Control Bits for Each PHY */
#define  USBC_PHY_TX_AMPLITUDE_TUNE		0x20
#define  USBC_PHY_TX_SLEWRATE_TUNE		0x22
#define  USBC_PHY_VBUSVALID_TH_SEL		0x25
#define  USBC_PHY_PULLUP_RES_SEL		0x27
#define  USBC_PHY_OTG_FUNC_EN			0x28
#define  USBC_PHY_VBUS_DET_EN			0x29
#define  USBC_PHY_DISCON_TH_SEL			0x2a

/* usb PHY common set, initialize */
void USBC_PHY_SetCommonConfig(void)
{
}

/**
 * usb PHY specific set
 * @hUSB: handle returned by USBC_open_otg,
 *        include some key data that the USBC need.
 *
 */
void USBC_PHY_SetPrivateConfig(__hdle hUSB)
{
}

/**
 * get PHY's common setting. for debug, to see if PHY is set correctly.
 *
 * return the 32bit usb PHY common setting value.
 */
__u32 USBC_PHY_GetCommonConfig(void)
{
	__u32 reg_val = 0;

	return reg_val;
}

/**
 * write usb PHY0's phy reg setting. mainly for phy0 standby.
 *
 * return the data wrote
 */
static __u32 usb_phy0_write(__u32 addr,
		__u32 data, __u32 dmask, void __iomem *usbc_base_addr)
{
	__u32 i = 0;

	data = data & 0x0f;
	addr = addr & 0x0f;
	dmask = dmask & 0x0f;

	USBC_Writeb((dmask<<4)|data, usbc_base_addr + 0x404 + 2);
	USBC_Writeb(addr|0x10, usbc_base_addr + 0x404);
	for (i = 0; i < 5 ; i++)
		;
	USBC_Writeb(addr|0x30, usbc_base_addr + 0x404);
	for (i = 0 ; i < 5 ; i++)
		;
	USBC_Writeb(addr|0x10, usbc_base_addr + 0x404);
	for (i = 0 ; i < 5 ; i++)
		;

	return (USBC_Readb(usbc_base_addr + 0x404 + 3) & 0x0f);
}

/**
 * Standby the usb phy with the input usb phy index number
 * @phy_index: usb phy index number, which used to select the phy to standby
 *
 */
void USBC_phy_Standby(__hdle hUSB, __u32 phy_index)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (phy_index == 0) {
		usb_phy0_write(0xB, 0x8, 0xf, usbc_otg->base_addr);
		usb_phy0_write(0x7, 0xf, 0xf, usbc_otg->base_addr);
		usb_phy0_write(0x1, 0xf, 0xf, usbc_otg->base_addr);
		usb_phy0_write(0x2, 0xf, 0xf, usbc_otg->base_addr);
	}
}

/**
 * Recover the standby phy with the input index number
 * @phy_index: usb phy index number
 *
 */
void USBC_Phy_Standby_Recover(__hdle hUSB, __u32 phy_index)
{
	__u32 i;

	if (phy_index == 0) {
		for (i = 0; i < 0x10; i++)
			;
	}
}

static __u32 USBC_Phy_TpWrite(__u32 usbc_no, __u32 addr, __u32 data, __u32 len)
{
	void __iomem *otgc_base = NULL;
	void __iomem *phyctl_val = NULL;
	__u32 temp = 0, dtmp = 0;
	__u32 j = 0;

	otgc_base = get_otgc_vbase();
	if (otgc_base == NULL)
		return 0;

	phyctl_val = otgc_base + USBPHYC_REG_o_PHYCTL;

	dtmp = data;
	for (j = 0; j < len; j++) {
		/* set the bit address to be write */
		temp = USBC_Readl(phyctl_val);
		temp &= ~(0xff << 8);
		temp |= ((addr + j) << 8);
		USBC_Writel(temp, phyctl_val);

		temp = USBC_Readb(phyctl_val);
		temp &= ~(0x1 << 7);
		temp |= (dtmp & 0x1) << 7;
		temp &= ~(0x1 << (usbc_no << 1));
		USBC_Writeb(temp, phyctl_val);

		temp = USBC_Readb(phyctl_val);
		temp |= (0x1 << (usbc_no << 1));
		USBC_Writeb(temp, phyctl_val);

		temp = USBC_Readb(phyctl_val);
		temp &= ~(0x1 << (usbc_no << 1));
		USBC_Writeb(temp, phyctl_val);
		dtmp >>= 1;
	}

	return data;
}

static __u32 USBC_Phy_Write(__u32 usbc_no, __u32 addr, __u32 data, __u32 len)
{
	return USBC_Phy_TpWrite(usbc_no, addr, data, len);
}

void UsbPhyCtl(void __iomem *regs)
{
	__u32 reg_val = 0;

	reg_val = USBC_Readl(regs + USBPHYC_REG_o_PHYCTL);
	reg_val |= (0x01 << USBC_PHY_CTL_VBUSVLDEXT);
	USBC_Writel(reg_val, (regs + USBPHYC_REG_o_PHYCTL));
}

void USBC_PHY_Set_Ctl(void __iomem *regs, __u32 mask)
{
	__u32 reg_val = 0;

	reg_val = USBC_Readl(regs + USBPHYC_REG_o_PHYCTL);
	reg_val |= (0x01 << mask);
	USBC_Writel(reg_val, (regs + USBPHYC_REG_o_PHYCTL));
}

void USBC_PHY_Clear_Ctl(void __iomem *regs, __u32 mask)
{
	__u32 reg_val = 0;

	reg_val = USBC_Readl(regs + USBPHYC_REG_o_PHYCTL);
	reg_val &= ~(0x01 << mask);
	USBC_Writel(reg_val, (regs + USBPHYC_REG_o_PHYCTL));
}

void UsbPhyInit(__u32 usbc_no)
{

	/* adjust the 45 ohm resistor */
	if (usbc_no == 0)
		USBC_Phy_Write(usbc_no, 0x0c, 0x01, 1);

	/* adjust USB0 PHY range and rate */
	USBC_Phy_Write(usbc_no, 0x20, 0x14, 5);

	/* adjust disconnect threshold */
	USBC_Phy_Write(usbc_no, 0x2a, 3, 2);
	/* by wangjx */
}

void UsbPhyEndReset(__u32 usbc_no)
{
	int i;

	if (usbc_no == 0) {
		/**
		 * Disable Sequelch Detect for a while
		 * before Release USB Reset.
		 */
		USBC_Phy_Write(usbc_no, 0x3c, 0x2, 2);
		for (i = 0; i < 0x100; i++)
			;
		USBC_Phy_Write(usbc_no, 0x3c, 0x0, 2);
	}
}

void usb_otg_phy_txtune(void __iomem *regs)
{
	__u32 reg_val = 0;

	reg_val = USBC_Readl(regs + USBC_REG_o_PHYTUNE);
#if defined(CONFIG_ARCH_SUN8IW18)
	reg_val |= (0x01 << 1);
#else
	reg_val |= 0x03 << 2;	/* TXRESTUNE */
#endif
	reg_val &= ~(0xf << 8);
	reg_val |= 0xc << 8;	/* TXVREFTUNE */
	USBC_Writel(reg_val, (regs + USBC_REG_o_PHYTUNE));
}

/*for new phy*/
int usbc_new_phyx_tp_write(void __iomem *regs,
		int addr, int data, int len)
{
	int temp = 0;
	int j = 0;
	int dtmp = 0;

	/*device: 0x410(phy_ctl)*/
	dtmp = data;

	for (j = 0; j < len; j++) {

		temp = USBC_Readb(regs + USBPHYC_REG_o_PHYCTL);
		temp |= (0x1 << 1);
		USBC_Writeb(temp, regs + USBPHYC_REG_o_PHYCTL);

		USBC_Writeb(addr + j, regs + USBPHYC_REG_o_PHYCTL + 1);

		temp = USBC_Readb(regs + USBPHYC_REG_o_PHYCTL);
		temp &= ~(0x1 << 0);
		USBC_Writeb(temp, regs + USBPHYC_REG_o_PHYCTL);

		temp = USBC_Readb(regs + USBPHYC_REG_o_PHYCTL);
		temp &= ~(0x1 << 7);
		temp |= (dtmp & 0x1) << 7;
		USBC_Writeb(temp, regs + USBPHYC_REG_o_PHYCTL);

		temp |= (0x1 << 0);
		USBC_Writeb(temp, regs + USBPHYC_REG_o_PHYCTL);

		temp &= ~(0x1 << 0);
		USBC_Writeb(temp, regs + USBPHYC_REG_o_PHYCTL);

		temp = USBC_Readb(regs + USBPHYC_REG_o_PHYCTL);
		temp &= ~(0x1 << 1);
		USBC_Writeb(temp, regs + USBPHYC_REG_o_PHYCTL);

		dtmp >>= 1;
	}

	return 0;
}

int usbc_new_phyx_tp_read(void __iomem *regs, int addr, int len)
{
	int temp = 0;
	int i = 0;
	int j = 0;
	int ret = 0;

	temp = USBC_Readb(regs + USBPHYC_REG_o_PHYCTL);
	temp |= (0x1 << 1);
	USBC_Writeb(temp, regs + USBPHYC_REG_o_PHYCTL);

	for (j = len; j > 0; j--) {
		USBC_Writeb((addr + j - 1), regs + USBPHYC_REG_o_PHYCTL + 1);

		for (i = 0; i < 0x4; i++)
			;

		temp = USBC_Readb(regs + USBC_REG_o_PHYSTATUS);
		ret <<= 1;
		ret |= (temp & 0x1);
	}

	temp = USBC_Readb(regs + USBPHYC_REG_o_PHYCTL);
	temp &= ~(0x1 << 1);
	USBC_Writeb(temp, regs + USBPHYC_REG_o_PHYCTL);

	return ret;
}


void usbc_new_phy_init(void __iomem *regs)
{
	int value = 0;
	u32 efuse_val  = 0;

	pr_debug("addr:%x,len:%x,value:%x\n", 0x03, 0x06,
			usbc_new_phyx_tp_read(regs, 0x03, 0x06));
	pr_debug("addr:%x,len:%x,value:%x\n", 0x16, 0x03,
			usbc_new_phyx_tp_read(regs, 0x16, 0x03));
	pr_debug("addr:%x,len:%x,value:%x\n", 0x0b, 0x08,
			usbc_new_phyx_tp_read(regs, 0x0b, 0x08));
	pr_debug("addr:%x,len:%x,value:%x\n", 0x09, 0x03,
			usbc_new_phyx_tp_read(regs, 0x09, 0x03));

	sunxi_get_module_param_from_sid(&efuse_val, EFUSE_OFFSET, 4);
	pr_debug("efuse_val:0x%x\n", efuse_val);

	usbc_new_phyx_tp_write(regs, 0x03, 0x3f, 0x06);
	pr_debug("addr:%x,len:%x,value:%x\n", 0x03, 0x3f,
			usbc_new_phyx_tp_read(regs, 0x03, 0x06));

	usbc_new_phyx_tp_write(regs, 0x1c, 0x03, 0x03);
	pr_debug("addr:%x,len:%x,value:%x\n", 0x1c, 0x03,
			usbc_new_phyx_tp_read(regs, 0x1c, 0x03));

	if (efuse_val & SUNXI_HCI_PHY_EFUSE_ADJUST) {
		/* Already calibrate completely, don't have to distinguish iref mode and vref mode */
		pr_debug("USB phy already calibrate completely\n");

		/* usbc-0 */
		value = (efuse_val & SUNXI_HCI_PHY_EFUSE_USB0TX) >> 5;
		usbc_new_phyx_tp_write(regs, 0x60, value, 0x04);

		value = (efuse_val & SUNXI_HCI_PHY_EFUSE_RES) >> 5;
		usbc_new_phyx_tp_write(regs, 0x44, value, 0x04);

		pr_debug("addr:%x,len:%x,value:%x\n", 0x60, 0x04,
				 usbc_new_phyx_tp_read(regs, 0x60, 0x04));
		pr_debug("addr:%x,len:%x,value:%x\n", 0x44, 0x04,
				 usbc_new_phyx_tp_read(regs, 0x44, 0x04));
	} else {
		pr_debug("USB phy don't calibrate\n");
	}

	pr_debug("addr:%x,len:%x,value:%x\n", 0x03, 0x06,
			usbc_new_phyx_tp_read(regs, 0x03, 0x06));
	pr_debug("addr:%x,len:%x,value:%x\n", 0x16, 0x03,
			usbc_new_phyx_tp_read(regs, 0x16, 0x03));
	pr_debug("addr:%x,len:%x,value:%x\n", 0x0b, 0x08,
			usbc_new_phyx_tp_read(regs, 0x0b, 0x08));
	pr_debug("addr:%x,len:%x,value:%x\n", 0x09, 0x03,
			usbc_new_phyx_tp_read(regs, 0x09, 0x03));
}

void usbc_new_phy_res_cal(void __iomem *regs)
{
	int value;

	/*clear software res cail*/
	usbc_new_phyx_tp_write(regs, 0x43, 0x0, 0x01);
	usbc_new_phyx_tp_write(regs, 0x41, 0x0, 0x01);
	usbc_new_phyx_tp_write(regs, 0x40, 0x0, 0x01);
	/*res cail*/
	usbc_new_phyx_tp_write(regs, 0x40, 0x01, 0x01);
	mdelay(1);
	usbc_new_phyx_tp_write(regs, 0x41, 0x01, 0x01);

	while (1) {
		if (usbc_new_phyx_tp_read(regs, 0x42, 0x01))
			break;
	}

	/*set res*/
	value = usbc_new_phyx_tp_read(regs, 0x49, 0x04);
	pr_debug("addr:%x,,value:%x\n", 0x49, value);
	usbc_new_phyx_tp_write(regs, 0x44, value, 0x04);
	usbc_new_phyx_tp_write(regs, 0x41, 0x0, 0x01);
	usbc_new_phyx_tp_write(regs, 0x40, 0x0, 0x01);
	usbc_new_phyx_tp_write(regs, 0x43, 0x01, 0x01);
}
