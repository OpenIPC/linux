/*
* mdrv_rqct_io.h- Sigmastar
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
