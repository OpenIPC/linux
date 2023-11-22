/*
 * mhal_arb.c- Sigmastar
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
#include "mdrv_types.h"
#include "registers.h"
#include "mhal_arb.h"
#include "mhal_miu_client.h"

// default policy
static struct miu_arb_reg arb_policy_def = {
    {
        {// group 0
         .cfg          = 0x801D,
         .burst        = 0x1008,
         .flowctrl0    = 0x0000,
         .flowctrl1    = 0x0000,
         .mbr_priority = 0x1000,
         .mbr_nolimit  = 0x0000},
        {// group 1
         .cfg          = 0x801D,
         .burst        = 0x2010,
         .flowctrl0    = 0x0000,
         .flowctrl1    = 0x0000,
         .mbr_priority = 0x0000,
         .mbr_nolimit  = 0x0000},
        {// group 2
         .cfg          = 0x801D,
         .burst        = 0x4020,
         .flowctrl0    = 0x0000,
         .flowctrl1    = 0x0000,
         .mbr_priority = 0xfffc,
         .mbr_nolimit  = 0x0000},
        {// group 3
         .cfg          = 0x801D,
         .burst        = 0x2010,
         .flowctrl0    = 0x0000,
         .flowctrl1    = 0x0000,
         .mbr_priority = 0x0000,
         .mbr_nolimit  = 0x0000},
    },
    // extern
    {
        {
            // group 0
            .burst_23 = 0x2010,
            .burst_4  = 0x0040,
            .mbr_sel0 = 0x0000,
            .mbr_sel1 = 0x0000,
        },
        {// group 1
         .burst_23 = 0x2010,
         .burst_4  = 0x0040,
         .mbr_sel0 = 0x0000,
         .mbr_sel1 = 0x0000},
        {// group 2
         .burst_23 = 0x2010,
         .burst_4  = 0x0040,
         .mbr_sel0 = 0x0000,
         .mbr_sel1 = 0x0000},
        {// group 3
         .burst_23 = 0x2010,
         .burst_4  = 0x0040,
         .mbr_sel0 = 0x0000,
         .mbr_sel1 = 0x0000},
    },
    .cfg = 0x9000,
};

static struct miu_arb_reg *arb_policies[MIU_ARB_POLICY_NUM] = {
    &arb_policy_def,
};

int halARBGetRegBase(int miu)
{
    if (miu == 0)
    {
        return BASE_REG_MIU_PA;
    }
    printk(KERN_ERR "MIU%d reg base not assigend\n", miu);
    return 0;
}
int halARBGetRegBase_e(void)
{
    return BASE_REG_MIU_E_PA;
}

void halARBLoadGroupSetting(struct miu_arb_grp_reg *greg, int base, int g)
{
    if (greg)
    {
        greg->cfg          = INREG16(base + REG_IGRP_CFG(g));
        greg->burst        = INREG16(base + REG_IGRP_BURST(g));
        greg->flowctrl0    = INREG16(base + REG_IGRP_FCTL0(g));
        greg->flowctrl1    = INREG16(base + REG_IGRP_FCTL1(g));
        greg->mbr_priority = INREG16(base + REG_IGRP_MBR_PRIO(g));
        greg->mbr_nolimit  = INREG16(base + REG_IGRP_MBR_NOLIMIT(g));
    }
}

void halARBLoadGroupSettings_e(struct miu_arb_grp_reg_e *greg, int base, int g)
{
    if (greg)
    {
        greg->burst_23 = INREG16(base + REG_IGRP_BURST_EX23(g));
        greg->burst_4  = INREG16(base + REG_IGRP_BURST_EX4(g));
        greg->mbr_sel0 = INREG16(base + REG_IGRP_MBR_SEL0(g));
        greg->mbr_sel1 = INREG16(base + REG_IGRP_MBR_SEL1(g));
    }
}
int halARBGetClientMaxMbrServIdx(struct miu_arb_grp_reg_e *greg, int mbr, int g)
{
    int sel = 0;
    if (greg)
    {
        if (mbr <= 7)
        {
            sel = ((greg->mbr_sel0 & IGMBR_SEL0_MASK(mbr)) >> IGMBR_SEL0_SHIFT(mbr));
        }
        else if (mbr >= 8 && mbr <= 15)
        {
            sel = ((greg->mbr_sel1 & IGMBR_SEL1_MASK(mbr)) >> IGMBR_SEL1_SHIFT(mbr));
        }
        else
        {
            sel = 0;
            printk(KERN_ERR "Invalid mbr %d\n", mbr);
        }
    }
    return sel;
}

int halARBGetClientBurst(struct miu_arb_grp_reg *greg, struct miu_arb_grp_reg_e *greg_e, int mbr, int g)
{
    int max_serv_idx, burst;
    max_serv_idx = halARBGetClientMaxMbrServIdx(greg_e, mbr, g);
    if (max_serv_idx == 0)
    {
        burst = (greg->burst & IGBURST_MBR_MASK) >> IGBURST_MBR_SHIFT;
    }
    else if (max_serv_idx == 1)
    {
        burst = (greg_e->burst_23 & IGBURST_MBR2_MASK) >> IGBURST_MBR2_SHIFT;
    }
    else if (max_serv_idx == 2)
    {
        burst = (greg_e->burst_23 & IGBURST_MBR3_MASK) >> IGBURST_MBR3_SHIFT;
    }
    else
    {
        burst = (greg_e->burst_4 & IGBURST_MBR4_MASK) >> IGBURST_MBR4_SHIFT;
    }
    return burst;
}
BOOL halARBIsFlowCTLEnable(struct miu_arb_grp_reg *greg, int mbr, char *period)
{
    *period = 0;

    if (greg->cfg & IGCFG_FCTL0_EN_BIT)
    {
        if (mbr == ((greg->flowctrl0 & IGFCTL_ID0_MASK) >> IGFCTL_ID0_SHIFT))
        {
            *period = (greg->flowctrl0 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            return 1;
        }
        if (mbr == ((greg->flowctrl0 & IGFCTL_ID1_MASK) >> IGFCTL_ID1_SHIFT))
        {
            *period = (greg->flowctrl0 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            return 1;
        }
    }
    if (greg->cfg & IGCFG_FCTL1_EN_BIT)
    {
        if (mbr == ((greg->flowctrl1 & IGFCTL_ID0_MASK) >> IGFCTL_ID0_SHIFT))
        {
            *period = (greg->flowctrl1 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            return 1;
        }
        if (mbr == ((greg->flowctrl1 & IGFCTL_ID1_MASK) >> IGFCTL_ID1_SHIFT))
        {
            *period = (greg->flowctrl1 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
            return 1;
        }
    }
    return 0;
}

BOOL halARBEnableFlowCTL(struct miu_arb_grp_reg *greg, int mbr, bool enable, char period)
{
    int id[2][2], peri[2];

    id[0][0] = (greg->flowctrl0 & IGFCTL_ID0_MASK) >> IGFCTL_ID0_SHIFT;
    id[0][1] = (greg->flowctrl0 & IGFCTL_ID1_MASK) >> IGFCTL_ID1_SHIFT;
    peri[0]  = (greg->flowctrl0 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
    id[1][0] = (greg->flowctrl1 & IGFCTL_ID0_MASK) >> IGFCTL_ID0_SHIFT;
    id[1][1] = (greg->flowctrl1 & IGFCTL_ID1_MASK) >> IGFCTL_ID1_SHIFT;
    peri[1]  = (greg->flowctrl1 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;

    if (enable)
    {
        do
        {
            // enabled group flow control cnt0 has the same period
            if ((greg->cfg & IGCFG_FCTL0_EN_BIT) && (peri[0] == period) && (id[0][0] == id[0][1]))
            {
                greg->flowctrl0 &= ~(IGFCTL_ID1_MASK);
                greg->flowctrl0 |= (mbr << IGFCTL_ID1_SHIFT);
                break;
            }
            else if ((greg->cfg & IGCFG_FCTL1_EN_BIT) && (peri[1] == period) && (id[1][0] == id[1][1]))
            {
                greg->flowctrl1 &= ~(IGFCTL_ID1_MASK);
                greg->flowctrl1 |= (mbr << IGFCTL_ID1_SHIFT);
                break;
            }
            else if (!(greg->cfg & IGCFG_FCTL0_EN_BIT))
            {
                greg->flowctrl0 =
                    (mbr << IGFCTL_ID0_SHIFT) | (mbr << IGFCTL_ID1_SHIFT) | (period << IGFCTL_PERIOD_SHIFT);
                greg->cfg |= IGCFG_FCTL0_EN_BIT;
                break;
            }
            else if (!(greg->cfg & IGCFG_FCTL1_EN_BIT))
            {
                greg->flowctrl1 =
                    (mbr << IGFCTL_ID0_SHIFT) | (mbr << IGFCTL_ID1_SHIFT) | (period << IGFCTL_PERIOD_SHIFT);
                greg->cfg |= IGCFG_FCTL1_EN_BIT;
                break;
            }
            printk(KERN_ERR "No free flow control id for period %d\n", period);
            return 1; // not free one, failed
        } while (1);
    }
    else
    {
        // disable client flow control
        if (greg->cfg & IGCFG_FCTL0_EN_BIT)
        {
            if ((mbr == id[0][0]) && (id[0][0] == id[0][1]))
            {
                greg->flowctrl0 &= (u16)(~(IGFCTL_ID0_MASK | IGFCTL_ID1_MASK | IGFCTL_PERIOD_MASK));
                greg->cfg &= ~(IGCFG_FCTL0_EN_BIT);
            }
            else if (mbr == id[0][0])
            {
                greg->flowctrl0 &= ~(IGFCTL_ID0_MASK);
                greg->flowctrl0 |= (id[0][1] << IGFCTL_ID0_SHIFT); // set id0 as id1
            }
            else if (mbr == id[0][1])
            {
                greg->flowctrl0 &= ~(IGFCTL_ID1_MASK);
                greg->flowctrl0 |= (id[0][0] << IGFCTL_ID1_SHIFT); // set id1 as id0
            }
        }
        if (greg->cfg & IGCFG_FCTL1_EN_BIT)
        {
            if ((mbr == id[1][0]) && (id[1][0] == id[1][1]))
            {
                greg->flowctrl1 &= (u16)(~(IGFCTL_ID0_MASK | IGFCTL_ID1_MASK | IGFCTL_PERIOD_MASK));
                greg->cfg &= ~(IGCFG_FCTL1_EN_BIT);
            }
            else if (mbr == id[1][0])
            {
                greg->flowctrl1 &= ~(IGFCTL_ID0_MASK);
                greg->flowctrl1 |= (id[1][1] << IGFCTL_ID0_SHIFT); // set id0 as id1
            }
            else if (mbr == id[1][1])
            {
                greg->flowctrl1 &= ~(IGFCTL_ID1_MASK);
                greg->flowctrl1 |= (id[1][0] << IGFCTL_ID1_SHIFT); // set id1 as id0
            }
        }
    }
    return 0;
}

void halARBLoadPolicy(int miu, int idx)
{
    int                 base;
    int                 g;
    struct miu_arb_reg *reg;

    if (idx >= MIU_ARB_POLICY_NUM)
    {
        return;
    }

    base = halARBGetRegBase(miu);
    if (!base)
    {
        return;
    }

    reg = arb_policies[idx];
    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        OUTREG16(base + REG_IGRP_CFG(g), reg->grp[g].cfg);
        OUTREG16(base + REG_IGRP_BURST(g), reg->grp[g].burst);
        OUTREG16(base + REG_IGRP_FCTL0(g), reg->grp[g].flowctrl0);
        OUTREG16(base + REG_IGRP_FCTL1(g), reg->grp[g].flowctrl1);
        OUTREG16(base + REG_IGRP_MBR_PRIO(g), reg->grp[g].mbr_priority);
        OUTREG16(base + REG_IGRP_MBR_NOLIMIT(g), reg->grp[g].mbr_nolimit);
    }
    // ENABLE tv-tool read/write MIU data, bank 0x1012, offset 0x7F BIT11
    OUTREG16((base + REG_OGRP_CFG), reg->cfg | BIT11);
}

int halARBGetOGCCFG(int base)
{
    return INREG16(base + REG_OGRP_CFG);
}

void halARBGroupBurstStore(struct miu_arb_grp_reg *greg, int base, int g, int burst)
{
    if (burst == 0)
    {
        greg->cfg &= ~(IGCFG_GRP_LIMIT_EN_BIT);
        greg->burst &= ~(IGBURST_GRP_MASK);
    }
    else
    {
        greg->cfg |= IGCFG_GRP_LIMIT_EN_BIT;
        greg->burst = (greg->burst & ~(IGBURST_GRP_MASK)) | (burst << IGBURST_GRP_SHIFT);
    }
    OUTREG16(base + REG_IGRP_CFG(g), greg->cfg);
    OUTREG16(base + REG_IGRP_BURST(g), greg->burst);
}

void halARBGroupMemberBurstStore(struct miu_arb_grp_reg *greg, int base, int g, int idx, int burst)
{
    if (burst == 0)
    {
        greg->cfg &= ~(IGCFG_MBR_LIMIT_EN_BIT);
        greg->burst &= ~(IGBURST_MBR_MASK);
    }
    else
    {
        greg->cfg |= IGCFG_MBR_LIMIT_EN_BIT;
        greg->burst = (greg->burst & ~(IGBURST_MBR_MASK)) | (burst << IGBURST_MBR_SHIFT);
    }
    OUTREG16(base + REG_IGRP_CFG(g), greg->cfg);
    OUTREG16(base + REG_IGRP_BURST(g), greg->burst);
}

void halARBClientNolimitStore(struct miu_arb_grp_reg *greg, int base, int c, int nolimit)
{
    int g, mbr;
    g   = c / MIU_GRP_CLIENT_NUM;
    mbr = c % MIU_GRP_CLIENT_NUM;

    if (nolimit == 0)
    {
        greg->mbr_nolimit &= ~(IGMBR_NOLIMIT_EN(mbr));
        if (!(greg->cfg & IGCFG_MBR_LIMIT_EN_BIT))
        {
            printk(KERN_ERR "Conflict: group %d member limit is disabled\n", g);
        }
    }
    else
    {
        greg->mbr_nolimit |= IGMBR_NOLIMIT_EN(mbr);
        if (!(greg->cfg & IGCFG_MBR_LIMIT_EN_BIT))
        {
            printk(KERN_ERR "NoNeed: group %d member limit is disabled already\n", g);
        }
    }
    OUTREG16(base + REG_IGRP_MBR_NOLIMIT(g), greg->mbr_nolimit);
}

void halARBClientPrioprityStore(struct miu_arb_grp_reg *greg, int base, int g)
{
    OUTREG16(base + REG_IGRP_MBR_PRIO(g), greg->mbr_priority);
}

void halARBClientFlowctrl(struct miu_arb_grp_reg *greg, int base, int g)
{
    OUTREG16(base + REG_IGRP_CFG(g), greg->cfg);
    OUTREG16(base + REG_IGRP_FCTL0(g), greg->flowctrl0);
    OUTREG16(base + REG_IGRP_FCTL1(g), greg->flowctrl1);
}

short halARBClientReserved(int id)
{
    if (id < MIU0_CLIENT_NUM)
    {
        return miu0_clients[id].bw_rsvd;
    }
    else if (id == CPU_W_CLIENT_ID)
    {
        return miu0_clients[MIU0_CLIENT_NUM - 4].bw_rsvd;
    }
    else if (id == DLA_HIWAY_R_ID)
    {
        return miu0_clients[MIU0_CLIENT_NUM - 3].bw_rsvd;
    }
    else if (id == DLA_HIWAY_W_ID)
    {
        return miu0_clients[MIU0_CLIENT_NUM - 2].bw_rsvd;
    }
    else if (id == CPU_R_CLIENT_ID)
    {
        return miu0_clients[MIU0_CLIENT_NUM - 1].bw_rsvd;
    }
    return 1;
}

void halARBResume(void)
{
    halARBLoadPolicy(0, MIU_ARB_POLICY_DEF);
}
