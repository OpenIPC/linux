/*
 * drivers/net/phy/realtek.c
 *
 * Driver for Realtek PHYs
 *
 * Author: Johnson Leung <r58129@freescale.com>
 *
 * Copyright (c) 2004 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/phy.h>
#include <linux/module.h>

#define RTL821x_PHYSR		0x11
#define RTL821x_PHYSR_DUPLEX	0x2000
#define RTL821x_PHYSR_SPEED	0xc000
#define RTL821x_INER		0x12
#define RTL821x_INER_INIT	0x6400
#define RTL821x_INSR		0x13

MODULE_DESCRIPTION("Realtek PHY driver");
MODULE_AUTHOR("Johnson Leung");
MODULE_LICENSE("GPL");

static int rtl820x_config_init(struct phy_device *phydev)
{
	int value;

	phy_write(phydev, 31, 7);
	
	value = phy_read(phydev, 19);
	value &= ~(0x3 << 4);
	phy_write(phydev, 19, value);
	
	phy_write(phydev, 31, 0);

	return 0;
}

static int rtl821x_ack_interrupt(struct phy_device *phydev)
{
	int err;

	err = phy_read(phydev, RTL821x_INSR);

	return (err < 0) ? err : 0;
}

static int rtl821x_config_intr(struct phy_device *phydev)
{
	int err;

	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_write(phydev, RTL821x_INER,
				RTL821x_INER_INIT);
	else
		err = phy_write(phydev, RTL821x_INER, 0);

	return err;
}

/* RTL8211B */
static struct phy_driver rtl821x_driver = {
	.phy_id		= 0x001cc915,
	.name		= "RTL8211E Gigabit Ethernet",
	.phy_id_mask	= 0x001fffff,
	.features	= PHY_GBIT_FEATURES,
	.flags		= PHY_HAS_INTERRUPT,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
	.ack_interrupt	= &rtl821x_ack_interrupt,
	.config_intr	= &rtl821x_config_intr,
	.driver		= { .owner = THIS_MODULE,},
};

/* RTL8201 */
static struct phy_driver rtl820x_driver = {
	.phy_id		= 0x001cc816,
	.name		= "RTL8201 Ethernet",
	.phy_id_mask	= 0x001fffff,
	.features	= PHY_BASIC_FEATURES | SUPPORTED_Pause |
			  SUPPORTED_Asym_Pause,
	.flags		= PHY_HAS_INTERRUPT,
	.config_init	= &rtl820x_config_init,
	.config_aneg	= &genphy_config_aneg,
	.read_status	= &genphy_read_status,
	.suspend	= genphy_suspend,
	.resume		= genphy_resume,
	.driver		= { .owner = THIS_MODULE,},
};

static int __init realtek_init(void)
{
	int ret;

	ret = phy_driver_register(&rtl821x_driver);
	if (ret)
		goto phy_1;

	ret = phy_driver_register(&rtl820x_driver);
	if (ret)
		goto phy_2;

	return 0;
phy_2:
	phy_driver_unregister (&rtl821x_driver);
phy_1:		
	return ret;
}

static void __exit realtek_exit(void)
{
	phy_driver_unregister(&rtl821x_driver);
	phy_driver_unregister(&rtl820x_driver);
}

module_init(realtek_init);
module_exit(realtek_exit);

static struct mdio_device_id __maybe_unused realtek_tbl[] = {
	{ 0x001cc915, 0x001fffff },
	{ 0x001cc816, 0x001fffff },
	{ }
};

MODULE_DEVICE_TABLE(mdio, realtek_tbl);
