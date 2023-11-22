/*
 * mdrv_sysfs_arb.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include "MsTypes.h"
#include "registers.h"
#include "ms_platform.h"
#include "cam_os_wrapper.h"
#include "mhal_arb.h"
#include "mdrv_miu.h"

static struct miu_device     arb_dev[MIU_NUM];
static struct miu_arb_handle arb_handle[MIU_NUM];

short miu_client_reserved(U16 id)
{
    return halARBClientReserved(id);
}
/*=============================================================*/
// Local function
/*=============================================================*/
#ifdef CONFIG_MIU_SUPPORT_SYNOPSYS
static int _get_reg_base(int miu)
{
    return halARBGetRegBase(miu);
}
static int _get_reg_base_e(void)
{
    return halARBGetRegBase_e();
}
#else
static int _get_reg_base(int miu)
{
    return halARBGetRegBase(miu);
}
static int _get_reg_base_e(void)
{
    return halARBGetRegBase_e();
}
#endif
static void _load_group_settings(struct miu_arb_grp_reg *greg, int base, int g)
{
    halARBLoadGroupSetting(greg, base, g);
}

static void _load_group_settings_e(struct miu_arb_grp_reg_e *greg, int base, int g)
{
    halARBLoadGroupSettings_e(greg, base, g);
}

static bool _is_flowctrl_enable(struct miu_arb_grp_reg *greg, int mbr, char *period)
{
    return halARBIsFlowCTLEnable(greg, mbr, period);
}

static bool _enable_flowctrl(struct miu_arb_grp_reg *greg, int mbr, bool enable, char period)
{
    return halARBEnableFlowCTL(greg, mbr, enable, period);
}

static char *_dump_as_text(char *str, char *end, unsigned char miu)
{
    int                      g, mbr, c, base, base_e = 0;
    u16                      outer_cfg, mbr_burst, mbr_prio;
    struct miu_arb_grp_reg   greg;
    struct miu_arb_grp_reg_e greg_e;
    char                     clientName[40];

    memset(clientName, 0, sizeof(clientName));
    base = _get_reg_base(miu);
    if (!base)
    {
        return str;
    }

    outer_cfg = halARBGetOGCCFG(base);
    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        /* Group settings */
        _load_group_settings(&greg, base, g);
        _load_group_settings_e(&greg_e, base_e, g);
        str += scnprintf(str, end - str,
                         ASCII_COLOR_BLUE "==== Group %d ================================\n" ASCII_COLOR_END, g);
        str += scnprintf(str, end - str, " Outer-Prio: ");
        if (outer_cfg & OGCFG_ROUND_ROBIN_EN)
        {
            str += scnprintf(str, end - str, "Round-Robin\n");
        }
        else if (outer_cfg & OGCFG_FIX_PRIO_EN)
        {
            int p = 0;
            while (p < MIU_ARB_OG_PRIO_NUM)
            {
                if (((outer_cfg & OGCFG_GRP_PRIO_MASK(p)) >> OGCFG_GRP_PRIO_MASK(p)) == g)
                {
                    break;
                }
                p++;
            };
            str += scnprintf(str, end - str, "%d\n", p);
        }
        str += scnprintf(str, end - str, " Inner-Prio: %s\n",
                         (greg.cfg & IGCFG_ROUND_ROBIN_BIT) ? "Round-Robin" : "Fix priority");
        str += scnprintf(str, end - str, " Burst     : %d\n",
                         (greg.cfg & IGCFG_GRP_LIMIT_EN_BIT) ? (greg.burst & IGBURST_GRP_MASK) >> IGBURST_GRP_SHIFT
                                                             : 0); // no limit
        str += scnprintf(str, end - str, " FlowCtrl0 : ");
        if (greg.cfg & IGCFG_FCTL0_EN_BIT)
        {
            c = (g * MIU_GRP_CLIENT_NUM) + ((greg.flowctrl0 & IGFCTL_ID0_MASK) >> IGFCTL_ID0_SHIFT);
            if (!miu_client_reserved(c))
            {
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%s, ", clientName);
            }
            c = (g * MIU_GRP_CLIENT_NUM) + ((greg.flowctrl0 & IGFCTL_ID1_MASK) >> IGFCTL_ID1_SHIFT);
            if (!miu_client_reserved(c))
            {
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%s, ", clientName);
            }
            str += scnprintf(str, end - str, "Preiod 0x%02X\n",
                             (greg.flowctrl0 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT);
        }
        else
        {
            str += scnprintf(str, end - str, "None\n");
        }
        str += scnprintf(str, end - str, " FlowCtrl1 : ");
        if (greg.cfg & IGCFG_FCTL1_EN_BIT)
        {
            c = (g * MIU_GRP_CLIENT_NUM) + ((greg.flowctrl1 & IGFCTL_ID0_MASK) >> IGFCTL_ID0_SHIFT);
            if (!miu_client_reserved(c))
            {
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%s, ", clientName);
            }
            c = (g * MIU_GRP_CLIENT_NUM) + ((greg.flowctrl1 & IGFCTL_ID1_MASK) >> IGFCTL_ID1_SHIFT);
            if (!miu_client_reserved(c))
            {
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%s, ", clientName);
            }
            str += scnprintf(str, end - str, "Preiod 0x%02X\n",
                             (greg.flowctrl1 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT);
        }
        else
        {
            str += scnprintf(str, end - str, "None\n");
        }

        /* Merber settings */
        str += scnprintf(str, end - str,
                         ASCII_COLOR_BLUE "---------------------------------------------\n" ASCII_COLOR_END);
        str += scnprintf(str, end - str, "Id:Client\tBurst\tPriority\n");
        for (mbr = 0; mbr < MIU_GRP_CLIENT_NUM; mbr++)
        {
            c = (g * MIU_GRP_CLIENT_NUM) + mbr;
            clientId_KernelProtectToName(c, clientName);

            if (!miu_client_reserved(c))
            {
                if ((greg.cfg & IGCFG_MBR_LIMIT_EN_BIT) && !(greg.mbr_nolimit & IGMBR_NOLIMIT_EN(mbr)))
                {
                    mbr_burst = halARBGetClientBurst(&greg, &greg_e, mbr, g);
                }
                else
                {
                    mbr_burst = 0; // not limit
                }
                if (greg.cfg & IGCFG_ROUND_ROBIN_BIT)
                {
                    mbr_prio = (greg.mbr_priority & IGMBR_PRIO_MASK(mbr)) >> IGMBR_PRIO_SHIFT(mbr);
                }
                else
                {
                    mbr_prio = 0xFF; // TODO: parse ID for fixed priority
                }
                str += scnprintf(str, end - str, "%2d:%s\t%d\t%d\n", c, clientName, mbr_burst, mbr_prio);
            }
        }
    }

    return str;
}

static char *_dump_as_reg(char *str, char *end, unsigned char miu)
{
    int                      g;
    int                      base, base_e;
    struct miu_arb_grp_reg   greg;
    struct miu_arb_grp_reg_e greg_e;

    base = _get_reg_base(miu);
    if (!base)
    {
        return str;
    }
    base_e = _get_reg_base_e();

    // miu0
    str += scnprintf(str, end - str, "    // miu%d\n", miu);
    str += scnprintf(str, end - str, "    {\n");

    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        _load_group_settings(&greg, base, g);
        str += scnprintf(str, end - str, "        {   // group %d\n", g);
        str += scnprintf(str, end - str, "            .cfg = 0x%04X,\n", greg.cfg);
        str += scnprintf(str, end - str, "            .burst = 0x%04X,\n", greg.burst);
        str += scnprintf(str, end - str, "            .flowctrl0 = 0x%04X,\n", greg.flowctrl0);
        str += scnprintf(str, end - str, "            .flowctrl1 = 0x%04X,\n", greg.flowctrl1);
        str += scnprintf(str, end - str, "            .mbr_priority = 0x%04X,\n", greg.mbr_priority);
        str += scnprintf(str, end - str, "            .mbr_nolimit = 0x%04X,\n", greg.mbr_nolimit);
        str += scnprintf(str, end - str, "        },\n");
    }

    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        _load_group_settings_e(&greg_e, base_e, g);
        str += scnprintf(str, end - str, "        {   // group %d\n", g);
        str += scnprintf(str, end - str, "            .burst_23 = 0x%04X,\n", greg_e.burst_23);
        str += scnprintf(str, end - str, "            .burst_4 = 0x%04X,\n", greg_e.burst_4);
        str += scnprintf(str, end - str, "            .mbr_sel0 = 0x%04X,\n", greg_e.mbr_sel0);
        str += scnprintf(str, end - str, "            .mbr_sel1 = 0x%04X,\n", greg_e.mbr_sel1);
        str += scnprintf(str, end - str, "        },\n");
    }
    str += scnprintf(str, end - str, "    },\n");
    str += scnprintf(str, end - str, "    .cfg = 0x%04X,\n", halARBGetOGCCFG(base));

    return str;
}

static ssize_t client_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned char m = MIU_IDX(dev->kobj.name[7]);
    u32           c = 0;

    c = simple_strtoul(buf, NULL, 10);
    if (c < MIU_ARB_CLIENT_NUM)
    {
        if (!miu_client_reserved(c))
        {
            arb_handle[m].client_selected = c;
        }
        else
        {
            printk(KERN_ERR "Invalid client %d\n", c);
        }
    }
    else
    {
        printk(KERN_ERR "Invalid client %d\n", c);
        return count;
    }
    return count;
}

static ssize_t client_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *        str = buf;
    char *        end = buf + PAGE_SIZE;
    int           c, g, i;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);
    char          clientName[40];

    memset(clientName, 0, sizeof(clientName));
    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        str += scnprintf(str, end - str, "ID:IP_name\t");
    }
    str += scnprintf(str, end - str, "\n");

    for (i = 0; i < MIU_GRP_CLIENT_NUM; i++)
    {
        for (g = 0; g < MIU_GRP_NUM; g++)
        {
            c = (g * MIU_GRP_CLIENT_NUM) + i;
            if (c != arb_handle[m].client_selected)
            {
                clientId_KernelProtectToName(c, clientName);
                if (!miu_client_reserved(c))
                {
                    str += scnprintf(str, end - str, "%2d:%s\t", c, clientName);
                }
                else
                {
                    str += scnprintf(str, end - str, "%2d:       \t", c);
                }
            }
            else
            {
                str += scnprintf(str, end - str, ASCII_COLOR_GREEN "%3d:%s\t" ASCII_COLOR_END, c, clientName);
            }
        }
        str += scnprintf(str, end - str, "\n");
    }
    return (str - buf);
}

DEVICE_ATTR(client, 0644, client_show, client_store);

static ssize_t group_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned char m = MIU_IDX(dev->kobj.name[7]);
    u32           g = 0;

    g = simple_strtoul(buf, NULL, 10);
    if (g < MIU_GRP_NUM)
    {
        arb_handle[m].group_selected = g;
    }
    else
    {
        printk(KERN_ERR "Invalid group %d\n", g);
        return count;
    }
    return count;
}

static ssize_t group_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *        str = buf;
    char *        end = buf + PAGE_SIZE;
    int           c, g, i;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);
    char          clientName[40];

    memset(clientName, 0, sizeof(clientName));

    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        str += scnprintf(str, end - str, "ID:IP_name\t");
    }
    str += scnprintf(str, end - str, "\n");

    for (i = 0; i < MIU_GRP_CLIENT_NUM; i++)
    {
        for (g = 0; g < MIU_GRP_NUM; g++)
        {
            c = (g * MIU_GRP_CLIENT_NUM) + i;
            if (g != arb_handle[m].group_selected)
            {
                if (!miu_client_reserved(c))
                {
                    clientId_KernelProtectToName(c, clientName);
                    str += scnprintf(str, end - str, "%2d:%s\t", c, clientName);
                }
                else
                {
                    str += scnprintf(str, end - str, "%2d:       \t", c);
                }
            }
            else
            {
                if (!miu_client_reserved(c))
                {
                    clientId_KernelProtectToName(c, clientName);
                    str += scnprintf(str, end - str, ASCII_COLOR_GREEN "%3d:%s\t" ASCII_COLOR_END, c, clientName);
                }
                else
                {
                    str += scnprintf(str, end - str, ASCII_COLOR_GREEN "%3d:       \t" ASCII_COLOR_END, c);
                }
            }
        }
        str += scnprintf(str, end - str, "\n");
    }
    return (str - buf);
}

DEVICE_ATTR(group, 0644, group_show, group_store);

static ssize_t group_burst_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int                    g, base, burst;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    burst = simple_strtoul(buf, NULL, 10);
    g     = arb_handle[m].group_selected;

    printk(KERN_ERR "Set group %d burst %d\n", g, burst);
    if (g < MIU_GRP_NUM)
    {
        base = _get_reg_base(m);
        if (!base)
        {
            return count;
        }

        _load_group_settings(&greg, base, g);
        halARBGroupBurstStore(&greg, base, g, burst);
    }
    return count;
}

static ssize_t group_burst_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, g;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    g = arb_handle[m].group_selected;
    if (g < MIU_GRP_NUM)
    {
        base = _get_reg_base(m);
        if (!base)
        {
            return (str - buf);
        }
        _load_group_settings(&greg, base, g);
        str += scnprintf(str, end - str, "Group %d burst: %d\n", g,
                         (greg.cfg & IGCFG_GRP_LIMIT_EN_BIT) ? (greg.burst & IGBURST_GRP_MASK) >> IGBURST_GRP_SHIFT
                                                             : 0); // no limit
    }

    return (str - buf);
}

DEVICE_ATTR(group_burst, 0644, group_burst_show, group_burst_store);

static ssize_t group_member_burst_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                        size_t count)
{
    int                      g, base, base_e, burst, max_serv_idx, cnt = 0;
    struct miu_arb_grp_reg   greg;
    struct miu_arb_grp_reg_e greg_e;
    unsigned char            m = MIU_IDX(dev->kobj.name[7]);
    char *                   pt;
    char *                   opt;

    burst = 0;
    pt    = kmalloc(strlen(buf) + 1, GFP_KERNEL);
    strcpy(pt, buf);
    while ((opt = strsep(&pt, ";, ")) != NULL)
    {
        if (cnt == 0)
        {
            max_serv_idx = simple_strtoul(opt, NULL, 10);
        }
        else if (cnt == 1)
        {
            burst = simple_strtoul(opt, NULL, 10);
        }
        cnt++;
    }
    kfree(pt);

    g = arb_handle[m].group_selected;

    // printk(KERN_ERR "Set group %d member burst %d\n", g, burst);
    printk(KERN_ERR "Set group %d ,max member service index %d, member burst %d\n", g, max_serv_idx, burst);
    if (g < MIU_GRP_NUM)
    {
        base = _get_reg_base(m);
        if (!base)
        {
            return count;
        }
        base_e = _get_reg_base_e();
        _load_group_settings(&greg, base, g);
        _load_group_settings_e(&greg_e, base_e, g);

        halARBGroupMemberBurstStore(&greg, base, g, max_serv_idx, burst);
    }
    return count;
}

static ssize_t group_member_burst_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                   str = buf;
    char *                   end = buf + PAGE_SIZE;
    int                      base, base_e, g;
    struct miu_arb_grp_reg   greg;
    struct miu_arb_grp_reg_e greg_e;
    unsigned char            m = MIU_IDX(dev->kobj.name[7]);

    g = arb_handle[m].group_selected;
    if (g < MIU_GRP_NUM)
    {
        base = _get_reg_base(m);
        if (!base)
        {
            return (str - buf);
        }
        base_e = _get_reg_base_e();

        _load_group_settings(&greg, base, g);
        _load_group_settings_e(&greg_e, base_e, g);

        if (greg.cfg & IGCFG_MBR_LIMIT_EN_BIT)
        {
            str += scnprintf(str, end - str, "Group %d member burst: %d %d %d %d\n", g,
                             (greg.burst & IGBURST_MBR_MASK) >> IGBURST_MBR_SHIFT,
                             (greg_e.burst_23 & IGBURST_MBR2_MASK) >> IGBURST_MBR2_SHIFT,
                             (greg_e.burst_23 & IGBURST_MBR3_MASK) >> IGBURST_MBR3_SHIFT,
                             (greg_e.burst_4 & IGBURST_MBR4_MASK) >> IGBURST_MBR4_SHIFT);
        }
        else
        {
            str += scnprintf(str, end - str, "Group %d member burst: %d\n", g, 0); // no limit
        }
    }

    return (str - buf);
}

DEVICE_ATTR(group_member_burst, 0644, group_member_burst_show, group_member_burst_store);

static ssize_t client_nolimit_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int                    c, g, base, nolimit;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    nolimit = simple_strtoul(buf, NULL, 10);
    c       = arb_handle[m].client_selected;
    g       = c / MIU_GRP_CLIENT_NUM;

    printk(KERN_ERR "Set client %d burst length to %s\n", c, nolimit ? "no limited" : "limited");
    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_reg_base(m);
        if (!base)
        {
            return count;
        }

        _load_group_settings(&greg, base, g);
        halARBClientNolimitStore(&greg, base, c, nolimit);
    }
    return count;
}

static ssize_t client_nolimit_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                   str = buf;
    char *                   end = buf + PAGE_SIZE;
    int                      base, c, g, mbr, burst, base_e;
    struct miu_arb_grp_reg   greg;
    struct miu_arb_grp_reg_e greg_e;
    unsigned char            m = MIU_IDX(dev->kobj.name[7]);

    c   = arb_handle[m].client_selected;
    g   = c / MIU_GRP_CLIENT_NUM;
    mbr = c % MIU_GRP_CLIENT_NUM;

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_reg_base(m);
        if (!base)
        {
            return (str - buf);
        }

        base_e = _get_reg_base_e();

        _load_group_settings(&greg, base, g);
        _load_group_settings_e(&greg_e, base_e, g);

        if (!(greg.cfg & IGCFG_MBR_LIMIT_EN_BIT) || (greg.mbr_nolimit & IGMBR_NOLIMIT_EN(mbr)))
        {
            // group member burst is no-limited or the specified member has limit mask to 1
            burst = 0;
        }
        else
        {
            burst = halARBGetClientBurst(&greg, &greg_e, mbr, g);
        }
        str += scnprintf(str, end - str, "Client %d burst length: %d, (0: no-limited)\n", c, burst);
    }

    return (str - buf);
}

DEVICE_ATTR(client_nolimit, 0644, client_nolimit_show, client_nolimit_store);

static ssize_t client_max_mbr_serv_idx_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                             size_t count)
{
    int                      c, g, mbr, base, base_e, max_serv_idx;
    struct miu_arb_grp_reg   greg;
    struct miu_arb_grp_reg_e greg_e;
    unsigned char            m = MIU_IDX(dev->kobj.name[7]);

    max_serv_idx = simple_strtoul(buf, NULL, 10);
    c            = arb_handle[m].client_selected;
    g            = c / MIU_GRP_CLIENT_NUM;
    mbr          = c % MIU_GRP_CLIENT_NUM;

    printk(KERN_ERR "Set client %d max member service index to (%d)\n", c, max_serv_idx);
    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_reg_base(m);
        if (!base)
        {
            return count;
        }
        base_e = _get_reg_base_e();

        _load_group_settings(&greg, base, g);
        _load_group_settings_e(&greg_e, base_e, g);

        if (max_serv_idx)
        {
            if (mbr <= 7)
            {
                greg_e.mbr_sel0 = (greg_e.mbr_sel0 & ~(IGMBR_SEL0_MASK(mbr))) | (max_serv_idx << IGMBR_SEL0_SHIFT(mbr));
                OUTREG16(base_e + REG_IGRP_MBR_SEL0(g), greg_e.mbr_sel0);
            }
            else
            {
                greg_e.mbr_sel1 = (greg_e.mbr_sel1 & ~(IGMBR_SEL1_MASK(mbr))) | (max_serv_idx << IGMBR_SEL1_SHIFT(mbr));
                OUTREG16(base_e + REG_IGRP_MBR_SEL1(g), greg_e.mbr_sel1);
            }
        }
    }
    return count;
}

static ssize_t client_max_mbr_serv_idx_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                   str = buf;
    char *                   end = buf + PAGE_SIZE;
    int                      base, base_e, c, g, mbr, max_serv_idx;
    struct miu_arb_grp_reg   greg;
    struct miu_arb_grp_reg_e greg_e;
    unsigned char            m = MIU_IDX(dev->kobj.name[7]);

    c   = arb_handle[m].client_selected;
    g   = c / MIU_GRP_CLIENT_NUM;
    mbr = c % MIU_GRP_CLIENT_NUM;

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_reg_base(m);
        if (!base)
        {
            return (str - buf);
        }
        base_e = _get_reg_base_e();

        _load_group_settings(&greg, base, g);
        _load_group_settings_e(&greg_e, base_e, g);

        max_serv_idx = halARBGetClientMaxMbrServIdx(&greg_e, mbr, g);

        str += scnprintf(str, end - str, "Client %d max member service index: %d\n", c, max_serv_idx);
    }

    return (str - buf);
}

DEVICE_ATTR(client_max_mbr_serv_idx, 0644, client_max_mbr_serv_idx_show, client_max_mbr_serv_idx_store);

static ssize_t client_prioprity_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int                    c, g, mbr, base, priority;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    priority = simple_strtoul(buf, NULL, 10);
    c        = arb_handle[m].client_selected;
    g        = c / MIU_GRP_CLIENT_NUM;
    mbr      = c % MIU_GRP_CLIENT_NUM;

    printk(KERN_ERR "Set client %d priority to %d\n", c, priority ? 1 : 0);
    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_reg_base(m);
        if (!base)
        {
            return count;
        }

        _load_group_settings(&greg, base, g);
        greg.mbr_priority = (greg.mbr_priority & ~(IGMBR_PRIO_MASK(mbr))) | IGMBR_PRIO(mbr, (priority ? 1 : 0));
        halARBClientPrioprityStore(&greg, base, g);
    }
    return count;
}

static ssize_t client_prioprity_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, g, mbr;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c   = arb_handle[m].client_selected;
    g   = c / MIU_GRP_CLIENT_NUM;
    mbr = c % MIU_GRP_CLIENT_NUM;

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_reg_base(m);
        if (!base)
        {
            return (str - buf);
        }

        _load_group_settings(&greg, base, g);
        str += scnprintf(str, end - str, "Client %d priority: %d\n", c,
                         ((greg.mbr_priority & IGMBR_PRIO_MASK(mbr)) >> IGMBR_PRIO_SHIFT(mbr)));
    }

    return (str - buf);
}

DEVICE_ATTR(client_priority, 0644, client_prioprity_show, client_prioprity_store);

static ssize_t client_flowctrl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int                    c, g, mbr, base;
    ssize_t                ret = count;
    u32                    enable, period, en_tmp;
    char                   peri_tmp;
    char *                 pt, *opt;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c = arb_handle[m].client_selected;

    if ((c == OVERALL_CLIENT_ID) || (c >= MIU_ARB_CLIENT_NUM) || miu_client_reserved(c))
    {
        printk(KERN_ERR "Invalid client %d, please set client by command: echo [id] > client\n", c);
        return count;
    }

    // check input parameters
    do
    {
        pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
        strcpy(pt, buf);
        if ((opt = strsep(&pt, ";, ")) != NULL)
        {
            enable = simple_strtoul(opt, NULL, 10);
            if (enable)
            {
                if ((opt = strsep(&pt, ";, ")) == NULL)
                {
                    ret = 0;
                    break;
                }
                period = simple_strtoul(opt, NULL, 10);
                if (!period || (period > (IGFCTL_PERIOD_MASK >> IGFCTL_PERIOD_SHIFT)))
                {
                    printk(KERN_ERR "Invalid period %d (1-%d)\n", period, (IGFCTL_PERIOD_MASK >> IGFCTL_PERIOD_SHIFT));
                    ret = 0;
                    break;
                }
            }
        }
        else
        {
            ret = 0;
        }
        break;
    } while (1);

    kfree(pt);
    if (ret == 0)
    {
        printk(KERN_ERR "Usage: echo [0/1] [period] > client_flowctrl\n");
        return count;
    }

    g    = c / MIU_GRP_CLIENT_NUM;
    mbr  = c % MIU_GRP_CLIENT_NUM;
    base = _get_reg_base(m);
    if (!base)
    {
        return count;
    }
    _load_group_settings(&greg, base, g);

    if (enable)
    {
        // to keep the original setting
        en_tmp = _is_flowctrl_enable(&greg, mbr, &peri_tmp);
        _enable_flowctrl(&greg, mbr, 0, 0);
        // restore the original settings if failed
        if (_enable_flowctrl(&greg, mbr, enable, period) && en_tmp)
        {
            _enable_flowctrl(&greg, mbr, en_tmp, peri_tmp);
        }
    }
    else
    {
        // disable client flow control
        _enable_flowctrl(&greg, mbr, 0, 0);
    }

    halARBClientFlowctrl(&greg, base, g);
    return count;
}

static ssize_t client_flowctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c[2], g, mbr[2], period;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char                   clientName[40];

    memset(clientName, 0, sizeof(clientName));
    str += scnprintf(str, end - str, "Flow Control:\n");
    str += scnprintf(str, end - str, "echo [id] > client\n");
    str += scnprintf(str, end - str, "enable:  echo 1 [period] > client_flowctrl\n");
    str += scnprintf(str, end - str, "disable: echo 0 > client_flowctrl\n");

    base = _get_reg_base(m);
    if (!base)
    {
        return (str - buf);
    }

    str += scnprintf(str, end - str, "\nNum:IP_name\t[period]\n");
    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        _load_group_settings(&greg, base, g);
        if (greg.cfg & IGCFG_FCTL0_EN_BIT)
        {
            mbr[0] = (greg.flowctrl0 & IGFCTL_ID0_MASK) >> IGFCTL_ID0_SHIFT;
            mbr[1] = (greg.flowctrl0 & IGFCTL_ID1_MASK) >> IGFCTL_ID1_SHIFT;
            period = (greg.flowctrl0 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            c[0]   = (g * MIU_GRP_CLIENT_NUM) + mbr[0];
            c[1]   = (g * MIU_GRP_CLIENT_NUM) + mbr[1];
            if (!miu_client_reserved(c[0]))
            {
                clientId_KernelProtectToName(c[0], clientName);
                str += scnprintf(str, end - str, "%3d:%s\t[  0x%02X]\n", c[0], clientName, period);
            }
            if (!miu_client_reserved(c[1]) && (c[1] != c[0]))
            {
                clientId_KernelProtectToName(c[1], clientName);
                str += scnprintf(str, end - str, "%3d:%s\t[  0x%02X]\n", c[1], clientName, period);
            }
        }
        if (greg.cfg & IGCFG_FCTL1_EN_BIT)
        {
            mbr[0] = (greg.flowctrl1 & IGFCTL_ID0_MASK) >> IGFCTL_ID0_SHIFT;
            mbr[1] = (greg.flowctrl1 & IGFCTL_ID1_MASK) >> IGFCTL_ID1_SHIFT;
            period = (greg.flowctrl1 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            c[0]   = (g * MIU_GRP_CLIENT_NUM) + mbr[0];
            c[1]   = (g * MIU_GRP_CLIENT_NUM) + mbr[1];
            if (!miu_client_reserved(c[0]))
            {
                clientId_KernelProtectToName(c[0], clientName);
                str += scnprintf(str, end - str, "%3d:%s\t[  0x%02X]\n", c[0], clientName, period);
            }
            if (!miu_client_reserved(c[1]) && (c[1] != c[0]))
            {
                clientId_KernelProtectToName(c[1], clientName);
                str += scnprintf(str, end - str, "%3d:%s\t[  0x%02X]\n", c[1], clientName, period);
            }
        }
    }

    return (str - buf);
}

DEVICE_ATTR(client_flowctrl, 0644, client_flowctrl_show, client_flowctrl_store);

static ssize_t dump_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    u32           input = 0;
    unsigned char m     = MIU_IDX(dev->kobj.name[7]);

    input = simple_strtoul(buf, NULL, 10);
    if (input >= MIU_ARB_DUMP_MAX)
    {
        printk(KERN_ERR "Invalid dump mode %d (0: text; 1: reg)\n", input);
        return count;
    }
    arb_handle[m].dump = input;
    return count;
}

static ssize_t dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *        str = buf;
    char *        end = buf + PAGE_SIZE;
    unsigned char m   = MIU_IDX(dev->kobj.name[7]);

    str += scnprintf(str, end - str, "Dump Settings:\n");
    str += scnprintf(str, end - str, "text: echo 0 > dump\n");
    str += scnprintf(str, end - str, "reg : echo 1 > dump\n\n");

    switch (arb_handle[m].dump)
    {
        case MIU_ARB_DUMP_TEXT:
            str = _dump_as_text(str, end, m);
            break;
        case MIU_ARB_DUMP_REG:
            str = _dump_as_reg(str, end, m);
            break;
        default:
            return 0;
    }
    return (str - buf);
}

DEVICE_ATTR(dump, 0644, dump_show, dump_store);

//////////////////////CONFIG_SUPPORT_ARB_v1//////////////////////
//////////////////////CONFIG_SUPPORT_ARB_v1//////////////////////
//////////////////////CONFIG_SUPPORT_ARB_v1//////////////////////
#ifdef CONFIG_SUPPORT_ARB_v1
static ssize_t priority_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    u32           input = 0;
    unsigned char m     = MIU_IDX(dev->kobj.name[7]);

    input = simple_strtoul(buf, NULL, 10);
    halARBPriority_store(input, m);
    return count;
}

static ssize_t priority_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *        str = buf;
    char *        end = buf + PAGE_SIZE;
    int           c;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);
    char          clientName[40];

    memset(clientName, 0, sizeof(clientName));
    str += scnprintf(str, end - str, "Priority (highest -> lowest):\n");
    str += scnprintf(str, end - str, "0, 1, 2, 3\n\n");

    c = halARBPolicySelect(m);

    if (c != MIU_ARB_CLIENT_ALL)
    {
        str += scnprintf(str, end - str, "Num:IP_name   [pri]\n");
        str += scnprintf(str, end - str, "%3d:%s[  %d]\n", c, clientId_KernelProtectToName(c, clientName),
                         halARBPriorityShow(m, c));
    }
    else
    {
        int g, i;

        for (g = 0; g < MIU_GRP_NUM; g++)
        {
            str += scnprintf(str, end - str, "Num:IP_name        ");
        }
        str += scnprintf(str, end - str, "\n");

        for (i = 0; i < MIU_GRP_CLIENT_NUM; i++)
        {
            for (g = 0; g < MIU_GRP_NUM; g++)
            {
                c = (g * MIU_GRP_CLIENT_NUM) + i;
                if (!miu_client_reserved(c))
                {
                    str += scnprintf(str, end - str, "%3d:%s[%d]  ", c, clientId_KernelProtectToName(c, clientName),
                                     halARBPriorityShow(m, c));
                }
                else
                {
                    str += scnprintf(str, end - str, "%3d:               ", c);
                }
            }
            str += scnprintf(str, end - str, "\n");
        }
    }
    return (str - buf);
}

DEVICE_ATTR(priority, 0644, priority_show, priority_store);

static ssize_t burst_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    u32           input = 0;
    unsigned char m     = MIU_IDX(dev->kobj.name[7]);

    input = simple_strtoul(buf, NULL, 10);
    halARBBurstStore(m);

    return count;
}

static ssize_t burst_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *        str = buf;
    char *        end = buf + PAGE_SIZE;
    int           i, c;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);
    char          clientName[40];

    memset(clientName, 0, sizeof(clientName));

    str += scnprintf(str, end - str, "Burst Option:\n");
    for (i = 0; i < MIU_ARB_BURST_NOLIM; i++)
    {
        str += scnprintf(str, end - str, " %d,", halARBGetBurstMapLen(i));
    }
    str += scnprintf(str, end - str, " %d(No Limited)\n\n", halARBGetBurstMapLen(MIU_ARB_BURST_NOLIM));

    c = halARBPolicySelect(m);

    if (c != MIU_ARB_CLIENT_ALL)
    {
        str += scnprintf(str, end - str, "Num:IP_name   [burst]\n");
        clientId_KernelProtectToName(c, clientName);

        if (halARBGetArbPolicyBurst(m, c) == halARBGetBurstMapLen(i))
        {
            str += scnprintf(str, end - str, "%3d:%s" ASCII_COLOR_RED "[   %2d]" ASCII_COLOR_END, c, clientName,
                             halARBGetArbPolicyBurst(m, c));
        }
        else
        {
            str += scnprintf(str, end - str, "%3d:%s[   %2d]", c, clientName, halARBGetArbPolicyBurst(m, c));
        }
        str += scnprintf(str, end - str, "\n");
    }
    else
    {
        int g;

        for (g = 0; g < MIU_GRP_NUM; g++)
        {
            str += scnprintf(str, end - str, "Num:IP_name            ");
        }
        str += scnprintf(str, end - str, "\n");

        for (i = 0; i < MIU_GRP_CLIENT_NUM; i++)
        {
            for (g = 0; g < MIU_GRP_NUM; g++)
            {
                c = (g * MIU_GRP_CLIENT_NUM) + i;
                if (!miu_client_reserved(c))
                {
                    clientId_KernelProtectToName(c, clientName);

                    if (halARBGetArbPolicyBurst(m, c) != halARBGetBurstMapLen(MIU_ARB_BURST_NOLIM))
                    {
                        str +=
                            scnprintf(str, end - str, "%3d:%s[   %2d]  ", c, clientName, halARBGetArbPolicyBurst(m, c));
                    }
                    else
                    {
                        str += scnprintf(str, end - str, "%3d:%s" ASCII_COLOR_RED "[   %2d]  " ASCII_COLOR_END, c,
                                         clientName, halARBGetArbPolicyBurst(m, c));
                    }
                }
                else
                {
                    str += scnprintf(str, end - str, "%3d:                   ", c);
                }
            }
            str += scnprintf(str, end - str, "\n");
        }
    }
    return (str - buf);
}

DEVICE_ATTR(burst, 0644, burst_show, burst_store);

static ssize_t promote_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int           c;
    u32           input = 0;
    unsigned char m     = MIU_IDX(dev->kobj.name[7]);

    input = simple_strtoul(buf, NULL, 10);
    input = input ? 1 : 0;

    c = halARBPolicySelect(m);
    if (c != MIU_ARB_CLIENT_ALL)
    {
        if (!miu_client_reserved(c))
        {
            halARBSetPromote(m, c, input);
        }
    }
    else
    {
        // all clients set to the same promote
        for (c = 0; c < MIU_ARB_CLIENT_NUM; c++)
        {
            halARBSetPromote(m, c, input);
        }
    }
    return count;
}

static ssize_t promote_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *        str = buf;
    char *        end = buf + PAGE_SIZE;
    int           c;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);
    char          clientName[40];

    memset(clientName, 0, sizeof(clientName));

    str += scnprintf(str, end - str, "Promote Setting:\n");
    str += scnprintf(str, end - str, "0 -> disable\n");
    str += scnprintf(str, end - str, "1 -> enable\n\n");

    c = halARBPolicySelect(m);

    if (c != MIU_ARB_CLIENT_ALL)
    {
        clientId_KernelProtectToName(c, clientName);
        str += scnprintf(str, end - str, "Num:IP_name   [promote]\n");
        str += scnprintf(str, end - str, "%3d:%s[      %d]\n", c, clientName, halARBGetPromote(m, c));
    }
    else
    {
        int i, g;

        for (g = 0; g < MIU_GRP_NUM; g++)
        {
            str += scnprintf(str, end - str, "Num:IP_name        ");
        }
        str += scnprintf(str, end - str, "\n");

        for (i = 0; i < MIU_GRP_CLIENT_NUM; i++)
        {
            for (g = 0; g < MIU_GRP_NUM; g++)
            {
                c = (g * MIU_GRP_CLIENT_NUM) + i;
                if (!miu_client_reserved(c))
                {
                    clientId_KernelProtectToName(c, clientName);
                    str += scnprintf(str, end - str, "%3d:%s[%d]  ", c, clientName, halARBGetPromote(m, c));
                }
                else
                {
                    str += scnprintf(str, end - str, "%3d:               ", c);
                }
            }
            str += scnprintf(str, end - str, "\n");
        }
    }
    return (str - buf);
}

DEVICE_ATTR(promote, 0644, promote_show, promote_store);

static ssize_t flowctrl_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int           c;
    ssize_t       ret = count;
    u32           enable, period, en_tmp;
    unsigned char peri_tmp;
    char *        pt, *opt;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);

    c = halARBPolicySelect(m);
    if ((c == MIU_ARB_CLIENT_ALL) || (c >= MIU_ARB_CLIENT_NUM) || miu_client_reserved(c))
        return count;

    // check input parameters
    do
    {
        pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
        strcpy(pt, buf);
        if ((opt = strsep(&pt, ";, ")) != NULL)
        {
            enable = simple_strtoul(opt, NULL, 10);
            if (enable)
            {
                if ((opt = strsep(&pt, ";, ")) == NULL)
                {
                    ret = 0;
                    break;
                }
                period = simple_strtoul(opt, NULL, 10);
                if (!period || (period > MIU_ARB_CNT_PERIOD_MAX))
                {
                    printk(KERN_ERR "Invalid period %d (1-%d)\n", period, MIU_ARB_CNT_PERIOD_MAX);
                    ret = 0;
                    break;
                }
            }
        }
        else
        {
            ret = 0;
        }
        break;
    } while (1);

    kfree(pt);
    if (ret == 0)
    {
        printk(KERN_ERR "Usage: echo [0/1] [period] > flowctrl\n");
        return count;
    }

    if (enable)
    {
        // to keep the original setting
        en_tmp = halARBFlowctrl_is_enable(m, c, &peri_tmp);
        halARBFlowctrl_enable(m, c, 0, 0);
        // restore the original settings if failed
        if (halARBFlowctrl_enable(m, c, enable, period) && en_tmp)
        {
            halARBFlowctrl_enable(m, c, en_tmp, peri_tmp);
        }
    }
    else
    {
        // disable client flow control
        halARBFlowctrl_enable(m, c, 0, 0);
    }

    return count;
}

static ssize_t flowctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *               str = buf;
    char *               end = buf + PAGE_SIZE;
    int                  c, g;
    struct arb_flowctrl *f;
    unsigned char        m = MIU_IDX(dev->kobj.name[7]);
    char                 clientName[40];

    memset(clientName, 0, sizeof(clientName));
    str += scnprintf(str, end - str, "Flow Control:\n");
    str += scnprintf(str, end - str, "echo [id] > client\n");
    str += scnprintf(str, end - str, "enable:  echo 1 [period] > flowctrl\n");
    str += scnprintf(str, end - str, "disable: echo 0 > flowctrl\n");

    str += scnprintf(str, end - str, "\nNum:IP_name   [period]\n");
    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        f = halARBGetFctrl(m, g);

        if (MIU_ARB_GET_CNT_EN(f->cnt0_enable) || MIU_ARB_GET_CNT_EN(f->cnt1_enable))
        {
            if (MIU_ARB_GET_CNT_ID0_EN(f->cnt0_enable))
            {
                c = (g * MIU_GRP_CLIENT_NUM) + MIU_ARB_GET_CNT_ID0(f->cnt0_id);
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%3d:%s[  0x%02X]\n", c, clientName, f->cnt0_period);
            }
            if (MIU_ARB_GET_CNT_ID1_EN(f->cnt0_enable))
            {
                c = (g * MIU_GRP_CLIENT_NUM) + MIU_ARB_GET_CNT_ID1(f->cnt0_id);
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%3d:%s[  0x%02X]\n", c, clientName, f->cnt0_period);
            }
            if (MIU_ARB_GET_CNT_ID0_EN(f->cnt1_enable))
            {
                c = (g * MIU_GRP_CLIENT_NUM) + MIU_ARB_GET_CNT_ID0(f->cnt1_id);
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%3d:%s[  0x%02X]\n", c, clientName, f->cnt1_period);
            }
            if (MIU_ARB_GET_CNT_ID1_EN(f->cnt1_enable))
            {
                c = (g * MIU_GRP_CLIENT_NUM) + MIU_ARB_GET_CNT_ID1(f->cnt1_id);
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%3d:%s[  0x%02X]\n", c, clientName, f->cnt1_period);
            }
        }
    }

    return (str - buf);
}

DEVICE_ATTR(flowctrl, 0644, flowctrl_show, flowctrl_store);

static ssize_t policy_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    u32 input = 0, i;

    input = simple_strtoul(buf, NULL, 10);
    if (input >= MIU_ARB_POLICY_NUM)
    {
        printk(KERN_ERR "Invalid policy %d\n", input);
        for (i = 0; i < MIU_ARB_POLICY_NUM; i++)
        {
            printk(KERN_ERR "%d: %s\n", halARBGetPolicyIdx(i), halARBGetPolicyName(i));
        }
        return count;
    }
    _load_policy(input);

    return count;
}

static ssize_t policy_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   i;

    str += scnprintf(str, end - str, "Policy:\n");
    for (i = 0; i < MIU_ARB_POLICY_NUM; i++)
    {
        if (i == halARBGetCurPolicy())
        {
            str += scnprintf(str, end - str, ASCII_COLOR_GREEN "%d: %s\n" ASCII_COLOR_END, halARBGetPolicyIdx(i),
                             halARBGetPolicyName(i));
        }
        else
        {
            str += scnprintf(str, end - str, "%d: %s\n", halARBGetPolicyIdx(i), halARBGetPolicyName(i));
        }
    }

    return (str - buf);
}

DEVICE_ATTR(policy, 0644, policy_show, policy_store);
#endif /////////CONFIG_SUPPORT_ARB_v1////////

#ifdef CONFIG_PM_SLEEP
void miu_arb_resume(void)
{
    halARBResume();
}
#endif

void create_miu_arb_node(struct bus_type *miu_subsys)
{
    int ret = 0, i;
    memset(arb_handle, 0, sizeof(arb_handle));

    for (i = 0; i < MIU_NUM; i++)
    {
        strcpy(arb_handle[i].name, "miu_arb0");
        arb_handle[i].name[7] += i;

        arb_dev[i].index         = 0;
        arb_dev[i].dev.kobj.name = (const char *)arb_handle[i].name;
        arb_dev[i].dev.bus       = miu_subsys;

        ret = device_register(&arb_dev[i].dev);
        if (ret)
        {
            printk(KERN_ERR "Failed to register %s device!! %d\n", arb_dev[i].dev.kobj.name, ret);
            return;
        }

#ifdef CONFIG_SUPPORT_ARB_v1
        device_create_file(&arb_dev[i].dev, &dev_attr_priority);
        device_create_file(&arb_dev[i].dev, &dev_attr_burst);
        device_create_file(&arb_dev[i].dev, &dev_attr_promote);
        device_create_file(&arb_dev[i].dev, &dev_attr_flowctrl);
        device_create_file(&arb_dev[i].dev, &dev_attr_policy);
#endif
        device_create_file(&arb_dev[i].dev, &dev_attr_client);
        device_create_file(&arb_dev[i].dev, &dev_attr_group);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_burst);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_member_burst);
        device_create_file(&arb_dev[i].dev, &dev_attr_client_nolimit);
        device_create_file(&arb_dev[i].dev, &dev_attr_client_max_mbr_serv_idx);
        device_create_file(&arb_dev[i].dev, &dev_attr_client_priority);
        device_create_file(&arb_dev[i].dev, &dev_attr_client_flowctrl);
        device_create_file(&arb_dev[i].dev, &dev_attr_dump);
    }
    halARBResume();
}
