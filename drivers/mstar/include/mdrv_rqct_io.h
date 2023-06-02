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

//! @file mdrv_rqct_io.h
//! @author MStar Semiconductior Inc.
//! @brief RQCT Driver IOCTL User's Interface.
//! \defgroup rqct_group RQ controller
//! @{
#ifndef _MDRV_RQCT_IO_H_
#define _MDRV_RQCT_IO_H_

////////////////////////////////////////////////////////////////////////////////
// Header Files
////////////////////////////////////////////////////////////////////////////////
#include <mdrv_rqct_st.h>

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

//! Magic Number of RQCT.
#define MAGIC_RQCT          ('q')
//! Use to Query version number of user interface.
#define IOCTL_RQCT_VERSION  _IOWR(MAGIC_RQCT, 0,unsigned int)
//! Use to set rq-control during streaming.
#define IOCTL_RQCT_S_CONF   _IOWR(MAGIC_RQCT, 1,rqct_conf)
//! Use to get rq-control during streaming.
#define IOCTL_RQCT_G_CONF   _IOWR(MAGIC_RQCT, 2,rqct_conf)

#endif//_MDRV_RQCT_IO_H_
//! @}
