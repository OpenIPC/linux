#include <linux/phy.h>
#include "phy_fix.h"

static const u32 phy_v272_fix_param[] = {
#include "festa_v272_2723.h"
};

static const u32 phy_v115_fix_param[] = {
#include "festa_s28v115_2c02.h"
};

static int phy_expanded_write_bulk(struct phy_device *phy_dev,
                                   const u32 reg_and_val[], int count)
{
    int i, v, ret = 0;
    u32 reg_addr;
    u16 val;

    v = phy_read(phy_dev, MII_BMCR);
    v = (u32)v | BMCR_PDOWN;
    phy_write(phy_dev, MII_BMCR, v);

    for (i = 0; i < (2 * count); i += 2) {
        reg_addr = reg_and_val[i];
        val = (u16)reg_and_val[i + 1];
        phy_write(phy_dev, MII_EXPMA, reg_addr);
        ret = phy_write(phy_dev, MII_EXPMD, val);
    }

    v = phy_read(phy_dev, MII_BMCR);
    v = (u32)v & (~BMCR_PDOWN);
    phy_write(phy_dev, MII_BMCR, v);

    return ret;
}

static int hisilicon_fephy_v272_fix(struct phy_device *phy_dev)
{
    int count;

    count = ARRAY_SIZE(phy_v272_fix_param);
    if (count % 2) {
        pr_warn("internal FEPHY fix register count is not right.\n");
    }
    count /= 2;
    phy_expanded_write_bulk(phy_dev, phy_v272_fix_param, count);

    return 0;
}

static int hisilicon_fephy_v115_fix(struct phy_device *phy_dev)
{
    int count;

    count = ARRAY_SIZE(phy_v115_fix_param);
    if (count % 2) {
        pr_warn("internal FEPHY fix register count is not right.\n");
    }
    count /= 2;

    phy_expanded_write_bulk(phy_dev, phy_v115_fix_param, count);

    return 0;
}

void phy_register_fixups(void)
{
    phy_register_fixup_for_uid(HISILICON_PHY_ID_FESTAV272,
                               HISILICON_PHY_MASK,
                               hisilicon_fephy_v272_fix);

    phy_register_fixup_for_uid(HISILICON_PHY_ID_FESTAV115,
                               HISILICON_PHY_MASK,
                               hisilicon_fephy_v115_fix);
}

void phy_unregister_fixups(void)
{
    phy_unregister_fixup_for_uid(HISILICON_PHY_ID_FESTAV272,
                                 HISILICON_PHY_MASK);

    phy_unregister_fixup_for_uid(HISILICON_PHY_ID_FESTAV115,
                                 HISILICON_PHY_MASK);
}
