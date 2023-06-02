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

//! @file mdrv_mvhe_io.h
//! @author MStar Semiconductior Inc.
//! @brief VHE Driver IOCTL User's Interface.
//! \defgroup mvhe_group VHE driver
//! @{
#ifndef _MDIV_MVHE_IO_H_
#define _MDIV_MVHE_IO_H_

////////////////////////////////////////////////////////////////////////////////
// Header Files
////////////////////////////////////////////////////////////////////////////////
#include <mdrv_mvhe_st.h>

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

//! Magic Number of MFEv5 driver.
#define MAGIC_MVHE              ('v')
//! Use to Query version number of user interface.
#define IOCTL_MVHE_VERSION      _IOWR(MAGIC_MVHE, 0,unsigned int)
//! Use to set parameters out of streaming.
#define IOCTL_MVHE_S_PARM       _IOWR(MAGIC_MVHE, 1,mvhe_parm)
//! Use to get parameters any time.
#define IOCTL_MVHE_G_PARM       _IOWR(MAGIC_MVHE, 2,mvhe_parm)
//! Use to transit the state to streaming-on.
#define IOCTL_MVHE_STREAMON       _IO(MAGIC_MVHE, 3)
//! Use to transit the state to streaming-off.
#define IOCTL_MVHE_STREAMOFF      _IO(MAGIC_MVHE, 4)
//! Use to set control during streaming.
#define IOCTL_MVHE_S_CTRL       _IOWR(MAGIC_MVHE, 5,mvhe_ctrl)
//! Use to get control during streaming.
#define IOCTL_MVHE_G_CTRL       _IOWR(MAGIC_MVHE, 6,mvhe_ctrl)
//! Use to encode a picture  during streaming.
#define IOCTL_MVHE_S_PICT       _IOWR(MAGIC_MVHE, 7,mvhe_buff)
//! Use to acquire the output bits of last coded picture.
#define IOCTL_MVHE_G_BITS       _IOWR(MAGIC_MVHE, 8,mvhe_buff)
//! Use to encode a picture and acquire the output at the same time.
#define IOCTL_MVHE_ENCODE       _IOWR(MAGIC_MVHE, 9,mvhe_buff[2])
//! Use to put user data.
#define IOCTL_MVHE_S_DATA       _IOWR(MAGIC_MVHE,10,mvhe_buff)

#endif//_MDIV_MVHE_IO_H_
//! @}

