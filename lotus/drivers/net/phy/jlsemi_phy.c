// SPDX-License-Identifier: GPL-2.0+

#include <linux/bitops.h>
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/delay.h>

#define JL11X1_PAGE_SELECT			0x1f

MODULE_DESCRIPTION("JLSemi PHY driver");
MODULE_AUTHOR("LOTUS");
MODULE_LICENSE("GPL");

static int jl11x1_read_page(struct phy_device *phydev)
{
	return __phy_read(phydev, JL11X1_PAGE_SELECT);
}

static int jl11x1_write_page(struct phy_device *phydev, int page)
{
	return __phy_write(phydev, JL11X1_PAGE_SELECT, page);
}

static void jl11x1_link_change_notify(struct phy_device *phydev)
{
	if (phydev->state == PHY_RUNNING && phydev->speed == SPEED_100) {
		int ret = phy_modify_paged_changed(phydev, 24, 24, (0x7 << 10),
		       (0x5 << 10));

		if (ret < 0)
			phydev_err(phydev, "phy_modify_paged_changed err: %d.\n",
				   ret);

		// set page 7 reg 16 tx 3
		ret = phy_modify_paged_changed(phydev, 7, 16, 0xf << 8, 0x3 << 8);

		if (ret < 0)
			phydev_err(phydev, "phy_modify_paged_changed(tx and rx skew) err: %d.\n",
				   ret);
	}
}

static int jl11x1_config_init(struct phy_device *phydev)
{
	int ret = 0;

	/* Set green LED for Link, yellow LED for Active */
	ret = phy_modify_paged_changed(phydev, 7, 19, 0x3 << 4, 0x00 << 4);

	if (ret < 0)
		phydev_err(phydev, "phy_modify_paged_changed(LED) err: %d.\n",
			   ret);

	return ret;
}

static struct phy_driver JL11x1_driver[] = {
	{
		.phy_id		= 0x937c4024,
		.name		= "JLSemi JL11x1",
		.phy_id_mask	= 0xffffffff,
		/* PHY_BASIC_FEATURES */
		.config_init	= &jl11x1_config_init,
		.link_change_notify	= jl11x1_link_change_notify,
		.read_page	= jl11x1_read_page,
		.write_page	= jl11x1_write_page,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
	},
};

module_phy_driver(JL11x1_driver);

static struct mdio_device_id __maybe_unused jlsemi_tbl[] = {
	{ 0x937c4024, 0xffffffff },
	{ }
};

MODULE_DEVICE_TABLE(mdio, jlsemi_tbl);

