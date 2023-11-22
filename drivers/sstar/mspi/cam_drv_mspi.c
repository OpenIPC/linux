/*
 * cam_drv_mspi.c- Sigmastar
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
#include "cam_drv_mspi.h"
s32 CamMspiOpen(tmspiHandle *mspihandle, u32 mspi_nr)
{
#if defined(__KERNEL__)
    mspihandle->Mspidev_Data = (void *)mspidev_open(mspi_nr);
    if (mspihandle->Mspidev_Data)
        return 0;
    else
        return -1;
#endif
}
s32 CamMspiRel(tmspiHandle *mspihandle)
{
#if defined(__KERNEL__)
    return mspidev_release((struct spidev_data *)mspihandle->Mspidev_Data);
#endif
}

s32 CamMspiSet(tmspiHandle *mspihandle, Mspidev_Cfg *mspidev_cfg)
{
#if defined(__KERNEL__)
    return mspidev_set((struct spidev_data *)mspihandle->Mspidev_Data, mspidev_cfg);
#endif
}

s32 CamMspiWrite(tmspiHandle *mspihandle, u8 *buf, u32 count)
{
#if defined(__KERNEL__)
    return mspidev_write((struct spidev_data *)mspihandle->Mspidev_Data, buf, count);
#endif
}
s32 CamMspiRead(tmspiHandle *mspihandle, u8 *buf, u32 count)
{
#if defined(__KERNEL__)
    return mspidev_read((struct spidev_data *)mspihandle->Mspidev_Data, buf, count);
#endif
}
s32 CamMspiTransfer(tmspiHandle *mspihandle, Mspidev_xfer *mspidev_xfer, u32 n_xfers)
{
#if defined(__KERNEL__)
    return mspidev_transfer((struct spidev_data *)mspihandle->Mspidev_Data, mspidev_xfer, n_xfers);
#endif
}

#if defined(__KERNEL__)
EXPORT_SYMBOL(CamMspiOpen);
EXPORT_SYMBOL(CamMspiRel);
EXPORT_SYMBOL(CamMspiSet);
EXPORT_SYMBOL(CamMspiWrite);
EXPORT_SYMBOL(CamMspiRead);
EXPORT_SYMBOL(CamMspiTransfer);
#endif
