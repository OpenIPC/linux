/*
 * mhal_arb.h- Sigmastar
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
#ifndef __HAL_ARB__
#define __HAL_ARB__

#include "MsTypes.h"
#include "mdrv_types.h"
#include "registers.h"
#include "ms_platform.h"
#include "regMIU.h"

#ifndef MIU_NUM
#define MIU_NUM (1)
#endif

#define MIU_IDX(c) (((c - '0') > MIU_NUM) ? 0 : c - '0')

/* Bandwidth adjustment related */
// policy
#define MIU_ARB_POLICY_DEF      0
#define MIU_ARB_POLICY_BGA2_DEF 1
#define MIU_ARB_POLICY_NUM      2
// dump
#define MIU_ARB_DUMP_TEXT 0
#define MIU_ARB_DUMP_REG  1
#define MIU_ARB_DUMP_MAX  2
// group priority
#define MIU_ARB_OG_PRIO_0   0
#define MIU_ARB_OG_PRIO_1   1
#define MIU_ARB_OG_PRIO_2   2
#define MIU_ARB_OG_PRIO_3   3
#define MIU_ARB_OG_PRIO_NUM 4

struct miu_arb_grp_reg
{
    u16 cfg; // inner group arbitration config
#define REG_IGRP_CFG(g)        (REG_ID_20 + BK_REG(0x10 * g))
#define IGCFG_ROUND_ROBIN_BIT  0x0001 // bit 0
#define IGCFG_FIX_PRIO_BIT     0x0002 // bit 1
#define IGCFG_MBR_LIMIT_EN_BIT 0x0004 // bit 2
#define IGCFG_GRP_LIMIT_EN_BIT 0x0008 // bit 3
#define IGCFG_FCTL0_EN_BIT     0x0100 // bit 8
#define IGCFG_FCTL1_EN_BIT     0x0200 // bit 9
    u16 burst;                        // bit[7:0] member burst length; bit[15:8] group burst length
#define REG_IGRP_BURST(g) (REG_ID_21 + BK_REG(0x10 * g))
#define IGBURST_MBR_SHIFT 0
#define IGBURST_MBR_MASK  0x00FF
#define IGBURST_GRP_SHIFT 8
#define IGBURST_GRP_MASK  0xFF00
    u16 flowctrl0;
#define REG_IGRP_FCTL0(g)   (REG_ID_2A + BK_REG(0x10 * g))
#define IGFCTL_ID0_SHIFT    0
#define IGFCTL_ID0_MASK     0x000F
#define IGFCTL_ID1_SHIFT    4
#define IGFCTL_ID1_MASK     0x00F0
#define IGFCTL_PERIOD_SHIFT 8
#define IGFCTL_PERIOD_MASK  0xFF00
    u16 flowctrl1;
#define REG_IGRP_FCTL1(g) (REG_ID_2B + BK_REG(0x10 * g))
    u16 mbr_priority;
#define REG_IGRP_MBR_PRIO(g)  (REG_ID_2C + BK_REG(0x10 * g))
#define IGMBR_PRIO(mbr, pri)  (pri << mbr)
#define IGMBR_PRIO_MASK(mbr)  (1 << mbr)
#define IGMBR_PRIO_SHIFT(mbr) (mbr)
    u16 mbr_nolimit;
#define REG_IGRP_MBR_NOLIMIT(g) (REG_ID_2E + BK_REG(0x10 * g))
#define IGMBR_NOLIMIT_EN(mbr)   (1 << mbr)
};
struct miu_arb_grp_reg_e
{
    u16 burst_23; // bit[7:0] member2 burst length; bit[15:8] member3 burst length
#define REG_IGRP_BURST_EX23(g) (REG_ID_30 + BK_REG(0x4 * g))
#define IGBURST_MBR2_SHIFT     0
#define IGBURST_MBR2_MASK      0x00FF
#define IGBURST_MBR3_SHIFT     8
#define IGBURST_MBR3_MASK      0xFF00
    u16 burst_4; // bit[7:0] member4 burst length;
#define REG_IGRP_BURST_EX4(g) (REG_ID_31 + BK_REG(0x4 * g))
#define IGBURST_MBR4_SHIFT    0
#define IGBURST_MBR4_MASK     0x00FF
    u16 mbr_sel0;
#define REG_IGRP_MBR_SEL0(g)  (REG_ID_32 + BK_REG(0x4 * g))
#define IGMBR_SEL0(mbr, sel)  (sel << (mbr << 1))
#define IGMBR_SEL0_MASK(mbr)  (0x3 << (mbr << 1))
#define IGMBR_SEL0_SHIFT(mbr) ((mbr << 1))
    u16 mbr_sel1;
#define REG_IGRP_MBR_SEL1(g)  (REG_ID_33 + BK_REG(0x4 * g))
#define IGMBR_SEL1(mbr, sel)  (sel << ((mbr - 8) << 1))
#define IGMBR_SEL1_MASK(mbr)  (0x3 << ((mbr - 8) << 1))
#define IGMBR_SEL1_SHIFT(mbr) (((mbr - 8) << 1))
};
struct miu_arb_reg
{
    struct miu_arb_grp_reg   grp[MIU_GRP_NUM];
    struct miu_arb_grp_reg_e grp_e[MIU_GRP_NUM];
    u16                      cfg; // outer groups arbitration config
#define REG_OGRP_CFG            (REG_ID_7F)
#define OGCFG_GRP_PRIO_SHIRT(p) (p << 1)
#define OGCFG_GRP_PRIO_MASK(p)  (0x3 << (p << 1))
#define OGCFG_FIX_PRIO_EN       0x0100 // bit 8
#define OGCFG_ROUND_ROBIN_EN    0x1000 // bit 12
};

struct miu_arb_handle
{
    char name[12];        // device name (miu_arbx)
    char dump;            // dump mode: readable text, register table
    char group_selected;  // group selected, 0~(MIU_GRP_NUM-1)
    char client_selected; // client selected, 1~(MIU_CLIENT_NUM-2)
};

/*=============================================================*/
// Local variable
/*=============================================================*/
short halARBClientReserved(int id);
int   halARBGetRegBase(int miu);
int   halARBGetRegBase_e(void);
void  halARBLoadGroupSetting(struct miu_arb_grp_reg *greg, int base, int g);
BOOL  halARBIsFlowCTLEnable(struct miu_arb_grp_reg *greg, int mbr, char *period);
BOOL  halARBEnableFlowCTL(struct miu_arb_grp_reg *greg, int mbr, bool enable, char period);
void  halARBLoadPolicy(int miu, int idx);
int   halARBGetOGCCFG(int base);
void  halARBGroupBurstStore(struct miu_arb_grp_reg *greg, int base, int g, int burst);
void  halARBGroupMemberBurstStore(struct miu_arb_grp_reg *greg, int base, int g, int idx, int burst);
void  halARBClientNolimitStore(struct miu_arb_grp_reg *greg, int base, int c, int nolimit);
void  halARBClientPrioprityStore(struct miu_arb_grp_reg *greg, int base, int g);
void  halARBClientFlowctrl(struct miu_arb_grp_reg *greg, int base, int g);
void  halARBResume(void);
int   halARBGetClientBurst(struct miu_arb_grp_reg *greg, struct miu_arb_grp_reg_e *greg_e, int mbr, int g);
int   halARBGetClientMaxMbrServIdx(struct miu_arb_grp_reg_e *greg, int mbr, int g);
void  halARBLoadGroupSettings_e(struct miu_arb_grp_reg_e *greg, int base, int g);
void  halARBLoadGroupSetting(struct miu_arb_grp_reg *greg, int base, int g);
#endif
