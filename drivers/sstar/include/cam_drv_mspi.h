/*
 * cam_drv_mspi.h- Sigmastar
 *
 * Copyright (c) [2019~2021] SigmaStar Technology.
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
#ifndef _CAM_DRV_MSPI_H_
#define _CAM_DRV_MSPI_H_

#if defined(__KERNEL__)
#include <linux/spi/spidev.h>
typedef struct spidev_data       Mspidev_Data;
typedef struct spidev_cfg        Mspidev_Cfg;
typedef struct mspi_ioc_transfer Mspidev_xfer;
typedef struct
{
    u32   mspi_nr;
    void *Mspidev_Data;
} tmspiHandle;

#endif

int CamMspiOpen(tmspiHandle *mspihandle, u32 mspi_nr);
int CamMspiRel(tmspiHandle *mspihandle);
int CamMspiSet(tmspiHandle *mspihandle, Mspidev_Cfg *mspidev_cfg);
int CamMspiWrite(tmspiHandle *mspihandle, u8 *buf, u32 count);
int CamMspiRead(tmspiHandle *mspihandle, u8 *buf, u32 count);
int CamMspiTransfer(tmspiHandle *mspihandle, Mspidev_xfer *mspidev_xfer, u32 n_xfers);
#endif
