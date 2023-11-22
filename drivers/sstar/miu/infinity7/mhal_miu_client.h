/*
 * mhal_miu_client.h- Sigmastar
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
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/device.h>
#if defined(CONFIG_COMPAT)
#include <linux/compat.h>
#endif
#include "regMIU.h"

#define MIU_CH_NONE 0
#define MIU_CH_RCMD 1
#define MIU_CH_WCMD 2

// common for all MIU
struct miu_client
{
    char* name;
    short bw_client_id;
    short bw_rsvd;
    short bw_enabled;
    short bw_dump_en;
    short bw_filter_en;
    short bw_max;
    short bw_avg;
    short bw_min;
    short effi_avg;
    short effi_min;
    short effi_max;
    short bw_max_div_effi;
    short bw_avg_div_effi;
};

extern struct miu_client miu0_wr_clients[];
extern struct miu_client miu0_rd_clients[];
#if 1
#define __MIU_CLIENT_NUM(a)                           \
    (                                                 \
        {                                             \
            int i = 0;                                \
            for (;; i++)                              \
                if (!strcmp(a[i].name, "CLIENT_NR ")) \
                    break;                            \
            i;                                        \
        })
#define MIU0_CLIENT_WR_NUM __MIU_CLIENT_NUM(miu0_wr_clients)
#define MIU0_CLIENT_RD_NUM __MIU_CLIENT_NUM(miu0_rd_clients)
#else

#define MIU0_CLIENT_WR_NUM sizeof(miu0_wr_clients) / sizeof(miu0_wr_clients[0])
#define MIU0_CLIENT_RD_NUM sizeof(miu0_rd_clients) / sizeof(miu0_rd_clients[0])
//#define MIU1_CLIENT_NUM sizeof(miu1_clients)/sizeof(miu1_clients[0])
#endif
