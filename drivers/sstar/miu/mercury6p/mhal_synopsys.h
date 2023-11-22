/*
 * mhal_synopsys.h- Sigmastar
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

#ifndef __MHAL_SYNOPSYS_H__
#define __MHAL_SYNOPSYS_H__

#include "MsTypes.h"
#include "mdrv_types.h"
#include "registers.h"
#include "ms_platform.h"
#include "regMIU.h"

// synopsys name bank and offset
struct miu_synopsys_reg
{
    char *name;
    u32   bank;
    u32   offset;
};

struct miu_synopsys_cam_para
{
    char *name;
    char *reg_name;
    u32   mask;
    u32   bit_shift;
};

struct miu_synopsys_handle
{
    char name[20];  // device name
    char dump_mode; // dump mode: readable text, register table
};

extern struct miu_synopsys_reg      synopsys_reg[];
extern struct miu_synopsys_cam_para synopsys_cam_para[];

#define MIU_SYNOPSYS_REG_NUM                                 \
    (                                                        \
        {                                                    \
            int i = 0;                                       \
            for (;; i++)                                     \
                if (!strcmp(synopsys_reg[i].name, "REG_NR")) \
                    break;                                   \
            i;                                               \
        })

#define MIU_SYNOPSYS_CAM_PARA_NUM                                  \
    (                                                              \
        {                                                          \
            int i = 0;                                             \
            for (;; i++)                                           \
                if (!strcmp(synopsys_cam_para[i].name, "PARA_NR")) \
                    break;                                         \
            i;                                                     \
        })

/*=============================================================*/
// Local function
/*=============================================================*/
u32 halSynopsysGetRegValue(struct miu_synopsys_reg *reg);
u32 halSynopsysSetRegValue(struct miu_synopsys_reg *reg, u32 value);
#endif
