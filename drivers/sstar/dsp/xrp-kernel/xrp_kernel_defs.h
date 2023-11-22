/*
 * xrp_kernel_defs.h- Sigmastar
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

#ifndef _XRP_KERNEL_DEFS_H_
#define _XRP_KERNEL_DEFS_H_

#define XRP_IOCTL_MAGIC    'r'
#define XRP_IOCTL_ALLOC    _IO(XRP_IOCTL_MAGIC, 1)
#define XRP_IOCTL_FREE     _IO(XRP_IOCTL_MAGIC, 2)
#define XRP_IOCTL_QUEUE    _IO(XRP_IOCTL_MAGIC, 3)
#define XRP_IOCTL_QUEUE_NS _IO(XRP_IOCTL_MAGIC, 4)

struct xrp_ioctl_alloc
{
    __u32 size;
    __u32 align;
    __u64 addr;
};

enum
{
    XRP_FLAG_READ       = 0x1,
    XRP_FLAG_WRITE      = 0x2,
    XRP_FLAG_READ_WRITE = 0x3,
};

struct xrp_ioctl_buffer
{
    __u32 flags;
    __u32 size;
    __u64 addr;
};

enum
{
    XRP_QUEUE_FLAG_NSID       = 0x4,
    XRP_QUEUE_FLAG_PRIO       = 0xff00,
    XRP_QUEUE_FLAG_PRIO_SHIFT = 8,

    XRP_QUEUE_VALID_FLAGS = XRP_QUEUE_FLAG_NSID | XRP_QUEUE_FLAG_PRIO,
};

struct xrp_ioctl_queue
{
    __u32 flags;
    __u32 in_data_size;
    __u32 out_data_size;
    __u32 buffer_size;
    __u64 in_data_addr;
    __u64 out_data_addr;
    __u64 buffer_addr;
    __u64 nsid_addr;
};

#endif /*_XRP_KERNEL_DEFS_H_*/
