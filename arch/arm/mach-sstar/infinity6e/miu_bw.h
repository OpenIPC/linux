/*
* miu_bw.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: Alterman.lin <alterman.lin@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <linux/device.h>

/*=============================================================*/
// Macro definition
/*=============================================================*/

#ifndef MIU_NUM
#define MIU_NUM         (1)
#endif

#define OVERALL_CLIENT_ID   (0x00)
#define CPU_CLIENT_ID       (0x70)
#define DLA_HIWAY_ID        (0x71)
#define MIU_GRP_CLIENT_NUM  (0x10)
#define MIU_GRP_NUM         (3)
#define MIU_CLIENT_NUM      (0x32)  // CPU + DLA highway + (16 clients per group, total 3 groups)
#define MIU_ARB_CLIENT_NUM  (0x30)  // 16 clients per group, total 3 groups

#define MIU_IDX(c)          (((c - '0') > MIU_NUM) ? 0 : c - '0')

/* Bandwidth measurement related */

/* Bandwidth adjustment related */
// policy
#define MIU_ARB_POLICY_DEF      0
#define MIU_ARB_POLICY_BGA2_DEF 1
#define MIU_ARB_POLICY_NUM      2
// dump
#define MIU_ARB_DUMP_TEXT   0
#define MIU_ARB_DUMP_REG    1
#define MIU_ARB_DUMP_MAX    2
// group priority
#define MIU_ARB_OG_PRIO_0   0
#define MIU_ARB_OG_PRIO_1   1
#define MIU_ARB_OG_PRIO_2   2
#define MIU_ARB_OG_PRIO_3   3
#define MIU_ARB_OG_PRIO_NUM 4

/* Log color related */
#define ASCII_COLOR_RED     "\033[1;31m"
#define ASCII_COLOR_WHITE   "\033[1;37m"
#define ASCII_COLOR_YELLOW  "\033[1;33m"
#define ASCII_COLOR_BLUE    "\033[1;36m"
#define ASCII_COLOR_GREEN   "\033[1;32m"
#define ASCII_COLOR_END     "\033[0m"

/*=============================================================*/
// Structure definition
/*=============================================================*/

struct miu_device {
    struct device dev;
    int index;
    int reg_dram_size;  // register setting for dram size
};

// common for all MIU
struct miu_client {
    const char *name;
    const short id;
    const short rsvd;
};

// dedicated for each MIU
struct miu_client_bw {
    short enabled;
};

/*=============================================================*/
// Export Functions
/*=============================================================*/

extern const char* miu_client_id_to_name(U16 id);
extern short miu_client_reserved(U16 id);
extern void create_miu_arb_node(struct bus_type *miu_subsys);
#ifdef CONFIG_PM_SLEEP
extern void miu_arb_resume(void);
#endif

