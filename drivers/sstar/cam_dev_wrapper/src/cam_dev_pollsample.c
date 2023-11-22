/*
 * cam_dev_pollsample.c- Sigmastar
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

#include "cam_os_wrapper.h"
#include "cam_dev_wrapper.h"
#include <cam_drv_poll.h>
#include <mdrv_pollsample_module.h>

//=============================================================================
int CamDevPollsampleOpen(char *name)
{
    struct file *filp;

    if (name)
    {
    }

    filp = (struct file *)CamOsMemCalloc(1, sizeof(struct file));
    if (pollsamp_open(filp) == 0) /* success */
    {
        return (int)filp;
    }
    else
    {
        CamOsMemRelease(filp);
        return -1;
    }
}

int CamDevPollsampleClose(int nFd)
{
    struct file *filp = (struct file *)nFd;

    pollsamp_release(filp);
    CamOsMemRelease(filp);

    return CAM_OS_OK;
}

int CamDevPollsampleIoctl(int nFd, unsigned long request, void *param)
{
    int          nRet = CAM_OS_OK;
    struct file *filp = (struct file *)nFd;

    if (pollsamp_ioctl(filp, request, (unsigned long)param) != 0)
    {
        nRet = CAM_OS_FAIL;
    }

    return nRet;
}

int CamDevPollsamplePoll(s32 nFd, s16 nEvent, s16 *pnRevent, s32 nTimeout)
{
    struct file *filp = (struct file *)nFd;
    u32          nEventKept, nRevent;

    filp->nPollTimeout = nTimeout;
    nRevent            = pollsamp_poll(filp, NULL);

    *pnRevent = nRevent & nEvent;
    CamOsPrintf("%s: revent=0x%x\n", __func__, *pnRevent);
    return (*pnRevent != 0) ? 1 : 0;
}
