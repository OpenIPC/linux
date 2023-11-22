/*
 * mhal_synopsys.c- Sigmastar
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
#include "mhal_synopsys.h"

struct miu_synopsys_reg synopsys_reg[] = {
    {"PWRCTL", BASE_REG_MIU0_0, REG_ID_0C},         {"DFISTAT", BASE_REG_MIU0_0, REG_ID_6F},

    {"SCHED", BASE_REG_MIU0_1, REG_ID_14},          {"SCHED1", BASE_REG_MIU0_1, REG_ID_15},
    {"PERFHPR1", BASE_REG_MIU0_1, REG_ID_17},       {"PERFLPR1", BASE_REG_MIU0_1, REG_ID_19},
    {"PERFWR1", BASE_REG_MIU0_1, REG_ID_1B},        {"SCHED3", BASE_REG_MIU0_1, REG_ID_1C},
    {"SCHED4", BASE_REG_MIU0_1, REG_ID_1D},         {"DBG0", BASE_REG_MIU0_1, REG_ID_40},
    {"DBG1", BASE_REG_MIU0_1, REG_ID_41},           {"SWCTL", BASE_REG_MIU0_1, REG_ID_48},
    {"SWSTAT", BASE_REG_MIU0_1, REG_ID_49},         {"PSTAT", BASE_REG_MIU0_1, REG_ID_7F},

    {"PCTRL_0", BASE_REG_MIU0_AXI0, REG_ID_24},     {"PCFGQOS0_0", BASE_REG_MIU0_AXI0, REG_ID_25},
    {"PCFGQOS1_0", BASE_REG_MIU0_AXI0, REG_ID_26},  {"PCFGWQOS0_0", BASE_REG_MIU0_AXI0, REG_ID_27},
    {"PCFGWQOS1_0", BASE_REG_MIU0_AXI0, REG_ID_28}, {"PCFGR_1", BASE_REG_MIU0_AXI0, REG_ID_2D},
    {"PCFGW_1", BASE_REG_MIU0_AXI0, REG_ID_2E},     {"PCTRL_1", BASE_REG_MIU0_AXI0, REG_ID_50},
    {"PCFGQOS0_1", BASE_REG_MIU0_AXI0, REG_ID_51},  {"PCFGQOS1_1", BASE_REG_MIU0_AXI0, REG_ID_52},
    {"PCFGWQOS0_1", BASE_REG_MIU0_AXI0, REG_ID_53}, {"PCFGWQOS1_1", BASE_REG_MIU0_AXI0, REG_ID_54},
    {"PCFGR_2", BASE_REG_MIU0_AXI0, REG_ID_59},     {"PCFGW_2", BASE_REG_MIU0_AXI0, REG_ID_5A},
    {"PCTRL_2", BASE_REG_MIU0_AXI0, REG_ID_7C},     {"PCFGQOS0_2", BASE_REG_MIU0_AXI0, REG_ID_7D},

    {"PCFGQOS1_2", BASE_REG_MIU0_AXI1, REG_ID_00},  {"REG_NR", 0, 0},
};

struct miu_synopsys_cam_para synopsys_cam_para[] = {
    {"lpr_xact_run_length", "PERFLPR1", 0xFF000000, 24},
    {"lpr_max_starve", "PERFLPR1", 0x0000FFFF, 0},
    {"hpr_xact_run_length", "PERFHPR1", 0xFF000000, 24},
    {"hpr_max_starve", "PERFHPR1", 0x0000FFFF, 0},
    {"w_xact_run_length", "PERFWR1", 0xFF000000, 24},
    {"w_max_starve", "PERFWR1", 0x0000FFFF, 0},
    {"dis_collision_page_opt", "DBG0", 0x00000010, 4},
    {"dis_wc", "DBG0", 0x00000001, 0},
    {"dis_rd_bypass", "DBG0", 0x00000002, 1},
    {"opt_vprw_sch", "SCHED", 0x80000000, 31},
    {"rdwr_idle_gap", "SCHED", 0x70000000, 24},
    {"lpddr4_opt_act_timing", "SCHED", 0x00008000, 15},
    {"lpr_num_entries", "SCHED", 0x00007F00, 8},
    {"autopre_rmw", "SCHED", 0x00000080, 7},
    {"dis_opt_ntt_by_pre", "SCHED", 0x00000040, 6},
    {"dis_opt_ntt_by_act", "SCHED", 0x00000020, 5},
    {"opt_wrcam_fill_level", "SCHED", 0x00000010, 4},
    {"rdwr_switch_policy_sel", "SCHED", 0x00000008, 3},
    {"pageclose", "SCHED", 0x00000004, 2},
    {"prefer_write", "SCHED", 0x00000002, 1},
    {"dis_opt_wrecc_collision_flush", "SCHED", 0x00000001, 0},
    {"opt_hit_gt_hpr", "SCHED1", 0x80000000, 31},
    {"page_hit_limit_rd", "SCHED1", 0x70000000, 28},
    {"page_hit_limit_wr", "SCHED1", 0x07000000, 24},
    {"visible_window_limit_rd", "SCHED1", 0x00700000, 20},
    {"visible_window_limit_wr", "SCHED1", 0x00070000, 16},
    {"delay_switch_write", "SCHED1", 0x0000F000, 12},
    {"pageclose_timer", "SCHED1", 0x000000FF, 0},
    {"rd_pghit_num_thresh", "SCHED3", 0xFF000000, 24},
    {"wr_pghit_num_thresh", "SCHED3", 0x00FF0000, 16},
    {"wrcam_highthresh", "SCHED3", 0x0000FF00, 8},
    {"wrcam_lowthresh", "SCHED3", 0x000000FF, 0},
    {"wr_page_exp_cycles", "SCHED4", 0xFF000000, 24},
    {"rd_page_exp_cycles", "SCHED4", 0x00FF0000, 16},
    {"wr_act_idle_gap", "SCHED4", 0x0000FF00, 8},
    {"rd_act_idle_gap", "SCHED4", 0x000000FF, 0},
    {"PARA_NR", "REG_NR", 0, 0},
};

u32 halSynopsysGetRegValue(struct miu_synopsys_reg *reg)
{
    return INREG32(reg->bank + reg->offset);
}

u32 halSynopsysSetRegValue(struct miu_synopsys_reg *reg, u32 value)
{
    return OUTREG32(reg->bank + reg->offset, value);
}
