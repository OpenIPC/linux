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
#define REG_IGRP_CFG()         (REG_ID_13)
#define IGCFG_ARB_TOGGLE       BIT2
#define IGCFG_GRP_LIMIT_EN_BIT BIT4

    /*r flowctrl offset 0x16~0x19*/
    /*w flowctrl offset 0x46~0x49*/
    u16 flowctrl0;
#define IGCFG_FCTL_EN_BIT       0x1
#define REG_IGRP_FCTL0()        (REG_ID_16)
#define IGFCTL_ID_SHIFT         4
#define IGFCTL_ID_MASK          0x00F0
#define IGFCTL_PERIODPASS_SHIFT 2
#define IGFCTL_PERIODPASS_MASK  0xc
#define IGFCTL_PERIOD_SHIFT     8
#define IGFCTL_PERIOD_MASK      0xFF00
    u16 flowctrl1;
#define REG_IGRP_FCTL1() (REG_ID_17)
    u16 flowctrl2;
#define REG_IGRP_FCTL2() (REG_ID_18)
    u16 flowctrl3;
#define REG_IGRP_FCTL3() (REG_ID_19)
    u16 flowctrl_no_req;
#define REG_NOT_REQ_OFF() (REG_ID_1A)
    //    not_reg_off_baseR = 0x1a;
    //    not_reg_off_baseW = 0x4a;

    u16 burst; // bit[7:0] member burst length; bit[15:8] group burst length
#define REG_IGRP_BURST()  (REG_ID_20)
#define IGBURST_LSB_SHIFT 0
#define IGBURST_LSB_MASK  0x00FF
#define IGBURST_MSB_SHIFT 8
#define IGBURST_MSB_MASK  0xFF00

    u16 burst_1; // bit[7:0] member burst length; bit[15:8] group burst length
#define REG_IGRP_BURST_1() (REG_ID_21)

    u16 mbr_sel0;
#define REG_IGRP_MBR_SEL0()   (REG_ID_22)
#define IGMBR_SEL0(mbr, sel)  (sel << (mbr << 1))
#define IGMBR_SEL0_MASK(mbr)  (0x3 << (mbr << 1))
#define IGMBR_SEL0_SHIFT(mbr) ((mbr << 1))
    u16 mbr_sel1;
#define REG_IGRP_MBR_SEL1()   (REG_ID_23)
#define IGMBR_SEL1(mbr, sel)  (sel << ((mbr - 8) << 1))
#define IGMBR_SEL1_MASK(mbr)  (0x3 << ((mbr - 8) << 1))
#define IGMBR_SEL1_SHIFT(mbr) ((mbr - 8) << 1)
    u16 mbr_nolimit;
#define REG_IGRP_MBR_NOLIMIT() (REG_ID_28)
#define IGMBR_NOLIMIT_EN(mbr)  (1 << mbr)
};

struct miu_arb_pre_reg
{            // pre arbiter
    u16 cfg; // inner pre arbitration config
#define REG_PRE_ARB_CFG()        (REG_ID_13)
#define PREACFG_ARB_TOGGLE       BIT2
#define PREACFG_GRP_LIMIT_EN_BIT BIT4

    u16 burst; // bit[7:0] member burst length; bit[15:8] group burst length
#define REG_PREARB_BURST()  (REG_ID_20)
#define PREABURST_LSB_SHIFT 0
#define PREABURST_LSB_MASK  0x00FF
#define PREABURST_MSB_SHIFT 8
#define PREABURST_MSB_MASK  0xFF00

    u16 burst_1; // bit[7:0] member burst length; bit[15:8] group burst length
#define REG_PREARB_BURST_1() (REG_ID_21)
    u16 mbr_sel0;
#define REG_PREARB_MBR_SEL0()   (REG_ID_22)
#define PREAMBR_SEL0(mbr, sel)  (sel << (mbr << 1))
#define PREAMBR_SEL0_MASK(mbr)  (0x3 << (mbr << 1))
#define PREAMBR_SEL0_SHIFT(mbr) ((mbr << 1))
    u16 mbr_sel1;
#define REG_PREARB_MBR_SEL1()   (REG_ID_23)
#define PREAMBR_SEL1(mbr, sel)  (sel << ((mbr - 8) << 1))
#define PREAMBR_SEL1_MASK(mbr)  (0x3 << ((mbr - 8) << 1))
#define PREAMBR_SEL1_SHIFT(mbr) ((mbr - 8) << 1)
    u16 mbr_nolimit;
#define REG_PREARB_MBR_NOLIMIT() (REG_ID_28)
#define PREAMBR_NOLIMIT_EN(mbr)  (1 << mbr)

    u16 flowctrl0;
#define PREACFG_FCTL_EN_BIT       0x1
#define REG_PREA_FCTL0()          (REG_ID_16)
#define PREAFCTL_ID_SHIFT         4
#define PREAFCTL_ID_MASK          0x00F0
#define PREAFCTL_PERIODPASS_SHIFT 2
#define PREAFCTL_PERIODPASS_MASK  0xc
#define PREAFCTL_PERIOD_SHIFT     16
#define PREAFCTL_PERIOD_MASK      0xFF00
    u16 flowctrl1;
#define REG_PREA_FCTL1() (REG_ID_17)
    u16 flowctrl_no_req;
#define REG_PREA_NOT_REG_OFF() (REG_ID_18)
    //#define REG_PREA_FCTL1() (REG_ID_17)

    //    not_reg_off_baseR = 0x18;
    //    not_reg_off_baseW = 0x48;
};

struct miu_arb_reg
{
    struct miu_arb_grp_reg grp_rcmd[MIU_GRP_NUM];
    struct miu_arb_grp_reg grp_wcmd[MIU_GRP_NUM];
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
int   halArbGetSpecOrderClientID(unsigned short idx);
int   halGroupArbGetClientGrpID(unsigned short clientid);
int   halArbPreGetID(unsigned short clientid);
short halARBClientWcmdReserved(int id);
short halARBClientRcmdReserved(int id);
// int halARBGetRegBase(int miu);
// int halARBGetRegBase_e(void);
int  halGrpArbGetRegBase(unsigned short grp_idx, unsigned short wcmd);
int  halPreArbGetRegBase(unsigned short prearb_idx, unsigned short wcmd);
void halARBGroupLoadSetting(struct miu_arb_grp_reg *greg, int base);
void halARBPreLoadSetting(struct miu_arb_pre_reg *greg, int base);

void halARBLoadPolicy(int miu, int idx);
void halARBGroupOrderBufferStore(struct miu_arb_grp_reg *greg, int base, int win_index, int mbr);
void halARBPreOrderBufferStore(struct miu_arb_pre_reg *greg, int base, int win_index, int mbr);
void halARBGroupClientBurstSelStore(struct miu_arb_grp_reg *greg, int base, int sel, int mbr);
void halARBPreClientBurstSelStore(struct miu_arb_pre_reg *greg, int base, int sel, int mbr);
void halARBGroupBurstStore(struct miu_arb_grp_reg *greg, int base, int idx, int burst);
void halARBPreBurstStore(struct miu_arb_pre_reg *greg, int base, int idx, int burst);
void halARBGroupClientNolimitStore(struct miu_arb_grp_reg *greg, int base, int c, int nolimit);
void halARBPreClientNolimitStore(struct miu_arb_pre_reg *greg, int base, int c, int nolimit);

void halARBResume(void);
int  halARBGroupGetOrderBuffer(int base, int win_index);
int  halARBPreGetOrderBuffer(int base, int win_index);
int  halARBGroupGetClientBurst(struct miu_arb_grp_reg *greg, int mbr);
int  halARBGroupGetClientBurstSel(struct miu_arb_grp_reg *greg, int mbr);
int  halARBPreGetClientBurst(struct miu_arb_pre_reg *greg, int mbr);
int  halARBPreGetClientBurstSel(struct miu_arb_pre_reg *greg, int mbr);
void halARBGroupClientFlowctrl(struct miu_arb_grp_reg *greg, int base, int g);
BOOL halARBIsGroupFlowCTLEnable(struct miu_arb_grp_reg *greg, int mbr, char *period);
BOOL halARBGroupEnableFlowCTL(struct miu_arb_grp_reg *greg, int mbr, bool enable, int period, int base);
BOOL halARBIsPreFlowCTLEnable(struct miu_arb_pre_reg *greg, int mbr, char *period);
BOOL halARBPreEnableFlowCTL(struct miu_arb_pre_reg *greg, int mbr, bool enable, int period, int base);
void halARBPreClientFlowctrl(struct miu_arb_pre_reg *greg, int base, int g);

#endif
