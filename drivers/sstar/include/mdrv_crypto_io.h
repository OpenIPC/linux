/*
* mdrv_crypto_io.h- Sigmastar
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
#ifndef _MDRV_AESDMA_IO_H
#define _MDRV_AESDMA_IO_H

#define IOCTL_AESDMA_GSESSION     (102)
#define IOCTL_AESDMA_FSESSION     (103)
#define IOCTL_AESDMA_CRYPT        (104)

#define CIOCGSESSION    _IO('c', IOCTL_AESDMA_GSESSION)
#define CIOCFSESSION    _IO('c', IOCTL_AESDMA_FSESSION)
#define CIOCCRYPT       _IO('c', IOCTL_AESDMA_CRYPT)


#endif
