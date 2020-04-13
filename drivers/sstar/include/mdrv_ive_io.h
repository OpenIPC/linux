/*
* mdrv_ive_io.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: karl.xiao <karl.xiao@sigmastar.com.tw>
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
#ifndef _MDRV_IVE_IO_H_
#define _MDRV_IVE_IO_H_

#define IVE_IOC_MAGIC 'I'
#define IVE_IOC_PROCESS                _IOW(IVE_IOC_MAGIC, 1, ive_ioc_config*)

#endif // _MDRV_IVE_IO_H_
