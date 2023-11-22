/*
 * xrp_internal.h- Sigmastar
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

#ifndef _XRP_INTERNAL_H_
#define _XRP_INTERNAL_H_

#include <linux/completion.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include "xrp_address_map.h"

struct device;
struct firmware;
struct xrp_hw_ops;
struct xrp_allocation_pool;

struct xrp_comm
{
    struct mutex      lock;
    void __iomem *    comm;
    struct completion completion;
    u32               priority;
};

struct xvp
{
    struct device *          dev;
    const char *             firmware_name;
    const struct firmware *  firmware;
    struct miscdevice        miscdev;
    const struct xrp_hw_ops *hw_ops;
    void *                   hw_arg;
    unsigned                 n_queues;

    u32 *             queue_priority;
    struct xrp_comm * queue;
    struct xrp_comm **queue_ordered;
    void __iomem *    comm;
    phys_addr_t       pmem;
    phys_addr_t       comm_phys;
    phys_addr_t       shared_size;
    atomic_t          reboot_cycle;
    atomic_t          reboot_cycle_complete;

    struct xrp_address_map address_map;

    bool host_irq_mode;

    struct xrp_allocation_pool *pool;
    bool                        off;
    int                         nodeid;
};

#endif /*_XRP_INTERNAL_H_*/
