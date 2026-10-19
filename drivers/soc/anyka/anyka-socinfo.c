/**
 * @file anyka-socinfo.c
 * @author anyka
 * @brief 
 * @version 0.1
 * @date 2022-02-11
 * 
 * @copyright Copyright Anyka (c) 2022
 * 
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

#include <mach/map.h>

#define AK_CHIP_ID                  (AK_VA_SYSCTRL + 0x00)
#define AK_CHIP_NAME_LEN            20

/* anyka chip infomation */
struct ak_chip_info {
    unsigned long int id;
    unsigned long int shift;
    unsigned long int mask;
    char name[AK_CHIP_NAME_LEN];
};

/* id tables */
const struct ak_chip_info ak_ci[] = {
    {0x535335, 8, 0xFFFFFF, "AK3918AV100"},     /* AK3918AV100 Chip */
    {0x535434, 8, 0xFFFFFF, "AK3918EV300L"},    /* AK3918EV300L Chip */
};

static const char *anyka_socinfo_soc_id(void)
{
    const char *soc_id = "Unknown";
    unsigned long chip_id, tmp_id;
    int i = 0;

    /* read chip id */
    chip_id = __raw_readl(AK_CHIP_ID);

    while (i < sizeof(ak_ci)/sizeof(ak_ci[0])) {

        tmp_id = (chip_id >> ak_ci[i].shift) & ak_ci[i].mask;

        /* matching */
        if (tmp_id == ak_ci[i].id) {
            soc_id = ak_ci[i].name;
            break;
        }
        
        i++;
    }

    return kstrdup_const(soc_id, GFP_KERNEL);
}

static int __init anyka_socinfo_init(void)
{
    struct soc_device_attribute *soc_dev_attr;
    struct soc_device *soc_dev;
    struct device_node *np;

    soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
    if (!soc_dev_attr)
        return -ENODEV;

    soc_dev_attr->family = "Anyka";

    np = of_find_node_by_path("/");
    of_property_read_string(np, "model", &soc_dev_attr->machine);
    of_node_put(np);

    soc_dev_attr->revision = "None";
    soc_dev_attr->soc_id = anyka_socinfo_soc_id();

    soc_dev = soc_device_register(soc_dev_attr);
    if (IS_ERR(soc_dev)) {
        /* kfree_const(soc_dev_attr->revision); */
        kfree_const(soc_dev_attr->soc_id);
        kfree(soc_dev_attr);
        return PTR_ERR(soc_dev);
    }

    dev_info(soc_device_to_device(soc_dev), "Anyka %s %s detected\n",
         soc_dev_attr->soc_id, soc_dev_attr->revision);

    return 0;
}

device_initcall(anyka_socinfo_init);
