/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <linux/phy.h>
#include "phy_fix.h"

static const u32 phy_v2_fix_param[] = {
#include "phy_v2.h"
};

static const u32 phy_v1_fix_param[] = {
#include "phy_v1.h"
};

static int phy_expanded_write_bulk(struct phy_device *phy_dev,
		  const u32 reg_and_val[], int count)
{
	int i, v;
	u32 reg_addr;
	u16 val;

	v = phy_read(phy_dev, MII_BMCR);
	v = (u32)v | BMCR_PDOWN;
	phy_write(phy_dev, MII_BMCR, v);

	for (i = 0; i < count; i += 2) { /* Process 2 data at a time. */
		reg_addr = reg_and_val[i];
		val = (u16)reg_and_val[i + 1];
		phy_write(phy_dev, MII_EXPMA, reg_addr);
		phy_write(phy_dev, MII_EXPMD, val);
	}

	v = phy_read(phy_dev, MII_BMCR);
	v = (u32)v & (~BMCR_PDOWN);
	phy_write(phy_dev, MII_BMCR, v);

	return 0;
}

static int xmedia_fephy_v272_fix(struct phy_device *phy_dev)
{
	int count;

	count = ARRAY_SIZE(phy_v2_fix_param);
	if (count % 2) /* must be an even number, mod 2 */
		pr_warn("internal FEPHY fix register count is not right.\n");
	phy_expanded_write_bulk(phy_dev, phy_v2_fix_param, count);

	return 0;
}

static int xmedia_fephy_v115_fix(struct phy_device *phy_dev)
{
	int count;

	count = ARRAY_SIZE(phy_v1_fix_param);
	if (count % 2) /* must be an even number, mod 2 */
		pr_warn("internal FEPHY fix register count is not right.\n");
	phy_expanded_write_bulk(phy_dev, phy_v1_fix_param, count);

	return 0;
}

void phy_register_fixups(void)
{
	phy_register_fixup_for_uid(XMEDIA_PHY_ID_V272,
							   XMEDIA_PHY_MASK,
							   xmedia_fephy_v272_fix);

	phy_register_fixup_for_uid(XMEDIA_PHY_ID_V115,
							   XMEDIA_PHY_MASK,
							   xmedia_fephy_v115_fix);
}

void phy_unregister_fixups(void)
{
	phy_unregister_fixup_for_uid(XMEDIA_PHY_ID_V272,
								 XMEDIA_PHY_MASK);

	phy_unregister_fixup_for_uid(XMEDIA_PHY_ID_V115,
								 XMEDIA_PHY_MASK);
}
