/*
 * mdrv_sysfs_arb_rwcmd.c- Sigmastar
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
#include "mhal_miu_client.h"

static struct miu_device     arb_dev[MIU_NUM];
static struct miu_arb_handle arb_handle[MIU_NUM];

#if 0 // neet to fix
static char *_dump_as_text(char *str, char *end, unsigned char miu)
{
    int g, mbr, c, base, base_e;
    u16 outer_cfg, mbr_burst, mbr_prio;
    struct miu_arb_grp_reg greg;
    struct miu_arb_grp_reg_e greg_e;
    char clientName[40];

    memset(clientName, 0, sizeof(clientName));
    base = _get_reg_base(miu);
    if (!base) {
        return str;
    }

    outer_cfg = halARBGetOGCCFG(base);
    for(g = 0; g < MIU_GRP_NUM; g++)
    {
        /* Group settings */
        _load_group_settings(&greg, base, g);
        _load_group_settings_e(&greg_e, base_e, g);
        str += scnprintf(str, end - str, ASCII_COLOR_BLUE"==== Group %d ================================\n"ASCII_COLOR_END, g);
        str += scnprintf(str, end - str, " Outer-Prio: ");
        if (outer_cfg & OGCFG_ROUND_ROBIN_EN) {
            str += scnprintf(str, end - str, "Round-Robin\n");
        }
        else if (outer_cfg & OGCFG_FIX_PRIO_EN) {
            int p = 0;
            while(p < MIU_ARB_OG_PRIO_NUM) {
                if (((outer_cfg & OGCFG_GRP_PRIO_MASK(p)) >> OGCFG_GRP_PRIO_MASK(p)) == g) {
                    break;
                }
                p++;
            };
            str += scnprintf(str, end - str, "%d\n", p);
        }
        str += scnprintf(str, end - str, " Inner-Prio: %s\n", (greg.cfg & IGCFG_ROUND_ROBIN_BIT) ?
                                                         "Round-Robin" :
                                                         "Fix priority");
        str += scnprintf(str, end - str, " Burst     : %d\n", (greg.cfg & IGCFG_GRP_LIMIT_EN_BIT) ?
                                                         (greg.burst & IGBURST_GRP_MASK) >> IGBURST_GRP_SHIFT :
                                                         0); // no limit
        str += scnprintf(str, end - str, " FlowCtrl0 : ");
        if (greg.cfg & IGCFG_FCTL0_EN_BIT) {
            c = (g * MIU_GRP_CLIENT_NUM) + ((greg.flowctrl0 & IGFCTL_ID0_MASK) >> IGFCTL_ID0_SHIFT);
            if (!miu_client_reserved(c)) {
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%s, ", clientName);
            }
            c = (g * MIU_GRP_CLIENT_NUM) + ((greg.flowctrl0 & IGFCTL_ID1_MASK) >> IGFCTL_ID1_SHIFT);
            if (!miu_client_reserved(c)) {
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%s, ", clientName);
            }
            str += scnprintf(str, end - str, "Preiod 0x%02X\n", (greg.flowctrl0 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT);
        }
        else {
            str += scnprintf(str, end - str, "None\n");
        }
        str += scnprintf(str, end - str, " FlowCtrl1 : ");
        if (greg.cfg & IGCFG_FCTL1_EN_BIT) {
            c = (g * MIU_GRP_CLIENT_NUM) + ((greg.flowctrl1 & IGFCTL_ID0_MASK) >> IGFCTL_ID0_SHIFT);
            if (!miu_client_reserved(c)) {
				clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%s, ", clientName);
            }
            c = (g * MIU_GRP_CLIENT_NUM) + ((greg.flowctrl1 & IGFCTL_ID1_MASK) >> IGFCTL_ID1_SHIFT);
            if (!miu_client_reserved(c)) {
				clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%s, ", clientName);
            }
            str += scnprintf(str, end - str, "Preiod 0x%02X\n", (greg.flowctrl1 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT);
        }
        else {
            str += scnprintf(str, end - str, "None\n");
        }

        /* Merber settings */
        str += scnprintf(str, end - str, ASCII_COLOR_BLUE"---------------------------------------------\n"ASCII_COLOR_END);
        str += scnprintf(str, end - str, "Id:Client\tBurst\tPriority\n");
        for (mbr = 0; mbr < MIU_GRP_CLIENT_NUM; mbr++) {
            c = (g * MIU_GRP_CLIENT_NUM) + mbr;
			clientId_KernelProtectToName(c, clientName);

            if (!miu_client_reserved(c)) {
                if ((greg.cfg & IGCFG_MBR_LIMIT_EN_BIT) && !(greg.mbr_nolimit & IGMBR_NOLIMIT_EN(mbr))) {
                    mbr_burst = halARBGetClientBurst(&greg, &greg_e, mbr, g);
                }
                else {
                    mbr_burst = 0; // not limit
                }
                if (greg.cfg & IGCFG_ROUND_ROBIN_BIT) {
                    mbr_prio = (greg.mbr_priority & IGMBR_PRIO_MASK(mbr)) >> IGMBR_PRIO_SHIFT(mbr);
                }
                else {
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
    int g;
    int base, base_e;
    struct miu_arb_grp_reg greg;
    struct miu_arb_grp_reg_e greg_e;

    base = _get_reg_base(miu);
    if (!base) {
        return str;
    }
    base_e = _get_reg_base_e();

    // miu0
    str += scnprintf(str, end - str, "    // miu%d\n", miu);
    str += scnprintf(str, end - str, "    {\n");

    for(g = 0; g < MIU_GRP_NUM; g++)
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

    for(g = 0; g < MIU_GRP_NUM; g++)
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
#endif

short miu_client_wcmd_reserved(U16 id)
{
    return halARBClientWcmdReserved(id);
}

short miu_client_rcmd_reserved(U16 id)
{
    return halARBClientRcmdReserved(id);
}

/*=============================================================*/
// Local function
/*=============================================================*/
static int _get_grparb_reg_base(unsigned short grp_idx, unsigned short wcmd)
{
    return halGrpArbGetRegBase(grp_idx, wcmd);
}
static int _get_prearb_reg_base(unsigned short prearb_idx, unsigned short wcmd)
{
    return halPreArbGetRegBase(prearb_idx, wcmd);
}

static void _load_grp_arb_settings(struct miu_arb_grp_reg *greg, int base)
{
    halARBGroupLoadSetting(greg, base);
}

static void _load_pre_arb_settings(struct miu_arb_pre_reg *greg, int base)
{
    halARBPreLoadSetting(greg, base);
}

#ifdef CONFIG_MIU_FLOWCTRL
static bool _is_flowctrl_enable(struct miu_arb_grp_reg *greg, int mbr, char *period)
{
    return halARBIsGroupFlowCTLEnable(greg, mbr, period);
}

static bool _enable_flowctrl(struct miu_arb_grp_reg *greg, int mbr, bool enable, int period, int base)
{
    return halARBGroupEnableFlowCTL(greg, mbr, enable, period, base);
}

static bool _is_Preflowctrl_enable(struct miu_arb_pre_reg *greg, int mbr, char *period)
{
    return halARBIsPreFlowCTLEnable(greg, mbr, period);
}

static bool _enable_Preflowctrl(struct miu_arb_pre_reg *greg, int mbr, bool enable, char period, int base)
{
    return halARBPreEnableFlowCTL(greg, mbr, enable, period, base);
}
#endif

static ssize_t client_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned char m = MIU_IDX(dev->kobj.name[7]);
    u32           c = 0;

    c = simple_strtoul(buf, NULL, 10);
    printk("IP %x\r\n", c);
    if (c < MIU_ARB_CLIENT_NUM)
    {
        //        if (!miu_client_wcmd_reserved(c) || !miu_client_rcmd_reserved(c))
        {
            arb_handle[m].client_selected = c;
        }
        //        else
        //        {
        //            printk(KERN_ERR "Invalid reserved client %d for rcmd/wcmd\r\n", c);
        //        }
    }
    else
    {
        printk(KERN_ERR "Invalid client %d for rcmd/wcmd\r\n", c);
        return count;
    }
    return count;
}
DEVICE_ATTR(client, 0200, NULL, client_store);

static ssize_t client_wcmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *        str = buf;
    char *        end = buf + PAGE_SIZE;
    int           c, g, i;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);
    char          clientName[20];
    char          clientNULL[20];

    memset(clientName, 0, sizeof(clientName));
    memset(clientName, 0, sizeof(clientNULL));
    strcpy(clientNULL, "                ");
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
            clientId_KernelProtectToName(c, clientName);
            if (c != arb_handle[m].client_selected)
            {
                if (!miu_client_wcmd_reserved(c))
                {
                    str += scnprintf(str, end - str, "%2d:%s\t", c, clientName);
                }
                else
                {
                    str += scnprintf(str, end - str, "%2d:%s\t", c, clientNULL);
                }
            }
            else
            {
                str += scnprintf(str, end - str, ASCII_COLOR_GREEN "%2d:%s\t" ASCII_COLOR_END, c, clientName);
            }
        }
        str += scnprintf(str, end - str, "\n");
    }

    if (MIU_CLIENT_SPECIAL_ORDER_NUM > 0)
    {
        str += scnprintf(str, end - str, "\n====== MIU wcmd client table for special order ======\r\n");
        for (i = 0; i < MIU_CLIENT_SPECIAL_ORDER_NUM; i++)
        {
            c = halArbGetSpecOrderClientID(i);
            clientId_KernelProtectToName(c, clientName);
            if (c != arb_handle[m].client_selected)
                str += scnprintf(str, end - str, "%3d:%s\n", c, clientName);
            else
                str += scnprintf(str, end - str, ASCII_COLOR_GREEN "%3d:%s\n" ASCII_COLOR_END, c, clientName);
        }
        str += scnprintf(str, end - str, "\n");
    }

    return (str - buf);
}
DEVICE_ATTR(client_wcmd, 0444, client_wcmd_show, NULL);

static ssize_t client_rcmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *        str = buf;
    char *        end = buf + PAGE_SIZE;
    int           c, g, i;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);
    char          clientName[20];
    char          clientNULL[20];

    memset(clientName, 0, sizeof(clientName));
    memset(clientName, 0, sizeof(clientNULL));
    strcpy(clientNULL, "                ");
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
            rcmd_clientId_KernelProtectToName(c, clientName);
            if (c != arb_handle[m].client_selected)
            {
                if (!miu_client_rcmd_reserved(c))
                {
                    str += scnprintf(str, end - str, "%2d:%s\t", c, clientName);
                }
                else
                {
                    str += scnprintf(str, end - str, "%2d:%s\t", c, clientNULL);
                }
            }
            else
            {
                str += scnprintf(str, end - str, ASCII_COLOR_GREEN "%2d:%s\t" ASCII_COLOR_END, c, clientName);
            }
        }
        str += scnprintf(str, end - str, "\n");
    }

    if (MIU_CLIENT_SPECIAL_ORDER_NUM > 0)
    {
        str += scnprintf(str, end - str, "\n====== MIU rcmd client table for special order ======\r\n");
        for (i = 0; i < MIU_CLIENT_SPECIAL_ORDER_NUM; i++)
        {
            c = halArbGetSpecOrderClientID(i);
            clientId_KernelProtectToName(c, clientName);
            if (c != arb_handle[m].client_selected)
                str += scnprintf(str, end - str, "%3d:%s\n", c, clientName);
            else
                str += scnprintf(str, end - str, ASCII_COLOR_GREEN "%3d:%s\n" ASCII_COLOR_END, c, clientName);
        }
        str += scnprintf(str, end - str, "\n");
    }

    return (str - buf);
}
DEVICE_ATTR(client_rcmd, 0444, client_rcmd_show, NULL);

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
DEVICE_ATTR(group, 0200, NULL, group_store);

static ssize_t group_wcmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *        str = buf;
    char *        end = buf + PAGE_SIZE;
    int           c, g, i;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);
    char          clientName[20];
    char          clientNULL[20];

    memset(clientName, 0, sizeof(clientName));
    memset(clientName, 0, sizeof(clientNULL));
    strcpy(clientNULL, "                ");

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
                if (!miu_client_wcmd_reserved(c))
                {
                    clientId_KernelProtectToName(c, clientName);
                    str += scnprintf(str, end - str, "%2d:%s\t", c, clientName);
                }
                else
                {
                    str += scnprintf(str, end - str, "%2d:%s\t", c, clientNULL);
                }
            }
            else
            {
                if (!miu_client_wcmd_reserved(c))
                {
                    clientId_KernelProtectToName(c, clientName);
                    str += scnprintf(str, end - str, ASCII_COLOR_GREEN "%2d:%s\t" ASCII_COLOR_END, c, clientName);
                }
                else
                {
                    str += scnprintf(str, end - str, ASCII_COLOR_GREEN "%2d:%s\t" ASCII_COLOR_END, c, clientNULL);
                }
            }
        }
        str += scnprintf(str, end - str, "\n");
    }

    if (MIU_CLIENT_SPECIAL_ORDER_NUM > 0)
    {
        str += scnprintf(str, end - str, "\n====== MIU rcmd client table for special order ======\r\n");
        for (i = 0; i < MIU_CLIENT_SPECIAL_ORDER_NUM; i++)
        {
            c = halArbGetSpecOrderClientID(i);
            clientId_KernelProtectToName(c, clientName);
            str += scnprintf(str, end - str, "%3d:%s\n", c, clientName);
        }
        str += scnprintf(str, end - str, "\n");
    }

    return (str - buf);
}
DEVICE_ATTR(group_wcmd, 0444, group_wcmd_show, NULL);

static ssize_t group_rcmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *        str = buf;
    char *        end = buf + PAGE_SIZE;
    int           c, g, i;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);
    char          clientName[20];
    char          clientNULL[20];

    memset(clientName, 0, sizeof(clientName));
    memset(clientName, 0, sizeof(clientNULL));
    strcpy(clientNULL, "                ");

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
                if (!miu_client_rcmd_reserved(c))
                {
                    rcmd_clientId_KernelProtectToName(c, clientName);
                    str += scnprintf(str, end - str, "%2d:%s\t", c, clientName);
                }
                else
                {
                    str += scnprintf(str, end - str, "%2d:%s\t", c, clientNULL);
                }
            }
            else
            {
                if (!miu_client_rcmd_reserved(c))
                {
                    rcmd_clientId_KernelProtectToName(c, clientName);
                    str += scnprintf(str, end - str, ASCII_COLOR_GREEN "%3d:%s\t" ASCII_COLOR_END, c, clientName);
                }
                else
                {
                    str += scnprintf(str, end - str, ASCII_COLOR_GREEN "%3d:%s\t" ASCII_COLOR_END, c, clientNULL);
                }
            }
        }
        str += scnprintf(str, end - str, "\n");
    }

    if (MIU_CLIENT_SPECIAL_ORDER_NUM > 0)
    {
        str += scnprintf(str, end - str, "\n====== MIU rcmd client table for special order ======\r\n");
        for (i = 0; i < MIU_CLIENT_SPECIAL_ORDER_NUM; i++)
        {
            c = halArbGetSpecOrderClientID(i);
            clientId_KernelProtectToName(c, clientName);
            str += scnprintf(str, end - str, "%3d:%s\n", c, clientName);
        }
        str += scnprintf(str, end - str, "\n");
    }

    return (str - buf);
}
DEVICE_ATTR(group_rcmd, 0444, group_rcmd_show, NULL);

static ssize_t group_wcmd_order_buffer_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                             size_t count)
{
    int                    c, g, mbr, base, order_buf_idx;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char *                 pt;
    char *                 opt;

    c = arb_handle[m].client_selected;
    if (c < MIU_ARB_CLIENT_NUM)
    {
        g = halGroupArbGetClientGrpID(c);
    }
    mbr = c % MIU_GRP_CLIENT_NUM;

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_WCMD);
        if (!base)
        {
            printk(KERN_ERR "[wcmd] Not support group %d \n", g);
            return count;
        }
        _load_grp_arb_settings(&greg, base);

        pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
        strcpy(pt, buf);
        while ((opt = strsep(&pt, ";, ")) != NULL)
        {
            order_buf_idx = simple_strtoul(opt, NULL, 10);
            if (order_buf_idx >= MIU_GARB_WIN_NUM)
            {
                printk(KERN_ERR "[wcmd] Fail, set client %d to group wcmd order bufer index (%d > %d)\n", c,
                       order_buf_idx, MIU_GARB_WIN_NUM);
                continue;
            }
            halARBGroupOrderBufferStore(&greg, base, order_buf_idx, mbr);
            printk(KERN_ERR "[wcmd] Set client %d to group wcmd order bufer index (%d)\n", c, order_buf_idx);
        }
        kfree(pt);
    }
    return count;
}

static ssize_t group_wcmd_order_buffer_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   base, g;
    int   i, order_buf_data;
    // struct miu_arb_grp_reg greg;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);

    g = arb_handle[m].group_selected;

    if (g < MIU_GRP_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_WCMD);
        if (!base)
        {
            printk(KERN_ERR "[wcmd] Not support group %d \n", g);
            return (str - buf);
        }

        str += scnprintf(str, end - str, "=== group arbiter %d wcmd order buffer ===\n", g);
        for (i = 0; i < 64; i++)
        {
            if (i > 0 && (i % 8) == 0)
            {
                str += scnprintf(str, end - str, "\n");
            }

            if (i == 0 || (i % 8) == 0)
            {
                str += scnprintf(str, end - str, "%2X:", i);
            }
            order_buf_data = halARBGroupGetOrderBuffer(base, i);

            str += scnprintf(str, end - str, "0x%X ", order_buf_data);
        }

        str += scnprintf(str, end - str, "\n==========================================\n");
    }

    return (str - buf);
}
DEVICE_ATTR(group_wcmd_order_buffer, 0644, group_wcmd_order_buffer_show, group_wcmd_order_buffer_store);

static ssize_t group_rcmd_order_buffer_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                             size_t count)
{
    int                    c, g, mbr, base, order_buf_idx;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char *                 pt;
    char *                 opt;

    c = arb_handle[m].client_selected;
    if (c < MIU_ARB_CLIENT_NUM)
    {
        g = halGroupArbGetClientGrpID(c);
    }
    mbr = c % MIU_GRP_CLIENT_NUM;

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_RCMD);
        if (!base)
        {
            printk(KERN_ERR "[wcmd] Not support group %d \n", g);
            return count;
        }
        _load_grp_arb_settings(&greg, base);

        pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
        strcpy(pt, buf);
        while ((opt = strsep(&pt, ";, ")) != NULL)
        {
            order_buf_idx = simple_strtoul(opt, NULL, 10);
            if (order_buf_idx >= MIU_GARB_WIN_NUM)
            {
                printk(KERN_ERR "[rcmd] Fail, set client %d to group rcmd order bufer index (%d > %d)\n", c,
                       order_buf_idx, MIU_GARB_WIN_NUM);
                continue;
            }
            halARBGroupOrderBufferStore(&greg, base, order_buf_idx, mbr);
            printk(KERN_ERR "[rcmd] Set client %d to group rcmd order bufer index (%d)\n", c, order_buf_idx);
        }
        kfree(pt);
    }
    return count;
}

static ssize_t group_rcmd_order_buffer_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   base, g;
    int   i, order_buf_data;
    // struct miu_arb_grp_reg greg;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);

    g = arb_handle[m].group_selected;

    if (g < MIU_GRP_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_RCMD);
        if (!base)
        {
            printk(KERN_ERR "[wcmd] Not support group %d \n", g);
            return (str - buf);
        }

        str += scnprintf(str, end - str, "=== group arbiter %d rcmd order buffer ===\n", g);
        for (i = 0; i < 64; i++)
        {
            if (i > 0 && (i % 8) == 0)
            {
                str += scnprintf(str, end - str, "\n");
            }

            if (i == 0 || (i % 8) == 0)
            {
                str += scnprintf(str, end - str, "%2X:", i);
            }
            order_buf_data = halARBGroupGetOrderBuffer(base, i);

            str += scnprintf(str, end - str, "0x%X ", order_buf_data);
        }

        str += scnprintf(str, end - str, "\n==========================================\n");
    }

    return (str - buf);
}
DEVICE_ATTR(group_rcmd_order_buffer, 0644, group_rcmd_order_buffer_show, group_rcmd_order_buffer_store);

static ssize_t group_wcmd_member_burst_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                             size_t count)
{
    int                    g, base, burst, max_serv_idx, cnt = 0;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char *                 pt;
    char *                 opt;

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
    printk(KERN_ERR "[wcmd] Set group %d ,max member service index %d, member burst %d \n", g, max_serv_idx, burst);
    if (g < MIU_GRP_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_WCMD);
        if (!base)
        {
            printk(KERN_ERR "[wcmd] Not support group %d \n", g);
            return count;
        }
        _load_grp_arb_settings(&greg, base);
        halARBGroupBurstStore(&greg, base, max_serv_idx, burst);
    }
    return count;
}

static ssize_t group_wcmd_member_burst_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, g;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    g = arb_handle[m].group_selected;
    if (g < MIU_GRP_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_WCMD);
        if (!base)
        {
            printk(KERN_ERR "[wcmd] Not support group %d \n", g);
            return (str - buf);
        }
        _load_grp_arb_settings(&greg, base);

        if (greg.cfg & IGCFG_GRP_LIMIT_EN_BIT)
        {
            str += scnprintf(str, end - str, "[wcmd] Group %d member burst: %d %d %d %d\n", g,
                             (greg.burst & IGBURST_LSB_MASK) >> IGBURST_LSB_SHIFT,
                             (greg.burst & IGBURST_MSB_MASK) >> IGBURST_MSB_SHIFT,
                             (greg.burst_1 & IGBURST_LSB_MASK) >> IGBURST_LSB_SHIFT,
                             (greg.burst_1 & IGBURST_MSB_MASK) >> IGBURST_MSB_SHIFT);
        }
        else
        {
            str += scnprintf(str, end - str, "[wcmd] Group %d member burst: %d\n", g, 0); // no limit
        }
    }

    return (str - buf);
}
DEVICE_ATTR(group_wcmd_member_burst, 0644, group_wcmd_member_burst_show, group_wcmd_member_burst_store);

static ssize_t group_rcmd_member_burst_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                             size_t count)
{
    int                    g, base, burst, max_serv_idx, cnt = 0;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char *                 pt;
    char *                 opt;

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

    printk(KERN_ERR "[rcmd] Set group %d ,max member service index %d, member burst %d\n", g, max_serv_idx, burst);
    if (g < MIU_GRP_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_RCMD);
        if (!base)
        {
            printk(KERN_ERR "[rcmd] Not support group %d \n", g);
            return count;
        }
        _load_grp_arb_settings(&greg, base);
        halARBGroupBurstStore(&greg, base, max_serv_idx, burst);
    }
    return count;
}

static ssize_t group_rcmd_member_burst_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, g;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    g = arb_handle[m].group_selected;
    if (g < MIU_GRP_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_RCMD);
        if (!base)
        {
            printk(KERN_ERR "[rcmd] Not support group %d \n", g);
            return (str - buf);
        }
        _load_grp_arb_settings(&greg, base);

        if (greg.cfg & IGCFG_GRP_LIMIT_EN_BIT)
        {
            str += scnprintf(str, end - str, "[rcmd] Group %d member burst: %d %d %d %d\n", g,
                             (greg.burst & IGBURST_LSB_MASK) >> IGBURST_LSB_SHIFT,
                             (greg.burst & IGBURST_MSB_MASK) >> IGBURST_MSB_SHIFT,
                             (greg.burst_1 & IGBURST_LSB_MASK) >> IGBURST_LSB_SHIFT,
                             (greg.burst_1 & IGBURST_MSB_MASK) >> IGBURST_MSB_SHIFT);
        }
        else
        {
            str += scnprintf(str, end - str, "[rcmd] Group %d member burst: %d\n", g, 0); // no limit
        }
    }

    return (str - buf);
}
DEVICE_ATTR(group_rcmd_member_burst, 0644, group_rcmd_member_burst_show, group_rcmd_member_burst_store);

static ssize_t group_wcmd_client_nolimit_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                               size_t count)
{
    int                    c, g, base, nolimit;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    nolimit = simple_strtoul(buf, NULL, 10);
    c       = arb_handle[m].client_selected;
    g       = halGroupArbGetClientGrpID(c);

    printk(KERN_ERR "[wcmd] Set client %d burst length to %s\n", c, nolimit ? "no limited" : "limited");
    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_WCMD);
        if (!base)
        {
            printk(KERN_ERR "[wcmd] Not support group %d \n", g);
            return count;
        }

        _load_grp_arb_settings(&greg, base);
        halARBGroupClientNolimitStore(&greg, base, c, nolimit);
    }
    return count;
}

static ssize_t group_wcmd_client_nolimit_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, g, mbr, burst;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c   = arb_handle[m].client_selected;
    g   = halGroupArbGetClientGrpID(c);
    mbr = c % MIU_GRP_CLIENT_NUM;

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_WCMD);
        if (!base)
        {
            printk(KERN_ERR "[wcmd] Not support group %d \n", g);
            return (str - buf);
        }

        _load_grp_arb_settings(&greg, base);

        if (!(greg.cfg & IGCFG_GRP_LIMIT_EN_BIT) || (greg.mbr_nolimit & IGMBR_NOLIMIT_EN(mbr)))
        {
            // group member burst is no-limited or the specified member has limit mask to 1
            burst = 0;
        }
        else
        {
            burst = halARBGroupGetClientBurst(&greg, mbr);
        }
        str += scnprintf(str, end - str, "[wcmd] Client %d burst length: %d, (0: no-limited)\n", c, burst);
    }

    return (str - buf);
}
DEVICE_ATTR(group_wcmd_client_nolimit, 0644, group_wcmd_client_nolimit_show, group_wcmd_client_nolimit_store);

static ssize_t group_rcmd_client_nolimit_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                               size_t count)
{
    int                    c, g, base, nolimit;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    nolimit = simple_strtoul(buf, NULL, 10);
    c       = arb_handle[m].client_selected;
    g       = halGroupArbGetClientGrpID(c);

    printk(KERN_ERR "[rcmd] Set client %d burst length to %s\n", c, nolimit ? "no limited" : "limited");
    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_RCMD);
        if (!base)
        {
            printk(KERN_ERR "[rcmd] Not support group %d \n", g);
            return count;
        }

        _load_grp_arb_settings(&greg, base);
        halARBGroupClientNolimitStore(&greg, base, c, nolimit);
        // halARBClientNolimitStore(&greg, base, c, nolimit);
    }
    return count;
}

static ssize_t group_rcmd_client_nolimit_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, g, mbr, burst;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c   = arb_handle[m].client_selected;
    g   = halGroupArbGetClientGrpID(c);
    mbr = c % MIU_GRP_CLIENT_NUM;

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_RCMD);
        if (!base)
        {
            printk(KERN_ERR "[rcmd] Not support group %d \n", g);
            return (str - buf);
        }

        _load_grp_arb_settings(&greg, base);

        if (!(greg.cfg & IGCFG_GRP_LIMIT_EN_BIT) || (greg.mbr_nolimit & IGMBR_NOLIMIT_EN(mbr)))
        {
            // group member burst is no-limited or the specified member has limit mask to 1
            burst = 0;
        }
        else
        {
            burst = halARBGroupGetClientBurst(&greg, mbr);
        }
        str += scnprintf(str, end - str, "[rcmd] Client %d burst length: %d, (0: no-limited)\n", c, burst);
    }

    return (str - buf);
}
DEVICE_ATTR(group_rcmd_client_nolimit, 0644, group_rcmd_client_nolimit_show, group_rcmd_client_nolimit_store);

static ssize_t group_wcmd_client_burst_sel_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                 size_t count)
{
    int                    c, g, mbr, base, max_serv_idx;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    max_serv_idx = simple_strtoul(buf, NULL, 10);
    c            = arb_handle[m].client_selected;
    g            = halGroupArbGetClientGrpID(c);
    mbr          = c % MIU_GRP_CLIENT_NUM;

    printk(KERN_ERR "[wcmd] Set client %d max member service index to (%d)\n", c, max_serv_idx);
    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_WCMD);
        if (!base)
        {
            printk(KERN_ERR "[wcmd] Not support group %d \n", g);
            return count;
        }

        _load_grp_arb_settings(&greg, base);

        halARBGroupClientBurstSelStore(&greg, base, max_serv_idx, mbr);
    }
    return count;
}

static ssize_t group_wcmd_client_burst_sel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, g, mbr, max_serv_idx;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c   = arb_handle[m].client_selected;
    g   = halGroupArbGetClientGrpID(c);
    mbr = c % MIU_GRP_CLIENT_NUM;

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_WCMD);
        if (!base)
        {
            printk(KERN_ERR "[wcmd] Not support group %d \n", g);
            return (str - buf);
        }

        _load_grp_arb_settings(&greg, base);

        max_serv_idx = halARBGroupGetClientBurstSel(&greg, mbr);

        str += scnprintf(str, end - str, "[wcmd] Client %d max member service index: %d\n", c, max_serv_idx);
    }

    return (str - buf);
}
DEVICE_ATTR(group_wcmd_client_burst_sel, 0644, group_wcmd_client_burst_sel_show, group_wcmd_client_burst_sel_store);

static ssize_t group_rcmd_client_burst_sel_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                 size_t count)
{
    int                    c, g, mbr, base, max_serv_idx;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    max_serv_idx = simple_strtoul(buf, NULL, 10);
    c            = arb_handle[m].client_selected;
    g            = halGroupArbGetClientGrpID(c);
    mbr          = c % MIU_GRP_CLIENT_NUM;

    printk(KERN_ERR "[rcmd] Set client %d max member service index to (%d)\n", c, max_serv_idx);
    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_RCMD);
        if (!base)
        {
            printk(KERN_ERR "[rcmd] Not support group %d \n", g);
            return count;
        }

        _load_grp_arb_settings(&greg, base);

        halARBGroupClientBurstSelStore(&greg, base, max_serv_idx, mbr);
    }
    return count;
}

static ssize_t group_rcmd_client_burst_sel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, g, mbr, max_serv_idx;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c   = arb_handle[m].client_selected;
    g   = halGroupArbGetClientGrpID(c);
    mbr = c % MIU_GRP_CLIENT_NUM;

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_grparb_reg_base(g, MIU_CH_RCMD);
        if (!base)
        {
            printk(KERN_ERR "[rcmd] Not support group %d \n", g);
            return (str - buf);
        }

        _load_grp_arb_settings(&greg, base);

        max_serv_idx = halARBGroupGetClientBurstSel(&greg, mbr);

        str += scnprintf(str, end - str, "[rcmd] Client %d max member service index: %d\n", c, max_serv_idx);
    }

    return (str - buf);
}
DEVICE_ATTR(group_rcmd_client_burst_sel, 0644, group_rcmd_client_burst_sel_show, group_rcmd_client_burst_sel_store);

static ssize_t prearb_wcmd_order_buffer_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                              size_t count)
{
    int                    c, pre_arb, mbr, base, order_buf_idx;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char *                 pt;
    char *                 opt;

    c = arb_handle[m].client_selected;
    if (c < MIU_ARB_CLIENT_NUM)
    {
        pre_arb = halArbPreGetID(c);
    }
    mbr = halGroupArbGetClientGrpID(c);

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_WCMD);
        if (!base)
        {
            return count;
        }
        _load_pre_arb_settings(&greg, base);

        pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
        strcpy(pt, buf);
        while ((opt = strsep(&pt, ";, ")) != NULL)
        {
            order_buf_idx = simple_strtoul(opt, NULL, 10);
            if (order_buf_idx >= MIU_GARB_WIN_NUM)
            {
                printk(KERN_ERR "[wcmd] Fail, set group ch=%d to pre-arbter %d order bufer index (%d > %d)\n", mbr,
                       pre_arb, order_buf_idx, MIU_GARB_WIN_NUM);
                continue;
            }
            halARBPreOrderBufferStore(&greg, base, order_buf_idx, mbr);
            printk(KERN_ERR "[wcmd] Set group ch=%d to pre-arbter %d order bufer index (%d)\n", mbr, pre_arb,
                   order_buf_idx);
        }
        kfree(pt);
    }
    return count;
}

static ssize_t prearb_wcmd_order_buffer_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   base, c, pre_arb;
    int   i, order_buf_data;
    // struct miu_arb_grp_reg greg;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);

    c       = arb_handle[m].client_selected;
    pre_arb = halArbPreGetID(c);

    if (pre_arb < MIU_PREARB_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_WCMD);
        if (!base)
        {
            return (str - buf);
        }

        str += scnprintf(str, end - str, "=== pre arbiter %d wcmd order buffer ===\n", pre_arb);
        for (i = 0; i < 64; i++)
        {
            if (i > 0 && (i % 8) == 0)
            {
                str += scnprintf(str, end - str, "\n");
            }

            if (i == 0 || (i % 8) == 0)
            {
                str += scnprintf(str, end - str, "%2X:", i);
            }
            order_buf_data = halARBPreGetOrderBuffer(base, i);

            str += scnprintf(str, end - str, "0x%X ", order_buf_data);
        }

        str += scnprintf(str, end - str, "\n==========================================\n");
    }

    return (str - buf);
}
DEVICE_ATTR(prearb_wcmd_order_buffer, 0644, prearb_wcmd_order_buffer_show, prearb_wcmd_order_buffer_store);

static ssize_t prearb_rcmd_order_buffer_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                              size_t count)
{
    int                    c, pre_arb, mbr, base, order_buf_idx;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char *                 pt;
    char *                 opt;

    c = arb_handle[m].client_selected;
    if (c < MIU_ARB_CLIENT_NUM)
    {
        pre_arb = halArbPreGetID(c);
    }
    mbr = halGroupArbGetClientGrpID(c);

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_RCMD);
        if (!base)
        {
            return count;
        }
        _load_pre_arb_settings(&greg, base);

        pt = kmalloc(strlen(buf) + 1, GFP_KERNEL);
        strcpy(pt, buf);
        while ((opt = strsep(&pt, ";, ")) != NULL)
        {
            order_buf_idx = simple_strtoul(opt, NULL, 10);
            if (order_buf_idx >= MIU_GARB_WIN_NUM)
            {
                printk(KERN_ERR "[rcmd] Fail, set group ch=%d to pre-arbter %d order bufer index (%d > %d)\n", mbr,
                       pre_arb, order_buf_idx, MIU_GARB_WIN_NUM);
                continue;
            }
            halARBPreOrderBufferStore(&greg, base, order_buf_idx, mbr);
            printk(KERN_ERR "[rcmd] Set group ch=%d to pre-arbter %d order bufer index (%d)\n", mbr, pre_arb,
                   order_buf_idx);
        }
        kfree(pt);
    }
    return count;
}

static ssize_t prearb_rcmd_order_buffer_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    int   base, c, pre_arb;
    int   i, order_buf_data;
    // struct miu_arb_grp_reg greg;
    unsigned char m = MIU_IDX(dev->kobj.name[7]);

    c       = arb_handle[m].client_selected;
    pre_arb = halArbPreGetID(c);

    if (pre_arb < MIU_PREARB_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_RCMD);
        if (!base)
        {
            return (str - buf);
        }

        str += scnprintf(str, end - str, "=== pre arbiter %d rcmd order buffer ===\n", pre_arb);
        for (i = 0; i < 64; i++)
        {
            if (i > 0 && (i % 8) == 0)
            {
                str += scnprintf(str, end - str, "\n");
            }

            if (i == 0 || (i % 8) == 0)
            {
                str += scnprintf(str, end - str, "%2X:", i);
            }
            order_buf_data = halARBPreGetOrderBuffer(base, i);

            str += scnprintf(str, end - str, "0x%X ", order_buf_data);
        }

        str += scnprintf(str, end - str, "\n==========================================\n");
    }

    return (str - buf);
}
DEVICE_ATTR(prearb_rcmd_order_buffer, 0644, prearb_rcmd_order_buffer_show, prearb_rcmd_order_buffer_store);

static ssize_t prearb_wcmd_member_burst_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                              size_t count)
{
    int                    c, pre_arb, base, burst, max_serv_idx, cnt = 0;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char *                 pt;
    char *                 opt;

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

    c       = arb_handle[m].client_selected;
    pre_arb = halArbPreGetID(c);

    printk(KERN_ERR "[wcmd] Set pre-arbiter %d ,max member service index %d, member burst %d \n", pre_arb, max_serv_idx,
           burst);
    if (pre_arb < MIU_PREARB_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_WCMD);
        if (!base)
        {
            return count;
        }
        _load_pre_arb_settings(&greg, base);
        halARBPreBurstStore(&greg, base, max_serv_idx, burst);
    }
    return count;
}

static ssize_t prearb_wcmd_member_burst_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, pre_arb;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c       = arb_handle[m].client_selected;
    pre_arb = halArbPreGetID(c);

    if (pre_arb < MIU_PREARB_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_WCMD);
        if (!base)
        {
            return (str - buf);
        }
        _load_pre_arb_settings(&greg, base);

        if (greg.cfg & PREACFG_GRP_LIMIT_EN_BIT)
        {
            str += scnprintf(str, end - str, "[wcmd] Pre-arbiter %d member burst: %d %d %d %d\n", pre_arb,
                             (greg.burst & PREABURST_LSB_MASK) >> PREABURST_LSB_SHIFT,
                             (greg.burst & PREABURST_MSB_MASK) >> PREABURST_MSB_SHIFT,
                             (greg.burst_1 & PREABURST_LSB_MASK) >> PREABURST_LSB_SHIFT,
                             (greg.burst_1 & PREABURST_MSB_MASK) >> PREABURST_MSB_SHIFT);
        }
        else
        {
            str += scnprintf(str, end - str, "[wcmd] Pre-arbiter %d member burst: %d\n", pre_arb, 0); // no limit
        }
    }

    return (str - buf);
}
DEVICE_ATTR(prearb_wcmd_member_burst, 0644, prearb_wcmd_member_burst_show, prearb_wcmd_member_burst_store);

static ssize_t prearb_rcmd_member_burst_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                              size_t count)
{
    int                    c, pre_arb, base, burst, max_serv_idx, cnt = 0;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char *                 pt;
    char *                 opt;

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

    c       = arb_handle[m].client_selected;
    pre_arb = halArbPreGetID(c);

    printk(KERN_ERR "[rcmd] Set pre-arbiter %d ,max member service index %d, member burst %d \n", pre_arb, max_serv_idx,
           burst);
    if (pre_arb < MIU_PREARB_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_RCMD);
        if (!base)
        {
            return count;
        }
        _load_pre_arb_settings(&greg, base);
        halARBPreBurstStore(&greg, base, max_serv_idx, burst);
    }
    return count;
}

static ssize_t prearb_rcmd_member_burst_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, pre_arb;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c       = arb_handle[m].client_selected;
    pre_arb = halArbPreGetID(c);

    if (pre_arb < MIU_PREARB_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_RCMD);
        if (!base)
        {
            return (str - buf);
        }
        _load_pre_arb_settings(&greg, base);

        if (greg.cfg & PREACFG_GRP_LIMIT_EN_BIT)
        {
            str += scnprintf(str, end - str, "[rcmd] Pre-arbiter %d member burst: %d %d %d %d\n", pre_arb,
                             (greg.burst & PREABURST_LSB_MASK) >> PREABURST_LSB_SHIFT,
                             (greg.burst & PREABURST_MSB_MASK) >> PREABURST_MSB_SHIFT,
                             (greg.burst_1 & PREABURST_LSB_MASK) >> PREABURST_LSB_SHIFT,
                             (greg.burst_1 & PREABURST_MSB_MASK) >> PREABURST_MSB_SHIFT);
        }
        else
        {
            str += scnprintf(str, end - str, "[rcmd] Pre-arbiter %d member burst: %d\n", pre_arb, 0); // no limit
        }
    }

    return (str - buf);
}
DEVICE_ATTR(prearb_rcmd_member_burst, 0644, prearb_rcmd_member_burst_show, prearb_rcmd_member_burst_store);

static ssize_t prearb_wcmd_client_nolimit_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                size_t count)
{
    int                    c, mbr, pre_arb, base, nolimit;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    nolimit = simple_strtoul(buf, NULL, 10);
    c       = arb_handle[m].client_selected;
    pre_arb = halArbPreGetID(c);
    mbr     = halGroupArbGetClientGrpID(c);

    printk(KERN_ERR "[wcmd] Set group ch=%d burst length to %s\n", mbr, nolimit ? "no limited" : "limited");
    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_WCMD);
        if (!base)
        {
            return count;
        }

        _load_pre_arb_settings(&greg, base);
        halARBPreClientNolimitStore(&greg, base, c, nolimit);
    }
    return count;
}

static ssize_t prearb_wcmd_client_nolimit_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, pre_arb, mbr, burst;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c       = arb_handle[m].client_selected;
    pre_arb = halArbPreGetID(c);
    mbr     = halGroupArbGetClientGrpID(c);

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_WCMD);
        if (!base)
        {
            return (str - buf);
        }

        _load_pre_arb_settings(&greg, base);

        if (!(greg.cfg & PREACFG_GRP_LIMIT_EN_BIT) || (greg.mbr_nolimit & PREAMBR_NOLIMIT_EN(mbr)))
        {
            // group member burst is no-limited or the specified member has limit mask to 1
            burst = 0;
        }
        else
        {
            burst = halARBPreGetClientBurst(&greg, mbr);
        }
        str += scnprintf(str, end - str, "[wcmd] Group ch=%d burst length: %d for pre-arbiter %d, (0: no-limited)\n",
                         mbr, burst, pre_arb);
    }

    return (str - buf);
}
DEVICE_ATTR(prearb_wcmd_client_nolimit, 0644, prearb_wcmd_client_nolimit_show, prearb_wcmd_client_nolimit_store);

static ssize_t prearb_rcmd_client_nolimit_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                size_t count)
{
    int                    c, mbr, pre_arb, base, nolimit;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    nolimit = simple_strtoul(buf, NULL, 10);
    c       = arb_handle[m].client_selected;
    pre_arb = halArbPreGetID(c);
    mbr     = halGroupArbGetClientGrpID(c);

    printk(KERN_ERR "[rcmd] Set group ch=%d burst length to %s\n", mbr, nolimit ? "no limited" : "limited");
    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_RCMD);
        if (!base)
        {
            return count;
        }

        _load_pre_arb_settings(&greg, base);
        halARBPreClientNolimitStore(&greg, base, c, nolimit);
    }
    return count;
}

static ssize_t prearb_rcmd_client_nolimit_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, pre_arb, mbr, burst;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c       = arb_handle[m].client_selected;
    pre_arb = halArbPreGetID(c);
    mbr     = halGroupArbGetClientGrpID(c);

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_RCMD);
        if (!base)
        {
            return (str - buf);
        }

        _load_pre_arb_settings(&greg, base);

        if (!(greg.cfg & PREACFG_GRP_LIMIT_EN_BIT) || (greg.mbr_nolimit & PREAMBR_NOLIMIT_EN(mbr)))
        {
            // group member burst is no-limited or the specified member has limit mask to 1
            burst = 0;
        }
        else
        {
            burst = halARBPreGetClientBurst(&greg, mbr);
        }
        str += scnprintf(str, end - str, "[rcmd] Group ch=%d burst length: %d for pre-arbiter %d, (0: no-limited)\n",
                         mbr, burst, pre_arb);
    }

    return (str - buf);
}
DEVICE_ATTR(prearb_rcmd_client_nolimit, 0644, prearb_rcmd_client_nolimit_show, prearb_rcmd_client_nolimit_store);

static ssize_t prearb_wcmd_client_burst_sel_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                  size_t count)
{
    int                    c, pre_arb, mbr, base, max_serv_idx;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    max_serv_idx = simple_strtoul(buf, NULL, 10);
    c            = arb_handle[m].client_selected;
    pre_arb      = halArbPreGetID(c);
    mbr          = halGroupArbGetClientGrpID(c);

    printk(KERN_ERR "[wcmd] Set group ch=%d max member service index to (%d) for pre-arbiter %d\n", mbr, max_serv_idx,
           pre_arb);
    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_WCMD);
        if (!base)
        {
            return count;
        }

        _load_pre_arb_settings(&greg, base);

        halARBPreClientBurstSelStore(&greg, base, max_serv_idx, mbr);
    }
    return count;
}

static ssize_t prearb_wcmd_client_burst_sel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, pre_arb, mbr, max_serv_idx;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c       = arb_handle[m].client_selected;
    pre_arb = halArbPreGetID(c);
    mbr     = halGroupArbGetClientGrpID(c);

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_WCMD);
        if (!base)
        {
            return (str - buf);
        }

        _load_pre_arb_settings(&greg, base);

        max_serv_idx = halARBPreGetClientBurstSel(&greg, mbr);

        str += scnprintf(str, end - str, "[wcmd] Group ch=%d max member service index: %d for pre-arbiter %d\n", mbr,
                         max_serv_idx, pre_arb);
    }

    return (str - buf);
}
DEVICE_ATTR(prearb_wcmd_client_burst_sel, 0644, prearb_wcmd_client_burst_sel_show, prearb_wcmd_client_burst_sel_store);

static ssize_t prearb_rcmd_client_burst_sel_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                  size_t count)
{
    int                    c, pre_arb, mbr, base, max_serv_idx;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    max_serv_idx = simple_strtoul(buf, NULL, 10);
    c            = arb_handle[m].client_selected;
    pre_arb      = halArbPreGetID(c);
    mbr          = halGroupArbGetClientGrpID(c);

    printk(KERN_ERR "[rcmd] Set group ch=%d max member service index to (%d) for pre-arbiter %d\n", mbr, max_serv_idx,
           pre_arb);
    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_RCMD);
        if (!base)
        {
            return count;
        }

        _load_pre_arb_settings(&greg, base);

        halARBPreClientBurstSelStore(&greg, base, max_serv_idx, mbr);
    }
    return count;
}

static ssize_t prearb_rcmd_client_burst_sel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, pre_arb, mbr, max_serv_idx;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c       = arb_handle[m].client_selected;
    pre_arb = halArbPreGetID(c);
    mbr     = halGroupArbGetClientGrpID(c);

    if (c < MIU_ARB_CLIENT_NUM)
    {
        base = _get_prearb_reg_base(pre_arb, MIU_CH_RCMD);
        if (!base)
        {
            return (str - buf);
        }

        _load_pre_arb_settings(&greg, base);

        max_serv_idx = halARBPreGetClientBurstSel(&greg, mbr);

        // str += scnprintf(str, end - str, "[rcmd] Client %d max member service index: %d\n", c, max_serv_idx);
        str += scnprintf(str, end - str, "[rcmd] Group ch=%d max member service index: %d for pre-arbiter %d\n", mbr,
                         max_serv_idx, pre_arb);
    }

    return (str - buf);
}
DEVICE_ATTR(prearb_rcmd_client_burst_sel, 0644, prearb_rcmd_client_burst_sel_show, prearb_rcmd_client_burst_sel_store);

#ifdef CONFIG_MIU_FLOWCTRL
static ssize_t prearb_rcmd_client_flowctrl_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                 size_t count)
{
    int                    g, mbr, base;
    ssize_t                ret = count;
    u32                    enable, period, en_tmp;
    char                   peri_tmp;
    char *                 pt, *opt;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    mbr = arb_handle[m].client_selected;

    if (mbr >= MIU_PREARB_CLIENT_NUM)
    {
        printk(KERN_ERR "Client ID fail\n");
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

    base = _get_prearb_reg_base(g, MIU_CH_RCMD);
    if (!base)
    {
        return count;
    }
    _load_pre_arb_settings(&greg, base);

    if (enable)
    {
        // to keep the original setting
        en_tmp = _is_Preflowctrl_enable(&greg, mbr, &peri_tmp);
        _enable_Preflowctrl(&greg, mbr, 0, 0, base);
        // restore the original settings if failed
        if (_enable_Preflowctrl(&greg, mbr, enable, period, base) && en_tmp)
        {
            _enable_Preflowctrl(&greg, mbr, en_tmp, peri_tmp, base);
        }
    }
    else
    {
        // disable client flow control
        _enable_Preflowctrl(&greg, mbr, 0, 0, base);
    }
    halARBPreClientFlowctrl(&greg, base, g);
    return count;
}

static ssize_t prearb_rcmd_client_flowctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, g, mbr, period;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char                   clientName[40];

    memset(clientName, 0, sizeof(clientName));
    str += scnprintf(str, end - str, "Flow Control (prearb ID 0~2):\n");
    str += scnprintf(str, end - str, "echo [id] > client\n");
    str += scnprintf(str, end - str, "enable:  echo 1 [period] > client_flowctrl\n");
    str += scnprintf(str, end - str, "disable: echo 0 > client_flowctrl\n");

    g = arb_handle[m].client_selected;
    if (g < MIU_PREARB_CLIENT_NUM)
    {
        printk(KERN_ERR "[wcmd] Not support group %d \n", g);
        return 0;
    }

    base = _get_prearb_reg_base(g, MIU_CH_RCMD);
    if (!base)
    {
        printk(KERN_ERR "[wcmd] Not support group %d \n", g);
        return 0;
    }

    str += scnprintf(str, end - str, "\nNum:IP_name\t[period]\n");
    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        _load_pre_arb_settings(&greg, base);
        if (greg.flowctrl0 & IGCFG_FCTL_EN_BIT)
        {
            mbr    = (greg.flowctrl0 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
            period = (greg.flowctrl0 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            str += scnprintf(str, end - str, "Group %d\t[  0x%02X]\n", mbr, period);
        }
        if (greg.flowctrl1 & IGCFG_FCTL_EN_BIT)
        {
            mbr    = (greg.flowctrl1 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
            period = (greg.flowctrl1 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            str += scnprintf(str, end - str, "Group %d\t[  0x%02X]\n", mbr, period);
        }
    }
    return (str - buf);
}
DEVICE_ATTR(prearb_rcmd_client_flowctrl, 0644, prearb_rcmd_client_flowctrl_show, prearb_rcmd_client_flowctrl_store);

static ssize_t prearb_wcmd_client_flowctrl_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                 size_t count)
{
    int                    g, mbr, base;
    ssize_t                ret = count;
    u32                    enable, period, en_tmp;
    char                   peri_tmp;
    char *                 pt, *opt;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    mbr = arb_handle[m].client_selected;

    if (mbr >= MIU_PREARB_CLIENT_NUM)
    {
        printk(KERN_ERR "Client ID fail\n");
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

    base = _get_prearb_reg_base(g, MIU_CH_RCMD);
    if (!base)
    {
        return count;
    }
    _load_pre_arb_settings(&greg, base);

    if (enable)
    {
        // to keep the original setting
        en_tmp = _is_Preflowctrl_enable(&greg, mbr, &peri_tmp);
        _enable_Preflowctrl(&greg, mbr, 0, 0, base);
        // restore the original settings if failed
        if (_enable_Preflowctrl(&greg, mbr, enable, period, base) && en_tmp)
        {
            _enable_Preflowctrl(&greg, mbr, en_tmp, peri_tmp, base);
        }
    }
    else
    {
        // disable client flow control
        _enable_Preflowctrl(&greg, mbr, 0, 0, base);
    }
    halARBPreClientFlowctrl(&greg, base, g);
    return count;
}

static ssize_t prearb_wcmd_client_flowctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, g, mbr, period;
    struct miu_arb_pre_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char                   clientName[40];

    memset(clientName, 0, sizeof(clientName));
    str += scnprintf(str, end - str, "Flow Control (prearb ID 0~2):\n");
    str += scnprintf(str, end - str, "echo [id] > client\n");
    str += scnprintf(str, end - str, "enable:  echo 1 [period] > client_flowctrl\n");
    str += scnprintf(str, end - str, "disable: echo 0 > client_flowctrl\n");

    g = arb_handle[m].client_selected;
    if (g < MIU_PREARB_CLIENT_NUM)
    {
        printk(KERN_ERR "[wcmd] Not support group %d \n", g);
        return 0;
    }

    base = _get_prearb_reg_base(g, MIU_CH_WCMD);
    if (!base)
    {
        printk(KERN_ERR "[wcmd] Not support group %d \n", g);
        return 0;
    }

    str += scnprintf(str, end - str, "\nNum:IP_name\t[period]\n");
    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        _load_pre_arb_settings(&greg, base);
        if (greg.flowctrl0 & IGCFG_FCTL_EN_BIT)
        {
            mbr    = (greg.flowctrl0 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
            period = (greg.flowctrl0 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            str += scnprintf(str, end - str, "Group %d\t[  0x%02X]\n", mbr, period);
        }
        if (greg.flowctrl1 & IGCFG_FCTL_EN_BIT)
        {
            mbr    = (greg.flowctrl1 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
            period = (greg.flowctrl1 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            str += scnprintf(str, end - str, "Group %d\t[  0x%02X]\n", mbr, period);
        }
    }
    return (str - buf);
}
DEVICE_ATTR(prearb_wcmd_client_flowctrl, 0644, prearb_wcmd_client_flowctrl_show, prearb_wcmd_client_flowctrl_store);

static ssize_t group_rcmd_client_flowctrl_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                size_t count)
{
    int                    c, g, mbr, base;
    ssize_t                ret = count;
    u32                    enable, period, en_tmp;
    char                   peri_tmp;
    char *                 pt, *opt;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c = arb_handle[m].client_selected;

    if (c < MIU_ARB_CLIENT_NUM)
    {
        g = halGroupArbGetClientGrpID(c);
    }
    else
    {
        printk(KERN_ERR "Client ID fail\n");
        return 0;
    }

    mbr = c % MIU_GRP_CLIENT_NUM;

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

    base = _get_grparb_reg_base(g, MIU_CH_RCMD);
    if (!base)
    {
        return count;
    }
    _load_grp_arb_settings(&greg, base);

    if (enable)
    {
        // to keep the original setting
        en_tmp = _is_flowctrl_enable(&greg, mbr, &peri_tmp);
        _enable_flowctrl(&greg, mbr, 0, 0, base);
        // restore the original settings if failed
        if (_enable_flowctrl(&greg, mbr, enable, period, base) && en_tmp)
        {
            _enable_flowctrl(&greg, mbr, en_tmp, peri_tmp, base);
        }
    }
    else
    {
        // disable client flow control
        _enable_flowctrl(&greg, mbr, 0, 0, base);
    }

    halARBGroupClientFlowctrl(&greg, base, g);
    return count;
}

static ssize_t group_rcmd_client_flowctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, g, mbr[2], period;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char                   clientName[40];

    memset(clientName, 0, sizeof(clientName));
    str += scnprintf(str, end - str, "Flow Control:\n");
    str += scnprintf(str, end - str, "echo [id] > client\n");
    str += scnprintf(str, end - str, "enable:  echo 1 [period] > client_flowctrl\n");
    str += scnprintf(str, end - str, "disable: echo 0 > client_flowctrl\n");

    c = arb_handle[m].client_selected;
    if (c < MIU_ARB_CLIENT_NUM)
    {
        g = halGroupArbGetClientGrpID(c);
    }

    base = _get_grparb_reg_base(g, MIU_CH_RCMD);
    if (!base)
    {
        printk(KERN_ERR "[wcmd] Not support group %d \n", g);
        return 0;
    }

    str += scnprintf(str, end - str, "\nNum:IP_name\t[period]\n");
    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        _load_grp_arb_settings(&greg, base);
        if (greg.flowctrl0 & IGCFG_FCTL_EN_BIT)
        {
            mbr[0] = (greg.flowctrl0 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
            period = (greg.flowctrl0 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            c      = (g * MIU_GRP_CLIENT_NUM) + mbr[0];

            clientId_KernelProtectToName(c, clientName);
            str += scnprintf(str, end - str, "%3d:%s\t[  0x%02X]\n", c, clientName, period);
        }
        if (greg.flowctrl1 & IGCFG_FCTL_EN_BIT)
        {
            mbr[0] = (greg.flowctrl1 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
            period = (greg.flowctrl1 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            c      = (g * MIU_GRP_CLIENT_NUM) + mbr[0];

            {
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%3d:%s\t[  0x%02X]\n", c, clientName, period);
            }
        }
        if (greg.flowctrl2 & IGCFG_FCTL_EN_BIT)
        {
            mbr[0] = (greg.flowctrl2 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
            period = (greg.flowctrl2 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            c      = (g * MIU_GRP_CLIENT_NUM) + mbr[0];

            {
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%3d:%s\t[  0x%02X]\n", c, clientName, period);
            }
        }
        if (greg.flowctrl3 & IGCFG_FCTL_EN_BIT)
        {
            mbr[0] = (greg.flowctrl3 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
            period = (greg.flowctrl3 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            c      = (g * MIU_GRP_CLIENT_NUM) + mbr[0];

            {
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%3d:%s\t[  0x%02X]\n", c, clientName, period);
            }
        }
    }

    return (str - buf);
}

DEVICE_ATTR(group_rcmd_client_flowctrl, 0644, group_rcmd_client_flowctrl_show, group_rcmd_client_flowctrl_store);

static ssize_t group_wcmd_client_flowctrl_store(struct device *dev, struct device_attribute *attr, const char *buf,
                                                size_t count)
{
    int                    c, g, mbr, base;
    ssize_t                ret = count;
    u32                    enable, period, en_tmp;
    char                   peri_tmp;
    char *                 pt, *opt;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);

    c = arb_handle[m].client_selected;

    if (c < MIU_ARB_CLIENT_NUM)
    {
        g = halGroupArbGetClientGrpID(c);
    }
    else
    {
        printk(KERN_ERR "Client ID fail\n");
        return count;
    }

    mbr = c % MIU_GRP_CLIENT_NUM;
    printk("SetFC Group 0x%x Client 0x%x\r\n", g, mbr);

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
                if (!period || (period > (IGFCTL_PERIOD_MASK)))
                {
                    printk(KERN_ERR "Invalid period %d (1-%d)\n", period, (IGFCTL_PERIOD_MASK));
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

    base = _get_grparb_reg_base(g, MIU_CH_WCMD);
    if (!base)
    {
        return count;
    }
    _load_grp_arb_settings(&greg, base);

    if (enable)
    {
        // to keep the original setting
        en_tmp = _is_flowctrl_enable(&greg, mbr, &peri_tmp);
        _enable_flowctrl(&greg, mbr, 0, 0, base);
        // restore the original settings if failed
        if (_enable_flowctrl(&greg, mbr, enable, period, base) && en_tmp)
        {
            _enable_flowctrl(&greg, mbr, en_tmp, peri_tmp, base);
        }
    }
    else
    {
        // disable client flow control
        _enable_flowctrl(&greg, mbr, 0, 0, base);
    }

    halARBGroupClientFlowctrl(&greg, base, g);
    return count;
}

static ssize_t group_wcmd_client_flowctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *                 str = buf;
    char *                 end = buf + PAGE_SIZE;
    int                    base, c, g, mbr[2], period;
    struct miu_arb_grp_reg greg;
    unsigned char          m = MIU_IDX(dev->kobj.name[7]);
    char                   clientName[40];

    memset(clientName, 0, sizeof(clientName));
    str += scnprintf(str, end - str, "Flow Control:\n");
    str += scnprintf(str, end - str, "echo [id] > client\n");
    str += scnprintf(str, end - str, "enable:  echo 1 [period] > client_flowctrl\n");
    str += scnprintf(str, end - str, "disable: echo 0 > client_flowctrl\n");

    c = arb_handle[m].client_selected;
    if (c < MIU_ARB_CLIENT_NUM)
    {
        g = halGroupArbGetClientGrpID(c);
    }

    base = _get_grparb_reg_base(g, MIU_CH_WCMD);
    if (!base)
    {
        printk(KERN_ERR "[wcmd] Not support group %d \n", g);
        return (str - buf);
    }

    str += scnprintf(str, end - str, "\nNum:IP_name\t[period]\n");
    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        _load_grp_arb_settings(&greg, base);
        if (greg.flowctrl0 & IGCFG_FCTL_EN_BIT)
        {
            mbr[0] = (greg.flowctrl0 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
            period = (greg.flowctrl0 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            c      = (g * MIU_GRP_CLIENT_NUM) + mbr[0];

            clientId_KernelProtectToName(c, clientName);
            str += scnprintf(str, end - str, "%3d:%s\t[  0x%02X]\n", c, clientName, period);
        }
        if (greg.flowctrl1 & IGCFG_FCTL_EN_BIT)
        {
            mbr[0] = (greg.flowctrl1 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
            period = (greg.flowctrl1 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            c      = (g * MIU_GRP_CLIENT_NUM) + mbr[0];

            {
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%3d:%s\t[  0x%02X]\n", c, clientName, period);
            }
        }
        if (greg.flowctrl2 & IGCFG_FCTL_EN_BIT)
        {
            mbr[0] = (greg.flowctrl2 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
            period = (greg.flowctrl2 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            c      = (g * MIU_GRP_CLIENT_NUM) + mbr[0];

            {
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%3d:%s\t[  0x%02X]\n", c, clientName, period);
            }
        }
        if (greg.flowctrl3 & IGCFG_FCTL_EN_BIT)
        {
            mbr[0] = (greg.flowctrl3 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
            period = (greg.flowctrl3 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            c      = (g * MIU_GRP_CLIENT_NUM) + mbr[0];

            {
                clientId_KernelProtectToName(c, clientName);
                str += scnprintf(str, end - str, "%3d:%s\t[  0x%02X]\n", c, clientName, period);
            }
        }
    }

    return (str - buf);
}

DEVICE_ATTR(group_wcmd_client_flowctrl, 0644, group_wcmd_client_flowctrl_show, group_wcmd_client_flowctrl_store);
#endif

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
            // str = _dump_as_text(str, end, m);
            break;
        case MIU_ARB_DUMP_REG:
            // str = _dump_as_reg(str, end, m);
            break;
        default:
            return 0;
    }
    return (str - buf);
}

DEVICE_ATTR(dump, 0644, dump_show, dump_store);

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

        device_create_file(&arb_dev[i].dev, &dev_attr_client);
        device_create_file(&arb_dev[i].dev, &dev_attr_group);
        // device_create_file(&arb_dev[i].dev, &dev_attr_group_burst);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_wcmd_order_buffer);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_rcmd_order_buffer);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_wcmd_member_burst);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_rcmd_member_burst);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_wcmd_client_nolimit);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_rcmd_client_nolimit);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_wcmd_client_burst_sel);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_rcmd_client_burst_sel);

        device_create_file(&arb_dev[i].dev, &dev_attr_prearb_wcmd_order_buffer);
        device_create_file(&arb_dev[i].dev, &dev_attr_prearb_rcmd_order_buffer);
        device_create_file(&arb_dev[i].dev, &dev_attr_prearb_wcmd_member_burst);
        device_create_file(&arb_dev[i].dev, &dev_attr_prearb_rcmd_member_burst);
        device_create_file(&arb_dev[i].dev, &dev_attr_prearb_wcmd_client_nolimit);
        device_create_file(&arb_dev[i].dev, &dev_attr_prearb_rcmd_client_nolimit);
        device_create_file(&arb_dev[i].dev, &dev_attr_prearb_wcmd_client_burst_sel);
        device_create_file(&arb_dev[i].dev, &dev_attr_prearb_rcmd_client_burst_sel);

#ifdef CONFIG_MIU_FLOWCTRL
        device_create_file(&arb_dev[i].dev, &dev_attr_prearb_wcmd_client_flowctrl);
        device_create_file(&arb_dev[i].dev, &dev_attr_prearb_rcmd_client_flowctrl);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_wcmd_client_flowctrl);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_rcmd_client_flowctrl);
#endif
        // device_create_file(&arb_dev[i].dev, &dev_attr_dump);
        device_create_file(&arb_dev[i].dev, &dev_attr_client_wcmd);
        device_create_file(&arb_dev[i].dev, &dev_attr_client_rcmd);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_wcmd);
        device_create_file(&arb_dev[i].dev, &dev_attr_group_rcmd);
    }

    halARBResume();
}
