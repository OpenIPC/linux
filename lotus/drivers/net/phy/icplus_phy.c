// SPDX-License-Identifier: GPL-2.0+

#include <linux/bitops.h>
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/delay.h>

#define ICPLUS_PAGE_SELECT			0x14

MODULE_DESCRIPTION("ICPlus PHYs driver");
MODULE_AUTHOR("LOTUS");
MODULE_LICENSE("GPL");

static int icplus_read_page(struct phy_device *phydev)
{
	return __phy_read(phydev, ICPLUS_PAGE_SELECT);
}

static int icplus_write_page(struct phy_device *phydev, int page)
{
	return __phy_write(phydev, ICPLUS_PAGE_SELECT, page);
}

static int icplus_config_init(struct phy_device *phydev)
{
	int ret = 0;

	/* Digital IO Pin Driving Control Register */
#ifdef CONFIG_FPGA_SUPPORT
	ret = phy_modify_paged_changed(phydev, 4, 22, (0x7 << 13), (0x6 << 13));
#else
	ret = phy_modify_paged_changed(phydev, 4, 22, (0x7 << 13), (0x4 << 13));
#endif
	if (ret < 0)
		phydev_err(phydev, "phy_modify_paged_changed(RXC DRIVE) err: %d.\n",
			   ret);
#ifdef CONFIG_FPGA_SUPPORT
	ret = phy_modify_paged_changed(phydev, 16, 26, 0x7FFF, 0x6DB6);
#else
	ret = phy_modify_paged_changed(phydev, 16, 26, 0x7FFF, 0x36DB);
#endif
	if (ret < 0)
		phydev_err(phydev, "phy_modify_paged_changed(RXDx DRIVE) err: %d.\n",
			   ret);
#ifdef CONFIG_FPGA_SUPPORT
	ret = phy_modify_paged_changed(phydev, 16, 27, 0x7FC7, 0x6D86);
#else
	ret = phy_modify_paged_changed(phydev, 16, 27, 0x7FC7, 0x2484);
#endif
	if (ret < 0)
		phydev_err(phydev, "phy_modify_paged_changed(MDIO/COL/CRS/TXC DRIVE) err: %d.\n",
			   ret);

	return ret;
}

static struct phy_driver icplus_driver[] = {
	{
		.phy_id		= 0x02430c54,
		.name		= "ICPlus IP101A/G",
		.phy_id_mask	= 0x0ffffff0,
		/* PHY_BASIC_FEATURES */
		.config_init	= &icplus_config_init,
		.read_page	= icplus_read_page,
		.write_page	= icplus_write_page,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
	},
};

module_phy_driver(icplus_driver);

static struct mdio_device_id __maybe_unused icplus_tbl[] = {
	{ 0x02430c54, 0x0ffffff0 },
	{ }
};

MODULE_DEVICE_TABLE(mdio, icplus_tbl);

