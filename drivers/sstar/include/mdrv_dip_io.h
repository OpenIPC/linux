/*
* mdrv_dip_io.h- Sigmastar
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
#ifndef _MDRV_DIP_IO_H
#define _MDRV_DIP_IO_H

//=============================================================================
// Includs
//=============================================================================

//=============================================================================
// Defines
//=============================================================================
// library information
#define MSIF_DIP_LIB_CODE               {'D','I','P','\0'}
#define MSIF_DIP_BUILDNUM               {'_','0','1','\0'}
#define MSIF_DIP_LIBVER                 (2)
#define MSIF_DIP_CHANGELIST             (677450)

//IO Ctrl defines:

#define IOCTL_DIP_SET_INTERRUP_NR             (0)
#define IOCTL_DIP_SET_CONFIG_NR               (1)
#define IOCTL_DIP_SET_WONCE_BASE_CONFIG_NR    (2)
#define IOCTL_DIP_GET_INTERRUPT_STATUS_NR     (3)
#define IOCTL_DIVP_INIT_CONFIG_NR     (4)
#define IOCTL_DIVP_DEINIT_CONFIG_NR     (5)
#define IOCTL_DIVP_CREATE_INST_NR     (6)
#define IOCTL_DIVP_DESTROY_INST_NR     (7)
#define IOCTL_DIVP_PROCESS_DRAM_DATA_NR     (8)
#define IOCTL_DIVP_CAPTURE_TIMING_NR     (9)
#define IOCTL_DIVP_SET_ATTR_TNR_NR     (10)
#define IOCTL_DIVP_SET_ATTR_DITYPE_NR     (11)
#define IOCTL_DIVP_SET_ATTR_ROTATE_NR     (12)
#define IOCTL_DIVP_SET_ATTR_WINDOW_NR     (13)
#define IOCTL_DIVP_SET_ATTR_MIRROR_NR     (14)
#define IOCTL_DIVP_SET_COVER_NR     (15)

#define IOCTL_DIP_MAX_NR                      (16)


// use 'm' as magic number
#define IOCTL_DIP_MAGIC                   ('3')


#define IOCTL_DIP_SET_INTERRUPT             _IO(IOCTL_DIP_MAGIC,  IOCTL_DIP_SET_INTERRUP_NR)
#define IOCTL_DIP_SET_CONFIG                _IO(IOCTL_DIP_MAGIC,  IOCTL_DIP_SET_CONFIG_NR)
#define IOCTL_DIP_SET_WONCE_BASE_CONFIG     _IO(IOCTL_DIP_MAGIC,  IOCTL_DIP_SET_WONCE_BASE_CONFIG_NR)
#define IOCTL_DIP_GET_INTERRUPT_STATUS      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIP_GET_INTERRUPT_STATUS_NR)
#define IOCTL_DIVP_INIT_CONFIG      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIVP_INIT_CONFIG_NR)
#define IOCTL_DIVP_DEINIT_CONFIG      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIVP_DEINIT_CONFIG_NR)
#define IOCTL_DIVP_CREATE_INST      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIVP_CREATE_INST_NR)
#define IOCTL_DIVP_DESTROY_INST      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIVP_DESTROY_INST_NR)
#define IOCTL_DIVP_PROCESS_DRAM_DATA      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIVP_PROCESS_DRAM_DATA_NR)
#define IOCTL_DIVP_CAPTURE_TIMING      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIVP_CAPTURE_TIMING_NR)
#define IOCTL_DIVP_SET_ATTR_TNR      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIVP_SET_ATTR_TNR_NR)
#define IOCTL_DIVP_SET_ATTR_DITYPE      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIVP_SET_ATTR_DITYPE_NR)
#define IOCTL_DIVP_SET_ATTR_ROTATE      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIVP_SET_ATTR_ROTATE_NR)
#define IOCTL_DIVP_SET_ATTR_WINDOW      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIVP_SET_ATTR_WINDOW_NR)
#define IOCTL_DIVP_SET_ATTR_MIRROR      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIVP_SET_ATTR_MIRROR_NR)
#define IOCTL_DIVP_SET_COVER      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIVP_SET_COVER_NR)


#endif //_MDRV_GFLIP_IO_H



