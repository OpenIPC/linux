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
#if 0
    {
        {   // group 0
            .cfg = 0x801D,
            .burst = 0x1008,
            .flowctrl0 = 0x0000,
            .flowctrl1 = 0x0000,
            .mbr_priority = 0x1000,
            .mbr_nolimit = 0x0000
        },
        {   // group 1
            .cfg = 0x801D,
            .burst = 0x2010,
            .flowctrl0 = 0x0000,
            .flowctrl1 = 0x0000,
            .mbr_priority = 0x0000,
            .mbr_nolimit = 0x0000
        },
        {   // group 2
            .cfg = 0x801D,
            .burst = 0x4020,
            .flowctrl0 = 0x0000,
            .flowctrl1 = 0x0000,
            .mbr_priority = 0xfffc,
            .mbr_nolimit = 0x0000
        },
        {   // group 3
            .cfg = 0x801D,
            .burst = 0x2010,
            .flowctrl0 = 0x0000,
            .flowctrl1 = 0x0000,
            .mbr_priority = 0x0000,
            .mbr_nolimit = 0x0000
        },
    },
    //extern
    {
        {   // group 0
            .burst_23 = 0x2010,
            .burst_4 = 0x0040,
            .mbr_sel0 = 0x0000,
            .mbr_sel1 = 0x0000,
        },
        {   // group 1
            .burst_23 = 0x2010,
            .burst_4 = 0x0040,
            .mbr_sel0 = 0x0000,
            .mbr_sel1 = 0x0000
        },
        {   // group 2
           .burst_23 = 0x2010,
           .burst_4 = 0x0040,
           .mbr_sel0 = 0x0000,
           .mbr_sel1 = 0x0000
        },
        {   // group 3
            .burst_23 = 0x2010,
            .burst_4 = 0x0040,
            .mbr_sel0 = 0x0000,
            .mbr_sel1 = 0x0000
        },
    },
    .cfg = 0x9000,
#endif
};

static struct miu_arb_reg *arb_policies[MIU_ARB_POLICY_NUM] = {
    &arb_policy_def,
};

int halArbGetSpecOrderClientID(unsigned short idx)
{
    return 0;
}

int halGroupArbGetClientGrpID(unsigned short clientid)
{
    return (clientid / MIU_GRP_CLIENT_NUM);
}

int halArbPreGetID(unsigned short clientid)
{
    return 0;
}

void halGrpArbRegsToggle(struct miu_arb_grp_reg *greg, int base)
{
    // Register toggle
    greg->cfg &= ~(IGCFG_ARB_TOGGLE);
    OUTREG16(base + REG_IGRP_CFG(), greg->cfg);
    greg->cfg |= IGCFG_ARB_TOGGLE;
    OUTREG16(base + REG_IGRP_CFG(), greg->cfg);
}

void halPreArbRegsToggle(struct miu_arb_pre_reg *greg, int base)
{
    // Register toggle
    greg->cfg &= ~(PREACFG_ARB_TOGGLE);
    OUTREG16(base + REG_PRE_ARB_CFG(), greg->cfg);
    greg->cfg |= PREACFG_ARB_TOGGLE;
    OUTREG16(base + REG_PRE_ARB_CFG(), greg->cfg);
}

int halGrpArbGetRegBase(unsigned short grp_idx, unsigned short wcmd)
{
    int offset = 0x00;

    if (wcmd)
        offset = REG_ID_30;

    switch (grp_idx)
    {
        case 0:
            return BASE_REG_MIU_GRP_SC0 + offset;
            break;
        case 1:
            return BASE_REG_MIU_GRP_SC1 + offset;
            break;
        case 2:
            return BASE_REG_MIU_GRP_ISP0 + offset;
            break;
        default:
            printk(KERN_ERR "Group arbiter %d is not support\r\n", grp_idx);
            break;
    }

    return 0;
}
int halPreArbGetRegBase(unsigned short prearb_idx, unsigned short wcmd)
{
    int offset = 0x00;

    if (wcmd)
        offset = REG_ID_30;

    return BASE_REG_MIU_PRE_ARB + offset;
}

void halARBGroupLoadSetting(struct miu_arb_grp_reg *greg, int base)
{
    if (greg)
    {
        greg->cfg             = INREG16(base + REG_IGRP_CFG());
        greg->flowctrl0       = INREG16(base + REG_IGRP_FCTL0());
        greg->flowctrl1       = INREG16(base + REG_IGRP_FCTL1());
        greg->flowctrl2       = INREG16(base + REG_IGRP_FCTL2());
        greg->flowctrl3       = INREG16(base + REG_IGRP_FCTL3());
        greg->flowctrl_no_req = INREG16(base + REG_NOT_REQ_OFF());

        greg->burst       = INREG16(base + REG_IGRP_BURST());
        greg->burst_1     = INREG16(base + REG_IGRP_BURST_1());
        greg->mbr_sel0    = INREG16(base + REG_IGRP_MBR_SEL0());
        greg->mbr_sel1    = INREG16(base + REG_IGRP_MBR_SEL1());
        greg->mbr_nolimit = INREG16(base + REG_IGRP_MBR_NOLIMIT());
    }
}

void halARBPreLoadSetting(struct miu_arb_pre_reg *greg, int base)
{
    if (greg)
    {
        greg->flowctrl0       = INREG16(base + REG_PREA_FCTL0());
        greg->flowctrl1       = INREG16(base + REG_PREA_FCTL1());
        greg->flowctrl_no_req = INREG16(base + REG_PREA_NOT_REG_OFF());
        greg->burst           = INREG16(base + REG_PREARB_BURST());
        greg->burst_1         = INREG16(base + REG_PREARB_BURST_1());
        greg->mbr_sel0        = INREG16(base + REG_PREARB_MBR_SEL0());
        greg->mbr_sel1        = INREG16(base + REG_PREARB_MBR_SEL1());
        greg->mbr_nolimit     = INREG16(base + REG_PREARB_MBR_NOLIMIT());
    }
}

// group arbiter
int halARBGroupGetOrderBuffer(int base, int win_index)
{
    U16 tmp;

    if (win_index > MIU_GARB_WIN_NUM)
        return MIU_GARB_WIN_NUM;

    tmp = INREG16(base + ((win_index / 4) << 2));
    tmp = (tmp >> ((win_index % 4) * 4)) & 0x000F;

    return tmp;
}

// group arbiter
int halARBPreGetOrderBuffer(int base, int win_index)
{
    U16 tmp;

    if (win_index > MIU_GARB_WIN_NUM)
        return MIU_GARB_WIN_NUM;

    tmp = INREG16(base + ((win_index / 4) << 2));
    tmp = (tmp >> ((win_index % 4) * 4)) & 0x000F;

    return tmp;
}

// group arbiter
int halARBGroupGetClientBurstSel(struct miu_arb_grp_reg *greg, int mbr)
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

// pre arbiter
int halARBGroupGetClientBurst(struct miu_arb_grp_reg *greg, int mbr)
{
    int max_serv_idx, burst;
    max_serv_idx = halARBGroupGetClientBurstSel(greg, mbr);
    if (max_serv_idx == 0)
    {
        burst = (greg->burst & IGBURST_LSB_MASK) >> IGBURST_LSB_SHIFT;
    }
    else if (max_serv_idx == 1)
    {
        burst = (greg->burst & IGBURST_MSB_MASK) >> IGBURST_MSB_SHIFT;
    }
    else if (max_serv_idx == 2)
    {
        burst = (greg->burst_1 & IGBURST_LSB_MASK) >> IGBURST_LSB_SHIFT;
    }
    else
    {
        burst = (greg->burst_1 & IGBURST_MSB_MASK) >> IGBURST_MSB_SHIFT;
    }
    return burst;
}

// pre arbiter
int halARBPreGetClientBurstSel(struct miu_arb_pre_reg *greg, int mbr)
{
    int sel = 0;

    if (greg)
    {
        if (mbr <= 7)
        {
            sel = ((greg->mbr_sel0 & PREAMBR_SEL0_MASK(mbr)) >> PREAMBR_SEL0_SHIFT(mbr));
        }
        else if (mbr >= 8 && mbr <= 15)
        {
            sel = ((greg->mbr_sel1 & PREAMBR_SEL1_MASK(mbr)) >> PREAMBR_SEL1_SHIFT(mbr));
        }
        else
        {
            sel = 0;
            printk(KERN_ERR "Invalid mbr %d\n", mbr);
        }
    }
    return sel;
}
// pre arbiter
int halARBPreGetClientBurst(struct miu_arb_pre_reg *greg, int mbr)
{
    int max_serv_idx, burst;

    max_serv_idx = halARBPreGetClientBurstSel(greg, mbr);
    if (max_serv_idx == 0)
    {
        burst = (greg->burst & PREABURST_LSB_MASK) >> PREABURST_LSB_SHIFT;
    }
    else if (max_serv_idx == 1)
    {
        burst = (greg->burst & PREABURST_MSB_MASK) >> PREABURST_MSB_SHIFT;
    }
    else if (max_serv_idx == 2)
    {
        burst = (greg->burst_1 & PREABURST_LSB_MASK) >> PREABURST_LSB_SHIFT;
    }
    else
    {
        burst = (greg->burst_1 & PREABURST_MSB_MASK) >> PREABURST_MSB_SHIFT;
    }
    return burst;
}

void halARBLoadPolicy(int miu, int idx)
{
    int                 base;
    int                 g;
    struct miu_arb_reg *reg;

    // need to fix
    return;

    if (idx >= MIU_ARB_POLICY_NUM)
    {
        return;
    }

    reg = arb_policies[idx];
    for (g = 0; g < MIU_GRP_NUM; g++)
    {
        base = halGrpArbGetRegBase(g, MIU_CH_RCMD);
        if (!base)
        {
            continue;
        }
        // group rcmd
        OUTREG16(base + REG_IGRP_CFG(), reg->grp_rcmd[g].cfg);
        OUTREG16(base + REG_IGRP_BURST(), reg->grp_rcmd[g].burst);
        OUTREG16(base + REG_IGRP_BURST_1(), reg->grp_rcmd[g].burst_1);
        OUTREG16(base + REG_IGRP_MBR_SEL0(), reg->grp_rcmd[g].mbr_sel0);
        OUTREG16(base + REG_IGRP_MBR_SEL1(), reg->grp_rcmd[g].mbr_sel1);
        OUTREG16(base + REG_IGRP_MBR_NOLIMIT(), reg->grp_rcmd[g].mbr_nolimit);
        // group wcmd
        base = halGrpArbGetRegBase(g, MIU_CH_WCMD);
        OUTREG16(base + REG_IGRP_CFG(), reg->grp_wcmd[g].cfg);
        OUTREG16(base + REG_IGRP_BURST(), reg->grp_wcmd[g].burst);
        OUTREG16(base + REG_IGRP_BURST_1(), reg->grp_wcmd[g].burst_1);
        OUTREG16(base + REG_IGRP_MBR_SEL0(), reg->grp_wcmd[g].mbr_sel0);
        OUTREG16(base + REG_IGRP_MBR_SEL1(), reg->grp_wcmd[g].mbr_sel1);
        OUTREG16(base + REG_IGRP_MBR_NOLIMIT(), reg->grp_wcmd[g].mbr_nolimit);
    }
}

// group arbiter
void halARBGroupOrderBufferStore(struct miu_arb_grp_reg *greg, int base, int win_index, int mbr)
{
    U16 tmp;

    if (win_index > MIU_GARB_WIN_NUM || mbr > MIU_GRP_CLIENT_NUM)
        return;

    tmp = INREG16(base + ((win_index / 4) << 2));
    tmp = tmp & ~(0x0F << ((win_index % 4) * 4));
    tmp = tmp | (mbr << ((win_index % 4) * 4));
    OUTREG16(base + ((win_index / 4) << 2), tmp);

    halGrpArbRegsToggle(greg, base);
}

// pre arbiter
void halARBPreOrderBufferStore(struct miu_arb_pre_reg *greg, int base, int win_index, int mbr)
{
    U16 tmp;

    if (win_index > MIU_GARB_WIN_NUM || mbr > MIU_GRP_CLIENT_NUM)
        return;

    tmp = INREG16(base + ((win_index / 4) << 2));
    tmp = tmp & ~(0x0F << ((win_index % 4) * 4));
    tmp = tmp | (mbr << ((win_index % 4) * 4));
    OUTREG16(base + ((win_index / 4) << 2), tmp);

    halPreArbRegsToggle(greg, base);
}

// group arbiter
void halARBGroupClientBurstSelStore(struct miu_arb_grp_reg *greg, int base, int sel, int mbr)
{
    if (greg)
    {
        if (sel < 4)
        {
            if (mbr <= 7)
            {
                greg->mbr_sel0 = (greg->mbr_sel0 & ~(IGMBR_SEL0_MASK(mbr))) | (sel << IGMBR_SEL0_SHIFT(mbr));
                OUTREG16(base + REG_IGRP_MBR_SEL0(), greg->mbr_sel0);
            }
            else
            {
                greg->mbr_sel1 = (greg->mbr_sel1 & ~(IGMBR_SEL1_MASK(mbr))) | (sel << IGMBR_SEL1_SHIFT(mbr));
                OUTREG16(base + REG_IGRP_MBR_SEL1(), greg->mbr_sel1);
            }
            halGrpArbRegsToggle(greg, base);
        }
    }
}

// pre arbiter
void halARBPreClientBurstSelStore(struct miu_arb_pre_reg *greg, int base, int sel, int mbr)
{
    if (greg)
    {
        if (sel < 4)
        {
            if (mbr <= 7)
            {
                greg->mbr_sel0 = (greg->mbr_sel0 & ~(PREAMBR_SEL0_MASK(mbr))) | (sel << PREAMBR_SEL0_SHIFT(mbr));
                OUTREG16(base + REG_PREARB_MBR_SEL0(), greg->mbr_sel0);
            }
            else
            {
                greg->mbr_sel1 = (greg->mbr_sel1 & ~(PREAMBR_SEL1_MASK(mbr))) | (sel << PREAMBR_SEL1_SHIFT(mbr));
                OUTREG16(base + REG_PREARB_MBR_SEL1(), greg->mbr_sel1);
            }
            halPreArbRegsToggle(greg, base);
        }
    }
}

// group arbiter burst
void halARBGroupBurstStore(struct miu_arb_grp_reg *greg, int base, int idx, int burst)
{
    if (burst == 0)
    {
        greg->cfg &= ~(IGCFG_GRP_LIMIT_EN_BIT);
    }
    else
    {
        greg->cfg |= IGCFG_GRP_LIMIT_EN_BIT;
        if (idx == 0)
        {
            greg->burst = (greg->burst & ~(IGBURST_LSB_MASK)) | (burst << IGBURST_LSB_SHIFT);
            OUTREG16(base + REG_IGRP_BURST(), greg->burst);
        }
        else if (idx == 1)
        {
            greg->burst = (greg->burst & ~(IGBURST_MSB_MASK)) | (burst << IGBURST_MSB_SHIFT);
            OUTREG16(base + REG_IGRP_BURST(), greg->burst);
        }
        else if (idx == 2)
        {
            greg->burst_1 = (greg->burst_1 & ~(IGBURST_LSB_MASK)) | (burst << IGBURST_LSB_SHIFT);
            OUTREG16(base + REG_IGRP_BURST_1(), greg->burst_1);
        }
        else
        {
            greg->burst_1 = (greg->burst_1 & ~(IGBURST_MSB_MASK)) | (burst << IGBURST_MSB_SHIFT);
            OUTREG16(base + REG_IGRP_BURST_1(), greg->burst_1);
        }
    }

    halGrpArbRegsToggle(greg, base);
}

// pre-arbiter burst
void halARBPreBurstStore(struct miu_arb_pre_reg *greg, int base, int idx, int burst)
{
    if (burst == 0)
    {
        greg->cfg &= ~(PREACFG_GRP_LIMIT_EN_BIT);
    }
    else
    {
        greg->cfg |= PREACFG_GRP_LIMIT_EN_BIT;
        if (idx == 0)
        {
            greg->burst = (greg->burst & ~(PREABURST_LSB_MASK)) | (burst << PREABURST_LSB_SHIFT);
            OUTREG16(base + REG_IGRP_BURST(), greg->burst);
        }
        else if (idx == 1)
        {
            greg->burst = (greg->burst & ~(PREABURST_MSB_MASK)) | (burst << PREABURST_MSB_SHIFT);
            OUTREG16(base + REG_IGRP_BURST(), greg->burst);
        }
        else if (idx == 2)
        {
            greg->burst_1 = (greg->burst_1 & ~(PREABURST_LSB_MASK)) | (burst << PREABURST_LSB_SHIFT);
            OUTREG16(base + REG_IGRP_BURST_1(), greg->burst_1);
        }
        else
        {
            greg->burst_1 = (greg->burst_1 & ~(PREABURST_MSB_MASK)) | (burst << PREABURST_MSB_SHIFT);
            OUTREG16(base + REG_IGRP_BURST_1(), greg->burst_1);
        }
    }
    // Register toggle
    halPreArbRegsToggle(greg, base);
}

void halARBGroupClientNolimitStore(struct miu_arb_grp_reg *greg, int base, int c, int nolimit)
{
    int g, mbr;

    g   = c / MIU_GRP_CLIENT_NUM;
    mbr = c % MIU_GRP_CLIENT_NUM;

    if (nolimit == 0)
    {
        greg->mbr_nolimit &= ~(IGMBR_NOLIMIT_EN(mbr));
        if (!(greg->cfg & IGCFG_GRP_LIMIT_EN_BIT))
        {
            printk(KERN_ERR "Conflict: group %d member limit is disabled\n", g);
        }
    }
    else
    {
        greg->mbr_nolimit |= IGMBR_NOLIMIT_EN(mbr);
        if (!(greg->cfg & IGCFG_GRP_LIMIT_EN_BIT))
        {
            printk(KERN_ERR "NoNeed: group %d member limit is disabled already\n", g);
        }
    }
    OUTREG16(base + REG_IGRP_MBR_NOLIMIT(), greg->mbr_nolimit);

    halGrpArbRegsToggle(greg, base);
}

void halARBPreClientNolimitStore(struct miu_arb_pre_reg *greg, int base, int c, int nolimit)
{
    int mbr, pre_arb;

    pre_arb = c / MIU_PREARB_CLIENT_MAX;
    mbr     = c / MIU_GRP_CLIENT_NUM;

    if (nolimit == 0)
    {
        greg->mbr_nolimit &= ~(PREAMBR_NOLIMIT_EN(mbr));
        if (!(greg->cfg & PREACFG_GRP_LIMIT_EN_BIT))
        {
            printk(KERN_ERR "Conflict: Pre-arbiter %d member limit is disabled\n", pre_arb);
        }
    }
    else
    {
        greg->mbr_nolimit |= PREAMBR_NOLIMIT_EN(mbr);
        if (!(greg->cfg & PREACFG_GRP_LIMIT_EN_BIT))
        {
            printk(KERN_ERR "NoNeed: Pre-arbiter %d member limit is disabled already\n", pre_arb);
        }
    }
    OUTREG16(base + REG_PREARB_MBR_NOLIMIT(), greg->mbr_nolimit);

    halPreArbRegsToggle(greg, base);
}

short halARBClientWcmdReserved(int id)
{
#if 1
    if (id < MIU0_CLIENT_WR_NUM)
    {
        return miu0_clients[id].bw_rsvd;
    }
#endif
    return 1;
}

short halARBClientRcmdReserved(int id)
{
#if 1
    if (id < MIU0_CLIENT_RD_NUM)
    {
        return miu0_clients[id].bw_rsvd;
    }
#endif
    return 1;
}

BOOL halARBIsPreFlowCTLEnable(struct miu_arb_pre_reg *greg, int mbr, char *period)
{
    *period = 0;

    if (greg->flowctrl0 & IGCFG_FCTL_EN_BIT)
    {
        *period = (greg->flowctrl0 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
        return 1;
    }
    if (greg->flowctrl1 & IGCFG_FCTL_EN_BIT)
    {
        *period = (greg->flowctrl1 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
        return 1;
    }
    return 0;
}

BOOL halARBIsGroupFlowCTLEnable(struct miu_arb_grp_reg *greg, int mbr, char *period)
{
    *period = 0;

    if (greg->flowctrl0 & IGCFG_FCTL_EN_BIT)
    {
        *period = (greg->flowctrl0 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
        return 1;
    }
    if (greg->flowctrl1 & IGCFG_FCTL_EN_BIT)
    {
        *period = (greg->flowctrl1 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
        return 1;
    }
    if (greg->flowctrl2 & IGCFG_FCTL_EN_BIT)
    {
        *period = (greg->flowctrl2 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
        return 1;
    }
    if (greg->flowctrl3 & IGCFG_FCTL_EN_BIT)
    {
        *period = (greg->flowctrl3 & IGFCTL_PERIOD_MASK) >> IGFCTL_PERIOD_SHIFT;
        return 1;
    }
    return 0;
}
BOOL halARBPreEnableFlowCTL(struct miu_arb_pre_reg *greg, int mbr, bool enable, int period, int base)
{
    int id[2];

    id[0] = (greg->flowctrl0 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
    id[1] = (greg->flowctrl1 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;

    if (enable)
    {
        do
        {
            if (!(greg->flowctrl0 & IGCFG_FCTL_EN_BIT))
            {
                greg->flowctrl0 = (mbr << IGFCTL_ID_SHIFT) | (period << IGFCTL_PERIOD_SHIFT);
                greg->flowctrl0 |= IGCFG_FCTL_EN_BIT;
                greg->flowctrl_no_req |= 0x1;
                break;
            }
            else if (!(greg->flowctrl1 & IGCFG_FCTL_EN_BIT))
            {
                greg->flowctrl1 = (mbr << IGFCTL_ID_SHIFT) | (period << IGFCTL_PERIOD_SHIFT);
                greg->flowctrl1 |= IGCFG_FCTL_EN_BIT;
                greg->flowctrl_no_req |= 0x1 << 1;
                break;
            }
            else
            {
                printk(KERN_ERR "No free flow control id\r\n");
                return 1; // not free one, failed
            }
        } while (1);
    }
    else
    {
        // disable client flow control
        if (greg->flowctrl0 & IGCFG_FCTL_EN_BIT)
        {
            if (mbr == id[0])
            {
                greg->flowctrl0 = 0;
            }
        }
        if (greg->flowctrl1 & IGCFG_FCTL_EN_BIT)
        {
            if (mbr == id[1])
            {
                greg->flowctrl1 = 0;
            }
        }
    }
    return 0;
}

void halARBPreClientFlowctrl(struct miu_arb_pre_reg *greg, int base, int g)
{
    OUTREG16(base + REG_IGRP_FCTL0(), greg->flowctrl0);
    OUTREG16(base + REG_IGRP_FCTL1(), greg->flowctrl1);
    OUTREG16(base + REG_PREA_NOT_REG_OFF(), greg->flowctrl_no_req);
    OUTREG16(base + REG_IGRP_CFG(), greg->cfg);
    halPreArbRegsToggle(greg, base);
}

void halARBGroupClientFlowctrl(struct miu_arb_grp_reg *greg, int base, int g)
{
    OUTREG16(base + REG_IGRP_FCTL0(), greg->flowctrl0);
    OUTREG16(base + REG_IGRP_FCTL1(), greg->flowctrl1);
    OUTREG16(base + REG_IGRP_FCTL2(), greg->flowctrl2);
    OUTREG16(base + REG_IGRP_FCTL3(), greg->flowctrl3);
    OUTREG16(base + REG_NOT_REQ_OFF(), greg->flowctrl_no_req);
    OUTREG16(base + REG_IGRP_CFG(), greg->cfg);
    halGrpArbRegsToggle(greg, base);
}

BOOL halARBGroupEnableFlowCTL(struct miu_arb_grp_reg *greg, int mbr, bool enable, int period, int base)
{
    int id[4];

    id[0] = (greg->flowctrl0 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
    id[1] = (greg->flowctrl1 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
    id[2] = (greg->flowctrl2 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;
    id[3] = (greg->flowctrl3 & IGFCTL_ID_MASK) >> IGFCTL_ID_SHIFT;

    if (enable)
    {
        do
        {
            if (!(greg->flowctrl0 & IGCFG_FCTL_EN_BIT))
            {
                greg->flowctrl0 = (mbr << IGFCTL_ID_SHIFT) | (period << IGFCTL_PERIOD_SHIFT);
                greg->flowctrl0 |= IGCFG_FCTL_EN_BIT;
                greg->flowctrl_no_req |= 0x1;
                break;
            }
            else if (!(greg->flowctrl1 & IGCFG_FCTL_EN_BIT))
            {
                greg->flowctrl1 = (mbr << IGFCTL_ID_SHIFT) | (period << IGFCTL_PERIOD_SHIFT);
                greg->flowctrl1 |= IGCFG_FCTL_EN_BIT;
                greg->flowctrl_no_req |= 0x1 << 1;
                break;
            }
            else if (!(greg->flowctrl2 & IGCFG_FCTL_EN_BIT))
            {
                greg->flowctrl2 = (mbr << IGFCTL_ID_SHIFT) | (period << IGFCTL_PERIOD_SHIFT);
                greg->flowctrl2 |= IGCFG_FCTL_EN_BIT;
                greg->flowctrl_no_req |= 0x1 << 2;
                break;
            }
            else if (!(greg->flowctrl3 & IGCFG_FCTL_EN_BIT))
            {
                greg->flowctrl3 = (mbr << IGFCTL_ID_SHIFT) | (period << IGFCTL_PERIOD_SHIFT);
                greg->flowctrl3 |= IGCFG_FCTL_EN_BIT;
                greg->flowctrl_no_req |= 0x1 << 3;
                break;
            }
            else
            {
                printk(KERN_ERR "No free flow control id\r\n");
                return 1; // not free one, failed
            }
        } while (1);
    }
    else
    {
        // disable client flow control
        if (greg->flowctrl0 & IGCFG_FCTL_EN_BIT)
        {
            if (mbr == id[0])
            {
                greg->flowctrl0 = 0;
            }
        }
        if (greg->flowctrl1 & IGCFG_FCTL_EN_BIT)
        {
            if (mbr == id[1])
            {
                greg->flowctrl1 = 0;
            }
        }
        if (greg->flowctrl2 & IGCFG_FCTL_EN_BIT)
        {
            if (mbr == id[2])
            {
                greg->flowctrl2 = 0;
            }
        }
        if (greg->flowctrl3 & IGCFG_FCTL_EN_BIT)
        {
            if (mbr == id[3])
            {
                greg->flowctrl3 = 0;
            }
        }
    }
    //    halGrpArbRegsToggle(greg, base);
    return 0;
}

void halARBResume(void)
{
    halARBLoadPolicy(0, MIU_ARB_POLICY_DEF);
}
