/*
 * cam_drv_poll.h- Sigmastar
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

#ifndef __CAM_DRV_POLL_H__
#define __CAM_DRV_POLL_H__

#define CAM_DRV_POLL_VERSION "v1.0.1"

#include <cam_os_wrapper.h>

#ifdef CAM_OS_RTK

#ifndef POLLIN
#define POLLIN     0x1
#define POLLPRI    0x2
#define POLLOUT    0x4
#define POLLERR    0x8
#define POLLRDNORM 0x40
#endif

struct file
{
    // u8    nPollval;      // the event to be polled
    s32   nPollTimeout; // used internally by poll
    void *private_data; // for drivers’ private use
};

typedef void poll_table;
#elif defined(__KERNEL__)
#include <linux/fs.h>
#include <linux/poll.h>
#endif

s32  CamDrvPollRegEventGrp(void);
void CamDrvPollDeRegEventGrp(u32 nEventID);
void CamDrvPollSetEvent(u32 nEventID, u32 nEventBits);
s32  CamDrvPollEvent(u32 nEventID, u32 nWaitBits, struct file *filp, poll_table *tPoll);

#endif /* __CAM_DRV_POLL_H__ */
