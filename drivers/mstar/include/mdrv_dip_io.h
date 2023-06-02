///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
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
///////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// @file   mdrv_sca_io.h
// @brief  GFlip KMD Driver Interface
// @author MStar Semiconductor Inc.
//////////////////////////////////////////////////////////////////////////////////////////////////

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
#define IOCTL_DIP_MAX_NR                      (4)


// use 'm' as magic number
#define IOCTL_DIP_MAGIC                   ('3')


#define IOCTL_DIP_SET_INTERRUPT             _IO(IOCTL_DIP_MAGIC,  IOCTL_DIP_SET_INTERRUP_NR)
#define IOCTL_DIP_SET_CONFIG                _IO(IOCTL_DIP_MAGIC,  IOCTL_DIP_SET_CONFIG_NR)
#define IOCTL_DIP_SET_WONCE_BASE_CONFIG     _IO(IOCTL_DIP_MAGIC,  IOCTL_DIP_SET_WONCE_BASE_CONFIG_NR)
#define IOCTL_DIP_GET_INTERRUPT_STATUS      _IO(IOCTL_DIP_MAGIC,  IOCTL_DIP_GET_INTERRUPT_STATUS_NR)


#endif //_MDRV_GFLIP_IO_H



