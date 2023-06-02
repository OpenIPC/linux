////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

//! @file mdrv_mmfe_io.h
//! @author MStar Semiconductior Inc.
//! @brief MFEv5 Driver IOCTL User's Interface.
//! \defgroup mmfe_group MFEv5 driver
//! @{
#ifndef _MDRV_MMFE_IO_H_
#define _MDRV_MMFE_IO_H_

////////////////////////////////////////////////////////////////////////////////
// Header Files
////////////////////////////////////////////////////////////////////////////////
#include <mdrv_mmfe_st.h>

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

//! Magic Number of MFEv5 driver.
#define MAGIC_MMFE              ('m')
//! Use to Query version number of user interface.
#define IOCTL_MMFE_VERSION      _IOWR(MAGIC_MMFE, 0,unsigned int)
//! Use to set parameters out of streaming.
#define IOCTL_MMFE_S_PARM       _IOWR(MAGIC_MMFE, 1,mmfe_parm)
//! Use to get parameters any time.
#define IOCTL_MMFE_G_PARM       _IOWR(MAGIC_MMFE, 2,mmfe_parm)
//! Use to transit the state to streaming-on.
#define IOCTL_MMFE_STREAMON       _IO(MAGIC_MMFE, 3)
//! Use to transit the state to streaming-off.
#define IOCTL_MMFE_STREAMOFF      _IO(MAGIC_MMFE, 4)
//! Use to set control during streaming.
#define IOCTL_MMFE_S_CTRL       _IOWR(MAGIC_MMFE, 5,mmfe_ctrl)
//! Use to get control during streaming.
#define IOCTL_MMFE_G_CTRL       _IOWR(MAGIC_MMFE, 6,mmfe_ctrl)
//! Use to encode a picture during streaming.
#define IOCTL_MMFE_S_PICT       _IOWR(MAGIC_MMFE, 7,mmfe_buff)
//! Use to acquire the output bits of last coded picture.
#define IOCTL_MMFE_G_BITS       _IOWR(MAGIC_MMFE, 8,mmfe_buff)
//! Use to encode a picture and acquire the output at the same time.
#define IOCTL_MMFE_ENCODE       _IOWR(MAGIC_MMFE, 9,mmfe_buff[2])
//! Use to put user data.
#define IOCTL_MMFE_S_DATA       _IOWR(MAGIC_MMFE,10,mmfe_buff)

#endif//_MDRV_MMFE_IO_H_
//! @}

