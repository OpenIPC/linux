/*
* cam_dev_pollsample.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: giggs.huang <giggs.huang@sigmastar.com.tw>
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

#ifndef __CAM_DEV_POLLSAMPLE_H__
#define __CAM_DEV_POLLSAMPLE_H__

int CamDevPollsampleOpen(char* name);
int CamDevPollsampleClose(int nFd);
int CamDevPollsampleIoctl(int nFd, unsigned long request, void *param);
int CamDevPollsamplePoll(s32 nFd, s16 nEvent, s16* pnRevent, s32 nTimeout);

#endif /* __CAM_DEV_POLLSAMPLE_H__ */
