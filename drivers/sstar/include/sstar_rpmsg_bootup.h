/*
 * sstar_rpmsg_bootup.h- Sigmastar
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

#ifndef _SSTAR_RPMSG_BOOTUP_H_
#define _SSTAR_RPMSG_BOOTUP_H_

#include "rpmsg_dualos_common.h"

#define EPT_ADDR_BOOTUP_SERVER (EPT_TYPE(EPT_TYPE_SIGMASTAR) | (0x200))

#define BOOTUP_TYPE_LOAD_IMAGE   (0x1)
#define BOOTUP_TYPE_BOOTUP       (0x2)
#define BOOTUP_TYPE_STAGE_NOTIFY (0x3)
#define BOOTUP_TYPE_RETURN       (0x4)

// units: second
#define LOAD_IMAGE_TIMEOUT  (60)
#define BOOTUP_TIMEOUT      (60)
#define BOOTUP_DONE_TIMEOUT (300)

struct bootup_common_header
{
    struct dualos_interos_header header;
    u8                           type;
};

struct bootup_image_info
{
    struct bootup_common_header common;
    char                        name[32];
    u64                         reserved;
    u64                         addr_phys;
    u64                         load_addr_phys;
    u64                         size;
};

struct bootup_info
{
    struct bootup_common_header common;
    char                        name[32];
    u64                         entry;
    u64                         bootargs_addr_phys;
    u64                         bootargs_load_addr_phys;
    u64                         bootargs_size;
};

struct bootup_stage_notify
{
    struct bootup_common_header common;
    u32                         stage;
};

struct bootup_return
{
    struct bootup_common_header common;
    int                         ret;
};

#define DUALOS_BOOTUP_MAGIC (0xf3f3f3f3)
#endif
