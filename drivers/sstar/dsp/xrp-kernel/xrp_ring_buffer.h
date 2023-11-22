/*
 * xrp_ring_buffer.h- Sigmastar
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

#ifndef _XRP_RING_BUFFER_H_
#define _XRP_RING_BUFFER_H_

struct xrp_ring_buffer
{
    __u32 read;
    __u32 write;
    __u32 size;
    __u32 reserved[1];
    __u8  data[0];
};

#endif /*_XRP_RING_BUFFER_H_*/
