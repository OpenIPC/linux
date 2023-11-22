/*
 * dualos-adaptor-common.h- Sigmastar
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

#ifndef _DUALOS_ADAPTOR_COMMON_H_
#define _DUALOS_ADAPTOR_COMMON_H_

#include <linux/skbuff.h>
#include <linux/kthread.h>
#include "rpmsg_dualos.h"

typedef struct dualos_interos_header dualos_interos_header;

typedef struct
{
    dualos_interos_header header;
    int                   err;
    int                   ret;
    int                   cmd;
    int                   len;
    u8                    data[0];
} adaptor_data;

typedef struct
{
    struct dualos_rpmsg_node *dnode;
    struct mutex              mutex;
    unsigned int              index;
    int                       remote_addr;
    int                       mod_id;
    int                       dev_id;
    int                       chn_id;
} adaptor_ept;

#define DUALOS_ADAPTOR_MAGIC (0xf2f2f2f2)

#endif /* _DUALOS_ADAPTOR_COMMON_H_ */
