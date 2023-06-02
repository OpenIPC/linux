/*
 * drivers/net/phy/ksz80x1r.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/micrel_phy.h>

/* ==========================================================================*/
#define MII_KSZ80X1R_INTCS			0x1B
#define KSZ80X1R_INTCS_JABBER			(1 << 15)
#define KSZ80X1R_INTCS_RECEIVE_ERR		(1 << 14)
#define KSZ80X1R_INTCS_PAGE_RECEIVE		(1 << 13)
#define KSZ80X1R_INTCS_PARELLEL			(1 << 12)
#define KSZ80X1R_INTCS_LINK_PARTNER_ACK		(1 << 11)
#define KSZ80X1R_INTCS_LINK_DOWN		(1 << 10)
#define KSZ80X1R_INTCS_REMOTE_FAULT		(1 << 9)
#define KSZ80X1R_INTCS_LINK_UP			(1 << 8)
#define KSZ80X1R_INTCS_ALL			(KSZ80X1R_INTCS_LINK_UP |\
						KSZ80X1R_INTCS_LINK_DOWN)

#define MII_KSZ80X1R_CTRL			0x1F
#define KSZ80X1R_CTRL_INT_ACTIVE_HIGH		(1 << 9)
#define KSZ80X1R_RMII_50MHZ_CLK			(1 << 7)

/* ==========================================================================*/
MODULE_DESCRIPTION("Micrel KSZ8081R");
MODULE_AUTHOR("Anthony Ginger <hfjiang@ambarella.com>");
MODULE_LICENSE("GPL");

/* ==========================================================================*/
static int ksz80x1r_config_init(struct phy_device *phydev)
{
	int regval;

	pr_debug("Set KSZ80X1R 50MHz Clock Mode...");
	regval = phy_read(phydev, MII_KSZ80X1R_CTRL);
	regval |= KSZ80X1R_RMII_50MHZ_CLK;
	return phy_write(phydev, MII_KSZ80X1R_CTRL, regval);
}

void ksz80x1r_prevent_loss(struct phy_device *phydev) {
	phy_write(phydev, 0x0d, 0x001c);
	phy_write(phydev, 0x0e, 0x0008);
	phy_write(phydev, 0x0d, 0x401c);
	phy_write(phydev, 0x0e, 0x0067);
	phy_write(phydev, 0x0d, 0x001c);
	phy_write(phydev, 0x0e, 0x0009);
	phy_write(phydev, 0x0d, 0x401c);
	phy_write(phydev, 0x0e, 0xffff);
	phy_write(phydev, 0x0d, 0x001c);
	phy_write(phydev, 0x0e, 0x000a);
	phy_write(phydev, 0x0d, 0x401c);
	phy_write(phydev, 0x0e, 0xffff);
}

int ksz80x1r_config_aneg(struct phy_device *phydev)
{
	int result;

	result = genphy_config_aneg(phydev);
	if(result > 0)
		ksz80x1r_prevent_loss(phydev);

	return result;
}

static int ksz80x1r_ack_interrupt(struct phy_device *phydev)
{
	int rc;

	rc = phy_read(phydev, MII_KSZ80X1R_INTCS);

	return (rc < 0) ? rc : 0;
}

static int ksz80x1r_set_interrupt(struct phy_device *phydev)
{
	int temp;
	temp = (PHY_INTERRUPT_ENABLED == phydev->interrupts) ?
		KSZ80X1R_INTCS_ALL : 0;
	return phy_write(phydev, MII_KSZ80X1R_INTCS, temp);
}

static int ksz80x1r_config_intr(struct phy_device *phydev)
{
	int temp, rc;

	/* set the interrupt pin active low */
	temp = phy_read(phydev, MII_KSZ80X1R_CTRL);
	temp &= ~KSZ80X1R_CTRL_INT_ACTIVE_HIGH;
	phy_write(phydev, MII_KSZ80X1R_CTRL, temp);
	rc = ksz80x1r_set_interrupt(phydev);
	return rc < 0 ? rc : 0;
}

static struct phy_driver ksz80x1r_driver = {
	.phy_id		= PHY_ID_KSZ8081,
	.name		= "Micrel KSZ8081 or KSZ8091",
	.phy_id_mask	= 0x00fffff0,
	.features	= (PHY_BASIC_FEATURES | SUPPORTED_Pause),
	.flags		= PHY_HAS_MAGICANEG | PHY_HAS_INTERRUPT,
	.config_init	= ksz80x1r_config_init,
	.config_aneg	= ksz80x1r_config_aneg,
	.read_status	= genphy_read_status,
	.ack_interrupt	= ksz80x1r_ack_interrupt,
	.config_intr	= ksz80x1r_config_intr,
	.driver		= { .owner = THIS_MODULE,},
};

static int __init ksz80x1r_init(void)
{
	return phy_driver_register(&ksz80x1r_driver);
}

static void __exit ksz80x1r_exit(void)
{
	phy_driver_unregister(&ksz80x1r_driver);
}

/* ==========================================================================*/
module_init(ksz80x1r_init);
module_exit(ksz80x1r_exit);

static struct mdio_device_id __maybe_unused ksz80x1r_tbl[] = {
	{ PHY_ID_KSZ8081, 0x0000fff0 },
	{ }
};
MODULE_DEVICE_TABLE(mdio, ksz80x1r_tbl);

