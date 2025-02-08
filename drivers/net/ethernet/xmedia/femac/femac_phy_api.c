/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <linux/module.h>
#include <linux/phy.h>
#include <linux/xmedia/fephy.h>

struct phy_device *fephy_dev = NULL;


void __femac_set_phy_device(struct phy_device *phy)
{
	if (fephy_dev) {
		pr_err("FE PHY Device has been set\n");
		return;
	}

	fephy_dev = phy;
}
EXPORT_SYMBOL(__femac_set_phy_device);

int femac_phy_read(u32 regnum)
{
        if (!fephy_dev)
                return -ENODEV;

        return phy_read(fephy_dev, regnum);
}
EXPORT_SYMBOL(femac_phy_read);

int femac_phy_write(u32 regnum, u16 val)
{
        if (!fephy_dev)
                return -ENODEV;

        return phy_write(fephy_dev, regnum, val);
}
EXPORT_SYMBOL(femac_phy_write);

